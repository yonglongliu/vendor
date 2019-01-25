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
#ifndef _ISP_DRV_H_
#define _ISP_DRV_H_
#include <fcntl.h>
#include <errno.h>
#include "sprd_isp_k.h"
#include "sprd_img.h"
#include "cmr_types.h"
#include "sensor_raw.h"
#include "isp_mw.h"

#ifndef LOCAL_INCLUDE_ONLY
#error "Hi, This is only for camdrv."
#endif

#define ISP_DRV_SLICE_WIN_NUM 0x18

struct isp_file {
	cmr_s32 fd;
	cmr_u32 chip_id;
	cmr_u32 isp_id;
	void *reserved;
};

struct isp_statis_mem_info {
	cmr_u32 isp_statis_mem_size;
	cmr_u32 isp_statis_mem_num;
	cmr_uint isp_statis_k_addr[2];
	cmr_uint isp_statis_u_addr;
	cmr_uint isp_statis_alloc_flag;
	cmr_s32 statis_mfd;
	cmr_s32 statis_buf_dev_fd;

	cmr_u32 isp_lsc_mem_size;
	cmr_u32 isp_lsc_mem_num;
	cmr_uint isp_lsc_physaddr;
	cmr_uint isp_lsc_virtaddr;
	cmr_uint isp_lsc_alloc_flag;
	cmr_s32 lsc_mfd;

	void *buffer_client_data;
	cmr_malloc alloc_cb;
	cmr_free free_cb;
	cmr_handle oem_handle;
};

struct isp_statis_info {
	cmr_u32 irq_type;
	cmr_u32 irq_property;
	cmr_u32 phy_addr;
	cmr_u32 vir_addr;
	cmr_u32 addr_offset;
	cmr_u32 kaddr[2];
	cmr_u32 buf_size;
	cmr_s32 mfd;
	cmr_s32 frame_id;
	cmr_u32 sec;
	cmr_u32 usec;
	cmr_s64 monoboottime;
};

struct isp_u_irq_info {
	cmr_s32 frame_id;
	cmr_u32 sec;
	cmr_u32 usec;
};

enum isp_fetch_format {
	ISP_FETCH_YUV422_3FRAME = 0x00,
	ISP_FETCH_YUYV,
	ISP_FETCH_UYVY,
	ISP_FETCH_YVYU,
	ISP_FETCH_VYUY,
	ISP_FETCH_YUV422_2FRAME,
	ISP_FETCH_YVU422_2FRAME,
	ISP_FETCH_NORMAL_RAW10,
	ISP_FETCH_CSI2_RAW10,
	ISP_FETCH_FORMAT_MAX
};

enum isp_store_format {
	ISP_STORE_UYVY = 0x00,
	ISP_STORE_YUV422_2FRAME,
	ISP_STORE_YVU422_2FRAME,
	ISP_STORE_YUV422_3FRAME,
	ISP_STORE_YUV420_2FRAME,
	ISP_STORE_YVU420_2FRAME,
	ISP_STORE_YUV420_3FRAME,
	ISP_STORE_FORMAT_MAX
};

enum isp_work_mode {
	ISP_SINGLE_MODE = 0x00,
	ISP_CONTINUE_MODE,
};

enum isp_match_mode {
	ISP_CAP_MODE = 0x00,
	ISP_EMC_MODE,
	ISP_DCAM_MODE,
	ISP_SIMULATION_MODE,
};

enum isp_endian {
	ISP_ENDIAN_LITTLE = 0x00,
	ISP_ENDIAN_BIG,
	ISP_ENDIAN_HALFBIG,
	ISP_ENDIAN_HALFLITTLE,
	ISP_ENDIAN_MAX
};

enum {
	ISP_SBS_MODE_OFF = 0,
	ISP_SBS_MODE_ON,
	ISP_SBS_MODE_LEFT,
	ISP_SBS_MODE_RIGHT,
	ISP_SBS_MODE_MAX
};

enum isp_drv_slice_type {
	ISP_DRV_LSC = 0x00,
	ISP_DRV_CSC,
	ISP_DRV_BDN,
	ISP_DRV_PWD,
	ISP_DRV_LENS,
	ISP_DRV_AEM,
	ISP_DRV_Y_AEM,
	ISP_DRV_RGB_AFM,
	ISP_DRV_Y_AFM,
	ISP_DRV_HISTS,
	ISP_DRV_DISPATCH,
	ISP_DRV_FETCH,
	ISP_DRV_STORE,
	ISP_DRV_SLICE_TYPE_MAX
};

