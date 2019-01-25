#ifndef _SENSOR_RAW_H_
#define _SENSOR_RAW_H_
#include "isp_type.h"
#include "cmr_sensor_info.h"

#define MAX_SCENEMODE_NUM 16
#define MAX_SPECIALEFFECT_NUM 16

#define SENSOR_AWB_CALI_NUM 0x09
#define SENSOR_PIECEWISE_SAMPLE_NUM 0x10
#define SENSOR_ENVI_NUM 6
#define RAW_INFO_END_ID 0x71717567
////////////////////////////////////////////////////////////
/********** shark2 tuning paramater **********/
////////////////////////////////////////////////////////////
#define _HAIT_MODIFIED_FLAG
#define SENSOR_CCE_NUM 0x09
#define SENSOR_CMC_NUM 0x09
#define SENSOR_CTM_NUM 0x09
#define SENSOR_HSV_NUM 0x09
#define SENSOR_LENS_NUM 0x09
#define SENSOR_BLC_NUM 0x09
#define SENSOR_MODE_NUM 0x02
#define SENSOR_AWBM_WHITE_NUM 0x05
#define SENSOR_LNC_RC_NUM 0x04
#define SENSOR_HIST2_ROI_NUM 0x04
#define SENSOR_GAMMA_POINT_NUM	257
#define SENSOR_GAMMA_NUM 9
#define SENSOR_LEVEL_NUM 16
#define SENSOR_CMC_POINT_NUM 9
#define SENSOR_SMART_LEVEL_NUM 25
struct sensor_gamma_curve {
	struct isp_point points[SENSOR_GAMMA_POINT_NUM];
};
/************************************************************************************/
//Pre-Wavelet denoise
/*
PWD:
1. sensor_pwd_shrink will have 3 sets for low light, normal light and high light each
2. sensor_pwd_radial is Algo reserved.
 */
struct sensor_pwd_radial {
	uint32_t radial_bypass;
	struct isp_pos center_pos;
	struct isp_pos center_square;
	uint32_t r2_thr;
	uint32_t p1_param;
	uint32_t p2_param;
	uint16_t gain_max_thr;
	uint16_t reserved0;
};

struct sensor_pwd_level {
	uint16_t gain_thrs0;
	uint16_t gain_thrs1;
	uint16_t nsr_thr_slope;
	uint16_t offset;
	uint16_t bitshif1;
	uint16_t bitshif0;
	uint32_t lum_ratio;
	struct sensor_pwd_radial radial_var;
	uint16_t addback;
	uint16_t reserved1;
};
struct sensor_pwd_param {
	cmr_uint *param_ptr; /* if not null, read from noise/xxxx_param.h */
	struct sensor_pwd_level pwd_level;
	cmr_u32 strength_level;
	cmr_u32 reserved2[2];
};
/************************************************************************************/
//Bad Pixel Correction
/*
BPC:
1. All BPC related parameters are Algo reserved.
2. intercept_b, slope_k and lut level will have 3 sets for low light, normal light and high light.
 */

struct sensor_bpc_flat {
	uint8_t flat_factor;
	uint8_t spike_coeff;
	uint8_t dead_coeff;
	uint8_t reserved;
	uint16_t lut_level[8];
	uint16_t slope_k[8];
	uint16_t intercept_b[8];
};

struct sensor_bpc_rules {
	struct isp_range k_val;
	uint8_t ktimes;
	uint8_t delt34;
	uint8_t safe_factor;
	uint8_t reserved;
};

struct sensor_bpc_comm {
	uint8_t bpc_mode;
	uint8_t mask_mode;
	uint8_t map_done_sel;
	uint8_t new_old_sel;
};

struct sensor_bpc_pvd {
	uint16_t bypass_pvd;
	uint16_t cntr_theshold;
};

struct sensor_bpc_level {
	struct sensor_bpc_flat bpc_flat;
	struct sensor_bpc_rules bpc_rules;
	struct sensor_bpc_pvd bpc_pvd;
};

