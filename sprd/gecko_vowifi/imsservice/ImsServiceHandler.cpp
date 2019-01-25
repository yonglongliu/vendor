#include "ImsService.h"
#include "ImsServiceHandler.h"
#include "mozilla/ClearOnShutdown.h"
#include "nsJSUtils.h"

#undef LOG
#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "ImsServiceHandler " , ## args)

namespace mozilla {
namespace dom {
namespace imsservice {

#define PRIMARY_SERVICE_ID 0

/**
 *  TelephonyListener Implementation
 */
NS_IMPL_ISUPPORTS(TelephonyListener, nsITelephonyListener)

nsresult
TelephonyListener::HandleCallInfo(nsITelephonyCallInfo* aInfo)
{
  uint32_t callIndex;
  uint16_t callState;
  uint32_t radioTech;

  aInfo->GetCallIndex(&callIndex);
  aInfo->GetCallState(&callState);
  aInfo->GetRadioTech(&radioTech);

  LOG("TelephonyListener HandleCallInfo callIndex:%d, callState:%d, radioTech:%d",
      callIndex, callState, radioTech);

  RefPtr<ImsService> imsService = ImsService::GetInstance();
  bool isInCall = false;

  if (callState != nsITelephonyService::CALL_STATE_DISCONNECTED &&
    callState != nsITelephonyService::CALL_STATE_UNKNOWN) {
    isInCall = true;
  }

  imsService->UpdateInCallState(isInCall);

  bool isVolteCall = false;
  bool isWifiCall = false;

  if (isInCall && radioTech == nsITelephonyCallInfo::RADIO_TECH_WIFI) {
    isWifiCall = true;
  }

  if (isInCall && radioTech == nsITelephonyCallInfo::RADIO_TECH_PS) {
    isVolteCall = true;
  }

  imsService->UpdateInCallType(isVolteCall, isWifiCall);

  return NS_OK;
}

NS_IMETHODIMP
TelephonyListener::CallStateChanged(uint32_t aLength,
                                    nsITelephonyCallInfo** aAllInfo)
{
  LOG("TelephonyListener CallStateChanged aLength:%d", aLength);

  for (uint32_t i = 0; i < aLength; ++i) {
    HandleCallInfo(aAllInfo[i]);
  }

  return NS_OK;
}

NS_IMETHODIMP
TelephonyListener::EnumerateCallState(nsITelephonyCallInfo* aInfo)
{
  return HandleCallInfo(aInfo);
}

NS_IMETHODIMP
TelephonyListener::EnumerateCallStateComplete()
{
  return NS_OK;
}

NS_IMETHODIMP
TelephonyListener::SupplementaryServiceNotification(uint32_t aServiceId,
                                                    int32_t aCallIndex,
                                                    uint16_t aNotification)
{
  return NS_OK;
}

NS_IMETHODIMP
TelephonyListener::NotifyConferenceError(const nsAString& aName,
                                         const nsAString& aMessage)
{
  return NS_OK;
}

NS_IMETHODIMP
TelephonyListener::NotifyCdmaCallWaiting(uint32_t aServiceId,
                                         const nsAString& aNumber,
                                         uint16_t aNumberPresentation,
                                         const nsAString& aName,
                                         uint16_t aNamePresentation)
{
  return NS_OK;
}

NS_IMETHODIMP
TelephonyListener::NotifyRingbackTone(bool playRingbackTone)
{
  return NS_OK;
}

NS_IMETHODIMP
TelephonyListener::NotifyTtyModeReceived(uint16_t mode)
{
  return NS_OK;
}

NS_IMETHODIMP
TelephonyListener::NotifyTelephonyCoverageLosing(uint16_t aType)
{
  return NS_OK;
}

bool
TelephonyListener::Listen(bool aStart)
{
  nsCOMPtr<nsITelephonyService> service =
                                    do_GetService(TELEPHONY_SERVICE_CONTRACTID);

  NS_ENSURE_TRUE(service, false);

  nsresult rv;
  if (aStart) {
    rv = service->RegisterListener(this);
  } else {
    rv = service->UnregisterListener(this);
  }

  return NS_SUCCEEDED(rv);
}

class ImsServiceHandler::ImsRegisterListener final : public nsIImsRegListener
{
public:
  NS_DECL_ISUPPORTS

  NS_IMETHOD
  NotifyEnabledStateChanged(bool aEnabled)
  {
    LOG("ImsRegisterListener NotifyEnabledStateChanged aEnabled:%d", aEnabled);
    return NS_OK;
  }

  NS_IMETHOD
  NotifyPreferredProfileChanged(uint16_t aProfile)
  {
    LOG("ImsRegisterListener NotifyPreferredProfileChanged aProfile:%d", aProfile);
    return NS_OK;
  }

  NS_IMETHOD
  NotifyCapabilityChanged(int16_t aCapability,
                          const nsAString& aUnregisteredReason)
  {
    LOG("ImsRegisterListener NotifyCapabilityChanged aCapability:%d", aCapability);
    LOG("ImsRegisterListener NotifyCapabilityChanged aUnregisteredReason:%s",
      NS_LossyConvertUTF16toASCII(aUnregisteredReason).get());
    return NS_OK;
  }

protected:
  ~ImsRegisterListener() { }
};

NS_IMPL_ISUPPORTS(ImsServiceHandler::ImsRegisterListener, nsIImsRegListener)

class ImsServiceHandler::WorkerMessageCallback final : public nsIRilSendWorkerMessageCallback
{
public:
  NS_DECL_ISUPPORTS

