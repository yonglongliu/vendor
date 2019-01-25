#ifndef mozilla_dom_ImsService_h
#define mozilla_dom_ImsService_h

#include "nsIImsService.h"
#include "nsIImsRegService.h"
#include "nsIGonkImsRegService.h"
#include "ImsServiceHandler.h"
#include "mozilla/StaticPtr.h"
#include <android/log.h>
#include <cutils/properties.h>

#include "nsServiceManagerUtils.h"
#include "nsIRadioInterfaceLayer.h"
#include "nsIGonkTelephonyService.h"
#include "nsITelephonyService.h"
#include "mozilla/Logging.h"
#include "ImsServiceHandler.h"
#include "nsIVowifiCallback.h"
#include "nsIVowifiInterfaceLayer.h"
#include "nsIMobileConnectionInfo.h"
#include "nsIMobileConnectionService.h"
#include "nsIImsConnManInterfaceLayer.h"

#include "nsIServiceManager.h"
#include "nsIWifi.h"
#include <nsIInterfaceRequestor.h>
#include "nsIInterfaceRequestorUtils.h"
#include "nsITimer.h"

#include "mozilla/dom/MozWifiManagerBinding.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/Navigator.h"
// #include "nsGlobalWindow.h"

namespace mozilla {
namespace dom {
namespace imsservice {
 
static LazyLogModule ImsServiceLog("ImsService");

// #define LOG( ... ) MOZ_LOG(ImsServiceLog, (mozilla::LogLevel)4, ( __VA_ARGS__ ))
class ImsService : public nsIImsService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIIMSSERVICE

  static already_AddRefed<ImsService> GetInstance();
  ImsService();

  class ImsServiceRequest
  {
  public:
    int16_t mRequestId;
    int16_t mEventCode;
    uint32_t mServiceId;
    int16_t mTargetType;
    ImsServiceRequest(int16_t requestId,
                      int16_t eventCode,
                      uint32_t serviceId,
                      int16_t targetType)
    : mRequestId(requestId)
    , mEventCode(eventCode)
    , mServiceId(serviceId)
    , mTargetType(targetType)
    {}
    nsString ToString()
    {
      nsString result;
      result.AppendLiteral("mRequestId:");
      result.Append(mRequestId);
      result.AppendLiteral("mEventCode:");
      result.Append(mEventCode);
      result.AppendLiteral("mServiceId:");
      result.Append(mServiceId);
      result.AppendLiteral("mTargetType:");
      result.Append(mTargetType);
      return result;
    }
    ~ImsServiceRequest() {}
  };

  class WifiScanInfo;
  friend class WifiScanInfo;

  class VoWifiCallback;
  friend class VoWifiCallback;

  enum CallEndEvent
  {
    WIFI_CALL_END = 1,
    VOLTE_CALL_END = 2
  };

  enum ImsPDNStatus
  {
    IMS_PDN_ACTIVE_FAILED = 0,
    IMS_PDN_READY = 1,
    IMS_PDN_START = 2
  };

  /** IMS registered state code. */
  enum ImsRegStatus
  {
    IMS_REG_STATE_INACTIVE            = 0,
    IMS_REG_STATE_REGISTERED          = 1,
    IMS_REG_STATE_REGISTERING         = 2,
    IMS_REG_STATE_REG_FAIL            = 3,
    IMS_REG_STATE_UNKNOWN             = 4,
    IMS_REG_STATE_ROAMING             = 5,
    IMS_REG_STATE_DEREGISTERING       = 6
  };

  /** IMS service code. */
  enum ImsServiceCode
  {
    ACTION_SWITCH_IMS_FEATURE = 100,
    ACTION_START_HANDOVER = 101,
    ACTION_NOTIFY_NETWORK_UNAVAILABLE = 102,
    ACTION_NOTIFY_VOWIFI_UNAVAILABLE = 103,
    ACTION_CANCEL_CURRENT_REQUEST = 104,
    ACTION_RELEASE_WIFI_RESOURCE = 105
  };

  /** WIFI service code. */
  enum WifiServiceCode
  {
    EVENT_WIFI_ATTACH_STATE_UPDATE = 200,
    EVENT_WIFI_ATTACH_SUCCESSED = 201,
    EVENT_WIFI_ATTACH_FAILED = 202,
    EVENT_WIFI_ATTACH_STOPED = 203,
    EVENT_WIFI_INCOMING_CALL = 204,
    EVENT_WIFI_ALL_CALLS_END = 205,
    EVENT_WIFI_REFRESH_RESAULT = 206,
    EVENT_WIFI_REGISTER_RESAULT = 207,
    EVENT_WIFI_RESET_RESAULT = 208,
    EVENT_WIFI_DPD_DISCONNECTED = 209,
    EVENT_WIFI_NO_RTP = 210,
    EVENT_WIFI_UNSOL_UPDATE = 211,
    EVENT_WIFI_RTP_RECEIVED = 212,
    EVENT_UPDATE_DATA_ROUTER_FINISHED = 213,
    EVENT_NOTIFY_CP_VOWIFI_ATTACH_SUCCESSED = 214
  };

  enum ImsHandoverType
  {
    IDEL_HANDOVER_TO_VOWIFI = 1,
    IDEL_HANDOVER_TO_VOLTE = 2,
    INCALL_HANDOVER_TO_VOWIFI = 3,
    INCALL_HANDOVER_TO_VOLTE = 4
  };

