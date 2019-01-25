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

#include <sys/types.h>
#include "isp_common_types.h"
#include "cmr_sensor_info.h"


typedef cmr_int (*proc_callback)(cmr_handle handle, cmr_u32 mode, void *param_ptr, cmr_u32 param_len);

#define ISP_FLASH_MAX_CELL                             40
#define ISP_MODE_NUM_MAX                               16
#define ISP_EVT_MASK                                   0x0000FF00
#define ISP_CALLBACK_EVT                               0x00040000
#define ISP_SNR_NAME_MAX_LEN                           64
#define ISP_MW_EVT_BASE                                3 //(1 << 3)

/*******************************enum type*************************************************/
enum isp_callback_cmd {
	ISP_CTRL_CALLBACK = 0x00000000,
	ISP_PROC_CALLBACK = 0x00000100,  // post process done
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
	ISP_FLASH_PRE_AFTER_PRE,
	ISP_FLASH_PRE_AFTER,
	ISP_FLASH_MAIN_BEFORE,
	ISP_FLASH_MAIN_LIGHTING,
	ISP_FLASH_MAIN_AE_MEASURE,
	ISP_FLASH_MAIN_AFTER_PRE,
	ISP_FLASH_MAIN_AFTER,
	ISP_FLASH_AF_DONE,
	ISP_FLASH_MODE_MAX
};

enum isp_ui_flash_status {
	ISP_UI_FLASH_STATUS_OFF,
	ISP_UI_FLASH_STATUS_ON,
	ISP_UI_FLASH_STATUS_TORCH,
	ISP_UI_FLASH_STATUS_AUTO,
};

enum isp_ae_awb_lock_unlock_mode {
	ISP_AE_AWB_LOCK = 0x09,
	ISP_AE_AWB_UNLOCK = 0x0a,
	ISP_AE_AWB_MAX
};

enum isp_awb_lock_unlock_mode {
	ISP_AWB_LOCK,
	ISP_AWB_UNLOCK,
	ISP_AWB_LOCK_MAX
};

enum isp_af_lock_unlock_mode{
	ISP_AF_LOCK = 0x00,
	ISP_AF_UNLOCK,
	ISP_AF_LOCK_MAX
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

enum isp_ae_lock_unlock_mode {
	ISP_AE_UNLOCK,
	ISP_AE_LOCK,
	ISP_AE_LOCK_UNLOCK_MAX
};

enum isp_posture_type {
	ISP_POSTURE_ACCELEROMETER,
	ISP_POSTURE_MAGNETIC,
	ISP_POSTURE_ORIENTATION,
	ISP_POSTURE_GYRO,
	ISP_POSTURE_MAX
};

enum isp_flash_led_tag {
	ISP_FLASH_LED_0 = 0x0001,
	ISP_FLASH_LED_1 = 0x0002
};

enum isp_flash_type {
	ISP_FLASH_TYPE_PREFLASH,
	ISP_FLASH_TYPE_MAIN,
	ISP_FLASH_TYPE_MAX
};

enum isp_capbility_cmd {
	ISP_VIDEO_SIZE,
	ISP_CAPTURE_SIZE,
	ISP_LOW_LUX_EB,
	ISP_CUR_ISO,
	ISP_DENOISE_LEVEL,
	ISP_DENOISE_INFO,
	ISP_REG_VAL,
	ISP_CTRL_GET_AE_LUM,  // for LLS feature
	ISP_CAPBILITY_MAX
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
	ISP_CTRL_AF,
	ISP_CTRL_SATURATION = 11,
	ISP_CTRL_AF_MODE = 12,
	ISP_CTRL_AUTO_CONTRAST,
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
	ISP_CTRL_FLASH_CTRL = 38,  // for isp tool
	ISP_CTRL_AE_CTRL = 39,  // for isp tool
	ISP_CTRL_AF_CTRL = 40,  // for isp tool
	ISP_CTRL_REG_CTRL = 41,  // for isp tool
	ISP_CTRL_DENOISE_PARAM_READ,  // for isp tool
	ISP_CTRL_DUMP_REG,  // for isp tool
	ISP_CTRL_AF_END_INFO,  // for isp tool
	ISP_CTRL_FLASH_NOTICE,
	ISP_CTRL_AE_FORCE_CTRL,  // for mp tool
	ISP_CTRL_GET_AE_STATE,  // for isp tool
	ISP_CTRL_SET_LUM,  // for isp tool
	ISP_CTRL_GET_LUM,  // for isp tool
	ISP_CTRL_SET_AF_POS,  // for isp tool
	ISP_CTRL_GET_AF_POS,  // for isp tool
	ISP_CTRL_GET_AF_MODE,  // for isp tool
	ISP_CTRL_FACE_AREA,
	ISP_CTRL_SCALER_TRIM,
	ISP_CTRL_START_3A,
	ISP_CTRL_STOP_3A,
	IST_CTRL_SNAPSHOT_NOTICE,
	ISP_CTRL_SFT_READ,
	ISP_CTRL_SFT_WRITE,
	ISP_CTRL_SFT_SET_PASS,  // added for sft
	ISP_CTRL_SFT_GET_AF_VALUE,  // added for sft
	ISP_CTRL_SFT_SET_BYPASS,  // added for sft
	ISP_CTRL_GET_AWB_GAIN,  // for mp tool
	ISP_CTRL_RANGE_FPS,
	ISP_CTRL_SET_AE_FPS,  // for LLS feature
	ISP_CTRL_BURST_NOTICE,  // burst mode notice
	ISP_CTRL_GET_INFO,
	ISP_CTRL_SET_AE_NIGHT_MODE,
	ISP_CTRL_SET_AE_AWB_LOCK_UNLOCK,
	ISP_CTRL_SET_AWB_LOCK_UNLOCK,
	ISP_CTRL_SET_AF_LOCK_UNLOCK,
	ISP_CTRL_SET_AE_LOCK_UNLOCK,
	ISP_CTRL_IFX_PARAM_UPDATE,
	ISP_CTRL_SET_DZOOM_FACTOR,
	ISP_CTRL_SET_CONVERGENCE_REQ,
	ISP_CTRL_SET_SNAPSHOT_FINISHED,
	ISP_CTRL_SET_EXIF_DEBUG_INFO,
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
	ISP_CTRL_GET_VCM_INFO,
	ISP_CTRL_SET_TUNING_MODE,

