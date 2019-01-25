/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30178 $ $Date: 2014-12-03 10:12:26 +0800 (Wed, 03 Dec 2014) $
 *
 */

#include "osal.h"

#ifndef _VTSP_CONSTANT_H_
#define _VTSP_CONSTANT_H_



#define VTSP_MASK_EXT_FAX_64K           (1 << 0)
#define VTSP_MASK_EXT_RESERVE_1         (1 << 1)  /* buffer & drop DTMF packets */
#define VTSP_MASK_EXT_DTMF_REMOVE       (1 << 2)  /* remove DTMF if detected */
#define VTSP_MASK_EXT_RESERVE_3         (1 << 3)
#define VTSP_MASK_EXT_RESERVE_4         (1 << 4)
#define VTSP_MASK_EXT_RESERVE_5         (1 << 5)
#define VTSP_MASK_EXT_RESERVE_6         (1 << 6)
#define VTSP_MASK_EXT_RESERVE_7         (1 << 7)
#define VTSP_MASK_EXT_SEND              (1 << 8)
#define VTSP_MASK_EXT_APPEND            (1 << 9)
#define VTSP_MASK_EXT_MARKER            (1 << 10) /* RTP Marker edge */
#define VTSP_MASK_EXT_RTP_EXTN          (1 << 11) /* RTP Header Extension */
#define VTSP_MASK_EXT_DR                (1 << 12) /* Data is DTMF Relay */
#define VTSP_MASK_EXT_CODER_CHANGE      (1 << 13) /* Encoder changed */
#define VTSP_MASK_EXT_G723_53           (1 << 14) /* Use 5.3k rate (6.3k default) */
#define VTSP_MASK_EXT_G722P1_32         (1 << 15) /* Use 32k rate (24k default) */
#define VTSP_MASK_EXT_GAMRNB_20MS_475   (1 << 16)
#define VTSP_MASK_EXT_GAMRNB_20MS_515   (1 << 17)
#define VTSP_MASK_EXT_GAMRNB_20MS_59    (1 << 18)
#define VTSP_MASK_EXT_GAMRNB_20MS_67    (1 << 19)
#define VTSP_MASK_EXT_GAMRNB_20MS_74    (1 << 20)
#define VTSP_MASK_EXT_GAMRNB_20MS_795   (1 << 21)
#define VTSP_MASK_EXT_GAMRNB_20MS_102   (1 << 22)
#define VTSP_MASK_EXT_GAMRNB_20MS_122   (1 << 23)
#define VTSP_MASK_EXT_GAMRWB_20MS_660   (1 << 16)
#define VTSP_MASK_EXT_GAMRWB_20MS_885   (1 << 17)
#define VTSP_MASK_EXT_GAMRWB_20MS_1265  (1 << 18)
#define VTSP_MASK_EXT_GAMRWB_20MS_1425  (1 << 19)
#define VTSP_MASK_EXT_GAMRWB_20MS_1585  (1 << 20)
#define VTSP_MASK_EXT_GAMRWB_20MS_1825  (1 << 21)
#define VTSP_MASK_EXT_GAMRWB_20MS_1985  (1 << 22)
#define VTSP_MASK_EXT_GAMRWB_20MS_2305  (1 << 23)
#define VTSP_MASK_EXT_GAMRWB_20MS_2385  (1 << 24)
#define VTSP_MASK_EXT_G711P1_R3_96      (1 << 25)
#define VTSP_MASK_EXT_G711P1_R2A_80     (1 << 26)
#define VTSP_MASK_EXT_G711P1_R2B_80     (1 << 27)
#define VTSP_MASK_EXT_G711P1_R1_64      (1 << 28)
#define VTSP_MASK_EXT_DTMFR_8K          (1 << 29)
#define VTSP_MASK_EXT_DTMFR_16K         (1 << 30)

#define VTSP_MASK_EXT_GAMRNB_ALL_RATES (VTSP_MASK_EXT_GAMRNB_20MS_475 | VTSP_MASK_EXT_GAMRNB_20MS_515 | VTSP_MASK_EXT_GAMRNB_20MS_59 | VTSP_MASK_EXT_GAMRNB_20MS_67 | VTSP_MASK_EXT_GAMRNB_20MS_74 | VTSP_MASK_EXT_GAMRNB_20MS_795 | VTSP_MASK_EXT_GAMRNB_20MS_102 | VTSP_MASK_EXT_GAMRNB_20MS_122)

#define VTSP_MASK_EXT_GAMRWB_ALL_RATES (VTSP_MASK_EXT_GAMRWB_20MS_660 | VTSP_MASK_EXT_GAMRWB_20MS_885 | VTSP_MASK_EXT_GAMRWB_20MS_1265 | VTSP_MASK_EXT_GAMRWB_20MS_1425 | VTSP_MASK_EXT_GAMRWB_20MS_1585 | VTSP_MASK_EXT_GAMRWB_20MS_1825 | VTSP_MASK_EXT_GAMRWB_20MS_1985 | VTSP_MASK_EXT_GAMRWB_20MS_2305 | VTSP_MASK_EXT_GAMRWB_20MS_2385)

#define VTSP_MASK_EXT_G711P1_ALL_RATES (VTSP_MASK_EXT_G711P1_R3_96 | VTSP_MASK_EXT_G711P1_R2A_80 | VTSP_MASK_EXT_G711P1_R2B_80 | VTSP_MASK_EXT_G711P1_R1_64)

#define VTSP_MASK_CODER_RATE_SET1_10MS  (1 << 0)

#define VTSP_MASK_CODER_BITRATE_G726_32             (1 << 0)
#define VTSP_MASK_CODER_BITRATE_G723_30MS_53        (1 << 1)
#define VTSP_MASK_CODER_BITRATE_G723_30MS_63        (1 << 2)
#define VTSP_MASK_CODER_BITRATE_G722P1_20MS_24      (1 << 3)
#define VTSP_MASK_CODER_BITRATE_G722P1_20MS_32      (1 << 4)
#define VTSP_MASK_CODER_BITRATE_GAMRNB_20MS_475     (1 << 5)
#define VTSP_MASK_CODER_BITRATE_GAMRNB_20MS_515     (1 << 6)
#define VTSP_MASK_CODER_BITRATE_GAMRNB_20MS_59      (1 << 7)
#define VTSP_MASK_CODER_BITRATE_GAMRNB_20MS_67      (1 << 8)
#define VTSP_MASK_CODER_BITRATE_GAMRNB_20MS_74      (1 << 9)
#define VTSP_MASK_CODER_BITRATE_GAMRNB_20MS_795     (1 << 10)
#define VTSP_MASK_CODER_BITRATE_GAMRNB_20MS_102     (1 << 11)
#define VTSP_MASK_CODER_BITRATE_GAMRNB_20MS_122     (1 << 12)
#define VTSP_MASK_CODER_BITRATE_GAMRWB_20MS_660     (1 << 5)
#define VTSP_MASK_CODER_BITRATE_GAMRWB_20MS_885     (1 << 6)
#define VTSP_MASK_CODER_BITRATE_GAMRWB_20MS_1265    (1 << 7)
#define VTSP_MASK_CODER_BITRATE_GAMRWB_20MS_1425    (1 << 8)
#define VTSP_MASK_CODER_BITRATE_GAMRWB_20MS_1585    (1 << 9)
#define VTSP_MASK_CODER_BITRATE_GAMRWB_20MS_1825    (1 << 10)
#define VTSP_MASK_CODER_BITRATE_GAMRWB_20MS_1985    (1 << 11)
#define VTSP_MASK_CODER_BITRATE_GAMRWB_20MS_2305    (1 << 12)
#define VTSP_MASK_CODER_BITRATE_GAMRWB_20MS_2385    (1 << 13)
#define VTSP_MASK_CODER_BITRATE_G711P1_R3_96        (1 << 14)
#define VTSP_MASK_CODER_BITRATE_G711P1_R2_80        (1 << 15)
#define VTSP_MASK_CODER_BITRATE_G711P1_R1_64        (1 << 16)

