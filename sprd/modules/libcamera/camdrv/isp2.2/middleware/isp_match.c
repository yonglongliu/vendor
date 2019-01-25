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
#define LOG_TAG "isp_match"
#include <time.h>
#include <sys/time.h>
#include "isp_match.h"

/**************************************** MACRO DEFINE *****************************************/
#define SENSOR_NUM_MAX 4
#define AE_SYNC_WAIT_TIMEOUT   200

/************************************* INTERNAL DATA TYPE **************************************/
struct ispbr_context {
	cmr_u32 user_cnt;
	cmr_handle isp_3afw_slave_handles;
	cmr_handle isp_3afw_master_handles;

	sem_t ae_sm;
	sem_t ae_updata_sm;
	sem_t ae_update_sm2;
	sem_t awb_sm;
	sem_t  module_sm;
	struct match_data_param match_param;
	struct ae_calc_out  slave_out;
	struct ae_calc_out  master_out;
	struct sensor_dual_otp_info *dual_otp[SENSOR_NUM_MAX];
};

static struct ispbr_context br_cxt;
static pthread_mutex_t g_br_mutex = PTHREAD_MUTEX_INITIALIZER;

/*************************************INTERNAK FUNCTION ****************************************/
cmr_handle isp_br_get_3a_handle(cmr_u32 is_master)
{
	cmr_handle rtn = NULL;

	ISP_LOGD("is_master %d", is_master);
	if(!is_master) {
		rtn = br_cxt.isp_3afw_slave_handles;
	} else {
		rtn = br_cxt.isp_3afw_master_handles;
	}
	return rtn;
}
cmr_int isp_br_ioctrl(cmr_u32 camera_id, cmr_int cmd, void *in, void *out)
{
	struct ispbr_context *cxt = &br_cxt;
	struct timespec ts;
	int result=0;

	if (camera_id >= SENSOR_NUM_MAX) {
		ISP_LOGE("invalid camera_id %u", camera_id);
		return -ISP_PARAM_ERROR;
	}

	if (((cmd & 1) && !out) || ((!(cmd & 1)) && !in)) {
		ISP_LOGE("invalid param: cmd=%ld, in=%p, out=%p", cmd, in, out);
		return -ISP_PARAM_ERROR;
	}

	ISP_LOGV("E camera_id=%u, cmd=%ld", camera_id, cmd);

	switch (cmd) {
	case SET_MATCH_AWB_DATA:
		sem_wait(&cxt->awb_sm);
		memcpy(&cxt->match_param.master_awb_info, in, sizeof(cxt->match_param.master_awb_info));
		sem_post(&cxt->awb_sm);
		break;
	case GET_MATCH_AWB_DATA:
		sem_wait(&cxt->awb_sm);
		memcpy(out, &cxt->match_param.master_awb_info, sizeof(cxt->match_param.master_awb_info));
		sem_post(&cxt->awb_sm);
		break;
	case SET_SLAVE_MODULE_INFO:
		sem_wait(&cxt->module_sm);
		memcpy(&cxt->match_param.module_info.module_sensor_info.slave_sensor_info, in,
			sizeof(cxt->match_param.module_info.module_sensor_info.slave_sensor_info));
		sem_post(&cxt->module_sm);
		break;
	case GET_SLAVE_MODULE_INFO:
		sem_wait(&cxt->module_sm);
		memcpy(out, &cxt->match_param.module_info.module_sensor_info.slave_sensor_info,
			sizeof(cxt->match_param.module_info.module_sensor_info.slave_sensor_info));
		sem_post(&cxt->module_sm);
		break;
	case SET_MASTER_MODULE_INFO:
		sem_wait(&cxt->module_sm);
		memcpy(&cxt->match_param.module_info.module_sensor_info.master_sensor_info, in,
			sizeof(cxt->match_param.module_info.module_sensor_info.master_sensor_info));
		sem_post(&cxt->module_sm);
		break;
	case GET_MASTER_MODULE_INFO:
		sem_wait(&cxt->module_sm);
		memcpy(out, &cxt->match_param.module_info.module_sensor_info.master_sensor_info,
			sizeof(cxt->match_param.module_info.module_sensor_info.master_sensor_info));
		sem_post(&cxt->module_sm);
		break;
	case SET_SLAVE_OTP_AE:
		sem_wait(&cxt->module_sm);
		memcpy(&cxt->match_param.module_info.module_otp_info.slave_ae_otp, in,
			sizeof(cxt->match_param.module_info.module_otp_info.slave_ae_otp));
		sem_post(&cxt->module_sm);
		break;
	case GET_SLAVE_OTP_AE:
		sem_wait(&cxt->module_sm);
		memcpy(out, &cxt->match_param.module_info.module_otp_info.slave_ae_otp,
			sizeof(cxt->match_param.module_info.module_otp_info.slave_ae_otp));
		sem_post(&cxt->module_sm);
		break;
	case SET_MASTER_OTP_AE:
		sem_wait(&cxt->module_sm);
		memcpy(&cxt->match_param.module_info.module_otp_info.master_ae_otp, in,
			sizeof(cxt->match_param.module_info.module_otp_info.master_ae_otp));
		sem_post(&cxt->module_sm);
		break;
	case GET_MASTER_OTP_AE:
		sem_wait(&cxt->module_sm);
		memcpy(out, &cxt->match_param.module_info.module_otp_info.master_ae_otp,
			sizeof(cxt->match_param.module_info.module_otp_info.master_ae_otp));
		sem_post(&cxt->module_sm);
		break;
	case SET_SLAVE_AE_SYNC_STATUS:
		sem_wait(&cxt->ae_update_sm2);
		memcpy(&cxt->match_param.slave_ae_info.ae_sync_status, in,
			sizeof(cxt->match_param.slave_ae_info.ae_sync_status));
		sem_post(&cxt->ae_update_sm2);
		break;
	case GET_SLAVE_AE_SYNC_STATUS:
		sem_wait(&cxt->ae_update_sm2);
		memcpy(out, &cxt->match_param.slave_ae_info.ae_sync_status,
			sizeof(cxt->match_param.slave_ae_info.ae_sync_status));
		sem_post(&cxt->ae_update_sm2);
		break;
	case SET_MASTER_AE_SYNC_STATUS:
		sem_wait(&cxt->ae_update_sm2);
		memcpy(&cxt->match_param.master_ae_info.ae_sync_status, in,
			sizeof(cxt->match_param.master_ae_info.ae_sync_status));
		sem_post(&cxt->ae_update_sm2);
		break;
	case GET_MASTER_AE_SYNC_STATUS:
		sem_wait(&cxt->ae_update_sm2);
		memcpy(out, &cxt->match_param.master_ae_info.ae_sync_status,
			sizeof(cxt->match_param.master_ae_info.ae_sync_status));
		sem_post(&cxt->ae_update_sm2);
		break;
	case SET_SLAVE_AESYNC_SETTING:
		sem_wait(&cxt->ae_update_sm2);
		int post = cxt->match_param.slave_ae_info.ae_sync_result.updata_flag;
		ISP_LOGD("master write, flag=%u", cxt->match_param.slave_ae_info.ae_sync_result.updata_flag);
		memcpy(&cxt->match_param.slave_ae_info.ae_sync_result, in,
			sizeof(cxt->match_param.slave_ae_info.ae_sync_result));
		sem_post(&cxt->ae_update_sm2);
		if (!post) {
			sem_post(&cxt->ae_updata_sm);
			ISP_LOGD("master post sm");
		}
		break;
	case GET_SLAVE_AESYNC_SETTING:
		if (clock_gettime(CLOCK_REALTIME, &ts)) {
			ISP_LOGE("clock_gettime err");
			break;
		} else {
			ts.tv_sec += 0;
			ts.tv_nsec += ms2ns(AE_SYNC_WAIT_TIMEOUT);
			if (ts.tv_nsec >= 1000000000) {
				ts.tv_nsec -= 1000000000;
				ts.tv_sec += 1;
			}
		}

		ISP_LOGD("slave wait sm");
		if(cxt->match_param.master_ae_info.ae_sync_status== SYNC_RUN)
			result=sem_timedwait(&cxt->ae_updata_sm, &ts);
		if (result == -1 && errno == ETIMEDOUT)
			ISP_LOGD("slave sem_timedwait \n");
		else
			ISP_LOGD("slave wait sm  result =%d, ae sync status =%d", result,
				cxt->match_param.master_ae_info.ae_sync_status );
		sem_wait(&cxt->ae_update_sm2);
		ISP_LOGD("slave read, flag=%u", cxt->match_param.slave_ae_info.ae_sync_result.updata_flag);
		memcpy(out, &cxt->match_param.slave_ae_info.ae_sync_result,
			sizeof(cxt->match_param.slave_ae_info.ae_sync_result));
		cxt->match_param.slave_ae_info.ae_sync_result.updata_flag = 0;
		sem_post(&cxt->ae_update_sm2);
		break;
	case SET_SLAVE_AECALC_RESULT:
		sem_wait(&cxt->ae_sm);
		memcpy(&cxt->match_param.slave_ae_info.ae_calc_result, in,
			sizeof(cxt->match_param.slave_ae_info.ae_calc_result));
		sem_post(&cxt->ae_sm);
		break;
	case GET_SLAVE_AECALC_RESULT:
		sem_wait(&cxt->ae_sm);
		memcpy(out, &cxt->match_param.slave_ae_info.ae_calc_result,
			sizeof(cxt->match_param.slave_ae_info.ae_calc_result));
		sem_post(&cxt->ae_sm);
		break;
	case SET_MASTER_AESYNC_SETTING:
		sem_wait(&cxt->ae_sm);
		memcpy(&cxt->match_param.master_ae_info.ae_sync_result, in,
			sizeof(cxt->match_param.master_ae_info.ae_sync_result));
		sem_post(&cxt->ae_sm);
		break;
	case GET_MASTER_AESYNC_SETTING:
		sem_wait(&cxt->ae_sm);
		memcpy(out, &cxt->match_param.master_ae_info.ae_sync_result,
			sizeof(cxt->match_param.master_ae_info.ae_sync_result));
		sem_post(&cxt->ae_sm);
		break;
	case SET_MASTER_AECALC_RESULT:
		sem_wait(&cxt->ae_sm);
		memcpy(&cxt->match_param.master_ae_info.ae_calc_result, in,
			sizeof(cxt->match_param.master_ae_info.ae_calc_result));
		sem_post(&cxt->ae_sm);
		break;
	case GET_MASTER_AECALC_RESULT:
		sem_wait(&cxt->ae_sm);
		memcpy(out, &cxt->match_param.master_ae_info.ae_calc_result,
			sizeof(cxt->match_param.master_ae_info.ae_calc_result));
		sem_post(&cxt->ae_sm);
		break;
	case SET_ALL_MODULE_AND_OTP:
		ISP_LOGW("not implemented");
		break;
	case GET_ALL_MODULE_AND_OTP:
		sem_wait(&cxt->ae_sm);
		memcpy(out, &cxt->match_param.module_info, sizeof(cxt->match_param.module_info));
		sem_post(&cxt->ae_sm);
		break;
	}
	ISP_LOGV("X");

	return 0;
}

