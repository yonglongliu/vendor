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
#ifdef SENSORHUB_WITH_ACCELEROMETER
        {.name =       "Accelerometer Sensor",
         .vendor =     "Sprd Group Ltd.",
         .version =    SENSOR_VERSION,
         .handle =     SENSOR_HANDLE_ACCELEROMETER,
         .type =       SENSOR_TYPE_ACCELEROMETER,
         .maxRange =   RANGE_A,
         .resolution = CONVERT_A,
         .power =      0.17f,
         .minDelay =   10000,
         .fifoReservedEventCount = 0,
         .fifoMaxEventCount =   1220,
         .stringType =         0,
         .requiredPermission = 0,
         .maxDelay =      200000,
         .flags = SENSOR_FLAG_CONTINUOUS_MODE,
         .reserved =          {}
        },
        {.name =       "Accelerometer Sensor (WAKE_UP)",
         .vendor =     "Sprd Group Ltd.",
         .version =    SENSOR_VERSION,
         .handle =     SENSOR_HANDLE_WAKE_UP_ACCELEROMETER,
         .type =       SENSOR_TYPE_ACCELEROMETER,
         .maxRange =   RANGE_A,
         .resolution = CONVERT_A,
         .power =      0.17f,
         .minDelay =   10000,
         .fifoReservedEventCount = 0,
         .fifoMaxEventCount =   200,
         .stringType =         0,
         .requiredPermission = 0,
         .maxDelay =      200000,
         .flags = SENSOR_FLAG_CONTINUOUS_MODE | SENSOR_FLAG_WAKE_UP,
         .reserved =          {}
        },
#endif  /* SENSORHUB_WITH_ACCELEROMETER */
#ifdef SENSORHUB_WITH_MAGNETIC
        {.name =       "Magnetic field Sensor",
         .vendor =     "Sprd Group Ltd.",
         .version =    SENSOR_VERSION,
         .handle =     SENSOR_HANDLE_GEOMAGNETIC_FIELD,
         .type =       SENSOR_TYPE_MAGNETIC_FIELD,
         .maxRange =   200.0f,
         .resolution = CONVERT_M,
         .power =      5.0f,
         .minDelay =   10000,
         .fifoReservedEventCount = 0,
         .fifoMaxEventCount =   1220,
         .stringType =         0,
         .requiredPermission = 0,
         .maxDelay =      200000,
         .flags = SENSOR_FLAG_CONTINUOUS_MODE,
         .reserved =          {}
        },
        {.name =       "Magnetic field Sensor (WAKE_UP)",
         .vendor =     "Sprd Group Ltd.",
         .version =    SENSOR_VERSION,
         .handle =     SENSOR_HANDLE_WAKE_UP_GEOMAGNETIC_FIELD,
         .type =       SENSOR_TYPE_MAGNETIC_FIELD,
         .maxRange =   200.0f,
         .resolution = CONVERT_M,
         .power =      5.0f,
         .minDelay =   10000,
         .fifoReservedEventCount = 0,
         .fifoMaxEventCount =   200,
         .stringType =         0,
         .requiredPermission = 0,
         .maxDelay =      200000,
         .flags = SENSOR_FLAG_CONTINUOUS_MODE | SENSOR_FLAG_WAKE_UP,
         .reserved =          {}
        },
#endif  /* SENSORHUB_WITH_MAGNETIC */
#ifdef SENSORHUB_WITH_GYROSCOPE
        {.name =       "Gyroscope Sensor",
         .vendor =     "Sprd Group Ltd.",
         .version =    SENSOR_VERSION,
         .handle =     SENSOR_HANDLE_GYROSCOPE,
         .type =       SENSOR_TYPE_GYROSCOPE,
         .maxRange =   40.0f,
         .resolution = CONVERT_GYRO,
         .power =      6.1f,
         .minDelay =   10000,
         .fifoReservedEventCount = 0,
         .fifoMaxEventCount =   1220,
         .stringType =         0,
         .requiredPermission = 0,
         .maxDelay =      200000,
         .flags = SENSOR_FLAG_CONTINUOUS_MODE,
         .reserved =          {}
        },
        {.name =       "Gyroscope Sensor (WAKE_UP)",
         .vendor =     "Sprd Group Ltd.",
         .version =    SENSOR_VERSION,
         .handle =     SENSOR_HANDLE_WAKE_UP_GYROSCOPE,
         .type =       SENSOR_TYPE_GYROSCOPE,
         .maxRange =   2000.0f,
         .resolution = CONVERT_GYRO,
         .power =      6.1f,
         .minDelay =   10000,
         .fifoReservedEventCount = 0,
         .fifoMaxEventCount =   200,
         .stringType =         0,
         .requiredPermission = 0,
         .maxDelay =      200000,
         .flags = SENSOR_FLAG_CONTINUOUS_MODE | SENSOR_FLAG_WAKE_UP,
         .reserved =          {}
        },
