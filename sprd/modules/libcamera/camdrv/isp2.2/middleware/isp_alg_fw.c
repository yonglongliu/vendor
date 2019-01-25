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
#define LOG_TAG "isp_alg_fw"

#include <math.h>
#include "isp_alg_fw.h"
#include "lib_ctrl.h"
#include "cmr_msg.h"
#include "ae_ctrl.h"
#include "awb_ctrl.h"
#include "smart_ctrl.h"
#include "af_ctrl.h"
#include "afl_ctrl.h"
#include "lsc_adv.h"
#include "isp_dev_access.h"
#include "isp_ioctrl.h"
#include "isp_param_file_update.h"
#include "pdaf_ctrl.h"
#include "af_sprd_adpt_v1.h"
#include "isp_match.h"
#include <dlfcn.h>
#include <immintrin.h>

cmr_u32 isp_cur_bv;
cmr_u32 isp_cur_ct;

#define LSC_ADV_ENABLE
#define PDLIB_PATH "libispalg.so"
#define MAX_BINNING_STATS_SIZE (640*480)
//#define ANTI_FLICKER_INFO_VERSION_NEW

cmr_s32 gAWBGainR = 1;
cmr_s32 gAWBGainB = 1;
//cmr_s32 gCntSendMsgLsc = 0;                   //lsc removed
//static cmr_u32 aeStatistic[32 * 32 * 4];      //lsc removed

struct isp_awb_calc_info {
	struct ae_calc_out ae_result;
	struct isp_awb_statistic_info *ae_stat_ptr;
	struct isp_binning_statistic_info *awb_stat_ptr;
	cmr_u64 k_addr;
	cmr_u64 u_addr;
	cmr_u32 type;
};

struct isp_alsc_calc_info {
	cmr_u32 *stat_ptr;
	cmr_s32 image_width;
	cmr_s32 image_height;
	struct awb_size stat_img_size;
	struct awb_size win_size;
	cmr_u32 awb_ct;
	cmr_s32 awb_r_gain;
	cmr_s32 awb_b_gain;
	cmr_u32 stable;
};

struct isp_alg_sw_init_in {
	cmr_handle dev_access_handle;
	struct sensor_libuse_info *lib_use_info;
	struct isp_size size;
	struct sensor_otp_cust_info *otp_data;
	struct sensor_pdaf_info *pdaf_info;
	struct isp_size	sensor_max_size;
};

typedef struct {
	int grid_size;
	int lpf_mode;
	int lpf_radius;
	int lpf_border;
	int border_patch;
	int border_expand;
	int shading_mode;
	int shading_pct;
} lsc2d_calib_param_t;


static nsecs_t ispalg_get_sys_timestamp(void)
{
	nsecs_t timestamp = 0;

	timestamp = systemTime(CLOCK_MONOTONIC);
	timestamp = timestamp / 1000000;

	return timestamp;
}

static cmr_int ispalg_get_rgb_gain(cmr_handle isp_fw_handle, cmr_u32 *param)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct isp_pm_param_data param_data;
	struct isp_pm_ioctl_input input = { NULL, 0 };
	struct isp_pm_ioctl_output output = { NULL, 0 };
	struct isp_dev_rgb_gain_info *gain_info;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_fw_handle;

	memset(&param_data, 0, sizeof(param_data));

	BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_ISP_SETTING, ISP_BLK_RGB_GAIN, NULL, 0);
	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_SINGLE_SETTING, &input, &output);
	if (ISP_SUCCESS == ret && 1 == output.param_num) {
		gain_info = (struct isp_dev_rgb_gain_info *)output.param_data->data_ptr;
		*param = gain_info->global_gain;
	} else {
		*param = 4096;
	}

	ISP_LOGV("D-gain global gain ori: %d\n", *param);

	return ret;
}

static cmr_int ispalg_ae_callback(cmr_handle isp_alg_handle, cmr_int cb_type, void *data)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	enum isp_callback_cmd cmd = 0;
	void *in = NULL;

	if (!cxt) {
		ret = -ISP_PARAM_NULL;
		goto exit;
	}

	switch (cb_type) {
	case AE_CB_FLASHING_CONVERGED:
	case AE_CB_CONVERGED:
	case AE_CB_CLOSE_PREFLASH:
	case AE_CB_PREFLASH_PERIOD_END:
	case AE_CB_CLOSE_MAIN_FLASH:
		cmd = ISP_AE_STAB_CALLBACK;
		break;
	case AE_CB_QUICKMODE_DOWN:
		cmd = ISP_QUICK_MODE_DOWN;
		break;
	case AE_CB_TOUCH_AE_NOTIFY:
	case AE_CB_STAB_NOTIFY:
		cmd = ISP_AE_STAB_NOTIFY;
		break;
	case AE_CB_AE_LOCK_NOTIFY:
		cmd = ISP_AE_LOCK_NOTIFY;
		break;
	case AE_CB_AE_UNLOCK_NOTIFY:
		cmd = ISP_AE_UNLOCK_NOTIFY;
		break;
	case AE_CB_HDR_START:
		cmd = ISP_HDR_EV_EFFECT_CALLBACK;
		break;
	case AE_CB_LED_NOTIFY:
		in = data;
		cmd = ISP_ONLINE_FLASH_CALLBACK;
		break;
	case AE_CB_FLASH_FIRED:
		in = data;
		cmd = ISP_AE_CB_FLASH_FIRED;
		break;
	case AE_CB_AE_CALCOUT_NOTIFY:
		in = data;
		cmd = ISP_AE_CALCOUT_NOTIFY;
		break;
	case AE_CB_EXPTIME_NOTIFY:
		cmd = ISP_AE_EXP_TIME;
		in = data;
		break;
	case AE_CB_PROCESS_OUT:
		break;
	default:
		cmd = ISP_AE_STAB_CALLBACK;
		break;
	}

	if (cxt->commn_cxt.callback) {
		cxt->commn_cxt.callback(cxt->commn_cxt.caller_id, ISP_CALLBACK_EVT | cmd, in, 0);
	}
exit:
	return ret;
}

static cmr_int ispalg_ae_set_cb(cmr_handle isp_alg_handle, cmr_int type, void *param0, void *param1)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	switch (type) {
	case ISP_AE_SET_GAIN:
		ret = cxt->ioctrl_ptr->set_gain(cxt->ioctrl_ptr->caller_handler, *(cmr_u32 *) param0);
		break;
	case ISP_AE_DUAL_SYNC_WRITE_SET:
		ret = cxt->ioctrl_ptr->write_aec_info(cxt->ioctrl_ptr->caller_handler, param0);
		break;
	case ISP_AE_DUAL_SYNC_READ_AEINFO:
		ret = cxt->ioctrl_ptr->read_aec_info(cxt->ioctrl_ptr->caller_handler, param0);
		break;
	case ISP_AE_DUAL_SYNC_READ_AEINFO_SLV:
		if (cxt->is_multi_mode && cxt->is_master) {
			cmr_handle isp_3a_handle_slv;
			struct isp_alg_fw_context *cxt_slv = NULL;

			isp_3a_handle_slv = isp_br_get_3a_handle(!cxt->is_master); //get slave 3a fw handle
			cxt_slv = (struct isp_alg_fw_context *)isp_3a_handle_slv;
			if (cxt_slv != NULL) {
				cxt->ioctrl_ptr_slv = cxt_slv->ioctrl_ptr;
				ret = cxt->ioctrl_ptr_slv->read_aec_info(cxt->ioctrl_ptr_slv->caller_handler, param0);
			}
			else
			{
				ISP_LOGE("failed to get slave sensor handle , it is not ready");
			}
		}
		break;
	case ISP_AE_SET_EXPOSURE:
		ret = cxt->ioctrl_ptr->set_exposure(cxt->ioctrl_ptr->caller_handler, *(cmr_u32 *) param0);
		break;
	case ISP_AE_EX_SET_EXPOSURE:
		ret = cxt->ioctrl_ptr->ex_set_exposure(cxt->ioctrl_ptr->caller_handler, (cmr_u32) param0);
		break;
	case ISP_AE_SET_MONITOR:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AE_MONITOR, param0, param1);
		break;
	case ISP_AE_SET_MONITOR_WIN:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AE_MONITOR_WIN, param0, param1);
		break;
	case ISP_AE_SET_MONITOR_BYPASS:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AE_MONITOR_BYPASS, param0, param1);
		break;
	case ISP_AE_SET_STATISTICS_MODE:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AE_STATISTICS_MODE, param0, param1);
		break;
	case ISP_AE_SET_AE_CALLBACK:
		ret = ispalg_ae_callback(cxt, *(cmr_int *)param0, param1);
		break;
	case ISP_AE_GET_SYSTEM_TIME:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_GET_AE_SYSTEM_TIME, param0, param1);
		break;
	case ISP_AE_SET_RGB_GAIN:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_RGB_GAIN, param0, param1);
		break;
	case ISP_AE_GET_FLASH_CHARGE:
		ret = cxt->commn_cxt.ops.flash_get_charge(cxt->commn_cxt.caller_id, param0, param1);
		break;
	case ISP_AE_GET_FLASH_TIME:
		ret = cxt->commn_cxt.ops.flash_get_time(cxt->commn_cxt.caller_id, param0, param1);
		break;
	case ISP_AE_SET_FLASH_CHARGE:
		ret = cxt->commn_cxt.ops.flash_set_charge(cxt->commn_cxt.caller_id, param0, param1);
		break;
	case ISP_AE_SET_FLASH_TIME:
		ret = cxt->commn_cxt.ops.flash_set_time(cxt->commn_cxt.caller_id, param0, param1);
		break;
	case ISP_AE_FLASH_CTRL:
		ret = cxt->commn_cxt.ops.flash_ctrl(cxt->commn_cxt.caller_id, param0, param1);
		break;
	case ISP_AE_GET_RGB_GAIN:
		ret = ispalg_get_rgb_gain(cxt, param0);
		break;
	case ISP_AE_SET_WBC_GAIN: {
			struct ae_awb_gain *awb_gain = (struct ae_awb_gain*)param0;
			ret = isp_dev_awb_gain(cxt->dev_access_handle, awb_gain->r, awb_gain->g, awb_gain->b);
		}
		break;
	default:
		break;
	}

	return ret;
}

static cmr_int ispalg_af_set_cb(cmr_handle isp_alg_handle, cmr_int type, void *param0, void *param1)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	switch (type) {
	case ISP_AF_SET_POS:
		if (cxt->ioctrl_ptr->set_focus) {
			ret = cxt->ioctrl_ptr->set_focus(cxt->ioctrl_ptr->caller_handler, *(cmr_u32 *) param0);
		}
		break;
	case AF_CB_CMD_SET_END_NOTICE:
		if (ISP_ZERO == cxt->commn_cxt.isp_callback_bypass) {
			ret = cxt->commn_cxt.callback(cxt->commn_cxt.caller_id, ISP_CALLBACK_EVT | ISP_AF_NOTICE_CALLBACK, param0, sizeof(struct isp_af_notice));
		}
		break;
	case AF_CB_CMD_SET_MOTOR_POS:
		if (cxt->ioctrl_ptr->set_pos) {
			ret = cxt->ioctrl_ptr->set_pos(cxt->ioctrl_ptr->caller_handler, *(cmr_u32 *) param0);
		}
		break;
	case AF_CB_CMD_GET_LENS_OTP:
		if (cxt->ioctrl_ptr->get_otp) {
			ret = cxt->ioctrl_ptr->get_otp(cxt->ioctrl_ptr->caller_handler, (uint16_t *) param0, (uint16_t *) param1);
		}
		break;
	case AF_CB_CMD_SET_MOTOR_BESTMODE:
		if (cxt->ioctrl_ptr->set_motor_bestmode) {
			ret = cxt->ioctrl_ptr->set_motor_bestmode(cxt->ioctrl_ptr->caller_handler);
		}
		break;
	case AF_CB_CMD_GET_MOTOR_POS:
		if (cxt->ioctrl_ptr->get_motor_pos) {
			ret = cxt->ioctrl_ptr->get_motor_pos(cxt->ioctrl_ptr->caller_handler,  (uint16_t *)param0);
		}
		break;
	case AF_CB_CMD_SET_VCM_TEST_MODE:
		if (cxt->ioctrl_ptr->set_test_vcm_mode) {
			ret = cxt->ioctrl_ptr->set_test_vcm_mode(cxt->ioctrl_ptr->caller_handler, (char *)param0);
		}
		break;
	case AF_CB_CMD_GET_VCM_TEST_MODE:
		if (cxt->ioctrl_ptr->get_test_vcm_mode) {
			ret = cxt->ioctrl_ptr->get_test_vcm_mode(cxt->ioctrl_ptr->caller_handler);
		}
		break;
	case AF_CB_CMD_SET_START_NOTICE:
		ret = cxt->commn_cxt.callback(cxt->commn_cxt.caller_id, ISP_CALLBACK_EVT | ISP_AF_NOTICE_CALLBACK, param0, sizeof(struct isp_af_notice));
		break;
	case ISP_AF_AE_AWB_LOCK:
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_PAUSE, NULL, param1);
		if (cxt->ops.awb_ops.ioctrl)
			ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_LOCK, NULL, NULL);
		break;
	case ISP_AF_AE_AWB_RELEASE:
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_RESTORE, NULL, param1);
		if (cxt->ops.awb_ops.ioctrl)
			ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_UNLOCK, NULL, NULL);
		break;
	case AF_CB_CMD_SET_AE_LOCK:
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_PAUSE, NULL, param1);
		break;
	case AF_CB_CMD_SET_AE_UNLOCK:
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_RESTORE, NULL, param1);
		break;
	case AF_CB_CMD_SET_AE_CAF_LOCK:
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_CAF_LOCKAE_START, NULL, param1);
		break;
	case AF_CB_CMD_SET_AE_CAF_UNLOCK:
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_CAF_LOCKAE_STOP, NULL, param1);
		break;
	case AF_CB_CMD_SET_AWB_LOCK:
		if (cxt->ops.awb_ops.ioctrl)
			ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_LOCK, NULL, NULL);
		break;
	case AF_CB_CMD_SET_AWB_UNLOCK:
		if (cxt->ops.awb_ops.ioctrl)
			ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_UNLOCK, NULL, NULL);
		break;
	case AF_CB_CMD_SET_LSC_LOCK:
		if (cxt->ops.lsc_ops.ioctrl)
			ret = cxt->ops.lsc_ops.ioctrl(cxt->lsc_cxt.handle, SMART_LSC_ALG_LOCK, NULL, NULL);
		break;
	case AF_CB_CMD_SET_LSC_UNLOCK:
		if (cxt->ops.lsc_ops.ioctrl)
			ret = cxt->ops.lsc_ops.ioctrl(cxt->lsc_cxt.handle, SMART_LSC_ALG_UNLOCK, NULL, NULL);
		break;
	case AF_CB_CMD_SET_NLM_LOCK:
		cxt->smart_cxt.lock_nlm_en = 1;
		break;
	case AF_CB_CMD_SET_NLM_UNLOCK:
		cxt->smart_cxt.lock_nlm_en = 0;
		break;
	case AF_CB_CMD_SET_MONITOR:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AF_MONITOR, param0, param1);
		break;
	case AF_CB_CMD_SET_MONITOR_WIN:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AF_MONITOR_WIN, param0, param1);
		break;
	case AF_CB_CMD_GET_MONITOR_WIN_NUM:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_GET_AF_MONITOR_WIN_NUM, param0, param1);
		break;
	case AF_CB_CMD_SET_AFM_BYPASS:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_RAW_AFM_BYPASS, param0, param1);
		break;
	case AF_CB_CMD_SET_AFM_SKIP_NUM:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AF_SKIP_NUM, param0, param1);
		break;
	case AF_CB_CMD_SET_AFM_MODE:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AF_WORK_MODE, param0, param1);
		break;
	case AF_CB_CMD_SET_AFM_NR_CFG:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AF_IIR_CFG, param0, param1);
		break;
	case AF_CB_CMD_SET_AFM_MODULES_CFG:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AF_MODULES_CFG, param0, param1);
		break;
	case AF_CB_CMD_GET_SYSTEM_TIME:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_GET_AF_SYSTEM_TIME, param0, param1);
		break;
	default:
		break;
	}

	return ret;
}

static cmr_int ispalg_pdaf_set_cb(cmr_handle isp_alg_handle, cmr_int type, void *param0, void *param1)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	ISP_LOGV("isp_pdaf_set_cb type = 0x%lx", type);
	switch (type) {
	case ISP_AF_SET_PD_INFO:
		if (cxt->ops.af_ops.ioctrl)
			ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_SET_PD_INFO, param0, param1);
		break;
	case ISP_PDAF_SET_CFG_PARAM:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_PDAF_CFG_PARAM, param0, param1);
		break;
	case ISP_PDAF_SET_PPI_INFO:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_PDAF_PPI_INFO, param0, param1);
		break;
	case ISP_PDAF_SET_BYPASS:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_PDAF_BYPASS, param0, param1);
		break;
	case ISP_PDAF_SET_WORK_MODE:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_PDAF_WORK_MODE, param0, param1);
		break;
	case ISP_PDAF_SET_EXTRACTOR_BYPASS:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_PDAF_EXTRACTOR_BYPASS, param0, param1);
		break;
	case ISP_PDAF_SET_ROI:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_PDAF_ROI, param0, param1);
		break;
	case ISP_PDAF_SET_SKIP_NUM:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_PDAF_SKIP_NUM, param0, param1);
		break;

	default:
		break;
	}

	return ret;
}

static cmr_int ispalg_afl_set_cb(cmr_handle isp_alg_handle, cmr_int type, void *param0, void *param1)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	switch (type) {
	case ISP_AFL_SET_CFG_PARAM:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AFL_CFG_PARAM, param0, param1);
		break;
	case ISP_AFL_NEW_SET_CFG_PARAM:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AFL_NEW_CFG_PARAM, param0, param1);
		break;
	case ISP_AFL_SET_BYPASS:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AFL_BYPASS, param0, param1);
		break;
	case ISP_AFL_NEW_SET_BYPASS:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AFL_NEW_BYPASS, param0, param1);
		break;
	case ISP_AFL_SET_STATS_BUFFER:
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_STSTIS_BUF, param0, param1);
		break;
	default:
		break;
	}

	return ret;
}

static cmr_int ispalg_smart_set_cb(cmr_handle isp_alg_handle, cmr_int type, void *param0, void *param1)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 i;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct smart_calc_result *smart_result = NULL;
	struct smart_block_result *block_result = NULL;
	struct isp_pm_ioctl_input io_pm_input = { NULL, 0 };
	struct isp_pm_param_data pm_param;
	UNUSED(param1);

	switch (type) {
	case ISP_SMART_SET_COMMON:
		smart_result = (struct smart_calc_result *)param0;
		for (i = 0; i < smart_result->counts; i++) {
			block_result = &smart_result->block_result[i];
			if (block_result->update == 0)
				continue;

			memset(&pm_param, 0, sizeof(pm_param));
			BLOCK_PARAM_CFG(io_pm_input, pm_param, ISP_PM_BLK_SMART_SETTING,
								block_result->block_id, block_result, sizeof(*block_result));
			ISP_LOGV("set param %d, id=%x, data=%p", i, block_result->block_id, block_result);
			ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_SMART, &io_pm_input, NULL);
			ISP_TRACE_IF_FAIL(ret, ("fail to set smart"));

#ifdef Y_GAMMA_SMART_WITH_RGB_GAMMA
			if (ISP_BLK_RGB_GAMC == block_result->block_id) {
				pm_param.id = ISP_BLK_Y_GAMMC;
				ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_SMART, &io_pm_input, NULL);
				ISP_TRACE_IF_FAIL(ret, ("fail to set smart"));
			}
#endif
		}

		break;

	case ISP_SMART_SET_GAMMA_CUR:
		{
			/* param0 should be (struct sensor_rgbgamma_curve *)  */
			void *gamma_cur = param0;

			memset(&pm_param, 0, sizeof(pm_param));
			BLOCK_PARAM_CFG(io_pm_input, pm_param,
								ISP_PM_BLK_GAMMA_CUR,
								ISP_BLK_RGB_GAMC,
								gamma_cur,
								sizeof(struct sensor_rgbgamma_curve));
			ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_OTHERS, &io_pm_input, NULL);
			ISP_TRACE_IF_FAIL(ret, ("fail to set gammar cur"));
		}
		break;

	default:
		ISP_LOGE("Unknown smart callback command: %d", (cmr_u32)type);
		break;
	}

	return ret;
}

