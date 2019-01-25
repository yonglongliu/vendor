/*
 *******************************************************************************
 * $Header$
 *
 *  Copyright (c) 2016-2025 Spreadtrum Inc. All rights reserved.
 *
 *  +-----------------------------------------------------------------+
 *  | THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY ONLY BE USED |
 *  | AND COPIED IN ACCORDANCE WITH THE TERMS AND CONDITIONS OF SUCH  |
 *  | A LICENSE AND WITH THE INCLUSION OF THE THIS COPY RIGHT NOTICE. |
 *  | THIS SOFTWARE OR ANY OTHER COPIES OF THIS SOFTWARE MAY NOT BE   |
 *  | PROVIDED OR OTHERWISE MADE AVAILABLE TO ANY OTHER PERSON. THE   |
 *  | OWNERSHIP AND TITLE OF THIS SOFTWARE IS NOT TRANSFERRED.        |
 *  |                                                                 |
 *  | THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT   |
 *  | ANY PRIOR NOTICE AND SHOULD NOT BE CONSTRUED AS A COMMITMENT BY |
 *  | SPREADTRUM INC.                                                 |
 *  +-----------------------------------------------------------------+
 *
 * $History$
 *
 *******************************************************************************
 */

/*!
 *******************************************************************************
 * Copyright 2016-2025 Spreadtrum, Inc. All rights reserved.
 *
 * \file
 * AF_Common.h
 *
 * \brief
 * common data for AF
 *
 * \date
 * 2016/01/03
 *
 * \author
 * Galen Tsai
 *
 *
 *******************************************************************************
 */

#ifndef __AFV1_COMMON_H__
#define  __AFV1_COMMON_H__

#include <stdio.h>
#include <memory.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifdef ANDROID
#include <jni.h>
#include <android/log.h>
#include <sys/system_properties.h>
#endif
// #include "AFv1_Type.h"
#include "cmr_types.h"

/*1.System info*/
#define VERSION             "2.127"
#define SUB_VERSION             "-1201-cafdiv0crash"	//use the date code to naming

#define STRING(s) #s

/*2.function error code*/
#define ERR_SUCCESS         0x0000
#define ERR_FAIL            0x0001
#define ERR_UNKNOW          0x0002

#define TOTAL_POS 1024
#define SOFT_LANDING_ENABLE 1

#define MAX_SAMPLE_NUM	    25
#define TOTAL_SAMPLE_NUM	29
#define ROUGH_SAMPLE_NUM	25	// MAX((ROUGH_SAMPLE_NUM_L3+ROUGH_SAMPLE_NUM_L2),(ROUGH_START_POS_L1+ROUGH_START_POS_L2))
#define FINE_SAMPLE_NUM		20
#define MAX_TIME_SAMPLE_NUM	100
#define G_SENSOR_Q_TOTAL (3)

#define AFAUTO_SCAN_STOP_NMAX (256)
#define FOCUS_STAT_WIN_TOTAL	(10)
#define MULTI_STATIC_TOTAL (9)
#define AF_RESULT_DATA_SIZE	(32)
#define AF_CHECK_SCENE_HISTORY	(15)
#define AF_SCENE_CAL_STDEV_TOTAL	(32)

#define ALG_OUT_SCENE 0
#define ALG_INDOOR_SCENE 1
#define ALG_DARK_SCENE 2
#define ALG_SCENE_NUM 3

#define AF_NOT_FINISHED 0
#define AF_FINISHED 1

#define AE_GAIN_UINIT 128
#define AE_GAIN_1x 0
#define AE_GAIN_2x 1
#define AE_GAIN_4x 2
#define AE_GAIN_8x 3
#define AE_GAIN_16x 4
#define AE_GAIN_32x 5
#define AE_GAIN_TOTAL 6

#define BOKEH_BOUNDARY_RATIO 8000	// based on 10000
#define BOKEH_SCAN_FROM 212		// limited in [0,1023]
#define BOKEH_SCAN_TO 342		// limited in [0,1023]
#define BOKEH_SCAN_STEP 12		// at least 20

#define PD_MAX_AREA 16
#define PD_MAX_MOVECOUNT 6

typedef struct _af_tuning_block_param {
	cmr_u8 *data;
	cmr_u32 data_len;
} af_tuning_block_param;

typedef struct _Bokeh_Result {
	cmr_u8 row_num;				/* The number of AF windows with row (i.e. vertical) *//* depend on the AF Scanning */
	cmr_u8 column_num;			/* The number of AF windows with row (i.e. horizontal) *//* depend on the AF Scanning */
	cmr_u32 win_peak_pos_num;
	cmr_u32 *win_peak_pos;		/* The seqence of peak position which be provided via struct isp_af_fullscan_info *//* depend on the AF Scanning */
	cmr_u16 vcm_dac_up_bound;
	cmr_u16 vcm_dac_low_bound;
	cmr_u16 boundary_ratio;		/* (Unit : Percentage) *//* depend on the AF Scanning */
	cmr_u32 af_peak_pos;
	cmr_u32 near_peak_pos;
	cmr_u32 far_peak_pos;
	cmr_u32 distance_reminder;
	cmr_u32 reserved[16];
} Bokeh_Result;

typedef enum _eAF_FILTER_TYPE {
	T_SOBEL9 = 0,
	T_SOBEL5,
	T_SPSMD,
	T_FV0,
	T_FV1,
	T_COV,
	T_TOTAL_FILTER_TYPE
} eAF_FILTER_TYPE;

typedef enum _eAF_OTP_TYPE {
	T_LENS_BY_DEFAULT = 0,
	T_LENS_BY_OTP,
	T_LENS_BY_TUNING,
} eAF_OTP_TYPE;

typedef enum _eAF_FV_RATIO_TYPE {
	T_R_Total = 0,
	T_R_J1,
	T_R_J2,
	T_R_J3,
	T_R_Slope_Total,
	T_R_SAMPLE_NUM
} eAF_FV_RATIO_TYPE;

typedef enum _eAF_MODE {
	SAF = 0,					// single zone AF
	CAF,						// continue AF
	VAF,						// Video CAF
	FAF,						// Face AF
	MAF,						// Multi zone AF
	PDAF,						// PDAF
	TMODE_1,					// Test mode 1
	Wait_Trigger				// wait for AF trigger
} eAF_MODE;

typedef enum _eAF_Triger_Type {
	RF_NORMAL = 0,				// noraml R/F search for AFT
	R_NORMAL,					// noraml Rough search for AFT
	F_NORMAL,					// noraml Fine search for AFT
	RF_FAST,					// Fast R/F search for AFT
	R_FAST,						// Fast Rough search for AFT
	F_FAST,						// Fast Fine search for AFT
	DEFOCUS,
	BOKEH,
	RE_TRIGGER,
} eAF_Triger_Type;

typedef enum _eSAF_Status {
	SAF_Status_Start = 0,
	SAF_Status_Init,
	SAF_Status_RSearch,
	SAF_Status_FSearch,
	SAF_Status_Move_Lens,
	SAF_Status_Stop
} eSAF_Status;

