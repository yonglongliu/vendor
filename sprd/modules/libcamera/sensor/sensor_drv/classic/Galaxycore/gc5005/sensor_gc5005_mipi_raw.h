/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * V4.0
 */
#define LOG_TAG "sensor_gc5005"

#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"

#if defined(CONFIG_CAMERA_ISP_VERSION_V3) ||                                   \
    defined(CONFIG_CAMERA_ISP_VERSION_V4)
#include "parameters/sensor_gc5005_raw_param_main.c"
#else
#include "parameters/sensor_gc5005_raw_param.c"
#endif

//#define FEATURE_OTP    /*OTP function switch*/
/*this is very important*/
#define VENDOR_NUM 1

#define SENSOR_NAME "gc5005"
#define I2C_SLAVE_ADDR 0x6e /* 8bit slave address*/

#define GC5005_PID_ADDR 0xf0
#define GC5005_PID_VALUE 0x50
#define GC5005_VER_ADDR 0xf1
#define GC5005_VER_VALUE 0x05

/* sensor parameters begin */
/* effective sensor output image size */
#define SNAPSHOT_WIDTH 2592
#define SNAPSHOT_HEIGHT 1944
#define PREVIEW_WIDTH 2592
#define PREVIEW_HEIGHT 1944

/*Mipi output*/
#define LANE_NUM 2
#define RAW_BITS 10

#define SNAPSHOT_MIPI_PER_LANE_BPS 864
#define PREVIEW_MIPI_PER_LANE_BPS 864

/*line time unit: 1ns*/
#define SNAPSHOT_LINE_TIME 16778
#define PREVIEW_LINE_TIME 16778

/* frame length*/
#define SNAPSHOT_FRAME_LENGTH 1984
#define PREVIEW_FRAME_LENGTH 1984

/* please ref your spec */
#define FRAME_OFFSET 0
#define SENSOR_MAX_GAIN 0x180 // 6x
#define SENSOR_BASE_GAIN 0x40
#define SENSOR_MIN_SHUTTER 4

/* please ref your spec
 * 1 : average binning
 * 2 : sum-average binning
 * 4 : sum binning
 */
#define BINNING_FACTOR 1

/* please ref spec
 * 1: sensor auto caculate
 * 0: driver caculate
 */
#define SUPPORT_AUTO_FRAME_LENGTH 0
/* sensor parameters end */

/* isp parameters, please don't change it*/
#if defined(CONFIG_CAMERA_ISP_VERSION_V3) ||                                   \
    defined(CONFIG_CAMERA_ISP_VERSION_V4)
#define ISP_BASE_GAIN 0x80
#else
#define ISP_BASE_GAIN 0x10
#endif
/* please don't change it */
#define EX_MCLK 24

#define GC5005_RAW_PARAM_COM 0x0000

//#define IMAGE_NORMAL_MIRROR
//#define IMAGE_H_MIRROR
#define IMAGE_V_MIRROR
//#define IMAGE_HV_MIRROR

#ifdef IMAGE_NORMAL_MIRROR
#define MIRROR 0x54
#define STARTY 0x02
#define STARTX 0x02
#endif

#ifdef IMAGE_H_MIRROR
#define MIRROR 0x55
#define STARTY 0x02
#define STARTX 0x03
#endif

#ifdef IMAGE_V_MIRROR
#define MIRROR 0x56
#define STARTY 0x01
#define STARTX 0x02
#endif

#ifdef IMAGE_HV_MIRROR
#define MIRROR 0x57
#define STARTY 0x01
#define STARTX 0x03
#endif

#if 0
/*==============================================================================
 * Description:
 * global variable
 *============================================================================*/
static struct hdr_info_t s_hdr_info;
static uint32_t s_current_default_frame_length;
struct sensor_ev_info_t s_sensor_ev_info;
#endif

#ifdef FEATURE_OTP

#include "parameters/sensor_gc5005_gcore_otp.c"

struct raw_param_info_tab s_gc5005_raw_param_tab[] = {
    {GC5005_RAW_PARAM_COM, &s_gc5005_mipi_raw_info, gc5005_gcore_identify_otp,
     gc5005_gcore_update_otp},
    {RAW_INFO_END_ID, PNULL, PNULL, PNULL}};
#else
struct raw_param_info_tab s_gc5005_raw_param_tab[] = {
    {GC5005_RAW_PARAM_COM, &s_gc5005_mipi_raw_info, PNULL, PNULL},
    {RAW_INFO_END_ID, PNULL, PNULL, PNULL}};

