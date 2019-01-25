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
#ifndef WIN32
#include <sys/types.h>
#include <utils/Log.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>
#include "sprd_isp_r6p10v2.h"
#include "sprd_img.h"
#include "isp_com.h"
#endif
#include "isp_type.h"
#include "isp_blocks_cfg.h"

typedef void (*isp_evt_cb) (cmr_int evt, void *data, void *privdata);

struct isp_file {
	cmr_s32 fd;
	cmr_u32 chip_id;
	cmr_u32 isp_id;
	void *reserved;
};

struct isp_statis_mem_info {
	cmr_uint isp_statis_mem_size;
	cmr_uint isp_statis_mem_num;
	cmr_uint isp_statis_k_addr[2];
	cmr_uint isp_statis_u_addr;
	cmr_uint isp_statis_alloc_flag;
	cmr_s32 statis_mfd;
	cmr_s32 statis_buf_dev_fd;

	cmr_uint isp_lsc_mem_size;
	cmr_uint isp_lsc_mem_num;
	cmr_uint isp_lsc_physaddr;
	cmr_uint isp_lsc_virtaddr;
	cmr_s32 lsc_mfd;

	void *buffer_client_data;
	void *cb_of_malloc;
	void *cb_of_free;
};

struct isp_statis_info {
	cmr_u32 irq_type;
	cmr_u32 irq_property;
	cmr_u32 phy_addr;
	cmr_u32 vir_addr;
	cmr_u32 kaddr[2];
	cmr_u32 buf_size;
	cmr_s32 mfd;
	cmr_u32 sec;
	cmr_u32	usec;
	cmr_s64 monoboottime;
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
	cmr_u16 slice_height;
	cmr_u32 input_fd;
};

struct isp_slice_param_v1 {
	enum isp_slice_pos_info pos_info;
	cmr_u32 slice_line;
	cmr_u32 complete_line;
	cmr_u32 all_line;
	struct isp_size max_size;
	struct isp_size all_slice_num;
	struct isp_size cur_slice_num;

	struct isp_trim_size size[ISP_SLICE_WIN_NUM_V1];
	cmr_u32 edge_info;
};

struct isp_interface_param_v1 {
	struct isp_data_param data;

	struct isp_dev_fetch_info fetch;
	struct isp_dev_store_info store;
	struct isp_dev_dispatch_info dispatch;
	struct isp_dev_arbiter_info arbiter;
	struct isp_dev_common_info com;
	struct isp_size src;
	struct isp_slice_param_v1 slice;
};

/*ISP Hardware Device*/
cmr_s32 isp_dev_open(cmr_s32 fd, isp_handle * handle);
cmr_s32 isp_dev_close(isp_handle handle);
cmr_u32 isp_dev_get_chip_id(isp_handle handle);
cmr_s32 isp_dev_reset(isp_handle handle);
cmr_s32 isp_dev_stop(isp_handle handle);
cmr_s32 isp_dev_enable_irq(isp_handle handle, cmr_u32 irq_mode);
cmr_s32 isp_dev_get_irq(isp_handle handle, cmr_u32 * evt_ptr);
cmr_s32 isp_dev_reg_write(isp_handle handle, cmr_u32 num, void *param_ptr);
cmr_s32 isp_dev_reg_read(isp_handle handle, cmr_u32 num, void *param_ptr);
cmr_s32 isp_dev_reg_fetch(isp_handle handle, cmr_u32 base_offset, cmr_u32 * buf, cmr_u32 len);
cmr_s32 isp_dev_set_statis_buf(isp_handle handle, struct isp_statis_buf_input *param);

