/*
 * Copyright (C) 2014 SAMSUNG S.LSI
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
#ifdef SEC_NFC_DTA_SUPPORT

#include <semaphore.h>
#include <errno.h>
#include <ScopedLocalRef.h>
#include <ScopedPrimitiveArray.h>
#include "NfcJniUtil.h"
#include "NfcAdaptation.h"
#include "OverrideLog.h"
#include "SyncEvent.h"
#include "NfcTag.h"
#include "JavaClassConstants.h"

#include "nfa_api.h"
#include "nfa_dta_api.h"

#include <cutils/properties.h>

// [START] NFCSM00000267 - Override sleep timeout for DTA
#define NCI_PROP_DTA_SLEEP_TIME 0x30
// [END] NFCSM00000267 - Override sleep timeout for DTA

namespace android
{
extern bool nfcManager_isNfcActive();
extern void nativeNfcTag_abortWaits ();
}

static bool         DTAmode = false;
static bool         isDTAEnabled = false;
static SyncEvent    sNfaDTAEvent;
static SyncEvent    sNfaDTAStartEvent;
static SyncEvent    sNfaDTAStopEvent;
static SyncEvent    sNfaDTAConfigEvent;

enum {
    DTA_ENABLE = 0x00,
    DTA_DISABLE,
    DTA_START,
    DTA_STOP,
    DTA_CONFIG,
};

static void nfaSECDTACallback (UINT8 event, tNFA_DTA_CBACK_DATA *eventData)
{
    ALOGD ("%s: Enter; event=0x%02X", __FUNCTION__, event);

    switch (event) {
    case NFA_DTA_ENABLED_EVT:
    {
        ALOGD ("%s: NFA_DTA_ENABLED_EVT - status = 0x%02X", __FUNCTION__, eventData->status);
        isDTAEnabled = (eventData->status == NFA_STATUS_OK) ? true : false;
        SyncEventGuard guard (sNfaDTAEvent);
        sNfaDTAEvent.notifyOne ();
    }
        break;
    case NFA_DTA_DISABLED_EVT:
    {
        ALOGD ("%s: NFA_DTA_DISABLED_EVT", __FUNCTION__);
        isDTAEnabled = false;
        SyncEventGuard guard (sNfaDTAEvent);
        sNfaDTAEvent.notifyOne ();
    }
        break;
    case NFA_DTA_START_EVT:
    {
        ALOGD ("%s: NFA_DTA_START_EVT - status = 0x%02X", __FUNCTION__, eventData->start);
        SyncEventGuard guard (sNfaDTAStartEvent);
        sNfaDTAStartEvent.notifyOne ();
    }
        break;
    case NFA_DTA_STOP_EVT:
    {
        ALOGD ("%s: NFA_DTA_STOP_EVT - status = 0x%02X", __FUNCTION__, eventData->status);
        SyncEventGuard guard (sNfaDTAStopEvent);
        sNfaDTAStopEvent.notifyOne ();
    }
        break;
    case NFA_DTA_CONFIG_EVT:
    {
        ALOGD ("%s: NFA_DTA_CONFIG_EVT - status = 0x%02X", __FUNCTION__, eventData->status);
        SyncEventGuard guard (sNfaDTAConfigEvent);
        sNfaDTAConfigEvent.notifyOne ();
        break;
    }
    default:
        break;
    }

    ALOGD ("%s: Exit", __FUNCTION__);
}

bool SECDTA_doEnable(bool autoStart)
{
    ALOGD ("%s: enter; auto_start=%d", __FUNCTION__, autoStart);
    tNFA_STATUS stat = NFA_STATUS_OK;

    if (true == DTAmode)
    {
        ALOGD ("%s: DTA already running", __FUNCTION__);
        return JNI_TRUE;
    }

    if (android::nfcManager_isNfcActive())
    {
        ALOGD ("%s: NFC is on. Please turn NFC off", __FUNCTION__);
        return JNI_FALSE;
    }

    NfcAdaptation& theInstance = NfcAdaptation::GetInstance();
    theInstance.Initialize(); //start GKI, NCI task, NFC task

    DTAmode = true;

    {
        SyncEventGuard guard (sNfaDTAEvent);
        tHAL_NFC_ENTRY* halFuncEntries = theInstance.GetHalEntryFuncs ();
        NFA_DTA_Init(halFuncEntries);

        stat = NFA_DTA_Enable(autoStart, nfaSECDTACallback);

        if (stat == NFA_STATUS_OK)
        {
            sNfaDTAEvent.wait ();

            if (true == isDTAEnabled)
                goto TheEnd;
        }
    }

    ALOGE ("%s: Failed DTA Enabling; error=0x%X", __FUNCTION__, stat);
    theInstance.Finalize();
    DTAmode = false;

TheEnd:
    ALOGD ("%s: exit", __FUNCTION__);
    return isDTAEnabled ? JNI_TRUE : JNI_FALSE;
}

bool SECDTA_doDisable()
{
    ALOGD ("%s: Enter", __FUNCTION__);
    tNFA_STATUS stat;

    if (!isDTAEnabled)
    {
        ALOGD ("%s: Already disabled", __FUNCTION__);
        return JNI_TRUE;
    }

    {
        SyncEventGuard guard(sNfaDTAEvent);
        stat = NFA_DTA_Disable();

        if(stat == NFA_STATUS_OK)
            sNfaDTAEvent.wait ();
    }

    android::nativeNfcTag_abortWaits();
    NfcAdaptation& theInstance = NfcAdaptation::GetInstance();
    NfcTag::getInstance().abort ();
    theInstance.Finalize();
    isDTAEnabled = false;
    DTAmode = false;

    ALOGD ("%s: Exit", __FUNCTION__);
    return JNI_TRUE;
}

bool SECDTA_doStart(int patternNo)
{
    ALOGD ("%s: Enter - patternNo : 0x%02X", __FUNCTION__, patternNo);
    tNFA_STATUS stat;
    if(false == DTAmode) {
        ALOGD ("%s: DTA is not running.", __FUNCTION__);
        return JNI_FALSE;
    }

// [START] NFCSM00000267 - Override sleep timeout for DTA
    ALOGD("%s: Override sleep timeout", __FUNCTION__);
    stat = NFA_SendVsCommand(NCI_PROP_DTA_SLEEP_TIME, 0, NULL, NULL);
    if (stat != NFA_STATUS_OK)
        ALOGE("%s: failed to set DTA sleep timeout", __FUNCTION__);
// [END] NFCSM00000267 - Override sleep timeout for DTA
    {
        SyncEventGuard guard(sNfaDTAStartEvent);
        stat = NFA_DTA_Start(patternNo, 0, NULL);
        if(NFA_STATUS_OK == stat)
            sNfaDTAStartEvent.wait();
    }

    ALOGD ("%s: EXit", __FUNCTION__);
    return (NFA_STATUS_OK == stat) ? JNI_TRUE : JNI_FALSE;
}


bool SECDTA_doStop()
{
    ALOGD ("%s: Enter", __FUNCTION__);
    tNFA_STATUS stat;
    if(false == DTAmode) {
        ALOGD ("%s: DTA is not running.", __FUNCTION__);
        return JNI_FALSE;
    }

    {
        SyncEventGuard guard(sNfaDTAStopEvent);
        stat = NFA_DTA_Stop();
        if(NFA_STATUS_OK == stat)
            sNfaDTAStopEvent.wait();
    }
    ALOGD ("%s: EXit", __FUNCTION__);
    return (NFA_STATUS_OK == stat) ? JNI_TRUE : JNI_FALSE;
}

bool SECDTA_doConfig(int configItem, int configData)
{
    ALOGD ("%s: Enter - ConfigItem = 0x%02X", __FUNCTION__, configItem);

    tNFA_STATUS stat;

    {
//    SyncEventGuard guard(sNfaDTAConfigEvent);
    stat = NFA_DTA_Config(configItem, configData);
//    if(NFA_STATUS_OK == stat)
//    sNfaDTAStartEvent.wait();
    }

    ALOGD ("%s: EXit", __FUNCTION__);
    return (NFA_STATUS_OK == stat) ? JNI_TRUE : JNI_FALSE;
}
/*********************************
 * Dta server/client
 ********************************/
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <Dta.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>

