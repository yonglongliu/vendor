/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 5312 $ $Date: 2008-02-18 19:05:41 -0600 (Mon, 18 Feb 2008) $
 */


#ifndef _H264_H_
#define _H264_H_

#include <coder_common.h>

#ifdef VIDEO_DEBUG_LOG
# define DBG(fmt, args...) \
        OSAL_logMsg("%s " fmt, __FUNCTION__, ## args)
#define ERR(fmt, args...) \
        OSAL_logMsg("%s ERROR" fmt, __FUNCTION__, ## args)
#else
# define DBG(fmt, args...)
# define ERR(fmt, args...)
#endif

/* NAL Unit Type is the last 5 bits of the first byte. */
#define H264_READ_NALU(byte) ((byte) & (0x1F))
/* Start bit - FU Header byte. */
#define H264_READ_FU_START_BIT(byte) ((byte) >> (7))
/* End bit - FU Header byte. */
#define H264_READ_FU_END_BIT(byte) ((byte) >> (6))

/* ITU-T H246 Recommendation - NALU Types. */
#define NALU_NON_IDR                (1)
#define NALU_PARTITION_A            (2)
#define NALU_PARTITION_B            (3)
#define NALU_PARTITION_C            (4)
#define NALU_IDR                    (5)
/* Supplemental enhancement information (SEI) */
#define NALU_SEI                    (6)
#define NALU_SPS                    (7)
#define NALU_PPS                    (8)
/* RFC 3984 - Section 5.2 */
#define NALU_STAP_A                 (24)
#define NALU_STAP_B                 (25)
#define NALU_MTAP16                 (26)
#define NALU_MTAP24                 (27)
#define NALU_FU_A                   (28)
#define NALU_FU_B                   (29)

/*
 * Maximum RTP payload size
 */
#define H264_ENC_MAX_RTP_SIZE (VIDEO_MAX_RTP_SZ - 204)

int H264_encGetData(
    Video_EncObj  *obj_ptr,
    Video_Packet *pkt_ptr,
    uint8 *data_ptr,
    vint szData,
    uint64 tsMs);

#endif
