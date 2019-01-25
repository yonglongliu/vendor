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
 * V1.0
 */
/*History
*Date                  Modification                                 Reason
*
*/

#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"

#include "parameters/sensor_gc5024_raw_param_main.c"

//#define FEATURE_OTP

#define NOT_SUPPORT_VIDEO

#define VENDOR_NUM 1
#define SENSOR_NAME "gc5024_mipi_raw"
#define I2C_SLAVE_ADDR 0x6e

#define gc5024_PID_ADDR 0xf0
#define gc5024_PID_VALUE 0x50
#define gc5024_VER_ADDR 0xf1
#define gc5024_VER_VALUE 0x24

/* sensor parameters begin */
/* effective sensor output image size */
#define VIDEO_WIDTH 1280
#define VIDEO_HEIGHT 1170
#define PREVIEW_WIDTH 2592
#define PREVIEW_HEIGHT 1944
#define SNAPSHOT_WIDTH 2592
#define SNAPSHOT_HEIGHT 1944

/*Raw Trim parameters*/
#define VIDEO_TRIM_X 0
#define VIDEO_TRIM_Y 0
#define VIDEO_TRIM_W VIDEO_WIDTH
#define VIDEO_TRIM_H VIDEO_HEIGHT
#define PREVIEW_TRIM_X 0
#define PREVIEW_TRIM_Y 0
#define PREVIEW_TRIM_W PREVIEW_WIDTH
#define PREVIEW_TRIM_H PREVIEW_HEIGHT
#define SNAPSHOT_TRIM_X 0
#define SNAPSHOT_TRIM_Y 0
#define SNAPSHOT_TRIM_W SNAPSHOT_WIDTH
#define SNAPSHOT_TRIM_H SNAPSHOT_HEIGHT

/*Mipi output*/
#define LANE_NUM 2
#define RAW_BITS 10

#define VIDEO_MIPI_PER_LANE_BPS 864    /* 2*Mipi clk */
#define PREVIEW_MIPI_PER_LANE_BPS 864  /* 2*Mipi clk */
#define SNAPSHOT_MIPI_PER_LANE_BPS 864 /* 2*Mipi clk */

/*line time unit: 0.1ns*/
#define VIDEO_LINE_TIME 16800
#define PREVIEW_LINE_TIME 16800
#define SNAPSHOT_LINE_TIME 16800

/* frame length*/
#define VIDEO_FRAME_LENGTH 1984
#define PREVIEW_FRAME_LENGTH 1984
#define SNAPSHOT_FRAME_LENGTH 1984

/* please ref your spec */
#define FRAME_OFFSET 0
#define SENSOR_MAX_GAIN 0x300
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

/*delay 1 frame to write sensor gain*/
//#define GAIN_DELAY_1_FRAME

/* sensor parameters end */

/* isp parameters, please don't change it*/
#define ISP_BASE_GAIN 0x80

/* please don't change it */
#define EX_MCLK 24

//#define IMAGE_NORMAL
#define IMAGE_H_MIRROR
//#define IMAGE_V_MIRROR
//#define IMAGE_HV_MIRROR

#ifdef IMAGE_NORMAL
#define MIRROR 0xd4
#define PH_SWITCH 0x1b
#define STARTY 0x02
#define STARTX 0x0c
#endif

#ifdef IMAGE_H_MIRROR
#define MIRROR 0xd5
#define PH_SWITCH 0x1a
#define STARTY 0x02
#define STARTX 0x01
#endif

#ifdef IMAGE_V_MIRROR
#define MIRROR 0xd6
#define PH_SWITCH 0x1b
#define STARTY 0x03
#define STARTX 0x0c
#endif

#ifdef IMAGE_HV_MIRROR
#define MIRROR 0xd7
#define PH_SWITCH 0x1a
#define STARTY 0x03
#define STARTX 0x01
#endif

/*==============================================================================
 * Description:
 * register setting
 *============================================================================*/