struct sensor_bpc_param_v1 {
	cmr_uint *param_ptr; /* if not null, read from noise/xxxx_param.h */
	struct sensor_bpc_comm bpc_comm;
	struct sensor_bpc_level bpc_level;
	cmr_u32 strength_level;
	cmr_u32 reserved[2];
};

/************************************************************************************/
//GrGb Correction
struct sensor_grgb_v1_level {
	uint16_t edge_thd;
	uint16_t diff_thd;
	uint16_t grid_thd;
	uint16_t reserved;
};
struct sensor_grgb_v1_param {
	cmr_uint *param_ptr; /* if not null, read from noise/xxxx_param.h */
	struct sensor_grgb_v1_level grgb_level;
	cmr_u32 strength_level;
	cmr_u32 reserved[2];
};
/************************************************************************************/
/*
   1. have 3 sets for low light,normal light and high light;
   2.these parameters should be changed under different iso value;
 */
struct sensor_nlm_flat_degree {
	uint8_t strength;
	uint8_t cnt;
	uint16_t thresh;
};
/*
   1.have 3 sets for low light, normal light and high light;
   2.these parameters should be changed under different iso value;
 */
struct sensor_nlm_flat {
	uint8_t flat_opt_bypass;
	uint8_t flat_thresh_bypass;//algorithm reserved
	uint16_t flat_opt_mode;
	struct sensor_nlm_flat_degree dgr_ctl[5];
};

/*
   1.have  3 sets for low light,normal light and high light;
   2.these parameters should be changed under different iso value;
 */
struct sensor_nlm_direction {
	uint8_t direction_mode_bypass;
	uint8_t dist_mode;
	uint8_t w_shift[3];
	uint8_t cnt_th;
	uint8_t reserved[2];
	uint16_t diff_th;
	uint16_t tdist_min_th;
};
/*
   1have 3 sets for low light,normal light and high light;
   2.these parameters should be changed under different iso value;
 */
struct sensor_nlm_den_strength {
	uint16_t den_length;
	uint16_t texture_dec;
	uint16_t lut_w[72];
};
/*
   1.have 3 sets for low light,normal light and high light;
   2.these parameters should be changed under different iso value;
 */

struct sensor_nlm_level{
	struct sensor_nlm_flat nlm_flat;
	struct sensor_nlm_direction nlm_dic;
	struct sensor_nlm_den_strength nlm_den;
	uint16_t imp_opt_bypass;
	uint16_t add_back;
};
struct sensor_vst_level{
	uint32_t vst_param[1024];
};

struct sensor_ivst_level{
	uint32_t ivst_param[1024];
};

struct sensor_flat_offset_level{
	uint32_t flat_offset_param[1024];
};

struct sensor_nlm_param {
	cmr_uint *param_nlm_ptr; /* if not null, read from noise/xxxx_param.h */
	cmr_uint * param_vst_ptr; /* if not null, read from noise/xxxx_param.h */
	cmr_uint * param_ivst_ptr; /* if not null, read from noise/xxxx_param.h */
	cmr_uint * param_flat_offset_ptr; /* if not null, read from noise/xxxx_param.h */
	struct sensor_nlm_level nlm_level;
	struct sensor_vst_level vst_level;
	struct sensor_ivst_level ivst_level;
	struct sensor_flat_offset_level flat_offset_level;
	cmr_uint strength_level;
	cmr_uint reserved[2];
};

/************************************************************************************/
// CFA
/*
CFA: All CFA related parameters are Algo reserved.
 */

struct sensor_cfae_comm {
	uint8_t grid_gain;
	uint8_t avg_mode;
	uint8_t inter_chl_gain;		// CFA inter-channel color gradient gain typical:1(suggest to keep the typical value)
	uint8_t reserved0;
	uint16_t grid_min_tr;
	uint16_t reserved1;
};

