/*
 * Copyright (C) 2012 The Android Open Source Project
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
 *  Communicate with secure elements that are attached to the NFC
 *  controller.
 */
#include <semaphore.h>
#include <errno.h>
#include <ScopedLocalRef.h>
#include "OverrideLog.h"
#include "SecureElement.h"
#include "config.h"
#include "PowerSwitch.h"
#include "JavaClassConstants.h"


/*****************************************************************************
**
** public variables
**
*****************************************************************************/
/* START [160503001J] Configure HCI setting for wired C/E by eSE chip type (Oberthure or Gemalto)*/
int gEseChipType = 0x00;        // 0x00 : Oberthur, 0x01 : Gemalto
int gSEId = 0x02;     // secure element ID to use in connectEE(), -1 means not set
int gGatePipe = -1; // gate id or static pipe id to use in connectEE(), -1 means not set
bool gUseStaticPipe = true;    // if true, use gGatePipe as static pipe id.  if false, use as gate id
bool gIsSupportConcurrentWCE = false;    // if true, support concurrent wired C/E.
bool gIsSetWcebyPropCmd = false;
/* END [160503001J] Configure HCI setting for wired C/E by eSE chip type (Oberthure or Gemalto)*/


namespace android
{
    extern void startRfDiscovery (bool isStart);
    extern void setUiccIdleTimeout (bool enable);

    /* START [J14120201] - Generic ESE ID */
    extern jint slsiGetGenSeId(int seId);
    /* END [J14120201] */

}

//////////////////////////////////////////////
//////////////////////////////////////////////


SecureElement SecureElement::sSecElem;
const char* SecureElement::APP_NAME = "nfc_jni";

/* START [J14127002] - active se override to eSe */
//const UINT16 ACTIVE_SE_USE_ANY = 0xFFFF;
const UINT16 ACTIVE_SE_USE_ANY = 0x02;
/* END [J14127002] */

// [START] System LSI - event source
struct transactionData {
    int     len;
    UINT8   pipe;
    UINT8*  data;
    struct transactionData *next;
} *trsDataFirst, *trsDataLast;
Mutex           trsMutext; // protects fields below

void SecureElement::notifyTransaction(void)
{
    struct transactionData *trsDataRemove = NULL;

    ALOGD ("%s: enter;", __FUNCTION__);
    trsMutext.lock();
    if (trsDataFirst == NULL)
    {
        trsMutext.unlock();
        ALOGD ("%s: exit, its empty;", __FUNCTION__);
        return;
    }

    do {
        trsDataRemove = trsDataFirst;
        trsDataFirst = trsDataFirst->next;

        int evtSrc = SecureElement::sSecElem.findDestByPipe(trsDataRemove->pipe);
        int aidLen = trsDataRemove->data[1];
        UINT8 *aid = &trsDataRemove->data[2];
        UINT8 dataLen = 0;
        UINT8 *data = NULL;

        ALOGD ("%s: evtSrc is %d", __FUNCTION__, evtSrc);
        if ((trsDataRemove->len > 2+aidLen) && (trsDataRemove->data[2+aidLen] == 0x82))
        {
            dataLen = trsDataRemove->data[2 + aidLen + 1];
            data = &trsDataRemove->data[2 + aidLen + 2];
        }
        SecureElement::sSecElem.notifyTransactionListenersOfAid (aid, aidLen, data, dataLen, evtSrc);

        free(trsDataRemove->data);
        free(trsDataRemove);

    } while (trsDataFirst != NULL);

    trsDataLast = NULL;
    trsMutext.unlock();
    ALOGD ("%s: exit;", __FUNCTION__);
}

bool SecureElement::requestNotifyTransaction(UINT8 pipe, UINT8 *data, int len)
{
    struct transactionData *trsDataNew = NULL;

    ALOGD ("%s: enter;", __FUNCTION__);
    trsMutext.lock();

    trsDataNew = (struct transactionData *)malloc(sizeof(struct transactionData));
    if (trsDataNew == NULL)
    {
        ALOGD ("%s: struct allocate failed.", __FUNCTION__);
        trsMutext.unlock();
        return false;
    }
    trsDataNew->data = (UINT8*)malloc(len);
    if (trsDataNew == NULL)
    {
        ALOGD ("%s: data allocate failed.", __FUNCTION__);
        free(trsDataNew);
        trsMutext.unlock();
        return false;
    }
    memcpy(trsDataNew->data, data, len);
    trsDataNew->len = len;
    trsDataNew->pipe = pipe;
    trsDataNew->next = NULL;

    if (trsDataLast != NULL)
        trsDataLast->next = trsDataNew;
    trsDataLast = trsDataNew;

    if (trsDataFirst == NULL)
    {
        ALOGD ("%s: Get pipe list to find the source of the event", __FUNCTION__);
        tNFA_STATUS nfaStat = NFA_HciGetGateAndPipeList (SecureElement::sSecElem.mNfaHciHandle);
        if (nfaStat != NFA_STATUS_OK)
        {
            ALOGD ("%s: NFA_HciGetGateAndPipeList failed.", __FUNCTION__);
            free(trsDataNew->data);
            free(trsDataNew);
            trsMutext.unlock();
            return false;
        }
        trsDataFirst = trsDataNew;
    }

    trsMutext.unlock();
    ALOGD ("%s: exit;", __FUNCTION__);
    return true;
}
// [END] System LSI - event source

/*******************************************************************************
**
** Function:        SecureElement
**
** Description:     Initialize member variables.
**
** Returns:         None
**
*******************************************************************************/
SecureElement::SecureElement ()
:   mActiveEeHandle (NFA_HANDLE_INVALID),
    mDestinationGate (4), //loopback gate
    mNfaHciHandle (NFA_HANDLE_INVALID),
    mNativeData (NULL),
    mIsInit (false),
    mActualNumEe (0),
    mNumEePresent(0),
    mbNewEE (true),   // by default we start w/thinking there are new EE
    mNewPipeId (0),
    mNewSourceGate (0),
    mActiveSeOverride(ACTIVE_SE_USE_ANY),
    mCommandStatus (NFA_STATUS_OK),
    mIsPiping (false),
    mCurrentRouteSelection (NoRoute),
    mActualResponseSize(0),
    mUseOberthurWarmReset (false),
    mActivatedInListenMode (false),
/* START [J14111101_Part3] - pending enable discovery during listen mode */
    mPeerFieldInListenMode (false),
/* END [J14111101_Part3] - pending enable discovery during listen mode */
    mOberthurWarmResetCommand (3),
    mRfFieldIsOn(false)
{
    memset (&mEeInfo, 0, sizeof(mEeInfo));
    memset (&mUiccInfo, 0, sizeof(mUiccInfo));
    memset (&mHciCfg, 0, sizeof(mHciCfg));
    memset (mResponseData, 0, sizeof(mResponseData));
    memset (mAidForEmptySelect, 0, sizeof(mAidForEmptySelect));
    memset (&mLastRfFieldToggle, 0, sizeof(mLastRfFieldToggle));
    memset (mVerInfo, 0, sizeof(mVerInfo));
// [START] System LSI - event source
    trsDataFirst = trsDataLast = NULL;
// [END] System LSI - event source
}


/*******************************************************************************
**
** Function:        ~SecureElement
**
** Description:     Release all resources.
**
** Returns:         None
**
*******************************************************************************/
SecureElement::~SecureElement ()
{
}


/*******************************************************************************
**
** Function:        getInstance
**
** Description:     Get the SecureElement singleton object.
**
** Returns:         SecureElement object.
**
*******************************************************************************/
SecureElement& SecureElement::getInstance()
{
    return sSecElem;
}


/*******************************************************************************
**
** Function:        setActiveSeOverride
**
** Description:     Specify which secure element to turn on.
**                  activeSeOverride: ID of secure element
**
** Returns:         None
**
*******************************************************************************/
void SecureElement::setActiveSeOverride(UINT8 activeSeOverride)
{
    ALOGD ("SecureElement::setActiveSeOverride, seid=0x%X", activeSeOverride);
    mActiveSeOverride = activeSeOverride;
}


