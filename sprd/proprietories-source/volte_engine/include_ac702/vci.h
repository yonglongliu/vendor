/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12523 $ $Date: 2010-07-13 18:57:44 -0400 (Tue, 13 Jul 2010) $
 */

#ifndef _VCI_H_
#define _VCI_H_

#include <osal.h>

#define VCI_EVENT_DESC_STRING_SZ (127)

typedef enum {
    VC_EVENT_NONE                   = 0,
    VC_EVENT_INIT_COMPLETE          = 1,
    VC_EVENT_START_ENC              = 2,
    VC_EVENT_START_DEC              = 3,
    VC_EVENT_STOP_ENC               = 4,
    VC_EVENT_STOP_DEC               = 5,
    VC_EVENT_SHUTDOWN               = 6,
    VC_EVENT_REMOTE_RECV_BW_KBPS    = 7,
    VC_EVENT_SEND_KEY_FRAME         = 8,
    VC_EVENT_NO_RTP                 = 9,
    VC_EVENT_PKT_LOSS_RATE          = 10,
    VC_EVENT_RESOLUTION_CHANGED     = 11,
    VC_EVENT_CODEC_TYPE_CHENGED     = 12,
} VC_Event;

typedef struct {
    uint8        *data_ptr;          /* data buffer */
    vint          length;            /* length of data */
    uint64        tsMs;              /* timestamp in milli-second */
    vint          flags;             /* SPS/PPS flags */
    uint8         rcsRtpExtnPayload; /* VCO - payload */
} VC_EncodedFrame;

/*
 * This is the JNI interface used by libvideo.so
 * ----------------------------------------------
 * Video Controller Interface (VCI) is the application side JNI interface to the Video Controller module.
 */

vint VCI_init(
    void);

void VCI_shutdown(
    void);

void VCI_sendFIR(
    void);

vint VCI_getEvent(
    VC_Event *event_ptr,
    char     *eventDesc_ptr,
    vint     *codecType_ptr,
    vint      timeout);

vint VCI_sendEncodedFrame(
    VC_EncodedFrame *frame_ptr);

vint VCI_getEncodedFrame(
    VC_EncodedFrame *frame_ptr);

#endif

