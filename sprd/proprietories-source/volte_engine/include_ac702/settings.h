/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30336 $ $Date: 2014-12-11 10:24:15 +0800 (Thu, 11 Dec 2014) $
 */

#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#include <ezxml.h>

/* SETTINGS Tag string definition */
#define SETTINGS_TAG_PROTOCOL               "protocol"
#define SETTINGS_TAG_SIP                    "sip"
#define SETTINGS_TAG_SIMPLE                 "simple"
#define SETTINGS_TAG_HANDOFF                "handoff"
#define SETTINGS_TAG_INTERFACE              "interface"
#define SETTINGS_TAG_AUDIO                  "audio"
#define SETTINGS_TAG_PARM                   "parm"
#define SETTINGS_TAG_REG_CAPABILITIES       "reg-capabilities"
#define SETTINGS_TAG_EX_CAPABILITIES        "exchange-capabilities"
#define SETTINGS_TAG_SRVCC_CAPABILITIES     "srvcc-capabilities"
#define SETTINGS_TAG_XCAP                   "xcap"
#define SETTINGS_TAG_SERVICE                "service"
#define SETTINGS_TAG_SMS                    "sms"
#define SETTINGS_TAG_SUPPLEMENTARY_SRV      "SupplementarySrv"
#define SETTINGS_TAG_TRANSPORT_PROTO        "transportProto"
#define SETTINGS_TAG_GSM                    "gsm"
#define SETTINGS_TAG_PROXY                  "proxy"
#define SETTINGS_TAG_PORT                   "port"
#define SETTINGS_TAG_EMERGENCY              "emergency"
#define SETTINGS_TAG_PROFILE                "profile"

/*
 * SETTINGS PARM string definition
 */

/* SIP parameters */
#define SETTINGS_PARM_THIS                  "this"
#define SETTINGS_PARM_AUDIO                 "audio"
#define SETTINGS_PARM_STREAM                "stream"
#define SETTINGS_PARM_ISI                   "isi"
#define SETTINGS_PARM_RING_TEMPLATE         "Ring_Template"
#define SETTINGS_PARM_REG_EXPIRE_SEC        "Reg_Expire_Sec"
#define SETTINGS_PARM_NAT_KEEP_ALIVE_SEC    "Nat_Keep_Alive_Sec"
#define SETTINGS_PARM_IPSEC_ENABLED         "Ipsec_Enabled"
#define SETTINGS_PARM_PRECONDITION_ENABLED  "Precondition_Enabled"
#define SETTINGS_PARM_MWI_EXPIRE_SEC        "Mwi_Expire_Sec"
#define SETTINGS_PARM_PRACK_ENABLED         "Prack_Enabled"
#define SETTINGS_PARM_CPIM_ENABLED          "Cpim_Enabled"
#define SETTINGS_PARM_ISIM_ENABLED          "Isim_Enabled"
#define SETTINGS_PARM_SESSION_TIMER         "Session_Timer"
#define SETTINGS_PARM_FORCE_MT_SESSION_TIMER \
                                            "Force_MT_Session_Timer"
#define SETTINGS_PARM_REG_EVENT_ENABLED     "Reg_Event_Enabled"
#define SETTINGS_PARM_MWI_EVENT_ENABLED     "Mwi_Event_Enabled"
#define SETTINGS_PARM_UA_NAME               "Ua_Name"

#define SETTINGS_PARM_TIMER_T1              "Timer_T1"
#define SETTINGS_PARM_TIMER_T2              "Timer_T2"
#define SETTINGS_PARM_TIMER_T4              "Timer_T4"
#define SETTINGS_PARM_TIMER_TCALL           "Timer_Tcall"
#define SETTINGS_PARM_REG_RETRY_BASE_TIME   "RegRetryBaseTime"
#define SETTINGS_PARM_REG_RETRY_MAX_TIME    "RegRetryMaxTime"
#define SETTINGS_PARM_Q_VALUE               "Q-Value"
#define SETTINGS_PARM_KEEP_ALIVE_ENABLED    "Keep_Alive_Enabled"
#define SETTINGS_PARM_PHONE_CONTEXT         "PhoneContext"
#define SETTINGS_PARM_MTU                   "Mtu"

