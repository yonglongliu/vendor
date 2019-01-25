
package com.android.sprd.telephony;

import android.content.Context;
import android.os.UserHandle;
import android.telephony.TelephonyManager;

public class RadioInteractorFactory{

    public static final String TAG = "RadioInteractorFactory";
    private static RadioInteractorFactory sInstance;
    Context mContext;

    private RadioInteractorHandler[] mRadioInteractorHandler;
    private RadioInteractorCore[] mRadioInteractorCores;

    public static RadioInteractorFactory init(Context context) {
        synchronized (RadioInteractorFactory.class) {
            if (sInstance == null && UserHandle.myUserId() == UserHandle.USER_SYSTEM) {
                sInstance = new RadioInteractorFactory(context);
            } else {
                UtilLog.loge("RadioInteractorFactory", "init() called multiple times!  sInstance = "
                        + sInstance);
            }
            return sInstance;
        }
    }

    public RadioInteractorFactory(Context context) {
        mContext = context;
        RadioInteractorNotifier radioInteractorNotifier = initRadioInteractorNotifier();
        startRILoop(radioInteractorNotifier);
        RadioInteractorProxy.init(context,radioInteractorNotifier, mRadioInteractorHandler);
    }

    public static RadioInteractorFactory getInstance() {
        return sInstance;
    }

    public void startRILoop(RadioInteractorNotifier radioInteractorNotifier) {
        int numPhones = TelephonyManager.getDefault().getPhoneCount();
        mRadioInteractorCores = new RadioInteractorCore[numPhones];
        mRadioInteractorHandler = new RadioInteractorHandler[numPhones];
        for (int i = 0; i < numPhones; i++) {
            mRadioInteractorCores[i] = new RadioInteractorCore(mContext, i);
            mRadioInteractorHandler[i] = new RadioInteractorHandler(
                    mRadioInteractorCores[i], radioInteractorNotifier,mContext);
         }
    }

    public RadioInteractorHandler getRadioInteractorHandler(int slotId) {
        if (mRadioInteractorHandler != null && slotId < mRadioInteractorHandler.length) {
            return mRadioInteractorHandler[slotId];
        }
        return null;
    }

    public RadioInteractorCore getRadioInteractorCore(int slotId) {
        if (mRadioInteractorCores != null && slotId < mRadioInteractorCores.length) {
            return mRadioInteractorCores[slotId];
        }
        return null;
    }

    public RadioInteractorNotifier initRadioInteractorNotifier() {
        return new RadioInteractorNotifier();
    }
}
