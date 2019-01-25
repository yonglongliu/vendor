
package com.android.sprd.telephony;

import static com.android.internal.telephony.RILConstants.GENERIC_FAILURE;
import static com.android.internal.telephony.RILConstants.RADIO_NOT_AVAILABLE;
import static com.android.internal.telephony.RILConstants.RIL_RESPONSE_ACKNOWLEDGEMENT;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_GET_SIM_CAPACITY;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_GET_DEFAULT_NAN;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_ENABLE_RAU_NOTIFY;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_SIM_GET_ATR;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_GET_HD_VOICE_STATE;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_ENABLE_COLP;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_STORE_SMS_TO_SIM;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_QUERY_SMS_STORAGE_MODE;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_SWITCH_MULTI_CALL;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_VIDEOPHONE_DIAL;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_VIDEOPHONE_CODEC;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_VIDEOPHONE_FALLBACK;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_VIDEOPHONE_STRING;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_VIDEOPHONE_LOCAL_MEDIA;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_VIDEOPHONE_CONTROL_IFRAME;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_DC_TRAFFIC_CLASS;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_ENABLE_LTE;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_ATTACH_DATACONN;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_ABORT_SEARCH_NETWORK;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_DC_FORCE_DETACH;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_SIM_POWER;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_SET_FACILITY_LOCK_FOR_USER;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_GET_SIMLOCK_REMAIN_TIMES;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_GET_SIMLOCK_STATUS;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_GET_SIMLOCK_DUMMYS;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_GET_SIMLOCK_WHITE_LIST;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_UPDATE_REAL_ECCLIST;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_GET_BAND_INFO;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_SET_BAND_INFO_MODE;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_SET_SPECIAL_RATCAP;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_QUERY_COLP;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_QUERY_COLR;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_MMI_ENTER_SIM;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_UPDATE_OPERATOR_NAME;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_SIMMGR_GET_SIM_STATUS;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_SET_XCAP_IP_ADDR;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_SEND_CMD;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_GET_SIM_STATUS;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_REATTACH;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_SET_PREFERRED_NETWORK_TYPE;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_SHUTDOWN;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_EXPLICIT_CALL_TRANSFER;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_SET_SMS_BEARER;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_SET_VOICE_DOMAIN;

import static com.android.sprd.telephony.RIConstants.RI_UNSOL_RI_CONNECTED;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_VIDEOPHONE_CODEC;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_VIDEOPHONE_DSCI;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_VIDEOPHONE_STRING;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_VIDEOPHONE_REMOTE_MEDIA;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_VIDEOPHONE_MM_RING;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_VIDEOPHONE_RELEASING;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_VIDEOPHONE_RECORD_VIDEO;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_VIDEOPHONE_MEDIA_START;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_RI_CONNECTED;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_ECC_NETWORKLIST_CHANGED;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_RAU_NOTIFY;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_CLEAR_CODE_FALLBACK;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_SIMLOCK_STATUS_CHANGED;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_SIMLOCK_SIM_EXPIRED;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_BAND_INFO;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_SET_SINGLE_PDN;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_SWITCH_PRIMARY_CARD;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_SIM_PS_REJECT;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_SIMMGR_SIM_STATUS_CHANGED;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_RADIO_CAPABILITY_CHANGED;
import java.util.ArrayList;
import java.util.Random;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicLong;

import android.content.Context;
import android.net.ConnectivityManager;
import android.os.AsyncResult;
import android.os.Bundle;
import android.os.Handler;
import android.os.HwBinder;
import android.os.Message;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.os.RemoteException;
import android.os.SystemClock;
import android.os.SystemProperties;
import android.os.WorkSource;
import android.os.Registrant;
import android.os.RegistrantList;
import android.telephony.Rlog;
import android.hardware.radio.V1_0.IccIo;
import android.hardware.radio.V1_0.SimApdu;
import android.hardware.radio.V1_0.RadioResponseInfo;
import android.hardware.radio.V1_0.RadioResponseType;
import android.hardware.radio.V1_0.RadioIndicationType;

import vendor.sprd.hardware.radio.V1_0.IExtRadio;
import android.net.ConnectivityManager;
import android.os.Message;
import android.os.RemoteException;
import android.util.SparseArray;

import com.android.internal.telephony.TelephonyProperties;

/**
 * {@hide}
 */

class RIRequest {
    static final String LOG_TAG = "RIRequest";

    //***** Class Variables
    static Random sRandom = new Random();
    static AtomicInteger sNextSerial = new AtomicInteger(0);
    private static Object sPoolSync = new Object();
    private static RIRequest sPool = null;
    private static int sPoolSize = 0;
    private static final int MAX_POOL_SIZE = 4;

    //***** Instance Variables
    int mSerial;
    int mRequest;
    Message mResult;
    RIRequest mNext;
    int mWakeLockType;
    WorkSource mWorkSource;
    String mClientId;
    // time in ms when RIL request was made
    long mStartTimeMs;

    /**
     * Retrieves a new RIRequest instance from the pool.
     *
     * @param request RI_REQUEST_*
     * @param result sent when operation completes
     * @return a RIRequest instance from the pool.
     */
    static RIRequest obtain(int request, Message result) {
        RIRequest rr = null;

        synchronized(sPoolSync) {
            if (sPool != null) {
                rr = sPool;
                sPool = rr.mNext;
                rr.mNext = null;
                sPoolSize--;
            }
        }

        if (rr == null) {
            rr = new RIRequest();
        }

        rr.mSerial = sNextSerial.getAndIncrement();

        rr.mRequest = request;
        rr.mResult = result;

        rr.mWakeLockType = RadioInteractorCore.INVALID_WAKELOCK;
        rr.mWorkSource = null;
        rr.mStartTimeMs = SystemClock.elapsedRealtime();
        if (result != null && result.getTarget() == null) {
            throw new NullPointerException("Message target must not be null");
        }

        return rr;
    }


    /**
     * Retrieves a new RIRequest instance from the pool and sets the clientId
     *
     * @param request RI_REQUEST_*
     * @param result sent when operation completes
     * @param workSource WorkSource to track the client
     * @return a RIRequest instance from the pool.
     */
    static RIRequest obtain(int request, Message result, WorkSource workSource) {
        RIRequest rr = null;

        rr = obtain(request, result);
        if(workSource != null) {
            rr.mWorkSource = workSource;
            rr.mClientId = String.valueOf(workSource.get(0)) + ":" + workSource.getName(0);
        } else {
            UtilLog.logd(LOG_TAG, "null workSource " + request);
        }

        return rr;
    }

    /**
     * Returns a RIRequest instance to the pool.
     *
     * Note: This should only be called once per use.
     */
    void release() {
        synchronized (sPoolSync) {
            if (sPoolSize < MAX_POOL_SIZE) {
                mNext = sPool;
                sPool = this;
                sPoolSize++;
                mResult = null;
                if(mWakeLockType != RadioInteractorCore.INVALID_WAKELOCK) {
                    //This is OK for some wakelock types and not others
                    if(mWakeLockType == RadioInteractorCore.FOR_WAKELOCK) {
                        UtilLog.loge(LOG_TAG, "RIRequest releasing with held wake lock: "
                                + serialString());
                    }
                }
            }
        }
    }

    private RIRequest() {
    }

    static void resetSerial() {
        // use a random so that on recovery we probably don't mix old requests
        // with new.
        sNextSerial.set(sRandom.nextInt());
    }

    String serialString() {
        //Cheesy way to do %04d
        StringBuilder sb = new StringBuilder(8);
        String sn;

        long adjustedSerial = (((long)mSerial) - Integer.MIN_VALUE)%10000;

        sn = Long.toString(adjustedSerial);

        sb.append('[');
        for (int i = 0, s = sn.length() ; i < 4 - s; i++) {
            sb.append('0');
        }

        sb.append(sn);
        sb.append(']');
        return sb.toString();
    }

    void onError(int error, Object ret) {
        CommandException ex;

        ex = CommandException.fromRilErrno(error);

        UtilLog.logd(LOG_TAG, serialString() + "< "
            + RadioInteractorCore.requestToString(mRequest)
            + " error: " + ex + " ret=" + RadioInteractorCore.retToString(mRequest, ret));

        if (mResult != null) {
            AsyncResult.forMessage(mResult, ret, ex);
            mResult.sendToTarget();
        }
    }
}

public class RadioInteractorCore{
    static final String TAG = "RadioInteractor";
    static final String RI_ACK_WAKELOCK_NAME = "RI_ACK_WL";
    static final boolean DBG = true;
    static final boolean VDBG = true;

    private static final int DEFAULT_WAKE_LOCK_TIMEOUT_MS = 60000;
    private static final int DEFAULT_ACK_WAKE_LOCK_TIMEOUT_MS = 200;

    // Variables used to differentiate ack messages from request while calling clearWakeLock()
    public static final int INVALID_WAKELOCK = -1;
    public static final int FOR_WAKELOCK = 0;
    public static final int FOR_ACK_WAKELOCK = 1;

    //***** Instance Variables

    final WakeLock mWakeLock;           // Wake lock associated with request/response
    final WakeLock mAckWakeLock;        // Wake lock associated with ack sent
    final int mWakeLockTimeout;         // Timeout associated with request/response
    final int mAckWakeLockTimeout;      // Timeout associated with ack sent
    // The number of wakelock requests currently active.  Don't release the lock
    // until dec'd to 0
    int mWakeLockCount;

