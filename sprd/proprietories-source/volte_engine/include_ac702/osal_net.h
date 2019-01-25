/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 1381 $ $Date: 2006-12-05 14:44:53 -0800 (Tue, 05 Dec 2006) $
 */

#ifndef _OSAL_NET_H_
#define _OSAL_NET_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "osal_types.h"
#include "osal_constant.h"
#include "osal_platform.h"

#define OSAL_NET_IPV6_STR_MAX    (46)             /* maximum ipv6 string length */
#define OSAL_NET_IPV4_STR_MAX    (16)             /* maximum ipv4 string length */
#define OSAL_NET_IPV6_WORD_SZ    (8)              /* ipv6 address word size */
#define OSAL_NET_DNS_UDP_MSG_MAX (512)            /* UDP messages of DNS response */
#define OSAL_NET_DNS_ANS_STR_MAX (128)
#define OSAL_NET_DNS_RETRY_MAX   (5)              /* Max retry times for DNS query */

#define OSAL_NET_SSL_FIFO        OSAL_IPC_FOLDER"ssl-fifo"
/*
 * DNS query types.
 */
typedef enum {
    OSAL_NET_RESOLVE_A    = 1,
    OSAL_NET_RESOLVE_SRV  = 2,
    OSAL_NET_RESOLVE_AAAA = 3,
    OSAL_NET_RESOLVE_NAPTR = 4,
} OSAL_NetResolveType;

/*
 * Socket types.
 */
typedef enum {
    OSAL_NET_SOCK_UDP    = 0,  /* Datagrams for IPV4*/
    OSAL_NET_SOCK_TCP    = 1,  /* Streams for IPV4 */
    OSAL_NET_SOCK_UDP_V6 = 2,  /* Datagrams for IPv6 */
    OSAL_NET_SOCK_TCP_V6 = 3,  /* Streams for IPv6 */
    OSAL_NET_SOCK_IPSEC        /* for IPSec */
} OSAL_NetSockType;

/*
 * Allowed SSL methods
 */
typedef enum {
    OSAL_NET_SSL_METHOD_CLIENT_SSLV3 = 1,
    OSAL_NET_SSL_METHOD_CLIENT_TLSV1 = 2,
    OSAL_NET_SSL_METHOD_CLIENT_ALL   = 3,
    OSAL_NET_SSL_METHOD_SERVER_SSLV3 = 4,
    OSAL_NET_SSL_METHOD_SERVER_TLSV1 = 5,
    OSAL_NET_SSL_METHOD_SERVER_ALL   = 6
} OSAL_NetSslMethod;

typedef enum {
    OSAL_NET_SOCK_NONBLOCKING   = 1, /* Non blocking I/O */
    OSAL_NET_SOCK_REUSE         = 2, /* Reuse ports */
    OSAL_NET_IP_TOS             = 3, /* Enable TOS */
    OSAL_NET_SOCK_RCVTIMEO_SECS = 4, /* Socket receive timeout. Value must be in seconds. */
    OSAL_NET_SOCK_TYPE          = 5, /* Gets type of socket. */
} OSAL_NetSockopt;

typedef enum {
    OSAL_NET_SSL_HASH_SHA1 = 1,
    OSAL_NET_SSL_HASH_MD5  = 2
} OSAL_NetSslHash;
/*
 * Address structure
 */
typedef struct {
    uint32           ipv4;
    uint16           ipv6[OSAL_NET_IPV6_WORD_SZ];
    uint16           port;
    OSAL_NetSockType type;
} OSAL_NetAddress;

typedef enum {
    OSAL_NET_RECV_NO_FLAGS = 0,
    OSAL_NET_RECV_PEEK = 1, /* This flag causes the receive operation to return
                             * data from the beginning of the receive queue
                             * without removing that data from the queue. Thus,
                             * a subsequent receive call will return the same
                             * data.
                             */
}OSAL_NetReceiveFlags;

/*
 * Function prototypes.
 */

OSAL_Status OSAL_netSocket(
    OSAL_NetSockId   *socket_ptr,
    OSAL_NetSockType  type);

OSAL_Status OSAL_netCloseSocket(
    OSAL_NetSockId *socket_ptr);

OSAL_Status OSAL_netIsSocketIdValid(
    OSAL_NetSockId *socket_ptr);

OSAL_Status OSAL_netConnectSocket(
    OSAL_NetSockId  *socket_ptr,
    OSAL_NetAddress *address_ptr);

OSAL_Status OSAL_netBindSocket(
    OSAL_NetSockId  *socket_ptr,
    OSAL_NetAddress *address_ptr);

OSAL_Status OSAL_netGetSocketAddress(
    OSAL_NetSockId  *socket_ptr,
    OSAL_NetAddress *address_ptr);

OSAL_Status OSAL_netListenOnSocket(
    OSAL_NetSockId *socket_ptr);

OSAL_Status OSAL_netAcceptOnSocket(
    OSAL_NetSockId  *socket_ptr,
    OSAL_NetSockId  *newSocket_ptr,
    OSAL_NetAddress *address_ptr);

OSAL_Status OSAL_netSocketReceive(
    OSAL_NetSockId      *socket_ptr,
    void                *buf_ptr,
    vint                *size_ptr,
    OSAL_NetReceiveFlags flags);

OSAL_Status OSAL_netSocketReceiveFrom(
    OSAL_NetSockId  *socket_ptr,
    void            *buf_ptr,
    vint            *size_ptr,
    OSAL_NetAddress *address_ptr);

OSAL_Status OSAL_netSocketSend(
    OSAL_NetSockId  *socket_ptr,
    void            *buf_ptr,
    vint            *size_ptr);

