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
#ifndef _S5K4H8YX_MIPI_RAW_H_
#define _S5K4H8YX_MIPI_RAW_H_

#include "cutils/properties.h"
#include <utils/Log.h>
//#include <dlfcn.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"
//#if defined(CONFIG_CAMERA_ISP_VERSION_V3) ||
// defined(CONFIG_CAMERA_ISP_VERSION_V4)
#include "parameters/sensor_s5k4h8yx_raw_param_v3.c"
//#else
//#endif
#include "isp_param_file_update.h"

#ifndef RAW_INFO_END_ID
#define RAW_INFO_END_ID 0x71717567
#endif

#define VENDOR_NUM 1
#define S5K4H8YX_I2C_ADDR_W 0x10
#define S5K4H8YX_I2C_ADDR_R 0x10

//#define S5K4H8YX_2_LANES
#define S5K4H8YX_4_LANES
#define SENSOR_NAME  "s5k4h8yx_mipi_raw"

/**
 * Name of the sensor_raw_param as a string
 */
#define S5K4H8YX_RAW_PARAM_AS_STR "S5K4H8YXRP"



#ifdef FEATURE_OTP
#define MODULE_ID_s5k4h8yx_ofilm 0x0007
#define MODULE_ID_s5k4h8yx_truly 0x0002
#define LSC_PARAM_QTY 240

struct otp_info_t {
    uint16_t flag;
    uint16_t module_id;
    uint16_t lens_id;
    uint16_t vcm_id;
    uint16_t vcm_driver_id;
    uint16_t year;
    uint16_t month;
    uint16_t day;
    uint16_t rg_ratio_current;
    uint16_t bg_ratio_current;
    uint16_t rg_ratio_typical;
    uint16_t bg_ratio_typical;
    uint16_t r_current;
    uint16_t g_current;
    uint16_t b_current;
    uint16_t r_typical;
    uint16_t g_typical;
    uint16_t b_typical;
    uint16_t vcm_dac_start;
    uint16_t vcm_dac_inifity;
    uint16_t vcm_dac_macro;
    uint16_t lsc_param[LSC_PARAM_QTY];
};

#include "sensor_s5k4h8yx_ofilm_otp.c"

static const struct raw_param_info_tab s_s5k4h8yx_raw_param_tab[] = {
    {MODULE_ID_s5k4h8yx_truly, NULL, s5k4h8yx_ofilm_identify_otp,
     s5k4h8yx_ofilm_update_otp},
    {RAW_INFO_END_ID, PNULL, PNULL, PNULL}};
#endif

static cmr_int s5k4h8yx_drv_write_exp_dummy(cmr_handle handle,
                                                    cmr_u16 expsure_line,
                                                    cmr_u16 dummy_line,
                                                    cmr_u16 size_index);
static cmr_int s5k4h8yx_drv_set_shutter(cmr_handle handle,
                                                 cmr_u16 shutter);
static cmr_u16 s5k4h8yx_drv_get_shutter(cmr_handle handle);
static cmr_int s5k4h8yx_drv_set_vts(cmr_handle handle, cmr_u16 VTS);
static cmr_u16 s5k4h8yx_drv_get_vts(cmr_handle handle);
static cmr_int s5k4h8yx_drv_read_gain(cmr_handle handle, cmr_u32 *param);
static cmr_int s5k4h8yx_drv_get_exif_info(cmr_handle handle,
                                                      void** param);



