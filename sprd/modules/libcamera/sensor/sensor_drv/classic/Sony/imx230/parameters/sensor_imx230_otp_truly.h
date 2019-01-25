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
#ifndef _SENSOR_IMX230_OTP_TRULY_H_
#define _SENSOR_IMX230_OTP_TRULY_H_

#define IMX230_OTP_SIZE 8192
#define IMX230_LSC_LEN 1658
#define IMX230_MASTER_LSC_LEN 1658
#define IMX230_SLAVE_LSC_LEN 400
#define IMX230_OTP_DATA3D_SIZE 2142
#define IMX230_OTP_MASTER_SLAVE_OFFSET 1702
#define IMX230_OTP_DATA3D_OFFSET 4384

static cmr_u8 imx230_otp_data[IMX230_OTP_SIZE];
static struct sensor_otp_cust_info imx230_otp_info;


static cmr_u8 imx230_i2c_read_otp_set(SENSOR_HW_HANDLE handle, cmr_u16 addr);
static int imx230_otp_init(SENSOR_HW_HANDLE handle);
static int imx230_otp_read_data(SENSOR_HW_HANDLE handle);
static unsigned long imx230_otp_read(SENSOR_HW_HANDLE handle, SENSOR_VAL_T* param);
static unsigned long imx230_parse_otp(SENSOR_HW_HANDLE handle, SENSOR_VAL_T* param);
static unsigned long imx230_otp_split(SENSOR_HW_HANDLE handle);


#define  imx230_i2c_read_otp(addr)    imx230_i2c_read_otp_set(handle, addr)

static cmr_u8 imx230_i2c_read_otp_set(SENSOR_HW_HANDLE handle, cmr_u16 addr)
{
	return sensor_grc_read_i2c(0xA0 >> 1, addr, BITS_ADDR16_REG8);
}


static int imx230_otp_init(SENSOR_HW_HANDLE handle)
{
	return 0;
}

