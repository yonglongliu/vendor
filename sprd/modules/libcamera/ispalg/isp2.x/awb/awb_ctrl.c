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
#define LOG_TAG "awb_ctrl"

#include "awb_ctrl.h"
#include "isp_adpt.h"
#include "cmr_common.h"

#define AWBCTRL_MSG_QUEUE_SIZE      100
#define AWBCTRL_EVT_BASE            0x2000
#define AWBCTRL_EVT_INIT            AWBCTRL_EVT_BASE
#define AWBCTRL_EVT_DEINIT          (AWBCTRL_EVT_BASE + 1)
#define AWBCTRL_EVT_IOCTRL          (AWBCTRL_EVT_BASE + 2)
#define AWBCTRL_EVT_PROCESS         (AWBCTRL_EVT_BASE + 3)
#define AWBCTRL_EVT_EXIT            (AWBCTRL_EVT_BASE + 4)

struct awbctrl_work_lib {
	cmr_handle lib_handle;
	struct adpt_ops_type *adpt_ops;
};

struct awbctrl_cxt {
	cmr_handle thr_handle;
	struct awbctrl_work_lib work_lib;
};

static cmr_int awbctrl_deinit_adpt(struct awbctrl_cxt *cxt_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct awbctrl_work_lib *lib_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param,param is NULL!");
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

static cmr_int awbctrl_init_lib(struct awbctrl_cxt *cxt_ptr, struct awb_ctrl_init_param *in_ptr, struct awb_ctrl_init_result *out_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct awbctrl_work_lib *lib_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param,param is NULL!");
		goto exit;
	}

	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_init) {
		lib_ptr->lib_handle = lib_ptr->adpt_ops->adpt_init(in_ptr, out_ptr);
	} else {
		ISP_LOGI("adpt_init fun is NULL");
	}
  exit:
	ISP_LOGI("done %ld", rtn);
	return rtn;
}

static cmr_int awbctrl_init_adpt(struct awbctrl_cxt *cxt_ptr, struct awb_ctrl_init_param *in_ptr, struct awb_ctrl_init_result *out_ptr)
{
	cmr_int rtn = ISP_SUCCESS;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check para, param is NULL!");
		goto exit;
	}

	/* find vendor adpter */
	rtn = adpt_get_ops(ADPT_LIB_AWB, &in_ptr->lib_param, &cxt_ptr->work_lib.adpt_ops);
	if (rtn) {
		ISP_LOGE("fail to get adapter layer ret = %ld", rtn);
		goto exit;
	}

	rtn = awbctrl_init_lib(cxt_ptr, in_ptr, out_ptr);
  exit:
	ISP_LOGI("done %ld", rtn);
	return rtn;
}

cmr_int awbctrl_ioctrl(struct awbctrl_cxt * cxt_ptr, enum awb_ctrl_cmd cmd, void *in_ptr, void *out_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct awbctrl_work_lib *lib_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param,param is NULL!");
		goto exit;
	}

	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_ioctrl) {
		rtn = lib_ptr->adpt_ops->adpt_ioctrl(lib_ptr->lib_handle, cmd, in_ptr, out_ptr);
	} else {
		ISP_LOGI("ioctrl fun is NULL");
	}
  exit:
	ISP_LOGV("cmd = %d,done %ld", cmd, rtn);
	return rtn;
}

static cmr_int awbctrl_process(struct awbctrl_cxt *cxt_ptr, struct awb_ctrl_calc_param *in_ptr, struct awb_ctrl_calc_result *out_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct awbctrl_work_lib *lib_ptr = NULL;
	if (!cxt_ptr) {
		ISP_LOGE("fail to check param, param is NULL!");
		goto exit;
	}
	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_process) {
		rtn = lib_ptr->adpt_ops->adpt_process(lib_ptr->lib_handle, in_ptr, out_ptr);
	} else {
		ISP_LOGI("process fun is NULL");
	}
  exit:
	ISP_LOGV("done %ld", rtn);
	return rtn;
}

