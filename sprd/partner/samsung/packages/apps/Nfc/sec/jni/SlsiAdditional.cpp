/*
 *    Copyright (C) 2014 SAMSUNG S.LSI
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at:
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 *   Author: Woonki Lee <woonki84.lee@samsung.com>
 *           Heejae Kim<heejae12.kim@samsung.com>
 *
 */

#include "OverrideLog.h"
#include "NfcJniUtil.h"
#include "NfcAdaptation.h"
#include "SyncEvent.h"
#include "PeerToPeer.h"
#include "RoutingManager.h"
#include "NfcTag.h"
#include "config.h"
#include "PowerSwitch.h"
#include "Pn544Interop.h"
#include "SecureElement.h"
#include "Mutex.h"
#include "IntervalTimer.h"
/* START [J15012301] */
#include <ScopedLocalRef.h>
#include <ScopedUtfChars.h>
/* END [J15012301] */
#include <ScopedPrimitiveArray.h>

extern "C"
{
    #include "nfa_api.h"
    #include "nfc_api.h"
    #include "nfa_p2p_api.h"
    #include "rw_api.h"
    #include "nfa_ee_api.h"
    #include "nfc_brcm_defs.h"
    #include "ce_api.h"
}

/* START [J16021501] - Block Cover Authentication during RF Activated */
extern bool gActivated;
/* END [J16021501] - Block Cover Authentication during RF Activated */

namespace android
{
/***** Extern Other Files Functions *****/
    extern void startRfDiscovery(bool isStart);
    extern void nfcManager_enableDiscoveryImpl (tNFA_TECHNOLOGY_MASK tech_mask, bool enable_lptd,
            bool reader_mode, bool enable_host_routing, bool restart, UINT16 discovery_duration, jboolean enable_p2p);
    extern void nfcManager_disableDiscoveryImpl (bool screenoffCE);
    extern bool nfcManager_isNfcActive();

    extern tNFA_STATUS stopPolling_rfDiscoveryDisabled();
    extern tNFA_STATUS startPolling_rfDiscoveryDisabled(tNFA_TECHNOLOGY_MASK tech_mask);

    extern tNFA_INTF_TYPE getCurrentRfInterface();
    extern void setCurrentRfInterface(tNFA_INTF_TYPE rfInterface);

/***** S.LSI Common Module *****/
    /* [J00000000] - SLSI callback hooker */
    void slsiEventHandler (UINT8 event, tNFA_CONN_EVT_DATA* data);
    /* [J00000002] - SLSI initialize */
    void slsiInitialize(void);
    /* [J00000003] - SLSI deintialize */
    void slsiDeinitialize(void);
    void slsiSetFlag(UINT16 flag);
    bool slsiIsFlag(UINT16 flag);
    void slsiClearFlag(UINT16 flag);

    bool checkFwVersion(int m1, int m2, int b1, int b2);
    bool lessFwVersion(int m1, int m2, int b1, int b2);

    /* START [J14120201] - Generic ESE ID */
    jint slsiGetSeId(jint genSeId);
    jint slsiGetGenSeId(int seId);
    jint slsiGetGenSeId(tNFA_HANDLE handle);
    /* END [J14120201] */

    /* START [J14111101] - pending enable discovery during listen mode */
    void enableDiscoveryAfterDeactivation (tNFA_TECHNOLOGY_MASK tech_mask, bool enable_lptd,
            bool reader_mode, bool enable_host_routing, bool restart, UINT16 discovery_duration, bool enable_p2p);
    void disableDiscoveryAfterDeactivation (void);
    /* END [J14111101] */

/***** S.LSI Additional Features *****/
    // Flip Cover
    jbyteArray nfcManager_transceiveAuthData(JNIEnv *e, jobject o, jbyteArray data);
    jbyteArray nfcManager_startCoverAuth(JNIEnv *e, jobject o);
    jboolean nfcManager_stopCoverAuth(JNIEnv *e, jobject o);

/* Addition for NFC service */
    /* START [J14111200] - Routing setting */
    jboolean nfcManager_setRoutingEntry(JNIEnv* e, jobject, jint type, jint value, jint route, jint power);
    jboolean nfcManager_clearRoutingEntry(JNIEnv*, jobject, jint type);
    /* END [J14111200] */

/* START [P161006001] - Configure Default SecureElement and LMRT */
    void nfcManager_doSetDefaultRoute(JNIEnv *e, jobject o, jint defaultSeId);
/* END [P161006001] - Configure Default SecureElement and LMRT */

    /* START [J14111303] - screen or power state */
    void nfcManager_doSetScreenOrPowerState (JNIEnv *e, jobject o, jint state);
    /* END [J14111303] */

    /* START [J14111100] - support secure element */
    jintArray nfcManager_doGetSecureElementList(JNIEnv* e, jobject);
    void nfcManager_doSelectSecureElement(JNIEnv*, jobject, jint seId);
    jint nfcManager_getSecureElementTechList(JNIEnv*, jobject);
    void nfcManager_setSecureElementListenTechMask(JNIEnv*, jobject, jint tech_mask);
    void nfcManager_doDeselectSecureElement(JNIEnv*, jobject,  jint seId);
    /* END [J14111100] */

    /* START [J14121101] - Remaining AID Size */
    jint nfcManager_getRemainingAidTableSize(JNIEnv*, jobject);
    /* END [J14121101] */

    /* [J14121201] - Aid Table Size */
    jint nfcManager_getAidTableSize(JNIEnv*, jobject);
    /* END [J14121201] */

    /* START [J15100101] - Get Tech information */
    jint nfcManager_getSeSupportedTech(JNIEnv*, jobject);
    /* END [J15100101] */

    /* START [J15052703] - FW Update Req : Get FW Version */
    extern bool sIsNfcOffStatus;
    extern tNFC_FW_VERSION fw_version;
    /* END [J15052703] */

/***** Workaround *****/
    int doWorkaround(UINT16 wrId, void *arg);
}

