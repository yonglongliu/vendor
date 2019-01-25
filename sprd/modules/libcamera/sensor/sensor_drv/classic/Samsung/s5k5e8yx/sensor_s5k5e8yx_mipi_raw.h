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
 * V5.0
 */
/*History
*Date                  Modification                                 Reason
*
*/
#ifndef SENSOR_S5K5E8YX_MIPI_RAW_H_
#define SENSOR_S5K5E8YX_MIPI_RAW_H_

#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"

#define SENSOR_S5K5E8YX_PIKE2_PARAM

#if defined(CONFIG_CAMERA_ISP_VERSION_V3) ||                                   \
    defined(CONFIG_CAMERA_ISP_VERSION_V4)
#ifdef SENSOR_S5K5E8YX_PIKE2_PARAM
#include "parameters_pike2/sensor_s5k5e8yx_raw_param_main.c"
#else
#include "parameters/sensor_s5k5e8yx_raw_param_main.c"
#endif
#else
#include "parameters/sensor_s5k5e8yx_raw_param.c"
#endif

#define VENDOR_NUM 2

#define CAMERA_IMAGE_180

#define SENSOR_NAME "s5k5e8yx_mipi_raw"
#define I2C_SLAVE_ADDR 0x20 /* 16bit slave address*/

#define S5K5E8YX_PID_ADDR 0x00
#define S5K5E8YX_PID_VALUE 0x5e
#define S5K5E8YX_VER_ADDR 0x01
#define S5K5E8YX_VER_VALUE 0x80

#if defined(CONFIG_CAMERA_SIZE_LIMIT_FOR_ANDROIDGO)
#define SNAPSHOT_WIDTH 1296
#define SNAPSHOT_HEIGHT 972
#else
#define SNAPSHOT_WIDTH 2592
#define SNAPSHOT_HEIGHT 1944
#endif
#define PREVIEW_WIDTH 1296
#define PREVIEW_HEIGHT 972

/*Raw Trim parameters*/
#define SNAPSHOT_TRIM_X 0
#define SNAPSHOT_TRIM_Y 0
#define SNAPSHOT_TRIM_W 2592
#define SNAPSHOT_TRIM_H 1944
#define PREVIEW_TRIM_X 0
#define PREVIEW_TRIM_Y 0
#define PREVIEW_TRIM_W 1296
#define PREVIEW_TRIM_H 972

#define LANE_NUM 2
#define RAW_BITS 10

#define SNAPSHOT_MIPI_PER_LANE_BPS 836
#define PREVIEW_MIPI_PER_LANE_BPS 836

/*line time unit: 0.1us*/
#define SNAPSHOT_LINE_TIME 16830
#define PREVIEW_LINE_TIME 30640

/* frame length*/
#define SNAPSHOT_FRAME_LENGTH 0x07B0
#define PREVIEW_FRAME_LENGTH 0x0426

/* please ref your spec */
#define FRAME_OFFSET 8
#define SENSOR_MAX_GAIN 0x200
#define SENSOR_BASE_GAIN 0x20
#define SENSOR_MIN_SHUTTER 4

/* please ref your spec
 *   1 : average binning
 *   2 : sum-average binning
 *   4 : sum binning
 *  */
#define BINNING_FACTOR 1

/* please ref spec
 *  * 1: sensor auto caculate
 *   * 0: driver caculate
 *    */
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

