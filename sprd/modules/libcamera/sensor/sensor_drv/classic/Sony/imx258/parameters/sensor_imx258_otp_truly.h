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
#ifndef _SENSOR_IMX258_OTP_TRULY_H_
#define _SENSOR_IMX258_OTP_TRULY_H_

#define IMX258_EEPROM_SLAVE_ADDR  (0xA2 >> 1)

#define IMX258_OTP_LSC_INFO_LEN 1658
static cmr_u8 imx258_opt_lsc_data[IMX258_OTP_LSC_INFO_LEN];
static struct sensor_otp_cust_info imx258_otp_info;

static cmr_u8 imx258_i2c_read_otp_set(SENSOR_HW_HANDLE handle, cmr_u16 addr);
static int imx258_otp_init(SENSOR_HW_HANDLE handle);
static int imx258_otp_read_data(SENSOR_HW_HANDLE handle);
static unsigned long imx258_otp_read(SENSOR_HW_HANDLE handle, SENSOR_VAL_T* param);
static unsigned long imx258_parse_otp(SENSOR_HW_HANDLE handle, SENSOR_VAL_T* param);

#define  imx258_i2c_read_otp(addr)    imx258_i2c_read_otp_set(handle, addr)

static cmr_u8 imx258_i2c_read_otp_set(SENSOR_HW_HANDLE handle, cmr_u16 addr)
{
	return sensor_grc_read_i2c(IMX258_EEPROM_SLAVE_ADDR, addr, BITS_ADDR16_REG8);
}

static int imx258_otp_init(SENSOR_HW_HANDLE handle)
{
	return 0;
}

static int imx258_otp_read_data(SENSOR_HW_HANDLE handle)
{
	return 0;
}

static unsigned long imx258_otp_read(SENSOR_HW_HANDLE handle, SENSOR_VAL_T* param)
{
	struct sensor_otp_cust_info *otp_info = NULL;

	SENSOR_LOGI("E");
	imx258_otp_read_data(handle);
	otp_info = &imx258_otp_info;

	if (1 != otp_info->single_otp.program_flag) {
		SENSOR_LOGI("otp error");
		param->pval = NULL;
		return -1;
	} else {
		param->pval = (void *)otp_info;
	}
	SENSOR_LOGI("param->pval = %p", param->pval);
	return 0;
}

static unsigned long imx258_parse_otp(SENSOR_HW_HANDLE handle, SENSOR_VAL_T* param)
{
	return 0;
}

#endif //_SENSOR_IMX258_OTP_TRULY_H_