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

#include <cutils/log.h>
#include <ScopedLocalRef.h>
#include <JNIHelp.h>
#include "config.h"
#include "JavaClassConstants.h"
#include "RoutingManager.h"

#include "PeerToPeer.h"

extern "C"
{
    #include "nfa_ee_api.h"
    #include "nfa_ce_api.h"
}
extern bool gActivated;
extern SyncEvent gDeactivatedEvent;

namespace android
{
/***** S.LSI Common Module *****/
    extern void slsiEventHandler (UINT8 event, tNFA_CONN_EVT_DATA* data);

/* START [J14127010] - Stop/Start discovery for setting LMRT */
    extern void startRfDiscovery(bool isStart);
/* END [J14127010] */
/* START [J14120201] - Generic ESE ID */
    extern jint slsiGetGenSeId(int seId);
/* END [J14120201] */
}

const JNINativeMethod RoutingManager::sMethods [] =
{
    {"doGetDefaultRouteDestination", "()I", (void*) RoutingManager::com_android_nfc_cardemulation_doGetDefaultRouteDestination},
    {"doGetDefaultOffHostRouteDestination", "()I", (void*) RoutingManager::com_android_nfc_cardemulation_doGetDefaultOffHostRouteDestination},
    {"doGetAidMatchingMode", "()I", (void*) RoutingManager::com_android_nfc_cardemulation_doGetAidMatchingMode}
};

static const int MAX_NUM_EE = 5;
/* START [J14111200] - Routing setting */
#define SEID_HCE        0x00
#define SEID_ESE        0x02
#define SEID_UICC       0x03
#define SEID_MAX        0x04

static tNFA_TECHNOLOGY_MASK gTechRouting[SEID_MAX][6] = {0,};
static tNFA_TECHNOLOGY_MASK gProtoRouting[SEID_MAX][6] = {0,};
/* END [J14111200] */

/* START [J14127005] - UICC_LISTEN_TECH_MASK Configuration */
unsigned long mUiccListenMask = NFA_TECHNOLOGY_MASK_A | NFA_TECHNOLOGY_MASK_B;
/* END [J14127005] */

/* START [J14127006] - AID RegisterStatus */
tNFA_EE_CBACK_DATA sAidRegisterStatus;
/* END [J14127006] */

RoutingManager::RoutingManager ()
{
    static const char fn [] = "RoutingManager::RoutingManager()";
    unsigned long num = 0;

    // Get the active SE
    if (GetNumValue("ACTIVE_SE", &num, sizeof(num)))
        mActiveSe = num;
    else
        mActiveSe = 0x00;

    // Get the "default" route
    if (GetNumValue("DEFAULT_ISODEP_ROUTE", &num, sizeof(num)))
        mDefaultEe = num;
    else
        mDefaultEe = 0x00;
    ALOGD("%s: default route is 0x%02X", fn, mDefaultEe);

    // Get the default "off-host" route.  This is hard-coded at the Java layer
    // but we can override it here to avoid forcing Java changes.
    if (GetNumValue("DEFAULT_OFFHOST_ROUTE", &num, sizeof(num)))
        mOffHostEe = num;
    else
        mOffHostEe = 0x03;

/* START [J15012601] - Listen Tech Mask for CE & HCE */
    if (GetNumValue("UICC_LISTEN_TECH_MASK", &num, sizeof(num)))
        mUiccListenMask = num;
    else
        mUiccListenMask = NFA_TECHNOLOGY_MASK_A | NFA_TECHNOLOGY_MASK_B;
/* END [J15012601] - Listen Tech Mask for CE & HCE */

    if (GetNumValue("AID_MATCHING_MODE", &num, sizeof(num)))
        mAidMatchingMode = num;
    else
        mAidMatchingMode = AID_MATCHING_EXACT_ONLY;

/* START [J14121701] - aid matching value from partial aid configuration */
    UINT8 buffer[10];
    int len = GetStrValue(NAME_PARTIAL_AID, (char *)buffer, sizeof(buffer));
    int lastMatchCode;
    for (lastMatchCode = len; lastMatchCode > 0; lastMatchCode--)
    {
        if (buffer[lastMatchCode] > 0x03)
            break;
    }

    ALOGD ("%s: lastMatchCode = 0x%X", fn, lastMatchCode);
    switch (buffer[lastMatchCode])
    {
    case 0x4:
        mAidMatchingMode = AID_MATCHING_EXACT_ONLY;
        break;
    case 0x5:
        mAidMatchingMode = AID_MATCHING_EXACT_OR_PREFIX;
        break;
    case 0x6:
        mAidMatchingMode = AID_MATCHING_PREFIX_ONLY;
        break;
    }
    ALOGD ("%s: mAidMatchingMode = 0x%X", fn, mAidMatchingMode);
/* END [J14121701] - aid matching value from partial aid configuration */

    ALOGD("%s: mOffHostEe=0x%02X", fn, mOffHostEe);

    memset (&mEeInfo, 0, sizeof(mEeInfo));
    mReceivedEeInfo = false;
    mSeTechMask = NFA_TECHNOLOGY_MASK_A | NFA_TECHNOLOGY_MASK_B;
    mSeProtoMask = NFA_PROTOCOL_MASK_ISO_DEP;
    memset (&mSeTechMaskonSubScreenState, NFA_TECHNOLOGY_MASK_A | NFA_TECHNOLOGY_MASK_B, sizeof(mSeTechMaskonSubScreenState));
    memset (&mSeProtoMaskonSubScreenState, NFA_PROTOCOL_MASK_ISO_DEP, sizeof(mSeProtoMaskonSubScreenState));
/* START [J15100101] - Get Tech information */
    mSeSupportedTech = 0x00;
/* END [J15100101] - Get Tech information */
}

RoutingManager::~RoutingManager ()
{
    NFA_EeDeregister (nfaEeCallback);
}

