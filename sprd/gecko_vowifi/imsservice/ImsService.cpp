#include "ImsService.h"
#include "mozilla/ClearOnShutdown.h"
#include "nsJSUtils.h"
#include "mozilla/ModuleUtils.h"



#include "nsIIccInfo.h"
#include "nsIIccService.h"

#undef LOG
#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "ImsService" , ## args)

namespace mozilla {
namespace dom {
namespace imsservice {

class ImsService::VoWifiCallback final : public nsIVowifiCallback
{
public:
  NS_DECL_ISUPPORTS

  NS_IMETHOD
  OnAttachFinished(bool aSuccess, uint16_t aErrorCode)
  {
    LOG("VoWifiCallback OnAttachFinished aSuccess:%d, aErrorCode:%d",
      aSuccess, aErrorCode);

    RefPtr<ImsService> imsService = ImsService::GetInstance();

    if (aSuccess) {
      LOG("VoWifiCallback On attach success -> mIsCalling:%d, "
        "mPendingAttachVowifiSuccess:%d, mPendingVowifiHandoverVowifiSuccess:%d,"
        " mIsS2bStopped:%d, mAttachVowifiSuccess:%d",
        imsService->mIsCalling,
        imsService->mPendingAttachVowifiSuccess,
        imsService->mPendingVowifiHandoverVowifiSuccess,
        imsService->mIsS2bStopped,
        imsService->mAttachVowifiSuccess);

      if(imsService->mFeatureSwitchRequest != NULL) {
        imsService->NotifyCPVowifiAttachSucceed();
        LOG("OnAttachFinished NotifyCPVowifiAttachSucceed finished.");

        if(imsService->mFeatureSwitchRequest->mEventCode
                                                    == ACTION_START_HANDOVER) {
          if (imsService->mIsCalling) {
            imsService->mInCallHandoverFeature
                                  = nsIImsService::FEATURE_TYPE_VOICE_OVER_WIFI;
            imsService->UpdateImsFeature();

            if (imsService->mWifiService != NULL) {
              imsService->mWifiService->UpdateDataRouterState(CALL_VOWIFI);
            }

            imsService->mIsPendingRegisterVowifi = true;

            if (imsService->mImsServiceCallback != NULL) {
                LOG("Wifi attach success -> operationSuccessed -> "
                  "IMS_OPERATION_HANDOVER_TO_VOWIFI");

                imsService->mImsServiceCallback->OperationSuccessed(
                                  imsService->mFeatureSwitchRequest->mRequestId,
                                  IMS_OPERATION_HANDOVER_TO_VOWIFI);
            }
          } else {
            imsService->UpdateImsFeature();

            imsService->mCurrentImsFeature
                                  = nsIImsService::FEATURE_TYPE_VOICE_OVER_WIFI;

            if (imsService->mImsServiceCallback != NULL) {
              LOG("Wifi attach success -> operationSuccessed -> "
                  "IMS_OPERATION_HANDOVER_TO_VOWIFI");

              imsService->mImsServiceCallback->OperationSuccessed(
                                  imsService->mFeatureSwitchRequest->mRequestId,
                                  IMS_OPERATION_HANDOVER_TO_VOWIFI);
            }

            if (imsService->mWifiService != NULL
                          && !imsService->mPendingVowifiHandoverVowifiSuccess) {
              imsService->mWifiService->ImsRegister();
            }

            imsService->mPendingAttachVowifiSuccess = false;

            if (imsService->mWifiService != NULL) {
              imsService->mWifiService->UpdateDataRouterState(CALL_NONE);
            }
          }

            LOG("Wifi attach success ->mFeatureSwitchRequest.mEventCode:%d, "
              "mCurrentImsFeature:%d, mIsCalling:%d, mIsVowifiCall:%d, "
              "mIsVolteCall:%d, mWifiRegistered:%d, mVolteRegistered:%d",
                                  imsService->mFeatureSwitchRequest->mEventCode,
                                  imsService->mCurrentImsFeature,
                                  imsService->mIsCalling,
                                  imsService->mIsVowifiCall,
                                  imsService->mIsVolteCall,
                                  imsService->mWifiRegistered,
                                  imsService->mVolteRegistered);

        } else if (imsService->mFeatureSwitchRequest->mEventCode
                                                == ACTION_SWITCH_IMS_FEATURE) {
          LOG("OnAttachFinished ACTION_SWITCH_IMS_FEATURE entered.");

          if (imsService->mWifiService != NULL) {
            imsService->mWifiService->ImsRegister();
          }
        }
      }

      imsService->mIsAPImsPdnActived = true;
      imsService->mIsS2bStopped = false;
      imsService->mAttachVowifiSuccess = true;

    } else {
      LOG("Wifi attach failed-> mAttachVowifiSuccess:%d",
        imsService->mAttachVowifiSuccess);

      if (imsService->mImsServiceCallback != NULL) {
        if (imsService->mFeatureSwitchRequest != NULL) {
          char str[100];
          snprintf(str, 100, "%d",aErrorCode);

          imsService->mImsServiceCallback->OperationFailed(
                                  imsService->mFeatureSwitchRequest->mRequestId,
                                  NS_ConvertASCIItoUTF16(str),
                                  (imsService->mFeatureSwitchRequest->mEventCode
                                      == ACTION_SWITCH_IMS_FEATURE)
                                            ? IMS_OPERATION_SWITCH_TO_VOWIFI
                                            : IMS_OPERATION_HANDOVER_TO_VOWIFI);

          imsService->mImsHandlers[imsService->mPrimaryServiceId]->
                            NotifyImsHandoverStatus(IMS_HANDOVER_ATTACH_FAIL);

          if (imsService->mPendingAttachVowifiSuccess
                              && !imsService->mIsCalling
                              && imsService->mFeatureSwitchRequest->mEventCode
                                == ACTION_START_HANDOVER) {
            imsService->mPendingAttachVowifiSuccess = false;

            if (imsService->mWifiService != NULL) {
              imsService->mWifiService->UpdateDataRouterState(CALL_NONE);
            }
          }

          LOG("Wifi attach failed-> operationFailed, clear mFeatureSwitchRequest.");
          imsService->mIsPendingRegisterVowifi = false;

          delete imsService->mFeatureSwitchRequest;
          imsService->mFeatureSwitchRequest = NULL;

          if (aErrorCode == 53766 && !imsService->mIsCalling) {
              imsService->mImsHandlers[imsService->mPrimaryServiceId]->
                                  SetIMSRegAddress(NS_ConvertASCIItoUTF16(""));
          }
        }
      }

      imsService->mIsAPImsPdnActived = false;
      imsService->mAttachVowifiSuccess = false;
    }

    LOG("OnAttachFinished finished.");
    return NS_OK;
  }

  NS_IMETHOD
  OnAttachStopped(uint16_t aStoppedReason)
  {
    LOG("VoWifiCallback OnAttachStopped aStoppedReason:%d", aStoppedReason);

    RefPtr<ImsService> imsService = ImsService::GetInstance();

    imsService->mIsAPImsPdnActived = false;
    imsService->mAttachVowifiSuccess = false;

    return NS_OK;
  }

  NS_IMETHOD
  OnAttachStateChanged(uint16_t aState)
  {
    RefPtr<ImsService> imsService = ImsService::GetInstance();

    LOG("VoWifiCallback OnAttachStateChanged aState:%d", aState);
    LOG("Wifi attach state uptate-> mWifiRegistered:%d, mIsS2bStopped:%d",
      imsService->mWifiRegistered, imsService->mIsS2bStopped);

    if (aState != S2b_STATE_CONNECTED) {
        imsService->mAttachVowifiSuccess = false;
    }

    if(aState != S2b_STATE_IDLE) {
        imsService->mIsS2bStopped = false;
    }

    return NS_OK;
  }

  NS_IMETHOD
  OnDPDDisconnected()
  {
    LOG("VoWifiCallback OnDPDDisconnected.");

    RefPtr<ImsService> imsService = ImsService::GetInstance();

    if(imsService->mImsServiceCallback != NULL) {
      imsService->mImsServiceCallback->OnDPDDisconnected();
    }

    return NS_OK;
  }

  NS_IMETHOD
  OnRegisterStateChanged(uint16_t aState, uint16_t aStateCode)
  {
    LOG("VoWifiCallback OnRegisterStateChanged aState:%d, aStateCode:%d",
      aState, aStateCode);

    RefPtr<ImsService> imsService = ImsService::GetInstance();

    bool result = aState == STATE_CONNECTED;

    imsService->mWifiRegistered = result && imsService->mIsAPImsPdnActived;
    imsService->mIsLoggingIn = false;

    LOG("Wifi register result -> mWifiRegistered:%d, mIsLoggingIn:%d, mIsAPImsPdnActived:%d",
      imsService->mWifiRegistered, imsService->mIsLoggingIn, imsService->mIsAPImsPdnActived);

    imsService->UpdateImsFeature();

    if (imsService->mFeatureSwitchRequest != NULL) {
      if (result) {
        if (imsService->mImsServiceCallback != NULL) {
          LOG("Wifi register result -> operationSuccessed -> VoWifi register success");

          imsService->mImsServiceCallback->OperationSuccessed(
                                  imsService->mFeatureSwitchRequest->mRequestId,
                                  (imsService->mFeatureSwitchRequest->mEventCode
                                    == ACTION_SWITCH_IMS_FEATURE)
                                            ? IMS_OPERATION_SWITCH_TO_VOWIFI
                                            : IMS_OPERATION_HANDOVER_TO_VOWIFI);

          if (imsService->mImsHandlers[imsService->mPrimaryServiceId]) {
            imsService->mImsHandlers[imsService->mPrimaryServiceId]
                                ->NotifyImsHandoverStatus(IMS_HANDOVER_SUCCESS);
          }
        }

        if(imsService->mFeatureSwitchRequest->mEventCode
                                              == ACTION_SWITCH_IMS_FEATURE) {

          if(imsService->mImsHandlers[imsService->mPrimaryServiceId]
                && imsService->mWifiService) {

             nsString addr;
             imsService->mWifiService->GetCurLocalAddress(addr);

              imsService->mImsHandlers[imsService->mPrimaryServiceId]
                                                      ->SetIMSRegAddress(addr);
          }
        }
      } else if (imsService->mImsServiceCallback != NULL) {

        if (imsService->mFeatureSwitchRequest) {
          imsService->mImsServiceCallback->OperationFailed(
                                imsService->mFeatureSwitchRequest->mRequestId,
                                NS_ConvertASCIItoUTF16("VoWifi register failed"),
                                (imsService->mFeatureSwitchRequest->mEventCode
                                    == ACTION_SWITCH_IMS_FEATURE)
                                            ? IMS_OPERATION_SWITCH_TO_VOWIFI
                                            : IMS_OPERATION_HANDOVER_TO_VOWIFI);
        }

        if (imsService->mImsHandlers[imsService->mPrimaryServiceId]) {
          imsService->mImsHandlers[imsService->mPrimaryServiceId]
                          ->NotifyImsHandoverStatus(IMS_HANDOVER_REGISTER_FAIL);
        }

        // Toast.makeText(ImsService.this, R.string.vowifi_register_error,
        //         Toast.LENGTH_LONG).show();

        if(imsService->mFeatureSwitchRequest &&
                                  imsService->mFeatureSwitchRequest->mEventCode
                                                == ACTION_SWITCH_IMS_FEATURE) {

          if (imsService->mImsHandlers[imsService->mPrimaryServiceId]) {
            imsService->mImsHandlers[imsService->mPrimaryServiceId]
                                  ->SetIMSRegAddress(NS_ConvertASCIItoUTF16(""));
          }
        }

        imsService->mAttachVowifiSuccess = false;
      }

      if(imsService->mFeatureSwitchRequest &&
                              imsService->mFeatureSwitchRequest->mTargetType
                              == nsIImsService::FEATURE_TYPE_VOICE_OVER_WIFI) {

        delete imsService->mFeatureSwitchRequest;
        imsService->mFeatureSwitchRequest = NULL;
      }
    }

    return NS_OK;
  }

