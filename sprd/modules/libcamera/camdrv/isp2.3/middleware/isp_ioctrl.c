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

#ifdef FEATRUE_ISP_FW_IOCTRL

#define SEPARATE_GAMMA_IN_VIDEO
#define VIDEO_GAMMA_INDEX                    (8)

#define COPY_LOG(l, L) \
{size_t len = copy_log(cxt->commn_cxt.log_isp + off, cxt->l##_cxt.log_##l, \
cxt->l##_cxt.log_##l##_size, L##_START, L##_END, off, total_size); \
if (len) {log.l##_off = off; off += len; log.l##_len = len;} else {log.l##_off = 0;}}

typedef cmr_int(*isp_io_fun) (cmr_handle isp_alg_handle, void *param_ptr);

struct isp_io_ctrl_fun {
	enum isp_ctrl_cmd cmd;
	isp_io_fun io_ctrl;
};

enum isp_ctrl_mode {
	ISP_CTRL_SET = 0x00,
	ISP_CTRL_GET,
	ISP_CTRL_MODE_MAX
};

struct isp_af_ctrl {
	enum isp_ctrl_mode mode;
	cmr_u32 step;
	cmr_u32 num;
	cmr_u32 stat_value[9];
};

static const char *DEBUG_MAGIC = "SPRD_ISP";	// 8 bytes
static const char *AE_START = "ISP_AE__";
static const char *AE_END = "ISP_AE__";
static const char *AF_START = "ISP_AF__";
static const char *AF_END = "ISP_AF__";
static const char *AFT_START = "ISP_AFT_";
static const char *AFT_END = "ISP_AFT_";
static const char *AWB_START = "ISP_AWB_";
static const char *AWB_END = "ISP_AWB_";
static const char *LSC_START = "ISP_LSC_";
static const char *LSC_END = "ISP_LSC_";
static const char *SMART_START = "ISP_SMART_";
static const char *SMART_END = "ISP_SMART_";
static const char *OTP_START = "ISP_OTP_";
static const char *OTP_END = "ISP_OTP_";

static cmr_s32 ispctl_set_awb_gain(cmr_handle isp_alg_handle)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_awbc_cfg awbc_cfg = { 0, 0, 0, 0, 0, 0 };
	struct awb_gain result = { 0, 0, 0 };
	struct isp_pm_ioctl_input ioctl_input = { NULL, 0 };
	struct isp_pm_param_data ioctl_data[ISP_MODE_MAX];
	cmr_s32 i;

	if (cxt->ops.awb_ops.ioctrl)
		ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_GET_GAIN, (void *)&result, NULL);
	ISP_TRACE_IF_FAIL(ret, ("fail to get awb gain"));

	memset(ioctl_data, 0x0, sizeof(ioctl_data));
	/*set awb gain */
	awbc_cfg.r_gain = result.r;
	awbc_cfg.g_gain = result.g;
	awbc_cfg.b_gain = result.b;
	awbc_cfg.r_offset = 0;
	awbc_cfg.g_offset = 0;
	awbc_cfg.b_offset = 0;

	if (cxt->zsl_flag) {
		for (i = 0; i < ISP_MODE_MAX; i++) {
			BLOCK_PARAM_CFG(ioctl_data[0], ISP_PM_BLK_AWBC,
					ISP_BLK_AWB_NEW,
					cxt->mode_id[i],
					&awbc_cfg,
					sizeof(awbc_cfg));
		}
		ioctl_input.param_num = ISP_MODE_MAX;
	} else {
		BLOCK_PARAM_CFG(ioctl_data[0], ISP_PM_BLK_AWBC,
				ISP_BLK_AWB_NEW,
				cxt->mode_id[0],
				&awbc_cfg,
				sizeof(awbc_cfg));
		ioctl_input.param_num = 1;
	}
	ioctl_input.param_data_ptr = ioctl_data;
	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_AWB, (void *)&ioctl_input, NULL);
	ISP_LOGV("set AWB_TAG:  ret=%d, gain=(%d, %d, %d)", ret, awbc_cfg.r_gain, awbc_cfg.g_gain, awbc_cfg.b_gain);

	return ret;
}

static cmr_s32 ispctl_set_awb_flash_gain(cmr_handle isp_alg_handle)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct awb_flash_info flash_awb;
	struct ae_awb_gain flash_wb_gain = { 0, 0, 0 };
	cmr_u32 ae_effect = 0;

	memset((void *)&flash_awb, 0, sizeof(struct awb_flash_info));

	ISP_LOGI("flash param rgb ratio = (%d,%d,%d), lum_ratio = %d", cxt->pm_flash_info->cur.r_ratio,
		cxt->pm_flash_info->cur.g_ratio, cxt->pm_flash_info->cur.b_ratio, cxt->pm_flash_info->cur.lum_ratio);

	if (ISP_SUCCESS != ret) {
		ISP_LOGE("fail to get flash cali parm ");
		return ret;
	}

	if (cxt->ops.ae_ops.ioctrl) {
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_FLASH_WB_GAIN, NULL, &flash_wb_gain);
		ISP_TRACE_IF_FAIL(ret, ("fail to get awb gain"));
	}

	if (cxt->ops.ae_ops.ioctrl)
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_FLASH_EFFECT, NULL, &ae_effect);
	ISP_TRACE_IF_FAIL(ret, ("fail to get ae flash effect"));

	flash_awb.effect = ae_effect;
	flash_awb.flash_ratio.r = cxt->pm_flash_info->cur.r_ratio;
	flash_awb.flash_ratio.g = cxt->pm_flash_info->cur.g_ratio;
	flash_awb.flash_ratio.b = cxt->pm_flash_info->cur.b_ratio;

	if (cxt->ae_cxt.flash_version) {
		/* dual flash */
		flash_awb.effect = 1024;
		flash_awb.flash_ratio.r = flash_wb_gain.r;
		flash_awb.flash_ratio.g = flash_wb_gain.g;
		flash_awb.flash_ratio.b = flash_wb_gain.b;
	}

	if (cxt->ops.awb_ops.ioctrl)
		ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_FLASHING, (void *)&flash_awb, NULL);
	ISP_TRACE_IF_FAIL(ret, ("fail to set awb flash gain"));

	ret = ispctl_set_awb_gain(cxt);
	ISP_TRACE_IF_FAIL(ret, ("fail to set awb gain"));

	return ret;
}

static cmr_s32 ispctl_smart_param_update(cmr_handle isp_alg_handle)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	cmr_u32 i = 0;
	struct smart_init_param smart_init_param;
	struct isp_pm_ioctl_input pm_input = { NULL, 0 };
	struct isp_pm_ioctl_output pm_output = { NULL, 0 };

	memset(&smart_init_param, 0, sizeof(smart_init_param));

	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_INIT_SMART, &pm_input, &pm_output);
	if ((ISP_SUCCESS == ret) && pm_output.param_data) {
		for (i = 0; i < pm_output.param_num; ++i) {
			smart_init_param.tuning_param[i].data.size = pm_output.param_data[i].data_size;
			smart_init_param.tuning_param[i].data.data_ptr = pm_output.param_data[i].data_ptr;
		}
	} else {
		ISP_LOGE("fail to get smart init param");
		return ret;
	}

	if (cxt->ops.smart_ops.ioctrl)
		ret = cxt->ops.smart_ops.ioctrl(cxt->smart_cxt.handle, ISP_SMART_IOCTL_GET_UPDATE_PARAM, (void *)&smart_init_param, NULL);
	if (ISP_SUCCESS != ret) {
		ISP_LOGE("fail to reinit smart param ");
		return ret;
	}

	ISP_LOGV("ISP_SMART: handle=%p, param=%p", cxt->smart_cxt.handle, pm_output.param_data->data_ptr);

	return ret;
}

static cmr_int ispctl_awb_mode(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_pm_param_data ioctl_data;
	struct isp_awbc_cfg awbc_cfg;
	struct awb_gain result;
	cmr_u32 awb_mode = *(cmr_u32 *) param_ptr;
	cmr_u32 awb_id;

	memset((void *)&ioctl_data, 0, sizeof(struct isp_pm_param_data));
	memset((void *)&awbc_cfg, 0, sizeof(struct isp_awbc_cfg));
	memset((void *)&result, 0, sizeof(struct awb_gain));

	switch (awb_mode) {
	case ISP_AWB_AUTO:
		awb_id = AWB_CTRL_WB_MODE_AUTO;
		break;
	case ISP_AWB_INDEX1:
		awb_id = AWB_CTRL_MWB_MODE_INCANDESCENT;
		break;
	case ISP_AWB_INDEX4:
		awb_id = AWB_CTRL_MWB_MODE_FLUORESCENT;
		break;
	case ISP_AWB_INDEX5:
		awb_id = AWB_CTRL_MWB_MODE_SUNNY;
		break;
	case ISP_AWB_INDEX6:
		awb_id = AWB_CTRL_MWB_MODE_CLOUDY;
		break;
	default:
		awb_id = AWB_CTRL_WB_MODE_AUTO;
		break;
	}

	ISP_LOGV("AWB_MODE :0x%x", awb_id);
	if (cxt->ops.awb_ops.ioctrl) {
		ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_SET_WB_MODE, (void *)&awb_id, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to set awb wb mode"));
	}

	return ret;
}

static cmr_int ispctl_ae_awb_bypass(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_u_blocks_info aem_block_info;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	cmr_u32 type = 0;
	cmr_u32 bypass = 0;

	if (NULL == param_ptr) {
		return ISP_PARAM_NULL;
	}

	memset(&aem_block_info, 0x0, sizeof(aem_block_info));
	type = *(cmr_u32 *) param_ptr;
	switch (type) {
	case 0:
		/*ae awb normal */
		aem_block_info.bypass = 0;
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_RAW_AEM_BYPASS, &aem_block_info, NULL);
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_BYPASS, &bypass, NULL);
		break;
	case 1:
		break;
	case 2:
		/*ae by pass */
		bypass = 1;
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_BYPASS, &bypass, NULL);
		break;
	case 3:
		/*awb by pass */
		aem_block_info.bypass = 1;
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_RAW_AEM_BYPASS, &aem_block_info, NULL);
		break;
	default:
		break;
	}

	ISP_LOGV("type=%d", type);
	return ret;
}

static cmr_int ispctl_ev(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct ae_set_ev set_ev = { 0 };

	if (NULL == param_ptr) {
		return ISP_PARAM_NULL;
	}

	set_ev.level = *(cmr_u32 *) param_ptr;
	if (cxt->ops.ae_ops.ioctrl)
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_EV_OFFSET, &set_ev, NULL);

	ISP_LOGV("ISP_AE: AE_SET_EV_OFFSET=%d, ret=%ld", set_ev.level, ret);

	return ret;
}

static cmr_int ispctl_flicker_bypass(cmr_handle isp_alg_handle, cmr_int bypass)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_u_blocks_info afl_block_info;

	memset(&afl_block_info, 0x0, sizeof(afl_block_info));
	afl_block_info.scene_id = ISP_MODE_PRV;
	afl_block_info.bypass = bypass;

	if (cxt->afl_cxt.afl_mode != AE_FLICKER_AUTO || cxt->sensor_fps.is_high_fps == 1)
		afl_block_info.bypass = 1;

	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AFL_NEW_BYPASS, &afl_block_info, NULL);
	if(ret) {
		ISP_LOGE("fail to set afl bypass");
	}

	return ret;
}

static cmr_int ispctl_flicker(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct ae_set_flicker set_flicker = { 0 };
	cmr_int bypass = 0;

	if (NULL == param_ptr) {
		return ISP_PARAM_NULL;
	}

	cxt->afl_cxt.afl_mode = *(cmr_u32 *) param_ptr;
	set_flicker.mode = *(cmr_u32 *) param_ptr;
	if (cxt->ops.ae_ops.ioctrl)
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_FLICKER, &set_flicker, NULL);
	ISP_LOGV("ISP_AE: AE_SET_FLICKER=%d, ret=%ld", set_flicker.mode, ret);

	ispctl_flicker_bypass(isp_alg_handle, bypass);

	return ret;
}