bool RoutingManager::initialize (nfc_jni_native_data* native)
{
    static const char fn [] = "RoutingManager::initialize()";
    tNFA_TECHNOLOGY_MASK hce_listen_tech_mask = NFA_TECHNOLOGY_MASK_A | NFA_TECHNOLOGY_MASK_B;
    unsigned long num = 0;
    mNativeData = native;

    tNFA_STATUS nfaStat;
    {
        SyncEventGuard guard (mEeRegisterEvent);
        ALOGD ("%s: try ee register", fn);
        nfaStat = NFA_EeRegister (nfaEeCallback);
        if (nfaStat != NFA_STATUS_OK)
        {
            ALOGE ("%s: fail ee register; error=0x%X", fn, nfaStat);
            return false;
        }
        mEeRegisterEvent.wait ();
    }

    mRxDataBuffer.clear ();

/* START [P160421001] - Patch for Dynamic SE Selection */
    if (mDefaultEe != 0) {
        clearRouting(0x07);         // Clear LMRT Information.
/* END [P160421001] - Patch for Dynamic SE Selection */

        {
            // Wait for EE info if needed
                SyncEventGuard guard (mEeInfoEvent);
                if (!mReceivedEeInfo)
                {
                    ALOGE("Waiting for EE info");
                    mEeInfoEvent.wait();
                }
            }
    }

    // Tell the host-routing to only listen on mUiccListenMask Options
    /* START [J15012601] - Listen Tech Mask for CE & HCE */
    nfaStat = NFA_CeSetIsoDepListenTech(mUiccListenMask);
    /* END [J15012601] - Listen Tech Mask for CE & HCE */
    if (nfaStat != NFA_STATUS_OK)
        ALOGE ("Failed to configure CE IsoDep technologies ... (%x)", nfaStat);

    // Register a wild-card for AIDs routed to the host
    nfaStat = NFA_CeRegisterAidOnDH (NULL, 0, stackCallback);
    if (nfaStat != NFA_STATUS_OK)
        ALOGE("Failed to register wildcard AID for DH ... (%x)", nfaStat);

    return true;
}

/* START [160616002P] - Update LMRT if it update by req notification.*/
void RoutingManager::setRoutingTableByReqNtf()
{
/* START [160616002P] - Update LMRT if it update by req notification.*/
    bool bIsUpdateLMRT = false;
/* END [160616002P] - Update LMRT if it update by req notification.*/
    tNFA_STATUS nfaStat;
    static const char fn [] = "RoutingManager::setRoutingTableByReqNtf()";

    ALOGD ("%s : Enter .... num_ee (0x%x), mSeTechMask(0x%x).", fn,  mEeInfo.num_ee, mSeTechMask);

        for (UINT8 i = 0; i < mEeInfo.num_ee; i++)
        {
            ALOGD ("%s   EE[%u] Handle: 0x%04x  techA: 0x%02x  techB: 0x%02x  techF: 0x%02x  techBprime: 0x%02x",
                fn, i, mEeInfo.ee_disc_info[i].ee_handle,
                mEeInfo.ee_disc_info[i].la_protocol,
                mEeInfo.ee_disc_info[i].lb_protocol,
                mEeInfo.ee_disc_info[i].lf_protocol,
                mEeInfo.ee_disc_info[i].lbp_protocol);

/* START [P160421001] - Patch for Dynamic SE Selection */
            if (mEeInfo.ee_disc_info[i].ee_handle == (mDefaultEe | NFA_HANDLE_GROUP_EE))
/* END [P160421001] - Patch for Dynamic SE Selection */
            {
                ALOGD ("%s : Update LMRT for new default SE. (bIsUpdateLMRT = %d)", fn, bIsUpdateLMRT);

                bIsUpdateLMRT = true;
/* START [160616002P] - Update LMRT if it update by req notification.*/
                mSeTechMask = NFA_TECHNOLOGY_MASK_A | NFA_TECHNOLOGY_MASK_B;
                mSeTechMaskonSubScreenState[0] = NFA_TECHNOLOGY_MASK_A | NFA_TECHNOLOGY_MASK_B;
                mSeTechMaskonSubScreenState[1] = NFA_TECHNOLOGY_MASK_A | NFA_TECHNOLOGY_MASK_B;
                mSeTechMaskonSubScreenState[2] = 0;
/* END [160616002P] - Update LMRT if it update by req notification.*/

/* START [160616001P] - Added CE listen tech for F-Type UICC*/
                if (mEeInfo.ee_disc_info[i].lf_protocol != 0)
                {
                    mSeTechMask |= NFA_TECHNOLOGY_MASK_F;
                    mSeTechMaskonSubScreenState[0] |= NFA_TECHNOLOGY_MASK_F;
                    mSeTechMaskonSubScreenState[1] |= NFA_TECHNOLOGY_MASK_F;
                    mSeTechMaskonSubScreenState[2] |= 0;
                }
/* END [160616001P] - Added CE listen tech for F-Type UICC*/

/* START [P1605170002] - Change CE listen tech configuration method for support 0x00 tech*/
                ALOGD("%s : Configuring tech mask 0x%02x on EE 0x%04x", fn, mSeTechMask, mEeInfo.ee_disc_info[i].ee_handle);
                nfaStat = NFA_CeConfigureUiccListenTech(mEeInfo.ee_disc_info[i].ee_handle, mSeTechMask);
                if (nfaStat != NFA_STATUS_OK)
                    ALOGE ("Failed to configure UICC listen technologies.");
                // Set technology routes to UICC if it's there
                nfaStat = NFA_EeSetDefaultTechRouting(mEeInfo.ee_disc_info[i].ee_handle, mSeTechMask, mSeTechMask,
                                                        0, mSeTechMaskonSubScreenState);
                if (nfaStat != NFA_STATUS_OK)
                    ALOGE ("Failed to configure UICC technology routing.");

                if ( ((mSeTechMask & NFA_TECHNOLOGY_MASK_A) == NFA_TECHNOLOGY_MASK_A) ||
                    ((mSeTechMask & NFA_TECHNOLOGY_MASK_B) == NFA_TECHNOLOGY_MASK_B) )
/* END [P1605170002] - Change CE listen tech configuration method for support 0x00 tech*/
                {
                    // Set protocol routes to DefaultSE if it's thers
                    mSeProtoMask = NFA_PROTOCOL_MASK_ISO_DEP;
                    mSeProtoMaskonSubScreenState[0] = NFA_PROTOCOL_MASK_ISO_DEP;
                    mSeProtoMaskonSubScreenState[1] = NFA_PROTOCOL_MASK_ISO_DEP;
                    mSeProtoMaskonSubScreenState[2] = 0;
                }

/* START [160616001P] - Added CE listen tech for F-Type UICC*/
                if ((mSeTechMask & NFA_TECHNOLOGY_MASK_F) == NFA_TECHNOLOGY_MASK_F)
                {
                    // Set protocol routes to DefaultSE if it's thers
                    mSeProtoMask |= NFA_PROTOCOL_MASK_T3T;
                    mSeProtoMaskonSubScreenState[0] |= NFA_PROTOCOL_MASK_T3T;
                    mSeProtoMaskonSubScreenState[1] |= NFA_PROTOCOL_MASK_T3T;
                    mSeProtoMaskonSubScreenState[2] |= 0;
                }
/* END [160616001P] - Added CE listen tech for F-Type UICC*/

                if(mSeProtoMask != 0x00)
                {
                    nfaStat = NFA_EeSetDefaultProtoRouting(mDefaultEe, mSeProtoMask,
                                                    mSeProtoMask, 0, mSeProtoMaskonSubScreenState);
                    if (nfaStat != NFA_STATUS_OK)
                        ALOGE ("Failed to configure F-UICC technology routing.");
                }
            }
        }

/* START [160616002P] - Update LMRT if it update by req notification.*/
    if( (mEeInfo.num_ee > 0) && (bIsUpdateLMRT) )
/* END [160616002P] - Update LMRT if it update by req notification.*/
    {
        nfaStat = NFA_EeUpdateNow();
        if (nfaStat != NFA_STATUS_OK)
            ALOGE ("Failed to update LMRT table.");
    }

    ALOGD ("%s: Exit mSeTechMask(0x%x) ", fn, mSeTechMask);
}
/* END [160616002P] - Update LMRT if it update by req notification.*/

