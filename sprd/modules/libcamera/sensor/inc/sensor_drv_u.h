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
#ifndef _SENSOR_DRV_U_H_
#define _SENSOR_DRV_U_H_
#include "jpeg_exif_header.h"
#include "cmr_common.h"
#include <sys/types.h>
#include "cmr_msg.h"
#include "sensor_raw.h"
#include "cmr_log.h"
#include "../../sensor/hw_drv/hw_sensor_drv.h"
#include "../../sensor/sensor_drv/sensor_ic_drv.h"

#ifdef __cplusplus
extern "C" {
#endif

/*sensor operation return status*/
#define SENSOR_SUCCESS CMR_CAMERA_SUCCESS
#define SENSOR_FAIL CMR_CAMERA_FAIL
#define SENSOR_CTX_ERROR CMR_CAMERA_INVALID_PARAM

/*bool type*/
#define SENSOR_FALSE 0
#define SENSOR_TRUE 1

#define SENSOR_Sleep(ms) usleep(ms * 1000)
#define SENSOR_MEMSET memset

#define BOOLEAN cmr_u32

#ifndef PNULL
#define PNULL ((void *)0)
#endif

#define LOCAL static

#define NUMBER_OF_ARRAY(a) (sizeof(a) / sizeof(a[0]))
#define ADDR_AND_LEN_OF_ARRAY(a) (SENSOR_REG_T *) a, NUMBER_OF_ARRAY(a)

#define SENSOR_DEFALUT_MCLK 24 /* MHZ */
#define SENSOR_DISABLE_MCLK 0  /* MHZ */
#define SENSOR_LOW_PULSE_RESET 0x00
#define SENSOR_HIGH_PULSE_RESET 0x01
#define SENSOR_RESET_PULSE_WIDTH_DEFAULT 10
#define SENSOR_RESET_PULSE_WIDTH_MAX 200

#define SENSOR_LOW_LEVEL_PWDN 0x00
#define SENSOR_HIGH_LEVEL_PWDN 0x01

#define CAP_MODE_BITS 16

/* bit define */
#define S_BIT_0 0x00000001
#define S_BIT_1 0x00000002
#define S_BIT_2 0x00000004
#define S_BIT_3 0x00000008
#define S_BIT_4 0x00000010
#define S_BIT_5 0x00000020
#define S_BIT_6 0x00000040
#define S_BIT_7 0x00000080
#define S_BIT_8 0x00000100

/*Image effect*/
#define SENSOR_IMAGE_EFFECT_NORMAL (0x01 << 0)
#define SENSOR_IMAGE_EFFECT_BLACKWHITE (0x01 << 1)
#define SENSOR_IMAGE_EFFECT_RED (0x01 << 2)
#define SENSOR_IMAGE_EFFECT_GREEN (0x01 << 3)
#define SENSOR_IMAGE_EFFECT_BLUE (0x01 << 4)
#define SENSOR_IMAGE_EFFECT_YELLOW (0x01 << 5)
#define SENSOR_IMAGE_EFFECT_NEGATIVE (0x01 << 6)
#define SENSOR_IMAGE_EFFECT_CANVAS (0x01 << 7)
#define SENSOR_IMAGE_EFFECT_EMBOSS (0x01 << 8)
#define SENSOR_IMAGE_EFFECT_OVEREXP (0x01 << 9)

/*While balance mode*/
#define SENSOR_WB_MODE_AUTO (0x01 << 0)
#define SENSOR_WB_MODE_INCANDESCENCE (0x01 << 1)
#define SENSOR_WB_MODE_U30 (0x01 << 2)
#define SENSOR_WB_MODE_CWF (0x01 << 3)
#define SENSOR_WB_MODE_FLUORESCENT (0x01 << 4)
#define SENSOR_WB_MODE_SUN (0x01 << 5)
#define SENSOR_WB_MODE_CLOUD (0x01 << 6)

/*Preview mode*/
#define SENSOR_ENVIROMENT_NORMAL (0x01 << 0)
#define SENSOR_ENVIROMENT_NIGHT (0x01 << 1)
#define SENSOR_ENVIROMENT_SUNNY (0x01 << 2)
#define SENSOR_ENVIROMENT_SPORTS (0x01 << 3)
#define SENSOR_ENVIROMENT_LANDSCAPE (0x01 << 4)
#define SENSOR_ENVIROMENT_PORTRAIT (0x01 << 5)
#define SENSOR_ENVIROMENT_PORTRAIT_NIGHT (0x01 << 6)
#define SENSOR_ENVIROMENT_BACKLIGHT (0x01 << 7)
#define SENSOR_ENVIROMENT_MARCO (0x01 << 8)

#define SENSOR_ENVIROMENT_MANUAL (0x01 << 30)
#define SENSOR_ENVIROMENT_AUTO (0x01 << 31)

/*YUV PATTERN*/
#define SENSOR_IMAGE_PATTERN_YUV422_YUYV 0x00
#define SENSOR_IMAGE_PATTERN_YUV422_YVYU 0x01
#define SENSOR_IMAGE_PATTERN_YUV422_UYVY 0x02
#define SENSOR_IMAGE_PATTERN_YUV422_VYUY 0x03
/*RAW RGB BAYER*/
#define SENSOR_IMAGE_PATTERN_RAWRGB_GR 0x00
#define SENSOR_IMAGE_PATTERN_RAWRGB_R 0x01
#define SENSOR_IMAGE_PATTERN_RAWRGB_B 0x02
#define SENSOR_IMAGE_PATTERN_RAWRGB_GB 0x03

#define SENSOR_WRITE_DELAY 0xffff

#define SENSOR_IOCTL_FUNC_NOT_REGISTER 0xffffffff

/*sensor focus mode*/
#define SENSOR_FOCUS_TRIG 0x01
#define SENSOR_FOCUS_ZONE (0x01 << 1)

/*sensor exposure mode*/
#define SENSOR_EXPOSURE_AUTO 0x01
#define SENSOR_EXPOSURE_ZONE (0x01 << 1)

/*sensor  aperture mode*/
#define SENSOR_APERT_PREVIEW_MODE 2
#define SENSOR_APERT_CAPTURE_MODE 2

/*sensor exposure time mode*/
#define SENSOR_EXP_TIME_PREVIEW_MODE                  2
#define SENSOR_EXP_TIME_CAPTURE_MODE                  2

typedef struct sensor_i2c_seq_tag                      SENSOR_I2C_SEQ_T, *SENSOR_I2C_SEQ_T_PTR;
typedef struct _sensor_otp_param_tag                  SENSOR_OTP_PARAM_T, *SENSOR_OTP_PARAM_T_PTR;

typedef cmr_int (*cmr_set_flash)(cmr_u32 set_param, cmr_u32 opt,
                                 cmr_handle oem_handle);

/*  isp param for raw */

/*  isp param for raw  end */

enum sensor_internal_evt {
    SENSOR_ERROR = CMR_EVT_SENSOR_BASE,
    SENSOR_FOCUS_MOVE
};

enum sensor_evt {
    CMR_SENSOR_ERROR = CMR_EVT_SENSOR_BASE,
    CMR_SENSOR_FOCUS_MOVE
};

typedef enum {
    SENSOR_OP_SUCCESS = SENSOR_SUCCESS,
    SENSOR_OP_PARAM_ERR,
    SENSOR_OP_STATUS_ERR,
    SENSOR_OP_ERR,
    SENSOR_OP_MAX = 0xFFFF
} ERR_SENSOR_E;

typedef enum {
    SENSOR_TYPE_NONE = 0x00,
    SENSOR_TYPE_IMG_SENSOR,
    SENSOR_TYPE_ATV,
    SENSOR_TYPE_MAX
} SENSOR_TYPE_E;

typedef enum {
    SENSOR_HDR_EV_LEVE_0 = 0,
    SENSOR_HDR_EV_LEVE_1,
    SENSOR_HDR_EV_LEVE_2,
    SENSOR_HDR_EV_LEVE_MAX
} SESOR_HDR_EV_LEVEL_E;

struct ae_valid_fn {
    cmr_u8 group_hold_flag;
    cmr_u8 valid_expo_num;
    cmr_u8 valid_gain_num;
};

typedef enum {
    SENSOR_EXT_FOCUS_NONE = 0x00,
    SENSOR_EXT_FOCUS_TRIG,
    SENSOR_EXT_FOCUS_ZONE,
    SENSOR_EXT_FOCUS_MULTI_ZONE,
    SENSOR_EXT_FOCUS_MACRO,
    SENSOR_EXT_FOCUS_WIN,
    SENSOR_EXT_FOCUS_CAF,
    SENSOR_EXT_FOCUS_CHECK_AF_GAIN,
    SENSOR_EXT_FOCUS_MAX
} SENSOR_EXT_FOCUS_CMD_E;

typedef enum {
    SENSOR_EXT_EXPOSURE_NONE = 0x00,
    SENSOR_EXT_EXPOSURE_AUTO,
    SENSOR_EXT_EXPOSURE_ZONE,
    SENSOR_EXT_EXPOSURE_MAX
} SENSOR_EXT_EXPOSURE_CMD_E;

typedef enum {
    SENSOR_EXT_FUNC_NONE = 0x00,
    SENSOR_EXT_FUNC_INIT,
    SENSOR_EXT_FOCUS_START,
    SENSOR_EXT_FOCUS_QUIT,
    SENSOR_EXT_EXPOSURE_START,
    SENSOR_EXT_EV,
    SENSOR_EXT_EXPOSURE_SL, /*0 is save the value, 1 is load the value*/
    SENSOR_EXT_GAIN_OVER_THRS,
    SENSOR_EXT_SET_FPS,
    SENSOR_EXT_FUNC_MAX
} SENSOR_EXT_FUNC_CMD_E;

typedef enum {
    SENSOR_EXIF_CTRL_EXPOSURETIME = 0x00,
    SENSOR_EXIF_CTRL_FNUMBER,
    SENSOR_EXIF_CTRL_EXPOSUREPROGRAM,
    SENSOR_EXIF_CTRL_SPECTRALSENSITIVITY,
    SENSOR_EXIF_CTRL_ISOSPEEDRATINGS,
    SENSOR_EXIF_CTRL_OECF,
    SENSOR_EXIF_CTRL_SHUTTERSPEEDVALUE,
    SENSOR_EXIF_CTRL_APERTUREVALUE,
    SENSOR_EXIF_CTRL_BRIGHTNESSVALUE,
    SENSOR_EXIF_CTRL_EXPOSUREBIASVALUE,
    SENSOR_EXIF_CTRL_MAXAPERTUREVALUE,
    SENSOR_EXIF_CTRL_SUBJECTDISTANCE,
    SENSOR_EXIF_CTRL_METERINGMODE,
    SENSOR_EXIF_CTRL_LIGHTSOURCE,
    SENSOR_EXIF_CTRL_FLASH,
    SENSOR_EXIF_CTRL_FOCALLENGTH,
    SENSOR_EXIF_CTRL_SUBJECTAREA,
    SENSOR_EXIF_CTRL_FLASHENERGY,
    SENSOR_EXIF_CTRL_SPATIALFREQUENCYRESPONSE,
    SENSOR_EXIF_CTRL_FOCALPLANEXRESOLUTION,
    SENSOR_EXIF_CTRL_FOCALPLANEYRESOLUTION,
    SENSOR_EXIF_CTRL_FOCALPLANERESOLUTIONUNIT,
    SENSOR_EXIF_CTRL_SUBJECTLOCATION,
    SENSOR_EXIF_CTRL_EXPOSUREINDEX,
    SENSOR_EXIF_CTRL_SENSINGMETHOD,
    SENSOR_EXIF_CTRL_FILESOURCE,
    SENSOR_EXIF_CTRL_SCENETYPE,
    SENSOR_EXIF_CTRL_CFAPATTERN,
    SENSOR_EXIF_CTRL_CUSTOMRENDERED,
    SENSOR_EXIF_CTRL_EXPOSUREMODE,
    SENSOR_EXIF_CTRL_WHITEBALANCE,
    SENSOR_EXIF_CTRL_DIGITALZOOMRATIO,
    SENSOR_EXIF_CTRL_FOCALLENGTHIN35MMFILM,
    SENSOR_EXIF_CTRL_SCENECAPTURETYPE,
    SENSOR_EXIF_CTRL_GAINCONTROL,
    SENSOR_EXIF_CTRL_CONTRAST,
    SENSOR_EXIF_CTRL_SATURATION,
    SENSOR_EXIF_CTRL_SHARPNESS,
    SENSOR_EXIF_CTRL_DEVICESETTINGDESCRIPTION,
    SENSOR_EXIF_CTRL_SUBJECTDISTANCERANGE,
    SENSOR_EXIF_CTRL_MAX,
} SENSOR_EXIF_CTRL_E;

typedef enum {
    DCAMERA_ENVIRONMENT_NORMAL = 0x00,
    DCAMERA_ENVIRONMENT_NIGHT,
    DCAMERA_ENVIRONMENT_SUNNY,
    DCAMERA_ENVIRONMENT_SPORTS,
    DCAMERA_ENVIRONMENT_LANDSCAPE,
    DCAMERA_ENVIRONMENT_PORTRAIT,
    DCAMERA_ENVIRONMENT_PORTRAIT_NIGHT,
    DCAMERA_ENVIRONMENT_BACKLIGHT,
    DCAMERA_ENVIRONMENT_MACRO,
    DCAMERA_ENVIRONMENT_MANUAL = 30,
    DCAMERA_ENVIRONMENT_AUTO = 31,
    DCAMERA_ENVIRONMENT_MAX
} DCAMERA_PARAM_ENVIRONMENT_E;

typedef enum {
    DCAMERA_WB_MODE_AUTO = 0x00,
    DCAMERA_WB_MODE_INCANDESCENCE,
    DCAMERA_WB_MODE_U30,
    DCAMERA_WB_MODE_CWF,
    DCAMERA_WB_MODE_FLUORESCENT,
    DCAMERA_WB_MODE_SUN,
    DCAMERA_WB_MODE_CLOUD,
    DCAMERA_WB_MODE_MAX
} DCAMERA_PARAM_WB_MODE_E;

typedef enum {
    DCAMERA_EFFECT_NORMAL = 0x00,
    DCAMERA_EFFECT_BLACKWHITE,
    DCAMERA_EFFECT_RED,
    DCAMERA_EFFECT_GREEN,
    DCAMERA_EFFECT_BLUE,
    DCAMERA_EFFECT_YELLOW,
    DCAMERA_EFFECT_NEGATIVE,
    DCAMERA_EFFECT_CANVAS,
    DCAMERA_EFFECT_RELIEVOS,
    DCAMERA_EFFECT_MAX
} DCAMERA_PARAM_EFFECT_E;

typedef enum { SENSOR_OTP_TYPE, MODULE_OTP_TYPE, OTP_TYPE_MAX } CAMERA_OTP_TYPE;

struct otp_data_tag {
    cmr_u32 size;
    void *data_ptr;
    cmr_u16 width;
};

typedef struct sensor_otp_data {
    struct otp_data_tag awb;
    struct otp_data_tag lsc;
    struct otp_data_tag af;
    struct otp_data_tag golden;
} OTP_DATA_INFO_TAG;


typedef void* SENSOR_HW_HANDLE;
typedef cmr_int (*SENSOR_IOCTL_FUNC_PTR)(cmr_handle handle, cmr_uint param);

typedef struct sensor_ioctl_func_tab_tag {
    /*1: Internal IOCTL function */
    // SENSOR_HW_HANDLE (*create)(void *privatedata);    // malloc
    // SENSOR_HW_HANDLE,  privatedata == sensor_drv_ctx.
    // void (*destroy)(SENSOR_HW_HANDLE *handle);
    //
    cmr_uint (*reset)(SENSOR_HW_HANDLE handle, cmr_uint param);
    uint32_t (*power)(SENSOR_HW_HANDLE handle, uint32_t param);
    cmr_uint (*enter_sleep)(SENSOR_HW_HANDLE handle, cmr_uint param);
    uint32_t (*identify)(SENSOR_HW_HANDLE handle, uint32_t param);
    cmr_uint (*write_reg)(SENSOR_HW_HANDLE handle, cmr_uint param);
    cmr_uint (*read_reg)(SENSOR_HW_HANDLE handle, cmr_uint param);
    /*Custom function */
    cmr_uint (*cus_func_1)(SENSOR_HW_HANDLE handle, cmr_uint param);
    unsigned long (*get_trim)(SENSOR_HW_HANDLE handle, uint32_t param);
    /*External IOCTL function */
    cmr_uint (*ae_enable)(SENSOR_HW_HANDLE handle, cmr_uint param);
    cmr_uint (*hmirror_enable)(SENSOR_HW_HANDLE handle, cmr_uint param);
    cmr_uint (*vmirror_enable)(SENSOR_HW_HANDLE handle, cmr_uint param);

    cmr_uint (*set_brightness)(SENSOR_HW_HANDLE handle, cmr_uint param);
    cmr_uint (*set_contrast)(SENSOR_HW_HANDLE handle, cmr_uint param);
    cmr_uint (*set_sharpness)(SENSOR_HW_HANDLE handle, cmr_uint param);
    cmr_uint (*set_saturation)(SENSOR_HW_HANDLE handle, cmr_uint param);
    cmr_uint (*set_preview_mode)(SENSOR_HW_HANDLE handle, cmr_uint param);

    cmr_uint (*set_image_effect)(SENSOR_HW_HANDLE handle, cmr_uint param);
    // low 16bits is resolution table index,hight 16bits is cap mode containing
    // normal and HDR.
    uint32_t (*before_snapshort)(SENSOR_HW_HANDLE handle, uint32_t param);
    cmr_uint (*after_snapshort)(SENSOR_HW_HANDLE handle, cmr_uint param);
    cmr_uint (*flash)(SENSOR_HW_HANDLE handle, cmr_uint param);
    cmr_uint (*read_ae_value)(SENSOR_HW_HANDLE handle, cmr_uint param);
    cmr_uint (*write_ae_value)(SENSOR_HW_HANDLE handle, cmr_uint param);
    cmr_uint (*read_gain_value)(SENSOR_HW_HANDLE handle, cmr_uint param);
    uint32_t (*write_gain_value)(SENSOR_HW_HANDLE handle, uint32_t param);
    cmr_uint (*read_gain_scale)(SENSOR_HW_HANDLE handle, cmr_uint param);
    cmr_uint (*set_frame_rate)(SENSOR_HW_HANDLE handle, cmr_uint param);
    uint32_t (*af_enable)(SENSOR_HW_HANDLE handle, uint32_t param);
    cmr_uint (*af_get_status)(SENSOR_HW_HANDLE handle, cmr_uint param);
    cmr_uint (*set_wb_mode)(SENSOR_HW_HANDLE handle, cmr_uint param);
    cmr_uint (*get_skip_frame)(SENSOR_HW_HANDLE handle, cmr_uint param);
    cmr_uint (*set_iso)(SENSOR_HW_HANDLE handle, cmr_uint param);
    cmr_uint (*set_exposure_compensation)(SENSOR_HW_HANDLE handle,
                                          cmr_uint param);
    cmr_uint (*check_image_format_support)(SENSOR_HW_HANDLE handle,
                                           cmr_uint param);
    cmr_uint (*change_image_format)(SENSOR_HW_HANDLE handle, cmr_uint param);
    cmr_uint (*set_zoom)(SENSOR_HW_HANDLE handle, cmr_uint param);
    /*CUSTOMER FUNCTION */
    unsigned long (*get_exif)(SENSOR_HW_HANDLE handle, unsigned long param);
    uint32_t (*set_focus)(SENSOR_HW_HANDLE handle, unsigned long param);
    cmr_uint (*set_anti_banding_flicker)(SENSOR_HW_HANDLE handle,
                                         cmr_uint param);
    cmr_uint (*set_video_mode)(SENSOR_HW_HANDLE handle, cmr_uint param);
    cmr_uint (*pick_jpeg_stream)(SENSOR_HW_HANDLE handle, cmr_uint param);
    cmr_uint (*set_meter_mode)(SENSOR_HW_HANDLE handle, cmr_uint param);
    cmr_uint (*get_status)(SENSOR_HW_HANDLE handle, cmr_uint param);
    uint32_t (*stream_on)(SENSOR_HW_HANDLE handle, uint32_t param);
    uint32_t (*stream_off)(SENSOR_HW_HANDLE handle, uint32_t param);
    unsigned long (*cfg_otp)(SENSOR_HW_HANDLE handle, unsigned long param);
    uint32_t (*ex_write_exp)(SENSOR_HW_HANDLE handle, unsigned long param);
    cmr_uint (*set_shutter_gain_delay_info)(void *);
    cmr_uint (*read_aec_info)(SENSOR_HW_HANDLE handle, cmr_uint param);
    // af control and DVT test funcs valid only af_enable works
    cmr_uint (*set_pos)(SENSOR_HW_HANDLE handle, uint16_t pos);
    cmr_uint (*get_otp)(SENSOR_HW_HANDLE handle, uint16_t *inf,
                        uint16_t *macro);
    cmr_uint (*get_motor_pos)(SENSOR_HW_HANDLE handle, uint16_t *pos);
    cmr_uint (*set_motor_bestmode)(SENSOR_HW_HANDLE handle);
    cmr_uint (*get_test_vcm_mode)(SENSOR_HW_HANDLE handle);
    cmr_uint (*set_test_vcm_mode)(SENSOR_HW_HANDLE handle, char *vcm_mode);
} SENSOR_IOCTL_FUNC_TAB_T, *SENSOR_IOCTL_FUNC_TAB_T_PTR;

struct raw_param_info_tab {
    cmr_u32 param_id;
    struct sensor_raw_info *info_ptr;
    cmr_u32 (*identify_otp)(SENSOR_HW_HANDLE handle, void *param_ptr);
    cmr_u32 (*cfg_otp)(SENSOR_HW_HANDLE handle, void *param_ptr);
};

#if 0
typedef struct sensor_trim_tag {
    cmr_u16 trim_start_x;
    cmr_u16 trim_start_y;
    cmr_u16 trim_width;
    cmr_u16 trim_height;
    cmr_u32 line_time;
    cmr_u32 bps_per_lane; /* bps_per_lane = 2 * mipi_clock */
    cmr_u32 frame_line;
    SENSOR_RECT_T scaler_trim;
} SENSOR_TRIM_T, *SENSOR_TRIM_T_PTR;
#endif
typedef struct _sensor_ext_fun_tag {
    cmr_u32 cmd;
    cmr_u32 param;
    SENSOR_RECT_T zone;
} SENSOR_EXT_FUN_T, *SENSOR_EXT_FUN_T_PTR;
typedef struct _sensor_ext_fun_param_tag {
    cmr_u8 cmd;
    cmr_u8 param;
    cmr_u8 param2;
    cmr_u16 zone_cnt;
    SENSOR_RECT_T zone[FOCUS_ZONE_CNT_MAX];
    // cmr_u8 is_need_focus_move; //out pram, for caf
} SENSOR_EXT_FUN_PARAM_T, *SENSOR_EXT_FUN_PARAM_T_PTR;

struct sensor_gain_thrs_tag {
    cmr_u8 cmd;
    cmr_u8 param;
    cmr_u32 gain_thrs;
};

typedef struct _sensor_val_tag {
    uint8_t type;
    void *pval;
} SENSOR_VAL_T, *SENSOR_VAL_T_PTR;

typedef struct sensor_mode_info_tag {
    enum sensor_mode mode;
    cmr_u16 width;  // width before trim,maybe after binning?
    cmr_u16 height; // height before trim,maybe after binning?
    cmr_u16 trim_start_x;
    cmr_u16 trim_start_y;
    cmr_u16 trim_width;
    cmr_u16 trim_height;
    cmr_u32 line_time;
    SENSOR_IMAGE_FORMAT image_format;
    cmr_u32 bps_per_lane;
    cmr_u32 frame_line;
    SENSOR_RECT_T scaler_trim;
    cmr_u16 out_width;  // sensor output width after binning and crop/trim
    cmr_u16 out_height; // sensor output height after binning and crop/trim
} SENSOR_MODE_INFO_T, *SENSOR_MODE_INFO_T_PTR;

typedef struct sensor_register_tag {
    cmr_u32 img_sensor_num;
    cmr_u8 cur_id;
    cmr_u8 is_register[SENSOR_ID_MAX];
} SENSOR_REGISTER_INFO_T, *SENSOR_REGISTER_INFO_T_PTR;

typedef struct sensor_exp_info_tag {
    SENSOR_IMAGE_FORMAT image_format;
    cmr_u32 image_pattern;
    cmr_u8 pclk_polarity;
    cmr_u8 vsync_polarity;
    cmr_u8 hsync_polarity;
    cmr_u8 pclk_delay;
    cmr_u16 source_width_max;
    cmr_u16 source_height_max;
    cmr_u32 environment_mode;
    cmr_u32 image_effect;
    cmr_u32 wb_mode;
    cmr_u32 step_count; /*bit[0:7]: count of step in brightness, contrast,
                           sharpness, saturation
                           bit[8:15] count of step in ISO
                           bit[16:23] count of step in exposure compensation
                           bit[24:31] reseved */
    SENSOR_MODE_INFO_T sensor_mode_info[SENSOR_MODE_MAX];
    struct sensor_ic_ops *sns_ops;
    struct sensor_raw_info *raw_info_ptr;
    SENSOR_EXTEND_INFO_T_PTR ext_info_ptr;
    cmr_u32 preview_skip_num;
    cmr_u32 capture_skip_num;
    cmr_u32 flash_capture_skip_num;
    cmr_u32 mipi_cap_skip_num;
    cmr_u32 preview_deci_num;
    cmr_u32 video_preview_deci_num;
    cmr_u16 threshold_eb;
    cmr_u16 threshold_mode;
    cmr_u16 threshold_start;
    cmr_u16 threshold_end;
    SENSOR_INF_T sensor_interface;
    const cmr_s8 *name;
    SENSOR_VIDEO_INFO_T sensor_video_info[SENSOR_MODE_MAX];
    cmr_u32 change_setting_skip_num;
    cmr_u32 sensor_image_type;
    cmr_u16 horizontal_view_angle;
    cmr_u16 vertical_view_angle;
    const cmr_s8 *sensor_version_info;
} SENSOR_EXP_INFO_T, *SENSOR_EXP_INFO_T_PTR;

#if 0
typedef struct sensor_static_info_tag {
    cmr_u32 f_num;        // f-number,focal ratio,actual f-number*100
    cmr_u32 focal_length; // actual focal_length*100
    cmr_u32 max_fps;      // max fps of sensor's all settings
    cmr_u32 max_adgain;   // AD-gain
    cmr_u32 ois_supported;
    cmr_u32 pdaf_supported;
    cmr_u32 exp_valid_frame_num;
    cmr_u32 clamp_level; // black level
    cmr_u32 adgain_valid_frame_num;
} SENSOR_STATIC_INFO_T;

typedef struct sensor_mode_fps_tag {
    enum sensor_mode mode;
    cmr_u32 max_fps;           // max fps in current sensor
                               // mode,10*1000000/(line_time*frame_line)
    cmr_u32 min_fps;           // min fps, we set it to 1.
    cmr_u32 is_high_fps;       // if max_fps > 30,then is high fps.
    cmr_u32 high_fps_skip_num; // max_fps/30
} SENSOR_MODE_FPS_T, *SENSOR_MODE_FPS_T_PTR;

typedef struct sensor_mode_fps_info_tag {
    cmr_u32 is_init;
    SENSOR_MODE_FPS_T sensor_mode_fps[SENSOR_MODE_MAX];
} SENSOR_MODE_FPS_INFO_T;

typedef struct sensor_info_tag {
    cmr_u8 salve_i2c_addr_w;
    cmr_u8 salve_i2c_addr_r;
    cmr_u8 reg_addr_value_bits;
    cmr_u8 hw_signal_polarity;
    cmr_u32 environment_mode;
    cmr_u32 image_effect;
    cmr_u32 wb_mode;
    cmr_u32 step_count;
    cmr_u16 reset_pulse_level;
    cmr_u16 reset_pulse_width;
    cmr_u32 power_down_level;
    cmr_u32 identify_count;
    SENSOR_REG_T identify_code[SENSOR_IDENTIFY_CODE_COUNT];
    SENSOR_AVDD_VAL_E avdd_val;
    cmr_u16 source_width_max;
    cmr_u16 source_height_max;
    const cmr_s8 *name;
    SENSOR_IMAGE_FORMAT image_format;
    cmr_u32 image_pattern;
    SENSOR_REG_TAB_INFO_T_PTR resolution_tab_info_ptr;
    SENSOR_IOCTL_FUNC_TAB_T_PTR ioctl_func_tab_ptr;
    struct sensor_raw_info **raw_info_ptr; /*sensor_raw_info*/
    SENSOR_EXTEND_INFO_T_PTR ext_info_ptr;
    SENSOR_AVDD_VAL_E iovdd_val;
    SENSOR_AVDD_VAL_E dvdd_val;
    cmr_u32 preview_skip_num;
    cmr_u32 capture_skip_num;
    cmr_u32 preview_deci_num;
    cmr_u32 flash_capture_skip_num;
    cmr_u32 mipi_cap_skip_num;
    cmr_u32 video_preview_deci_num;
    cmr_u16 threshold_eb;
    cmr_u16 threshold_mode;
    cmr_u16 threshold_start;
    cmr_u16 threshold_end;
    cmr_u8 i2c_dev_handler;
    SENSOR_INF_T sensor_interface;
    SENSOR_VIDEO_INFO_T_PTR video_tab_info_ptr;
    cmr_u32 change_setting_skip_num;
    cmr_u16 horizontal_view_angle;
    cmr_u16 vertical_view_angle;
    const cmr_s8 *sensor_version_info;
    cmr_u8 focus_eb;
} SENSOR_INFO_T;
#endif

typedef enum {
    SENSOR_PARAM_WB_MODE_AUTO = 0x00,
    SENSOR_PARAM_WB_MODE_INCANDESCENCE,
    SENSOR_PARAM_WB_MODE_U30,
    SENSOR_PARAM_WB_MODE_CWF,
    SENSOR_PARAM_WB_MODE_FLUORESCENT,
    SENSOR_PARAM_WB_MODE_SUN,
    SENSOR_PARAM_WB_MODE_CLOUD,
    SENSOR_PARAM_WB_MODE_MAX
} SENSOR_PARAM_WB_MODE_E;

typedef enum {
    SENSOR_PARAM_ENVIRONMENT_NORMAL = 0x00,
    SENSOR_PARAM_ENVIRONMENT_NIGHT,
    SENSOR_PARAM_ENVIRONMENT_SUNNY,
    SENSOR_PARAM_ENVIRONMENT_SPORTS,
    SENSOR_PARAM_ENVIRONMENT_LANDSCAPE,
    SENSOR_PARAM_ENVIRONMENT_PORTRAIT,
    SENSOR_PARAM_ENVIRONMENT_PORTRAIT_NIGHT,
    SENSOR_PARAM_ENVIRONMENT_BACKLIGHT,
    SENSOR_PARAM_ENVIRONMENT_MACRO,
    SENSOR_PARAM_ENVIRONMENT_MANUAL = 30,
    SENSOR_PARAM_ENVIRONMENT_AUTO = 31,
    SENSOR_PARAM_ENVIRONMENT_MAX
} SENSOR_PARAM_ENVIRONMENT_E;

typedef enum {
    SENSOR_PARAM_EFFECT_NORMAL = 0x00,
    SENSOR_PARAM_EFFECT_BLACKWHITE,
    SENSOR_PARAM_EFFECT_RED,
    SENSOR_PARAM_EFFECT_GREEN,
    SENSOR_PARAM_EFFECT_BLUE,
    SENSOR_PARAM_EFFECT_YELLOW,
    SENSOR_PARAM_EFFECT_NEGATIVE,
    SENSOR_PARAM_EFFECT_CANVAS,
    SENSOR_PARAM_EFFECT_RELIEVOS,
    SENSOR_PARAM_EFFECT_MAX
} SENSOR_PARAM_EFFECT_E;

#define SENSOR_ONE_I2C 1
#define SENSOR_ZERO_I2C 0
#define SENSOR_16_BITS_I2C 2
#define SENSOR_CHECK_STATUS_INTERVAL 5

#define SENSOR_FOCUS_MOVE_INTERVAL 90

#define SENSOR_LOW_SIXTEEN_BIT 0xffff

#ifndef SCI_TRUE
#define SCI_TRUE 1
#define SCI_FALSE 0
#endif

#define SIGN_0 0x73
#define SIGN_1 0x69
#define SIGN_2 0x67
#define SIGN_3 0x6e

#define SENSOR_DRV_CHECK_ZERO(a)                                               \
    do {                                                                       \
        if (PNULL == a) {                                                      \
            SENSOR_LOGE("Sensor_driver_u, zero pointer \n");                   \
            return SENSOR_CTX_ERROR;                                           \
        }                                                                      \
    } while (0)

#define SENSOR_DRV_CHECK_ZERO_VOID(a)                                          \
    do {                                                                       \
        if (PNULL == a) {                                                      \
            SENSOR_LOGE("Sensor_driver_u, zero pointer \n");                   \
            return;                                                            \
        }                                                                      \
    } while (0)

#define SENSOR_DRV_CHECK_PTR(a)                                                \
    do {                                                                       \
        if (PNULL == a) {                                                      \
            SENSOR_LOGE("Sensor_driver_u, null pointer \n");                   \
            return SENSOR_FAIL;                                                \
        }                                                                      \
    } while (0)

#define SENSOR_PARAM_NUM 8
#ifdef CONFIG_USE_CAMERASERVER_PROC
#define SENSOR_PARA "/data/misc/cameraserver/sensor.file"
#else
#define SENSOR_PARA "/data/misc/media/sensor.file"
#endif

enum SENSOR_EVT_TYPE {
    SENSOR_EVT_INIT = CMR_EVT_SENSOR_BASE,
    SENSOR_EVT_SET_MODE,
    SENSOR_EVT_STREAM_ON,
    SENSOR_EVT_STREAM_OFF,
    SENSOR_EVT_AF_INIT,
    SENSOR_EVT_DEINIT,
    SENSOR_EVT_SET_MODE_DONE,
    SENSOR_EVT_CFG_OTP
};

#define CALI_FILE_DIR "/data/"

struct sns_thread_cxt {
    cmr_uint is_inited;
    cmr_handle thread_handle;
    sem_t sensor_sync_sem;
    pthread_mutex_t sensor_mutex;
};

struct sensor_drv_context {
    BOOLEAN sensor_isInit;
    BOOLEAN sensor_identified;
    BOOLEAN sensor_param_saved;
    BOOLEAN reserverd1;
    cmr_u8 sensor_index[SENSOR_ID_MAX];
    cmr_u16 i2c_addr;
    cmr_int fd_sensor; /*sensor device id, used when sensor dev alive*/
    cmr_u32 is_calibration;
    cmr_u32 stream_on;
    SENSOR_INFO_T *sensor_list_ptr[SENSOR_ID_MAX];
    SENSOR_INFO_T *sensor_info_ptr;
    SENSOR_EXP_INFO_T sensor_exp_info; /*!!!BE CAREFUL!!! for the 3rd party
                                          issue, the SENSOR_EXP_INFO_T must
                                          equal the sensor_exp_info*/
    SENSOR_TYPE_E sensor_type;
    enum sensor_mode sensor_mode[SENSOR_ID_MAX];
    SENSOR_REGISTER_INFO_T sensor_register_info;
    cmr_u32 flash_mode;
    cmr_u32 padding;
    cmr_uint is_autotest;
    cmr_int is_main_sensor;
    cmr_int is_main2_sensor;
    cmr_int is_register_sensor;
    EXIF_SPEC_PIC_TAKING_COND_T default_exif;
    struct sns_thread_cxt ctrl_thread_cxt;
    cmr_u32 exit_flag;
    cmr_u32 error_cnt;
    cmr_uint lnc_addr_bakup[8][4];
    cmr_u32 bypass_mode;
    struct drv_fov_info fov_info;

    cmr_handle sensor_hw_handler;
    void *module_list[SENSOR_ID_MAX];
    void *current_module;
    cmr_handle hw_drv_handle;
    cmr_handle otp_drv_handle;
    cmr_handle af_drv_handle;
    cmr_handle sns_ic_drv_handle;
};

#define CMR_SENSOR_DEV_NAME "/dev/sprd_sensor"
cmr_int sensor_stream_on(struct sensor_drv_context *sensor_cxt);
/*common functions for OEM*/
cmr_int sensor_open_common(struct sensor_drv_context *sensor_cxt,
                           cmr_u32 sensor_id, cmr_uint is_autotest);

cmr_int sensor_close_common(struct sensor_drv_context *sensor_cxt,
                            cmr_u32 sensor_id);

void sensor_set_cxt_common(struct sensor_drv_context *sensor_cxt);

cmr_int sensor_set_mode_common(cmr_handle sns_module_handle, cmr_u32 mode);

cmr_int sensor_set_modone_common(struct sensor_drv_context *sensor_cxt);

cmr_int sensor_get_mode_common(cmr_handle sns_module_handle, cmr_u32 *mode);

cmr_int sensor_update_isparm_from_file(struct sensor_drv_context *sensor_cxt,
                                       cmr_u32 sensor_id);

cmr_int sensor_is_init_common(struct sensor_drv_context *sensor_cxt);

cmr_int sensor_stream_ctrl_common(struct sensor_drv_context *sensor_cxt,
                                  cmr_u32 on_off);

cmr_int sensor_set_exif_common(cmr_handle sns_module_handle, cmr_u32 cmdin,
                               cmr_u32 param);

cmr_int sensor_get_exif_common(cmr_handle sns_module_handle, void **param);

cmr_int sensor_get_info_common(struct sensor_drv_context *sensor_cxt,
                               SENSOR_EXP_INFO_T **sensor_exp_info_pptr);

cmr_int sns_dev_get_flash_level(struct sensor_drv_context *sensor_cxt,
                                struct sensor_flash_level *level);

cmr_int sensor_get_flash_level(struct sensor_drv_context *sensor_cxt,
                               struct sensor_flash_level *level);

cmr_int sensor_set_mode_done_common(cmr_handle sns_module_handle);

cmr_int sensor_drv_ioctl(cmr_handle sns_module_handle, enum sns_cmd cmd,
                         void *param);

cmr_int hw_Sensor_GetMode(cmr_handle handle, cmr_u32 *mode);
#define Sensor_GetMode(mode) hw_Sensor_GetMode(handle, mode)

cmr_int hw_Sensor_SetMode_WaitDone(cmr_handle handle);
#define Sensor_SetMode_WaitDone(void) hw_Sensor_SetMode_WaitDone(handle)

cmr_int hw_Sensor_SetSensorExifInfo(cmr_handle handle, SENSOR_EXIF_CTRL_E cmd,
                                    cmr_u32 param);
#define Sensor_SetSensorExifInfo(cmd, param)                                   \
    hw_Sensor_SetSensorExifInfo(handle, cmd, param)

EXIF_SPEC_PIC_TAKING_COND_T *hw_Sensor_GetSensorExifInfo(cmr_handle handle);
#define Sensor_GetSensorExifInfo(void) hw_Sensor_GetSensorExifInfo(handle)

cmr_int hw_Sensor_SetMode(cmr_handle handle, cmr_u32 mode);
#define Sensor_SetMode(mode) hw_Sensor_SetMode(handle, mode)

SENSOR_EXP_INFO_T *Sensor_GetInfo(void);
SENSOR_EXP_INFO_T *Sensor_GetInfo_withid(cmr_u32 id);

cmr_int hw_Sensor_SetFlash(SENSOR_HW_HANDLE handle, uint32_t is_open);
#define Sensor_SetFlash(is_open) hw_Sensor_SetFlash(handle, is_open)

// utest refer
#ifdef __cplusplus
}
#endif

#endif