cmr_s32 ispalg_alsc_calc(cmr_handle isp_alg_handle,
		  cmr_u32 * ae_stat_r, cmr_u32 * ae_stat_g, cmr_u32 * ae_stat_b,
		  struct awb_size * stat_img_size,
		  struct awb_size * win_size,
		  cmr_s32 image_width, cmr_s32 image_height,
		  cmr_u32 awb_ct,
		  cmr_s32 awb_r_gain, cmr_s32 awb_b_gain,
		  cmr_u32 ae_stable, struct ae_ctrl_callback_in *ae_in)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	lsc_adv_handle_t lsc_adv_handle = cxt->lsc_cxt.handle;
	cmr_handle pm_handle = cxt->handle_pm;
	struct isp_pm_ioctl_input io_pm_input = { NULL, 0 };
	struct isp_pm_ioctl_output io_pm_output = { NULL, 0 };
	struct isp_pm_param_data pm_param;
	struct alsc_update_info update_info = { 0, 0, NULL };
	cmr_u32 lsc_sprd_version = cxt->lsc_cxt.lsc_sprd_version;

	if (lsc_sprd_version >= 2) {

		cmr_u32 i = 0;
		memset(&pm_param, 0, sizeof(pm_param));

		if (NULL == ae_stat_r) {
			ISP_LOGE("fail to  check stat info param");
			return ISP_ERROR;
		}

		BLOCK_PARAM_CFG(io_pm_input, pm_param, ISP_PM_BLK_LSC_INFO, ISP_BLK_2D_LSC, PNULL, 0);
		ret = isp_pm_ioctl(pm_handle, ISP_PM_CMD_GET_SINGLE_SETTING, (void *)&io_pm_input, (void *)&io_pm_output);
		struct isp_lsc_info *lsc_info = (struct isp_lsc_info *)io_pm_output.param_data->data_ptr;
		struct isp_2d_lsc_param *lsc_tab_param_ptr = (struct isp_2d_lsc_param *)(cxt->lsc_cxt.lsc_tab_address);
		if (NULL == lsc_tab_param_ptr || NULL == lsc_info || ISP_SUCCESS != ret) {
			ISP_LOGE("fail to get param");
			return ISP_ERROR;
		}

		struct lsc_adv_calc_param calc_param;
		struct lsc_adv_calc_result calc_result = { 0 };
		memset(&calc_param, 0, sizeof(calc_param));

		calc_result.dst_gain = (cmr_u16 *) lsc_info->data_ptr;
		calc_param.stat_img.r = ae_stat_r;
		calc_param.stat_img.gr = ae_stat_g;
		calc_param.stat_img.gb = ae_stat_g;
		calc_param.stat_img.b = ae_stat_b;
		calc_param.stat_size.w = stat_img_size->w;
		calc_param.stat_size.h = stat_img_size->h;
		calc_param.gain_width = lsc_info->gain_w;
		calc_param.gain_height = lsc_info->gain_h;
		calc_param.block_size.w = win_size->w;
		calc_param.block_size.h = win_size->h;
		calc_param.lum_gain = (cmr_u16 *) lsc_info->param_ptr;
		calc_param.ct = awb_ct;
		calc_param.r_gain = awb_r_gain;
		calc_param.b_gain = awb_b_gain;
		calc_param.grid = lsc_info->grid;
		calc_param.captureFlashEnvRatio = ae_in->flash_param.captureFlashEnvRatio;
		calc_param.captureFlash1ofALLRatio = ae_in->flash_param.captureFlash1ofALLRatio;

		gAWBGainR = awb_r_gain;
		gAWBGainB = awb_b_gain;

		calc_param.bv = ae_in->ae_output.cur_bv;
		calc_param.ae_stable = ae_stable;
		calc_param.isp_mode = cxt->commn_cxt.isp_mode;
		calc_param.isp_id = ISP_2_0;
		calc_param.camera_id = cxt->camera_id;
		calc_param.awb_pg_flag = cxt->awb_cxt.awb_pg_flag;

		calc_param.img_size.w = image_width;
		calc_param.img_size.h = image_height;

		calc_param.handle_pm = pm_handle;

		for (i = 0; i < 9; i++) {
			calc_param.lsc_tab_address[i] = lsc_tab_param_ptr->map_tab[i].param_addr;
		}
		calc_param.lsc_tab_size = cxt->lsc_cxt.lsc_tab_size;

		/*AF lock ALSC */
		if (cxt->lsc_cxt.isp_smart_lsc_lock == 0) {
			if (cxt->ops.lsc_ops.process)
				ret = cxt->ops.lsc_ops.process(lsc_adv_handle, &calc_param, &calc_result);
			if (ISP_SUCCESS != ret) {
				ISP_LOGE("fail to do lsc adv gain map calc");
				return ret;
			}
		}

		if (cxt->ops.lsc_ops.ioctrl)
			ret = cxt->ops.lsc_ops.ioctrl(lsc_adv_handle, ALSC_GET_UPDATE_INFO, NULL, (void *)&update_info);
		if (ISP_SUCCESS != ret)
			ISP_LOGE("fail to get ALSC update flag!");

		if (update_info.alsc_update_flag == 1){
			BLOCK_PARAM_CFG(io_pm_input, pm_param, ISP_PM_BLK_LSC_MEM_ADDR, ISP_BLK_2D_LSC, update_info.lsc_buffer_addr, lsc_info->gain_w*lsc_info->gain_h*4*sizeof(cmr_u16));
			ret = isp_pm_ioctl(pm_handle, ISP_PM_CMD_SET_OTHERS, &io_pm_input, NULL);
		}
	}

	return ret;
}

static cmr_int ispalg_handle_sensor_sof(cmr_handle isp_alg_handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_pm_ioctl_input input = { NULL, 0 };
	struct isp_pm_ioctl_output output = { NULL, 0 };
	struct isp_pm_param_data *param_data = NULL;
	struct isp_af_ts af_ts;
	cmr_u32 sec = 0;
	cmr_u32 usec = 0;
	cmr_u32 i;

	ISP_CHECK_HANDLE_VALID(isp_alg_handle);

	isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_ISP_SETTING, &input, &output);
	param_data = output.param_data;
	for (i = 0; i < output.param_num; i++) {
		if (ISP_BLK_AE_NEW == param_data->id) {
			if (ISP_PM_BLK_ISP_SETTING == param_data->cmd) {
				ret = isp_dev_cfg_block(cxt->dev_access_handle, param_data->data_ptr, param_data->id);
				ISP_TRACE_IF_FAIL(ret, ("fail to isp_dev_cfg_block"));
			}
		} else {
			ret = isp_dev_cfg_block(cxt->dev_access_handle, param_data->data_ptr, param_data->id);
			ISP_TRACE_IF_FAIL(ret, ("fail to isp_dev_cfg_block"));
		}

		if (ISP_BLK_RGB_GAMC == param_data->id) {
			cxt->gamma_sof_cnt = 0;
			cxt->update_gamma_eb = 0;
		}
		param_data++;
	}

	if (cxt->ops.af_ops.ioctrl) {
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_GET_AF_SYSTEM_TIME, &sec, &usec);
		af_ts.timestamp = sec * 1000000000LL + usec * 1000LL;
		af_ts.capture = 0;
		ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_SET_DCAM_TIMESTAMP, (void *)(&af_ts), NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to set AF_CMD_SET_DCAM_TIMESTAMP"));
	}

	ret = isp_dev_comm_shadow(cxt->dev_access_handle, ISP_ONE);
	ISP_TRACE_IF_FAIL(ret, ("fail to set dev shadow "));

	return ret;
}

static cmr_int ispalg_aem_stats_parser(cmr_handle isp_alg_handle, void *data)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_awb_statistic_info *ae_stat_ptr = NULL;
	struct isp_statis_buf_input statis_buf;
	struct isp_statis_info *statis_info = (struct isp_statis_info *)data;
	cmr_u64 k_addr = 0;
	cmr_u64 u_addr = 0;
	cmr_u32 val0 = 0;
	cmr_u32 val1 = 0;
	cmr_u32 i = 0;

	ISP_CHECK_HANDLE_VALID(isp_alg_handle);

	if (cxt->is_multi_mode == ISP_DUAL_NORMAL) {
		cxt->ae_cxt.time.sec = statis_info->sec;
		cxt->ae_cxt.time.usec = statis_info->usec;
		cxt->ae_cxt.monoboottime = statis_info->monoboottime;
	}

	k_addr = statis_info->phy_addr;
	u_addr = statis_info->vir_addr;

	ae_stat_ptr = &cxt->aem_stats;
	for (i = 0x00; i < ISP_RAW_AEM_ITEM; i++) {
		val0 = *((cmr_u32 *) u_addr + i * 2);
		val1 = *(((cmr_u32 *) u_addr) + i * 2 + 1);
		ae_stat_ptr->r_info[i] = ((val1 >> 11) & 0x1fffff) << cxt->ae_cxt.shift;
		ae_stat_ptr->g_info[i] = (((val1 & 0x7ff) << 11) | ((val0 >> 21) & 0x3ff)) << cxt->ae_cxt.shift;
		ae_stat_ptr->b_info[i] = (val0 & 0x1fffff) << cxt->ae_cxt.shift;
	}
	memset((void *)&statis_buf, 0, sizeof(statis_buf));
	statis_buf.buf_size = statis_info->buf_size;
	statis_buf.phy_addr = statis_info->phy_addr;
	statis_buf.vir_addr = statis_info->vir_addr;
	statis_buf.buf_property = ISP_AEM_BLOCK;
	statis_buf.buf_flag = 1;
	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_STSTIS_BUF, &statis_buf, NULL);
	if (ret) {
		ISP_LOGE("fail to set statis buf");
	}
	cxt->aem_is_update = 1;
	return ret;
}

cmr_int ispalg_start_ae_process(cmr_handle isp_alg_handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct ae_calc_in in_param;
	struct awb_gain gain;
	struct awb_gain cur_gain;
	struct ae_calc_out ae_result;
	nsecs_t time_start = 0;
	nsecs_t time_end = 0;
	cmr_u32 awb_mode = 0;
	struct afl_ctrl_proc_out afl_info;
	cmr_int nxt_flicker = 0;

	if (cxt->ops.awb_ops.ioctrl) {
		ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_GET_GAIN, (void *)&gain, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to AWB_CTRL_CMD_GET_GAIN"));
	}

	if (cxt->ops.awb_ops.ioctrl) {
		ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_GET_CUR_GAIN, (void *)&cur_gain, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to AWB_CTRL_CMD_GET_CUR_GAIN"));
	}

	if(cxt->ops.awb_ops.ioctrl){
		ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_GET_WB_MODE, NULL, (void *)&awb_mode);
		ISP_TRACE_IF_FAIL(ret, ("fail to AWB_CTRL_CMD_GET_GAIN"));
	}

	if (cxt->ops.afl_ops.ioctrl) {
		ret = cxt->ops.afl_ops.ioctrl(cxt->afl_cxt.handle, AFL_GET_INFO, (void *)&afl_info, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to AFL_GET_INFO"));
	}
	if (afl_info.flag) {
		if (afl_info.cur_flicker) {
			nxt_flicker = AE_FLICKER_50HZ;
		} else {
			nxt_flicker = AE_FLICKER_60HZ;
		}
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_FLICKER, &nxt_flicker, NULL);
	}

	memset((void *)&ae_result, 0, sizeof(ae_result));
	memset((void *)&in_param, 0, sizeof(in_param));
	if ((0 == gain.r) || (0 == gain.g) || (0 == gain.b) || (0 == cur_gain.r) || (0 == cur_gain.g) || (0 == cur_gain.b) ) {
		in_param.awb_gain_r = 1024;
		in_param.awb_gain_g = 1024;
		in_param.awb_gain_b = 1024;
		in_param.awb_cur_gain_r = 1024;
		in_param.awb_cur_gain_g = 1024;
		in_param.awb_cur_gain_b = 1024;
		in_param.awb_mode = awb_mode;
	} else {
		in_param.awb_gain_r = gain.r;
		in_param.awb_gain_g = gain.g;
		in_param.awb_gain_b = gain.b;
		in_param.awb_cur_gain_r = cur_gain.r;
		in_param.awb_cur_gain_g = cur_gain.g;
		in_param.awb_cur_gain_b = cur_gain.b;
		in_param.awb_mode = awb_mode;
	}

	in_param.stat_fmt = AE_AEM_FMT_RGB;
	if (AE_AEM_FMT_RGB & in_param.stat_fmt) {
		in_param.rgb_stat_img = (cmr_u32 *) & cxt->aem_stats.r_info[0];
		in_param.stat_img = (cmr_u32 *) & cxt->aem_stats.r_info[0];
	}

	in_param.sec = cxt->ae_cxt.time.sec;
	in_param.usec = cxt->ae_cxt.time.usec;
	in_param.binning_stat_info.r_info = cxt->binning_stats.r_info;
	in_param.binning_stat_info.g_info = cxt->binning_stats.g_info;
	in_param.binning_stat_info.b_info = cxt->binning_stats.b_info;
	in_param.binning_stat_info.binning_size.w = cxt->binning_stats.binning_size.w;
	in_param.binning_stat_info.binning_size.h = cxt->binning_stats.binning_size.h;
	in_param.is_update = cxt->aem_is_update;
	in_param.sensor_fps.mode = cxt->sensor_fps.mode;
	in_param.sensor_fps.max_fps = cxt->sensor_fps.max_fps;
	in_param.sensor_fps.min_fps = cxt->sensor_fps.min_fps;
	in_param.sensor_fps.is_high_fps = cxt->sensor_fps.is_high_fps;
	in_param.sensor_fps.high_fps_skip_num = cxt->sensor_fps.high_fps_skip_num;
	time_start = ispalg_get_sys_timestamp();
	if (cxt->ops.ae_ops.process) {
		ret = cxt->ops.ae_ops.process(cxt->ae_cxt.handle, &in_param, &ae_result);
		ISP_TRACE_IF_FAIL(ret, ("fail to ae process"));
	}
	cxt->smart_cxt.isp_smart_eb = 1;
	time_end = ispalg_get_sys_timestamp();
	ISP_LOGV("SYSTEM_TEST-ae:%ldms", (unsigned long)(time_end - time_start));

	if (AL_AE_LIB == cxt->lib_use_info->ae_lib_info.product_id) {
		cxt->ae_cxt.log_alc_ae = ae_result.log_ae.log;
		cxt->ae_cxt.log_alc_ae_size = ae_result.log_ae.size;
	}

	ISP_LOGV("done %ld", ret);
	return ret;
}

cmr_int ispalg_awb_pre_process(cmr_handle isp_alg_handle,
				struct ae_ctrl_callback_in *ae_in,
				struct awb_ctrl_calc_param * out_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct ae_monitor_info info;
	float gain = 0;
	float exposure = 0;
	cmr_s32 bv = 0;
	cmr_s32 iso = 0;
	struct ae_get_ev ae_ev;
	struct isp_pm_ioctl_input io_pm_input = { NULL, 0 };
	struct isp_pm_ioctl_output io_pm_output = { NULL, 0 };
	struct isp_pm_param_data pm_param;
	cmr_s32 i = 0;

	if (!out_ptr || !ae_in || !isp_alg_handle) {
		ret = ISP_PARAM_NULL;
		goto exit;
	}
	memset(&ae_ev, 0, sizeof(ae_ev));
	memset(&info, 0, sizeof(info));
	ISP_LOGV("cur_iso:%d monitor_info h:%d w:%d again:%d ev_level:%d",
		ae_in->ae_output.cur_iso,
		ae_in->monitor_info.win_size.h,
		ae_in->monitor_info.win_size.w,
		ae_in->ae_output.cur_again,
		ae_in->ae_ev.ev_index);

	out_ptr->quick_mode = 0;
	out_ptr->stat_img.chn_img.r = cxt->aem_stats.r_info;
	out_ptr->stat_img.chn_img.g = cxt->aem_stats.g_info;
	out_ptr->stat_img.chn_img.b = cxt->aem_stats.b_info;

	out_ptr->stat_img_awb.chn_img.r = cxt->binning_stats.r_info;
	out_ptr->stat_img_awb.chn_img.g = cxt->binning_stats.g_info;
	out_ptr->stat_img_awb.chn_img.b = cxt->binning_stats.b_info;
	out_ptr->stat_width_awb = cxt->binning_stats.binning_size.w;
	out_ptr->stat_height_awb = cxt->binning_stats.binning_size.h;
	out_ptr->bv = ae_in->ae_output.cur_bv;

	out_ptr->ae_info.bv = ae_in->ae_output.cur_bv;
	out_ptr->ae_info.iso = ae_in->ae_output.cur_iso;
	out_ptr->ae_info.gain = ae_in->ae_output.cur_again;
	out_ptr->ae_info.exposure = ae_in->ae_output.exposure_time;
	out_ptr->ae_info.f_value = 2.2;
	out_ptr->ae_info.stable = ae_in->ae_output.is_stab;
	out_ptr->ae_info.ev_index = ae_in->ae_ev.ev_index;
	memcpy(out_ptr->ae_info.ev_table, ae_in->ae_ev.ev_tab, 16 * sizeof(cmr_s32));

	out_ptr->scalar_factor = (ae_in->monitor_info.win_size.h / 2) * (ae_in->monitor_info.win_size.w / 2);

	memset(&pm_param, 0, sizeof(pm_param));

	// CMC
	BLOCK_PARAM_CFG(io_pm_input, pm_param, ISP_PM_BLK_CMC10, ISP_BLK_CMC10, 0, 0);
	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_SINGLE_SETTING, &io_pm_input, &io_pm_output);

	if (io_pm_output.param_data != NULL && ISP_SUCCESS == ret) {
		cmr_u16 *cmc_info = io_pm_output.param_data->data_ptr;

		if (cmc_info != NULL) {
#define CMC10(n) (((n)>>13)?((n)-(1<<14)):(n))
			for (i = 0; i < 9; i++) {
				out_ptr->matrix[i] = CMC10(cmc_info[i]);
			}
		}
	}
	// GAMMA
	BLOCK_PARAM_CFG(io_pm_input, pm_param, ISP_PM_BLK_GAMMA, ISP_BLK_RGB_GAMC, 0, 0);
	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_SINGLE_SETTING, &io_pm_input, &io_pm_output);

	if (io_pm_output.param_data != NULL && ISP_SUCCESS == ret) {
		struct isp_dev_gamma_info *gamma_info = io_pm_output.param_data->data_ptr;

		if (gamma_info != NULL) {
			for (i = 0; i < 256; i++) {
				out_ptr->gamma[i] = (gamma_info->gamc_nodes.nodes_r[i].node_y + gamma_info->gamc_nodes.nodes_r[i + 1].node_y) / 2;
			}
		}
	}
exit:
	ISP_LOGV("done %ld", ret);
	return ret;
}

cmr_int ispalg_awb_post_process(cmr_handle isp_alg_handle, struct awb_ctrl_calc_result *awb_output)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_pm_ioctl_input ioctl_input;
	struct isp_pm_param_data ioctl_data;
	struct isp_awbc_cfg awbc_cfg;

	if (!awb_output || !isp_alg_handle) {
		ret = ISP_PARAM_NULL;
		goto exit;
	}

	memset((void *)&ioctl_input, 0, sizeof(ioctl_input));
	memset((void *)&ioctl_data, 0, sizeof(ioctl_data));
	memset((void *)&awbc_cfg, 0, sizeof(awbc_cfg));

	/*set awb gain */
	awbc_cfg.r_gain = awb_output->gain.r;
	awbc_cfg.g_gain = awb_output->gain.g;
	awbc_cfg.b_gain = awb_output->gain.b;
	awbc_cfg.r_offset = 0;
	awbc_cfg.g_offset = 0;
	awbc_cfg.b_offset = 0;

	ioctl_data.id = ISP_BLK_AWB_NEW;
	ioctl_data.cmd = ISP_PM_BLK_AWBC;
	ioctl_data.data_ptr = &awbc_cfg;
	ioctl_data.data_size = sizeof(awbc_cfg);

	ioctl_input.param_data_ptr = &ioctl_data;
	ioctl_input.param_num = 1;

	if (0 == awbc_cfg.r_gain && 0 == awbc_cfg.g_gain && 0 == awbc_cfg.b_gain) {
		awbc_cfg.r_gain = 1800;
		awbc_cfg.g_gain = 1024;
		awbc_cfg.b_gain = 1536;
	}
	if(awb_output->update_gain){
		ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_AWB, (void *)&ioctl_input, NULL);
	}

	cxt->awb_cxt.log_awb = awb_output->log_awb.log;
	cxt->awb_cxt.log_awb_size = awb_output->log_awb.size;

	if (ISP_SUCCESS == ret) {
		cxt->awb_cxt.alc_awb = awb_output->use_ccm | (awb_output->use_lsc << 8);
	}

exit:
	ISP_LOGV("done %ld", ret);
	return ret;
}

