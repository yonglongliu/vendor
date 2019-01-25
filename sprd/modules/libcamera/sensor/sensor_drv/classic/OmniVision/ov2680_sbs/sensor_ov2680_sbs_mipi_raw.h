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
 * V2.0
 */
#ifndef _SENSOR_OV2680_SBS_MIPI_RAW_H_
#define _SENSOR_OV2680_SBS_MIPI_RAW_H_

#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"

#include "parameters/sensor_ov2680_raw_param_main.c"

#define SENSOR_NAME "ov2680_mipi_raw"
#if 1                       // defined(CONFIG_DUAL_MODULE)
#define I2C_SLAVE_ADDR 0x20 /* 16bit slave address*/
#else
#define I2C_SLAVE_ADDR 0x6c /* 16bit slave address*/
#endif

#define VENDOR_NUM 1
#define BINNING_FACTOR 1
#define ov2680_PID_ADDR 0x300A
#define ov2680_PID_VALUE 0x26
#define ov2680_VER_ADDR 0x300B
#define ov2680_VER_VALUE 0x80

/* sensor parameters begin */
/* effective sensor output image size */
#define SNAPSHOT_WIDTH 1600  // 5344
#define SNAPSHOT_HEIGHT 1200 // 4016
#define PREVIEW_WIDTH 1600   // 2672
#define PREVIEW_HEIGHT 1200

/* frame length*/
#define SNAPSHOT_FRAME_LENGTH 1294
#define PREVIEW_FRAME_LENGTH 1294

/*Mipi output*/
#define LANE_NUM 1
#define RAW_BITS 10

#define SNAPSHOT_MIPI_PER_LANE_BPS 628
#define PREVIEW_MIPI_PER_LANE_BPS 628

/* please ref your spec */
#define FRAME_OFFSET 4 // 8 // 16 // 32
#define SENSOR_MAX_GAIN 0x3ff
#define SENSOR_BASE_GAIN 0x10
#define SENSOR_MIN_SHUTTER 4

/* isp parameters, please don't change it*/
#if defined(CONFIG_CAMERA_ISP_VERSION_V3) ||                                   \
    defined(CONFIG_CAMERA_ISP_VERSION_V4)
#define ISP_BASE_GAIN 0x80
#else
#define ISP_BASE_GAIN 0x10
#endif

/* please don't change it */
#define EX_MCLK 24
#define IMAGE_NORMAL_MIRROR
#ifdef SBS_SENSOR_FRONT
#define CAMERA_IMAGE_180
#endif
/*==============================================================================
 * Description:
 * global variable
 *============================================================================*/
// static struct hdr_info_t s_hdr_info;
// static uint32_t s_current_default_frame_length;
// struct sensor_ev_info_t s_sensor_ev_info;

LOCAL const SENSOR_REG_T ov2680_com_mipi_raw2[] = {};
// 800x600
LOCAL const SENSOR_REG_T ov2680_com_mipi_raw[] = {
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
#if 0 // def CONFIG_CAMERA_IMAGE_180
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
    {0x5000, 0x00},
    {0x5002, 0x30},
    {0x5080, 0x00},
    {0x5081, 0x41},
    {0x0100, 0x00}

};

