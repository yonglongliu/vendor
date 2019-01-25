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
#ifndef _SENSOR_OV5675_MIPI_RAW_H_
#define _SENSOR_OV5675_MIPI_RAW_H_

#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"

#if defined(CONFIG_CAMERA_ISP_VERSION_V3) ||                                   \
    defined(CONFIG_CAMERA_ISP_VERSION_V4)
#include "parameters/sensor_ov5675_dual_raw_param_main.c"
#else
#include "sensor_ov5675_dual_raw_param.c"
#endif

#define VENDOR_NUM 2

#define CAMERA_IMAGE_180

#define SENSOR_NAME "ov5675_mipi_raw"
#if defined(CONFIG_DUAL_CAMERA)
#define I2C_SLAVE_ADDR 0x6c /* 8bit slave address*/
#else
#define I2C_SLAVE_ADDR 0x20 /* 8bit slave address*/
#endif

#define ov5675_PID_ADDR 0x300B
#define ov5675_PID_VALUE 0x56
#define ov5675_VER_ADDR 0x300C
#define ov5675_VER_VALUE 0x75

/* sensor parameters begin */
/* effective sensor output image size */
#define SNAPSHOT_WIDTH 2592
#define SNAPSHOT_HEIGHT 1944
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

/*Mipi output*/
#define LANE_NUM 2
#define RAW_BITS 10

#define SNAPSHOT_MIPI_PER_LANE_BPS 900 /* 2*Mipi clk */ // 738
#define PREVIEW_MIPI_PER_LANE_BPS 900 /* 2*Mipi clk */  // 738

/*line time unit: 1us*/
#define SNAPSHOT_LINE_TIME 16700 // 20833
#define PREVIEW_LINE_TIME 16700  // 33333

/* frame length*/
#define SNAPSHOT_FRAME_LENGTH 2000
#define PREVIEW_FRAME_LENGTH 2000

/* please ref your spec */
#define FRAME_OFFSET 4
#define SENSOR_MAX_GAIN 0x7c0
#define SENSOR_BASE_GAIN 0x80
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
#if defined(CONFIG_CAMERA_ISP_VERSION_V3) ||                                   \
    defined(CONFIG_CAMERA_ISP_VERSION_V4)
#define ISP_BASE_GAIN 0x80
#else
#define ISP_BASE_GAIN 0x10
#endif
/* please don't change it */
#define EX_MCLK 24

static struct sensor_ic_ops s_ov5675_ops_tab;
struct sensor_raw_info *s_ov5675_dual_mipi_raw_info_ptr =
    &s_ov5675_dual_mipi_raw_info;


