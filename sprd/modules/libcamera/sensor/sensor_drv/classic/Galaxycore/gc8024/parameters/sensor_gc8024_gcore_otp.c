#include <utils/Log.h>
#include "sensor.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"
#include <cutils/properties.h>
//#define OTP_AUTO_LOAD_LSC_gc8024_gcore
#define MODULE_NAME       		"yyy"    //module vendor name
#define MODULE_ID_gc8024_gcore		0x0000    //xxx: sensor P/N;  yyy: module vendor

#define RG_TYPICAL   				0x0000	//modified when write LSC !!!
#define BG_TYPICAL				0x0000	//modified when write LSC!!!

#define INFO_START_ADD         		0x01
#define INFO_WIDTH      	   		0x07
#define WB_START_ADD           		0x6d
#define WB_WIDTH               		0x02
#define DD_BASE_ADDR           		0x11 
#define LSC_BASE_ADDR 		   	0x01
struct otp_info_t {
	uint16_t module_id;
	uint16_t lens_id;
	uint16_t vcm_id;
	uint16_t vcm_start_position;
	uint16_t vcm_driver_id;
	uint16_t year;
	uint16_t month;
	uint16_t day;
	uint16_t rg_gain;
	uint16_t bg_gain;
	uint16_t wb_flag;// 0:Empty 1:Success 2:Invalid
	uint16_t dd_param_x[DD_PARAM_QTY];
	uint16_t dd_param_y[DD_PARAM_QTY];
	uint16_t dd_param_type[DD_PARAM_QTY];
	uint16_t dd_cnt;
	uint16_t dd_flag;// 0:Empty 1:Success 2:Invalid
	uint16_t lsc_flag;// 0:Empty 1:Success 2:Invalid
	uint8_t lsc_r[LSC_WIDTH][LSC_HEIGHT];
	uint8_t lsc_g[LSC_WIDTH][LSC_HEIGHT];
	uint8_t lsc_b[LSC_WIDTH][LSC_HEIGHT];	
};
static struct otp_info_t s_gc8024_gcore_otp_info={0x00};
typedef enum{
	otp_page0=0,
	otp_page1,
}otp_page;

typedef enum{
	otp_close=0,
	otp_open,
}otp_state;


static uint8_t gc8024_read_otp(uint8_t addr)
{
	uint8_t value;

	Sensor_WriteReg(0xfe,0x00);
	Sensor_WriteReg(0xd5,addr);
	Sensor_WriteReg(0xf3,0x20);
	value = Sensor_ReadReg(0xd7);

	return value;
}

static void gc8024_select_page_otp(otp_page otp_select_page)
{
	uint8_t page;
	
	Sensor_WriteReg(0xfe,0x00);
	page = Sensor_ReadReg(0xd4);

	switch(otp_select_page)
	{
	case otp_page0:
		page = page & 0xfb;
		break;
	case otp_page1:
		page = page | 0x04;
		break;
	default:
		break;
	}

	usleep(5 * 1000);
	Sensor_WriteReg(0xd4,page);	

}




