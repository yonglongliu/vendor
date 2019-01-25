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

 /* Version = 1.0 */
#ifndef _ISP_DEBUG_INFO_H_
#define _ISP_DEBUG_INFO_H_

#define AE_DEBUG_VERSION_ID (1)
#define AF_DEBUG_VERSION_ID (1)
#define AWB_DEBUG_VERSION_ID (1)

typedef enum {
	EM_AE_MODE,
	EM_AWB_MODE,
	EM_AF_MODE,
} EM_MODE_INFO;

struct debug_ae_param {
	cmr_u8 magic[16];
	cmr_u32 version;			/*version No. for this structure */
	char alg_id[32];
	cmr_u32 lock_status;		/*0: unlock; 1: lock */
	cmr_u32 FD_AE_status;		/*0: FD-AE is on; 1: FD-AE is off */
	cmr_u32 cur_lum;			/*the average of image:0 ~255 */
	cmr_u32 target_lum;			/*the ae target lum: 0 ~255 */
	cmr_u32 target_zone;		/*ae target lum stable zone: 0~255 */
	cmr_u32 target_lum_ori;
	cmr_u32 target_zone_ori;
	cmr_u32 max_index;			/*the max index in current ae table: 1~1024 */
	cmr_u32 cur_index;			/*the current index of ae table in ae now: 1~1024 */
	cmr_u32 min_index;			/*the mini index in current ae table: 1~1024 */
	cmr_u32 max_fps;			/*the max fps: 1~120 */
	cmr_u32 min_fps;			/*the mini fps:1~120 */
	cmr_u32 exposure_time;		/*exposure time for current frame */
	cmr_u32 cur_fps;			/*current fps:1~120 */
	cmr_u32 cur_ev;				/*ev index: 0~15 */
	cmr_u32 cur_metering_mode;	/*current metering mode */
	cmr_u32 cur_exp_line;		/*current exposure line: the value is related to the resolution */
	cmr_u32 cur_dummy;			/*dummy line: the value is related to the resolution & fps */
	cmr_u32 cur_again;			/*current analog gain */
	cmr_u32 cur_dgain;			/*current digital gain */
	cmr_u32 cur_iso;			/*current iso mode */
	cmr_u32 cur_flicker;		/*flicker mode:0: 50HZ;1:60HZ */
	cmr_u32 cur_scene;			/*current scene mode */
	cmr_u32 cur_bv;				/*bv parameter */
	cmr_u32 flash_effect;		/*flash effect ratio */
	cmr_u32 is_stab;			/*ae stable status */
	cmr_u32 line_time;			/*exposure time for one line */
	cmr_u32 frame_line;			/*frame line length */
	cmr_u32 cur_work_mode;		/*current work mode */
	cmr_u32 histogram[256];		/*luma histogram of current frame */
	cmr_u32 r_stat_info[1024];	/*r channel of ae statics state data */
	cmr_u32 g_stat_info[1024];	/*g channel of ae statics state data */
	cmr_u32 b_stat_info[1024];	/*b channel of ae statics state data */
	cmr_u8 TC_AE_status;		/*touch ae enable */
	cmr_s8 TC_target_offset;	/*touch target offset */
	cmr_u16 TC_cur_lum;			/*touch  lum */

	cmr_s8 mulaes_target_offset;
	cmr_s8 region_target_offset;
	cmr_s8 flat_target_offset;
	cmr_s8 reserved0;
	cmr_s32 reserved[1014];		/*resurve for future */
};

struct debug_awb_param {
	cmr_u8 magic[16];
	cmr_u32 version;

	cmr_u32 r_gain;
	cmr_u32 g_gain;
	cmr_u32 b_gain;
	cmr_u32 cur_ct;

	cmr_u32 cur_awb_mode;
	cmr_u32 cur_work_mode;

	/* awb calibration of golden sensor */
	cmr_u32 golden_r;
	cmr_u32 golden_g;
	cmr_u32 golden_b;

	/* awb calibration of random sensor */
	cmr_u32 random_r;
	cmr_u32 random_g;
	cmr_u32 random_b;

	cmr_s32 cur_bv;				/*current bv */
	cmr_s32 cur_iso;			/*current iso value */

