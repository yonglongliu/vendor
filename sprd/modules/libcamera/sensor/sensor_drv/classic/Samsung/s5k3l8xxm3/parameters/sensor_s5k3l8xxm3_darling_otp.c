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

#define MODULE_NAME       		"darling"    //module vendor name
#define MODULE_ID_5k3l8xxm3_darling		0x0005    //xxx: sensor P/N;  yyy: module vendor
#define LSC_PARAM_QTY 240

struct otp_info_t {
	int flag;
	int MID;
	int Year;
	int Month;
	int Day;
	int LID;
	int VCMID;
	int DriverICID;
	int RGr_ratio;
	int BGr_ratio;
	int GbGr_ratio;
	int rg_ratio_typical;
	int bg_ratio_typical;
	int r_typical;
	int g_typical;
	int b_typical;
	int VCM_start;
	int VCM_end;
} otp_info;
static struct otp_info_t s_s5k3l8xxm3_darling_otp_info={0x00};

#define OTP_AUTO_LOAD_LSC_5k3l8xxm3_darling

#define RG_TYPICAL_5k3l8xxm3_darling    		0x0242
#define BG_TYPICAL_5k3l8xxm3_darling		    0x02a9
#define R_TYPICAL_5k3l8xxm3_darling		0x0000
#define G_TYPICAL_5k3l8xxm3_darling		0x0000
#define B_TYPICAL_5k3l8xxm3_darling		0x0000

#define RGr_ratio_Typical 512
#define BGr_ratio_Typical 512
#define GbGr_ratio_Typical 1024
//extern void Sensor_WriteReg(u16 addr, u32 para);
//extern unsigned char Sensor_ReadReg(u32 addr);

static uint32_t s5k3l8xxm3_darling_read_otp_info(SENSOR_HW_HANDLE handle, void *param_ptr)
{
	uint32_t rtn = SENSOR_SUCCESS;
	struct otp_info_t *otp_info = (struct otp_info_t *)param_ptr;
	uint32_t PageCount = 0x0f00;
	uint32_t awb_flag1 = 0x00;
	uint32_t awb_flag2 = 0x00;

	otp_info->rg_ratio_typical = RG_TYPICAL_5k3l8xxm3_darling;
	otp_info->bg_ratio_typical = BG_TYPICAL_5k3l8xxm3_darling;
	otp_info->r_typical = R_TYPICAL_5k3l8xxm3_darling;
	otp_info->g_typical = G_TYPICAL_5k3l8xxm3_darling;
	otp_info->b_typical = B_TYPICAL_5k3l8xxm3_darling;
	
	Sensor_WriteReg(0x0A02,0x01);
	Sensor_WriteReg(0x0A00,0x01);
	usleep(100*5);
	unsigned char val = 0;
	val=Sensor_ReadReg(0x0A04);//flag of info and awb
	if(val == 0x01){
		otp_info->flag = 0x01;
		otp_info->MID = Sensor_ReadReg(0x0A06);
		otp_info->Year= Sensor_ReadReg(0x0A07);
		otp_info->Month= Sensor_ReadReg(0x0A08);
		otp_info->Day= Sensor_ReadReg(0x0A09);
		otp_info->LID = Sensor_ReadReg(0x0A0A);
		otp_info->VCMID = Sensor_ReadReg(0x0A0B);
		otp_info->DriverICID = Sensor_ReadReg(0x0A0C);
		otp_info->RGr_ratio =
		(Sensor_ReadReg(0x0A0E)<<8)+Sensor_ReadReg(0x0A0F);
		otp_info->BGr_ratio =
		(Sensor_ReadReg(0x0A10)<<8)+Sensor_ReadReg(0x0A11);
		otp_info->GbGr_ratio =
		(Sensor_ReadReg(0x0A12)<<8)+Sensor_ReadReg(0x0A13);
	}
	else if(val == 0x00){
		otp_info->flag=0x00;
		otp_info->MID =0x00;
		otp_info->Year=0x00;
		otp_info->Month=0x00;
		otp_info->Day=0x00;
		otp_info->LID = 0x00;
		otp_info->RGr_ratio = 0x00;
		otp_info->BGr_ratio = 0x00;
		otp_info->GbGr_ratio = 0x00;
	}
	val=Sensor_ReadReg(0x0A24);//flag of info and awb
	if(val == 0x01){
		otp_info->flag = 0x01;
		otp_info->MID = Sensor_ReadReg(0x0A26);
		otp_info->Year= Sensor_ReadReg(0x0A27);
		otp_info->Month= Sensor_ReadReg(0x0A28);
		otp_info->Day= Sensor_ReadReg(0x0A29);
		otp_info->LID = Sensor_ReadReg(0x0A2A);
		otp_info->VCMID = Sensor_ReadReg(0x0A2B);
		otp_info->DriverICID = Sensor_ReadReg(0x0A2C);
		otp_info->RGr_ratio =
		(Sensor_ReadReg(0x0A2E)<<8)+Sensor_ReadReg(0x0A2F);
		otp_info->BGr_ratio =
		(Sensor_ReadReg(0x0A30)<<8)+Sensor_ReadReg(0x0A31);
		otp_info->GbGr_ratio =
		(Sensor_ReadReg(0x0A32)<<8)+Sensor_ReadReg(0x0A33);
	}
	val=Sensor_ReadReg(0x0A14);//falg of VCM
	if(val == 0x01){
		otp_info->flag += 0x04;
		otp_info->VCM_start =
		(Sensor_ReadReg(0x0A18)<<8)+Sensor_ReadReg(0x0A19);
		otp_info->VCM_end =
		(Sensor_ReadReg(0x0A16)<<8)+Sensor_ReadReg(0x0A17);
	}
	else if(val == 0x00){
		otp_info->flag += 0x00;
		otp_info->VCM_start = 0x00;
		otp_info->VCM_end = 0x00;
	}
	val=Sensor_ReadReg(0x0A34);//falg of VCM
	if(val == 0x01){
		otp_info->flag += 0x04;
		otp_info->VCM_start =
		(Sensor_ReadReg(0x0A38)<<8)+Sensor_ReadReg(0x0A39);
		otp_info->VCM_end =
		(Sensor_ReadReg(0x0A36)<<8)+Sensor_ReadReg(0x0A37);
	}
	Sensor_WriteReg(0x0A00,0x00);
	return rtn;
}

