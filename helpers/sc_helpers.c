#include <string.h>
#include "stdint.h"

/*
This function returns an OpenSSH-formated RSA public key as specified in standard RFC 4253

e is a pointer to the bytes of the exponent
eLen is the length of the exponent in bytes
n is a pointer to the bytes of the key
nLen is the length of the key in bytes
buf is a pointer to a buffer where the formated key will be written
bufLen is the length of the buffer
*/
int toOpenSSHPublicRSA(uint8_t* e, uint32_t eLen, uint8_t* n, uint32_t nLen, uint8_t* buf, uint32_t bufLen){
    uint8_t* ptr = buf;
    uint8_t rsa_tag[] = {0x73, 0x73, 0x68, 0x2d, 0x72, 0x73, 0x61};

    if(7+4+eLen+4+nLen > bufLen) {
        //the buffer was not big enough
        return -1;
    }

    memmove(ptr, rsa_tag, 7);
    ptr += 7;

    memmove(ptr, &eLen, 4);
    ptr += 4;
    memmove(ptr, e, eLen);
    ptr += eLen;

    memmove(ptr, &nLen, 4);
    ptr += 4;
    memmove(ptr, n, nLen);
    ptr += nLen;

    return ptr - buf;
}