#define VTSP_DETECT_DTMF                (1 << 0)
#define VTSP_DETECT_VAD                 (1 << 1)
#define VTSP_DETECT_FMTD                (1 << 2)
#define VTSP_DETECT_CALLERID_ONHOOK     (1 << 3)    /* FXO only */
#define VTSP_DETECT_CALLERID_OFFHOOK    (1 << 4)    /* FXO only */
#define VTSP_DETECT_UTD                 (1 << 5)    /* FXO only */

#define VTSP_DETECT_KEY                 (1 << 6)
#define VTSP_DETECT_REMOTE_DTMF         (1 << 7)

#define VTSP_TONE_BREAK_MIX         (1 << 0)

#define VTSP_RING_NMAX              (uint16)(~0)
#define VTSP_RING_TMAX              (uint32)(~0)
#define VTSP_TONE_NMAX              (uint16)(~0)
#define VTSP_TONE_TMAX              (uint32)(~0)

#define VTSP_DIGIT_MAX_SZ           (33)
#define VTSP_EXTMAP_URI_SZ          (128)

#define VTSP_CODER_G711U            (0)
#define VTSP_CODER_G711A            (1)
#define VTSP_CODER_CN               (2)
#define VTSP_CODER_G729             (3)
#define VTSP_CODER_G726_32K         (4)
#define VTSP_CODER_ILBC_20MS        (5)
#define VTSP_CODER_ILBC_30MS        (6)
#define VTSP_CODER_G723_30MS        (7)
#define VTSP_CODER_DTMF             (8)
#define VTSP_CODER_T38              (9)
#define VTSP_CODER_16K_MU           (10)
#define VTSP_CODER_G722             (11)
#define VTSP_CODER_G722P1_20MS      (12)
#define VTSP_CODER_TONE             (13)
#define VTSP_CODER_GAMRNB_20MS_OA   (14) /* Octet-Align format. */
#define VTSP_CODER_GAMRNB_20MS_BE   (15) /* Bandwidth-Efficient format. */
#define VTSP_CODER_GAMRWB_20MS_OA   (16) /* Octet-Align format. */
#define VTSP_CODER_GAMRWB_20MS_BE   (17) /* Bandwidth-Efficient format. */
#define VTSP_CODER_G711P1U          (18)
#define VTSP_CODER_G711P1A          (19)
#define VTSP_CODER_SILK_20MS_8K     (20)
#define VTSP_CODER_SILK_20MS_16K    (21)
#define VTSP_CODER_SILK_20MS_24K    (22)

#define VTSP_ENCODER_NUM            (VTSP_CODER_SILK_20MS_24K+1)
#define VTSP_DECODER_NUM            (VTSP_CODER_SILK_20MS_24K+1)
#define VTSP_CODER_NUM              (VTSP_ENCODER_NUM)  

#define VTSP_CODER_VIDEO_H264       (0)
#define VTSP_CODER_VIDEO_H263       (1)

#define VTSP_ENCODER_VIDEO_NUM      (VTSP_CODER_VIDEO_H263+1)
#define VTSP_DECODER_VIDEO_NUM      (VTSP_CODER_VIDEO_H263+1)
#define VTSP_CODER_VIDEO_NUM        (VTSP_ENCODER_VIDEO_NUM)  

#define VTSP_CODER_UNAVAIL          (uint16)(~0)
#define VTSP_CODER_ABORT            (uint16)(~1)

/* The below constant names are reserved for future use
 */
#define VTSP_CODER_G726_40K         (VTSP_CODER_UNAVAIL)
#define VTSP_CODER_G726_24K         (VTSP_CODER_UNAVAIL)
#define VTSP_CODER_G726_16K         (VTSP_CODER_UNAVAIL)

#define VTSP_MASK_CODER_G711U       (1 << VTSP_CODER_G711U)
#define VTSP_MASK_CODER_G711A       (1 << VTSP_CODER_G711A)
#define VTSP_MASK_CODER_CN          (1 << VTSP_CODER_CN)
#define VTSP_MASK_CODER_G729        (1 << VTSP_CODER_G729)
#define VTSP_MASK_CODER_G726_32K    (1 << VTSP_CODER_G726_32K)
#define VTSP_MASK_CODER_DTMF        (1 << VTSP_CODER_DTMF)
#define VTSP_MASK_CODER_ILBC_20MS   (1 << VTSP_CODER_ILBC_20MS)
#define VTSP_MASK_CODER_ILBC_30MS   (1 << VTSP_CODER_ILBC_30MS)
#define VTSP_MASK_CODER_G723_30MS   (1 << VTSP_CODER_G723_30MS)
#define VTSP_MASK_CODER_T38         (1 << VTSP_CODER_T38)
#define VTSP_MASK_CODER_16K_MU      (1 << VTSP_CODER_16K_MU)
#define VTSP_MASK_CODER_G722        (1 << VTSP_CODER_G722)
#define VTSP_MASK_CODER_G722P1_20MS (1 << VTSP_CODER_G722P1_20MS)
#define VTSP_MASK_CODER_G722P2      (1 << VTSP_CODER_G722P2)
#define VTSP_MASK_CODER_TONE        (1 << VTSP_CODER_TONE)
#define VTSP_MASK_CODER_GAMRNB_20MS_OA (1 << VTSP_CODER_GAMRNB_20MS_OA)
#define VTSP_MASK_CODER_GAMRNB_20MS_BE (1 << VTSP_CODER_GAMRNB_20MS_BE)
#define VTSP_MASK_CODER_GAMRWB_20MS_OA (1 << VTSP_CODER_GAMRWB_20MS_OA)
#define VTSP_MASK_CODER_GAMRWB_20MS_BE (1 << VTSP_CODER_GAMRWB_20MS_BE)
#define VTSP_MASK_CODER_G711P1U     (1 << VTSP_CODER_G711P1U)
#define VTSP_MASK_CODER_G711P1A     (1 << VTSP_CODER_G711P1A)
#define VTSP_MASK_CODER_VIDEO_H263  (1 << VTSP_CODER_VIDEO_H263)
#define VTSP_MASK_CODER_VIDEO_H264  (1 << VTSP_CODER_VIDEO_H264)
#define VTSP_MASK_CODER_SILK_20MS_8K  (1 << VTSP_CODER_SILK_20MS_8K)
#define VTSP_MASK_CODER_SILK_20MS_16K (1 << VTSP_CODER_SILK_20MS_16K)
#define VTSP_MASK_CODER_SILK_20MS_24K (1 << VTSP_CODER_SILK_20MS_24K)