  NS_IMETHOD
  OnReregisterFinished(bool aIsSuccess, uint16_t aErrorCode)
  {
    LOG("VoWifiCallback OnReregisterFinished aIsSuccess:%d, aErrorCode:%d",
      aIsSuccess, aErrorCode);

    return NS_OK;
  }

  NS_IMETHOD
  OnResetFinished(uint16_t aResult, uint16_t aErrorCode)
  {
    LOG("VoWifiCallback OnResetFinished aResult:%d, aErrorCode:%d",
      aResult, aErrorCode);

    RefPtr<ImsService> imsService = ImsService::GetInstance();

    if(aResult == SUCCESS) {
        imsService->mWifiRegistered = false;
        imsService->UpdateImsFeature();
    }

    if(imsService->mReleaseVowifiRequest) {
      if (imsService->mImsServiceCallback) {
        int16_t actionType;

        if (imsService->mReleaseVowifiRequest->mEventCode
                                          == ACTION_RELEASE_WIFI_RESOURCE) {
          actionType = IMS_OPERATION_RELEASE_WIFI_RESOURCE;

        } else if (imsService->mReleaseVowifiRequest->mEventCode
                                      == ACTION_NOTIFY_VOWIFI_UNAVAILABLE) {
          actionType = IMS_OPERATION_SET_VOWIFI_UNAVAILABLE;

        } else {

          actionType = IMS_OPERATION_CANCEL_CURRENT_REQUEST;
        }

        if (actionType == IMS_OPERATION_SET_VOWIFI_UNAVAILABLE) {
          imsService->SetNotifyCallbackTimeout(aResult == SUCCESS);

        } else if (aResult == SUCCESS) {
          LOG("Wifi reset result -> operationSuccessed -> wifi release success");

          imsService->mImsServiceCallback->OperationSuccessed(
                              imsService->mReleaseVowifiRequest->mRequestId,
                              actionType);

          delete imsService->mReleaseVowifiRequest;
          imsService->mReleaseVowifiRequest = NULL;
        } else {
          LOG("Wifi reset result -> operationFailed -> wifi release fail ");

          char str[100];
          snprintf(str, 100, "%s%d", "wifi release fail:", aErrorCode);

          imsService->mImsServiceCallback->OperationFailed(
                              imsService->mReleaseVowifiRequest->mRequestId,
                              NS_ConvertASCIItoUTF16(str),
                              actionType);

          delete imsService->mReleaseVowifiRequest;
          imsService->mReleaseVowifiRequest = NULL;
        }
      }
    }

    return NS_OK;
  }

  NS_IMETHOD
  OnUpdateDRStateFinished()
  {
    RefPtr<ImsService> imsService = ImsService::GetInstance();

    LOG("VoWifiCallback OnUpdateDRStateFinished mCallEndType:%d",
      imsService->mCallEndType);

    imsService->NotifyCpCallEnd();

    LOG("Update data router finished -> mInCallHandoverFeature:%d, "
      "mPendingVowifiHandoverVowifiSuccess:%d, mPendingVolteHandoverVolteSuccess:%d",
      imsService->mInCallHandoverFeature,
      imsService->mPendingVowifiHandoverVowifiSuccess,
      imsService->mPendingVolteHandoverVolteSuccess);

    if (imsService->mFeatureSwitchRequest != NULL
                        && (imsService->mInCallHandoverFeature
                          != nsIImsService::FEATURE_TYPE_UNKNOWN
                          || imsService->mPendingVolteHandoverVolteSuccess
                          || imsService->mPendingVowifiHandoverVowifiSuccess)) {

      if (imsService->mIsVolteCall
          || imsService->mPendingVolteHandoverVolteSuccess) {

        if (imsService->mFeatureSwitchRequest->mEventCode == ACTION_START_HANDOVER
                              && imsService->mFeatureSwitchRequest->mTargetType
                                == nsIImsService::FEATURE_TYPE_VOICE_OVER_LTE
                              && imsService->mCurrentImsFeature
                                == nsIImsService::FEATURE_TYPE_VOICE_OVER_LTE) {

          if (imsService->mIsS2bStopped) {
              LOG("Update data router finished->notifyDataRouter");

            if (imsService->mImsHandlers[imsService->mPrimaryServiceId]){
              imsService->mImsHandlers[imsService->mPrimaryServiceId]
                                                          ->NotifyDataRouter();
            }

          } else {
              imsService->mPendingReregister = true;

          }

          LOG("Update data router finished -> mIsS2bStopped:%d, mPendingReregister:%d",
            imsService->mIsS2bStopped, imsService->mPendingReregister);

        } else if (imsService->mFeatureSwitchRequest->mEventCode
                              == ACTION_START_HANDOVER
                            && imsService->mFeatureSwitchRequest->mTargetType
                              == nsIImsService::FEATURE_TYPE_VOICE_OVER_WIFI
                            && imsService->mCurrentImsFeature
                              == nsIImsService::FEATURE_TYPE_VOICE_OVER_WIFI) {

          int16_t type = 6;
          nsString info = NS_ConvertASCIItoUTF16("IEEE-802.11;i-wlan-node-id=");
          nsString bssid = imsService->mConnectedWifiBSSID;

          info.Append(bssid);
          info.ReplaceSubstring(MOZ_UTF16(":"), MOZ_UTF16(""));

          LOG("Update data router finished->notifyImsNetworkInfo->type:%d, info:%s",
            type, NS_LossyConvertUTF16toASCII(info).get());

          if (imsService->mImsHandlers[imsService->mPrimaryServiceId]) {
            imsService->mImsHandlers[imsService->mPrimaryServiceId]
                                            ->NotifyImsNetworkInfo(type, info);
          }
        }

        if (imsService->mPendingVolteHandoverVolteSuccess) {

          if(imsService->mFeatureSwitchRequest != NULL) {

            delete imsService->mFeatureSwitchRequest;
            imsService->mFeatureSwitchRequest = NULL;

          }

          imsService->mPendingVolteHandoverVolteSuccess = false;
        }
      } else if (imsService->mIsVowifiCall
                          || imsService->mPendingVowifiHandoverVowifiSuccess) {

        if (imsService->mFeatureSwitchRequest->mEventCode == ACTION_START_HANDOVER
                            && imsService->mFeatureSwitchRequest->mTargetType
                                == nsIImsService::FEATURE_TYPE_VOICE_OVER_LTE
                            && imsService->mCurrentImsFeature
                                == nsIImsService::FEATURE_TYPE_VOICE_OVER_LTE) {

          LOG("Update data router finished->reRegister->type:%d, nfo:%s",
                  imsService->mNetworkType,
                  NS_LossyConvertUTF16toASCII(imsService->mNetworkInfo).get());

          if(imsService->mIsS2bStopped) {

            if (imsService->mWifiService != NULL) {
              imsService->mWifiService->ImsReregister(imsService->mNetworkType,
                                                      imsService->mNetworkInfo);
            }

          } else {
            imsService->mPendingReregister = true;
          }

          LOG("Update data router finished->mIsS2bStopped:%d, mPendingReregister:%d",
              imsService->mIsS2bStopped, imsService->mPendingReregister);

        } else if (imsService->mFeatureSwitchRequest->mEventCode
                          == ACTION_START_HANDOVER
                          && imsService->mFeatureSwitchRequest->mTargetType
                              == nsIImsService::FEATURE_TYPE_VOICE_OVER_WIFI
                          && imsService->mCurrentImsFeature
                              == nsIImsService::FEATURE_TYPE_VOICE_OVER_WIFI) {

          int16_t type = -1;//"-1" means "EN_MTC_ACC_NET_IEEE_802_11"
          nsString info;
          nsString bssid = imsService->mConnectedWifiBSSID;
          info.Append(bssid);
          info.ReplaceSubstring(MOZ_UTF16(":"), MOZ_UTF16(""));

          LOG("Update data router finished->reRegister->type:%d, info:%s",
              type, NS_LossyConvertUTF16toASCII(info).get());

          if (imsService->mWifiService != NULL) {
            imsService->mWifiService->ImsReregister(type, info);
          }
        }

        if (imsService->mPendingVowifiHandoverVowifiSuccess) {

          if(imsService->mFeatureSwitchRequest != NULL) {

            delete imsService->mFeatureSwitchRequest;
            imsService->mFeatureSwitchRequest = NULL;
          }

          imsService->mPendingVowifiHandoverVowifiSuccess = false;
        }
      }
    }
    return NS_OK;
  }

  NS_IMETHOD
  OnCallIncoming(uint32_t callId, uint16_t aType)
  {
    LOG("VoWifiCallback OnCallIncoming callId:%d, aType:%d",callId, aType);

    RefPtr<ImsService> imsService = ImsService::GetInstance();

    if (imsService->mCurrentImsFeature
                      == nsIImsService::FEATURE_TYPE_VOICE_OVER_WIFI
                  && !imsService->mIsVowifiCall  && !imsService->mIsVolteCall) {

      if (imsService->mWifiService != NULL) {
        imsService->mWifiService->UpdateDataRouterState(CALL_VOWIFI);
      }
    }

    imsService->UpdateInCallState(true);

    return NS_OK;
  }