static uint32_t gc8024_gcore_read_otp_info(void *param_ptr)
{
	uint32_t rtn = SENSOR_SUCCESS;
	struct otp_info_t *otp_info=(struct otp_info_t *)param_ptr;
	uint8_t check_info_flag0,check_info_flag1;
	uint8_t index,i,j,cnt=0;
	uint8_t info_start_add,wb_start_add;
	uint8_t total_number=0; 
	uint8_t check_flag,type;
	uint8_t dd0=0,dd1=0,dd2=0;
	uint16_t x,y;


	/*TODO*/
	gc8024_select_page_otp(otp_page0);
	check_info_flag0 = gc8024_read_otp(0x00);

	//lens shading

	switch((check_info_flag0>>6)&0x03)
	{
	case 0x00:
		SENSOR_PRINT("gc8024_gcore_read_otp_info: OTP Shading is Empty !!\n");
		otp_info->lsc_flag = otp_info->lsc_flag|0x00;
		break;
	case 0x01:
		SENSOR_PRINT("gc8024_gcore_read_otp_info: OTP Shading is Valid !!\n");
		for(i=0; i<6; i++)
		{
			for(j=0; j<6;j++)
			{
				otp_info->lsc_r[i][j] = gc8024_read_otp(LSC_BASE_ADDR + 6*i + j);
				SENSOR_PRINT("gc8024_gcore_read_otp_info: lsc_r_date[%d][%d]= 0x%x \n",i,j,otp_info->lsc_r[i][j]);
			}
		}
		for(i=0; i<6; i++)
		{			
			for(j=0; j<6;j++)
			{
				otp_info->lsc_g[i][j] = gc8024_read_otp(LSC_BASE_ADDR + 36 + 6*i + j);
				SENSOR_PRINT("gc8024_gcore_read_otp_info: lsc_g_date[%d][%d]= 0x%x \n",i,j,otp_info->lsc_g[i][j]);
			}
		}
		for(i=0; i<6; i++)
		{
			for(j=0; j<6;j++)
			{
				otp_info->lsc_b[i][j] = gc8024_read_otp(LSC_BASE_ADDR + 72 + 6*i + j);
				SENSOR_PRINT("gc8024_gcore_read_otp_info: lsc_b_date[%d][%d]= 0x%x \n",i,j,otp_info->lsc_b[i][j]);
			}
		}
		otp_info->lsc_flag = otp_info->lsc_flag|0x01;
		break;
	case 0x02:
		SENSOR_PRINT_HIGH("gc8024_gcore_read_otp_info: OTP Shading is Invalid !!\n");
		otp_info->lsc_flag = otp_info->lsc_flag|0x02;
		break;
	default :
		break;
	}

//wb
	for(index=0;index<2;index++)
	{
		switch((check_info_flag0>>(4 - 2 * index))&0x03)
		{
		case 0x00:
			SENSOR_PRINT("gc8024_gcore_read_otp_info: OTP wb group%d is Empty !!\n", index + 1);
			otp_info->wb_flag = otp_info->wb_flag|0x00;
			break;
		case 0x01:
			SENSOR_PRINT("gc8024_gcore_read_otp_info: OTP wb group%d is Valid !!\n", index + 1);
			wb_start_add = WB_START_ADD + index * WB_WIDTH;
			otp_info->rg_gain = gc8024_read_otp(wb_start_add);
			otp_info->bg_gain = gc8024_read_otp(wb_start_add + 1); 
			otp_info->wb_flag = otp_info->wb_flag|0x01;
			break;
		case 0x02:
			SENSOR_PRINT("gc8024_gcore_read_otp_info: OTP wb group%d is Invalid !!\n", index + 1);			
			otp_info->wb_flag = otp_info->wb_flag|0x02;
			break;
		default :
			break;
		}
	}
	
	gc8024_select_page_otp(otp_page1);
	check_info_flag1 = gc8024_read_otp(0x00);
	
	SENSOR_PRINT("gc8024_gcore_read_otp_info:check_info_flag1 = 0x%x\n",check_info_flag1);

	//dd
	switch((check_info_flag1>>2)&0x03)
	{
	case 0x00:
		SENSOR_PRINT("gc8024_gcore_read_otp_info: OTP DD is Empty !!\n");
		otp_info->dd_flag = 0x00;
		break;
	case 0x01:
		SENSOR_PRINT("gc8024_gcore_read_otp_info: OTP DD is Valid !!\n");
		
		total_number = gc8024_read_otp(0x0f) + gc8024_read_otp(0x10);
		
		SENSOR_PRINT("gc8024_gcore_read_otp_info:total_number = %d\n",total_number);
		for(i=0; i<total_number; i++)
		{
			check_flag = gc8024_read_otp(DD_BASE_ADDR + 4*i + 3);
				
			if(check_flag&0x10)
			{//Read OTP
				type = check_flag&0x0f;
		
				dd0 = gc8024_read_otp(DD_BASE_ADDR + 4*i);
				dd1 = gc8024_read_otp(DD_BASE_ADDR + 4*i + 1); 	
				dd2 = gc8024_read_otp(DD_BASE_ADDR + 4*i + 2);
				x = ((dd1&0x0f)<<8) + dd0;
				y = (dd2<<4) + ((dd1&0xf0)>>4);
				SENSOR_PRINT("gc8024_gcore_read_otp_info : type = %d , x = %d , y = %d \n",type,x,y);
				SENSOR_PRINT("gc8024_gcore_read_otp_info : dd0 = %d , dd1 = %d , dd2 = %d \n",dd0,dd1,dd2);
				if(type == 3)
				{
					for(j=0; j<4; j++)
					{
						otp_info->dd_param_x[cnt] = x;
						otp_info->dd_param_y[cnt] = y + j;
						otp_info->dd_param_type[cnt++] = 2;
					}
				}
				else
				{
					otp_info->dd_param_x[cnt] = x;
					otp_info->dd_param_y[cnt] = y;
					otp_info->dd_param_type[cnt++] = type;
				}
		
			}
			else
			{
				SENSOR_PRINT("gc8024_gcore_read_otp_info:check_id[%d] = %x,checkid error!!\n",i,check_flag);
			}
		}
		otp_info->dd_cnt = cnt;
		otp_info->dd_flag = 0x01;
		break;
	case 0x02:
		SENSOR_PRINT("gc8024_gcore_read_otp_info: OTP DD is Invalid !!\n");
		otp_info->dd_flag = 0x02;
		break;
	default :
		break;
	}


	for(index=0;index<2;index++)
	{
		switch((check_info_flag1>>(6 - 2 * index))&0x03)
		{
		case 0x00:
			SENSOR_PRINT("gc8024_gcore_read_otp_info: OTP info group%d is Empty !!\n", index + 1);
			break;
		case 0x01:
			SENSOR_PRINT("gc8024_gcore_read_otp_info: OTP info group%d is Valid !!\n", index + 1);
			info_start_add = INFO_START_ADD + index * INFO_WIDTH;
			otp_info->module_id = gc8024_read_otp(info_start_add);
			otp_info->lens_id = gc8024_read_otp(info_start_add + 1); 
			otp_info->vcm_driver_id = gc8024_read_otp(info_start_add + 2)&0x0f;
			otp_info->vcm_start_position = gc8024_read_otp(info_start_add + 2)&0xf0;
			otp_info->vcm_id = gc8024_read_otp(info_start_add + 3);
			otp_info->year = gc8024_read_otp(info_start_add + 4);
			otp_info->month = gc8024_read_otp(info_start_add + 5);
			otp_info->day = gc8024_read_otp(info_start_add + 6);
			break;
		case 0x02:
			SENSOR_PRINT("gc8024_gcore_read_otp_info: OTP info group%d is Invalid !!\n", index + 1);
			break;
		default :
			break;
		}
		
	}
	

	/*print otp information*/
	SENSOR_PRINT("module_id=0x%x",otp_info->module_id);
	SENSOR_PRINT("lens_id=0x%x",otp_info->lens_id);
	SENSOR_PRINT("vcm_id=0x%x",otp_info->vcm_id);
	SENSOR_PRINT("vcm_driver_id=0x%x",otp_info->vcm_driver_id);
	SENSOR_PRINT("vcm_start_position=0x%x",otp_info->vcm_start_position);
	SENSOR_PRINT("data=%d-%d-%d",otp_info->year,otp_info->month,otp_info->day);
	SENSOR_PRINT("wb_r=0x%x",otp_info->rg_gain);
	SENSOR_PRINT("wb_b=0x%x",otp_info->bg_gain);

	
	return rtn;
}