#endif

static struct sensor_ic_ops s_gc5005_ops_tab;
struct sensor_raw_info *s_gc5005_mipi_raw_info_ptr = &s_gc5005_mipi_raw_info;

static const SENSOR_REG_T gc5005_init_setting[] = {
    /*SYS*/
    {0xfe, 0x00},
    {0xfe, 0x00},
    {0xfe, 0x00},
    {0xf7, 0x01},
    {0xf8, 0x11},
    {0xf9, 0xaa},
    {0xfa, 0x84},
    {0xfc, 0x8a},
    {0xfe, 0x03},
    {0x10, 0x01},
    {0xfc, 0x8e},
    {0xfe, 0x00},
    {0xfe, 0x00},
    {0xfe, 0x00},
    {0x88, 0x03},
    {0xe7, 0xc0},

    /*ANALOG & CISCTL*/
    {0xfe, 0x00},
    {0x03, 0x06},
    {0x04, 0xfc},
    {0x05, 0x01},
    {0x06, 0xc5},
    {0x07, 0x00},
    {0x08, 0x10},
    {0x09, 0x00},
    {0x0a, 0x14},
    {0x0b, 0x00},
    {0x0c, 0x10},
    {0x0d, 0x07},
    {0x0e, 0xa0},
    {0x0f, 0x0a},
    {0x10, 0x30},
    {0x17, MIRROR}, // Don't Change Here!!!
    {0x18, 0x02},
    {0x19, 0x0a},
    {0x1a, 0x1b},
    {0x1c, 0x0c},
    {0x1d, 0x19},
    {0x21, 0x16},
    {0x24, 0xb0},
    {0x25, 0xc1},
    {0x27, 0x64},
    {0x29, 0x28},
    {0x2a, 0xc3},
    {0x31, 0x40},
    {0x32, 0xf8},
    {0xcd, 0xca},
    {0xce, 0xff},
    {0xcf, 0x70},
    {0xd0, 0xd2},
    {0xd1, 0xa0},
    {0xd3, 0x23},
    {0xd8, 0x12},
    {0xdc, 0xb3},
    {0xe1, 0x1b},
    {0xe2, 0x00},
    {0xe4, 0x78},
    {0xe6, 0x1f},
    {0xe7, 0xc0},
    {0xe8, 0x01},
    {0xe9, 0x02},
    {0xec, 0x01},
    {0xed, 0x02},

    /*ISP*/
    {0x80, 0x50}, // DD Enable
    {0x90, 0x01},
    {0x92, STARTY}, // Don't Change Here!!!
    {0x94, STARTX}, // Don't Change Here!!!
    {0x95, 0x07},
    {0x96, 0x98},
    {0x97, 0x0a},
    {0x98, 0x20},

    /*Gain*/
    {0x99, 0x00},
    {0x9a, 0x08},
    {0x9b, 0x10},
    {0x9c, 0x18},
    {0x9d, 0x19},
    {0x9e, 0x1a},
    {0x9f, 0x1b},
    {0xa0, 0x1c},
    {0xb0, 0x50},
    {0xb1, 0x01},
    {0xb2, 0x00},
    {0xb6, 0x00},

    /*DD*/
    {0xfe, 0x01},
    {0xc2, 0x02},
    {0xc3, 0xe0},
    {0xc4, 0xd9},
    {0xc5, 0x00},
    {0xfe, 0x00},

    /*BLK*/
    {0x40, 0x22},
    {0x4e, 0x3c},
    {0x4f, 0x3c},
    {0x60, 0x00},
    {0x61, 0x80},
    {0xab, 0x00},
    {0xac, 0x30},

    /*Dark Sun*/
    {0x68, 0xf4},
    {0x6a, 0x00},
    {0x6b, 0x00},
    {0x6c, 0x50},
    {0x6e, 0xc9},

    /*MIPI*/
    {0xfe, 0x03},
    {0x01, 0x07},
    {0x02, 0x33},
    {0x03, 0x93},
    {0x04, 0x04},
    {0x05, 0x00},
    {0x06, 0x00},
    {0x11, 0x2b},
    {0x12, 0xa8},
    {0x13, 0x0c},
    {0x15, 0x00},
    {0x18, 0x01},
    {0x1b, 0x14},
    {0x1c, 0x14},
    {0x21, 0x10},
    {0x22, 0x05},
    {0x23, 0x30},
    {0x24, 0x02},
    {0x25, 0x16},
    {0x26, 0x09},
    {0x29, 0x06},
    {0x2a, 0x0d},
    {0x2b, 0x09},
    {0xfe, 0x00},
};

