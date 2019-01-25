#include "NetworkUtils.h"
#include "ImsConnManMonitor.h"
#include "ImsConnManStateInfo.h"
#include "portinglayer/PortingConstants.h"
#include "nsContentUtils.h"
#include "nsIMobileConnectionInfo.h"
#include "mozilla/Services.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/SettingChangeNotificationBinding.h"


BEGIN_VOWIFI_NAMESPACE

static const char* MOZSETTINGS_CHANGED_ID = "mozsettings-changed";
static const char* SETTING_KEY_AIRPLANEMODE_ENABLED = "airplaneMode.enabled";
static const char* NETWORK_CONNECTION_STATE_CHANGED = "network-connection-state-changed";
static const char* SETTING_KEY_WIFI_CALLING_ENABLED = "ril.ims.enabled";
static const char* SETTING_KEY_WIFI_CALLING_PREFERRED = "ril.ims.preferredProfile";

const string NetworkUtils::TAG = "NetworkUtils";
AirplaneModeChangedListener *NetworkUtils::mpApmListener = nullptr;
WifiNetworkListener *NetworkUtils::mpWifiNetworkListener = nullptr;
MobileConnectionListener *NetworkUtils::mpMobileConnectionListener = nullptr;
WifiCallingEnabledListener *NetworkUtils::mpWfcEnabledListener = nullptr;
WifiCallingPreferredListener *NetworkUtils::mpWfcPreferredListener = nullptr;

void NetworkUtils::createReadDatabaseTask() {
    nsCOMPtr<nsISettingsService> settings = do_GetService("@mozilla.org/settingsService;1");
    nsCOMPtr<nsISettingsServiceLock> settingsLock;
    nsresult rv = settings->CreateLock(nullptr, getter_AddRefs(settingsLock));
    if (NS_FAILED(rv)) {
        LOGE(_TAG(NetworkUtils::TAG), "createReadDatabaseTask: Create settingsLock failed !!!!!");
        return;
    }

    RefPtr<ReadWifiCallingEnabledCallback> wfcEnabledCallback = new ReadWifiCallingEnabledCallback();
    rv = settingsLock->Get(SETTING_KEY_WIFI_CALLING_ENABLED, wfcEnabledCallback);
    if (NS_FAILED(rv)) {
        LOGE(_TAG(NetworkUtils::TAG), "createReadDatabaseTask: settingsLock->Get wfc enabled failed !!!!!");
    }

    RefPtr<ReadWifiCallingPreferredCallback> wfcPreferredCallback = new ReadWifiCallingPreferredCallback();
    rv = settingsLock->Get(SETTING_KEY_WIFI_CALLING_PREFERRED, wfcPreferredCallback);
    if (NS_FAILED(rv)) {
        LOGE(_TAG(NetworkUtils::TAG), "createReadDatabaseTask: settingsLock->Get wfc preferred failed !!!!!");
    }

    RefPtr<ReadAirplaneModeSettingsCallback> apmCallback = new ReadAirplaneModeSettingsCallback();
    rv = settingsLock->Get(SETTING_KEY_AIRPLANEMODE_ENABLED, apmCallback);
    if (NS_FAILED(rv)) {
        LOGE(_TAG(NetworkUtils::TAG), "createReadDatabaseTask: settingsLock->Get APM failed !!!!!");
    }

    LOGD(_TAG(NetworkUtils::TAG), "createReadDatabaseTask");
}

void NetworkUtils::registerListenerAfterBoot() {
    if (nullptr == mpApmListener) {
        mpApmListener = new AirplaneModeChangedListener();
        mpApmListener->registerApmChangedListener();
    }

    if (nullptr == mpWfcEnabledListener) {
        mpWfcEnabledListener = new WifiCallingEnabledListener();
        mpWfcEnabledListener->registerWifiCallingEnabledListener();
    }

    if (nullptr == mpWfcPreferredListener) {
        mpWfcPreferredListener = new WifiCallingPreferredListener();
        mpWfcPreferredListener->registerWifiCallingPreferredListener();
    }
}

