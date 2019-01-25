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
#ifndef _SENSOR_S5K3P8SM_MIPI_RAW_
#define _SENSOR_S5K3P8SM_MIPI_RAW_

#include "cutils/properties.h"
#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"
//#include "sensor_s5k3p8sm_raw_param.c"
#include "isp_param_file_update.h"
#if defined(CONFIG_CAMERA_OIS_FUNC)
#include "OIS_main.h"
#else
#endif
#include "parameters/sensor_s5k3p8sm_raw_param_v3.c"

#define VENDOR_NUM 1
#define S5K3P8SM_I2C_ADDR_W 0x10
#define S5K3P8SM_I2C_ADDR_R 0x10
//#define S5K3P8SM_I2C_ADDR_W        0x2d
//#define S5K3P8SM_I2C_ADDR_R        0x2d
#define S5K3P8SM_RAW_PARAM_COM 0x0000

#ifndef RAW_INFO_END_ID
#define RAW_INFO_END_ID 0x71717567
#endif

#if 0
SENSOR_HW_HANDLE _s5k3p8sm_Create(void *privatedata);
void _s5k3p8sm_Destroy(SENSOR_HW_HANDLE handle);
static unsigned long _s5k3p8sm_GetResolutionTrimTab(SENSOR_HW_HANDLE handle,
                                                    unsigned long param);
static unsigned long _s5k3p8sm_Identify(SENSOR_HW_HANDLE handle,
                                        unsigned long param);
static uint32_t _s5k3p8sm_GetRawInof(SENSOR_HW_HANDLE handle);
static unsigned long _s5k3p8sm_StreamOn(SENSOR_HW_HANDLE handle,
                                        unsigned long param);
static unsigned long _s5k3p8sm_StreamOff(SENSOR_HW_HANDLE handle,
                                         unsigned long param);
static uint32_t _s5k3p8sm_com_Identify_otp(SENSOR_HW_HANDLE handle,
                                           void *param_ptr);
static unsigned long _s5k3p8sm_PowerOn(SENSOR_HW_HANDLE handle,
                                       unsigned long power_on);
static unsigned long _s5k3p8sm_write_exposure(SENSOR_HW_HANDLE handle,
                                              unsigned long param);
static unsigned long _s5k3p8sm_write_gain(SENSOR_HW_HANDLE handle,
                                          unsigned long param);
static unsigned long _s5k3p8sm_access_val(SENSOR_HW_HANDLE handle,
                                          unsigned long param);
static unsigned long s5k3p8sm_write_af(SENSOR_HW_HANDLE handle,
                                       unsigned long param);
static unsigned long _s5k3p8sm_GetExifInfo(SENSOR_HW_HANDLE handle,
                                           unsigned long param);
static unsigned long _s5k3p8sm_BeforeSnapshot(SENSOR_HW_HANDLE handle,
                                              unsigned long param);
static uint16_t _s5k3p8sm_get_shutter(SENSOR_HW_HANDLE handle);
static unsigned long _s5k3p8sm_ex_write_exposure(SENSOR_HW_HANDLE handle,
                                                 unsigned long param);
#endif

static cmr_int s5k3p8sm_drv_com_Identify_otp(cmr_handle handle,
                                           void *param_ptr);

static const struct raw_param_info_tab s_s5k3p8sm_raw_param_tab[] = {
    {S5K3P8SM_RAW_PARAM_COM, NULL, s5k3p8sm_drv_com_Identify_otp, NULL},
    {RAW_INFO_END_ID, PNULL, PNULL, PNULL}};