#define VTSP_BLOCK_LINEAR_5MS_SZ    (80)    /* bytes in 5ms linear audio */
#define VTSP_BLOCK_LINEAR_10MS_SZ   (160)   /* bytes in 10ms linear audio */
#define VTSP_BLOCK_G711_10MS_SZ     (80)    /* bytes in 10ms G711 u,a audio */
#define VTSP_BLOCK_G711_CN_SZ       (1)     /* bytes in G711 u,a CN audio */
#define VTSP_BLOCK_G729_10MS_SZ     (10)    /* bytes in 10ms G729 audio */
#define VTSP_BLOCK_G729_SID_SZ      (2)     /* bytes in 10ms G729 SID */
#define VTSP_BLOCK_G726_32K_10MS_SZ (40)    /* bytes in 10ms G726 audio */
#define VTSP_BLOCK_G726_32K_SID_SZ  (1)     /* bytes in CN for G.726 */
#define VTSP_BLOCK_ILBC_20MS_SZ     (38)    /* bytes in 20ms for ILBC */
#define VTSP_BLOCK_ILBC_30MS_SZ     (50)    /* bytes in 30ms for ILBC */
#define VTSP_BLOCK_G723_30MS_63_SZ  (24)    /* bytes in 30ms for 6.3k/s*/
#define VTSP_BLOCK_G723_30MS_53_SZ  (20)    /* bytes in 30ms for 5.3k/s*/
#define VTSP_BLOCK_G723_SID_SZ      (4)     /* bytes in 30ms for G723 SID */
#define VTSP_BLOCK_16K_MU_SZ        (160)   /* bytes in 10ms Mulaw @16K samps */
#define VTSP_BLOCK_G722_SZ          (80)    /* bytes in 10ms for G722 */
#define VTSP_BLOCK_G722P1_24KBPS_20MS_SZ (60)  /* bytes for G722P1 @ 24 kpbs */
#define VTSP_BLOCK_G722P1_32KBPS_20MS_SZ (80)  /* bytes for G722P1 @ 32 kpbs */
#define VTSP_BLOCK_G722P1_48KBPS_20MS_SZ (120) /* bytes for G722P1 @ 48 kpbs */
#define VTSP_BLOCK_G722P2_SZ        (XXX var)    /* TBD */

/* AMR octet-align format payload size including CMR. */
#define VTSP_BLOCK_GAMRNB_20MS_OA_MR122_SZ  (33)  /* bytes in 20ms for 12.2kb/s  */
#define VTSP_BLOCK_GAMRNB_20MS_OA_MR102_SZ  (28)  /* bytes in 20ms for 10.2kb/s  */
#define VTSP_BLOCK_GAMRNB_20MS_OA_MR792_SZ  (22)  /* bytes in 20ms for 7.95kb/s  */
#define VTSP_BLOCK_GAMRNB_20MS_OA_MR74_SZ   (21)  /* bytes in 20ms for 7.4 kb/s  */
#define VTSP_BLOCK_GAMRNB_20MS_OA_MR67_SZ   (19)  /* bytes in 20ms for 6.7 kb/s  */
#define VTSP_BLOCK_GAMRNB_20MS_OA_MR59_SZ   (17)  /* bytes in 20ms for 5.9 kb/s  */
#define VTSP_BLOCK_GAMRNB_20MS_OA_MR515_SZ  (15)  /* bytes in 20ms for 5.15kb/s  */
#define VTSP_BLOCK_GAMRNB_20MS_OA_MR475_SZ  (14)  /* bytes in 20ms for 4.75kb/s  */
#define VTSP_BLOCK_GAMRNB_20MS_OA_MRDTX_SZ  (7)   /* bytes in 20ms for GAMRNB SID */
/* AMR bandwidth-efficient format payload size including CMR. */
#define VTSP_BLOCK_GAMRNB_20MS_BE_MR122_SZ  (32)  /* bytes in 20ms for 12.2kb/s  */
#define VTSP_BLOCK_GAMRNB_20MS_BE_MR102_SZ  (27)  /* bytes in 20ms for 10.2kb/s  */
#define VTSP_BLOCK_GAMRNB_20MS_BE_MR792_SZ  (22)  /* bytes in 20ms for 7.95kb/s  */
#define VTSP_BLOCK_GAMRNB_20MS_BE_MR74_SZ   (20)  /* bytes in 20ms for 7.4 kb/s  */
#define VTSP_BLOCK_GAMRNB_20MS_BE_MR67_SZ   (18)  /* bytes in 20ms for 6.7 kb/s  */
#define VTSP_BLOCK_GAMRNB_20MS_BE_MR59_SZ   (16)  /* bytes in 20ms for 5.9 kb/s  */
#define VTSP_BLOCK_GAMRNB_20MS_BE_MR515_SZ  (15)  /* bytes in 20ms for 5.15kb/s  */
#define VTSP_BLOCK_GAMRNB_20MS_BE_MR475_SZ  (14)  /* bytes in 20ms for 4.75kb/s  */
#define VTSP_BLOCK_GAMRNB_20MS_BE_MRDTX_SZ  (7)   /* bytes in 20ms for GAMRNB SID */
/* AMR octet-align format payload size including CMR. */
#define VTSP_BLOCK_GAMRWB_20MS_OA_MR2385_SZ (62)  /* bytes in 20ms for 23.85kb/s  */
#define VTSP_BLOCK_GAMRWB_20MS_OA_MR2305_SZ (60)  /* bytes in 20ms for 23.05kb/s  */
#define VTSP_BLOCK_GAMRWB_20MS_OA_MR1985_SZ (52)  /* bytes in 20ms for 19.85kb/s  */
#define VTSP_BLOCK_GAMRWB_20MS_OA_MR1825_SZ (48)  /* bytes in 20ms for 18.25kb/s  */
#define VTSP_BLOCK_GAMRWB_20MS_OA_MR1585_SZ (42)  /* bytes in 20ms for 15.85kb/s  */
#define VTSP_BLOCK_GAMRWB_20MS_OA_MR1425_SZ (38)  /* bytes in 20ms for 14.25kb/s  */
#define VTSP_BLOCK_GAMRWB_20MS_OA_MR1265_SZ (34)  /* bytes in 20ms for 12.65kb/s  */
#define VTSP_BLOCK_GAMRWB_20MS_OA_MR885_SZ  (25)  /* bytes in 20ms for 8.85 kb/s  */
#define VTSP_BLOCK_GAMRWB_20MS_OA_MR660_SZ  (19)  /* bytes in 20ms for 6.60 kb/s  */
#define VTSP_BLOCK_GAMRWB_20MS_OA_MRDTX_SZ  (7)   /* bytes in 20ms for GAMRWB SID */
/* AMRWB bandwidth-efficient format payload size including CMR. */
#define VTSP_BLOCK_GAMRWB_20MS_BE_MR2385_SZ (61)  /* bytes in 20ms for 23.85kb/s  */
#define VTSP_BLOCK_GAMRWB_20MS_BE_MR2305_SZ (59)  /* bytes in 20ms for 23.05kb/s  */
#define VTSP_BLOCK_GAMRWB_20MS_BE_MR1985_SZ (51)  /* bytes in 20ms for 19.85kb/s  */
#define VTSP_BLOCK_GAMRWB_20MS_BE_MR1825_SZ (47)  /* bytes in 20ms for 18.25kb/s  */
#define VTSP_BLOCK_GAMRWB_20MS_BE_MR1585_SZ (41)  /* bytes in 20ms for 15.85kb/s  */
#define VTSP_BLOCK_GAMRWB_20MS_BE_MR1425_SZ (37)  /* bytes in 20ms for 14.25kb/s  */
#define VTSP_BLOCK_GAMRWB_20MS_BE_MR1265_SZ (33)  /* bytes in 20ms for 12.65kb/s  */
#define VTSP_BLOCK_GAMRWB_20MS_BE_MR885_SZ  (24)  /* bytes in 20ms for 8.85 kb/s  */
#define VTSP_BLOCK_GAMRWB_20MS_BE_MR660_SZ  (18)  /* bytes in 20ms for 6.60 kb/s  */
#define VTSP_BLOCK_GAMRWB_20MS_BE_MRDTX_SZ  (7)   /* bytes in 20ms for GAMRWB SID */
/* AMR-NB/WB NO-DATA fromat payload size including CMR */
#define VTSP_BLOCK_GAMR_NODATA_SZ           (2)

