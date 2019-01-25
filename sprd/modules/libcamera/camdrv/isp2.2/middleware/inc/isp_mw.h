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
#ifndef _ISP_MW_H_
#define _ISP_MW_H_

#ifndef WIN32
#include <sys/types.h>
#include "isp_exif.h"
#endif

#include "isp_type.h"

#include "sprd_isp_r6p10v2.h"
#include "cmr_sensor_info.h"

//#define LSC_ADV_ENABLE
typedef cmr_int(*proc_callback) (cmr_handle handler_id, cmr_u32 mode, void *param_ptr, cmr_u32 param_len);

#define ISP_EVT_MASK	 0x0000FF00

#define ISP_FLASH_MAX_CELL	40
#define ISP_MODE_NUM_MAX 16
#define ISP_BINNING_MAX_STAT_W    640
#define ISP_BINNING_MAX_STAT_H     480

#define ISP_CTRL_EVT_INIT                    (1 << 2)
#define ISP_CTRL_EVT_DEINIT                  (1 << 3)
#define ISP_CTRL_EVT_CONTINUE                (1 << 4)
#define ISP_CTRL_EVT_CONTINUE_STOP           (1 << 5)
#define ISP_CTRL_EVT_SIGNAL                  (1 << 6)
#define ISP_CTRL_EVT_SIGNAL_NEXT             (1 << 7)
#define ISP_CTRL_EVT_IOCTRL                  (1 << 8)
#define ISP_CTRL_EVT_TX                      (1 << 9)
#define ISP_CTRL_EVT_SOF                     (1 << 10)
#define ISP_CTRL_EVT_EOF                     (1 << 11)
#define ISP_CTRL_EVT_AE                      (1 << 12)
#define ISP_CTRL_EVT_AWB                     (1 << 13)
#define ISP_CTRL_EVT_AF                      (1 << 14)
#define ISP_CTRL_EVT_CTRL_SYNC               (1 << 15)
#define ISP_CTRL_EVT_CONTINUE_AF             (1 << 16)
#define ISP_CTRL_EVT_PDAF		     (1 << 17)
#define ISP_CTRL_EVT_BINNING                     (1 << 18)

#define ISP_CTRL_EVT_MONITOR_STOP            (1 << 31)
#define ISP_CTRL_EVT_MASK                    (cmr_u32)(ISP_CTRL_EVT_INIT \
					|ISP_CTRL_EVT_CONTINUE_STOP|ISP_CTRL_EVT_DEINIT|ISP_CTRL_EVT_CONTINUE \
					|ISP_CTRL_EVT_SIGNAL|ISP_CTRL_EVT_SIGNAL_NEXT|ISP_CTRL_EVT_IOCTRL \
					|ISP_CTRL_EVT_TX|ISP_CTRL_EVT_SOF|ISP_CTRL_EVT_EOF|ISP_CTRL_EVT_AWB \
					|ISP_CTRL_EVT_AE|ISP_CTRL_EVT_AF|ISP_CTRL_EVT_CTRL_SYNC|ISP_CTRL_EVT_CONTINUE_AF \
					|ISP_CTRL_EVT_BINNING|ISP_CTRL_EVT_PDAF|ISP_CTRL_EVT_MONITOR_STOP)

#define ISP_THREAD_QUEUE_NUM                 (100)

#define ISP_PROC_AFL_DONE                    (1 << 2)
#define ISP_PROC_AFL_STOP                    (1 << 3)
#define ISP_AFL_EVT_MASK                     (cmr_u32)(ISP_PROC_AFL_DONE | ISP_PROC_AFL_STOP)

#define ISP_CALLBACK_EVT                     0x00040000

#define CAMERA_ISP_RAWAEM_FLAG 11	// buffer queue flag, sync with hal
#define Y_GAMMA_SMART_WITH_RGB_GAMMA
#define ISP_PARAM_VERSION                    (0x00030002)

