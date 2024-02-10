#include <assert.h>
#include <string.h>
#include "camkes.h"
#include "lib_debug/Debug.h"
#include "OS_Dataport.h"

#include "if_KeyStore.h" /* for constants such as CEK_SIZE and CSRK_SIZE */
#include "if_Crypto.h"   /* for enum if_Crypto_Key */

#include "bcm2837/bcm2837_gpio.h"
#include "bcm2837/bcm2837_spi.h"

#include "wolftpm/tpm2_wrap.h"
#include "hal/tpm_io.h"

#define CHKRCV(a,b) if ((a) != TPM_RC_SUCCESS) {\
  Debug_LOG_ERROR(b); \
  exit(1); \
}
#define CHKRCI(a,b) if ((a) != TPM_RC_SUCCESS) {\
  Debug_LOG_ERROR(#a " failed!"); \
  return b; \
}

/* One NV index can be at most 2048 bytes large on this TPM
 * Somehow, a WOLFTPM_KEYBLOB structure is ~2300 bytes
 * Thus, we need two NV indices to store the EK...
 */
#define NV_MAX_SIZE 2048
#define NV_INDEX_1 TPM_20_OWNER_NV_SPACE
#define NV_INDEX_2 NV_INDEX_1 + NV_MAX_SIZE


static OS_Dataport_t entropyPort = OS_DATAPORT_ASSIGN(entropy_port);
static OS_Dataport_t keystorePort = OS_DATAPORT_ASSIGN(keystore_port);
static OS_Dataport_t cryptoPort = OS_DATAPORT_ASSIGN(crypto_port);

/* Context required for wolfTPM */
WOLFTPM2_DEV dev  = {0};
TPM2HalIoCb ioCb  = TPM2_IoCb_TRENTOS_SPI;
WOLFTPM2_KEY srk  = {0};
WOLFTPM2_KEYBLOB cek  = {0};
WOLFTPM2_KEY csrk = {0};
WOLFTPM2_NV nv1	  = {0};
WOLFTPM2_NV nv2	  = {0};
uint32_t nv_off   = sizeof(cek) - NV_MAX_SIZE;

/* Declarations of local helper functions */
void create_rsa_key(WOLFTPM2_KEYBLOB *key, uint32_t bits);
void create_and_load_rsa_key(WOLFTPM2_KEY *key, uint32_t bits);
void first_setup(void);

/* Initialization that must be done before initializing 
 * the specific interfaces
 *
 * TODO: TRENTOS handbook page 26 says pre_init must not block?
 */