cmr_int ispalg_start_awb_process(cmr_handle isp_alg_handle,
				struct ae_ctrl_callback_in *ae_in,
				struct awb_ctrl_calc_result * awb_output)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	nsecs_t time_start = 0;
	nsecs_t time_end = 0;

	if (!isp_alg_handle || !ae_in || !awb_output) {
		ret = ISP_PARAM_NULL;
		goto exit;
	}

	memset((void *)&cxt->awb_param, 0, sizeof(struct awb_ctrl_calc_result));

	ret = ispalg_awb_pre_process((cmr_handle) cxt, ae_in, &cxt->awb_param);

	time_start = ispalg_get_sys_timestamp();
	if (cxt->ops.awb_ops.process) {
		ret = cxt->ops.awb_ops.process(cxt->awb_cxt.handle, &cxt->awb_param, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to do awb process"));
	}
	time_end = ispalg_get_sys_timestamp();
	ISP_LOGV("SYSTEM_TEST-awb:%zd ms", (unsigned long)(time_end - time_start));

	if (cxt->ops.awb_ops.ioctrl) {
		ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_RESULT_INFO, (void *)awb_output, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to AWB_CTRL_CMD_GET_GAIN"));
	}

	ret = ispalg_awb_post_process((cmr_handle) cxt, awb_output);

exit:
	ISP_LOGV("done %ld", ret);
	return ret;
}

static cmr_int isp_prepare_atm_param(cmr_handle isp_alg_handle,
                                     struct smart_proc_input *smart_proc_in)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	smart_proc_in->r_info = cxt->binning_stats.r_info;
	smart_proc_in->g_info = cxt->binning_stats.g_info;
	smart_proc_in->b_info = cxt->binning_stats.b_info;
	smart_proc_in->win_num_w = cxt->binning_stats.binning_size.w;
	smart_proc_in->win_num_h = cxt->binning_stats.binning_size.h;
	smart_proc_in->aem_shift = 0;
	smart_proc_in->win_size_w = 1;
	smart_proc_in->win_size_h = 1;

	if (smart_proc_in->r_info == NULL)
		ISP_LOGE("fail to access null r/g/b ptr %p/%p/%p\n",
			smart_proc_in->r_info,
			smart_proc_in->g_info,
			smart_proc_in->b_info);

	return ret;
}

static cmr_int ispalg_aeawb_post_process(cmr_handle isp_alg_handle,
					 struct ae_ctrl_callback_in *ae_in,
					 struct awb_ctrl_calc_result *awb_output)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct smart_proc_input smart_proc_in;
	struct ae_monitor_info info;
	struct awb_size win_size = { 0, 0 };
	struct afctrl_ae_info *ae_info;
	struct afctrl_awb_info *awb_info;
	nsecs_t time_start = 0;
	nsecs_t time_end = 0;
	cmr_s32 bv = 0;
	cmr_s32 bv_gain = 0;

	if (!isp_alg_handle || !ae_in || !awb_output) {
		ret = ISP_PARAM_NULL;
		goto exit;
	}

	memset(&info, 0, sizeof(info));
	memset(&smart_proc_in, 0, sizeof(smart_proc_in));
	CMR_MSG_INIT(message);

	time_start = ispalg_get_sys_timestamp();
	if (1 == cxt->smart_cxt.isp_smart_eb) {

		ISP_LOGV("bv:%d exp_line:%d again:%d cur_lum:%d target_lum:%d FlashEnvRatio:%f Flash1ofALLRatio:%f",
			ae_in->ae_output.cur_bv,
			ae_in->ae_output.cur_exp_line,
			ae_in->ae_output.cur_again,
			ae_in->ae_output.cur_lum,
			ae_in->ae_output.target_lum,
			ae_in->flash_param.captureFlashEnvRatio,
			ae_in->flash_param.captureFlash1ofALLRatio);
		smart_proc_in.cal_para.gamma_tab = cxt->smart_cxt.tunning_gamma_cur;
		smart_proc_in.cal_para.bv = ae_in->ae_output.cur_bv;
		smart_proc_in.cal_para.bv_gain = ae_in->ae_output.cur_again;
		smart_proc_in.cal_para.flash_ratio = ae_in->flash_param.captureFlashEnvRatio * 256;
		smart_proc_in.cal_para.flash_ratio1 = ae_in->flash_param.captureFlash1ofALLRatio * 256;
		smart_proc_in.cal_para.ct = awb_output->ct;
		smart_proc_in.alc_awb = cxt->awb_cxt.alc_awb;
		smart_proc_in.mode_flag = cxt->commn_cxt.mode_flag;
		smart_proc_in.scene_flag = cxt->commn_cxt.scene_flag;
		smart_proc_in.lsc_sprd_version = cxt->lsc_cxt.lsc_sprd_version;
		smart_proc_in.lock_nlm = cxt->smart_cxt.lock_nlm_en;
		smart_proc_in.lock_ee = cxt->smart_cxt.lock_ee_en;
		smart_proc_in.lock_precdn = cxt->smart_cxt.lock_precdn_en;
		smart_proc_in.lock_cdn = cxt->smart_cxt.lock_cdn_en;
		smart_proc_in.lock_postcdn = cxt->smart_cxt.lock_postcdn_en;
		smart_proc_in.lock_ccnr = cxt->smart_cxt.lock_ccnr_en;
		smart_proc_in.lock_ynr = cxt->smart_cxt.lock_ynr_en;
		isp_prepare_atm_param(isp_alg_handle, &smart_proc_in);
		if (cxt->ops.smart_ops.calc) {
			ret = cxt->ops.smart_ops.calc(cxt->smart_cxt.handle, &smart_proc_in);
			ISP_TRACE_IF_FAIL(ret, ("fail to do _smart_calc"));
		}
		cxt->smart_cxt.log_smart = smart_proc_in.log;
		cxt->smart_cxt.log_smart_size = smart_proc_in.size;

		struct isp_pm_param_data param_data_alsc;
		struct isp_pm_ioctl_input param_data_alsc_input = { NULL, 0 };
		struct isp_pm_ioctl_output param_data_alsc_output = { NULL, 0 };
		memset(&param_data_alsc, 0, sizeof(param_data_alsc));

		BLOCK_PARAM_CFG(param_data_alsc_input, param_data_alsc, ISP_PM_BLK_LSC_GET_LSCTAB, ISP_BLK_2D_LSC, NULL, 0);
		ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_SINGLE_SETTING,
				   (void *)&param_data_alsc_input,
				   (void *)&param_data_alsc_output);
		ISP_RETURN_IF_FAIL(ret, ("fail to ISP_PM_CMD_GET_SINGLE_SETTING"));
		cxt->lsc_cxt.lsc_tab_address = param_data_alsc_output.param_data->data_ptr;
		cxt->lsc_cxt.lsc_tab_size = param_data_alsc_output.param_data->data_size;

		/*send message to alsc process */
		struct isp_alsc_calc_info alsc_info;

		//TBD need remove memcpy & param ae_out_stats
		memcpy(cxt->lsc_cxt.ae_out_stats.r_info, cxt->aem_stats.r_info, 1024 * sizeof(cmr_u32));
		memcpy(cxt->lsc_cxt.ae_out_stats.g_info, cxt->aem_stats.g_info, 1024 * sizeof(cmr_u32));
		memcpy(cxt->lsc_cxt.ae_out_stats.b_info, cxt->aem_stats.b_info, 1024 * sizeof(cmr_u32));

		alsc_info.awb_b_gain = awb_output->gain.b;
		alsc_info.awb_r_gain = awb_output->gain.r;
		alsc_info.awb_ct = awb_output->ct;
		alsc_info.stat_img_size.w = ae_in->monitor_info.win_num.w;
		alsc_info.stat_img_size.h = ae_in->monitor_info.win_num.h;
		alsc_info.stable = ae_in->ae_output.is_stab;
		alsc_info.image_width = cxt->commn_cxt.src.w;
		alsc_info.image_height = cxt->commn_cxt.src.h;

		ret = ispalg_alsc_calc(isp_alg_handle,
				       cxt->lsc_cxt.ae_out_stats.r_info,
				       cxt->lsc_cxt.ae_out_stats.g_info,
				       cxt->lsc_cxt.ae_out_stats.b_info,
				       &alsc_info.stat_img_size, &alsc_info.win_size,
				       alsc_info.image_width, alsc_info.image_height,
				       alsc_info.awb_ct, alsc_info.awb_r_gain, alsc_info.awb_b_gain,
				       alsc_info.stable, ae_in);
		ISP_TRACE_IF_FAIL(ret, ("fail to alsc_calc"));
	}
	time_end = ispalg_get_sys_timestamp();
	ISP_LOGV("SYSTEM_TEST-smart:%ldms", (unsigned long)(time_end - time_start));

	isp_cur_bv = ae_in->ae_output.cur_bv;
	isp_cur_ct = awb_output->ct;

	ae_info = &cxt->ae_info;
	awb_info = &cxt->awb_info;

	if (0 == ae_info->is_update) {
		memset((void *)ae_info, 0, sizeof(struct afctrl_ae_info));
		ae_info->img_blk_info.block_w = 32;
		ae_info->img_blk_info.block_h = 32;
		ae_info->img_blk_info.chn_num = 3;
		ae_info->img_blk_info.pix_per_blk = 1;
		ae_info->img_blk_info.data = (cmr_u32 *) &cxt->aem_stats;
		ae_info->ae_rlt_info.bv = ae_in->ae_output.cur_bv;
		ae_info->ae_rlt_info.is_stab = ae_in->ae_output.is_stab;
		ae_info->ae_rlt_info.cur_exp_line = ae_in->ae_output.cur_exp_line;
		ae_info->ae_rlt_info.cur_dummy = ae_in->ae_output.cur_dummy;
		ae_info->ae_rlt_info.frame_line = ae_in->ae_output.frame_line;
		ae_info->ae_rlt_info.line_time = ae_in->ae_output.line_time;
		ae_info->ae_rlt_info.cur_again = ae_in->ae_output.cur_again;
		ae_info->ae_rlt_info.cur_dgain = ae_in->ae_output.cur_dgain;
		ae_info->ae_rlt_info.cur_lum = ae_in->ae_output.cur_lum;
		ae_info->ae_rlt_info.target_lum = ae_in->ae_output.target_lum;
		ae_info->ae_rlt_info.target_lum_ori = ae_in->ae_output.target_lum_ori;
		ae_info->ae_rlt_info.flag4idx = ae_in->ae_output.flag4idx;
		ae_info->ae_rlt_info.cur_ev = ae_in->ae_output.cur_ev;
		ae_info->ae_rlt_info.cur_index = ae_in->ae_output.cur_index;
		ae_info->ae_rlt_info.cur_iso = ae_in->ae_output.cur_iso;
		ae_info->is_update = 1;
	}

	if (0 == awb_info->is_update) {
		memset((void *)awb_info, 0, sizeof(struct afctrl_awb_info));
		awb_info->r_gain = awb_output->gain.r;
		awb_info->g_gain = awb_output->gain.g;
		awb_info->b_gain = awb_output->gain.b;
		awb_info->is_update = 1;
	}

	message.msg_type = ISP_CTRL_EVT_AF;
	message.sub_msg_type = AF_DATA_IMG_BLK;
	message.sync_flag = CMR_MSG_SYNC_NONE;
	ret = cmr_thread_msg_send(cxt->thr_afhandle, &message);
	if (ret) {
		ISP_LOGE("fail to send evt af, ret %ld", ret);
		free(message.data);
	}

exit:
	ISP_LOGV("done ret %ld", ret);
	return ret;
}

cmr_int ispalg_ae_process(cmr_handle isp_alg_handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	ISP_CHECK_HANDLE_VALID(isp_alg_handle);

	ret = ispalg_start_ae_process((cmr_handle) cxt);
exit:

	return ret;
}

static cmr_int ispalg_awb_process(cmr_handle isp_alg_handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct awb_ctrl_calc_result awb_output;
	struct ae_calc_results ae_result;
	struct ae_ctrl_callback_in ae_ctrl_calc_result;

	memset(&awb_output, 0, sizeof(awb_output));
	memset(&ae_result, 0, sizeof(ae_result));
	memset(&ae_ctrl_calc_result, 0, sizeof(ae_ctrl_calc_result));

	if (0 ==cxt->aem_is_update) {
		ISP_LOGI("aem is not update\n");
		goto exit;
	}
	if (cxt->awb_cxt.sw_bypass)
		goto exit;

	if (cxt->ops.ae_ops.ioctrl) {
		cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_CALC_RESULTS, NULL, &ae_result);
		ae_ctrl_calc_result.is_skip_cur_frame = ae_result.is_skip_cur_frame;
		ae_ctrl_calc_result.ae_output = ae_result.ae_output;
		ae_ctrl_calc_result.ae_result = ae_result.ae_result;
		ae_ctrl_calc_result.ae_ev = ae_result.ae_ev;
		ae_ctrl_calc_result.monitor_info = ae_result.monitor_info;
		ae_ctrl_calc_result.flash_param.captureFlashEnvRatio = ae_result.flash_param.captureFlashEnvRatio;
		ae_ctrl_calc_result.flash_param.captureFlash1ofALLRatio = ae_result.flash_param.captureFlash1ofALLRatio;
	}
	ret = ispalg_start_awb_process((cmr_handle) cxt, &ae_ctrl_calc_result, &awb_output);
	if (ret) {
		ISP_LOGE("fail to start awb process");
		goto exit;
	}

	ret = ispalg_aeawb_post_process((cmr_handle) cxt, &ae_ctrl_calc_result, &awb_output);
	if (ret) {
		ISP_LOGE("fail to proc aeawb ");
		goto exit;
	}
exit:
	return ret;
}

cmr_int ispalg_afl_process(cmr_handle isp_alg_handle, void *data)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	cmr_int bypass = 0;
	cmr_u32 cur_flicker = 0;
	cmr_u32 cur_exp_flag = 0;
	cmr_s32 ae_exp_flag = 0;
	float ae_exp = 0.0;
	struct isp_pm_param_data param_data;
	struct isp_pm_ioctl_input input = { NULL, 0 };
	struct isp_pm_ioctl_output output = { NULL, 0 };
	struct afl_proc_in afl_input;
	struct isp_statis_info *statis_info = NULL;
	cmr_u32 k_addr = 0;
	cmr_u32 u_addr = 0;

	ISP_CHECK_HANDLE_VALID(isp_alg_handle);
	statis_info = (struct isp_statis_info *)data;
	k_addr = statis_info->phy_addr;
	u_addr = statis_info->vir_addr;
	//memcpy((void *)&ae_stat_ptr, (void *)u_addr, sizeof(struct isp_awb_statistic_info));
	cxt->afl_cxt.ae_stats = cxt->aem_stats;

	bypass = 1;
	if (cxt->ops.afl_ops.ioctrl) {
		ret = cxt->ops.afl_ops.ioctrl(cxt->afl_cxt.handle, AFL_SET_BYPASS, &bypass, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to AFL_SET_BYPASS"));
	}

	if (cxt->ops.ae_ops.ioctrl) {
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_FLICKER_MODE, NULL, &cur_flicker);
		ISP_LOGV("cur flicker mode %d", cur_flicker);

		//exposure 1/33 s  -- 302921 (+/-10)
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_EXP, NULL, &ae_exp);
	}
	if (fabs(ae_exp - 0.04) < 0.000001 || ae_exp > 0.04) {//0.06
		ae_exp_flag = 1;
	}
	ISP_LOGV("ae_exp %f; ae_exp_flag %d", ae_exp, ae_exp_flag);

	if (cxt->ops.ae_ops.ioctrl) {
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_FLICKER_SWITCH_FLAG, &cur_exp_flag, NULL);
		ISP_LOGV("cur exposure flag %d", cur_exp_flag);
	}

	afl_input.ae_stat_ptr = &cxt->afl_cxt.ae_stats;
	afl_input.ae_exp_flag = ae_exp_flag;
	afl_input.cur_exp_flag = cur_exp_flag;
	afl_input.cur_flicker = cur_flicker;
	afl_input.vir_addr = u_addr;
	afl_input.afl_mode = cxt->afl_cxt.afl_mode;
	afl_input.handle_pm = cxt->handle_pm;

	memset((void *)&cxt->afl_stat_buf, 0, sizeof(cxt->afl_stat_buf));
	cxt->afl_stat_buf.buf_size = statis_info->buf_size;
	cxt->afl_stat_buf.phy_addr = statis_info->phy_addr;
	cxt->afl_stat_buf.vir_addr = statis_info->vir_addr;
	cxt->afl_stat_buf.buf_property = ISP_AFL_BLOCK;
	cxt->afl_stat_buf.buf_flag = 1;
	afl_input.private_len = sizeof(struct isp_statis_buf_input);
	afl_input.private_data = &cxt->afl_stat_buf;

	if (cxt->ops.afl_ops.process)
		ret = cxt->ops.afl_ops.process(cxt->afl_cxt.handle, &afl_input, NULL);

exit:
	ISP_LOGV("done ret %ld", ret);
	return ret;
}

static cmr_int ispalg_af_process(cmr_handle isp_alg_handle, cmr_u32 data_type, void *in_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct afctrl_calc_in calc_param;
	struct afctrl_calc_out calc_result;
	struct isp_statis_info *statis_info = NULL;
	cmr_u32 k_addr = 0;
	cmr_u32 u_addr = 0;
	cmr_s32 i = 0;

	ISP_CHECK_HANDLE_VALID(isp_alg_handle);
	memset((void *)&calc_param, 0, sizeof(calc_param));
	memset((void *)&calc_result, 0, sizeof(calc_result));
	ISP_LOGV("begin data_type %d", data_type);
	switch (data_type) {
	case AF_DATA_AFM_STAT:
	case AF_DATA_AF: {
			struct isp_statis_buf_input statis_buf;
			statis_info = (struct isp_statis_info *)in_ptr;
			k_addr = statis_info->kaddr[0];
			u_addr = statis_info->vir_addr;

			cmr_u32 af_temp[30];
			for (i = 0; i < 30; i++) {
				af_temp[i] = *((cmr_u32 *)(unsigned long)u_addr + i);
			}
			calc_param.data_type = AF_DATA_AF;
			calc_param.sensor_fps.is_high_fps = cxt->sensor_fps.is_high_fps;
			calc_param.sensor_fps.high_fps_skip_num = cxt->sensor_fps.high_fps_skip_num;
			calc_param.data = (void *)(af_temp);
			if (cxt->ops.af_ops.process)
				ret = cxt->ops.af_ops.process(cxt->af_cxt.handle, (void *)&calc_param, &calc_result);

			memset((void *)&statis_buf, 0, sizeof(statis_buf));
			statis_buf.buf_size = statis_info->buf_size;
			statis_buf.phy_addr = statis_info->phy_addr;
			statis_buf.vir_addr = statis_info->vir_addr;
			statis_buf.kaddr[0] = statis_info->kaddr[0];
			statis_buf.kaddr[1] = statis_info->kaddr[1];
			statis_buf.buf_property = ISP_AFM_BLOCK;
			statis_buf.buf_flag = 1;
			ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_STSTIS_BUF, &statis_buf, NULL);
			if (ret) {
				ISP_LOGE("fail to set statis buf");
			}
			break;
		}
	case AF_DATA_IMG_BLK: {
			if (cxt->ops.af_ops.ioctrl) {
				struct afctrl_ae_info *ae_info = &cxt->ae_info;
				struct afctrl_awb_info *awb_info = &cxt->awb_info;
				if (1 == ae_info->is_update) {
					ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_SET_AE_INFO, (void *)ae_info, NULL);
					ISP_TRACE_IF_FAIL(ret, ("AF_CMD_SET_AE_INFO fail "));
					ae_info->is_update = 0;
				}

				if (awb_info->is_update) {
					ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_SET_AWB_INFO, (void *)awb_info, NULL);
					ISP_TRACE_IF_FAIL(ret, ("AF_CMD_SET_AWB_INFO fail "));
					awb_info->is_update = 0;
				}
			}
			calc_param.data_type = AF_DATA_IMG_BLK;
			if (cxt->ops.af_ops.process) {
				ret = cxt->ops.af_ops.process(cxt->af_cxt.handle, (void *)&calc_param, (void *)&calc_result);
			}
			break;
		}
	case AF_DATA_AE:{
			/*struct af_ae_info ae_info;
			struct ae_calc_out *ae_result = (struct ae_calc_out *)in_ptr;
			cmr_u32 line_time = ae_result->line_time;
			cmr_u32 frame_len = ae_result->frame_line;
			cmr_u32 dummy_line = ae_result->cur_dummy;
			cmr_u32 exp_line = ae_result->cur_exp_line;
			cmr_u32 frame_time;

			memset((void *)&ae_info, 0, sizeof(ae_info));
			ae_info.exp_time = ae_result->cur_exp_line * line_time / 10;
			ae_info.gain = ae_result->cur_again;
			frame_len = (frame_len > (exp_line + dummy_line)) ? frame_len : (exp_line + dummy_line);
			frame_time = frame_len * line_time / 10;
			frame_time = frame_time > 0 ? frame_time : 1;
			ae_info.cur_fps = 1000000 / frame_time;
			ae_info.cur_lum = ae_result->cur_lum;
			ae_info.target_lum = 128;
			ae_info.is_stable = ae_result->is_stab;
			ae_info.cur_index = ae_result->cur_index;
			ae_info.cur_ev = ae_result->cur_ev;
			ae_info.cur_dgain = ae_result->cur_dgain;
			ae_info.cur_iso = ae_result->cur_iso;
			ae_info.flag = ae_result->flag;
			ae_info.ae_data = ae_result->ae_data;
			ae_info.ae_data_size = ae_result->ae_data_size;
			ae_info.target_lum_ori = ae_result->target_lum_ori;
			ae_info.flag4idx = ae_result->flag4idx;
			ae_info.log_ae.log = ae_result->log_ae.log;
			ae_info.log_ae.size = ae_result->log_ae.size;
			calc_param.data_type = AF_DATA_AE;
			calc_param.data = (void *)(&ae_info);
			if (cxt->ops.af_ops.process)
				ret = cxt->ops.af_ops.process(cxt->af_cxt.handle, (void *)&calc_param, (void *)&calc_result);*/
			break;
		}
	case AF_DATA_FD:
		break;
	default:
		break;
	}

	ISP_LOGV("done %ld", ret);
	return ret;
}

