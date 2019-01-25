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

#include "af_ctrl.h"
#include <cutils/properties.h>

#define AFCTRL_EVT_BASE				0x2000
#define AFCTRL_EVT_INIT				AFCTRL_EVT_BASE
#define AFCTRL_EVT_DEINIT			(AFCTRL_EVT_BASE + 1)
#define AFCTRL_EVT_IOCTRL			(AFCTRL_EVT_BASE + 2)
#define AFCTRL_EVT_PROCESS			(AFCTRL_EVT_BASE + 3)
#define AFCTRL_EVT_EXIT				(AFCTRL_EVT_BASE + 4)
#define AFCTRL_THREAD_QUEUE_NUM                 (100)

struct afctrl_work_lib {
	cmr_handle lib_handle;
	struct adpt_ops_type *adpt_ops;
};

struct afctrl_cxt {
	cmr_handle thr_handle;
	cmr_handle caller_handle;
	struct afctrl_work_lib work_lib;
	struct afctrl_calc_out proc_out;
	af_ctrl_cb af_set_cb;
};

struct af_ctrl_msg_ctrl {
	cmr_int cmd;
	void *in;
	void *out;
};

static uint32_t af_get_otp(void *handle_af, uint16_t * inf, uint16_t * macro)
{
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;

	if (cxt_ptr->af_set_cb) {
		cxt_ptr->af_set_cb(cxt_ptr->caller_handle, AF_CB_CMD_GET_LENS_OTP, inf, macro);
	}

	return ISP_SUCCESS;
}

static uint32_t af_set_motor_pos(void *handle_af, cmr_u16 in_param)
{
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;

	if (cxt_ptr->af_set_cb) {
		cxt_ptr->af_set_cb(cxt_ptr->caller_handle, AF_CB_CMD_SET_MOTOR_POS, (void *)&in_param, NULL);
	}

	return ISP_SUCCESS;
}

static uint32_t af_get_motor_pos(void *handle_af, cmr_u16 * in_param)
{
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;

	if (cxt_ptr->af_set_cb) {
		cxt_ptr->af_set_cb(cxt_ptr->caller_handle, AF_CB_CMD_GET_MOTOR_POS, (void *)in_param, NULL);
	}

	return ISP_SUCCESS;
}

static uint32_t af_set_motor_bestmode(void *handle_af)
{
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;

	if (cxt_ptr->af_set_cb) {
		cxt_ptr->af_set_cb(cxt_ptr->caller_handle, AF_CB_CMD_SET_MOTOR_BESTMODE, NULL, NULL);
	}

	return ISP_SUCCESS;
}

static uint32_t af_set_vcm_test_mode(void *handle_af, char *vcm_mode)
{
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;

	if (cxt_ptr->af_set_cb) {
		cxt_ptr->af_set_cb(cxt_ptr->caller_handle, AF_CB_CMD_SET_VCM_TEST_MODE, vcm_mode, NULL);
	}

	return ISP_SUCCESS;
}

static uint32_t af_get_vcm_test_mode(void *handle_af)
{
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;

	if (cxt_ptr->af_set_cb) {
		cxt_ptr->af_set_cb(cxt_ptr->caller_handle, AF_CB_CMD_GET_VCM_TEST_MODE, NULL, NULL);
	}

	return ISP_SUCCESS;
}

static cmr_s32 af_end_notice(void *handle_af, struct af_result_param *in_param)
{
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;
	struct af_notice af_notice;

	af_notice.mode = AF_MOVE_END;
	af_notice.valid_win = in_param->suc_win;
	af_notice.focus_type = in_param->focus_type;
	if (cxt_ptr->af_set_cb) {
		cxt_ptr->af_set_cb(cxt_ptr->caller_handle, AF_CB_CMD_SET_END_NOTICE, (void *)&af_notice, NULL);
	}

	return ISP_SUCCESS;
}

static cmr_s32 af_start_notice(void *handle_af, struct af_result_param *in_param)
{
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;
	struct af_notice af_notice;

	af_notice.mode = AF_MOVE_START;
	af_notice.valid_win = 0x00;
	af_notice.focus_type = in_param->focus_type;
	if (cxt_ptr->af_set_cb) {
		cxt_ptr->af_set_cb(cxt_ptr->caller_handle, AF_CB_CMD_SET_START_NOTICE, (void *)&af_notice, NULL);
	}

	return ISP_SUCCESS;
}