/*******************************************************************************
**
** Function:        initialize
**
** Description:     Initialize all member variables.
**                  native: Native data.
**
** Returns:         True if ok.
**
*******************************************************************************/
bool SecureElement::initialize (nfc_jni_native_data* native)
{
    static const char fn [] = "SecureElement::initialize";
    tNFA_STATUS nfaStat;
    unsigned long num = 0;

    ALOGD ("%s: enter", fn);

    if (GetNumValue("NFA_HCI_DEFAULT_DEST_GATE", &num, sizeof(num)))
        mDestinationGate = num;
    ALOGD ("%s: Default destination gate: 0x%X", fn, mDestinationGate);

/* START [160503001J] Configure HCI setting for wired C/E by eSE chip type (Oberthure or Gemalto)*/
    gEseChipType = 0x00;                            // Oberthur (0x00) , Gemalto (0x01)
    gIsSetWcebyPropCmd = true;
    gUseStaticPipe = true;                          // Gemalto (false), Oberthur (true)
    gIsSupportConcurrentWCE = false;        // Gemalto (true), Oberthur (false)
    gGatePipe = 0x72;
    mOberthurWarmResetCommand = false;

    GetNumValue("NFA_HCI_USE_FORCE_SETTING", &num, sizeof(num));
    if(num != 0x01)
    {
        ALOGD ("%s: Checking eSE Chip Type.", fn);
        SyncEventGuard guard (mSecVsCmdEvent);
        nfaStat = NFA_SendVsCommand(0x06, 0x00, NULL, getESEchipTypeCallback);
        if (nfaStat == NFA_STATUS_OK)
            mSecVsCmdEvent.wait ();
    }
    else
    {
        ALOGD ("%s: Set HCE Network Type by libnfc-sec.conf", fn);
        gIsSetWcebyPropCmd = false;
    }

    if (GetNumValue("OBERTHUR_WARM_RESET_COMMAND", &num, sizeof(num)))
    {
        mUseOberthurWarmReset = true;
        mOberthurWarmResetCommand = (UINT8) num;
    }

    if(!gIsSetWcebyPropCmd)
    {
         if (GetNumValue("NFA_HCI_ESE_CHIP_TYPE", &num, sizeof(num)))
         {
             gEseChipType = num;
         }

         if(gEseChipType == 0x01)   // Gemalto eSE
    {
    if (GetNumValue("NFA_HCI_DEFAULT_STATIC_PIPE_TYPE", &num, sizeof(num)))
    {
        if(num == 0x00)
        {
            gUseStaticPipe = false;
                gGatePipe = 0xF0;       // 0xF0 : APDU gate for GP.
                 }
                 else   // static
                    gUseStaticPipe = true;
             }

                gIsSupportConcurrentWCE = true;
             if(!gUseStaticPipe)        // dynamic pipe
             {
                 mUseOberthurWarmReset = false;
             }
        }
        else    // Oberthur eSE
        {
                gIsSupportConcurrentWCE = false;
    }

    if(gIsSupportConcurrentWCE == true)
        ALOGD ("%s: Support concurrent wired C/E", fn);
    else
        ALOGD ("%s: Not support concurrent wired C/E", fn);
    }
/* END [160503001J] Configure HCI setting for wired C/E by eSE chip type (Oberthure or Gemalto)*/

/* START [J15012901] Enabling SE (eSE, UICC) during NFC On */
    // active SE, if not set active all SEs, use the first one.
    if (GetNumValue("ACTIVE_SE", &num, sizeof(num)))
    {
        mActiveSeOverride = num;
        gSEId = mActiveSeOverride;          // for connectEE()
        ALOGD ("%s: Active SE override: 0x%X", fn, mActiveSeOverride);
    }
    else
        mActiveSeOverride = 0x00;
/* END [J15012901] Enabling SE (eSE, UICC) during NFC On */

    mActiveEeHandle = NFA_HANDLE_INVALID;
    mNfaHciHandle = NFA_HANDLE_INVALID;

    mNativeData     = native;
    mActualNumEe    = MAX_NUM_EE;
    mbNewEE         = true;
    mNewPipeId      = 0;
    mNewSourceGate  = 0;
    mRfFieldIsOn    = false;
    mActivatedInListenMode = false;
/* START [J14111101_Part3] - pending enable discovery during listen mode */
    mPeerFieldInListenMode = false;
/* END [J14111101_Part3] - pending enable discovery during listen mode */
    mCurrentRouteSelection = NoRoute;
    memset (mEeInfo, 0, sizeof(mEeInfo));
    memset (&mUiccInfo, 0, sizeof(mUiccInfo));
    memset (&mHciCfg, 0, sizeof(mHciCfg));
    mUsedAids.clear ();
    memset(mAidForEmptySelect, 0, sizeof(mAidForEmptySelect));

    // if no SE is to be used, get out.
    if (mActiveSeOverride == 0)
    {
        ALOGD ("%s: No SE; No need to initialize SecureElement", fn);
        return (false);
    }

    // Get Fresh EE info.
    if (! getEeInfo())
        return (false);

    // If the controller has an HCI Network, register for that
    for (size_t xx = 0; xx < mActualNumEe; xx++)
    {
        if ((mEeInfo[xx].num_interface > 0) && (mEeInfo[xx].ee_interface[0] == NCI_NFCEE_INTERFACE_HCI_ACCESS) )
        {
            ALOGD ("%s: Found HCI network, try hci register", fn);

            SyncEventGuard guard (mHciRegisterEvent);

            nfaStat = NFA_HciRegister (const_cast<char*>(APP_NAME), nfaHciCallback, TRUE);
            if (nfaStat != NFA_STATUS_OK)
            {
                ALOGE ("%s: fail hci register; error=0x%X", fn, nfaStat);
                return (false);
            }
            mHciRegisterEvent.wait();
            break;
        }
    }

    GetStrValue(NAME_AID_FOR_EMPTY_SELECT, (char*)&mAidForEmptySelect[0], sizeof(mAidForEmptySelect));

    mIsInit = true;

    ALOGD ("%s: exit", fn);
    return (true);
}


/*******************************************************************************
**
** Function:        finalize
**
** Description:     Release all resources.
**
** Returns:         None
**
*******************************************************************************/
void SecureElement::finalize ()
{
    static const char fn [] = "SecureElement::finalize";
    ALOGD ("%s: enter", fn);

    if (mNfaHciHandle != NFA_HANDLE_INVALID)
        NFA_HciDeregister (const_cast<char*>(APP_NAME));

    mNfaHciHandle = NFA_HANDLE_INVALID;
    mNativeData   = NULL;
    mIsInit       = false;
    mActualNumEe  = 0;
    mNumEePresent = 0;
    mNewPipeId    = 0;
    mNewSourceGate = 0;
    mIsPiping = false;
    memset (mEeInfo, 0, sizeof(mEeInfo));
    memset (&mUiccInfo, 0, sizeof(mUiccInfo));

    ALOGD ("%s: exit", fn);
}


/*******************************************************************************
**
** Function:        getEeInfo
**
** Description:     Get latest information about execution environments from stack.
**
** Returns:         True if at least 1 EE is available.
**
*******************************************************************************/
bool SecureElement::getEeInfo()
{
    static const char fn [] = "SecureElement::getEeInfo";
    ALOGD ("%s: enter; mbNewEE=%d, mActualNumEe=%d", fn, mbNewEE, mActualNumEe);
    tNFA_STATUS nfaStat = NFA_STATUS_FAILED;

    // If mbNewEE is true then there is new EE info.
    if (mbNewEE)
    {
        mActualNumEe = MAX_NUM_EE;

        if ((nfaStat = NFA_EeGetInfo (&mActualNumEe, mEeInfo)) != NFA_STATUS_OK)
        {
            ALOGE ("%s: fail get info; error=0x%X", fn, nfaStat);
            mActualNumEe = 0;
        }
        else
        {
            mbNewEE = false;

            ALOGD ("%s: num EEs discovered: %u", fn, mActualNumEe);
            if (mActualNumEe != 0)
            {
                for (UINT8 xx = 0; xx < mActualNumEe; xx++)
                {
                    if ((mEeInfo[xx].num_interface != 0) && (mEeInfo[xx].ee_interface[0] != NCI_NFCEE_INTERFACE_HCI_ACCESS) )
                        mNumEePresent++;

                    ALOGD ("%s: EE[%u] Handle: 0x%04x  Status: %s  Num I/f: %u: (0x%02x, 0x%02x)  Num TLVs: %u",
                          fn, xx, mEeInfo[xx].ee_handle, eeStatusToString(mEeInfo[xx].ee_status), mEeInfo[xx].num_interface,
                          mEeInfo[xx].ee_interface[0], mEeInfo[xx].ee_interface[1], mEeInfo[xx].num_tlvs);

                    for (size_t yy = 0; yy < mEeInfo[xx].num_tlvs; yy++)
                    {
                        ALOGD ("%s: EE[%u] TLV[%u]  Tag: 0x%02x  Len: %u  Values[]: 0x%02x  0x%02x  0x%02x ...",
                              fn, xx, yy, mEeInfo[xx].ee_tlv[yy].tag, mEeInfo[xx].ee_tlv[yy].len, mEeInfo[xx].ee_tlv[yy].info[0],
                              mEeInfo[xx].ee_tlv[yy].info[1], mEeInfo[xx].ee_tlv[yy].info[2]);
                    }
                }
            }
        }
    }
    ALOGD ("%s: exit; mActualNumEe=%d, mNumEePresent=%d", fn, mActualNumEe,mNumEePresent);
    return (mActualNumEe != 0);
}


/*******************************************************************************
**
** Function         TimeDiff
**
** Description      Computes time difference in milliseconds.
**
** Returns          Time difference in milliseconds
**
*******************************************************************************/
static UINT32 TimeDiff(timespec start, timespec end)
{
    end.tv_sec -= start.tv_sec;
    end.tv_nsec -= start.tv_nsec;

    if (end.tv_nsec < 0) {
        end.tv_nsec += 10e8;
        end.tv_sec -=1;
    }

    return (end.tv_sec * 1000) + (end.tv_nsec / 10e5);
}

/*******************************************************************************
**
** Function:        isRfFieldOn
**
** Description:     Can be used to determine if the SE is in an RF field
**
** Returns:         True if the SE is activated in an RF field
**
*******************************************************************************/
bool SecureElement::isRfFieldOn() {
    AutoMutex mutex(mMutex);
    if (mRfFieldIsOn) {
        return true;
    }
    struct timespec now;
    int ret = clock_gettime(CLOCK_MONOTONIC, &now);
    if (ret == -1) {
        ALOGE("isRfFieldOn(): clock_gettime failed");
        return false;
    }
    if (TimeDiff(mLastRfFieldToggle, now) < 50) {
        // If it was less than 50ms ago that RF field
        // was turned off, still return ON.
        return true;
    } else {
        return false;
    }
}