static cmr_int ispalg_pdaf_process(cmr_handle isp_alg_handle, cmr_u32 data_type, void *in_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_statis_info *statis_info = NULL;
	cmr_u32 k_addr = 0;
	cmr_u32 u_addr = 0;
	cmr_s32 i = 0;
	struct pdaf_ctrl_process_in pdaf_param_in;
	struct pdaf_ctrl_param_out pdaf_param_out;
	struct isp_statis_buf_input statis_buf;
	UNUSED(data_type);

	memset((void *)&pdaf_param_in, 0x00, sizeof(pdaf_param_in));
	ISP_CHECK_HANDLE_VALID(isp_alg_handle);

	statis_info = (struct isp_statis_info *)in_ptr;
	k_addr = statis_info->phy_addr;
	u_addr = statis_info->vir_addr;

	memset((void *)&statis_buf, 0, sizeof(statis_buf));

	pdaf_param_in.u_addr = u_addr;

	if (cxt->ops.pdaf_ops.ioctrl)
		cxt->ops.pdaf_ops.ioctrl(cxt->pdaf_cxt.handle, PDAF_CTRL_CMD_GET_BUSY, NULL, &pdaf_param_out);

	ISP_LOGV("pdaf_is_busy=%d\n", pdaf_param_out.is_busy);
	if (!pdaf_param_out.is_busy) {
		if (cxt->ops.pdaf_ops.process)
			ret = cxt->ops.pdaf_ops.process(cxt->pdaf_cxt.handle, &pdaf_param_in, NULL);
	}

	statis_buf.buf_size = statis_info->buf_size;
	statis_buf.phy_addr = statis_info->phy_addr;
	statis_buf.vir_addr = statis_info->vir_addr;
	statis_buf.kaddr[0] = statis_info->kaddr[0];
	statis_buf.kaddr[1] = statis_info->kaddr[1];
	statis_buf.buf_property = ISP_PDAF_BLOCK;
	statis_buf.buf_flag = 1;
	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_STSTIS_BUF, &statis_buf, NULL);

	ISP_LOGV("done %ld", ret);
	return ret;
}

static cmr_u32 ispalg_binning_data_cvt(cmr_u32 bayermode, cmr_u32 width, cmr_u32 height, cmr_u16 * raw_in, struct isp_binning_statistic_info *binning_info)
{
	cmr_u32 ret = 0;
	cmr_u32 i, j;
	cmr_u32 *binning_r = binning_info->r_info;
	cmr_u32 *binning_g = binning_info->g_info;
	cmr_u32 *binning_b = binning_info->b_info;
	cmr_u32 blk_id = 0;
	cmr_u32 *binning_block[4] = { binning_g, binning_r, binning_b, binning_g };
	cmr_u16 pixel_type;

	if (NULL == raw_in || NULL == binning_r || NULL == binning_g || NULL == binning_b) {
		ISP_LOGE("fail to check input param");
		return -1;
	}

	if ((width % 2 != 0) || (height % 2 != 0)) {
		ISP_LOGE("fail to Alignment");
		return -1;
	}

	for (i = 0; i < height; i += 2) {
		for (j = 0; j < width; j += 2) {
			binning_r[blk_id] = 0;
			binning_g[blk_id] = 0;
			binning_b[blk_id] = 0;
			pixel_type = bayermode ^ ((((i) & 1) << 1) | ((j) & 1));
			binning_block[pixel_type][blk_id] += raw_in[i * width + j];

			pixel_type = bayermode ^ ((((i) & 1) << 1) | ((j + 1) & 1));
			binning_block[pixel_type][blk_id] += raw_in[i * width + j + 1];

			pixel_type = bayermode ^ ((((i + 1) & 1) << 1) | ((j) & 1));
			binning_block[pixel_type][blk_id] += raw_in[(i + 1) * width + j];

			pixel_type = bayermode ^ ((((i + 1) & 1) << 1) | ((j + 1) & 1));
			binning_block[pixel_type][blk_id] += raw_in[(i + 1) * width + j + 1];

			binning_g[blk_id] = (binning_g[blk_id] + 1) >> 1;

			blk_id++;
		}
	}

	binning_info->binning_size.w = width / 2;
	binning_info->binning_size.h = height / 2;
	return ret;
}

static void pixel_split_sse(unsigned int *src, unsigned short *dst, int num) {
    unsigned int *tmp_src = src;
    unsigned short *tmp_dst = dst;
    int i;

    if (!tmp_src || !tmp_dst || num < 0) {
        return;
    }

    __m128i mask = _mm_set1_epi32(0x3FF);

    int mlt_8 = num / 8;
    int remain = num % 8;

    //#pragma omp parallel for
    for (i = 0; i < mlt_8; i++) {
        __m128i xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm8;

        // load 4 unsigned int
        //xmm3 = _mm_load_si128((__m128i *)tmp_src);
        xmm3 = _mm_stream_load_si128((__m128i *)tmp_src);
        xmm1 = _mm_and_si128(xmm3, mask);  // 1 0, 4 0, 7 0, 10 0
        xmm2 = _mm_srli_epi32(xmm3, 10);
        xmm2 = _mm_and_si128(xmm2, mask);  // 2 0, 5 0, 8 0, 11 0
        xmm3 = _mm_srli_epi32(xmm3, 20);
        xmm3 = _mm_and_si128(xmm3, mask);  // 3 0, 6 0, 9 0, 12 0
        xmm4 = _mm_or_si128(xmm1, _mm_slli_si128(xmm2, 2)); // 1 2, 4 5, 7 8, 10 11
        xmm5 = _mm_or_si128(xmm3, _mm_srli_si128(xmm1, 2)); // 3 4, 6 7, 9 10, 12 0
        xmm6 = _mm_or_si128(xmm2, _mm_slli_si128(xmm3, 2)); // 2 3, 5 6, 8 9; 11 12
        xmm1 = _mm_unpacklo_epi32(xmm4, xmm5); // 1 2, 3 4, 4 5, 6 7
        xmm2 = _mm_unpacklo_epi32(_mm_srli_si128(xmm6, 4), _mm_srli_si128(xmm4, 8)); // 5 6, 7 8, 8 9, 10 11
        xmm8 = _mm_unpackhi_epi32(xmm5, _mm_srli_si128(xmm6, 4)); // 9 10, 11 12, 12 0, 0 0
        xmm7 = _mm_unpacklo_epi64(xmm1, xmm2); // 1 2, 3 4, 5 6, 7 8

        tmp_src += 4;

        // load 4 unsigned int
        //xmm3 = _mm_load_si128((__m128i *)tmp_src);
        xmm3 = _mm_stream_load_si128((__m128i *)tmp_src);
        xmm1 = _mm_and_si128(xmm3, mask);  // 13 0, 16 0, 19 0, 22 0
        xmm2 = _mm_srli_epi32(xmm3, 10);
        xmm2 = _mm_and_si128(xmm2, mask);  // 14 0, 17 0, 20 0, 23 0
        xmm3 = _mm_srli_epi32(xmm3, 20);
        xmm3 = _mm_and_si128(xmm3, mask);  // 15 0, 18 0, 21 0, 24 0
        xmm4 = _mm_or_si128(xmm1, _mm_slli_si128(xmm2, 2)); // 13 14, 16 17, 19 20, 22 23
        xmm5 = _mm_or_si128(xmm3, _mm_srli_si128(xmm1, 2)); // 15 16, 18 19, 21 22, 24 0
        xmm6 = _mm_or_si128(xmm2, _mm_slli_si128(xmm3, 2)); // 14 15, 17 18, 20 21, 23 24
        xmm1 = _mm_unpacklo_epi32(xmm4, xmm5); // 13 14, 15 16, 16 17, 18 19
        xmm2 = _mm_unpacklo_epi32(_mm_srli_si128(xmm6, 4), _mm_srli_si128(xmm4, 8)); // 17 18, 19 20, 20 21, 22 23
        xmm3 = _mm_unpackhi_epi32(xmm5, _mm_srli_si128(xmm6, 4)); // 21 22, 23 24, 24 0, 0 0
        xmm8 = _mm_unpacklo_epi64(xmm8, xmm1); // 9 10, 11 12, 13 14, 15 16
        xmm1 = _mm_unpacklo_epi64(xmm2, xmm3); // 17 18, 19 20, 21 22, 23 24

        //_mm_store_si128((__m128i *)tmp_dst, xmm7);
        _mm_stream_si128((__m128i *)tmp_dst, xmm7);
        tmp_dst += 8;
        //_mm_store_si128((__m128i *)tmp_dst, xmm8);
        _mm_stream_si128((__m128i *)tmp_dst, xmm8);
        tmp_dst += 8;
        //_mm_store_si128((__m128i *)tmp_dst, xmm1);
        _mm_stream_si128((__m128i *)tmp_dst, xmm1);
        tmp_dst += 8;

        tmp_src += 4;
    }

    for (i = 0; i < remain; i++) {
        unsigned int val = *tmp_src++;
        *tmp_dst++ = val & 0x3FF;
        *tmp_dst++ = (val >> 10) & 0x3FF;
        *tmp_dst++ = (val >> 20) & 0x3FF;
    }
}

/*
p_dst_sse: the temp buffer to store the 16bit raw data.
p_dst_sse = (unsigned short *)memalign(16, 3 * pixel_num * sizeof(unsigned short));
*/
static cmr_u16 *binning_pixel_split(cmr_u16 *binning_statis_ptr, cmr_u32 *src_ptr, cmr_u32 pixel_num)
{
	cmr_u16 *p_dst_sse = binning_statis_ptr;

	if (p_dst_sse == NULL) {
		ISP_LOGE("error binning_statis_ptr null");
		return NULL;
	}

	pixel_split_sse(src_ptr, p_dst_sse, pixel_num);
	return p_dst_sse;
}

static cmr_int ispalg_binning_stats_parser(cmr_handle isp_alg_handle, void *data)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_statis_buf_input statis_buf;
	struct isp_statis_info *statis_info = (struct isp_statis_info *)data;
	cmr_u64 k_addr = 0;
	cmr_u64 u_addr = 0;
	cmr_u32 val = 0;
	cmr_u32 last_val0 = 0;
	cmr_u32 last_val1 = 0;
	cmr_u32 src_w = cxt->commn_cxt.src.w;
	cmr_u32 src_h = cxt->commn_cxt.src.h;
	cmr_u32 binning_w = 0;
	cmr_u32 binning_h = 0;
	cmr_u32 double_binning_num = 0;
	cmr_u32 remainder = 0;
	cmr_u16 *raw_data_ptr = NULL;
	cmr_u16 *binning_img_ptr = NULL;
	cmr_u32 bayermode = cxt->commn_cxt.image_pattern;
	struct isp_pm_param_data param_data;
	struct isp_pm_ioctl_input input = { NULL, 0 };
	struct isp_pm_ioctl_output output = { NULL, 0 };
	struct isp_dev_binning4awb_info *binning_info = NULL;

	ISP_CHECK_HANDLE_VALID(isp_alg_handle);
	k_addr = statis_info->phy_addr;
	u_addr = statis_info->vir_addr;

	memset(&param_data, 0, sizeof(param_data));

	BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_ISP_SETTING, ISP_BLK_BINNING4AWB, NULL, 0);
	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_SINGLE_SETTING, &input, &output);

	if (ISP_SUCCESS != ret || NULL == output.param_data) {
		ISP_LOGE("fail to check output.param_data\n");
		return ret;
	}

	binning_info = (struct isp_dev_binning4awb_info *)output.param_data->data_ptr;
	if (NULL == binning_info) {
		ISP_LOGE("Get param failed\n");
		return ISP_ERROR;
	}

	binning_w = (src_w >> binning_info->hx) & ~0x1;
	binning_h = (src_h >> binning_info->vx) & ~0x1;
	double_binning_num = binning_w * binning_h / 6 * 2;
	remainder = binning_w * binning_h % 6;

	ISP_LOGV("bayer: %d, src_size=(%d,%d), hx=%d, vx=%d, binning:w:%d, binning_h:%d, double_binning_num:%d",
				bayermode, src_w, src_h,
				binning_info->hx, binning_info->vx,
				binning_w, binning_h, double_binning_num);

	raw_data_ptr = binning_pixel_split(cxt->binning_statis_ptr, (cmr_u32 *)u_addr, double_binning_num);

	binning_img_ptr = raw_data_ptr + double_binning_num * 3;
	last_val0 = *((cmr_u32 *) u_addr + double_binning_num);
	last_val1 = *((cmr_u32 *) u_addr + double_binning_num + 1);

	switch (remainder) {
	case 1:{
			*binning_img_ptr++ = last_val0 & 0x3FF;
			break;
		}
	case 2:{
			*binning_img_ptr++ = last_val0 & 0x3FF;
			*binning_img_ptr++ = (last_val0 >> 10) & 0x3FF;
			break;
		}
	case 3:{
			*binning_img_ptr++ = last_val0 & 0x3FF;
			*binning_img_ptr++ = (last_val0 >> 10) & 0x3FF;
			*binning_img_ptr++ = (last_val0 >> 20) & 0x3FF;
			break;
		}
	case 4:{
			*binning_img_ptr++ = last_val0 & 0x3FF;
			*binning_img_ptr++ = (last_val0 >> 10) & 0x3FF;
			*binning_img_ptr++ = (last_val0 >> 20) & 0x3FF;
			*binning_img_ptr++ = last_val1 & 0x3FF;
			break;
		}
	case 5:{
			*binning_img_ptr++ = last_val0 & 0x3FF;
			*binning_img_ptr++ = (last_val0 >> 10) & 0x3FF;
			*binning_img_ptr++ = (last_val0 >> 20) & 0x3FF;
			*binning_img_ptr++ = last_val1 & 0x3FF;
			*binning_img_ptr++ = (last_val1 >> 10) & 0x3FF;
			break;
		}
	default:
		break;
	}

	if (raw_data_ptr) {
		ispalg_binning_data_cvt(bayermode, binning_w, binning_h, raw_data_ptr, &cxt->binning_stats);
	}

	ISP_LOGV("binning_stats_size=(%d, %d)\n", cxt->binning_stats.binning_size.w, cxt->binning_stats.binning_size.h);

	memset((void *)&statis_buf, 0, sizeof(statis_buf));
	statis_buf.buf_size = statis_info->buf_size;
	statis_buf.phy_addr = statis_info->phy_addr;
	statis_buf.vir_addr = statis_info->vir_addr;
	statis_buf.buf_property = ISP_BINNING_BLOCK, statis_buf.buf_flag = 1;
	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_STSTIS_BUF, &statis_buf, NULL);
	if (ret) {
		ISP_LOGE("fail to set statis buf");
	}

	ISP_LOGV("done %ld", ret);
	return ret;
}

static cmr_int ispalg_evt_process_cb(cmr_handle isp_alg_handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct ips_out_param callback_param = { 0x00 };
	struct isp_interface_param_v1 *interface_ptr_v1 = &cxt->commn_cxt.interface_param_v1;

	ISP_LOGV("isp start raw proc callback\n");
	if (NULL != cxt->commn_cxt.callback) {
		callback_param.output_height = interface_ptr_v1->data.input_size.h;
		ISP_LOGV("callback ISP_PROC_CALLBACK");
		cxt->commn_cxt.callback(cxt->commn_cxt.caller_id,
					ISP_CALLBACK_EVT | ISP_PROC_CALLBACK,
					(void *)&callback_param,
					sizeof(struct ips_out_param));
	}

	ISP_LOGV("isp end raw proc callback\n");
	return ret;
}

void ispalg_dev_evt_cb(cmr_int evt, void *data, void *privdata)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)privdata;
	CMR_MSG_INIT(message);

	if (!cxt) {
		ISP_LOGE("fail to check input param");
		return;
	}

	message.msg_type = evt;
	message.sub_msg_type = 0;
	message.sync_flag = CMR_MSG_SYNC_NONE;
	message.alloc_flag = 1;
	message.data = data;
	if (ISP_CTRL_EVT_AF == message.msg_type)
		ret = cmr_thread_msg_send(cxt->thr_afhandle, &message);
	else if (ISP_PROC_AFL_DONE == message.msg_type)
		ret = cmr_thread_msg_send(cxt->thr_afl_handle, &message);
	else
		ret = cmr_thread_msg_send(cxt->thr_handle, &message);
	if (ret) {
		ISP_LOGE("fail to send a message, evt is %ld", evt);
		free(message.data);
	}
}

cmr_int ispalg_afl_thread_proc(struct cmr_msg *message, void *p_data)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)p_data;

	if (!message || !p_data) {
		ISP_LOGE("fail to check input param ");
		goto exit;
	}
	ISP_LOGV("message.msg_type 0x%x, data %p", message->msg_type, message->data);

	switch (message->msg_type) {
	case ISP_PROC_AFL_DONE:
		ret = ispalg_afl_process((cmr_handle) cxt, message->data);
		break;
	default:
		ISP_LOGV("don't support msg, 0x%x", message->msg_type);
		break;
	}
exit:
	ISP_LOGV("done %ld", ret);
	return ret;

}

cmr_int ispalg_afthread_proc(struct cmr_msg *message, void *p_data)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)p_data;

	if (!message || !p_data) {
		ISP_LOGE("fail to check input param ");
		goto exit;
	}
	ISP_LOGV("message.msg_type 0x%x, data %p", message->msg_type, message->data);

	switch (message->msg_type) {
	case ISP_CTRL_EVT_AF:
		ret = ispalg_af_process((cmr_handle) cxt, message->sub_msg_type, message->data);
		break;
	default:
		ISP_LOGV("don't support msg");
		break;
	}
exit:
	ISP_LOGV("done %ld", ret);
	return ret;

}

cmr_int isp_alg_thread_proc(struct cmr_msg *message, void *p_data)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)p_data;

	if (!message || !p_data) {
		ISP_LOGE("fail to check input param ");
		goto exit;
	}
	ISP_LOGV("message.msg_type 0x%x, data %p", message->msg_type, message->data);

	switch (message->msg_type) {
	case ISP_CTRL_EVT_TX:
		ret = ispalg_evt_process_cb((cmr_handle) cxt);
		break;
	case ISP_CTRL_EVT_AE:
		ret = ispalg_aem_stats_parser((cmr_handle) cxt, message->data);
		break;
	case ISP_CTRL_EVT_SOF:
		if (cxt->gamma_sof_cnt_eb) {
			cxt->gamma_sof_cnt++;
			if (cxt->gamma_sof_cnt >= 2) {
				cxt->update_gamma_eb = 1;
			}
		}

		ret = ispalg_ae_process((cmr_handle) cxt);
		if (ret)
			ISP_LOGE("fail to start ae process");
		ret = ispalg_awb_process((cmr_handle) cxt);
		if (ret)
			ISP_LOGE("fail to start awb process");
		cxt->aem_is_update = 0;
		ret = ispalg_handle_sensor_sof((cmr_handle) cxt);
		break;
	case ISP_CTRL_EVT_BINNING:
		ret = ispalg_binning_stats_parser((cmr_handle) cxt, message->data);
		break;
	case ISP_CTRL_EVT_PDAF:
		ret = ispalg_pdaf_process((cmr_handle) cxt, message->sub_msg_type, message->data);
		break;
	default:
		ISP_LOGV("don't support msg");
		break;
	}
exit:
	ISP_LOGV("done %ld", ret);
	return ret;
}

