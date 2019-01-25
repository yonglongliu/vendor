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
#include "sensor.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"

#define OTP_AUTO_LOAD_LSC_s5k4h8yx_ofilm

#define RG_TYPICAL_s5k4h8yx_ofilm    		0x0242
#define BG_TYPICAL_s5k4h8yx_ofilm		    0x02a9
#define R_TYPICAL_s5k4h8yx_ofilm		0x0000
#define G_TYPICAL_s5k4h8yx_ofilm		0x0000
#define B_TYPICAL_s5k4h8yx_ofilm		0x0000

static uint32_t s5k4h8yx_ofilm_read_otp_info(SENSOR_HW_HANDLE handle, void *param_ptr)
{
	uint32_t rtn = SENSOR_SUCCESS;
	struct otp_info_t *otp_info = (struct otp_info_t *)param_ptr;
	uint32_t PageCount = 0x0f00;
	uint32_t awb_flag1 = 0x00;
	uint32_t awb_flag2 = 0x00;

	otp_info->rg_ratio_typical = RG_TYPICAL_s5k4h8yx_ofilm;
	otp_info->bg_ratio_typical = BG_TYPICAL_s5k4h8yx_ofilm;
	otp_info->r_typical = R_TYPICAL_s5k4h8yx_ofilm;
	otp_info->g_typical = G_TYPICAL_s5k4h8yx_ofilm;
	otp_info->b_typical = B_TYPICAL_s5k4h8yx_ofilm;

	/*TODO*/

	Sensor_WriteReg(0x0100, (0x01<<8)| (Sensor_ReadReg(0x0100) & 0x00ff));
	usleep(10 * 1000);

	Sensor_WriteReg(0x0A02, PageCount); //page set
	Sensor_WriteReg(0x0A00, 0x0100); //otp enable read
	usleep(2 * 1000);

	awb_flag1 = (Sensor_ReadReg(0x0A04)>>8);
	SENSOR_LOGI("awb_flag1 =%d", awb_flag1);
	awb_flag2 = (Sensor_ReadReg(0x0A20)>>8);
	SENSOR_LOGI("awb_flag2 =%d", awb_flag2);

	if (0x1 == awb_flag1) {
		SENSOR_LOGI("otp is in group 1");
		otp_info->flag = awb_flag1;
		otp_info->module_id=(Sensor_ReadReg(0x0a05)>>8);
		otp_info->year=(Sensor_ReadReg(0x0a06)>>8);
		otp_info->month=(Sensor_ReadReg(0x0a07)>>8);
		otp_info->day=(Sensor_ReadReg(0x0a08)>>8);
		otp_info->lens_id = (Sensor_ReadReg(0x0a09)>>8);
		otp_info->vcm_id= (Sensor_ReadReg(0x0a0a)>>8);
		otp_info->vcm_driver_id= (Sensor_ReadReg(0x0a0b)>>8);
		otp_info->rg_ratio_current = (Sensor_ReadReg(0x0a0d)&0x03ff);
		otp_info->bg_ratio_current = (Sensor_ReadReg(0x0a0f)&0x03ff);
	} else if (0x1 == awb_flag2) {
		SENSOR_LOGI("otp is in group 2");
		otp_info->flag = awb_flag2;
		otp_info->module_id=(Sensor_ReadReg(0x0a21)>>8);
		otp_info->year=(Sensor_ReadReg(0x0a22)>>8);
		otp_info->month=(Sensor_ReadReg(0x0a23)>>8);
		otp_info->day=(Sensor_ReadReg(0x0a24)>>8);
		otp_info->lens_id = (Sensor_ReadReg(0x0a25)>>8);
		otp_info->vcm_id= (Sensor_ReadReg(0x0a26)>>8);
		otp_info->vcm_driver_id= (Sensor_ReadReg(0x0a27)>>8);

		otp_info->rg_ratio_current = (Sensor_ReadReg(0x0a29)&0x03ff);
		otp_info->bg_ratio_current = (Sensor_ReadReg(0x0a2b)&0x03ff);
	} else {
		SENSOR_LOGI("otp all value is wrong");
		rtn=SENSOR_FAIL;
	}
	Sensor_WriteReg(0x0a00, 0x00); //otp disable read

	SENSOR_LOGI("flag=0x%x",otp_info->flag);
	SENSOR_LOGI("module_id=0x%x",otp_info->module_id);
	SENSOR_LOGI("lens_id=0x%x",otp_info->lens_id);
	SENSOR_LOGI("vcm_id=0x%x",otp_info->vcm_id);
	SENSOR_LOGI("vcm_driver_id=0x%x",otp_info->vcm_driver_id);
	SENSOR_LOGI("date=%d-%d-%d",otp_info->year,otp_info->month,otp_info->day);
	SENSOR_LOGI("rg_ratio_current=0x%x",otp_info->rg_ratio_current);
	SENSOR_LOGI("bg_ratio_current=0x%x",otp_info->bg_ratio_current);
	SENSOR_LOGI("rg_ratio_typical=0x%x",otp_info->rg_ratio_typical);
	SENSOR_LOGI("bg_ratio_typical=0x%x",otp_info->bg_ratio_typical);
	SENSOR_LOGI("r_current=0x%x",otp_info->r_current);
	SENSOR_LOGI("g_current=0x%x",otp_info->g_current);
	SENSOR_LOGI("b_current=0x%x",otp_info->b_current);
	SENSOR_LOGI("r_typical=0x%x",otp_info->r_typical);
	SENSOR_LOGI("g_typical=0x%x",otp_info->g_typical);
	SENSOR_LOGI("b_typical=0x%x",otp_info->b_typical);
	SENSOR_LOGI("vcm_dac_start=0x%x",otp_info->vcm_dac_start);
	SENSOR_LOGI("vcm_dac_inifity=0x%x",otp_info->vcm_dac_inifity);
	SENSOR_LOGI("vcm_dac_macro=0x%x",otp_info->vcm_dac_macro);

	return rtn;
}

