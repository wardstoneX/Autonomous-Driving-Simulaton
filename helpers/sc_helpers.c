#include <string.h>
#include "stdint.h"
#include "lib_debug/Debug.h"
#include "sc_helpers.h"
#include "OS_Crypto.h"


uint32_t formatForPython(uint32_t e, uint32_t nLen, uint8_t* n, uint32_t bufLen, uint8_t* buf){
    uint8_t* pointer = buf;

    if(bufLen < 4+4+nLen) {
        Debug_LOG_ERROR("The buffer is too small!");
    }

    memcpy(pointer, &e, 4);
    pointer += 4;
    memcpy(pointer, &nLen, 4);
    pointer += 4;
    memcpy(pointer, n, nLen);
    pointer += nLen;

    return pointer - buf;
}

uint32_t encrypt_AES_GCM(OS_Crypto_Handle_t hCrypto, uint8_t* keyBytes, uint8_t* plaintext, size_t plaintextLen, uint8_t* iv, uint8_t* payload, size_t payloadLen){
    if(payloadLen < 12 + plaintextLen) {
        Debug_LOG_ERROR("The provided buffer is too small: it is %d bytes, has to be at least %d bytes", payloadLen, 12+plaintextLen);
        return -1;
    }

    memcpy(payload, iv, 12);

    OS_CryptoKey_Handle_t hKey;
    //uint8_t keyArray[32];
    //memcpy(keyArray, keyBytes, 32);
    static const OS_CryptoKey_Data_t aes256Data =
    {
        .type = OS_CryptoKey_TYPE_AES,
        .attribs.keepLocal = true,
        //.data.aes.bytes = keyBytes,
        .data.aes.len    = 32
    };
    memcpy((uint8_t*) aes256Data.data.aes.bytes, keyBytes, 32);
    Debug_LOG_INFO("DUMPING RECEIVED KEYBYTES");
    Debug_DUMP_INFO(keyBytes, 32);
    Debug_LOG_INFO("DUMPING LOADED KEYBYTES");
    Debug_DUMP_INFO(aes256Data.data.aes.bytes, 32);

    if(OS_CryptoKey_import(&hKey, hCrypto, &aes256Data) != OS_SUCCESS) {
        Debug_LOG_ERROR("There was an error importing the key");
        return -1;
    }

    OS_CryptoCipher_Handle_t hCipher;
    OS_CryptoCipher_init(&hCipher,
                        hCrypto,
                        hKey,
                        OS_CryptoCipher_ALG_AES_GCM_ENC,
                        iv,
                        12);
    OS_CryptoCipher_start(hCipher, NULL, 0);
    size_t outputSize = payloadLen - 12;
    OS_CryptoCipher_process(hCipher, plaintext, plaintextLen, payload + 12, &outputSize);

    if(outputSize != plaintextLen) {
        Debug_LOG_ERROR("Only %d of %d bytes were encrypted successfully", outputSize, plaintextLen);
        return -1;
    }
    Debug_LOG_INFO("DUMPING PAYLOAD");
    Debug_DUMP_INFO(payload, 12+plaintextLen);

    return 0;
}

uint32_t decrypt(OS_Crypto_Handle_t hCrypto, uint8_t* keyBytes, uint8_t* ciphertext, size_t ciphertextLen, uint8_t* plaintext, size_t plaintextLen){
    if(plaintextLen < ciphertextLen - 12) {
        Debug_LOG_ERROR("The provided buffer is too small: it is %d bytes, has to be at least %d bytes", plaintextLen, ciphertextLen - 12);
        return -1;
    }

    uint8_t iv[12];
    memcpy(iv, ciphertext, 12);

    OS_CryptoKey_Handle_t hKey;
    static const OS_CryptoKey_Data_t aes256Data =
    {
        .type = OS_CryptoKey_TYPE_AES,
        .attribs.keepLocal = true,
        .data.aes.len    = 32
    };
    memcpy((uint8_t*) aes256Data.data.aes.bytes, keyBytes, 32);
    Debug_LOG_INFO("DUMPING RECEIVED KEYBYTES");
    Debug_DUMP_INFO(keyBytes, 32);
    Debug_LOG_INFO("DUMPING LOADED KEYBYTES");
    Debug_DUMP_INFO(aes256Data.data.aes.bytes, 32);

    if(OS_CryptoKey_import(&hKey, hCrypto, &aes256Data) != OS_SUCCESS) {
        Debug_LOG_ERROR("There was an error importing the key");
        return -1;
    }

    OS_CryptoCipher_Handle_t hCipher;
    OS_CryptoCipher_init(&hCipher,
                        hCrypto,
                        hKey,
                        OS_CryptoCipher_ALG_AES_GCM_DEC,
                        iv,
                        12);
    OS_CryptoCipher_start(hCipher, NULL, 0);
    size_t outputSize = ciphertextLen - 12;
    OS_CryptoCipher_process(hCipher, ciphertext+12, ciphertextLen-12, plaintext, &outputSize);

    if(outputSize != ciphertextLen - 12) {
        Debug_LOG_ERROR("Only %d of %d bytes were encrypted successfully", outputSize, ciphertextLen - 12);
        return -1;
    }
    Debug_LOG_INFO("DUMPING PLAINTEXT");
    Debug_DUMP_INFO(plaintext, ciphertextLen - 12);

    return 0;
}