cmr_int isp_alg_create_thread(cmr_handle isp_alg_handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	ret = cmr_thread_create(&cxt->thr_handle, ISP_THREAD_QUEUE_NUM, isp_alg_thread_proc, (void *)cxt);
	if (CMR_MSG_SUCCESS != ret) {
		ISP_LOGE("fail to create process thread");
		ret = -ISP_ERROR;
	}
	ret = cmr_thread_set_name(cxt->thr_handle, "algfw");
	if (CMR_MSG_SUCCESS != ret) {
		ISP_LOGE("fail to set fw name");
		ret = -ISP_ERROR;
	}

	ret = cmr_thread_create(&cxt->thr_afhandle, ISP_THREAD_QUEUE_NUM, ispalg_afthread_proc, (void *)cxt);
	if (CMR_MSG_SUCCESS != ret) {
		ISP_LOGE("fail to create process thread");
		ret = -ISP_ERROR;
	}
	ret = cmr_thread_set_name(cxt->thr_afhandle, "afstats");
	if (CMR_MSG_SUCCESS != ret) {
		ISP_LOGE("fail to set af name");
		ret = -ISP_ERROR;
	}

	ret = cmr_thread_create(&cxt->thr_afl_handle, ISP_THREAD_QUEUE_NUM, ispalg_afl_thread_proc, (void *)cxt);
	if (CMR_MSG_SUCCESS != ret) {
		ISP_LOGE("fail to create afl process thread");
		ret = -ISP_ERROR;
	}
	ret = cmr_thread_set_name(cxt->thr_afl_handle, "afl_stats");
	if (CMR_MSG_SUCCESS != ret) {
		ISP_LOGE("fail to set af name");
		ret = -ISP_ERROR;
	}

	return ret;
}

cmr_int isp_alg_destroy_thread_proc(cmr_handle isp_alg_handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	if (!isp_alg_handle) {
		ISP_LOGE("fail to check isp_alg_handle");
		ret = ISP_ERROR;
		goto exit;
	}

	if (cxt->thr_handle) {
		ret = cmr_thread_destroy(cxt->thr_handle);
		if (!ret) {
			cxt->thr_handle = (cmr_handle) NULL;
		} else {
			ISP_LOGE("fail to destroy process thread");
		}
	}

	if (cxt->thr_afhandle) {
		ret = cmr_thread_destroy(cxt->thr_afhandle);
		if (!ret) {
			cxt->thr_afhandle = (cmr_handle) NULL;
		} else {
			ISP_LOGE("fail to destroy process thread");
		}
	}

	if (cxt->thr_afl_handle) {
		ret = cmr_thread_destroy(cxt->thr_afl_handle);
		if (!ret) {
			cxt->thr_afl_handle = (cmr_handle) NULL;
		} else {
			ISP_LOGE("fail to destroy process thread");
		}
	}

exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

static cmr_u32 ispalg_get_param_index(struct sensor_raw_resolution_info *input_size_trim, struct isp_size *size)
{
	cmr_u32 param_index = 0x01;
	cmr_u32 i;

	for (i = 0x01; i < ISP_INPUT_SIZE_NUM_MAX; i++) {
		if (size->h == input_size_trim[i].height) {
			param_index = i;
			break;
		}
	}

	return param_index;
}

static cmr_int ispalg_ae_init(struct isp_alg_fw_context *cxt)
{
	cmr_int ret = ISP_SUCCESS;
	struct ae_init_in ae_input;
	struct isp_pm_ioctl_output output;
	struct isp_pm_param_data *param_data = NULL;
	struct ae_init_out result;
	struct isp_pm_param_data flash_param_data;
	struct isp_pm_ioctl_input input = { NULL, 0 };
	cmr_u32 num = 0;
	cmr_u32 i = 0;
	cmr_u32 dflash_num = 0;

	memset(&flash_param_data, 0, sizeof(flash_param_data));
	BLOCK_PARAM_CFG(input, flash_param_data, ISP_PM_BLK_ISP_SETTING, ISP_BLK_FLASH_CALI, NULL, 0);


	memset(&output, 0, sizeof(output));
	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_SINGLE_SETTING, &input, &output);
	if (ISP_SUCCESS == ret && 1 == output.param_num) {
	cxt->pm_flash_info = (struct isp_flash_param *)output.param_data->data_ptr;
	} else {
	ret = ISP_ERROR;
	}


	ISP_LOGI("flash param rgb ratio = (%d,%d,%d), lum_ratio = %d",
	cxt->pm_flash_info->cur.r_ratio, cxt->pm_flash_info->cur.g_ratio, cxt->pm_flash_info->cur.b_ratio, cxt->pm_flash_info->cur.lum_ratio);


	memset((void *)&result, 0, sizeof(result));
	memset(&output, 0, sizeof(output));
	memset((void *)&ae_input, 0, sizeof(ae_input));

	/*get dual flash tuning parameters*/
	memset(&output, 0, sizeof(output));
	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_INIT_DUAL_FLASH, NULL, &output);
	if (ISP_SUCCESS != ret) {
		ISP_LOGE("fail to get dual flash param");
		return ret;
	}

	if (0 == output.param_num) {
		ISP_LOGE("fail to check param: dual flash param num=%d", output.param_num);
		return ISP_ERROR;
	}

	param_data = output.param_data;
	for (i = 0; i < output.param_num; ++i) {
		if (NULL != param_data->data_ptr) {
			ae_input.flash_tuning[dflash_num].param = param_data->data_ptr;
			ae_input.flash_tuning[dflash_num].size = param_data->data_size;
			++dflash_num;
		}
		++param_data;
	}

	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_INIT_AE, NULL, &output);
	if (ISP_SUCCESS != ret) {
		ISP_LOGE("fail to get ae init param");
		return ret;
	}

	if (0 == output.param_num) {
		ISP_LOGE("fail to check param: ae param num=%d", output.param_num);
		return ISP_ERROR;
	}

	param_data = output.param_data;
	ae_input.has_force_bypass = param_data->user_data[0];
	for (i = 0; i < output.param_num; ++i) {
		if (NULL != param_data->data_ptr) {
			ae_input.param[num].param = param_data->data_ptr;
			ae_input.param[num].size = param_data->data_size;
			++num;
		}
		++param_data;
	}
	ae_input.param_num = num;
	ae_input.dflash_num = dflash_num;
	ae_input.resolution_info.frame_size.w = cxt->commn_cxt.src.w;
	ae_input.resolution_info.frame_size.h = cxt->commn_cxt.src.h;
	ae_input.resolution_info.frame_line = cxt->commn_cxt.input_size_trim[1].frame_line;
	ae_input.resolution_info.line_time = cxt->commn_cxt.input_size_trim[1].line_time;
	ae_input.resolution_info.sensor_size_index = 1;
	ae_input.camera_id = cxt->camera_id;
	ae_input.lib_param = cxt->lib_use_info->ae_lib_info;
	ae_input.caller_handle = (cmr_handle) cxt;
	ae_input.ae_set_cb = ispalg_ae_set_cb;
	cxt->ae_cxt.win_num.w =32;
	cxt->ae_cxt.win_num.h = 32;
	ae_input.monitor_win_num.w = cxt->ae_cxt.win_num.w ;
	ae_input.monitor_win_num.h = cxt->ae_cxt.win_num.h;


	if (cxt->is_multi_mode == ISP_DUAL_NORMAL) {
		// TODO: change ae_role here
		ae_input.sensor_role = cxt->is_master;
		ae_input.is_multi_mode = cxt->is_multi_mode;
		ISP_LOGI("sensor_role=%d, is_multi_mode=%d, ae_role=%d",
			cxt->is_master, cxt->is_multi_mode , ae_input.sensor_role);

		/* save otp info */
		if (cxt->is_multi_mode) {
			ae_input.otp_info_ptr = cxt->otp_data;
			ae_input.is_master = cxt->is_master;
		}

		ae_input.ptr_isp_br_ioctrl = isp_br_ioctrl;
	}


	if (cxt->ops.ae_ops.init) {
		ret = cxt->ops.ae_ops.init(&ae_input, &cxt->ae_cxt.handle,(cmr_handle)&result);
		ISP_TRACE_IF_FAIL(ret, ("fail to do ae_ctrl_init"));
	}
	cxt->ae_cxt.flash_version = result.flash_ver;
	if (cxt->ops.ae_ops.ioctrl)
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_FLASH_ON_OFF_THR, (void *)&cxt->pm_flash_info->cur.auto_flash_thr, NULL);
	return ret;
}

static cmr_int ispalg_awb_init(struct isp_alg_fw_context *cxt)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_pm_ioctl_input input;
	struct isp_pm_ioctl_output output;
	struct awb_ctrl_init_param param;
	struct ae_monitor_info info;

	memset((void *)&input, 0, sizeof(input));
	memset((void *)&output, 0, sizeof(output));
	memset((void *)&param, 0, sizeof(param));
	memset((void *)&info, 0, sizeof(info));

	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_INIT_AWB, &input, &output);
	ISP_TRACE_IF_FAIL(ret, ("fail to get awb init param"));

	if (cxt->ops.ae_ops.ioctrl) {
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_MONITOR_INFO, NULL, (void *)&info);
		ISP_RETURN_IF_FAIL(ret, ("fail to get ae monitor info"));
	}
	if (AL_AE_LIB == cxt->lib_use_info->ae_lib_info.product_id) {
		void *ais_handle = NULL;
		if (cxt->ops.ae_ops.ioctrl) {
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_AIS_HANDLE, NULL, (void *)&ais_handle);
			ISP_TRACE_IF_FAIL(ret, ("fail to get ae ais handle"));
		}
		param.priv_handle = ais_handle;
		param.awb_enable = 1;
	} else {
		//if use AIS, AWB this does not  need for awb_ctrl
		param.camera_id = cxt->camera_id;
		param.base_gain = 1024;
		param.awb_enable = 1;
		param.wb_mode = 0;
		param.stat_img_format = AWB_CTRL_STAT_IMG_CHN;
		param.stat_img_size.w = info.win_num.w;
		param.stat_img_size.h = info.win_num.h;
		param.stat_win_size.w = info.win_size.w;
		param.stat_win_size.h = info.win_size.h;

		param.stat_img_size.w = cxt->binning_stats.binning_size.w;
		param.stat_img_size.h = cxt->binning_stats.binning_size.h;

		param.tuning_param = output.param_data->data_ptr;
		param.param_size = output.param_data->data_size;
		param.lib_param = cxt->lib_use_info->awb_lib_info;
		ISP_LOGV(" param addr is %p size %d", param.tuning_param, param.param_size);

		param.otp_info_ptr = cxt->otp_data;
		param.is_master = cxt->is_master;
	}

	if (cxt->ops.awb_ops.init) {
		ret = cxt->ops.awb_ops.init(&param, &cxt->awb_cxt.handle);
		ISP_TRACE_IF_FAIL(ret, ("failed to do awb_ctrl_init"));
	}
	ISP_LOGI("done %ld", ret);
	return ret;
}

static cmr_int ispalg_afl_init(struct isp_alg_fw_context *cxt, struct isp_alg_sw_init_in *input_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct afl_ctrl_init_in afl_input;

	if (!cxt || !input_ptr) {
		ret = ISP_PARAM_ERROR;
		goto exit;
	}

	afl_input.dev_handle = cxt->dev_access_handle;
	afl_input.size.w = input_ptr->size.w;
	afl_input.size.h = input_ptr->size.h;
	afl_input.vir_addr = &cxt->afl_cxt.vir_addr;
	afl_input.caller_handle = (cmr_handle) cxt;
	afl_input.afl_set_cb = ispalg_afl_set_cb;
	afl_input.version = cxt->afl_cxt.version;
	if (cxt->ops.afl_ops.init)
		ret = cxt->ops.afl_ops.init(&cxt->afl_cxt.handle, &afl_input);
	ISP_LOGI("done %ld, version:%d", ret, afl_input.version);
	return ret;
exit:
	ISP_LOGI("isp_afl_sw_init failed: done %ld, ", ret);
	return ret;
}

static cmr_int ispalg_smart_init(struct isp_alg_fw_context *cxt)
{
	cmr_int ret = ISP_SUCCESS;
	struct smart_init_param smart_init_param;
	struct isp_pm_ioctl_input pm_input;
	struct isp_pm_ioctl_output pm_output;
	cmr_u32 i = 0;

	memset(&pm_input, 0, sizeof(pm_input));
	memset(&pm_output, 0, sizeof(pm_output));

	cxt->smart_cxt.isp_smart_eb = 0;
	memset(&smart_init_param, 0, sizeof(smart_init_param));

	smart_init_param.camera_id = cxt->camera_id;

	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_INIT_SMART, &pm_input, &pm_output);
	if (ISP_SUCCESS == ret) {
		for (i = 0; i < pm_output.param_num; ++i) {
			smart_init_param.tuning_param[i].data.size = pm_output.param_data[i].data_size;
			smart_init_param.tuning_param[i].data.data_ptr = pm_output.param_data[i].data_ptr;
		}
	} else {
		ISP_LOGE("fail to get smart init param ");
		return ret;
	}
	smart_init_param.caller_handle = (cmr_handle)cxt;
	smart_init_param.smart_set_cb = ispalg_smart_set_cb;

	if (cxt->ops.smart_ops.init) {
		cxt->smart_cxt.handle = cxt->ops.smart_ops.init(&smart_init_param, NULL);
		if (NULL == cxt->smart_cxt.handle) {
			ISP_LOGE("fail to do smart init");
			return ret;
		}
	}
exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

static cmr_int ispalg_af_init(struct isp_alg_fw_context *cxt)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 is_af_support = 1;
	struct afctrl_init_in af_input;
	struct isp_pm_ioctl_input af_pm_input;
	struct isp_pm_ioctl_output af_pm_output;
	struct af_log_info af_param = {NULL, 0};
	struct isp_pm_param_data param_data;
	struct isp_pm_ioctl_input input = { NULL, 0 };
	struct isp_pm_ioctl_output output = { NULL, 0 };

	if (NULL == cxt || NULL == cxt->ioctrl_ptr)
		return ret;

	if (NULL == cxt->ioctrl_ptr->set_pos)
		is_af_support = 0;

	memset((void *)&af_input, 0, sizeof(af_input));
	memset((void *)&af_pm_input, 0, sizeof(af_pm_input));
	memset((void *)&af_pm_output, 0, sizeof(af_pm_output));

	af_input.camera_id = cxt->camera_id;
	af_input.lib_param = cxt->lib_use_info->af_lib_info;
	af_input.caller_handle = (cmr_handle) cxt;
	af_input.af_set_cb = ispalg_af_set_cb;
	af_input.src.w = cxt->commn_cxt.src.w;
	af_input.src.h = cxt->commn_cxt.src.h;
	af_input.is_supoprt = is_af_support;

	if(1 == is_af_support) {
		//get af tuning parameters
		memset((void *)&output, 0, sizeof(output));
		ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_INIT_AF_NEW, NULL, &output);
		if(ISP_SUCCESS == ret && NULL != output.param_data){
			af_input.aftuning_data = output.param_data[0].data_ptr;
			af_input.aftuning_data_len = output.param_data[0].data_size;
		}

		//get af trigger tuning parameters
		memset((void *)&output, 0, sizeof(output));
		ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_INIT_AFT, NULL, &output);
		if(ISP_SUCCESS == ret && NULL != output.param_data){
			af_input.afttuning_data = output.param_data[0].data_ptr;
			af_input.afttuning_data_len = output.param_data[0].data_size;
		}

		//get pdaf tuning parameters
		memset((void *)&output, 0, sizeof(output));
		memset(&param_data, 0, sizeof(param_data));
		BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_ISP_SETTING, ISP_BLK_PDAF_TUNE, NULL, 0);
		ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_SINGLE_SETTING, &input, &output);
		if(ISP_SUCCESS == ret && 1 == output.param_num && NULL != output.param_data){
			af_input.pdaftuning_data = output.param_data[0].data_ptr;
			af_input.pdaftuning_data_len = output.param_data[0].data_size;
		}
	}

	af_input.otp_info_ptr = cxt->otp_data;
	af_input.is_master = cxt->is_master;

	if (cxt->ops.af_ops.init) {
		ret = cxt->ops.af_ops.init(&af_input, &cxt->af_cxt.handle);
		ISP_TRACE_IF_FAIL(ret, ("fail to do af_ctrl_init"));
	}

	if (cxt->ops.af_ops.ioctrl) {
		ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_GET_AF_LOG_INFO, (void *)&af_param, NULL);
		cxt->af_cxt.log_af = af_param.log_cxt;
		cxt->af_cxt.log_af_size = af_param.log_len;
	}
	ISP_TRACE_IF_FAIL(ret, ("fail to do af_ctrl_init"));

	return ret;
}

static cmr_int ispalg_pdaf_init(struct isp_alg_fw_context *cxt, struct isp_alg_sw_init_in *input_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct pdaf_ctrl_init_in pdaf_input;
	struct pdaf_ctrl_init_out pdaf_output;

	memset(&pdaf_input, 0x00, sizeof(pdaf_input));
	memset(&pdaf_output, 0x00, sizeof(pdaf_output));

	pdaf_input.camera_id = cxt->camera_id;
	pdaf_input.caller_handle = (cmr_handle) cxt;
	pdaf_input.pdaf_support = cxt->pdaf_cxt.pdaf_support;
	pdaf_input.pdaf_set_cb = ispalg_pdaf_set_cb;
	pdaf_input.pd_info = input_ptr->pdaf_info;
	pdaf_input.sensor_max_size = input_ptr->sensor_max_size;
	pdaf_input.handle_pm = cxt->handle_pm;

	if (SENSOR_PDAF_TYPE3_ENABLE == cxt->pdaf_cxt.pdaf_support) {
		pdaf_input.otp_info_ptr = cxt->otp_data;
		pdaf_input.is_master= cxt->is_master;
	}


	if (cxt->ops.pdaf_ops.init)
		ret = cxt->ops.pdaf_ops.init(&pdaf_input, &pdaf_output, &cxt->pdaf_cxt.handle);

exit:
	if (ret) {
		ISP_LOGE("fail to do PDAF initialize");
	}
	ISP_LOGI("done %ld", ret);

	return ret;
}

static cmr_int ispalg_lsc_init(struct isp_alg_fw_context *cxt)
{
	cmr_u32 ret = ISP_SUCCESS;
	cmr_s32 i = 0;
	lsc_adv_handle_t lsc_adv_handle = NULL;
	struct lsc_adv_init_param lsc_param;
	cmr_handle pm_handle = cxt->handle_pm;
	cmr_u16 *lsc_table = NULL;

	struct isp_pm_ioctl_input io_pm_input;
	struct isp_pm_ioctl_output io_pm_output;
	struct isp_pm_param_data pm_param;
	struct isp_pm_ioctl_input get_pm_input;
	struct isp_pm_ioctl_output get_pm_output;
	struct isp_pm_ioctl_input pm_tab_input;
	struct isp_pm_ioctl_output pm_tab_output;
	struct isp_pm_param_data pm_tab_param;

	memset(&lsc_param, 0, sizeof(struct lsc_adv_init_param));
	memset(&io_pm_input, 0, sizeof(struct isp_pm_ioctl_input));
	memset(&io_pm_output, 0, sizeof(struct isp_pm_ioctl_output));
	memset(&pm_param, 0, sizeof(struct isp_pm_param_data));
	memset(&get_pm_input, 0, sizeof(struct isp_pm_ioctl_input));
	memset(&get_pm_output, 0, sizeof(struct isp_pm_ioctl_output));
	memset(&pm_tab_input, 0, sizeof(struct isp_pm_ioctl_input));
	memset(&pm_tab_output, 0, sizeof(struct isp_pm_ioctl_output));
	memset(&pm_tab_param, 0, sizeof(struct isp_pm_param_data));

	BLOCK_PARAM_CFG(pm_tab_input, pm_tab_param, ISP_PM_BLK_LSC_GET_LSCTAB, ISP_BLK_2D_LSC, NULL, 0);
	ret = isp_pm_ioctl(pm_handle, ISP_PM_CMD_GET_SINGLE_SETTING, (void *)&pm_tab_input, (void *)&pm_tab_output);
	cxt->lsc_cxt.lsc_tab_address = pm_tab_output.param_data->data_ptr;
	struct isp_2d_lsc_param *lsc_tab_param_ptr = (struct isp_2d_lsc_param *)(cxt->lsc_cxt.lsc_tab_address);

	BLOCK_PARAM_CFG(io_pm_input, pm_param, ISP_PM_BLK_LSC_INFO, ISP_BLK_2D_LSC, PNULL, 0);
	ret = isp_pm_ioctl(pm_handle, ISP_PM_CMD_GET_SINGLE_SETTING, (void *)&io_pm_input, (void *)&io_pm_output);
	struct isp_lsc_info *lsc_info = (struct isp_lsc_info *)io_pm_output.param_data->data_ptr;

	ret = isp_pm_ioctl(pm_handle, ISP_PM_CMD_GET_INIT_ALSC, &get_pm_input, &get_pm_output);
	if (ISP_SUCCESS != ret) {
		ISP_LOGE("fail to get alsc init param");
	}

	ISP_LOGV(" _alsc_init");

	if (get_pm_output.param_data->data_size != sizeof(struct lsc2_tune_param)) {
		lsc_param.tune_param_ptr = NULL;
		ISP_LOGE("fail to get alsc param from sensor file");
	} else {
		lsc_param.tune_param_ptr = get_pm_output.param_data->data_ptr;
	}

	//_alsc_set_param(&lsc_param);   // for LSC2.X neet to reopen

	lsc_param.otp_info_ptr = cxt->otp_data;
	lsc_param.is_master = cxt->is_master;

	for (i = 0; i < 9; i++) {
		lsc_param.lsc_tab_address[i] = lsc_tab_param_ptr->map_tab[i].param_addr;
	}

	lsc_param.img_height = lsc_tab_param_ptr->resolution.h;
	lsc_param.img_width = lsc_tab_param_ptr->resolution.w;
	lsc_param.gain_width = lsc_info->gain_w;
	lsc_param.gain_height = lsc_info->gain_h;
	lsc_param.lum_gain = (cmr_u16 *) lsc_info->data_ptr;
	lsc_param.grid = lsc_info->grid;
	lsc_param.camera_id = cxt->camera_id;
	lsc_param.lib_param = cxt->lib_use_info->lsc_lib_info;

	switch (cxt->commn_cxt.image_pattern) {
	case SENSOR_IMAGE_PATTERN_RAWRGB_GR:
		lsc_param.gain_pattern = LSC_GAIN_PATTERN_RGGB;
		break;

	case SENSOR_IMAGE_PATTERN_RAWRGB_R:
		lsc_param.gain_pattern = LSC_GAIN_PATTERN_GRBG;
		break;

	case SENSOR_IMAGE_PATTERN_RAWRGB_B:
		lsc_param.gain_pattern = LSC_GAIN_PATTERN_GBRG;
		break;

	case SENSOR_IMAGE_PATTERN_RAWRGB_GB:
		lsc_param.gain_pattern = LSC_GAIN_PATTERN_BGGR;
		break;

	default:
		break;
	}
	lsc_param.is_master     = cxt->is_master;
	lsc_param.is_multi_mode = cxt->is_multi_mode;

	lsc_table = lsc_param.lsc_otp_table_addr;
	if (NULL == cxt->lsc_cxt.handle) {
		if (cxt->ops.lsc_ops.init) {
			ret = cxt->ops.lsc_ops.init(&lsc_param, &lsc_adv_handle);
			if (NULL == lsc_adv_handle) {
				ISP_LOGE("fail to do lsc adv init");
				if (NULL != lsc_table)
					free(lsc_table);
				return ISP_ERROR;
			}
		}

		cxt->lsc_cxt.handle = lsc_adv_handle;
	}
	if (NULL != lsc_table)
		free(lsc_table);

	return ret;
}

