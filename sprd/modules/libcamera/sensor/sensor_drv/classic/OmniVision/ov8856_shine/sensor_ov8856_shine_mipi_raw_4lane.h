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

#ifndef _SENSOR_ov8856_SHINE_MIPI_RAW_H_
#define _SENSOR_ov8856_SHINE_MIPI_RAW_H_

#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"
#include "isp_com.h"
#ifndef _SENSOR_RAW_PIKE2_H_
#include "parameters_sharkle/sensor_ov8856_raw_param_main.c"
#else
#include "paramerers_4lane/sensor_ov8856_raw_param_main.c"
#endif
//#define FEATURE_OTP

#define VENDOR_NUM 1
#define SENSOR_NAME "ov8856"

#define I2C_SLAVE_ADDR 0x6c // 0x20 		/* 8bit slave address*/

#define ov8856_PID_ADDR 0x300B
#define ov8856_PID_VALUE 0x88
#define ov8856_VER_ADDR 0x300C
#define ov8856_VER_VALUE 0x5A

/* sensor parameters begin */

/* effective sensor output image size */
#define VIDEO_WIDTH 1280
#define VIDEO_HEIGHT 720
#define PREVIEW_WIDTH 1632
#define PREVIEW_HEIGHT 1224
#define SNAPSHOT_WIDTH 3264
#define SNAPSHOT_HEIGHT 2448

/*Raw Trim parameters*/
#define VIDEO_TRIM_X 0
#define VIDEO_TRIM_Y 0
#define VIDEO_TRIM_W 1280
#define VIDEO_TRIM_H 720
#define PREVIEW_TRIM_X 0
#define PREVIEW_TRIM_Y 0
#define PREVIEW_TRIM_W 1632
#define PREVIEW_TRIM_H 1224
#define SNAPSHOT_TRIM_X 0
#define SNAPSHOT_TRIM_Y 0
#define SNAPSHOT_TRIM_W 3264
#define SNAPSHOT_TRIM_H 2448

/*Mipi output*/
#define LANE_NUM 4
#define RAW_BITS 10

#define VIDEO_MIPI_PER_LANE_BPS 776    /* 2*Mipi clk */
#define PREVIEW_MIPI_PER_LANE_BPS 776  /* 2*Mipi clk */
#define SNAPSHOT_MIPI_PER_LANE_BPS 776 /* 2*Mipi clk */

/*line time unit: ns*/
#define VIDEO_LINE_TIME 13417
#define PREVIEW_LINE_TIME 26833
#define SNAPSHOT_LINE_TIME 13417

/* frame length*/
#define VIDEO_FRAME_LENGTH 828
#define PREVIEW_FRAME_LENGTH 1246
#define SNAPSHOT_FRAME_LENGTH 2496

/* please ref your spec */
#define FRAME_OFFSET 6
#define SENSOR_MAX_GAIN 0x7c0 // x15.5 A gain
#define SENSOR_BASE_GAIN 0x80
#define SENSOR_MIN_SHUTTER 6

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
/* sensor parameters end */

/* isp parameters, please don't change it*/
#define ISP_BASE_GAIN 0x80

/* please don't change it */
#define EX_MCLK 24
#define IMAGE_H_MIRROR

//#define IMAGE_V_MIRROR //IMAGE_NORMAL_MIRROR; IMAGE_H_MIRROR; IMAGE_V_MIRROR;
// IMAGE_HV_MIRROR

/*==============================================================================
 * Description:
 * register setting
 *============================================================================*/

