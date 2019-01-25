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
#define LOG_TAG "pdaf_adpt"

#include <assert.h>
#include <stdlib.h>
#include <cutils/properties.h>

#include "pdaf_sprd_adpt.h"
#include "pdaf_ctrl.h"
#include "isp_adpt.h"
#include "dlfcn.h"
#include "pd_algo.h"
#include "cmr_common.h"

struct sprd_pdaf_context {
	cmr_u32 camera_id;
	cmr_u8 pd_open;
	cmr_u8 is_busy;
	cmr_u32 frame_id;
	cmr_u32 token_id;
	cmr_u8 pd_enable;
	cmr_handle caller_handle;
	isp_pdaf_cb pdaf_set_cb;
	 cmr_int(*pd_set_buffer) (struct pd_frame_in * cb_param);
	void *caller;
	PD_GlobalSetting pd_gobal_setting;
	struct pdaf_ppi_info ppi_info;
	struct pdaf_roi_info roi_info;
	struct sprd_pdaf_report_t report_data;
	 cmr_u32(*pdaf_set_pdinfo_to_af) (void *handle, struct pd_result * in_parm);
	 cmr_u32(*pdaf_set_cfg_parm) (void *handle, struct isp_dev_pdaf_info * in_parm);
	 cmr_u32(*pdaf_set_bypass) (void *handle, cmr_u32 in_parm);
	 cmr_u32(*pdaf_set_work_mode) (void *handle, cmr_u32 in_parm);
	 cmr_u32(*pdaf_set_skip_num) (void *handle, cmr_u32 in_parm);
	 cmr_u32(*pdaf_set_ppi_info) (void *handle, struct pdaf_ppi_info * in_parm);
	 cmr_u32(*pdaf_set_roi) (void *handle, struct pdaf_roi_info * in_parm);
	 cmr_u32(*pdaf_set_extractor_bypass) (void *handle, cmr_u32 in_parm);
};

#define PDAF_PATTERN_COUNT	 8
#ifdef CONFIG_ISP_2_5
struct isp_dev_pdaf_info pdafTestCase[] = {
	//bypass,  corrector_bypass      phase_map_corr_en; block_size; grid_mode;win;block;gain_upperbound;phase_txt_smooth;phase_gfilter;phase_flat_smoother;
	//hot_pixel_th[3]dead_pixel_th[3];flat_th;edge_ratio_hv;edge_ratio_rd;edge_ratio_hv_rd;phase_left_addr;phase_right_addr;phase_pitch;
	//pattern_pixel_is_right[64];
	//pattern_pixel_row[64];
	//pattern_pixel_col[64];
	//gain_ori_left[2];gain_ori_right[2];extractor_bypass;mode_sel;skip_num; phase_data_dword_num;
	//pdaf_blc_r;pdaf_blc_b;pdaf_blc_gr;pdaf_blc_gb;phase_cfg_rdy;phase_skip_num_clr;
	{
	 0, {2, 2},{1048, 792, 1048 + 2048, 792 + 1536},
	 {0, 0, 1, 1, 1, 1, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,},
	 {5, 5, 8, 8, 21, 21, 24, 24,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,},
	 {2, 18, 1, 17, 10, 26, 9, 25,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,},
	  1, 0
	 },
};
#else
struct isp_dev_pdaf_info pdafTestCase[] = {
	//bypass,  corrector_bypass      phase_map_corr_en; block_size; grid_mode;win;block;gain_upperbound;phase_txt_smooth;phase_gfilter;phase_flat_smoother;
	//hot_pixel_th[3]dead_pixel_th[3];flat_th;edge_ratio_hv;edge_ratio_rd;edge_ratio_hv_rd;phase_left_addr;phase_right_addr;phase_pitch;
	//pattern_pixel_is_right[64];
	//pattern_pixel_row[64];
	//pattern_pixel_col[64];
	//gain_ori_left[2];gain_ori_right[2];extractor_bypass;mode_sel;skip_num; phase_data_dword_num;
	//pdaf_blc_r;pdaf_blc_b;pdaf_blc_gr;pdaf_blc_gb;phase_cfg_rdy;phase_skip_num_clr;
	{
	 0, 0, 1, {2, 2}, 1, {1048, 792, 1048 + 2048, 792 + 1536}, {24, 24, 4208 - 24, 3120 - 24},
	 {600, 800, 200, 400}, 1, 1, 0,
	 {0, 1, 2}, {255, 511, 765}, 765, 127, 0, 510,
	 0x86000000, 0x86800000,
	 0,
	 {0, 0, 1, 1, 1, 1, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,},
	 {5, 5, 8, 8, 21, 21, 24, 24,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,},
	 {2, 18, 1, 17, 10, 26, 9, 25,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,},
	 {2172, 16332}, {8087, 15728}, 0, 1, 0, 0xB,
	 {511, 1023, 127, 255}, {0, 0}, {0, 0}
	 },
};
#endif