static cmr_u32 isp_alg_sw_init(struct isp_alg_fw_context *cxt, struct isp_alg_sw_init_in *input_ptr)
{
	cmr_int ret = ISP_SUCCESS;

	ret = ispalg_ae_init(cxt);
	ISP_TRACE_IF_FAIL(ret, ("fail to do ae_init"));

	ret = ispalg_afl_init(cxt, input_ptr);
	ISP_RETURN_IF_FAIL(ret, ("fail to do afl_init"));

	ret = ispalg_awb_init(cxt);
	ISP_TRACE_IF_FAIL(ret, ("fail to do awb_init"));

	ret = ispalg_smart_init(cxt);
	ISP_TRACE_IF_FAIL(ret, ("fail to do smart_init"));

	ret = ispalg_af_init(cxt);
	ISP_TRACE_IF_FAIL(ret, ("fail to do af_init"));

	ret = ispalg_pdaf_init(cxt, input_ptr);
	ISP_TRACE_IF_FAIL(ret, ("fail to do pdaf_init"));

	ret = ispalg_lsc_init(cxt);
	ISP_TRACE_IF_FAIL(ret, ("fail to do lsc_init"));

	ISP_LOGI("done %ld", ret);
	return ret;
}

static cmr_int isp_pm_sw_init(cmr_handle isp_alg_handle, struct isp_init_param *input_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct sensor_raw_info *sensor_raw_info_ptr = (struct sensor_raw_info *)input_ptr->setting_param_ptr;
	struct sensor_version_info *version_info = PNULL;
	struct isp_pm_init_input input;
	struct isp_pm_init_output output;
	cmr_u32 i = 0;

	memset(&input, 0, sizeof(input));
	memset(&output, 0, sizeof(output));
	cxt->sn_cxt.sn_raw_info = sensor_raw_info_ptr;
	isp_pm_raw_para_update_from_file(sensor_raw_info_ptr);

	input.num = MAX_MODE_NUM;
	version_info = (struct sensor_version_info *)sensor_raw_info_ptr->version_info;
	input.sensor_name = version_info->sensor_ver_name.sensor_name;

	for (i = 0; i < MAX_MODE_NUM; i++) {
		input.tuning_data[i].data_ptr = sensor_raw_info_ptr->mode_ptr[i].addr;
		input.tuning_data[i].size = sensor_raw_info_ptr->mode_ptr[i].len;
		input.fix_data[i] = sensor_raw_info_ptr->fix_ptr[i];
	}
	input.nr_fix_info = &(sensor_raw_info_ptr->nr_fix);

	cxt->handle_pm = isp_pm_init(&input, &output);
	cxt->commn_cxt.multi_nr_flag = output.multi_nr_flag;
	cxt->commn_cxt.src.w = input_ptr->size.w;
	cxt->commn_cxt.src.h = input_ptr->size.h;
	cxt->camera_id = input_ptr->camera_id;

	cxt->commn_cxt.callback = input_ptr->ctrl_callback;
	cxt->commn_cxt.caller_id = input_ptr->oem_handle;
	cxt->commn_cxt.ops = input_ptr->ops;

	/* init sensor param */
	cxt->ioctrl_ptr = sensor_raw_info_ptr->ioctrl_ptr;
	cxt->commn_cxt.image_pattern = sensor_raw_info_ptr->resolution_info_ptr->image_pattern;
	memcpy(cxt->commn_cxt.input_size_trim,
	       sensor_raw_info_ptr->resolution_info_ptr->tab,
	       ISP_INPUT_SIZE_NUM_MAX * sizeof(struct sensor_raw_resolution_info));
	cxt->commn_cxt.param_index = ispalg_get_param_index(cxt->commn_cxt.input_size_trim, &input_ptr->size);


	return ret;
}

static cmr_u32 isp_alg_sw_deinit(cmr_handle isp_alg_handle)
{
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	if (cxt->ops.pdaf_ops.deinit)
		cxt->ops.pdaf_ops.deinit(&cxt->pdaf_cxt.handle);

	if (cxt->ops.af_ops.deinit)
		cxt->ops.af_ops.deinit(&cxt->af_cxt.handle);

	if (cxt->ops.ae_ops.deinit)
		cxt->ops.ae_ops.deinit(&cxt->ae_cxt.handle);

	if (cxt->ops.lsc_ops.deinit)
		cxt->ops.lsc_ops.deinit(&cxt->lsc_cxt.handle);

	if (cxt->ops.smart_ops.deinit)
		cxt->ops.smart_ops.deinit(&cxt->smart_cxt.handle, NULL, NULL);

	if (cxt->ops.awb_ops.deinit)
		cxt->ops.awb_ops.deinit(&cxt->awb_cxt.handle);

	if (cxt->ops.afl_ops.deinit)
		cxt->ops.afl_ops.deinit(&cxt->afl_cxt.handle);

	ISP_LOGI("done");
	return ISP_SUCCESS;
}

static cmr_int load_ispalg_library(cmr_handle adpt_handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)adpt_handle;

	ISP_CHECK_HANDLE_VALID(adpt_handle);
	cxt->ispalg_lib_handle = dlopen(PDLIB_PATH, RTLD_NOW);
	if (!cxt->ispalg_lib_handle) {
		ISP_LOGE("failed to dlopen");
		goto error_dlopen;
	}
	/*init af_ctrl_ops*/
	cxt->ops.af_ops.init = dlsym(cxt->ispalg_lib_handle, "af_ctrl_init");
	if (!cxt->ops.af_ops.init) {
		ISP_LOGE("failed to dlsym af_ops.init");
		goto error_dlsym;
	}
	cxt->ops.af_ops.deinit = dlsym(cxt->ispalg_lib_handle, "af_ctrl_deinit");
	if (!cxt->ops.af_ops.deinit) {
		ISP_LOGE("failed to dlsym af_ops.deinit");
		goto error_dlsym;
	}
	cxt->ops.af_ops.process = dlsym(cxt->ispalg_lib_handle, "af_ctrl_process");
	if (!cxt->ops.af_ops.process) {
		ISP_LOGE("failed to dlsym af_ops.process");
		goto error_dlsym;
	}
	cxt->ops.af_ops.ioctrl= dlsym(cxt->ispalg_lib_handle, "af_ctrl_ioctrl");
	if (!cxt->ops.af_ops.ioctrl) {
		ISP_LOGE("failed to dlsym af_ops.ioctrl");
		goto error_dlsym;
	}
	/*init afl_ctrl_ops*/
	cxt->ops.afl_ops.init = dlsym(cxt->ispalg_lib_handle, "afl_ctrl_init");
	if (!cxt->ops.afl_ops.init) {
		ISP_LOGE("failed to dlsym afl_ops.init");
		goto error_dlsym;
	}
	cxt->ops.afl_ops.deinit = dlsym(cxt->ispalg_lib_handle, "afl_ctrl_deinit");
	if (!cxt->ops.afl_ops.deinit) {
		ISP_LOGE("failed to dlsym afl_ops.deinit");
		goto error_dlsym;
	}
	cxt->ops.afl_ops.process = dlsym(cxt->ispalg_lib_handle, "afl_ctrl_process");
	if (!cxt->ops.afl_ops.process) {
		ISP_LOGE("failed to dlsym afl_ops.process");
		goto error_dlsym;
	}
	cxt->ops.afl_ops.ioctrl= dlsym(cxt->ispalg_lib_handle, "afl_ctrl_ioctrl");
	if (!cxt->ops.afl_ops.ioctrl) {
		ISP_LOGE("fail to dlsym afl_ops.afl_ctrl_ioctrl");
		goto error_dlsym;
	}
	cxt->ops.afl_ops.config= dlsym(cxt->ispalg_lib_handle, "afl_ctrl_cfg");
	if (!cxt->ops.afl_ops.config) {
		ISP_LOGE("failed to dlsym afl_ops.cfg");
		goto error_dlsym;
	}
	cxt->ops.afl_ops.config_new= dlsym(cxt->ispalg_lib_handle, "aflnew_ctrl_cfg");
	if (!cxt->ops.afl_ops.config_new) {
		ISP_LOGE("failed to dlsym afl_ops.cfg_new");
		goto error_dlsym;
	}
	/*init ae_ctrl_ops*/
	cxt->ops.ae_ops.init = dlsym(cxt->ispalg_lib_handle, "ae_ctrl_init");
	if (!cxt->ops.ae_ops.init) {
		ISP_LOGE("failed to dlsym ae_ops.init");
		goto error_dlsym;
	}
	cxt->ops.ae_ops.deinit = dlsym(cxt->ispalg_lib_handle, "ae_ctrl_deinit");
	if (!cxt->ops.ae_ops.deinit) {
		ISP_LOGE("failed to dlsym ae_ops.deinit");
		goto error_dlsym;
	}
	cxt->ops.ae_ops.process = dlsym(cxt->ispalg_lib_handle, "ae_ctrl_process");
	if (!cxt->ops.ae_ops.process) {
		ISP_LOGE("failed to dlsym ae_ops.process");
		goto error_dlsym;
	}
	cxt->ops.ae_ops.ioctrl= dlsym(cxt->ispalg_lib_handle, "ae_ctrl_ioctrl");
	if (!cxt->ops.ae_ops.ioctrl) {
		ISP_LOGE("failed to dlsym ae_ops.ioctrl");
		goto error_dlsym;
	}

	/*init awb_ctrl_ops*/
	cxt->ops.awb_ops.init = dlsym(cxt->ispalg_lib_handle, "awb_ctrl_init");
	if (!cxt->ops.awb_ops.init) {
		ISP_LOGE("failed to dlsym awb_ops.init");
		goto error_dlsym;
	}
	cxt->ops.awb_ops.deinit = dlsym(cxt->ispalg_lib_handle, "awb_ctrl_deinit");
	if (!cxt->ops.awb_ops.deinit) {
		ISP_LOGE("failed to dlsym awb_ops.deinit");
		goto error_dlsym;
	}
	cxt->ops.awb_ops.process = dlsym(cxt->ispalg_lib_handle, "awb_ctrl_process");
	if (!cxt->ops.awb_ops.process) {
		ISP_LOGE("failed to dlsym awb_ops.process");
		goto error_dlsym;
	}
	cxt->ops.awb_ops.ioctrl= dlsym(cxt->ispalg_lib_handle, "awb_ctrl_ioctrl");
	if (!cxt->ops.awb_ops.ioctrl) {
		ISP_LOGE("failed to dlsym awb_ops.ioctrl");
		goto error_dlsym;
	}
	/*init pdaf_ctrl_ops*/
	cxt->ops.pdaf_ops.init = dlsym(cxt->ispalg_lib_handle, "pdaf_ctrl_init");
	if (!cxt->ops.pdaf_ops.init) {
		ISP_LOGE("failed to dlsym pdaf_ops.init");
		goto error_dlsym;
	}
	cxt->ops.pdaf_ops.deinit = dlsym(cxt->ispalg_lib_handle, "pdaf_ctrl_deinit");
	if (!cxt->ops.pdaf_ops.deinit) {
		ISP_LOGE("failed to dlsym pdaf_ops.deinit");
		goto error_dlsym;
	}
	cxt->ops.pdaf_ops.process = dlsym(cxt->ispalg_lib_handle, "pdaf_ctrl_process");
	if (!cxt->ops.pdaf_ops.process) {
		ISP_LOGE("failed to dlsym pdaf_ops.process");
		goto error_dlsym;
	}
	cxt->ops.pdaf_ops.ioctrl= dlsym(cxt->ispalg_lib_handle, "pdaf_ctrl_ioctrl");
	if (!cxt->ops.pdaf_ops.ioctrl) {
		ISP_LOGE("failed to dlsym pdaf_ops.ioctrl");
		goto error_dlsym;
	}
	/*init smart_ctrl_ops*/
	cxt->ops.smart_ops.init = dlsym(cxt->ispalg_lib_handle, "smart_ctl_init");
	if (!cxt->ops.smart_ops.init) {
		ISP_LOGE("failed to dlsym smart_ops.init");
		goto error_dlsym;
	}
	cxt->ops.smart_ops.deinit = dlsym(cxt->ispalg_lib_handle, "smart_ctl_deinit");
	if (!cxt->ops.smart_ops.deinit) {
		ISP_LOGE("failed to dlsym smart_ops.deinit");
		goto error_dlsym;
	}
	cxt->ops.smart_ops.ioctrl= dlsym(cxt->ispalg_lib_handle, "smart_ctl_ioctl");
	if (!cxt->ops.smart_ops.ioctrl) {
		ISP_LOGE("failed to dlsym smart_ops.ioctrl");
		goto error_dlsym;
	}
	cxt->ops.smart_ops.calc= dlsym(cxt->ispalg_lib_handle, "_smart_calc");
	if (!cxt->ops.smart_ops.calc) {
		ISP_LOGE("failed to dlsym smart_ops.calc");
		goto error_dlsym;
	}
	cxt->ops.smart_ops.block_disable= dlsym(cxt->ispalg_lib_handle, "smart_ctl_block_disable");
	if (!cxt->ops.smart_ops.block_disable) {
		ISP_LOGE("failed to dlsym smart_ops.block_disable");
		goto error_dlsym;
	}
	cxt->ops.smart_ops.block_enable= dlsym(cxt->ispalg_lib_handle, "smart_ctl_block_enable_recover");
	if (!cxt->ops.smart_ops.block_enable) {
		ISP_LOGE("failed to dlsym smart_ops.block_enable");
		goto error_dlsym;
	}
	cxt->ops.smart_ops.NR_disable= dlsym(cxt->ispalg_lib_handle, "smart_ctl_NR_block_disable");
	if (!cxt->ops.smart_ops.NR_disable) {
		ISP_LOGE("failed to dlsym smart_ops.NR_disable");
		goto error_dlsym;
	}
	/*init lsc_ctrl_ops*/
	cxt->ops.lsc_ops.init = dlsym(cxt->ispalg_lib_handle, "lsc_ctrl_init");
	if (!cxt->ops.lsc_ops.init) {
		ISP_LOGE("failed to dlsym lsc_ops.init");
		goto error_dlsym;
	}
	cxt->ops.lsc_ops.deinit = dlsym(cxt->ispalg_lib_handle, "lsc_ctrl_deinit");
	if (!cxt->ops.lsc_ops.deinit) {
		ISP_LOGE("failed to dlsym lsc_ops.deinit");
		goto error_dlsym;
	}
	cxt->ops.lsc_ops.process = dlsym(cxt->ispalg_lib_handle, "lsc_ctrl_process");
	if (!cxt->ops.lsc_ops.process) {
		ISP_LOGE("failed to dlsym lsc_ops.process");
		goto error_dlsym;
	}
	cxt->ops.lsc_ops.ioctrl= dlsym(cxt->ispalg_lib_handle, "lsc_ctrl_ioctrl");
	if (!cxt->ops.lsc_ops.ioctrl) {
		ISP_LOGE("failed to dlsym lsc_ops.ioctrl");
		goto error_dlsym;
	}

	return 0;
error_dlsym:
	dlclose(cxt->ispalg_lib_handle);
	cxt->ispalg_lib_handle = NULL;
error_dlopen:

	return ret;
}

static cmr_int ispalg_libops_init(cmr_handle adpt_handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct  isp_alg_fw_context *)adpt_handle;

	ISP_CHECK_HANDLE_VALID(adpt_handle);
	ret = load_ispalg_library(cxt);
	if (ret) {
		ISP_LOGE("failed to load library");
	}

	return ret;
}

