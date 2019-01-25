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
#ifndef _SENSOR_C2580_MIPI_RAW_H_
#define _SENSOR_C2580_MIPI_RAW_H_

#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"

#define VENDOR_NUM 1
#define c2580_I2C_ADDR_W 0x3C
#define c2580_I2C_ADDR_R 0x3C
#define SENSOR_NAME "c2580_mipi_raw"
#define c2580_ES_OFFSET 10

#if 0
static int s_c2580_gain = 0;
static int s_c2580_gain_bak = 0;
static int s_c2580_shutter_bak = 0;
static int s_capture_shutter = 0;
static int s_capture_VTS = 0;
#endif

/*sensor private data*/
typedef struct {
    cmr_int gain;
    cmr_int gain_bak;
    cmr_int shutter;
    cmr_int cap_shutter;
    cmr_int cap_vts;
} PRIVATE_DATA;

#if 0
static unsigned long c2580_access_val(SENSOR_HW_HANDLE handle,
                                      unsigned long param);
static unsigned long _c2580_GetResolutionTrimTab(SENSOR_HW_HANDLE handle,
                                                uint32_t param);
static uint32_t _c2580_PowerOn(SENSOR_HW_HANDLE handle, uint32_t power_on);
static uint32_t _c2580_Identify(SENSOR_HW_HANDLE handle, uint32_t param);
static uint32_t _c2580_BeforeSnapshot(SENSOR_HW_HANDLE handle, uint32_t param);
static uint32_t _c2580_after_snapshot(SENSOR_HW_HANDLE handle, uint32_t param);
static uint32_t _c2580_StreamOn(SENSOR_HW_HANDLE handle, uint32_t param);
static uint32_t _c2580_StreamOff(SENSOR_HW_HANDLE handle, uint32_t param);
static uint32_t _c2580_write_exposure(SENSOR_HW_HANDLE handle, uint32_t param);
static uint32_t _c2580_write_gain(SENSOR_HW_HANDLE handle, uint32_t param);
static uint32_t _c2580_write_af(SENSOR_HW_HANDLE handle, uint32_t param);
static uint32_t _c2580_flash(SENSOR_HW_HANDLE handle, uint32_t param);
static uint32_t _c2580_ExtFunc(SENSOR_HW_HANDLE handle, uint32_t ctl_param);
static uint32_t _c2580_set_video_mode(SENSOR_HW_HANDLE handle, uint32_t param);
static int _c2580_get_shutter(SENSOR_HW_HANDLE handle);
static int _c2580_get_gain16(SENSOR_HW_HANDLE handle);
static int _c2580_get_vts(SENSOR_HW_HANDLE handle);

static uint32_t _c2580_cfg_otp(SENSOR_HW_HANDLE handle,uint32_t  param);
static uint32_t _c2580_update_otp(void* param_ptr);
static uint32_t _c2580_Identify_otp(void* param_ptr);


static const struct raw_param_info_tab s_c2580_raw_param_tab[]={
	{0x01, &s_c2580_mipi_raw_info, _c2580_Identify_otp, _c2580_update_otp},
	{RAW_INFO_END_ID, PNULL, PNULL, PNULL}
};
#endif


static const SENSOR_REG_T c2580_common_init[] = {};