RoutingManager& RoutingManager::getInstance ()
{
    static RoutingManager manager;
    return manager;
}

void RoutingManager::enableRoutingToHost()
{
    tNFA_STATUS nfaStat;
    ALOGD("%s: Enter .........mDefaultEe(0x%x)", __FUNCTION__, mDefaultEe);
    {
        SyncEventGuard guard (mRoutingEvent);

        // Route Nfc-A to host if we don't have a SE( Run during Screen on)
/* START [160616002P] - Update LMRT if it update by req notification.*/
        if (mDefaultEe == 0x00)
        {
            ALOGD("%s: [1] mDefaultEe(0x%x) , mSeTechMask(0x%x),  ", __FUNCTION__, mDefaultEe, mSeTechMask);
            mSeTechMaskonSubScreenState[0] = 0;
            mSeTechMaskonSubScreenState[1] = NFA_TECHNOLOGY_MASK_A | NFA_TECHNOLOGY_MASK_B;
            mSeTechMaskonSubScreenState[2] = 0;
            nfaStat = NFA_EeSetDefaultTechRouting (mDefaultEe, mSeTechMask, 0, 0, mSeTechMaskonSubScreenState);
        }
        else
        {
            mSeTechMaskonSubScreenState[0] = NFA_TECHNOLOGY_MASK_A | NFA_TECHNOLOGY_MASK_B;
            mSeTechMaskonSubScreenState[1] = NFA_TECHNOLOGY_MASK_A | NFA_TECHNOLOGY_MASK_B;
            mSeTechMaskonSubScreenState[2] = 0;
            nfaStat = NFA_EeSetDefaultTechRouting (mDefaultEe, mSeTechMask, mSeTechMask, 0, mSeTechMaskonSubScreenState);

        }
        if (nfaStat == NFA_STATUS_OK)
            mRoutingEvent.wait ();
        else
            ALOGE ("Fail to set default tech routing");
/* END [160616002P] - Update LMRT if it update by req notification.*/

        // Default routing for IsoDep protocol
        if (mDefaultEe == 0x00)
        {
            mSeProtoMaskonSubScreenState[0] = 0;
            mSeProtoMaskonSubScreenState[1] = NFA_PROTOCOL_MASK_ISO_DEP;
            mSeProtoMaskonSubScreenState[2] = 0;
            nfaStat = NFA_EeSetDefaultProtoRouting(mDefaultEe, mSeProtoMask, 0, 0, mSeProtoMaskonSubScreenState);
            if (nfaStat == NFA_STATUS_OK)
                mRoutingEvent.wait ();
            else
                ALOGE ("Fail to set default proto routing");
        }
        else
        {
            if ( ((mSeTechMask & NFA_TECHNOLOGY_MASK_A) == NFA_TECHNOLOGY_MASK_A) ||
                ((mSeTechMask & NFA_TECHNOLOGY_MASK_B) == NFA_TECHNOLOGY_MASK_B) )
            {
                mSeProtoMask = NFA_PROTOCOL_MASK_ISO_DEP;
                mSeProtoMaskonSubScreenState[0] = NFA_PROTOCOL_MASK_ISO_DEP;
                mSeProtoMaskonSubScreenState[1] = NFA_PROTOCOL_MASK_ISO_DEP;
                mSeProtoMaskonSubScreenState[2] = 0;
            }

/* START [160616001P] - Added CE listen tech for F-Type UICC*/
            if ((mSeTechMask & NFA_TECHNOLOGY_MASK_F) == NFA_TECHNOLOGY_MASK_F)
            {
                mSeProtoMask |= NFA_PROTOCOL_MASK_T3T;
                mSeProtoMaskonSubScreenState[0] |= NFA_PROTOCOL_MASK_T3T;
                mSeProtoMaskonSubScreenState[1] |= NFA_PROTOCOL_MASK_T3T;
                mSeProtoMaskonSubScreenState[2] = 0;
            }
            ALOGD("%s: [2] mDefaultEe(0x%x) , Protoocl(0x%x),  ", __FUNCTION__, mDefaultEe, mSeProtoMask);
            if(mSeProtoMask != 0x00)
            {
                nfaStat = NFA_EeSetDefaultProtoRouting(mDefaultEe, mSeProtoMask,
                                   mSeProtoMask, 0, mSeProtoMaskonSubScreenState);
                if (nfaStat != NFA_STATUS_OK)
                ALOGE ("Failed to configure technology routing for default SE.");
            }
             /* END [160616001P] - Added CE listen tech for F-Type UICC*/
        }
    }
    ALOGD("%s: Exit", __FUNCTION__);
}

