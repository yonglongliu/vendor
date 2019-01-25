#ifndef _ISP_LSC_ADV_H_
#define _ISP_LSC_ADV_H_

/*----------------------------------------------------------------------------*
 **				Dependencies				*
 **---------------------------------------------------------------------------*/
#ifdef WIN32
#include "data_type.h"
#include "win_dummy.h"
#else
#include "sensor_raw.h"
#include <sys/types.h>
#include <pthread.h>
#include <android/log.h>
//#include <utils/Log.h>

//#include "isp_com.h"
#endif

#include "stdio.h"
#include "isp_pm.h"

/**---------------------------------------------------------------------------*
**				Compiler Flag				*
**---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

#define max(A,B) (((A) > (B)) ? (A) : (B))
#define min(A,B) (((A) < (B)) ? (A) : (B))

/**---------------------------------------------------------------------------*
**				Micro Define				**
**----------------------------------------------------------------------------*/

/*
#define LSC_ADV_DEBUG_STR       "[ALSC]: L %d, %s: "
#define LSC_ADV_DEBUG_ARGS    __LINE__,__FUNCTION__

#define LOG_TAG "[ALSC]"
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define ALOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define ALOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define ALOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)

#define LSC_ADV_LOGE(format,...) ALOGE(LSC_ADV_DEBUG_STR format, LSC_ADV_DEBUG_ARGS, ##__VA_ARGS__)
#define LSC_ADV_LOGW(format,...) ALOGW(LSC_ADV_DEBUG_STR, format, LSC_ADV_DEBUG_ARGS, ##__VA_ARGS__)
#define LSC_ADV_LOGI(format,...) ALOGI(LSC_ADV_DEBUG_STR format, LSC_ADV_DEBUG_ARGS, ##__VA_ARGS__)
#define LSC_ADV_LOGD(format,...) ALOGD(LSC_ADV_DEBUG_STR format, LSC_ADV_DEBUG_ARGS, ##__VA_ARGS__)
#define LSC_ADV_LOGV(format,...) ALOGV(LSC_ADV_DEBUG_STR format, LSC_ADV_DEBUG_ARGS, ##__VA_ARGS__)
*/

/**---------------------------------------------------------------------------*
**				Data Structures 				*
**---------------------------------------------------------------------------*/
	typedef void *lsc_adv_handle_t;

#define ISP_1_0 	1
#define ISP_2_0 	2

#define ISP_ALSC_SUCCESS 0
#define ISP_ALSC_ERROR -1

#define NUM_ROWS 24
#define NUM_COLS 32

#define MAX_SEGS 8

#define MAX_WIDTH   64
#define MAX_HEIGHT  64

/*RAW RGB BAYER*/
#define SENSOR_IMAGE_PATTERN_RAWRGB_GR                0x00
#define SENSOR_IMAGE_PATTERN_RAWRGB_R                 0x01
#define SENSOR_IMAGE_PATTERN_RAWRGB_B                 0x02
#define SENSOR_IMAGE_PATTERN_RAWRGB_GB                0x03

	enum alsc_io_ctrl_cmd {
		SMART_LSC_ALG_UNLOCK = 0,
		SMART_LSC_ALG_LOCK = 1,
		ALSC_CMD_GET_DEBUG_INFO = 2,
		LSC_INFO_TO_AWB = 3,
		ALSC_GET_VER = 4,
		ALSC_FLASH_MAIN_BEFORE = 5,
		ALSC_FLASH_MAIN_AFTER = 6,
		ALSC_FW_STOP = 7,
		ALSC_FW_START = 8,
		ALSC_FW_START_END = 9,
		ALSC_FLASH_PRE_BEFORE = 10,
		ALSC_FLASH_PRE_AFTER = 11,
		ALSC_FLASH_MAIN_LIGHTING = 12,
		ALSC_FLASH_PRE_LIGHTING = 13,
		ALSC_GET_TOUCH = 14,
		ALSC_FW_PROC_START = 15,
		ALSC_FW_PROC_START_END = 16,
		ALSC_GET_UPDATE_INFO = 17,
		ALSC_UNLOCK_UPDATE_FLAG = 18,
	};

	struct tg_alsc_debug_info {
		cmr_u8 *log;
		cmr_u32 size;
	};

	struct alsc_ver_info {
		cmr_u32 LSC_SPD_VERSION;	// LSC version of Spreadtrum
	};

	struct alsc_update_info {
		cmr_u32 alsc_update_flag;
		cmr_u16 can_update_dest;
		cmr_u16 *lsc_buffer_addr;
	};

	struct debug_lsc_param {
		char LSC_version[50];
		cmr_u16 TB_DNP[12];
		cmr_u16 TB_A[12];
		cmr_u16 TB_TL84[12];
		cmr_u16 TB_D65[12];
		cmr_u16 TB_CWF[12];
		cmr_u32 STAT_R[5];
		cmr_u32 STAT_GR[5];
		cmr_u32 STAT_GB[5];
		cmr_u32 STAT_B[5];
		cmr_u32 gain_width;
		cmr_u32 gain_height;
		cmr_u32 gain_pattern;
		cmr_u32 grid;

		//SLSC
		cmr_s32 error_x10000[9];
		cmr_s32 eratio_before_smooth_x10000[9];
		cmr_s32 eratio_after_smooth_x10000[9];
		cmr_s32 final_ratio_x10000;
		cmr_s32 final_index;
		cmr_u16 smart_lsc_table[1024 * 4];

		//ALSC
		cmr_u16 alsc_lsc_table[1024 * 4];

		//ALSC_SMOOTH
		cmr_u16 alsc_smooth_lsc_table[1024 * 4];

		//OUTPUT
		cmr_u16 last_lsc_table[1024 * 4];
		cmr_u16 output_lsc_table[1024 * 4];
	};

	enum lsc_gain_pattern {
		LSC_GAIN_PATTERN_GRBG = 0,
		LSC_GAIN_PATTERN_RGGB = 1,
		LSC_GAIN_PATTERN_BGGR = 2,
		LSC_GAIN_PATTERN_GBRG = 3,
	};

	struct LSC_info2AWB {
		cmr_u16 value[2];		//final_index;
		cmr_u16 weight[2];		// final_ratio;
	};