static const SENSOR_REG_T c2580_common_init1[] = {
    {0x0103, 0x01},
    {SENSOR_WRITE_DELAY, 0x0a},
    {0x0100, 0x00},
    // for group hold
    {0x3400, 0x00},
    {0x3404, 0x05},
    {0x3500, 0x10},
    {0xe000, 0x02}, // es H  0xE002
    {0xe001, 0x02},
    {0xe002, 0x02},
    {0xe003, 0x02}, // es L   0xE005
    {0xe004, 0x03},
    {0xe005, 0x68},
    {0xe006, 0x02}, // gain    0xE008
    {0xe007, 0x05},
    {0xe008, 0x00},
    {0xe009, 0x03}, // vts H  0xE00B
    {0xe00A, 0x40},
    {0xe00B, 0x02},
    {0xe00C, 0x03}, // vts L  0xE00E
    {0xe00D, 0x41},
    {0xe00E, 0x68},
    {0x3500, 0x00},

    {0x0101, 0x03}, // 1600*1200 setting
    {0x3009, 0x03}, //
    {0x300b, 0x03}, //
    //{0x3084,0x40},//;;80
    //{0x3089,0x10},//
    //{0x3c03,0x00},//
    {0x3180, 0xd0}, //
    {0x3181, 0x40}, //

    {0x3280, 0x06}, //
    {0x3281, 0x05}, //
    {0x3282, 0x93}, //
    {0x3283, 0xd5}, //
    {0x3287, 0x46}, //
    {0x3288, 0x5f}, //
    {0x3289, 0x30}, //
    {0x328A, 0x21}, //
    {0x328B, 0x44}, //
    {0x328C, 0x34}, //
    {0x328D, 0x55}, //
    {0x328E, 0x00}, //
    //{0x3089,0x18},//
    {0x308a, 0x00}, //
    {0x308b, 0x00}, //

    {0x3584, 0x00}, //
    {0x3209, 0x80},
    {0x320C, 0x00}, //
    {0x320E, 0x08}, //
    //{0x3209,0x80},//
    {0x3210, 0x11}, //
    {0x3211, 0x09}, //
    {0x3212, 0x1a}, //
    {0x3213, 0x15}, //
    {0x3214, 0x17}, //
    {0x3215, 0x09}, //
    {0x3216, 0x17}, //
    {0x3217, 0x06}, //
    {0x3218, 0x28}, //
    {0x3219, 0x12}, //
    {0x321A, 0x00}, //
    {0x321B, 0x04}, //
    {0x321C, 0x00}, //

    {0x3200, 0x03}, //
    {0x3201, 0x46}, //
    {0x3202, 0x00}, //
    {0x3203, 0x17}, //
    {0x3204, 0x01}, //
    {0x3205, 0x64}, //
    {0x3206, 0x00}, //
    {0x3207, 0xde}, //
    {0x3208, 0x83}, //

    {0x0303, 0x01}, //
    {0x0304, 0x00}, //
    {0x0305, 0x03}, //
    {0x0307, 0x59}, //
    {0x0309, 0x30}, //

    {0x0342, 0x0b}, //
    {0x0343, 0x5c}, //

    {0x3087, 0x90}, // ;aec/agc: off
    //{0x3084,0x40},//
    {0x3089, 0x18}, //
    //{0x308a,0x00},//
    //{0x308b,0x00},//
    {0x3c03, 0x01},

    {0x3d00, 0xad}, // ;;af; BPC ON
    {0x3d01, 0x17}, //; 01just for single white pixel//20150302 old 0x15
    {0x3d07, 0x48}, //; //20150302 old 0x40
    {0x3d08, 0x48}, //;//20150302 old 0x40

    {0x3805, 0x06}, // mipi
    {0x3806, 0x06},
    {0x3807, 0x06},
    {0x3808, 0x14},
    {0x3809, 0xc4},
    {0x380a, 0x6c},
    {0x380b, 0x8c},
    {0x380c, 0x21},
};
static const SENSOR_REG_T c2580_800_600_setting[] = {
    {0x0101, 0x03}, //
    {0x3009, 0x03}, //
    {0x300b, 0x03}, //
    {0x3805, 0x06}, //
    {0x3806, 0x04}, //
    {0x3807, 0x03}, //
    {0x3808, 0x14}, //
    {0x3809, 0x75}, //
    {0x380a, 0x6D}, //
    {0x380b, 0xCC}, //
    {0x380c, 0x21}, //
    {0x0340, 0x02}, //
    {0x0341, 0x67}, //
    {0x0342, 0x06}, //
    {0x0343, 0xb2}, //
    {0x034c, 0x03}, //
    {0x034d, 0x20}, //
    {0x034e, 0x02}, //
    {0x034f, 0x58}, //
    {0x0383, 0x03}, //
    {0x0387, 0x03}, //
    {0x3021, 0x28}, //
    {0x3022, 0x02}, //
    {0x3280, 0x00}, //
    {0x3281, 0x05}, //
    {0x3084, 0x40}, //;;80
    {0x3089, 0x10}, //
    {0x3c03, 0x00}, //
    {0x3180, 0xd0}, //
    {0x3181, 0x40}, //
    {0x3280, 0x04}, //
    {0x3281, 0x05}, //
    {0x3282, 0x73}, //
    {0x3283, 0xd1}, //
    {0x3287, 0x46}, //
    {0x3288, 0x5f}, //
    {0x3289, 0x30}, //
    {0x328A, 0x21}, //
    {0x328B, 0x44}, //
    {0x328C, 0x34}, //
    {0x328D, 0x55}, //
    {0x328E, 0x00}, //
    {0x308a, 0x00}, //
    {0x308b, 0x00}, //
    {0x320C, 0x00}, //
    {0x320E, 0x08}, //
    {0x3210, 0x22}, //
    {0x3211, 0x14}, //
    {0x3212, 0x40}, //
    {0x3213, 0x35}, //
    {0x3214, 0x30}, //
    {0x3215, 0x40}, //
    {0x3216, 0x2a}, //
    {0x3217, 0x50}, //
    {0x3218, 0x31}, //
    {0x3219, 0x12}, //
    {0x321A, 0x00}, //
    {0x321B, 0x04}, //
    {0x321C, 0x00}, //
    {0x0303, 0x01}, //
    {0x0304, 0x01}, //
    {0x0305, 0x03}, //
    {0x0307, 0x69}, //
    {0x0309, 0x10}, //
    {0x3087, 0x90}, // ;aec/agc: off
    {0x3d00, 0x8d}, // ;;af; BPC ON
    {0x3d01, 0x01}, //; 01just for single white pixel
    {0x3d07, 0x10}, //;
    {0x3d08, 0x10}, //;

    /*{0x0101,0x03},//
    {0x3009,0x02},//
    {0x300b,0x02},//
    {0x3805,0x06},//
    {0x3806,0x04},//
    {0x3807,0x03},//
    {0x3808,0x14},//
    {0x3809,0x75},//
    {0x380a,0x6D},//
    {0x380b,0xCC},//
    {0x380c,0x21},//
    {0x0340,0x02},//
    {0x0341,0x68},//
    {0x0342,0x06},//
    {0x0343,0xb2},//
    {0x034c,0x03},//
    {0x034d,0x20},//
    {0x034e,0x02},//
    {0x034f,0x58},//
    {0x0383,0x03},//
    {0x0387,0x03},//
    {0x3021,0x28},//
    {0x3022,0x02},//
    {0x3280,0x00},//
    {0x3281,0x05},//
    {0x3282,0x83},//
    {0x3283,0x12},//
    {0x3287,0x40},//
    {0x3288,0x57},//
    {0x3289,0x0c},//
    {0x328A,0x21},//  0x00->0x21 20141027
    {0x328B,0x44},//
    {0x328C,0x34},//
    {0x328D,0x55},//
    {0x328E,0x00},//
    {0x308a,0x00},//
    {0x308b,0x00},//
    {0x320C,0x00},//
    {0x320E,0x08},//
    {0x3210,0x22},//
    {0x3211,0x14},//
    {0x3212,0x28},//
    {0x3213,0x14},//
    {0x3214,0x14},//
    {0x3215,0x40},//
    {0x3216,0x1a},//
    {0x3217,0x50},//
    {0x3218,0x31},//
    {0x3219,0x12},//
    {0x321A,0x00},//
    {0x321B,0x04},//
    {0x321C,0x00},//
    {0x0303,0x01},//
    {0x0304,0x01},//
    {0x0305,0x03},//
    {0x0307,0x69},//
    {0x0309,0x10},//
    {0x3087,0xD0},// ;aec/agc: off  0x90->0xD0  gain, es
    {0x0202,0x02},//
    {0x0203,0x28},//
    {0x0205,0x30},//
    {0x3089,0x10},//
    {0x3c03,0x00},//
    {0x3180,0xd0},//
    {0x3181,0x40},//
    {0x3d00,0x8d},// ;;af; BPC ON
    {0x3d01,0x01},//; 01just for single white pixel
    {0x3d07,0x10},//;
    {0x3d08,0x10},//;*/
};

