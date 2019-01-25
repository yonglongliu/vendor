/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30369 $ $Date: 2014-12-11 19:09:13 +0800 (Thu, 11 Dec 2014) $
 *
 * Description: 
 * ISI header file. 
 * XXX: Auto generation of Java Natives
 *  - This file in not D2 coding standard compliant. e.g. ISI_INP, ISI_OUT etc.
 *  - No comments allowed in this file, after this header comment
 *  - See ISI programmer's guide for description of API defined in this file
 */

#ifndef _ISI_H_
#define _ISI_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <osal.h>

#define ISI_INP /* input variable */
#define ISI_OUT /* output variable */
#define ISI_IO  /* input / output variable */

typedef enum {
    ISI_ADDRESS_STRING_SZ                   = 255,
    ISI_LONG_ADDRESS_STRING_SZ              = 511,
    ISI_COUNTRY_STRING_SZ                   = 3,
    ISI_SUBJECT_STRING_SZ                   = 127,
    ISI_PRESENCE_STRING_SZ                  = 1023,
    //ISI_TEXT_STRING_SZ                    = 2304,
    ISI_TEXT_STRING_SZ                      = 512,
    ISI_CODER_TABLE_STRING_SZ               = 1023,
    ISI_CODER_STRING_SZ                     = 31,
    ISI_CODER_DESCRIPTION_STRING_SZ         = 127,
    ISI_SECURITY_KEY_STRING_SZ              = 40,
    ISI_VERSION_STRING_SZ                   = 32,
    ISI_FILE_PATH_STRING_SZ                 = 1023,
    ISI_FILE_TRANSFER_ID_SZ                 = 127,
    ISI_ID_STRING_SZ                        = 31,
    ISI_DATE_TIME_STRING_SZ                 = 31,
    ISI_EVENT_DESC_STRING_SZ                = 127,
    ISI_AKA_AUTH_RESP_STRING_SZ             = 16,
    ISI_AKA_AUTH_CK_STRING_SZ               = 16,
    ISI_AKA_AUTH_IK_STRING_SZ               = 16,
    ISI_AKA_AUTH_RAND_STRING_SZ             = 16,
    ISI_AKA_AUTH_AUTN_STRING_SZ             = 16,
    ISI_AKA_AUTH_AUTS_STRING_SZ             = 14,
    ISI_PASSWORD_SZ                         = 7,
    ISI_BSID_STRING_SZ                      = 24,
    ISI_CONTRIBUTION_ID_STRING_SZ           = 49,
    ISI_HISTORY_INFO_STRING_SZ              = 128,
    ISI_USSD_STRING_SZ                      = 1023,
    ISI_INSTANCE_STRING_SZ                  = 64,
    ISI_PROVISIONING_DATA_STRING_SZ         = 1023,
    ISI_TEL_EVENT_STRING_SZ                 = 31,
    ISI_MEDIA_ATTR_STRING_SZ                = 1023
} ISI_SringSize;


typedef enum {
    ISI_AUDIO_CODER_NUM                     = 8,
    ISI_VIDEO_CODER_NUM                     = 4,
    ISI_CODER_NUM                           = ISI_AUDIO_CODER_NUM + 
                                                  ISI_VIDEO_CODER_NUM,
    ISI_CONF_USERS_NUM                      = 6
} ISI_Count;

typedef enum {
    ISI_RETURN_OK                           = 0,
    ISI_RETURN_FAILED                       = 1,
    ISI_RETURN_TIMEOUT                      = 2,
    ISI_RETURN_INVALID_CONF_ID              = 3,
    ISI_RETURN_INVALID_CALL_ID              = 4,
    ISI_RETURN_INVALID_TEL_EVENT_ID         = 5,
    ISI_RETURN_INVALID_SERVICE_ID           = 6,
    ISI_RETURN_INVALID_PRESENCE_ID          = 7,
    ISI_RETURN_INVALID_MESSAGE_ID           = 8,
    ISI_RETURN_INVALID_COUNTRY              = 9,
    ISI_RETURN_INVALID_PROTOCOL             = 10,
    ISI_RETURN_INVALID_CREDENTIALS          = 11,
    ISI_RETURN_INVALID_SESSION_DIR          = 12,
    ISI_RETURN_INVALID_SERVER_TYPE          = 13,
    ISI_RETURN_INVALID_ADDRESS              = 14,
    ISI_RETURN_INVALID_TEL_EVENT            = 15,
    ISI_RETURN_INVALID_CODER                = 16,
    ISI_RETURN_NOT_SUPPORTED                = 17,
    ISI_RETURN_DONE                         = 18,
    ISI_RETURN_SERVICE_ALREADY_ACTIVE       = 19,
    ISI_RETURN_SERVICE_NOT_ACTIVE           = 20,
    ISI_RETURN_SERVICE_BUSY                 = 21,
    ISI_RETURN_NOT_INIT                     = 22,
    ISI_RETURN_MUTEX_ERROR                  = 23,
    ISI_RETURN_INVALID_TONE                 = 24,
    ISI_RETURN_INVALID_CHAT_ID              = 25,
    ISI_RETURN_INVALID_FILE_ID              = 26,
    ISI_RETURN_OK_4G_PLUS                   = 27,
    ISI_RETURN_NO_MEM                       = 28,
    ISI_RETURN_LAST                         = 29
} ISI_Return;

typedef enum {
    ISI_CALL_STATE_INITIATING               = 0,
    ISI_CALL_STATE_INCOMING                 = 1,
    ISI_CALL_STATE_ACTIVE                   = 2,
    ISI_CALL_STATE_ONHOLD                   = 3,
    ISI_CALL_STATE_XFERING                  = 4,
    ISI_CALL_STATE_XFEREE                   = 5,
    ISI_CALL_STATE_XFERWAIT                 = 6,
    ISI_CALL_STATE_INCOMING_RSRC_NOT_READY  = 7,
    ISI_CALL_STATE_INVALID                  = 8
} ISI_CallState;

typedef enum {
    ISI_CHAT_STATE_INITIATING          = 0,
    ISI_CHAT_STATE_ACTIVE              = 1,
    ISI_CHAT_STATE_INVALID             = 2
} ISI_ChatState;

typedef enum {
    ISI_SESSION_TYPE_NONE                   = 0,
    ISI_SESSION_TYPE_AUDIO                  = 1,
    ISI_SESSION_TYPE_VIDEO                  = 2,
    ISI_SESSION_TYPE_CHAT                   = 4,   /* MSRP */
    ISI_SESSION_TYPE_EMERGENCY              = 8,
    ISI_SESSION_TYPE_SECURITY_AUDIO         = 16,
    ISI_SESSION_TYPE_SECURITY_VIDEO         = 32,
    ISI_SESSION_TYPE_CONFERENCE             = 64,
    ISI_SESSION_TYPE_SECURITY_CHAT          = 128, /* MSRPoTLS */
    ISI_SESSION_TYPE_INVALID                = 256
} ISI_SessionType;