static const SENSOR_REG_T ov8856_init_setting[] = {
    // Initial setting_4lane_3264x2448_30fps
    {0x0103, 0x01}, {0x0100, 0x00}, {0x0300, 0x04}, {0x0302, 0x61},
    {0x0303, 0x00}, {0x031e, 0x0c}, {0x3000, 0x00}, {0x300e, 0x00},
    {0x3010, 0x00}, {0x3015, 0x84}, {0x3018, 0x72}, {0x3021, 0x23},
    {0x3033, 0x24}, {0x3500, 0x00}, {0x3501, 0x4c}, {0x3502, 0xe0},
    {0x3503, 0x08}, {0x3505, 0x83}, {0x3508, 0x01}, {0x3509, 0x80},
    {0x350c, 0x00}, {0x350d, 0x80}, {0x350e, 0x04}, {0x350f, 0x00},
    {0x3510, 0x00}, {0x3511, 0x02}, {0x3512, 0x00}, {0x3600, 0x72},
    {0x3601, 0x40}, {0x3602, 0x30}, {0x3610, 0xc5}, {0x3611, 0x58},
    {0x3612, 0x5c}, {0x3613, 0xca}, {0x3614, 0x60}, {0x3628, 0xff},
    {0x3629, 0xff}, {0x362a, 0xff}, {0x3633, 0x10}, {0x3634, 0x10},
    {0x3635, 0x10}, {0x3636, 0x10}, {0x3663, 0x08}, {0x3669, 0x34},
    {0x366e, 0x08}, {0x3706, 0x86}, {0x370b, 0x7e}, {0x3714, 0x27},
    {0x3730, 0x12}, {0x3733, 0x10}, {0x3764, 0x00}, {0x3765, 0x00},
    {0x3769, 0x62}, {0x376a, 0x2a}, {0x376b, 0x30}, {0x3780, 0x00},
    {0x3781, 0x24}, {0x3782, 0x00}, {0x3783, 0x23}, {0x3798, 0x2f},
    {0x37a1, 0x60}, {0x37a8, 0x6a}, {0x37ab, 0x3f}, {0x37c2, 0x14},
    {0x37c3, 0xf1}, {0x37c9, 0x80}, {0x37cb, 0x16}, {0x37cc, 0x16},
    {0x37cd, 0x16}, {0x37ce, 0x16}, {0x3800, 0x00}, {0x3801, 0x00},
    {0x3802, 0x00}, {0x3803, 0x0c}, {0x3804, 0x0c}, {0x3805, 0xdf},
    {0x3806, 0x09}, {0x3807, 0xa3}, {0x3808, 0x06}, {0x3809, 0x60},
    {0x380a, 0x04}, {0x380b, 0xc8}, {0x380c, 0x0f}, {0x380d, 0x18},
    {0x380e, 0x04}, {0x380f, 0xde}, {0x3810, 0x00}, {0x3811, 0x08},
    {0x3812, 0x00}, {0x3813, 0x02}, {0x3814, 0x03}, {0x3815, 0x01},
    {0x3816, 0x00}, {0x3817, 0x00}, {0x3818, 0x00}, {0x3819, 0x00},
    {0x3820, 0x90}, {0x3821, 0x67}, {0x382a, 0x03}, {0x382b, 0x01},
    {0x3830, 0x06}, {0x3836, 0x02}, {0x3862, 0x04}, {0x3863, 0x08},
    {0x3cc0, 0x33}, {0x3d85, 0x17}, {0x3d8c, 0x73}, {0x3d8d, 0xde},
    {0x4001, 0xe0}, {0x4003, 0x40}, {0x4008, 0x00}, {0x4009, 0x05},
    {0x400a, 0x00}, {0x400b, 0x84}, {0x400f, 0x80}, {0x4010, 0xf0},
    {0x4011, 0xff}, {0x4012, 0x02}, {0x4013, 0x01}, {0x4014, 0x01},
    {0x4015, 0x01}, {0x4042, 0x00}, {0x4043, 0x80}, {0x4044, 0x00},
    {0x4045, 0x80}, {0x4046, 0x00}, {0x4047, 0x80}, {0x4048, 0x00},
    {0x4049, 0x80}, {0x4041, 0x03}, {0x404c, 0x20}, {0x404d, 0x00},
    {0x404e, 0x20}, {0x4203, 0x80}, {0x4307, 0x30}, {0x4317, 0x00},
    {0x4503, 0x08}, {0x4601, 0x80}, {0x4800, 0x44}, {0x4816, 0x53},
    {0x481b, 0x58}, {0x481f, 0x27}, {0x4837, 0x14}, {0x483c, 0x0f},
    {0x484b, 0x05}, {0x5000, 0x77}, {0x5001, 0x0a}, {0x5004, 0x04},
    {0x502e, 0x03}, {0x5030, 0x41}, {0x5795, 0x00}, {0x5796, 0x10},
    {0x5797, 0x10}, {0x5798, 0x73}, {0x5799, 0x73}, {0x579a, 0x00},
    {0x579b, 0x28}, {0x579c, 0x00}, {0x579d, 0x16}, {0x579e, 0x06},
    {0x579f, 0x20}, {0x57a0, 0x04}, {0x57a1, 0xa0}, {0x5780, 0x14},
    {0x5781, 0x0f}, {0x5782, 0x44}, {0x5783, 0x02}, {0x5784, 0x01},
    {0x5785, 0x01}, {0x5786, 0x00}, {0x5787, 0x04}, {0x5788, 0x02},
    {0x5789, 0x0f}, {0x578a, 0xfd}, {0x578b, 0xf5}, {0x578c, 0xf5},
    {0x578d, 0x03}, {0x578e, 0x08}, {0x578f, 0x0c}, {0x5790, 0x08},
    {0x5791, 0x04}, {0x5792, 0x00}, {0x5793, 0x52}, {0x5794, 0xa3},
    {0x59f8, 0x3d}, {0x5a08, 0x02}, {0x5b00, 0x02}, {0x5b01, 0x10},
    {0x5b02, 0x03}, {0x5b03, 0xcf}, {0x5b05, 0x6c}, {0x5e00, 0x00},
//{0x0100, 0x01},
#ifdef FEATURE_OTP // enable lsc otp
// otp LSC_en
#else
// otp LSC_disable
#endif
};

