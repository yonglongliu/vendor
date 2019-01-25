/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 1381 $ $Date: 2006-12-05 14:44:53 -0800 (Tue, 05 Dec 2006) $
 */

#ifndef _VIER_H_
#define _VIER_H_

#ifndef INCLUDE_VIER

#define VIDEO_VTSP_CMD_Q_NAME      "vtsp-cmdqVideo"
#define VIDEO_VTSP_EVT_Q_NAME      "vtsp-evtqVideo"
#define VIDEO_VTSP_RTCP_MSG_Q_NAME "vtsp-rtcpmsgqVideo"
#define VIDEO_VTSP_RTCP_EVT_Q_NAME "vtsp-rtcpevtqVideo"

#define VIER_init() (OSAL_SUCCESS)
#define VIER_shutdown() 
#else

#define VIDEO_VTSP_CMD_Q_NAME      "vier-cmdqVideo"
#define VIDEO_VTSP_EVT_Q_NAME      "vier-evtqVideo"
#define VIDEO_VTSP_RTCP_MSG_Q_NAME "vier-rtcpmsgqVideo"
#define VIDEO_VTSP_RTCP_EVT_Q_NAME "vier-rtcpevtqVideo"

OSAL_Status VIER_init(
    void);

OSAL_Status VIER_shutdown(
    void);

OSAL_Status VIER_writeVtspEvt(
    void* data, uint32 length);

OSAL_Status VIER_writeRtcpCmd(
    void* data, uint32 length);

#endif
#endif