namespace android
{
static SyncEvent        sNfaSetConfigEvent;  // event for Set_Config....
static SyncEvent        sNfaVsCmdEvent;

static UINT8            gNfaVsCmdResult;
static Mutex            gMutex;
static IntervalTimer    gTimer;
static IntervalTimer    gTimerNxpP2p;
static bool             bIsSecElemSelected = false;  //has NFC service selected a sec elem
static bool             bIsSlsiInit = false;
static bool             bNxpP2p = false;
static UINT16           gFlipCoverDataLen;
static UINT8            *pFlipCoverData;

tNFC_ACTIVATE_DEVT      gActivationData;
UINT16                  gRunningPatchFlag;
UINT16                  gVendorConfig;
UINT8                   gScreenState;
UINT8                   gCurrentScreenState;
UINT32                  gFwVer = 0;

/* START [J15012301] - For Sending SEc's Proprietary Commands */
static SyncEvent            sNfaSecVsCmdEvent;
static UINT8                sNfaSecVsCmdResult;
static UINT16               NCI_data_len;
static UINT8               *NCI_p_data;

#define VS_NO_RESPONSE        0xFF
#define VS_ZERO_LENGTH        0xFE
/* END [J15012301] - For Sending SEc's Proprietary Commands */

static struct enableDiscoveryData {
    UINT16  tech_mask;
    bool    enable_lptd;
    bool    reader_mode;
    bool    enable_host_routing;
    bool    restart;
    bool    discovery_duration;
    bool    enable_p2p;
} sEnableDiscoveryData;

static bool bDiscoveryEnabled = false;  //is polling or listening

/******************* START S.LSI Flag Setting *************************/
#define SLSI_PATCHFLAG_NONE                     0x0000
#define SLSI_PATCHFLAG_WAIT_ENABLE_DISCOVERY    0x0002
#define SLSI_PATCHFLAG_WAIT_DISABLE_DISCOVERY   0x0004
#define SLSI_PATCHFLAG_WAIT_SCREEN_STATE        0x0008

#define SLSI_WRFLAG_SKIP_RF_NTF                 0x0010
#define SLSI_WRFLAG_NXP_P2P                     0x0040  /* PATCHID: J14111107 */
#define SLSI_WRFLAG_BLOCK_FLIPCOVER             0x0080  /* PATCHID: J15012201 */
#define SLSI_WRFLAG_FLIPCOVER_AUTHING           0x0100  /* PATCHID: J15012201 */
/******************* END S.LSI Flag Setting *************************/

#define VENDCFG_MASK_NONE        0x00
#define VENDCFG_MASK_NFCEE_OFF   0x01   /* TODO: remove it, it is update by L */
#define VENDCFG_NFCEE_OFF        2
#define VENDCFG_NFCEE_ON         3

#define NCI_PROP_GET_RFREG_OID      0x21
#define NCI_PROP_SET_RFREG_OID      0x22
#define NCI_PROP_SIM_TEST           0x31
#define NCI_PROP_PRBS_TEST          0x32
#define NCI_PROP_SCREEN_STATE_OID   0x38
#define NCI_PROP_PARTIAL_AID_OID    0x39

#define NFA_PROP_GET_RFREG_EVT      0x61
#define NFA_PROP_SET_RFREG_EVT      0x62

#define VS_NO_RESPONSE      0xFF
#define VS_ZERO_LENGTH      0xFE

#define DEFAULT_TECH_MASK           (NFA_TECHNOLOGY_MASK_A \
                                     | NFA_TECHNOLOGY_MASK_B \
                                     | NFA_TECHNOLOGY_MASK_F \
                                     | NFA_TECHNOLOGY_MASK_ISO15693 \
                                     | NFA_TECHNOLOGY_MASK_A_ACTIVE \
                                     | NFA_TECHNOLOGY_MASK_F_ACTIVE \
                                     | NFA_TECHNOLOGY_MASK_KOVIO)

#define POWER_STATE_ON              6
#define POWER_STATE_OFF             7

/* START [J14120201] - Generic ESE ID */
#define SEID_HOST       0x00
#define SEID_ESE        0x02
#define SEID_UICC       0x03

#define SEID_GEN_HOST   0x00
#define SEID_GEN_ESE    0x01
#define SEID_GEN_UICC   0x02
/* END [J14120201] */

/* START [P161006001] - Configure Default SecureElement and LMRT */
// Routeing Type Masks
#define ROUTE_TYPE_TECH     0x01
#define ROUTE_TYPE_PROTO    0x02

// Power State Masks
#define POWER_STATE_SWITCHED_ON        0x01
#define POWER_STATE_SWITCHED_OFF       0x02
#define POWER_STATE_BATTERY_OFF        0x04
#define POWER_STATE_SWITCHED_ON_SUB1   0x08
#define POWER_STATE_SWITCHED_ON_SUB2   0x10
#define POWER_STATE_SWITCHED_ON_SUB3   0x20

// SEC Screen State Masks
#define SEC_SCREEN_STATE_OFF        POWER_STATE_SWITCHED_ON_SUB1
#define SEC_SCREEN_STATE_ON_LOCK    POWER_STATE_SWITCHED_ON_SUB2
/* END [P161006001] - Configure Default SecureElement and LMRT */

#define FW_VER(m1, m2, b1, b2)      ((m1 * 0x10000) + (m2 * 0x100) + (b2 < 10 ? b2 : b2+6))
#define FW_VER_1_1_14               FW_VER(1,1,0,14)

static void nfcManager_doSetScreenOrPowerStateImpl();
static void slsiDoPendedImpl (union sigval);
static void setPartialAID(UINT8 *p_option, UINT8 option_length);

static void setPartialAIDCallback (UINT8 event, UINT16 param_len, UINT8* p_param);
static void nfaSimTestCallback(tNFC_VS_EVT event, UINT16 data_len, UINT8 *p_data);
static void nfaPrbsTestCallback(tNFC_VS_EVT event, UINT16 data_len, UINT8 *p_data);
static void nfaFlipCoverCallback(UINT8 event, UINT16 data_len, UINT8 *p_data);
static void doSetScreenStateCallback (UINT8 event, UINT16 param_len, UINT8* p_param);

static void setN3RfReg(void);
static void setN3RfRegCallback (UINT8 event, UINT16 param_len, UINT8* p_param);

static void doWrNxpP2p (union sigval);   /* [J14111107] - NXP P2P */

static UINT32 getFwVersion();