LOCAL const SENSOR_REG_T ov2680_640X480_mipi_raw[] = {
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

LOCAL const SENSOR_REG_T ov2680_800X600_mipi_raw[] = {
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
    {0x3605, 0x00}, // bit3:   1: use external regulator  0: use
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
    {0x5000, 0x00},
    {0x5002, 0x30},
    {0x5080, 0x00},
    {0x5081, 0x41},
    {0x0100, 0x00}

};

LOCAL const SENSOR_REG_T ov2680_1600X1200_altek_mipi_raw[] = {
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
#if 0 // ndef CAMERA_IMAGE_180
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
    {0x5000, 0x00},
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
#if 1
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
LOCAL const SENSOR_REG_T ov2680_1600X1200_mipi_raw2[] = {
    /*	;------------------------------------
            ;@@ 2680 Full 1600x1200 RAW 30fps 1lane*/
    {0x0103, 0x01}, //
    {0xffff, 0x0a}, //
    {0x3002, 0x00}, //
    {0x3016, 0x1c}, //
    {0x3018, 0x44}, //
    {0x3020, 0x00}, //
    {0x3080, 0x02}, //
    {0x3082, 0x37}, //
    {0x3084, 0x09}, //
    {0x3085, 0x04}, //
    {0x3086, 0x01}, //
    {0x3501, 0x4e}, //
    {0x3502, 0xe0}, //
    {0x3503, 0x03}, //
    {0x350b, 0x36}, //
    {0x3600, 0xb4}, //
    {0x3603, 0x35}, //
    {0x3604, 0x24}, //
    {0x3605, 0x00}, //
    {0x3620, 0x24}, //
    {0x3621, 0x34}, //
    {0x3622, 0x03}, //
    {0x3628, 0x00}, //
    {0x3701, 0x64}, //
    {0x3705, 0x3c}, //
    {0x370c, 0x50}, //
    {0x370d, 0x00}, //
    {0x3718, 0x80}, //
    {0x3720, 0x00}, //
    {0x3721, 0x09}, //
    {0x3722, 0x0b}, //
    {0x3723, 0x48}, //
    {0x3738, 0x99}, //
    {0x370a, 0x21}, //
    {0x3717, 0x58}, //
    {0x3781, 0x80}, //
    {0x3784, 0x0c}, //
    {0x3789, 0x60}, //
    {0x3800, 0x00}, //
    {0x3801, 0x00}, //
    {0x3802, 0x00}, //
    {0x3803, 0x00}, //
    {0x3804, 0x06}, //
    {0x3805, 0x4f}, //
    {0x3806, 0x04}, //
    {0x3807, 0xbf}, //
    {0x3808, 0x06}, //
    {0x3809, 0x40}, //
    {0x380a, 0x04}, //
    {0x380b, 0xb0}, //
    {0x380c, 0x06}, //
    {0x380d, 0xe8}, // ;e7 ;e4 ;a4
    {0x380e, 0x04}, // ;05
    {0x380f, 0xdc}, // ;0e
    {0x3810, 0x00}, //
    {0x3811, 0x08}, //
    {0x3812, 0x00}, //
    {0x3813, 0x08}, //
    {0x3814, 0x11}, //
    {0x3815, 0x11}, //
    {0x3819, 0x04}, //
#ifndef CAMERA_IMAGE_180
    {0x3820, 0xc0},
    {0x3821, 0x00},
#else
    {0x3820, 0xc4},
    {0x3821, 0x04},
#endif
    {0x4000, 0x81}, //
    {0x4001, 0x40}, //
    {0x4008, 0x02}, //
    {0x4009, 0x09}, //
    {0x4602, 0x02}, //
    {0x481f, 0x36}, //
    {0x4825, 0x36}, //
    {0x4837, 0x30}, //
                    //	{0x5000, 0x00},//
    {0x5002, 0x30}, //
    {0x5080, 0x00}, //
    {0x5081, 0x41}, //
    //{0x0100, 0x01},//
    {0x5780, 0x3e}, //
    {0x5781, 0x0f}, //
    {0x5782, 0x04}, //
    {0x5783, 0x02}, //
    {0x5784, 0x01}, //
    {0x5785, 0x01}, //
    {0x5786, 0x00}, //
    {0x5787, 0x04}, //
    {0x5788, 0x02}, //
    {0x5789, 0x00}, //
    {0x578a, 0x01}, //
    {0x578b, 0x02}, //
    {0x578c, 0x03}, //
    {0x578d, 0x03}, //
    {0x578e, 0x08}, //
    {0x578f, 0x0c}, //
    {0x5790, 0x08}, //
    {0x5791, 0x04}, //
    {0x5792, 0x00}, //
    {0x5793, 0x00}, //
    {0x5794, 0x03}, //

#if 0
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
LOCAL const SENSOR_REG_T ov2680_1600X1200_mipi_raw[] = {
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
#if 0 // def CONFIG_CAMERA_IMAGE_180
    {0x3820, 0xc0}, {0x3821, 0x00},
#else
    {0x3820, 0xc4}, {0x3821, 0x04},
#endif
    {0x4008, 0x02}, {0x4009, 0x09}, {0x481b, 0x43}, {0x4837, 0x18},

};

static SENSOR_STATIC_INFO_T s_ov2680_static_info[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .static_info =
         {
#ifdef SBS_MODE_SENSOR
             .f_num = 220,
             .focal_length = 227,
#else
             .f_num = 200,
             .focal_length = 354,
#endif
             .max_fps = 0,
             .max_adgain = 8 * 4,
             .ois_supported = 0,
             .pdaf_supported = 0,
             .exp_valid_frame_num = 1,
#ifdef SBS_MODE_SENSOR
             .clamp_level = 4,
#else
             .clamp_level = 16,
#endif
             .adgain_valid_frame_num = 1,
             .fov_info = {{4.614f, 3.444f}, 4.222f}}}
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
       {SENSOR_MODE_SNAPSHOT_TWO_THIRD, 0, 1, 0, 0}}}}
    /*If there are multiple modules,please add here*/
};