void RoutingManager::disableRoutingToHost()
{
    tNFA_STATUS nfaStat;

    ALOGD("%s: Enter", __FUNCTION__);

    {
        SyncEventGuard guard (mRoutingEvent);
        // Default routing for NFC-A technology if we don't have a SE
        {
/* START [J16012801] - set LMRT power config for screen state */
            mSeTechMaskonSubScreenState[0] = 0;
            mSeTechMaskonSubScreenState[1] = 0;
            mSeTechMaskonSubScreenState[2] = 0;
/* END [J16012801] - set LMRT power config for screen state */
            nfaStat = NFA_EeSetDefaultTechRouting (mDefaultEe, 0, 0, 0, mSeTechMaskonSubScreenState);
        }
            if (nfaStat == NFA_STATUS_OK)
                mRoutingEvent.wait ();
            else
                ALOGE ("Fail to set default tech routing");

        // Default routing for IsoDep protocol
/* START [J16012801] - set LMRT power config for screen state */
        {
            mSeProtoMaskonSubScreenState[0] = 0;
            mSeProtoMaskonSubScreenState[1] = 0;
            mSeProtoMaskonSubScreenState[2] = 0;
            nfaStat = NFA_EeSetDefaultProtoRouting(mDefaultEe, 0, 0, 0, mSeProtoMaskonSubScreenState);
        }
/* END [J16012801] - set LMRT power config for screen state */
        if (nfaStat == NFA_STATUS_OK)
            mRoutingEvent.wait ();
        else
            ALOGE ("Fail to set default proto routing");
    }
}

/* START [J14127009] - route aid for power state */
bool RoutingManager::addAidRouting(const UINT8* aid, UINT8 aidLen, int route, int power)
{
    static const char fn [] = "RoutingManager::addAidRouting";
    ALOGD ("%s: enter  (route = 0x%x )", fn, route);
    SyncEventGuard guard (mAidEvent);
    tNFA_STATUS nfaStat = NFA_EeAddAidRouting(route, aidLen, (UINT8*) aid, power);
/* END [J14127009] */

    if (nfaStat == NFA_STATUS_OK)
    {
/* START [J14127006] - AID RegisterStatus */
        mAidEvent.wait ();
        if(sAidRegisterStatus.status == NFA_STATUS_OK) {
            ALOGD ("%s: routed AID : %u", fn, sAidRegisterStatus.status);
            return true;
        }
        else {
            ALOGE ("%s: failed to route AID : %u", fn, sAidRegisterStatus.status);
            return false;
        }
/* END [J14127006] */
    } else
    {
        ALOGE ("%s: failed to route AID", fn);
        return false;
    }
}

bool RoutingManager::removeAidRouting(const UINT8* aid, UINT8 aidLen)
{
    static const char fn [] = "RoutingManager::removeAidRouting";
    ALOGD ("%s: enter", fn);
    SyncEventGuard guard (mAidEvent);
    tNFA_STATUS nfaStat = NFA_EeRemoveAidRouting(aidLen, (UINT8*) aid);
    if (nfaStat == NFA_STATUS_OK)
    {
/* START [J14127006] - AID RegisterStatus */
        mAidEvent.wait ();
        if(sAidRegisterStatus.status == NFA_STATUS_OK) {
            ALOGD ("%s: routed AID : %u", fn, sAidRegisterStatus.status);
            return true;
        }
        else {
            ALOGE ("%s: failed to route AID : %u", fn, sAidRegisterStatus.status);
            return false;
        }
/* END [J14127006] */
    } else
    {
        ALOGE ("%s: failed to remove AID", fn);
        return false;
    }
}

bool RoutingManager::commitRouting()
{
    static const char fn [] = "RoutingManager::commitRouting";
    tNFA_STATUS nfaStat = 0;
    ALOGD ("%s", fn);
/* START [J14127010] - Stop/Start discovery for setting LMRT */
    android::startRfDiscovery (false);
/* END [J14127010] */
    {
        SyncEventGuard guard (mEeUpdateEvent);
        nfaStat = NFA_EeUpdateNow();
        if (nfaStat == NFA_STATUS_OK)
        {
            mEeUpdateEvent.wait (); //wait for NFA_EE_UPDATED_EVT
        }
    }
/* START [J14127010] - Stop/Start discovery for setting LMRT */
    android::startRfDiscovery (true);
/* END [J14127010] */
    return (nfaStat == NFA_STATUS_OK);
}

void RoutingManager::onNfccShutdown ()
{
    static const char fn [] = "RoutingManager:onNfccShutdown";
/* START [P160421001] - Patch for Dynamic SE Selection */
    if (mDefaultEe != 0x00) return;
/* END [P160421001] - Patch for Dynamic SE Selection */

    tNFA_STATUS nfaStat = NFA_STATUS_FAILED;
    UINT8 actualNumEe = MAX_NUM_EE;
    tNFA_EE_INFO eeInfo[MAX_NUM_EE];

    memset (&eeInfo, 0, sizeof(eeInfo));
    if ((nfaStat = NFA_EeGetInfo (&actualNumEe, eeInfo)) != NFA_STATUS_OK)
    {
        ALOGE ("%s: fail get info; error=0x%X", fn, nfaStat);
        return;
    }
    if (actualNumEe != 0)
    {
        for (UINT8 xx = 0; xx < actualNumEe; xx++)
        {
            if ((eeInfo[xx].num_interface != 0)
                && (eeInfo[xx].ee_interface[0] != NCI_NFCEE_INTERFACE_HCI_ACCESS)
                && (eeInfo[xx].ee_status == NFA_EE_STATUS_ACTIVE))
            {
                ALOGD ("%s: Handle: 0x%04x Change Status Active to Inactive", fn, eeInfo[xx].ee_handle);
                SyncEventGuard guard (mEeSetModeEvent);
                if ((nfaStat = NFA_EeModeSet (eeInfo[xx].ee_handle, NFA_EE_MD_DEACTIVATE)) == NFA_STATUS_OK)
                {
                    mEeSetModeEvent.wait (); //wait for NFA_EE_MODE_SET_EVT
                }
                else
                {
                    ALOGE ("Failed to set EE inactive");
                }
            }
        }
    }
    else
    {
        ALOGD ("%s: No active EEs found", fn);
    }
}