static const SENSOR_REG_T gc5024_init_setting[] = {
    /* SYS */
    {0xfe, 0x00},
    {0xfe, 0x00},
    {0xfe, 0x00},
    {0xf7, 0x01},
    {0xf8, 0x11},
    {0xf9, 0xae},
    {0xfa, 0x84},
    {0xfc, 0xae},
    {0xfe, 0x00},
    {0xfe, 0x00},
    {0xfe, 0x00},
    {0x88, 0x03},
    {0xe7, 0xc0},

    /* Analog */
    {0xfe, 0x00},
    {0x03, 0x06},
    {0x04, 0xfc},
    {0x05, 0x01},
    {0x06, 0xc5},
    {0x07, 0x00},
    {0x08, 0x1e},
    {0x0a, 0x00},
    {0x0c, 0x00},
    {0x0d, 0x07},
    {0x0e, 0xa8},
    {0x0f, 0x0a},
    {0x10, 0x40},
    {0x11, 0x31},
    {0x12, 0x26},
    {0x13, 0x10},
    {0x17, MIRROR}, // Don't Change Here!!!
    {0x18, 0x02},
    {0x19, 0x0d},
    {0x1a, PH_SWITCH}, // Don't Change Here!!!
    {0x1b, 0x11},
    {0x1c, 0x2c},
    {0x1e, 0x50},
    {0x1f, 0x70},
    {0x20, 0x86},
    {0x21, 0x0b},
    {0x24, 0xb0},
    {0x29, 0x3b},
    {0x2d, 0x16},
    {0x2f, 0x77},
    {0x32, 0x49},
    {0xcd, 0xaa},
    {0xd0, 0xd2},
    {0xd1, 0xd4},
    {0xd2, 0xcb},
    {0xd3, 0x33}, // 73    change
    {0xd8, 0x18},
    {0xd9, 0x70},
    {0xda, 0x03},
    {0xdb, 0x30},
    {0xdc, 0xba},
    {0xe2, 0x20},
    {0xe4, 0x70},
    {0xe6, 0x08},
    {0xe8, 0x03},
    {0xe9, 0x02},
    {0xea, 0x03},
    {0xeb, 0x03},
    {0xec, 0x03},
    {0xed, 0x02},
    {0xee, 0x03},
    {0xef, 0x03},

    /* ISP */
    {0x80, 0x50},
    {0x8d, 0x07},
    {0x90, 0x01},
    {0x92, STARTY}, // Don't Change Here!!!
    {0x94, STARTX}, // Don't Change Here!!!
    {0x95, 0x07},
    {0x96, 0x98},
    {0x97, 0x0a},
    {0x98, 0x20},

    /* Gain */
    {0x99, 0x01},
    {0x9a, 0x02},
    {0x9b, 0x03},
    {0x9c, 0x04},
    {0x9d, 0x0d},
    {0x9e, 0x15},
    {0x9f, 0x1d},
    {0xb0, 0x4b},
    {0xb1, 0x01},
    {0xb2, 0x00},
    {0xb6, 0x00},

    /* BLK */
    {0x40, 0x22},
    {0x4e, 0x3c}, // Don't Change Here!!!
    {0x4f, 0x00}, // Don't Change Here!!!
    {0x60, 0x00},
    {0x61, 0x80},
    {0xfe, 0x02},
    {0xa4, 0x30},
    {0xa5, 0x00},

    /* Dark Sun */
    {0x40, 0x96},
    {0x42, 0x0f},
    {0x45, 0xca},
    {0x47, 0xff},
    {0x48, 0xc8},

    /* DD */
    {0x80, 0x90},
    {0x81, 0x50},
    {0x82, 0x60},
    {0x84, 0x20},
    {0x85, 0x10},
    {0x86, 0x04},
    {0x87, 0x20},
    {0x88, 0x10},
    {0x89, 0x04},

    /*ASDE*/
    {0xa0, 0x30},
    {0xa3, 0xac},

    /* Degrid */
    {0x8a, 0x00},

    /* MIPI */
    {0xfe, 0x03},
    {0x10, 0x00}, // Stream off
    {0x01, 0x07},
    {0x02, 0x34},
    {0x03, 0x13},
    {0x04, 0x04},
    {0x05, 0x00},
    {0x06, 0x80},
    {0x11, 0x2b},
    {0x12, 0xa8},
    {0x13, 0x0c},
    {0x15, 0x00},
    {0x16, 0x09},
    {0x18, 0x01},
    {0x21, 0x10},
    {0x22, 0x05},
    {0x23, 0x30},
    {0x24, 0x10},
    {0x25, 0x16}, // 14
    {0x26, 0x08},
    {0x29, 0x06}, // 05
    {0x2a, 0x0a},
    {0x2b, 0x08},
    {0x42, 0x20},
    {0x43, 0x0a},
    {0xfe, 0x00},
};

static const SENSOR_REG_T gc5024_preview_setting[] = {
    /*
    {0xfe, 0x03},
    {0x10, 0x00},
    {0xfe, 0x00},
    */
};

static const SENSOR_REG_T gc5024_snapshot_setting[] = {

};