static cmr_int ispctl_snapshot_notice(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_snapshot_notice *isp_notice = param_ptr;
	struct ae_snapshot_notice ae_notice;

	if (NULL == cxt || NULL == isp_notice) {
		ISP_LOGE("fail to get valid handle %p and isp_notice %p ", cxt, isp_notice);
		return ISP_PARAM_NULL;
	}

	ae_notice.type = isp_notice->type;
	ae_notice.preview_line_time = isp_notice->preview_line_time;
	ae_notice.capture_line_time = isp_notice->capture_line_time;
	if (cxt->ops.ae_ops.ioctrl)
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_SNAPSHOT_NOTICE, &ae_notice, NULL);

	ISP_LOGV("done %ld", ret);
	return ret;
}

static cmr_int ispctl_flash_notice(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 ratio = 0;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_flash_notice *flash_notice = (struct isp_flash_notice *)param_ptr;
	struct ae_flash_notice ae_notice;
	enum smart_ctrl_flash_mode flash_mode = 0;
	enum awb_ctrl_flash_status awb_flash_status = 0;
	float captureFlashEnvRatio=0.0; //0-1, flash/ (flash+environment)
	float captureFlash1ofALLRatio=0.0; //0-1,  flash1 / (flash1+flash2)
	struct alsc_flash_info flash_info = { 0, 0};

	if (NULL == cxt || NULL == flash_notice) {
		ISP_LOGE("fail to get valid handle %p,notice %p ", cxt, flash_notice);
		return ISP_PARAM_NULL;
	}

	switch (flash_notice->mode) {
	case ISP_FLASH_PRE_BEFORE:
		ispctl_flicker_bypass(isp_alg_handle, 1);
		cxt->lsc_flash_onoff = 1;
		if (cxt->ops.lsc_ops.ioctrl)
			ret = cxt->ops.lsc_ops.ioctrl(cxt->lsc_cxt.handle, ALSC_FLASH_PRE_BEFORE, NULL, NULL);

		ae_notice.mode = AE_FLASH_PRE_BEFORE;
		if (cxt->ops.af_ops.ioctrl)
			ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_SET_FLASH_NOTICE, (void *)&(flash_notice->mode), NULL);

		ae_notice.power.max_charge = flash_notice->power.max_charge;
		ae_notice.power.max_time = flash_notice->power.max_time;
		ae_notice.capture_skip_num = flash_notice->capture_skip_num;
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_FLASH_NOTICE, &ae_notice, NULL);

		if (cxt->ops.awb_ops.ioctrl) {
			ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_FLASH_BEFORE_P, NULL, NULL);
			awb_flash_status = AWB_FLASH_PRE_BEFORE;
			ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_SET_FLASH_STATUS, (void *)&awb_flash_status, NULL);
		}

		flash_mode = SMART_CTRL_FLASH_PRE;
		if (cxt->ops.smart_ops.ioctrl)
			ret = cxt->ops.smart_ops.ioctrl(cxt->smart_cxt.handle, ISP_SMART_IOCTL_SET_FLASH_MODE, (void *)&flash_mode, NULL);
		break;

	case ISP_FLASH_PRE_LIGHTING:
		if (ISP_SUCCESS == ret)
			ratio = cxt->pm_flash_info->cur.lum_ratio;

		ae_notice.mode = AE_FLASH_PRE_LIGHTING;
		ae_notice.flash_ratio = ratio;
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_FLASH_NOTICE, &ae_notice, NULL);

		awb_flash_status = AWB_FLASH_PRE_LIGHTING;
		if (cxt->ops.awb_ops.ioctrl) {
			ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_SET_FLASH_STATUS, (void *)&awb_flash_status, NULL);
			ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_FLASH_OPEN_P, NULL, NULL);
		}

		flash_mode = SMART_CTRL_FLASH_PRE;
		if (cxt->ops.smart_ops.ioctrl)
			ret = cxt->ops.smart_ops.ioctrl(cxt->smart_cxt.handle, ISP_SMART_IOCTL_SET_FLASH_MODE, (void *)&flash_mode, NULL);
		if (cxt->ops.af_ops.ioctrl)
			ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_SET_FLASH_NOTICE, (void *)&(flash_notice->mode), NULL);
		if (cxt->ops.lsc_ops.ioctrl)
			ret = cxt->ops.lsc_ops.ioctrl(cxt->lsc_cxt.handle, ALSC_FLASH_PRE_LIGHTING, NULL, NULL);

		break;

	case ISP_FLASH_PRE_AFTER:
		ispctl_flicker_bypass(isp_alg_handle, 0);
		ae_notice.mode = AE_FLASH_PRE_AFTER;
		ae_notice.will_capture = flash_notice->will_capture;
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_FLASH_NOTICE, &ae_notice, NULL);
		awb_flash_status = AWB_FLASH_PRE_AFTER;
		if (cxt->ops.awb_ops.ioctrl)
			ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_SET_FLASH_STATUS, (void *)&awb_flash_status, NULL);

		if (cxt->ops.awb_ops.ioctrl)
			ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_FLASH_CLOSE, NULL, NULL);
		ret = ispctl_set_awb_gain((cmr_handle) cxt);

		flash_mode = SMART_CTRL_FLASH_CLOSE;
		if (cxt->ops.smart_ops.ioctrl)
			ret = cxt->ops.smart_ops.ioctrl(cxt->smart_cxt.handle, ISP_SMART_IOCTL_SET_FLASH_MODE, (void *)&flash_mode, NULL);
		if (cxt->ops.af_ops.ioctrl)
			ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_SET_FLASH_NOTICE, (void *)&(flash_notice->mode), NULL);

		//lnc flash update
		cxt->lsc_flash_onoff = 0;
		captureFlashEnvRatio = 0.0; //0-1, flash/ (flash+environment)
		captureFlash1ofALLRatio = 0.0; //0-1,  flash1 / (flash1+flash2)
		if (cxt->ops.ae_ops.ioctrl) {
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_FLASH_ENV_RATIO, NULL, (void *)&captureFlashEnvRatio);
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_FLASH_ONE_OF_ALL_RATIO, NULL, (void *)&captureFlash1ofALLRatio);
		}
		flash_info.io_captureFlashEnvRatio = captureFlashEnvRatio;
		flash_info.io_captureFlash1Ratio = captureFlash1ofALLRatio;
		if (cxt->ops.lsc_ops.ioctrl)
			ret = cxt->ops.lsc_ops.ioctrl(cxt->lsc_cxt.handle, ALSC_FLASH_PRE_AFTER, (void*)&flash_info, NULL);
		break;

	case ISP_FLASH_MAIN_BEFORE:
		ispctl_flicker_bypass(isp_alg_handle, 1);
		ae_notice.mode = AE_FLASH_MAIN_BEFORE;
		if (cxt->ops.af_ops.ioctrl)
			ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_SET_FLASH_NOTICE, (void *)&(flash_notice->mode), NULL);
		if (cxt->ops.ae_ops.ioctrl) {
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_FLASH_NOTICE, &ae_notice, NULL);
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_EXP_GAIN, NULL, NULL);
		}
		awb_flash_status = AWB_FLASH_MAIN_BEFORE;
		if (cxt->ops.awb_ops.ioctrl)
			ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_SET_FLASH_STATUS, (void *)&awb_flash_status, NULL);
		ret = ispctl_set_awb_flash_gain((cmr_handle) cxt);
		if (cxt->ops.awb_ops.ioctrl)
			ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_LOCK, NULL, NULL);

		cxt->lsc_flash_onoff = 1;
		if (cxt->ops.lsc_ops.ioctrl)
			ret = cxt->ops.lsc_ops.ioctrl(cxt->lsc_cxt.handle, ALSC_FLASH_MAIN_BEFORE, NULL, NULL);
		break;

	case ISP_FLASH_MAIN_LIGHTING:
		if (cxt->ops.lsc_ops.ioctrl)
			ret = cxt->ops.lsc_ops.ioctrl(cxt->lsc_cxt.handle, ALSC_FLASH_MAIN_LIGHTING, NULL, NULL);

		ae_notice.mode = AE_FLASH_MAIN_LIGHTING;
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_FLASH_NOTICE, &ae_notice, NULL);

		awb_flash_status = AWB_FLASH_MAIN_LIGHTING;
		if (cxt->ops.awb_ops.ioctrl) {
			ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_SET_FLASH_STATUS, (void *)&awb_flash_status, NULL);
			ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_FLASH_OPEN_M, NULL, NULL);
		}

		flash_mode = SMART_CTRL_FLASH_MAIN;
		if (cxt->ops.smart_ops.ioctrl)
			ret = cxt->ops.smart_ops.ioctrl(cxt->smart_cxt.handle, ISP_SMART_IOCTL_SET_FLASH_MODE, (void *)&flash_mode, NULL);
		if (cxt->ops.af_ops.ioctrl)
			ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_SET_FLASH_NOTICE, (void *)&(flash_notice->mode), NULL);
		break;

	case ISP_FLASH_MAIN_AE_MEASURE:
		ae_notice.mode = AE_FLASH_MAIN_AE_MEASURE;
		ae_notice.flash_ratio = 0;
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_FLASH_NOTICE, &ae_notice, NULL);

		if (!cxt->ae_cxt.flash_version) {
			awb_flash_status = AWB_FLASH_MAIN_MEASURE;
			if (cxt->ops.awb_ops.ioctrl)
				ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_SET_FLASH_STATUS, (void *)&awb_flash_status, NULL);
		}
		break;

	case ISP_FLASH_MAIN_AFTER:
		ispctl_flicker_bypass(isp_alg_handle, 0);
		if (cxt->ops.lsc_ops.ioctrl)
			ret = cxt->ops.lsc_ops.ioctrl(cxt->lsc_cxt.handle, ALSC_FLASH_MAIN_AFTER, NULL, NULL);
		cxt->lsc_flash_onoff = 0;
		ae_notice.mode = AE_FLASH_MAIN_AFTER;
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_FLASH_NOTICE, &ae_notice, NULL);

		awb_flash_status = AWB_FLASH_MAIN_AFTER;
		if (cxt->ops.awb_ops.ioctrl)
			ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_SET_FLASH_STATUS, (void *)&awb_flash_status, NULL);

		if (cxt->ops.awb_ops.ioctrl)
			ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_FLASH_CLOSE, NULL, NULL);
		ret = ispctl_set_awb_gain((cmr_handle) cxt);
		if (cxt->ops.awb_ops.ioctrl) {
			ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_UNLOCK, NULL, NULL);
			ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_FLASH_SNOP, NULL, NULL);
		}

		flash_mode = SMART_CTRL_FLASH_CLOSE;
		if (cxt->ops.smart_ops.ioctrl)
			ret = cxt->ops.smart_ops.ioctrl(cxt->smart_cxt.handle, ISP_SMART_IOCTL_SET_FLASH_MODE, (void *)&flash_mode, NULL);
		if (cxt->ops.af_ops.ioctrl)
			ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_SET_FLASH_NOTICE, (void *)&(flash_notice->mode), NULL);
		break;

	case ISP_FLASH_AF_DONE:
		break;

	case ISP_FLASH_SLAVE_FLASH_TORCH:
		ae_notice.mode = AE_LED_FLASH_ON;
		if (cxt->camera_id == 1) {
			if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_FLASH_NOTICE, &ae_notice, NULL);
		}
		break;

	case ISP_FLASH_SLAVE_FLASH_AUTO:
		ae_notice.mode = AE_LED_FLASH_AUTO;
		if (cxt->camera_id == 1) {
			if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_FLASH_NOTICE, &ae_notice, NULL);
		}
		break;

	case ISP_FLASH_SLAVE_FLASH_OFF:
		ae_notice.mode = AE_LED_FLASH_OFF;
		if (cxt->camera_id == 1) {
			if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_FLASH_NOTICE, &ae_notice, NULL);
		}
		break;

	default:
		break;
	}

	return ret;
}

static cmr_int ispctl_iso(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct ae_set_iso set_iso = { 0 };

	if (NULL == param_ptr)
		return ISP_PARAM_NULL;

	set_iso.mode = *(cmr_u32 *) param_ptr;
	if (cxt->ops.ae_ops.ioctrl)
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_ISO, &set_iso, NULL);
	ISP_LOGV("ISO=%d, ret=%ld", set_iso.mode, ret);

	return ret;
}