struct sensor_cfae_ee_thr {
	uint16_t cfai_ee_edge_tr;
	uint16_t cfai_ee_diagonal_tr;
	uint16_t cfai_ee_grid_tr;
	uint16_t cfai_doee_clip_tr;
	uint16_t strength_tr_pos;
	uint16_t strength_tr_neg;
};

struct sensor_cfae_ee {
	uint16_t cfa_ee_bypass;
	uint16_t doee_base;
	uint16_t cfai_ee_saturation_level;
	uint16_t ee_strength_pos;
	uint16_t ee_strength_neg;
	uint16_t reserved;
	struct sensor_cfae_ee_thr ee_thr;
};

struct sensor_cfae_undir {
	uint16_t cfa_uni_dir_intplt_tr;
	uint16_t cfai_ee_uni_dir_tr;
	uint16_t plt_diff_tr;
	uint16_t reserved;
};

struct sensor_cfae_level {
	struct sensor_cfae_comm cfa_comm;
	struct sensor_cfae_ee cfa_ee;
	struct sensor_cfae_undir cfa_undir;
};

struct sensor_cfa_param_v1 {
	cmr_uint *param_ptr; /* if not null, read from noise/xxxx_param.h */
	struct sensor_cfae_level cfae_level;
	cmr_u32 strength_level;
	cmr_u32 reserved[2];
};

/************************************************************************************/
//pre-color noise remove in rgb domain

struct sensor_rgb_precdn_level{
	uint16_t thru0;
	uint16_t thru1;
	uint16_t thrv0;
	uint16_t thrv1;
	uint16_t median_mode;
	uint16_t median_thr;
};

struct sensor_rgb_precdn_param {
	cmr_uint *param_ptr; /* if not null, read from noise/xxxx_param.h */
	struct sensor_rgb_precdn_level rgb_precdn_level;
	uint32_t strength_level;
	cmr_u32 reserved[2];
};

/************************************************************************************/
//Auto-exposure monitor in YIQ domain
/*
GMA:
GMA will have two sets for indoor and outdoor seperately.
 */
struct sensor_y_aem_param {
	uint32_t y_gamma_bypass;
	uint32_t skip_num;
	uint32_t cur_index;
	struct isp_pos win_start;
	struct isp_size win_size;
	struct sensor_gamma_curve gamma_tab[9];		////should change to struct sensor_y_gamma tab;  (yangyang.liu)
};



/************************************************************************************/
//Anti-flicker
struct sensor_y_afl_param {
	uint8_t skip_num;
	uint8_t line_step;
	uint8_t frm_num;
	uint8_t reserved0;
	uint16_t v_height;
	uint16_t reserved1;
	struct isp_pos col_st_ed;
};



/************************************************************************************/
// AF monitor in YUV domain
struct sensor_y_afm_level {
	uint32_t iir_bypass;
	uint8_t skip_num;
	uint8_t afm_format;	//filter choose control
	uint8_t afm_position_sel;	//choose afm after CFA or auto contrust adjust
	uint8_t shift;
	uint16_t win[25][4];
	uint16_t coef[11];		//int16
	uint16_t reserved1;
};

struct sensor_y_afm_param {
	cmr_uint *param_ptr; /* if not null, read from noise/xxxx_param.h */
	struct sensor_y_afm_level y_afm_level;
	uint32_t strength_level;
	cmr_u32 reserved[2];
};
/************************************************************************************/
//pre-color denoise in YUV domain

struct sensor_yuv_precdn_comm {
	uint8_t mode;
	uint8_t median_writeback_en;
	uint8_t median_mode;
	uint8_t den_stren;
	uint8_t uv_joint;
	uint8_t median_thr_u[2];
	uint8_t median_thr_v[2];
	uint16_t median_thr;
	uint8_t uv_thr;
	uint8_t y_thr;
	uint8_t reserved[3];
};

struct sensor_yuv_precdn_level{
	struct sensor_yuv_precdn_comm precdn_comm;
	uint8_t r_segu[2][7];							// param1
	uint8_t r_segv[2][7];							// param2
	uint8_t r_segy[2][7];							// param3
	uint8_t dist_w[25];							// param4
	uint8_t reserved0;
};

