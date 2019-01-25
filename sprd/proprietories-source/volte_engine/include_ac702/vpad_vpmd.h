/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 1381 $ $Date: 2006-12-05 14:44:53 -0800 (Tue, 05 Dec 2006) $
 */

#ifndef _VPAD_VPMD_H_
#define _VPAD_VPMD_H_

#include <osal.h>

/* Define VPAD wrappers. */
#ifdef INCLUDE_VPAD

#define VPAD_INIT()                      VPAD_init()
#define VPAD_DESTROY()                   VPAD_destroy()
#define VPAD_IS_READY()                  VPAD_isReady()
#define VPAD_WRITE_VIDEO_CMDEVT(a, b)    VPAD_writeVideoCmdEvt(a, b)
#define VPAD_READ_VIDEO_CMDEVT(a, b, c)  VPAD_readVideoCmdEvt(a, b, c)
#define VPAD_WRITE_VIDEO_STREAM(a, b)    VPAD_writeVideoStream(a, b)
#define VPAD_READ_VIDEO_STREAM(a, b, c)  VPAD_readVideoStream(a, b, c)
#define VPAD_WRITE_VOICE_STREAM(a, b)    VPAD_writeVoiceStream(a, b)
#define VPAD_READ_VOICE_STREAM(a, b, c)  VPAD_readVoiceStream(a, b, c)
#define VPAD_WRITE_ISI_RPC(a, b)         VPAD_writeIsiRpc(a, b)
#define VPAD_READ_ISI_RPC(a, b, c)       VPAD_readIsiRpc(a, b, c)
#define VPAD_WRITE_ISI_EVT_RPC(a, b)     VPAD_writeIsiEvtRpc(a, b)
#define VPAD_READ_ISI_EVT_RPC(a, b, c)   VPAD_readIsiEvtRpc(a, b, c)
#define VPAD_WRITE_SIP(a, b)             VPAD_writeSip(a, b)
#define VPAD_READ_SIP(a, b, c)           VPAD_readSip(a, b, c)
#define VPAD_WRITE_ISIP(a, b)            VPAD_writeIsip(a, b)
#define VPAD_READ_ISIP(a, b, c)          VPAD_readIsip(a, b, c)
#define VPAD_WRITE_CSM_EVT(a, b)         VPAD_writeCsmEvt(a, b)
#define VPAD_GET_VOICE_STREAM_READ_FD()  VPAD_getVoiceStreamReadFd()

#else
/* Empty VPMD wrappers. */
#define VPAD_INIT()                      OSAL_SUCCESS
#define VPAD_DESTROY()                   OSAL_SUCCESS
#define VPAD_IS_READY()                  OSAL_TRUE
#define VPAD_WRITE_VIDEO_CMDEVT(a, b)    OSAL_SUCCESS
#define VPAD_READ_VIDEO_CMDEVT(a, b, c)  OSAL_SUCCESS
#define VPAD_WRITE_VIDEO_STREAM(a, b)    OSAL_SUCCESS
#define VPAD_READ_VIDEO_STREAM(a, b, c)  OSAL_SUCCESS
#define VPAD_WRITE_VOICE_STREAM(a, b)    OSAL_SUCCESS
#define VPAD_READ_VOICE_STREAM(a, b, c)  OSAL_SUCCESS
#define VPAD_WRITE_ISI_RPC(a, b)         OSAL_SUCCESS
#define VPAD_READ_ISI_RPC(a, b, c)       OSAL_SUCCESS
#define VPAD_WRITE_ISI_EVT_RPC(a, b)     OSAL_SUCCESS
#define VPAD_READ_ISI_EVT_RPC(a, b, c)   OSAL_SUCCESS
#define VPAD_WRITE_SIP(a, b)             OSAL_SUCCESS
#define VPAD_READ_ISIP(a, b, c)          OSAL_SUCCESS
#define VPAD_WRITE_ISIP(a, b)            OSAL_SUCCESS
#define VPAD_READ_SIP(a, b, c)           OSAL_SUCCESS
#define VPAD_WRITE_CSM_EVT(a, b)         OSAL_SUCCESS
#define VPAD_GET_VOICE_STREAM_READ_FD()  OSAL_SUCCESS

#endif

/* VPAD FIFO */
/*
 * VPAD write data to these FIFO.
 * VPAD_MUX will read it and write data to device.
 */
#define VPAD_VIDEO_CMDEVT_W_FIFO    OSAL_IPC_FOLDER"vpad-videoCmdEvt-w"
#define VPAD_VIDEO_STREAM_W_FIFO    OSAL_IPC_FOLDER"vpad-videoStream-w"
#define VPAD_VOICE_STREAM_W_FIFO    OSAL_IPC_FOLDER"vpad-voiceStream-w"
#define VPAD_ISI_RPC_W_FIFO         OSAL_IPC_FOLDER"vpad-isiRpc-w"
#define VPAD_ISI_EVT_RPC_W_FIFO     OSAL_IPC_FOLDER"vpad-isiEvtRpc-w"
#define VPAD_SIP_W_FIFO             OSAL_IPC_FOLDER"vpad-sip-w"
#define VPAD_ISIP_W_FIFO            OSAL_IPC_FOLDER"vpad-isip-w"
#define VPAD_CSM_EVT_W_FIFO         OSAL_IPC_FOLDER"vpad-csmEvt-w"
/*
 * VPAD read data from these FIFO.
 * VPAD_MUX will read data from device and write data to these FIFO.
 */
