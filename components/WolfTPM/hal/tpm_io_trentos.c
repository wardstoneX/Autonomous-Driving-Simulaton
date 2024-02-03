#include <assert.h>
#include <string.h>
#include "camkes.h"
#include "lib_debug/Debug.h"

#include "bcm2837/bcm2837_gpio.h"
#include "bcm2837/bcm2837_spi.h"
#include "tpm_io.h"

int TPM2_IoCb_TRENTOS_SPI(TPM2_CTX *ctx, const byte *txBuf, byte *rxBuf,
    word16 xferSz, void *userCtx) {
  bcm2837_spi_transfernb((char*) txBuf, (char*) rxBuf, xferSz);
  return TPM_RC_SUCCESS;
}