/*******************************************************************************
**
** Function:        isActivatedInListenMode
**
** Description:     Can be used to determine if the SE is activated in listen mode
**
** Returns:         True if the SE is activated in listen mode
**
*******************************************************************************/
bool SecureElement::isActivatedInListenMode() {
    return mActivatedInListenMode;
}

/* START [J14111101_Part3] - pending enable discovery during listen mode */
/*******************************************************************************
**
** Function:        setIsActivatedInListenMode
**
** Description:     Can be used to determine if the SE is activated in listen mode
**
** Returns:         True if the SE is activated in listen mode
**
*******************************************************************************/
void SecureElement::setIsPeerInListenMode(bool isActivated) {
	mPeerFieldInListenMode = isActivated;
}

bool SecureElement::isPeerInListenMode() {
    return mPeerFieldInListenMode;
}
/* END [J14111101_Part3] - pending enable discovery during listen mode */


/*******************************************************************************
**
** Function:        getSecureElementIdList
**
** Description:     Get a list of ID's of all secure elements.
**                  e: Java Virtual Machine.
**
** Returns:         List of ID's.
**
*******************************************************************************/
jintArray SecureElement::getSecureElementIdList (JNIEnv* e)
{
    static const char fn [] = "SecureElement::getSecureElementIdList";
    ALOGD ("%s: enter", fn);

    if (!mIsInit)
    {
        ALOGE ("%s: not init", fn);
        return NULL;
    }

    if (! getEeInfo())
    {
        ALOGE ("%s: no sec elem", fn);
        return NULL;
    }

    jintArray list = e->NewIntArray (mNumEePresent); //allocate array
    jint seId = 0;
    int cnt = 0;
    for (int ii = 0; ii < mActualNumEe && cnt < mNumEePresent; ii++)
    {
        if ((mEeInfo[ii].num_interface == 0) || (mEeInfo[ii].ee_interface[0] == NCI_NFCEE_INTERFACE_HCI_ACCESS) )
            continue;
        seId = mEeInfo[ii].ee_handle & ~NFA_HANDLE_GROUP_EE;
        e->SetIntArrayRegion (list, cnt++, 1, &seId);
        ALOGD ("%s: index=%d; se id=0x%X", fn, ii, seId);
    }
    ALOGD("%s: exit", fn);
    return list;
}


/*******************************************************************************
**
** Function:        activate
**
** Description:     Turn on the secure element.
**                  seID: ID of secure element; 0xF3 or 0xF4.
**
** Returns:         True if ok.
**
*******************************************************************************/
bool SecureElement::activate (jint seID)
{
    static const char fn [] = "SecureElement::activate";
    tNFA_STATUS nfaStat = NFA_STATUS_FAILED;
    int numActivatedEe = 0;

    ALOGD ("%s: enter; seID=0x%X", fn, seID);

    if (!mIsInit)
    {
        ALOGE ("%s: not init", fn);
        return false;
    }

    if (mActiveEeHandle != NFA_HANDLE_INVALID)
    {
        ALOGD ("%s: already active", fn);
        return true;
    }

    // Get Fresh EE info if needed.
    if (! getEeInfo())
    {
        ALOGE ("%s: no EE info", fn);
        return false;
    }

    UINT16 overrideEeHandle = 0;
    // If the Active SE is overridden
    if (mActiveSeOverride && (mActiveSeOverride != ACTIVE_SE_USE_ANY))
        overrideEeHandle = NFA_HANDLE_GROUP_EE | mActiveSeOverride;

    ALOGD ("%s: override ee h=0x%X", fn, overrideEeHandle );

/* START [160503001J] Configure HCI setting for wired C/E by eSE chip type (Oberthure or Gemalto)*/
    if(gIsSupportConcurrentWCE != true)
    {
        mMutex.lock();
        if (mRfFieldIsOn) {
            ALOGE("%s: RF field indication still on, resetting", fn);
            mRfFieldIsOn = false;
        }
        mMutex.unlock();
    }
/* END [160503001J] Configure HCI setting for wired C/E by eSE chip type (Oberthure or Gemalto)*/

    //activate every discovered secure element
/* START [J15022301] Patch code to activate specific SeID */
    if((seID != EE_HANDLE_ESE) && (seID != EE_HANDLE_UICC))
    {
        for (int index=0; index < mActualNumEe; index++)
        {
            tNFA_EE_INFO& eeItem = mEeInfo[index];

            if ((eeItem.ee_handle == EE_HANDLE_ESE) || (eeItem.ee_handle == EE_HANDLE_UICC))
            {
            /* START [J15012901] Enabling SE (eSE, UICC) during NFC On */
#if 0
                if (overrideEeHandle && (overrideEeHandle != eeItem.ee_handle) )
                    continue;   // do not enable all SEs; only the override one
#endif
            /* END [J15012901] Enabling SE (eSE, UICC) during NFC On */

                if (eeItem.ee_status != NFC_NFCEE_STATUS_INACTIVE)
                {
                    ALOGD ("%s: h=0x%X already activated", fn, eeItem.ee_handle);
                    numActivatedEe++;
                    continue;
                }

                {
                    SyncEventGuard guard (mEeSetModeEvent);
                    ALOGD ("%s: set EE mode activate; h=0x%X", fn, eeItem.ee_handle);
                    if ((nfaStat = NFA_EeModeSet (eeItem.ee_handle, NFA_EE_MD_ACTIVATE)) == NFA_STATUS_OK)
                    {
                        mEeSetModeEvent.wait (); //wait for NFA_EE_MODE_SET_EVT
                        if (eeItem.ee_status == NFC_NFCEE_STATUS_ACTIVE)
                            numActivatedEe++;
                    }
                    else
                        ALOGE ("%s: NFA_EeModeSet failed; error=0x%X", fn, nfaStat);
                }
            }
        }
    }
    else
    {
        overrideEeHandle = NFA_HANDLE_GROUP_EE | seID;
        SyncEventGuard guard (mEeSetModeEvent);
        ALOGD ("%s: set EE mode activate; h=0x%X", fn, overrideEeHandle);
        if ((nfaStat = NFA_EeModeSet (overrideEeHandle, NFA_EE_MD_ACTIVATE)) == NFA_STATUS_OK)
        {
            mEeSetModeEvent.wait (); //wait for NFA_EE_MODE_SET_EVT
        }
        else
            ALOGE ("%s: NFA_EeModeSet failed; error=0x%X", fn, nfaStat);
    }
/* END [J15022301] Patch code to activate specific SeID */

    mActiveEeHandle = getDefaultEeHandle();
    if (mActiveEeHandle == NFA_HANDLE_INVALID)
        ALOGE ("%s: ee handle not found", fn);
    ALOGD ("%s: exit; active ee h=0x%X", fn, mActiveEeHandle);
    return mActiveEeHandle != NFA_HANDLE_INVALID;
}


/*******************************************************************************
**
** Function:        deactivate
**
** Description:     Turn off the secure element.
**                  seID: ID of secure element; 0xF3 or 0xF4.
**
** Returns:         True if ok.
**
*******************************************************************************/
bool SecureElement::deactivate (jint seID)
{
    static const char fn [] = "SecureElement::deactivate";
    bool retval = false;

    ALOGD ("%s: enter; seID=0x%X, mActiveEeHandle=0x%X", fn, seID, mActiveEeHandle);

    if (!mIsInit)
    {
        ALOGE ("%s: not init", fn);
        goto TheEnd;
    }

    //if the controller is routing to sec elems or piping,
    //then the secure element cannot be deactivated
    if ((mCurrentRouteSelection == SecElemRoute) || mIsPiping)
    {
        ALOGE ("%s: still busy", fn);
        goto TheEnd;
    }

    if (mActiveEeHandle == NFA_HANDLE_INVALID)
    {
        ALOGE ("%s: invalid EE handle", fn);
        goto TheEnd;
    }

    mActiveEeHandle = NFA_HANDLE_INVALID;
    retval = true;

TheEnd:
    ALOGD ("%s: exit; ok=%u", fn, retval);
    return retval;
}