static cmr_s32 af_lock_module(void *handle_af, cmr_int af_locker_type)
{
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;
	cmr_int rtn = ISP_SUCCESS;

	if (NULL == cxt_ptr->af_set_cb) {
		ISP_LOGE("fail to check param!");
		return ISP_PARAM_NULL;
	}

	switch (af_locker_type) {
	case AF_LOCKER_AE:
		rtn = cxt_ptr->af_set_cb(cxt_ptr->caller_handle, AF_CB_CMD_SET_AE_LOCK, NULL, NULL);
		break;
	case AF_LOCKER_AE_CAF:
		rtn = cxt_ptr->af_set_cb(cxt_ptr->caller_handle, AF_CB_CMD_SET_AE_CAF_LOCK, NULL, NULL);
		break;
	case AF_LOCKER_AWB:
		rtn = cxt_ptr->af_set_cb(cxt_ptr->caller_handle, AF_CB_CMD_SET_AWB_LOCK, NULL, NULL);
		break;
	case AF_LOCKER_LSC:
		rtn = cxt_ptr->af_set_cb(cxt_ptr->caller_handle, AF_CB_CMD_SET_LSC_LOCK, NULL, NULL);
		break;
	case AF_LOCKER_NLM:
		rtn = cxt_ptr->af_set_cb(cxt_ptr->caller_handle, AF_CB_CMD_SET_NLM_LOCK, NULL, NULL);
		break;
	default:
		ISP_LOGE("fail to do af lock,af_locker_type is not supported!");
		break;
	}

	return rtn;
}

static cmr_s32 af_unlock_module(void *handle_af, cmr_int af_locker_type)
{
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;
	cmr_int rtn = ISP_SUCCESS;

	if (NULL == cxt_ptr->af_set_cb) {
		ISP_LOGE("fail to check param!");
		return ISP_PARAM_NULL;
	}

	switch (af_locker_type) {
	case AF_LOCKER_AE:
		rtn = cxt_ptr->af_set_cb(cxt_ptr->caller_handle, AF_CB_CMD_SET_AE_UNLOCK, NULL, NULL);
		break;
	case AF_LOCKER_AE_CAF:
		rtn = cxt_ptr->af_set_cb(cxt_ptr->caller_handle, AF_CB_CMD_SET_AE_CAF_UNLOCK, NULL, NULL);
		break;
	case AF_LOCKER_AWB:
		rtn = cxt_ptr->af_set_cb(cxt_ptr->caller_handle, AF_CB_CMD_SET_AWB_UNLOCK, NULL, NULL);
		break;
	case AF_LOCKER_LSC:
		rtn = cxt_ptr->af_set_cb(cxt_ptr->caller_handle, AF_CB_CMD_SET_LSC_UNLOCK, NULL, NULL);
		break;
	case AF_LOCKER_NLM:
		rtn = cxt_ptr->af_set_cb(cxt_ptr->caller_handle, AF_CB_CMD_SET_NLM_UNLOCK, NULL, NULL);
		break;
	default:
		ISP_LOGE("fail to unlock, af_unlocker_type is not supported!");
		break;
	}

	return rtn;
}

static cmr_s32 af_set_monitor(void *handle_af, struct af_monitor_set *in_param, cmr_u32 cur_envi)
{
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;

	if (cxt_ptr->af_set_cb) {
		cxt_ptr->af_set_cb(cxt_ptr->caller_handle, AF_CB_CMD_SET_MONITOR, (void *)in_param, (void *)&cur_envi);
	}

	return ISP_SUCCESS;
}

static cmr_s32 af_set_monitor_win(void *handle_af, struct af_monitor_win *in_param)
{
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;

	if (cxt_ptr->af_set_cb) {
		cxt_ptr->af_set_cb(cxt_ptr->caller_handle, AF_CB_CMD_SET_MONITOR_WIN, (void *)(in_param->win_pos), NULL);
	}

	return ISP_SUCCESS;
}

static cmr_s32 af_get_monitor_win_num(void *handle_af, cmr_u32 * win_num)
{
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;

	if (cxt_ptr->af_set_cb) {
		cxt_ptr->af_set_cb(cxt_ptr->caller_handle, AF_CB_CMD_GET_MONITOR_WIN_NUM, (void *)win_num, NULL);
	}

	return ISP_SUCCESS;
}