#define LSCCTRL_EVT_BASE            0x2000
#define LSCCTRL_EVT_INIT            LSCCTRL_EVT_BASE
#define LSCCTRL_EVT_DEINIT          (LSCCTRL_EVT_BASE + 1)
#define LSCCTRL_EVT_IOCTRL          (LSCCTRL_EVT_BASE + 2)
#define LSCCTRL_EVT_PROCESS         (LSCCTRL_EVT_BASE + 3)

	enum lsc_return_value {
		LSC_SUCCESS = 0x00,
		LSC_ERROR,
		LSC_PARAM_ERROR,
		LSC_PARAM_NULL,
		LSC_FUN_NULL,
		LSC_HANDLER_NULL,
		LSC_HANDLER_ID_ERROR,
		LSC_ALLOC_ERROR,
		LSC_FREE_ERROR,
		LSC_RTN_MAX
	};

/*
enum interp_table_index{
	LSC_TAB_A = 0,
	LSC_TAB_TL84,
	LSC_TAB_CWF,
	LSC_TAB_D65,
	LSC_TAB_DNP,
};

struct LSC_Segs{
	cmr_s32 table_pair[2];//The index to the shading table pair.
	cmr_s32 max_CT;//The maximum CT that the pair will be used.
	cmr_s32 min_CT;//The minimum CT that the pair will be used.
};

struct LSC_Setting{
	struct LSC_Segs segs[MAX_SEGS];//The shading table pairs.
	cmr_s32 seg_count;//The number of shading table pairs.

	double smooth_ratio;//the smooth ratio for the output weighting
	cmr_s32 proc_inter;//process interval of LSC(unit=frames)

	cmr_s32 num_seg_queue;//The number of elements for segement voting.
	cmr_s32 num_seg_vote_th;//The voting threshold for segmenet voting.
};
*/

	struct lsc_adv_tune_param {
		cmr_u32 enable;
		cmr_u32 alg_id;

		cmr_u32 debug_level;
		cmr_u32 restore_open;

		/* alg 0 */
		cmr_s32 strength_level;
		float pa;				//threshold for seg
		float pb;
		cmr_u32 fft_core_id;	//fft param ID
		cmr_u32 con_weight;		//convergence rate
		cmr_u32 freq;

		/* alg 1 */
		//global
		cmr_u32 alg_effective_freq;
		double gradient_threshold_rg_coef[5];
		double gradient_threshold_bg_coef[5];
		cmr_u32 thres_bv;
		double ds_sub_pixel_ratio;
		double es_statistic_credibility;
		cmr_u32 thres_s1_mi;
		double es_credibility_s3_ma;
		cmr_s32 WindowSize_rg;
		cmr_s32 WindowSize_bg;
		double dSigma_rg_dx;
		double dSigma_rg_dy;
		double dSigma_bg_dx;
		double dSigma_bg_dy;
		double iir_factor;
	};

	struct alsc_alg0_turn_para {
		float pa;				//threshold for seg
		float pb;
		cmr_u32 fft_core_id;	//fft param ID
		cmr_u32 con_weight;		//convergence rate
		cmr_u32 bv;
		cmr_u32 ct;
		cmr_u32 pre_ct;
		cmr_u32 pre_bv;
		cmr_u32 restore_open;
		cmr_u32 freq;
		float threshold_grad;
	};

	struct lsc_weight_value {
		cmr_s32 value[2];
		cmr_u32 weight[2];
	};