	cmr_u32 r_stat_info[1024];	/*r channel of awb statics state data */
	cmr_u32 g_stat_info[1024];	/*g channel of awb statics state data */
	cmr_u32 b_stat_info[1024];	/*b channel of awb statics state data */

	cmr_u32 reserved[256];
};

struct debug_af_param {
	// System
	cmr_u8 magic[16];
	cmr_u32 version;			/*version No. for this structure */
	cmr_u32 af_start_time[50];	/*[50]:AF execute frame; record start time of AF start to execute in each frame */
	cmr_u32 af_end_time[50];	/*[50]:AF execute frame; record end time of AF end to execute in each frame */
	cmr_u32 work_time;			/*work time = AF finish time - AF start time */

	// From AE
	cmr_u16 bAEisConverge;		/*flag: check AE is converged or not */
	cmr_u16 AE_BV;				/*brightness value */
	cmr_u16 AE_EXP;				/*exposure time (ms) */
	cmr_u16 AE_Gain;			/*128: gain1x = 128 */

	// AF INFO
	cmr_u8 af_mode;				/*0: manual; 1: auto; 2: macro; 3: INF; 4: hyper */
	cmr_u8 af_search_type;		/*0: auto; 1: type1; 2: type2; 3: type3; 4: type4; 5: type5 */
	cmr_u8 af_filter_type[6];	/*[6]: six filters; af_filter_type[i]:0: auto; 1: type1; 2: type2; 3: type3; 4: type4; 5: type5 */
	cmr_u8 face_zone[4];		/*face_zone[0]:X_POS; face_zone[1]:Y_POS; face_zone[2]:Width;face_zone[3]:Height; */
	cmr_u32 win_num;			/*number of focus windows: 1~25 */
	cmr_u32 work_mode;			/*0: SAF; 1: CAF; 2: Face-AF; 3: multi-AF; 4: touch AF */
	cmr_u16 win[25][4];			/*win[0][0]:zone0_X_POS; win[0][1]:zone0_Y_POS; win[0][2]:zone0_Width; win[0][3]:zone0_Height; */
	cmr_u32 start_pos;			/*focus position before AF work */

	//Search data
	cmr_u16 scan_pos[2][50];	/*[2]:rough/fine search; [50]:AF execute frame; record each scan position during foucsing */
	cmr_u32 af_value[2][6][25][50];	/*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; record focus value of each frame during foucsing */
	cmr_u32 af_pixel_mean[2][4][25][50];	/*[2]:rough/fine search; [4]:color order: 0:R, 1:Gr, 2:Gb, 3:B; [25]:focus zone; [50]:AF execute frame; record pixel mean of each AF zone during foucsing */
	cmr_u16 his_cov[2][25][50];	/*[2]:rough/fine search; [25]:focus zone; [50]:AF execute frame; record each covariance of histogram of each AF zone during foucsing */
	cmr_u8 MaxIdx[2][6][25][50];	/*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; index of max statistic value */
	cmr_u8 MinIdx[2][6][25][50];	/*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; index of min statistic value */
	cmr_u8 BPMinIdx[2][6][25][50];	/*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; index of min statistic value before peak */
	cmr_u8 APMinIdx[2][6][25][50];	/*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; index of min statistic value after peak */
	cmr_u8 ICBP[2][6][25][50];	/*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; increase count before peak */
	cmr_u8 DCBP[2][6][25][50];	/*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; decrease count before peak */
	cmr_u8 EqBP[2][6][25][50];	/*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; equal count before peak */
	cmr_u8 ICAP[2][6][25][50];	/*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; increase count after peak */
	cmr_u8 DCAP[2][6][25][50];	/*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; decrease count after peak */
	cmr_u8 EqAP[2][6][25][50];	/*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; equal count after peak */
	cmr_s8 BPC[2][6][25][50];	/*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; ICBP - DCBP */
	cmr_s8 APC[2][6][25][50];	/*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; ICAP - DCAP */
	cmr_u16 BP_R10000[2][4][6][25][50];	/*[2]:rough/fine search; [4]:four type; [6]:six filters; [25]:focus zone; [50]:AF execute frame; FV ratio X 10000 before peak */
	cmr_u16 AP_R10000[2][4][6][25][50];	/*[2]:rough/fine search; [4]:four type; [6]:six filters; [25]:focus zone; [50]:AF execute frame; FV ratio X 10000 after peak */
	cmr_s32 fv_slope_10000[2][4][6][25][50];	/*[2]:rough/fine search; [4]:four type; [6]:six filters; [25]:focus zone; [50]:AF execute frame; record slope of focus value during this foucs */
	cmr_u32 pdaf_confidence[64];	/*[64]: PD zone; confidence value of phase detection */
	cmr_s16 pdaf_diff[64];		/*[64]: PD zone; different value of phase detection */

