/*
* Copyright (C) 2012 The Android Open Source Project8u7
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
#ifndef _SENSOR_C2390_MIPI_RAW_H_
#define _SENSOR_C2390_MIPI_RAW_H_

#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"

#if 0 // defined(CONFIG_CAMERA_ISP_VERSION_V3) ||
      // defined(CONFIG_CAMERA_ISP_VERSION_V4)
//#include "sensor_c2390_raw_param_v3.c"
#include "sensor_c2390_raw_param_main.c"

#else
#include "parameters/sensor_c2390_raw_param_main.c"
#endif

#define VENDOR_NUM 1
#define SENSOR_NAME "c2390_mipi_raw"
#define I2C_SLAVE_ADDR 0x6c /* 8bit slave address*/

#define c2390_PID_ADDR 0x0000
#define c2390_PID_VALUE 0x02
#define c2390_VER_ADDR 0x0001
#define c2390_VER_VALUE 0x03
/* sensor parameters begin */
/* effective sensor output image size */
#define SNAPSHOT_WIDTH 1920
#define SNAPSHOT_HEIGHT 1080
#define PREVIEW_WIDTH 1920
#define PREVIEW_HEIGHT 1080

/*Mipi output*/
#define LANE_NUM 2
#define RAW_BITS 10

#define SNAPSHOT_MIPI_PER_LANE_BPS 400
#define PREVIEW_MIPI_PER_LANE_BPS 400

/*line time unit: 0.1us*/
#define SNAPSHOT_LINE_TIME 30150 //
#define PREVIEW_LINE_TIME 30150  //

/* frame length*/
#define SNAPSHOT_FRAME_LENGTH 1104 // 0x450
#define PREVIEW_FRAME_LENGTH 1104  // 0x450

/* please ref your spec */
#define FRAME_OFFSET 4
#define SENSOR_MAX_GAIN 0xFFFF // 256 multiple
#define SENSOR_BASE_GAIN 0x10
#define SENSOR_MIN_SHUTTER 2
#if 0 // def CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
#undef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
#endif

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

//#define DYNA_BLC_ENABLE
#ifdef DYNA_BLC_ENABLE
static uint32_t BLC_FLAG = 0;
#define GAIN_THRESHOLD_HIGH 0x6c
#define GAIN_THRESHOLD_LOW 0x50
#define DATA_THRESHOLD 16
#endif