void pre_init(void) {
  Debug_LOG_INFO("Initializing SPI for TPM");
  /* TRENTOS SPI library initialization */
  if (!bcm2837_spi_begin(regBase)) {
    Debug_LOG_ERROR("Failed to initialize SPI!");
    return;
  }
  bcm2837_spi_setBitOrder(BCM2837_SPI_BIT_ORDER_MSBFIRST);
  bcm2837_spi_setDataMode(BCM2837_SPI_MODE0);
  /* Infineon Optiga SLB 9670 TPM has a maximum frequency of 38-45 MHz,
   * depending on "t_SLEW" (see datasheet). Here, we use 31.25 MHz because
   * that's the closest value the SPI library supports.
   * Note that the Raspberry Pi is running at 250 MHz, not 400 MHz, because of
   * settings in config.txt!
   */
  bcm2837_spi_setClockDivider(BCM2837_SPI_CLOCK_DIVIDER_8);
  bcm2837_spi_setChipSelectPolarity(BCM2837_SPI_CS1, 0);
  bcm2837_spi_chipSelect(BCM2837_SPI_CS1);

  /* Pin 18 (GPIO 24) needs to be high */
  bcm2837_gpio_fsel(RPI_GPIO_P1_18, 1); /* set as output pin */
  bcm2837_gpio_set(RPI_GPIO_P1_18); /* set high */
  Debug_LOG_INFO("SPI initialized successfully");

  Debug_LOG_INFO("Sending TPM initialization commands");
  CHKRCV(wolfTPM2_Init(&dev, ioCb, NULL), "Failed to initialize TPM!");
  Debug_LOG_INFO("TPM initialized successfully");


/* ENABLE THIS TO CLEAR THE TPM AND GENERATE NEW PERSISTENT CEK */
#ifdef CLEAR_TPM
  Debug_LOG_INFO("Clearing TPM");
  CHKRCV(wolfTPM2_Clear(&dev), "Failed to clear TPM!");
  Debug_LOG_INFO("TPM cleared successfully");
#endif


  /* TODO: After increasing timeout, it works, but why does it take so long? */
  /* Create the **real** SRK, which is needed for the storage hiearchy.
   * Not the "cSRK", which, if I understood the assignment correctly, is
   * just a regular RSA key with a fancy name.
   */
  Debug_LOG_INFO("Creating Storage Root Key");
  CHKRCV(
      wolfTPM2_CreateSRK(&dev, &srk, TPM_ALG_RSA, NULL, 0),
      "Failed to create SRK!");
  Debug_LOG_INFO("Created Storage Root Key");

  Debug_LOG_INFO("Trying to open NV Storage 1...");
  if (wolfTPM2_NVOpen(&dev, &nv1, NV_INDEX_1, NULL, 0) != TPM_RC_SUCCESS) {
    Debug_LOG_ERROR("Opening NV Storage failed.");
    first_setup();
    Debug_LOG_ERROR("NEW cEK GENERATED. IMPORT INTO CLIENT:");
    for (int i = 0; i < CEK_SIZE; i++)
      printf("%02x ", cek.pub.publicArea.unique.rsa.buffer[i]);
    printf("\n");
    exit(1);
  }
  Debug_LOG_INFO("Trying to open NV Storage 2...");
  CHKRCV(wolfTPM2_NVOpen(&dev, &nv2, NV_INDEX_2, NULL, 0),
      "Opening NV storage 2 failed!");
  Debug_LOG_INFO("Opened NV storage.");

  Debug_LOG_INFO("Reading cEK...");
  uint32_t sz = NV_MAX_SIZE;
  CHKRCV(wolfTPM2_NVReadAuth(&dev, &nv1, NV_INDEX_1,
	(byte*) &cek, &sz, 0),
      "Failed to read cEK!");
  assert(sz == NV_MAX_SIZE);
  sz = sizeof(cek) - NV_MAX_SIZE;
  CHKRCV(wolfTPM2_NVReadAuth(&dev, &nv2, NV_INDEX_2,
	((byte*) &cek) + NV_MAX_SIZE, &sz, 0),
      "Failed to read cEK!");
  assert(sz == sizeof(cek) - NV_MAX_SIZE);
  Debug_LOG_INFO("Read cEK.");
  Debug_LOG_DEBUG("cEK is:");
  Debug_DUMP_DEBUG(cek.pub.publicArea.unique.rsa.buffer, CEK_SIZE);

  Debug_LOG_INFO("Loading cEK into TPM...");
  CHKRCV(wolfTPM2_LoadKey(&dev, &cek, &srk.handle), "Failed to load cEK!");
  Debug_LOG_INFO("Loaded cEK into TPM.");

  Debug_LOG_INFO("Generating cSRK...");
  create_and_load_rsa_key(&csrk, CSRK_SIZE * 8);
  Debug_LOG_INFO("Generated cSRK.");

  Debug_LOG_INFO("TPM initialized succesfully!");
}