static uint32_t s5k3l8xxm3_darling_update_awb(SENSOR_HW_HANDLE handle, void *param_ptr)
{
	uint32_t rtn = SENSOR_SUCCESS;
	struct otp_info_t *otp_info=(struct otp_info_t *)param_ptr;
	uint32_t g_gain=0,r_gain=0,b_gain = 0,g_gain_b=0,g_gain_r=0;

	int R_gain,B_gain,Gb_gain,Gr_gain,Base_gain;
	
	if(((otp_info->flag)&0x03) != 0x01) 
		return rtn;
	
	R_gain = (RGr_ratio_Typical*1000) / otp_info->RGr_ratio;
	B_gain = (BGr_ratio_Typical*1000) / otp_info->BGr_ratio;
	Gb_gain = (GbGr_ratio_Typical*1000) / otp_info->GbGr_ratio;
	Gr_gain = 1000;
	Base_gain = R_gain;
	if(Base_gain>B_gain)
		Base_gain=B_gain;
	if(Base_gain>Gb_gain) 
		Base_gain=Gb_gain;
	if(Base_gain>Gr_gain) 
		Base_gain=Gr_gain;
	R_gain = 0x100 * R_gain / Base_gain;
	B_gain = 0x100 * B_gain / Base_gain;
	Gb_gain = 0x100 * Gb_gain / Base_gain;
	Gr_gain = 0x100 * Gr_gain / Base_gain;
	if(Gr_gain>0x100){
		Sensor_WriteReg(0x020E,Gr_gain>>8);
		Sensor_WriteReg(0x020F,Gr_gain&0xff);
	}
	if(R_gain>0x100){
		Sensor_WriteReg(0x0210,R_gain>>8);
		Sensor_WriteReg(0x0211,R_gain&0xff);
	}
	if(B_gain>0x100){
		Sensor_WriteReg(0x0212,B_gain>>8);
		Sensor_WriteReg(0x0213,B_gain&0xff);
	}
	if(Gb_gain>0x100){
		Sensor_WriteReg(0x0214,Gb_gain>>8);
		Sensor_WriteReg(0x0215,Gb_gain&0xff);
	}
	return rtn;
}
/*After Power up
s5k3l8xxm3_darling_read_OTP(&s5k3l8xxm3_darling_OTP);
After Sensor Initial
s5k3l8xxm3_darling_apply_OTP(&s5k3l8xxm3_darling_OTP);
*/
#ifndef OTP_AUTO_LOAD_LSC_5k3l8xxm3_darling
static uint32_t s5k3l8xxm3_darling_update_lsc(SENSOR_HW_HANDLE handle, void *param_ptr)
{
	uint32_t rtn = SENSOR_SUCCESS;
	struct otp_info_t *otp_info=(struct otp_info_t *)param_ptr;

	/*TODO*/

	return rtn;
}

#endif
static uint32_t s5k3l8xxm3_darling_update_otp(SENSOR_HW_HANDLE handle, void *param_ptr)
{
	uint32_t rtn = SENSOR_SUCCESS;
	struct otp_info_t *otp_info=(struct otp_info_t *)param_ptr;

	rtn=s5k3l8xxm3_darling_update_awb(handle, param_ptr);
	if(rtn!=SENSOR_SUCCESS)
	{
		SENSOR_LOGI("OTP awb appliy error!");
		return rtn;
	}

#ifndef OTP_AUTO_LOAD_LSC_5k3l8xxm3_darling

	rtn=s5k3l8xxm3_darling_update_lsc(handle, param_ptr);
	if(rtn!=SENSOR_SUCCESS)
	{
		SENSOR_LOGI("OTP lsc appliy error!");
		return rtn;
	}
#endif
	SENSOR_LOGI("OTP appliy success!");

	return rtn;
}


static uint32_t s5k3l8xxm3_darling_identify_otp(SENSOR_HW_HANDLE handle, void *param_ptr)
{
	uint32_t rtn = SENSOR_SUCCESS;

	rtn=s5k3l8xxm3_darling_read_otp_info(handle, param_ptr);
	SENSOR_LOGI("rtn=%d",rtn);

	return rtn;
}
static struct raw_param_info_tab s_5k3l8xxm3_darling_raw_param_tab[] = 
	{MODULE_ID_5k3l8xxm3_darling, &s_s5k3l8xxm3_mipi_raw_info, s5k3l8xxm3_darling_identify_otp, s5k3l8xxm3_darling_update_otp};

static struct otp_info_t *s_s5k3l8xxm3_otp_info_ptr=&s_s5k3l8xxm3_darling_otp_info;
static struct raw_param_info_tab *s_s5k3l8xxm3_raw_param_tab_ptr=&s_5k3l8xxm3_darling_raw_param_tab;  /*otp function interface*/