	//AF threshold
	cmr_u16 UB_Ratio_TH[2][4][6];	/*[2]:rough/fine search; [4]:four type; [6]:six filters; The Up bound threshold of FV ratio, type[0]:total, type[1]:3 sample, type[2]:5 sample, type[3]:7 sample */
	cmr_u16 LB_Ratio_TH[2][4][6];	/*[2]:rough/fine search; [4]:four type; [6]:six filters; The low bound threshold of FV ratio, type[0]:total, type[1]:3 sample, type[2]:5 sample, type[3]:7 sample */

	//AF result
	cmr_u32 suc_win_num;		/*number of windows of sucess focus: 0~25 */
	cmr_u8 result_win[25];		/*[25]: focus zone; result_win[i]: AF result of each zone */
	cmr_u8 reserved0[3];
	cmr_u32 cur_pos;			/*current focus motor position: 0~1023 */
	cmr_u16 peak_pos[2][6][25];	/*[2]:rough/fine search; [6]:six filters; [25]:focus zone;      peak position: 0~1023 */
	cmr_u16 choose_filter;		/*choose which filter is used to final focus peak */
	cmr_u16 choose_zone;		/*choose which zone is used to final focus peak */
	cmr_u8 total_work_frame;	/*total work frames during AF work */
	cmr_u8 rs_total_frame;		/*total work frames during rough search */
	cmr_u8 fs_total_frame;		/*total work frames during fine search */
	cmr_u8 rs_dir;				/*direction of rough search: 0:Far to near, 1:near to far */
	cmr_u8 fs_dir;				/*direction of fine search: 0:Far to near, 1:near to far */
	cmr_u8 reserved1[3];
	//Reserve
	cmr_u32 reserved2[256];		/*resurve for future */
};
/*
struct debug_lsc_param{
	cmr_s32 error[9];
	cmr_s32 eratio_before_smooth[9];
	cmr_s32 eratio_after_smooth[9];
	cmr_s32 final_ratio;
	cmr_s32 final_index;
	cmr_s32 seg_num;
	cmr_u16 smart_lsc_table[1024*4];
	cmr_u16 alsc_lsc_table[1024*4];
	cmr_u16 otp_tab[1024*4];
	cmr_u32 reserved[256];
};
*/

struct smart_component {
	cmr_u32 type;
	cmr_u32 offset;
	cmr_s32 fix_data[4];
};

struct smart_block {
	cmr_u32 block_id;
	cmr_u32 smart_id;
	cmr_u32 component_num;
	struct smart_component component[4];
};

struct debug_smart_param {
	cmr_u32 version;
	struct smart_block block[30];
	cmr_u32 enable_num;
	cmr_u32 reserved[256];
};

typedef struct debug_isp_info {
	struct debug_ae_param ae;
	struct debug_awb_param awb;
	struct debug_af_param af;
	struct debug_smart_param smart;
//       struct debug_lsc_param lsc;
} DEBUG_ISP_INFO_T;

