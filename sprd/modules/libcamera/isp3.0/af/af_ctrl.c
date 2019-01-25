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
#define LOG_TAG "af_ctrl"

#include <stdlib.h>
#include "isp_type.h"
#include "af_ctrl.h"
#include "isp_adpt.h"
#include "cmr_msg.h"
#include "af_adpt.h"

#define SUPPORT_AF_THREAD

#define AFCTRL_MSG_QUEUE_SIZE			100
#define AFCTRL_EVT_BASE				0x2000
#define AFCTRL_EVT_INIT				AFCTRL_EVT_BASE
#define AFCTRL_EVT_DEINIT			(AFCTRL_EVT_BASE + 1)
#define AFCTRL_EVT_IOCTRL			(AFCTRL_EVT_BASE + 2)
#define AFCTRL_EVT_PROCESS			(AFCTRL_EVT_BASE + 3)
#define AFCTRL_EVT_EXIT				(AFCTRL_EVT_BASE + 4)

#define AFCTRL_VCM_EVT_BASE			(AFCTRL_EVT_BASE + 0x20)
#define AFCTRL_VCM_EVT_INIT			AFCTRL_VCM_EVT_BASE
#define AFCTRL_VCM_EVT_PROCESS			(AFCTRL_VCM_EVT_BASE + 1)
#define AFCTRL_VCM_EVT_EXIT			(AFCTRL_VCM_EVT_BASE + 2)

struct af_ctrl_thread_context {
	cmr_handle ctrl_thr_handle;
	cmr_handle vcm_thr_handle;
};

struct afctrl_context {
	cmr_u32 is_inited;
	cmr_int camera_id;
	cmr_u8 af_support;
	cmr_int af_bypass;
	//cmr_s32 frame_id;
	//enum af_ctrl_mode_type af_mode;
	//cmr_int sence_mode;
	//cmr_int flash_on;
	cmr_handle adpt_handle;
	cmr_handle caller_handle;
	sem_t sync_sm;
	struct af_ctrl_thread_context thread_cxt;
	//enum af_ctrl_scene_mode af_scene_mode;
	//struct af_ctrl_roi_info_type af_roi_info;
	//struct af_ctrl_sensor_info_type sensor_info;
	//struct af_ctrl_auxiliary_info auxiliary_info;
	struct af_ctrl_cb_ops_type cb_ops;
	struct adpt_ops_type *af_adpt_ops;
};

struct af_ctrl_msg_ctrl {
	cmr_int cmd;
	struct af_ctrl_param_in *in;
	struct af_ctrl_param_out *out;
};

struct af_ctrl_msg_proc {
	struct af_ctrl_process_in *in;
	struct af_ctrl_process_out *out;
};

static cmr_int afctrl_evtctrl(cmr_handle handle, cmr_int cmd,
			      struct af_ctrl_param_in *in,
			      struct af_ctrl_param_out *out);

static cmr_int afctrl_start_notify(cmr_handle handle, void *data)
{
	cmr_int ret = -ISP_ERROR;
	struct afctrl_context *cxt = (struct afctrl_context *)handle;

	UNUSED(data);
	if (cxt->cb_ops.start_notify) {
		ret = cxt->cb_ops.start_notify(cxt->caller_handle, NULL);
	} else {
		ISP_LOGE("cb is null");
		ret = -ISP_CALLBACK_NULL;
	}

	return ret;
}

static cmr_int afctrl_end_notify(cmr_handle handle,
				 struct af_result_param *data)
{
	cmr_int ret = -ISP_ERROR;
	struct afctrl_context *cxt = (struct afctrl_context *)handle;

	if (cxt->cb_ops.end_notify) {
		ret = cxt->cb_ops.end_notify(cxt->caller_handle, data);
	} else {
		ISP_LOGE("cb is null");
		ret = -ISP_CALLBACK_NULL;
	}

	return ret;
}

static cmr_int afctrl_lock_ae_awb(cmr_handle handle, void *data)
{
	cmr_int ret = -ISP_ERROR;
	struct afctrl_context *cxt = (struct afctrl_context *)handle;

	if (cxt->cb_ops.lock_ae_awb) {
		ret = cxt->cb_ops.lock_ae_awb(cxt->caller_handle, data);
	} else {
		ISP_LOGE("cb is null");
		ret = -ISP_CALLBACK_NULL;
	}

	return ret;
}

static cmr_int afctrl_config_pdaf_enable(cmr_handle handle, void *data)
{
	cmr_int ret = -ISP_ERROR;
	struct afctrl_context *cxt = (struct afctrl_context *)handle;

	if (cxt->cb_ops.cfg_pdaf_enable) {
		ret = cxt->cb_ops.cfg_pdaf_enable(cxt->caller_handle, data);
	} else {
		ISP_LOGE("cb is null");
		ret = -ISP_CALLBACK_NULL;
	}

	return ret;
}