static const SENSOR_REG_T ov5675_init_setting[] = {
    {0x0100, 0x00},
    {0x0103, 0x01},
    {0x0300, 0x05},
    {0x0302, 0x96},
    {0x0303, 0x00},
    {0x030d, 0x1e},
    {0x3002, 0x21},
    {0x3107, 0x01}, // 0x23->0x01
    {0x3501, 0x20},
    {0x3503, 0x0c},
    {0x3508, 0x03},
    {0x3509, 0x00},
    {0x3600, 0x66},
    {0x3602, 0x30},
    {0x3610, 0xa5},
    {0x3612, 0x93},
    {0x3620, 0x80},
    {0x3642, 0x0e},
    {0x3661, 0x00},
    {0x3662, 0x10},
    {0x3664, 0xf3},
    {0x3665, 0x9e},
    {0x3667, 0xa5},
    {0x366e, 0x55},
    {0x366f, 0x55},
    {0x3670, 0x11},
    {0x3671, 0x11},
    {0x3672, 0x11},
    {0x3673, 0x11},
    {0x3714, 0x24},
    {0x371a, 0x3e},
    {0x3733, 0x10},
    {0x3734, 0x00},
    {0x373d, 0x24},
    {0x3764, 0x20},
    {0x3765, 0x20},
    {0x3766, 0x12},
    {0x37a1, 0x14},
    {0x37a8, 0x1c},
    {0x37ab, 0x0f},
    {0x37c2, 0x04},
    {0x37cb, 0x00},
    {0x37cc, 0x00},
    {0x37cd, 0x00},
    {0x37ce, 0x00},
    {0x37d8, 0x02},
    {0x37d9, 0x08},
    {0x37dc, 0x04},
    {0x3800, 0x00},
    {0x3801, 0x00},
    {0x3802, 0x00},
    {0x3803, 0x04},
    {0x3804, 0x0a},
    {0x3805, 0x3f},
    {0x3806, 0x07},
    {0x3807, 0xb3},
    {0x3808, 0x0a},
    {0x3809, 0x20},
    {0x380a, 0x07},
    {0x380b, 0x98},
    {0x380c, 0x02},
    {0x380d, 0xee},
    {0x380e, 0x07},
    {0x380f, 0xd0},
    {0x3811, 0x10},
    {0x3813, 0x0c},
    {0x3814, 0x01},
    {0x3815, 0x01},
    {0x3816, 0x01},
    {0x3817, 0x01},
    {0x381e, 0x02},
    {0x3820, 0x88},
    {0x3821, 0x01},
    {0x3832, 0x04},
    {0x3c80, 0x08}, // 0x01->0x08
    {0x3c82, 0x00},
    {0x3c83, 0xb1}, // 0xc8->0xb1
    {0x3c8c, 0x10}, // 0x0f->0x10
    {0x3c8d, 0x00}, // 0xa0->0x00
    {0x3c90, 0x00}, // 0x07->0x00
    {0x3c91, 0x00},
    {0x3c92, 0x00},
    {0x3c93, 0x00},
    {0x3c94, 0x00}, // 0xd0->0x00
    {0x3c95, 0x00}, // 0x50->0x00
    {0x3c96, 0x00}, // 0x35->0x00
    {0x3c97, 0x00},
    {0x4001, 0xe0},
    {0x4008, 0x02},
    {0x4009, 0x0d},
    {0x400f, 0x80},
    {0x4013, 0x02},
    {0x4040, 0x00},
    {0x4041, 0x07},
    {0x404c, 0x50},
    {0x404e, 0x20},
    {0x4500, 0x06},
    {0x4503, 0x00},
    {0x450a, 0x04},
    {0x4809, 0x04},
    {0x480c, 0x12},
    {0x4819, 0x70},
    {0x4825, 0x32},
    {0x4826, 0x32},
    {0x482a, 0x06},
    {0x4833, 0x08},
    {0x4837, 0x0d},
    {0x5000, 0x77},
    {0x5b00, 0x01},
    {0x5b01, 0x10},
    {0x5b02, 0x01},
    {0x5b03, 0xdb},
    {0x5b05, 0x6c},
    {0x5e10, 0xfc},
    {0x3500, 0x00},
    {0x3501, 0x3E}, // ;max expo= ([380e,380f]-4)/2.
    {0x3502, 0x60},
    {0x3503, 0x78}, // ;Bit[6:4]=1, AE1 do not delay frame, [2]=0 real gain
    {0x3508, 0x04},
    {0x3509, 0x00}, // ;[3508,3509]=0x0080 is 1xgain
    {0x3832, 0x48}, // ; [7:4]vsync_width ; R3002[5] p_fsin_oen
    {0x3c90, 0x00}, // ;MIPI Continuous mode (07 Gated mode)
    {0x5780, 0x3e}, // ;strong DPC
    {0x5781, 0x0f},
    {0x5782, 0x44},
    {0x5783, 0x02},
    {0x5784, 0x01},
    {0x5785, 0x01},
    {0x5786, 0x00},
    {0x5787, 0x04},
    {0x5788, 0x02},
    {0x5789, 0x0f},
    {0x578a, 0xfd},
    {0x578b, 0xf5},
    {0x578c, 0xf5},
    {0x578d, 0x03},
    {0x578e, 0x08},
    {0x578f, 0x0c},
    {0x5790, 0x08},
    {0x5791, 0x06},
    {0x5792, 0x00},
    {0x5793, 0x52},
    {0x5794, 0xa3},
    {0x4003, 0x40} //;BLC target
};

