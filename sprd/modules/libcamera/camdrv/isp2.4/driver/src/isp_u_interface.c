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

#include "isp_drv.h"
#include "cmr_common.h"

typedef cmr_s32(*isp_cfg_fun_ptr) (cmr_handle handle, void *param_ptr);

struct isp_cfg_fun {
	cmr_u32 sub_block;
	isp_cfg_fun_ptr cfg_fun;
};

static struct isp_cfg_fun s_isp_cfg_fun_tab[] = {
	{ISP_BLK_PRE_GBL_GAIN, isp_u_pgg_block},
	{ISP_BLK_BLC, isp_u_blc_block},
	{ISP_BLK_NLM, isp_u_nlm_block},
#ifdef NOT_EXIST_IN_ISP2_0
	{ISP_BLK_POSTBLC, isp_u_post_blc_block},
#endif
	{ISP_BLK_RGB_GAIN, isp_u_rgb_gain_block},
#ifdef NOT_EXIST_IN_ISP2_0
	{ISP_BLK_NLC, isp_u_nlc_block},
#endif
	{ISP_BLK_2D_LSC, isp_u_2d_lsc_block},
#ifdef NOT_EXIST_IN_ISP2_0
	{ISP_BLK_1D_LSC, isp_u_1d_lsc_block},
#endif
	{ISP_BLK_BINNING4AWB, isp_u_binning4awb_block},
	{ISP_BLK_BPC, isp_u_bpc_block},
	{ISP_BLK_GRGB, isp_u_grgb_block},
	{ISP_BLK_CFA, isp_u_cfa_block},
	{ISP_BLK_CMC10, isp_u_cmc_block},
	{ISP_BLK_RGB_GAMC, isp_u_gamma_block},
	{ISP_BLK_HSV, isp_u_hsv_block},
	{ISP_BLK_POSTERIZE, isp_u_posterize_block},
	{ISP_BLK_CCE, isp_u_cce_matrix_block},
	{ISP_BLK_CSS, isp_u_css_block},
	{ISP_BLK_BDN, isp_u_bdn_block},
	{ISP_BLK_UVDIV, isp_u_uvd_block},
	{ISP_BLK_YUV_PRECDN, isp_u_yuv_precdn_block},
	{ISP_BLK_RGB_PRECDN, isp_u_frgb_precdn_block},
	{ISP_BLK_BRIGHT, isp_u_brightness_block},
#ifdef NOT_EXIST_IN_ISP2_0
	{ISP_BLK_CONTRAST, isp_u_contrast_block},
#endif
	{ISP_BLK_HIST, isp_u_hist_block},
	{ISP_BLK_HIST2, isp_u_hist2_block},
	{ISP_BLK_EDGE, isp_u_edge_block},
	{ISP_BLK_Y_GAMMC, isp_u_ygamma_block},
	{ISP_BLK_UV_CDN, isp_u_yuv_cdn_block},
	{ISP_BLK_UV_POSTCDN, isp_u_yuv_postcdn_block},
	{ISP_BLK_Y_DELAY, isp_u_ydelay_block},
	{ISP_BLK_SATURATION, isp_u_csa_block},
	{ISP_BLK_HUE, isp_u_hue_block},
	{ISP_BLK_IIRCNR_IIR, isp_u_iircnr_block},
	{ISP_BLK_IIRCNR_YRANDOM, isp_u_yrandom_block},
	{ISP_BLK_AE_NEW, isp_u_raw_aem_block},
	{ISP_BLK_AWB_NEW, isp_u_awbc_block},
	{ISP_BLK_RGB_AFM, isp_u_raw_afm_block},
	{ISP_BLK_UV_PREFILTER, isp_u_prefilter_block},
	{ISP_BLK_YIQ_AFL_V1, isp_u_anti_flicker_block},
	{ISP_BLK_YIQ_AFL_V3, isp_u_anti_flicker_new_block},
#ifdef NOT_EXIST_IN_ISP2_0
	{ISP_BLK_PDAF_CORRECT, isp_u_pdaf_correction},
	{ISP_BLK_3DNR_PRE,isp_u_3dnr_pre_block},
	{ISP_BLK_3DNR_CAP,isp_u_3dnr_cap_block},
	{ISP_BLK_YUV_NOISEFILTER, isp_u_noise_filter_block},
	{ISP_BLK_YNR, isp_u_ynr_block},
	{ISP_BLK_RGB_DITHER,isp_u_rgb_dither_block},
#endif
};