struct isp_dev_pdaf_info *ispGetPdafTestCase(cmr_u32 index)
{
	return &pdafTestCase[index];
}

cmr_int _ispSetPdafParam(void *param, cmr_u32 index)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_s32 i = 0;
	struct isp_dev_pdaf_info *pdaf_info_ptr = (struct isp_dev_pdaf_info *)param;
	struct isp_dev_pdaf_info *pdafTestCase_ptr = ispGetPdafTestCase(index);
#ifndef CONFIG_ISP_2_5
	cmr_u16 phase_left[128] = { 0 }, phase_right[128] = {
	0};
	cmr_u32 data_left[128] = { 0 }, data_right[128] = {
	0};

	pdaf_info_ptr->bypass = 0;
	pdafTestCase_ptr->phase_left_addr = (cmr_u32) & phase_left[0];
	pdafTestCase_ptr->phase_right_addr = (cmr_u32) & phase_right[0];
	pdafTestCase_ptr->data_ptr_left[0] = (cmr_u32) & data_left[0];
	pdafTestCase_ptr->data_ptr_left[1] = 0;
	pdafTestCase_ptr->data_ptr_right[0] = (cmr_u32) & data_right[0];
	pdafTestCase_ptr->data_ptr_right[1] = 0;

	pdaf_info_ptr->corrector_bypass = pdafTestCase_ptr->corrector_bypass;
	pdaf_info_ptr->phase_map_corr_en = pdafTestCase_ptr->phase_map_corr_en;
	pdaf_info_ptr->block_size = pdafTestCase_ptr->block_size;
	pdaf_info_ptr->grid_mode = pdafTestCase_ptr->grid_mode;
	pdaf_info_ptr->win = pdafTestCase_ptr->win;
	pdaf_info_ptr->block = pdafTestCase_ptr->block;
	pdaf_info_ptr->gain_upperbound = pdafTestCase_ptr->gain_upperbound;
	pdaf_info_ptr->phase_txt_smooth = pdafTestCase_ptr->phase_txt_smooth;
	pdaf_info_ptr->phase_gfilter = pdafTestCase_ptr->phase_gfilter;
	pdaf_info_ptr->phase_flat_smoother = pdafTestCase_ptr->phase_flat_smoother;
	pdaf_info_ptr->flat_th = pdafTestCase_ptr->flat_th;
	pdaf_info_ptr->edge_ratio_hv = pdafTestCase_ptr->edge_ratio_hv;
	pdaf_info_ptr->edge_ratio_rd = pdafTestCase_ptr->edge_ratio_rd;
	pdaf_info_ptr->edge_ratio_hv_rd = pdafTestCase_ptr->edge_ratio_hv_rd;
	pdaf_info_ptr->phase_left_addr = pdafTestCase_ptr->phase_left_addr;
	pdaf_info_ptr->phase_right_addr = pdafTestCase_ptr->phase_right_addr;
	pdaf_info_ptr->phase_pitch = pdafTestCase_ptr->phase_pitch;
	pdaf_info_ptr->gain_ori_left[0] = pdafTestCase_ptr->gain_ori_left[0];
	pdaf_info_ptr->gain_ori_left[1] = pdafTestCase_ptr->gain_ori_left[1];
	pdaf_info_ptr->gain_ori_right[0] = pdafTestCase_ptr->gain_ori_right[0];
	pdaf_info_ptr->gain_ori_right[1] = pdafTestCase_ptr->gain_ori_right[1];

	cmr_s32 block_x = (pdaf_info_ptr->win.end_x - pdaf_info_ptr->win.start_x) / (8 << pdaf_info_ptr->block_size.width);
	cmr_s32 block_y = (pdaf_info_ptr->win.end_y - pdaf_info_ptr->win.start_y) / (8 << pdaf_info_ptr->block_size.height);
	cmr_u32 phasepixel_total_num = block_x * block_y * PDAF_PATTERN_COUNT / 2;
	pdaf_info_ptr->phase_data_dword_num = (phasepixel_total_num + 5) / 6;
	pdaf_info_ptr->extractor_bypass = pdafTestCase_ptr->extractor_bypass;
	pdaf_info_ptr->mode_sel = pdafTestCase_ptr->mode_sel;
	pdaf_info_ptr->skip_num = pdafTestCase_ptr->skip_num;
	pdaf_info_ptr->pdaf_blc = pdafTestCase_ptr->pdaf_blc;

	for (i = 0; i < 3; i++) {
		pdaf_info_ptr->hot_pixel_th[i] = pdafTestCase_ptr->hot_pixel_th[i];
		pdaf_info_ptr->dead_pixel_th[i] = pdafTestCase_ptr->dead_pixel_th[i];
	}

	for (i = 0; i < 64; i++) {
		pdaf_info_ptr->pattern_pixel_is_right[i] = pdafTestCase_ptr->pattern_pixel_is_right[i];
		pdaf_info_ptr->pattern_pixel_row[i] = pdafTestCase_ptr->pattern_pixel_row[i];
		pdaf_info_ptr->pattern_pixel_col[i] = pdafTestCase_ptr->pattern_pixel_col[i];
	}

	memset((void *)(uintptr_t) pdaf_info_ptr->phase_left_addr, 0, 0x100);
	memset((void *)(uintptr_t) pdaf_info_ptr->phase_right_addr, 0, 0x100);
