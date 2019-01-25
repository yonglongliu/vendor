/*
 * Copyright (C) 2015 The Android Open Source Project
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
#define LOG_TAG "alk_adpt_ae"

#include <dlfcn.h>
#include <cutils/properties.h>
#include "ae_ctrl_types.h"
#include "isp_adpt.h"
#include "allib_ae.h"
#include "allib_ae_errcode.h"
#include "alwrapper_ae.h"
#include "alwrapper_ae_errcode.h"
#include "sensor_exposure_queue.h"
#include "alwrapper_yhist.h"
#include "isp_mlog.h"
#include "isp_bridge.h"


#define AELIB_PATH "libalAELib.so"

#define SENSOR_GAIN_BASE       128
#define LIB_GAIN_BASE          100
#define F_NUM_BASE             100
#define MAX_EXP_LINE_CNT       65535
#define BRACKET_NUM            5
#define SENSOR_EXP_US_BASE     1000000000 /*x1000us*/
#define SENSOR_EXP_BASE        1000 /*x1000us*/
#define TUNING_EXPOSURE_NUM    300

enum aealtek_work_mode {
	AEALTEK_PREVIEW,
	AEALTEK_BRACKET,
	AEALTEK_SNAPSHOT,
	AEALTEK_MAX
};

enum ae_tuning_mode {
	TUNING_MODE_EXPOSURE,
	TUNING_MODE_GAIN,
	TUNING_MODE_OBCLAMP,
	TUNING_MODE_USER_DEF
};

struct aealtek_lib_ops {
	BOOL (*load_func)(struct alaeruntimeobj_t *aec_run_obj, UINT32 identityID);
	void (*get_lib_ver)(struct alaelib_version_t *AE_LibVersion);
};

struct aealtek_exposure_param {
	cmr_u32 exp_line;
	cmr_u32 exp_time;
	cmr_u32 dummy;
	cmr_u32 gain;
	cmr_u32 size_index;
	cmr_u32 iso;
	cmr_u32 isp_d_gain;
};

struct aealtek_sensor_exp_data {
	struct aealtek_exposure_param lib_exp;
	struct aealtek_exposure_param actual_exp;
	struct aealtek_exposure_param write_exp;
};

struct aealtek_ui_param {
	struct ae_ctrl_param_work work_info;
	struct ae_ctrl_param_work work_info_slv;
	enum ae_ctrl_scene_mode scene;
	enum ae_ctrl_flicker_mode flicker;
	enum ae_ctrl_iso_mode iso;
	cmr_s32 ui_ev_level;
	enum ae_ctrl_measure_lum_mode weight;
	struct ae_ctrl_param_range_fps fps;
	enum ae_ctrl_hdr_ev_level  hdr_level;
};

struct aealtek_lib_ui_param {
	struct ae_ctrl_param_work work_info;
	enum ae_scene_mode_t scene;
	enum ae_iso_mode_t iso;
	cmr_s32 ui_ev_level;
	enum ae_metering_mode_type_t weight;
	struct ae_ctrl_param_range_fps fps;
	enum ae_antiflicker_mode_t flicker;
};

struct aealtek_status {
	struct aealtek_ui_param ui_param;
	struct aealtek_lib_ui_param lib_ui_param;
	cmr_s32 is_quick_mode;
	cmr_s32 is_hdr_status;
	cmr_u32 min_frame_length;
	cmr_u32 min_frame_length_slv;
};

enum aealtek_flash_state {
	AEALTEK_FLASH_STATE_PREPARE_ON,
	AEALTEK_FLASH_STATE_LIGHTING,
	AEALTEK_FLASH_STATE_CLOSE,
	AEALTEK_FLASH_STATE_KEEPING,
	AEALTEK_FLASH_STATE_MAX,
};

struct aealtek_lib_exposure_data {
	/*bracket*/
	cmr_u32 valid_exp_num;
	struct aealtek_exposure_param bracket_exp[BRACKET_NUM];

	/*init*/
	struct aealtek_exposure_param init;

	/*preview*/
	struct aealtek_exposure_param preview;

	/*snapshot*/
	struct aealtek_exposure_param  snapshot;
};

struct aealtek_led_cfg {
	cmr_s32 idx;
	cmr_s32 current;
	cmr_u32 time_ms;
};

struct aealtek_flash_cfg {
	cmr_s32 led_num;
	struct aealtek_led_cfg led_0;
	struct aealtek_led_cfg led_1;
	struct aealtek_exposure_param exp_cell;
};

struct aealtek_flash_param {
	cmr_s32 enable;
	enum aealtek_flash_state flash_state;
	struct aealtek_flash_cfg pre_flash_before;
	struct aealtek_flash_cfg main_flash_est;
	struct isp_flash_led_info led_info;
};

struct aealtek_touch_param {
	cmr_u32 touch_flag;
	cmr_u32 ctrl_roi_changed_flag;
	cmr_u32 ctrl_roi_converge_flag;
	cmr_u32 ctrl_convergence_req_flag;
};

struct aealtek_update_list {
	cmr_s32 is_iso:1;
	cmr_s32 is_target_lum:1;
	cmr_s32 is_quick:1;
	cmr_s32 is_scene:1;
	cmr_s32 is_weight:1;
	cmr_s32 is_touch_zone:1;
	cmr_s32 is_ev:1;
	cmr_s32 is_fps:1;
	cmr_s32 is_flicker:1;
};

struct aealtek_stat_info {
	struct isp_ae_statistic_info ae_stats;
	cmr_u32 statsy[ISP_AE_BLOCK];
};

struct aealtek_lib_data {
	cmr_s32 is_called_hwisp_cfg; //called default hw isp config
	struct alhw3a_ae_cfginfo_t hwisp_cfg;
	struct ae_output_data_t output_data;
	struct al3awrapper_stats_ae_t stats_data;
	struct ae_output_data_t temp_output_data;
	struct calib_wb_gain_t ae_otp_data;
	struct calib_wb_gain_t ae_otp_data_slv;
	struct aealtek_lib_exposure_data exposure_array;
};

struct aealtek_tuning_info {
	cmr_s32 manual_ae_on;
	cmr_s32 num;
	enum ae_tuning_mode tuning_mode;
	cmr_s32 exposure[TUNING_EXPOSURE_NUM];
	cmr_s32 gain[TUNING_EXPOSURE_NUM];
};

struct aealtek_script_info {
	cmr_u8 is_script_mode;
	enum ae_script_mode_t pre_script_mode;
	enum ae_script_mode_t script_mode;
	cmr_u8 is_flash_on;
};

/*ae altek context*/
struct aealtek_cxt {
	cmr_u32 is_inited;
	cmr_u32 is_processed;
	cmr_u32 work_cnt; //work number by called
	cmr_u32 camera_id;
	cmr_handle caller_handle;
	cmr_handle lib_handle;
	struct ae_ctrl_init_in init_in_param;
	struct alaeruntimeobj_t al_obj;
	struct aealtek_lib_ops lib_ops;

	void *seq_handle;

	cmr_s32 lock_cnt;
	cmr_s32 hdr_lock_cnt;
	cmr_s32 pre_cnt;
	cmr_s32 ae_state;

	cmr_s32 flash_skip_number;
	cmr_u16 is_refocus;
	cmr_u8 is_master;
	struct aealtek_script_info script_info;
	struct aealtek_status prv_status;
	struct aealtek_status cur_status;
	struct aealtek_status nxt_status;
	struct aealtek_update_list update_list;

	struct aealtek_flash_param flash_param;
	struct aealtek_touch_param touch_param;

	struct aealtek_lib_data lib_data;

	struct ae_ctrl_proc_in proc_in;
	struct aealtek_sensor_exp_data sensor_exp_data;
	struct aealtek_exposure_param pre_write_exp_data;
	struct aealtek_exposure_param pre_write_exp_data_slv;
	struct aealtek_tuning_info tuning_info;
	struct aealtek_stat_info stat_info[10];
	void *tuning_param[ISP_INDEX_MAX];
	cmr_u32 stat_info_num;
	cmr_u32 is_3dcalibration;/**add for 3d calibration*/
	int flash_state_machine;
	cmr_u32 hdr_enable;
	cmr_u32 main_flash_status;
	cmr_u32 zsl_mode;
};

static cmr_int aealtek_reset_touch_ack(struct aealtek_cxt *cxt_ptr);
static cmr_int aealtek_set_lock(struct aealtek_cxt *cxt_ptr, cmr_int is_lock, const char *cb_func);
static cmr_int aealtek_set_hdr_lock(struct aealtek_cxt *cxt_ptr, cmr_int is_lock, const char *cb_func);
static cmr_int aealtek_set_flash_est(struct aealtek_cxt *cxt_ptr, cmr_u32 is_reset);
static cmr_int aealtek_enable_debug_report(struct aealtek_cxt *cxt_ptr, cmr_int enable);
static cmr_int aealtek_set_tuning_param(struct aealtek_cxt *cxt_ptr, void *tuning_param);
static cmr_int aealtek_set_lib_roi(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr);
static cmr_int aealtek_set_hdr_ev(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr);
/********function start******/
static void aealtek_print_lib_log(struct aealtek_cxt *cxt_ptr, struct ae_output_data_t *in_ptr)
{
	#define report_str "report data:"
	#define output_str "output data:"


	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, in_ptr);
		goto exit;
	}
#if 0
	ISP_LOGI(output_str"hw3a_frame_id:%d", in_ptr->hw3a_frame_id);
	ISP_LOGI(output_str"sys_sof_index:%d", in_ptr->sys_sof_index);
	ISP_LOGI(output_str"hw3a_ae_block_nums:%d", in_ptr->hw3a_ae_block_nums);
	ISP_LOGI(output_str"hw3a_ae_block_totalpixels:%d", in_ptr->hw3a_ae_block_totalpixels);

	ISP_LOGI(output_str"ae_targetmean:%d", in_ptr->ae_targetmean);
	ISP_LOGI(output_str"ae_cur_mean:%d", in_ptr->ae_cur_mean);
	ISP_LOGI(output_str"ae_nonwt_mean:%d", in_ptr->ae_nonwt_mean);
	ISP_LOGI(output_str"ae_center_mean2x2:%d", in_ptr->ae_center_mean2x2);
	ISP_LOGI(output_str"BVresult:%d", in_ptr->BVresult);
	ISP_LOGI(output_str"BG_BVResult:%d", in_ptr->BG_BVResult);

	ISP_LOGI(output_str"ae_converged:%d", in_ptr->ae_converged);
	ISP_LOGI(output_str"ae_need_flash_flg:%d", in_ptr->ae_need_flash_flg);
	ISP_LOGI(output_str"ae_est_status:%d", in_ptr->ae_est_status);
	ISP_LOGI(output_str"fe_est_status:%d", in_ptr->fe_est_status);

	ISP_LOGI(output_str"udCur_ISO:%d", in_ptr->udCur_ISO);
	ISP_LOGI(output_str"udCur_exposure_time:%d", in_ptr->udCur_exposure_time);
	ISP_LOGI(output_str"udCur_exposure_line:%d", in_ptr->udCur_exposure_line);
	ISP_LOGI(output_str"udCur_sensor_ad_gain:%d", in_ptr->udCur_sensor_ad_gain);
	ISP_LOGI(output_str"udCur_ISP_D_gain:%d", in_ptr->udCur_ISP_D_gain);
	ISP_LOGI(output_str"uwCur_fps:%d", in_ptr->uwCur_fps);

	ISP_LOGI(output_str"udISO:%d", in_ptr->udISO);
	ISP_LOGI(output_str"udexposure_time:%d", in_ptr->udexposure_time);
	ISP_LOGI(output_str"udexposure_line:%d", in_ptr->udexposure_line);
	ISP_LOGI(output_str"udsensor_ad_gain:%d", in_ptr->udsensor_ad_gain);
	ISP_LOGI(output_str"udISP_D_gain:%d", in_ptr->udISP_D_gain);

	ISP_LOGI(output_str"ae_roi_change_st:%d", in_ptr->ae_roi_change_st);
	ISP_LOGI(output_str"flicker_mode:%d", in_ptr->flicker_mode);
	ISP_LOGI(output_str"ae_metering_mode:%d", in_ptr->ae_metering_mode);
	ISP_LOGI(output_str"bv_delta:%d", in_ptr->bv_delta);
	ISP_LOGI(output_str"ae_non_comp_bv_val:%d", in_ptr->ae_non_comp_bv_val);
	ISP_LOGI(output_str"udADGainx100PerISO100Gain:%d", in_ptr->udADGainx100PerISO100Gain);
#endif

#if 0
	ISP_LOGI(report_str"hw3a_frame_id:%d", in_ptr->rpt_3a_update.ae_update.hw3a_frame_id);
	ISP_LOGI(report_str"sys_sof_index:%d", in_ptr->rpt_3a_update.ae_update.sys_sof_index);
	ISP_LOGI(report_str"hw3a_ae_block_nums:%d", in_ptr->rpt_3a_update.ae_update.hw3a_ae_block_nums);
	ISP_LOGI(report_str"hw3a_ae_block_totalpixels:%d", in_ptr->rpt_3a_update.ae_update.hw3a_ae_block_totalpixels);

	ISP_LOGI(report_str"targetmean:%d", in_ptr->rpt_3a_update.ae_update.targetmean);
	ISP_LOGI(report_str"curmean:%d", in_ptr->rpt_3a_update.ae_update.curmean);
	ISP_LOGI(report_str"avgmean:%d", in_ptr->rpt_3a_update.ae_update.avgmean);
	ISP_LOGI(report_str"center_mean2x2:%d", in_ptr->rpt_3a_update.ae_update.center_mean2x2);
	ISP_LOGI(report_str"bv_val:%d", in_ptr->rpt_3a_update.ae_update.bv_val);
	ISP_LOGI(report_str"BG_BVResult:%d", in_ptr->rpt_3a_update.ae_update.BG_BVResult);

	ISP_LOGI(report_str"ae_converged:%d", in_ptr->rpt_3a_update.ae_update.ae_converged);
	ISP_LOGI(report_str"ae_need_flash_flg:%d", in_ptr->rpt_3a_update.ae_update.ae_need_flash_flg);
	ISP_LOGI(report_str"ae_LibStates:%d", in_ptr->rpt_3a_update.ae_update.ae_LibStates);
	ISP_LOGI(report_str"ae_FlashStates:%d", in_ptr->rpt_3a_update.ae_update.ae_FlashStates);

	ISP_LOGI(report_str"sensor_ad_gain:%d", in_ptr->rpt_3a_update.ae_update.sensor_ad_gain);
	ISP_LOGI(report_str"isp_d_gain:%d", in_ptr->rpt_3a_update.ae_update.isp_d_gain);
	ISP_LOGI(report_str"exposure_time:%d", in_ptr->rpt_3a_update.ae_update.exposure_time);
	ISP_LOGI(report_str"exposure_line:%d", in_ptr->rpt_3a_update.ae_update.exposure_line);
	ISP_LOGI(report_str"exposure_max_line:%d", in_ptr->rpt_3a_update.ae_update.exposure_max_line);
	ISP_LOGI(report_str"ae_cur_iso:%d", in_ptr->rpt_3a_update.ae_update.ae_cur_iso);

	ISP_LOGI(report_str"ae_roi_change_st:%d", in_ptr->rpt_3a_update.ae_update.ae_roi_change_st);
	ISP_LOGI(report_str"flicker_mode:%d", in_ptr->rpt_3a_update.ae_update.flicker_mode);
	ISP_LOGI(report_str"ae_metering_mode:%d", in_ptr->rpt_3a_update.ae_update.ae_metering_mode);
	ISP_LOGI(report_str"bv_delta:%d", in_ptr->rpt_3a_update.ae_update.bv_delta);
	ISP_LOGI(report_str"cur_fps:%d", in_ptr->rpt_3a_update.ae_update.cur_fps);
	ISP_LOGI(report_str"ae_non_comp_bv_val:%d", in_ptr->rpt_3a_update.ae_update.ae_non_comp_bv_val);
#endif
exit:
	return;
}

static cmr_int aealtek_set_min_frame_length(struct aealtek_cxt *cxt_ptr, cmr_int max_fps, cmr_int line_time)
{
	cmr_int ret = ISP_ERROR;


	if (!cxt_ptr) {
		ISP_LOGE("param %p is NULL !!!", cxt_ptr);
		ret = ISP_PARAM_NULL;
		goto exit;
	}
	if (0 != max_fps && 0 != line_time) {
		cxt_ptr->cur_status.min_frame_length = SENSOR_EXP_US_BASE / max_fps / line_time;

		if (cxt_ptr->is_refocus && cxt_ptr->is_master) {
			cxt_ptr->cur_status.min_frame_length_slv = SENSOR_EXP_US_BASE / max_fps
				/ cxt_ptr->cur_status.ui_param.work_info_slv.resolution.line_time;
			ISP_LOGI("min_frame_length_slv=%d", cxt_ptr->cur_status.min_frame_length_slv);
		}
	} else {
		cxt_ptr->cur_status.min_frame_length = 0;
	}
	ISP_LOGI("min_frame_length=%d", cxt_ptr->cur_status.min_frame_length);
	return ISP_SUCCESS;
exit:
	return ret;
}

static cmr_int aealtek_set_dummy(struct aealtek_cxt *cxt_ptr, struct aealtek_exposure_param *exp_param_ptr)
{
	cmr_int ret = ISP_ERROR;
	cmr_u32 min_frame_length = 0;


	if (!cxt_ptr || !exp_param_ptr) {
		ISP_LOGE("param %p %p is NULL !!!", cxt_ptr, exp_param_ptr);
		ret = ISP_PARAM_NULL;
		goto exit;
	}
	exp_param_ptr->dummy = 0;
	min_frame_length = cxt_ptr->cur_status.min_frame_length;
	if (0 != min_frame_length) {
		if (exp_param_ptr->exp_line < min_frame_length) {
			exp_param_ptr->dummy = min_frame_length - exp_param_ptr->exp_line;
		}
	}
	return ISP_SUCCESS;
exit:
	return ret;
}

static cmr_int aealtek_convert_otp(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_otp_data otp_data
		, struct calib_wb_gain_t  *lib_otp_ptr)
{
	cmr_int ret = ISP_ERROR;

	if (!cxt_ptr) {
		ISP_LOGE("param %p is NULL !!!", cxt_ptr);
		ret = ISP_PARAM_NULL;
		goto exit;
	}

	if (0 != otp_data.r
		&& 0 != otp_data.g
		&& 0 != otp_data.b) {
		lib_otp_ptr->r = otp_data.r;
		lib_otp_ptr->g = otp_data.g;
		lib_otp_ptr->b = otp_data.b;
		lib_otp_ptr->minISO = otp_data.iso;
	} else {
		lib_otp_ptr->r = 1500;
		lib_otp_ptr->g = 1300;
		lib_otp_ptr->b = 1600;
		lib_otp_ptr->minISO = 50;
		ISP_LOGE("NO OTP DATA !!!");
	}
	return ISP_SUCCESS;
exit:
	return ret;
}