/*ISP 3DNR proc*/
cmr_s32 isp_dev_3dnr(isp_handle handle, struct isp_3dnr_info *param);
/*ISP Capability*/
cmr_s32 isp_u_capability_chip_id(isp_handle handle, cmr_u32 * chip_id);
cmr_s32 isp_u_capability_continue_size(isp_handle handle, cmr_u16 * width, cmr_u16 * height);
cmr_s32 isp_u_capability_single_size(isp_handle handle, cmr_u16 * width, cmr_u16 * height);
cmr_s32 isp_u_capability_awb_win(isp_handle handle, cmr_u16 * width, cmr_u16 * height);
cmr_u32 isp_u_capability_awb_default_gain(isp_handle handle);
cmr_u32 isp_u_capability_af_max_win_num(isp_handle handle);
cmr_s32 isp_u_capability_time(isp_handle handle, cmr_u32 * sec, cmr_u32 * usec);
/*ISP Sub Block: Fetch*/
cmr_s32 isp_u_fetch_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_fetch_slice_size(isp_handle handle, cmr_u32 w, cmr_u32 h);
cmr_s32 isp_u_fetch_raw_transaddr(isp_handle handle, struct isp_dev_block_addr *addr);

/*ISP Sub Block: BLC*/
cmr_s32 isp_u_blc_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_blc_slice_size(isp_handle handle, cmr_u32 w, cmr_u32 h);
cmr_s32 isp_u_blc_slice_info(isp_handle handle, cmr_u32 info);

/*ISP Sub Block: lens shading calibration*/
cmr_s32 isp_u_2d_lsc_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_2d_lsc_bypass(isp_handle handle, cmr_u32 bypass);
cmr_s32 isp_u_2d_lsc_param_update(isp_handle handle, cmr_u32 flag);
cmr_s32 isp_u_2d_lsc_pos(isp_handle handle, cmr_u32 x, cmr_u32 y);
cmr_s32 isp_u_2d_lsc_grid_size(isp_handle handle, cmr_u32 w, cmr_u32 h);
cmr_s32 isp_u_2d_lsc_slice_size(isp_handle handle, cmr_u32 w, cmr_u32 h);
cmr_s32 isp_u_2d_lsc_transaddr(isp_handle handle, struct isp_statis_buf_input *buf);

/*ISP Sub Block: AWBM*/
cmr_s32 isp_u_awb_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_awbm_statistics(isp_handle handle, cmr_u32 * r_info, cmr_u32 * g_info, cmr_u32 * b_info);
cmr_s32 isp_u_awbm_bypass(isp_handle handle, cmr_u32 bypass);
cmr_s32 isp_u_awbm_mode(isp_handle handle, cmr_u32 mode);
cmr_s32 isp_u_awbm_skip_num(isp_handle handle, cmr_u32 num);
cmr_s32 isp_u_awbm_block_offset(isp_handle handle, cmr_u32 x, cmr_u32 y);
cmr_s32 isp_u_awbm_block_size(isp_handle handle, cmr_u32 w, cmr_u32 h);
cmr_s32 isp_u_awbm_shift(isp_handle handle, cmr_u32 shift);
cmr_s32 isp_u_awbc_bypass(isp_handle handle, cmr_u32 bypass);
cmr_s32 isp_u_awbc_gain(isp_handle handle, cmr_u32 r, cmr_u32 g, cmr_u32 b);
cmr_s32 isp_u_awbc_thrd(isp_handle handle, cmr_u32 r, cmr_u32 g, cmr_u32 b);
cmr_s32 isp_u_awbc_gain_offset(isp_handle handle, cmr_u32 r, cmr_u32 g, cmr_u32 b);

/*ISP Sub Block: BPC*/
cmr_s32 isp_u_bpc_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_bpc_bypass(isp_handle handle, cmr_u32 bypass);
cmr_s32 isp_u_bpc_mode(isp_handle handle, cmr_u32 mode);
cmr_s32 isp_u_bpc_param_common(isp_handle handle, cmr_u32 pattern_type, cmr_u32 detect_thrd, cmr_u32 super_bad_thrd);
cmr_s32 isp_u_bpc_thrd(isp_handle handle, cmr_u32 flat, cmr_u32 std, cmr_u32 texture);
cmr_s32 isp_u_bpc_map_addr(isp_handle handle, cmr_u32 addr);
cmr_s32 isp_u_bpc_pixel_num(isp_handle handle, cmr_u32 pixel_num);