static const SENSOR_REG_T s5k3p8sm_common_init[] = {
    {0x6028, 0x4000},
    {0x6010, 0x0001}, // Reset
    {0xffff, 0x000a}, // must add delay >3ms
    {0x6214, 0x7971},
    {0x6218, 0x7150},
    {0x6028, 0x2000}, // TP
    {0x602A, 0x3074},
    {0x6F12, 0x0000},
    {0x6F12, 0x0000},
    {0x6F12, 0x0000},
    {0x6F12, 0x0000},
    {0x6F12, 0x0000},
    {0x6F12, 0x0000},
    {0x6F12, 0x0449},
    {0x6F12, 0x0348},
    {0x6F12, 0x0860},
    {0x6F12, 0xCA8D},
    {0x6F12, 0x101A},
    {0x6F12, 0x8880},
    {0x6F12, 0x00F0},
    {0x6F12, 0x52B8},
    {0x6F12, 0x2000},
    {0x6F12, 0x31B8},
    {0x6F12, 0x2000},
    {0x6F12, 0x1E80},
    {0x6F12, 0x0000},
    {0x6F12, 0x0000},
    {0x6F12, 0x0000},
    {0x6F12, 0x0000},
    {0x6F12, 0x70B5},
    {0x6F12, 0x0646},
    {0x6F12, 0x2C48},
    {0x6F12, 0x0022},
    {0x6F12, 0x0168},
    {0x6F12, 0x0C0C},
    {0x6F12, 0x8DB2},
    {0x6F12, 0x2946},
    {0x6F12, 0x2046},
    {0x6F12, 0x00F0},
    {0x6F12, 0x61F8},
    {0x6F12, 0x3046},
    {0x6F12, 0x00F0},
    {0x6F12, 0x63F8},
    {0x6F12, 0x2748},
    {0x6F12, 0x284A},
    {0x6F12, 0x0188},
    {0x6F12, 0x1180},
    {0x6F12, 0x911C},
    {0x6F12, 0x4088},
    {0x6F12, 0x0880},
    {0x6F12, 0x2946},
    {0x6F12, 0x2046},
    {0x6F12, 0xBDE8},
    {0x6F12, 0x7040},
    {0x6F12, 0x0122},
    {0x6F12, 0x00F0},
    {0x6F12, 0x50B8},
    {0x6F12, 0x2DE9},
    {0x6F12, 0xF041},
    {0x6F12, 0x8046},
    {0x6F12, 0x1E48},
    {0x6F12, 0x1446},
    {0x6F12, 0x0F46},
    {0x6F12, 0x4068},
    {0x6F12, 0x0022},
    {0x6F12, 0x85B2},
    {0x6F12, 0x060C},
    {0x6F12, 0x2946},
    {0x6F12, 0x3046},
    {0x6F12, 0x00F0},
    {0x6F12, 0x42F8},
    {0x6F12, 0x1B48},
    {0x6F12, 0x408C},
    {0x6F12, 0x70B1},
    {0x6F12, 0x94F8},
    {0x6F12, 0x6B00},
    {0x6F12, 0x4021},
    {0x6F12, 0xB1FB},
    {0x6F12, 0xF0F0},
    {0x6F12, 0x1849},
    {0x6F12, 0x194A},
    {0x6F12, 0x098B},
    {0x6F12, 0x5288},
    {0x6F12, 0x891A},
    {0x6F12, 0x0901},
    {0x6F12, 0x91FB},
    {0x6F12, 0xF0F0},
    {0x6F12, 0x84B2},
    {0x6F12, 0x05E0},
    {0x6F12, 0x2246},
    {0x6F12, 0x3946},
    {0x6F12, 0x4046},
    {0x6F12, 0x00F0},
    {0x6F12, 0x35F8},
    {0x6F12, 0x0446},
    {0x6F12, 0x0122},
    {0x6F12, 0x2946},
    {0x6F12, 0x3046},
    {0x6F12, 0x00F0},
    {0x6F12, 0x25F8},
    {0x6F12, 0x2046},
    {0x6F12, 0xBDE8},
    {0x6F12, 0xF081},
    {0x6F12, 0x10B5},
    {0x6F12, 0x0022},
    {0x6F12, 0xAFF2},
    {0x6F12, 0x9B01},
    {0x6F12, 0x0C48},
    {0x6F12, 0x00F0},
    {0x6F12, 0x2AF8},
    {0x6F12, 0x054C},
    {0x6F12, 0x0022},
    {0x6F12, 0xAFF2},
    {0x6F12, 0x6F01},
    {0x6F12, 0x2060},
    {0x6F12, 0x0948},
    {0x6F12, 0x00F0},
    {0x6F12, 0x22F8},
    {0x6F12, 0x6060},
    {0x6F12, 0x10BD},
    {0x6F12, 0x0000},
    {0x6F12, 0x2000},
    {0x6F12, 0x31B0},
    {0x6F12, 0x2000},
    {0x6F12, 0x4A00},
    {0x6F12, 0x4000},
    {0x6F12, 0x950C},
    {0x6F12, 0x2000},
    {0x6F12, 0x2F10},
    {0x6F12, 0x2000},
    {0x6F12, 0x1B10},
    {0x6F12, 0x2000},
    {0x6F12, 0x2F40},
    {0x6F12, 0x0000},
    {0x6F12, 0x15E9},
    {0x6F12, 0x0000},
    {0x6F12, 0x9ECD},
    {0x6F12, 0x40F2},
    {0x6F12, 0xA37C},
    {0x6F12, 0xC0F2},
    {0x6F12, 0x000C},
    {0x6F12, 0x6047},
    {0x6F12, 0x41F2},
    {0x6F12, 0xE95C},
    {0x6F12, 0xC0F2},
    {0x6F12, 0x000C},
    {0x6F12, 0x6047},
    {0x6F12, 0x49F6},
    {0x6F12, 0xCD6C},
    {0x6F12, 0xC0F2},
    {0x6F12, 0x000C},
    {0x6F12, 0x6047},
    {0x6F12, 0x4DF6},
    {0x6F12, 0x1B0C},
    {0x6F12, 0xC0F2},
    {0x6F12, 0x000C},
    {0x6F12, 0x6047},
    {0x6F12, 0x3108},
    {0x6F12, 0x021C},
    {0x6F12, 0x0000},
    {0x6F12, 0x0005},
    {0x602A, 0x4A00},
    {0x6F12, 0x0088},
    {0x6F12, 0x0D70},
    {0x6028, 0x4000},
    {0x0202, 0x0006}, // 0x0100 },
    //{ 0x0204, 0x0140 },
    {0x6F12, 0x0D70},
    {0x0200, 0x0618},
    {0x021E, 0x0300},
    {0x021C, 0x0000},
    {0x6028, 0x2000},
    {0x602A, 0x162C},
    {0x6F12, 0x8080},
    {0x602A, 0x164C},
    {0x6F12, 0x5555},
    {0x6F12, 0x5500},
    {0x602A, 0x166C},
    {0x6F12, 0x4040},
    {0x6F12, 0x4040},
    {0x6028, 0x4000},
    {0x3606, 0x0103},
    {0xF496, 0x004D},
    {0xF470, 0x0008},
    {0xF43A, 0x0015},
    {0xF484, 0x0006},
    {0xF442, 0x44C6},
    {0xF408, 0xFFF7},
    {0xF494, 0x1010},
    {0xF4D4, 0x0038},
    {0xF4D6, 0x0039},
    {0xF4D8, 0x0034},
    {0xF4DA, 0x0035},
    {0xF4DC, 0x0038},
    {0xF4DE, 0x0039},
    {0xF47C, 0x001F},
    {0xF62E, 0x00C5},
    {0xF630, 0x00CD},
    {0xF632, 0x00DD},
    {0xF634, 0x00E5},
    {0xF636, 0x00F5},
    {0xF638, 0x00FD},
    {0xF63A, 0x010D},
    {0xF63C, 0x0115},
    {0xF63E, 0x0125},
    {0xF640, 0x012D},
    {0x3070, 0x0100},
    {0x3090, 0x0904},
    {0x31C0, 0x00C8},
    {0x6028, 0x2000},
    {0x602A, 0x18F0},
    {0x6F12, 0x0100},
    {0x602A, 0x18F6},
    {0x6F12, 0x002F},
    {0x6F12, 0x002F},
    {0x6F12, 0xF440},
};