typedef enum {
    ISI_MSG_RPT_NONE                        = 0,

    ISI_MSG_RPT_DELIVERY_SUCCESS            = 1,
    ISI_MSG_RPT_DELIVERY_FAILED             = 2,
    ISI_MSG_RPT_DELIVERY_FORBIDDEN          = 4,
    ISI_MSG_RPT_DELIVERY_ERROR              = 8,
    
    ISI_MSG_RPT_DISPLAY_SUCCESS             = 16,
    ISI_MSG_RPT_DISPLAY_ERROR               = 32,
    ISI_MSG_RPT_DISPLAY_FORBIDDEN           = 64,
            
    ISI_MSG_RPT_PROCESSING_SUCCESS          = 128,
    ISI_MSG_RPT_PROCESSING_ERROR            = 256,
    ISI_MSG_RPT_PROCESSING_FORBIDDEN        = 512,
    ISI_MSG_RPT_PROCESSING_STORED           = 1024,
    
    ISI_MSG_RPT_INVALID                     = 2048
} ISI_MessageReport;

typedef enum {
    ISI_MSG_TYPE_TEXT                       = 0,
    ISI_MSG_TYPE_PDU_3GPP,
    ISI_MSG_TYPE_PDU_3GPP2
} ISI_MessageType;

typedef enum {
    ISI_FILE_ATTR_RENDER     = 1,
    ISI_FILE_ATTR_ATTACHMENT = 2
} ISI_FileAttribute;

typedef enum {
    ISI_FILE_SUCCESS_REPORT  = 1,
    ISI_FILE_FAILURE_REPORT  = 2
} ISI_FileReport;

typedef enum {
    ISI_FILETYPE_NONE                       = 0,
    ISI_FILETYPE_TEXT_PLAIN                 = 1,
    ISI_FILETYPE_PHOTO_JPEG                 = 2,
    ISI_FILETYPE_PHOTO_GIF                  = 3,
    ISI_FILETYPE_PHOTO_BMP                  = 4,
    ISI_FILETYPE_PHOTO_PNG                  = 5,
    ISI_FILETYPE_VIDEO_3GP                  = 6,
    ISI_FILETYPE_VIDEO_MP4                  = 7,
    ISI_FILETYPE_VIDEO_WMV                  = 8,
    ISI_FILETYPE_VIDEO_AVC                  = 9,
    ISI_FILETYPE_VIDEO_MPEG                 = 10,
    ISI_FILETYPE_AUDIO_AMR                  = 11,
    ISI_FILETYPE_AUDIO_AAC                  = 12,
    ISI_FILETYPE_AUDIO_MP3                  = 13,
    ISI_FILETYPE_AUDIO_WMA                  = 14,
    ISI_FILETYPE_AUDIO_M4A                  = 15,
    ISI_FILETYPE_AUDIO_3GPP                 = 16,
    ISI_FILETYPE_INVALID                    = 17
} ISI_FileType;

typedef enum {
    ISI_ID_TYPE_NONE                        = 0,
    ISI_ID_TYPE_CALL                        = 1,
    ISI_ID_TYPE_TEL_EVENT                   = 2,
    ISI_ID_TYPE_SERVICE                     = 3,
    ISI_ID_TYPE_PRESENCE                    = 4,
    ISI_ID_TYPE_MESSAGE                     = 5,
    ISI_ID_TYPE_CHAT                        = 6,
    ISI_ID_TYPE_FILE                        = 7,
    ISI_ID_TYPE_USSD                        = 8,
    ISI_ID_TYPE_INVALID                     = 9
} ISI_IdType;

typedef enum {
    ISI_SESSION_DIR_NONE                    = -1,
    ISI_SESSION_DIR_INACTIVE                = 0,
    ISI_SESSION_DIR_SEND_ONLY               = 1,
    ISI_SESSION_DIR_RECV_ONLY               = 2,
    ISI_SESSION_DIR_SEND_RECV               = 3,
    ISI_SESSION_DIR_INVALID                 = 4
} ISI_SessionDirection;

typedef enum {
    ISI_RESOURCE_STATUS_NOT_READY           = 0,
    ISI_RESOURCE_STATUS_REMOTE_READY        = 1,
    ISI_RESOURCE_STATUS_LOCAL_READY         = 2,
    ISI_RESOURCE_STATUS_FAILURE             = 4,
} ISI_ResourceStatus;

typedef enum {
    ISI_SRVCC_STATUS_NONE                   = 1,
    ISI_SRVCC_STATUS_INCOMING_NOTIFY        = 2,
    ISI_SRVCC_STATUS_ALERTING_SEND          = 4,
    ISI_SRVCC_STATUS_ALERTING_RECEIVE       = 8,
    ISI_SRVCC_STATUS_MID_CALL_SEND          = 16,
    ISI_SRVCC_STATUS_MID_CALL_RECEIVE       = 32,
} ISI_SrvccStatus;

typedef enum {
    ISI_SUPSRV_HFEXIST_NONE                 = 0,
    ISI_SUPSRV_HFEXIST_ALERT_INFO_CW        = 1,
    ISI_SUPSRV_HFEXIST_HISTORY_INFO         = 2,
    ISI_SUPSRV_HFEXIST_VIRTUAL_RING         = 4,
    ISI_SUPSRV_HFEXIST_MEDIA_CONVERT        = 8, /* Support audio/video convert */
    /* ISI_SUPSRV_HFEXIST_XXX_INFO      = 4, template for future */
} ISI_SupsrvHfExist;

