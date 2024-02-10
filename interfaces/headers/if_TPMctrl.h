#pragma once

#include "OS_Dataport.h"
#include <stdint.h>

typedef struct {
  void (*shutdown)(void);
} if_TPMctrl_t;

#define IF_TPMCTRL_ASSIGN(_rpc_)        \
{                                       \
  .shutdown	= _rpc_##_shutdown, \
}