// 4632x3480 30FPS v560M mipi1392M 4lane
// frame length	0x0E2A
// 1H time	91.9

// 4640x3488 30FPS Mclk24 v560M mipi1464Mbps/lane 4lane
// frame length	0x0E3B
// 1H time	91.4
static const SENSOR_REG_T s5k3p8sm_4640x3488_4lane_setting[] = {
    {0x6028, 0x4000}, {0x0136, 0x1800}, {0x0304, 0x0006}, {0x0306, 0x0069},
    {0x0302, 0x0001}, {0x0300, 0x0003}, {0x030C, 0x0004}, {0x030E, 0x007A},
    {0x030A, 0x0001}, {0x0308, 0x0008}, {0x0344, 0x0008}, {0x0346, 0x0008},
    {0x0348, 0x1227}, {0x034A, 0x0DA7}, {0x034C, 0x1220}, {0x034E, 0x0DA0},
    {0x0408, 0x0000}, {0x0900, 0x0011}, {0x0380, 0x0001}, {0x0382, 0x0001},
    {0x0384, 0x0001}, {0x0386, 0x0001}, {0x0400, 0x0000}, {0x0404, 0x0010},
    {0x0342, 0x1400}, {0x0340, 0x0E3B}, {0x0B0E, 0x0000}, {0x0216, 0x0000},
    {0x3604, 0x0002}, {0x3664, 0x0019}, {0x3004, 0x0003}, {0x3000, 0x0001},
    {0x317A, 0x0130}, {0x1006, 0x0002}, {0x6028, 0x2000}, {0x602A, 0x19E0},
    {0x6F12, 0x0001}, {0x602A, 0x18F6}, {0x6F12, 0x002F}, {0x6F12, 0x002F},
    {0x31A4, 0x0102}, {0x0100, 0x0000}, // steam off
    //{ 0x0100, 0x0100 },//streaming on
};