static const SENSOR_REG_T ov5675_preview_setting[] = {
    /* Common setting */
    {0x3662, 0x08}, {0x3714, 0x28}, {0x371a, 0x3e}, {0x37c2, 0x14},
    {0x37d9, 0x04}, {0x3800, 0x00}, {0x3801, 0x00}, {0x3802, 0x00},
    {0x3803, 0x00}, {0x3804, 0x0a}, {0x3805, 0x3f}, {0x3806, 0x07},
    {0x3807, 0xb7}, {0x3808, 0x05}, {0x3809, 0x10}, {0x380a, 0x03},
    {0x380b, 0xcc}, {0x380c, 0x02}, {0x380d, 0xee}, {0x380e, 0x07},
    {0x380f, 0xd0}, {0x3811, 0x08}, {0x3813, 0x08}, {0x3814, 0x03},
    {0x3815, 0x01}, {0x3816, 0x03},
#ifndef CAMERA_IMAGE_180
    {0x3820, 0x96}, // vsyn48_blc on, vflip on
    {0x3821, 0x41}, // hsync_en_o, mirror off, dig_bin on
    {0x450b, 0x20}, // need to set when flip
#else
    {0x3820, 0x90}, // vsyn48_blc on, vflip off
    {0x3821, 0x47}, // hsync_en_o, mirror on, dig_bin on
#endif
    {0x3821, 0x01}, {0x4008, 0x00}, {0x4009, 0x07}, {0x4041, 0x03}};

static const SENSOR_REG_T ov5675_snapshot_setting[] = {
    {0x3662, 0x10}, {0x3714, 0x24}, {0x371a, 0x3e}, {0x37c2, 0x04},
    {0x37d9, 0x08}, {0x3800, 0x00}, {0x3801, 0x00}, {0x3802, 0x00},
    {0x3803, 0x04}, {0x3804, 0x0a}, {0x3805, 0x3f}, {0x3806, 0x07},
    {0x3807, 0xb3}, {0x3808, 0x0a}, {0x3809, 0x20}, {0x380a, 0x07},
    {0x380b, 0x98}, {0x380c, 0x02}, {0x380d, 0xee}, {0x380e, 0x07},
    {0x380f, 0xd0}, {0x3811, 0x10}, {0x3813, 0x0c}, {0x3814, 0x01},
    {0x3815, 0x01}, {0x3816, 0x01},
#ifndef CAMERA_IMAGE_180
    {0x3820, 0x96}, // vflip on
    {0x3821, 0x41}, // hsync_en_o, mirror off, dig_bin off
    {0x450b, 0x20}, // need to set when flip
#else
    {0x3820, 0x90}, // vflip off
    {0x3821, 0x47}, // hsync_en_o, mirror on, dig_bin off
#endif
    {0x3821, 0x01}, {0x4008, 0x02}, {0x4009, 0x0d}, {0x4041, 0x07}};

static const SENSOR_REG_T ov5675_VGA_setting[] = {
    {0x3501, 0x0f}, {0x3502, 0x80}, {0x3662, 0x08}, {0x3714, 0x24},
    {0x371a, 0x3f}, {0x37c2, 0x24}, {0x37d9, 0x04}, {0x3800, 0x00},
    {0x3801, 0x00}, {0x3802, 0x00}, {0x3803, 0x10}, {0x3804, 0x0a},
    {0x3805, 0x3f}, {0x3806, 0x07}, {0x3807, 0xaf}, {0x3808, 0x02},
    {0x3809, 0x80}, {0x380a, 0x01}, {0x380b, 0xe0}, {0x380c, 0x0b},
    {0x380d, 0xb8}, {0x380e, 0x01}, {0x380f, 0xf4}, {0x3811, 0x08},
    {0x3813, 0x02}, {0x3814, 0x07}, {0x3815, 0x01}, {0x3816, 0x07},
#ifdef CAMERA_IMAGE_180
    {0x3820, 0xb5}, {0x373d, 0x26},
#else
    {0x3820, 0x8d}, {0x373d, 0x24},
#endif
    {0x3821, 0x00}, {0x4008, 0x00}, {0x4009, 0x03}, {0x4041, 0x03},

};