struct isp_data_param {
	enum isp_work_mode work_mode;
	enum isp_match_mode input;
	enum isp_format input_format;
	cmr_u32 format_pattern;
	struct isp_size input_size;
	struct isp_addr input_addr;
	struct isp_addr input_vir;
	enum isp_format output_format;
	enum isp_match_mode output;
	struct isp_addr output_addr;
	cmr_u32 input_fd;
};

struct isp_drv_slice_param {
	cmr_u32 slice_line;
	cmr_u32 complete_line;
	cmr_u32 all_line;
	struct isp_size max_size;
	struct isp_size all_slice_num;
	struct isp_size cur_slice_num;

	struct isp_trim_size size[ISP_DRV_SLICE_WIN_NUM];
	cmr_u32 edge_info;
};

struct isp_drv_interface_param {
	struct isp_data_param data;

	struct isp_dev_fetch_info_v1 fetch;
	struct isp_dev_store_info_v1 store;
	struct isp_dev_dispatch_info_v1 dispatch;
	struct isp_dev_arbiter_info_v1 arbiter;
	struct isp_dev_common_info_v1 com;
	struct isp_size src;
	struct isp_drv_slice_param slice;
	struct isp_sbs_info sbs_info;
};

cmr_s32 isp_dev_open(cmr_s32 fd, cmr_handle * handle);
cmr_s32 isp_dev_close(cmr_handle handle);
cmr_s32 isp_dev_cfg_start(cmr_handle handle);
cmr_s32 isp_dev_set_statis_buf(cmr_handle handle, struct isp_statis_buf_input *param);
cmr_s32 isp_dev_set_slice_raw_info(cmr_handle handle, struct isp_raw_proc_info *param);
cmr_s32 isp_dev_3dnr(cmr_handle handle, struct isp_3dnr_info *param);

cmr_s32 isp_u_capability_continue_size(cmr_handle handle, cmr_u32 * width, cmr_u32 * height);
cmr_s32 isp_u_capability_time(cmr_handle handle, cmr_u32 * sec, cmr_u32 * usec);

cmr_s32 isp_u_fetch_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_fetch_slice_size(cmr_handle handle, cmr_u32 w, cmr_u32 h);
cmr_s32 isp_u_fetch_raw_transaddr(cmr_handle handle, struct isp_dev_block_addr *addr);

cmr_s32 isp_u_blc_block(cmr_handle handle, void *block_info);

cmr_s32 isp_u_2d_lsc_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_2d_lsc_slice_size(cmr_handle handle, cmr_u32 w, cmr_u32 h);
cmr_s32 isp_u_2d_lsc_transaddr(cmr_handle handle, struct isp_statis_buf_input *buf);

cmr_s32 isp_u_awbc_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_awbc_gain(cmr_handle handle, cmr_u32 r, cmr_u32 g, cmr_u32 b);

cmr_s32 isp_u_bpc_block(cmr_handle handle, void *block_info);

cmr_s32 isp_u_bdn_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_bdn_bypass(cmr_handle handle, cmr_u32 bypass);
cmr_s32 isp_u_bdn_slice_size(cmr_handle handle, cmr_u32 w, cmr_u32 h);
cmr_s32 isp_u_bdn_diswei(cmr_handle handle, cmr_u32 diswei_level);
cmr_s32 isp_u_bdn_ranwei(cmr_handle handle, cmr_u32 ranwei_level);

cmr_s32 isp_u_grgb_block(cmr_handle handle, void *block_info);

cmr_s32 isp_u_cfa_block(cmr_handle handle, void *block_info);

cmr_s32 isp_u_cmc_block(cmr_handle handle, void *block_info);

cmr_s32 isp_u_gamma_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_gamma_status(cmr_handle handle, cmr_u32 * status);

cmr_s32 isp_u_cce_matrix_block(cmr_handle handle, void *block_info);

cmr_s32 isp_u_prefilter_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_prefilter_bypass(cmr_handle handle, cmr_u32 bypass);
cmr_s32 isp_u_prefilter_writeback(cmr_handle handle, cmr_u32 writeback);
cmr_s32 isp_u_prefilter_thrd(cmr_handle handle, cmr_u32 y_thrd, cmr_u32 u_thrd, cmr_u32 v_thrd);
cmr_s32 isp_u_prefilter_slice_size(cmr_handle handle, cmr_u32 w, cmr_u32 h);
cmr_s32 isp_u_prefilter_slice_info(cmr_handle handle, cmr_u32 info);

