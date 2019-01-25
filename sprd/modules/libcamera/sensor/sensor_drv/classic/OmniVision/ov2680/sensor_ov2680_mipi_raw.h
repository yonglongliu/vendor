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
#ifndef _SENSOR_OV2680_H_
#define _SENSOR_OV2680_H_

#include "cutils/properties.h"
#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"
#if defined(CONFIG_CAMERA_ISP_VERSION_V3) ||                                   \
    defined(CONFIG_CAMERA_ISP_VERSION_V4)
#include "parameters/sensor_ov2680_raw_param_v3.c"
#else
#include "parameters/sensor_ov2680_raw_param_v2.c"
#endif
#ifdef CONFIG_CAMERA_RT_REFOCUS
#include "al3200.h"
#endif
#define VENDOR_NUM 1
#define ov2680_I2C_ADDR_W (0x6c >> 1)
#define ov2680_I2C_ADDR_R (0x6c >> 1)
#define RAW_INFO_END_ID 0x71717567
#define FRAME_OFFSET 4

#undef SENSOR_NAME
#define SENSOR_NAME "ov2680_mipi_raw"

#define CONFIG_CAMERA_IMAGE_180
#define ov2680_MIN_FRAME_LEN_PRV 0x484
#define ov2680_MIN_FRAME_LEN_CAP 0x7B6
#define ov2680_RAW_PARAM_COM 0x0000

typedef struct ov2680_private_data{
    cmr_u16 VTS;
}PRIVATE_DATA;

/* STATEMENT:  Don't define global variables*/

// 800x600
static const SENSOR_REG_T ov2680_com_mipi_raw[] = {
    {0x0103, 0x01},
    {0xffff, 0xa0},
    {0x3002, 0x00},
    {0x3016, 0x1c},
    {0x3018, 0x44},
    {0x3020, 0x00},
    {0x3080, 0x02},
    {0x3082, 0x37},
    {0x3084, 0x09},
    {0x3085, 0x04},
    {0x3086, 0x01},
    {0x3501, 0x26},
    {0x3502, 0x40},
    {0x3503, 0x03},
    {0x350b, 0x36},
    {0x3600, 0xb4},
    {0x3603, 0x39},
    {0x3604, 0x24},
    {0x3605, 0x00}, // bit3:   1: use external
                    // regulator  0: use
                    // internal regulator
    {0x3620, 0x26},
    {0x3621, 0x37},
    {0x3622, 0x04},
    {0x3628, 0x00},
    {0x3705, 0x3c},
    {0x370c, 0x50},
    {0x370d, 0xc0},
    {0x3718, 0x88},
    {0x3720, 0x00},
    {0x3721, 0x00},
    {0x3722, 0x00},
    {0x3723, 0x00},
    {0x3738, 0x00},
    {0x370a, 0x23},
    {0x3717, 0x58},
    {0x3781, 0x80},
    {0x3784, 0x0c},
    {0x3789, 0x60},
    {0x3800, 0x00},
    {0x3801, 0x00},
    {0x3802, 0x00},
    {0x3803, 0x00},
    {0x3804, 0x06},
    {0x3805, 0x4f},
    {0x3806, 0x04},
    {0x3807, 0xbf},
    {0x3808, 0x03},
    {0x3809, 0x20},
    {0x380a, 0x02},
    {0x380b, 0x58},
    {0x380c, 0x06},
    {0x380d, 0xac},
    {0x380e, 0x02},
    {0x380f, 0x84},
    {0x3810, 0x00},
    {0x3811, 0x04},
    {0x3812, 0x00},
    {0x3813, 0x04},
    {0x3814, 0x31},
    {0x3815, 0x31},
    {0x3819, 0x04},
#if 0//def CONFIG_CAMERA_IMAGE_180
    {0x3820, 0xc2},
    {0x3821, 0x00},
#else
    {0x3820, 0xc6},
    {0x3821, 0x04},
#endif
    {0x4000, 0x81},
    {0x4001, 0x40},
    {0x4008, 0x00},
    {0x4009, 0x03},
    {0x4602, 0x02},
    {0x481b, 0x43},
    {0x481f, 0x36},
    {0x4825, 0x36},
    {0x4837, 0x30},
    {0x5002, 0x30},
    {0x5080, 0x00},
    {0x5081, 0x41},
    {0x0100, 0x00}

};