struct sensor_yuv_precdn_param {
	cmr_uint *param_ptr; /* if not null, read from noise/xxxx_param.h */
	struct sensor_yuv_precdn_level yuv_precdn_level;
	cmr_u32 strength_level;
	cmr_u32 reserved3[2];
};

/************************************************************************************/
//pre-filter in YUV domain
struct sensor_prfy_level {
	uint16_t prfy_writeback;
	uint16_t nr_thr_y;
};

struct sensor_prfy_param {
	cmr_uint *param_ptr; /* if not null, read from noise/xxxx_param.h */
	struct sensor_prfy_level prfy_level;
	cmr_u32 strength_level;
	cmr_u32 reserved3[2];
};


/************************************************************************************/
//Color Denoise

struct sensor_uv_cdn_level {
	uint8_t median_thru0;
	uint8_t median_thru1;
	uint8_t median_thrv0;
	uint8_t median_thrv1;
	uint8_t u_ranweight[31];
	uint8_t v_ranweight[31];
	uint8_t cdn_gaussian_mode;
	uint8_t median_mode;
	uint8_t median_writeback_en;
	uint8_t filter_bypass;
	uint16_t median_thr;
};

struct sensor_uv_cdn_param {
	cmr_uint *param_ptr; /* if not null, read from noise/xxxx_param.h */
	struct sensor_uv_cdn_level uv_cdn_level;
	cmr_u32 strength_level;
	cmr_u32 reserved2[2];
};

/************************************************************************************/
//Edge Enhancement
struct sensor_ee_pn {
	uint8_t negative;
	uint8_t positive;
};
/*
sensor_ee_flat:
All parameters are Algo reserved.
 */
struct sensor_ee_flat {
	uint8_t thr1;
	uint8_t thr2;
	uint16_t reserved;
};
/*
sensor_ee_txt:
All parameters are Algo reserved.
 */
struct sensor_ee_txt {
	uint8_t ee_txt_thr1;
	uint8_t ee_txt_thr2;
	uint8_t ee_txt_thr3;
	uint8_t reserved;
};

/*
sensor_ee_corner:
ee_corner_th, ee_corner_gain and ee_corner_sm are Algo reserved.
 */
struct sensor_ee_corner {
	uint8_t ee_corner_cor;
	uint8_t reserved0[3];
	struct sensor_ee_pn ee_corner_th;
	struct sensor_ee_pn ee_corner_gain;
	struct sensor_ee_pn ee_corner_sm;
	uint16_t reserved1;
};
/*
sensor_ee_smooth:
All parameters are Algo reserved.
 */
struct sensor_ee_smooth {
	uint8_t ee_edge_smooth_mode;
	uint8_t ee_flat_smooth_mode;
	uint8_t ee_smooth_thr;
	uint8_t ee_smooth_strength;
};
/*
sensor_ee_ipd:
ipd_flat_thr is Algo reserved.
 */
struct sensor_ee_ipd {
	uint8_t ipd_bypass;
	uint8_t ipd_flat_thr;
	uint16_t reserved;
};
/*
sensor_ee_ratio:
All parameteres are Algo reserved.
 */
struct sensor_ee_ratio {
	uint8_t ratio1;
	uint8_t ratio2;
	uint16_t reserved;
};
/*
sensor_ee_t_cfg:
All parameters are Algo reserved.
 */
struct sensor_ee_t_cfg {
	uint16_t ee_t1_cfg;
	uint16_t ee_t2_cfg;
	uint16_t ee_t3_cfg;
	uint16_t ee_t4_cfg;
};
/*
sensor_ee_r_cfg:
All paramteres are Algo reserved.
 */
