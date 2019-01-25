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
#define LOG_TAG "afl_ctrl"

#include <stdlib.h>
#include "cmr_msg.h"
#include "afl_ctrl.h"
#include "afl_adpt.h"
#include "isp_adpt.h"

#define AFLCTRL_MSG_QUEUE_SIZE      100
#define AFLCTRL_EVT_BASE            0x2000
#define AFLCTRL_EVT_INIT            AFLCTRL_EVT_BASE
#define AFLCTRL_EVT_DEINIT          (AFLCTRL_EVT_BASE + 1)
#define AFLCTRL_EVT_IOCTRL          (AFLCTRL_EVT_BASE + 2)
#define AFLCTRL_EVT_PROCESS         (AFLCTRL_EVT_BASE + 3)
#define AFLCTRL_EVT_EXIT            (AFLCTRL_EVT_BASE + 4)

/*ctrl thread context*/
struct aflctrl_ctrl_thr_cxt {
	cmr_handle thr_handle;
	cmr_int err_code;
};

struct aflctrl_work_lib {
	cmr_handle lib_handle;
	struct adpt_ops_type *adpt_ops;
};


/*ae ctrl context*/
struct aflctrl_cxt {
	cmr_u32 is_inited;
	cmr_u32 camera_id;
	cmr_handle caller_handle;
	struct aflctrl_work_lib work_lib;
	struct aflctrl_ctrl_thr_cxt ctrl_thr_cxt;
	cmr_int err_code;
	struct afl_ctrl_param_out ioctrl_out;
	struct afl_ctrl_proc_out proc_out;
	struct afl_ctrl_init_in init_in_param;
};

static cmr_int aflctrl_deinit_adpt(struct aflctrl_cxt *cxt)
{
	cmr_int ret = ISP_SUCCESS;
	struct aflctrl_work_lib *lib_ptr = NULL;

	if (!cxt) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}
	lib_ptr = &cxt->work_lib;
	if (lib_ptr->adpt_ops->adpt_deinit) {
		ret = lib_ptr->adpt_ops->adpt_deinit(lib_ptr->lib_handle);
	} else {
		ISP_LOGI("ioctrl fun is NULL");
	}

exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

static cmr_int aflctrl_ioctrl(struct aflctrl_cxt *cxt, enum afl_ctrl_cmd cmd, struct afl_ctrl_param_in *in_ptr, struct afl_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct aflctrl_work_lib *lib_ptr = NULL;

	if (!cxt) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}
	lib_ptr = &cxt->work_lib;
	if (lib_ptr->adpt_ops->adpt_ioctrl) {
		ret = lib_ptr->adpt_ops->adpt_ioctrl(lib_ptr->lib_handle, cmd, in_ptr, out_ptr);
	} else {
		ISP_LOGI("ioctrl fun is NULL");
	}
exit:
	ISP_LOGV("done %ld", ret);
	return ret;
}

static cmr_int aflctrl_process(struct aflctrl_cxt *cxt, struct afl_ctrl_proc_in *in_ptr, struct afl_ctrl_proc_out *out_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct aflctrl_work_lib *lib_ptr = NULL;

	if (!cxt) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}
	lib_ptr = &cxt->work_lib;
	if (lib_ptr->adpt_ops->adpt_process) {
		ret = lib_ptr->adpt_ops->adpt_process(lib_ptr->lib_handle, in_ptr, out_ptr);
	} else {
		ISP_LOGI("ioctrl fun is NULL");
	}
exit:
	ISP_LOGV("done %ld", ret);
	return ret;
}

