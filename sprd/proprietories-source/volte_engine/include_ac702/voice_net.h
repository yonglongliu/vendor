/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 1381 $ $Date: 2006-12-05 14:44:53 -0800 (Tue, 05 Dec 2006) $
 */

#ifndef __VOICE_NET_H__
#define __VOICE_NET_H__

#ifndef INCLUDE_VPR
#include <osal_net.h>

#define VOICE_NET_SOCKET(a, b)                    OSAL_netSocket(a, b)
#define VOICE_NET_IS_SOCKETID_VALID(a)            OSAL_netIsSocketIdValid(a)
#define VOICE_NET_CLOSE_SOCKET(a)                 OSAL_netCloseSocket(a)
#define VOICE_NET_SOCKET_SEND_TO(a, b, c, d)      OSAL_netSocketSendTo(a, b, c, d)
#define VOICE_NET_SOCKET_RECEIVE_FROM(a, b, c, d) OSAL_netSocketReceiveFrom(a, b, c, d)
#define VOICE_NET_BIND_SOCKET(a, b)               OSAL_netBindSocket(a, b)
#define VOICE_NET_CONNECT_SOCKET(a, b)            OSAL_netConnectSocket(a, b)
#define VOICE_NET_SET_SOCKET_OPTIONS(a, b, c)     OSAL_netSetSocketOptions(a, b, c)

#else
#include <osal_net.h>
#include <voer_net.h>

#define VOICE_NET_SOCKET(a, b)                    VPR_netVoiceSocket(a, b)
#define VOICE_NET_IS_SOCKETID_VALID(a)            VPR_netIsSocketIdValid(a)
#define VOICE_NET_SOCKET_SEND_TO(a, b, c, d)      VPR_netVoiceSocketSendTo(a, b, c, d)
#define VOICE_NET_SOCKET_RECEIVE_FROM(a, b, c, d) VPR_netVoiceSocketReceiveFrom(a, b, c, d)
#define VOICE_NET_BIND_SOCKET(a, b)               VPR_netVoiceBindSocket(a, b)
#define VOICE_NET_CONNECT_SOCKET(a, b)            VPR_netVoiceConnectSocket(a, b)
#define VOICE_NET_SET_SOCKET_OPTIONS(a, b, c)     VPR_netVoiceSetSocketOptions(a, b, c)
#define VOICE_NET_CLOSE_SOCKET(a)                 VPR_netVoiceCloseSocket(a)

#endif
#endif