#define VTSP_BLOCK_G711P1_R3_SZ     (60) /* bytes in 5ms for G711P1 Mode R3 */
#define VTSP_BLOCK_G711P1_R2_SZ     (50) /* bytes in 5ms for G711P1 Mode R2 */
#define VTSP_BLOCK_G711P1_R1_SZ     (40)  /* bytes in 5ms for G711P1 Mode R1 */
#define VTSP_BLOCK_SILK_20MS_SZ     (250) /* max bytes in 20ms 24kHz @ 40kbps */

#define VTSP_BLOCK_MAX_ENC_BYTES_SZ (VTSP_BLOCK_SILK_20MS_SZ)

/* max bytes in stream payload XXX - could change if 32k sampling added */
#define VTSP_BLOCK_MAX_SZ           (VTSP_BLOCK_16K_MU_SZ)

/* In the FromNetwork direction, VTSP accepts multiple blocks per payload.
 * In the ToNetwork direction, VTSP sends single blocks per payload.
 */
#define VTSP_PAYLOAD_TO_NET_MAX_SZ    (VTSP_BLOCK_MAX_SZ)
#define VTSP_PAYLOAD_FROM_NET_MAX_SZ  (4 * VTSP_BLOCK_MAX_SZ)

#define VTSP_STREAM_PEER_NETWORK    (uint16)(~1)

#define VTSP_HEARTBEAT_EVENT_COUNT  (1000)  /* * 10ms = 10sec */ 

/* Event Message constants
 */
#define VTSP_INFC_GLOBAL            (uint16)(~0)
#define VTSP_INFC_ANY               (uint16)(~1)
#define VTSP_INFC_PSTN              (uint16)(~2)
#define VTSP_INFC_FXS               (uint16)(0)
#define VTSP_INFC_VIDEO             (uint16)(256)

#define VTSP_TIMEOUT_NO_WAIT        (OSAL_NO_WAIT)
#define VTSP_TIMEOUT_FOREVER        (OSAL_WAIT_FOREVER)

/* VTSP stun packet payload size - Bug 2684 */
#define VTSP_STUN_PAYLOAD_SZ       (512)

/* Echo cancellation tail length constants */
#define VTSP_EC_TAIL_LENGTH_16MS   (128)
#define VTSP_EC_TAIL_LENGTH_32MS   (256)
#define VTSP_EC_TAIL_LENGTH_48MS   (384)
#define VTSP_EC_TAIL_LENGTH_64MS   (512)
#define VTSP_EC_TAIL_LENGTH_128MS  (1024)

/*
 * Flow direction constants.
 */
typedef enum { 
    VTSP_FLOW_DIR_LOCAL_PLAY    = (1 << 0),
    VTSP_FLOW_DIR_LOCAL_RECORD  = (1 << 1),
    VTSP_FLOW_DIR_PEER_PLAY     = (1 << 2),
    VTSP_FLOW_DIR_PEER_RECORD   = (1 << 3)
} VTSP_FlowDir;

/*
 * Flow stop mask constants
 */
#define VTSP_FLOW_STOP_DTMF_0       (1 << 0)
#define VTSP_FLOW_STOP_DTMF_1       (1 << 1)
#define VTSP_FLOW_STOP_DTMF_2       (1 << 2)
#define VTSP_FLOW_STOP_DTMF_3       (1 << 3)
#define VTSP_FLOW_STOP_DTMF_4       (1 << 4)
#define VTSP_FLOW_STOP_DTMF_5       (1 << 5)
#define VTSP_FLOW_STOP_DTMF_6       (1 << 6)
#define VTSP_FLOW_STOP_DTMF_7       (1 << 7)
#define VTSP_FLOW_STOP_DTMF_8       (1 << 8)
#define VTSP_FLOW_STOP_DTMF_9       (1 << 9)
#define VTSP_FLOW_STOP_DTMF_STAR    (1 << 10)
#define VTSP_FLOW_STOP_DTMF_POUND   (1 << 11)
#define VTSP_FLOW_STOP_DTMF_A       (1 << 12)
#define VTSP_FLOW_STOP_DTMF_B       (1 << 13)
#define VTSP_FLOW_STOP_DTMF_C       (1 << 14)
#define VTSP_FLOW_STOP_DTMF_D       (1 << 15)
#define VTSP_FLOW_STOP_DTMF_ANY     ((1 << 16) - 1)
#define VTSP_FLOW_STOP_ANY          (VTSP_FLOW_STOP_DTMF_ANY)

/*
 * RTCP constants
 */
#define VTSP_MASK_RTCP_SR           (1 << 0)
#define VTSP_MASK_RTCP_XR           (1 << 1)
#define VTSP_MASK_RTCP_RR           (1 << 2)
#define VTSP_MASK_RTCP_BYE          (1 << 3)
#define VTSP_MASK_RTCP_FB_NACK      (1 << 4)
#define VTSP_MASK_RTCP_FB_PLI       (1 << 5)
#define VTSP_MASK_RTCP_FB_FIR       (1 << 6)
#define VTSP_MASK_RTCP_FB_TMMBR     (1 << 7)
#define VTSP_MASK_RTCP_FB_TMMBN     (1 << 8)

/*
 * VTSP_Return
 * The assigned values are subject to change.
 */
typedef enum { 
    VTSP_OK             =  1,
    VTSP_E_INIT         = -1,
    VTSP_E_HW           = -2,
    VTSP_E_CONFIG       = -3,
    VTSP_E_SHUTDOWN     = -4,
    VTSP_E_TEMPL_NUM    = -5,
    VTSP_E_TEMPL        = -6,
    VTSP_E_TIMEOUT      = -7,
    VTSP_E_NO_MSG       = -8,
    VTSP_E_INFC         = -9,
    VTSP_E_INFC_HW      = -10,
    VTSP_E_STREAM_NUM   = -11,
    VTSP_E_STREAM_DATA  = -12,
    VTSP_E_STREAM_NA    = -13,
    VTSP_E_RESOURCE     = -14,
    VTSP_E_CID_EOT      = -15,
    VTSP_E_CID_SZ       = -16,
    VTSP_E_DIGIT        = -17,
    VTSP_E_FIFO_ERROR   = -18,
    VTSP_E_JB_ERROR     = -19,
    VTSP_E_ARG          = -20,
    VTSP_E_FLOW_ID      = -21,
    VTSP_E_FLOW_BUSY    = -22,
    VTSP_E_FLOW_DONE    = -23,
    VTSP_E_CODER_TYPE   = -24,
    VTSP_E_FLOW_DATA    = -25,
    VTSP_E_CONTROL      = -26,
    VTSP_E_FLOW_DIR     = -27,
    VTSP_E_FLOW_SESSION = -28
} VTSP_Return;

