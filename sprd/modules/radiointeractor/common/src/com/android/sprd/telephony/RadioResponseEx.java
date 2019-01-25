package com.android.sprd.telephony;

import java.util.ArrayList;

import android.hardware.radio.V1_0.AppStatus;
import com.android.sprd.telephony.uicc.IccCardApplicationStatusEx;
import com.android.sprd.telephony.uicc.IccCardStatusEx;
import com.android.sprd.telephony.uicc.IccIoResult;

import android.hardware.radio.V1_0.CardStatus;
import android.hardware.radio.V1_0.RadioError;
import android.hardware.radio.V1_0.RadioResponseInfo;
import vendor.sprd.hardware.radio.V1_0.IExtRadioResponse;

import android.os.AsyncResult;
import android.os.Message;

public class RadioResponseEx extends IExtRadioResponse.Stub {

    RadioInteractorCore mRi;

    public RadioResponseEx(RadioInteractorCore ri) {
        mRi = ri;
    }

    static void sendMessageResponse(Message msg, Object ret) {
        if (msg != null) {
            AsyncResult.forMessage(msg, ret, null);
            msg.sendToTarget();
        }
    }

    public void videoPhoneDialResponse(RadioResponseInfo responseInfo) {
        responseVoid(responseInfo);
    }

    public void videoPhoneCodecResponse(RadioResponseInfo responseInfo) {
        responseVoid(responseInfo);
    }

    public void videoPhoneFallbackResponse(RadioResponseInfo responseInfo) {
        responseVoid(responseInfo);
    }

    public void videoPhoneStringResponse(RadioResponseInfo responseInfo) {
        responseVoid(responseInfo);
    }

    public void videoPhoneLocalMediaResponse(RadioResponseInfo responseInfo) {
        responseVoid(responseInfo);
    }

    public void videoPhoneControlIFrameResponse(RadioResponseInfo responseInfo) {
        responseVoid(responseInfo);
    }

    public void switchMultiCallResponse(RadioResponseInfo responseInfo) {
        responseVoid(responseInfo);
    }

    public void setTrafficClassResponse(RadioResponseInfo responseInfo) {
        responseVoid(responseInfo);
    }

    public void enableLTEResponse(RadioResponseInfo responseInfo) {
        responseVoid(responseInfo);
    }

    public void attachDataResponse(RadioResponseInfo responseInfo) {
        responseVoid(responseInfo);
    }

    public void stopQueryNetworkResponse(RadioResponseInfo responseInfo) {
        responseVoid(responseInfo);
    }

    public void forceDeatchResponse(RadioResponseInfo responseInfo) {
        responseVoid(responseInfo);
    }

    public void getHDVoiceStateResponse(RadioResponseInfo responseInfo, int state) {
        responseInts(responseInfo, state);
    }

    public void simmgrSimPowerResponse(RadioResponseInfo responseInfo) {
        responseVoid(responseInfo);
    }

    public void enableRauNotifyResponse(RadioResponseInfo responseInfo) {
        responseVoid(responseInfo);
    }

    public void setCOLPResponse(RadioResponseInfo responseInfo) {
        responseVoid(responseInfo);
    }

    public void getDefaultNANResponse(RadioResponseInfo responseInfo, String name) {
        responseString(responseInfo, name);
    }

    public void simGetAtrResponse(RadioResponseInfo responseInfo, String atr) {
        responseString(responseInfo, atr);
    }

    public void getSimCapacityResponse(RadioResponseInfo responseInfo, ArrayList<String> data) {
        responseStringArrayList(responseInfo, data);
    }

    public void storeSmsToSimResponse(RadioResponseInfo responseInfo) {
        responseVoid(responseInfo);
    }

    public void querySmsStorageModeResponse(RadioResponseInfo responseInfo, String atr) {
        responseString(responseInfo, atr);
    }

    public void getSimlockRemaintimesResponse(RadioResponseInfo responseInfo, int remainingRetries) {
        responseInts(responseInfo, remainingRetries);
    }

    public void setFacilityLockForUserResponse(RadioResponseInfo responseInfo) {
        responseVoid(responseInfo);
    }

