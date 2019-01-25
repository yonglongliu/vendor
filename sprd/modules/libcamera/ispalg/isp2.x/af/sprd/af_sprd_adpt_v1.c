/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define LOG_TAG "af_sprd_adpt_v1"
#define ATRACE_TAG (ATRACE_TAG_CAMERA | ATRACE_TAG_HAL)
#include <cutils/trace.h>

#include <assert.h>
#include <cutils/properties.h>
#include <dlfcn.h>
#include <inttypes.h>

#include "af_ctrl.h"
#include "af_sprd_adpt_v1.h"

#ifndef UNUSED
#define     UNUSED(param)  (void)(param)
#endif

#define FOCUS_STAT_DATA_NUM 2

#ifndef MAX
#define  MAX( _x, _y ) ( ((_x) > (_y)) ? (_x) : (_y) )
#endif

#ifndef MIN
#define  MIN( _x, _y ) ( ((_x) < (_y)) ? (_x) : (_y) )
#endif

static const char *state_string[] = {
	"manual",
	"normal_af",
	"caf",
	"record caf",
	"faf",
	"fullscan",
	"picture",
};

#define STATE_STRING(state)    state_string[state]

static const char *focus_state_str[] = {
	"af idle",
	"af searching",
};

#define FOCUS_STATE_STR(state)    focus_state_str[state]

static char AFlog_buffer[2048] = { 0 };

#ifndef CONFIG_ISP_2_5
static struct af_iir_nr_info_u af_iir_nr[3] = {
	{							// weak
	 .iir_nr_en = 1,
	 .iir_g0 = 378,
	 .iir_c1 = -676,
	 .iir_c2 = -324,
	 .iir_c3 = 512,
	 .iir_c4 = 1024,
	 .iir_c5 = 512,
	 .iir_g1 = 300,
	 .iir_c6 = -537,
	 .iir_c7 = -152,
	 .iir_c8 = 512,
	 .iir_c9 = 1024,
	 .iir_c10 = 512,
	 },
	{							// medium
	 .iir_nr_en = 1,
	 .iir_g0 = 185,
	 .iir_c1 = 0,
	 .iir_c2 = -229,
	 .iir_c3 = 512,
	 .iir_c4 = 1024,
	 .iir_c5 = 512,
	 .iir_g1 = 133,
	 .iir_c6 = 0,
	 .iir_c7 = -20,
	 .iir_c8 = 512,
	 .iir_c9 = 1024,
	 .iir_c10 = 512,
	 },
	{							// strong
	 .iir_nr_en = 1,
	 .iir_g0 = 81,
	 .iir_c1 = 460,
	 .iir_c2 = -270,
	 .iir_c3 = 512,
	 .iir_c4 = 1024,
	 .iir_c5 = 512,
	 .iir_g1 = 60,
	 .iir_c6 = 344,
	 .iir_c7 = 74,
	 .iir_c8 = 512,
	 .iir_c9 = 1024,
	 .iir_c10 = 512,
	 },
};

static struct af_enhanced_module_info_u af_enhanced_module = {
	.chl_sel = 0,
	.nr_mode = 2,
	.center_weight = 2,
	.fv_enhanced_mode = {5, 5},
	.clip_en = {0, 0},
	.max_th = {131071, 131071},
	.min_th = {100, 100},
	.fv_shift = {0, 0},
	.fv1_coeff = {
				  -2, -2, -2, -2, 16, -2, -2, -2, -2,
				  -3, 5, 3, 5, 0, -5, 3, -5, -3,
				  3, 5, -3, -5, 0, 5, -3, -5, 3,
				  0, -8, 0, -8, 16, 0, 0, 0, 0},
};
#endif

char libafv1_path[][20] = {
	"libspafv1.so",
	"libspafv1_le.so",
	"libaf_v2.so",
	"libaf_v3.so",
	"libaf_v4.so",
	"libaf_v5.so",
};

static cmr_s32 _check_handle(cmr_handle handle)
{
	cmr_s32 rtn = AFV1_SUCCESS;
	af_ctrl_t *af = (af_ctrl_t *) handle;

	if (NULL == af) {
		ISP_LOGE("fail to get valid cxt pointer");
		return AFV1_ERROR;
	}

	return rtn;
}

// misc
static cmr_u64 get_systemtime_ns()
{
	cmr_s64 timestamp = systemTime(CLOCK_MONOTONIC);
	return timestamp;
}

// afm hardware
static void afm_enable(af_ctrl_t * af)
{
	cmr_u32 bypass = 0;

	af->cb_ops.af_monitor_bypass(af->caller, (void *)&bypass);
}

static void afm_disable(af_ctrl_t * af)
{
	cmr_u32 bypass = 1;
	af->cb_ops.af_monitor_bypass(af->caller, (void *)&bypass);
}

static void afm_setup(af_ctrl_t * af)
{
#ifndef CONFIG_ISP_2_5
	struct af_enhanced_module_info_u afm_enhanced_module;
#endif
	cmr_u32 mode = 1;

	af->cb_ops.af_monitor_skip_num(af->caller, (void *)&af->afm_skip_num);
	af->cb_ops.af_monitor_mode(af->caller, (void *)&mode);
#ifndef CONFIG_ISP_2_5
	af->cb_ops.af_monitor_iir_nr_cfg(af->caller, (void *)&(af_iir_nr[af->afm_tuning.iir_level]));

	memcpy(&(afm_enhanced_module), &af_enhanced_module, sizeof(struct af_enhanced_module_info_u));
	afm_enhanced_module.nr_mode = af->afm_tuning.nr_mode;
	afm_enhanced_module.center_weight = af->afm_tuning.cw_mode;
	afm_enhanced_module.fv_enhanced_mode[0] = af->afm_tuning.fv0_e;
	afm_enhanced_module.fv_enhanced_mode[1] = af->afm_tuning.fv1_e;
	af->cb_ops.af_monitor_module_cfg(af->caller, (void *)&(afm_enhanced_module));
#endif
}

static cmr_u32 afm_get_win_num(struct afctrl_init_in *input_param)
{
	cmr_u32 num;
	struct afctrl_init_in *input_ptr = input_param;
	input_ptr->cb_ops.get_monitor_win_num(input_ptr->caller, &num);
	ISP_LOGI("win_num %d", num);
	return num;
}

static void afm_set_win(af_ctrl_t * af, win_coord_t * win, cmr_s32 num, cmr_s32 hw_num)
{
	cmr_s32 i;
	struct af_monitor_win winparam;

	for (i = num; i < hw_num; ++i) {
		// some strange hardware behavior
		win[i].start_x = 0;
		win[i].start_y = 0;
		win[i].end_x = 2;
		win[i].end_y = 2;
	}

	winparam.win_pos = (struct af_win_rect *)win;	//todo : compare with kernel type

	af->cb_ops.set_monitor_win(af->caller, &winparam);
}

static cmr_s32 afm_get_fv(af_ctrl_t * af, cmr_u64 * fv, cmr_u32 filter_mask, cmr_s32 roi_num)
{
	cmr_s32 i = 0;
	cmr_u64 *p = fv;

	if (filter_mask & SOBEL5_BIT) {	// not impelmented in this AFM
	}

	if (filter_mask & SOBEL9_BIT) {	// not impelmented in this AFM
	}

	if (filter_mask & SPSMD_BIT) {	// not impelmented in this AFM
	}

	if (filter_mask & ENHANCED_BIT) {
		for (i = 0; i < roi_num; ++i) {
			//ISP_LOGI("fv0[%d]:%15" PRIu64 ", fv1[%d]:%15" PRIu64 ".", i, af->af_fv_val.af_fv0[i], i, af->af_fv_val.af_fv1[i]);
			*p++ = af->af_fv_val.af_fv0[i];
		}
	}

	return 0;
}

// start hardware
static cmr_s32 do_start_af(af_ctrl_t * af)
{
	afm_set_win(af, af->roi.win, af->roi.num, af->isp_info.win_num);
	afm_setup(af);
	afm_enable(af);
	return 0;
}

// stop hardware
static cmr_s32 do_stop_af(af_ctrl_t * af)
{
	afm_disable(af);
	return 0;
}

// len
static cmr_u16 lens_get_pos(af_ctrl_t * af)
{
	cmr_u16 pos = 0;

	if (NULL == af->cb_ops.af_get_motor_pos) {
		ISP_LOGE("af->af_get_motor_pos null");
	} else {
		af->cb_ops.af_get_motor_pos(af->caller, &pos);
	}
	if (0 == pos || pos > 1023) {
		pos = af->lens.pos;
	}

	ISP_LOGV("pos = %d", pos);
	return pos;
}

static void lens_move_to(af_ctrl_t * af, cmr_u16 pos)
{
	cmr_u16 last_pos = 0;

	if (NULL == af->cb_ops.af_set_motor_pos) {
		ISP_LOGE("af->af_set_motor_pos null error");
		return;
	}

	last_pos = lens_get_pos(af);
	if (last_pos != pos) {
		ISP_LOGI("pos = %d", pos);
		af->cb_ops.af_set_motor_pos(af->caller, pos);
		af->lens.pos = pos;
	} else {
		ISP_LOGV("pos %d was set last time", pos);
	}
}

static void calc_roi(af_ctrl_t * af, const struct af_trig_info *win, eAF_MODE alg_mode)
{
	cmr_u32 i;
	AF_HW_Wins hw_wins;

	if (NULL == win) {
		ISP_LOGV("win is NULL, use default roi");
	} else {
		ISP_LOGV("valid_win = %d, mode = %d", win->win_num, win->mode);
		AF_Roi af_roi;
		for (i = 0; i < win->win_num; ++i) {
			af_roi.index = i;
			af_roi.start_x = win->win_pos[i].sx;
			af_roi.start_y = win->win_pos[i].sy;
			af_roi.end_x = win->win_pos[i].ex;
			af_roi.end_y = win->win_pos[i].ey;
			af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_Record_Wins, &af_roi);
			ISP_LOGV("win %d: start_x = %d, start_y = %d, end_x = %d, end_y = %d", i, win->win_pos[i].sx, win->win_pos[i].sy, win->win_pos[i].ex, win->win_pos[i].ey);
		}
		if (0 == win->win_num)
			win = NULL;
	}
	hw_wins.win_settings = (void *)win;
	hw_wins.af_mode = alg_mode;
	af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_Set_Hw_Wins, &hw_wins);
}

static cmr_s32 compare_timestamp(af_ctrl_t * af)
{

	if (af->dcam_timestamp < af->vcm_timestamp + 100000000LL)
		return DCAM_AFTER_VCM_NO;
	else
		return DCAM_AFTER_VCM_YES;
}

static void notify_start(af_ctrl_t * af, cmr_u32 focus_type)
{
	ISP_LOGI(".");
	struct af_result_param af_result;
	af_result.focus_type = focus_type;
	af->cb_ops.start_notice(af->caller, &af_result);
}

static void notify_stop(af_ctrl_t * af, cmr_s32 win_num, cmr_u32 focus_type)
{
	struct af_result_param af_result;
	af_result.suc_win = win_num;
	af_result.focus_type = focus_type;
	ISP_LOGI(". %s ", (win_num) ? "Suc" : "Fail");

	af->cb_ops.end_notice(af->caller, &af_result);
}

// i/f to AF model
static cmr_u8 if_set_wins(cmr_u32 index, cmr_u32 start_x, cmr_u32 start_y, cmr_u32 end_x, cmr_u32 end_y, void *cookie)
{
	af_ctrl_t *af = cookie;
	roi_info_t *roi = &af->roi;

	if (0 == index)
		roi->num = 1;
	else
		roi->num = roi->num + 1;

	if (roi->num <= sizeof(roi->win) / sizeof(roi->win[0])) {
		roi->win[roi->num - 1].start_x = start_x;
		roi->win[roi->num - 1].start_y = start_y;
		roi->win[roi->num - 1].end_x = end_x;
		roi->win[roi->num - 1].end_y = end_y;
		ISP_LOGI("if_set_wins %d %d %d %d %d", roi->num - 1, roi->win[roi->num - 1].start_x, roi->win[roi->num - 1].start_y, roi->win[roi->num - 1].end_x, roi->win[roi->num - 1].end_y);
	}

	return 0;
}

static cmr_u8 if_get_win_info(cmr_u32 * hw_num, cmr_u32 * isp_w, cmr_u32 * isp_h, void *cookie)
{
	af_ctrl_t *af = cookie;

	*isp_w = af->isp_info.width;
	*isp_h = af->isp_info.height;
	*hw_num = af->isp_info.win_num;
	return 0;
}

static cmr_u8 if_statistics_wait_cal_done(void *cookie)
{
	UNUSED(cookie);
	return 0;
}

static cmr_u8 if_statistics_get_data(cmr_u64 fv[T_TOTAL_FILTER_TYPE], _af_stat_data_t * p_stat_data, void *cookie)
{
	af_ctrl_t *af = cookie;
	cmr_u64 spsmd[MAX_ROI_NUM];
	cmr_u64 sum = 0;
	memset(fv, 0, sizeof(fv[0]) * T_TOTAL_FILTER_TYPE);
	memset(&(spsmd[0]), 0, sizeof(cmr_u64) * MAX_ROI_NUM);
	afm_get_fv(af, spsmd, ENHANCED_BIT, af->roi.num);

	switch (af->state) {
	case STATE_FAF:
		sum = spsmd[0] + spsmd[1] + spsmd[2] + spsmd[3] + 8 * spsmd[4] + spsmd[5] + spsmd[6] + spsmd[7] + spsmd[8];
		fv[T_SPSMD] = sum;
		break;
	default:
		sum = spsmd[9];			//3///3x3 windows,the 9th window is biggest covering all the other window
		//sum = spsmd[1] + 8 * spsmd[2];        /// the 0th window cover 1st and 2nd window,1st window cover 2nd window
		fv[T_SPSMD] = sum;
		break;
	}

	if (p_stat_data) {			// for caf calc
		p_stat_data->roi_num = af->roi.num;
		p_stat_data->stat_num = FOCUS_STAT_DATA_NUM;
		p_stat_data->p_stat = &(af->af_fv_val.af_fv0[0]);
	}
	ISP_LOGV("[%d][%d]spsmd sum %" PRIu64 "", af->state, af->roi.num, sum);

	return 0;
}

static cmr_u8 if_statistics_set_data(cmr_u32 set_stat, void *cookie)
{
	af_ctrl_t *af = cookie;

	af->afm_tuning.fv0_e = (set_stat & 0x0f);
	af->afm_tuning.fv1_e = (set_stat & 0xf0) >> 4;
	af->afm_tuning.nr_mode = (set_stat & 0xff00) >> 8;
	af->afm_tuning.cw_mode = (set_stat & 0xff0000) >> 16;
	af->afm_tuning.iir_level = (set_stat & 0xff000000) >> 24;

	ISP_LOGI("[0x%x] fv0e %d, fv1e %d, nr %d, cw %d iir %d", set_stat, af->afm_tuning.fv0_e, af->afm_tuning.fv1_e, af->afm_tuning.nr_mode, af->afm_tuning.cw_mode, af->afm_tuning.iir_level);
	afm_setup(af);
	return 0;
}

static cmr_u8 if_lens_get_pos(cmr_u16 * pos, void *cookie)
{
	af_ctrl_t *af = cookie;
	*pos = lens_get_pos(af);
	return 0;
}

static cmr_u8 if_lens_move_to(cmr_u16 pos, void *cookie)
{
	af_ctrl_t *af = cookie;
	AF_Timestamp timestamp;

	af->vcm_timestamp = get_systemtime_ns();
	timestamp.type = AF_TIME_VCM;
	timestamp.time_stamp = af->vcm_timestamp;
	af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_Set_Time_Stamp, &timestamp);
	lens_move_to(af, pos);
	return 0;
}