/*******************************************************************************
**
** Function:        notifyTransactionListenersOfAid
**
** Description:     Notify the NFC service about a transaction event from secure element.
**                  aid: Buffer contains application ID.
**                  aidLen: Length of application ID.
**
** Returns:         None
**
*******************************************************************************/
void SecureElement::notifyTransactionListenersOfAid (const UINT8* aidBuffer, UINT8 aidBufferLen, const UINT8* dataBuffer, UINT8 dataBufferLen, UINT32 evtSrc)
{
    static const char fn [] = "SecureElement::notifyTransactionListenersOfAid";
    ALOGD ("%s: enter; aid len=%u", fn, aidBufferLen);
    ALOGD ("%s: event source %u", fn, evtSrc);
/* START [J14120201] - Generic ESE ID */
    evtSrc = android::slsiGetGenSeId(evtSrc);
    ALOGD ("%s: event source(gen) %u", fn, evtSrc);
/* END [J14120201] */

    if (aidBufferLen == 0) {
        return;
    }

    JNIEnv* e = NULL;
    ScopedAttach attach(mNativeData->vm, &e);
    if (e == NULL)
    {
        ALOGE ("%s: jni env is null", fn);
        return;
    }

    const UINT16 tlvMaxLen = aidBufferLen + 10;
    UINT8* tlv = new UINT8 [tlvMaxLen];
    if (tlv == NULL)
    {
        ALOGE ("%s: fail allocate tlv", fn);
        return;
    }

    memcpy (tlv, aidBuffer, aidBufferLen);
    UINT16 tlvActualLen = aidBufferLen;

    ScopedLocalRef<jobject> tlvJavaArray(e, e->NewByteArray(tlvActualLen));
    if (tlvJavaArray.get() == NULL)
    {
        ALOGE ("%s: fail allocate array", fn);
        goto TheEnd;
    }

    e->SetByteArrayRegion ((jbyteArray)tlvJavaArray.get(), 0, tlvActualLen, (jbyte *)tlv);
    if (e->ExceptionCheck())
    {
        e->ExceptionClear();
        ALOGE ("%s: fail fill array", fn);
        goto TheEnd;
    }

    if(dataBufferLen > 0)
    {
        const UINT16 dataTlvMaxLen = dataBufferLen + 10;
        UINT8* datatlv = new UINT8 [dataTlvMaxLen];
        if (datatlv == NULL)
        {
            ALOGE ("%s: fail allocate tlv", fn);
            return;
        }

        memcpy (datatlv, dataBuffer, dataBufferLen);
        UINT16 dataTlvActualLen = dataBufferLen;

        ScopedLocalRef<jobject> dataTlvJavaArray(e, e->NewByteArray(dataTlvActualLen));
        if (dataTlvJavaArray.get() == NULL)
        {
            ALOGE ("%s: fail allocate array", fn);
            goto Clean;
        }

        e->SetByteArrayRegion ((jbyteArray)dataTlvJavaArray.get(), 0, dataTlvActualLen, (jbyte *)datatlv);
        if (e->ExceptionCheck())
        {
            e->ExceptionClear();
            ALOGE ("%s: fail fill array", fn);
            goto Clean;
        }

        e->CallVoidMethod (mNativeData->manager, android::gCachedNfcManagerNotifyTransactionListeners, tlvJavaArray.get(), dataTlvJavaArray.get(), evtSrc);
        if (e->ExceptionCheck())
        {
            e->ExceptionClear();
            ALOGE ("%s: fail notify", fn);
                goto Clean;
        }

     Clean:
        delete [] datatlv;
    }
    else
    {
        e->CallVoidMethod (mNativeData->manager, android::gCachedNfcManagerNotifyTransactionListeners, tlvJavaArray.get(), NULL, evtSrc);
        if (e->ExceptionCheck())
        {
            e->ExceptionClear();
            ALOGE ("%s: fail notify", fn);
            goto TheEnd;
        }
    }
TheEnd:
    delete [] tlv;
    ALOGD ("%s: exit", fn);
}

/*******************************************************************************
**
** Function:        notifyConnectivityListeners
**
** Description:     Notify the NFC service about a connectivity event from secure element.
**
** Returns:         None
**
*******************************************************************************/
void SecureElement::notifyConnectivityListeners ()
{
    static const char fn [] = "SecureElement::notifyConnectivityListeners";
    ALOGD ("%s: enter; ", fn);

    JNIEnv* e = NULL;
    ScopedAttach attach(mNativeData->vm, &e);
    if (e == NULL)
    {
        ALOGE ("%s: jni env is null", fn);
        return;
    }

/*
    e->CallVoidMethod (mNativeData->manager, android::gCachedNfcManagerNotifyConnectivityListeners);
    if (e->ExceptionCheck())
    {
        e->ExceptionClear();
        ALOGE ("%s: fail notify", fn);
        goto TheEnd;
    }
*/

TheEnd:
    ALOGD ("%s: exit", fn);
}

/*******************************************************************************
**
** Function:        connectEE
**
** Description:     Connect to the execution environment.
**
** Returns:         True if ok.
**
*******************************************************************************/
bool SecureElement::connectEE ()
{
    static const char fn [] = "SecureElement::connectEE";
    tNFA_STATUS nfaStat = NFA_STATUS_FAILED;
    bool        retVal = false;
    UINT8       destHost = 0;
    unsigned long num = 0;
    char pipeConfName[40];
    tNFA_HANDLE  eeHandle = mActiveEeHandle;

    ALOGD ("%s: enter, mActiveEeHandle: 0x%04x, SEID: 0x%x, pipe_gate_num=%d, use pipe=%d",
        fn, mActiveEeHandle, gSEId, gGatePipe, gUseStaticPipe);

    if (!mIsInit)
    {
        ALOGE ("%s: not init", fn);
        return (false);
    }

    if (gSEId != -1)
    {
        eeHandle = gSEId | NFA_HANDLE_GROUP_EE;
        ALOGD ("%s: Using SEID: 0x%x", fn, eeHandle );
    }

    if (eeHandle == NFA_HANDLE_INVALID)
    {
        ALOGE ("%s: invalid handle 0x%X", fn, eeHandle);
        return (false);
    }

    tNFA_EE_INFO *pEE = findEeByHandle (eeHandle);

    if (pEE == NULL)
    {
        ALOGE ("%s: Handle 0x%04x  NOT FOUND !!", fn, eeHandle);
        return (false);
    }

/* START [160503001J] Configure HCI setting for wired C/E by eSE chip type (Oberthure or Gemalto)*/
    if(gIsSupportConcurrentWCE != true)
    {
        // Disable RF discovery completely while the DH is connected
        android::startRfDiscovery(false);
    }
/* END [160503001J] Configure HCI setting for wired C/E by eSE chip type (Oberthure or Gemalto)*/

    // Disable UICC idle timeout while the DH is connected
    //android::setUiccIdleTimeout (false);

    mNewSourceGate = 0;

/* START [J15022303] Configure default pipe and gate for wired C/E. */
    if (gUseStaticPipe)
    {
        if (gGatePipe == -1)
        {
            // pipe/gate num was not specifed by app, get from config file
            mNewPipeId = 0;

            // Construct the PIPE name based on the EE handle (e.g. NFA_HCI_STATIC_PIPE_ID_F3 for UICC0).
            snprintf (pipeConfName, sizeof(pipeConfName), "NFA_HCI_STATIC_PIPE_ID_%02X", eeHandle & NFA_HANDLE_MASK);

            if (GetNumValue(pipeConfName, &num, sizeof(num)) && (num != 0))
            {
                mNewPipeId = num;
                ALOGD ("%s: Using static pipe id: 0x%X", __FUNCTION__, mNewPipeId);
            }
            else
                ALOGD ("%s: Did not find value '%s' defined in the .conf", __FUNCTION__, pipeConfName);
        }
        else
            mNewPipeId = gGatePipe;
    }
    else
    {
        mNewPipeId = 0;
        mDestinationGate= gGatePipe;
    }
/* END [J15022303] Configure default pipe and gate for wired C/E. */

/* START [160503001J] Configure HCI setting for wired C/E by eSE chip type (Oberthure or Gemalto)*/
    if (gUseStaticPipe)
/* END [160503001J] Configure HCI setting for wired C/E by eSE chip type (Oberthure or Gemalto)*/
    {
        UINT8 host = 0x01;
        UINT8 gate = 0xF0;

        nfaStat = NFA_HciAddStaticPipe(mNfaHciHandle, host, gate, mNewPipeId);
        if (nfaStat != NFA_STATUS_OK)
        {
            ALOGE ("%s: fail create static pipe; error=0x%X", fn, nfaStat);
            retVal = false;
            goto TheEnd;
        }
    }
    else
    {
/* START [P1604040001] - Support Dual-SIM solution */
#if 1         // Convert Host_ID by SE_ID : UICC (Host_ID : 0x02, SE_ID : 0x03), eSE (Host_ID : 0x03, SE_ID : 0x02)
        if(gSEId == 0x03)
            destHost = 0x02;
        else
            destHost = 0x03;
#else
        if ( (pEE->num_tlvs >= 1) && (pEE->ee_tlv[0].tag == NFA_EE_TAG_HCI_HOST_ID) )
            destHost = pEE->ee_tlv[0].info[0];
        else
            destHost = 2;
#endif
/* END [P1604040001] - Support Dual-SIM solution */

        // Get a list of existing gates and pipes
        {
            ALOGD ("%s: get gate, pipe list", fn);
            SyncEventGuard guard (mPipeListEvent);
            nfaStat = NFA_HciGetGateAndPipeList (mNfaHciHandle);
            if (nfaStat == NFA_STATUS_OK)
            {
                mPipeListEvent.wait();
                if (mHciCfg.status == NFA_STATUS_OK)
                {
                    for (UINT8 xx = 0; xx < mHciCfg.num_pipes; xx++)
                    {
// START [20160829001] Debugging Code for HCI Network.
#if 0
                        ALOGD ("%s: check gate, pipe list : pipe_id(0x%x) =====", fn, mHciCfg.pipe[xx].pipe_id);
                        ALOGD ("%s: check gate, pipe list : local_gate(0x%x)", fn, mHciCfg.pipe[xx].local_gate);
                        ALOGD ("%s: check gate, pipe list : dest_host(0x%x)", fn, mHciCfg.pipe[xx].dest_host);
                        ALOGD ("%s: check gate, pipe list : dest_gate(0x%x)", fn, mHciCfg.pipe[xx].dest_gate);
#endif
// START [20160829001] Debugging Code for HCI Network.

                        if ( (mHciCfg.pipe[xx].dest_host == destHost)
                         &&  (mHciCfg.pipe[xx].dest_gate == mDestinationGate) )
                        {
                            mNewSourceGate = mHciCfg.pipe[xx].local_gate;
                            mNewPipeId     = mHciCfg.pipe[xx].pipe_id;

                            ALOGD ("%s: found configured gate: 0x%02x  pipe: 0x%02x", fn, mNewSourceGate, mNewPipeId);
                            break;
                        }
                        ALOGD ("%s: Not found configured gate: 0x%02x  pipe: 0x%02x", fn, mNewSourceGate, mNewPipeId);
                    }
                }
            }
        }

        if (mNewSourceGate == 0)
        {
            ALOGD ("%s: allocate gate", fn);
            //allocate a source gate and store in mNewSourceGate
            SyncEventGuard guard (mAllocateGateEvent);
// [START] Patch for L
            if ((nfaStat = NFA_HciAllocGate (mNfaHciHandle, mDestinationGate)) != NFA_STATUS_OK)
// [END] Patch for L
            {
                ALOGE ("%s: fail allocate source gate; error=0x%X", fn, nfaStat);
                goto TheEnd;
            }
            mAllocateGateEvent.wait ();
            if (mCommandStatus != NFA_STATUS_OK)
               goto TheEnd;
        }

        if (mNewPipeId == 0)
        {
            ALOGD ("%s: create pipe", fn);

            ALOGD ("%s: mNewSourceGate(0x%X), mDestinationGate(0x%X)", fn, mNewSourceGate, mDestinationGate);

            SyncEventGuard guard (mCreatePipeEvent);
            nfaStat = NFA_HciCreatePipe (mNfaHciHandle, mNewSourceGate, destHost, mDestinationGate);
            if (nfaStat != NFA_STATUS_OK)
            {
                ALOGE ("%s: fail create pipe; error=0x%X", fn, nfaStat);
                goto TheEnd;
            }
            mCreatePipeEvent.wait ();
            if (mCommandStatus != NFA_STATUS_OK)
               goto TheEnd;
        }

        {
            ALOGD ("%s: open pipe", fn);
            SyncEventGuard guard (mPipeOpenedEvent);
            nfaStat = NFA_HciOpenPipe (mNfaHciHandle, mNewPipeId);
            if (nfaStat != NFA_STATUS_OK)
            {
                ALOGE ("%s: fail open pipe; error=0x%X", fn, nfaStat);
                goto TheEnd;
            }
            mPipeOpenedEvent.wait ();
            if (mCommandStatus != NFA_STATUS_OK)
            {
                ALOGE ("%s: fail open pipe; error=0x%X", fn, mCommandStatus);
                goto TheEnd;
            }
        }
    }

    retVal = true;

TheEnd:
    mIsPiping = retVal;
    if (!retVal)
    {
        // if open failed we need to de-allocate the gate
        disconnectEE(0);
    }

    ALOGD ("%s: exit; ok=%u", fn, retVal);
    return retVal;
}