//      added by wnaghao @2015-12-23
//      for real-time set or display
struct debug_ae_display_param {
	cmr_u8 magic[16];
	cmr_u32 version;			/*version No. for this structure */
	cmr_u32 lock_status;		/*0: unlock; 1: lock */
	cmr_u32 FD_AE_status;		/*0: FD-AE is on; 1: FD-AE is off */
	cmr_u32 cur_lum;			/*the average of image:0 ~255 */
	cmr_u32 target_lum;			/*the ae target lum: 0 ~255 */
	cmr_u32 target_zone;		/*ae target lum stable zone: 0~255 */
	cmr_u32 max_index;			/*the max index in current ae table: 1~1024 */
	cmr_u32 cur_index;			/*the current index of ae table in ae now: 1~1024 */
	cmr_u32 min_index;			/*the mini index in current ae table: 1~1024 */
	cmr_u32 max_fps;			/*the max fps: 1~120 */
	cmr_u32 min_fps;			/*the mini fps:1~120 */
	cmr_u32 exposure_time;		/*exposure time for current frame */
	cmr_u32 cur_fps;			/*current fps:1~120 */
	cmr_u32 cur_ev;				/*ev index: 0~15 */
	cmr_u32 cur_metering_mode;	/*current metering mode */
	cmr_u32 cur_exp_line;		/*current exposure line: the value is related to the resolution */
	cmr_u32 cur_dummy;			/*dummy line: the value is related to the resolution & fps */
	cmr_u32 cur_again;			/*current analog gain */
	cmr_u32 cur_dgain;			/*current digital gain */
	cmr_u32 cur_iso;			/*current iso mode */
	cmr_u32 cur_flicker;		/*flicker mode:0: 50HZ;1:60HZ */
	cmr_u32 cur_scene;			/*current scene mode */
	cmr_u32 cur_bv;				/*bv parameter */
	cmr_u32 flash_effect;		/*flash effect ratio */
	cmr_u32 is_stab;			/*ae stable status */
	cmr_u32 line_time;			/*exposure time for one line */
	cmr_u32 frame_line;			/*frame line length */
	cmr_u32 cur_work_mode;		/*current work mode */
	cmr_u32 histogram[256];		/*luma histogram of current frame */
	cmr_u32 r_stat_info[1024];	/*r channel of ae statics state data */
	cmr_u32 g_stat_info[1024];	/*g channel of ae statics state data */
	cmr_u32 b_stat_info[1024];	/*b channel of ae statics state data */
	cmr_u8 TC_AE_status;		/*touch ae enable */
	cmr_s8 TC_target_offset;	/*touch target offset */
	cmr_u16 TC_cur_lum;			/*touch  lum */

	cmr_s8 mulaes_target_offset;
	cmr_s8 region_target_offset;
	cmr_s8 flat_target_offset;
	cmr_s8 reserved0;
	cmr_s32 reserved[1022];		/*resurve for future */
};

struct debug_awb_display_param {
	cmr_u8 magic[16];
	cmr_u32 version;

	cmr_u32 r_gain;
	cmr_u32 g_gain;
	cmr_u32 b_gain;
	cmr_u32 cur_ct;

	cmr_u32 cur_awb_mode;
	cmr_u32 cur_work_mode;

	/* awb calibration of golden sensor */
	cmr_u32 golden_r;
	cmr_u32 golden_g;
	cmr_u32 golden_b;

	/* awb calibration of random sensor */
	cmr_u32 random_r;
	cmr_u32 random_g;
	cmr_u32 random_b;

	cmr_s32 cur_bv;				/*current bv */
	cmr_s32 cur_iso;			/*current iso value */

	cmr_u32 r_stat_info[1024];	/*r channel of awb statics state data */
	cmr_u32 g_stat_info[1024];	/*g channel of awb statics state data */
	cmr_u32 b_stat_info[1024];	/*b channel of awb statics state data */

	cmr_u32 reserved[256];
};

struct debug_af_display_param {
	// System
	cmr_u8 magic[16];
	cmr_u32 version;			/*version No. for this structure */
	cmr_u32 af_start_time[50];	/*[50]:AF execute frame; record start time of AF start to execute in each frame */
	cmr_u32 af_end_time[50];	/*[50]:AF execute frame; record end time of AF end to execute in each frame */
	cmr_u32 work_time;			/*work time = AF finish time - AF start time */

	// From AE
	cmr_u16 bAEisConverge;		/*flag: check AE is converged or not */
	cmr_u16 AE_BV;				/*brightness value */
	cmr_u16 AE_EXP;				/*exposure time (ms) */
	cmr_u16 AE_Gain;			/*128: gain1x = 128 */

