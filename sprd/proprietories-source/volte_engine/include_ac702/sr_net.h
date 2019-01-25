/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 1381 $ $Date: 2006-12-05 14:44:53 -0800 (Tue, 05 Dec 2006) $
 */

#ifndef _SR_NET_H_
#define _SR_NET_H_

/*
 * These are the interfaces that both VPR and SR need to implement
 * VPR needs to implement these for MSAPP (SAPP on the Modem processor)
 * SR needs to implement these for ASAPP (SAPP on the application processor)
 */

OSAL_Status SR_netSocket(
    OSAL_NetSockId   *socket_ptr,
    OSAL_NetSockType  type);

OSAL_Status SR_netCloseSocket(
    OSAL_NetSockId *socket_ptr);

OSAL_Status SR_netIsSocketIdValid(
    OSAL_NetSockId *socket_ptr);

OSAL_Status SR_netConnectSocket(
    OSAL_NetSockId  *socket_ptr,
    OSAL_NetAddress *address_ptr);

OSAL_Status SR_netBindSocket(
    OSAL_NetSockId  *socket_ptr,
    OSAL_NetAddress *address_ptr);

OSAL_Status SR_netGetSocketAddress(
    OSAL_NetSockId  *socket_ptr,
    OSAL_NetAddress *address_ptr);

OSAL_Status SR_netListenOnSocket(
    OSAL_NetSockId *socket_ptr);

OSAL_Status SR_netAcceptOnSocket(
    OSAL_NetSockId  *socket_ptr,
    OSAL_NetSockId  *newSocket_ptr,
    OSAL_NetAddress *address_ptr);

OSAL_Status SR_netSocketReceive(
    OSAL_NetSockId      *socket_ptr,
    void                *buf_ptr,
    vint                *size_ptr,
    OSAL_NetReceiveFlags flags);

OSAL_Status SR_netSocketReceiveFrom(
    OSAL_NetSockId  *socket_ptr,
    void            *buf_ptr,
    vint            *size_ptr,
    OSAL_NetAddress *address_ptr);

OSAL_Status SR_netSocketSend(
    OSAL_NetSockId  *socket_ptr,
    void            *buf_ptr,
    vint            *size_ptr);

OSAL_Status SR_netSocketSendTo(
    OSAL_NetSockId  *socket_ptr,
    void            *buf_ptr,
    vint            *size_ptr,
    OSAL_NetAddress *address_ptr);

OSAL_Status SR_netSetSocketOptions(
    OSAL_NetSockId  *socket_ptr,
    OSAL_NetSockopt  option,
    int              value);

OSAL_Status SR_netGetSocketOptions(
    OSAL_NetSockId  *socket_ptr,
    OSAL_NetSockopt  option,
    OSAL_Boolean    *value_ptr);

OSAL_Status SR_netStringToAddress(
    int8            *ddn_ptr,
    OSAL_NetAddress *address_ptr);

OSAL_Status SR_netAddressToString(
    int8            *ddn_ptr,
    OSAL_NetAddress *address_ptr);

OSAL_Status SR_netSslInit(
    void);

OSAL_Status SR_netSslConnect(
    OSAL_NetSslId  *ssl_ptr,
    OSAL_NetSockId *socket_ptr);

OSAL_Status SR_netSslAccept(
    OSAL_NetSslId *ssl_ptr,
    OSAL_NetSockId *socket_ptr);

OSAL_Status SR_netSsl(
    OSAL_NetSslId     *ssl_ptr,
    OSAL_NetSslMethod  method);

OSAL_Status SR_netSslClose(
    OSAL_NetSslId  *ssl_ptr,
    OSAL_NetSockId *socket_ptr);

OSAL_Status SR_netSslSend(
    OSAL_NetSslId  *ssl_ptr,
    void           *buf_ptr,
    vint           *size_ptr,
    OSAL_NetSockId *socket_ptr);

OSAL_Status SR_netSslReceive(
    OSAL_NetSslId  *ssl_ptr,
    void           *buf_ptr,
    vint           *size_ptr,
    OSAL_NetSockId *socket_ptr);

OSAL_Status SR_netSslSetSocket(
    OSAL_NetSockId *socket_ptr,
    OSAL_NetSslId  *ssl_ptr);

OSAL_Status SR_netSslGetFingerprint(
    OSAL_NetSslCert *cert_ptr,
    OSAL_NetSslHash  hash,
    unsigned char   *fingerprint,
    vint             fpMaxLen,
    vint            *fpLen_ptr);

OSAL_Status SR_netSslSetCertVerifyCB(
    OSAL_NetSslId  *ssl_ptr,
    void           *certValidateCB);

OSAL_Status SR_netSslSetCert(
    OSAL_NetSslId   *ssl_ptr,
    OSAL_NetSslCert *cert_ptr);

OSAL_Status SR_netSslCertDestroy(
    OSAL_NetSslCert  *cert_ptr);

OSAL_Status SR_netSslCertGen(
    OSAL_NetSslCert *cert_ptr,
    vint             bits,
    vint             serial,
    vint             days);

OSAL_Status SR_netResolve(
    int8                *query_ptr,
    int8                *ans_ptr,
    vint                 ansSize,
    vint                 timeoutsec,
    vint                 retries,
    OSAL_NetResolveType  qtype);

#endif