typedef enum _eCAF_Status {
	CAF_Status_Start = 0,
	CAF_Status_Init,
	CAF_Status_RSearch,
	CAF_Status_FSearch,
	CAF_Status_Move_Lens,
	CAF_Status_Stop
} eCAF_Status;

typedef enum _eSAF_Main_Process {
	SAF_Main_INIT = 0,
	SAF_Main_SEARCH,
	SAF_Main_DONE
} eSAF_Main_Process;

typedef enum _eCAF_Main_Process {
	CAF_Main_INIT = 0,
	CAF_Main_SEARCH,
	CAF_Main_DONE
} eCAF_Main_Process;

typedef enum _eSAF_Search_Process {
	SAF_Search_INIT = 0,
	SAF_Search_SET_ROUGH_SEARCH,
	SAF_Search_ROUGH_SEARCH,
	SAF_Search_ROUGH_HAVE_PEAK,
	SAF_Search_SET_FINE_SEARCH,
	SAF_Search_FINE_SEARCH,
	SAF_Search_FINE_HAVE_PEAK,
	SAF_Search_NO_PEAK,
	SAF_Search_DONE,
	SAF_Search_DONE_BY_UI,
	SAF_Search_TOTAL,

} eSAF_Search_Process;

typedef enum _eCAF_Search_Process {
	CAF_Search_INIT = 0,
	CAF_Search_SET_ROUGH_SEARCH,
	CAF_Search_ROUGH_SEARCH,
	CAF_Search_ROUGH_HAVE_PEAK,
	CAF_Search_SET_FINE_SEARCH,
	CAF_Search_FINE_SEARCH,
	CAF_Search_FINE_HAVE_PEAK,
	CAF_Search_NO_PEAK,
	CAF_Search_DONE,
	CAF_Search_DONE_BY_UI,
	CAF_Search_TOTAL,

} eCAF_Search_Process;

typedef enum _eAF_Result {
	AF_INIT = 0,
	AF_FIND_HC_PEAK,			// high confidence peak
	AF_FIND_LC_PEAK,			// low confidence peak
	AF_NO_PEAK,
	AF_TOO_FAR,
	AF_TOO_NEAR,
	AF_REVERSE,
	AF_SKY,
	AF_UNKNOW
} eAF_Result;

typedef enum _e_LOCK {
	LOCK = 0,
	UNLOCK,
} e_LOCK;

typedef enum _e_DIR {
	FAR2NEAR = 0,
	NEAR2FAR,
} e_DIR;

typedef enum _e_RESULT {
	NO_PEAK = 0,
	HAVE_PEAK,
} e_RESULT;

typedef enum _e_AF_TRIGGER {
	NO_TRIGGER = 0,
	AF_TRIGGER,
} e_AF_TRIGGER;

typedef enum _e_AF_BOOL {
	AF_FALSE = 0,
	AF_TRUE,
} e_AF_BOOL;

typedef enum _e_AF_AE_Gain {
	AE_1X = 0,
	AE_2X,
	AE_4X,
	AE_8X,
	AE_16X,
	AE_32X,
	AE_Gain_Total,
} e_AF_AE_Gain;

enum {
	PD_NO_HINT,
	PD_DIR_ONLY,
	PD_DIR_RANGE,
} PDAF_HINT_INFO;

enum {
	SENSOR_X_AXIS,
	SENSOR_Y_AXIS,
	SENSOR_Z_AXIS,
	SENSOR_AXIS_TOTAL,
};

enum {
	DISTANCE_10CM,
	DISTANCE_20CM,
	DISTANCE_30CM,
	DISTANCE_40CM,
	DISTANCE_50CM,
	DISTANCE_60CM,
	DISTANCE_70CM,
	DISTANCE_80CM,
	DISTANCE_90CM,
	DISTANCE_120CM,
	DISTANCE_200CM,
	DISTANCE_500CM,
	DISTANCE_MAP_TOTAL,
};

enum {
	AF_TIME_DCAM,
	AF_TIME_VCM,
	AF_TIME_CAPTURE,
	AF_TIME_TYPE_TOTAL,
};
// =========================================================================================//
// Public Structure Instance
// =========================================================================================//
// #pragma pack(push, 1)

#pragma pack(push,4)
typedef struct _AE_Report {
	cmr_u8 bAEisConverge;		//flag: check AE is converged or not
	cmr_s16 AE_BV;				//brightness value
	cmr_u16 AE_EXP;				//exposure time (ms)
	cmr_u16 AE_Gain;			//X128: gain1x = 128
	cmr_u32 AE_Pixel_Sum;		//AE pixel sum which needs to match AF blcok
	cmr_u16 AE_Idx;				//AE exposure level
	cmr_u32 cur_fps;
	cmr_u32 cur_lum;
	cmr_u32 cur_index;
	cmr_u32 cur_ev;
	cmr_u32 cur_iso;
	cmr_u32 target_lum;
	cmr_u32 target_lum_ori;
	cmr_u32 flag4idx;
	cmr_s32 bisFlashOn;
	cmr_u32 reserved[10];
} AE_Report;

typedef struct _AF_FV_DATA {
	cmr_u8 MaxIdx[MAX_SAMPLE_NUM];	//index of max statistic value
	cmr_u8 MinIdx[MAX_SAMPLE_NUM];	//index of min statistic value

	cmr_u8 BPMinIdx[MAX_SAMPLE_NUM];	//index of min statistic value before peak
	cmr_u8 APMinIdx[MAX_SAMPLE_NUM];	//index of min statistic value after peak

	cmr_u8 ICBP[MAX_SAMPLE_NUM];	//increase count before peak
	cmr_u8 DCBP[MAX_SAMPLE_NUM];	//decrease count before peak
	cmr_u8 EqBP[MAX_SAMPLE_NUM];	//equal count before peak
	cmr_u8 ICAP[MAX_SAMPLE_NUM];	//increase count after peak
	cmr_u8 DCAP[MAX_SAMPLE_NUM];	//decrease count after peak
	cmr_u8 EqAP[MAX_SAMPLE_NUM];	//equal count after peak
	cmr_s8 BPC[MAX_SAMPLE_NUM];	//ICBP - DCBP
	cmr_s8 APC[MAX_SAMPLE_NUM];	//ICAP - DCAP

	cmr_u64 Val[MAX_SAMPLE_NUM];	//statistic value
	cmr_u64 Start_Val;			//statistic value of start position

	cmr_u16 BP_R10000[T_R_SAMPLE_NUM][MAX_SAMPLE_NUM];	//FV ratio X 10000 before peak
	cmr_u16 AP_R10000[T_R_SAMPLE_NUM][MAX_SAMPLE_NUM];	//FV ratio X 10000 after peak

	cmr_u8 Search_Result[MAX_SAMPLE_NUM];	//search result of focus data
	cmr_u16 peak_pos[MAX_SAMPLE_NUM];	//peak position of focus data
	cmr_u64 PredictPeakFV[MAX_SAMPLE_NUM];	//statistic value

} AF_FV_DATA;

