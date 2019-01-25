/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.nfc.dhimpl;

/* START [J15020401] - Enable WCE with UICC/eSE. */
import android.annotation.SdkConstant;
import android.annotation.SdkConstant.SdkConstantType;
import android.content.SharedPreferences;
/* END [J15020401] - Enable WCE with UICC/eSE. */
import android.content.Context;
import android.nfc.ErrorCodes;
import android.nfc.tech.Ndef;
import android.nfc.tech.TagTechnology;
import android.util.Log;

import com.android.nfc.DeviceHost;
import com.android.nfc.LlcpException;
import com.android.nfc.NfcDiscoveryParameters;

/**
 * Native interface to the NFC Manager functions
 */
public class NativeNfcManager implements DeviceHost {
    private static final String TAG = "NativeNfcManager";
    static final String PREF = "NciDeviceHost";

    static final int DEFAULT_LLCP_MIU = 1980;
    static final int DEFAULT_LLCP_RWSIZE = 2;

    static final String DRIVER_NAME = "android-nci";

/* START [J15020401] - Enable WCE with UICC/eSE. */
    private static final byte[][] EE_WIPE_APDUS = {
        {(byte)0x00, (byte)0xa4, (byte)0x04, (byte)0x00, (byte)0x00},
        {(byte)0x00, (byte)0xa4, (byte)0x04, (byte)0x00, (byte)0x07, (byte)0xa0, (byte)0x00,
            (byte)0x00, (byte)0x04, (byte)0x76, (byte)0x20, (byte)0x10, (byte)0x00},
        {(byte)0x80, (byte)0xe2, (byte)0x01, (byte)0x03, (byte)0x00},
        {(byte)0x00, (byte)0xa4, (byte)0x04, (byte)0x00, (byte)0x00},
        {(byte)0x00, (byte)0xa4, (byte)0x04, (byte)0x00, (byte)0x07, (byte)0xa0, (byte)0x00,
            (byte)0x00, (byte)0x04, (byte)0x76, (byte)0x30, (byte)0x30, (byte)0x00},
        {(byte)0x80, (byte)0xb4, (byte)0x00, (byte)0x00, (byte)0x00},
        {(byte)0x00, (byte)0xa4, (byte)0x04, (byte)0x00, (byte)0x00},
    };

    static {
        System.loadLibrary("nfc_sec_jni");
    }

    @SdkConstant(SdkConstantType.BROADCAST_INTENT_ACTION)
    public static final String INTERNAL_TARGET_DESELECTED_ACTION = "com.android.nfc.action.INTERNAL_TARGET_DESELECTED";
/* END [J15020401] - Enable WCE with UICC/eSE. */
    /* Native structure */
    private long mNative;

    private final DeviceHostListener mListener;
    private final Context mContext;


    public NativeNfcManager(Context context, DeviceHostListener listener) {
        mListener = listener;
        initializeNativeStructure();
        mContext = context;
    }

    public native boolean initializeNativeStructure();

    private native boolean doDownload();

    public native int doGetLastError();

    @Override
    public void checkFirmware() {
        doDownload();
    }

    private native boolean doInitialize();

    @Override
    public boolean initialize() {
/* START [J15020401] - Enable WCE with UICC/eSE. */
        SharedPreferences prefs = mContext.getSharedPreferences(PREF, Context.MODE_PRIVATE);
        SharedPreferences.Editor editor = prefs.edit();

        if (prefs.getBoolean(NativeNfcSecureElement.PREF_SE_WIRED, false)) {
            try {
                Thread.sleep (12000);
                editor.putBoolean(NativeNfcSecureElement.PREF_SE_WIRED, false);
                editor.apply();
            } catch (InterruptedException e) { }
        }
/* END [J15020401] - Enable WCE with UICC/eSE. */
        return doInitialize();
    }

    private native boolean doDeinitialize();

    @Override
    public boolean deinitialize() {
/* START [J15020401] - Enable WCE with UICC/eSE. */
        SharedPreferences prefs = mContext.getSharedPreferences(PREF, Context.MODE_PRIVATE);
        SharedPreferences.Editor editor = prefs.edit();

        editor.putBoolean(NativeNfcSecureElement.PREF_SE_WIRED, false);
        editor.apply();
/* END [J15020401] - Enable WCE with UICC/eSE. */
        return doDeinitialize();
    }

