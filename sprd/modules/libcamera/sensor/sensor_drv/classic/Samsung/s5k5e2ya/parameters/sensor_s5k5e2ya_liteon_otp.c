#include <utils/Log.h>
#include "sensor.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"
#include <cutils/properties.h>

#define MODULE_NAME       		"liteon"    //module vendor name
#define MODULE_ID_s5k5e2ya_liteon		0x0002    //s5k5e2ya: sensor P/N;  liteon: module vendor
#define LSC_PARAM_QTY 240

struct otp_info_t {
	uint16_t flag;
	uint16_t module_id;
	uint16_t lens_id;
	uint16_t vcm_id;
	uint16_t vcm_driver_id;
	uint16_t year;
	uint16_t month;
	uint16_t day;
	uint16_t rg_ratio_current;
	uint16_t bg_ratio_current;
	uint16_t rg_ratio_typical;
	uint16_t bg_ratio_typical;
	uint16_t r_current;
	uint16_t g_current;
	uint16_t b_current;
	uint16_t r_typical;
	uint16_t g_typical;
	uint16_t b_typical;
	uint16_t vcm_dac_start;
	uint16_t vcm_dac_inifity;
	uint16_t vcm_dac_macro;
	uint16_t lsc_param[LSC_PARAM_QTY];
};
static struct otp_info_t s_s5k5e2ya_liteon_otp_info={0x00};
#define RG_RATIO_TYPICAL_s5k5e2ya_liteon    	0x02D6
#define BG_RATIO_TYPICAL_s5k5e2ya_liteon		0x0247
#define R_TYPICAL_s5k5e2ya_liteon		0x0000
#define G_TYPICAL_s5k5e2ya_liteon		0x0000
#define B_TYPICAL_s5k5e2ya_liteon		0x0000

#define S5K5E2YAM_OTP_RG_MAX  0x31F  //0.78 x 1024  = 799
#define S5K5E2YAM_OTP_RG_MIN  0x248  //0.57 x 1024  = 584
#define S5K5E2YAM_OTP_BG_MAX  0x2B9  //0.68 x 1024  = 697
#define S5K5E2YAM_OTP_BG_MIN  0x1EC  //0.48 x 1024  = 492