static cmr_int aflctrl_ctrl_thr_proc(struct cmr_msg *message, void *p_data)
{
	cmr_int ret = ISP_SUCCESS;
	struct aflctrl_cxt *cxt = (struct aflctrl_cxt *)p_data;

	if (!message || !p_data) {
		ISP_LOGE("param error");
		goto exit;
	}
	ISP_LOGV("message.msg_type 0x%x, data %p", message->msg_type,
		 message->data);

	switch (message->msg_type) {
	case AFLCTRL_EVT_INIT:
		break;
	case AFLCTRL_EVT_DEINIT:
		ret = aflctrl_deinit_adpt(cxt);
		break;
	case AFLCTRL_EVT_EXIT:
		break;
	case AFLCTRL_EVT_IOCTRL:
		ret = aflctrl_ioctrl(cxt, (enum afl_ctrl_cmd)message->sub_msg_type, (struct afl_ctrl_param_in *)message->data, &cxt->ioctrl_out);
		break;
	case AFLCTRL_EVT_PROCESS:
		ret = aflctrl_process(cxt, (struct afl_ctrl_proc_in *)message->data, &cxt->proc_out);
		break;
	default:
		ISP_LOGE("don't support msg");
		break;
	}
exit:
	ISP_LOGV("done %ld", ret);
	return ret;
}

