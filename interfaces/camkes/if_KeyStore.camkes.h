#pragma once

/* TODO: Why CAmkES refuses to compile with uint32_t*? And why is it
 * OK that I replace it with the non-pointer type...? */
#define IF_KEYSTORE_CAMKES \
  void getCEK_RSA2048(out uint32_t pExp); \
  void getCSRK_RSA1024(out uint32_t pExp);