static cmr_s32 af_monitor_bypass(void *handle_af, cmr_u32 * bypass)
{
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;

	if (cxt_ptr->af_set_cb) {
		cxt_ptr->af_set_cb(cxt_ptr->caller_handle, AF_CB_CMD_SET_AFM_BYPASS, (void *)bypass, NULL);
	}

	return ISP_SUCCESS;
}

static cmr_s32 af_monitor_skip_num(void *handle_af, cmr_u32 * afm_skip_num)
{
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;

	if (cxt_ptr->af_set_cb) {
		cxt_ptr->af_set_cb(cxt_ptr->caller_handle, AF_CB_CMD_SET_AFM_SKIP_NUM, (void *)afm_skip_num, NULL);
	}

	return ISP_SUCCESS;
}

static cmr_s32 af_monitor_mode(void *handle_af, cmr_u32 * afm_mode)
{
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;

	if (cxt_ptr->af_set_cb) {
		cxt_ptr->af_set_cb(cxt_ptr->caller_handle, AF_CB_CMD_SET_AFM_MODE, (void *)afm_mode, NULL);
	}

	return ISP_SUCCESS;
}

static cmr_s32 af_monitor_iir_nr_cfg(void *handle_af, void *af_iir_nr)
{
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;

	if (cxt_ptr->af_set_cb) {
		cxt_ptr->af_set_cb(cxt_ptr->caller_handle, AF_CB_CMD_SET_AFM_NR_CFG, (void *)af_iir_nr, NULL);
	}

	return ISP_SUCCESS;
}

static cmr_s32 af_monitor_module_cfg(void *handle_af, void *af_enhanced_module)
{
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;

	if (cxt_ptr->af_set_cb) {
		cxt_ptr->af_set_cb(cxt_ptr->caller_handle, AF_CB_CMD_SET_AFM_MODULES_CFG, (void *)af_enhanced_module, NULL);
	}

	return ISP_SUCCESS;
}

static cmr_s32 af_get_system_time(cmr_handle handle_af, cmr_u32 * sec, cmr_u32 * usec)
{
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;

	if (cxt_ptr->af_set_cb) {
		cxt_ptr->af_set_cb(cxt_ptr->caller_handle, AF_CB_CMD_GET_SYSTEM_TIME, sec, usec);
	}

	return 0;
}

// SharkLE Only ++
static cmr_s32 af_set_pulse_line(cmr_handle handle_af, cmr_u32 line)
{
	UNUSED(handle_af);
	UNUSED(line);
#ifdef CONFIG_ISP_2_3
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;

	if (cxt_ptr->af_set_cb) {
		cxt_ptr->af_set_cb(cxt_ptr->caller_handle, AF_CB_CMD_SET_PULSE_LINE, (void *)&line, NULL);
	}
#endif
	return 0;
}

static cmr_s32 af_set_next_vcm_pos(cmr_handle handle_af, cmr_u32 pos)
{
	UNUSED(handle_af);
	UNUSED(pos);
#ifdef CONFIG_ISP_2_3
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;
	ISP_LOGD("af_set_next_vcm_pos = %d", pos);

	if (cxt_ptr->af_set_cb) {
		cxt_ptr->af_set_cb(cxt_ptr->caller_handle, AF_CB_CMD_SET_NEXT_VCM_POS, (void *)&pos, NULL);
	}
#endif
	return 0;
}

static cmr_s32 af_set_pulse_log(cmr_handle handle_af, cmr_u32 flag)
{
	UNUSED(handle_af);
	UNUSED(flag);
#ifdef CONFIG_ISP_2_3
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;
	// ISP_LOGD("af_set_pulse_log = %d",flag);
	if (cxt_ptr->af_set_cb) {
		cxt_ptr->af_set_cb(cxt_ptr->caller_handle, AF_CB_CMD_SET_PULSE_LOG, (void *)&flag, NULL);
	}
#endif
	return 0;
}

static cmr_s32 af_set_clear_next_vcm_pos(cmr_handle handle_af)
{
	UNUSED(handle_af);
#ifdef CONFIG_ISP_2_3
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;

	if (cxt_ptr->af_set_cb) {
		cxt_ptr->af_set_cb(cxt_ptr->caller_handle, AF_CB_CMD_SET_CLEAR_NEXT_VCM_POS, NULL, NULL);
	}
#endif
	return 0;
}