static uint32_t s5k5e2ya_liteon_read_otp_info(void *param_ptr)
{
	uint32_t rtn = SENSOR_SUCCESS;
	struct otp_info_t *otp_info=(struct otp_info_t *)param_ptr;
	otp_info->rg_ratio_typical=RG_RATIO_TYPICAL_s5k5e2ya_liteon;
	otp_info->bg_ratio_typical=BG_RATIO_TYPICAL_s5k5e2ya_liteon;
	otp_info->r_typical=R_TYPICAL_s5k5e2ya_liteon;
	otp_info->g_typical=G_TYPICAL_s5k5e2ya_liteon;
	otp_info->b_typical=B_TYPICAL_s5k5e2ya_liteon;

	/*TODO*/
	//read lsc data. check moudule for debug
	{
		uint32_t i;
		uint32_t lsc_data_count = 0;
	
		Sensor_WriteReg(0x0a00, 0x04); //make initial state
		Sensor_WriteReg(0x0a02, 0x09); //page set
		Sensor_WriteReg(0x0a00, 0x01); //otp enable read
		usleep(2 * 1000);
		for( i = 0; i < 64 ; i++){
			lsc_data_count += Sensor_ReadReg(0x0A04 + i );	
		}
			
		Sensor_WriteReg(0x0a00, 0x04); //make initial state
		Sensor_WriteReg(0x0a00, 0x00); //disable enable read
	
		if(lsc_data_count == 0) {
			SENSOR_PRINT("OTP LSC no data!\n");
			return rtn;
		}			
	}
		
	Sensor_WriteReg(0x0a00, 0x04); //make initial state
	Sensor_WriteReg(0x0a02, 0x04); //page set
	Sensor_WriteReg(0x0a00, 0x01); //otp enable read
	usleep(2 * 1000);
	otp_info->module_id=Sensor_ReadReg(0x0A04);
	SENSOR_PRINT("module_id=0x%x",otp_info->module_id);
	
	if(otp_info->module_id == 0x02){//Liteon
		SENSOR_PRINT("otp is in group 4");
	}
	else{
		Sensor_WriteReg(0x0a00, 0x04); //make initial state
		Sensor_WriteReg(0x0a02, 0x03); //page set
		Sensor_WriteReg(0x0a00, 0x01); //otp enable read
		usleep(2 * 1000);
		otp_info->module_id=Sensor_ReadReg(0x0A04);
		SENSOR_PRINT("module_id=0x%x",otp_info->module_id);
	
		if(otp_info->module_id == 0x02){//Liteon
			SENSOR_PRINT("otp is in group 3");
		}
		else{
			Sensor_WriteReg(0x0a00, 0x04); //make initial state
			Sensor_WriteReg(0x0a02, 0x02); //page set
			Sensor_WriteReg(0x0a00, 0x01); //otp enable read
			usleep(2 * 1000);
			otp_info->module_id=Sensor_ReadReg(0x0A04);
			SENSOR_PRINT("module_id=0x%x",otp_info->module_id);

			if(otp_info->module_id == 0x02){//Liteon
				SENSOR_PRINT("otp is in group 2");
			}
			else{
				rtn=SENSOR_FAIL;
				SENSOR_PRINT("otp all value is zero");
			}
		}
	}
	
	if(otp_info->module_id == 0x02)
	{
		otp_info->rg_ratio_current=	 (Sensor_ReadReg(0x0a09) << 8 ) | Sensor_ReadReg(0x0a0a);
		otp_info->bg_ratio_current=	(Sensor_ReadReg(0x0a0b) << 8 ) | Sensor_ReadReg(0x0a0c);
		//write to register
		SENSOR_PRINT("otp_info->rg_ratio_current=0x%x\n", otp_info->rg_ratio_current);	
		SENSOR_PRINT("otp_info->bg_ratio_current=0x%x\n", otp_info->bg_ratio_current);
		if((otp_info->rg_ratio_current> S5K5E2YAM_OTP_RG_MAX) || (otp_info->rg_ratio_current< S5K5E2YAM_OTP_RG_MIN) || (otp_info->bg_ratio_current> S5K5E2YAM_OTP_BG_MAX) || (otp_info->bg_ratio_current< S5K5E2YAM_OTP_BG_MIN)){
			SENSOR_PRINT("OTP AWB data error!\n");
			rtn=SENSOR_FAIL;
			otp_info->module_id = 0x00;
		} 
	}
	
	Sensor_WriteReg(0x0a00, 0x04); //make initial state
	Sensor_WriteReg(0x0a00, 0x00); //disable enable read

	/*print otp information*/
	SENSOR_PRINT("flag=0x%x",otp_info->flag);
	SENSOR_PRINT("module_id=0x%x",otp_info->module_id);
	SENSOR_PRINT("lens_id=0x%x",otp_info->lens_id);
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

static void s5k5e2ya_liteon_enable_awb_otp(void)
{
	/*TODO enable awb otp update*/

}

static uint32_t s5k5e2ya_liteon_update_awb(void *param_ptr)
{
	uint32_t rtn = SENSOR_SUCCESS;
	struct otp_info_t *otp_info=(struct otp_info_t *)param_ptr;
	s5k5e2ya_liteon_enable_awb_otp();

	/*TODO*/
	uint16_t stream_value = 0;
	uint32_t g_gain=0,r_gain=0,b_gain = 0,g_gain_b ,g_gain_r;

	//calculate R,G,B gain
	if(otp_info->bg_ratio_current < otp_info->bg_ratio_typical){
		if(otp_info->rg_ratio_current< otp_info->rg_ratio_typical){
			g_gain= 0x100;
			b_gain = 0x100 * otp_info->bg_ratio_typical / otp_info->bg_ratio_current;
			r_gain = 0x100 * otp_info->rg_ratio_typical / otp_info->rg_ratio_current;
		}
		else{
	        r_gain = 0x100;
			g_gain = 0x100 * otp_info->rg_ratio_current / otp_info->rg_ratio_typical;
			b_gain = g_gain * otp_info->bg_ratio_typical / otp_info->bg_ratio_current;	        
		}
	}
	else{
		if(otp_info->rg_ratio_current < otp_info->rg_ratio_typical){
	        b_gain = 0x100;
			g_gain = 0x100 * otp_info->bg_ratio_current / otp_info->bg_ratio_typical;
			r_gain = g_gain * otp_info->rg_ratio_typical / otp_info->rg_ratio_current;
		}
		else{
	       	g_gain_b = 0x100*otp_info->bg_ratio_current / otp_info->bg_ratio_typical;
		    g_gain_r = 0x100*otp_info->rg_ratio_current / otp_info->rg_ratio_typical;
			
			if(g_gain_b > g_gain_r)	{
				b_gain = 0x100;
				g_gain = g_gain_b;
				r_gain = g_gain * otp_info->rg_ratio_typical / otp_info->rg_ratio_current;
			}
			else	{
				r_gain = 0x100;
				g_gain = g_gain_r;
				b_gain= g_gain * otp_info->bg_ratio_typical / otp_info->bg_ratio_current;
			}        
		}	
	}

	//write to register
	SENSOR_PRINT("r_Gain=0x%x\n", r_gain);	
	SENSOR_PRINT("g_Gain=0x%x\n", g_gain);	
	SENSOR_PRINT("b_Gain=0x%x\n", b_gain);	

	rtn=Sensor_WriteReg(0x020e, (g_gain&0xff00)>>8);
	rtn=Sensor_WriteReg(0x020f, g_gain&0xff);
	rtn=Sensor_WriteReg(0x0210, (r_gain&0xff00)>>8);
	rtn=Sensor_WriteReg(0x0211, r_gain&0xff);
	rtn=Sensor_WriteReg(0x0212, (b_gain&0xff00)>>8);
	rtn=Sensor_WriteReg(0x0213, b_gain&0xff);
	rtn=Sensor_WriteReg(0x0214, (g_gain&0xff00)>>8);
	rtn=Sensor_WriteReg(0x0215, g_gain&0xff);
	
	return rtn;
	
}

static void s5k5e2ya_liteon_enable_lsc_otp(void)
{
	/*TODO enable lsc otp update*/
	Sensor_WriteReg(0x0b00,0x01);
}

static uint32_t s5k5e2ya_liteon_update_lsc(void *param_ptr)
{
	uint32_t rtn = SENSOR_SUCCESS;
	struct otp_info_t *otp_info=(struct otp_info_t *)param_ptr;
    s5k5e2ya_liteon_enable_lsc_otp();

	/*TODO*/
	
	return rtn;
}

static uint32_t s5k5e2ya_liteon_test_awb(void *param_ptr)
{
	uint32_t flag = 1;
	struct otp_info_t *otp_info=(struct otp_info_t *)param_ptr;
	char value[PROPERTY_VALUE_MAX];
    property_get("persist.sys.camera.otp.awb", value, "normal");
		
	if(!strcmp(value,"normal")){
		SENSOR_PRINT("apply awb otp normally!");
		otp_info->rg_ratio_typical=RG_RATIO_TYPICAL_s5k5e2ya_liteon;
		otp_info->bg_ratio_typical=BG_RATIO_TYPICAL_s5k5e2ya_liteon;
		otp_info->r_typical=R_TYPICAL_s5k5e2ya_liteon;
		otp_info->g_typical=G_TYPICAL_s5k5e2ya_liteon;
		otp_info->b_typical=B_TYPICAL_s5k5e2ya_liteon;
		
	} else if(!strcmp(value,"test_mode")){
		SENSOR_PRINT("apply awb otp on test mode!");
		otp_info->rg_ratio_typical=RG_RATIO_TYPICAL_s5k5e2ya_liteon*1.5;
		otp_info->bg_ratio_typical=BG_RATIO_TYPICAL_s5k5e2ya_liteon*1.5;
		otp_info->r_typical=G_TYPICAL_s5k5e2ya_liteon;
		otp_info->g_typical=G_TYPICAL_s5k5e2ya_liteon;
		otp_info->b_typical=G_TYPICAL_s5k5e2ya_liteon;
		
	} else {
		SENSOR_PRINT("without apply awb otp!");
		flag = 0;
    }
	return flag;
}

static uint32_t s5k5e2ya_liteon_test_lsc(void)
{
	uint32_t flag = 1;
	char value[PROPERTY_VALUE_MAX];
	property_get("persist.sys.camera.otp.lsc", value, "on");
	
	if(!strcmp(value,"on")){
		SENSOR_PRINT("apply lsc otp normally!");
		flag = 1;
	} else{
		SENSOR_PRINT("without apply lsc otp!");
		flag = 0;
	}
    return flag;
}

static uint32_t s5k5e2ya_liteon_update_otp(void *param_ptr)
{
	uint32_t rtn = SENSOR_SUCCESS;
	struct otp_info_t *otp_info=(struct otp_info_t *)param_ptr;

	/*update awb*/
	if(s5k5e2ya_liteon_test_awb(otp_info)){
		rtn=s5k5e2ya_liteon_update_awb(param_ptr);
		if(rtn!=SENSOR_SUCCESS)
		{
			SENSOR_PRINT_ERR("OTP awb apply error!");
			return rtn;
		}
	} else {
		rtn = SENSOR_SUCCESS;
	}
	
	/*update lsc*/
	if(s5k5e2ya_liteon_test_lsc()){
		rtn=s5k5e2ya_liteon_update_lsc(param_ptr);
		if(rtn!=SENSOR_SUCCESS)
		{
			SENSOR_PRINT_ERR("OTP lsc apply error!");
			return rtn;
		}
	} else {
		rtn = SENSOR_SUCCESS;
	}

	return rtn;
}

static uint32_t s5k5e2ya_liteon_identify_otp(void *param_ptr)
{
	uint32_t rtn = SENSOR_SUCCESS;
    struct otp_info_t *otp_info=(struct otp_info_t *)param_ptr;

	rtn=s5k5e2ya_liteon_read_otp_info(param_ptr);
	if(rtn!=SENSOR_SUCCESS){
		SENSOR_PRINT_ERR("read otp information failed\n!");		
		return rtn;
	} else if(MODULE_ID_s5k5e2ya_liteon !=otp_info->module_id){
		SENSOR_PRINT_ERR("identify otp fail! module id mismatch!");		
		rtn=SENSOR_FAIL;
	} else {
		SENSOR_PRINT("identify otp success!");
	}

	return rtn;
}

static struct raw_param_info_tab s_s5k5e2ya_liteon_raw_param_tab[] = 
	{MODULE_ID_s5k5e2ya_liteon, &s_s5k5e2ya_mipi_raw_info, s5k5e2ya_liteon_identify_otp, s5k5e2ya_liteon_update_otp};