static uint32_t gc8024_gcore_update_dd(void *param_ptr)
{
	uint32_t rtn = SENSOR_SUCCESS;
	struct otp_info_t *otp_info=(struct otp_info_t *)param_ptr;
	uint16_t i=0,j=0,n=0,m=0,s=0,e=0;
	uint16_t x=0,y=0;
	uint16_t temp_x=0,temp_y=0;
	uint8_t temp_type=0;
	uint8_t temp_val0,temp_val1,temp_val2;
	/*TODO*/

	if(0x01 ==otp_info->dd_flag)
	{
#if defined(IMAGE_NORMAL_MIRROR)
		for(i=0; i<otp_info->dd_cnt; i++)
		{
			if(otp_info->dd_param_type[i]==0)
			{	otp_info->dd_param_x[i]= WINDOW_WIDTH - otp_info->dd_param_x[i] + 1;	}
			else if(otp_info->dd_param_type[i]==1)
			{	otp_info->dd_param_x[i]= WINDOW_WIDTH - otp_info->dd_param_x[i] - 1;	}
			else
			{	otp_info->dd_param_x[i]= WINDOW_WIDTH - otp_info->dd_param_x[i];	}
		}
#elif defined(IMAGE_H_MIRROR)
		//do nothing
#elif defined(IMAGE_V_MIRROR)
		for(i=0; i<otp_info->dd_cnt; i++)
		{
			if(otp_info->dd_param_type[i]==0)
			{	
				otp_info->dd_param_x[i]= WINDOW_WIDTH - otp_info->dd_param_x[i];
				otp_info->dd_param_y[i]= WINDOW_HEIGHT - otp_info->dd_param_y[i];
			}
			else if(otp_info->dd_param_type[i]==1)
			{
				otp_info->dd_param_x[i]= WINDOW_WIDTH - otp_info->dd_param_x[i]-1;
				otp_info->dd_param_y[i]= WINDOW_HEIGHT - otp_info->dd_param_y[i]+1;
			}
			else
			{
				otp_info->dd_param_x[i]= WINDOW_WIDTH - otp_info->dd_param_x[i] ;
				otp_info->dd_param_y[i]= WINDOW_HEIGHT - otp_info->dd_param_y[i] + 1;
			}
		}
#elif defined(IMAGE_HV_MIRROR)
		for(i=0; i<otp_info->dd_cnt; i++)
		{	otp_info->dd_param_y[i]= WINDOW_HEIGHT - otp_info->dd_param_y[i] + 1;	}
#endif

		//for(i=0; i<otp_info->dd_cnt; i++)
		//{
		//	SENSOR_PRINT("gc8024_gcore_update_dd 1111, x = %d , y = %d \n",otp_info->dd_param_x[i],otp_info->dd_param_y[i]);	
		//}


		//y
		for(i=0 ; i< otp_info->dd_cnt-1; i++) 
		{
			for(j = i+1; j < otp_info->dd_cnt; j++) 
			{  
				if(otp_info->dd_param_y[i] > otp_info->dd_param_y[j])  
				{  
					temp_x = otp_info->dd_param_x[i] ; otp_info->dd_param_x[i] = otp_info->dd_param_x[j] ;  otp_info->dd_param_x[j] = temp_x;
					temp_y = otp_info->dd_param_y[i] ; otp_info->dd_param_y[i] = otp_info->dd_param_y[j] ;  otp_info->dd_param_y[j] = temp_y;
					temp_type = otp_info->dd_param_type[i] ; otp_info->dd_param_type[i] = otp_info->dd_param_type[j]; otp_info->dd_param_type[j]= temp_type;
				} 
			}
		
		}
		
		//x
		for(i=0; i<otp_info->dd_cnt; i++)
		{
			if(otp_info->dd_param_y[i]==otp_info->dd_param_y[i+1])
			{
				s=i++;
				while((otp_info->dd_param_y[s] == otp_info->dd_param_y[i+1])&&(i<otp_info->dd_cnt-1))
					i++;
				e=i;

				for(n=s; n<e; n++)
				{
					for(m=n+1; m<e+1; m++)
					{
						if(otp_info->dd_param_x[n] > otp_info->dd_param_x[m])
						{
							temp_x = otp_info->dd_param_x[n] ; otp_info->dd_param_x[n] = otp_info->dd_param_x[m] ;  otp_info->dd_param_x[m] = temp_x;
							temp_y = otp_info->dd_param_y[n] ; otp_info->dd_param_y[n] = otp_info->dd_param_y[m] ;  otp_info->dd_param_y[m] = temp_y;
							temp_type = otp_info->dd_param_type[n] ; otp_info->dd_param_type[n] = otp_info->dd_param_type[m]; otp_info->dd_param_type[m]= temp_type;
						}
					}
				}

			}

		}

		
		//write SRAM
		Sensor_WriteReg(0xfe,0x01);
		Sensor_WriteReg(0xbe,0x00);
		Sensor_WriteReg(0xa9,0x01);

		for(i=0; i<otp_info->dd_cnt; i++)
		{
			temp_val0 = otp_info->dd_param_x[i]& 0x00ff;
			temp_val1 = ((otp_info->dd_param_y[i]<<4)& 0x00f0) + ((otp_info->dd_param_x[i]>>8)&0X000f);
			temp_val2 = (otp_info->dd_param_y[i]>>4) & 0xff;
			Sensor_WriteReg(0xaa,i);
			Sensor_WriteReg(0xac,temp_val0);
			Sensor_WriteReg(0xac,temp_val1);
			Sensor_WriteReg(0xac,temp_val2);
			Sensor_WriteReg(0xac,otp_info->dd_param_type[i]);
			SENSOR_PRINT("gc8024_gcore_update_dd travis, val0 = 0x%x , val1 = 0x%x , val2 = 0x%x \n",temp_val0,temp_val1,temp_val2);
			SENSOR_PRINT("gc8024_gcore_update_dd travis, x = %d , y = %d \n",((temp_val1&0x0f)<<8) + temp_val0,(temp_val2<<4) + ((temp_val1&0xf0)>>4));	
		}

		Sensor_WriteReg(0xbe,0x01);
		Sensor_WriteReg(0xfe,0x00);
	}

	return rtn;
}