static const SENSOR_REG_T c2580_1600_1200_setting[] = {

    {0x0103, 0x01},
    // mDELAY{10},
    // Y order
    {0x3880, 0x00},

    // mirror,flip
    {0x3c00, 0x03},
    {0x0101, 0x03},

    // interface
    {0x3805, 0x06},
    {0x3806, 0x06},
    {0x3807, 0x06},
    {0x3808, 0x14},
    {0x3809, 0xC4},
    {0x380a, 0x6C},
    {0x380b, 0x8C},
    {0x380c, 0x21},
    {0x380e, 0x01},

    // analog
    {0x3200, 0x05},
    {0x3201, 0xe8},
    {0x3202, 0x06},
    {0x3203, 0x08},
    {0x3208, 0xc3},
    {0x3280, 0x07},
    {0x3281, 0x05},
    {0x3282, 0xb2},
    {0x3283, 0x12},
    {0x3284, 0x0D},
    {0x3285, 0x65},
    {0x3286, 0x7c},
    {0x3287, 0x67},
    {0x3288, 0x00},
    {0x3289, 0x00},
    {0x328A, 0x00},
    {0x328B, 0x44},
    {0x328C, 0x34},
    {0x328D, 0x55},
    {0x328E, 0x00},
    {0x308a, 0x00},
    {0x308b, 0x01},
    {0x320C, 0x0C},
    {0x320E, 0x08},
    {0x3210, 0x22},
    {0x3211, 0x0f},
    {0x3212, 0xa0},
    {0x3213, 0x30},
    {0x3214, 0x80},
    {0x3215, 0x20},
    {0x3216, 0x50},
    {0x3217, 0x21},
    {0x3218, 0x31},
    {0x3219, 0x12},
    {0x321A, 0x00},
    {0x321B, 0x02},
    {0x321C, 0x00},

    // blc
    {0x3109, 0x04},
    {0x310a, 0x42},
    {0x3180, 0xf0},
    {0x3108, 0x01},

    // timing
    {0x0304, 0x00},
    {0x0342, 0x06},
    {0x034b, 0xb7},
    {0x3000, 0x81},
    {0x3001, 0x44},
    {0x3009, 0x05},
    {0x300b, 0x04},

    // aec
    {0x3083, 0x5C},
    {0x3084, 0x3f},
    {0x3085, 0x54},
    {0x3086, 0x00},
    {0x3087, 0x00},
    {0x3088, 0xf8},
    {0x3089, 0x14},
    {0x3f08, 0x10},

    // lens
    {0x3C80, 0x70},
    {0x3C81, 0x70},
    {0x3C82, 0x70},
    {0x3C83, 0x71},
    {0x3C84, 0x0},
    {0x3C85, 0x0},
    {0x3C86, 0x0},
    {0x3C87, 0x0},
    {0x3C88, 0x81},
    {0x3C89, 0x6d},
    {0x3C8A, 0x6d},
    {0x3C8B, 0x70},
    {0x3C8C, 0x0},
    {0x3C8D, 0x1},
    {0x3C8E, 0x10},
    {0x3C8F, 0xff},
    {0x3C90, 0x28},
    {0x3C91, 0x20},
    {0x3C92, 0x20},
    {0x3C93, 0x22},
    {0x3C94, 0x50},
    {0x3C95, 0x47},
    {0x3C96, 0x47},
    {0x3C97, 0x49},
    {0x3C98, 0xaa},

    // awb
    {0x0210, 0x05},
    {0x0211, 0x75},
    {0x0212, 0x04},
    {0x0213, 0x00},
    {0x0214, 0x04},
    {0x0215, 0x30},
    {0x3f8c, 0x75},
    {0x3f80, 0x0},
    {0x3f81, 0x57},
    {0x3f82, 0x66},
    {0x3f83, 0x65},
    {0x3f84, 0x73},
    {0x3f85, 0x9f},
    {0x3f86, 0x28},
    {0x3f87, 0x07},
    {0x3f88, 0x6b},
    {0x3f89, 0x79},
    {0x3f8a, 0x49},
    {0x3f8b, 0x8},

    // dns
    {0x3d00, 0x8f},
    {0x3d01, 0x30},
    {0x3d02, 0xff},
    {0x3d03, 0x30},
    {0x3d04, 0xff},
    {0x3d05, 0x08},
    {0x3d06, 0x10},
    {0x3d07, 0x10},
    {0x3d08, 0xf0},
    {0x3d09, 0x50},
    {0x3d0a, 0x30},
    {0x3d0b, 0x00},
    {0x3d0c, 0x00},
    {0x3d0d, 0x20},
    {0x3d0e, 0x00},
    {0x3d0f, 0x50},
    {0x3d10, 0x00},
    {0x3d11, 0x10},
    {0x3d12, 0x00},
    {0x3d13, 0xf0},

    // edge
    {0x3d20, 0x03},
    {0x3d21, 0x10},
    {0x3d22, 0x40},

    // ccm
    {0x3e00, 0x9e},
    {0x3e01, 0x82},
    {0x3e02, 0x8b},
    {0x3e03, 0x84},
    {0x3e04, 0x8a},
    {0x3e05, 0xb1},

    // gamma
    {0x3d80, 0x13},
    {0x3d81, 0x1f},
    {0x3d82, 0x2f},
    {0x3d83, 0x4a},
    {0x3d84, 0x56},
    {0x3d85, 0x62},
    {0x3d86, 0x6d},
    {0x3d87, 0x76},
    {0x3d88, 0x7f},
    {0x3d89, 0x87},
    {0x3d8a, 0x96},
    {0x3d8b, 0xa5},
    {0x3d8c, 0xbf},
    {0x3d8d, 0xd5},
    {0x3d8e, 0xea},
    {0x3d8f, 0x15},

    // sde
    {0x3e80, 0x21},
    {0x3e83, 0x01},
    {0x3e84, 0x1f},
    {0x3e85, 0x1f},
    {0x3e88, 0x9f},
    {0x3e89, 0x00},
    {0x3e8b, 0x10},
    {0x3e8c, 0x40},
    {0x3e8d, 0x80},
    {0x3e8e, 0x30},

    // stream on
    //	{0x0100, 0x01},
    {0x0205, 0x01},

    /*{0x0101,0x03},//
    {0x3009,0x04},//0x04->0x03 20141028
    {0x300b,0x04},//0x04->0x03 20141028
    {0x0340,0x04},//
    {0x0341,0xc8},//
    {0x0342,0x06},//
    {0x0343,0xb2},//
    {0x034c,0x06},//
    {0x034d,0x40},//
    {0x034e,0x04},//
    {0x034f,0xb0},//
    {0x0383,0x01},//
    {0x0387,0x01},//
    {0x3021,0x00},//
    {0x3022,0x01},//
    {0x3280,0x00},//
    {0x3281,0x05},//
    {0x3282,0x83},//
    {0x3283,0x12},//
    {0x3287,0x40},//
    {0x3288,0x57},//
    {0x3289,0x0c},//
    {0x328A,0x21},//  0x00->0x21 20141027
    {0x328B,0x44},//
    {0x328C,0x34},//
    {0x328D,0x55},//
    {0x328E,0x00},//
    {0x308a,0x00},//
    {0x308b,0x00},//
    {0x320C,0x00},//
    {0x320E,0x08},//
    {0x3210,0x22},//
    {0x3211,0x14},//
    {0x3212,0x28},//
    {0x3213,0x14},//
    {0x3214,0x14},//
    {0x3215,0x40},//
    {0x3216,0x1a},//
    {0x3217,0x50},//
    {0x3218,0x31},//
    {0x3219,0x12},//
    {0x321A,0x00},//
    {0x321B,0x04},//
    {0x321C,0x00},//
    {0x0303,0x01},//
    {0x0304,0x01},//
    {0x0305,0x03},//
    {0x0307,0x69},//
    {0x0309,0x10},//
    {0x3087,0xD0},// ;aec/agc: off  0x90->0xD0
    //{0x0202,0x04},//
    //{0x0203,0x50},//
    //{0x0205,0x1f},//
    {0x3089,0x10},//
    {0x3c03,0x00},//
    {0x3180,0xd0},//
    {0x3181,0x40},//
    {0x3d00,0x8d},// ;;af; BPC ON
    {0x3d01,0x01},//; 01just for single white pixel
    {0x3d07,0x10},//;
    {0x3d08,0x10},//;*/
};