////initial setting V24 F1X9
static const SENSOR_REG_T s5k4h8yx_common_init_new[] = {
    {0x6028, 0x2000}, {0x602A, 0x1FD0}, {0x6F12, 0x0448}, {0x6F12, 0x0349},
    {0x6F12, 0x0160}, {0x6F12, 0xC26A}, {0x6F12, 0x511A}, {0x6F12, 0x8180},
    {0x6F12, 0x00F0}, {0x6F12, 0x60B8}, {0x6F12, 0x2000}, {0x6F12, 0x20E8},
    {0x6F12, 0x2000}, {0x6F12, 0x13A0}, {0x6F12, 0x0000}, {0x6F12, 0x0000},
    {0x6F12, 0x0000}, {0x6F12, 0x0000}, {0x6F12, 0x38B5}, {0x6F12, 0x0021},
    {0x6F12, 0x0446}, {0x6F12, 0x8DF8}, {0x6F12, 0x0010}, {0x6F12, 0x00F5},
    {0x6F12, 0xB470}, {0x6F12, 0x0122}, {0x6F12, 0x6946}, {0x6F12, 0x00F0},
    {0x6F12, 0x59F8}, {0x6F12, 0x9DF8}, {0x6F12, 0x0000}, {0x6F12, 0xFF28},
    {0x6F12, 0x05D0}, {0x6F12, 0x0020}, {0x6F12, 0x08B1}, {0x6F12, 0x04F2},
    {0x6F12, 0x6914}, {0x6F12, 0x2046}, {0x6F12, 0x38BD}, {0x6F12, 0x0120},
    {0x6F12, 0xF8E7}, {0x6F12, 0x10B5}, {0x6F12, 0x92B0}, {0x6F12, 0x0C46},
    {0x6F12, 0x4822}, {0x6F12, 0x6946}, {0x6F12, 0x00F0}, {0x6F12, 0x46F8},
    {0x6F12, 0x0020}, {0x6F12, 0x6946}, {0x6F12, 0x04EB}, {0x6F12, 0x4003},
    {0x6F12, 0x0A5C}, {0x6F12, 0x02F0}, {0x6F12, 0x0F02}, {0x6F12, 0x04F8},
    {0x6F12, 0x1020}, {0x6F12, 0x0A5C}, {0x6F12, 0x401C}, {0x6F12, 0x1209},
    {0x6F12, 0x5A70}, {0x6F12, 0x4828}, {0x6F12, 0xF2D3}, {0x6F12, 0x12B0},
    {0x6F12, 0x10BD}, {0x6F12, 0x2DE9}, {0x6F12, 0xF041}, {0x6F12, 0x164E},
    {0x6F12, 0x0F46}, {0x6F12, 0x06F1}, {0x6F12, 0x1105}, {0x6F12, 0xA236},
    {0x6F12, 0xB0B1}, {0x6F12, 0x1449}, {0x6F12, 0x1248}, {0x6F12, 0x0968},
    {0x6F12, 0x0078}, {0x6F12, 0xB1F8}, {0x6F12, 0x6A10}, {0x6F12, 0xC007},
    {0x6F12, 0x0ED0}, {0x6F12, 0x0846}, {0x6F12, 0xFFF7}, {0x6F12, 0xBEFF},
    {0x6F12, 0x84B2}, {0x6F12, 0x2946}, {0x6F12, 0x2046}, {0x6F12, 0xFFF7},
    {0x6F12, 0xD0FF}, {0x6F12, 0x4FF4}, {0x6F12, 0x9072}, {0x6F12, 0x3146},
    {0x6F12, 0x04F1}, {0x6F12, 0x4800}, {0x6F12, 0x00F0}, {0x6F12, 0x16F8},
    {0x6F12, 0x002F}, {0x6F12, 0x05D0}, {0x6F12, 0x3146}, {0x6F12, 0x2846},
    {0x6F12, 0xBDE8}, {0x6F12, 0xF041}, {0x6F12, 0x00F0}, {0x6F12, 0x13B8},
    {0x6F12, 0xBDE8}, {0x6F12, 0xF081}, {0x6F12, 0x0022}, {0x6F12, 0xAFF2},
    {0x6F12, 0x5501}, {0x6F12, 0x0348}, {0x6F12, 0x00F0}, {0x6F12, 0x10B8},
    {0x6F12, 0x2000}, {0x6F12, 0x0C40}, {0x6F12, 0x2000}, {0x6F12, 0x0560},
    {0x6F12, 0x0000}, {0x6F12, 0x152D}, {0x6F12, 0x48F6}, {0x6F12, 0x296C},
    {0x6F12, 0xC0F2}, {0x6F12, 0x000C}, {0x6F12, 0x6047}, {0x6F12, 0x41F2},
    {0x6F12, 0x950C}, {0x6F12, 0xC0F2}, {0x6F12, 0x000C}, {0x6F12, 0x6047},
    {0x6F12, 0x49F2}, {0x6F12, 0x514C}, {0x6F12, 0xC0F2}, {0x6F12, 0x000C},
    {0x6F12, 0x6047}, {0x6F12, 0x0000}, {0x6F12, 0x0000}, {0x6F12, 0x0000},
    {0x6F12, 0x0000}, {0x6F12, 0x0000}, {0x6F12, 0x4088}, {0x6F12, 0x0166},
    {0x6F12, 0x0000}, {0x6F12, 0x0002}, {0x5360, 0x0004}, {0x3078, 0x0059},
    {0x307C, 0x0025}, {0x36D0, 0x00DD}, {0x36D2, 0x0100}, {0x306A, 0x00EF},

};

