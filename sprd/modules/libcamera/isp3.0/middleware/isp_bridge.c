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
#define LOG_TAG "isp_brigde"

#include "isp_bridge.h"

/**************************************** MACRO DEFINE *****************************************/
#define SENSOR_NUM_MAX 4

/************************************* INTERNAL DATA TYPE **************************************/
struct ispbr_context {
	cmr_u32 user_cnt;
	cmr_handle isp_3afw_handles[SENSOR_NUM_MAX];
	sem_t ae_sm;
	sem_t awb_sm;
	struct match_data_param match_param;
	struct sensor_dual_otp_info *dual_otp[SENSOR_NUM_MAX];
};

static struct ispbr_context br_cxt;
static pthread_mutex_t g_br_mutex = PTHREAD_MUTEX_INITIALIZER;

/*************************************INTERNAK FUNCTION ****************************************/

cmr_handle isp_br_get_3a_handle(cmr_u32 camera_id)
{
	if (camera_id >= SENSOR_NUM_MAX) {
		ISP_LOGE("fail camera_id %d", camera_id);
		return NULL;
	}
	return br_cxt.isp_3afw_handles[camera_id];
}

cmr_int isp_br_ioctrl(cmr_u32 camera_id, cmr_int cmd, void *in, void *out)
{
	struct ispbr_context *cxt = &br_cxt;

	if (camera_id >= SENSOR_NUM_MAX) {
		ISP_LOGE("fail camera_id %d", camera_id);
		return -ISP_PARAM_ERROR;
	}
	switch (cmd) {
	case GET_MATCH_AWB_DATA:
		sem_wait(&cxt->awb_sm);
		memcpy(out, &cxt->match_param.awb_data, sizeof(cxt->match_param.awb_data));
		sem_post(&cxt->awb_sm);
		break;
	case SET_MATCH_AWB_DATA:
		sem_wait(&cxt->awb_sm);
		memcpy(&cxt->match_param.awb_data, in, sizeof(cxt->match_param.awb_data));
		sem_post(&cxt->awb_sm);
		break;
	case GET_MATCH_AE_DATA:
		sem_wait(&cxt->ae_sm);
		memcpy(out, &cxt->match_param.ae_data, sizeof(cxt->match_param.ae_data));
		sem_post(&cxt->ae_sm);
		break;
	case SET_MATCH_AE_DATA:
		sem_wait(&cxt->ae_sm);
		memcpy(&cxt->match_param.ae_data, in, sizeof(cxt->match_param.ae_data));
		sem_post(&cxt->ae_sm);
		break;
	}

	return 0;
}

cmr_int isp_br_init(cmr_u32 camera_id, cmr_handle isp_3a_handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct ispbr_context *cxt = &br_cxt;

	if (camera_id >= SENSOR_NUM_MAX) {
		ISP_LOGE("fail camera_id %d", camera_id);
		return -ISP_PARAM_ERROR;
	}
	cxt->isp_3afw_handles[camera_id] = isp_3a_handle;

	pthread_mutex_lock(&g_br_mutex);
	cxt->user_cnt++;
	pthread_mutex_unlock(&g_br_mutex);
	ISP_LOGI("cnt = %d", cxt->user_cnt);
	if (1 == cxt->user_cnt) {
		sem_init(&cxt->ae_sm, 0, 1);
		sem_init(&cxt->awb_sm, 0, 1);
	}

	return ret;
}

cmr_int isp_br_deinit(cmr_u32 camera_id)
{
	cmr_int ret = ISP_SUCCESS;
	struct ispbr_context *cxt = &br_cxt;
	cmr_u8 i = 0;

	if (camera_id >= SENSOR_NUM_MAX) {
		ISP_LOGE("fail camera_id %d", camera_id);
		return -ISP_PARAM_ERROR;
	}
	cxt->isp_3afw_handles[camera_id] = NULL;
	pthread_mutex_lock(&g_br_mutex);
	cxt->user_cnt--;
	pthread_mutex_unlock(&g_br_mutex);
	ISP_LOGI("cnt = %d", cxt->user_cnt);
	if (0 == cxt->user_cnt) {
		sem_destroy(&cxt->ae_sm);
		sem_destroy(&cxt->awb_sm);
		for (i = 0; i < SENSOR_NUM_MAX; i++)
			cxt->isp_3afw_handles[i] = NULL;
	}

	return ret;
}

cmr_int isp_br_save_dual_otp(cmr_u32 camera_id, struct sensor_dual_otp_info *dual_otp)
{
	cmr_int ret = ISP_SUCCESS;
	struct ispbr_context *cxt = &br_cxt;

	if (camera_id >= SENSOR_NUM_MAX) {
		ISP_LOGE("fail save camera_id %d dual otp", camera_id);
		goto exit;
	}
	cxt->dual_otp[camera_id] = dual_otp;
exit:
	return ret;
}

cmr_int isp_br_get_dual_otp(cmr_u32 camera_id, struct sensor_dual_otp_info **dual_otp)
{
	cmr_int ret = ISP_SUCCESS;
	struct ispbr_context *cxt = &br_cxt;

	if (camera_id >= SENSOR_NUM_MAX) {
		ISP_LOGE("fail get camera_id %d dual otp", camera_id);
		goto exit;
	}
	*dual_otp = cxt->dual_otp[camera_id];
exit:
	return ret;
}
