#include <utils/Log.h>
#include "sensor.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"

//#define OTP_AUTO_LOAD_LSC_ov8856s_lightarray

#define RG_TYPICAL_ov8856s_lightarray     0x013F
#define BG_TYPICAL_ov8856s_lightarray	    0x012C
#define R_TYPICAL_ov8856s_lightarray		0x0000
#define G_TYPICAL_ov8856s_lightarray		0x0000
#define B_TYPICAL_ov8856s_lightarray		0x0000

static uint32_t ov8856s_lightarray_read_otp_info(SENSOR_HW_HANDLE handle,void *param_ptr)
{
	uint32_t rtn = SENSOR_SUCCESS;
	struct otp_info_t *otp_info=(struct otp_info_t *)param_ptr;
	otp_info->rg_ratio_typical=RG_TYPICAL_ov8856s_lightarray;
	otp_info->bg_ratio_typical=BG_TYPICAL_ov8856s_lightarray;
	otp_info->r_typical=R_TYPICAL_ov8856s_lightarray;
	otp_info->g_typical=G_TYPICAL_ov8856s_lightarray;
	otp_info->b_typical=B_TYPICAL_ov8856s_lightarray;

	/*TODO*/
	int otp_flag, addr, temp, i;
	//set 0x5001[3] to 
	int temp1;
    Sensor_WriteReg(0x0100, 0x01);
	temp1 = Sensor_ReadReg(0x5001);
	Sensor_WriteReg(0x5001, (0x00 & 0x08) | (temp1 & (~0x08)));
	// read OTP into buffer
	Sensor_WriteReg(0x3d84, 0xC0);
	Sensor_WriteReg(0x3d88, 0x70); // OTP start address
	Sensor_WriteReg(0x3d89, 0x10);
	Sensor_WriteReg(0x3d8A, 0x72); // OTP end address
	Sensor_WriteReg(0x3d8B, 0x0a);
	Sensor_WriteReg(0x3d81, 0x01); // load otp into buffer
	usleep(10 * 1000);
	
	// OTP base information and WB calibration data
	otp_flag = Sensor_ReadReg(0x7010);
	addr = 0;
	if((otp_flag & 0xc0) == 0x40) {
		addr = 0x7011; // base address of info group 1
	}
	else if((otp_flag & 0x30) == 0x10) {
		addr = 0x701a; // base address of info group 2,0x7019
	}
	
	if(addr != 0) {
		otp_info->flag = 0xC0; // valid info and AWB in OTP
		otp_info->module_id = Sensor_ReadReg(addr);
		otp_info->lens_id = Sensor_ReadReg( addr + 1);
		otp_info->year = Sensor_ReadReg( addr + 2);
		otp_info->month = Sensor_ReadReg( addr + 3);
		otp_info->day = Sensor_ReadReg(addr + 4);
		temp = Sensor_ReadReg(addr + 7);
		otp_info->rg_ratio_current= (Sensor_ReadReg(addr + 5)<<2) + ((temp>>6) & 0x03);
		otp_info->bg_ratio_current= (Sensor_ReadReg(addr + 6)<<2) + ((temp>>4) & 0x03);
	}
	else {
		otp_info->flag = 0x00; // not info and AWB in OTP
		otp_info->module_id = 0;
		otp_info->lens_id = 0;
		otp_info->year = 0;
		otp_info->month = 0;
		otp_info->day = 0;
		otp_info->rg_ratio_current = 0;
		otp_info->bg_ratio_current = 0;
	}

	// OTP VCM Calibration
	otp_flag = Sensor_ReadReg(0x7023); //0x7021
	addr = 0;
	if((otp_flag & 0xc0) == 0x40) {
		addr = 0x7022; // base address of VCM Calibration group 1,0x22
	}else if((otp_flag & 0x30) == 0x10) {
		addr = 0x7025; // base address of VCM Calibration group 2,0x7025
	}
	if(addr != 0) {
		otp_info->flag |= 0x20;
		temp = Sensor_ReadReg(addr + 2);
		otp_info->vcm_dac_start = (Sensor_ReadReg(addr)<<2) | ((temp>>6) & 0x03);
		otp_info->vcm_dac_inifity=otp_info->vcm_dac_start;
		otp_info->vcm_dac_macro = (Sensor_ReadReg(addr + 1) << 2) | ((temp>>4) & 0x03);
	}
	else {
		otp_info->vcm_dac_start = 0;
		otp_info->vcm_dac_inifity = 0;
		otp_info->vcm_dac_macro = 0;
	}
	
	// OTP Lenc Calibration
	otp_flag = Sensor_ReadReg(0x7028); 
	addr = 0;
	int checksum2=0;
	if((otp_flag & 0xc0) == 0x40) {
		addr = 0x7029; // base address of Lenc Calibration group 1,
	}
	else if((otp_flag & 0x30) == 0x10) {
		addr = 0x711a; // base address of Lenc Calibration group 2,
	}
	if(addr != 0) {
		for(i=0;i<240;i++) {
			otp_info->lsc_param[i]=Sensor_ReadReg(addr + i);
			checksum2 += otp_info->lsc_param[i];
		}
		checksum2 = (checksum2)%255 +1;
		
		SENSOR_PRINT("checksum2=0x%x, 0x%x ",checksum2,Sensor_ReadReg(addr + 240));
		if(Sensor_ReadReg((addr + 240)) == checksum2){
			otp_info->flag |= 0x10;
		}
	}
	else {
		for(i=0;i<240;i++) {
			otp_info->lsc_param[i]=0;
		}
	}
	
	for(i=0x7010;i<=0x720a;i++) {
		Sensor_WriteReg(i,0); // clear OTP buffer, recommended use continuous write to accelarate,0x720a
	}
	//set 0x5001[3] to "1"
	temp1 = Sensor_ReadReg(0x5001);
	Sensor_WriteReg(0x5001, (0x08 & 0x08) | (temp1 & (~0x08)));

    Sensor_WriteReg(0x0100, 0x00);

	/*print otp information*/
	SENSOR_PRINT("flag=0x%x",otp_info->flag);
	SENSOR_PRINT("module_id=0x%x",otp_info->module_id);
	SENSOR_PRINT("lens_id=0x%x",otp_info->lens_id);
	SENSOR_PRINT("vcm_id=0x%x",otp_info->vcm_id);
	SENSOR_PRINT("vcm_id=0x%x",otp_info->vcm_id);
	SENSOR_PRINT("vcm_driver_id=0x%x",otp_info->vcm_driver_id);
	SENSOR_PRINT("data=%d-%d-%d",otp_info->year,otp_info->month,otp_info->day);
	SENSOR_PRINT("rg_ratio_current=0x%x",otp_info->rg_ratio_current);
 	SENSOR_PRINT("bg_ratio_current=0x%x",otp_info->bg_ratio_current);
	SENSOR_PRINT("rg_ratio_typical=0x%x",otp_info->rg_ratio_typical);
	SENSOR_PRINT("bg_ratio_typical=0x%x",otp_info->bg_ratio_typical);
	SENSOR_PRINT("r_current=0x%x",otp_info->r_current);
	SENSOR_PRINT("g_current=0x%x",otp_info->g_current);
	SENSOR_PRINT("b_current=0x%x",otp_info->b_current);
	SENSOR_PRINT("r_typical=0x%x",otp_info->r_typical);
	SENSOR_PRINT("g_typical=0x%x",otp_info->g_typical);
	SENSOR_PRINT("b_typical=0x%x",otp_info->b_typical);
	SENSOR_PRINT("vcm_dac_start=0x%x",otp_info->vcm_dac_start);
	SENSOR_PRINT("vcm_dac_inifity=0x%x",otp_info->vcm_dac_inifity);
	SENSOR_PRINT("vcm_dac_macro=0x%x",otp_info->vcm_dac_macro);
	
	return rtn;
}