static cmr_int awbctrl_ctrl_thr_proc(struct cmr_msg *message, cmr_handle p_data)
{
	cmr_int rtn = ISP_SUCCESS;
	struct awbctrl_cxt *cxt_ptr = (struct awbctrl_cxt *)p_data;

	if (!message || !p_data) {
		ISP_LOGE("fail to check param");
		goto exit;
	}
	ISP_LOGV("message.msg_type 0x%x, data %p", message->msg_type, message->data);

	switch (message->msg_type) {
	case AWBCTRL_EVT_INIT:
		break;
	case AWBCTRL_EVT_DEINIT:
		rtn = awbctrl_deinit_adpt(cxt_ptr);
		break;
	case AWBCTRL_EVT_EXIT:
		break;
	case AWBCTRL_EVT_IOCTRL:
		rtn = awbctrl_ioctrl(cxt_ptr, ((struct awb_ctrl_msg_ctrl *)message->data)->cmd, ((struct awb_ctrl_msg_ctrl *)message->data)->in, ((struct awb_ctrl_msg_ctrl *)message->data)->out);
		break;
	case AWBCTRL_EVT_PROCESS:
		rtn = awbctrl_process(cxt_ptr, (struct awb_ctrl_calc_param *)message->data, NULL);
		break;
	default:
		ISP_LOGE("fail to check param, don't support msg");
		break;
	}

  exit:
	ISP_LOGV("done %ld", rtn);
	return rtn;
}

static cmr_int awbctrl_create_thread(struct awbctrl_cxt *cxt_ptr)
{
	cmr_int rtn = ISP_SUCCESS;

	rtn = cmr_thread_create(&cxt_ptr->thr_handle, ISP_THREAD_QUEUE_NUM, awbctrl_ctrl_thr_proc, (cmr_handle) cxt_ptr);
	if (rtn) {
		ISP_LOGE("fail to create ctrl thread");
		rtn = ISP_ERROR;
		goto exit;
	}
	rtn = cmr_thread_set_name(cxt_ptr->thr_handle, "awbctrl");
	if (CMR_MSG_SUCCESS != rtn) {
		ISP_LOGE("fail to set awbctrl name");
		rtn = CMR_MSG_SUCCESS;
	}
  exit:
	ISP_LOGI("done %ld", rtn);
	return rtn;
}

static cmr_int awbctrl_destroy_thread(struct awbctrl_cxt *cxt_ptr)
{
	cmr_int rtn = ISP_SUCCESS;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check input param");
		rtn = ISP_ERROR;
		goto exit;
	}

	if (cxt_ptr->thr_handle) {
		rtn = cmr_thread_destroy(cxt_ptr->thr_handle);
		if (!rtn) {
			cxt_ptr->thr_handle = NULL;
		} else {
			ISP_LOGE("fail to destroy ctrl thread %ld", rtn);
		}
	}
  exit:
	ISP_LOGI("done %ld", rtn);
	return rtn;
}

cmr_int awb_ctrl_ioctrl(cmr_handle handle, enum awb_ctrl_cmd cmd, cmr_handle in_ptr, cmr_handle out_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct awbctrl_cxt *cxt_ptr = (struct awbctrl_cxt *)handle;
	struct awb_ctrl_msg_ctrl msg_ctrl;

	ISP_CHECK_HANDLE_VALID(handle);
	CMR_MSG_INIT(message);
	if ((AWB_SYNC_MSG_BEGIN < cmd) && (AWB_SYNC_MSG_END > cmd)) {
		msg_ctrl.cmd = cmd;
		msg_ctrl.in = in_ptr;
		msg_ctrl.out = out_ptr;
		message.data = &msg_ctrl;
		message.msg_type = AWBCTRL_EVT_IOCTRL;
		message.sync_flag = CMR_MSG_SYNC_PROCESSED;
		rtn = cmr_thread_msg_send(cxt_ptr->thr_handle, &message);
	} else if ((AWB_DIRECT_MSG_BEGIN < cmd) && (AWB_DIRECT_MSG_END > cmd)) {
		rtn = awbctrl_ioctrl(cxt_ptr, cmd, in_ptr, out_ptr);
	}

  exit:
	ISP_LOGV("cmd = %d,done %ld", cmd, rtn);
	return rtn;
}