// 2320x1744 30FPS Mclk24 v560M mipi1464Mbps/lane 4lane
// frame length	0x3643
// 1H time	91.4
static const SENSOR_REG_T s5k3p8sm_2320x1744_4lane_setting[] = {
    {0x6028, 0x4000}, {0x0136, 0x1800}, {0x0304, 0x0006}, {0x0306, 0x0069},
    {0x0302, 0x0001}, {0x0300, 0x0003}, {0x030C, 0x0004}, {0x030E, 0x007A},
    {0x030A, 0x0001}, {0x0308, 0x0008}, {0x0344, 0x0008}, {0x0346, 0x0008},
    {0x0348, 0x1227}, {0x034A, 0x0DA7}, {0x034C, 0x0910}, {0x034E, 0x06D0},
    {0x0408, 0x0000}, {0x0900, 0x0112}, {0x0380, 0x0001}, {0x0382, 0x0001},
    {0x0384, 0x0001}, {0x0386, 0x0003}, {0x0400, 0x0001}, {0x0404, 0x0020},
    {0x0342, 0x1400}, {0x0340, 0x0E3B}, {0x0B0E, 0x0000}, {0x0216, 0x0000},
    {0x3604, 0x0002}, {0x3664, 0x0019}, {0x3004, 0x0003}, {0x3000, 0x0001},
    {0x317A, 0x00A0}, {0x1006, 0x0002}, {0x6028, 0x2000}, {0x602A, 0x19E0},
    {0x6F12, 0x0001}, {0x602A, 0x18F6}, {0x6F12, 0x002F}, {0x6F12, 0x002F},
    {0x31A4, 0x0102},
    //{ 0x0100, 0x0100 },//streaming on
};

/* 2320 x 1748 30fps 720Mbps */
static const SENSOR_REG_T s5k3p8sm_2320x1748_4lane_setting[] = {
#if 1

#else
    {0x6028, 0x4000}, {0x0344, 0x0000}, {0x0346, 0x0000}, {0x0348, 0x090F},
    {0x034A, 0x06D3}, {0x034C, 0x0910}, {0x034E, 0x06D4}, {0x3002, 0x0001},
    {0x0136, 0x1800}, {0x0304, 0x0006}, {0x0306, 0x008C}, {0x0302, 0x0001},
    {0x0300, 0x0008}, {0x030C, 0x0004}, {0x030E, 0x0078}, {0x030A, 0x0001},
    {0x0308, 0x0008}, {0x3008, 0x0001}, {0x0800, 0x0000}, {0x0200, 0x0200},
    {0x0202, 0x0100}, {0x021C, 0x0200}, {0x021E, 0x0100}, {0x0342, 0x141C},
    {0x0340, 0x0708}, {0x3072, 0x03C0},
#endif
};