static cmr_int ispctl_brightness(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_s32 i = 0;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_bright_cfg cfg = { 0 };
	struct isp_pm_param_data param_data[ISP_MODE_MAX];
	struct isp_pm_ioctl_input input = { NULL, 0 };
	struct isp_pm_ioctl_output output = { NULL, 0 };

	cfg.factor = *(cmr_u32 *) param_ptr;
	memset(param_data, 0x0, sizeof(param_data));
	if (cxt->zsl_flag) {
		for (i = 0; i < ISP_MODE_MAX; i++) {
			BLOCK_PARAM_CFG(param_data[i], ISP_PM_BLK_BRIGHT,
					ISP_BLK_BRIGHT,
					cxt->mode_id[i],
					&cfg, sizeof(cfg));
		}
		input.param_num = ISP_MODE_MAX;
	} else {
		BLOCK_PARAM_CFG(param_data[0], ISP_PM_BLK_BRIGHT,
				ISP_BLK_BRIGHT,
				cxt->mode_id[0],
				&cfg, sizeof(cfg));
		input.param_num = 1;
	}
	input.param_data_ptr = param_data;
	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_OTHERS, &input, &output);

	return ret;
}

static cmr_int ispctl_contrast(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_s32 i = 0;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_contrast_cfg cfg = { 0 };
	struct isp_pm_param_data param_data[ISP_MODE_MAX];
	struct isp_pm_ioctl_input input = { NULL, 0 };
	struct isp_pm_ioctl_output output = { NULL, 0 };

	cfg.factor = *(cmr_u32 *) param_ptr;
	memset(param_data, 0x0, sizeof(param_data));
	if (cxt->zsl_flag) {
		for (i = 0; i < ISP_MODE_MAX; i++) {
			BLOCK_PARAM_CFG(param_data[i], ISP_PM_BLK_CONTRAST,
					ISP_BLK_CONTRAST,
					cxt->mode_id[i],
					&cfg, sizeof(cfg));
		}
		input.param_num = ISP_MODE_MAX;
	} else {
		BLOCK_PARAM_CFG(param_data[0], ISP_PM_BLK_CONTRAST,
				ISP_BLK_CONTRAST,
				cxt->mode_id[0],
				&cfg, sizeof(cfg));
		input.param_num = 1;
	}
	input.param_data_ptr = param_data;
	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_OTHERS, &input, &output);

	return ret;
}

static cmr_int ispctl_saturation(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_s32 i = 0;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_saturation_cfg cfg = { 0 };
	struct isp_pm_param_data param_data[ISP_MODE_MAX];
	struct isp_pm_ioctl_input input = { NULL, 0 };
	struct isp_pm_ioctl_output output = { NULL, 0 };

	cfg.factor = *(cmr_u32 *) param_ptr;
	memset(param_data, 0x0, sizeof(param_data));
	if (cxt->zsl_flag) {
		for (i = 0; i < ISP_MODE_MAX; i++) {
			BLOCK_PARAM_CFG(param_data[i], ISP_PM_BLK_SATURATION,
					ISP_BLK_SATURATION,
					cxt->mode_id[i],
					&cfg, sizeof(cfg));
		}
		input.param_num = ISP_MODE_MAX;
	} else {
		BLOCK_PARAM_CFG(param_data[0], ISP_PM_BLK_SATURATION,
				ISP_BLK_SATURATION,
				cxt->mode_id[0],
				&cfg, sizeof(cfg));
		input.param_num = 1;
	}
	input.param_data_ptr = param_data;
	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_OTHERS, &input, &output);

	return ret;
}

static cmr_int ispctl_sharpness(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_s32 i = 0;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_edge_cfg cfg = { 0 };
	struct isp_pm_param_data param_data[ISP_MODE_MAX];
	struct isp_pm_ioctl_input input = { NULL, 0 };
	struct isp_pm_ioctl_output output = { NULL, 0 };

	cfg.factor = *(cmr_u32 *) param_ptr;
	memset(param_data, 0x0, sizeof(param_data));
	if (cxt->zsl_flag) {
		for (i = 0; i < ISP_MODE_MAX; i++) {
			BLOCK_PARAM_CFG(param_data[i], ISP_PM_BLK_EDGE_STRENGTH,
					ISP_BLK_EDGE,
					cxt->mode_id[i],
					&cfg, sizeof(cfg));
		}
		input.param_num = ISP_MODE_MAX;
	} else {
		BLOCK_PARAM_CFG(param_data[0], ISP_PM_BLK_EDGE_STRENGTH,
				ISP_BLK_EDGE,
				cxt->mode_id[0],
				&cfg, sizeof(cfg));
		input.param_num = 1;
	}
	input.param_data_ptr = param_data;
	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_OTHERS, &input, &output);

	return ret;
}

static cmr_int ispctl_video_mode(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	cmr_s32 mode = 0;
	cmr_u32 idx = 0;
	struct ae_set_fps fps;
	struct isp_pm_param_data param_data;
	struct isp_pm_ioctl_input input = { NULL, 0 };
	struct isp_pm_ioctl_output output = { NULL, 0 };

	if (NULL == param_ptr) {
		ISP_LOGE("fail to get valid param !");
		return ISP_ERROR;
	}
	memset((void *)&param_data, 0, sizeof(struct isp_pm_param_data));

	ISP_LOGV("param val=%d", *((cmr_s32 *) param_ptr));

	if (0 == *((cmr_s32 *) param_ptr)) {
		mode = ISP_MODE_ID_PRV_0;
	} else {
		ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_MODEID_BY_FPS, param_ptr, &mode);
		if (ret) {
			ISP_LOGE("fail to get mode ID by fps");
		}
	}

	fps.min_fps = *((cmr_u32 *) param_ptr);
	fps.max_fps = 0;
	if (cxt->ops.ae_ops.ioctrl) {
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_FPS, &fps, NULL);
		ISP_TRACE_IF_FAIL(ret, ("fail to set ae fps"));
	}

	if (0 != *((cmr_u32 *) param_ptr)) {
		cmr_u32 work_mode = 2;
		if (cxt->ops.awb_ops.ioctrl) {
			ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_SET_WORK_MODE, &work_mode, NULL);
			ISP_RETURN_IF_FAIL(ret, ("fail to awb set_work_mode"));
		}
	} else {
		cmr_u32 work_mode = 0;
		if (cxt->ops.awb_ops.ioctrl) {
			ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_SET_WORK_MODE, &work_mode, NULL);
			ISP_RETURN_IF_FAIL(ret, ("fail to awb set_work_mode"));
		}
	}

#ifdef SEPARATE_GAMMA_IN_VIDEO
	if (*((cmr_u32 *) param_ptr) != 0) {
		idx = VIDEO_GAMMA_INDEX;
		if (cxt->ops.smart_ops.block_disable)
			cxt->ops.smart_ops.block_disable(cxt->smart_cxt.handle, ISP_SMART_GAMMA);
		BLOCK_PARAM_CFG(param_data, ISP_PM_BLK_GAMMA, ISP_BLK_RGB_GAMC, mode, &idx, sizeof(idx));
		input.param_num = 1;
		input.param_data_ptr = &param_data;
		isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_OTHERS, (void *)&input, (void *)&output);
#ifdef Y_GAMMA_SMART_WITH_RGB_GAMMA
		BLOCK_PARAM_CFG(param_data, ISP_PM_BLK_YGAMMA, ISP_BLK_Y_GAMMC, mode, &idx, sizeof(idx));
		input.param_num = 1;
		input.param_data_ptr = &param_data;
		isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_OTHERS, (void *)&input, (void *)&output);
#endif
	} else {
		if (cxt->ops.smart_ops.block_enable)
			cxt->ops.smart_ops.block_enable(cxt->smart_cxt.handle, ISP_SMART_GAMMA);
	}
#endif
	return ret;
}

static cmr_int ispctl_range_fps(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_range_fps *range_fps = (struct isp_range_fps *)param_ptr;
	struct ae_set_fps fps;

	if (NULL == range_fps) {
		ISP_LOGE("fail to get valid param !");
		return ISP_PARAM_NULL;
	}

	ISP_LOGV("param val=%d", *((cmr_s32 *) param_ptr));

	fps.min_fps = range_fps->min_fps;
	fps.max_fps = range_fps->max_fps;
	if (cxt->ops.ae_ops.ioctrl)
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_FPS, &fps, NULL);

	return ret;
}

static cmr_int ispctl_ae_online(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	if (cxt->ops.ae_ops.ioctrl)
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_ONLINE_CTRL, param_ptr, param_ptr);

	return ret;
}

static cmr_int ispctl_ae_force(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct ae_calc_out ae_result;
	cmr_u32 ae;

	memset((void *)&ae_result, 0, sizeof(struct ae_calc_out));

	if (NULL == param_ptr) {
		ISP_LOGE("fail to get valid param!");
		return ISP_PARAM_NULL;
	}

	ae = *(cmr_u32 *) param_ptr;

	if (0 == ae) {		//lock
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_FORCE_PAUSE, NULL, (void *)&ae_result);
	} else {		//unlock
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_FORCE_RESTORE, NULL, (void *)&ae_result);
	}

	ISP_LOGV("ret %ld", ret);

	return ret;
}

static cmr_int ispctl_get_ae_state(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	cmr_u32 param = 0;

	if (NULL == param_ptr) {
		ISP_LOGE("fail to get valid param!");
		return ISP_PARAM_NULL;
	}

	if (cxt->ops.ae_ops.ioctrl)
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_AE_STATE, NULL, (void *)&param);

	if (AE_STATE_LOCKED == param) {	//lock
		*(cmr_u32 *) param_ptr = 0;
	} else {		//unlock
		*(cmr_u32 *) param_ptr = 1;
	}

	ISP_LOGV("ret %ld param %d ae %d", ret, param, *(cmr_u32 *) param_ptr);

	return ret;
}

static cmr_int ispctl_set_ae_fps(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct ae_set_fps *fps = (struct ae_set_fps *)param_ptr;

	if (NULL == fps) {
		ISP_LOGE("fail to get valid param!");
		return ISP_PARAM_NULL;
	}

	ISP_LOGV("LLS min_fps =%d, max_fps = %d", fps->min_fps, fps->max_fps);

	if (cxt->ops.ae_ops.ioctrl)
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_FPS, fps, NULL);

	return ret;
}

static cmr_s32 ispctl_get_ae_debug_info(cmr_handle isp_alg_handle)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct tg_ae_ctrl_alc_log ae_log = { NULL, 0 };

	if (NULL == cxt) {
		ISP_LOGE("fail to get AE debug info !");
		ret = ISP_ERROR;
		return ret;
	}

	if (cxt->ops.ae_ops.ioctrl) {
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_DEBUG_INFO, NULL, (void *)&ae_log);
		if (ISP_SUCCESS != ret) {
			ISP_LOGE("fail to get AE debug info!");
		}
	}
	cxt->ae_cxt.log_ae = ae_log.log;
	cxt->ae_cxt.log_ae_size = ae_log.size;

	return ret;
}

static cmr_s32 ispctl_get_alsc_debug_info(cmr_handle isp_alg_handle)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct tg_alsc_debug_info lsc_log = { NULL, 0 };
	cmr_u32 lsc_sprd_version = cxt->lsc_cxt.lsc_sprd_version;

	if (NULL == cxt) {
		ISP_LOGE("fail to get ALSC debug info error!");
		ret = ISP_ERROR;
		return ret;
	}

	if (lsc_sprd_version >= 3) {
		if (cxt->ops.lsc_ops.ioctrl)
			ret = cxt->ops.lsc_ops.ioctrl(cxt->lsc_cxt.handle, ALSC_CMD_GET_DEBUG_INFO, NULL, (void *)&lsc_log);
		if (ISP_SUCCESS != ret) {
			ISP_LOGE("fail to get  ALSC debug info failed!");
		}
		cxt->lsc_cxt.log_lsc = lsc_log.log;
		cxt->lsc_cxt.log_lsc_size = lsc_log.size;
	}

	return ret;
}

static size_t calc_log_size(const void *log, size_t size, const char *begin_magic, const char *end_magic)
{
	if (!log || !size)
		return 0;

	return size + strlen(begin_magic) + strlen(end_magic);
}

#define COPY_MAGIC(m) \
{size_t len; len = strlen(m); memcpy((char *)dst + off, m, len); off += len;}

static size_t copy_log(void *dst, const void *log, size_t size,
			const char *begin_magic, const char *end_magic,
			size_t offset, cmr_u32 total_size)
{
	size_t off = 0;

	if (!log || !size)
		return 0;

	if ((offset + size + strlen(begin_magic) + strlen(end_magic)) >
			(size_t)total_size) {
		ISP_LOGE("fail to mempcy");
		return ISP_ERROR;
	}

	COPY_MAGIC(begin_magic);
	memcpy((char *)dst + off, log, size);
	off += size;
	COPY_MAGIC(end_magic);
	return off;
}