/* START [J00000000] - SLSI callback hooker */
/*******************************************************************************
**
** Block description:   slsi event hooker
**
** User case:           slsiEventHandler (event, data)
**
*******************************************************************************/
void slsiEventHandler (UINT8 event, tNFA_CONN_EVT_DATA* data)
{
    tNFA_STATUS status = NFA_STATUS_FAILED;
    tNFA_DEACTIVATE_TYPE deact_type = NFA_DEACTIVATE_TYPE_IDLE;

    if (!bIsSlsiInit)
    {
        ALOGD("%s: slsi additional is alread deinit", __FUNCTION__);
        return;
    }

    ALOGD("%s: event= %u", __FUNCTION__, event);

    switch (event)
    {
    case NFA_ACTIVATED_EVT:

        {
            tNFA_ACTIVATED& activated = data->activated;
            gActivationData = activated.activate_ntf;    // TODO: is it copied?
/* START [J15033101] - TECH recovery after NXP_P2P workaround */
            if(slsiIsFlag(SLSI_WRFLAG_NXP_P2P))
                gTimerNxpP2p.kill();
/* END [J15033101] - TECH recovery after NXP_P2P workaround */
        }
        break;

    case NFA_DEACTIVATED_EVT:
    case NFA_CE_DEACTIVATED_EVT:

        /* START [J14111107] - NXP P2P */
        gTimerNxpP2p.kill();
        if (slsiIsFlag(SLSI_WRFLAG_NXP_P2P))
            doWorkaround(SLSI_WRFLAG_NXP_P2P, NULL);
        /* END [J14111107] - NXP P2P */

/* START [J14111101] - pending enable discovery during listen mode */
        if (SecureElement::getInstance().isPeerInListenMode() != true)
        {
            if ((slsiIsFlag(SLSI_PATCHFLAG_WAIT_ENABLE_DISCOVERY)) ||
                (slsiIsFlag(SLSI_PATCHFLAG_WAIT_DISABLE_DISCOVERY)) ||
                (slsiIsFlag(SLSI_PATCHFLAG_WAIT_SCREEN_STATE)))
            {
                gTimer.set (100, slsiDoPendedImpl); // TODO: is it enough 100ms?
            }
        }
/* END [J14111101] - pending enable discovery during listen mode */
        break;

    case NFA_NDEF_DETECT_EVT:
    ALOGD ("%s: KHJ, 0x%02X, 0x%02X", __FUNCTION__, data->ndef_detect.detail_status, data->ndef_detect.detail_status);
        if ((NCI_STATUS_RF_PROTOCOL_ERR == data->ndef_detect.detail_status) ||
            (NCI_STATUS_TIMEOUT == data->ndef_detect.detail_status))
        { /* START [J14111107] - NXP P2P */
            if (gActivationData.protocol == NCI_PROTOCOL_ISO_DEP &&
                gActivationData.rf_tech_param.mode == NCI_DISCOVERY_TYPE_POLL_A)
            {
                bool val = true;
                doWorkaround(SLSI_WRFLAG_NXP_P2P, (void *)&val);
            }
            /* END [J14111107] - NXP P2P */
        }
        break;

    }
}
/* END [J00000000] - SLSI callback hooker */

static void slsiDoPendedImpl (union sigval)
{
    ALOGD ("%s: enter", __FUNCTION__);
    gMutex.lock();
/* START [J14111101_Part3] - pending enable discovery during listen mode */
    if (SecureElement::getInstance().isPeerInListenMode())
/* END [J14111101_Part3] - pending enable discovery during listen mode */
    {
        ALOGD ("%s: activated listen again", __FUNCTION__);
    }
    else
    {
        if (slsiIsFlag(SLSI_PATCHFLAG_WAIT_ENABLE_DISCOVERY))
        {
            slsiClearFlag(SLSI_PATCHFLAG_WAIT_ENABLE_DISCOVERY);
            nfcManager_enableDiscoveryImpl (sEnableDiscoveryData.tech_mask,
                    sEnableDiscoveryData.enable_lptd,
                    sEnableDiscoveryData.reader_mode,
                    sEnableDiscoveryData.enable_host_routing,
                    sEnableDiscoveryData.restart,
                    sEnableDiscoveryData.discovery_duration,
                    sEnableDiscoveryData.enable_p2p);
        }

        if (slsiIsFlag(SLSI_PATCHFLAG_WAIT_DISABLE_DISCOVERY))
        {
            slsiClearFlag(SLSI_PATCHFLAG_WAIT_DISABLE_DISCOVERY);
            nfcManager_disableDiscoveryImpl (true);
        }

        if (slsiIsFlag(SLSI_PATCHFLAG_WAIT_SCREEN_STATE))
        {
            slsiClearFlag(SLSI_PATCHFLAG_WAIT_SCREEN_STATE);
            nfcManager_doSetScreenOrPowerStateImpl ();
        }
    }
    gMutex.unlock();
    ALOGD ("%s: exit", __FUNCTION__);
}

/* START [J00000001] - SLSI vendor config */
/*******************************************************************************
**
** Block description:   set vendor configuration
**
** User case:           be called by user
**
*******************************************************************************/
void setVenConfigValue(int config)
{
    UINT16 venConfig = (UINT16) config;
    ALOGD ("%s: venConfig : 0x%X", __FUNCTION__, venConfig);

    switch (venConfig)
    {
/* START [J14111201] - disable SE during NFC-OFF */
    case VENDCFG_NFCEE_OFF:
        gVendorConfig |= VENDCFG_MASK_NFCEE_OFF;
        break;

    case VENDCFG_NFCEE_ON:
        gVendorConfig &= ~VENDCFG_MASK_NFCEE_OFF;
        break;
    }
/* END [J14111201] */
}
/* END [J00000001] */

/* START [J00000002] - SLSI initialize */
/*******************************************************************************
**
** Block description:   startup configuration
**
** User case:           slsiStartupConfig()
**
*******************************************************************************/
void slsiInitialize(void)
{
    memset((void *)&sEnableDiscoveryData, 0, sizeof(sEnableDiscoveryData));

    bIsSlsiInit = true;
    gRunningPatchFlag = SLSI_PATCHFLAG_NONE;

    getFwVersion();

/* START [J14121202] - skip RF INFO notify to service */
    slsiSetFlag(SLSI_WRFLAG_SKIP_RF_NTF);
/* END [J14121202] */
    UINT8  field_info_param[] = { 0x01 };
    ALOGD ("%s: Enabling RF field events", __FUNCTION__);
    if (NFA_SetConfig(NCI_PARAM_ID_RF_FIELD_INFO, sizeof(field_info_param), &field_info_param[0]) != NFA_STATUS_OK)
        ALOGE ("%s: fail to set RF field", __FUNCTION__);

    /* START [J15012602] Set 'AID Matching Mode' */
    unsigned long   num = 0;
    UINT8 para_matching[2]={0,0};
    UINT8 para_len = 0x02;

    para_matching[0] = 0x01;
    if (GetNumValue("AID_MATCHING_MODE", &num, sizeof(num)))
    {
        if(num == 0x00)
            para_matching[1] = 0x04;        // Exact matching only
        else if(num == 0x01)
            para_matching[1] = 0x06;        // Exact+Prefix matching
        else
            para_matching[1] = 0x05;        // Prefix matching only
    }
    else
        para_matching[1] = 0x06;

    setPartialAID(para_matching, 0x02);
    /* END [J15012602] Set 'AID Matching Mode' */

    gVendorConfig = VENDCFG_MASK_NONE;
}
/* END [J00000002] - SLSI initialize */