/* Helper functions */
void first_setup(void) {
  /* Create the cEK */
  Debug_LOG_INFO("Creating cEK");
  create_rsa_key(&cek, CEK_SIZE * 8);
  Debug_LOG_INFO("Created cEK (%d bits)",
                 cek.pub.publicArea.unique.rsa.size * 8);
  assert(cek.pub.publicArea.unique.rsa.size == CEK_SIZE);

  /* Create two new NV indices, because one is not enough for
   * storing a key of size 2324 bytes. */
  Debug_LOG_INFO("Creating new NV indices");
  word32 nvAttr = 0;
  WOLFTPM2_HANDLE nvParent = { .hndl = TPM_RH_OWNER };
  wolfTPM2_GetNvAttributesTemplate(TPM_RH_OWNER, &nvAttr);
  CHKRCV(
      wolfTPM2_NVCreateAuth(&dev, &nvParent, &nv1, NV_INDEX_1,
	nvAttr, NV_MAX_SIZE, NULL, 0),
      "Failed to create NV index!");
  CHKRCV(
      wolfTPM2_NVCreateAuth(&dev, &nvParent, &nv2, NV_INDEX_2,
	nvAttr, NV_MAX_SIZE, NULL, 0),
      "Failed to create NV index!");

  /* Store cEK and cSRK there */
  Debug_LOG_INFO("Size of cEK is %d", sizeof(cek));
  CHKRCV(
      wolfTPM2_NVWriteAuth(&dev, &nv1, NV_INDEX_1,
	(byte*) &cek, NV_MAX_SIZE, 0),
      "Failed to save cEK in NV!");
  CHKRCV(
      wolfTPM2_NVWriteAuth(&dev, &nv2, NV_INDEX_2,
	((byte*) &cek) + NV_MAX_SIZE, sizeof(cek) - NV_MAX_SIZE, 0),
      "Failed to save cEK in NV!");
}

void create_rsa_key(WOLFTPM2_KEYBLOB *key, uint32_t bits) {
  TPMT_PUBLIC tmpt;
  wolfTPM2_GetKeyTemplate_RSA(
      &tmpt,
      TPMA_OBJECT_sensitiveDataOrigin
      | TPMA_OBJECT_userWithAuth
      | TPMA_OBJECT_decrypt);
  tmpt.parameters.rsaDetail.keyBits = bits;
  CHKRCV(
      wolfTPM2_CreateKey(&dev, key, &srk.handle, &tmpt, NULL, 0),
      "Failed to create key!");
}

void create_and_load_rsa_key(WOLFTPM2_KEY *key, uint32_t bits) {
  TPMT_PUBLIC tmpt;
  wolfTPM2_GetKeyTemplate_RSA(
      &tmpt,
      TPMA_OBJECT_sensitiveDataOrigin
      | TPMA_OBJECT_userWithAuth
      | TPMA_OBJECT_decrypt);
  tmpt.parameters.rsaDetail.keyBits = bits;
  CHKRCV(
      wolfTPM2_CreateAndLoadKey(&dev, key, &srk.handle, &tmpt, NULL, 0),
      "Failed to create key!");
}

WOLFTPM2_KEY *getKey(enum if_Crypto_Key key) {
  switch (key) {
    case IF_CRYPTO_KEY_CEK:
      return (WOLFTPM2_KEY*) &cek;
    case IF_CRYPTO_KEY_CSRK:
      return &csrk;
    default:
      Debug_LOG_ERROR("Unknown key %d!", key);
      return NULL;
  }
}

/* if_OS_Entropy */

size_t entropy_rpc_read(const size_t len) {
  size_t sz = OS_Dataport_getSize(entropyPort);
  sz = len > sz ? sz : len;
  wolfTPM2_GetRandom(&dev, OS_Dataport_getBuf(entropyPort), sz);
  return sz;
}

/* if_Keystore */

/* Places the cEK into the dataport. */
void keystore_rpc_getCEK_RSA2048(uint32_t *exp) {
  *exp = cek.pub.publicArea.parameters.rsaDetail.exponent;
  memcpy(
      OS_Dataport_getBuf(keystorePort),
      cek.pub.publicArea.unique.rsa.buffer, CEK_SIZE);
  Debug_DUMP_INFO(OS_Dataport_getBuf(keystorePort), CEK_SIZE);
}