cmr_s32 isp_cfg_block(cmr_handle handle, void *param_ptr, cmr_u32 sub_block)
{
	cmr_s32 ret = ISP_SUCCESS;
	cmr_u32 i = 0, cnt = 0;
	isp_cfg_fun_ptr cfg_fun_ptr = PNULL;

	cnt = sizeof(s_isp_cfg_fun_tab) / sizeof(s_isp_cfg_fun_tab[0]);
	for (i = 0; i < cnt; i++) {
		if (sub_block == s_isp_cfg_fun_tab[i].sub_block) {
			cfg_fun_ptr = s_isp_cfg_fun_tab[i].cfg_fun;
			break;
		}
	}

	if (PNULL != cfg_fun_ptr) {
		ret = cfg_fun_ptr(handle, param_ptr);
	}

	return ret;
}

cmr_u32 isp_get_cfa_default_param(struct isp_drv_interface_param * isp_context_ptr, struct isp_dev_cfa_info * cfa_param)
{
	cmr_s32 ret = ISP_SUCCESS;
	UNUSED(isp_context_ptr);
	UNUSED(cfa_param);

#ifdef NOT_EXIST_IN_ISP2_0
	cfa_param->bypass = 0;

	cfa_param->grid_thr = 500;
	cfa_param->min_grid_new = 500;

	cfa_param->grid_gain_new = 4;
	cfa_param->strong_edge_thr = 127;
	cfa_param->weight_control_bypass = 1;
	cfa_param->uni_dir_intplt_thr_new = 20;

	cfa_param->smooth_area_thr = 0;
	cfa_param->cdcr_adj_factor = 8;

	cfa_param->readblue_high_sat_thr = 280;
	cfa_param->grid_dir_weight_t1 = 8;
	cfa_param->grid_dir_weight_t2 = 8;

	cfa_param->low_lux_03_thr = 100;
	cfa_param->round_diff_03_thr = 100;
	cfa_param->low_lux_12_thr = 200;
	cfa_param->round_diff_12_thr = 200;

	cfa_param->css_bypass = 0;
	cfa_param->css_edge_thr = 150;
	cfa_param->css_weak_edge_thr = 100;
	cfa_param->css_texture1_thr = 20;
	cfa_param->css_texture2_thr = 5;

	cfa_param->css_uv_val_thr = 15;
	cfa_param->css_uv_diff_thr = 30;
	cfa_param->css_gray_thr = 40;

	cfa_param->css_pix_similar_thr = 200;
	cfa_param->css_green_edge_thr = 0;
	cfa_param->css_green_weak_edge_thr = 0;

	cfa_param->css_green_tex1_thr = 0;
	cfa_param->css_green_tex2_thr = 5;
	cfa_param->css_green_flat_thr = 0;
	cfa_param->css_edge_corr_ratio_r = 120;
	cfa_param->css_edge_corr_ratio_b = 80;
	cfa_param->css_text1_corr_ratio_r = 0;
	cfa_param->css_text1_corr_ratio_b = 0;
	cfa_param->css_text2_corr_ratio_r = 50;
	cfa_param->css_text2_corr_ratio_b = 50;
	cfa_param->css_flat_corr_ratio_r = 0;
	cfa_param->css_flat_corr_ratio_b = 0;
	cfa_param->css_wedge_corr_ratio_r = 100;
	cfa_param->css_wedge_corr_ratio_b = 60;

	cfa_param->css_alpha_for_tex2 = 8;
	cfa_param->css_skin_u_top[0] = 508;
	cfa_param->css_skin_u_down[0] = 308;
	cfa_param->css_skin_v_top[0] = 636;
	cfa_param->css_skin_v_down[0] = 532;
	cfa_param->css_skin_u_top[1] = 511;
	cfa_param->css_skin_u_down[1] = 461;
	cfa_param->css_skin_v_top[1] = 557;
	cfa_param->css_skin_v_down[1] = 517;
#endif

	return ret;
}