cmr_s32 isp_u_brightness_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_brightness_slice_size(cmr_handle handle, cmr_u32 w, cmr_u32 h);
cmr_s32 isp_u_brightness_slice_info(cmr_handle handle, cmr_u32 info);

cmr_s32 isp_u_contrast_block(cmr_handle handle, void *block_info);

cmr_s32 isp_u_hist_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_hist_auto_rst_disable(cmr_handle handle, cmr_u32 off);
cmr_s32 isp_u_hist_ratio(cmr_handle handle, cmr_u16 low_ratio, cmr_u16 high_ratio);
cmr_s32 isp_u_hist_maxmin(cmr_handle handle, cmr_u32 in_min, cmr_u32 in_max, cmr_u32 out_min, cmr_u32 out_max);
cmr_s32 isp_u_hist_clear_eb(cmr_handle handle, cmr_u32 eb);
cmr_s32 isp_u_hist_statistic(cmr_handle handle, void *out_value);
cmr_s32 isp_u_hist_statistic_num(cmr_handle handle, cmr_u32 * num);

cmr_s32 isp_u_afm_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_afm_bypass(cmr_handle handle, cmr_u32 bypass);
cmr_s32 isp_u_afm_shift(cmr_handle handle, cmr_u32 shift);
cmr_s32 isp_u_afm_mode(cmr_handle handle, cmr_u32 mode);
cmr_s32 isp_u_afm_skip_num(cmr_handle handle, cmr_u32 skip_num);
cmr_s32 isp_u_afm_skip_num_clr(cmr_handle handle, cmr_u32 is_clear);
cmr_s32 isp_u_afm_win(cmr_handle handle, void *win_rangs);
cmr_s32 isp_u_afm_statistic(cmr_handle handle, void *out_statistic);
cmr_s32 isp_u_afm_win_num(cmr_handle handle, cmr_u32 * win_num);
cmr_s32 isp_u_raw_afm_spsmd_square_en(cmr_handle handle, cmr_u32 en);

cmr_s32 isp_u_edge_block(cmr_handle handle, void *block_info);

cmr_s32 isp_u_emboss_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_emboss_bypass(cmr_handle handle, cmr_u32 bypass);
cmr_s32 isp_u_emboss_param(cmr_handle handle, cmr_u32 step);

cmr_s32 isp_u_fcs_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_fcs_bypass(cmr_handle handle, cmr_u32 bypass);
cmr_s32 isp_u_fcs_mode(cmr_handle handle, cmr_u32 mode);

cmr_s32 isp_u_css_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_css_bypass(cmr_handle handle, cmr_u32 bypass);
cmr_s32 isp_u_css_thrd(cmr_handle handle, cmr_u8 * low_thr, cmr_u8 * low_sum_thr, cmr_u8 lum_thr, cmr_u8 chr_thr);
cmr_s32 isp_u_css_slice_size(cmr_handle handle, cmr_u32 w, cmr_u32 h);
cmr_s32 isp_u_css_ratio(cmr_handle handle, cmr_u8 * ratio);

cmr_s32 isp_u_csa_block(cmr_handle handle, void *block_info);

cmr_s32 isp_u_store_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_store_slice_size(cmr_handle handle, cmr_u32 w, cmr_u32 h);

cmr_s32 isp_u_nlc_block(cmr_handle handle, void *block_info);

cmr_s32 isp_u_nawbm_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_nawbm_bypass(cmr_handle handle, cmr_u32 bypass);

cmr_s32 isp_u_pre_wavelet_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_pre_wavelet_bypass(cmr_handle handle, cmr_u32 bypass);

cmr_s32 isp_u_binning4awb_block(cmr_handle handle, void *block_info);

cmr_s32 isp_u_pgg_block(cmr_handle handle, void *block_info);