enum isp_alg_set_cmd {
	ISP_AE_SET_GAIN,
	ISP_AE_SET_MONITOR,
	ISP_AE_SET_MONITOR_WIN,
	ISP_AE_SET_MONITOR_BYPASS,
	ISP_AE_SET_STATISTICS_MODE,
	ISP_AE_SET_STATS_MONITOR,
	ISP_AE_SET_RGB_GAIN,
	ISP_AE_SET_AE_CALLBACK,
	ISP_AE_SET_EXPOSURE,
	ISP_AE_EX_SET_EXPOSURE,
	ISP_AE_SET_FLASH_CHARGE,
	ISP_AE_SET_FLASH_TIME,
	ISP_AE_GET_SYSTEM_TIME,
	ISP_AE_GET_FLASH_CHARGE,
	ISP_AE_GET_FLASH_TIME,
	ISP_AE_FLASH_CTRL,
	ISP_AE_GET_RGB_GAIN,
	ISP_AE_SET_WBC_GAIN,
	ISP_AE_MULTI_WRITE,
	ISP_AE_DUAL_SYNC_WRITE_SET,
	ISP_AE_DUAL_SYNC_READ_AEINFO,
	ISP_AE_DUAL_SYNC_READ_AEINFO_SLV,
	 /*AF*/ ISP_AF_SET_POS,
	ISP_AF_AE_AWB_LOCK,
	ISP_AF_AE_AWB_RELEASE,
	ISP_AF_SET_PD_INFO,
	ISP_PDAF_SET_CFG_PARAM,
	ISP_PDAF_BLOCK_CFG,
	ISP_PDAF_SET_BYPASS,
	ISP_PDAF_SET_WORK_MODE,
	ISP_PDAF_SET_SKIP_NUM,
	ISP_PDAF_SET_PPI_INFO,
	ISP_PDAF_SET_ROI,
	ISP_PDAF_SET_EXTRACTOR_BYPASS,
	ISP_AFL_SET_CFG_PARAM,
	ISP_AFL_SET_BYPASS,
	ISP_AFL_NEW_SET_CFG_PARAM,
	ISP_AFL_NEW_SET_BYPASS,
	ISP_AFL_SET_STATS_BUFFER,
	ISP_AFM_TYPE2_STS,
	ISP_AFM_TYPE1_STS,

};

enum isp_callback_cmd {
	ISP_CTRL_CALLBACK = 0x00000000,
	ISP_PROC_CALLBACK = 0x00000100,
	ISP_AF_NOTICE_CALLBACK = 0x00000200,
	ISP_SKIP_FRAME_CALLBACK = 0x00000300,
	ISP_AE_STAB_CALLBACK = 0x00000400,
	ISP_AF_STAT_CALLBACK = 0x00000500,
	ISP_AF_STAT_END_CALLBACK = 0x00000600,
	ISP_AWB_STAT_CALLBACK = 0x00000700,
	ISP_CONTINUE_AF_NOTICE_CALLBACK = 0x00000800,
	ISP_AE_CHG_CALLBACK = 0x00000900,
	ISP_ONLINE_FLASH_CALLBACK = 0x00000A00,
	ISP_QUICK_MODE_DOWN = 0x00000B00,
	ISP_AE_STAB_NOTIFY = 0x00000C00,
	ISP_AE_LOCK_NOTIFY = 0x00000D00,
	ISP_AE_UNLOCK_NOTIFY = 0x00000E00,
	ISP_AE_SYNC_INFO = 0x00000F00,
	ISP_AE_EXP_TIME = 0x00001000,
	ISP_VCM_STEP = 0x00002000,
	ISP_HDR_EV_EFFECT_CALLBACK = 0x00003000,
	ISP_AE_CB_FLASH_FIRED = 0x00004000,
	ISP_AE_CALCOUT_NOTIFY = 0x00005000,
	ISP_CALLBACK_CMD_MAX = 0xffffffff
};

enum isp_video_mode {
	ISP_VIDEO_MODE_CONTINUE = 0x00,
	ISP_VIDEO_MODE_SINGLE,
	ISP_VIDEO_MODE_MAX
};

enum isp_focus_mode {
	ISP_FOCUS_NONE = 0x00,
	ISP_FOCUS_TRIG,
	ISP_FOCUS_ZONE,
	ISP_FOCUS_MULTI_ZONE,
	ISP_FOCUS_MACRO,
	ISP_FOCUS_WIN,
	ISP_FOCUS_CONTINUE,
	ISP_FOCUS_MANUAL,
	ISP_FOCUS_VIDEO,
	ISP_FOCUS_BYPASS,
	ISP_FOCUS_MACRO_FIXED,
	ISP_FOCUS_PICTURE,
	ISP_FOCUS_FULLSCAN,
	ISP_FOCUS_MAX
};

enum isp_focus_move_mode {
	ISP_FOCUS_MOVE_START = 0x00,
	ISP_FOCUS_MOVE_END,
	ISP_FOCUS_MOVE_MAX
};

