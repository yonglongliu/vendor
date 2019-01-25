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
#define LOG_TAG "alk_adpt_awb"

#include <stdlib.h>
#include "cutils/properties.h"
#include <dlfcn.h>
#include <unistd.h>
#include "isp_type.h"
#include "awb_ctrl_types.h"
#include "isp_adpt.h"
#include "allib_awb.h"
#include "allib_awb_errcode.h"
#include "alwrapper_awb.h"
#include "isp_mlog.h"
#include "isp_bridge.h"

/**************************************** MACRO DEFINE *****************************************/
#define LIBRARY_PATH "libalAWBLib.so"
#define AWB_EXIF_DEBUG_INFO_SIZE 5120
#define AWB_OTP_DEFAULT_R_GAIN 2007
#define AWB_OTP_DEFAULT_G_GAIN 1000
#define AWB_OTP_DEFAULT_B_GAIN 1460


/************************************* INTERNAL DATA TYPE ***************************************/
struct awb_altek_lib_ops {
	cmr_int (*load_func)(struct allib_awb_runtime_obj_t *awb_run_obj);
	void (*get_version)(struct allib_awb_lib_version_t *version);
};

struct awb_altek_context {
	cmr_u32 camera_id;
	cmr_u32 is_inited;
	cmr_u8 is_master;
	cmr_u32 wb_mode;
	cmr_u32 scene_mode;
	cmr_handle caller_handle;
	cmr_handle altek_lib_handle;
	cmr_u32 is_lock;
	cmr_u32 is_bypass;
	awb_callback callback;
	void *tuning_param[ISP_INDEX_MAX];
	void *tuning_param_slv[ISP_INDEX_MAX];
	struct awb_altek_lib_ops ops;
	struct isp_awb_gain cur_gain;
	struct awb_ctrl_work_param work_mode;
	struct allib_awb_runtime_obj_t lib_func;
	struct allib_awb_output_data_t cur_process_out;
	struct al3awrapper_stats_awb_t awb_stats;
	struct awb_ctrl_recover_param recover;
	struct awb_flash_info flash_info;
	struct report_update_t report;
	cmr_s8 exif_debug_info[AWB_EXIF_DEBUG_INFO_SIZE];
};
/*********************************************END***********************************************/
static cmr_int awbaltek_convert_wb_mode(enum awb_ctrl_wb_mode input_mode);
/*************************************INTERNAK FUNCTION ****************************************/
static cmr_int awbaltek_load_library(cmr_handle adpt_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awb_altek_context                    *cxt = (struct awb_altek_context *)adpt_handle;

	cxt->altek_lib_handle = dlopen(LIBRARY_PATH, RTLD_NOW);
	if (!cxt->altek_lib_handle) {
		ret = -ISP_ERROR;
		ISP_LOGE("failed to dlopen");
		goto exit;
	}
	cxt->ops.load_func = dlsym(cxt->altek_lib_handle, "allib_awb_loadfunc");
	if (!cxt->ops.load_func) {
		ret = -ISP_ERROR;
		ISP_LOGE("failed to dlsym load_func");
		goto exit;
	}

	cxt->ops.get_version = dlsym(cxt->altek_lib_handle, "allib_awb_getlib_version");
	if (!cxt->ops.load_func) {
		ret = -ISP_ERROR;
		ISP_LOGE("failed to dlsym get_version");
		goto exit;
	}
exit:
	if (ret) {
		if (cxt->altek_lib_handle) {
			dlclose(cxt->altek_lib_handle);
		}
	}
	return ret;
}

static cmr_int awbaltek_unload_library(cmr_handle adpt_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awb_altek_context                    *cxt = (struct awb_altek_context *)adpt_handle;

	if (cxt->altek_lib_handle) {
		dlclose(cxt->altek_lib_handle);
		cxt->altek_lib_handle = NULL;
	}
	ISP_LOGV("done %ld", ret);
	return ret;
}

static cmr_int awbaltek_set_wb_mode(cmr_handle adpt_handle, union awb_ctrl_cmd_in *input_ptr, union awb_ctrl_cmd_out *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awb_altek_context                    *cxt = (struct awb_altek_context *)adpt_handle;
	struct allib_awb_set_parameter_t            input;

	UNUSED(output_ptr);
	cxt->wb_mode = input_ptr->wb_mode.wb_mode;
	if (cxt->wb_mode == 0 && cxt->scene_mode != 0)
		return ISP_SUCCESS;
	input.type = alawb_set_param_awb_mode_setting;
	input.para.awb_mode_setting.wbmode_type = awbaltek_convert_wb_mode(cxt->wb_mode);
	input.para.awb_mode_setting.manual_ct = input_ptr->wb_mode.manual_ct;
	ret = (cmr_int)cxt->lib_func.set_param(&input, cxt->lib_func.awb);
	if (ret) {
		ISP_LOGE("failed to set wb mode 0x%lx", ret);
	}

	return ret;
}

static cmr_int awbaltek_set_dzoom(cmr_handle adpt_handle, union awb_ctrl_cmd_in *input_ptr, union awb_ctrl_cmd_out *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awb_altek_context                    *cxt = (struct awb_altek_context *)adpt_handle;
	struct allib_awb_set_parameter_t            input;

	UNUSED(output_ptr);
	input.type = alawb_set_param_dzoom_factor;
	input.para.dzoom_factor = input_ptr->dzoom_factor;
	ret = (cmr_int)cxt->lib_func.set_param(&input, cxt->lib_func.awb);
	if (ret) {
		ISP_LOGE("failed to set dzoom factor 0x%lx", ret);
	}

	return ret;
}

static cmr_int awbaltek_match_slave_awb_info(cmr_handle adpt_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awb_altek_context                    *cxt = (struct awb_altek_context *)adpt_handle;
	struct match_data_param                     match_param;
	struct allib_awb_output_data_t              match_output;

	if (cxt->work_mode.is_refocus && cxt->is_master) {
		/* match */
		ret = (cmr_int)cxt->lib_func.match(cxt->lib_func.awb, &match_output);
		if (ret)
			ISP_LOGE("failed to match awb info %ld", ret);

		cmr_bzero(&match_param, sizeof(match_param));
		match_param.awb_data.gain.r = match_output.wbgain.r_gain;
		match_param.awb_data.gain.g = match_output.wbgain.g_gain;
		match_param.awb_data.gain.b = match_output.wbgain.b_gain;
		match_param.awb_data.gain_balanced.r = match_output.wbgain_balanced.r_gain;
		match_param.awb_data.gain_balanced.g = match_output.wbgain_balanced.g_gain;
		match_param.awb_data.gain_balanced.b = match_output.wbgain_balanced.b_gain;
		match_param.awb_data.gain_flash_off.r = match_output.wbgain_flash_off.r_gain;
		match_param.awb_data.gain_flash_off.g = match_output.wbgain_flash_off.g_gain;
		match_param.awb_data.gain_flash_off.b = match_output.wbgain_flash_off.b_gain;
		match_param.awb_data.gain_capture.r = match_output.wbgain_capture.r_gain;
		match_param.awb_data.gain_capture.g = match_output.wbgain_capture.g_gain;
		match_param.awb_data.gain_capture.b = match_output.wbgain_capture.b_gain;
		match_param.awb_data.ct = match_output.color_temp;
		match_param.awb_data.ct_flash_off = match_output.color_temp_flash_off;
		match_param.awb_data.ct_capture = match_output.color_temp_capture;
		match_param.awb_data.is_update = match_output.awb_update;
		match_param.awb_data.light_source = match_output.light_source;
		match_param.awb_data.awb_states = match_output.awb_states;
		match_param.awb_data.awb_decision = match_output.awb_decision;
		isp_br_ioctrl(cxt->camera_id, SET_MATCH_AWB_DATA, &match_param.awb_data, NULL);
		ISP_LOGI("camera_id %d set match_data:%d %d %d,ct:%d", cxt->camera_id,
			 match_param.awb_data.gain.r,
			 match_param.awb_data.gain.g,
			 match_param.awb_data.gain.b,
			 match_param.awb_data.ct);
		isp_mlog(AWB_FILE, "master gain.r:%d, gain.g:%d, gain.b:%d\n, gain_balanced.r:%d, gain_balanced.g:%d, gain_balanced.b:%d\n,ct:%d,\
			\n\n\nslave gain.r:%d, gain.g:%d, gain.b:%d\n, gain_balanced.r:%d, gain_balanced.g:%d, gain_balanced.b:%d\n,ct:%d",
			cxt->cur_process_out.wbgain.r_gain,
			cxt->cur_process_out.wbgain.g_gain,
			cxt->cur_process_out.wbgain.b_gain,
			cxt->cur_process_out.wbgain_balanced.r_gain,
			cxt->cur_process_out.wbgain_balanced.g_gain,
			cxt->cur_process_out.wbgain_balanced.b_gain,
			cxt->cur_process_out.color_temp,
			match_param.awb_data.gain.r,
			match_param.awb_data.gain.g,
			match_param.awb_data.gain.b,
			match_param.awb_data.gain_balanced.r,
			match_param.awb_data.gain_balanced.g,
			match_param.awb_data.gain_balanced.b,
			match_param.awb_data.ct);
	}

	return ret;
}