void NetworkUtils::registerListenerAfterAttachService() {
    if (nullptr == mpWifiNetworkListener) {
        mpWifiNetworkListener = new WifiNetworkListener();
        mpWifiNetworkListener->registerWifiNetworkListener();
    }

    if (nullptr == mpMobileConnectionListener) {
        mpMobileConnectionListener = new MobileConnectionListener();
        mpMobileConnectionListener->registerMobileConnectionListener();
    }
}

void NetworkUtils::unregisterListenerAfterDetachService() {
    if (nullptr != mpWifiNetworkListener) mpWifiNetworkListener->unregisterWifiNetworkListener();
    if (nullptr != mpMobileConnectionListener) mpMobileConnectionListener->unregisterMobileConnectionListener();
}

void NetworkUtils::update(shared_ptr<ImsConnManService> &service) {
    if (nullptr != mpWifiNetworkListener) mpWifiNetworkListener->update(service);
    if (nullptr != mpMobileConnectionListener) mpMobileConnectionListener->update(service);
    if (nullptr != mpWfcEnabledListener) mpWfcEnabledListener->update(service);
    if (nullptr != mpWfcPreferredListener) mpWfcPreferredListener->update(service);
}

int NetworkUtils::getCurrentWifiRssi() {
    return PortingInterface::getWifiRssi();
}

int NetworkUtils::getCurrentLteRsrp() {
    return PortingInterface::getLteRsrp();
}

bool NetworkUtils::isPcscfAddressReachable(const string &localAddr, const string &pcscfAddr) {
    bool isPingSucess = false;
    if (localAddr.empty() || pcscfAddr.empty()) {
        LOGE(_TAG(NetworkUtils::TAG), "isPcscfAddressReachable: IP address is null or empty, not reachable !!!!!");
        return isPingSucess;
    }

    string cmdString;
    if (string::npos == pcscfAddr.find_first_of(":")) {
        cmdString = "ping -c 1 -W 2 -I " + localAddr + " " + pcscfAddr;
    } else {
        cmdString = "ping6 -c 1 -W 2 -I " + localAddr + " " + pcscfAddr;
    }

    LOGD(_TAG(NetworkUtils::TAG), "isPcscfAddressReachable: ping cmd = %s", cmdString.c_str());

    int ret = -1;
    sig_t oldSignal = signal(SIGCHLD, SIG_DFL);
    ret = system(cmdString.c_str());
    signal(SIGCHLD, oldSignal);
    if (0 == ret) {
        isPingSucess = true;
        LOGD(_TAG(NetworkUtils::TAG), "isPcscfAddressReachable: Ping IP address is reachable");
    } else {
        LOGE(_TAG(NetworkUtils::TAG), "isPcscfAddressReachable: Ping IP address is unreachable, ret = %d, errno = %s", \
                ret, strerror(errno));
    }

    return isPingSucess;
}

/******************** class AirplaneModeChangedListener begin ********************/
NS_IMPL_ISUPPORTS(AirplaneModeChangedListener, nsIObserver)

AirplaneModeChangedListener::AirplaneModeChangedListener() {
    mObserverService = mozilla::services::GetObserverService();
    if (!mObserverService) {
        LOGE(_TAG(NetworkUtils::TAG), "AirplaneModeChangedListener: Could not acquire nsIObserverService !!!!!");
        return;
    }
}

AirplaneModeChangedListener::~AirplaneModeChangedListener() {
}

void AirplaneModeChangedListener::registerApmChangedListener() {
    if (mObserverService)
        mObserverService->AddObserver(this, MOZSETTINGS_CHANGED_ID, false);
}

void AirplaneModeChangedListener::unregisterApmChangedListener() {
    if (mObserverService)
        mObserverService->RemoveObserver(this, MOZSETTINGS_CHANGED_ID);
}

