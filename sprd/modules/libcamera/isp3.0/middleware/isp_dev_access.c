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
#define LOG_TAG "isp_dev_access"
#define ATRACE_TAG (ATRACE_TAG_CAMERA | ATRACE_TAG_HAL)

#include <stdlib.h>
#include <cutils/trace.h>
#include "cutils/properties.h"
#include <unistd.h>
#include "isp_common_types.h"
#include "isp_dev_access.h"
#include "isp_drv.h"

/**************************************** MACRO DEFINE *****************************************/
#define ISP_BRITNESS_GAIN_0            50
#define ISP_BRITNESS_GAIN_1            70
#define ISP_BRITNESS_GAIN_2            90
#define ISP_BRITNESS_GAIN_3            100
#define ISP_BRITNESS_GAIN_4            120
#define ISP_BRITNESS_GAIN_5            160
#define ISP_BRITNESS_GAIN_6            200
/************************************* INTERNAL DATA TYPE ***************************************/
struct iommu_mem {
	cmr_uint virt_addr;
	cmr_uint phy_addr;
	cmr_int mfd;
	cmr_u32 size;
	cmr_u32 num;
};

struct isp_dev_access_context {
	cmr_u32 camera_id;
	cmr_u32 is_inited;
	cmr_handle caller_handle;
	cmr_handle isp_driver_handle;
	struct isp_dev_init_in input_param;
	struct iommu_mem fw_buffer;
	cmr_u8 pdaf_supported;
	cmr_u32 idx;
};

/*************************************INTERNAK FUNCTION ***************************************/

/*************************************EXTERNAL FUNCTION ***************************************/
cmr_int isp_dev_access_init(struct isp_dev_init_in *input_ptr, cmr_handle *isp_dev_handle)
{
	ATRACE_BEGIN(__func__);

	cmr_int                                ret = ISP_SUCCESS;
	struct isp_dev_access_context          *cxt = NULL;
	struct isp_dev_init_info               input;
	struct isp_init_mem_param              load_input;
	cmr_int                                fw_size = 0;
	cmr_u32                                fw_buf_num = 1;
	cmr_int                                i = 0;

	if (!input_ptr || !isp_dev_handle) {
		ISP_LOGE("input is NULL, 0x%lx", (cmr_int)input_ptr);
		ret = ISP_PARAM_NULL;
		goto exit;
	}

	*isp_dev_handle = NULL;
	cxt = (struct isp_dev_access_context *)malloc(sizeof(struct isp_dev_access_context));
	if (NULL == cxt) {
		ISP_LOGE("failed to malloc");
		ret = ISP_ALLOC_ERROR;
		goto exit;
	}
	ISP_LOGV("input_ptr->camera_id %d", input_ptr->camera_id);
	cmr_bzero(cxt, sizeof(*cxt));
	cxt->camera_id = input_ptr->camera_id;
	cxt->caller_handle = input_ptr->caller_handle;
	cxt->pdaf_supported = input_ptr->pdaf_supported;

	input.camera_id = input_ptr->camera_id;
	input.width = input_ptr->init_param.size.w;
	input.height = input_ptr->init_param.size.h;
	input.alloc_cb = input_ptr->init_param.alloc_cb;
	input.free_cb = input_ptr->init_param.free_cb;
	input.mem_cb_handle = input_ptr->mem_cb_handle;
	input.shading_bin_addr = input_ptr->shading_bin_addr;
	input.shading_bin_size = input_ptr->shading_bin_size;
	input.irp_bin_addr = input_ptr->irp_bin_addr;
	input.irp_bin_size = input_ptr->irp_bin_size;
	input.cbc_bin_addr = input_ptr->pdaf_cbcp_bin_addr;
	input.cbc_bin_size = input_ptr->pdaf_cbc_bin_size;
	input.pdaf_supported = input_ptr->pdaf_supported;
	ISP_LOGI("cbc addr is %p, cbc size is 0x%x", (cmr_u32 *)input.cbc_bin_addr, input.cbc_bin_size);
	memcpy(&cxt->input_param, input_ptr, sizeof(struct isp_dev_init_in));

	ret = isp_dev_init(&input, &cxt->isp_driver_handle);
	if (ret) {
		ISP_LOGE("failed to dev initialized");
		goto exit;
	}

	ret = isp_dev_start(cxt->isp_driver_handle);
	if (ret) {
		ISP_LOGE("failed to dev_start %ld", ret);
		goto exit;
	}
exit:
	if (ret) {
		if (cxt) {
			isp_dev_deinit(cxt->isp_driver_handle);
			free((void *)cxt);
		}
	} else {
		cxt->is_inited = 1;
		*isp_dev_handle = (cmr_handle)cxt;
	}

	ATRACE_END();
	return ret;
}

cmr_int isp_dev_access_set_tuning_bin(cmr_handle isp_dev_handle, union isp_dev_ctrl_cmd_in *input_ptr)
{
	cmr_int                                ret = ISP_SUCCESS;
	struct isp_dev_access_context          *cxt = NULL;
	struct isp_dev_init_info               input;
	cmr_int                                fw_size = 0;
	cmr_u32                                fw_buf_num = 1;
	cmr_u32                                kaddr[2];
	cmr_u64                                kaddr_temp;
	cmr_s32                                fds[2];

	if (!input_ptr || !isp_dev_handle) {
		ISP_LOGI("input is NULL, 0x%lx", (cmr_int)input_ptr);
		ret = ISP_PARAM_NULL;
		goto exit;
	}

	cxt = isp_dev_handle;
	cxt->idx = input_ptr->value;
	ISP_LOGV("idx %d value %d", cxt->idx, input_ptr->value);
	ret = isp_dev_sel_iq_param_index(cxt->isp_driver_handle,cxt->idx);
	if (0 != ret) {
		ISP_LOGE("failed to load binary %ld", ret);
		goto exit;
	}
exit:
	return ret;
}
cmr_int isp_dev_access_deinit(cmr_handle isp_dev_handle)
{
	ATRACE_BEGIN(__func__);

	cmr_int                                ret = ISP_SUCCESS;
	struct isp_dev_access_context          *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	ISP_CHECK_HANDLE_VALID(isp_dev_handle);

	ret = isp_dev_deinit(cxt->isp_driver_handle);
	free((void *)cxt);
	ISP_LOGI("done %ld", ret);
	ATRACE_END();
	return ret;
}

cmr_int isp_dev_access_capability(cmr_handle isp_dev_handle, enum isp_capbility_cmd cmd, void *param_ptr)
{
	cmr_int                                ret = ISP_SUCCESS;
	struct isp_dev_access_context          *cxt = (struct isp_dev_access_context *)isp_dev_handle;
	struct isp_video_limit                 *limit = (struct isp_video_limit *)param_ptr;
	struct isp_img_size                    size = {0, 0};

	limit->width = 0;
	limit->height = 0;
	switch (cmd) {
	case ISP_VIDEO_SIZE:
	//	limit->width  = 1920; //TBD
	//	limit->height = 1080; //TBD
		ret = isp_dev_capability_video_size(cxt->isp_driver_handle, &size);
		if (!ret) {
			limit->width = (cmr_u16)size.width;
			limit->height = (cmr_u16)size.height;
		}
		break;
	case ISP_CAPTURE_SIZE:
	//	limit->width  = 1920; //TBD
	//	limit->height = 1080; //TBD
		ret = isp_dev_capability_single_size(cxt->isp_driver_handle, &size);
		if (!ret) {
			limit->width = (cmr_u16)size.width;
			limit->height = (cmr_u16)size.height;
		}
		break;
	case ISP_REG_VAL:
		ISP_LOGI("don't support");
		break;
	default:
		break;
	}
	ISP_LOGI("width height %d %d,", limit->width, limit->height);

	return ret;
}

cmr_u32 _isp_dev_access_convert_saturation(cmr_u32 value)
{
	cmr_u32 convert_value = ISP_SATURATION_LV4;

	switch (value) {
	case 0:
		convert_value = ISP_SATURATION_LV1;
		break;
	case 1:
		convert_value = ISP_SATURATION_LV2;
		break;
	case 2:
		convert_value = ISP_SATURATION_LV3;
		break;
	case 3:
		convert_value = ISP_SATURATION_LV4;
		break;
	case 4:
		convert_value = ISP_SATURATION_LV5;
		break;
	case 5:
		convert_value = ISP_SATURATION_LV6;
		break;
	case 6:
		convert_value = ISP_SATURATION_LV7;
		break;
	case 7:
		convert_value = ISP_SATURATION_LV8;
	default:
		ISP_LOGI("don't support saturation %d", value);
	}
	ISP_LOGI("set saturation %d", convert_value);
	return convert_value;
}

cmr_u32 _isp_dev_access_convert_contrast(cmr_u32 value)
{
	cmr_u32 convert_value = ISP_CONTRAST_LV4;

	switch (value) {
	case 0:
		convert_value = ISP_CONTRAST_LV1;
		break;
	case 1:
		convert_value = ISP_CONTRAST_LV2;
		break;
	case 2:
		convert_value = ISP_CONTRAST_LV3;
		break;
	case 3:
		convert_value = ISP_CONTRAST_LV4;
		break;
	case 4:
		convert_value = ISP_CONTRAST_LV5;
		break;
	case 5:
		convert_value = ISP_CONTRAST_LV6;
		break;
	case 6:
		convert_value = ISP_CONTRAST_LV7;
		break;
	case 7:
		convert_value = ISP_CONTRAST_LV8;
		break;
	default:
		ISP_LOGE("don't support contrast %d", value);
		break;
	}
	ISP_LOGI("set contrast %d", convert_value);
	return convert_value;
}