static cmr_int aflctrl_create_thread(struct aflctrl_cxt *cxt)
{
	cmr_int ret = -1;

	ret = cmr_thread_create(&cxt->ctrl_thr_cxt.thr_handle,
				AFLCTRL_MSG_QUEUE_SIZE,
				aflctrl_ctrl_thr_proc, (void *)cxt);
	if (CMR_MSG_SUCCESS != ret) {
		ISP_LOGE("failed to create main thread %ld", ret);
		ret = ISP_ERROR;
	}

	ISP_LOGV("done %ld", ret);
	return ret;
}
static cmr_int aflctrl_destroy_thread(struct aflctrl_cxt *cxt)
{
	cmr_int ret = ISP_SUCCESS;
	struct aflctrl_ctrl_thr_cxt *ctrl_thr_cxt = NULL;

	if (!cxt) {
		ISP_LOGE("in parm error");
		ret = ISP_ERROR;
		goto exit;
	}
	ctrl_thr_cxt = &cxt->ctrl_thr_cxt;
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

static cmr_int aflctrl_init_lib(struct aflctrl_cxt *cxt, struct afl_ctrl_init_in *in_ptr, struct afl_ctrl_init_out *out_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct aflctrl_work_lib *lib_ptr = NULL;

	if (!cxt) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}

	lib_ptr = &cxt->work_lib;
	if (lib_ptr->adpt_ops->adpt_init) {
		ret = lib_ptr->adpt_ops->adpt_init(in_ptr, out_ptr, &lib_ptr->lib_handle);
	} else {
		ISP_LOGI("ioctrl fun is NULL");
	}
exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

static cmr_int aflctrl_init_adpt(struct aflctrl_cxt *cxt, struct afl_ctrl_init_in *in_ptr, struct afl_ctrl_init_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;
	struct aflctrl_work_lib *lib_ptr = NULL;

	if (!cxt) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}

	/* find vendor adpter */
	ret = adpt_get_ops(ADPT_LIB_AFL, &in_ptr->lib_param, &cxt->work_lib.adpt_ops);
	if (ret) {
		ISP_LOGE("failed to get adapter layer ret = %ld", ret);
		goto exit;
	}

	ret = aflctrl_init_lib(cxt, in_ptr, out_ptr);
exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int afl_ctrl_init(struct afl_ctrl_init_in *in_ptr, struct afl_ctrl_init_out *out_ptr, cmr_handle *handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct aflctrl_cxt *cxt = NULL;

#ifndef CONFIG_CAMERA_AFL_AUTO_DETECTION
	ISP_LOGI("afl disabled");
	return ISP_SUCCESS;
#endif

	if (!in_ptr || !out_ptr || !handle) {
		ISP_LOGE("init param is null, input_ptr is %p, output_ptr is %p", in_ptr, out_ptr);
		ret = ISP_PARAM_NULL;
		goto exit;
	}

	*handle = NULL;
	cxt = (struct aflctrl_cxt *)malloc(sizeof(*cxt));
	if (NULL == cxt) {
		ISP_LOGE("failed to create ae ctrl context!");
		ret = ISP_ALLOC_ERROR;
		goto exit;
	}
	cmr_bzero(cxt, sizeof(*cxt));

	cxt->camera_id = in_ptr->camera_id;
	cxt->caller_handle = in_ptr->caller_handle;
	cxt->init_in_param = *in_ptr;
	cxt->err_code = ISP_SUCCESS;

	ret = aflctrl_create_thread(cxt);
	if (ret) {
		goto exit;
	}

	ret = aflctrl_init_adpt(cxt, in_ptr, out_ptr);
	if (ret) {
		goto error_adpt_init;
	}
	*handle = (cmr_handle)cxt;
	cxt->is_inited = 1;
	return 0;
error_adpt_init:
	aflctrl_destroy_thread(cxt);
exit:
	if (cxt)
		free((void *)cxt);
	ISP_LOGE("ret=%ld", ret);
	return ret;
}

cmr_int afl_ctrl_deinit(cmr_handle handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct aflctrl_cxt *cxt = (struct aflctrl_cxt *)handle;
	CMR_MSG_INIT(message);

#ifndef CONFIG_CAMERA_AFL_AUTO_DETECTION
	return ISP_SUCCESS;
#endif

	ISP_CHECK_HANDLE_VALID(handle);

	if (0 == cxt->is_inited) {
		ISP_LOGW("has been de-initialized");
		goto exit;
	}

	message.msg_type = AFLCTRL_EVT_DEINIT;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	message.alloc_flag = 0;
	message.data = NULL;
	ret = cmr_thread_msg_send(cxt->ctrl_thr_cxt.thr_handle, &message);
	if (ret) {
		ISP_LOGE("failed to send msg to main thr %ld", ret);
		goto exit;
	}

	aflctrl_destroy_thread(cxt);
	free((void *)handle);
exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int afl_ctrl_ioctrl(cmr_handle handle, enum afl_ctrl_cmd cmd, struct afl_ctrl_param_in *in_ptr, struct afl_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct aflctrl_cxt *cxt = (struct aflctrl_cxt *)handle;
	CMR_MSG_INIT(message);

#ifndef CONFIG_CAMERA_AFL_AUTO_DETECTION
	return ISP_SUCCESS;
#endif

	ISP_CHECK_HANDLE_VALID(handle);

	if (cmd >= AFL_CTRL_CMD_MAX) {
		ISP_LOGE("input param is error %d", cmd);
		goto exit;
	}

	message.msg_type = AFLCTRL_EVT_IOCTRL;
	message.sub_msg_type = cmd;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	message.alloc_flag = 0;
	message.data = (void *)in_ptr;
	ret = cmr_thread_msg_send(cxt->ctrl_thr_cxt.thr_handle, &message);
	if (ret) {
		ISP_LOGE("failed to send msg to main thr %ld", ret);
		goto exit;
	}
	if (out_ptr) {
		*out_ptr = cxt->ioctrl_out;
	}
exit:
	ISP_LOGV("cmd = %d, done %ld", cmd, ret);
	return ret;
}

cmr_int afl_ctrl_process(cmr_handle handle, struct afl_ctrl_proc_in *in_ptr, struct afl_ctrl_proc_out *out_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct aflctrl_cxt *cxt = (struct aflctrl_cxt *)handle;
	CMR_MSG_INIT(message);

#ifndef CONFIG_CAMERA_AFL_AUTO_DETECTION
	return ISP_SUCCESS;
#endif

	ISP_CHECK_HANDLE_VALID(handle);

	if (!in_ptr || !out_ptr) {
		ISP_LOGI("input param is error %p", in_ptr);
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

	message.msg_type = AFLCTRL_EVT_PROCESS;
	message.sync_flag = CMR_MSG_SYNC_NONE;
	ret = cmr_thread_msg_send(cxt->ctrl_thr_cxt.thr_handle, &message);
	if (ret) {
		ISP_LOGE("failed to send msg to main thr %ld", ret);
		if (message.data)
			free(message.data);
		goto exit;
	}
exit:
	ISP_LOGV("done %ld", ret);
	return ret;
}