static const SENSOR_REG_T s5k5e8yx_init_setting[] = {
    {0x0100, 0x00}, // stream off
    {0x3906, 0x7E}, // global setting
    {0x3C01, 0x0F}, {0x3C14, 0x00}, {0x3235, 0x08}, {0x3063, 0x2E},
    {0x307A, 0x10}, {0x307B, 0x0E}, {0x3079, 0x20}, {0x3070, 0x05},
    {0x3067, 0x06}, {0x3071, 0x62}, {0x3203, 0x43}, {0x3205, 0x43},
    {0x320b, 0x42}, {0x3007, 0x00}, {0x3008, 0x14}, {0x3020, 0x58},
    {0x300D, 0x34}, {0x300E, 0x17}, {0x3021, 0x02}, {0x3010, 0x59},
    {0x3002, 0x01}, {0x3005, 0x01}, {0x3008, 0x04}, {0x300F, 0x70},
    {0x3010, 0x69}, {0x3017, 0x10}, {0x3019, 0x19}, {0x300C, 0x62},
    {0x3064, 0x10}, {0x3C08, 0x0E}, {0x3C09, 0x10}, {0x3C31, 0x0D},
    {0x3C32, 0xAC}, {0x3929, 0x07}, {0x0136, 0x18}, // preview_setting
    {0x0137, 0x00}, {0x0305, 0x06}, {0x0306, 0x18}, {0x0307, 0xA8},
    {0x0308, 0x34}, {0x0309, 0x42}, {0x3C1F, 0x00}, {0x3C17, 0x00},
    {0x3C0B, 0x04}, {0x3C1C, 0x47}, {0x3C1D, 0x15}, {0x3C14, 0x04},
    {0x3C16, 0x00}, {0x0820, 0x03}, {0x0821, 0x44}, {0x0114, 0x01},
    {0x0344, 0x00}, {0x0345, 0x08}, {0x0346, 0x00}, {0x0347, 0x08},
    {0x0348, 0x0A}, {0x0349, 0x27}, {0x034A, 0x07}, {0x034B, 0x9F},
    {0x034C, 0x05}, {0x034D, 0x10}, {0x034E, 0x03}, {0x034F, 0xCC},
    {0x0900, 0x01}, {0x0901, 0x22}, {0x0381, 0x01}, {0x0383, 0x01},
    {0x0385, 0x01}, {0x0387, 0x03}, {0x0340, 0x04}, {0x0341, 0x26},
    {0x0342, 0x14}, {0x0343, 0x50}, {0x0200, 0x00}, {0x0201, 0x00},
    {0x0202, 0x03}, {0x0203, 0xDE}, {0x3303, 0x02}, {0x3400, 0x01},
    {0x3C16, 0x00}, {0x3C0D, 0x04},
    //{0x0100, 0x01},
    //{0x3C22, 0x00},
    //{0x3C22, 0x00},
    //{0x3C0D, 0x00},

};

static const SENSOR_REG_T s5k5e8yx_preview_setting[] = {
    {0x0100, 0x00}, // stream off
    {0x0136, 0x18}, // preview_setting
    {0x0137, 0x00}, {0x0305, 0x06}, {0x0306, 0x18}, {0x0307, 0xA8},
    {0x0308, 0x34}, {0x0309, 0x42}, {0x3C1F, 0x00}, {0x3C17, 0x00},
    {0x3C0B, 0x04}, {0x3C1C, 0x47}, {0x3C1D, 0x15}, {0x3C14, 0x04},
    {0x3C16, 0x00}, {0x0820, 0x03}, {0x0821, 0x44}, {0x0114, 0x01},
    {0x0344, 0x00}, {0x0345, 0x08}, {0x0346, 0x00}, {0x0347, 0x08},
    {0x0348, 0x0A}, {0x0349, 0x27}, {0x034A, 0x07}, {0x034B, 0x9F},
    {0x034C, 0x05}, {0x034D, 0x10}, {0x034E, 0x03}, {0x034F, 0xCC},
    {0x0900, 0x01}, {0x0901, 0x22}, {0x0381, 0x01}, {0x0383, 0x01},
    {0x0385, 0x01}, {0x0387, 0x03}, {0x0340, 0x04}, {0x0341, 0x26},
    {0x0342, 0x14}, {0x0343, 0x50}, {0x0200, 0x00}, {0x0201, 0x00},
    {0x0202, 0x03}, {0x0203, 0xDE}, {0x3303, 0x02}, {0x3400, 0x01},
    {0x3C16, 0x00}, {0x3C0D, 0x04},
    //{0x0100, 0x01},
    //{0x3C22, 0x00},
    //{0x3C22, 0x00},
    //{0x3C0D, 0x00},
};