cmr_int isp_alg_fw_init(struct isp_alg_fw_init_in * input_ptr, cmr_handle * isp_alg_handle)
{
	cmr_int ret = ISP_SUCCESS;

	if (!input_ptr || !isp_alg_handle) {
		ISP_LOGE("fail to check input param, 0x%lx", (cmr_uint) input_ptr);
		ret = ISP_PARAM_NULL;
		goto exit;
	}

	struct isp_alg_fw_context *cxt = NULL;
	struct isp_alg_sw_init_in isp_alg_input;
	struct sensor_raw_info *sensor_raw_info_ptr = (struct sensor_raw_info *)input_ptr->init_param->setting_param_ptr;
	struct sensor_libuse_info *libuse_info = NULL;
	cmr_u32 *binning_info = NULL;
	cmr_u32 max_binning_num = ISP_BINNING_MAX_STAT_W * ISP_BINNING_MAX_STAT_H / 4;

	*isp_alg_handle = NULL;

	cxt = (struct isp_alg_fw_context *)malloc(sizeof(struct isp_alg_fw_context));
	if (!cxt) {
		ISP_LOGE("fail to malloc");
		ret = ISP_ALLOC_ERROR;
		goto exit;
	}
	memset(cxt, 0, sizeof(*cxt));

	cxt->binning_statis_ptr = (unsigned short *)memalign(16, MAX_BINNING_STATS_SIZE * sizeof(cmr_u16));
	if (!cxt->binning_statis_ptr) {
		ISP_LOGE("binning statis buffer malloc failed");
		ret = ISP_ALLOC_ERROR;
		goto exit;
	}

	ret = isp_pm_sw_init(cxt, input_ptr->init_param);

	cxt->dev_access_handle = input_ptr->dev_access_handle;
	isp_alg_input.lib_use_info = sensor_raw_info_ptr->libuse_info;
	isp_alg_input.size.w = input_ptr->init_param->size.w;
	isp_alg_input.size.h = input_ptr->init_param->size.h;
	cxt->lib_use_info = sensor_raw_info_ptr->libuse_info;

	cxt->otp_data = input_ptr->init_param->otp_data;
	isp_alg_input.otp_data = input_ptr->init_param->otp_data;
	cxt->is_master = input_ptr->init_param->is_master;
	cxt->is_multi_mode = input_ptr->init_param->is_multi_mode;
	isp_alg_input.pdaf_info = input_ptr->init_param->pdaf_info;
	isp_alg_input.sensor_max_size = input_ptr->init_param->sensor_max_size;

	binning_info = (cmr_u32 *) malloc(max_binning_num * 3 * sizeof(cmr_u32));
	if (!binning_info) {
		ISP_LOGE("fail to malloc binning buf");
		ret = ISP_ALLOC_ERROR;
		goto exit;
	}
	memset(binning_info, 0, max_binning_num * 3 * sizeof(cmr_u32));
	cxt->binning_stats.r_info = binning_info;
	cxt->binning_stats.g_info = binning_info + max_binning_num;
	cxt->binning_stats.b_info = cxt->binning_stats.g_info + max_binning_num;

	cmr_u32 binning_hx = 4;
	cmr_u32 binning_vx = 4;
	cmr_u32 src_w = cxt->commn_cxt.src.w;
	cmr_u32 src_h = cxt->commn_cxt.src.h;
	cmr_u32 binning_w = (src_w >> binning_hx) & ~0x1;
	cmr_u32 binning_h = (src_h >> binning_vx) & ~0x1;
	cxt->binning_stats.binning_size.w = binning_w / 2;
	cxt->binning_stats.binning_size.h = binning_h / 2;
	cxt->pdaf_cxt.pdaf_support = input_ptr->init_param->ex_info.pdaf_supported;
    /*0:afl_old mode, 1:afl_new mode*/
	cxt->afl_cxt.version = 1;

	if (cxt->is_multi_mode == ISP_DUAL_NORMAL)
	isp_br_init(cxt->is_master, cxt);

	ret = ispalg_libops_init(cxt);

	if (ret) {
		ISP_LOGE("failed to init library and ops");
	}
	ret = isp_alg_sw_init(cxt, &isp_alg_input);

	if (ret) {
		goto exit;
	}

	ret = isp_alg_create_thread((cmr_handle) cxt);

exit:
	if (ret) {
		if (cxt) {
			isp_alg_destroy_thread_proc((cmr_handle) cxt);
			isp_alg_sw_deinit((cmr_handle) cxt);
			if (binning_info) {
				free((void *)binning_info);
			}
			if (cxt->binning_statis_ptr) {
				free(cxt->binning_statis_ptr);
			}
			free((void *)cxt);
		}
	} else {
		*isp_alg_handle = (cmr_handle) cxt;
		isp_dev_access_evt_reg(cxt->dev_access_handle, ispalg_dev_evt_cb, (cmr_handle) cxt);
	}

	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int isp_alg_fw_deinit(cmr_handle isp_alg_handle)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	if (!cxt) {
		ISP_LOGE("fail to get cxt pointer");
		goto exit;
	}
	isp_alg_destroy_thread_proc((cmr_handle) cxt);

	ret = isp_alg_sw_deinit((cmr_handle) cxt);
	ISP_TRACE_IF_FAIL(ret, ("fail to do _ispAlgDeInit"));

	ret = isp_pm_deinit(cxt->handle_pm, NULL, NULL);
	ISP_TRACE_IF_FAIL(ret, ("fail to do isp_pm_deinit"));

	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_RESET, NULL, NULL);
	ISP_TRACE_IF_FAIL(ret, ("fail to do isp uncfg"));

	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_STOP, NULL, NULL);
	ISP_TRACE_IF_FAIL(ret, ("fail to do isp_dev_stop"));


	if (cxt->ae_cxt.log_alc) {
		free(cxt->ae_cxt.log_alc);
		cxt->ae_cxt.log_alc = NULL;
	}

	if (cxt->commn_cxt.log_isp) {
		free(cxt->commn_cxt.log_isp);
		cxt->commn_cxt.log_isp = NULL;
	}

	if (cxt->is_multi_mode == ISP_DUAL_NORMAL)
		isp_br_deinit(cxt->is_master);

	if (cxt->binning_stats.r_info) {
		free((void *)cxt->binning_stats.r_info);
	}

	if (cxt->binning_statis_ptr) {
		ISP_LOGV("free binning_statis_ptr");
		free(cxt->binning_statis_ptr);
		cxt->binning_statis_ptr = NULL;
	}

	if (cxt) {
		free((void *)cxt);
		cxt = NULL;
	}

exit:
	ISP_LOGI("done %d", ret);
	return ret;
}

static cmr_s32 isp_alg_cfg(cmr_handle isp_alg_handle)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_pm_ioctl_input input;
	struct isp_pm_ioctl_output output;
	struct isp_pm_param_data *param_data;
	cmr_u32 i = 0;

	cxt->gamma_sof_cnt = 0;
	cxt->gamma_sof_cnt_eb = 0;
	cxt->update_gamma_eb = 0;

	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_RESET, NULL, NULL);
	ISP_TRACE_IF_FAIL(ret, ("fail to do isp_dev_reset"));

	isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_ISP_ALL_SETTING, &input, &output);
	param_data = output.param_data;
	for (i = 0; i < output.param_num; i++) {
		if (ISP_BLK_2D_LSC == param_data->id) {
			isp_dev_lsc_update(cxt->dev_access_handle, 1);
		}
		isp_dev_cfg_block(cxt->dev_access_handle, param_data->data_ptr, param_data->id);
		if (ISP_BLK_2D_LSC == param_data->id) {
			/*cxt->capture_mode : 0 normal mode; 1 zsl mode*/
			if (0 == cxt->capture_mode) {
				isp_dev_lsc_update(cxt->dev_access_handle, 1);
			} else if (1 == cxt->capture_mode) {
				isp_dev_lsc_update(cxt->dev_access_handle, 0);
			}
		}
		param_data++;
	}
	if (cxt->afl_cxt.handle) {
		((struct isp_anti_flicker_cfg *)cxt->afl_cxt.handle)->width = cxt->commn_cxt.src.w;
		((struct isp_anti_flicker_cfg *)cxt->afl_cxt.handle)->height = cxt->commn_cxt.src.h;
	}
	if(cxt->afl_cxt.version) {
		if (cxt->ops.afl_ops.config_new)
			ret = cxt->ops.afl_ops.config_new(cxt->afl_cxt.handle);
	} else {
		if (cxt->ops.afl_ops.config)
			ret = cxt->ops.afl_ops.config(cxt->afl_cxt.handle);
	}

	if (ISP_SUCCESS != ret) {
		ISP_LOGE("fail to do anti_flicker param update");
		return ret;
	}

	return ret;
}

static cmr_int ae_set_work_mode(cmr_handle isp_alg_handle, cmr_u32 new_mode, cmr_u32 fly_mode, struct isp_video_start *param_ptr)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct ae_set_work_param ae_param;
	enum ae_work_mode ae_mode = 0;

	memset(&ae_param, 0, sizeof(ae_param));
	switch (new_mode) {
	case ISP_MODE_ID_PRV_0:
	case ISP_MODE_ID_PRV_1:
	case ISP_MODE_ID_PRV_2:
	case ISP_MODE_ID_PRV_3:
		ae_mode = AE_WORK_MODE_COMMON;
		break;

	case ISP_MODE_ID_CAP_0:
	case ISP_MODE_ID_CAP_1:
	case ISP_MODE_ID_CAP_2:
	case ISP_MODE_ID_CAP_3:
		ae_mode = AE_WORK_MODE_CAPTURE;
		break;

	case ISP_MODE_ID_VIDEO_0:
	case ISP_MODE_ID_VIDEO_1:
	case ISP_MODE_ID_VIDEO_2:
	case ISP_MODE_ID_VIDEO_3:
		ae_mode = AE_WORK_MODE_VIDEO;
		break;

	default:
		break;
	}

	ae_param.mode = ae_mode;
	ae_param.fly_eb = fly_mode;
	ae_param.highflash_measure.highflash_flag = param_ptr->is_need_flash;
	ae_param.highflash_measure.capture_skip_num = param_ptr->capture_skip_num;
	ae_param.resolution_info.frame_size.w = cxt->commn_cxt.src.w;
	ae_param.resolution_info.frame_size.h = cxt->commn_cxt.src.h;
	ae_param.resolution_info.frame_line = cxt->commn_cxt.input_size_trim[cxt->commn_cxt.param_index].frame_line;
	ae_param.resolution_info.line_time = cxt->commn_cxt.input_size_trim[cxt->commn_cxt.param_index].line_time;
	ae_param.resolution_info.sensor_size_index = cxt->commn_cxt.param_index;
	ae_param.is_snapshot = param_ptr->is_snapshot;
	ae_param.dv_mode = param_ptr->dv_mode;

	ae_param.sensor_fps.mode = param_ptr->sensor_fps.mode;
	ae_param.sensor_fps.max_fps = param_ptr->sensor_fps.max_fps;
	ae_param.sensor_fps.min_fps = param_ptr->sensor_fps.min_fps;
	ae_param.sensor_fps.is_high_fps = param_ptr->sensor_fps.is_high_fps;
	ae_param.sensor_fps.high_fps_skip_num = param_ptr->sensor_fps.high_fps_skip_num;
	ae_param.win_num.h = cxt->ae_cxt.win_num.h;
	ae_param.win_num.w = cxt->ae_cxt.win_num.w;

	ae_param.win_size.w = ((ae_param.resolution_info.frame_size.w / ae_param.win_num.w) / 2) * 2;
	ae_param.win_size.h = ((ae_param.resolution_info.frame_size.h / ae_param.win_num.h) / 2) * 2;
	if (ae_param.win_size.w < 120) {
		ae_param.shift = 0;
	} else {
		ae_param.shift = 1;
	}
	cxt->ae_cxt.shift = ae_param.shift;
	if (cxt->ops.awb_ops.ioctrl) {
		ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_GET_CT_TABLE20, NULL, (void *)&ae_param.ct_table);
	}
	if (cxt->ops.ae_ops.ioctrl) {
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_VIDEO_START, &ae_param, NULL);
	}
	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AE_SHIFT, &ae_param.shift, NULL);

	return ret;
}

static cmr_int isp_update_alg_param(cmr_handle isp_alg_handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct smart_proc_input smart_proc_in;
	struct awb_gain result;
	struct isp_pm_ioctl_input ioctl_output = { PNULL, 0 };
	struct isp_pm_ioctl_input ioctl_input = { PNULL, 0 };
	struct isp_pm_param_data ioctl_data;
	struct isp_awbc_cfg awbc_cfg;
	struct alsc_ver_info lsc_ver = { 0 };
	cmr_u32 ct = 0;
	cmr_s32 bv = 0;
	cmr_s32 bv_gain = 0;

	/*update aem information */
	cxt->aem_is_update = 0;
//      memset((void*)&cxt->aem_stats, 0, sizeof(cxt->aem_stats));

	/*update awb gain */
	if (cxt->ops.awb_ops.ioctrl)
		ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_GET_GAIN, (void *)&result, NULL);
	awbc_cfg.r_gain = result.r;
	awbc_cfg.g_gain = result.g;
	awbc_cfg.b_gain = result.b;
	awbc_cfg.r_offset = 0;
	awbc_cfg.g_offset = 0;
	awbc_cfg.b_offset = 0;
	ioctl_data.id = ISP_BLK_AWB_NEW;
	ioctl_data.cmd = ISP_PM_BLK_AWBC;
	ioctl_data.data_ptr = &awbc_cfg;
	ioctl_data.data_size = sizeof(awbc_cfg);
	ioctl_input.param_data_ptr = &ioctl_data;
	ioctl_input.param_num = 1;
	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_AWB, (void *)&ioctl_input, NULL);

	/*update smart param */
	if (cxt->ops.awb_ops.ioctrl)
		ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_GET_CT, (void *)&ct, NULL);
	if (cxt->ops.ae_ops.ioctrl) {
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_BV_BY_LUM_NEW, NULL, (void *)&bv);
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_BV_BY_GAIN, NULL, (void *)&bv_gain);
	}
	if (cxt->ops.smart_ops.ioctrl)
		ret = cxt->ops.smart_ops.ioctrl(cxt->smart_cxt.handle, ISP_SMART_IOCTL_SET_WORK_MODE, (void *)&cxt->commn_cxt.isp_mode, NULL);

	if (cxt->ops.lsc_ops.ioctrl){
		ret = cxt->ops.lsc_ops.ioctrl(cxt->lsc_cxt.handle, ALSC_GET_VER, NULL, (void *)&lsc_ver);
		ISP_TRACE_IF_FAIL(ret, ("fail to Get ALSC ver info!"));
	}
	cxt->lsc_cxt.lsc_sprd_version = lsc_ver.LSC_SPD_VERSION;

	memset(&ioctl_data, 0, sizeof(ioctl_data));
	BLOCK_PARAM_CFG(ioctl_input, ioctl_data, ISP_PM_BLK_GAMMA_TAB, ISP_BLK_RGB_GAMC, PNULL, 0);
	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_SINGLE_SETTING, (void *)&ioctl_input, (void *)&ioctl_output);
	ISP_TRACE_IF_FAIL(ret, ("fail to get GAMMA TAB"));

	if (ioctl_output.param_num > 0 && ioctl_output.param_data_ptr && ioctl_output.param_data_ptr->data_ptr) {
		cxt->smart_cxt.tunning_gamma_cur = ioctl_output.param_data_ptr->data_ptr;
	}

	memset(&smart_proc_in, 0, sizeof(smart_proc_in));
	if ((0 != bv_gain) && (0 != ct)) {


		smart_proc_in.cal_para.gamma_tab = cxt->smart_cxt.tunning_gamma_cur;
		smart_proc_in.cal_para.bv = bv;
		smart_proc_in.cal_para.bv_gain = bv_gain;
		smart_proc_in.cal_para.ct = ct;
		smart_proc_in.alc_awb = cxt->awb_cxt.alc_awb;
		smart_proc_in.mode_flag = cxt->commn_cxt.mode_flag;
		smart_proc_in.scene_flag = cxt->commn_cxt.scene_flag;
		smart_proc_in.lsc_sprd_version = cxt->lsc_cxt.lsc_sprd_version;
		isp_prepare_atm_param(isp_alg_handle, &smart_proc_in);
		if (cxt->ops.smart_ops.calc)
			ret = cxt->ops.smart_ops.calc(cxt->smart_cxt.handle, &smart_proc_in);
	}

#if 0
//#ifdef LSC_ADV_ENABLE
	cmr_handle lsc_adv_handle = cxt->lsc_cxt.handle;
	struct isp_pm_ioctl_input input = { PNULL, 0 };
	struct isp_pm_ioctl_output output = { PNULL, 0 };
	struct isp_pm_param_data param_data;
	struct lsc_adv_calc_param calc_param;
	struct lsc_adv_calc_result calc_result = { 0 };
	cmr_u32 i = 0;
	memset(&param_data, 0, sizeof(param_data));
	memset(&calc_param, 0, sizeof(calc_param));

	BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_LSC_INFO, ISP_BLK_2D_LSC, PNULL, 0);
	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_SINGLE_SETTING, (void *)&input, (void *)&output);
	struct isp_lsc_info *lsc_info = (struct isp_lsc_info *)output.param_data->data_ptr;

	struct isp_2d_lsc_param *lsc_tab_pram_ptr = (struct isp_2d_lsc_param *)(cxt->lsc_cxt.lsc_tab_address);

	calc_result.dst_gain = (cmr_u16 *) lsc_info->data_ptr;
	struct awb_size stat_img_size;
	struct awb_size win_size;
	struct isp_ae_grgb_statistic_info *stat_info;
	BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_AEM_STATISTIC, ISP_BLK_AE_NEW, NULL, 0);
	isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_SINGLE_SETTING, (void *)&input, (void *)&output);
	stat_info = output.param_data->data_ptr;

	if (cxt->ops.awb_ops.ioctrl) {
		cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_GET_STAT_SIZE, (void *)&stat_img_size, NULL);
		cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_GET_WIN_SIZE, (void *)&win_size, NULL);
	}

	//ISP_LOGV("0x%x, 0x%x, 0x%x", stat_info->r_info, stat_info->g_info, stat_info->b_info);
//      calc_param.stat_img.r  = stat_info->r_info;
//      calc_param.stat_img.gr = stat_info->g_info;
//      calc_param.stat_img.gb = stat_info->g_info;
//      calc_param.stat_img.b  = stat_info->b_info;
	calc_param.stat_img.r = cxt->aem_stats.r_info;
	calc_param.stat_img.gr = cxt->aem_stats.g_info;
	calc_param.stat_img.gb = cxt->aem_stats.g_info;
	calc_param.stat_img.b = cxt->aem_stats.b_info;
	calc_param.stat_size.w = stat_img_size.w;
	calc_param.stat_size.h = stat_img_size.h;
	calc_param.gain_width = lsc_info->gain_w;
	calc_param.gain_height = lsc_info->gain_h;
	calc_param.lum_gain = (cmr_u16 *) lsc_info->param_ptr;
	calc_param.block_size.w = win_size.w;
	calc_param.block_size.h = win_size.h;
	calc_param.ct = ct;
	calc_param.bv = isp_cur_bv;
	calc_param.isp_id = ISP_2_0;

	calc_param.r_gain = gAWBGainR;
	calc_param.b_gain = gAWBGainB;
	calc_param.img_size.w = cxt->commn_cxt.src.w;
	calc_param.img_size.h = cxt->commn_cxt.src.h;

	if (lsc_tab_pram_ptr == 0)
		return 0;
	else {
		for (i = 0; i < 9; i++) {
			calc_param.lsc_tab_address[i] = lsc_tab_pram_ptr->map_tab[i].param_addr;
		}
	}

	if (cxt->ops.lsc_ops.process)
		cxt->ops.lsc_ops.process(lsc_adv_handle, &calc_param, &calc_result);

	BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_LSC_INFO, ISP_BLK_2D_LSC, PNULL, 0);
	input.param_data_ptr = &param_data;

	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_OTHERS, &input, NULL);

#endif

	return ret;
}

static cmr_int ispalg_update_alsc_result(cmr_handle isp_alg_handle, cmr_handle out_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_pm_ioctl_input input = { PNULL, 0 };
	struct isp_pm_ioctl_output output = { PNULL, 0 };
	struct isp_pm_param_data param_data_alsc;
	struct isp_pm_ioctl_input io_pm_input = { PNULL, 0 };
	struct isp_pm_param_data pm_param;
	struct isp_lsc_info *lsc_info_new = NULL;
	struct isp_2d_lsc_param *lsc_tab_pram_ptr = NULL;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct alsc_fwstart_info *fwstart_info = (struct alsc_fwstart_info *)out_ptr;
	cmr_s32 i = 0;
	cmr_s32 dst_gain_size = 0;
	cmr_u16* dst_gain_tmp = NULL;

	memset(&param_data_alsc, 0, sizeof(param_data_alsc));
	BLOCK_PARAM_CFG(input, param_data_alsc, ISP_PM_BLK_LSC_INFO, ISP_BLK_2D_LSC, PNULL, 0);
	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_SINGLE_SETTING, (void *)&input, (void *)&output);
	if (ISP_SUCCESS != ret || NULL == output.param_data) {
		ISP_LOGE("fail to check output.param_data");
		return ISP_PARAM_ERROR;
	}

	lsc_info_new = (struct isp_lsc_info *)output.param_data->data_ptr;
	lsc_tab_pram_ptr = (struct isp_2d_lsc_param *)(cxt->lsc_cxt.lsc_tab_address);
	for (i = 0; i < 9; i++)
		fwstart_info->lsc_tab_address_new[i] = lsc_tab_pram_ptr->map_tab[i].param_addr;
	fwstart_info->gain_width_new = lsc_info_new->gain_w;
	fwstart_info->gain_height_new = lsc_info_new->gain_h;
	fwstart_info->image_pattern_new = cxt->commn_cxt.image_pattern;
	fwstart_info->grid_new = lsc_info_new->grid;
	fwstart_info->camera_id = cxt->camera_id;

	dst_gain_size = lsc_info_new->gain_w*lsc_info_new->gain_h*4*sizeof(cmr_u16);
	dst_gain_tmp = (cmr_u16*)malloc(dst_gain_size);
	if (NULL == dst_gain_tmp) {
		ret = ISP_ALLOC_ERROR;
		ISP_LOGE("fail to alloc dst_gain_tmp");
		return ret;
	}
	fwstart_info->lsc_result_address_new = dst_gain_tmp;
	if (cxt->ops.lsc_ops.ioctrl) {
		ret = cxt->ops.lsc_ops.ioctrl(cxt->lsc_cxt.handle, ALSC_FW_START, (void *)fwstart_info, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to ALSC_FW_START"));
	}

	BLOCK_PARAM_CFG(io_pm_input, pm_param, ISP_PM_BLK_LSC_MEM_ADDR, ISP_BLK_2D_LSC, dst_gain_tmp, dst_gain_size);
	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_OTHERS, &io_pm_input, NULL);

	free(dst_gain_tmp);
	dst_gain_tmp = NULL;

	return ret;
}