cmr_int awb_ctrl_init(struct awb_ctrl_init_param * input_ptr, cmr_handle * handle_awb)
{
	cmr_int rtn = ISP_SUCCESS;
	struct awbctrl_cxt *cxt_ptr = NULL;
	struct awb_ctrl_init_result result;

	memset((void *)&result, 0, sizeof(result));

	cxt_ptr = (struct awbctrl_cxt *)malloc(sizeof(*cxt_ptr));
	if (NULL == cxt_ptr) {
		ISP_LOGE("fail to create awb ctrl context!");
		rtn = ISP_ALLOC_ERROR;
		goto exit;
	}
	memset(cxt_ptr, 0, sizeof(*cxt_ptr));

	rtn = awbctrl_create_thread(cxt_ptr);
	if (rtn) {
		goto exit;
	}

	rtn = awbctrl_init_adpt(cxt_ptr, input_ptr, &result);
	if (rtn) {
		goto error_adpt_init;
	}

	*handle_awb = (cmr_handle) cxt_ptr;

	return rtn;

  error_adpt_init:
	awbctrl_destroy_thread(cxt_ptr);
  exit:
	if (cxt_ptr) {
		free((cmr_handle) cxt_ptr);
		cxt_ptr = NULL;
	}
	ISP_LOGI("done %ld", rtn);

	return rtn;
}

cmr_int awb_ctrl_deinit(cmr_handle * handle_awb)
{
	cmr_int rtn = ISP_SUCCESS;
	struct awbctrl_cxt *cxt_ptr = *handle_awb;
	CMR_MSG_INIT(message);

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param, in parm is NULL");
		return ISP_ERROR;
	}

	message.msg_type = AWBCTRL_EVT_DEINIT;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	message.alloc_flag = 0;
	message.data = NULL;
	rtn = cmr_thread_msg_send(cxt_ptr->thr_handle, &message);
	if (rtn) {
		ISP_LOGE("fail to send msg to main thr %ld", rtn);
		goto exit;
	}
	rtn = awbctrl_destroy_thread(cxt_ptr);
	if (rtn) {
		ISP_LOGE("fail to destroy awbctrl thread %ld", rtn);
		goto exit;
	}

  exit:
	if (cxt_ptr) {
		free((cmr_handle) cxt_ptr);
		*handle_awb = NULL;
	}

	ISP_LOGI("done %ld", rtn);
	return rtn;
}

cmr_int awb_ctrl_process(cmr_handle handle_awb, struct awb_ctrl_calc_param * in_ptr, struct awb_ctrl_calc_result * result)
{
	cmr_int rtn = ISP_SUCCESS;
	struct awbctrl_cxt *cxt_ptr = (struct awbctrl_cxt *)handle_awb;
	UNUSED(result);
	ISP_CHECK_HANDLE_VALID(handle_awb);

	CMR_MSG_INIT(message);
	if (!handle_awb || !in_ptr) {
		ISP_LOGE("fail to check param, param is NULL!");
		rtn = AWB_ERROR;
		goto exit;
	}

	message.data = malloc(sizeof(struct awb_ctrl_calc_param));
	if (!message.data) {
		ISP_LOGE("fail to malloc msg");
		rtn = ISP_ALLOC_ERROR;
		goto exit;
	}
	memcpy(message.data, (cmr_handle) in_ptr, sizeof(struct awb_ctrl_calc_param));
	message.alloc_flag = 1;

	message.msg_type = AWBCTRL_EVT_PROCESS;
	message.sync_flag = CMR_MSG_SYNC_NONE;
	rtn = cmr_thread_msg_send(cxt_ptr->thr_handle, &message);

	if (rtn) {
		ISP_LOGE("fail to send msg to main thr %ld", rtn);
		if (message.data)
			free(message.data);
		goto exit;
	}

  exit:
	ISP_LOGV("done %ld", rtn);
	return rtn;
}
