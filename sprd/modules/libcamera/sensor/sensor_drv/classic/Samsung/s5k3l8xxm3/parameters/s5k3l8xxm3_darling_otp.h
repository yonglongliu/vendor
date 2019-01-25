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
#ifndef _SENSOR_S5K3L8XXM3_OTP_TRULY_H_
#define _SENSOR_S5K3L8XXM3_OTP_TRULY_H_

#define S5K4L8XXM3_OTP_SIZE 8192
#define S5K4L8XXM3_OTP_LSC_LEN 1210  //1658
#define S5K4L8XXM3_OTP_PDAF_SIZE 600 //500
#define S5K3L8XXM3_MASTER_LSC_LEN 1400
#define S5K3L8XXM3_SLAVE_LSC_LEN 400
#define S5K3L8XXM3_OTP_DATA3D_SIZE 1705
#define S5K3L8XXM3_OTP_MASTER_SLAVE_OFFSET 4430
#define S5K3L8XXM3_OTP_DATA3D_OFFSET 6271

static cmr_u8 s5k3l8xxm3_otp_data[S5K4L8XXM3_OTP_SIZE];
static struct sensor_otp_cust_info s5k3l8xxm3_otp_info;

static cmr_u8 s5k3l8xxm3_i2c_read_otp_set(SENSOR_HW_HANDLE handle, cmr_u16 addr);
static int s5k3l8xxm3_otp_init(SENSOR_HW_HANDLE handle);
static int s5k3l8xxm3_otp_read_data(SENSOR_HW_HANDLE handle);
static unsigned long s5k3l8xxm3_otp_read(SENSOR_HW_HANDLE handle, SENSOR_VAL_T* param);
static unsigned long s5k3l8xxm3_parse_otp(SENSOR_HW_HANDLE handle, SENSOR_VAL_T* param);
static unsigned long s5k3l8xxm3_otp_split(SENSOR_HW_HANDLE handle);

#define  s5k3l8xxm3_i2c_read_otp(addr)    s5k3l8xxm3_i2c_read_otp_set(handle, addr)

static cmr_u8 s5k3l8xxm3_i2c_read_otp_set(SENSOR_HW_HANDLE handle, cmr_u16 addr)
{
	return sensor_grc_read_i2c(0xA0 >> 1, addr, BITS_ADDR16_REG8);
}


static int s5k3l8xxm3_otp_init(SENSOR_HW_HANDLE handle)
{
	return 0;
}