char *SOCK_ADDR = "com.slsi.nfc.dta";
void* dta_server(void *arg)
{
    void *tmpRet = NULL;
    int server, client;
    int serverLen, clientLen;
    struct sockaddr_un serverAddr, clientAddr;
    unsigned char cmd, ret = 0;

    ALOGD("%s: dta server initialize", __FUNCTION__);
    if (access(SOCK_ADDR, F_OK) == 0)
        unlink(SOCK_ADDR);

    if ((server = socket(AF_LOCAL, SOCK_STREAM, PF_UNIX)) < 0)
    {
        ALOGD("%s: socket fail", __FUNCTION__);
        return tmpRet;
    }
    ALOGD("%s: socket OK", __FUNCTION__);

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sun_family = AF_LOCAL;
    serverAddr.sun_path[0] = '\0';
    strcpy(serverAddr.sun_path+1, SOCK_ADDR);
    serverLen = 1 + strlen(SOCK_ADDR) + offsetof(struct sockaddr_un, sun_path);

    if (bind(server, (struct sockaddr *)&serverAddr, serverLen) < 0)
    {
        ALOGD("%s: bind fail", __FUNCTION__);
        close (server);
        return tmpRet;
    }
    ALOGD("%s: bind OK", __FUNCTION__);

    if (listen(server, 5) == -1)
    {
        ALOGD("%s: listen fail", __FUNCTION__);
        close (server);
        return tmpRet;
    }
    ALOGD("%s: listen OK", __FUNCTION__);

    while (ret != 0xFF)
    {
        client = accept(server, NULL, NULL);
        if (client == -1)
            // accetp error
            continue;
        ALOGD("%s: accept OK", __FUNCTION__);

        if (read(client, &cmd, 1) != 1)
        {
            close(client);
            continue;
        }

        // handler
        ret = 0;
        ALOGD("%s: event id: %d", __FUNCTION__, cmd);
        switch (cmd) {
        case DTA_ENABLE:
            if (read(client, &cmd, 1) != 1)
                ret = 0xFF;
            else
                SECDTA_doEnable((int)cmd);
            break;

        case DTA_DISABLE:
            SECDTA_doDisable();
            break;

        case DTA_START:
/* START [J16072801] To sync up new DTA application */
            unsigned char arg[2];
            if(read(client, arg, 2) != 2)
                ret = 0xFF;
            else
                SECDTA_doStart((int)((int)(arg[1] << 8 | (int)arg[0])));
/* END [J16072801] To sync up new DTA application */
            break;

        case DTA_STOP:
            SECDTA_doStop();
            break;

        case DTA_CONFIG:
            {
                unsigned char arg[2];
                if (read(client, arg, 2) != 2)
                    ret = 0xFF;
                else
                    SECDTA_doConfig((int)arg[0], (int)arg[1]);
            }
            break;

        default:
            ALOGD("%s: dta server terminate", __FUNCTION__);
            ret = 0xFF;
            break;
        }

        if (ret != 0xFF)
            write(client, &ret, 1);

        close(client);
    }
    close (server);

    return tmpRet;
}

