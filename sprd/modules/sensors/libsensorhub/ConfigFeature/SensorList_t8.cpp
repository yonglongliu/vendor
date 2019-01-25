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

#include "sensors/SensorHandleSync.h"
#include "sensors/SensorConfigFeature.h"

#include "SensorList.h"

static struct sensor_t sSensorList[] = {
    {
        .name = "ST LIS3DH 3-axis Accelerometer",
        .vendor = "SPRD",
        .version = 1,
        .handle = SENSOR_HANDLE_ACCELEROMETER,
        .type = SENSOR_TYPE_ACCELEROMETER,
        .maxRange = (GRAVITY_EARTH * 2.0f),
        .resolution = (GRAVITY_EARTH) / 1024.0f,
        .power = 0.145f,
        .minDelay = 5000,  // fastest is 200Hz
        .fifoReservedEventCount = 0,
        .fifoMaxEventCount = 0,
        .stringType = SENSOR_STRING_TYPE_ACCELEROMETER,
        .requiredPermission = "",
        .maxDelay = 1000000,
        .flags = SENSOR_FLAG_CONTINUOUS_MODE,
        {}
    },
    {
        .name = "EPL2182 Light sensor",
        .vendor = "Eminent",
        .version = 1,
        .handle = SENSOR_HANDLE_LIGHT,
        .type = SENSOR_TYPE_LIGHT,
        .maxRange = 1.0f,
        .resolution = 102400.0f,
        .power = 0.5f,
        .minDelay = 0,
        .fifoReservedEventCount = 0,
        .fifoMaxEventCount = 0,
        .stringType = SENSOR_STRING_TYPE_LIGHT,
        .requiredPermission = "",
        .maxDelay = 0,
        .flags = SENSOR_FLAG_ON_CHANGE_MODE,
        {}
    },
    {
        .name = "EPL2182 Proximity sensor",
        .vendor = "Eminent",
        .version = 1,
        .handle = SENSOR_HANDLE_PROXIMITY,
        .type = SENSOR_TYPE_PROXIMITY,
        .maxRange = 1.0f,
        .resolution = 10240.0f,
        .power = 0.5f,
        .minDelay = 20000,
        .fifoReservedEventCount = 0,
        .fifoMaxEventCount = 0,
        .stringType = SENSOR_STRING_TYPE_PROXIMITY,
        .requiredPermission = "",
        .maxDelay = 0,
        .flags = SENSOR_FLAG_ON_CHANGE_MODE,
        {}
    },
};

static shub_drv_cfg_t sSensorDrvCfg[] = {
    {
        .ctlName    = "/dev/lis3dh_acc",
        .ctlMode    = 0,
        .drvName    = "accelerometer",
        .drvMode    = 0,
        .sensorList = sSensorList,
        .listSize = 1,  // ARRAY_SIZE(sSensorList),
    },
    {
        .ctlName    = "/dev/epl2182_pls",
        .ctlMode    = 0,
        .drvName    = "proximity",
        .drvMode    = 0,
        .sensorList = &sSensorList[1],
        .listSize = 2,  // ARRAY_SIZE(sSensorList),
    },
};

static shub_cfg_t sSensorCfg = {
    .cfgSize = ARRAY_SIZE(sSensorDrvCfg),
    .cfg     = sSensorDrvCfg,
};


/*
 * DO NOT CHANGE
 * */
pshub_cfg_t shubGetDrvConfig(void) {
    return &sSensorCfg;
}

const struct sensor_t * shubGetList(void) {
    return sSensorList;
}

int shubGetListSize(void) {
    return ARRAY_SIZE(sSensorList);
}
