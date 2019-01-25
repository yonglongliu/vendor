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

#ifndef SENSORIOCTLSYNC_H_
#define SENSORIOCTLSYNC_H_

#define LIS3DH_ACC_IOCTL_BASE 77
#define ELAN_IOCTL_MAGIC 0x1C


#define ACCELEROMETER_IOCTL_BASE    LIS3DH_ACC_IOCTL_BASE
#define LIGHT_IOCTL_BASE            ELAN_IOCTL_MAGIC
#define PROXIMITY_IOCTL_BASE        ELAN_IOCTL_MAGIC

#define SET_DELAY   0
#define GET_DELAY   1
#define SET_ENABLE  2
#define GET_ENABLE  3

/** The following define the IOCTL command values via the ioctl macros */
#define ACCELEROMETER_IOCTL_SET_DELAY      _IOW(ACCELEROMETER_IOCTL_BASE, SET_DELAY, int)
#define ACCELEROMETER_IOCTL_GET_DELAY      _IOR(ACCELEROMETER_IOCTL_BASE, GET_DELAY, int)
#define ACCELEROMETER_IOCTL_SET_ENABLE     _IOW(ACCELEROMETER_IOCTL_BASE, SET_ENABLE, int)
#define ACCELEROMETER_IOCTL_GET_ENABLE     _IOR(ACCELEROMETER_IOCTL_BASE, GET_ENABLE, int)

#define LIGHT_IOCTL_SET_DELAY   _IOW(LIGHT_IOCTL_BASE, SET_DELAY, int)
#define LIGHT_IOCTL_GET_DELAY   _IOR(LIGHT_IOCTL_BASE, GET_DELAY, int)
#define LIGHT_IOCTL_SET_ENABLE  _IOW(LIGHT_IOCTL_BASE, 4/* SET_ENABLE */, int)
#define LIGHT_IOCTL_GET_ENABLE  _IOR(LIGHT_IOCTL_BASE, GET_ENABLE, int)

#define PROXIMITY_IOCTL_SET_DELAY   _IOW(PROXIMITY_IOCTL_BASE, SET_DELAY, int)
#define PROXIMITY_IOCTL_GET_DELAY   _IOR(PROXIMITY_IOCTL_BASE, GET_DELAY, int)
#define PROXIMITY_IOCTL_SET_ENABLE  _IOW(PROXIMITY_IOCTL_BASE, 3/* SET_ENABLE */, int)
#define PROXIMITY_IOCTL_GET_ENABLE  _IOR(PROXIMITY_IOCTL_BASE, GET_ENABLE, int)

#endif  // SENSORIOCTLSYNC_H_