static const SENSOR_REG_T ov8856_preview_setting[] = {
    // OV8856 4lane 1632x1224 30fps
    // line time:1932/144M(pclk) 13416.66666 frame length:2482
    {0x0100, 0x00}, {0x3501, 0x4c}, {0x3502, 0xe0}, {0x366e, 0x08},
    {0x3714, 0x27}, {0x37c2, 0x14}, {0x3801, 0x00}, {0x3802, 0x00},
    {0x3803, 0x0c}, {0x3804, 0x0c}, {0x3805, 0xdf}, {0x3806, 0x09},
    {0x3807, 0xa3}, {0x3808, 0x06}, {0x3809, 0x60}, {0x380a, 0x04},
    {0x380b, 0xc8}, {0x380c, 0x0f}, {0x380d, 0x18}, {0x380e, 0x04},
    {0x380f, 0xde}, {0x3811, 0x08}, {0x3813, 0x02}, {0x3814, 0x03},
    {0x3820, 0x90}, {0x3821, 0x67}, {0x382a, 0x03}, {0x4009, 0x05},
    {0x4837, 0x14}, {0x5795, 0x00}, {0x5796, 0x10}, {0x5797, 0x10},
    {0x5798, 0x73}, {0x5799, 0x73}, {0x579b, 0x28}, {0x579d, 0x16},
    {0x579e, 0x06}, {0x579f, 0x20}, {0x57a0, 0x04}, {0x57a1, 0xa0},
//{0x0100, 0x01},
#ifdef IMAGE_NORMAL_MIRROR
    {0x3820, 0x90}, {0x3821, 0x61}, {0x502e, 0x03}, {0x5001, 0x0a},
    {0x5004, 0x00}, {0x376b, 0x30},
#endif
#ifdef IMAGE_H_MIRROR
    {0x3820, 0x90}, {0x3821, 0x67}, {0x502e, 0x03}, {0x5001, 0x0a},
    {0x5004, 0x04}, {0x376b, 0x30},
#endif
#ifdef IMAGE_V_MIRROR
    {0x3820, 0xd6}, {0x3821, 0x61}, {0x502e, 0x00}, {0x5001, 0x0e},
    {0x5004, 0x00}, {0x376b, 0x36},
#endif
#ifdef IMAGE_HV_MIRROR
    {0x3820, 0xd6}, {0x3821, 0x67}, {0x502e, 0x00}, {0x5001, 0x0e},
    {0x5004, 0x04}, {0x376b, 0x36},
#endif
};