typedef struct _AF_FV {
	AF_FV_DATA Filter[T_TOTAL_FILTER_TYPE];
	AE_Report AE_Rpt[MAX_SAMPLE_NUM];

} AF_FV;
#pragma pack(pop)

#pragma pack(push,1)
typedef struct _AF_FILTER_TH {
	cmr_u16 UB_Ratio_TH[T_R_SAMPLE_NUM];	//The Up bound threshold of FV ratio, [0]:total, [1]:3 sample, [2]:5 sample, [3]:7 sample
	cmr_u16 LB_Ratio_TH[T_R_SAMPLE_NUM];	//The low bound threshold of FV ratio, [0]:total, [1]:3 sample, [2]:5 sample, [3]:7 sample
	cmr_u32 MIN_FV_TH;			//The threshold of FV to check the real curve
} AF_FILTER_TH;

typedef struct _AF_TH {
	AF_FILTER_TH Filter[T_TOTAL_FILTER_TYPE];

} AF_TH;
#pragma pack(pop)

#pragma pack(push,4)
typedef struct _SAF_SearchData {
	cmr_u8 SAF_RS_TotalFrame;	//total work frames during rough search
	cmr_u8 SAF_FS_TotalFrame;	//total work frames during fine search

	cmr_u8 SAF_RS_LensMoveCnt;	//total lens execute frame during rough search
	cmr_u8 SAF_FS_LensMoveCnt;	//total lens execute frame during fine search

	cmr_u8 SAF_RS_StatisticCnt;	//total statistic frame during rough search
	cmr_u8 SAF_FS_StatisticCnt;	//total statistic frame during fine search

	cmr_u8 SAF_RS_MaxSearchTableNum;	//maxima number of search table during rough search
	cmr_u8 SAF_FS_MaxSearchTableNum;	//maxima number of search table during fine search

	cmr_u8 SAF_RS_DIR;			//direction of rough search:Far to near or near to far
	cmr_u8 SAF_FS_DIR;			//direction of fine search:Far to near or near to far

	cmr_u8 SAF_Skip_Frame;		//skip this frame or not

	cmr_u16 SAF_RSearchTable[ROUGH_SAMPLE_NUM];
	cmr_u16 SAF_RSearchTableByTuning[TOTAL_SAMPLE_NUM];
	cmr_u16 SAF_FSearchTable[FINE_SAMPLE_NUM];

	AF_FV SAF_RFV;				// The FV data of rough search
	AF_FV SAF_FFV;				// The FV data of fine search

	AF_FV SAF_Pre_RFV;			//The last time FV data of rough search
	AF_FV SAF_Pre_FFV;			//The last time FV data of fine search

	AF_TH SAF_RS_TH;			// The threshold of rough search
	AF_TH SAF_FS_TH;			// The threshold of fine search

} SAF_SearchData;

typedef struct _CAF_SearchData {
	cmr_u8 CAF_RS_TotalFrame;
	cmr_u8 CAF_FS_TotalFrame;

	cmr_u16 CAF_RSearchTable[ROUGH_SAMPLE_NUM];
	cmr_u16 CAF_FSearchTable[FINE_SAMPLE_NUM];

	AF_FV CAF_RFV;
	AF_FV CAF_FFV;

	AF_FV CAF_Pre_RFV;
	AF_FV CAF_Pre_FFV;

} CAF_SearchData;
#pragma pack(pop)

#pragma pack(push,1)
typedef struct _AF_Scan_Table {
	// Rough search
	cmr_u16 POS_L1;
	cmr_u16 POS_L2;
	cmr_u16 POS_L3;
	cmr_u16 POS_L4;

	cmr_u16 Sample_num_L1_L2;
	cmr_u16 Sample_num_L2_L3;
	cmr_u16 Sample_num_L3_L4;

	cmr_u16 Normal_Start_Idx;
	cmr_u16 Rough_Sample_Num;

	// Fine search
	cmr_u16 Fine_Sample_Num;
	cmr_u16 Fine_Search_Interval;
	cmr_u16 Fine_Init_Num;

} AF_Scan_Table;

typedef struct aftuning_coeff_s {
	cmr_u32 saf_coeff[8];
	cmr_u32 caf_coeff[8];
	cmr_u32 saf_stat_param[AE_Gain_Total];
	cmr_u32 caf_stat_param[AE_Gain_Total];
	cmr_u8 reserve[32 * 4];
} aftuning_coeff_t;

typedef struct aftuning_param_s {
	cmr_u32 enable_adapt_af;
	cmr_u32 _alg_select;
	cmr_u32 _lowlight_agc;
	cmr_u32 _flat_rto;
	cmr_u32 _falling_rto;
	cmr_u32 _rising_rto;
	cmr_u32 _stat_min_value[AE_Gain_Total];
	cmr_u32 _stat_min_diff[AE_Gain_Total];
	cmr_u32 _break_rto;
	cmr_u32 _turnback_rto;
	cmr_u32 _forcebreak_rto;
	cmr_u32 _break_count;
	cmr_u32 _max_shift_idx;
	cmr_u32 _lowlight_ma_count;
	cmr_s32 _posture_diff_slop;
	cmr_u32 _temporal_flat_slop;
	cmr_u32 _limit_search_interval;
	cmr_u32 _sky_scene_thr;
	cmr_u32 _min_fine_idx;
	cmr_u8 reserve[128 - 4];
} aftuning_param_t;

typedef struct pdaftuning_param_s {
	cmr_u32 min_pd_vcm_steps;
	cmr_u32 max_pd_vcm_steps;
	cmr_u32 coc_range;
	cmr_u32 far_tolerance;
	cmr_u32 near_tolerance;
	cmr_u32 err_limit;
	cmr_u32 pd_converge_thr;
	cmr_u32 pd_converge_thr_2nd;
	cmr_u32 pd_focus_times_thr;
	cmr_u32 pd_thread_sync_frm;
	cmr_u32 pd_thread_sync_frm_init;
	cmr_u32 min_process_frm;
	cmr_u32 max_process_frm;
	cmr_u32 pd_conf_thr;
	cmr_u32 pd_conf_thr_2nd;
	cmr_u32 reserved[52];
} pdaftuning_param_t;

typedef struct _AF_Tuning_Para {
	//AF Scan Table
	AF_Scan_Table Scan_Table_Para[AE_Gain_Total];

	//AF Threshold
	AF_TH SAF_RS_TH[AE_Gain_Total];	//The threshold of rough search
	AF_TH SAF_FS_TH[AE_Gain_Total];	//The threshold of fine search

	cmr_s32 dummy[200];
} AF_Tuning_Para;

typedef struct _Lens_Info {
	//Lens Info
	cmr_u16 Lens_MC_MIN;		//minimal mechenical position
	cmr_u16 Lens_MC_MAX;		//maximal mechenical position
	cmr_u16 Lens_INF;			//INF position
	cmr_u16 Lens_MACRO;			//MACRO position
	cmr_u16 Lens_Hyper;			//Hyper Focus position
	cmr_u16 One_Frame_Max_Move;	//skip one frame if move position is bigger than TH

} Lens_Info;