static unsigned long s5k3l8xxm3_otp_split(SENSOR_HW_HANDLE handle)
{
	cmr_u16 i = 0;
	cmr_u8 high_val = 0;
	cmr_u8 low_val = 0;
	cmr_u32 checksum = 0;
	cmr_u8 *buffer = NULL;
	struct sensor_single_otp_info *single_otp = NULL;

	SENSOR_LOGI("E");
	single_otp = &s5k3l8xxm3_otp_info.single_otp;
	single_otp->program_flag = s5k3l8xxm3_otp_data[0];
	SENSOR_LOGI("program_flag = %d", single_otp->program_flag);
	if (1 != single_otp->program_flag) {
		SENSOR_LOGI("failed to read otp or the otp is wrong data");
		return -1;
	}
	single_otp->module_info.year = s5k3l8xxm3_otp_data[3];
	single_otp->module_info.month = s5k3l8xxm3_otp_data[4];
	single_otp->module_info.day = s5k3l8xxm3_otp_data[5];
	single_otp->module_info.mid = s5k3l8xxm3_otp_data[1];
	single_otp->module_info.lens_id = s5k3l8xxm3_otp_data[7];
	single_otp->module_info.vcm_id = s5k3l8xxm3_otp_data[8];
	single_otp->module_info.driver_ic_id = s5k3l8xxm3_otp_data[9];

	high_val = s5k3l8xxm3_otp_data[51];
	low_val = s5k3l8xxm3_otp_data[52];
	single_otp->iso_awb_info.iso = (high_val << 8 | low_val);
	high_val = s5k3l8xxm3_otp_data[53];
	low_val = s5k3l8xxm3_otp_data[54];
	single_otp->iso_awb_info.gain_r = (high_val << 8 | low_val);
	high_val = s5k3l8xxm3_otp_data[55];
	low_val = s5k3l8xxm3_otp_data[56];
	single_otp->iso_awb_info.gain_g = (high_val << 8 | low_val);
	high_val = s5k3l8xxm3_otp_data[57];
	low_val = s5k3l8xxm3_otp_data[58];
	single_otp->iso_awb_info.gain_b = (high_val << 8 | low_val);
	SENSOR_LOGI("iso = %d single_otp->gain_r = %d %d %d", single_otp->iso_awb_info.iso, single_otp->iso_awb_info.gain_r,single_otp->iso_awb_info.gain_g,single_otp->iso_awb_info.gain_b);

	single_otp->lsc_info.lsc_data_addr = &s5k3l8xxm3_otp_data[61];
	single_otp->lsc_info.lsc_data_size = S5K4L8XXM3_OTP_LSC_LEN;

	single_otp->pdaf_info.pdaf_data_addr = &s5k3l8xxm3_otp_data[1529];
	single_otp->pdaf_info.pdaf_data_size = S5K4L8XXM3_OTP_PDAF_SIZE;

	single_otp->af_info.flag = s5k3l8xxm3_otp_data[23];
	if (0 == single_otp->af_info.flag)
		SENSOR_LOGI("af otp is wrong");

	high_val = s5k3l8xxm3_otp_data[27];
	low_val = s5k3l8xxm3_otp_data[28];
	single_otp->af_info.infinite_cali = (high_val << 8 | low_val) >> 6;
	high_val = s5k3l8xxm3_otp_data[29];
	low_val= s5k3l8xxm3_otp_data[30];
	single_otp->af_info.macro_cali = (high_val << 8 | low_val) >> 6;
	SENSOR_LOGI("infinite_cali = %d macro_cali = %d",  single_otp->af_info.infinite_cali,single_otp->af_info.macro_cali);

	s5k3l8xxm3_otp_info.total_otp.data_ptr = (void *)s5k3l8xxm3_otp_data;
	s5k3l8xxm3_otp_info.total_otp.size = S5K4L8XXM3_OTP_SIZE;

	/*set below when use dual camera otp*/
	buffer = (cmr_u8 *)s5k3l8xxm3_otp_data;
	s5k3l8xxm3_otp_info.dual_otp.dual_flag = 1;
	s5k3l8xxm3_otp_info.dual_otp.data_3d.data_ptr = (void *)(buffer + S5K3L8XXM3_OTP_DATA3D_OFFSET);
	s5k3l8xxm3_otp_info.dual_otp.data_3d.size = S5K3L8XXM3_OTP_DATA3D_SIZE;
	buffer += S5K3L8XXM3_OTP_MASTER_SLAVE_OFFSET;

	s5k3l8xxm3_otp_info.dual_otp.master_lsc_info.lsc_data_addr = buffer;
	s5k3l8xxm3_otp_info.dual_otp.master_lsc_info.lsc_data_size = S5K3L8XXM3_MASTER_LSC_LEN;
	buffer += S5K3L8XXM3_MASTER_LSC_LEN;

	s5k3l8xxm3_otp_info.dual_otp.slave_lsc_info.lsc_data_addr = buffer;
	s5k3l8xxm3_otp_info.dual_otp.slave_lsc_info.lsc_data_size = S5K3L8XXM3_SLAVE_LSC_LEN;
	buffer += S5K3L8XXM3_SLAVE_LSC_LEN;
	buffer += 25;
	s5k3l8xxm3_otp_info.dual_otp.master_iso_awb_info.iso = (buffer[0] << 8 | buffer[1]);
	buffer += 2;
	s5k3l8xxm3_otp_info.dual_otp.master_iso_awb_info.gain_r = (buffer[0] << 8 | buffer[1]);
	buffer += 2;
	s5k3l8xxm3_otp_info.dual_otp.master_iso_awb_info.gain_g = (buffer[0] << 8 | buffer[1]);
	buffer += 2;
	s5k3l8xxm3_otp_info.dual_otp.master_iso_awb_info.gain_b = (buffer[0] << 8 | buffer[1]);
	buffer += 2;
	s5k3l8xxm3_otp_info.dual_otp.slave_iso_awb_info.iso = (buffer[0] << 8 | buffer[1]);
	buffer += 2;
	s5k3l8xxm3_otp_info.dual_otp.slave_iso_awb_info.gain_r = (buffer[0] << 8 | buffer[1]);
	buffer += 2;
	s5k3l8xxm3_otp_info.dual_otp.slave_iso_awb_info.gain_g = (buffer[0] << 8 | buffer[1]);
	buffer += 2;
	s5k3l8xxm3_otp_info.dual_otp.slave_iso_awb_info.gain_b = (buffer[0] << 8 | buffer[1]);
	
	/*if have dual otp,use master otp replace single otp*/
	s5k3l8xxm3_otp_info.single_otp.iso_awb_info = s5k3l8xxm3_otp_info.dual_otp.master_iso_awb_info;
	s5k3l8xxm3_otp_info.single_otp.lsc_info = s5k3l8xxm3_otp_info.dual_otp.master_lsc_info;
	SENSOR_LOGI("master iso r g b %d %d,%d,%d,lsc size=%d", s5k3l8xxm3_otp_info.dual_otp.master_iso_awb_info.iso,
				s5k3l8xxm3_otp_info.dual_otp.master_iso_awb_info.gain_r,
				s5k3l8xxm3_otp_info.dual_otp.master_iso_awb_info.gain_g,
				s5k3l8xxm3_otp_info.dual_otp.master_iso_awb_info.gain_b,
				s5k3l8xxm3_otp_info.dual_otp.master_lsc_info.lsc_data_size);
	SENSOR_LOGI("slave iso r g b %d %d,%d,%d,lsc size=%d", s5k3l8xxm3_otp_info.dual_otp.slave_iso_awb_info.iso,
				s5k3l8xxm3_otp_info.dual_otp.slave_iso_awb_info.gain_r,
				s5k3l8xxm3_otp_info.dual_otp.slave_iso_awb_info.gain_g,
				s5k3l8xxm3_otp_info.dual_otp.slave_iso_awb_info.gain_b,
				s5k3l8xxm3_otp_info.dual_otp.slave_lsc_info.lsc_data_size);
	{
		FILE *fp = NULL;
		char value[PROPERTY_VALUE_MAX];

		property_get("debug.camera.dump.otp",(char *)value,"0");
		if(atoi(value)) {
			fp = fopen("/data/misc/media/s5k3l8otp.bin", "wb");
			if (NULL == fp) {
				SENSOR_LOGI("failed to open");
			} else {
				fwrite(s5k3l8xxm3_otp_info.total_otp.data_ptr, 1, s5k3l8xxm3_otp_info.total_otp.size, fp);
				fclose(fp);
			}
		}
	}
	SENSOR_LOGI("X");
	return 0;
}