/*******************************************************************************
**
** Function:        disconnectEE
**
** Description:     Disconnect from the execution environment.
**                  seID: ID of secure element.
**
** Returns:         True if ok.
**
*******************************************************************************/
bool SecureElement::disconnectEE (jint seID)
{
    static const char fn [] = "SecureElement::disconnectEE";
    tNFA_STATUS nfaStat = NFA_STATUS_FAILED;
    tNFA_HANDLE eeHandle = seID;

    ALOGD("%s: seID=0x%X; handle=0x%04x", fn, seID, eeHandle);

    if (mUseOberthurWarmReset)
    {
        //send warm-reset command to Oberthur secure element which deselects the applet;
        //this is an Oberthur-specific command;
        ALOGD("%s: try warm-reset on pipe id 0x%X; cmd=0x%X", fn, mNewPipeId, mOberthurWarmResetCommand);
        SyncEventGuard guard (mRegistryEvent);
        nfaStat = NFA_HciSetRegistry (mNfaHciHandle, mNewPipeId,
                1, 1, &mOberthurWarmResetCommand);
        if (nfaStat == NFA_STATUS_OK)
        {
            mRegistryEvent.wait ();
            ALOGD("%s: completed warm-reset on pipe 0x%X", fn, mNewPipeId);
        }
    }

/* START [160503001J] Configure HCI setting for wired C/E by eSE chip type (Oberthure or Gemalto)*/
    if ( ( gEseChipType == 0x01 ) && (gUseStaticPipe != true) )     // Gemalto eSE
    {
        ALOGD("%s: NFA_HCI_EVT_EVT_SOFT_RESET", fn);

        SyncEventGuard guard (mTransceiveEvent);
        nfaStat = NFA_HciSendEvent (mNfaHciHandle, mNewPipeId, EVT_SOFT_RESET, 0x00, NULL, 00, NULL, 100);

        if (nfaStat == NFA_STATUS_OK)
        {
            ALOGD("%s: wait to terminate NFA_HCI_EVT_EVT_SOFT_RESET", fn);
            mTransceiveEvent.wait (100);
        }
    }
/* END [160503001J] Configure HCI setting for wired C/E by eSE chip type (Oberthure or Gemalto)*/

    mIsPiping = false;

    // Re-enable UICC low-power mode
    //android::setUiccIdleTimeout (true);
    // Re-enable RF discovery
    // Note that it only effactuates the current configuration,
    // so if polling/listening were configured OFF (forex because
    // the screen was off), they will stay OFF with this call.
    android::startRfDiscovery(true);

    return true;
}


/*******************************************************************************
**
** Function:        transceive
**
** Description:     Send data to the secure element; read it's response.
**                  xmitBuffer: Data to transmit.
**                  xmitBufferSize: Length of data.
**                  recvBuffer: Buffer to receive response.
**                  recvBufferMaxSize: Maximum size of buffer.
**                  recvBufferActualSize: Actual length of response.
**                  timeoutMillisec: timeout in millisecond.
**
** Returns:         True if ok.
**
*******************************************************************************/
bool SecureElement::transceive (UINT8* xmitBuffer, INT32 xmitBufferSize, UINT8* recvBuffer,
        INT32 recvBufferMaxSize, INT32& recvBufferActualSize, INT32 timeoutMillisec)
{
    static const char fn [] = "SecureElement::transceive";
    tNFA_STATUS nfaStat = NFA_STATUS_FAILED;
    bool isSuccess = false;
    bool waitOk = false;
    UINT8 newSelectCmd[NCI_MAX_AID_LEN + 10];

    ALOGD ("%s: enter; xmitBufferSize=%ld; recvBufferMaxSize=%ld; timeout=%ld", fn, xmitBufferSize, recvBufferMaxSize, timeoutMillisec);

    // Check if we need to replace an "empty" SELECT command.
    // 1. Has there been a AID configured, and
    // 2. Is that AID a valid length (i.e 16 bytes max), and
    // 3. Is the APDU at least 4 bytes (for header), and
    // 4. Is INS == 0xA4 (SELECT command), and
    // 5. Is P1 == 0x04 (SELECT by AID), and
    // 6. Is the APDU len 4 or 5 bytes.
    //
    // Note, the length of the configured AID is in the first
    //   byte, and AID starts from the 2nd byte.
    if (mAidForEmptySelect[0]                           // 1
        && (mAidForEmptySelect[0] <= NCI_MAX_AID_LEN)   // 2
        && (xmitBufferSize >= 4)                        // 3
        && (xmitBuffer[1] == 0xA4)                      // 4
        && (xmitBuffer[2] == 0x04)                      // 5
        && (xmitBufferSize <= 5))                       // 6
    {
        UINT8 idx = 0;

        // Copy APDU command header from the input buffer.
        memcpy(&newSelectCmd[0], &xmitBuffer[0], 4);
        idx = 4;

        // Set the Lc value to length of the new AID
        newSelectCmd[idx++] = mAidForEmptySelect[0];

        // Copy the AID
        memcpy(&newSelectCmd[idx], &mAidForEmptySelect[1], mAidForEmptySelect[0]);
        idx += mAidForEmptySelect[0];

        // If there is an Le (5th byte of APDU), add it to the end.
        if (xmitBufferSize == 5)
            newSelectCmd[idx++] = xmitBuffer[4];

        // Point to the new APDU
        xmitBuffer = &newSelectCmd[0];
        xmitBufferSize = idx;

        ALOGD ("%s: Empty AID SELECT cmd detected, substituting AID from config file, new length=%d", fn, idx);
    }

    {
        SyncEventGuard guard (mTransceiveEvent);
        mActualResponseSize = 0;
        memset (mResponseData, 0, sizeof(mResponseData));
/* START [160503001J] Configure HCI setting for wired C/E by eSE chip type (Oberthure or Gemalto)*/
        if ( (mNewPipeId == STATIC_PIPE_0x72) ||
             (mNewSourceGate ==  APDU_GATE_0xF0 && gUseStaticPipe != 0x01) )
            nfaStat = NFA_HciSendEvent (mNfaHciHandle, mNewPipeId, EVT_SEND_DATA, xmitBufferSize, xmitBuffer, sizeof(mResponseData), mResponseData, timeoutMillisec);
        else
            nfaStat = NFA_HciSendEvent (mNfaHciHandle, mNewPipeId, NFA_HCI_EVT_POST_DATA, xmitBufferSize, xmitBuffer, sizeof(mResponseData), mResponseData, timeoutMillisec);
/* END [160503001J] Configure HCI setting for wired C/E by eSE chip type (Oberthure or Gemalto)*/

        if (nfaStat == NFA_STATUS_OK)
        {
            waitOk = mTransceiveEvent.wait (timeoutMillisec);
            if (waitOk == false) //timeout occurs
            {
                ALOGE ("%s: wait response timeout", fn);
                goto TheEnd;
            }
        }
        else
        {
            ALOGE ("%s: fail send data; error=0x%X", fn, nfaStat);
            goto TheEnd;
        }
    }

    if (mActualResponseSize > recvBufferMaxSize)
        recvBufferActualSize = recvBufferMaxSize;
    else
        recvBufferActualSize = mActualResponseSize;

    memcpy (recvBuffer, mResponseData, recvBufferActualSize);
    isSuccess = true;

TheEnd:
    ALOGD ("%s: exit; isSuccess: %d; recvBufferActualSize: %ld", fn, isSuccess, recvBufferActualSize);
    return (isSuccess);
}