static const SENSOR_REG_T c2390_init_setting[] = {
    {0x0103, 0x01},
    {SENSOR_WRITE_DELAY, 0x1a},
    {0x0003, 0x00},
    {0x0003, 0x00},
    {0x0003, 0x00},
    {0x0003, 0x00},
    {0x0003, 0x00},
    {0x0003, 0x00},
    {0x0003, 0x00},
    {0x0003, 0x00},
    {0x0003, 0x00},
    {0x0003, 0x00},
    {0x0003, 0x00},
    {0x0101, 0x01},
    {0x3000, 0x80},
    {0x3080, 0x01},
    {0x3081, 0x14},
    {0x3082, 0x01},
    {0x3083, 0x4b},
    {0x3087, 0xd0},
    {0x3089, 0x10},
    {0x3180, 0x10},
    {0x3182, 0x30},
    {0x3183, 0x10},
    {0x3184, 0x20},
    {0x3185, 0xc0},
    {0x3189, 0x50},
    {0x3c03, 0x00},
    {0x3f8c, 0x00},
    {0x320f, 0x48},
    {0x3023, 0x00},
    {0x3d00, 0x33},
    {0x3c9d, 0x01},
    {0x3f08, 0x00},
    {0x0309, 0x2e},
    {0x0303, 0x01},
    {0x0304, 0x01},
    {0x0307, 0x56},
    {0x3508, 0x00},
    {0x3509, 0xcc},
    {0x3292, 0x28},
    {0x350a, 0x22},
    {0x3209, 0x05},
    {0x0003, 0x00},
    {0x0003, 0x00},
    {0x0003, 0x00},
    {0x0003, 0x00},
    {0x3209, 0x04},
    {0x3108, 0xcd},
    {0x3109, 0x7f},
    {0x310a, 0x44},
    {0x310b, 0x42},
    {0x3112, 0x60},
    {0x3113, 0x01},
    {0x3114, 0xc0},
    {0x3115, 0x40},
    {0x3905, 0x01},
    {0x3980, 0x01},
    {0x3881, 0x04},
    {0x3882, 0x11},
    {0x328b, 0x03},
    {0x328c, 0x00},
    {0x3981, 0x57},
    {0x3180, 0x10},
    {0x3213, 0x00},
    {0x3205, 0x40},
    {0x3208, 0x8d},
    {0x3210, 0x12},
    {0x3211, 0x40},
    {0x3212, 0x50},
    {0x3215, 0xc0},
    {0x3216, 0x70},
    {0x3217, 0x08},
    {0x3218, 0x20},
    {0x321a, 0x80},
    {0x321b, 0x00},
    {0x321c, 0x1a},
    {0x321e, 0x00},
    {0x3223, 0x20},
    {0x3224, 0x88},
    {0x3225, 0x00},
    {0x3226, 0x08},
    {0x3227, 0x00},
    {0x3228, 0x00},
    {0x3229, 0x08},
    {0x322a, 0x00},
    {0x322b, 0x44},
    {0x308a, 0x00},
    {0x308b, 0x00},
    {0x3280, 0x06},
    {0x3281, 0x30},
    {0x3282, 0x08},
    {0x3283, 0x51},
    {0x3284, 0x0d},
    {0x3285, 0x48},
    {0x3286, 0x3b},
    {0x3287, 0x07},
    {0x3288, 0x00},
    {0x3289, 0x00},
    {0x328a, 0x08},
    {0x328d, 0x01},
    {0x328e, 0x20},
    {0x328f, 0x0d},
    {0x3290, 0x10},
    {0x3291, 0x00},
    {0x3292, 0x28},
    {0x3293, 0x00},
    {0x3216, 0x50},
    {0x3217, 0x04},
    {0x3205, 0x20},
    {0x3215, 0x50},
    {0x3223, 0x10},
    {0x3280, 0x06},
    {0x3282, 0x04},
    {0x3283, 0x50},
    {0x308b, 0x00},
    {0x3184, 0x20},
    {0x3185, 0xc0},
    {0x3189, 0x50},
    {0x3280, 0x86},
    {0x0003, 0x00},
    {0x3280, 0x06},
    {0x0383, 0x01},
    {0x0387, 0x01},
    {0x0340, 0x04},
    {0x0341, 0x50},
    {0x0342, 0x08},
    {0x0343, 0x1c},
    {0x034c, 0x07},
    {0x034d, 0x80},
    {0x034e, 0x04},
    {0x034f, 0x38},
    {0x3b80, 0x42},
    {0x3b81, 0x10},
    {0x3b82, 0x10},
    {0x3b83, 0x10},
    {0x3b84, 0x04},
    {0x3b85, 0x04},
    {0x3b86, 0x80},
    {0x3b86, 0x80},
    {0x3021, 0x11},
    {0x3022, 0x22},
    {0x3209, 0x04},
    {0x3584, 0x12},
    {0x3805, 0x05},
    {0x3806, 0x03},
    {0x3807, 0x03},
    {0x3808, 0x0c},
    {0x3809, 0x64},
    {0x380a, 0x5b},
    {0x380b, 0xe6},
    {0x3500, 0x10},
    {0x308c, 0x20},
    {0x308d, 0x31},
    {0x3403, 0x00},
    {0x3407, 0x01},
    {0x3410, 0x04},
    {0x3414, 0x01},
    {0xe000, 0x32},
    {0xe001, 0x85},
    {0xe002, 0x48},
    {0xe030, 0x32},
    {0xe031, 0x85},
    {0xe032, 0x48},
    {0x3500, 0x00},
    {0x3a87, 0x02},
    {0x3a88, 0x08},
    {0x3a89, 0x30},
    {0x3a8a, 0x01},
    {0x3a8b, 0x90},
    {0x3a80, 0x88},
    {0x3a81, 0x02},
    {0x3009, 0x09},
    {0x300b, 0x08},
    {0x034b, 0x47},
    {0x0202, 0x01},
    {0x0203, 0x4c},

    ///////////////////////////
    {0x3500, 0x10},
    {0x3404, 0x04},
    {0xe000, 0x02},
    {0xe001, 0x05},
    {0xe002, 0x00},
    {0xe003, 0x32},
    {0xe004, 0x8e},
    {0xe005, 0x20},
    {0xe006, 0x02},
    {0xe007, 0x16},
    {0xe008, 0x01},
    {0xe009, 0x02},
    {0xe00a, 0x17},
    {0xe00b, 0x00},
    {0x3500, 0x00},

    //////////////////////////

    {0x3411, 0x18}, // ;use group 5
    {0x3415, 0x03},
    {0x3500, 0x10},
    {0xe120, 0x35},
    {0xe121, 0x00},
    {0xe122, 0x02},
    {0xe123, 0x03},
    {0xe124, 0x09},
    {0xe125, 0x2e},
    {0xe126, 0x32},
    {0xe127, 0x8e},
    {0xe128, 0x10},
    {0x3500, 0x00},

};