enum isp_flash_mode {
	ISP_FLASH_PRE_BEFORE,
	ISP_FLASH_PRE_LIGHTING,
	ISP_FLASH_PRE_AFTER,
	ISP_FLASH_MAIN_BEFORE,
	ISP_FLASH_MAIN_LIGHTING,
	ISP_FLASH_MAIN_AE_MEASURE,
	ISP_FLASH_MAIN_AFTER,
	ISP_FLASH_AF_DONE,
	ISP_FLASH_SLAVE_FLASH_OFF,
	ISP_FLASH_SLAVE_FLASH_TORCH,
	ISP_FLASH_SLAVE_FLASH_AUTO,
	ISP_FLASH_MODE_MAX
};

enum isp_ae_awb_lock_unlock_mode {
	ISP_AWB_UNLOCK = 0x00,
	ISP_AWB_LOCK,
	ISP_AE_AWB_LOCK = 0x09,
	ISP_AE_AWB_UNLOCK = 0x0a,
	ISP_AE_AWB_MAX
};

enum isp_ae_mode {
	ISP_AUTO = 0x00,
	ISP_NIGHT,
	ISP_SPORT,
	ISP_PORTRAIT,
	ISP_LANDSCAPE,
	ISP_AE_MODE_MAX
};

enum isp_awb_mode {
	ISP_AWB_INDEX0 = 0x00,
	ISP_AWB_INDEX1,
	ISP_AWB_INDEX2,
	ISP_AWB_INDEX3,
	ISP_AWB_INDEX4,
	ISP_AWB_INDEX5,
	ISP_AWB_INDEX6,
	ISP_AWB_INDEX7,
	ISP_AWB_INDEX8,
	ISP_AWB_AUTO,
	ISP_AWB_OFF,
	ISP_AWB_MAX
};

enum isp_format {
	ISP_DATA_YUV422_3FRAME = 0x00,
	ISP_DATA_YUV422_2FRAME,
	ISP_DATA_YVU422_2FRAME,
	ISP_DATA_YUYV,
	ISP_DATA_UYVY,
	ISP_DATA_YVYU,
	ISP_DATA_VYUY,
	ISP_DATA_YUV420_2FRAME,
	ISP_DATA_YVU420_2FRAME,
	ISP_DATA_YUV420_3_FRAME,
	ISP_DATA_NORMAL_RAW10,
	ISP_DATA_CSI2_RAW10,
	ISP_DATA_ALTEK_RAW10,
	ISP_DATA_FORMAT_MAX
};

enum isp_capture_mode {
	ISP_CAP_MODE_AUTO,
	ISP_CAP_MODE_ZSL,
	ISP_CAP_MODE_HDR,
	ISP_CAP_MODE_VIDEO,
	ISP_CAP_MODE_VIDEO_HDR,
	ISP_CAP_MODE_BRACKET,
	ISP_CAP_MODE_RAW_DATA,
	ISP_CAP_MODE_HIGHISO,
	ISP_CAP_MODE_HIGHISO_RAW_CAP,
	ISP_CAP_MODE_DRAM,
	ISP_CAP_MODE_MAX
};