static int s5k3l8xxm3_otp_read_data(SENSOR_HW_HANDLE handle)
{
	cmr_u16 i = 0;
	static cmr_u8 first_flag = 1;
	cmr_u16 checksum = 0;
	cmr_u32 checksum_total = 0;
	cmr_u32 ret_value = SENSOR_SUCCESS;

	SENSOR_LOGI("E");
	//if (first_flag)
	{
		s5k3l8xxm3_otp_data[0] = s5k3l8xxm3_i2c_read_otp(0x0000);
		if (1 != s5k3l8xxm3_otp_data[0]) {
			SENSOR_LOGI("failed to read otp or the otp is wrong data");
			return -1;
		}
		//checksum_total += s5k3l8xxm3_otp_data[0];
		for (i = 0; i < S5K4L8XXM3_OTP_SIZE; i++) {
			s5k3l8xxm3_otp_data[i] = s5k3l8xxm3_i2c_read_otp(0x0000 + i);
			if (i < S5K4L8XXM3_OTP_SIZE - 2)
				checksum_total += s5k3l8xxm3_otp_data[i] ;
		}
		checksum = s5k3l8xxm3_otp_data[S5K4L8XXM3_OTP_SIZE-2] << 8 | s5k3l8xxm3_otp_data[S5K4L8XXM3_OTP_SIZE-1];
		SENSOR_LOGI("checksum_total=0x%x, checksum=0x%x", checksum_total, checksum);
		if ((checksum_total & 0xffff) != checksum ) {
			SENSOR_LOGI("checksum error!");
			return -1;
		}
		ret_value = s5k3l8xxm3_otp_split(handle);
		first_flag = 0;
	}
	SENSOR_LOGI("X");
	return ret_value;
}