// 1920X1080 60FPS v560M mipi660M 4lane 2x2binning
// frame length	0x06E7
// 1H time	94.3
static const SENSOR_REG_T s5k3p8sm_1920x1080_4lane_setting[] = {
    {0x6028, 0x2000}, {0x602A, 0x2E26}, {0x6F12, 0x0103}, {0x6028, 0x4000},
    {0x3D7C, 0x0010}, {0x3D88, 0x0064}, {0x3D8A, 0x0068}, {0x0344, 0x018C},
    {0x0346, 0x029C}, {0x0348, 0x109B}, {0x034A, 0x0B0B}, {0x034C, 0x0780},
    {0x034E, 0x0438}, {0x0408, 0x0004}, {0x0900, 0x0122}, {0x0380, 0x0001},
    {0x0382, 0x0003}, {0x0384, 0x0001}, {0x0386, 0x0003}, {0x0400, 0x0001},
    {0x0404, 0x0010}, {0x0114, 0x0300}, {0x0110, 0x0002}, {0x0136, 0x1800},
    {0x0304, 0x0006}, {0x0306, 0x008C}, {0x0302, 0x0001}, {0x0300, 0x0004},
    {0x030C, 0x0004}, {0x030E, 0x0037}, {0x030A, 0x0001}, {0x0308, 0x0008},
    {0x0342, 0x14A2}, {0x0340, 0x06E7}, {0x0B0E, 0x0100}, {0x0216, 0x0000},
    // { 0x0100, 0x0000 }, //steam off
    // { 0x0100, 0x0100 },//steam on
};

// 1920X1080 30FPS v560M mipi660M 4lane 2x2binning
// frame length	0x0DCE
// 1H time	94.3
static const SENSOR_REG_T s5k3p8sm_1920x1080_4lane_30fps_setting[] = {
    {0x6028, 0x2000}, {0x602A, 0x2E26}, {0x6F12, 0x0103}, {0x6028, 0x4000},
    {0x3D7C, 0x0010}, {0x3D88, 0x0064}, {0x3D8A, 0x0068}, {0x0344, 0x018C},
    {0x0346, 0x029C}, {0x0348, 0x109B}, {0x034A, 0x0B0B}, {0x034C, 0x0780},
    {0x034E, 0x0438}, {0x0408, 0x0004}, {0x0900, 0x0122}, {0x0380, 0x0001},
    {0x0382, 0x0003}, {0x0384, 0x0001}, {0x0386, 0x0003}, {0x0400, 0x0001},
    {0x0404, 0x0010}, {0x0114, 0x0300}, {0x0110, 0x0002}, {0x0136, 0x1800},
    {0x0304, 0x0006}, {0x0306, 0x008C}, {0x0302, 0x0001}, {0x0300, 0x0004},
    {0x030C, 0x0004}, {0x030E, 0x0037}, {0x030A, 0x0001}, {0x0308, 0x0008},
    {0x0342, 0x14A2}, {0x0340, 0x0DCE}, {0x0B0E, 0x0100}, {0x0216, 0x0000},
    //{0x0100,0x0100	}, //steam on
};

// 1280x720 120FPS v560M mipi660M 4lane 3x3binning
// frame length	0x038A
// 1H time	91.9

