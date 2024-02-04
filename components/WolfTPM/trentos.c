#include <assert.h>
#include <string.h>
#include "camkes.h"
#include "lib_debug/Debug.h"
#include "OS_Dataport.h"

#include "if_KeyStore.h" /* for constants such as EK_SIZE and CSRK_SIZE */

#include "bcm2837/bcm2837_gpio.h"
#include "bcm2837/bcm2837_spi.h"

#include "wolftpm/tpm2_wrap.h"
#include "hal/tpm_io.h"

#define CHKRCV(a,b) if ((a) != TPM_RC_SUCCESS) {\
  Debug_LOG_ERROR(b); \
  exit(1); \
}
#define CHKRCI(a) if ((a) != TPM_RC_SUCCESS) {\
  Debug_LOG_ERROR(#a " failed!"); \
  return 1; \
}

static OS_Dataport_t entropyPort = OS_DATAPORT_ASSIGN(entropy_port);
static OS_Dataport_t keystorePort = OS_DATAPORT_ASSIGN(keystore_port);

/* Context required for wolfTPM */
WOLFTPM2_DEV dev = {0};
TPM2HalIoCb ioCb = TPM2_IoCb_TRENTOS_SPI;
WOLFTPM2_KEY ek = {0};
WOLFTPM2_KEY srk = {0};
WOLFTPM2_KEY csrk = {0};

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

  Debug_LOG_INFO("Clearing TPM");
  CHKRCV(wolfTPM2_Clear(&dev), "Failed to clear TPM!");
  Debug_LOG_INFO("TPM cleared successfully");

  Debug_LOG_INFO("Getting device info");
  WOLFTPM2_CAPS caps;
  
  /* Note: Info string copied from wolfTPM's examples/wrap/wrap_test.c */
  CHKRCV(wolfTPM2_GetCapabilities(&dev, &caps), "Failed to get device info!");
  Debug_LOG_INFO(
      "Mfg %s (%d), Vendor %s, Fw %u.%u (0x%x), "
      "FIPS 140-2 %d, CC-EAL4 %d\n",
      caps.mfgStr, caps.mfg, caps.vendorStr, caps.fwVerMajor,
      caps.fwVerMinor, caps.fwVerVendor, caps.fips140_2, caps.cc_eal4);

  Debug_LOG_INFO("Creating EK");
  CHKRCV(wolfTPM2_CreateEK(&dev, &ek, TPM_ALG_RSA), "Failed to create EK");
  Debug_LOG_INFO(
      "Created EK (%d bits)",
      ek.pub.publicArea.unique.rsa.size * 8);
  assert(ek.pub.publicArea.unique.rsa.size == EK_SIZE);

  /* TODO: This gets timeout. Why? */
  /* Create the **real** SRK, which is needed for the storage hiearchy.
   * Not the "cSRK", which, if I understood the assignment correctly, is
   * just a regular RSA key with a fancy name.
   */
  Debug_LOG_INFO("Creating Storage Root Key");
  CHKRCV(
      wolfTPM2_CreateSRK(&dev, &srk, TPM_ALG_RSA, NULL, 0),
      "Failed to create SRK!");
  Debug_LOG_INFO("Created Storage Root Key");

  /* Now, create the cSRK */
  /* TODO */
}

/* if_OS_Entropy */

size_t entropy_rpc_read(const size_t len) {
  size_t sz = OS_Dataport_getSize(entropyPort);
  sz = len > sz ? sz : len;
  wolfTPM2_GetRandom(&dev, OS_Dataport_getBuf(entropyPort), sz);
  return sz;
}

/* if_Keystore */

/* Places the EK into the dataport.
 * TODO: Do we also need to know exponent?
 */
void keystore_rpc_getEK_RSA2048(void) {
  memcpy(
      OS_Dataport_getBuf(keystorePort),
      ek.pub.publicArea.unique.rsa.buffer, EK_SIZE);
}

/* Places the cSRK into the dataport.
 * TODO: Do we also need to know exponent?
 */
void keystore_rpc_getCSRK_RSA1024(void) {
  /* TODO: At the moment, this is broken, because real SRK, and thus any
   * other keys (which require is as a parent), fails to generate */
  memcpy(
      OS_Dataport_getBuf(keystorePort),
      csrk.pub.publicArea.unique.rsa.buffer, CSRK_SIZE);
}

/*
int keystore_rpc_storeKey(int keyLen) {
}
*/

// loadKey()

/* if_Crypto */


/*
 * wolfTPM2_CreatePrimaryKey    to create parent of the key
 * wolfTPM2_GetKeyTemplate_RSA    to create template of the key
 */