/* Capabilities parameters */
#define SETTINGS_PARM_IP_VOICE_CALL         "Ip_Voice_Call"
#define SETTINGS_PARM_IP_VIDEO_CALL         "Ip_Video_Call"
#define SETTINGS_PARM_CHAT                  "Chat"
#define SETTINGS_PARM_IMAGE_SHARE           "Image_Share"
#define SETTINGS_PARM_VIDEO_SHARE           "Video_Share"
#define SETTINGS_PARM_FILE_TRANSFER         "File_Transfer"
#define SETTINGS_PARM_IMAGE_SHARE           "Image_Share"
#define SETTINGS_PARM_SMS_OVER_IP           "Sms_Over_Ip"
#define SETTINGS_PARM_DISCOVERY_VIA_PRESENCE \
                                            "Discover_Via_Presence"
#define SETTINGS_PARM_MESSAGING             "Messaging"
#define SETTINGS_PARM_VIDEO_SHARE_WITHOUT_CALL \
                                            "Video_Share_Without_Call"
#define SETTINGS_PARM_SOCIAL_PRESENCE       "Social_Presence"
#define SETTINGS_PARM_GEOLOCATION_PUSH      "Geolocation_Push"
#define SETTINGS_PARM_GEOLOCATION_PULL      "Geolocation_Pull"
#define SETTINGS_PARM_FILE_TRANSFER_HTTP    "File_Transfer_Http"
#define SETTINGS_PARM_FILE_TRANSFER_THUMBNAIL "File_Transfer_Thumbnail"
#define SETTINGS_PARM_FILE_TRANSFER_STORE_FWD "File_Transfer_Store_Forward"
/* RCS Telephony caps, it's used for registration. */
#define SETTINGS_PARM_RCS_TELEPHONY_CS      "Rcs_Telephony_Cs"
#define SETTINGS_PARM_RCS_TELEPHONY_VOLTE   "Rcs_Telephony_Volte"
#define SETTINGS_PARM_RCS_TELEPHONY_VOHSPA  "Rcs_Telephony_Vohspa"
#define SETTINGS_PARM_SRVCC_ALERTING        "Alerting"
#define SETTINGS_PARM_SRVCC_MID_CALL        "Mid_Call"

/* SIMPLE parameters */
#define SETTINGS_PARM_PRESENCE_EXPIRE_SEC   "Presence_Expire_Sec"
#define SETTINGS_PARM_FILE_PATH             "File_Path"
#define SETTINGS_PARM_FILE_PREPEND          "File_Prepend"
#define SETTINGS_PARM_FILE_FLOW_CTRL_DEPTH  "File_Flow_Ctrl_Depth"

/* Handoff parameters */
#define SETTINGS_PARM_VDN                   "Vdn"
#define SETTINGS_PARM_VDI                   "Vdi"

/* GSM parameters */
#define SETTINGS_PARM_TERMINAL              "Terminal"
#define SETTINGS_PARM_FILE                  "File"
#define SETTINGS_PARM_EXT_DIAL_CMD_ENABLED  "Ext_Dial_Cmd_Enabled"
#define SETTINGS_PARM_EMERGENCY_NUMBER      "Number"

/* XCAP parameters */
#define SETTINGS_PARM_BLACK_LIST            "Black_List"
#define SETTINGS_PARM_WHITE_LIST            "White_List"
#define SETTINGS_PARM_TIMEOUT               "Timeout"

/* Parm used for CSM */
#define SETTINGS_PARM_RCS_PROVISIONING_ENABLED \
                                            "Rcs_Provisioning_Enabled"
#define SETTINGS_PARM_IM_CONF_SERVER        "conf-fcty-uri"
#define SETTINGS_PARM_NAT_URL_FMT           "NatUrlFmt"
#define SETTINGS_PARM_INT_URL_FMR           "IntUrlFmt"
#define SETTINGS_PARM_SMS_PDU_FMT           "PduFmt"
#define SETTINGS_PARM_SMS_TYPE              "SmsType"
#define SETTINGS_PARM_PS_RT_MEDIA           "psRTMedia"
#define SETTINGS_PARM_WIFI_RT_MEDIA         "wifiRTMedia"
#define SETTINGS_PARM_SERVER                "Server"
#define SETTINGS_PARM_USER                  "User"
#define SETTINGS_PARM_URI                   "Uri"
#define SETTINGS_PARM_PASSWORD              "Password"

#define SETTINGS_PARM_PS_SIGNALLING         "psSignalling"
#define SETTINGS_PARM_WIFI_SIGNALLING       "wifiSignalling"
#define SETTINGS_PARM_PS_MEDIA              "psMedia"
#define SETTINGS_PARM_WIFI_MEDIA            "wifiMedia"