typedef enum {
    VTSP_TEMPL_CODE_NONE       = 0,
    VTSP_TEMPL_CODE_CODEC      = 1,
    VTSP_TEMPL_CODE_JB         = 2,
    VTSP_TEMPL_CODE_FIFO       = 3,
    VTSP_TEMPL_CODE_RING       = 4,
    VTSP_TEMPL_CODE_TONE       = 5,
    VTSP_TEMPL_CODE_DIGIT      = 6,
    VTSP_TEMPL_CODE_DTMF_DET   = 7,
    VTSP_TEMPL_CODE_EC         = 8,
    VTSP_TEMPL_CODE_SOFT_GAIN  = 9,
    VTSP_TEMPL_CODE_REF_LEVEL  = 10,
    VTSP_TEMPL_CODE_NFE        = 11,
    VTSP_TEMPL_CODE_FSKS       = 12,
    VTSP_TEMPL_CODE_FSKR       = 13,
    VTSP_TEMPL_CODE_FMTD       = 14,
    VTSP_TEMPL_CODE_CID        = 15,
    VTSP_TEMPL_CODE_VAD        = 16,
    VTSP_TEMPL_CODE_TIC        = 17,
    VTSP_TEMPL_CODE_UTD        = 18,
    VTSP_TEMPL_CODE_LOOP       = 19,
    VTSP_TEMPL_CODE_LOOP_X     = 20,
    VTSP_TEMPL_CODE_LOOP_X2    = 21,
    VTSP_TEMPL_CODE_TONE_QUAD  = 22,
    VTSP_TEMPL_CODE_RTCP       = 23,
    VTSP_TEMPL_CODE_AEC        = 24,
    VTSP_TEMPL_CODE_RTP        = 25,
    VTSP_TEMPL_CODE_FR38       = 26,
    VTSP_TEMPL_CODE_DTMF       = 27,
    VTSP_TEMPL_CODE_CN         = 28,
    VTSP_TEMPL_CODE_DEBUG      = 29
} VTSP_TemplateCode;

typedef enum { 
    VTSP_TEMPL_CONTROL_GR909_HEMF       = 0,
    VTSP_TEMPL_CONTROL_GR909_FEMF       = 1,
    VTSP_TEMPL_CONTROL_GR909_RFAULT     = 2,
    VTSP_TEMPL_CONTROL_GR909_ROFFHOOK   = 3,
    VTSP_TEMPL_CONTROL_GR909_REN        = 4,
    VTSP_TEMPL_CONTROL_GR909_ALL        = 5
} VTSP_TemplateControlGr909;

typedef enum {
    VTSP_TEMPL_CONTROL_CID_FORMAT       = 0,
    VTSP_TEMPL_CONTROL_CID_MDID_TIMEOUT = 1,
    VTSP_TEMPL_CONTROL_CID_FSK_PWR      = 2,
    VTSP_TEMPL_CONTROL_CID_SAS_PWR      = 3,
    VTSP_TEMPL_CONTROL_CID_CAS_PWR      = 4
} VTSP_TemplateCidControl;

typedef enum { 
    VTSP_TEMPL_CONTROL_EC_NORMAL      = 0,
    VTSP_TEMPL_CONTROL_EC_BYPASS      = 1,
    VTSP_TEMPL_CONTROL_EC_FREEZE      = 2,
    VTSP_TEMPL_CONTROL_EC_FMTD        = 3,
    VTSP_TEMPL_CONTROL_EC_TAIL_LENGTH = 4
} VTSP_TemplateControlEchoCanc;

typedef enum { 
    VTSP_TEMPL_CONTROL_FR38_RATE_MAX_2400 = 0,
    VTSP_TEMPL_CONTROL_FR38_RATE_MAX_4800,
    VTSP_TEMPL_CONTROL_FR38_RATE_MAX_7200,
    VTSP_TEMPL_CONTROL_FR38_RATE_MAX_9600,
    VTSP_TEMPL_CONTROL_FR38_RATE_MAX_12200,
    VTSP_TEMPL_CONTROL_FR38_RATE_MAX_14400
} VTSP_TemplateControlFr38RateMax;

typedef enum {
    VTSP_TEMPL_CONTROL_FR38_RATE_MGMT_NO_PASS_TCF = 1,
    VTSP_TEMPL_CONTROL_FR38_RATE_MGMT_PASS_TCF    = 2,
} VTSP_TemplateControlFr38RateMgmt;

typedef enum {
    VTSP_TEMPL_CONTROL_FR38_ECM_MODE_DISABLE = 0,
    VTSP_TEMPL_CONTROL_FR38_ECM_MODE_ENABLE
} VTSP_TemplateControlFr38EcmMode;

typedef enum {
    VTSP_TEMPL_CONTROL_FR38_ECC_MODE_NONE = 0,
    VTSP_TEMPL_CONTROL_FR38_ECC_MODE_RED,
    VTSP_TEMPL_CONTROL_FR38_ECC_MODE_FEC
} VTSP_TemplateControlFr38EccMode;

typedef enum {
    VTSP_TEMPL_CONTROL_FR38_RATE_MAX   = 0,
    VTSP_TEMPL_CONTROL_FR38_JITTER_MAX = 1,
    VTSP_TEMPL_CONTROL_FR38_ECC        = 2,
    VTSP_TEMPL_CONTROL_FR38_RATE_MGMT  = 3,
    VTSP_TEMPL_CONTROL_FR38_ECM_MODE   = 4,
    VTSP_TEMPL_CONTROL_FR38_CED_LEN    = 5,
} VTSP_TemplateControlFr38;

typedef enum { 
    VTSP_TEMPL_CONTROL_AEC_NORMAL         = 0,
    VTSP_TEMPL_CONTROL_AEC_BYPASS         = 1,
    VTSP_TEMPL_CONTROL_AEC_BYPASS_AGC     = 2,
    VTSP_TEMPL_CONTROL_AEC_HANDSET_MODE   = 3,
    VTSP_TEMPL_CONTROL_AEC_HANDSFREE_MODE = 4,
    VTSP_TEMPL_CONTROL_AEC_TAIL_LENGTH    = 5,
    VTSP_TEMPL_CONTROL_AEC_FREEZE         = 6,
    VTSP_TEMPL_CONTROL_AEC_COMFORT_NOISE  = 7,
    VTSP_TEMPL_CONTROL_AEC_HALF_DUPLEX    = 8
} VTSP_TemplateControlAec;

typedef enum { 
    VTSP_TEMPL_CONTROL_CN_POWER_ATTEN = 0,
} VTSP_TemplateControlCn;

typedef enum { 
    VTSP_TEMPL_CONTROL_JB_VOICE  = 0,
    VTSP_TEMPL_CONTROL_JB_FIXED  = 1,
    VTSP_TEMPL_CONTROL_JB_FREEZE = 2,
} VTSP_TemplateControlJb;

typedef enum { 
    VTSP_TEMPL_CONTROL_RTCP_INTERVAL = 0,
    VTSP_TEMPL_CONTROL_RTCP_MASK     = 1,
    VTSP_TEMPL_CONTROL_RTCP_TOS      = 2,
    VTSP_TEMPL_CONTROL_RTCP_FB       = 3,
} VTSP_TemplateControlRtcp;

typedef enum { 
    VTSP_TEMPL_CONTROL_RTP_TOS       = 0,
    VTSP_TEMPL_CONTROL_RTP_TS_INIT   = 1,
    VTSP_TEMPL_CONTROL_RTP_SN_INIT   = 2,
    VTSP_TEMPL_CONTROL_RTP_REDUNDANT = 3
} VTSP_TemplateControlRtp;

