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
#ifndef _SENSOR_OV540_MIPI_RAW_H_
#define _SENSOR_OV540_MIPI_RAW_H_

#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"
#if defined(CONFIG_CAMERA_ISP_VERSION_V3) ||                                   \
    defined(CONFIG_CAMERA_ISP_VERSION_V4)
#include "parameters/sensor_ov5640_raw_param_main.c"
#else
#endif

#define ov5640_I2C_ADDR_W (0x3c >> 1)
#define ov5640_I2C_ADDR_R (0x3c >> 1)
#define SENSOR_NAME "ov5640_mipi_raw"

#define VENDOR_NUM 1
LOCAL const SENSOR_REG_T ov5640_com_mipi_raw[] = {
    {0x3103, 0x11}, /* sysclk from pad*/
    {0x3008, 0x82}, /*software reset*/
    {SENSOR_WRITE_DELAY, 0x0a},
    {0x3008, 0x42},
    {0x3103, 0x03},
    {0x3017, 0x00},
    {0x3018, 0x00},
    {0x3034, 0x1a},
#if (SC_FPGA == 0)
    {0x3035, 0x12},
#else
    {0x3035, 0x21},
#endif
    {0x3036, 0x54},
    {0x3037, 0x13},
    {0x3108, 0x01},
    {0x3630, 0x36},
    {0x3631, 0x0e},
    {0x3632, 0xe2},
    {0x3633, 0x12},
    {0x3621, 0xe0},
    {0x3704, 0xa0},
    {0x3703, 0x5a},
    {0x3715, 0x78},
    {0x3717, 0x01},
    {0x370b, 0x60},
    {0x3705, 0x1a},
    {0x3905, 0x02},
    {0x3906, 0x10},
    {0x3901, 0x0a},
    {0x3731, 0x12},
    {0x3600, 0x08},
    {0x3601, 0x33},
    {0x302d, 0x60},
    {0x3620, 0x52},
    {0x371b, 0x20},
    {0x471c, 0x50},
    {0x3a13, 0x43},
    {0x3a18, 0x00},
    {0x3a19, 0xf8},
    {0x3635, 0x13},
    {0x3636, 0x03},
    {0x3634, 0x40},
    {0x3622, 0x01},
    {0x3c01, 0x34},
    {0x3c04, 0x28},
    {0x3c05, 0x98},
    {0x3c06, 0x00},
    {0x3c07, 0x07},
    {0x3c08, 0x00},
    {0x3c09, 0x1c},
    {0x3c0a, 0x9c},
    {0x3c0b, 0x40},
    {0x3820, 0x41},
    {0x3821, 0x03},
    {0x3814, 0x31},
    {0x3815, 0x31},
    {0x3800, 0x00},
    {0x3801, 0x00},
    {0x3802, 0x00},
    {0x3803, 0x04},
    {0x3804, 0x0a},
    {0x3805, 0x3f},
    {0x3806, 0x07},
    {0x3807, 0x9b},
    {0x3808, 0x05},
    {0x3809, 0x00},
    {0x380a, 0x03},
    {0x380b, 0xc0},
    {0x380c, 0x07},
    {0x380d, 0x68},
    {0x380e, 0x04},
    {0x380f, 0x9d},
    {0x3810, 0x00},
    {0x3811, 0x10},
    {0x3812, 0x00},
    {0x3813, 0x06},
    {0x3618, 0x00},
    {0x3612, 0x29},
    {0x3708, 0x64},
    {0x3709, 0x52},
    {0x370c, 0x03},
    {0x3a02, 0x03},
    {0x3a03, 0xd8},
    {0x3a08, 0x01},
    {0x3a09, 0x27},
    {0x3a0a, 0x00},
    {0x3a0b, 0xf6},
    {0x3a0e, 0x03},
    {0x3a0d, 0x04},
    {0x3a14, 0x03},
    {0x3a15, 0xd8},
    {0x4001, 0x02},
    {0x4004, 0x02},
    {0x3000, 0x00},
    {0x3002, 0x1c},
    {0x3004, 0xff},
    {0x3006, 0xc3},
    {0x300e, 0x45},
    {0x302e, 0x08},
    {0x4300, 0xf8},
    {0x501f, 0x03},
    {0x4713, 0x03},
    {0x4407, 0x04},
    {0x440e, 0x00},
    {0x460b, 0x37},
    {0x460c, 0x20},
    {0x4837, 0x0a},
    {0x3824, 0x04},
    {0x5000, 0x06},
    {0x5001, 0x00},
    {0x3a0f, 0x36},
    {0x3a10, 0x2e},
    {0x3a1b, 0x38},
    {0x3a1e, 0x2c},
    {0x3a11, 0x70},
    {0x3a1f, 0x18},
    {0x3503, 0x03}, // bit0 agc bit1 aec
    // 0:auto 1:maul
    {0x3501, 0x37},
    {0x3502, 0x50},
    {0x350b, 0x3f},
    {0x3406, 0x01}, // awb 0:auto 1:disable
    {0x3400, 0x04},
    {0x3401, 0x00},
    {0x3402, 0x04},
    {0x3403, 0x00},
    {0x3404, 0x04},
    {0x3405, 0x00},
    {0x3602, 0x00},
    {0x3603, 0x00},
    {0x3605, 0x46},
    {0x3604, 0x05}};

