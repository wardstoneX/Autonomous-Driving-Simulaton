#include <string.h>

#include "interfaces/if_OS_Entropy.h"
#include "if_KeyStore.h"

#include "lib_debug/Debug.h"
#include "camkes.h"

if_OS_Entropy_t entropy	= IF_OS_ENTROPY_ASSIGN(entropy_rpc, entropy_dp);
if_KeyStore_t keystore	= IF_KEYSTORE_ASSIGN(keystore_rpc, keystore_dp);

char ek[EK_SIZE];

int run(void) {
  char buf[128];
  int rd = entropy.read(80);
  memcpy(buf, OS_Dataport_getBuf(entropy.dataport), rd);
  Debug_LOG_DEBUG("Got %d bytes:", rd);
  Debug_DUMP_DEBUG(buf, rd);

  Debug_LOG_DEBUG("Reading EK.");
  keystore.getEK_RSA2048();
  memcpy(ek, OS_Dataport_getBuf(keystore.dataport), EK_SIZE);
  Debug_DUMP_DEBUG(ek, EK_SIZE);
  return 0;
}