void RoutingManager::notifyActivated ()
{
    JNIEnv* e = NULL;
    ScopedAttach attach(mNativeData->vm, &e);
    if (e == NULL)
    {
        ALOGE ("jni env is null");
        return;
    }

    e->CallVoidMethod (mNativeData->manager, android::gCachedNfcManagerNotifyHostEmuActivated);
    if (e->ExceptionCheck())
    {
        e->ExceptionClear();
        ALOGE ("fail notify");
    }
}

void RoutingManager::notifyDeactivated ()
{
    mRxDataBuffer.clear();
    JNIEnv* e = NULL;
    ScopedAttach attach(mNativeData->vm, &e);
    if (e == NULL)
    {
        ALOGE ("jni env is null");
        return;
    }

    e->CallVoidMethod (mNativeData->manager, android::gCachedNfcManagerNotifyHostEmuDeactivated);
    if (e->ExceptionCheck())
    {
        e->ExceptionClear();
        ALOGE ("fail notify");
    }
}


void RoutingManager::notifyLmrtFull ()
{
    JNIEnv* e = NULL;
    ScopedAttach attach(mNativeData->vm, &e);
    if (e == NULL)
    {
        ALOGE ("jni env is null");
        return;
    }

    e->CallVoidMethod (mNativeData->manager, android::gCachedNfcManagerNotifyAidRoutingTableFull);
    if (e->ExceptionCheck())
    {
        e->ExceptionClear();
        ALOGE ("fail notify");
    }
}

void RoutingManager::handleData (const UINT8* data, UINT32 dataLen, tNFA_STATUS status)
{
    if (dataLen <= 0)
    {
        ALOGE("no data");
        goto TheEnd;
    }

    if (status == NFA_STATUS_CONTINUE)
    {
        mRxDataBuffer.insert (mRxDataBuffer.end(), &data[0], &data[dataLen]); //append data; more to come
        return; //expect another NFA_CE_DATA_EVT to come
    }
    else if (status == NFA_STATUS_OK)
    {
        mRxDataBuffer.insert (mRxDataBuffer.end(), &data[0], &data[dataLen]); //append data
        //entire data packet has been received; no more NFA_CE_DATA_EVT
    }
    else if (status == NFA_STATUS_FAILED)
    {
        ALOGE("RoutingManager::handleData: read data fail");
        goto TheEnd;
    }

    {
        JNIEnv* e = NULL;
        ScopedAttach attach(mNativeData->vm, &e);
        if (e == NULL)
        {
            ALOGE ("jni env is null");
            goto TheEnd;
        }

        ScopedLocalRef<jobject> dataJavaArray(e, e->NewByteArray(mRxDataBuffer.size()));
        if (dataJavaArray.get() == NULL)
        {
            ALOGE ("fail allocate array");
            goto TheEnd;
        }

        e->SetByteArrayRegion ((jbyteArray)dataJavaArray.get(), 0, mRxDataBuffer.size(),
                (jbyte *)(&mRxDataBuffer[0]));
        if (e->ExceptionCheck())
        {
            e->ExceptionClear();
            ALOGE ("fail fill array");
            goto TheEnd;
        }

        e->CallVoidMethod (mNativeData->manager, android::gCachedNfcManagerNotifyHostEmuData, dataJavaArray.get());
        if (e->ExceptionCheck())
        {
            e->ExceptionClear();
            ALOGE ("fail notify");
        }
    }
TheEnd:
    mRxDataBuffer.clear();
}