NS_IMETHODIMP
AirplaneModeChangedListener::Observe(nsISupports *aSubject, const char * aTopic, const char16_t * aData) {
    if (0 == strncmp(aTopic, MOZSETTINGS_CHANGED_ID, strlen(MOZSETTINGS_CHANGED_ID))) {
        RootedDictionary<dom::SettingChangeNotification> setting(nsContentUtils::RootingCx());

        if (!WrappedJSToDictionary(aSubject, setting)) {
            return NS_ERROR_FAILURE;
        }

        if (!setting.mKey.EqualsASCII(SETTING_KEY_AIRPLANEMODE_ENABLED)) {
            return NS_ERROR_FAILURE;
        }

        if (!setting.mValue.isBoolean()) {
            return NS_ERROR_FAILURE;
        }

        bool isAirplaneModeEnabled = setting.mValue.toBoolean();
        PortingInterface::setAirplaneModeEnabled(isAirplaneModeEnabled);

        LOGD(_TAG(NetworkUtils::TAG), "AirplaneModeChangedListener: isAirplaneModeEnabled = %s", \
                isAirplaneModeEnabled ? "true" : "false");
    }

    return NS_OK;
}
/******************** class AirplaneModeChangedListener end ********************/


/******************** class ReadAirplaneModeSettingsCallback begin ********************/
NS_IMPL_ISUPPORTS(ReadAirplaneModeSettingsCallback, nsISettingsServiceCallback)

ReadAirplaneModeSettingsCallback::ReadAirplaneModeSettingsCallback() {
}

ReadAirplaneModeSettingsCallback::~ReadAirplaneModeSettingsCallback() {
}

NS_IMETHODIMP
ReadAirplaneModeSettingsCallback::Handle(const nsAString & aName, JS::HandleValue aResult) {
    if (!aResult.isBoolean()) {
        LOGE(_TAG(NetworkUtils::TAG), "ReadAirplaneModeSettingsCallback: isBoolean() is \"false\" !!!!!");
        return NS_ERROR_FAILURE;
    }

    bool isAirplaneModeEnabled = aResult.toBoolean();
    PortingInterface::setAirplaneModeEnabled(isAirplaneModeEnabled);
    LOGD(_TAG(NetworkUtils::TAG), "ReadAirplaneModeSettingsCallback: isAirplaneModeEnabled = %s", \
            isAirplaneModeEnabled ? "true" : "false");

    if (ImsConnManStateInfo::isReceivedIccInfoChanged()) {
        int iccCardState = PortingInterface::getIccCardState();
        LOGD(_TAG(NetworkUtils::TAG), "ReadAirplaneModeSettingsCallback: iccCardState = %d", iccCardState);

        if (SIM_STATE_READY == iccCardState) {
            ImsConnManMonitor::getInstance().updateSimCardState(iccCardState);
        }
    }

    ImsConnManStateInfo::setReadAirplaneModeState(true);

    return NS_OK;
}

NS_IMETHODIMP
ReadAirplaneModeSettingsCallback::HandleError(const nsAString & aErrorMessage) {
    return NS_OK;
}
/******************** class ReadAirplaneModeSettingsCallback end ********************/


/******************** class WifiNetworkListener begin ********************/
NS_IMPL_ISUPPORTS(WifiNetworkListener, nsIObserver)

WifiNetworkListener::WifiNetworkListener() {
    mObserverService = mozilla::services::GetObserverService();
    if (!mObserverService) {
        LOGE(_TAG(NetworkUtils::TAG), "WifiNetworkListener: Could not acquire nsIObserverService !!!!!");
        return;
    }
}

WifiNetworkListener::~WifiNetworkListener() {
}

void WifiNetworkListener::update(shared_ptr<ImsConnManService> &service) {
    if (nullptr != service) {
        LOGD(_TAG(NetworkUtils::TAG), "WifiNetworkListener: ImsConnManService isn't nullptr");
        mpService = service;
    } else {
        LOGD(_TAG(NetworkUtils::TAG), "WifiNetworkListener: ImsConnManService is nullptr");
        if (nullptr != mpService) mpService.reset();
    }
}