#else
	pdaf_info_ptr->bypass = 0;
	pdaf_info_ptr->block_size = pdafTestCase_ptr->block_size;
	pdaf_info_ptr->win = pdafTestCase_ptr->win;
	pdaf_info_ptr->mode = pdafTestCase_ptr->mode;
	pdaf_info_ptr->skip_num = pdafTestCase_ptr->skip_num;

	for (i = 0; i < 64; i++) {
		pdaf_info_ptr->pattern_pixel_is_right[i] = pdafTestCase_ptr->pattern_pixel_is_right[i];
		pdaf_info_ptr->pattern_pixel_row[i] = pdafTestCase_ptr->pattern_pixel_row[i];
		pdaf_info_ptr->pattern_pixel_col[i] = pdafTestCase_ptr->pattern_pixel_col[i];
	}
#endif
	return rtn;
}

cmr_int isp_get_pdaf_default_param(struct isp_dev_pdaf_info * pdaf_param)
{
	cmr_s32 rtn = ISP_SUCCESS;
	_ispSetPdafParam(pdaf_param, 0);

	return rtn;
}

static cmr_int pdaf_setup(cmr_handle pdaf)
{
	cmr_int rtn = ISP_SUCCESS;

	ISP_CHECK_HANDLE_VALID(pdaf);
	struct sprd_pdaf_context *cxt = (struct sprd_pdaf_context *)pdaf;
	/* user for debug */
	/*struct isp_dev_pdaf_info pdaf_param;

	   memset(&pdaf_param, 0x00, sizeof(pdaf_param));
	   rtn = isp_get_pdaf_default_param(&pdaf_param);
	   cxt->pdaf_set_cfg_parm(cxt->caller, &pdaf_param); */
	if (cxt->pdaf_set_bypass) {
		cxt->pdaf_set_bypass(cxt->caller, 0);
	}
	if (cxt->pdaf_set_work_mode) {
		cxt->pdaf_set_work_mode(cxt->caller, 1);
	}
	if (cxt->pdaf_set_skip_num) {
		cxt->pdaf_set_skip_num(cxt->caller, 0);
	}
	if (cxt->pdaf_set_ppi_info) {
		cxt->pdaf_set_ppi_info(cxt->caller, &(cxt->ppi_info));
	}
	if (cxt->pdaf_set_roi) {
		cxt->pdaf_set_roi(cxt->caller, &(cxt->roi_info));
	}
	if (cxt->pdaf_set_extractor_bypass) {
		cxt->pdaf_set_extractor_bypass(cxt->caller, 0);
	}
	return rtn;
}

