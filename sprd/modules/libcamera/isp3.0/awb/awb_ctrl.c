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

#include <stdlib.h>
#include "cutils/properties.h"
#include <unistd.h>
#include "isp_type.h"
#include "awb_ctrl.h"
#include "isp_adpt.h"

/**************************************** MACRO DEFINE *****************************************/
#define AWBCTRL_MSG_QUEUE_SIZE                                    100

#define AWBCTRL_EVT_BASE                                          (1<<28)
#define AWBCTRL_EVT_INIT                                          AWBCTRL_EVT_BASE
#define AWBCTRL_EVT_DEINIT                                        (AWBCTRL_EVT_BASE+1)
#define AWBCTRL_EVT_IOCTRL                                        (AWBCTRL_EVT_BASE+2)
#define AWBCTRL_EVT_PROCESS                                       (AWBCTRL_EVT_BASE+3)
#define AWBCTRL_EVT_EXIT                                          (AWBCTRL_EVT_BASE+4)


/************************************* INTERNAL DATA TYPE ***************************************/
typedef cmr_int (*awb_ioctrl_fun)(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr);

struct awbctrl_thread_context {
	cmr_handle ctrl_thr_handle;
};

struct awbctrl_context {
	cmr_u32 is_inited;
	cmr_u32 camera_id;
	cmr_handle caller_handle;
	cmr_handle lib_handle;
	cmr_int err_code;
	cmr_s16 lock_num;
	cmr_u16 lock_flag;
	struct adpt_ops_type *awb_adpt_ops;
	struct awb_ctrl_init_in input_param;
	struct awbctrl_thread_context thread_cxt;
	awb_callback caller_callback;
	union awb_ctrl_cmd_out ioctrl_out;
	struct awb_ctrl_process_out process_out;
	struct isp_awb_gain cur_gain;
	struct isp_awb_gain cur_gain_balanced;
	struct isp3a_awb_hw_cfg hw_cfg;
	cmr_u32 cur_ct;
	void *priv_handle;
	sem_t sync_sm;
};

struct awbctrl_ioctrl_fun {
	enum awb_ctrl_cmd cmd;
	awb_ioctrl_fun ioctrl;
};