OSAL_Status OSAL_netSocketSendTo(
    OSAL_NetSockId  *socket_ptr,
    void            *buf_ptr,
    vint            *size_ptr,
    OSAL_NetAddress *address_ptr);

OSAL_Status OSAL_netSetSocketOptions(
    OSAL_NetSockId  *socket_ptr,
    OSAL_NetSockopt  option,
    int              value);

OSAL_Status OSAL_netGetSocketOptions(
    OSAL_NetSockId  *socket_ptr,
    OSAL_NetSockopt  option,
    int             *value_ptr);

OSAL_Status OSAL_netStringToAddress(
    int8            *ddn_ptr,
    OSAL_NetAddress *address_ptr);

OSAL_Status OSAL_netAddressToString(
    int8            *ddn_ptr,
    OSAL_NetAddress *address_ptr);

OSAL_Status OSAL_netSslInit(
    void);

OSAL_Status OSAL_netSslConnect(
    OSAL_NetSslId *ssl_ptr);

OSAL_Status OSAL_netSslAccept(
    OSAL_NetSslId *ssl_ptr);

OSAL_Status OSAL_netSsl(
    OSAL_NetSslId     *ssl_ptr,
    OSAL_NetSslMethod  method);

OSAL_Status OSAL_netSslClose(
    OSAL_NetSslId *ssl_ptr);

OSAL_Status OSAL_netIsSslIdValid(
    OSAL_NetSslId *ssl_ptr);

OSAL_Status OSAL_netSslSend(
    OSAL_NetSslId *ssl_ptr,
    void          *buf_ptr,
    vint          *size_ptr);

OSAL_Status OSAL_netSslReceive(
    OSAL_NetSslId *ssl_ptr,
    void          *buf_ptr,
    vint          *size_ptr);

OSAL_Status OSAL_netSslSetSocket(
    OSAL_NetSockId *socket_ptr,
    OSAL_NetSslId  *ssl_ptr);

OSAL_Status OSAL_netSslGetFingerprint(
    OSAL_NetSslCert *cert_ptr,
    OSAL_NetSslHash  hash,
    unsigned char   *fingerprint,
    vint             fpMaxLen,
    vint            *fpLen_ptr);

OSAL_Status OSAL_netSslSetCertVerifyCB(
    OSAL_NetSslId  *ssl_ptr,
    void           *certValidateCB);

OSAL_Status OSAL_netSslSetCert(
    OSAL_NetSslId   *ssl_ptr,
    OSAL_NetSslCert *cert_ptr);

OSAL_Status OSAL_netSslCertDestroy(
    OSAL_NetSslCert  *cert_ptr);

OSAL_Status OSAL_netSslCertGen(
    OSAL_NetSslCert *cert_ptr,
    vint             bits,
    vint             serial,
    vint             days);

OSAL_Status OSAL_netResolve(
    int8                *query_ptr,
    int8                *ans_ptr,
    vint                 ansSize,
    vint                 timeoutsec,
    vint                 retries,
    OSAL_NetResolveType  qtype);

OSAL_Status OSAL_netGetHostByName(
    int8            *name_ptr,
    int8            *ans_ptr,
    vint             ansSize,
    OSAL_NetAddress *address_ptr);

uint32 OSAL_netNtohl(
    uint32 val);

uint32 OSAL_netHtonl(
    uint32 val);

uint16 OSAL_netNtohs(
    uint16 val);

uint16 OSAL_netHtons(
    uint16 val);

void OSAL_netIpv6Ntoh(
    uint16 *dst_ptr,
    uint16 *src_ptr);

void OSAL_netIpv6Hton(
    uint16 *dst_ptr,
    uint16 *src_ptr);

void OSAL_netAddrHton(
    OSAL_NetAddress *dst_ptr,
    OSAL_NetAddress *src_ptr);

void OSAL_netAddrNtoh(
    OSAL_NetAddress *dst_ptr,
    OSAL_NetAddress *src_ptr);

void OSAL_netAddrPortHton(
    OSAL_NetAddress *dst_ptr,
    OSAL_NetAddress *src_ptr);

void OSAL_netAddrPortNtoh(
    OSAL_NetAddress *dst_ptr,
    OSAL_NetAddress *src_ptr);

OSAL_Boolean OSAL_netIpv6IsAddrZero(
    uint16 *addr_ptr);

OSAL_Boolean OSAL_netIsAddrZero(
    OSAL_NetAddress *addr_ptr);

OSAL_Boolean OSAL_netIsAddrLoopback(
    OSAL_NetAddress *addr_ptr);

void OSAL_netAddrCpy(
    OSAL_NetAddress *dst_ptr,
    OSAL_NetAddress *src_ptr);

void OSAL_netIpv6AddrCpy(
    uint16  *dst_ptr,
    uint16  *src_ptr);

void OSAL_netAddrPortCpy(
    OSAL_NetAddress *dst_ptr,
    OSAL_NetAddress *src_ptr);

OSAL_Boolean OSAL_netIsAddrEqual(
    OSAL_NetAddress *addrA_ptr,
    OSAL_NetAddress *addrB_ptr);

OSAL_Boolean OSAL_netIsAddrPortEqual(
    OSAL_NetAddress *addrA_ptr,
    OSAL_NetAddress *addrB_ptr);

OSAL_Boolean OSAL_netIsAddrIpv6(
    OSAL_NetAddress *addr_ptr);

void OSAL_netAddrClear(
    OSAL_NetAddress *addr_ptr);

void OSAL_netIpv6AddrAnyInit(
    uint16 *addr_ptr);

void OSAL_netGetMac(
        char*   infcName,
        char*   mac);

#ifdef __cplusplus
}
#endif

#endif