static cmr_int awbaltek_set_sof_frame_id(cmr_handle adpt_handle, union awb_ctrl_cmd_in *input_ptr, union awb_ctrl_cmd_out *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awb_altek_context                    *cxt = (struct awb_altek_context *)adpt_handle;
	struct allib_awb_set_parameter_t            input;

	UNUSED(output_ptr);
	input.type = alawb_set_param_sys_sof_frame_idx;
	input.para.sys_sof_frame_idx = input_ptr->sof_frame_idx;
	ret = (cmr_int)cxt->lib_func.set_param(&input, cxt->lib_func.awb);
	if (ret) {
		ISP_LOGE("failed to set sof frame id 0x%lx", ret);
	}

	awbaltek_match_slave_awb_info(cxt);

	return ret;
}

static cmr_int awbaltek_set_lock(cmr_handle adpt_handle, union awb_ctrl_cmd_in *input_ptr, union awb_ctrl_cmd_out *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awb_altek_context                    *cxt = (struct awb_altek_context *)adpt_handle;
	struct allib_awb_set_parameter_t            input;

	UNUSED(output_ptr);
	ISP_LOGI("is_lock %d", cxt->is_lock);
	if (1 != cxt->is_lock) {
		input.type = alawb_set_param_manual_flow;
		input.para.awb_manual_flow.manual_setting = alawb_flow_lock;
		input.para.awb_manual_flow.manual_wbgain.r_gain = input_ptr->lock_param.wbgain.r;
		input.para.awb_manual_flow.manual_wbgain.g_gain = input_ptr->lock_param.wbgain.g;
		input.para.awb_manual_flow.manual_wbgain.b_gain = input_ptr->lock_param.wbgain.b;
		input.para.awb_manual_flow.manual_ct = input_ptr->lock_param.ct;
		ret = (cmr_int)cxt->lib_func.set_param(&input, cxt->lib_func.awb);
		if (ret) {
			ISP_LOGE("failed to lock");
		} else {
			cxt->is_lock = 1;
		}
	} else {
		ISP_LOGI("AWB has locked!");
	}
exit:
	return ret;
}

static cmr_int awbaltek_set_unlock(cmr_handle adpt_handle, union awb_ctrl_cmd_in *input_ptr, union awb_ctrl_cmd_out *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awb_altek_context                    *cxt = (struct awb_altek_context *)adpt_handle;
	struct allib_awb_set_parameter_t            input;

	UNUSED(input_ptr);
	UNUSED(output_ptr);
	ISP_LOGI("is_lock %d", cxt->is_lock);
	cmr_bzero(&input, sizeof(input));
	input.type = alawb_set_param_manual_flow;
	input.para.awb_manual_flow.manual_setting = alawb_flow_none;
	ret = (cmr_int)cxt->lib_func.set_param(&input, cxt->lib_func.awb);
	cxt->is_lock = 0;
exit:
	return ret;
}

static cmr_int awbaltek_set_flash_close(cmr_handle adpt_handle, union awb_ctrl_cmd_in *input_ptr, union awb_ctrl_cmd_out *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awb_altek_context                    *cxt = (struct awb_altek_context *)adpt_handle;
	struct allib_awb_set_parameter_t            input;

	UNUSED(output_ptr);
	cxt->flash_info.flash_status = input_ptr->flash_status;
	cxt->flash_info.flash_mode = AWB_CTRL_FLASH_END;

	input.type = alawb_set_param_state_under_flash;
	input.para.state_under_flash = alawb_set_flash_none;
	ret = (cmr_int)cxt->lib_func.set_param(&input, cxt->lib_func.awb);

exit:
	return ret;
}

static cmr_int awbaltek_set_ae_report(cmr_handle adpt_handle, union awb_ctrl_cmd_in *input_ptr, union awb_ctrl_cmd_out *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awb_altek_context                    *cxt = (struct awb_altek_context *)adpt_handle;
	struct allib_awb_set_parameter_t            input;
	struct isp3a_ae_report                      *report_ptr;

	UNUSED(output_ptr);
	input.type = alawb_set_param_update_ae_report;
	report_ptr = &input_ptr->ae_info.report_data;
	input.para.ae_report_update.ae_converge = report_ptr->ae_converge_st;
	input.para.ae_report_update.ae_state = report_ptr->ae_state;
	input.para.ae_report_update.bv = report_ptr->BV;
	input.para.ae_report_update.fe_state = report_ptr->fe_state;
	input.para.ae_report_update.iso = report_ptr->ISO;
	input.para.ae_report_update.non_comp_bv = report_ptr->non_comp_BV;
	input.para.ae_report_update.flash_param_preview.flash_gain.r_gain = report_ptr->flash_param_preview.wbgain_led1.r;
	input.para.ae_report_update.flash_param_preview.flash_gain.g_gain = report_ptr->flash_param_preview.wbgain_led1.g;
	input.para.ae_report_update.flash_param_preview.flash_gain.b_gain = report_ptr->flash_param_preview.wbgain_led1.b;
	input.para.ae_report_update.flash_param_preview.flash_gain_led2.r_gain = report_ptr->flash_param_preview.wbgain_led2.r;
	input.para.ae_report_update.flash_param_preview.flash_gain_led2.g_gain = report_ptr->flash_param_preview.wbgain_led2.g;
	input.para.ae_report_update.flash_param_preview.flash_gain_led2.b_gain = report_ptr->flash_param_preview.wbgain_led2.b;
	input.para.ae_report_update.flash_param_preview.flash_ratio = report_ptr->flash_param_preview.blending_ratio_led1;
	input.para.ae_report_update.flash_param_preview.flash_ratio_led2 = report_ptr->flash_param_preview.blending_ratio_led2;
	input.para.ae_report_update.flash_param_preview.LED1_CT = report_ptr->flash_param_preview.color_temp_led1;
	input.para.ae_report_update.flash_param_preview.LED2_CT = report_ptr->flash_param_preview.color_temp_led2;
	input.para.ae_report_update.flash_param_capture.flash_gain.r_gain = report_ptr->flash_param_capture.wbgain_led1.r;
	input.para.ae_report_update.flash_param_capture.flash_gain.g_gain = report_ptr->flash_param_capture.wbgain_led1.g;
	input.para.ae_report_update.flash_param_capture.flash_gain.b_gain = report_ptr->flash_param_capture.wbgain_led1.b;
	input.para.ae_report_update.flash_param_capture.flash_gain_led2.r_gain = report_ptr->flash_param_capture.wbgain_led2.r;
	input.para.ae_report_update.flash_param_capture.flash_gain_led2.g_gain = report_ptr->flash_param_capture.wbgain_led2.g;
	input.para.ae_report_update.flash_param_capture.flash_gain_led2.b_gain = report_ptr->flash_param_capture.wbgain_led2.b;
	input.para.ae_report_update.flash_param_capture.flash_ratio = report_ptr->flash_param_capture.blending_ratio_led1;
	input.para.ae_report_update.flash_param_capture.flash_ratio_led2 = report_ptr->flash_param_capture.blending_ratio_led2;
	input.para.ae_report_update.flash_param_capture.LED1_CT = report_ptr->flash_param_capture.color_temp_led1;
	input.para.ae_report_update.flash_param_capture.LED2_CT = report_ptr->flash_param_capture.color_temp_led2;
	ret = (cmr_int)cxt->lib_func.set_param(&input, cxt->lib_func.awb);
	if (ret) {
		ISP_LOGE("failed to set ae info");
	}

exit:
	return ret;
}

static cmr_int awbaltek_set_af_report(cmr_handle adpt_handle, union awb_ctrl_cmd_in *input_ptr, union awb_ctrl_cmd_out *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awb_altek_context                    *cxt = (struct awb_altek_context *)adpt_handle;
	struct allib_awb_set_parameter_t            input;

	UNUSED(output_ptr);
	input.type = alawb_set_param_update_af_report;
	ISP_LOGV("af report size %d", input_ptr->af_report.data_size);
	memcpy(&input.para.af_report_update, input_ptr->af_report.data, sizeof(struct af_report_update_t));
	ret = (cmr_int)cxt->lib_func.set_param(&input, cxt->lib_func.awb);
	if (ret) {
		ISP_LOGE("failed to set ae info");
	}
exit:
	return ret;
}

