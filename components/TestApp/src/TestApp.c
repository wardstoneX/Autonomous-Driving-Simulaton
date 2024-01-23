#include <string.h>

#include "interfaces/if_OS_Entropy.h"
#include "lib_debug/Debug.h"
#include "camkes.h"

if_OS_Entropy_t entropy = IF_OS_ENTROPY_ASSIGN(entropy_rpc, entropy_dp);

int run(void) {
  char buf[128];
  int rd = entropy.read(80);
  memcpy(buf, OS_Dataport_getBuf(entropy.dataport), rd);
  Debug_LOG_DEBUG("Got %d bytes:", rd);
  Debug_DUMP_DEBUG(buf, rd);
  return 0;
}