/****sharkle new setting,change mipi clock,line_time,frame_length etc****/
static const SENSOR_REG_T ov5675_init_setting1[] = {
    {0x0100, 0x00}, {0x0103, 0x01}, {0x0300, 0x05}, {0x0302, 0x97},
    {0x0303, 0x00}, {0x030d, 0x1e}, {0x3002, 0x21}, {0x3107, 0x01},
    {0x3501, 0x20}, {0x3503, 0x0c}, {0x3508, 0x03}, {0x3509, 0x00},
    {0x3600, 0x66}, {0x3602, 0x30}, {0x3610, 0xa5}, {0x3612, 0x93},
    {0x3620, 0x80}, {0x3642, 0x0e}, {0x3661, 0x00}, {0x3662, 0x10},
    {0x3664, 0xf3}, {0x3665, 0x9e}, {0x3667, 0xa5}, {0x366e, 0x55},
    {0x366f, 0x55}, {0x3670, 0x11}, {0x3671, 0x11}, {0x3672, 0x11},
    {0x3673, 0x11}, {0x3714, 0x24}, {0x371a, 0x3e}, {0x3733, 0x10},
    {0x3734, 0x00}, {0x373d, 0x24}, {0x3764, 0x20}, {0x3765, 0x20},
    {0x3766, 0x12}, {0x37a1, 0x14}, {0x37a8, 0x1c}, {0x37ab, 0x0f},
    {0x37c2, 0x04}, {0x37cb, 0x00}, {0x37cc, 0x00}, {0x37cd, 0x00},
    {0x37ce, 0x00}, {0x37d8, 0x02}, {0x37d9, 0x08}, {0x37dc, 0x04},
    {0x3800, 0x00}, {0x3801, 0x00}, {0x3802, 0x00}, {0x3803, 0x04},
    {0x3804, 0x0a}, {0x3805, 0x3f}, {0x3806, 0x07}, {0x3807, 0xb3},
    {0x3808, 0x0a}, {0x3809, 0x20}, {0x380a, 0x07}, {0x380b, 0x98},
    {0x380c, 0x02}, {0x380d, 0xee}, {0x380e, 0x07}, {0x380f, 0xd0},
    {0x3811, 0x10}, {0x3813, 0x0c}, {0x3814, 0x01}, {0x3815, 0x01},
    {0x3816, 0x01}, {0x3817, 0x01}, {0x381e, 0x02}, {0x3820, 0x88},
    {0x3821, 0x01}, {0x3832, 0x04}, {0x3c80, 0x08}, {0x3c82, 0x00},
    {0x3c83, 0xb1}, {0x3c8c, 0x10}, {0x3c8d, 0x00}, {0x3c90, 0x00},
    {0x3c91, 0x00}, {0x3c92, 0x00}, {0x3c93, 0x00}, {0x3c94, 0x00},
    {0x3c95, 0x00}, {0x3c96, 0x00}, {0x3c97, 0x00}, {0x4001, 0xe0},
    {0x4008, 0x02}, {0x4009, 0x0d}, {0x400f, 0x80}, {0x4013, 0x02},
    {0x4040, 0x00}, {0x4041, 0x07}, {0x404c, 0x50}, {0x404e, 0x20},
    {0x4500, 0x06}, {0x4503, 0x00}, {0x450a, 0x04}, {0x4809, 0x04},
    {0x480c, 0x12}, {0x4819, 0x70}, {0x4825, 0x32}, {0x4826, 0x32},
    {0x482a, 0x06}, {0x4833, 0x08}, {0x4837, 0x0d}, {0x5000, 0x77},
    {0x5b00, 0x01}, {0x5b01, 0x10}, {0x5b02, 0x01}, {0x5b03, 0xdb},
    {0x5b05, 0x6c}, {0x5e10, 0xfc}, {0x3500, 0x00}, {0x3501, 0x3E},
    {0x3502, 0x60}, {0x3503, 0x78}, {0x3508, 0x04}, {0x3509, 0x00},
    {0x3832, 0x48}, {0x3c90, 0x00}, {0x5780, 0x3e}, {0x5781, 0x0f},
    {0x5782, 0x44}, {0x5783, 0x02}, {0x5784, 0x01}, {0x5785, 0x01},
    {0x5786, 0x00}, {0x5787, 0x04}, {0x5788, 0x02}, {0x5789, 0x0f},
    {0x578a, 0xfd}, {0x578b, 0xf5}, {0x578c, 0xf5}, {0x578d, 0x03},
    {0x578e, 0x08}, {0x578f, 0x0c}, {0x5790, 0x08}, {0x5791, 0x06},
    {0x5792, 0x00}, {0x5793, 0x52}, {0x5794, 0xa3}, {0x4003, 0x40},
    {0x3d8c, 0x71}, {0x3d8d, 0xE7}, {0x37cb, 0x09}, {0x37cc, 0x15},
    {0x37cd, 0x1f}, {0x37ce, 0x1f},
    //{0x0100, 0x01},
};