typedef struct _AF_Tuning {
	// Lens Info
	Lens_Info Lens_Para;
	AF_Tuning_Para SAFTuningPara;	//SAF parameters
	AF_Tuning_Para CAFTuningPara;	//CAF parameters
	AF_Tuning_Para VCAFTuningPara;	//Video CAF parameters
	cmr_u32 tuning_ver_code;	//ex Algo AF2.104 , tuning-0001-> 0x21040001
	cmr_u32 tuning_date_code;	//ex 20170306 -> 0x20170306
	aftuning_coeff_t af_coeff;	//AF coefficient for control speed and overshot
	aftuning_param_t adapt_af_param;	//adapt AF parameter
	cmr_u8 dummy[400];
} AF_Tuning;

typedef struct _afscan_status_s {
	cmr_u32 n_stops;
	cmr_u32 pos_from;
	cmr_u32 pos_to;
	cmr_u32 pos_jump;
	cmr_u32 scan_idx;
	cmr_u32 stat_log[AFAUTO_SCAN_STOP_NMAX];
	cmr_u32 stat_sum_log[AFAUTO_SCAN_STOP_NMAX];
	cmr_u32 scan_tbl[AFAUTO_SCAN_STOP_NMAX];
	cmr_u32 frmid[AFAUTO_SCAN_STOP_NMAX];
	cmr_s32 bv_log[AFAUTO_SCAN_STOP_NMAX];
	cmr_u32 luma[AFAUTO_SCAN_STOP_NMAX];
	cmr_u32 gain[AFAUTO_SCAN_STOP_NMAX];
	cmr_u32 ae_state[AFAUTO_SCAN_STOP_NMAX];
	cmr_u32 exp[AFAUTO_SCAN_STOP_NMAX];
	cmr_u32 scan_tbl_posidx[AFAUTO_SCAN_STOP_NMAX];
	cmr_u32 scan_tbl_pos[AFAUTO_SCAN_STOP_NMAX];
	cmr_u32 scan_tbl_stat[AFAUTO_SCAN_STOP_NMAX];
	cmr_u32 scan_tbl_stat2[AFAUTO_SCAN_STOP_NMAX];
	cmr_u32 scan_tbl_macrv[AFAUTO_SCAN_STOP_NMAX];
	cmr_s32 scan_tbl_slop[AFAUTO_SCAN_STOP_NMAX];
	cmr_s32 scan_tbl_maslop[AFAUTO_SCAN_STOP_NMAX];
	cmr_u32 scan_tbl_aeluma[AFAUTO_SCAN_STOP_NMAX];
	cmr_u32 scan_tbl_afluma[AFAUTO_SCAN_STOP_NMAX];
	cmr_u32 scan_tbl_jump[AFAUTO_SCAN_STOP_NMAX];
	cmr_u32 scan_tbl_pkidx[AFAUTO_SCAN_STOP_NMAX];
	cmr_u32 scan_tbl_start;
	cmr_u32 scan_tbl_end;
	cmr_u32 scan_interval;
	cmr_u32 peak_idx;
	cmr_u32 peak_pos;
	cmr_u32 peak_stat;
	cmr_u32 peak_stat_sum;
	cmr_u32 turnback_status;
	cmr_u32 valley_stat;
	cmr_u32 valley_idx;
	cmr_u32 last_idx;
	cmr_u32 last_stat;
	cmr_u32 alg_sts;
	cmr_u32 scan_dir;
	cmr_u32 last_dir;
	cmr_u32 init_pos;
	cmr_u32 tbl_falling_cnt[AFAUTO_SCAN_STOP_NMAX];
	cmr_u32 tbl_flat_cnt[AFAUTO_SCAN_STOP_NMAX];
	cmr_u32 turnback_cnt;
	cmr_u32 turnback_idx;
	cmr_u32 turnback_cond;
	cmr_u32 break_idx;
	cmr_u32 break_cond;
	cmr_u32 fine_begin;
	cmr_u32 fine_end;
	cmr_u32 fine_stat;
	cmr_u32 fine_pkidx;
	cmr_u32 blcpeak_idx;
	cmr_u32 bound_cnt;
	cmr_u32 stat_vari_rto;
	cmr_u32 stat_rising_rto;
	cmr_u32 stat_falling_rto;
	cmr_u32 vly_far_stat;
	cmr_u32 vly_far_rto;
	cmr_u32 vly_far_pos;
	cmr_u32 vly_near_stat;
	cmr_u32 vly_near_rto;
	cmr_u32 vly_near_pos;
	cmr_u32 breakcount;
	cmr_u32 breakratio;

	cmr_u32 frm_num;
	cmr_u32 pkfrm_num;
	cmr_u32 vly_far_idx;
	cmr_u32 vly_near_idx;
	cmr_u32 spdup_num;
	cmr_s32 local_scrtbl[AFAUTO_SCAN_STOP_NMAX];
	cmr_s32 localma_scrtbl[AFAUTO_SCAN_STOP_NMAX];
	cmr_s32 localsum_scrtbl[AFAUTO_SCAN_STOP_NMAX];
	cmr_s32 localluma_scrtbl[AFAUTO_SCAN_STOP_NMAX];
	cmr_u32 local_idx;
	cmr_u32 localma_idx;
	cmr_u32 localsum_idx;
	cmr_u32 localluma_idx;
	cmr_u32 lock_pos;
	// multi AF
	cmr_u32 multi_pkstat[MULTI_STATIC_TOTAL];
	cmr_u32 multi_pkpos[MULTI_STATIC_TOTAL];
	cmr_u32 multi_pkidx[MULTI_STATIC_TOTAL];
	cmr_u32 multi_pkfrm[MULTI_STATIC_TOTAL];
	cmr_s32 multi_pkscr[MULTI_STATIC_TOTAL];
	cmr_u32 multi_pk_far;
	cmr_u32 multi_pk_near;
	cmr_u32 multi_same_focal;

	cmr_u32 min_stat_diff;
	cmr_u32 min_stat_val;

	cmr_u32 fine_range;
	cmr_u32 luma_slop;
	cmr_u32 scan_algorithm;

	cmr_u32 pos_far;
	cmr_u32 pos_near;
	cmr_u32 far_idx;
	cmr_u32 near_idx;
	cmr_s32 scan_tbl_full_slop[AFAUTO_SCAN_STOP_NMAX];
	cmr_s32 far_slop;
	cmr_s32 near_slop;
	cmr_u32 scan_tbl_slop_type;
	cmr_u32 far_falling_cnt;
	cmr_u32 near_falling_cnt;
	cmr_s32 lk_loacl_scrtbl[AFAUTO_SCAN_STOP_NMAX];
	cmr_u32 lk_local_idx;
	cmr_u32 focus_inf;
	cmr_u32 focus_macro;
	cmr_u32 peak_inverse;
	cmr_u32 peak_calculate;
	cmr_u32 peak_calc_cond;
	cmr_s32 phase_diff_tbl[AFAUTO_SCAN_STOP_NMAX];
	cmr_u32 pd_conf_tbl[AFAUTO_SCAN_STOP_NMAX];
	cmr_u32 pd_peak_idx;
	cmr_u32 inverse_calc_pos;
	cmr_u32 fit_peak_far_limit;
	cmr_u32 fit_peak_near_limit;
	cmr_u32 reserve[62];		// for temp debug
} afscan_status_t;