// SharkLE Only --

static cmr_int afctrl_process(struct afctrl_cxt *cxt_ptr, struct afctrl_calc_in *in_ptr, struct af_result_param *out_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct afctrl_work_lib *lib_ptr = NULL;
	cmr_s8 value[PROPERTY_VALUE_MAX];
	cmr_u16 motor_pos = 0;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param!");
		goto exit;
	}

	property_get("persist.sys.isp.vcm.tuning.mode", (char *)value, "0");
	if (1 == atoi((char *)value)) {
		property_get("persist.sys.isp.vcm.position", (char *)value, "0");
		motor_pos = (cmr_u16) atoi((char *)value);
		af_set_motor_pos(cxt_ptr, motor_pos);
		goto exit;
	}

	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_process) {
		rtn = lib_ptr->adpt_ops->adpt_process(lib_ptr->lib_handle, in_ptr, out_ptr);
	} else {
		ISP_LOGI("ioctrl fun is NULL");
	}
  exit:
	ISP_LOGV("done %ld", rtn);
	return rtn;
}

static cmr_int afctrl_deinit_adpt(struct afctrl_cxt *cxt_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct afctrl_work_lib *lib_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}

	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_deinit) {
		rtn = lib_ptr->adpt_ops->adpt_deinit(lib_ptr->lib_handle, NULL, NULL);
		lib_ptr->lib_handle = NULL;
	} else {
		ISP_LOGI("adpt_deinit fun is NULL");
	}

	ISP_LOGI(" af_deinit is OK!");
  exit:
	ISP_LOGI("done %ld", rtn);
	return rtn;
}

static cmr_int afctrl_evtctrl(cmr_handle handle_af, cmr_int cmd, void *in_ptr, void *out_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;
	struct afctrl_work_lib *lib_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGV("fail to check param is NULL!");
		goto exit;
	}

	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_ioctrl) {
		rtn = lib_ptr->adpt_ops->adpt_ioctrl(lib_ptr->lib_handle, cmd, in_ptr, out_ptr);
	} else {
		ISP_LOGI("ioctrl fun is NULL");
	}

  exit:
	ISP_LOGV("cmd = %ld,done %ld", cmd, rtn);
	return rtn;
}

static cmr_int afctrl_ctrl_thr_proc(struct cmr_msg *message, void *p_data)
{
	cmr_int rtn = ISP_SUCCESS;
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)p_data;

	if (!message || !p_data) {
		ISP_LOGE("fail to check param");
		goto exit;
	}
	ISP_LOGV("message.msg_type 0x%x, data %p", message->msg_type, message->data);

	switch (message->msg_type) {
	case AFCTRL_EVT_INIT:
		break;
	case AFCTRL_EVT_DEINIT:
		rtn = afctrl_deinit_adpt(cxt_ptr);
		break;
	case AFCTRL_EVT_EXIT:
		break;
	case AFCTRL_EVT_IOCTRL:
		rtn = afctrl_evtctrl(cxt_ptr, ((struct af_ctrl_msg_ctrl *)message->data)->cmd, ((struct af_ctrl_msg_ctrl *)message->data)->in, ((struct af_ctrl_msg_ctrl *)message->data)->out);
		break;
	case AFCTRL_EVT_PROCESS:
		rtn = afctrl_process(cxt_ptr, (struct afctrl_calc_in *)message->data, (struct af_result_param *)&cxt_ptr->proc_out);
		break;
	default:
		ISP_LOGE("fail to proc ,don't support msg");
		break;
	}

  exit:
	ISP_LOGV("done %ld", rtn);
	return rtn;
}

static cmr_int afctrl_create_thread(struct afctrl_cxt *cxt_ptr)
{
	cmr_int rtn = ISP_SUCCESS;

	rtn = cmr_thread_create(&cxt_ptr->thr_handle, AFCTRL_THREAD_QUEUE_NUM, afctrl_ctrl_thr_proc, (void *)cxt_ptr);
	if (rtn) {
		ISP_LOGE("fail to create ctrl thread");
		rtn = ISP_ERROR;
		goto exit;
	}
	rtn = cmr_thread_set_name(cxt_ptr->thr_handle, "afctrl");
	if (CMR_MSG_SUCCESS != rtn) {
		ISP_LOGE("fail to set afctrl name");
		rtn = CMR_MSG_SUCCESS;
	}
  exit:
	ISP_LOGI("af_ctrl thread rtn %ld", rtn);
	return rtn;
}

