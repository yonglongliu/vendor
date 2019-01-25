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
#define LOG_TAG "pdaf_ctrl"

#include <stdlib.h>
#include "pdaf_ctrl.h"
#include "isp_adpt.h"
#include "cmr_msg.h"

#define PDAFCTRL_MSG_QUEUE_SIZE			100
#define PDAFCTRL_EVT_BASE				0x2000
#define PDAFCTRL_EVT_INIT				PDAFCTRL_EVT_BASE
#define PDAFCTRL_EVT_DEINIT				(PDAFCTRL_EVT_BASE + 1)
#define PDAFCTRL_EVT_IOCTRL				(PDAFCTRL_EVT_BASE + 2)
#define PDAFCTRL_EVT_PROCESS				(PDAFCTRL_EVT_BASE + 3)
#define PDAFCTRL_EVT_EXIT				(PDAFCTRL_EVT_BASE + 4)

struct pdaf_ctrl_thread_context {
	cmr_handle ctrl_thr_handle;
};

struct pdafctrl_context {
	cmr_int camera_id;
	cmr_u8 pdaf_support;
	cmr_int pdaf_bypass;
	cmr_handle adpt_handle;
	cmr_handle caller_handle;
	struct pdaf_ctrl_thread_context thread_cxt;
	struct pdaf_ctrl_cb_ops_type cb_ops;
	struct adpt_ops_type *pdaf_adpt_ops;
	isp_pdaf_cb pdaf_set_cb;
	struct pdafctrl_work_lib work_lib;
};

struct pdaf_init_msg_ctrl {
	struct pdaf_ctrl_init_in *in;
	struct pdaf_ctrl_init_out *out;
};

struct pdaf_ctrl_msg_ctrl {
	cmr_int cmd;
	struct pdaf_ctrl_param_in *in;
	struct pdaf_ctrl_param_out *out;
};

static cmr_u32 pdaf_set_pdinfo_to_af(void *handle, struct pd_result *in_param)
{
	struct pdafctrl_context *cxt_ptr = (struct pdafctrl_context *)handle;

	if (cxt_ptr->pdaf_set_cb) {
		cxt_ptr->pdaf_set_cb(cxt_ptr->caller_handle, ISP_AF_SET_PD_INFO, in_param, NULL);
	}

	return ISP_SUCCESS;
}

static cmr_u32 pdaf_set_cfg_param(void *handle, struct isp_dev_pdaf_info *in_param)
{
	struct pdafctrl_context *cxt_ptr = (struct pdafctrl_context *)handle;

	ISP_LOGI("pdaf_set_cfg_param");
	if (cxt_ptr->pdaf_set_cb) {
		cxt_ptr->pdaf_set_cb(cxt_ptr->caller_handle, ISP_PDAF_SET_CFG_PARAM, in_param, NULL);
	}

	return ISP_SUCCESS;
}

static cmr_u32 pdaf_set_bypass(void *handle, cmr_u32 in_param)
{
	struct pdafctrl_context *cxt_ptr = (struct pdafctrl_context *)handle;

	if (cxt_ptr->pdaf_set_cb) {
		cxt_ptr->pdaf_set_cb(cxt_ptr->caller_handle, ISP_PDAF_SET_BYPASS, &in_param, NULL);
	}

	return ISP_SUCCESS;
}

static cmr_u32 pdaf_set_work_mode(void *handle, cmr_u32 in_param)
{
	struct pdafctrl_context *cxt_ptr = (struct pdafctrl_context *)handle;

	if (cxt_ptr->pdaf_set_cb) {
		cxt_ptr->pdaf_set_cb(cxt_ptr->caller_handle, ISP_PDAF_SET_WORK_MODE, &in_param, NULL);
	}

	return ISP_SUCCESS;
}

static cmr_u32 pdaf_set_skip_num(void *handle, cmr_u32 in_param)
{
	struct pdafctrl_context *cxt_ptr = (struct pdafctrl_context *)handle;

	if (cxt_ptr->pdaf_set_cb) {
		cxt_ptr->pdaf_set_cb(cxt_ptr->caller_handle, ISP_PDAF_SET_SKIP_NUM, &in_param, NULL);
	}

	return ISP_SUCCESS;
}

static cmr_u32 pdaf_set_ppi_info(void *handle, struct pdaf_ppi_info *in_param)
{
	struct pdafctrl_context *cxt_ptr = (struct pdafctrl_context *)handle;

	if (cxt_ptr->pdaf_set_cb) {
		cxt_ptr->pdaf_set_cb(cxt_ptr->caller_handle, ISP_PDAF_SET_PPI_INFO, in_param, NULL);
	}

	return ISP_SUCCESS;
}