LOCAL const SENSOR_REG_T ov5640_1280X960_mipi_raw[] = {
    {0x3008, 0x42}, {0x3034, 0x1a},
#if (SC_FPGA == 0)
    {0x3035, 0x12},
#else
    {0x3035, 0x21},
#endif
    {0x3036, 0x54}, {0x3037, 0x13}, {0x3108, 0x01}, {0x3820, 0x41},
    {0x3821, 0x03}, {0x3814, 0x31}, {0x3815, 0x31}, {0x3800, 0x00},
    {0x3801, 0x00}, {0x3802, 0x00}, {0x3803, 0x04}, {0x3804, 0x0a},
    {0x3805, 0x3f}, {0x3806, 0x07}, {0x3807, 0x9b}, {0x3808, 0x05},
    {0x3809, 0x00}, {0x380a, 0x03}, {0x380b, 0xc0}, {0x380c, 0x07},
    {0x380d, 0x68}, {0x380e, 0x05}, {0x380f, 0x94}, {0x3810, 0x00},
    {0x3811, 0x10}, {0x3812, 0x00}, {0x3813, 0x06}, {0x3618, 0x00},
    {0x3612, 0x29}, {0x3708, 0x64}, {0x3709, 0x52}, {0x370c, 0x03},
    {0x3a02, 0x03}, {0x3a03, 0xd8}, {0x3a14, 0x03}, {0x3a15, 0xd8},
    {0x4004, 0x02}, {0x4837, 0x0a}};

LOCAL const SENSOR_REG_T ov5640_2592X1944_mipi_raw[] = {
    {0x3008, 0x42}, {0x3034, 0x1a},
#if (SC_FPGA == 0)
    {0x3035, 0x12},
#else
    {0x3035, 0x21},
#endif
    {0x3036, 0x69}, {0x3037, 0x13}, {0x3108, 0x01}, {0x3820, 0x41},
    {0x3821, 0x03}, {0x3814, 0x11}, {0x3815, 0x11}, {0x3800, 0x00},
    {0x3801, 0x00}, {0x3802, 0x00}, {0x3803, 0x00}, {0x3804, 0x0a},
    {0x3805, 0x3f}, {0x3806, 0x07}, {0x3807, 0x9f}, {0x3808, 0x0a},
    {0x3809, 0x20}, {0x380a, 0x07}, {0x380b, 0x98}, {0x380c, 0x0b},
    {0x380d, 0x1c}, {0x380e, 0x07}, {0x380f, 0xb0}, {0x3810, 0x00},
    {0x3811, 0x10}, {0x3812, 0x00}, {0x3813, 0x04}, {0x3618, 0x04},
    {0x3612, 0x2b}, {0x3708, 0x64}, {0x3709, 0x12}, {0x370c, 0x00},
    {0x3a02, 0x07}, {0x3a03, 0xb0}, {0x3a14, 0x07}, {0x3a15, 0xb0},
    {0x4004, 0x06}, {0x4837, 0x0a}};

