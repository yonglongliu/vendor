/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __OIS_MAIN_H__
#define __OIS_MAIN_H__

#include "sensor_drv_u.h"

uint32_t ois_pre_open(SENSOR_HW_HANDLE handle);
uint32_t OpenOIS(SENSOR_HW_HANDLE handle);
uint32_t CloseOIS(SENSOR_HW_HANDLE handle);
uint32_t SetOisMode(SENSOR_HW_HANDLE handle, unsigned char mode);
unsigned char GetOisMode(SENSOR_HW_HANDLE handle);
unsigned short OisLensRead(SENSOR_HW_HANDLE handle, unsigned short cmd);
uint32_t OisLensWrite(SENSOR_HW_HANDLE handle, unsigned short cmd);
uint32_t OIS_write_af(SENSOR_HW_HANDLE handle, uint32_t param);
uint32_t Ois_get_pose_dis(SENSOR_HW_HANDLE handle, uint32_t *up2h,
                          uint32_t *h2down);

#endif // __OIS_MAIN_H__