static cmr_int afctrl_config_pdaf_config(cmr_handle handle, void *data)
{
	cmr_int ret = -ISP_ERROR;
	struct afctrl_context *cxt = (struct afctrl_context *)handle;

	if (cxt->cb_ops.cfg_pdaf_config) {
		ret = cxt->cb_ops.cfg_pdaf_config(cxt->caller_handle, data);
	} else {
		ISP_LOGE("cb is null");
		ret = -ISP_CALLBACK_NULL;
	}

	return ret;
}

static cmr_int afctrl_config_af_stats(cmr_handle handle, void *data)
{
	cmr_int ret = -ISP_ERROR;
	struct afctrl_context *cxt = (struct afctrl_context *)handle;

	ISP_LOGV("E");
	if (cxt->cb_ops.cfg_af_stats) {
		ret = cxt->cb_ops.cfg_af_stats(cxt->caller_handle, data);
	} else {
		ISP_LOGE("cb is null");
		ret = -ISP_CALLBACK_NULL;
	}

	return ret;
}

static cmr_int afctrl_get_timestamp(cmr_handle handle, cmr_u32 *sec, cmr_u32 *usec)
{
	cmr_int ret = -ISP_ERROR;
	struct afctrl_context *cxt = (struct afctrl_context *)handle;

	if (cxt->cb_ops.get_system_time) {
		ret = cxt->cb_ops.get_system_time(cxt->caller_handle, sec, usec);
	} else {
		ISP_LOGE("cb is null");
		ret = -ISP_CALLBACK_NULL;
	}

	return ret;
}

static cmr_int afctrl_vcm_process(cmr_handle handle, void *pos)
{
	cmr_int ret = -ISP_ERROR;
	struct afctrl_context *cxt = (struct afctrl_context *)handle;
	struct af_ctrl_motor_pos *pos_info = (struct af_ctrl_motor_pos *)pos;
	struct af_ctrl_param_in in;

	cmr_bzero(&in, sizeof(in));

	in.pos_info = *pos_info;

	ISP_LOGI("motor_pos %d offset %d wait ms %d",
		in.pos_info.motor_pos, in.pos_info.motor_offset, in.pos_info.vcm_wait_ms);

	/* set pos via fw callback */
	if (cxt->cb_ops.set_pos) {
		ret = cxt->cb_ops.set_pos(cxt->caller_handle, pos_info);
	} else {
		ISP_LOGE("cb is null");
		ret = -ISP_CALLBACK_NULL;
	}
	/* wait for stabilizing vcm form tuning param */
	cmr_msleep(pos_info->vcm_wait_ms);

	/* notify lib vcm moving finished */
	ret = afctrl_evtctrl(cxt, AF_CTRL_CMD_SET_AF_POS_DONE, &in, NULL);
	if (ret)
		ISP_LOGI("ret = %ld", ret);
	return ret;
}

static cmr_int afctrl_set_pos(cmr_handle handle, struct af_ctrl_motor_pos *pos)
{
	cmr_int ret = -ISP_ERROR;
	struct afctrl_context *cxt = (struct afctrl_context *)handle;
	struct af_ctrl_motor_pos *pos_info = NULL;
	CMR_MSG_INIT(message);

	pos_info = (struct af_ctrl_motor_pos *)malloc(sizeof(*pos_info));

	if (!pos_info) {
		ISP_LOGE("failed to malloc pos info");
		ret = -ISP_ALLOC_ERROR;
		goto error_malloc;
	}
	*pos_info = *pos;

	message.msg_type = AFCTRL_VCM_EVT_PROCESS;
	message.sync_flag = CMR_MSG_SYNC_NONE;
	message.alloc_flag = 1;
	message.data = (void *)pos_info;
	ret = cmr_thread_msg_send(cxt->thread_cxt.vcm_thr_handle, &message);
	if (ret) {
		ISP_LOGE("failed to send msg to main thr %ld", ret);
		goto exit;
	}

	return ret;
exit:
	free(pos_info);
error_malloc:
	return ret;
}

static cmr_int afctrl_evtctrl(cmr_handle handle, cmr_int cmd,
			      struct af_ctrl_param_in *in,
			      struct af_ctrl_param_out *out)
{
	cmr_int ret = -ISP_ERROR;
	struct afctrl_context *cxt = (struct afctrl_context *)handle;