/* ISIM parameters */
#define SETTINGS_PARM_DOMAIN                "Domain"
#define SETTINGS_PARM_PCSCF                 "Pcscf"
#define SETTINGS_PARM_IMPU                  "Impu"
#define SETTINGS_PARM_IMPI                  "Impi"
#define SETTINGS_PARM_SMSC                  "Smsc"
#define SETTINGS_PARM_IMEI_URI              "Imei_Uri"
#define SETTINGS_PARM_INSTANCE_ID           "Instance_Id"
#define SETTINGS_PARM_SIP_PORT              "Sip_Port"
#define SETTINGS_PARM_AUDIO_PORT            "Audio_Port"
#define SETTINGS_PARM_AUDIO_PORT_POOL_SIZE  "Audio_Port_Pool_Size"
#define SETTINGS_PARM_VIDEO_PORT            "Video_Port"
#define SETTINGS_PARM_VIDEO_PORT_POOL_SIZE  "Video_Port_Pool_Size"
#define SETTINGS_PARM_SIP_PROTECTED_PORT    "Sip_Protected_Port"
#define SETTINGS_PARM_SIP_PROTECTED_PORT_POOL_SIZE \
                                            "Sip_Protected_Port_Pool_Size"
#define SETTINGS_PARM_IPSEC_SPI             "Ipsec_Spi"
#define SETTINGS_PARM_IPSEC_SPI_POOL_SIZE   "Ipsec_Spi_Pool_Size"
#define SETTINGS_PARM_AKA_KI                "Aka_Ki"
#define SETTINGS_PARM_AKA_OP                "Aka_Opo"
#define SETTINGS_PARM_AKA_OPC               "Aka_Opc"
#define SETTINGS_PARM_AUDIO_CONF_SERVER     "Audio_Conf_Server"
#define SETTINGS_PARM_VIDEO_CONF_SERVER     "Video_Conf_Server"

/* MC parameters */
#define SETTINGS_PARM_RTP_PORT              "Rtp_Port"
#define SETTINGS_PARM_RTCP_INTERVAL         "Rtcp_Interval"
#define SETTINGS_PARM_TONE_AUTO_CALLPROGRESS \
                                            "Tone_Auto_Callprogress"
#define SETTINGS_PARM_TIMER_RTP_INACTIVITY  "Timer_Rtp_Inactivity"

/* RIR parameters */
#define SETTINGS_PARM_WM_PROXY              "Wm_Proxy"
#define SETTINGS_PARM_LOG_FILE              "Log_File"
#define SETTINGS_PARM_PING_SERVER           "Ping_Server"
#define SETTINGS_PARM_ACCEPT_ALL            "Accept_All"
#define SETTINGS_PARM_RESET_DELAY           "Reset_Delay"
#define SETTINGS_PARM_NOTIFY                "Notify"
#define SETTINGS_PARM_INTERFACE_NAME        "Name"
#define SETTINGS_PARM_INTERFACE_TYPE        "Type"
#define SETTINGS_PARM_INTERFACE_ENABLED     "Enabled"
#define SETTINGS_PARM_INTERFACE_PRIORITY    "Priority"

/*
 * SETTINGS attribute string definition
 */
#define SETTINGS_ATTR_NAME                  "name"
#define SETTINGS_ATTR_VALUE                 "value"
#define SETTINGS_ATTR_ID                    "id"
#define SETTINGS_ATTR_CAP_DISCOVERY         "cap-discovery"
#define SETTINGS_ATTR_COMMON_STACK          "common-stack"

/*
 * Parameter value strings.
 */
#define SETTINGS_PARM_VALUE_SIP_O_UDP       "SIPoUDP"
#define SETTINGS_PARM_VALUE_SIP_O_TCP       "SIPoTCP"
#define SETTINGS_PARM_VALUE_SIP_O_TLS       "SIPoTLS"
#define SETTINGS_PARM_VALUE_SRTP            "SRTP"
#define SETTINGS_PARM_VALUE_MSRP            "MSRP"
#define SETTINGS_PARM_VALUE_MSRP_O_TLS      "MSRPoTLS"
#define SETTINGS_PARM_VALUE_PDU_FMT_TPDU    "TPDU"
#define SETTINGS_PARM_VALUE_PDU_FMT_RPDU    "RPDU"
#define SETTINGS_PARM_VALUE_SMS_TYPE_3GPP   "3GPP"
#define SETTINGS_PARM_VALUE_SMS_TYPE_3GPP2  "3GPP2"