/* START [J0000003] - SLSI deintialize */
/*******************************************************************************
**
** Block description:   slsi deinitialize
**
** User case:           slsiDeinitialize()
**
*******************************************************************************/
void slsiDeinitialize(void)
{
    gRunningPatchFlag = SLSI_PATCHFLAG_NONE;

/* START [J14111201] - disable SE during NFC-OFF */
    if ((gVendorConfig & VENDCFG_MASK_NFCEE_OFF) == VENDCFG_MASK_NFCEE_OFF)
        RoutingManager::getInstance().onNfccShutdown();
/* END [J14111201] */

    gVendorConfig = VENDCFG_MASK_NONE;

/* START [J14111100] - support secure element */
    bIsSecElemSelected = false;
/* END [J14111100] */
    bIsSlsiInit = false;
}
/* END [J00000003] - SLSI deintialize */

/* START [J14111101] - pending enable discovery during listen mode */
/*******************************************************************************
**
** Function:        pending enable discovery during listen mode
**
** Description:     Set screen state
**
** Returns:         None
**
*****
**************************************************************************/
void enableDiscoveryAfterDeactivation (tNFA_TECHNOLOGY_MASK tech_mask, bool enable_lptd,
    bool reader_mode, bool enable_host_routing, bool restart, UINT16 discovery_duration,
    bool enable_p2p)
{
    gMutex.lock();
    slsiClearFlag(SLSI_PATCHFLAG_WAIT_DISABLE_DISCOVERY);
    slsiSetFlag(SLSI_PATCHFLAG_WAIT_ENABLE_DISCOVERY);

    sEnableDiscoveryData.tech_mask             = tech_mask;
    sEnableDiscoveryData.enable_lptd           = enable_lptd;
    sEnableDiscoveryData.reader_mode           = reader_mode;
    sEnableDiscoveryData.enable_host_routing   = enable_host_routing;
    sEnableDiscoveryData.restart               = restart;
    sEnableDiscoveryData.discovery_duration    = discovery_duration;
    sEnableDiscoveryData.enable_p2p            = enable_p2p;
    gMutex.unlock();
}

void disableDiscoveryAfterDeactivation (void)
{
    gMutex.lock();
    slsiClearFlag(SLSI_PATCHFLAG_WAIT_ENABLE_DISCOVERY);
    slsiSetFlag(SLSI_PATCHFLAG_WAIT_DISABLE_DISCOVERY);
    gMutex.unlock();
}
/* END [J14111101] */

/*******************************************************************************
**
** Block description:   Set partial AID
**
** User case:           setPartialAID(p_option, option_length)
**                       - p_option:       partial setting
**                       - option_length:  length of p_option
**
** Returns:             void
**
*******************************************************************************/
static void setPartialAID(UINT8 *p_option, UINT8 option_length)
{
    tNFA_STATUS stat = NFA_STATUS_FAILED;
    UINT8 buffer[10];
    UINT8 length, *p_payload;

    ALOGD ("%s: enter", __FUNCTION__);

    memset(buffer, 0, 5);

    if (p_option == NULL || option_length < 1)
    {
        if(GetStrValue(NAME_PARTIAL_AID, (char *)buffer, sizeof(buffer)))
        {
            length = buffer[0];
            p_payload = buffer + 1;
        }
        else
            goto TheEnd;
    }
    else
    {
        length = option_length;
        p_payload = p_option;
    }

    ALOGD ("%s: length=%d", __FUNCTION__, length);

    {
        SyncEventGuard guard (sNfaVsCmdEvent);
        stat = NFA_SendVsCommand(NCI_PROP_PARTIAL_AID_OID, length, p_payload, setPartialAIDCallback);
        if (stat == NFA_STATUS_OK)
            sNfaVsCmdEvent.wait ();
    }

TheEnd:
    ALOGD ("%s: exit; state=0x%x", __FUNCTION__, stat);
}

static void setPartialAIDCallback (UINT8 event, UINT16 param_len, UINT8* p_param)
{
    ALOGD ("%s: event=0x%X", __FUNCTION__, event);
    SyncEventGuard guard (sNfaVsCmdEvent);
    sNfaVsCmdEvent.notifyOne();
}

/*******************************************************************************
**
** Function:        doWrNxpP2p
**
** Description:     Start or stop workaround to facilitate P2P connection with NXP controllers.
**                  isStartWorkAround: true to start work-around; false to stop work-around.
**
** Returns:         None.
**
*******************************************************************************/
#define NAME_WR_NXP_P2P  "NAME_WR_NXP_P2P"
void doWrNxpP2p (union sigval)
{ /* START [J14111107] - NXP P2P */
    ALOGD ("%s: enter; isStartWorkAround=%u, WarType:%d", __FUNCTION__, bNxpP2p, gRunningPatchFlag);
    tNFA_STATUS stat = NFA_STATUS_FAILED;
    unsigned long isNxpP2pWr = 1;

    if (!GetNumValue(NAME_WR_NXP_P2P, &isNxpP2pWr, sizeof(isNxpP2pWr)))
        isNxpP2pWr = 1;

    if (isNxpP2pWr == 0)
    {
        ALOGD ("%s: work around does not enabled", __FUNCTION__);
        return;
    }

    startRfDiscovery (false);
    if (bNxpP2p)
    {
        stopPolling_rfDiscoveryDisabled ();

        // if you want listen only during WR, remove it
        ALOGD("%s: Enable F tech only", __FUNCTION__);
        if (startPolling_rfDiscoveryDisabled (NFA_TECHNOLOGY_MASK_F) == NFA_STATUS_OK) {
            slsiSetFlag(SLSI_WRFLAG_NXP_P2P);
/* START [J15033101] - TECH recovery after NXP_P2P workaround */
            bNxpP2p = false;
            gTimerNxpP2p.set(100, doWrNxpP2p);
/* END [J15033101] - TECH recovery after NXP_P2P workaround */
        }
    }
    else
    {
        slsiClearFlag(SLSI_WRFLAG_NXP_P2P);

        // if you want listen only during WR, remove it
        stopPolling_rfDiscoveryDisabled ();

        startPolling_rfDiscoveryDisabled (sEnableDiscoveryData.tech_mask);
    }

    startRfDiscovery (true);
    ALOGD ("%s: exit", __FUNCTION__);
}
bool lessFwVersion(int m1, int m2, int b1, int b2)
{
    return (gFwVer < FW_VER(m1, m2, b1, b2));
}

