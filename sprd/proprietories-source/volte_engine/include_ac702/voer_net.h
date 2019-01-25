/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 1381 $ $Date: 2006-12-05 14:44:53 -0800 (Tue, 05 Dec 2006) $
 */

#ifndef _VOER_NET_H_
#define _VOER_NET_H_

OSAL_Status VPR_netVoiceSocket(
    OSAL_NetSockId   *socket_ptr,
    OSAL_NetSockType  type);

OSAL_Status VPR_netIsSocketIdValid(
    OSAL_NetSockId *socket_ptr);

OSAL_Status VPR_netVoiceBindSocket(
    OSAL_NetSockId  *socket_ptr,
    OSAL_NetAddress *address_ptr);

OSAL_Status VPR_netVoiceSetSocketOptions(
    OSAL_NetSockId  *socket_ptr,
    OSAL_NetSockopt  option,
    int              value);

OSAL_Status VPR_netVoiceSocketSendTo(
    OSAL_NetSockId  *socket_ptr,
    void            *buf_ptr,
    vint            *size_ptr,
    OSAL_NetAddress *address_ptr);

OSAL_Status VPR_netVoiceSocketReceiveFrom(
    OSAL_NetSockId  *socket_ptr,
    void            *buf_ptr,
    vint            *size_ptr,
    OSAL_NetAddress *address_ptr);

OSAL_Status VPR_netVoiceConnectSocket(
    OSAL_NetSockId  *socket_ptr,
    OSAL_NetAddress *address_ptr);
    
OSAL_Status VPR_netVoiceCloseSocket(
    OSAL_NetSockId *socket_ptr);


#endif