static const SENSOR_REG_T ov5675_preview_setting1[] = {
    //@@ 1296x972_30FPS_MIPI_2_LANE_906Mbps

    /* Common setting */
    {0x0100, 0x00}, {0x030e, 0x05}, {0x3016, 0x32}, {0x3106, 0x15},
    {0x3501, 0x1f}, {0x3662, 0x08}, {0x3714, 0x28}, {0x371a, 0x3e},
    {0x37c2, 0x14}, {0x37d9, 0x04}, {0x3803, 0x00}, {0x3807, 0xb7},
    {0x3808, 0x05}, {0x3809, 0x10}, {0x380a, 0x03}, {0x380b, 0xcc},
    {0x380c, 0x05}, {0x380d, 0xdc}, {0x380e, 0x03}, {0x380f, 0xe8},
    {0x3811, 0x08}, {0x3813, 0x08}, {0x3814, 0x03}, {0x3816, 0x03},
#ifndef CAMERA_IMAGE_180
    {0x3820, 0x8b}, {0x373d, 0x24},
#else
    {0x3820, 0xb3}, {0x373d, 0x26},
#endif
    {0x3821, 0x01}, {0x4008, 0x00}, {0x4009, 0x07}, {0x4041, 0x03},
    {0x3502, 0x20},
    //{0x0100, 0x01},
};

static const SENSOR_REG_T ov5675_VGA_setting1[] = {
    {0x0100, 0x00}, {0x030e, 0x05}, {0x3016, 0x32}, {0x3106, 0x15},
    {0x3501, 0x0f}, {0x3662, 0x08}, {0x3714, 0x24}, {0x371a, 0x3f},
    {0x37c2, 0x24}, {0x37d9, 0x04}, {0x3803, 0x10}, {0x3807, 0xaf},
    {0x3808, 0x02}, {0x3809, 0x80}, {0x380a, 0x01}, {0x380b, 0xe0},
    {0x380c, 0x0b}, {0x380d, 0xb8}, {0x380e, 0x01}, {0x380f, 0xf4},
    {0x3811, 0x08}, {0x3813, 0x02}, {0x3814, 0x07}, {0x3816, 0x07},
#ifndef CAMERA_IMAGE_180
    {0x3820, 0x8d}, {0x373d, 0x24},
#else
    {0x3820, 0xb5}, {0x373d, 0x26},
#endif
    {0x3821, 0x00}, {0x4008, 0x00}, {0x4009, 0x03}, {0x4041, 0x03},
    {0x3502, 0x80},
    //{0x0100, 0x01},
};

static const SENSOR_REG_T ov5675_snapshot_setting1[] = {
    //@@ 2592X1944_24FPS_MIPI_2_LANE_906Mbps
    /*Common setting*/
    {0x0100, 0x00}, {0x030e, 0x05}, {0x3016, 0x32}, {0x3106, 0x15},
    {0x3501, 0x3e}, {0x3662, 0x10}, {0x3714, 0x24}, {0x371a, 0x3e},
    {0x37c2, 0x04}, {0x37d9, 0x08}, {0x3803, 0x04}, {0x3807, 0xb3},
    {0x3808, 0x0a}, {0x3809, 0x20}, {0x380a, 0x07}, {0x380b, 0x98},
    {0x380c, 0x02}, {0x380d, 0xee}, {0x380e, 0x07}, {0x380f, 0xd0},
    {0x3811, 0x10}, {0x3813, 0x0c}, {0x3814, 0x01}, {0x3816, 0x01},
#ifndef CAMERA_IMAGE_180
    {0x3820, 0x88}, {0x373d, 0x24},
#else
    {0x3820, 0xb0}, {0x373d, 0x26},
#endif
    {0x3821, 0x01}, {0x4008, 0x02}, {0x4009, 0x0d}, {0x4041, 0x07},
    {0x3502, 0x60},
    //{0x0100, 0x01},
};

