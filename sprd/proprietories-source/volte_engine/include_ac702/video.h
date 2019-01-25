/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12262 $ $Date: 2010-06-10 18:55:56 -0400 (Thu, 10 Jun 2010) $
 */

#ifndef _VIDEO_H_
#define _VIDEO_H_

#include <osal.h>
#include <rtp.h>

#ifdef VIDEO_DEBUG_LOG
# define DBG(fmt, args...) \
        OSAL_logMsg("%s " fmt, __FUNCTION__, ## args)
#define ERR(fmt, args...) \
        OSAL_logMsg("%s ERROR" fmt, __FUNCTION__, ## args)
#else
# define DBG(fmt, args...)
# define ERR(fmt, args...)
#endif

typedef enum {
    VIDEO_FORMAT_NONE = 0,
    VIDEO_FORMAT_YCbCr_420_P,
    VIDEO_FORMAT_YCbCr_420_SP,
    VIDEO_FORMAT_YCrCb_420_SP,
    VIDEO_FORMAT_YUYV,
    VIDEO_FORMAT_RGB_565,
    VIDEO_FORMAT_RGB_32,
    VIDEO_FORMAT_RGB_24,
    VIDEO_FORMAT_ENCODED_H264
} VideoFormat;

/*
 * Maximum RTP payload size
 */
#define VIDEO_MAX_RTP_SZ    (RTP_SAMPLES_MAX)

/*
 * Max width, height, the video engine can handle.
 */
#define VIDEO_WIDTH_MAX     (1280)
#define VIDEO_HEIGHT_MAX    (720)

/*
 * Maximum size of a video packet after re-assembly
 * Give a min compression ratio of 10?
 */
#define VIDEO_NET_BUFFER_SZ (((VIDEO_WIDTH_MAX * VIDEO_HEIGHT_MAX * 2) / 5))

/*
 * Identifies a camera in native.
 */
#define VIDEO_CALLID_CAMERA (0x1FEF)

/*
 * Flags for rotation
 */
#define VIDEO_FLAGS_ROTATE_90         (1)
#define VIDEO_FLAGS_ROTATE_180        (2)
#define VIDEO_FLAGS_ROTATE_270        (4)

/*
 * Video Picture.
 */
typedef struct {
    int              width;
    int              height;
    int              stride;
    int              bpp;
    uint64           ts;
    int              id;
    VideoFormat      format;
    void            *base_ptr;
    int              size;
    int              muted;
    int              rotation;
} Video_Picture;

/*
 * For exchanging packet data with coders.
 */
typedef struct {
    void   *buf_ptr;
    int     sz;
    uint64  ts;
    int     mark;
    int     seqn;
} Video_Packet;

#endif