static const SENSOR_REG_T ov2680_640X480_mipi_raw[] = {
    {0x3086, 0x01}, {0x3501, 0x26}, {0x3502, 0x40},
    {0x3620, 0x26}, {0x3621, 0x37}, {0x3622, 0x04},
    {0x370a, 0x23}, {0x370d, 0xc0}, {0x3718, 0x88},
    {0x3721, 0x00}, {0x3722, 0x00}, {0x3723, 0x00},
    {0x3738, 0x00}, {0x3800, 0x00}, {0x3801, 0xa0},
    {0x3802, 0x00}, {0x3803, 0x78}, {0x3804, 0x05},
    {0x3805, 0xaf}, {0x3806, 0x04}, {0x3807, 0x47},
    {0x3808, 0x02}, {0x3809, 0x80}, {0x380a, 0x01},
    {0x380b, 0xe0}, {0x380c, 0x06}, {0x380d, 0xac},
    {0x380e, 0x02}, {0x380f, 0x84}, {0x3810, 0x00},
    {0x3811, 0x04}, {0x3812, 0x00}, {0x3813, 0x04},
    {0x3814, 0x31}, {0x3815, 0x31},
#ifdef CONFIG_CAMERA_IMAGE_180
    {0x3820, 0xc2}, {0x3821, 0x00},
#else
    {0x3820, 0xc6}, {0x3821, 0x04},
#endif
    {0x4008, 0x00}, {0x4009, 0x03}, {0x4837, 0x30},
    {0x0100, 0x00}

};

static const SENSOR_REG_T ov2680_800X600_mipi_raw[] = {
    {0x0103, 0x01},
    {0x3002, 0x00},
    {0x3016, 0x1c},
    {0x3018, 0x44},
    {0x3020, 0x00},

    {0x3080, 0x02},
    {0x3082, 0x37},
    {0x3084, 0x09},
    {0x3085, 0x04},
    {0x3086, 0x01},

    {0x3501, 0x26},
    {0x3502, 0x40},
    {0x3503, 0x03},
    {0x350b, 0x36},
    {0x3600, 0xb4},
    {0x3603, 0x39},
    {0x3604, 0x24},
    {0x3605,
     0x00}, // bit3:   1: use external regulator  0: use internal regulator
    {0x3620, 0x26},
    {0x3621, 0x37},
    {0x3622, 0x04},
    {0x3628, 0x00},
    {0x3705, 0x3c},
    {0x370c, 0x50},
    {0x370d, 0xc0},
    {0x3718, 0x88},
    {0x3720, 0x00},
    {0x3721, 0x00},
    {0x3722, 0x00},
    {0x3723, 0x00},
    {0x3738, 0x00},
    {0x370a, 0x23},
    {0x3717, 0x58},
    {0x3781, 0x80},
    {0x3784, 0x0c},
    {0x3789, 0x60},
    {0x3800, 0x00},
    {0x3801, 0x00},
    {0x3802, 0x00},
    {0x3803, 0x00},
    {0x3804, 0x06},
    {0x3805, 0x4f},
    {0x3806, 0x04},
    {0x3807, 0xbf},
    {0x3808, 0x03},
    {0x3809, 0x20},
    {0x380a, 0x02},
    {0x380b, 0x58},
    {0x380c, 0x06}, // hts 1708 6ac  1710 6ae
    {0x380d, 0xae},
    {0x380e, 0x02}, // vts 644
    {0x380f, 0x84},
    {0x3810, 0x00},
    {0x3811, 0x04},
    {0x3812, 0x00},
    {0x3813, 0x04},
    {0x3814, 0x31},
    {0x3815, 0x31},
    {0x3819, 0x04},
#ifdef CONFIG_CAMERA_IMAGE_180
    {0x3820, 0xc2}, // bit[2] flip
    {0x3821, 0x00}, // bit[2] mirror
#else
    {0x3820, 0xc6}, // bit[2] flip
    {0x3821, 0x05}, // bit[2] mirror
#endif
    {0x4000, 0x81},
    {0x4001, 0x40},
    {0x4008, 0x00},
    {0x4009, 0x03},
    {0x4602, 0x02},
    {0x481f, 0x36},
    {0x4825, 0x36},
    {0x4837, 0x30},
    {0x5002, 0x30},
    {0x5080, 0x00},
    {0x5081, 0x41},
    {0x0100, 0x00}

};