void SecureElement::notifyModeSet (tNFA_HANDLE eeHandle, bool success)
{
    static const char* fn = "SecureElement::notifyModeSet";

    ALOGD("%s : Enter - eehandle = %d, success = %d", __FUNCTION__, eeHandle, success);

// [START] System LSI - GED bug
    //if (success)
    if (!success)
// [END] System LSI - GED bug
    {
        tNFA_EE_INFO *pEE = sSecElem.findEeByHandle (eeHandle);
        if (pEE)
        {
            pEE->ee_status ^= 1;
            ALOGD ("%s: NFA_EE_MODE_SET_EVT; pEE->ee_status: %s (0x%04x)", fn, SecureElement::eeStatusToString(pEE->ee_status), pEE->ee_status);
        }
        else
            ALOGE ("%s: NFA_EE_MODE_SET_EVT; EE: 0x%04x not found.  mActiveEeHandle: 0x%04x", fn, eeHandle, sSecElem.mActiveEeHandle);
    }
    SyncEventGuard guard (sSecElem.mEeSetModeEvent);
    sSecElem.mEeSetModeEvent.notifyOne();
}

/*******************************************************************************
**
** Function:        notifyListenModeState
**
** Description:     Notify the NFC service about whether the SE was activated
**                  in listen mode.
**                  isActive: Whether the secure element is activated.
**
** Returns:         None
**
*******************************************************************************/
void SecureElement::notifyListenModeState (bool isActivated) {
    static const char fn [] = "SecureElement::notifyListenMode";

    ALOGD ("%s: enter; listen mode active=%u", fn, isActivated);

    JNIEnv* e = NULL;
    ScopedAttach attach(mNativeData->vm, &e);
    if (e == NULL)
    {
        ALOGE ("%s: jni env is null", fn);
        return;
    }

/* START [J14127003] - blcok double notify for secure element */
    if ( mActivatedInListenMode == isActivated)
    {
        ALOGD ("%s: already set to %d", __FUNCTION__, isActivated);
        return;
    }
/* END [J14127003] */

    mActivatedInListenMode = isActivated;
    if (isActivated) {
        e->CallVoidMethod (mNativeData->manager, android::gCachedNfcManagerNotifySeListenActivated);
    }
    else {
        e->CallVoidMethod (mNativeData->manager, android::gCachedNfcManagerNotifySeListenDeactivated);
    }

    if (e->ExceptionCheck())
    {
        e->ExceptionClear();
        ALOGE ("%s: fail notify", fn);
    }

    ALOGD ("%s: exit", fn);
}

/*******************************************************************************
**
** Function:        notifyRfFieldEvent
**
** Description:     Notify the NFC service about RF field events from the stack.
**                  isActive: Whether any secure element is activated.
**
** Returns:         None
**
*******************************************************************************/
void SecureElement::notifyRfFieldEvent (bool isActive)
{
    static const char fn [] = "SecureElement::notifyRfFieldEvent";
    ALOGD ("%s: enter; is active=%u", fn, isActive);

    JNIEnv* e = NULL;
    ScopedAttach attach(mNativeData->vm, &e);
    if (e == NULL)
    {
        ALOGE ("%s: jni env is null", fn);
        return;
    }

    mMutex.lock();
    int ret = clock_gettime (CLOCK_MONOTONIC, &mLastRfFieldToggle);
    if (ret == -1) {
        ALOGE("%s: clock_gettime failed", fn);
        // There is no good choice here...
    }

    mMutex.unlock();

    if (e->ExceptionCheck())
    {
        e->ExceptionClear();
        ALOGE ("%s: fail notify", fn);
    }
    ALOGD ("%s: exit", fn);
}

/*******************************************************************************
**
** Function:        resetRfFieldStatus
**
** Description:     Resets the field status.
**                  isActive: Whether any secure element is activated.
**
** Returns:         None
**
*******************************************************************************/
void SecureElement::resetRfFieldStatus ()
{
    static const char fn [] = "SecureElement::resetRfFieldStatus`";
    ALOGD ("%s: enter;", fn);

    mMutex.lock();
    mRfFieldIsOn = false;
    int ret = clock_gettime (CLOCK_MONOTONIC, &mLastRfFieldToggle);
    if (ret == -1) {
        ALOGE("%s: clock_gettime failed", fn);
        // There is no good choice here...
    }
    mMutex.unlock();

    ALOGD ("%s: exit", fn);
}


/*******************************************************************************
**
** Function:        storeUiccInfo
**
** Description:     Store a copy of the execution environment information from the stack.
**                  info: execution environment information.
**
** Returns:         None
**
*******************************************************************************/
void SecureElement::storeUiccInfo (tNFA_EE_DISCOVER_REQ& info)
{
    static const char fn [] = "SecureElement::storeUiccInfo";
    ALOGD ("%s:  Status: %u   Num EE: %u", fn, info.status, info.num_ee);

    SyncEventGuard guard (mUiccInfoEvent);
    memcpy (&mUiccInfo, &info, sizeof(mUiccInfo));
    for (UINT8 xx = 0; xx < info.num_ee; xx++)
    {
        //for each technology (A, B, F, B'), print the bit field that shows
        //what protocol(s) is support by that technology
        ALOGD ("%s   EE[%u] Handle: 0x%04x  techA: 0x%02x  techB: 0x%02x  techF: 0x%02x  techBprime: 0x%02x",
                fn, xx, info.ee_disc_info[xx].ee_handle,
                info.ee_disc_info[xx].la_protocol,
                info.ee_disc_info[xx].lb_protocol,
                info.ee_disc_info[xx].lf_protocol,
                info.ee_disc_info[xx].lbp_protocol);
    }
    mUiccInfoEvent.notifyOne ();
}

/*******************************************************************************
**
** Function         getSeVerInfo
**
** Description      Gets version information and id for a secure element.  The
**                  seIndex parmeter is the zero based index of the secure
**                  element to get verion info for.  The version infommation
**                  is returned as a string int the verInfo parameter.
**
** Returns          ture on success, false on failure
**
*******************************************************************************/
bool SecureElement::getSeVerInfo(int seIndex, char * verInfo, int verInfoSz, UINT8 * seid)
{
    ALOGD("%s: enter, seIndex=%d", __FUNCTION__, seIndex);

    if (seIndex > (mActualNumEe-1))
    {
        ALOGE("%s: invalid se index: %d, only %d SEs in system", __FUNCTION__, seIndex, mActualNumEe);
        return false;
    }

    *seid = mEeInfo[seIndex].ee_handle;

    if ((mEeInfo[seIndex].num_interface == 0) || (mEeInfo[seIndex].ee_interface[0] == NCI_NFCEE_INTERFACE_HCI_ACCESS) )
    {
        return false;
    }

    strncpy(verInfo, "Version info not available", verInfoSz-1);
    verInfo[verInfoSz-1] = '\0';


// [START] wired C/E
    //UINT8 pipe = (mEeInfo[seIndex].ee_handle == EE_HANDLE_ESE) ? 0x70 : 0x71;
    //UINT8 host = (pipe == STATIC_PIPE_0x70) ? 0x02 : 0x03;
    //UINT8 gate = (pipe == STATIC_PIPE_0x70) ? 0xF0 : 0xF1;
    UINT8 pipe = 0x72;
    UINT8 host = 0x01;
    UINT8 gate = 0xF0;
// [END] wired C/E


    tNFA_STATUS nfaStat = NFA_HciAddStaticPipe(mNfaHciHandle, host, gate, pipe);
    if (nfaStat != NFA_STATUS_OK)
    {
        ALOGE ("%s: NFA_HciAddStaticPipe() failed, pipe = 0x%x, error=0x%X", __FUNCTION__, pipe, nfaStat);
        return true;
    }

    SyncEventGuard guard (mVerInfoEvent);
    if (NFA_STATUS_OK == (nfaStat = NFA_HciGetRegistry (mNfaHciHandle, pipe, 0x02)))
    {
        if (false == mVerInfoEvent.wait(200))
        {
            ALOGE ("%s: wait response timeout", __FUNCTION__);
        }
        else
        {
            snprintf(verInfo, verInfoSz-1, "Oberthur OS S/N: 0x%02x%02x%02x", mVerInfo[0], mVerInfo[1], mVerInfo[2]);
            verInfo[verInfoSz-1] = '\0';
        }
    }
    else
    {
        ALOGE ("%s: NFA_HciGetRegistry () failed: 0x%X", __FUNCTION__, nfaStat);
    }
    return true;
}

/*******************************************************************************
**
** Function         getActualNumEe
**
** Description      Returns number of secure elements we know about.
**
** Returns          Number of secure elements we know about.
**
*******************************************************************************/
UINT8 SecureElement::getActualNumEe()
{
    return mActualNumEe;
}

