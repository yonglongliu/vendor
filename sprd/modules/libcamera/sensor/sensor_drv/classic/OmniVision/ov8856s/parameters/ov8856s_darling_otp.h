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
#ifndef _SENSOR_OV8856S_OTP_TRULY_H_
#define _SENSOR_OV8856S_OTP_TRULY_H_

#define OTP_LSC_INFO_LEN 400  //1658
static cmr_u8 ov8856s_opt_lsc_data[OTP_LSC_INFO_LEN];
static struct sensor_otp_cust_info ov8856s_otp_info;
static int ov8856s_otp_init(SENSOR_HW_HANDLE handle);
static int ov8856s_otp_read_data(SENSOR_HW_HANDLE handle);
static unsigned long ov8856s_otp_read(SENSOR_HW_HANDLE handle, SENSOR_VAL_T* param);
static unsigned long ov8856s_parse_otp(SENSOR_HW_HANDLE handle, SENSOR_VAL_T* param);

#define  ov8856s_i2c_read_otp(addr)    Sensor_ReadReg(0x7010 + (addr))

static int ov8856s_otp_init(SENSOR_HW_HANDLE handle)
{
	return 0;
}

static uint32_t otp_length=0;

static int ov8856s_otp_read_data(SENSOR_HW_HANDLE handle)
{
	cmr_u16 i = 0;
	cmr_u8 high_val = 0;
	cmr_u8 low_val = 0;
	cmr_u8 otp_version = 0;
	cmr_u16 start_addr = 0x0000;
	cmr_u32 checksum = 0;
	static cmr_u8 first_flag = 1;
	static struct sensor_single_otp_info *single_otp = NULL;

	SENSOR_LOGI("E");
	single_otp = &ov8856s_otp_info.single_otp;
	//if (first_flag)
	{
		//otp_version = ov8856s_i2c_read_otp(0x000E);
		int temp1;
		Sensor_WriteReg(0x0100, 0x01);
		temp1 = Sensor_ReadReg(0x5001);
		Sensor_WriteReg(0x5001, (0x00 & 0x08) | (temp1 & (~0x08)));
		// read OTP into buffer
		Sensor_WriteReg(0x3d84, 0xC0);
		Sensor_WriteReg(0x3d88, 0x70); // OTP start address
		Sensor_WriteReg(0x3d89, 0x10);
		Sensor_WriteReg(0x3d8A, 0x72); // OTP end address
		Sensor_WriteReg(0x3d8B, 0x0f);
		Sensor_WriteReg(0x3d81, 0x01); // load otp into buffer
		usleep(10 * 1000);

		// OTP base information and WB calibration data
		otp_version = Sensor_ReadReg(0x7010);
		single_otp->program_flag = ov8856s_i2c_read_otp(0);
		SENSOR_LOGI("program_flag = %d", single_otp->program_flag);
#if 0
		FILE *fd=fopen("/data/misc/media/ov8856s-otp.dump.bin","wb+");
		for (i = 0; i < 463; i++) {
			low_val = ov8856s_i2c_read_otp(i);
			fwrite((char *)&low_val,1,1,fd);
		}
		fclose(fd);
#endif
		if(single_otp->program_flag == 0x0011){
			otp_version = 1;
			start_addr = 0x001E;
		}
		else if(single_otp->program_flag == 0x0014){
			otp_version = 2;
			start_addr = 0x0001;
		}else{
			SENSOR_LOGI("failed to read otp or the otp is wrong data");
			single_otp->program_flag = 0;
			temp1 = Sensor_ReadReg(0x5001);
			Sensor_WriteReg(0x5001, (0x08 & 0x08) | (temp1 & (~0x08)));
			Sensor_WriteReg(0x0100, 0x00);
			return -1;
		}
		single_otp->module_info.year = ov8856s_i2c_read_otp(start_addr + 0x0003);
		single_otp->module_info.month = ov8856s_i2c_read_otp(start_addr + 0x0004);
		single_otp->module_info.day = ov8856s_i2c_read_otp(start_addr + 0x0005);
		single_otp->module_info.mid = ov8856s_i2c_read_otp(start_addr + 0x0001);
		single_otp->module_info.lens_id = ov8856s_i2c_read_otp(start_addr + 0x0007);
		single_otp->module_info.vcm_id = ov8856s_i2c_read_otp(start_addr + 0x0008);
		single_otp->module_info.driver_ic_id = ov8856s_i2c_read_otp(start_addr + 0x0009);

		high_val = ov8856s_i2c_read_otp(start_addr + 0x0013);
		low_val = ov8856s_i2c_read_otp(start_addr + 0x0014);
		single_otp->iso_awb_info.iso = (high_val << 8 | low_val);
		high_val = ov8856s_i2c_read_otp(start_addr + 0x0015);
		low_val = ov8856s_i2c_read_otp(start_addr + 0x0016);
		single_otp->iso_awb_info.gain_r = (high_val << 8 | low_val);
		high_val = ov8856s_i2c_read_otp(start_addr + 0x0017);
		low_val = ov8856s_i2c_read_otp(start_addr + 0x0018);
		single_otp->iso_awb_info.gain_g = (high_val << 8 | low_val);
		high_val = ov8856s_i2c_read_otp(start_addr + 0x0019);
		low_val = ov8856s_i2c_read_otp(start_addr + 0x001A);
		single_otp->iso_awb_info.gain_b = (high_val << 8 | low_val);
		SENSOR_LOGI("iso = 0x%x single_otp->gain_r = 0x%x %x %x", single_otp->iso_awb_info.iso, single_otp->iso_awb_info.gain_r,single_otp->iso_awb_info.gain_g,single_otp->iso_awb_info.gain_b);

		for (i = 0; i < OTP_LSC_INFO_LEN; i++) {
			ov8856s_opt_lsc_data[i] = ov8856s_i2c_read_otp(0x003B + i);
		}
		single_otp->lsc_info.lsc_data_addr = ov8856s_opt_lsc_data;
		single_otp->lsc_info.lsc_data_size = sizeof(ov8856s_opt_lsc_data);
		otp_length=463;
	
		high_val = ov8856s_i2c_read_otp(otp_length-2);
		low_val = ov8856s_i2c_read_otp(otp_length-1);
		single_otp->checksum= (high_val << 8 | low_val);

		for (i = 0; i < otp_length-2; i++) {
			checksum += ov8856s_i2c_read_otp(0x0000 + i);
		}

		SENSOR_LOGI("checksum = 0x%x single_otp->checksum = 0x%x %x %x %x %x", checksum, single_otp->checksum,high_val,low_val,otp_version,otp_length);
#if 0
		FILE *fd=fopen("/data/misc/media/dual-otp.dump.bin","wb+");
		for (i = 0; i < otp_length; i++) {
			low_val = ov8856s_i2c_read_otp(0x0000 + i);
			fwrite((char *)&low_val,1,1,fd);
		}
		fclose(fd);
#endif

		//for(i=0x7010; i <= 0x720a; i++) {
		//	Sensor_WriteReg(i,0); // clear OTP buffer, recommended use continuous write to accelarate,0x720a
		//}
		//set 0x5001[3] to "1"
		temp1 = Sensor_ReadReg(0x5001);
		Sensor_WriteReg(0x5001, (0x08 & 0x08) | (temp1 & (~0x08)));
		Sensor_WriteReg(0x0100, 0x00);
		if ((checksum&0xffff)  != single_otp->checksum) {
			SENSOR_LOGI("checksum error!");
			single_otp->program_flag = 0;
			return -1;
		} else {
			single_otp->program_flag = 1;
		}
		first_flag = 0;
	}
	SENSOR_LOGI("X");
	return 0;
}