// smart1.0_log_info
	struct lsc_adv_info {
		cmr_u32 flat_num;
		cmr_u32 flat_status1;
		cmr_u32 flat_status2;
		cmr_u32 stat_r_var;
		cmr_u32 stat_b_var;
		cmr_u32 cali_status;

		cmr_u32 alg_calc_cnt;
		struct lsc_weight_value cur_index;
		struct lsc_weight_value calc_index;
		cmr_u32 cur_ct;

		cmr_u32 alg2_enable;
		cmr_u32 alg2_seg1_num;
		cmr_u32 alg2_seg2_num;
		cmr_u32 alg2_seg_num;
		cmr_u32 alg2_seg_valid;
		cmr_u32 alg2_cnt;
		cmr_u32 center_same_num[4];
	};

	struct lsc_adv_context {
		cmr_u32 LSC_SPD_VERSION;
		cmr_u32 grid;
		cmr_s32 gain_pattern;
		cmr_s32 gain_width;
		cmr_s32 gain_height;

		cmr_s32 alg_locked;		// lock alsc or not from ioctrl
		cmr_u32 alg_open;		// open front camera or not

		pthread_mutex_t mutex_stat_buf;
		cmr_u32 *stat_ptr;
		cmr_u32 *stat_for_alsc;

		void *lsc_alg;
		cmr_u16 *lum_gain;
		cmr_u32 pbayer_r_sums[NUM_ROWS * NUM_COLS];
		cmr_u32 pbayer_gr_sums[NUM_ROWS * NUM_COLS];
		cmr_u32 pbayer_gb_sums[NUM_ROWS * NUM_COLS];
		cmr_u32 pbayer_b_sums[NUM_ROWS * NUM_COLS];
		float color_gain[NUM_ROWS * NUM_COLS * 4];
		float color_gain_bak[NUM_ROWS * NUM_COLS * 4];
		struct alsc_alg0_turn_para alg0_turn_para;

		//update flag
		cmr_u32 alsc_update_flag;
		cmr_u16 *lsc_buffer_addr;
	};