static cmr_u8 if_lens_wait_stop(void *cookie)
{
	UNUSED(cookie);
	return 0;
}

static cmr_u8 if_lock_partial_ae(cmr_u32 lock, void *cookie)
{
	af_ctrl_t *af = cookie;
	ISP_LOGV("%s, lock_num = %d", LOCK == lock ? "lock" : "unlock", af->ae_partial_lock_num);

	if (LOCK == ! !lock) {
		if (0 == af->ae_partial_lock_num) {
			af->cb_ops.lock_module(af->caller, AF_LOCKER_AE_CAF);
			af->ae_partial_lock_num++;
		}
	} else {
		if (af->ae_partial_lock_num) {
			af->cb_ops.unlock_module(af->caller, AF_LOCKER_AE_CAF);
			af->ae_partial_lock_num--;
		}
	}

	return 0;
}

static cmr_u8 if_lock_ae(e_LOCK lock, void *cookie)
{
	af_ctrl_t *af = cookie;
	ISP_LOGV("%s, lock_num = %d", LOCK == lock ? "lock" : "unlock", af->ae_lock_num);

	if (LOCK == lock) {
		if (0 == af->ae_lock_num) {
			af->cb_ops.lock_module(af->caller, AF_LOCKER_AE);
			af->ae_lock_num++;
		}
	} else {
		if (af->ae_lock_num) {
			af->cb_ops.unlock_module(af->caller, AF_LOCKER_AE);
			af->ae_lock_num--;
		}
	}

	return 0;
}

static cmr_u8 if_lock_awb(e_LOCK lock, void *cookie)
{
	af_ctrl_t *af = cookie;
	ISP_LOGV("%s, lock_num = %d", LOCK == lock ? "lock" : "unlock", af->awb_lock_num);

	if (LOCK == lock) {
		if (0 == af->awb_lock_num) {
			af->cb_ops.lock_module(af->caller, AF_LOCKER_AWB);
			af->awb_lock_num++;
		}
	} else {
		if (af->awb_lock_num) {
			af->cb_ops.unlock_module(af->caller, AF_LOCKER_AWB);
			af->awb_lock_num--;
		}
	}

	return 0;
}

static cmr_u8 if_lock_lsc(e_LOCK lock, void *cookie)
{
	af_ctrl_t *af = cookie;
	ISP_LOGV("%s, lock_num = %d", LOCK == lock ? "lock" : "unlock", af->lsc_lock_num);

	if (LOCK == lock) {
		if (0 == af->lsc_lock_num) {
			af->cb_ops.lock_module(af->caller, AF_LOCKER_LSC);
			af->lsc_lock_num++;
		}
	} else {
		if (af->lsc_lock_num) {
			af->cb_ops.unlock_module(af->caller, AF_LOCKER_LSC);
			af->lsc_lock_num--;
		}
	}

	return 0;
}

static cmr_u8 if_lock_nlm(e_LOCK lock, void *cookie)
{
	af_ctrl_t *af = cookie;
	ISP_LOGV("%s, lock_num = %d", LOCK == lock ? "lock" : "unlock", af->nlm_lock_num);

	if (LOCK == lock) {
		if (0 == af->nlm_lock_num) {
			af->cb_ops.lock_module(af->caller, AF_LOCKER_NLM);
			af->nlm_lock_num++;
		}
	} else {
		if (af->nlm_lock_num) {
			af->cb_ops.unlock_module(af->caller, AF_LOCKER_NLM);
			af->nlm_lock_num--;
		}
	}

	return 0;
}

static cmr_u8 if_get_sys_time(cmr_u64 * time, void *cookie)
{
	af_ctrl_t *af = (af_ctrl_t *) cookie;
	cmr_u32 sec, usec;

	af->cb_ops.af_get_system_time(af->caller, &sec, &usec);
	*time = (cmr_u64) sec *1000000000 + (cmr_u64) usec *1000;
	// *time = get_systemtime_ns();
	return 0;
}

static cmr_u8 if_sys_sleep_time(cmr_u16 sleep_time, void *cookie)
{
	UNUSED(cookie);
	// ISP_LOGV("vcm_timestamp %lld ms", (cmr_s64) af->vcm_timestamp);
	usleep(sleep_time * 1000);
	return 0;
}

static cmr_u8 if_get_ae_report(AE_Report * rpt, void *cookie)
{
	af_ctrl_t *af = (af_ctrl_t *) cookie;
	ae_info_t *ae = &af->ae;
	cmr_u32 line_time = ae->ae_report.line_time;
	cmr_u32 frame_len = ae->ae_report.frame_line;
	cmr_u32 dummy_line = ae->ae_report.cur_dummy;
	cmr_u32 exp_line = ae->ae_report.cur_exp_line;
	cmr_u32 frame_time;

	rpt->bAEisConverge = ae->ae_report.is_stab;
	rpt->AE_BV = ae->ae_report.bv;
	rpt->AE_EXP = (exp_line * line_time) / 10000;	// 0.1us -> ms
	rpt->AE_Gain = ae->ae_report.cur_again;
	rpt->AE_Pixel_Sum = af->Y_sum_normalize;

	frame_len = (frame_len > (exp_line + dummy_line)) ? frame_len : (exp_line + dummy_line);
	frame_time = frame_len * line_time;
	frame_time = frame_time > 0 ? frame_time : 1;
	if (0 == ((exp_line + dummy_line) * line_time)) {
		ISP_LOGI("Get wrong exposure info , exp_line %d line_time %d", (exp_line + dummy_line), line_time);
	}
	rpt->cur_fps = (1000000000 / MAX(1, (exp_line + dummy_line) * line_time));
	rpt->cur_lum = ae->ae_report.cur_lum;
	rpt->cur_index = ae->ae_report.cur_index;
	rpt->cur_ev = ae->ae_report.cur_ev;
	rpt->cur_iso = ae->ae_report.cur_iso;
	rpt->target_lum = ae->ae_report.target_lum;
	rpt->target_lum_ori = ae->ae_report.target_lum_ori;
	rpt->flag4idx = ae->ae_report.flag4idx;
	rpt->bisFlashOn = af->flash_on;
	return 0;
}

static cmr_u8 if_set_af_exif(const void *data, void *cookie)
{
	af_ctrl_t *af = cookie;
	UNUSED(data);
	property_get("persist.sys.camera.isp.af.dump", af->AF_MODE, "none");

	if (0 == strcmp(af->AF_MODE, "on")) {
		FILE *fp = NULL;
		if (STATE_NORMAL_AF == af->state)
			fp = fopen("/data/misc/cameraserver/saf_debug_info.jpg", "wb");
		else
			fp = fopen("/data/misc/cameraserver/caf_debug_info.jpg", "wb");

		if (NULL == fp) {
			ISP_LOGE("dump af_debug_info failure");
			return 0;
		}
		fwrite("ISP_AF__", 1, strlen("ISP_AF__"), fp);
		fwrite(af->af_alg_cxt, 1, af->af_dump_info_len, fp);
		fclose(fp);
	}

	return 0;
}

static cmr_u8 if_get_otp(AF_OTP_Data * pAF_OTP, void *cookie)
{
	af_ctrl_t *af = cookie;

	if (af->otp_info.rdm_data.macro_cali > af->otp_info.rdm_data.infinite_cali) {
		pAF_OTP->bIsExist = (T_LENS_BY_OTP);
		pAF_OTP->INF = af->otp_info.rdm_data.infinite_cali;
		pAF_OTP->MACRO = af->otp_info.rdm_data.macro_cali;
		ISP_LOGI("get otp (infi,macro) = (%d,%d)", pAF_OTP->INF, pAF_OTP->MACRO);
	} else {
		ISP_LOGW("skip invalid otp (infi,macro) = (%d,%d)", af->otp_info.rdm_data.infinite_cali, af->otp_info.rdm_data.macro_cali);
	}

	return 0;
}

static cmr_u8 if_get_motor_pos(cmr_u16 * motor_pos, void *cookie)
{
	af_ctrl_t *af = cookie;
	if (NULL != motor_pos) {
		*motor_pos = lens_get_pos(af);
	}

	return 0;
}

static cmr_u8 if_set_motor_sacmode(void *cookie)
{
	af_ctrl_t *af = cookie;

	if (NULL != af->cb_ops.af_set_motor_bestmode)
		af->cb_ops.af_set_motor_bestmode(af->caller);

	return 0;
}

static cmr_u8 if_binfile_is_exist(cmr_u8 * bisExist, void *cookie)
{
	UNUSED(cookie);

	ISP_LOGI("Enter");
	*bisExist = 0;
	ISP_LOGI("Exit");
	return 0;
}

static cmr_u8 if_af_log(const char *format, ...)
{
	va_list arg;
	va_start(arg, format);
	vsnprintf(AFlog_buffer, 2048, format, arg);
	va_end(arg);
	ISP_LOGI("ISP_AFv1: %s", AFlog_buffer);

	return 0;
}

static cmr_u8 if_af_start_notify(eAF_MODE AF_mode, void *cookie)
{
	af_ctrl_t *af = cookie;
	roi_info_t *r = &af->roi;
	cmr_u32 i;
	UNUSED(AF_mode);

	AF_Roi af_roi;
	for (i = 0; i < r->num; ++i) {
		af_roi.index = i;
		af_roi.start_x = r->win[i].start_x;
		af_roi.start_y = r->win[i].start_y;
		af_roi.end_x = r->win[i].end_x;
		af_roi.end_y = r->win[i].end_y;
		af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_Record_Wins, &af_roi);
	}

	return 0;
}

static cmr_u8 if_af_end_notify(eAF_MODE AF_mode, void *cookie)
{
	UNUSED(AF_mode);
	UNUSED(cookie);
	return 0;
}

static cmr_u8 if_phase_detection_get_data(pd_algo_result_t * pd_result, void *cookie)
{
	af_ctrl_t *af = cookie;

	memcpy(pd_result, &(af->pd), sizeof(pd_algo_result_t));
	// pd_result->pd_roi_dcc = 17;

	return 0;
}

static cmr_u8 if_motion_sensor_get_data(motion_sensor_result_t * ms_result, void *cookie)
{
	af_ctrl_t *af = cookie;

	if (NULL == ms_result) {
		return 1;
	}

	ms_result->g_sensor_queue[SENSOR_X_AXIS][ms_result->sensor_g_queue_cnt] = af->gsensor_info.vertical_up;
	ms_result->g_sensor_queue[SENSOR_Y_AXIS][ms_result->sensor_g_queue_cnt] = af->gsensor_info.vertical_down;
	ms_result->g_sensor_queue[SENSOR_Z_AXIS][ms_result->sensor_g_queue_cnt] = af->gsensor_info.horizontal;
	ms_result->timestamp = af->gsensor_info.timestamp;

	return 0;
}

// SharkLE Only ++
// helper function
// copy the original lens_move_to()
static void lens_move_to_sharkle(af_ctrl_t * af, cmr_u16 pos)
{
	// ISP_LOGD(" lens_move_to_sharkle, pos= %d",pos);

	cmr_u16 last_pos = 0;

	if (NULL == af->cb_ops.af_set_next_vcm_pos) {
		ISP_LOGE("af->af_set_next_vcm_pos null error");
		return;
	}

	last_pos = lens_get_pos(af);
	ISP_LOGD(" lens_move_to_sharkle, last_pos= %d", last_pos);

	if (last_pos != pos) {
		af->cb_ops.af_set_next_vcm_pos(af->caller, pos);
		af->lens.pos = pos;
	} else {
		ISP_LOGV("pos %d was set last time", pos);
	}
}

// original driver new support
static cmr_u8 if_af_set_pulse_line(cmr_u32 line, void *cookie)
{

	af_ctrl_t *af = cookie;
	ISP_LOGD(" if_af_set_pulse_line = %d", line);

	if (NULL != af->cb_ops.af_set_pulse_line)
		af->cb_ops.af_set_pulse_line(af->caller, line);

	return 0;
}

// copy the original if_lens_move_to
static cmr_u8 if_af_set_next_vcm_pos(cmr_u32 pos, void *cookie)
{
	ISP_LOGD(" if_af_set_next_vcm_pos, pos= %d", pos);

	af_ctrl_t *af = cookie;
	AF_Timestamp timestamp;
	af->vcm_timestamp = get_systemtime_ns();
	timestamp.type = AF_TIME_VCM;
	timestamp.time_stamp = af->vcm_timestamp;
	af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_Set_Time_Stamp, &timestamp);
	lens_move_to_sharkle(af, pos);
	return 0;

	/*
	   af_ctrl_t *af = cookie;
	   if (NULL != af->af_set_next_vcm_pos)
	   af->af_set_next_vcm_pos(af->caller, pos);

	   return 0;
	 */
}

static cmr_u8 if_af_set_pulse_log(cmr_u32 flag, void *cookie)
{

	af_ctrl_t *af = cookie;
	ISP_LOGD(" if_af_set_pulse_log = %d", flag);

	if (NULL != af->cb_ops.af_set_pulse_log)
		af->cb_ops.af_set_pulse_log(af->caller, flag);

	return 0;
}

static cmr_u8 if_af_set_clear_next_vcm_pos(void *cookie)
{

	af_ctrl_t *af = cookie;

	if (NULL != af->cb_ops.af_set_clear_next_vcm_pos)
		af->cb_ops.af_set_clear_next_vcm_pos(af->caller);

	return 0;
}

// SharkLE Only --

// trigger stuffs
#define LOAD_SYMBOL(handle, sym, name) \
{sym=dlsym(handle, name); if(NULL==sym) {ISP_LOGE("dlsym fail: %s", name); return -1;}}

static cmr_s32 load_trigger_symbols(af_ctrl_t * af)
{
	LOAD_SYMBOL(af->trig_lib, af->trig_ops.init, "caf_trigger_init");
	LOAD_SYMBOL(af->trig_lib, af->trig_ops.deinit, "caf_trigger_deinit");
	LOAD_SYMBOL(af->trig_lib, af->trig_ops.calc, "caf_trigger_calculation");
	LOAD_SYMBOL(af->trig_lib, af->trig_ops.ioctrl, "caf_trigger_ioctrl");
	return 0;
}

static cmr_s32 load_trigger_lib(af_ctrl_t * af, const char *name)
{
	af->trig_lib = dlopen(name, RTLD_NOW);

	if (NULL == af->trig_lib) {
		ISP_LOGE("fail to load af trigger lib%s", name);
		return -1;
	}

	if (0 != load_trigger_symbols(af)) {
		dlclose(af->trig_lib);
		af->trig_lib = NULL;
		return -1;
	}

	return 0;
}

static cmr_s32 unload_trigger_lib(af_ctrl_t * af)
{
	if (af->trig_lib) {
		dlclose(af->trig_lib);
		af->trig_lib = NULL;
	}
	return 0;
}

static cmr_u8 if_aft_binfile_is_exist(cmr_u8 * is_exist, void *cookie)
{

	af_ctrl_t *af = cookie;
	char *aft_tuning_path = "/data/misc/cameraserver/aft_tuning.bin";
	FILE *fp = NULL;

	if (0 == access(aft_tuning_path, R_OK)) {	// read request successs
		cmr_s32 len = 0;

		fp = fopen(aft_tuning_path, "rb");
		if (NULL == fp) {
			*is_exist = 0;
			return 0;
		}

		fseek(fp, 0, SEEK_END);
		len = ftell(fp);
		if (len < 0 || (cmr_u32) len != af->trig_ops.handle.tuning_param_len) {
			ISP_LOGW("aft_tuning.bin len dismatch with aft_alg len %d", af->trig_ops.handle.tuning_param_len);
			fclose(fp);
			*is_exist = 0;
			return 0;
		}

		fclose(fp);
		*is_exist = 1;
	} else {
		*is_exist = 0;
	}
	return 0;
}