static const SENSOR_REG_T s5k5e8yx_snapshot_setting[] = {
    {0x0100, 0x00}, // stream off
    {0x0136, 0x18}, {0x0137, 0x00}, {0x0305, 0x06}, {0x0306, 0x18},
    {0x0307, 0xA8}, {0x0308, 0x34}, {0x0309, 0x42}, {0x3C1F, 0x00},
    {0x3C17, 0x00}, {0x3C0B, 0x04}, {0x3C1C, 0x47}, {0x3C1D, 0x15},
    {0x3C14, 0x04}, {0x3C16, 0x00}, {0x0820, 0x03}, {0x0821, 0x44},
    {0x0114, 0x01}, {0x0344, 0x00}, {0x0345, 0x08}, {0x0346, 0x00},
    {0x0347, 0x08}, {0x0348, 0x0A}, {0x0349, 0x27}, {0x034A, 0x07},
    {0x034B, 0x9F}, {0x034C, 0x0A}, {0x034D, 0x20}, {0x034E, 0x07},
    {0x034F, 0x98}, {0x0900, 0x00}, {0x0901, 0x00}, {0x0381, 0x01},
    {0x0383, 0x01}, {0x0385, 0x01}, {0x0387, 0x01}, {0x0340, 0x07},
    {0x0341, 0xB0}, {0x0342, 0x0B}, {0x0343, 0x28}, {0x0200, 0x00},
    {0x0201, 0x00}, {0x0202, 0x03}, {0x0203, 0xDE}, {0x3303, 0x02},
    {0x3400, 0x01}, {0x3C16, 0x00}, {0x3C0D, 0x04},
    //{0x0100, 0x01},
    //{0x3C22, 0x00},
    //{0x3C22, 0x00},
    //{0x3C0D, 0x00},
};

static struct sensor_res_tab_info s_s5k5e8yx_resolution_tab_raw[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .reg_tab = {{ADDR_AND_LEN_OF_ARRAY(s5k5e8yx_init_setting), PNULL, 0,
                  .width = 0, .height = 0, .xclk_to_sensor = EX_MCLK,
                  .image_format = SENSOR_IMAGE_FORMAT_RAW},

                 {ADDR_AND_LEN_OF_ARRAY(s5k5e8yx_preview_setting), PNULL, 0,
                  .width = PREVIEW_WIDTH, .height = PREVIEW_HEIGHT,
                  .xclk_to_sensor = EX_MCLK,
                  .image_format = SENSOR_IMAGE_FORMAT_RAW},

                 {ADDR_AND_LEN_OF_ARRAY(s5k5e8yx_snapshot_setting), PNULL, 0,
                  .width = SNAPSHOT_WIDTH, .height = SNAPSHOT_HEIGHT,
                  .xclk_to_sensor = EX_MCLK,
                  .image_format = SENSOR_IMAGE_FORMAT_RAW}}}
    /*If there are multiple modules,please add here*/
};

static SENSOR_TRIM_T s_s5k5e8yx_resolution_trim_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .trim_info =
         {
             {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
             {.trim_start_x = PREVIEW_TRIM_X,
              .trim_start_y = PREVIEW_TRIM_Y,
              .trim_width = PREVIEW_TRIM_W,
              .trim_height = PREVIEW_TRIM_H,
              .line_time = PREVIEW_LINE_TIME,
              .bps_per_lane = PREVIEW_MIPI_PER_LANE_BPS,
              .frame_line = PREVIEW_FRAME_LENGTH,
              .scaler_trim =
                  {.x = 0, .y = 0, .w = PREVIEW_TRIM_W, .h = PREVIEW_TRIM_H}},

             {.trim_start_x = SNAPSHOT_TRIM_X,
              .trim_start_y = SNAPSHOT_TRIM_Y,
              .trim_width = SNAPSHOT_TRIM_W,
              .trim_height = SNAPSHOT_TRIM_H,
              .line_time = SNAPSHOT_LINE_TIME,
              .bps_per_lane = SNAPSHOT_MIPI_PER_LANE_BPS,
              .frame_line = SNAPSHOT_FRAME_LENGTH,
              .scaler_trim =
                  {.x = 0, .y = 0, .w = SNAPSHOT_TRIM_W, .h = SNAPSHOT_TRIM_H}},
         }}
    /*If there are multiple modules,please add here*/
};

static const SENSOR_REG_T
    s_s5k5e8yx_preview_size_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
        /*video mode 0: ?fps */
        {{0xffff, 0xff}},
        /* video mode 1:?fps */
        {{0xffff, 0xff}},
        /* video mode 2:?fps */
        {{0xffff, 0xff}},
        /* video mode 3:?fps */
        {{0xffff, 0xff}}};

