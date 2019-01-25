package com.android.sprd.telephony;

import vendor.sprd.hardware.radio.V1_0.VideoPhoneDSCI;
import vendor.sprd.hardware.radio.V1_0.IExtRadioIndication;

import java.util.ArrayList;

import android.os.AsyncResult;

import static com.android.sprd.telephony.RIConstants.RI_UNSOL_VIDEOPHONE_CODEC;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_VIDEOPHONE_DSCI;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_VIDEOPHONE_STRING;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_VIDEOPHONE_REMOTE_MEDIA;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_VIDEOPHONE_MM_RING;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_VIDEOPHONE_RELEASING;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_VIDEOPHONE_RECORD_VIDEO;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_VIDEOPHONE_MEDIA_START;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_ECC_NETWORKLIST_CHANGED;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_SIMLOCK_SIM_EXPIRED;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_RAU_NOTIFY;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_CLEAR_CODE_FALLBACK;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_RI_CONNECTED;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_SIMLOCK_STATUS_CHANGED;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_BAND_INFO;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_SWITCH_PRIMARY_CARD;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_SIM_PS_REJECT;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_SIMMGR_SIM_STATUS_CHANGED;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_RADIO_CAPABILITY_CHANGED;
import static com.android.internal.telephony.RILConstants.RIL_UNSOL_RIL_CONNECTED;

public class RadioIndicationEx extends IExtRadioIndication.Stub {

    RadioInteractorCore mRi;

    public RadioIndicationEx(RadioInteractorCore ri) {
        mRi = ri;
    }

    public void rilConnected(int indicationType) {
        mRi.processIndication(indicationType);

        mRi.unsljLog(RIL_UNSOL_RIL_CONNECTED);

        mRi.notifyRegistrantsRilConnectionChanged(15);
    }

    public void videoPhoneCodecInd(int indicationType, ArrayList<Integer> data) {
        mRi.processIndication(indicationType);

        int response[] = RadioInteractorCore.arrayListToPrimitiveArray(data);
        if (RadioInteractorCore.DBG) {
            mRi.unsljLogRet(RI_UNSOL_VIDEOPHONE_CODEC, response);
        }

        if (mRi.mUnsolVPCodecRegistrants != null) {
            mRi.mUnsolVPCodecRegistrants.notifyRegistrants(new AsyncResult(null, response, null));
        }
    }

    public void videoPhoneDSCIInd(int indicationType, vendor.sprd.hardware.radio.V1_0.VideoPhoneDSCI data) {
        mRi.processIndication(indicationType);

        VideoPhoneDSCI dsci = new VideoPhoneDSCI();
        dsci.id = data.id;
        dsci.idr = data.idr;
        dsci.stat = data.stat;
        dsci.type = data.type;
        dsci.mpty = data.mpty;
        dsci.number = data.number;
        dsci.numType = data.numType;
        dsci.bsType = data.bsType;
        dsci.cause = data.cause;
        dsci.location = data.location;

        if (RadioInteractorCore.DBG) {
            mRi.unsljLogRet(RI_UNSOL_VIDEOPHONE_DSCI, dsci);
        }

        if (dsci.cause == 47 || dsci.cause == 57 || dsci.cause == 50 ||
                dsci.cause == 58 || dsci.cause == 69 || dsci.cause == 88) {
            if (mRi.mUnsolVPFallBackRegistrants != null) {
                if ((dsci.cause == 50 || dsci.cause == 57) && (dsci.location <= 2)) {
                    mRi.mUnsolVPFallBackRegistrants.notifyRegistrants(
                            new AsyncResult(null,new AsyncResult(dsci.idr,
                            new AsyncResult(dsci.number, (dsci.location == 2 ?
                                    (dsci.cause + 200) : (dsci.cause + 100)), null), null),null));
                } else {
                    mRi.mUnsolVPFallBackRegistrants.notifyRegistrants(
                            new AsyncResult(null,new AsyncResult(dsci.idr,
                            new AsyncResult(dsci.number, dsci.cause, null), null),null));
                }
            } else {
                if (mRi.mUnsolVPFailRegistrants != null) {
                    mRi.mUnsolVPFailRegistrants.notifyRegistrants(
                            new AsyncResult(null,new AsyncResult(dsci.idr,
                            new AsyncResult(dsci.number, dsci.cause, null), null),null));
                }
            }
        }
    }