/*******************************************************************************
**
** Function:        nfaHciCallback
**
** Description:     Receive Host Controller Interface-related events from stack.
**                  event: Event code.
**                  eventData: Event data.
**
** Returns:         None
**
*******************************************************************************/
void SecureElement::nfaHciCallback (tNFA_HCI_EVT event, tNFA_HCI_EVT_DATA* eventData)
{
    static const char fn [] = "SecureElement::nfaHciCallback";
    ALOGD ("%s: event=0x%X", fn, event);

    switch (event)
    {
    case NFA_HCI_REGISTER_EVT:
        {
            ALOGD ("%s: NFA_HCI_REGISTER_EVT; status=0x%X; handle=0x%X", fn,
                    eventData->hci_register.status, eventData->hci_register.hci_handle);
            SyncEventGuard guard (sSecElem.mHciRegisterEvent);
            sSecElem.mNfaHciHandle = eventData->hci_register.hci_handle;
            sSecElem.mHciRegisterEvent.notifyOne();
        }
        break;

    case NFA_HCI_ALLOCATE_GATE_EVT:
        {
            ALOGD ("%s: NFA_HCI_ALLOCATE_GATE_EVT; status=0x%X; gate=0x%X", fn, eventData->status, eventData->allocated.gate);
            SyncEventGuard guard (sSecElem.mAllocateGateEvent);
            sSecElem.mCommandStatus = eventData->status;
            sSecElem.mNewSourceGate = (eventData->allocated.status == NFA_STATUS_OK) ? eventData->allocated.gate : 0;
            sSecElem.mAllocateGateEvent.notifyOne();
        }
        break;

    case NFA_HCI_DEALLOCATE_GATE_EVT:
        {
            tNFA_HCI_DEALLOCATE_GATE& deallocated = eventData->deallocated;
            ALOGD ("%s: NFA_HCI_DEALLOCATE_GATE_EVT; status=0x%X; gate=0x%X", fn, deallocated.status, deallocated.gate);
            SyncEventGuard guard (sSecElem.mDeallocateGateEvent);
            sSecElem.mDeallocateGateEvent.notifyOne();
        }
        break;

    case NFA_HCI_GET_GATE_PIPE_LIST_EVT:
        {
            ALOGD ("%s: NFA_HCI_GET_GATE_PIPE_LIST_EVT; status=0x%X; num_pipes: %u  num_gates: %u", fn,
                    eventData->gates_pipes.status, eventData->gates_pipes.num_pipes, eventData->gates_pipes.num_gates);
            SyncEventGuard guard (sSecElem.mPipeListEvent);
            sSecElem.mCommandStatus = eventData->gates_pipes.status;
            sSecElem.mHciCfg = eventData->gates_pipes;
            sSecElem.mPipeListEvent.notifyOne();
// [START] System LSI - event source
            sSecElem.notifyTransaction();
// [END] System LSI - event source
        }
        break;

    case NFA_HCI_CREATE_PIPE_EVT:
        {
            ALOGD ("%s: NFA_HCI_CREATE_PIPE_EVT (81 12); status=0x%X; pipe=0x%X; src gate=0x%X; dest host=0x%X; dest gate=0x%X", fn,
                    eventData->created.status, eventData->created.pipe, eventData->created.source_gate, eventData->created.dest_host, eventData->created.dest_gate);
            SyncEventGuard guard (sSecElem.mCreatePipeEvent);
            sSecElem.mCommandStatus = eventData->created.status;
            sSecElem.mNewPipeId = eventData->created.pipe;
            sSecElem.mCreatePipeEvent.notifyOne();
        }
        break;

    case NFA_HCI_OPEN_PIPE_EVT:
        {
            ALOGD ("%s: NFA_HCI_OPEN_PIPE_EVT; status=0x%X; pipe=0x%X", fn, eventData->opened.status, eventData->opened.pipe);
            SyncEventGuard guard (sSecElem.mPipeOpenedEvent);
            sSecElem.mCommandStatus = eventData->opened.status;
            sSecElem.mPipeOpenedEvent.notifyOne();
        }
        break;

    case NFA_HCI_EVENT_SENT_EVT:
        ALOGD ("%s: NFA_HCI_EVENT_SENT_EVT; status=0x%X", fn, eventData->evt_sent.status);
        break;

    case NFA_HCI_RSP_RCVD_EVT: //response received from secure element
        {
            tNFA_HCI_RSP_RCVD& rsp_rcvd = eventData->rsp_rcvd;
            ALOGD ("%s: NFA_HCI_RSP_RCVD_EVT; status: 0x%X; code: 0x%X; pipe: 0x%X; len: %u", fn,
                    rsp_rcvd.status, rsp_rcvd.rsp_code, rsp_rcvd.pipe, rsp_rcvd.rsp_len);
        }
        break;

    case NFA_HCI_GET_REG_RSP_EVT :
        ALOGD ("%s: NFA_HCI_GET_REG_RSP_EVT; status: 0x%X; pipe: 0x%X, len: %d", fn,
                eventData->registry.status, eventData->registry.pipe, eventData->registry.data_len);
// [START] wired C/E
        //if (eventData->registry.data_len >= 19 && ((eventData->registry.pipe == STATIC_PIPE_0x70) || (eventData->registry.pipe == STATIC_PIPE_0x71)))
        if (eventData->registry.data_len >= 19 && (eventData->registry.pipe == STATIC_PIPE_0x72))
// [END] wired C/E
        {
            SyncEventGuard guard (sSecElem.mVerInfoEvent);
            // Oberthur OS version is in bytes 16,17, and 18
            sSecElem.mVerInfo[0] = eventData->registry.reg_data[16];
            sSecElem.mVerInfo[1] = eventData->registry.reg_data[17];
            sSecElem.mVerInfo[2] = eventData->registry.reg_data[18];
            sSecElem.mVerInfoEvent.notifyOne ();
        }
        break;

    case NFA_HCI_EVENT_RCVD_EVT:
        ALOGD ("%s: NFA_HCI_EVENT_RCVD_EVT; code: 0x%X; pipe: 0x%X; data len: %u", fn,
                eventData->rcvd_evt.evt_code, eventData->rcvd_evt.pipe, eventData->rcvd_evt.evt_len);

/* START [160503001J] Configure HCI setting for wired C/E by eSE chip type (Oberthure or Gemalto)*/
        if ( (eventData->rcvd_evt.pipe == STATIC_PIPE_0x72) ||
             ((sSecElem.mNewSourceGate == APDU_GATE_0xF0) && (gUseStaticPipe != 0x01)) )
/* END [160503001J] Configure HCI setting for wired C/E by eSE chip type (Oberthure or Gemalto)*/
        {
            ALOGD ("%s: NFA_HCI_EVENT_RCVD_EVT; data from static pipe", fn);
            SyncEventGuard guard (sSecElem.mTransceiveEvent);
            sSecElem.mActualResponseSize = (eventData->rcvd_evt.evt_len > MAX_RESPONSE_SIZE) ? MAX_RESPONSE_SIZE : eventData->rcvd_evt.evt_len;
            sSecElem.mTransceiveEvent.notifyOne ();
        }
        else if (eventData->rcvd_evt.evt_code == NFA_HCI_EVT_POST_DATA)
        {
            ALOGD ("%s: NFA_HCI_EVENT_RCVD_EVT; NFA_HCI_EVT_POST_DATA", fn);
            SyncEventGuard guard (sSecElem.mTransceiveEvent);
            sSecElem.mActualResponseSize = (eventData->rcvd_evt.evt_len > MAX_RESPONSE_SIZE) ? MAX_RESPONSE_SIZE : eventData->rcvd_evt.evt_len;
            sSecElem.mTransceiveEvent.notifyOne ();
        }
        else if (eventData->rcvd_evt.evt_code == NFA_HCI_EVT_TRANSACTION)
        {
            ALOGD ("%s: NFA_HCI_EVENT_RCVD_EVT; NFA_HCI_EVT_TRANSACTION", fn);
            // If we got an AID, notify any listeners
            if ((eventData->rcvd_evt.evt_len > 3) && (eventData->rcvd_evt.p_evt_buf[0] == 0x81) )
            {
// [START] System LSI - event source
                if (!sSecElem.requestNotifyTransaction(eventData->rcvd_evt.pipe,
                                    eventData->rcvd_evt.p_evt_buf, eventData->rcvd_evt.evt_len))
                {
                    int aidlen = eventData->rcvd_evt.p_evt_buf[1];
                    UINT8* data = NULL;
                    UINT8 datalen = 0;
                    if((eventData->rcvd_evt.evt_len > 2+aidlen) && (eventData->rcvd_evt.p_evt_buf[2+aidlen] == 0x82))
                    {
                        datalen = eventData->rcvd_evt.p_evt_buf[2+aidlen+1];
                        data  = &eventData->rcvd_evt.p_evt_buf[2+aidlen+2];
                    }

                    ALOGD ("%s: request NotifyTransaction() failed.", fn);
                    sSecElem.notifyTransactionListenersOfAid (&eventData->rcvd_evt.p_evt_buf[2],aidlen,data,datalen,0);
                }
// [END] System LSI - event source
            }
        }
        else if (eventData->rcvd_evt.evt_code == NFA_HCI_EVT_CONNECTIVITY)
        {
            ALOGD ("%s: NFA_HCI_EVENT_RCVD_EVT; NFA_HCI_EVT_CONNECTIVITY", fn);

            int pipe = (eventData->rcvd_evt.pipe);
            sSecElem.notifyConnectivityListeners ();
        }
        else
        {
            ALOGD ("%s: NFA_HCI_EVENT_RCVD_EVT; ################################### eventData->rcvd_evt.evt_code = 0x%x , NFA_HCI_EVT_CONNECTIVITY = 0x%x", fn, eventData->rcvd_evt.evt_code, NFA_HCI_EVT_CONNECTIVITY);

            ALOGD ("%s: NFA_HCI_EVENT_RCVD_EVT; ################################### ", fn);

        }
        break;

    case NFA_HCI_SET_REG_RSP_EVT: //received response to write registry command
        {
            tNFA_HCI_REGISTRY& registry = eventData->registry;
            ALOGD ("%s: NFA_HCI_SET_REG_RSP_EVT; status=0x%X; pipe=0x%X", fn, registry.status, registry.pipe);
            SyncEventGuard guard (sSecElem.mRegistryEvent);
            sSecElem.mRegistryEvent.notifyOne ();
            break;
        }

    default:
        ALOGE ("%s: unknown event code=0x%X ????", fn, event);
        break;
    }
}