////////////////////////////// HLSC_V2.0 structure //////////////////////////////

	struct lsc2_tune_param {	// if modified, please contact to TOOL team
		// system setting
		cmr_u32 LSC_SPD_VERSION;	// LSC version of Spreadtrum
		cmr_u32 number_table;	// number of used original shading tables

		// control_param
		cmr_u32 alg_mode;
		cmr_u32 table_base_index;
		cmr_u32 user_mode;
		cmr_u32 freq;
		cmr_u32 IIR_weight;

		// slsc2_param
		cmr_u32 num_seg_queue;
		cmr_u32 num_seg_vote_th;
		cmr_u32 IIR_smart2;

		// alsc1_param
		cmr_s32 strength;

		// alsc2_param
		cmr_u32 lambda_r;
		cmr_u32 lambda_b;
		cmr_u32 weight_r;
		cmr_u32 weight_b;

		// post_gain
		cmr_u32 bv2gainw_en;
		cmr_u32 bv2gainw_p_bv[6];
		cmr_u32 bv2gainw_b_gainw[6];
		cmr_u32 bv2gainw_adjust_threshold;

		// flash_gain
		cmr_u32 flash_enhance_en;
		cmr_u32 flash_enhance_max_strength;
		cmr_u32 flash_enahnce_gain;
	};

	struct lsc2_context {
		pthread_mutex_t mutex_stat_buf;
		cmr_u32 LSC_SPD_VERSION;	// LSC version of Spreadtrum
		cmr_u32 alg_locked;		// lock alsc or not by ioctrl
		cmr_u32 flash_mode;
		cmr_u32 pre_flash_mode;
		cmr_u32 between_pre_main_flash_count;
		cmr_u32 alg_open;		// complie alg0.c or alg2.c
		cmr_u32 img_width;
		cmr_u32 img_height;
		cmr_u32 gain_width;
		cmr_u32 gain_height;
		cmr_u32 gain_pattern;
		cmr_u32 grid;
		cmr_u32 dual_cam_id;
		cmr_u32 camera_id;		// 0. back camera_master  ,  1. front camera_master

		cmr_u16 *lsc_tab_address[9];	// log the using table address
		cmr_u16 *lsc_OTP_tab_copy;	// the copy of OTP table from init
		cmr_u16 *lsc_OTP_tab_storage[8];	// the storage to save OTP tab
		cmr_u16 *lsc_OTP_tab_storage_binning[8];	// the storage to save binning OTP tab
		cmr_u16 *lsc_OTP_tab_storage_720p[9];	// the storage to save 720p OTP tab
		cmr_u16 *lsc_pm0;		// log the tab0 from pm
		cmr_u16 *lsc_table_ptr_r;	// storage to save Rfirst table
		cmr_u16 *tabptr[9];		// address of origianl shading table will be used to interperlation in slsc2
		cmr_u16 *tabptrPlane[9];	// address R-first shading table ( lsc_table_ptr )

		void *control_param;
		void *slsc2_param;
		void *alsc1_param;
		void *alsc2_param;
		void *lsc1d_param;
		void *post_shading_gain_param;
		void *lsc_flash_proc_param;
		void *lsc_last_info;

		// tmp storage
		cmr_u16 *color_gain_r;
		cmr_u16 *color_gain_gr;
		cmr_u16 *color_gain_gb;
		cmr_u16 *color_gain_b;

		// debug info output address
		void *lsc_debug_info_ptr;

		// AEM storage address
		cmr_u32 *stat_for_lsc;

		// Copy the dst_gain address
		cmr_u16 *dst_gain;

		// flag in ALSC LIB
		cmr_u32 frame_count;
		cmr_u32 alg_count;
		cmr_u32 alg_quick_in;

		// otp
		cmr_u32 lsc_otp_table_flag;	// 0 non-OTP, 1 OTP table
		cmr_u32 lsc_otp_table_flag_binning;	// 0 non-OTP, 1 OTP table
		cmr_u32 lsc_otp_table_flag_720p;	// 0 non-OTP, 1 OTP table
		cmr_u32 lsc_otp_oc_flag;	// 0 no OTP data, 1 OTP data
		cmr_u32 lsc_otp_grid;
		cmr_u32 lsc_otp_table_width;
		cmr_u32 lsc_otp_table_height;
		cmr_u32 lsc_otp_oc_r_x;
		cmr_u32 lsc_otp_oc_r_y;
		cmr_u32 lsc_otp_oc_gr_x;
		cmr_u32 lsc_otp_oc_gr_y;
		cmr_u32 lsc_otp_oc_gb_x;
		cmr_u32 lsc_otp_oc_gb_y;
		cmr_u32 lsc_otp_oc_b_x;
		cmr_u32 lsc_otp_oc_b_y;

		// lsc command set address
		void *lsc_eng_cmd_set_ptr;
		void *lsc_eng_cmd_set2_ptr;

		//fw start/stop
		cmr_u16 *fwstop_output_table;
		cmr_u16 *fwstart_new_scaled_table;
		cmr_u16 can_update_dest;
		cmr_u16 fw_start_end;

		//dual cam
		cmr_u8 is_master;
		cmr_u32 is_multi_mode;

		//update flag
		cmr_u32 alsc_update_flag;
		cmr_u16 *lsc_buffer_addr;
	};

// change mode (fw_start, fw_stop)
	struct alsc_fwstart_info {
		cmr_u16 *lsc_result_address_new;
		cmr_u16 *lsc_tab_address_new[9];
		cmr_u32 gain_width_new;
		cmr_u32 gain_height_new;
		cmr_u32 image_pattern_new;
		cmr_u32 grid_new;
		cmr_u32 camera_id;		// 0. back camera_master  ,  1. front camera_master
	};

//for fw proc start
	struct alsc_fwprocstart_info {
		cmr_u16 *lsc_result_address_new;
		cmr_u16 *lsc_tab_address_new[9];
		cmr_u32 gain_width_new;
		cmr_u32 gain_height_new;
		cmr_u32 image_pattern_new;
		cmr_u32 grid_new;
		cmr_u32 camera_id;		// 0. back camera_master  ,  1. front camera_master
	};