cmr_u32 _isp_dev_access_convert_sharpness(cmr_u32 value)
{
	cmr_u32 convert_value = ISP_SHARPNESS_LV4;

	switch (value) {
	case 0:
		convert_value = ISP_SHARPNESS_LV1;
		break;
	case 1:
		convert_value = ISP_SHARPNESS_LV2;
		break;
	case 2:
		convert_value = ISP_SHARPNESS_LV3;
		break;
	case 3:
		convert_value = ISP_SHARPNESS_LV4;
		break;
	case 4:
		convert_value = ISP_SHARPNESS_LV5;
		break;
	case 5:
		convert_value = ISP_SHARPNESS_LV6;
		break;
	case 6:
		convert_value = ISP_SHARPNESS_LV7;
		break;
	case 7:
		convert_value = ISP_SHARPNESS_LV8;
		break;
	default:
		ISP_LOGE("don't support sharpness %d", value);
		break;
	}
	ISP_LOGI("set sharpness %d", convert_value);
	return convert_value;
}
cmr_u32 _isp_dev_access_convert_effect(cmr_u32 value)
{
	cmr_u32 convert_value = ISP_SPECIAL_EFFECT_OFF;

	switch (value) {
	case CAMERA_EFFECT_NONE:
		convert_value = ISP_SPECIAL_EFFECT_OFF;
		break;
	case CAMERA_EFFECT_MONO:
		convert_value = ISP_SPECIAL_EFFECT_GRAYSCALE;
		break;
	case CAMERA_EFFECT_RED:
		convert_value = ISP_SPECIAL_EFFECT_REDPOINTS;
		break;
	case CAMERA_EFFECT_GREEN:
		convert_value = ISP_SPECIAL_EFFECT_GREENPOINTS;
		break;
	case CAMERA_EFFECT_BLUE:
		convert_value = ISP_SPECIAL_EFFECT_COLDVINTAGE; //ISP_SPECIAL_EFFECT_BLUEPOINTS
		break;
	case CAMERA_EFFECT_YELLOW:
		convert_value = ISP_SPECIAL_EFFECT_WARMVINTAGE; //ISP_SPECIAL_EFFECT_REDYELLOWPOINTS
		break;
	case CAMERA_EFFECT_NEGATIVE:
		convert_value = ISP_SPECIAL_EFFECT_NEGATIVE;
		break;
	case CAMERA_EFFECT_SEPIA:
		convert_value = ISP_SPECIAL_EFFECT_SEPIA;
		break;
	case CAMERA_EFFECT_SOLARIZE:
		convert_value = ISP_SPECIAL_EFFECT_SOLARIZE;
		break;
	case CAMERA_EFFECT_WARMVINTAGE:
		convert_value = ISP_SPECIAL_EFFECT_WARMVINTAGE;
		break;
	case CAMERA_EFFECT_COLDVINTAGE:
		convert_value = ISP_SPECIAL_EFFECT_COLDVINTAGE;
		break;
	case CAMERA_EFFECT_WASHOUT:
		convert_value = ISP_SPECIAL_EFFECT_WASHOUT;
		break;
	case CAMERA_EFFECT_POSTERISE:
		convert_value = ISP_SPECIAL_EFFECT_POSTERISE;
		break;
	case CAMERA_EFFECT_USERDEFINED:
		convert_value = ISP_SPECIAL_EFFECT_USERDEFINED;
		break;
	default:
		ISP_LOGI("don't support %d", value);
		break;
	}
	ISP_LOGI("set effect %d", convert_value);
	return convert_value;
}

cmr_int isp_dev_access_set_brightness(cmr_handle isp_dev_handle, union isp_dev_ctrl_cmd_in *input_ptr)
{
	cmr_int                                ret = ISP_SUCCESS;
	cmr_u32                                mode = 0;
	struct isp_brightness_gain             brightness_gain;
	struct isp_dev_access_context          *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	brightness_gain.uw_gain[0] = ISP_BRITNESS_GAIN_0;
	brightness_gain.uw_gain[1] = ISP_BRITNESS_GAIN_1;
	brightness_gain.uw_gain[2] = ISP_BRITNESS_GAIN_2;
	brightness_gain.uw_gain[3] = ISP_BRITNESS_GAIN_3;
	brightness_gain.uw_gain[4] = ISP_BRITNESS_GAIN_4;
	brightness_gain.uw_gain[5] = ISP_BRITNESS_GAIN_5;
	brightness_gain.uw_gain[6] = ISP_BRITNESS_GAIN_6;
	switch (input_ptr->value) {
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
		mode = input_ptr->value;
		break;
	default:
		ISP_LOGE("don't support %d", input_ptr->value);
		goto exit;
	}
	ret = isp_dev_cfg_brightness_gain(cxt->isp_driver_handle, &brightness_gain);
	if (ret) {
		ISP_LOGE("failed to set brightness gain %ld", ret);
		goto exit;
	}
	ret = isp_dev_cfg_brightness_mode(cxt->isp_driver_handle, mode);
	ISP_LOGI("mode %d, done %ld", mode, ret);
exit:
	return ret;
}

cmr_int _isp_dev_access_set_ccm(cmr_handle isp_dev_handle, union isp_dev_ctrl_cmd_in *input_ptr)
{
	cmr_int                                ret = ISP_SUCCESS;
	cmr_u32                                i = 0, len = MIN(IQ_CCM_INFO, CCM_TABLE_LEN);
	struct isp_iq_ccm_info                 ccm_info;
	struct isp_dev_access_context          *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	for (i = 0; i < len; i++) {
		ccm_info.ad_ccm[i] = input_ptr->ccm_table[i];
	}
	ret = isp_dev_cfg_ccm(cxt->isp_driver_handle, &ccm_info);

	return ret;
}

cmr_int isp_dev_access_ioctl(cmr_handle isp_dev_handle, enum isp_dev_access_ctrl_cmd cmd, union isp_dev_ctrl_cmd_in *input_ptr, union isp_dev_ctrl_cmd_out *output_ptr)
{
	cmr_int                                ret = ISP_SUCCESS;
	cmr_u32                                value = 0;
	struct isp_dev_access_context          *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	ISP_CHECK_HANDLE_VALID(isp_dev_handle);

	ISP_LOGV("cmd: %d", cmd);
	switch (cmd) {
	case ISP_DEV_ACCESS_SET_AWB_GAIN:
		break;
	case ISP_DEV_ACCESS_SET_ISP_SPEED:
		break;
	case ISP_DEV_ACCESS_SET_3A_PARAM:
		break;
	case ISP_DEV_ACCESS_SET_AE_PARAM:
		break;
	case ISP_DEV_ACCESS_SET_AWB_PARAM:
		break;
	case ISP_DEV_ACCESS_SET_AF_PARAM:
		break;
	case ISP_DEV_ACCESS_SET_ANTIFLICKER:
		break;
	case ISP_DEV_ACCESS_SET_YHIS_PARAM:
		break;
	case ISP_DEV_ACCESS_SET_BRIGHTNESS:
		ret = isp_dev_access_set_brightness(isp_dev_handle, input_ptr);
		break;
	case ISP_DEV_ACCESS_SET_TUNING_BIN:
		isp_dev_access_set_tuning_bin(isp_dev_handle, input_ptr);
		break;
	case ISP_DEV_ACCESS_SET_SATURATION:
		if (!input_ptr) {
			ISP_LOGI("failed to set sharpness,input is null");
			break;
		}
		value = _isp_dev_access_convert_saturation(input_ptr->value);
		ret = isp_dev_cfg_saturation(cxt->isp_driver_handle, value);
		break;
	case ISP_DEV_ACCESS_SET_CONSTRACT:
		if (!input_ptr) {
			ISP_LOGI("failed to set contrast,input is null");
			break;
		}
		value = _isp_dev_access_convert_contrast(input_ptr->value);
		ret = isp_dev_cfg_contrast(cxt->isp_driver_handle, value);
		break;
	case ISP_DEV_ACCESS_SET_SHARPNESS:
		if (!input_ptr) {
			ISP_LOGI("failed to set sharpness,input is null");
			break;
		}
		value = _isp_dev_access_convert_sharpness(input_ptr->value);
		ret = isp_dev_cfg_sharpness(cxt->isp_driver_handle, value);
		break;
	case ISP_DEV_ACCESS_SET_SPECIAL_EFFECT:
		if (!input_ptr) {
			ISP_LOGI("failed to set sharpness,input is null");
			break;
		}
		value = _isp_dev_access_convert_effect(input_ptr->value);
		ret = isp_dev_cfg_special_effect(cxt->isp_driver_handle, value);
		break;
	case ISP_DEV_ACCESS_SET_SUB_SAMPLE:
		break;
	case ISP_DEV_ACCESS_SET_STATIS_BUF:
		break;
	case ISP_DEV_ACCESS_SET_COLOR_TEMP:
		ISP_LOGV("color temp %d", input_ptr->value);
		ret = isp_dev_cfg_color_temp(cxt->isp_driver_handle, input_ptr->value);
		break;
	case ISP_DEV_ACCESS_SET_CCM:
		ret = _isp_dev_access_set_ccm(isp_dev_handle, input_ptr);
		break;
	case ISP_DEV_ACCESS_GET_STATIS_BUF:
		break;
	case ISP_DEV_ACCESS_GET_TIME:
		if (!output_ptr) {
			ISP_LOGI("failed to get dev time,output is null");
			break;
		}
		ret = isp_dev_get_timestamp(cxt->isp_driver_handle, &output_ptr->time.sec, &output_ptr->time.usec);
		break;
	default:
		ISP_LOGI("don't support %d", cmd);
		break;
	}
	return ret;
}

static void isp_dev_access_set_af_hw_cfg(struct af_cfg_info *af_cfg_info, struct isp3a_af_hw_cfg *af_hw_cfg)
{
	cmr_s32 i = 0;

	memcpy(af_cfg_info, af_hw_cfg, sizeof(struct af_cfg_info));

	ISP_LOGV("token_id = %d", af_hw_cfg->token_id);
	ISP_LOGV("uw_size_ratio_x = %d uw_size_ratio_y = %d uw_blk_num_x = %d uw_blk_num_y = %d",
		af_hw_cfg->af_region.uw_size_ratio_x, af_hw_cfg->af_region.uw_size_ratio_y,
		af_hw_cfg->af_region.uw_blk_num_x, af_hw_cfg->af_region.uw_blk_num_y);
	ISP_LOGV("uw_offset_ratio_x = %d uw_offset_ratio_y = %d",
		af_hw_cfg->af_region.uw_offset_ratio_x, af_hw_cfg->af_region.uw_offset_ratio_y);
	ISP_LOGV("enable_af_lut = %d", af_hw_cfg->enable_af_lut);

	for (i = 0; i < 259; i++)
		ISP_LOGV("auw_lut[%d] = %d", i, af_hw_cfg->auw_lut[i]);
	for (i = 0; i < 259; i++)
		ISP_LOGV("auw_af_lut[%d] = %d", i, af_hw_cfg->auw_af_lut[i]);
	for (i = 0; i < 6; i++)
		ISP_LOGV("auc_weight[%d] = %d", i, af_hw_cfg->auc_weight[i]);

	ISP_LOGV("uw_sh = %d", af_hw_cfg->uw_sh);
	ISP_LOGV("uc_th_mode = %d", af_hw_cfg->uc_th_mode);
	for (i = 0; i < 82; i++)
		ISP_LOGV("auc_index[%d] = %d", i, af_hw_cfg->auc_index[i]);
	for (i = 0; i < 4; i++)
		ISP_LOGV("auw_th[%d] = %d", i, af_hw_cfg->auw_th[i]);
	for (i = 0; i < 4; i++)
		ISP_LOGV("pw_tv[%d] = %d", i, af_hw_cfg->pw_tv[i]);
	ISP_LOGV("ud_af_offset = %d", af_hw_cfg->ud_af_offset);
	ISP_LOGV("af_py_enable= %d", af_hw_cfg->af_py_enable);
	ISP_LOGV("af_lpf_enable = %d", af_hw_cfg->af_lpf_enable);
	ISP_LOGV("filter_mode = %ld", af_hw_cfg->filter_mode);
	ISP_LOGV("uc_filter_id = %d", af_hw_cfg->uc_filter_id);
	ISP_LOGV("uw_ine_cnt = %d", af_hw_cfg->uw_ine_cnt);
}

