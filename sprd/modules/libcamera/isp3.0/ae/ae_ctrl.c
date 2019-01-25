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
#define LOG_TAG "ae_ctrl"

#include <stdlib.h>
#include "cmr_msg.h"
#include "ae_ctrl.h"
#include "ae_adpt.h"
#include "isp_adpt.h"

#define AECTRL_MSG_QUEUE_SIZE      100
#define AECTRL_EVT_BASE            0x2000
#define AECTRL_EVT_INIT            AECTRL_EVT_BASE
#define AECTRL_EVT_DEINIT          (AECTRL_EVT_BASE + 1)
#define AECTRL_EVT_IOCTRL          (AECTRL_EVT_BASE + 2)
#define AECTRL_EVT_PROCESS         (AECTRL_EVT_BASE + 3)
#define AECTRL_EVT_EXIT            (AECTRL_EVT_BASE + 4)

/*ctrl thread context*/
struct aectrl_ctrl_thr_cxt {
	cmr_handle thr_handle;
	cmr_int err_code;
};

struct aectrl_work_lib {
	cmr_handle lib_handle;
	struct adpt_ops_type *adpt_ops;
};


/*ae ctrl context*/
struct aectrl_cxt {
	cmr_u32 is_inited;
	cmr_u32 camera_id;
	cmr_handle caller_handle;
	struct aectrl_work_lib work_lib;
	struct aectrl_ctrl_thr_cxt ctrl_thr_cxt;
	struct ae_ctrl_param_out ioctrl_out;
	pthread_mutex_t ioctrl_out_mutex;
	struct ae_ctrl_proc_out proc_out;
	struct ae_ctrl_init_in init_in_param;
};

static cmr_int aectrl_deinit_adpt(struct aectrl_cxt *cxt_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct aectrl_work_lib *lib_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}
	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_deinit) {
		ret = lib_ptr->adpt_ops->adpt_deinit(lib_ptr->lib_handle);
	} else {
		ISP_LOGI("ioctrl fun is NULL");
	}

exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

static cmr_int aectrl_ioctrl(struct aectrl_cxt *cxt_ptr, enum ae_ctrl_cmd cmd, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct aectrl_work_lib *lib_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}
	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_ioctrl) {
		ret = lib_ptr->adpt_ops->adpt_ioctrl(lib_ptr->lib_handle, cmd, in_ptr, out_ptr);
	} else {
		ISP_LOGI("ioctrl fun is NULL");
	}
exit:
	ISP_LOGV("done %ld", ret);
	return ret;
}

static cmr_int aectrl_process(struct aectrl_cxt *cxt_ptr, struct ae_ctrl_proc_in *in_ptr, struct ae_ctrl_proc_out *out_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct aectrl_work_lib *lib_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}
	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_process) {
		ret = lib_ptr->adpt_ops->adpt_process(lib_ptr->lib_handle, in_ptr, out_ptr);
	} else {
		ISP_LOGI("ioctrl fun is NULL");
	}
exit:
	ISP_LOGV("done %ld", ret);
	return ret;
}

static cmr_int aectrl_ctrl_thr_proc(struct cmr_msg *message, void *p_data)
{
	cmr_int ret = ISP_SUCCESS;
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt *)p_data;

	if (!message || !p_data) {
		ISP_LOGE("param error");
		goto exit;
	}
	ISP_LOGV("message.msg_type 0x%x, data %p", message->msg_type,
		 message->data);

	switch (message->msg_type) {
	case AECTRL_EVT_INIT:
		break;
	case AECTRL_EVT_DEINIT:
		ret = aectrl_deinit_adpt(cxt_ptr);
		break;
	case AECTRL_EVT_EXIT:
		break;
	case AECTRL_EVT_IOCTRL:
		ret = aectrl_ioctrl(cxt_ptr, (enum ae_ctrl_cmd)message->sub_msg_type, (struct ae_ctrl_param_in *)message->data, &cxt_ptr->ioctrl_out);
		break;
	case AECTRL_EVT_PROCESS:
		ret = aectrl_process(cxt_ptr, (struct ae_ctrl_proc_in *)message->data, &cxt_ptr->proc_out);
		break;
	default:
		ISP_LOGE("don't support msg");
		break;
	}
	cxt_ptr->ctrl_thr_cxt.err_code = ret;