#endif  /* SENSORHUB_WITH_GYROSCOPE */
#ifdef SENSORHUB_WITH_LIGHT
        {.name =       "Light Sensor",
         .vendor =     "Sprd Group Ltd.",
         .version =    SENSOR_VERSION,
         .handle =     SENSOR_HANDLE_LIGHT,
         .type =       SENSOR_TYPE_LIGHT,
         .maxRange =   10240.0f,
         .resolution = 1.0f,
         .power =      0.15f,
         .minDelay =   0,
         .fifoReservedEventCount = 0,
         .fifoMaxEventCount =      0,
         .stringType =         NULL,
         .requiredPermission = NULL,
         .maxDelay =           0,
         .flags = SENSOR_FLAG_ON_CHANGE_MODE,
         .reserved =          {}
        },
        {.name =       "Light Sensor (WAKE_UP)",
         .vendor =     "Sprd Group Ltd.",
         .version =    SENSOR_VERSION,
         .handle =     SENSOR_HANDLE_WAKE_UP_LIGHT,
         .type =       SENSOR_TYPE_LIGHT,
         .maxRange =   10240.0f,
         .resolution = 1.0f,
         .power =      0.15f,
         .minDelay =   0,
         .fifoReservedEventCount = 0,
         .fifoMaxEventCount =      0,
         .stringType =         NULL,
         .requiredPermission = NULL,
         .maxDelay =           0,
         .flags = SENSOR_FLAG_ON_CHANGE_MODE| SENSOR_FLAG_WAKE_UP,
         .reserved =          {}
        },
#endif  /* SENSORHUB_WITH_LIGHT */
#ifdef SENSORHUB_WITH_PROXIMITY
        {.name =       "Proximity Sensor",
         .vendor =     "Sprd Group Ltd.",
         .version =    SENSOR_VERSION,
         .handle =     SENSOR_HANDLE_WAKE_UP_PROXIMITY,
         .type =       SENSOR_TYPE_PROXIMITY,
         .maxRange =   5.0f,
         .resolution = 1.0f,
         .power =      0.15f,
         .minDelay =   0,
         .fifoReservedEventCount = 0,
         .fifoMaxEventCount =      0,
         .stringType =         NULL,
         .requiredPermission = NULL,
         .maxDelay =           0,
         .flags = SENSOR_FLAG_ON_CHANGE_MODE | SENSOR_FLAG_WAKE_UP,
         .reserved =          {}
        },
#endif  /* SENSORHUB_WITH_PROXIMITY */
#ifdef SENSORHUB_WITH_PRESSURE
        {.name =       "Pressure",
         .vendor =     "Sprd Group Ltd.",
         .version =    SENSOR_VERSION,
         .handle =     SENSOR_HANDLE_PRESSURE,
         .type =       SENSOR_TYPE_PRESSURE,
         .maxRange =   1.0f,
         .resolution = 1.0f,
         .power =      0.15f,
         .minDelay =   10000,
         .fifoReservedEventCount = 0,
         .fifoMaxEventCount =      0,
         .stringType =         NULL,
         .requiredPermission = NULL,
         .maxDelay =           0,
         .flags = SENSOR_FLAG_CONTINUOUS_MODE,
         .reserved =          {}
        },
        {.name =       "Pressure (WAKE_UP)",
         .vendor =     "Sprd Group Ltd.",
         .version =    SENSOR_VERSION,
         .handle =     SENSOR_HANDLE_WAKE_UP_PRESSURE,
         .type =       SENSOR_TYPE_PRESSURE,
         .maxRange =   1.0f,
         .resolution = 1.0f,
         .power =      0.15f,
         .minDelay =   10000,
         .fifoReservedEventCount = 0,
         .fifoMaxEventCount =      0,
         .stringType =         NULL,
         .requiredPermission = NULL,
         .maxDelay =           0,
         .flags = SENSOR_FLAG_CONTINUOUS_MODE | SENSOR_FLAG_WAKE_UP,
         .reserved =          {}
        },
#endif  /* SENSORHUB_WITH_PRESSURE */
};

static shub_drv_cfg_t sSensorDrvCfg[] = {
    {
        .ctlName    = "/sys/class/sprd_sensorhub/sensor_hub/",
        .ctlMode    = 1,
        .drvName    = "SPRDSensor",
        .drvMode    = 1,
        .sensorList = sSensorList,
        .listSize = ARRAY_SIZE(sSensorList),
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