void WifiNetworkListener::registerWifiNetworkListener() {
    if (mObserverService)
        mObserverService->AddObserver(this, NETWORK_CONNECTION_STATE_CHANGED, false);
}

void WifiNetworkListener::unregisterWifiNetworkListener() {
    if (mObserverService)
        mObserverService->RemoveObserver(this, NETWORK_CONNECTION_STATE_CHANGED);
}

NS_IMETHODIMP
WifiNetworkListener::Observe(nsISupports *aSubject, const char * aTopic, const char16_t * aData) {
    if (0 == strncmp(aTopic, NETWORK_CONNECTION_STATE_CHANGED, strlen(NETWORK_CONNECTION_STATE_CHANGED))) {
        int networkType, networkState;
        nsCOMPtr<nsINetworkInfo> networkInfo = do_QueryInterface(aSubject);
        if (!networkInfo) {
            LOGE(_TAG(NetworkUtils::TAG), "WifiNetworkListener: Could not acquire nsINetworkInfo !!!!!");
            return NS_ERROR_FAILURE;
        }

        if (NS_OK == networkInfo->GetType(&networkType) && NS_OK == networkInfo->GetState(&networkState)) {
            if (nsINetworkInfo::NETWORK_TYPE_WIFI == networkType) {
                switch (networkState) {
                    case WIFI_STATE_CONNECTED:
                        if (!ImsConnManStateInfo::isWifiConnected()) {
                            LOGD(_TAG(NetworkUtils::TAG), "WifiNetworkListener: Wifi has been \"Connected\"");

                            ImsConnManStateInfo::setWifiConnected(true);
                            if (nullptr != mpService)
                                mpService->onWifiNetworkConnected();
                        }
                        break;

                    case WIFI_STATE_DISCONNECTED:
                        if (ImsConnManStateInfo::isWifiConnected()) {
                            LOGD(_TAG(NetworkUtils::TAG), "WifiNetworkListener: Wifi has been \"Disconnected\"");

                            ImsConnManStateInfo::setWifiConnected(false);
                            if (nullptr != mpService)
                                mpService->onWifiNetworkDisconnected();
                        }
                        break;

                    default:
                        LOGE(_TAG(NetworkUtils::TAG), "WifiNetworkListener: Wifi network state error, networkState = %d", networkState);
                        break;
                }
            }
        } else {
            LOGE(_TAG(NetworkUtils::TAG), "WifiNetworkListener: Could not acquire network type or state by GetType() or GetState() !!!!!");
        }
    }

    return NS_OK;
}
/******************** class WifiNetworkListener end ********************/


/******************** class MobileConnectionListener begin ********************/
NS_IMPL_ISUPPORTS(MobileConnectionListener, nsIMobileConnectionListener)

MobileConnectionListener::MobileConnectionListener() : mMobileNetworkType("") {
    nsCOMPtr<nsIMobileConnectionService> mobileService = do_GetService(NS_MOBILE_CONNECTION_SERVICE_CONTRACTID);
    if (!mobileService) {
        LOGE(_TAG(NetworkUtils::TAG), "MobileConnectionListener: Could not acquire nsIMobileConnectionService !!!!!");
        return;
    }

    mobileService->GetItemByServiceId(PortingInterface::getDefaultDataPhoneId(), getter_AddRefs(mMobileConnection));
    if (!mMobileConnection) {
        LOGE(_TAG(NetworkUtils::TAG), "MobileConnectionListener: Could not acquire nsIMobileConnection !!!!!");
        return;
    }
}

MobileConnectionListener::~MobileConnectionListener() {
}

void MobileConnectionListener::update(shared_ptr<ImsConnManService> &service) {
    if (nullptr != service) {
        LOGD(_TAG(NetworkUtils::TAG), "MobileConnectionListener: ImsConnManService isn't nullptr");
        mpService = service;
    } else {
        LOGD(_TAG(NetworkUtils::TAG), "MobileConnectionListener: ImsConnManService is nullptr");
        if (nullptr != mpService) mpService.reset();
    }
}