static cmr_int ispctl_get_info(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_info *info_ptr = param_ptr;
	cmr_u32 total_size = 0;
	cmr_u32 mem_offset = 0;
	struct sprd_isp_debug_info *p;
	struct _isp_log_info log;
	size_t off;

	if (NULL == info_ptr) {
		ISP_LOGE("fail to get valid param ");
		return ISP_PARAM_NULL;
	}

	if (cxt->awb_cxt.alc_awb) {
		total_size = cxt->awb_cxt.log_alc_awb_size + cxt->awb_cxt.log_alc_lsc_size;

		if (cxt->ae_cxt.log_alc_size < total_size) {
			if (cxt->ae_cxt.log_alc != NULL) {
				free(cxt->ae_cxt.log_alc);
				cxt->ae_cxt.log_alc = NULL;
			}
			cxt->ae_cxt.log_alc = malloc(total_size);
			if (cxt->ae_cxt.log_alc == NULL) {
				cxt->ae_cxt.log_alc_size = 0;
				return ISP_ERROR;
			}
			cxt->ae_cxt.log_alc_size = total_size;
		}

		if (cxt->awb_cxt.log_alc_awb != NULL) {
			memcpy(cxt->ae_cxt.log_alc, cxt->awb_cxt.log_alc_awb, cxt->awb_cxt.log_alc_awb_size);
		}
		mem_offset += cxt->awb_cxt.log_alc_awb_size;
		if (cxt->awb_cxt.log_alc_lsc != NULL) {
			memcpy(cxt->ae_cxt.log_alc + mem_offset, cxt->awb_cxt.log_alc_lsc, cxt->awb_cxt.log_alc_lsc_size);
		}
		mem_offset += cxt->awb_cxt.log_alc_lsc_size;

		info_ptr->addr = cxt->ae_cxt.log_alc;
		info_ptr->size = cxt->ae_cxt.log_alc_size;
	} else {
		if (ISP_SUCCESS != ispctl_get_ae_debug_info(cxt)) {
			ISP_LOGE("fail to get ae debug info");
		}

		if (ISP_SUCCESS != ispctl_get_alsc_debug_info(cxt)) {
			ISP_LOGE("fail to get alsc debug info");
		}

		total_size = sizeof(struct sprd_isp_debug_info) + sizeof(isp_log_info_t)
		    + calc_log_size(cxt->ae_cxt.log_ae, cxt->ae_cxt.log_ae_size, AE_START, AE_END)
		    + calc_log_size(cxt->af_cxt.log_af, cxt->af_cxt.log_af_size, AF_START, AF_END)
		    + calc_log_size(cxt->aft_cxt.log_aft, cxt->aft_cxt.log_aft_size, AFT_START, AFT_END)
		    + calc_log_size(cxt->awb_cxt.log_awb, cxt->awb_cxt.log_awb_size, AWB_START, AWB_END)
		    + calc_log_size(cxt->lsc_cxt.log_lsc, cxt->lsc_cxt.log_lsc_size, LSC_START, LSC_END)
		    + calc_log_size(cxt->smart_cxt.log_smart, cxt->smart_cxt.log_smart_size, SMART_START, SMART_END)
		    + sizeof(cmr_u32); /*for end flag */

		if (cxt->otp_data != NULL) {
			total_size += calc_log_size(cxt->otp_data->total_otp.data_ptr,
				cxt->otp_data->total_otp.size, OTP_START, OTP_END);
		}

		if (cxt->commn_cxt.log_isp_size < total_size) {
			if (cxt->commn_cxt.log_isp != NULL) {
				free(cxt->commn_cxt.log_isp);
				cxt->commn_cxt.log_isp = NULL;
			}
			cxt->commn_cxt.log_isp = malloc(total_size);
			if (cxt->commn_cxt.log_isp == NULL) {
				ISP_LOGE("fail to malloc %d", total_size);
				cxt->commn_cxt.log_isp_size = 0;
				info_ptr->addr = 0;
				info_ptr->size = 0;
				return ISP_ERROR;
			}
			cxt->commn_cxt.log_isp_size = total_size;
		}

		p = (struct sprd_isp_debug_info *)cxt->commn_cxt.log_isp;
		p->debug_startflag = SPRD_DEBUG_START_FLAG;
		*((cmr_u32 *) ((cmr_u8 *) p + total_size - 4)) = SPRD_DEBUG_END_FLAG;
		p->debug_len = total_size;
		p->version_id = SPRD_DEBUG_VERSION_ID;

		memset(&log, 0, sizeof(log));
		memcpy(log.magic, DEBUG_MAGIC, sizeof(log.magic));
		log.ver = 0;

		off = sizeof(struct sprd_isp_debug_info) + sizeof(isp_log_info_t);
		COPY_LOG(ae, AE);
		COPY_LOG(af, AF);
		COPY_LOG(aft, AFT);
		COPY_LOG(awb, AWB);
		COPY_LOG(lsc, LSC);
		COPY_LOG(smart, SMART);

		if (cxt->otp_data != NULL) {
			size_t len = copy_log(cxt->commn_cxt.log_isp + off,
					cxt->otp_data->total_otp.data_ptr,
					cxt->otp_data->total_otp.size,
					OTP_START, OTP_END, off, total_size);
			if (len) {
				log.otp_off = off;
				off += len;
				log.otp_len = len;
			} else {
				log.otp_off = 0;
			}
		}
		memcpy((char *)cxt->commn_cxt.log_isp + sizeof(struct sprd_isp_debug_info), &log, sizeof(log));

		info_ptr->addr = cxt->commn_cxt.log_isp;
		info_ptr->size = cxt->commn_cxt.log_isp_size;
	}

	ISP_LOGV("ISP INFO:addr 0x%p, size = %d", info_ptr->addr, info_ptr->size);
	return ret;
}

static cmr_int ispctl_get_awb_gain(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct awb_gain result;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_awbc_cfg *awbc_cfg = (struct isp_awbc_cfg *)param_ptr;

	if (NULL == awbc_cfg) {
		ISP_LOGE("fail to get valid param!");
		return ISP_PARAM_NULL;
	}
	memset(&result, 0, sizeof(result));
	if (cxt->ops.awb_ops.ioctrl)
		ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_GET_GAIN, (void *)&result, NULL);

	awbc_cfg->r_gain = result.r;
	awbc_cfg->g_gain = result.g;
	awbc_cfg->b_gain = result.b;
	awbc_cfg->r_offset = 0;
	awbc_cfg->g_offset = 0;
	awbc_cfg->b_offset = 0;

	ISP_LOGV("ret %ld r %d g %d b %d", ret, result.r, result.g, result.b);

	return ret;
}

static cmr_int ispctl_awb_ct(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	cmr_u32 param = 0;

	if (NULL == param_ptr) {
		ISP_LOGE("fail to get valid param !");
		return ISP_PARAM_NULL;
	}

	if (cxt->ops.awb_ops.ioctrl)
		ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_GET_CT, (void *)&param, NULL);
	*(cmr_u32 *) param_ptr = param;

	return ret;
}

static cmr_int ispctl_set_lum(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	cmr_u32 param = 0;

	if (NULL == param_ptr) {
		ISP_LOGE("fail to get valid param !");
		return ISP_PARAM_NULL;
	}

	if (cxt->ops.ae_ops.ioctrl)
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_TARGET_LUM, param_ptr, (void *)&param);

	ISP_LOGV("ret %ld param %d Lum %d", ret, param, *(cmr_u32 *) param_ptr);

	return ret;
}

static cmr_int ispctl_get_lum(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	cmr_u32 param = 0;

	if (NULL == param_ptr) {
		ISP_LOGE("fail to get valid param !");
		return ISP_PARAM_NULL;
	}

	if (cxt->ops.ae_ops.ioctrl)
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_LUM, NULL, (void *)&param);
	*(cmr_u32 *) param_ptr = param;

	ISP_LOGV("ret %ld param %d Lum %d", ret, param, *(cmr_u32 *) param_ptr);

	return ret;
}

static cmr_int ispctl_af_stop(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	UNUSED(param_ptr);
	if (cxt->ops.af_ops.ioctrl)
		ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_SET_AF_STOP, NULL, NULL);

	return ret;
}

static cmr_int ispctl_online_flash(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	UNUSED(param_ptr);

	cxt->commn_cxt.callback(cxt->commn_cxt.caller_id,
				ISP_CALLBACK_EVT | ISP_ONLINE_FLASH_CALLBACK,
				param_ptr, 0);

	return ret;
}

static cmr_int ispctl_ae_measure_lum(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct ae_set_weight set_weight = { 0 };

	if (NULL == param_ptr) {
		ISP_LOGE("fail to get valid param!");
		return ISP_PARAM_NULL;
	}

	set_weight.mode = *(cmr_u32 *) param_ptr;
	if (cxt->ops.ae_ops.ioctrl)
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_WEIGHT, &set_weight, NULL);
	ISP_LOGV("ISP_AE: AE_SET_WEIGHT=%d, ret=%ld", set_weight.mode, ret);

	return ret;
}

static cmr_int ispctl_scene_mode(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct ae_set_scene set_scene = { 0 };

	if (NULL == param_ptr) {
		ISP_LOGE("fail to get valid param !");
		return ISP_PARAM_NULL;
	}

	set_scene.mode = *(cmr_u32 *) param_ptr;
	cxt->commn_cxt.scene_flag = set_scene.mode;
	if (cxt->ops.ae_ops.ioctrl)
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_SCENE_MODE, &set_scene, NULL);

	return ret;
}

static cmr_int ispctl_sfti_read(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;

	UNUSED(isp_alg_handle);
	UNUSED(param_ptr);
	return ret;
}

static cmr_int ispctl_sfti_write(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	UNUSED(isp_alg_handle);
	UNUSED(param_ptr);

	return ret;
}

static cmr_int ispctl_sfti_set_pass(cmr_handle isp_alg_handle, void *param_ptr)
{
	UNUSED(isp_alg_handle);
	UNUSED(param_ptr);

	return ISP_SUCCESS;
}

static cmr_int ispctl_sfti_set_bypass(cmr_handle isp_alg_handle, void *param_ptr)
{
	UNUSED(isp_alg_handle);
	UNUSED(param_ptr);

	return ISP_SUCCESS;
}

static cmr_int ispctl_sfti_get_af_value(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	UNUSED(isp_alg_handle);
	UNUSED(param_ptr);
	return ret;
}

static cmr_int ispctl_af_get_full_scan_info(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_af_fullscan_info *af_fullscan_info = (struct isp_af_fullscan_info *)param_ptr;

	if (cxt->ops.af_ops.ioctrl)
		ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle,
					AF_CMD_GET_AF_FULLSCAN_INFO,
					(void *)af_fullscan_info, NULL);

	return ret;
}

static cmr_int ispctl_af_bypass(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	cmr_u32 *bypass = (cmr_u32 *) param_ptr;

	if (cxt->ops.af_ops.ioctrl)
		ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_SET_AF_BYPASS, (void *)bypass, NULL);

	return ret;
}

static cmr_int ispctl_af(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_af_win *af_ptr = (struct isp_af_win *)param_ptr;
	struct af_trig_info trig_info;
	cmr_u32 i;

	trig_info.win_num = af_ptr->valid_win;
	switch (af_ptr->mode) {
	case ISP_FOCUS_TRIG:
		trig_info.mode = AF_MODE_NORMAL;
		break;
	case ISP_FOCUS_MACRO:
		trig_info.mode = AF_MODE_MACRO;
		break;
	case ISP_FOCUS_CONTINUE:
		trig_info.mode = AF_MODE_CONTINUE;
		break;
	case ISP_FOCUS_MANUAL:
		trig_info.mode = AF_MODE_MANUAL;
		break;
	case ISP_FOCUS_VIDEO:
		trig_info.mode = AF_MODE_VIDEO;
		break;
	default:
		trig_info.mode = AF_MODE_NORMAL;
		break;
	}

	for (i = 0; i < trig_info.win_num; i++) {
		trig_info.win_pos[i].sx = af_ptr->win[i].start_x;
		trig_info.win_pos[i].sy = af_ptr->win[i].start_y;
		trig_info.win_pos[i].ex = af_ptr->win[i].end_x;
		trig_info.win_pos[i].ey = af_ptr->win[i].end_y;
	}
	if (cxt->ops.af_ops.ioctrl)
		ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_SET_AF_START, (void *)&trig_info, NULL);

	return ret;
}

