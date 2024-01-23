/* Definitions of all the constants needed for interfacing with the TPM */

#ifndef TPM_DEFS_H
#define TPM_DEFS_H
#include <stdint.h>

/* Read if first bit on wire is set, write otherwise
 * See TPM Interface Specification, pages 103-104
 */
#define TPM_READ	0x80
#define TPM_WRITE	0x00

/* Size of the buffer where commands and responses are stored */
#define CMD_BUF_SIZE MAX_SPI_FRAMESIZE

#define TPM_HEADER_SIZE		4	/* Header size for SPI */
#define TPM_COMM_HEADER_SIZE	10	/* Header size for TPM commands */

/* TODO: Why? This is how wolfTPM defines it, but I don't know whether that's
 * platform-specific or defined by some SPI standard.
 */
#define MAX_SPI_FRAMESIZE 64

/* Various registers of the TPM
 * Copied from wolfTPM's tpm2_tis.c */
#define TPM_BASE_ADDRESS (0xD40000u)
#define TPM_ACCESS(l)           (TPM_BASE_ADDRESS | 0x0000u | ((l) << 12u))
#define TPM_INTF_CAPS(l)        (TPM_BASE_ADDRESS | 0x0014u | ((l) << 12u))
#define TPM_DID_VID(l)          (TPM_BASE_ADDRESS | 0x0F00u | ((l) << 12u))
#define TPM_RID(l)              (TPM_BASE_ADDRESS | 0x0F04u | ((l) << 12u))
#define TPM_INT_ENABLE(l)       (TPM_BASE_ADDRESS | 0x0008u | ((l) << 12u))
#define TPM_INT_VECTOR(l)       (TPM_BASE_ADDRESS | 0x000Cu | ((l) << 12u))
#define TPM_INT_STATUS(l)       (TPM_BASE_ADDRESS | 0x0010u | ((l) << 12u))
#define TPM_STS(l)              (TPM_BASE_ADDRESS | 0x0018u | ((l) << 12u))
#define TPM_BURST_COUNT(l)      (TPM_BASE_ADDRESS | 0x0019u | ((l) << 12u))
#define TPM_DATA_FIFO(l)        (TPM_BASE_ADDRESS | 0x0024u | ((l) << 12u))
#define TPM_XDATA_FIFO(l)       (TPM_BASE_ADDRESS | 0x0083u | ((l) << 12u))

/* TIS pages 58-59 */
enum tpm_access {
  TPM_ACCESS_VALID            = 0x80,
  TPM_ACCESS_ACTIVE_LOCALITY  = 0x20,
  TPM_ACCESS_REQUEST_PENDING  = 0x04,
  TPM_ACCESS_REQUEST_USE      = 0x02,
};

enum tpm_tis_status {
    TPM_STS_VALID               = 0x80,
    TPM_STS_COMMAND_READY       = 0x40,
    TPM_STS_GO                  = 0x20,
    TPM_STS_DATA_AVAIL          = 0x10,
    TPM_STS_DATA_EXPECT         = 0x08,
    TPM_STS_SELF_TEST_DONE      = 0x04,
    TPM_STS_RESP_RETRY          = 0x02,
};

/* Command Codes
 * Copied from wolfTPM's tpm2.h */