cmr_s32 pdaf_otp_info_parser(struct pdaf_ctrl_init_in * in_p)
{
	struct sensor_otp_section_info *pdaf_otp_info_ptr = NULL;
	struct sensor_otp_section_info *module_info_ptr = NULL;

	if (NULL != in_p->otp_info_ptr) {
		if (in_p->otp_info_ptr->otp_vendor == OTP_VENDOR_SINGLE) {
			pdaf_otp_info_ptr = in_p->otp_info_ptr->single_otp.pdaf_info;
			module_info_ptr = in_p->otp_info_ptr->single_otp.module_info;
			ISP_LOGV("pass pdaf otp, single cam");
		} else if (in_p->otp_info_ptr->otp_vendor == OTP_VENDOR_SINGLE_CAM_DUAL || in_p->otp_info_ptr->otp_vendor == OTP_VENDOR_DUAL_CAM_DUAL) {
			if (in_p->is_master == 1) {
				pdaf_otp_info_ptr = in_p->otp_info_ptr->dual_otp.master_pdaf_info;
				module_info_ptr = in_p->otp_info_ptr->dual_otp.master_module_info;
				ISP_LOGV("pass pdaf otp, dual cam master");
			} else {
				pdaf_otp_info_ptr = NULL;
				module_info_ptr = NULL;
				ISP_LOGV("dual cam slave pdaf is NULL");
			}
		}
	} else {
		pdaf_otp_info_ptr = NULL;
		module_info_ptr = NULL;
		ISP_LOGE("pdaf otp_info_ptr is NULL");
	}

	if (NULL != pdaf_otp_info_ptr && NULL != module_info_ptr) {
		cmr_u8 *module_info = (cmr_u8 *) module_info_ptr->rdm_info.data_addr;

		if (NULL != module_info) {
			if ((module_info[4] == 0 && module_info[5] == 1)
				|| (module_info[4] == 0 && module_info[5] == 2)
				|| (module_info[4] == 0 && module_info[5] == 3)
				|| (module_info[4] == 0 && module_info[5] == 4)
				|| (module_info[4] == 1 && module_info[5] == 0 && (module_info[0] != 0x53 || module_info[1] != 0x50 || module_info[2] != 0x52 || module_info[3] != 0x44))
				|| (module_info[4] == 2 && module_info[5] == 0)
				|| (module_info[4] == 3 && module_info[5] == 0)
				|| (module_info[4] == 4 && module_info[5] == 0)) {
				ISP_LOGV("pdaf otp map v0.4");
				in_p->pdaf_otp.otp_data = pdaf_otp_info_ptr->rdm_info.data_addr;
				in_p->pdaf_otp.size = pdaf_otp_info_ptr->rdm_info.data_size;
			} else if (module_info[4] == 1 && module_info[5] == 0 && module_info[0] == 0x53 && module_info[1] == 0x50 && module_info[2] == 0x52 && module_info[3] == 0x44) {
				ISP_LOGV("pdaf otp map v1.0");
				in_p->pdaf_otp.otp_data = (void *)((cmr_u8 *) pdaf_otp_info_ptr->rdm_info.data_addr + 1);
				in_p->pdaf_otp.size = pdaf_otp_info_ptr->rdm_info.data_size - 1;
			} else {
				in_p->pdaf_otp.otp_data = NULL;
				in_p->pdaf_otp.size = 0;
				ISP_LOGE("pdaf otp map version error");
			}
		} else {
			in_p->pdaf_otp.otp_data = NULL;
			in_p->pdaf_otp.size = 0;
			ISP_LOGE("pdaf module_info is NULL");
		}
	} else {
		ISP_LOGE("pfaf pdaf_otp_info_ptr = %p, module_info_ptr = %p. Parser fail !", pdaf_otp_info_ptr, module_info_ptr);
	}

	return ISP_SUCCESS;
}