#define VPAD_VIDEO_CMDEVT_R_FIFO    OSAL_IPC_FOLDER"vpad-videoCmdEvt-r"
#define VPAD_VIDEO_STREAM_R_FIFO    OSAL_IPC_FOLDER"vpad-videoStream-r"
#define VPAD_VOICE_STREAM_R_FIFO    OSAL_IPC_FOLDER"vpad-voiceStream-r"
#define VPAD_ISI_RPC_R_FIFO         OSAL_IPC_FOLDER"vpad-isiRpc-r"
#define VPAD_ISI_EVT_RPC_R_FIFO     OSAL_IPC_FOLDER"vpad-isiEvtRpc-r"
#define VPAD_SIP_R_FIFO             OSAL_IPC_FOLDER"vpad-sip-r"
#define VPAD_ISIP_R_FIFO            OSAL_IPC_FOLDER"vpad-isip-r"
#define VPAD_CSM_EVT_R_FIFO         OSAL_IPC_FOLDER"vpad-csmEvt-r"

/* VPAD error recovery delay in millisecond. */
#define VPAD_ERROR_RECOVERY_DELAY    (1000)

/* VPMD FIFO */
/*
 * VPMD write data to these FIFO.
 * VPMD_MUX will read it and write data to device.
 */
#define VPMD_VIDEO_CMDEVT_W_FIFO    OSAL_IPC_FOLDER"vpmd-videoCmdEvt-w"
#define VPMD_VIDEO_STREAM_W_FIFO    OSAL_IPC_FOLDER"vpmd-videoStream-w"
#define VPMD_VOICE_STREAM_W_FIFO    OSAL_IPC_FOLDER"vpmd-voiceStream-w"
#define VPMD_ISI_RPC_W_FIFO         OSAL_IPC_FOLDER"vpmd-isiRpc-w"
#define VPMD_ISI_EVT_RPC_W_FIFO     OSAL_IPC_FOLDER"vpmd-isiEvtRpc-w"
#define VPMD_SIP_W_FIFO             OSAL_IPC_FOLDER"vpmd-sip-w"
#define VPMD_ISIP_W_FIFO            OSAL_IPC_FOLDER"vpmd-isip-w"
#define VPMD_CSM_EVT_W_FIFO         OSAL_IPC_FOLDER"vpmd-csmEvt-w"
/*
 * VPMD read data from these FIFO.
 * VPMD_MUX will read data from device and write data to these FIFO.
 */
#define VPMD_VIDEO_CMDEVT_R_FIFO    OSAL_IPC_FOLDER"vpmd-videoCmdEvt-r"
#define VPMD_VIDEO_STREAM_R_FIFO    OSAL_IPC_FOLDER"vpmd-videoStream-r"
#define VPMD_VOICE_STREAM_R_FIFO    OSAL_IPC_FOLDER"vpmd-voiceStream-r"
#define VPMD_ISI_RPC_R_FIFO         OSAL_IPC_FOLDER"vpmd-isiRpc-r"
#define VPMD_ISI_EVT_RPC_R_FIFO     OSAL_IPC_FOLDER"vpmd-isiEvtRpc-r"
#define VPMD_SIP_R_FIFO             OSAL_IPC_FOLDER"vpmd-sip-r"
#define VPMD_ISIP_R_FIFO            OSAL_IPC_FOLDER"vpmd-isip-r"
#define VPMD_CSM_EVT_R_FIFO         OSAL_IPC_FOLDER"vpmd-csmEvt-r"

typedef enum {
    VPMD_VIDEO_CMDEVT_TARGET,
    VPMD_VIDEO_STREAM_TARGET,
    VPMD_VOICE_STREAM_TARGET,
    VPMD_ISI_RPC_TARGET,
    VPMD_ISI_EVT_RPC_TARGET,
    VPMD_SIP_TARGET,
    VPMD_ISIP_TARGET,
    VPMD_CSM_EVT_TARGET,
    VPMD_VPAD_TARGET, /* For informing VPAD that VPMD is ready. */
    VPMD_VPMD_TARGET, /* For informing VPMD that VPAD is ready. */
} VPMD_CommTarget;

typedef struct {
    char    name[128];
    int     fid;
    uint32  sz;
#ifdef VPAD_MUX_STATS
    uint32  pktCount;
    uint32  szCount;
#endif
} MODEM_Terminal; // Make this look like an OSAL Message Queue

OSAL_Status VPMD_allocate(void);

/* Masters are created by VPMD (the simulated device) */
OSAL_Status VPMD_init(
        void);
void VPMD_destroy(
        void);
OSAL_Boolean VPMD_isReady(
        void);


OSAL_Status VPMD_muxWriteDevice(
    void *buf_ptr,
    vint *size_ptr);