void RoutingManager::stackCallback (UINT8 event, tNFA_CONN_EVT_DATA* eventData)
{
    static const char fn [] = "RoutingManager::stackCallback";
    ALOGD("%s: event=0x%X", fn, event);
    RoutingManager& routingManager = RoutingManager::getInstance();

    switch (event)
    {
    case NFA_CE_REGISTERED_EVT:
        {
            tNFA_CE_REGISTERED& ce_registered = eventData->ce_registered;
            ALOGD("%s: NFA_CE_REGISTERED_EVT; status=0x%X; h=0x%X", fn, ce_registered.status, ce_registered.handle);
/* START [J14120901] - set ce listen tech */
            SyncEventGuard guard(routingManager.mAidEvent);
            routingManager.mAidOnDH = ce_registered.handle;
            routingManager.mAidEvent.notifyOne();
/* END [J14120901] */
        }
        break;

    case NFA_CE_DEREGISTERED_EVT:
        {
            tNFA_CE_DEREGISTERED& ce_deregistered = eventData->ce_deregistered;
            ALOGD("%s: NFA_CE_DEREGISTERED_EVT; h=0x%X", fn, ce_deregistered.handle);
/* START [J14120901] - set ce listen tech */
            SyncEventGuard guard(routingManager.mAidEvent);
            routingManager.mAidOnDH = NFA_HANDLE_INVALID;
            routingManager.mAidEvent.notifyOne();
/* END [J14120901] */
        }
        break;

    case NFA_CE_ACTIVATED_EVT:
        {
            routingManager.notifyActivated();
        }
        break;

    case NFA_DEACTIVATED_EVT:
    case NFA_CE_DEACTIVATED_EVT:
        {
            ALOGD("%s: NFA_DEACTIVATED_EVT, NFA_CE_DEACTIVATED_EVT", fn);
            routingManager.notifyDeactivated();
/* START [J00000000] - SLSI callback hooker */
            android::slsiEventHandler (event, eventData);
/* END [J00000000] - SLSI callback hooker */
            SyncEventGuard g (gDeactivatedEvent);
            gActivated = false; //guard this variable from multi-threaded access
            gDeactivatedEvent.notifyOne ();
        }
        break;

    case NFA_CE_DATA_EVT:
        {
            tNFA_CE_DATA& ce_data = eventData->ce_data;
            ALOGD("%s: NFA_CE_DATA_EVT; stat=0x%X; h=0x%X; data len=%u", fn, ce_data.status, ce_data.handle, ce_data.len);
            getInstance().handleData(ce_data.p_data, ce_data.len, ce_data.status);
        }
        break;
/* START [P1605170002] - Change CE listen tech configuration method for support 0x00 tech*/
    default :
            ALOGD("%s: Unknown event (0x5x)", fn, event);
        break;
/* END [P1605170002] - Change CE listen tech configuration method for support 0x00 tech*/
    }
}
/*******************************************************************************
**
** Function:        nfaEeCallback
**
** Description:     Receive execution environment-related events from stack.
**                  event: Event code.
**                  eventData: Event data.
**
** Returns:         None
**
*******************************************************************************/
void RoutingManager::nfaEeCallback (tNFA_EE_EVT event, tNFA_EE_CBACK_DATA* eventData)
{
    static const char fn [] = "RoutingManager::nfaEeCallback";

/* START [J14111100] - support secure element */
    SecureElement& se = SecureElement::getInstance();
/* END [J14111100] */

    RoutingManager& routingManager = RoutingManager::getInstance();

    switch (event)
    {
    case NFA_EE_REGISTER_EVT:
        {
            SyncEventGuard guard (routingManager.mEeRegisterEvent);
            ALOGD ("%s: NFA_EE_REGISTER_EVT; status=%u", fn, eventData->ee_register);
            routingManager.mEeRegisterEvent.notifyOne();
        }
        break;

    case NFA_EE_MODE_SET_EVT:
        {
            SyncEventGuard guard (routingManager.mEeSetModeEvent);
            ALOGD ("%s: NFA_EE_MODE_SET_EVT; status: 0x%04X  handle: 0x%04X  mActiveEeHandle: 0x%04X", fn,
                    eventData->mode_set.status, eventData->mode_set.ee_handle, se.mActiveEeHandle);
            routingManager.mEeSetModeEvent.notifyOne();

/* START [J14111100] - support secure element */
            se.notifyModeSet(eventData->mode_set.ee_handle, eventData->mode_set.status);
/* END [J14111100] */

/* START [J14127007] - Setup NFCEE configuration after enabling NFCEE */
            if ((eventData->mode_set.status == NFA_EE_STATUS_ACTIVE) &&
                (eventData->mode_set.ee_handle == routingManager.mDefaultEe | NFA_HANDLE_GROUP_EE))
            {
                tNFA_STATUS nfaStat;
                nfaStat = NFA_CeConfigureUiccListenTech ((routingManager.mDefaultEe | NFA_HANDLE_GROUP_EE), mUiccListenMask);

                if (nfaStat != NFA_STATUS_OK)
                    ALOGE ("Failed to configure UICC listen technologies");
            }
            else
                ALOGD ("%s : Does not update listen tech for SE.", fn);
/* END [J14127007] */
        }
        break;

    case NFA_EE_SET_TECH_CFG_EVT:
        {
            ALOGD ("%s: NFA_EE_SET_TECH_CFG_EVT; status=0x%X", fn, eventData->status);
            SyncEventGuard guard(routingManager.mRoutingEvent);
            routingManager.mRoutingEvent.notifyOne();
        }
        break;

    case NFA_EE_SET_PROTO_CFG_EVT:
        {
            ALOGD ("%s: NFA_EE_SET_PROTO_CFG_EVT; status=0x%X", fn, eventData->status);
            SyncEventGuard guard(routingManager.mRoutingEvent);
            routingManager.mRoutingEvent.notifyOne();
        }
        break;

    case NFA_EE_ACTION_EVT:
        {
            tNFA_EE_ACTION& action = eventData->action;
            if (action.trigger == NFC_EE_TRIG_SELECT)
                ALOGD ("%s: NFA_EE_ACTION_EVT; h=0x%X; trigger=select (0x%X)", fn, action.ee_handle, action.trigger);
            else if (action.trigger == NFC_EE_TRIG_APP_INIT)
            {
                tNFC_APP_INIT& app_init = action.param.app_init;
                ALOGD ("%s: NFA_EE_ACTION_EVT; h=0x%X; trigger=app-init (0x%X); aid len=%u; data len=%u", fn,
                        action.ee_handle, action.trigger, app_init.len_aid, app_init.len_data);
            }
            else if (action.trigger == NFC_EE_TRIG_RF_PROTOCOL)
                ALOGD ("%s: NFA_EE_ACTION_EVT; h=0x%X; trigger=rf protocol (0x%X)", fn, action.ee_handle, action.trigger);
            else if (action.trigger == NFC_EE_TRIG_RF_TECHNOLOGY)
                ALOGD ("%s: NFA_EE_ACTION_EVT; h=0x%X; trigger=rf tech (0x%X)", fn, action.ee_handle, action.trigger);
            else
                ALOGE ("%s: NFA_EE_ACTION_EVT; h=0x%X; unknown trigger (0x%X)", fn, action.ee_handle, action.trigger);
        }
        break;

    case NFA_EE_DISCOVER_REQ_EVT:
        {
            ALOGD ("%s: NFA_EE_DISCOVER_REQ_EVT; status=0x%X; num ee=%u, mReceivedEeInfo=0x%X", fn,
                    eventData->discover_req.status, eventData->discover_req.num_ee, routingManager.mReceivedEeInfo);
            {
                SyncEventGuard guard (routingManager.mEeInfoEvent);
                memcpy (&routingManager.mEeInfo, &eventData->discover_req, sizeof(routingManager.mEeInfo));
                /* START [J15100101] - Get Tech information */
                routingManager.setSeSupportedTech();
                /* END [J15100101] - Get Tech information */

/* START [160616002P] - Update LMRT if it update by req notification.*/
                routingManager.setRoutingTableByReqNtf();
/* END [160616002P] - Update LMRT if it update by req notification.*/

                routingManager.mReceivedEeInfo = true;
                routingManager.mEeInfoEvent.notifyOne();
            }
        }
        break;

    case NFA_EE_NO_CB_ERR_EVT:
        ALOGD ("%s: NFA_EE_NO_CB_ERR_EVT  status=%u", fn, eventData->status);
        break;

    case NFA_EE_ADD_AID_EVT:
        {
            ALOGD ("%s: NFA_EE_ADD_AID_EVT  status=%u", fn, eventData->status);
            if(eventData->status == NFA_STATUS_BUFFER_FULL)
            {
                ALOGD ("%s: AID routing table is FULL!!!", fn);
                RoutingManager::getInstance().notifyLmrtFull();
            }
/* START [J14127006] - AID RegisterStatus */
            SyncEventGuard guard(routingManager.mAidEvent);
            sAidRegisterStatus.status = eventData->status;
            routingManager.mAidEvent.notifyOne();
/* END [J14127006] */
        }
        break;

    case NFA_EE_REMOVE_AID_EVT:
        {
            ALOGD ("%s: NFA_EE_REMOVE_AID_EVT  status=%u", fn, eventData->status);
/* START [J14127006] - AID RegisterStatus */
            SyncEventGuard guard(routingManager.mAidEvent);
            sAidRegisterStatus.status = eventData->status;
            routingManager.mAidEvent.notifyOne();
/* END [J14127006] */
        }
        break;

    case NFA_EE_NEW_EE_EVT:
        {
            ALOGD ("%s: NFA_EE_NEW_EE_EVT  h=0x%X; status=%u", fn,
                eventData->new_ee.ee_handle, eventData->new_ee.ee_status);
        }
        break;

    case NFA_EE_UPDATED_EVT:
        {
            ALOGD("%s: NFA_EE_UPDATED_EVT", fn);
            SyncEventGuard guard(routingManager.mEeUpdateEvent);
            routingManager.mEeUpdateEvent.notifyOne();
        }
        break;

    default:
        ALOGE ("%s: unknown event=%u ????", fn, event);
        break;
    }
}