/* START [160503001J] Configure HCI setting for wired C/E by eSE chip type (Oberthure or Gemalto)*/
void SecureElement::getESEchipTypeCallback (UINT8 event, UINT16 param_len, UINT8* p_param)
{
    ALOGD ("%s: event=0x%X", __FUNCTION__, event);

    //  for Oberthur eSE
    gEseChipType = 0x00;                            // Oberthur (0x00) , Gemalto (0x01)
    gUseStaticPipe = true;                          // Gemalto (false), Oberthur (true)
    gIsSupportConcurrentWCE = false;        // Gemalto (true), Oberthur (false)
    gIsSetWcebyPropCmd = true;
    gGatePipe = 0x72;
    sSecElem.mOberthurWarmResetCommand = false;

    SyncEventGuard guard (sSecElem.mSecVsCmdEvent);
    if(param_len > 0)
    {
        if(p_param[0x03] == 0x02)           //  for Gemalto eSE
        {
            gUseStaticPipe = true;
            gIsSupportConcurrentWCE = true;
            sSecElem.mOberthurWarmResetCommand = false;
        }
    }

    if(param_len > 0)
        ALOGD ("%s: eSE Chip Type =0x%X", __FUNCTION__, p_param[0x03]);
    else
        ALOGD ("%s: eSE Chip Type =OT", __FUNCTION__);

    sSecElem.mSecVsCmdEvent.notifyOne();
}
/* END [160503001J] Configure HCI setting for wired C/E by eSE chip type (Oberthure or Gemalto)*/

/*******************************************************************************
**
** Function:        findEeByHandle
**
** Description:     Find information about an execution environment.
**                  eeHandle: Handle to execution environment.
**
** Returns:         Information about an execution environment.
**
*******************************************************************************/
tNFA_EE_INFO *SecureElement::findEeByHandle (tNFA_HANDLE eeHandle)
{
    for (UINT8 xx = 0; xx < mActualNumEe; xx++)
    {
        if (mEeInfo[xx].ee_handle == eeHandle)
            return (&mEeInfo[xx]);
    }
    return (NULL);
}


/*******************************************************************************
**
** Function:        getDefaultEeHandle
**
** Description:     Get the handle to the execution environment.
**
** Returns:         Handle to the execution environment.
**
*******************************************************************************/
tNFA_HANDLE SecureElement::getDefaultEeHandle ()
{
    UINT16 overrideEeHandle = NFA_HANDLE_GROUP_EE | mActiveSeOverride;
    // Find the first EE that is not the HCI Access i/f.
    for (UINT8 xx = 0; xx < mActualNumEe; xx++)
    {
        ALOGD("xx = %d - num_interface = %d, ee_interface = %d, ee_status = %d", xx, mEeInfo[xx].num_interface, mEeInfo[xx].ee_interface[0], mEeInfo[xx].ee_status);

        if (mActiveSeOverride && (overrideEeHandle != mEeInfo[xx].ee_handle))
            continue; //skip all the EE's that are ignored
        if ((mEeInfo[xx].num_interface != 0) &&
            (mEeInfo[xx].ee_interface[0] != NCI_NFCEE_INTERFACE_HCI_ACCESS) &&
            (mEeInfo[xx].ee_status != NFC_NFCEE_STATUS_INACTIVE))
            return (mEeInfo[xx].ee_handle);
    }
    return NFA_HANDLE_INVALID;
}


/*******************************************************************************
**
** Function:        findUiccByHandle
**
** Description:     Find information about an execution environment.
**                  eeHandle: Handle of the execution environment.
**
** Returns:         Information about the execution environment.
**
*******************************************************************************/
tNFA_EE_DISCOVER_INFO *SecureElement::findUiccByHandle (tNFA_HANDLE eeHandle)
{
    for (UINT8 index = 0; index < mUiccInfo.num_ee; index++)
    {
        if (mUiccInfo.ee_disc_info[index].ee_handle == eeHandle)
        {
            return (&mUiccInfo.ee_disc_info[index]);
        }
    }
    ALOGE ("SecureElement::findUiccByHandle:  ee h=0x%4x not found", eeHandle);
    return NULL;
}



// [START] System LSI - event source
/*******************************************************************************
**
** Function:        findDestByPipe
**
** Description:     Find a destination of the pipe.
**                  pipe: pipe of the EVT_TRANSACTION.
**
** Returns:         Destination of the pipe.
**
*******************************************************************************/
int SecureElement::findDestByPipe (UINT8 pipe)
{
    int dest = -1;
    for (UINT8 xx = 0; xx < sSecElem.mHciCfg.num_pipes; xx++)
    {
        ALOGD ("SecureElement::findDestByPipe: find event source from mHciCfg: pipe(%d)[%d], recved pipe[%d]", xx, sSecElem.mHciCfg.pipe[xx].pipe_id, pipe);
        if (sSecElem.mHciCfg.pipe[xx].pipe_id == pipe)
        {
            dest = sSecElem.mHciCfg.pipe[xx].dest_host;
            break;
        }
    }

    if (dest == -1)
    {
        for (UINT8 xx = 0; xx < sSecElem.mHciCfg.num_uicc_created_pipes; xx++)
        {
            ALOGD ("SecureElement::findDestByPipe: find event source from mHciCfg: pipe(%d)[%d], recved pipe[%d]", xx, sSecElem.mHciCfg.uicc_created_pipe[xx].pipe_id, pipe);
            if (sSecElem.mHciCfg.uicc_created_pipe[xx].pipe_id == pipe)
            {
                dest = sSecElem.mHciCfg.uicc_created_pipe[xx].dest_host;
                break;
            }
        }
    }

    switch (dest)
    {
    case NFA_HCI_HOST_CONTROLLER:
        return 0x01;            // HCI network
    case NFA_HCI_DH_HOST:
        return 0x00;
    case NFA_HCI_UICC_HOST:
        return EE_HANDLE_UICC & ~NFA_HANDLE_GROUP_EE;
    case (NFA_HCI_UICC_HOST+1): // eSE
        return EE_HANDLE_ESE & ~NFA_HANDLE_GROUP_EE;
    }
    return -1;
}
// [END] System LSI - event source



/*******************************************************************************
**
** Function:        eeStatusToString
**
** Description:     Convert status code to status text.
**                  status: Status code
**
** Returns:         None
**
*******************************************************************************/
const char* SecureElement::eeStatusToString (UINT8 status)
{
    switch (status)
    {
    case NFC_NFCEE_STATUS_ACTIVE:
        return("Connected/Active");
    case NFC_NFCEE_STATUS_INACTIVE:
        return("Connected/Inactive");
    case NFC_NFCEE_STATUS_REMOVED:
        return("Removed");
    }
    return("?? Unknown ??");
}


/*******************************************************************************
**
** Function:        connectionEventHandler
**
** Description:     Receive card-emulation related events from stack.
**                  event: Event code.
**                  eventData: Event data.
**
** Returns:         None
**
*******************************************************************************/
void SecureElement::connectionEventHandler (UINT8 event, tNFA_CONN_EVT_DATA* /*eventData*/)
{
    switch (event)
    {
    case NFA_CE_UICC_LISTEN_CONFIGURED_EVT:
        {
            SyncEventGuard guard (mUiccListenEvent);
            mUiccListenEvent.notifyOne ();
        }
        break;
    }
}

/*******************************************************************************
**
** Function:        isBusy
**
** Description:     Whether controller is routing listen-mode events to
**                  secure elements or a pipe is connected.
**
** Returns:         True if either case is true.
**
*******************************************************************************/
bool SecureElement::isBusy ()
{
    bool retval = (mCurrentRouteSelection == SecElemRoute) || mIsPiping;
    ALOGD ("SecureElement::isBusy: %u", retval);
    return retval;
}

/* START [160503001J] Configure HCI setting for wired C/E by eSE chip type (Oberthure or Gemalto)*/
bool SecureElement::isSupportedConcurrentWCE ()
{
    ALOGD ("SecureElement::isSupportedConcurrentWCE: %u", gIsSupportConcurrentWCE);
    return gIsSupportConcurrentWCE;
}
/* END [160503001J] Configure HCI setting for wired C/E by eSE chip type (Oberthure or Gemalto)*/