void isp_dev_access_convert_ae_param(struct isp3a_ae_hw_cfg *from, struct ae_cfg_info *to)
{
	if (!from || !to) {
		ISP_LOGE("param %p %p is NULL !!!", from, to);
		return;
	}
	to->token_id = from->tokenID;
	to->ae_region.border_ratio_x = from->region.border_ratio_X;
	to->ae_region.border_ratio_y = from->region.border_ratio_Y;
	to->ae_region.blk_num_x = from->region.blk_num_X;
	to->ae_region.blk_num_y = from->region.blk_num_Y;
	to->ae_region.offset_ratio_x = from->region.offset_ratio_X;
	to->ae_region.offset_ratio_y = from->region.offset_ratio_Y;
}

void isp_dev_access_convert_afl_param(struct isp3a_afl_hw_cfg *from, struct antiflicker_cfg_info *to)
{
	if (!from || !to) {
		ISP_LOGE("param %p %p is NULL !!!", from, to);
		return;
	}
	to->token_id = from->token_id;
	to->offset_ratio_x = from->offset_ratiox;
	to->offset_ratio_y = from->offset_ratioy;
}

void isp_dev_access_convert_awb_param(struct isp3a_awb_hw_cfg *data, struct awb_cfg_info *awb_param)
{
	cmr_int                                ret = ISP_SUCCESS;
	cmr_u32                                i = 0;

	//memcpy(&awb_param, data, sizeof(AWB_CfgInfo));
	awb_param->token_id = data->token_id;
	awb_param->awb_region.blk_num_x = data->region.blk_num_X;
	awb_param->awb_region.blk_num_y = data->region.blk_num_Y;
	awb_param->awb_region.border_ratio_x = data->region.border_ratio_X;
	awb_param->awb_region.border_ratio_y = data->region.border_ratio_Y;
	awb_param->awb_region.offset_ratio_x = data->region.offset_ratio_X;
	awb_param->awb_region.offset_ratio_y = data->region.offset_ratio_Y;
	for (i = 0; i < 16; i++) {
		awb_param->y_factor[i] = data->uc_factor[i];
	}
	for (i = 0; i < 33; i++) {
		awb_param->bbr_factor[i] = data->bbr_factor[i];
	}
	awb_param->r_gain = data->uw_rgain;
	awb_param->g_gain = data->uw_ggain;
	awb_param->b_gain = data->uw_bgain;
	awb_param->cr_shift = data->uccr_shift;
	awb_param->offset_shift = data->uc_offset_shift;
	awb_param->quantize = data->uc_quantize;
	awb_param->damp = data->uc_damp;
	awb_param->sum_shift = data->uc_sum_shift;
	awb_param->t_his.enable = data->t_his.benable;
	awb_param->t_his.cr_end = data->t_his.ccrend;
	awb_param->t_his.cr_purple = data->t_his.ccrpurple;
	awb_param->t_his.cr_start = data->t_his.ccrstart;
	awb_param->t_his.grass_end = data->t_his.cgrass_end;
	awb_param->t_his.grass_offset = data->t_his.cgrass_offset;
	awb_param->t_his.grass_start = data->t_his.cgrass_start;
	awb_param->t_his.offset_down = data->t_his.coffsetdown;
	awb_param->t_his.offset_up = data->t_his.cooffsetup;
	awb_param->t_his.offset_bbr_w_end = data->t_his.coffset_bbr_w_end;
	awb_param->t_his.offset_bbr_w_start = data->t_his.coffset_bbr_w_start;
	awb_param->t_his.his_interp = data->t_his.dhisinterp;
	awb_param->t_his.damp_grass = data->t_his.ucdampgrass;
	awb_param->t_his.offset_purple = data->t_his.ucoffsetpurple;
	awb_param->t_his.yfac_w = data->t_his.ucyfac_w;
	awb_param->r_linear_gain = data->uwrlinear_gain;
	awb_param->b_linear_gain = data->uwblinear_gain;
	ISP_LOGI("token_id = %d, uccr_shift = %d, uc_damp = %d, uc_offset_shift = %d\n",
		awb_param->token_id, awb_param->cr_shift, awb_param->damp, awb_param->offset_shift);
	ISP_LOGV("uc_quantize = %d\n, uc_sum_shift = %d\n, uwblinear_gain = %d\n, uwrlinear_gain = %d\n,\
		uw_bgain = %d\n, uw_rgain = %d\n, uw_ggain = %d\n",
		awb_param->quantize, awb_param->sum_shift, awb_param->b_linear_gain, awb_param->r_linear_gain,
		awb_param->b_gain, awb_param->g_gain, awb_param->r_gain);
	for (i = 0; i < 16; i++) {
		ISP_LOGV("uc_factor[%d] = %d\n", i, awb_param->y_factor[i]);
	}
	for (i = 0; i < 33; i++) {
		ISP_LOGV("bbr_factor[%d] = %d\n", i, awb_param->bbr_factor[i]);
	}
	ISP_LOGV("region:blk_num_X = %d, blk_num_Y = %d\n, border_ratio_X = %d, border_ratio_Y = %d\n,\
		offset_ratio_X = %d, offset_ratio_Y = %d\n",
		awb_param->awb_region.blk_num_x, awb_param->awb_region.blk_num_y,
		awb_param->awb_region.border_ratio_x, awb_param->awb_region.border_ratio_y,
		awb_param->awb_region.offset_ratio_x, awb_param->awb_region.offset_ratio_y);
	ISP_LOGV("t_his:benable = %d\n ccrend = %d\n ccrpurple = %d\n ccrstart = %d\n \
		cgrass_end = %d\n cgrass_offset = %d\n cgrass_start = %d\n coffsetdown = %d\n \
		coffset_bbr_w_end = %d\n coffset_bbr_w_start = %d\n cooffsetup = %d\n \
		dhisinterp = %d\n ucdampgrass = %d\n ucoffsetpurple = %d\n ucyfac_w = %d\n",
		awb_param->t_his.enable, awb_param->t_his.cr_end, awb_param->t_his.cr_purple,
		awb_param->t_his.cr_start, awb_param->t_his.grass_end, awb_param->t_his.grass_offset,
		awb_param->t_his.grass_start, awb_param->t_his.offset_down, awb_param->t_his.offset_bbr_w_end,
		awb_param->t_his.offset_bbr_w_start, awb_param->t_his.offset_up, awb_param->t_his.his_interp,
		awb_param->t_his.damp_grass, awb_param->t_his.offset_purple, awb_param->t_his.yfac_w);

}
#define FPGA_TEST     1

