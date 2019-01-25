
package com.android.sprd.telephony.server;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.widget.Toast;
import com.android.sprd.telephony.RadioInteractorFactory;
import com.android.sprd.telephony.UtilLog;

public class BootupReceiver extends BroadcastReceiver {
    public static final String TAG = "BootupReceiver";
    static boolean mIsStart = false;

    @Override
    public void onReceive(Context context, Intent intent) {
        UtilLog.logd(TAG, intent.getAction());
        if (Intent.ACTION_BOOT_COMPLETED.equals(intent.getAction())) {
            Toast.makeText(context, "RadioInteractor Service  start", Toast.LENGTH_LONG).show();
        } else  if (!mIsStart) {
            RadioInteractorFactory.init(context);
            mIsStart = true;
        }
    }
}
