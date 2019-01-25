#ifndef __SPRD_VOWIFI_PORTING_CONSTANTS_H__
#define __SPRD_VOWIFI_PORTING_CONSTANTS_H__

#include "PortingMacro.h"
#include "nsIIccService.h"
#include "nsITelephonyService.h"
#include "nsINetworkInterface.h"
#include "nsIImsService.h"
#include <string>


BEGIN_VOWIFI_NAMESPACE

static const bool PORTING_DEBUG = false;

static const int INVALID_PHONE_ID   = -1;
static const int PRIMARY_PHONE_ID   = 0;

static const int WIFI_SIGNAL_STRENGTH_INVALID   = -127;
static const int LTE_SIGNAL_STRENGTH_DEFAULT    = -140;
static const int LTE_SIGNAL_STRENGTH_INVALID    = 0x7FFFFFFF;

static const std::string PORTING_TAG = "[PortingLayer] ";
static std::string gPortingLogString;

inline const char* _PTAG(const std::string& tag) {
    gPortingLogString = PORTING_TAG + tag;
    return gPortingLogString.c_str();
}

typedef enum {
    SIM_STATE_UNKNOWN           = nsIIcc::CARD_STATE_UNKNOWN,
    SIM_STATE_READY             = nsIIcc::CARD_STATE_READY,
    SIM_STATE_PIN_REQUIRED      = nsIIcc::CARD_STATE_PIN_REQUIRED,
    SIM_STATE_PUK_REQUIRED      = nsIIcc::CARD_STATE_PUK_REQUIRED,
    SIM_STATE_PERM_DISABLED     = nsIIcc::CARD_STATE_PERMANENT_BLOCKED, /* PERM_DISABLED means ICC is permanently disabled due to puk fails */
    SIM_STATE_NETWORK_LOCKED    = nsIIcc::CARD_STATE_NETWORK_LOCKED,    /* NETWORK_LOCKED means ICC is locked on NETWORK PERSONALIZATION */
    SIM_STATE_ABSENT            = nsIIcc::CARD_STATE_UNDETECTED,
    //SIM_STATE_NOT_READY,
} SIM_CARD_STATE;

typedef enum {
    SIM_CARD_TYPE_SIM,
    SIM_CARD_TYPE_NOT_SIM,
    SIM_CARD_TYPE_UNKNOWN = 0xFFFF,
} SIM_CARD_TYPE;

typedef enum {
    CALL_STATE_IDLE         = nsITelephonyService::CALL_STATE_DISCONNECTED,
    CALL_STATE_DIALING      = nsITelephonyService::CALL_STATE_DIALING,
    CALL_STATE_ALERTING     = nsITelephonyService::CALL_STATE_ALERTING,
    CALL_STATE_CONNECTED    = nsITelephonyService::CALL_STATE_CONNECTED,
    CALL_STATE_HELD         = nsITelephonyService::CALL_STATE_HELD,
    CALL_STATE_DISCONNECTED = nsITelephonyService::CALL_STATE_DISCONNECTED,
    CALL_STATE_INCOMING     = nsITelephonyService::CALL_STATE_INCOMING,
} CALL_STATE_TYPE;

typedef enum {
    WIFI_STATE_CONNECTING    = nsINetworkInfo::NETWORK_STATE_CONNECTING,
    WIFI_STATE_CONNECTED     = nsINetworkInfo::NETWORK_STATE_CONNECTED,
    WIFI_STATE_DISCONNECTING = nsINetworkInfo::NETWORK_STATE_DISCONNECTING,
    WIFI_STATE_DISCONNECTED  = nsINetworkInfo::NETWORK_STATE_DISCONNECTED,
} WIFI_STATE_TYPE;

typedef enum {
    IMS_TYPE_UNKNOWN        = nsIImsService::FEATURE_TYPE_UNKNOWN,
    IMS_TYPE_VOLTE          = nsIImsService::FEATURE_TYPE_VOICE_OVER_LTE,
    IMS_TYPE_VOWIFI         = nsIImsService::FEATURE_TYPE_VOICE_OVER_WIFI,
} IMS_REGISTER_TYPE;

typedef enum {
    IMS_CALL_TYPE_UNKNOWN   = -1,
    IMS_CALL_TYPE_AUDIO,
    IMS_CALL_TYPE_VIDEO     = 3,
} IMS_CALL_TYPE;

typedef enum {
    VOLTE_REG_STATE_INACTIVE,
    VOLTE_REG_STATE_REGISTERED,
    VOLTE_REG_STATE_REGISTERING,
    VOLTE_REG_STATE_FAILED,
    VOLTE_REG_STATE_UNKNOWN,
    VOLTE_REG_STATE_ROAMING,
    VOLTE_REG_STATE_DEREGISTERING,
} VOLTE_REGISTER_STATE;

END_VOWIFI_NAMESPACE

#endif