static cmr_int ispctl_burst_notice(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	UNUSED(isp_alg_handle);
	UNUSED(param_ptr);

	return ret;
}

static cmr_int ispctl_special_effect(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_s32 i = 0;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_pm_param_data param_data[ISP_MODE_MAX];
	struct isp_pm_ioctl_input input = { NULL, 0 };
	struct isp_pm_ioctl_output output = { NULL, 0 };

	memset(param_data, 0x0, sizeof(param_data));
	if (cxt->zsl_flag) {
		for (i = 0; i < ISP_MODE_MAX; i++) {
			BLOCK_PARAM_CFG(param_data[i], ISP_PM_BLK_SPECIAL_EFFECT,
					ISP_BLK_CCE,
					cxt->mode_id[i],
					param_ptr, sizeof(param_ptr));
		}
		input.param_num = ISP_MODE_MAX;
	} else {
		BLOCK_PARAM_CFG(param_data[0], ISP_PM_BLK_SPECIAL_EFFECT,
				ISP_BLK_CCE,
				cxt->mode_id[0],
				param_ptr, sizeof(param_ptr));
		input.param_num = 1;
	}
	input.param_data_ptr = param_data;
	isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_SPECIAL_EFFECT, (void *)&input, (void *)&output);

	return ret;
}

static cmr_int ispctl_fix_param_update(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct sensor_raw_info *sensor_raw_info_ptr = NULL;
	struct isp_pm_init_input input;
	cmr_u32 i;
	struct sensor_version_info *version_info = PNULL;
	cmr_u32 param_source = 0;
	struct isp_pm_ioctl_input awb_input = { NULL, 0 };
	struct isp_pm_ioctl_output awb_output = { NULL, 0 };
	struct awb_data_info awb_data_ptr = { NULL, 0 };
	UNUSED(param_ptr);

	ISP_LOGV("E");

	if (NULL == isp_alg_handle || NULL == cxt->sn_cxt.sn_raw_info) {
		ISP_LOGE("fail to update param");
		ret = ISP_ERROR;
		return ret;
	}
	sensor_raw_info_ptr = (struct sensor_raw_info *)cxt->sn_cxt.sn_raw_info;
	if (sensor_raw_info_ptr == NULL) {
		ISP_LOGV("sensor_raw_info_ptr is  null");
	}

	version_info = (struct sensor_version_info *)sensor_raw_info_ptr->version_info;
	input.sensor_name = version_info->sensor_ver_name.sensor_name;

	if (NULL == cxt->handle_pm) {
		ISP_LOGE("fail to get valid param!");
		return ISP_PARAM_NULL;
	}

	/* set param source flag */
	param_source = ISP_PARAM_FROM_TOOL;
	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_PARAM_SOURCE, (void *)&param_source, NULL);

	input.num = MAX_MODE_NUM;
	for (i = 0; i < MAX_MODE_NUM; i++) {
		if (NULL != sensor_raw_info_ptr->mode_ptr[i].addr) {
			input.tuning_data[i].data_ptr = sensor_raw_info_ptr->mode_ptr[i].addr;
			input.tuning_data[i].size = sensor_raw_info_ptr->mode_ptr[i].len;
			input.fix_data[i] = sensor_raw_info_ptr->fix_ptr[i];
		} else {
			input.tuning_data[i].data_ptr = NULL;
			input.tuning_data[i].size = 0;
		}
	}
	input.nr_fix_info = &(sensor_raw_info_ptr->nr_fix);

	ret = isp_pm_update(cxt->handle_pm, ISP_PM_CMD_UPDATE_ALL_PARAMS, &input, PNULL);
	if (ISP_SUCCESS != ret) {
		ISP_LOGE("fail to update isp param");
		return ret;
	}
	param_source = 0;
	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_PARAM_SOURCE, (void *)&param_source, NULL);

	ret = ispctl_smart_param_update((cmr_handle) cxt);
	if (ISP_SUCCESS != ret) {
		ISP_LOGE("fail to update smart param");
		return ret;
	}
	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_INIT_AWB, &awb_input, &awb_output);
	if (ISP_SUCCESS == ret && awb_output.param_num) {
		awb_data_ptr.data_ptr = (void *)awb_output.param_data->data_ptr;
		awb_data_ptr.data_size = awb_output.param_data->data_size;
		if (cxt->ops.awb_ops.ioctrl)
			cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_SET_UPDATE_TUNING_PARAM, (void *)&awb_data_ptr, NULL);
	}

	{
		struct isp_pm_ioctl_input input = { NULL, 0 };
		struct isp_pm_ioctl_output output = { NULL, 0 };

		ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_INIT_AE, &input, &output);
		if (ISP_SUCCESS == ret && output.param_num) {
			cmr_s32 bypass = 0;
			cmr_u32 target_lum = 0;
			cmr_u32 *target_lum_ptr = NULL;

			bypass = output.param_data->user_data[0];
			target_lum_ptr = (cmr_u32 *) output.param_data->data_ptr;
			target_lum = target_lum_ptr[3];
			if (cxt->ops.ae_ops.ioctrl) {
				cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_BYPASS, &bypass, NULL);
				cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_TARGET_LUM, &target_lum, NULL);
			}
		}

		if (ISP_SUCCESS == ret) {
			if (cxt->ops.ae_ops.ioctrl)
				cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_FLASH_ON_OFF_THR, (void *)&cxt->pm_flash_info->cur.auto_flash_thr, NULL);
		}
		ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_INIT_AF, &input, &output);
		if (ISP_SUCCESS == ret && output.param_num) {
			cmr_s32 bypass = 0;
			bypass = output.param_data->user_data[0];
			if (cxt->ops.af_ops.ioctrl)
				cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_SET_AF_BYPASS, (void *)&bypass, NULL);
		}
	}
	return ret;
}

static cmr_int ispctl_get_ad_gain_exp_info(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_adgain_exp_info *info_ptr = (struct isp_adgain_exp_info *)param_ptr;
	cmr_s32 gain = 0;
	cmr_u32 exp_time = 0;
	cmr_int bv = 0;

	if (cxt->ops.ae_ops.ioctrl) {
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_GAIN, NULL, (void *)&gain);
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_EXP_TIME, NULL, (void *)&exp_time);
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_BV_BY_LUM_NEW, NULL, (void *)&bv);
	}

	if (!ret) {
		info_ptr->adgain = (cmr_u32) gain;
		info_ptr->exp_time = exp_time;
		info_ptr->bv = bv;
	}
	ISP_LOGV("adgain = %d, exp = %d, bv = %d", info_ptr->adgain, info_ptr->exp_time, info_ptr->bv);
	return ret;
}

static cmr_int ispctl_3ndr_ioctrl(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_3dnr_ctrl_param *isp_3dnr = (struct isp_3dnr_ctrl_param *)param_ptr;
	struct ae_calc_out ae_result;

	if (NULL == isp_alg_handle || NULL == param_ptr) {
		ISP_LOGE("fail to get valid cxt=%p and param_ptr=%p", isp_alg_handle, param_ptr);
		return ISP_PARAM_NULL;
	}

	memset((void *)&ae_result, 0, sizeof(struct ae_calc_out));

	if (isp_3dnr->enable) {
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_PAUSE, NULL, (void *)&ae_result);
		if (cxt->ops.smart_ops.block_disable)
			cxt->ops.smart_ops.block_disable(cxt->smart_cxt.handle, ISP_SMART_LNC);
		if (cxt->ops.smart_ops.block_disable)
			cxt->ops.smart_ops.block_disable(cxt->smart_cxt.handle, ISP_SMART_CMC);
		if (cxt->ops.smart_ops.block_disable)
			cxt->ops.smart_ops.block_disable(cxt->smart_cxt.handle, ISP_SMART_GAMMA);
		if (cxt->ops.smart_ops.NR_disable)
			cxt->ops.smart_ops.NR_disable(cxt->smart_cxt.handle, 1);
		if (cxt->ops.awb_ops.ioctrl)
			ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_LOCK, NULL, NULL);
		if (cxt->ops.lsc_ops.ioctrl)
			ret = cxt->ops.lsc_ops.ioctrl(cxt->lsc_cxt.handle, SMART_LSC_ALG_LOCK, NULL, NULL);
	} else {
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_RESTORE, NULL, (void *)&ae_result);
		if (cxt->ops.smart_ops.block_enable)
			cxt->ops.smart_ops.block_enable(cxt->smart_cxt.handle, ISP_SMART_LNC);
		if (cxt->ops.smart_ops.block_enable)
			cxt->ops.smart_ops.block_enable(cxt->smart_cxt.handle, ISP_SMART_CMC);
		if (cxt->ops.smart_ops.block_enable)
			cxt->ops.smart_ops.block_enable(cxt->smart_cxt.handle, ISP_SMART_GAMMA);
		if (cxt->ops.smart_ops.NR_disable)
			cxt->ops.smart_ops.NR_disable(cxt->smart_cxt.handle, 0);
		if (cxt->ops.awb_ops.ioctrl)
			ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_UNLOCK, NULL, NULL);
		if (cxt->ops.lsc_ops.ioctrl)
			ret = cxt->ops.lsc_ops.ioctrl(cxt->lsc_cxt.handle, SMART_LSC_ALG_UNLOCK, NULL, NULL);
	}

	return ISP_SUCCESS;
}

static cmr_int ispctl_param_update(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_pm_init_input input;
	cmr_u32 param_source = 0;
	struct isp_mode_param *mode_param_ptr = param_ptr;
	cmr_u32 i;
	struct isp_pm_ioctl_input awb_input = { NULL, 0 };
	struct isp_pm_ioctl_output awb_output = { NULL, 0 };
	struct awb_data_info awb_data_ptr = { NULL, 0 };

	ISP_LOGV("--IOCtrl--PARAM_UPDATE--");

	if (NULL == mode_param_ptr) {
		ISP_LOGE("fail to get valid param!");
		return ISP_PARAM_NULL;
	}

	if (NULL == cxt || NULL == cxt->handle_pm) {
		ISP_LOGE("fail to get valid param!");
		return ISP_PARAM_NULL;
	}

	param_source = ISP_PARAM_FROM_TOOL;
	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_PARAM_SOURCE, (void *)&param_source, NULL);

	input.num = MAX_MODE_NUM;
	for (i = 0; i < MAX_MODE_NUM; i++) {
		if (mode_param_ptr->mode_id == i) {
			input.tuning_data[i].data_ptr = mode_param_ptr;
			input.tuning_data[i].size = mode_param_ptr->size;
		} else {
			input.tuning_data[i].data_ptr = NULL;
			input.tuning_data[i].size = 0;
		}
		mode_param_ptr = (struct isp_mode_param *)((cmr_u8 *) mode_param_ptr + mode_param_ptr->size);
	}

	ret = isp_pm_update(cxt->handle_pm, ISP_PM_CMD_UPDATE_ALL_PARAMS, &input, NULL);
	if (ISP_SUCCESS != ret) {
		ISP_LOGE("fail to update  isp param update ");
		return ret;
	}
	param_source = 0;
	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_PARAM_SOURCE, (void *)&param_source, NULL);

	ret = ispctl_smart_param_update((cmr_handle) cxt);
	if (ISP_SUCCESS != ret) {
		ISP_LOGE("fail to update smart param update ");
		return ret;
	}

	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_INIT_AWB, &awb_input, &awb_output);
	if (ISP_SUCCESS == ret && awb_output.param_num) {
		awb_data_ptr.data_ptr = (void *)awb_output.param_data->data_ptr;
		awb_data_ptr.data_size = awb_output.param_data->data_size;
		if (cxt->ops.awb_ops.ioctrl)
			cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_SET_UPDATE_TUNING_PARAM, (void *)&awb_data_ptr, NULL);
	}

	{
		struct isp_pm_ioctl_input input = { NULL, 0 };
		struct isp_pm_ioctl_output output = { NULL, 0 };

		ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_GET_INIT_AE, &input, &output);
		if (ISP_SUCCESS == ret && output.param_num) {
			cmr_s32 bypass = 0;

			bypass = output.param_data->user_data[0];
			if (cxt->ops.ae_ops.ioctrl)
				cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_BYPASS, &bypass, NULL);
		}
	}
	return ret;
}