cmr_int isp_br_init(cmr_u32 camera_id, cmr_handle isp_3a_handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct ispbr_context *cxt = &br_cxt;

	ISP_LOGI("E is_master %d", camera_id);
	if(!camera_id) {
		cxt->isp_3afw_slave_handles = isp_3a_handle;
		br_cxt.match_param.slave_ae_info.ae_sync_status=SYNC_INIT;
	} else {
		cxt->isp_3afw_master_handles = isp_3a_handle;
		br_cxt.match_param.master_ae_info.ae_sync_status=SYNC_INIT;
	}
	pthread_mutex_lock(&g_br_mutex);
	cxt->user_cnt++;
	pthread_mutex_unlock(&g_br_mutex);
	ISP_LOGI("cnt = %d", cxt->user_cnt);
	if (1 == cxt->user_cnt) {
		sem_init(&cxt->ae_sm, 0, 1);
		sem_init(&cxt->ae_updata_sm, 0, 0);
		sem_init(&cxt->ae_update_sm2, 0, 1);
		sem_init(&cxt->awb_sm, 0, 1);
		sem_init(&cxt->module_sm, 0, 1);
		br_cxt.match_param.slave_ae_info.ae_sync_result.updata_flag = 0;
	}
	return ret;
}

cmr_int isp_br_deinit(cmr_u32 camera_id)
{
	cmr_int ret = ISP_SUCCESS;
	struct ispbr_context *cxt = &br_cxt;

	if(!camera_id) {
		cxt->isp_3afw_slave_handles = NULL;
		br_cxt.match_param.slave_ae_info.ae_sync_status=SYNC_DEINIT;
	} else {
		cxt->isp_3afw_master_handles = NULL;
		br_cxt.match_param.master_ae_info.ae_sync_status=SYNC_DEINIT;
	}
	pthread_mutex_lock(&g_br_mutex);
	cxt->user_cnt--;
	pthread_mutex_unlock(&g_br_mutex);
	ISP_LOGI("cnt = %d", cxt->user_cnt);
	if (0 == cxt->user_cnt) {
		sem_destroy(&cxt->ae_sm);
		sem_destroy(&cxt->awb_sm);
		sem_destroy(&cxt->ae_updata_sm);
		sem_destroy(&cxt->ae_update_sm2);
		sem_destroy(&cxt->module_sm);
		cxt->isp_3afw_slave_handles = NULL;
		cxt->isp_3afw_master_handles = NULL;
		br_cxt.match_param.slave_ae_info.ae_sync_result.updata_flag = 0;
	}

	return ret;
}

cmr_int isp_br_save_dual_otp(cmr_u32 camera_id, struct sensor_dual_otp_info *dual_otp)
{
	int32_t ret = ISP_SUCCESS;
	struct ispbr_context *cxt = &br_cxt;

	if (camera_id >= SENSOR_NUM_MAX) {
		ISP_LOGE("fail save camera_id %d dual otp", camera_id);
		ret = ISP_PARAM_ERROR;
		goto exit;
	}
	cxt->dual_otp[camera_id] = dual_otp;
exit:
	return ret;
}

cmr_int isp_br_get_dual_otp(cmr_u32 camera_id, struct sensor_dual_otp_info **dual_otp)
{
	int32_t ret = ISP_SUCCESS;
	struct ispbr_context *cxt = &br_cxt;

	if (camera_id >= SENSOR_NUM_MAX) {
		ISP_LOGE("fail get camera_id %d dual otp", camera_id);
		ret = ISP_PARAM_ERROR;
		goto exit;
	}
	*dual_otp = cxt->dual_otp[camera_id];
exit:
	return ret;
}
