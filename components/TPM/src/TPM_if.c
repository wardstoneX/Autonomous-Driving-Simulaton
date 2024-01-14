#include <string.h>
#include "camkes.h"
#include "lib_debug/Debug.h"

#include "bcm2837/bcm2837_gpio.h"
#include "bcm2837/bcm2837_spi.h"
#include "TPM_defs.h"

/* To some extent, adapted from TPM2_TIS_READ in wolfTPM's tpm2_tis.c.
 * I have added explanations as to what all these bit operations mean
 * according to the TPM Interface Specification.
 */
void TPM_Read(uint32_t addr, char *buf, unsigned len) {
  char txBuf[MAX_SPI_FRAMESIZE+TPM_HEADER_SIZE] = {0};
//  char rxBuf[MAX_SPI_FRAMESIZE+TPM_HEADER_SIZE] = {0};
  Debug_LOG_DEBUG("TPM Read of size %d at addr %p", len, (void*) addr);

  /* First byte:
   *  Bit 7: read => 1, write => 0
   *  Bit 6: rsvd (TODO: what is that?)
   *  Bit 5-0: size of xfer - 1
   */
  txBuf[0] = TPM_READ | ((len & 0xFF) - 1);
  /* Next 4 bytes: address, in big endian */
  txBuf[1] = (addr>>16) & 0xFF;
  txBuf[2] = (addr>>8)  & 0xFF;
  txBuf[3] = (addr)     & 0xFF;

  /* Now perform the actual SPI transfer */
  /* TODO: Do we need to check wait states? I remember some comment claiming
   * that the Infineon devices guarantee no wait state, but not sure... */
  bcm2837_spi_transfernb(txBuf, buf, len);
  /* TODO: wolfTPM cuts off the first TPM_HEADER_SIZE bytes. Why? */
}

void TPM_Write(uint32_t addr, char *buf, unsigned len) {
  char txBuf[MAX_SPI_FRAMESIZE+TPM_HEADER_SIZE] = {0};

  /* First byte:
   *  Bit 7: read => 1, write => 0
   *  Bit 6: rsvd (TODO: what is that?)
   *  Bit 5-0: size of xfer - 1
   */
  txBuf[0] = TPM_WRITE | ((len & 0xFF) - 1);
  /* Next 4 bytes: address, in big endian */
  txBuf[1] = (addr>>16) & 0xFF;
  txBuf[2] = (addr>>8)  & 0xFF;
  txBuf[3] = (addr)     & 0xFF;
  /* After that, the data that we want to send */
  memcpy(txBuf + TPM_HEADER_SIZE, buf, len);
  
  /* TODO: wolfTPM cuts off the first TPM_HEADER_SIZE bytes. Why? */
  bcm2837_spi_writenb(txBuf, len);
}

int TPM_Init(void) {
  /* TRENTOS SPI library initialization */
  if (!bcm2837_spi_begin(regBase)) return 1;
  bcm2837_spi_setBitOrder(BCM2837_SPI_BIT_ORDER_MSBFIRST);
  bcm2837_spi_setDataMode(BCM2837_SPI_MODE0);
  /* Infineon Optiga SLB 9670 TPM has a maximum frequency of 38-45MHz,
   * depending on "t_SLEW" (see datasheet). Here, we use 25MHz because that's
   * the closest value the SPI library supports.
   */
  bcm2837_spi_setClockDivider(BCM2837_SPI_CLOCK_DIVIDER_16);
  bcm2837_spi_chipSelect(BCM2837_SPI_CS1);
//  bcm2837_spi_setChipSelectPolarity(BCM2837_SPI_CS1, 0); // TODO: confirm

  /* GPIO 18 needs to be high */
  bcm2837_gpio_fsel(RPI_GPIO_P1_18, 1); /* set as output pin */
  bcm2837_gpio_set(RPI_GPIO_P1_18); /* set high */

  /* Wait for chip to become available */
  char access = 0;
  do {
    TPM_Read(TPM_ACCESS(0), &access, sizeof(access));
    Debug_LOG_DEBUG("Got: %hhx", access);
  } while (!((access & TPM_ACCESS_VALID) && (access != 0xFF)));

  /* Request locality*/
  access = TPM_ACCESS_REQUEST_USE;
  TPM_Write(TPM_ACCESS(0), &access, sizeof(access));
  /* Check for valid response */
  TPM_Read(TPM_ACCESS(0), &access, sizeof(access));
  if ((access & (TPM_ACCESS_ACTIVE_LOCALITY | TPM_ACCESS_VALID))
             == (TPM_ACCESS_ACTIVE_LOCALITY | TPM_ACCESS_VALID) ) {
    return 0;
  } else {
    Debug_LOG_ERROR("Invalid response from TPM!");
    return 1;
  }
}