typedef enum { 
    VTSP_TEMPL_UTD_TONE_TYPE_MOD    = 1,
    VTSP_TEMPL_UTD_TONE_TYPE_CNG    = 2,
    VTSP_TEMPL_UTD_TONE_TYPE_RING   = 3,
    VTSP_TEMPL_UTD_TONE_TYPE_BUSY   = 4,
    VTSP_TEMPL_UTD_TONE_TYPE_REOR   = 5,
    VTSP_TEMPL_UTD_TONE_TYPE_NU     = 6,
    VTSP_TEMPL_UTD_TONE_TYPE_SIT    = 7,
    VTSP_TEMPL_UTD_TONE_TYPE_DIAL   = 8,
    VTSP_TEMPL_UTD_TONE_TYPE_UNK    = 9
} VTSP_TemplateUtdToneType;

typedef enum { 
    VTSP_TEMPL_UTD_TONE_SINGLE  = 0,
    VTSP_TEMPL_UTD_TONE_DUAL    = 1,
    VTSP_TEMPL_UTD_TONE_MOD     = 2,
} VTSP_TemplateUtdToneControlDual;

typedef enum { 
    VTSP_TEMPL_UTD_SIT_US       = 1,
    VTSP_TEMPL_UTD_SIT_EURO     = 2
} VTSP_TemplateUtdToneControlSIT;

typedef enum {
    VTSP_TEMPL_UTD_MASK_ALL     = (~0),
    VTSP_TEMPL_UTD_MASK_MOD     = 0x0100,
    VTSP_TEMPL_UTD_MASK_CNG     = 0x0080,
    VTSP_TEMPL_UTD_MASK_RING    = 0x0040,
    VTSP_TEMPL_UTD_MASK_BUSY    = 0x0020,
    VTSP_TEMPL_UTD_MASK_REOR    = 0x0010,
    VTSP_TEMPL_UTD_MASK_NU      = 0x0008,
    VTSP_TEMPL_UTD_MASK_SIT     = 0x0004,
    VTSP_TEMPL_UTD_MASK_DIAL    = 0x0002,
    VTSP_TEMPL_UTD_MASK_UNK     = 0x0001,
} VTSP_TemplateUtdMask;

typedef enum {
    VTSP_TEMPL_CONTROL_UTD_MASK = 0,
    VTSP_TEMPL_CONTROL_UTD_TONE = 1
} VTSP_TemplateControlUtd;

typedef enum {
    VTSP_TEMPL_CONTROL_TONE_QUAD_MONO  = 0x0000,
    VTSP_TEMPL_CONTROL_TONE_QUAD_DUAL  = 0x0000,
    VTSP_TEMPL_CONTROL_TONE_QUAD_TRI   = 0x0000,
    VTSP_TEMPL_CONTROL_TONE_QUAD_QUAD  = 0x0000,
    VTSP_TEMPL_CONTROL_TONE_QUAD_MOD   = 0x4400,
    VTSP_TEMPL_CONTROL_TONE_QUAD_SIT   = 0x1230,
    VTSP_TEMPL_CONTROL_TONE_QUAD_BONG  = 0x11AA,
    VTSP_TEMPL_CONTROL_TONE_QUAD_DELTA = 0x8888
} VTSP_TemplateControlToneQuad;


typedef enum {
    VTSP_TEMPL_CONTROL_FMTD_GBL_PWR_MIN_INFC = 0,
    VTSP_TEMPL_CONTROL_FMTD_GBL_PWR_MIN_PEER = 1,
    VTSP_TEMPL_CONTROL_FMTD_DETECT_MASK      = 2 
} VTSP_TemplateControlFmtd;

typedef enum { 
    VTSP_CONTROL_INFC_IO_AUDIO_DRIVER_ENABLE  = 0,
    VTSP_CONTROL_INFC_IO_AUDIO_DRIVER_DISABLE = 1
} VTSP_TemplateControlInfcIOAudioAttach;

typedef enum { 
    VTSP_CONTROL_INFC_POWER_OPEN   = 1,
    VTSP_CONTROL_INFC_POWER_TEST   = 2,
    VTSP_CONTROL_INFC_POWER_ACTIVE = 3,
    VTSP_CONTROL_INFC_POWER_DOWN   = 4,
    VTSP_CONTROL_INFC_POLARITY_FWD = 5,
    VTSP_CONTROL_INFC_POLARITY_REV = 6,
    VTSP_CONTROL_INFC_ONHOOK       = 7,
    VTSP_CONTROL_INFC_OFFHOOK      = 8,
    VTSP_CONTROL_INFC_FLASH        = 9
} VTSP_ControlInfcLine;

typedef enum { 
    VTSP_CONTROL_INFC_IO_FXS_TO_PSTN       = 1,
    VTSP_CONTROL_INFC_IO_LED               = 2,
    VTSP_CONTROL_INFC_IO_GAIN_TX           = 3,
    VTSP_CONTROL_INFC_IO_GAIN_RX           = 4,
    VTSP_CONTROL_INFC_IO_MUTE_INPUT_SW     = 5,
    VTSP_CONTROL_INFC_IO_MUTE_INPUT_HW     = 6,
    VTSP_CONTROL_INFC_IO_MUTE_OUTPUT_SW    = 7,
    VTSP_CONTROL_INFC_IO_MUTE_OUTPUT_HW    = 8,
    VTSP_CONTROL_INFC_IO_INPUT_HANDSET_MIC = 9,
    VTSP_CONTROL_INFC_IO_INPUT_HEADSET_MIC = 10,
    VTSP_CONTROL_INFC_IO_OUTPUT_SPEAKER    = 11,
    VTSP_CONTROL_INFC_IO_OUTPUT_HANDSET    = 12,
    VTSP_CONTROL_INFC_IO_OUTPUT_HEADSET    = 13,
    VTSP_CONTROL_INFC_IO_LOOPBACK_DIGITAL  = 14,
    VTSP_CONTROL_INFC_IO_LOOPBACK_ANALOG   = 15,
    VTSP_CONTROL_INFC_IO_PULSE_GENERATE    = 16,
    VTSP_CONTROL_INFC_IO_AUDIO_ATTACH      = 17,
    VTSP_CONTROL_INFC_IO_GR909             = 18,
    VTSP_CONTROL_INFC_IO_BALANCED_RINGING  = 19,
    VTSP_CONTROL_INFC_IO_GENERIC           = 20
} VTSP_ControlInfcIO;

typedef enum { 
    VTSP_EVENT_MSG_CODE_TRACE                   = 0,
    VTSP_EVENT_MSG_CODE_RING_GENERATE           = 1,
    VTSP_EVENT_MSG_CODE_RING_DETECT             = 2,
    VTSP_EVENT_MSG_CODE_HOOK                    = 3,
    VTSP_EVENT_MSG_CODE_TONE_GENERATE           = 4,
    VTSP_EVENT_MSG_CODE_TONE_DETECT             = 5,
    VTSP_EVENT_MSG_CODE_DIGIT_GENERATE          = 6,
    VTSP_EVENT_MSG_CODE_TIMER                   = 7,
    VTSP_EVENT_MSG_CODE_CID_DETECT              = 8,
    VTSP_EVENT_MSG_CODE_CID_GENERATE            = 9,
    VTSP_EVENT_MSG_CODE_SHUTDOWN                = 10,
    VTSP_EVENT_MSG_CODE_ERROR                   = 11,
    VTSP_EVENT_MSG_CODE_RTP                     = 12,
    VTSP_EVENT_MSG_CODE_JB                      = 13,
    VTSP_EVENT_MSG_CODE_STATISTIC               = 14,
    VTSP_EVENT_MSG_CODE_FLOW                    = 15,
    VTSP_EVENT_MSG_CODE_RTCP                    = 16,
    VTSP_EVENT_MSG_CODE_T38                     = 18,
    VTSP_EVENT_MSG_CODE_BENCHMARK               = 20,
    VTSP_EVENT_MSG_CODE_KEY_DETECT              = 21,
    VTSP_EVENT_MSG_CODE_EC                      = 22,
    VTSP_EVENT_MSG_CODE_STUN_RECV               = 23,
    VTSP_EVENT_MSG_CODE_STUN_RECV_VIDEO         = 24,
    VTSP_EVENT_MSG_CODE_RESOURCE                = 25,
    VTSP_EVENT_MSG_CODE_GR909                   = 26,
    VTSP_EVENT_MSG_CODE_HW                      = 27,
    VTSP_EVENT_MSG_CODE_LIP_SYNC_AUDIO          = 28,
    VTSP_EVENT_MSG_CODE_LIP_SYNC_VIDEO          = 29,
    VTSP_EVENT_MSG_CODE_VIDEO_DIM               = 30,
    VTSP_EVENT_MSG_CODE_SHUTDOWN_VIDEO          = 31,
    /* Request the video controller to generate a specific type of RTCP feedback */
    VTSP_EVENT_MSG_CODE_RTCP_FEEDBACK_REQUEST   = 32,
} VTSP_EventMsgCodes;

