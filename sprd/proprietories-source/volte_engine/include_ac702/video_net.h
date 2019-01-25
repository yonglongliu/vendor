/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 1381 $ $Date: 2006-12-05 14:44:53 -0800 (Tue, 05 Dec 2006) $
 */

#ifndef __VIDEO_NET_H__
#define __VIDEO_NET_H__

#ifndef INCLUDE_VPR
#include <osal_net.h>

#define VIDEO_NET_SOCKET(a, b)                    OSAL_netSocket(a, b)
#define VIDEO_NET_CLOSE_SOCKET(a)                 OSAL_netCloseSocket(a)
#define VIDEO_NET_SOCKET_SEND_TO(a, b, c, d)      OSAL_netSocketSendTo(a, b, c, d)
#define VIDEO_NET_SOCKET_RECEIVE_FROM(a, b, c, d) OSAL_netSocketReceiveFrom(a, b, c, d)
#define VIDEO_NET_BIND_SOCKET(a, b)               OSAL_netBindSocket(a, b)
#define VIDEO_NET_SET_SOCKET_OPTIONS(a, b, c)     OSAL_netSetSocketOptions(a, b, c)

#else
#include <osal_net.h>
#include <vier_net.h>

#define VIDEO_NET_SOCKET(a, b)                    VIER_netSocket(a, b)
#define VIDEO_NET_CLOSE_SOCKET(a)                 VIER_netCloseSocket(a)
#define VIDEO_NET_SOCKET_SEND_TO(a, b, c, d)      VIER_netSocketSendTo(a, b, c, d)
#define VIDEO_NET_SOCKET_RECEIVE_FROM(a, b, c, d) VIER_netSocketReceiveFrom(a, b, c, d)
#define VIDEO_NET_BIND_SOCKET(a, b)               VIER_netBindSocket(a, b)
#define VIDEO_NET_SET_SOCKET_OPTIONS(a, b, c)     VIER_netSetSocketOptions(a, b, c)

#endif
#endif
