#include <assert.h>
#include <string.h>
#include "camkes.h"
#include "lib_debug/Debug.h"

#include "bcm2837/bcm2837_gpio.h"
#include "bcm2837/bcm2837_spi.h"
#include "TPM_defs.h"
#include "TPM_packet.h"

#define CMD_BUF_SIZE MAX_SPI_FRAMESIZE
char cmdBuf[CMD_BUF_SIZE];

/* To some extent, adapted from TPM2_TIS_READ in wolfTPM's tpm2_tis.c.
 * I have added explanations as to what all these bit operations mean
 * according to the TPM Interface Specification.
 */
void TPM_Read(uint32_t addr, char *buf, unsigned len) {
  char txBuf[MAX_SPI_FRAMESIZE+TPM_HEADER_SIZE] = {0};
  char rxBuf[MAX_SPI_FRAMESIZE+TPM_HEADER_SIZE] = {0};
//  Debug_LOG_DEBUG("TPM Read of size %d at addr %p", len, (void*) addr);

  /* First byte:
   *  Bit 7: read => 1, write => 0
   *  Bit 6: rsvd (TODO: what is that?)
   *  Bit 5-0: size of xfer - 1
   */
  txBuf[0] = TPM_READ | ((len & 0xFF) - 1);
  /* Next 3 bytes: address, in big endian */
  txBuf[1] = (addr>>16) & 0xFF;
  txBuf[2] = (addr>>8)  & 0xFF;
  txBuf[3] = (addr)     & 0xFF;

  /* Now perform the actual SPI transfer */
  /* TODO: Do we need to check wait states? I remember some comment claiming
   * that the Infineon devices guarantee no wait state, but not sure... */
  bcm2837_spi_transfernb(txBuf, rxBuf, len + TPM_HEADER_SIZE);
  /* According to RPi_SPI_Flash.c:
   *   We can't use the rx_buffer directly, because bcm2837_spi_transfernb()
   *   already populates it when sending the tx bytes. The actual data received
   *   starts at offset tx_len then.
   * In this case, the data that was sent is always TPM_HEADER_SIZE bytes long.
   */
  memcpy(buf, rxBuf + TPM_HEADER_SIZE, len);
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
  
  bcm2837_spi_writenb(txBuf, len + TPM_HEADER_SIZE);
}

int TPM_Init(void) {
  /* TRENTOS SPI library initialization */
  if (!bcm2837_spi_begin(regBase)) return 1;
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

  /* Wait for chip to become available */
  char access = 0;
  int timeout = 16;
  do {
    TPM_Read(TPM_ACCESS(0), &access, sizeof(access));
    Debug_LOG_DEBUG("Got: %hhx", access);
  } while (--timeout && !((access & TPM_ACCESS_VALID) && (access != 0xFF)));

  /* Request locality*/
  access = TPM_ACCESS_REQUEST_USE;
  TPM_Write(TPM_ACCESS(0), &access, sizeof(access));
  /* improvised wait loop */
  timeout = 4096;
  while (--timeout);
  /* Check for valid response */
  Debug_LOG_DEBUG("Checking for valid response to locality request.\n");
  TPM_Read(TPM_ACCESS(0), &access, sizeof(access));
  Debug_LOG_DEBUG("Got: %x\n", access);
  if ((access & (TPM_ACCESS_ACTIVE_LOCALITY | TPM_ACCESS_VALID))
             == (TPM_ACCESS_ACTIVE_LOCALITY | TPM_ACCESS_VALID) ) {
    return 0;
  } else {
    Debug_LOG_ERROR("Invalid response from TPM!");
    return 1;
  }
}

void TPM_WaitForStatus(char statusMask, char value) {
  /* TODO: Do we need to implement timeout? */
  char status = 0;
  while (1) {
    TPM_Read(TPM_STS(0), &status, sizeof(status));
    if ((status & statusMask) == value) break;
    seL4_Yield();
  }
}

/* Comparable to TPM2_TIS_SendCommand() in wolfTPM. I think it's more clear
 * to call the thing being sent a packet rather than a command.
 * See also: TPM TIS spec, page 47
 */