static uint32_t ov8856s_lightarray_update_awb(SENSOR_HW_HANDLE handle,void *param_ptr)
{
	uint32_t rtn = SENSOR_SUCCESS;
	struct otp_info_t *otp_info=(struct otp_info_t *)param_ptr;

	/*TODO*/
	int rg, bg, R_gain, G_gain, B_gain, Base_gain, temp, i;

	// apply OTP WB Calibration
	if (otp_info->flag & 0x40) {
		rg = otp_info->rg_ratio_current;
		bg = otp_info->bg_ratio_current;
	
	//calculate G gain
		R_gain = (otp_info->rg_ratio_typical*1000) / rg;
		B_gain = (otp_info->bg_ratio_typical*1000) / bg;
		G_gain = 1000;
	
		if (R_gain < 1000 || B_gain < 1000)
		{
		if (R_gain < B_gain)
			Base_gain = R_gain;
		else
			Base_gain = B_gain;
		}
		else
		{
			Base_gain = G_gain;
		}
		R_gain = 0x400 * R_gain / (Base_gain);
		B_gain = 0x400 * B_gain / (Base_gain);
		G_gain = 0x400 * G_gain / (Base_gain);

		SENSOR_PRINT("r_Gain=0x%x\n", R_gain);	
		SENSOR_PRINT("g_Gain=0x%x\n", G_gain);	
		SENSOR_PRINT("b_Gain=0x%x\n", B_gain);	
	
		// update sensor WB gain
		if (R_gain>0x400) {
			Sensor_WriteReg(0x5019, R_gain>>8);
			Sensor_WriteReg(0x501a, R_gain & 0x00ff);
		}
		if (G_gain>0x400) {
			Sensor_WriteReg(0x501b, G_gain>>8);
			Sensor_WriteReg(0x501c, G_gain & 0x00ff);
		}
		if (B_gain>0x400) {
			Sensor_WriteReg(0x501d, B_gain>>8);
			Sensor_WriteReg(0x501e, B_gain & 0x00ff);
		}
	}
	
	return rtn;
}