static UINT32 getFwVersion()
{
    tNFC_FW_VERSION fwVersion;

    ALOGD("%s : Enter", __FUNCTION__);
    memset(&fwVersion, 0, sizeof(fwVersion));

    /* START [J15052703] - FW Update Req : Get FW Version*/
    if(sIsNfcOffStatus) // NFC-OFF Status
        fwVersion = NfcAdaptation::fw_version;
    /* END [J15052703] - FW Update Req : Get FW Version*/
    else // NFC-ON Status
        fwVersion = NFC_getFWVersion();

    if(fwVersion.major_version)
    {
        gFwVer =  (UINT32)fwVersion.major_version * 0x10000;
        gFwVer += (UINT16)fwVersion.minor_version * 0x100;
        //gFwVer += (UINT16)fwVersion.build_info_high;       // Can't not display high value of build info.
        gFwVer += (UINT16)fwVersion.build_info_low;
        ALOGD("%s : F/W Version = %x.%x.%x (%x)", __FUNCTION__, fwVersion.major_version,
        fwVersion.minor_version, fwVersion.build_info_low, gFwVer);
    }
    ALOGD("%s : Exit", __FUNCTION__);
    return gFwVer;
}

/*******************************************************************************
**
** Function:        nfcManager_doGetSecureElementList
**
** Description:     Get a list of secure element handles.
**                  e: JVM environment.
**                  o: Java object.
**
** Returns:         List of secure element handles.
**
*******************************************************************************/
jintArray nfcManager_doGetSecureElementList(JNIEnv* e, jobject)
{
    ALOGD ("%s", __FUNCTION__);
    return SecureElement::getInstance().getSecureElementIdList (e);
}

/*******************************************************************************
**
** Function:        nfcManager_doSelectSecureElement
**
** Description:     NFC controller starts routing data in listen mode.
**                  e: JVM environment.
**                  o: Java object.
**
** Returns:         None
**
*******************************************************************************/
void nfcManager_doSelectSecureElement(JNIEnv*, jobject, jint seId)
{
    ALOGD ("%s: enter", __FUNCTION__);
    bool stat = true;

    if (bIsSecElemSelected)
    {
        ALOGD ("%s: already selected", __FUNCTION__);
        goto TheEnd;
    }

    PowerSwitch::getInstance ().setLevel (PowerSwitch::FULL_POWER);

    startRfDiscovery (false);

    stat = SecureElement::getInstance().activate (seId);

    bIsSecElemSelected = true;

    startRfDiscovery (true);
    PowerSwitch::getInstance ().setModeOn (PowerSwitch::SE_ROUTING);

TheEnd:
    ALOGD ("%s: exit", __FUNCTION__);
}

/* START [J14120901] - set ce listen tech */
/*******************************************************************************
**
** Function:        nfcManager_getSecureElementTechList
**
**
** CAS_Kr : Added on AR2.2.2 : [Request] Test code for eSE setting
**  1)API for getting  Technology infor mation which is supported on eSE.
**
*******************************************************************************/
jint nfcManager_getSecureElementTechList(JNIEnv*, jobject)
{
    ALOGD ("%s: enter", __FUNCTION__);
    unsigned long listenMask = RoutingManager::getInstance().getCeListenTech();

    ALOGD ("%s: exit - ret = 0x%02X", __FUNCTION__, listenMask);
    return listenMask;
}

void nfcManager_setSecureElementListenTechMask(JNIEnv*, jobject, jint tech_mask)
{
    ALOGD ("%s: enter - tech_mask = 0x%02X", __FUNCTION__, tech_mask);
    unsigned int se_id = 0x02;      // This variable is prepared for supporting both of SE.

    startRfDiscovery (false);

    PeerToPeer::getInstance().enableP2pListening(false);
    RoutingManager::getInstance().setCeListenTech(tech_mask);

    startRfDiscovery (true);

    ALOGD ("%s: exit", __FUNCTION__);
}
/* END [J14120901] */


/*******************************************************************************
**
** Function:        nfcManager_doDeselectSecureElement
**
** Description:     NFC controller stops routing data in listen mode.
**                  e: JVM environment.
**                  o: Java object.
**
** Returns:         None
**
*******************************************************************************/
void nfcManager_doDeselectSecureElement(JNIEnv*, jobject,  jint seId)
{
    ALOGD ("%s: enter", __FUNCTION__);
    bool bRestartDiscovery = false;

    if (! bIsSecElemSelected)
    {
        ALOGE ("%s: already deselected", __FUNCTION__);
        goto TheEnd2;
    }

    if (PowerSwitch::getInstance ().getLevel() == PowerSwitch::LOW_POWER)
    {
        ALOGD ("%s: do not deselect while power is OFF", __FUNCTION__);
        bIsSecElemSelected = false;
        goto TheEnd;
    }

    // TODO: how to know this?
    if (1/*sRfEnabled*/) {
        // Stop RF Discovery if we were polling
        startRfDiscovery (false);
        bRestartDiscovery = true;
    }

    //if controller is not routing to sec elems AND there is no pipe connected,
    //then turn off the sec elems
    if (SecureElement::getInstance().isBusy() == false)
        SecureElement::getInstance().deactivate (seId);

    bIsSecElemSelected = false;
TheEnd:
    if (bRestartDiscovery)
        startRfDiscovery (true);

    //if nothing is active after this, then tell the controller to power down
    if (! PowerSwitch::getInstance ().setModeOff (PowerSwitch::SE_ROUTING))
        PowerSwitch::getInstance ().setLevel (PowerSwitch::LOW_POWER);

TheEnd2:
    ALOGD ("%s: exit", __FUNCTION__);
}


/* START [J14121101] - Remaining AID Size */
/*******************************************************************************
 **
 ** Function:        nfcManager_getRemainingAidTableSize
 **
 ** Description:     Calculate the free space (bytes) in listen-mode routing table.
 **
 ** Returns:         Number of bytes free.
 **
 *******************************************************************************/
jint nfcManager_getRemainingAidTableSize(JNIEnv*, jobject)
{
    int remainingAidTableSize = 0;

    remainingAidTableSize = NFA_GetRemainingAidTableSize();
    ALOGD ("%s: Remaining AID Table Size : %d", __FUNCTION__, remainingAidTableSize);

    return remainingAidTableSize;
}
/* END [J14121101] */


/* START [J14121201] - Aid Table Size */
/*******************************************************************************
 **
 ** Function:        nfcManager_getAidTableSize
 **
 ** Description:     This function is called to get the AID routing table max size.
 **
 ** Returns:         NFA_EE_MAX_AID_CFG_LEN(230)
 **
 *******************************************************************************/
jint nfcManager_getAidTableSize(JNIEnv*, jobject)
{
    return NFA_GetAidTableSize();
}
/* END [J14121201] */

/* START [J14111200] - Routing setting */
/*******************************************************************************
**
** Function:        isSe
**
*******************************************************************************/
static bool isSe(JNIEnv* e, int seId)
{
    bool ret = false;

    jintArray seArray = SecureElement::getInstance().getSecureElementIdList (e);
    if (seArray != NULL) {
        jint* seList = e->GetIntArrayElements (seArray, 0);
        jsize seLength = e->GetArrayLength(seArray);
        int listIndex;

        for (listIndex = 0; listIndex < seLength; listIndex++) { // Is UICC inserted?
            if (seList[listIndex] == seId) {
                ret = true;
                break;
             }
        }
        e->ReleaseIntArrayElements(seArray, seList, 0);
    } else {
        ALOGD ("%s: seArray is NULL", __FUNCTION__);
    }

    return ret;
}

