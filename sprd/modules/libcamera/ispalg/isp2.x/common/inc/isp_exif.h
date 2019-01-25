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
#ifndef _ISP_EXIF_H_
#define _ISP_EXIF_H_

#include "cmr_types.h"

#define EXIF_APP3	0XFFE3
#define APP3_STATUS	0X1234
struct exif_blc_param {
	cmr_u32 mode;
	cmr_u16 r;
	cmr_u16 gr;
	cmr_u16 gb;
	cmr_u16 b;
};

struct exif_nlc_param {
	cmr_u16 r_node[29];
	cmr_u16 g_node[29];
	cmr_u16 b_node[29];
	cmr_u16 l_node[29];
};

struct exif_lnc_param {
	cmr_u16 grid;
	cmr_u16 r_pec;
	cmr_u16 g_pec;
	cmr_u16 b_pec;
};

struct exif_awb_map {
	cmr_u16 *addr;
	cmr_u32 len;				//by bytes
};

struct exif_ae_param {
	cmr_u32 iso;
	cmr_u32 exposure;
	cmr_u32 gain;
	cmr_u32 cur_lum;
};

struct exif_awb_param {
	cmr_u16 alg_id;
	cmr_u16 r_gain;
	cmr_u16 g_gain;
	cmr_u16 b_gain;
};

struct exif_bpc_param {
	cmr_u16 flat_thr;
	cmr_u16 std_thr;
	cmr_u16 texture_thr;
	cmr_u16 reserved;
};

struct exif_denoise_param {
	cmr_u32 write_back;
	cmr_u16 r_thr;
	cmr_u16 g_thr;
	cmr_u16 b_thr;
	cmr_u8 diswei[19];
	cmr_u8 ranwei[31];
	cmr_u8 reserved1;
	cmr_u8 reserved0;
};

struct exif_grgb_param {
	cmr_u16 edge_thr;
	cmr_u16 diff_thr;
};

struct exif_cfa_param {
	cmr_u16 edge_thr;
	cmr_u16 diff_thr;
};

struct exif_cmc_param {
	cmr_u16 matrix[9];
	cmr_u16 reserved;
};

struct exif_cce_parm {
	cmr_u16 matrix[9];
	cmr_u16 y_shift;
	cmr_u16 u_shift;
	cmr_u16 v_shift;
};

struct exif_gamma_param {
	cmr_u16 axis[2][26];
};

struct exif_cce_uvdiv {
	cmr_u8 thrd[7];
	cmr_u8 t[2];
	cmr_u8 m[3];
};

struct exif_pref_param {
	cmr_u8 write_back;
	cmr_u8 y_thr;
	cmr_u8 u_thr;
	cmr_u8 v_thr;
};

struct exif_bright_param {
	cmr_u8 factor;
};

struct exif_contrast_param {
	cmr_u8 factor;
};

struct exif_hist_param {
	cmr_u16 low_ratio;
	cmr_u16 high_ratio;
	cmr_u8 mode;
	cmr_u8 reserved2;
	cmr_u8 reserved1;
	cmr_u8 reserved0;
};

struct exif_auto_contrast_param {
	cmr_u8 mode;
	cmr_u8 reserved2;
	cmr_u8 reserved1;
	cmr_u8 reserved0;
};

struct exif_saturation_param {
	cmr_u8 factor;
};

struct exif_af_param {
	cmr_u8 magic[16];
	cmr_u16 alg_id;
	cmr_u16 cur_step;
	cmr_u16 edge_info[32];
	cmr_u32 denoise_lv;
	cmr_u32 win_num;
	cmr_u32 suc_win;
	cmr_u32 mode;
	cmr_u32 step_cnt;
	cmr_u16 win[9][4];
	cmr_u16 pos[32];
	cmr_u32 value[9][32];
	cmr_u16 time[32];
};

struct exif_emboss_param {
	cmr_u8 step;
	cmr_u8 reserved2;
	cmr_u8 reserved1;
	cmr_u8 reserved0;
};

struct exif_edge_info {
	cmr_u8 detail_thr;
	cmr_u8 smooth_thr;
	cmr_u8 strength;
	cmr_u8 reserved;
};

struct exif_global_gain_param {
	cmr_u32 gain;
};

struct exif_chn_gain_param {
	cmr_u8 r_gain;
	cmr_u8 g_gain;
	cmr_u8 b_gain;
	cmr_u8 reserved0;
	cmr_u16 r_offset;
	cmr_u16 g_offset;
	cmr_u16 b_offset;
	cmr_u16 reserved1;
};

struct exif_flash_cali_param {
	cmr_u16 effect;
	cmr_u16 lum_ratio;
	cmr_u16 r_ratio;
	cmr_u16 g_ratio;
	cmr_u16 b_ratio;
};

struct exif_css_param {
	cmr_u8 low_thr[7];
	cmr_u8 lum_thr;
	cmr_u8 low_sum_thr[7];
	cmr_u8 chr_thr;
	cmr_u8 ratio[8];
};

struct eixf_read_check {
	cmr_u16 app_head;
	cmr_u16 status;
};
typedef struct exif_isp_info {
	cmr_u32 is_exif_validate;
	cmr_u32 tool_version;
	cmr_u32 version_id;
	cmr_u32 info_len;
	cmr_u32 blc_bypass;
	cmr_u32 nlc_bypass;
	cmr_u32 lnc_bypass;
	cmr_u32 ae_bypass;
	cmr_u32 awb_bypass;
	cmr_u32 bpc_bypass;
	cmr_u32 denoise_bypass;
	cmr_u32 grgb_bypass;
	cmr_u32 cmc_bypass;
	cmr_u32 gamma_bypass;
	cmr_u32 uvdiv_bypass;
	cmr_u32 pref_bypass;
	cmr_u32 bright_bypass;
	cmr_u32 contrast_bypass;
	cmr_u32 hist_bypass;
	cmr_u32 auto_contrast_bypass;
	cmr_u32 af_bypass;
	cmr_u32 edge_bypass;
	cmr_u32 fcs_bypass;
	cmr_u32 css_bypass;
	cmr_u32 saturation_bypass;
	cmr_u32 hdr_bypass;
	cmr_u32 glb_gain_bypass;
	cmr_u32 chn_gain_bypass;

	struct exif_blc_param blc;
	struct exif_nlc_param nlc;
	struct exif_lnc_param lnc;
	struct exif_ae_param ae;
	struct exif_awb_param awb;
	struct exif_bpc_param bpc;
	struct exif_denoise_param denoise;
	struct exif_grgb_param grgb;
	struct exif_cfa_param cfa;
	struct exif_cmc_param cmc;
	struct exif_gamma_param gamma;
	struct exif_cce_parm cce;
	struct exif_cce_uvdiv uv_div;
	struct exif_pref_param pref;
	struct exif_bright_param bright;
	struct exif_contrast_param contrast;
	struct exif_hist_param hist;
	struct exif_auto_contrast_param auto_contrast;
	struct exif_saturation_param saturation;
	struct exif_css_param css;
	struct exif_af_param af;
	struct exif_edge_info edge;
	struct exif_emboss_param emboss;
	struct exif_global_gain_param global;
	struct exif_chn_gain_param chn;
	struct exif_flash_cali_param flash;
	struct eixf_read_check exif_check;

} EXIF_ISP_INFO_T;

typedef struct exif_isp_debug_info {
	void *addr;
	cmr_s32 size;
} EXIF_ISP_DEBUG_INFO_T;

#endif