//update flash info
	struct alsc_flash_info {
		float io_captureFlashEnvRatio;
		float io_captureFlash1Ratio;
	};

////////////////////////////// calculation dependent //////////////////////////////

	struct lsc_eng_cmd_set {
		cmr_s32 eng_lsc_dump_aem;
		cmr_s32 eng_lsc_dump_intable;
		cmr_s32 eng_lsc_dump_outtable;
		cmr_s32 eng_lsc_dump_otptable;
		cmr_s32 eng_lsc_dump_param_intable;
		cmr_s32 eng_lsc_dump_otptrans_intable;
		cmr_s32 eng_lsc_otp_disable;
	};

	struct lsc_eng_cmd_set2 {
		cmr_s32 eng_lsc_lock;
		cmr_s32 eng_lsc_set_unit_table;
		cmr_s32 eng_lsc_set_tab_enable;
		cmr_s32 eng_lsc_set_tab_index;
	};

	struct lsc_size {
		cmr_u32 w;
		cmr_u32 h;
	};

	struct lsc_adv_init_param {
		cmr_u32 alg_open;		// complie alg0.c or alg2.c
		cmr_u32 img_width;
		cmr_u32 img_height;
		cmr_u32 gain_width;
		cmr_u32 gain_height;
		cmr_u32 gain_pattern;
		cmr_u32 grid;
		cmr_u32 camera_id;		// 0. back camera_master  ,  1. front camera_master

		// isp2.1 added , need to modify to match old version
		struct third_lib_info lib_param;

		/* added parameters */
		void *tune_param_ptr;
		cmr_u16 *lsc_tab_address[9];	// the copy of table in parameter file
		struct lsc2_tune_param lsc2_tune_param;	// HLSC_V2.0 tuning structure

		/* no use in lsc_adv2 */
		cmr_u32 param_level;
		cmr_u16 *lum_gain;		// space to save pre_table from smart1.0
		struct lsc_adv_tune_param tune_param;

		//otp data
		cmr_u32 lsc_otp_table_en;
		cmr_u32 lsc_otp_table_width;
		cmr_u32 lsc_otp_table_height;
		cmr_u32 lsc_otp_grid;
		cmr_u16 *lsc_otp_table_addr;

		cmr_u32 lsc_otp_oc_en;
		cmr_u32 lsc_otp_oc_r_x;
		cmr_u32 lsc_otp_oc_r_y;
		cmr_u32 lsc_otp_oc_gr_x;
		cmr_u32 lsc_otp_oc_gr_y;
		cmr_u32 lsc_otp_oc_gb_x;
		cmr_u32 lsc_otp_oc_gb_y;
		cmr_u32 lsc_otp_oc_b_x;
		cmr_u32 lsc_otp_oc_b_y;

		//dual cam
		cmr_u8 is_master;
		cmr_u32 is_multi_mode;

		void *otp_info_ptr;

		//add lsc buffer addr
		cmr_u16 *lsc_buffer_addr;
	};

	struct statistic_raw_t {
		cmr_u32 *r;
		cmr_u32 *gr;
		cmr_u32 *gb;
		cmr_u32 *b;
	};

	struct lsc_adv_calc_param {
		struct statistic_raw_t stat_img;	// statistic value of 4 channels
		struct lsc_size stat_size;	// size of statistic value matrix
		cmr_s32 gain_width;		// width  of shading table
		cmr_s32 gain_height;	// height of shading table
		cmr_u32 ct;				// ct from AWB calc
		cmr_s32 r_gain;			// r_gain from AWB calc
		cmr_s32 b_gain;			// b_gain from AWB calc
		cmr_u32 bv;				// bv from AE calc
		cmr_u32 isp_mode;		// about the mode of interperlation of shading table
		cmr_u32 isp_id;			// 0. alg0.c ,  2. alg2.c
		cmr_u32 camera_id;		// 0. back camera_master  ,  1. front camera_master
		struct lsc_size img_size;	// raw size
		cmr_s32 grid;			// grid size

		// no use in HLSC_V2.0
		struct lsc_size block_size;
		cmr_u16 *lum_gain;		// pre_table from smart1.0
		cmr_u32 ae_stable;		// ae stable info from AE calc
		cmr_s32 awb_pg_flag;

		cmr_u16 *lsc_tab_address[9];	// lsc_tab_address
		cmr_u32 lsc_tab_size;

		// not fount in isp_app.c
		cmr_u32 pre_bv;
		cmr_u32 pre_ct;

		//for single and dual flash.
		float captureFlashEnvRatio;	//0-1, flash/ (flash+environment)
		float captureFlash1ofALLRatio;	//0-1,  flash1 / (flash1+flash2)

		cmr_handle handle_pm;
	};

	struct lsc_adv_calc_result {
		cmr_u16 *dst_gain;
	};

	struct lsc_lib_ops {
		cmr_s32(*alsc_calc) (void *handle, struct lsc_adv_calc_param * param, struct lsc_adv_calc_result * adv_calc_result);
		void *(*alsc_init) (struct lsc_adv_init_param * param);
		 cmr_s32(*alsc_deinit) (void *handle);
		 cmr_s32(*alsc_io_ctrl) (void *handler, enum alsc_io_ctrl_cmd cmd, void *in_param, void *out_param);
	};

	struct lsc_ctrl_context {
		pthread_mutex_t status_lock;
		void *alsc_handle;		// alsc handler
		void *lib_handle;
		struct lsc_lib_ops lib_ops;
		struct third_lib_info *lib_info;
		cmr_u16 *dst_gain;
		cmr_u16 *lsc_buffer;
		cmr_u32 gain_pattern;
	};

	struct binning_info {
		float ratio;			// binning = 1/2,  double = 2
	};

	struct crop_info {
		unsigned int start_x;
		unsigned int start_y;
		unsigned int width;
		unsigned int height;
	};

	enum lsc_transform_action {
		LSC_BINNING = 0,
		LSC_CROP = 1,
	};

	struct lsc_table_transf_info {
		unsigned int img_width;
		unsigned int img_height;
		unsigned int grid;
		unsigned int gain_width;
		unsigned int gain_height;

		unsigned short *pm_tab0;
		unsigned short *tab;
	};

