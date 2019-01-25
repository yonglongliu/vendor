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
#ifndef _ISP_COMMON_TYPES_H_
#define _ISP_COMMON_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <sys/types.h>
#include <utils/Log.h>
#include "cmr_msg.h"
#include "isp_type.h"
#include "sprd_isp_altek.h"
#include "isp_mw.h"
#include "cmr_log.h"

#define ISP_PRODUCT_NAME_LEN				20
#define cmr_bzero(b, len)				memset((b), '\0', (len))
#define cmr_msleep(x)					usleep((x) * 1000)
#define cmr_usleep(x)					usleep((x))

#ifndef MIN
#define MIN(x, y) (((x) < (y))?(x):(y))
#endif

#define ISP_CHECK_HANDLE_VALID(handle) \
	do { \
		if (!handle) { \
			return -ISP_ERROR; \
		} \
	} while (0)

#define ISP_AE_BLOCK 256
/**********************************ENUM************************************/
enum isp_return_val {
	ISP_SUCCESS = 0x00,
	ISP_PARAM_NULL,
	ISP_PARAM_ERROR,
	ISP_CALLBACK_NULL,
	ISP_ALLOC_ERROR,
	ISP_NO_READY,
	ISP_ERROR,
	ISP_RETURN_MAX
};

enum isp3a_callback_cmd {
	ISP3A_AF_NOTICE_CALLBACK = 0x00000200,	//AF notice
	ISP3A_AE_STAB_CALLBACK = 0x00000400,
	ISP3A_AE_QUICK_MODE_DOWN = 0x00000B00,
	ISP3A_AE_STAB_NOTIFY = 0x00000C00,
	ISP3A_AE_LOCK_NOTIFY = 0x00000D00,
	ISP3A_AE_UNLOCK_NOTIFY = 0x00000E00,
	ISP3A_CALLBACK_CMD_MAX
};

enum isp3a_statis_format {
	ISP3A_STATIS_FORMAT_MAX
};

enum isp3a_dev_evt {
	ISP3A_DEV_STATICS_DONE,
	ISP3A_DEV_SENSOR_SOF,
	ISP3A_DEV_EVT_MAX
};

enum statics_data_type {
	ISP3A_AE,
	ISP3A_AWB,
	ISP3A_AF,
	ISP3A_AFL,
	ISP3A_YHIS,
	ISP3A_SUB_SAMPLE,
	ISP3A_STATICS_TYPE_MAX
};

enum isp3a_work_mode {
	ISP3A_WORK_MODE_PREVIEW,
	ISP3A_WORK_MODE_CAPTURE,
	ISP3A_WORK_MODE_VIDEO,
	ISP3A_WORK_MODE_MAX
};

enum isp3a_opmode_idx {
	ISP3A_OPMODE_NORMALLV = 0,
	ISP3A_OPMODE_AF_FLASH_AF,
	ISP3A_OPMODE_FLASH_AE,
	ISP3A_OPMODE_MAX,
};

enum isp3a_ae_ctrl_state {
	ISP3A_AE_CTRL_ST_INACTIVE,
	ISP3A_AE_CTRL_ST_SEARCHING,
	ISP3A_AE_CTRL_ST_CONVERGED, //normal
	ISP3A_AE_CTRL_ST_FLASH_PREPARE_CONVERGED,
	ISP3A_AE_CTRL_ST_FLASH_ON_CONVERGED,
	ISP3A_AE_CTRL_ST_MAX
};

/**********************************STRUCT***********************************/
struct isp3a_flash_param {
	cmr_u32 blending_ratio_led1;
	struct isp_awb_gain wbgain_led1;
	cmr_u32 color_temp_led1;
	cmr_u32 blending_ratio_led2;
	struct isp_awb_gain wbgain_led2;
	cmr_u32 color_temp_led2;
};

struct isp3a_ae_report {
	cmr_u32 hw3a_frame_id;   // frame ID when HW3a updated
	cmr_u32 block_num;             // total blocks
	cmr_u32 block_totalpixels;   // total pixels in each blocks
	cmr_int bv_val;                          // current BV value
	cmr_u32 ae_state;
	cmr_u32 fe_state;
	cmr_u16 ae_converge_st;
	cmr_s32 BV;
	cmr_u32 non_comp_BV;
	cmr_u32 ISO;
	cmr_u32 flash_ratio;
	cmr_u32 center_mean2x2;
	cmr_u32 ae_nonwt_mean;
	cmr_u32 ae_center_mean2x2;
	cmr_u32 avg_mean;
	cmr_u16 cur_mean;
	cmr_u16 target_mean;
	cmr_u16 sensor_ad_gain;
	cmr_u32 exp_line;
	float exp_time;
	float fps;
	struct isp3a_flash_param flash_param_preview;
	struct isp3a_flash_param flash_param_capture;