    @Override
    public String getName() {
        return DRIVER_NAME;
    }

    @Override
    public native boolean sendRawFrame(byte[] data);

    @Override
    public native boolean routeAid(byte[] aid, int route, int power);

    @Override
    public native boolean unrouteAid(byte[] aid);

/* START [P160421001] - Patch for Dynamic SE Selection */
    @Override
    public native boolean setRoutingEntry(int type, int value, int route, int power);

    @Override
    public native boolean clearRoutingEntry(int type);

    @Override
    public native void doSetDefaultSe(int defaultSeId);

    @Override
    public native int doGetDefaultSe();
/* END [P160421001] - Patch for Dynamic SE Selection */

/* START [P161006001] - Configure Default SecureElement and LMRT */
    @Override
    public native void SetDefaultRoute(int defaultSeId);
/* END [P161006001] - Configure Default SecureElement and LMRT */

    @Override
    public native boolean commitRouting();

    @Override
    public void registerT3tIdentifier(byte[] t3tIdentifier) {
        return;
    }

    @Override
    public void deregisterT3tIdentifier(byte[] t3tIdentifier) {
        return;
    }

    @Override
    public void clearT3tIdentifiersCache() {
        return;
    }

    @Override
    public int getLfT3tMax() {
        return 0;
    }

    private native void doEnableDiscovery(int techMask,
                                          boolean enableLowPowerPolling,
                                          boolean enableReaderMode,
                                          boolean enableHostRouting,
                                          boolean enableP2p,
                                          boolean restart);
    @Override
    public void enableDiscovery(NfcDiscoveryParameters params, boolean restart) {
        doEnableDiscovery(params.getTechMask(), params.shouldEnableLowPowerDiscovery(),
                params.shouldEnableReaderMode(), params.shouldEnableHostRouting(),
                params.shouldEnableP2p(), restart);
    }

    @Override
    public native void disableDiscovery();

    private native NativeLlcpConnectionlessSocket doCreateLlcpConnectionlessSocket(int nSap,
            String sn);

    @Override
    public LlcpConnectionlessSocket createLlcpConnectionlessSocket(int nSap, String sn)
            throws LlcpException {
        LlcpConnectionlessSocket socket = doCreateLlcpConnectionlessSocket(nSap, sn);
        if (socket != null) {
            return socket;
        } else {
            /* Get Error Status */
            int error = doGetLastError();

            Log.d(TAG, "failed to create llcp socket: " + ErrorCodes.asString(error));

            switch (error) {
                case ErrorCodes.ERROR_BUFFER_TO_SMALL:
                case ErrorCodes.ERROR_INSUFFICIENT_RESOURCES:
                    throw new LlcpException(error);
                default:
                    throw new LlcpException(ErrorCodes.ERROR_SOCKET_CREATION);
            }
        }
    }

    private native NativeLlcpServiceSocket doCreateLlcpServiceSocket(int nSap, String sn, int miu,
            int rw, int linearBufferLength);
    @Override
    public LlcpServerSocket createLlcpServerSocket(int nSap, String sn, int miu,
            int rw, int linearBufferLength) throws LlcpException {
        LlcpServerSocket socket = doCreateLlcpServiceSocket(nSap, sn, miu, rw, linearBufferLength);
        if (socket != null) {
            return socket;
        } else {
            /* Get Error Status */
            int error = doGetLastError();

            Log.d(TAG, "failed to create llcp socket: " + ErrorCodes.asString(error));

            switch (error) {
                case ErrorCodes.ERROR_BUFFER_TO_SMALL:
                case ErrorCodes.ERROR_INSUFFICIENT_RESOURCES:
                    throw new LlcpException(error);
                default:
                    throw new LlcpException(ErrorCodes.ERROR_SOCKET_CREATION);
            }
        }
    }