static unsigned long ov8856s_otp_read(SENSOR_HW_HANDLE handle, SENSOR_VAL_T* param)
{
	struct sensor_otp_cust_info *otp_info = NULL;
	cmr_int ret = 0;

	SENSOR_LOGI("E");
	ret = ov8856s_otp_read_data(handle);
	otp_info = &ov8856s_otp_info;

	if (ret || 1 != otp_info->single_otp.program_flag) {
		SENSOR_LOGI("otp error");
		param->pval = NULL;
		return -1;
	}
	param->pval = (void *)otp_info;
	SENSOR_LOGI("param->pval = %p", param->pval);
	return 0;
}

static unsigned long ov8856s_parse_otp(SENSOR_HW_HANDLE handle, SENSOR_VAL_T* param)
{
	struct sensor_single_otp_info *single_otp = NULL;
	cmr_u8 *buff = NULL;
	cmr_u16 i = 0;
	cmr_u16 j = 0;
	cmr_u16 start_addr = 0;
	cmr_u16 otp_version = 0;
	cmr_u8 high_val = 0;
	cmr_u8 low_val = 0;
	cmr_u32 checksum = 0;

	SENSOR_LOGI("E");
	if (NULL == param->pval) {
		SENSOR_LOGI("ov8856s_parse_otp is NULL data");
		return -1;
	}
	buff = param->pval;
	otp_length = 463;//ov8856s_i2c_read_otp(0x000E)==1?1733:2033;
	single_otp = &ov8856s_otp_info.single_otp;

	if (0x0022 <  buff[0]) {
		SENSOR_LOGI("ov8856s_parse_otp is wrong data");
		param->pval = NULL;
		return -1;
	} else {
		single_otp->program_flag = buff[0];
		SENSOR_LOGI("program_flag = %d %x", single_otp->program_flag,otp_length);
 		 if(single_otp->program_flag == 0x0011){
			otp_version = 1;
			start_addr = 0x001E;
		}
		else if(single_otp->program_flag == 0x0014){
			otp_version = 2;
			start_addr = 0x0001;
		}else{
			SENSOR_LOGI("failed to read otp or the otp is wrong data");
			param->pval = NULL;
			return -1;
		}

		single_otp->module_info.year = buff[start_addr + 0x0003];
		single_otp->module_info.month = buff[start_addr + 0x0004];
		single_otp->module_info.day = buff[start_addr + 0x0005];
		single_otp->module_info.mid = buff[start_addr + 0x0001];
		single_otp->module_info.lens_id = buff[start_addr + 0x0007];
		single_otp->module_info.vcm_id = buff[start_addr + 0x0008];
		single_otp->module_info.driver_ic_id = buff[start_addr + 0x0009];

		high_val = buff[start_addr + 0x0013];
		low_val = buff[start_addr + 0x0014];
		single_otp->iso_awb_info.iso = (high_val << 8 | low_val);
		high_val = buff[start_addr + 0x0015];
		low_val = buff[start_addr + 0x0016];
		single_otp->iso_awb_info.gain_r = (high_val << 8 | low_val);
		high_val = buff[start_addr + 0x0017];
		low_val = buff[start_addr + 0x0018];
		single_otp->iso_awb_info.gain_g = (high_val << 8 | low_val);
		high_val = buff[start_addr + 0x0019];
		low_val = buff[start_addr + 0x001A];
		single_otp->iso_awb_info.gain_b = (high_val << 8 | low_val);

		for (j = 0; j < OTP_LSC_INFO_LEN; j++) {
			ov8856s_opt_lsc_data[j] = buff[ 0x003B + j];
		}
		single_otp->lsc_info.lsc_data_addr = ov8856s_opt_lsc_data;
		single_otp->lsc_info.lsc_data_size = sizeof(ov8856s_opt_lsc_data);

		single_otp->checksum = buff[otp_length-2]<<8|buff[otp_length-1];
		for (i = 0; i < otp_length-2; i++) {
			checksum += buff[i];
		}
		SENSOR_LOGI("checksum = 0x%x single_otp->checksum = 0x%x", checksum, single_otp->checksum);

		if ((checksum &0xffff) != single_otp->checksum) {
			SENSOR_LOGI("checksum error!");
			single_otp->program_flag = 0;
			param->pval = NULL;
			return -1;
		}else
			single_otp->program_flag = 1;
		param->pval = (void *)&ov8856s_otp_info;
	}
	SENSOR_LOGI("param->pval = %p", param->pval);

	return 0;
}
#endif