cmr_s32 isp_set_arbiter(cmr_handle handle)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct isp_drv_interface_param *isp_context_ptr = (struct isp_drv_interface_param *)handle;
	struct isp_dev_arbiter_info_v1 *isp_arbiter_ptr = &isp_context_ptr->arbiter;

	if (ISP_SIMULATION_MODE == isp_context_ptr->data.input) {
		isp_arbiter_ptr->endian_ch0.fetch_endian = ISP_ENDIAN_BIG;
	} else {
		isp_arbiter_ptr->endian_ch0.fetch_endian = ISP_ENDIAN_LITTLE;
	}
	/*to be fixed*/
	isp_arbiter_ptr->endian_ch0.bpc_endian = ISP_ENDIAN_LITTLE;
	isp_arbiter_ptr->endian_ch0.lens_endian = ISP_ENDIAN_BIG;
	isp_arbiter_ptr->endian_ch0.store_endian = ISP_ENDIAN_BIG;
	if (isp_context_ptr->data.input ==ISP_EMC_MODE || isp_context_ptr->data.input ==ISP_SIMULATION_MODE)
		isp_arbiter_ptr->endian_ch0.store_endian = ISP_ENDIAN_LITTLE;
	isp_arbiter_ptr->endian_ch0.fetch_bit_reorder = ISP_ZERO;

	return ret;
}

cmr_s32 isp_set_dispatch(cmr_handle handle)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct isp_drv_interface_param *isp_context_ptr = (struct isp_drv_interface_param *)handle;
	struct isp_dev_dispatch_info_v1 *isp_dispatch_ptr = &isp_context_ptr->dispatch;

	isp_dispatch_ptr->bayer_ch0 = isp_context_ptr->data.format_pattern;

	return ret;
}

static enum isp_fetch_format isp_get_fetch_format(enum isp_format in_format)
{
	enum isp_fetch_format format = ISP_FETCH_FORMAT_MAX;

	switch (in_format) {
	case ISP_DATA_YUV422_3FRAME:
		format = ISP_FETCH_YUV422_3FRAME;
		break;
	case ISP_DATA_YUYV:
		format = ISP_FETCH_YUYV;
		break;
	case ISP_DATA_UYVY:
		format = ISP_FETCH_UYVY;
		break;
	case ISP_DATA_YVYU:
		format = ISP_FETCH_YVYU;
		break;
	case ISP_DATA_VYUY:
		format = ISP_FETCH_VYUY;
		break;
	case ISP_DATA_YUV422_2FRAME:
		format = ISP_FETCH_YUV422_2FRAME;
		break;
	case ISP_DATA_YVU422_2FRAME:
		format = ISP_FETCH_YVU422_2FRAME;
		break;
	case ISP_DATA_NORMAL_RAW10:
		format = ISP_FETCH_NORMAL_RAW10;
		break;
	case ISP_DATA_CSI2_RAW10:
		format = ISP_FETCH_CSI2_RAW10;
		break;
	default:
		break;
	}

	return format;
}

static cmr_s32 isp_get_fetch_pitch(struct isp_pitch *pitch_ptr, cmr_u16 width, enum isp_format format)
{
	cmr_s32 ret = ISP_SUCCESS;

	pitch_ptr->chn0 = ISP_ZERO;
	pitch_ptr->chn1 = ISP_ZERO;
	pitch_ptr->chn2 = ISP_ZERO;

	switch (format) {
	case ISP_DATA_YUV422_3FRAME:
		pitch_ptr->chn0 = width;
		pitch_ptr->chn1 = width >> ISP_ONE;
		pitch_ptr->chn2 = width >> ISP_ONE;
		break;
	case ISP_DATA_YUV422_2FRAME:
	case ISP_DATA_YVU422_2FRAME:
		pitch_ptr->chn0 = width;
		pitch_ptr->chn1 = width;
		break;
	case ISP_DATA_YUYV:
	case ISP_DATA_UYVY:
	case ISP_DATA_YVYU:
	case ISP_DATA_VYUY:
		pitch_ptr->chn0 = width << ISP_ONE;
		break;
	case ISP_DATA_NORMAL_RAW10:
		pitch_ptr->chn0 = width << ISP_ONE;
		break;
	case ISP_DATA_CSI2_RAW10:
		pitch_ptr->chn0 = (width * 5) >> ISP_TWO;
		break;
	default:
		break;
	}

	return ret;
}