	/*
	 * warning if you wanna send async msg
	 * please add msg id below here
	 * */
	ISP_CTRL_SYNC_NONE_MSG_BEGIN,
	ISP_CTRL_SYNC_NONE_MSG_END,

	/*
	 * warning if you wanna set ioctrl directly
	 * please add msg id below here
	  * */
	ISP_CTRL_DIRECT_MSG_BEGIN,
	ISP_CTRL_SET_AUX_SENSOR_INFO,
	ISP_CTRL_SET_PREV_PDAF_RAW,
	ISP_CTRL_SET_PREV_PDAF_OPEN,
	ISP_CTRL_SET_HAF_ENABLE,
	ISP_CTRL_DIRECT_MSG_END,
	ISP_CTRL_MAX
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
	ISP_CAP_MODE_BURST,
	ISP_CAP_MODE_MAX
};

enum isp_tuning_mode {
	ISP_TUNING_PREVIEW_BINNING = 0x00, /*bining size for non zsl preview*/
	ISP_TUNING_PREVIEW_FULL,   /*sensor full size for non zsl preview*/
	ISP_TUNING_VIDEO_BINNING,  /*sensor bining size for video record*/
	ISP_TUNING_VIDEO_FULL,        /*sensor full size for video record*/
	ISP_TUNING_CAPTURE_BINNING,    /*sensor bining size for capture*/
	ISP_TUNING_CAPTURE_FULL,       /*sensor full size for capture*/
	ISP_TUNING_MULTILAYER_BINNING,/*bining size of multilayer capture*/
	ISP_TUNING_MULTILAYER_FULL, /*full size of multilayer capture*/
	ISP_TUNING_HDR_BINNING,      /*sensor bining size for  hdr capture*/
	ISP_TUNING_HDR_FULL,            /*sensor full size for  hdr capture*/
	ISP_TUNING_SLOWVIDEO,        /*sensor size for  slowmotion*/
	ISP_TUNING_MAX
};

enum isp_mw_evt {
	ISP_MW_CFG_BUF = ISP_MW_EVT_BASE,
	ISP_MW_ALTEK_RAW,
};

/*********************************struct type*********************************************/
struct isp_af_notice {
	cmr_u32 mode;
	cmr_u32 valid_win;
};

struct isp_flash_power {
	cmr_s32 max_charge;  // mA
	cmr_s32 max_time;  // ms
};

struct isp_flash_led_info {
	struct isp_flash_power power_0;
	struct isp_flash_power power_1;
	cmr_u32 led_tag; // isp_flash_led_tag
};

struct isp_flash_notice {
	enum isp_flash_mode mode;
	cmr_u32 flash_ratio;
	cmr_u32 will_capture;
	struct isp_flash_led_info led_info;
	cmr_s32 capture_skip_num;
};

struct isp_af_win {
	enum isp_focus_mode mode;
	struct isp_pos_rect win[9];
	cmr_u32 valid_win;
	cmr_u32 ae_touch;
	struct isp_pos_rect ae_touch_rect;
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

struct isp_face_info {
	cmr_u32   sx;
	cmr_u32   sy;
	cmr_u32   ex;
	cmr_u32   ey;
	cmr_u32   brightness;
	cmr_u32   pose;
	cmr_s32 angle;
};

struct isp_face_area {
	cmr_u16 type;  // focus or ae,
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
	struct isp_addr img_addr_phy;  // isp_add is from kernel header(sprd_isp_altek.h)
	struct isp_addr img_addr_vir;
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

struct isp_flash_cfg {
	cmr_u32 type; // enum isp_flash_type
	cmr_u32 led_idx; //enum isp_flash_led
};

struct isp_ops {
	cmr_s32 (*flash_get_charge)(void *handler, struct isp_flash_cfg *cfg_ptr, struct isp_flash_cell *cell);
	cmr_s32 (*flash_get_time)(void *handler, struct isp_flash_cfg *cfg_ptr, struct isp_flash_cell *cell);
	cmr_s32 (*flash_set_charge)(void *handler, struct isp_flash_cfg *cfg_ptr, struct isp_flash_element *element);
	cmr_s32 (*flash_ctrl)(void *handler, struct isp_flash_cfg *cfg_ptr, struct isp_flash_element *element);
	cmr_s32 (*flash_set_time)(void *handler, struct isp_flash_cfg *cfg_ptr, struct isp_flash_element *element);
};

struct isp_ex_jpg_info {
	cmr_u8 mirror;   /*  0: off, 1: turn on */
	cmr_uint orientation;
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
	struct isp_ex_jpg_info ex_jpg_info;
};

struct isp_sensor_fps_info {
	cmr_u32 mode;	//sensor mode
	cmr_u32 max_fps;
	cmr_u32 min_fps;
	cmr_u32 is_high_fps;
	cmr_u32 high_fps_skip_num;
};

struct isp_adgain_exp_info {
	cmr_u32 adgain;
	cmr_u32 exp_time;
};

struct isp_yimg_info {
	cmr_uint yaddr[2];
	cmr_u32 lock[2];
};

struct yimg_info {
	cmr_uint                                  y_addr[2];
	cmr_uint                                  y_size;
	cmr_s32                                   ready[2];
	cmr_s32                                   sec;
	cmr_s32                                   usec;
	cmr_uint                                  camera_id;
};

struct yuv_info_t {
	cmr_uint                                  camera_id;
	cmr_u8                                    *yuv_addr;
	cmr_u32                                   width;
	cmr_u32                                   height;
};

struct pd_frame_in {
	cmr_handle caller_handle;
	cmr_u32 camera_id;
	cmr_uint src_phy_addr;
	cmr_uint src_vir_addr;
	cmr_u32 fd;
};

struct pd_raw_open {
	cmr_u8 open;
	cmr_u32 size;
	cmr_int (*pd_set_buffer) (struct pd_frame_in *cb_param);
};

struct trim_info {
	cmr_u32 image_width;
	cmr_u32 image_height;
	cmr_u32 trim_start_x;
	cmr_u32 trim_start_y;
	cmr_u32 trim_width;
	cmr_u32 trim_height;
};

struct pd_raw_info {
	cmr_u32 frame_id;
	void *addr;
	cmr_u32 len;
	cmr_uint sec;
	cmr_uint usec;
	cmr_u32 format;
	cmr_u32 pattern;
	struct trim_info roi;
	struct pd_frame_in pd_in;
	cmr_int (*pd_callback) (struct pd_frame_in *cb_param);
};

struct isp_init_param {
	cmr_u32 camera_id;
	cmr_u16 is_refocus;
	void *setting_param_ptr;
	void *setting_param_ptr_slv; // slave sensor
	struct isp_size size;
	proc_callback ctrl_callback;
	cmr_handle oem_handle;
	struct isp_data_info calibration_param;
	void *sensor_lsc_golden_data;
	struct isp_ops ops;
	struct isp_data_info mode_ptr[ISP_MODE_NUM_MAX];
	cmr_malloc alloc_cb;
	cmr_free   free_cb;
	void *setting_param_list_ptr[3];//0:back,1:front,2:dual back,
	struct isp_sensor_ex_info ex_info;
	struct isp_sensor_ex_info ex_info_slv;
	struct sensor_otp_cust_info *otp_data;
	struct sensor_otp_cust_info *otp_data_slv;
	struct sensor_dual_otp_info *dual_otp;
	struct sensor_pdaf_info *pdaf_info;
	cmr_u32 image_pattern;
};

struct isp_video_limit {
	cmr_u16 width;
	cmr_u16 height;
	cmr_u32 res;
};

struct isp_range_fps {
	cmr_u16 min_fps;
	cmr_u16 max_fps;
};

struct isp_ae_fps {
	cmr_u32 min_fps;
	cmr_u32 max_fps;
};

struct isp_info {
	void *addr;
	cmr_int size;
	cmr_u8 update_exif;
	struct isp_ex_jpg_info ex_jpg_exif;
};

struct isp_hdr_ev_param {
	cmr_s32 level;
	cmr_s32 skip_frame_num;  // return from isp
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

struct isp_video_start {
	struct isp_size size;
	struct isp_sensor_resolution_info resolution_info;
	cmr_u16 is_slow_motion;
	cmr_u16 is_refocus;
	enum isp_format format;
	enum isp_video_mode mode;
	enum isp_tuning_mode tuning_mode;
	cmr_u32 work_mode;
	cmr_u32 capture_mode; //enum isp_capture_mode
	cmr_uint lsc_buf_size;
	cmr_uint lsc_buf_num;
	cmr_uint lsc_phys_addr;
	cmr_uint lsc_virt_addr;
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
	cmr_s32 raw_buf_fd[ISP_RAWBUF_NUM];
	cmr_uint raw_buf_phys_addr[ISP_RAWBUF_NUM];
	cmr_uint raw_buf_virt_addr[ISP_RAWBUF_NUM];
	cmr_u32  raw_buf_cnt;
	cmr_uint raw_buf_size;
	cmr_uint raw_buf_width;
	cmr_uint raw_buf_height;
	cmr_s32 highiso_buf_fd;
	cmr_uint highiso_buf_phys_addr;
	cmr_uint highiso_buf_virt_addr;
	cmr_uint highiso_buf_size;
	struct isp_size live_view_sz;
	struct isp_size lv_size;
	struct isp_size video_size;
};

struct ips_in_param {
	struct isp_img_frm src_frame;
	cmr_u32 src_avail_height;
	cmr_u32 src_slice_height;
	struct isp_img_frm dst_frame;
	struct isp_img_frm dst2_frame;
	cmr_u32 dst_slice_height;
	cmr_u32 dst2_slice_height;
	struct isp_sensor_resolution_info resolution_info;
	struct isp_sensor_fps_info sensor_fps;
	cmr_u32 cap_mode;
	cmr_s32 raw_buf_fd[ISP_RAWBUF_NUM];
	cmr_uint raw_buf_phys_addr[ISP_RAWBUF_NUM];
	cmr_uint raw_buf_virt_addr[ISP_RAWBUF_NUM];
	cmr_u32  raw_buf_cnt;
	cmr_uint raw_buf_size;
	cmr_uint raw_buf_width;
	cmr_uint raw_buf_height;
	cmr_s32 highiso_buf_fd;
	cmr_uint highiso_buf_phys_addr;
	cmr_uint highiso_buf_virt_addr;
	cmr_uint highiso_buf_size;
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

struct isp_snapshot_notice {
	cmr_u32 type;
	cmr_u32 preview_line_time;
	cmr_u32 capture_line_time;
};

struct isp_img_param {
	cmr_u32                    img_fmt;
	cmr_u32                    channel_id;
	cmr_u32                    base_id;
	cmr_u32                    count;
	cmr_u32                    length;
	cmr_u32                    slice_height;
	cmr_u32                    start_buf_id;
	cmr_u32                    is_reserved_buf;
	cmr_u32                    flag;
	cmr_u32                    index[ISP_GRAB_BUF_MAX];
	struct isp_size            img_size;
	struct isp_img_mfd         img_fd[ISP_GRAB_BUF_MAX];
	struct isp_addr            addr[ISP_GRAB_BUF_MAX];
	struct isp_addr            addr_vir[ISP_GRAB_BUF_MAX];
	cmr_uint                   zsl_private;
};

cmr_int isp_init(struct isp_init_param *input_ptr, cmr_handle *isp_handle);
cmr_int isp_deinit(cmr_handle isp_handle);
cmr_int isp_capability(cmr_handle isp_handle, enum isp_capbility_cmd cmd, void *param_ptr);
cmr_int isp_ioctl(cmr_handle isp_handle, enum isp_ctrl_cmd cmd, void *param_ptr);
cmr_int isp_video_start(cmr_handle isp_handle, struct isp_video_start *param_ptr);
cmr_int isp_video_stop(cmr_handle isp_handle);
cmr_int isp_proc_start(cmr_handle isp_handle, struct ips_in_param *input_ptr, struct ips_out_param *output_ptr);
cmr_int isp_proc_next(cmr_handle isp_handle, struct ipn_in_param *input_ptr, struct ips_out_param *output_ptr);
cmr_int isp_cap_buff_cfg(cmr_handle isp_handle, struct isp_img_param *buf_cfg);
cmr_int isp_drammode_takepic (cmr_handle isp_handle, cmr_u32 is_start);
void ispmw_dev_buf_cfg_evt_cb(cmr_handle isp_handle, isp_buf_cfg_evt_cb grab_event_cb);
#endif
