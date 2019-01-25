/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 1381 $ $Date: 2006-12-05 14:44:53 -0800 (Tue, 05 Dec 2006) $
 */

#ifndef _VIER_NET_H_
#define _VIER_NET_H_

OSAL_Status VIER_netSocket(
    OSAL_NetSockId   *socket_ptr,
    OSAL_NetSockType  type);

OSAL_Status VIER_netCloseSocket(
    OSAL_NetSockId *socket_ptr);

OSAL_Status VIER_netBindSocket(
    OSAL_NetSockId  *socket_ptr,
    OSAL_NetAddress *address_ptr);

OSAL_Status VIER_netSocketReceiveFrom(
    OSAL_NetSockId  *socket_ptr,
    void            *buf_ptr,
    vint            *size_ptr,
    OSAL_NetAddress *address_ptr);

OSAL_Status VIER_netSocketSendTo(
    OSAL_NetSockId  *socket_ptr,
    void            *buf_ptr,
    vint            *size_ptr,
    OSAL_NetAddress *address_ptr);

OSAL_Status VIER_netSetSocketOptions(
    OSAL_NetSockId  *socket_ptr,
    OSAL_NetSockopt  option,
    int              value);

#endif
