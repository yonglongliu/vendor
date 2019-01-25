/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 5312 $ $Date: 2008-02-18 19:05:41 -0600 (Mon, 18 Feb 2008) $
 */

#ifndef _H263_H_
#define _H263_H_

#include <coder_common.h>

/*
 * Max RTP size supported.
 */
#define H263_ENC_MAX_RTP_SIZE (VIDEO_MAX_RTP_SZ - 204)


/*
 * Different mode's header size.
 */
#define H263_MODE_A_HDR_SIZE (4)
#define H263_MODE_B_HDR_SIZE (8)
#define H263_MODE_C_HDR_SIZE (12)

int H263_encGetData(
    Video_EncObj  *obj_ptr,
    Video_Packet *pkt_ptr,
    uint8 *data_ptr,
    vint szData,
    uint64 tsMs);

#endif