static const SENSOR_REG_T gc5024_video_setting[] = {
    /*
    {0xfe, 0x03},
    {0x10, 0x00},
    {0xfe, 0x00},
    */
};

static struct sensor_res_tab_info s_gc5024_resolution_tab_raw[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .reg_tab = {{ADDR_AND_LEN_OF_ARRAY(gc5024_init_setting), PNULL, 0,
                  .width = 0, .height = 0, .xclk_to_sensor = EX_MCLK,
                  .image_format = SENSOR_IMAGE_FORMAT_RAW},
#ifndef NOT_SUPPORT_VIDEO
                 {ADDR_AND_LEN_OF_ARRAY(gc5024_video_setting), PNULL, 0,
                  .width = VIDEO_WIDTH, .height = VIDEO_HEIGHT,
                  .xclk_to_sensor = EX_MCLK,
                  .image_format = SENSOR_IMAGE_FORMAT_RAW},

                 {ADDR_AND_LEN_OF_ARRAY(gc5024_preview_setting), PNULL, 0,
                  .width = PREVIEW_WIDTH, .height = PREVIEW_HEIGHT,
                  .xclk_to_sensor = EX_MCLK,
                  .image_format = SENSOR_IMAGE_FORMAT_RAW},
#endif
                 {ADDR_AND_LEN_OF_ARRAY(gc5024_snapshot_setting), PNULL, 0,
                  .width = SNAPSHOT_WIDTH, .height = SNAPSHOT_HEIGHT,
                  .xclk_to_sensor = EX_MCLK,
                  .image_format = SENSOR_IMAGE_FORMAT_RAW}}},

    /*If there are multiple modules,please add here*/
};

static SENSOR_TRIM_T s_gc5024_resolution_trim_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .trim_info =
         {
             {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
#ifndef NOT_SUPPORT_VIDEO
             {.trim_start_x = VIDEO_TRIM_X,
              .trim_start_y = VIDEO_TRIM_Y,
              .trim_width = VIDEO_TRIM_W,
              .trim_height = VIDEO_TRIM_H,
              .line_time = VIDEO_LINE_TIME,
              .bps_per_lane = VIDEO_MIPI_PER_LANE_BPS,
              .frame_line = VIDEO_FRAME_LENGTH,
              .scaler_trim = {.x = VIDEO_TRIM_X,
                              .y = VIDEO_TRIM_Y,
                              .w = VIDEO_TRIM_W,
                              .h = VIDEO_TRIM_H}},

             {.trim_start_x = PREVIEW_TRIM_X,
              .trim_start_y = PREVIEW_TRIM_Y,
              .trim_width = PREVIEW_TRIM_W,
              .trim_height = PREVIEW_TRIM_H,
              .line_time = PREVIEW_LINE_TIME,
              .bps_per_lane = PREVIEW_MIPI_PER_LANE_BPS,
              .frame_line = PREVIEW_FRAME_LENGTH,
              .scaler_trim = {.x = PREVIEW_TRIM_X,
                              .y = PREVIEW_TRIM_Y,
                              .w = PREVIEW_TRIM_W,
                              .h = PREVIEW_TRIM_H}},
#endif
             {.trim_start_x = SNAPSHOT_TRIM_X,
              .trim_start_y = SNAPSHOT_TRIM_Y,
              .trim_width = SNAPSHOT_TRIM_W,
              .trim_height = SNAPSHOT_TRIM_H,
              .line_time = SNAPSHOT_LINE_TIME,
              .bps_per_lane = SNAPSHOT_MIPI_PER_LANE_BPS,
              .frame_line = SNAPSHOT_FRAME_LENGTH,
              .scaler_trim = {.x = SNAPSHOT_TRIM_X,
                              .y = SNAPSHOT_TRIM_Y,
                              .w = SNAPSHOT_TRIM_W,
                              .h = SNAPSHOT_TRIM_H}},
         }},

    /*If there are multiple modules,please add here*/

};

static SENSOR_REG_T gc5024_shutter_reg[] = {
    {0xfe, 0x00}, {0x32, 0x49}, {0xb0, 0x4b},
    {0xfe, 0x00}, {0x04, 0xB0}, {0x03, 0x04},
};

static struct sensor_i2c_reg_tab gc5024_shutter_tab = {
    .settings = gc5024_shutter_reg, .size = ARRAY_SIZE(gc5024_shutter_reg),
};

static SENSOR_REG_T gc5024_again_reg[] = {
    //{0x0204, 0x00},
    {0xfe, 0x00}, {0x29, 0x3b}, {0xd8, 0x18}, {0xe8, 0x03}, {0xe9, 0x02},
    {0xec, 0x03}, {0xed, 0x02}, {0xb6, 0x03}, {0xb1, 0x01}, {0xb2, 0},

};

