#pragma once

#include "OS_Dataport.h"
#include <stdint.h>

enum if_Crypto_Key {
  IF_CRYPTO_KEY_CEK,
  IF_CRYPTO_KEY_CSRK
};

typedef struct {
  int (*decrypt_RSA_OAEP)(int key, int *len);
  int (*encrypt_RSA_OAEP)(int key, int *len);
  OS_Dataport_t dataport;
} if_Crypto_t;

#define IF_CRYPTO_ASSIGN(_rpc_, _port_)         \
{                                               \
  .decrypt_RSA_OAEP	= _rpc_##_decrypt_RSA_OAEP, \
  .encrypt_RSA_OAEP	= _rpc_##_encrypt_RSA_OAEP, \
  .dataport		= OS_DATAPORT_ASSIGN(_port_)      \
}