/* 
 * RECOURCE EVT Msg:
 *  VTSP_EventMsg.infc     = (infc)
 *  VTSP_EventMsg.code     = VTSP_EVENT_MSG_CODE_RESOURCE
 *  VTSP_EventMsg.reason   = VTSP_EVENT_REC_EVT
 *  VTSP_EventMsg.streamId = (streamId)
 *  VTSP_EventMsg.evt1     = n/a
 *  VTSP_EventMsg.evt2     = n/a
 *
 */


/* 
 * T38 EVT Msg:
 *  VTSP_EventMsg.infc     = (infc)
 *  VTSP_EventMsg.code     = VTSP_EVENT_MSG_CODE_T38
 *  VTSP_EventMsg.reason   = VTSP_EVENT_T38_EVT
 *  VTSP_EventMsg.streamId = (streamId)
 *  VTSP_EventMsg.evt1     = (mask from VTSP_T38Evt1)
 *  VTSP_EventMsg.evt2     = (mask from VTSP_T38Evt2)
 *
 */
typedef enum {   /* these Evt1 events correspond to FR38 events 1-9 */
     VTSP_T38_EVT1_CRC_E              = (1 << 0),
     VTSP_T38_EVT1_UNRECOV_PKT        = (1 << 1),
     VTSP_T38_EVT1_JB_OFLOW           = (1 << 2),
     VTSP_T38_EVT1_MDM_OFLOW          = (1 << 3),
     VTSP_T38_EVT1_INV_UDP_FRAME      = (1 << 4),
     VTSP_T38_EVT1_CRPT_RED_FRAME     = (1 << 5),
     VTSP_T38_EVT1_CRPT_FEC_FRAME     = (1 << 6),
     VTSP_T38_EVT1_CRPT_PRI_FRAME     = (1 << 7),
     VTSP_T38_EVT1_UDPTL_FRM_E        = (1 << 8)
} VTSP_T38Evt1;

typedef enum {  /* these Evt2 events correspond to FR38 events 34-39 */
     VTSP_T38_EVT2_T1_TE              =  (1 << 0),
     VTSP_T38_EVT2_T3_TE              =  (1 << 1),
     VTSP_T38_EVT2_EPT_TE             =  (1 << 2),
     VTSP_T38_EVT2_V21_CD_T           =  (1 << 3),
     VTSP_T38_EVT2_T30_PREAM_T        =  (1 << 4),
     VTSP_T38_EVT2_INACT_T            =  (1 << 5)
} VTSP_T38Evt2;

typedef enum { 
    VTSP_EVENT_INACTIVE        = 0,
    VTSP_EVENT_ACTIVE          = 1,
    VTSP_EVENT_COMPLETE        = 2,
    VTSP_EVENT_HALTED          = 3,
    VTSP_EVENT_ERROR_INFC      = 4,
    VTSP_EVENT_HOOK_SEIZE      = 5,
    VTSP_EVENT_HOOK_RELEASE    = 6,
    VTSP_EVENT_HOOK_FLASH      = 7,
    VTSP_EVENT_MAX_TIMEOUT     = 8,
    VTSP_EVENT_MAX_REPEAT      = 9,
    VTSP_EVENT_OVERFLOW        = 10,
    VTSP_EVENT_LEADING         = 11,
    VTSP_EVENT_TRAILING        = 12,
    VTSP_EVENT_FLOW_RESOURCE   = 24,
    VTSP_EVENT_RING_BREAK      = 30,
    VTSP_EVENT_RING_MAKE       = 31,
    VTSP_EVENT_RING_DONE       = 32,
    VTSP_EVENT_FAX_PREAMBLE    = 33,
    VTSP_EVENT_FAX_ANSWER      = 34,
    VTSP_EVENT_FAX_CALLING     = 35,
    VTSP_EVENT_FAX_OTHER       = 36,
    VTSP_EVENT_FAX_TIMEOUT     = 37,
    VTSP_EVENT_T38_DISCON      = 38,
    VTSP_EVENT_T38_NOT_FAX     = 39,
    VTSP_EVENT_T38_FRAME_LOSS  = 40,
    VTSP_EVENT_T38_STATISTIC   = 41,
    VTSP_EVENT_T38_EVT         = 42,
    VTSP_EVENT_T38_BUF_FULL    = 43,
    VTSP_EVENT_T38_PKT_SZ_E    = 44,
    VTSP_EVENT_HOOK_POLARITY_FWD = 45,
    VTSP_EVENT_HOOK_POLARITY_REV = 46,
    VTSP_EVENT_HOOK_POWER_DOWN   = 47,
    VTSP_EVENT_HOOK_DISCONNECT = 48,
    VTSP_EVENT_HOOK_PULSE      = 49,
    VTSP_EVENT_RTCP_BYE        = 50,
    VTSP_EVENT_RTCP_RR         = 51,
    VTSP_EVENT_RTCP_SR         = 52,
    VTSP_EVENT_RTCP_SS         = 53,
    VTSP_EVENT_RTCP_MR         = 54,
    VTSP_EVENT_RTCP_CS         = 55,
    VTSP_EVENT_RTCP_XR         = 56,
    VTSP_EVENT_RTCP_FB_GNACK   = 57,
    VTSP_EVENT_RTCP_FB_TMMBR   = 58,
    VTSP_EVENT_RTCP_FB_TMMBN   = 59,
    VTSP_EVENT_RTCP_FB_PLI     = 60,
    VTSP_EVENT_RTCP_FB_FIR     = 61,
    VTSP_EVENT_LIP_SYNC_RTCP_RECEIVED     = 62,
    VTSP_EVENT_LIP_SYNC_RTP_TS            = 63,
    VTSP_EVENT_ECHO_CANC_ON    = 71,
    VTSP_EVENT_ECHO_CANC_OFF   = 72,
    VTSP_EVENT_ECHO_CANC_FRZ   = 73,
    VTSP_EVENT_KEYPAD_PRESS    = 80,
    VTSP_EVENT_KEYPAD_RELEASE  = 81,
    VTSP_EVENT_ERROR_RT        = 82,
    VTSP_EVENT_ERROR_HW_THERMAL = 83,
    VTSP_EVENT_ERROR_HW_POWER  = 84,
    VTSP_EVENT_ERROR_HW_RESET  = 85,
    VTSP_EVENT_CID_PRIMARY_TIMEOUT  = 86,
    VTSP_EVENT_CID_INCOMING_TIMEOUT = 87,
    VTSP_EVENT_TONE_DIR_LOCAL  = 88,
    VTSP_EVENT_TONE_DIR_STREAM = 89, 
    VTSP_EVENT_REC_INFC_AVAILABLE     = 90,
    VTSP_EVENT_REC_INFC_UNAVAILABLE   = 91,
    VTSP_EVENT_REC_STREAM_AVAILABLE   = 92,
    VTSP_EVENT_REC_STREAM_UNAVAILABLE = 93,
    VTSP_EVENT_HW_AUDIO_ATTACH        = 94,
    VTSP_EVENT_HW_AUDIO_DETACH        = 95,
} VTSP_EventConstant;