void MobileConnectionListener::registerMobileConnectionListener() {
    if (mMobileConnection)
        mMobileConnection->RegisterListener(this);
}

void MobileConnectionListener::unregisterMobileConnectionListener() {
    if (mMobileConnection)
        mMobileConnection->UnregisterListener(this);
}

NS_IMETHODIMP MobileConnectionListener::NotifyVoiceChanged() {
    return NS_OK;
}

NS_IMETHODIMP MobileConnectionListener::NotifyDataChanged() {
    string newMobileNetworkType;

    nsCOMPtr<nsIMobileConnectionInfo> dataInfo;
    mMobileConnection->GetData(getter_AddRefs(dataInfo));
    if (!dataInfo) {
        LOGE(_TAG(NetworkUtils::TAG), "MobileConnectionListener: Could not acquire nsIMobileConnectionInfo !!!!!");
        return NS_ERROR_FAILURE;
    }

    nsString type;
    if (NS_OK == dataInfo->GetType(type)) {
        newMobileNetworkType = NS_ConvertUTF16toUTF8(type).get();
    } else {
        LOGE(_TAG(NetworkUtils::TAG), "MobileConnectionListener: Could not acquire network type by GetType() !!!!!");
        return NS_ERROR_FAILURE;
    }

    if (newMobileNetworkType.empty()) newMobileNetworkType = "unknown";
    if (0 != newMobileNetworkType.compare(mMobileNetworkType)) {
        mMobileNetworkType = newMobileNetworkType;
        LOGD(_TAG(NetworkUtils::TAG), "MobileConnectionListener: newMobileNetworkType = \"%s\"", newMobileNetworkType.c_str());

        if (nullptr != mpService)
            mpService->onServiceStateChanged(mMobileNetworkType);
    }

    return NS_OK;
}

NS_IMETHODIMP MobileConnectionListener::NotifyDataError(const nsAString & message) {
    return NS_OK;
}

NS_IMETHODIMP MobileConnectionListener::NotifyCFStateChanged(uint16_t action,
                                                               uint16_t reason,
                                                               const nsAString & number,
                                                               uint16_t timeSeconds,
                                                               uint16_t serviceClass) {
    return NS_OK;
}

NS_IMETHODIMP MobileConnectionListener::NotifyEmergencyCbModeChanged(bool active, uint32_t timeoutMs) {
    return NS_OK;
}

NS_IMETHODIMP MobileConnectionListener::NotifyOtaStatusChanged(const nsAString & status) {
    return NS_OK;
}

NS_IMETHODIMP MobileConnectionListener::NotifyRadioStateChanged() {
    return NS_OK;
}

NS_IMETHODIMP MobileConnectionListener::NotifyClirModeChanged(uint32_t mode) {
    return NS_OK;
}

NS_IMETHODIMP MobileConnectionListener::NotifyLastKnownNetworkChanged() {
    return NS_OK;
}

NS_IMETHODIMP MobileConnectionListener::NotifyLastKnownHomeNetworkChanged() {
    return NS_OK;
}

NS_IMETHODIMP MobileConnectionListener::NotifyNetworkSelectionModeChanged() {
    return NS_OK;
}

NS_IMETHODIMP MobileConnectionListener::NotifyDeviceIdentitiesChanged() {
    return NS_OK;
}

NS_IMETHODIMP MobileConnectionListener::NotifySignalStrengthChanged() {
    return NS_OK;
}
/******************** class MobileConnectionListener end ********************/


/******************** class WifiCallingEnabledListener begin ********************/
NS_IMPL_ISUPPORTS(WifiCallingEnabledListener, nsIObserver)

WifiCallingEnabledListener::WifiCallingEnabledListener() {
    mObserverService = mozilla::services::GetObserverService();
    if (!mObserverService) {
        LOGE(_TAG(NetworkUtils::TAG), "WifiCallingEnabledListener: Could not acquire nsIObserverService !!!!!");
        return;
    }
}