static struct sensor_i2c_reg_tab gc5024_again_tab = {
    .settings = gc5024_again_reg, .size = ARRAY_SIZE(gc5024_again_reg),
};

static SENSOR_REG_T gc5024_dgain_reg[] = {

};

static struct sensor_i2c_reg_tab gc5024_dgain_tab = {
    .settings = gc5024_dgain_reg, .size = ARRAY_SIZE(gc5024_dgain_reg),
};

static SENSOR_REG_T gc5024_frame_length_reg[] = {

};

static struct sensor_i2c_reg_tab gc5024_frame_length_tab = {
    .settings = gc5024_frame_length_reg,
    .size = ARRAY_SIZE(gc5024_frame_length_reg),
};

static struct sensor_aec_i2c_tag gc5024_aec_info = {
    .slave_addr = (I2C_SLAVE_ADDR >> 1),
    .addr_bits_type = SENSOR_I2C_REG_8BIT,
    .data_bits_type = SENSOR_I2C_VAL_8BIT,
    .shutter = &gc5024_shutter_tab,
    .again = &gc5024_again_tab,
    .dgain = &gc5024_dgain_tab,
    .frame_length = &gc5024_frame_length_tab,
};

static SENSOR_STATIC_INFO_T s_gc5024_static_info[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .static_info = {.f_num = 200,
                     .focal_length = 354,
                     .max_fps = 0,
                     .max_adgain = 15 * 2,
                     .ois_supported = 0,
                     .pdaf_supported = 0,
                     .exp_valid_frame_num = 1,
                     .clamp_level = 64,
                     .adgain_valid_frame_num = 1,
                     .fov_info = {{4.614f, 3.444f}, 4.222f}}}
    /*If there are multiple modules,please add here*/
};

static SENSOR_MODE_FPS_INFO_T s_gc5024_mode_fps_info[VENDOR_NUM] = {
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

static struct sensor_module_info s_gc5024_module_info_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .module_info = {.major_i2c_addr = I2C_SLAVE_ADDR >> 1,
                     .minor_i2c_addr = I2C_SLAVE_ADDR >> 1,

                     .reg_addr_value_bits = SENSOR_I2C_REG_8BIT |
                                            SENSOR_I2C_VAL_8BIT |
                                            SENSOR_I2C_FREQ_400,

                     .avdd_val = SENSOR_AVDD_2800MV,
                     .iovdd_val = SENSOR_AVDD_1800MV,
                     .dvdd_val = SENSOR_AVDD_1600MV,
                     //.dvdd_val = SENSOR_AVDD_1400MV,
                     .image_pattern = SENSOR_IMAGE_PATTERN_RAWRGB_R,

                     .preview_skip_num = 1,
                     .capture_skip_num = 1,
                     .flash_capture_skip_num = 6,
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
                     .change_setting_skip_num = 1,
                     .horizontal_view_angle = 65,
                     .vertical_view_angle = 60}}

    /*If there are multiple modules,please add here*/
};

static struct sensor_ic_ops s_gc5024_ops_tab;
struct sensor_raw_info *s_gc5024_mipi_raw_info_ptr = &s_gc5024_mipi_raw_info;

SENSOR_INFO_T g_gc5024_mipi_raw_info = {
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
    .power_down_level = SENSOR_HIGH_LEVEL_PWDN,
    .identify_count = 1,
    .identify_code = {{.reg_addr = gc5024_PID_ADDR,
                       .reg_value = gc5024_PID_VALUE},
                      {.reg_addr = gc5024_VER_ADDR,
                       .reg_value = gc5024_VER_VALUE}},

    .source_width_max = SNAPSHOT_WIDTH,
    .source_height_max = SNAPSHOT_HEIGHT,
    .name = (cmr_s8 *)SENSOR_NAME,
    .image_format = SENSOR_IMAGE_FORMAT_RAW,

    .module_info_tab = s_gc5024_module_info_tab,
    .module_info_tab_size = ARRAY_SIZE(s_gc5024_module_info_tab),

    .resolution_tab_info_ptr = s_gc5024_resolution_tab_raw,
    .sns_ops = &s_gc5024_ops_tab,
    .raw_info_ptr = &s_gc5024_mipi_raw_info_ptr,

    .video_tab_info_ptr = NULL,
    .sensor_version_info = (cmr_s8 *)"gc5024_v1",
};