    // Variables used to identify releasing of WL on wakelock timeouts
    volatile int mWlSequenceNum = 0;
    volatile int mAckWlSequenceNum = 0;

    SparseArray<RIRequest> mRequestList = new SparseArray<RIRequest>();

    volatile IExtRadio mExtRadioProxy = null;
    final AtomicLong mRadioProxyCookie = new AtomicLong(0);
    final Integer mPhoneId;
    boolean mIsMobileNetworkSupported;
    RadioResponseEx mRadioResponseEx;
    RadioIndicationEx mRadioIndicationEx;
    final RadioProxyDeathRecipient mRadioProxyDeathRecipient;
    final RilHandler mRilHandler;

    private WorkSource mRILDefaultWorkSource;
    private WorkSource mActiveWakelockWorkSource;
    boolean mHasRealSimStateChanged;

    //***** Events
    static final int EVENT_WAKE_LOCK_TIMEOUT    = 2;
    static final int EVENT_ACK_WAKE_LOCK_TIMEOUT    = 4;
    static final int EVENT_BLOCKING_RESPONSE_TIMEOUT = 5;
    static final int EVENT_RADIO_PROXY_DEAD     = 6;

  //***** Constants
    static final String[] HIDL_SERVICE_NAME = {"slot1", "slot2", "slot3"};
    static final int IRADIO_GET_SERVICE_DELAY_MILLIS = 4 * 1000;

    private int mRilVersion = -1;