typedef enum {
    ISI_EVENT_NONE                          = 0,
    ISI_EVENT_NET_UNAVAILABLE               = 1,
    ISI_EVENT_CONTACT_RECEIVED              = 2,
    ISI_EVENT_CONTACT_SEND_OK               = 3,
    ISI_EVENT_CONTACT_SEND_FAILED           = 4,
    ISI_EVENT_SUB_TO_PRES_RECEIVED          = 5,
    ISI_EVENT_SUB_TO_PRES_SEND_OK           = 6,
    ISI_EVENT_SUB_TO_PRES_SEND_FAILED       = 7,
    ISI_EVENT_SUBSCRIPTION_RESP_RECEIVED    = 8,
    ISI_EVENT_SUBSCRIPTION_RESP_SEND_OK     = 9,
    ISI_EVENT_SUBSCRIPTION_RESP_SEND_FAILED = 10,
    ISI_EVENT_PRES_RECEIVED                 = 11,
    ISI_EVENT_PRES_SEND_OK                  = 12,
    ISI_EVENT_PRES_SEND_FAILED              = 13,
    ISI_EVENT_MESSAGE_RECEIVED              = 14,
    ISI_EVENT_MESSAGE_REPORT_RECEIVED       = 15,
    ISI_EVENT_MESSAGE_SEND_OK               = 16,
    ISI_EVENT_MESSAGE_SEND_FAILED           = 17,
    ISI_EVENT_CREDENTIALS_REJECTED          = 18,
    ISI_EVENT_TEL_EVENT_RECEIVED            = 19,
    ISI_EVENT_TEL_EVENT_SEND_OK             = 20,
    ISI_EVENT_TEL_EVENT_SEND_FAILED         = 21,
    ISI_EVENT_SERVICE_ACTIVE                = 22,
    ISI_EVENT_SERVICE_INACTIVE              = 23,
    ISI_EVENT_SERVICE_INIT_OK               = 24,
    ISI_EVENT_SERVICE_INIT_FAILED           = 25,
    ISI_EVENT_SERVICE_HANDOFF               = 26,
    ISI_EVENT_CALL_FAILED                   = 27,
    ISI_EVENT_CALL_XFER_PROGRESS            = 28,
    ISI_EVENT_CALL_XFER_REQUEST             = 29,
    ISI_EVENT_CALL_XFER_FAILED              = 30,
    ISI_EVENT_CALL_XFER_COMPLETED           = 31,
    ISI_EVENT_CALL_MODIFY_FAILED            = 32,
    ISI_EVENT_CALL_MODIFY_COMPLETED         = 33,
    ISI_EVENT_CALL_HOLD                     = 34,
    ISI_EVENT_CALL_RESUME                   = 35,
    ISI_EVENT_CALL_REJECTED                 = 36,
    ISI_EVENT_CALL_ACCEPTED                 = 37,
    ISI_EVENT_CALL_ACKNOWLEDGED             = 38,
    ISI_EVENT_CALL_DISCONNECTED             = 39,
    ISI_EVENT_CALL_INCOMING                 = 40,
    ISI_EVENT_CALL_TRYING                   = 41,
    ISI_EVENT_CALL_HANDOFF                  = 42,
    ISI_EVENT_PROTOCOL_READY                = 43,
    ISI_EVENT_PROTOCOL_FAILED               = 44,

    ISI_EVENT_CHAT_INCOMING                 = 45,
    ISI_EVENT_CHAT_TRYING                   = 46,
    ISI_EVENT_CHAT_ACKNOWLEDGED             = 47,
    ISI_EVENT_CHAT_ACCEPTED                 = 48,
    ISI_EVENT_CHAT_DISCONNECTED             = 49,
    ISI_EVENT_CHAT_FAILED                   = 50,
    ISI_EVENT_GROUP_CHAT_INCOMING           = 51,
    ISI_EVENT_GROUP_CHAT_PRES_RECEIVED      = 52,
    ISI_EVENT_GROUP_CHAT_NOT_AUTHORIZED     = 53,
    ISI_EVENT_DEFERRED_CHAT_INCOMING        = 54,

    ISI_EVENT_CALL_MODIFY                   = 55,
    ISI_EVENT_FILE_SEND_PROGRESS            = 56,
    ISI_EVENT_FILE_SEND_PROGRESS_COMPLETED  = 57,
    ISI_EVENT_FILE_SEND_PROGRESS_FAILED     = 58,
    ISI_EVENT_FILE_RECV_PROGRESS            = 59,
    ISI_EVENT_FILE_RECV_PROGRESS_COMPLETED  = 60,
    ISI_EVENT_FILE_RECV_PROGRESS_FAILED     = 61,
    ISI_EVENT_CALL_FLASH_HOLD               = 68,
    ISI_EVENT_CALL_FLASH_RESUME             = 69,
    ISI_EVENT_PRES_CAPS_RECEIVED            = 70,

    ISI_EVENT_MESSAGE_COMPOSING_ACTIVE      = 71,
    ISI_EVENT_MESSAGE_COMPOSING_IDLE        = 72,
    ISI_EVENT_FILE_REQUEST                  = 73, /* for RCS-e content share */
    ISI_EVENT_FILE_ACCEPTED                 = 74, /* file and image transfer */
    ISI_EVENT_FILE_REJECTED                 = 75, /*            .            */
    ISI_EVENT_FILE_PROGRESS                 = 76, /*            .            */
    ISI_EVENT_FILE_COMPLETED                = 77, /*            .            */
    ISI_EVENT_FILE_FAILED                   = 78, /*            .            */
    ISI_EVENT_FILE_CANCELLED                = 79, /*            .            */
    ISI_EVENT_FILE_ACKNOWLEDGED             = 80, /*            .            */

    ISI_EVENT_FILE_TRYING                   = 81, /*            .            */
    ISI_EVENT_AKA_AUTH_REQUIRED             = 82, /* AKA authentication required */
    ISI_EVENT_IPSEC_SETUP                   = 83, /*
                                                   * IPSec protected port and
                                                   * spi setup information.
                                                   */
    ISI_EVENT_IPSEC_RELEASE                 = 84, /*
                                                   * IPSec protected port and
                                                   * spi release information.
                                                   */
    ISI_EVENT_USSD_REQUEST                  = 85,
    ISI_EVENT_USSD_DISCONNECT               = 86,
    ISI_EVENT_USSD_SEND_OK                  = 87,
    ISI_EVENT_USSD_SEND_FAILED              = 88,
    ISI_EVENT_CALL_HANDOFF_START            = 89,
    ISI_EVENT_CALL_HANDOFF_SUCCESS          = 90,

    ISI_EVENT_CALL_HANDOFF_FAILED           = 91,
    ISI_EVENT_CALL_BEING_FORWARDED          = 92,
    /* Indicate RCS provisioning data is set */
    ISI_EVENT_RCS_PROVISIONING              = 93, 
    ISI_EVENT_CALL_ACCEPT_ACK               = 94, /* Received ACK to 200 OK */
    ISI_EVENT_CALL_RTP_INACTIVE_TMR_DISABLE = 95,
    ISI_EVENT_CALL_RTP_INACTIVE_TMR_ENABLE  = 96,
    ISI_EVENT_SERVICE_ACTIVATING            = 97, /* Not active yet, activating */
    ISI_EVENT_CALL_VIDEO_REQUEST_KEY        = 98, /* Request a key frame */
    ISI_EVENT_CALL_CANCEL_MODIFY            = 99,
    ISI_EVENT_CALL_EARLY_MEDIA              = 100,

    ISI_EVENT_LAST,
    ISI_EVENT_INVALID                       = ISI_EVENT_LAST
} ISI_Event;

