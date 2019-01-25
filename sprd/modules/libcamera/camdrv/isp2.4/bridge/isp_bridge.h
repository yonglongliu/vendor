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
#ifndef _ISP_BRIDGE_H_
#define _ISP_BRIDGE_H_

#include "isp_type.h"
#include "cmr_sensor_info.h"

#define SENSOR_NUM_MAX 4

typedef cmr_int(*func_isp_br_ioctrl) (cmr_u32 camera_id, cmr_int cmd, void *in, void *out);

enum isp_br_ioctl_cmd {
	SET_MATCH_AWB_DATA = 0,
	GET_MATCH_AWB_DATA,
	SET_MATCH_AE_DATA,
	GET_MATCH_AE_DATA,
	SET_MATCH_BV_DATA,
	GET_MATCH_BV_DATA,
	SET_STAT_AWB_DATA,
	GET_STAT_AWB_DATA,
	SET_GAIN_AWB_DATA,
	GET_GAIN_AWB_DATA,
	SET_MODULE_INFO,
	GET_MODULE_INFO,
	SET_OTP_AE,
	GET_OTP_AE,
	SET_OTP_AWB,
	GET_OTP_AWB,
	SET_ALL_MODULE_AND_OTP,
	GET_ALL_MODULE_AND_OTP,
	AE_WAIT_SEM,
	AE_POST_SEM,
	AWB_WAIT_SEM,
	AWB_POST_SEM,
};

struct awb_stat_data {
	cmr_u32 r_info[1024];
	cmr_u32 g_info[1024];
	cmr_u32 b_info[1024];
};

struct awb_gain_data {
	cmr_u32 r_gain;
	cmr_u32 g_gain;
	cmr_u32 b_gain;
};

struct awb_match_data {
	cmr_u32 ct;
};

struct ae_otp_param {
	struct sensor_otp_ae_info otp_info;
};

struct awb_otp_param {
	struct sensor_otp_awb_info awb_otp_info;
};

struct sensor_info {
	cmr_s16 min_exp_line;
	cmr_s16 max_again;
	cmr_s16 min_again;
	cmr_s16 sensor_gain_precision;
	cmr_u32 line_time;
};

struct module_sensor_info {
	struct sensor_info sensor_info[SENSOR_NUM_MAX];
};

struct module_otp_info {
	struct ae_otp_param ae_otp[SENSOR_NUM_MAX];
	struct awb_otp_param awb_otp[SENSOR_NUM_MAX];
};

struct module_info {
	struct module_sensor_info module_sensor_info;
	struct module_otp_info module_otp_info;
};

struct ae_match_data {
	cmr_u32 gain;
	cmr_u32 isp_gain;
	struct sensor_ex_exposure exp;
};

struct match_data_param {
	struct module_info module_info;
	struct ae_match_data ae_info;
	struct awb_match_data awb_info;
	struct awb_stat_data awb_stat[SENSOR_NUM_MAX];
	struct awb_gain_data awb_gain;
	cmr_u16 bv;
};

cmr_handle isp_br_get_3a_handle(cmr_u32 camera_id);
cmr_int isp_br_init(cmr_u32 camera_id, cmr_handle isp_3a_handle);
cmr_int isp_br_deinit(cmr_u32 camera_id);
cmr_int isp_br_ioctrl(cmr_u32 camera_id, cmr_int cmd, void *in, void *out);
cmr_int isp_br_save_dual_otp(cmr_u32 camera_id, struct sensor_dual_otp_info *dual_otp);
cmr_int isp_br_get_dual_otp(cmr_u32 camera_id, struct sensor_dual_otp_info **dual_otp);
#endif
