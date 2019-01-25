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
 * V2.0
 */
 /*History
 *Date                  Modification                                 Reason
 *
 */

#include <utils/Log.h>
#include "sensor.h"
#include "af_ad5823.h"

#define ad5823_VCM_SLAVE_ADDR (0x18 >> 1)



static int ad5823_drv_create(struct af_drv_init_para *input_ptr, cmr_handle* sns_af_drv_handle) 
{
	uint32_t ret = AF_SUCCESS;
	
	CHECK_PTR(input_ptr);
	ret = af_drv_create(input_ptr,sns_af_drv_handle);
	
	if (ret != AF_SUCCESS) {
		ret = AF_FAIL;
	} else {
		ret = ad5823_drv_set_mode(*sns_af_drv_handle);
		if (ret != AF_SUCCESS)
			ret = AF_FAIL;
	}

	return ret;
}
static int ad5823_drv_delete(cmr_handle sns_af_drv_handle, void* param) 
{
	uint32_t ret = AF_SUCCESS;
	CHECK_PTR(sns_af_drv_handle);
	
	ret = af_drv_delete(sns_af_drv_handle,param);
	return ret;
}

/*==============================================================================
 * Description:
 * calculate vcm driver dac code and write to vcm driver;
 * you can change this function according to your spec if it's necessary
 * pos: dac code for vcm driver
 *============================================================================*/
static int ad5823_drv_set_pos(cmr_handle sns_af_drv_handle, uint16_t pos)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	CHECK_PTR(sns_af_drv_handle);
	struct sns_af_drv_cxt *af_drv_cxt = (struct sns_af_drv_cxt*)sns_af_drv_handle;
	
	uint8_t cmd_val[5] = { 0x00 };
	uint16_t slave_addr = ad5823_VCM_SLAVE_ADDR;
	uint16_t cmd_len = 0;
	uint16_t target_code=pos&0x3ff;
	uint16_t step_4bit = 0x09;

	SENSOR_PRINT("target_code=%d", target_code);

	//MSB
	cmd_val[0] = 0x04;
	cmd_val[1] = ((target_code>>8) & 0xff) |0x04;
	cmd_len = 2;
	ret_value = hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, slave_addr, (uint8_t *) & cmd_val[0], cmd_len);

	//LSB
	cmd_val[0] = 0x05;
	cmd_val[1] = target_code& 0xff;
	cmd_len = 2;
	ret_value = hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, slave_addr, (uint8_t *) & cmd_val[0], cmd_len);
	
	af_drv_cxt->current_pos = target_code;
	
	return ret_value;
}



static int ad5823_drv_ioctl(cmr_handle sns_af_drv_handle, enum sns_cmd cmd, void* param) 
{
	uint32_t ret_value = AF_SUCCESS;
	
	CHECK_PTR(sns_af_drv_handle);
	struct sns_af_drv_cxt *af_drv_cxt = (struct sns_af_drv_cxt*)sns_af_drv_handle;
	
	switch(cmd) {
		case CMD_SNS_AF_SET_BEST_MODE:
			break;
		case CMD_SNS_AF_GET_TEST_MODE:
			break;
		case CMD_SNS_AF_SET_TEST_MODE:
			break;
		default:
			break;
	}

	return ret_value;
}

struct sns_af_drv_entry ad5823_drv_entry =
{
		.motor_avdd_val = SENSOR_AVDD_2800MV,
		.default_work_mode = 2,
		.af_ops =
		{
			.create  = ad5823_drv_create,
			.delete  = ad5823_drv_delete,
			.set_pos = ad5823_drv_set_pos,
			.get_pos = NULL,
			.ioctl   = ad5823_drv_ioctl,
		},
};

/*==============================================================================
 * Description:
 * init vcm driver 
 * you can change this function acording your spec if it's necessary
 * mode:
 * 1: Direct Mode
 * 2: Dual Level Control Mode
 * 3: Linear Slope Cntrol Mode
 *============================================================================*/
static int ad5823_drv_set_mode(cmr_handle sns_af_drv_handle)
{
	
	uint8_t cmd_val[5] = { 0x00 };
	uint16_t slave_addr = 0;
	uint16_t cmd_len = 0;
	uint16_t mode = 0;
	uint32_t ret_value = SENSOR_SUCCESS;
	
	CHECK_PTR(sns_af_drv_handle);
	struct sns_af_drv_cxt *af_drv_cxt = (struct sns_af_drv_cxt*)sns_af_drv_handle;

	slave_addr = ad5823_VCM_SLAVE_ADDR;
	
	if(af_drv_cxt->af_work_mode) {
		mode = af_drv_cxt->af_work_mode;
	} else {
		mode = ad5823_drv_entry.default_work_mode;
	}
	CMR_LOGI("mode: %d",mode);

	//reset
	cmd_len = 2;
	cmd_val[0] = 0x01;
	cmd_val[1] = 0x01;
	ret_value = hw_Sensor_WriteI2C(af_drv_cxt->hw_handle,slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
	if(ret_value){
		CMR_LOGE("cmd[0]:0x%x,cmd[1]:0x%x write failed",cmd_val[0],cmd_val[1]);
	}

	switch (mode) {
	case 1:
		break;
	case 2:
	{
		//mode
		cmd_len = 2;
		cmd_val[0] = 0x02;
		cmd_val[1] = 0x00;
		ret_value = hw_Sensor_WriteI2C(af_drv_cxt->hw_handle,slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
		if(ret_value){
			CMR_LOGE("cmd[0]:0x%x,cmd[1]:0x%x write failed",cmd_val[0],cmd_val[1]);
		}

		//vcm_move_time
		cmd_val[0] = 0x03;
		cmd_val[1] = 0x80;
		ret_value = hw_Sensor_WriteI2C(af_drv_cxt->hw_handle,slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
		if(ret_value){
			CMR_LOGE("cmd[0]:0x%x,cmd[1]:0x%x write failed",cmd_val[0],cmd_val[1]);
		}
	}
		break;
	case 3:
		break;
	}
	
	return ret_value;
}

