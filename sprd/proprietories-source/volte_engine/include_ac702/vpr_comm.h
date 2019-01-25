/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 1381 $ $Date: 2006-12-05 14:44:53 -0800 (Tue, 05 Dec 2006) $
 */
#ifndef __VPR_COMM_H__
#define __VPR_COMM_H__

#include "../ve/vtsp/vtsp_private/_vtsp_private.h"
#include "../csm/csm_event.h"
#include "isi.h"
#include "isip.h"

#define VPR_NET_MAX_DATA_SIZE_OCTETS (2500)
#define VPR_MAX_VIDEO_STREAMS        (2)
#define VPR_MAX_VOICE_STREAMS        (5)
#define VPR_STRING_SZ                (256)

/*
 * Maximum sr sockets, it's the sum of sip(six, two unprotect plus four
 * protected)
 */
#define VPR_MAX_SR_SOCKETS           (6)

/* Maximum size of packets buffer for caching packet from SR */
#define VPR_PACKET_BUFFER_SIZE_MAX   (8192)

/*
 * Max number of Security-Server header fields that can be contained 
 * in re-register requests. This is same as SAPP_SEC_SRV_HF_MAX_NUM
 */
#define VPR_SEC_SRV_HF_MAX_NUM       (3)

typedef enum {
    VPR_TYPE_VTSP_CMD, /* Indicates '_VTSP_CmdMsg' object is relevant */
    VPR_TYPE_VTSP_EVT, /* Indicates 'VTSP_EventMsg' object is relevant */
    VPR_TYPE_RTCP_CMD, /* Indicates '_VTSP_RtcpCmdMsg' object is relevant */
    VPR_TYPE_RTCP_EVT, /* Indicates '_VTSP_RtcpEventMsg' object is relevant */
    VPR_TYPE_ISIP,     /* Indicates 'ISIP_Message' object is relevant */
    VPR_TYPE_NET,      /* Indicates 'VPR_Net' object is relevant */
    VPR_TYPE_CSM_EVT,  /* Indicates 'CSM_InputEvent' object is relevant */
    VPR_TYPE_NETWORK_MODE, /* Indicates network mode change */
} VPR_CommType;

typedef enum {
    VPR_NET_TYPE_RTP_SEND_PKT, /* VPR Net command to send RTP data */
    VPR_NET_TYPE_RTP_RECV_PKT, /* VPR Net event for recv’d RTP data */
    VPR_NET_TYPE_SIP_SEND_PKT, /* VPR Net command to send SIP data */
    VPR_NET_TYPE_SIP_RECV_PKT, /* VPR Net event for recv’d SIP data */
    VPR_NET_TYPE_RTCP_MSG,     /* VPR Net command to send video RTCP data */
    VPR_NET_TYPE_RTCP_EVT,     /* VPR Net event for recv’d video RTCP data */
    VPR_NET_TYPE_CREATE_VOICE_RTP, /*
                                    * VPR Net command to create video
                                    * rtp socket.
                                    */
    VPR_NET_TYPE_CREATE_VIDEO_RTP, /*
                                    * VPR Net command to create video
                                    * rtp socket.
                                    */
    VPR_NET_TYPE_CREATE_SIP,   /* VPR Net command to create sip socket */
    VPR_NET_TYPE_CLOSE,        /* VPR Net command to close a net interface */
    VPR_NET_TYPE_ERROR,        /* VPR Net event for errors with sockets */
    VPR_NET_TYPE_REG_SEC_CFG,  /* VPR Net IPSec/Reg configuration */
} VPR_NetType;

typedef enum {
    VPR_NET_STATUS_READ_ERROR,
    VPR_NET_STATUS_WRITE_ERROR,
    VPR_NET_STATUS_OPEN_ERROR,
    VPR_NET_STATUS_CLOSE_ERROR,
} VPR_NetStatusType;

typedef enum {
    VPR_NETWORK_MODE_NONE,
    VPR_NETWORK_MODE_WIFI,
    VPR_NETWORK_MODE_LTE,
} VPR_NetworkMode;

typedef struct {
    VPR_NetType     type;          /* The type of VPR Net command/event */
    OSAL_NetSockId  referenceId;   /* A reference ID so commands and
                                    * status can be mapped to certain socket
                                    * ID's
                                    */

    OSAL_NetAddress localAddress;  /* The local IP address & port associated
                                    * with the command or status event
                                    */
    OSAL_NetAddress remoteAddress; /* The remote IP address & port associated
                                    * with the command or status event
                                    */
    union {
        struct {
            vint            tosValue;      /* TOS value when sending RTP */

            /* Larger packets will be sent in chunks.
             * The chunkNumber and packetEnd are used to indicate when a
             * complete packet is copied. packetEnd represents is this packet 
             * is the end of packet(s).
             */
             vint            chunkNumber;
             vint            packetEnd;

             uint32          packetLen; /* The length of the packet (chunk) */
             /* The packet (or chunk) data */
             uint8           packetData[VPR_NET_MAX_DATA_SIZE_OCTETS];
        } packet;
        struct {
            VPR_NetStatusType evtType; /* The type of status */
            vint              errnum;  /* If the type is an error this is the
                                        * errno to set
                                        */
        } status;
        struct {
            char            preconfiguredRoute[VPR_STRING_SZ];
            char            secSrvHfs[VPR_SEC_SRV_HF_MAX_NUM][VPR_STRING_SZ];
            OSAL_IpsecSa    inboundSAc;
            OSAL_IpsecSa    inboundSAs;
            OSAL_IpsecSa    outboundSAc;
        } regsecData;
     } u;
} VPR_Net;

typedef struct {
    /* Data union must at beginning, don't move it */
    union {
        _VTSP_CmdMsg       vtspCmd;
        _VTSP_RtcpCmdMsg   vtspRtcpCmd;
        _VTSP_RtcpEventMsg vtspRtcpEvt;
        VTSP_EventMsg      vtspEvt;
        ISIP_Message       isipMsg;
        VPR_Net            vprNet;
        CSM_InputEvent     csmEvt;
        VPR_NetworkMode    networkMode;
    }u;
    VPR_CommType type; /* The type of VPR data. It represents which
                        * of the union-ed objects is relevant
                        */
    vint         targetModule; /* only for vpad/vpmd mux/demux */
    vint         targetSize;
} VPR_Comm;

#endif