static struct sensor_res_tab_info s_ov2680_resolution_tab_raw[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .reg_tab =
         {
#ifdef SBS_MODE_SENSOR
             {ADDR_AND_LEN_OF_ARRAY(ov2680_1600X1200_mipi_raw2), PNULL, 0,
              .width = 0, .height = 0, .xclk_to_sensor = EX_MCLK,
              .image_format = SENSOR_IMAGE_FORMAT_RAW},
             {ADDR_AND_LEN_OF_ARRAY(ov2680_com_mipi_raw2), PNULL, 0,
              .width = 1600, .height = 1200, .xclk_to_sensor = 24,
              .image_format = SENSOR_IMAGE_FORMAT_RAW},
             {ADDR_AND_LEN_OF_ARRAY(ov2680_com_mipi_raw2), PNULL, 0,
              .width = 1600, .height = 1200, .xclk_to_sensor = 24,
              .image_format = SENSOR_IMAGE_FORMAT_RAW},
#else
             {ADDR_AND_LEN_OF_ARRAY(ov2680_com_mipi_raw), PNULL, 0, .width = 0,
              .height = 0, .xclk_to_sensor = EX_MCLK,
              .image_format = SENSOR_IMAGE_FORMAT_RAW},
//{ADDR_AND_LEN_OF_ARRAY(ov2680_800X600_mipi_raw), 800, 600, 24,
// SENSOR_IMAGE_FORMAT_RAW},
#ifdef CONFIG_CAMERA_DUAL_SYNC
             {ADDR_AND_LEN_OF_ARRAY(ov2680_1600X1200_altek_mipi_raw), PNULL, 0,
              .width = 1600, .height = 1200, .xclk_to_sensor = EX_MCLK,
              .image_format = SENSOR_IMAGE_FORMAT_RAW},
#else
             {ADDR_AND_LEN_OF_ARRAY(ov2680_1600X1200_mipi_raw), PNULL, 0,
              .width = 1600, .height = 1200, .xclk_to_sensor = EX_MCLK,
              .image_format = SENSOR_IMAGE_FORMAT_RAW},
#endif
#endif
         }}
    /*If there are multiple modules,please add here*/
};

static SENSOR_TRIM_T s_ov2680_resolution_trim_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .trim_info =
         {
             {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
             //{0, 0, 800, 600, 51800, 330, 644, {0, 0, 800, 600}},
             {0,
              0,
              1600,
              1200,
              53590,
              628,
              1244,
              {0, 0, 1600, 1200}}, // 25757.6ns
#ifdef SBS_MODE_SENSOR
             {0,
              0,
              1600,
              1200,
              53590,
              628,
              1244,
              {0, 0, 1600, 1200}}, // 25757.6ns
#endif
         }}

    /*If there are multiple modules,please add here*/
};

static const SENSOR_REG_T
    s_ov2680_preview_size_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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
    s_ov2680_capture_size_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
        /*video mode 0: ?fps*/
        {{.reg_addr = 0xffff, .reg_value = 0xff}},
        /* video mode 1:?fps*/
        {{.reg_addr = 0xffff, .reg_value = 0xff}},
        /* video mode 2:?fps*/
        {{.reg_addr = 0xffff, .reg_value = 0xff}},
        /* video mode 3:?fps*/
        {{.reg_addr = 0xffff, .reg_value = 0xff}},
};

static SENSOR_VIDEO_INFO_T s_ov2680_video_info[SENSOR_MODE_MAX] = {
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{30, 30, 270, 90}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
     (SENSOR_REG_T **)s_ov2680_preview_size_video_tab},
    {{{2, 5, 338, 1000}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
     (SENSOR_REG_T **)s_ov2680_capture_size_video_tab},
};

static struct sensor_module_info s_ov2680_module_info_tab[VENDOR_NUM] = {
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

                     .preview_skip_num = 1,
                     .capture_skip_num = 3,
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
                             .bus_width = 4,
                             .pixel_width = 10,
                             .is_loose = 0,
                         },
                     .change_setting_skip_num = 1,
                     .horizontal_view_angle = 35,
                     .vertical_view_angle = 35}}

    /*If there are multiple modules,please add here*/
};

static struct sensor_ic_ops s_ov2680_ops_tab;
struct sensor_raw_info *s_ov2680_sbs_mipi_raw_info_ptr =
    &s_ov2680_mipi_raw_info;

SENSOR_INFO_T g_ov2680_sbs_mipi_raw_info = {
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
    .identify_code =
        {
            {.reg_addr = ov2680_PID_ADDR, .reg_value = ov2680_PID_VALUE},
            {.reg_addr = ov2680_VER_ADDR, .reg_value = ov2680_VER_VALUE},
        },

    .source_width_max = SNAPSHOT_WIDTH,
    .source_height_max = SNAPSHOT_HEIGHT,
    .name = (cmr_s8 *)SENSOR_NAME,
    .image_format = SENSOR_IMAGE_FORMAT_RAW,

    .module_info_tab = s_ov2680_module_info_tab,
    .module_info_tab_size = ARRAY_SIZE(s_ov2680_module_info_tab),

    .resolution_tab_info_ptr = s_ov2680_resolution_tab_raw,
    .sns_ops = &s_ov2680_ops_tab,
    .raw_info_ptr = &s_ov2680_sbs_mipi_raw_info_ptr,

    .video_tab_info_ptr = NULL,
    .sensor_version_info = (cmr_s8 *)"ov2680v1",
};
#endif