/************************************** LOCAL FUNCTION** ***************************************/
static cmr_int awbctrl_create_thread(cmr_handle awb_ctrl_handle);
static cmr_int awbctrl_ctrl_thread_proc(struct cmr_msg *message, void *p_data);
static cmr_int awbctrl_destroy_thread(cmr_handle awb_ctrl_handle);
static cmr_int awbctrl_get_lib_inf(cmr_handle awb_ctrl_handle);
static cmr_int awbctrl_init_lib(cmr_handle awb_ctrl_handle);
static cmr_int awbctrl_deinit_lib(cmr_handle awb_ctrl_handle);
static awb_ioctrl_fun awbctrl_get_ioctrl_fun(enum awb_ctrl_cmd cmd);
static cmr_int awbctrl_ioctrl(cmr_handle awb_ctrl_handle, enum awb_ctrl_cmd cmd, void *input_ptr, void *output_ptr);
static cmr_int awbctrl_process(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr);
static cmr_int awbctrl_set_scenemode(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr);
static cmr_int awbctrl_set_wbmode(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr);
static cmr_int awbctrl_set_flashmode(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr);
static cmr_int awbctrl_set_workmode(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr);
static cmr_int awbctr_set_workmode(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr);
static cmr_int awbctrl_set_faceinfo(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr);
static cmr_int awbctrl_set_dzoom(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr);
static cmr_int awbctrl_set_sof_frame_idx(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr);
static cmr_int awbctrl_set_bypass(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr);
static cmr_int awbctrl_get_gain(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr);
static cmr_int awbctrl_set_flash_open_p(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr);
static cmr_int awbctrl_set_flash_open_m(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr);
static cmr_int awbctrl_set_flashing(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr);
static cmr_int awbctrl_set_flash_close(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr);
static cmr_int awbctrl_set_lock(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr);
static cmr_int awbctrl_set_unlock(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr);
static cmr_int awbctrl_get_ct(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr);
static cmr_int awbctrl_set_flash_before_p(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr);
static cmr_int awbctrl_set_flash_status(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr);
static cmr_int awbctrl_set_ae_info(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr);
static cmr_int awbctrl_set_af_info(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr);
static cmr_int awbctrl_get_exif_debug_info(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr);
static cmr_int awbctrl_get_debug_info(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr);
static cmr_int awbctrl_set_slave_iso(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr);
static cmr_int awbctrl_set_master(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr);
static cmr_int awbctrl_set_tuning_mode(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr);
/**************************************LOCAL VARIABLE******************************************/
static struct awbctrl_ioctrl_fun s_awbctrl_func[AWB_CTRL_CMD_MAX+1] = {
	{AWB_CTRL_CMD_SET_BASE, NULL},
	{AWB_CTRL_CMD_SET_SCENE_MODE, awbctrl_set_scenemode},
	{AWB_CTRL_CMD_SET_WB_MODE, awbctrl_set_wbmode},
	{AWB_CTRL_CMD_SET_FLASH_MODE, awbctrl_set_flashmode},
	{AWB_CTRL_CMD_SET_STAT_IMG_SIZE, NULL},
	{AWB_CTRL_CMD_SET_STAT_IMG_FORMAT, NULL},
	{AWB_CTRL_CMD_SET_WORK_MODE, awbctr_set_workmode},
	{AWB_CTRL_CMD_SET_FACE_INFO, awbctrl_set_faceinfo},
	{AWB_CTRL_CMD_SET_DZOOM_FACTOR, awbctrl_set_dzoom},
	{AWB_CTRL_CMD_SET_SOF_FRAME_IDX, awbctrl_set_sof_frame_idx},
	{AWB_CTRL_CMD_SET_BYPASS, awbctrl_set_bypass},
	{AWB_CTRL_CMD_GET_PARAM_WIN_START, NULL},
	{AWB_CTRL_CMD_GET_PARAM_WIN_SIZE, NULL},
	{AWB_CTRL_CMD_GET_GAIN, awbctrl_get_gain},
	{AWB_CTRL_CMD_FLASH_OPEN_P, awbctrl_set_flash_open_p},
	{AWB_CTRL_CMD_FLASH_OPEN_M, awbctrl_set_flash_open_m},
	{AWB_CTRL_CMD_FLASHING, awbctrl_set_flashing},
	{AWB_CTRL_CMD_FLASH_CLOSE, awbctrl_set_flash_close},
	{AWB_CTRL_CMD_LOCK, awbctrl_set_lock},
	{AWB_CTRL_CMD_UNLOCK, awbctrl_set_unlock},
	{AWB_CTRL_CMD_GET_STAT_SIZE, NULL},
	{AWB_CTRL_CMD_GET_WIN_SIZE, NULL},
	{AWB_CTRL_CMD_GET_CT, awbctrl_get_ct},
	{AWB_CTRL_CMD_FLASH_BEFORE_P, awbctrl_set_flash_before_p},
	{AWB_CTRL_CMD_SET_FLASH_STATUS, awbctrl_set_flash_status},
	{AWB_CTRL_CMD_SET_AE_REPORT, awbctrl_set_ae_info},
	{AWB_CTRL_CMD_SET_AF_REPORT, awbctrl_set_af_info},
	{AWB_CTRL_CMD_GET_EXIF_DEBUG_INFO, awbctrl_get_exif_debug_info},
	{AWB_CTRL_CMD_GET_DEBUG_INFO, awbctrl_get_debug_info},
	{AWB_CTRL_CMD_SET_SLAVE_ISO_SPEED, awbctrl_set_slave_iso},
	{AWB_CTRL_CMD_SET_MASTER, awbctrl_set_master},
	{AWB_CTRL_CMD_SET_TUNING_MODE, awbctrl_set_tuning_mode},
	{AWB_CTRL_CMD_MAX, NULL},
};