// 1280x720 120FPS Mclk24 v560M mipi1464Mbps/lane 4lane
// frame length	0x03C5
// 1H time	86.3
static const SENSOR_REG_T s5k3p8sm_1280x720_4lane_setting[] = {
    {0x6028, 0x4000}, {0x0136, 0x1800}, {0x0304, 0x0006}, {0x0306, 0x008C},
    {0x0302, 0x0001}, {0x0300, 0x0004}, {0x030C, 0x0004}, {0x030E, 0x007A},
    {0x030A, 0x0001}, {0x0308, 0x0008}, {0x0344, 0x0198}, {0x0346, 0x02A0},
    {0x0348, 0x1097}, {0x034A, 0x0B0F}, {0x034C, 0x0500}, {0x034E, 0x02D0},
    {0x0408, 0x0000}, {0x0900, 0x0113}, {0x0380, 0x0001}, {0x0382, 0x0001},
    {0x0384, 0x0001}, {0x0386, 0x0005}, {0x0400, 0x0001}, {0x0404, 0x0030},
    {0x0342, 0x12E0}, {0x0340, 0x03C5}, {0x0B0E, 0x0100}, {0x0216, 0x0000},
    {0x3604, 0x0001}, {0x3664, 0x0011}, {0x3004, 0x0004}, {0x3000, 0x0000},
    {0x317A, 0x0007}, {0x1006, 0x0003}, {0x6028, 0x2000}, {0x602A, 0x19E0},
    {0x6F12, 0x0001}, {0x602A, 0x18F6}, {0x6F12, 0x00AF}, {0x6F12, 0x00AF},
    {0x31A4, 0x0102}, {0x0100, 0x0000}, // steam off
    //{ 0x0100, 0x0100 },//streaming on
};

static struct sensor_res_tab_info s_s5k3p8sm_resolution_Tab_RAW[VENDOR_NUM] = {
    {
      .module_id = MODULE_SUNNY,
      .reg_tab = {
        {ADDR_AND_LEN_OF_ARRAY(s5k3p8sm_common_init), PNULL, 0,
        .width = 0, .height = 0,
        .xclk_to_sensor = 24, .image_format = SENSOR_IMAGE_FORMAT_RAW},

        {ADDR_AND_LEN_OF_ARRAY(s5k3p8sm_1280x720_4lane_setting), PNULL, 0,
        .width = 1280, .height = 720,
        .xclk_to_sensor = 24, .image_format = SENSOR_IMAGE_FORMAT_RAW},

        {ADDR_AND_LEN_OF_ARRAY(s5k3p8sm_2320x1744_4lane_setting), PNULL, 0,
        .width = 2320, .height = 1744,
        .xclk_to_sensor = 24, .image_format = SENSOR_IMAGE_FORMAT_RAW},

        {ADDR_AND_LEN_OF_ARRAY(s5k3p8sm_4640x3488_4lane_setting), PNULL, 0,
        .width = 4640, .height = 3488,
        .xclk_to_sensor = 24, .image_format = SENSOR_IMAGE_FORMAT_RAW}}
    }
/*If there are multiple modules,please add here*/
};

static SENSOR_TRIM_T s_s5k3p8sm_Resolution_Trim_Tab[VENDOR_NUM] = {
    {
     .module_id = MODULE_SUNNY,
     .trim_info = {
       {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
       {.trim_start_x = 0, .trim_start_y = 0,
        .trim_width = 1280, .trim_height = 720,
        .line_time = 8629, .bps_per_lane = 1320,
        .frame_line = 965,
        .scaler_trim = {.x = 0, .y = 0, .w = 1280, .h = 720}},
       {
        .trim_start_x = 0,.trim_start_y = 0,
        .trim_width = 2320,.trim_height = 1744,
        .line_time = 9140,.bps_per_lane = 1320,
        .frame_line = 3693,
        .scaler_trim = {.x = 0, .y = 0, .w = 2320, .h = 1744}},
       {
        .trim_start_x = 0, .trim_start_y = 0,
        .trim_width = 4640,   .trim_height = 3488,
        .line_time = 9140, .bps_per_lane = 2784,
        .frame_line = 3693,
        .scaler_trim = {.x = 0, .y = 0, .w = 4640, .h = 3488}},
      }}

    /*If there are multiple modules,please add here*/
};

static const SENSOR_REG_T
    s_s5k3p8sm_2576x1932_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
        /*video mode 0: ?fps*/
        {{0xffff, 0xff}},
        /* video mode 1:?fps*/
        {{0xffff, 0xff}},
        /* video mode 2:?fps*/
        {{0xffff, 0xff}},
        /* video mode 3:?fps*/
        {{0xffff, 0xff}}};