enum isp_ctrl_cmd {
	ISP_CTRL_AWB_MODE = 0,
	ISP_CTRL_SCENE_MODE = 1,
	ISP_CTRL_AE_MEASURE_LUM = 2,
	ISP_CTRL_EV = 3,
	ISP_CTRL_FLICKER = 4,
	ISP_CTRL_AEAWB_BYPASS = 5,
	ISP_CTRL_SPECIAL_EFFECT = 6,
	ISP_CTRL_BRIGHTNESS = 7,
	ISP_CTRL_CONTRAST = 8,
	ISP_CTRL_HIST,
	ISP_CTRL_AUTO_CONTRAST,
	ISP_CTRL_SATURATION = 11,
	ISP_CTRL_AF,
	ISP_CTRL_AF_MODE = 13,
	ISP_CTRL_CSS,
	ISP_CTRL_HDR = 15,
	ISP_CTRL_GLOBAL_GAIN,
	ISP_CTRL_CHN_GAIN,
	ISP_CTRL_GET_EXIF_INFO,
	ISP_CTRL_ISO = 19,
	ISP_CTRL_WB_TRIM,
	ISP_CTRL_PARAM_UPDATE,
	ISP_CTRL_FLASH_EG,
	ISP_CTRL_VIDEO_MODE,
	ISP_CTRL_AF_STOP,
	ISP_CTRL_AE_TOUCH,
	ISP_CTRL_AE_INFO,
	ISP_CTRL_SHARPNESS,
	ISP_CTRL_GET_FAST_AE_STAB,
	ISP_CTRL_GET_AE_STAB,
	ISP_CTRL_GET_AE_CHG,
	ISP_CTRL_GET_AWB_STAT,
	ISP_CTRL_GET_AF_STAT,
	ISP_CTRL_GAMMA,
	ISP_CTRL_DENOISE,
	ISP_CTRL_SMART_AE,
	ISP_CTRL_CONTINUE_AF,
	ISP_CTRL_AF_DENOISE,
	ISP_CTRL_FLASH_CTRL = 38,	// for isp tool
	ISP_CTRL_AE_CTRL = 39,	// for isp tool
	ISP_CTRL_AF_CTRL = 40,	// for isp tool
	ISP_CTRL_REG_CTRL = 41,	// for isp tool
	ISP_CTRL_DENOISE_PARAM_READ,	//for isp tool
	ISP_CTRL_DUMP_REG,	//for isp tool
	ISP_CTRL_AF_END_INFO,	// for isp tool
	ISP_CTRL_FLASH_NOTICE,
	ISP_CTRL_AE_FORCE_CTRL,	// for mp tool
	ISP_CTRL_GET_AE_STATE,	// for isp tool
	ISP_CTRL_SET_LUM,	// for isp tool
	ISP_CTRL_GET_LUM,	// for isp tool
	ISP_CTRL_SET_AF_POS,	// for isp tool
	ISP_CTRL_GET_AF_POS,	// for isp tool
	ISP_CTRL_GET_AF_MODE,	// for isp tool
	ISP_CTRL_FACE_AREA,
	ISP_CTRL_SCALER_TRIM,
	ISP_CTRL_START_3A,
	ISP_CTRL_STOP_3A,
	IST_CTRL_SNAPSHOT_NOTICE,
	ISP_CTRL_SFT_READ,
	ISP_CTRL_SFT_WRITE,
	ISP_CTRL_SFT_SET_PASS,	// added for sft
	ISP_CTRL_SFT_GET_AF_VALUE,	// added for sft
	ISP_CTRL_SFT_SET_BYPASS,	// added for sft
	ISP_CTRL_GET_AWB_GAIN,	// for mp tool
	ISP_CTRL_GET_AWB_CT,
	ISP_CTRL_RANGE_FPS,
	ISP_CTRL_SET_AE_FPS,	// for LLS feature
	ISP_CTRL_BURST_NOTICE,	// burst mode notice
	ISP_CTRL_GET_INFO,
	ISP_CTRL_SET_AE_NIGHT_MODE,
	ISP_CTRL_SET_AE_AWB_LOCK_UNLOCK,
	ISP_CTRL_SET_AE_LOCK_UNLOCK,
	ISP_CTRL_TOOL_SET_SCENE_PARAM,
	ISP_CTRL_IFX_PARAM_UPDATE,
	ISP_CTRL_FORCE_AE_QUICK_MODE,
	ISP_CTRL_DENOISE_PARAM_UPDATE,	//for isp tool
	ISP_CTRL_SET_AE_EXP_TIME,
	ISP_CTRL_SET_AE_SENSITIVITY,
	ISP_CTRL_SET_DZOOM_FACTOR,
	ISP_CTRL_SET_CONVERGENCE_REQ,
	ISP_CTRL_SET_SNAPSHOT_FINISHED,
	ISP_CTRL_GET_EXIF_DEBUG_INFO,
	ISP_CTRL_GET_CUR_ADGAIN_EXP,
	ISP_CTRL_SET_FLASH_MODE,
	ISP_CTRL_SET_AE_MODE,
	ISP_CTRL_SET_AE_FIX_EXP_TIME,
	ISP_CTRL_SET_AE_FIX_SENSITIVITY,
	ISP_CTRL_SET_AE_FIX_FRAM_DURA,
	ISP_CTRL_SET_AE_MANUAL_EXPTIME,
	ISP_CTRL_SET_AE_MANUAL_GAIN,
	ISP_CTRL_SET_AE_MANUAL_ISO,
	ISP_CTRL_SET_AE_ENGINEER_MODE,
	ISP_CTRL_GET_YIMG_INFO,
	ISP_CTRL_SET_PREV_YIMG,
	ISP_CTRL_SET_PREV_YUV,
	ISP_CTRL_SET_PREV_PDAF_RAW,
	ISP_CTRL_GET_VCM_INFO,
	ISP_CTRL_GET_FPS,
	ISP_CTRL_GET_LEDS_CTRL,
	/* warning if you wanna send async msg
	 * please add msg id below here */
	ISP_CTRL_SYNC_NONE_MSG_BEGIN,
	ISP_CTRL_SYNC_NONE_MSG_END,
	/* warning if you wanna set ioctrl directly
	 * please add msg id below here */
	ISP_CTRL_DIRECT_MSG_BEGIN,
	ISP_CTRL_SET_AUX_SENSOR_INFO,
	ISP_CTRL_DIRECT_MSG_END,
	ISP_CTRL_SET_DCAM_TIMESTAMP,
	ISP_CTRL_GET_FULLSCAN_INFO,
	ISP_CTRL_SET_AF_BYPASS,
	ISP_CTRL_POST_3DNR, //for post 3dnr
	ISP_CTRL_3DNR,
	ISP_CTRL_SENSITIVITY,
	ISP_CTRL_MAX
};