static cmr_int awbaltek_set_slave_iso_speed(cmr_handle adpt_handle, union awb_ctrl_cmd_in *input_ptr, union awb_ctrl_cmd_out *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awb_altek_context                    *cxt = (struct awb_altek_context *)adpt_handle;
	struct allib_awb_set_parameter_t            input;

	UNUSED(output_ptr);
	input.type = alawb_set_param_slave_iso_speed;
	ISP_LOGI("slave iso speed %d", input_ptr->iso_speed);
	input.para.slave_iso_speed = input_ptr->iso_speed;
	ret = (cmr_int)cxt->lib_func.set_param(&input, cxt->lib_func.awb);
	if (ret) {
		ISP_LOGE("failed to set slave iso speed");
	}
exit:
	return ret;
}

static cmr_int awbaltek_set_bypass(cmr_handle adpt_handle, union awb_ctrl_cmd_in *input_ptr, union awb_ctrl_cmd_out *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awb_altek_context                    *cxt = (struct awb_altek_context *)adpt_handle;
	struct allib_awb_set_parameter_t            input;

	UNUSED(output_ptr);
	input.type = alawb_set_param_manual_flow;
	if (1 == input_ptr->bypass) {
		input.para.awb_manual_flow.manual_setting = alawb_flow_bypass;
	} else {
		input.para.awb_manual_flow.manual_setting = alawb_flow_none;
	}
	ret = (cmr_int)cxt->lib_func.set_param(&input, cxt->lib_func.awb);
	if (!ret) {
		if (1 == input_ptr->bypass) {
			cxt->is_bypass = 1;
		} else {
			cxt->is_bypass = 0;
		}
	}

exit:
	return ret;
}

static cmr_int awbaltek_set_flash_before_p(cmr_handle adpt_handle, union awb_ctrl_cmd_in *input_ptr, union awb_ctrl_cmd_out *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awb_altek_context                    *cxt = (struct awb_altek_context *)adpt_handle;
	struct allib_awb_set_parameter_t            input;

	UNUSED(input_ptr);
	UNUSED(output_ptr);
	input.type = alawb_set_param_state_under_flash;
	input.para.state_under_flash = alawb_set_flash_prepare_under_flashon;
	ret = (cmr_int)cxt->lib_func.set_param(&input, cxt->lib_func.awb);

exit:
	return ret;
}

static cmr_int awbaltek_set_flash_before_m(cmr_handle adpt_handle, union awb_ctrl_cmd_in *input_ptr, union awb_ctrl_cmd_out *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awb_altek_context                    *cxt = (struct awb_altek_context *)adpt_handle;
	struct allib_awb_set_parameter_t            input;

	UNUSED(output_ptr);
	if (1 != cxt->is_lock) {
		input.type = alawb_set_param_manual_flow;
		input.para.awb_manual_flow.manual_setting = alawb_flow_lock;
		input.para.awb_manual_flow.manual_wbgain.r_gain = input_ptr->lock_param.wbgain.r;
		input.para.awb_manual_flow.manual_wbgain.g_gain = input_ptr->lock_param.wbgain.g;
		input.para.awb_manual_flow.manual_wbgain.b_gain = input_ptr->lock_param.wbgain.b;
		input.para.awb_manual_flow.manual_ct = input_ptr->lock_param.ct;
		ret = (cmr_int)cxt->lib_func.set_param(&input, cxt->lib_func.awb);
		if (!ret) {
			cxt->is_lock = 1;
		}
	} else {
		ISP_LOGI("AWB has locked!");
	}
exit:
	return ret;
}

static cmr_int awbaltek_set_master(cmr_handle adpt_handle, union awb_ctrl_cmd_in *input_ptr, union awb_ctrl_cmd_out *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awb_altek_context                    *cxt = (struct awb_altek_context *)adpt_handle;

	UNUSED(output_ptr);
	cxt->is_master = input_ptr->is_master;
	ISP_LOGI("camera_id %d master = %d", cxt->camera_id, cxt->is_master);
exit:
	return ret;
}

static cmr_int awbaltek_get_gain(cmr_handle adpt_handle, union awb_ctrl_cmd_in *input_ptr, union awb_ctrl_cmd_out *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awb_altek_context                    *cxt = (struct awb_altek_context *)adpt_handle;
	struct allib_awb_get_parameter_t            get_input;

	UNUSED(input_ptr);
	if (!output_ptr) {
		ISP_LOGE("error,output is NULL");
		goto exit;
	}
	output_ptr->gain.r = (float)0;
	output_ptr->gain.g = (float)0;
	output_ptr->gain.b = (float)0;
	get_input.type = alawb_get_param_wbgain;
	ret = (cmr_int)cxt->lib_func.get_param(&get_input, cxt->lib_func.awb);
	if (ret) {
		ISP_LOGE("failed to get gain");
	} else {
		output_ptr->gain.r = get_input.para.wbgain.r_gain;
		output_ptr->gain.g = get_input.para.wbgain.g_gain;
		output_ptr->gain.b = get_input.para.wbgain.b_gain;
	}
exit:
	return ret;
}

static cmr_int awbaltek_get_ct(cmr_handle adpt_handle, union awb_ctrl_cmd_in *input_ptr, union awb_ctrl_cmd_out *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awb_altek_context                    *cxt = (struct awb_altek_context *)adpt_handle;
	struct allib_awb_get_parameter_t            get_input;

	UNUSED(input_ptr);
	if (!output_ptr) {
		ISP_LOGE("error,output is NULL");
		goto exit;
	}
	output_ptr->ct = 0;
	get_input.type = alawb_get_param_color_temperature;
	ret = (cmr_int)cxt->lib_func.get_param(&get_input, cxt->lib_func.awb);
	if (ret) {
		ISP_LOGE("failed to get gain");
	} else {
		output_ptr->ct = get_input.para.color_temp;
	}
exit:
	return ret;
}

static cmr_int awbaltek_get_exif_info(cmr_handle adpt_handle, union awb_ctrl_cmd_in *input_ptr, union awb_ctrl_cmd_out *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awb_altek_context                    *cxt = (struct awb_altek_context *)adpt_handle;
	struct allib_awb_get_parameter_t            get_input;

	UNUSED(input_ptr);
	if (!output_ptr) {
		ISP_LOGI("output is NULL");
		goto exit;
	}
	output_ptr->debug_info.size = 0;
	output_ptr->debug_info.addr = NULL;

	get_input.type = alawb_get_param_debug_data_exif;
	ret = (cmr_int)cxt->lib_func.get_param(&get_input, cxt->lib_func.awb);
	if (ret) {
		ISP_LOGE("failed to get gain");
	} else {
		output_ptr->debug_info.size = get_input.para.debug_data.data_size;
		output_ptr->debug_info.addr = get_input.para.debug_data.data_addr;
	}
	ISP_LOGI("exif debug info addr %p size %ld", output_ptr->debug_info.addr, output_ptr->debug_info.size);

exit:
	return ret;
}

static cmr_int awbaltek_get_debug_info(cmr_handle adpt_handle, union awb_ctrl_cmd_in *input_ptr, union awb_ctrl_cmd_out *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awb_altek_context                    *cxt = (struct awb_altek_context *)adpt_handle;
	struct allib_awb_get_parameter_t            get_input;

	UNUSED(input_ptr);
	if (!output_ptr) {
		ISP_LOGI("output is NULL");
		goto exit;
	}
	output_ptr->debug_info.size = 0;
	output_ptr->debug_info.addr = NULL;
	get_input.type = alawb_get_param_debug_data_full;
	ret = (cmr_int)cxt->lib_func.get_param(&get_input, cxt->lib_func.awb);
	if (ret) {
		ISP_LOGE("failed to get gain");
	} else {
		output_ptr->debug_info.size = get_input.para.debug_data.data_size;
		output_ptr->debug_info.addr = get_input.para.debug_data.data_addr;
	}
	ISP_LOGI("debug info addr %p size %ld", output_ptr->debug_info.addr, output_ptr->debug_info.size);
exit:
	return ret;
}