static SENSOR_VIDEO_INFO_T s_s5k3p8sm_video_info[] = {
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{30, 30, 164, 100}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
     (SENSOR_REG_T **)s_s5k3p8sm_2576x1932_video_tab},
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL}};

static SENSOR_STATIC_INFO_T s_s5k3p8sm_static_info[VENDOR_NUM] = {
    {
     .module_id = MODULE_SUNNY,
     .static_info = {
         .f_num = 220,
         .focal_length = 357,
         .max_fps = 0,
         .max_adgain = 16 * 256,
         .ois_supported = 0,
         .pdaf_supported = SENSOR_PDAF_TYPE3_ENABLE,
         .exp_valid_frame_num = 1,
         .clamp_level = 64,
         .adgain_valid_frame_num = 1,
         .fov_info = {{4.640f, 3.488f}, 3.563f}}
    }
    /*If there are multiple modules,please add here*/
};

static SENSOR_MODE_FPS_INFO_T s_s5k3p8sm_mode_fps_info[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
       {.is_init = 0,
         /*1:mode,2:max_fps,3:min_fps,4:is_high_fps,5:high_fps_skip_num*/
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

static struct sensor_module_info s_s5k3p8sm_module_info_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .module_info = {
         .major_i2c_addr = S5K3P8SM_I2C_ADDR_W,
         .minor_i2c_addr = S5K3P8SM_I2C_ADDR_W,

         .reg_addr_value_bits = SENSOR_I2C_REG_16BIT | SENSOR_I2C_VAL_16BIT |
                                SENSOR_I2C_FREQ_400,

         .avdd_val = SENSOR_AVDD_2800MV,
         .iovdd_val = SENSOR_AVDD_1800MV,
         .dvdd_val = SENSOR_AVDD_1000MV,

         .image_pattern = SENSOR_IMAGE_PATTERN_RAWRGB_GR,

         .preview_skip_num = 3,
         .capture_skip_num = 3,
         .flash_capture_skip_num = 6,
         .mipi_cap_skip_num = 0,
         .preview_deci_num = 0,
         .video_preview_deci_num = 0,

         .sensor_interface = {
              .type = SENSOR_INTERFACE_TYPE_CSI2,
              .bus_width = 4,
              .pixel_width = 10,
              .is_loose = 0,
          },

         .change_setting_skip_num = 3,
         .horizontal_view_angle = 48,
         .vertical_view_angle = 48
      }
    }
};

static struct sensor_ic_ops s5k3p8sm_ops_tab;
static struct sensor_raw_info *s_s5k3p8sm_mipi_raw_info_ptr =
    &s_s5k3p8sm_mipi_raw_info;

SENSOR_INFO_T g_s5k3p8sm_mipi_raw_info = {
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
    .reset_pulse_width = 5,
    .power_down_level = SENSOR_LOW_LEVEL_PWDN,

    .identify_count = 1,
    .identify_code = {{ .reg_addr = 0x0, .reg_value = 0x5e},
                      { .reg_addr = 0x1, .reg_value = 0x20}},

    .source_width_max = 4640,
    .source_height_max = 3488,
    .name = (cmr_s8 *) "s5k3p8sm_mipi_raw",

    .image_format = SENSOR_IMAGE_FORMAT_RAW,

    .resolution_tab_info_ptr = s_s5k3p8sm_resolution_Tab_RAW,
    .sns_ops = &s5k3p8sm_ops_tab,
    .raw_info_ptr = &s_s5k3p8sm_mipi_raw_info_ptr,
    .module_info_tab = s_s5k3p8sm_module_info_tab,
    .module_info_tab_size = ARRAY_SIZE(s_s5k3p8sm_module_info_tab),
    .ext_info_ptr = NULL,

    .video_tab_info_ptr = s_s5k3p8sm_video_info,
    .sensor_version_info = (cmr_s8 *) "s5k3p8sm_truly_v1",
};
#endif