enum isp_capbility_cmd {
	ISP_VIDEO_SIZE,
	ISP_CAPTURE_SIZE,
	ISP_LOW_LUX_EB,
	ISP_CUR_ISO,
	ISP_DENOISE_LEVEL,
	ISP_DENOISE_INFO,
	ISP_REG_VAL,
	ISP_CTRL_GET_AE_LUM,	//for LLS feature
	ISP_CAPBILITY_MAX
};

enum isp_ae_lock_unlock_mode {
	ISP_AE_UNLOCK,
	ISP_AE_LOCK,
	ISP_AE_LOCK_UNLOCK_MAX
};

enum isp_flash_led_tag {
	ISP_FLASH_LED_0 = 0x0001,
	ISP_FLASH_LED_1 = 0x0002
};

enum {
	ISP_SINGLE = 0,
	ISP_DUAL_NORMAL,
	ISP_DUAL_SBS,
	ISP_CAMERA_MAX
};

struct isp_flash_cfg {
	cmr_u32 type;		// enum isp_flash_type
	cmr_u32 led_idx;	//enum isp_flash_led
	cmr_u32 led0_enable;
	cmr_u32 led1_enable;
};

struct isp_3dnr_ctrl_param {
	cmr_u32 enable;
	cmr_u32 count;
};

struct isp_adgain_exp_info {
	cmr_u32 adgain;
	cmr_u32 exp_time;
	cmr_u32 bv;
};

struct isp_yimg_info {
	cmr_uint yaddr[2];
	cmr_u32 lock[2];
};

struct yimg_info {
	cmr_uint y_addr[2];
	cmr_uint y_size;
	cmr_s32 ready[2];
	cmr_s32 sec;
	cmr_s32 usec;
	cmr_uint camera_id;
};

struct yuv_info_t {
	cmr_uint camera_id;
	cmr_u8 *yuv_addr;
	cmr_u32 width;
	cmr_u32 height;
};

struct pd_frame_in {
	cmr_handle caller_handle;
	cmr_u32 camera_id;
	void *private_data;
};

struct trim_info {
	cmr_u32 image_width;
	cmr_u32 image_height;
	cmr_u32 trim_start_x;
	cmr_u32 trim_start_y;
	cmr_u32 trim_width;
	cmr_u32 trim_height;
};

struct isp_af_notice {
	cmr_u32 mode;
	cmr_u32 valid_win;
	cmr_u32 focus_type;
	cmr_u32 reserved[10];
};

enum isp_flash_type {
	ISP_FLASH_TYPE_PREFLASH,
	ISP_FLASH_TYPE_MAIN,
	ISP_FLASH_TYPE_MAX
};

struct isp_flash_power {
	cmr_s32 max_charge;	//mA
	cmr_s32 max_time;	//ms
};

struct isp_flash_led_info {
	struct isp_flash_power power_0;
	struct isp_flash_power power_1;
	cmr_u32 led_tag;	// isp_flash_led_tag
};

struct isp_flash_notice {
	enum isp_flash_mode mode;
	cmr_u32 flash_ratio;
	cmr_u32 will_capture;
	struct isp_flash_power power;
	cmr_s32 capture_skip_num;
	struct isp_flash_led_info led_info;
};

struct isp_af_win {
	enum isp_focus_mode mode;
	struct isp_pos_rect win[9];
	cmr_u32 valid_win;
	cmr_u32 ae_touch;
	struct isp_pos_rect ae_touch_rect;
};