  NS_IMETHOD
  OnAllCallsEnd()
  {
    LOG("VoWifiCallback OnAllCallsEnd");

    RefPtr<ImsService> imsService = ImsService::GetInstance();

    if (imsService->mImsServiceCallback != NULL) {

      imsService->mImsServiceCallback->ImsCallEnd(
                                  nsIImsService::FEATURE_TYPE_VOICE_OVER_WIFI);

      LOG("OnAllCallsEnd-> mIsVowifiCall:%d, mIsVolteCall:%d, "
          "mInCallHandoverFeature:%d, mIsPendingRegisterVolte:%d, "
          "mIsPendingRegisterVowifi:%d",
          imsService->mIsVowifiCall, imsService->mIsVolteCall,
          imsService->mInCallHandoverFeature, imsService->mIsPendingRegisterVolte,
          imsService->mIsPendingRegisterVowifi);

      if (imsService->mFeatureSwitchRequest != NULL) {

        if (imsService->mFeatureSwitchRequest->mEventCode
                                                    == ACTION_START_HANDOVER) {

          if (imsService->mImsHandlers[imsService->mPrimaryServiceId] != NULL) {
            int16_t voWiFICallCount = 0;

            if (imsService->mWifiService != NULL) {
              imsService->mWifiService->GetCallCount(&voWiFICallCount);
            }

            if (imsService->mIsVolteCall && voWiFICallCount == 0) {
              if (imsService->mCurrentImsFeature
                                      != nsIImsService::FEATURE_TYPE_UNKNOWN) {
                LOG("OnAllCallsEnd->mCurrentImsFeature:%d",
                    imsService->mCurrentImsFeature);

                imsService->UpdateInCallState(false);
              }

              imsService->mCallEndType = WIFI_CALL_END;

              if (imsService->mInCallHandoverFeature
                            != imsService->mFeatureSwitchRequest->mTargetType) {

                if (imsService->mFeatureSwitchRequest->mTargetType
                              == nsIImsService::FEATURE_TYPE_VOICE_OVER_WIFI) {

                  imsService->mPendingAttachVowifiSuccess = true;

                  if (imsService->mIsVowifiCall) {
                    imsService->mPendingVowifiHandoverVowifiSuccess = true;
                  }

                } else if (imsService->mFeatureSwitchRequest->mTargetType
                                == nsIImsService::FEATURE_TYPE_VOICE_OVER_LTE) {
                  imsService->mPendingActivePdnSuccess = true;
                }
              }

              LOG("OnAllCallsEnd->mPendingAttachVowifiSuccess:%d, "
                  "mPendingActivePdnSuccess:%d",
                  imsService->mPendingAttachVowifiSuccess,
                  imsService->mPendingActivePdnSuccess);

              if (imsService->mFeatureSwitchRequest->mTargetType
                              == nsIImsService::FEATURE_TYPE_VOICE_OVER_WIFI) {

                if (imsService->mIsVolteCall
                                      && imsService->mIsPendingRegisterVowifi) {

                  if (imsService->mWifiService != NULL) {
                    imsService->mWifiService->ImsRegister();
                  }
                }

                imsService->mIsPendingRegisterVowifi = false;
              }

              imsService->mInCallHandoverFeature
                                          = nsIImsService::FEATURE_TYPE_UNKNOWN;

              if (!imsService->mPendingAttachVowifiSuccess
                                    && !imsService->mPendingActivePdnSuccess ) {

                if (imsService->mWifiService != NULL) {
                  imsService->mWifiService->UpdateDataRouterState(CALL_NONE);
                }
              }

              if (imsService->mIsVowifiCall && imsService->mCurrentImsFeature
                                  == nsIImsService::FEATURE_TYPE_VOICE_OVER_WIFI
                                    && imsService->mFeatureSwitchRequest != NULL 
                                    && !imsService->mPendingAttachVowifiSuccess 
                                    && !imsService->mPendingActivePdnSuccess) {

                delete imsService->mFeatureSwitchRequest;
                imsService->mFeatureSwitchRequest = NULL;
              }
            }

          } else {
            LOG("OnAllCallsEnd->ImsServiceHandler is null");
          }
        }
      } else {
        if (imsService->mImsHandlers[imsService->mPrimaryServiceId] != NULL) {

          int16_t voWiFICallCount = 0;

          if (imsService->mWifiService != NULL) {
             imsService->mWifiService->GetCallCount(&voWiFICallCount);
          }

          if (imsService->mIsVolteCall && voWiFICallCount == 0) {
            LOG("OnAllCallsEnd->mCurrentImsFeature:%d",
                imsService->mCurrentImsFeature);

            imsService->UpdateInCallState(false);
            imsService->mCallEndType = WIFI_CALL_END;
            imsService->mInCallHandoverFeature = nsIImsService::FEATURE_TYPE_UNKNOWN;

            if (imsService->mWifiService != NULL) {
              imsService->mWifiService->UpdateDataRouterState(CALL_NONE);
            }
          }
        }
      }

      if (imsService->mImsHandlers[imsService->mPrimaryServiceId] != NULL) {
        int16_t voWiFICallCount = 0;

        if (imsService->mWifiService != NULL) {
          imsService->mWifiService->GetCallCount(&voWiFICallCount);
        }

        if (imsService->mIsVolteCall && voWiFICallCount == 0) {

          if (imsService->mIsVowifiCall) {
            imsService->mIsVowifiCall = false;

          } else if (imsService->mIsVolteCall) {
            imsService->mIsVolteCall = false;
          }
        }
      } else {
        LOG("OnAllCallsEnd->ImsServiceHandler is null");
      }
    }
    return NS_OK;
  }

  NS_IMETHOD
  OnMediaQualityChanged(bool aIsVideo, int16_t aLose, int16_t aJitter, int16_t aRtt)
  {
    LOG("VoWifiCallback OnMediaQualityChanged aIsVideo:%d", aIsVideo);
    LOG("VoWifiCallback OnMediaQualityChanged aLose:%d", aLose);
    LOG("VoWifiCallback OnMediaQualityChanged aJitter:%d", aJitter);
    LOG("VoWifiCallback OnMediaQualityChanged aRtt:%d", aRtt);

    RefPtr<ImsService> imsService = ImsService::GetInstance();

    if (imsService->mImsServiceCallback) {
      imsService->mImsServiceCallback->OnMediaQualityChanged(aIsVideo,
                                                             aLose,
                                                             aJitter,
                                                             aRtt);
    }

    return NS_OK;
  }

  NS_IMETHOD
  OnNoRtpReceived(bool aIsVideo)
  {
    LOG("VoWifiCallback OnNoRtpReceived aIsVideo:%d", aIsVideo);

    RefPtr<ImsService> imsService = ImsService::GetInstance();

    if (imsService->mImsServiceCallback) {
      imsService->mImsServiceCallback->OnNoRtpReceived(aIsVideo);
    }

    return NS_OK;
  }

  NS_IMETHOD
  OnRtpReceived(bool aIsVideo)
  {
    LOG("VoWifiCallback OnRtpReceived aIsVideo:%d", aIsVideo);

    RefPtr<ImsService> imsService = ImsService::GetInstance();

    if (imsService->mImsServiceCallback) {
      imsService->mImsServiceCallback->OnRtpReceived(aIsVideo);
    }

    return NS_OK;
  }

  NS_IMETHOD
  OnUnsolicitedUpdate(uint16_t aStateCode)
  {
    RefPtr<ImsService> imsService = ImsService::GetInstance();

    LOG("VoWifiCallback OnCallIncoming aStateCode:%d", aStateCode);
    LOG("Wifi unsol update -> mIsVowifiCall:%d, mIsVolteCall:%d, "
        "mIsS2bStopped:%d, mPendingReregister:%d",
        imsService->mIsVowifiCall,
        imsService->mIsVolteCall,
        imsService->mIsS2bStopped,
        imsService->mPendingReregister);

    if (aStateCode == SECURITY_STOP){
      imsService->mIsS2bStopped = true;
    }

    if (imsService->mFeatureSwitchRequest != NULL
                                && imsService->mFeatureSwitchRequest->mEventCode
                                == ACTION_START_HANDOVER
                                && (aStateCode == SECURITY_DPD_DISCONNECTED
                                    || aStateCode == SECURITY_REKEY_FAILED
                                    || aStateCode == SECURITY_STOP)) {

      if(imsService->mFeatureSwitchRequest->mTargetType
                                  == nsIImsService::FEATURE_TYPE_VOICE_OVER_LTE
                                && imsService->mCurrentImsFeature
                                  == nsIImsService::FEATURE_TYPE_VOICE_OVER_LTE
                                && aStateCode == SECURITY_STOP) {

        if (imsService->mIsS2bStopped && imsService->mPendingReregister) {

          if (imsService->mInCallHandoverFeature
                                      != nsIImsService::FEATURE_TYPE_UNKNOWN) {

            if (imsService->mIsVolteCall) {

              if (imsService->mImsHandlers[imsService->mPrimaryServiceId]) {
                imsService->mImsHandlers[imsService->mPrimaryServiceId]
                                                            ->NotifyDataRouter();
              }
            } else if (imsService->mIsVowifiCall) {

              if (imsService->mWifiService) {
                imsService->mWifiService->ImsReregister(imsService->mNetworkType,
                                                        imsService->mNetworkInfo);
              }
            }

            imsService->mPendingReregister = false;
          }
        }
      }
    }

    imsService->NotifyWiFiError(aStateCode);
    return NS_OK;
  }

protected:
  ~VoWifiCallback() { }
};

NS_IMPL_ISUPPORTS(ImsService::VoWifiCallback, nsIVowifiCallback)

class ImsService::WifiScanInfo final : public nsIWifiScanResultsReady
{

public:
  NS_DECL_ISUPPORTS\

  NS_IMETHOD
  Onready(uint32_t count, nsIWifiScanResult** results) {

    RefPtr<ImsService> imsService = ImsService::GetInstance();

    for (uint32_t i = 0 ; i < count ; i++) {
      nsString bssid;
      results[i]->GetBssid(bssid);

      bool connected;
      results[i]->GetConnected(&connected);

      if (connected) {
        imsService->mConnectedWifiBSSID = bssid;
      }

      LOG(" WifiScanInfo[%d]: bssid:%s, connected:%d",
        i, NS_LossyConvertUTF16toASCII(bssid).get(), connected);
    }

    LOG("ImsService WifiScanInfo mConnectedWifiBSSID:%s",
      NS_LossyConvertUTF16toASCII(imsService->mConnectedWifiBSSID).get());

    return NS_OK;
  }