cmr_s32 isp_u_comm_start(cmr_handle handle, cmr_u32 start);
cmr_s32 isp_u_comm_in_mode(cmr_handle handle, cmr_u32 mode);
cmr_s32 isp_u_comm_out_mode(cmr_handle handle, cmr_u32 mode);
cmr_s32 isp_u_comm_fetch_endian(cmr_handle handle, cmr_u32 endian, cmr_u32 bit_reorder);
cmr_s32 isp_u_comm_bpc_endian(cmr_handle handle, cmr_u32 endian);
cmr_s32 isp_u_comm_store_endian(cmr_handle handle, cmr_u32 endian);
cmr_s32 isp_u_comm_fetch_data_format(cmr_handle handle, cmr_u32 format);
cmr_s32 isp_u_comm_store_format(cmr_handle handle, cmr_u32 format);
cmr_s32 isp_u_comm_burst_size(cmr_handle handle, cmr_u16 burst_size);
cmr_s32 isp_u_comm_mem_switch(cmr_handle handle, cmr_u8 mem_switch);
cmr_s32 isp_u_comm_shadow(cmr_handle handle, cmr_u32 shadow);
cmr_s32 isp_u_comm_shadow_all(cmr_handle handle, cmr_u8 shadow);
cmr_s32 isp_u_comm_bayer_mode(cmr_handle handle, cmr_u32 nlc_bayer, cmr_u32 awbc_bayer, cmr_u32 wave_bayer, cmr_u32 cfa_bayer, cmr_u32 gain_bayer);
cmr_s32 isp_u_comm_isp_s32_clear(cmr_handle handle, cmr_u32 isp_s32_num);
cmr_s32 isp_u_comm_get_isp_s32_raw(cmr_handle handle, cmr_u32 * raw);
cmr_s32 isp_u_comm_pmu_raw_mask(cmr_handle handle, cmr_u8 raw_mask);
cmr_s32 isp_u_comm_hw_mask(cmr_handle handle, cmr_u32 hw_logic);
cmr_s32 isp_u_comm_hw_enable(cmr_handle handle, cmr_u32 hw_logic);
cmr_s32 isp_u_comm_pmu_pmu_sel(cmr_handle handle, cmr_u8 sel);
cmr_s32 isp_u_comm_sw_enable(cmr_handle handle, cmr_u32 sw_logic);
cmr_s32 isp_u_comm_preview_stop(cmr_handle handle, cmr_u8 eb);
cmr_s32 isp_u_comm_set_shadow_control(cmr_handle handle, cmr_u32 control);
cmr_s32 isp_u_comm_shadow_control_clear(cmr_handle handle, cmr_u8 eb);
cmr_s32 isp_u_comm_axi_stop(cmr_handle handle, cmr_u8 eb);
cmr_s32 isp_u_comm_slice_cnt_enable(cmr_handle handle, cmr_u8 eb);
cmr_s32 isp_u_comm_preform_cnt_enable(cmr_handle handle, cmr_u8 eb);
cmr_s32 isp_u_comm_set_slice_num(cmr_handle handle, cmr_u8 num);
cmr_s32 isp_u_comm_get_slice_num(cmr_handle handle, cmr_u8 * slice_num);
cmr_s32 isp_u_comm_perform_cnt_rstatus(cmr_handle handle, cmr_u32 * status);
cmr_s32 isp_u_comm_preform_cnt_status(cmr_handle handle, cmr_u32 * status);

cmr_s32 isp_u_glb_gain_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_glb_gain_bypass(cmr_handle handle, cmr_u32 bypass);
cmr_s32 isp_u_glb_gain_set(cmr_handle handle, cmr_u32 gain);
cmr_s32 isp_u_glb_gain_slice_size(cmr_handle handle, cmr_u16 w, cmr_u16 h);

cmr_s32 isp_u_rgb_gain_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_rgb_dither_block(cmr_handle handle, void *block_info);

cmr_s32 isp_u_hue_block(cmr_handle handle, void *block_info);

cmr_s32 isp_u_nbpc_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_nbpc_bypass(cmr_handle handle, cmr_u32 bypass);

cmr_s32 isp_u_1d_lsc_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_1d_lsc_slice_size(cmr_handle handle, cmr_u32 width, cmr_u32 height);
cmr_s32 isp_u_raw_aem_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_raw_aem_bypass(cmr_handle handle, void *block_info);
cmr_s32 isp_u_raw_aem_mode(cmr_handle handle, cmr_u32 mode);
cmr_s32 isp_u_raw_aem_skip_num(cmr_handle handle, cmr_u32 skip_num);
cmr_s32 isp_u_raw_aem_shift(cmr_handle handle, void *shift);
cmr_s32 isp_u_raw_aem_offset(cmr_handle handle, cmr_u32 x, cmr_u32 y);
cmr_s32 isp_u_raw_aem_blk_size(cmr_handle handle, cmr_u32 width, cmr_u32 height);
cmr_s32 isp_u_raw_aem_slice_size(cmr_handle handle, cmr_u32 width, cmr_u32 height);
cmr_s32 isp_u_nlm_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_ct_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_hsv_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_frgb_precdn_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_posterize_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_raw_afm_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_raw_afm_slice_size(cmr_handle handle, cmr_u32 width, cmr_u32 height);
cmr_s32 isp_u_raw_afm_bypass(cmr_handle handle, cmr_u32 bypass);
cmr_s32 isp_u_raw_afm_mode(cmr_handle handle, cmr_u32 mode);
cmr_s32 isp_u_raw_afm_skip_num(cmr_handle handle, cmr_u32 skip_num);
cmr_s32 isp_u_raw_afm_iir_nr_cfg(cmr_handle handle, void *block_info);
cmr_s32 isp_u_raw_afm_modules_cfg(cmr_handle handle, void *block_info);

