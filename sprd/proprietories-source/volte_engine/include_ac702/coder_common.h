/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 5312 $ $Date: 2008-02-18 19:05:41 -0600 (Mon, 18 Feb 2008) $
 */

#ifndef CODER_COMMON_H_
#define CODER_COMMON_H_

#include <video.h>
/*
 * Max packet fragments in a stream.
 */
#define ENC_MAX_PKTS     (64)

/*
 * Encoder/Decoder objects.
 * Note: These objects are all provate and application has no data in them.
 * Keep this private feature for swapable coders.
 */
typedef struct {
    char            outbuf[VIDEO_NET_BUFFER_SZ];
    uint64          pts;
    int             len;
    int             lastType;                   /* Only for H264 */
    struct {
        uint8      *buf_ptr;
        uint8      *end_ptr;
        uint64      ts;
        int         num;
        int         consumed;                   /* Only for H264 */
        uint8      *start1_ptr[ENC_MAX_PKTS];
        uint8      *start2_ptr[ENC_MAX_PKTS];
        int         index;
        uint8       hdr[8];                     /* Only for H263 */
    } pkt;
} Video_EncObj;

#endif /* CODER_COMMON_H_ */