	// AF INFO
	cmr_u8 af_mode;				/*0: manual; 1: auto; 2: macro; 3: INF; 4: hyper */
	cmr_u8 af_search_type;		/*0: auto; 1: type1; 2: type2; 3: type3; 4: type4; 5: type5 */
	cmr_u8 af_filter_type[6];	/*[6]: six filters; af_filter_type[i]:0: auto; 1: type1; 2: type2; 3: type3; 4: type4; 5: type5 */
	cmr_u8 face_zone[4];		/*face_zone[0]:X_POS; face_zone[1]:Y_POS; face_zone[2]:Width;face_zone[3]:Height; */
	cmr_u32 win_num;			/*number of focus windows: 1~25 */
	cmr_u32 work_mode;			/*0: SAF; 1: CAF; 2: Face-AF; 3: multi-AF; 4: touch AF */
	cmr_u16 win[25][4];			/*win[0][0]:zone0_X_POS; win[0][1]:zone0_Y_POS; win[0][2]:zone0_Width; win[0][3]:zone0_Height; */
	cmr_u32 start_pos;			/*focus position before AF work */

	//Search data
	cmr_u16 scan_pos[2][50];	/*[2]:rough/fine search; [50]:AF execute frame; record each scan position during foucsing */
	cmr_u32 af_value[2][6][25][50];	/*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; record focus value of each frame during foucsing */
	cmr_u32 af_pixel_mean[2][4][25][50];	/*[2]:rough/fine search; [4]:color order: 0:R, 1:Gr, 2:Gb, 3:B; [25]:focus zone; [50]:AF execute frame; record pixel mean of each AF zone during foucsing */
	cmr_u16 his_cov[2][25][50];	/*[2]:rough/fine search; [25]:focus zone; [50]:AF execute frame; record each covariance of histogram of each AF zone during foucsing */
	cmr_u8 MaxIdx[2][6][25][50];	/*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; index of max statistic value */
	cmr_u8 MinIdx[2][6][25][50];	/*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; index of min statistic value */
	cmr_u8 BPMinIdx[2][6][25][50];	/*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; index of min statistic value before peak */
	cmr_u8 APMinIdx[2][6][25][50];	/*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; index of min statistic value after peak */
	cmr_u8 ICBP[2][6][25][50];	/*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; increase count before peak */
	cmr_u8 DCBP[2][6][25][50];	/*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; decrease count before peak */
	cmr_u8 EqBP[2][6][25][50];	/*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; equal count before peak */
	cmr_u8 ICAP[2][6][25][50];	/*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; increase count after peak */
	cmr_u8 DCAP[2][6][25][50];	/*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; decrease count after peak */
	cmr_u8 EqAP[2][6][25][50];	/*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; equal count after peak */
	cmr_s8 BPC[2][6][25][50];	/*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; ICBP - DCBP */
	cmr_s8 APC[2][6][25][50];	/*[2]:rough/fine search; [6]:six filters; [25]:focus zone; [50]:AF execute frame; ICAP - DCAP */
	cmr_u16 BP_R10000[2][4][6][25][50];	/*[2]:rough/fine search; [4]:four type; [6]:six filters; [25]:focus zone; [50]:AF execute frame; FV ratio X 10000 before peak */
	cmr_u16 AP_R10000[2][4][6][25][50];	/*[2]:rough/fine search; [4]:four type; [6]:six filters; [25]:focus zone; [50]:AF execute frame; FV ratio X 10000 after peak */
	cmr_s32 fv_slope_10000[2][4][6][25][50];	/*[2]:rough/fine search; [4]:four type; [6]:six filters; [25]:focus zone; [50]:AF execute frame; record slope of focus value during this foucs */
	cmr_u32 pdaf_confidence[64];	/*[64]: PD zone; confidence value of phase detection */
	cmr_s16 pdaf_diff[64];		/*[64]: PD zone; different value of phase detection */

	//AF threshold
	cmr_u16 UB_Ratio_TH[2][4][6];	/*[2]:rough/fine search; [4]:four type; [6]:six filters; The Up bound threshold of FV ratio, type[0]:total, type[1]:3 sample, type[2]:5 sample, type[3]:7 sample */
	cmr_u16 LB_Ratio_TH[2][4][6];	/*[2]:rough/fine search; [4]:four type; [6]:six filters; The low bound threshold of FV ratio, type[0]:total, type[1]:3 sample, type[2]:5 sample, type[3]:7 sample */