bool sendMsgToDtaServer(unsigned char msg, int argc, unsigned char *argv)
{
    int client, clientLen;
    struct sockaddr_un clientAddr;
    unsigned char ret;

    if ((client = socket(AF_LOCAL, SOCK_STREAM, PF_UNIX)) < 0)
    {
        ALOGD("%s: socket fail", __FUNCTION__);
        return false;
    }

    memset(&clientAddr, 0, sizeof(clientAddr));
    clientAddr.sun_family = AF_LOCAL;
    clientAddr.sun_path[0] = '\0';
    strcpy(clientAddr.sun_path + 1, SOCK_ADDR);
    clientLen = 1 + strlen(SOCK_ADDR) + offsetof(struct sockaddr_un, sun_path);

    if (connect(client, (struct sockaddr *)&clientAddr, clientLen) < 0)
    {
        ALOGD("%s: connect fail", __FUNCTION__);
        close(client);
        return false;
    }

    if (write(client, &msg, 1) != 1)
    {
        ALOGD("%s: write fail", __FUNCTION__);
        close(client);
        return false;
    }

    if (argc > 0 && argv != NULL)
    {
        ALOGD("%s: send args %d", __FUNCTION__, argc);
        if (write(client, argv, argc) != argc)
        {
            ALOGD("%s: write arg fail", __FUNCTION__);
            close(client);
            return false;
        }
        ALOGD("%s: write success!", __FUNCTION__);
    }

    if (read(client, &ret, 1) != 1)
    {
        ALOGD("%s: read fail", __FUNCTION__);
        close(client);
        return false;
    }

    close(client);
    ALOGD("%s: result: %d", __FUNCTION__, ret);
    return true;
}

bool startDtaServer()
{
    pthread_attr_t  attr;
    pthread_t    task;
    ALOGD("%s: enter", __FUNCTION__);
    pthread_attr_init(&attr);
    if ( pthread_create( &task, &attr, dta_server, NULL))
    {
        ALOGD("%s : pthread_create failed", __FUNCTION__);
    }
    pthread_attr_destroy(&attr);

    ALOGD("%s: exit", __FUNCTION__);
    return true;
}

bool stopDtaServer()
{
    sendMsgToDtaServer('E', 0, NULL);
    return true;
}

#else

// Dummy API for Undefined SEC_NFC_DTA_SUPPORT Feature.

bool startDtaServer()
{
    return false;
}

bool stopDtaServer()
{
    return false;
}

#endif //SEC_NFC_DTA_SUPPORT