static const SENSOR_REG_T ov2680_1600X1200_altek_mipi_raw[] = {
    {0x0103, 0x01},
    {0x3002, 0x00},
    {0x3016, 0x1c},
    {0x3018, 0x44},
    {0x3020, 0x00},
    {0x3080, 0x02},
    {0x3082, 0x37},
    {0x3084, 0x09},
    {0x3085, 0x04},
    {0x3086, 0x00},
    {0x3501, 0x4e},
    {0x3502, 0xe0},
    {0x3503, 0x03},
    {0x350b, 0x36},
    {0x3600, 0xb4},
    {0x3603, 0x35},
    {0x3604, 0x24},
    {0x3605, 0x00},
    {0x3620, 0x24},
    {0x3621, 0x34},
    {0x3622, 0x03},
    {0x3628, 0x00},
    {0x3701, 0x64},
    {0x3705, 0x3c},
    {0x370c, 0x50},
    {0x370d, 0xc0},
    {0x3718, 0x80},
    {0x3720, 0x00},
    {0x3721, 0x09},
    {0x3722, 0x06},
    {0x3723, 0x59},
    {0x3738, 0x99},
    {0x370a, 0x21},
    {0x3717, 0x58},
    {0x3781, 0x80},
    {0x3784, 0x0c},
    {0x3789, 0x60},
    {0x3800, 0x00},
    {0x3801, 0x00},
    {0x3802, 0x00},
    {0x3803, 0x00},
    {0x3804, 0x06},
    {0x3805, 0x4f},
    {0x3806, 0x04},
    {0x3807, 0xbf},
    {0x3808, 0x06},
    {0x3809, 0x40},
    {0x380a, 0x04},
    {0x380b, 0xb0},
    {0x380c, 0x06},
    {0x380d, 0xa4},
    {0x380e, 0x05},
    {0x380f, 0x0e},
    {0x3810, 0x00},
    {0x3811, 0x08},
    {0x3812, 0x00},
    {0x3813, 0x08},
    {0x3814, 0x11},
    {0x3815, 0x11},
    {0x3819, 0x04},
#if 0//ndef CAMERA_IMAGE_180
    {0x3820, 0xc0},
    {0x3821, 0x00},
#else
	{0x3820, 0xc4},
	{0x3821, 0x04},
#endif
    {0x4000, 0x81},
    {0x4001, 0x40},
    {0x4008, 0x02},
    {0x4009, 0x09},
    {0x4602, 0x02},
    {0x481f, 0x36},
    {0x4825, 0x36},
    {0x4837, 0x18},
    {0x5002, 0x30},
    {0x5080, 0x00},
    {0x5081, 0x41},
#if 0 // original setting , it clould output depth map
	{0x0100, 0x01},
	{0x0103, 0x01},
	{0x3002, 0x00},
	{0x3016, 0x1c},
	{0x3018, 0x44},
	{0x3020, 0x00},
	{0x3080, 0x02},
	{0x3082, 0x37},
	{0x3084, 0x09},
	{0x3085, 0x04},
	{0x3086, 0x01},
	{0x3501, 0x4e},
	{0x3502, 0xe0},
	{0x3503, 0x03},
	{0x350b, 0x36},
	{0x3600, 0xb4},
	{0x3603, 0x35},
	{0x3604, 0x24},
	{0x3605, 0x00},
	{0x3620, 0x24},
	{0x3621, 0x34},
	{0x3622, 0x03},
	{0x3628, 0x00},
	{0x3701, 0x64},
	{0x3705, 0x3c},
	{0x370c, 0x50},
	{0x370d, 0x00},
	{0x3718, 0x80},
	{0x3720, 0x00},
	{0x3721, 0x09},
	{0x3722, 0x0b},
	{0x3723, 0x48},
	{0x3738, 0x99},
	{0x370a, 0x21},
	{0x3717, 0x58},
	{0x3781, 0x80},
	{0x3784, 0x0c},
	{0x3789, 0x60},
	{0x3800, 0x00},
	{0x3801, 0x00},
	{0x3802, 0x00},
	{0x3803, 0x00},
	{0x3804, 0x06},
	{0x3805, 0x4f},
	{0x3806, 0x04},
	{0x3807, 0xbf},
	{0x3808, 0x06},
	{0x3809, 0x40},
	{0x380a, 0x04},
	{0x380b, 0xb0},
	{0x380c, 0x06},
	{0x380d, 0xa4},
	{0x380e, 0x05},
	{0x380f, 0x0e},
	{0x3810, 0x00},
	{0x3811, 0x08},
	{0x3812, 0x00},
	{0x3813, 0x08},
	{0x3814, 0x11},
	{0x3815, 0x11},
	{0x3819, 0x04},
	{0x3820, 0xc0},
	{0x3821, 0x00},
	{0x4000, 0x81},
	{0x4001, 0x40},
	{0x4008, 0x02},
	{0x4009, 0x09},
	{0x4602, 0x02},
	{0x481f, 0x36},
	{0x4825, 0x36},
	{0x4837, 0x30},
	{0x5002, 0x30},
	{0x5080, 0x00},
	{0x5081, 0x41},
#else // it is altek setting it can sync but d2 is not OK

    //{0x0100, 0x01},
    {0x5780, 0x3e},
    {0x5781, 0x0f},
    {0x5782, 0x04},
    {0x5783, 0x02},
    {0x5784, 0x01},
    {0x5785, 0x01},
    {0x5786, 0x00},
    {0x5787, 0x04},
    {0x5788, 0x02},
    {0x5789, 0x00},
    {0x578a, 0x01},
    {0x578b, 0x02},
    {0x578c, 0x03},
    {0x578d, 0x03},
    {0x578e, 0x08},
    {0x578f, 0x0c},
    {0x5790, 0x08},
    {0x5791, 0x04},
    {0x5792, 0x00},
    {0x5793, 0x00},
    {0x5794, 0x03},
#endif
#ifdef CONFIG_CAMERA_DUAL_SYNC
    /*dual cam sync begin*/
    {0x3002, 0x00},
    {0x3823, 0x30},
    {0x3824, 0x00}, // cs
    {0x3825, 0x20},

    {0x3826, 0x00},
    {0x3827, 0x06},
/*dual cam sync end*/
#endif

};