static cmr_int ispctl_ae_touch(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	cmr_s32 out_param = 0;
	struct isp_pos_rect *rect = NULL;
	struct ae_set_tuoch_zone touch_zone;

	if (NULL == param_ptr) {
		ISP_LOGE("fail to get valid param");
		return ISP_PARAM_NULL;
	}

	memset(&touch_zone, 0, sizeof(touch_zone));
	rect = (struct isp_pos_rect *)param_ptr;
	touch_zone.touch_zone.x = rect->start_x;
	touch_zone.touch_zone.y = rect->start_y;
	touch_zone.touch_zone.w = rect->end_x - rect->start_x + 1;
	touch_zone.touch_zone.h = rect->end_y - rect->start_y + 1;
	if (cxt->ops.ae_ops.ioctrl)
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_TOUCH_ZONE, &touch_zone, &out_param);

	if(touch_zone.touch_zone.w != 1 || touch_zone.touch_zone.h != 1) {
		if (cxt->ops.lsc_ops.ioctrl)
			ret = cxt->ops.lsc_ops.ioctrl(cxt->lsc_cxt.handle, ALSC_GET_TOUCH, NULL, NULL);
	}
	ISP_LOGV("w,h=(%d,%d)", touch_zone.touch_zone.w, touch_zone.touch_zone.h);

	return ret;
}

static cmr_int ispctl_af_mode(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	cmr_u32 set_mode;

	switch (*(cmr_u32 *) param_ptr) {
	case ISP_FOCUS_TRIG:
		set_mode = AF_MODE_NORMAL;
		break;
	case ISP_FOCUS_MACRO:
		set_mode = AF_MODE_MACRO;
		break;
	case ISP_FOCUS_CONTINUE:
		set_mode = AF_MODE_CONTINUE;
		break;
	case ISP_FOCUS_VIDEO:
		set_mode = AF_MODE_VIDEO;
		break;
	case ISP_FOCUS_MANUAL:
		set_mode = AF_MODE_MANUAL;
		break;
	case ISP_FOCUS_PICTURE:
		set_mode = AF_MODE_PICTURE;
		break;
	case ISP_FOCUS_FULLSCAN:
		set_mode = AF_MODE_FULLSCAN;
		break;
	default:
		set_mode = AF_MODE_NORMAL;
		break;
	}

	if (cxt->ops.af_ops.ioctrl)
		ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_SET_AF_MODE, (void *)&set_mode, NULL);

	return ret;
}

static cmr_int ispctl_get_af_mode(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	cmr_u32 param = 0;

	if (cxt->ops.af_ops.ioctrl)
		ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_GET_AF_MODE, (void *)&param, NULL);

	switch (param) {
	case AF_MODE_NORMAL:
		*(cmr_u32 *) param_ptr = ISP_FOCUS_TRIG;
		break;
	case AF_MODE_MACRO:
		*(cmr_u32 *) param_ptr = ISP_FOCUS_MACRO;
		break;
	case AF_MODE_CONTINUE:
		*(cmr_u32 *) param_ptr = ISP_FOCUS_CONTINUE;
		break;
	case AF_MODE_MANUAL:
		*(cmr_u32 *) param_ptr = ISP_FOCUS_MANUAL;
		break;
	case AF_MODE_VIDEO:
		*(cmr_u32 *) param_ptr = ISP_FOCUS_VIDEO;
		break;
	default:
		*(cmr_u32 *) param_ptr = ISP_FOCUS_TRIG;
		break;
	}

	return ret;
}

static cmr_int ispctl_af_info(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_af_ctrl *af_ctrl_ptr = (struct isp_af_ctrl *)param_ptr;
	struct isp_u_blocks_info afm_block_info;
	cmr_u32 isp_tool_af_test;

	memset(&afm_block_info, 0x0, sizeof(afm_block_info));
	if (ISP_CTRL_SET == af_ctrl_ptr->mode) {
		isp_tool_af_test = 1;
		if (cxt->ops.af_ops.ioctrl)
			ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_SET_ISP_TOOL_AF_TEST, &isp_tool_af_test, NULL);
		afm_block_info.afm_info.bypass = 0;
		afm_block_info.afm_info.skip_num = 0;
		afm_block_info.afm_info.cur_envi = INDOOR_SCENE;
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_SET_AF_MONITOR, (void *)&afm_block_info, NULL);
		if (cxt->ops.af_ops.ioctrl) {
			ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_SET_DEFAULT_AF_WIN, NULL, NULL);
			ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_SET_AF_POS, (void *)&af_ctrl_ptr->step, NULL);
		}
	} else if (ISP_CTRL_GET == af_ctrl_ptr->mode) {
		cmr_u32 cur_pos = 0;
		struct isp_af_statistic_info afm_stat;

		memset((void *)&afm_stat, 0, sizeof(afm_stat));
		if (cxt->ops.af_ops.ioctrl)
			ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_GET_AF_CUR_POS, (void *)&cur_pos, NULL);
		af_ctrl_ptr->step = cur_pos;
	} else {
		isp_tool_af_test = 0;
		afm_block_info.bypass = 1;
		if (cxt->ops.af_ops.ioctrl)
			ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_SET_ISP_TOOL_AF_TEST, (void *)&isp_tool_af_test, NULL);
		ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_RAW_AFM_BYPASS, &afm_block_info, NULL);
	}

	return ret;
}

static cmr_int ispctl_get_af_pos(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	if (cxt->ops.af_ops.ioctrl)
		ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_GET_AF_CUR_POS, param_ptr, NULL);

	return ret;
}

static cmr_int ispctl_set_af_pos(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	if (cxt->ops.af_ops.ioctrl)
		ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_SET_AF_POS, param_ptr, NULL);

	return ret;
}

static cmr_int ispctl_reg(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;

	UNUSED(isp_alg_handle);
	UNUSED(param_ptr);
	return ret;
}

static cmr_int ispctl_scaler_trim(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_trim_size *trim = (struct isp_trim_size *)param_ptr;

	if (NULL != trim) {
		struct ae_trim scaler;

		scaler.x = trim->x;
		scaler.y = trim->y;
		scaler.w = trim->w;
		scaler.h = trim->h;

		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_STAT_TRIM, &scaler, NULL);
	}

	return ret;
}

static cmr_int ispctl_face_area(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_face_area *face_area = (struct isp_face_area *)param_ptr;

	if (NULL != face_area) {
		struct ae_fd_param fd_param;
		cmr_s32 i;

		fd_param.width = face_area->frame_width;
		fd_param.height = face_area->frame_height;
		fd_param.face_num = face_area->face_num;
		for (i = 0; i < fd_param.face_num; ++i) {
			fd_param.face_area[i].rect.start_x = face_area->face_info[i].sx;
			fd_param.face_area[i].rect.start_y = face_area->face_info[i].sy;
			fd_param.face_area[i].rect.end_x = face_area->face_info[i].ex;
			fd_param.face_area[i].rect.end_y = face_area->face_info[i].ey;
			fd_param.face_area[i].face_lum = face_area->face_info[i].brightness;
			fd_param.face_area[i].pose = face_area->face_info[i].pose;
		}
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_FD_PARAM, &fd_param, NULL);
	}

	return ret;
}

static cmr_int ispctl_af_face_area(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_face_area *face_area = (struct isp_face_area *)param_ptr;

	if (NULL != face_area) {
		struct ae_fd_param fd_param;
		cmr_s32 i;

		fd_param.width = face_area->frame_width;
		fd_param.height = face_area->frame_height;
		fd_param.face_num = face_area->face_num;
		for (i = 0; i < fd_param.face_num; ++i) {
			fd_param.face_area[i].rect.start_x = face_area->face_info[i].sx;
			fd_param.face_area[i].rect.start_y = face_area->face_info[i].sy;
			fd_param.face_area[i].rect.end_x = face_area->face_info[i].ex;
			fd_param.face_area[i].rect.end_y = face_area->face_info[i].ey;
			fd_param.face_area[i].face_lum = face_area->face_info[i].brightness;
			fd_param.face_area[i].pose = face_area->face_info[i].pose;
		}
		if (cxt->ops.af_ops.ioctrl)
			ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_SET_FACE_DETECT, (void *)param_ptr, NULL);
	}

	return ret;
}

static cmr_int ispctl_start_3a(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct ae_calc_out ae_result;
	cmr_u32 af_bypass = 0;
	UNUSED(param_ptr);

	memset((void *)&ae_result, 0, sizeof(struct ae_calc_out));

	if (cxt->ops.ae_ops.ioctrl)
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_FORCE_RESTORE, NULL, (void *)&ae_result);
	if (cxt->ops.awb_ops.ioctrl)
		ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_UNLOCK, NULL, NULL);
	if (cxt->ops.af_ops.ioctrl)
		ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_SET_AF_BYPASS, (void *)&af_bypass, NULL);

	ISP_LOGV("done");

	return ISP_SUCCESS;
}

static cmr_int ispctl_stop_3a(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct ae_calc_out ae_result;
	cmr_u32 af_bypass = 1;
	UNUSED(param_ptr);

	memset((void *)&ae_result, 0, sizeof(struct ae_calc_out));

	if (cxt->ops.ae_ops.ioctrl)
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_FORCE_PAUSE, NULL, (void *)&ae_result);
	if (cxt->ops.awb_ops.ioctrl)
		ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_LOCK, NULL, NULL);
	if (cxt->ops.af_ops.ioctrl)
		ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_SET_AF_BYPASS, (void *)&af_bypass, NULL);

	ISP_LOGV("done");

	return ISP_SUCCESS;
}

static cmr_int ispctl_hdr(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isp_hdr_param *isp_hdr = (struct isp_hdr_param *)param_ptr;
	struct ae_hdr_param ae_hdr = { 0x00, 0x00 };
	cmr_s16 smart_block_eb[ISP_SMART_MAX_BLOCK_NUM];

	if (NULL == isp_alg_handle || NULL == param_ptr) {
		ISP_LOGE("fail to get valid cxt=%p and param_ptr=%p", isp_alg_handle, param_ptr);
		return ISP_PARAM_NULL;
	}

	memset(&smart_block_eb, 0x00, sizeof(smart_block_eb));

	ae_hdr.hdr_enable = isp_hdr->hdr_enable;
	ae_hdr.ev_effect_valid_num = isp_hdr->ev_effect_valid_num;

	if (cxt->ops.ae_ops.ioctrl)
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_HDR_START, &ae_hdr, NULL);
	if (ae_hdr.hdr_enable) {
		if (cxt->ops.smart_ops.block_disable)
			cxt->ops.smart_ops.block_disable(cxt->smart_cxt.handle, ISP_SMART_LNC);
		if (cxt->ops.smart_ops.block_disable)
			cxt->ops.smart_ops.block_disable(cxt->smart_cxt.handle, ISP_SMART_CMC);
		if (cxt->ops.smart_ops.block_disable)
			cxt->ops.smart_ops.block_disable(cxt->smart_cxt.handle, ISP_SMART_GAMMA);
		if (cxt->ops.smart_ops.NR_disable)
			cxt->ops.smart_ops.NR_disable(cxt->smart_cxt.handle, 1);
		if (cxt->ops.awb_ops.ioctrl)
			ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_LOCK, NULL, NULL);
		if (cxt->ops.lsc_ops.ioctrl)
			ret = cxt->ops.lsc_ops.ioctrl(cxt->lsc_cxt.handle, SMART_LSC_ALG_LOCK, NULL, NULL);
	} else {
		if (cxt->ops.smart_ops.block_enable)
			cxt->ops.smart_ops.block_enable(cxt->smart_cxt.handle, ISP_SMART_LNC);
		if (cxt->ops.smart_ops.block_enable)
			cxt->ops.smart_ops.block_enable(cxt->smart_cxt.handle, ISP_SMART_CMC);
		if (cxt->ops.smart_ops.block_enable)
			cxt->ops.smart_ops.block_enable(cxt->smart_cxt.handle, ISP_SMART_GAMMA);
		if (cxt->ops.smart_ops.NR_disable)
			cxt->ops.smart_ops.NR_disable(cxt->smart_cxt.handle, 0);
		if (cxt->ops.awb_ops.ioctrl)
			ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_UNLOCK, NULL, NULL);
		if (cxt->ops.lsc_ops.ioctrl)
			ret = cxt->ops.lsc_ops.ioctrl(cxt->lsc_cxt.handle, SMART_LSC_ALG_UNLOCK, NULL, NULL);
	}

	return ISP_SUCCESS;
}