cmr_s32 isp_u_raw_afm_skip_num_clr(cmr_handle handle, cmr_u32 clear);
cmr_s32 isp_u_raw_afm_win(cmr_handle handle, void *win_range);
cmr_s32 isp_u_raw_afm_win_num(cmr_handle handle, cmr_u32 * win_num);

cmr_s32 isp_u_anti_flicker_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_anti_flicker_bypass(cmr_handle handle, void *block_info);
cmr_s32 isp_u_anti_flicker_new_bypass(cmr_handle handle, void *block_info);
cmr_s32 isp_u_anti_flicker_new_block(cmr_handle handle, void *block_info);

cmr_s32 isp_u_yuv_precdn_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_hist_v1_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_hist_slice_size(cmr_handle handle, cmr_u32 width, cmr_u32 height);
cmr_s32 isp_u_hist2_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_yuv_cdn_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_yuv_postcdn_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_ygamma_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_ydelay_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_iircnr_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_yrandom_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_fetch_start_isp(cmr_handle handle, cmr_u32 fetch_start);
cmr_s32 isp_u_dispatch_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_dispatch_ch0_size(cmr_handle handle, cmr_u32 width, cmr_u32 height);
cmr_s32 isp_u_arbiter_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_comm_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_comm_shadow_ctrl(cmr_handle handle, cmr_u32 shadow_done);
cmr_s32 isp_u_3a_ctrl(cmr_handle handle, cmr_u32 enable);
cmr_s32 isp_u_afl_ctrl(cmr_handle handle, cmr_u32 afl_sel);
cmr_s32 isp_cfg_block(cmr_handle handle, void *param_ptr, cmr_u32 sub_block);
cmr_s32 isp_set_arbiter(cmr_handle handle);
cmr_s32 isp_set_dispatch(cmr_handle handle);
cmr_s32 isp_get_fetch_addr(struct isp_drv_interface_param * isp_context_ptr, struct isp_dev_fetch_info_v1 * fetch_ptr);
cmr_s32 isp_set_fetch_param(cmr_handle handle);
cmr_s32 isp_set_store_param(cmr_handle handle);
cmr_s32 isp_set_slice_size(cmr_handle handle);
cmr_s32 isp_cfg_slice_size(cmr_handle handle, struct isp_drv_slice_param *slice_ptr);
cmr_s32 isp_set_comm_param(cmr_handle handle);
cmr_s32 isp_cfg_comm_data(cmr_handle handle, struct isp_dev_common_info_v1 * param_ptr);
cmr_s32 isp_u_bq_init_bufqueue(cmr_handle handle);
cmr_s32 isp_u_bq_enqueue_buf(cmr_handle handle, cmr_u64 k_addr, cmr_u64 u_addr, cmr_u32 type);
cmr_s32 isp_u_bq_dequeue_buf(cmr_handle handle, cmr_u64 * k_addr, cmr_u64 * u_addr, cmr_u32 type);

cmr_s32 isp_u_rgb2y_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_yuv_nlm_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_yuv_nlm_slice_size(cmr_handle handle, cmr_u32 width, cmr_u32 height);
cmr_s32 isp_u_dispatch_yuv_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_uvd_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_post_blc_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_ynr_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_noise_filter_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_pdaf_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_pdaf_bypass(cmr_handle handle, cmr_u32 *bypass);
cmr_s32 isp_u_pdaf_skip_num(cmr_handle handle, cmr_u32 *skip_num);
cmr_s32 isp_u_pdaf_work_mode(cmr_handle handle, cmr_u32 *work_mode);
cmr_s32 isp_u_pdaf_ppi_info(cmr_handle handle, void *ppi_info);
cmr_s32 isp_u_pdaf_extractor_bypass(cmr_handle handle, cmr_u32 *bypass);
cmr_s32 isp_u_pdaf_roi(cmr_handle handle, void *roi);
cmr_s32 isp_u_pdaf_correction(cmr_handle handle, void *correction_param);
cmr_s32 isp_u_3dnr_cap_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_3dnr_pre_block(cmr_handle handle, void *block_info);


#endif