	sem_wait(&cxt->sync_sm);
	if (cxt->is_inited)
		ret = cxt->af_adpt_ops->adpt_ioctrl(cxt->adpt_handle, cmd, in, out);
	sem_post(&cxt->sync_sm);
	return ret;
}

static cmr_int afctrl_evtprocess(cmr_handle handle,
				 struct af_ctrl_process_in *in,
				 struct af_ctrl_process_out *out)
{
	cmr_int ret = -ISP_ERROR;
	struct afctrl_context *cxt = (struct afctrl_context *)handle;

	if (cxt->is_inited)
		ret = cxt->af_adpt_ops->adpt_process(cxt->adpt_handle, in, out);
	return ret;
}

static cmr_int afctrl_thread_proc(struct cmr_msg *message, void *p_data)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_handle handle = (cmr_handle) p_data;
	struct afctrl_context *cxt = (struct afctrl_context *)handle;
	struct af_ctrl_msg_ctrl *msg_ctrl = NULL;
	struct af_ctrl_msg_proc *msg_proc = NULL;

	if (!message || !p_data) {
		ISP_LOGE("param error message = %p, p_data = %p", message, p_data);
		ret = -ISP_PARAM_NULL;
		goto exit;
	}
	ISP_LOGV("message.msg_type 0x%x, data %p", message->msg_type, message->data);

	switch (message->msg_type) {
	case AFCTRL_EVT_INIT:
		break;
	case AFCTRL_EVT_DEINIT:
		break;
	case AFCTRL_EVT_EXIT:
		break;
	case AFCTRL_EVT_IOCTRL:
		msg_ctrl = (struct af_ctrl_msg_ctrl *)message->data;
		ret = afctrl_evtctrl(handle, msg_ctrl->cmd,
				   msg_ctrl->in, msg_ctrl->out);
		break;
	case AFCTRL_EVT_PROCESS:
		msg_proc = (struct af_ctrl_msg_proc *)message->data;
		ret = afctrl_evtprocess(handle, msg_proc->in,
					msg_proc->out);
		break;
	default:
		ISP_LOGE("don't support msg");
		break;
	}
exit:
	ISP_LOGV("done %ld", ret);
	return ret;
}

static cmr_int afctrl_create_thread(cmr_handle handle)
{
	cmr_int ret = -ISP_ERROR;
	struct afctrl_context *cxt = (struct afctrl_context *)handle;

	ret = cmr_thread_create(&cxt->thread_cxt.ctrl_thr_handle,
				AFCTRL_MSG_QUEUE_SIZE,
				afctrl_thread_proc,
				(void *)handle);
	ISP_LOGV("%p", cxt->thread_cxt.ctrl_thr_handle);
	if (CMR_MSG_SUCCESS != ret) {
		ISP_LOGE("failed to create main thread %ld", ret);
		ret = ISP_ERROR;
	}

	ISP_LOGV("done %ld", ret);
	return ret;
}

static cmr_int afctrl_destroy_thread(cmr_handle handle)
{
	cmr_int ret = -ISP_ERROR;
	struct afctrl_context *cxt = (struct afctrl_context *)handle;
	struct af_ctrl_thread_context *af_thread_cxt;

	af_thread_cxt = &cxt->thread_cxt;
	if (af_thread_cxt->ctrl_thr_handle) {
		ret = cmr_thread_destroy(af_thread_cxt->ctrl_thr_handle);
		if (ret) {
			ISP_LOGE("failed to destroy ctrl thread %ld", ret);
		}
		af_thread_cxt->ctrl_thr_handle = (cmr_handle) NULL;
	}
exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

static cmr_int afctrl_vcm_thread_proc(struct cmr_msg *message, void *p_data)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_handle handle = (cmr_handle) p_data;
	struct afctrl_context *cxt = (struct afctrl_context *)handle;

	if (!message || !p_data) {
		ISP_LOGE("param error message = %p, p_data = %p", message, p_data);
		ret = -ISP_PARAM_NULL;
		goto exit;
	}
	ISP_LOGV("message.msg_type 0x%x, data %p", message->msg_type, message->data);

	switch (message->msg_type) {
	case AFCTRL_VCM_EVT_INIT:
		break;
	case AFCTRL_VCM_EVT_EXIT:
		break;
	case AFCTRL_VCM_EVT_PROCESS:
		ret = afctrl_vcm_process(handle, message->data);
		break;
	default:
		ISP_LOGE("don't support msg");
		break;
	}
exit:
	ISP_LOGV("done %ld", ret);
	return ret;
}

