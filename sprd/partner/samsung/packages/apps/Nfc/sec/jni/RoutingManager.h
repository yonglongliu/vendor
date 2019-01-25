/*
 * Copyright (C) 2013 The Android Open Source Project
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

/*
 *  Manage the listen-mode routing table.
 */
#pragma once
#include "SyncEvent.h"
#include "NfcJniUtil.h"
#include "RouteDataSet.h"
/* START [J14111100] - support secure element */
#include "SecureElement.h"
/* END [J14111100] - support secure element */
#include <vector>
extern "C"
{
    #include "nfa_api.h"
    #include "nfa_ee_api.h"
}

class RoutingManager
{
public:
    static RoutingManager& getInstance ();
    bool initialize(nfc_jni_native_data* native);
    void enableRoutingToHost();
    void disableRoutingToHost();

/* START [160616002P] - Update LMRT if it update by req notification.*/
    void setRoutingTableByReqNtf();
/* END [160616002P] - Update LMRT if it update by req notification.*/

/* START [J14111200] - Routing setting */
    void setDefaultTechRouting (int seId, int value, int power);
    void setDefaultProtoRouting (int seId, int value, int power);
    bool clearRouting(int type);
/* END [J14111200] */

/* START [J14127009] - route aid for power state */
    bool addAidRouting(const UINT8* aid, UINT8 aidLen, int route, int power);
/* END [J14127009] */

/* START [J14120901] - set ce listen tech */
    bool useAidOnDH(bool useAidOnDH);
    unsigned long getCeListenTech(void);
    void setCeListenTech (unsigned long tech_mask);
/* END [J14120901] */

/* START [J15072201] - pending enable discovery during listen mode */
    void setCeDiscoveryMask (unsigned long tech_mask);
/* END [J15072201] - pending enable discovery during listen mode */

/* START [J15100101] - Get Tech information */
    int getSeSupportedTech ();
/* END [J15100101] */

/* START [P160421001] - Patch for Dynamic SE Selection */
    void setDefaultSe(int defaultSeId);
    int getDefaultSe();
/* END [P160421001] - Patch for Dynamic SE Selection */

    bool removeAidRouting(const UINT8* aid, UINT8 aidLen);
    bool commitRouting();
    void onNfccShutdown();
    int registerJniFunctions (JNIEnv* e);
private:
    RoutingManager();
    ~RoutingManager();
    RoutingManager(const RoutingManager&);
    RoutingManager& operator=(const RoutingManager&);

    void handleData (const UINT8* data, UINT32 dataLen, tNFA_STATUS status);
    void notifyActivated ();
    void notifyDeactivated ();
    void notifyLmrtFull ();

/* START [J15100101] - Get Tech information */
    void setSeSupportedTech ();
/* END [J15100101] */

    // See AidRoutingManager.java for corresponding
    // AID_MATCHING_ constants

/* START [TODO2014112702] */
    // Every routing table entry is matched exact (SLSI: TODO: SUPERSET(0x01) , SUBSET(0x02), PARTTERN(0x03))
/* END [TODO2014112702] */
    static const int AID_MATCHING_EXACT_ONLY = 0x00;
    // Every routing table entry can be matched either exact or prefix
    static const int AID_MATCHING_EXACT_OR_PREFIX = 0x01;
    // Every routing table entry is matched as a prefix
    static const int AID_MATCHING_PREFIX_ONLY = 0x02;

    static void nfaEeCallback (tNFA_EE_EVT event, tNFA_EE_CBACK_DATA* eventData);
    static void stackCallback (UINT8 event, tNFA_CONN_EVT_DATA* eventData);
    static int com_android_nfc_cardemulation_doGetDefaultRouteDestination (JNIEnv* e);
    static int com_android_nfc_cardemulation_doGetDefaultOffHostRouteDestination (JNIEnv* e);
    static int com_android_nfc_cardemulation_doGetAidMatchingMode (JNIEnv* e);

    std::vector<UINT8> mRxDataBuffer;

    // Fields below are final after initialize()
    nfc_jni_native_data* mNativeData;
    int mDefaultEe;
    int mOffHostEe;
    int mActiveSe;
    int mAidMatchingMode;
/* START [J15100101] - Get Tech information */
    int mSeSupportedTech;
/* END [J15100101] */

    bool mReceivedEeInfo;
    tNFA_EE_DISCOVER_REQ mEeInfo;
    tNFA_TECHNOLOGY_MASK mSeTechMask;
    tNFA_TECHNOLOGY_MASK mSeTechMaskonSubScreenState [3];
    tNFA_PROTOCOL_MASK mSeProtoMask;
    tNFA_PROTOCOL_MASK mSeProtoMaskonSubScreenState [3];
    static const JNINativeMethod sMethods [];
    SyncEvent mEeRegisterEvent;
    SyncEvent mRoutingEvent;
    SyncEvent mAidEvent;      /* [J14127009] */
    SyncEvent mEeUpdateEvent;
    SyncEvent mEeInfoEvent;
    SyncEvent mEeSetModeEvent;
/* START [J14120901] - set ce listen tech */
    tNFA_HANDLE mAidOnDH;
/* END [J14120901] */
};