	//AF result
	cmr_u32 suc_win_num;		/*number of windows of sucess focus: 0~25 */
	cmr_u8 result_win[25];		/*[25]: focus zone; result_win[i]: AF result of each zone */
	cmr_u8 reserved0[3];
	cmr_u32 cur_pos;			/*current focus motor position: 0~1023 */
	cmr_u16 peak_pos[2][6][25];	/*[2]:rough/fine search; [6]:six filters; [25]:focus zone;      peak position: 0~1023 */
	cmr_u16 choose_filter;		/*choose which filter is used to final focus peak */
	cmr_u16 choose_zone;		/*choose which zone is used to final focus peak */
	cmr_u8 total_work_frame;	/*total work frames during AF work */
	cmr_u8 rs_total_frame;		/*total work frames during rough search */
	cmr_u8 fs_total_frame;		/*total work frames during fine search */
	cmr_u8 rs_dir;				/*direction of rough search: 0:Far to near, 1:near to far */
	cmr_u8 fs_dir;				/*direction of fine search: 0:Far to near, 1:near to far */
	cmr_u8 reserved1[3];
	//Reserve
	cmr_u32 reserved2[256];		/*resurve for future */
};

typedef struct debug_isp_display_info {
	struct debug_ae_display_param ae;
	struct debug_awb_display_param awb;
	struct debug_af_display_param af;
} DEBUG_ISP_DISPLAY_INFO_T;

struct em_ae_param_set {
	cmr_u16 ver;
	cmr_u8 lock_ae;				/* 0:unloc; 1:lock */
	cmr_u32 exposure;			/* unit: us */
	cmr_u32 gain;				/* 128 means 1 time gain , user can set any value to it */
	cmr_s32 min_fps;			/* e.g. 2000 means 20.00fps , 0 means Auto */
	cmr_s32 max_fps;
	cmr_s8 iso;
	cmr_s8 ev_index;			/* not real value , just index !! */
	cmr_s8 flicker;
	cmr_s8 FD_AE;				/* 0:off; 1:on */
	cmr_s8 metering_mode;
	cmr_s8 scene_mode;
	cmr_s8 reserve_case;
	cmr_s16 reserve_len;		/*len for reserve */
	cmr_u8 reserve_info[2048];	/* reserve for future */
};

struct em_af_param_set {
	cmr_u16 ver;
//      ......
	cmr_s8 reserve_case;
	cmr_s16 reserve_len;		/*len for reserve */
	cmr_u8 reserve_info[2048];	/* reserve for future */
};

struct em_awb_param_set {
	cmr_u16 ver;
//      ......
	cmr_s8 reserve_case;
	cmr_s16 reserve_len;		/*len for reserve */
	cmr_u8 reserve_info[2048];	/* reserve for future */
};

typedef struct em_3A_param_set {
	struct em_ae_param_set ae;
	struct em_af_param_set af;
	struct em_awb_param_set awb;
} EM_3A_PARAM_SET;

struct em_param {
	EM_MODE_INFO em_mode;
	void *param_ptr;
};

struct sprd_isp_debug_info {
	cmr_u32 debug_startflag;	//0x65786966
	cmr_u32 debug_len;
	cmr_u32 version_id;			//0x00000001
	//cmr_u8 data[0];
	//cmr_u32 debug_endflag;  // 0x637a6768
};

typedef struct _isp_log_info {
	cmr_s32 ae_off;
	cmr_u32 ae_len;
	cmr_s32 af_off;
	cmr_u32 af_len;
	cmr_s32 aft_off;
	cmr_u32 aft_len;
	cmr_s32 awb_off;
	cmr_u32 awb_len;
	cmr_s32 lsc_off;
	cmr_u32 lsc_len;
	cmr_s32 smart_off;
	cmr_u32 smart_len;
	cmr_s32 otp_off;
	cmr_u32 otp_len;
	cmr_s32 microdepth_off;
	cmr_u32 microdepth_len;
	cmr_s32 ver;
	char AF_version[20];		//AF-yyyymmdd-xx
	char magic[8];
} isp_log_info_t;

#define SPRD_DEBUG_START_FLAG   0x65786966
#define SPRD_DEBUG_END_FLAG     0x637a6768
#define SPRD_DEBUG_VERSION_ID   (0x00000004)

#endif
