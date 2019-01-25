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
#ifndef _SNS_AF_DRV_H_
#define _SNS_AF_DRV_H_

#include <utils/Log.h>
#include "jpeg_exif_header.h"
#include "cmr_type.h"
#include "cmr_common.h"
#include "cmr_sensor_info.h"
#include "sensor_raw.h"
#include "sensor_drv_u.h"

#define VCM_UNIDENTIFY 0xFFFF

#define AF_SUCCESS          CMR_CAMERA_SUCCESS
#define AF_FAIL             CMR_CAMERA_FAIL

#ifndef CHECK_PTR
#define CHECK_PTR(expr)\
    if ((expr) == NULL) {\
        ALOGE("ERROR: NULL pointer detected " #expr);\
        return AF_FAIL;\
     }
#endif


struct af_drv_init_para
{
	SENSOR_HW_HANDLE hw_handle;
	uint32_t af_work_mode;
	/*you can add your param here*/
};

struct sns_af_drv_ops
{
	/*used in sensor_drv_u*/
	int (*create)(struct af_drv_init_para *input_ptr, cmr_handle* sns_af_drv_handle); /*create vcm handle*/
	int (*delete)(cmr_handle sns_af_drv_handle, void* param); /*delete vcm handle*/

	/*export to isp*/
	int (*set_pos)(cmr_handle sns_af_drv_handle, uint16_t pos); /*customer must realize*/
	int (*get_pos)(cmr_handle sns_af_drv_handle, uint16_t *pos); /*can be NULL*/
	int (*ioctl)(cmr_handle sns_af_drv_handle, enum sns_cmd cmd, void* param);
};

struct sns_af_drv_entry
{
	SENSOR_AVDD_VAL_E motor_avdd_val;  /*vcm working power*/
	uint32_t default_work_mode;
	struct sns_af_drv_ops af_ops;
};

struct sns_af_drv_cxt
{
	cmr_handle hw_handle; /*hardware handle*/
	uint32_t af_work_mode; /*Actual working mode*/
	uint32_t current_pos;  /*vcm steps last time*/
	void  *private;        /*vcm private data*/
};

struct sns_af_drv_cfg
{
	struct sns_af_drv_entry *af_drv_entry;
	uint32_t af_work_mode; /*config by user*/
};

int af_drv_create(struct af_drv_init_para *input_ptr, cmr_handle* sns_af_drv_handle);
int af_drv_delete(cmr_handle sns_af_drv_handle, void* param);
#endif