static const SENSOR_REG_T s5k4h8yx_3264x2448_2lane_setting_new[] = {
    /*
    3264x2448_15fps_vt140M_2lane_mipi642M
    width   3264
    height  2448
    frame rate  14.97
    mipi_lane_num   2
    mipi_per_lane_bps   642
    line time(0.1us unit)   267.4285714
    frame length    2498
    Extclk  24M
    max gain    0x0200
    base gain   0x0020
    raw bits    raw10
    bayer patter    Gr first
    OB level    64
    offset  8
    min shutter 4
    address data    */
    {0x6028, 0x4000}, {0x602A, 0x6214}, {0x6F12, 0x7971}, {0x602A, 0x6218},
    {0x6F12, 0x7150}, {0x6028, 0x2000}, {0x602A, 0x0EC6}, {0x6F12, 0x0000},
    {0xF490, 0x0030}, {0xF47A, 0x0012}, {0xF428, 0x0200}, {0xF48E, 0x0010},
    {0xF45C, 0x0004}, {0x0B04, 0x0101}, {0x0B00, 0x0080}, {0x6028, 0x2000},
    {0x602A, 0x0C40}, {0x6F12, 0x0140}, {0x31FA, 0x0000}, {0x0200, 0x0800},
    {0x0202, 0x0465}, {0x31AA, 0x0004}, {0x1006, 0x0006}, {0x31FA, 0x0000},
    {0x0204, 0x0020}, {0x0344, 0x0008}, {0x0348, 0x0CC7}, {0x0346, 0x0008},
    {0x034A, 0x0997}, {0x034C, 0x0CC0}, {0x034E, 0x0990}, {0x0342, 0x0EA0},
    {0x0340, 0x09C2}, {0x0900, 0x0111}, {0x0380, 0x0001}, {0x0382, 0x0001},
    {0x0384, 0x0001}, {0x0386, 0x0001}, {0x0400, 0x0002}, {0x0404, 0x0010},
    {0x0114, 0x0130}, {0x0136, 0x1800}, {0x0300, 0x0005}, {0x0302, 0x0002},
    {0x0304, 0x0004}, {0x0306, 0x0075}, {0x030C, 0x0004}, {0x030E, 0x006B},
    {0x3008, 0x0000},

};
static const SENSOR_REG_T s5k4h8yx_3264x2448_4lane_setting_new[] = {
    /*3264x2448_30fps_vt280M_4lane_mipi700M
    width   3264
    height  2448
    frame rate  29.94
    mipi_lane_num   4
    mipi_per_lane_bps   700
    line time(0.1us unit)   133.71
    frame length    2498
    Extclk  24M
     max gain   0x0200
    base gain   0x0020
    raw bits    raw10
    bayer patter    Gr first
    OB level    64
    offset  8
    min shutter 4
    address data  */
    {0x6028, 0x4000}, {0x602A, 0x6214}, {0x6F12, 0x7971}, {0x602A, 0x6218},
    {0x6F12, 0x7150}, {0x6028, 0x2000}, {0x602A, 0x0EC6}, {0x6F12, 0x0000},
    {0xF490, 0x0030}, {0xF47A, 0x0012}, {0xF428, 0x0200}, {0xF48E, 0x0010},
    {0xF45C, 0x0004}, {0x0B04, 0x0101}, {0x0B00, 0x0080}, {0x6028, 0x2000},
    {0x602A, 0x0C40}, {0x6F12, 0x0140}, {0x0200, 0x0618}, {0x0202, 0x0904},
    {0x31AA, 0x0004}, {0x1006, 0x0006}, {0x31FA, 0x0000}, {0x0204, 0x0020},
    {0x0344, 0x0008}, {0x0348, 0x0CC7}, {0x0346, 0x0008}, {0x034A, 0x0997},
    {0x034C, 0x0CC0}, {0x034E, 0x0990}, {0x0342, 0x0EA0}, {0x0340, 0x09C2},
    {0x0900, 0x0111}, {0x0380, 0x0001}, {0x0382, 0x0001}, {0x0384, 0x0001},
    {0x0386, 0x0001}, {0x0400, 0x0002}, {0x0404, 0x0010}, {0x0114, 0x0330},
    {0x0136, 0x1800}, {0x0300, 0x0005}, {0x0302, 0x0001}, {0x0304, 0x0006},
    {0x0306, 0x00AF}, {0x030C, 0x0006}, {0x030E, 0x00AF}, {0x3008, 0x0000},

};
static const SENSOR_REG_T s5k4h8yx_common_init_new1[] = {
    /*for V22	FGX9
address	data*/
    {0x6028, 0x4000}, {0x602A, 0x6214}, {0x6F12, 0x7971},
    {0x602A, 0x6218}, {0x6F12, 0x7150}, {0x0806, 0x0001},
};
static const SENSOR_REG_T s5k4h8yx_3264x2448_2lane_setting_new1[] = {
    /*full size
3264x2448_15fps_vt140M_2lane_mipi642M
width	3264
height	2448
frame rate	29.94
mipi_lane_num	2
mipi_per_lane_bps	642
line time(0.1us unit)	267.4285714
frame length	2498
Extclk	24M
 max gain	0x0200
base gain	0x0020
raw bits	raw10
bayer patter 	Gr first
OB level	64
offset	8
min shutter	4
address	data*/
    {0x6028, 0x2000}, {0x602A, 0x0EC6}, {0x6F12, 0x0000}, {0xFCFC, 0x4000},
    {0xF490, 0x0030}, {0xF47A, 0x0012}, {0xF428, 0x0200}, {0xF48E, 0x0010},
    {0xF45C, 0x0004}, {0x0B04, 0x0101}, {0x0B00, 0x0080}, {0x6028, 0x2000},
    {0x602A, 0x0C40}, {0x6F12, 0x0140}, {0xFCFC, 0x4000}, {0x0200, 0x0618},
    {0x0202, 0x0465}, {0x31AA, 0x0004}, {0x1006, 0x0006}, {0x31FA, 0x0000},
    {0x0204, 0x0020}, {0x0344, 0x0008}, {0x0348, 0x0CC7}, {0x0346, 0x0008},
    {0x034A, 0x0997}, {0x034C, 0x0CC0}, {0x034E, 0x0990}, {0x0342, 0x0EA0},
    {0x0340, 0x09C2}, {0x0900, 0x0111}, {0x0380, 0x0001}, {0x0382, 0x0001},
    {0x0384, 0x0001}, {0x0386, 0x0001}, {0x0400, 0x0002}, {0x0404, 0x0010},
    {0x0114, 0x0130}, {0x0136, 0x1800}, {0x0300, 0x0005}, {0x0302, 0x0002},
    {0x0304, 0x0004}, {0x0306, 0x0075}, {0x030C, 0x0004}, {0x030E, 0x006B},
    {0x3008, 0x0000},
};
static const SENSOR_REG_T s5k4h8yx_3264x2448_4lane_setting_new1[] = {
    /*full size
3264x2448_30fps_vt280M_4lane_mipi700M
width	3264
height	2448
frame rate	29.94
mipi_lane_num	4
mipi_per_lane_bps	700
line time(0.1us unit)	133.7142857
frame length	2498
Extclk	24M
 max gain	0x0200
base gain	0x0020
raw bits	raw10
bayer patter 	Gr first
OB level	64
offset	8
min shutter	4
address	data*/
    {0x6028, 0x2000}, {0x602A, 0x0EC6}, {0x6F12, 0x0000},
    {0xFCFC, 0x4000}, {0xF490, 0x0030}, {0xF47A, 0x0012},
    {0xF428, 0x0200}, {0xF48E, 0x0010}, {0xF45C, 0x0004},
    {0x0B04, 0x0101}, {0x0B00, 0x0080}, {0x6028, 0x2000},
    {0x602A, 0x0C40}, {0x6F12, 0x0140}, {0xFCFC, 0x4000},
    {0x0200, 0x0618}, {0x0202, 0x0904}, {0x31AA, 0x0004},
    {0x1006, 0x0006}, {0x31FA, 0x0000}, {0x0204, 0x0020},
    {0x0344, 0x0008}, {0x0348, 0x0CC7}, {0x0346, 0x0008},
    {0x034A, 0x0997}, {0x034C, 0x0CC0}, {0x034E, 0x0990},
    {0x0342, 0x0EA0}, {0x0340, 0x09C2}, {0x0900, 0x0111},
    {0x0380, 0x0001}, {0x0382, 0x0001}, {0x0384, 0x0001},
    {0x0386, 0x0001}, {0x0400, 0x0002}, {0x0404, 0x0010},
    {0x0114, 0x0330}, {0x0136, 0x1800}, {0x0300, 0x0005},
    {0x0302, 0x0001}, {0x0304, 0x0006}, {0x0306, 0x00AF},
    {0x030C, 0x0006}, {0x030E, 0x00BE}, // AF},
    {0x3008, 0x0000},
};