static cmr_int afctrl_create_vcm_thread(cmr_handle handle)
{
	cmr_int ret = -ISP_ERROR;
	struct afctrl_context *cxt = (struct afctrl_context *)handle;

	ret = cmr_thread_create(&cxt->thread_cxt.vcm_thr_handle,
				AFCTRL_MSG_QUEUE_SIZE,
				afctrl_vcm_thread_proc,
				(void *)handle);
	ISP_LOGV("%p", cxt->thread_cxt.vcm_thr_handle);
	if (CMR_MSG_SUCCESS != ret) {
		ISP_LOGE("failed to create vcm thread %ld", ret);
	}

	ISP_LOGV("done %ld", ret);
	return ret;
}

static cmr_int afctrl_destroy_vcm_thread(cmr_handle handle)
{
	cmr_int ret = -ISP_ERROR;
	struct afctrl_context *cxt = (struct afctrl_context *)handle;
	struct af_ctrl_thread_context *af_thread_cxt;

	af_thread_cxt = &cxt->thread_cxt;
	if (af_thread_cxt->vcm_thr_handle) {
		ret = cmr_thread_destroy(af_thread_cxt->vcm_thr_handle);
		if (ret) {
			ISP_LOGE("failed to destroy ctrl thread %ld", ret);
		}
		af_thread_cxt->vcm_thr_handle = (cmr_handle) NULL;
	}
exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int af_ctrl_init(struct af_ctrl_init_in *in,
		     struct af_ctrl_init_out *out, cmr_handle *handle)
{
	cmr_int ret = -ISP_ERROR;
	/* create handle */
	struct afctrl_context *cxt = NULL;
	struct af_adpt_init_in adpt_in;

	cxt = (struct afctrl_context *)malloc(sizeof(*cxt));
	if (NULL == cxt) {
		ISP_LOGE("failed to malloc cxt");
		ret = -ISP_CALLBACK_NULL;
		goto error_malloc;
	}

	/* init param */
	cmr_bzero(cxt, sizeof(*cxt));
	cxt->camera_id = in->camera_id;
	cxt->af_support = in->af_support;
	if (!cxt->af_support) {
		ISP_LOGI("this module isnot support af");
		ret = ISP_SUCCESS;
		goto sucess_exit;
	}
	/* find vendor adpter */
	ret = adpt_get_ops(ADPT_LIB_AF, &in->af_lib_info, &cxt->af_adpt_ops);
	if (ret) {
		ISP_LOGE("failed to get adapter layer ret = %ld", ret);
		goto error_get_adpt;
	}
	sem_init(&cxt->sync_sm, 0, 1);
	cxt->caller_handle = in->caller_handle;
	cxt->cb_ops = in->af_ctrl_cb_ops;

	cmr_bzero(&adpt_in, sizeof(adpt_in));
	adpt_in.ctrl_in = in;
	adpt_in.caller_handle = cxt;
	adpt_in.cb_ctrl_ops.set_pos = afctrl_set_pos;
	adpt_in.cb_ctrl_ops.start_notify = afctrl_start_notify;
	adpt_in.cb_ctrl_ops.end_notify = afctrl_end_notify;
	adpt_in.cb_ctrl_ops.lock_ae_awb = afctrl_lock_ae_awb;
	adpt_in.cb_ctrl_ops.cfg_af_stats = afctrl_config_af_stats;
	adpt_in.cb_ctrl_ops.cfg_pdaf_enable = afctrl_config_pdaf_enable;
	adpt_in.cb_ctrl_ops.cfg_pdaf_config = afctrl_config_pdaf_config;
	adpt_in.cb_ctrl_ops.get_system_time = afctrl_get_timestamp;

#ifdef SUPPORT_AF_THREAD
	/* create thread */
	ret = afctrl_create_thread((cmr_handle) cxt);
	if (ret) {
		ISP_LOGE("failed to create thread ret = %ld", ret);
		goto error_main_thread;
	}
#endif
	/* create vcm thread */
	ret = afctrl_create_vcm_thread((cmr_handle) cxt);
	if (ret) {
		ISP_LOGE("failed to create thread ret = %ld", ret);
		goto error_vcm_thread;
	}

	sem_wait(&cxt->sync_sm);
	/* adpter init */
	ret = cxt->af_adpt_ops->adpt_init(&adpt_in, out, &cxt->adpt_handle);
	if (ret) {
		sem_post(&cxt->sync_sm);
		ISP_LOGE("failed to init adapter layer ret = %ld", ret);
		goto error_init_adpt;
	}
	sem_post(&cxt->sync_sm);

sucess_exit:
	*handle = (cmr_handle) cxt;
	cxt->is_inited = 1;
	return ret;

	cxt->af_adpt_ops->adpt_deinit(cxt->adpt_handle);
error_init_adpt:
	afctrl_destroy_vcm_thread(cxt);
error_vcm_thread:
#ifdef SUPPORT_AF_THREAD
	afctrl_destroy_thread(cxt);
error_main_thread:
#endif
	sem_destroy(&cxt->sync_sm);
error_get_adpt:
	free(cxt);
	cxt = NULL;
error_malloc:
	return ret;
}

cmr_int af_ctrl_deinit(cmr_handle handle)
{
	cmr_int ret = -ISP_ERROR;
	struct afctrl_context *cxt = (struct afctrl_context *)handle;

	if (!handle) {
		ISP_LOGE("handle is null");
		goto exit;
	}
	if (!cxt->af_support) {
		ret = ISP_SUCCESS;
		goto sucess_exit;
	}

	ret = afctrl_destroy_vcm_thread(cxt);
	if (ret)
		ISP_LOGE("failed to destroy vcm thread ret = %ld", ret);

#ifdef SUPPORT_AF_THREAD
	ret = afctrl_destroy_thread(cxt);
	if (ret)
		ISP_LOGE("failed to destroy thread ret = %ld", ret);
#endif
	sem_wait(&cxt->sync_sm);
	ret = cxt->af_adpt_ops->adpt_deinit(cxt->adpt_handle);
	if (ret)
		ISP_LOGE("failed to deinit adapter layer ret = %ld", ret);
	cxt->is_inited = 0;
	sem_post(&cxt->sync_sm);
	sem_destroy(&cxt->sync_sm);

sucess_exit:
	cmr_bzero(cxt, sizeof(*cxt));
	/* free handle */
	free(cxt);
	cxt = NULL;
exit:
	return 0;
}

cmr_int af_ctrl_process(cmr_handle handle, struct af_ctrl_process_in *in,
			struct af_ctrl_process_out *out)
{
	cmr_int ret = ISP_SUCCESS;
	struct afctrl_context *cxt = (struct afctrl_context *)handle;
#ifdef SUPPORT_AF_THREAD
	struct af_ctrl_msg_proc msg_proc;
	CMR_MSG_INIT(message);

	if (!cxt->af_support)
		goto exit;
	msg_proc.in = in;
	msg_proc.out = out;

	message.msg_type = AFCTRL_EVT_PROCESS;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	/* if use SYNC_NONE, please modify stats release */
	//message.sync_flag = CMR_MSG_SYNC_NONE;
	message.alloc_flag = 0;
	message.data = &msg_proc;
	ret = cmr_thread_msg_send(cxt->thread_cxt.ctrl_thr_handle, &message);
	if (ret) {
		ISP_LOGE("failed to send msg to main thr %ld", ret);
		goto exit;
	}
exit:
	return ret;
#else
	if (cxt->af_support)
		ret = afctrl_evtprocess(handle, in, out);
	else
		ret = ISP_SUCCESS;

	return ret;
#endif
}

cmr_int af_ctrl_ioctrl(cmr_handle handle, cmr_int cmd,
		       struct af_ctrl_param_in *in,
		       struct af_ctrl_param_out *out)
{
	cmr_int ret = -ISP_ERROR;
	struct afctrl_context *cxt = (struct afctrl_context *)handle;
#ifdef SUPPORT_AF_THREAD
	struct af_ctrl_msg_ctrl msg_ctrl;
	CMR_MSG_INIT(message);

	if (!cxt->af_support) {
		ret = ISP_SUCCESS;
		goto exit;
	}
	msg_ctrl.cmd = cmd;
	msg_ctrl.in = in;
	msg_ctrl.out = out;
	if (0) {//{AF_CTRL_CMD_SET_PROC_START == cmd) {
		message.data = malloc(sizeof(msg_ctrl));
		message.msg_type = AFCTRL_EVT_IOCTRL;
		message.sync_flag = CMR_MSG_SYNC_NONE;
		message.alloc_flag = 1;
		memcpy(message.data, &msg_ctrl, sizeof(msg_ctrl));
	} else {
		message.msg_type = AFCTRL_EVT_IOCTRL;
		message.sync_flag = CMR_MSG_SYNC_PROCESSED;
		message.alloc_flag = 0;
		message.data = &msg_ctrl;
	}
	ret = cmr_thread_msg_send(cxt->thread_cxt.ctrl_thr_handle, &message);
	if (ret) {
		ISP_LOGE("failed to send msg to main thr %ld", ret);
		goto exit;
	}
exit:
	return ret;
#else
	if (cxt->af_support)
		ret = afctrl_evtctrl(handle, cmd, in, out);
	else
		ret = ISP_SUCCESS;

	return ret;
#endif
}
