#include <string.h>
#include "camkes.h"
#include "OS_Dataport.h"

static OS_Dataport_t port = OS_DATAPORT_ASSIGN(entropy_port);

size_t entropy_rpc_read(const size_t len) {
  int sz = OS_Dataport_getSize(port);
  sz = len > sz ? sz : len;
  memset(OS_Dataport_getBuf(port), 0xff, sz);
  return sz;
}