WifiCallingEnabledListener::~WifiCallingEnabledListener() {
}

void WifiCallingEnabledListener::update(shared_ptr<ImsConnManService> &service) {
    if (nullptr != service) {
        LOGD(_TAG(NetworkUtils::TAG), "WifiCallingEnabledListener: ImsConnManService isn't nullptr");
        mpService = service;
    } else {
        LOGD(_TAG(NetworkUtils::TAG), "WifiCallingEnabledListener: ImsConnManService is nullptr");
        if (nullptr != mpService) mpService.reset();
    }
}

void WifiCallingEnabledListener::registerWifiCallingEnabledListener() {
    if (mObserverService)
        mObserverService->AddObserver(this, MOZSETTINGS_CHANGED_ID, false);
}

void WifiCallingEnabledListener::unregisterWifiCallingEnabledListener() {
    if (mObserverService)
        mObserverService->RemoveObserver(this, MOZSETTINGS_CHANGED_ID);
}

NS_IMETHODIMP
WifiCallingEnabledListener::Observe(nsISupports *aSubject, const char * aTopic, const char16_t * aData) {
    if (0 == strncmp(aTopic, MOZSETTINGS_CHANGED_ID, strlen(MOZSETTINGS_CHANGED_ID))) {
        RootedDictionary<dom::SettingChangeNotification> setting(nsContentUtils::RootingCx());

        if (!WrappedJSToDictionary(aSubject, setting)) {
            return NS_ERROR_FAILURE;
        }

        if (!setting.mKey.EqualsASCII(SETTING_KEY_WIFI_CALLING_ENABLED)) {
            return NS_ERROR_FAILURE;
        }

        if (!setting.mValue.isBoolean()) {
            return NS_ERROR_FAILURE;
        }

        bool isWfcEnabled = setting.mValue.toBoolean();
        if (!isWfcEnabled) {
            ImsConnManStateInfo::setWifiCallingEnabled(false);
            ImsConnManStateInfo::setWifiCallingMode(WFC_MODE_UNKNOWN);
            LOGD(_TAG(NetworkUtils::TAG), "WifiCallingEnabledListener: isWfcEnabled = \"false\"");

            if (ImsConnManStateInfo::isServiceAttached() && nullptr != mpService) {
                mpService->onWifiCallingChanged(isWfcEnabled);
            }
        }
    }

    return NS_OK;
}
/******************** class WifiCallingEnabledListener end ********************/


/******************** class ReadWifiCallingEnabledCallback begin ********************/
NS_IMPL_ISUPPORTS(ReadWifiCallingEnabledCallback, nsISettingsServiceCallback)

ReadWifiCallingEnabledCallback::ReadWifiCallingEnabledCallback() {
}

ReadWifiCallingEnabledCallback::~ReadWifiCallingEnabledCallback() {
}

NS_IMETHODIMP
ReadWifiCallingEnabledCallback::Handle(const nsAString & aName, JS::HandleValue aResult) {
    if (!aResult.isBoolean()) {
        LOGE(_TAG(NetworkUtils::TAG), "ReadWifiCallingEnabledCallback: isBoolean() is \"false\" !!!!!");
        return NS_ERROR_FAILURE;
    }

    bool isWfcEnabled = aResult.toBoolean();
    ImsConnManStateInfo::setWifiCallingEnabled(isWfcEnabled);
    LOGD(_TAG(NetworkUtils::TAG), "ReadWifiCallingEnabledCallback: isWfcEnabled = %s", \
            isWfcEnabled ? "true" : "false");

    return NS_OK;
}

NS_IMETHODIMP
ReadWifiCallingEnabledCallback::HandleError(const nsAString & aErrorMessage) {
    return NS_OK;
}
/******************** class ReadWifiCallingEnabledCallback end ********************/


/******************** class WifiCallingPreferredListener begin ********************/
NS_IMPL_ISUPPORTS(WifiCallingPreferredListener, nsIObserver)