static cmr_int ispctl_set_ae_night_mode(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	cmr_u32 night_mode = 0;

	if (NULL == param_ptr) {
		return ISP_PARAM_NULL;
	}

	night_mode = *(cmr_u32 *) param_ptr;
	if (cxt->ops.ae_ops.ioctrl)
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_NIGHT_MODE, &night_mode, NULL);

	ISP_LOGV("ISP_AE: AE_SET_NIGHT_MODE=%d, ret=%ld", night_mode, ret);

	return ret;
}

static cmr_int ispctl_set_ae_awb_lock_unlock(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct ae_calc_out ae_result;
	cmr_u32 ae_awb_mode = 0;

	if (NULL == param_ptr) {
		return ISP_PARAM_NULL;
	}
	memset((void *)&ae_result, 0, sizeof(struct ae_calc_out));

	ae_awb_mode = *(cmr_u32 *) param_ptr;
	if (ISP_AWB_LOCK == ae_awb_mode) {
		ISP_LOGV("AWB Lock");
		if (cxt->ops.awb_ops.ioctrl)
			ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_LOCK, NULL, NULL);
	} else if (ISP_AWB_UNLOCK == ae_awb_mode) {
		ISP_LOGV("AWB UnLock");
		if (cxt->ops.awb_ops.ioctrl)
			ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_UNLOCK, NULL, NULL);
	} else if (ISP_AE_AWB_LOCK == ae_awb_mode) {	// AE & AWB Lock
		ISP_LOGV("AE & AWB Lock");
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_PAUSE, NULL, (void *)&ae_result);
		if (cxt->ops.awb_ops.ioctrl)
			ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_LOCK, NULL, NULL);
	} else if (ISP_AE_AWB_UNLOCK == ae_awb_mode) {	// AE & AWB Unlock
		ISP_LOGV("AE & AWB Un-Lock\n");
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_RESTORE, NULL, (void *)&ae_result);
		if (cxt->ops.awb_ops.ioctrl)
			ret = cxt->ops.awb_ops.ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_UNLOCK, NULL, NULL);
	} else {
		ISP_LOGV("Unsupported AE & AWB mode (%d)\n", ae_awb_mode);
	}

	return ISP_SUCCESS;
}

static cmr_int ispctl_set_ae_lock_unlock(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct ae_calc_out ae_result;
	cmr_u32 ae_mode;

	if (NULL == param_ptr) {
		return ISP_PARAM_NULL;
	}
	memset((void *)&ae_result, 0, sizeof(struct ae_calc_out));

	ae_mode = *(cmr_u32 *) param_ptr;
	if (ISP_AE_LOCK == ae_mode) {	// AE & AWB Lock
		ISP_LOGV("AE Lock\n");
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_PAUSE, NULL, (void *)&ae_result);
	} else if (ISP_AE_UNLOCK == ae_mode) {	// AE  Unlock
		ISP_LOGV("AE Un-Lock\n");
		if (cxt->ops.ae_ops.ioctrl)
			ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_RESTORE, NULL, (void *)&ae_result);
	} else {
		ISP_LOGV("Unsupported AE  mode (%d)\n", ae_mode);
	}

	return ISP_SUCCESS;
}

static cmr_int ispctl_denoise_param_read(cmr_handle isp_alg_handle, void *param_ptr)
{
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct sensor_raw_info *raw_sensor_ptr = cxt->sn_cxt.sn_raw_info;
	struct isp_mode_param *mode_common_ptr = (struct isp_mode_param *)raw_sensor_ptr->mode_ptr[0].addr;
	struct denoise_param_update *update_param = (struct denoise_param_update *)param_ptr;
	cmr_u32 i;
	struct sensor_raw_fix_info *fix_data_ptr = PNULL;
	struct sensor_nr_fix_info *nr_fix = PNULL;
	fix_data_ptr = raw_sensor_ptr->fix_ptr[0];
	nr_fix = &raw_sensor_ptr->nr_fix;

	if (NULL == param_ptr) {
		return ISP_PARAM_NULL;
	}

	update_param->multi_nr_flag = SENSOR_MULTI_MODE_FLAG;
	update_param->nr_scene_map_ptr = nr_fix->nr_scene_ptr;
	update_param->nr_level_number_map_ptr = nr_fix->nr_level_number_ptr;
	update_param->nr_default_level_map_ptr = nr_fix->nr_default_level_ptr;
	if (update_param->nr_level_number_map_ptr) {
		ISP_LOGV("ISP_TOOL:update_param->nr_level_number_map_ptr sizeof = %zd",
			sizeof(update_param->nr_level_number_map_ptr));
	} else {
		ISP_LOGV("ISP_TOOL: nr map is null");
	}

	for (i = 0; i < mode_common_ptr->block_num; i++) {
		struct isp_block_header *header = &(mode_common_ptr->block_header[i]);

		switch (header->block_id) {
		case ISP_BLK_PDAF_CORRECT:
				update_param->pdaf_correction_level_ptr = (struct sensor_pdaf_correction_level *)fix_data_ptr->nr.nr_set_group.pdaf_correct;	//0x14
				break;
		case ISP_BLK_BPC:
				update_param->bpc_level_ptr = (struct sensor_bpc_level *)fix_data_ptr->nr.nr_set_group.bpc;	//0x19
				break;
		case ISP_BLK_GRGB:
				update_param->grgb_level_ptr = (struct sensor_grgb_level *)fix_data_ptr->nr.nr_set_group.grgb;	//0x1A
				break;
		case ISP_BLK_NLM:
				update_param->nlm_level_ptr = (struct sensor_nlm_level *)fix_data_ptr->nr.nr_set_group.nlm;	//0x15
				update_param->vst_level_ptr = (struct sensor_vst_level *)fix_data_ptr->nr.nr_set_group.vst;	//0x16
				update_param->ivst_level_ptr = (struct sensor_ivst_level *)fix_data_ptr->nr.nr_set_group.ivst;	//0x17
				break;
		case ISP_BLK_RGB_DITHER:
				update_param->rgb_dither_level_ptr = (struct sensor_rgb_dither_level *)fix_data_ptr->nr.nr_set_group.rgb_dither;	//0x18
				break;
		case ISP_BLK_CFA:
				update_param->cfae_level_ptr = (struct sensor_cfai_level *)fix_data_ptr->nr.nr_set_group.cfa;	//0x1B
				break;
		case ISP_BLK_RGB_AFM:
				update_param->rgb_afm_level_ptr = (struct sensor_rgb_afm_level *)fix_data_ptr->nr.nr_set_group.rgb_afm;	//0x1C
				break;
		case ISP_BLK_UVDIV:
				update_param->cce_uvdiv_level_ptr = (struct sensor_cce_uvdiv_level *)fix_data_ptr->nr.nr_set_group.uvdiv;	//0x1D
				break;
		case ISP_BLK_3DNR_PRE:
				update_param->dnr_pre_level_ptr = (struct sensor_3dnr_level *)fix_data_ptr->nr.nr_set_group.nr3d_pre;	//0x1E
				break;
		case ISP_BLK_3DNR_CAP:
				update_param->dnr_cap_level_ptr = (struct sensor_3dnr_level *)fix_data_ptr->nr.nr_set_group.nr3d_cap;	//0x1F
				break;
		case ISP_BLK_EDGE:
				update_param->ee_level_ptr = (struct sensor_ee_level *)fix_data_ptr->nr.nr_set_group.edge;	//0x20
				break;
		case ISP_BLK_YUV_PRECDN:
				update_param->yuv_precdn_level_ptr = (struct sensor_yuv_precdn_level *)fix_data_ptr->nr.nr_set_group.yuv_precdn;	//0x21
				break;
		case ISP_BLK_YNR:
				update_param->ynr_level_ptr = (struct sensor_ynr_level *)fix_data_ptr->nr.nr_set_group.ynr;	//0x22
				break;
		case ISP_BLK_UV_CDN:
				update_param->uv_cdn_level_ptr = (struct sensor_uv_cdn_level *)fix_data_ptr->nr.nr_set_group.cdn;	//0x23
				break;
		case ISP_BLK_UV_POSTCDN:
				update_param->uv_postcdn_level_ptr = (struct sensor_uv_postcdn_level *)fix_data_ptr->nr.nr_set_group.postcdn;	//0x24
				break;
		case ISP_BLK_IIRCNR_IIR:
				update_param->iircnr_level_ptr = (struct sensor_iircnr_level *)fix_data_ptr->nr.nr_set_group.iircnr;	//0x25
				break;
		case ISP_BLK_YUV_NOISEFILTER:
				update_param->yuv_noisefilter_level_ptr = (struct sensor_yuv_noisefilter_level *)fix_data_ptr->nr.nr_set_group.yuv_noisefilter;	//0x26
				break;
		default:
			break;
		}
	}

	return ISP_SUCCESS;
}

static cmr_int ispctl_sensor_denoise_param_update(cmr_handle isp_alg_handle, void *param_ptr)
{
	UNUSED(isp_alg_handle);

	if (NULL == param_ptr) {
		return ISP_PARAM_NULL;
	}
	ISP_LOGV(" done");
	return ISP_SUCCESS;
}

static cmr_int ispctl_dump_reg(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	UNUSED(param_ptr);
	UNUSED(isp_alg_handle);

	return ret;
}

static cmr_int ispctl_tool_set_scene_param(cmr_handle isp_alg_handle, void *param_ptr)
{
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct isptool_scene_param *scene_parm = NULL;
	struct isp_pm_ioctl_input ioctl_input;
	struct isp_pm_param_data ioctl_data[ISP_MODE_MAX];
	struct isp_awbc_cfg awbc_cfg;
	struct smart_proc_input smart_proc_in;
	cmr_s32 i;
	cmr_u32 ret = ISP_SUCCESS;

	memset((void *)&smart_proc_in, 0, sizeof(struct smart_proc_input));
	memset(ioctl_data, 0x0, sizeof(ioctl_data));

	cxt->takepicture_mode = CAMERA_ISP_SIMULATION_MODE;
	scene_parm = (struct isptool_scene_param *)param_ptr;
	/*set awb gain */
	awbc_cfg.r_gain = scene_parm->awb_gain_r;
	awbc_cfg.g_gain = scene_parm->awb_gain_g;
	awbc_cfg.b_gain = scene_parm->awb_gain_b;
	awbc_cfg.r_offset = 0;
	awbc_cfg.g_offset = 0;
	awbc_cfg.b_offset = 0;

	if (!awbc_cfg.r_gain && !awbc_cfg.g_gain && !awbc_cfg.b_gain) {
		awbc_cfg.r_gain = 1800;
		awbc_cfg.g_gain = 1024;
		awbc_cfg.b_gain = 1536;
	}

	if (cxt->zsl_flag) {
		for (i = 0; i < ISP_MODE_MAX; i++) {
			BLOCK_PARAM_CFG(ioctl_data[i], ISP_PM_BLK_AWBC,
					ISP_BLK_AWB_NEW,
					cxt->mode_id[i],
					&awbc_cfg,
					sizeof(awbc_cfg));
		}
		ioctl_input.param_num = ISP_MODE_MAX;
	} else {
		BLOCK_PARAM_CFG(ioctl_data[0], ISP_PM_BLK_AWBC,
				ISP_BLK_AWB_NEW,
				cxt->mode_id[0],
				&awbc_cfg,
				sizeof(awbc_cfg));
		ioctl_input.param_num = 1;
	}

	ioctl_input.param_data_ptr = ioctl_data;
	ret = isp_pm_ioctl(cxt->handle_pm, ISP_PM_CMD_SET_AWB, (void *)&ioctl_input, NULL);
	if (ISP_SUCCESS != ret) {
		ISP_LOGE("fail to set awb gain ");
		return ret;
	}

	smart_proc_in.lsc_sprd_version = cxt->lsc_cxt.lsc_sprd_version;
	smart_proc_in.cal_para.gamma_tab = cxt->smart_cxt.tunning_gamma_cur;
	smart_proc_in.cal_para.bv = scene_parm->smart_bv;
	smart_proc_in.cal_para.bv_gain = scene_parm->gain;
	smart_proc_in.cal_para.ct = scene_parm->smart_ct;
	smart_proc_in.alc_awb = cxt->awb_cxt.alc_awb;
	smart_proc_in.handle_pm = cxt->handle_pm;
	smart_proc_in.mode_flag = cxt->commn_cxt.mode_flag;
	if (cxt->ops.smart_ops.calc)
		ret = cxt->ops.smart_ops.calc(cxt->smart_cxt.handle, &smart_proc_in);
	if (ISP_SUCCESS != ret) {
		ISP_LOGE("fail to set smart gain");
		return ret;
	}

	return ret;
}