static const SENSOR_REG_T s5k4h8yx_1632x1224_4lane_setting_new1[] = {
    /*binning size
1632x1224_60fps_vt280M_4lane_mipi700M
width	3264
height	2448
frame rate	60.02
mipi_lane_num	4
mipi_per_lane_bps	700
line time(0.1us unit)	133.7142857
frame length	1246
Extclk	24M
 max gain	0x0200
base gain	0x0020
raw bits	raw10
bayer patter 	Gr first
OB level	64
offset	8
min shutter	4
address	data*/
    {0x6028, 0x2000}, {0x602A, 0x0EC6}, {0x6F12, 0x0000}, {0xFCFC, 0x4000},
    {0xF490, 0x0030}, {0xF47A, 0x0012}, {0xF428, 0x0200}, {0xF48E, 0x0010},
    {0xF45C, 0x0004}, {0x0B04, 0x0101}, {0x0B00, 0x0080}, {0x6028, 0x2000},
    {0x602A, 0x0C40}, {0x6F12, 0x0140}, {0xFCFC, 0x4000}, {0x0200, 0x0618},
    {0x0202, 0x0904}, {0x31AA, 0x0004}, {0x1006, 0x0006}, {0x31FA, 0x0000},
    {0x0204, 0x0020}, {0x0344, 0x0008}, {0x0348, 0x0CC7}, {0x0346, 0x0008},
    {0x034A, 0x0997}, {0x034C, 0x0660}, {0x034E, 0x04C8}, {0x0342, 0x0EA0},
    {0x0340, 0x04DE}, {0x0900, 0x0212}, {0x0380, 0x0001}, {0x0382, 0x0001},
    {0x0384, 0x0001}, {0x0386, 0x0003}, {0x0400, 0x0002}, {0x0404, 0x0020},
    {0x0114, 0x0330}, {0x0136, 0x1800}, {0x0300, 0x0005}, {0x0302, 0x0001},
    {0x0304, 0x0006}, {0x0306, 0x00AF}, {0x030C, 0x0006}, {0x030E, 0x00AF},
    {0x3008, 0x0000},

};