static const SENSOR_REG_T c2390_preview_setting[] = {

};

static const SENSOR_REG_T c2390_snapshot_setting[] = {

};

static struct sensor_res_tab_info s_c2390_resolution_tab_raw[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .reg_tab = {{ADDR_AND_LEN_OF_ARRAY(c2390_init_setting), PNULL, 0,
                  .width = 0, .height = 0, .xclk_to_sensor = EX_MCLK,
                  .image_format = SENSOR_IMAGE_FORMAT_RAW},

                 {ADDR_AND_LEN_OF_ARRAY(c2390_preview_setting), PNULL, 0,
                  .width = PREVIEW_WIDTH, .height = PREVIEW_HEIGHT,
                  .xclk_to_sensor = EX_MCLK,
                  .image_format = SENSOR_IMAGE_FORMAT_RAW},

                 {ADDR_AND_LEN_OF_ARRAY(c2390_snapshot_setting), PNULL, 0,
                  .width = SNAPSHOT_WIDTH, .height = SNAPSHOT_HEIGHT,
                  .xclk_to_sensor = EX_MCLK,
                  .image_format = SENSOR_IMAGE_FORMAT_RAW}}}
    /*If there are multiple modules,please add here*/
};

static SENSOR_TRIM_T s_c2390_resolution_trim_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .trim_info =
         {
             {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
             {.trim_start_x = 0,
              .trim_start_y = 0,
              .trim_width = PREVIEW_WIDTH,
              .trim_height = PREVIEW_HEIGHT,
              .line_time = PREVIEW_LINE_TIME,
              .bps_per_lane = PREVIEW_MIPI_PER_LANE_BPS,
              .frame_line = PREVIEW_FRAME_LENGTH,
              .scaler_trim =
                  {.x = 0, .y = 0, .w = PREVIEW_WIDTH, .h = PREVIEW_HEIGHT}},
             {.trim_start_x = 0,
              .trim_start_y = 0,
              .trim_width = SNAPSHOT_WIDTH,
              .trim_height = SNAPSHOT_HEIGHT,
              .line_time = SNAPSHOT_LINE_TIME,
              .bps_per_lane = SNAPSHOT_MIPI_PER_LANE_BPS,
              .frame_line = SNAPSHOT_FRAME_LENGTH,
              .scaler_trim =
                  {.x = 0, .y = 0, .w = SNAPSHOT_WIDTH, .h = SNAPSHOT_HEIGHT}},
         }}

    /*If there are multiple modules,please add here*/
};

static const SENSOR_REG_T
    s_c2390_preview_size_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
        /*video mode 0: ?fps */
        {{0xffff, 0xff}},
        /* video mode 1:?fps */
        {{0xffff, 0xff}},
        /* video mode 2:?fps */
        {{0xffff, 0xff}},
        /* video mode 3:?fps */
        {{0xffff, 0xff}}};

static const SENSOR_REG_T
    s_c2390_capture_size_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
        /*video mode 0: ?fps */
        {{0xffff, 0xff}},
        /* video mode 1:?fps */
        {{0xffff, 0xff}},
        /* video mode 2:?fps */
        {{0xffff, 0xff}},
        /* video mode 3:?fps */
        {{0xffff, 0xff}}};

static SENSOR_VIDEO_INFO_T s_c2390_video_info[SENSOR_MODE_MAX] = {
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{30, 30, 334, 90}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
     (SENSOR_REG_T **)s_c2390_preview_size_video_tab},
    {{{15, 15, 334, 1000}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
     (SENSOR_REG_T **)s_c2390_capture_size_video_tab},
};

static SENSOR_STATIC_INFO_T s_c2390_static_info[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .static_info = {.f_num = 220,
                     .focal_length = 346,
                     .max_fps = 0,
                     .max_adgain = 8,
                     .ois_supported = 0,
                     .pdaf_supported = 0,
                     .exp_valid_frame_num = 1,
                     .clamp_level = 0,
                     .adgain_valid_frame_num = 1,
                     .fov_info = {{4.614f, 3.444f}, 4.222f}}}
    /*If there are multiple modules,please add here*/
};