static cmr_u8 if_is_aft_mlog(cmr_u32 * is_save, void *cookie)
{
	UNUSED(cookie);
	char value[PROPERTY_VALUE_MAX] = { '\0' };

	property_get(AF_SAVE_MLOG_STR, value, "no");

	if (!strcmp(value, "save")) {
		*is_save = 1;
	}
	ISP_LOGV("is_save %d", *is_save);
	return 0;
}

static cmr_u8 if_aft_log(cmr_u32 log_level, const char *format, ...)
{
	va_list arg;
	va_start(arg, format);
	vsnprintf(AFlog_buffer, 2048, format, arg);
	va_end(arg);
	switch (log_level) {
	case AFT_LOG_VERBOSE:
		ALOGV("%s", AFlog_buffer);
		break;
	case AFT_LOG_DEBUG:
		ALOGD("%s", AFlog_buffer);
		break;
	case AFT_LOG_INFO:
		ALOGI("%s", AFlog_buffer);
		break;
	case AFT_LOG_WARN:
		ALOGW("%s", AFlog_buffer);
		break;
	case AFT_LOG_ERROR:
		ALOGE("%s", AFlog_buffer);
		break;
	default:
		ISP_LOGV("default log level not support");
		break;
	}

	return 0;
}

static cmr_s32 load_af_symbols(af_ctrl_t * af)
{
	LOAD_SYMBOL(af->af_lib, af->af_ops.init, "AF_init");
	LOAD_SYMBOL(af->af_lib, af->af_ops.deinit, "AF_deinit");
	LOAD_SYMBOL(af->af_lib, af->af_ops.calc, "AF_Process_Frame");
	LOAD_SYMBOL(af->af_lib, af->af_ops.ioctrl, "AF_IOCtrl_process");
	return 0;
}

static cmr_s32 load_af_lib(af_ctrl_t * af, const char *name)
{
	af->af_lib = dlopen(name, RTLD_NOW);

	if (NULL == af->af_lib) {
		cmr_u32 i = 0;
		while (i < sizeof(libafv1_path) / sizeof(libafv1_path[0]) && NULL == af->af_lib) {
			af->af_lib = dlopen(libafv1_path[i], RTLD_NOW);
			i++;
		}
		if (NULL == af->af_lib) {
			ISP_LOGE("fail to load af lib%s", name);
			return -1;
		}
	}

	if (0 != load_af_symbols(af)) {
		dlclose(af->af_lib);
		af->af_lib = NULL;
		return -1;
	}

	return 0;
}

static cmr_s32 unload_af_lib(af_ctrl_t * af)
{
	if (af->af_lib) {
		dlclose(af->af_lib);
		af->af_lib = NULL;
	}
	return 0;
}

/* initialization */
static void *af_init(af_ctrl_t * af)
{
	// tuning data from common_mode
	af_tuning_block_param af_tuning_data;
	AF_Ctrl_Ops AF_Ops;
	struct isp_haf_tune_param *pdaf_tune_data;
	haf_tuning_param_t haf_tuning_data;
	void *alg_cxt = NULL;
	cmr_u32 i;

	AF_Ops.cookie = af;
	AF_Ops.statistics_wait_cal_done = if_statistics_wait_cal_done;
	AF_Ops.statistics_get_data = if_statistics_get_data;
	AF_Ops.statistics_set_data = if_statistics_set_data;
	AF_Ops.lens_get_pos = if_lens_get_pos;
	AF_Ops.lens_move_to = if_lens_move_to;
	AF_Ops.lens_wait_stop = if_lens_wait_stop;
	AF_Ops.lock_ae = if_lock_ae;
	AF_Ops.lock_awb = if_lock_awb;
	AF_Ops.lock_lsc = if_lock_lsc;
	AF_Ops.get_sys_time = if_get_sys_time;
	AF_Ops.sys_sleep_time = if_sys_sleep_time;
	AF_Ops.get_ae_report = if_get_ae_report;
	AF_Ops.set_af_exif = if_set_af_exif;
	AF_Ops.get_otp_data = if_get_otp;
	AF_Ops.get_motor_pos = if_get_motor_pos;
	AF_Ops.set_motor_sacmode = if_set_motor_sacmode;
	AF_Ops.binfile_is_exist = if_binfile_is_exist;
	AF_Ops.af_log = if_af_log;
	AF_Ops.af_start_notify = if_af_start_notify;
	AF_Ops.af_end_notify = if_af_end_notify;
	AF_Ops.phase_detection_get_data = if_phase_detection_get_data;
	AF_Ops.motion_sensor_get_data = if_motion_sensor_get_data;
	AF_Ops.set_wins = if_set_wins;
	AF_Ops.get_win_info = if_get_win_info;
	AF_Ops.lock_ae_partial = if_lock_partial_ae;

	// SharkLE Only ++
	AF_Ops.set_pulse_line = if_af_set_pulse_line;
	AF_Ops.set_next_vcm_pos = if_af_set_next_vcm_pos;
	AF_Ops.set_pulse_log = if_af_set_pulse_log;
	AF_Ops.set_clear_next_vcm_pos = if_af_set_clear_next_vcm_pos;
	// SharkLE Only --

	memset((void *)&af_tuning_data, 0, sizeof(af_tuning_data));
	af_tuning_data.data = af->aftuning_data;
	af_tuning_data.data_len = af->aftuning_data_len;

	pdaf_tune_data = (struct isp_haf_tune_param *)af->pdaftuning_data;
	if (pdaf_tune_data != NULL) {
		ISP_LOGI("PDAF Tuning 0[%d] 1[%d] 14[%d] ", pdaf_tune_data->isp_pdaf_tune_data[0].min_pd_vcm_steps, pdaf_tune_data->isp_pdaf_tune_data[0].max_pd_vcm_steps,
				 pdaf_tune_data->isp_pdaf_tune_data[0].pd_conf_thr_2nd);
		for (i = 0; i < 3; i++) {
			haf_tuning_data.PDAF_Tuning_Data[i].min_pd_vcm_steps = pdaf_tune_data->isp_pdaf_tune_data[i].min_pd_vcm_steps;
			haf_tuning_data.PDAF_Tuning_Data[i].max_pd_vcm_steps = pdaf_tune_data->isp_pdaf_tune_data[i].max_pd_vcm_steps;
			haf_tuning_data.PDAF_Tuning_Data[i].coc_range = pdaf_tune_data->isp_pdaf_tune_data[i].coc_range;
			haf_tuning_data.PDAF_Tuning_Data[i].far_tolerance = pdaf_tune_data->isp_pdaf_tune_data[i].far_tolerance;
			haf_tuning_data.PDAF_Tuning_Data[i].near_tolerance = pdaf_tune_data->isp_pdaf_tune_data[i].near_tolerance;
			haf_tuning_data.PDAF_Tuning_Data[i].err_limit = pdaf_tune_data->isp_pdaf_tune_data[i].err_limit;
			haf_tuning_data.PDAF_Tuning_Data[i].pd_converge_thr = pdaf_tune_data->isp_pdaf_tune_data[i].pd_converge_thr;
			haf_tuning_data.PDAF_Tuning_Data[i].pd_converge_thr_2nd = pdaf_tune_data->isp_pdaf_tune_data[i].pd_converge_thr_2nd;
			haf_tuning_data.PDAF_Tuning_Data[i].pd_focus_times_thr = pdaf_tune_data->isp_pdaf_tune_data[i].pd_focus_times_thr;
			haf_tuning_data.PDAF_Tuning_Data[i].pd_thread_sync_frm = pdaf_tune_data->isp_pdaf_tune_data[i].pd_thread_sync_frm;
			haf_tuning_data.PDAF_Tuning_Data[i].pd_thread_sync_frm_init = pdaf_tune_data->isp_pdaf_tune_data[i].pd_thread_sync_frm_init;
			haf_tuning_data.PDAF_Tuning_Data[i].min_process_frm = pdaf_tune_data->isp_pdaf_tune_data[i].min_process_frm;
			haf_tuning_data.PDAF_Tuning_Data[i].max_process_frm = pdaf_tune_data->isp_pdaf_tune_data[i].max_process_frm;
			haf_tuning_data.PDAF_Tuning_Data[i].pd_conf_thr = pdaf_tune_data->isp_pdaf_tune_data[i].pd_conf_thr;
			haf_tuning_data.PDAF_Tuning_Data[i].pd_conf_thr_2nd = pdaf_tune_data->isp_pdaf_tune_data[i].pd_conf_thr_2nd;
		}
	} else {
		ISP_LOGI("PDAF Tuning NULL!");
		haf_tuning_data.PDAF_Tuning_Data[0].min_pd_vcm_steps = 1;	//Use Default Setting
	}

	if (0 != load_af_lib(af, AF_LIB))
		return NULL;

	alg_cxt = af->af_ops.init(&AF_Ops, &af_tuning_data, &haf_tuning_data, &af->af_dump_info_len, AF_SYS_VERSION);
	return alg_cxt;
}

static cmr_s32 trigger_init(af_ctrl_t * af, const char *lib_name)
{
	struct aft_tuning_block_param aft_in;
	char value[PROPERTY_VALUE_MAX] = { '\0' };

	if (0 != load_trigger_lib(af, lib_name))
		return -1;

	if (NULL == af->afttuning_data || 0 == af->afttuning_data_len) {
		ISP_LOGW("aft tuning param error ");
		aft_in.data_len = 0;
		aft_in.data = NULL;
	} else {
		ISP_LOGI("aft tuning param ok ");
		aft_in.data_len = af->afttuning_data_len;
		aft_in.data = af->afttuning_data;

		property_get(AF_SAVE_MLOG_STR, value, "no");
		if (!strcmp(value, "save")) {
			FILE *fp = NULL;
			fp = fopen("/data/misc/cameraserver/aft_tuning_params.bin", "wb");
			fwrite(aft_in.data, 1, aft_in.data_len, fp);
			fclose(fp);
			ISP_LOGV("aft tuning size = %d", aft_in.data_len);
		}
	}
	af->trig_ops.handle.is_multi_mode = af->is_multi_mode;
	af->trig_ops.handle.aft_ops.aft_cookie = af;
	af->trig_ops.handle.aft_ops.get_sys_time = if_get_sys_time;
	af->trig_ops.handle.aft_ops.binfile_is_exist = if_aft_binfile_is_exist;
	af->trig_ops.handle.aft_ops.is_aft_mlog = if_is_aft_mlog;
	af->trig_ops.handle.aft_ops.aft_log = if_aft_log;

	af->trig_ops.init(&aft_in, &af->trig_ops.handle);
	af->trig_ops.ioctrl(&af->trig_ops.handle, AFT_CMD_GET_AE_SKIP_INFO, &af->trig_ops.ae_skip_info, NULL);
	return 0;
}

static cmr_s32 trigger_set_mode(af_ctrl_t * af, enum aft_mode mode)
{
	af->trig_ops.ioctrl(&af->trig_ops.handle, AFT_CMD_SET_AF_MODE, &mode, NULL);
	return 0;
}

static cmr_s32 trigger_start(af_ctrl_t * af)
{
	af->trig_ops.ioctrl(&af->trig_ops.handle, AFT_CMD_SET_CAF_RESET, NULL, NULL);
	return 0;
}

static cmr_s32 trigger_stop(af_ctrl_t * af)
{
	af->trig_ops.ioctrl(&af->trig_ops.handle, AFT_CMD_SET_CAF_STOP, NULL, NULL);
	return 0;
}

static cmr_s32 trigger_calc(af_ctrl_t * af, struct aft_proc_calc_param *prm, struct aft_proc_result *res)
{
	af->trig_ops.calc(&af->trig_ops.handle, prm, res);
	return 0;
}

static cmr_s32 trigger_deinit(af_ctrl_t * af)
{
	af->trig_ops.deinit(&af->trig_ops.handle);
	unload_trigger_lib(af);
	return 0;
}

// test mode
static void set_manual(af_ctrl_t * af, char *test_param)
{
	UNUSED(test_param);
	af->state = STATE_MANUAL;
	af->focus_state = AF_IDLE;
	// property_set("af_set_pos","0");// to fix lens to position 0
	trigger_stop(af);

	ISP_LOGV("Now is in ISP_FOCUS_MANUAL mode");
	ISP_LOGV("pls adb shell setprop \"af_set_pos\" 0~1023 to fix lens position");
}

static void trigger_caf(af_ctrl_t * af, char *test_param)
{
	AF_Trigger_Data aft_in;

	char *p1 = test_param;
	char *p2;
	char *p3;
	property_set("af_set_pos", "none");

	while (*p1 != '~' && *p1 != '\0')
		p1++;
	*p1++ = '\0';
	p2 = p1;
	while (*p2 != '~' && *p2 != '\0')
		p2++;
	*p2++ = '\0';
	p3 = p2;
	while (*p3 != '~' && *p3 != '\0')
		p3++;
	*p3++ = '\0';
	memset(&aft_in, 0, sizeof(AF_Trigger_Data));
	af->request_mode = AF_MODE_NORMAL;	//not need trigger to work when caf_start_monitor
	af->state = STATE_CAF;
	af->focus_state = AF_SEARCHING;
	af->algo_mode = CAF;
	aft_in.AFT_mode = af->algo_mode;
	aft_in.bisTrigger = AF_TRIGGER;
	aft_in.AF_Trigger_Type = atoi(test_param);
	aft_in.defocus_param.scan_from = (atoi(p1) > 0 && atoi(p1) < 1023) ? (atoi(p1)) : (0);
	aft_in.defocus_param.scan_to = (atoi(p2) > 0 && atoi(p2) < 1023) ? (atoi(p2)) : (0);
	aft_in.defocus_param.per_steps = (atoi(p3) > 0 && atoi(p3) < 200) ? (atoi(p3)) : (0);

	trigger_stop(af);
	af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_TRIGGER, &aft_in);	//test_param is in _eAF_Triger_Type,     RF_NORMAL = 0,        //noraml R/F search for AFT RF_FAST = 3,              //Fast R/F search for AFT
	do_start_af(af);
}

static void af_stop_search(af_ctrl_t * af);
static void trigger_saf(af_ctrl_t * af, char *test_param)
{
	AF_Trigger_Data aft_in;
	UNUSED(test_param);
	property_set("af_set_pos", "none");

	memset(&aft_in, 0, sizeof(AF_Trigger_Data));
	af->request_mode = AF_MODE_NORMAL;
	af->state = STATE_NORMAL_AF;
	trigger_set_mode(af, AFT_MODE_NORMAL);
	trigger_stop(af);
	if (AF_SEARCHING == af->focus_state) {
		af_stop_search(af);
	}
	//af->defocus = (1 == atoi(test_param))? (1):(af->defocus);
	//saf_start(af, NULL);  //SAF, win is NULL using default
	//ISP_LOGV("_eAF_Triger_Type = %d", (1 == af->defocus) ? DEFOCUS : RF_NORMAL);
	af->algo_mode = SAF;
	aft_in.AFT_mode = af->algo_mode;
	aft_in.bisTrigger = AF_TRIGGER;
	// aft_in.AF_Trigger_Type = (1 == af->defocus) ? DEFOCUS : RF_NORMAL;
	aft_in.AF_Trigger_Type = DEFOCUS;
	af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_TRIGGER, &aft_in);
	do_start_af(af);
	af->vcm_stable = 0;
	af->focus_state = AF_SEARCHING;
}