typedef enum {
    ISI_TEL_EVENT_DTMF                      = 0,
    ISI_TEL_EVENT_DTMF_OOB                  = 1,
    ISI_TEL_EVENT_VOICEMAIL                 = 2,
    ISI_TEL_EVENT_CALL_FORWARD              = 3,
    ISI_TEL_EVENT_FEATURE                   = 4,
    ISI_TEL_EVENT_FLASHHOOK                 = 5,
    ISI_TEL_EVENT_RESERVED_1                = 6,
    ISI_TEL_EVENT_RESERVED_2                = 7,
    ISI_TEL_EVENT_RESERVED_3                = 8,
    ISI_TEL_EVENT_RESERVED_4                = 9,
    ISI_TEL_EVENT_RESERVED_5                = 10,
    ISI_TEL_EVENT_SEND_USSD                 = 11,
    ISI_TEL_EVENT_GET_SERVICE_ATTIBUTE      = 12,
    ISI_TEL_EVENT_DTMF_STRING               = 13,
    ISI_TEL_EVENT_DTMF_DETECT               = 14,
    ISI_TEL_EVENT_INVALID                   = 15
} ISI_TelEvent;

typedef enum {
    ISI_DTMFDECT_TYPE_LEADING               = 0,
    ISI_DTMFDECT_TYPE_TRAILING
} ISI_DtmfDectType;

typedef enum {
    ISI_SERVER_TYPE_STUN                    = 0,
    ISI_SERVER_TYPE_PROXY                   = 1,
    ISI_SERVER_TYPE_REGISTRAR               = 2,
    ISI_SERVER_TYPE_OUTBOUND_PROXY          = 3,
    ISI_SERVER_TYPE_RELAY                   = 4,
    ISI_SERVER_TYPE_STORAGE                 = 5,
    ISI_SERVER_TYPE_CHAT                    = 6,
    ISI_SERVER_TYPE_INVALID                 = 7
} ISI_ServerType;

typedef enum {
    ISI_PORT_TYPE_SIP                       = 0,
    ISI_PORT_TYPE_AUDIO                     = 1,
    ISI_PORT_TYPE_VIDEO                     = 2,
    ISI_PORT_TYPE_INVALID                   = 3
} ISI_PortType;

typedef enum {
    ISI_MEDIA_CNTRL_SPEAKER = 0,
    ISI_MEDIA_CNTRL_AEC_BYPASS,
    ISI_MEDIA_CNTRL_GAIN_CTRL,
    ISI_MEDIA_CNTRL_CN_GAIN_CTRL,
    ISI_MEDIA_CNTRL_LAST,
} ISI_MediaControl;

typedef enum {
    ISI_FORWARD_NONE                        = -1,
    ISI_FORWARD_UNCONDITIONAL               = 0,
    ISI_FORWARD_BUSY                        = 1,
    ISI_FORWARD_NO_REPLY                    = 2,
    ISI_FORWARD_UNREACHABLE                 = 3,
    ISI_FORWARD_INVALID                     = 4
} ISI_FwdCond;

typedef enum {
    ISI_TONE_NONE                           = -1,
    ISI_TONE_DTMF_0                         = 0,
    ISI_TONE_DTMF_1                         = 1,
    ISI_TONE_DTMF_2                         = 2,
    ISI_TONE_DTMF_3                         = 3,
    ISI_TONE_DTMF_4                         = 4,
    ISI_TONE_DTMF_5                         = 5,
    ISI_TONE_DTMF_6                         = 6,
    ISI_TONE_DTMF_7                         = 7,
    ISI_TONE_DTMF_8                         = 8,
    ISI_TONE_DTMF_9                         = 9,
    ISI_TONE_DTMF_STAR                      = 10,
    ISI_TONE_DTMF_POUND                     = 11,
    ISI_TONE_DTMF_A                         = 12,
    ISI_TONE_DTMF_B                         = 13,
    ISI_TONE_DTMF_C                         = 14,
    ISI_TONE_DTMF_D                         = 15,
    ISI_TONE_CALL_WAITING                   = 16,
    ISI_TONE_BUSY                           = 17,
    ISI_TONE_RINGBACK                       = 18,
    ISI_TONE_ERROR                          = 19,
    ISI_TONE_LAST                           = 20,
} ISI_AudioTone;

typedef enum {
    ISI_SERVICE_ATTRIBUTE_PCSCF  = 0,
    ISI_SERVICE_ATTRIBUTE_IMPU   = 1,
    ISI_SERVICE_ATTRIBUTE_IMPI   = 2,
    ISI_SERVICE_ATTRIBUTE_DOMAIN = 3,
    ISI_SERVICE_ATTRIBUTE_AUTH   = 4,
} ISI_SeviceAttribute;

typedef enum {
    ISI_SERVICE_AKA_RESPONSE_SUCCESS = 0,
    ISI_SERVICE_AKA_RESPONSE_NETWORK_FAILURE,
    ISI_SERVICE_AKA_RESPONSE_SYNC_FAILURE
} ISI_ServiceAkaRespResult;

typedef enum {
    ISI_SESSION_CID_TYPE_NONE = 0,
    ISI_SESSION_CID_TYPE_INVOCATION,
    ISI_SESSION_CID_TYPE_SUPPRESSION,
    ISI_SESSION_CID_TYPE_USE_ALIAS, /* Use alias CID */
} ISI_SessionCidType;

typedef enum {
    ISI_NETWORK_ACCESS_TYPE_NONE = 0,
    ISI_NETWORK_ACCESS_TYPE_3GPP_GERAN,
    ISI_NETWORK_ACCESS_TYPE_3GPP_UTRAN_FDD,
    ISI_NETWORK_ACCESS_TYPE_3GPP_UTRAN_TDD,
    ISI_NETWORK_ACCESS_TYPE_3GPP_E_UTRAN_FDD,
    ISI_NETWORK_ACCESS_TYPE_3GPP_E_UTRAN_TDD,
    ISI_NETWORK_ACCESS_TYPE_IEEE_802_11,
} ISI_NetworkAccessType;

typedef enum {
    ISI_FEATURE_TYPE_VOLTE_CALL = 1,
    ISI_FEATURE_TYPE_VOLTE_SMS  = 2,
    ISI_FEATURE_TYPE_RCS        = 4,
} ISI_FeatureType;

typedef enum {
    ISI_USSD_TYPE_SEND             = 0,
    ISI_USSD_TYPE_REPLY,
    ISI_USSD_TYPE_DISCONNECT
} ISI_UssdType;

typedef enum {
    ISI_NETWORK_MODE_LTE,
    ISI_NETWORK_MODE_WIFI
} ISI_NetworksMode;

/* RT MEDIA  protocol enumeration */
typedef enum {
    ISI_TRASPORT_PROTO_RT_MEDIA_RTP = 0,
    ISI_TRASPORT_PROTO_RT_MEDIA_SRTP,
} ISI_RTMedia;