////initial setting
static const SENSOR_REG_T s5k4h8yx_common_init[] = {
    {0x6028, 0x4000},
    {0x6010, 0x0001}, // Reset
    {0xffff, 0x000a}, // must add delay >3ms
    {0x602A, 0x6214},
    {0x6F12, 0x7971},
    {0x602A, 0x6218},
    {0x6F12, 0x7150},
    {0x6028, 0x2000},
    {0x602A, 0x0EC6},
    {0x6F12, 0x0000},
    {0xFCFC, 0x4000},
    {0xF490, 0x0030},
    {0xF47A, 0x0012},
    {0xF428, 0x0200},
    {0xF48E, 0x0010},
    {0xF45C, 0x0004},
    {0x0B04, 0x0101},
    {0x0B00, 0x0080}, // shading off;  if
                      // 0x0B00,0x0180
                      // //shading on
    {0x6028, 0x2000},
    {0x602A, 0x0C40},
    {0x6F12, 0x0140},
};

///////////res1//////////
/*full size setting, 3264x2448,30fps,vt280M,mipi700M,4lane
frame length    0x09C2
line time       133.7
*/
static const SENSOR_REG_T s5k4h8yx_3264x2448_4lane_setting[] = {
    {0x6028, 0x4000}, {0x0200, 0x0618}, {0x0202, 0x0904}, {0x31AA, 0x0004},
    {0x1006, 0x0006}, {0x31FA, 0x0000}, {0x0344, 0x0008}, {0x0348, 0x0CC7},
    {0x0346, 0x0008}, {0x034A, 0x0997}, {0x034C, 0x0CC0}, {0x034E, 0x0990},
    {0x0342, 0x0EA0}, {0x0340, 0x09C2}, {0x0900, 0x0111}, {0x0380, 0x0001},
    {0x0382, 0x0001}, {0x0384, 0x0001}, {0x0386, 0x0001}, {0x0400, 0x0002},
    {0x0404, 0x0010}, {0x0114, 0x0330}, {0x0136, 0x1800}, {0x0300, 0x0005},
    {0x0302, 0x0001}, {0x0304, 0x0006}, {0x0306, 0x00AF}, {0x030C, 0x0006},
    {0x030E, 0x00AF}, {0x3008, 0x0000},
};