typedef struct af_ctrl_pd_info_s {
	cmr_u32 info_type;
	cmr_u32 predict_direction;
	cmr_u32 predict_peak;
	cmr_u32 predict_far_stop;
	cmr_u32 predict_near_stop;
	cmr_u32 phase_diff_value;
	cmr_u32 confidence_level;
} pd_info_t;

typedef struct _af_control_status_s {
	cmr_u32 state;
	cmr_u32 frmid;
	cmr_u32 scene_reg;
	cmr_u32 scene_stable;
	cmr_u32 stop_event;
	cmr_u32 env_ena;
	cmr_u32 env_reconv_cnt;
	cmr_u32 env_reconv_limit;
	cmr_u32 motor_excitation;
	cmr_u32 idsp_enable;
	cmr_u32 idsp_resume_thd;
	cmr_u32 idsp_reconv_thd;
	cmr_u32 idsp_reconv_limit;
	cmr_u32 idsp_reconv_cnt;
	cmr_u32 stat_nsy_enable;
	cmr_u32 idsp_frmbuff_clr_cnt;
	cmr_u32 idsp_frmbuff_clr_limit;
	cmr_u32 idsp_reset_frmid;
	cmr_u32 debug_cb;
	cmr_u32 env_avgy_histroy[AF_CHECK_SCENE_HISTORY];
	cmr_u32 lock_status;
	cmr_u32 last_lock_status;
	pd_info_t pd_info;
	cmr_u32 reserve[64];		// for temp debug
} afctrl_status_t;

typedef struct af_scan_env_info_s {
	cmr_u32 fps;
	cmr_u32 ae_state;
	cmr_u32 exp_boundary;
	cmr_u32 ae_y_avg;
	cmr_u32 ae_y_tgt;
	cmr_u32 curr_gain;
	cmr_u32 exp_time;
	cmr_u32 y_stdev;
	cmr_u32 y_stable;
	cmr_s32 curr_bv;
	cmr_s32 next_bv;
	cmr_s32 diff_bv;
	cmr_s32 phase_diff;
	cmr_u32 pd_conf;
	cmr_u32 af_y_avg;
	cmr_u32 posture_status;
} scan_env_info_t;

typedef struct af_scan_info_s {
	cmr_u32 lowlight_af;
	cmr_u32 dark_af;
	cmr_u32 lowcontrast_af;
	cmr_u32 curr_fpos;
	scan_env_info_t env;
	scan_env_info_t report;
	cmr_u32 luma_slop;
	cmr_u32 times_result;
	cmr_u32 position_result[AF_RESULT_DATA_SIZE];
	cmr_u32 frmid_result[AF_RESULT_DATA_SIZE];
	cmr_u32 coast_result[AF_RESULT_DATA_SIZE];
	cmr_u32 ma_count;
	cmr_u32 reserve[32];		// for temp debug 
} afscan_info_t;

typedef struct af_prescan_info_s {
	cmr_u32 curr_index;
	cmr_u32 frmid[AF_CHECK_SCENE_HISTORY];
	scan_env_info_t env[AF_CHECK_SCENE_HISTORY];
	cmr_u32 reserved[64];
} af_prescan_info_t;
/* ========================== Structure ============================ */
typedef struct afstat_frame_buffer_s {
	cmr_u32 curr_frm_stat[FOCUS_STAT_WIN_TOTAL];
	// cmr_u32 curr_frm_y[FOCUS_STAT_WIN_TOTAL];
	cmr_u32 last_frm_stat[FOCUS_STAT_WIN_TOTAL];
	// cmr_u32 last_frm_y[FOCUS_STAT_WIN_TOTAL];
	// cmr_u32 focus_block_idx;
	// cmr_u32 peak_block_edge;
	// cmr_u32 peak_block_y;
	// cmr_s32 peak_block_edge_rela;
	// cmr_s32 peak_block_y_rela;
	// cmr_s32 peak_block_around_rela[FOCUS_STAT_AROUND_BLOCK_DATA];
	cmr_u32 stat_weight;
	cmr_u32 stat_sum;
	cmr_u32 luma_avg;
	// cmr_u32 multi_grid_sum[MULTI_STATIC_TOTAL];
	cmr_u32 multi_stat_tbl[AFAUTO_SCAN_STOP_NMAX][MULTI_STATIC_TOTAL];	/* debug * info * of * defocus * function */
	cmr_u32 reserve[32];		// for temp debug
} afstat_frmbuf_t;

typedef struct defocus_param_s {
	cmr_u32 scan_from;
	cmr_u32 scan_to;
	cmr_u32 per_steps;
} defocus_param_t;

typedef struct afdbg_ctrl_s {
	cmr_u32 alg_msg;
	cmr_u32 dump_info;
	defocus_param_t defocus;
	cmr_u32 reserve[16];		// for temp debug
} afdbg_ctrl_t;

typedef struct _af_process_s {
	afctrl_status_t ctrl_status;
	afscan_status_t scan_status;
	afscan_info_t scan_info;
	afstat_frmbuf_t stat_data;
	afdbg_ctrl_t dbg_ctrl;
	aftuning_param_t adapt_af_param;	// adapt AF parameter
	af_prescan_info_t pre_scan_info;
	cmr_u32 reserve[128];		// for temp debug
} _af_process_t;

typedef struct motion_sensor_result_s {
	cmr_s64 timestamp;
	uint32_t sensor_g_posture;
	uint32_t sensor_g_queue_cnt;
	float g_sensor_queue[SENSOR_AXIS_TOTAL][G_SENSOR_Q_TOTAL];
	cmr_u32 reserved[12];
} motion_sensor_result_t;

typedef struct pd_algo_focuing_s {
	cmr_u32 B_frmid;
	cmr_u32 B_confidence[5];
	double B_pd_value[5];
	cmr_s32 B_delta_vcm;

	cmr_u32 frmid[PD_MAX_MOVECOUNT];
	cmr_u32 Area0_confidence[PD_MAX_MOVECOUNT];
	double Area0_pd_value[PD_MAX_MOVECOUNT];
	cmr_u32 Area1_confidence[PD_MAX_MOVECOUNT];
	double Area1_pd_value[PD_MAX_MOVECOUNT];
	cmr_u32 Area2_confidence[PD_MAX_MOVECOUNT];
	double Area2_pd_value[PD_MAX_MOVECOUNT];
	cmr_u32 Area3_confidence[PD_MAX_MOVECOUNT];
	double Area3_pd_value[PD_MAX_MOVECOUNT];
	cmr_u32 Area4_confidence[PD_MAX_MOVECOUNT];
	double Area4_pd_value[PD_MAX_MOVECOUNT];
	cmr_u32 cur_vcm_pos[PD_MAX_MOVECOUNT];
	cmr_s32 delta_vcm[PD_MAX_MOVECOUNT];
	cmr_u32 fv_info[PD_MAX_MOVECOUNT];
	cmr_u32 af_frmid[PD_MAX_MOVECOUNT];
	cmr_u32 ae_stable[PD_MAX_MOVECOUNT];
	cmr_u32 curr_luma[PD_MAX_MOVECOUNT];
	cmr_u32 reserved[64 - (PD_MAX_MOVECOUNT * 2)];
} pd_algo_focusing_t;

