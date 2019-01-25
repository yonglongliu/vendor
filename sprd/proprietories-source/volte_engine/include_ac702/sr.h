/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 1381 $ $Date: 2006-12-05 14:44:53 -0800 (Tue, 05 Dec 2006) $
 */

#ifndef _SR_H_
#define _SR_H_

#ifndef INCLUDE_SR
#define SR_start() (OSAL_SUCCESS)
#define SR_allocate() (OSAL_SUCCESS)
#define SR_init() (OSAL_SUCCESS)
#define SR_shutdown() 
/* OSAL_FAIL for not redirected if it's not 4G+ */
#define SR_processReceivedSipPacket(a, b, c, d) (OSAL_FAIL)
#define SR_addCallId(a)
#define SR_removeCallId(a)
#define SR_setNetworkMode(a) (OSAL_SUCCESS)
#define SR_clientConnect(a, b, c) (SIP_OK)
#define SR_setRegSecData(a, b, c, d, e, f, g)
#define SR_getRegSecData(a, b, c, d, e, f, g, h, i, j) 
#else

OSAL_Status SR_start(
    void);

OSAL_Status SR_allocate(
    void);

OSAL_Status SR_init(
    void);

OSAL_Status SR_shutdown(
    void);

OSAL_Status SR_processReceivedSipPacket(
    OSAL_NetSockId sockId,
    tSipIntMsg    *pMsg,
    char          *pB,
    int            len);

OSAL_Status SR_addCallId(
    char *dialogId);

OSAL_Status SR_removeCallId(
    char *dialogId);

OSAL_Status SR_setNetworkMode(
    char *mode);

vint SR_clientConnect(
    tSipHandle      hUa,
    tLocalIpConn   *pLclConn,
    tTransportType  type);

OSAL_Status SR_setRegSecData(
    char           *preconfiguredRoute,
    char           *secSrvHfs,
    int             secSrvHfNum,
    int             secSrvStrlen,
    OSAL_IpsecSa   *inboundSAc,
    OSAL_IpsecSa   *inboundSAs,
    OSAL_IpsecSa   *outboundSAc);

OSAL_Status SR_getRegSecData(
    char           *preconfiguredRoute,
    int             routeLen,
    char           *secSrvHfs,
    int             secSrvHfNum,
    int             secSrvStrlen,
    uint16         *portUs,
    uint16         *portUc,
    OSAL_IpsecSa   *inboundSAc,
    OSAL_IpsecSa   *inboundSAs,
    OSAL_IpsecSa   *outboundSAc);

#endif
#endif