    public void videoPhoneStringInd(int indicationType, String data) {
        mRi.processIndication(indicationType);

        if (RadioInteractorCore.DBG) {
            mRi.unsljLogRet(RI_UNSOL_VIDEOPHONE_STRING, data);
        }

        if (mRi.mUnsolVPStrsRegistrants != null) {
            mRi.mUnsolVPStrsRegistrants.notifyRegistrants(new AsyncResult(null, data, null));
        }
    }

    public void videoPhoneRemoteMediaInd(int indicationType, ArrayList<Integer> data) {
        mRi.processIndication(indicationType);

        int response[] = RadioInteractorCore.arrayListToPrimitiveArray(data);
        if (RadioInteractorCore.DBG) {
            mRi.unsljLogRet(RI_UNSOL_VIDEOPHONE_REMOTE_MEDIA, response);
        }

        if (mRi.mUnsolVPRemoteMediaRegistrants != null) {
            mRi.mUnsolVPRemoteMediaRegistrants.notifyRegistrants(new AsyncResult(null, response, null));
        }
    }

    public void videoPhoneMMRingInd(int indicationType, int data) {
        mRi.processIndication(indicationType);

        if (RadioInteractorCore.DBG) {
            mRi.unsljLogRet(RI_UNSOL_VIDEOPHONE_MM_RING, data);
        }

        if (mRi.mUnsolVPMMRingRegistrants != null) {
            mRi.mUnsolVPMMRingRegistrants.notifyRegistrants(new AsyncResult(null, data, null));
        }
    }

    public void videoPhoneReleasingInd(int indicationType, String data) {
        mRi.processIndication(indicationType);
        if (RadioInteractorCore.DBG) {
            mRi.unsljLogRet(RI_UNSOL_VIDEOPHONE_RELEASING, data);
        }

        if (mRi.mUnsolVPFailRegistrants != null) {
            mRi.mUnsolVPFailRegistrants.notifyRegistrants(new AsyncResult(null, data, null));
        }
    }

    public void videoPhoneRecordVideoInd(int indicationType, int data) {
        mRi.processIndication(indicationType);

        if (RadioInteractorCore.DBG) {
            mRi.unsljLogRet(RI_UNSOL_VIDEOPHONE_RECORD_VIDEO, data);
        }

        if (mRi.mUnsolVPRecordVideoRegistrants != null) {
            mRi.mUnsolVPRecordVideoRegistrants.notifyRegistrants(new AsyncResult(null, data, null));
        }
    }

    public void videoPhoneMediaStartInd(int indicationType, int data) {
        mRi.processIndication(indicationType);

        if (RadioInteractorCore.DBG) {
            mRi.unsljLogRet(RI_UNSOL_VIDEOPHONE_MEDIA_START, data);
        }

        if (mRi.mUnsolVPMediaStartRegistrants != null) {
            mRi.mUnsolVPMediaStartRegistrants.notifyRegistrants(new AsyncResult(null, data, null));
        }
    }

    public void eccNetworkListChangedInd(int indicationType, String data) {
        mRi.processIndication(indicationType);

        if (RadioInteractorCore.DBG) {
            mRi.unsljLogRet(RI_UNSOL_ECC_NETWORKLIST_CHANGED, data);
        }

        if (mRi.mUnsolEccNetChangedRegistrants != null) {
            mRi.mUnsolEccNetChangedRegistrants.notifyRegistrants(new AsyncResult(null, data, null));
        }
    }

    public void rauSuccessInd(int indicationType) {
        mRi.processIndication(indicationType);

        if (RadioInteractorCore.DBG) {
            mRi.unsljLog(RI_UNSOL_RAU_NOTIFY);
        }

        if (mRi.mUnsolRauSuccessRegistrants != null) {
            mRi.mUnsolRauSuccessRegistrants.notifyRegistrants();
        }
    }