static const SENSOR_REG_T ov2680_1600X1200_mipi_raw[] = {
#if (SC_FPGA == 0)
    {0x3086, 0x00},
#else
    {0x3086, 0x02},
#endif
    {0x3501, 0x4e}, {0x3502, 0xe0}, {0x3620, 0x26}, {0x3621, 0x37},
    {0x3622, 0x04}, {0x370a, 0x21}, {0x370d, 0xc0}, {0x3718, 0x88},
    {0x3721, 0x00}, {0x3722, 0x00}, {0x3723, 0x00}, {0x3738, 0x00},
    {0x3800, 0x00}, {0x3801, 0x00}, {0x3802, 0x00}, {0x3803, 0x00},
    {0x3804, 0x06}, {0x3805, 0x4f}, {0x3806, 0x04}, {0x3807, 0xbf},
    {0x3808, 0x06}, {0x3809, 0x40}, {0x380a, 0x04}, {0x380b, 0xb0},
    {0x380c, 0x06}, {0x380d, 0xa4},
#if (SC_FPGA == 0)
    {0x380e, 0x05}, {0x380f, 0x0e},
#else
    {0x380e, 0x0a}, {0x380f, 0x1c},
#endif
    {0x3811, 0x08}, {0x3813, 0x08}, {0x3814, 0x11}, {0x3815, 0x11},
#if 0 //def CONFIG_CAMERA_IMAGE_180
    {0x3820, 0xc0}, {0x3821, 0x00},
#else
    {0x3820, 0xc4}, {0x3821, 0x04},
#endif
    {0x4008, 0x02}, {0x4009, 0x09}, {0x481b, 0x43}, {0x4837, 0x18},

};

static struct sensor_res_tab_info s_ov2680_resolution_tab_raw[VENDOR_NUM] = {
    {
      .module_id = MODULE_SUNNY,
      .reg_tab = {
        {ADDR_AND_LEN_OF_ARRAY(ov2680_com_mipi_raw), PNULL, 0,
        .width = 0, .height = 0,
        .xclk_to_sensor = 24, .image_format = SENSOR_IMAGE_FORMAT_RAW},
#ifdef CONFIG_CAMERA_DUAL_SYNC
        {ADDR_AND_LEN_OF_ARRAY(ov2680_1600X1200_altek_mipi_raw), PNULL, 0,
        .width = 1600, .height = 1200,
        .xclk_to_sensor = 24, .image_format = SENSOR_IMAGE_FORMAT_RAW},
#else
        {ADDR_AND_LEN_OF_ARRAY(ov2680_1600X1200_mipi_raw), PNULL, 0,
        .width = 1600, .height = 1200,
        .xclk_to_sensor = 24, .image_format = SENSOR_IMAGE_FORMAT_RAW}
#endif
    }}
/*If there are multiple modules,please add here*/
};