static void calibration_ae_mean(af_ctrl_t * af, char *test_param)
{
	FILE *fp = fopen("/data/misc/cameraserver/calibration_ae_mean.txt", "ab");
	cmr_u8 i = 0;
	cmr_u16 pos = 0;

	UNUSED(test_param);
	if_lock_lsc(LOCK, af);
	if_lock_ae(LOCK, af);
	if_statistics_get_data(af->fv_combine, NULL, af);
	pos = lens_get_pos(af);
	ISP_LOGV("VCM registor pos :%d", pos);
	af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_Record_Vcm_Pos, &pos);
	af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_Record_FV, &af->fv_combine[T_SPSMD]);
	for (i = 0; i < 9; i++) {
		ISP_LOGV
			("pos %d AE_MEAN_WIN_%d R %d G %d B %d r_avg_all %d g_avg_all %d b_avg_all %d FV %" PRIu64 "\n",
			 pos, i, af->ae_cali_data.r_avg[i], af->ae_cali_data.g_avg[i],
			 af->ae_cali_data.b_avg[i], af->ae_cali_data.r_avg_all, af->ae_cali_data.g_avg_all, af->ae_cali_data.b_avg_all, af->fv_combine[T_SPSMD]);
		fprintf(fp,
				"pos %d AE_MEAN_WIN_%d R %d G %d B %d r_avg_all %d g_avg_all %d b_avg_all %d FV %" PRIu64 "\n",
				pos, i, af->ae_cali_data.r_avg[i], af->ae_cali_data.g_avg[i],
				af->ae_cali_data.b_avg[i], af->ae_cali_data.r_avg_all, af->ae_cali_data.g_avg_all, af->ae_cali_data.b_avg_all, af->fv_combine[T_SPSMD]);
	}
	fclose(fp);

}

static void set_vcm_mode(af_ctrl_t * af, char *vcm_mode)
{
	if (NULL != af->cb_ops.af_set_test_vcm_mode)
		af->cb_ops.af_set_test_vcm_mode(af->caller, vcm_mode);

	return;
}

static void get_vcm_mode(af_ctrl_t * af, char *vcm_mode)
{
	UNUSED(vcm_mode);
	if (NULL != af->cb_ops.af_get_test_vcm_mode)
		af->cb_ops.af_get_test_vcm_mode(af->caller);

	return;
}

static void lock_block(af_ctrl_t * af, char *block)
{
	cmr_u32 lock = 0;

	lock = atoi(block);

	if (lock & LOCK_AE)
		if_lock_ae(LOCK, af);
	if (lock & LOCK_LSC)
		if_lock_lsc(LOCK, af);
	if (lock & LOCK_NLM)
		if_lock_nlm(LOCK, af);
	if (lock & LOCK_AWB)
		if_lock_awb(LOCK, af);

	return;
}

static void trigger_defocus(af_ctrl_t * af, char *test_param)
{
	char *p1 = test_param;

	while (*p1 != '~' && *p1 != '\0')
		p1++;
	*p1++ = '\0';

	af->defocus = atoi(test_param);
	ISP_LOGV("af->defocus : %d \n", af->defocus);

	return;
}

static void set_roi(af_ctrl_t * af, char *test_param)
{
	char *p1 = NULL;
	char *p2 = NULL;
	char *string = NULL;
	cmr_s32 len = 0;
	cmr_u32 read_len = 0;
	cmr_u8 num = 0;
	roi_info_t *r = &af->roi;
	FILE *fp = NULL;
	UNUSED(test_param);

	if (0 == access("/data/misc/cameraserver/AF_roi.bin", R_OK)) {
		fp = fopen("/data/misc/cameraserver/AF_roi.bin", "rb");
		if (NULL == fp) {
			ISP_LOGI("open file AF_roi.bin fails");
			return;
		}

		fseek(fp, 0, SEEK_END);
		len = ftell(fp);
		if (len < 0) {
			ISP_LOGI("fail to get offset");
			fclose(fp);
			return;
		}
		string = malloc(len);
		if (NULL == string) {
			ISP_LOGI("malloc len of file AF_roi.bin fails");
			fclose(fp);
			return;
		}
		fseek(fp, 0, SEEK_SET);
		read_len = fread(string, 1, len, fp);
		if (read_len != (cmr_u32) len) {
			ISP_LOGE("fail to read bin.");
		}
		fclose(fp);
		// parsing argumets start
		p1 = p2 = string;
		while (*p2 != '~' && *p2 != '\0')
			p2++;
		*p2++ = '\0';

		r->num = atoi(p1);

		num = 0;
		while (num < r->num) {	// set AF ROI
			p1 = p2;
			while (*p2 != '~' && *p2 != '\0')
				p2++;
			*p2++ = '\0';
			r->win[num].start_x = atoi(p1);
			r->win[num].start_x = (r->win[num].start_x >> 1) << 1;
			p1 = p2;
			while (*p2 != '~' && *p2 != '\0')
				p2++;
			*p2++ = '\0';
			r->win[num].start_y = atoi(p1);
			r->win[num].start_y = (r->win[num].start_y >> 1) << 1;
			p1 = p2;
			while (*p2 != '~' && *p2 != '\0')
				p2++;
			*p2++ = '\0';
			r->win[num].end_x = atoi(p1);
			r->win[num].end_x = (r->win[num].end_x >> 1) << 1;
			p1 = p2;
			while (*p2 != '~' && *p2 != '\0')
				p2++;
			*p2++ = '\0';
			r->win[num].end_y = atoi(p1);
			r->win[num].end_y = (r->win[num].end_y >> 1) << 1;
			ISP_LOGI("ROI %d win,(startx,starty,endx,endy) = (%d,%d,%d,%d)", num, r->win[num].start_x, r->win[num].start_y, r->win[num].end_x, r->win[num].end_y);
			num++;
		}
		// parsing argumets end
		if (NULL != string)
			free(string);

	} else {
		ISP_LOGI("file AF_roi.bin doesn't exist");
		return;
	}

	return;
}

static test_mode_command_t test_mode_set[] = {
	{"ISP_FOCUS_MANUAL", 0, &set_manual},
	{"ISP_FOCUS_CAF", 0, &trigger_caf},
	{"ISP_FOCUS_SAF", 0, &trigger_saf},
	{"ISP_FOCUS_CALIBRATION_AE_MEAN", 0, &calibration_ae_mean},
	{"ISP_FOCUS_VCM_SET_MODE", 0, &set_vcm_mode},
	{"ISP_FOCUS_VCM_GET_MODE", 0, &get_vcm_mode},
	{"ISP_FOCUS_LOCK_BLOCK", 0, &lock_block},
	{"ISP_FOCUS_DEFOCUS", 0, &trigger_defocus},
	{"ISP_FOCUS_SET_ROI", 0, &set_roi},
	{"ISP_DEFAULT", 0, NULL},
};

static void set_af_test_mode(af_ctrl_t * af, char *af_mode)
{
#define CALCULATE_KEY(string,string_const) key=1; \
	while( *string!='~' && *string!='\0' ){ \
		key=key+*string; \
		string++; \
	} \
	if( 0==string_const ) \
		*string = '\0';

	char *p1 = af_mode;
	cmr_u64 key = 0, i = 0;

	CALCULATE_KEY(p1, 0);

	while (i < sizeof(test_mode_set) / sizeof(test_mode_set[0])) {
		ISP_LOGV("command,key,target_key:%s,%" PRIu64 " %" PRIu64 "", test_mode_set[i].command, test_mode_set[i].key, key);
		if (key == test_mode_set[i].key)
			break;
		i++;
	}

	if (sizeof(test_mode_set) / sizeof(test_mode_set[0]) <= i) {	// out of range in test mode,so initialize its ops
		ISP_LOGV("AF test mode Command is undefined,start af test mode initialization");
		i = 0;
		while (i < sizeof(test_mode_set) / sizeof(test_mode_set[0])) {
			p1 = test_mode_set[i].command;
			CALCULATE_KEY(p1, 1);
			test_mode_set[i].key = key;
			ISP_LOGV("command,key:%s,%" PRIu64 "", test_mode_set[i].command, test_mode_set[i].key);
			i++;
		}
		set_manual(af, NULL);
		return;
	}

	if (NULL != test_mode_set[i].command_func)
		test_mode_set[i].command_func(af, p1 + 1);
}

/* called each frame */
static cmr_s32 af_test_lens(af_ctrl_t * af, cmr_u16 pos)
{
	cmr_u32 force_stop;

	force_stop = AFV1_TRUE;
	af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_STOP, &force_stop);
	af->af_ops.calc(af->af_alg_cxt);

	ISP_LOGV("af_pos_set3 %d", pos);
	lens_move_to(af, pos);
	ISP_LOGV("af_pos_set4 %d", pos);
	return 0;
}

// non-zsl,easy for motor moving and capturing
static void *loop_for_test_mode(void *data_client)
{
	af_ctrl_t *af = NULL;
	char AF_MODE[PROPERTY_VALUE_MAX] = { '\0' };
	char AF_POS[PROPERTY_VALUE_MAX] = { '\0' };
	af = data_client;

	while (0 == af->test_loop_quit) {
		property_get("af_mode", AF_MODE, "none");
		ISP_LOGV("test AF_MODE %s", AF_MODE);
		if (0 != strcmp(AF_MODE, "none") && 0 != strcmp(AF_MODE, "ISP_DEFAULT")) {
			set_af_test_mode(af, AF_MODE);
			property_set("af_mode", "ISP_DEFAULT");
		}
		property_get("af_set_pos", AF_POS, "none");
		ISP_LOGV("test AF_POS %s", AF_POS);
		if (0 != strcmp(AF_POS, "none")) {
			af_test_lens(af, (cmr_u16) atoi(AF_POS));
		}
	}
	af->test_loop_quit = 1;

	ISP_LOGV("test mode loop quit");

	return 0;
}

// af process functions
static void faf_start(af_ctrl_t * af, struct af_trig_info *win)
{
	AF_Trigger_Data aft_in;
	af->algo_mode = FAF;
	memset(&aft_in, 0, sizeof(AF_Trigger_Data));
	aft_in.AFT_mode = af->algo_mode;
	aft_in.bisTrigger = AF_TRIGGER;
	aft_in.AF_Trigger_Type = (RF_NORMAL);
	calc_roi(af, win, af->algo_mode);
	af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_TRIGGER, &aft_in);
	do_start_af(af);
	af->vcm_stable = 0;
	notify_start(af, AF_FOCUS_FAF);
}

static cmr_s32 faf_process_frame(af_ctrl_t * af)
{
	cmr_u32 alg_mode;
	AF_Result af_result;

	af->af_ops.calc(af->af_alg_cxt);
	af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_Get_Alg_Mode, &alg_mode);
	if (Wait_Trigger == alg_mode) {
		af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_Get_Result, &af_result);

		notify_stop(af, HAVE_PEAK == af_result.AF_Result ? 1 : 0, AF_FOCUS_FAF);
		ISP_LOGI("notify_stop, result = %d mode = %d ", af_result.AF_Result, af_result.af_mode);
		return 1;
	} else {
		return 0;
	}
}

static void saf_start(af_ctrl_t * af, struct af_trig_info *win)
{
	AF_Trigger_Data aft_in;
	af->algo_mode = SAF;
	memset(&aft_in, 0, sizeof(AF_Trigger_Data));
	aft_in.AFT_mode = af->algo_mode;
	aft_in.bisTrigger = AF_TRIGGER;
	aft_in.AF_Trigger_Type = (1 == af->defocus) ? (DEFOCUS) : (RF_NORMAL);
	calc_roi(af, win, af->algo_mode);
	af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_TRIGGER, &aft_in);
	do_start_af(af);
	af->vcm_stable = 0;
}

static cmr_s32 saf_process_frame(af_ctrl_t * af)
{
	cmr_u32 alg_mode;
	AF_Result af_result;
	af->af_ops.calc(af->af_alg_cxt);
	af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_Get_Alg_Mode, &alg_mode);
	if (Wait_Trigger == alg_mode) {
		af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_Get_Result, &af_result);

		notify_stop(af, HAVE_PEAK == af_result.AF_Result ? 1 : 0, AF_FOCUS_SAF);
		ISP_LOGI("notify_stop, result = %d mode = %d ", af_result.AF_Result, af_result.af_mode);
		return 1;
	} else {
		return 0;
	}
}

static void caf_start(af_ctrl_t * af, struct aft_proc_result *p_aft_result)
{
	char value[PROPERTY_VALUE_MAX] = { '\0' };
	AF_Trigger_Data aft_in;
	char *token = NULL;
	cmr_u32 scan_from = 0;
	cmr_u32 scan_to = 0;
	cmr_u32 per_steps = 0;

	property_get("persist.sys.caf.enable", value, "1");
	if (atoi(value) != 1)
		return;

	memset(&aft_in, 0, sizeof(AF_Trigger_Data));
	af->algo_mode = STATE_CAF == af->state ? CAF : VAF;
	ISP_LOGI("current af->algo_mode %d", af->algo_mode);
	calc_roi(af, NULL, af->algo_mode);

	aft_in.AFT_mode = af->algo_mode;
	aft_in.bisTrigger = AF_TRIGGER;
	aft_in.trigger_source = p_aft_result->is_caf_trig;
	property_get("persist.sys.isp.caf.defocus", value, "0");
	if (atoi(value) == 0) {
		aft_in.AF_Trigger_Type = (p_aft_result->is_need_rough_search) ? (RF_NORMAL) : (RF_FAST);
	} else {
		token = strtok(value, ":");
		if (token != NULL) {
			scan_from = atoi(token);
			token = strtok(NULL, ":");
		}

		if (token != NULL) {
			scan_to = atoi(token);
			token = strtok(NULL, ":");
		}

		if (token != NULL) {
			per_steps = atoi(token);
		}
		ISP_LOGI("scan_from %d, scan_to %d, per_steps %d,", scan_from, scan_to, per_steps);
		aft_in.AF_Trigger_Type = DEFOCUS;
		aft_in.defocus_param.scan_from = (scan_from > 0 && scan_from < 1023) ? scan_from : 0;
		aft_in.defocus_param.scan_to = (scan_to > 0 && scan_to < 1023) ? scan_to : 0;
		aft_in.defocus_param.per_steps = (per_steps > 0 && per_steps < 200) ? per_steps : 0;
	}
	af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_TRIGGER, &aft_in);
	do_start_af(af);
	if (AFT_TRIG_CB == p_aft_result->is_caf_trig) {
		af->cb_trigger = AFV1_TRUE;
		notify_start(af, AF_FOCUS_CAF);
	}
	af->vcm_stable = 0;
}

static cmr_s32 caf_process_frame(af_ctrl_t * af)
{
	cmr_u32 alg_mode;
	AF_Result af_result;

	af->af_ops.calc(af->af_alg_cxt);
	af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_Get_Alg_Mode, &alg_mode);
	if (Wait_Trigger == alg_mode) {
		af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_Get_Result, &af_result);
		ISP_LOGI("result = %d mode = %d ", af_result.AF_Result, af_result.af_mode);

		if (AFV1_TRUE == af->cb_trigger) {
			notify_stop(af, HAVE_PEAK == af_result.AF_Result ? 1 : 0, AF_FOCUS_CAF);
			ISP_LOGI("notify_stop.");
			af->cb_trigger = AFV1_FALSE;
		}
		return 1;
	} else {
		return 0;
	}
}

static void af_stop_search(af_ctrl_t * af)
{
	cmr_u32 force_stop;

	force_stop = AFV1_TRUE;
	ISP_LOGI("focus_state = %s", FOCUS_STATE_STR(af->focus_state));
	af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_STOP, &force_stop);	//modifiy for force stop to SAF/Flow control
	af->af_ops.calc(af->af_alg_cxt);
	af->focus_state = AF_IDLE;
}