    public void getSimlockStatusResponse(RadioResponseInfo responseInfo, int status) {
        responseInts(responseInfo, status);
    }

    public void getSimlockDummysResponse(RadioResponseInfo responseInfo, ArrayList<Integer> selectResponse) {
        responseIntArrayList(responseInfo, selectResponse);
    }

    public void getSimlockWhitelistResponse(RadioResponseInfo responseInfo, String whitelist) {
        responseString(responseInfo, whitelist);
    }

    public void updateEcclistResponse(RadioResponseInfo responseInfo) {
        responseVoid(responseInfo);
    }

    public void getBandInfoResponse(RadioResponseInfo responseInfo, String bandInfo) {
        responseString(responseInfo, bandInfo);
    }

    public void setBandInfoModeResponse(RadioResponseInfo responseInfo) {
        responseVoid(responseInfo);
    }

    public void setSinglePDNResponse(RadioResponseInfo responseInfo) {
        responseVoid(responseInfo);
    }

    public void setSpecialRatcapResponse(RadioResponseInfo responseInfo) {
        responseVoid(responseInfo);
    }

    public void queryColpResponse(RadioResponseInfo responseInfo, int result) {
        responseInts(responseInfo, result);
    }

    public void queryColrResponse(RadioResponseInfo responseInfo, int result) {
        responseInts(responseInfo, result);
    }

    public void mmiEnterSimResponse(RadioResponseInfo responseInfo) {
        responseVoid(responseInfo);
    }

    public void updateOperatorNameResponse(RadioResponseInfo responseInfo) {
        responseVoid(responseInfo);
    }

    public void simmgrGetSimStatusResponse(RadioResponseInfo responseInfo, CardStatus cardStatus) {
        responseSimStatus(responseInfo, cardStatus);
    }

    public void setXcapIPAddressResponse(RadioResponseInfo responseInfo) {
        responseVoid(responseInfo);
    }

    public void sendCmdAsyncResponse(RadioResponseInfo responseInfo, String response) {
        responseString(responseInfo, response);
    }

    public void getIccCardStatusExtResponse(RadioResponseInfo responseInfo, CardStatus cardStatus) {
        responseSimStatus(responseInfo, cardStatus);
    }

    public void reAttachResponse(RadioResponseInfo responseInfo) {
        responseVoid(responseInfo);
    }

    public void setPreferredNetworkTypeExtResponse(RadioResponseInfo responseInfo) {
        responseVoid(responseInfo);
    }

    public void requestShutdownExtResponse(RadioResponseInfo responseInfo) {
        responseVoid(responseInfo);
    }

    public void explicitCallTransferExtResponse(RadioResponseInfo responseInfo) {
        responseVoid(responseInfo);
    }

    public void setSmsBearerResponse(RadioResponseInfo responseInfo) {
        responseVoid(responseInfo);
    }

    public void setVoiceDomainResponse(RadioResponseInfo responseInfo) {
        responseVoid(responseInfo);
    }