int RoutingManager::registerJniFunctions (JNIEnv* e)
{
    static const char fn [] = "RoutingManager::registerJniFunctions";
    ALOGD ("%s", fn);
    return jniRegisterNativeMethods (e, "com/android/nfc/cardemulation/AidRoutingManager", sMethods, NELEM(sMethods));
}

int RoutingManager::com_android_nfc_cardemulation_doGetDefaultRouteDestination (JNIEnv*)
{
/* START [J14120201] - Generic ESE ID */
    ALOGD ("doGetDefaultRouteDestination : mDefaultEe(0x%x)",getInstance().mDefaultEe);
    return android::slsiGetGenSeId(getInstance().mDefaultEe);
/* END [J14120201] */
}

int RoutingManager::com_android_nfc_cardemulation_doGetDefaultOffHostRouteDestination (JNIEnv*)
{
/* START [J14120201] - Generic ESE ID */
    ALOGD ("doGetDefaultOffHostRouteDestination : mOffHostEe(0x%x)", getInstance().mOffHostEe);
    return android::slsiGetGenSeId(getInstance().mOffHostEe);
/* END [J14120201] */
}

int RoutingManager::com_android_nfc_cardemulation_doGetAidMatchingMode (JNIEnv*)
{
    return getInstance().mAidMatchingMode;
}

/* START [J15072201] - pending enable discovery during listen mode */
void RoutingManager::setCeDiscoveryMask (unsigned long tech_mask)
{
    mUiccListenMask = tech_mask;
    ALOGD("%s: Enter - defaultEe = 0x%02X, mUiccListenMask = 0x%02X", __FUNCTION__, mDefaultEe, mUiccListenMask);

    useAidOnDH(false);

/* START [P1605170002] - Change CE listen tech configuration method for support 0x00 tech*/
    NFA_CeSetIsoDepListenTech(mUiccListenMask);
    NFA_CeConfigureUiccListenTech ((mDefaultEe | NFA_HANDLE_GROUP_EE), mUiccListenMask);
/* END [P1605170002] - Change CE listen tech configuration method for support 0x00 tech*/
    if((mUiccListenMask & NFA_TECHNOLOGY_MASK_A) || (mUiccListenMask & NFA_TECHNOLOGY_MASK_B))
        useAidOnDH(true);
}
/* END [J15072201] - pending enable discovery during listen mode */

/* START [P160421001] - Patch for Dynamic SE Selection */
void RoutingManager::setDefaultSe (int defaultSeId)
{
    mDefaultEe = defaultSeId;

/* START [160616002P] - Update LMRT if it update by req notification.*/
    // Set mSeTechMask to mUiccListenMask because defaultSE is HCE.
    if(mDefaultEe == 0x00)
        mSeTechMask = mUiccListenMask;
/* END [160616002P] - Update LMRT if it update by req notification.*/

    ALOGD("%s: Exit - mDefaultEe(0x%x), mOffHostEe(0x%x)", __FUNCTION__, mDefaultEe, mOffHostEe);
}

int RoutingManager::getDefaultSe ()
{
    ALOGD("%s: Exit - mDefaultEe(0x%x), mOffHostEe(0x%x)", __FUNCTION__, mDefaultEe, mOffHostEe);
    return mDefaultEe;
}
/* END [P160421001] - Patch for Dynamic SE Selection */

/* START [J14120901] - set ce listen tech */
void RoutingManager::setCeListenTech (unsigned long tech_mask)
{
    bool isDefaultEe = false;
    int seId;

    mUiccListenMask = tech_mask;
    ALOGD("%s: Enter - defaultEe = 0x%02X, mUiccListenMask = 0x%02X", __FUNCTION__, mDefaultEe, mUiccListenMask);
    ALOGD("%s: p2p listen tech = 0x%02X", __FUNCTION__, PeerToPeer::getInstance().getP2pListenMask());

    useAidOnDH(false);

    /* Remove Previous Setting */
    for (UINT8 i = 0; i < mEeInfo.num_ee; i++)
    {
        seId = mEeInfo.ee_disc_info[i].ee_handle & ~NFA_HANDLE_GROUP_EE;
        NFA_CeConfigureUiccListenTech (mEeInfo.ee_disc_info[i].ee_handle, 0);
        if (mDefaultEe == seId)
            isDefaultEe = true;
    }

/* START [P1605170002] - Change CE listen tech configuration method for support 0x00 tech*/
/* START [P161014001] - Enable/Disable RF Discovery Tech Bit Mask */
/*
    if(mUiccListenMask == 0x00)
    {
        NFA_DisableListening();
    }
    else
*/
/* END [P161014001] - Enable/Disable RF Discovery Tech Bit Mask */
    {
        NFA_EnableListening();
        NFA_CeSetIsoDepListenTech(mUiccListenMask);
		NFA_CeConfigureUiccListenTech ((mDefaultEe | NFA_HANDLE_GROUP_EE), mUiccListenMask);
    if((mUiccListenMask & NFA_TECHNOLOGY_MASK_A) || (mUiccListenMask & NFA_TECHNOLOGY_MASK_B))
        useAidOnDH(true);
}
/* END [P1605170002] - Change CE listen tech configuration method for support 0x00 tech*/
}