static void caf_monitor_trigger(af_ctrl_t * af, struct aft_proc_calc_param *prm, struct aft_proc_result *result)
{
	struct af_trig_info win;

	if (AF_SEARCHING != af->focus_state) {
		if (result->is_caf_trig || (AFV1_TRUE == af->force_trigger && af->ae.ae_report.is_stab)) {
			ISP_LOGI("lib trigger af %d, force trigger %d", result->is_caf_trig, af->force_trigger);
			if (AFV1_TRUE == af->force_trigger) {
				trigger_start(af);
			}
			if (AFT_DATA_FD == prm->active_data_type) {
				win.win_num = 1;
				win.win_pos[0].sx = prm->fd_info.face_info[0].sx;
				win.win_pos[0].ex = prm->fd_info.face_info[0].ex;
				win.win_pos[0].sy = prm->fd_info.face_info[0].sy;
				win.win_pos[0].ey = prm->fd_info.face_info[0].ey;
				ISP_LOGI("face win num %d, x:%d y:%d e_x:%d e_y:%d", win.win_num, win.win_pos[0].sx, win.win_pos[0].sy, win.win_pos[0].ex, win.win_pos[0].ey);
				af->pre_state = af->state;
				af->state = STATE_FAF;
				faf_start(af, &win);
			} else {
				caf_start(af, result);
			}
			af->focus_state = AF_SEARCHING;
			af->force_trigger = (AFV1_FALSE);
		} else if (result->is_cancel_caf) {
			cmr_u32 alg_mode;
			af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_Get_Alg_Mode, &alg_mode);
			ISP_LOGW("cancel af while not searching AF_mode = %d", alg_mode);
		}
	} else {
		if (result->is_cancel_caf || result->is_caf_trig || AFV1_TRUE == af->force_trigger) {
			AF_Trigger_Data aft_in;
			cmr_u32 force_stop;
			ISP_LOGI("af retrigger, cancel af %d, trigger af %d, force trigger %d", result->is_cancel_caf, result->is_caf_trig, af->force_trigger);
			memset(&aft_in, 0, sizeof(AF_Trigger_Data));
			aft_in.AFT_mode = af->algo_mode;
			aft_in.bisTrigger = AF_TRIGGER;
			aft_in.AF_Trigger_Type = (RE_TRIGGER);
			aft_in.trigger_source = result->is_caf_trig;
			force_stop = AFV1_FALSE;

			if (AFV1_SUCCESS == af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_STOP, &force_stop)) {
				af->af_ops.calc(af->af_alg_cxt);
				win.win_num = 0;
				if (AFT_DATA_FD == prm->active_data_type) {
					win.win_num = 1;
					win.win_pos[0].sx = prm->fd_info.face_info[0].sx;
					win.win_pos[0].ex = prm->fd_info.face_info[0].ex;
					win.win_pos[0].sy = prm->fd_info.face_info[0].sy;
					win.win_pos[0].ey = prm->fd_info.face_info[0].ey;
					ISP_LOGI("retrigger face win num %d, x:%d y:%d e_x:%d e_y:%d", win.win_num, win.win_pos[0].sx, win.win_pos[0].sy, win.win_pos[0].ex, win.win_pos[0].ey);
					af->algo_mode = FAF;
					af->state = STATE_FAF;
				}
				calc_roi(af, &win, af->algo_mode);
				af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_TRIGGER, &aft_in);
				do_start_af(af);
				ISP_LOGI("AF retrigger start \n");
			} else {
				cmr_u32 alg_mode;
				af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_Get_Alg_Mode, &alg_mode);
				ISP_LOGI("AF retrigger no support @%d \n", alg_mode);
			}
		}
	}
}

static void caf_monitor_calc(af_ctrl_t * af, struct aft_proc_calc_param *prm)
{
	struct aft_proc_result res;
	memset(&res, 0, sizeof(res));

	trigger_calc(af, prm, &res);
	ISP_LOGV("is_caf_trig = %d, is_cancel_caf = %d, is_need_rough_search = %d", res.is_caf_trig, res.is_cancel_caf, res.is_need_rough_search);

	if ((0 == af->flash_on) && (STATE_CAF == af->state || STATE_RECORD_CAF == af->state)) {
		caf_monitor_trigger(af, prm, &res);
	}
}

static void caf_monitor_process_af(af_ctrl_t * af)
{
	cmr_u64 fv[10];
	struct aft_proc_calc_param *prm = &(af->prm_trigger);

	memset(fv, 0, sizeof(fv));
	memset(prm, 0, sizeof(struct aft_proc_calc_param));
	afm_get_fv(af, fv, ENHANCED_BIT, af->roi.num);

	prm->afm_info.win_cfg.win_cnt = af->roi.num;
	memcpy(prm->afm_info.win_cfg.win_pos, af->roi.win, af->roi.num * sizeof(win_coord_t));	// be carefull that the af->roi.num should be less than af->roi.win number and prm->afm_info.win_cfg.win_pos number
	prm->afm_info.filter_info.filter_num = 1;
	prm->afm_info.filter_info.filter_data[0].data = fv;
	prm->afm_info.filter_info.filter_data[0].type = 1;
	prm->active_data_type = AFT_DATA_AF;

	caf_monitor_calc(af, prm);
}

#define CALC_HIST(sum, gain, pixels, awb_gain, max, hist) \
{cmr_u64 v=((cmr_u64)(sum)*(gain))/((cmr_u64)(pixels)*(awb_gain)); \
v=v>(max)?(max):v; hist[v]++;}

static void calc_histogram(af_ctrl_t * af, isp_awb_statistic_hist_info_t * stat)
{
	cmr_u32 gain_r = af->awb.r_gain;
	cmr_u32 gain_g = af->awb.g_gain;
	cmr_u32 gain_b = af->awb.b_gain;
	cmr_u32 pixels = af->ae.win_size;
	cmr_u32 awb_base_gain = 1024;
	cmr_u32 max_value = 255;
	cmr_u32 i, j;

	if (pixels < 1)
		return;

	memset(stat->r_hist, 0, sizeof(stat->r_hist));
	memset(stat->g_hist, 0, sizeof(stat->g_hist));
	memset(stat->b_hist, 0, sizeof(stat->b_hist));

	cmr_u32 ae_skip_line = 0;
	if (af->trig_ops.ae_skip_info.ae_select_support) {
		ae_skip_line = af->trig_ops.ae_skip_info.ae_skip_line;
	}

	for (i = ae_skip_line; i < (32 - ae_skip_line); ++i) {
		for (j = ae_skip_line; j < (32 - ae_skip_line); ++j) {
			CALC_HIST(stat->r_info[32 * i + j], gain_r, pixels, awb_base_gain, max_value, stat->r_hist);
			CALC_HIST(stat->g_info[32 * i + j], gain_g, pixels, awb_base_gain, max_value, stat->g_hist);
			CALC_HIST(stat->b_info[32 * i + j], gain_b, pixels, awb_base_gain, max_value, stat->b_hist);
		}
	}
}

static void caf_monitor_process_ae(af_ctrl_t * af, const struct af_ae_calc_out *ae, isp_awb_statistic_hist_info_t * stat)
{
	struct aft_proc_calc_param *prm = &(af->prm_trigger);
	ISP_LOGV("focus_state = %s", FOCUS_STATE_STR(af->focus_state));

	memset(prm, 0, sizeof(struct aft_proc_calc_param));

	calc_histogram(af, stat);

	prm->active_data_type = AFT_DATA_IMG_BLK;
	prm->img_blk_info.block_w = 32;
	prm->img_blk_info.block_h = 32;
	prm->img_blk_info.pix_per_blk = af->ae.win_size;
	prm->img_blk_info.chn_num = 3;
	prm->img_blk_info.data = (cmr_u32 *) stat;
	prm->ae_info.exp_time = ae->cur_exp_line * ae->line_time / 10;
	prm->ae_info.gain = ae->cur_again;
	prm->ae_info.cur_lum = ae->cur_lum;
	prm->ae_info.target_lum = ae->target_lum;
	prm->ae_info.is_stable = ae->is_stab;
	prm->ae_info.bv = ae->bv;
	prm->ae_info.y_sum = af->Y_sum_trigger;
	prm->ae_info.cur_scene = OUT_SCENE;
	prm->ae_info.registor_pos = (cmr_u32) lens_get_pos(af);
	//ISP_LOGI("exp_time = %d, gain = %d, cur_lum = %d, is_stable = %d, bv = %d", prm->ae_info.exp_time, prm->ae_info.gain, prm->ae_info.cur_lum, prm->ae_info.is_stable, prm->ae_info.bv);

	caf_monitor_calc(af, prm);
}

static void caf_monitor_process_sensor(af_ctrl_t * af, struct afctrl_sensor_info_t *in)
{
	struct afctrl_sensor_info_t *aux_sensor_info = (struct afctrl_sensor_info_t *)in;
	uint32_t sensor_type = aux_sensor_info->type;
	struct aft_proc_calc_param *prm = &(af->prm_trigger);

	memset(prm, 0, sizeof(struct aft_proc_calc_param));
	switch (sensor_type) {
	case AF_SENSOR_ACCELEROMETER:
		prm->sensor_info.sensor_type = AFT_POSTURE_ACCELEROMETER;
		prm->sensor_info.x = aux_sensor_info->gsensor_info.vertical_down;
		prm->sensor_info.y = aux_sensor_info->gsensor_info.vertical_up;
		prm->sensor_info.z = aux_sensor_info->gsensor_info.horizontal;
		break;
	case AF_SENSOR_GYROSCOPE:
		prm->sensor_info.sensor_type = AFT_POSTURE_GYRO;
		prm->sensor_info.x = aux_sensor_info->gyro_info.x;
		prm->sensor_info.y = aux_sensor_info->gyro_info.y;
		prm->sensor_info.z = aux_sensor_info->gyro_info.z;
		break;
	default:
		break;
	}
	prm->active_data_type = AFT_DATA_SENSOR;
	ISP_LOGV("[%d] sensor type %d %f %f %f ", af->state, prm->sensor_info.sensor_type, prm->sensor_info.x, prm->sensor_info.y, prm->sensor_info.z);
	caf_monitor_calc(af, prm);
}

static void caf_monitor_process_phase_diff(af_ctrl_t * af)
{
	struct aft_proc_calc_param *prm = &(af->prm_trigger);

	memset(prm, 0, sizeof(struct aft_proc_calc_param));
	prm->active_data_type = AFT_DATA_PD;
	prm->pd_info.pd_enable = af->pd.pd_enable;
	prm->pd_info.effective_frmid = af->pd.effective_frmid;
	prm->pd_info.pd_roi_num = af->pd.pd_roi_num;
	memcpy(&(prm->pd_info.confidence[0]), &(af->pd.confidence[0]), sizeof(cmr_u32) * (MIN(af->pd.pd_roi_num, PD_MAX_AREA)));
	memcpy(&(prm->pd_info.pd_value[0]), &(af->pd.pd_value[0]), sizeof(double) * (MIN(af->pd.pd_roi_num, PD_MAX_AREA)));
	memcpy(&(prm->pd_info.pd_roi_dcc[0]), &(af->pd.pd_roi_dcc[0]), sizeof(cmr_u32) * (MIN(af->pd.pd_roi_num, PD_MAX_AREA)));
	prm->comm_info.otp_inf_pos = af->otp_info.rdm_data.infinite_cali;
	prm->comm_info.otp_macro_pos = af->otp_info.rdm_data.macro_cali;
	prm->comm_info.registor_pos = (cmr_u32) lens_get_pos(af);
	ISP_LOGV("[%d] pd data in ", prm->pd_info.effective_frmid);
	caf_monitor_calc(af, prm);

	return;
}

static void caf_monitor_process_fd(af_ctrl_t * af)
{

	struct aft_proc_calc_param *prm = &(af->prm_trigger);
	cmr_u8 i = 0;

	memset(prm, 0, sizeof(struct aft_proc_calc_param));
	prm->active_data_type = AFT_DATA_FD;

	i = sizeof(prm->fd_info.face_info) / sizeof(prm->fd_info.face_info[0]);
	prm->fd_info.face_num = i > af->face_info.face_num ? af->face_info.face_num : i;

	i = 0;
	while (i < prm->fd_info.face_num) {
		prm->fd_info.face_info[i].sx = af->face_info.face_info[i].sx;
		prm->fd_info.face_info[i].ex = af->face_info.face_info[i].ex;
		prm->fd_info.face_info[i].sy = af->face_info.face_info[i].sy;
		prm->fd_info.face_info[i].ey = af->face_info.face_info[i].ey;
		i++;
	}
	prm->fd_info.frame_width = af->face_info.frame_width;
	prm->fd_info.frame_height = af->face_info.frame_height;

	caf_monitor_calc(af, prm);
}

static void caf_monitor_process(af_ctrl_t * af)
{
	if (af->trigger_source_type & AF_DATA_FD) {
		af->trigger_source_type &= (~AF_DATA_FD);
		caf_monitor_process_fd(af);
	}

	if (af->trigger_source_type & AF_DATA_PD) {
		af->trigger_source_type &= (~AF_DATA_PD);
		caf_monitor_process_phase_diff(af);
	}

	if (af->trigger_source_type & AF_DATA_AF) {
		af->trigger_source_type &= (~AF_DATA_AF);
		caf_monitor_process_af(af);
	}

	if (af->trigger_source_type & AF_DATA_AE) {
		af->trigger_source_type &= (~AF_DATA_AE);
		caf_monitor_process_ae(af, &(af->ae.ae_report), &(af->rgb_stat));
	}

	if (af->trigger_source_type & AF_DATA_G) {
		struct afctrl_sensor_info_t aux_sensor_info;
		aux_sensor_info.type = AF_SENSOR_ACCELEROMETER;
		aux_sensor_info.gsensor_info.vertical_up = af->gsensor_info.vertical_up;
		aux_sensor_info.gsensor_info.vertical_down = af->gsensor_info.vertical_down;
		aux_sensor_info.gsensor_info.horizontal = af->gsensor_info.horizontal;
		aux_sensor_info.gsensor_info.timestamp = af->gsensor_info.timestamp;
		caf_monitor_process_sensor(af, &aux_sensor_info);
		af->trigger_source_type &= (~AF_DATA_G);
	}

	return;
}

// af ioctrl functions
static cmr_s32 af_sprd_set_af_mode(cmr_handle handle, void *param0)
{
	af_ctrl_t *af = (af_ctrl_t *) handle;
	cmr_u32 af_mode = *(cmr_u32 *) param0;
	cmr_s32 rtn = AFV1_SUCCESS;
	enum aft_mode mode;
	cmr_u16 pos = 0;

	ISP_LOGI("af state = %s, focus state = %s, set af_mode = %d", STATE_STRING(af->state), FOCUS_STATE_STR(af->focus_state), af_mode);
	property_get("af_mode", af->AF_MODE, "none");
	if (0 != strcmp(af->AF_MODE, "none")) {
		ISP_LOGI("AF_MODE %s is not null, af test mode", af->AF_MODE);
		pos = lens_get_pos(af);
		ISP_LOGV("VCM registor pos :%d", pos);
		af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_Record_Vcm_Pos, &pos);
		return rtn;
	}

	af->pre_state = af->state;
	switch (af_mode) {
	case AF_MODE_NORMAL:
		af->force_trigger = (AFV1_FALSE);
		af->request_mode = af_mode;
		af->state = STATE_NORMAL_AF;
		trigger_set_mode(af, AFT_MODE_NORMAL);
		trigger_stop(af);
		break;
	case AF_MODE_CONTINUE:
	case AF_MODE_VIDEO:
		af->request_mode = af_mode;
		if (AF_SEARCHING == af->focus_state) {
			ISP_LOGI("last af was not done, af state %s, pre_state %s", STATE_STRING(af->state), STATE_STRING(af->pre_state));
			af_stop_search(af);
		}
		af->state = AF_MODE_CONTINUE == af_mode ? STATE_CAF : STATE_RECORD_CAF;
		af->algo_mode = STATE_CAF == af->state ? CAF : VAF;
		calc_roi(af, NULL, af->algo_mode);
		do_start_af(af);
		mode = STATE_CAF == af->state ? AFT_MODE_CONTINUE : AFT_MODE_VIDEO;
		trigger_set_mode(af, mode);
		trigger_start(af);
		break;
	case AF_MODE_PICTURE:
		break;
	case AF_MODE_FULLSCAN:
		af->request_mode = af_mode;
		af->state = STATE_FULLSCAN;
		trigger_set_mode(af, AFT_MODE_NORMAL);
		trigger_stop(af);
		break;
	default:
		ISP_LOGW("af_mode %d is not supported", af_mode);
		break;
	}

	return rtn;
}

