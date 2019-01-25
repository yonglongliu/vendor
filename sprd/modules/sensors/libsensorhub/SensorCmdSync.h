/*
 * Copyright (C) 2008 The Android Open Source Project
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

#ifndef SENSORCMDSYNC_H_
#define SENSORCMDSYNC_H_

#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

/******************************
#   Acronym table
#       NonWakeUp:NWku
#       WakeUp:Wku
#       Driver:Drv
#       Command:Cmd
#       Sensor:Sen
#       Manager:Mgr
#       Enable:En
#       Disable:Dsb
#       Calibrator:Calib
#       Function:Fun
#       Check:Chk
#       Kernel:Knl
#       Reserved:Rsv
#       Sensors Hal:Hal
#       Receiver:Rcvr
#       Transceiver :Trx
#       Attributes Node:Attr
 ******************************/
/*
 * define
 * */
#define BitSetData(x, y)     (x | (1 << y))
#define BitCheck(x, y)       (x & (1 << y))
#define BitClean(x, y)       (x & (~(1 << y)))

/*
 *Structure
 * */
typedef struct {
    uint8_t     Cmd;
    uint8_t     Length;
    uint16_t   Id;
    void    *DataPoint;
}mMsg_t, *pmMsg_t;

typedef struct {
    uint8_t Cmd;
    uint8_t Length;
    uint16_t HandleID;
    union {
        uint8_t udata[4];
        struct {
            int8_t status;
            int8_t type;
        };
    };
    union {
        float fdata[3];
        struct {
            float x;
            float y;
            float z;
        };
        struct {
            uint64_t    pedometer;
        };
        struct {
            float   heart_rate;
        };
    };
    int64_t timestamp;     // millisecond
}mSenMsgTpA_t, *pmSenMsgTpA_t;

typedef struct {
    uint8_t Cmd;
    uint8_t Length;
    uint16_t HandleID;
    union {
        uint8_t udata[4];
        struct {
            int8_t status;
            int8_t type;
        };
    };
    union {
        float fdata[6];
        struct {
            float uncalib[3];
            float bias[3];
        };
    };
    int64_t timestamp;     // millisecond
}mSenMsgTpB_t, *pmSenMsgTpB_t;

typedef enum {
    /*MCU Internel Cmd: Range 0~63*/
    /*Driver Control enable/disable/set rate*/
    eMcuCmd = 0,
    eMcuSenEn,
    eMcuSenDsb,
    eMcuSenSetRate,
    /*Sensors Calibrator*/
    eMcuSenCalib,
    eMcuSenSelftest,
    /*User profile*/
    eMcuSenSetting,
    /*Kernel Cmd*/
    eMcuEnable,
    eMcuBatch,
    eMcuFlush,
    eMcuShowEnList,
    eMcuShowSenInfo,
    eMcuCalib,
    eMcuCustomSetting,

    eKnlCmd = 64,
    eKnlSenData,
    eKnlSenDataPackage,  // for multi package use.
    eKnlEnList,
    eKnlLog,
    eKnlResetSenPower,
    eKnlCalibInfo,
    eKnlCalibData,
    eKnlRsvrAttrEnable,
    eKnlRsvrAttrBatch,

    eHalCmd = 128,
    eHalSenData,
    eHalFlush,
    eHalLog,
    eHalSensorInfo,
    eHalLogCtl,

    eSpecialCmd = 192,
    eMcuReady,
    eCommonDriverReady,
} teCmd;

#define MAXI_CWM_MSG_SIZE (40)

#endif  // SENSORCMDSYNC_H_