/* Places the cSRK into the dataport. */
void keystore_rpc_getCSRK_RSA1024(uint32_t *exp) {
  *exp = csrk.pub.publicArea.parameters.rsaDetail.exponent;
  memcpy(
      OS_Dataport_getBuf(keystorePort),
      csrk.pub.publicArea.unique.rsa.buffer, CSRK_SIZE);
  Debug_DUMP_INFO(OS_Dataport_getBuf(keystorePort), CSRK_SIZE);
}

/* Stores key in NV memory.
 * Returns a handle in case of success, or -1 in case of failure.
 * Expects the raw key data on the dataport.
 */
uint32_t keystore_rpc_storeKey(uint32_t keyLen) {
  uint32_t start = nv_off;
  CHKRCI(wolfTPM2_NVWriteAuth(&dev, &nv2, NV_INDEX_2,
	                      (byte*) &keyLen, sizeof(uint32_t), nv_off), -1);
  nv_off += sizeof(uint32_t);

  CHKRCI(wolfTPM2_NVWriteAuth(&dev, &nv2, NV_INDEX_2,
	                      OS_Dataport_getBuf(keystorePort),
			      keyLen, nv_off), -1);
  nv_off += keyLen;
  return start;
}

/* Reads key from NV memory. Takes a handle (which actually is an offset...)
 * supplied by storeKey() as argument.
 * Returns 0 in case of success, or non-zero in case of failure.
 * Expects the raw key data on the dataport.
 */
int keystore_rpc_loadKey(uint32_t hdl, uint32_t *keyLen) {
  /* Read length of key */
  uint32_t sz = sizeof(uint32_t);
  uint32_t off = hdl;
  CHKRCI(wolfTPM2_NVReadAuth(&dev, &nv2, NV_INDEX_2,
	                    (byte*) keyLen, &sz, off), 1);
  assert(sz == sizeof(keyLen));
  off += sz;
  /* Read key data */
  sz = *keyLen;
  CHKRCI(wolfTPM2_NVReadAuth(&dev, &nv2, NV_INDEX_2,
	                     OS_Dataport_getBuf(keystorePort), &sz, off), 1);
  assert(sz == *keyLen);
  return 0;
}

/* if_Crypto */

/* Decrypt a message that was encrypted with RSA/OAEP.
 *
 * key is actually of type enum if_Crypto_Key.
 * TODO: How to specify an enum in an CAmkES interface file?
 *
 * *len should be the length of the ciphertext. After the function returns,
 * it will be set to the length of the decrypted text.
 *
 * Input of ciphertext and output of decrypted text happen over the dataport.
 */
int crypto_rpc_decrypt_RSA_OAEP(int key, int *len) {
  WOLFTPM2_KEY *pKey = getKey(key);

  /* Note: Looking at wolfTPM2_RsaDecrypt source code, it appears that they
   * copy the ciphertext into an internal buffer, perform operations on that,
   * and finally copy the result to the output buffer.
   * So it shouldn't be a problem that input and output pointer are the same.
   */
  CHKRCI(
      wolfTPM2_RsaDecrypt(&dev, pKey, TPM_ALG_OAEP,
	                  OS_Dataport_getBuf(cryptoPort), *len,
			  OS_Dataport_getBuf(cryptoPort), len),
      1);
  return 0;
}

/* Encrypt a message with RSA/OAEP.
 *
 * key is actually of type enum if_Crypto_Key.
 * TODO: How to specify an enum in an CAmkES interface file?
 *
 * *len should be the length of the message. After the function returns,
 * it will be set to the length of the encrypted text.
 *
 * Input of message and output of ciphertext happen over the dataport.
 */
int crypto_rpc_encrypt_RSA_OAEP(int key, int *len) {
  WOLFTPM2_KEY *pKey = getKey(key);

  /* Note: Similar to decrypt_RSA_OAEP(), I assumte that it's fine that input
   * and output buffer are the same.
   */
  CHKRCI(
      wolfTPM2_RsaEncrypt(&dev, pKey, TPM_ALG_OAEP,
	                  OS_Dataport_getBuf(cryptoPort), *len,
			  OS_Dataport_getBuf(cryptoPort), len),
      1);
  return 0;
}