static uint32_t gc8024_gcore_update_awb(void *param_ptr)
{
	uint32_t rtn = SENSOR_SUCCESS;
	struct otp_info_t *otp_info=(struct otp_info_t *)param_ptr;

	uint16_t r_gain_current = 0 , g_gain_current = 0 , b_gain_current = 0 , base_gain = 0;
	uint16_t r_gain = 0 , g_gain = 0 , b_gain = 0 ;


	/*TODO*/
	if(0x01==(otp_info->wb_flag&0x01))
	{
		r_gain_current = RG_TYPICAL /otp_info->rg_gain;
		b_gain_current = BG_TYPICAL/otp_info->bg_gain;
		g_gain_current = 1;

		base_gain = (r_gain_current<b_gain_current) ? r_gain_current : b_gain_current;
		base_gain = (base_gain<g_gain_current) ? base_gain : g_gain_current;
		//SENSOR_PRINT("GC8024_MIPI_UPDATE_AWB travis, r_gain_current = 0x%x , b_gain_current = 0x%x , base_gain = 0x%x \n",r_gain_current,b_gain_current,base_gain);

		r_gain = 0x80 * r_gain_current / base_gain;
		g_gain = 0x80 * g_gain_current / base_gain;
		b_gain = 0x80 * b_gain_current / base_gain;
		//SENSOR_PRINT("GC8024_MIPI_UPDATE_AWB travis, r_gain = 0x%x , g_gain = 0x%x , b_gain = 0x%x \n",r_gain,g_gain,b_gain);

		Sensor_WriteReg(0xfe,0x00);
		Sensor_WriteReg(0xb8,g_gain);
		Sensor_WriteReg(0xb9,g_gain);
		Sensor_WriteReg(0xba,r_gain);
		Sensor_WriteReg(0xbb,r_gain);
		Sensor_WriteReg(0xbc,b_gain);
		Sensor_WriteReg(0xbd,b_gain);
		Sensor_WriteReg(0xbe,g_gain);
		Sensor_WriteReg(0xbf,g_gain);
	}

	return rtn;
}