typedef unsigned int ISI_Id;

ISI_Return ISI_allocate(
    ISI_INP char *country_ptr);

ISI_Return ISI_init(
    ISI_INP char *country_ptr);

ISI_Return ISI_getVersion(
    ISI_OUT char *version_ptr);

ISI_Return ISI_shutdown(
    void);

ISI_Return ISI_getEvent(
    ISI_OUT ISI_Id     *serviceId_ptr,
    ISI_OUT ISI_Id     *id_ptr,
    ISI_OUT ISI_IdType *idType_ptr,
    ISI_OUT ISI_Event  *event_ptr,
    ISI_OUT char       *eventDesc_ptr,
    ISI_INP int         timeout);

ISI_Return ISI_allocService(
    ISI_OUT ISI_Id *serviceId_ptr,
    ISI_INP int     protocol);

ISI_Return ISI_activateService(
    ISI_INP ISI_Id serviceId);

ISI_Return ISI_deactivateService(
    ISI_INP ISI_Id serviceId);

ISI_Return ISI_freeService(
    ISI_INP ISI_Id serviceId);

ISI_Return ISI_serviceMakeCidPrivate(
    ISI_INP ISI_Id serviceId,
    ISI_INP int    value);

ISI_Return ISI_serviceSetBsid (
    ISI_INP ISI_Id                 serviceId,
    ISI_INP ISI_NetworkAccessType  type,
    ISI_INP char                  *bsid_ptr);

ISI_Return ISI_serviceSetImeiUri (
    ISI_INP ISI_Id  serviceId,
    ISI_INP char   *imei_ptr);

ISI_Return ISI_serviceSetInterface(
    ISI_INP ISI_Id  serviceId,
    ISI_INP char   *name_ptr,
    ISI_INP char   *address_ptr);

ISI_Return ISI_serviceSetUri(
    ISI_INP ISI_Id  serviceId,
    ISI_INP char   *uri_ptr);

ISI_Return ISI_serviceSetInstanceId(
    ISI_INP ISI_Id  serviceId,
    ISI_INP char   *instance_ptr);

ISI_Return ISI_serviceSetServer(
    ISI_INP ISI_Id          serviceId,
    ISI_INP char           *server_ptr,
    ISI_INP ISI_ServerType  type);

ISI_Return ISI_serviceSetPort(
    ISI_INP ISI_Id        serviceId,
    ISI_INP int           port,
    ISI_INP int           poolSize,
    ISI_INP ISI_PortType  type);

ISI_Return ISI_serviceSetCredentials(
    ISI_INP ISI_Id  serviceId,
    ISI_INP char   *username_ptr,
    ISI_INP char   *password_ptr,
    ISI_INP char   *realm_ptr);

ISI_Return ISI_serviceSetIpsec(
    ISI_INP ISI_Id  serviceId,
    ISI_INP int     port,
    ISI_INP int     portPoolSz,
    ISI_INP int     spi,
    ISI_INP int     spiPoolSz);

ISI_Return ISI_serviceBlockUser(
    ISI_INP ISI_Id  serviceId,
    ISI_INP char   *username_ptr);

ISI_Return ISI_serviceUnblockUser(
    ISI_INP ISI_Id  serviceId,
    ISI_INP char   *username_ptr);

ISI_Return ISI_serviceSetCapabilities(
    ISI_INP ISI_Id  serviceId,
    ISI_INP char   *capabilities_ptr);

ISI_Return ISI_addCoderToService(
    ISI_INP ISI_Id  serviceId,
    ISI_INP char   *coderName_ptr,
    ISI_INP char   *coderDescription_ptr);

ISI_Return ISI_removeCoderFromService(
    ISI_INP ISI_Id  serviceId,
    ISI_INP char   *coderName_ptr);

ISI_Return ISI_removeCoderFromServiceByPayloadType(
    ISI_INP ISI_Id  serviceId,
    ISI_INP int     payloadType);

ISI_Return ISI_serviceForwardCalls(
    ISI_OUT ISI_Id      *evtId_ptr,
    ISI_INP ISI_Id       serviceId,
    ISI_INP ISI_FwdCond  condition,
    ISI_INP int          enable,
    ISI_INP char        *to_ptr,
    ISI_INP int          timeout);

ISI_Return ISI_sendTelEventToRemote(
    ISI_OUT ISI_Id       *evtId_ptr,
    ISI_INP ISI_Id        callId,
    ISI_INP ISI_TelEvent  telEvent,
    ISI_INP int           arg0,
    ISI_INP int           arg1);

ISI_Return ISI_sendTelEventStringToRemote(
    ISI_OUT ISI_Id       *evtId_ptr,
    ISI_INP ISI_Id        callId,
    ISI_INP ISI_TelEvent  telEvent,
    ISI_INP char         *string_ptr,
    ISI_INP int           durationMs);

ISI_Return ISI_getTelEventFromRemote(
    ISI_INP ISI_Id        evtId,
    ISI_OUT ISI_Id       *callId_ptr,
    ISI_OUT ISI_TelEvent *telEvent_ptr,
    ISI_OUT int          *arg0_ptr,
    ISI_OUT int          *arg1_ptr,
    ISI_OUT char         *from_ptr,
    ISI_OUT char         *dateTime_ptr);

/*
 * DEPRECATED: Do not use this routine.  There should be no reason
 * to use this routine.  When the response is received a
 * ISI_EVENT_TEL_EVENT_SEND_OK or ISI_EVENT_TEL_EVENT_SEND_FAILED event
 * is generated with the ISI id of the event.  There's no reason for
 * ISI to track the 'event type' (a.k.a. ISI_TelEvent).  That's the
 * applications responsibility.
 */
ISI_Return ISI_getTelEventResponse(
    ISI_INP ISI_Id        evtId,
    ISI_OUT ISI_TelEvent *telEvent_ptr);

ISI_Return ISI_initiateCall(
    ISI_OUT ISI_Id            *callId_ptr,
    ISI_INP ISI_Id             serviceId,
    ISI_INP char              *to_ptr,
    ISI_INP char              *subject_ptr,
    ISI_INP ISI_SessionCidType cidType,
    ISI_INP char              *mediaAttribute_ptr);

ISI_Return ISI_sendUSSD(
    ISI_OUT ISI_Id          *evtId_ptr,
    ISI_INP ISI_Id           serviceId,
    ISI_INP char            *ussd);


ISI_Return ISI_sendUssd(
    ISI_OUT ISI_Id            *msgId_ptr,
    ISI_INP ISI_Id             serviceId,
    ISI_INP ISI_UssdType       reason,
    ISI_INP char              *to_ptr,
    ISI_INP char              *msg_ptr);    