///////////res2///////////////////
/*full size setting, 3264x2448,15fps,vt140M,mipi642M,2lane
frame length    0x09C2
line time       267.4
*/
static const SENSOR_REG_T s5k4h8yx_3264x2448_2lane_setting[] = {
    {0x6028, 0x4000}, {0x0200, 0x0618}, {0x0202, 0x0465}, {0x31AA, 0x0004},
    {0x1006, 0x0006}, {0x31FA, 0x0000}, {0x0344, 0x0008}, {0x0348, 0x0CC7},
    {0x0346, 0x0008}, {0x034A, 0x0997}, {0x034C, 0x0CC0}, {0x034E, 0x0990},
    {0x0342, 0x0EA0}, {0x0340, 0x09C2}, {0x0900, 0x0111}, {0x0380, 0x0001},
    {0x0382, 0x0001}, {0x0384, 0x0001}, {0x0386, 0x0001}, {0x0400, 0x0002},
    {0x0404, 0x0010}, {0x0114, 0x0130}, {0x0136, 0x1800}, {0x0300, 0x0005},
    {0x0302, 0x0002}, {0x0304, 0x0004}, {0x0306, 0x0075}, {0x030C, 0x0004},
    {0x030E, 0x006B}, {0x3008, 0x0000},
};

///////////res3//////////
/*small size setting, 1632x1224,30fps,vt280M,mipi700M,4lane
frame length    0x04DE
line time       133.7
*/
static const SENSOR_REG_T s5k4h8yx_1632x1224_4lane_setting[] = {
    {0xFCFC, 0x4000}, {0x0200, 0x0618}, {0x0202, 0x0904}, {0x31AA, 0x0004},
    {0x1006, 0x0006}, {0x31FA, 0x0000}, {0x0204, 0x0020}, {0x020E, 0x0100},
    {0x0344, 0x0008}, {0x0348, 0x0CC7}, {0x0346, 0x0008}, {0x034A, 0x0997},
    {0x034C, 0x0660}, {0x034E, 0x04C8}, {0x0342, 0x0EA0}, {0x0340, 0x04DE},
    {0x0900, 0x0212}, {0x0380, 0x0001}, {0x0382, 0x0001}, {0x0384, 0x0001},
    {0x0386, 0x0003}, {0x0400, 0x0002}, {0x0404, 0x0020}, {0x0114, 0x0330},
    {0x0136, 0x1800}, {0x0300, 0x0005}, {0x0302, 0x0001}, {0x0304, 0x0006},
    {0x0306, 0x00AF}, {0x030C, 0x0006}, {0x030E, 0x00AF}, {0x3008, 0x0000},
};