	void *y_stats;     // HW3a AE Y stats addr
	cmr_u32 sys_sof_index;     // current SOF ID, should be maintained by framework
	cmr_int bv_delta;        // delta BV from converge

	cmr_u32 debug_data_size;
	void *debug_data;
	void *rgb_stats;
};

struct isp_ae_statistic_info {
	cmr_u32 r_info[ISP_AE_BLOCK];
	cmr_u32 g_info[ISP_AE_BLOCK];
	cmr_u32 b_info[ISP_AE_BLOCK];
	cmr_u32 y_hist[ISP_AE_BLOCK];
};

struct isp3a_ae_info {
	cmr_u32 ae_ctrl_converged_flag; //0:non converge,  1: converged
	cmr_u32 ae_ctrl_state; //see isp3a_ae_ctrl_state
	cmr_u32 ae_ctrl_locked; //0:unlock, 1:lock
	cmr_u32 ae_ctrl_stability;  //0:unstability,   1: stability

	struct isp3a_ae_report report_data;
	cmr_u8 valid;
};

struct isp3a_awb_his {
	cmr_u8 benable;//bEnable;
	cmr_s8 ccrstart;//cCrStart;
	cmr_s8 ccrend;//cCrEnd;
	cmr_s8 cooffsetup;//cOffsetUp;
	cmr_s8 coffsetdown;//cOffsetDown;
	cmr_s8 ccrpurple;//cCrPurple;
	cmr_u8 ucoffsetpurple;//ucOffsetPurPle;
	cmr_s8 cgrass_offset;//cGrassOffset;
	cmr_s8 cgrass_start;//cGrassStart;
	cmr_s8 cgrass_end;//cGrassEnd;
	cmr_u8 ucdampgrass;//;//ucDampGrass;
	cmr_s8 coffset_bbr_w_start;//cOffset_bbr_w_start;
	cmr_s8 coffset_bbr_w_end;//cOffset_bbr_w_end;
	cmr_u8 ucyfac_w;//ucYFac_w;
	cmr_u32 dhisinterp;//dHisInterp;
};

struct isp3a_hw_region {
	cmr_u16 border_ratio_X; // 100% use of current sensor cropped area
	cmr_u16 border_ratio_Y; // 100% use of current sensor cropped area
	cmr_u16 blk_num_X; // fixed value for AE lib
	cmr_u16 blk_num_Y; // fixed value for AE lib
	cmr_u16 offset_ratio_X; // 0% offset of left of current sensor cropped area
	cmr_u16 offset_ratio_Y; // 0% offset of top of current sensor cropped area
};

struct isp3a_ae_hw_cfg {
	cmr_u32 tokenID;
	struct isp3a_hw_region region;
};

struct isp3a_awb_hw_cfg {
	cmr_u8 token_id;//TokenID;
	struct isp3a_hw_region region;//tAWBRegion;
	cmr_u8 uc_factor[16];//ucYFactor[16];
	cmr_s8 bbr_factor[33];//BBrFactor[33];
	cmr_u16 uw_rgain;//uwRGain;
	cmr_u16 uw_ggain;//uwGGain;
	cmr_u16 uw_bgain;//uwBGain;
	cmr_u8 uccr_shift;//ucCrShift;
	cmr_u8 uc_offset_shift;//ucOffsetShift;
	cmr_u8 uc_quantize;//ucQuantize;
	cmr_u8 uc_damp;//ucDamp;
	cmr_u8 uc_sum_shift;//ucSumShift;
	struct isp3a_awb_his t_his;//tHis;
	cmr_u16 uwrlinear_gain;//uwRLinearGain;
	cmr_u16 uwblinear_gain;//uwBLinearGain;
};