/*******************************************************************************
**
** Function:        nfcManager_setRoutingEntry
**
*******************************************************************************/
jboolean nfcManager_setRoutingEntry(JNIEnv* e, jobject, jint type,
        jint value, jint route, jint power)
{
    int switchOn, switchOff, sub_state[3];
    int powerState[6];
    int seId, value2;

    ALOGD("%s: Enter, type=%d, value=0x%x, route(gen)=%d, power=%x", __FUNCTION__, type, value, route, power);

/* START [J14120201] - Generic ESE ID */
    seId = (int)slsiGetSeId(route);
/* END [J14120201] */

    // select tech or proto value
    if (type == 0x01)
    {
        value2 = value;
        /*
        value2 = 0;
        if (value & 0x01) value2 |= NFA_TECHNOLOGY_MASK_A;
        if (value & 0x02) value2 |= NFA_TECHNOLOGY_MASK_B;
        if (value & 0x04) value2 |= NFA_TECHNOLOGY_MASK_F;
        */
    }
    else if (type == 0x02)
    {
        value2 = 0;
        if (value & 0x01) value2 |= NFA_PROTOCOL_MASK_ISO_DEP;
        if (value & 0x02) value2 |= NFA_PROTOCOL_MASK_NFC_DEP;
    }
    else {
       ALOGE ("%s: %d is unknown type", __FUNCTION__, type);
       return JNI_FALSE;
    }
/* if we have some request for switching SE ID, use under the code
    if (seId != 0x00 && !isSe(e, seId))
    {
        ALOGE ("%s: %d is not connected", __FUNCTION__, seId);
        if ((seId == SEID_UICC) && isSe(e, SEID_ESE)) {
            seId = SEID_ESE;
            // need to be clear switchOff flag
            // power &= ~0x02;
        }
        else if ((seId == SEID_ESE) && isSe(e, SEID_UICC))
            seId = SEID_UICC;
        else
        {
            ALOGD ("%s: exit - fail.", __FUNCTION__);
            return JNI_FALSE;
        }

        ALOGD ("%s: change to %d", __FUNCTION__, seId);
    }
*/

    switchOn    = (0x01 & power) ? value2 : 0;
    switchOff   = (0x02 & power) ? value2 : 0;
    sub_state[0]= (0x08 & power) ? value2 : 0;
    sub_state[1]= (0x10 & power) ? value2 : 0;
    sub_state[2]= (0x20 & power) ? value2 : 0;

    if (type == 0x01)
        RoutingManager::getInstance().setDefaultTechRouting (seId, value2, power);
    else if (type == 0x02)
        RoutingManager::getInstance().setDefaultProtoRouting (seId, value2, power);

/* START [J14111101_Part2] - pending enable discovery during listen mode */
    unsigned long listenMask = RoutingManager::getInstance().getCeListenTech();

/* START [P1605170002] - Change CE listen tech configuration method for support 0x00 tech*/
    if (type == 0x01)
		listenMask = value2;
	
	RoutingManager::getInstance().setCeListenTech(listenMask);
/* END [P1605170002] - Change CE listen tech configuration method for support 0x00 tech*/	
/* END [J14111101_Part2] - pending enable discovery during listen mode */

    ALOGD ("%s: exit", __FUNCTION__);
    return JNI_TRUE;
}

/*******************************************************************************
**
** Function:        clearRoutingEntry
**
*******************************************************************************/
jboolean nfcManager_clearRoutingEntry(JNIEnv*, jobject, jint type)
{
    return (RoutingManager::getInstance().clearRouting(type) ? JNI_TRUE : JNI_FALSE);
}

/* END [J14111200] */

/* START [P161006001] - Configure Default SecureElement and LMRT */
void nfcManager_doSetDefaultRoute(JNIEnv *e, jobject o, jint defaultSeId)
{
    int TechValue = 0x00;
    int ProtoValue = 0x00;
    int PowerValue = 0x00;
    int SEID = slsiGetSeId(defaultSeId);

    RoutingManager::getInstance().setDefaultSe(SEID);
    RoutingManager::getInstance().clearRouting(ROUTE_TYPE_TECH | ROUTE_TYPE_PROTO);

    if(SEID == SEID_HOST)
    {
        TechValue = NFA_TECHNOLOGY_MASK_A | NFA_TECHNOLOGY_MASK_B;
        ProtoValue = NFA_PROTOCOL_MASK_ISO_DEP;
        PowerValue = POWER_STATE_SWITCHED_ON | SEC_SCREEN_STATE_ON_LOCK;
    }
    else if(SEID == SEID_ESE || SEID == SEID_UICC)
    {
        TechValue = NFA_TECHNOLOGY_MASK_A | NFA_TECHNOLOGY_MASK_B;
        ProtoValue = NFA_PROTOCOL_MASK_ISO_DEP;
        PowerValue = POWER_STATE_SWITCHED_ON | POWER_STATE_SWITCHED_OFF | SEC_SCREEN_STATE_OFF | SEC_SCREEN_STATE_ON_LOCK;
    }

    RoutingManager::getInstance().setDefaultTechRouting(SEID, TechValue, PowerValue);
    RoutingManager::getInstance().setDefaultProtoRouting(SEID, ProtoValue, PowerValue);
    RoutingManager::getInstance().commitRouting();
}
/* END [P161006001] - Configure Default SecureElement and LMRT */

/* START [J14111303] - screen or power state */
/*******************************************************************************
**
** Function:        nfcManager_doSetScreenOrPowerState
**
** Description:     Set screen state
**
** Returns:         None
**
*****
**************************************************************************/
void nfcManager_doSetScreenOrPowerState (JNIEnv *e, jobject o, jint state)
{
    if (state == POWER_STATE_ON || state == POWER_STATE_OFF)
    {
        /* power state off means to disable NFCEE (NFC service is disabled)
         * power state on means to keep NFCEE state (NFC service is running but power is off) */
        ALOGD ("%s: enter; power state=0x%X", __FUNCTION__, state);
        setVenConfigValue(state == POWER_STATE_ON ? VENDCFG_NFCEE_ON : VENDCFG_NFCEE_OFF);
        goto TheEnd;
    }

    ALOGD ("%s: enter; screen state=0x%X", __FUNCTION__, state);

    if(!nfcManager_isNfcActive())
    {
      ALOGD("%s: NFC stack is closed. skip sending screen state", __FUNCTION__);
      return;
    }
/*
    if (slsiIsFlag(SLSI_WRFLAG_FLIPCOVER_AUTHING))
    {
        ALOGD("%s: flip cover authentication is running. stop it!!", __FUNCTION__);
        nfcManager_stopCoverAuth (e, o);
    }
*/

    gScreenState = state;
    gMutex.lock();
/* START [J15033101] - TECH recovery after NXP_P2P workaround */
    slsiClearFlag(SLSI_WRFLAG_NXP_P2P);
/* END [J15033101] - TECH recovery after NXP_P2P workaround */

/* START [J14111101] - pending enable discovery during listen mode */
    if (SecureElement::getInstance().isPeerInListenMode())
/* END [J14111101] - pending enable discovery during listen mode */
    {
        slsiSetFlag(SLSI_PATCHFLAG_WAIT_SCREEN_STATE);
    }
    else
        nfcManager_doSetScreenOrPowerStateImpl ();
    gMutex.unlock();

TheEnd:
    ALOGD ("%s: exit", __FUNCTION__);
}