/* VPMD interfaces for Video command/events write/read to/from VPAD */
OSAL_Status VPMD_writeVideoCmdEvt(
    void *buf_ptr,
    vint  size);

OSAL_Status VPMD_readVideoCmdEvt(
    void *buf_ptr,
    vint  size,
    vint  timeout);

/* VPMD interfaces for Video streams */
OSAL_Status VPMD_writeVideoStream(
    void *buf_ptr,
    vint  size);

OSAL_Status VPMD_readVideoStream(
    void *buf_ptr,
    vint  size,
    vint  timeout);

/* VPMD interfaces for Voice streams */
OSAL_Status VPMD_writeVoiceStream(
    void *buf_ptr,
    vint  size);

OSAL_Status VPMD_readVoiceStream(
    void *buf_ptr,
    vint  size,
    vint  timeout);

/* VPMD interfaces for ISI RPC */
OSAL_Status VPMD_writeIsiRpc(
    void *buf_ptr,
    vint  size);

OSAL_Status VPMD_readIsiRpc(
    void *buf_ptr,
    vint  size,
    vint  timeout);

/* VPMD interfaces for ISI event RPC */
OSAL_Status VPMD_writeIsiEvtRpc(
    void *buf_ptr,
    vint  size);

OSAL_Status VPMD_readIsiEvtRpc(
    void *buf_ptr,
    vint  size,
    vint  timeout);

/* VPMD interfaces for sip */
OSAL_Status VPMD_writeSip(
    void *buf_ptr,
    vint  size);

OSAL_Status VPMD_readSip(
    void *buf_ptr,
    vint  size,
    vint  timeout);

/* VPMD interfaces for isip */
OSAL_Status VPMD_writeIsip(
    void *buf_ptr,
    vint  size);

OSAL_Status VPMD_readIsip(
    void *buf_ptr,
    vint  size,
    vint  timeout);

/* VPMD interfaces for CSM event */
OSAL_Status VPMD_readCsmEvt(
    void *buf_ptr,
    vint  size,
    vint  timeout);

int VPMD_getVoiceStreamReadFd(
    void);

int VPMD_getVideoCmdEvtReadFd(
    void);

int VPMD_getVideoStreamReadFd(
    void);

int VPMD_getSipReadFd(
    void);

int VPMD_getIsipReadFd(
    void);

int VPMD_getCsmEvtReadFd(
    void);

/* VPAD public function */
/*
 * Added abstraction to distinct the service task from common i/o
 * Only one process in the system could/must initService
 * other process would be simple user of vpad read/write functions.
 * user could query the readiness of the service
 */
OSAL_Status VPAD_init(
        void);
void VPAD_destroy(
        void);
OSAL_Boolean VPAD_isReady(
        void);

/* VPAD interfaces for video commands/events write/read to/from VPMD. */

OSAL_Status VPAD_writeVideoCmdEvt(
    void *buf_ptr,
    vint  size);

OSAL_Status VPAD_readVideoCmdEvt(
    void  *buf_ptr,
    vint   size,
    vint   timeout);

/* VPAD interfaces for video streams. */
OSAL_Status VPAD_readVideoStream(
    void *buf_ptr,
    vint  size,
    vint  timeout);

OSAL_Status VPAD_writeVideoStream(
    void *buf_ptr,
    vint  size);

/* VPAD interfaces for voice streams. */
OSAL_Status VPAD_readVoiceStream(
    void *buf_ptr,
    vint  size,
    vint  timeout);

OSAL_Status VPAD_writeVoiceStream(
    void *buf_ptr,
    vint  size);

/* VPAD interfaces for ISI RPC */
OSAL_Status VPAD_readIsiRpc(
    void *buf_ptr,
    vint  size,
    vint  timeout);

OSAL_Status VPAD_writeIsiRpc(
    void *buf_ptr,
    vint  size);

/* VPAD interfaces for ISI event RPC */
OSAL_Status VPAD_readIsiEvtRpc(
    void *buf_ptr,
    vint  size,
    vint  timeout);

OSAL_Status VPAD_writeIsiEvtRpc(
    void *buf_ptr,
    vint  size);

int VPAD_getVoiceStreamReadFd(
    void);

/* VPAD interfaces for sip */
OSAL_Status VPAD_readSip(
    void *buf_ptr,
    vint  size,
    vint  timeout);

OSAL_Status VPAD_writeSip(
    void *buf_ptr,
    vint  size);

/* VPAD interfaces for isip */
OSAL_Status VPAD_readIsip(
    void *buf_ptr,
    vint  size,
    vint  timeout);

OSAL_Status VPAD_writeIsip(
    void *buf_ptr,
    vint  size);

/* VPAD interface for CSM event */
OSAL_Status VPAD_writeCsmEvt(
    void *buf_ptr,
    vint  size);

#ifdef VP4G_PLUS_MODEM_TEST

int VPAD_getVideoCmdEvtReadFd(
    void);

int VPAD_getVideoStreamReadFd(
    void);

int VPAD_getSipReadFd(
    void);

int VPAD_getIsipReadFd(
    void);

#endif /* VP4G_PLUS_MODEM_TEST */
#endif //_VPAD_VPMD_H_