/*ISP Sub Block: bilateral denoise*/
cmr_s32 isp_u_bdn_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_bdn_bypass(isp_handle handle, cmr_u32 bypass);
cmr_s32 isp_u_bdn_slice_size(isp_handle handle, cmr_u32 w, cmr_u32 h);
cmr_s32 isp_u_bdn_diswei(isp_handle handle, cmr_u32 diswei_level);
cmr_s32 isp_u_bdn_ranwei(isp_handle handle, cmr_u32 ranwei_level);

/*ISP Sub Block: GRGB*/
cmr_s32 isp_u_grgb_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_grgb_bypass(isp_handle handle, cmr_u32 bypass);
cmr_s32 isp_u_grgb_thrd(isp_handle handle, cmr_u32 edge, cmr_u32 diff);

/*ISP Sub Block: CFA*/
cmr_s32 isp_u_cfa_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_cfa_thrd(isp_handle handle, cmr_u32 edge, cmr_u32 ctrl);
cmr_s32 isp_u_cfa_slice_size(isp_handle handle, cmr_u32 w, cmr_u32 h);
cmr_s32 isp_u_cfa_slice_info(isp_handle handle, cmr_u32 info);

/*ISP Sub Block: CMC*/
cmr_s32 isp_u_cmc_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_cmc_bypass(isp_handle handle, cmr_u32 bypass);
cmr_s32 isp_u_cmc_matrix(isp_handle handle, cmr_u16 * matrix_ptr);

/*ISP Sub Block: GAMMA*/
cmr_s32 isp_u_gamma_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_gamma_status(isp_handle handle, cmr_u32 * status);
cmr_s32 isp_u_gamma_bypass(isp_handle handle, cmr_u32 bypass);
cmr_s32 isp_u_gamma_node(isp_handle handle, cmr_u16 * node_ptr);

/*ISP Sub Block: CCE*/
cmr_s32 isp_u_cce_matrix_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_cce_uv_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_cce_uvdivision_bypass(isp_handle handle, cmr_u32 bypass);
cmr_s32 isp_u_cce_mode(isp_handle handle, cmr_u32 mode);
cmr_s32 isp_u_cce_matrix(isp_handle handle, cmr_u16 * matrix_ptr);
cmr_s32 isp_u_cce_shift(isp_handle handle, cmr_u32 y_shift, cmr_u32 u_shift, cmr_u32 v_shift);
cmr_s32 isp_u_cce_uvd(isp_handle handle, cmr_u8 * div_ptr);
cmr_s32 isp_u_cce_uvc(isp_handle handle, cmr_u8 * t_ptr, cmr_u8 * m_ptr);

/*ISP Sub Block: Pre-Filter*/
cmr_s32 isp_u_prefilter_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_prefilter_bypass(isp_handle handle, cmr_u32 bypass);
cmr_s32 isp_u_prefilter_writeback(isp_handle handle, cmr_u32 writeback);
cmr_s32 isp_u_prefilter_thrd(isp_handle handle, cmr_u32 y_thrd, cmr_u32 u_thrd, cmr_u32 v_thrd);
cmr_s32 isp_u_prefilter_slice_size(isp_handle handle, cmr_u32 w, cmr_u32 h);
cmr_s32 isp_u_prefilter_slice_info(isp_handle handle, cmr_u32 info);

/*ISP Sub Block: Brightness*/
cmr_s32 isp_u_brightness_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_brightness_slice_size(isp_handle handle, cmr_u32 w, cmr_u32 h);
cmr_s32 isp_u_brightness_slice_info(isp_handle handle, cmr_u32 info);

/*ISP Sub Block: Contrast*/
cmr_s32 isp_u_contrast_block(isp_handle handle, void *block_info);