static const SENSOR_REG_T ov8856_snapshot_setting[] = {
    // OV8856 4lane 3264x2448 30fps
    // line time:1932/144M(pclk) 13416.66666 frame length:2496
    {0x0100, 0x00}, {0x3501, 0x9a}, {0x3502, 0x20}, {0x366e, 0x10},
    {0x3714, 0x23}, {0x37c2, 0x04}, {0x3801, 0x00}, {0x3802, 0x00},
    {0x3803, 0x0c}, {0x3804, 0x0c}, {0x3805, 0xdf}, {0x3806, 0x09},
    {0x3807, 0xa3}, {0x3808, 0x0c}, {0x3809, 0xc0}, {0x380a, 0x09},
    {0x380b, 0x90}, {0x380c, 0x07}, {0x380d, 0x8c}, {0x380e, 0x09},
    {0x380f, 0xc0}, {0x3811, 0x10}, {0x3813, 0x04}, {0x3814, 0x01},
    {0x3820, 0x80}, {0x3821, 0x46}, {0x382a, 0x01}, {0x4009, 0x0b},
    {0x4837, 0x14}, {0x5795, 0x02}, {0x5796, 0x20}, {0x5797, 0x20},
    {0x5798, 0xd5}, {0x5799, 0xd5}, {0x579b, 0x50}, {0x579d, 0x2c},
    {0x579e, 0x0c}, {0x579f, 0x40}, {0x57a0, 0x09}, {0x57a1, 0x40},
//{0x0100, 0x01},
// Full Size 8M
#ifdef IMAGE_NORMAL_MIRROR
    {0x3820, 0x80}, {0x3821, 0x40}, {0x502e, 0x03}, {0x5001, 0x0a},
    {0x5004, 0x00}, {0x376b, 0x30},
#endif
#ifdef IMAGE_H_MIRROR
    {0x3820, 0x80}, {0x3821, 0x46}, {0x502e, 0x03}, {0x5001, 0x0a},
    {0x5004, 0x04}, {0x376b, 0x30},
#endif
#ifdef IMAGE_V_MIRROR
    {0x3820, 0x80}, {0x3821, 0x46}, {0x502e, 0x03}, {0x5001, 0x0a},
    {0x5004, 0x04}, {0x376b, 0x30},
#endif
#ifdef IMAGE_HV_MIRROR
    {0x3820, 0x80}, {0x3821, 0x46}, {0x502e, 0x03}, {0x5001, 0x0a},
    {0x5004, 0x04}, {0x376b, 0x30},
#endif
};