#ifndef OTP_AUTO_LOAD_LSC_ov8856s_lightarray

static uint32_t ov8856s_lightarray_update_lsc(SENSOR_HW_HANDLE handle,void *param_ptr)
{
	uint32_t rtn = SENSOR_SUCCESS;
	struct otp_info_t *otp_info=(struct otp_info_t *)param_ptr;

	/*TODO*/
	int i=0,temp=0;
	if (otp_info->flag & 0x10) {
		SENSOR_PRINT("apply otp lsc \n");	
		temp = Sensor_ReadReg(0x5000);
		temp = 0x20 | temp;
		Sensor_WriteReg(0x5000, temp);
		for(i=0;i<240;i++) {
			Sensor_WriteReg(0x5900 + i, otp_info->lsc_param[i]);
		}
	}
	
	return rtn;
}

#endif
static uint32_t ov8856s_lightarray_update_otp(SENSOR_HW_HANDLE handle,void *param_ptr)
{
	uint32_t rtn = SENSOR_SUCCESS;
	struct otp_info_t *otp_info=(struct otp_info_t *)param_ptr;

	rtn=ov8856s_lightarray_update_awb(handle,param_ptr);
	if(rtn!=SENSOR_SUCCESS)
	{
		SENSOR_PRINT_ERR("OTP awb appliy error!");
		return rtn;
	}

	#ifndef OTP_AUTO_LOAD_LSC_ov8856s_lightarray
	
	rtn=ov8856s_lightarray_update_lsc(handle,param_ptr);
	if(rtn!=SENSOR_SUCCESS)
	{
		SENSOR_PRINT_ERR("OTP lsc appliy error!");
		return rtn;
	}
	#endif
	
	return rtn;
}
static uint32_t ov8856s_lightarray_identify_otp(SENSOR_HW_HANDLE handle,void *param_ptr)
{
	uint32_t rtn = SENSOR_SUCCESS;

	rtn=ov8856s_lightarray_read_otp_info(handle,param_ptr);
	SENSOR_PRINT("rtn=%d",rtn);

	return rtn;
}