static cmr_u32 pdaf_set_roi(void *handle, struct pdaf_roi_info *in_param)
{
	struct pdafctrl_context *cxt_ptr = (struct pdafctrl_context *)handle;

	if (cxt_ptr->pdaf_set_cb) {
		cxt_ptr->pdaf_set_cb(cxt_ptr->caller_handle, ISP_PDAF_SET_ROI, in_param, NULL);
	}

	return ISP_SUCCESS;
}

static cmr_u32 pdaf_set_extractor_bypass(void *handle, cmr_u32 in_param)
{
	struct pdafctrl_context *cxt_ptr = (struct pdafctrl_context *)handle;

	if (cxt_ptr->pdaf_set_cb) {
		cxt_ptr->pdaf_set_cb(cxt_ptr->caller_handle, ISP_PDAF_SET_EXTRACTOR_BYPASS, &in_param, NULL);
	}

	return ISP_SUCCESS;
}

static cmr_int pdafctrl_init_adpt(cmr_handle handle, struct pdaf_ctrl_init_in *in, struct pdaf_ctrl_init_out *out)
{
	cmr_int ret = ISP_SUCCESS;
	struct pdafctrl_context *cxt_ptr = (struct pdafctrl_context *)handle;
	struct pdafctrl_work_lib *lib_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param is NULL!");
		goto exit;
	}

	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_init) {
		lib_ptr->lib_handle = lib_ptr->adpt_ops->adpt_init(in, out);
	} else {
		ISP_LOGI("adpt_init fun is NULL");
	}

  exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

static cmr_int pdafctrl_deinit_adpt(cmr_handle handle)
{
	cmr_int rtn = ISP_SUCCESS;
	struct pdafctrl_context *cxt_ptr = (struct pdafctrl_context *)handle;
	struct pdafctrl_work_lib *lib_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param is NULL!");
		goto exit;
	}

	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_deinit) {
		rtn = lib_ptr->adpt_ops->adpt_deinit(lib_ptr->lib_handle, NULL, NULL);
		lib_ptr->lib_handle = NULL;
	} else {
		ISP_LOGI("adpt_deinit fun is NULL");
	}

  exit:
	ISP_LOGI("done %ld", rtn);
	return rtn;
}

static cmr_int pdafctrl_ioctrl(cmr_handle handle, cmr_int cmd, struct pdaf_ctrl_param_in *in, struct pdaf_ctrl_param_out *out)
{
	cmr_int rtn = ISP_SUCCESS;
	struct pdafctrl_context *cxt_ptr = (struct pdafctrl_context *)handle;
	struct pdafctrl_work_lib *lib_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param!");
		goto exit;
	}

	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_ioctrl) {
		rtn = lib_ptr->adpt_ops->adpt_ioctrl(lib_ptr->lib_handle, cmd, in, out);
	} else {
		ISP_LOGI("ioctrl fun is NULL");
	}

  exit:
	ISP_LOGI("cmd = %ld,done %ld", cmd, rtn);
	return rtn;

}

static cmr_int pdafctrl_process(cmr_handle handle, struct pdaf_ctrl_process_in *in, struct pdaf_ctrl_process_out *out)
{
	cmr_int rtn = ISP_SUCCESS;
	struct pdafctrl_context *cxt_ptr = (struct pdafctrl_context *)handle;
	struct pdafctrl_work_lib *lib_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param is NULL!");
		goto exit;
	}
	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_process) {
		rtn = lib_ptr->adpt_ops->adpt_process(lib_ptr->lib_handle, in, out);
	} else {
		ISP_LOGI("ioctrl fun is NULL");
	}
  exit:
	ISP_LOGI("done %ld", rtn);
	return rtn;

}

static cmr_int pdafctrl_thread_proc(struct cmr_msg *message, void *p_data)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_handle handle = (cmr_handle) p_data;
	struct pdaf_ctrl_msg_ctrl *msg_ctrl = NULL;
	struct pdaf_init_msg_ctrl *msg_init = NULL;

	if (!message || !p_data) {
		ISP_LOGE("param error message = %p, p_data = %p", message, p_data);
		ret = -ISP_PARAM_NULL;
		goto exit;
	}
	ISP_LOGV("message.msg_type 0x%x, data %p", message->msg_type, message->data);

	switch (message->msg_type) {
	case PDAFCTRL_EVT_INIT:
		msg_init = (struct pdaf_init_msg_ctrl *)message->data;
		ret = pdafctrl_init_adpt(handle, msg_init->in, msg_init->out);
		break;
	case PDAFCTRL_EVT_DEINIT:
		ret = pdafctrl_deinit_adpt(handle);
		break;
	case PDAFCTRL_EVT_EXIT:
		break;
	case PDAFCTRL_EVT_IOCTRL:
		msg_ctrl = (struct pdaf_ctrl_msg_ctrl *)message->data;
		ret = pdafctrl_ioctrl(handle, msg_ctrl->cmd, msg_ctrl->in, msg_ctrl->out);
		break;
	case PDAFCTRL_EVT_PROCESS:
		ret = pdafctrl_process(handle, (struct pdaf_ctrl_process_in *)message->data, NULL);
		break;
	default:
		ISP_LOGE("fail to proc,don't support msg");
		break;
	}
  exit:
	ISP_LOGV("done %ld", ret);
	return ret;
}