  enum ImsOperationType
  {
    IMS_OPERATION_SWITCH_TO_VOWIFI = 0,
    IMS_OPERATION_SWITCH_TO_VOLTE = 1,
    IMS_OPERATION_HANDOVER_TO_VOWIFI = 2,
    IMS_OPERATION_HANDOVER_TO_VOLTE = 3,
    IMS_OPERATION_SET_VOWIFI_UNAVAILABLE = 4,
    IMS_OPERATION_CANCEL_CURRENT_REQUEST = 5,
    IMS_OPERATION_CP_REJECT_SWITCH_TO_VOWIFI = 6,
    IMS_OPERATION_CP_REJECT_HANDOVER_TO_VOWIFI = 7,
    IMS_OPERATION_RELEASE_WIFI_RESOURCE = 8
  };

  enum ImsHandoverResult
  {
    IMS_HANDOVER_REGISTER_FAIL = 0,
    IMS_HANDOVER_SUCCESS = 1,
    IMS_HANDOVER_PDN_BUILD_FAIL = 2,
    IMS_HANDOVER_RE_REGISTER_FAIL = 3,
    IMS_HANDOVER_ATTACH_FAIL = 4,
    IMS_HANDOVER_ATTACH_SUCCESS = 5,
    IMS_HANDOVER_SRVCC_FAILED = 6
  };

  enum UnsolicitedCode
  {
    SECURITY_DPD_DISCONNECTED = 1,
    SIP_TIMEOUT = 2,
    SIP_LOGOUT = 3,
    SECURITY_REKEY_FAILED = 4,
    SECURITY_STOP = 5
  };

  enum CallType
  {
    NO_CALL = -1,
    VOLTE_CALL = 0,
    WIFI_CALL = 2
  };

  enum S2bEventCode
  {
    S2b_STATE_IDLE = 0,
    S2b_STATE_PROGRESS = 1,
    S2b_STATE_CONNECTED = 2
  };

  enum ImsStackResetResult
  {
    INVALID_ID = -1,
    FAIL = 0,
    SUCCESS = 1
  };

  enum DataRouterState
  {
    CALL_VOLTE,
    CALL_VOWIFI,
    CALL_NONE
  };

  enum RegisterState
  {
    STATE_IDLE        = 0,
    STATE_PROGRESSING = 1,
    STATE_CONNECTED   = 2
  };

  enum IncomingCallAction {
    NORMAL,
    REJECT
  };

  enum WifiState
  {
    CONNECTED, 
    DISCONNECTED
  };

  enum VoLteServiceState
  {
    // Single Radio Voice Call Continuity(SRVCC) progress state
    HANDOVER_STARTED   = 0,
    HANDOVER_COMPLETED = 1,
    HANDOVER_FAILED    = 2,
    HANDOVER_CANCELED  = 3   
  };

  nsCOMPtr<nsIRadioInterfaceLayer> mRadioService;
  nsCOMPtr<nsIImsRegService> mImsRegService;
  nsCOMArray<ImsServiceHandler> mImsHandlers;
  nsCOMPtr<nsIVowifiInterfaceLayer> mWifiService;
  RefPtr<nsIVowifiCallback> mVoWifiCallback;
  RefPtr<nsIImsServiceCallback> mImsServiceCallback;

  nsCOMPtr<nsIRadioInterface> mRadioInterface;

  static void TimerSuccessCallback(nsITimer* aTimer, void* aClosure);
  static void TimerFailureCallback(nsITimer* aTimer, void* aClosure);
  nsresult SetNotifyCallbackTimeout(bool result);

  int16_t GetReuestId();
  void OnReceiveHandoverEvent(bool isCalling,
                              int16_t requestId,
                              int16_t targetType);
  void NotifyCpCallEnd();
  void NotifyWiFiError(uint16_t aErrorCode);
  void acquireConnectedWifiBSSID();
  void UpdateImsRegisterState();
  void UpdateImsFeature();
  void OnRegisterStateChange();
  void UpdateInCallState(bool isInCall);
  void NotifyCPVowifiAttachSucceed();
  bool IsWifiRegistered();
  void NotifyCMImsCapabilityChange(bool volteEnable, bool wifiEnable);
  void UpdateInCallType(bool volteCall, bool wifiCall);

private:
  uint32_t mNumRadioInterfaces;
  virtual ~ImsService();
  static StaticRefPtr<ImsService> sSingleton;
  uint32_t mPrimaryServiceId;
  uint16_t mRequestId;
  bool mVoLTERegisted;
  bool mIsCalling;
  bool mPendingActivePdnSuccess;
  bool mPendingAttachVowifiSuccess;
  bool mPendingVowifiHandoverVowifiSuccess;
  bool mPendingVolteHandoverVolteSuccess;
  bool mPendingReregister;
  bool mIsS2bStopped;
  bool mAttachVowifiSuccess;
  bool mIsVowifiCall;
  bool mIsVolteCall;
  bool mWifiRegistered;
  bool mVolteRegistered;

  bool mIsAPImsPdnActived;
  bool mIsPendingRegisterVowifi;
  bool mIsPendingRegisterVolte;
  bool mIsLoggingIn;
  bool mIsCPImsPdnActived;
  bool mPendingCPSelfManagement;
  int16_t mInCallHandoverFeature;
  int16_t mCurrentImsFeature;
  int16_t mCallEndType;
  int16_t mNetworkType;
  nsString mNetworkInfo;
  int16_t mAliveCallLose;
  int16_t mAliveCallJitter;
  int16_t mAliveCallRtt;
  int16_t mSrvccCapbility;
  bool mIsWifiCalling;
  ImsServiceRequest* mFeatureSwitchRequest;
  ImsServiceRequest* mReleaseVowifiRequest;
  nsString mConnectedWifiBSSID;
  nsCOMPtr<nsITimer> mTimer;
};
} // namespace imsservice
} // namespace dom
} // namespace mozilla

#endif //mozilla_dom_ImsService_h