static struct sensor_res_tab_info s_c2580_resolution_tab_raw[VENDOR_NUM] = {
    {
      .module_id = MODULE_SUNNY,
      .reg_tab = {
        {ADDR_AND_LEN_OF_ARRAY(c2580_common_init), PNULL, 0,
        .width = 0, .height = 0,
        .xclk_to_sensor = 24, .image_format = SENSOR_IMAGE_FORMAT_YUV422},

        {ADDR_AND_LEN_OF_ARRAY(c2580_800_600_setting), PNULL, 0,
        .width = 800, .height = 600,
        .xclk_to_sensor = 24, .image_format = SENSOR_IMAGE_FORMAT_YUV422},

        /*{ADDR_AND_LEN_OF_ARRAY(sp8407_snapshot_setting1), PNULL, 0,
        .width = 1600, .height = 1200,
        .xclk_to_sensor = 24, .image_format = SENSOR_IMAGE_FORMAT_YUV422}*/}
    }
/*If there are multiple modules,please add here*/
};

static SENSOR_TRIM_T s_c2580_resolution_trim_tab[VENDOR_NUM] = {
    {
     .module_id = MODULE_SUNNY,
     .trim_info = {
       {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
       {.trim_start_x = 0, .trim_start_y = 0,
        .trim_width = 800,   .trim_height = 600,
        .line_time = 54400, .bps_per_lane = 315,
        .frame_line = 616,
        .scaler_trim = {.x = 0, .y = 0, .w = 800, .h = 600}},
       /*{
        .trim_start_x = 0,.trim_start_y = 0,
        .trim_width = 1600,.trim_height = 1200,
        .line_time = 54400,.bps_per_lane = 315,
        .frame_line = 1224,
        .scaler_trim = {.x = 0, .y = 0, .w = 1600, .h = 1200}},*/
      }}

    /*If there are multiple modules,please add here*/
};


static const SENSOR_REG_T s_c2580_800x600_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
    /*video mode 0: ?fps*/
    {{0xffff, 0xff}},
    /* video mode 1:?fps*/
    {{0xffff, 0xff}},
    /* video mode 2:?fps*/
    {{0xffff, 0xff}},
    /* video mode 3:?fps*/
    {{0xffff, 0xff}}};

