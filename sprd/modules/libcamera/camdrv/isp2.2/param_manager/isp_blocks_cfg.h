/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _ISP_BLOCKS_CFG_H_
#define _ISP_BLOCKS_CFG_H_

#ifdef WIN32
#include <memory.h>
#include <string.h>
#include <malloc.h>
#include "isp_type.h"
#endif

#include "isp_com.h"
#include "isp_pm_com_type.h"
#include "isp_com_alg.h"
#include "smart_ctrl.h"
#include <cutils/properties.h>
#include "isp_video.h"
#include "cmr_types.h"
#include "sprd_isp_r6p10v2.h"

#ifdef	 __cplusplus
extern "C" {
#endif

/*************************************************************************/
#define array_offset(type, member) (intptr_t)(&((type*)0)->member)

#define ISP_PM_LEVEL_DEFAULT 3
#define ISP_PM_CCE_DEFAULT 0

/*************************************************************************/

#define ISP_PM_CMC_MAX_INDEX 9
#define ISP_PM_CMC_SHIFT 18

enum isp_bit_reorder {
	ISP_LSB = 0x00,
};

enum isp_slice_type {
	ISP_FETCH = 0x00,
};

enum isp_slice_type_v1 {
	ISP_LSC_V1 = 0x00,
	ISP_CSC_V1,
	ISP_BDN_V1,
	ISP_PWD_V1,
	ISP_LENS_V1,
	ISP_AEM_V1,
	ISP_Y_AEM_V1,
	ISP_RGB_AFM_V1,
	ISP_Y_AFM_V1,
	ISP_HISTS_V1,
	ISP_DISPATCH_V1,
	ISP_FETCH_V1,
	ISP_STORE_V1,
	ISP_SLICE_TYPE_MAX_V1
};

enum isp_slice_pos_info {
	ISP_SLICE_MID,
	ISP_SLICE_LAST,
};

struct isp_slice_param {
	enum isp_slice_pos_info pos_info;
	cmr_u32 slice_line;
	cmr_u32 complete_line;
	cmr_u32 all_line;
	struct isp_size max_size;
	struct isp_size all_slice_num;
	struct isp_size cur_slice_num;

	struct isp_trim_size size[ISP_SLICE_WIN_NUM];
	cmr_u32 edge_info;
};

struct isp_awbm_param {
	cmr_u32 bypass;
	struct isp_pos win_start;
	struct isp_size win_size;
};

struct isp_ae_grgb_statistic_info {
	cmr_u32 r_info[1024];
	cmr_u32 g_info[1024];
	cmr_u32 b_info[1024];
};

struct isp_binning_statistic_info {
	cmr_u32 *r_info;
	cmr_u32 *g_info;
	cmr_u32 *b_info;
	struct isp_size binning_size;
};

#define ISP_NLC_POINTER_NUM 29
#define ISP_NLC_POINTER_L_NUM 27

struct isp_blc_offset {
	cmr_u16 r;
	cmr_u16 gr;
	cmr_u16 gb;
	cmr_u16 b;
};

struct isp_blc_param {
	struct isp_sample_point_info cur_idx;
	struct isp_dev_blc_info cur;
	struct isp_blc_offset offset[SENSOR_BLC_NUM];
};

struct isp_lsc_info {
	struct isp_sample_point_info cur_idx;
	void *data_ptr;
	void *param_ptr;
	cmr_u32 len;
	cmr_u32 grid;
	cmr_u32 gain_w;
	cmr_u32 gain_h;
};

struct isp_lsc_ctrl_in {
	cmr_u32 image_pattern;
	struct isp_lsc_info *lsc_input;
};

struct isp_lnc_map {
	cmr_u32 ct;
	cmr_u32 grid_mode;
	cmr_u32 grid_pitch;
	cmr_u32 gain_w;
	cmr_u32 gain_h;
	cmr_u32 grid;
	void *param_addr;
	cmr_u32 len;
};

struct isp_lnc_param {
	cmr_u32 update_flag;
	struct isp_dev_lsc_info cur;
	struct isp_sample_point_info cur_index_info;	/*for two lsc parameters to interplate */
	struct isp_lnc_map map;	//current lsc map
	struct isp_lnc_map map_tab[ISP_COLOR_TEMPRATURE_NUM];
	struct isp_size resolution;
	cmr_u32 tab_num;
	cmr_u32 lnc_param_max_size;
};

struct isp_awbc_cfg {
	cmr_u32 r_gain;
	cmr_u32 g_gain;
	cmr_u32 b_gain;
	cmr_u32 r_offset;
	cmr_u32 g_offset;
	cmr_u32 b_offset;
};

struct isp_ae_statistic_info {
	cmr_u32 y[1024];
};

struct isp_ae_param {
	struct isp_ae_statistic_info stat_info;
};

struct isp_af_statistic_info {
	cmr_u64 info[32];
	cmr_u32 info_tshark3[105];
};

struct isp_afm_param {
	struct isp_af_statistic_info cur;
};

struct isp_af_param {
	struct isp_afm_param afm;
};

struct isp_gamma_info {
	cmr_u16 axis[2][26];
	cmr_u8 index[28];
};

struct isp_bright_cfg {
	cmr_u32 factor;
};

struct isp_bright_param {
	cmr_u32 cur_index;
	struct isp_dev_brightness_info cur;
	cmr_u8 bright_tab[16];
	cmr_u8 scene_mode_tab[MAX_SCENEMODE_NUM];
};

//for contrast
struct isp_contrast_cfg {
	cmr_u32 factor;
};

struct isp_contrast_param {
	cmr_u32 cur_index;
	struct isp_dev_contrast_info cur;
	cmr_u8 tab[16];
	cmr_u8 scene_mode_tab[MAX_SCENEMODE_NUM];
};

struct isp_saturation_cfg {
	cmr_u32 factor;
};

struct isp_hue_cfg {
	cmr_u32 factor;
};

struct isp_edge_cfg {
	cmr_u32 factor;
};

struct isp_flash_attrib_param {
	cmr_u32 r_sum;
	cmr_u32 gr_sum;
	cmr_u32 gb_sum;
	cmr_u32 b_sum;
};

struct isp_flash_attrib_cali {
	struct isp_flash_attrib_param global;
	struct isp_flash_attrib_param random;
};

struct isp_flash_info {
	cmr_u32 lum_ratio;
	cmr_u32 r_ratio;
	cmr_u32 g_ratio;
	cmr_u32 b_ratio;
	cmr_u32 auto_flash_thr;
};

struct isp_flash_param {
	struct isp_flash_info cur;
	struct isp_flash_attrib_cali attrib;
};
struct isp_dualflash_param {
	cmr_u8 version;
	cmr_u8 alg_id;
	cmr_u8 flashLevelNum1;
	cmr_u8 flashLevelNum2;
	cmr_u8 preflahLevel1;
	cmr_u8 preflahLevel2;
	cmr_u16 preflashBrightness;
	cmr_u16 brightnessTarget;
	cmr_u16 brightnessTargetMax;
	cmr_u16 foregroundRatioHigh;
	cmr_u16 foregroundRatioLow;
	cmr_u8 flashMask[1024];
	cmr_u16 brightnessTable[1024];
	cmr_u16 rTable[1024];
	cmr_u16 bTable[1024];
};

struct isp_dual_flash_param {
	struct isp_dualflash_param cur;
};

struct isp_dev_bokeh_micro_depth_param{
	cmr_u32 tuning_exist;
	cmr_u32 enable;
	cmr_u32  fir_mode;
	cmr_u32 fir_len;
	cmr_s32 hfir_coeff[7];
	cmr_s32 vfir_coeff[7];
	cmr_u32 fir_channel;
	cmr_u32 fir_cal_mode;
	cmr_s32 fir_edge_factor;
	cmr_u32 depth_mode;
	cmr_u32 smooth_thr;
	cmr_u32 touch_factor;
	cmr_u32 scale_factor;
	cmr_u32 refer_len;
	cmr_u32 merge_factor;
	cmr_u32 similar_factor;
	cmr_u32 similar_coeff[3];
	cmr_u32 tmp_mode;
	cmr_s32 tmp_coeff[8];
	cmr_u32 tmp_thr;
	cmr_u32 reserved[100];
};

struct isp_bokeh_micro_depth_param {
	struct isp_dev_bokeh_micro_depth_param cur;
};

struct isp_anti_flicker_param {
	struct isp_antiflicker_param cur;
};

struct isp_envi_detect_param {
	cmr_u32 enable;
	struct isp_range envi_range[SENSOR_ENVI_NUM];
};

struct isp_pre_global_gain_param {
	struct isp_dev_pre_glb_gain_info cur;
};

struct isp_postblc_param {
	struct isp_dev_post_blc_info cur;
	struct isp_sample_point_info cur_idx;
	struct sensor_blc_offset offset[SENSOR_BLC_NUM];
};

struct isp_pdaf_correction_param {
	struct pdaf_correction_param cur;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_pdaf_extraction_param {
	struct pdaf_extraction_param cur;
};

struct isp_dev_noise_filter_param {
	struct isp_dev_noise_filter_info cur;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_rgb_gain_param {
	struct isp_dev_rgb_gain_info cur;
};

struct isp_nlc_param {
	struct isp_dev_nlc_info cur;
};

struct isp_2d_lsc_param {
	struct isp_sample_point_info cur_index_info;
	struct isp_dev_2d_lsc_info cur;
	struct isp_data_info final_lsc_param;	//store the resulted lsc params
	struct isp_lnc_map map_tab[ISP_COLOR_TEMPRATURE_NUM];
	cmr_u32 tab_num;
	struct isp_lsc_info lsc_info;
	struct isp_size resolution;
	cmr_u32 update_flag;
	cmr_u32 is_init;

	void *tmp_ptr_a;
	void *tmp_ptr_b;
};

struct isp_1d_lsc_param {
	struct isp_sample_point_info cur_index_info;
	struct isp_dev_1d_lsc_info cur;
	struct sensor_1d_lsc_map map[SENSOR_LENS_NUM];
};

struct isp_binning4awb_param {
	struct isp_dev_binning4awb_info cur;
};

struct isp_awb_param {
	cmr_u32 ct_value;
	struct isp_dev_awb_info cur;
	struct isp_awb_statistic_info stat;
	struct isp_data_info awb_statistics[4];
};

struct isp_rgb_aem_param {
	struct isp_dev_raw_aem_info cur;
	struct isp_awb_statistic_info stat;
};

struct isp_rgb_afm_param {
	struct isp_dev_rgb_afm_info cur;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *param_ptr1;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_bpc_param {
	struct isp_dev_bpc_info cur;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_grgb_param {
	struct isp_dev_grgb_info cur;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_ynr_param {
	struct isp_dev_ynr_info cur;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_3d_nr_pre_param {
	struct isp_3dnr_const_param cur;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_3d_nr_cap_param {
	struct isp_3dnr_const_param cur;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_nlm_param {
	cmr_u32 cur_level;
	cmr_u32 level_num;
	struct isp_dev_nlm_info cur;
	struct isp_data_info vst_map;
	struct isp_data_info ivst_map;
	cmr_uint *nlm_ptr;
	cmr_uint *vst_ptr;
	cmr_uint *ivst_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_cfa_param {
	struct isp_dev_cfa_info cur;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_cmc10_param {
	struct isp_dev_cmc10_info cur;
	struct isp_sample_point_info cur_idx_info;
	cmr_u16 matrix[SENSOR_CMC_NUM][SENSOR_CMC_POINT_NUM];
	cmr_u16 result_cmc[SENSOR_CMC_POINT_NUM];
	cmr_u16 reserved;
	cmr_u32 reduce_percent;	//reduce saturation.
};

struct isp_frgb_gamc_param {
	struct isp_dev_gamma_info cur;
	struct sensor_rgbgamma_curve final_curve;
	struct isp_sample_point_info cur_idx;
	struct sensor_rgbgamma_curve curve_tab[SENSOR_GAMMA_NUM];
};

struct isp_cce_param {
	struct isp_dev_cce_info cur;
	/*R/G/B coef to change cce */
	cmr_s32 cur_level[2];
	/*0: color cast, 1: gain offset */
	cmr_u16 cce_coef[2][3];
	cmr_u16 bakup_cce_coef[3];
	cmr_u32 prv_idx;
	cmr_u32 cur_idx;
	cmr_u32 is_specialeffect;
	struct isp_dev_cce_info cce_tab[16];
	struct isp_dev_cce_info specialeffect_tab[MAX_SPECIALEFFECT_NUM];
};

struct isp_cce_uvdiv_param {
	struct isp_dev_uvd_info cur;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_hsv_param {
	struct isp_dev_hsv_info cur;
	struct isp_sample_point_info cur_idx;
	struct isp_data_info final_map;
	struct isp_data_info map[SENSOR_HSV_NUM];
	struct isp_data_info specialeffect_tab[MAX_SPECIALEFFECT_NUM];
};

struct isp_yuv_pre_cdn_param {
	struct isp_dev_yuv_precdn_info cur;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_posterize_param {
	struct isp_dev_posterize_info cur;
	struct isp_dev_posterize_info specialeffect_tab[MAX_SPECIALEFFECT_NUM];
};

struct isp_yiq_afl_param_v1 {
	struct isp_dev_anti_flicker_info cur;
};

struct isp_yiq_afl_param_v3 {
	struct isp_dev_anti_flicker_new_info cur;
};

struct isp_rgb_dither_param {
	struct isp_dev_rgb_dither_info cur;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_hist_param {
	struct isp_dev_hist_info cur;
};

struct isp_hist2_param {
	struct isp_dev_hist2_info cur;
};

struct isp_uv_cdn_param {
	struct isp_dev_yuv_cdn_info cur;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_edge_param {
	struct isp_dev_edge_info cur;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_chrom_saturation_param {
	struct isp_dev_csa_info cur;
	cmr_u32 cur_u_idx;
	cmr_u32 cur_v_idx;
	cmr_u8 tab[2][SENSOR_LEVEL_NUM];
	cmr_u8 scene_mode_tab[2][MAX_SCENEMODE_NUM];
};

struct isp_hue_param {
	struct isp_dev_hue_info cur;
	cmr_u32 cur_idx;
	cmr_s16 tab[SENSOR_LEVEL_NUM];
};

struct isp_uv_postcdn_param {
	struct isp_dev_post_cdn_info cur;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_yuv_ygamma_param {
	struct isp_dev_ygamma_info cur;
	cmr_u32 cur_idx;
	struct isp_sample_point_info cur_idx_weight;
	struct sensor_gamma_curve final_curve;
	struct sensor_gamma_curve curve_tab[SENSOR_GAMMA_NUM];
	struct sensor_gamma_curve specialeffect_tab[MAX_SPECIALEFFECT_NUM];
};

struct isp_ydelay_param {
	struct isp_dev_ydelay_info cur;
};

struct isp_iircnr_iir_param {
	struct isp_dev_iircnr_info cur;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_iircnr_yrandom_param {
	struct isp_dev_yrandom_info cur;
};


struct dcam_blc_param {
	struct sprd_blc_info cur;
	struct isp_sample_point_info cur_idx;
	struct isp_blc_offset offset[SENSOR_BLC_NUM];
};

struct dcam_ae_stat_threshold {
	cmr_u32 r_thr_high;
	cmr_u32 r_thr_low;
	cmr_u32 g_thr_high;
	cmr_u32 g_thr_low;
	cmr_u32 b_thr_high;
	cmr_u32 b_thr_low;
};

struct dcam_ae_statistic_info {
	cmr_u32 sum_r_ue[1024];
	cmr_u32 sum_r_oe[1024];
	cmr_u32 sum_r_ae[1024];
	cmr_u32 cnt_r_ue[1024];
	cmr_u32 cnt_r_oe[1024];

	cmr_u32 sum_g_ue[1024];
	cmr_u32 sum_g_oe[1024];
	cmr_u32 sum_g_ae[1024];
	cmr_u32 cnt_g_ue[1024];
	cmr_u32 cnt_g_oe[1024];

	cmr_u32 sum_b_ue[1024];
	cmr_u32 sum_b_oe[1024];
	cmr_u32 sum_b_ae[1024];
	cmr_u32 cnt_b_ue[1024];
	cmr_u32 cnt_b_oe[1024];
};

struct dcam_rgb_aem_param {
	struct sprd_aem_info cur;
	struct dcam_ae_statistic_info stat;
};

struct isp_context {
	cmr_u32 is_validate;
	cmr_u32 mode_id;


// 3A owner:
//      struct isp_awb_param awb;
	struct isp_ae_param ae;
	struct isp_af_param af;
	struct isp_smart_param smart;
	struct isp_sft_af_param sft_af;
	struct isp_aft_param aft;
	struct isp_alsc_param alsc;

//isp related tuning block
	struct isp_bright_param bright;
	struct isp_contrast_param contrast;
	struct isp_flash_param flash;
	struct isp_dualflash_param dual_flash;
	struct isp_dev_bokeh_micro_depth_param bokeh_micro_depth;
	struct isp_haf_tune_param pdaf_tune;
	struct isp_antiflicker_param anti_flicker;
	struct isp_envi_detect_param envi_detect;

	struct isp_pre_global_gain_param pre_gbl_gain;
	struct isp_blc_param blc;
	struct isp_postblc_param post_blc;
	struct isp_rgb_gain_param rgb_gain;
	struct isp_nlc_param nlc;
	struct isp_2d_lsc_param lsc_2d;
	struct isp_1d_lsc_param lsc_1d;
	struct isp_binning4awb_param binning4awb;
	struct isp_awb_param awb;
	struct isp_rgb_aem_param aem;
	struct isp_rgb_afm_param afm;
	struct isp_bpc_param bpc;

	struct isp_grgb_param grgb;
	struct isp_ynr_param ynr;
	struct isp_pdaf_correction_param pdaf_correct;
	struct isp_pdaf_extraction_param pdaf_extraction;
	struct isp_nlm_param nlm;
	struct isp_cfa_param cfa;
	struct isp_cmc10_param cmc10;
	struct isp_frgb_gamc_param frgb_gamc;

	struct isp_cce_param cce;
	struct isp_hsv_param hsv;
	struct isp_yuv_pre_cdn_param yuv_pre_cdn;
	struct isp_posterize_param posterize;
	struct isp_yiq_afl_param_v1 yiq_afl_v1;
	struct isp_yiq_afl_param_v3 yiq_afl_v3;
	struct isp_rgb_dither_param rgb_dither;

	struct isp_hist_param hist;
	struct isp_hist2_param hist2;
	struct isp_uv_cdn_param uv_cdn;
	struct isp_edge_param edge;
	struct isp_chrom_saturation_param saturation;
	struct isp_hue_param hue;
	struct isp_uv_postcdn_param uv_postcdn;
	struct isp_yuv_ygamma_param yuv_ygamma;
	struct isp_ydelay_param ydelay;
	struct isp_iircnr_iir_param iircnr_iir;
	struct isp_iircnr_yrandom_param iircnr_yrandom;
	struct isp_cce_uvdiv_param uv_div;
	struct isp_dev_noise_filter_param yuv_noisefilter;
	struct isp_3d_nr_pre_param nr_3d_pre;
	struct isp_3d_nr_cap_param nr_3d_cap;

	struct dcam_blc_param dcam_blc;
	struct isp_2d_lsc_param dcam_2d_lsc;
	struct dcam_rgb_aem_param dcam_aem;
};

/*******************************isp_block_com******************************/
cmr_s32 PM_CLIP(cmr_s32 x, cmr_s32 bottom, cmr_s32 top);

cmr_s32 _is_print_log();

cmr_s32 _pm_check_smart_param(struct smart_block_result *block_result, struct isp_range *range, cmr_u32 comp_num, cmr_u32 type);

cmr_s32 _pm_common_rest(void *blk_addr, cmr_u32 size);

cmr_u32 _pm_get_lens_grid_mode(cmr_u32 grid);

cmr_u16 _pm_get_lens_grid_pitch(cmr_u32 grid_pitch, cmr_u32 width, cmr_u32 flag);

cmr_u32 _ispLog2n(cmr_u32 index);

cmr_u32 _pm_calc_nr_addr_offset(cmr_u32 mode_flag, cmr_u32 scene_flag, cmr_u32 * one_multi_mode_ptr);

/*******************************isp_pm_blocks******************************/

cmr_s32 _pm_pre_gbl_gain_init(void *dst_pre_gbl_gain, void *src_pre_gbl_gain, void *param1, void *param2);
cmr_s32 _pm_pre_gbl_gain_set_param(void *pre_gbl_gain_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_pre_gbl_gain_get_param(void *pre_gbl_gain_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_blc_init(void *dst_blc_param, void *src_blc_param, void *param1, void *param_ptr2);
cmr_s32 _pm_blc_set_param(void *blc_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_blc_get_param(void *blc_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_blc_init(void *dst_blc_param, void *src_blc_param, void *param1, void *param_ptr2);
cmr_s32 _pm_blc_set_param(void *blc_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_blc_get_param(void *blc_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_u32 _pm_pdaf_correct_convert_param(void *dst_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag);
cmr_s32 _pm_pdaf_correct_init(void *dst_pdaf_correct_param, void *src_pdaf_correct_param, void *param1, void *param2);
cmr_s32 _pm_pdaf_correct_set_param(void *pdaf_correct_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_pdaf_correct_get_param(void *pdaf_correct_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_pdaf_extraction_init(void *dst_pdaf_extraction_param, void *src_pdaf_extraction_param, void *param1, void *param2);
cmr_s32 _pm_pdaf_extraction_set_param(void *pdaf_extraction_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_pdaf_extraction_get_param(void *pdaf_extraction_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_postblc_init(void *dst_postblc_param, void *src_postblc_param, void *param1, void *param_ptr2);
cmr_s32 _pm_postblc_set_param(void *postblc_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_postblc_get_param(void *postblc_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_rgb_gain_init(void *dst_gbl_gain, void *src_gbl_gain, void *param1, void *param2);
cmr_s32 _pm_rgb_gain_set_param(void *gbl_gain_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_rgb_gain_get_param(void *gbl_gain_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_rgb_dither_init(void *dst_rgb_dither_param, void *src_rgb_dither_param, void *param1, void *param_ptr2);
cmr_s32 _pm_rgb_dither_set_param(void *rgb_dither_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_rgb_dither_get_param(void *rgb_dither_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_u32 _pm_nlm_convert_param(void *dst_nlm_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag);
cmr_s32 _pm_nlm_init(void *dst_nlm_param, void *src_nlm_param, void *param1, void *param_ptr2);
cmr_s32 _pm_nlm_set_param(void *nlm_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_nlm_get_param(void *nlm_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);
cmr_s32 _pm_nlm_deinit(void *nlm_param);

cmr_s32 _pm_2d_lsc_init(void *dst_lnc_param, void *src_lnc_param, void *param1, void *param2);
cmr_s32 _pm_2d_lsc_set_param(void *lnc_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_2d_lsc_get_param(void *lnc_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);
cmr_s32 _pm_2d_lsc_deinit(void *lnc_param);

cmr_s32 _pm_1d_lsc_init(void *dst_lnc_param, void *src_lnc_param, void *param1, void *param2);
cmr_s32 _pm_1d_lsc_set_param(void *lnc_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_1d_lsc_get_param(void *lnc_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_binning4awb_init(void *dst_binning4awb, void *src_binning4awb, void *param1, void *param2);
cmr_s32 _pm_binning4awb_set_param(void *binning4awb_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_binning4awb_get_param(void *binning4awb_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

//awbc
cmr_s32 _pm_awb_init(void *dst_pwd, void *src_pwd, void *param1, void *param2);
cmr_s32 _pm_awb_set_param(void *pwd_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_awb_get_param(void *pwd_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_frgb_gamc_init(void *dst_gamc_param, void *src_gamc_param, void *param1, void *param_ptr2);
cmr_s32 _pm_frgb_gamc_set_param(void *gamc_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_frgb_gamc_get_param(void *gamc_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_u32 _pm_bpc_convert_param(void *dst_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag);
cmr_s32 _pm_bpc_init(void *dst_bpc_param, void *src_bpc_param, void *param1, void *param2);
cmr_s32 _pm_bpc_set_param(void *bpc_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_bpc_get_param(void *bpc_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_u32 _pm_grgb_convert_param(void *dst_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag);
cmr_s32 _pm_grgb_init(void *dst_grgb_param, void *src_grgb_param, void *param1, void *param2);
cmr_s32 _pm_grgb_set_param(void *grgb_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_grgb_get_param(void *grgb_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_u32 _pm_cfa_convert_param(void *dst_cfae_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag);
cmr_s32 _pm_cfa_init(void *dst_cfae_param, void *src_cfae_param, void *param1, void *param_ptr2);
cmr_s32 _pm_cfa_set_param(void *cfae_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_cfa_get_param(void *cfa_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_u32 _pm_rgb_afm_convert_param(void *dst_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag);
cmr_s32 _pm_rgb_afm_init(void *dst_rgb_afm, void *src_rgb_aef, void *param1, void *param2);
cmr_s32 _pm_rgb_afm_set_param(void *rgb_afm_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_rgb_afm_get_param(void *rgb_afm_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_cmc10_init(void *dst_cmc10_param, void *src_cmc10_param, void *param1, void *param_ptr2);
cmr_s32 _pm_cmc10_adjust(struct isp_cmc10_param *cmc_ptr, cmr_u32 is_reduce);
cmr_s32 _pm_cmc10_set_param(void *cmc10_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_cmc10_get_param(void *cmc10_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_yuv_ygamma_init(void *dst_gamc_param, void *src_gamc_param, void *param1, void *param_ptr2);
cmr_s32 _pm_yuv_ygamma_set_param(void *gamc_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_yuv_ygamma_get_param(void *gamc_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_hsv_init(void *dst_hsv_param, void *src_hsv_param, void *param1, void *param2);
cmr_s32 _pm_hsv_set_param(void *hsv_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_hsv_get_param(void *hsv_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);
cmr_s32 _pm_hsv_deinit(void *hsv_param);

cmr_s32 _pm_posterize_init(void *dst_pstrz_param, void *src_pstrz_param, void *param1, void *param_ptr2);
cmr_s32 _pm_posterize_set_param(void *pstrz_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_posterize_get_param(void *pstrz_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_cce_adjust_hue_saturation(struct isp_cce_param *cce_param, cmr_u32 hue, cmr_u32 saturation);
cmr_s32 _pm_cce_adjust_gain_offset(struct isp_cce_param *cce_param, cmr_u16 r_gain, cmr_u16 g_gain, cmr_u16 b_gain);
cmr_s32 _pm_cce_adjust(struct isp_cce_param *cce_param);
cmr_s32 _pm_cce_init(void *dst_cce_param, void *src_cce_param, void *param1, void *param2);
cmr_s32 _pm_cce_set_param(void *cce_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_cce_get_param(void *cce_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_u32 _pm_uv_div_convert_param(void *dst_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag);
cmr_s32 _pm_uv_div_init(void *dst_uv_div_param, void *src_uv_div_param, void *param1, void *param_ptr2);
cmr_s32 _pm_uv_div_set_param(void *uv_div_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_uv_div_get_param(void *uv_div_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_yiq_afl_init_v1(void *dst_afl_param, void *src_afl_param, void *param1, void *param_ptr2);
cmr_s32 _pm_yiq_afl_set_param_v1(void *afl_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_yiq_afl_get_param_v1(void *afl_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_yiq_afl_init_v3(void *dst_afl_param, void *src_afl_param, void *param1, void *param_ptr2);
cmr_s32 _pm_yiq_afl_set_param_v3(void *afl_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_yiq_afl_get_param_v3(void *afl_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_u32 _pm_3d_nr_pre_convert_param(void *dst_3d_nr_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag);
cmr_s32 _pm_3d_nr_pre_init(void *dst_ynr_param, void *src_ynr_param, void *param1, void *param_ptr2);
cmr_s32 _pm_3d_nr_pre_set_param(void *ynr_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_3d_nr_pre_get_param(void *ynr_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_u32 _pm_3d_nr_cap_convert_param(void *dst_3d_nr_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag);
cmr_s32 _pm_3d_nr_cap_init(void *dst_ynr_param, void *src_ynr_param, void *param1, void *param_ptr2);
cmr_s32 _pm_3d_nr_cap_set_param(void *ynr_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_3d_nr_cap_get_param(void *ynr_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_u32 _pm_yuv_precdn_convert_param(void *dst_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag);
cmr_s32 _pm_yuv_precdn_init(void *dst_precdn_param, void *src_precdn_param, void *param1, void *param2);
cmr_s32 _pm_yuv_precdn_set_param(void *precdn_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_yuv_precdn_get_param(void *precdn_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_u32 _pm_ynr_convert_param(void *dst_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag);
cmr_s32 _pm_ynr_init(void *dst_ynr_param, void *src_ynr_param, void *param1, void *param_ptr2);
cmr_s32 _pm_ynr_set_param(void *ynr_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_ynr_get_param(void *ynr_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_brightness_init(void *dst_brightness, void *src_brightness, void *param1, void *param2);
cmr_s32 _pm_brightness_set_param(void *bright_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_brightness_get_param(void *bright_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_contrast_init(void *dst_contrast, void *src_contrast, void *param1, void *param2);
cmr_s32 _pm_contrast_set_param(void *contrast_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_contrast_get_param(void *contrast_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_hist_init(void *dst_hist_param, void *src_hist_param, void *param1, void *param2);
cmr_s32 _pm_hist_set_param(void *hist_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_hist_get_param(void *hist_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_hist2_init(void *dst_hist2_param, void *src_hist2_param, void *param1, void *param2);
cmr_s32 _pm_hist2_set_param(void *hist2_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_hist2_get_param(void *hist2_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_u32 _pm_edge_convert_param(void *dst_edge_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag);
cmr_s32 _pm_edge_init(void *dst_edge_param, void *src_edge_param, void *param1, void *param2);
cmr_s32 _pm_edge_set_param(void *edge_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_edge_get_param(void *edge_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_u32 _pm_uv_cdn_convert_param(void *dst_cdn_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag);
cmr_s32 _pm_uv_cdn_init(void *dst_cdn_param, void *src_cdn_param, void *param1, void *param2);
cmr_s32 _pm_uv_cdn_set_param(void *cdn_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_uv_cdn_get_param(void *cdn_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_u32 _pm_uv_postcdn_convert_param(void *dst_postcdn_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag);
cmr_s32 _pm_uv_postcdn_init(void *dst_postcdn_param, void *src_postcdn_param, void *param1, void *param_ptr2);
cmr_s32 _pm_uv_postcdn_set_param(void *postcdn_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_uv_postcdn_get_param(void *postcdn_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_ydelay_init(void *dst_ydelay, void *src_ydelay, void *param1, void *param2);
cmr_s32 _pm_ydelay_set_param(void *ydelay_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_ydelay_get_param(void *ydelay_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_saturation_init(void *dst_csa_param, void *src_csa_param, void *param1, void *param_ptr2);
cmr_s32 _pm_saturation_set_param(void *csa_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_saturation_get_param(void *csa_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_hue_init(void *dst_hue_param, void *src_hue_param, void *param1, void *param_ptr2);
cmr_s32 _pm_hue_set_param(void *hue_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_hue_get_param(void *hue_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_u32 _pm_iircnr_iir_convert_param(void *dst_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag);
cmr_s32 _pm_iircnr_iir_init(void *dst_iircnr_param, void *src_iircnr_param, void *param1, void *param_ptr2);
cmr_s32 _pm_iircnr_iir_set_param(void *iircnr_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_iircnr_iir_get_param(void *iircnr_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_iircnr_yrandom_init(void *dst_iircnr_param, void *src_iircnr_param, void *param1, void *param_ptr2);
cmr_s32 _pm_iircnr_yrandom_set_param(void *iircnr_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_iircnr_yrandom_get_param(void *iircnr_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_u32 _pm_yuv_noisefilter_convert_param(void *dst_yuv_noisefilter_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag);
cmr_s32 _pm_yuv_noisefilter_init(void *dst_ynr_param, void *src_ynr_param, void *param1, void *param_ptr2);
cmr_s32 _pm_yuv_noisefilter_set_param(void *ynr_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_yuv_noisefilter_get_param(void *ynr_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_flashlight_init(void *dst_flash_param, void *src_flash_param, void *param1, void *param2);
cmr_s32 _pm_flashlight_set_param(void *flash_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_flashlight_get_param(void *flash_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_envi_detect_init(void *dst_envi_detect_param, void *src_envi_detect_param, void *param1, void *param2);
cmr_s32 _pm_envi_detect_set_param(void *envi_detect_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_envi_detect_get_param(void *envi_detect_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_nlc_init(void *dst_nlc_param, void *src_nlc_param, void *param1, void *param2);
cmr_s32 _pm_nlc_set_param(void *nlc_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_nlc_get_param(void *nlc_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_rgb_aem_init(void *dst_rgb_aem, void *src_rgb_aem, void *param1, void *param2);
cmr_s32 _pm_rgb_aem_set_param(void *rgb_aem_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_rgb_aem_get_param(void *rgb_aem_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_csc_init(void *dst_csc_param, void *src_csc_param, void *param1, void *param2);
cmr_s32 _pm_csc_set_param(void *csc_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_csc_get_param(void *csc_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_rgb_afm_init(void *dst_afm_param, void *src_afm_param, void *param1, void *param_ptr2);
cmr_s32 _pm_rgb_afm_set_param(void *afm_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_rgb_afm_get_param(void *afm_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_smart_init(void *dst_smart_param, void *src_smart_param, void *param1, void *param_ptr2);
cmr_s32 _pm_smart_set_param(void *smart_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_smart_get_param(void *smart_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_aft_init(void *dst_aft_param, void *src_aft_param, void *param1, void *param_ptr2);
cmr_s32 _pm_aft_set_param(void *aft_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_aft_get_param(void *aft_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_alsc_init(void *dst_alsc_param, void *src_alsc_param, void *param1, void *param_ptr2);
cmr_s32 _pm_alsc_set_param(void *alsc_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_alsc_get_param(void *alsc_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_sft_af_init(void *dst_sft_af_param, void *src_sft_af_param, void *param1, void *param_ptr2);
cmr_s32 _pm_sft_af_set_param(void *aft_af_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_sft_af_get_param(void *aft_af_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_ae_new_init(void *dst_ae_new_param, void *src_ae_new_param, void *param1, void *param_ptr2);
cmr_s32 _pm_ae_new_set_param(void *ae_new_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_ae_new_get_param(void *ae_new_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_awb_new_init(void *dst_awb_new_param, void *src_awb_new_param, void *param1, void *param_ptr2);
cmr_s32 _pm_awb_new_set_param(void *awb_new_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_awb_new_get_param(void *awb_new_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_af_new_init(void *dst_af_new_param, void *src_af_new_param, void *param1, void *param_ptr2);
cmr_s32 _pm_af_new_set_param(void *af_new_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_af_new_get_param(void *af_new_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_dualflash_init(void *dst_flash_param, void *src_flash_param, void *param1, void *param2);
cmr_s32 _pm_dualflash_set_param(void *flash_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_dualflash_get_param(void *flash_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_pdaf_tune_init(void *dst_pdaf_tune_param, void *src_pdaf_tune_param, void *param1, void *param2);
cmr_s32 _pm_pdaf_tune_set_param(void *pdaf_tune_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_pdaf_tune_get_param(void *pdaf_tune_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

struct isp_block_operations {
	cmr_s32(*init) (void *blk_ptr, void *param_ptr0, void *param_ptr1, void *param_ptr2);
	cmr_s32(*set) (void *blk_ptr, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
	cmr_s32(*get) (void *blk_ptr, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
	cmr_s32(*reset) (void *blk_ptr, cmr_u32 size);
	cmr_s32(*deinit) (void *blk_ptr);
};

struct isp_block_cfg {
	cmr_u32 id;
	cmr_u32 offset;
	cmr_u32 param_size;
	struct isp_block_operations *ops;
};

struct isp_block_cfg *isp_pm_get_block_cfg(cmr_u32 id);

#ifdef	 __cplusplus
}
#endif
#endif