/*ISP Sub Block: HIST*/
cmr_s32 isp_u_hist_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_hist_bypass(isp_handle handle, cmr_u32 bypass);
cmr_s32 isp_u_hist_mode(isp_handle handle, cmr_u32 mode);
cmr_s32 isp_u_hist_auto_rst_disable(isp_handle handle, cmr_u32 off);
cmr_s32 isp_u_hist_mode(isp_handle handle, cmr_u32 mode);
cmr_s32 isp_u_hist_ratio(isp_handle handle, cmr_u16 low_ratio, cmr_u16 high_ratio);
cmr_s32 isp_u_hist_maxmin(isp_handle handle, cmr_u32 in_min, cmr_u32 in_max, cmr_u32 out_min, cmr_u32 out_max);
cmr_s32 isp_u_hist_clear_eb(isp_handle handle, cmr_u32 eb);
cmr_s32 isp_u_hist_statistic(isp_handle handle, void *out_value);
cmr_s32 isp_u_hist_statistic_num(isp_handle handle, cmr_u32 * num);

/*ISP Sub Block: AFM*/
cmr_s32 isp_u_afm_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_afm_bypass(isp_handle handle, cmr_u32 bypass);
cmr_s32 isp_u_afm_shift(isp_handle handle, cmr_u32 shift);
cmr_s32 isp_u_afm_mode(isp_handle handle, cmr_u32 mode);
cmr_s32 isp_u_afm_skip_num(isp_handle handle, cmr_u32 skip_num);
cmr_s32 isp_u_afm_skip_num_clr(isp_handle handle, cmr_u32 is_clear);
cmr_s32 isp_u_afm_win(isp_handle handle, void *win_rangs);
cmr_s32 isp_u_afm_statistic(isp_handle handle, void *out_statistic);
cmr_s32 isp_u_afm_win_num(isp_handle handle, cmr_u32 * win_num);
cmr_s32 isp_u_raw_afm_statistic_r6p9(isp_handle handle, void *statis);
cmr_s32 isp_u_raw_afm_spsmd_square_en(isp_handle handle, cmr_u32 en);
cmr_s32 isp_u_raw_afm_overflow_protect(isp_handle handle, cmr_u32 en);
cmr_s32 isp_u_raw_afm_subfilter(isp_handle handle, cmr_u32 average, cmr_u32 median);
cmr_s32 isp_u_raw_afm_spsmd_touch_mode(isp_handle handle, cmr_u32 mode);
cmr_s32 isp_u_raw_afm_shfit(isp_handle handle, cmr_u32 spsmd, cmr_u32 sobel5, cmr_u32 sobel9);
cmr_s32 isp_u_raw_afm_threshold_rgb(isp_handle handle, void *thr_rgb);
/*ISP Sub Block: EDGE*/
cmr_s32 isp_u_edge_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_edge_bypass(isp_handle handle, cmr_u32 bypass);
cmr_s32 isp_u_edge_param(isp_handle handle, cmr_u32 detail, cmr_u32 smooth, cmr_u32 strength);

/*ISP Sub Block: Emboss*/
cmr_s32 isp_u_emboss_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_emboss_bypass(isp_handle handle, cmr_u32 bypass);
cmr_s32 isp_u_emboss_param(isp_handle handle, cmr_u32 step);

/*ISP Sub Block: FCS*/
cmr_s32 isp_u_fcs_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_fcs_bypass(isp_handle handle, cmr_u32 bypass);
cmr_s32 isp_u_fcs_mode(isp_handle handle, cmr_u32 mode);

/*ISP Sub Block: CSS*/
cmr_s32 isp_u_css_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_css_bypass(isp_handle handle, cmr_u32 bypass);
cmr_s32 isp_u_css_thrd(isp_handle handle, cmr_u8 * low_thr, cmr_u8 * low_sum_thr, cmr_u8 lum_thr, cmr_u8 chr_thr);
cmr_s32 isp_u_css_slice_size(isp_handle handle, cmr_u32 w, cmr_u32 h);
cmr_s32 isp_u_css_ratio(isp_handle handle, cmr_u8 * ratio);