static SENSOR_MODE_FPS_INFO_T s_c2390_mode_fps_info[VENDOR_NUM] = {
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
       {SENSOR_MODE_SNAPSHOT_TWO_THIRD, 0, 1, 0, 0}}}}
    /*If there are multiple modules,please add here*/
};

static struct sensor_module_info s_c2390_module_info_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .module_info =
         {
             .major_i2c_addr = I2C_SLAVE_ADDR >> 1,
             .minor_i2c_addr = I2C_SLAVE_ADDR >> 1,

             .reg_addr_value_bits = SENSOR_I2C_REG_16BIT | SENSOR_I2C_VAL_8BIT |
                                    SENSOR_I2C_FREQ_400,

             .avdd_val = SENSOR_AVDD_3300MV,
             .iovdd_val = SENSOR_AVDD_1800MV,
             .dvdd_val = SENSOR_AVDD_1500MV,

             .image_pattern = SENSOR_IMAGE_PATTERN_RAWRGB_B,

             .preview_skip_num = 3,
             .capture_skip_num = 3,
             .mipi_cap_skip_num = 0,
             .preview_deci_num = 0,
             .video_preview_deci_num = 0,

             .threshold_eb = 0,
             .threshold_mode = 0,
             .threshold_start = 0,
             .threshold_end = 0,

             .sensor_interface =
                 {
                     .type = SENSOR_INTERFACE_TYPE_CSI2,
                     .bus_width = LANE_NUM,
                     .pixel_width = RAW_BITS,
                     .is_loose = 0,
                 },

             .change_setting_skip_num = 2,
             .horizontal_view_angle = 65,
             .vertical_view_angle = 60,
         }}
    /*If there are multiple modules,please add here*/
};

/*==============================================================================
 * Description:
 * sensor all info
 * please modify this variable acording your spec
 *============================================================================*/
static struct sensor_ic_ops s_c2390_ops_tab;
struct sensor_raw_info *s_c2390_mipi_raw_info_ptr = &s_c2390_mipi_raw_info;

SENSOR_INFO_T g_c2390_mipi_raw_info = {
    .hw_signal_polarity = SENSOR_HW_SIGNAL_PCLK_P | SENSOR_HW_SIGNAL_VSYNC_P |
                          SENSOR_HW_SIGNAL_HSYNC_P,

    .environment_mode = SENSOR_ENVIROMENT_NORMAL | SENSOR_ENVIROMENT_NIGHT,

    .image_effect = SENSOR_IMAGE_EFFECT_NORMAL |
                    SENSOR_IMAGE_EFFECT_BLACKWHITE | SENSOR_IMAGE_EFFECT_RED |
                    SENSOR_IMAGE_EFFECT_GREEN | SENSOR_IMAGE_EFFECT_BLUE |
                    SENSOR_IMAGE_EFFECT_YELLOW | SENSOR_IMAGE_EFFECT_NEGATIVE |
                    SENSOR_IMAGE_EFFECT_CANVAS,

    .wb_mode = 0,
    .step_count = 7,

    .reset_pulse_level = SENSOR_LOW_PULSE_RESET,
    .reset_pulse_width = 50,
    .power_down_level = SENSOR_LOW_LEVEL_PWDN,

    .identify_count = 1,

    .identify_code = {{c2390_PID_ADDR, c2390_PID_VALUE},
                      {c2390_VER_ADDR, c2390_VER_VALUE}},

    .source_width_max = SNAPSHOT_WIDTH,
    .source_height_max = SNAPSHOT_HEIGHT,
    .name = (cmr_s8 *)SENSOR_NAME,

    .image_format = SENSOR_IMAGE_FORMAT_RAW,

    .resolution_tab_info_ptr = s_c2390_resolution_tab_raw,
    .sns_ops = &s_c2390_ops_tab,
    .module_info_tab = s_c2390_module_info_tab,
    .module_info_tab_size = ARRAY_SIZE(s_c2390_module_info_tab),

    .raw_info_ptr = &s_c2390_mipi_raw_info_ptr,
    .video_tab_info_ptr = s_c2390_video_info,
    .sensor_version_info = (cmr_s8 *)"c2390_v1",
};

#endif