cmr_int isp_dev_access_start_multiframe(cmr_handle isp_dev_handle, struct isp_dev_access_start_in *param_ptr)
{
	cmr_int                                ret = ISP_SUCCESS;
	struct isp_dev_access_context          *cxt = (struct isp_dev_access_context *)isp_dev_handle;
	struct scenario_info_ap                input_data;
	struct cfg_3a_info                     cfg3a_info;
	struct dld_sequence                    dldseq_info;
	cmr_u32                                iso_gain = 0;
	struct isp_awb_gain_info               awb_gain_info;
	cmr_u32                                isp_id = 0;
	struct isp_sensor_resolution_info      *resolution_ptr = NULL;
	struct isp_raw_data                    isp_raw_mem;
	cmr_u32                                capture_mode = 0;
	struct isp_cfg_img_param               img_buf_param;
	struct scenario_info_ap                tSecnarioInfo;
	struct isp_iq_otp_info                 iq_info;
	struct isp_dev_init_param              init_param;
	char                                   value[PROPERTY_VALUE_MAX];

	ISP_CHECK_HANDLE_VALID(isp_dev_handle);

	init_param.width = param_ptr->common_in.resolution_info.sensor_size.w;
	init_param.height = param_ptr->common_in.resolution_info.sensor_size.h;
	init_param.camera_id = cxt->camera_id;
	property_get("persist.sys.camera.raw.mode", value, "jpeg");
	if (!strcmp(value, "raw")) {
		init_param.raw_mode = 1;
	} else {
		init_param.raw_mode = 0;
	}
	isp_dev_set_init_param(cxt->isp_driver_handle, &init_param);

	if (param_ptr->common_in.sensor_fps.is_high_fps) {
		if ((param_ptr->common_in.sensor_fps.high_fps_skip_num - 1) > 0) {
			ret = isp_dev_set_deci_num(cxt->isp_driver_handle,
							  param_ptr->common_in.sensor_fps.high_fps_skip_num - 1);
			if (ret) {
				ISP_LOGE("failed to deci num");
			}
		}
	}

#if defined(CONFIG_CAMERA_NO_DCAM_DATA_PATH)
	cxt->input_param.init_param.size.w = param_ptr->common_in.size.w;
	cxt->input_param.init_param.size.h = param_ptr->common_in.size.h;
#else
	cxt->input_param.init_param.size.w = param_ptr->common_in.resolution_info.sensor_size.w;
	cxt->input_param.init_param.size.h = param_ptr->common_in.resolution_info.sensor_size.h;
#endif
	isp_dev_set_capture_mode(cxt->isp_driver_handle, param_ptr->common_in.capture_mode);

	ret = isp_dev_get_isp_id(cxt->isp_driver_handle, &isp_id);
	if (ISP_SUCCESS != ret) {
		ISP_LOGE("isp_dev_get_isp_id error %ld", ret);
		goto exit;
	}

	memset(&tSecnarioInfo, 0, sizeof(tSecnarioInfo));
	resolution_ptr = &param_ptr->common_in.resolution_info;
	tSecnarioInfo.tSensorInfo.cbc_enabled = 0;
	tSecnarioInfo.tSensorInfo.uwOriginalWidth = resolution_ptr->sensor_max_size.w;
	tSecnarioInfo.tSensorInfo.uwOriginalHeight = resolution_ptr->sensor_max_size.h;
	tSecnarioInfo.tSensorInfo.uwWidth = resolution_ptr->sensor_output_size.w;
	tSecnarioInfo.tSensorInfo.uwHeight = resolution_ptr->sensor_output_size.h;
	tSecnarioInfo.tSensorInfo.uwCropStartX = resolution_ptr->crop.st_x;
	tSecnarioInfo.tSensorInfo.uwCropStartY = resolution_ptr->crop.st_y;
	tSecnarioInfo.tSensorInfo.uwCropEndX = resolution_ptr->sensor_max_size.w + resolution_ptr->crop.st_x - 1;
	tSecnarioInfo.tSensorInfo.uwCropEndY = resolution_ptr->sensor_max_size.h + resolution_ptr->crop.st_y - 1;
	tSecnarioInfo.tSensorInfo.udLineTime = resolution_ptr->line_time/10;
	tSecnarioInfo.tSensorInfo.uwClampLevel = cxt->input_param.init_param.ex_info.clamp_level;
	tSecnarioInfo.tSensorInfo.uwFrameRate = resolution_ptr->fps.max_fps*100;
	tSecnarioInfo.tSensorInfo.ucSensorMouduleType = 0;//0-sensor1  1-sensor2
	tSecnarioInfo.tSensorInfo.nColorOrder = cxt->input_param.init_param.image_pattern;

#if defined(CONFIG_CAMERA_NO_DCAM_DATA_PATH)
	tSecnarioInfo.tScenarioOutBypassFlag.bBypassLV = 1;
	tSecnarioInfo.tScenarioOutBypassFlag.bBypassVideo = 1;
	tSecnarioInfo.tScenarioOutBypassFlag.bBypassStill = 1;
#else
	tSecnarioInfo.tScenarioOutBypassFlag.bBypassLV = 0;
	tSecnarioInfo.tScenarioOutBypassFlag.bBypassVideo = 0;
	tSecnarioInfo.tScenarioOutBypassFlag.bBypassStill = 0;
#endif
	tSecnarioInfo.tScenarioOutBypassFlag.bBypassMetaData = 0;
	if (resolution_ptr->sensor_output_size.w == resolution_ptr->sensor_max_size.w)
		tSecnarioInfo.tSensorInfo.ucSensorMode = 0;
	else if (resolution_ptr->sensor_output_size.w < resolution_ptr->sensor_max_size.w)
		tSecnarioInfo.tSensorInfo.ucSensorMode = 1;

	if (ISP_CAP_MODE_HIGHISO == param_ptr->common_in.capture_mode ||
		ISP_CAP_MODE_BURST == param_ptr->common_in.capture_mode) {
#if VIDEO_USE
		/*decrease isp scaling bandwith*/
		tSecnarioInfo.tBayerSCLOutInfo.uwBayerSCLOutWidth = param_ptr->common_in.video_size.w;
		tSecnarioInfo.tBayerSCLOutInfo.uwBayerSCLOutHeight = param_ptr->common_in.video_size.h;
#else
		tSecnarioInfo.tScenarioOutBypassFlag.bBypassVideo = 1;
		tSecnarioInfo.tBayerSCLOutInfo.uwBayerSCLOutWidth = param_ptr->common_in.lv_size.w;//cxt->input_param.init_param.size.w;
		tSecnarioInfo.tBayerSCLOutInfo.uwBayerSCLOutHeight = param_ptr->common_in.lv_size.h;//cxt->input_param.init_param.size.h;
#endif
		ISP_LOGI("BayerSCL w %d h %d\n", tSecnarioInfo.tBayerSCLOutInfo.uwBayerSCLOutWidth,
			 tSecnarioInfo.tBayerSCLOutInfo.uwBayerSCLOutHeight);
	} else {
		tSecnarioInfo.tScenarioOutBypassFlag.bBypassLV = 1;
		tSecnarioInfo.tScenarioOutBypassFlag.bBypassVideo = 1;
		tSecnarioInfo.tBayerSCLOutInfo.uwBayerSCLOutWidth = 0;
		tSecnarioInfo.tBayerSCLOutInfo.uwBayerSCLOutHeight = 0;
	}
	tSecnarioInfo.tIqParamIdxInfo.iq_param_idx_still = 0;

	if (SENSOR_PDAF_TYPE3_ENABLE == cxt->pdaf_supported)
		tSecnarioInfo.tSensorInfo.cbc_enabled = 1;

	ISP_LOGI("size %dx%d, line time %d frameRate %d clamp %d image_pattern %d sensor_mode %d",
		tSecnarioInfo.tSensorInfo.uwWidth, tSecnarioInfo.tSensorInfo.uwHeight,
		tSecnarioInfo.tSensorInfo.udLineTime, tSecnarioInfo.tSensorInfo.uwFrameRate,
		tSecnarioInfo.tSensorInfo.uwClampLevel, tSecnarioInfo.tSensorInfo.nColorOrder,
		tSecnarioInfo.tSensorInfo.ucSensorMode);
	ISP_LOGI("sensor out %d %d %d %d %d %d cbc_en %d", tSecnarioInfo.tSensorInfo.uwCropStartX,
		tSecnarioInfo.tSensorInfo.uwCropStartY, tSecnarioInfo.tSensorInfo.uwCropEndX,
		tSecnarioInfo.tSensorInfo.uwCropEndY, tSecnarioInfo.tSensorInfo.uwOriginalWidth,
		tSecnarioInfo.tSensorInfo.uwOriginalHeight, tSecnarioInfo.tSensorInfo.cbc_enabled);
	ret = isp_dev_cfg_scenario_info(cxt->isp_driver_handle, &tSecnarioInfo);
	if (ISP_SUCCESS != ret) {
		ISP_LOGE("isp_dev_cfg_scenario_info error %ld", ret);
		return -1;
	}
	if (SENSOR_PDAF_TYPE3_ENABLE == cxt->pdaf_supported) {
		ret = isp_dev_load_cbc(cxt->isp_driver_handle);
		if (0 != ret) {
			ISP_LOGE("failed to load cbc");
			return -1;
		}
	}
	/*set still image buffer format*/

#if defined(CONFIG_CAMERA_NO_DCAM_DATA_PATH)
	if (ISP_CAP_MODE_BURST == param_ptr->common_in.capture_mode) {
		memset(&img_buf_param, 0, sizeof(img_buf_param));

		if (2 == isp_id) {
			img_buf_param.img_id = ISP_IMG_PREVIEW;
		} else {
			img_buf_param.img_id = ISP_IMG_STILL_CAPTURE;
		}
		img_buf_param.format = ISP_OUT_IMG_YUY2;

		if (ISP_CAP_MODE_HIGHISO == param_ptr->common_in.capture_mode
			|| ISP_CAP_MODE_DRAM == param_ptr->common_in.capture_mode
			|| ISP_CAP_MODE_BURST == param_ptr->common_in.capture_mode) {
			img_buf_param.format = ISP_OUT_IMG_NV12;
			img_buf_param.width = cxt->input_param.init_param.size.w;
			img_buf_param.height = cxt->input_param.init_param.size.h;
		} else {
			img_buf_param.width = param_ptr->common_in.video_size.w;
			img_buf_param.height = param_ptr->common_in.video_size.h;
		}
		img_buf_param.dram_eb = 0;
		img_buf_param.buf_num = 4;
		if (ISP_OUT_IMG_YUY2 == img_buf_param.format)
			img_buf_param.line_offset = 2 * img_buf_param.width;
		else if (ISP_OUT_IMG_NV12 == img_buf_param.format)
			img_buf_param.line_offset = img_buf_param.width;
		img_buf_param.addr[0].chn0 = 0x2FFFFFFF;
		img_buf_param.addr[1].chn0 = 0x2FFFFFFF;
		img_buf_param.addr[2].chn0 = 0x2FFFFFFF;
		img_buf_param.addr[3].chn0 = 0x2FFFFFFF;
		ISP_LOGI("set still image buffer param img_id %d w %d h %d", img_buf_param.img_id,
				img_buf_param.width, img_buf_param.height);
		ret = isp_dev_set_img_param(cxt->isp_driver_handle, &img_buf_param);
	}
#else
	memset(&img_buf_param, 0, sizeof(img_buf_param));

	if (2 == isp_id) {
		img_buf_param.img_id = ISP_IMG_PREVIEW;
	} else {
		img_buf_param.img_id = ISP_IMG_STILL_CAPTURE;
	}
	img_buf_param.format = ISP_OUT_IMG_YUY2;

	if (ISP_CAP_MODE_HIGHISO == param_ptr->common_in.capture_mode
		|| ISP_CAP_MODE_DRAM == param_ptr->common_in.capture_mode
		|| ISP_CAP_MODE_BURST == param_ptr->common_in.capture_mode) {
		img_buf_param.format = ISP_OUT_IMG_NV12;
		img_buf_param.width = cxt->input_param.init_param.size.w;
		img_buf_param.height = cxt->input_param.init_param.size.h;
	} else {
		img_buf_param.width = param_ptr->common_in.video_size.w;
		img_buf_param.height = param_ptr->common_in.video_size.h;
	}
	img_buf_param.dram_eb = 0;
	img_buf_param.buf_num = 4;
	if (ISP_OUT_IMG_YUY2 == img_buf_param.format)
		img_buf_param.line_offset = 2 * img_buf_param.width;
	else if (ISP_OUT_IMG_NV12 == img_buf_param.format)
		img_buf_param.line_offset = img_buf_param.width;
	img_buf_param.addr[0].chn0 = 0x2FFFFFFF;
	img_buf_param.addr[1].chn0 = 0x2FFFFFFF;
	img_buf_param.addr[2].chn0 = 0x2FFFFFFF;
	img_buf_param.addr[3].chn0 = 0x2FFFFFFF;
	ISP_LOGI("set still image buffer param img_id %d w %d h %d", img_buf_param.img_id,
			img_buf_param.width, img_buf_param.height);
	ret = isp_dev_set_img_param(cxt->isp_driver_handle, &img_buf_param);
#endif

#ifdef FPGA_TEST
	// SubSample
	cfg3a_info.subsample_info.token_id = 0x100;
	cfg3a_info.subsample_info.buffer_image_size = 1158*870;
	cfg3a_info.subsample_info.offset_ratio_x = 0;
	cfg3a_info.subsample_info.offset_ratio_y = 0;

	// Yhis configuration:
	cfg3a_info.yhis_info.token_id = 0x120;
	cfg3a_info.yhis_info.yhis_region.border_ratio_x = 100;
	cfg3a_info.yhis_info.yhis_region.border_ratio_y = 100;
	cfg3a_info.yhis_info.yhis_region.blk_num_x = 16;
	cfg3a_info.yhis_info.yhis_region.blk_num_y = 16;
	cfg3a_info.yhis_info.yhis_region.offset_ratio_x = 0;
	cfg3a_info.yhis_info.yhis_region.offset_ratio_y = 0;
#else
	memcpy(&cfg3a_info.hw_cfg.antiflicker_info, &param_ptr->hw_cfg.afl_cfg, sizeof(struct antiflicker_cfg_info));
	memcpy(&cfg3a_info.hw_cfg.yhis_info, &param_ptr->hw_cfg.yhis_cfg, sizeof(struct yhis_cfg_info);
	memcpy(&cfg3a_info.hw_cfg.subsample_info, &param_ptr->hw_cfg.subsample_cfg, sizeof(struct subsample_cfg_info));
#endif

	isp_dev_access_convert_ae_param(&param_ptr->hw_cfg.ae_cfg, &cfg3a_info.ae_info);
	isp_dev_access_convert_afl_param(&param_ptr->hw_cfg.afl_cfg, &cfg3a_info.antiflicker_info);
	isp_dev_access_convert_awb_param(&param_ptr->hw_cfg.awb_cfg, &cfg3a_info.awb_info);
	isp_dev_access_set_af_hw_cfg(&cfg3a_info.af_info, &param_ptr->hw_cfg.af_cfg);

	ret = isp_dev_cfg_3a_param(cxt->isp_driver_handle, &cfg3a_info);
	if (ret) {
		ISP_LOGE("failed to cfg 3a %ld", ret);
		goto exit;
	}
#ifdef FPGA_TEST
	dldseq_info.preview_baisc_dld_seq_length = 4;
	dldseq_info.preview_baisc_dld_seq[0] = 2;
	dldseq_info.preview_baisc_dld_seq[1] = 3;
	dldseq_info.preview_baisc_dld_seq[2] = 1;
	dldseq_info.preview_baisc_dld_seq[3] = 3;

	dldseq_info.preview_adv_dld_seq_length = 1;
	dldseq_info.preview_adv_dld_seq[0] = 1;

	dldseq_info.fast_converge_baisc_dld_seq_length = 0;
	dldseq_info.fast_converge_baisc_dld_seq[0] = 10;
#else
	memcpy(&dldseq_info, &param_ptr->dld_seq, sizeof(struct dld_sequence));
#endif
	ret = isp_dev_cfg_dld_seq(cxt->isp_driver_handle, &dldseq_info);
	if (ret) {
		ISP_LOGE("failed to cfg dld sequence %ld", ret);
		goto exit;
	}

	iso_gain = param_ptr->hw_iso_speed;
	ISP_LOGI("cfg hw_iso_speed:%d", iso_gain);
	ret = isp_dev_cfg_iso_speed(cxt->isp_driver_handle, &iso_gain);
	if (ret) {
		ISP_LOGE("failed to cfg iso speed %ld", ret);
		goto exit;
	}

	awb_gain_info.r = param_ptr->hw_cfg.awb_gain.r;
	awb_gain_info.g = param_ptr->hw_cfg.awb_gain.g;
	awb_gain_info.b = param_ptr->hw_cfg.awb_gain.b;
	ret = isp_dev_cfg_awb_gain(cxt->isp_driver_handle, &awb_gain_info);
	if (ret) {
		ISP_LOGE("failed to set awb gain %ld", ret);
		goto exit;
	}
	awb_gain_info.r = param_ptr->hw_cfg.awb_gain_balanced.r;
	awb_gain_info.g = param_ptr->hw_cfg.awb_gain_balanced.g;
	awb_gain_info.b = param_ptr->hw_cfg.awb_gain_balanced.b;
	ret = isp_dev_cfg_awb_gain_balanced(cxt->isp_driver_handle, &awb_gain_info);

	if (ISP_CAP_MODE_HIGHISO == param_ptr->common_in.capture_mode ||
	    ISP_CAP_MODE_BURST == param_ptr->common_in.capture_mode) {
#if defined(CONFIG_CAMERA_NO_DCAM_DATA_PATH)
		if (ISP_CAP_MODE_HIGHISO == param_ptr->common_in.capture_mode) {
			/*set still image buffer format*/
			memset(&img_buf_param, 0, sizeof(img_buf_param));

			img_buf_param.format = ISP_OUT_IMG_YUY2;
			img_buf_param.img_id = ISP_IMG_PREVIEW;
			img_buf_param.dram_eb = 0;
			img_buf_param.buf_num = 4;
			img_buf_param.width = param_ptr->common_in.lv_size.w;//cxt->input_param.init_param.size.w;
			img_buf_param.height = param_ptr->common_in.lv_size.h;//cxt->input_param.init_param.size.h;
			img_buf_param.line_offset = img_buf_param.width * 2;
			img_buf_param.addr[0].chn0 = 0x2FFFFFFF;
			img_buf_param.addr[1].chn0 = 0x2FFFFFFF;
			img_buf_param.addr[2].chn0 = 0x2FFFFFFF;
			img_buf_param.addr[3].chn0 = 0x2FFFFFFF;
			ISP_LOGI("set preview image buffer param img_id %d w %d h %d", img_buf_param.img_id,
				img_buf_param.width, img_buf_param.height);
			ret = isp_dev_set_img_param(cxt->isp_driver_handle, &img_buf_param);

		}
#else
		/*set still image buffer format*/
		memset(&img_buf_param, 0, sizeof(img_buf_param));

		img_buf_param.format = ISP_OUT_IMG_YUY2;
		img_buf_param.img_id = ISP_IMG_PREVIEW;
		img_buf_param.dram_eb = 0;
		img_buf_param.buf_num = 4;
		img_buf_param.width = param_ptr->common_in.lv_size.w;//cxt->input_param.init_param.size.w;
		img_buf_param.height = param_ptr->common_in.lv_size.h;//cxt->input_param.init_param.size.h;
		img_buf_param.line_offset = img_buf_param.width * 2;
		img_buf_param.addr[0].chn0 = 0x2FFFFFFF;
		img_buf_param.addr[1].chn0 = 0x2FFFFFFF;
		img_buf_param.addr[2].chn0 = 0x2FFFFFFF;
		img_buf_param.addr[3].chn0 = 0x2FFFFFFF;
		ISP_LOGI("set preview image buffer param img_id %d w %d h %d", img_buf_param.img_id,
			img_buf_param.width, img_buf_param.height);
		ret = isp_dev_set_img_param(cxt->isp_driver_handle, &img_buf_param);

#if VIDEO_USE
		memset(&img_buf_param, 0, sizeof(img_buf_param));
		img_buf_param.format = ISP_OUT_IMG_YUY2;
		img_buf_param.img_id = ISP_IMG_VIDEO;
		img_buf_param.dram_eb = 0;
		img_buf_param.buf_num = 4;
		img_buf_param.width = param_ptr->common_in.video_size.w;//cxt->input_param.init_param.size.w;
		img_buf_param.height = param_ptr->common_in.video_size.h;//cxt->input_param.init_param.size.h;
		img_buf_param.line_offset = img_buf_param.width * 2;
		img_buf_param.addr[0].chn0 = 0x2FFFFFFF;
		img_buf_param.addr[1].chn0 = 0x2FFFFFFF;
		img_buf_param.addr[2].chn0 = 0x2FFFFFFF;
		img_buf_param.addr[3].chn0 = 0x2FFFFFFF;
		ISP_LOGI("set video image buffer param img_id %d  w %d h %d", img_buf_param.img_id,
			img_buf_param.width, img_buf_param.height);
		ret = isp_dev_set_img_param(cxt->isp_driver_handle, &img_buf_param);
#endif
#endif
		memset(&isp_raw_mem, 0, sizeof(isp_raw_mem));
//		isp_raw_mem.fd[0] = param_ptr->common_in.raw_buf_fd[0];
//		isp_raw_mem.phy_addr[0] = param_ptr->common_in.raw_buf_phys_addr[0];
//		isp_raw_mem.virt_addr[0] = param_ptr->common_in.raw_buf_virt_addr[0];
		memcpy(isp_raw_mem.fd, param_ptr->common_in.raw_buf_fd, sizeof(cmr_s32) * param_ptr->common_in.raw_buf_cnt);
		memcpy(isp_raw_mem.phy_addr, param_ptr->common_in.raw_buf_phys_addr, sizeof(unsigned int) * param_ptr->common_in.raw_buf_cnt);
		memcpy(isp_raw_mem.virt_addr, param_ptr->common_in.raw_buf_virt_addr, sizeof(cmr_u64) * param_ptr->common_in.raw_buf_cnt);
		isp_raw_mem.cnt = param_ptr->common_in.raw_buf_cnt;

		isp_raw_mem.size = param_ptr->common_in.raw_buf_size;
		isp_raw_mem.width = param_ptr->common_in.raw_buf_width;
		isp_raw_mem.height = param_ptr->common_in.raw_buf_height;
		isp_raw_mem.fmt = ISP_OUTPUT_ALTEK_RAW10; //ISP_OUTPUT_ALTEK_RAW10;
		ret = isp_dev_set_rawaddr(cxt->isp_driver_handle, &isp_raw_mem);
		ISP_LOGI("raw10_buf fd 0x%x phy_addr 0x%x virt_addr 0x%x", isp_raw_mem.fd[0],
			isp_raw_mem.phy_addr[0], (cmr_u32)isp_raw_mem.virt_addr[0]);

		memset(&isp_raw_mem, 0, sizeof(isp_raw_mem));
		isp_raw_mem.fd[0] = param_ptr->common_in.highiso_buf_fd;
		isp_raw_mem.phy_addr[0] = param_ptr->common_in.highiso_buf_phys_addr;
		isp_raw_mem.virt_addr[0] = param_ptr->common_in.highiso_buf_virt_addr;
		isp_raw_mem.cnt = 1;
		isp_raw_mem.size = param_ptr->common_in.highiso_buf_size;
		isp_raw_mem.width = param_ptr->common_in.raw_buf_width;
		isp_raw_mem.height = param_ptr->common_in.raw_buf_height;
		ret = isp_dev_highiso_mode(cxt->isp_driver_handle, &isp_raw_mem);
		if (ret) {
			ISP_LOGE("failed to set highiso mode %ld", ret);
			goto exit;
		}
	}

#if defined(CONFIG_CAMERA_NO_DCAM_DATA_PATH)
#else
	ret = isp_dev_stream_on(cxt->isp_driver_handle);
#endif

exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int isp_dev_access_stop_multiframe(cmr_handle isp_dev_handle)
{
	cmr_int                                ret = ISP_SUCCESS;
	struct isp_dev_access_context          *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	ret = isp_dev_stream_off(cxt->isp_driver_handle);
	ret = isp_dev_stop(cxt->isp_driver_handle);

	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int isp_dev_access_start_postproc(cmr_handle isp_dev_handle, struct isp_dev_postproc_in *input_ptr, struct isp_dev_postproc_out *output_ptr)
{
	cmr_int                                ret = ISP_SUCCESS;
	struct isp_dev_access_context          *cxt = (struct isp_dev_access_context *)isp_dev_handle;
	struct scenario_info_ap                scenario_in;
	struct cfg_3a_info                     cfg_info;
	struct isp_cfg_img_param               img_param;
	struct isp_awb_gain_info               awb_gain;
	struct dld_sequence                    dldseq;
	struct isp_cfg_img_param               img_buf_param;
	cmr_u32                                iso_gain = 0;
	cmr_u32                                cap_mode = 0;
	struct isp_raw_data                    isp_raw_mem;
	struct isp_img_mem                     img_mem;
	struct isp_sensor_resolution_info      *resolution_ptr = NULL;
	struct isp_img_size                    size;
	struct isp_dev_init_param              init_param;
	char                                   value[PROPERTY_VALUE_MAX];

	UNUSED(output_ptr);
	ISP_CHECK_HANDLE_VALID(isp_dev_handle);
	ret = isp_dev_start(cxt->isp_driver_handle);
	if (ret) {
		ISP_LOGE("failed to dev_start %ld", ret);
		goto exit;
	}

	init_param.width = input_ptr->src_frame.img_size.w;
	init_param.height = input_ptr->src_frame.img_size.h;
	init_param.camera_id = cxt->camera_id;
	property_get("persist.sys.camera.raw.mode", value, "jpeg");
	if (!strcmp(value, "raw")) {
		init_param.raw_mode = 1;
	} else {
		init_param.raw_mode = 0;
	}
	isp_dev_set_init_param(cxt->isp_driver_handle, &init_param);

	if (2 == input_ptr->cap_mode)
		cap_mode = ISP_CAP_MODE_HIGHISO_RAW_CAP;
	else
		cap_mode = ISP_CAP_MODE_RAW_DATA;
	isp_dev_set_capture_mode(cxt->isp_driver_handle, cap_mode);
	ISP_LOGI("cap_mode = %d", cap_mode);

	memset(&isp_raw_mem, 0, sizeof(struct isp_raw_data));
#if 0
	isp_raw_mem.fd[0] = input_ptr->dst2_frame.img_fd.y;
	isp_raw_mem.phy_addr[0] = input_ptr->dst2_frame.img_addr_phy.chn0;
	isp_raw_mem.virt_addr[0] = input_ptr->dst2_frame.img_addr_vir.chn0;
	isp_raw_mem.size = (input_ptr->dst2_frame.img_size.w*input_ptr->dst2_frame.img_size.h)*3*2/2;
	isp_raw_mem.width = input_ptr->dst2_frame.img_size.w;
	isp_raw_mem.height = input_ptr->dst2_frame.img_size.h;
	isp_raw_mem.fmt = ISP_OUTPUT_RAW10;
	isp_raw_mem.cnt	= 1;
#else
	memcpy(isp_raw_mem.fd, input_ptr->input_ptr->raw_buf_fd, sizeof(cmr_s32) * input_ptr->input_ptr->raw_buf_cnt);
	memcpy(isp_raw_mem.phy_addr, input_ptr->input_ptr->raw_buf_phys_addr, sizeof(unsigned int) * input_ptr->input_ptr->raw_buf_cnt);
	memcpy(isp_raw_mem.virt_addr, input_ptr->input_ptr->raw_buf_virt_addr, sizeof(cmr_u64) * input_ptr->input_ptr->raw_buf_cnt);
	isp_raw_mem.size = input_ptr->input_ptr->raw_buf_size;
	isp_raw_mem.width = input_ptr->input_ptr->raw_buf_width;
	isp_raw_mem.height = input_ptr->input_ptr->raw_buf_height;
	isp_raw_mem.fmt = ISP_OUTPUT_ALTEK_RAW10;
	isp_raw_mem.cnt = input_ptr->input_ptr->raw_buf_cnt;
#endif
	ret = isp_dev_set_rawaddr(cxt->isp_driver_handle, &isp_raw_mem);
	ISP_LOGI("raw10_buf fd 0x%x phy_addr 0x%x virt_addr 0x%x", isp_raw_mem.fd[0],
		isp_raw_mem.phy_addr[0], (cmr_u32)isp_raw_mem.virt_addr[0]);

	memset(&img_mem, 0, sizeof(struct isp_img_mem));
	img_mem.img_fmt = input_ptr->dst_frame.img_fmt;
	img_mem.yaddr = input_ptr->dst_frame.img_addr_phy.chn0;
	img_mem.uaddr = input_ptr->dst_frame.img_addr_phy.chn1;
	img_mem.vaddr = input_ptr->dst_frame.img_addr_phy.chn2;
	img_mem.yaddr_vir = input_ptr->dst_frame.img_addr_vir.chn0;
	img_mem.uaddr_vir = input_ptr->dst_frame.img_addr_vir.chn1;
	img_mem.vaddr_vir = input_ptr->dst_frame.img_addr_vir.chn2;
	img_mem.img_y_fd = input_ptr->dst_frame.img_fd.y;
	img_mem.img_u_fd = input_ptr->dst_frame.img_fd.u;
	img_mem.width = input_ptr->dst_frame.img_size.w;
	img_mem.height = input_ptr->dst_frame.img_size.h;
	ISP_LOGI("yuv_raw fd 0x%x yaddr 0x%lx yaddr_vir 0x%lx", img_mem.img_y_fd, img_mem.yaddr, img_mem.yaddr_vir);
	isp_dev_set_post_yuv_mem(cxt->isp_driver_handle, &img_mem);

	memset(&img_mem, 0, sizeof(struct isp_img_mem));
	img_mem.img_fmt = input_ptr->src_frame.img_fmt;
	img_mem.yaddr = input_ptr->src_frame.img_addr_phy.chn0;
	img_mem.uaddr = input_ptr->src_frame.img_addr_phy.chn1;
	img_mem.vaddr = input_ptr->src_frame.img_addr_phy.chn2;
	img_mem.yaddr_vir = input_ptr->src_frame.img_addr_vir.chn0;
	img_mem.uaddr_vir = input_ptr->src_frame.img_addr_vir.chn1;
	img_mem.vaddr_vir = input_ptr->src_frame.img_addr_vir.chn2;
	img_mem.img_y_fd = input_ptr->src_frame.img_fd.y;
	img_mem.img_u_fd = input_ptr->src_frame.img_fd.u;
	img_mem.width = input_ptr->src_frame.img_size.w;
	img_mem.height = input_ptr->src_frame.img_size.h;
	ISP_LOGI("sns_raw fd 0x%x yaddr 0x%lx yaddr_vir 0x%lx", img_mem.img_y_fd, img_mem.yaddr, img_mem.yaddr_vir);

	isp_dev_set_fetch_src_buf(cxt->isp_driver_handle, &img_mem);

	size.height = input_ptr->src_frame.img_size.h;
	size.width = input_ptr->src_frame.img_size.w;
#if 0
	ret = isp_dev_alloc_highiso_mem(cxt->isp_driver_handle, &isp_raw_mem, &size);
	if (ret) {
		ISP_LOGE("failed to alloc highiso mem %ld", ret);
		goto exit;
	}
#else
	memset(&isp_raw_mem, 0, sizeof(isp_raw_mem));
	isp_raw_mem.fd[0] = input_ptr->input_ptr->highiso_buf_fd;
	isp_raw_mem.phy_addr[0] = input_ptr->input_ptr->highiso_buf_phys_addr;
	isp_raw_mem.virt_addr[0] = input_ptr->input_ptr->highiso_buf_virt_addr;
	isp_raw_mem.cnt = 1;
	isp_raw_mem.size = input_ptr->input_ptr->highiso_buf_size;
	isp_raw_mem.width = input_ptr->input_ptr->raw_buf_width;
	isp_raw_mem.height = input_ptr->input_ptr->raw_buf_height;
#endif

	ret = isp_dev_highiso_mode(cxt->isp_driver_handle, &isp_raw_mem);
	if (ret) {
		ISP_LOGE("failed to set highiso mode %ld", ret);
		goto exit;
	}

#ifdef FPGA_TEST
	resolution_ptr = &input_ptr->resolution_info;
	memset(&scenario_in, 0, sizeof(scenario_in));
	scenario_in.tSensorInfo.ucSensorMode = 0;
	if (input_ptr->src_frame.img_size.w == resolution_ptr->sensor_max_size.w)
		scenario_in.tSensorInfo.ucSensorMode = 0;
	else if (input_ptr->src_frame.img_size.w < resolution_ptr->sensor_max_size.w)
		scenario_in.tSensorInfo.ucSensorMode = 1;
	scenario_in.tSensorInfo.cbc_enabled = 0;
	scenario_in.tSensorInfo.ucSensorMouduleType = 0;//0-sensor1  1-sensor2
	scenario_in.tSensorInfo.uwWidth = input_ptr->src_frame.img_size.w;
	scenario_in.tSensorInfo.uwHeight = input_ptr->src_frame.img_size.h;
	scenario_in.tSensorInfo.udLineTime = resolution_ptr->line_time/10;
	scenario_in.tSensorInfo.uwFrameRate = resolution_ptr->fps.max_fps*100;
	scenario_in.tSensorInfo.nColorOrder = cxt->input_param.init_param.image_pattern;
	scenario_in.tSensorInfo.uwClampLevel = 64;
	scenario_in.tSensorInfo.uwOriginalWidth = resolution_ptr->sensor_max_size.w;
	scenario_in.tSensorInfo.uwOriginalHeight = resolution_ptr->sensor_max_size.h;
	scenario_in.tSensorInfo.uwCropStartX = resolution_ptr->crop.st_x;
	scenario_in.tSensorInfo.uwCropStartY = resolution_ptr->crop.st_y;
	scenario_in.tSensorInfo.uwCropEndX = resolution_ptr->sensor_max_size.w + resolution_ptr->crop.st_x - 1;
	scenario_in.tSensorInfo.uwCropEndY = resolution_ptr->sensor_max_size.h + resolution_ptr->crop.st_y - 1;

	ISP_LOGI("uwOriginal w-h %dx%d", scenario_in.tSensorInfo.uwOriginalWidth, scenario_in.tSensorInfo.uwOriginalHeight);
	ISP_LOGI("image_pattern: %d", scenario_in.tSensorInfo.nColorOrder);

	scenario_in.tScenarioOutBypassFlag.bBypassLV = 0;
	scenario_in.tScenarioOutBypassFlag.bBypassVideo = 1;
	scenario_in.tScenarioOutBypassFlag.bBypassStill = 0;
	scenario_in.tScenarioOutBypassFlag.bBypassMetaData = 0;

	scenario_in.tBayerSCLOutInfo.uwBayerSCLOutWidth = 0;
	scenario_in.tBayerSCLOutInfo.uwBayerSCLOutHeight = 0;
	if (ISP_CAP_MODE_RAW_DATA == cap_mode || ISP_CAP_MODE_HIGHISO_RAW_CAP == cap_mode) {
		scenario_in.tBayerSCLOutInfo.uwBayerSCLOutWidth = 960;//cxt->input_param.init_param.size.w;
		scenario_in.tBayerSCLOutInfo.uwBayerSCLOutHeight = 720;//cxt->input_param.init_param.size.h;
		ISP_LOGI("bayer scaler wxh %dx%d\n", scenario_in.tSensorInfo.uwWidth, scenario_in.tSensorInfo.uwHeight);
	}

	if (SENSOR_PDAF_TYPE3_ENABLE == cxt->pdaf_supported)
		scenario_in.tSensorInfo.cbc_enabled = 1;

	ISP_LOGI("size %dx%d, line time %d frameRate %d", scenario_in.tSensorInfo.uwWidth, scenario_in.tSensorInfo.uwHeight,
		scenario_in.tSensorInfo.udLineTime, scenario_in.tSensorInfo.uwFrameRate);
	ret = isp_dev_cfg_scenario_info(cxt->isp_driver_handle, &scenario_in);
	if (ISP_SUCCESS != ret) {
		ISP_LOGE("isp_dev_cfg_scenario_info error %ld", ret);
		return -1;
	}
	if (SENSOR_PDAF_TYPE3_ENABLE == cxt->pdaf_supported) {
		ret = isp_dev_load_cbc(cxt->isp_driver_handle);
		if (ret) {
			ISP_LOGE("failed to load cbc");
			return ret;
		}
	}

	/*set still image buffer format*/
	memset(&img_buf_param, 0, sizeof(img_buf_param));

	img_buf_param.format = ISP_OUT_IMG_NV12;
	img_buf_param.img_id = ISP_IMG_STILL_CAPTURE;
	img_buf_param.dram_eb = 0;
	img_buf_param.buf_num = 4;
	img_buf_param.width = input_ptr->src_frame.img_size.w;
	img_buf_param.height = input_ptr->src_frame.img_size.h;
	img_buf_param.line_offset = input_ptr->src_frame.img_size.w;
	img_buf_param.addr[0].chn0 = 0x2FFFFFFF;
	img_buf_param.addr[1].chn0 = 0x2FFFFFFF;
	img_buf_param.addr[2].chn0 = 0x2FFFFFFF;
	img_buf_param.addr[3].chn0 = 0x2FFFFFFF;
	ISP_LOGI("set still image buffer param img_id %d w %d h %d", img_buf_param.img_id,
			img_buf_param.width, img_buf_param.height);
	ret = isp_dev_set_img_param(cxt->isp_driver_handle, &img_buf_param);


	/*set preview image buffer format*/
	memset(&img_buf_param, 0, sizeof(img_buf_param));
	if (ISP_CAP_MODE_RAW_DATA == cap_mode || ISP_CAP_MODE_HIGHISO_RAW_CAP == cap_mode) {
		img_buf_param.format = ISP_OUT_IMG_YUY2;
		img_buf_param.img_id = ISP_IMG_PREVIEW;
		img_buf_param.dram_eb = 0;
		img_buf_param.buf_num = 4;
		img_buf_param.width = 960;//cxt->input_param.init_param.size.w;
		img_buf_param.height = 720;//cxt->input_param.init_param.size.h;
		img_buf_param.line_offset = (2 * img_buf_param.width);
		img_buf_param.addr[0].chn0 = 0x2FFFFFFF;
		img_buf_param.addr[1].chn0 = 0x2FFFFFFF;
		img_buf_param.addr[2].chn0 = 0x2FFFFFFF;
		img_buf_param.addr[3].chn0 = 0x2FFFFFFF;
		ISP_LOGI("set preview image buffer param img_id %d w %d h %d", img_buf_param.img_id,
			img_buf_param.width, img_buf_param.height);
		ret = isp_dev_set_img_param(cxt->isp_driver_handle, &img_buf_param);
	}

#endif

#ifndef FPGA_TEST
	ret = isp_dev_cfg_scenario_info(cxt->isp_driver_handle, &scenario_in);
	if (ret) {
		ISP_LOGE("failed to cfg scenatio info %ld", ret);
		goto exit;
	}
#endif
#ifdef FPGA_TEST
	cfg_info.subsample_info.token_id = 0x100;
	cfg_info.subsample_info.buffer_image_size = 320*240;
	cfg_info.subsample_info.offset_ratio_x = 0;
	cfg_info.subsample_info.offset_ratio_y = 0;
	cfg_info.yhis_info.token_id = 0x120;
	cfg_info.yhis_info.yhis_region.border_ratio_x = 100;
	cfg_info.yhis_info.yhis_region.border_ratio_y = 100;
	cfg_info.yhis_info.yhis_region.blk_num_x = 16;
	cfg_info.yhis_info.yhis_region.blk_num_y = 16;
	cfg_info.yhis_info.yhis_region.offset_ratio_x = 0;
	cfg_info.yhis_info.yhis_region.offset_ratio_y = 0;
#else
	memcpy(&cfg_info.hw_cfg.tAntiFlickerInfo, input_ptr.hw_cfg.afl_cfg, sizeof(struct antiflicker_cfg_info));
	memcpy(&cfg_info.hw_cfg.tYHisInfo, input_ptr.hw_cfg.yhis_cfg, sizeof(struct yhis_cfg_info);
	memcpy(&cfg_info.hw_cfg.tSubSampleInfo, input_ptr.hw_cfg.subsample_cfg, sizeof(struct subsample_cfg_info));
#endif
	isp_dev_access_convert_ae_param(&input_ptr->hw_cfg.ae_cfg, &cfg_info.ae_info);
	isp_dev_access_convert_afl_param(&input_ptr->hw_cfg.afl_cfg, &cfg_info.antiflicker_info);
	isp_dev_access_convert_awb_param(&input_ptr->hw_cfg.awb_cfg, &cfg_info.awb_info);
	isp_dev_access_set_af_hw_cfg(&cfg_info.af_info, &input_ptr->hw_cfg.af_cfg);
	ret = isp_dev_cfg_3a_param(cxt->isp_driver_handle, &cfg_info);
	if (ret) {
		ISP_LOGE("failed to cfg 3a %ld", ret);
		goto exit;
	}
//	img_param.img_id = ;//0-preview, 1-video, 2-still capture 3-statistics
#ifdef FPGA_TEST
	dldseq.preview_baisc_dld_seq_length = 4;
	dldseq.preview_baisc_dld_seq[0] = 1;
	dldseq.preview_baisc_dld_seq[1] = 1;
	dldseq.preview_baisc_dld_seq[2] = 1;
	dldseq.preview_baisc_dld_seq[3] = 1;
	dldseq.preview_adv_dld_seq_length = 3;
	dldseq.preview_adv_dld_seq[0] = 1;
	dldseq.preview_adv_dld_seq[1] = 1;
	dldseq.preview_adv_dld_seq[2] = 1;
	dldseq.fast_converge_baisc_dld_seq_length = 2;
	dldseq.fast_converge_baisc_dld_seq[0] = 3;
	dldseq.fast_converge_baisc_dld_seq[1] = 3;
#else
	memcpy(&dldseq, &input_ptr->dldseq, sizeof(struct dld_sequence));
#endif
	ret = isp_dev_cfg_dld_seq(cxt->isp_driver_handle, &dldseq);
	if (ret) {
		ISP_LOGE("failed to cfg_dld_seq");
		goto  exit;
	}

	iso_gain = input_ptr->hw_iso_speed;
	ISP_LOGI("cfg hw_iso_speed:%d", iso_gain);
	ret = isp_dev_cfg_iso_speed(cxt->isp_driver_handle, &iso_gain);
	if (ret) {
		ISP_LOGE("failed to cfg iso speed %ld", ret);
		goto exit;
	}

	awb_gain.r = input_ptr->awb_gain.r;
	awb_gain.g = input_ptr->awb_gain.g;
	awb_gain.b = input_ptr->awb_gain.b;
	ret = isp_dev_cfg_awb_gain(cxt->isp_driver_handle, &awb_gain);
	if (ret) {
		ISP_LOGE("failed to cfg awb gain");
		goto exit;
	}
	awb_gain.r = input_ptr->awb_gain_balanced.r;
	awb_gain.g = input_ptr->awb_gain_balanced.g;
	awb_gain.b = input_ptr->awb_gain_balanced.b;
	ret = isp_dev_cfg_awb_gain_balanced(cxt->isp_driver_handle, &awb_gain);
	if (ret) {
		ISP_LOGE("failed to cfg awb gain balanced");
		goto exit;
	}

	ret = isp_dev_stream_on(cxt->isp_driver_handle);
	isp_dev_access_drammode_takepic(isp_dev_handle, 1);
	/*wait untile receive raw to yuv finish*/
	usleep(800*1000);
	isp_dev_access_drammode_takepic(isp_dev_handle, 0);

	isp_dev_stream_off(cxt->isp_driver_handle);
	isp_dev_stop(cxt->isp_driver_handle);

exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int isp_dev_access_cap_buf_cfg(cmr_handle isp_dev_handle, struct isp_dev_img_param *parm)
{
	cmr_int                                ret = ISP_SUCCESS;
	struct isp_dev_access_context          *cxt = (struct isp_dev_access_context *)isp_dev_handle;
	struct isp_img_mem                     img_mem;
	cmr_u32                                i;

	ISP_CHECK_HANDLE_VALID(isp_dev_handle);
	memset(&img_mem, 0, sizeof(struct isp_img_mem));

	ISP_LOGI("count %d", parm->count);
	for (i = 0; i < parm->count; i++) {
		if (parm->img_fd[i].y == 0 || parm->addr_vir[i].chn0 == 0)
			continue;
		img_mem.img_fmt = parm->img_fmt;
		img_mem.channel_id = parm->channel_id;
		img_mem.base_id = parm->base_id;
		img_mem.is_reserved_buf = parm->is_reserved_buf;
		img_mem.yaddr = parm->addr[i].chn0;
		img_mem.uaddr = parm->addr[i].chn1;
		img_mem.vaddr = parm->addr[i].chn2;
		img_mem.yaddr_vir = parm->addr_vir[i].chn0;
		img_mem.uaddr_vir = parm->addr_vir[i].chn1;
		img_mem.vaddr_vir = parm->addr_vir[i].chn2;
		img_mem.img_y_fd = parm->img_fd[i].y;
		img_mem.img_u_fd = parm->img_fd[i].u;
		img_mem.img_v_fd = parm->img_fd[i].v;
		img_mem.width = parm->img_size.w;
		img_mem.height = parm->img_size.h;
		ISP_LOGI("still fd 0x%0x", parm->img_fd[i].y);
		isp_dev_cfg_cap_buf(cxt->isp_driver_handle, &img_mem);
	}

	return ret;

}

void isp_dev_access_evt_reg(cmr_handle isp_dev_handle, isp_evt_cb isp_event_cb, void *privdata)
{
	cmr_int                                ret = ISP_SUCCESS;
	struct isp_dev_access_context          *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	ISP_LOGI("reg evt callback");
	isp_dev_evt_reg(cxt->isp_driver_handle, isp_event_cb, privdata);
}

void isp_dev_access_cfg_buf_evt_reg(cmr_handle isp_dev_handle, cmr_handle grab_handle, isp_evt_cb isp_event_cb)
{
	cmr_int                                ret = ISP_SUCCESS;
	struct isp_dev_access_context          *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	ISP_LOGI("reg evt callback");
	isp_dev_buf_cfg_evt_reg(cxt->isp_driver_handle, grab_handle,  isp_event_cb);
}

cmr_int isp_dev_access_cfg_awb_param(cmr_handle isp_dev_handle, struct isp3a_awb_hw_cfg *data)
{
	cmr_int                                ret = ISP_SUCCESS;
	struct isp_dev_access_context          *cxt = (struct isp_dev_access_context *)isp_dev_handle;
	struct awb_cfg_info                            awb_param;
	cmr_u32                                i = 0;

	//memcpy(&awb_param, data, sizeof(AWB_CfgInfo));
	awb_param.token_id = data->token_id;
	awb_param.awb_region.blk_num_x = data->region.blk_num_X;
	awb_param.awb_region.blk_num_y = data->region.blk_num_Y;
	awb_param.awb_region.border_ratio_x = data->region.border_ratio_X;
	awb_param.awb_region.border_ratio_y = data->region.border_ratio_Y;
	awb_param.awb_region.offset_ratio_x = data->region.offset_ratio_X;
	awb_param.awb_region.offset_ratio_y = data->region.offset_ratio_Y;
	for (i = 0; i < /*AWB_UCYFACTOR_NUM*/16 ; i++) {
		awb_param.y_factor[i] = data->uc_factor[i];
	}
	for (i = 0; i < 33; i++) {
		awb_param.bbr_factor[i] = data->bbr_factor[i];
	}
	awb_param.r_gain = data->uw_rgain;
	awb_param.g_gain = data->uw_ggain;
	awb_param.b_gain = data->uw_bgain;
	awb_param.cr_shift = data->uccr_shift;
	awb_param.offset_shift = data->uc_offset_shift;
	awb_param.quantize = data->uc_quantize;
	awb_param.damp    = data->uc_damp;
	awb_param.sum_shift = data->uc_sum_shift;
	awb_param.t_his.enable = data->t_his.benable;
	awb_param.t_his.cr_end = data->t_his.ccrend;
	awb_param.t_his.cr_purple = data->t_his.ccrpurple;
	awb_param.t_his.cr_start = data->t_his.ccrstart;
	awb_param.t_his.grass_end = data->t_his.cgrass_end;
	awb_param.t_his.grass_offset = data->t_his.cgrass_offset;
	awb_param.t_his.grass_start = data->t_his.cgrass_start;
	awb_param.t_his.offset_down = data->t_his.coffsetdown;
	awb_param.t_his.offset_up = data->t_his.cooffsetup;
	awb_param.t_his.offset_bbr_w_end = data->t_his.coffset_bbr_w_end;
	awb_param.t_his.offset_bbr_w_start = data->t_his.coffset_bbr_w_start;
	awb_param.t_his.his_interp = data->t_his.dhisinterp;
	awb_param.t_his.damp_grass = data->t_his.ucdampgrass;
	awb_param.t_his.offset_purple = data->t_his.ucoffsetpurple;
	awb_param.t_his.yfac_w = data->t_his.ucyfac_w;
	awb_param.r_linear_gain = data->uwrlinear_gain;
	awb_param.b_linear_gain = data->uwblinear_gain;
	ISP_LOGV("token_id = %d, uccr_shift = %d, uc_damp = %d, uc_offset_shift = %d\n",
		awb_param.token_id, awb_param.cr_shift, awb_param.damp, awb_param.offset_shift);
	ISP_LOGV("uc_quantize = %d\n, uc_sum_shift = %d\n, uwblinear_gain = %d\n, uwrlinear_gain = %d\n,\
		uw_bgain = %d\n, uw_rgain = %d\n, uw_ggain = %d\n",
		awb_param.quantize, awb_param.sum_shift, awb_param.b_linear_gain, awb_param.r_linear_gain,
		awb_param.b_gain, awb_param.r_gain, awb_param.g_gain);
	for (i = 0; i < 16; i++) {
		ISP_LOGV("uc_factor[%d] = %d\n", i, awb_param.y_factor[i]);
	}
	for (i = 0; i < 33; i++) {
		ISP_LOGV("bbr_factor[%d] = %d\n", i, awb_param.bbr_factor[i]);
	}
/*
	ISP_LOGV("region:blk_num_X = %d, blk_num_Y = %d\n, border_ratio_X = %d, border_ratio_Y = %d\n,\
		offset_ratio_X = %d, offset_ratio_Y = %d\n",
		awb_param.tAWBRegion.uwBlkNumX, awb_param.tAWBRegion.uwBlkNumY,
		awb_param.tAWBRegion.uwBorderRatioX, awb_param.tAWBRegion.uwBorderRatioY,
		awb_param.tAWBRegion.uwOffsetRatioX, awb_param.tAWBRegion.uwOffsetRatioY);
	ISP_LOGV("t_his:benable = %d\n ccrend = %d\n ccrpurple = %d\n ccrstart = %d\n \
		cgrass_end = %d\n cgrass_offset = %d\n cgrass_start = %d\n coffsetdown = %d\n \
		coffset_bbr_w_end = %d\n coffset_bbr_w_start = %d\n cooffsetup = %d\n \
		dhisinterp = %d\n ucdampgrass = %d\n ucoffsetpurple = %d\n ucyfac_w = %d\n",
		awb_param.tHis.bEnable, awb_param.tHis.cCrEnd, awb_param.tHis.cCrPurple,
		awb_param.tHis.cCrStart, awb_param.tHis.cGrassEnd, awb_param.tHis.cGrassOffset,
		awb_param.tHis.cGrassStart, awb_param.tHis.cOffsetDown, awb_param.tHis.cOffset_bbr_w_end,
		awb_param.tHis.cOffset_bbr_w_start, awb_param.tHis.cOffsetUp, awb_param.tHis.dHisInterp,
		awb_param.tHis.ucDampGrass, awb_param.tHis.ucOffsetPurPle, awb_param.tHis.ucYFac_w);
*/
	ret = isp_dev_cfg_awb_param(cxt->isp_driver_handle, &awb_param);//TBD

	return ret;
}

cmr_int isp_dev_access_cfg_awb_gain(cmr_handle isp_dev_handle, struct isp_awb_gain *data)
{
	cmr_int                                ret = ISP_SUCCESS;
	struct isp_dev_access_context          *cxt = (struct isp_dev_access_context *)isp_dev_handle;
	struct isp_awb_gain_info               gain;

	gain.r = data->r;
	gain.g = data->g;
	gain.b = data->b;
	ret = isp_dev_cfg_awb_gain(cxt->isp_driver_handle, &gain);

	return ret;
}

cmr_int isp_dev_access_cfg_awb_gain_balanced(cmr_handle isp_dev_handle, struct isp_awb_gain *data)
{
	cmr_int                                ret = ISP_SUCCESS;
	struct isp_dev_access_context          *cxt = (struct isp_dev_access_context *)isp_dev_handle;
	struct isp_awb_gain_info               gain;

	gain.r = data->r;
	gain.g = data->g;
	gain.b = data->b;
	ISP_LOGV("balanced gain %d %d %d", gain.r, gain.g, gain.b);
	ret = isp_dev_cfg_awb_gain_balanced(cxt->isp_driver_handle, &gain);

	return ret;
}

cmr_int isp_dev_access_set_stats_buf(cmr_handle isp_dev_handle, struct isp_statis_buf *buf)
{
	cmr_int                                ret = ISP_SUCCESS;
	struct isp_dev_access_context          *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	ISP_LOGV("E");
	ret = isp_dev_set_statis_buf(cxt->isp_driver_handle, buf);

	return ret;
}

cmr_int isp_dev_access_cfg_af_param(cmr_handle isp_dev_handle, struct isp3a_af_hw_cfg *data)
{
	cmr_int                                ret = ISP_SUCCESS;
	struct isp_dev_access_context          *cxt = (struct isp_dev_access_context *)isp_dev_handle;
	struct af_cfg_info                     af_param;

	memcpy(&af_param, data, sizeof(struct af_cfg_info));
	ret = isp_dev_cfg_af_param(cxt->isp_driver_handle, &af_param);

	return ret;
}

cmr_int isp_dev_access_cfg_iso_speed(cmr_handle isp_dev_handle, cmr_u32 *data)
{
	cmr_int                                ret = ISP_SUCCESS;
	struct isp_dev_access_context          *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	ret = isp_dev_cfg_iso_speed(cxt->isp_driver_handle, data);
	return ret;
}

cmr_int isp_dev_access_get_exif_debug_info(cmr_handle isp_dev_handle, struct debug_info1 *exif_info)
{
	cmr_int                                ret = ISP_SUCCESS;
	struct isp_dev_access_context          *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	ret = isp_dev_get_iq_param(cxt->isp_driver_handle, exif_info, NULL);

	ISP_LOGV("done %ld", ret);
	return ret;
}

cmr_int isp_dev_access_get_img_exif_debug_info(cmr_handle isp_dev_handle, struct debug_info1 *exif_info)
{
	cmr_int                                ret = ISP_SUCCESS;
	struct isp_dev_access_context          *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	ret = isp_dev_get_img_iq_param(cxt->isp_driver_handle, exif_info, NULL);

	ISP_LOGV("done %ld", ret);
	return ret;
}

cmr_int isp_dev_access_set_cfg_otp_info(cmr_handle isp_dev_handle, struct isp_iq_otp_info *data)
{
	cmr_int                                ret = ISP_SUCCESS;
	struct isp_dev_access_context          *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	if (!data) {
		ISP_LOGE("data is null error.");
		return -1;
	}

	ret = isp_dev_cfg_otp_info(cxt->isp_driver_handle, data);

	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int isp_dev_access_get_debug_info(cmr_handle isp_dev_handle, struct debug_info2 *debug_info)
{
	cmr_int                                ret = ISP_SUCCESS;
	struct isp_dev_access_context          *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	ret = isp_dev_get_iq_param(cxt->isp_driver_handle, NULL, debug_info);

	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int isp_dev_access_cfg_sof_info(isp_handle isp_dev_handle, struct isp_sof_cfg_info *data)
{
	cmr_int                                ret = ISP_SUCCESS;
	struct isp_dev_access_context          *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	ret = isp_dev_cfg_sof_info(cxt->isp_driver_handle, data);
	return ret;
}

cmr_int isp_dev_access_drammode_takepic(isp_handle isp_dev_handle, cmr_u32 is_start)
{
	cmr_int                                ret = ISP_SUCCESS;
	struct isp_dev_access_context          *cxt = (struct isp_dev_access_context *)isp_dev_handle;
	ISP_LOGI("is_start %d", is_start);
	ret = isp_dev_drammode_takepic(cxt->isp_driver_handle, is_start);
	return ret;
}

cmr_int isp_dev_access_set_skip_num(isp_handle isp_dev_handle, cmr_u32 skip_num)
{
	cmr_int                                ret = ISP_SUCCESS;
	struct isp_dev_access_context          *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	ret = isp_dev_set_skip_num(cxt->isp_driver_handle, skip_num);
	return ret;
}