static const SENSOR_REG_T gc5005_preview_setting[] = {};

static const SENSOR_REG_T gc5005_snapshot_setting[] = {};

static struct sensor_res_tab_info s_gc5005_resolution_tab_raw[VENDOR_NUM] = {
    {
      .module_id = MODULE_SUNNY,
      .reg_tab = {
        {ADDR_AND_LEN_OF_ARRAY(gc5005_init_setting), PNULL, 0,
        .width = 0, .height = 0,
        .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW},

        {ADDR_AND_LEN_OF_ARRAY(gc5005_preview_setting), PNULL, 0,
        .width = PREVIEW_WIDTH,.height = PREVIEW_HEIGHT,
        .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW},

        {ADDR_AND_LEN_OF_ARRAY(gc5005_snapshot_setting), PNULL, 0,
        .width = SNAPSHOT_WIDTH, .height = SNAPSHOT_HEIGHT,
        .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW}}
    }
/*If there are multiple modules,please add here*/
};

static SENSOR_TRIM_T s_gc5005_resolution_trim_tab[VENDOR_NUM] = {
    {
     .module_id = MODULE_SUNNY,
     .trim_info = {
        {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
        {
         .trim_start_x = 0,  .trim_start_y = 0,
         .trim_width = PREVIEW_WIDTH, .trim_height = PREVIEW_HEIGHT,
         .line_time = PREVIEW_LINE_TIME, .bps_per_lane = PREVIEW_MIPI_PER_LANE_BPS,
         .frame_line = PREVIEW_FRAME_LENGTH,
         .scaler_trim = { .x = 0, .y = 0, .w = PREVIEW_WIDTH, .h = PREVIEW_HEIGHT}},
        {
         .trim_start_x = 0,.trim_start_y = 0,
         .trim_width = SNAPSHOT_WIDTH,.trim_height = SNAPSHOT_HEIGHT,
         .line_time = SNAPSHOT_LINE_TIME,.bps_per_lane = SNAPSHOT_MIPI_PER_LANE_BPS,
         .frame_line = SNAPSHOT_FRAME_LENGTH,
         .scaler_trim = { .x = 0, .y = 0, .w = SNAPSHOT_WIDTH, .h = SNAPSHOT_HEIGHT}},
        }}

    /*If there are multiple modules,please add here*/
};

static const SENSOR_REG_T
    s_gc5005_preview_size_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
        /*video mode 0: ?fps */
        {{0xffff, 0xff}},
        /* video mode 1:?fps */
        {{0xffff, 0xff}},
        /* video mode 2:?fps */
        {{0xffff, 0xff}},
        /* video mode 3:?fps */
        {{0xffff, 0xff}}};

static const SENSOR_REG_T
    s_gc5005_capture_size_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
        /*video mode 0: ?fps */
        {{0xffff, 0xff}},
        /* video mode 1:?fps */
        {{0xffff, 0xff}},
        /* video mode 2:?fps */
        {{0xffff, 0xff}},
        /* video mode 3:?fps */
        {{0xffff, 0xff}}};

static SENSOR_VIDEO_INFO_T s_gc5005_video_info[SENSOR_MODE_MAX] = {
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{30, 30, 270, 90}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
     (SENSOR_REG_T **)s_gc5005_preview_size_video_tab},
    {{{2, 5, 338, 1000}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
     (SENSOR_REG_T **)s_gc5005_capture_size_video_tab},
};

static SENSOR_STATIC_INFO_T s_gc5005_static_info[VENDOR_NUM] = {
    {
     .module_id = MODULE_SUNNY,
     .static_info = {
         .f_num = 220,
         .focal_length = 346,
         .max_fps = 0,
         .max_adgain = 6,
         .ois_supported = 0,
         .pdaf_supported = 0,
         .exp_valid_frame_num = 1,
         .clamp_level = 0,
         .adgain_valid_frame_num = 1,
         .fov_info = {{4.614f, 3.444f}, 4.222f}}
    }
    /*If there are multiple modules,please add here*/
};