struct isp3a_af_stats_roi {
	cmr_u16 uw_size_ratio_x;
	cmr_u16 uw_size_ratio_y;
	cmr_u16 uw_blk_num_x;
	cmr_u16 uw_blk_num_y;
	cmr_u16 uw_offset_ratio_x;
	cmr_u16 uw_offset_ratio_y;
};

struct isp3a_af_hw_cfg {
	cmr_u16 token_id;
	struct isp3a_af_stats_roi af_region;
	cmr_u8 enable_af_lut;
	cmr_u16 auw_lut[259];
	cmr_u16 auw_af_lut[259];
	cmr_u8 auc_weight[6];
	cmr_u16 uw_sh;
	cmr_u8 uc_th_mode;
	cmr_u8 auc_index[82];
	cmr_u16 auw_th[4];
	cmr_u16 pw_tv[4];
	cmr_u32 ud_af_offset;
	cmr_u8 af_py_enable;
	cmr_u8 af_lpf_enable;
	cmr_int filter_mode;
	cmr_u8 uc_filter_id;
	cmr_u16 uw_ine_cnt;
};

enum isp3a_af_pd_config_type {
	ISP3A_PD_CONFIG_ENABLE = 1,
	ISP3A_PD_CONFIG_ROI = (1 << 1),
	ISP3A_PD_CONFIG_RESET = (1 << 2),
};

struct isp3a_roi_t {
	/* top x position */
	cmr_u16 start_x;
	/* top y position */
	cmr_u16 start_y;
	/* crop width */
	cmr_u16 width;
	/* crop height */
	cmr_u16 height;
};

struct isp3a_pd_config_t {
	cmr_u32 type;
	cmr_u16 token_id;
	cmr_u8 pd_enable;
	struct isp3a_roi_t pd_roi;
};

struct isp3a_afl_hw_cfg {
	cmr_u16 token_id;
	cmr_u16 offset_ratiox;
	cmr_u16 offset_ratioy;
};
/*
struct isp3a_yhis_hw_cfg {
};

struct isp3a_subsample_hw_cfg {
};
*/
struct ispae_sync_info {
	cmr_u32 exposure_time;
	cmr_u32 adgain;
	cmr_u32 iso;
	cmr_s32 result_bv;
};

struct ispae_sync_info_output {
	struct ispae_sync_info master;
	struct ispae_sync_info slaver;
};

//struct isp_statistics_info {
//	cmr_u32 frame_id;
//	void *statistics_data;
//};

struct isp_lib_config {
	cmr_u32 product_id;	//lib owner
	cmr_u32 version_id;	//lib version
	cmr_s8 product_name_low[ISP_PRODUCT_NAME_LEN];
	cmr_s8 product_name_high[ISP_PRODUCT_NAME_LEN];
};

struct isp3a_timestamp {
	cmr_u32 sec;
	cmr_u32 usec;
};

struct isp3a_statistics_data {
	cmr_u32 frame_num;
	cmr_u32 used_flag;
	cmr_s32 used_num;
	cmr_u32 size;
	cmr_u32 format;
	struct isp3a_timestamp timestamp;
	void *addr;
};

struct isp3a_pdaf_altek_report_t {
	cmr_u32 token_id;
	cmr_u32 frame_id;
	cmr_u8 enable;
	struct isp3a_timestamp time_stamp;
	float pd_value;
	void *pd_reg_out;
	cmr_u32 pd_reg_size;
};

struct isp_bin_info {
	void *ae_addr;
	void *awb_addr;
	void *af_addr;
	void *isp_3a_addr;
	void *scene_addr;
	cmr_u32 isp_3a_size;
	void *isp_shading_addr;
	cmr_u32 isp_shading_size;
	cmr_u32 size;
	struct sensor_otp_iso_awb_info *otp_data_addr;
	void *isp_caf_addr;
	cmr_u32 isp_caf_size;
	void *isp_pdaf_addr;
	cmr_u32 isp_pdaf_size;
};
/**********************************FUNCTION***********************************/
typedef cmr_int (*isp3a_callback) (cmr_handle handle, cmr_u32 cb_type,
				  void *param_ptr, cmr_u32 param_len);

typedef void (*isp3a_evt_cb)(cmr_int evt, void *data, void *privdata);

typedef cmr_int (*isp3a_release_buf)(cmr_handle handle, cmr_int type, struct isp3a_statistics_data *buf_ptr);

#ifdef __cplusplus
}
#endif
#endif