static unsigned long imx230_otp_split(SENSOR_HW_HANDLE handle)
{
	cmr_u16 i = 0;
	cmr_u8 high_val = 0;
	cmr_u8 low_val = 0;
	cmr_u32 checksum = 0;
	cmr_u8 *buffer = NULL;
	struct sensor_single_otp_info *single_otp = NULL;

	SENSOR_LOGI("E");
	single_otp = &imx230_otp_info.single_otp;
	single_otp->program_flag = imx230_otp_data[0];
	SENSOR_LOGI("program_flag = %d", single_otp->program_flag);
	if (1 != single_otp->program_flag) {
		SENSOR_LOGI("failed to read otp or the otp is wrong data");
		return -1;
	}
	checksum += single_otp->program_flag;
	single_otp->module_info.year = imx230_otp_data[1];
	checksum += single_otp->module_info.year;
	single_otp->module_info.month = imx230_otp_data[2];
	checksum += single_otp->module_info.month;
	single_otp->module_info.day = imx230_otp_data[3];
	checksum += single_otp->module_info.day;
	single_otp->module_info.mid = imx230_otp_data[4];
	checksum += single_otp->module_info.mid;
	single_otp->module_info.lens_id = imx230_otp_data[5];
	checksum += single_otp->module_info.lens_id;
	single_otp->module_info.vcm_id = imx230_otp_data[6];
	checksum += single_otp->module_info.vcm_id;
	single_otp->module_info.driver_ic_id = imx230_otp_data[7];
	checksum += single_otp->module_info.driver_ic_id;

	high_val = imx230_otp_data[16];
	checksum += high_val;
	low_val = imx230_otp_data[17];
	checksum += low_val;
	single_otp->iso_awb_info.iso = (high_val << 8 | low_val);
	high_val = imx230_otp_data[18];
	checksum += high_val;
	low_val = imx230_otp_data[19];
	checksum += low_val;
	single_otp->iso_awb_info.gain_r = (high_val << 8 | low_val);
	high_val = imx230_otp_data[20];
	checksum += high_val;
	low_val = imx230_otp_data[21];
	checksum += low_val;
	single_otp->iso_awb_info.gain_g = (high_val << 8 | low_val);
	high_val = imx230_otp_data[22];
	checksum += high_val;
	low_val = imx230_otp_data[23];
	checksum += low_val;
	single_otp->iso_awb_info.gain_b = (high_val << 8 | low_val);

	for (i = 0; i < IMX230_LSC_LEN; i++) {
		checksum += imx230_otp_data[32+i];
	}

	single_otp->lsc_info.lsc_data_addr = &imx230_otp_data[32];
	single_otp->lsc_info.lsc_data_size = IMX230_LSC_LEN;

	single_otp->af_info.flag = imx230_otp_data[1696];
	if (0 == single_otp->af_info.flag)
		SENSOR_LOGI("af otp is wrong");

	checksum += single_otp->af_info.flag;

	low_val = imx230_otp_data[1697];
	checksum += low_val;
	high_val = imx230_otp_data[1698];
	checksum += high_val;
	single_otp->af_info.infinite_cali = (high_val << 8 | low_val);
	low_val = imx230_otp_data[1699];
	checksum += low_val;
	high_val = imx230_otp_data[1700];
	checksum += high_val;
	single_otp->af_info.macro_cali = (high_val << 8 | low_val);

	single_otp->checksum = imx230_otp_data[1701];

	SENSOR_LOGI("infinite_cali = %d, macro_cali = %d", single_otp->af_info.infinite_cali, single_otp->af_info.macro_cali);
	SENSOR_LOGI("checksum = 0x%x single_otp->checksum = 0x%x", checksum, single_otp->checksum);

	if ((checksum % 0xff) != single_otp->checksum) {
		SENSOR_LOGI("checksum error!");
		single_otp->program_flag = 0;
		return -1;
	}
	imx230_otp_info.total_otp.data_ptr = (void *)imx230_otp_data;
	imx230_otp_info.total_otp.size = IMX230_OTP_SIZE;

	/*set below when use dual camera otp*/
	buffer = (cmr_u8 *)imx230_otp_data;
	imx230_otp_info.dual_otp.dual_flag = 1;
	imx230_otp_info.dual_otp.data_3d.data_ptr = (void *)(buffer + IMX230_OTP_DATA3D_OFFSET);
	imx230_otp_info.dual_otp.data_3d.size = IMX230_OTP_DATA3D_SIZE;
	buffer += IMX230_OTP_MASTER_SLAVE_OFFSET;
	imx230_otp_info.dual_otp.master_iso_awb_info.iso = (buffer[0] << 8 | buffer[1]);
	buffer += 2;
	imx230_otp_info.dual_otp.master_iso_awb_info.gain_r = (buffer[0] << 8 | buffer[1]);
	buffer += 2;
	imx230_otp_info.dual_otp.master_iso_awb_info.gain_g = (buffer[0] << 8 | buffer[1]);
	buffer += 2;
	imx230_otp_info.dual_otp.master_iso_awb_info.gain_b = (buffer[0] << 8 | buffer[1]);
	buffer += 2;
	imx230_otp_info.dual_otp.master_lsc_info.lsc_data_addr = buffer;
	imx230_otp_info.dual_otp.master_lsc_info.lsc_data_size = IMX230_MASTER_LSC_LEN;
	buffer += IMX230_MASTER_LSC_LEN;
	imx230_otp_info.dual_otp.slave_iso_awb_info.iso = (buffer[0] << 8 | buffer[1]);
	buffer += 2;
	imx230_otp_info.dual_otp.slave_iso_awb_info.gain_r = (buffer[0] << 8 | buffer[1]);
	buffer += 2;
	imx230_otp_info.dual_otp.slave_iso_awb_info.gain_g = (buffer[0] << 8 | buffer[1]);
	buffer += 2;
	imx230_otp_info.dual_otp.slave_iso_awb_info.gain_b = (buffer[0] << 8 | buffer[1]);
	buffer += 2;
	imx230_otp_info.dual_otp.slave_lsc_info.lsc_data_addr = buffer;
	imx230_otp_info.dual_otp.slave_lsc_info.lsc_data_size = IMX230_SLAVE_LSC_LEN;
	/*if have dual otp,use master otp replace single otp*/
	imx230_otp_info.single_otp.iso_awb_info = imx230_otp_info.dual_otp.master_iso_awb_info;
	imx230_otp_info.single_otp.lsc_info = imx230_otp_info.dual_otp.master_lsc_info;
	SENSOR_LOGI("master iso r g b %d %d,%d,%d,lsc size=%d", imx230_otp_info.dual_otp.master_iso_awb_info.iso,
				imx230_otp_info.dual_otp.master_iso_awb_info.gain_r,
				imx230_otp_info.dual_otp.master_iso_awb_info.gain_g,
				imx230_otp_info.dual_otp.master_iso_awb_info.gain_b,
				imx230_otp_info.dual_otp.master_lsc_info.lsc_data_size);
	SENSOR_LOGI("slave iso r g b %d %d,%d,%d,lsc size=%d", imx230_otp_info.dual_otp.slave_iso_awb_info.iso,
				imx230_otp_info.dual_otp.slave_iso_awb_info.gain_r,
				imx230_otp_info.dual_otp.slave_iso_awb_info.gain_g,
				imx230_otp_info.dual_otp.slave_iso_awb_info.gain_b,
				imx230_otp_info.dual_otp.slave_lsc_info.lsc_data_size);
	return 0;
}