    private void responseSimStatus(RadioResponseInfo responseInfo, CardStatus cardStatus) {
        RIRequest rr = mRi.processResponse(responseInfo);

        if (rr != null) {
            IccCardStatusEx iccCardStatus = new IccCardStatusEx();
            iccCardStatus.setCardState(cardStatus.cardState);
            iccCardStatus.setUniversalPinState(cardStatus.universalPinState);
            iccCardStatus.mGsmUmtsSubscriptionAppIndex = cardStatus.gsmUmtsSubscriptionAppIndex;
            iccCardStatus.mCdmaSubscriptionAppIndex = cardStatus.cdmaSubscriptionAppIndex;
            iccCardStatus.mImsSubscriptionAppIndex = cardStatus.imsSubscriptionAppIndex;
            int numApplications = cardStatus.applications.size();

            // limit to maximum allowed applications
            if (numApplications > IccCardStatusEx.CARD_MAX_APPS) {
                numApplications = IccCardStatusEx.CARD_MAX_APPS;
            }
            iccCardStatus.mApplications = new IccCardApplicationStatusEx[numApplications];
            for (int i = 0; i < numApplications; i++) {
                AppStatus rilAppStatus = cardStatus.applications.get(i);
                IccCardApplicationStatusEx appStatus = new IccCardApplicationStatusEx();
                appStatus.app_type       = appStatus.AppTypeFromRILInt(rilAppStatus.appType);
                appStatus.app_state      = appStatus.AppStateFromRILInt(rilAppStatus.appState);
                appStatus.perso_substate = appStatus.PersoSubstateFromRILInt(
                        rilAppStatus.persoSubstate);
                appStatus.aid            = rilAppStatus.aidPtr;
                appStatus.app_label      = rilAppStatus.appLabelPtr;
                appStatus.pin1_replaced  = rilAppStatus.pin1Replaced;
                appStatus.pin1           = appStatus.PinStateFromRILInt(rilAppStatus.pin1);
                appStatus.pin2           = appStatus.PinStateFromRILInt(rilAppStatus.pin2);
                iccCardStatus.mApplications[i] = appStatus;
             }
             UtilLog.logd(UtilLog.TAG, "responseSimStatus: from HIDL: " + iccCardStatus);
             if (responseInfo.error == RadioError.NONE) {
                 sendMessageResponse(rr.mResult, iccCardStatus);
             }
             mRi.processResponseDone(rr, responseInfo, iccCardStatus);
        }
    }

    private void responseIccIo(RadioResponseInfo responseInfo,
            android.hardware.radio.V1_0.IccIoResult result) {
        RIRequest rr = mRi.processResponse(responseInfo);

        if (rr != null) {
            IccIoResult ret = null;
            if (responseInfo.error == RadioError.NONE) {
                ret = new IccIoResult(result.sw1, result.sw2, result.simResponse);
                sendMessageResponse(rr.mResult, ret);
            }
            mRi.processResponseDone(rr, responseInfo, ret);
        }
    }

    private void responseVoid(RadioResponseInfo responseInfo) {
        RIRequest rr = mRi.processResponse(responseInfo);

        if (rr != null) {
            Object ret = null;
            if (responseInfo.error == RadioError.NONE) {
                sendMessageResponse(rr.mResult, ret);
            }
            mRi.processResponseDone(rr, responseInfo, ret);
        }
    }

    private void responseInts(RadioResponseInfo responseInfo, int ...var) {
        final ArrayList<Integer> ints = new ArrayList<>();
        for (int i = 0; i < var.length; i++) {
            ints.add(var[i]);
        }
        responseIntArrayList(responseInfo, ints);
    }

    private void responseIntArrayList(RadioResponseInfo responseInfo, ArrayList<Integer> var) {
        RIRequest rr = mRi.processResponse(responseInfo);

        if (rr != null) {
            Object ret = null;
            if (responseInfo.error == RadioError.NONE) {
                int[] response = new int[var.size()];
                for (int i = 0; i < var.size(); i++) {
                    response[i] = var.get(i);
                }
                ret = response;
                sendMessageResponse(rr.mResult, ret);
            }
            mRi.processResponseDone(rr, responseInfo, ret);
        }
    }

    private void responseString(RadioResponseInfo responseInfo, String str) {
        RIRequest rr = mRi.processResponse(responseInfo);

        if (rr != null) {
            if (responseInfo.error == RadioError.NONE) {
                sendMessageResponse(rr.mResult, str);
            }
            mRi.processResponseDone(rr, responseInfo, str);
        }
    }

    private void responseStrings(RadioResponseInfo responseInfo, String ...str) {
        ArrayList<String> strings = new ArrayList<>();
        for (int i = 0; i < str.length; i++) {
            strings.add(str[i]);
        }
        responseStringArrayList(responseInfo, strings);
    }

    private void responseStringArrayList(RadioResponseInfo responseInfo, ArrayList<String> strings) {
        RIRequest rr = mRi.processResponse(responseInfo);

        if (rr != null) {
            String[] ret = null;
            if (responseInfo.error == RadioError.NONE) {
                ret = new String[strings.size()];
                for (int i = 0; i < strings.size(); i++) {
                    ret[i] = strings.get(i);
                }
                sendMessageResponse(rr.mResult, ret);
            }
            mRi.processResponseDone(rr, responseInfo, ret);
        }
    }
}
