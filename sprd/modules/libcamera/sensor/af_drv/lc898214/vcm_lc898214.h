/*
 * Copyright (C) 2012 The Android Open Source Project
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
#include <utils/Log.h>
#include "sensor_drv_u.h"
#include "sns_af_drv.h"

#define LOG_TAG "af_lc898214"

#define LC898214_VCM_SLAVE_ADDR (0xE4 >> 1)
#define SENSOR_SUCCESS           0
#define POSE_UP_HORIZONTAL       32
#define POSE_DOWN_HORIZONTAL     37

static int _lc898214_drv_init(cmr_handle sns_af_drv_handle);
static int lc898214_drv_create(struct af_drv_init_para *input_ptr, cmr_handle* sns_af_drv_handle);
static int lc898214_drv_delete(cmr_handle sns_af_drv_handle, void* param);
static int lc898214_drv_set_pos(cmr_handle sns_af_drv_handle, uint32_t pos);
static int lc898214_drv_ioctl(cmr_handle sns_af_drv_handle, enum sns_cmd cmd, void* param);

struct lc898214_pri_data 
{
	int16_t Hall_Max;
	int16_t Hall_Min; 
	int16_t HallValue;
	int16_t rCode;
};