WifiCallingPreferredListener::WifiCallingPreferredListener() {
    mObserverService = mozilla::services::GetObserverService();
    if (!mObserverService) {
        LOGE(_TAG(NetworkUtils::TAG), "WifiCallingPreferredListener: Could not acquire nsIObserverService !!!!!");
        return;
    }
}

WifiCallingPreferredListener::~WifiCallingPreferredListener() {
}

void WifiCallingPreferredListener::update(shared_ptr<ImsConnManService> &service) {
    if (nullptr != service) {
        LOGD(_TAG(NetworkUtils::TAG), "WifiCallingPreferredListener: ImsConnManService isn't nullptr");
        mpService = service;
    } else {
        LOGD(_TAG(NetworkUtils::TAG), "WifiCallingPreferredListener: ImsConnManService is nullptr");
        if (nullptr != mpService) mpService.reset();
    }
}

void WifiCallingPreferredListener::registerWifiCallingPreferredListener() {
    if (mObserverService)
        mObserverService->AddObserver(this, MOZSETTINGS_CHANGED_ID, false);
}

void WifiCallingPreferredListener::unregisterWifiCallingPreferredListener() {
    if (mObserverService)
        mObserverService->RemoveObserver(this, MOZSETTINGS_CHANGED_ID);
}

NS_IMETHODIMP
WifiCallingPreferredListener::Observe(nsISupports *aSubject, const char * aTopic, const char16_t * aData) {
    if (0 == strncmp(aTopic, MOZSETTINGS_CHANGED_ID, strlen(MOZSETTINGS_CHANGED_ID))) {
        AutoSafeJSContext cx;
        RootedDictionary<dom::SettingChangeNotification> setting(cx);

        if (!WrappedJSToDictionary(cx, aSubject, setting)) {
            return NS_ERROR_FAILURE;
        }

        if (!setting.mKey.EqualsASCII(SETTING_KEY_WIFI_CALLING_PREFERRED)) {
            return NS_ERROR_FAILURE;
        }

        if (!setting.mValue.isString()) {
            return NS_ERROR_FAILURE;
        }

        nsAutoJSString jsString;
        if (!jsString.init(cx, setting.mValue.toString())) {
            LOGE(_TAG(NetworkUtils::TAG), "WifiCallingPreferredListener: nsAutoJSString.init() failed !!!!!");
            return NS_ERROR_FAILURE;
        }

        string wfcPreferred;
        wfcPreferred = NS_ConvertUTF16toUTF8(jsString).get();
        LOGD(_TAG(NetworkUtils::TAG), "WifiCallingPreferredListener: wfcPreferred = \"%s\"", wfcPreferred.c_str());

        if (0 == wfcPreferred.compare("cellular-preferred")) {
            ImsConnManStateInfo::setWifiCallingEnabled(true);
            LOGD(_TAG(NetworkUtils::TAG), "WifiCallingPreferredListener: isWfcEnabled = \"true\"");

            if (!ImsConnManStateInfo::isServiceAttached()) {
                ImsConnManMonitor::getInstance().onBindOperatorService();
            }

            ImsConnManStateInfo::setWifiCallingMode(WFC_MODE_CELLULAR_PREFERRED);
            if (nullptr != mpService) mpService->onWifiCallingModeChanged(WFC_MODE_CELLULAR_PREFERRED);
        } else if (0 == wfcPreferred.compare("cellular-only")) {
            ImsConnManStateInfo::setWifiCallingEnabled(false);
            LOGD(_TAG(NetworkUtils::TAG), "WifiCallingPreferredListener: isWfcEnabled = \"false\"");

            ImsConnManStateInfo::setWifiCallingMode(WFC_MODE_CELLULAR_ONLY);
            if (nullptr != mpService) mpService->onWifiCallingModeChanged(WFC_MODE_CELLULAR_ONLY);
        } else if (0 == wfcPreferred.compare("wifi-preferred")) {
            ImsConnManStateInfo::setWifiCallingEnabled(true);
            LOGD(_TAG(NetworkUtils::TAG), "WifiCallingPreferredListener: isWfcEnabled = \"true\"");

            if (!ImsConnManStateInfo::isServiceAttached()) {
                ImsConnManMonitor::getInstance().onBindOperatorService();
            }

            ImsConnManStateInfo::setWifiCallingMode(WFC_MODE_WIFI_PREFERRED);
            if (nullptr != mpService) mpService->onWifiCallingModeChanged(WFC_MODE_WIFI_PREFERRED);
        } else if (0 == wfcPreferred.compare("wifi-only")) {
            ImsConnManStateInfo::setWifiCallingEnabled(true);
            LOGD(_TAG(NetworkUtils::TAG), "WifiCallingPreferredListener: isWfcEnabled = \"true\"");

            if (!ImsConnManStateInfo::isServiceAttached()) {
                ImsConnManMonitor::getInstance().onBindOperatorService();
            }

            ImsConnManStateInfo::setWifiCallingMode(WFC_MODE_WIFI_ONLY);
            if (nullptr != mpService) mpService->onWifiCallingModeChanged(WFC_MODE_WIFI_ONLY);
        } else {
            LOGE(_TAG(NetworkUtils::TAG), "WifiCallingPreferredListener: unknown wfcPreferred !!!!!");
        }
    }

    return NS_OK;
}
/******************** class WifiCallingPreferredListener end ********************/