struct isp_af_fullscan_info {
	/* Register Parameters */
	/* These params will depend on the AF setting */
	cmr_u8 row_num;		/* The number of AF windows with row (i.e. vertical) *//* depend on the AF Scanning */
	cmr_u8 column_num;	/* The number of AF windows with row (i.e. horizontal) *//* depend on the AF Scanning */
	cmr_u32 *win_peak_pos;	/* The seqence of peak position which be provided via struct isp_af_fullscan_info *//* depend on the AF Scanning */
	cmr_u16 vcm_dac_up_bound;
	cmr_u16 vcm_dac_low_bound;
	cmr_u16 boundary_ratio;	/*  (Unit : Percentage) *//* depend on the AF Scanning */
	cmr_u32 af_peak_pos;
	cmr_u32 near_peak_pos;
	cmr_u32 far_peak_pos;
	cmr_u32 distance_reminder;
	cmr_u32 reserved[16];
	/* The configuration for the af scanning */
	//cmr_u8 valid_depth_clip; /* The up bound of valid_depth */ /* For Tuning */
	//cmr_u8 method; /* The depth method. (Resaved) */ /* For Tuning */
	/* No need to be set */
	/* Customer Parameter */
	//cmr_u16 sel_x; /* The point which be touched */
	//cmr_u16 sel_y; /* The point which be touched */
	//cmr_u8 sel_size; /* The size of area which be touched (Resaved) */ /* For Tuning */
	//cmr_u16 slope; /* For Tuning */
	//cmr_u8 valid_depth; /* For Tuning */
};

enum af_aux_sensor_type {
	AF_ACCELEROMETER,
	AF_MAGNETIC_FIELD,
	AF_GYROSCOPE,
	AF_LIGHT,
	AF_PROXIMITY,
};

struct af_gyro_info_t {
	cmr_s64 timestamp;
	float x;
	float y;
	float z;
};

struct af_gsensor_info {
	cmr_s64 timestamp;
	float vertical_up;
	float vertical_down;
	float horizontal;
	cmr_u32 valid;
};

struct af_aux_sensor_info_t {
	enum af_aux_sensor_type type;
	union {
		struct af_gyro_info_t gyro_info;
		struct af_gsensor_info gsensor_info;
	};
};

struct isp_af_ts {
	cmr_u64 timestamp;
	cmr_u32 capture;
};

struct isp_face_info {
	cmr_u32 sx;
	cmr_u32 sy;
	cmr_u32 ex;
	cmr_u32 ey;
	cmr_u32 brightness;
	cmr_s32 pose;
	cmr_s32 angle;
};

struct isp_face_area {
	cmr_u16 type;		//focus or ae,
	cmr_u16 face_num;
	cmr_u16 frame_width;
	cmr_u16 frame_height;
	struct isp_face_info face_info[10];
};

struct isp_img_mfd {
	cmr_u32 y;
	cmr_u32 u;
	cmr_u32 v;
};

struct isp_img_frm {
	enum isp_format img_fmt;
	struct isp_size img_size;
	struct isp_img_mfd img_fd;
	struct isp_addr img_addr_phy;
	struct isp_addr img_addr_vir;
	cmr_u32 format_pattern;
};

struct isp_flash_element {
	cmr_u16 index;
	cmr_u16 val;
};

struct isp_flash_cell {
	cmr_u8 type;
	cmr_u8 count;
	cmr_u8 def_val;
	struct isp_flash_element element[ISP_FLASH_MAX_CELL];
};

struct isp_sensor_ex_info {
	cmr_u32 f_num;
	cmr_u32 focal_length;
	cmr_u32 max_fps;
	cmr_u32 max_adgain;
	cmr_u32 ois_supported;
	cmr_u32 pdaf_supported;
	cmr_u32 exp_valid_frame_num;
	cmr_u32 clamp_level;
	cmr_u32 adgain_valid_frame_num;
	cmr_u32 preview_skip_num;
	cmr_u32 capture_skip_num;
	cmr_s8 *name;
	cmr_s8 *sensor_version_info;
	struct af_pose_dis pos_dis;
	cmr_u32 af_supported;
};

struct isp_video_limit {
	cmr_u16 width;
	cmr_u16 height;
	cmr_u32 res;
};

struct isp_sensor_fps_info {
	cmr_u32 mode;		//sensor mode
	cmr_u32 max_fps;
	cmr_u32 min_fps;
	cmr_u32 is_high_fps;
	cmr_u32 high_fps_skip_num;
};

struct isp_snapshot_notice {
	cmr_u32 type;
	cmr_u32 preview_line_time;
	cmr_u32 capture_line_time;
};

struct isp_range_fps {
	cmr_u16 min_fps;
	cmr_u16 max_fps;
};

struct isp_ae_fps {
	cmr_u32 min_fps;
	cmr_u32 max_fps;
};