/*************************************INTERNAL FUNCTION ***************************************/
cmr_int awbctrl_create_thread(cmr_handle awb_ctrl_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awbctrl_context                      *cxt = (struct awbctrl_context *)awb_ctrl_handle;

	ret = cmr_thread_create(&cxt->thread_cxt.ctrl_thr_handle,
				AWBCTRL_MSG_QUEUE_SIZE,
				awbctrl_ctrl_thread_proc,
				(void *)awb_ctrl_handle);
	ISP_LOGV("%p", cxt->thread_cxt.ctrl_thr_handle);
	if (CMR_MSG_SUCCESS != ret) {
		ISP_LOGE("failed to create main thread");
		ret = ISP_ERROR;
	}

	return ret;
}

cmr_int awbctrl_ctrl_thread_proc(struct cmr_msg *message, void *p_data)
{
	cmr_int                                     ret = ISP_SUCCESS;
	cmr_handle                                  awb_ctrl_handle = (cmr_handle)p_data;
	struct awbctrl_context                      *cxt = (struct awbctrl_context *)awb_ctrl_handle;

	if (!message || !p_data) {
		ISP_LOGE("param error");
		goto exit;
	}
	ISP_LOGI("message.msg_type 0x%x, data %p", message->msg_type, message->data);

	switch (message->msg_type) {
	case AWBCTRL_EVT_INIT:
		break;
	case AWBCTRL_EVT_DEINIT:
		break;
	case AWBCTRL_EVT_EXIT:
		break;
	case AWBCTRL_EVT_IOCTRL:
		ret = awbctrl_ioctrl(awb_ctrl_handle, message->sub_msg_type, message->data, (void *)&(cxt->ioctrl_out));
		break;
	case AWBCTRL_EVT_PROCESS:
		ret = awbctrl_process(awb_ctrl_handle, message->data, (void *)&(cxt->process_out));
		break;
	default:
		ISP_LOGI("don't support msg");
		break;
	}
exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int awbctrl_destroy_thread(cmr_handle awb_ctrl_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awbctrl_context                      *cxt = (struct awbctrl_context *)awb_ctrl_handle;
	struct awbctrl_thread_context               *awb_thread_cxt;

	if (!awb_ctrl_handle) {
		ISP_LOGE("in parm error");
		ret = ISP_ERROR;
		goto exit;
	}
	awb_thread_cxt = &cxt->thread_cxt;
	if (awb_thread_cxt->ctrl_thr_handle) {
		ret = cmr_thread_destroy(awb_thread_cxt->ctrl_thr_handle);
		if (!ret) {
			awb_thread_cxt->ctrl_thr_handle = (cmr_handle)NULL;
		} else {
			ISP_LOGE("failed to destroy ctrl thread %ld", ret);
		}
	}
exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int awbctrl_init_lib(cmr_handle awb_ctrl_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awbctrl_context                      *cxt = (struct awbctrl_context *)awb_ctrl_handle;
	struct awb_ctrl_init_out                    output_param;

	if (cxt->awb_adpt_ops->adpt_init) {
		ret = cxt->awb_adpt_ops->adpt_init((void *)&cxt->input_param, (void *)&output_param, &cxt->lib_handle);
		if (!ret) {
			cxt->cur_gain = output_param.gain;
			cxt->cur_ct = output_param.ct;
			cxt->cur_gain_balanced = output_param.gain_balanced;
			cxt->hw_cfg = output_param.hw_cfg;
		}
	}

	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int awbctrl_deinit_lib(cmr_handle awb_ctrl_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awbctrl_context                      *cxt = (struct awbctrl_context *)awb_ctrl_handle;

	if (cxt->awb_adpt_ops->adpt_deinit) {
		ret = cxt->awb_adpt_ops->adpt_deinit(cxt->lib_handle);
	}
	ISP_LOGI("done %ld", ret);

	return ret;
}

awb_ioctrl_fun awbctrl_get_ioctrl_fun(enum awb_ctrl_cmd cmd)
{
	awb_ioctrl_fun                               io_ctrl = NULL;
	cmr_u32                                      total_num = AWB_CTRL_CMD_MAX;
	cmr_u32                                      i = 0;

	for (i = 0; i < total_num; i++) {
		if (cmd == s_awbctrl_func[i].cmd) {
			io_ctrl = s_awbctrl_func[i].ioctrl;
			break;
		}
	}
	return io_ctrl;
}

cmr_int awbctrl_ioctrl(cmr_handle awb_ctrl_handle, enum awb_ctrl_cmd cmd, void *input_ptr, void *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	awb_ioctrl_fun                              ioctrl_fun = NULL;
	struct awbctrl_context                      *cxt = (struct awbctrl_context *)awb_ctrl_handle;

	ioctrl_fun = awbctrl_get_ioctrl_fun(cmd);
	if (NULL != ioctrl_fun) {
		ret = ioctrl_fun(awb_ctrl_handle, input_ptr, output_ptr);
	} else {
		ISP_LOGI("ioctrl fun is NULL, cmd = %d", cmd);
	}
exit:
	return ret;
}

cmr_int awbctrl_process(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awbctrl_context                      *cxt = (struct awbctrl_context *)awb_ctrl_handle;

	if (cxt->awb_adpt_ops->adpt_process) {
		ret = cxt->awb_adpt_ops->adpt_process(cxt->lib_handle, input_ptr, output_ptr);
	}
	return ret;
}

cmr_int awbctrl_set_scenemode(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awbctrl_context                      *cxt = (struct awbctrl_context *)awb_ctrl_handle;

	if (cxt->awb_adpt_ops->adpt_ioctrl) {
		ret = cxt->awb_adpt_ops->adpt_ioctrl(cxt->lib_handle, AWB_CTRL_CMD_SET_SCENE_MODE, input_ptr, output_ptr);
	}

	return ret;
}

cmr_int awbctrl_set_wbmode(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awbctrl_context                      *cxt = (struct awbctrl_context *)awb_ctrl_handle;

	if (cxt->awb_adpt_ops->adpt_ioctrl) {
		ret = cxt->awb_adpt_ops->adpt_ioctrl(cxt->lib_handle, AWB_CTRL_CMD_SET_WB_MODE, input_ptr, output_ptr);
	}

	return ret;
}

cmr_int awbctrl_set_flashmode(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awbctrl_context                      *cxt = (struct awbctrl_context *)awb_ctrl_handle;

	if (cxt->awb_adpt_ops->adpt_ioctrl) {
		ret = cxt->awb_adpt_ops->adpt_ioctrl(cxt->lib_handle, AWB_CTRL_CMD_SET_FLASH_MODE, input_ptr, output_ptr);
	}

	return ret;
}

cmr_int awbctrl_set_workmode(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awbctrl_context                      *cxt = (struct awbctrl_context *)awb_ctrl_handle;

	if (cxt->awb_adpt_ops->adpt_ioctrl) {
		ret = cxt->awb_adpt_ops->adpt_ioctrl(cxt->lib_handle, AWB_CTRL_CMD_SET_WORK_MODE, input_ptr, output_ptr);
	}

	return ret;
}

cmr_int awbctr_set_workmode(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awbctrl_context                      *cxt = (struct awbctrl_context *)awb_ctrl_handle;

	if (cxt->awb_adpt_ops->adpt_ioctrl) {
		ret = cxt->awb_adpt_ops->adpt_ioctrl(cxt->lib_handle, AWB_CTRL_CMD_SET_WORK_MODE, input_ptr, output_ptr);
	}

	return ret;
}

cmr_int awbctrl_set_faceinfo(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awbctrl_context                      *cxt = (struct awbctrl_context *)awb_ctrl_handle;

	if (cxt->awb_adpt_ops->adpt_ioctrl) {
		ret = cxt->awb_adpt_ops->adpt_ioctrl(cxt->lib_handle, AWB_CTRL_CMD_SET_FACE_INFO, input_ptr, output_ptr);
	}

	return ret;
}

cmr_int awbctrl_set_dzoom(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awbctrl_context                      *cxt = (struct awbctrl_context *)awb_ctrl_handle;

	if (cxt->awb_adpt_ops->adpt_ioctrl) {
		ret = cxt->awb_adpt_ops->adpt_ioctrl(cxt->lib_handle, AWB_CTRL_CMD_SET_DZOOM_FACTOR, input_ptr, output_ptr);
	}

	return ret;
}

cmr_int awbctrl_set_sof_frame_idx(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awbctrl_context                      *cxt = (struct awbctrl_context *)awb_ctrl_handle;

	if (cxt->awb_adpt_ops->adpt_ioctrl) {
		ret = cxt->awb_adpt_ops->adpt_ioctrl(cxt->lib_handle, AWB_CTRL_CMD_SET_SOF_FRAME_IDX, input_ptr, output_ptr);
	}

	return ret;
}

cmr_int awbctrl_set_bypass(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awbctrl_context                      *cxt = (struct awbctrl_context *)awb_ctrl_handle;

	if (cxt->awb_adpt_ops->adpt_ioctrl) {
		ret = cxt->awb_adpt_ops->adpt_ioctrl(cxt->lib_handle, AWB_CTRL_CMD_SET_BYPASS, input_ptr, output_ptr);
	}

	return ret;
}

cmr_int awbctrl_get_gain(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awbctrl_context                      *cxt = (struct awbctrl_context *)awb_ctrl_handle;

	if (cxt->awb_adpt_ops->adpt_ioctrl) {
		ret = cxt->awb_adpt_ops->adpt_ioctrl(cxt->lib_handle, AWB_CTRL_CMD_GET_GAIN, input_ptr, output_ptr);
	}

	return ret;
}

cmr_int awbctrl_set_flash_open_p(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awbctrl_context                      *cxt = (struct awbctrl_context *)awb_ctrl_handle;

	if (cxt->awb_adpt_ops->adpt_ioctrl) {
		ret = cxt->awb_adpt_ops->adpt_ioctrl(cxt->lib_handle, AWB_CTRL_CMD_FLASH_OPEN_P, input_ptr, output_ptr);
	}

	return ret;
}

cmr_int awbctrl_set_flash_open_m(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awbctrl_context                      *cxt = (struct awbctrl_context *)awb_ctrl_handle;

	if (cxt->awb_adpt_ops->adpt_ioctrl) {
		ret = cxt->awb_adpt_ops->adpt_ioctrl(cxt->lib_handle, AWB_CTRL_CMD_FLASH_OPEN_M, input_ptr, output_ptr);
	}

	return ret;
}

cmr_int awbctrl_set_flashing(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awbctrl_context                      *cxt = (struct awbctrl_context *)awb_ctrl_handle;

	if (cxt->awb_adpt_ops->adpt_ioctrl) {
		ret = cxt->awb_adpt_ops->adpt_ioctrl(cxt->lib_handle, AWB_CTRL_CMD_FLASHING, input_ptr, output_ptr);
	}

	return ret;
}

cmr_int awbctrl_set_flash_close(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awbctrl_context                      *cxt = (struct awbctrl_context *)awb_ctrl_handle;

	if (cxt->awb_adpt_ops->adpt_ioctrl) {
		ret = cxt->awb_adpt_ops->adpt_ioctrl(cxt->lib_handle, AWB_CTRL_CMD_FLASH_CLOSE, input_ptr, output_ptr);
	}

	//ret = awbctrl_set_unlock(awb_ctrl_handle, NULL, NULL);

	return ret;
}

static cmr_int awbctrl_set_lock(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awbctrl_context                      *cxt = (struct awbctrl_context *)awb_ctrl_handle;

	ISP_LOGI("lock_flag %d %d", cxt->lock_flag, cxt->lock_num);

	if (1 == cxt->lock_flag) {
		cxt->lock_num++;
		goto exit;
	}
	if (cxt->awb_adpt_ops->adpt_ioctrl) {
		ret = cxt->awb_adpt_ops->adpt_ioctrl(cxt->lib_handle, AWB_CTRL_CMD_LOCK, input_ptr, output_ptr);
		if (!ret) {
			cxt->lock_num++;
			cxt->lock_flag = 1;
		}
	}
exit:
	return ret;
}

static cmr_int awbctrl_set_unlock(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awbctrl_context                      *cxt = (struct awbctrl_context *)awb_ctrl_handle;

	ISP_LOGI("lock_flag %d %d", cxt->lock_flag, cxt->lock_num);

	if (0 == cxt->lock_flag) {
		cxt->lock_num = 0;
		goto exit;
	}
	if (cxt->lock_num > 0) {
		cxt->lock_num--;
	} else {
		cxt->lock_num = 0;
	}
	if (cxt->awb_adpt_ops->adpt_ioctrl && (0 == cxt->lock_num)) {
		ret = cxt->awb_adpt_ops->adpt_ioctrl(cxt->lib_handle, AWB_CTRL_CMD_UNLOCK, input_ptr, output_ptr);
		if (!ret) {
			cxt->lock_flag = 0;
		}
	}
exit:
	return ret;
}

cmr_int awbctrl_get_ct(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awbctrl_context                      *cxt = (struct awbctrl_context *)awb_ctrl_handle;

	if (cxt->awb_adpt_ops->adpt_ioctrl) {
		ret = cxt->awb_adpt_ops->adpt_ioctrl(cxt->lib_handle, AWB_CTRL_CMD_GET_CT, input_ptr, output_ptr);
	}

	return ret;
}

cmr_int awbctrl_set_flash_before_p(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awbctrl_context                      *cxt = (struct awbctrl_context *)awb_ctrl_handle;

	if (cxt->awb_adpt_ops->adpt_ioctrl) {
		ret = cxt->awb_adpt_ops->adpt_ioctrl(cxt->lib_handle, AWB_CTRL_CMD_FLASH_BEFORE_P, input_ptr, output_ptr);
	}

	return ret;
}

cmr_int awbctrl_set_flash_status(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awbctrl_context                      *cxt = (struct awbctrl_context *)awb_ctrl_handle;

	if (cxt->awb_adpt_ops->adpt_ioctrl) {
		ret = cxt->awb_adpt_ops->adpt_ioctrl(cxt->lib_handle, AWB_CTRL_CMD_SET_FLASH_STATUS, input_ptr, output_ptr);
	}

	return ret;
}

cmr_int awbctrl_set_ae_info(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awbctrl_context                      *cxt = (struct awbctrl_context *)awb_ctrl_handle;

	if (cxt->awb_adpt_ops->adpt_ioctrl) {
		ret = cxt->awb_adpt_ops->adpt_ioctrl(cxt->lib_handle, AWB_CTRL_CMD_SET_AE_REPORT, input_ptr, output_ptr);
	}

	return ret;
}

cmr_int awbctrl_set_af_info(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awbctrl_context                      *cxt = (struct awbctrl_context *)awb_ctrl_handle;

	if (cxt->awb_adpt_ops->adpt_ioctrl) {
		ret = cxt->awb_adpt_ops->adpt_ioctrl(cxt->lib_handle, AWB_CTRL_CMD_SET_AF_REPORT, input_ptr, output_ptr);
	}

	return ret;
}

cmr_int awbctrl_get_exif_debug_info(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awbctrl_context                      *cxt = (struct awbctrl_context *)awb_ctrl_handle;

	if (cxt->awb_adpt_ops->adpt_ioctrl) {
		ret = cxt->awb_adpt_ops->adpt_ioctrl(cxt->lib_handle, AWB_CTRL_CMD_GET_EXIF_DEBUG_INFO, input_ptr, output_ptr);
	}

	return ret;
}

cmr_int awbctrl_get_debug_info(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awbctrl_context                      *cxt = (struct awbctrl_context *)awb_ctrl_handle;

	if (cxt->awb_adpt_ops->adpt_ioctrl) {
		ret = cxt->awb_adpt_ops->adpt_ioctrl(cxt->lib_handle, AWB_CTRL_CMD_GET_DEBUG_INFO, input_ptr, output_ptr);
	}

	return ret;
}

cmr_int awbctrl_set_slave_iso(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awbctrl_context                      *cxt = (struct awbctrl_context *)awb_ctrl_handle;

	if (cxt->awb_adpt_ops->adpt_ioctrl) {
		ret = cxt->awb_adpt_ops->adpt_ioctrl(cxt->lib_handle, AWB_CTRL_CMD_SET_SLAVE_ISO_SPEED, input_ptr, output_ptr);
	}

	return ret;
}

cmr_int awbctrl_set_master(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awbctrl_context                      *cxt = (struct awbctrl_context *)awb_ctrl_handle;

	if (cxt->awb_adpt_ops->adpt_ioctrl) {
		ret = cxt->awb_adpt_ops->adpt_ioctrl(cxt->lib_handle, AWB_CTRL_CMD_SET_MASTER, input_ptr, output_ptr);
	}

	return ret;
}

cmr_int awbctrl_set_tuning_mode(cmr_handle awb_ctrl_handle, void *input_ptr, void *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awbctrl_context                      *cxt = (struct awbctrl_context *)awb_ctrl_handle;

	if (cxt->awb_adpt_ops->adpt_ioctrl) {
		ret = cxt->awb_adpt_ops->adpt_ioctrl(cxt->lib_handle, AWB_CTRL_CMD_SET_TUNING_MODE, input_ptr, output_ptr);
	}

	return ret;
}

/*************************************EXTERNAL FUNCTION ***************************************/
cmr_int awb_ctrl_init(struct awb_ctrl_init_in *input_ptr, struct awb_ctrl_init_out *output_ptr, cmr_handle *awb_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awbctrl_context                      *cxt = NULL;

	if (!input_ptr || !output_ptr || !awb_handle) {
		ISP_LOGE("init param is null, input_ptr is %p, output_ptr is %p", input_ptr, output_ptr);
		ret = ISP_PARAM_NULL;
		goto exit;
	}

	*awb_handle = NULL;
	cxt = (struct awbctrl_context *)malloc(sizeof(*cxt));
	if (NULL == cxt) {
		ISP_LOGE("failed to create awb ctrl context!");
		ret = ISP_ALLOC_ERROR;
		goto exit;
	}
	cmr_bzero(cxt, sizeof(*cxt));

	cxt->camera_id = input_ptr->camera_id;
	cxt->caller_handle = input_ptr->caller_handle;
	cxt->caller_callback = input_ptr->awb_cb;
	cxt->err_code = ISP_SUCCESS;
	cxt->input_param = *input_ptr;
	input_ptr->lib_config.product_id = 0;
	input_ptr->lib_config.version_id = 0;
	ret = adpt_get_ops(ADPT_LIB_AWB, &input_ptr->lib_config, &cxt->awb_adpt_ops);
	if (ret) {
		ISP_LOGE("failed to get adapter layer ret = %ld", ret);
		goto exit;
	}
#ifdef USE_THREAD
	ret = awbctrl_create_thread((cmr_handle)cxt);
	if (ret) {
		ISP_LOGE("failed to create thread ret = %ld", ret);
		goto error_c_thread;
	}
#endif
	sem_init(&cxt->sync_sm, 0, 1);
	sem_wait(&cxt->sync_sm);
	ret = awbctrl_init_lib((cmr_handle)cxt);
	if (ret) {
		ISP_LOGE("failed to init adapter layer ret = %ld", ret);
	} else {
		output_ptr->gain = cxt->cur_gain;
		output_ptr->ct = cxt->cur_ct;
		output_ptr->gain_balanced = cxt->cur_gain_balanced;
		output_ptr->hw_cfg = cxt->hw_cfg;
	}
	sem_post(&cxt->sync_sm);
exit:
	if (ret) {
//		awbctrl_destroy_thread(cxt);
		if (cxt) {
			free(cxt);
		}
	} else {
		cxt->is_inited = 1;
		*awb_handle = (cmr_handle)cxt;
	}
	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int awb_ctrl_deinit(cmr_handle awb_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awbctrl_context                      *cxt = (struct awbctrl_context *)awb_handle;

	ISP_CHECK_HANDLE_VALID(awb_handle);

	if (0 == cxt->is_inited) {
		ISP_LOGI("has been de-initialized");
		goto exit;
	}
#ifdef USE_THREAD
	ret = awbctrl_destroy_thread(awb_handle);
	if (ret) {
		ISP_LOGE("failed to destroy thr %ld", ret);
	}
	cxt->is_inited = 0;
#else
	sem_wait(&cxt->sync_sm);
	ret = awbctrl_deinit_lib(awb_handle);
	cxt->is_inited = 0;
	sem_post(&cxt->sync_sm);
#endif
	sem_destroy(&cxt->sync_sm);
	cmr_bzero(cxt, sizeof(*cxt));
	free((void *)awb_handle);
exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int awb_ctrl_ioctrl(cmr_handle awb_handle, enum awb_ctrl_cmd cmd, union awb_ctrl_cmd_in *input_ptr, union awb_ctrl_cmd_out *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awbctrl_context                      *cxt = (struct awbctrl_context *)awb_handle;
#ifdef USE_THREAD
	CMR_MSG_INIT(message);
#endif
	ISP_CHECK_HANDLE_VALID(awb_handle);

	if ((cmd >= AWB_CTRL_CMD_MAX) || (0 == cxt->is_inited)) {
		ISP_LOGI("input param is error %p", input_ptr);
		goto exit;
	}
#ifdef USE_THREAD
	message.msg_type = AWBCTRL_EVT_IOCTRL;
	message.sub_msg_type = cmd;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	message.alloc_flag = 0;
	message.data = (void *)input_ptr;
	ret = cmr_thread_msg_send(cxt->thread_cxt.ctrl_thr_handle, &message);
	if (ret) {
		ISP_LOGE("failed to send msg to main thr %ld", ret);
		goto exit;
	}
	if (output_ptr) {
		*output_ptr = cxt->ioctrl_out;
	}
#else
	sem_wait(&cxt->sync_sm);
	if (0 == cxt->is_inited) {
		sem_post(&cxt->sync_sm);
		goto exit;
	}
	ret = awbctrl_ioctrl(awb_handle, cmd, (void *)input_ptr, (void *)output_ptr);
	sem_post(&cxt->sync_sm);
#endif
exit:
	ISP_LOGV("cmd = %d, done %ld", cmd, ret);
	return ret;
}

cmr_int awb_ctrl_process(cmr_handle awb_handle, struct awb_ctrl_process_in *input_ptr, struct awb_ctrl_process_out *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awbctrl_context                      *cxt = (struct awbctrl_context *)awb_handle;
#ifdef USE_THREAD
	CMR_MSG_INIT(message);
#endif

	ISP_CHECK_HANDLE_VALID(awb_handle);

	if (!input_ptr || !output_ptr || (0 == cxt->is_inited)) {
		ISP_LOGI("input param is error %p", input_ptr);
		goto exit;
	}
#ifdef USE_THREAD
	message.msg_type = AWBCTRL_EVT_PROCESS;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	message.alloc_flag = 0;
	message.data = (void *)input_ptr;
	ret = cmr_thread_msg_send(cxt->thread_cxt.ctrl_thr_handle, &message);
	if (ret) {
		ISP_LOGE("failed to send msg to main thr %ld", ret);
		goto exit;
	}
	*output_ptr = cxt->process_out;
#else
	sem_wait(&cxt->sync_sm);
	if (0 == cxt->is_inited) {
		sem_post(&cxt->sync_sm);
		goto exit;
	}
	ret = awbctrl_process(awb_handle, (void *)input_ptr, (void *)output_ptr);
	sem_post(&cxt->sync_sm);
#endif
exit:
	ISP_LOGV("done %ld", ret);
	return ret;
}
