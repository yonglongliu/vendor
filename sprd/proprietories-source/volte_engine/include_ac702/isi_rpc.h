/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30028 $ $Date: 2014-11-21 19:05:32 +0800 (Fri, 21 Nov 2014) $
 *
 */

#ifndef _ISI_RPC_H
#define _ISI_RPC_H_

#include <osal.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Queue name of isi rpc from server to client. */
#define ISI_RPC_UP_QUEUE_NAME       "isi-rpc-up-queue"
/* Queue name of isi rpc from cliet to server. */
#define ISI_RPC_DOWN_QUEUE_NAME     "isi-rpc-down-queue"
/* Queue name of isi evt rpc from server to client. */
#define ISI_EVT_RPC_UP_QUEUE_NAME   "isi-evt-rpc-up-queue"
/* Queue name of isi evt rpc from cliet to server. */
#define ISI_EVT_RPC_DOWN_QUEUE_NAME "isi-evt-rpc-down-queue"
/* ISI rpc and evt rpc queue size. */
#define ISI_RPC_MSGQ_LEN            (4)

typedef enum {
    ISI_INIT,
    ISI_GET_VERSION,
    ISI_SHUTDOWN,
    ISI_GET_EVENT,
    ISI_ALLOC_SERVICE,
    ISI_ACTIVATE_SERVICE,
    ISI_DEACTIVATE_SERVICE,
    ISI_FREE_SERVICE,
    ISI_SERVICE_MAKE_CID_PRIVATE,
    ISI_SERVICE_SET_BSID,
    ISI_SERVICE_SET_IMEI_URI,
    ISI_SERVICE_SET_INTERFACE,
    ISI_SERVICE_SET_URI,
    ISI_SERVICE_SET_CAPABILITIES,
    ISI_SERVICE_SET_SERVER,
    ISI_SERVICE_SET_PORT,
    ISI_SERVICE_SET_IPSEC,
    ISI_SERVICE_SET_FILE_PATH,
    ISI_SERVICE_SET_CREDENTIALS,
    ISI_SERVICE_SET_EMERGENCY,
    ISI_SERVICE_SET_DEVICEID,
    ISI_SERVICE_BLOCK_USER,
    ISI_SERVICE_UNBLOCK_USER,
    ISI_ADD_CODER_TO_SERVICE,
    ISI_REMOVE_CODER_FROM_SERVICE,
    ISI_SERVICE_FORWARD_CALLS,
    ISI_INITIATE_CALL,
    ISI_SEND_USSD,
    ISI_TERMINATE_CALL,
    ISI_MODIFY_CALL,
    ISI_ACCEPT_CALL_MODIFY,
    ISI_REJECT_CALL_MODIFY,
    ISI_GET_CALL_STATE,
    ISI_ADD_CODER_TO_CALL,
    ISI_REMOVE_CODER_FROM_CALL,
    ISI_GET_CODER_DESCRIPTION,
    ISI_GET_CALL_HANDOFF,
    ISI_GET_CALL_HEADER,
    ISI_GET_SUPSRV_HEADER,
    ISI_GET_SUPSRV_HISTORY_INFO,
    ISI_SET_CALL_SESSION_DIRECTION,
    ISI_GET_CALL_SESSION_DIRECTION,
    ISI_SET_CALL_SESSION_TYPE,
    ISI_GET_CALL_SESSION_TYPE,
    ISI_ACKNOWLEDGE_CALL,
    ISI_HOLD_CALL,
    ISI_RESUME_CALL,
    ISI_ACCEPT_CALL,
    ISI_REJECT_CALL,
    ISI_FORWARD_CALL,
    ISI_BLIND_TRANSFER_CALL,
    ISI_ATTENDED_TRANSFER_CALL,
    ISI_CONSULTATIVE_TRANSFER_CALL,
    ISI_GENERATE_TONE,
    ISI_STOP_TONE,
    ISI_SEND_TELEVENT_TO_REMOTE,
    ISI_SEND_TELEVENT_STRING_TO_REMOTE,
    ISI_GET_TELEVENT_FROM_REMOTE,
    ISI_GET_TELEVENT_RESPONSE,
    ISI_START_CONF_CALL,
    ISI_ADD_CALL_TO_CONF,
    ISI_REMOVE_CALL_FROM_CONF,
    ISI_SEND_MESSAGE,
    ISI_SEND_MESSAGE_REPORT,
    ISI_READ_MESSAGE_REPORT,
    ISI_GET_MESSAGE_HEADER,
    ISI_READ_MESSAGE,
    ISI_SEND_FILE,
    ISI_ACCEPT_FILE,
    ISI_ACKNOWLEDGE_FILE,
    ISI_REJECT_FILE,
    ISI_CANCEL_FILE,
    ISI_READ_FILE_PROGRESS,
    ISI_GET_FILE_HEADER,
    ISI_ADD_CONTACT,
    ISI_REMOVE_CONTACT,
    ISI_READ_CONTACT,
    ISI_SUBSCRIBE_TO_PRESENCE,
    ISI_UNSUBSCRIBE_FROM_PRESENCE,
    ISI_READ_SUBSCRIBE_TO_PRESENCE_REQUEST,
    ISI_READ_SUBSCRIPTION_TO_PRESENCE_RESPONSE,
    ISI_ALLOW_SUBSCRIPTION_TO_PRESENCE,
    ISI_DENY_SUBSCRIPTION_TO_PRESENCE,
    ISI_SEND_PRESENCE,
    ISI_READ_PRESENCE,
    ISI_SEND_CAPABILITIES,
    ISI_READ_CAPABILITIES,
    ISI_SET_CHAT_NICKNAME,
    ISI_INITIATE_GROUP_CHAT,
    ISI_INITIATE_GROUP_CHAT_ADHOC,
    ISI_INITIATE_CHAT,
    ISI_ACCEPT_CHAT,
    ISI_REJECT_CHAT,
    ISI_ACKNOWLEDGE_CHAT,
    ISI_DISCONNECT_CHAT,
    ISI_GET_CHAT_HEADER,
    ISI_GET_GROUP_CHAT_HEADER,
    ISI_READ_GROUP_CHAT_PRESENCE,
    ISI_INVITE_GROUP_CHAT,
    ISI_JOIN_GROUP_CHAT,
    ISI_KICK_GROUP_CHAT,
    ISI_DESTROY_GROUP_CHAT,
    ISI_SEND_CHAT_MESSAGE,
    ISI_COMPOSING_CHAT_MESSAGE,
    ISI_SEND_CHAT_MESSAGE_REPORT,
    ISI_SEND_CHAT_FILE,
    ISI_SEND_PRIVATE_GROUP_CHAT_MESSAGE,
    ISI_SEND_GROUP_CHAT_PRESENCE,
    ISI_GET_CHAT_STATE,
    ISI_MEDIA_CONTROL,
    ISI_GET_SERVICE_ATRIBUTE,
    ISI_SET_AKA_AUTH_RESP,
    ISI_GET_AKA_AUTH_CHALLENGE,
    ISI_SERVICE_GET_IPSEC,
    ISI_DIAG_AUDIO_RECORD,
    ISI_DIAG_AUDIO_PLAY,
    ISI_GET_NEXT_SERVICE,
    ISI_SET_FEATURE,
    ISI_SERVICE_SET_FEATURE,
    ISI_SERVICE_GET_FEATURE,
    ISI_SET_CALL_RESOURCE_STATUS,
    ISI_GET_CALL_RESOURCE_STATUS,
    ISI_GET_CALL_SRVCC_STATUS,
    ISI_READ_USSD,
    ISI_SET_PROVISIONING_DATA,
    ISI_UPDATE_CALL_SESSION,
    ISI_FUNC_LAST
} ISI_ApiName;