  NS_IMETHOD
  HandleResponse(JS::HandleValue response, bool *_retval)
  {
    LOG("WorkerMessageCallback HandleResponse...");
    if (!response.isObject()) {
      MOZ_ASSERT(false, "Not a object.");
      return NS_ERROR_INVALID_ARG;
    }

    mozilla::AutoSafeJSContext cx;
    JS::Rooted<JSObject*> obj(cx, &response.toObject());
    JS::Rooted<JS::Value> v(cx);

    nsAutoJSString rilMessageType;
    if (!JS_GetProperty(cx, obj, "rilMessageType", &v) || !v.isString()) {
      LOG("WorkerMessageCallback HandleResponse rilMessageType is null");
    }

    if (!rilMessageType.init(cx, v)) {
      LOG("Couldn't initialize rilMessageType from response.");
    }

    LOG("WorkerMessageCallback, HandleResponse rilMessageType:%s",
      NS_LossyConvertUTF16toASCII(rilMessageType).get());

    nsAutoJSString errorMsg;
    if (!JS_GetProperty(cx, obj, "errorMsg", &v) || !v.isString()) {
      LOG("WorkerMessageCallback HandleResponse errorMsg is null");
    }

    if (!errorMsg.init(cx, v)) {
      LOG("Couldn't initialize errorMsg from response.");
    }

    LOG("WorkerMessageCallback, HandleResponse errorMsg:%s",
      NS_LossyConvertUTF16toASCII(errorMsg).get());

    RefPtr<ImsService> imsService = ImsService::GetInstance();

    if (!errorMsg.IsEmpty()) {

      if (rilMessageType.Equals(NS_ConvertASCIItoUTF16("requestImsHandover"))) {

        imsService->NotifyImsHandoverStateChange(true,
                                   nsIImsService::IMS_HANDOVER_ACTION_CONFIRMED);

      } else if (rilMessageType.Equals(NS_ConvertASCIItoUTF16("getImsPcscfAddress"))) {

        nsAutoJSString address;
        if (!JS_GetProperty(cx, obj, "addr", &v) || !v.isString()) {
          LOG("WorkerMessageCallback HandleResponse errorMsg is null");
        }

        if (!address.init(cx, v)) {
          LOG("Couldn't initialize errorMsg from response.");
        }

        LOG("WorkerMessageCallback UpdateImsPcscfAddress.");
        imsService->mImsHandlers[PRIMARY_SERVICE_ID]
                                               ->UpdateImsPcscfAddress(address);
      }
    } else {

      if (rilMessageType.Equals(NS_ConvertASCIItoUTF16("requestImsHandover"))) {
        imsService->NotifyImsHandoverStateChange(false,
                                  nsIImsService::IMS_HANDOVER_ACTION_CONFIRMED);

      } else if (rilMessageType.Equals(NS_ConvertASCIItoUTF16("getImsPcscfAddress"))) {
        imsService->mImsHandlers[PRIMARY_SERVICE_ID]
                            ->UpdateImsPcscfAddress(NS_ConvertASCIItoUTF16(""));
      }
    }

    return NS_OK;
  }

protected:
  ~WorkerMessageCallback() { }
};

NS_IMPL_ISUPPORTS(ImsServiceHandler::WorkerMessageCallback,
                  nsIRilSendWorkerMessageCallback)

NS_IMPL_ISUPPORTS(ImsServiceHandler, nsIImsServiceHandler)

ImsServiceHandler::ImsServiceHandler(uint32_t aServiceId)
: mServiceId(aServiceId)
, mImsRegAddress(NS_ConvertASCIItoUTF16(""))
, mImsPscfAddress(NS_ConvertASCIItoUTF16(""))
, mImsRegisted(false)
, mImsServiceState(new ImsServiceState(false, IMS_REG_STATE_INACTIVE, -1))

{
  LOG("ImsServiceHandler constructor...");

  mImsRegListener = new ImsRegisterListener();

  nsCOMPtr<nsIRadioInterfaceLayer> radioService =
                              do_GetService(NS_RADIOINTERFACELAYER_CONTRACTID);

  if (!radioService) {
    NS_WARNING("Could not acquire nsIRadioInterfaceLayer!");
    return;
  }

  nsresult rv = radioService->GetRadioInterface(aServiceId,
                                               getter_AddRefs(mRadioInterface));

  if (NS_FAILED(rv) || !mRadioInterface) {
    NS_WARNING("Could not acquire nsIRadioInterface!");
    return;
  }

  nsCOMPtr<nsIImsRegService> imsRegService =
                                       do_GetService(IMS_REG_SERVICE_CONTRACTID);

  rv = imsRegService->GetHandlerByServiceId(aServiceId,
                                                getter_AddRefs(mImsRegHandler));

  if (NS_FAILED(rv) || !mImsRegHandler) {
    NS_WARNING("Could not acquire nsIImsRegHandler!");
    return;
  }

  mImsRegHandler->RegisterListener(mImsRegListener);

  mTelephonyListener = new TelephonyListener();
  mTelephonyListener->Listen(true);
}

/**
 * nsIImsServiceHandler interface.
 */
NS_IMETHODIMP
ImsServiceHandler::IsWifiCallingSupported(const nsAString& type,
                                          JS::HandleValue aMessage,
                                          nsIRilSendWorkerMessageCallback *callback,
                                          bool* isSupported)
{
  LOG("IsWifiCallingSupported is called type:%s",
                                      NS_LossyConvertUTF16toASCII(type).get());

  char* charType = ToNewUTF8String(type);
  uint32_t requestType = VOWIFI_REQUEST_TYPE_NULL;

  bool isVowifiSupportedRequest = false;
  bool isVowifiSupported = false;

  for (uint32_t i = 0; i < 15; ++i) {
    if (strcmp(charType, requestList[i]) == 0) {
      isVowifiSupportedRequest = true;
      requestType = i;
      break;
    }
  }

  RefPtr<ImsService> imsService = ImsService::GetInstance();
  int16_t currentImsFeature;
  int16_t inCallHandoverFeature;
  imsService->GetCurrentImsFeature(&currentImsFeature);
  imsService->GetInCallHandoverFeature(&inCallHandoverFeature);

  if ((imsService->IsWifiRegistered()
      && currentImsFeature == nsIImsService::FEATURE_TYPE_VOICE_OVER_WIFI)
    || inCallHandoverFeature == nsIImsService::FEATURE_TYPE_VOICE_OVER_LTE) {

    isVowifiSupported = true;
  }

  if (isVowifiSupportedRequest && isVowifiSupported) {
    switch (requestType) {

      case VOWIFI_REQUEST_TYPE_DIAL:
        Dial(aMessage, callback);
        break;

      case VOWIFI_REQUEST_TYPE_GET_CURRENT_CALL:
        GetCurrentCalls(aMessage, callback);
        break;

      case VOWIFI_REQUEST_TYPE_HANGUP_CALL:
        HangUpCall(aMessage, callback);
        break;

      case VOWIFI_REQUEST_TYPE_HANGUP_FOREGROUND:
        HangUpForeground(aMessage, callback);
        break;

      case VOWIFI_REQUEST_TYPE_HANGUP_BACKGROUND:
        HangUpBackground(aMessage, callback);
        break;

      case VOWIFI_REQUEST_TYPE_SWITCH_ACTIVE_CALL:
        SwitchActiveCall(aMessage, callback);
        break;

      case VOWIFI_REQUEST_TYPE_UDUB:
        Udub(aMessage, callback);
        break;

      case VOWIFI_REQUEST_TYPE_ANSWER_CALL:
        AnswerCall(aMessage, callback);
        break;

      case VOWIFI_REQUEST_TYPE_CONFERENCE_CALL:
        ConferenceCall(aMessage, callback);
        break;

      case VOWIFI_REQUEST_TYPE_SEPARATE_CALL:
        SeparateCall(aMessage, callback);
        break;

      case VOWIFI_REQUEST_TYPE_SET_MUTE:
        SetMute(aMessage, callback);
        break;

      case VOWIFI_REQUEST_TYPE_SEND_SMS:
        SendSMS(aMessage, callback);
        break;

      case VOWIFI_REQUEST_TYPE_SET_SMSC_ADDRESS:
        SetSmscAddress(aMessage, callback);
        break;

      case VOWIFI_REQUEST_TYPE_GET_SMSC_ADDRESS:
        GetSmscAddress(aMessage, callback);
        break;

      case VOWIFI_REQUEST_TYPE_ACK_SMS:
        AckSMS(aMessage, callback);
        break;

      default:
        break;
    }
    *isSupported = true;

  } else {
    *isSupported = false;

  }

  return NS_OK;
}

/**
 * nsIImsServiceHandler interface.
 */
NS_IMETHODIMP
ImsServiceHandler::Dial(JS::HandleValue aMessage,
                        nsIRilSendWorkerMessageCallback *callback)
{
  LOG("Dial is called...");
  if (!aMessage.isObject()) {
    MOZ_ASSERT(false, "Not a object.");
    return NS_ERROR_INVALID_ARG;
  }

  mozilla::AutoSafeJSContext cx;
  JS::Rooted<JSObject*> obj(cx, &aMessage.toObject());
  JS::Rooted<JS::Value> v(cx);

  nsAutoJSString dialNumber;
  if (!JS_GetProperty(cx, obj, "number", &v) || !v.isString()) {
    LOG("Dial: the number in dial request is null");
  }

  if (!dialNumber.init(cx, v)) {
    LOG("Couldn't initialize number from the request.");
  }

  if (!JS_GetProperty(cx, obj, "isEmergency", &v) || !v.isBoolean()) {
    LOG("Dial: the isEmergency in dial request is null");
  }

  bool isEmergency = ToBoolean(v);
  nsString number = dialNumber;
  LOG("Dial is called, isEmergency:%d", isEmergency);
  LOG("Dial is called, number:%s", NS_LossyConvertUTF16toASCII(number).get());

  // Currently, it does not support emergency call over vowifi, and the emergency
  // call over volte when vowifi is registered also is not supported.So when the
  // emergency call is dialed, it will notify the not supporting error message to
  // terminate the call. We will support the ECC over volte when vowifi is registered
  //  in the future.

  if (isEmergency) {
    if (callback) {
      mozilla::AutoSafeJSContext context;
      JS::Rooted<JSObject*> object(context, &aMessage.toObject());

      JS::Rooted<JS::Value> errorMsg(context);
      JS::Rooted<JSString*> error(context, JS_NewStringCopyZ(cx, "Not supported"));

      errorMsg.setString(error);
      JS_SetProperty(context, object, "errorMsg", errorMsg);

      JS::Rooted<JS::Value> callMessage(context, JS::ObjectValue(*object));
      bool resultValue = true;
      bool *result = &resultValue;
      callback->HandleResponse(callMessage, result);
    }
    return NS_OK;
  }

  nsCOMPtr<nsIVowifiInterfaceLayer> wifiService
                              = do_GetService(VOWIFIINTERFACELAYER_CONTRACTID);

  if (wifiService && !isEmergency) {
    wifiService->Dial(number, isEmergency, callback);
  }

  return NS_OK;
}

/**
 * nsIImsServiceHandler interface.
 */
NS_IMETHODIMP
ImsServiceHandler::GetCurrentCalls(JS::HandleValue aMessage,
                                   nsIRilSendWorkerMessageCallback *callback)
{
  LOG("GetCurrentCalls is called...");
  nsCOMPtr<nsIVowifiInterfaceLayer> wifiService
                               = do_GetService(VOWIFIINTERFACELAYER_CONTRACTID);
  if (wifiService) {
    // wifiService->GetCurrentCalls(callback);
  }

  mozilla::AutoSafeJSContext cx;
  JS::Rooted<JSObject*> obj(cx);
  obj = JS_NewPlainObject(cx);

  JS::Rooted<JSObject*> objs(cx);
  objs = JS_NewPlainObject(cx);

  JS::Rooted<JS::Value> v(cx, JS::ObjectValue(*obj));
  JS_SetProperty(cx, objs, "calls", v);

  JS::Rooted<JS::Value> message(cx, JS::ObjectValue(*objs));
  bool resultValue = true;
  bool *result = &resultValue;
  callback->HandleResponse(message, result);
  return NS_OK;
}

/**
 * nsIImsServiceHandler interface.
 */
NS_IMETHODIMP
ImsServiceHandler::HangUpCall(JS::HandleValue aMessage,
                              nsIRilSendWorkerMessageCallback *callback)
{
  LOG("HangUpCall is called...");
  if (!aMessage.isObject()) {
    MOZ_ASSERT(false, "Not a object.");
    return NS_ERROR_INVALID_ARG;
  }

  mozilla::AutoSafeJSContext cx;
  JS::Rooted<JSObject*> obj(cx, &aMessage.toObject());
  JS::Rooted<JS::Value> v(cx);

  if (!JS_GetProperty(cx, obj, "callIndex", &v) || !v.isNumber()) {
    LOG("Dial: the number in dial request is null");
  }

  uint32_t callIndex = v.toNumber();

  nsCOMPtr<nsIVowifiInterfaceLayer> wifiService
                               = do_GetService(VOWIFIINTERFACELAYER_CONTRACTID);

  if (wifiService) {
    wifiService->HungUpCall(callIndex, callback);
  }

  return NS_OK;
}

/**
 * nsIImsServiceHandler interface.
 */
NS_IMETHODIMP
ImsServiceHandler::HangUpForeground(JS::HandleValue aMessage,
                                    nsIRilSendWorkerMessageCallback *callback)
{
  nsCOMPtr<nsIVowifiInterfaceLayer> wifiService
                              = do_GetService(VOWIFIINTERFACELAYER_CONTRACTID);

  if (wifiService) {
    wifiService->HangUpForeground(callback);
  }

  return NS_OK;
}

/**
 * nsIImsServiceHandler interface.
 */
NS_IMETHODIMP
ImsServiceHandler::HangUpBackground(JS::HandleValue aMessage,
                                    nsIRilSendWorkerMessageCallback *callback)
{
  nsCOMPtr<nsIVowifiInterfaceLayer> wifiService
                              = do_GetService(VOWIFIINTERFACELAYER_CONTRACTID);

  if (wifiService) {
    wifiService->HangUpBackground(callback);
  }

  return NS_OK;
}

/**
 * nsIImsServiceHandler interface.
 */
NS_IMETHODIMP
ImsServiceHandler::SwitchActiveCall(JS::HandleValue aMessage,
                                    nsIRilSendWorkerMessageCallback *callback)
{
  nsCOMPtr<nsIVowifiInterfaceLayer> wifiService
                              = do_GetService(VOWIFIINTERFACELAYER_CONTRACTID);

  if (wifiService) {
    wifiService->SwitchActiveCall(callback);
  }

  return NS_OK;
}

/**
 * nsIImsServiceHandler interface.
 */
NS_IMETHODIMP
ImsServiceHandler::Udub(JS::HandleValue aMessage,
                        nsIRilSendWorkerMessageCallback *callback)
{
  nsCOMPtr<nsIVowifiInterfaceLayer> wifiService
                               = do_GetService(VOWIFIINTERFACELAYER_CONTRACTID);

  if (wifiService) {
    wifiService->Reject(callback);
  }

  return NS_OK;
}

/**
 * nsIImsServiceHandler interface.
 */
NS_IMETHODIMP
ImsServiceHandler::AnswerCall(JS::HandleValue aMessage,
                              nsIRilSendWorkerMessageCallback *callback)
{
  nsCOMPtr<nsIVowifiInterfaceLayer> wifiService
                              = do_GetService(VOWIFIINTERFACELAYER_CONTRACTID);

  if (wifiService) {
    wifiService->AnswerCall(callback);
  }

  return NS_OK;
}

/**
 * nsIImsServiceHandler interface.
 */
NS_IMETHODIMP
ImsServiceHandler::ConferenceCall(JS::HandleValue aMessage,
                                  nsIRilSendWorkerMessageCallback *callback)
{
  nsCOMPtr<nsIVowifiInterfaceLayer> wifiService
                              = do_GetService(VOWIFIINTERFACELAYER_CONTRACTID);

  if (wifiService) {
    wifiService->ConferenceCall(callback);
  }

  return NS_OK;
}

/**
 * nsIImsServiceHandler interface.
 */
NS_IMETHODIMP
ImsServiceHandler::SeparateCall(JS::HandleValue aMessage,
                                nsIRilSendWorkerMessageCallback *callback)
{
  LOG("HangUpCall is called...");
  if (!aMessage.isObject()) {
    MOZ_ASSERT(false, "Not a object.");
    return NS_ERROR_INVALID_ARG;
  }

  mozilla::AutoSafeJSContext cx;
  JS::Rooted<JSObject*> obj(cx, &aMessage.toObject());
  JS::Rooted<JS::Value> v(cx);

  if (!JS_GetProperty(cx, obj, "callIndex", &v) || !v.isNumber()) {
    LOG("Dial: the number in dial request is null");
  }                                              
  uint32_t callIndex = v.toNumber();

  nsCOMPtr<nsIVowifiInterfaceLayer> wifiService
                              = do_GetService(VOWIFIINTERFACELAYER_CONTRACTID);

  if (wifiService) {
    wifiService->SeparateCall(callIndex, callback);
  }

  return NS_OK;
}

/**
 * nsIImsServiceHandler interface.
 */
NS_IMETHODIMP
ImsServiceHandler::SetMute(JS::HandleValue aMessage,
                           nsIRilSendWorkerMessageCallback *callback)
{
  nsCOMPtr<nsIVowifiInterfaceLayer> wifiService
                              = do_GetService(VOWIFIINTERFACELAYER_CONTRACTID);

  if (wifiService) {
    wifiService->SetMute(callback);
  }

  return NS_OK;
}

/**
 * nsIImsServiceHandler interface.
 */
NS_IMETHODIMP
ImsServiceHandler::SendSMS(JS::HandleValue aMessage,
                           nsIRilSendWorkerMessageCallback *callback)
{
  LOG("SendSMS is called...");

  nsCOMPtr<nsIVowifiInterfaceLayer> wifiService
                              = do_GetService(VOWIFIINTERFACELAYER_CONTRACTID);
  if (wifiService) {
    wifiService->SendSMS(aMessage, callback);
  }
 
  return NS_OK;
}


/**
 * nsIImsServiceHandler interface.
 */
NS_IMETHODIMP
ImsServiceHandler::SetSmscAddress(JS::HandleValue aMessage,
                                  nsIRilSendWorkerMessageCallback *callback)
{
  LOG("SetSmscAddress is called...");
  nsCOMPtr<nsIVowifiInterfaceLayer> wifiService
                              = do_GetService(VOWIFIINTERFACELAYER_CONTRACTID);
  if (wifiService) {
    wifiService->SetSmscAddress(aMessage, callback);
  }

  return NS_OK;
}


/**
 * nsIImsServiceHandler interface.
 */
NS_IMETHODIMP
ImsServiceHandler::GetSmscAddress(JS::HandleValue aMessage,
                                  nsIRilSendWorkerMessageCallback *callback)
{
  LOG("GetSmscAddress is called...");
  nsCOMPtr<nsIVowifiInterfaceLayer> wifiService
                              = do_GetService(VOWIFIINTERFACELAYER_CONTRACTID);

  if (wifiService) {
    wifiService->GetSmscAddress(aMessage, callback);
  }

  return NS_OK;
}


/**
 * nsIImsServiceHandler interface.
 */
NS_IMETHODIMP
ImsServiceHandler::AckSMS(JS::HandleValue aMessage,
                                  nsIRilSendWorkerMessageCallback *callback)
{
  LOG("AckSMS is called...");
  nsCOMPtr<nsIVowifiInterfaceLayer> wifiService
                              = do_GetService(VOWIFIINTERFACELAYER_CONTRACTID);

  if (wifiService) {
    wifiService->AckSms(aMessage, callback);
  }

  return NS_OK;
}



/**
 * nsIImsServiceHandler interface.
 */
NS_IMETHODIMP
ImsServiceHandler::NotifyImsRegStateChanged(int16_t regState)
{
  LOG("ImsServiceHandler NotifyImsRegStateChanged regState:%d", regState);

  mImsServiceState->mRegState = regState;
  mImsServiceState->mImsRegistered = (regState == IMS_REG_STATE_REGISTERED);

  RefPtr<ImsService> imsService = ImsService::GetInstance();
  imsService->OnRegisterStateChange();

  return NS_OK;
}

/**
 * nsIImsServiceHandler interface.
 */
NS_IMETHODIMP
ImsServiceHandler::SetIMSRegAddress(const nsAString & addr)
{
  LOG("SetIMSRegAddress addr:%s", NS_LossyConvertUTF16toASCII(addr).get());
  mImsRegAddress = addr;
  nsCOMPtr<nsIVowifiInterfaceLayer> wifiService
                              = do_GetService(VOWIFIINTERFACELAYER_CONTRACTID);

  if (wifiService) {
    LOG("calling wifiService->SetVolteUsedLocalAddr");
    wifiService->SetVolteUsedLocalAddr(mImsRegAddress);
  }

  return NS_OK;
}

/**
 * nsIImsServiceHandler interface.
 */
NS_IMETHODIMP
ImsServiceHandler::GetIMSRegAddress(nsAString & addr)
{
  addr = mImsRegAddress;
  return NS_OK;
}


/**
 * nsIImsServiceHandler interface.
 */
NS_IMETHODIMP
ImsServiceHandler::IsImsRegistered(bool* aRegisted)
{
  *aRegisted = mImsServiceState->mImsRegistered;
  return NS_OK;
}

/**
 * nsIImsServiceHandler interface.
 */
NS_IMETHODIMP
ImsServiceHandler::GetVolteRegisterState(int16_t* aRegState)
{
  *aRegState = mImsServiceState->mRegState;
  return NS_OK;
}

/**
 * nsIImsServiceHandler interface.
 */
NS_IMETHODIMP
ImsServiceHandler::GetSrvccState(int16_t* aSrvccState)
{
  if (mImsServiceState != NULL) {
    *aSrvccState = mImsServiceState->mSrvccState;
  }
  return NS_OK;
}

/**
 * nsIImsServiceHandler interface.
 */
NS_IMETHODIMP
ImsServiceHandler::SetSrvccState(int16_t aSrvccState)
{
  if (mImsServiceState != NULL) {
    mImsServiceState->mSrvccState = aSrvccState;
  }
  return NS_OK;
}

/**
 * nsIImsServiceHandler interface.
 */
NS_IMETHODIMP
ImsServiceHandler::UpdateImsPcscfAddress(const nsAString & addr)
{
  mImsPscfAddress = addr;
  return NS_OK;
}

/**
 * nsIImsServiceHandler interface.
 */
NS_IMETHODIMP
ImsServiceHandler::EnableIms(bool aEnabled)
{
  LOG("EnableIms aEnabled:%d", aEnabled);

  if (mImsRegHandler) {
    mImsRegHandler->SetEnabled(aEnabled, NULL);
  }

  return NS_OK;
}

void
ImsServiceHandler::NotifyImsPdnStateChanged(int16_t pdnState)
{
  LOG("ImsServiceHandler NotifyImsPdnStateChanged pdnState:%d", pdnState);
}

void
ImsServiceHandler::RequestImsHandover(int16_t type)
{
  LOG("ImsServiceHandler RequestImsHandover type:%d", type);

  mozilla::AutoSafeJSContext cx;
  nsString request = NS_ConvertASCIItoUTF16("requestImsHandover");

  JS::Rooted<JSObject*> requestObj(cx);
  requestObj = JS_NewPlainObject(cx);

  JS::Rooted<JS::Value> requestType(cx);
  requestType.setInt32(type);
  JS_SetProperty(cx, requestObj, "type", requestType);

  JS::Rooted<JS::Value> requestMessage(cx, JS::ObjectValue(*requestObj));

  RefPtr<WorkerMessageCallback> callback = new WorkerMessageCallback();
  mRadioInterface->SendWorkerMessage(request, requestMessage, callback);
}

void
ImsServiceHandler::NotifyImsCallEnd(int16_t type)
{
  LOG("ImsServiceHandler NotifyImsCallEnd type:%d", type);

  mozilla::AutoSafeJSContext cx;
  nsString request = NS_ConvertASCIItoUTF16("notifyImsCallEnd");

  JS::Rooted<JSObject*> requestObj(cx);
  requestObj = JS_NewPlainObject(cx);

  JS::Rooted<JS::Value> requestType(cx);
  requestType.setInt32(type);
  JS_SetProperty(cx, requestObj, "type", requestType);

  JS::Rooted<JS::Value> requestMessage(cx, JS::ObjectValue(*requestObj));

  mRadioInterface->SendWorkerMessage(request, requestMessage, NULL);
}

void
ImsServiceHandler::NotifyImsHandoverStatus(int16_t status)
{
  LOG("ImsServiceHandler NotifyImsHandoverStatus status:%d", status);

  mozilla::AutoSafeJSContext cx;
  nsString request = NS_ConvertASCIItoUTF16("notifyImsHandoverStatus");

  JS::Rooted<JSObject*> requestObj(cx);
  requestObj = JS_NewPlainObject(cx);

  JS::Rooted<JS::Value> requestType(cx);
  requestType.setInt32(status);
  JS_SetProperty(cx, requestObj, "status", requestType);

  JS::Rooted<JS::Value> requestMessage(cx, JS::ObjectValue(*requestObj));

  mRadioInterface->SendWorkerMessage(request, requestMessage, NULL);
}

void
ImsServiceHandler::NotifyDataRouter()
{
  LOG("NotifyDataRouter");
  mozilla::AutoSafeJSContext cx;
  nsString request = NS_ConvertASCIItoUTF16("notifyDataRouter");

  JS::Rooted<JSObject*> requestObj(cx);
  requestObj = JS_NewPlainObject(cx);
  JS::Rooted<JS::Value> requestMessage(cx, JS::ObjectValue(*requestObj));

  mRadioInterface->SendWorkerMessage(request, requestMessage, NULL);
}

void
ImsServiceHandler::NotifyImsNetworkInfo(int16_t type, const nsAString & addr)
{
  LOG("NotifyImsNetworkInfo type:%d, addr:%s",
    type, NS_LossyConvertUTF16toASCII(addr).get());

  mozilla::AutoSafeJSContext cx;
  nsString request = NS_ConvertASCIItoUTF16("notifyImsNetworkInfo");

  JS::Rooted<JSObject*> requestObj(cx);
  requestObj = JS_NewPlainObject(cx);

  JS::Rooted<JS::Value> requestType(cx);
  requestType.setInt32(type);
  JS_SetProperty(cx, requestObj, "type", requestType);

  JS::Rooted<JSString*> addrString(cx,
               JS_NewStringCopyZ(cx, NS_LossyConvertUTF16toASCII(addr).get()));

  JS::Rooted<JS::Value> requestAddr(cx);
  requestAddr.setString(addrString);
  JS_SetProperty(cx, requestObj, "addr", requestAddr);

  JS::Rooted<JS::Value> requestMessage(cx, JS::ObjectValue(*requestObj));

  mRadioInterface->SendWorkerMessage(request, requestMessage, NULL);
}

void
ImsServiceHandler::NotifyVoWifiEnable(bool aEnabled)
{
  mozilla::AutoSafeJSContext cx;
  nsString request = NS_ConvertASCIItoUTF16("notifyVoWifiEnable");

  JS::Rooted<JSObject*> requestObj(cx);
  requestObj = JS_NewPlainObject(cx);

  JS::Rooted<JS::Value> requestType(cx);
  requestType.setBoolean(aEnabled);
  JS_SetProperty(cx, requestObj, "enabled", requestType);

  JS::Rooted<JS::Value> requestMessage(cx, JS::ObjectValue(*requestObj));
  mRadioInterface->SendWorkerMessage(request, requestMessage, NULL);
}

void
ImsServiceHandler::UpdateImsFeatures(bool volteEnable, bool wifiEnable)
{
  LOG("UpdateImsFeatures volteEnable:%d, wifiEnable:%d",
      volteEnable, wifiEnable);

  nsCOMPtr<nsIGonkImsRegService> imsRegService =
                                  do_GetService(GONK_IMSREGSERVICE_CONTRACTID);

  if(volteEnable) {
    mEnabledFeatures[nsIImsService::FEATURE_TYPE_VOICE_OVER_LTE]
                                  = nsIImsService::FEATURE_TYPE_VOICE_OVER_LTE;

    imsRegService->NotifyCapabilityChanged(mServiceId,
                            nsIImsRegHandler::IMS_CAPABILITY_VOICE_OVER_CELLULAR,
                            NS_ConvertASCIItoUTF16(""));

    // TODO There is no VT switch for kaios now.
    if(false/*ImsManager.isVtEnabledByUser(mContext)*/) {
      mEnabledFeatures[nsIImsService::FEATURE_TYPE_VIDEO_OVER_LTE]
                                  = nsIImsService::FEATURE_TYPE_VIDEO_OVER_LTE;
    } else {
      mEnabledFeatures[nsIImsService::FEATURE_TYPE_VIDEO_OVER_LTE]
                                          = nsIImsService::FEATURE_TYPE_UNKNOWN;
    }
    mEnabledFeatures[nsIImsService::FEATURE_TYPE_UT_OVER_LTE]
                                      = nsIImsService::FEATURE_TYPE_UT_OVER_LTE;
  } else {
    mEnabledFeatures[nsIImsService::FEATURE_TYPE_VOICE_OVER_LTE]
                                          = nsIImsService::FEATURE_TYPE_UNKNOWN;
    mEnabledFeatures[nsIImsService::FEATURE_TYPE_VIDEO_OVER_LTE]
                                          = nsIImsService::FEATURE_TYPE_UNKNOWN;
    mEnabledFeatures[nsIImsService::FEATURE_TYPE_UT_OVER_LTE]
                                          = nsIImsService::FEATURE_TYPE_UNKNOWN;
  }

  if(wifiEnable) {
    mEnabledFeatures[nsIImsService::FEATURE_TYPE_VOICE_OVER_WIFI]
                                  = nsIImsService::FEATURE_TYPE_VOICE_OVER_WIFI;

    imsRegService->NotifyCapabilityChanged(mServiceId,
                                nsIImsRegHandler::IMS_CAPABILITY_VOICE_OVER_WIFI,
                                NS_ConvertASCIItoUTF16(""));

    // TODO There is no VT switch for kaios now.
    if(false /*ImsManager.isVtEnabledByUser(mContext)*/) {
      mEnabledFeatures[nsIImsService::FEATURE_TYPE_VIDEO_OVER_WIFI]
                                  = nsIImsService::FEATURE_TYPE_VIDEO_OVER_WIFI;
    } else {
      mEnabledFeatures[nsIImsService::FEATURE_TYPE_VIDEO_OVER_WIFI]
                                          = nsIImsService::FEATURE_TYPE_UNKNOWN;
    }
    mEnabledFeatures[nsIImsService::FEATURE_TYPE_UT_OVER_WIFI]
                                    = nsIImsService::FEATURE_TYPE_UT_OVER_WIFI;
  } else {
    mEnabledFeatures[nsIImsService::FEATURE_TYPE_VOICE_OVER_WIFI]
                                          = nsIImsService::FEATURE_TYPE_UNKNOWN;
    mEnabledFeatures[nsIImsService::FEATURE_TYPE_VIDEO_OVER_WIFI]
                                          = nsIImsService::FEATURE_TYPE_UNKNOWN;
    mEnabledFeatures[nsIImsService::FEATURE_TYPE_UT_OVER_WIFI]
                                          = nsIImsService::FEATURE_TYPE_UNKNOWN;
  }

  if (!wifiEnable && !volteEnable) {
    imsRegService->NotifyCapabilityChanged(mServiceId,
                                      nsIImsRegHandler::IMS_CAPABILITY_UNKNOWN,
                                      NS_ConvertASCIItoUTF16(""));
  }

  LOG("UpdateImsFeaturesmEnabledFeatures[nsIImsService::FEATURE_TYPE_VOICE_OVER_LTE]:%d",
      mEnabledFeatures[nsIImsService::FEATURE_TYPE_VOICE_OVER_LTE]);

  if(imsRegService != NULL) {
    // TODO there is some mismatch for capabilities definition between kaios
    // and android.
  }
}

void
ImsServiceHandler::SetImsPcscfAddress(const nsAString & addr)
{
  nsString pcscfAddress;
  nsCString utf8Str = NS_ConvertUTF16toUTF8(addr);

  if (utf8Str.Contains(':')) {
    pcscfAddress.Append(NS_ConvertASCIItoUTF16("2,\""));
    pcscfAddress.Append(addr);
    pcscfAddress.Append(NS_ConvertASCIItoUTF16("\""));
  } else {
    pcscfAddress.Append(NS_ConvertASCIItoUTF16("1,\""));
    pcscfAddress.Append(addr);
    pcscfAddress.Append(NS_ConvertASCIItoUTF16("\""));
  }

  mozilla::AutoSafeJSContext cx;
  nsString request = NS_ConvertASCIItoUTF16("setImsPcscfAddress");

  JS::Rooted<JSObject*> requestObj(cx);
  requestObj = JS_NewPlainObject(cx);

  JS::Rooted<JSString*> addrString(cx,
                JS_NewStringCopyZ(cx, NS_LossyConvertUTF16toASCII(pcscfAddress).get()));
  JS::Rooted<JS::Value> requestAddr(cx);
  requestAddr.setString(addrString);

  JS_SetProperty(cx, requestObj, "addr", requestAddr);

  JS::Rooted<JS::Value> requestMessage(cx, JS::ObjectValue(*requestObj));
  mRadioInterface->SendWorkerMessage(request, requestMessage, NULL);
}

void
ImsServiceHandler::NotifyImsRegister(bool aRegisted)
{
  LOG("NotifyImsRegister aRegisted:%d", aRegisted);
}

void
ImsServiceHandler::QueryImsPcscfAddress()
{
  LOG("QueryImsPcscfAddress");
  mozilla::AutoSafeJSContext cx;
  nsString request = NS_ConvertASCIItoUTF16("getImsPcscfAddress");

  JS::Rooted<JSObject*> requestObj(cx);
  requestObj = JS_NewPlainObject(cx);
  JS::Rooted<JS::Value> requestMessage(cx, JS::ObjectValue(*requestObj));

  RefPtr<WorkerMessageCallback> callback = new WorkerMessageCallback();
  mRadioInterface->SendWorkerMessage(request, requestMessage, callback);
}

void
ImsServiceHandler::GetImsPcscfAddress(nsAString & addr)
{
  addr = mImsPscfAddress;
}

void
ImsServiceHandler::NotifyWifiCalling(bool isWiFiCalling)
{
  mozilla::AutoSafeJSContext cx;
  nsString request = NS_ConvertASCIItoUTF16("notifyVoWifiCallStateChanged");

  JS::Rooted<JSObject*> requestObj(cx);
  requestObj = JS_NewPlainObject(cx);

  JS::Rooted<JS::Value> requestType(cx);
  requestType.setBoolean(isWiFiCalling);
  JS_SetProperty(cx, requestObj, "isWiFiCalling", requestType);

  JS::Rooted<JS::Value> requestMessage(cx, JS::ObjectValue(*requestObj));
  mRadioInterface->SendWorkerMessage(request, requestMessage, NULL);
}

void
ImsServiceHandler::NotifyHandoverCallInfo(const nsAString & callInfo)
{
  mozilla::AutoSafeJSContext cx;
  nsString request = NS_ConvertASCIItoUTF16("notifyHandoverCallInfo");

  JS::Rooted<JSObject*> requestObj(cx);
  requestObj = JS_NewPlainObject(cx);

  JS::Rooted<JSString*> callInfoString(cx,
      JS_NewStringCopyZ(cx, NS_LossyConvertUTF16toASCII(callInfo).get()));
  JS::Rooted<JS::Value> requestCallInfo(cx);
  requestCallInfo.setString(callInfoString);

  JS_SetProperty(cx, requestObj, "callInfo", requestCallInfo);

  JS::Rooted<JS::Value> requestMessage(cx, JS::ObjectValue(*requestObj));
  mRadioInterface->SendWorkerMessage(request, requestMessage, NULL);
}

ImsServiceHandler::~ImsServiceHandler()
{
  LOG("~ImsServiceHandler");
  mImsRegHandler->UnregisterListener(mImsRegListener);
  mImsRegHandler = nullptr;

  mImsRegListener = nullptr;
  mRadioInterface = nullptr;

  delete mImsServiceState;
  mImsServiceState = nullptr;

  mTelephonyListener->Listen(false);
  mTelephonyListener = nullptr;
}

} // namespace imsservice
} // namespace dom
} // namespace mozilla