static uint32_t s5k4h8yx_ofilm_update_awb(SENSOR_HW_HANDLE handle, void *param_ptr)
{
	uint32_t rtn = SENSOR_SUCCESS;
	struct otp_info_t *otp_info=(struct otp_info_t *)param_ptr;
	uint32_t g_gain=0,r_gain=0,b_gain = 0,g_gain_b=0,g_gain_r=0;

	//calculate R,G,B gain
	if (otp_info->bg_ratio_current < otp_info->bg_ratio_typical) {
		if (otp_info->rg_ratio_current< otp_info->rg_ratio_typical) {
			g_gain= 0x100;
			b_gain = 0x100 * otp_info->bg_ratio_typical / otp_info->bg_ratio_current;
			r_gain = 0x100 * otp_info->rg_ratio_typical / otp_info->rg_ratio_current;
		} else {
			r_gain = 0x100;
			g_gain = 0x100 * otp_info->rg_ratio_current / otp_info->rg_ratio_typical;
			b_gain = g_gain * otp_info->bg_ratio_typical / otp_info->bg_ratio_current;
		}
	} else {
		if (otp_info->rg_ratio_current < otp_info->rg_ratio_typical)	{
			b_gain = 0x100;
			g_gain = 0x100 * otp_info->bg_ratio_current / otp_info->bg_ratio_typical;
			r_gain = g_gain * otp_info->rg_ratio_typical / otp_info->rg_ratio_current;
		} else {
			g_gain_b = 0x100*otp_info->bg_ratio_current / otp_info->bg_ratio_typical;
			g_gain_r = 0x100*otp_info->rg_ratio_current / otp_info->rg_ratio_typical;

			if (g_gain_b > g_gain_r) {
				b_gain = 0x100;
				g_gain = g_gain_b;
				r_gain = g_gain * otp_info->rg_ratio_typical / otp_info->rg_ratio_current;
			} else {
				r_gain = 0x100;
				g_gain = g_gain_r;
				b_gain= g_gain * otp_info->bg_ratio_typical / otp_info->bg_ratio_current;
			}
		}
	}

	//write to register
	SENSOR_LOGI("r_Gain=0x%x\n", r_gain);
	SENSOR_LOGI("g_Gain=0x%x\n", g_gain);
	SENSOR_LOGI("b_Gain=0x%x\n", b_gain);

	Sensor_WriteReg(0x3058, 0x0100);
	rtn=Sensor_WriteReg(0x020e, g_gain&0xffff);
	rtn=Sensor_WriteReg(0x0210, r_gain&0xffff);
	rtn=Sensor_WriteReg(0x0212, b_gain&0xffff);
	rtn=Sensor_WriteReg(0x0214, g_gain&0xffff);

	return rtn;
}
#ifndef OTP_AUTO_LOAD_LSC_s5k4h8yx_ofilm

static uint32_t s5k4h8yx_ofilm_update_lsc(SENSOR_HW_HANDLE handle, void *param_ptr)
{
	uint32_t rtn = SENSOR_SUCCESS;
	struct otp_info_t *otp_info=(struct otp_info_t *)param_ptr;

	/*TODO*/

	return rtn;
}

#endif
static uint32_t s5k4h8yx_ofilm_update_otp(SENSOR_HW_HANDLE handle, void *param_ptr)
{
	uint32_t rtn = SENSOR_SUCCESS;
	struct otp_info_t *otp_info=(struct otp_info_t *)param_ptr;

	rtn=s5k4h8yx_ofilm_update_awb(handle, param_ptr);
	if(rtn!=SENSOR_SUCCESS)
	{
		SENSOR_LOGI("OTP awb appliy error!");
		return rtn;
	}

	#ifndef OTP_AUTO_LOAD_LSC_s5k4h8yx_ofilm

	rtn=s5k4h8yx_ofilm_update_lsc(handle, param_ptr);
	if(rtn!=SENSOR_SUCCESS)
	{
		SENSOR_LOGI("OTP lsc appliy error!");
		return rtn;
	}
	#endif

	return rtn;
}


static uint32_t s5k4h8yx_ofilm_identify_otp(SENSOR_HW_HANDLE handle, void *param_ptr)
{
	uint32_t rtn = SENSOR_SUCCESS;

	rtn=s5k4h8yx_ofilm_read_otp_info(handle, param_ptr);
	SENSOR_LOGI("rtn=%d",rtn);

	return rtn;
}