static const SENSOR_REG_T
    s_c2580_1600x1200_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
        /*video mode 0: ?fps*/
        {{0xffff, 0xff}},
        /* video mode 1:?fps*/
        {{0xffff, 0xff}},
        /* video mode 2:?fps*/
        {{0xffff, 0xff}},
        /* video mode 3:?fps*/
        {{0xffff, 0xff}}};

static SENSOR_VIDEO_INFO_T s_c2580_video_info[] = {
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    //{{{30, 30, 544, 100}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0,
    // 0}},(SENSOR_REG_T**)s_c2580_800x600_video_tab},
    //{{{15, 15, 544, 100}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0,
    // 0}},(SENSOR_REG_T**)s_c2580_1600x1200_video_tab},
    {{{15, 15, 544, 100}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
     (SENSOR_REG_T **)s_c2580_1600x1200_video_tab},
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL}};


static SENSOR_STATIC_INFO_T s_c2580_static_info[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .static_info = {
        .f_num = 220,
        .focal_length = 346,
        .max_fps = 0,
        .max_adgain = 8,
        .ois_supported = 0,
        .pdaf_supported = 0,
        .exp_valid_frame_num = 1,
        .clamp_level = 0,
        .adgain_valid_frame_num = 1,
        .fov_info = {{4.614f, 3.444f}, 4.222f}}
    }
    /*If there are multiple modules,please add here*/
};