/*ISP Sub Block: CSA*/
cmr_s32 isp_u_csa_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_csa_bypass(isp_handle handle, cmr_u32 bypass);
cmr_s32 isp_u_csa_factor(isp_handle handle, cmr_u32 factor);

/*ISP Sub Block: Store*/
cmr_s32 isp_u_store_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_store_slice_size(isp_handle handle, cmr_u32 w, cmr_u32 h);

/*ISP Sub Block: NLC*/
cmr_s32 isp_u_nlc_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_nlc_bypass(isp_handle handle, cmr_u32 bypass);
cmr_s32 isp_u_nlc_r_node(isp_handle handle, cmr_u16 * r_node_ptr);
cmr_s32 isp_u_nlc_g_node(isp_handle handle, cmr_u16 * g_node_ptr);
cmr_s32 isp_u_nlc_b_node(isp_handle handle, cmr_u16 * b_node_ptr);
cmr_s32 isp_u_nlc_l_node(isp_handle handle, cmr_u16 * l_node_ptr);

/*ISP Sub Block: Nawbm*/
cmr_s32 isp_u_nawbm_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_nawbm_bypass(isp_handle handle, cmr_u32 bypass);

/*ISP Sub Block: Pre Wavelet*/
cmr_s32 isp_u_pre_wavelet_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_pre_wavelet_bypass(isp_handle handle, cmr_u32 bypass);

/*ISP Sub Block: Bing4awb*/
cmr_s32 isp_u_binning4awb_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_binning4awb_bypass(isp_handle handle, cmr_u32 bypass);
cmr_s32 isp_u_binning4awb_endian(isp_handle handle, cmr_u32 endian);
cmr_s32 isp_u_binning4awb_scaling_ratio(isp_handle handle, cmr_u32 vertical, cmr_u32 horizontal);
cmr_s32 isp_u_binning4awb_get_scaling_ratio(isp_handle handle, cmr_u32 * vertical, cmr_u32 * horizontal);
cmr_s32 isp_u_binning4awb_mem_addr(isp_handle handle, cmr_u32 phy_addr);
cmr_s32 isp_u_binning4awb_statistics_buf(isp_handle handle, cmr_u32 * buf_id);
cmr_s32 isp_u_binning4awb_transaddr(isp_handle handle, cmr_u32 phys0, cmr_u32 phys1);
cmr_s32 isp_u_binning4awb_initbuf(isp_handle handle);

/*ISP Sub Block: Pre Glb Gain*/
cmr_s32 isp_u_pgg_block(isp_handle handle, void *block_info);;