static cmr_int ispctl_force_ae_quick_mode(cmr_handle isp_alg_handle, void *param_ptr)
{
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 force_quick_mode = *(cmr_u32 *) param_ptr;

	if (cxt->ops.ae_ops.ioctrl)
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_FORCE_QUICK_MODE, (void *)&force_quick_mode, NULL);
	return ret;
}

static cmr_int ispctl_set_ae_exp_time(cmr_handle isp_alg_handle, void *param_ptr)
{
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 exp_time = *(cmr_u32 *) param_ptr;

	if (cxt->ops.ae_ops.ioctrl)
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_EXP_TIME, (void *)&exp_time, NULL);
	return ret;
}

static cmr_int ispctl_set_ae_sensitivity(cmr_handle isp_alg_handle, void *param_ptr)
{
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 sensitivity = *(cmr_u32 *) param_ptr;

	if (cxt->ops.ae_ops.ioctrl)
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_SENSITIVITY, (void *)&sensitivity, NULL);
	return ret;
}

static cmr_int ispctl_set_dcam_timestamp(cmr_handle isp_alg_handle, void *param_ptr)
{
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	cmr_int ret = ISP_SUCCESS;
	struct isp_af_ts *af_ts = (struct isp_af_ts *)param_ptr;

	if (cxt->ops.af_ops.ioctrl)
		ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_SET_DCAM_TIMESTAMP, (void *)af_ts, NULL);
	return ret;
}

static cmr_int ispctl_set_aux_sensor_info(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	if (0 != cxt->commn_cxt.aux_sensor_skip_num++ % 3)
		goto exit;
	if (3 <= cxt->commn_cxt.aux_sensor_skip_num)
		cxt->commn_cxt.aux_sensor_skip_num = 0;

	if (cxt->ops.af_ops.ioctrl)
		ret = cxt->ops.af_ops.ioctrl(cxt->af_cxt.handle, AF_CMD_SET_UPDATE_AUX_SENSOR, (void *)param_ptr, NULL);

	if (cxt->ops.ae_ops.ioctrl) {
		struct ae_aux_sensor_info ae_sensor_info;
		struct af_aux_sensor_info_t *aux_sensor_info = (struct af_aux_sensor_info_t *)param_ptr;

		memset((void*)&ae_sensor_info, 0, sizeof(struct ae_aux_sensor_info));

		switch (aux_sensor_info->type) {
		case AF_ACCELEROMETER:
			ae_sensor_info.gsensor_info.vertical_up = aux_sensor_info->gsensor_info.vertical_up;
			ae_sensor_info.gsensor_info.vertical_down = aux_sensor_info->gsensor_info.vertical_down;
			ae_sensor_info.gsensor_info.horizontal = aux_sensor_info->gsensor_info.horizontal;
			ae_sensor_info.gsensor_info.timestamp = aux_sensor_info->gsensor_info.timestamp;
			ae_sensor_info.gsensor_info.valid = 1;
			ae_sensor_info.type = AE_ACCELEROMETER;
			break;
		case AF_MAGNETIC_FIELD:
			break;
		case AF_GYROSCOPE:
			ae_sensor_info.gyro_info.timestamp = aux_sensor_info->gyro_info.timestamp;
			ae_sensor_info.gyro_info.x = aux_sensor_info->gyro_info.x;
			ae_sensor_info.gyro_info.y = aux_sensor_info->gyro_info.y;
			ae_sensor_info.gyro_info.z = aux_sensor_info->gyro_info.z;
			ae_sensor_info.gyro_info.valid = 1;
			ae_sensor_info.type = AE_GYROSCOPE;
			break;
		case AF_LIGHT:
			break;
		case AF_PROXIMITY:
			break;
		default:
			ISP_LOGI("sensor type not support");
			break;
		}
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_SET_UPDATE_AUX_SENSOR, (void*)&ae_sensor_info, NULL);
	}
exit:
	return ret;
}

static cmr_int ispctl_get_fps(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	cmr_u32 param = 0;

	if (NULL == param_ptr) {
		ISP_LOGE("fail to get valid param !");
		return ISP_PARAM_NULL;
	}
	if (cxt->ops.ae_ops.ioctrl)
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_FPS, NULL, (void *)&param);
	*(cmr_u32 *) param_ptr = param;

	ISP_LOGV("ret %ld param %d fps %d", ret, param, *(cmr_u32 *) param_ptr);

	return ret;
}

static cmr_int ispctl_get_leds_ctrl(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;
	struct ae_leds_ctrl *leds_ctrl = (struct ae_leds_ctrl *)param_ptr;

	if (NULL == param_ptr) {
		ISP_LOGE("fail to get valid param !");
		return ISP_PARAM_NULL;
	}
	if (cxt->ops.ae_ops.ioctrl)
		ret = cxt->ops.ae_ops.ioctrl(cxt->ae_cxt.handle, AE_GET_LEDS_CTRL, NULL, (void *)leds_ctrl);

	ISP_LOGV("ret %ld led0_en=%d led1_en=%d", ret, leds_ctrl->led0_ctrl, leds_ctrl->led1_ctrl);
	return ret;
}

static cmr_int ispctl_post_3dnr(cmr_handle isp_alg_handle, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_alg_fw_context *cxt = (struct isp_alg_fw_context *)isp_alg_handle;

	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_POST_3DNR, (void *)param_ptr, NULL);

	return ret;
}

static struct isp_io_ctrl_fun s_isp_io_ctrl_fun_tab[] = {
	{IST_CTRL_SNAPSHOT_NOTICE, ispctl_snapshot_notice},
	{ISP_CTRL_AE_MEASURE_LUM, ispctl_ae_measure_lum},
	{ISP_CTRL_EV, ispctl_ev},
	{ISP_CTRL_FLICKER, ispctl_flicker},
	{ISP_CTRL_ISO, ispctl_iso},
	{ISP_CTRL_AE_TOUCH, ispctl_ae_touch},
	{ISP_CTRL_FLASH_NOTICE, ispctl_flash_notice},
	{ISP_CTRL_VIDEO_MODE, ispctl_video_mode},
	{ISP_CTRL_SCALER_TRIM, ispctl_scaler_trim},
	{ISP_CTRL_RANGE_FPS, ispctl_range_fps},
	{ISP_CTRL_FACE_AREA, ispctl_face_area},
	{ISP_CTRL_AF_FACE_AREA, ispctl_af_face_area},

	{ISP_CTRL_AEAWB_BYPASS, ispctl_ae_awb_bypass},
	{ISP_CTRL_AWB_MODE, ispctl_awb_mode},

	{ISP_CTRL_AF, ispctl_af},
	{ISP_CTRL_GET_FULLSCAN_INFO, ispctl_af_get_full_scan_info},
	{ISP_CTRL_SET_AF_BYPASS, ispctl_af_bypass},
	{ISP_CTRL_BURST_NOTICE, ispctl_burst_notice},
	{ISP_CTRL_SFT_READ, ispctl_sfti_read},
	{ISP_CTRL_SFT_WRITE, ispctl_sfti_write},
	{ISP_CTRL_SFT_SET_PASS, ispctl_sfti_set_pass},	// added for sft
	{ISP_CTRL_SFT_SET_BYPASS, ispctl_sfti_set_bypass},	// added for sft
	{ISP_CTRL_SFT_GET_AF_VALUE, ispctl_sfti_get_af_value},	// added for sft
	{ISP_CTRL_AF_MODE, ispctl_af_mode},
	{ISP_CTRL_AF_STOP, ispctl_af_stop},
	{ISP_CTRL_FLASH_CTRL, ispctl_online_flash},

	{ISP_CTRL_SCENE_MODE, ispctl_scene_mode},
	{ISP_CTRL_SPECIAL_EFFECT, ispctl_special_effect},
	{ISP_CTRL_BRIGHTNESS, ispctl_brightness},
	{ISP_CTRL_CONTRAST, ispctl_contrast},
	{ISP_CTRL_SATURATION, ispctl_saturation},
	{ISP_CTRL_SHARPNESS, ispctl_sharpness},
	{ISP_CTRL_HDR, ispctl_hdr},

	{ISP_CTRL_PARAM_UPDATE, ispctl_param_update},
	{ISP_CTRL_IFX_PARAM_UPDATE, ispctl_fix_param_update},
	{ISP_CTRL_GET_CUR_ADGAIN_EXP, ispctl_get_ad_gain_exp_info},
	{ISP_CTRL_AE_CTRL, ispctl_ae_online},	// for isp tool cali
	{ISP_CTRL_SET_LUM, ispctl_set_lum},	// for tool cali
	{ISP_CTRL_GET_LUM, ispctl_get_lum},	// for tool cali
	{ISP_CTRL_AF_CTRL, ispctl_af_info},	// for tool cali
	{ISP_CTRL_SET_AF_POS, ispctl_set_af_pos},	// for tool cali
	{ISP_CTRL_GET_AF_POS, ispctl_get_af_pos},	// for tool cali
	{ISP_CTRL_GET_AF_MODE, ispctl_get_af_mode},	// for tool cali
	{ISP_CTRL_REG_CTRL, ispctl_reg},	// for tool cali
	{ISP_CTRL_AF_END_INFO, ispctl_reg},	// for tool cali
	{ISP_CTRL_DENOISE_PARAM_READ, ispctl_denoise_param_read},	//for tool cali
	{ISP_CTRL_DUMP_REG, ispctl_dump_reg},	//for tool cali
	{ISP_CTRL_START_3A, ispctl_start_3a},
	{ISP_CTRL_STOP_3A, ispctl_stop_3a},

	{ISP_CTRL_AE_FORCE_CTRL, ispctl_ae_force},	// for mp tool cali
	{ISP_CTRL_GET_AE_STATE, ispctl_get_ae_state},	// for mp tool cali
	{ISP_CTRL_GET_AWB_GAIN, ispctl_get_awb_gain},	// for mp tool cali
	{ISP_CTRL_GET_AWB_CT, ispctl_awb_ct},
	{ISP_CTRL_SET_AE_FPS, ispctl_set_ae_fps},	//for LLS feature
	{ISP_CTRL_GET_INFO, ispctl_get_info},
	{ISP_CTRL_SET_AE_NIGHT_MODE, ispctl_set_ae_night_mode},
	{ISP_CTRL_SET_AE_AWB_LOCK_UNLOCK, ispctl_set_ae_awb_lock_unlock},	// AE & AWB Lock or Unlock
	{ISP_CTRL_SET_AE_LOCK_UNLOCK, ispctl_set_ae_lock_unlock},	//AE Lock or Unlock
	{ISP_CTRL_TOOL_SET_SCENE_PARAM, ispctl_tool_set_scene_param},	// for tool scene param setting
	{ISP_CTRL_FORCE_AE_QUICK_MODE, ispctl_force_ae_quick_mode},
	{ISP_CTRL_DENOISE_PARAM_UPDATE, ispctl_sensor_denoise_param_update},	//for tool cali
	{ISP_CTRL_SET_AE_EXP_TIME, ispctl_set_ae_exp_time},
	{ISP_CTRL_SET_AE_SENSITIVITY, ispctl_set_ae_sensitivity},
	{ISP_CTRL_SET_DCAM_TIMESTAMP, ispctl_set_dcam_timestamp},
	{ISP_CTRL_SET_AUX_SENSOR_INFO, ispctl_set_aux_sensor_info},
	{ISP_CTRL_GET_FPS, ispctl_get_fps},
	{ISP_CTRL_GET_LEDS_CTRL, ispctl_get_leds_ctrl},
	{ISP_CTRL_POST_3DNR, ispctl_post_3dnr},	//for 3dnr module
	{ISP_CTRL_3DNR, ispctl_3ndr_ioctrl},
	{ISP_CTRL_MAX, NULL}
};

static isp_io_fun isp_ioctl_get_fun(enum isp_ctrl_cmd cmd)
{
	isp_io_fun io_ctrl = NULL;
	cmr_u32 total_num = 0;
	cmr_u32 i = 0;

	total_num = sizeof(s_isp_io_ctrl_fun_tab) / sizeof(struct isp_io_ctrl_fun);
	for (i = 0; i < total_num; i++) {
		if (cmd == s_isp_io_ctrl_fun_tab[i].cmd) {
			io_ctrl = s_isp_io_ctrl_fun_tab[i].io_ctrl;
			break;
		}
	}

	return io_ctrl;
}
#endif