typedef struct pd_algo_result_s {
	cmr_u32 pd_enable;
	cmr_u32 effective_pos;
	cmr_u32 effective_frmid;
	cmr_u32 confidence[PD_MAX_AREA];
	double pd_value[PD_MAX_AREA];
	cmr_u32 pd_roi_dcc[PD_MAX_AREA];
	cmr_u32 pd_roi_num;
	cmr_u32 reserved[16];
} pd_algo_result_t;

typedef struct _exposure_result_s {
	cmr_u32 is_converge;		//flag: check AE is converged or not
	// not
	cmr_u32 cur_fps;
	cmr_u32 cur_lum;
	cmr_u32 cur_iso;
	cmr_u32 cur_exp;
	cmr_u32 target_lum;
	cmr_u32 reserved[30];
} exposure_result_t;

typedef struct _focus_stat_result_s {
	cmr_u32 center_stat;
	cmr_u32 full_stat;
	cmr_u32 reserved[30];
} focus_stat_result_t;

typedef struct _pdaf_process_s {
	pd_algo_focusing_t pd_focusing;
	pd_algo_result_t pd_result;
	cmr_u32 proc_status;
	cmr_u32 vcm_drive_frmid;
	cmr_u32 pre_pd_frmid;
	cmr_u32 confidence_level;
	cmr_u32 pd_active;
	cmr_u32 cd_active;
	cmr_u32 procedure;
	cmr_s32 predict_dir;
	cmr_u32 predict_far;
	cmr_u32 predict_near;
	cmr_u32 roi_dcc;
	cmr_s32 delta_vcm_pos;
	cmr_u32 tc_en;
	cmr_s32 tc_stat_th;
	cmr_u32 tc_vcm_pos;
	float tc_err_vcm_sum;
	cmr_u32 tc_calc_idx;
	cmr_u32 prv_vcm_pos;
	cmr_u32 cur_vcm_pos;
	cmr_u32 last_vcm_pos;
	cmr_u32 vcm_moving_flag;
	cmr_u32 vcm_move_count;
	cmr_s32 vcm_delta_pos;
	cmr_u32 pd_focus_times;
	cmr_u32 period_a;
	cmr_u32 period_b;
	cmr_u32 err_count;
	cmr_u32 select_roi;
	double phase_diff_value;
	cmr_u32 conf_value;
	cmr_u32 phase_diff_dir;
	cmr_u32 hybrid_focus;
	cmr_u32 fv_stat_sum;
	cmr_u32 fv_stat_weight;
	cmr_u32 init_pd_frmid;
	cmr_u32 predict_vcm_pos;
	cmr_u32 vcm_smooth_table[PD_MAX_MOVECOUNT];
	cmr_u32 target_vcm_pos;
	cmr_u32 statistics_dir;
	cmr_u32 vcm_register_pos;
	cmr_u32 curr_luma;
	cmr_u32 target_luma;
	cmr_u32 ae_stable;
	cmr_u32 curr_bv;
	cmr_u32 reserved[128 - 5];
} _pdaf_process_t;

typedef struct motion_sensor_data_s {
	cmr_u32 sensor_type;
	float x;
	float y;
	float z;
	cmr_u32 reserved[12];
} motion_sensor_data_t;

typedef struct lens_distance_map_s {
	cmr_u32 valid_code;
	cmr_u32 steps_table[DISTANCE_MAP_TOTAL];	//vcm step
	cmr_u32 distance_table[DISTANCE_MAP_TOTAL];	//distance by mm, 10/20/30/40/50/60/70/80/90/120/200/Inf
	cmr_u32 reserved[50];
} lens_distance_map_t;

typedef struct _filter_clip {
	cmr_u32 spsmd_max;
	cmr_u32 spsmd_min;
	cmr_u32 sobel_max;
	cmr_u32 sobel_min;
} filter_clip_t;

typedef struct _win_coord_alg {
	cmr_u32 start_x;
	cmr_u32 start_y;
	cmr_u32 end_x;
	cmr_u32 end_y;
} win_coord;

typedef struct _AF_Softlanding_Config {
	cmr_u32 anti_noise_pos;		//the point to which could jump directly
	cmr_u32 SAF_max_jump_len;	//if cur_pos > anti_noise_pos, the max move step in one frame
	cmr_u32 SAF_loop_oneframe[ALG_SCENE_NUM];	//in one frame the times could jump
	cmr_u32 SAF_loop_step[ALG_SCENE_NUM];	//in one frame the step could jump in each time
	cmr_u32 SAF_loop_sleep;		//in one frame the sleeptime after each little jump
	cmr_u32 QUIT_max_step;
	cmr_u32 QUIT_step;
	cmr_u32 QUIT_sleep;
	cmr_u32 reserved[36];
} AF_Softlanding_Config;

typedef struct _weight_setting_s {
	cmr_u8 table_sum;
	cmr_u8 weight_value[FOCUS_STAT_WIN_TOTAL];
	cmr_u8 reserved[13];
} _weight_setting_t;			// four byte aligment 

typedef struct _af_weight_table_s {
	_weight_setting_t mode_0_weight_table[ALG_SCENE_NUM];
	_weight_setting_t mode_1_weight_table[ALG_SCENE_NUM];
	_weight_setting_t mode_2_weight_table[ALG_SCENE_NUM];
	_weight_setting_t mode_3_weight_table[ALG_SCENE_NUM];
} _weight_table_t;

typedef struct _face_af_tuning_s {
	cmr_u32 big_size_thr;		// big face thr
	cmr_u32 middle_size_thr;	// middle face thr
	cmr_u32 little_size_thr;	// little face thr
	cmr_u32 absolute_thr;		// too little face thr
	cmr_u32 big_reduce_ratio[ALG_SCENE_NUM];	// outdoor/indoor/dark
	cmr_u32 middle_reduce_ratio[ALG_SCENE_NUM];	// outdoor/indoor/dark
	cmr_u32 little_reduce_ratio[ALG_SCENE_NUM];	// outdoor/indoor/dark
	cmr_u32 absolute_thr_ratio;	// when fit in too little face thr ,then enlarge the face area
	cmr_u32 reserved[9];
} face_af_tuning_t;

typedef struct _microdepth_s {
	cmr_u32 tuning_enable;
	cmr_u32 near_thr_low;
	cmr_u32 near_thr_up;
	cmr_u32 far_thr_low;
	cmr_u32 far_thr_up;
	cmr_u32 golden_vcm_steps[DISTANCE_MAP_TOTAL * 2];	// 4*DISTANCE_MAP_TOTAL*2 =96 bytes
	cmr_u32 golden_distance[DISTANCE_MAP_TOTAL * 2];	// 4*DISTANCE_MAP_TOTAL*2 =96 bytes
	cmr_u32 reverved[40];
} microdepth_t;

