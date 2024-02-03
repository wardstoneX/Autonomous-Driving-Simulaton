#include <assert.h>
#include <string.h>
#include "camkes.h"
#include "lib_debug/Debug.h"
#include "OS_Dataport.h"

#include "bcm2837/bcm2837_gpio.h"
#include "bcm2837/bcm2837_spi.h"

#include "wolftpm/tpm2_wrap.h"
#include "hal/tpm_io.h"

static OS_Dataport_t port = OS_DATAPORT_ASSIGN(entropy_port);

/* Context required for wolfTPM */
WOLFTPM2_DEV dev;
TPM2HalIoCb ioCb = TPM2_IoCb_TRENTOS_SPI;

/* Initialization that must be done before initializing 
 * the specific interfaces
 *
 * TODO: TRENTOS handbook page 26 says pre_init must not block?
 */
void pre_init(void) {
  Debug_LOG_INFO("Initializing SPI for TPM");
  /* TRENTOS SPI library initialization */
  if (!bcm2837_spi_begin(regBase)) {
    Debug_LOG_ERROR("Failed to initialize SPI!");
    return;
  }
  bcm2837_spi_setBitOrder(BCM2837_SPI_BIT_ORDER_MSBFIRST);
  bcm2837_spi_setDataMode(BCM2837_SPI_MODE0);
  /* Infineon Optiga SLB 9670 TPM has a maximum frequency of 38-45 MHz,
   * depending on "t_SLEW" (see datasheet). Here, we use 31.25 MHz because
   * that's the closest value the SPI library supports.
   * Note that the Raspberry Pi is running at 250 MHz, not 400 MHz, because of
   * settings in config.txt!
   */
  bcm2837_spi_setClockDivider(BCM2837_SPI_CLOCK_DIVIDER_8);
  bcm2837_spi_setChipSelectPolarity(BCM2837_SPI_CS1, 0);
  bcm2837_spi_chipSelect(BCM2837_SPI_CS1);

  /* Pin 18 (GPIO 24) needs to be high */
  bcm2837_gpio_fsel(RPI_GPIO_P1_18, 1); /* set as output pin */
  bcm2837_gpio_set(RPI_GPIO_P1_18); /* set high */
  Debug_LOG_INFO("SPI initialized successfully");

  Debug_LOG_INFO("Sending TPM initialization commands");
  if (wolfTPM2_Init(&dev, ioCb, NULL) != TPM_RC_SUCCESS)
    Debug_LOG_ERROR("Failed to initialize TPM!");
  Debug_LOG_INFO("TPM initialized successfully");
}

size_t entropy_rpc_read(const size_t len) {
  size_t sz = OS_Dataport_getSize(port);
  sz = len > sz ? sz : len;
  wolfTPM2_GetRandom(&dev, OS_Dataport_getBuf(port), sz);
  return sz;
}