cmr_s32 isp_get_fetch_addr(struct isp_drv_interface_param * isp_context_ptr, struct isp_dev_fetch_info_v1 * fetch_ptr)
{
	cmr_s32 ret = ISP_SUCCESS;
	cmr_u16 fetch_width = fetch_ptr->size.width;
	cmr_u32 start_col = ISP_ZERO;
	cmr_u32 end_col = ISP_ZERO;
	cmr_u32 mipi_word_num_start[16] = { 0,
		1, 1, 1, 1,
		2, 2, 2,
		3, 3, 3,
		4, 4, 4,
		5, 5
	};
	cmr_u32 mipi_word_num_end[16] = { 0,
		2, 2, 2, 2,
		3, 3, 3, 3,
		4, 4, 4, 4,
		5, 5, 5
	};
	UNUSED(isp_context_ptr);
	end_col = start_col + fetch_width - ISP_ONE;

	fetch_ptr->mipi_byte_rel_pos = start_col & 0x0f;
	fetch_ptr->mipi_word_num = (((end_col + 1) >> 4) * 5 + mipi_word_num_end[(end_col + 1) & 0x0f]) -
	    (((start_col + 1) >> 4) * 5 + mipi_word_num_start[(start_col + 1) & 0x0f]) + 1;

	return ret;
}

cmr_s32 isp_set_fetch_param(cmr_handle handle)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct isp_drv_interface_param *isp_context_ptr = (struct isp_drv_interface_param *)handle;
	struct isp_dev_fetch_info_v1 *fetch_param_ptr = &isp_context_ptr->fetch;

	fetch_param_ptr->bypass = ISP_ZERO;
	fetch_param_ptr->subtract = ISP_ZERO;
	fetch_param_ptr->size.width = isp_context_ptr->data.input_size.w;
	fetch_param_ptr->size.height = isp_context_ptr->data.input_size.h;
	fetch_param_ptr->addr.chn0 = isp_context_ptr->data.input_addr.chn0;
	fetch_param_ptr->addr.chn1 = isp_context_ptr->data.input_addr.chn1;
	fetch_param_ptr->addr.chn2 = isp_context_ptr->data.input_addr.chn2;
	fetch_param_ptr->addr.img_fd = isp_context_ptr->data.input_addr.img_fd;

	fetch_param_ptr->color_format = isp_get_fetch_format(isp_context_ptr->data.input_format);
	isp_get_fetch_pitch((struct isp_pitch *)&(fetch_param_ptr->pitch),
		isp_context_ptr->data.input_size.w,
		isp_context_ptr->data.input_format);
	ISP_LOGI("fetch format %d pitch %d, y addr %x img_fd is%x\n",
		fetch_param_ptr->color_format,
		fetch_param_ptr->pitch.chn0,
		fetch_param_ptr->addr.chn0,
		isp_context_ptr->data.input_addr.img_fd);

	return ret;
}

static enum isp_store_format isp_get_store_format(enum isp_format in_format)
{
	enum isp_store_format format = ISP_STORE_FORMAT_MAX;

	switch (in_format) {
	case ISP_DATA_UYVY:
		format = ISP_STORE_UYVY;
		break;
	case IMG_DATA_TYPE_YUV420:
		format = ISP_STORE_YVU420_2FRAME;
		break;
	case ISP_DATA_YVU422_2FRAME:
		format = ISP_STORE_YVU422_2FRAME;
		break;
	case ISP_DATA_YUV422_3FRAME:
		format = ISP_STORE_YUV422_3FRAME;
		break;
	case ISP_DATA_YUV420_2FRAME:
		format = ISP_STORE_YUV420_2FRAME;
		break;
	case ISP_DATA_YVU420_2FRAME:
		format = ISP_STORE_YVU420_2FRAME;
		break;
	case ISP_DATA_YUV420_3_FRAME:
		format = ISP_STORE_YUV420_3FRAME;
		break;
	default:
		break;
	}