ISI_Return ISI_terminateCall(
    ISI_INP ISI_Id callId,
    ISI_INP char* reasonDesc);

ISI_Return ISI_modifyCall(
    ISI_INP ISI_Id callId,
    ISI_INP char   *reason);

ISI_Return ISI_cancelModifyCall(
    ISI_INP ISI_Id callId);

ISI_Return ISI_acceptCallModify(
    ISI_INP ISI_Id callId);

ISI_Return ISI_rejectCallModify(
    ISI_INP ISI_Id  callId,
    ISI_INP char   *reason);

ISI_Return ISI_getCallState(
    ISI_INP ISI_Id         callId,
    ISI_OUT ISI_CallState *callState_ptr);

ISI_Return ISI_addCoderToCall(
    ISI_INP ISI_Id  callId,
    ISI_INP char   *coderName_ptr,
    ISI_INP char   *coderDescription_ptr);

ISI_Return ISI_removeCoderFromCall(
    ISI_INP ISI_Id  callId,
    ISI_INP char   *coderName_ptr);

ISI_Return ISI_setCallSessionDirection(
    ISI_INP ISI_Id               callId,
    ISI_INP ISI_SessionType      callType,
    ISI_INP ISI_SessionDirection direction);

ISI_Return ISI_getCallSessionDirection(
    ISI_INP ISI_Id                callId,
    ISI_INP ISI_SessionType       callType,
    ISI_OUT ISI_SessionDirection *callDir_ptr);

ISI_Return ISI_getCallHeader(
    ISI_INP ISI_Id   callId,
    ISI_OUT char    *subject_ptr,
    ISI_OUT char    *from_ptr);

ISI_Return ISI_getSupsrvHeader(
    ISI_INP ISI_Id   callId,
    ISI_OUT int     *supsrvHfExist_ptr);

ISI_Return ISI_getSupsrvHistoryInfo(
    ISI_INP ISI_Id   callId,
    ISI_OUT char    *historyInfo_ptr);

ISI_Return ISI_getCallSessionType(
    ISI_INP ISI_Id           callId,
    ISI_OUT uint16          *callType_ptr);

ISI_Return ISI_updateCallSession(
    ISI_INP ISI_Id           callId,
    ISI_INP char            *mediaAttribute_ptr);

ISI_Return ISI_getCoderDescription(
    ISI_INP ISI_Id  callId,
    ISI_INP char   *codername_ptr,
    ISI_OUT char   *coderDescription_ptr);

ISI_Return ISI_acknowledgeCall(
    ISI_INP ISI_Id callId);

ISI_Return ISI_holdCall(
    ISI_INP ISI_Id callId);

ISI_Return ISI_resumeCall(
    ISI_INP ISI_Id callId);

ISI_Return ISI_acceptCall(
    ISI_INP ISI_Id callId);

ISI_Return ISI_rejectCall(
    ISI_INP ISI_Id  callId,
    ISI_INP char   *reason_ptr);

ISI_Return ISI_forwardCall(
    ISI_INP ISI_Id  callId,
    ISI_INP char   *to_ptr);

ISI_Return ISI_blindTransferCall(
    ISI_INP ISI_Id  callId,
    ISI_INP char   *to_ptr);

ISI_Return ISI_attendedTransferCall(
    ISI_INP ISI_Id  callId,
    ISI_INP char   *to_ptr);

ISI_Return ISI_consultativeTransferCall(
    ISI_INP ISI_Id  callId,
    ISI_INP ISI_Id  toCallId);

ISI_Return ISI_confTransferCall(
    ISI_INP ISI_Id  callId,
    ISI_INP char   *to_ptr,
    ISI_INP char   *rsrcList_ptr);

ISI_Return ISI_initiateConfCall(
    ISI_OUT ISI_Id            *callId_ptr,
    ISI_INP ISI_Id             serviceId,
    ISI_INP char              *to_ptr,
    ISI_INP char              *subject_ptr,
    ISI_INP ISI_SessionCidType cidType,
    ISI_INP char              *mediaAttribute_ptr,
    ISI_INP char              *rsrcList_ptr);

ISI_Return ISI_conferenceKickCall(
    ISI_INP ISI_Id  callId,
    ISI_INP char   *participant_ptr);

ISI_Return ISI_generateTone(
    ISI_INP ISI_Id        callId,
    ISI_INP ISI_AudioTone tone,
    ISI_INP int           duration);

ISI_Return ISI_stopTone(
    ISI_INP ISI_Id        callId);

ISI_Return ISI_startConfCall(
    ISI_OUT ISI_Id *confId_ptr,
    ISI_INP ISI_Id  callId0,
    ISI_INP ISI_Id  callId1);

ISI_Return ISI_addCallToConf(
    ISI_INP ISI_Id confId,
    ISI_INP ISI_Id callId);

ISI_Return ISI_removeCallFromConf(
    ISI_INP ISI_Id confId,
    ISI_INP ISI_Id callId);

ISI_Return ISI_sendMessage(
    ISI_OUT ISI_Id                  *msgId_ptr,
    ISI_INP ISI_Id                   serviceId,
    ISI_INP ISI_MessageType          type,
    ISI_INP char                    *to_ptr,
    ISI_INP char                    *subject_ptr,
    ISI_INP char                    *msg_ptr,
    ISI_INP int                      msgLen,
    ISI_INP ISI_MessageReport        report,
    ISI_INP char                    *reportId_ptr);

ISI_Return ISI_sendMessageReport(
    ISI_OUT ISI_Id                  *msgId_ptr,
    ISI_INP ISI_Id                   serviceId,
    ISI_INP char                    *to_ptr,
    ISI_INP ISI_MessageReport        report,
    ISI_INP char                    *reportId_ptr);

ISI_Return ISI_readMessageReport(
    ISI_INP ISI_Id                   msgId,
    ISI_OUT ISI_Id                  *chatId_ptr,
    ISI_OUT char                    *from_ptr,
    ISI_OUT char                    *dateTime_ptr,
    ISI_OUT ISI_MessageReport       *report_ptr,
    ISI_OUT char                    *reportId_ptr);

ISI_Return ISI_getMessageHeader(
    ISI_INP ISI_Id              msgId,
    ISI_OUT ISI_MessageType    *type_ptr,
    ISI_OUT char               *subject_ptr,
    ISI_OUT char               *from_ptr,
    ISI_OUT char               *dateTime_ptr,
    ISI_OUT ISI_MessageReport  *reports_ptr,
    ISI_OUT char               *reportId_ptr);

ISI_Return ISI_readMessage(
    ISI_INP ISI_Id  msgId,
    ISI_OUT ISI_Id *chatId_ptr,
    ISI_OUT char   *part_ptr,
    ISI_IO  int    *len_ptr); // (i/o) variable