typedef enum {
    TPM_CC_FIRST                    = 0x0000011F,
    TPM_CC_NV_UndefineSpaceSpecial  = TPM_CC_FIRST,
    TPM_CC_EvictControl             = 0x00000120,
    TPM_CC_HierarchyControl         = 0x00000121,
    TPM_CC_NV_UndefineSpace         = 0x00000122,
    TPM_CC_ChangeEPS                = 0x00000124,
    TPM_CC_ChangePPS                = 0x00000125,
    TPM_CC_Clear                    = 0x00000126,
    TPM_CC_ClearControl             = 0x00000127,
    TPM_CC_ClockSet                 = 0x00000128,
    TPM_CC_HierarchyChangeAuth      = 0x00000129,
    TPM_CC_NV_DefineSpace           = 0x0000012A,
    TPM_CC_PCR_Allocate             = 0x0000012B,
    TPM_CC_PCR_SetAuthPolicy        = 0x0000012C,
    TPM_CC_PP_Commands              = 0x0000012D,
    TPM_CC_SetPrimaryPolicy         = 0x0000012E,
    TPM_CC_FieldUpgradeStart        = 0x0000012F,
    TPM_CC_ClockRateAdjust          = 0x00000130,
    TPM_CC_CreatePrimary            = 0x00000131,
    TPM_CC_NV_GlobalWriteLock       = 0x00000132,
    TPM_CC_GetCommandAuditDigest    = 0x00000133,
    TPM_CC_NV_Increment             = 0x00000134,
    TPM_CC_NV_SetBits               = 0x00000135,
    TPM_CC_NV_Extend                = 0x00000136,
    TPM_CC_NV_Write                 = 0x00000137,
    TPM_CC_NV_WriteLock             = 0x00000138,
    TPM_CC_DictionaryAttackLockReset = 0x00000139,
    TPM_CC_DictionaryAttackParameters = 0x0000013A,
    TPM_CC_NV_ChangeAuth            = 0x0000013B,
    TPM_CC_PCR_Event                = 0x0000013C,
    TPM_CC_PCR_Reset                = 0x0000013D,
    TPM_CC_SequenceComplete         = 0x0000013E,
    TPM_CC_SetAlgorithmSet          = 0x0000013F,
    TPM_CC_SetCommandCodeAuditStatus = 0x00000140,
    TPM_CC_FieldUpgradeData         = 0x00000141,
    TPM_CC_IncrementalSelfTest      = 0x00000142,
    TPM_CC_SelfTest                 = 0x00000143,
    TPM_CC_Startup                  = 0x00000144,
    TPM_CC_Shutdown                 = 0x00000145,
    TPM_CC_StirRandom               = 0x00000146,
    TPM_CC_ActivateCredential       = 0x00000147,
    TPM_CC_Certify                  = 0x00000148,
    TPM_CC_PolicyNV                 = 0x00000149,
    TPM_CC_CertifyCreation          = 0x0000014A,
    TPM_CC_Duplicate                = 0x0000014B,
    TPM_CC_GetTime                  = 0x0000014C,
    TPM_CC_GetSessionAuditDigest    = 0x0000014D,
    TPM_CC_NV_Read                  = 0x0000014E,
    TPM_CC_NV_ReadLock              = 0x0000014F,
    TPM_CC_ObjectChangeAuth         = 0x00000150,
    TPM_CC_PolicySecret             = 0x00000151,
    TPM_CC_Rewrap                   = 0x00000152,
    TPM_CC_Create                   = 0x00000153,
    TPM_CC_ECDH_ZGen                = 0x00000154,
    TPM_CC_HMAC                     = 0x00000155,
    TPM_CC_Import                   = 0x00000156,
    TPM_CC_Load                     = 0x00000157,
    TPM_CC_Quote                    = 0x00000158,
    TPM_CC_RSA_Decrypt              = 0x00000159,
    TPM_CC_HMAC_Start               = 0x0000015B,
    TPM_CC_SequenceUpdate           = 0x0000015C,
    TPM_CC_Sign                     = 0x0000015D,
    TPM_CC_Unseal                   = 0x0000015E,
    TPM_CC_PolicySigned             = 0x00000160,
    TPM_CC_ContextLoad              = 0x00000161,
    TPM_CC_ContextSave              = 0x00000162,
    TPM_CC_ECDH_KeyGen              = 0x00000163,
    TPM_CC_EncryptDecrypt           = 0x00000164,
    TPM_CC_FlushContext             = 0x00000165,
    TPM_CC_LoadExternal             = 0x00000167,
    TPM_CC_MakeCredential           = 0x00000168,
    TPM_CC_NV_ReadPublic            = 0x00000169,
    TPM_CC_PolicyAuthorize          = 0x0000016A,
    TPM_CC_PolicyAuthValue          = 0x0000016B,
    TPM_CC_PolicyCommandCode        = 0x0000016C,
    TPM_CC_PolicyCounterTimer       = 0x0000016D,
    TPM_CC_PolicyCpHash             = 0x0000016E,
    TPM_CC_PolicyLocality           = 0x0000016F,
    TPM_CC_PolicyNameHash           = 0x00000170,
    TPM_CC_PolicyOR                 = 0x00000171,
    TPM_CC_PolicyTicket             = 0x00000172,
    TPM_CC_ReadPublic               = 0x00000173,
    TPM_CC_RSA_Encrypt              = 0x00000174,
    TPM_CC_StartAuthSession         = 0x00000176,
    TPM_CC_VerifySignature          = 0x00000177,
    TPM_CC_ECC_Parameters           = 0x00000178,
    TPM_CC_FirmwareRead             = 0x00000179,
    TPM_CC_GetCapability            = 0x0000017A,
    TPM_CC_GetRandom                = 0x0000017B,
    TPM_CC_GetTestResult            = 0x0000017C,
    TPM_CC_Hash                     = 0x0000017D,
    TPM_CC_PCR_Read                 = 0x0000017E,
    TPM_CC_PolicyPCR                = 0x0000017F,
    TPM_CC_PolicyRestart            = 0x00000180,
    TPM_CC_ReadClock                = 0x00000181,
    TPM_CC_PCR_Extend               = 0x00000182,
    TPM_CC_PCR_SetAuthValue         = 0x00000183,
    TPM_CC_NV_Certify               = 0x00000184,
    TPM_CC_EventSequenceComplete    = 0x00000185,
    TPM_CC_HashSequenceStart        = 0x00000186,
    TPM_CC_PolicyPhysicalPresence   = 0x00000187,
    TPM_CC_PolicyDuplicationSelect  = 0x00000188,
    TPM_CC_PolicyGetDigest          = 0x00000189,
    TPM_CC_TestParms                = 0x0000018A,
    TPM_CC_Commit                   = 0x0000018B,
    TPM_CC_PolicyPassword           = 0x0000018C,
    TPM_CC_ZGen_2Phase              = 0x0000018D,
    TPM_CC_EC_Ephemeral             = 0x0000018E,
    TPM_CC_PolicyNvWritten          = 0x0000018F,
    TPM_CC_PolicyTemplate           = 0x00000190,
    TPM_CC_CreateLoaded             = 0x00000191,
    TPM_CC_PolicyAuthorizeNV        = 0x00000192,
    TPM_CC_EncryptDecrypt2          = 0x00000193,
    TPM_CC_LAST                     = TPM_CC_EncryptDecrypt2,

    CC_VEND                         = 0x20000000,
    TPM_CC_Vendor_TCG_Test          = CC_VEND + 0x0000,
#if defined(WOLFTPM_ST33) || defined(WOLFTPM_AUTODETECT)
    TPM_CC_SetMode                  = CC_VEND + 0x0307,
    TPM_CC_SetCommandSet            = CC_VEND + 0x0309,
    TPM_CC_GetRandom2               = CC_VEND + 0x030E,
#endif
#ifdef WOLFTPM_ST33
    TPM_CC_RestoreEK                = CC_VEND + 0x030A,
    TPM_CC_SetCommandSetLock        = CC_VEND + 0x030B,
    TPM_CC_GPIO_Config              = CC_VEND + 0x030F,
#endif
#ifdef WOLFTPM_NUVOTON
    TPM_CC_NTC2_PreConfig           = CC_VEND + 0x0211,
    TPM_CC_NTC2_GetConfig           = CC_VEND + 0x0213,
#endif
} TPM_CC_T;
typedef uint32_t TPM_CC;