/*ISP Sub Block: COMMON*/
cmr_s32 isp_u_comm_start(isp_handle handle, cmr_u32 start);
cmr_s32 isp_u_comm_in_mode(isp_handle handle, cmr_u32 mode);
cmr_s32 isp_u_comm_out_mode(isp_handle handle, cmr_u32 mode);
cmr_s32 isp_u_comm_fetch_endian(isp_handle handle, cmr_u32 endian, cmr_u32 bit_reorder);
cmr_s32 isp_u_comm_bpc_endian(isp_handle handle, cmr_u32 endian);
cmr_s32 isp_u_comm_store_endian(isp_handle handle, cmr_u32 endian);
cmr_s32 isp_u_comm_fetch_data_format(isp_handle handle, cmr_u32 format);
cmr_s32 isp_u_comm_store_format(isp_handle handle, cmr_u32 format);
cmr_s32 isp_u_comm_burst_size(isp_handle handle, cmr_u16 burst_size);
cmr_s32 isp_u_comm_mem_switch(isp_handle handle, cmr_u8 mem_switch);
cmr_s32 isp_u_comm_shadow(isp_handle handle, cmr_u32 shadow);
cmr_s32 isp_u_comm_shadow_all(isp_handle handle, cmr_u8 shadow);
cmr_s32 isp_u_comm_bayer_mode(isp_handle handle, cmr_u32 nlc_bayer, cmr_u32 awbc_bayer, cmr_u32 wave_bayer, cmr_u32 cfa_bayer, cmr_u32 gain_bayer);
cmr_s32 isp_u_comm_isp_s32_clear(isp_handle handle, cmr_u32 isp_s32_num);
cmr_s32 isp_u_comm_get_isp_s32_raw(isp_handle handle, cmr_u32 * raw);
cmr_s32 isp_u_comm_pmu_raw_mask(isp_handle handle, cmr_u8 raw_mask);
cmr_s32 isp_u_comm_hw_mask(isp_handle handle, cmr_u32 hw_logic);
cmr_s32 isp_u_comm_hw_enable(isp_handle handle, cmr_u32 hw_logic);
cmr_s32 isp_u_comm_pmu_pmu_sel(isp_handle handle, cmr_u8 sel);
cmr_s32 isp_u_comm_sw_enable(isp_handle handle, cmr_u32 sw_logic);
cmr_s32 isp_u_comm_preview_stop(isp_handle handle, cmr_u8 eb);
cmr_s32 isp_u_comm_set_shadow_control(isp_handle handle, cmr_u32 control);
cmr_s32 isp_u_comm_shadow_control_clear(isp_handle handle, cmr_u8 eb);
cmr_s32 isp_u_comm_axi_stop(isp_handle handle, cmr_u8 eb);
cmr_s32 isp_u_comm_slice_cnt_enable(isp_handle handle, cmr_u8 eb);
cmr_s32 isp_u_comm_preform_cnt_enable(isp_handle handle, cmr_u8 eb);
cmr_s32 isp_u_comm_set_slice_num(isp_handle handle, cmr_u8 num);
cmr_s32 isp_u_comm_get_slice_num(isp_handle handle, cmr_u8 * slice_num);
cmr_s32 isp_u_comm_perform_cnt_rstatus(isp_handle handle, cmr_u32 * status);
cmr_s32 isp_u_comm_preform_cnt_status(isp_handle handle, cmr_u32 * status);

/*ISP Sub Block: Glb gain*/
cmr_s32 isp_u_glb_gain_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_glb_gain_bypass(isp_handle handle, cmr_u32 bypass);
cmr_s32 isp_u_glb_gain_set(isp_handle handle, cmr_u32 gain);
cmr_s32 isp_u_glb_gain_slice_size(isp_handle handle, cmr_u16 w, cmr_u16 h);

/*ISP Sub Block: RGB gain*/
cmr_s32 isp_u_rgb_gain_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_rgb_dither_block(isp_handle handle, void *block_info);

/*ISP Sub Block: Hue*/
cmr_s32 isp_u_hue_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_hue_bypass(isp_handle handle, cmr_u32 bypass);
cmr_s32 isp_u_hue_Factor(isp_handle handle, cmr_u32 factor);

/*ISP Sub Block: new bad pixel correction*/
cmr_s32 isp_u_nbpc_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_nbpc_bypass(isp_handle handle, cmr_u32 bypass);