static struct sensor_res_tab_info s_ov5675_resolution_tab_raw_new[VENDOR_NUM] =
    {
        {.module_id = MODULE_SUNNY,
         .reg_tab = {{ADDR_AND_LEN_OF_ARRAY(ov5675_init_setting1), PNULL, 0,
                      .width = 0, .height = 0, .xclk_to_sensor = EX_MCLK,
                      .image_format = SENSOR_IMAGE_FORMAT_RAW},

                     {ADDR_AND_LEN_OF_ARRAY(ov5675_VGA_setting1), PNULL, 0,
                      .width = 640, .height = 480, .xclk_to_sensor = EX_MCLK,
                      .image_format = SENSOR_IMAGE_FORMAT_RAW},

                     {ADDR_AND_LEN_OF_ARRAY(ov5675_preview_setting1), PNULL, 0,
                      .width = PREVIEW_WIDTH, .height = PREVIEW_HEIGHT,
                      .xclk_to_sensor = EX_MCLK,
                      .image_format = SENSOR_IMAGE_FORMAT_RAW},

                     {ADDR_AND_LEN_OF_ARRAY(ov5675_snapshot_setting1), PNULL, 0,
                      .width = SNAPSHOT_WIDTH, .height = SNAPSHOT_HEIGHT,
                      .xclk_to_sensor = EX_MCLK,
                      .image_format = SENSOR_IMAGE_FORMAT_RAW}}},

        /*If there are multiple modules,please add here*/
};

static SENSOR_TRIM_T s_ov5675_resolution_trim_tab_new[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .trim_info =
         {
             {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
             {.trim_start_x = 0,
              .trim_start_y = 0,
              .trim_width = 640,
              .trim_height = 480,
              .line_time = 66667,
              .bps_per_lane = 906,
              .frame_line = 500,
              .scaler_trim = {.x = 0, .y = 0, .w = 640, .h = 480}},
             {.trim_start_x = PREVIEW_TRIM_X,
              .trim_start_y = PREVIEW_TRIM_Y,
              .trim_width = PREVIEW_TRIM_W,
              .trim_height = PREVIEW_TRIM_H,
              .line_time = 33333,
              .bps_per_lane = 906,
              .frame_line = 1000,
              .scaler_trim =
                  {.x = 0, .y = 0, .w = PREVIEW_TRIM_W, .h = PREVIEW_TRIM_H}},
             {.trim_start_x = SNAPSHOT_TRIM_X,
              .trim_start_y = SNAPSHOT_TRIM_Y,
              .trim_width = SNAPSHOT_TRIM_W,
              .trim_height = SNAPSHOT_TRIM_H,
              .line_time = SNAPSHOT_LINE_TIME,
              .bps_per_lane = 906,
              .frame_line = SNAPSHOT_FRAME_LENGTH,
              .scaler_trim =
                  {.x = 0, .y = 0, .w = SNAPSHOT_TRIM_W, .h = SNAPSHOT_TRIM_H}},
         }},

    /*If there are multiple modules,please add here*/
};

static struct sensor_res_tab_info s_ov5675_resolution_tab_raw[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .reg_tab = {{ADDR_AND_LEN_OF_ARRAY(ov5675_init_setting), PNULL, 0,
                  .width = 0, .height = 0, .xclk_to_sensor = EX_MCLK,
                  .image_format = SENSOR_IMAGE_FORMAT_RAW},

                 {ADDR_AND_LEN_OF_ARRAY(ov5675_VGA_setting), PNULL, 0,
                  .width = 640, .height = 480, .xclk_to_sensor = EX_MCLK,
                  .image_format = SENSOR_IMAGE_FORMAT_RAW},

                 {ADDR_AND_LEN_OF_ARRAY(ov5675_preview_setting), PNULL, 0,
                  .width = PREVIEW_WIDTH, .height = PREVIEW_HEIGHT,
                  .xclk_to_sensor = EX_MCLK,
                  .image_format = SENSOR_IMAGE_FORMAT_RAW},

                 {ADDR_AND_LEN_OF_ARRAY(ov5675_snapshot_setting), PNULL, 0,
                  .width = SNAPSHOT_WIDTH, .height = SNAPSHOT_HEIGHT,
                  .xclk_to_sensor = EX_MCLK,
                  .image_format = SENSOR_IMAGE_FORMAT_RAW}}}

    /*If there are multiple modules,please add here*/
};

static SENSOR_TRIM_T s_ov5675_resolution_trim_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .trim_info =
         {
             {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
             {.trim_start_x = 0,
              .trim_start_y = 0,
              .trim_width = 640,
              .trim_height = 480,
              .line_time = 66667,
              .bps_per_lane = 900,
              .frame_line = 500,
              .scaler_trim = {.x = 0, .y = 0, .w = 640, .h = 480}},
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
         }},

    /*If there are multiple modules,please add here*/
};