static SENSOR_TRIM_T s_ov2680_Resolution_Trim_Tab[VENDOR_NUM] = {
    {
     .module_id = MODULE_SUNNY,
     .trim_info = {
       {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
      /* {.trim_start_x = 0,.trim_start_y = 0, .trim_width = 800,.trim_height = 600,
        .line_time = 51800,.bps_per_lane = 330, .frame_line = 644,
        .scaler_trim = {.x = 0, .y = 0, .w = 800, .h = 600}},*/

       {.trim_start_x = 0,.trim_start_y = 0, .trim_width = 1600,.trim_height = 1200,
        .line_time = 25758,.bps_per_lane = 628, .frame_line = 1294,
        .scaler_trim = {.x = 0, .y = 0, .w = 1600, .h = 1200}}}
    }
    /*If there are multiple modules,please add here*/
};


static const SENSOR_REG_T
    s_ov2680_640X480_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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
    s_ov2680_1600X1200_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
    /*video mode 0: ?fps*/
    {{.reg_addr = 0xffff, .reg_value = 0xff}},
    /* video mode 1:?fps*/
    {{.reg_addr = 0xffff, .reg_value = 0xff}},
    /* video mode 2:?fps*/
    {{.reg_addr = 0xffff, .reg_value = 0xff}},
    /* video mode 3:?fps*/
    {{.reg_addr = 0xffff, .reg_value = 0xff}},
};


LOCAL SENSOR_VIDEO_INFO_T s_ov2680_video_info[] = {
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    //{{{30, 30, 284, 90}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0,
    // 0}},(SENSOR_REG_T**)s_ov2680_640X480_video_tab},
    {{{.min_frate = 25, .max_frate = 25, .line_time = 258, .gain = 64},
      {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
     (SENSOR_REG_T **)s_ov2680_1600X1200_video_tab},
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL}};

static SENSOR_STATIC_INFO_T s_ov2680_static_info[VENDOR_NUM] = {
    {
     .module_id = MODULE_SUNNY,
     .static_info = {
         .f_num = 240,
         .focal_length = 200,
         .max_fps = 0,
         .max_adgain = 8,
         .ois_supported = 0,
         .pdaf_supported = 0,
         .exp_valid_frame_num = 1,
         .clamp_level = 16,
         .adgain_valid_frame_num = 1,
         .fov_info = {{2.84f, 2.15f}, 2.15f}}
    }
    /*If there are multiple modules,please add here*/
};

static SENSOR_MODE_FPS_INFO_T s_ov2680_mode_fps_info[VENDOR_NUM] = {
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

static struct sensor_module_info s_ov2680_module_info_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .module_info = {
         .major_i2c_addr = 0x6c >> 1,
         .minor_i2c_addr = 0x6c >> 1,

         .reg_addr_value_bits = SENSOR_I2C_REG_16BIT | SENSOR_I2C_REG_8BIT |
                                 SENSOR_I2C_FREQ_100,

         .avdd_val = SENSOR_AVDD_2800MV,
         .iovdd_val = SENSOR_AVDD_1800MV,
         .dvdd_val = SENSOR_AVDD_1500MV,

         .image_pattern = SENSOR_IMAGE_PATTERN_RAWRGB_B,

         .preview_skip_num = 1,
         .capture_skip_num = 0,
         .flash_capture_skip_num = 6,
         .mipi_cap_skip_num = 0,
         .preview_deci_num = 0,
         .video_preview_deci_num = 0,

         .sensor_interface =
         {  .type = SENSOR_INTERFACE_TYPE_CSI2,  .bus_width = 1,
            .pixel_width = 10,                   .is_loose = 0},

         .change_setting_skip_num = 1,
         .horizontal_view_angle = 48,
         .vertical_view_angle = 48
      }
    }

/*If there are multiple modules,please add here*/
};

static struct sensor_ic_ops s_ov2680_ops_tab;
static struct sensor_raw_info *s_ov2680_mipi_raw_info_ptr = 
                                    &s_ov2680_mipi_raw_info;

SENSOR_INFO_T g_ov2680_mipi_raw_info = {
    .hw_signal_polarity = SENSOR_HW_SIGNAL_PCLK_N | SENSOR_HW_SIGNAL_VSYNC_N |
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
    .power_down_level = SENSOR_LOW_LEVEL_PWDN,

    .identify_count = 1,
    .identify_code = {{.reg_addr = 0x0A,.reg_value = 0x26},
                      {.reg_addr = 0x0B,.reg_value = 0x80,}},

    .source_width_max = 1600,
    .source_height_max = 1200,
    .name = (cmr_s8 *) SENSOR_NAME,

    .image_format = SENSOR_IMAGE_FORMAT_RAW,

    .resolution_tab_info_ptr = s_ov2680_resolution_tab_raw,
    .sns_ops = &s_ov2680_ops_tab,
    .raw_info_ptr = &s_ov2680_mipi_raw_info_ptr,
    .ext_info_ptr = NULL,
    .module_info_tab = s_ov2680_module_info_tab,
    .module_info_tab_size = ARRAY_SIZE(s_ov2680_module_info_tab),

    .video_tab_info_ptr = s_ov2680_video_info,
    .sensor_version_info = (cmr_s8 *)"ov2680v1"
};
#endif
