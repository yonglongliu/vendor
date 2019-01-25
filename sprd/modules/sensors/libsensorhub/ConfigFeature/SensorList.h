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

#ifndef CONFIGFEATURE_SENSORLIST_H_
#define CONFIGFEATURE_SENSORLIST_H_

__BEGIN_DECLS

/*
 * The SENSORS Module
 */
#define SENSOR_VERSION 1

/* the GP2A is a binary proximity sensor that triggers around 5 cm on
 * this hardware */
#define PROXIMITY_THRESHOLD_GP2A  5.0f
#define LIGHT_SENSOR_POLLTIME     2000000000

#define CONVERT_A        0.01f
#define CONVERT_M        0.01f
#define CONVERT_GYRO     0.01f
#define CONVERT_PS       1.0f
#define CONVERT_ALL      0.01f
#define CONVERT_PRESSURE 100
#define CONVERT_RV       10000

#define CONVERT_1       1.0f
#define CONVERT_10      0.1f
#define CONVERT_100     0.01f
#define CONVERT_1000    0.001f
#define CONVERT_10000   0.0001f

// 1000 LSG = 1G
#define LSG                         (1000.0f)

#define RANGE_PRESS                 (100000.0f)
// data from freeMotionD is already in android units
#define RANGE_A                     (8*GRAVITY_EARTH)
#define RESOLUTION_A                (GRAVITY_EARTH / LSG)

// data from freeMotionD is already in android units
#define RANGE_M                     (500.0f)  // +/-2.5G = +/- 250uT = 500uT range
#define RESOLUTION_M                (0.152f)  // 0.152uT/LSB

/* conversion of orientation data to degree units */
#define CONVERT_O                   (0.1f)
#define CONVERT_O_A                 (CONVERT_O)
#define CONVERT_O_P                 (CONVERT_O)
#define CONVERT_O_R                 (-CONVERT_O)

// data from freeMotionD is already in android units
#define RANGE_GYRO                  (2000.0f*(float)M_PI/180.0f)
#define RESOLUTION_GYRO             (0.000000001f)

#define CONVERT_ROT_XYZ             (1.0f)

#define CONVERT_TEMP                (1.0f/10.0f)
#define RANGE_T                     (85.0f)
#define RESOLUTION_T                (0.1f)

// conversion of pressure data to mb units
#define RANGE_P                     (1100.0f)
#define RESOLUTION_P                (1.0f)
/* Press sensor is in Pa, Android wants mb */
#define CONVERT_PRESS               (1.0f/100.0f)

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

/*
 * Config Feature
 * */
/*
SENSORHUB_WITH_ACCELEROMETER
SENSORHUB_WITH_GYROSCOPE
SENSORHUB_WITH_LIGHT
SENSORHUB_WITH_MAGNETIC
SENSORHUB_WITH_PROXIMITY
SENSORHUB_WITH_PRESSURE
*/

#if ((defined SENSORHUB_WITH_ACCELEROMETER) && (defined SENSORHUB_WITH_GYROSCOPE))
#define SENSORHUB_WITH_ACC_AND_GYRO
#endif

#if ((defined SENSORHUB_WITH_ACCELEROMETER) && (defined SENSORHUB_WITH_MAGNETIC))
#define SENSORHUB_WITH_ACC_AND_MAG
#endif

#if ((defined SENSORHUB_WITH_ACCELEROMETER) && (defined SENSORHUB_WITH_GYROSCOPE) && (defined SENSORHUB_WITH_MAGNETIC))
#define SENSORHUB_WITH_ACC_AND_GYRO_AND_MAG
#endif

#if ((defined SENSORHUB_WITH_ACCELEROMETER) && (defined SENSORHUB_WITH_PROXIMITY))
#define SENSORHUB_WITH_ACC_AND_PROX
#endif

#if ((defined SENSORHUB_WITH_ACCELEROMETER) && (defined SENSORHUB_WITH_PROXIMITY) && (defined SENSORHUB_WITH_LIGHT))
#define SENSORHUB_WITH_ACC_AND_PROX_AND_LIG
#endif

__END_DECLS

#endif  // CONFIGFEATURE_SENSORLIST_H_