exit:
	ISP_LOGV("done %ld", ret);
	return ret;
}

static cmr_int aectrl_create_thread(struct aectrl_cxt *cxt_ptr)
{
	cmr_int ret = -1;

	ret = cmr_thread_create(&cxt_ptr->ctrl_thr_cxt.thr_handle,
				AECTRL_MSG_QUEUE_SIZE,
				aectrl_ctrl_thr_proc, (void *)cxt_ptr);
	if (CMR_MSG_SUCCESS != ret) {
		ISP_LOGE("failed to create main thread %ld", ret);
		ret = ISP_ERROR;
	}

	ISP_LOGV("done %ld", ret);
	return ret;
}
static cmr_int aectrl_destroy_thread(struct aectrl_cxt *cxt_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct aectrl_ctrl_thr_cxt *ctrl_thr_cxt = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("in parm error");
		ret = ISP_ERROR;
		goto exit;
	}
	ctrl_thr_cxt = &cxt_ptr->ctrl_thr_cxt;
	if (ctrl_thr_cxt->thr_handle) {
		ret = cmr_thread_destroy(ctrl_thr_cxt->thr_handle);
		if (!ret) {
			ctrl_thr_cxt->thr_handle = NULL;
		} else {
			ISP_LOGE("failed to destroy ctrl thread %ld", ret);
		}
	}
exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

static cmr_int aectrl_init_lib(struct aectrl_cxt *cxt_ptr, struct ae_ctrl_init_in *in_ptr, struct ae_ctrl_init_out *out_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct aectrl_work_lib *lib_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}

	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_init) {
		ret = lib_ptr->adpt_ops->adpt_init(in_ptr, out_ptr, &lib_ptr->lib_handle);
	} else {
		ISP_LOGI("ioctrl fun is NULL");
	}
exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

static cmr_int aectrl_init_adpt(struct aectrl_cxt *cxt_ptr, struct ae_ctrl_init_in *in_ptr, struct ae_ctrl_init_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;
	struct aectrl_work_lib *lib_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}

	/* find vendor adpter */
	ret = adpt_get_ops(ADPT_LIB_AE, &in_ptr->lib_param, &cxt_ptr->work_lib.adpt_ops);
	if (ret) {
		ISP_LOGE("failed to get adapter layer ret = %ld", ret);
		goto exit;
	}

	ret = aectrl_init_lib(cxt_ptr, in_ptr, out_ptr);
exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int ae_ctrl_init(struct ae_ctrl_init_in *in_ptr, struct ae_ctrl_init_out *out_ptr, cmr_handle *handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct aectrl_cxt *cxt_ptr = NULL;

	if (!in_ptr || !out_ptr || !handle) {
		ISP_LOGE("init param is null, input_ptr is %p, output_ptr is %p", in_ptr, out_ptr);
		ret = ISP_PARAM_NULL;
		goto exit;
	}

	*handle = NULL;
	cxt_ptr = (struct aectrl_cxt *)malloc(sizeof(*cxt_ptr));
	if (NULL == cxt_ptr) {
		ISP_LOGE("failed to create ae ctrl context!");
		ret = ISP_ALLOC_ERROR;
		goto exit;
	}
	cmr_bzero(cxt_ptr, sizeof(*cxt_ptr));

	cxt_ptr->camera_id = in_ptr->camera_id;
	cxt_ptr->caller_handle = in_ptr->caller_handle;
	cxt_ptr->init_in_param = *in_ptr;
	pthread_mutex_init(&cxt_ptr->ioctrl_out_mutex, NULL);

	ret = aectrl_create_thread(cxt_ptr);
	if (ret) {
		goto exit;
	}

	ret = aectrl_init_adpt(cxt_ptr, in_ptr, out_ptr);
	if (ret) {
		goto error_adpt_init;
	}
	*handle = (cmr_handle)cxt_ptr;
	cxt_ptr->is_inited = 1;
	return 0;
error_adpt_init:
	aectrl_destroy_thread(cxt_ptr);
exit:
	if (cxt_ptr) {
		free((void *)cxt_ptr);
	}
	return ret;
}