struct isp_hdr_param {
	cmr_u32 hdr_enable;
	cmr_u32 ev_effect_valid_num;
};

struct isp_info {
	void *addr;
	cmr_s32 size;
};

struct isp_hdr_ev_param {
	cmr_s32 level;
	cmr_s32 skip_frame_num;	//return from isp
};

struct isp_sensor_resolution_info {
	struct isp_size yuv_img_size;
	struct isp_size sensor_size;
	struct isp_rect crop;
	struct isp_range_fps fps;
	cmr_u32 line_time;
	cmr_u32 frame_line;
	cmr_u32 size_index;
	cmr_u32 max_gain;
	struct isp_size sensor_max_size;
	struct isp_size sensor_output_size;
};

struct isp_sbs_info {
	cmr_u32 sbs_mode;
	struct isp_size img_size;
};

struct ips_in_param {
	struct isp_img_frm src_frame;
	cmr_u32 src_avail_height;
	cmr_u32 src_slice_height;
	struct isp_img_frm dst_frame;
	cmr_u32 dst_slice_height;

	struct isp_img_frm dst2_frame;
	cmr_u32 dst2_slice_height;
	struct isp_sensor_resolution_info resolution_info;
	struct isp_sensor_fps_info sensor_fps;
	cmr_u32 cap_mode;
	struct isp_sbs_info sbs_info;
	cmr_handle oem_handle;
	cmr_malloc alloc_cb;
	cmr_free free_cb;
	cmr_u32 sensor_id;
};

struct ips_out_param {
	cmr_u32 output_height;
};

struct ipn_in_param {
	cmr_u32 src_avail_height;
	cmr_u32 src_slice_height;
	struct isp_addr img_addr_phy;
	struct isp_addr src_addr_phy;
	struct isp_addr dst_addr_phy;
};

struct isp_video_start {
	cmr_u16 is_snapshot;
	cmr_u32 dv_mode;
	void *cb_of_malloc;
	void *cb_of_free;
	void *buffer_client_data;

	struct isp_size size;
	struct isp_sensor_resolution_info resolution_info;
	cmr_u16 is_slow_motion;
	cmr_u16 is_refocus;
	enum isp_format format;
	enum isp_video_mode mode;
	cmr_u32 work_mode;
	cmr_u32 capture_mode;	//enum isp_capture_mode
	cmr_uint lsc_buf_size;
	cmr_uint lsc_buf_num;
	cmr_uint lsc_phys_addr;
	cmr_uint lsc_virt_addr;
	cmr_s32 lsc_mfd;
	cmr_uint b4awb_mem_size;
	cmr_uint b4awb_mem_num;
	cmr_uint b4awb_phys_addr_array[2];
	cmr_uint b4awb_virt_addr_array[2];
	cmr_uint anti_flicker_buf_size;
	cmr_uint anti_flicker_buf_num;
	cmr_uint anti_flicker_phys_addr;
	cmr_uint anti_flicker_virt_addr;
	cmr_u32 is_need_flash;
	cmr_u32 capture_skip_num;
	struct isp_sensor_fps_info sensor_fps;
	void *tuning_ae_addr;
	cmr_s32 raw_buf_fd;
	cmr_uint raw_buf_phys_addr;
	cmr_uint raw_buf_virt_addr;
	cmr_uint raw_buf_size;
	cmr_uint raw_buf_width;
	cmr_uint raw_buf_height;
	cmr_s32 highiso_buf_fd;
	cmr_uint highiso_buf_phys_addr;
	cmr_uint highiso_buf_virt_addr;
	cmr_uint highiso_buf_size;
	struct isp_size live_view_sz;
	cmr_u8 pdaf_enable;
	cmr_handle oem_handle;
	cmr_malloc alloc_cb;
	cmr_free free_cb;
};

struct isp_img_param {
	cmr_u32 img_fmt;
	cmr_u32 channel_id;
	cmr_u32 base_id;
	cmr_u32 count;
	cmr_u32 length;
	cmr_u32 slice_height;
	cmr_u32 start_buf_id;
	cmr_u32 is_reserved_buf;
	cmr_u32 flag;
	cmr_u32 index;
	struct isp_size img_size;
	struct isp_img_mfd img_fd;
	struct isp_addr addr;
	struct isp_addr addr_vir;
	cmr_uint zsl_private;
};

//add two struct defination for the 3DNR capture.
struct isp_buffer
{
        cmr_u8 *buffer;
	cmr_s32 fd;
};