  NS_IMETHOD
  Onfailure() {
    LOG("ImsService WifiScanInfo Onfailure");
    return NS_OK;
  }

protected:
  ~WifiScanInfo() { }
};

NS_IMPL_ISUPPORTS(ImsService::WifiScanInfo, nsIWifiScanResultsReady)

NS_IMPL_ISUPPORTS(ImsService, nsIImsService)

/* static */ StaticRefPtr<ImsService> ImsService::sSingleton;

/* static */ already_AddRefed<ImsService>

ImsService::GetInstance()
{
  if (!sSingleton) {
    sSingleton = new ImsService();
    ClearOnShutdown(&sSingleton);
  }

  RefPtr<ImsService> service = sSingleton.get();
  return service.forget();
}

ImsService::ImsService()
: mPrimaryServiceId(0)
, mRequestId(-1)
, mVoLTERegisted(false)
, mIsCalling(false)
, mPendingActivePdnSuccess(false)
, mPendingAttachVowifiSuccess(false)
, mPendingVowifiHandoverVowifiSuccess(false)
, mPendingVolteHandoverVolteSuccess(false)
, mPendingReregister(false)
, mIsS2bStopped(false)
, mAttachVowifiSuccess(false)
, mIsVowifiCall(false)
, mIsVolteCall(false)
, mWifiRegistered(false)
, mVolteRegistered(false)
, mIsAPImsPdnActived(false)
, mIsPendingRegisterVowifi(false)
, mIsPendingRegisterVolte(false)
, mIsLoggingIn(false)
, mIsCPImsPdnActived(false)
, mPendingCPSelfManagement(false)
, mInCallHandoverFeature(-1)
, mCurrentImsFeature(-1)
, mCallEndType(-1)
, mNetworkType(-1)
, mNetworkInfo(NS_ConvertASCIItoUTF16("Network info is null"))
, mAliveCallLose(-1)
, mAliveCallJitter(-1)
, mAliveCallRtt(-1)
, mSrvccCapbility(-1)
, mIsWifiCalling(false)
, mFeatureSwitchRequest(NULL)
, mReleaseVowifiRequest(NULL)
{
  LOG("ImsService constructor...");

  mRadioService = do_GetService(NS_RADIOINTERFACELAYER_CONTRACTID);

  if (!mRadioService) {
    NS_WARNING("Could not acquire nsIRadioInterfaceLayer!");
    return;
  }

  mRadioService->GetNumRadioInterfaces(&mNumRadioInterfaces);

  for (uint32_t i = 0; i < mNumRadioInterfaces; ++i)
  {
    LOG("ImsService constructor i:%d", i);
    mImsHandlers.AppendObject(new ImsServiceHandler(i));
  }

  mImsRegService = do_GetService(IMS_REG_SERVICE_CONTRACTID);
  nsresult rv = mRadioService->GetRadioInterface(0,
                                            getter_AddRefs(mRadioInterface));

  if (NS_FAILED(rv) || !mRadioInterface) {
    NS_WARNING("Could not acquire nsIRadioInterface!");
    return;
  }

  if (!mImsRegService) {
    NS_WARNING("Could not acquire nsIImsRegService!");
    return;
  }

  mWifiService = do_GetService(VOWIFIINTERFACELAYER_CONTRACTID);

  if (!mWifiService) {
    NS_WARNING("Could not acquire nsIVowifiInterfaceLayer!");
    return;
  }
  mVoWifiCallback = new VoWifiCallback();
  mWifiService->RegisterCallback(mVoWifiCallback);

  nsCOMPtr<nsIImsConnManInterfaceLayer> imsConnectionManager
                          = do_GetService(IMSCONNMANINTERFACELAYER_CONTRACTID);

  if (imsConnectionManager) {
    imsConnectionManager->Init();
  }

}

NS_IMETHODIMP
ImsService::GetHandlerByServiceId(uint32_t aServiceId,
                                  nsIImsServiceHandler* *aImsHandler)
{
  LOG("ImsService GetHandlerByServiceId aServiceId:%d", aServiceId);

  *aImsHandler = nullptr;
  *aImsHandler = static_cast<nsIImsServiceHandler*>(
                                    moz_xmalloc(sizeof(nsIImsServiceHandler*)));

  NS_ENSURE_TRUE(*aImsHandler, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*aImsHandler = mImsHandlers[aServiceId]);

  return NS_OK;
}

/* unsigned short switchImsFeature (in short type); */
NS_IMETHODIMP ImsService::SwitchImsFeature(int16_t type, int16_t *_retval)
{
  // If current ims register state is registed, and same as the switch to,
  // will do nothing.

  LOG("ImsService before acquire mConnectedWifiBSSID:%s",
    NS_LossyConvertUTF16toASCII(mConnectedWifiBSSID).get());

  acquireConnectedWifiBSSID();

  int16_t requestId;
  if ((mWifiRegistered && type == nsIImsService::FEATURE_TYPE_VOICE_OVER_WIFI)
                  || (mVoLTERegisted
                      && type == nsIImsService::FEATURE_TYPE_VOICE_OVER_LTE)) {
    // Do nothing, return -1.
    requestId = -1;
    LOG("SwitchImsFeature no need to switch the type:%d", type);

  } else {
    requestId = ImsService::GetReuestId();

    if(mFeatureSwitchRequest  != NULL) {

      if(mImsServiceCallback) {
          mImsServiceCallback
                  ->OperationFailed(requestId,
                      NS_ConvertASCIItoUTF16("Repetitive operation"),
                      (type == nsIImsService::FEATURE_TYPE_VOICE_OVER_LTE)
                        ? nsIImsServiceCallback::IMS_OPERATION_SWITCH_TO_VOLTE
                        : nsIImsServiceCallback::IMS_OPERATION_SWITCH_TO_VOWIFI);
      }

      LOG("ImsService SwitchImsFeature: mFeatureSwitchRequest is exist!");
    } else {
      ImsService::OnReceiveHandoverEvent(false,
                                         requestId/*requestId*/,
                                         type/*targetType*/);
    }
  }

  *_retval = requestId;
  return NS_OK;
}

/* unsigned short startHandover (in short targetType); */
NS_IMETHODIMP ImsService::StartHandover(int16_t targetType, int16_t *_retval)
{
  LOG("StartHandover->mIsPendingRegisterVowifi:%d, mIsPendingRegisterVolte:%d, "
    "mAttachVowifiSuccess:%d", mIsPendingRegisterVowifi ,
    mIsPendingRegisterVolte, mAttachVowifiSuccess);

  int16_t id = GetReuestId();
  acquireConnectedWifiBSSID();

  if (mIsPendingRegisterVowifi) {
    mIsPendingRegisterVowifi = false;

    delete mFeatureSwitchRequest;
    mFeatureSwitchRequest = NULL;

  } else if(mIsPendingRegisterVolte) {
    mIsPendingRegisterVolte = false;

    delete mFeatureSwitchRequest;
    mFeatureSwitchRequest = NULL;
  }

  if(mFeatureSwitchRequest != NULL) {

    if(mImsServiceCallback != NULL) {
      mImsServiceCallback->OperationFailed(
                      id,
                      NS_ConvertASCIItoUTF16("Already handle one request."),
                      (targetType == nsIImsService::FEATURE_TYPE_VOICE_OVER_LTE)
                          ? IMS_OPERATION_HANDOVER_TO_VOLTE
                          : IMS_OPERATION_HANDOVER_TO_VOWIFI);
    }
    LOG("StartHandover-> mFeatureSwitchRequest is exist!");
  } else {
    OnReceiveHandoverEvent(true, id, targetType);
  }

  *_retval = id;
  return NS_OK;
}

/* void notifyNetworkUnavailable (); */
NS_IMETHODIMP ImsService::NotifyNetworkUnavailable()
{
  return NS_OK;
}

/* short getCurrentImsFeature (); */
NS_IMETHODIMP ImsService::GetCurrentImsFeature(int16_t *_retval)
{
  *_retval = mCurrentImsFeature;
  return NS_OK;
}

/* short GetInCallHandoverFeature (); */
NS_IMETHODIMP ImsService::GetInCallHandoverFeature(int16_t *_retval)
{
  *_retval = mInCallHandoverFeature;
  return NS_OK;
}

/* void setImsServiceListener (in nsIImsServiceCallback listener); */
NS_IMETHODIMP ImsService::SetImsServiceListener(nsIImsServiceCallback *listener)
{
  mImsServiceCallback = listener;
  return NS_OK;
}

/* AString getImsRegAddress (); */
NS_IMETHODIMP ImsService::GetImsRegAddress(nsAString & _retval)
{
  if (mImsHandlers[mPrimaryServiceId]) {
    mImsHandlers[mPrimaryServiceId]->GetIMSRegAddress(_retval);

  } else {
    _retval = NS_ConvertASCIItoUTF16("");
    LOG("GetImsRegAddress mImsHandlers is null.");
  }

  return NS_OK;
}

/* unsigned short releaseVoWifiResource (); */
NS_IMETHODIMP ImsService::ReleaseVoWifiResource(int16_t *_retval)
{
  int16_t id = GetReuestId();

  if (mReleaseVowifiRequest != NULL) {

    if (mImsServiceCallback != NULL) {
        mImsServiceCallback->OperationFailed(
                            id,
                            NS_ConvertASCIItoUTF16("Already handle one request."),
                            IMS_OPERATION_RELEASE_WIFI_RESOURCE);
    }
    LOG("Release voWifi resource-> mReleaseVowifiRequest is exist!");

  } else {

    if (mWifiService != NULL) {
        mWifiService->ResetAll(DISCONNECTED, 0);
    }

    mReleaseVowifiRequest = new ImsServiceRequest(id,
                                  ACTION_RELEASE_WIFI_RESOURCE /* eventCode */,
                                  mPrimaryServiceId /* serviceId */,
                                  nsIImsService::FEATURE_TYPE_VOICE_OVER_LTE);
    LOG("Release voWifi resource-> wifi state: DISCONNECTED");
  }

  mAttachVowifiSuccess = false;

  *_retval = id;
  return NS_OK;
}

/* unsigned short setVoWifiUnavailable (in unsigned short wifiState, in boolean isOnlySendAT); */
NS_IMETHODIMP ImsService::SetVoWifiUnavailable(uint16_t wifiState,
                                               bool isOnlySendAT,
                                               int16_t *_retval)
{
  int16_t id = GetReuestId();

  LOG("Notify wovifi unavailable -> wifiState:%d, isOnlySendAT:%d, mIsCPImsPdnActived:%d",
      wifiState, isOnlySendAT, mIsCPImsPdnActived);

  if(mReleaseVowifiRequest != NULL) {
    if(mImsServiceCallback != NULL) {
        mImsServiceCallback->OperationFailed(
                            id,
                            NS_ConvertASCIItoUTF16("Already handle one request."),
                            IMS_OPERATION_SET_VOWIFI_UNAVAILABLE);
    }
    LOG("Notify wovifi unavailable-> mReleaseVowifiRequest is exist!");

  } else {
    mReleaseVowifiRequest = new ImsServiceRequest(id/*requestId*/,
                                  ACTION_NOTIFY_VOWIFI_UNAVAILABLE /*eventCode*/,
                                  mPrimaryServiceId /*serviceId*/,
                                  nsIImsService::FEATURE_TYPE_VOICE_OVER_LTE);

    if (mFeatureSwitchRequest != NULL && mFeatureSwitchRequest->mEventCode
                              == ACTION_START_HANDOVER && mIsCPImsPdnActived) {

      LOG("Notify wovifi unavailable -> operationSuccessed -> "
        "IMS_OPERATION_SET_VOWIFI_UNAVAILABLE : only notify CM");

      mImsServiceCallback->OperationSuccessed(mReleaseVowifiRequest->mRequestId,
                                              IMS_OPERATION_SET_VOWIFI_UNAVAILABLE);

      delete mReleaseVowifiRequest;
      mReleaseVowifiRequest = NULL;

    } else {

      if (!isOnlySendAT) {

        nsCOMPtr<nsIMobileConnectionService> service =
            do_GetService(NS_MOBILE_CONNECTION_SERVICE_CONTRACTID);

        nsCOMPtr<nsIMobileConnection> connection;
        int32_t radioState = 0;

        if (service) {
          service->GetItemByServiceId(mPrimaryServiceId,
                                      getter_AddRefs(connection));
          connection->GetRadioState(&radioState);
        } else {
          LOG("Can not get nsIMobileConnectionService \n");
        }

        int16_t delaySend = radioState
                  == nsIMobileConnection::MOBILE_RADIO_STATE_ENABLED ? 500 : 0;
        LOG("Notify wovifi unavailable -> delaySend:%d", delaySend);

        if(mFeatureSwitchRequest == NULL ||
                   mFeatureSwitchRequest->mEventCode != ACTION_START_HANDOVER) {

          if (mWifiService) {
            mWifiService->ResetAll(wifiState == 0 ? DISCONNECTED : CONNECTED, delaySend);
          }

        } else {

          if (mWifiService) {
            mWifiService->ResetAll(wifiState == 0 ? DISCONNECTED : CONNECTED, 0);
          }
        }

      } else {
        LOG("Notify wovifi unavailable -> operationSuccessed -> "
          "IMS_OPERATION_SET_VOWIFI_UNAVAILABLE");

        if (mImsServiceCallback) {
          mImsServiceCallback->OperationSuccessed(
                                          mReleaseVowifiRequest->mRequestId,
                                          IMS_OPERATION_SET_VOWIFI_UNAVAILABLE);
        }

        delete mReleaseVowifiRequest;
        mReleaseVowifiRequest = NULL;
      }
    }

    if (!mIsCalling) {

      if (mImsHandlers[mPrimaryServiceId] != NULL) {

        mImsHandlers[mPrimaryServiceId]->NotifyVoWifiEnable(false);
        mPendingCPSelfManagement = true;

        LOG("Notify wovifi unavailable-> notifyVoWifiUnavaliable. "
          "mPendingCPSelfManagement:%d", mPendingCPSelfManagement);
      }
    }

    if (mFeatureSwitchRequest != NULL) {
      delete mFeatureSwitchRequest;
      mFeatureSwitchRequest = NULL;
    }
  }

  LOG("Notify wovifi unavailable-> mPendingAttachVowifiSuccess:%d",
    mPendingAttachVowifiSuccess);

  if(mPendingAttachVowifiSuccess) {

    LOG("Notify wovifi unavailable->mPendingAttachVowifiSuccess is true->"
      "mCallEndType:%d, mIsCalling:%d", mCallEndType, mIsCalling);

    mPendingAttachVowifiSuccess = false;

    if(mCallEndType != -1 && !mIsCalling) {
      LOG("Notify wovifi unavailable-> mCallEndType:%d", mCallEndType);
      NotifyCpCallEnd();
    }
  }

  mAttachVowifiSuccess = false;

  *_retval = id;
  return NS_OK;
}

/* unsigned short cancelCurrentRequest (); */
NS_IMETHODIMP ImsService::CancelCurrentRequest(int16_t *_retval)
{
  int16_t id = GetReuestId();

  LOG("Cancel current request->, mAttachVowifiSuccess:%d", mAttachVowifiSuccess);

  if (mFeatureSwitchRequest) {
    LOG("CancelCurrentRequest->"
      "mFeatureSwitchRequest mRequestId:%d,"
      "mFeatureSwitchRequest mEventCode:%d,"
      "mFeatureSwitchRequest mServiceId:%d,"
      "mFeatureSwitchRequest mTargetType:%d",
      mFeatureSwitchRequest->mRequestId,
      mFeatureSwitchRequest->mEventCode,
      mFeatureSwitchRequest->mServiceId,
      mFeatureSwitchRequest->mTargetType);
  }

  if(mFeatureSwitchRequest != NULL && mFeatureSwitchRequest->mTargetType
                               == nsIImsService::FEATURE_TYPE_VOICE_OVER_WIFI) {

    if(mReleaseVowifiRequest != NULL) {

      if(mImsServiceCallback != NULL) {
        mImsServiceCallback->OperationFailed(
                            id,
                            NS_ConvertASCIItoUTF16("Already handle one request."),
                            IMS_OPERATION_CANCEL_CURRENT_REQUEST);
      }
      LOG("Cancel current request-> mReleaseVowifiRequest is exist!");

    } else {
      mReleaseVowifiRequest = new ImsServiceRequest(
                              id/*requestId*/,
                              IMS_OPERATION_CANCEL_CURRENT_REQUEST /*eventCode*/,
                              mPrimaryServiceId /*serviceId*/,
                              nsIImsService::FEATURE_TYPE_VOICE_OVER_LTE);
    }

    /* Do not attached now, same as wifi do not connected */
    if (mWifiService) {
      mWifiService->ResetAll(DISCONNECTED, 0);
    }

    if (mFeatureSwitchRequest->mEventCode == ACTION_SWITCH_IMS_FEATURE
                                                     && mAttachVowifiSuccess) {

      if(mImsHandlers[mPrimaryServiceId] != NULL) {
        mImsHandlers[mPrimaryServiceId]
                                 ->SetIMSRegAddress(NS_ConvertASCIItoUTF16(""));
      }
    }

    if(mImsHandlers[mPrimaryServiceId] != NULL) {
      mImsHandlers[mPrimaryServiceId]->NotifyVoWifiEnable(false);
      mPendingCPSelfManagement = true;
    }

    mAttachVowifiSuccess = false;

    delete mFeatureSwitchRequest;
    mFeatureSwitchRequest = NULL;

    if(mPendingAttachVowifiSuccess) {
      mPendingAttachVowifiSuccess = false;
      LOG("Cancel current request-> mPendingAttachVowifiSuccess is true!");

      if(mCallEndType != -1 && !mIsCalling) {
        LOG("Cancel current request-> mCallEndType:%d", mCallEndType);
        NotifyCpCallEnd();
      }
    }
  } else {
    if(mImsServiceCallback != NULL) {
        mImsServiceCallback->OperationFailed(
                                      id/*requestId*/,
                                      NS_ConvertASCIItoUTF16( "Invalid Request"),
                                      IMS_OPERATION_CANCEL_CURRENT_REQUEST);
    } else {
      LOG("Cancel current request -> mImsServiceCallback is null ");
    }
  }

  *_retval = id;
  return NS_OK;
}

/* void terminateCalls (in unsigned short wifiState); */
NS_IMETHODIMP ImsService::TerminateCalls(uint16_t wifiState)
{
  return NS_OK;
}

/* AString getCurPcscfAddress (); */
NS_IMETHODIMP ImsService::GetCurPcscfAddress(nsAString & _retval)
{
  nsString address = NS_ConvertASCIItoUTF16("");

  if(mWifiService != NULL) {
    mWifiService->GetCurPcscfAddress(address);
  }

  _retval = address;
  return NS_OK;
}

/* void setMonitorPeriodForNoData (in unsigned short millis); */
NS_IMETHODIMP ImsService::SetMonitorPeriodForNoData(uint16_t millis)
{
  return NS_OK;
}

/* void showVowifiNotification (); */
NS_IMETHODIMP ImsService::ShowVowifiNotification()
{
  return NS_OK;
}

/* AString getCurLocalAddress (); */
NS_IMETHODIMP ImsService::GetCurLocalAddress(nsAString & _retval)
{
  nsString address = NS_ConvertASCIItoUTF16("");

  if(mWifiService != NULL) {
    mWifiService->GetCurLocalAddress(address);
  }

  _retval = address;
  return NS_OK;
}

/* unsigned short getCurrentImsVideoState (); */
NS_IMETHODIMP ImsService::GetCurrentImsVideoState(int16_t *_retval)
{
  // It only support voice call currently
  *_retval = nsITelephonyCallInfo::STATE_AUDIO_ONLY;
  return NS_OK;
}

/* unsigned short getAliveCallLose (); */
NS_IMETHODIMP ImsService::GetAliveCallLose(int16_t *_retval)
{
  LOG("getAliveCallLose->mIsVowifiCall:%d, mIsVolteCall:%d", mIsVowifiCall,
    mIsVolteCall);

  int16_t aliveCallLose = -1;

  if (mIsVowifiCall) {

    if (mWifiService != NULL) {
      mWifiService->GetAliveCallLose(&aliveCallLose);
    } else {
      LOG("getAliveCallLose->WifiService is null");
    }

  } else if (mIsVolteCall) {
    aliveCallLose = mAliveCallLose;
  }

  *_retval = aliveCallLose;
  return NS_OK;
}

/* unsigned short getAliveCallJitter (); */
NS_IMETHODIMP ImsService::GetAliveCallJitter(int16_t *_retval)
{
  LOG("GetAliveCallJitter->mIsVowifiCall:%d, mIsVolteCall:%d", mIsVowifiCall,
    mIsVolteCall);

  int16_t aliveCallJitter = -1;

  if (mIsVowifiCall) {

    if (mWifiService != NULL) {
      mWifiService->GetAliveCallJitter(&aliveCallJitter);
    } else {
      LOG("GetAliveCallJitter->WifiService is null");
    }

  } else if (mIsVolteCall) {
    aliveCallJitter = mAliveCallJitter;
  }

  *_retval = aliveCallJitter;
  return NS_OK;
}

/* unsigned short getAliveCallRtt (); */
NS_IMETHODIMP ImsService::GetAliveCallRtt(int16_t *_retval)
{
  LOG("GetAliveCallRtt->mIsVowifiCall:%d, mIsVolteCall:%d", mIsVowifiCall,
    mIsVolteCall);

  int16_t aliveCallRtt = -1;

  if (mIsVowifiCall) {

    if (mWifiService != NULL) {
      mWifiService->GetAliveCallRtt(&aliveCallRtt);
    } else {
      LOG("GetAliveCallRtt->WifiService is null");
    }

  } else if (mIsVolteCall) {
    aliveCallRtt = mAliveCallRtt;
  }

  *_retval = aliveCallRtt;
  return NS_OK;
}

/* unsigned short getVolteRegisterState (); */
NS_IMETHODIMP ImsService::GetVolteRegisterState(int16_t *_retval)
{
  int16_t regState;

  if (mImsHandlers[mPrimaryServiceId] != NULL) {
    mImsHandlers[mPrimaryServiceId]->GetVolteRegisterState(&regState);

  } else {
    regState = -1;
    LOG("GetVolteRegisterState -> ImsServiceHandler is null.");
  }

  *_retval = regState;
  return NS_OK;
}

/* unsigned short getCallType (); */
NS_IMETHODIMP ImsService::GetCallType(int16_t *_retval)
{
  int16_t callType;

  if (mIsVolteCall) {
    callType = VOLTE_CALL;

  } else if (mIsVowifiCall) {
    callType = WIFI_CALL;

  } else {
    callType = NO_CALL;
  }

  *_retval = callType;
  return NS_OK;
}

/* void notifySrvccCallInfos (in AString srvccCallInfo); */
NS_IMETHODIMP ImsService::NotifySrvccCallInfos(const nsAString & srvccCallInfo)
{
  LOG("notifySrvccCallInfos -> srvccCallInfo:%s",
      NS_LossyConvertUTF16toASCII(srvccCallInfo).get());

  if (mImsHandlers[mPrimaryServiceId]) {
    mImsHandlers[mPrimaryServiceId]->NotifyHandoverCallInfo(srvccCallInfo);
  }

  return NS_OK;
}

/* AString getImsPcscfAddress (); */
NS_IMETHODIMP ImsService::GetImsPcscfAddress(nsAString & _retval)
{
  nsString addr = NS_ConvertASCIItoUTF16("");

  if (mImsHandlers[mPrimaryServiceId] != NULL) {
    mImsHandlers[mPrimaryServiceId]->GetImsPcscfAddress(addr);
  }

  _retval = addr;
  return NS_OK;
}

/* void setVowifiRegister (in unsigned short action); */
NS_IMETHODIMP ImsService::SetVowifiRegister(uint16_t action)
{
  if (mImsServiceCallback != NULL) {
    mImsServiceCallback->OnSetVowifiRegister(action);
  }

  return NS_OK;
}

/* void addImsPdnStateListener (in unsigned short slotId, in nsIImsPdnStateListener listener); */
NS_IMETHODIMP ImsService::AddImsPdnStateListener(uint16_t slotId, nsIImsPdnStateListener *listener)
{
  return NS_OK;
}

/* void removeImsPdnStateListener (in unsigned short slotId, in nsIImsPdnStateListener listener); */
NS_IMETHODIMP ImsService::RemoveImsPdnStateListener(uint16_t slotId, nsIImsPdnStateListener *listener)
{
  return NS_OK;
}

/* unsigned short getCLIRStatus (in unsigned short phoneId); */
NS_IMETHODIMP ImsService::GetCLIRStatus(uint16_t phoneId, uint16_t *_retval)
{
  return NS_OK;
}

/* unsigned short updateCLIRStatus (in unsigned short action); */
NS_IMETHODIMP ImsService::UpdateCLIRStatus(uint16_t action, uint16_t *_retval)
{
  return NS_OK;
}

NS_IMETHODIMP ImsService::NotifyImsHandoverStateChange(bool allow, int16_t state)
{
  LOG("ImsService NotifyImsHandoverStateChange allow:%d, state:%d", allow, state);

  if(mFeatureSwitchRequest == NULL) {
    LOG("NotifyImsHandoverStateChange->there is no handover request active!");
    return NS_OK;
  }

  if (!allow) {

    if (mImsServiceCallback != NULL) {

        if (mFeatureSwitchRequest->mTargetType ==
                                  nsIImsService::FEATURE_TYPE_VOICE_OVER_LTE) {

          mImsServiceCallback->OperationFailed(
                mFeatureSwitchRequest->mRequestId,
                NS_ConvertASCIItoUTF16("Do not allow."),
                (mFeatureSwitchRequest->mEventCode == ACTION_SWITCH_IMS_FEATURE)
                  ? IMS_OPERATION_SWITCH_TO_VOLTE
                  : IMS_OPERATION_HANDOVER_TO_VOLTE);

        } else {

          mImsServiceCallback->OperationFailed(
                mFeatureSwitchRequest->mRequestId,
                NS_ConvertASCIItoUTF16("Do not allow."),
                (mFeatureSwitchRequest->mEventCode == ACTION_SWITCH_IMS_FEATURE)
                  ? IMS_OPERATION_CP_REJECT_SWITCH_TO_VOWIFI
                  : IMS_OPERATION_CP_REJECT_HANDOVER_TO_VOWIFI);
        }

        delete mFeatureSwitchRequest;
        mFeatureSwitchRequest = NULL;

    } else {
      LOG("NotifyImsHandoverStateChange->mImsServiceCallback is null!");
    }

  } else if (mFeatureSwitchRequest->mEventCode == ACTION_SWITCH_IMS_FEATURE) {

    if (mFeatureSwitchRequest->mTargetType ==
                                  nsIImsService::FEATURE_TYPE_VOICE_OVER_LTE) {

        if (state == IMS_HANDOVER_PDN_BUILD_FAIL
                                      || state == IMS_HANDOVER_REGISTER_FAIL) {

            if (mImsServiceCallback != NULL) {
                mImsServiceCallback->OperationFailed(
                                    mFeatureSwitchRequest->mRequestId,
                                    NS_ConvertASCIItoUTF16("VOLTE pdn failed."),
                                    IMS_OPERATION_SWITCH_TO_VOLTE);
            }

          delete mFeatureSwitchRequest;
          mFeatureSwitchRequest = NULL;
        }

    } else if (mFeatureSwitchRequest->mTargetType ==
                                  nsIImsService::FEATURE_TYPE_VOICE_OVER_WIFI) {

      if (state == nsIImsService::IMS_HANDOVER_ACTION_CONFIRMED) {

        char value[PROPERTY_VALUE_MAX];
        property_get("persist.sys.s2b.enabled", value, "true");
        LOG("NotifyImsHandoverStateChange->persist.sys.s2b.enabled:%s", value);

        if (mWifiService) {
          if (strcmp(value, "true") == 0) {
            mWifiService->Attach();
          } else {
            mWifiService->ImsRegister();
          }
        }
      }
    }

  } else if (mFeatureSwitchRequest->mEventCode == ACTION_START_HANDOVER) {

    LOG("ImsService NotifyImsHandoverStateChange mFeatureSwitchRequest->mTargetType:%d", mFeatureSwitchRequest->mTargetType);
    if (mFeatureSwitchRequest->mTargetType ==
                                   nsIImsService::FEATURE_TYPE_VOICE_OVER_LTE) {

        if(state == IMS_HANDOVER_PDN_BUILD_FAIL ||
                                          state == IMS_HANDOVER_REGISTER_FAIL) {

          mImsServiceCallback->OperationFailed(
                                    mFeatureSwitchRequest->mRequestId,
                                    NS_ConvertASCIItoUTF16("VOLTE pdn failed."),
                                    IMS_OPERATION_HANDOVER_TO_VOLTE);

          delete mFeatureSwitchRequest;
          mFeatureSwitchRequest = NULL;

          if (!mIsCalling && mPendingActivePdnSuccess) {
            mPendingActivePdnSuccess = false;

            if (mWifiService) {
              mWifiService->UpdateDataRouterState(CALL_NONE);
            }

            LOG("NotifyImsHandoverStateChange->ACTION_START_HANDOVER fail, "
              "mPendingActivePdnSuccess is true!");
          }

        } else if (state == IMS_HANDOVER_SRVCC_FAILED) {
            mImsServiceCallback->OnSrvccFaild();
            LOG("NotifyImsHandoverStateChange->IMS_HANDOVER_SRVCC_FAILED.");
        }

    } else if (mFeatureSwitchRequest->mTargetType ==
                                 nsIImsService::FEATURE_TYPE_VOICE_OVER_WIFI) {

      char value[PROPERTY_VALUE_MAX];
      property_get("persist.sys.s2b.enabled", value, "true");
      LOG("NotifyImsHandoverStateChange->persist.sys.s2b.enabled:%s", value);

      if (state == nsIImsService::IMS_HANDOVER_ACTION_CONFIRMED
                                     && strcmp(value, "true") == 0 && mWifiService) {
        mWifiService->Attach();
      }
    }
  }
  return NS_OK;
}

void
ImsService::NotifyCPVowifiAttachSucceed()
{
  LOG("NotifyCPVowifiAttachSucceed.");

  if (mImsHandlers[mPrimaryServiceId]) {
    mImsHandlers[mPrimaryServiceId]
                        ->NotifyImsHandoverStatus(IMS_HANDOVER_ATTACH_SUCCESS);
  } else {
    LOG("NotifyCPVowifiAttachSucceed mImsHandler is null.");
  }
}

void
ImsService::UpdateImsFeature()
{
  bool volteRegistered = false;
  int16_t srvccState;

  if (mImsHandlers[mPrimaryServiceId] != NULL) {
    mImsHandlers[mPrimaryServiceId]->IsImsRegistered(&volteRegistered);
    mImsHandlers[mPrimaryServiceId]->GetSrvccState(&srvccState);
  }

  LOG("updateImsFeature->mPrimaryServiceId:%d, volteRegistered:%d",
    mPrimaryServiceId, volteRegistered);

  UpdateImsRegisterState();
  bool isImsRegistered = false;

  if (mInCallHandoverFeature != nsIImsService::FEATURE_TYPE_UNKNOWN) {

    if (mInCallHandoverFeature == nsIImsService::FEATURE_TYPE_VOICE_OVER_WIFI) {
                mCurrentImsFeature = nsIImsService::FEATURE_TYPE_VOICE_OVER_WIFI;

      isImsRegistered = true;

    } else if (mImsHandlers[mPrimaryServiceId] != NULL
                                          && srvccState == HANDOVER_COMPLETED) {

      mCurrentImsFeature = nsIImsService::FEATURE_TYPE_UNKNOWN;
      isImsRegistered = false;

    } else {
        mCurrentImsFeature = nsIImsService::FEATURE_TYPE_VOICE_OVER_LTE;
        isImsRegistered = true;

    }

  } else if (volteRegistered) {
    mCurrentImsFeature = nsIImsService::FEATURE_TYPE_VOICE_OVER_LTE;

    if (mImsHandlers[mPrimaryServiceId] != NULL) {
       isImsRegistered = true;
    }

  } else if (mWifiRegistered) {
    mCurrentImsFeature = nsIImsService::FEATURE_TYPE_VOICE_OVER_WIFI;

    if(mImsHandlers[mPrimaryServiceId] != NULL) {
      isImsRegistered = true;
    }

  } else {
      mCurrentImsFeature = nsIImsService::FEATURE_TYPE_UNKNOWN;
      isImsRegistered = false;
  }

  NotifyCMImsCapabilityChange(
            mCurrentImsFeature == nsIImsService::FEATURE_TYPE_VOICE_OVER_LTE,
            mCurrentImsFeature == nsIImsService::FEATURE_TYPE_VOICE_OVER_WIFI);

  if (mImsHandlers[mPrimaryServiceId] != NULL) {
    mImsHandlers[mPrimaryServiceId]->UpdateImsFeatures(
            mCurrentImsFeature == nsIImsService::FEATURE_TYPE_VOICE_OVER_LTE,
            mCurrentImsFeature == nsIImsService::FEATURE_TYPE_VOICE_OVER_WIFI);

    if (mCurrentImsFeature == nsIImsService::FEATURE_TYPE_VOICE_OVER_WIFI) {

      if (mWifiService != NULL) {
        nsString addr;
        mWifiService->GetCurPcscfAddress(addr);
        mImsHandlers[mPrimaryServiceId]->SetImsPcscfAddress(addr);
      }
    }

    mImsHandlers[mPrimaryServiceId]->NotifyImsRegister(isImsRegistered);
    UpdateImsRegisterState();
  }

  LOG("updateImsFeature->mWifiRegistered:%d, volteRegistered:%d, "
      "mCurrentImsFeature:%d, mInCallHandoverFeature:%d",
      mWifiRegistered, volteRegistered, mCurrentImsFeature,
      mInCallHandoverFeature);
}

NS_IMETHODIMP
ImsService::NotifyImsNetworkInfoChange(int16_t type, const nsAString & info)
{
  LOG("NotifyImsNetworkInfoChange -> type:%d, info:%s",
      type, NS_LossyConvertUTF16toASCII(info).get());

  if (mFeatureSwitchRequest != NULL) {
    LOG("NotifyImsNetworkInfoChange-> mCurrentImsFeature:%d,"
      "mFeatureSwitchRequest mRequestId:%d,"
      "mFeatureSwitchRequest mEventCode:%d,"
      "mFeatureSwitchRequest mServiceId:%d,"
      "mFeatureSwitchRequest mTargetType:%d",
      mCurrentImsFeature,
      mFeatureSwitchRequest->mRequestId,
      mFeatureSwitchRequest->mEventCode,
      mFeatureSwitchRequest->mServiceId,
      mFeatureSwitchRequest->mTargetType);

    mNetworkType = type;
    mNetworkInfo = info;

  } else {
    LOG("NotifyImsNetworkInfoChange->mFeatureSwitchRequest is null.");
  }

  return NS_OK;
}

NS_IMETHODIMP
ImsService::NotifyWifiParamEvent(int16_t lose, int16_t jitter, int16_t rtt,
                                 int16_t timeout)
{
  LOG("NotifyWifiParamEvent->isvideo:false, lose:%d, jitter:%d,rtt:%d, timeout:%d",
      lose, jitter, rtt, timeout);

  mAliveCallLose = lose;
  mAliveCallJitter = jitter;
  mAliveCallRtt = rtt;

  if (mImsServiceCallback != NULL) {

    mImsServiceCallback->OnMediaQualityChanged(false,
                                               mAliveCallLose,
                                               mAliveCallJitter,
                                               mAliveCallRtt);
    if (timeout == 1) {
      LOG("NotifyWifiParamEvent->onRtpReceived->isvideo:false");
      mImsServiceCallback->OnNoRtpReceived(false);
    }

  } else {
    LOG("NotifyWifiParamEvent->mImsServiceCallback is null");
  }

  return NS_OK;
}

NS_IMETHODIMP
ImsService::NotifyImsPdnStatusChange(int16_t state)
{
  LOG("ImsService NotifyImsPdnStatusChange state:%d", state);

  if (state == IMS_PDN_READY) {
    mIsAPImsPdnActived = false;
    mIsCPImsPdnActived = true;
    mPendingCPSelfManagement = false;

  } else {
    mIsCPImsPdnActived = false;

    if (!mIsAPImsPdnActived) {
      mWifiRegistered = false;
      UpdateImsFeature();
      LOG("NotifyImsPdnStatusChange leave UpdateImsFeature");
    }

    if (state == IMS_PDN_ACTIVE_FAILED) {

      if ((mPendingCPSelfManagement || mFeatureSwitchRequest == NULL)
                                                         && !mWifiRegistered) {

        if (mImsHandlers[mPrimaryServiceId] != NULL) {
           mImsHandlers[mPrimaryServiceId]
            ->SetIMSRegAddress(NS_ConvertASCIItoUTF16(""));
        }
      }

      mPendingCPSelfManagement = false;
    }
  }

  if (mFeatureSwitchRequest) {
    LOG("NotifyImsNetworkInfoChange-> mCurrentImsFeature:%d,"
      "mFeatureSwitchRequest mRequestId:%d,"
      "mFeatureSwitchRequest mEventCode:%d,"
      "mFeatureSwitchRequest mServiceId:%d,"
      "mFeatureSwitchRequest mTargetType:%d",
      mCurrentImsFeature,
      mFeatureSwitchRequest->mRequestId,
      mFeatureSwitchRequest->mEventCode,
      mFeatureSwitchRequest->mServiceId,
      mFeatureSwitchRequest->mTargetType);
  }

  LOG("NotifyImsPdnStatusChange-> mIsCalling:%d, mIsCPImsPdnActived:%d,"
    "mIsAPImsPdnActived:%d, mWifiRegistered:%d, mVolteRegistered:%d,"
    "mPendingCPSelfManagement:%d, mPendingActivePdnSuccess:%d",
      mIsCalling, mIsCPImsPdnActived, mIsAPImsPdnActived, mWifiRegistered,
      mVolteRegistered, mPendingCPSelfManagement, mPendingActivePdnSuccess);

  if (mImsServiceCallback != NULL) {
    mImsServiceCallback->ImsPdnStateChange(state);
  }

  // If the switch request is null, we will start the volte call as default.
  if (mFeatureSwitchRequest == NULL && state == IMS_PDN_READY) {
    bool isImsRegistered;

    if (mImsHandlers[mPrimaryServiceId]) {
      mImsHandlers[mPrimaryServiceId]->IsImsRegistered(&isImsRegistered);
    }

    LOG("NotifyImsPdnStatusChange->mIsPendingRegisterVolte:%d, isImsRegistered:%d",
        mIsPendingRegisterVolte, isImsRegistered);

    // If pdn is ready when handover from vowifi to volte but volte is not
    // registered , never to turn on ims.
    // If Volte is registered , never to turn on ims.
    LOG("NotifyImsPdnStatusChange->mIsVolteCall:%d, mIsVowifiCall:%d",
        mIsVolteCall, mIsVowifiCall);

    if (!((mIsPendingRegisterVolte && !isImsRegistered) || isImsRegistered)
        && !mIsVolteCall && !mIsVowifiCall) {

      LOG("Switch request is null, but the pdn start, will enable the ims.");

      if (mImsHandlers[mPrimaryServiceId] != NULL) {
        mImsHandlers[mPrimaryServiceId]->QueryImsPcscfAddress();
        mImsHandlers[mPrimaryServiceId]->EnableIms(true);
      }
    }

  } else if (mFeatureSwitchRequest != NULL && mFeatureSwitchRequest->mTargetType
                                              == FEATURE_TYPE_VOICE_OVER_LTE) {

    if (state == IMS_PDN_ACTIVE_FAILED) {

      if (mImsServiceCallback != NULL) {
        LOG("NotifyImsPdnStatusChange -> operationFailed");

        mImsServiceCallback->OperationFailed(
                  mFeatureSwitchRequest->mRequestId,
                  NS_ConvertASCIItoUTF16("VOLTE pdn failed."),
                  (mFeatureSwitchRequest->mEventCode == ACTION_START_HANDOVER) ?
                          IMS_OPERATION_HANDOVER_TO_VOLTE:
                          IMS_OPERATION_SWITCH_TO_VOLTE);
      } else {
        LOG("NotifyImsPdnStatusChange->mImsServiceCallback is null!");
      }

      if (!mIsCalling && mFeatureSwitchRequest->mEventCode
                                                    == ACTION_START_HANDOVER) {
        mPendingActivePdnSuccess = false;

        if (mWifiService) {
          mWifiService->UpdateDataRouterState(CALL_NONE);
        }
      }

      delete mFeatureSwitchRequest;
      mFeatureSwitchRequest = NULL;

      mIsPendingRegisterVolte = false;

    } else if (state == IMS_PDN_READY) {
      if (mFeatureSwitchRequest->mEventCode == ACTION_SWITCH_IMS_FEATURE) {

        if (mImsHandlers[mPrimaryServiceId] != NULL) {
          // TODO Move the imsEnable from imsRegService to ImsService.
          // mImsHandlers[mPrimaryServiceId]->turnOnIms();
        }
        if (mImsServiceCallback != NULL) {
          LOG("NotifyImsPdnStatusChange -> operationSuccessed-> IMS_OPERATION_SWITCH_TO_VOLTE");

          mImsServiceCallback->OperationSuccessed(mFeatureSwitchRequest->mRequestId,
                                                  IMS_OPERATION_SWITCH_TO_VOLTE);
        }

      } else if(mFeatureSwitchRequest->mEventCode == ACTION_START_HANDOVER) {

        if (mIsCalling) {
          mInCallHandoverFeature = nsIImsService::FEATURE_TYPE_VOICE_OVER_LTE;
          mIsPendingRegisterVolte = true;
          UpdateImsFeature();

          if (mImsServiceCallback != NULL) {
            LOG("NotifyImsPdnStatusChange -> operationSuccessed-> IMS_OPERATION_HANDOVER_TO_VOLTE");

            mImsServiceCallback->OperationSuccessed(mFeatureSwitchRequest->mRequestId,
                                                    IMS_OPERATION_HANDOVER_TO_VOLTE);
          }

          if (mWifiService) {
            mWifiService->DeattachHandover(true);
            mWifiService->UpdateDataRouterState(CALL_VOLTE);
          }

          mWifiRegistered= false;
          UpdateImsFeature();
          mPendingActivePdnSuccess = false;

        } else {
          mIsPendingRegisterVolte = true;
          UpdateImsFeature();
          mCurrentImsFeature = nsIImsService::FEATURE_TYPE_VOICE_OVER_LTE;

          LOG("NotifyImsPdnStatusChange -> mCurrentImsFeature:%d, mIsCalling:%d",
            mCurrentImsFeature, mIsCalling);

          if (mImsServiceCallback != NULL) {
            LOG("NotifyImsPdnStatusChange -> operationSuccessed-> IMS_OPERATION_HANDOVER_TO_VOLTE");

            mImsServiceCallback->OperationSuccessed(mFeatureSwitchRequest->mRequestId,
                                                    IMS_OPERATION_HANDOVER_TO_VOLTE);
          }

          if (mWifiService) {
            mWifiService->DeattachHandover(true);
            mWifiService->UpdateDataRouterState(CALL_NONE);
          }

          mWifiRegistered= false;
          UpdateImsFeature();
          mPendingActivePdnSuccess = false;
        }
      }
    }
  }

  return NS_OK;
}


NS_IMETHODIMP
ImsService::NotifySrvccState(uint32_t aServiceId, int16_t srvccState)
{
  LOG("NotifySrvccState->srvccState:%d", srvccState);

  if (mImsHandlers[mPrimaryServiceId]) {
    mImsHandlers[mPrimaryServiceId]->SetSrvccState(srvccState);
  }

  UpdateImsRegisterState();

  if (aServiceId == mPrimaryServiceId) {
    UpdateImsFeature();

    if (mWifiService) {
      // mWifiService->OnSRVCCStateChanged(srvccState);
    }

    if(srvccState == HANDOVER_COMPLETED && mWifiRegistered) {
      mWifiRegistered = false;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
ImsService::NotifyCallEnded(uint32_t aServiceId, int16_t radioTech)
{
  LOG("NotifyCallEnded->aServiceId:%d, radioTech:%d", aServiceId, radioTech);

  if (radioTech == nsITelephonyCallInfo::RADIO_TECH_PS ||
    radioTech == nsITelephonyCallInfo::RADIO_TECH_WIFI ||
    radioTech == nsITelephonyCallInfo::RADIO_TECH_CS) {
    int16_t voWiFICallCount = 0;

    if (mWifiService) {
      mWifiService->GetCallCount(&voWiFICallCount);
    }

    if (voWiFICallCount == 0) {

      mCallEndType = VOLTE_CALL_END;
      LOG("NotifyCallEnded-> mIsVowifiCall:%d, mIsVolteCall:%d, mInCallHandoverFeature%d",
        mIsVowifiCall, mIsVolteCall, mInCallHandoverFeature);

      if(mFeatureSwitchRequest
                  && mFeatureSwitchRequest->mEventCode == ACTION_START_HANDOVER
                  && aServiceId == mFeatureSwitchRequest->mServiceId) {

        if (mInCallHandoverFeature != mFeatureSwitchRequest->mTargetType) {

          if (mFeatureSwitchRequest->mTargetType
                               == nsIImsService::FEATURE_TYPE_VOICE_OVER_WIFI) {
            mPendingAttachVowifiSuccess = true;

          } else if (mFeatureSwitchRequest->mTargetType
                               == nsIImsService::FEATURE_TYPE_VOICE_OVER_LTE) {
            mPendingActivePdnSuccess = true;

            if(mIsVolteCall){
                mPendingVolteHandoverVolteSuccess = true;
            }
          }
        }

        if(mFeatureSwitchRequest->mTargetType
                               == nsIImsService::FEATURE_TYPE_VOICE_OVER_WIFI) {

          if (mIsVolteCall && mIsPendingRegisterVowifi) {

            if (mWifiService) {
              mWifiService->ImsRegister();
            }

            mIsLoggingIn = true;
          }

          mIsPendingRegisterVowifi = false;
        }

        LOG("NotifyCallEnded-> mPendingAttachVowifiSuccess:%d, mPendingActivePdnSuccess:%d, mIsLoggingIn:%d",
          mPendingAttachVowifiSuccess, mPendingActivePdnSuccess, mIsLoggingIn);

        if (mIsVolteCall && mFeatureSwitchRequest->mTargetType
                                   == nsIImsService::FEATURE_TYPE_VOICE_OVER_LTE
                 && !mPendingAttachVowifiSuccess && !mPendingActivePdnSuccess) {

          if (mFeatureSwitchRequest) {
            delete mFeatureSwitchRequest;
            mFeatureSwitchRequest = NULL;
          }

          LOG("NotifyCallEnded-> This is volte call,so mFeatureSwitchRequest has been emptyed.");
        }
      }

      if(mCurrentImsFeature != nsIImsService::FEATURE_TYPE_UNKNOWN) {
        LOG("NotifyCallEnded->mCurrentImsFeature:%d", mCurrentImsFeature);
        UpdateInCallState(false);
      }

      mInCallHandoverFeature = nsIImsService::FEATURE_TYPE_UNKNOWN;

      if (!mPendingAttachVowifiSuccess && !mPendingActivePdnSuccess ) {

        if (mWifiService) {
          mWifiService->UpdateDataRouterState(CALL_NONE);
        }
      }

      if (mIsVowifiCall) {
          mIsVowifiCall = false;

      } else if (mIsVolteCall) {
          mIsVolteCall = false;
      }
    }
  }

  LOG("NotifyCallEnded->aServiceId:%d, mIsVolteCall:%d, mIsVowifiCall:%d, mInCallHandoverFeature:%d",
      aServiceId, mIsVolteCall, mIsVowifiCall, mInCallHandoverFeature);

  return NS_OK;
}

int16_t
ImsService::GetReuestId()
{
  mRequestId++;

  if (mRequestId > 100){
      mRequestId = 0;
  }

  return mRequestId;
}

void
ImsService::OnReceiveHandoverEvent(bool isCalling,
                                   int16_t requestId,
                                   int16_t targetType)
{
  LOG("ImsService OnReceiveHandoverEvent isCalling:%d, requestId:%d,targetType:%d",
    isCalling, requestId, targetType);

  if (!mIsCalling && mPendingActivePdnSuccess) {
    mPendingActivePdnSuccess =  false;

    if (mCallEndType != -1){
      LOG("ImsService onReceiveHandoverEvent mCallEndType:%d", mCallEndType);
      NotifyCpCallEnd();
    }
  }

  if (!mIsCalling && mPendingAttachVowifiSuccess) {
    mPendingAttachVowifiSuccess =  false;

    if(mCallEndType != -1) {
      LOG("ImsService onReceiveHandoverEvent mCallEndType:%d", mCallEndType);
      NotifyCpCallEnd();
    }
  }

  mFeatureSwitchRequest = new ImsServiceRequest(
    requestId,
    isCalling ? ACTION_START_HANDOVER : ACTION_SWITCH_IMS_FEATURE /*eventCode*/,
    mPrimaryServiceId/*serviceId*/,
    targetType);

  if (mImsHandlers[mPrimaryServiceId]) {

    if (mFeatureSwitchRequest->mTargetType ==
                                  nsIImsService::FEATURE_TYPE_VOICE_OVER_LTE) {

        mImsHandlers[mPrimaryServiceId]->RequestImsHandover(isCalling
                           ? INCALL_HANDOVER_TO_VOLTE : IDEL_HANDOVER_TO_VOLTE);

    } else if (mFeatureSwitchRequest->mTargetType ==
                                  nsIImsService::FEATURE_TYPE_VOICE_OVER_WIFI) {

        mImsHandlers[mPrimaryServiceId]->RequestImsHandover(isCalling
                          ? INCALL_HANDOVER_TO_VOWIFI : IDEL_HANDOVER_TO_VOWIFI);
    }
  }
}

void
ImsService::NotifyCpCallEnd()
{
  if (mCallEndType != -1 && !mIsCalling) {

    if (mImsHandlers[mPrimaryServiceId]) {
      mImsHandlers[mPrimaryServiceId]->NotifyImsCallEnd(mCallEndType);
      mCallEndType = -1;

    } else {
      LOG("notifyCpCallEnd->notifyImsCallEnd-> mImsHandlers is null");
    }
  }
}

void
ImsService::NotifyWiFiError(uint16_t aErrorCode)
{
  LOG("NotifyWiFiError-> aErrorCode:%d", aErrorCode);

  if (mImsServiceCallback) {
    mImsServiceCallback->OnVoWiFiError(aErrorCode);
  }
}

void
ImsService::acquireConnectedWifiBSSID()
{
  nsCOMPtr<nsIInterfaceRequestor> ir
      = do_GetService("@mozilla.org/telephony/system-worker-manager;1");
  nsCOMPtr<nsIWifi> wifi = do_GetInterface(ir);

  if (wifi) {
    nsCOMPtr<nsIWifiScanResultsReady> scanResult = new WifiScanInfo();
    wifi->GetWifiScanResults(scanResult);

  } else {
    LOG("ImsService wifi worker is null.");
  }
}

void
ImsService::UpdateImsRegisterState()
{
  if (mImsHandlers[mPrimaryServiceId] != NULL) {
    bool isImsRegistered = false;

    mImsHandlers[mPrimaryServiceId]->IsImsRegistered(&isImsRegistered);
    mVolteRegistered = isImsRegistered ? true : false;

  } else {
    LOG("UpdateImsRegisterState mImsHandlers is null.");
    mVolteRegistered = false;
  }
}

void
ImsService::OnRegisterStateChange()
{
  bool isImsRegistered = false;
  int16_t volteRegState;

  mImsHandlers[mPrimaryServiceId]->IsImsRegistered(&isImsRegistered);
  mImsHandlers[mPrimaryServiceId]->GetVolteRegisterState(&volteRegState);

  LOG("OnRegisterStateChange-> mIsCalling:%d, mVolteRegistered:%d, "
      "mImsHandlers->isImsRegistered():%d, mIsLoggingIn:%d, "
      "mIsPendingRegisterVolte:%d",
      mIsCalling, mVolteRegistered, isImsRegistered, mIsLoggingIn,
      mIsPendingRegisterVolte);

  if(volteRegState == IMS_REG_STATE_REGISTERING
                              || volteRegState == IMS_REG_STATE_DEREGISTERING) {

    LOG("OnRegisterStateChange-> pending status "
        "mImsHandlers.getVolteRegisterState():%d", volteRegState);
    return;
  }

  if(mFeatureSwitchRequest == NULL && mIsPendingRegisterVolte) {
      mIsPendingRegisterVolte = false;
  }

  //If CP reports CIREGU as 1,3 , IMS Feature will be updated as Volte registered state firstly.
  if (volteRegState == IMS_REG_STATE_REGISTERED
                                   || volteRegState == IMS_REG_STATE_REG_FAIL) {

      mVolteRegistered = (volteRegState == IMS_REG_STATE_REGISTERED);

    if (!mIsLoggingIn) {
      UpdateImsFeature();
    }

  } else {

    if (mVolteRegistered != isImsRegistered) {
      mVolteRegistered = isImsRegistered;

      if(!mIsLoggingIn) {
        UpdateImsFeature();
      }
    }
  }

  if(mFeatureSwitchRequest != NULL &&
                          mFeatureSwitchRequest->mServiceId == mPrimaryServiceId
                          && mFeatureSwitchRequest->mTargetType
                          == nsIImsService::FEATURE_TYPE_VOICE_OVER_LTE) {

    if (isImsRegistered) {

      if (mIsPendingRegisterVolte && mWifiService != NULL) {
        mWifiService->ResetAll(DISCONNECTED, 0);

      } else {

        if (mWifiService) {
          mWifiService->ImsDeregister();
          mWifiService->Deattach();
        }

        // Set wifi registered state as false when make de-register
        // operation in handover.
        mWifiRegistered= false;
        UpdateImsFeature();

      }

      mIsPendingRegisterVolte = false;

      if (mImsServiceCallback != NULL) {

        if (mFeatureSwitchRequest->mEventCode == ACTION_SWITCH_IMS_FEATURE) {
          LOG("OnRegisterStateChange -> operationSuccessed -> IMS_OPERATION_SWITCH_TO_VOLTE");

          mImsServiceCallback->OperationSuccessed(
                                              mFeatureSwitchRequest->mRequestId,
                                              IMS_OPERATION_SWITCH_TO_VOLTE);

          delete mFeatureSwitchRequest;
          mFeatureSwitchRequest = NULL;
        }

       } else {
          LOG("OnRegisterStateChange -> operationSuccessed, mImsServiceCallback is null!");
       }

    } else if(mImsServiceCallback != NULL) {

      if(mFeatureSwitchRequest->mEventCode == ACTION_SWITCH_IMS_FEATURE) {
        LOG("OnRegisterStateChange -> operationFailed -> IMS_OPERATION_SWITCH_TO_VOLTE");

        mImsServiceCallback->OperationFailed(
                                mFeatureSwitchRequest->mRequestId,
                                NS_ConvertASCIItoUTF16("VoLTE register failed"),
                                IMS_OPERATION_SWITCH_TO_VOLTE);

        delete mFeatureSwitchRequest;
        mFeatureSwitchRequest = NULL;
        mIsPendingRegisterVolte = false;
      }

    } else {
      LOG("OnRegisterStateChange -> operationFailed, mImsServiceCallback is null!");
    }

    LOG("OnRegisterStateChange-> mPendingActivePdnSuccess:%d, mIsCPImsPdnActived:%d",
        mPendingActivePdnSuccess, mIsCPImsPdnActived);

      if (!mPendingActivePdnSuccess && mIsCPImsPdnActived 
                && mFeatureSwitchRequest != NULL
                && mFeatureSwitchRequest->mEventCode == ACTION_START_HANDOVER) {

        LOG("OnRegisterStateChange -> ACTION_START_HANDOVER, clear mFeatureSwitchRequest");
        delete mFeatureSwitchRequest;
        mFeatureSwitchRequest = NULL;
        mIsPendingRegisterVolte = false;
     }
  }
}

void
ImsService::UpdateInCallState(bool isInCall)
{
  if (mIsCalling != isInCall) {
    mIsCalling = isInCall;

    if (mIsCalling &&
        (mCurrentImsFeature !=nsIImsService::FEATURE_TYPE_VOICE_OVER_WIFI)) {

      if (mWifiService != NULL) {
        // mWifiService->UpdateIncomingCallAction(REJECT);
      }

    } else {

      if (mWifiService != NULL) {
        // mWifiService->UpdateIncomingCallAction(NORMAL);
      }
    }
  }

  if ((mCurrentImsFeature == nsIImsService::FEATURE_TYPE_VOICE_OVER_WIFI
                                            && isInCall != mIsWifiCalling)
                                            || (!isInCall && mIsWifiCalling)) {
    mIsWifiCalling = isInCall;

    if (mImsHandlers[mPrimaryServiceId] != NULL) {
      mImsHandlers[mPrimaryServiceId]->NotifyWifiCalling(mIsWifiCalling);
    }
  }

  if (!mIsCalling && mImsHandlers[mPrimaryServiceId] != NULL) {
    int16_t srvccState;
    mImsHandlers[mPrimaryServiceId]->GetSrvccState(&srvccState);

    if (srvccState == HANDOVER_COMPLETED) {
      mImsHandlers[mPrimaryServiceId]->SetSrvccState(-1);
    }
  }

  if (!isInCall) {

    if (mIsVowifiCall) {
        mIsVowifiCall = false;

    } else if (mIsVolteCall) {
        mIsVolteCall = false;
    }
  }

  LOG("updateInCallState->isInCall:%d, mIsWifiCalling:%d, mIsVolteCall:%d",
      isInCall, mIsWifiCalling, mIsVolteCall);
}

bool
ImsService::IsWifiRegistered()
{
 return mWifiRegistered;
}

void
ImsService::NotifyCMImsCapabilityChange(bool volteEnable, bool wifiEnable)
{
  LOG("NotifyCMImsCapabilityChange->volteEnable:%d, wifiEnable:%d",
      volteEnable, wifiEnable);

  if (mImsServiceCallback) {
    int16_t capability = nsIImsRegHandler::IMS_CAPABILITY_UNKNOWN;

    if (volteEnable) {
      capability = nsIImsRegHandler::IMS_CAPABILITY_VOICE_OVER_CELLULAR;

    } else if (wifiEnable) {
      capability = nsIImsRegHandler::IMS_CAPABILITY_VOICE_OVER_WIFI;
    }

    mImsServiceCallback->OnImsCapabilityChange(capability);
  } else {
    LOG("NotifyCMImsCapabilityChange mImsServiceCallback is null");
  }

}

void
ImsService::UpdateInCallType(bool volteCall, bool wifiCall)
{
  LOG("UpdateInCallType->volteCall:%d, wifiCall:%d",
      volteCall, wifiCall);
  mIsVolteCall = volteCall;
  mIsVowifiCall = wifiCall;

  if (mWifiService) {

    if (mIsVolteCall) {
      mWifiService->UpdateDataRouterState(CALL_VOLTE);
    }

    if (mIsVowifiCall) {
      mWifiService->UpdateDataRouterState(CALL_VOWIFI);
    }
  }
}

nsresult
ImsService::SetNotifyCallbackTimeout(bool result)
{
  LOG("VideoCallProvider::SetNotifyCallbackTimeout result:%d", result);

  if (!mTimer) {
    mTimer = do_CreateInstance("@mozilla.org/timer;1");
    NS_ENSURE_STATE(mTimer);
  }

  if (result) {
    mTimer->InitWithFuncCallback(TimerSuccessCallback, this,
                                 100,
                                 nsITimer::TYPE_ONE_SHOT);
  } else {
    mTimer->InitWithFuncCallback(TimerFailureCallback, this,
                                 100,
                                 nsITimer::TYPE_ONE_SHOT);
  }

  return NS_OK;
}

void
ImsService::TimerSuccessCallback(nsITimer* aTimer, void* aClosure)
{
  LOG("ImsService::TimerSuccessCallback");
  RefPtr<ImsService> imsService = ImsService::GetInstance();
  if (imsService->mImsServiceCallback && imsService->mReleaseVowifiRequest) {
    imsService->mImsServiceCallback->OperationSuccessed(
                                  imsService->mReleaseVowifiRequest->mRequestId,
                                  IMS_OPERATION_SET_VOWIFI_UNAVAILABLE);

    delete imsService->mReleaseVowifiRequest;
    imsService->mReleaseVowifiRequest = NULL;
  }
}

void
ImsService::TimerFailureCallback(nsITimer* aTimer, void* aClosure)
{
  LOG("ImsService::TimerFailureCallback");
  RefPtr<ImsService> imsService = ImsService::GetInstance();
  if (imsService->mImsServiceCallback && imsService->mReleaseVowifiRequest) {
    char str[100];
    uint16_t aErrorCode;
    snprintf(str, 100, "%s%d", "wifi release fail:", aErrorCode);

    imsService->mImsServiceCallback->OperationFailed(
                                  imsService->mReleaseVowifiRequest->mRequestId,
                                  NS_ConvertASCIItoUTF16(str),
                                  IMS_OPERATION_SET_VOWIFI_UNAVAILABLE);

    delete imsService->mReleaseVowifiRequest;
    imsService->mReleaseVowifiRequest = NULL;
  }
}

ImsService::~ImsService()
{
  LOG("ImsService destructor...");
  mImsHandlers.Clear();
  mImsRegService = nullptr;
  mRadioService = nullptr;
  mWifiService->UnregisterCallback();
  mWifiService = nullptr;
  mVoWifiCallback = nullptr;
  mImsServiceCallback = nullptr;
}

NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(ImsService,
                                         ImsService::GetInstance)

NS_DEFINE_NAMED_CID(NS_IMSSERVICE_CID);

static const mozilla::Module::CIDEntry kImsServiceCIDs[] = {
  { &kNS_IMSSERVICE_CID, false, nullptr, ImsServiceConstructor },
  { nullptr }
};

static const mozilla::Module::ContractIDEntry kImsServiceContracts[] = {
  { IMSSERVICE_CONTRACTID, &kNS_IMSSERVICE_CID },
  { nullptr }
};

static const mozilla::Module kImsServiceModule = {
  mozilla::Module::kVersion,
  kImsServiceCIDs,
  kImsServiceContracts,
  nullptr
};
} // namespace imsservice
} // namespace dom
} // namespace mozilla
NSMODULE_DEFN(ImsServiceModule) = &mozilla::dom::imsservice::kImsServiceModule;