typedef struct _haf_tuning_param_s {
	// default param for outdoor/indoor/dark
	pdaftuning_param_t PDAF_Tuning_Data[ALG_SCENE_NUM];
	cmr_u32 reverved[256];
} haf_tuning_param_t;

typedef struct _af_tuning_param {
	cmr_u8 flag;				// Tuning parameter switch, 1 enable tuning parameter, 0 disenable it
#if 1							//16*18+4*9 = 324 bytes ,win config 502*3 = 1506 bytes
	AF_Softlanding_Config Soft_landing_param;	// 192bytes
	face_af_tuning_t face_param;	// 92 bytes
	_weight_table_t weight_table;	//288 bytes
	microdepth_t microdepth_param;	// 176bytes + 196 bytes
	cmr_u8 dummy1[324 + 1506 - 192 - 92 - 288 - 372];
#else							//long time for unused
	//filter_clip_t filter_clip[ALG_SCENE_NUM][AE_GAIN_TOTAL];       // AF filter threshold, 
	//cmr_s32 bv_threshold[ALG_SCENE_NUM][ALG_SCENE_NUM];     //BV threshold
	//AF_Window_Config SAF_win;       // SAF window config ,502bytes
	//AF_Window_Config CAF_win;       // CAF window config ,502bytes
	//AF_Window_Config VAF_win;       // VAF window config ,502bytes
#endif
	// default param for indoor/outdoor/dark
	AF_Tuning AF_Tuning_Data[ALG_SCENE_NUM];	// Algorithm related parameter
#if 1							// 2 bytes
	cmr_u8 dummy2[2];
#else							// long time for unused
	//cmr_u8 soft_landing_dly;
	//cmr_u8 soft_landing_step;
#endif
	cmr_u8 vcm_hysteresis;
	cmr_u8 dummy[98];			// for 4-bytes alignment issue
} af_tuning_param_t;
#pragma pack(pop)

#pragma pack(push,4)
typedef struct _CAF_Tuning_Para {
	cmr_s32 dummy;
} CAF_Tuning_Para;

typedef struct _SAF_INFO {
	cmr_u32 Cur_AFT_Type;		//the search method
	cmr_u8 SAF_Main_Process;	//the process state of SAF main
	cmr_u8 SAF_Search_Process;	//the process state of SAF search
	cmr_u8 SAF_Status;
	cmr_u8 SAF_RResult;			//rough search result
	cmr_u8 SAF_FResult;			//fine search result
	cmr_u8 SAF_Total_work_frame;	//total work frames during SAF work
	cmr_u8 SAF_AE_Gain;			//AE gain
	cmr_u16 SAF_Start_POS;		//focus position before AF work
	cmr_u16 SAF_Cur_POS;		//current focus positon
	cmr_u16 SAF_Final_POS;		//final move positon
	cmr_u16 SAF_Final_VCM_POS;	//final move positon load from VCM
	cmr_u16 SAF_RPeak_POS;		//Peak positon of rough search
	cmr_u16 SAF_FPeak_POS;		//Peak positon of fine search
	cmr_u64 SAF_SYS_TIME_ENTER[MAX_TIME_SAMPLE_NUM];	//save each time while entering SAF search
	cmr_u64 SAF_SYS_TIME_EXIT[MAX_TIME_SAMPLE_NUM];	//save each time while entering SAF search
	cmr_u32 env_converge_frm;
	Lens_Info Lens_Para;		//current lens parameters
	AF_Scan_Table SAF_Scan_Table_Para;	//current scan table parameters
} SAF_INFO;

typedef struct _CAF_INFO {
	cmr_u8 CAF_mode;

} CAF_INFO;

typedef struct _SAF_Data {
	SAF_INFO sAFInfo;
	SAF_SearchData sAFSearchData;

} SAF_Data;

typedef struct _CAF_Data {
	CAF_INFO cAFInfo;
	CAF_SearchData cAFSearchData;
	CAF_Tuning_Para cAFTuningPara;

} CAF_Data;

typedef struct _AF_OTP_Data {
	cmr_u8 bIsExist;
	cmr_u16 INF;
	cmr_u16 MACRO;
	cmr_u32 reserved[10];
} AF_OTP_Data;

typedef struct _af_stat_data_s {
	cmr_u32 roi_num;
	cmr_u32 stat_num;
	cmr_u64 *p_stat;
} _af_stat_data_t;

typedef struct _AF_Ctrl_Ops {
	void *cookie;
	 cmr_u8(*statistics_wait_cal_done) (void *cookie);

	 cmr_u8(*statistics_get_data) (cmr_u64 fv[T_TOTAL_FILTER_TYPE], _af_stat_data_t * p_stat_data, void *cookie);
	 cmr_u8(*statistics_set_data) (cmr_u32 set_stat, void *cookie);
	 cmr_u8(*phase_detection_get_data) (pd_algo_result_t * pd_result, void *cookie);
	 cmr_u8(*motion_sensor_get_data) (motion_sensor_result_t * ms_result, void *cookie);
	 cmr_u8(*lens_get_pos) (cmr_u16 * pos, void *cookie);
	 cmr_u8(*lens_move_to) (cmr_u16 pos, void *cookie);
	 cmr_u8(*lens_wait_stop) (void *cookie);
	 cmr_u8(*lock_ae) (e_LOCK bisLock, void *cookie);
	 cmr_u8(*lock_awb) (e_LOCK bisLock, void *cookie);
	 cmr_u8(*lock_lsc) (e_LOCK bisLock, void *cookie);
	 cmr_u8(*get_sys_time) (cmr_u64 * pTime, void *cookie);

	 cmr_u8(*get_ae_report) (AE_Report * pAE_rpt, void *cookie);

	 cmr_u8(*set_af_exif) (const void *pAF_data, void *cookie);

	 cmr_u8(*sys_sleep_time) (cmr_u16 sleep_time, void *cookie);

	 cmr_u8(*get_otp_data) (AF_OTP_Data * pAF_OTP, void *cookie);

	 cmr_u8(*get_motor_pos) (cmr_u16 * motor_pos, void *cookie);
	 cmr_u8(*set_motor_sacmode) (void *cookie);

	 cmr_u8(*binfile_is_exist) (cmr_u8 * bisExist, void *cookie);
	 cmr_u8(*get_vcm_param) (cmr_u32 * param, void *cookie);
	 cmr_u8(*af_log) (const char *format, ...);

	 cmr_u8(*af_start_notify) (eAF_MODE AF_mode, void *cookie);

	 cmr_u8(*af_end_notify) (eAF_MODE AF_mode, void *cookie);

	 cmr_u8(*set_wins) (cmr_u32 index, cmr_u32 start_x, cmr_u32 start_y, cmr_u32 end_x, cmr_u32 end_y, void *cookie);
	 cmr_u8(*get_win_info) (cmr_u32 * hw_num, cmr_u32 * isp_w, cmr_u32 * isp_h, void *cookie);
	 cmr_u8(*lock_ae_partial) (cmr_u32 is_lock, void *cookie);
	// SharkLE Only ++
	 cmr_u8(*set_pulse_line) (cmr_u32 line, void *cookie);
	 cmr_u8(*set_next_vcm_pos) (cmr_u32 pos, void *cookie);
	 cmr_u8(*set_pulse_log) (cmr_u32 flag, void *cookie);
	 cmr_u8(*set_clear_next_vcm_pos) (void *cookie);
	// SharkLE Only --
} AF_Ctrl_Ops;