cmr_handle sprd_pdaf_adpt_init(void *in, void *out)
{
	cmr_int ret = ISP_SUCCESS;
	struct pdaf_ctrl_init_in *in_p = (struct pdaf_ctrl_init_in *)in;
	struct sprd_pdaf_context *cxt = NULL;
	struct isp_alg_fw_context *isp_ctx = NULL;
	cmr_u16 i;
	UNUSED(out);

	if (!in_p) {
		ISP_LOGE("fail to check init param %p!!!", in_p);
		ret = ISP_PARAM_NULL;
		goto exit;
	}
	// parser pdaf otp info
	pdaf_otp_info_parser(in_p);

	char otp_pdaf_name[1024];

	char value[PROPERTY_VALUE_MAX];
	property_get("ro.debuggable", value, "0");
	if (!strcmp(value, "1")) {
		// userdebug: replace otp with binary file specific by property
		property_get("debug.isp.pdaf.otp.filename", otp_pdaf_name, "/dev/null");
		if (strcmp(otp_pdaf_name, "/dev/null") != 0) {
			FILE *fp = fopen(otp_pdaf_name, "rb");
			if (fp != NULL) {
				if (in_p->pdaf_otp.otp_data != NULL) {
					in_p->pdaf_otp.size = fread(in_p->pdaf_otp.otp_data, 1, 1024, fp);
				}
				fclose(fp);
			}
		}
	} else {
		// user: replace pdaf otp if /productinfo/otp_pdaf.bin exist
		strcpy(otp_pdaf_name, "/productinfo/otp_pdaf.bin");
		FILE *fp = fopen(otp_pdaf_name, "rb");
		if (fp != NULL) {
			if (in_p->pdaf_otp.otp_data != NULL) {
				in_p->pdaf_otp.size = fread(in_p->pdaf_otp.otp_data, 1, 1024, fp);
			}
			fclose(fp);
		}
	}

	isp_ctx = (struct isp_alg_fw_context *)in_p->caller_handle;
	cxt = (struct sprd_pdaf_context *)malloc(sizeof(*cxt));
	if (NULL == cxt) {
		ISP_LOGE("fail to malloc pdaf");
		ret = -ISP_ALLOC_ERROR;
		goto exit;
	}

	memset(cxt, 0, sizeof(*cxt));
	cxt->caller = in_p->caller;
	cxt->caller_handle = in_p->caller_handle;
	cxt->pdaf_set_pdinfo_to_af = in_p->pdaf_set_pdinfo_to_af;
	cxt->pdaf_set_cfg_parm = in_p->pdaf_set_cfg_param;
	cxt->pdaf_set_bypass = in_p->pdaf_set_bypass;
	cxt->pdaf_set_work_mode = in_p->pdaf_set_work_mode;
	cxt->pdaf_set_skip_num = in_p->pdaf_set_skip_num;
	cxt->pdaf_set_ppi_info = in_p->pdaf_set_ppi_info;
	cxt->pdaf_set_roi = in_p->pdaf_set_roi;
	cxt->pdaf_set_extractor_bypass = in_p->pdaf_set_extractor_bypass;
	/*TBD dSensorID 0:for imx258 1: for OV13855 2: for 3L8 */
	if (SENSOR_VENDOR_IMX258 == in_p->pd_info->vendor_type) {
		cxt->pd_gobal_setting.dSensorMode = SENSOR_ID_0;
	} else if (SENSOR_VENDOR_OV13855 == in_p->pd_info->vendor_type) {
		cxt->pd_gobal_setting.dSensorMode = SENSOR_ID_1;
	} else if (SENSOR_VENDOR_S5K3L8XXM3 == in_p->pd_info->vendor_type) {
		cxt->pd_gobal_setting.dSensorMode = SENSOR_ID_2;
	} else {
		ISP_LOGE("fail to support the sensor:%d\n", in_p->pd_info->vendor_type);
		goto exit;
	}

	ISP_LOGI("PDALGO Init. Sensor Mode[%d] ", cxt->pd_gobal_setting.dSensorMode);

	#ifdef CONFIG_ISP_2_5
	cxt->ppi_info.block_size.height = in_p->pd_info->pd_block_h;
	cxt->ppi_info.block_size.width = in_p->pd_info->pd_block_w;
	for (i=0; i< in_p->pd_info->pd_pos_size * 2; i++) {
		cxt->ppi_info.pattern_pixel_is_right[i] = in_p->pd_info->pd_is_right[i];
		cxt->ppi_info.pattern_pixel_row[i] = in_p->pd_info->pd_pos_row[i];
		cxt->ppi_info.pattern_pixel_col[i] = in_p->pd_info->pd_pos_col[i];
	}

	if (cxt->pd_gobal_setting.dSensorMode ==SENSOR_ID_1 ) {
		cxt->roi_info.win.x = ROI_X_1;
		cxt->roi_info.win.w = ROI_Y_1;
		cxt->roi_info.win.y = ROI_X_1 + ROI_Width;
		cxt->roi_info.win.h = ROI_Y_1 + ROI_Height;
		cxt->pd_gobal_setting.dBeginX = BEGIN_X_1;
		cxt->pd_gobal_setting.dBeginY = BEGIN_Y_1;
	} else if(cxt->pd_gobal_setting.dSensorMode ==SENSOR_ID_2){
		cxt->roi_info.win.x = ROI_X_2;
		cxt->roi_info.win.w = ROI_Y_2;
		cxt->roi_info.win.y = ROI_X_2 + ROI_Width;
		cxt->roi_info.win.h = ROI_Y_2 + ROI_Height;
		cxt->pd_gobal_setting.dBeginX = BEGIN_X_2;
		cxt->pd_gobal_setting.dBeginY = BEGIN_Y_2;
	} else {
		cxt->roi_info.win.x = ROI_X_0;
		cxt->roi_info.win.w = ROI_Y_0;
		cxt->roi_info.win.y = ROI_X_0 + ROI_Width;
		cxt->roi_info.win.h = ROI_Y_0 + ROI_Height;
		cxt->pd_gobal_setting.dBeginX = BEGIN_X_0;
		cxt->pd_gobal_setting.dBeginY = BEGIN_Y_0;
	}
	#else
	cxt->ppi_info.block.start_x = in_p->pd_info->pd_offset_x;
	cxt->ppi_info.block.end_x = in_p->pd_info->pd_end_x;
	cxt->ppi_info.block.start_y = in_p->pd_info->pd_offset_y;
	cxt->ppi_info.block.end_y = in_p->pd_info->pd_end_y;
	cxt->ppi_info.block_size.height = in_p->pd_info->pd_block_h;
	cxt->ppi_info.block_size.width = in_p->pd_info->pd_block_w;
	for (i = 0; i < in_p->pd_info->pd_pos_size * 2; i++) {
		cxt->ppi_info.pattern_pixel_is_right[i] = in_p->pd_info->pd_is_right[i];
		cxt->ppi_info.pattern_pixel_row[i] = in_p->pd_info->pd_pos_row[i];
		cxt->ppi_info.pattern_pixel_col[i] = in_p->pd_info->pd_pos_col[i];
	}

	if (cxt->pd_gobal_setting.dSensorMode == SENSOR_ID_1) {
		cxt->roi_info.win.start_x = ROI_X_1;
		cxt->roi_info.win.start_y = ROI_Y_1;
		cxt->roi_info.win.end_x = ROI_X_1 + ROI_Width;
		cxt->roi_info.win.end_y = ROI_Y_1 + ROI_Height;
		cxt->pd_gobal_setting.dBeginX = BEGIN_X_1;
		cxt->pd_gobal_setting.dBeginY = BEGIN_Y_1;
	} else if (cxt->pd_gobal_setting.dSensorMode == SENSOR_ID_2) {
		cxt->roi_info.win.start_x = ROI_X_2;
		cxt->roi_info.win.start_y = ROI_Y_2;
		cxt->roi_info.win.end_x = ROI_X_2 + ROI_Width;
		cxt->roi_info.win.end_y = ROI_Y_2 + ROI_Height;
		cxt->pd_gobal_setting.dBeginX = BEGIN_X_2;
		cxt->pd_gobal_setting.dBeginY = BEGIN_Y_2;
	} else {
		cxt->roi_info.win.start_x = ROI_X_0;
		cxt->roi_info.win.start_y = ROI_Y_0;
		cxt->roi_info.win.end_x = ROI_X_0 + ROI_Width;
		cxt->roi_info.win.end_y = ROI_Y_0 + ROI_Height;
		cxt->pd_gobal_setting.dBeginX = BEGIN_X_0;
		cxt->pd_gobal_setting.dBeginY = BEGIN_Y_0;
	}
	cmr_s32 block_num_x = (cxt->roi_info.win.end_x - cxt->roi_info.win.start_x) / (8 << cxt->ppi_info.block_size.width);
	cmr_s32 block_num_y = (cxt->roi_info.win.end_y - cxt->roi_info.win.start_y) / (8 << cxt->ppi_info.block_size.height);
	cmr_u32 phasepixel_total_num = block_num_x * block_num_y * in_p->pd_info->pd_pos_size;
	cxt->roi_info.phase_data_write_num = (phasepixel_total_num + 5) / 6;
	#endif
	cxt->pd_gobal_setting.dImageW = in_p->sensor_max_size.w;
	cxt->pd_gobal_setting.dImageH = in_p->sensor_max_size.h;
	cxt->pd_gobal_setting.OTPBuffer = in_p->pdaf_otp.otp_data;
	cxt->pd_gobal_setting.dCalibration = 1;
	cxt->pd_gobal_setting.dOVSpeedup = 1;
	//0: Normal, 1:Mirror+Flip
	cxt->pd_gobal_setting.dSensorSetting = in_p->pd_info->sns_orientation;
	ISP_LOGV("gobal_setting.dSensorSetting = %d\n", cxt->pd_gobal_setting.dSensorSetting);

	property_get("debug.isp.pdaf.otp.dump", otp_pdaf_name, "/dev/null");
	if (strcmp(otp_pdaf_name, "/dev/null") != 0) {
		FILE *fp = fopen(otp_pdaf_name, "wb");
		if (fp != NULL) {
			if (in_p->pdaf_otp.otp_data != NULL) {
				fwrite(in_p->pdaf_otp.otp_data, 1, in_p->pdaf_otp.size, fp);
			}
			fclose(fp);
		}
	}

	ret = PD_Init((void *)&cxt->pd_gobal_setting);

	if (ret) {
		ISP_LOGE("fail to init lib %ld", ret);
		goto exit;
	}

	cxt->is_busy = 0;

	return (cmr_handle) cxt;

  exit:
	if (cxt) {
		free(cxt);
		cxt = NULL;
	}
	return (cmr_handle) cxt;
}