static int imx230_otp_read_data(SENSOR_HW_HANDLE handle)
{
	cmr_u16 i = 0;
	static cmr_u8 first_flag = 1;
	cmr_u16 checksum = 0;
	cmr_u32 checksum_total = 0;
	cmr_u32 ret_value = SENSOR_SUCCESS;

	SENSOR_LOGI("E");
	//if (first_flag)
	{
		imx230_otp_data[0] = imx230_i2c_read_otp(0x0000);
		if (1 != imx230_otp_data[0]) {
			SENSOR_LOGI("failed to read otp or the otp is wrong data");
			return -1;
		}
		checksum_total += imx230_otp_data[0];
		for (i = 1; i < IMX230_OTP_SIZE; i++) {
			imx230_otp_data[i] = imx230_i2c_read_otp(0x0000 + i);
			if (i < IMX230_OTP_SIZE - 2)
				checksum_total += imx230_otp_data[i] ;
		}
		checksum = imx230_otp_data[IMX230_OTP_SIZE - 2] << 8 | imx230_otp_data[IMX230_OTP_SIZE - 1];
		SENSOR_LOGI("checksum_total=0x%x, checksum=0x%x", checksum_total, checksum);
		if ((checksum_total & 0xffff) != checksum ) {
			SENSOR_LOGI("checksum error!");
			return -1;
		}
		ret_value = imx230_otp_split(handle);
		first_flag = 0;
	}
	SENSOR_LOGI("X");
	return ret_value;
}

static unsigned long imx230_otp_read(SENSOR_HW_HANDLE handle, SENSOR_VAL_T* param)
{
	cmr_u16 i = 0;
	cmr_u32 ret_value = SENSOR_SUCCESS;

	SENSOR_LOGI("E");
	ret_value = imx230_otp_read_data(handle);
	if (ret_value) {
		SENSOR_LOGI("otp error");
		param->pval = NULL;
		return ret_value;
	}
	param->pval = (void *)&imx230_otp_info;
	SENSOR_LOGI("param->pval = %p", param->pval);
	return 0;
}

static unsigned long imx230_parse_otp(SENSOR_HW_HANDLE handle, SENSOR_VAL_T* param)
{
	cmr_u16 i = 0;
	cmr_u8 *buff = NULL;
	cmr_u16 checksum = 0;
	cmr_u32 checksum_total = 0;
	cmr_u32 ret_value = SENSOR_SUCCESS;

	SENSOR_LOGI("E");
	if (NULL == param->pval) {
		SENSOR_LOGI("imx230_parse_dual_otp is NULL data");
		return -1;
	}
	buff = param->pval;
	imx230_otp_data[0] = buff[0];
	if (1 != imx230_otp_data[0]) {
		SENSOR_LOGI("otp error");
		param->pval = NULL;
		return -1;
	}
	checksum_total += imx230_otp_data[0];
	for (i = 1; i < IMX230_OTP_SIZE; i++) {
		imx230_otp_data[i] = buff[i];
		if (i < IMX230_OTP_SIZE - 2)
			checksum_total += imx230_otp_data[i] ;
	}
	checksum = imx230_otp_data[IMX230_OTP_SIZE - 2] << 8 | imx230_otp_data[IMX230_OTP_SIZE - 1];
	SENSOR_LOGI("checksum_total=0x%x, checksum=0x%x", checksum_total, checksum);
	if ((checksum_total & 0xffff) != checksum ) {
		SENSOR_LOGI("checksum error!");
		param->pval = NULL;
		return -1;
	}
	ret_value = imx230_otp_split(handle);
	if (ret_value) {
		SENSOR_LOGI("otp error");
		param->pval = NULL;
		return ret_value;
	}
	param->pval = (void *)&imx230_otp_info;
	SENSOR_LOGI("param->pval = %p", param->pval);
	return 0;
}
#endif
