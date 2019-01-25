#ifndef __SPRD_VOWIFI_IMS_CONNMAN_STATE_INFO_H__
#define __SPRD_VOWIFI_IMS_CONNMAN_STATE_INFO_H__

#include <string>

using std::string;


BEGIN_VOWIFI_NAMESPACE

class ImsConnManStateInfo {
public:
    explicit ImsConnManStateInfo();
    virtual ~ImsConnManStateInfo();

    /**
     * Used for setting state of ImsConnMan Service attached.
     *
     * @param:
     * false: ImsConnMan Service isn't attached;
     * true : ImsConnMan Service is attached;
     */
    static void setServiceAttached(bool isAttached);

    /**
     * Return ImsConnMan Service is attached or not.
     */
    static bool isServiceAttached();

    /**
     * Used for setting state of Wifi connected.
     *
     * @param:
     * false: Wifi isn't connected;
     * true : Wifi is connected;
     */
    static void setWifiConnected(bool isConnected);

    /**
     * Return Wifi is connected or not.
     */
    static bool isWifiConnected();

    /**
     * Used for setting state of Wifi-calling on-off.
     *
     * @param:
     * false: wifi calling is disabled;
     * true : wifi calling is enabled;
     */
    static void setWifiCallingEnabled(bool isEnabled);

    /**
     * Return Wifi-calling on-off is enabled or not.
     */
    static bool isWifiCallingEnabled();

    /**
     * Used for setting Wifi-calling mode.
     *
     * @param:
     * WFC_MODE_CELLULAR_PREFERRED
     * WFC_MODE_CELLULAR_ONLY
     * WFC_MODE_WIFI_PREFERRED
     * WFC_MODE_WIFI_ONLY
     */
    static void setWifiCallingMode(int mode);

    /**
     * Return Wifi-calling mode.
     *
     * @return value:
     * WFC_MODE_CELLULAR_PREFERRED
     * WFC_MODE_CELLULAR_ONLY
     * WFC_MODE_WIFI_PREFERRED
     * WFC_MODE_WIFI_ONLY
     */
    static int getWifiCallingMode();

    /**
     * Used for setting state of Volte registered.
     *
     * @param:
     * false: volte isn't registered;
     * true : volte is registered;
     */
    static void setVolteRegistered(bool isRegistered);

    /**
     * Return Volte is registered or not.
     */
    static bool isVolteRegistered();

    /**
     * Used for setting state of Vowifi registered.
     *
     * @param:
     * false: vowifi isn't registered;
     * true : vowifi is registered;
     */
    static void setVowifiRegistered(bool isRegistered);

    /**
     * Return Vowifi is registered or not.
     */
    static bool isVowifiRegistered();

    /**
     * Return DUT is airplane mode enabled or not.
     */
    static bool isAirplaneModeEnabled();

    /**
     * Return cellular network is Lte or not.
     */
    static bool isLteCellularNetwork();

    /**
     * Used for setting call state.
     *
     * @param: (PortingConstants.h)
     * CALL_STATE_IDLE
     * CALL_STATE_DIALING
     * CALL_STATE_ALERTING
     * CALL_STATE_CONNECTED
     * CALL_STATE_HELD
     * CALL_STATE_DISCONNECTED
     * CALL_STATE_INCOMING
     */
    static void setCallState(int callState);

    /**
     * Return call state.
     *
     * @return: (PortingConstants.h)
     * CALL_STATE_IDLE
     * CALL_STATE_DIALING
     * CALL_STATE_ALERTING
     * CALL_STATE_CONNECTED
     * CALL_STATE_HELD
     * CALL_STATE_DISCONNECTED
     * CALL_STATE_INCOMING
     */
    static int getCallState();

    /**
     * Return call state is idle.
     *
     */
    static bool isIdleState();

    /**
     * Used for setting call mode.
     *
     * @param:
     * CALL_MODE_NO_CALL
     * CALL_MODE_CS_CALL
     * CALL_MODE_VOLTE_CALL
     * CALL_MODE_VOWIFI_CALL
     */
    static void setCallMode(int callMode);

    /**
     * Return call mode.
     *
     * @return:
     * MODE_NO_CALL
     * MODE_CS_CALL
     * MODE_VOLTE_CALL
     * MODE_VOWIFI_CALL
     */
    static int getCallMode();

    /**
     * Return call type.
     *
     * @return:
     * IMS_CALL_TYPE_UNKNOWN
     * IMS_CALL_TYPE_AUDIO
     * IMS_CALL_TYPE_VIDEO
     */
    static int getImsCallType();

    /**
     * Used for setting Ims call is active.
     *
     * @param:
     * false: Ims call isn't actice;
     * true : Ims call is active;
     */
    static void setImsCallActive(bool isActive);

    /**
     * Return Ims call is active or not.
     */
    static bool isImsCallActive();

    /**
     * Used for setting state of received valid Vowifi rtp packet.
     *
     * @param:
     * false: didn't received valid Vowifi rtp packet;
     * true : received valid Vowifi rtp packet;
     */
    static void setRecValidVowifiRtp(bool isReceived);

    /**
     * Return received valid Vowifi rtp packet or not.
     */
    static bool isRecValidVowifiRtp();

    /**
     * Used for setting state of start activate Volte.
     *
     * @param:
     * false: successfully or unsuccessfully activate Volte;
     * true : start activate Volte;
     */
    static void setStartActivateVolte(bool isStart);

    /**
     * Return start activate Volte or not.
     */
    static bool isStartActivateVolte();

    /**
     * Used for setting state of pending message queue.
     *
     * @param:
     * false: Service message queue operation isn't pending;
     * true : Service message queue operation is pending;
     */
    static void setMessageQueuePendingState(bool isPending);

    /**
     * Return message queue operation is pending process or not.
     */
    static bool isMessageQueuePending();

    /**
     * Used for setting pending message id.
     *
     * @param:
     * msgId: pending message id;
     */
    static void setPendingMessageId(int msgId);

    /**
     * Return pending message id.
     */
    static int getPendingMessageId();

    /**
     * Used for KaiOS.
     */
    static void setReadAirplaneModeState(bool isRead);
    static bool isReadAirplaneMode();
    static void setReceivedIccInfoChangedState(bool isReceived);
    static bool isReceivedIccInfoChanged();

private:
    static bool mIsServiceAttached;
    static bool mIsWifiConnected;
    static bool mIsWifiCallingEnabled;
    static int mWifiCallingMode;
    static bool mIsVolteRegistered;
    static bool mIsVowifiRegistered;
    static int mCallState;
    static int mCallMode;
    static bool mIsImsCallActive;
    static bool mIsRecValidVowifiRtp;
    static bool mIsStartActivateVolte;
    static bool mIsMessageQueuePending;
    static int mPendingMessageId;

    static bool mIsReadAirplaneMode;
    static bool mIsReceivedIccInfoChanged;
};

END_VOWIFI_NAMESPACE

#endif