	return format;
}

static cmr_s32 isp_get_store_pitch(struct isp_pitch *pitch_ptr, cmr_u16 width, enum isp_format format)
{
	cmr_s32 ret = ISP_SUCCESS;

	pitch_ptr->chn0 = ISP_ZERO;
	pitch_ptr->chn1 = ISP_ZERO;
	pitch_ptr->chn2 = ISP_ZERO;

	switch (format) {
	case ISP_DATA_YUV422_3FRAME:
	case ISP_DATA_YUV420_3_FRAME:
		pitch_ptr->chn0 = width;
		pitch_ptr->chn1 = width >> ISP_ONE;
		pitch_ptr->chn2 = width >> ISP_ONE;
		break;
	case ISP_DATA_YUV422_2FRAME:
	case ISP_DATA_YVU422_2FRAME:
	case ISP_DATA_YUV420_2FRAME:
	case ISP_DATA_YVU420_2FRAME:
		pitch_ptr->chn0 = width;
		pitch_ptr->chn1 = width;
		break;
	case ISP_DATA_UYVY:
		pitch_ptr->chn0 = width << ISP_ONE;
		break;
	default:
		break;
	}

	return ret;
}

cmr_s32 isp_set_store_param(cmr_handle handle)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct isp_drv_interface_param *isp_context_ptr = (struct isp_drv_interface_param *)handle;
	struct isp_dev_store_info_v1 *store_param_ptr = &isp_context_ptr->store;

	store_param_ptr->bypass = 0;
	store_param_ptr->border.right_border = 0;
	store_param_ptr->border.left_border = 0;
	store_param_ptr->border.up_border = 0;
	store_param_ptr->border.down_border = 0;

	store_param_ptr->color_format = isp_get_store_format(isp_context_ptr->data.output_format);
	store_param_ptr->addr.chn0 = isp_context_ptr->data.output_addr.chn0;
	store_param_ptr->addr.chn1 = isp_context_ptr->data.output_addr.chn1;
	store_param_ptr->addr.chn2 = isp_context_ptr->data.output_addr.chn2;
	store_param_ptr->addr.img_fd = isp_context_ptr->data.output_addr.img_fd;
	ISP_LOGI("isp store dst addr is %x, fmt is %x, in fmt is %x\n",
		store_param_ptr->addr.chn0, store_param_ptr->color_format,
		isp_context_ptr->data.output_format);
	isp_get_store_pitch((struct isp_pitch *)&(store_param_ptr->pitch),
		isp_context_ptr->data.input_size.w, isp_context_ptr->data.output_format);

	return ret;
}

cmr_s32 isp_set_slice_size(cmr_handle handle)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct isp_drv_interface_param *isp_context_ptr = (struct isp_drv_interface_param *)handle;
	struct isp_drv_slice_param *isp_slice_ptr = &isp_context_ptr->slice;
	cmr_s32 i = 0;

	for (i = 0; i < ISP_DRV_SLICE_TYPE_MAX; i++) {
		isp_slice_ptr->size[i].x = 0;
		isp_slice_ptr->size[i].y = 0;
		isp_slice_ptr->size[i].w = isp_context_ptr->data.input_size.w;
		isp_slice_ptr->size[i].h = isp_context_ptr->data.input_size.h;
	}

	return ret;
}