/******************** class ReadWifiCallingPreferredCallback begin ********************/
NS_IMPL_ISUPPORTS(ReadWifiCallingPreferredCallback, nsISettingsServiceCallback)

ReadWifiCallingPreferredCallback::ReadWifiCallingPreferredCallback() {
}

ReadWifiCallingPreferredCallback::~ReadWifiCallingPreferredCallback() {
}

NS_IMETHODIMP
ReadWifiCallingPreferredCallback::Handle(const nsAString & aName, JS::HandleValue aResult) {
    if (!aResult.isString()) {
        LOGE(_TAG(NetworkUtils::TAG), "ReadWifiCallingPreferredCallback: isString() is \"false\" !!!!!");
        return NS_ERROR_FAILURE;
    }

    JSContext *cx = nsContentUtils::GetCurrentJSContext();
    NS_ENSURE_TRUE(cx, NS_OK);

    nsAutoJSString jsString;
    if (!jsString.init(cx, aResult.toString())) {
        LOGE(_TAG(NetworkUtils::TAG), "ReadWifiCallingPreferredCallback: nsAutoJSString.init() failed !!!!!");
        return NS_ERROR_FAILURE;
    }

    string wfcPreferred;
    wfcPreferred = NS_ConvertUTF16toUTF8(jsString).get();

    if (0 == wfcPreferred.compare("cellular-preferred")) {
        ImsConnManStateInfo::setWifiCallingMode(WFC_MODE_CELLULAR_PREFERRED);
    } else if (0 == wfcPreferred.compare("cellular-only")) {
        ImsConnManStateInfo::setWifiCallingMode(WFC_MODE_CELLULAR_ONLY);
    } else if (0 == wfcPreferred.compare("wifi-preferred")) {
        ImsConnManStateInfo::setWifiCallingMode(WFC_MODE_WIFI_PREFERRED);
    } else if (0 == wfcPreferred.compare("wifi-only")) {
        ImsConnManStateInfo::setWifiCallingMode(WFC_MODE_WIFI_ONLY);
    } else {
        LOGE(_TAG(NetworkUtils::TAG), "ReadWifiCallingPreferredCallback: unknown wfcPreferred !!!!!");
    }

    LOGD(_TAG(NetworkUtils::TAG), "ReadWifiCallingPreferredCallback: wfcPreferred = \"%s\"", wfcPreferred.c_str());
    return NS_OK;
}

NS_IMETHODIMP
ReadWifiCallingPreferredCallback::HandleError(const nsAString & aErrorMessage) {
    return NS_OK;
}
/******************** class ReadWifiCallingPreferredCallback end ********************/

END_VOWIFI_NAMESPACE