struct sensor_ee_r_cfg {
	uint8_t ee_r1_cfg;
	uint8_t ee_r2_cfg;
	uint8_t ee_r3_cfg;
	uint8_t reserved;
};
/*
sensor_ee_param:
ee_mode, sigma and chip_after_smooth_en are Algo reserved.
There will be at least 3 sets of edge_thr_d for low light, normal light and high light.
 */

struct sensor_ee_level{
	struct sensor_ee_pn ee_str_m;
	struct sensor_ee_pn ee_str_d;
	struct sensor_ee_pn ee_incr_d;
	struct sensor_ee_pn ee_thr_d;
	struct sensor_ee_pn ee_incr_m;
	struct sensor_ee_pn ee_incr_b;
	struct sensor_ee_pn ee_str_b;
	struct sensor_ee_pn ee_cv_clip_x;
	struct sensor_ee_flat ee_flat_thrx;
	struct sensor_ee_txt ee_txt_thrx;
	struct sensor_ee_corner ee_corner_xx;
	struct sensor_ee_smooth ee_smooth_xx;
	struct sensor_ee_ipd ipd_xx;
	struct sensor_ee_ratio ratio;
	struct sensor_ee_t_cfg ee_tx_cfg;
	struct sensor_ee_r_cfg ee_rx_cfg;
	uint8_t mode;
	uint8_t sigma;
	uint8_t ee_clip_after_smooth_en;
	uint8_t reserved0;
};

struct sensor_ee_param {
	cmr_uint *param_ptr; /* if not null, read from noise/xxxx_param.h */
	struct sensor_ee_level ee_level;
	cmr_u32 strength_level;
	cmr_u32 cur_idx;
	cmr_s8 tab[SENSOR_LEVEL_NUM];
	cmr_s8 scenemode[MAX_SCENEMODE_NUM];
	cmr_u32 reserved[2];
};
/************************************************************************************/
//post-color denoise
struct sensor_postcdn_thr {
	uint16_t thr0;//algorithm reserved
	uint16_t thr1;//algorithm reserved
};

struct sensor_postcdn_r_seg {
	uint8_t r_seg0[7];
	uint8_t r_seg1[7];
	uint8_t reserved0[2];
};

struct sensor_postcdn_distw {
	uint8_t distw[15][5];
	uint8_t reserved0;
};

/*
   uint16_t adpt_med_thr;							//have 3 sets for low light,normal light and high light
   uint8_t r_segu[2][7];
   uint8_t r_segv[2][7];
   uint8_t r_distw[15][5];
 */
struct sensor_uv_postcdn_level{
	struct sensor_postcdn_r_seg r_segu;
	struct sensor_postcdn_r_seg r_segv;
	struct sensor_postcdn_distw r_distw;
	struct sensor_postcdn_thr thruv;
	struct sensor_postcdn_thr thru;
	struct sensor_postcdn_thr thrv;
	uint16_t adpt_med_thr;
	uint8_t median_mode;
	uint8_t uv_joint;
	uint8_t median_res_wb_en;
	uint8_t postcdn_mode;
	uint8_t downsample_bypass;
};

struct sensor_uv_postcdn_param {
	cmr_uint *param_ptr; /* if not null, read from noise/xxxx_param.h */
	struct sensor_uv_postcdn_level uv_postcdn_level;
	cmr_u16 strength_level;
	cmr_u32 reserved[2];
};

/************************************************************************************/
//IIR Noise Remove
/*
sensor_iircnr_iir:
the following parameters in sensor_iircnr_iir are Algo reserved:
iircnr_iir_mode
iircnr_uv_pg_th
iircnr_uv_s_th
iircnr_uv_low_thr2
iircnr_slope
iircnr_middle_factor
iircnr_y_th :have at least 3 sets for low light, normal light and high light each
iircnr_ymd_u :have at least 3 sets for low light, normal light and high light each
iircnr_ymd_v :have at least 3 sets for low light, normal light and high light each
iircnr_uv_high_thr2 : have at least 3 sets for low light, normal light and high light each
iircnr_uv_th :have at least 3 sets for low light, normal light and high light each
iircnr_uv_dist :have at least 3 sets for low light, normal light and high light each

the following  parameters in sensor_iircnr_iir will have at least 3 sets for low light, normal light and high light each:
iircnr_uv_low_thr1
iircnr_alpha_low_u
iircnr_alpha_low_v

 */

