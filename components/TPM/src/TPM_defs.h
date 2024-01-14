/* Definitions of all the constants needed for interfacint with the TPM */

/* Read if first bit on wire is set, write otherwise
 * See TPM Interface Specification, pages 103-104
 */
#define TPM_READ	0x80
#define TPM_WRITE	0x00

#define TPM_HEADER_SIZE	4

/* TODO: Why? This is how wolfTPM defines it, but I don't know whether that's
 * platform-specific or defined by some SPI standard.
 */
#define MAX_SPI_FRAMESIZE 64

/* TIS pages 58-59*/
enum tpm_access {
  TPM_ACCESS_VALID            = 0x80,
  TPM_ACCESS_ACTIVE_LOCALITY  = 0x20,
  TPM_ACCESS_REQUEST_PENDING  = 0x04,
  TPM_ACCESS_REQUEST_USE      = 0x02,
};

/* Various registers of the TPM
 * Copied from wolfTPM's tpm2_tis.c */
#define TPM_BASE_ADDRESS (0xD40000u) /* TODO: spec says 0xFED40000 */
#define TPM_ACCESS(l)           (TPM_BASE_ADDRESS | 0x0000u | ((l) << 12u))
#if 0
#define TPM_INTF_CAPS(l)        (TPM_BASE_ADDRESS | 0x0014u | ((l) << 12u))
#define TPM_DID_VID(l)          (TPM_BASE_ADDRESS | 0x0F00u | ((l) << 12u))
#define TPM_RID(l)              (TPM_BASE_ADDRESS | 0x0F04u | ((l) << 12u))
#endif