#ifndef OTP_AUTO_LOAD_LSC_gc8024_gcore

static uint32_t gc8024_gcore_update_lsc(void *param_ptr)
{
	uint32_t rtn = SENSOR_SUCCESS;
	struct otp_info_t *otp_info=(struct otp_info_t *)param_ptr;
	uint8_t i, j, cnter = 0;
	/*TODO*/
	if(0x01 == otp_info->lsc_flag)
	{
		Sensor_WriteReg(0xfe,0x00);	
		Sensor_WriteReg(0x8d,0x03);//lsc_sram_clock
		Sensor_WriteReg(0xfe,0x01);
		Sensor_WriteReg(0xc9,0x01);
		
		
#ifdef IMAGE_NORMAL_MIRROR
		cnter = 0;
		Sensor_WriteReg(0xcf,0x08);//R
		for(i=0; i<6; i++)
		{
			for(j=6; j>0; j--)
			{
				Sensor_WriteReg(0xca,cnter++);
				Sensor_WriteReg(0xcc,otp_info->lsc_r[i][j-1]);
			}
		}
		cnter = 0;
		Sensor_WriteReg(0xcf,0x04);//G
		for(i=0; i<6; i++)
		{
			for(j=6; j>0; j--)
			{
				Sensor_WriteReg(0xca,cnter++);
				Sensor_WriteReg(0xcc,otp_info->lsc_r[i][j-1]);
			}
		}
		cnter = 0;
		Sensor_WriteReg(0xcf,0x02);//B
		for(i=0; i<6; i++)
		{
			for(j=6; j>0; j--)
			{
				Sensor_WriteReg(0xca,cnter++);
				Sensor_WriteReg(0xcc,otp_info->lsc_r[i][j-1]);
			}
		}

#endif
				
#ifdef IMAGE_H_MIRROR
			cnter = 0;
			Sensor_WriteReg(0xcf,0x08);//R
			for(i=0; i<6; i++)
			{
				for(j=0; j<6; j++)
				{
					Sensor_WriteReg(0xca,cnter++);
					Sensor_WriteReg(0xcc,otp_info->lsc_r[i][j]);
				}
			}
			cnter = 0;
			Sensor_WriteReg(0xcf,0x04);//G
			for(i=0; i<6; i++)
			{
				for(j=0; j<6; j++)
				{
					Sensor_WriteReg(0xca,cnter++);
					Sensor_WriteReg(0xcc,otp_info->lsc_g[i][j]);
				}
			}
			cnter = 0;
			Sensor_WriteReg(0xcf,0x02);//B
			for(i=0; i<6; i++)
			{
				for(j=0; j<6; j++)
				{
					Sensor_WriteReg(0xca,cnter++);
					Sensor_WriteReg(0xcc,otp_info->lsc_b[i][j]);
				}
			}
			
#endif
				
#ifdef IMAGE_V_MIRROR
			cnter = 0;
			Sensor_WriteReg(0xcf,0x08);//R
			for(i=6; i>0; i--)
			{
				for(j=6; j>0; j--)
				{
					Sensor_WriteReg(0xca,cnter++);
					Sensor_WriteReg(0xcc,otp_info->lsc_r[i-1][j-1]);
				}
			}
			cnter = 0;
			Sensor_WriteReg(0xcf,0x04);//G
			for(i=6; i>0; i--)
			{
				for(j=6; j>0; j--)
				{
					Sensor_WriteReg(0xca,cnter++);
					Sensor_WriteReg(0xcc,otp_info->lsc_g[i-1][j-1]);
				}
			}
			cnter = 0;
			Sensor_WriteReg(0xcf,0x02);//B
			for(i=6; i>0; i--)
			{
				for(j=6; j>0; j--)
				{
					Sensor_WriteReg(0xca,cnter++);
					Sensor_WriteReg(0xcc,otp_info->lsc_b[i-1][j-1]);
				}
			}		
#endif
				
#ifdef IMAGE_HV_MIRROR
			cnter = 0;
			Sensor_WriteReg(0xcf,0x08);//R
			for(i=6; i>0; i--)
			{
				for(j=0; j<6; j++)
				{
					Sensor_WriteReg(0xca,cnter++);
					Sensor_WriteReg(0xcc,otp_info->lsc_r[i-1][j]);
				}
			}
			cnter = 0;
			Sensor_WriteReg(0xcf,0x04);//G
			for(i=6; i>0; i--)
			{
				for(j=0; j<6; j++)
				{
					Sensor_WriteReg(0xca,cnter++);
					Sensor_WriteReg(0xcc,otp_info->lsc_g[i-1][j]);
				}
			}
			cnter = 0;
			Sensor_WriteReg(0xcf,0x02);//B
			for(i=6; i>0; i--)
			{
				for(j=0; j<6; j++)
				{
					Sensor_WriteReg(0xca,cnter++);
					Sensor_WriteReg(0xcc,otp_info->lsc_b[i-1][j]);
				}
			}		
#endif
		
			Sensor_WriteReg(0xcf,0x01);
			Sensor_WriteReg(0xa0,0x61);//[0]LSC Enable	
			Sensor_WriteReg(0xfe,0x00);
	}
	return rtn;
}