static cmr_int aealtek_convert_lib_exposure2outdata(struct aealtek_cxt *cxt_ptr, struct aealtek_exposure_param *from_ptr, struct ae_output_data_t *to_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;

	if (!cxt_ptr || !from_ptr || !to_ptr) {
		ISP_LOGE("param %p %p %p is NULL error!", cxt_ptr, from_ptr, to_ptr);
		goto exit;
	}
	to_ptr->rpt_3a_update.ae_update.exposure_line = from_ptr->exp_line;
	to_ptr->rpt_3a_update.ae_update.exposure_time = from_ptr->exp_time;
	to_ptr->rpt_3a_update.ae_update.sensor_ad_gain = from_ptr->gain;
	to_ptr->rpt_3a_update.ae_update.ae_cur_iso = from_ptr->iso;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_lib_exposure2sensor(struct aealtek_cxt *cxt_ptr, struct ae_output_data_t *from_ptr, struct aealtek_exposure_param *to_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;

	if (!cxt_ptr || !from_ptr || !to_ptr) {
		ISP_LOGE("param %p %p %p is NULL error!", cxt_ptr, from_ptr, to_ptr);
		goto exit;
	}
	to_ptr->exp_line = from_ptr->rpt_3a_update.ae_update.exposure_line;
	to_ptr->exp_time = from_ptr->rpt_3a_update.ae_update.exposure_time;
	to_ptr->gain = from_ptr->rpt_3a_update.ae_update.sensor_ad_gain;
	to_ptr->iso = from_ptr->rpt_3a_update.ae_update.ae_cur_iso;
	to_ptr->size_index = cxt_ptr->nxt_status.ui_param.work_info.resolution.sensor_size_index;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_lib_report2out(struct ae_report_update_t *from_ptr, struct isp3a_ae_report *to_ptr)
{
	cmr_int ret = ISP_ERROR;


	if (!from_ptr || !to_ptr) {
		ISP_LOGE("param %p %p is NULL error!", from_ptr, to_ptr);
		goto exit;
	}
	to_ptr->hw3a_frame_id = from_ptr->hw3a_frame_id;
//	to_ptr->block_num = from_ptr->hw3a_ae_block_num;
	to_ptr->block_totalpixels = from_ptr->hw3a_ae_block_totalpixels;
	to_ptr->bv_val = from_ptr->bv_val;
	to_ptr->y_stats = from_ptr->hw3a_ae_stats;
	to_ptr->sys_sof_index = from_ptr->sys_sof_index;
	to_ptr->bv_delta = from_ptr->bv_delta;

	to_ptr->ae_state = from_ptr->ae_LibStates;
	to_ptr->fe_state = from_ptr->ae_FlashStates;
	to_ptr->ae_converge_st = from_ptr->ae_converged;
	to_ptr->BV = from_ptr->bv_val;
	to_ptr->non_comp_BV = from_ptr->ae_non_comp_bv_val;
	to_ptr->ISO = from_ptr->ae_cur_iso;
	to_ptr->avg_mean = from_ptr->avgmean;
	to_ptr->center_mean2x2 = from_ptr->center_mean2x2;
	to_ptr->cur_mean = from_ptr->curmean;
	to_ptr->target_mean = from_ptr->targetmean;
	to_ptr->sensor_ad_gain = from_ptr->sensor_ad_gain;
	to_ptr->exp_line = from_ptr->exposure_line;
	to_ptr->exp_time = from_ptr->exposure_time;
	to_ptr->fps = from_ptr->cur_fps;

#if 0
	//	to_ptr->flash_ratio = from_ptr->flash_ratio;
	//	to_ptr->flash_gain_r = from_ptr->flash_gain.r_gain;
	//	to_ptr->flash_gain_g = from_ptr->flash_gain.g_gain;
	//	to_ptr->flash_gain_b = from_ptr->flash_gain.b_gain;
	//	to_ptr->flash_gain_r_led2 = from_ptr->flash_gain_led2.r_gain;
	//	to_ptr->flash_gain_g_led2 = from_ptr->flash_gain_led2.g_gain;
	//	to_ptr->flash_gain_b_led2 = from_ptr->flash_gain_led2.b_gain;
	//	to_ptr->flash_ratio_led2 = from_ptr->flash_ratio_led2;
	//	to_ptr->flash_ct_led1 = from_ptr->flash_c;
	//	to_ptr->flash_ct_led2;
#endif

	to_ptr->debug_data_size = from_ptr->ae_debug_valid_size;
	to_ptr->debug_data = from_ptr->ae_debug_data;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_lib_to_out_report(struct ae_output_data_t *from_ptr, struct isp3a_ae_report *to_ptr)
{
	cmr_int ret = ISP_ERROR;


	if (!from_ptr || !to_ptr) {
		ISP_LOGE("param %p %p is NULL error!", from_ptr, to_ptr);
		goto exit;
	}

	to_ptr->ae_nonwt_mean = from_ptr->ae_nonwt_mean;
	to_ptr->ae_center_mean2x2 = from_ptr->ae_center_mean2x2;
	ret = aealtek_lib_report2out(&from_ptr->rpt_3a_update.ae_update, to_ptr);
	if (ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_lib_to_out_info(struct aealtek_cxt *cxt_ptr, struct ae_output_data_t *from_ptr, struct isp3a_ae_info *to_ptr)
{
	cmr_int ret = ISP_ERROR;

	if (!cxt_ptr || !from_ptr || !to_ptr) {
		ISP_LOGE("param %p %p %p is NULL error!", cxt_ptr, from_ptr, to_ptr);
		goto exit;
	}

	if (cxt_ptr->lock_cnt)
		to_ptr->ae_ctrl_locked = 1;
	else
		to_ptr->ae_ctrl_locked = 0;

	to_ptr->ae_ctrl_converged_flag = from_ptr->rpt_3a_update.ae_update.ae_converged;
	to_ptr->ae_ctrl_state = cxt_ptr->ae_state;

	ISP_LOGI("ae_ctrl_locked=%d, is_converg=%d, ae_state=%d",
			to_ptr->ae_ctrl_locked,
			to_ptr->ae_ctrl_converged_flag,
			to_ptr->ae_ctrl_state);

	ret = aealtek_lib_to_out_report(from_ptr, &to_ptr->report_data);
	if (ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_unload_lib(struct aealtek_cxt *cxt_ptr)
{
	cmr_int ret = ISP_ERROR;

	if (!cxt_ptr) {
		ISP_LOGE("param %p is NULL !!!", cxt_ptr);
		ret = ISP_PARAM_NULL;
		goto exit;
	}

	if (cxt_ptr->lib_handle) {
		dlclose(cxt_ptr->lib_handle);
		cxt_ptr->lib_handle = NULL;
	}
	return ISP_SUCCESS;
exit:
	return ret;
}

static cmr_int aealtek_get_default_param(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_work *work_ptr, struct aealtek_status *st_ptr)
{
	cmr_int ret = ISP_ERROR;


	if (!cxt_ptr || !st_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, st_ptr);
		goto exit;
	}
	st_ptr->ui_param.work_info = *work_ptr;

	st_ptr->is_quick_mode = 0;
	if (0 == st_ptr->ui_param.work_info.resolution.frame_size.w
		|| 0 == st_ptr->ui_param.work_info.resolution.frame_size.h) {
		st_ptr->ui_param.work_info.work_mode = ISP3A_WORK_MODE_PREVIEW;
		st_ptr->ui_param.work_info.capture_mode = ISP_CAP_MODE_AUTO;
		st_ptr->ui_param.work_info.resolution.frame_size.w = 1600;
		st_ptr->ui_param.work_info.resolution.frame_size.h = 1200;
		st_ptr->ui_param.work_info.resolution.frame_line = 1600;
		st_ptr->ui_param.work_info.resolution.line_time = 10 * SENSOR_EXP_BASE; //dummy
		st_ptr->ui_param.work_info.resolution.sensor_size_index = 1;
		st_ptr->ui_param.work_info.resolution.max_fps = 30;
		st_ptr->ui_param.work_info.resolution.max_gain = 8;
	}
	st_ptr->ui_param.scene = AE_CTRL_SCENE_NORMAL;
	st_ptr->ui_param.flicker = AE_CTRL_FLICKER_50HZ;
	st_ptr->ui_param.iso = AE_CTRL_ISO_AUTO;
	st_ptr->ui_param.ui_ev_level = 3;
	st_ptr->ui_param.weight = AE_CTRL_MEASURE_LUM_CENTER;
	st_ptr->ui_param.fps.min_fps = 2;
	st_ptr->ui_param.fps.max_fps = 30;
	cxt_ptr->flash_state_machine = AEALTEK_FLASH_STATE_CLOSE;
	cxt_ptr->main_flash_status = 0;

	if (0 == cxt_ptr->init_in_param.sensor_static_info.f_num) {
		cxt_ptr->init_in_param.sensor_static_info.f_num = 280;
	}
	if (0 == cxt_ptr->init_in_param.sensor_static_info.max_gain) {
		cxt_ptr->init_in_param.sensor_static_info.max_gain = 8;
	}
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static void aealtek_flicker_ui2lib(enum ae_ctrl_flicker_mode from, enum ae_antiflicker_mode_t *to_ptr)
{
	enum ae_antiflicker_mode_t lib_flicker = 0;


	if (!to_ptr) {
		ISP_LOGE("param is NULL !!!");
		goto exit;
	}
	switch (from) {
	case AE_CTRL_FLICKER_50HZ:
		lib_flicker = ANTIFLICKER_50HZ;
		break;
	case AE_CTRL_FLICKER_60HZ:
		lib_flicker = ANTIFLICKER_60HZ;
		break;
	case AE_CTRL_FLICKER_OFF:
		lib_flicker = ANTIFLICKER_OFF;
		break;
	case AE_CTRL_FLICKER_AUTO:
		lib_flicker = ANTIFLICKER_50HZ;
		break;
	default:
		ISP_LOGW("not support flicker mode %d", from);
		break;
	}
	*to_ptr = lib_flicker;
exit:
	return;
}

static void aealtek_antiflicker_mode_ui2lib(enum ae_ctrl_flicker_mode from, enum antiflicker_ui_mode_t *to_ptr)
{
	enum antiflicker_ui_mode_t lib_antiflicker = 0;

	if (!to_ptr) {
		ISP_LOGE("param is NULL !!!");
		goto exit;
	}
	switch (from) {
	case AE_CTRL_FLICKER_AUTO:
		lib_antiflicker = ANTIFLICKER_UI_MODE_AUTO;
		break;
	case AE_CTRL_FLICKER_50HZ:
		lib_antiflicker = ANTIFLICKER_UI_MODE_FIXED_50HZ;
		break;
	case AE_CTRL_FLICKER_60HZ:
		lib_antiflicker = ANTIFLICKER_UI_MODE_FIXED_60HZ;
		break;
	case AE_CTRL_FLICKER_OFF:
		lib_antiflicker = ANTIFLICKER_UI_MODE_OFF;
		break;
	default:
		ISP_LOGW("not support antiflicker mode %d", from);
		break;
	}
	*to_ptr = lib_antiflicker;
exit:
	return;
}

static void aealtek_weight_ui2lib(enum ae_ctrl_measure_lum_mode from, enum ae_metering_mode_type_t *to_ptr)
{
	enum ae_metering_mode_type_t lib_metering = 0;


	if (!to_ptr) {
		ISP_LOGE("param is NULL !!!");
		goto exit;
	}
	switch (from) {
	case AE_CTRL_MEASURE_LUM_AVG:
		lib_metering = AE_METERING_AVERAGE;
		break;
	case AE_CTRL_MEASURE_LUM_CENTER:
		//lib_metering = AE_METERING_CENTERWT;
		lib_metering = AE_METERING_CENTERWT;
		break;
	case AE_CTRL_MEASURE_LUM_SPOT:
		//lib_metering = AE_METERING_SPOTWT;
		lib_metering = AE_METERING_CENTERWT;
		break;
	case AE_CTRL_MEASURE_LUM_INTELLIWT:
		lib_metering = AE_METERING_CENTERWT;
		break;
	case AE_CTRL_MEASURE_LUM_TOUCH:
		lib_metering = AE_METERING_CENTERWT;
		break;
	case AE_CTRL_MEASURE_LUM_USERDEF:
		lib_metering = AE_METERING_USERDEF_WT;
		break;
	default:
		lib_metering = AE_METERING_CENTERWT;
		ISP_LOGW("not support weight mode %d", from);
		break;
	}
	*to_ptr = lib_metering;
	ISP_LOGI("weight form %d mode %d", from,  *to_ptr = lib_metering);
exit:
	return;
}

static cmr_int aealtek_sensor_info_ui2lib(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_resolution *from_ptr, struct ae_sensor_info_t *to_ptr)
{
	cmr_int ret = ISP_ERROR;
	struct ae_ctrl_param_sensor_static_info *static_sensor_ptr = NULL;


	if (!cxt_ptr || !from_ptr || !to_ptr) {
		ISP_LOGE("param %p %p %p is NULL error!", cxt_ptr, from_ptr, to_ptr);
		goto exit;
	}
	static_sensor_ptr = &cxt_ptr->init_in_param.sensor_static_info;

	/*	sensor info setting, RAW size, fps , etc. */
	to_ptr->min_fps = 100 * from_ptr->min_fps;
	to_ptr->max_fps = 100 * from_ptr->max_fps;
	to_ptr->min_line_cnt = 1;
	to_ptr->max_line_cnt = MAX_EXP_LINE_CNT;
	to_ptr->exposuretime_per_exp_line_ns = from_ptr->line_time;
	to_ptr->min_gain = 100 * 1;
	to_ptr->max_gain = 100 * from_ptr->max_gain;

	to_ptr->sensor_left = 0;
	to_ptr->sensor_top = 0;
	to_ptr->sensor_width = from_ptr->frame_size.w;
	to_ptr->sensor_height = from_ptr->frame_size.h;

	to_ptr->sensor_fullwidth = from_ptr->frame_size.w;
	to_ptr->sensor_fullheight = from_ptr->frame_size.h;

	to_ptr->ois_supported = static_sensor_ptr->ois_supported;
	to_ptr->f_number = (float)(1.0 * static_sensor_ptr->f_num / F_NUM_BASE);
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld", ret);
	return ret;
}

static cmr_int aealtek_convert_ui2initlib(struct aealtek_cxt *cxt_ptr, struct aealtek_status *from_ptr, struct ae_set_parameter_init_t *to_ptr)
{
	cmr_int ret = ISP_ERROR;
	struct ae_ctrl_param_resolution *resolution_ptr = NULL;


	if (!cxt_ptr || !from_ptr || !to_ptr) {
		ISP_LOGE("param %p %p %p is NULL error!", cxt_ptr, from_ptr, to_ptr);
		goto exit;
	}
	cmr_bzero(to_ptr, sizeof(*to_ptr));
	resolution_ptr = &from_ptr->ui_param.work_info.resolution;

	/*  tuning parameter setting  */
	to_ptr->ae_setting_data = cxt_ptr->init_in_param.tuning_param[0];

	/* initial UI setting */
	aealtek_flicker_ui2lib(cxt_ptr->nxt_status.ui_param.flicker, &to_ptr->ae_set_antibaning_mode);
	aealtek_weight_ui2lib(cxt_ptr->nxt_status.ui_param.weight, &to_ptr->ae_metering_mode);

	/*	basic control param  */
	to_ptr->ae_enable = 1;
	to_ptr->ae_lock = 0;

	/*	sensor info setting, RAW size, fps , etc. */
	ret = aealtek_sensor_info_ui2lib(cxt_ptr, resolution_ptr, &to_ptr->preview_sensor_info);
	if (ret)
		goto exit;

	to_ptr->capture_sensor_info = to_ptr->preview_sensor_info;

	to_ptr->ae_calib_wb_gain.calib_r_gain = cxt_ptr->lib_data.ae_otp_data.r;
	to_ptr->ae_calib_wb_gain.calib_g_gain = cxt_ptr->lib_data.ae_otp_data.g;
	to_ptr->ae_calib_wb_gain.calib_b_gain = cxt_ptr->lib_data.ae_otp_data.b;
	to_ptr->ae_calib_wb_gain.minISO = cxt_ptr->lib_data.ae_otp_data.minISO;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld", ret);
	return ret;
}

static cmr_int aealtek_sensor_info_ui2lib_slv(struct aealtek_cxt *cxt_ptr,
					      struct ae_ctrl_param_resolution *from_ptr,
					      struct ae_sensor_info_t *to_ptr)
{
	cmr_int ret = ISP_ERROR;
	struct ae_ctrl_param_sensor_static_info *static_sensor_ptr = NULL;


	if (!cxt_ptr || !from_ptr || !to_ptr) {
		ISP_LOGE("param %p %p %p is NULL error!", cxt_ptr, from_ptr, to_ptr);
		goto exit;
	}
	static_sensor_ptr = &cxt_ptr->init_in_param.sensor_static_info_slv;

	/*	sensor info setting, RAW size, fps , etc. */
	to_ptr->min_fps = 100 * from_ptr->min_fps;
	to_ptr->max_fps = 100 * from_ptr->max_fps;
	to_ptr->min_line_cnt = 1;
	to_ptr->max_line_cnt = MAX_EXP_LINE_CNT;
	to_ptr->exposuretime_per_exp_line_ns = from_ptr->line_time;
	to_ptr->min_gain = 100 * 1;
	to_ptr->max_gain = 100 * from_ptr->max_gain;

	to_ptr->sensor_left = 0;
	to_ptr->sensor_top = 0;
	to_ptr->sensor_width = from_ptr->frame_size.w;
	to_ptr->sensor_height = from_ptr->frame_size.h;

	to_ptr->sensor_fullwidth = from_ptr->frame_size.w;
	to_ptr->sensor_fullheight = from_ptr->frame_size.h;

	to_ptr->ois_supported = static_sensor_ptr->ois_supported;
	to_ptr->f_number = (float)(1.0 * static_sensor_ptr->f_num / F_NUM_BASE);
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld", ret);
	return ret;
}

cmr_int aealtek_set_slv_otp(struct calib_wb_gain_t acalibwbgain,
			    struct alaeruntimeobj_t *aaelibcallback,
			    struct ae_output_data_t *ae_output,
			    void *ae_runtimedat)
{
	cmr_int ret = ERR_WPR_AE_SUCCESS;
	struct ae_set_param_t localparam;    /* local parameter set */

	if (acalibwbgain.r == 0 || acalibwbgain.g == 0 || acalibwbgain.b == 0)
		return ERR_WRP_AE_INVALID_INPUT_WB;

	memset(&localparam, 0, sizeof(struct ae_set_param_content_t));
	localparam.ae_set_param_type = AE_SET_PARAM_OTP_WB_DAT_SLV;
	localparam.set_param.ae_calib_wb_gain_slv_sensor.calib_r_gain = acalibwbgain.r;
	localparam.set_param.ae_calib_wb_gain_slv_sensor.calib_g_gain = acalibwbgain.g;
	localparam.set_param.ae_calib_wb_gain_slv_sensor.calib_b_gain = acalibwbgain.b;
	localparam.set_param.ae_calib_wb_gain_slv_sensor.minISO = acalibwbgain.minISO;
	if (0 == localparam.set_param.ae_calib_wb_gain_slv_sensor.minISO) {
		localparam.set_param.ae_calib_wb_gain_slv_sensor.minISO = _DEFAULT_WRAPPER_MINISO;
	}

	ret = aaelibcallback->set_param(&localparam, ae_output, ae_runtimedat);
	if (ret != ERR_WPR_AE_SUCCESS)
		return ret;

	return ret;
}

static cmr_int aealtek_load_otp(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_otp_data otp_data, struct ae_ctrl_otp_data otp_data_slv)
{
	cmr_int ret = ISP_ERROR;

	UINT32 lib_ret = 0;
	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct calib_wb_gain_t  *lib_otp_ptr;

	if (!cxt_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}
	obj_ptr = &cxt_ptr->al_obj;

	lib_otp_ptr = &cxt_ptr->lib_data.ae_otp_data;
	ret = aealtek_convert_otp(cxt_ptr, otp_data, lib_otp_ptr);
	if (ret)
		goto exit;

	ISP_LOGI("ae_otp_data r=%d,g=%d,b=%d", cxt_ptr->lib_data.ae_otp_data.r,
			cxt_ptr->lib_data.ae_otp_data.g,
			cxt_ptr->lib_data.ae_otp_data.b);

	if (obj_ptr) {
		lib_ret = al3awrapperae_updateotp2aelib(cxt_ptr->lib_data.ae_otp_data,
								obj_ptr, &cxt_ptr->lib_data.output_data, obj_ptr->ae);
		if (lib_ret)
			goto exit;
	}

	if (cxt_ptr->is_refocus && cxt_ptr->is_master) {
		lib_otp_ptr = &cxt_ptr->lib_data.ae_otp_data_slv;
		ret = aealtek_convert_otp(cxt_ptr, otp_data_slv, lib_otp_ptr);
		if (ret)
			goto exit;
		ISP_LOGI("ae_otp_data_slv r=%d,g=%d,b=%d", cxt_ptr->lib_data.ae_otp_data_slv.r,
				cxt_ptr->lib_data.ae_otp_data_slv.g,
				cxt_ptr->lib_data.ae_otp_data_slv.b);
		if (obj_ptr) {
			lib_ret = aealtek_set_slv_otp(cxt_ptr->lib_data.ae_otp_data_slv,
									obj_ptr, &cxt_ptr->lib_data.output_data, obj_ptr->ae);
			if (lib_ret)
				goto exit;
		}
	}

	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld lib_ret=%d !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_load_lib(struct aealtek_cxt *cxt_ptr)
{
	cmr_int ret = ISP_ERROR;
	cmr_int is_ret = 0;
	struct alaeruntimeobj_t *obj_ptr = NULL;
	UINT32 identityID = 0;


	if (!cxt_ptr) {
		ISP_LOGE("param %p is NULL !!!", cxt_ptr);
		ret = ISP_PARAM_NULL;
		goto exit;
	}

	cxt_ptr->lib_handle = dlopen(AELIB_PATH, RTLD_NOW);
	if (!cxt_ptr->lib_handle) {
		ISP_LOGE("failed to dlopen");
		ret = ISP_ERROR;
		goto exit;
	}

	cxt_ptr->lib_ops.load_func = dlsym(cxt_ptr->lib_handle, "allib_ae_loadfunc");
	if (!cxt_ptr->lib_ops.load_func) {
		ISP_LOGE("failed to dlsym load func");
		ret = ISP_ERROR;
		goto error_dlsym;
	}
	cxt_ptr->lib_ops.get_lib_ver = dlsym(cxt_ptr->lib_handle, "allib_ae_getlib_version");
	if (!cxt_ptr->lib_ops.get_lib_ver) {
		ISP_LOGE("failed to dlsym get lib ver");
		ret = ISP_ERROR;
		goto error_dlsym;
	}

	return ISP_SUCCESS;
error_dlsym:
	aealtek_unload_lib(cxt_ptr);
exit:
	ISP_LOGE("ret=%ld", ret);
	return ret;
}

static cmr_int aealtek_get_lib_ver(struct aealtek_cxt *cxt_ptr)
{
	cmr_int ret = ISP_ERROR;
	struct alaelib_version_t lib_ver;


	if (!cxt_ptr) {
		ISP_LOGE("param %p is NULL error!", cxt_ptr);
		ret = ISP_PARAM_NULL;
		goto exit;
	}
	cmr_bzero(&lib_ver, sizeof(lib_ver));
	if (cxt_ptr->lib_ops.get_lib_ver)
		cxt_ptr->lib_ops.get_lib_ver(&lib_ver);
	ISP_LOGI("LIB Ver %04d.%04d", lib_ver.major_version, lib_ver.minor_version);

	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld", ret);
	return ret;
}

static cmr_int aealtek_init(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_init_in *in_ptr, struct ae_ctrl_init_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;
	cmr_int is_ret = 0;
	struct alaeruntimeobj_t *obj_ptr = NULL;
	UINT32 identityID = 0;
	struct ae_set_parameter_init_t init_setting;
	struct seq_init_in seq_in;
	cmr_u32 i = 0;


	if (!cxt_ptr || !in_ptr || !out_ptr) {
		ISP_LOGE("init param is null, input_ptr is %p, output_ptr is %p", in_ptr, out_ptr);
		ret = ISP_PARAM_NULL;
		goto exit;
	}

	obj_ptr = &cxt_ptr->al_obj;
	if (!obj_ptr) {
		goto exit;
	}
	identityID = cxt_ptr->camera_id;
	is_ret = cxt_ptr->lib_ops.load_func(obj_ptr, identityID);
	if (!is_ret) {
		ret = ISP_ERROR;
		ISP_LOGE("load func is failed!");
		goto exit;
	}
	aealtek_get_lib_ver(cxt_ptr);

	if (obj_ptr->initial)
		obj_ptr->initial(&obj_ptr->ae);

	ret = aealtek_load_otp(cxt_ptr, in_ptr->otp_data, in_ptr->otp_data_slv);
	if (ret)
		goto exit;

	ret = aealtek_get_default_param(cxt_ptr, &in_ptr->preview_work, &cxt_ptr->cur_status);
	if (ret)
		goto exit;
	cxt_ptr->cur_status.ui_param.work_info_slv = in_ptr->preview_work_slv;

	cxt_ptr->prv_status = cxt_ptr->cur_status;
	cxt_ptr->nxt_status = cxt_ptr->cur_status;

	seq_in.capture_skip_num = cxt_ptr->init_in_param.sensor_static_info.capture_skip_num;
	seq_in.preview_skip_num = cxt_ptr->init_in_param.sensor_static_info.preview_skip_num;
	seq_in.exp_valid_num = cxt_ptr->init_in_param.sensor_static_info.exposure_valid_num;
	seq_in.gain_valid_num = cxt_ptr->init_in_param.sensor_static_info.gain_valid_num;
	seq_in.idx_start_from = 1;

	ISP_LOGI("cap skip num=%d, preview skip num=%d, exp_valid_num=%d,gain_valid_num=%d",
			seq_in.capture_skip_num, seq_in.preview_skip_num,
			seq_in.exp_valid_num, seq_in.gain_valid_num);

	ret = seq_init(10, &seq_in, &cxt_ptr->seq_handle);
	if (ret || NULL == cxt_ptr->seq_handle)
		goto exit;

	ret = aealtek_enable_debug_report(cxt_ptr, 1);
	if (ret)
		goto exit;

	for (i = 0; i < ISP_INDEX_MAX; i++) {
		cxt_ptr->tuning_param[i] = in_ptr->tuning_param[i];
		ISP_LOGI("ae_addr=%p", cxt_ptr->tuning_param[i]);
	}
	ret = aealtek_set_tuning_param(cxt_ptr, in_ptr->tuning_param[0]);
	if (ret)
		ISP_LOGW("ae set tuning bin failed");

	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld", ret);
	return ret;
}

static void aealtek_change_ae_state(struct aealtek_cxt *cxt_ptr, enum isp3a_ae_ctrl_state from, enum isp3a_ae_ctrl_state to)
{
	UNUSED(from);
	cxt_ptr->ae_state = to;
}

static void aealtek_change_flash_state(struct aealtek_cxt *cxt_ptr, enum aealtek_flash_state from, enum aealtek_flash_state to)
{
	UNUSED(from);
	cxt_ptr->flash_param.flash_state = to;
}

static cmr_int aealtek_get_def_hwisp_cfg(struct alhw3a_ae_cfginfo_t *cfg_ptr)
{
	cmr_int ret = ISP_ERROR;


	if (!cfg_ptr) {
		ISP_LOGE("param %p is NULL error!", cfg_ptr);
		goto exit;
	}
	cfg_ptr->tokenid = 0x01;
	cfg_ptr->taeregion.uwborderratiox = 100;   // 100% use of current sensor cropped area
	cfg_ptr->taeregion.uwborderratioy = 100;   // 100% use of current sensor cropped area
	cfg_ptr->taeregion.uwblknumx = 16;         // fixed value for AE lib
	cfg_ptr->taeregion.uwblknumy = 16;         // fixed value for AE lib
	cfg_ptr->taeregion.uwoffsetratiox = 0;     // 0% offset of left of current sensor cropped area
	cfg_ptr->taeregion.uwoffsetratioy = 0;     // 0% offset of top of current sensor cropped area

	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld", ret);
	return ret;
}

static cmr_int aealtek_to_ae_hw_cfg(struct alhw3a_ae_cfginfo_t *from_ptr, struct isp3a_ae_hw_cfg *to_ptr)
{
	cmr_int ret = ISP_ERROR;

	if (!from_ptr || !to_ptr) {
		ISP_LOGE("param %p %p is NULL error!", from_ptr, to_ptr);
		goto exit;
	}
	to_ptr->tokenID = from_ptr->tokenid;
	to_ptr->region.border_ratio_X = from_ptr->taeregion.uwborderratiox;
	to_ptr->region.border_ratio_Y = from_ptr->taeregion.uwborderratioy;
	to_ptr->region.blk_num_X = from_ptr->taeregion.uwblknumx;
	to_ptr->region.blk_num_Y = from_ptr->taeregion.uwblknumy;
	to_ptr->region.offset_ratio_X = from_ptr->taeregion.uwoffsetratiox;
	to_ptr->region.offset_ratio_Y = from_ptr->taeregion.uwoffsetratioy;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld", ret);
	return ret;
}

static cmr_int aealtek_get_hwisp_cfg(struct aealtek_cxt *cxt_ptr, struct isp3a_ae_hw_cfg *out_ptr)
{
	cmr_int ret = ISP_ERROR;
	cmr_int lib_ret = 0;

	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct ae_get_param_t in_param;


	if (!cxt_ptr || !out_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, out_ptr);
		goto exit;
	}
	obj_ptr = &cxt_ptr->al_obj;

	if (cxt_ptr->lib_data.is_called_hwisp_cfg) {
		in_param.ae_get_param_type = AE_GET_ALHW3A_CONFIG;

		if (obj_ptr && obj_ptr->get_param)
			lib_ret = obj_ptr->get_param(&in_param, obj_ptr->ae);
		if (lib_ret)
			goto exit;
		cxt_ptr->lib_data.hwisp_cfg = in_param.para.alhw3a_aeconfig;
	} else {
		cxt_ptr->lib_data.is_called_hwisp_cfg = 1;
		ret = aealtek_get_def_hwisp_cfg(&cxt_ptr->lib_data.hwisp_cfg);
		if (ret)
			goto exit;
	}

	ret = aealtek_to_ae_hw_cfg(&cxt_ptr->lib_data.hwisp_cfg, out_ptr);
	if (ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_enable_debug_report(struct aealtek_cxt *cxt_ptr, cmr_int enable)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct ae_set_param_t in_param;
	struct ae_output_data_t *output_param_ptr = NULL;
	enum ae_set_param_type_t type = 0;
	struct ae_set_param_content_t *param_ct_ptr = NULL;


	if (!cxt_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}

	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;


	param_ct_ptr->ae_enableDebugLog = enable;
	type = AE_SET_PARAM_ENABLE_DEBUG_REPORT;
	in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret)
		goto exit;

	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_tuning_param(struct aealtek_cxt *cxt_ptr, void *tuning_param)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct ae_set_param_t in_param;
	struct ae_output_data_t *output_param_ptr = NULL;
	enum ae_set_param_type_t type = 0;
	struct ae_set_param_content_t *param_ct_ptr = NULL;


	if (!cxt_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}

	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;


	param_ct_ptr->ae_setting_data = tuning_param;
	type = AE_SET_PARAM_RELOAD_SETFILE;
	in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret)
		goto exit;

	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld,tuning_param:%p !!!", ret, lib_ret, tuning_param);
	return ret;
}

static cmr_int aealtek_save_aec_info(struct aealtek_cxt *cxt_ptr,
					   struct ae_ctrl_param_sensor_exposure *exp_ptr,
					   struct ae_ctrl_param_sensor_gain *gain_ptr,
					   struct ae_ctrl_param_sensor_exposure *exp_ptr_slv,
					   struct ae_ctrl_param_sensor_gain *gain_ptr_slv)
{
	cmr_int ret = ISP_ERROR;

	if (!cxt_ptr || !exp_ptr || !gain_ptr || !exp_ptr_slv || !gain_ptr_slv) {
		ISP_LOGE("param %p %p %p is NULL error!", cxt_ptr, exp_ptr, gain_ptr);
		ret = ISP_PARAM_NULL;
		goto exit;
	}
	cxt_ptr->pre_write_exp_data.exp_line = exp_ptr->exp_line;
	cxt_ptr->pre_write_exp_data.dummy = exp_ptr->dummy;
	cxt_ptr->pre_write_exp_data.gain = gain_ptr->gain;

	cxt_ptr->pre_write_exp_data_slv.exp_line = exp_ptr_slv->exp_line;
	cxt_ptr->pre_write_exp_data_slv.dummy = exp_ptr_slv->dummy;
	cxt_ptr->pre_write_exp_data_slv.gain = gain_ptr_slv->gain;

	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld", ret);
	return ret;
}

static cmr_int aealtek_write_to_sensor(struct aealtek_cxt *cxt_ptr,
				       struct ae_ctrl_param_sensor_exposure *exp_ptr,
				       struct ae_ctrl_param_sensor_gain *gain_ptr,
				       cmr_int force_write_sensor)
{
	cmr_int ret = ISP_ERROR;

	if (!cxt_ptr || !exp_ptr || !gain_ptr) {
		ISP_LOGE("param %p %p %p is NULL error!", cxt_ptr, exp_ptr, gain_ptr);
		ret = ISP_PARAM_NULL;
		goto exit;
	}

	if (cxt_ptr->init_in_param.ops_in.set_exposure) {
		if (0 != exp_ptr->exp_line && (force_write_sensor || cxt_ptr->pre_write_exp_data.exp_line != exp_ptr->exp_line
				|| cxt_ptr->pre_write_exp_data.dummy != exp_ptr->dummy)) {
			(*cxt_ptr->init_in_param.ops_in.set_exposure)(cxt_ptr->caller_handle, exp_ptr);
		}
	}

	if (cxt_ptr->init_in_param.ops_in.set_again) {
		if (0 != gain_ptr->gain && (force_write_sensor || cxt_ptr->pre_write_exp_data.gain != gain_ptr->gain)) {
			(*cxt_ptr->init_in_param.ops_in.set_again)(cxt_ptr->caller_handle, gain_ptr);
		}
	}

	cxt_ptr->pre_write_exp_data.exp_line = exp_ptr->exp_line;
	cxt_ptr->pre_write_exp_data.dummy = exp_ptr->dummy;
	cxt_ptr->pre_write_exp_data.gain = gain_ptr->gain;

	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld", ret);
	return ret;
}

static cmr_int aealtek_pre_calc_ae(struct aealtek_cxt *cxt_ptr,
				   struct ae_ctrl_param_sensor_exposure *sensor_exp,
				   struct ae_ctrl_param_sensor_gain *sensor_gain)
{
	cmr_int ret = ISP_ERROR;

	if (!cxt_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}
	if (cxt_ptr->is_refocus && !cxt_ptr->is_master) {
		ISP_LOGI("return when in slave sensor");
		return ISP_SUCCESS;
	}
	sensor_exp->exp_line = cxt_ptr->sensor_exp_data.write_exp.exp_line;
	sensor_exp->dummy = cxt_ptr->sensor_exp_data.write_exp.dummy;
	sensor_exp->size_index = cxt_ptr->sensor_exp_data.write_exp.size_index;
	sensor_gain->gain = cxt_ptr->sensor_exp_data.write_exp.gain * SENSOR_GAIN_BASE / LIB_GAIN_BASE;

	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld", ret);
	return ret;
}

static cmr_int aealtek_pre_to_sensor(struct aealtek_cxt *cxt_ptr, cmr_int is_sync_call, cmr_int force_write_sensor)
{
	cmr_int ret = ISP_ERROR;
	struct ae_ctrl_param_sensor_exposure sensor_exp;
	struct ae_ctrl_param_sensor_gain sensor_gain;

	if (!cxt_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}
	if (cxt_ptr->is_refocus && !cxt_ptr->is_master) {
		ISP_LOGI("return when in slave sensor");
		return ISP_SUCCESS;
	}
	if (0 == is_sync_call) {
		sensor_exp.exp_line = cxt_ptr->sensor_exp_data.write_exp.exp_line;
		sensor_exp.dummy = cxt_ptr->sensor_exp_data.write_exp.dummy;
		sensor_exp.size_index = cxt_ptr->sensor_exp_data.write_exp.size_index;
		sensor_gain.gain = cxt_ptr->sensor_exp_data.write_exp.gain * SENSOR_GAIN_BASE / LIB_GAIN_BASE;
	} else {
		sensor_exp.exp_line = cxt_ptr->sensor_exp_data.lib_exp.exp_line;
		sensor_exp.dummy = cxt_ptr->sensor_exp_data.lib_exp.dummy;
		sensor_exp.size_index = cxt_ptr->sensor_exp_data.lib_exp.size_index;
		sensor_gain.gain = cxt_ptr->sensor_exp_data.lib_exp.gain * SENSOR_GAIN_BASE / LIB_GAIN_BASE;
	}
	ret = aealtek_write_to_sensor(cxt_ptr, &sensor_exp, &sensor_gain, force_write_sensor);
	if (ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld", ret);
	return ret;
}

static void aealtek_reset_touch_param(struct aealtek_cxt *cxt_ptr)
{
	if (!cxt_ptr)
		return;

	cmr_bzero(&cxt_ptr->touch_param, sizeof(cxt_ptr->touch_param));
}

static cmr_int aealtek_set_boost(struct aealtek_cxt *cxt_ptr, cmr_u32 is_speed)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct ae_set_param_t in_param;
	struct ae_output_data_t *output_param_ptr = NULL;
	enum ae_set_param_type_t type = 0;
	struct ae_set_param_content_t *param_ct_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("param %p is NULL error!", cxt_ptr);
		goto exit;
	}

	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;

	if (is_speed)
		param_ct_ptr->converge_speedlv = AE_CONVERGE_FAST;
	else
		param_ct_ptr->converge_speedlv = AE_CONVERGE_NORMAL;

	type = AE_SET_PARAM_CONVERGE_SPD;
	in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_iso(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct ae_set_param_t in_param;
	struct ae_output_data_t *output_param_ptr = NULL;
	enum ae_set_param_type_t type = 0;
	struct ae_set_param_content_t *param_ct_ptr = NULL;

	UNUSED(out_ptr);
	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}
	cxt_ptr->nxt_status.ui_param.iso = in_ptr->iso.iso_mode;

	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;


	param_ct_ptr->isolevel = in_ptr->iso.iso_mode;
	ISP_LOGI("ISOLevel=%d",
		param_ct_ptr->isolevel);
	type = AE_SET_PARAM_ISO_MODE;
	in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	cxt_ptr->nxt_status.lib_ui_param.iso = param_ct_ptr->isolevel;
	cxt_ptr->update_list.is_iso = 1;

	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_fix_exposure_time(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;
	cmr_u32 exp_time = 0;

	UNUSED(out_ptr);
	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, in_ptr);
		goto exit;
	}
	ISP_LOGI("exp_time=%d", in_ptr->value);
	exp_time = in_ptr->value;

	cxt_ptr->sensor_exp_data.lib_exp.exp_line = SENSOR_EXP_BASE*exp_time/cxt_ptr->nxt_status.ui_param.work_info.resolution.line_time;
	cxt_ptr->sensor_exp_data.lib_exp.exp_time = exp_time;

	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld,!!!", ret);
	return ret;
}

static cmr_int aealtek_set_fix_sensitivity(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;
	cmr_u32 sensitivity = 0;

	UNUSED(out_ptr);
	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, in_ptr);
		goto exit;
	}
	ISP_LOGI("sensitivity=%d", in_ptr->value);
	sensitivity = in_ptr->value;

	cxt_ptr->sensor_exp_data.lib_exp.gain = sensitivity;

	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_set_fix_frame_duration(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;
	cmr_u32 frame_duration = 0;

	UNUSED(out_ptr);
	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, in_ptr);
		goto exit;
	}
	ISP_LOGI("frame_duration=%d", in_ptr->value);
	frame_duration = in_ptr->value;

	cxt_ptr->cur_status.min_frame_length = frame_duration / cxt_ptr->cur_status.ui_param.work_info.resolution.line_time;

	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_set_manaul_exptime(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct ae_set_param_t in_param;
	struct ae_output_data_t *output_param_ptr = NULL;
	enum ae_set_param_type_t type = 0;
	struct ae_set_param_content_t *param_ct_ptr = NULL;

	UNUSED(out_ptr);
	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}

	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;


	param_ct_ptr->manual_exptime = in_ptr->value;
	ISP_LOGI("manaul_exptime=%d", in_ptr->value);
	type = AE_SET_PARAM_MANUAL_EXPTIME;
	in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_manaul_gain(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct ae_set_param_t in_param;
	struct ae_output_data_t *output_param_ptr = NULL;
	enum ae_set_param_type_t type = 0;
	struct ae_set_param_content_t *param_ct_ptr = NULL;

	UNUSED(out_ptr);
	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}

	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;


	param_ct_ptr->manual_adgain = in_ptr->value;
	ISP_LOGI("manual_adgain=%d", in_ptr->value);
	type = AE_SET_PARAM_MANAUL_GAIN;
	in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_manaul_iso(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct ae_set_param_t in_param;
	struct ae_output_data_t *output_param_ptr = NULL;
	enum ae_set_param_type_t type = 0;
	struct ae_set_param_content_t *param_ct_ptr = NULL;

	UNUSED(out_ptr);
	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}

	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;


	param_ct_ptr->manual_iso = in_ptr->value;
	ISP_LOGI("manual_iso=%d", in_ptr->value);
	type = AE_SET_PARAM_MANUAL_ISO;
	in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_tuning_mode(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr)
{
	cmr_int ret = ISP_ERROR;
	cmr_u32 idx_num;

	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}
	idx_num = in_ptr->idx_num;
	ret = aealtek_set_tuning_param(cxt_ptr, cxt_ptr->tuning_param[idx_num]);
	if (ret) {
		ISP_LOGI("idx_num=%d ae_addr %p", idx_num, cxt_ptr->tuning_param[idx_num]);
	}
exit:
	return  ISP_SUCCESS;
}

static cmr_int aealtek_set_engineer_mode(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct ae_set_param_t in_param;
	struct ae_output_data_t *output_param_ptr = NULL;
	enum ae_set_param_type_t type = 0;
	struct ae_set_param_content_t *param_ct_ptr = NULL;

	UNUSED(out_ptr);
	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}

	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;

	param_ct_ptr->ae_engineer_mode_param.engineer_type = 65535;
	param_ct_ptr->ae_engineer_mode_param.ucengineermode = in_ptr->value;
	ISP_LOGI("engineer_mode=%d", in_ptr->value);
	type = AE_SET_PARAM_ENGINEER_MODE;
	in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_exp_comp(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct ae_set_param_t in_param;
	struct ae_output_data_t *output_param_ptr = NULL;
	enum ae_set_param_type_t type = 0;
	struct ae_set_param_content_t *param_ct_ptr = NULL;

	UNUSED(out_ptr);
	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, in_ptr);
		goto exit;
	}
	cxt_ptr->nxt_status.ui_param.ui_ev_level = in_ptr->exp_comp.level;

	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;

	param_ct_ptr->aeui_evcomp_level = in_ptr->exp_comp.level;
	type = AE_SET_PARAM_UI_EVCOMP_LEVEL;
	in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	ISP_LOGV("UI level = %ld, ev_value = %d", in_ptr->exp_comp.level, output_param_ptr->ui_EV_comp);
	cxt_ptr->nxt_status.lib_ui_param.ui_ev_level = param_ct_ptr->aeui_evcomp_level;
	cxt_ptr->update_list.is_ev = 1;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_bypass(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct ae_set_param_t in_param;
	struct ae_output_data_t *output_param_ptr = NULL;
	enum ae_set_param_type_t type = 0;
	struct ae_set_param_content_t *param_ct_ptr = NULL;

	UNUSED(out_ptr);
	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, in_ptr);
		goto exit;
	}
	ISP_LOGI("eb=%d", in_ptr->value);

	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;

	param_ct_ptr->ae_enable = in_ptr->value;
	type = AE_SET_PARAM_ENABLE;
	in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_flicker(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct ae_set_param_t in_param;
	struct ae_output_data_t *output_param_ptr = NULL;
	enum ae_set_param_type_t type = 0;
	struct ae_set_param_content_t *param_ct_ptr = NULL;
	enum ae_antiflicker_mode_t lib_flicker_mode = 0;
	enum antiflicker_ui_mode_t antiflicker_ui_mode = 0;

	UNUSED(out_ptr);
	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, in_ptr);
		goto exit;
	}
	cxt_ptr->nxt_status.ui_param.flicker = in_ptr->flicker.flicker_mode;

	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;

	aealtek_flicker_ui2lib(in_ptr->flicker.flicker_mode, &lib_flicker_mode);
	aealtek_antiflicker_mode_ui2lib(in_ptr->flicker.flicker_mode, &antiflicker_ui_mode);

	param_ct_ptr->ae_set_antibaning_mode = lib_flicker_mode;
	type = AE_SET_PARAM_ANTIFLICKERMODE;
	in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	cxt_ptr->nxt_status.lib_ui_param.flicker = (enum ae_antiflicker_mode_t)(antiflicker_ui_mode);
	cxt_ptr->update_list.is_flicker = 1;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_scene_mode(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct ae_set_param_t in_param;
	struct ae_output_data_t *output_param_ptr = NULL;
	enum ae_set_param_type_t type = 0;
	struct ae_set_param_content_t *param_ct_ptr = NULL;
	enum ae_scene_mode_t lib_scene_mode = 0;

	UNUSED(out_ptr);
	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}
	cxt_ptr->nxt_status.ui_param.scene = in_ptr->scene.scene_mode;
	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	#if 0
	param_ct_ptr = &in_param.set_param;
	switch (in_ptr->scene.scene_mode) {
	case AE_CTRL_SCENE_NORMAL:
		lib_scene_mode = SCENE_MODE_AUTO;
		break;
	case AE_CTRL_SCENE_NIGHT:
		lib_scene_mode = SCENE_MODE_NIGHT;
		break;
	case AE_CTRL_SCENE_SPORT:
		lib_scene_mode = SCENE_MODE_SPORT;
		break;
	case AE_CTRL_SCENE_PORTRAIT:
		lib_scene_mode = SCENE_MODE_PORTRAIT;
		break;
	case AE_CTRL_SCENE_LANDSCAPE:
		lib_scene_mode = SCENE_MODE_LANDSCAPE;
		break;
	default:
		ISP_LOGW("NOT defined ui scene %ld!", in_ptr->scene.scene_mode);
		break;
	}
	param_ct_ptr->ae_scene_mode = lib_scene_mode;
	type = AE_SET_PARAM_SCENE_MODE;
	in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	#endif
	lib_ret = al3awrapperae_setscenesetting(&in_ptr->scene.scene_info, obj_ptr, output_param_ptr, obj_ptr->ae);
	ISP_LOGI("scene %ld lib_scene_mode %d scene addr %p lib_ret=%ld",
		 in_ptr->scene.scene_mode, lib_scene_mode, in_ptr->scene.scene_info.puc_addr, lib_ret);
	if (lib_ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_fps(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct ae_set_param_t in_param;
	struct ae_output_data_t *output_param_ptr = NULL;
	enum ae_set_param_type_t type = 0;
	struct ae_set_param_content_t *param_ct_ptr = NULL;

	cmr_u32 max_fps = 0;
	cmr_u32 line_time = 0;

	UNUSED(out_ptr);
	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, in_ptr);
		goto exit;
	}
	if (cxt_ptr->cur_status.ui_param.work_info.sensor_fps.is_high_fps) {
		ISP_LOGI("high fps!");
		return ISP_SUCCESS;
	}
	cxt_ptr->nxt_status.ui_param.fps = in_ptr->range_fps;

	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;

	param_ct_ptr->min_fps = 100 * in_ptr->range_fps.min_fps;
	param_ct_ptr->max_fps = 100 * in_ptr->range_fps.max_fps;
	type = AE_SET_PARAM_FPS;
	in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret)
		goto exit;

	max_fps = cxt_ptr->nxt_status.ui_param.fps.max_fps;
	line_time = cxt_ptr->cur_status.ui_param.work_info.resolution.line_time;
	ISP_LOGI("linetime=%d,fps:%f,%f", line_time,
		 cxt_ptr->nxt_status.ui_param.fps.min_fps,
		 cxt_ptr->nxt_status.ui_param.fps.max_fps);

	ret = aealtek_set_min_frame_length(cxt_ptr, max_fps, line_time);
	if (ret)
		ISP_LOGW("warning set_min_frame ret=%ld !!!", ret);

	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_lib_metering_mode(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct ae_set_param_t in_param;
	struct ae_output_data_t *output_param_ptr = NULL;
	enum ae_set_param_type_t type = 0;
	struct ae_set_param_content_t *param_ct_ptr = NULL;
	enum ae_metering_mode_type_t ae_mode = 0;


	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, in_ptr);
		goto exit;
	}
	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;

	aealtek_weight_ui2lib(in_ptr->measure_lum.lum_mode, &ae_mode);
	param_ct_ptr->ae_metering_mode = ae_mode;

	type = AE_SET_PARAM_METERING_MODE;
	in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_measure_lum(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	UNUSED(out_ptr);
	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, in_ptr);
		goto exit;
	}

	if (cxt_ptr->nxt_status.ui_param.weight == in_ptr->measure_lum.lum_mode) {
		return ISP_SUCCESS;
	}
	cxt_ptr->nxt_status.ui_param.weight = in_ptr->measure_lum.lum_mode;

	ISP_LOGI("flash_enable:%d,touch_flag:%d,lum_mode:%ld", cxt_ptr->flash_param.enable
			, cxt_ptr->touch_param.touch_flag, in_ptr->measure_lum.lum_mode);


	if ((AE_CTRL_MEASURE_LUM_TOUCH != in_ptr->measure_lum.lum_mode)
		&& cxt_ptr->touch_param.touch_flag) {
		aealtek_reset_touch_param(cxt_ptr);
		ret = aealtek_reset_touch_ack(cxt_ptr);
		if (ret)
			goto exit;
		if (cxt_ptr->flash_param.enable) {
			cxt_ptr->flash_param.enable = 0;
			aealtek_set_lock(cxt_ptr, 0, __func__);
			ret = aealtek_set_flash_est(cxt_ptr, 1);
			if (ret)
				goto exit;
		}
	}
	ret = aealtek_set_lib_metering_mode(cxt_ptr, in_ptr);
	if (ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_set_stat_trim(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct ae_set_param_t in_param;
	struct ae_output_data_t output_param;
	enum ae_set_param_type_t type = 0;
	struct ae_set_param_content_t param_ct;

	UNUSED(out_ptr);
	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_reset_touch_ack(struct aealtek_cxt *cxt_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct ae_set_param_t in_param;
	struct ae_output_data_t *output_param_ptr = NULL;
	enum ae_set_param_type_t type = 0;
	struct ae_ctrl_param_in param_in;


	if (!cxt_ptr) {
		ISP_LOGE("param %p is NULL error!", cxt_ptr);
		goto exit;
	}
	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;

	type = AE_SET_PARAM_RESET_ROI_ACK;
	in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	param_in.touch_zone.zone.x = 0;
	param_in.touch_zone.zone.y = 0;
	param_in.touch_zone.zone.w = 0;
	param_in.touch_zone.zone.h = 0;
	ret = aealtek_set_lib_roi(cxt_ptr, &param_in, NULL);
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_lib_roi(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct ae_set_param_t in_param;
	struct ae_output_data_t *output_param_ptr = NULL;
	enum ae_set_param_type_t type = 0;
	struct rect_roi_config_t *roi_ptr = NULL;

	UNUSED(out_ptr);
	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, in_ptr);
		goto exit;
	}
	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	roi_ptr = &in_param.set_param.ae_set_roi_setting;

	roi_ptr->roi_count = 1; // 1  > AL_MAX_ROI_NUM ? AL_MAX_ROI_NUM:1;
	roi_ptr->roi[0].weight = 1;
	roi_ptr->roi[0].roi.left = in_ptr->touch_zone.zone.x;
	roi_ptr->roi[0].roi.top = in_ptr->touch_zone.zone.y;
	roi_ptr->roi[0].roi.w = in_ptr->touch_zone.zone.w;
	roi_ptr->roi[0].roi.h = in_ptr->touch_zone.zone.h;
	roi_ptr->ref_frame_height = 0;
	roi_ptr->ref_frame_width = 0;
	type = AE_SET_PARAM_ROI;
	in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_touch_zone(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;
	struct ae_ctrl_param_in temp_param;
	cmr_s32 end_x;
	cmr_s32 end_y;

	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, in_ptr);
		goto exit;
	}
	ISP_LOGI("x=%d,y=%d,w=%d,h=%d,touch_flag:%d", in_ptr->touch_zone.zone.x,
			in_ptr->touch_zone.zone.y,
			in_ptr->touch_zone.zone.w,
			in_ptr->touch_zone.zone.h,
			cxt_ptr->touch_param.touch_flag);

	if (0 == in_ptr->touch_zone.zone.x &&
	    0 == in_ptr->touch_zone.zone.y &&
	    0 == in_ptr->touch_zone.zone.w &&
	    0 == in_ptr->touch_zone.zone.h &&
	    1 == cxt_ptr->touch_param.touch_flag) {
		return ISP_SUCCESS;
	}

	end_x = in_ptr->touch_zone.zone.w + in_ptr->touch_zone.zone.x;
	end_y = in_ptr->touch_zone.zone.h + in_ptr->touch_zone.zone.y;
	if (in_ptr->touch_zone.zone.w <= 0
		|| in_ptr->touch_zone.zone.h <= 0
		|| end_x >= (cmr_s32)cxt_ptr->cur_status.ui_param.work_info.resolution.frame_size.w
		|| end_y >= (cmr_s32)cxt_ptr->cur_status.ui_param.work_info.resolution.frame_size.h
		|| end_x <= in_ptr->touch_zone.zone.x
		|| end_y <= in_ptr->touch_zone.zone.y) {
		if (cxt_ptr->touch_param.touch_flag) {
			aealtek_reset_touch_param(cxt_ptr);
			ret = aealtek_reset_touch_ack(cxt_ptr);
			if (ret)
				goto exit;
		}
		return ISP_SUCCESS;
	}
	aealtek_reset_touch_param(cxt_ptr);

	cxt_ptr->touch_param.touch_flag = 1;
	temp_param.measure_lum.lum_mode = AE_CTRL_MEASURE_LUM_TOUCH;
	ret = aealtek_set_measure_lum(cxt_ptr, &temp_param, NULL);
	if (ret) {
		ISP_LOGE("failed to set ae measure");
		goto exit;
	}
	ret = aealtek_set_lib_roi(cxt_ptr, in_ptr, out_ptr);
	if (ret) {
		ISP_LOGE("failed to set lib roi");
		goto exit;
	}
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_set_script_mode(struct aealtek_cxt *cxt_ptr, enum ae_script_mode_t script_mode)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct ae_set_param_t in_param;
	struct ae_output_data_t *output_param_ptr = NULL;
	enum ae_set_param_type_t type = 0;
	struct ae_set_param_content_t *param_ct_ptr = NULL;


	if (!cxt_ptr) {
		ISP_LOGE("param %p is NULL error!", cxt_ptr);
		goto exit;
	}
	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;

	type = AE_SET_PARAM_SET_SCRIPT_MODE;
	in_param.ae_set_param_type = type;
	param_ct_ptr->script_mode = script_mode;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_convergence_req(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	UNUSED(out_ptr);
	if (!cxt_ptr) {
		ISP_LOGE("param %p is NULL error!", cxt_ptr);
		goto exit;
	}
	if (cxt_ptr->touch_param.touch_flag) {
		cxt_ptr->touch_param.ctrl_convergence_req_flag = 1;

		ret = aealtek_set_boost(cxt_ptr, 1);
		if (ret)
			goto exit;
	} else if (in_ptr && 1 == in_ptr->value) {/**add for 3d calibration*/
		ISP_LOGE("set 3dcalibration enable");
		cxt_ptr->is_3dcalibration = 1;
	}
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_get_lib_init_expousre(struct aealtek_cxt *cxt_ptr, struct aealtek_lib_exposure_data *exposure_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct ae_get_param_t in_param;
	enum ae_get_param_type_t type = 0;


	if (!cxt_ptr || !exposure_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, exposure_ptr);
		goto exit;
	}
	obj_ptr = &cxt_ptr->al_obj;
	memset(&in_param, 0x00, sizeof(in_param));
	type = AE_GET_INIT_EXPOSURE_PARAM;
	in_param.ae_get_param_type = type;
	if (obj_ptr && obj_ptr->get_param)
		lib_ret = obj_ptr->get_param(&in_param, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	exposure_ptr->init.gain = in_param.para.ae_get_init_expo_param.ad_gain;
	exposure_ptr->init.exp_line = in_param.para.ae_get_init_expo_param.exp_linecount;
	exposure_ptr->init.exp_time = in_param.para.ae_get_init_expo_param.exp_time;
	exposure_ptr->init.iso = in_param.para.ae_get_expo_param.ISO;
	ISP_LOGI("ad_gain=%d, exp_line=%d, exp_time=%d, iso=%d",
			exposure_ptr->init.gain,
			exposure_ptr->init.exp_line,
			exposure_ptr->init.exp_time,
			exposure_ptr->init.iso);

	ISP_LOGI("init_expo ad_gain=%d, exp_line=%d, exp_time=%d, iso=%d",
		in_param.para.ae_get_expo_param.init_expo.ad_gain,
		in_param.para.ae_get_expo_param.init_expo.exp_linecount,
		in_param.para.ae_get_expo_param.init_expo.exp_time,
		in_param.para.ae_get_expo_param.init_expo.ISO);
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_get_iq_info(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_iq_info *iq_info_ptr)
{
	cmr_int ret = ISP_ERROR;
	cmr_int lib_ret = 0;
	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct ae_get_param_t in_param;
	enum ae_get_param_type_t type = 0;

	if (!cxt_ptr) {
		ISP_LOGE("param %p is NULL error!", cxt_ptr);
		goto exit;
	}
	obj_ptr = &cxt_ptr->al_obj;
	memset(&in_param, 0x00, sizeof(in_param));
	type = AE_GET_IQINFO_TO_ISP;
	in_param.ae_get_param_type = type;
	if (obj_ptr && obj_ptr->get_param)
		lib_ret = obj_ptr->get_param(&in_param, obj_ptr->ae);
	if (lib_ret)
		goto exit;

	memcpy(iq_info_ptr, &in_param.para.ae_iq_info, sizeof(struct ae_ctrl_iq_info));

	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_get_lib_expousre(struct aealtek_cxt *cxt_ptr, enum aealtek_work_mode mode, struct aealtek_lib_exposure_data *exposure_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct ae_get_param_t in_param;
	enum ae_get_param_type_t type = 0;

	cmr_u32 i = 0;


	if (!cxt_ptr || !exposure_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, exposure_ptr);
		goto exit;
	}
	obj_ptr = &cxt_ptr->al_obj;

	type = AE_GET_EXPOSURE_PARAM;
	in_param.ae_get_param_type = type;
	if (obj_ptr && obj_ptr->get_param)
		lib_ret = obj_ptr->get_param(&in_param, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	switch (mode) {
	case AEALTEK_PREVIEW:
		exposure_ptr->preview.gain = in_param.para.ae_get_expo_param.cur_expo.ad_gain;
		exposure_ptr->preview.exp_line = in_param.para.ae_get_expo_param.cur_expo.exp_linecount;
		exposure_ptr->preview.exp_time = in_param.para.ae_get_expo_param.cur_expo.exp_time;
		exposure_ptr->preview.iso = in_param.para.ae_get_expo_param.cur_expo.ISO;
		ISP_LOGI("preview ad_gain=%d, exp_line=%d, exp_time=%d",
				exposure_ptr->preview.gain,
				exposure_ptr->preview.exp_line,
				exposure_ptr->preview.exp_time);
		break;
	case AEALTEK_SNAPSHOT:
		exposure_ptr->snapshot.gain = in_param.para.ae_get_expo_param.still_expo.ad_gain;
		exposure_ptr->snapshot.exp_line = in_param.para.ae_get_expo_param.still_expo.exp_linecount;
		exposure_ptr->snapshot.exp_time = in_param.para.ae_get_expo_param.still_expo.exp_time;
		exposure_ptr->snapshot.iso = in_param.para.ae_get_expo_param.still_expo.ISO;
		ISP_LOGI("snapshot ad_gain=%d, exp_line=%d, exp_time=%d",
				exposure_ptr->snapshot.gain,
				exposure_ptr->snapshot.exp_line,
				exposure_ptr->snapshot.exp_time);
		break;
	case AEALTEK_BRACKET:
		exposure_ptr->valid_exp_num = in_param.para.ae_get_expo_param.valid_exp_num;

		for (i = 0; i < exposure_ptr->valid_exp_num; ++i) {
			exposure_ptr->bracket_exp[i].gain = in_param.para.ae_get_expo_param.bracket_expo[i].ad_gain;
			exposure_ptr->bracket_exp[i].exp_line = in_param.para.ae_get_expo_param.bracket_expo[i].exp_linecount;
			exposure_ptr->bracket_exp[i].exp_time = in_param.para.ae_get_expo_param.bracket_expo[i].exp_time;
			exposure_ptr->bracket_exp[i].iso = in_param.para.ae_get_expo_param.bracket_expo[i].ISO;
			exposure_ptr->bracket_exp[i].size_index = cxt_ptr->nxt_status.ui_param.work_info.resolution.sensor_size_index;
		}
		break;
	default:
		goto exit;
		break;
	}
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_init_lib_setting(struct aealtek_cxt *cxt_ptr, struct ae_set_parameter_init_t *init_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct ae_set_param_t in_param;
	struct ae_output_data_t *output_param_ptr = NULL;
	enum ae_set_param_type_t type = 0;
	struct ae_set_param_content_t *param_ct_ptr = NULL;


	if (!cxt_ptr || !init_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, init_ptr);
		goto exit;
	}
	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;

	type = AE_SET_PARAM_INIT_SETTING;
	in_param.ae_set_param_type = type;
	param_ct_ptr->ae_initial_setting = *init_ptr;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_first_work(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	struct ae_set_parameter_init_t param_init;
	struct aealtek_lib_exposure_data ae_exposure;
	cmr_u32 max_fps = 0;
	cmr_u32 line_time = 0;

	UNUSED(in_ptr);
	UNUSED(out_ptr);
	if (!cxt_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}

	ret = aealtek_convert_ui2initlib(cxt_ptr, &cxt_ptr->nxt_status, &param_init);
	if (ret)
		goto exit;

	ret = aealtek_init_lib_setting(cxt_ptr, &param_init);
	if (ret)
		goto exit;

	ret = aealtek_get_lib_init_expousre(cxt_ptr, &ae_exposure);
	if (ret)
		goto exit;

	ret = aealtek_convert_lib_exposure2outdata(cxt_ptr, &ae_exposure.init, &cxt_ptr->lib_data.output_data);
	if (ret)
		goto exit;

	ret = aealtek_lib_exposure2sensor(cxt_ptr, &cxt_ptr->lib_data.output_data, &cxt_ptr->sensor_exp_data.lib_exp);
	if (ret)
		goto exit;

	max_fps = cxt_ptr->nxt_status.ui_param.work_info.resolution.max_fps;
	line_time = cxt_ptr->nxt_status.ui_param.work_info.resolution.line_time;
	ret = aealtek_set_min_frame_length(cxt_ptr, max_fps, line_time);
	if (ret)
		ISP_LOGW("warning set_min_frame ret=%ld !!!", ret);

	ret = aealtek_set_dummy(cxt_ptr, &cxt_ptr->sensor_exp_data.lib_exp);
	if (ret)
		ISP_LOGW("warning set_dummy ret=%ld !!!", ret);

	cxt_ptr->sensor_exp_data.write_exp = cxt_ptr->sensor_exp_data.lib_exp;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld", ret);
	return ret;
}

static cmr_int aealtek_work_preview(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_work *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;
	cmr_int lib_ret = 0;
	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct ae_set_param_t set_in_param;
	struct ae_output_data_t *output_data_ptr = NULL;
	enum ae_set_param_type_t type = 0;
	struct ae_set_param_content_t *param_ct_ptr;
	struct ae_sensor_info_t *preview_sensor_ptr = NULL;
	struct aealtek_lib_exposure_data ae_exposure;

	cmr_u32 max_fps = 0;
	cmr_u32 line_time = 0;

	UNUSED(out_ptr);
	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, in_ptr);
		goto exit;
	}

	obj_ptr = &cxt_ptr->al_obj;
	output_data_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &set_in_param.set_param;


	max_fps = in_ptr->resolution.max_fps;
	line_time = in_ptr->resolution.line_time;
	ret = aealtek_set_min_frame_length(cxt_ptr, max_fps, line_time);
	if (ret)
		ISP_LOGW("warning set_min_frame ret=%ld !!!", ret);

	/* preview_sensor_info */
	preview_sensor_ptr = &param_ct_ptr->normal_sensor_info;
	ret = aealtek_sensor_info_ui2lib(cxt_ptr, &in_ptr->resolution, preview_sensor_ptr);
	if (ret)
		goto exit;
	ISP_LOGI("param min_fps=%d lib min_fps=%d", in_ptr->resolution.min_fps,
			set_in_param.set_param.normal_sensor_info.min_fps);
	type = AE_SET_PARAM_SENSOR_INFO;
	set_in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&set_in_param, output_data_ptr, obj_ptr->ae);
	if (lib_ret) {
		ISP_LOGE("lib set sensor info lib_ret=%ld is error!", lib_ret);
		ret = ISP_ERROR;
		goto exit;
	}

	if (cxt_ptr->is_refocus && cxt_ptr->is_master) {
		/* slv_preview_sensor_info */
		cxt_ptr->nxt_status.ui_param.work_info_slv.resolution.min_fps = in_ptr->resolution.min_fps;

		preview_sensor_ptr = &param_ct_ptr->slave_sensor_info;
		ret = aealtek_sensor_info_ui2lib_slv(cxt_ptr,
					&cxt_ptr->nxt_status.ui_param.work_info_slv.resolution,
					preview_sensor_ptr);
		if (ret)
			goto exit;
		ISP_LOGI("slv param min_fps=%d lib min_fps=%d",
			 cxt_ptr->nxt_status.ui_param.work_info_slv.resolution.min_fps,
			 set_in_param.set_param.slave_sensor_info.min_fps);
		type = AE_SET_PARAM_SENSOR_INFO_SLV;
		set_in_param.ae_set_param_type = type;
		if (obj_ptr && obj_ptr->set_param)
			lib_ret = obj_ptr->set_param(&set_in_param, output_data_ptr, obj_ptr->ae);
		if (lib_ret) {
			ISP_LOGE("slv lib set sensor info lib_ret=%ld is error!", lib_ret);
			ret = ISP_ERROR;
			goto exit;
		}
	}

	/* SET_PARAM_REGEN_EXP_INFO */
	type = AE_SET_PARAM_REGEN_EXP_INFO;
	set_in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&set_in_param, output_data_ptr, obj_ptr->ae);
	if (lib_ret) {
		ISP_LOGE("lib set regen lib_ret=%ld is error!", lib_ret);
		ret = ISP_ERROR;
		goto exit;
	}

	if (cxt_ptr->is_processed == 1) {
		ret = aealtek_get_lib_expousre(cxt_ptr, AEALTEK_PREVIEW, &ae_exposure);
		if (ret)
			goto exit;
		ret = aealtek_convert_lib_exposure2outdata(cxt_ptr, &ae_exposure.preview, &cxt_ptr->lib_data.output_data);
		if (ret)
			goto exit;
		ret = aealtek_lib_exposure2sensor(cxt_ptr, &cxt_ptr->lib_data.output_data, &cxt_ptr->sensor_exp_data.lib_exp);
		if (ret)
			goto exit;
		ret = aealtek_set_dummy(cxt_ptr, &cxt_ptr->sensor_exp_data.lib_exp);
		if (ret)
			ISP_LOGW("warning set_dummy ret=%ld !!!", ret);
		cxt_ptr->sensor_exp_data.write_exp = cxt_ptr->sensor_exp_data.lib_exp;
	}
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_capture_mode(struct aealtek_cxt *cxt_ptr, enum isp_capture_mode cap_mode)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct ae_set_param_t in_param;
	struct ae_output_data_t *output_param_ptr = NULL;
	enum ae_set_param_type_t type = 0;
	struct ae_set_param_content_t *param_ct_ptr = NULL;
	enum ae_capture_mode_t lib_cap_mode = 0;

	if (!cxt_ptr) {
		ISP_LOGE("param %p is NULL error!", cxt_ptr);
		goto exit;
	}
	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;

	switch (cap_mode) {
	case ISP_CAP_MODE_AUTO:
		lib_cap_mode = CAPTURE_MODE_AUTO;
		break;
	case ISP_CAP_MODE_ZSL:
		lib_cap_mode = CAPTURE_MODE_ZSL;
		break;
	case ISP_CAP_MODE_HDR:
		lib_cap_mode = CAPTURE_MODE_HDR;
		break;
	case ISP_CAP_MODE_VIDEO:
		lib_cap_mode = CAPTURE_MODE_VIDEO;
		break;
	case ISP_CAP_MODE_VIDEO_HDR:
		lib_cap_mode = CAPTURE_MODE_VIDEO_HDR;
		break;
	case ISP_CAP_MODE_BRACKET:
		lib_cap_mode = CAPTURE_MODE_BRACKET;
		break;
	default:
		ISP_LOGW("NOT defined cap mode %ld", (cmr_int)cap_mode);
		break;
	}

	ISP_LOGI("lib_cap_mode=%d", lib_cap_mode);
	type = AE_SET_PARAM_CAPTURE_MODE;
	in_param.ae_set_param_type = type;
	param_ct_ptr->capture_mode = lib_cap_mode;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_capture_hdr(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct ae_set_param_t set_in_param;
	struct ae_output_data_t *output_data_ptr = NULL;
	enum ae_set_param_type_t type = 0;
	struct ae_set_param_content_t *param_ct_ptr;
	struct ae_sensor_info_t *cap_sensor_ptr = NULL;

	struct aealtek_lib_exposure_data ae_exposure;

	UNUSED(out_ptr);
	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, in_ptr);
		goto exit;
	}
	cxt_ptr->nxt_status.is_hdr_status = 1;


	ret = aealtek_set_capture_mode(cxt_ptr, ISP_CAP_MODE_BRACKET);
	if (ret)
		goto exit;

	obj_ptr = &cxt_ptr->al_obj;
	output_data_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &set_in_param.set_param;

	/* get 3 exposure time & gain */
	cmr_bzero(param_ct_ptr->ae_bracket_param.bracket_evComp, sizeof(param_ct_ptr->ae_bracket_param.bracket_evComp));
	set_in_param.bforceupdateflg   = 1;
	param_ct_ptr->capture_mode = CAPTURE_MODE_BRACKET;
	param_ct_ptr->ae_bracket_param.valid_exp_num = 3;
	param_ct_ptr->ae_bracket_param.bracket_evComp[0] = -2000;
	param_ct_ptr->ae_bracket_param.bracket_evComp[1] =  0;
	param_ct_ptr->ae_bracket_param.bracket_evComp[2] = 2000;
	type = AE_SET_PARAM_BRACKET_MODE;
	set_in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&set_in_param, output_data_ptr, obj_ptr->ae);
	if (lib_ret) {
		ISP_LOGE("lib set hdr lib_ret=%ld is error!", lib_ret);
		ret = ISP_ERROR;
		goto exit;
	}

	/* capture_sensor_info */
	cap_sensor_ptr = &param_ct_ptr->capture_sensor_info;
	ret = aealtek_sensor_info_ui2lib(cxt_ptr, &in_ptr->work_param.resolution, cap_sensor_ptr);
	if (ret)
		goto exit;

	type = AE_SET_PARAM_CAPTURE_SENSOR_INFO;
	set_in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&set_in_param, output_data_ptr, obj_ptr->ae);
	if (lib_ret) {
		ISP_LOGE("lib set sensor info lib_ret=%ld is error!", lib_ret);
		ret = ISP_ERROR;
		goto exit;
	}

	/* SET_PARAM_REGEN_EXP_INFO */
	type = AE_SET_PARAM_REGEN_EXP_INFO;
	set_in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&set_in_param, output_data_ptr, obj_ptr->ae);
	if (lib_ret) {
		ISP_LOGE("lib set regen lib_ret=%ld is error!", lib_ret);
		ret = ISP_ERROR;
		goto exit;
	}

	ret = aealtek_get_lib_expousre(cxt_ptr, AEALTEK_BRACKET, &ae_exposure);
	if (ret)
		goto exit;
	cxt_ptr->lib_data.exposure_array = ae_exposure;
	ret = aealtek_convert_lib_exposure2outdata(cxt_ptr, &cxt_ptr->lib_data.exposure_array.bracket_exp[0], &cxt_ptr->lib_data.output_data);
	if (ret)
		goto exit;

	ISP_LOGI("hdr exp_array:%d,%d,%d", cxt_ptr->lib_data.exposure_array.bracket_exp[0].exp_line,
		 cxt_ptr->lib_data.exposure_array.bracket_exp[1].exp_line,
		 cxt_ptr->lib_data.exposure_array.bracket_exp[2].exp_line);

	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_capture_normal(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct ae_set_param_t set_in_param;
	struct ae_output_data_t *output_data_ptr = NULL;
	enum ae_set_param_type_t type = 0;
	struct ae_set_param_content_t *param_ct_ptr;
	struct ae_sensor_info_t *cap_sensor_ptr = NULL;

	struct aealtek_lib_exposure_data ae_exposure;

	UNUSED(out_ptr);
	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, in_ptr);
		goto exit;
	}

	ret = aealtek_set_capture_mode(cxt_ptr, ISP_CAP_MODE_AUTO);
	if (ret)
		goto exit;

	obj_ptr = &cxt_ptr->al_obj;
	output_data_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &set_in_param.set_param;

	/* capture_sensor_info */
	cap_sensor_ptr = &param_ct_ptr->capture_sensor_info;
	ret = aealtek_sensor_info_ui2lib(cxt_ptr, &in_ptr->work_param.resolution, cap_sensor_ptr);
	if (ret)
		goto exit;

	type = AE_SET_PARAM_CAPTURE_SENSOR_INFO;
	set_in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&set_in_param, output_data_ptr, obj_ptr->ae);
	if (lib_ret) {
		ISP_LOGE("lib set sensor info lib_ret=%ld is error!", lib_ret);
		ret = ISP_ERROR;
		goto exit;
	}

	if (cxt_ptr->is_refocus && cxt_ptr->is_master) {
		/* slv capture_sensor_info */
		cap_sensor_ptr = &param_ct_ptr->slave_capture_sensor_info;
		ret = aealtek_sensor_info_ui2lib_slv(cxt_ptr,
					&cxt_ptr->nxt_status.ui_param.work_info_slv.resolution,
					cap_sensor_ptr);
		if (ret)
			goto exit;

		type = AE_SET_PARAM_CAPTURE_SENSOR_INFO_SLV;
		set_in_param.ae_set_param_type = type;
		if (obj_ptr && obj_ptr->set_param)
			lib_ret = obj_ptr->set_param(&set_in_param, output_data_ptr, obj_ptr->ae);
		if (lib_ret) {
			ISP_LOGE("slv lib set sensor info lib_ret=%ld is error!", lib_ret);
			ret = ISP_ERROR;
			goto exit;
		}
	}

	/* SET_PARAM_REGEN_EXP_INFO */
	type = AE_SET_PARAM_REGEN_EXP_INFO;
	set_in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&set_in_param, output_data_ptr, obj_ptr->ae);
	if (lib_ret) {
		ISP_LOGE("lib set regen lib_ret=%ld is error!", lib_ret);
		ret = ISP_ERROR;
		goto exit;
	}

	ret = aealtek_get_lib_expousre(cxt_ptr, AEALTEK_SNAPSHOT, &ae_exposure);
	if (ret)
		goto exit;
	cxt_ptr->lib_data.exposure_array = ae_exposure;
	if (cxt_ptr->main_flash_status == 0) {
		ret = aealtek_convert_lib_exposure2outdata(cxt_ptr, &cxt_ptr->lib_data.exposure_array.snapshot, &cxt_ptr->lib_data.output_data);
		if (ret)
			goto exit;
		cxt_ptr->sensor_exp_data.actual_exp = cxt_ptr->lib_data.exposure_array.snapshot;
	}
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_work_capture(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;
	enum isp_capture_mode cap_mode = 0;


	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}
	cap_mode = in_ptr->work_param.capture_mode;
	ISP_LOGI("cap_mode=%d", cap_mode);
	ret = aealtek_set_capture_mode(cxt_ptr, cap_mode);
	if (ret)
		goto exit;

	switch (cap_mode) {
	case ISP_CAP_MODE_HDR:
		ret = aealtek_capture_hdr(cxt_ptr, in_ptr, out_ptr);
		break;
	default:
		ret = aealtek_capture_normal(cxt_ptr, in_ptr, out_ptr);
		break;
	}
	if (ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_set_capture_mode_auto(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr)
{
	cmr_int ret = ISP_ERROR;
	enum isp_capture_mode cap_mode = 0;

	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}

	cap_mode = in_ptr->work_param.capture_mode;
	ret = aealtek_set_capture_mode(cxt_ptr, cap_mode);

	if (ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_work_video(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct ae_set_param_t in_param;
	struct ae_output_data_t output_param;
	enum ae_set_param_type_t type = 0;
	struct ae_set_param_content_t param_ct;

	enum isp3a_work_mode work_mode = 0;

	UNUSED(out_ptr);
	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_set_sync_mode(struct aealtek_cxt *cxt_ptr, cmr_u8 is_sync_mode)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct ae_set_param_t in_param;
	struct ae_output_data_t *output_param_ptr = NULL;
	enum ae_set_param_type_t type = 0;
	struct ae_set_param_content_t *param_ct_ptr = NULL;


	if (!cxt_ptr) {
		ISP_LOGE("param %p is NULL error!", cxt_ptr);
		goto exit;
	}
	ISP_LOGI("is_sync_mode=%d", is_sync_mode);

	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;

	param_ct_ptr->us_dual_sync_mode = is_sync_mode;
	type = AE_SET_PARAM_SYNC_MODE;
	in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_work_mode(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct ae_set_param_t in_param;
	struct ae_output_data_t output_param;
	enum ae_set_param_type_t type = 0;
	struct ae_set_param_content_t param_ct;
	enum isp3a_work_mode work_mode = 0;
	struct seq_init_in seq_in;
	cmr_int force_write_sensor = 0;


	if (!cxt_ptr || !in_ptr || !out_ptr) {
		ISP_LOGE("param %p %p %p is NULL error!", cxt_ptr, in_ptr, out_ptr);
		goto exit;
	}

	ISP_LOGI("frame size:%dx%d, line_time=%d", in_ptr->work_param.resolution.frame_size.w,
			in_ptr->work_param.resolution.frame_size.h,
			in_ptr->work_param.resolution.line_time);

	cxt_ptr->nxt_status.ui_param.work_info = in_ptr->work_param;

	seq_in.capture_skip_num = cxt_ptr->init_in_param.sensor_static_info.capture_skip_num;
	seq_in.preview_skip_num = cxt_ptr->init_in_param.sensor_static_info.preview_skip_num;
	seq_in.exp_valid_num = cxt_ptr->init_in_param.sensor_static_info.exposure_valid_num;
	seq_in.gain_valid_num = cxt_ptr->init_in_param.sensor_static_info.gain_valid_num;
	if (in_ptr->work_param.sensor_fps.is_high_fps) {
		cxt_ptr->nxt_status.ui_param.work_info.resolution.max_fps = in_ptr->work_param.sensor_fps.max_fps;
		cxt_ptr->nxt_status.ui_param.work_info.resolution.min_fps = in_ptr->work_param.sensor_fps.max_fps;

		if (in_ptr->work_param.sensor_fps.high_fps_skip_num >= seq_in.exp_valid_num)
			seq_in.exp_valid_num = 0;
		else
			seq_in.exp_valid_num = seq_in.exp_valid_num - in_ptr->work_param.sensor_fps.high_fps_skip_num;
		if (in_ptr->work_param.sensor_fps.high_fps_skip_num >= seq_in.gain_valid_num)
			seq_in.gain_valid_num = 0;
		else
			seq_in.gain_valid_num = seq_in.gain_valid_num - in_ptr->work_param.sensor_fps.high_fps_skip_num;

		ISP_LOGI("cap skip num=%d, preview skip num=%d, exp_valid_num=%d,gain_valid_num=%d",
				seq_in.capture_skip_num, seq_in.preview_skip_num,
				seq_in.exp_valid_num, seq_in.gain_valid_num);

		ret = aealtek_set_boost(cxt_ptr, 0);
	}

	seq_deinit(cxt_ptr->seq_handle);
	seq_in.idx_start_from = 1;
	ret = seq_init(10, &seq_in, &cxt_ptr->seq_handle);
	if (ret || NULL == cxt_ptr->seq_handle)
		goto exit;

	//cxt_ptr->is_refocus = in_ptr->work_param.is_refocus;
	if (cxt_ptr->is_refocus && cxt_ptr->is_master)
		ret = aealtek_set_sync_mode(cxt_ptr, 1);

	cxt_ptr->zsl_mode = in_ptr->work_param.capture_mode;
	work_mode = in_ptr->work_param.work_mode;
	ISP_LOGI("work_mode=%ld", in_ptr->work_param.work_mode);
	switch (work_mode) {
	case ISP3A_WORK_MODE_PREVIEW:
		if (1 == cxt_ptr->nxt_status.is_hdr_status)
			aealtek_set_hdr_lock(cxt_ptr, 0, __func__);
		cxt_ptr->nxt_status.is_hdr_status = 0;
		if (in_ptr->work_param.capture_mode != ISP_CAP_MODE_AUTO &&
		    in_ptr->work_param.capture_mode != ISP_CAP_MODE_BURST) {
			in_ptr->work_param.capture_mode = ISP_CAP_MODE_AUTO;
			ret = aealtek_set_capture_mode_auto(cxt_ptr, in_ptr);
		}
		ret = aealtek_work_preview(cxt_ptr, &cxt_ptr->nxt_status.ui_param.work_info, out_ptr);
		force_write_sensor = 1;
		break;
	case ISP3A_WORK_MODE_CAPTURE:
		ret = aealtek_work_capture(cxt_ptr, in_ptr, out_ptr);
		force_write_sensor = 1;
		break;
	case ISP3A_WORK_MODE_VIDEO:
		cxt_ptr->nxt_status.is_hdr_status = 0;
		//ret = aealtek_work_video(cxt_ptr, in_ptr, out_ptr);
		ret = 0;
		break;
	default:
		ISP_LOGE("not support work mode %d", work_mode);
		break;
	}

	ret = aealtek_get_hwisp_cfg(cxt_ptr, &out_ptr->proc_out.hw_cfg);
	if (ret)
		goto exit;

	ret = aealtek_lib_exposure2sensor(cxt_ptr, &cxt_ptr->lib_data.output_data, &cxt_ptr->sensor_exp_data.lib_exp);
	if (ret)
		goto exit;

	ret = aealtek_lib_to_out_info(cxt_ptr, &cxt_ptr->lib_data.output_data, &out_ptr->proc_out.ae_info);
	if (ret)
		goto exit;
	if (cxt_ptr->tuning_info.manual_ae_on && ISP3A_WORK_MODE_PREVIEW == work_mode) {
		if (TUNING_MODE_USER_DEF != cxt_ptr->tuning_info.tuning_mode) {
			cxt_ptr->sensor_exp_data.lib_exp.exp_time = cxt_ptr->tuning_info.exposure[cxt_ptr->tuning_info.num];
			cxt_ptr->sensor_exp_data.lib_exp.exp_line =
				SENSOR_EXP_BASE*cxt_ptr->tuning_info.exposure[cxt_ptr->tuning_info.num]/cxt_ptr->nxt_status.ui_param.work_info.resolution.line_time;
			cxt_ptr->sensor_exp_data.lib_exp.gain = cxt_ptr->tuning_info.gain[cxt_ptr->tuning_info.num];
			ISP_LOGI("get num:%d tuning exp_gain:%d,%d", cxt_ptr->tuning_info.num,
				 cxt_ptr->sensor_exp_data.lib_exp.exp_line,
				 cxt_ptr->sensor_exp_data.lib_exp.gain);
			cxt_ptr->tuning_info.num++;
		}

	}

	if (ISP3A_WORK_MODE_CAPTURE != work_mode || !cxt_ptr->nxt_status.is_hdr_status) {
		if (cxt_ptr->tuning_info.manual_ae_on && TUNING_MODE_USER_DEF == cxt_ptr->tuning_info.tuning_mode) {
			ISP_LOGI("dvt mode");
		} else {
			ret = aealtek_pre_to_sensor(cxt_ptr, 1, force_write_sensor);
			if (ret)
				goto exit;
		}
	}

	++cxt_ptr->work_cnt;
	cxt_ptr->cur_status.ui_param.work_info = cxt_ptr->nxt_status.ui_param.work_info;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_set_lib_lock(struct aealtek_cxt *cxt_ptr, cmr_int is_lock)
{
	cmr_int ret = ISP_ERROR;
	cmr_int lib_ret = 0;
	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct ae_set_param_t in_param;
	struct ae_output_data_t *output_param_ptr = NULL;
	enum ae_set_param_type_t type = 0;
	struct ae_set_param_content_t *param_ct_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}

	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;

	param_ct_ptr->ae_lock = is_lock;

	ISP_LOGI("is_lock=%d", param_ct_ptr->ae_lock);
	type = AE_SET_PARAM_LOCKAE;
	in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_lock(struct aealtek_cxt *cxt_ptr, cmr_int is_lock, const char *cb_func)
{
	cmr_int ret = ISP_ERROR;

	if (!cxt_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}
	ISP_LOGI("lock_cnt=%d, is_lock=%ld cb: %s", cxt_ptr->lock_cnt, is_lock, cb_func);

	if (is_lock) {
		cxt_ptr->lock_cnt++;
	} else {
		if (cxt_ptr->lock_cnt > 0)
			cxt_ptr->lock_cnt--;
	}

	if (cxt_ptr->lock_cnt > 0)
		is_lock = 1;
	else
		is_lock = 0;

	ret = aealtek_set_lib_lock(cxt_ptr, is_lock);
	if (ret)
		goto exit;

	return ISP_SUCCESS;
exit:
	ISP_LOGI("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_set_hdr_lock(struct aealtek_cxt *cxt_ptr, cmr_int is_lock, const char *cb_func)
{
	cmr_int ret = ISP_ERROR;

	if (!cxt_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}
	ISP_LOGI("hdr_lock_cnt=%d, is_lock=%ld cb: %s", cxt_ptr->hdr_lock_cnt, is_lock, cb_func);

	if (is_lock && 0 == cxt_ptr->hdr_lock_cnt) {
		ret = aealtek_set_lib_lock(cxt_ptr, is_lock);
		cxt_ptr->hdr_lock_cnt++;
	} else if (0 == is_lock && cxt_ptr->hdr_lock_cnt > 0) {
		ret = aealtek_set_lib_lock(cxt_ptr, is_lock);
		cxt_ptr->hdr_lock_cnt--;
	} else {
		return ISP_SUCCESS;
	}

	if (ret)
		goto exit;

	return ISP_SUCCESS;
exit:
	ISP_LOGI("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_set_flash_est(struct aealtek_cxt *cxt_ptr, cmr_u32 is_reset)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct ae_set_param_t in_param;
	struct ae_output_data_t *output_param_ptr = NULL;
	enum ae_set_param_type_t type = 0;
	struct ae_set_param_content_t *param_ct_ptr = NULL;
	enum ae_flash_st_t lib_led_st = 0;


	if (!cxt_ptr) {
		ISP_LOGE("param %p is NULL error!", cxt_ptr);
		goto exit;
	}

	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;

	if (is_reset)
		type = AE_SET_PARAM_RESET_FLASH_EST;
	else
		type = AE_SET_PARAM_FLASHAE_EST;
	in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_hw_flash_status(struct aealtek_cxt *cxt_ptr, cmr_int is_lighting)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct ae_set_param_t in_param;
	struct ae_output_data_t *output_param_ptr = NULL;
	enum ae_set_param_type_t type = 0;
	struct ae_set_param_content_t *param_ct_ptr = NULL;
	enum ae_flash_st_t lib_led_st = 0;


	if (!cxt_ptr) {
		ISP_LOGE("param %p is NULL error!", cxt_ptr);
		goto exit;
	}

	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;

	if (is_lighting)
		lib_led_st = AE_FLASH_ON;
	else
		lib_led_st = AE_FLASH_OFF;
	param_ct_ptr->ae_flash_st = lib_led_st;
	type = AE_SET_PARAM_FLASH_ST;
	in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_lib_ui_flash_mode(struct aealtek_cxt *cxt_ptr, enum isp_ui_flash_status ui_flash_status)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct ae_set_param_t in_param;
	struct ae_output_data_t *output_param_ptr = NULL;
	enum ae_set_param_type_t type = 0;
	struct ae_set_param_content_t *param_ct_ptr = NULL;
	enum al3a_fe_ui_flash_mode lib_flash_mode = 0;


	if (!cxt_ptr) {
		ISP_LOGE("param %p is NULL error!", cxt_ptr);
		goto exit;
	}

	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;

	switch (ui_flash_status) {
	case ISP_UI_FLASH_STATUS_OFF:
		lib_flash_mode = AL3A_UI_FLASH_OFF;
		break;
	case ISP_UI_FLASH_STATUS_AUTO:
		lib_flash_mode = AL3A_UI_FLASH_AUTO;
		break;
	case ISP_UI_FLASH_STATUS_ON:
		lib_flash_mode = AL3A_UI_FLASH_ON;
		break;
	case ISP_UI_FLASH_STATUS_TORCH:
		lib_flash_mode = AL3A_UI_FLASH_TORCH;
		break;
	default:
		ISP_LOGW("NOT defined ui flash status %ld!", (cmr_int)ui_flash_status);
		break;
	}

	ISP_LOGI("lib_flash_mode=%d", lib_flash_mode);
	param_ct_ptr->flash_mode = lib_flash_mode;
	type = AE_SET_PARAM_FLASH_MODE;
	in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_flash_prepare_on(struct aealtek_cxt *cxt_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct ae_set_param_t in_param;
	struct ae_output_data_t *output_param_ptr = NULL;
	enum ae_set_param_type_t type = 0;
	struct ae_set_param_content_t *param_ct_ptr = NULL;


	if (!cxt_ptr) {
		ISP_LOGE("param %p is NULL error!", cxt_ptr);
		goto exit;
	}

	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;

	type = AE_SET_PARAM_PRE_FLASHAE_EST;
	in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_flash_with_lighting(struct aealtek_cxt *cxt_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct ae_set_param_t in_param;
	struct ae_output_data_t *output_param_ptr = NULL;
	enum ae_set_param_type_t type = 0;
	struct ae_set_param_content_t *param_ct_ptr = NULL;


	if (!cxt_ptr) {
		ISP_LOGE("param %p is NULL error!", cxt_ptr);
		goto exit;
	}

	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;

	type = AE_SET_PARAM_PRECAP_START;
	in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_preflash_before(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_flash_notice *notice_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	cmr_int index = 0;
	cmr_int exp_gain = 0;


	cmr_bzero(&cxt_ptr->flash_param, sizeof(cxt_ptr->flash_param));
	cxt_ptr->flash_param.enable = 1;

	cxt_ptr->flash_param.led_info = notice_ptr->led_info;

	aealtek_change_flash_state(cxt_ptr, cxt_ptr->flash_param.flash_state, AEALTEK_FLASH_STATE_PREPARE_ON);
	return rtn;
}

static cmr_int aealtek_set_flash_notice(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_flash_notice *notice_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct ae_set_param_t in_param;
	struct ae_output_data_t output_param;
	enum ae_set_param_type_t type = 0;
	struct ae_set_param_content_t param_ct;

	enum isp_flash_mode mode = 0;

	UNUSED(out_ptr);
	if (!cxt_ptr || !notice_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, notice_ptr);
		goto exit;
	}

	mode = notice_ptr->flash_mode;
	ISP_LOGI("mode=%d", mode);
	switch (mode) {
	case ISP_FLASH_PRE_BEFORE:
		ISP_LOGI("=========pre flash before");
		ret = aealtek_set_preflash_before(cxt_ptr, notice_ptr);
		if (ret)
			goto exit;
		ret = aealtek_set_flash_prepare_on(cxt_ptr);
		if (ret)
			goto exit;
		ret = aealtek_set_boost(cxt_ptr, 1);
		if (ret)
			goto exit;
		break;

	case ISP_FLASH_PRE_LIGHTING:
		ISP_LOGI("=========pre flash lighting");
		ret = aealtek_set_lock(cxt_ptr, 0, __func__);
		if (ret)
			goto exit;

		/*
		ret = aealtek_set_flash_est(cxt_ptr, 0);
		if (ret)
			goto exit;
		aealtek_set_hw_flash_status(cxt_ptr, 1);
		*/

		aealtek_change_flash_state(cxt_ptr, cxt_ptr->flash_param.flash_state, AEALTEK_FLASH_STATE_LIGHTING);
		cxt_ptr->flash_skip_number = 4;

		break;

	case ISP_FLASH_PRE_AFTER:
		ISP_LOGI("=========pre flash after");
		aealtek_change_flash_state(cxt_ptr, cxt_ptr->flash_param.flash_state, AEALTEK_FLASH_STATE_CLOSE);
		cxt_ptr->flash_state_machine = AEALTEK_FLASH_STATE_CLOSE;

		aealtek_set_hw_flash_status(cxt_ptr, 0);
		aealtek_set_boost(cxt_ptr, 0);

		ret = aealtek_convert_lib_exposure2outdata(cxt_ptr, &cxt_ptr->flash_param.pre_flash_before.exp_cell, &cxt_ptr->lib_data.output_data);
		if (ret)
			goto exit;
		ret = aealtek_lib_exposure2sensor(cxt_ptr, &cxt_ptr->lib_data.output_data, &cxt_ptr->sensor_exp_data.lib_exp);
		if (ret)
			goto exit;
		break;

	case ISP_FLASH_MAIN_BEFORE:
		ISP_LOGI("=========main flash before");
		aealtek_change_flash_state(cxt_ptr, cxt_ptr->flash_param.flash_state, AEALTEK_FLASH_STATE_MAX);

		if (cxt_ptr->init_in_param.ops_in.flash_set_charge) {
			struct isp_flash_cfg flash_cfg;
			struct isp_flash_element  flash_element;

			flash_cfg.type = ISP_FLASH_TYPE_MAIN;

			if (ISP_FLASH_LED_0 & cxt_ptr->flash_param.led_info.led_tag) {
				if (0 < cxt_ptr->flash_param.main_flash_est.led_0.idx) {
					flash_cfg.led_idx = 1;
					flash_element.index = cxt_ptr->flash_param.main_flash_est.led_0.idx-1;
					flash_element.val = cxt_ptr->flash_param.main_flash_est.led_0.current;
					ISP_LOGI("led_idx %d  level index %d val %d", flash_cfg.led_idx, flash_element.index, flash_element.val);
					cxt_ptr->init_in_param.ops_in.flash_set_charge(cxt_ptr->caller_handle, &flash_cfg, &flash_element);
				}
			}

			if (ISP_FLASH_LED_1 & cxt_ptr->flash_param.led_info.led_tag) {
				if (0 < cxt_ptr->flash_param.main_flash_est.led_1.idx) {
					flash_cfg.led_idx = 2;
					flash_element.index = cxt_ptr->flash_param.main_flash_est.led_1.idx-1;
					flash_element.val = cxt_ptr->flash_param.main_flash_est.led_1.current;
					ISP_LOGI("led_idx %d level index %d val %d", flash_cfg.led_idx, flash_element.index, flash_element.val);
					cxt_ptr->init_in_param.ops_in.flash_set_charge(cxt_ptr->caller_handle, &flash_cfg, &flash_element);
				}
			}
		}
		cxt_ptr->main_flash_status = 1;
		break;

	case ISP_FLASH_MAIN_LIGHTING:
		/* burst is zsl */
		if (ISP_CAP_MODE_BURST == cxt_ptr->zsl_mode)
			ret = cxt_ptr->init_in_param.ops_in.flash_set_skip(cxt_ptr->caller_handle, 5);
		break;
	case ISP_FLASH_MAIN_AFTER:
		ISP_LOGI("=========main flash after");
		cxt_ptr->main_flash_status = 0;
		ret = cxt_ptr->init_in_param.ops_in.flash_set_skip(cxt_ptr->caller_handle, 0);
		aealtek_set_hw_flash_status(cxt_ptr, 0);
		ret = aealtek_convert_lib_exposure2outdata(cxt_ptr, &cxt_ptr->flash_param.pre_flash_before.exp_cell, &cxt_ptr->lib_data.output_data);
		if (ret)
			goto exit;
		ret = aealtek_lib_exposure2sensor(cxt_ptr, &cxt_ptr->lib_data.output_data, &cxt_ptr->sensor_exp_data.lib_exp);
		if (ret)
			goto exit;
		break;
	default:
		break;
	}
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_get_flash_effect(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_SUCCESS;

	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct ae_set_param_t in_param;
	struct ae_output_data_t output_param;
	enum ae_set_param_type_t type = 0;
	struct ae_set_param_content_t param_ct;

	UNUSED(out_ptr);
	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}
exit:
	return ret;
}

static cmr_int aealtek_get_iso_from_adgain(struct aealtek_cxt *cxt_ptr, cmr_u32 *from, cmr_u32 *to)
{
	cmr_int ret = ISP_ERROR;
	cmr_int lib_ret = 0;

	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct ae_get_param_t in_param;


	if (!cxt_ptr || !from || !to) {
		ISP_LOGE("param %p %p %p is NULL error!", cxt_ptr, from, to);
		goto exit;
	}
	obj_ptr = &cxt_ptr->al_obj;

	in_param.ae_get_param_type = AE_GET_ISO_FROM_ADGAIN;
	in_param.para.iso_adgain_info.ad_gain = *from;

	if (obj_ptr && obj_ptr->get_param)
		lib_ret = obj_ptr->get_param(&in_param, obj_ptr->ae);
	if (lib_ret)
		goto exit;

	*to = in_param.para.iso_adgain_info.ISO;

	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_sof_to_lib(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct aealtek_exposure_param exp_param)
{
	cmr_int ret = ISP_ERROR;
	cmr_u32 frame_len_cur = 0;
	cmr_u32 min_frame_length = 0;
	cmr_int lib_ret = 0;
	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct ae_set_param_t in_param;
	struct ae_output_data_t *output_param_ptr = NULL;
	enum ae_set_param_type_t type = 0;
	struct ae_set_param_content_t *param_ct_ptr = NULL;


	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}

	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;

	type = AE_SET_PARAM_SOF_NOTIFY;
	param_ct_ptr->sof_notify_param.sys_sof_index = in_ptr->sof_param.frame_index;
	param_ct_ptr->sof_notify_param.exp_linecount = exp_param.exp_line;
	param_ct_ptr->sof_notify_param.exp_adgain = exp_param.gain;
	param_ct_ptr->sof_notify_param.exp_time = exp_param.exp_time;
	min_frame_length = cxt_ptr->cur_status.min_frame_length;
	frame_len_cur = (exp_param.exp_line > min_frame_length) ? exp_param.exp_line:min_frame_length;
	if (frame_len_cur >= exp_param.exp_line) {
		param_ct_ptr->sof_notify_param.cuFPS =
			100*(SENSOR_EXP_US_BASE/(frame_len_cur * cxt_ptr->nxt_status.ui_param.work_info.resolution.line_time));
	} else {
		param_ct_ptr->sof_notify_param.cuFPS =
			100*(SENSOR_EXP_US_BASE/((exp_param.exp_line+exp_param.dummy)*cxt_ptr->nxt_status.ui_param.work_info.resolution.line_time));
	}
	ISP_LOGV("sof_index =%ld linecount=%d dummy=%d cuFPS=%d !",
		 in_ptr->sof_param.frame_index, exp_param.exp_line, exp_param.dummy, param_ct_ptr->sof_notify_param.cuFPS/100);
	in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret) {
		ISP_LOGE("lib_ret=%ld", lib_ret);
		goto exit;
	}
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld", ret);
	return ret;
}

static cmr_int aealtek_get_sync_info_from_lib(struct aealtek_cxt *cxt_ptr, struct aealtek_exposure_param *exp_gain)
{
	cmr_int ret = ISP_ERROR;
	struct ae_match_runtime_data_t master_dat;
	struct ae_match_runtime_data_t slave_dat;
	struct alaeruntimeobj_t *obj_ptr = NULL;
	cmr_u32 line_time;
	cmr_u32 line_time_slv;
	cmr_u32 min_frame_length = 0;
	cmr_u32 min_frame_length_slv = 0;
	struct match_data_param match_param;

	if (!cxt_ptr) {
		ISP_LOGE("param is NULL error,%p!", cxt_ptr);
		goto exit;
	}

	memset(&master_dat, 0x00, sizeof(struct ae_match_runtime_data_t));
	memset(&slave_dat, 0x00, sizeof(struct ae_match_runtime_data_t));
	exp_gain->dummy = 0;
	line_time = cxt_ptr->nxt_status.ui_param.work_info.resolution.line_time;
	line_time_slv = cxt_ptr->nxt_status.ui_param.work_info_slv.resolution.line_time;
	min_frame_length = cxt_ptr->cur_status.min_frame_length;
	min_frame_length_slv = cxt_ptr->cur_status.min_frame_length_slv;

	ISP_LOGV("master line_time =%d min_frame_length=%d,slave line_time = %d,min_frame_length_slv=%d",
			line_time, min_frame_length, line_time_slv, min_frame_length_slv);
	master_dat.uc_sensor_mode = 0;
	master_dat.sensor_ad_gain = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.sensor_ad_gain;
	master_dat.bv_val = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.bv_val;
	master_dat.EXIF_ISO = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.ae_cur_iso;
	master_dat.exposure_line = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.exposure_line;
	master_dat.exposure_time = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.exposure_time;
	master_dat.ISO = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.ae_cur_iso;
	master_dat.uwCur_fps = SENSOR_EXP_US_BASE / ((cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.exposure_line + cxt_ptr->sensor_exp_data.lib_exp.dummy)
			* line_time);
	master_dat.ISP_D_gain = 0;

	ISP_LOGI("master BV: %d, ISO : %d, EXIF ISO: %d, gain: %d, Line: %d, dummy: %d, Time: %d, ISP D gain: %d, fps: %d \r\n",
			master_dat.bv_val, master_dat.ISO, master_dat.EXIF_ISO,
			master_dat.sensor_ad_gain, master_dat.exposure_line, cxt_ptr->sensor_exp_data.lib_exp.dummy,
			master_dat.exposure_time, master_dat.ISP_D_gain, master_dat.uwCur_fps);

	obj_ptr = &cxt_ptr->al_obj;

	if (obj_ptr && obj_ptr->sync_process)
		ret = obj_ptr->sync_process(&master_dat, obj_ptr->ae, &slave_dat);

	ISP_LOGI("get slave BV: %d, ISO : %d, EXIF ISO: %d, gain: %d, Line: %d, Time: %d, ISP D gain: %d, fps: %d \r\n",
			slave_dat.bv_val, slave_dat.ISO, slave_dat.EXIF_ISO,
			slave_dat.sensor_ad_gain, slave_dat.exposure_line, slave_dat.exposure_time, slave_dat.ISP_D_gain,
			slave_dat.uwCur_fps);

	if (slave_dat.exposure_line < min_frame_length_slv) {
		exp_gain->dummy = min_frame_length_slv - slave_dat.exposure_line;
	}
	slave_dat.uwCur_fps = SENSOR_EXP_US_BASE / ((slave_dat.exposure_line + exp_gain->dummy) * line_time_slv);
	ISP_LOGV("before slave fps: %d", slave_dat.uwCur_fps);

	if ((master_dat.exposure_line > min_frame_length || slave_dat.exposure_line > min_frame_length_slv) && min_frame_length && min_frame_length_slv) {
		if (master_dat.exposure_time > slave_dat.exposure_time) {
			ISP_LOGI("slave: should set slave sensor dummy line to follow master low fps");
			exp_gain->dummy =
				(master_dat.exposure_time - slave_dat.exposure_time) * SENSOR_EXP_BASE / line_time_slv;
		} else if (master_dat.exposure_time < slave_dat.exposure_time) {
			ISP_LOGI("master: should set master sensor dummy line to follow slave low fps");
			cxt_ptr->sensor_exp_data.write_exp.dummy =
				((slave_dat.exposure_time - master_dat.exposure_time) * SENSOR_EXP_BASE) / line_time;
		}
	}
	slave_dat.uwCur_fps = SENSOR_EXP_US_BASE / ((slave_dat.exposure_line + exp_gain->dummy) * line_time_slv);
	ISP_LOGV("after actual slave fps: %d", slave_dat.uwCur_fps);

	cmr_bzero(&match_param, sizeof(match_param));
	match_param.ae_data.iso = slave_dat.ISO;
	match_param.ae_data.exif_iso = slave_dat.EXIF_ISO;
	match_param.ae_data.exposure_time = slave_dat.exposure_time;
	match_param.ae_data.exposure_line = slave_dat.exposure_line;
	match_param.ae_data.sensor_ad_gain = slave_dat.sensor_ad_gain;
	match_param.ae_data.isp_d_gain = slave_dat.ISP_D_gain;
	match_param.ae_data.uw_cur_fps = slave_dat.uwCur_fps;
	match_param.ae_data.bv_val = slave_dat.bv_val;
	match_param.ae_data.uc_sensor_mode = slave_dat.uc_sensor_mode;
	match_param.ae_data.master_bgbv = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.BG_BVResult;
	match_param.ae_data.master_curmean = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.curmean;
	isp_br_ioctrl(cxt_ptr->camera_id, SET_MATCH_AE_DATA, &match_param.ae_data, NULL);

	exp_gain->exp_line = slave_dat.exposure_line;
	exp_gain->exp_time = slave_dat.exposure_time;
	exp_gain->gain = slave_dat.sensor_ad_gain * SENSOR_GAIN_BASE / LIB_GAIN_BASE;
	exp_gain->iso = slave_dat.ISO;
	isp_mlog(AE_FILE, "ISO:%d,\nexp_time:%d, exp_line:%d,\nagain:%d,\nbv:%d, bg_bv:%d,\nd_clum:%d, c_lum:%d, t_lum:%d, sys_sof_index:%d,\
		 \n\n\nslave ISO:%d,\nexp_time:%d, exp_line:%d,\nagain:%d, bv:%d",
		 cxt_ptr->lib_data.output_data.udISO,
		 cxt_ptr->lib_data.output_data.udCur_exposure_time,
		 cxt_ptr->lib_data.output_data.udexposure_line,
		 cxt_ptr->lib_data.output_data.udsensor_ad_gain,
		 cxt_ptr->lib_data.output_data.bvresult,
		 cxt_ptr->lib_data.output_data.bg_bvresult,
		 (cxt_ptr->lib_data.output_data.ae_default_target)*4/100,
		 (cxt_ptr->lib_data.output_data.ae_cur_mean)*4/100,
		 (cxt_ptr->lib_data.output_data.ae_targetmean)*4/100,
		 cxt_ptr->lib_data.output_data.sys_sof_index,
		 slave_dat.ISO,
		 slave_dat.exposure_time,
		 slave_dat.exposure_line,
		 slave_dat.sensor_ad_gain,
		 slave_dat.bv_val);

	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld", ret);
	return ret;
}

static cmr_int aealtek_set_sof_to_lib_slv(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct aealtek_exposure_param exp_param)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct ae_set_param_t in_param;
	struct ae_output_data_t *output_param_ptr = NULL;
	enum ae_set_param_type_t type = 0;
	struct ae_set_param_content_t *param_ct_ptr = NULL;


	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}

	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;

	type = AE_SET_PARAM_SOF_NOTIFY_SLV;
	param_ct_ptr->sof_notify_param_slv_sensor.sys_sof_index = in_ptr->sof_param.frame_index;
	param_ct_ptr->sof_notify_param_slv_sensor.exp_linecount = exp_param.exp_line;
	param_ct_ptr->sof_notify_param_slv_sensor.exp_adgain = exp_param.gain;
	param_ct_ptr->sof_notify_param_slv_sensor.exp_time = exp_param.exp_time;

	in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret) {
		ISP_LOGE("lib_ret=%ld", lib_ret);
		goto exit;
	}
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld", ret);
	return ret;
}

/* TBD this code will remove to drv_u */
#include <sprd_sensor_k.h>
static cmr_int aealtek_handle_aec_info(struct aealtek_cxt *cxt_ptr,
					   struct ae_ctrl_param_sensor_exposure *master_exp_ptr,
					   struct ae_ctrl_param_sensor_gain *master_gain_ptr,
					   struct ae_ctrl_param_sensor_exposure *slave_exp_ptr,
					   struct ae_ctrl_param_sensor_gain *slave_gain_ptr)
{
	struct sensor_muti_aec_i2c_tag muti_aec_info;
	struct sensor_aec_reg_info aeinfo;
	struct sensor_aec_i2c_tag *aec_info;
	cmr_u8 i = 0;
	cmr_u16 size = 0;

	memset(&aeinfo, 0x00, sizeof(aeinfo));
	memset(&muti_aec_info, 0x00, sizeof(muti_aec_info));

	aeinfo.exp.exposure = master_exp_ptr->exp_line;
	aeinfo.exp.dummy = master_exp_ptr->dummy;
	aeinfo.exp.size_index = master_exp_ptr->size_index;
	aeinfo.gain = master_gain_ptr->gain;

	cxt_ptr->init_in_param.ops_in.read_aec_info(cxt_ptr->caller_handle, &aeinfo);
	aec_info = aeinfo.aec_i2c_info_out;

	/* write aec i2c */
	cmr_u16 sensor_id[AEC_I2C_SENSOR_MAX];
	cmr_u16 i2c_save_addr[AEC_I2C_SENSOR_MAX];
	cmr_u16 addr_bits_type[AEC_I2C_SENSOR_MAX];
	cmr_u16 data_bits_type[AEC_I2C_SENSOR_MAX];
	struct sensor_reg_tag msettings[AEC_I2C_SETTINGS_MAX];
	struct sensor_reg_tag ssettings[AEC_I2C_SETTINGS_MAX];

	muti_aec_info.sensor_id = sensor_id;
	muti_aec_info.id_size = AEC_I2C_SENSOR_MAX;
	muti_aec_info.i2c_slave_addr = i2c_save_addr;
	muti_aec_info.i2c_slave_len = AEC_I2C_SENSOR_MAX;
	muti_aec_info.addr_bits_type = addr_bits_type;
	muti_aec_info.addr_bits_type_len = AEC_I2C_SENSOR_MAX;
	muti_aec_info.data_bits_type = data_bits_type;
	muti_aec_info.data_bits_type_len = AEC_I2C_SENSOR_MAX;

	muti_aec_info.master_i2c_tab = msettings;
	muti_aec_info.slave_i2c_tab = ssettings;

	sensor_id[0] = cxt_ptr->camera_id;
	i2c_save_addr[0] = aec_info->slave_addr;
	addr_bits_type[0] = aec_info->addr_bits_type;
	data_bits_type[0] = aec_info->data_bits_type;

	for (i = 0; i < aec_info->frame_length->size; i++) {
		msettings[size].reg_addr = aec_info->frame_length->settings[i].reg_addr;
		msettings[size++].reg_value = aec_info->frame_length->settings[i].reg_value;
	}
	for (i = 0; i < aec_info->shutter->size; i++) {
		msettings[size].reg_addr = aec_info->shutter->settings[i].reg_addr;
		msettings[size++].reg_value = aec_info->shutter->settings[i].reg_value;
	}
	for (i = 0; i < aec_info->again->size; i++) {
		msettings[size].reg_addr = aec_info->again->settings[i].reg_addr;
		msettings[size++].reg_value = aec_info->again->settings[i].reg_value;
	}
	for (i = 0; i < aec_info->dgain->size; i++) {
		msettings[size].reg_addr = aec_info->dgain->settings[i].reg_addr;
		msettings[size++].reg_value = aec_info->dgain->settings[i].reg_value;
	}
	muti_aec_info.msize = aec_info->shutter->size +
				aec_info->again->size +
				aec_info->dgain->size +
				aec_info->frame_length->size;

	aeinfo.exp.exposure = slave_exp_ptr->exp_line;
	aeinfo.exp.dummy = slave_exp_ptr->dummy;
	aeinfo.exp.size_index = slave_exp_ptr->size_index;
	aeinfo.gain = slave_gain_ptr->gain;
	cxt_ptr->init_in_param.ops_in.read_aec_info_slv(cxt_ptr->caller_handle, &aeinfo);
	aec_info = aeinfo.aec_i2c_info_out;

	sensor_id[1] = cxt_ptr->camera_id + 2;
	i2c_save_addr[1] = aec_info->slave_addr;
	addr_bits_type[1] = aec_info->addr_bits_type;
	data_bits_type[1] = aec_info->data_bits_type;
	size = 0;
	for (i = 0; i < aec_info->frame_length->size; i++) {
		ssettings[size].reg_addr = aec_info->frame_length->settings[i].reg_addr;
		ssettings[size++].reg_value = aec_info->frame_length->settings[i].reg_value;
	}
	for (i = 0; i < aec_info->shutter->size; i++) {
		ssettings[size].reg_addr = aec_info->shutter->settings[i].reg_addr;
		ssettings[size++].reg_value = aec_info->shutter->settings[i].reg_value;
	}
	for (i = 0; i < aec_info->again->size; i++) {
		ssettings[size].reg_addr = aec_info->again->settings[i].reg_addr;
		ssettings[size++].reg_value = aec_info->again->settings[i].reg_value;
	}
	for (i = 0; i < aec_info->dgain->size; i++) {
		ssettings[size].reg_addr = aec_info->dgain->settings[i].reg_addr;
		ssettings[size++].reg_value = aec_info->dgain->settings[i].reg_value;
	}
	muti_aec_info.ssize = aec_info->shutter->size +
				aec_info->again->size +
				aec_info->dgain->size +
				aec_info->frame_length->size;

	cxt_ptr->init_in_param.ops_in.write_aec_info(cxt_ptr->caller_handle, &muti_aec_info);
	return 0;
}

static cmr_int aealtek_callback_sync_info(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr)
{
	cmr_int ret = ISP_ERROR;
	struct ae_ctrl_param_sensor_exposure sensor_exp;
	struct ae_ctrl_param_sensor_gain sensor_gain;
	struct ae_ctrl_param_sensor_exposure sensor_exp_slv;
	struct ae_ctrl_param_sensor_gain sensor_gain_slv;
	struct aealtek_exposure_param exp_param;

	if (!cxt_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}

	sensor_exp_slv.dummy = 0;
	sensor_exp_slv.size_index = cxt_ptr->cur_status.ui_param.work_info_slv.resolution.sensor_size_index;

	aealtek_get_sync_info_from_lib(cxt_ptr, &exp_param);
	sensor_exp_slv.exp_line = exp_param.exp_line;
	sensor_gain_slv.gain = exp_param.gain;
	sensor_exp_slv.dummy = exp_param.dummy;

	ret = aealtek_set_sof_to_lib_slv(cxt_ptr, in_ptr, exp_param);
	if (ret)
		goto exit;

	sensor_gain.gain = 0;
	memset(&sensor_exp, 0x00, sizeof(sensor_exp));
	ret = aealtek_pre_calc_ae(cxt_ptr, &sensor_exp, &sensor_gain);
	if ((sensor_exp_slv.exp_line != cxt_ptr->pre_write_exp_data_slv.exp_line
		|| sensor_exp_slv.dummy != cxt_ptr->pre_write_exp_data_slv.dummy
		|| sensor_gain_slv.gain != cxt_ptr->pre_write_exp_data_slv.gain
		|| sensor_exp.exp_line != cxt_ptr->pre_write_exp_data.exp_line
		|| sensor_exp.dummy != cxt_ptr->pre_write_exp_data.dummy
		|| sensor_gain.gain != cxt_ptr->pre_write_exp_data.gain)
		&& 0 != sensor_exp_slv.exp_line && 0 != sensor_gain_slv.gain
		&& 0 != sensor_exp.exp_line && 0 != sensor_gain.gain) {

		aealtek_handle_aec_info(cxt_ptr, &sensor_exp, &sensor_gain, &sensor_exp_slv, &sensor_gain_slv);
	}
	ret = aealtek_save_aec_info(cxt_ptr, &sensor_exp, &sensor_gain, &sensor_exp_slv, &sensor_gain_slv);

	if (cxt_ptr->init_in_param.ops_in.set_iso_slv) {
		ISP_LOGV("iso_speed_slv =%d", exp_param.iso);
		cxt_ptr->init_in_param.ops_in.set_iso_slv(cxt_ptr->caller_handle, exp_param.iso);
	}

	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld", ret);
	return ret;
}

static cmr_int aealtek_set_sof(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;
	struct seq_item in_est;
	struct seq_cell out_actual;
	struct seq_cell out_write;
	char ae_exp[PROPERTY_VALUE_MAX];
	char ae_gain[PROPERTY_VALUE_MAX];
	struct ae_ctrl_callback_in callback_in;

	if (!cxt_ptr || !in_ptr || !out_ptr) {
		ISP_LOGE("param %p %p %p is NULL error!", cxt_ptr, in_ptr, out_ptr);
		goto exit;
	}
	memset(&callback_in, 0x00, sizeof(callback_in));

	if (cxt_ptr->hdr_enable && cxt_ptr->nxt_status.is_hdr_status) {
		aealtek_set_hdr_lock(cxt_ptr, 1, __func__);
		ret = aealtek_set_hdr_ev(cxt_ptr, in_ptr, out_ptr);
		return ISP_SUCCESS;
	}

	if (cxt_ptr->main_flash_status && cxt_ptr->pre_cnt == 0) {
		ret = aealtek_convert_lib_exposure2outdata(cxt_ptr,
					&cxt_ptr->flash_param.main_flash_est.exp_cell, &cxt_ptr->lib_data.output_data);
		if (ret) {
			ISP_LOGW("failed !!!");
			goto exit;
		}
		ret = aealtek_lib_exposure2sensor(cxt_ptr, &cxt_ptr->lib_data.output_data, &cxt_ptr->sensor_exp_data.lib_exp);
		if (ret) {
			ISP_LOGW("failed !!!");
			goto exit;
		}
		cxt_ptr->sensor_exp_data.write_exp = cxt_ptr->sensor_exp_data.lib_exp;
		aealtek_pre_to_sensor(cxt_ptr, 1, 1);
		cxt_ptr->pre_cnt++;
		if (ret) {
			ISP_LOGW("failed !!!");
			goto exit;
		}
	}
	ISP_LOGV("pre_cnt=%d main_flash_status=%d", cxt_ptr->pre_cnt, cxt_ptr->main_flash_status);
	if (cxt_ptr->main_flash_status == 0) {
		cxt_ptr->pre_cnt = 0;
	}
	if (ISP3A_WORK_MODE_CAPTURE == cxt_ptr->cur_status.ui_param.work_info.work_mode) {
		return ISP_SUCCESS;
	}

	if (1 == in_ptr->sof_param.frame_index)
		seq_reset(cxt_ptr->seq_handle);

	if (4 == in_ptr->sof_param.frame_index) {
		ret = aealtek_set_boost(cxt_ptr, 0);
		if (ret)
			ISP_LOGW("set boost failed !!!");
	}

	cxt_ptr->sensor_exp_data.actual_exp = cxt_ptr->sensor_exp_data.lib_exp;
	cxt_ptr->sensor_exp_data.write_exp = cxt_ptr->sensor_exp_data.lib_exp;

	ISP_LOGI("sof_index:%ld,exp_line=%d,gain=%d,exp_time=%d,camera_id=%d", in_ptr->sof_param.frame_index,
				cxt_ptr->sensor_exp_data.write_exp.exp_line,
				cxt_ptr->sensor_exp_data.write_exp.gain,
				cxt_ptr->sensor_exp_data.write_exp.exp_time,
				cxt_ptr->camera_id);

	in_est.work_mode = SEQ_WORK_PREVIEW;
	in_est.cell.frame_id = in_ptr->sof_param.frame_index;
	if (cxt_ptr->tuning_info.manual_ae_on && TUNING_MODE_USER_DEF == cxt_ptr->tuning_info.tuning_mode) {
		cmr_bzero(ae_exp, sizeof(ae_exp));
		cmr_bzero(ae_gain, sizeof(ae_gain));
		property_get("persist.sys.isp.ae.exp_time", ae_exp, "100");
		property_get("persist.sys.isp.ae.gain", ae_gain, "100");
		in_est.cell.exp_time = atoi(ae_exp);
		in_est.cell.exp_line = SENSOR_EXP_BASE*in_est.cell.exp_time/cxt_ptr->nxt_status.ui_param.work_info.resolution.line_time;
		in_est.cell.gain = atoi(ae_gain);
		in_est.cell.dummy = 0;
	} else {
		in_est.cell.exp_line = cxt_ptr->sensor_exp_data.lib_exp.exp_line;
		in_est.cell.exp_time = cxt_ptr->sensor_exp_data.lib_exp.exp_time;
		in_est.cell.gain = cxt_ptr->sensor_exp_data.lib_exp.gain;
		in_est.cell.dummy = cxt_ptr->sensor_exp_data.lib_exp.dummy;
	}

	if (cxt_ptr->script_info.is_script_mode) {
		cmr_bzero(ae_exp, sizeof(ae_exp));
		property_get("persist.sys.isp.ae.script_mode", ae_exp, "none");
		if (!strcmp("ae", ae_exp)) {
			cxt_ptr->script_info.script_mode = AE_SCRIPT_ON;
		} else if (!strcmp("fe", ae_exp)) {
			cxt_ptr->script_info.script_mode = FE_SCRIPT_ON;
		} else {
			cxt_ptr->script_info.script_mode = AE_SCRIPT_OFF;
		}

		if (cxt_ptr->script_info.pre_script_mode != cxt_ptr->script_info.script_mode) {
			ret = aealtek_set_script_mode(cxt_ptr, cxt_ptr->script_info.script_mode);
		}

		cxt_ptr->script_info.pre_script_mode = cxt_ptr->script_info.script_mode;
	}

	ret = seq_put(cxt_ptr->seq_handle, &in_est, &out_actual, &out_write);
	if (ret) {
		ISP_LOGW("warning seq_put=%ld !!!", ret);
		out_actual = in_est.cell;
		out_write = in_est.cell;
	}

	ISP_LOGI("cxt_ptr->is_refocus %d sout_actual exp_line=%d, exp_time=%d, gain=%d",
		 cxt_ptr->is_refocus, out_actual.exp_line, out_actual.exp_time, out_actual.gain);

	if (0 == out_actual.exp_line || 0 == out_actual.gain) {
		out_actual = in_est.cell;
	}

	cxt_ptr->sensor_exp_data.actual_exp.exp_line = out_actual.exp_line;
	cxt_ptr->sensor_exp_data.actual_exp.exp_time = out_actual.exp_time;
	cxt_ptr->sensor_exp_data.actual_exp.gain = out_actual.gain;
	cxt_ptr->sensor_exp_data.actual_exp.dummy = out_actual.dummy;

	cxt_ptr->sensor_exp_data.write_exp.exp_line = out_write.exp_line;
	cxt_ptr->sensor_exp_data.write_exp.exp_time = out_write.exp_time;
	cxt_ptr->sensor_exp_data.write_exp.gain = out_write.gain;
	cxt_ptr->sensor_exp_data.write_exp.dummy = out_write.dummy;

	ret = aealtek_set_dummy(cxt_ptr, &cxt_ptr->sensor_exp_data.write_exp);
	if (ret)
		ISP_LOGW("warning set_dummy ret=%ld !!!", ret);
	ret = aealtek_set_dummy(cxt_ptr, &cxt_ptr->sensor_exp_data.actual_exp);
	if (ret)
		ISP_LOGW("warning set_dummy ret=%ld !!!", ret);
	ret = aealtek_set_sof_to_lib(cxt_ptr, in_ptr, cxt_ptr->sensor_exp_data.actual_exp);

	ret = aealtek_get_iso_from_adgain(cxt_ptr,
					  &cxt_ptr->sensor_exp_data.actual_exp.gain,
					  &callback_in.ae_cfg_info.hw_iso_speed);
	if (ret)
		goto exit;

	callback_in.ae_cfg_info.curmean = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.curmean;
	callback_in.ae_cfg_info.bv_val = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.bv_val;
	callback_in.ae_cfg_info.bg_bvresult = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.BG_BVResult;
	callback_in.ae_cfg_info.ae_cur_iso = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.ae_cur_iso;

	aealtek_get_iq_info(cxt_ptr, &callback_in.ae_cfg_info.ae_iq_info);

	cxt_ptr->init_in_param.ops_in.ae_callback(cxt_ptr->caller_handle, AE_CTRL_CB_SOF, &callback_in);
	if (ret)
		goto exit;

	if (!cxt_ptr->is_refocus && 0 == cxt_ptr->nxt_status.is_hdr_status) {
		ret = aealtek_pre_to_sensor(cxt_ptr, 0, 0);
		if (ret)
			goto exit;
	}

	if (cxt_ptr->is_refocus && cxt_ptr->is_master) {
		ret = aealtek_callback_sync_info(cxt_ptr, in_ptr);
	}
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld", ret);
	return ret;
}

static cmr_int aealtek_get_bv_by_lum(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	UNUSED(in_ptr);
	if (!cxt_ptr || !out_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, out_ptr);
		goto exit;
	}
	out_ptr->bv = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.bv_val;
	ISP_LOGI("bv=%d", out_ptr->bv);
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_get_bv_by_gain(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	UNUSED(in_ptr);
	if (!cxt_ptr || !out_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, out_ptr);
		goto exit;
	}
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_get_flicker_mode(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	UNUSED(in_ptr);
	if (!cxt_ptr || !out_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, out_ptr);
		goto exit;
	}
	out_ptr->flicker_mode = cxt_ptr->nxt_status.lib_ui_param.flicker;
	ISP_LOGI("flicker_mode=%d", out_ptr->flicker_mode);
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_set_fd_param(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct ae_set_param_t in_param;
	struct ae_output_data_t *output_param_ptr = NULL;
	enum ae_set_param_type_t type = 0;
	struct rect_roi_config_t *roi_ptr = NULL;
	cmr_u16 i = 0;
	cmr_u16 face_num = 0;
	cmr_u32 sx = 0;
	cmr_u32 sy = 0;
	cmr_u32 ex = 0;
	cmr_u32 ey = 0;

	UNUSED(out_ptr);
	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, in_ptr);
		goto exit;
	}

	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	roi_ptr = &in_param.set_param.ae_set_face_roi_setting;

	face_num = in_ptr->face_area.face_num;

	roi_ptr->roi_count = face_num > AL_MAX_ROI_NUM ? AL_MAX_ROI_NUM:face_num;
	for (i = 0; i < face_num; ++i) {
		sx = in_ptr->face_area.face_info[i].sx;
		sy = in_ptr->face_area.face_info[i].sy;
		ex = in_ptr->face_area.face_info[i].ex;
		ey = in_ptr->face_area.face_info[i].ey;

		roi_ptr->roi[i].weight = 0;
		roi_ptr->roi[i].roi.left = sx;
		roi_ptr->roi[i].roi.top = sy;
		roi_ptr->roi[i].roi.w = ex - sx + 1;
		roi_ptr->roi[i].roi.h = ey - sy + 1;
		roi_ptr->ref_frame_height = 0;
		roi_ptr->ref_frame_width = 0;
	}

	type = AE_SET_PARAM_FACE_ROI;
	in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_gyro_param(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct ae_set_param_t in_param;
	struct ae_output_data_t *output_param_ptr = NULL;
	enum ae_set_param_type_t type = 0;
	struct ae_set_param_content_t *param_ct_ptr = NULL;
	cmr_int i = 0;

	UNUSED(out_ptr);
	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}
	cxt_ptr->nxt_status.ui_param.iso = in_ptr->iso.iso_mode;

	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;


	param_ct_ptr->ae_gyro_info.param_mode_int = in_ptr->gyro.param_type;
	for (i = 0; i < 3; ++i) {
		param_ct_ptr->ae_gyro_info.uinfo[i] = in_ptr->gyro.uinfo[i];
		param_ct_ptr->ae_gyro_info.finfo[i] = in_ptr->gyro.finfo[i];
	}
	type = AE_SET_PARAM_GYRO_INFO;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_hdr_enable(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	UNUSED(out_ptr);
	if (!cxt_ptr) {
		ISP_LOGE("param %p is NULL error!", cxt_ptr);
		goto exit;
	}
	cxt_ptr->hdr_enable = in_ptr->soft_hdr_ev.enable;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_set_hdr_ev(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;
	enum ae_ctrl_hdr_ev_level level = 0;

	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct ae_set_param_t in_param;
	struct ae_output_data_t output_param;
	enum ae_set_param_type_t type = 0;
	struct ae_set_param_content_t *param_ct_ptr = NULL;

	UNUSED(out_ptr);
	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}

	cxt_ptr->nxt_status.ui_param.hdr_level = in_ptr->sof_param.frame_index - 1;
	level = cxt_ptr->nxt_status.ui_param.hdr_level;
	ISP_LOGI("hdr_level=%d", level);
	if (level >= 3) {
		return ISP_SUCCESS;
	}

	switch (level) {
	case AE_CTRL_HDR_EV_UNDEREXPOSURE:
		cxt_ptr->sensor_exp_data.lib_exp = cxt_ptr->lib_data.exposure_array.bracket_exp[0];
		ISP_LOGI("UNDEREXPOSURE exp_line=%d exp_time=%d,gain=%d",
			cxt_ptr->sensor_exp_data.lib_exp.exp_line,
			cxt_ptr->sensor_exp_data.lib_exp.exp_time,
			cxt_ptr->sensor_exp_data.lib_exp.gain);
		break;
	case AE_CTRL_HDR_EV_NORMAL:
		cxt_ptr->sensor_exp_data.lib_exp = cxt_ptr->lib_data.exposure_array.bracket_exp[1];
		ISP_LOGI("NORMAL exp_line=%d exp_time=%d,gain=%d",
			cxt_ptr->sensor_exp_data.lib_exp.exp_line,
			cxt_ptr->sensor_exp_data.lib_exp.exp_time,
			cxt_ptr->sensor_exp_data.lib_exp.gain);
		break;
	case AE_CTRL_HDR_EV_OVEREXPOSURE:
		cxt_ptr->sensor_exp_data.lib_exp = cxt_ptr->lib_data.exposure_array.bracket_exp[2];
		ISP_LOGI("OVEREXPOSURE exp_line=%d exp_time=%d,gain=%d",
			cxt_ptr->sensor_exp_data.lib_exp.exp_line,
			cxt_ptr->sensor_exp_data.lib_exp.exp_time,
			cxt_ptr->sensor_exp_data.lib_exp.gain);
		break;
	default:
		break;
	}
	ret = aealtek_set_dummy(cxt_ptr, &cxt_ptr->sensor_exp_data.lib_exp);
	if (ret)
		ISP_LOGW("warning set_dummy ret=%ld !!!", ret);
	ISP_LOGI("exp_line=%d dummy=%d size_index=%d gain=%d",
		cxt_ptr->sensor_exp_data.lib_exp.exp_line,
		cxt_ptr->sensor_exp_data.lib_exp.dummy,
		cxt_ptr->sensor_exp_data.lib_exp.size_index,
		cxt_ptr->sensor_exp_data.lib_exp.gain);
	ret = aealtek_pre_to_sensor(cxt_ptr, 1, 0);

	return ISP_SUCCESS;
exit:
	return ret;
}

static cmr_int aealtek_set_awb_info(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_awb_gain *awb_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct ae_set_param_t in_param;
	struct ae_output_data_t *output_param_ptr = NULL;
	enum ae_set_param_type_t type = 0;
	struct ae_set_param_content_t *param_ct_ptr = NULL;


	if (!cxt_ptr || !awb_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, awb_ptr);
		goto exit;
	}

	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;

	if (awb_ptr->normal_gain.r == 0
		&& awb_ptr->normal_gain.g == 0
		&& awb_ptr->normal_gain.b == 0) {
		param_ct_ptr->ae_awb_info.color_temp = 5000;
		param_ct_ptr->ae_awb_info.gain_r = cxt_ptr->lib_data.ae_otp_data.r;
		param_ct_ptr->ae_awb_info.gain_g = cxt_ptr->lib_data.ae_otp_data.g;
		param_ct_ptr->ae_awb_info.gain_b = cxt_ptr->lib_data.ae_otp_data.b;
		param_ct_ptr->ae_awb_info.awb_state = 1;
	} else {
		param_ct_ptr->ae_awb_info.color_temp = awb_ptr->color_temp;
		param_ct_ptr->ae_awb_info.gain_r = awb_ptr->normal_gain.r;
		param_ct_ptr->ae_awb_info.gain_g = awb_ptr->normal_gain.g;
		param_ct_ptr->ae_awb_info.gain_b = awb_ptr->normal_gain.b;

		param_ct_ptr->ae_awb_info.awb_balanced_wb.r_gain = awb_ptr->balance_gain.r;
		param_ct_ptr->ae_awb_info.awb_balanced_wb.g_gain = awb_ptr->balance_gain.g;
		param_ct_ptr->ae_awb_info.awb_balanced_wb.b_gain = awb_ptr->balance_gain.b;

		param_ct_ptr->ae_awb_info.awb_state = awb_ptr->awb_state;
	}
#if 0
	ISP_LOGI("awb color_temp=%d, r=%d, g=%d, b=%d, awb_state=%d",
		param_ct_ptr->ae_awb_info.color_temp,
		param_ct_ptr->ae_awb_info.gain_r,
		param_ct_ptr->ae_awb_info.gain_g,
		param_ct_ptr->ae_awb_info.gain_b,
		param_ct_ptr->ae_awb_info.awb_state);
#endif
	type = AE_SET_PARAM_AWB_INFO;
	in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret) {
		ISP_LOGE("lib set awb info %ld is error!", lib_ret);
		goto exit;
	}
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_awb_report(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	UNUSED(out_ptr);
	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}
	ret = aealtek_set_awb_info(cxt_ptr, &in_ptr->awb_report);
	if (ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld", ret);
	return ret;
}

static cmr_int aealtek_set_af_report(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct ae_set_param_t in_param;
	struct ae_output_data_t *output_param_ptr = NULL;
	enum ae_set_param_type_t type = 0;
	struct ae_set_param_content_t *param_ct_ptr = NULL;

	UNUSED(out_ptr);
	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, in_ptr);
		goto exit;
	}
	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;

	param_ct_ptr->ae_af_info.af_state = in_ptr->af_report.state;
	param_ct_ptr->ae_af_info.af_distance = in_ptr->af_report.distance;

	type = AE_SET_PARAM_AF_INFO;
	in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret) {
		ISP_LOGE("lib set af info %ld is error!", lib_ret);
		goto exit;
	}
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_snapshot_finished(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	UNUSED(in_ptr);
	UNUSED(out_ptr);
	if (!cxt_ptr) {
		ISP_LOGE("param %p is NULL error!", cxt_ptr);
		goto exit;
	}
	ISP_LOGI("flash_enable:%d,touch_flag:%d", cxt_ptr->flash_param.enable, cxt_ptr->touch_param.touch_flag);
	if (cxt_ptr->flash_param.enable) {
		ret = aealtek_set_flash_est(cxt_ptr, 1);
		if (ret)
			goto exit;
		cxt_ptr->flash_param.enable = 0;
		aealtek_set_lock(cxt_ptr, 0, __func__);
	}
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_set_master(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	UNUSED(out_ptr);
	if (!cxt_ptr) {
		ISP_LOGE("param %p is NULL error!", cxt_ptr);
		goto exit;
	}
	cxt_ptr->is_master = in_ptr->value;
	ISP_LOGI("camera_id %d master = %d", cxt_ptr->camera_id, cxt_ptr->is_master);
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_get_flash_eb(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	UNUSED(in_ptr);
	if (!cxt_ptr || !out_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, out_ptr);
		goto exit;
	}
	out_ptr->flash_eb = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.ae_need_flash_flg;
	ISP_LOGI("need flash=%d", out_ptr->flash_eb);
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_get_iso(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	UNUSED(in_ptr);
	if (!cxt_ptr || !out_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, out_ptr);
		goto exit;
	}
	out_ptr->iso_val = cxt_ptr->sensor_exp_data.lib_exp.iso;
	ISP_LOGI("iso_val=%d", out_ptr->iso_val);
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_get_ae_state(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	UNUSED(in_ptr);
	if (!cxt_ptr || !out_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, out_ptr);
		goto exit;
	}
	out_ptr->ae_state = cxt_ptr->lib_data.output_data.ae_est_status;
	ISP_LOGI("ae_state=%d", out_ptr->ae_state);
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_get_ev_table(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	UNUSED(in_ptr);
	if (!cxt_ptr || !out_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, out_ptr);
		goto exit;
	}
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_get_debug_data(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	UNUSED(in_ptr);
	if (!cxt_ptr || !out_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, out_ptr);
		goto exit;
	}
	out_ptr->debug_param.size = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.ae_debug_valid_size;
	out_ptr->debug_param.data = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.ae_debug_data;

	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_get_exif_data(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	UNUSED(in_ptr);
	if (!cxt_ptr || !out_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, out_ptr);
		goto exit;
	}
	out_ptr->exif_param.size = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.ae_commonexif_valid_size;
	out_ptr->exif_param.data = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.ae_commonexif_data;

	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_get_ext_debug_info(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	UNUSED(in_ptr);
	if (!cxt_ptr || !out_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, out_ptr);
		goto exit;
	}
	out_ptr->debug_info.flash_flag = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.ae_need_flash_flg;
	out_ptr->debug_info.fn_value = cxt_ptr->init_in_param.sensor_static_info.f_num;
	out_ptr->debug_info.valid_ad_gain = cxt_ptr->sensor_exp_data.actual_exp.gain;
	out_ptr->debug_info.valid_exposure_line = cxt_ptr->sensor_exp_data.actual_exp.exp_line;
	out_ptr->debug_info.valid_exposure_time = cxt_ptr->sensor_exp_data.actual_exp.exp_time;
	ISP_LOGI("exp_line %d gain %d! exptime %d", out_ptr->debug_info.valid_exposure_line, out_ptr->debug_info.valid_ad_gain, out_ptr->debug_info.valid_exposure_time);
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_get_hw_iso_speed(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	UNUSED(in_ptr);
	if (!cxt_ptr || !out_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, out_ptr);
		goto exit;
	}

	ret = aealtek_get_iso_from_adgain(cxt_ptr, &cxt_ptr->sensor_exp_data.actual_exp.gain, &out_ptr->hw_iso_speed);
	if (ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_get_exp_gain(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	UNUSED(in_ptr);
	if (!cxt_ptr || !out_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, out_ptr);
		goto exit;
	}
	out_ptr->exp_gain.exposure_time = cxt_ptr->sensor_exp_data.actual_exp.exp_time;
	out_ptr->exp_gain.exposure_line = cxt_ptr->sensor_exp_data.actual_exp.exp_line;
	out_ptr->exp_gain.dummy = cxt_ptr->sensor_exp_data.actual_exp.dummy;
	out_ptr->exp_gain.gain = cxt_ptr->sensor_exp_data.actual_exp.gain;

	ISP_LOGI("exp:%d,gain:%d", out_ptr->exp_gain.exposure_time, out_ptr->exp_gain.gain);

	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_set_ui_flash_mode(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	UNUSED(out_ptr);
	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, in_ptr);
		goto exit;
	}

	ret = aealtek_set_lib_ui_flash_mode(cxt_ptr, in_ptr->flash.flash_mode);
	if (ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_set_y_hist_stats(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;
	struct al3awrapper_stats_yhist_t  wrapper_y_hist;

	UNUSED(out_ptr);
	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, in_ptr);
		goto exit;
	}

	memset(&wrapper_y_hist, 0x00, sizeof(wrapper_y_hist));
	wrapper_y_hist.b_is_stats_byaddr = TRUE;
	wrapper_y_hist.pt_hist_y = cxt_ptr->stat_info[cxt_ptr->stat_info_num].ae_stats.y_hist;

	if (!in_ptr->y_hist_stat->addr) {
		ISP_LOGE("y hist stat data is NULL");
		goto exit;
	}
	ret = al3awrapper_dispatchhw3a_yhiststats((struct isp_drv_meta_yhist_t *)in_ptr->y_hist_stat->addr, &wrapper_y_hist);
	if (ret) {
		ISP_LOGE("failed to dispatch yhist");
		goto exit;
	}

	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_get_lib_script_info(struct aealtek_cxt *cxt_ptr, struct ae_output_data_t *from_ptr, struct aealtek_exposure_param *to_ptr)
{
	cmr_int ret = ISP_ERROR;


	if (!cxt_ptr || !from_ptr || !to_ptr) {
		ISP_LOGE("param %p %p %p is NULL error!", cxt_ptr, from_ptr, to_ptr);
		goto exit;
	}
	ISP_LOGV("ae_script_mode=%d", from_ptr->ae_script_mode);
	ISP_LOGV("max_cnt=%d, cur_cnt=%d", from_ptr->ae_script_info.udmaxscriptcnt,
			from_ptr->ae_script_info.udcurrentscriptcnt);

	to_ptr->exp_line = from_ptr->ae_script_info.udcurrentscriptexpline;
	to_ptr->exp_time = from_ptr->ae_script_info.udcurrentscriptexptime;
	to_ptr->gain = from_ptr->ae_script_info.udcurrentscriptgain;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_get_tuning_data(struct aealtek_cxt *cxt_ptr)
{
	cmr_int ret = ISP_ERROR;
	char ae_property[PROPERTY_VALUE_MAX];
	FILE *fp = NULL;
	cmr_s32 tem_value;
	char str_value[50];
	char file_name[128];
	cmr_s32 flag = 0;
	cmr_s32 exp_num = 0;
	cmr_s32 gain_num = 0;

	cmr_bzero(ae_property, sizeof(ae_property));
	property_get("persist.sys.isp.ae.manual", ae_property, "off");
	ISP_LOGI("persist.sys.isp.ae.manual: %s", ae_property);
	if (!strcmp("on", ae_property)) {
		cxt_ptr->tuning_info.manual_ae_on = 1;
		cmr_bzero(ae_property, sizeof(ae_property));
		property_get("persist.sys.isp.ae.tuning.type", ae_property, "none");
		ISP_LOGI("persist.sys.isp.ae.tuning.type: %s", ae_property);
		if (!strcmp("exposure_time", ae_property)) {
			cxt_ptr->tuning_info.tuning_mode = TUNING_MODE_EXPOSURE;
			sprintf(file_name, "/data/misc/media/exposure.txt");
		} else if (!strcmp("gain", ae_property)) {
			cxt_ptr->tuning_info.tuning_mode = TUNING_MODE_GAIN;
			sprintf(file_name, "/data/misc/media/gain.txt");
		} else if (!strcmp("obclamp", ae_property)) {
			cxt_ptr->tuning_info.tuning_mode = TUNING_MODE_OBCLAMP;
			sprintf(file_name, "/data/misc/media/obclamp.txt");
		} else if (!strcmp("user_def", ae_property)) {
			cxt_ptr->tuning_info.tuning_mode = TUNING_MODE_USER_DEF;
		}

		if (TUNING_MODE_USER_DEF != cxt_ptr->tuning_info.tuning_mode) {
			if (0 == cxt_ptr->tuning_info.num) {
				fp = fopen(file_name, "r");
				if (NULL == fp) {
					ISP_LOGE("failed to open tuning file:%s", file_name);
					goto exit;
				}
				while (NULL != fgets(str_value, sizeof(str_value)-1, fp)) {
					tem_value = atoi(str_value);
					if (0 == tem_value) {
						flag = 0;
					} else if (0 ==  flag) {
						cxt_ptr->tuning_info.gain[gain_num++] = tem_value;
						flag++;
					} else {
						cxt_ptr->tuning_info.exposure[exp_num++] = tem_value;
						flag = 0;
					}
				}
				fclose(fp);
			}
		}
	} else {
		cxt_ptr->tuning_info.manual_ae_on = 0;
	}

	cmr_bzero(ae_property, sizeof(ae_property));
	property_get("persist.sys.isp.ae.script", ae_property, "off");
	ISP_LOGI("persist.sys.isp.ae.script: %s", ae_property);
	if (!strcmp("on", ae_property)) {
		cxt_ptr->script_info.is_script_mode = 1;
	} else {
		cxt_ptr->script_info.is_script_mode = 0;
	}

	return ISP_SUCCESS;
exit:
	cxt_ptr->tuning_info.manual_ae_on = 0;
	ISP_LOGE("done %ld", ret);
	return ret;
}

static cmr_int ae_altek_adpt_init(void *in, void *out, cmr_handle *handle)
{
	cmr_int ret = ISP_ERROR;
	struct aealtek_cxt *cxt_ptr = NULL;
	struct ae_ctrl_init_in *in_ptr = (struct ae_ctrl_init_in *)in;
	struct ae_ctrl_init_out *out_ptr = (struct ae_ctrl_init_out *)out;

	if (!in_ptr || !out_ptr || !handle) {
		ISP_LOGE("init param %p %p is null !!!", in_ptr, out_ptr);
		ret = ISP_PARAM_NULL;
		goto exit;
	}

	*handle = NULL;
	cxt_ptr = (struct aealtek_cxt *)malloc(sizeof(*cxt_ptr));
	if (NULL == cxt_ptr) {
		ISP_LOGE("failed to create ae ctrl context!");
		ret = ISP_ALLOC_ERROR;
		goto exit;
	}
	cmr_bzero(cxt_ptr, sizeof(*cxt_ptr));

	cxt_ptr->camera_id = in_ptr->camera_id;
	cxt_ptr->caller_handle = in_ptr->caller_handle;
	cxt_ptr->init_in_param = *in_ptr;
	cxt_ptr->is_refocus = in_ptr->preview_work.is_refocus;
	cxt_ptr->is_master = in_ptr->is_master;

	ISP_LOGI("init frame %dx%d,lint_time=%d,index=%d",
		cxt_ptr->init_in_param.preview_work.resolution.frame_size.w,
		cxt_ptr->init_in_param.preview_work.resolution.frame_size.h,
		cxt_ptr->init_in_param.preview_work.resolution.line_time,
		cxt_ptr->init_in_param.preview_work.resolution.sensor_size_index);

	ret = aealtek_load_lib(cxt_ptr);
	if (ret) {
		goto exit;
	}
	ret = aealtek_init(cxt_ptr, in_ptr, out_ptr);
	if (ret) {
		goto exit;
	}
	ret = aealtek_first_work(cxt_ptr, NULL, NULL);
	if (ret)
		goto exit;
	ret = aealtek_get_iso_from_adgain(cxt_ptr, &cxt_ptr->sensor_exp_data.lib_exp.gain, &out_ptr->hw_iso_speed);
	if (ret)
		goto exit;

	ret = aealtek_get_hwisp_cfg(cxt_ptr, &out_ptr->hw_cfg);
	if (ret)
		goto exit;

	ret = aealtek_pre_to_sensor(cxt_ptr, 1, 0);
	if (ret)
		goto exit;

	ret = aealtek_get_tuning_data(cxt_ptr);
	if (ret)
		ISP_LOGW("get tuning data failed %ld", ret);

	ret = aealtek_set_boost(cxt_ptr, 1);
	if (ret)
		ISP_LOGW("set boost failed !!!");

	*handle = (cmr_handle)cxt_ptr;
	cxt_ptr->is_inited = 1;
	return ISP_SUCCESS;
exit:
	if (cxt_ptr) {
		aealtek_unload_lib(cxt_ptr);

		free((void *)cxt_ptr);
	}
	return ret;
}

static cmr_int ae_altek_adpt_deinit(cmr_handle handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct aealtek_cxt *cxt_ptr = (struct aealtek_cxt *)handle;

	ISP_CHECK_HANDLE_VALID(handle);

	if (0 == cxt_ptr->is_inited) {
		ISP_LOGW("has been de-initialized");
		goto exit;
	}
	if (cxt_ptr->al_obj.deinit)
		cxt_ptr->al_obj.deinit(cxt_ptr->al_obj.ae);

	aealtek_unload_lib(cxt_ptr);

	seq_deinit(cxt_ptr->seq_handle);

	cxt_ptr->is_inited = 0;
	free((void *)handle);
exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

static cmr_int ae_altek_adpt_ioctrl(cmr_handle handle, cmr_int cmd, void *in, void *out)
{
	cmr_int ret = ISP_SUCCESS;
	struct aealtek_cxt *cxt_ptr = (struct aealtek_cxt *)handle;
	struct ae_ctrl_param_in *in_ptr = (struct ae_ctrl_param_in *)in;
	struct ae_ctrl_param_out *out_ptr = (struct ae_ctrl_param_out *)out;


	if (!handle) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}
	switch (cmd) {
	case AE_CTRL_GET_LUM:
		break;
	case AE_CTRL_GET_ISO:
		ret = aealtek_get_iso(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_GET_EV_TABLE:
		ret = aealtek_get_ev_table(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_SET_BYPASS:
		ret = aealtek_set_bypass(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_SET_FLICKER:
		ret = aealtek_set_flicker(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_SET_SCENE_MODE:
		ret = aealtek_set_scene_mode(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_SET_ISO:
		ret = aealtek_set_iso(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_SET_FPS:
		ret = aealtek_set_fps(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_SET_EXP_COMP:
		ret = aealtek_set_exp_comp(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_SET_MEASURE_LUM:
		ret = aealtek_set_measure_lum(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_SET_STAT_TRIM:
		ret = aealtek_set_stat_trim(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_SET_TOUCH_ZONE:
		ret = aealtek_set_touch_zone(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_SET_WORK_MODE:
		ret = aealtek_set_work_mode(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_SET_PAUSE:
		ret = aealtek_set_lock(cxt_ptr, 1, __func__);
		break;
	case AE_CTRL_SET_RESTORE:
		ret = aealtek_set_lock(cxt_ptr, 0, __func__);
		break;
	case AE_CTRL_SET_FLASH_NOTICE:
		ret = aealtek_set_flash_notice(cxt_ptr, &in_ptr->flash_notice, out_ptr);
		break;
	case AE_CTRL_GET_FLASH_EFFECT:
		ret = aealtek_get_flash_effect(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_GET_AE_STATE:
		ret = aealtek_get_ae_state(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_GET_FLASH_EB:
		ret = aealtek_get_flash_eb(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_SET_SOF:
		ret = aealtek_set_sof(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_GET_BV_BY_LUM:
		ret = aealtek_get_bv_by_lum(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_GET_BV_BY_GAIN:
		ret = aealtek_get_bv_by_gain(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_SET_FORCE_PAUSE:
		aealtek_set_lock(cxt_ptr, 1, __func__);
		break;
	case AE_CTRL_SET_FORCE_RESTORE:
		aealtek_set_lock(cxt_ptr, 0, __func__);
		break;
	case AE_CTRL_GET_MONITOR_INFO:
		break;
	case AE_CTRL_GET_FLICKER_MODE:
		ret = aealtek_get_flicker_mode(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_SET_FD_ENABLE:
		break;
	case AE_CTRL_SET_FD_PARAM:
		ret = aealtek_set_fd_param(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_GET_SKIP_FRAME_NUM:
		break;
	case AE_CTRL_SET_QUICK_MODE:
		break;
	case AE_CTRL_SET_GYRO_PARAM:
		ret = aealtek_set_gyro_param(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_SET_HDR_EV:
		break;
	case AE_CTRL_SET_HDR_ENABLE:
		ret = aealtek_set_hdr_enable(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_GET_HWISP_CONFIG:
		break;
	case AE_CTRL_SET_AWB_REPORT:
		ret = aealtek_set_awb_report(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_SET_AF_REPORT:
		ret = aealtek_set_af_report(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_SET_CONVERGENCE_REQ:
		ret = aealtek_set_convergence_req(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_SET_SNAPSHOT_FINISHED:
		ret = aealtek_set_snapshot_finished(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_SET_MASTER:
		ret = aealtek_set_master(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_GET_DEBUG_DATA:
		ret = aealtek_get_debug_data(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_GET_EXIF_DATA:
		ret = aealtek_get_exif_data(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_GET_EXT_DEBUG_INFO:
		ret = aealtek_get_ext_debug_info(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_GET_HW_ISO_SPEED:
		ret = aealtek_get_hw_iso_speed(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_GET_EXP_GAIN:
		ret = aealtek_get_exp_gain(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_SET_FLASH_MODE:
		ret = aealtek_set_ui_flash_mode(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_SET_Y_HIST_STATS:
		ret = aealtek_set_y_hist_stats(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_SET_FIX_EXPOSURE_TIME:
		ret = aealtek_set_fix_exposure_time(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_SET_FIX_SENSITIVITY:
		ret = aealtek_set_fix_sensitivity(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_SET_FIX_FRAME_DURATION:
		ret = aealtek_set_fix_frame_duration(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_SET_MANUAL_EXPTIME:
		ret = aealtek_set_manaul_exptime(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_SET_MANUAL_GAIN:
		ret = aealtek_set_manaul_gain(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_SET_MANUAL_ISO:
		ret = aealtek_set_manaul_iso(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_SET_ENGINEER_MODE:
		ret = aealtek_set_engineer_mode(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_SET_TUNING_MODE:
		ret = aealtek_set_tuning_mode(cxt_ptr, in_ptr);
		break;
	default:
		ISP_LOGE("cmd %ld is not defined!", cmd);
		break;
	};
exit:

	return ret;
}

static cmr_int aealtek_pre_process(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_proc_in *in_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_uint lib_ret = 0;
	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct al3awrapper_stats_ae_t *ppatched_aedat;


	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, in_ptr);
		goto exit;
	}
	obj_ptr = &cxt_ptr->al_obj;

	ppatched_aedat = &cxt_ptr->lib_data.stats_data;
	ppatched_aedat->ptstatsy = cxt_ptr->stat_info[cxt_ptr->stat_info_num].statsy;
	ppatched_aedat->ptstatsr = cxt_ptr->stat_info[cxt_ptr->stat_info_num].ae_stats.r_info;
	ppatched_aedat->ptstatsg = cxt_ptr->stat_info[cxt_ptr->stat_info_num].ae_stats.g_info;
	ppatched_aedat->ptstatsb = cxt_ptr->stat_info[cxt_ptr->stat_info_num].ae_stats.b_info;

	if (obj_ptr) {
		if (!in_ptr->stat_data_ptr->addr) {
			ISP_LOGE("LIB AE stat data is NULL");
			goto exit;
		}
		lib_ret = al3awrapper_dispatchhw3a_aestats((struct isp_drv_meta_ae_t *)in_ptr->stat_data_ptr->addr,
							   &cxt_ptr->lib_data.stats_data,
							   obj_ptr, obj_ptr->ae);
		if (ERR_WPR_AE_SUCCESS != lib_ret) {
			ret = ISP_ERROR;
			ISP_LOGE("dispatch lib_ret=%ld", lib_ret);
			goto exit;
		}
	}
	return ISP_SUCCESS;
exit:
	ISP_LOGI("done %ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static void aealtek_flash_info_to_awb(struct ae_report_update_t *from_ptr, struct isp3a_ae_report *to_ptr)
{
	to_ptr->flash_param_preview.blending_ratio_led1 = from_ptr->preflash_report.flash_ratio;
	to_ptr->flash_param_preview.blending_ratio_led2 = from_ptr->preflash_report.flash_ratio_led2;
	to_ptr->flash_param_preview.color_temp_led1 = from_ptr->preflash_report.LED1_CT;
	to_ptr->flash_param_preview.color_temp_led2 = from_ptr->preflash_report.LED2_CT;
	to_ptr->flash_param_preview.wbgain_led1.r = from_ptr->preflash_report.flash_gain.r_gain;
	to_ptr->flash_param_preview.wbgain_led1.g = from_ptr->preflash_report.flash_gain.g_gain;
	to_ptr->flash_param_preview.wbgain_led1.b = from_ptr->preflash_report.flash_gain.b_gain;
	to_ptr->flash_param_preview.wbgain_led2.r = from_ptr->preflash_report.flash_gain_led2.r_gain;
	to_ptr->flash_param_preview.wbgain_led2.g = from_ptr->preflash_report.flash_gain_led2.g_gain;
	to_ptr->flash_param_preview.wbgain_led2.b = from_ptr->preflash_report.flash_gain_led2.b_gain;

	to_ptr->flash_param_capture.blending_ratio_led1 = from_ptr->mainflash_report.flash_ratio;
	to_ptr->flash_param_capture.blending_ratio_led2 = from_ptr->mainflash_report.flash_ratio_led2;
	to_ptr->flash_param_capture.color_temp_led1 = from_ptr->mainflash_report.LED1_CT;
	to_ptr->flash_param_capture.color_temp_led2 = from_ptr->mainflash_report.LED2_CT;
	to_ptr->flash_param_capture.wbgain_led1.r = from_ptr->mainflash_report.flash_gain.r_gain;
	to_ptr->flash_param_capture.wbgain_led1.g = from_ptr->mainflash_report.flash_gain.g_gain;
	to_ptr->flash_param_capture.wbgain_led1.b = from_ptr->mainflash_report.flash_gain.b_gain;
	to_ptr->flash_param_capture.wbgain_led2.r = from_ptr->mainflash_report.flash_gain_led2.r_gain;
	to_ptr->flash_param_capture.wbgain_led2.g = from_ptr->mainflash_report.flash_gain_led2.g_gain;
	to_ptr->flash_param_capture.wbgain_led2.b = from_ptr->mainflash_report.flash_gain_led2.b_gain;
}

static cmr_u32 aealtek_flash_process(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_callback_in *callback_in
		, cmr_u32 *is_special_converge_flag)
{
	cmr_u32 is_callback = 0;
	cmr_int ret = ISP_SUCCESS;

	/*flash*/
	if (cxt_ptr->flash_param.enable) {
		ISP_LOGI("======lib flash converged =%d,ae_st=%d,flash_st=%d",
				cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.ae_converged,
				cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.ae_LibStates,
				cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.ae_FlashStates);
		switch (cxt_ptr->flash_param.flash_state) {
		case AEALTEK_FLASH_STATE_PREPARE_ON:
			ISP_LOGV("========flash led prepare on");

			*is_special_converge_flag = 1;
			if (cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.ae_converged
				&& AE_EST_WITH_LED_PRE_DONE == cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.ae_FlashStates) {
				if (cxt_ptr->flash_state_machine == AEALTEK_FLASH_STATE_PREPARE_ON) {
					return ISP_SUCCESS;
				}
				cxt_ptr->flash_state_machine = AEALTEK_FLASH_STATE_PREPARE_ON;
				aealtek_set_lock(cxt_ptr, 1, __func__);
				aealtek_change_ae_state(cxt_ptr, cxt_ptr->ae_state, ISP3A_AE_CTRL_ST_FLASH_PREPARE_CONVERGED);

				cxt_ptr->flash_param.pre_flash_before.led_num = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.preflash_ctrldat.ucDICTotal_idx;
				cxt_ptr->flash_param.pre_flash_before.led_0.idx = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.preflash_ctrldat.ucDIC1_idx;
				cxt_ptr->flash_param.pre_flash_before.led_0.current = (cmr_s32)cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.preflash_ctrldat.fLED1Current;
				cxt_ptr->flash_param.pre_flash_before.led_1.idx = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.preflash_ctrldat.ucDIC2_idx;
				cxt_ptr->flash_param.pre_flash_before.led_1.current = (cmr_s32)cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.preflash_ctrldat.fLED2Current;

				cxt_ptr->flash_param.pre_flash_before.exp_cell.gain = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.flash_off_exp_dat.ad_gain;
				cxt_ptr->flash_param.pre_flash_before.exp_cell.exp_line = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.flash_off_exp_dat.exposure_line;
				cxt_ptr->flash_param.pre_flash_before.exp_cell.exp_time = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.flash_off_exp_dat.exposure_time;
				cxt_ptr->flash_param.pre_flash_before.exp_cell.iso = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.flash_off_exp_dat.ISO;

				if (cxt_ptr->init_in_param.ops_in.flash_set_charge) {
					struct isp_flash_cfg flash_cfg;
					struct isp_flash_element  flash_element;

					ISP_LOGI("========flash pre exp:%d,%d,%d,%d",
							cxt_ptr->flash_param.pre_flash_before.exp_cell.gain,
							cxt_ptr->flash_param.pre_flash_before.exp_cell.exp_line,
							cxt_ptr->flash_param.pre_flash_before.exp_cell.exp_time,
							cxt_ptr->flash_param.pre_flash_before.exp_cell.iso);

					ISP_LOGI("========flash pre led_info:%d,%d,%d,%d,%d,%d",
							cxt_ptr->flash_param.pre_flash_before.led_num,
							cxt_ptr->flash_param.led_info.led_tag,
							cxt_ptr->flash_param.pre_flash_before.led_0.idx,
							cxt_ptr->flash_param.pre_flash_before.led_0.current,
							cxt_ptr->flash_param.pre_flash_before.led_1.idx,
							cxt_ptr->flash_param.pre_flash_before.led_1.current);

					flash_cfg.type = ISP_FLASH_TYPE_PREFLASH;

					if (ISP_FLASH_LED_0 & cxt_ptr->flash_param.led_info.led_tag) {
						if (0 < cxt_ptr->flash_param.pre_flash_before.led_0.idx) {
							flash_cfg.led_idx = 1;
							flash_element.index = cxt_ptr->flash_param.pre_flash_before.led_0.idx-1;
							flash_element.val = cxt_ptr->flash_param.pre_flash_before.led_0.current;
							cxt_ptr->init_in_param.ops_in.flash_set_charge(cxt_ptr->caller_handle, &flash_cfg, &flash_element);
						}
					}

					if (ISP_FLASH_LED_1 & cxt_ptr->flash_param.led_info.led_tag) {
						if (0 < cxt_ptr->flash_param.pre_flash_before.led_1.idx) {
							flash_cfg.led_idx = 2;
							flash_element.index = cxt_ptr->flash_param.pre_flash_before.led_1.idx-1;
							flash_element.val = cxt_ptr->flash_param.pre_flash_before.led_1.current;
							cxt_ptr->init_in_param.ops_in.flash_set_charge(cxt_ptr->caller_handle, &flash_cfg, &flash_element);
						}
					}
				}
				/*if (cxt_ptr->touch_param.touch_flag
					&& cxt_ptr->touch_param.ctrl_roi_changed_flag) {
					cxt_ptr->init_in_param.ops_in.ae_callback(cxt_ptr->caller_handle, AE_CTRL_CB_CONVERGED, callback_in);
				} else {
					cxt_ptr->init_in_param.ops_in.ae_callback(cxt_ptr->caller_handle, AE_CTRL_CB_CONVERGED, callback_in);
				}*/
				is_callback = 1;

			}
			break;
		case AEALTEK_FLASH_STATE_LIGHTING:
			ISP_LOGV("========flash led on");

			*is_special_converge_flag = 1;
			if (cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.ae_converged
				&& AE_EST_WITH_LED_DONE == cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.ae_FlashStates) {
				if (cxt_ptr->flash_state_machine == AEALTEK_FLASH_STATE_LIGHTING) {
					return ISP_SUCCESS;
				}
				cxt_ptr->flash_state_machine = AEALTEK_FLASH_STATE_LIGHTING;
				aealtek_set_lock(cxt_ptr, 1, __func__);
				aealtek_change_ae_state(cxt_ptr, cxt_ptr->ae_state, ISP3A_AE_CTRL_ST_FLASH_ON_CONVERGED);

				/*cxt_ptr->init_in_param.ops_in.ae_callback(cxt_ptr->caller_handle, AE_CTRL_CB_FLASHING_CONVERGED, callback_in);*/
				is_callback = 1;

				cxt_ptr->flash_param.main_flash_est.led_num = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.mainflash_ctrldat.ucDICTotal_idx;
				cxt_ptr->flash_param.main_flash_est.led_0.idx = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.mainflash_ctrldat.ucDIC1_idx;
				cxt_ptr->flash_param.main_flash_est.led_0.current = (cmr_s32)cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.mainflash_ctrldat.fLED1Current;
				cxt_ptr->flash_param.main_flash_est.led_1.idx = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.mainflash_ctrldat.ucDIC2_idx;
				cxt_ptr->flash_param.main_flash_est.led_1.current = (cmr_s32)cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.mainflash_ctrldat.fLED2Current;

				cxt_ptr->flash_param.main_flash_est.exp_cell.gain = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.snapshot_exp_dat.ad_gain;
				cxt_ptr->flash_param.main_flash_est.exp_cell.exp_line = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.snapshot_exp_dat.exposure_line;
				cxt_ptr->flash_param.main_flash_est.exp_cell.exp_time = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.snapshot_exp_dat.exposure_time;
				cxt_ptr->flash_param.main_flash_est.exp_cell.iso = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.snapshot_exp_dat.ISO;

				ISP_LOGI("========flash main exp:%d,%d,%d,%d",
						cxt_ptr->flash_param.main_flash_est.exp_cell.gain,
						cxt_ptr->flash_param.main_flash_est.exp_cell.exp_line,
						cxt_ptr->flash_param.main_flash_est.exp_cell.exp_time,
						cxt_ptr->flash_param.main_flash_est.exp_cell.iso);

				ISP_LOGI("========flash main led_info:%d,%d,%d,%d,%d,%d",
						cxt_ptr->flash_param.main_flash_est.led_num,
						cxt_ptr->flash_param.led_info.led_tag,
						cxt_ptr->flash_param.main_flash_est.led_0.idx,
						cxt_ptr->flash_param.main_flash_est.led_0.current,
						cxt_ptr->flash_param.main_flash_est.led_1.idx,
						cxt_ptr->flash_param.main_flash_est.led_1.current);

				aealtek_change_flash_state(cxt_ptr, cxt_ptr->flash_param.flash_state, AEALTEK_FLASH_STATE_KEEPING);
			}
			break;
		default:
			break;
		}

		aealtek_flash_info_to_awb(&cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update, &callback_in->proc_out.ae_info.report_data);
	}

	return is_callback;
}

static cmr_int aealtek_post_process(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_proc_in *in_ptr)
{
	cmr_int ret = ISP_ERROR;
	struct ae_ctrl_callback_in callback_in;
	cmr_u32 is_special_converge_flag = 0;
	cmr_u32 data_length = 0;
	struct ae_script_param_t  *ae_script_info = NULL;
	cmr_u32 flash_callback = 0;

	if (!cxt_ptr) {
		ISP_LOGE("param %p is NULL error!", cxt_ptr);
		goto exit;
	}
	memset(&callback_in, 0x00, sizeof(callback_in));
	if (!cxt_ptr->tuning_info.manual_ae_on
			&& AE_DISABLED != cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.ae_LibStates
			&& AE_LOCKED != cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.ae_LibStates) {
		ret = aealtek_lib_exposure2sensor(cxt_ptr, &cxt_ptr->lib_data.output_data, &cxt_ptr->sensor_exp_data.lib_exp);
		if (ret)
			goto exit;

		ret = aealtek_set_dummy(cxt_ptr, &cxt_ptr->sensor_exp_data.lib_exp);
		if (ret)
			ISP_LOGW("warning set_dummy ret=%ld !!!", ret);
	}

	if (cxt_ptr->script_info.is_script_mode) {
		ae_script_info = &cxt_ptr->lib_data.output_data.ae_script_info;
		ISP_LOGI("scriptstate:%d, mode:%d,flashon:%d,flashmode:%d"
				, ae_script_info->scriptstate
				, cxt_ptr->script_info.script_mode
				, ae_script_info->bisneedflashTurnOn
				, ae_script_info->ucFlashMode);
		if (AE_SCRIPT_ON == cxt_ptr->script_info.script_mode || FE_SCRIPT_ON == cxt_ptr->script_info.script_mode) {
			ret = aealtek_get_lib_script_info(cxt_ptr, &cxt_ptr->lib_data.output_data, &cxt_ptr->sensor_exp_data.lib_exp);
			if (ret)
				goto exit;

			if (FE_SCRIPT_ON == cxt_ptr->script_info.script_mode) {
				struct isp_flash_cfg flash_cfg;
				struct isp_flash_element  flash_element;

				if (0 == ae_script_info->ucFlashMode) {
					flash_cfg.type = ISP_FLASH_TYPE_PREFLASH;
				} else if (1 == ae_script_info->ucFlashMode) {
					flash_cfg.type = ISP_FLASH_TYPE_MAIN;
				}

				if (cxt_ptr->script_info.is_flash_on != ae_script_info->bisneedflashTurnOn) {
					if (ae_script_info->bisneedflashTurnOn) {
						ISP_LOGI("script led1:%d,%f,led2:%d,%f"
								, ae_script_info->udled1index
								, ae_script_info->fled1current
								, ae_script_info->udled2index
								, ae_script_info->fled2current);
						flash_cfg.led_idx = 1;
						flash_element.index = ae_script_info->udled1index;
						flash_element.val = ae_script_info->fled1current;
						cxt_ptr->init_in_param.ops_in.flash_set_charge(cxt_ptr->caller_handle, &flash_cfg, &flash_element);
						flash_cfg.led_idx = 2;
						flash_element.index = ae_script_info->udled2index;
						flash_element.val = ae_script_info->fled2current;
						cxt_ptr->init_in_param.ops_in.flash_set_charge(cxt_ptr->caller_handle, &flash_cfg, &flash_element);

						flash_cfg.led_idx = 1; /*turn on flash*/
					} else {
						flash_cfg.led_idx = 0; /*close flash*/
					}
					cxt_ptr->init_in_param.ops_in.flash_ctrl(cxt_ptr->caller_handle, &flash_cfg, NULL);
					cxt_ptr->script_info.is_flash_on = ae_script_info->bisneedflashTurnOn;
				}
			}
		}

		if (SCRIPT_DONE == ae_script_info->scriptstate) {
			cxt_ptr->script_info.script_mode = AE_SCRIPT_OFF;
			ret = aealtek_set_script_mode(cxt_ptr, AE_SCRIPT_OFF);
			cxt_ptr->script_info.is_script_mode = 0;
		}

		ret = aealtek_set_dummy(cxt_ptr, &cxt_ptr->sensor_exp_data.lib_exp);
		if (ret)
			ISP_LOGW("warning set_dummy ret=%ld !!!", ret);
	}

	aealtek_change_ae_state(cxt_ptr, cxt_ptr->ae_state, ISP3A_AE_CTRL_ST_SEARCHING);

	/* touch */
	if (cxt_ptr->touch_param.touch_flag) {
		if (cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.ae_roi_change_st)
			cxt_ptr->touch_param.ctrl_roi_changed_flag = 1;

		ISP_LOGI("======touch_req =%d,%d,%d", cxt_ptr->touch_param.ctrl_convergence_req_flag,
				cxt_ptr->touch_param.ctrl_roi_changed_flag,
				cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.ae_LibStates);

		/* wait roi converged command */
		if (cxt_ptr->touch_param.ctrl_convergence_req_flag) {
			is_special_converge_flag = 1;

			if (cxt_ptr->touch_param.ctrl_roi_changed_flag
				&& AE_ROI_CONVERGED == cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.ae_LibStates) {
				ret = aealtek_set_boost(cxt_ptr, 0);
				if (ret)
					ISP_LOGW("set boost failed !!!");
				ISP_LOGI("CB TOUCH_CONVERGED");
				aealtek_change_ae_state(cxt_ptr, cxt_ptr->ae_state, ISP3A_AE_CTRL_ST_CONVERGED);
				cxt_ptr->init_in_param.ops_in.ae_callback(cxt_ptr->caller_handle, AE_CTRl_CB_TOUCH_CONVERGED, &callback_in);
				//cxt_ptr->touch_param.touch_flag = 0;
				cxt_ptr->touch_param.ctrl_convergence_req_flag = 0;
			}
		}
	}
	flash_callback = aealtek_flash_process(cxt_ptr, &callback_in, &is_special_converge_flag);

	if (0 == is_special_converge_flag
		&& cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.ae_converged) {
		aealtek_change_ae_state(cxt_ptr, cxt_ptr->ae_state, ISP3A_AE_CTRL_ST_CONVERGED);
	}

	if (cxt_ptr->init_in_param.ops_in.ae_callback) {
		aealtek_lib_to_out_info(cxt_ptr, &cxt_ptr->lib_data.output_data, &callback_in.proc_out.ae_info);
		if ((cxt_ptr->ae_state == AE_CONVERGED || cxt_ptr->ae_state == AE_LOCKED) ||
		   (cxt_ptr->is_3dcalibration && cxt_ptr->ae_state == AE_CONVERGED))/**add for 3d calibration send stable notify*/
			cxt_ptr->init_in_param.ops_in.ae_callback(cxt_ptr->caller_handle, AE_CTRL_CB_STAB_NOTIFY, &callback_in);

		callback_in.proc_out.priv_size = sizeof(cxt_ptr->lib_data.output_data.rpt_3a_update);
		callback_in.proc_out.priv_data = &cxt_ptr->lib_data.output_data.rpt_3a_update;

		callback_in.proc_out.ae_frame.is_skip_cur_frame = 0;
		callback_in.proc_out.ae_frame.stats_buff_ptr = in_ptr->stat_data_ptr;
		callback_in.proc_out.ae_frame.awb_stats_buff_ptr = in_ptr->awb_stat_data_ptr;
		data_length = ARRAY_SIZE(cxt_ptr->stat_info) - 1;
		callback_in.proc_out.ae_info.report_data.rgb_stats = &cxt_ptr->stat_info[cxt_ptr->stat_info_num].ae_stats;
		callback_in.proc_out.ae_info.valid = 1;
		cxt_ptr->init_in_param.ops_in.ae_callback(cxt_ptr->caller_handle, AE_CTRL_CB_PROC_OUT, &callback_in);
		if (data_length == cxt_ptr->stat_info_num) {
			cxt_ptr->stat_info_num = 0;
		} else {
			cxt_ptr->stat_info_num++;
		}

		if (flash_callback)
			cxt_ptr->init_in_param.ops_in.ae_callback(cxt_ptr->caller_handle, AE_CTRL_CB_CONVERGED, &callback_in);
	}

	return ISP_SUCCESS;
exit:
	ISP_LOGI("ret=%ld", ret);
	return ret;
}

static cmr_int aealtek_process(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_proc_in *in_ptr, struct ae_ctrl_proc_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_uint lib_ret = 0;
	struct alaeruntimeobj_t *obj_ptr = NULL;
	struct ae_output_data_t *out_data_ptr = NULL;

	UNUSED(out_ptr);
	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, in_ptr);
		goto exit;
	}
	obj_ptr = &cxt_ptr->al_obj;
	out_data_ptr = &cxt_ptr->lib_data.output_data;

	if (obj_ptr && obj_ptr->estimation)
		lib_ret = obj_ptr->estimation(&cxt_ptr->lib_data.stats_data, obj_ptr->ae, out_data_ptr);
	if (lib_ret) {
		ISP_LOGW("ae estimation failed lib_ret:0x%x", (cmr_u32)lib_ret);
		if (_AL_AELIB_INVALID_PARAM == lib_ret) {
			if (cxt_ptr->touch_param.touch_flag) {
				aealtek_reset_touch_param(cxt_ptr);
				ret = aealtek_reset_touch_ack(cxt_ptr);
			}
		}
	}

	if (cxt_ptr->is_processed == 0) {
		cxt_ptr->is_processed = 1;
	}

	ISP_LOGI("mean=%d, BV=%d, exposure_line=%d, gain=%d, ae_states:%d, sof_index %d camera_id:%d",
		out_data_ptr->rpt_3a_update.ae_update.curmean,
		out_data_ptr->rpt_3a_update.ae_update.bv_val,
		out_data_ptr->rpt_3a_update.ae_update.exposure_line,
		out_data_ptr->rpt_3a_update.ae_update.sensor_ad_gain,
		out_data_ptr->rpt_3a_update.ae_update.ae_LibStates,
		out_data_ptr->rpt_3a_update.ae_update.sys_sof_index,
		cxt_ptr->camera_id);

	aealtek_print_lib_log(cxt_ptr, &cxt_ptr->lib_data.output_data);
	if (!cxt_ptr->is_refocus) {
		isp_mlog(AE_FILE, "ISO:%d, exp_time:%d, exp_line:%d, again:%d, bv:%d, bg_bv:%d,d_clum:%d, c_lum:%d, t_lum:%d, sys_sof_index:%d",
			 cxt_ptr->lib_data.output_data.udISO,
			 cxt_ptr->lib_data.output_data.udCur_exposure_time,
			 cxt_ptr->lib_data.output_data.udexposure_line,
			 cxt_ptr->lib_data.output_data.udsensor_ad_gain,
			 cxt_ptr->lib_data.output_data.bvresult,
			 cxt_ptr->lib_data.output_data.bg_bvresult,
			 (cxt_ptr->lib_data.output_data.ae_default_target)*4/100,
			 (cxt_ptr->lib_data.output_data.ae_cur_mean)*4/100,
			 (cxt_ptr->lib_data.output_data.ae_targetmean)*4/100,
			 cxt_ptr->lib_data.output_data.sys_sof_index);
	}
	return ISP_SUCCESS;
exit:
	ISP_LOGI("done %ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int ae_altek_adpt_process(cmr_handle handle, void *in, void *out)
{
	cmr_int ret = ISP_ERROR;
	struct aealtek_cxt *cxt_ptr = (struct aealtek_cxt *)handle;
	struct ae_ctrl_proc_in *in_ptr = (struct ae_ctrl_proc_in *)in;
	struct ae_ctrl_proc_out *out_ptr = (struct ae_ctrl_proc_out *)out;


	if (!handle || !in) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}
	cxt_ptr->proc_in = *in_ptr;


	if (!in_ptr->stat_data_ptr) {
		ISP_LOGE("param stat data is NULL error!");
		goto exit;
	}

	ret = aealtek_pre_process(cxt_ptr, in_ptr);
	if (ret)
		goto exit;
	ret = aealtek_process(cxt_ptr, in_ptr, out_ptr);
	if (ret)
		goto exit;
	ret = aealtek_post_process(cxt_ptr, in_ptr);
	if (ret)
		goto exit;

	if (cxt_ptr->flash_skip_number > 0) {
		cxt_ptr->flash_skip_number--;
		ISP_LOGI("flash skip_number:%d, flash_state:%d\n", cxt_ptr->flash_skip_number, cxt_ptr->flash_param.flash_state);
		if (0 == cxt_ptr->flash_skip_number && AEALTEK_FLASH_STATE_LIGHTING == cxt_ptr->flash_param.flash_state) {
			ret = aealtek_set_flash_est(cxt_ptr, 0);
			ret = aealtek_set_hw_flash_status(cxt_ptr, 1);
		}
	}

	return ISP_SUCCESS;
exit:
{
	struct ae_ctrl_callback_in callback_in;

		callback_in.proc_out.ae_frame.is_skip_cur_frame = 1;
		callback_in.proc_out.ae_frame.stats_buff_ptr = in_ptr->stat_data_ptr;
		callback_in.proc_out.ae_frame.awb_stats_buff_ptr = in_ptr->awb_stat_data_ptr;
		cxt_ptr->init_in_param.ops_in.ae_callback(cxt_ptr->caller_handle, AE_CTRL_CB_PROC_OUT, &callback_in);
}
	ISP_LOGI("done %ld", ret);
	return ret;
}

/*************************************ADAPTER CONFIGURATION ***************************************/
static struct isp_lib_config ae_altek_lib_info = {
	.product_id = 0,
	.version_id = 0,
};

static struct adpt_ops_type ae_altek_adpt_ops = {
	.adpt_init = ae_altek_adpt_init,
	.adpt_deinit = ae_altek_adpt_deinit,
	.adpt_process = ae_altek_adpt_process,
	.adpt_ioctrl = ae_altek_adpt_ioctrl,
};

struct adpt_register_type ae_altek_adpt_info = {
	.lib_type = ADPT_LIB_AE,
	.lib_info = &ae_altek_lib_info,
	.ops = &ae_altek_adpt_ops,
};