    protected RegistrantList mUnsolRadiointeractorRegistrants = new RegistrantList();
    protected RegistrantList mUnsolRIConnectedRegistrants = new RegistrantList();
    protected RegistrantList mUnsolVPCodecRegistrants = new RegistrantList();
    protected RegistrantList mUnsolVPFailRegistrants = new RegistrantList();
    protected RegistrantList mUnsolVPFallBackRegistrants = new RegistrantList();
    protected RegistrantList mUnsolVPStrsRegistrants = new RegistrantList();
    protected RegistrantList mUnsolVPRemoteMediaRegistrants = new RegistrantList();
    protected RegistrantList mUnsolVPMMRingRegistrants = new RegistrantList();
    protected RegistrantList mUnsolVPRecordVideoRegistrants = new RegistrantList();
    protected RegistrantList mUnsolVPMediaStartRegistrants = new RegistrantList();
    protected RegistrantList mUnsolEccNetChangedRegistrants = new RegistrantList();
    protected RegistrantList mUnsolRauSuccessRegistrants = new RegistrantList();
    protected RegistrantList mUnsolClearCodeFallbackRegistrants = new RegistrantList();
    protected RegistrantList mSimlockStatusChangedRegistrants = new RegistrantList();
    protected RegistrantList mUnsolBandInfoRegistrants = new RegistrantList();
    protected RegistrantList mUnsolSwitchPrimaryCardRegistrants = new RegistrantList();
    protected RegistrantList mUnsolRealSimStateChangedRegistrants = new RegistrantList();
    protected RegistrantList mUnsolRadioCapabilityChangedRegistrants = new RegistrantList();
    protected RegistrantList mUnsolExpireSimdRegistrants = new RegistrantList();
    public RadioInteractorCore(Context context, Integer instanceId) {
        mPhoneId = instanceId;

        ConnectivityManager cm = (ConnectivityManager)context.getSystemService(
                Context.CONNECTIVITY_SERVICE);
        mIsMobileNetworkSupported = cm.isNetworkSupported(ConnectivityManager.TYPE_MOBILE);

        mRadioResponseEx = new RadioResponseEx(this);
        mRadioIndicationEx = new RadioIndicationEx(this);
        mRilHandler = new RilHandler();
        mRadioProxyDeathRecipient = new RadioProxyDeathRecipient();

        PowerManager pm = (PowerManager)context.getSystemService(Context.POWER_SERVICE);
        mWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, TAG);
        mWakeLock.setReferenceCounted(false);
        mAckWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, RI_ACK_WAKELOCK_NAME);
        mAckWakeLock.setReferenceCounted(false);
        mWakeLockTimeout = SystemProperties.getInt(TelephonyProperties.PROPERTY_WAKE_LOCK_TIMEOUT,
                DEFAULT_WAKE_LOCK_TIMEOUT_MS);
        mAckWakeLockTimeout = SystemProperties.getInt(
                TelephonyProperties.PROPERTY_WAKE_LOCK_TIMEOUT, DEFAULT_ACK_WAKE_LOCK_TIMEOUT_MS);
        mWakeLockCount = 0;
        mRILDefaultWorkSource = new WorkSource(context.getApplicationInfo().uid,
                context.getPackageName());

        // set radio callback; needed to set RadioIndication callback (should be done after
        // wakelock stuff is initialized above as callbacks are received on separate binder threads)
        getExtRadioProxy(null);
    }

    class RilHandler extends Handler {
        //***** Handler implementation
        @Override
        public void handleMessage(Message msg) {
            RIRequest rr;

            switch (msg.what) {
                case EVENT_WAKE_LOCK_TIMEOUT:
                    // Haven't heard back from the last request.  Assume we're
                    // not getting a response and  release the wake lock.

                    // The timer of WAKE_LOCK_TIMEOUT is reset with each
                    // new send request. So when WAKE_LOCK_TIMEOUT occurs
                    // all requests in mRequestList already waited at
                    // least DEFAULT_WAKE_LOCK_TIMEOUT_MS but no response.
                    //
                    // Note: Keep mRequestList so that delayed response
                    // can still be handled when response finally comes.

                    synchronized (mRequestList) {
                        if (msg.arg1 == mWlSequenceNum && clearWakeLock(FOR_WAKELOCK)) {
                            if (VDBG) {
                                int count = mRequestList.size();
                                UtilLog.logd(TAG, "WAKE_LOCK_TIMEOUT " +
                                        " mRequestList=" + count);
                                for (int i = 0; i < count; i++) {
                                    rr = mRequestList.valueAt(i);
                                    UtilLog.logd(TAG, i + ": [" + rr.mSerial + "] "
                                            + requestToString(rr.mRequest));
                                }
                            }
                        }
                    }
                    break;

                case EVENT_ACK_WAKE_LOCK_TIMEOUT:
                    if (msg.arg1 == mAckWlSequenceNum && clearWakeLock(FOR_ACK_WAKELOCK)) {
                        if (VDBG) {
                            UtilLog.logd(TAG, "ACK_WAKE_LOCK_TIMEOUT");
                        }
                    }
                    break;

                case EVENT_BLOCKING_RESPONSE_TIMEOUT:
                    int serial = msg.arg1;
                    rr = findAndRemoveRequestFromList(serial);
                    // If the request has already been processed, do nothing
                    if(rr == null) {
                        break;
                    }

                    //build a response if expected
                    if (rr.mResult != null) {
                        Object timeoutResponse = getResponseForTimedOutRILRequest(rr);
                        AsyncResult.forMessage( rr.mResult, timeoutResponse, null);
                        rr.mResult.sendToTarget();
                    }

                    decrementWakeLock(rr);
                    rr.release();
                    break;

                case EVENT_RADIO_PROXY_DEAD:
                    riljLog("handleMessage: EVENT_RADIO_PROXY_DEAD cookie = " + msg.obj +
                            " mRadioProxyCookie = " + mRadioProxyCookie.get());
                    if ((long) msg.obj == mRadioProxyCookie.get()) {
                        resetProxyAndRequestList();

                        // todo: rild should be back up since message was sent with a delay. this is
                        // a hack.
                        getExtRadioProxy(null);
                    }
                    break;
            }
        }
    }

    void processIndication(int indicationType) {
        if (indicationType == RadioIndicationType.UNSOLICITED_ACK_EXP) {
            sendAck();
            if (DBG) riljLog("Unsol response received; Sending ack to ril.cpp");
        } else {
            // ack is not expected to be sent back. Nothing is required to be done here.
        }
    }

    /**
     * Function to send ack and acquire related wakelock
     */
    private void sendAck() {
        // TODO: Remove rr and clean up acquireWakelock for response and ack
        RIRequest rr = RIRequest.obtain(RIL_RESPONSE_ACKNOWLEDGEMENT, null,
                mRILDefaultWorkSource);
        acquireWakeLock(rr, FOR_ACK_WAKELOCK);
        IExtRadio extRadioProxy = getExtRadioProxy(null);
        if (extRadioProxy != null) {
            try {
                extRadioProxy.responseAcknowledgement();
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "sendAck", e);
                UtilLog.loge(TAG, "sendAck: " + e);
            }
        } else {
            UtilLog.loge(TAG, "Error trying to send ack, radioProxy = null");
        }
        rr.release();
    }

    private String getWorkSourceClientId(WorkSource workSource) {
        if (workSource != null) {
            return String.valueOf(workSource.get(0)) + ":" + workSource.getName(0);
        }

        return null;
    }

    private void acquireWakeLock(RIRequest rr, int wakeLockType) {
        synchronized (rr) {
            if (rr.mWakeLockType != INVALID_WAKELOCK) {
                UtilLog.logd(TAG, "Failed to aquire wakelock for " + rr.serialString());
                return;
            }

            switch(wakeLockType) {
                case FOR_WAKELOCK:
                    synchronized (mWakeLock) {
                        mWakeLock.acquire();
                        mWakeLockCount++;
                        mWlSequenceNum++;

                        Message msg = mRilHandler.obtainMessage(EVENT_WAKE_LOCK_TIMEOUT);
                        msg.arg1 = mWlSequenceNum;
                        mRilHandler.sendMessageDelayed(msg, mWakeLockTimeout);
                    }
                    break;
                case FOR_ACK_WAKELOCK:
                    synchronized (mAckWakeLock) {
                        mAckWakeLock.acquire();
                        mAckWlSequenceNum++;

                        Message msg = mRilHandler.obtainMessage(EVENT_ACK_WAKE_LOCK_TIMEOUT);
                        msg.arg1 = mAckWlSequenceNum;
                        mRilHandler.sendMessageDelayed(msg, mAckWakeLockTimeout);
                    }
                    break;
                default: //WTF
                    UtilLog.logd(TAG, "Acquiring Invalid Wakelock type " + wakeLockType);
                    return;
            }
            rr.mWakeLockType = wakeLockType;
        }
    }

    private boolean clearWakeLock(int wakeLockType) {
        if (wakeLockType == FOR_WAKELOCK) {
            synchronized (mWakeLock) {
                if (mWakeLockCount == 0 && !mWakeLock.isHeld()) return false;
                UtilLog.logd(TAG, "NOTE: mWakeLockCount is " + mWakeLockCount
                        + "at time of clearing");
                mWakeLockCount = 0;
                mWakeLock.release();
                mActiveWakelockWorkSource = null;
                return true;
            }
        } else {
            synchronized (mAckWakeLock) {
                if (!mAckWakeLock.isHeld()) return false;
                mAckWakeLock.release();
                return true;
            }
        }
    }

    /**
     * In order to prevent calls to Telephony from waiting indefinitely
     * low-latency blocking calls will eventually time out. In the event of
     * a timeout, this function generates a response that is returned to the
     * higher layers to unblock the call. This is in lieu of a meaningful
     * response.
     * @param rr The RI Request that has timed out.
     * @return A default object, such as the one generated by a normal response
     * that is returned to the higher layers.
     **/
    private static Object getResponseForTimedOutRILRequest(RIRequest rr) {
        if (rr == null ) return null;

        Object timeoutResponse = null;
        // TODO
        return timeoutResponse;
    }

    final class RadioProxyDeathRecipient implements HwBinder.DeathRecipient {
        @Override
        public void serviceDied(long cookie) {
            // Deal with service going away
            UtilLog.logd(TAG, "serviceDied");
            // todo: temp hack to send delayed message so that rild is back up by then
            //mRilHandler.sendMessage(mRilHandler.obtainMessage(EVENT_RADIO_PROXY_DEAD, cookie));
            mRilHandler.sendMessageDelayed(
                    mRilHandler.obtainMessage(EVENT_RADIO_PROXY_DEAD, cookie),
                    IRADIO_GET_SERVICE_DELAY_MILLIS);
        }
    }

    private void resetProxyAndRequestList() {
        mExtRadioProxy = null;

        // increment the cookie so that death notification can be ignored
        mRadioProxyCookie.incrementAndGet();

        setRadioState(RadioState.RADIO_UNAVAILABLE);

        RIRequest.resetSerial();
        // Clear request list on close
        clearRequestList(RADIO_NOT_AVAILABLE, false);

        // todo: need to get service right away so setResponseFunctions() can be called for
        // unsolicited indications. getService() is not a blocking call, so it doesn't help to call
        // it here. Current hack is to call getService() on death notification after a delay.
    }

    enum RadioState {
        RADIO_OFF,         /* Radio explicitly powered off (eg CFUN=0) */
        RADIO_UNAVAILABLE, /* Radio unavailable (eg, resetting or not booted) */
        RADIO_ON;          /* Radio is on */

        public boolean isOn() /* and available...*/ {
            return this == RADIO_ON;
        }

        public boolean isAvailable() {
            return this != RADIO_UNAVAILABLE;
        }
    }

    enum SuppService {
        UNKNOWN, SWITCH, SEPARATE, TRANSFER, CONFERENCE, REJECT, HANGUP, RESUME;
    }

    protected void setRadioState(RadioState newState) {

    }

    public int getPhoneId() {
        return mPhoneId;
    }

    /**
     * Release each request in mRequestList then clear the list
     * @param error is the RIL_Errno sent back
     * @param loggable true means to print all requests in mRequestList
     */
    private void clearRequestList(int error, boolean loggable) {
        RIRequest rr;
        synchronized (mRequestList) {
            int count = mRequestList.size();
            if (VDBG && loggable) {
                UtilLog.logd(TAG, "clearRequestList " + " mWakeLockCount="
                        + mWakeLockCount + " mRequestList=" + count);
            }

            for (int i = 0; i < count; i++) {
                rr = mRequestList.valueAt(i);
                if (VDBG && loggable) {
                    UtilLog.logd(TAG, i + ": [" + rr.mSerial + "] "
                            + requestToString(rr.mRequest));
                }
                rr.onError(error, null);
                decrementWakeLock(rr);
                rr.release();
            }
            mRequestList.clear();
        }
    }

    private IExtRadio getExtRadioProxy(Message result) {
        if (!mIsMobileNetworkSupported) {
            if (VDBG) UtilLog.logd(TAG, "getExtRadioProxy: Not calling getService(): wifi-only");
            if (result != null) {
                AsyncResult.forMessage(result, null,
                        CommandException.fromRilErrno(RADIO_NOT_AVAILABLE));
                result.sendToTarget();
            }
            return null;
        }

        if (mExtRadioProxy != null) {
            return mExtRadioProxy;
        }

        try {
            mExtRadioProxy = IExtRadio.getService(HIDL_SERVICE_NAME[mPhoneId == null ? 0 : mPhoneId]);
            if (mExtRadioProxy != null) {
                mExtRadioProxy.linkToDeath(mRadioProxyDeathRecipient,
                        mRadioProxyCookie.incrementAndGet());
                mExtRadioProxy.setExtResponseFunctions(mRadioResponseEx, mRadioIndicationEx);
            } else {
                UtilLog.loge(TAG, "getExtRadioProxy: mExtRadioProxy == null");
            }
        } catch (RemoteException | RuntimeException e) {
            mExtRadioProxy = null;
            UtilLog.loge(TAG, "ExtRadioProxy getService/setResponseFunctions: " + e);
        }

        if (mExtRadioProxy == null) {
            if (result != null) {
                AsyncResult.forMessage(result, null,
                        CommandException.fromRilErrno(RADIO_NOT_AVAILABLE));
                result.sendToTarget();
            }

            // if service is not up, treat it like death notification to try to get service again
            mRilHandler.sendMessageDelayed(
                    mRilHandler.obtainMessage(EVENT_RADIO_PROXY_DEAD,
                            mRadioProxyCookie.incrementAndGet()),
                    IRADIO_GET_SERVICE_DELAY_MILLIS);
        }

        return mExtRadioProxy;
    }

    private void addRequest(RIRequest rr) {
        acquireWakeLock(rr, FOR_WAKELOCK);
        synchronized (mRequestList) {
            rr.mStartTimeMs = SystemClock.elapsedRealtime();
            mRequestList.append(rr.mSerial, rr);
        }
    }

    private RIRequest obtainRequest(int request, Message result, WorkSource workSource) {
        RIRequest rr = RIRequest.obtain(request, result, workSource);
        addRequest(rr);
        return rr;
    }

    private void handleRadioProxyExceptionForRR(RIRequest rr, String caller, Exception e) {
        UtilLog.loge(TAG, caller + ": " + e);
        resetProxyAndRequestList();

        // service most likely died, handle exception like death notification to try to get service
        // again
        mRilHandler.sendMessageDelayed(
                mRilHandler.obtainMessage(EVENT_RADIO_PROXY_DEAD,
                        mRadioProxyCookie.incrementAndGet()),
                IRADIO_GET_SERVICE_DELAY_MILLIS);
    }

    public void videoPhoneDial(String address, String sub_address, int clirMode, Message result) {
        IExtRadio extRadioProxy = getExtRadioProxy(result);
        if (extRadioProxy != null) {
            RIRequest rr = obtainRequest(RI_REQUEST_VIDEOPHONE_DIAL, result,
                    mRILDefaultWorkSource);

            if (DBG) {
                UtilLog.logd(TAG, rr.serialString() + "> " + requestToString(rr.mRequest)
                        + " address = " + address + " sub_address = " + sub_address
                        + " clirMode = " + clirMode);
            }
            vendor.sprd.hardware.radio.V1_0.VideoPhoneDial vpDial =
                    new vendor.sprd.hardware.radio.V1_0.VideoPhoneDial();
            vpDial.address = convertNullToEmptyString(address);
            vpDial.subAddress = convertNullToEmptyString(sub_address);
            vpDial.clir = clirMode;

            try {
                extRadioProxy.videoPhoneDial(rr.mSerial, vpDial);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "videoPhoneDial", e);
            }
        }
    }

    public void videoPhoneCodec(int type, Bundle param, Message result) {
        IExtRadio extRadioProxy = getExtRadioProxy(result);
        if (extRadioProxy != null) {
            RIRequest rr = obtainRequest(RI_REQUEST_VIDEOPHONE_CODEC, result,
                    mRILDefaultWorkSource);

            if (DBG) {
                UtilLog.logd(TAG, rr.serialString() + "> " + requestToString(rr.mRequest)
                        + " type = " + type + " param = " + param);
            }
            vendor.sprd.hardware.radio.V1_0.VideoPhoneCodec vpCodec =
                    new vendor.sprd.hardware.radio.V1_0.VideoPhoneCodec();
            vpCodec.type = type;

            try {
                extRadioProxy.videoPhoneCodec(rr.mSerial, vpCodec);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "videoPhoneCodec", e);
            }
        }
    }

    public void videoPhoneFallback(Message result) {
        IExtRadio extRadioProxy = getExtRadioProxy(result);
        if (extRadioProxy != null) {
            RIRequest rr = obtainRequest(RI_REQUEST_VIDEOPHONE_FALLBACK, result,
                    mRILDefaultWorkSource);

            if (DBG) {
                UtilLog.logd(TAG, rr.serialString() + "> " + requestToString(rr.mRequest));
            }

            try {
                extRadioProxy.videoPhoneFallback(rr.mSerial);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "videoPhoneFallback", e);
            }
        }
    }

    public void videoPhoneString(String str, Message result) {
        IExtRadio extRadioProxy = getExtRadioProxy(result);
        if (extRadioProxy != null) {
            RIRequest rr = obtainRequest(RI_REQUEST_VIDEOPHONE_STRING, result,
                    mRILDefaultWorkSource);

            if (DBG) {
                UtilLog.logd(TAG, rr.serialString() + "> " + requestToString(rr.mRequest)
                        + " str = " + str);
            }

            try {
                extRadioProxy.videoPhoneString(rr.mSerial, convertNullToEmptyString(str));
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "videoPhoneString", e);
            }
        }
    }

    public void videoPhoneLocalMedia(int datatype, int sw, boolean bReplaceImg, Message result) {
        IExtRadio extRadioProxy = getExtRadioProxy(result);
        if (extRadioProxy != null) {
            RIRequest rr = obtainRequest(RI_REQUEST_VIDEOPHONE_LOCAL_MEDIA, result,
                    mRILDefaultWorkSource);

            if (DBG) {
                UtilLog.logd(TAG, rr.serialString() + "> " + requestToString(rr.mRequest)
                        + " datatype = " + datatype + " sw = " + sw + " bReplaceImg = " + bReplaceImg);
            }

            try {
                extRadioProxy.videoPhoneLocalMedia(rr.mSerial, datatype, sw, bReplaceImg);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "videoPhoneLocalMedia", e);
            }
        }
    }

    public void videoPhoneControlIFrame(boolean isIFrame, boolean needIFrame, Message result) {
        IExtRadio extRadioProxy = getExtRadioProxy(result);
        if (extRadioProxy != null) {
            RIRequest rr = obtainRequest(RI_REQUEST_VIDEOPHONE_CONTROL_IFRAME, result,
                    mRILDefaultWorkSource);

            if (DBG) {
                UtilLog.logd(TAG, rr.serialString() + "> " + requestToString(rr.mRequest)
                        + " isIFrame = " + isIFrame + " needIFrame = " + needIFrame);
            }

            try {
                extRadioProxy.videoPhoneControlIFrame(rr.mSerial, isIFrame, needIFrame);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "videoPhoneControlIFrame", e);
            }
        }
    }

    public void switchMultiCall(int mode, Message result) {
        IExtRadio extRadioProxy = getExtRadioProxy(result);
        if (extRadioProxy != null) {
            RIRequest rr = obtainRequest(RI_REQUEST_SWITCH_MULTI_CALL, result,
                    mRILDefaultWorkSource);

            if (DBG) {
                UtilLog.logd(TAG, rr.serialString() + "> " + requestToString(rr.mRequest)
                        + " mode = " + mode);
            }

            try {
                extRadioProxy.switchMultiCall(rr.mSerial, mode);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "switchMultiCall", e);
            }
        }
    }

    public void setTrafficClass(int type, Message result) {
        IExtRadio extRadioProxy = getExtRadioProxy(result);
        if (extRadioProxy != null) {
            RIRequest rr = obtainRequest(RI_REQUEST_DC_TRAFFIC_CLASS, result,
                    mRILDefaultWorkSource);

            if (DBG) {
                UtilLog.logd(TAG, rr.serialString() + "> " + requestToString(rr.mRequest)
                        + " type = " + type);
            }

            try {
                extRadioProxy.setTrafficClass(rr.mSerial, type);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "setTrafficClass", e);
            }
        }
    }

    public void enableLTE(boolean enabled, Message result) {
        IExtRadio extRadioProxy = getExtRadioProxy(result);
        if (extRadioProxy != null) {
            RIRequest rr = obtainRequest(RI_REQUEST_ENABLE_LTE, result,
                    mRILDefaultWorkSource);

            if (DBG) {
                UtilLog.logd(TAG, rr.serialString() + "> " + requestToString(rr.mRequest)
                        + " enabled = " + enabled);
            }

            try {
                extRadioProxy.enableLTE(rr.mSerial, enabled);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "enableLTE", e);
            }
        }
    }

    public void attachData(boolean enabled, Message result) {
        IExtRadio extRadioProxy = getExtRadioProxy(result);
        if (extRadioProxy != null) {
            RIRequest rr = obtainRequest(RI_REQUEST_ATTACH_DATACONN, result,
                    mRILDefaultWorkSource);

            if (DBG) {
                UtilLog.logd(TAG, rr.serialString() + "> " + requestToString(rr.mRequest)
                        + " enabled = " + enabled);
            }

            try {
                extRadioProxy.attachData(rr.mSerial, enabled);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "attachData", e);
            }
        }
    }

    public void stopQueryNetwork(Message result) {
        IExtRadio extRadioProxy = getExtRadioProxy(result);
        if (extRadioProxy != null) {
            RIRequest rr = obtainRequest(RI_REQUEST_ABORT_SEARCH_NETWORK, result,
                    mRILDefaultWorkSource);

            if (DBG) {
                UtilLog.logd(TAG, rr.serialString() + "> " + requestToString(rr.mRequest));
            }

            try {
                extRadioProxy.stopQueryNetwork(rr.mSerial);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "stopQueryNetwork", e);
            }
        }
    }

    public void forceDeatch(Message result) {
        IExtRadio extRadioProxy = getExtRadioProxy(result);
        if (extRadioProxy != null) {
            RIRequest rr = obtainRequest(RI_REQUEST_DC_FORCE_DETACH, result,
                    mRILDefaultWorkSource);

            if (DBG) {
                UtilLog.logd(TAG, rr.serialString() + "> " + requestToString(rr.mRequest));
            }

            try {
                extRadioProxy.forceDeatch(rr.mSerial);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "forceDeatch", e);
            }
        }
    }

    public void getHDVoiceState(Message result) {
        IExtRadio extRadioProxy = getExtRadioProxy(result);
        if (extRadioProxy != null) {
            RIRequest rr = obtainRequest(RI_REQUEST_GET_HD_VOICE_STATE, result,
                    mRILDefaultWorkSource);

            if (DBG) {
                UtilLog.logd(TAG, rr.serialString() + "> " + requestToString(rr.mRequest));
            }

            try {
                extRadioProxy.getHDVoiceState(rr.mSerial);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "getHDVoiceState", e);
            }
        }
    }

    public void simmgrSimPower(boolean enabled, Message result) {
        IExtRadio extRadioProxy = getExtRadioProxy(result);
        if (extRadioProxy != null) {
            RIRequest rr = obtainRequest(RI_REQUEST_SIM_POWER, result,
                    mRILDefaultWorkSource);

            if (DBG) {
                UtilLog.logd(TAG, rr.serialString() + "> " + requestToString(rr.mRequest)
                        + " enabled = " + enabled);
            }

            try {
                extRadioProxy.simmgrSimPower(rr.mSerial, enabled);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "simmgrSimPower", e);
            }
        }
    }

    public void enableRauNotify(Message result) {
        IExtRadio extRadioProxy = getExtRadioProxy(result);
        if (extRadioProxy != null) {
            RIRequest rr = obtainRequest(RI_REQUEST_ENABLE_RAU_NOTIFY, result,
                    mRILDefaultWorkSource);

            if (DBG) {
                UtilLog.logd(TAG, rr.serialString() + "> " + requestToString(rr.mRequest));
            }

            try {
                extRadioProxy.enableRauNotify(rr.mSerial);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "enableRauNotify", e);
            }
        }
    }

    public void setCOLP(boolean enabled, Message result) {
        IExtRadio extRadioProxy = getExtRadioProxy(result);
        if (extRadioProxy != null) {
            RIRequest rr = obtainRequest(RI_REQUEST_ENABLE_COLP, result,
                    mRILDefaultWorkSource);

            if (DBG) {
                UtilLog.logd(TAG, rr.serialString() + "> " + requestToString(rr.mRequest)
                        + " enabled = " + enabled);
            }

            try {
                extRadioProxy.setCOLP(rr.mSerial, enabled);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "setCOLP", e);
            }
        }
    }

    public void getDefaultNAN(Message result) {
        IExtRadio extRadioProxy = getExtRadioProxy(result);
        if (extRadioProxy != null) {
            RIRequest rr = obtainRequest(RI_REQUEST_GET_DEFAULT_NAN, result,
                    mRILDefaultWorkSource);

            if (DBG) {
                UtilLog.logd(TAG, rr.serialString() + "> " + requestToString(rr.mRequest));
            }

            try {
                extRadioProxy.getDefaultNAN(rr.mSerial);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "getDefaultNAN", e);
            }
        }
    }

    public void simGetAtr(Message result) {
        IExtRadio extRadioProxy = getExtRadioProxy(result);
        if (extRadioProxy != null) {
            RIRequest rr = obtainRequest(RI_REQUEST_SIM_GET_ATR, result,
                    mRILDefaultWorkSource);

            if (DBG) {
                UtilLog.logd(TAG, rr.serialString() + "> " + requestToString(rr.mRequest));
            }

            try {
                extRadioProxy.simGetAtr(rr.mSerial);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "simGetAtr", e);
            }
        }
    }

    public void getSimCapacity(Message result) {
        IExtRadio extRadioProxy = getExtRadioProxy(result);
        if (extRadioProxy != null) {
            RIRequest rr = obtainRequest(RI_REQUEST_GET_SIM_CAPACITY, result,
                    mRILDefaultWorkSource);

            if (DBG) {
                UtilLog.logd(TAG, rr.serialString() + "> " + requestToString(rr.mRequest));
            }

            try {
                extRadioProxy.getSimCapacity(rr.mSerial);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "getSimCapacity", e);
            }
        }
    }

    public void storeSmsToSim(boolean enabled, Message result) {
        IExtRadio extRadioProxy = getExtRadioProxy(result);
        if (extRadioProxy != null) {
            RIRequest rr = obtainRequest(RI_REQUEST_STORE_SMS_TO_SIM, result,
                    mRILDefaultWorkSource);

            if (DBG) {
                UtilLog.logd(TAG, rr.serialString() + "> " + requestToString(rr.mRequest)
                        + " enabled = " + enabled);
            }

            try {
                extRadioProxy.storeSmsToSim(rr.mSerial, enabled);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "storeSmsToSim", e);
            }
        }
    }

    public void querySmsStorageMode(Message result) {
        IExtRadio extRadioProxy = getExtRadioProxy(result);
        if (extRadioProxy != null) {
            RIRequest rr = obtainRequest(RI_REQUEST_QUERY_SMS_STORAGE_MODE, result,
                    mRILDefaultWorkSource);

            if (DBG) {
                UtilLog.logd(TAG, rr.serialString() + "> " + requestToString(rr.mRequest));
            }

            try {
                extRadioProxy.querySmsStorageMode(rr.mSerial);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "querySmsStorageMode", e);
            }
        }
    }

    public void getSimlockRemaintimes(int type, Message result) {
        IExtRadio extRadioProxy = getExtRadioProxy(result);
        if (extRadioProxy != null) {
            RIRequest rr = obtainRequest(RI_REQUEST_GET_SIMLOCK_REMAIN_TIMES, result,
                    mRILDefaultWorkSource);

            if (DBG) {
                UtilLog.logd(TAG, rr.serialString() + "> " + requestToString(rr.mRequest)
                        + " type = " + type);
            }

            try {
                extRadioProxy.getSimlockRemaintimes(rr.mSerial, type);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "getSimlockRemaintimes", e);
            }
        }
    }

    public void setFacilityLockForUser(String facility, boolean lockState, Message result) {
        IExtRadio extRadioProxy = getExtRadioProxy(result);
        if (extRadioProxy != null) {
            RIRequest rr = obtainRequest(RI_REQUEST_SET_FACILITY_LOCK_FOR_USER, result,
                    mRILDefaultWorkSource);

            if (DBG) {
                UtilLog.logd(TAG, rr.serialString() + "> " + requestToString(rr.mRequest)
                        + " facility = " + facility + " lockState = " + lockState);
            }

            try {
                extRadioProxy.setFacilityLockForUser(rr.mSerial,
                        convertNullToEmptyString(facility), lockState);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "setFacilityLockForUser", e);
            }
        }
    }

    public void getSimlockStatus(int type, Message result) {
        IExtRadio extRadioProxy = getExtRadioProxy(result);
        if (extRadioProxy != null) {
            RIRequest rr = obtainRequest(RI_REQUEST_GET_SIMLOCK_STATUS, result,
                    mRILDefaultWorkSource);

            if (DBG) {
                UtilLog.logd(TAG, rr.serialString() + "> " + requestToString(rr.mRequest)
                        + " type = " + type);
            }

            try {
                extRadioProxy.getSimlockStatus(rr.mSerial, type);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "getSimlockStatus", e);
            }
        }
    }

    public void getSimlockDummys(Message result) {
        IExtRadio extRadioProxy = getExtRadioProxy(result);
        if (extRadioProxy != null) {
            RIRequest rr = obtainRequest(RI_REQUEST_GET_SIMLOCK_DUMMYS, result,
                    mRILDefaultWorkSource);

            if (DBG) {
                UtilLog.logd(TAG, rr.serialString() + "> " + requestToString(rr.mRequest));
            }

            try {
                extRadioProxy.getSimlockDummys(rr.mSerial);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "getSimlockDummys", e);
            }
        }
    }

    public void getSimlockWhitelist(int type, Message result) {
        IExtRadio extRadioProxy = getExtRadioProxy(result);
        if (extRadioProxy != null) {
            RIRequest rr = obtainRequest(RI_REQUEST_GET_SIMLOCK_WHITE_LIST, result,
                    mRILDefaultWorkSource);

            if (DBG) {
                UtilLog.logd(TAG, rr.serialString() + "> " + requestToString(rr.mRequest)
                        + " type = " + type);
            }

            try {
                extRadioProxy.getSimlockWhitelist(rr.mSerial, type);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "getSimlockWhitelist", e);
            }
        }
    }

    public void updateEcclist(String ecclist, Message result) {
        IExtRadio extRadioProxy = getExtRadioProxy(result);
        if (extRadioProxy != null) {
            RIRequest rr = obtainRequest(RI_REQUEST_UPDATE_REAL_ECCLIST, result,
                    mRILDefaultWorkSource);

            if (DBG) {
                UtilLog.logd(TAG, rr.serialString() + "> " + requestToString(rr.mRequest)
                        + " ecclist = " + ecclist);
            }
            try {
                extRadioProxy.updateEcclist(rr.mSerial, convertNullToEmptyString(ecclist));
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "updateEcclist", e);

            }
        }
    }

    public void getBandInfo(Message result) {
        IExtRadio extRadioProxy = getExtRadioProxy(result);
        if (extRadioProxy != null) {
            RIRequest rr = obtainRequest(RI_REQUEST_GET_BAND_INFO, result,
                    mRILDefaultWorkSource);

            if (DBG) {
                UtilLog.logd(TAG, rr.serialString() + "> " + requestToString(rr.mRequest));
            }

            try {
                extRadioProxy.getBandInfo(rr.mSerial);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "getBandInfo", e);
            }
        }
    }

    public void setBandInfoMode(int type, Message result) {
        IExtRadio extRadioProxy = getExtRadioProxy(result);
        if (extRadioProxy != null) {
            RIRequest rr = obtainRequest(RI_REQUEST_SET_BAND_INFO_MODE, result,
                    mRILDefaultWorkSource);

            if (DBG) {
                UtilLog.logd(TAG, rr.serialString() + "> " + requestToString(rr.mRequest)
                        + " type = " + type);
            }

            try {
                extRadioProxy.setBandInfoMode(rr.mSerial, type);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "setBandInfoMode", e);
            }
        }
    }

    public void setSinglePDN(boolean isSinglePDN, Message result) {
        IExtRadio extRadioProxy = getExtRadioProxy(result);
        if (extRadioProxy != null) {
            RIRequest rr = obtainRequest(RI_REQUEST_SET_SINGLE_PDN, result,
                    mRILDefaultWorkSource);

            if (DBG) {
                UtilLog.logd(TAG, rr.serialString() + "> " + requestToString(rr.mRequest)
                        + " isSinglePDN = " + isSinglePDN);
            }

            try {
                extRadioProxy.setSinglePDN(rr.mSerial, isSinglePDN);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "setSinglePDN", e);
            }
        }
    }

    public void setSpecialRatcap(int type, Message result) {
        IExtRadio extRadioProxy = getExtRadioProxy(result);
        if (extRadioProxy != null) {
            RIRequest rr = obtainRequest(RI_REQUEST_SET_SPECIAL_RATCAP, result,
                    mRILDefaultWorkSource);

            if (DBG) {
                UtilLog.logd(TAG, rr.serialString() + "> " + requestToString(rr.mRequest)
                        + " type = " + type);
            }

            try {
                extRadioProxy.setSpecialRatcap(rr.mSerial, type);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "setSpecialRatcap", e);
            }
        }
    }

    public void queryColp(Message result) {
        IExtRadio extRadioProxy = getExtRadioProxy(result);
        if (extRadioProxy != null) {
            RIRequest rr = obtainRequest(RI_REQUEST_QUERY_COLP, result,
                    mRILDefaultWorkSource);

            if (DBG) {
                UtilLog.logd(TAG, rr.serialString() + "> " + requestToString(rr.mRequest));
            }

            try {
                extRadioProxy.queryColp(rr.mSerial);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "queryColp", e);
            }
        }
    }

    public void queryColr(Message result) {
        IExtRadio extRadioProxy = getExtRadioProxy(result);
        if (extRadioProxy != null) {
            RIRequest rr = obtainRequest(RI_REQUEST_QUERY_COLR, result,
                    mRILDefaultWorkSource);

            if (DBG) {
                UtilLog.logd(TAG, rr.serialString() + "> " + requestToString(rr.mRequest));
            }

            try {
                extRadioProxy.queryColr(rr.mSerial);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "queryColr", e);
            }
        }
    }

    public void mmiEnterSim(String data, Message result) {
        IExtRadio extRadioProxy = getExtRadioProxy(result);
        if (extRadioProxy != null) {
            RIRequest rr = obtainRequest(RI_REQUEST_MMI_ENTER_SIM, result,
                    mRILDefaultWorkSource);

            if (DBG) {
                UtilLog.logd(TAG, rr.serialString() + "> " + requestToString(rr.mRequest)
                        + " data = " + data);
            }

            try {
                extRadioProxy.mmiEnterSim(rr.mSerial, convertNullToEmptyString(data));
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "mmiEnterSim", e);
            }
        }
    }

    public void updateOperatorName(String plmn, Message result) {
        IExtRadio extRadioProxy = getExtRadioProxy(result);
        if (extRadioProxy != null) {
            RIRequest rr = obtainRequest(RI_REQUEST_UPDATE_OPERATOR_NAME, result,
                    mRILDefaultWorkSource);

            if (DBG) {
                UtilLog.logd(TAG, rr.serialString() + "> " + requestToString(rr.mRequest)
                        + " plmn = " + plmn);
            }

            try {
                extRadioProxy.updateOperatorName(rr.mSerial, convertNullToEmptyString(plmn));
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "updateOperatorName", e);
            }
        }
    }

    public void simmgrGetSimStatus(Message result) {
        IExtRadio extRadioProxy = getExtRadioProxy(result);
        if (extRadioProxy != null) {
            RIRequest rr = obtainRequest(RI_REQUEST_SIMMGR_GET_SIM_STATUS, result,
                    mRILDefaultWorkSource);

            if (DBG) {
                UtilLog.logd(TAG, rr.serialString() + "> " + requestToString(rr.mRequest));
            }

            try {
                extRadioProxy.simmgrGetSimStatus(rr.mSerial);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "simmgrGetSimStatus", e);
            }
        }
    }

    public void setXcapIPAddress(String cid, String ipv4Addr, String ipv6Addr, Message result) {
        IExtRadio extRadioProxy = getExtRadioProxy(result);
        if (extRadioProxy != null) {
            RIRequest rr = obtainRequest(RI_REQUEST_SET_XCAP_IP_ADDR, result,
                    mRILDefaultWorkSource);

            if (DBG) {
                UtilLog.logd(TAG, rr.serialString() + "> " + requestToString(rr.mRequest)
                        + " cid = " + cid + " ipv4Addr = " + ipv4Addr + " ipv6Addr = " + ipv6Addr);
            }

            try {
                extRadioProxy.setXcapIPAddress(rr.mSerial,
                        convertNullToEmptyString(cid),
                        convertNullToEmptyString(ipv4Addr),
                        convertNullToEmptyString(ipv6Addr));
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "setXcapIPAddress", e);
            }
        }
    }

    public void sendCmdAsync(String cmd, Message result) {
        IExtRadio extRadioProxy = getExtRadioProxy(result);
        if (extRadioProxy != null) {
            RIRequest rr = obtainRequest(RI_REQUEST_SEND_CMD, result,
                    mRILDefaultWorkSource);

            if (DBG) {
                UtilLog.logd(TAG, rr.serialString() + "> " + requestToString(rr.mRequest)
                        + " cmd = " + cmd);
            }

            try {
                extRadioProxy.sendCmdAsync(rr.mSerial, convertNullToEmptyString(cmd));
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "sendCmdAsync", e);
            }
        }
    }

    public void reAttach(Message result) {
        IExtRadio extRadioProxy = getExtRadioProxy(result);
        if (extRadioProxy != null) {
            RIRequest rr = obtainRequest(RI_REQUEST_REATTACH, result,
                    mRILDefaultWorkSource);

            if (DBG) {
                UtilLog.logd(TAG, rr.serialString() + "> " + requestToString(rr.mRequest));
            }

            try {
                extRadioProxy.reAttach(rr.mSerial);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "reAttach", e);
            }
        }
    }

    public void explicitCallTransfer(Message result) {
        IExtRadio extRadioProxy = getExtRadioProxy(result);
        if (extRadioProxy != null) {
            RIRequest rr = obtainRequest(RI_REQUEST_EXPLICIT_CALL_TRANSFER, result,
                    mRILDefaultWorkSource);

            if (DBG) {
                UtilLog.logd(TAG, rr.serialString() + "> " + requestToString(rr.mRequest));
            }

            try {
                extRadioProxy.explicitCallTransferExt(rr.mSerial);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "explicitCallTransfer", e);
            }
        }
    }

    public void getIccCardStatus(Message result) {
        IExtRadio extRadioProxy = getExtRadioProxy(result);
        if (extRadioProxy != null) {
            RIRequest rr = obtainRequest(RI_REQUEST_GET_SIM_STATUS, result,
                    mRILDefaultWorkSource);

            if (DBG) {
                UtilLog.logd(TAG, rr.serialString() + "> " + requestToString(rr.mRequest));
            }

            try {
                extRadioProxy.getIccCardStatusExt(rr.mSerial);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "getIccCardStatus", e);
            }
        }
    }

    public void setPreferredNetworkType(int networkType , Message result) {
        IExtRadio extRadioProxy = getExtRadioProxy(result);
        if (extRadioProxy != null) {
            RIRequest rr = obtainRequest(RI_REQUEST_SET_PREFERRED_NETWORK_TYPE, result,
                    mRILDefaultWorkSource);

            if (DBG) {
                UtilLog.logd(TAG, rr.serialString() + "> " + requestToString(rr.mRequest)
                        + " networkType = " + networkType);
            }

            try {
                extRadioProxy.setPreferredNetworkTypeExt(rr.mSerial, networkType);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "setPreferredNetworkType", e);
            }
        }
    }

    public void requestShutdown(Message result) {
        IExtRadio extRadioProxy = getExtRadioProxy(result);
        if (extRadioProxy != null) {
            RIRequest rr = obtainRequest(RI_REQUEST_SHUTDOWN, result,
                    mRILDefaultWorkSource);

            if (DBG) {
                UtilLog.logd(TAG, rr.serialString() + "> " + requestToString(rr.mRequest));
            }

            try {
                extRadioProxy.requestShutdownExt(rr.mSerial);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "requestShutdown", e);
            }
        }
    }

    public void setSmsBearer(int type, Message result) {
        IExtRadio extRadioProxy = getExtRadioProxy(result);
        if (extRadioProxy != null) {
            RIRequest rr = obtainRequest(RI_REQUEST_SET_SMS_BEARER, result,
                    mRILDefaultWorkSource);

            if (DBG) {
                UtilLog.logd(TAG, rr.serialString() + "> " + requestToString(rr.mRequest)
                        + " type = " + type);
            }

            try {
                extRadioProxy.setSmsBearer(rr.mSerial, type);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "setSmsBearer", e);
            }
        }
    }

    public void setVoiceDomain(int type, Message result) {
        IExtRadio extRadioProxy = getExtRadioProxy(result);
        if (extRadioProxy != null) {
            RIRequest rr = obtainRequest(RI_REQUEST_SET_VOICE_DOMAIN, result,
                    mRILDefaultWorkSource);

            if (DBG) {
                UtilLog.logd(TAG, rr.serialString() + "> " + requestToString(rr.mRequest)
                        + " type = " + type);
            }

            try {
                extRadioProxy.setVoiceDomain(rr.mSerial, type);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "setVoiceDomain", e);
            }
        }
    }

    void notifyRegistrantsRilConnectionChanged(int rilVer) {
        mRilVersion = rilVer;
        if (mUnsolRIConnectedRegistrants != null) {
            mUnsolRIConnectedRegistrants.notifyRegistrants(
                                new AsyncResult (null, new Integer(rilVer), null));
        }
    }

    public void registerForUnsolRadioInteractor(Handler h, int what, Object obj) {

        Registrant r = new Registrant(h, what, obj);
        mUnsolRadiointeractorRegistrants.add(r);
    }

    public void unregisterForUnsolRadioInteractor(Handler h) {
        mUnsolRadiointeractorRegistrants.remove(h);
    }

    public void registerForUnsolRiConnected(Handler h, int what, Object obj) {

        Registrant r = new Registrant(h, what, obj);
        mUnsolRIConnectedRegistrants.add(r);
        mUnsolRIConnectedRegistrants
        .notifyRegistrants(new AsyncResult(null, new Integer(mRilVersion), null));
    }

    public void unregisterForUnsolRiConnected(Handler h) {
        mUnsolRIConnectedRegistrants.remove(h);
    }

    public void registerForsetOnVPCodec(Handler h, int what, Object obj) {
        Registrant r = new Registrant(h, what, obj);
        mUnsolVPCodecRegistrants.add(r);
    }

    public void unregisterForsetOnVPCodec(Handler h) {
        mUnsolVPCodecRegistrants.remove(h);
    }

    public void registerForsetOnVPFallBack(Handler h, int what, Object obj) {
        Registrant r = new Registrant(h, what, obj);
        mUnsolVPFallBackRegistrants.add(r);
    }

    public void unregisterForsetOnVPFallBack(Handler h) {
        mUnsolVPFallBackRegistrants.remove(h);
    }

    public void registerForsetOnVPString(Handler h, int what, Object obj) {
        Registrant r = new Registrant(h, what, obj);
        mUnsolVPStrsRegistrants.add(r);
    }

    public void unregisterForsetOnVPString(Handler h) {
        mUnsolVPStrsRegistrants.remove(h);
    }

    public void registerForsetOnVPRemoteMedia(Handler h, int what, Object obj) {
        Registrant r = new Registrant(h, what, obj);
        mUnsolVPRemoteMediaRegistrants.add(r);
    }

    public void unregisterForsetOnVPRemoteMedia(Handler h) {
        mUnsolVPRemoteMediaRegistrants.remove(h);
    }

    public void registerForsetOnVPMMRing(Handler h, int what, Object obj) {
        Registrant r = new Registrant(h, what, obj);
        mUnsolVPMMRingRegistrants.add(r);
    }

    public void unregisterForsetOnVPMMRing(Handler h) {
        mUnsolVPMMRingRegistrants.remove(h);
    }

    public void registerForsetOnVPFail(Handler h, int what, Object obj) {
        Registrant r = new Registrant(h, what, obj);
        mUnsolVPFailRegistrants.add(r);
    }

    public void unregisterForsetOnVPFail(Handler h) {
        mUnsolVPFailRegistrants.remove(h);
    }

    public void registerForsetOnVPRecordVideo(Handler h, int what, Object obj) {
        Registrant r = new Registrant(h, what, obj);
        mUnsolVPRecordVideoRegistrants.add(r);
    }

    public void unregisterForsetOnVPRecordVideo(Handler h) {
        mUnsolVPRecordVideoRegistrants.remove(h);
    }

    public void registerForsetOnVPMediaStart(Handler h, int what, Object obj) {
        Registrant r = new Registrant(h, what, obj);
        mUnsolVPMediaStartRegistrants.add(r);
    }

    public void unregisterForsetOnVPMediaStart(Handler h) {
        mUnsolVPMediaStartRegistrants.remove(h);
    }

    public void registerForEccNetChanged(Handler h, int what, Object obj) {
        Registrant r = new Registrant(h, what, obj);
        mUnsolEccNetChangedRegistrants.add(r);
    }

    public void unregisterForEccNetChanged(Handler h) {
        mUnsolEccNetChangedRegistrants.remove(h);
    }

    public void registerForRauSuccess(Handler h, int what, Object obj) {
        Registrant r = new Registrant(h, what, obj);
        mUnsolRauSuccessRegistrants.add(r);
    }

    public void unregisterForRauSuccess(Handler h) {
        mUnsolRauSuccessRegistrants.remove(h);
    }

    public void registerForClearCodeFallback(Handler h, int what, Object obj) {
        Registrant r = new Registrant(h, what, obj);
        mUnsolClearCodeFallbackRegistrants.add(r);
    }

    public void unregisterForClearCodeFallback(Handler h) {
        mUnsolClearCodeFallbackRegistrants.remove(h);
    }
    public void registerForSwitchPrimaryCard(Handler h, int what, Object obj) {
        Registrant r = new Registrant(h, what, obj);
        mUnsolSwitchPrimaryCardRegistrants.add(r);
    }

    public void unregisterForSwitchPrimaryCard(Handler h) {
        mUnsolSwitchPrimaryCardRegistrants.remove(h);
    }

    public void registerForRealSimStateChanged(Handler h,int what,Object obj) {
        Registrant r = new Registrant(h,what,obj);
        mUnsolRealSimStateChangedRegistrants.add(r);
        if (mHasRealSimStateChanged) {
            // If RI_UNSOL_SIMMGR_SIM_STATUS_CHANGED has already been reported, notify
            // registers right now.
            mUnsolRealSimStateChangedRegistrants
                    .notifyRegistrants(new AsyncResult(null, null, null));
        }
    }

    public void unregisterForRealSimStateChanged(Handler h) {
        mUnsolRealSimStateChangedRegistrants.remove(h);
    }

    public void registerForRadioCapabilityChanged(Handler h,int what,Object obj) {
        Registrant r = new Registrant(h,what,obj);
        mUnsolRadioCapabilityChangedRegistrants.add(r);
    }

    public void unregisterForRadioCapabilityChanged(Handler h) {
        mUnsolRadioCapabilityChangedRegistrants.remove(h);
    }

    public void registerForSimlockStatusChanged(Handler h, int what, Object obj) {
        Registrant r = new Registrant (h, what, obj);
        mSimlockStatusChangedRegistrants.add(r);
    }

    public void unregisterForSimlockStatusChanged(Handler h) {
        mSimlockStatusChangedRegistrants.remove(h);
    }

    public void registerForBandInfo(Handler h, int what, Object obj) {

        Registrant r = new Registrant(h, what, obj);
        mUnsolBandInfoRegistrants.add(r);
    }

    public void unregisterForBandInfo(Handler h) {
        mUnsolBandInfoRegistrants.remove(h);
    }

    public void registerForExpireSim(Handler h, int what, Object obj) {
        Registrant r = new Registrant(h, what, obj);
        mUnsolExpireSimdRegistrants.add(r);
    }

    public void unregisterForExpireSim(Handler h) {
        mUnsolExpireSimdRegistrants.remove(h);
    }

    /**
     * This is a helper function to be called when a RadioResponse callback is called.
     * It takes care of acks, wakelocks, and finds and returns RIRequest corresponding to the
     * response if one is found.
     * @param responseInfo RadioResponseInfo received in response callback
     * @return RIRequest corresponding to the response
     */
    RIRequest processResponse(RadioResponseInfo responseInfo) {
        int serial = responseInfo.serial;
        int error = responseInfo.error;
        int type = responseInfo.type;

        RIRequest rr = null;

        if (type == RadioResponseType.SOLICITED_ACK) {
            synchronized (mRequestList) {
                rr = mRequestList.get(serial);
            }
            if (rr == null) {
                UtilLog.logd(TAG, "Unexpected solicited ack response! sn: " + serial);
            } else {
                //decrementWakeLock(rr); //need to do?
                if (DBG) {
                    UtilLog.logd(TAG, rr.serialString() + " Ack < " + requestToString(rr.mRequest));
                }
            }
            return rr;
        }

        rr = findAndRemoveRequestFromList(serial);
        if (rr == null) {
            UtilLog.loge(TAG, "processResponse: Unexpected response! serial: " + serial
                    + " error: " + error);
            return null;
        }

        // Time logging for RIL command and storing it in TelephonyHistogram.
        //addToRilHistogram(rr);//need to do?

        if (type == RadioResponseType.SOLICITED_ACK_EXP) {
            sendAck();
            if (DBG) {
                UtilLog.logd(TAG, "Response received for " + rr.serialString() + " "
                        + requestToString(rr.mRequest) + " Sending ack to ril.cpp");
            }
        } else {
            // ack sent for SOLICITED_ACK_EXP above; nothing to do for SOLICITED response
        }
        return rr;
    }

    private RIRequest findAndRemoveRequestFromList(int serial) {
        RIRequest rr = null;
        synchronized (mRequestList) {
            rr = mRequestList.get(serial);
            if (rr != null) {
                mRequestList.remove(serial);
            }
        }

        return rr;
    }

    private void decrementWakeLock(RIRequest rr) {
        synchronized (rr) {
            switch(rr.mWakeLockType) {
                case FOR_WAKELOCK:
                    synchronized (mWakeLock) {

                        if (mWakeLockCount > 1) {
                            mWakeLockCount--;
                        } else {
                            mWakeLockCount = 0;
                            mWakeLock.release();
                        }
                    }
                    break;
                case FOR_ACK_WAKELOCK:
                    //We do not decrement the ACK wakelock
                    break;
                case INVALID_WAKELOCK:
                    break;
                default:
                    UtilLog.logd(TAG, "Decrementing Invalid Wakelock type " + rr.mWakeLockType);
            }
            rr.mWakeLockType = INVALID_WAKELOCK;
        }
    }

    private String convertNullToEmptyString(String string) {
        return string != null ? string : "";
    }

    void processResponseDone(RIRequest rr, RadioResponseInfo responseInfo, Object ret) {
        if (responseInfo.error == 0) {
            if (DBG) {
                riljLog(rr.serialString() + "< " + requestToString(rr.mRequest)
                        + " " + retToString(rr.mRequest, ret));
            }
        } else {
            if (DBG) {
                riljLog(rr.serialString() + "< " + requestToString(rr.mRequest)
                        + " error " + responseInfo.error);
            }
            rr.onError(responseInfo.error, ret);
        }
    }

    static String requestToString(int request) {
        switch (request) {
            case RI_REQUEST_GET_SIM_CAPACITY:
                return "GET_SIM_CAPACITY";
            case RI_REQUEST_GET_DEFAULT_NAN:
                return "GET_DEFAULT_NAN";
            case RI_REQUEST_ENABLE_RAU_NOTIFY:
                return "ENABLE_RAU_NOTIFY";
            case RI_REQUEST_SIM_GET_ATR:
                return "SIM_GET_ATR";
            case RI_REQUEST_GET_HD_VOICE_STATE:
                return "GET_HD_VOICE_STATE";
            case RI_REQUEST_ENABLE_COLP:
                return "ENABLE_COLP";
            case RI_REQUEST_STORE_SMS_TO_SIM:
                return "STORE_SMS_TO_SIM";
            case RI_REQUEST_QUERY_SMS_STORAGE_MODE:
                return "QUERY_SMS_STORAGE_MODE";
            case RI_REQUEST_SWITCH_MULTI_CALL:
                return "SWITCH_MULTI_CALL";
            case RI_REQUEST_VIDEOPHONE_DIAL:
                return "VIDEOPHONE_DIAL";
            case RI_REQUEST_VIDEOPHONE_CODEC:
                return "VIDEOPHONE_CODEC";
            case RI_REQUEST_VIDEOPHONE_FALLBACK:
                return "VIDEOPHONE_FALLBACK";
            case RI_REQUEST_VIDEOPHONE_STRING:
                return "VIDEOPHONE_STRING";
            case RI_REQUEST_VIDEOPHONE_LOCAL_MEDIA:
                return "VIDEOPHONE_LOCAL_MEDIA";
            case RI_REQUEST_VIDEOPHONE_CONTROL_IFRAME:
                return "VIDEOPHONE_CONTROL_IFRAME";
            case RI_REQUEST_DC_TRAFFIC_CLASS:
                return "DC_TRAFFIC_CLASS";
            case RI_REQUEST_ENABLE_LTE:
                return "ENABLE_LTE";
            case RI_REQUEST_ATTACH_DATACONN:
                return "ATTACH_DATACONN";
            case RI_REQUEST_ABORT_SEARCH_NETWORK:
                return "ABORT_SEARCH_NETWORK";
            case RI_REQUEST_DC_FORCE_DETACH:
                return "DC_FORCE_DETACH";
            case RIL_RESPONSE_ACKNOWLEDGEMENT:
                return "RESPONSE_ACKNOWLEDGEMENT";
            case RI_REQUEST_SET_FACILITY_LOCK_FOR_USER:
                return "SET_FACILITY_LOCK_BY_USER";
            case RI_REQUEST_GET_SIMLOCK_REMAIN_TIMES:
                return "GET_SIMLOCK_REMAIN_TIMES";
            case RI_REQUEST_GET_SIMLOCK_STATUS:
                return "GET_SIMLOCK_STATUS";
            case RI_REQUEST_GET_SIMLOCK_DUMMYS:
                return "GET_SIMLOCK_DUMMYS";
            case RI_REQUEST_GET_SIMLOCK_WHITE_LIST:
                return "GET_SIMLOCK_WHITE_LIST";
            case RI_REQUEST_SIM_POWER:
                return "SIM_POWER";
            case RI_REQUEST_UPDATE_REAL_ECCLIST:
                return"UPDATE_REAL_ECCLIST";
            case RI_REQUEST_GET_BAND_INFO:
                return "GET_BAND_INFO";
            case RI_REQUEST_SET_BAND_INFO_MODE:
                return "SET_BAND_INFO_MODE";
            case RI_REQUEST_SET_SINGLE_PDN:
                return "SET_SINGLE_PDN";
            case RI_REQUEST_SET_SPECIAL_RATCAP:
                return "SET_NETWORK_RAT_PREFERRED";
            case RI_REQUEST_QUERY_COLP:
                return "QUERY_COLP";
            case RI_REQUEST_QUERY_COLR:
                return "QUERY_COLR";
            case RI_REQUEST_MMI_ENTER_SIM:
                return "MMI_ENTER_SIM";
            case RI_REQUEST_UPDATE_OPERATOR_NAME:
                return "UPDATE_OPERATOR_NAME";
            case RI_REQUEST_SIMMGR_GET_SIM_STATUS:
                return "SIMMGR_GET_SIM_STATUS";
            case RI_REQUEST_SET_XCAP_IP_ADDR:
                return "SET_XCAP_IP_ADDR";
            case RI_REQUEST_SEND_CMD:
                return "SEND_CMD";
            case RI_REQUEST_GET_SIM_STATUS:
                return "GET_SIM_STATUS";
            case RI_REQUEST_REATTACH:
                return "REATTACH";
            case RI_REQUEST_SET_PREFERRED_NETWORK_TYPE:
                return "SET_PREFERRED_NETWORK_TYPE";
            case RI_REQUEST_SHUTDOWN:
                return "SHUTDOWN";
            case RI_REQUEST_EXPLICIT_CALL_TRANSFER:
                return "EXPLICIT_CALL_TRANSFER";
            case RI_REQUEST_SET_SMS_BEARER:
                return "SET_SMS_BEARER";
            case RI_REQUEST_SET_VOICE_DOMAIN:
                return "SET_VOICE_DOMAIN";
            default:
                return "<unknown request>";
         }
     }

    static String responseToString(int request) {
        switch(request) {
            case RI_UNSOL_RI_CONNECTED:
                return "UNSOL_RI_CONNECTED";
            case RI_UNSOL_VIDEOPHONE_CODEC:
                return "UNSOL_VIDEOPHONE_CODEC";
            case RI_UNSOL_VIDEOPHONE_DSCI:
                return "UNSOL_VIDEOPHONE_DSCI";
            case RI_UNSOL_VIDEOPHONE_STRING:
                return "UNSOL_VIDEOPHONE_STRING";
            case RI_UNSOL_VIDEOPHONE_REMOTE_MEDIA:
                return "UNSOL_VIDEOPHONE_REMOTE_MEDIA";
            case RI_UNSOL_VIDEOPHONE_MM_RING:
                return "UNSOL_VIDEOPHONE_MM_RING";
            case RI_UNSOL_VIDEOPHONE_RELEASING:
                return "UNSOL_VIDEOPHONE_RELEASING";
            case RI_UNSOL_VIDEOPHONE_RECORD_VIDEO:
                return "UNSOL_VIDEOPHONE_RECORD_VIDEO";
            case RI_UNSOL_VIDEOPHONE_MEDIA_START:
                return "UNSOL_VIDEOPHONE_MEDIA_START";
            case RI_UNSOL_ECC_NETWORKLIST_CHANGED:
                return "UNSOL_ECC_NETWORKLIST_CHANGED";
            case RI_UNSOL_RAU_NOTIFY:
                return "UNSOL_RAU_NOTIFY";
            case RI_UNSOL_CLEAR_CODE_FALLBACK:
                return "UNSOL_CLEAR_CODE_FALLBACK";
            case RI_UNSOL_SIMLOCK_STATUS_CHANGED:
                return "UNSOL_SIMLOCK_STATUS_CHANGED";
            case RI_UNSOL_SIMLOCK_SIM_EXPIRED:
                return "UNSOL_SIMLOCK_SIM_EXPIRED";
            case RI_UNSOL_BAND_INFO:
                return "UNSOL_BAND_INFO";
            case RI_UNSOL_SWITCH_PRIMARY_CARD:
                return "UNSOL_SWITCH_PRIMARY_CARD";
            case RI_UNSOL_SIM_PS_REJECT:
                return "UNSOL_SIM_PS_REJECT";
            case RI_UNSOL_SIMMGR_SIM_STATUS_CHANGED:
                return "UNSOL_SIMMGR_SIM_STATUS_CHANGED";
            case RI_UNSOL_RADIO_CAPABILITY_CHANGED:
                return "UNSOL_RADIO_CAPABILITY_CHANGED";
            default:
                return "<unknown response>";
        }
    }

    static String retToString(int req, Object ret) {
        if (ret == null) return "";
        StringBuilder sb;
        String s;
        int length;

        if (ret instanceof int[]) {
            int[] intArray = (int[]) ret;
            length = intArray.length;
            sb = new StringBuilder("{");
            if (length > 0) {
                int i = 0;
                sb.append(intArray[i++]);
                while (i < length) {
                    sb.append(", ").append(intArray[i++]);
                }
            }
            sb.append("}");
            s = sb.toString();
        } else if (ret instanceof String[]) {
            String[] strings = (String[]) ret;
            length = strings.length;
            sb = new StringBuilder("{");
            if (length > 0) {
            int i = 0;
            sb.append(strings[i++]);
            while (i < length) {
                sb.append(", ").append(strings[i++]);
            }
        }
        sb.append("}");
        s = sb.toString();
        } else {
            s = ret.toString();
        }
        return s;
    }

    void unsljLog(int response) {
        riljLog("[UNSL]< " + responseToString(response));
    }

    void unsljLogRet(int response, Object ret) {
        riljLog("[UNSL]< " + responseToString(response) + " " + retToString(response, ret));
    }

    void riljLog(String msg) {
        Rlog.d(TAG, msg
                + (mPhoneId != null ? (" [SUB" + mPhoneId + "]") : ""));
    }

    public static int[] arrayListToPrimitiveArray(ArrayList<Integer> ints) {
        int[] ret = new int[ints.size()];
        for (int i = 0; i < ret.length; i++) {
            ret[i] = ints.get(i);
        }
        return ret;
    }

}