static const SENSOR_REG_T
    s_ov5675_preview_size_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
        /*video mode 0: ?fps*/
        {{.reg_addr = 0xffff, .reg_value = 0xff}},
        /* video mode 1:?fps*/
        {{.reg_addr = 0xffff, .reg_value = 0xff}},
        /* video mode 2:?fps*/
        {{.reg_addr = 0xffff, .reg_value = 0xff}},
        /* video mode 3:?fps*/
        {{.reg_addr = 0xffff, .reg_value = 0xff}},
};

static const SENSOR_REG_T
    s_ov5675_capture_size_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
        /*video mode 0: ?fps*/
        {{.reg_addr = 0xffff, .reg_value = 0xff}},
        /* video mode 1:?fps*/
        {{.reg_addr = 0xffff, .reg_value = 0xff}},
        /* video mode 2:?fps*/
        {{.reg_addr = 0xffff, .reg_value = 0xff}},
        /* video mode 3:?fps*/
        {{.reg_addr = 0xffff, .reg_value = 0xff}},
};

static SENSOR_VIDEO_INFO_T s_ov5675_video_info[SENSOR_MODE_MAX] = {
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{30, 30, 270, 90}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
     (SENSOR_REG_T **)s_ov5675_preview_size_video_tab},
    {{{2, 5, 338, 1000}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
     (SENSOR_REG_T **)s_ov5675_capture_size_video_tab},
};

static SENSOR_STATIC_INFO_T s_ov5675_static_info[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .static_info = {.f_num = 220,
                     .focal_length = 346,
                     .max_fps = 0,
                     .max_adgain = 16,
                     .ois_supported = 0,
                     .pdaf_supported = 0,
                     .exp_valid_frame_num = 1,
                     .clamp_level = 64,
                     .adgain_valid_frame_num = 1,
                     .fov_info = {{2.9457f, 2.214f}, 2.1011f}}},

    /*If there are multiple modules,please add here*/
};

static SENSOR_MODE_FPS_INFO_T s_ov5675_mode_fps_info[VENDOR_NUM] = {
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
       {SENSOR_MODE_SNAPSHOT_TWO_THIRD, 0, 1, 0, 0}}}},

    /*If there are multiple modules,please add here*/
};

static struct sensor_module_info s_ov5675_module_info_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .module_info = {.major_i2c_addr = 0x6c >> 1,
                     .minor_i2c_addr = 0x20 >> 1,

                     .reg_addr_value_bits = SENSOR_I2C_REG_16BIT |
                                            SENSOR_I2C_VAL_8BIT |
                                            SENSOR_I2C_FREQ_400,

                     .avdd_val = SENSOR_AVDD_2800MV,
                     .iovdd_val = SENSOR_AVDD_1800MV,
                     .dvdd_val = SENSOR_AVDD_1200MV,

                     .image_pattern = SENSOR_IMAGE_PATTERN_RAWRGB_B,

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
                     .vertical_view_angle = 60}},

    /*If there are multiple modules,please add here*/
};
/*==============================================================================
 * Description:
 * sensor all info
 * please modify this variable acording your spec
 *============================================================================*/
SENSOR_INFO_T g_ov5675_dual_mipi_raw_info = {
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
    .identify_code = {{ov5675_PID_ADDR, ov5675_PID_VALUE},
                      {ov5675_VER_ADDR, ov5675_VER_VALUE}},

    .source_width_max = SNAPSHOT_WIDTH,
    .source_height_max = SNAPSHOT_HEIGHT,
    .name = (cmr_s8 *)SENSOR_NAME,
    .image_format = SENSOR_IMAGE_FORMAT_RAW,

    .resolution_tab_info_ptr = s_ov5675_resolution_tab_raw,
    .sns_ops = &s_ov5675_ops_tab,
    .raw_info_ptr = &s_ov5675_dual_mipi_raw_info_ptr,
    .ext_info_ptr = NULL,

    .module_info_tab = s_ov5675_module_info_tab,
    .module_info_tab_size = ARRAY_SIZE(s_ov5675_module_info_tab),
    .video_tab_info_ptr = s_ov5675_video_info,

    .sensor_version_info = (cmr_s8 *)"ov5675_v1",
};

#endif