bool RoutingManager::useAidOnDH(bool useAidOnDH)
{
    tNFA_STATUS nfaStat;
    SyncEventGuard guard(mAidEvent);

    ALOGD("%s: Enter - useAidOnDH = %s", __FUNCTION__, (useAidOnDH == true ? "TRUE" : "FALSE"));

    if(true == useAidOnDH)
    {
        nfaStat = NFA_CeRegisterAidOnDH(NULL, 0, stackCallback);
        if (nfaStat != NFA_STATUS_OK)
            ALOGE("Failed to register wildcard AID for DH");
        else
        {
            mAidEvent.wait();
            return true;
        }
    }
    else if(false == useAidOnDH && NFA_HANDLE_INVALID != mAidOnDH)
    {
        nfaStat = NFA_CeDeregisterAidOnDH(mAidOnDH);
        if (nfaStat != NFA_STATUS_OK)
            ALOGE("Failed to deregister wildcard AID for DH");
        else
        {
            mAidEvent.wait();
            return true;
        }
    }

    return false;
}

unsigned long RoutingManager::getCeListenTech(void)
{
    return mUiccListenMask;
}
/* END [J14120901] */


/* START [J14111200] - Routing setting */
void RoutingManager::setDefaultTechRouting (int seId, int value, int power)
{
    UINT16 tech = value;
    tNFA_TECHNOLOGY_MASK *powerState = gTechRouting[seId];
    UINT8 mask = 0x01;
    int bit;
    for (bit = 0; bit < 6; bit++)
    {
        powerState[bit] = (mask & (UINT8)power) ? tech : 0;
        mask = mask << 1;
    }

    tNFA_STATUS nfaStat;
    SyncEventGuard guard (mRoutingEvent);

    nfaStat = NFA_EeSetDefaultTechRouting (seId, powerState[0], powerState[1], powerState[2], powerState + 3);
    if(nfaStat == NFA_STATUS_OK){
        mRoutingEvent.wait ();
        ALOGD ("tech routing SUCCESS (seId: %d)", seId);
    }
    else{
        ALOGE ("Fail to set default tech routing (SeId: %d) State: %d", seId, nfaStat);
    }
}

void RoutingManager::setDefaultProtoRouting (int seId, int value, int power)
{
    UINT16 proto = value;
    tNFA_TECHNOLOGY_MASK *powerState = gProtoRouting[seId];
    UINT8 mask = 0x01;
    int bit;
    for (bit = 0; bit < 6; bit++)
    {
        powerState[bit] = (mask & power) ? proto : 0;
        mask = mask << 1;
    }

    tNFA_STATUS nfaStat;
    SyncEventGuard guard (mRoutingEvent);

    nfaStat = NFA_EeSetDefaultProtoRouting (seId, powerState[0], powerState[1], powerState[2], powerState + 3);
    if(nfaStat == NFA_STATUS_OK){
        mRoutingEvent.wait ();
        ALOGD ("%s : proto routing SUCCESS (seId: %d , proto: 0x%x)",  __FUNCTION__, seId, proto);
    }
    else{
        ALOGE ("Fail to set default proto routing (SeId: %d) State: %d", seId, nfaStat);
    }
}

bool RoutingManager::clearRouting(int type)
{
    int clear[6] = {0, 0, 0, 0, 0, 0};
    bool ret = true;
    ALOGD ("%s : Enter", __FUNCTION__);

    if (type & 0x01) {
        ALOGD ("clear all of tech based routing");
        memset(gTechRouting, 0, sizeof(gTechRouting));
        setDefaultTechRouting(SEID_HCE, 0, 0);
        setDefaultTechRouting(SEID_UICC, 0, 0);
        setDefaultTechRouting(SEID_ESE, 0, 0);
    }

    if (type & 0x02) {
        ALOGD ("clear all of proto based routing");
        memset(gProtoRouting, 0, sizeof(gProtoRouting));
        setDefaultProtoRouting(SEID_HCE, 0, 0);
        setDefaultProtoRouting(SEID_UICC, 0, 0);
        setDefaultProtoRouting(SEID_ESE, 0, 0);
    }

    if (type & 0x04) {
        ALOGD ("clear all of aid based routing");
        UINT8 clearAID[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        UINT8 aidLen = 8;
        ret = (ret && RoutingManager::getInstance().removeAidRouting(clearAID, aidLen));
    }

    ALOGD ("%s: exit", __FUNCTION__);
    return ret;
}
/* END [J14111200] */

/* START [J15100101] - Get Tech information */
/*  0x80 : UICC Mifare
    0x40 : UICC TypeF
    0x20 : UICC TypeB
    0x10 : UICC TypeA
    0x08 : ESE Mifare
    0x04 : RFU
    0x02 : ESE TypeB
    0x01 : ESE TypeA
*/
void RoutingManager::setSeSupportedTech ()
{
    for (UINT8 i = 0; i < mEeInfo.num_ee; i++)
    {
        int supported = 0x00;
        if (mEeInfo.ee_disc_info[i].la_protocol != 0)
            supported |= 0x01;
        if (mEeInfo.ee_disc_info[i].lb_protocol != 0)
            supported |= 0x02;
        if (mEeInfo.ee_disc_info[i].lf_protocol != 0)
            supported |= 0x04;
        if (mEeInfo.ee_disc_info[i].other_tech == 0xF0)
            supported |= 0x08;

        ALOGD ("%s: ee_handle :0x%x , 0x%x", __FUNCTION__, mEeInfo.ee_disc_info[i].ee_handle, supported);

        if ((mEeInfo.ee_disc_info[i].ee_handle & ~NFA_HANDLE_GROUP_EE) == SEID_ESE)
            mSeSupportedTech |= supported;
        else if ((mEeInfo.ee_disc_info[i].ee_handle & ~NFA_HANDLE_GROUP_EE) == SEID_UICC)
            mSeSupportedTech |= (supported << 4);
    }
}

int RoutingManager::getSeSupportedTech ()
{
    ALOGD ("%s: SE supported tech 0x%x", __FUNCTION__, mSeSupportedTech);
    return mSeSupportedTech;
}
/* END [J15100101] */

