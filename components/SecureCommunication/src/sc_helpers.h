#include "stdint.h"
#include "stddef.h"
#include "interfaces/if_OS_Socket.h"
#include <camkes.h>
#include "OS_Socket.h"
#include "OS_Crypto.h"

uint32_t formatForPython(uint32_t e, uint32_t nLen, uint8_t* n, uint32_t bufLen, uint8_t* buf);

/*
This function encrypts a message using the provided key and IV.
The IV is appended as plaintext to the beginning of the encrypted text.
@param  hCrypto         A previously initialized OS_Crypto_Handle_t object.
@param  keyBytes        Pointer to 32 bytes of data to be used as a key.
@param  plaintext       Pointer to a buffer where the text to be encrypted is.
@param  plaintextLen    Length in bytes of the plaintext buffer.
@param  iv              Pointer to 12 bytes of data to be used as an initialization vector.
@param  payload         Pointer to a buffer where the resulting payload (iv + ciphertext) will be writen.
@param  payloadLen      Size in bytes of the payload buffer.
*/
uint32_t encrypt_AES_GCM(OS_Crypto_Handle_t hCrypto, uint8_t* keyBytes, uint8_t* plaintext, size_t plaintextLen, uint8_t* iv, uint8_t* payload, size_t payloadLen);

/*
This function decrypts a message with AES256 in GCM mode.
The first 12 bytes of data passed to this function will be used as the required initialization vector.
@param  hCrypto         A previously initialized OS_Crypto_Handle_t object.
@param  keyBytes        Pointer to 32 bytes of data to be used as a key.
@param  ciphertext      Pointer to the data that is to be decrypted (including 12 bytes of IV).
@param  ciphertextLen   Length in bytes of the ciphertext (IV included).
@param  plaintext       Pointer to a buffer where the decrypted message will be written.
@param  plaintextLen    Size of the plaintext buffer.
*/
uint32_t decrypt_AES_GCM(OS_Crypto_Handle_t hCrypto, uint8_t* keyBytes, uint8_t* ciphertext, size_t ciphertextLen, uint8_t* plaintext, size_t plaintextLen);

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
