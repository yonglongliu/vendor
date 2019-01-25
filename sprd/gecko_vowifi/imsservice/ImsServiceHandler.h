#ifndef mozilla_dom_ImsServiceHandler_h
#define mozilla_dom_ImsServiceHandler_h

#include <nsIImsRegService.h>
#include "mozilla/StaticPtr.h"
#include <android/log.h>
#include "nsString.h"
#include "nsServiceManagerUtils.h"
#include "nsIRadioInterfaceLayer.h"
#include "nsIGonkTelephonyService.h"
#include "nsITelephonyService.h"
#include "nsITelephonyCallInfo.h"
#include "mozilla/Logging.h"

namespace mozilla {
namespace dom {
namespace imsservice {

class TelephonyListener : public nsITelephonyListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSITELEPHONYLISTENER

  TelephonyListener() { }

  bool Listen(bool aStart);

protected:
  virtual ~TelephonyListener() { }

private:
  nsresult HandleCallInfo(nsITelephonyCallInfo* aInfo);
};

class ImsServiceHandler final : public nsIImsServiceHandler
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIIMSSERVICEHANDLER
  enum {
    VOWIFI_REQUEST_TYPE_NULL = -1,
    VOWIFI_REQUEST_TYPE_DIAL = 0,
    VOWIFI_REQUEST_TYPE_GET_CURRENT_CALL = 1,
    VOWIFI_REQUEST_TYPE_HANGUP_CALL = 2,
    VOWIFI_REQUEST_TYPE_HANGUP_FOREGROUND = 3,
    VOWIFI_REQUEST_TYPE_HANGUP_BACKGROUND = 4,
    VOWIFI_REQUEST_TYPE_SWITCH_ACTIVE_CALL = 5,
    VOWIFI_REQUEST_TYPE_UDUB = 6,
    VOWIFI_REQUEST_TYPE_ANSWER_CALL = 7,
    VOWIFI_REQUEST_TYPE_CONFERENCE_CALL = 8,
    VOWIFI_REQUEST_TYPE_SEPARATE_CALL =9,
    VOWIFI_REQUEST_TYPE_SET_MUTE = 10,
    VOWIFI_REQUEST_TYPE_SEND_SMS = 11,
    VOWIFI_REQUEST_TYPE_SET_SMSC_ADDRESS = 12,
    VOWIFI_REQUEST_TYPE_GET_SMSC_ADDRESS = 13,
    VOWIFI_REQUEST_TYPE_ACK_SMS = 14
  };
  enum ImsRegState
  {
    IMS_REG_STATE_INACTIVE            = 0,
    IMS_REG_STATE_REGISTERED          = 1,
    IMS_REG_STATE_REGISTERING         = 2,
    IMS_REG_STATE_REG_FAIL            = 3,
    IMS_REG_STATE_UNKNOWN             = 4,
    IMS_REG_STATE_ROAMING             = 5,
    IMS_REG_STATE_DEREGISTERING       = 6
  };
  class ImsServiceState
  {
  public:
    bool mImsRegistered;
    int16_t mRegState;
    int16_t mSrvccState;
    ImsServiceState(bool imsRegistered,
                    int16_t regState,
                    int16_t srvccState)
    : mImsRegistered(imsRegistered)
    , mRegState(regState)
    , mSrvccState(srvccState)
    {}
    ~ImsServiceState() {}
  };

  const char* requestList[15] = {
    "dial",
    "getCurrentCalls",
    "hangUpCall",
    "hangUpForeground",
    "hangUpBackground",
    "switchActiveCall",
    "udub",
    "answerCall",
    "conferenceCall",
    "separateCall",
    "setMute",
    "sendSMS",
    "setSmscAddress",
    "getSmscAddress",
    "ackSMS"
  };

  //"VoLTE", "ViLTE", "VoWiFi", "ViWiFi","VOLTE-UT", "VOWIFI-UT"
  int16_t mEnabledFeatures[8] = {
          nsIImsService::FEATURE_TYPE_UNKNOWN,
          nsIImsService::FEATURE_TYPE_UNKNOWN,
          nsIImsService::FEATURE_TYPE_UNKNOWN,
          nsIImsService::FEATURE_TYPE_UNKNOWN,
          nsIImsService::FEATURE_TYPE_UNKNOWN,
          nsIImsService::FEATURE_TYPE_UNKNOWN,
          nsIImsService::FEATURE_TYPE_UNKNOWN,
          nsIImsService::FEATURE_TYPE_UNKNOWN
  };

  //"VoLTE", "ViLTE", "VoWiFi", "ViWiFi","VOLTE-UT", "VOWIFI-UT"
  int16_t mDisabledFeatures[8] = {
          nsIImsService::FEATURE_TYPE_UNKNOWN,
          nsIImsService::FEATURE_TYPE_UNKNOWN,
          nsIImsService::FEATURE_TYPE_UNKNOWN,
          nsIImsService::FEATURE_TYPE_UNKNOWN,
          nsIImsService::FEATURE_TYPE_UNKNOWN,
          nsIImsService::FEATURE_TYPE_UNKNOWN,
          nsIImsService::FEATURE_TYPE_UNKNOWN,
          nsIImsService::FEATURE_TYPE_UNKNOWN
  };

  class ImsRegisterListener;
  ImsServiceHandler(uint32_t aServiceId);

  class WorkerMessageCallback;
  friend class WorkerMessageCallback;

  void RequestImsHandover(int16_t type);
  void NotifyImsCallEnd(int16_t type);
  void NotifyImsHandoverStatus(int16_t status);
  void NotifyWifiCalling(bool isWifiCalling);
  void NotifyImsPdnStateChanged(int16_t pdnState);
  void NotifyDataRouter();
  void NotifyImsNetworkInfo(int16_t type, const nsAString & addr);
  void NotifyVoWifiEnable(bool aEnabled);
  void UpdateImsFeatures(bool volteEnable, bool wifiEnable);
  void SetImsPcscfAddress(const nsAString & addr);
  void NotifyImsRegister(bool aRegisted);
  void QueryImsPcscfAddress();
  void GetImsPcscfAddress(nsAString & addr);
  void NotifyHandoverCallInfo(const nsAString & callInfo);

private:
  uint32_t mServiceId;
  nsCOMPtr<nsIRadioInterface> mRadioInterface;
  // nsCOMPtr<nsITelephonyService> mTelephonyService;
  RefPtr<nsIImsRegListener> mImsRegListener;
  RefPtr<TelephonyListener> mTelephonyListener;
  nsCOMPtr<nsIImsRegHandler> mImsRegHandler;
  // nsCOMArray<nsITelephonyCallInfo> mCallInfoList;
  nsString mImsRegAddress;
  nsString mImsPscfAddress;
  bool mImsRegisted;
  ImsServiceState* mImsServiceState;
  virtual ~ImsServiceHandler();
};

} // namespace imsservice
} // namespace dom
} // namespace mozilla

#endif //mozilla_dom_ImsServiceHandler_h