static cmr_s32 af_sprd_set_af_trigger(cmr_handle handle, void *param0)
{
	af_ctrl_t *af = (af_ctrl_t *) handle;
	struct af_trig_info *win = (struct af_trig_info *)param0;	//win = (struct isp_af_win *)param;
	AF_Trigger_Data aft_in;
	cmr_s32 rtn = AFV1_SUCCESS;

	ISP_LOGI("trigger af state = %s", STATE_STRING(af->state));
	property_set("af_mode", "none");
	af->test_loop_quit = 1;

	if (AF_SEARCHING == af->focus_state) {
		ISP_LOGI("last af was not done, af state %s, pre_state %s", STATE_STRING(af->state), STATE_STRING(af->pre_state));
		af_stop_search(af);
	}

	if (STATE_FULLSCAN == af->state) {
		af->algo_mode = CAF;
		memset(&aft_in, 0, sizeof(AF_Trigger_Data));
		aft_in.AFT_mode = af->algo_mode;
		aft_in.bisTrigger = AF_TRIGGER;
		aft_in.AF_Trigger_Type = BOKEH;
		af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_TRIGGER, &aft_in);
		do_start_af(af);
	} else {
		saf_start(af, win);
	}
	af->focus_state = AF_SEARCHING;
	ISP_LOGI("saf start done \n");
	return rtn;
}

static cmr_s32 af_sprd_set_af_cancel(cmr_handle handle, void *param0)
{
	UNUSED(param0);
	af_ctrl_t *af = (af_ctrl_t *) handle;
	cmr_s32 rtn = AFV1_SUCCESS;
	ISP_LOGI("cancel af state = %s", STATE_STRING(af->state));

	return rtn;
}

static cmr_s32 af_sprd_set_af_bypass(cmr_handle handle, void *param0)
{
	af_ctrl_t *af = (af_ctrl_t *) handle;
	char value[PROPERTY_VALUE_MAX] = { '\0' };
	cmr_s32 rtn = AFV1_SUCCESS;

	if (NULL == param0) {
		ISP_LOGE("param null error");
		rtn = AFV1_ERROR;
		return rtn;
	}

	property_get("persist.sys.isp.af.bypass", value, "0");
	if (atoi(value) == 0) {
		ISP_LOGI("param = %d", *(cmr_u32 *) param0);
		af->bypass = *(cmr_u32 *) param0;
	} else {
		ISP_LOGI("af bypass cmd is NOT allowed %d", atoi(value));
	}

	return rtn;
}

static cmr_s32 af_sprd_set_flash_notice(cmr_handle handle, void *param0)
{
	af_ctrl_t *af = (af_ctrl_t *) handle;
	cmr_u32 flash_status = *(cmr_u32 *) param0;
	cmr_s32 rtn = AFV1_SUCCESS;

	ISP_LOGV("flash_status %u", flash_status);
	switch (flash_status) {
	case AF_FLASH_PRE_BEFORE:
	case AF_FLASH_PRE_LIGHTING:
	case AF_FLASH_MAIN_BEFORE:
	case AF_FLASH_MAIN_LIGHTING:
		if (0 == af->flash_on) {
			af->flash_on = 1;
		}
		break;
	case AF_FLASH_MAIN_AFTER:
	case AF_FLASH_PRE_AFTER:
		if (1 == af->flash_on) {
			af->flash_on = 0;
		}
		break;
	default:
		break;
	}

	return rtn;
}

static void ae_calc_win_size(af_ctrl_t * af, struct afctrl_fwstart_info *param)
{
	cmr_u32 w, h;
	if (param->size.w && param->size.h) {
		w = ((param->size.w / 32) >> 1) << 1;
		h = ((param->size.h / 32) >> 1) << 1;
		af->ae.win_size = w * h;
	} else {
		af->ae.win_size = 1;
	}
}

static cmr_s32 af_sprd_set_video_start(cmr_handle handle, void *param0)
{
	af_ctrl_t *af = (af_ctrl_t *) handle;
	struct afctrl_fwstart_info *in_ptr = (struct afctrl_fwstart_info *)param0;

	ae_calc_win_size(af, in_ptr);
	af->isp_info.width = in_ptr->size.w;
	af->isp_info.height = in_ptr->size.h;
	ISP_LOGI("af state = %s, focus state = %s; image width = %d, height = %d", STATE_STRING(af->state), FOCUS_STATE_STR(af->focus_state), in_ptr->size.w, in_ptr->size.h);

	property_get("af_mode", af->AF_MODE, "none");
	if (0 != strcmp(af->AF_MODE, "none")) {
		ISP_LOGI("AF_MODE %s is not null, af test mode", af->AF_MODE);
		return AFV1_SUCCESS;
	}
	calc_roi(af, NULL, af->algo_mode);
	do_start_af(af);
	if (STATE_CAF == af->state || STATE_RECORD_CAF == af->state || STATE_NORMAL_AF == af->state) {
		trigger_start(af);		// for hdr capture no af mode update at whole procedure
	}

	if (STATE_RECORD_CAF == af->state) {
		af->force_trigger = AFV1_TRUE;
	}

	return AFV1_SUCCESS;
}

static cmr_s32 af_sprd_set_video_stop(cmr_handle handle, void *param0)
{
	UNUSED(param0);
	af_ctrl_t *af = (af_ctrl_t *) handle;
	ISP_LOGI("af state = %s, focus state = %s", STATE_STRING(af->state), FOCUS_STATE_STR(af->focus_state));

	if (STATE_CAF == af->state || STATE_RECORD_CAF == af->state || STATE_NORMAL_AF == af->state) {
		trigger_stop(af);
	}

	if (AF_SEARCHING == af->focus_state) {
		ISP_LOGW("serious problem, current af is not done, af state %s", STATE_STRING(af->state));
		af_stop_search(af);		//wait saf/caf done, caf : for camera enter, switch to manual; saf : normal non zsl
	}
	do_stop_af(af);
	return AFV1_SUCCESS;
}

static void ae_calibration(af_ctrl_t * af, struct af_img_blk_statistic *rgb)
{
	cmr_u32 i, j, r_sum[9], g_sum[9], b_sum[9];

	memset(r_sum, 0, sizeof(r_sum));
	memset(g_sum, 0, sizeof(g_sum));
	memset(b_sum, 0, sizeof(b_sum));

	for (i = 0; i < 32; i++) {
		for (j = 0; j < 32; j++) {
			r_sum[(i / 11) * 3 + j / 11] += rgb->r_info[i * 32 + j] / af->ae.win_size;
			g_sum[(i / 11) * 3 + j / 11] += rgb->g_info[i * 32 + j] / af->ae.win_size;
			b_sum[(i / 11) * 3 + j / 11] += rgb->b_info[i * 32 + j] / af->ae.win_size;
		}
	}
	af->ae_cali_data.r_avg[0] = r_sum[0] / 121;
	af->ae_cali_data.r_avg_all = af->ae_cali_data.r_avg[0];
	af->ae_cali_data.r_avg[1] = r_sum[1] / 121;
	af->ae_cali_data.r_avg_all += af->ae_cali_data.r_avg[1];
	af->ae_cali_data.r_avg[2] = r_sum[2] / 110;
	af->ae_cali_data.r_avg_all += af->ae_cali_data.r_avg[2];
	af->ae_cali_data.r_avg[3] = r_sum[3] / 121;
	af->ae_cali_data.r_avg_all += af->ae_cali_data.r_avg[3];
	af->ae_cali_data.r_avg[4] = r_sum[4] / 121;
	af->ae_cali_data.r_avg_all += af->ae_cali_data.r_avg[4];
	af->ae_cali_data.r_avg[5] = r_sum[5] / 110;
	af->ae_cali_data.r_avg_all += af->ae_cali_data.r_avg[5];
	af->ae_cali_data.r_avg[6] = r_sum[6] / 110;
	af->ae_cali_data.r_avg_all += af->ae_cali_data.r_avg[6];
	af->ae_cali_data.r_avg[7] = r_sum[7] / 110;
	af->ae_cali_data.r_avg_all += af->ae_cali_data.r_avg[7];
	af->ae_cali_data.r_avg[8] = r_sum[8] / 100;
	af->ae_cali_data.r_avg_all += af->ae_cali_data.r_avg[8];
	af->ae_cali_data.r_avg_all /= 9;

	af->ae_cali_data.g_avg[0] = g_sum[0] / 121;
	af->ae_cali_data.g_avg_all = af->ae_cali_data.g_avg[0];
	af->ae_cali_data.g_avg[1] = g_sum[1] / 121;
	af->ae_cali_data.g_avg_all += af->ae_cali_data.g_avg[1];
	af->ae_cali_data.g_avg[2] = g_sum[2] / 110;
	af->ae_cali_data.g_avg_all += af->ae_cali_data.g_avg[2];
	af->ae_cali_data.g_avg[3] = g_sum[3] / 121;
	af->ae_cali_data.g_avg_all += af->ae_cali_data.g_avg[3];
	af->ae_cali_data.g_avg[4] = g_sum[4] / 121;
	af->ae_cali_data.g_avg_all += af->ae_cali_data.g_avg[4];
	af->ae_cali_data.g_avg[5] = g_sum[5] / 110;
	af->ae_cali_data.g_avg_all += af->ae_cali_data.g_avg[5];
	af->ae_cali_data.g_avg[6] = g_sum[6] / 110;
	af->ae_cali_data.g_avg_all += af->ae_cali_data.g_avg[6];
	af->ae_cali_data.g_avg[7] = g_sum[7] / 110;
	af->ae_cali_data.g_avg_all += af->ae_cali_data.g_avg[7];
	af->ae_cali_data.g_avg[8] = g_sum[8] / 100;
	af->ae_cali_data.g_avg_all += af->ae_cali_data.g_avg[8];
	af->ae_cali_data.g_avg_all /= 9;

	af->ae_cali_data.b_avg[0] = b_sum[0] / 121;
	af->ae_cali_data.b_avg_all = af->ae_cali_data.b_avg[0];
	af->ae_cali_data.b_avg[1] = b_sum[1] / 121;
	af->ae_cali_data.b_avg_all += af->ae_cali_data.b_avg[1];
	af->ae_cali_data.b_avg[2] = b_sum[2] / 110;
	af->ae_cali_data.b_avg_all += af->ae_cali_data.b_avg[2];
	af->ae_cali_data.b_avg[3] = b_sum[3] / 121;
	af->ae_cali_data.b_avg_all += af->ae_cali_data.b_avg[3];
	af->ae_cali_data.b_avg[4] = b_sum[4] / 121;
	af->ae_cali_data.b_avg_all += af->ae_cali_data.b_avg[4];
	af->ae_cali_data.b_avg[5] = b_sum[5] / 110;
	af->ae_cali_data.b_avg_all += af->ae_cali_data.b_avg[5];
	af->ae_cali_data.b_avg[6] = b_sum[6] / 110;
	af->ae_cali_data.b_avg_all += af->ae_cali_data.b_avg[6];
	af->ae_cali_data.b_avg[7] = b_sum[7] / 110;
	af->ae_cali_data.b_avg_all += af->ae_cali_data.b_avg[7];
	af->ae_cali_data.b_avg[8] = b_sum[8] / 100;
	af->ae_cali_data.b_avg_all += af->ae_cali_data.b_avg[8];
	af->ae_cali_data.b_avg_all /= 9;

	ISP_LOGV("(r,g,b) in block4 is (%d,%d,%d)", af->ae_cali_data.r_avg[4], af->ae_cali_data.g_avg[4], af->ae_cali_data.b_avg[4]);
}

static void set_af_RGBY(af_ctrl_t * af, struct af_img_blk_statistic *rgb)
{
#define AE_BLOCK_W 32
#define AE_BLOCK_H 32

	cmr_u32 Y_sx = 0, Y_ex = 0, Y_sy = 0, Y_ey = 0, r_sum = 0, g_sum = 0, b_sum = 0, y_sum = 0;
	float ae_area;
	cmr_u16 width, height, i = 0, blockw, blockh, index;

	width = af->isp_info.width;
	height = af->isp_info.height;

	memcpy(&(af->rgb_stat.r_info[0]), rgb->r_info, sizeof(af->rgb_stat.r_info));
	memcpy(&(af->rgb_stat.g_info[0]), rgb->g_info, sizeof(af->rgb_stat.g_info));
	memcpy(&(af->rgb_stat.b_info[0]), rgb->b_info, sizeof(af->rgb_stat.b_info));

	af->roi_RGBY.num = af->roi.num;

	af->roi_RGBY.Y_sum[af->roi.num] = 0;
	for (i = 0; i < af->roi.num; i++) {
		Y_sx = af->roi.win[i].start_x / (width / AE_BLOCK_W);
		Y_ex = af->roi.win[i].end_x / (width / AE_BLOCK_W);
		Y_sy = af->roi.win[i].start_y / (height / AE_BLOCK_H);
		Y_ey = af->roi.win[i].end_y / (height / AE_BLOCK_H);
		// exception
		if (Y_sx == Y_ex)
			Y_ex = Y_sx + 1;
		// exception
		if (Y_sy == Y_ey)
			Y_ey = Y_sy + 1;

		r_sum = 0;
		g_sum = 0;
		b_sum = 0;
		for (blockw = Y_sx; blockw < Y_ex; blockw++) {
			for (blockh = Y_sy; blockh < Y_ey; blockh++) {
				index = blockh * AE_BLOCK_W + blockw;
				r_sum = r_sum + af->rgb_stat.r_info[index];
				g_sum = g_sum + af->rgb_stat.g_info[index];
				b_sum = b_sum + af->rgb_stat.b_info[index];
			}
		}

		ae_area = 1.0 * (Y_ex - Y_sx) * (Y_ey - Y_sy);
		y_sum = (((0.299 * r_sum) + (0.587 * g_sum) + (0.114 * b_sum)) / ae_area);
		af->roi_RGBY.R_sum[i] = r_sum;
		af->roi_RGBY.G_sum[i] = g_sum;
		af->roi_RGBY.B_sum[i] = b_sum;
		af->roi_RGBY.Y_sum[i] = y_sum;
		// ISP_LOGV("y_sum[%d] = %d",i,y_sum);
	}

	if (0 != af->roi.num) {
		switch (af->state) {
		case STATE_FAF:
			af->Y_sum_trigger = af->roi_RGBY.Y_sum[af->roi.num - 1];
			af->Y_sum_normalize = af->roi_RGBY.Y_sum[af->roi.num - 1];
			break;
		default:
			af->Y_sum_trigger = af->roi_RGBY.Y_sum[af->roi.num - 1];
			af->Y_sum_normalize = af->roi_RGBY.Y_sum[af->roi.num - 1];
			break;
		}
	}

	property_get("af_mode", af->AF_MODE, "none");
	if (0 != strcmp(af->AF_MODE, "none")) {	// test mode only
		ae_calibration(af, rgb);
	}

}

