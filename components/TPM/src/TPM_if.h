#include <stdint.h>
#include "TPM_defs.h"
#include "TPM_packet.h"

int TPM_Init(void);
TPM_RC TPM_SendCommandU16(TPM_CC cmd, uint16_t arg, TPM2_Packet *packet);