ISI_Return ISI_sendFile(
    ISI_OUT ISI_Id             *fileId_ptr,
    ISI_INP ISI_Id              serviceId,
    ISI_INP char               *to_ptr,
    ISI_INP char               *subject_ptr,
    ISI_INP char               *filePath_ptr,
    ISI_INP ISI_FileType        fileType,
    ISI_INP ISI_FileAttribute   attribute,
    ISI_INP int                 fileSize,
    ISI_INP ISI_FileReport      report);

ISI_Return ISI_acceptFile(
    ISI_INP ISI_Id   fileId);

ISI_Return ISI_acknowledgeFile(
    ISI_INP ISI_Id fileId);

ISI_Return ISI_rejectFile(
    ISI_INP ISI_Id         fileId,
    ISI_INP const char    *reason_ptr);

ISI_Return ISI_cancelFile(
    ISI_INP ISI_Id         fileId,
    ISI_INP const char    *reason_ptr);

ISI_Return ISI_getFileHeader(
    ISI_INP ISI_Id        fileId,
    ISI_OUT char         *subject_ptr,
    ISI_OUT char         *from_ptr);

ISI_Return ISI_readFileProgress(
    ISI_INP ISI_Id              fileId,
    /* ISI_OUT ISI_Id       *chatId_ptr, */
    ISI_OUT char                *filePath_ptr,
    ISI_OUT ISI_FileType        *fileType_ptr,
    ISI_OUT ISI_FileAttribute   *fileAttr_ptr,
    ISI_OUT int                 *fileSize_ptr,
    ISI_OUT int                 *progress_ptr);

ISI_Return ISI_addContact(
    ISI_OUT ISI_Id *presId_ptr,
    ISI_INP ISI_Id  serviceId,
    ISI_INP char   *user_ptr,
    ISI_INP char   *group_ptr,
    ISI_INP char   *name_ptr);

ISI_Return ISI_removeContact(
    ISI_OUT ISI_Id *presId_ptr,
    ISI_INP ISI_Id  serviceId,
    ISI_INP char   *user_ptr,
    ISI_INP char   *group_ptr,
    ISI_INP char   *name_ptr);

ISI_Return ISI_readContact(
    ISI_INP ISI_Id  presId,
    ISI_OUT char   *user_ptr,
    ISI_OUT char   *group_ptr,
    ISI_OUT char   *name_ptr,
    ISI_OUT char   *subscription_ptr,
    ISI_INP int     len);

ISI_Return ISI_subscribeToPresence(
    ISI_OUT ISI_Id *presId_ptr,
    ISI_INP ISI_Id  serviceId,
    ISI_INP char   *user_ptr);

ISI_Return ISI_unsubscribeFromPresence(
    ISI_OUT  ISI_Id *presId_ptr,
    ISI_INP  ISI_Id  serviceId,
    ISI_INP  char   *user_ptr);

ISI_Return ISI_readSubscribeToPresenceRequest(
    ISI_INP ISI_Id  presId,
    ISI_OUT char   *user_ptr);

ISI_Return ISI_readSubscriptionToPresenceResponse(
    ISI_INP ISI_Id  presId,
    ISI_OUT char   *user_ptr,
    ISI_OUT int    *isAllowed_ptr);

ISI_Return ISI_allowSubscriptionToPresence(
    ISI_OUT ISI_Id *presId_ptr,
    ISI_INP ISI_Id  serviceId,
    ISI_INP char   *user_ptr);

ISI_Return ISI_denySubscriptionToPresence(
    ISI_OUT  ISI_Id *presId_ptr,
    ISI_INP  ISI_Id  serviceId,
    ISI_INP  char   *user_ptr);

ISI_Return ISI_sendPresence(
    ISI_OUT ISI_Id *presId_ptr,
    ISI_INP ISI_Id  serviceId,
    ISI_INP char   *to_ptr,
    ISI_INP char   *presence_ptr);

ISI_Return ISI_readPresence(
    ISI_INP ISI_Id  presId,
    ISI_OUT ISI_Id *callId_ptr,
    ISI_OUT char   *from_ptr,
    ISI_OUT char   *presence_ptr,
    ISI_INP int     len);

ISI_Return ISI_sendCapabilities(
    ISI_OUT ISI_Id *capsId_ptr,
    ISI_INP ISI_Id  serviceId,
    ISI_INP char   *to_ptr,
    ISI_INP char   *capabilities_ptr,
    ISI_INP int     priority);

ISI_Return ISI_readCapabilities(
    ISI_INP ISI_Id  capsId,
    ISI_OUT ISI_Id *capsId_ptr,
    ISI_OUT char   *from_ptr,
    ISI_OUT char   *capabilities_ptr,
    ISI_INP int     len);

ISI_Return ISI_getCallHandoff(
    ISI_INP ISI_Id  callId,
    ISI_OUT char   *to_ptr);

ISI_Return ISI_setChatNickname(
    ISI_INP ISI_Id   serviceId,
    ISI_INP char    *nickname_ptr);

ISI_Return ISI_initiateGroupChat(
    ISI_OUT ISI_Id             *chatId_ptr,
    ISI_INP ISI_Id              serviceId,
    ISI_INP char               *roomName_ptr,
    ISI_INP char               *subject_ptr,
    ISI_INP char               *password_ptr,
    ISI_INP ISI_SessionType     sessionType);

ISI_Return ISI_initiateGroupChatAdhoc(
    ISI_OUT ISI_Id             *chatId_ptr,
    ISI_INP ISI_Id              serviceId,
    ISI_INP char               *participant_ptr,
    ISI_INP char               *conferenceUri_ptr,
    ISI_INP char               *subject_ptr,
    ISI_INP ISI_SessionType     sessionType);

ISI_Return ISI_initiateChat(
    ISI_OUT ISI_Id            *chatId_ptr,
    ISI_INP ISI_Id             serviceId,
    ISI_INP char              *to_ptr,
    ISI_INP char              *subject_ptr,
    ISI_INP char              *message_ptr,
    ISI_INP ISI_MessageReport  reports,
    ISI_INP char              *reportId_ptr,
    ISI_INP ISI_SessionType     sessionType);

ISI_Return ISI_acceptChat(
    ISI_INP ISI_Id   chatId);

ISI_Return ISI_rejectChat(
    ISI_INP ISI_Id   chatId,
    ISI_INP char    *reason_ptr);

ISI_Return ISI_acknowledgeChat(
    ISI_INP ISI_Id   chatId);

ISI_Return ISI_disconnectChat(
    ISI_INP ISI_Id   chatId);

ISI_Return ISI_getChatHeader(
    ISI_INP ISI_Id  chatId,
    ISI_OUT char   *subject_ptr,
    ISI_OUT char   *remoteAddress_ptr,
    ISI_OUT ISI_Id *messageId_ptr);