static cmr_s32 sprd_pdaf_adpt_deinit(cmr_handle adpt_handle, void *param, void *result)
{
	struct sprd_pdaf_context *cxt = (struct sprd_pdaf_context *)adpt_handle;
	cmr_s32 ret = ISP_SUCCESS;

	UNUSED(param);
	UNUSED(result);
	if (cxt) {
		/* deinit lib */
		ret = (cmr_s32) PD_Uninit();
		memset(cxt, 0x00, sizeof(*cxt));
		free(cxt);
		cxt = NULL;
	}
	ISP_LOGI("done,ret = %d", ret);

	return ret;
}

static cmr_s32 sprd_pdaf_adpt_process(cmr_handle adpt_handle, void *in, void *out)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct sprd_pdaf_context *cxt = (struct sprd_pdaf_context *)adpt_handle;
	struct pdaf_ctrl_process_in *proc_in = (struct pdaf_ctrl_process_in *)in;
	struct pdaf_ctrl_callback_in callback_in;
	struct pd_result pd_calc_result;
	cmr_s32 dRectX = 0;
	cmr_s32 dRectY = 0;
	cmr_s32 dRectW = 0;
	cmr_s32 dRectH = 0;
	cmr_s32 dBuf_width = 0;
	cmr_s32 dBuf_height = 0;
	cmr_s32 *pPD_left = NULL;
	cmr_s32 *pPD_right = NULL;
	cmr_s32 *pPD_left_rotation = NULL;
	cmr_s32 *pPD_right_rotation = NULL;
	cmr_s32 *pPD_left_final = NULL;
	cmr_s32 *pPD_right_final = NULL;
	cmr_s32 *pPD_left_reorder = NULL;
	cmr_s32 *pPD_right_reorder = NULL;
	cmr_s32 i;
	cmr_u8 *ucOTPBuffer = NULL;
	cmr_s32 otp_orientation = 0;
	cmr_u8 OTPSensorStatus = 0;
	cmr_s32 area_index;

	UNUSED(out);
	if (!in) {
		ISP_LOGE("fail to init param %p!!!", in);
		ret = ISP_PARAM_NULL;
		return ret;
	}

	ISP_CHECK_HANDLE_VALID(adpt_handle);
	memset(&callback_in, 0, sizeof(callback_in));
	memset(&pd_calc_result, 0, sizeof(pd_calc_result));
	cxt->is_busy = 1;
	void *pInPhaseBuf_left = (cmr_s32 *) (cmr_uint) (proc_in->u_addr);
	void *pInPhaseBuf_right = (cmr_s32 *) (cmr_uint) (proc_in->u_addr + ISP_PDAF_STATIS_BUF_SIZE / 2);
	ISP_LOGV("pInPhaseBuf_left = %p", pInPhaseBuf_left);

	if (cxt->pd_gobal_setting.dSensorMode == SENSOR_ID_1) {
		dRectX = ROI_X_1;
		dRectY = ROI_Y_1;
		dRectW = ROI_Width;
		dRectH = ROI_Height;
	} else if (cxt->pd_gobal_setting.dSensorMode == SENSOR_ID_2) {
		dRectX = ROI_X_2;
		dRectY = ROI_Y_2;
		dRectW = ROI_Width;
		dRectH = ROI_Height;
	} else {
		dRectX = ROI_X_0;
		dRectY = ROI_Y_0;
		dRectW = ROI_Width;
		dRectH = ROI_Height;
	}

	ucOTPBuffer = (cmr_u8 *) cxt->pd_gobal_setting.OTPBuffer;

	//Check OTP orientation
	if (ucOTPBuffer != NULL) {
		OTPSensorStatus = (*(ucOTPBuffer - 2936) & 0x0C) >> 2;	//0x0B8C - 0x0014 = 0xB78 = 2936
		if (OTPSensorStatus == 3) {
			otp_orientation = 1;
		}
	}

	pPD_left = (cmr_s32 *) malloc(PD_PIXEL_NUM * sizeof(cmr_s32));
	pPD_right = (cmr_s32 *) malloc(PD_PIXEL_NUM * sizeof(cmr_s32));
	pPD_left_rotation = (cmr_s32 *) malloc(PD_PIXEL_NUM * sizeof(cmr_s32));
	pPD_right_rotation = (cmr_s32 *) malloc(PD_PIXEL_NUM * sizeof(cmr_s32));
	pPD_left_reorder = (cmr_s32 *) malloc(PD_PIXEL_NUM * sizeof(cmr_s32));
	pPD_right_reorder = (cmr_s32 *) malloc(PD_PIXEL_NUM * sizeof(cmr_s32));

	ISP_LOGI("PDALGO Converter. Sensor[%d] OTP[%d] Mode[%d]", cxt->pd_gobal_setting.dSensorSetting, otp_orientation, cxt->pd_gobal_setting.dSensorMode);
	ret = PD_PhaseFormatConverter((cmr_u8 *) pInPhaseBuf_left, (cmr_u8 *) pInPhaseBuf_right, pPD_left, pPD_right, PD_PIXEL_NUM, PD_PIXEL_NUM);

	if (cxt->pd_gobal_setting.dSensorSetting != otp_orientation) {
		for (i = 0; i < PD_PIXEL_NUM; i++) {
			pPD_left_rotation[i] = pPD_left[PD_PIXEL_NUM - i - 1];
			pPD_right_rotation[i] = pPD_right[PD_PIXEL_NUM - i - 1];
		}
		pPD_left_final = pPD_left_rotation;
		pPD_right_final = pPD_right_rotation;
	} else {
		pPD_left_final = pPD_left;
		pPD_right_final = pPD_right;
	}

	//Phase Pixel Reorder: for 3L8
	if (cxt->pd_gobal_setting.dSensorMode == SENSOR_ID_2) {
		dBuf_width = ROI_Width / SENSOR_ID_2_BLOCK_W * 4;
		dBuf_height = ROI_Height / SENSOR_ID_2_BLOCK_H * 4;
		ISP_LOGI("PDALGO Reorder Buf. W[%d] H[%d]", dBuf_width, dBuf_height);
		ret = PD_PhasePixelReorder(pPD_left_final, pPD_right_final, pPD_left_reorder, pPD_right_reorder, dBuf_width, dBuf_height);
	}

	for (area_index = 0; area_index < AREA_LOOP; area_index++) {
		if (cxt->pd_gobal_setting.dSensorMode == SENSOR_ID_2) {
			ret = PD_DoType2((void *)pPD_left_reorder, (void *)pPD_right_reorder, dRectX, dRectY, dRectW, dRectH, area_index);
		} else {
			ret = PD_DoType2((void *)pPD_left_final, (void *)pPD_right_final, dRectX, dRectY, dRectW, dRectH, area_index);
		}

		if (ret) {
			ISP_LOGE("fail to do pd algo.");
			goto exit;
		}
	}
	for (area_index = 0; area_index < AREA_LOOP; area_index++) {
		ret = PD_GetResult(&pd_calc_result.pdConf[area_index], &pd_calc_result.pdPhaseDiff[area_index], &pd_calc_result.pdGetFrameID, &pd_calc_result.pdDCCGain[area_index], area_index);
		if (ret) {
			ISP_LOGE("fail to do get pd_result.");
			goto exit;
		}
	}
	ret = PD_GetResult(&pd_calc_result.pdConf[4], &pd_calc_result.pdPhaseDiff[4], &pd_calc_result.pdGetFrameID, &pd_calc_result.pdDCCGain[4], 4);
	if (ret) {
		ISP_LOGE("fail to do get pd_result.");
		goto exit;
	}
	pd_calc_result.pd_roi_num = AREA_LOOP + 1;
	ISP_LOGV("PD_GetResult pd_calc_result.pdConf[4] = %d, pd_calc_result.pdPhaseDiff[4] = 0x%lf, DCC[4]= %d", pd_calc_result.pdConf[4], pd_calc_result.pdPhaseDiff[4], pd_calc_result.pdDCCGain[4]);
	cxt->pdaf_set_pdinfo_to_af(cxt->caller, &pd_calc_result);
	cxt->frame_id++;
  exit:
	cxt->is_busy = 0;

	free(pPD_left);
	free(pPD_right);
	free(pPD_left_rotation);
	free(pPD_right_rotation);
	free(pPD_left_reorder);
	free(pPD_right_reorder);
	return ret;
}

