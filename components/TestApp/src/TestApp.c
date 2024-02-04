#include <string.h>

#include "interfaces/if_OS_Entropy.h"
#include "if_KeyStore.h"
#include "if_Crypto.h"

#include "lib_debug/Debug.h"
#include "camkes.h"

if_OS_Entropy_t entropy	= IF_OS_ENTROPY_ASSIGN(entropy_rpc, entropy_dp);
if_KeyStore_t keystore	= IF_KEYSTORE_ASSIGN(keystore_rpc, keystore_dp);
if_Crypto_t crypto	= IF_CRYPTO_ASSIGN(crypto_rpc, crypto_dp);

char cek[CEK_SIZE];
char csrk[CSRK_SIZE];

int run(void) {

  /* Entropy */

  char buf[128];
  int rd = entropy.read(80);
  memcpy(buf, OS_Dataport_getBuf(entropy.dataport), rd);
  Debug_LOG_DEBUG("Got %d bytes:", rd);
  Debug_DUMP_DEBUG(buf, rd);

  /* Getting cEK/cSRK */

  uint32_t exp;
  Debug_LOG_DEBUG("Reading cEK.");
  keystore.getCEK_RSA2048(&exp);
  memcpy(cek, OS_Dataport_getBuf(keystore.dataport), CEK_SIZE);
  Debug_LOG_DEBUG("Got cEK (exponent %d)", exp);
  Debug_DUMP_DEBUG(cek, CEK_SIZE);

  Debug_LOG_DEBUG("Reading cSRK.");
  keystore.getCSRK_RSA1024(&exp);
  memcpy(csrk, OS_Dataport_getBuf(keystore.dataport), CSRK_SIZE);
  Debug_LOG_DEBUG("Got cSRK (exponent %d)", exp);
  Debug_DUMP_DEBUG(csrk, CSRK_SIZE);

  /* NV Storage */

  Debug_LOG_DEBUG("Storing some data in NV.");
  memset(OS_Dataport_getBuf(keystore.dataport), 0xAA, 128);
  uint32_t hdl = keystore.storeKey(128, 1234);
  if (hdl == -1) {
    Debug_LOG_ERROR("Failed to store the data!");
    return 1;
  }
  Debug_LOG_DEBUG("Got handle %d", hdl);

  Debug_LOG_DEBUG("Reading back the data.");
  /* Zero everything to make sure that we are actually reading fresh data. */
  memset(OS_Dataport_getBuf(keystore.dataport), 0x00, 128);
  uint32_t kLen = 0, kExp = 0;
  if (keystore.loadKey(hdl, &kLen, &kExp)) {
    Debug_LOG_ERROR("Failed to read key!");
    return 1;
  }
  Debug_LOG_DEBUG("Read back: length %d, exp %d", kLen, kExp);
  Debug_DUMP_DEBUG(OS_Dataport_getBuf(keystore.dataport), kLen);

  /* Encryption/Decryption */
  int len = 32;
  memset(OS_Dataport_getBuf(crypto.dataport), 0xCC, len);
  Debug_LOG_DEBUG("Message for encryption:");
  Debug_DUMP_DEBUG(OS_Dataport_getBuf(crypto.dataport), len);

  Debug_LOG_DEBUG("Encrypting: encrypt(cEK, encrypt(cSRK, data))");
  if (crypto.encrypt_RSA_OAEP(IF_CRYPTO_KEY_CSRK, &len)) return 1;
  if (crypto.encrypt_RSA_OAEP(IF_CRYPTO_KEY_CEK, &len)) return 1;
  Debug_LOG_DEBUG("Got %d bytes of encrypted data:", len);
  Debug_DUMP_DEBUG(OS_Dataport_getBuf(crypto.dataport), len);

  Debug_LOG_DEBUG("Decrypting: decrypt(cSRK, decrypt(cEK, ciphertext)");
  if (crypto.decrypt_RSA_OAEP(IF_CRYPTO_KEY_CEK, &len)) return 1;
  if (crypto.decrypt_RSA_OAEP(IF_CRYPTO_KEY_CSRK, &len)) return 1;
  Debug_LOG_DEBUG("Got %d bytes of decrypted data:", len);
  Debug_DUMP_DEBUG(OS_Dataport_getBuf(crypto.dataport), len);

  return 0;
}