static const SENSOR_REG_T
    s_s5k5e8yx_capture_size_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
        /*video mode 0: ?fps */
        {{0xffff, 0xff}},
        /* video mode 1:?fps */
        {{0xffff, 0xff}},
        /* video mode 2:?fps */
        {{0xffff, 0xff}},
        /* video mode 3:?fps */
        {{0xffff, 0xff}}};

static SENSOR_VIDEO_INFO_T s_s5k5e8yx_video_info[SENSOR_MODE_MAX] = {
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{30, 30, 270, 90}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
     (SENSOR_REG_T **)s_s5k5e8yx_preview_size_video_tab},
    {{{2, 5, 338, 1000}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
     (SENSOR_REG_T **)s_s5k5e8yx_capture_size_video_tab},
};

static SENSOR_STATIC_INFO_T s_s5k5e8yx_static_info[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .static_info = {.f_num = 220,
                     .focal_length = 239,
                     .max_fps = 0,
                     .max_adgain = 16*256,
                     .ois_supported = 0,
                     .pdaf_supported = 0,
                     .exp_valid_frame_num = 1,
                     .clamp_level = 64,
                     .adgain_valid_frame_num = 1,
                     .fov_info = {{2.929f, 2.188f}, 2.39f}}}
    /*If there are multiple modules,please add here*/
};

static SENSOR_MODE_FPS_INFO_T s_s5k5e8yx_mode_fps_info[VENDOR_NUM] = {
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

static struct sensor_module_info s_s5k5e8yx_module_info_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .module_info = {.major_i2c_addr = I2C_SLAVE_ADDR >> 1,
                     .minor_i2c_addr = I2C_SLAVE_ADDR >> 1,

                     .reg_addr_value_bits = SENSOR_I2C_REG_16BIT |
                                            SENSOR_I2C_VAL_8BIT |
                                            SENSOR_I2C_FREQ_400,

                     .avdd_val = SENSOR_AVDD_2800MV,
                     .iovdd_val = SENSOR_AVDD_1800MV,
                     .dvdd_val = SENSOR_AVDD_1200MV,

                     .image_pattern = SENSOR_IMAGE_PATTERN_RAWRGB_GR,

                     .preview_skip_num = 1,
                     .capture_skip_num = 1,
                     .flash_capture_skip_num = 6,
                     .mipi_cap_skip_num = 0,
                     .preview_deci_num = 0,
                     .video_preview_deci_num = 0,

                     .sensor_interface = {.type = SENSOR_INTERFACE_TYPE_CSI2,
                                          .bus_width = LANE_NUM,
                                          .pixel_width = RAW_BITS,
                                          .is_loose = 0},

                     .change_setting_skip_num = 1,
                     .horizontal_view_angle = 65,
                     .vertical_view_angle = 60}}

    /*If there are multiple modules,please add here*/
};

static struct sensor_ic_ops s5k5e8yx_ops_tab;
static struct sensor_raw_info *s_s5k5e8yx_mipi_raw_info_ptr =
    &s_s5k5e8yx_mipi_raw_info;
/*==============================================================================
 * Description:
 * sensor all info
 * please modify this variable acording your spec
 *============================================================================*/
SENSOR_INFO_T g_s5k5e8yx_mipi_raw_info = {
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

    .identify_code = {{S5K5E8YX_PID_ADDR, S5K5E8YX_PID_VALUE},
                      {S5K5E8YX_VER_ADDR, S5K5E8YX_VER_VALUE}},

    .source_width_max = SNAPSHOT_WIDTH,
    .source_height_max = SNAPSHOT_HEIGHT,

    .name = (cmr_s8 *)SENSOR_NAME,
    .image_format = SENSOR_IMAGE_FORMAT_RAW,

    .resolution_tab_info_ptr = s_s5k5e8yx_resolution_tab_raw,
    .sns_ops = &s5k5e8yx_ops_tab,
    .raw_info_ptr = &s_s5k5e8yx_mipi_raw_info_ptr,
    .ext_info_ptr = NULL,

    .module_info_tab = s_s5k5e8yx_module_info_tab,
    .module_info_tab_size = ARRAY_SIZE(s_s5k5e8yx_module_info_tab),
    .video_tab_info_ptr = s_s5k5e8yx_video_info,

    .sensor_version_info = (cmr_s8 *)"s5k5e8yx_v1",
};

#endif