cmr_int isp_alg_fw_start(cmr_handle isp_alg_handle, struct isp_video_start * in_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_interface_param_v1 *interface_ptr_v1 = &cxt->commn_cxt.interface_param_v1;
	struct isp_statis_mem_info statis_mem_input;
	struct isp_size org_size;
	struct isp_pm_ioctl_input io_pm_input = { NULL, 0 };
	struct isp_pm_param_data pm_param;
	struct alsc_fwstart_info fwstart_info = { NULL, {NULL}, 0, 0, 5, 0, 0};
	struct afctrl_fwstart_info af_start_info;
	cmr_s32 mode = 0, dv_mode = 0;

	if (!isp_alg_handle || !in_ptr) {
		ret = ISP_PARAM_ERROR;
		goto exit;
	}

	cxt->capture_mode = in_ptr->capture_mode;
	cxt->sensor_fps.mode = in_ptr->sensor_fps.mode;
	cxt->sensor_fps.max_fps = in_ptr->sensor_fps.max_fps;
	cxt->sensor_fps.min_fps = in_ptr->sensor_fps.min_fps;
	cxt->sensor_fps.is_high_fps = in_ptr->sensor_fps.is_high_fps;
	cxt->sensor_fps.high_fps_skip_num = in_ptr->sensor_fps.high_fps_skip_num;

	org_size.w = cxt->commn_cxt.src.w;
	org_size.h = cxt->commn_cxt.src.h;
	cxt->commn_cxt.src.w = in_ptr->size.w;
	cxt->commn_cxt.src.h = in_ptr->size.h;

	memset(&statis_mem_input, 0, sizeof(struct isp_statis_mem_info));
	statis_mem_input.buffer_client_data = in_ptr->buffer_client_data;
	statis_mem_input.cb_of_malloc = in_ptr->cb_of_malloc;
	statis_mem_input.cb_of_free = in_ptr->cb_of_free;
	statis_mem_input.isp_lsc_physaddr = in_ptr->lsc_phys_addr;
	statis_mem_input.isp_lsc_virtaddr = in_ptr->lsc_virt_addr;
	statis_mem_input.lsc_mfd = in_ptr->lsc_mfd;

	ret = isp_dev_statis_buf_malloc(cxt->dev_access_handle, &statis_mem_input);
	ISP_RETURN_IF_FAIL(ret, ("fail to malloc buf"));
	interface_ptr_v1->data.work_mode = ISP_CONTINUE_MODE;
	interface_ptr_v1->data.input = ISP_CAP_MODE;
	interface_ptr_v1->data.input_format = in_ptr->format;
	interface_ptr_v1->data.format_pattern = cxt->commn_cxt.image_pattern;
	interface_ptr_v1->data.input_size.w = in_ptr->size.w;
	interface_ptr_v1->data.input_size.h = in_ptr->size.h;
	interface_ptr_v1->data.output_format = ISP_DATA_UYVY;
	interface_ptr_v1->data.output = ISP_DCAM_MODE;
	interface_ptr_v1->data.slice_height = in_ptr->size.h;

	ret = isp_dev_set_interface(interface_ptr_v1);
	ISP_RETURN_IF_FAIL(ret, ("fail to set param"));

	switch (in_ptr->work_mode) {
	case 0:		/*preview */
		ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_MODEID_BY_RESOLUTION, in_ptr, &mode);
		break;
	case 1:		/*capture */
		ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_MODEID_BY_RESOLUTION, in_ptr, &mode);
		break;
	case 2:
		mode = ISP_MODE_ID_VIDEO_0;
		break;
	default:
		mode = ISP_MODE_ID_PRV_0;
		break;
	}
	if (SENSOR_MULTI_MODE_FLAG != cxt->commn_cxt.multi_nr_flag) {
		if ((mode != cxt->commn_cxt.isp_mode) && (org_size.w != cxt->commn_cxt.src.w)) {
			BLOCK_PARAM_CFG(io_pm_input, pm_param, ISP_PM_BLK_CFA_CFG, ISP_BLK_CFA, PNULL, 0);
			pm_param.data_ptr = (void *)&cxt->commn_cxt.src.w;
			io_pm_input.param_data_ptr = &pm_param;
			ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_OTHERS, &io_pm_input, NULL);
			cxt->commn_cxt.isp_mode = mode;
			ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_MODE, &cxt->commn_cxt.isp_mode, NULL);
		}
	} else {

		if (0 != in_ptr->dv_mode) {
			ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_DV_MODEID_BY_RESOLUTION, in_ptr, &dv_mode);
			cxt->commn_cxt.mode_flag = dv_mode;
		} else {
			cxt->commn_cxt.mode_flag = mode;
		}
		if (cxt->commn_cxt.mode_flag != (cmr_u32) cxt->commn_cxt.isp_mode) {
			BLOCK_PARAM_CFG(io_pm_input, pm_param, ISP_PM_BLK_CFA_CFG, ISP_BLK_CFA, PNULL, 0);
			pm_param.data_ptr = (void *)&cxt->commn_cxt.src.w;
			io_pm_input.param_data_ptr = &pm_param;
			ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_OTHERS, &io_pm_input, NULL);
			cxt->commn_cxt.isp_mode = cxt->commn_cxt.mode_flag;
			ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_MODE, &cxt->commn_cxt.isp_mode, NULL);
		}
	}

	/* isp param index */
	cxt->commn_cxt.param_index = ispalg_get_param_index(cxt->commn_cxt.input_size_trim, &in_ptr->size);
	/* todo: base on param_index to get sensor line_time/frame_line */

	ret = isp_update_alg_param(cxt);
	ISP_RETURN_IF_FAIL(ret, ("fail to isp smart param calc"));

	/*TBD pdaf_support will get form sensor,pdaf_en will get from oem*/
	ISP_LOGI("cxt->pdaf_cxt.pdaf_support = %d, in_ptr->pdaf_enable = %d",
		cxt->pdaf_cxt.pdaf_support, in_ptr->pdaf_enable);
	if (SENSOR_PDAF_TYPE3_ENABLE == cxt->pdaf_cxt.pdaf_support && in_ptr->pdaf_enable) {
		if (cxt->ops.pdaf_ops.ioctrl) {
			ret = cxt->ops.pdaf_ops.ioctrl(cxt->pdaf_cxt.handle, PDAF_CTRL_CMD_SET_PARAM, NULL, NULL);
			ISP_RETURN_IF_FAIL(ret, ("fail to cfg pdaf"));
		}
	} else {
		if (cxt->ops.pdaf_ops.ioctrl) {
			ret = cxt->ops.pdaf_ops.ioctrl(cxt->pdaf_cxt.handle, PDAF_CTRL_CMD_DISABLE_PDAF, NULL, NULL);
			ISP_RETURN_IF_FAIL(ret, ("fail to disable pdaf"));
		}
	}

	ISP_RETURN_IF_FAIL(ret, ("fail to cfg pdaf param"));

	ret = isp_dev_trans_addr(cxt->dev_access_handle);
	ISP_RETURN_IF_FAIL(ret, ("fail to trans isp buff"));

	/* update lsc reslut */
	ret = ispalg_update_alsc_result(cxt, (void *)&fwstart_info);
	ISP_RETURN_IF_FAIL(ret, ("fail to update alsc result"));

	ret = isp_alg_cfg(cxt);
	ISP_RETURN_IF_FAIL(ret, ("fail to do isp cfg"));
	if (cxt->ops.awb_ops.ioctrl) {
		ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_SET_WORK_MODE, &in_ptr->work_mode, NULL);
		ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_GET_PIX_CNT, &in_ptr->size, NULL);
		ISP_RETURN_IF_FAIL(ret, ("fail to set_awb_work_mode"));
	}

	ret = ae_set_work_mode(cxt, mode, 1, in_ptr);
	ISP_RETURN_IF_FAIL(ret, ("fail to do ae cfg"));

	ret = isp_dev_start(cxt->dev_access_handle, interface_ptr_v1);
	ISP_RETURN_IF_FAIL(ret, ("fail to do video isp start"));
	cxt->gamma_sof_cnt_eb = 1;

	memset(&af_start_info, 0, sizeof(struct afctrl_fwstart_info));
	if (cxt->af_cxt.handle && ((ISP_VIDEO_MODE_CONTINUE == in_ptr->mode))) {
		if (cxt->ops.af_ops.ioctrl) {
			af_start_info.size = in_ptr->size;
			ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_SET_ISP_START_INFO, (void *)&af_start_info, NULL);
			ISP_TRACE_IF_FAIL(ret, ("fail to set af_start_info"));
		}
	}

	if (cxt->ops.lsc_ops.ioctrl){
		ret = cxt->ops.lsc_ops.ioctrl(cxt->lsc_cxt.handle, ALSC_FW_START_END, (void *)&fwstart_info, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to end alsc_fw_start"));
	}
exit:
	ISP_LOGV("done %ld", ret);
	return ret;
}

cmr_int isp_alg_fw_stop(cmr_handle isp_alg_handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	if (cxt->ops.ae_ops.ioctrl) {
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_VIDEO_STOP, NULL, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to AE_VIDEO_STOP"));
	}
	if (cxt->ops.awb_ops.ioctrl) {
		ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_VIDEO_STOP_NOTIFY, NULL, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to AWB_CTRL_CMD_VIDEO_STOP_NOTIFY"));
	}
	if (cxt->ops.af_ops.ioctrl) {
		ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_SET_ISP_STOP_INFO, NULL, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to AF_CMD_SET_ISP_STOP_INFO"));
	}

	if (cxt->ops.lsc_ops.ioctrl) {
		ret = cxt->ops.lsc_ops.ioctrl(cxt->lsc_cxt.handle, ALSC_FW_STOP, NULL, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to ALSC_FW_STOP"));
	}

	ISP_RETURN_IF_FAIL(ret, ("fail to stop isp alg fw"));

exit:
	ISP_LOGV("done %ld", ret);
	return ret;
}

cmr_int isp_slice_raw_proc(struct isp_alg_fw_context * cxt, struct ips_in_param * in_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_raw_proc_info slice_raw_info;;
	struct isp_dev_access_context *isp_dev = NULL;
	struct isp_file *file = NULL;

	if (!cxt || !in_ptr) {
		ret = ISP_PARAM_ERROR;
		goto exit;
	}

	isp_dev = (struct isp_dev_access_context *)cxt->dev_access_handle;
	file = (struct isp_file *)(isp_dev->isp_driver_handle);
	memset((void *)&slice_raw_info, 0x0, sizeof(struct isp_raw_proc_info));

	slice_raw_info.in_size.width = in_ptr->src_frame.img_size.w;
	slice_raw_info.in_size.height = in_ptr->src_frame.img_size.h;
	slice_raw_info.out_size.width = in_ptr->dst_frame.img_size.w;
	slice_raw_info.out_size.height = in_ptr->dst_frame.img_size.h;
	slice_raw_info.img_fd = in_ptr->dst_frame.img_fd.y;
	slice_raw_info.img_vir.chn0 = in_ptr->dst_frame.img_addr_vir.chn0;
	slice_raw_info.img_vir.chn1 = in_ptr->dst_frame.img_addr_vir.chn1;
	slice_raw_info.img_vir.chn2 = in_ptr->dst_frame.img_addr_vir.chn2;
	slice_raw_info.img_offset.chn0 = in_ptr->dst_frame.img_addr_phy.chn0;
	slice_raw_info.img_offset.chn1 = in_ptr->dst_frame.img_addr_phy.chn1;
	slice_raw_info.img_offset.chn2 = in_ptr->dst_frame.img_addr_phy.chn2;

	ret = ioctl(file->fd, SPRD_ISP_IO_RAW_CAP, &slice_raw_info);
	ISP_RETURN_IF_FAIL(ret, ("fail to do isp raw cap ioctl"));

	isp_u_fetch_start_isp(isp_dev->isp_driver_handle, ISP_ONE);

exit:
	return ret;
}

cmr_int isp_alg_proc_start(cmr_handle isp_alg_handle, struct ips_in_param * in_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_interface_param_v1 *interface_ptr_v1 = &cxt->commn_cxt.interface_param_v1;
	struct isp_size org_size;
	struct isp_pm_ioctl_input io_pm_input = { NULL, 0 };
	struct isp_pm_param_data pm_param;
	struct isp_video_start param;
	cmr_s32 mode = 0;

	ISP_LOGV("isp proc start\n");
	// SBS lsc added 1
	struct isp_pm_ioctl_input input = { PNULL, 0 };
	struct isp_pm_ioctl_output output = { PNULL, 0 };
	struct isp_pm_param_data param_data_alsc;
	struct isp_lsc_info *lsc_info_new = NULL;
	struct alsc_fwprocstart_info fwprocstart_info = { NULL, {NULL}, 0, 0, 5, 0, 0};
	struct isp_2d_lsc_param *lsc_tab_pram_ptr = NULL;

	org_size.w = cxt->commn_cxt.src.w;
	org_size.h = cxt->commn_cxt.src.h;

	cxt->commn_cxt.src.w = in_ptr->src_frame.img_size.w;
	cxt->commn_cxt.src.h = in_ptr->src_frame.img_size.h;

	interface_ptr_v1->data.work_mode = ISP_SINGLE_MODE;
	if (cxt->takepicture_mode == CAMERA_ISP_SIMULATION_MODE) {
		interface_ptr_v1->data.input = ISP_SIMULATION_MODE;
	} else {
		interface_ptr_v1->data.input = ISP_EMC_MODE;
	}
	interface_ptr_v1->data.input_format = in_ptr->src_frame.img_fmt;

	if (INVALID_FORMAT_PATTERN == in_ptr->src_frame.format_pattern) {
		interface_ptr_v1->data.format_pattern = cxt->commn_cxt.image_pattern;
	} else {
		interface_ptr_v1->data.format_pattern = cxt->commn_cxt.image_pattern;;
	}
	interface_ptr_v1->data.input_size.w = in_ptr->src_frame.img_size.w;
	interface_ptr_v1->data.input_size.h = in_ptr->src_frame.img_size.h;
	interface_ptr_v1->data.input_addr.chn0 = in_ptr->src_frame.img_addr_phy.chn0;
	interface_ptr_v1->data.input_addr.chn1 = in_ptr->src_frame.img_addr_phy.chn1;
	interface_ptr_v1->data.input_addr.chn2 = in_ptr->src_frame.img_addr_phy.chn2;
	interface_ptr_v1->data.input_vir.chn0 = in_ptr->src_frame.img_addr_vir.chn0;
	interface_ptr_v1->data.input_vir.chn1 = in_ptr->src_frame.img_addr_vir.chn1;
	interface_ptr_v1->data.input_vir.chn2 = in_ptr->src_frame.img_addr_vir.chn2;
	interface_ptr_v1->data.input_fd = in_ptr->src_frame.img_fd.y;
	interface_ptr_v1->data.slice_height = in_ptr->src_frame.img_size.h;

	interface_ptr_v1->data.output_format = in_ptr->dst_frame.img_fmt;
	if (cxt->takepicture_mode == CAMERA_ISP_SIMULATION_MODE) {
		interface_ptr_v1->data.output = ISP_SIMULATION_MODE;
	} else {
		interface_ptr_v1->data.output = ISP_EMC_MODE;
	}
	interface_ptr_v1->data.output_addr.chn0 = in_ptr->dst_frame.img_addr_phy.chn0;
	interface_ptr_v1->data.output_addr.chn1 = in_ptr->dst_frame.img_addr_phy.chn1;
	interface_ptr_v1->data.output_addr.chn2 = in_ptr->dst_frame.img_addr_phy.chn2;

	ret = isp_dev_set_interface(interface_ptr_v1);
	ISP_RETURN_IF_FAIL(ret, ("fail to set param"));

	param.work_mode = 1;
	param.size.w = cxt->commn_cxt.src.w;
	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_MODEID_BY_RESOLUTION, &param, &mode);
	ISP_RETURN_IF_FAIL(ret, ("fail to get isp_mode"));

	if (org_size.w != cxt->commn_cxt.src.w) {
		BLOCK_PARAM_CFG(io_pm_input, pm_param, ISP_PM_BLK_CFA_CFG, ISP_BLK_CFA, PNULL, 0);
		pm_param.data_ptr = (void *)&cxt->commn_cxt.src.w;
		io_pm_input.param_data_ptr = &pm_param;
		ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_OTHERS, &io_pm_input, NULL);
		cxt->commn_cxt.isp_mode = mode;
		cxt->commn_cxt.mode_flag = mode;
		ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_MODE, &cxt->commn_cxt.isp_mode, NULL);
		ISP_RETURN_IF_FAIL(ret, ("isp_pm_ioctl fail "));
	}

	/* isp param index */
	cxt->commn_cxt.param_index = ispalg_get_param_index(cxt->commn_cxt.input_size_trim, &in_ptr->src_frame.img_size);
	/* todo: base on param_index to get sensor line_time/frame_line */

	ret = isp_dev_trans_addr(cxt->dev_access_handle);
	ISP_RETURN_IF_FAIL(ret, ("fail to trans isp buff"));

	// SBS lsc added 2, update lsc reslut
	memset(&param_data_alsc, 0, sizeof(param_data_alsc));
	BLOCK_PARAM_CFG(input, param_data_alsc, ISP_PM_BLK_LSC_INFO, ISP_BLK_2D_LSC, PNULL, 0);
	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_SINGLE_SETTING, (void *)&input, (void *)&output);
	ISP_TRACE_IF_FAIL(ret, ("ISP_PM_CMD_GET_SINGLE_SETTING fail"));
	lsc_info_new = (struct isp_lsc_info *)output.param_data->data_ptr;
	lsc_tab_pram_ptr = (struct isp_2d_lsc_param *)(cxt->lsc_cxt.lsc_tab_address);
	fwprocstart_info.lsc_result_address_new = (cmr_u16 *) lsc_info_new->data_ptr;
	for(int i=0; i<9;i++)
		fwprocstart_info.lsc_tab_address_new[i] = lsc_tab_pram_ptr->map_tab[i].param_addr;//tab
	fwprocstart_info.gain_width_new = lsc_info_new->gain_w;
	fwprocstart_info.gain_height_new = lsc_info_new->gain_h;
	fwprocstart_info.image_pattern_new = cxt->commn_cxt.image_pattern;
	fwprocstart_info.grid_new  = lsc_info_new->grid;
	fwprocstart_info.camera_id = cxt->camera_id;

	if (cxt->ops.lsc_ops.ioctrl)
		ret = cxt->ops.lsc_ops.ioctrl(cxt->lsc_cxt.handle, ALSC_FW_PROC_START, (void *)&fwprocstart_info, NULL);

	ret = isp_alg_cfg(cxt);
	ISP_RETURN_IF_FAIL(ret, ("fail to do isp cfg"));

	ret = isp_dev_start(cxt->dev_access_handle, interface_ptr_v1);
	ISP_RETURN_IF_FAIL(ret, ("fail to video isp start"));
	cxt->gamma_sof_cnt_eb = 1;

	// SBS lsc added 3
	if (cxt->ops.lsc_ops.ioctrl)
		ret = cxt->ops.lsc_ops.ioctrl(cxt->lsc_cxt.handle, ALSC_FW_PROC_START_END, (void *)&fwprocstart_info, NULL);

	ISP_LOGV("isp start raw proc\n");
	ret = isp_slice_raw_proc(cxt, in_ptr);
	ISP_RETURN_IF_FAIL(ret, ("fail to isp_slice_raw_proc"));

exit:
	ISP_LOGV("done %ld", ret);
	return ret;
}

cmr_int isp_alg_proc_next(cmr_handle isp_alg_handle, struct ipn_in_param * in_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	UNUSED(in_ptr);
	/*do not support silce capture function now */
	ISP_LOGV("If need slice capture process, add releated code!");
exit:
	ISP_LOGV("done %ld", ret);
	return ret;
}

cmr_int isp_alg_fw_ioctl(cmr_handle isp_alg_handle, enum isp_ctrl_cmd io_cmd, void *param_ptr, cmr_s32(*call_back) ())
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	enum isp_ctrl_cmd cmd = io_cmd & 0x7fffffff;
	io_fun io_ctrl = NULL;

	cxt->commn_cxt.isp_callback_bypass = io_cmd & 0x80000000;
	io_ctrl = _ispGetIOCtrlFun(cmd);
	if (NULL != io_ctrl) {
		ret = io_ctrl(cxt, param_ptr, call_back);
	} else {
		ISP_LOGV("io_ctrl fun is null, cmd %d", cmd);
	}

	if (NULL != cxt->commn_cxt.callback) {
		cxt->commn_cxt.callback(cxt->commn_cxt.caller_id, ISP_CALLBACK_EVT | ISP_CTRL_CALLBACK | cmd, NULL, ISP_ZERO);
	}

	return ret;
}

cmr_int isp_alg_fw_capability(cmr_handle isp_alg_handle, enum isp_capbility_cmd cmd, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	switch (cmd) {
	case ISP_LOW_LUX_EB:{
			cmr_u32 out_param = 0;
			if (cxt->ops.ae_ops.ioctrl)
				ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_FLASH_EB, NULL, &out_param);
			*((cmr_u32 *) param_ptr) = out_param;
			break;
		}
	case ISP_CUR_ISO:{
			cmr_u32 out_param = 0;
			if (cxt->ops.ae_ops.ioctrl)
				ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_ISO, NULL, &out_param);
			*((cmr_u32 *) param_ptr) = out_param;
			break;
		}
	case ISP_CTRL_GET_AE_LUM:{
			cmr_u32 out_param = 0;
			if (cxt->ops.ae_ops.ioctrl)
				ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_BV_BY_LUM_NEW, NULL, &out_param);
			*((cmr_u32 *) param_ptr) = out_param;
			break;
		}
	default:
		break;
	}

	ISP_LOGV("done %ld", ret);
	return ret;
}