static void nfcManager_doSetScreenOrPowerStateImpl()
{

    UINT8 screen_state[2] = {0x00, 0x00};
    tNFA_STATUS stat = NFA_STATUS_FAILED;

    ALOGD ("%s: enter; request state=0x%X, current state=0x%X", __FUNCTION__, gScreenState, gCurrentScreenState);
    if (gScreenState == gCurrentScreenState)
        return;

    screen_state[0] = gScreenState;
    {
        SyncEventGuard guard (sNfaVsCmdEvent);
        stat = NFA_SendVsCommand(NCI_PROP_SCREEN_STATE_OID, 1, screen_state, doSetScreenStateCallback);

        if (stat == NFA_STATUS_OK)
        {
            if (sNfaVsCmdEvent.wait (2000) == false)
                ALOGE ("%s: vs command timed out", __FUNCTION__);
        }
    }

    ALOGD ("%s: exit", __FUNCTION__);
}

static void doSetScreenStateCallback (UINT8 event, UINT16 param_len, UINT8* p_param)
{
    ALOGD ("%s: Enter; event=0x%X", __FUNCTION__, event);

    SyncEventGuard guard (sNfaVsCmdEvent);
    sNfaVsCmdEvent.notifyOne();

    // update
    if (param_len > 0 && p_param[param_len - 1] == NFA_STATUS_OK)
        gCurrentScreenState = gScreenState;

    ALOGD ("%s: Exit", __FUNCTION__);
}
/* END [J14111303] */

/*******************************************************************************
**
** Block description:   flag control
**
** User case:
**
** Returns:
**
*******************************************************************************/
void slsiSetFlag(UINT16 flag)
{
    gRunningPatchFlag |= flag;
}
bool slsiIsFlag(UINT16 flag)
{
    return ((gRunningPatchFlag & flag) == flag);
}
void slsiClearFlag(UINT16 flag)
{
    gRunningPatchFlag &= ~flag;
}

/* START [J00000004] - Flip cover */
/*******************************************************************************
**
** Block description:   flip cover
**
** Returns:             Response data
**
*******************************************************************************/
/*******************************************************************************
**
** Function:        nfcManager_transceiveAuthData
**
** Description:     This function is called to send an flip cover authentication
**                         data to NFCC.
**
**                    e: JVM environment.
**                    o: Java object.
**                    data : The command parameter.
**
** Returns:         Response data.
*******************************************************************************/
jbyteArray nfcManager_transceiveAuthData(JNIEnv *e, jobject o, jbyteArray data)
{
    tNFA_STATUS stat = NFA_STATUS_FAILED;
    jbyteArray resp_data = NULL;

    ScopedByteArrayRO bytes(e, data);
    uint8_t* buf = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(&bytes[0]));
    size_t bufLen = bytes.size();

    ALOGD ("%s : bytes.size() : %d ", __FUNCTION__, bytes.size());

    SyncEventGuard guard (sNfaVsCmdEvent);
    gNfaVsCmdResult = VS_NO_RESPONSE;

    stat = NFA_SendVsCommand (0x3B, bufLen, buf, nfaFlipCoverCallback);
    if (stat == NFA_STATUS_OK) {
        ALOGD("%s: waiting sNfaVsCmdEvent", __FUNCTION__);
        sNfaVsCmdEvent.wait();

        if(gFlipCoverDataLen >= 4) {
            if(pFlipCoverData[gFlipCoverDataLen - 1] == 0x0) {
                ALOGD("%s: transceiveAuthData is succeed!", __FUNCTION__);
            } else {
                ALOGE("%s: transceiveAuthData is failed!", __FUNCTION__);
                ALOGE("%s: Response is [%d].", __FUNCTION__, pFlipCoverData[gFlipCoverDataLen - 1]);
                //stat = NFA_STATUS_FAILED;
            }
        } else {
            ALOGE("%s: transceiveAuthData response length is less than 4.", __FUNCTION__);
            //stat = NFA_STATUS_FAILED;
        }

        //resp_data = e->NewByteArray ( (jint) gFlipCoverDataLen);
        //e->SetByteArrayRegion (resp_data, 0, gFlipCoverDataLen,  (jbyte*)pFlipCoverData);

        resp_data = e->NewByteArray ( (jint) (gFlipCoverDataLen - 3));
        e->SetByteArrayRegion (resp_data, 0, gFlipCoverDataLen - 3,  (jbyte*)(pFlipCoverData+3));
    }
    else
        ALOGE("%s: NFA_SendVsCommand() failed: %d", __FUNCTION__, stat);

    return resp_data;
}