typedef enum { 
    _VTSP_STREAM_DIR_ENDED   = 0,
    VTSP_STREAM_DIR_INACTIVE = 1,
    VTSP_STREAM_DIR_SENDONLY = 2,
    VTSP_STREAM_DIR_RECVONLY = 3,
    VTSP_STREAM_DIR_SENDRECV = 4
} VTSP_StreamDir;

typedef enum { 
    VTSP_EVENT_TYPE_CID_ONHOOK  = 1,
    VTSP_EVENT_TYPE_CID_OFFHOOK = 2,
    VTSP_EVENT_TYPE_CID_DTMF    = 3
} VTSP_EventTypeCallerId;

typedef enum {
    VTSP_EVENT_TYPE_TONE_BASIC  = 0,
    VTSP_EVENT_TYPE_TONE_QUAD   = 1
} VTSP_EventTypeTone;

typedef enum {
    VTSP_RFC4733_TYPE_EVENT = 0,
    VTSP_RFC4733_TYPE_TONE  = 1
} VTSP_Rfc4733Type;

typedef enum {
    VTSP_EVENT_TYPE_GR909_HEMF     = 0,
    VTSP_EVENT_TYPE_GR909_FEMF     = 1,
    VTSP_EVENT_TYPE_GR909_RFAULT   = 2,
    VTSP_EVENT_TYPE_GR909_ROFFHOOK = 3,
    VTSP_EVENT_TYPE_GR909_REN      = 4
} VTSP_EventTypeGr909;
/* 
 * VTSP_CIDDataFields
 * --------
 *
 * Caller ID Field Types (Parameter or Message Types)
 *
 * See:
 * British Telecom SIN242 Issue 2 page 30
 * "Calling Line Identification Presentation (CLIP)"
 *
 * See:
 * ETS 300 659-1 
 *
 * The format for all CLI messages is :
 *
 * { Message Type, Message Length, { Message... }, Checksum }
 *
 * where
 *
 * { Message...} = { Parameter Type, Parameter Length, Parameter bytes ..., 
 *                      Parameter Type, Parameter Length, Parameter
 *                      bytes... }
 *
 * The Message Type for CLIP is 0x80 indicating 'Supplimentary
 * Information Message'.  Other message types may be used for other
 * purposes.
 *
 * There are eight parameter types associated with CLIP:
 * Parameter Types:
 * 0x01 = Time & date, Length = 8
 * 0x02 = Calling line directory number (DN)
 * 0x03 = Called directory number
 *   Directory number may include the chars: ' ', '-', '(', or ')'
 * 0x04 = Reason for absense of DN, Length = 1
 * 0x07 = Caller name/Text
 * 0x08 = Reason for absence of name, Length = 1
 * 0x0B = Visual Indicator, Length = 1
 * 0x11 = Call Type, Length = 1
 *    Call Type Parameters:
 *      0x01 = Voice Call
 *      0x02 = CLI Ring Back when free call
 *      0x03 = Calling Name Delivery
 *      0x81 = Message Waiting Call
 * 0x13 = Network Message System Status, Length = 1
 *    Network Message System Status Parameters:
 *      0x0 = no messages
 *      0x1 = 1 or some number of messages waiting
 *      0x2 .. = number of messages waiting in the system
 *
 * SIN242: "If Call Type is not sent the TR shall treat the message
 * as Voice Type.  If the Call Type is not recognized by the TR then
 * Call Types with values of 127 and below may be displayed and
 * stored.  Messages with an unrecognized Call Type of 128 and above
 * shall be ignored"
 */
typedef enum { 
    VTSP_CIDDATA_FIELD_RAW         = -1,
    VTSP_CIDDATA_FIELD_FIRST       = -2,
    VTSP_CIDDATA_FIELD_NEXT        = -3,
    VTSP_CIDDATA_FIELD_DATE_TIME   = 0x01,
    VTSP_CIDDATA_FIELD_NUMBER      = 0x02,
    VTSP_CIDDATA_FIELD_BLOCK       = 0x04,
    VTSP_CIDDATA_FIELD_NAME        = 0x07,
    VTSP_CIDDATA_FIELD_VI          = 0x0B,
    VTSP_CIDDATA_FIELD_CALL_TYPE   = 0x11,
    VTSP_CIDDATA_FIELD_NETWORK_MSG = 0x13,
    VTSP_CIDDATA_FIELD_SPECIAL     = 0xFF,
    VTSP_MODEMDID_FIELD_NUMBER     = 0x09
} VTSP_CIDDataFields;

/*
 * CID Country Codes.
 */
typedef enum {
    VTSP_TEMPL_CID_FORMAT_US      = 0,
    VTSP_TEMPL_CID_FORMAT_JP      = 1,
    VTSP_TEMPL_CID_FORMAT_UK_FSK  = 2,
    VTSP_TEMPL_CID_FORMAT_UK_DTMF = 3,
    VTSP_TEMPL_CID_FORMAT_DATA_US = 4,
    VTSP_TEMPL_CID_FORMAT_DATA_JP = 5
} VTSP_TemplateCidFormat;

typedef enum { 
    VTSP_TEMPL_CONTROL_TIC_SET_FLASH_TIME   = 0,
    VTSP_TEMPL_CONTROL_TIC_SET_RELEASE_TIME = 1,
    VTSP_TEMPL_CONTROL_TIC_SET_PDD_PARAMS   = 2
} VTSP_TemplateControlTic;

typedef enum {
    VTSP_TEMPL_CONTROL_DTMF_POWER      = 0,
    VTSP_TEMPL_CONTROL_DTMF_DURATION   = 1,
    VTSP_TEMPL_CONTROL_DTMF_SILENCE    = 2,
    VTSP_TEMPL_CONTROL_DTMF_HI_TWIST   = 3,
    VTSP_TEMPL_CONTROL_DTMF_LO_TWIST   = 4,
    VTSP_TEMPL_CONTROL_DTMF_FRE_DEV    = 5,
    VTSP_TEMPL_CONTROL_DTMF_ERR_FRAMES = 6
} VTSP_TemplateControlDtmf;

typedef enum {
    VTSP_TEMPL_CONTROL_DEBUG_PCMDUMP   = 0,
    VTSP_TEMPL_CONTROL_DEBUG_TRACE     = 1
} VTSP_TemplateControlDebug;

/*
 * SRTP Definitions
 */

#define VTSP_SRTP_KEY_STRING_MAX_LEN     (30)

/*
 * Define enums for SRTP security services
 */
typedef enum {
    VTSP_SRTP_SECURITY_SERVICE_NONE          = 0,    /* no security */
    VTSP_SRTP_SECURITY_SERVICE_CONF_AUTH_80  = 1,    /* conf and auth with 80 bit auth tag */
    VTSP_SRTP_SECURITY_SERVICE_CONF_AUTH_32  = 2     /* conf and auth with 32 bit auth tag */
} VTSP_SRTP_SecurityType;



#endif

