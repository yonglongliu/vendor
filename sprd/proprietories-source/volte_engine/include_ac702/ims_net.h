/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 1381 $ $Date: 2006-12-05 14:44:53 -0800 (Tue, 05 Dec 2006) $
 */

#ifndef __IMS_NET_H__
#define __IMS_NET_H__

#ifndef INCLUDE_SR
#include <osal_net.h>

#define IMS_NET_SOCKET(a, b)                    OSAL_netSocket(a, b)
#define IMS_NET_CLOSE_SOCKET(a)                 OSAL_netCloseSocket(a)

#define IMS_NET_SOCKET_SEND(a, b, c)            OSAL_netSocketSend(a, b, c)
#define IMS_NET_SOCKET_SEND_TO(a, b, c, d)      OSAL_netSocketSendTo(a, b, c, d)

#define IMS_NET_SOCKET_RECEIVE(a, b, c, d) \
        OSAL_netSocketReceive(a, b, c, d)
#define IMS_NET_SOCKET_RECEIVE_FROM(a, b, c, d) \
        OSAL_netSocketReceiveFrom(a, b, c, d)

#define IMS_NET_BIND_SOCKET(a, b)               OSAL_netBindSocket(a, b)
#define IMS_NET_CONNECT_SOCKET(a, b)            OSAL_netConnectSocket(a, b)
#define IMS_NET_ACCEPT_ON_SOCKET(a, b, c)       OSAL_netAcceptOnSocket(a, b, c)
#define IMS_NET_LISTEN_ON_SOCKET(a)             OSAL_netListenOnSocket(a)

#define IMS_NET_GET_SOCKET_ADDRESS(a, b)        OSAL_netGetSocketAddress(a, b)
#define IMS_NET_SET_SOCKET_OPTIONS(a, b, c) \
        OSAL_netSetSocketOptions(a, b, c)

#define IMS_NET_SSL_INIT()                      OSAL_netSslInit()
#define IMS_NET_SSL(a, b)                       OSAL_netSsl(a, b)
#define IMS_NET_SSL_SET_SOCKET(a, b)            OSAL_netSslSetSocket(a, b)
#define IMS_NET_SSL_CLOSE(a, b)                 OSAL_netSslClose(a)
#define IMS_NET_SSL_CONNECT(a, b)               OSAL_netSslConnect(a)
#define IMS_NET_SSL_RECEIVE(a, b, c, d)         OSAL_netSslReceive(a, b, c)
#define IMS_NET_SSL_SEND(a, b, c, d)            OSAL_netSslSend(a, b, c)
#define IMS_NET_SSL_ACCEPT(a, b)                OSAL_netSslAccept(a)
#define IMS_NET_SSL_CERT_GEN(a, b, c, d)        OSAL_netSslCertGen(a, b, c, d)
#define IMS_NET_SSL_SET_CERT(a, b)              OSAL_netSslSetCert(a, b)
#define IMS_NET_SSL_CERT_DESTROY(a)             OSAL_netSslCertDestroy(a);
#define IMS_NET_SSL_GET_FINGERPRINT(a, b, c, d, e) \
        OSAL_netSslGetFingerprint(a, b, c, d, e)

#define IMS_NET_RESOLVE(a, b, c, d, e, f)       OSAL_netResolve(a, b, c, d, e, f)

#else
#include <osal_net.h>
#include <sr_net.h>

#define IMS_NET_SOCKET(a, b)                    SR_netSocket(a, b)
#define IMS_NET_CLOSE_SOCKET(a)                 SR_netCloseSocket(a)

#define IMS_NET_SOCKET_SEND(a, b, c)            SR_netSocketSend(a, b, c)
#define IMS_NET_SOCKET_SEND_TO(a, b, c, d)      SR_netSocketSendTo(a, b, c, d)

#define IMS_NET_SOCKET_RECEIVE(a, b, c, d)      SR_netSocketReceive(a, b, c, d)
#define IMS_NET_SOCKET_RECEIVE_FROM(a, b, c, d) \
        SR_netSocketReceiveFrom(a, b, c, d)

#define IMS_NET_BIND_SOCKET(a, b)               SR_netBindSocket(a, b)
#define IMS_NET_CONNECT_SOCKET(a, b)            SR_netConnectSocket(a, b)
#define IMS_NET_ACCEPT_ON_SOCKET(a, b, c)       SR_netAcceptOnSocket(a, b, c)
#define IMS_NET_LISTEN_ON_SOCKET(a)             SR_netListenOnSocket(a)

#define IMS_NET_GET_SOCKET_ADDRESS(a, b)        SR_netGetSocketAddress(a, b)
#define IMS_NET_SET_SOCKET_OPTIONS(a, b, c)     SR_netSetSocketOptions(a, b, c)

#define IMS_NET_SSL_INIT()                      SR_netSslInit()
#define IMS_NET_SSL(a, b)                       SR_netSsl(a, b)
#define IMS_NET_SSL_SET_SOCKET(a, b)            SR_netSslSetSocket(a, b)
#define IMS_NET_SSL_CLOSE(a, b)                 SR_netSslClose(a, b)
#define IMS_NET_SSL_CONNECT(a, b)               SR_netSslConnect(a, b)
#define IMS_NET_SSL_RECEIVE(a, b, c, d)         SR_netSslReceive(a, b, c, d)
#define IMS_NET_SSL_SEND(a, b, c, d)            SR_netSslSend(a, b, c, d)
#define IMS_NET_SSL_ACCEPT(a, b)                SR_netSslAccept(a, b)
#define IMS_NET_SSL_CERT_GEN(a, b, c, d)        SR_netSslCertGen(a, b, c, d)
#define IMS_NET_SSL_SET_CERT(a, b)              SR_netSslSetCert(a, b)
#define IMS_NET_SSL_CERT_DESTROY(a)             SR_netSslCertDestroy(a);
#define IMS_NET_SSL_GET_FINGERPRINT(a, b, c, d, e) \
        SR_netSslGetFingerprint(a, b, c, d, e)

#define IMS_NET_RESOLVE(a, b, c, d, e, f)       SR_netResolve(a, b, c, d, e, f)

#endif

#endif