static SENSOR_MODE_FPS_INFO_T s_gc5005_mode_fps_info[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
       {.is_init = 0,
         {{SENSOR_MODE_COMMON_INIT, 0, 1, 0, 0},
         {SENSOR_MODE_PREVIEW_ONE, 0, 1, 0, 0},
         {SENSOR_MODE_SNAPSHOT_ONE_FIRST, 0, 1, 0, 0},
         {SENSOR_MODE_SNAPSHOT_ONE_SECOND, 0, 1, 0, 0},
         {SENSOR_MODE_SNAPSHOT_ONE_THIRD, 0, 1, 0, 0},
         {SENSOR_MODE_PREVIEW_TWO, 0, 1, 0, 0},
         {SENSOR_MODE_SNAPSHOT_TWO_FIRST, 0, 1, 0, 0},
         {SENSOR_MODE_SNAPSHOT_TWO_SECOND, 0, 1, 0, 0},
         {SENSOR_MODE_SNAPSHOT_TWO_THIRD, 0, 1, 0, 0}}}
    }
    /*If there are multiple modules,please add here*/
};

static struct sensor_module_info s_gc5005_module_info_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .module_info = {
         .major_i2c_addr = I2C_SLAVE_ADDR >> 1,
         .minor_i2c_addr = I2C_SLAVE_ADDR >> 1,

         .reg_addr_value_bits = SENSOR_I2C_REG_8BIT | SENSOR_I2C_VAL_8BIT |
                                    SENSOR_I2C_FREQ_400,

         .avdd_val = SENSOR_AVDD_2800MV,
         .iovdd_val = SENSOR_AVDD_1800MV,
         .dvdd_val = SENSOR_AVDD_1200MV,

         .image_pattern = SENSOR_IMAGE_PATTERN_RAWRGB_R,

         .preview_skip_num = 1,
         .capture_skip_num = 1,
         .flash_capture_skip_num = 6,
         .mipi_cap_skip_num = 0,
         .preview_deci_num = 0,
         .video_preview_deci_num = 0,

         .sensor_interface = {
              .type = SENSOR_INTERFACE_TYPE_CSI2,
              .bus_width = LANE_NUM,
              .pixel_width = RAW_BITS,
              .is_loose = 0,
          },

         .change_setting_skip_num = 1,
         .horizontal_view_angle = 65,
         .vertical_view_angle = 60
      }
    }

/*If there are multiple modules,please add here*/
};

/*==============================================================================
 * Description:
 * sensor all info
 * please modify this variable acording your spec
 *============================================================================*/
SENSOR_INFO_T g_gc5005_mipi_raw_info = {
    .hw_signal_polarity = SENSOR_HW_SIGNAL_PCLK_P | SENSOR_HW_SIGNAL_VSYNC_P |
                          SENSOR_HW_SIGNAL_HSYNC_P,
    .environment_mode = SENSOR_ENVIROMENT_NORMAL | SENSOR_ENVIROMENT_NIGHT,
    .image_effect = SENSOR_IMAGE_EFFECT_NORMAL | SENSOR_IMAGE_EFFECT_BLACKWHITE |
                    SENSOR_IMAGE_EFFECT_RED | SENSOR_IMAGE_EFFECT_GREEN |
                    SENSOR_IMAGE_EFFECT_BLUE | SENSOR_IMAGE_EFFECT_YELLOW |
                    SENSOR_IMAGE_EFFECT_NEGATIVE | SENSOR_IMAGE_EFFECT_CANVAS,

    .wb_mode = 0,
    .step_count = 7,
    .reset_pulse_level = SENSOR_LOW_PULSE_RESET,
    .reset_pulse_width = 50,
    .power_down_level = SENSOR_HIGH_LEVEL_PWDN,
    .identify_count = 1,
    .identify_code = {{GC5005_PID_ADDR, GC5005_PID_VALUE},
                      {GC5005_VER_ADDR, GC5005_VER_VALUE}},

    .source_width_max = SNAPSHOT_WIDTH,
    .source_height_max = SNAPSHOT_HEIGHT,
    .name = (cmr_s8 *)SENSOR_NAME,

    .image_format = SENSOR_IMAGE_FORMAT_RAW,

    .resolution_tab_info_ptr = s_gc5005_resolution_tab_raw,
    .sns_ops = &s_gc5005_ops_tab,
    .raw_info_ptr = &s_gc5005_mipi_raw_info_ptr,
    .module_info_tab = s_gc5005_module_info_tab,
    .module_info_tab_size = ARRAY_SIZE(s_gc5005_module_info_tab),

    .ext_info_ptr = NULL,
    .sensor_version_info = (cmr_s8 *) "gc5005_v1",
};