    private native NativeLlcpSocket doCreateLlcpSocket(int sap, int miu, int rw,
            int linearBufferLength);
    @Override
    public LlcpSocket createLlcpSocket(int sap, int miu, int rw,
            int linearBufferLength) throws LlcpException {
        LlcpSocket socket = doCreateLlcpSocket(sap, miu, rw, linearBufferLength);
        if (socket != null) {
            return socket;
        } else {
            /* Get Error Status */
            int error = doGetLastError();

            Log.d(TAG, "failed to create llcp socket: " + ErrorCodes.asString(error));

            switch (error) {
                case ErrorCodes.ERROR_BUFFER_TO_SMALL:
                case ErrorCodes.ERROR_INSUFFICIENT_RESOURCES:
                    throw new LlcpException(error);
                default:
                    throw new LlcpException(ErrorCodes.ERROR_SOCKET_CREATION);
            }
        }
    }

    @Override
    public native boolean doCheckLlcp();

    @Override
    public native boolean doActivateLlcp();

    private native void doResetTimeouts();

    @Override
    public void resetTimeouts() {
        doResetTimeouts();
    }

    @Override
    public native void doAbort();

    private native boolean doSetTimeout(int tech, int timeout);
    @Override
    public boolean setTimeout(int tech, int timeout) {
        return doSetTimeout(tech, timeout);
    }

    private native int doGetTimeout(int tech);
    @Override
    public int getTimeout(int tech) {
        return doGetTimeout(tech);
    }


    @Override
    public boolean canMakeReadOnly(int ndefType) {
        return (ndefType == Ndef.TYPE_1 || ndefType == Ndef.TYPE_2);
    }

    @Override
    public int getMaxTransceiveLength(int technology) {
        switch (technology) {
            case (TagTechnology.NFC_A):
            case (TagTechnology.MIFARE_CLASSIC):
            case (TagTechnology.MIFARE_ULTRALIGHT):
                return 253; // PN544 RF buffer = 255 bytes, subtract two for CRC
            case (TagTechnology.NFC_B):
                /////////////////////////////////////////////////////////////////
                // Broadcom: Since BCM2079x supports this, set NfcB max size.
                //return 0; // PN544 does not support transceive of raw NfcB
                return 253; // PN544 does not support transceive of raw NfcB
            case (TagTechnology.NFC_V):
                return 253; // PN544 RF buffer = 255 bytes, subtract two for CRC
            case (TagTechnology.ISO_DEP):
                /* The maximum length of a normal IsoDep frame consists of:
                 * CLA, INS, P1, P2, LC, LE + 255 payload bytes = 261 bytes
                 * such a frame is supported. Extended length frames however
                 * are not supported.
                 */
                return 261; // Will be automatically split in two frames on the RF layer
            case (TagTechnology.NFC_F):
                return 252; // PN544 RF buffer = 255 bytes, subtract one for SoD, two for CRC
            default:
                return 0;
        }

    }

    private native void doSetP2pInitiatorModes(int modes);
    @Override
    public void setP2pInitiatorModes(int modes) {
        doSetP2pInitiatorModes(modes);
    }

    private native void doSetP2pTargetModes(int modes);
    @Override
    public void setP2pTargetModes(int modes) {
        doSetP2pTargetModes(modes);
    }

    @Override
    public boolean getExtendedLengthApdusSupported() {
        // TODO check BCM support
        return false;
    }

/* START [J15020401] - Enable WCE with UICC/eSE. */
    @Override
    public byte[][] getWipeApdus() {
        return EE_WIPE_APDUS;
    }
/* END [J15020401] - Enable WCE with UICC/eSE. */

    @Override
    public int getDefaultLlcpMiu() {
        return DEFAULT_LLCP_MIU;
    }

    @Override
    public int getDefaultLlcpRwSize() {
        return DEFAULT_LLCP_RWSIZE;
    }

    private native String doDump();
    @Override
    public String dump() {
        return doDump();
    }

    private native void doEnableScreenOffSuspend();
    @Override
    public boolean enableScreenOffSuspend() {
        doEnableScreenOffSuspend();
        return true;
    }

    private native void doDisableScreenOffSuspend();
    @Override
    public boolean disableScreenOffSuspend() {
        doDisableScreenOffSuspend();
        return true;
    }

/* START [J1504130001] - Support SE on Device Power Off State */
    @Override
    public native void doSetScreenOrPowerState(int state);
/* END [J1504130001] - Support SE on Device Power Off State */

/* START [P1604040001] - Support Dual-SIM solution */
    @Override
    public native void doSetPreferredSimSlot(int preferredSim);
/* END [P1604040001] - Support Dual-SIM solution */

