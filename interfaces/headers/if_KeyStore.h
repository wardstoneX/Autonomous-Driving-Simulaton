#pragma once

#include "OS_Dataport.h"
#include <stdint.h>

#define EK_SIZE 256

typedef struct {
  void (*getEK_RSA2048)(void);
  OS_Dataport_t dataport;
} if_KeyStore_t;

#define IF_KEYSTORE_ASSIGN(_rpc_, _port_) \
{ \
  .getEK_RSA2048	= _rpc_ ## _getEK_RSA2048, \
  .dataport		= OS_DATAPORT_ASSIGN(_port_) \
}
