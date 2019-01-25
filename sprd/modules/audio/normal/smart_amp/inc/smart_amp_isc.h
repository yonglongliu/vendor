/*
* Copyright (C) 2010 The Android Open Source Project
* Copyright (C) 2012-2015, The Linux Foundation. All rights reserved.
*
* Not a Contribution, Apache license notifications and license are retained
* for attribution purposes only.
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

#ifndef __SMART_AMP_ISC_H__
#define __SMART_AMP_ISC_H__

typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
#ifndef AUDIO_SERVER_64_BIT
typedef unsigned long long uint64_t;
#endif

struct ISC_MMODE_MSPK_T;

int32_t init_ISC(struct ISC_MMODE_MSPK_T **isc_mmode_mspk_ptrptr, uint8_t *para_in, uint8_t *monitor_out[]);
int32_t config_para_ISC(struct ISC_MMODE_MSPK_T *isc_mmode_mspk_ptr, uint8_t *pModeName);
int32_t close_ISC(struct ISC_MMODE_MSPK_T *isc_mmode_mspk_ptr);
int32_t sigpath_proc_mono_ISC(struct ISC_MMODE_MSPK_T *isc_mmode_mspk_ptr, int32_t *input, int32_t *output);
int32_t input_battery_voltage_ISC(int32_t battery_voltage);
int32_t get_delta_battery_voltage_mv_ISC(int32_t *delta_batvol_ptr);

#endif