typedef enum {
    SETTINGS_RETURN_OK      = 0,
    SETTINGS_RETURN_ERROR
} SETTINGS_Return;

typedef enum {
    SETTINGS_TYPE_CSM       = 0,
    SETTINGS_TYPE_SAPP,
#ifdef INCLUDE_MC
    SETTINGS_TYPE_MC,
#endif
#ifdef INCLUDE_ISIM
    SETTINGS_TYPE_ISIM,
#endif
#ifdef INCLUDE_MGT
    SETTINGS_TYPE_MGT,
#endif
#ifdef INCLUDE_GAPP
    SETTINGS_TYPE_GAPP,
#endif
#ifdef INCLUDE_RIR
    SETTINGS_TYPE_RIR,
#endif
    SETTINGS_TYPE_LAST
} SETTINGS_Type;

/* xml configuration files folder path */
#ifndef SETTINGS_CONFIG_DEFAULT_FOLDER
#define SETTINGS_CONFIG_DEFAULT_FOLDER "//system//bin"
#endif

#ifdef VPORT_4G_PLUS_APROC
#define SETTINGS_CSM_XML_FILE_NAME      "csm_aproc.xml"
#define SETTINGS_SAPP_XML_FILE_NAME     "sapp_aproc.xml"
#define SETTINGS_RIR_XML_FILE_NAME      "rir_aproc.xml"
#define SETTINGS_GAPP_XML_FILE_NAME     "gapp_aproc.xml"
#else
#define SETTINGS_CSM_XML_FILE_NAME      "csm.xml"
#define SETTINGS_SAPP_XML_FILE_NAME     "sapp.xml"
#define SETTINGS_RIR_XML_FILE_NAME      "rir.xml"
#define SETTINGS_GAPP_XML_FILE_NAME     "gapp.xml"
#endif

#define SETTINGS_MC_XML_FILE_NAME       "mc.xml"
#define SETTINGS_ISIM_XML_FILE_NAME     "isim.xml"
#define SETTINGS_MGT_XML_FILE_NAME      "mgt.xml"

typedef enum {
    SETTINGS_NESTED_NONE    = 0,
    SETTINGS_NESTED_ONE,
    SETTINGS_NESTED_TWO
} SETTINGS_Nested_Level;

void * SETTINGS_cfgMemAlloc(
    int         cfgIndex);

void SETTINGS_memFreeDoc(
    int         cfgIndex,
    void       *cfg_ptr);

SETTINGS_Return SETTINGS_stringToContainer(
    int         cfgIndex,
    char       *doc_ptr,
    int         docLen,
    void       *cfg_ptr);

SETTINGS_Return SETTINGS_getContainer(
    int         cfgIndex,
    const char *filePath,
    void       *cfg_ptr);

char* SETTINGS_getParmValue(
    int         cfgIndex,
    int         nestedMode,
    void       *cfg_ptr,
    const char *tag_ptr,
    const char *chdOne_ptr,
    const char *chdTwo_ptr,
    const char *parm_ptr);

char* SETTINGS_getAttrValue(
    int         cfgIndex,
    int         nestedMode,
    void       *cfg_ptr,
    const char *tag_ptr,
    const char *chdOne_ptr,
    const char *chdTwo_ptr,
    const char *attr_ptr);

char* SETTINGS_xmlGetParmValue(
    ezxml_t     xml_ptr,
    const char *tag_ptr,
    const char *parmName_ptr);

char* SETTINGS_xmlGet2NestedParmValue(
    ezxml_t     xml_ptr,
    const char *parentTag_ptr,
    const char *childOneTag_ptr,
    const char *childTwoTag_ptr,
    const char *parmName_ptr);

char* SETTINGS_xmlGetNestedParmValue(
    ezxml_t     xml_ptr,
    const char *parentTag_ptr,
    const char *childTag_ptr,
    const char *parmName_ptr);

char* SETTINGS_xmlGetTagAttribute(
    ezxml_t     xml_ptr,
    const char *tag_ptr,
    const char *attr_ptr);

char* SETTINGS_xmlGet2NestedTagAttribute(
    ezxml_t     xml_ptr,
    const char *parentTag_ptr,
    const char *childOneTag_ptr,
    const char *childTwoTag_ptr,
    const char *attr_ptr);

#endif