static cmr_int awbaltek_init(cmr_handle adpt_handle, struct awb_ctrl_init_in *input_ptr, struct awb_ctrl_init_out *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awb_altek_context                    *cxt = (struct awb_altek_context *)adpt_handle;
	struct allib_awb_runtime_obj_t              *lib_func_ptr = NULL;
	struct allib_awb_lib_version_t              awb_version;
	struct allib_awb_set_parameter_t            set_param;
	struct allib_awb_set_parameter_t            set_otp_param;
	struct allib_awb_get_parameter_t            get_init_param;
	struct allib_awb_get_parameter_t            get_isp_cfg;
	struct allib_awb_output_data_t              output;
	FILE                                        *fp = NULL;
	cmr_u32                                     tuning_size = 0;
	cmr_int                                     i = 0;

	if (!cxt->ops.load_func) {
		ISP_LOGE("failed to load func, it is NULL");
		ret = ISP_ERROR;
		goto exit;
	}

	ret = cxt->ops.load_func(&cxt->lib_func);
	if (!ret) {
		ISP_LOGE("failed to load lib function");
		goto exit;
	}
	if (cxt->lib_func.obj_verification != sizeof(struct allib_awb_output_data_t)) {
		ISP_LOGE("failed to match, AWB structure isn't match");
	}
	/* get version */
	cxt->ops.get_version(&awb_version);
	ISP_LOGI("awb version, major %d, minor %d", awb_version.major_version, awb_version.minor_version);

	if (!cxt->lib_func.initial || !cxt->lib_func.deinit || !cxt->lib_func.get_param
	     || !cxt->lib_func.set_param || !cxt->lib_func.estimation) {
		ISP_LOGE("failed to get lib functions %p, %p, %p, %p, %p",
			 cxt->lib_func.initial, cxt->lib_func.deinit,
			 cxt->lib_func.get_param, cxt->lib_func.set_param,
			 cxt->lib_func.estimation);
		goto exit;
	}
	if (!input_ptr->awb_cb) {
		ISP_LOGE("failed to callback, it is NULL");
		goto exit;
	}
	cxt->callback = input_ptr->awb_cb;

	/* initialize */
	ret = (cmr_int)cxt->lib_func.initial(&cxt->lib_func);
	if (ret) {
		ISP_LOGE("failed to initialize %lx", ret);
	}

	/* test pattern */
	set_param.type = alawb_set_param_test_fix_patten;
	set_param.para.test_fix_patten = 0;
	ret = (cmr_int)cxt->lib_func.set_param(&set_param, cxt->lib_func.awb);
	if (ret) {
		ISP_LOGE("failed to set otp %lx", ret);
	}

	/* set OTP */
	set_otp_param.type = alawb_set_param_camera_calib_data;
	if (0 == input_ptr->calibration_gain.r
		|| 0 == input_ptr->calibration_gain.g
		|| 0 == input_ptr->calibration_gain.b) {
		set_otp_param.para.awb_calib_data.calib_r_gain = AWB_OTP_DEFAULT_R_GAIN;
		set_otp_param.para.awb_calib_data.calib_g_gain = AWB_OTP_DEFAULT_G_GAIN;
		set_otp_param.para.awb_calib_data.calib_b_gain = AWB_OTP_DEFAULT_B_GAIN;
	} else {
		set_otp_param.para.awb_calib_data.calib_r_gain = (cmr_u16)input_ptr->calibration_gain.r;
		set_otp_param.para.awb_calib_data.calib_g_gain = (cmr_u16)input_ptr->calibration_gain.g;
		set_otp_param.para.awb_calib_data.calib_b_gain = (cmr_u16)input_ptr->calibration_gain.b;
	}
	ISP_LOGI("otp gain %d %d %d", set_otp_param.para.awb_calib_data.calib_r_gain,
		set_otp_param.para.awb_calib_data.calib_g_gain, set_otp_param.para.awb_calib_data.calib_b_gain);
	ret = (cmr_int)cxt->lib_func.set_param(&set_otp_param, cxt->lib_func.awb);
	if (ret) {
		ISP_LOGE("failed to set otp %lx", ret);
	}

	for (i = 0; i < ISP_INDEX_MAX; i++) {
		cxt->tuning_param[i] = input_ptr->tuning_param[i];
		ISP_LOGI("set tuning file %p", input_ptr->tuning_param[i]);
	}
	if (input_ptr->tuning_param[0]) {
		set_param.type = alawb_set_param_tuning_file;
		set_param.para.tuning_file = input_ptr->tuning_param[0];
		ret = (cmr_int)cxt->lib_func.set_param(&set_param, cxt->lib_func.awb);
		if (ret) {
			ISP_LOGE("failed to set tuning file %lx", ret);
		}
	}

	if (cxt->work_mode.is_refocus && cxt->is_master) {
		set_otp_param.type = alawb_set_param_slave_calib_data;

		if (0 == input_ptr->calibration_gain_slv.r
			|| 0 == input_ptr->calibration_gain_slv.g
			|| 0 == input_ptr->calibration_gain_slv.b) {
			set_otp_param.para.awb_calib_data.calib_r_gain = AWB_OTP_DEFAULT_R_GAIN;
			set_otp_param.para.awb_calib_data.calib_g_gain = AWB_OTP_DEFAULT_G_GAIN;
			set_otp_param.para.awb_calib_data.calib_b_gain = AWB_OTP_DEFAULT_B_GAIN;
		} else {
			set_otp_param.para.awb_calib_data.calib_r_gain = (cmr_u16)input_ptr->calibration_gain_slv.r;
			set_otp_param.para.awb_calib_data.calib_g_gain = (cmr_u16)input_ptr->calibration_gain_slv.g;
			set_otp_param.para.awb_calib_data.calib_b_gain = (cmr_u16)input_ptr->calibration_gain_slv.b;
		}
		ISP_LOGI("slv otp gain %d %d %d", set_otp_param.para.awb_calib_data.calib_r_gain,
			set_otp_param.para.awb_calib_data.calib_g_gain,
			set_otp_param.para.awb_calib_data.calib_b_gain);
		ret = (cmr_int)cxt->lib_func.set_param(&set_otp_param, cxt->lib_func.awb);
		if (ret) {
			ISP_LOGE("failed to set otp %lx", ret);
		}

		for (i = 0; i < ISP_INDEX_MAX; i++) {
			cxt->tuning_param_slv[i] = input_ptr->tuning_param_slv[i];
			ISP_LOGI("set tuning file %p", input_ptr->tuning_param_slv[i]);
		}
		if (input_ptr->tuning_param_slv[0]) {
			ISP_LOGI("set slv tuning file");
			set_param.type = alawb_set_param_slave_tuning_file;
			set_param.para.tuning_file = input_ptr->tuning_param_slv[0];
			ret = (cmr_int)cxt->lib_func.set_param(&set_param, cxt->lib_func.awb);
			if (ret) {
				ISP_LOGE("failed to set tuning file %lx", ret);
			}
		}
	}

	/* get isp cfg */
	get_isp_cfg.type = alawb_get_param_init_isp_config;
	ret = (cmr_int)cxt->lib_func.get_param(&get_isp_cfg, cxt->lib_func.awb);
	if (ret) {
		ISP_LOGE("failed to get isp cfg");
	} else {
		memcpy((void *)&output_ptr->hw_cfg, get_isp_cfg.para.awb_hw_config, sizeof(struct isp3a_awb_hw_cfg));
		ISP_LOGI("cur_frame %d, cur_sof %d", get_isp_cfg.hw3a_curframeidx, get_isp_cfg.sys_cursof_frameidx);
	}
	ISP_LOGI("token_id = %d, uccr_shift = %d, uc_damp = %d, uc_offset_shift = %d",
		output_ptr->hw_cfg.token_id, output_ptr->hw_cfg.uccr_shift,
		output_ptr->hw_cfg.uc_damp, output_ptr->hw_cfg.uc_offset_shift);
	ISP_LOGV("uc_quantize = %d\n, uc_sum_shift = %d\n, uwblinear_gain = %d\n, uwrlinear_gain = %d\n,\
		uw_bgain = %d\n, uw_rgain = %d\n, uw_ggain = %d\n",
		output_ptr->hw_cfg.uc_quantize, output_ptr->hw_cfg.uc_sum_shift,
		output_ptr->hw_cfg.uwblinear_gain, output_ptr->hw_cfg.uwrlinear_gain,
		output_ptr->hw_cfg.uw_bgain, output_ptr->hw_cfg.uw_rgain,
		output_ptr->hw_cfg.uw_ggain);
	for (i = 0; i < 16; i++) {
		ISP_LOGV("uc_factor[%ld] = %d\n", i, output_ptr->hw_cfg.uc_factor[i]);
	}
	for (i = 0; i < 33; i++) {
		ISP_LOGV("bbr_factor[%ld] = %d\n", i, output_ptr->hw_cfg.bbr_factor[i]);
	}
	ISP_LOGV("region:\n blk_num_X = %d\n, blk_num_Y = %d\n, border_ratio_X = %d\n, border_ratio_Y = %d\n,\
		offset_ratio_X = %d\n, offset_ratio_Y = %d\n",
		output_ptr->hw_cfg.region.blk_num_X, output_ptr->hw_cfg.region.blk_num_Y,
		output_ptr->hw_cfg.region.border_ratio_X, output_ptr->hw_cfg.region.border_ratio_Y,
		output_ptr->hw_cfg.region.offset_ratio_X, output_ptr->hw_cfg.region.offset_ratio_Y);
	ISP_LOGV("t_his:\n benable = %d\n ccrend = %d\n ccrpurple = %d\n ccrstart = %d\n \
		cgrass_end = %d\n cgrass_offset = %d\n cgrass_start = %d\n coffsetdown = %d\n \
		coffset_bbr_w_end = %d\n coffset_bbr_w_start = %d\n cooffsetup = %d\n \
		dhisinterp = %d\n ucdampgrass = %d\n ucoffsetpurple = %d\n ucyfac_w = %d\n",
		output_ptr->hw_cfg.t_his.benable,
		output_ptr->hw_cfg.t_his.ccrend,
		output_ptr->hw_cfg.t_his.ccrpurple,
		output_ptr->hw_cfg.t_his.ccrstart,
		output_ptr->hw_cfg.t_his.cgrass_end,
		output_ptr->hw_cfg.t_his.cgrass_offset,
		output_ptr->hw_cfg.t_his.cgrass_start,
		output_ptr->hw_cfg.t_his.coffsetdown,
		output_ptr->hw_cfg.t_his.coffset_bbr_w_end,
		output_ptr->hw_cfg.t_his.coffset_bbr_w_start,
		output_ptr->hw_cfg.t_his.cooffsetup,
		output_ptr->hw_cfg.t_his.dhisinterp,
		output_ptr->hw_cfg.t_his.ucdampgrass,
		output_ptr->hw_cfg.t_his.ucoffsetpurple,
		output_ptr->hw_cfg.t_his.ucyfac_w);

	/* set default setting */
	set_param.type = alawb_set_param_awb_mode_setting;
	set_param.para.awb_mode_setting.wbmode_type = AL3A_WB_MODE_AUTO;
	set_param.para.awb_mode_setting.manual_ct = 0;
	ret = (cmr_int)cxt->lib_func.set_param(&set_param, cxt->lib_func.awb);
	if (ret) {
		ISP_LOGE("failed to set mode %lx %ld", ret, (cmr_int)cxt->lib_func.awb);
	}

	/* set response setting */
	set_param.type = alawb_set_param_response_setting;
	set_param.para.awb_response_setting.response_type = alawb_response_stable;
	set_param.para.awb_response_setting.response_level = alawb_response_level_normal;
	ret = cxt->lib_func.set_param(&set_param, cxt->lib_func.awb);
	if (ret) {
		ISP_LOGE("failed to set response setting %lx", ret);
		goto exit;
	}
	/* get init param */
	get_init_param.type = alawb_get_param_init_setting;
	ret = cxt->lib_func.get_param(&get_init_param, cxt->lib_func.awb);
	if (ret) {
		ISP_LOGE("failed to get init param %lx", ret);
	}
	output_ptr->ct = (cmr_u32)get_init_param.para.awb_init_data.color_temperature;
	output_ptr->gain.r = get_init_param.para.awb_init_data.initial_wbgain.r_gain;
	output_ptr->gain.g = get_init_param.para.awb_init_data.initial_wbgain.g_gain;
	output_ptr->gain.b = get_init_param.para.awb_init_data.initial_wbgain.b_gain;
	output_ptr->gain_balanced.r = get_init_param.para.awb_init_data.initial_wbgain_balanced.r_gain;
	output_ptr->gain_balanced.g = get_init_param.para.awb_init_data.initial_wbgain_balanced.g_gain;
	output_ptr->gain_balanced.b = get_init_param.para.awb_init_data.initial_wbgain_balanced.b_gain;
	ISP_LOGI("awb lib processing frame %d %d", get_init_param.hw3a_curframeidx, get_init_param.sys_cursof_frameidx);
	ISP_LOGI("gain %d, %d, %d, %d", output_ptr->gain.r, output_ptr->gain.g, output_ptr->gain.b, output_ptr->ct);

	set_param.type = alawb_set_param_awb_debug_mask;
	set_param.para.awb_debug_mask = alawb_dbg_none;
	ret = cxt->lib_func.set_param(&set_param, cxt->lib_func.awb);
	if (ret) {
		ISP_LOGE("failed to set debug %lx", ret);
		goto exit;
	}

	/* update Digital zoom factor if changed */
	set_param.type = alawb_set_param_dzoom_factor;
	set_param.para.dzoom_factor = 1.0;               //. default 1.0x
	ret = cxt->lib_func.set_param(&set_param, cxt->lib_func.awb);
	if (ret) {
		ISP_LOGE("failed to set dzoom %lx", ret);
		goto exit;
	}
exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

static cmr_int awbaltek_deinit(cmr_handle adpt_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awb_altek_context                    *cxt = (struct awb_altek_context *)adpt_handle;

	if (!cxt->lib_func.deinit) {
		ISP_LOGE("failed to deinit, func is NULL");
		ret = ISP_ERROR;
		goto exit;
	}
	ret = (cmr_int)cxt->lib_func.deinit(&cxt->lib_func);
exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

static cmr_int awbaltek_set_workmode(cmr_handle adpt_handle, enum awb_ctrl_cmd cmd, union awb_ctrl_cmd_in *input_ptr, union awb_ctrl_cmd_out *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awb_altek_context                    *cxt = (struct awb_altek_context *)adpt_handle;

	UNUSED(cmd);
	UNUSED(output_ptr);
	cxt->work_mode.work_mode = input_ptr->work_param.work_mode;
	cxt->work_mode.capture_mode = input_ptr->work_param.capture_mode;
	//cxt->work_mode.is_refocus = input_ptr->work_param.is_refocus;
	cxt->work_mode.sensor_size.w = input_ptr->work_param.sensor_size.w;
	cxt->work_mode.sensor_size.h = input_ptr->work_param.sensor_size.h;

	return ret;
}

static cmr_int awbaltek_set_face(cmr_handle adpt_handle, enum awb_ctrl_cmd cmd, union awb_ctrl_cmd_in *input_ptr, union awb_ctrl_cmd_out *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awb_altek_context                    *cxt = (struct awb_altek_context *)adpt_handle;
	struct allib_awb_set_parameter_t            input;
	struct allib_awb_output_data_t              output;
	cmr_u32                                     i = 0;
	cmr_u32                                     index = 0;
	cmr_u32                                     temp = 0;
	cmr_u32                                     face_area_max = 0;
	struct isp_face_area                        face_data = input_ptr->face_info;

	UNUSED(cmd);
	UNUSED(output_ptr);
	if (0 == face_data.face_num) {
		ISP_LOGI("set face num is 0");
		goto exit;
	}
	for (i = 0; i < face_data.face_num; i++) {
		temp = (face_data.face_info[i].ex-face_data.face_info[i].sx)*(face_data.face_info[i].ey-face_data.face_info[i].ex);
		if (face_area_max < temp) {
			face_area_max = temp;
			index = i;
		}
	}
	input.type = alawb_set_param_face_info_setting;
	input.para.face_info.flag_face = 1;
	input.para.face_info.x = face_data.face_info[index].sx;
	input.para.face_info.y = face_data.face_info[index].sy;
	input.para.face_info.w = face_data.face_info[index].ex - face_data.face_info[index].sx;
	input.para.face_info.h = face_data.face_info[index].ey - face_data.face_info[index].sy;
	input.para.face_info.frame_w = face_data.frame_width;
	input.para.face_info.frame_h = face_data.frame_height;
	ret = cxt->lib_func.set_param(&input, cxt->lib_func.awb);
	if (ret) {
		ISP_LOGE("failed to set face info %ld", ret);
	}
exit:
	ISP_LOGV("done %ld", ret);
	return ret;
}

static cmr_int awbaltek_open_pre_flash(cmr_handle adpt_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awb_altek_context                    *cxt = (struct awb_altek_context *)adpt_handle;
	struct allib_awb_set_parameter_t            input;

	/* set flash state */
	input.type = alawb_set_param_state_under_flash;
	input.para.state_under_flash = alawb_set_flash_under_flashon;
	ret = (cmr_int)cxt->lib_func.set_param(&input, cxt->lib_func.awb);
	if (ret) {
		ISP_LOGE("failed to set flash state %lx", ret);
	}
	/* set response level */
	input.type = alawb_set_param_response_setting;
	input.para.awb_response_setting.response_type = alawb_response_quick_act;
	input.para.awb_response_setting.response_level = alawb_response_level_normal;
	ret = (cmr_int)cxt->lib_func.set_param(&input, cxt->lib_func.awb);
	if (ret) {
		ISP_LOGE("failed to set response %lx", ret);
	}
	cxt->flash_info.flash_status = AWB_CTRL_FLASH_PRE_LIGHTING;
	cxt->flash_info.flash_mode = AWB_CTRL_FLASH_PRE;
	cxt->flash_info.gain_flash_off.r = cxt->cur_process_out.report_3a_update.awb_update.wbgain_flash_off.r_gain;
	cxt->flash_info.gain_flash_off.g = cxt->cur_process_out.report_3a_update.awb_update.wbgain_flash_off.g_gain;
	cxt->flash_info.gain_flash_off.b = cxt->cur_process_out.report_3a_update.awb_update.wbgain_flash_off.b_gain;
	cxt->flash_info.ct_flash_off = cxt->cur_process_out.report_3a_update.awb_update.color_temp_flash_off;
	ISP_LOGI("gain_flash_off %d,%d,%d, ct_flash_off %d", cxt->flash_info.gain_flash_off.r,
			cxt->flash_info.gain_flash_off.g, cxt->flash_info.gain_flash_off.b,
			cxt->flash_info.ct_flash_off);
	return ret;
}

static cmr_int awbaltek_set_tuning_param(cmr_handle adpt_handle, void *tuning_param)
{
	struct allib_awb_set_parameter_t   set_param;
	cmr_int ret = ISP_ERROR;
	struct awb_altek_context                    *cxt = (struct awb_altek_context *)adpt_handle;

	if(!cxt) {
		ISP_LOGE("param is NULL error!");
		return ret;
	}
	ISP_LOGI("tuning_param %p", tuning_param);
	if (tuning_param) {
		ISP_LOGI("set tuning file");
		set_param.type = alawb_set_param_tuning_file;
		set_param.para.tuning_file = tuning_param;
		ret = (cmr_int)cxt->lib_func.set_param(&set_param, cxt->lib_func.awb);
		if (ret) {
			ISP_LOGE("failed to set tuning file %lx", ret);
		}
	}
	return ret;
}

static cmr_int awbaltek_set_tuning_param_slv(cmr_handle adpt_handle, void *tuning_param_slv)
{
	struct allib_awb_set_parameter_t   set_param;
	cmr_int ret = ISP_ERROR;
	struct awb_altek_context                    *cxt = (struct awb_altek_context *)adpt_handle;

	if(!cxt) {
		ISP_LOGE("param is NULL error!");
		return ret;
	}
	ISP_LOGI("tuning_param %p", tuning_param_slv);
	if (tuning_param_slv) {
		ISP_LOGI("set slv tuning file");
		set_param.type = alawb_set_param_slave_tuning_file;
		set_param.para.tuning_file = tuning_param_slv;
		ret = (cmr_int)cxt->lib_func.set_param(&set_param, cxt->lib_func.awb);
		if (ret) {
			ISP_LOGE("failed to set tuning file %lx", ret);
		}
	}
	return ret;
}

static cmr_int awbaltek_set_tuning_mode(cmr_handle adpt_handle, union awb_ctrl_cmd_in *input_ptr)
{
	cmr_int ret = ISP_ERROR;
	cmr_u32 idx_num;
	struct awb_altek_context                   *cxt = (struct awb_altek_context *)adpt_handle;

	if (!cxt) {
		ISP_LOGE("param is NULL error!");
		return ret;
	}
	idx_num = input_ptr->idx_num;
	ISP_LOGI("idx_num=%d awb_addr %p", idx_num, cxt->tuning_param[idx_num]);
	ret = awbaltek_set_tuning_param(adpt_handle, (void *)cxt->tuning_param[idx_num]);

	if (cxt->work_mode.is_refocus && cxt->is_master)
		ret = awbaltek_set_tuning_param_slv(adpt_handle, (void *)cxt->tuning_param_slv[idx_num]);
	return ret;
}

static cmr_int awbaltek_set_scene_mode(cmr_handle adpt_handle, union awb_ctrl_cmd_in *input_ptr)
{
	cmr_int ret = ISP_ERROR;
	struct awb_altek_context                   *cxt = (struct awb_altek_context *)adpt_handle;

	if (!cxt) {
		ISP_LOGE("param is NULL error!");
		return ret;
	}

	ISP_LOGI("awb_addr %p mode %d ", input_ptr->scene_info.puc_addr, input_ptr->scene_info.uw_mode);
	cxt->scene_mode = input_ptr->scene_info.uw_mode;
	if (cxt->scene_mode == 0 && cxt->wb_mode != 0)
		return ISP_SUCCESS;
	ret = al3awrapperawb_setscenesetting(&input_ptr->scene_info,  &cxt->lib_func);
	ISP_LOGI("ret %ld ", ret);
	return ret;

}

static cmr_int awbaltek_ioctrl(cmr_handle adpt_handle, enum awb_ctrl_cmd cmd, union awb_ctrl_cmd_in *input_ptr, union awb_ctrl_cmd_out *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awb_altek_context                    *cxt = (struct awb_altek_context *)adpt_handle;
	struct allib_awb_set_parameter_t            input;
	struct allib_awb_get_parameter_t            get_input;

	switch (cmd) {
	case AWB_CTRL_CMD_SET_WB_MODE:
		ret = awbaltek_set_wb_mode(adpt_handle, input_ptr, output_ptr);
		break;
	case AWB_CTRL_CMD_SET_FLASH_MODE:
		break;
	case AWB_CTRL_CMD_SET_WORK_MODE:
		ret = awbaltek_set_workmode(adpt_handle, cmd, input_ptr, output_ptr);
		break;
	case AWB_CTRL_CMD_SET_FACE_INFO:
		ret = awbaltek_set_face(adpt_handle, cmd, input_ptr, output_ptr);
		break;
	case AWB_CTRL_CMD_SET_DZOOM_FACTOR:
		ret = awbaltek_set_dzoom(adpt_handle, input_ptr, output_ptr);
		break;
	case AWB_CTRL_CMD_SET_SOF_FRAME_IDX:
		ret = awbaltek_set_sof_frame_id(adpt_handle, input_ptr, output_ptr);
		break;
	case AWB_CTRL_CMD_GET_GAIN:
		ret = awbaltek_get_gain(adpt_handle, input_ptr, output_ptr);
		break;
	case AWB_CTRL_CMD_FLASH_OPEN_P:
		ret = awbaltek_open_pre_flash(adpt_handle);
		break;
	case AWB_CTRL_CMD_FLASH_OPEN_M:
		break;
	case AWB_CTRL_CMD_FLASHING:
		break;
	case AWB_CTRL_CMD_FLASH_CLOSE:
		ret = awbaltek_set_flash_close(adpt_handle, input_ptr, output_ptr);
		break;
	case AWB_CTRL_CMD_LOCK:
		ret = awbaltek_set_lock(adpt_handle, input_ptr, output_ptr);
		break;
	case AWB_CTRL_CMD_SET_MASTER:
		ret = awbaltek_set_master(adpt_handle, input_ptr, output_ptr);
		break;
	case AWB_CTRL_CMD_UNLOCK:
		ret = awbaltek_set_unlock(adpt_handle, input_ptr, output_ptr);
		break;
	case AWB_CTRL_CMD_GET_CT:
		ret = awbaltek_get_ct(adpt_handle, input_ptr, output_ptr);
		break;
	case AWB_CTRL_CMD_FLASH_BEFORE_P://TBD
		ret = awbaltek_set_flash_before_p(adpt_handle, input_ptr, output_ptr);
		break;
	case AWB_CTRL_CMD_FLASH_BEFORE_M:
		ret = awbaltek_set_flash_before_m(adpt_handle, input_ptr, output_ptr);
		break;
	case AWB_CTRL_CMD_SET_FLASH_STATUS:
		break;
	case AWB_CTRL_CMD_SET_AE_REPORT://TBD
		ret = awbaltek_set_ae_report(adpt_handle, input_ptr, output_ptr);
		break;
	case AWB_CTRL_CMD_SET_AF_REPORT:
		ret = awbaltek_set_af_report(adpt_handle, input_ptr, output_ptr);
		break;
	case AWB_CTRL_CMD_SET_BYPASS:
		ret = awbaltek_set_bypass(adpt_handle, input_ptr, output_ptr);
		break;
	case AWB_CTRL_CMD_SET_SLAVE_ISO_SPEED:
		ret = awbaltek_set_slave_iso_speed(adpt_handle, input_ptr, output_ptr);
		break;
	case AWB_CTRL_CMD_SET_SCENE_MODE:
		ret = awbaltek_set_scene_mode(adpt_handle, input_ptr);
		break;
	case AWB_CTRL_CMD_GET_STAT_SIZE:
	case AWB_CTRL_CMD_GET_WIN_SIZE:
	case AWB_CTRL_CMD_SET_STAT_IMG_SIZE:
	case AWB_CTRL_CMD_SET_STAT_IMG_FORMAT:
	case AWB_CTRL_CMD_GET_PARAM_WIN_START:
	case AWB_CTRL_CMD_GET_PARAM_WIN_SIZE:
		break;
	case AWB_CTRL_CMD_GET_EXIF_DEBUG_INFO:
		ret = awbaltek_get_exif_info(adpt_handle, input_ptr, output_ptr);
		break;
	case AWB_CTRL_CMD_GET_DEBUG_INFO:
		ret = awbaltek_get_debug_info(adpt_handle, input_ptr, output_ptr);
		break;
	case AWB_CTRL_CMD_SET_TUNING_MODE:
		ret = awbaltek_set_tuning_mode(adpt_handle, input_ptr);
		break;
	default:
		break;
	}

exit:
	ISP_LOGV("cmd %d, done %ld", cmd, ret);
	return ret;
}

static cmr_int awbaltek_update_ae_report(cmr_handle adpt_handle, struct isp3a_ae_info *ae_info)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awb_altek_context                    *cxt = (struct awb_altek_context *)adpt_handle;
	struct allib_awb_set_parameter_t            set_param;

	set_param.type = alawb_set_param_update_ae_report;
	set_param.para.ae_report_update.ae_state = ae_info->report_data.ae_state;
	set_param.para.ae_report_update.ae_converge = ae_info->report_data.ae_converge_st;
	set_param.para.ae_report_update.bv = ae_info->report_data.BV;
	set_param.para.ae_report_update.non_comp_bv = ae_info->report_data.non_comp_BV;// non compensated bv value
	set_param.para.ae_report_update.iso = ae_info->report_data.ISO;// ISO Speed
	set_param.para.ae_report_update.fe_state = ae_info->report_data.fe_state;
	set_param.para.ae_report_update.flash_param_preview.flash_gain.r_gain = ae_info->report_data.flash_param_preview.wbgain_led1.r;
	set_param.para.ae_report_update.flash_param_preview.flash_gain.g_gain = ae_info->report_data.flash_param_preview.wbgain_led1.g;
	set_param.para.ae_report_update.flash_param_preview.flash_gain.b_gain = ae_info->report_data.flash_param_preview.wbgain_led1.b;
	set_param.para.ae_report_update.flash_param_preview.flash_gain_led2.r_gain = ae_info->report_data.flash_param_preview.wbgain_led2.r;
	set_param.para.ae_report_update.flash_param_preview.flash_gain_led2.g_gain = ae_info->report_data.flash_param_preview.wbgain_led2.g;
	set_param.para.ae_report_update.flash_param_preview.flash_gain_led2.b_gain = ae_info->report_data.flash_param_preview.wbgain_led2.b;
	set_param.para.ae_report_update.flash_param_preview.flash_ratio = (cmr_u32)ae_info->report_data.flash_param_preview.blending_ratio_led1;
	set_param.para.ae_report_update.flash_param_preview.flash_ratio_led2 = (cmr_u32)ae_info->report_data.flash_param_preview.blending_ratio_led2;
	set_param.para.ae_report_update.flash_param_preview.LED1_CT = (cmr_u32)ae_info->report_data.flash_param_preview.color_temp_led1;
	set_param.para.ae_report_update.flash_param_preview.LED2_CT = (cmr_u32)ae_info->report_data.flash_param_preview.color_temp_led2;
	set_param.para.ae_report_update.flash_param_capture.flash_gain.r_gain = ae_info->report_data.flash_param_capture.wbgain_led1.r;
	set_param.para.ae_report_update.flash_param_capture.flash_gain.g_gain = ae_info->report_data.flash_param_capture.wbgain_led1.g;
	set_param.para.ae_report_update.flash_param_capture.flash_gain.b_gain = ae_info->report_data.flash_param_capture.wbgain_led1.b;
	set_param.para.ae_report_update.flash_param_capture.flash_gain_led2.r_gain = ae_info->report_data.flash_param_capture.wbgain_led1.r;
	set_param.para.ae_report_update.flash_param_capture.flash_gain_led2.g_gain = ae_info->report_data.flash_param_capture.wbgain_led1.g;
	set_param.para.ae_report_update.flash_param_capture.flash_gain_led2.b_gain = ae_info->report_data.flash_param_capture.wbgain_led1.b;
	set_param.para.ae_report_update.flash_param_capture.flash_ratio = (cmr_u32)ae_info->report_data.flash_param_capture.blending_ratio_led1;
	set_param.para.ae_report_update.flash_param_capture.flash_ratio_led2 = (cmr_u32)ae_info->report_data.flash_param_capture.blending_ratio_led2;
	set_param.para.ae_report_update.flash_param_capture.LED1_CT = (cmr_u32)ae_info->report_data.flash_param_capture.color_temp_led1;
	set_param.para.ae_report_update.flash_param_capture.LED2_CT = (cmr_u32)ae_info->report_data.flash_param_capture.color_temp_led2;

	ret = cxt->lib_func.set_param(&set_param, cxt->lib_func.awb);//TBD
	if (ret) {
		ISP_LOGE("failed to update ae information %lx", ret);
	}

	return ret;
}

static cmr_int awbaltek_process(cmr_handle adpt_handle, struct awb_ctrl_process_in *input_ptr, struct awb_ctrl_process_out *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awb_altek_context                    *cxt = (struct awb_altek_context *)adpt_handle;
	struct awb_report_update_t                  *report_ptr;

	if (cxt->is_bypass) {
		output_ptr->is_update = 0;
		ISP_LOGI("awb is bypass");
		goto exit;
	}
	if (cxt->lib_func.estimation && cxt->lib_func.set_param) {
		/* dispatch stats */
		ret = al3awrapper_dispatchhw3a_awbstats(input_ptr->statistics_data->addr, (void *)(&cxt->awb_stats));
		if (ret) {
			ISP_LOGE("failed to dispatch stats %lx", ret);
		}

		/* set ae param */
		ret = awbaltek_update_ae_report(cxt, &input_ptr->ae_info);
		if (ret) {
			ISP_LOGE("failed to update ae report %lx", ret);
		}
		/* estimation */
		ret = (cmr_int)cxt->lib_func.estimation((void *)(&cxt->awb_stats), cxt->lib_func.awb, &cxt->cur_process_out);
		if (ret) {
			ISP_LOGE("failed to estimation %ld", ret);
		} else {
			report_ptr = &cxt->cur_process_out.report_3a_update.awb_update;
			output_ptr->ct = cxt->cur_process_out.color_temp;
			output_ptr->ct_flash_off = cxt->cur_process_out.color_temp_flash_off;
			output_ptr->ct_capture = cxt->cur_process_out.color_temp_capture;
			output_ptr->gain.r = cxt->cur_process_out.wbgain.r_gain;
			output_ptr->gain.g = cxt->cur_process_out.wbgain.g_gain;
			output_ptr->gain.b = cxt->cur_process_out.wbgain.b_gain;
			output_ptr->gain_balanced.r = cxt->cur_process_out.wbgain_balanced.r_gain;
			output_ptr->gain_balanced.g = cxt->cur_process_out.wbgain_balanced.g_gain;
			output_ptr->gain_balanced.b = cxt->cur_process_out.wbgain_balanced.b_gain;
			output_ptr->gain_flash_off.r = cxt->cur_process_out.wbgain_flash_off.r_gain;
			output_ptr->gain_flash_off.g = cxt->cur_process_out.wbgain_flash_off.g_gain;
			output_ptr->gain_flash_off.b = cxt->cur_process_out.wbgain_flash_off.b_gain;
			output_ptr->gain_capture.r = cxt->cur_process_out.wbgain_capture.r_gain;
			output_ptr->gain_capture.g = cxt->cur_process_out.wbgain_capture.g_gain;
			output_ptr->gain_capture.b = cxt->cur_process_out.wbgain_capture.b_gain;
			output_ptr->awb_decision = cxt->cur_process_out.awb_decision;
			output_ptr->awb_mode = cxt->cur_process_out.awb_mode;
			output_ptr->hw3a_frame_id = report_ptr->hw3a_frame_id;
			output_ptr->is_update = cxt->cur_process_out.awb_update;
			output_ptr->light_source = cxt->cur_process_out.light_source;
			output_ptr->awb_states = AWB_CTRL_STATUS_NORMAL;
			if (AL3A_WB_STATE_PREPARE_UNDER_FLASHON_DONE == report_ptr->awb_states
				|| AL3A_WB_STATE_UNDER_FLASHON_AWB_DONE == report_ptr->awb_states) {
				output_ptr->awb_states = AWB_CTRL_STATUS_CONVERGE;
				ISP_LOGI("flash_off ct:%d,rgb:%d %d %d,flash_capture ct:%d,rgb:%d %d %d",
						output_ptr->ct_flash_off, output_ptr->gain_flash_off.r,
						output_ptr->gain_flash_off.g, output_ptr->gain_flash_off.b,
						output_ptr->ct_capture, output_ptr->gain_capture.r,
						output_ptr->gain_capture.g, output_ptr->gain_capture.b);
			}

			ISP_LOGI("awb mode %d, awb_states:%d, gain %d %d %d, gain_blanced %d %d %d,camera_id:%d",
				     output_ptr->awb_mode, report_ptr->awb_states,
				     output_ptr->gain.r, output_ptr->gain.g, output_ptr->gain.b,
				     output_ptr->gain_balanced.r, output_ptr->gain_balanced.g, output_ptr->gain_balanced.b, cxt->camera_id);
			ISP_LOGV("awb update %d, frame id %d", output_ptr->is_update, output_ptr->hw3a_frame_id);
			ISP_LOGV("awb ct %d, light source %d", output_ptr->ct, output_ptr->light_source);
			if (!cxt->work_mode.is_refocus) {
				isp_mlog(AWB_FILE, "wbgain:(%d, %d, %d), CT:%d, light_source:%d, awb_decision:%d, wbgain_balanced:(%d, %d, %d)",
					 cxt->cur_process_out.wbgain.r_gain,
					 cxt->cur_process_out.wbgain.g_gain,
					 cxt->cur_process_out.wbgain.b_gain,
					 cxt->cur_process_out.color_temp,
					 cxt->cur_process_out.light_source,
					 cxt->cur_process_out.awb_decision,
					 cxt->cur_process_out.wbgain_balanced.r_gain,
					 cxt->cur_process_out.wbgain_balanced.g_gain,
					 cxt->cur_process_out.wbgain_balanced.b_gain);
			}
		}
	} else {
		ISP_LOGE("don't have process interface");
	}

exit:
	ISP_LOGV("done %ld", ret);
	return ret;
}

static cmr_int awbaltek_convert_wb_mode(enum awb_ctrl_wb_mode input_mode)
{
	cmr_int                                     wb_mode = AL3A_WB_MODE_AUTO;

	switch (input_mode) {
	case AWB_CTRL_WB_MODE_AUTO:
		wb_mode = AL3A_WB_MODE_AUTO;
		break;
	case AWB_CTRL_MWB_MODE_SUNNY:
		wb_mode = AL3A_WB_MODE_DAYLIGHT;
		break;
	case AWB_CTRL_MWB_MODE_CLOUDY:
		wb_mode = AL3A_WB_MODE_CLOUDY_DAYLIGHT;
		break;
	case AWB_CTRL_MWB_MODE_FLUORESCENT:
		wb_mode = AL3A_WB_MODE_FLUORESCENT;
		break;
	case AWB_CTRL_MWB_MODE_INCANDESCENT:
		wb_mode = AL3A_WB_MODE_INCANDESCENT;
		break;
	case AWB_CTRL_MWB_MODE_DAYLIGHT:
		wb_mode = AL3A_WB_MODE_DAYLIGHT;
		break;
	case AWB_CTRL_MWB_MODE_WHITE_FLUORESCENT:
		//wb_mode = alawb_mode_white_fluorescent;
		break;
	case AWB_CTRL_MWB_MODE_TUNGSTEN:
		//wb_mode = alawb_mode_tungsten;
		break;
	case AWB_CTRL_MWB_MODE_SHADE:
		wb_mode = AL3A_WB_MODE_SHADE;
		break;
	case AWB_CTRL_MWB_MODE_TWILIGHT:
		wb_mode = AL3A_WB_MODE_TWILIGHT;
		break;
	case AWB_CTRL_MWB_MODE_FACE:
		//wb_mode = alawb_mode_face;
		break;
	case AWB_CTRL_MWB_MODE_MANUAL_GAIN:
		//wb_mode = alawb_mode_manual_gain;
		break;
	case AWB_CTRL_MWB_MODE_MANUAL_CT:
		wb_mode = AL3A_WB_MODE_MANUAL_CT;
		break;
	default:
		break;
	}
	ISP_LOGI("wb_mode is %ld", wb_mode);
	return wb_mode;
}
/*************************************EXTERNAL FUNCTION ***************************************/
static cmr_int awb_altek_adpt_init(void *input_ptr, void *output_ptr, cmr_handle *adpt_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awb_ctrl_init_in                     *input_param_ptr = (struct awb_ctrl_init_in *)input_ptr;
	struct awb_ctrl_init_out                    *output_param_ptr = (struct awb_ctrl_init_out *)output_ptr;
	struct awb_altek_context                    *cxt = NULL;

	if (!input_ptr || !output_ptr || !adpt_handle) {
		ISP_LOGE("input param is NULL,input is %p, output is %p", input_ptr, output_ptr);
		ret = ISP_PARAM_NULL;
		goto exit;
	}

	*adpt_handle = NULL;
	cxt = (struct awb_altek_context *)malloc(sizeof(struct awb_altek_context));
	if (!cxt) {
		ISP_LOGE("failed to maolloc context");
		ret = ISP_ALLOC_ERROR;
		goto exit;
	}
	cmr_bzero(cxt, sizeof(*cxt));
	cxt->caller_handle = input_param_ptr->caller_handle;
	cxt->camera_id = input_param_ptr->camera_id;
	cxt->work_mode.is_refocus = input_param_ptr->is_refocus;
	cxt->is_master = input_param_ptr->is_master;

	ret = awbaltek_load_library((cmr_handle)cxt);
	if (ret) {
		ISP_LOGE("failed to load altek library");
		ret = -ISP_ERROR;
		goto exit;
	}
	ret = awbaltek_init((cmr_handle)cxt, input_param_ptr, output_param_ptr);
exit:
	if (ret) {
		if (cxt) {
			ret = awbaltek_unload_library((cmr_handle)cxt);
			free((void *)cxt);
		}
	} else {
		cxt->is_inited = 1;
		*adpt_handle = (cmr_handle)cxt;
	}
	ISP_LOGI("done %ld", ret);
	return ret;
}

static cmr_int awb_altek_adpt_deinit(cmr_handle adpt_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awb_altek_context                    *cxt = (struct awb_altek_context *)adpt_handle;

	if (!adpt_handle) {
		ISP_LOGE("input param is NULL");
		goto exit;
	}
	if (0 == cxt->is_inited) {
		ISP_LOGV("awb adapter has already been deinited");
		goto exit;
	}
	awbaltek_deinit(adpt_handle);
	awbaltek_unload_library(adpt_handle);
	cmr_bzero(cxt, sizeof(*cxt));
	free((void *)cxt);
exit:
	ISP_LOGV("done %ld", ret);
	return ret;
}

static cmr_int awb_altek_adpt_ioctrl(cmr_handle adpt_handle, cmr_int cmd, void *input_ptr, void *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	union awb_ctrl_cmd_in                       *input_param_ptr = (union awb_ctrl_cmd_in *)input_ptr;
	union awb_ctrl_cmd_out                      *output_param_ptr = (union awb_ctrl_cmd_out *)output_ptr;

	if (!adpt_handle) {
		ISP_LOGI("input is NULL");
		goto exit;
	}
	ret = awbaltek_ioctrl(adpt_handle, cmd, input_ptr, output_ptr);
exit:
	ISP_LOGV("done %ld", ret);
	return ret;
}

static cmr_int awb_altek_adpt_process(cmr_handle adpt_handle, void *input_ptr, void *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct awb_ctrl_process_in                  *input_param_ptr = (struct awb_ctrl_process_in *)input_ptr;
	struct awb_ctrl_process_out                 *output_param_ptr = (struct awb_ctrl_process_out *)output_ptr;

	if (!adpt_handle || !input_ptr || !output_ptr) {
		ISP_LOGI("input is NULL,0x%lx, 0x%lx", (cmr_uint)adpt_handle, (cmr_uint)input_ptr);
		goto exit;
	}
	ret = awbaltek_process(adpt_handle, input_ptr, output_ptr);
exit:
	ISP_LOGV("done %ld", ret);
	return ret;
}

/*************************************ADAPTER CONFIGURATION ***************************************/
static struct isp_lib_config awb_altek_lib_info = {
	.product_id = 0,
	.version_id = 0,
};

static struct adpt_ops_type awb_altek_adpt_ops = {
	.adpt_init = awb_altek_adpt_init,
	.adpt_deinit = awb_altek_adpt_deinit,
	.adpt_process = awb_altek_adpt_process,
	.adpt_ioctrl = awb_altek_adpt_ioctrl,
};

struct adpt_register_type awb_altek_adpt_info = {
	.lib_type = ADPT_LIB_AWB,
	.lib_info = &awb_altek_lib_info,
	.ops = &awb_altek_adpt_ops,
};