ISI_Return ISI_getGroupChatHeader(
    ISI_INP ISI_Id  chatId,
    ISI_OUT char   *subject_ptr,
    ISI_OUT char   *conferenceUri_ptr,
    ISI_OUT char   *participants_ptr);

ISI_Return ISI_readGroupChatPresence(
    ISI_INP ISI_Id  presId,
    ISI_OUT ISI_Id *chatId_ptr,
    ISI_OUT char   *from_ptr,
    ISI_OUT char   *presence_ptr,
    ISI_INP int     len);

ISI_Return ISI_inviteGroupChat(
    ISI_INP ISI_Id   chatId,
    ISI_INP char    *participant_ptr,
    ISI_INP char    *reason_ptr);

ISI_Return ISI_joinGroupChat(
    ISI_OUT ISI_Id             *chatId_ptr,
    ISI_INP ISI_Id              serviceId,
    ISI_INP char               *roomName_ptr,
    ISI_INP char               *password_ptr,
    ISI_INP ISI_SessionType     sessionType);

ISI_Return ISI_kickGroupChat(
    ISI_INP ISI_Id   chatId,
    ISI_INP char    *participant_ptr,
    ISI_INP char    *reason_ptr);

ISI_Return ISI_destroyGroupChat(
    ISI_INP ISI_Id   chatId,
    ISI_INP char    *reason_ptr);

ISI_Return ISI_sendChatMessage(
    ISI_OUT ISI_Id             *msgId_ptr,
    ISI_INP ISI_Id              chatId,
    ISI_INP char               *msg_ptr,
    ISI_INP ISI_MessageReport   report,
    ISI_INP char               *reportId_ptr);

ISI_Return ISI_composingChatMessage(
    ISI_INP ISI_Id              chatId);

ISI_Return ISI_sendChatMessageReport(
    ISI_OUT ISI_Id             *msgId_ptr,
    ISI_INP ISI_Id              chatId,
    ISI_INP ISI_MessageReport   report,
    ISI_INP char               *reportId_ptr);

ISI_Return ISI_sendChatFile(
    ISI_OUT ISI_Id       *msgId_ptr,
    ISI_INP ISI_Id        chatId,
    ISI_INP char         *subject_ptr,
    ISI_INP char         *filePath_ptr,
    ISI_INP ISI_FileType  fileType);

ISI_Return ISI_sendPrivateGroupChatMessage(
    ISI_OUT ISI_Id  *msgId_ptr,
    ISI_INP ISI_Id   chatId,
    ISI_INP char    *partipant_ptr,
    ISI_INP char    *msg_ptr);

ISI_Return ISI_sendGroupChatPresence(
    ISI_OUT ISI_Id *presId_ptr,
    ISI_INP ISI_Id  chatId,
    ISI_INP char   *pres_ptr);

ISI_Return ISI_getChatState(
    ISI_INP ISI_Id         chatId,
    ISI_OUT ISI_ChatState *chatState_ptr);

ISI_Return ISI_mediaControl(
    ISI_INP ISI_MediaControl cmd,
    ISI_INP int              arg);

ISI_Return ISI_serviceSetFilePath(
    ISI_INP ISI_Id          serviceId,
    ISI_INP char           *filePath_ptr,
    ISI_INP char           *filePrepend_ptr);

/* Pete1 to remove in future maybe?? */
ISI_Return ISI_getServiceAtribute(
    ISI_OUT ISI_Id              *id_ptr,
    ISI_INP ISI_Id               serviceId,
    ISI_INP ISI_SeviceAttribute  cmd,
    ISI_INP char                *arg1_ptr,
    ISI_INP char                *arg2_ptr);
/* Pete1 to remove in future maybe?? */
ISI_Return ISI_setAkaAuthResp(
    ISI_INP ISI_Id  serviceId,
    ISI_INP int     result,
    ISI_INP char   *resp_ptr,
    ISI_INP int     resLength,
    ISI_INP char   *auts_ptr,
    ISI_INP char   *ck_ptr,
    ISI_INP char   *ik_ptr);

ISI_Return ISI_getAkaAuthChallenge(
    ISI_INP ISI_Id  serviceId,
    ISI_OUT char   *rand_ptr,
    ISI_OUT char   *autn_ptr);

ISI_Return ISI_serviceGetIpsec(
    ISI_INP ISI_Id  serviceId,
    ISI_OUT int    *portUc_ptr,
    ISI_OUT int    *portUs_ptr,
    ISI_OUT int    *portPc_ptr,
    ISI_OUT int    *portPs_ptr,
    ISI_OUT int    *spiUc_ptr,
    ISI_OUT int    *spiUs_ptr,
    ISI_OUT int    *spiPc_ptr,
    ISI_OUT int    *spiPs_ptr);

/* Diagnostic API to do audio record. */
ISI_Return ISI_diagAudioRecord(
    ISI_INP char   *file_ptr);

/* Diagnostic API to do audio play. */
ISI_Return ISI_diagAudioPlay(
    ISI_INP char   *file_ptr);

ISI_Return ISI_serviceSetEmergency(
    ISI_INP ISI_Id  serviceId,
    ISI_INP int     isEmergency);

ISI_Return ISI_getNextService(
    ISI_IO  ISI_Id *serviceId_ptr,
    ISI_OUT int    *protocol_ptr,
    ISI_OUT int    *isEmergency_ptr,
    ISI_OUT int    *features_ptr,
    ISI_OUT int    *isActivated_ptr);

ISI_Return ISI_setFeature(
    ISI_INP int features);

ISI_Return ISI_serviceSetFeature(
    ISI_INP ISI_Id serviceId,
    ISI_INP int    features);

ISI_Return ISI_serviceGetFeature(
    ISI_INP ISI_Id serviceId,
    ISI_OUT int   *features_ptr);

ISI_Return ISI_setCallResourceStatus(
    ISI_INP ISI_Id               callId,
    ISI_INP ISI_ResourceStatus   rsrcStatus);

ISI_Return ISI_getCallResourceStatus(
    ISI_Id               callId,
    ISI_ResourceStatus  *rsrcStatus_ptr,
    int                 *audioRtpPort,
    int                 *videoRtPPort);

ISI_Return ISI_getCallSrvccStatus(
    ISI_INP ISI_Id               callId,
    ISI_OUT ISI_SrvccStatus     *srvccStatus_ptr);

ISI_Return ISI_readUssd(
    ISI_INP ISI_Id  msgId,
    ISI_OUT char   *part_ptr,
    ISI_IO  int    *len_ptr);

ISI_Return ISI_setProvisioningData(
    ISI_INP ISI_Id      serviceId,
    ISI_INP const char *xmlDoc_ptr);

#ifdef __cplusplus
}
#endif

#endif