cmr_s32 isp_u_1d_lsc_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_1d_lsc_slice_size(isp_handle handle, cmr_u32 width, cmr_u32 height);
cmr_s32 isp_u_raw_aem_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_raw_aem_bypass(isp_handle handle, void *block_info);
cmr_s32 isp_u_raw_aem_mode(isp_handle handle, cmr_u32 mode);
cmr_s32 isp_u_raw_aem_statistics(isp_handle handle, cmr_u32 * r_info, cmr_u32 * g_info, cmr_u32 * b_info);
cmr_s32 isp_u_raw_aem_skip_num(isp_handle handle, cmr_u32 skip_num);
cmr_s32 isp_u_raw_aem_shift(isp_handle handle, void *shift);
cmr_s32 isp_u_raw_aem_offset(isp_handle handle, cmr_u32 x, cmr_u32 y);
cmr_s32 isp_u_raw_aem_blk_size(isp_handle handle, cmr_u32 width, cmr_u32 height);
cmr_s32 isp_u_raw_aem_slice_size(isp_handle handle, cmr_u32 width, cmr_u32 height);
cmr_s32 isp_u_nlm_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_ct_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_hsv_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_frgb_precdn_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_posterize_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_raw_afm_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_raw_afm_slice_size(isp_handle handle, cmr_u32 width, cmr_u32 height);
cmr_s32 isp_u_raw_afm_type1_statistic(isp_handle handle, void *statis);
cmr_s32 isp_u_raw_afm_type2_statistic(isp_handle handle, void *statis);
cmr_s32 isp_u_raw_afm_bypass(isp_handle handle, cmr_u32 bypass);
cmr_s32 isp_u_raw_afm_mode(isp_handle handle, cmr_u32 mode);
cmr_s32 isp_u_raw_afm_skip_num(isp_handle handle, cmr_u32 skip_num);
cmr_s32 isp_u_raw_afm_iir_nr_cfg(isp_handle handle, void *block_info);
cmr_s32 isp_u_raw_afm_modules_cfg(isp_handle handle, void *block_info);

cmr_s32 isp_u_raw_afm_skip_num_clr(isp_handle handle, cmr_u32 clear);
cmr_s32 isp_u_raw_afm_spsmd_rtgbot_enable(isp_handle handle, cmr_u32 enable);
cmr_s32 isp_u_raw_afm_spsmd_diagonal_enable(isp_handle handle, cmr_u32 enable);
cmr_s32 isp_u_raw_afm_spsmd_cal_mode(isp_handle handle, cmr_u32 mode);
cmr_s32 isp_u_raw_afm_sel_filter1(isp_handle handle, cmr_u32 sel_filter);
cmr_s32 isp_u_raw_afm_sel_filter2(isp_handle handle, cmr_u32 sel_filter);
cmr_s32 isp_u_raw_afm_sobel_type(isp_handle handle, cmr_u32 type);
cmr_s32 isp_u_raw_afm_spsmd_type(isp_handle handle, cmr_u32 type);
cmr_s32 isp_u_raw_afm_sobel_threshold(isp_handle handle, cmr_u32 min, cmr_u32 max);
cmr_s32 isp_u_raw_afm_spsmd_threshold(isp_handle handle, cmr_u32 min, cmr_u32 max);
cmr_s32 isp_u_raw_afm_win(isp_handle handle, void *win_range);
cmr_s32 isp_u_raw_afm_win_num(isp_handle handle, cmr_u32 * win_num);
cmr_s32 isp_u_anti_flicker_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_anti_flicker_statistic(isp_handle handle, void *addr);
cmr_s32 isp_u_anti_flicker_bypass(isp_handle handle, void *block_info);
cmr_s32 isp_u_anti_flicker_new_bypass(isp_handle handle, void *block_info);
cmr_s32 isp_u_anti_flicker_new_block(isp_handle handle, void *block_info);