    public void clearCodeFallbackInd(int indicationType) {
        mRi.processIndication(indicationType);

        if (RadioInteractorCore.DBG) {
            mRi.unsljLog(RI_UNSOL_CLEAR_CODE_FALLBACK);
        }

        if (mRi.mUnsolClearCodeFallbackRegistrants != null) {
            mRi.mUnsolClearCodeFallbackRegistrants.notifyRegistrants();
        }
    }

    public void rilConnectedInd(int indicationType) {
        mRi.processIndication(indicationType);

        if (RadioInteractorCore.DBG) {
            mRi.unsljLog(RI_UNSOL_RI_CONNECTED);
        }

        if (mRi.mUnsolRIConnectedRegistrants != null) {
            mRi.mUnsolRIConnectedRegistrants.notifyRegistrants();
        }
    }

    public void simlockStatusChangedInd(int indicationType) {
        mRi.processIndication(indicationType);

        if (RadioInteractorCore.DBG) {
            mRi.unsljLog(RI_UNSOL_SIMLOCK_STATUS_CHANGED);
        }

        if (mRi.mSimlockStatusChangedRegistrants != null) {
            mRi.mSimlockStatusChangedRegistrants.notifyRegistrants();
        }
    }

    public void simlockSimExpiredInd(int indicationType, int data) {
        mRi.processIndication(indicationType);

        if (RadioInteractorCore.DBG) {
            mRi.unsljLogRet(RI_UNSOL_SIMLOCK_SIM_EXPIRED, data);
        }

        if (mRi.mUnsolExpireSimdRegistrants != null) {
            mRi.mUnsolExpireSimdRegistrants.notifyRegistrants(new AsyncResult(null, data, null));
        }
    }

    public void bandInfoInd(int indicationType, String data) {
        mRi.processIndication(indicationType);

        if (RadioInteractorCore.DBG) {
            mRi.unsljLogRet(RI_UNSOL_BAND_INFO, data);
        }

        if (mRi.mUnsolBandInfoRegistrants != null) {
            mRi.mUnsolBandInfoRegistrants.notifyRegistrants(new AsyncResult(null, data, null));
        }
    }

    public void switchPrimaryCardInd(int indicationType, int data) {
        mRi.processIndication(indicationType);

        if (RadioInteractorCore.DBG) {
            mRi.unsljLogRet(RI_UNSOL_SWITCH_PRIMARY_CARD, data);
        }

        if (mRi.mUnsolSwitchPrimaryCardRegistrants != null) {
            mRi.mUnsolSwitchPrimaryCardRegistrants.notifyRegistrants(new AsyncResult(null, data, null));
        }
    }

   //not notify
    public void simPSRejectInd(int indicationType) {
        mRi.processIndication(indicationType);

        if (RadioInteractorCore.DBG) {
            mRi.unsljLog(RI_UNSOL_SIM_PS_REJECT);
        }
    }

    public void simMgrSimStatusChangedInd(int indicationType) {
        mRi.processIndication(indicationType);

        if (RadioInteractorCore.DBG) {
            mRi.unsljLog(RI_UNSOL_SIMMGR_SIM_STATUS_CHANGED);
        }

        mRi.mHasRealSimStateChanged = true;

        if (mRi.mUnsolRealSimStateChangedRegistrants != null) {
            mRi.mUnsolRealSimStateChangedRegistrants.notifyRegistrants();
        }
    }

    public void radioCapabilityChangedInd(int indicationType) {
        mRi.processIndication(indicationType);

        if (RadioInteractorCore.DBG) {
            mRi.unsljLog(RI_UNSOL_RADIO_CAPABILITY_CHANGED);
        }

        if (mRi.mUnsolRadioCapabilityChangedRegistrants != null) {
            mRi.mUnsolRadioCapabilityChangedRegistrants.notifyRegistrants();
        }
    }
}