typedef struct _AF_Trigger_Data {
	cmr_u8 bisTrigger;
	cmr_u32 AF_Trigger_Type;
	cmr_u32 AFT_mode;
	defocus_param_t defocus_param;
	cmr_u32 re_trigger;
	cmr_u32 trigger_source;
	cmr_u32 reserved[6];
} AF_Trigger_Data;

typedef struct _AF_Win {
	cmr_u16 Set_Zone_Num;		// FV zone number
	cmr_u16 AF_Win_X[FOCUS_STAT_WIN_TOTAL];
	cmr_u16 AF_Win_Y[FOCUS_STAT_WIN_TOTAL];
	cmr_u16 AF_Win_W[FOCUS_STAT_WIN_TOTAL];
	cmr_u16 AF_Win_H[FOCUS_STAT_WIN_TOTAL];
	cmr_u16 reserved[4 * 20];
} AF_Win;

typedef struct _AF_face_info {
	cmr_u32 sx;
	cmr_u32 sy;
	cmr_u32 ex;
	cmr_u32 ey;
	cmr_u32 reserved[4];
} AF_face_info;

typedef struct _AF_face_area {
	cmr_u32 face_num;
	AF_face_info face_info[10];
	cmr_u32 reserved[4];
} AF_face_area;

typedef struct _Bokeh_tuning_param {
	cmr_u16 from_pos;
	cmr_u16 to_pos;
	cmr_u16 move_step;
	cmr_u16 vcm_dac_up_bound;
	cmr_u16 vcm_dac_low_bound;
	cmr_u16 boundary_ratio;		/* (Unit : Percentage) *//* depend on the AF Scanning */
} Bokeh_tuning_param;

typedef struct _af_time_info_s {
	cmr_u64 vcm_timestamp;
	cmr_u64 dcam_timestamp;
	cmr_u64 takepic_timestamp;
	cmr_u64 local_frame_id;
	cmr_u64 reserved[15];
} _af_time_info_t;

typedef struct _af_history_info_s {
	cmr_u64 timestamp_info;
	cmr_u32 history_index;
	pd_algo_result_t pd_result[AF_RESULT_DATA_SIZE];
	exposure_result_t ae_result[AF_RESULT_DATA_SIZE];
	focus_stat_result_t stat_result[AF_RESULT_DATA_SIZE];
	cmr_u32 reserved[100];
} _af_history_info_t;

// SharkLE Only ++
typedef struct _le_dac_info {
	cmr_u32 pulse_line;
	cmr_u32 dac;
	cmr_u32 pulse_sec;
	cmr_u32 pulse_usec;
	cmr_u32 vcm_mv_sec;
	cmr_u32 vcm_mv_usec;
} le_dac_info;
// SharkLE Only --

typedef struct _AF_Data {
	cmr_s8 AF_Version[40];
	cmr_s8 AF_sub_version[40];
	cmr_u32 AF_mode;
	cmr_u32 Pre_AF_mode;
	AF_Trigger_Data AFT_Data;
	SAF_Data sAF_Data;
	CAF_Data cAF_Data;
	AE_Report AE_Rpt[MAX_TIME_SAMPLE_NUM];
	AF_OTP_Data AF_OTP;
	AF_Win AF_Win_Data;
	cmr_u32 vcm_register;
	AF_Tuning AF_Tuning_Data;
	_af_process_t af_proc_data;
	motion_sensor_result_t sensor_result;
	lens_distance_map_t dis_map;
	Bokeh_tuning_param bokeh_param;
	AF_face_area face_info;
	cmr_u32 hw_num;
	cmr_u32 isp_w;
	cmr_u32 isp_h;
	cmr_u32 dump_log;
	cmr_u32 hysteresis_step;
	cmr_u32 cur_scene;
	cmr_u32 pre_scene;
	_af_time_info_t time_info;
	_pdaf_process_t pdaf_proc_data;
	pdaftuning_param_t PDAF_Tuning_Data;
	AF_Softlanding_Config Soft_landing_param;
	face_af_tuning_t face_param;	// 92 bytes
	_weight_table_t weight_table;	// 288 bytes
	_af_history_info_t history_data;
	_af_history_info_t runtime_data;
	cmr_u32 postpone_notice;
	cmr_u32 distance_reminder;
	cmr_u32 near_dist;
	cmr_u32 far_dist;
	// distance independant
	cmr_u32 category[9];
	cmr_u32 near_thr_low;
	cmr_u32 near_thr_up;
	cmr_u32 far_thr_low;
	cmr_u32 far_thr_up;
	cmr_u32 max_fv_near;
	cmr_u32 max_fv_far;
	cmr_u32 near_little_fv;
	cmr_u32 far_little_fv;
	cmr_u32 near_maxfv_index;
	cmr_u32 far_maxfv_index;
	cmr_s32 far_shift;
	cmr_u32 face_delay;
	cmr_u64 FV_record;
	microdepth_t microdepth_param;	// 176bytes
	cmr_u16 SAF_Softlanding_dac[MAX_SAMPLE_NUM * 2];
	cmr_u32 SAF_Softlanding_index;
	cmr_u32 SAF_Softlanding_enable;
	cmr_u32 Tuning_Buffer_checksum;
	cmr_u32 is_flash;
	// SharkLE Only ++
	le_dac_info le_dac_info_his[MAX_TIME_SAMPLE_NUM];
	cmr_u32 le_dac_info_his_cont;
	// SharkLE Only --
	cmr_u32 reserved[500];
	AF_Ctrl_Ops AF_Ops;

} AF_Data;

typedef struct _AF_Bokeh_Test {
	cmr_u8(*bokeh_set_scan_range) (cmr_u32 * pos_from, cmr_u32 * pos_to, cmr_u32 * steps, cmr_u16 INF, cmr_u16 MACRO);
	cmr_u8(*bokeh_set_window) (AF_Win * wins, cmr_u32 isp_w, cmr_u32 isp_h);
	cmr_u8(*bokeh_calc) (const cmr_u32 multi_stat_tbl[][9], const cmr_u32 * multi_pkpos, const cmr_u32 * pos_tbl, cmr_u32 pkfrm_num, Bokeh_Result * result);
	cmr_u8 Bokeh_Test_init_state;
} AF_Bokeh_Test;
AF_Bokeh_Test Bokeh_Test;
#pragma pack(pop)

#endif