    /**
     * Notifies Ndef Message (TODO: rename into notifyTargetDiscovered)
     */
    private void notifyNdefMessageListeners(NativeNfcTag tag) {
        mListener.onRemoteEndpointDiscovered(tag);
    }

/* START [J15020401] - Enable WCE with UICC/eSE. */
    /**
     * Notifies transaction
     */
    private void notifyTargetDeselected() {
        mListener.onCardEmulationDeselected();
    }

    /**
     * Notifies transaction
     */
    private void notifyTransactionListeners(byte[] aid, byte[] data, int evtSrc) {
        mListener.onCardEmulationAidSelected(aid, data, evtSrc);
    }
/* END [J15020401] - Enable WCE with UICC/eSE. */

    /**
     * Notifies P2P Device detected, to activate LLCP link
     */
    private void notifyLlcpLinkActivation(NativeP2pDevice device) {
        mListener.onLlcpLinkActivated(device);
    }

    /**
     * Notifies P2P Device detected, to activate LLCP link
     */
    private void notifyLlcpLinkDeactivated(NativeP2pDevice device) {
        mListener.onLlcpLinkDeactivated(device);
    }

    /**
     * Notifies first packet received from remote LLCP
     */
    private void notifyLlcpLinkFirstPacketReceived(NativeP2pDevice device) {
        mListener.onLlcpFirstPacketReceived(device);
    }

/* START [J15020401] - Enable WCE with UICC/eSE. */
    private void notifySeFieldActivated() {
        mListener.onRemoteFieldActivated();
    }

    private void notifySeFieldDeactivated() {
        mListener.onRemoteFieldDeactivated();
    }

    private void notifySeListenActivated() {
        mListener.onSeListenActivated();
    }

    private void notifySeListenDeactivated() {
        mListener.onSeListenDeactivated();
    }

    private void notifySeApduReceived(byte[] apdu) {
        mListener.onSeApduReceived(apdu);
    }

    public native int getFWVersion();

    //public native boolean setRoutingEntry(int type, int value, int route, int power); 

    //public native boolean clearRoutingEntry(int type); 

    public native void doPrbsOn(int tech, int rate); 

    public native void doPrbsOff(); 

    public native int SWPSelfTest(int ch);

    private void notifySeEmvCardRemoval() {
        mListener.onSeEmvCardRemoval();
    }

    private void notifySeMifareAccess(byte[] block) {
        mListener.onSeMifareAccess(block);
    }
/* END [J15020401] - Enable WCE with UICC/eSE. */

/* START [J15100101] - Get Tech information */
    public native int doGetSeSupportedTech();
/* END [J15100101] - Get Tech information */

/* START [J14121201] - Aid Table Size */
    public native int getAidTableSize();
/* END [J14121201] - Aid Table Size */

/* START [J14121101] - Remaining AID Size */
    public native int getRemainingAidTableSize();
/* END [J14121101] - Remaining AID Size */



    private void notifyHostEmuActivated() {
        mListener.onHostCardEmulationActivated();
    }

    private void notifyHostEmuData(byte[] data) {
        mListener.onHostCardEmulationData(data);
    }

    private void notifyHostEmuDeactivated() {
        mListener.onHostCardEmulationDeactivated();
    }

    private void notifyRfFieldActivated() {
        mListener.onRemoteFieldActivated();
    }

    private void notifyRfFieldDeactivated() {
        mListener.onRemoteFieldDeactivated();
    }

    private void notifyAidRoutingTableFull() {
        //mSeListener.onRemoteFieldDeactivated();
    }

/* START [J00000004] - Flip cover */
    @Override
    public native byte[] transceiveAuthData(byte[] data);

    @Override
    public native byte[] startCoverAuth();

    @Override
    public native boolean stopCoverAuth();
/* END [J00000004] */

/* START [J15052702] - FW Update Req : FW Update Notify*/
    private void notifyFirmwareDownloadStatus(int status) {
        Log.d(TAG, "notifyFirmwareDownloadStatus " + status);
        //mListener.onFirmwareDownloadStatusNotify(status);
    }
/* END [J15052702] - FW Update Req : FW Update Notify*/

}