static cmr_s32 af_sprd_set_ae_info(cmr_handle handle, void *param0)
{
	af_ctrl_t *af = (af_ctrl_t *) handle;
	struct afctrl_ae_info *ae_info = (struct afctrl_ae_info *)param0;
	struct af_img_blk_statistic *ae_stat_ptr = (struct af_img_blk_statistic *)ae_info->img_blk_info.data;
	cmr_s32 rtn = AFV1_SUCCESS;
	if ((0 == af->isp_info.width) || (0 == af->isp_info.height)) {
		return AFV1_ERROR;
	}

	set_af_RGBY(af, (void *)ae_stat_ptr);
	memcpy(&(af->ae.ae_report), &(ae_info->ae_rlt_info), sizeof(struct af_ae_calc_out));
	af->trigger_source_type |= AF_DATA_AE;
	return rtn;
}

static cmr_s32 af_sprd_set_awb_info(cmr_handle handle, void *param0)
{
	af_ctrl_t *af = (af_ctrl_t *) handle;
	struct afctrl_awb_info *awb = (struct afctrl_awb_info *)param0;
	af->awb.r_gain = awb->r_gain;
	af->awb.g_gain = awb->g_gain;
	af->awb.b_gain = awb->b_gain;
	// af->trigger_source_type |= AF_DATA_AWB;
	return AFV1_SUCCESS;
}

static cmr_s32 af_sprd_set_face_detect(cmr_handle handle, void *param0)
{
	af_ctrl_t *af = (af_ctrl_t *) handle;
	struct afctrl_face_info *face = (struct afctrl_face_info *)param0;
	cmr_s32 rtn = AFV1_SUCCESS;
	if (NULL != face && 0 != face->face_num) {
		memcpy(&af->face_info, face, sizeof(struct afctrl_face_info));
		af->trigger_source_type |= AF_DATA_FD;
	}
	return rtn;
}

static cmr_s32 af_sprd_set_dcam_timestamp(cmr_handle handle, void *param0)
{
	af_ctrl_t *af = (af_ctrl_t *) handle;
	struct afctrl_ts_info *af_ts = (struct afctrl_ts_info *)param0;
	cmr_s32 timecompare = 0;
	cmr_u16 pos[2] = { 0 };
	AF_Timestamp timestamp;

	timecompare = compare_timestamp(af);
	if (0 == af_ts->capture) {
		af->dcam_timestamp = af_ts->timestamp;
		timestamp.type = AF_TIME_DCAM;
		timestamp.time_stamp = af->dcam_timestamp;
		af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_Set_Time_Stamp, &timestamp);
		//ISP_LOGI("dcam_timestamp %" PRIu64 " ", (cmr_s64) af->dcam_timestamp);
		if (AF_IDLE == af->focus_state && DCAM_AFTER_VCM_YES == timecompare && 0 == af->vcm_stable) {
			af->vcm_stable = 1;
		}
	} else if (1 == af_ts->capture) {
		af->takepic_timestamp = af_ts->timestamp;
		timestamp.type = AF_TIME_CAPTURE;
		timestamp.time_stamp = af->takepic_timestamp;
		af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_Set_Time_Stamp, &timestamp);
		//ISP_LOGI("takepic_timestamp %" PRIu64 " ", (cmr_s64) af->takepic_timestamp);
		ISP_LOGV("takepic_timestamp - vcm_timestamp =%" PRId64 " ms", ((cmr_s64) af->takepic_timestamp - (cmr_s64) af->vcm_timestamp) / 1000000);

		if (0 == af->ts_counter) {
			pos[0] = lens_get_pos(af);
			ISP_LOGI("VCM registor pos0 :%d", pos[0]);
			af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_Record_Vcm_Pos, &pos[0]);
		} else if (1 == af->ts_counter) {
			pos[1] = lens_get_pos(af);
			ISP_LOGI("VCM registor pos1 :%d", pos[1]);
		}
		af->ts_counter++;
		if (2 == af->ts_counter) {
			af->ts_counter = 0;
		}
	}

	return AFV1_SUCCESS;
}

static cmr_s32 af_sprd_set_pd_info(cmr_handle handle, void *param0)
{
	af_ctrl_t *af = (af_ctrl_t *) handle;
	struct pd_result *pd_calc_result = (struct pd_result *)param0;

	memset(&(af->pd), 0, sizeof(pd_algo_result_t));
	af->pd.effective_frmid = (cmr_u32) pd_calc_result->pdGetFrameID;
	af->pd.pd_enable = (af->pd.effective_frmid) ? 1 : 0;
	af->pd.pd_roi_num = pd_calc_result->pd_roi_num;
	// transfer full phase diff data value to algorithm
	memcpy(&(af->pd.confidence[0]), &(pd_calc_result->pdConf[0]), sizeof(cmr_u32) * (af->pd.pd_roi_num));
	memcpy(&(af->pd.pd_value[0]), &(pd_calc_result->pdPhaseDiff[0]), sizeof(double) * (af->pd.pd_roi_num));
	memcpy(&(af->pd.pd_roi_dcc[0]), &(pd_calc_result->pdDCCGain[0]), sizeof(cmr_u32) * (af->pd.pd_roi_num));
	af->trigger_source_type |= AF_DATA_PD;
	ISP_LOGV("PD\t%lf\t%lf\t%lf\t%lf\n", pd_calc_result->pdPhaseDiff[0], pd_calc_result->pdPhaseDiff[1], pd_calc_result->pdPhaseDiff[2], pd_calc_result->pdPhaseDiff[3]);
	ISP_LOGV("Conf\t%d\t%d\t%d\t%d Total [%d]\n", pd_calc_result->pdConf[0], pd_calc_result->pdConf[1], pd_calc_result->pdConf[2], pd_calc_result->pdConf[3], af->pd.pd_roi_num);
	ISP_LOGV
		("[%d]PD_GetResult pd_calc_result.pdConf[4] = %d, pd_calc_result.pdPhaseDiff[4] = %lf, pd_calc_result->pdDCCGain[4] = %d",
		 pd_calc_result->pdGetFrameID, pd_calc_result->pdConf[4], pd_calc_result->pdPhaseDiff[4], pd_calc_result->pdDCCGain[4]);

	return AFV1_SUCCESS;
}

static cmr_s32 af_sprd_set_update_aux_sensor(cmr_handle handle, void *param0)
{
	af_ctrl_t *af = (af_ctrl_t *) handle;
	struct afctrl_sensor_info_t *aux_sensor_info = (struct afctrl_sensor_info_t *)param0;

	switch (aux_sensor_info->type) {
	case AF_SENSOR_ACCELEROMETER:
		ISP_LOGV("accelerometer vertical_up = %f vertical_down = %f horizontal = %f", aux_sensor_info->gsensor_info.vertical_up,
				 aux_sensor_info->gsensor_info.vertical_down, aux_sensor_info->gsensor_info.horizontal);
		af->gsensor_info.vertical_up = aux_sensor_info->gsensor_info.vertical_up;
		af->gsensor_info.vertical_down = aux_sensor_info->gsensor_info.vertical_down;
		af->gsensor_info.horizontal = aux_sensor_info->gsensor_info.horizontal;
		af->gsensor_info.timestamp = aux_sensor_info->gsensor_info.timestamp;
		af->gsensor_info.valid = 1;
		af->trigger_source_type |= AF_DATA_G;
		break;
	case AF_SENSOR_MAGNETIC_FIELD:
		ISP_LOGV("magnetic field E");
		break;
	case AF_SENSOR_GYROSCOPE:
		ISP_LOGV("gyro E");
		break;
	case AF_SENSOR_LIGHT:
		ISP_LOGV("light E");
		break;
	case AF_SENSOR_PROXIMITY:
		ISP_LOGV("proximity E");
		break;
	default:
		ISP_LOGI("sensor type not support");
		break;
	}

	return AFV1_SUCCESS;
}

static cmr_s32 af_sprd_get_fullscan_info(cmr_handle handle, void *param0)
{
	af_ctrl_t *af = (af_ctrl_t *) handle;
	struct af_fullscan_info *af_fullscan_info = (struct af_fullscan_info *)param0;
	Bokeh_Result result;
	result.win_peak_pos_num = sizeof(af->win_peak_pos) / sizeof(af->win_peak_pos[0]);
	result.win_peak_pos = af->win_peak_pos;
	af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_Get_Bokeh_Result, &result);
	if (NULL != af_fullscan_info) {
		af_fullscan_info->row_num = result.row_num;
		af_fullscan_info->column_num = result.column_num;
		af_fullscan_info->win_peak_pos = result.win_peak_pos;
		af_fullscan_info->vcm_dac_low_bound = result.vcm_dac_low_bound;
		af_fullscan_info->vcm_dac_up_bound = result.vcm_dac_up_bound;
		af_fullscan_info->boundary_ratio = result.boundary_ratio;

		af_fullscan_info->af_peak_pos = result.af_peak_pos;
		af_fullscan_info->near_peak_pos = result.near_peak_pos;
		af_fullscan_info->far_peak_pos = result.far_peak_pos;
		af_fullscan_info->distance_reminder = result.distance_reminder;
	}
	return AFV1_SUCCESS;
}

// SharkLE Only ++
static cmr_s32 af_sprd_set_dac_info(cmr_handle handle, void *param0)
{
	af_ctrl_t *af = (af_ctrl_t *) handle;
	af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_Set_Dac_info, param0);

	return AFV1_SUCCESS;
}

// SharkLE Only --

cmr_s32 af_sprd_adpt_inctrl(cmr_handle handle, cmr_s32 cmd, void *param0, void *param1)
{
	UNUSED(param1);
	af_ctrl_t *af = (af_ctrl_t *) handle;
	cmr_int rtn = AFV1_SUCCESS;

	switch (cmd) {
	case AF_CMD_SET_AF_POS:
		if (NULL != af->cb_ops.af_set_motor_pos) {
			af->cb_ops.af_set_motor_pos(af->caller, *(cmr_u16 *) param0);
		}
		break;

	case AF_CMD_SET_AF_MODE:
		rtn = af_sprd_set_af_mode(handle, param0);
		break;

	case AF_CMD_SET_TUNING_MODE:
		break;
	case AF_CMD_SET_SCENE_MODE:
		break;

	case AF_CMD_SET_AF_STOP:
		rtn = af_sprd_set_af_cancel(handle, param0);
		break;

	case AF_CMD_SET_AF_RESTART:
		break;
	case AF_CMD_SET_CAF_RESET:
		break;
	case AF_CMD_SET_CAF_STOP:
		break;
	case AF_CMD_SET_AF_FINISH:
		break;

	case AF_CMD_SET_AF_BYPASS:
		rtn = af_sprd_set_af_bypass(handle, param0);
		break;

	case AF_CMD_SET_DEFAULT_AF_WIN:
		break;

	case AF_CMD_SET_FLASH_NOTICE:
		rtn = af_sprd_set_flash_notice(handle, param0);
		break;

	case AF_CMD_SET_ISP_START_INFO:
		rtn = af_sprd_set_video_start(handle, param0);
		break;

	case AF_CMD_SET_ISP_TOOL_AF_TEST:
		break;
	case AF_CMD_SET_CAF_TRIG_START:
		break;

	case AF_CMD_SET_AE_INFO:
		rtn = af_sprd_set_ae_info(handle, param0);
		break;

	case AF_CMD_SET_AWB_INFO:
		rtn = af_sprd_set_awb_info(handle, param0);
		break;

	case AF_CMD_SET_FACE_DETECT:
		rtn = af_sprd_set_face_detect(handle, param0);
		break;

	case AF_CMD_SET_PD_INFO:
		rtn = af_sprd_set_pd_info(handle, param0);
		ISP_LOGV("pdaf set callback end");
		break;

	case AF_CMD_SET_UPDATE_AUX_SENSOR:
		rtn = af_sprd_set_update_aux_sensor(handle, param0);
		break;

	case AF_CMD_SET_AF_START:
		rtn = af_sprd_set_af_trigger(handle, param0);
		break;

	case AF_CMD_SET_ISP_STOP_INFO:
		rtn = af_sprd_set_video_stop(handle, param0);
		break;

	case AF_CMD_SET_DCAM_TIMESTAMP:
		rtn = af_sprd_set_dcam_timestamp(handle, param0);
		break;
		// SharkLE Only ++
	case AF_CMD_SET_DAC_INFO:
		rtn = af_sprd_set_dac_info(handle, param0);
		break;
		// SharkLE Only --
	default:
		ISP_LOGW("set cmd not support! cmd: %d", cmd);
		rtn = AFV1_ERROR;
		break;
	}

	return rtn;
}

cmr_s32 af_sprd_adpt_outctrl(cmr_handle handle, cmr_s32 cmd, void *param0, void *param1)
{
	UNUSED(param1);
	af_ctrl_t *af = (af_ctrl_t *) handle;
	cmr_int rtn = AFV1_SUCCESS;
	switch (cmd) {
	case AF_CMD_GET_AF_MODE:
		*(cmr_u32 *) param0 = af->request_mode;
		break;

	case AF_CMD_GET_AF_CUR_POS:
		*(cmr_u32 *) param0 = (cmr_u32) lens_get_pos(af);
		break;

	case AF_CMD_GET_AF_FULLSCAN_INFO:
		rtn = af_sprd_get_fullscan_info(handle, param0);
		break;

	case AF_CMD_GET_AF_LOG_INFO:
		{
			struct af_log_info *log_info = (struct af_log_info *)param0;
			log_info->log_cxt = af->af_alg_cxt;
			log_info->log_len = af->af_dump_info_len;
			ISP_LOGI("Get AF Log info 0x%x ", log_info->log_len);
			break;
		}
	case AF_CMD_GET_AFT_LOG_INFO:
		{
			struct af_log_info *log_info = (struct af_log_info *)param0;
			log_info->log_cxt = af->trig_ops.handle.aft_sub_handle;
			log_info->log_len = af->trig_ops.handle.aft_dump_info_len;
			ISP_LOGI("Get AFT Log info 0x%x ", log_info->log_len);
			break;
		}
	default:
		ISP_LOGW("cmd not support! cmd: %d", cmd);
		rtn = AFV1_ERROR;
		break;
	}

	return rtn;
}