TPM_RC TPM_SendPacket(TPM2_Packet* packet) {
  char status = 0;

  /* Make sure TPM is expecting a command */
  TPM_Read(TPM_STS(0), &status, sizeof(status));
  if (!(status & TPM_STS_COMMAND_READY)) {
    /* Tell TPM to expect a command */
    Debug_LOG_DEBUG("Telling TPM to expect command.");
    status = TPM_STS_COMMAND_READY;
    TPM_Write(TPM_STS(0), &status, sizeof(status));
    /* Wait for TPM to become ready */
    TPM_WaitForStatus(TPM_STS_COMMAND_READY, TPM_STS_COMMAND_READY);
  }

  /* Write the packet */
  Debug_LOG_DEBUG("Writing packet data to TPM.");
  int pos = 0, xferSz;
  char burstCount = 0;
  while (pos < packet->pos) {
    /* BURST_COUNT indicates how many bytes can be read from or written to
     * the TPM without inserting wait states
     */
    TPM_Read(TPM_BURST_COUNT(0), &burstCount, sizeof(burstCount));
    /* TODO: On a big endian architecture, we'd have to reverse the bytes.
     *       Do we need to support anything except the Raspberry Pi?
     */
    if (burstCount == 0) {
      /* Burst count of 0 means TPM not yet ready to accept data */
      seL4_Yield();
      continue;
    }

    xferSz = packet->pos - pos;
    if (xferSz > burstCount) xferSz = burstCount;
    TPM_Write(TPM_DATA_FIFO(0), packet->buf + pos, xferSz);

    pos += xferSz;
    if (pos < packet->pos)
      TPM_WaitForStatus(TPM_STS_DATA_EXPECT, TPM_STS_DATA_EXPECT);
  }

  /* Wait for TPM_STS_DATA_EXPECT = 0 and TPM_STS_VALID = 1 */
  Debug_LOG_DEBUG("Wait for TPM_STS_DATA_EXPECT = 0 and TPM_STS_VALID = 1");
  TPM_WaitForStatus(TPM_STS_DATA_EXPECT | TPM_STS_VALID, TPM_STS_VALID);

  /* Tell TPM to execute command */
  status = TPM_STS_GO;
  TPM_Write(TPM_STS(0), &status, sizeof(status));

  /* Read response */
  Debug_LOG_DEBUG("Reading response from the TPM");
  pos = 0;
  int rspSz = TPM_HEADER_SIZE; /* Real size of data will be extracted
				  from TPM header */
  while (pos < rspSz) {
    TPM_WaitForStatus(TPM_STS_DATA_AVAIL, TPM_STS_DATA_AVAIL);
    TPM_Read(TPM_BURST_COUNT(0), &burstCount, sizeof(burstCount));
    assert(burstCount != 0);

    xferSz = rspSz - pos;
    if (xferSz > burstCount) xferSz = burstCount;
    TPM_Read(TPM_DATA_FIFO(0), packet->buf + pos, xferSz);
    Debug_LOG_DEBUG("Got %d bytes:", xferSz);
    Debug_DUMP_DEBUG(packet->buf + pos, xferSz);
    pos += xferSz;

    if (pos == TPM_HEADER_SIZE) {
      /* Extract real size from header, then keep reading data */
      uint32_t tmpSz;
      memcpy(&tmpSz, packet->buf + 2, sizeof(uint32_t));
      rspSz = TPM2_Packet_SwapU32(tmpSz);
      Debug_LOG_DEBUG("Real response size: %d", rspSz);
      if (rspSz < 0 || rspSz > packet->size) {
        Debug_LOG_ERROR("Response size %d out of bounds!", rspSz);
	return TPM_RC_FAILURE;
      }
    }
  }

  /* Tell TPM that we're done */
  status = TPM_STS_COMMAND_READY;
  TPM_Write(TPM_STS(0), &status, sizeof(status));

  /* Check response code of the TPM's response */
  uint16_t tmpRc;
  memcpy(&tmpRc, packet->buf + 6, sizeof(uint16_t));
  Debug_LOG_DEBUG("Got RC: %x", tmpRc);

  return TPM_RC_SUCCESS;
}

TPM_RC TPM_SendCommandU16(TPM_CC cmd, uint16_t arg) {
  TPM_RC rc;
  TPM2_Packet packet;
  TPM2_Packet_InitBuf(&packet, cmdBuf, sizeof(cmdBuf));
  TPM2_Packet_AppendU16(&packet, arg);
  /* TODO: Do we ever need TPM_ST_SESSIONS? */
  TPM2_Packet_Finalize(&packet, TPM_ST_NO_SESSIONS, cmd);
  rc = TPM_SendPacket(&packet);
  if (rc != TPM_RC_SUCCESS) return rc;
  /* TODO: Possibly extra checks on return value ? */
  return rc;
}