static struct sensor_res_tab_info s_s5k4h8yx_resolution_tab_raw[VENDOR_NUM] = {
    {
      .module_id = MODULE_SUNNY,
      .reg_tab = {
        {ADDR_AND_LEN_OF_ARRAY(s5k4h8yx_common_init_new1), PNULL, 0,
        .width = 0, .height = 0,
        .xclk_to_sensor = 24, .image_format = SENSOR_IMAGE_FORMAT_RAW},

        {ADDR_AND_LEN_OF_ARRAY(s5k4h8yx_1632x1224_4lane_setting), PNULL, 0,
        .width = 1632, .height = 1224,
        .xclk_to_sensor = 24, .image_format = SENSOR_IMAGE_FORMAT_RAW},

        {ADDR_AND_LEN_OF_ARRAY(s5k4h8yx_3264x2448_4lane_setting_new1), PNULL, 0,
        .width = 3264, .height = 2448,
        .xclk_to_sensor = 24, .image_format = SENSOR_IMAGE_FORMAT_RAW}}
    }
/*If there are multiple modules,please add here*/
};


static SENSOR_TRIM_T s_s5k4h8yx_resolution_trim_tab[VENDOR_NUM] = {
    {
     .module_id = MODULE_SUNNY,
     .trim_info = {
       {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
#ifdef S5K4H8YX_2_LANES
      /* {.trim_start_x = 0,.trim_start_y = 0,.trim_width = 1632,.trim_height = 1224,
        .line_time = 26742,.bps_per_lane = 700, .frame_line = 1246,
        .scaler_trim = {.x = 0, .y = 0, .w = 1632, .h = 1224}}, */
       {.trim_start_x = 0,.trim_start_y = 0,.trim_width = 3264,.trim_height = 2448,
        .line_time = 26742,.bps_per_lane = 700,.frame_line = 2498,
        .scaler_trim = {.x = 0, .y = 0, .w = 3264, .h = 2448}},
#else
       {.trim_start_x = 0,.trim_start_y = 0,.trim_width = 1632,.trim_height = 1224,
        .line_time = 13371,.bps_per_lane = 700, .frame_line = 2580,
        .scaler_trim = {.x = 0, .y = 0, .w = 1632, .h = 1224}},

       {.trim_start_x = 0,.trim_start_y = 0,.trim_width = 3264,.trim_height = 2448,
        .line_time = 13371,.bps_per_lane = 700, .frame_line = 2580,
        .scaler_trim = {.x = 0, .y = 0, .w = 3264, .h = 2448}},
#endif
      }}
    /*If there are multiple modules,please add here*/
};


static const SENSOR_REG_T
    s_s5k4h8yx_1632x1224_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
        /*video mode 0: ?fps*/
        {{0xffff, 0xff}},
        /* video mode 1:?fps*/
        {{0xffff, 0xff}},
        /* video mode 2:?fps*/
        {{0xffff, 0xff}},
        /* video mode 3:?fps*/
        {{0xffff, 0xff}}};

static const SENSOR_REG_T
    s_s5k4h8yx_1920x1080_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
        /*video mode 0: ?fps*/
        {{0xffff, 0xff}},
        /* video mode 1:?fps*/
        {{0xffff, 0xff}},
        /* video mode 2:?fps*/
        {{0xffff, 0xff}},
        /* video mode 3:?fps*/
        {{0xffff, 0xff}}};

static const SENSOR_REG_T
    s_s5k4h8yx_3264x2448_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
        /*video mode 0: ?fps*/
        {{0xffff, 0xff}},
        /* video mode 1:?fps*/
        {{0xffff, 0xff}},
        /* video mode 2:?fps*/
        {{0xffff, 0xff}},
        /* video mode 3:?fps*/
        {{0xffff, 0xff}}};

