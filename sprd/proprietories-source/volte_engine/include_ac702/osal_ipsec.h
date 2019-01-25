/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 1381 $ $Date: 2006-12-05 14:44:53 -0800 (Tue, 05 Dec 2006) $
 */

#ifndef _OSAL_IPSEC_H_
#define _OSAL_IPSEC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "osal_types.h"
#include "osal_constant.h"
#include "osal_platform.h"
#include "osal_net.h"

/*
 * Constant definition
 */
#define OSAL_IPSEC_KEY_LENGTH_HMAC_MD5      (16) /* 128-bit */
#define OSAL_IPSEC_KEY_LENGTH_HMAC_SHA1     (20)  /* 160-bit */
#define OSAL_IPSEC_KEY_LENGTH_3DES_CBC      (24)  /* 192-bit */
#define OSAL_IPSEC_KEY_LENGTH_AES_CBC       (16)  /* 128-bit */
#define OSAL_IPSEC_KEY_LENGTH_MAX           OSAL_IPSEC_KEY_LENGTH_3DES_CBC

/*
 * Maximum size of PFKEY message
 */
#define OSAL_IPSEC_PFKEY_MSG_SIZE_MAX       0xFFFF

/* Maximum bytes for pfkey request */
#define OSAL_IPSEC_PFKEY_REQUEST_BYTE_MAX   (64)
#define OSAL_IPSEC_STRING_SZ   (64)
/* maximum ip string length */
#define OSAL_NET_IP_STR_MAX    (46)

/*
 * IPSec direction for security policy
 */
typedef enum {
    OSAL_IPSEC_DIR_IN = 0,
    OSAL_IPSEC_DIR_OUT,
    OSAL_PSEC_DIR_LAST = OSAL_IPSEC_DIR_OUT,
} OSAL_IpsecDirection;

/*
 * IPSec transport for security policy
 */
typedef enum {
    OSAL_IPSEC_TRANSPORT_ANY = 0,
    OSAL_IPSEC_TRANSPORT_UDP,
    OSAL_IPSEC_TRANSPORT_TCP,
    OSAL_IPSEC_TRANSPORT_LAST = OSAL_IPSEC_TRANSPORT_TCP,
} OSAL_IpsecTransport;

/*
 * IPSec mode for security policy
 */
typedef enum {
    OSAL_IPSEC_SP_MODE_TRANSPORT = 0,
    OSAL_IPSEC_SP_MODE_TUNNEL,
    OSAL_IPSEC_SP_MODE_LAST = OSAL_IPSEC_SP_MODE_TUNNEL,
} OSAL_IpsecMode;

/*
 * IPSec level for security policy
 */
typedef enum {
    OSAL_IPSEC_SP_LEVEL_DEFAULT = 0,
    OSAL_IPSEC_SP_LEVEL_USE,
    OSAL_IPSEC_SP_LEVEL_REQUIRE,
    OSAL_IPSEC_SP_LEVEL_UNIQUE,
    OSAL_IPSEC_SP_LEVEL_LAST = OSAL_IPSEC_SP_LEVEL_UNIQUE,
} OSAL_IpsecLevel;

/*
 * IPSec protocol for security association
 */
typedef enum {
    OSAL_IPSEC_PROTOCOL_ESP = 0,     /* Default */
    OSAL_IPSEC_PROTOCOL_AH,
    OSAL_IPSEC_PROTOCOL_LAST = OSAL_IPSEC_PROTOCOL_AH,
} OSAL_IpsecProtocol;

/*
 * IPSec autheticaiton algorithm  for security association
 */
typedef enum {
    OSAL_IPSEC_AUTH_ALG_HMAC_MD5_96 = 0,
    OSAL_IPSEC_AUTH_ALG_HMAC_SHA1_96,
    OSAL_IPSEC_AUTH_ALG_NULL,
    OSAL_IPSEC_AUTH_ALG_LAST = OSAL_IPSEC_AUTH_ALG_NULL,
} OSAL_IpsecAuthAlg;

/*
 * IPSec encrypt algorithm  for security association
 */
typedef enum {
    OSAL_IPSEC_ENC_ALG_NULL = 0,     /* Default */
    OSAL_IPSEC_ENC_ALG_DES_EDE3_CBC,
    OSAL_IPSEC_ENC_ALG_AES_CBC,      /* Requires by TS 33.203 */
    OSAL_IPSEC_ENC_ALG_LAST = OSAL_IPSEC_ENC_ALG_AES_CBC,
} OSAL_IpsecEncryptAlg;

/*
 * IPSEC security policy
 */
typedef struct {
    OSAL_NetAddress     srcAddr;    /* source address */
    OSAL_NetAddress     dstAddr;    /* destination address */
    OSAL_IpsecProtocol  protocol;   /* ESP, AH or both */
    OSAL_IpsecDirection dir;        /* in or out */
    OSAL_IpsecTransport transport;  /* UDP, TCP or any */
    OSAL_IpsecMode      mode;       /* transport, tunnel or any */
    OSAL_IpsecLevel     level;      /* default, use, require or unique */
    uint32              reqId;      /* Request Id */
} OSAL_IpsecSp;

/*
 * IPSEC security association
 */
typedef struct {
    OSAL_NetAddress      srcAddr;                       /* source address */
    OSAL_NetAddress      dstAddr;                       /* destination address */
    OSAL_IpsecProtocol   protocol;                      /* ESP or AH */
    OSAL_IpsecMode       mode;                          /* transport, tunnel or any */
    uint32               spi;                           /* Security Parameter Index */
    uint32               reqId;                         /* Request Id */
    OSAL_IpsecAuthAlg    algAh;
    OSAL_IpsecEncryptAlg algEsp;
    char                 keyAh[OSAL_IPSEC_KEY_LENGTH_MAX + 1];  /* Key for AH. Plus 1 for end of string */
    char                 keyEsp[OSAL_IPSEC_KEY_LENGTH_MAX + 1]; /* Key for ESP. Plus 1 for end of string*/
} OSAL_IpsecSa;

OSAL_Status OSAL_ipsecCreateSA(
        OSAL_IpsecSa *sa_ptr,
        OSAL_IpsecSp *sp_ptr);

OSAL_Status OSAL_ipsecDeleteSA(
        OSAL_IpsecSa *sa_ptr,
        OSAL_IpsecSp *sp_ptr);

#ifdef __cplusplus
}
#endif

#endif