cmr_s32 isp_u_anti_flicker_transaddr(isp_handle handle, cmr_u32 phys_addr);
cmr_s32 isp_u_yuv_precdn_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_hist_v1_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_hist_slice_size(isp_handle handle, cmr_u32 width, cmr_u32 height);
cmr_s32 isp_u_hist2_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_yuv_cdn_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_yuv_postcdn_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_ygamma_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_ydelay_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_iircnr_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_yrandom_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_fetch_start_isp(isp_handle handle, cmr_u32 fetch_start);
cmr_s32 isp_u_dispatch_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_dispatch_ch0_size(isp_handle handle, cmr_u32 width, cmr_u32 height);
cmr_s32 isp_u_arbiter_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_comm_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_shadow_ctrl_all(isp_handle handle, cmr_u32 auto_shadow);
cmr_s32 isp_u_awbm_shadow_ctrl(isp_handle handle, cmr_u32 shadow_done);
cmr_s32 isp_u_ae_shadow_ctrl(isp_handle handle, cmr_u32 shadow_done);
cmr_s32 isp_u_af_shadow_ctrl(isp_handle handle, cmr_u32 shadow_done);
cmr_s32 isp_u_afl_shadow_ctrl(isp_handle handle, cmr_u32 shadow_done);
cmr_s32 isp_u_comm_shadow_ctrl(isp_handle handle, cmr_u32 shadow_done);
cmr_s32 isp_u_3a_ctrl(isp_handle handle, cmr_u32 enable);
cmr_s32 isp_u_comm_channel0_y_aem_pos(isp_handle handle, cmr_u32 pos);
cmr_s32 isp_u_comm_channel1_y_aem_pos(isp_handle handle, cmr_u32 pos);
cmr_s32 isp_cfg_block(isp_handle handle, void *param_ptr, cmr_u32 sub_block);
cmr_s32 isp_set_arbiter(isp_handle isp_handler);
cmr_s32 isp_set_dispatch(isp_handle isp_handler);
cmr_s32 isp_get_fetch_addr(struct isp_interface_param_v1 *isp_context_ptr, struct isp_dev_fetch_info *fetch_ptr);
cmr_s32 isp_set_fetch_param(isp_handle isp_handler);
cmr_s32 isp_set_store_param(isp_handle isp_handler);
cmr_s32 isp_set_slice_size(isp_handle isp_handler);
cmr_s32 isp_cfg_slice_size(isp_handle handle, struct isp_slice_param_v1 *slice_ptr);
cmr_s32 isp_set_comm_param(isp_handle isp_handler);
cmr_s32 isp_cfg_comm_data(isp_handle handle, struct isp_dev_common_info *param_ptr);
cmr_s32 isp_cfg_all_shadow(isp_handle handle, cmr_u32 auto_shadow);
cmr_s32 isp_cfg_awbm_shadow(isp_handle handle, cmr_u32 shadow_done);
cmr_s32 isp_cfg_ae_shadow(isp_handle handle, cmr_u32 shadow_done);
cmr_s32 isp_cfg_af_shadow(isp_handle handle, cmr_u32 shadow_done);
cmr_s32 isp_cfg_afl_shadow(isp_handle handle, cmr_u32 shadow_done);
cmr_s32 isp_cfg_comm_shadow(isp_handle handle, cmr_u32 shadow_done);
cmr_s32 isp_cfg_3a_single_frame_shadow(isp_handle handle, cmr_u32 enable);
cmr_s32 isp_u_bq_init_bufqueue(isp_handle handle);
cmr_s32 isp_u_bq_enqueue_buf(isp_handle handle, cmr_u64 k_addr, cmr_u64 u_addr, cmr_u32 type);
cmr_s32 isp_u_bq_dequeue_buf(isp_handle handle, cmr_u64 * k_addr, cmr_u64 * u_addr, cmr_u32 type);

cmr_s32 isp_u_rgb2y_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_yuv_nlm_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_yuv_nlm_slice_size(isp_handle handle, cmr_u32 width, cmr_u32 height);
cmr_s32 isp_u_dispatch_yuv_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_uvd_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_post_blc_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_ynr_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_noise_filter_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_rarius_lsc_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_pdaf_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_pdaf_bypass(isp_handle handle, cmr_u32 *bypass);
cmr_s32 isp_u_pdaf_skip_num(isp_handle handle, cmr_u32 *skip_num);
cmr_s32 isp_u_pdaf_work_mode(isp_handle handle, cmr_u32 *work_mode);
cmr_s32 isp_u_pdaf_ppi_info(isp_handle handle, void *ppi_info);
cmr_s32 isp_u_pdaf_extractor_bypass(isp_handle handle, cmr_u32 *bypass);
cmr_s32 isp_u_pdaf_roi(isp_handle handle, void *roi);
cmr_s32 isp_u_pdaf_correction(isp_handle handle, void *correction_param);
cmr_s32 isp_u_3dnr_cap_block(isp_handle handle, void *block_info);
cmr_s32 isp_u_3dnr_pre_block(isp_handle handle, void *block_info);
#endif
