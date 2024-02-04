#pragma once

/* TODO: Can I somehow specify in CAmkES that "key" isn't an integer, it's
 *       an enum of type enum if_Crypto_Key (see headers/if_Crypto.h)? */
#define IF_CRYPTO_CAMKES \
  int decrypt_RSA_OAEP(in int key, inout int pLen); \
  int encrypt_RSA_OAEP(in int key, inout int pLen);
