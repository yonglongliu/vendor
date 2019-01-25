#ifndef __SPRD_VOWIFI_IMS_CONNMAN_NETWORK_UTILS_H__
#define __SPRD_VOWIFI_IMS_CONNMAN_NETWORK_UTILS_H__

#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsISettingsService.h"
#include "nsIMobileConnectionService.h"
#include <string>

using std::string;
using std::shared_ptr;


BEGIN_VOWIFI_NAMESPACE

class AirplaneModeChangedListener;
class ReadAirplaneModeSettingsCallback;
class WifiNetworkListener;
class MobileConnectionListener;
class WifiCallingEnabledListener;
class WifiCallingPreferredListener;

class NetworkUtils {
public:
    static void createReadDatabaseTask();

    static void registerListenerAfterBoot();
    static void registerListenerAfterAttachService();
    static void unregisterListenerAfterDetachService();

    static void update(shared_ptr<ImsConnManService> &service);

    static int getCurrentWifiRssi();
    static int getCurrentLteRsrp();

    static bool isPcscfAddressReachable(const string &localAddr, const string &pcscfAddr);

public:
    static const string TAG;

private:
    static AirplaneModeChangedListener *mpApmListener;
    static WifiNetworkListener *mpWifiNetworkListener;
    static MobileConnectionListener *mpMobileConnectionListener;
    static WifiCallingEnabledListener *mpWfcEnabledListener;
    static WifiCallingPreferredListener *mpWfcPreferredListener;
};

class AirplaneModeChangedListener final : public nsIObserver {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER

    explicit AirplaneModeChangedListener();

    void registerApmChangedListener();
    void unregisterApmChangedListener();

private:
    virtual ~AirplaneModeChangedListener();

private:
    nsCOMPtr<nsIObserverService> mObserverService;
};

class ReadAirplaneModeSettingsCallback final : public nsISettingsServiceCallback {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISETTINGSSERVICECALLBACK

    explicit ReadAirplaneModeSettingsCallback();

private:
    virtual ~ReadAirplaneModeSettingsCallback();
};

class WifiNetworkListener final : public nsIObserver {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER

    explicit WifiNetworkListener();

    void update(shared_ptr<ImsConnManService> &service);

    void registerWifiNetworkListener();
    void unregisterWifiNetworkListener();

private:
    virtual ~WifiNetworkListener();

private:
    shared_ptr<ImsConnManService> mpService;
    nsCOMPtr<nsIObserverService> mObserverService;
};

class MobileConnectionListener final : public nsIMobileConnectionListener {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIMOBILECONNECTIONLISTENER

    explicit MobileConnectionListener();

    void update(shared_ptr<ImsConnManService> &service);

    void registerMobileConnectionListener();
    void unregisterMobileConnectionListener();

private:
    virtual ~MobileConnectionListener();

private:
    string mMobileNetworkType;

    shared_ptr<ImsConnManService> mpService;
    nsCOMPtr<nsIMobileConnection> mMobileConnection;
};

class WifiCallingEnabledListener final : public nsIObserver {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER

    explicit WifiCallingEnabledListener();

    void update(shared_ptr<ImsConnManService> &service);

    void registerWifiCallingEnabledListener();
    void unregisterWifiCallingEnabledListener();

private:
    virtual ~WifiCallingEnabledListener();

private:
    shared_ptr<ImsConnManService> mpService;
    nsCOMPtr<nsIObserverService> mObserverService;
};

class ReadWifiCallingEnabledCallback final : public nsISettingsServiceCallback {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISETTINGSSERVICECALLBACK

    explicit ReadWifiCallingEnabledCallback();

private:
    virtual ~ReadWifiCallingEnabledCallback();
};

class WifiCallingPreferredListener final : public nsIObserver {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER

    explicit WifiCallingPreferredListener();

    void update(shared_ptr<ImsConnManService> &service);

    void registerWifiCallingPreferredListener();
    void unregisterWifiCallingPreferredListener();

private:
    virtual ~WifiCallingPreferredListener();

private:
    shared_ptr<ImsConnManService> mpService;
    nsCOMPtr<nsIObserverService> mObserverService;
};

class ReadWifiCallingPreferredCallback final : public nsISettingsServiceCallback {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISETTINGSSERVICECALLBACK

    explicit ReadWifiCallingPreferredCallback();

private:
    virtual ~ReadWifiCallingPreferredCallback();
};

END_VOWIFI_NAMESPACE

#endif