cmr_s32 af_otp_info_parser(struct afctrl_init_in * init_param)
{
	struct sensor_otp_section_info *af_otp_info_ptr = NULL;
	struct sensor_otp_section_info *module_info_ptr = NULL;
	cmr_u16 af_rdm_otp_len = 0;
	cmr_u8 *module_info = NULL;
	cmr_u8 *af_rdm_otp_data = NULL;

	if (NULL != init_param->otp_info_ptr) {
		if (init_param->otp_info_ptr->otp_vendor == OTP_VENDOR_SINGLE) {
			af_otp_info_ptr = init_param->otp_info_ptr->single_otp.af_info;
			module_info_ptr = init_param->otp_info_ptr->single_otp.module_info;
			ISP_LOGV("pass af otp, single cam");
		} else if (init_param->otp_info_ptr->otp_vendor == OTP_VENDOR_SINGLE_CAM_DUAL || init_param->otp_info_ptr->otp_vendor == OTP_VENDOR_DUAL_CAM_DUAL) {
			if (init_param->is_master == 1) {
				af_otp_info_ptr = init_param->otp_info_ptr->dual_otp.master_af_info;
				module_info_ptr = init_param->otp_info_ptr->dual_otp.master_module_info;
				ISP_LOGV("pass af otp, dual cam master");
			} else {
				af_otp_info_ptr = init_param->otp_info_ptr->dual_otp.slave_af_info;
				module_info_ptr = init_param->otp_info_ptr->dual_otp.slave_module_info;
				ISP_LOGV("pass af otp, dual cam slave");
			}
		}
	} else {
		af_otp_info_ptr = NULL;
		module_info_ptr = NULL;
		ISP_LOGE("af otp_info_ptr is NULL");
	}

	if (NULL != af_otp_info_ptr && NULL != module_info_ptr) {
		af_rdm_otp_len = af_otp_info_ptr->rdm_info.data_size;
		module_info = (cmr_u8 *) module_info_ptr->rdm_info.data_addr;

		if (NULL != module_info) {
			if ((module_info[4] == 0 && module_info[5] == 1)
				|| (module_info[4] == 0 && module_info[5] == 2)
				|| (module_info[4] == 0 && module_info[5] == 3)
				|| (module_info[4] == 0 && module_info[5] == 4)
				|| (module_info[4] == 1 && module_info[5] == 0 && (module_info[0] != 0x53 || module_info[1] != 0x50 || module_info[2] != 0x52 || module_info[3] != 0x44))
				|| (module_info[4] == 2 && module_info[5] == 0) || (module_info[4] == 3 && module_info[5] == 0) || (module_info[4] == 4 && module_info[5] == 0)) {
				ISP_LOGV("af otp map v0.4");
				af_rdm_otp_data = (cmr_u8 *) af_otp_info_ptr->rdm_info.data_addr;
			} else if (module_info[4] == 1 && module_info[5] == 0 && module_info[0] == 0x53 && module_info[1] == 0x50 && module_info[2] == 0x52 && module_info[3] == 0x44) {
				ISP_LOGV("af otp map v1.0");
				af_rdm_otp_data = (cmr_u8 *) af_otp_info_ptr->rdm_info.data_addr + 1;
			} else {
				af_rdm_otp_data = NULL;
				ISP_LOGE("af otp map version error");
			}
		} else {
			af_rdm_otp_data = NULL;
			ISP_LOGE("af module_info is NULL");
		}

		if (NULL != af_rdm_otp_data && 0 != af_rdm_otp_len) {
			init_param->otp_info.rdm_data.infinite_cali = (af_rdm_otp_data[1] << 8) | af_rdm_otp_data[0];
			init_param->otp_info.rdm_data.macro_cali = (af_rdm_otp_data[3] << 8) | af_rdm_otp_data[2];
		} else {
			ISP_LOGE("af_rdm_otp_data = %p, af_rdm_otp_len = %d. Parser fail !", af_rdm_otp_data, af_rdm_otp_len);
			init_param->otp_info.rdm_data.infinite_cali = 0;
			init_param->otp_info.rdm_data.macro_cali = 0;
		}
	} else {
		ISP_LOGE("af otp_info_ptr = %p, module_info_ptr = %p. Parser fail !", af_otp_info_ptr, module_info_ptr);
	}

	return AFV1_SUCCESS;
}

cmr_handle sprd_afv1_init(void *in, void *out)
{
	af_ctrl_t *af = NULL;
	char value[PROPERTY_VALUE_MAX] = { '\0' };
	ISP_LOGI("Enter");
	struct afctrl_init_in *init_param = (struct afctrl_init_in *)in;
	struct afctrl_init_out *result = (struct afctrl_init_out *)out;

	if (NULL == init_param) {
		ISP_LOGE("fail to init param:%p, result:%p", init_param, result);
		return NULL;
	}
	// parser af otp info
	af_otp_info_parser(init_param);

	init_param->otp_info.gldn_data.infinite_cali = 0;
	init_param->otp_info.gldn_data.macro_cali = 0;
	ISP_LOGV("af otp golden [%d %d]  rdm [%d %d]",
			 init_param->otp_info.gldn_data.infinite_cali, init_param->otp_info.gldn_data.macro_cali, init_param->otp_info.rdm_data.infinite_cali, init_param->otp_info.rdm_data.macro_cali);

	if (NULL == init_param->aftuning_data || 0 == init_param->aftuning_data_len) {
		ISP_LOGE("fail to get sensor tuning param data");
		return NULL;
	}

	af = (af_ctrl_t *) malloc(sizeof(*af));
	if (NULL == af) {
		ISP_LOGE("fail to malloc af_ctrl_t");
		return NULL;
	}
	memset(af, 0, sizeof(*af));
	af->isp_info.width = init_param->src.w;
	af->isp_info.height = init_param->src.h;
	af->isp_info.win_num = afm_get_win_num(init_param);
	af->caller = init_param->caller;
	af->otp_info.gldn_data.infinite_cali = init_param->otp_info.gldn_data.infinite_cali;
	af->otp_info.gldn_data.macro_cali = init_param->otp_info.gldn_data.macro_cali;
	af->otp_info.rdm_data.infinite_cali = init_param->otp_info.rdm_data.infinite_cali;
	af->otp_info.rdm_data.macro_cali = init_param->otp_info.rdm_data.macro_cali;
	af->is_multi_mode = init_param->is_multi_mode;
	af->cb_ops = init_param->cb_ops;

	// set tuning buffer pointer
	af->aftuning_data = init_param->aftuning_data;
	af->aftuning_data_len = init_param->aftuning_data_len;
	af->pdaftuning_data = init_param->pdaftuning_data;
	af->pdaftuning_data_len = init_param->pdaftuning_data_len;
	af->afttuning_data = init_param->afttuning_data;
	af->afttuning_data_len = init_param->afttuning_data_len;

	ISP_LOGI("width = %d, height = %d, win_num = %d, is_multi_mode %d", af->isp_info.width, af->isp_info.height, af->isp_info.win_num, af->is_multi_mode);
	ISP_LOGI
		("module otp data (infi,macro) = (%d,%d), gldn (infi,macro) = (%d,%d)",
		 af->otp_info.rdm_data.infinite_cali, af->otp_info.rdm_data.macro_cali, af->otp_info.gldn_data.infinite_cali, af->otp_info.gldn_data.macro_cali);

	af->af_alg_cxt = af_init(af);
	if (NULL == af->af_alg_cxt) {
		ISP_LOGE("fail to init lib func AF_init");
		free(af);
		af = NULL;
		return NULL;
	}

	if (trigger_init(af, CAF_TRIGGER_LIB) != 0) {
		ISP_LOGE("fail to init trigger");
		goto ERROR_INIT;
	}
	af->trigger_source_type = 0;

	af->pre_state = af->state = STATE_CAF;
	af->focus_state = AF_IDLE;
	af->force_trigger = AFV1_FALSE;
	af->cb_trigger = AFV1_FALSE;

	af->ae_lock_num = 1;
	af->awb_lock_num = 0;
	af->lsc_lock_num = 0;
	af->nlm_lock_num = 0;
	af->ae_partial_lock_num = 0;

	af->afm_tuning.iir_level = 1;
	af->afm_tuning.nr_mode = 2;
	af->afm_tuning.cw_mode = 2;
	af->afm_tuning.fv0_e = 5;
	af->afm_tuning.fv1_e = 5;

	af->dcam_timestamp = 0xffffffffffffffff;
	af->test_loop_quit = 1;
	//property_set("af_mode", "none");

	result->log_info.log_cxt = (cmr_u8 *) af->af_alg_cxt;
	result->log_info.log_len = af->af_dump_info_len;

	property_get("persist.sys.isp.af.bypass", value, "0");
	af->bypass = ! !atoi(value);
	ISP_LOGV("property af bypass %s[%d]", value, ! !atoi(value));

	ISP_LOGI("Exit");
	return (cmr_handle) af;

  ERROR_INIT:
	af->af_ops.deinit(af->af_alg_cxt);
	unload_af_lib(af);
	memset(af, 0, sizeof(*af));
	free(af);
	af = NULL;

	return (cmr_handle) af;
}

cmr_s32 sprd_afv1_deinit(cmr_handle handle, void *param, void *result)
{
	UNUSED(param);
	UNUSED(result);
	ISP_LOGI("Enter");
	af_ctrl_t *af = (af_ctrl_t *) handle;
	cmr_s32 rtn = AFV1_SUCCESS;

	rtn = _check_handle(handle);
	if (AFV1_SUCCESS != rtn) {
		ISP_LOGE("fail to check handle");
		return AFV1_ERROR;
	}

	property_set("af_mode", "none");
	property_set("af_set_pos", "none");
	if (0 == af->test_loop_quit) {
		af->test_loop_quit = 1;
		pthread_join(af->test_loop_handle, NULL);
		af->test_loop_handle = 0;
	}

	afm_disable(af);
	trigger_deinit(af);

	af->af_ops.deinit(af->af_alg_cxt);
	unload_af_lib(af);

	memset(af, 0, sizeof(*af));
	free(af);
	af = NULL;
	ISP_LOGI("Exit");
	return rtn;
}

cmr_s32 sprd_afv1_process(cmr_handle handle, void *in, void *out)
{
	af_ctrl_t *af = (af_ctrl_t *) handle;
	struct afctrl_calc_in *inparam = (struct afctrl_calc_in *)in;
	nsecs_t system_time0 = 0;
	nsecs_t system_time1 = 0;
	nsecs_t system_time_trigger = 0;
	cmr_u32 *af_fv_val = NULL;
	cmr_u32 afm_skip_num = 0;
	cmr_s32 rtn = AFV1_SUCCESS;
	cmr_u8 i = 0;
	UNUSED(out);
	rtn = _check_handle(handle);
	if (AFV1_SUCCESS != rtn) {
		ISP_LOGE("fail to check handle");
		return AFV1_ERROR;
	}

	if (NULL == inparam || (AF_DATA_AF == inparam->data_type && NULL == inparam->data)) {
		ISP_LOGE("fail to get input param data");
		return AFV1_ERROR;
	}
	memset(af->AF_MODE, '\0', sizeof(af->AF_MODE));
	if (1 == af->test_loop_quit) {
		property_get("af_mode", af->AF_MODE, "none");
		if (0 == strcmp(af->AF_MODE, "ISP_FOCUS_MANUAL")) {
			af->test_loop_quit = 0;
			pthread_attr_t attr;
			pthread_attr_init(&attr);
			pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
			ISP_LOGI("pthread_create test thread.");
			rtn = pthread_create(&af->test_loop_handle, &attr, (void *(*)(void *))loop_for_test_mode, (void *)af);
			pthread_attr_destroy(&attr);
			if (rtn) {
				ISP_LOGE("fail to create loop manual mode");
				return 0;
			}
		}
	}

	if (1 == af->bypass) {
		return 0;
	}

	system_time0 = systemTime(CLOCK_MONOTONIC);
	ATRACE_BEGIN(__FUNCTION__);
	ISP_LOGV("state = %s, focus_state = %s, data_type %d", STATE_STRING(af->state), FOCUS_STATE_STR(af->focus_state), inparam->data_type);
	switch (inparam->data_type) {
	case AF_DATA_AF:
		af_fv_val = (cmr_u32 *) (inparam->data);

#ifdef CONFIG_ISP_2_5
		for (i = 0; i < 10; i++) {
			af->af_fv_val.af_fv0[i] = af_fv_val[6 * (i >> 1) + (i & 0x01) + 4];
			af->af_fv_val.af_fv1[i] = af_fv_val[6 * (i >> 1) + (i & 0x01) + 2];
		}
#endif

#ifdef CONFIG_ISP_2_4
		for (i = 0; i < 10; i++) {
			cmr_u64 high = af_fv_val[95 + i / 2];
			high = (i & 0x01) ? ((high & 0x00FF0000) << 16) : ((high & 0x000000FF) << 32);
			af->af_fv_val.af_fv0[i] = af_fv_val[61 + i * 3] + high;	// spsmd g channels

			high = af_fv_val[95 + i / 2];
			high = (i & 0x01) ? ((high & 0x0F000000) << 12) : ((high & 0x00000F00) << 24);
			af->af_fv_val.af_fv1[i] = af_fv_val[31 + i * 3] + high;	// soble9x9 g channels
		}
#endif

#if defined(CONFIG_ISP_2_1) || defined(CONFIG_ISP_2_2) || defined(CONFIG_ISP_2_3)// ISP2.1/2.2/2.3 share same AFM filter,
		for (i = 0; i < 10; i++) {
			af->af_fv_val.af_fv0[i] = ((((cmr_u64) af_fv_val[20 + i]) & 0x00000fff) << 32) | (((cmr_u64) af_fv_val[i]));
			af->af_fv_val.af_fv1[i] = (((((cmr_u64) af_fv_val[20 + i]) >> 12) & 0x00000fff) << 32) | ((cmr_u64) af_fv_val[10 + i]);
		}
#endif

		if (inparam->sensor_fps.is_high_fps) {
			afm_skip_num = inparam->sensor_fps.high_fps_skip_num - 1;
			if (afm_skip_num != af->afm_skip_num) {
				af->afm_skip_num = afm_skip_num;
				ISP_LOGI("af.skip_num %d", af->afm_skip_num);
				af->cb_ops.af_monitor_skip_num(af->caller, (void *)&af->afm_skip_num);
			}
		} else {
			af->afm_skip_num = 0;
		}
		af->trigger_source_type |= AF_DATA_AF;
		break;

	case AF_DATA_IMG_BLK:
		if (STATE_CAF == af->state || STATE_RECORD_CAF == af->state || STATE_NORMAL_AF == af->state) {
			caf_monitor_process(af);
		}
		break;

	default:
		ISP_LOGV("unsupported data type: %d", inparam->data_type);
		rtn = AFV1_ERROR;
		break;
	}

	system_time_trigger = systemTime(CLOCK_MONOTONIC);
	ISP_LOGV("SYSTEM_TEST-trigger:%dus", (cmr_s32) ((system_time_trigger - system_time0) / 1000));

	if (AF_DATA_AF == inparam->data_type) {
		switch (af->state) {
		case STATE_NORMAL_AF:
			if (AF_SEARCHING == af->focus_state) {
				rtn = saf_process_frame(af);
				if (1 == rtn) {
					af->focus_state = AF_IDLE;
					trigger_start(af);
				}
			}
			break;
		case STATE_FULLSCAN:
		case STATE_CAF:
		case STATE_RECORD_CAF:
			if (AF_SEARCHING == af->focus_state) {
				rtn = caf_process_frame(af);
				if (1 == rtn) {
					af->focus_state = AF_IDLE;
					trigger_start(af);
				}
			} else {
				af->af_ops.ioctrl(af->af_alg_cxt, AF_IOCTRL_Set_Pre_Trigger_Data, NULL);
			}
			break;
		case STATE_FAF:
			rtn = faf_process_frame(af);
			if (1 == rtn) {
				af->focus_state = AF_IDLE;
				af->state = af->pre_state;
				trigger_start(af);
			}
			break;
		case STATE_MANUAL:
			af->af_ops.calc(af->af_alg_cxt);
			break;
		case STATE_PICTURE:
			break;
		default:
			ISP_LOGW("af->state %s is not supported", STATE_STRING(af->state));
			break;
		}
	}
	ATRACE_END();
	system_time1 = systemTime(CLOCK_MONOTONIC);
	ISP_LOGV("SYSTEM_TEST-af:%dus", (cmr_s32) ((system_time1 - system_time0) / 1000));

	return rtn;
}

cmr_s32 sprd_afv1_ioctrl(cmr_handle handle, cmr_s32 cmd, void *param0, void *param1)
{
	cmr_int rtn = AFV1_SUCCESS;
	rtn = _check_handle(handle);
	if (AFV1_SUCCESS != rtn) {
		ISP_LOGE("fail to check cxt");
		return AFV1_ERROR;
	}

	ISP_LOGV("cmd is 0x%x", cmd);
	if ((AF_CMD_SET_BASE < cmd) && (AF_CMD_SET_MAX > cmd))
		rtn = af_sprd_adpt_inctrl(handle, cmd, param0, param1);
	else if ((AF_CMD_GET_BASE < cmd) && (AF_CMD_GET_MAX > cmd))
		rtn = af_sprd_adpt_outctrl(handle, cmd, param0, param1);

	ISP_LOGV("rtn %ld", rtn);
	return rtn;

}

struct adpt_ops_type af_sprd_adpt_ops_ver1 = {
	.adpt_init = sprd_afv1_init,
	.adpt_deinit = sprd_afv1_deinit,
	.adpt_process = sprd_afv1_process,
	.adpt_ioctrl = sprd_afv1_ioctrl,
};