/*******************************************************************************
**
** Function:        nfcManager_startCoverAuth
**
** Description:     Send flip cover authentication start command  to CLF.
**
**                    e: JVM environment.
**                    o: Java object.
**
** Returns:         Response data.
*******************************************************************************/
jbyteArray nfcManager_startCoverAuth(JNIEnv *e, jobject o)
{
    tNFA_STATUS stat = NFA_STATUS_FAILED;
    jbyteArray resp_data = NULL;

    UINT8  p[241];
    p[0] = 0x00;
    ALOGD ("%s: enter", __FUNCTION__);

/* START [J15012201] - block flip cover in RF field */
    if (slsiIsFlag(SLSI_WRFLAG_BLOCK_FLIPCOVER) ||
/* START [J16021501] - Block Cover Authentication during RF Activated */
        gActivated)
/* END [J16021501] - Block Cover Authentication during RF Activated */
    {
        UINT8 err_code[] = {0xE0};
        ALOGE ("%s: It cannot start cover auth during RF activated", __FUNCTION__);
        resp_data = e->NewByteArray (1);
        e->SetByteArrayRegion (resp_data, 0, 1,  (jbyte*)(err_code));
        return resp_data;
    }

    startRfDiscovery (false);
    slsiSetFlag(SLSI_WRFLAG_FLIPCOVER_AUTHING);
/* END [J15012201] - block flip cover in RF field */

    //startRfDiscovery (false);

    SyncEventGuard guard (sNfaVsCmdEvent);
    gNfaVsCmdResult = VS_NO_RESPONSE;

    //Authentication Start CMD : 2F 3C 01 00
    //Authentication Start RSP : 4F 3C 13 (serial numbers) 00

    stat = NFA_SendVsCommand (0x3C, 1, p, nfaFlipCoverCallback);
    if (stat == NFA_STATUS_OK) {
        ALOGD("%s: waiting sNfaVsCmdEvent", __FUNCTION__);
        sNfaVsCmdEvent.wait();

        if(gFlipCoverDataLen >= 4) {
            if(pFlipCoverData[3] == 0x0) {
                ALOGD("%s: startCoverAuth is succeed!", __FUNCTION__);
            } else {
                ALOGE("%s: startCoverAuth is failed!", __FUNCTION__);
                ALOGE("%s: Response is [%d].", __FUNCTION__, pFlipCoverData[3]);
                //stat = NFA_STATUS_FAILED;
            }
            resp_data = e->NewByteArray ( (jint) (gFlipCoverDataLen - 3));
            e->SetByteArrayRegion (resp_data, 0, gFlipCoverDataLen - 3,  (jbyte*)(pFlipCoverData+3));
        } else {
            ALOGE("%s: startCoverAuth response length is less than 4.", __FUNCTION__);
            //stat = NFA_STATUS_FAILED;
            resp_data = e->NewByteArray ( (jint) gFlipCoverDataLen);
            e->SetByteArrayRegion (resp_data, 0, gFlipCoverDataLen,  (jbyte*)pFlipCoverData);
        }

    }
    else
        ALOGE("%s: NFA_SendVsCommand() failed: %d", __FUNCTION__, stat);

    ALOGD("%s: exit; gNfaVsCmdResult = 0x%02X", __FUNCTION__, gNfaVsCmdResult);

    return resp_data;
}

/*******************************************************************************
**
** Function:        nfcManager_stopCoverAuth
**
** Description:     Send flip cover authentication end command  to CLF.
**
**                    e: JVM environment.
**                    o: Java object.
**
** Returns:         True if ok.
*******************************************************************************/
jboolean nfcManager_stopCoverAuth(JNIEnv *e, jobject o)
{
    tNFA_STATUS stat = NFA_STATUS_FAILED;

    ALOGD ("%s: enter", __FUNCTION__);

    SyncEventGuard guard (sNfaVsCmdEvent);
    gNfaVsCmdResult = VS_NO_RESPONSE;

    //Authentication END  CMD : 2F 3D
    //Authentication END  RSP : STATUS_OK(0)  or ERROR (1)
    stat = NFA_SendVsCommand (0x3D, 0, NULL, nfaFlipCoverCallback);
    if (stat == NFA_STATUS_OK) {
        ALOGD("%s: waiting sNfaVsCmdEvent", __FUNCTION__);
        sNfaVsCmdEvent.wait();

        //startRfDiscovery (true);

        if(gFlipCoverDataLen >= 4) {
            if(pFlipCoverData[3] == 0x0) {
                ALOGD("%s: stopCoverAuth is succeed!", __FUNCTION__);
            } else {
                ALOGE("%s: stopCoverAuth is failed!", __FUNCTION__);
                ALOGE("%s: Response is [%d].", __FUNCTION__, pFlipCoverData[3]);
                stat = NFA_STATUS_FAILED;
            }
        } else {
            ALOGE("%s: stopCoverAuth response length is less than 4.", __FUNCTION__);
            stat = NFA_STATUS_FAILED;
        }
    }
    else
        ALOGE("%s: NFA_SendVsCommand() failed: %d", __FUNCTION__, stat);

/* START [J15012201] - block flip cover in RF field */
    slsiClearFlag(SLSI_WRFLAG_FLIPCOVER_AUTHING);
/* END [J15012201] - block flip cover in RF field */
    ALOGD("%s: exit; gNfaVsCmdResult = 0x%02X", __FUNCTION__, gNfaVsCmdResult);

    return (stat == NFA_STATUS_OK);
}

// callback
static void nfaFlipCoverCallback(UINT8 event, UINT16 data_len, UINT8 *p_data)
{
    ALOGD("[SEC_VS_NCI_RSP] %s: enter; event=0x%02X, len=%d", __FUNCTION__, event, data_len);

    gFlipCoverDataLen = data_len;
    pFlipCoverData = p_data;

    for (int i=0; i < data_len; i++) {
        ALOGD("[%d]%02X", i, p_data[i]);
    }

    SyncEventGuard guard (sNfaVsCmdEvent);
    if (data_len)
        gNfaVsCmdResult = p_data[data_len - 1];
    else
        gNfaVsCmdResult = VS_ZERO_LENGTH;

    sNfaVsCmdEvent.notifyOne();
    ALOGD("[SEC_VS_NCI_RSP] %s: exit", __FUNCTION__);
}


/////////////////////////////
// SE ID
/* START [J14120201] - Generic ESE ID */
jint slsiGetSeId(jint genSeId)
{
    switch (genSeId)
    {
    case SEID_GEN_ESE:
        return SEID_ESE;
    case SEID_GEN_UICC:
        return SEID_UICC;
    case SEID_GEN_HOST:
        return SEID_HOST;
    }
    return 0xFF;
}

jint slsiGetGenSeId(int seId)
{
    switch (seId)
    {
    case SEID_ESE:
        return SEID_GEN_ESE;
    case SEID_UICC:
        return SEID_GEN_UICC;
    case SEID_HOST:
        return SEID_GEN_HOST;
    }
    return 0xFF;
}

jint slsiGetGenSeId(tNFA_HANDLE handle)
{
    int seId = handle & ~NFA_HANDLE_GROUP_EE;
    return slsiGetGenSeId(seId);
}

/* END [J14120201] */

/* START [J15100101] - Get Tech information */
jint nfcManager_getSeSupportedTech(JNIEnv*, jobject)
{
    return RoutingManager::getInstance().getSeSupportedTech();
}
/* END [J15100101] */

//////////////////////////////
// Workaround function
int doWorkaround(UINT16 wrId, void *arg)
{
    switch (wrId)
    {
    case SLSI_WRFLAG_NXP_P2P:
        if (arg != NULL)
            bNxpP2p = *((bool *)arg);
        else
            bNxpP2p = false;

        gTimerNxpP2p.set(10, doWrNxpP2p);
        break;

        default:
            break;
    }

    return 0;
}

} /* namespace android */
