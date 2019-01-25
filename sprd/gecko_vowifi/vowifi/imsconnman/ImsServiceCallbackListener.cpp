#include "ImsServiceCallbackListener.h"
#include "nsIImsServiceCallback.h"


BEGIN_VOWIFI_NAMESPACE

const string ImsServiceCallbackListener::TAG = "ImsServiceCallbackListener";

NS_IMPL_ISUPPORTS(ImsServiceCallbackListener, nsIImsServiceCallback)

ImsServiceCallbackListener::ImsServiceCallbackListener() {
}

ImsServiceCallbackListener::~ImsServiceCallbackListener() {
}

void ImsServiceCallbackListener::update(shared_ptr<ImsConnManService> &service) {
    if (nullptr != service) {
        LOGD(_TAG(TAG), "update: ImsConnManService isn't nullptr");
        mpService = service;
    } else {
        LOGD(_TAG(TAG), "update: ImsConnManService is nullptr");
        if (nullptr != mpService) mpService.reset();
    }
}

NS_IMETHODIMP ImsServiceCallbackListener::OperationSuccessed(int16_t aId, uint16_t aType) {
    if (nullptr != mpService) mpService->operationSuccessed(static_cast<int>(aId), static_cast<int>(aType));
    return NS_OK;
}

NS_IMETHODIMP ImsServiceCallbackListener::OperationFailed(int16_t aId, const nsAString & aReason, uint16_t aType) {
    if (nullptr != mpService) mpService->operationFailed(static_cast<int>(aId), NS_ConvertUTF16toUTF8(aReason).get(), static_cast<int>(aType));
    return NS_OK;
}

NS_IMETHODIMP ImsServiceCallbackListener::ImsCallEnd(uint16_t aType) {
    LOGD(_TAG(TAG), "ImsCallEnd: aType = %d", aType);
    if (nullptr != mpService) mpService->imsCallEndProcess();
    return NS_OK;
}

NS_IMETHODIMP ImsServiceCallbackListener::ImsPdnStateChange(int16_t aState) {
    if (nullptr != mpService) mpService->voltePdnStateChanged(static_cast<int>(aState));
    return NS_OK;
}

NS_IMETHODIMP ImsServiceCallbackListener::OnDPDDisconnected() {
    return NS_OK;
}

NS_IMETHODIMP ImsServiceCallbackListener::OnNoRtpReceived(bool isVideo) {
    if (nullptr != mpService) mpService->noRtpReceivedProcess(isVideo);
    return NS_OK;
}

NS_IMETHODIMP ImsServiceCallbackListener::OnVoWiFiError(uint16_t statusCode) {
    if (nullptr != mpService) mpService->onUnsolicitedVowifiError(static_cast<int>(statusCode));
    return NS_OK;
}

NS_IMETHODIMP ImsServiceCallbackListener::OnRtpReceived(bool isVideo) {
    if (nullptr != mpService) mpService->rtpReceivedProcess(isVideo);
    return NS_OK;
}

NS_IMETHODIMP ImsServiceCallbackListener::OnMediaQualityChanged(bool isVideo, int16_t lose, int16_t jitter, int16_t rtt) {
    if (nullptr != mpService) mpService->mediaQualityReceivedProcess(isVideo, static_cast<int>(lose), static_cast<int>(jitter), static_cast<int>(rtt));
    return NS_OK;
}

NS_IMETHODIMP ImsServiceCallbackListener::OnSrvccFaild() {
    if (nullptr != mpService) mpService->srvccFaildProcess();
    return NS_OK;
}

NS_IMETHODIMP ImsServiceCallbackListener::OnVideoStateChanged(uint16_t videoState) {
    LOGD(_TAG(TAG), "OnVideoStateChanged: videoState = %d", videoState);
    if (nullptr != mpService) mpService->imsCallTypeChanged();
    return NS_OK;
}

NS_IMETHODIMP ImsServiceCallbackListener::OnSetVowifiRegister(uint16_t action) {
    return NS_OK;
}

NS_IMETHODIMP ImsServiceCallbackListener::OnImsCapabilityChange(int16_t aCapability) {
    LOGD(_TAG(TAG), "OnImsCapabilityChange: aCapability = %d", aCapability);

    int imsBearer = IMS_TYPE_UNKNOWN;
    int capability = static_cast<int>(aCapability);
    switch (capability) {
        case nsIImsRegHandler::IMS_CAPABILITY_UNKNOWN:
            imsBearer = IMS_TYPE_UNKNOWN;
            break;

        case nsIImsRegHandler::IMS_CAPABILITY_VOICE_OVER_CELLULAR:
            imsBearer = IMS_TYPE_VOLTE;
            break;

        case nsIImsRegHandler::IMS_CAPABILITY_VOICE_OVER_WIFI:
            imsBearer = IMS_TYPE_VOWIFI;
            break;

        default:
            LOGD(_TAG(TAG), "OnImsCapabilityChange: invalid Capability: %d, please check !!!!!", capability);
            imsBearer = IMS_TYPE_UNKNOWN;
            break;
    }

    if (nullptr != mpService) mpService->onImsRegisterStateChange(imsBearer);
    return NS_OK;
}

END_VOWIFI_NAMESPACE