static const SENSOR_REG_T ov8856_video_setting[] = {
    // Video_OV8856 4lane 1280x720 90fps
    // line time:1932/144M(pclk) 13416.66666 frame length:828
    {0x0100, 0x00}, {0x3501, 0x32}, {0x3502, 0xc0}, {0x366e, 0x08},
    {0x3714, 0x27}, {0x37c2, 0x14}, {0x3801, 0xe0}, {0x3802, 0x01},
    {0x3803, 0xbc}, {0x3804, 0x0b}, {0x3805, 0xff}, {0x3806, 0x07},
    {0x3807, 0xf3}, {0x3808, 0x05}, {0x3809, 0x00}, {0x380a, 0x02},
    {0x380b, 0xd0}, {0x380c, 0x07}, {0x380d, 0x8c}, {0x380e, 0x03},
    {0x380f, 0x3c}, {0x3811, 0x48}, {0x3813, 0x26}, {0x3814, 0x03},
    {0x3820, 0x90}, {0x3821, 0x67}, {0x382a, 0x03}, {0x4009, 0x05},
    {0x4837, 0x16}, {0x5795, 0x00}, {0x5796, 0x10}, {0x5797, 0x10},
    {0x5798, 0x73}, {0x5799, 0x73}, {0x579b, 0x00}, {0x579d, 0x00},
    {0x579e, 0x05}, {0x579f, 0xa0}, {0x57a0, 0x03}, {0x57a1, 0x20},
//{0x0100, 0x01},
#ifdef IMAGE_NORMAL_MIRROR
    {0x3820, 0x90}, {0x3821, 0x61}, {0x502e, 0x03}, {0x5001, 0x0a},
    {0x5004, 0x00}, {0x376b, 0x30},
#endif
#ifdef IMAGE_H_MIRROR
    {0x3820, 0x90}, {0x3821, 0x67}, {0x502e, 0x03}, {0x5001, 0x0a},
    {0x5004, 0x04}, {0x376b, 0x30},
#endif
#ifdef IMAGE_V_MIRROR
    {0x3820, 0xd6}, {0x3821, 0x61}, {0x502e, 0x00}, {0x5001, 0x0e},
    {0x5004, 0x00}, {0x376b, 0x36},
#endif
#ifdef IMAGE_HV_MIRROR
    {0x3820, 0xd6}, {0x3821, 0x67}, {0x502e, 0x00}, {0x5001, 0x0e},
    {0x5004, 0x04}, {0x376b, 0x36},
#endif
};

static struct sensor_res_tab_info s_ov8856_resolution_tab_raw[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .reg_tab =
         {{ADDR_AND_LEN_OF_ARRAY(ov8856_init_setting), PNULL, 0, .width = 0,
           .height = 0, .xclk_to_sensor = EX_MCLK,
           .image_format = SENSOR_IMAGE_FORMAT_RAW},

          {ADDR_AND_LEN_OF_ARRAY(ov8856_video_setting), PNULL, 0,
           .width = VIDEO_WIDTH, .height = VIDEO_HEIGHT,
           .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW},

          {ADDR_AND_LEN_OF_ARRAY(ov8856_preview_setting), PNULL, 0,
           .width = PREVIEW_WIDTH, .height = PREVIEW_HEIGHT,
           .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW},

          {ADDR_AND_LEN_OF_ARRAY(ov8856_snapshot_setting), PNULL, 0,
           .width = SNAPSHOT_WIDTH, .height = SNAPSHOT_HEIGHT,
           .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW}}}

    /*If there are multiple modules,please add here*/
};

static SENSOR_TRIM_T s_ov8856_resolution_trim_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .trim_info =
         {
             {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},

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
         }}

    /*If there are multiple modules,please add here*/

};

static SENSOR_REG_T ov8856_shutter_reg[] = {
    {0x3502, 0x00}, {0x3501, 0x00}, {0x3500, 0x00},
};

static struct sensor_i2c_reg_tab ov8856_shutter_tab = {
    .settings = ov8856_shutter_reg, .size = ARRAY_SIZE(ov8856_shutter_reg),
};

static SENSOR_REG_T ov8856_again_reg[] = {
    {0x320a, 0x01}, {0x3208, 0x01},

    {0x3508, 0x00}, {0x3509, 0x00},

    //{0x3208, 0x11}, {0x3208, 0xA1},
};

static struct sensor_i2c_reg_tab ov8856_again_tab = {
    .settings = ov8856_again_reg, .size = ARRAY_SIZE(ov8856_again_reg),
};

static SENSOR_REG_T ov8856_dgain_reg[] = {
    {0x5019, 0x00}, {0x501a, 0x00}, {0x501b, 0x00}, {0x501c, 0x00},
    {0x501d, 0x00}, {0x501e, 0x00}, {0x501f, 0x00}, {0x5020, 0x00},

    {0x3208, 0x11}, {0x3208, 0xA1},

};