static cmr_int afctrl_destroy_thread(struct afctrl_cxt *cxt_ptr)
{
	cmr_int rtn = ISP_SUCCESS;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param");
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

static cmr_int afctrl_init_lib(struct afctrl_cxt *cxt_ptr, struct afctrl_init_in *in_ptr, struct afctrl_init_out *out_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct afctrl_work_lib *lib_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param!");
		goto exit;
	}

	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_init) {
		lib_ptr->lib_handle = lib_ptr->adpt_ops->adpt_init(in_ptr, out_ptr);
		if (NULL == lib_ptr->lib_handle) {
			ISP_LOGE("fail to check af lib_handle!");
			ret = ISP_ERROR;
		}
	} else {
		ISP_LOGI("adpt_init fun is NULL");
	}
  exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

static cmr_int afctrl_init_adpt(struct afctrl_cxt *cxt_ptr, struct afctrl_init_in *in_ptr, struct afctrl_init_out *out_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	ISP_LOGI("E %ld", rtn);

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param!");
		goto exit;
	}

	/* find vendor adpter */
	rtn = adpt_get_ops(ADPT_LIB_AF, &in_ptr->lib_param, &cxt_ptr->work_lib.adpt_ops);
	if (rtn) {
		ISP_LOGE("fail to get adapter layer ret = %ld", rtn);
		goto exit;
	}

	rtn = afctrl_init_lib(cxt_ptr, in_ptr, out_ptr);
  exit:
	ISP_LOGI("done %ld", rtn);
	return rtn;
}

cmr_int af_ctrl_init(struct afctrl_init_in * input_ptr, cmr_handle * handle_af)
{
	cmr_int rtn = ISP_SUCCESS;
	struct afctrl_cxt *cxt_ptr = NULL;
	struct afctrl_init_out result;

	if (!input_ptr->is_supoprt) {
		ISP_LOGV("sensor don't support AF");
		return rtn;
	}
	memset((void *)&result, 0, sizeof(result));
	input_ptr->cb_ops.start_notice = af_start_notice;
	input_ptr->cb_ops.end_notice = af_end_notice;
	input_ptr->cb_ops.set_monitor = af_set_monitor;
	input_ptr->cb_ops.set_monitor_win = af_set_monitor_win;
	input_ptr->cb_ops.get_monitor_win_num = af_get_monitor_win_num;
	input_ptr->cb_ops.lock_module = af_lock_module;
	input_ptr->cb_ops.unlock_module = af_unlock_module;
	input_ptr->cb_ops.af_get_otp = af_get_otp;
	input_ptr->cb_ops.af_set_motor_pos = af_set_motor_pos;
	input_ptr->cb_ops.af_get_motor_pos = af_get_motor_pos;
	input_ptr->cb_ops.af_set_motor_bestmode = af_set_motor_bestmode;
	input_ptr->cb_ops.af_set_test_vcm_mode = af_set_vcm_test_mode;
	input_ptr->cb_ops.af_get_test_vcm_mode = af_get_vcm_test_mode;
	input_ptr->cb_ops.af_monitor_bypass = af_monitor_bypass;
	input_ptr->cb_ops.af_monitor_skip_num = af_monitor_skip_num;
	input_ptr->cb_ops.af_monitor_mode = af_monitor_mode;
	input_ptr->cb_ops.af_monitor_iir_nr_cfg = af_monitor_iir_nr_cfg;
	input_ptr->cb_ops.af_monitor_module_cfg = af_monitor_module_cfg;
	input_ptr->cb_ops.af_get_system_time = af_get_system_time;

	// SharkLE Only ++
	input_ptr->cb_ops.af_set_pulse_line = af_set_pulse_line;
	input_ptr->cb_ops.af_set_next_vcm_pos = af_set_next_vcm_pos;
	input_ptr->cb_ops.af_set_pulse_log = af_set_pulse_log;
	input_ptr->cb_ops.af_set_clear_next_vcm_pos = af_set_clear_next_vcm_pos;
	// SharkLE Only --

	cxt_ptr = (struct afctrl_cxt *)malloc(sizeof(*cxt_ptr));
	if (NULL == cxt_ptr) {
		ISP_LOGE("fail to create af ctrl context!");
		rtn = ISP_ALLOC_ERROR;
		goto exit;
	}
	memset((void *)cxt_ptr, 0x00, sizeof(*cxt_ptr));

	input_ptr->caller = (void *)cxt_ptr;
	cxt_ptr->af_set_cb = input_ptr->af_set_cb;
	cxt_ptr->caller_handle = input_ptr->caller_handle;
	rtn = afctrl_create_thread(cxt_ptr);
	if (rtn) {
		goto exit;
	}

	rtn = afctrl_init_adpt(cxt_ptr, input_ptr, &result);
	if (rtn) {
		goto error_adpt_init;
	}

	*handle_af = (cmr_handle) cxt_ptr;

	ISP_LOGI(" done %ld", rtn);
	return rtn;

  error_adpt_init:
	rtn = afctrl_destroy_thread(cxt_ptr);
	if (rtn) {
		ISP_LOGE("fail to destroy afctrl thr %ld", rtn);
	}

  exit:
	if (cxt_ptr) {
		free((void *)cxt_ptr);
		cxt_ptr = NULL;
	}

	return ISP_SUCCESS;
}

