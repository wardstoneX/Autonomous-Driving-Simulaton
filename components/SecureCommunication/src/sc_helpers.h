#include "stdint.h"
#include "stddef.h"
#include "interfaces/if_OS_Socket.h"
#include <camkes.h>
#include "OS_Socket.h"
#include "OS_Crypto.h"

uint32_t formatForPython(uint32_t e, uint32_t nLen, uint8_t* n, uint32_t bufLen, uint8_t* buf);
uint32_t encrypt_AES_GCM(OS_Crypto_Handle_t hCrypto, uint8_t* keyBytes, uint8_t* plaintext, size_t plaintextLen, uint8_t* iv, uint8_t* payload, size_t payloadLen);
uint32_t decrypt(OS_Crypto_Handle_t hCrypto, uint8_t* keyBytes, uint8_t* ciphertext, size_t ciphertextLen, uint8_t* plaintext, size_t plaintextLen);

seL4_Word secureCommunication_rpc_get_sender_id(void);

#define CHECK_IS_RUNNING(_currentState_)                                       \
    do                                                                         \
    {                                                                          \
        if (_currentState_ != RUNNING)                                         \
        {                                                                      \
            if (_currentState_ == FATAL_ERROR)                                 \
            {                                                                  \
                Debug_LOG_ERROR("%s: FATAL_ERROR occurred in the NetworkStack" \
                                , __func__);                                   \
                return OS_ERROR_ABORTED;                                       \
            }                                                                  \
            else                                                               \
            {                                                                  \
                Debug_LOG_TRACE("%s: NetworkStack currently not running",      \
                __func__);                                                     \
               return OS_ERROR_NOT_INITIALIZED;                                \
            }                                                                  \
        }                                                                      \
    } while (0)