struct sensor_iircnr_level{
	uint8_t   iircnr_iir_mode;
	uint8_t   iircnr_y_th;
	uint8_t   reserved0[2];
	uint16_t iircnr_uv_th;
	uint16_t iircnr_uv_dist;
	uint16_t iircnr_uv_pg_th;
	uint16_t iircnr_uv_low_thr1;
	uint16_t iircnr_uv_low_thr2;
	uint16_t reserved1;
	uint32_t iircnr_ymd_u;
	uint32_t iircnr_ymd_v;
	uint8_t   iircnr_uv_s_th;
	uint8_t   iircnr_slope;
	uint8_t   reserved2[2];
	uint16_t iircnr_alpha_low_u;
	uint16_t iircnr_alpha_low_v;
	uint32_t iircnr_middle_factor;
	uint32_t iircnr_uv_high_thr2;
};

struct sensor_iircnr_param {
	cmr_uint *param_ptr; /* if not null, read from noise/xxxx_param.h */
	struct sensor_iircnr_level iircnr_level;
	cmr_u32 strength_level;
	cmr_u32 reserved[2];
};
/*
sensor_iircnr_yrandom:All parameters are Algo reserved
 */
struct sensor_iircnr_yrandom_level {
	uint8_t   yrandom_shift;
	uint8_t   reserved0[3];
	uint32_t yrandom_seed;
	uint16_t yrandom_offset;
	uint16_t reserved1;
	uint8_t   yrandom_takebit[8];
};

struct sensor_iircnr_yrandom_param {
	cmr_uint *param_ptr; /* if not null, read from noise/xxxx_param.h */
	struct sensor_iircnr_yrandom_level iircnr_yrandom_level;
	cmr_u32 strength_level;
	cmr_u32 reserved[2];
};

/************************************************************************************/
//bilateral denoise

struct sensor_bdn_level {
	cmr_u8 diswei_tab[10][19];
	cmr_u8 ranwei_tab[10][31];
	cmr_u32 radial_bypass;
	cmr_u32 addback;
	struct isp_pos center;
	struct isp_pos delta_sqr;
	struct isp_curve_coeff coeff;
	cmr_u32 dis_sqr_offset;
};

struct sensor_bdn_param {
	cmr_uint *param_ptr; /* if not null, read from noise/xxxx_param.h */
	struct sensor_bdn_level bdn_level;
	cmr_u32 strength_level;
	cmr_u32 reserved[2];
};
/************************************************************************************/
/*
uvdiv:
1. All uvdiv parameters are reserved.
2. There will be at least 2 sets of uvdiv parameters for different illuminance.
 */
struct sensor_cce_uvdiv_level{
	uint8_t lum_th_h_len;
	uint8_t lum_th_h;
	uint8_t lum_th_l_len;
	uint8_t lum_th_l;
	uint8_t chroma_min_h;
	uint8_t chroma_min_l;
	uint8_t chroma_max_h;
	uint8_t chroma_max_l;
	uint8_t u_th1_h;
	uint8_t u_th1_l;
	uint8_t u_th0_h;
	uint8_t u_th0_l;
	uint8_t v_th1_h;
	uint8_t v_th1_l;
	uint8_t v_th0_h;
	uint8_t v_th0_l;
	uint8_t ratio[9];
	uint8_t base;
	uint8_t reserved0[2];
};

struct sensor_cce_uvdiv_param_v1 {
	cmr_uint *param_ptr; /* if not null, read from noise/xxxx_param.h */
	struct sensor_cce_uvdiv_level cce_uvdiv_level;
	cmr_u32 strength_level;
	cmr_u32 reserved1[2];
};
//////////////////////////////////////////////////
/********** parameter block definition **********/
//////////////////////////////////////////////////


