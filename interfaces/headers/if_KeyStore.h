#pragma once

#include "OS_Dataport.h"
#include <stdint.h>

#define CEK_SIZE	256
#define CSRK_SIZE	128

typedef struct {
  void (*getCEK_RSA2048)(uint32_t *exp);
  void (*getCSRK_RSA1024)(uint32_t *exp);
  OS_Dataport_t dataport;
} if_KeyStore_t;

#define IF_KEYSTORE_ASSIGN(_rpc_, _port_) \
{ \
  .getCEK_RSA2048	= _rpc_ ## _getCEK_RSA2048, \
  .getCSRK_RSA1024	= _rpc_ ## _getCSRK_RSA1024, \
  .dataport		= OS_DATAPORT_ASSIGN(_port_) \
}