cmr_s32 isp_cfg_slice_size(cmr_handle handle, struct isp_drv_slice_param *slice_ptr)
{
	cmr_s32 ret = ISP_SUCCESS;

#ifdef NOT_EXIST_IN_ISP2_0
	ret = isp_u_1d_lsc_slice_size(handle,
				      slice_ptr->size[ISP_DRV_LSC].w,
				      slice_ptr->size[ISP_DRV_LSC].h);
	ISP_RETURN_IF_FAIL(ret, ("fail to cfg isp 1d lsc slice size"));
#endif
	ret = isp_u_2d_lsc_slice_size(handle,
				      slice_ptr->size[ISP_DRV_LENS].w,
				      slice_ptr->size[ISP_DRV_LENS].h);
	ISP_RETURN_IF_FAIL(ret, ("fail to cfg isp 2d lsc slice size"));

	ret = isp_u_bdn_slice_size(handle, slice_ptr->size[ISP_DRV_BDN].w,
					slice_ptr->size[ISP_DRV_BDN].h);
	ISP_RETURN_IF_FAIL(ret, ("isp bdn slice size error"));

	ret = isp_u_raw_aem_slice_size(handle,
				       slice_ptr->size[ISP_DRV_AEM].w,
				       slice_ptr->size[ISP_DRV_AEM].h);
	ISP_RETURN_IF_FAIL(ret, ("fail to cfg isp raw aem slice size"));

	ret = isp_u_raw_afm_slice_size(handle,
				       slice_ptr->size[ISP_DRV_RGB_AFM].w,
				       slice_ptr->size[ISP_DRV_RGB_AFM].h);
	ISP_RETURN_IF_FAIL(ret, ("fail to cfg isp raw afm slice size"));

	ret = isp_u_hist_slice_size(handle,
				    slice_ptr->size[ISP_DRV_HISTS].w,
				    slice_ptr->size[ISP_DRV_HISTS].h);
	ISP_RETURN_IF_FAIL(ret, ("fail to cfg isp hist slice size"));

	ret = isp_u_dispatch_ch0_size(handle,
				      slice_ptr->size[ISP_DRV_DISPATCH].w,
				      slice_ptr->size[ISP_DRV_DISPATCH].h);
	ISP_RETURN_IF_FAIL(ret, ("fail to cfg isp dispatch ch0 size"));

	ret = isp_u_fetch_slice_size(handle,
				     slice_ptr->size[ISP_DRV_STORE].w,
				     slice_ptr->size[ISP_DRV_STORE].h);
	ISP_RETURN_IF_FAIL(ret, ("fail to cfg isp fetch slice size"));

	ret = isp_u_store_slice_size(handle,
				     slice_ptr->size[ISP_DRV_STORE].w,
				     slice_ptr->size[ISP_DRV_STORE].h);
	ISP_RETURN_IF_FAIL(ret, ("fail to cfg isp store slice size"));

	return ret;
}

cmr_s32 isp_set_comm_param(cmr_handle handle)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct isp_drv_interface_param *isp_context_ptr = (struct isp_drv_interface_param *)handle;
	struct isp_dev_common_info_v1 *com_param_ptr = &isp_context_ptr->com;

	if (ISP_EMC_MODE == isp_context_ptr->data.input) {
		com_param_ptr->fetch_sel_0 = 0x2;
		com_param_ptr->store_sel_0 = 0x2;
		com_param_ptr->fetch_sel_1 = 0x3;
		com_param_ptr->store_sel_1 = 0x3;
	} else if (ISP_SIMULATION_MODE == isp_context_ptr->data.input) {
		com_param_ptr->fetch_sel_0 = 0x2;
		com_param_ptr->store_sel_0 = 0x2;
		com_param_ptr->fetch_sel_1 = 0x3;
		com_param_ptr->store_sel_1 = 0x3;
	} else if (ISP_CAP_MODE == isp_context_ptr->data.input) {
		com_param_ptr->fetch_sel_0 = 0x0;
		com_param_ptr->store_sel_0 = 0x0;
		com_param_ptr->fetch_sel_1 = 0x3;
		com_param_ptr->store_sel_1 = 0x3;
	}
	com_param_ptr->fetch_color_format = 0;
	com_param_ptr->store_color_format = 2;


	com_param_ptr->lbuf_off.ydly_lbuf_offset = 0x121;
	com_param_ptr->lbuf_off.comm_lbuf_offset = 0x480;

	return ret;
}

cmr_s32 isp_cfg_comm_data(cmr_handle handle, struct isp_dev_common_info_v1 * param_ptr)
{
	cmr_s32 ret = ISP_SUCCESS;

	ret = isp_u_comm_block(handle, (void *)param_ptr);
	ISP_RETURN_IF_FAIL(ret, ("store block cfg error"));

	return ret;
}