enum ISP_BLK_ID {
	ISP_BLK_ID_BASE = 0,


	ISP_BLK_SHARKL_BASE					= 0x1000,
	ISP_BLK_FLASH_CALI = 0x1018,
	ISP_BLK_ENVI_DETECT = 0x1028,

	//TSHARK2
	ISP_BLK_SHARK2_BASE =				0x2000,
	ISP_BLK_PRE_GBL_GAIN_V1 = 0x2001,
	ISP_BLK_BLC_V1 = 0x2002,
	ISP_BLK_RGB_GAIN_V1 = 0x2003,
	ISP_BLK_PRE_WAVELET_V1 = 0x2004,
	ISP_BLK_NLC_V1 = 0x2005,
	ISP_BLK_2D_LSC = 0x2006,
	ISP_BLK_1D_LSC = 0x2007,
	ISP_BLK_BINNING4AWB_V1 = 0x2008,
	ISP_BLK_AWB_V1 = 0x2009,
	ISP_BLK_AE_V1 = 0x200A,
	ISP_BLK_BPC_V1 = 0x200B,
	ISP_BLK_BL_NR_V1 = 0x200C,
	ISP_BLK_GRGB_V1 = 0x200D,
	ISP_BLK_RGB_GAIN2 = 0x200E,
	ISP_BLK_NLM = 0x200F, // + ISP_BLK_VST + ISP_BLK_IVST
	ISP_BLK_CFA_V1 = 0x2010,
	ISP_BLK_CMC10 = 0x2011,
	ISP_BLK_RGB_GAMC = 0x2012,
	ISP_BLK_CMC8 = 0x2013,
	ISP_BLK_CTM = 0x2014,
	ISP_BLK_CCE_V1 = 0x2015,
	ISP_BLK_HSV = 0x2016,
	ISP_BLK_RADIAL_CSC = 0x2017,
	ISP_BLK_RGB_PRECDN = 0x2018,
	ISP_BLK_POSTERIZE = 0x2019,
	ISP_BLK_AF_V1 = 0x201A,
	ISP_BLK_YIQ_AEM = 0x201B,
	ISP_BLK_YIQ_AFL = 0x201C,
	ISP_BLK_YIQ_AFM = 0x201D,
	ISP_BLK_YUV_PRECDN = 0x201E,
	ISP_BLK_PREF_V1 = 0x201F,
	ISP_BLK_BRIGHT = 0x2020,
	ISP_BLK_CONTRAST = 0x2021,
	ISP_BLK_HIST_V1 = 0x2022,
	ISP_BLK_HIST2 = 0x2023,
	ISP_BLK_AUTO_CONTRAST_V1 = 0x2024,//auto-contrast
	ISP_BLK_UV_CDN = 0x2025,
	ISP_BLK_EDGE_V1 = 0x2026,
	ISP_BLK_EMBOSS_V1 = 0x2027,
	ISP_BLK_CSS_V1 = 0x2028,
	ISP_BLK_SATURATION_V1 = 0x2029,
	ISP_BLK_HUE_V1 = 0x202A,
	ISP_BLK_UV_POSTCDN = 0x202B,
	ISP_BLK_Y_GAMMC = 0x202D,
	ISP_BLK_YDELAY = 0x202E,
	ISP_BLK_IIRCNR_IIR = 0x202F,
	ISP_BLK_UVDIV_V1 = 0x2030,
	ISP_BLK_IIRCNR_YRANDOM = 0x2031,

	ISP_BLK_SMART = 0x2032,

	//pike
	//	ISP_BLK_PIKE_BASE =  0x3000,
	//	ISP_BLK_WDR = 0x3003,
	ISP_BLK_RGB2Y = 0x3004,
	ISP_BLK_UV_PREFILTER = 0x3007,
	ISP_BLK_YUV_NLM = 0x3008,

	ISP_BLK_EXT,
	ISP_BLK_ID_MAX,
};
#endif