static cmr_s32 pdafsprd_adpt_set_param(cmr_handle adpt_handle, struct pdaf_ctrl_param_in *in)
{
	cmr_int ret = ISP_SUCCESS;
	struct sprd_pdaf_context *cxt = (struct sprd_pdaf_context *)adpt_handle;
	UNUSED(in);

	ISP_CHECK_HANDLE_VALID(adpt_handle);
	pdaf_setup(cxt);
	return ret;
}

static cmr_int pdafsprd_adpt_get_busy(cmr_handle adpt_handle, struct pdaf_ctrl_param_out *out)
{
	cmr_int ret = ISP_SUCCESS;
	struct sprd_pdaf_context *cxt = (struct sprd_pdaf_context *)adpt_handle;

	ISP_CHECK_HANDLE_VALID(adpt_handle);

	out->is_busy = cxt->is_busy;
	return ret;
}

static cmr_s32 pdafsprd_adpt_disable_pdaf(cmr_handle adpt_handle, struct pdaf_ctrl_param_in *in)
{
	cmr_int ret = ISP_SUCCESS;
	struct sprd_pdaf_context *cxt = (struct sprd_pdaf_context *)adpt_handle;
	UNUSED(in);

	ISP_CHECK_HANDLE_VALID(adpt_handle);
	if (cxt->pdaf_set_bypass) {
		cxt->pdaf_set_bypass(cxt->caller, 1);
	}
	return ret;
}