struct isp_3dnr_info
{
        struct isp_buffer image[3];
        cmr_u32 width;
        cmr_u32 height;
        cmr_s8 mv_x;
        cmr_s8 mv_y;
        cmr_u8 blending_no;
};

struct isp_ops {
	cmr_s32(*flash_get_charge) (void *handler, struct isp_flash_cfg * cfg_ptr, struct isp_flash_cell * cell);
	cmr_s32(*flash_get_time) (void *handler, struct isp_flash_cfg * cfg_ptr, struct isp_flash_cell * cell);
	cmr_s32(*flash_set_charge) (void *handler, struct isp_flash_cfg * cfg_ptr, struct isp_flash_element * element);
	cmr_s32(*flash_set_time) (void *handler, struct isp_flash_cfg * cfg_ptr, struct isp_flash_element * element);
	cmr_s32(*flash_ctrl) (void *handler, struct isp_flash_cfg * cfg_ptr, struct isp_flash_element * element);
};

struct isp_init_param {
	void *setting_param_ptr;
	struct isp_size size;
	proc_callback ctrl_callback;
	cmr_handle oem_handle;
	struct isp_data_info calibration_param;
	cmr_u32 camera_id;
	void *sensor_lsc_golden_data;
	struct isp_ops ops;
	cmr_malloc alloc_cb;
	cmr_free free_cb;
	void *setting_param_list_ptr[3];	//0:back,1:front,2:dual back
	struct isp_sensor_ex_info ex_info;
	struct sensor_otp_cust_info *otp_data;
	struct sensor_data_info pdaf_otp;
	struct sensor_pdaf_info *pdaf_info;
	struct isp_size sensor_max_size;
#ifdef CONFIG_CAMERA_RT_REFOCUS
	struct isp_sensor_ex_info ex_info_slv;
	void *setting_param_ptr_slv;	// slave sensor
	struct sensor_otp_cust_info *otp_data_slv;
#endif

	void *setting_param_ptr_slv; // slave sensor
	struct isp_sensor_ex_info ex_info_slv;
	struct sensor_otp_cust_info *otp_data_slv;
	cmr_u32 is_multi_mode;
	cmr_u8 is_master;
	cmr_u32 image_pattern;
	cmr_s32 dcam_fd;
};

typedef cmr_int(*isp_cb_of_malloc) (cmr_uint type, cmr_uint * size_ptr, cmr_uint * sum_ptr, cmr_uint * phy_addr, cmr_uint * vir_addr, cmr_s32 * mfd, void *private_data);
typedef cmr_int(*isp_cb_of_free) (cmr_uint type, cmr_uint * phy_addr, cmr_uint * vir_addr, cmr_s32 * fd, cmr_uint sum, void *private_data);
typedef cmr_int(*isp_ae_cb) (cmr_handle handle, cmr_int type, void *param0, void *param1);
typedef cmr_int(*isp_af_cb) (cmr_handle handle, cmr_int type, void *param0, void *param1);
typedef cmr_int(*isp_pdaf_cb) (cmr_handle handle, cmr_int type, void *param0, void *param1);
typedef cmr_int(*isp_afl_cb) (cmr_handle handle, cmr_int type, void *param0, void *param1);

/**---------------------------------------------------------------------------*
**				API					*
**----------------------------------------------------------------------------*/

cmr_int isp_init(struct isp_init_param *ptr, cmr_handle * isp_handler);
cmr_int isp_deinit(cmr_handle isp_handler);
cmr_int isp_capability(cmr_handle isp_handler, enum isp_capbility_cmd cmd, void *param_ptr);
cmr_int isp_ioctl(cmr_handle isp_handler, enum isp_ctrl_cmd cmd, void *param_ptr);
cmr_int isp_video_start(cmr_handle isp_handler, struct isp_video_start *param_ptr);
cmr_int isp_video_stop(cmr_handle isp_handler);
cmr_int isp_proc_start(cmr_handle isp_handler, struct ips_in_param *in_param_ptr, struct ips_out_param *out_ptr);
cmr_int isp_proc_next(cmr_handle isp_handler, struct ipn_in_param *in_ptr, struct ips_out_param *out_ptr);
cmr_int isp_cap_buff_cfg(cmr_handle isp_handle, struct isp_img_param *buf_cfg);
void ispmw_dev_buf_cfg_evt_cb(cmr_handle isp_handle, isp_buf_cfg_evt_cb grab_event_cb);
void isp_statis_evt_cb(cmr_int evt, void *data, void *privdata);
void isp_irq_proc_evt_cb(cmr_int evt, void *data, void *privdata);
/**---------------------------------------------------------------------------*/

#endif