static SENSOR_MODE_FPS_INFO_T s_c2580_mode_fps_info[VENDOR_NUM] = {
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

static struct sensor_module_info s_c2580_module_info_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .module_info = {
         .major_i2c_addr = 0x78 >> 1,
         .minor_i2c_addr = 0x78 >> 1,

         .reg_addr_value_bits = SENSOR_I2C_REG_16BIT | SENSOR_I2C_VAL_8BIT |
                                SENSOR_I2C_FREQ_400,

         .avdd_val = SENSOR_AVDD_2800MV,
         .iovdd_val = SENSOR_AVDD_1800MV,
         .dvdd_val = SENSOR_AVDD_1500MV,

         .image_pattern = SENSOR_IMAGE_PATTERN_YUV422_YUYV,

         .preview_skip_num = 1,
         .capture_skip_num = 1,
         .mipi_cap_skip_num = 0,
         .preview_deci_num = 0,
         .video_preview_deci_num = 0,

         .threshold_eb = 0,
         .threshold_mode = 0,
         .threshold_start = 0,
         .threshold_end = 0,

         .sensor_interface = {
              .type = SENSOR_INTERFACE_TYPE_CSI2,
              .bus_width = 1,
              .pixel_width = 10,
              .is_loose = 0,
          },

         .change_setting_skip_num = 3,
         .horizontal_view_angle = 48,
         .vertical_view_angle = 48
      }
    }
/*If there are multiple modules,please add here*/
};

static struct sensor_ic_ops s_c2580_ops_tab;
struct sensor_raw_info *s_c2580_mipi_raw_info_ptr = NULL;

SENSOR_INFO_T g_c2580_mipi_raw_info = {
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
    .reset_pulse_width = 10,

    .power_down_level = SENSOR_LOW_LEVEL_PWDN,

    .identify_count = 1,
    .identify_code = {{0x0000, 0x02},
                      {0x0001, 0x02}},

    .source_width_max = 1600,
    .source_height_max = 1200,
    .name = (cmr_s8 *) SENSOR_NAME,

    .image_format = SENSOR_IMAGE_FORMAT_YUV422,

    .resolution_tab_info_ptr = s_c2580_resolution_tab_raw,
    .sns_ops = &s_c2580_ops_tab,
    .module_info_tab = s_c2580_module_info_tab,
    .module_info_tab_size = ARRAY_SIZE(s_c2580_module_info_tab),

    .raw_info_ptr = &s_c2580_mipi_raw_info_ptr,
    .video_tab_info_ptr = s_c2580_video_info,

    .sensor_version_info = (cmr_s8 *) "c2580_v1",
};
#endif