static cmr_s32 sprd_pdaf_adpt_ioctrl(cmr_handle adpt_handle, cmr_s32 cmd, void *in, void *out)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct pdaf_ctrl_param_in *in_ptr = (struct pdaf_ctrl_param_in *)in;

	ISP_CHECK_HANDLE_VALID(adpt_handle);
	ISP_LOGV("cmd = %d", cmd);

	switch (cmd) {
	case PDAF_CTRL_CMD_SET_PARAM:
		ret = pdafsprd_adpt_set_param(adpt_handle, in_ptr);
		break;
	case PDAF_CTRL_CMD_GET_BUSY:
		ret = pdafsprd_adpt_get_busy(adpt_handle, out);
		break;
	case PDAF_CTRL_CMD_DISABLE_PDAF:
		ret = pdafsprd_adpt_disable_pdaf(adpt_handle, in_ptr);
		break;
	default:
		ISP_LOGE("fail to case cmd = %d", cmd);
		break;
	}

	return ret;
}

struct adpt_ops_type sprd_pdaf_adpt_ops = {
	.adpt_init = sprd_pdaf_adpt_init,
	.adpt_deinit = sprd_pdaf_adpt_deinit,
	.adpt_process = sprd_pdaf_adpt_process,
	.adpt_ioctrl = sprd_pdaf_adpt_ioctrl,
};