cmr_int af_ctrl_deinit(cmr_handle * handle_af)
{
	cmr_int rtn = ISP_SUCCESS;
	struct afctrl_cxt *cxt_ptr = *handle_af;
	CMR_MSG_INIT(message);

	if (!cxt_ptr) {
		ISP_LOGV("sensor don't support AF");
		return rtn;
	}
	message.msg_type = AFCTRL_EVT_DEINIT;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	message.alloc_flag = 0;
	message.data = NULL;
	rtn = cmr_thread_msg_send(cxt_ptr->thr_handle, &message);
	if (rtn) {
		ISP_LOGE("fail to send msg to main thr %ld", rtn);
		goto exit;
	}

	rtn = afctrl_destroy_thread(cxt_ptr);
	if (rtn) {
		ISP_LOGE("fail to destroy afctrl thr %ld", rtn);
		goto exit;
	}

  exit:
	if (cxt_ptr) {
		free((void *)cxt_ptr);
		*handle_af = NULL;
	}

	ISP_LOGI("done %ld", rtn);
	return rtn;
}

cmr_int af_ctrl_process(cmr_handle handle_af, void *in_ptr, struct afctrl_calc_out * result)
{
	cmr_int rtn = ISP_SUCCESS;
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;

	if (!handle_af) {
		ISP_LOGV("sensor don't support AF");
		return rtn;
	}
	CMR_MSG_INIT(message);

	message.data = malloc(sizeof(struct afctrl_calc_in));
	if (!message.data) {
		ISP_LOGE("fail to malloc msg");
		rtn = ISP_ALLOC_ERROR;
		goto exit;
	}
	memcpy(message.data, (void *)in_ptr, sizeof(struct afctrl_calc_in));
	message.alloc_flag = 1;

	message.msg_type = AFCTRL_EVT_PROCESS;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	rtn = cmr_thread_msg_send(cxt_ptr->thr_handle, &message);

	if (rtn) {
		ISP_LOGE("fail to send msg to main thr %ld", rtn);
		if (message.data)
			free(message.data);
		goto exit;
	}

	if (result) {
		*result = cxt_ptr->proc_out;
	}

  exit:
	ISP_LOGV("done %ld", rtn);
	return rtn;
}

cmr_int af_ctrl_ioctrl(cmr_handle handle_af, cmr_int cmd, void *in_ptr, void *out_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;
	struct af_ctrl_msg_ctrl msg_ctrl;

	if (!handle_af) {
		ISP_LOGV("sensor don't support AF");
		return rtn;
	}
	CMR_MSG_INIT(message);
	msg_ctrl.cmd = cmd;
	msg_ctrl.in = in_ptr;
	msg_ctrl.out = out_ptr;
	message.data = &msg_ctrl;
	message.msg_type = AFCTRL_EVT_IOCTRL;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	rtn = cmr_thread_msg_send(cxt_ptr->thr_handle, &message);

  exit:
	ISP_LOGV("done %ld", rtn);
	return rtn;
}