/**---------------------------------------------------------------------------*
**					Data Prototype				**
**----------------------------------------------------------------------------*/
	typedef lsc_adv_handle_t(*fun_lsc_adv_init) (struct lsc_adv_init_param * param);
	typedef const char *(*fun_lsc_adv_get_ver_str) (lsc_adv_handle_t handle);
	typedef cmr_s32(*fun_lsc_adv_calculation) (lsc_adv_handle_t handle, struct lsc_adv_calc_param * param, struct lsc_adv_calc_result * result);
	typedef cmr_s32(*fun_lsc_adv_ioctrl) (lsc_adv_handle_t handle, enum alsc_io_ctrl_cmd cmd, void *in_param, void *out_param);
	typedef cmr_s32(*fun_lsc_adv_deinit) (lsc_adv_handle_t handle);

//lsc.so API
	lsc_adv_handle_t lsc_adv_init(struct lsc_adv_init_param *param);
	const char *lsc_adv_get_ver_str(lsc_adv_handle_t handle);
	cmr_s32 lsc_adv_calculation(lsc_adv_handle_t handle, struct lsc_adv_calc_param *param, struct lsc_adv_calc_result *result);
	cmr_s32 lsc_adv_ioctrl(lsc_adv_handle_t handle, enum alsc_io_ctrl_cmd cmd, void *in_param, void *out_param);
	cmr_s32 lsc_adv_deinit(lsc_adv_handle_t handle);
	cmr_s32 is_print_lsc_log(void);

// extern used API
	cmr_int lsc_ctrl_init(struct lsc_adv_init_param *input_ptr, cmr_handle * handle_lsc);
	cmr_int lsc_ctrl_deinit(cmr_handle * handle_lsc);
	cmr_int lsc_ctrl_process(cmr_handle handle_lsc, struct lsc_adv_calc_param *in_ptr, struct lsc_adv_calc_result *result);
	cmr_int lsc_ctrl_ioctrl(cmr_handle handle_lsc, cmr_s32 cmd, void *in_ptr, void *out_ptr);

	cmr_s32 otp_lsc_mod(cmr_u16 * otpLscTabGolden, cmr_u16 * otpLSCTabRandom,	//T1, T2
						cmr_u16 * otpLscTabGoldenRef,	//Ts1
						cmr_u32 * otpAWBMeanGolden, cmr_u32 * otpAWBMeanRandom, cmr_u16 * otpLscTabGoldenMod,	//output: Td2
						cmr_u32 gainWidth, cmr_u32 gainHeight, cmr_s32 bayerPattern);

	cmr_s32 lsc_table_transform(struct lsc_table_transf_info *src, struct lsc_table_transf_info *dst, enum lsc_transform_action action, void *action_info);

/**----------------------------------------------------------------------------*
**					Compiler Flag				**
**----------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
#endif
// End