/* Response Codes
 * Copied from wolfTPM's tpm2.h */
typedef enum {
    TPM_RC_SUCCESS  = 0x000,
    TPM_RC_BAD_TAG  = 0x01E,

    RC_VER1 = 0x100,
    TPM_RC_INITIALIZE           = RC_VER1 + 0x000,
    TPM_RC_FAILURE              = RC_VER1 + 0x001,
    TPM_RC_SEQUENCE             = RC_VER1 + 0x003,
    TPM_RC_PRIVATE              = RC_VER1 + 0x00B,
    TPM_RC_HMAC                 = RC_VER1 + 0x019,
    TPM_RC_DISABLED             = RC_VER1 + 0x020,
    TPM_RC_EXCLUSIVE            = RC_VER1 + 0x021,
    TPM_RC_AUTH_TYPE            = RC_VER1 + 0x024,
    TPM_RC_AUTH_MISSING         = RC_VER1 + 0x025,
    TPM_RC_POLICY               = RC_VER1 + 0x026,
    TPM_RC_PCR                  = RC_VER1 + 0x027,
    TPM_RC_PCR_CHANGED          = RC_VER1 + 0x028,
    TPM_RC_UPGRADE              = RC_VER1 + 0x02D,
    TPM_RC_TOO_MANY_CONTEXTS    = RC_VER1 + 0x02E,
    TPM_RC_AUTH_UNAVAILABLE     = RC_VER1 + 0x02F,
    TPM_RC_REBOOT               = RC_VER1 + 0x030,
    TPM_RC_UNBALANCED           = RC_VER1 + 0x031,
    TPM_RC_COMMAND_SIZE         = RC_VER1 + 0x042,
    TPM_RC_COMMAND_CODE         = RC_VER1 + 0x043,
    TPM_RC_AUTHSIZE             = RC_VER1 + 0x044,
    TPM_RC_AUTH_CONTEXT         = RC_VER1 + 0x045,
    TPM_RC_NV_RANGE             = RC_VER1 + 0x046,
    TPM_RC_NV_SIZE              = RC_VER1 + 0x047,
    TPM_RC_NV_LOCKED            = RC_VER1 + 0x048,
    TPM_RC_NV_AUTHORIZATION     = RC_VER1 + 0x049,
    TPM_RC_NV_UNINITIALIZED     = RC_VER1 + 0x04A,
    TPM_RC_NV_SPACE             = RC_VER1 + 0x04B,
    TPM_RC_NV_DEFINED           = RC_VER1 + 0x04C,
    TPM_RC_BAD_CONTEXT          = RC_VER1 + 0x050,
    TPM_RC_CPHASH               = RC_VER1 + 0x051,
    TPM_RC_PARENT               = RC_VER1 + 0x052,
    TPM_RC_NEEDS_TEST           = RC_VER1 + 0x053,
    TPM_RC_NO_RESULT            = RC_VER1 + 0x054,
    TPM_RC_SENSITIVE            = RC_VER1 + 0x055,
    RC_MAX_FM0                  = RC_VER1 + 0x07F,

    RC_FMT1 = 0x080,
    TPM_RC_ASYMMETRIC       = RC_FMT1 + 0x001,
    TPM_RC_ATTRIBUTES       = RC_FMT1 + 0x002,
    TPM_RC_HASH             = RC_FMT1 + 0x003,
    TPM_RC_VALUE            = RC_FMT1 + 0x004,
    TPM_RC_HIERARCHY        = RC_FMT1 + 0x005,
    TPM_RC_KEY_SIZE         = RC_FMT1 + 0x007,
    TPM_RC_MGF              = RC_FMT1 + 0x008,
    TPM_RC_MODE             = RC_FMT1 + 0x009,
    TPM_RC_TYPE             = RC_FMT1 + 0x00A,
    TPM_RC_HANDLE           = RC_FMT1 + 0x00B,
    TPM_RC_KDF              = RC_FMT1 + 0x00C,
    TPM_RC_RANGE            = RC_FMT1 + 0x00D,
    TPM_RC_AUTH_FAIL        = RC_FMT1 + 0x00E,
    TPM_RC_NONCE            = RC_FMT1 + 0x00F,
    TPM_RC_PP               = RC_FMT1 + 0x010,
    TPM_RC_SCHEME           = RC_FMT1 + 0x012,
    TPM_RC_SIZE             = RC_FMT1 + 0x015,
    TPM_RC_SYMMETRIC        = RC_FMT1 + 0x016,
    TPM_RC_TAG              = RC_FMT1 + 0x017,
    TPM_RC_SELECTOR         = RC_FMT1 + 0x018,
    TPM_RC_INSUFFICIENT     = RC_FMT1 + 0x01A,
    TPM_RC_SIGNATURE        = RC_FMT1 + 0x01B,
    TPM_RC_KEY              = RC_FMT1 + 0x01C,
    TPM_RC_POLICY_FAIL      = RC_FMT1 + 0x01D,
    TPM_RC_INTEGRITY        = RC_FMT1 + 0x01F,
    TPM_RC_TICKET           = RC_FMT1 + 0x020,
    TPM_RC_RESERVED_BITS    = RC_FMT1 + 0x021,
    TPM_RC_BAD_AUTH         = RC_FMT1 + 0x022,
    TPM_RC_EXPIRED          = RC_FMT1 + 0x023,
    TPM_RC_POLICY_CC        = RC_FMT1 + 0x024,
    TPM_RC_BINDING          = RC_FMT1 + 0x025,
    TPM_RC_CURVE            = RC_FMT1 + 0x026,
    TPM_RC_ECC_POINT        = RC_FMT1 + 0x027,
    RC_MAX_FMT1             = RC_FMT1 + 0x03F,

    RC_WARN = 0x900,
    TPM_RC_CONTEXT_GAP      = RC_WARN + 0x001,
    TPM_RC_OBJECT_MEMORY    = RC_WARN + 0x002,
    TPM_RC_SESSION_MEMORY   = RC_WARN + 0x003,
    TPM_RC_MEMORY           = RC_WARN + 0x004,
    TPM_RC_SESSION_HANDLES  = RC_WARN + 0x005,
    TPM_RC_OBJECT_HANDLES   = RC_WARN + 0x006,
    TPM_RC_LOCALITY         = RC_WARN + 0x007,
    TPM_RC_YIELDED          = RC_WARN + 0x008,
    TPM_RC_CANCELED         = RC_WARN + 0x009,
    TPM_RC_TESTING          = RC_WARN + 0x00A,
    TPM_RC_REFERENCE_H0     = RC_WARN + 0x010,
    TPM_RC_REFERENCE_H1     = RC_WARN + 0x011,
    TPM_RC_REFERENCE_H2     = RC_WARN + 0x012,
    TPM_RC_REFERENCE_H3     = RC_WARN + 0x013,
    TPM_RC_REFERENCE_H4     = RC_WARN + 0x014,
    TPM_RC_REFERENCE_H5     = RC_WARN + 0x015,
    TPM_RC_REFERENCE_H6     = RC_WARN + 0x016,
    TPM_RC_REFERENCE_S0     = RC_WARN + 0x018,
    TPM_RC_REFERENCE_S1     = RC_WARN + 0x019,
    TPM_RC_REFERENCE_S2     = RC_WARN + 0x01A,
    TPM_RC_REFERENCE_S3     = RC_WARN + 0x01B,
    TPM_RC_REFERENCE_S4     = RC_WARN + 0x01C,
    TPM_RC_REFERENCE_S5     = RC_WARN + 0x01D,
    TPM_RC_REFERENCE_S6     = RC_WARN + 0x01E,
    TPM_RC_NV_RATE          = RC_WARN + 0x020,
    TPM_RC_LOCKOUT          = RC_WARN + 0x021,
    TPM_RC_RETRY            = RC_WARN + 0x022,
    TPM_RC_NV_UNAVAILABLE   = RC_WARN + 0x023,
    RC_MAX_WARN             = RC_WARN + 0x03F,

    TPM_RC_NOT_USED         = RC_WARN + 0x07F,

    TPM_RC_H        = 0x000,
    TPM_RC_P        = 0x040,
    TPM_RC_S        = 0x800,
    TPM_RC_1        = 0x100,
    TPM_RC_2        = 0x200,
    TPM_RC_3        = 0x300,
    TPM_RC_4        = 0x400,
    TPM_RC_5        = 0x500,
    TPM_RC_6        = 0x600,
    TPM_RC_7        = 0x700,
    TPM_RC_8        = 0x800,
    TPM_RC_9        = 0x900,
    TPM_RC_A        = 0xA00,
    TPM_RC_B        = 0xB00,
    TPM_RC_C        = 0xC00,
    TPM_RC_D        = 0xD00,
    TPM_RC_E        = 0xE00,
    TPM_RC_F        = 0xF00,
    TPM_RC_N_MASK   = 0xF00,

    /* use negative codes for internal errors */
    TPM_RC_TIMEOUT = -100,
} TPM_RC_T;
typedef int32_t TPM_RC; /* type is unsigned 16-bits, but internally use signed 32-bit */