static SENSOR_VIDEO_INFO_T s_s5k4h8yx_video_info[] = {
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    //{{{30, 30, 219, 100}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0,
    // 0}},(SENSOR_REG_T**)s_s5k4h8yx_1632x1224_video_tab},
    {{{15, 15, 131, 64}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
     (SENSOR_REG_T **)s_s5k4h8yx_3264x2448_video_tab},
    //{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL}};

static SENSOR_STATIC_INFO_T s_s5k4h8yx_static_info[VENDOR_NUM] = {
    {
     .module_id = MODULE_SUNNY,
     .static_info = {
         .f_num = 200,
         .focal_length = 287,
         .max_fps = 0,
         .max_adgain = 16 * 256,
         .ois_supported = 0,
         .pdaf_supported = 0,
         .exp_valid_frame_num = 1,
         .clamp_level = 64,
         .adgain_valid_frame_num = 1,
         .fov_info = {{3.656f, 2.742f}, 3.01f}}
    }
    /*If there are multiple modules,please add here*/
};

static SENSOR_MODE_FPS_INFO_T s_s5k4h8yx_mode_fps_info[VENDOR_NUM] = {
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

static struct sensor_module_info s_s5k4h8yx_module_info_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .module_info = {
         .major_i2c_addr = 0x10 ,
         .minor_i2c_addr = 0x10 ,

         .reg_addr_value_bits = SENSOR_I2C_REG_16BIT | SENSOR_I2C_VAL_16BIT |
                                SENSOR_I2C_FREQ_400,

         .avdd_val = SENSOR_AVDD_2800MV,
         .iovdd_val = SENSOR_AVDD_1800MV,
         .dvdd_val = SENSOR_AVDD_1200MV,

         .image_pattern = SENSOR_IMAGE_PATTERN_RAWRGB_GB,

         .preview_skip_num = 3,
         .capture_skip_num = 3,
         .flash_capture_skip_num = 6,
         .mipi_cap_skip_num = 0,
         .preview_deci_num = 0,
         .video_preview_deci_num = 0,

#if defined(S5K4H8YX_2_LANES)
         .sensor_interface = 
         {.type = SENSOR_INTERFACE_TYPE_CSI2, .bus_width = 2,
          .pixel_width = 10, .is_loose = 0},
#elif defined(S5K4H8YX_4_LANES)
         .sensor_interface = 
         {.type = SENSOR_INTERFACE_TYPE_CSI2, .bus_width = 4,
          .pixel_width = 10, .is_loose = 0},
#endif
         .change_setting_skip_num = 1,
         .horizontal_view_angle = 48,
         .vertical_view_angle = 48
      }
    }

/*If there are multiple modules,please add here*/
};

/*==============================================================================
 * Description:
 * sensor all info
 * please modify this variable acording your spec
 *============================================================================*/
static struct sensor_raw_info *s_s5k4h8yx_mipi_raw_info_ptr = &s_s5k4h8yx_mipi_raw_info;
static struct sensor_ic_ops s5k4h8yx_ops_tab;

SENSOR_INFO_T g_s5k4h8yx_mipi_raw_info = {
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
    .identify_code = {{0x0, 0x2},
                      {0x1, 0x19}},

    .source_width_max = 3264,
    .source_height_max = 2448,
    .name = (cmr_s8 *)SENSOR_NAME,
    .image_format = SENSOR_IMAGE_FORMAT_RAW,

    .resolution_tab_info_ptr = s_s5k4h8yx_resolution_tab_raw,
    .sns_ops = &s5k4h8yx_ops_tab,
    .raw_info_ptr = &s_s5k4h8yx_mipi_raw_info_ptr,
    .ext_info_ptr = NULL,
    .module_info_tab = s_s5k4h8yx_module_info_tab,
    .module_info_tab_size = ARRAY_SIZE(s_s5k4h8yx_module_info_tab),
    .video_tab_info_ptr = s_s5k4h8yx_video_info,
    .sensor_version_info = (cmr_s8 *) "s5k4h8yx_truly_v1",
};
#endif