cmr_int ae_ctrl_deinit(cmr_handle handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt *)handle;
	CMR_MSG_INIT(message);

	ISP_CHECK_HANDLE_VALID(handle);

	if (0 == cxt_ptr->is_inited) {
		ISP_LOGW("has been de-initialized");
		goto exit;
	}

	message.msg_type = AECTRL_EVT_DEINIT;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	message.alloc_flag = 0;
	message.data = NULL;
	ret = cmr_thread_msg_send(cxt_ptr->ctrl_thr_cxt.thr_handle, &message);
	if (ret) {
		ISP_LOGE("failed to send msg to main thr %ld", ret);
		goto exit;
	}

	aectrl_destroy_thread(cxt_ptr);
	pthread_mutex_destroy(&cxt_ptr->ioctrl_out_mutex);
	free((void *)handle);
exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int ae_ctrl_ioctrl(cmr_handle handle, enum ae_ctrl_cmd cmd, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt *)handle;
	CMR_MSG_INIT(message);
	cmr_int need_msg = 1;

	ISP_CHECK_HANDLE_VALID(handle);

	if (AE_CTRL_CMD_MAX <= cmd) {
		ISP_LOGI("not support %d command", cmd);
		goto exit;
	}

	message.msg_type = AECTRL_EVT_IOCTRL;
	message.sub_msg_type = cmd;
	if ((AE_CTRL_SYNC_NONE_MSG_BEGIN < cmd) &&
		(AE_CTRL_SYNC_NONE_MSG_END > cmd)) {
		message.data = malloc(sizeof(*in_ptr));
		if (!message.data) {
			ISP_LOGE("failed to malloc msg");
			ret = -ISP_ALLOC_ERROR;
			goto exit;
		}
		memcpy(message.data, in_ptr, sizeof(*in_ptr));
		message.alloc_flag = 1;
		message.sync_flag = CMR_MSG_SYNC_NONE;
	} else if ((AE_CTRL_DIRECT_MSG_BEGIN < cmd) &&
		   (AE_CTRL_DIRECT_MSG_END > cmd)) {
		need_msg = 0;
		ret = aectrl_ioctrl(cxt_ptr, cmd, in_ptr, &cxt_ptr->ioctrl_out);
		if (ret)
			goto exit;
	} else {
		message.sync_flag = CMR_MSG_SYNC_PROCESSED;
		message.alloc_flag = 0;
		message.data = (void *)in_ptr;
	}

	pthread_mutex_lock(&cxt_ptr->ioctrl_out_mutex);
	if (need_msg) {
		ret = cmr_thread_msg_send(cxt_ptr->ctrl_thr_cxt.thr_handle, &message);
		if (ret) {
			ISP_LOGE("failed to send msg to main thr %ld", ret);
			if (message.alloc_flag && message.data)
				free(message.data);

			pthread_mutex_unlock(&cxt_ptr->ioctrl_out_mutex);
			goto exit;
		}
	}
	if (out_ptr) {
		*out_ptr = cxt_ptr->ioctrl_out;
	}
	ret = cxt_ptr->ctrl_thr_cxt.err_code;
	pthread_mutex_unlock(&cxt_ptr->ioctrl_out_mutex);
exit:
	ISP_LOGV("cmd = %d,done %ld", cmd, ret);
	return ret;
}

cmr_int ae_ctrl_process(cmr_handle handle, struct ae_ctrl_proc_in *in_ptr, struct ae_ctrl_proc_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt *)handle;
	CMR_MSG_INIT(message);

	ISP_CHECK_HANDLE_VALID(handle);

	if (!in_ptr || !out_ptr) {
		ISP_LOGI("input param %p %p is error !!!", in_ptr, out_ptr);
		goto exit;
	}
	message.data = malloc(sizeof(*in_ptr));
	if (!message.data) {
		ISP_LOGE("failed to malloc msg");
		ret = -ISP_ALLOC_ERROR;
		goto exit;
	}
	memcpy(message.data, in_ptr, sizeof(*in_ptr));
	message.alloc_flag = 1;

	message.msg_type = AECTRL_EVT_PROCESS;
	message.sync_flag = CMR_MSG_SYNC_NONE;
	ret = cmr_thread_msg_send(cxt_ptr->ctrl_thr_cxt.thr_handle, &message);
	if (ret) {
		ISP_LOGE("failed to send msg to main thr %ld", ret);
		if (message.data)
			free(message.data);
		goto exit;
	}
	return ISP_SUCCESS;
exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