static cmr_int pdafctrl_create_thread(cmr_handle handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct pdafctrl_context *cxt = (struct pdafctrl_context *)handle;

	ret = cmr_thread_create(&cxt->thread_cxt.ctrl_thr_handle, PDAFCTRL_MSG_QUEUE_SIZE, pdafctrl_thread_proc, (void *)handle);
	if (CMR_MSG_SUCCESS != ret) {
		ISP_LOGE("fail to create main thread %ld", ret);
		ret = ISP_ERROR;
		goto exit;
	}
	ret = cmr_thread_set_name(cxt->thread_cxt.ctrl_thr_handle, "pdafctrl");
	if (CMR_MSG_SUCCESS != ret) {
		ISP_LOGE("fail to set pdafctrl name");
		ret = CMR_MSG_SUCCESS;
	}
  exit:
	ISP_LOGV("done %ld", ret);
	return ret;
}

static cmr_int pdafctrl_destroy_thread(cmr_handle handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct pdafctrl_context *cxt = (struct pdafctrl_context *)handle;
	struct pdaf_ctrl_thread_context *pdaf_thread_cxt;

	pdaf_thread_cxt = &cxt->thread_cxt;
	if (pdaf_thread_cxt->ctrl_thr_handle) {
		ret = cmr_thread_destroy(pdaf_thread_cxt->ctrl_thr_handle);
		if (ret) {
			ISP_LOGE("fail to destroy ctrl thread %ld", ret);
		}
		pdaf_thread_cxt->ctrl_thr_handle = NULL;
	}
  exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

static cmr_int pdafctrl_init_adapt(struct pdafctrl_context *cxt, struct pdaf_ctrl_init_in *in, struct pdaf_ctrl_init_out *out)
{
	cmr_int ret = ISP_SUCCESS;
	CMR_MSG_INIT(message);
	struct pdaf_init_msg_ctrl msg_init;

	msg_init.in = in;
	msg_init.out = out;
	message.msg_type = PDAFCTRL_EVT_INIT;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	message.alloc_flag = 0;
	message.data = &msg_init;
	ret = cmr_thread_msg_send(cxt->thread_cxt.ctrl_thr_handle, &message);
	if (ret) {
		ISP_LOGE("fail to send msg to main thr %ld", ret);
		goto exit;
	}

  exit:
	ISP_LOGI("done ret = %ld", ret);
	return ret;
}

cmr_int pdaf_ctrl_init(struct pdaf_ctrl_init_in * in, struct pdaf_ctrl_init_out * out, cmr_handle * handle)
{
	cmr_int ret = ISP_SUCCESS;
	/* create handle */
	struct pdafctrl_context *cxt = NULL;

	if (!in || !handle) {
		ISP_LOGE("fail to check init param, input_ptr is %p", in);
		ret = ISP_PARAM_NULL;
		goto exit;
	}

	in->pdaf_set_pdinfo_to_af = pdaf_set_pdinfo_to_af;
	in->pdaf_set_cfg_param = pdaf_set_cfg_param;
	in->pdaf_set_bypass = pdaf_set_bypass;
	in->pdaf_set_work_mode = pdaf_set_work_mode;
	in->pdaf_set_skip_num = pdaf_set_skip_num;
	in->pdaf_set_ppi_info = pdaf_set_ppi_info;
	in->pdaf_set_roi = pdaf_set_roi;
	in->pdaf_set_extractor_bypass = pdaf_set_extractor_bypass;

	*handle = NULL;
	cxt = (struct pdafctrl_context *)malloc(sizeof(*cxt));
	if (NULL == cxt) {
		ISP_LOGE("fail to malloc cxt");
		ret = ISP_ALLOC_ERROR;
		goto exit;
	}

	/* init param */
	memset((void *)cxt, 0x00, sizeof(*cxt));
	cxt->camera_id = in->camera_id;
	cxt->pdaf_support = in->pdaf_support;
	ISP_LOGI("cxt->pdaf_support = %d", cxt->pdaf_support);
	//cxt->init_in_param = *in;
	if (!cxt->pdaf_support) {
		ISP_LOGI("this module isnot support pdaf");
		ret = ISP_SUCCESS;
		goto sucess_exit;
	}

	/* find vendor adpter */
	ret = adpt_get_ops(ADPT_LIB_PDAF, &in->lib_param, &cxt->work_lib.adpt_ops);

	if (ret) {
		ISP_LOGE("fail to get adapter layer ret = %ld", ret);
		goto exit;
	}
	in->caller = (void *)cxt;
	cxt->pdaf_set_cb = in->pdaf_set_cb;
	cxt->caller_handle = in->caller_handle;
	/* create thread */
	ret = pdafctrl_create_thread((cmr_handle) cxt);

	if (ret) {
		ISP_LOGE("fail to create thread ret = %ld", ret);
		goto exit;
	}
	/* adpter init */
	ret = pdafctrl_init_adapt(cxt, in, out);

	if (ret) {
		ISP_LOGE("fail to init adapter layer ret = %ld", ret);
		goto error_adpt_init;
	}
  sucess_exit:
	ISP_LOGI("done ret=%ld", ret);
	*handle = (cmr_handle) cxt;
	return ret;
  error_adpt_init:

	pdafctrl_destroy_thread(cxt);
  exit:
	if (cxt) {
		free(cxt);
		cxt = NULL;
	}
	return ret;
}

cmr_int pdaf_ctrl_deinit(cmr_handle * handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct pdafctrl_context *cxt_ptr = *handle;
	CMR_MSG_INIT(message);

	if (!cxt_ptr) {
		ISP_LOGE("fail to deinit, handle_pdaf is NULL");
		return -ISP_ERROR;
	}
	if (!cxt_ptr->pdaf_support) {
		ret = ISP_SUCCESS;
		goto exit;
	}
	message.msg_type = PDAFCTRL_EVT_DEINIT;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	message.alloc_flag = 0;
	message.data = NULL;
	ret = cmr_thread_msg_send(cxt_ptr->thread_cxt.ctrl_thr_handle, &message);
	if (ret) {
		ISP_LOGE("fail to send msg to main thr %ld", ret);
	}
	ret = pdafctrl_destroy_thread(cxt_ptr);
	if (ret)
		ISP_LOGE("fail to destroy thread ret = %ld", ret);

  exit:
	if (cxt_ptr) {
		free((void *)cxt_ptr);
		*handle = NULL;
	}
	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int pdaf_ctrl_process(cmr_handle handle, struct pdaf_ctrl_process_in * in, struct pdaf_ctrl_process_out * out)
{
	cmr_int ret = ISP_SUCCESS;
	struct pdafctrl_context *cxt = (struct pdafctrl_context *)handle;
	CMR_MSG_INIT(message);
	ISP_CHECK_HANDLE_VALID(handle);
	UNUSED(out);
	if (!in) {
		ISP_LOGE("fail to check param,input param %p !!!", in);
		goto exit;
	}

	if (!cxt->pdaf_support) {
		ret = ISP_SUCCESS;
		goto exit;
	}

	message.data = malloc(sizeof(*in));
	if (!message.data) {
		ISP_LOGE("fail to malloc msg");
		ret = -ISP_ALLOC_ERROR;
		goto exit;
	}
	memcpy(message.data, in, sizeof(*in));
	message.alloc_flag = 1;
	message.msg_type = PDAFCTRL_EVT_PROCESS;
	message.sync_flag = CMR_MSG_SYNC_NONE;
	ret = cmr_thread_msg_send(cxt->thread_cxt.ctrl_thr_handle, &message);
	if (ret) {
		ISP_LOGE("fail to send msg to main thr %ld", ret);
		if (message.data)
			free(message.data);
		goto exit;
	}
	return ISP_SUCCESS;
  exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int pdaf_ctrl_ioctrl(cmr_handle handle, cmr_int cmd, struct pdaf_ctrl_param_in * in, struct pdaf_ctrl_param_out * out)
{
	cmr_int ret = ISP_SUCCESS;
	struct pdafctrl_context *cxt = (struct pdafctrl_context *)handle;

	struct pdaf_ctrl_msg_ctrl msg_ctrl;
	CMR_MSG_INIT(message);

	ISP_CHECK_HANDLE_VALID(handle);
	if (!cxt->pdaf_support) {
		ret = ISP_SUCCESS;
		goto exit;
	}

	if (cmd > PDAF_CTRL_CMD_DIRECT_BEGIN && cmd < PDAF_CTRL_CMD_DIRECT_END) {
		ret = pdafctrl_ioctrl(handle, cmd, in, out);
	} else {
		msg_ctrl.cmd = cmd;
		msg_ctrl.in = in;
		msg_ctrl.out = out;
		message.msg_type = PDAFCTRL_EVT_IOCTRL;
		message.sync_flag = CMR_MSG_SYNC_PROCESSED;
		message.alloc_flag = 0;
		message.data = &msg_ctrl;

		ret = cmr_thread_msg_send(cxt->thread_cxt.ctrl_thr_handle, &message);
		if (ret) {
			ISP_LOGE("fail to send msg to main thr %ld", ret);
			goto exit;
		}
	}

  exit:
	return ret;
}
