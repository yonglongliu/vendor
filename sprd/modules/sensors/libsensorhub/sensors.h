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

#ifndef SENSORS_H_
#define SENSORS_H_

#include <stdint.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include <linux/input.h>

#include <hardware/hardware.h>
#include <hardware/sensors.h>

#include <sensors/SensorLocalLogdef.h>
#include <sensors/SensorHandleSync.h>
#include "SensorIoCtlSync.h"
#include "SensorCmdSync.h"

// #define SUPPORT_INPUT_SENSOR
#define EVENT_DYNAMIC_ADAPT

__BEGIN_DECLS
struct iio_event {
    __u8 data[MAXI_CWM_MSG_SIZE];
};

#ifdef EVENT_DYNAMIC_ADAPT
typedef char pseudo_event;
#elif defined SUPPORT_INPUT_SENSOR
typedef struct input_event pseudo_event;
#else
typedef struct iio_event pseudo_event;
#endif /* SUPPORT_INPUT_SENSOR */
/*****************************************************************************/

#define DELAY_OUT_TIME                  0x7FFFFFFF

#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX 108
#endif

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#define ACC_UNIT_CONVERSION(value) ((value) * GRAVITY_EARTH / (1024.0f))

/*****************************************************************************/



#define SENSOR_STATE_MASK           (0x7FFF)

/*****************************************************************************/
/*****************************************************************************/

#define FREEMOTIOND_EVENT_X        ABS_X
#define FREEMOTIOND_EVENT_Y        ABS_Y
#define FREEMOTIOND_EVENT_Z        ABS_Z

#define EVENT_TYPE_YAW              REL_RX
#define EVENT_TYPE_PITCH            REL_RY
#define EVENT_TYPE_ROLL             REL_RZ
#define EVENT_TYPE_ORIENT_STATUS    REL_WHEEL

#define EVENT_TYPE_PRESSURE         REL_HWHEEL
#define EVENT_TYPE_TEMPERATURE      ABS_MISC


#define EVENT_TYPE_ROT_X            ABS_X
#define EVENT_TYPE_ROT_Y            ABS_Y
#define EVENT_TYPE_ROT_Z            ABS_Z

#define EVENT_TYPE_PROXIMITY        ABS_DISTANCE
#define EVENT_TYPE_LIGHT            REL_MISC


__END_DECLS

#endif  // SENSORS_H_
