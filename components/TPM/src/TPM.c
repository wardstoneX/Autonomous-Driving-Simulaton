#include <string.h>
#include "camkes.h"
#include "lib_debug/Debug.h"
#include "OS_Dataport.h"

#include "TPM_if.h"

static OS_Dataport_t port = OS_DATAPORT_ASSIGN(entropy_port);

TPM_RC TPM_Startup(void) {
  return TPM_SendCommandU16(TPM_CC_Startup, TPM_SU_CLEAR);
}

/* Initialization that must be done before initializing 
 * the specific interfaces
 *
 * TODO: TRENTOS handbook page 26 says pre_init must not block?
 */
void pre_init(void) {
  Debug_LOG_DEBUG("Initializing TPM");
  if (TPM_Init() || (TPM_Startup() != TPM_RC_SUCCESS)) {
    Debug_LOG_ERROR("TPM initialization failed!");
  } else {
    Debug_LOG_INFO("TPM initialized successfully.");
  }
}

size_t entropy_rpc_read(const size_t len) {
  int sz = OS_Dataport_getSize(port);
  sz = len > sz ? sz : len;
  memset(OS_Dataport_getBuf(port), 0xff, sz);
  return sz;
}
