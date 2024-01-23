#include <assert.h>
#include <string.h>
#include "camkes.h"
#include "lib_debug/Debug.h"
#include "OS_Dataport.h"

#include "TPM_if.h"

static OS_Dataport_t port = OS_DATAPORT_ASSIGN(entropy_port);

TPM_RC TPM_Startup(void) {
  TPM2_Packet packet;
  return TPM_SendCommandU16(TPM_CC_Startup, TPM_SU_CLEAR, &packet);
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
  size_t sz = OS_Dataport_getSize(port), pos = 0;
  sz = len > sz ? sz : len;

  TPM2_Packet packet;
  uint16_t actual;
  TPM_RC rc;
  while (pos < sz) {
    rc = TPM_SendCommandU16(TPM_CC_GetRandom, sz - pos, &packet);
    if (rc != TPM_RC_SUCCESS) {
      Debug_LOG_ERROR("Failed to get entropy: error code %x", rc);
      return pos;
    }
    /* Contents of returned packet:
     *  2 bytes size of data
     *  then the data itself
     */
    TPM2_Packet_ParseU16(&packet, &actual);
    Debug_LOG_DEBUG("Requested %d bytes of entropy, got %d", sz - pos, actual);
    memcpy(OS_Dataport_getBuf(port) + pos,
	   packet.buf + packet.pos,
	   actual);
    pos += actual;
  }

  return sz;
}