/* Structure Tags
 * Copied from wolfTPM's tpm2.h */
typedef enum {
    TPM_ST_RSP_COMMAND          = 0x00C4,
    TPM_ST_NULL                 = 0x8000,
    TPM_ST_NO_SESSIONS          = 0x8001,
    TPM_ST_SESSIONS             = 0x8002,
    TPM_ST_ATTEST_NV            = 0x8014,
    TPM_ST_ATTEST_COMMAND_AUDIT = 0x8015,
    TPM_ST_ATTEST_SESSION_AUDIT = 0x8016,
    TPM_ST_ATTEST_CERTIFY       = 0x8017,
    TPM_ST_ATTEST_QUOTE         = 0x8018,
    TPM_ST_ATTEST_TIME          = 0x8019,
    TPM_ST_ATTEST_CREATION      = 0x801A,
    TPM_ST_CREATION             = 0x8021,
    TPM_ST_VERIFIED             = 0x8022,
    TPM_ST_AUTH_SECRET          = 0x8023,
    TPM_ST_HASHCHECK            = 0x8024,
    TPM_ST_AUTH_SIGNED          = 0x8025,
    TPM_ST_FU_MANIFEST          = 0x8029,
} TPM_ST_T;
typedef uint16_t TPM_ST;

/* Startup Type */
typedef enum {
    TPM_SU_CLEAR = 0x0000,
    TPM_SU_STATE = 0x0001,
} TPM_SU_T;
typedef uint16_t TPM_SU;

#endif