static unsigned long s5k3l8xxm3_otp_read(SENSOR_HW_HANDLE handle, SENSOR_VAL_T* param)
{
	cmr_u16 i = 0;
	cmr_u32 ret_value = SENSOR_SUCCESS;

	SENSOR_LOGI("E");
	ret_value = s5k3l8xxm3_otp_read_data(handle);
	if (ret_value) {
		SENSOR_LOGI("otp error");
		param->pval = NULL;
		return ret_value;
	}
	param->pval = (void *)&s5k3l8xxm3_otp_info;
	SENSOR_LOGI("param->pval = %p", param->pval);
	return 0;
}

static unsigned long s5k3l8xxm3_parse_otp(SENSOR_HW_HANDLE handle, SENSOR_VAL_T* param)
{
	cmr_u16 i = 0;
	cmr_u8 *buff = NULL;
	cmr_u16 checksum = 0;
	cmr_u32 checksum_total = 0;
	cmr_u32 ret_value = SENSOR_SUCCESS;

	SENSOR_LOGI("E");
	if (NULL == param->pval) {
		SENSOR_LOGI("s5k3l8xxm3_parse_dual_otp is NULL data");
		return -1;
	}
	buff = param->pval;
	s5k3l8xxm3_otp_data[0] = buff[0];
	if (1 != s5k3l8xxm3_otp_data[0]) {
		SENSOR_LOGI("otp error");
		param->pval = NULL;
		return -1;
	}
	checksum_total += s5k3l8xxm3_otp_data[0];
	for (i = 1; i < S5K4L8XXM3_OTP_SIZE; i++) {
		s5k3l8xxm3_otp_data[i] = buff[i];
		if (i < S5K4L8XXM3_OTP_SIZE - 2)
			checksum_total += s5k3l8xxm3_otp_data[i] ;
	}
	checksum = s5k3l8xxm3_otp_data[S5K4L8XXM3_OTP_SIZE-2] << 8 | s5k3l8xxm3_otp_data[S5K4L8XXM3_OTP_SIZE-1];
	SENSOR_LOGI("checksum_total=0x%x, checksum=0x%x", checksum_total, checksum);

#if 0
	FILE *fd=fopen("/data/misc/media/dual-otp.dump2.bin","wb+");
	for (i = 0; i < 8192; i++) {
		fwrite((char *)&buff[i],1,1,fd);
	}
	fclose(fd);
#endif

	if ((checksum_total & 0xffff) != checksum ) {
		SENSOR_LOGI("checksum error!");
		param->pval = NULL;
		return -1;
	}
	ret_value = s5k3l8xxm3_otp_split(handle);
	if (ret_value) {
		SENSOR_LOGI("otp error");
		param->pval = NULL;
		return ret_value;
	}
	param->pval = (void *)&s5k3l8xxm3_otp_info;
	SENSOR_LOGI("param->pval = %p", param->pval);
	return 0;
}
#endif