static struct sensor_i2c_reg_tab ov8856_dgain_tab = {
    .settings = ov8856_dgain_reg, .size = ARRAY_SIZE(ov8856_dgain_reg),
};

static SENSOR_REG_T ov8856_frame_length_reg[] = {
    {0x380e, 0x00}, {0x380f, 0x00},
};

static struct sensor_i2c_reg_tab ov8856_frame_length_tab = {
    .settings = ov8856_frame_length_reg,
    .size = ARRAY_SIZE(ov8856_frame_length_reg),
};

static struct sensor_aec_i2c_tag ov8856_aec_info = {
    .slave_addr = (I2C_SLAVE_ADDR >> 1),
    .addr_bits_type = SENSOR_I2C_REG_16BIT,
    .data_bits_type = SENSOR_I2C_VAL_8BIT,
    .shutter = &ov8856_shutter_tab,
    .again = &ov8856_again_tab,
    .dgain = &ov8856_dgain_tab,
    .frame_length = &ov8856_frame_length_tab,
};

static SENSOR_STATIC_INFO_T s_ov8856_static_info[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .static_info = {.f_num = 200,
                     .focal_length = 354,
                     .max_fps = 30,
                     .max_adgain = 62,
                     .ois_supported = 0,
                     .pdaf_supported = 0,
                     .exp_valid_frame_num = 1,
                     .clamp_level = 64,
                     .adgain_valid_frame_num = 0,
                     .fov_info = {{4.614f, 3.444f}, 4.222f}}}
    /*If there are multiple modules,please add here*/
};

static SENSOR_MODE_FPS_INFO_T s_ov8856_mode_fps_info[VENDOR_NUM] = {
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

static struct sensor_module_info s_ov8856_module_info_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .module_info = {.major_i2c_addr = I2C_SLAVE_ADDR >> 1,
                     .minor_i2c_addr = I2C_SLAVE_ADDR >> 1,

                     .reg_addr_value_bits = SENSOR_I2C_REG_16BIT |
                                            SENSOR_I2C_VAL_8BIT |
                                            SENSOR_I2C_FREQ_400,

                     .avdd_val = SENSOR_AVDD_2800MV,
                     .iovdd_val = SENSOR_AVDD_1800MV,
                     .dvdd_val = SENSOR_AVDD_1200MV,

                     .image_pattern = SENSOR_IMAGE_PATTERN_RAWRGB_B,

                     .preview_skip_num = 0,
                     .capture_skip_num = 0,
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

static struct sensor_ic_ops s_ov8856_ops_tab;
struct sensor_raw_info *s_ov8856_shine_mipi_raw_info_ptr =
    &s_ov8856_mipi_raw_info;

/*==============================================================================
 * Description:
 * sensor all info
 * please modify this variable acording your spec
 *============================================================================*/
SENSOR_INFO_T g_ov8856_shine_mipi_raw_info = {
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
    .identify_code = {{.reg_addr = ov8856_PID_ADDR,
                       .reg_value = ov8856_PID_VALUE},
                      {.reg_addr = ov8856_VER_ADDR,
                       .reg_value = ov8856_VER_VALUE}},

    .source_width_max = SNAPSHOT_WIDTH,
    .source_height_max = SNAPSHOT_HEIGHT,
    .name = (cmr_s8 *)SENSOR_NAME,
    .image_format = SENSOR_IMAGE_FORMAT_RAW,

    .module_info_tab = s_ov8856_module_info_tab,
    .module_info_tab_size = ARRAY_SIZE(s_ov8856_module_info_tab),

    .resolution_tab_info_ptr = s_ov8856_resolution_tab_raw,
    .sns_ops = &s_ov8856_ops_tab,
    .raw_info_ptr = &s_ov8856_shine_mipi_raw_info_ptr,

    .video_tab_info_ptr = NULL,
    .sensor_version_info = (cmr_s8 *)"ov8856_v1",
};

#endif