#ifndef INCLUDE_4G_PLUS_RCS
#define ISI_serverAllocate()                    (OSAL_SUCCESS)
#define ISI_serverStart()                       (OSAL_SUCCESS)
#define ISI_serverInit()                        (OSAL_SUCCESS)
#define ISI_serverShutdown()
#define ISI_serverGetEvent                      ISI_getEvent
#define ISI_serverGetProvisioningData(...)      (ISI_RETURN_OK)
#define ISI_serverSetNetworkMode(...)
#define ISI_serverSetMediaSessionType(...)      (ISI_RETURN_OK)
#define ISI_serverSetRTMediaSessionType(...)    (ISI_RETURN_OK)
#define ISI_serverSetDomain(a)
#define ISI_serverSetConfCallId(a)              (ISI_RETURN_OK)

#else
#ifndef ISI_RPC_DEBUG
#define ISI_rpcDbgPrintf(fmt, args...)
#else
#define ISI_rpcDbgPrintf(fmt, args...) \
         OSAL_logMsg("[%s:%d] " fmt, __FUNCTION__, __LINE__, ## args)
#endif

OSAL_Status ISI_serverAllocate(
    void);

OSAL_Status ISI_serverStart(
    void);

OSAL_Status ISI_serverInit(
    void);

OSAL_Status ISI_serverShutdown(
    void);

ISI_Return ISI_serverGetEvent(
    ISI_Id     *serviceId_ptr,
    ISI_Id     *id_ptr,
    ISI_IdType *idType_ptr,
    ISI_Event  *event_ptr,
    char       *eventDesc_ptr,
    int         timeout);

ISI_Return ISI_serverGetProvisioningData(
    ISI_Id serviceId,
    char   *xmlDoc_ptr,
    char   *uri_ptr,
    char   *username_ptr,
    char   *password_ptr,
    char   *realm_ptr,
    char   *proxy_ptr,
    char   *obProxy_ptr,
    char   *imConfUri_ptr);

ISI_Return ISI_serverSetNetworkMode(
    ISI_NetworksMode mode);

ISI_Return ISI_serverSetDomain(
    const char *domain_ptr);

ISI_Return ISI_serverSetMediaSessionType(
    ISI_NetworksMode    mode,
    ISI_SessionType     type);

ISI_Return ISI_serverSetRTMediaSessionType(
    ISI_NetworksMode    mode,
    ISI_RTMedia     type);

ISI_Return ISI_serverSetConfCallId(
    ISI_Id  callId);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _ISI_RPC_H_ */