#endif
static uint32_t gc8024_gcore_update_otp(void *param_ptr)
{
	uint32_t rtn = SENSOR_SUCCESS;
	struct otp_info_t *otp_info=(struct otp_info_t *)param_ptr;

	rtn=gc8024_gcore_update_dd(param_ptr);
	if(rtn!=SENSOR_SUCCESS)
	{
		SENSOR_PRINT_ERR("OTP awb appliy error!");
		return rtn;
	}


	rtn=gc8024_gcore_update_awb(param_ptr);
	if(rtn!=SENSOR_SUCCESS)
	{
		SENSOR_PRINT_ERR("OTP awb appliy error!");
		return rtn;
	}

	#ifndef OTP_AUTO_LOAD_LSC_gc8024_gcore
	
	rtn=gc8024_gcore_update_lsc(param_ptr);
	if(rtn!=SENSOR_SUCCESS)
	{
		SENSOR_PRINT_ERR("OTP lsc appliy error!");
		return rtn;
	}
	#endif
	
	return rtn;
}


static void gc8024_gcore_enable_otp(otp_state state)
{
	uint8_t otp_clk,otp_en;
	otp_clk = Sensor_ReadReg(0xfa);
	otp_en= Sensor_ReadReg(0xd4);	
	if(state)	
	{ 
		otp_clk = otp_clk | 0x40;
		otp_en = otp_en | 0x80;
		usleep(5 * 1000);
		Sensor_WriteReg(0xfa,otp_clk);	// 0xfa[6]:OTP_CLK_en
		Sensor_WriteReg(0xd4,otp_en);	// 0xd4[7]:OTP_en	
	
		SENSOR_PRINT("Enter GC8024_MIPI_enable_OTP: Enable OTP!\n");		
	}
	else			
	{
		otp_en = otp_en & 0x7f;
		otp_clk = otp_clk & 0xbf;
		usleep(5 * 1000);
		Sensor_WriteReg(0xd4,otp_en);
		Sensor_WriteReg(0xfa,otp_clk);

		SENSOR_PRINT("Enter GC8024_MIPI_enable_OTP: Disable OTP!\n");
	}

}


static uint32_t gc8024_gcore_identify_otp(void *param_ptr)
{
	uint32_t rtn = SENSOR_SUCCESS;
	
	Sensor_WriteReg(0xfe,0x00);
	Sensor_WriteReg(0xfe,0x00);
	Sensor_WriteReg(0xfe,0x00);
	Sensor_WriteReg(0xf7,0x95);
	Sensor_WriteReg(0xf8,0x08);
	Sensor_WriteReg(0xf9,0x00);
	Sensor_WriteReg(0xfa,0x09);
	Sensor_WriteReg(0xfc,0xce);
	gc8024_gcore_enable_otp(otp_open);
	rtn=gc8024_gcore_read_otp_info(param_ptr);
	gc8024_gcore_enable_otp(otp_close);
	SENSOR_PRINT("rtn=%d",rtn);

	return rtn;
}

static struct raw_param_info_tab s_gc8024_gcore_raw_param_tab[] = 
	{MODULE_ID_gc8024_gcore, &s_gc8024_mipi_raw_info, gc8024_gcore_identify_otp, gc8024_gcore_update_otp};
