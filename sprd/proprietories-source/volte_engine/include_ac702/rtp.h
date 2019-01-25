/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29192 $ $Date: 2014-10-08 17:15:16 +0800 (Wed, 08 Oct 2014) $
 */

#ifndef _RTP_H_
#define _RTP_H_

/*
 * Includes.
 */
#include <osal.h>

#ifndef VTSP_ENABLE_SRTP
# include <osal_net.h>
#endif

#define RTP_MEMCPY         OSAL_memCpy

/*
 * Return codes.
 */
#define RTP_OK            (1)
#define RTP_ERR           (0)

/*
 * RTP Version.
 */
#define RTP_VERSION       (2)

/*
 * Contributing sources (at mixers).
 */
#define RTP_CSRCCOUNT_MAX  (15)
#define RTP_CSRCCOUNT_MIN  (0)

/*
 * Max payload size in samples RTP can handle.
 */
#if defined(PROVIDER_LGUPLUS)
#define RTP_SAMPLES_MAX    (1300)
#else
#define RTP_SAMPLES_MAX    (1512)
#endif

/*
 * Define max allowance for header extension + padding in octets.
 * Bug 1397, add 4 extra for authentication tag
 */
#define RTP_EXTRA_STORAGE  (32 + 4)

/*
 * Define Max RTP payload buffer size.
 */
#define RTP_BUFSZ_MAX      (RTP_SAMPLES_MAX + RTP_EXTRA_STORAGE)

/*
 * RTP options.
 */
#define RTP_PADDING        (1 << 7)
#define RTP_EXTENSION      (1 << 6)
#define RTP_MARKER         (1 << 5)

/*
 * SRTP related definitions
 */
#define RTP_KEY_STRING_MAX_LEN  (30)

/*
 * Define enums for SRTP security services
 */
typedef enum {
    RTP_SECURITY_SERVICE_NONE          = 0,    /* no security */
    RTP_SECURITY_SERVICE_CONF_AUTH_80  = 1,    /* conf and auth with 80 bit auth tag */
    RTP_SECURITY_SERVICE_CONF_AUTH_32  = 2     /* conf and auth with 32 bit auth tag */
} RTP_SecurityType;

/*
 * Internal structure stores SRTP policy information
 */
typedef struct {
    RTP_SecurityType    serviceType;
    uint32              ssrcInit;
    uint8               key[RTP_KEY_STRING_MAX_LEN];
} _SRTP_Policy;

/*
 * RTP_REDUN (RFC2198) Constants.
 */
#define RTP_REDUN_LEVEL_MAX    (6)
#define RTP_REDUN_HOP_MAX      (6)
#define RTP_REDUN_CACHE_MAX    (RTP_REDUN_LEVEL_MAX * RTP_REDUN_HOP_MAX + 4)

#define RTP_REDUN_DYN_TYPE_DEFAULT    (99)
#define RTP_REDUN_DYN_TYPE_MIN        (96)
#define RTP_REDUN_DYN_TYPE_MAX        (128)
#define RTP_REDUN_HEADER_LEN          (4)
#define RTP_REDUN_PRIMARY_HEADER_LEN  (1)
#define RTP_REDUN_CACHE_RESET         (1)

/*
 * Define RFC2198 RTP redundant header.
 */
typedef struct {
    uint8  blkPT;
    uint32 tStamp;
    uint16 blkLen;
} RTP_RdnHdr;

/*
 * Define RTP_RDN (RFC2198) object.
 */
typedef struct {
    RTP_RdnHdr hdr;
    uint8      data[RTP_BUFSZ_MAX];
} RTP_RdnObj;

/*
 * Define RTP cache (RFC2198) object.
 */
typedef struct {
    uint8      level;          /* Redun level */
    uint8      hop;            /* Delayed hop */
    uint8      avail;          /* Num. of redundant data can be packed */
    uint8      rem;            /* = (validLen % hop) */    
    uint16     curr;           /* Current index to redundant cache buffer */
    RTP_RdnObj rdnObj_ary[RTP_REDUN_CACHE_MAX]; /* the redundant cache buffer */
} RTP_RdnCache;

/*
 * Define unpacked RTP header.
 */
typedef struct {
    uint8  vers;
    uint8  padding;
    uint8  extension;
    uint8  csrcCount;
    uint8  marker;
    uint8  type;
    uint16 seqn;
    uint32 ts;
    uint32 ssrc;
} RTP_MinHdr;

/*
 * Define RTP packet.
 */
typedef struct {
    RTP_MinHdr rtpMinHdr;                   /* RTP min header */
    uint32     csrcList[RTP_CSRCCOUNT_MAX]; /* List of contributing sources */
    uint8      payload[RTP_BUFSZ_MAX];      /* Payload + Hdr_ext + Padding */
} RTP_Pkt;

/*
 * Define RTP object.
 */
typedef struct {
    void         *context_ptr;    /* private structure */
    _SRTP_Policy  srtpPolicy;     /* private structure */
    RTP_Pkt       pkt;            /* Pakcet, packed or unpacked */
    uint16        payloadSize;    /* Size of payload packet in bytes */
    uint16        packetSize;     /* Size of the packet in octets, packed */
    uint32        preamble;       /* Preamble for packet, V|P|X|CC|M */
    uint32        tStamp;         /* Time Stamp */
    uint16        seqRandom;      /* use random or 0 for Sequence Number */
    uint16        tsRandom;       /* use random or 0 for time stamp */
    RTP_RdnCache *rdnCache_ptr;   /* redundant cache object pointer */
    uint8         rdnDynType;     /* dynamic payload type for RFC2198 pkt */
} RTP_Obj;

/*
 * Function prototypes.
 */
void RTP_redunInit(
    RTP_Obj *obj_ptr);

void RTP_redunShutdown(
    RTP_Obj *obj_ptr);

void RTP_redunReset(
    RTP_Obj *obj_ptr,
    uint8    rdnDynType);

void RTP_init(
    RTP_Obj *obj_ptr);

void RTP_shutdown(
    RTP_Obj *obj_ptr);

void RTP_srtpinit(
    RTP_Obj          *obj_ptr,
    RTP_SecurityType  serviceType,
    char             *key_ptr,
    uvint             keyLen);

void RTP_setOpts(
    RTP_Obj *obj_ptr,
    uint32   opts);

void RTP_setCsrc(
    RTP_Obj *obj_ptr,
    uint32  *csrc_ptr);

void RTP_getCsrc(
    RTP_Obj *obj_ptr,
    uint32  *csrc_ptr);

vint RTP_xmitPkt(
    RTP_Obj *obj_ptr,
    uint8   *buf_ptr,
    uint16   payloadSize,
    uint8    payloadType);

vint RTP_recvPkt(
    RTP_Obj *obj_ptr,
    uint8   *buf_ptr,
    uint16   packetSize);

uvint RTP_getContextSize(
    void);

#endif /* _RTP_H_ */