static SENSOR_STATIC_INFO_T s_ov5640_static_info[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .static_info = {.f_num = 200,
                     .focal_length = 354,
                     .max_fps = 0,
                     .max_adgain = 15 * 2,
                     .ois_supported = 0,
                     .pdaf_supported = 0,

#ifdef CONFIG_CAMERA_PDAF_TYPE
                     .pdaf_supported = CONFIG_CAMERA_PDAF_TYPE,
#else
                     .pdaf_supported = 0,
#endif
                     .exp_valid_frame_num = 1,
                     .clamp_level = 64,
                     .adgain_valid_frame_num = 1,
                     .fov_info = {{4.614f, 3.444f}, 4.222f}}}
    /*If there are multiple modules,please add here*/
};

static SENSOR_MODE_FPS_INFO_T s_ov5640_mode_fps_info[VENDOR_NUM] = {
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

static struct sensor_res_tab_info s_ov5640_resolution_Tab_RAW[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .reg_tab = {{ADDR_AND_LEN_OF_ARRAY(ov5640_com_mipi_raw), PNULL, 0,
                  .width = 0, .height = 0, .xclk_to_sensor = 24,
                  .image_format = SENSOR_IMAGE_FORMAT_RAW},

                 {ADDR_AND_LEN_OF_ARRAY(ov5640_1280X960_mipi_raw), PNULL, 0,
                  .width = 1280, .height = 960, .xclk_to_sensor = 24,
                  .image_format = SENSOR_IMAGE_FORMAT_RAW}

     }}
    /*If there are multiple modules,please add here*/
};

static SENSOR_TRIM_T s_ov5640_Resolution_Trim_Tab[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .trim_info = {{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
                   {.trim_start_x = 0,
                    .trim_start_y = 0,
                    .trim_width = 1280,
                    .trim_height = 960,
                    .line_time = 282,
                    .bps_per_lane = 56,
                    .frame_line = 1280,
                    .scaler_trim = {.x = 0, .y = 0, .w = 1280, .h = 960}}}}
    /*If there are multiple modules,please add here*/
};

static struct sensor_module_info s_ov5640_module_info_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .module_info = {.major_i2c_addr = ov5640_I2C_ADDR_W,
                     .minor_i2c_addr = ov5640_I2C_ADDR_R,

                     .reg_addr_value_bits =
                         SENSOR_I2C_REG_16BIT | SENSOR_I2C_REG_8BIT,

                     .avdd_val = SENSOR_AVDD_2800MV,
                     .iovdd_val = SENSOR_AVDD_1800MV,
                     .dvdd_val = SENSOR_AVDD_1500MV,

                     .image_pattern = SENSOR_IMAGE_PATTERN_RAWRGB_GB,

                     .preview_skip_num = 3,
                     .capture_skip_num = 3,
                     .flash_capture_skip_num = 6,
                     .mipi_cap_skip_num = 0,
                     .preview_deci_num = 0,
                     .video_preview_deci_num = 0,

                     .sensor_interface = {.type = SENSOR_INTERFACE_TYPE_CSI2,
                                          .bus_width = 2,
                                          .pixel_width = 10,
                                          .is_loose = 0},

                     .change_setting_skip_num = 3,
                     .horizontal_view_angle = 48,
                     .vertical_view_angle = 48}}

    /*If there are multiple modules,please add here*/
};

static struct sensor_ic_ops s_ov5640_ops_tab;
struct sensor_raw_info *s_ov5640_mipi_raw_info_ptr = &s_ov5640_mipi_raw_info;

SENSOR_INFO_T g_ov5640_mipi_raw_info = {
    .hw_signal_polarity = SENSOR_HW_SIGNAL_PCLK_N | SENSOR_HW_SIGNAL_VSYNC_N |
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
    .identify_code = {{0x0A, 0x56}, {0x0B, 0x40}},

    .source_width_max = 1280,
    .source_height_max = 960,
    .name = (cmr_s8 *)SENSOR_NAME,
    .image_format = SENSOR_IMAGE_FORMAT_RAW,

    .resolution_tab_info_ptr = s_ov5640_resolution_Tab_RAW,
    .sns_ops = &s_ov5640_ops_tab,
    .raw_info_ptr = &s_ov5640_mipi_raw_info_ptr,
    .ext_info_ptr = NULL,
    .module_info_tab = s_ov5640_module_info_tab,
    .module_info_tab_size = ARRAY_SIZE(s_ov5640_module_info_tab),

    .video_tab_info_ptr = NULL,
    .sensor_version_info = (cmr_s8 *)"ov5640v1"};
#endif
