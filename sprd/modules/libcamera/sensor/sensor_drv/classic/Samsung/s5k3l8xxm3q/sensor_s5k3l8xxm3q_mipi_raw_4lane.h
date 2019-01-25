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

#include "parameters/sensor_s5k3l8xxm3q_raw_param_main.c"

#define FEATURE_OTP

#define VENDOR_NUM 1
#define SENSOR_NAME "s5k3l8xxm3q_mipi_raw"
#define I2C_SLAVE_ADDR 0x5A // 0x20 //

#define s5k3l8xxm3q_PID_ADDR 0x0000
#define s5k3l8xxm3q_PID_VALUE 0x30c8
#define s5k3l8xxm3q_VER_ADDR 0x0004
#define s5k3l8xxm3q_VER_VALUE 0x10ff

/* sensor parameters begin */
/* effective sensor output image size */
#define VIDEO_WIDTH 1280
#define VIDEO_HEIGHT 720
#define PREVIEW_WIDTH 2096
#define PREVIEW_HEIGHT 1560
#define SNAPSHOT_WIDTH 4208
#define SNAPSHOT_HEIGHT 3120

/*Raw Trim parameters*/
#define VIDEO_TRIM_X 0
#define VIDEO_TRIM_Y 0
#define VIDEO_TRIM_W 1280
#define VIDEO_TRIM_H 720
#define PREVIEW_TRIM_X 0
#define PREVIEW_TRIM_Y 0
#define PREVIEW_TRIM_W 2096
#define PREVIEW_TRIM_H 1560
#define SNAPSHOT_TRIM_X 0
#define SNAPSHOT_TRIM_Y 0
#define SNAPSHOT_TRIM_W 4208
#define SNAPSHOT_TRIM_H 3120

/*Mipi output*/
#define LANE_NUM 4
#define RAW_BITS 10

#define VIDEO_MIPI_PER_LANE_BPS 1124    /* 2*Mipi clk */
#define PREVIEW_MIPI_PER_LANE_BPS 1124  /* 2*Mipi clk */
#define SNAPSHOT_MIPI_PER_LANE_BPS 1124 /* 2*Mipi clk */

/*line time unit: 0.1ns*/
#define VIDEO_LINE_TIME 10263
#define PREVIEW_LINE_TIME 10256
#define SNAPSHOT_LINE_TIME 10256

/* frame length*/
#define VIDEO_FRAME_LENGTH 812
#define PREVIEW_FRAME_LENGTH 3206
#define SNAPSHOT_FRAME_LENGTH 3234

/* please ref your spec */
#define FRAME_OFFSET 8
#define SENSOR_MAX_GAIN 0x200
#define SENSOR_BASE_GAIN 0x20
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

/*==============================================================================
 * Description:
 * register setting
 *============================================================================*/

static const SENSOR_REG_T s5k3l8xxm3q_init_setting[] = {
    {0x6028, 0x4000}, {0x6214, 0xFFFF}, {0x6216, 0xFFFF}, {0x6218, 0x0000},
    {0x621A, 0x0000},

    {0x6028, 0x2000}, {0x602A, 0x2450}, {0x6F12, 0x0448}, {0x6F12, 0x0349},
    {0x6F12, 0x0160}, {0x6F12, 0xC26A}, {0x6F12, 0x511A}, {0x6F12, 0x8180},
    {0x6F12, 0x00F0}, {0x6F12, 0x48B8}, {0x6F12, 0x2000}, {0x6F12, 0x2588},
    {0x6F12, 0x2000}, {0x6F12, 0x16C0}, {0x6F12, 0x0000}, {0x6F12, 0x0000},
    {0x6F12, 0x0000}, {0x6F12, 0x0000}, {0x6F12, 0x10B5}, {0x6F12, 0x00F0},
    {0x6F12, 0x5DF8}, {0x6F12, 0x2748}, {0x6F12, 0x4078}, {0x6F12, 0x0028},
    {0x6F12, 0x0AD0}, {0x6F12, 0x00F0}, {0x6F12, 0x5CF8}, {0x6F12, 0x2549},
    {0x6F12, 0xB1F8}, {0x6F12, 0x1403}, {0x6F12, 0x4200}, {0x6F12, 0x2448},
    {0x6F12, 0x4282}, {0x6F12, 0x91F8}, {0x6F12, 0x9610}, {0x6F12, 0x4187},
    {0x6F12, 0x10BD}, {0x6F12, 0x70B5}, {0x6F12, 0x0446}, {0x6F12, 0x2148},
    {0x6F12, 0x0022}, {0x6F12, 0x4068}, {0x6F12, 0x86B2}, {0x6F12, 0x050C},
    {0x6F12, 0x3146}, {0x6F12, 0x2846}, {0x6F12, 0x00F0}, {0x6F12, 0x4CF8},
    {0x6F12, 0x2046}, {0x6F12, 0x00F0}, {0x6F12, 0x4EF8}, {0x6F12, 0x14F8},
    {0x6F12, 0x680F}, {0x6F12, 0x6178}, {0x6F12, 0x40EA}, {0x6F12, 0x4100},
    {0x6F12, 0x1749}, {0x6F12, 0xC886}, {0x6F12, 0x1848}, {0x6F12, 0x2278},
    {0x6F12, 0x007C}, {0x6F12, 0x4240}, {0x6F12, 0x1348}, {0x6F12, 0xA230},
    {0x6F12, 0x8378}, {0x6F12, 0x43EA}, {0x6F12, 0xC202}, {0x6F12, 0x0378},
    {0x6F12, 0x4078}, {0x6F12, 0x9B00}, {0x6F12, 0x43EA}, {0x6F12, 0x4000},
    {0x6F12, 0x0243}, {0x6F12, 0xD0B2}, {0x6F12, 0x0882}, {0x6F12, 0x3146},
    {0x6F12, 0x2846}, {0x6F12, 0xBDE8}, {0x6F12, 0x7040}, {0x6F12, 0x0122},
    {0x6F12, 0x00F0}, {0x6F12, 0x2AB8}, {0x6F12, 0x10B5}, {0x6F12, 0x0022},
    {0x6F12, 0xAFF2}, {0x6F12, 0x8701}, {0x6F12, 0x0B48}, {0x6F12, 0x00F0},
    {0x6F12, 0x2DF8}, {0x6F12, 0x084C}, {0x6F12, 0x0022}, {0x6F12, 0xAFF2},
    {0x6F12, 0x6D01}, {0x6F12, 0x2060}, {0x6F12, 0x0848}, {0x6F12, 0x00F0},
    {0x6F12, 0x25F8}, {0x6F12, 0x6060}, {0x6F12, 0x10BD}, {0x6F12, 0x0000},
    {0x6F12, 0x2000}, {0x6F12, 0x0550}, {0x6F12, 0x2000}, {0x6F12, 0x0C60},
    {0x6F12, 0x4000}, {0x6F12, 0xD000}, {0x6F12, 0x2000}, {0x6F12, 0x2580},
    {0x6F12, 0x2000}, {0x6F12, 0x16F0}, {0x6F12, 0x0000}, {0x6F12, 0x2221},
    {0x6F12, 0x0000}, {0x6F12, 0x2249}, {0x6F12, 0x42F2}, {0x6F12, 0x351C},
    {0x6F12, 0xC0F2}, {0x6F12, 0x000C}, {0x6F12, 0x6047}, {0x6F12, 0x42F2},
    {0x6F12, 0xE11C}, {0x6F12, 0xC0F2}, {0x6F12, 0x000C}, {0x6F12, 0x6047},
    {0x6F12, 0x40F2}, {0x6F12, 0x077C}, {0x6F12, 0xC0F2}, {0x6F12, 0x000C},
    {0x6F12, 0x6047}, {0x6F12, 0x42F2}, {0x6F12, 0x492C}, {0x6F12, 0xC0F2},
    {0x6F12, 0x000C}, {0x6F12, 0x6047}, {0x6F12, 0x4BF2}, {0x6F12, 0x453C},
    {0x6F12, 0xC0F2}, {0x6F12, 0x000C}, {0x6F12, 0x6047}, {0x6F12, 0x0000},
    {0x6F12, 0x0000}, {0x6F12, 0x0000}, {0x6F12, 0x0000}, {0x6F12, 0x0000},
    {0x6F12, 0x0000}, {0x6F12, 0x0000}, {0x6F12, 0x30C8}, {0x6F12, 0x0157},
    {0x6F12, 0x0000}, {0x6F12, 0x0003},

    {0x6028, 0x2000}, {0x602A, 0x1082}, {0x6F12, 0x8010}, {0x6028, 0x4000},
    {0x31CE, 0x0001}, {0x0200, 0x00C6}, {0x3734, 0x0010}, {0x3736, 0x0001},
    {0x3738, 0x0001}, {0x37CC, 0x0001}, {0x3744, 0x0100}, {0x3762, 0x0105},
    {0x3764, 0x0105}, {0x376A, 0x00F0}, {0x344A, 0x000F}, {0x344C, 0x003D},
    {0xF460, 0x0030}, {0xF414, 0x24C2}, {0xF416, 0x0183}, {0xF468, 0x4005},
    {0x3424, 0x0A07}, {0x3426, 0x0F07}, {0x3428, 0x0F07}, {0x341E, 0x0804},
    {0x3420, 0x0C0C}, {0x3422, 0x2D2D}, {0xF462, 0x003A}, {0x3450, 0x0010},
    {0x3452, 0x0010}, {0xF446, 0x0020}, {0xF44E, 0x000C}, {0x31FA, 0x0007},
    {0x31FC, 0x0161}, {0x31FE, 0x0009}, {0x3200, 0x000C}, {0x3202, 0x007F},
    {0x3204, 0x00A2}, {0x3206, 0x007D}, {0x3208, 0x00A4}, {0x3334, 0x00A7},
    {0x3336, 0x00A5}, {0x3338, 0x0033}, {0x333A, 0x0006}, {0x333C, 0x009F},
    {0x333E, 0x008C}, {0x3340, 0x002D}, {0x3342, 0x000A}, {0x3344, 0x002F},
    {0x3346, 0x0008}, {0x3348, 0x009F}, {0x334A, 0x008C}, {0x334C, 0x002D},
    {0x334E, 0x000A}, {0x3350, 0x000A}, {0x320A, 0x007B}, {0x320C, 0x0161},
    {0x320E, 0x007F}, {0x3210, 0x015F}, {0x3212, 0x007B}, {0x3214, 0x00B0},
    {0x3216, 0x0009}, {0x3218, 0x0038}, {0x321A, 0x0009}, {0x321C, 0x0031},
    {0x321E, 0x0009}, {0x3220, 0x0038}, {0x3222, 0x0009}, {0x3224, 0x007B},
    {0x3226, 0x0001}, {0x3228, 0x0010}, {0x322A, 0x00A2}, {0x322C, 0x00B1},
    {0x322E, 0x0002}, {0x3230, 0x015D}, {0x3232, 0x0001}, {0x3234, 0x015D},
    {0x3236, 0x0001}, {0x3238, 0x000B}, {0x323A, 0x0016}, {0x323C, 0x000D},
    {0x323E, 0x001C}, {0x3240, 0x000D}, {0x3242, 0x0054}, {0x3244, 0x007B},
    {0x3246, 0x00CC}, {0x3248, 0x015D}, {0x324A, 0x007E}, {0x324C, 0x0095},
    {0x324E, 0x0085}, {0x3250, 0x009D}, {0x3252, 0x008D}, {0x3254, 0x009D},
    {0x3256, 0x007E}, {0x3258, 0x0080}, {0x325A, 0x0001}, {0x325C, 0x0005},
    {0x325E, 0x0085}, {0x3260, 0x009D}, {0x3262, 0x0001}, {0x3264, 0x0005},
    {0x3266, 0x007E}, {0x3268, 0x0080}, {0x326A, 0x0053}, {0x326C, 0x007D},
    {0x326E, 0x00CB}, {0x3270, 0x015E}, {0x3272, 0x0001}, {0x3274, 0x0005},
    {0x3276, 0x0009}, {0x3278, 0x000C}, {0x327A, 0x007E}, {0x327C, 0x0098},
    {0x327E, 0x0009}, {0x3280, 0x000C}, {0x3282, 0x007E}, {0x3284, 0x0080},
    {0x3286, 0x0044}, {0x3288, 0x0163}, {0x328A, 0x0045}, {0x328C, 0x0047},
    {0x328E, 0x007D}, {0x3290, 0x0080}, {0x3292, 0x015F}, {0x3294, 0x0162},
    {0x3296, 0x007D}, {0x3298, 0x0000}, {0x329A, 0x0000}, {0x329C, 0x0000},
    {0x329E, 0x0000}, {0x32A0, 0x0008}, {0x32A2, 0x0010}, {0x32A4, 0x0018},
    {0x32A6, 0x0020}, {0x32A8, 0x0000}, {0x32AA, 0x0008}, {0x32AC, 0x0010},
    {0x32AE, 0x0018}, {0x32B0, 0x0020}, {0x32B2, 0x0020}, {0x32B4, 0x0020},
    {0x32B6, 0x0020}, {0x32B8, 0x0000}, {0x32BA, 0x0000}, {0x32BC, 0x0000},
    {0x32BE, 0x0000}, {0x32C0, 0x0000}, {0x32C2, 0x0000}, {0x32C4, 0x0000},
    {0x32C6, 0x0000}, {0x32C8, 0x0000}, {0x32CA, 0x0000}, {0x32CC, 0x0000},
    {0x32CE, 0x0000}, {0x32D0, 0x0000}, {0x32D2, 0x0000}, {0x32D4, 0x0000},
    {0x32D6, 0x0000}, {0x32D8, 0x0000}, {0x32DA, 0x0000}, {0x32DC, 0x0000},
    {0x32DE, 0x0000}, {0x32E0, 0x0000}, {0x32E2, 0x0000}, {0x32E4, 0x0000},
    {0x32E6, 0x0000}, {0x32E8, 0x0000}, {0x32EA, 0x0000}, {0x32EC, 0x0000},
    {0x32EE, 0x0000}, {0x32F0, 0x0000}, {0x32F2, 0x0000}, {0x32F4, 0x000A},
    {0x32F6, 0x0002}, {0x32F8, 0x0008}, {0x32FA, 0x0010}, {0x32FC, 0x0020},
    {0x32FE, 0x0028}, {0x3300, 0x0038}, {0x3302, 0x0040}, {0x3304, 0x0050},
    {0x3306, 0x0058}, {0x3308, 0x0068}, {0x330A, 0x0070}, {0x330C, 0x0080},
    {0x330E, 0x0088}, {0x3310, 0x0098}, {0x3312, 0x00A0}, {0x3314, 0x00B0},
    {0x3316, 0x00B8}, {0x3318, 0x00C8}, {0x331A, 0x00D0}, {0x331C, 0x00E0},
    {0x331E, 0x00E8}, {0x3320, 0x0017}, {0x3322, 0x002F}, {0x3324, 0x0047},
    {0x3326, 0x005F}, {0x3328, 0x0077}, {0x332A, 0x008F}, {0x332C, 0x00A7},
    {0x332E, 0x00BF}, {0x3330, 0x00D7}, {0x3332, 0x00EF}, {0x3352, 0x00A5},
    {0x3354, 0x00AF}, {0x3356, 0x0187}, {0x3358, 0x0000}, {0x335A, 0x009E},
    {0x335C, 0x016B}, {0x335E, 0x0015}, {0x3360, 0x00A5}, {0x3362, 0x00AF},
    {0x3364, 0x01FB}, {0x3366, 0x0000}, {0x3368, 0x009E}, {0x336A, 0x016B},
    {0x336C, 0x0015}, {0x336E, 0x00A5}, {0x3370, 0x00A6}, {0x3372, 0x0187},
    {0x3374, 0x0000}, {0x3376, 0x009E}, {0x3378, 0x016B}, {0x337A, 0x0015},
    {0x337C, 0x00A5}, {0x337E, 0x00A6}, {0x3380, 0x01FB}, {0x3382, 0x0000},
    {0x3384, 0x009E}, {0x3386, 0x016B}, {0x3388, 0x0015}, {0x319A, 0x0005},
    {0x1006, 0x0005}, {0x3416, 0x0001}, {0x308C, 0x0008}, {0x307C, 0x0240},
    {0x375E, 0x0050}, {0x31CE, 0x0101}, {0x374E, 0x0007}, {0x3460, 0x0001},
    {0x3052, 0x0002}, {0x3058, 0x0100}, {0x6028, 0x2000}, {0x602A, 0x108A},
    {0x6F12, 0x0359}, {0x6F12, 0x0100}, {0x6028, 0x4000}, {0x1124, 0x4100},
    {0x1126, 0x0000}, {0x112C, 0x4100}, {0x112E, 0x0000}, {0x3442, 0x0100},
    {0x3098, 0x0364}, //

};

static const SENSOR_REG_T s5k3l8xxm3q_preview_setting[] = {
    {0x6028, 0x2000}, {0x602A, 0x0F74}, {0x6F12, 0x0040}, {0x6F12, 0x0040},
    {0x6028, 0x4000}, {0x0344, 0x0008}, {0x0346, 0x0008}, {0x0348, 0x1077},
    {0x034A, 0x0C37}, {0x034C, 0x0830}, {0x034E, 0x0618}, {0x0900, 0x0112},
    {0x0380, 0x0001}, {0x0382, 0x0001}, {0x0384, 0x0001}, {0x0386, 0x0003},
    {0x0400, 0x0001}, {0x0404, 0x0020}, {0x0114, 0x0300}, {0x0110, 0x0002},
    {0x0136, 0x1800}, {0x0304, 0x0006}, {0x0306, 0x00B1}, {0x0302, 0x0001},
    {0x0300, 0x0005}, {0x030C, 0x0006}, {0x030E, 0x0119}, {0x030A, 0x0001},
    {0x0308, 0x0008}, {0x0342, 0x16B0}, {0x0340, 0x0C86}, {0x0202, 0x0200},
    {0x0200, 0x00C6}, {0x0B04, 0x0101}, {0x0B08, 0x0000}, {0x0B00, 0x0007},
    {0x316A, 0x00A0},

    //{ 0x0100, 0x0100 },	//streaming on
};

static const SENSOR_REG_T s5k3l8xxm3q_snapshot_setting[] = {
    {0x6028, 0x2000}, {0x602A, 0x0F74}, {0x6F12, 0x0040}, {0x6F12, 0x0040},
    {0x6028, 0x4000}, {0x0344, 0x0008}, {0x0346, 0x0008}, {0x0348, 0x1077},
    {0x034A, 0x0C37}, {0x034C, 0x1070}, {0x034E, 0x0C30}, {0x0900, 0x0011},
    {0x0380, 0x0001}, {0x0382, 0x0001}, {0x0384, 0x0001}, {0x0386, 0x0001},
    {0x0400, 0x0000}, {0x0404, 0x0010}, {0x0114, 0x0300}, {0x0110, 0x0002},
    {0x0136, 0x1800}, {0x0304, 0x0006}, {0x0306, 0x00B1}, {0x0302, 0x0001},
    {0x0300, 0x0005}, {0x030C, 0x0006}, {0x030E, 0x0119}, {0x030A, 0x0001},
    {0x0308, 0x0008}, {0x0342, 0x16B0}, {0x0340, 0x0CA2}, {0x0202, 0x0200},
    {0x0200, 0x00C6}, {0x0B04, 0x0101}, {0x0B08, 0x0000}, {0x0B00, 0x0007},
    {0x316A, 0x00A0},

    //{ 0x0100, 0x0100 },	//streaming on
};

static const SENSOR_REG_T s5k3l8xxm3q_video_setting[] = {
    // 720p setting @120fps, 24Mclk,p566.4M,1124M/lane,4lanes
    {0x6028, 0x2000}, {0x602A, 0x0F74}, {0x6F12, 0x0040}, {0x6F12, 0x0040},
    {0x6028, 0x4000}, {0x0344, 0x00C0}, {0x0346, 0x01E8}, {0x0348, 0x0FBF},
    {0x034A, 0x0A57}, {0x034C, 0x0500}, {0x034E, 0x02D0}, {0x0900, 0x0113},
    {0x0380, 0x0001}, {0x0382, 0x0001}, {0x0384, 0x0001}, {0x0386, 0x0005},
    {0x0400, 0x0001}, {0x0404, 0x0030}, {0x0114, 0x0300}, {0x0110, 0x0002},
    {0x0136, 0x1800}, {0x0304, 0x0006}, {0x0306, 0x00B1}, {0x0302, 0x0001},
    {0x0300, 0x0005}, {0x030C, 0x0006}, {0x030E, 0x0119}, {0x030A, 0x0001},
    {0x0308, 0x0008}, {0x0342, 0x16B0}, {0x0340, 0x032C}, {0x0202, 0x0200},
    {0x0200, 0x00C6}, {0x0B04, 0x0101}, {0x0B08, 0x0000}, {0x0B00, 0x0007},
    {0x316A, 0x00A0},

};

static struct sensor_res_tab_info s_s5k3l8xxm3q_resolution_tab_raw[VENDOR_NUM] =
    {
        {.module_id = MODULE_QTECH,
         .reg_tab = {{ADDR_AND_LEN_OF_ARRAY(s5k3l8xxm3q_init_setting), PNULL, 0,
                      .width = 0, .height = 0, .xclk_to_sensor = EX_MCLK,
                      .image_format = SENSOR_IMAGE_FORMAT_RAW},

                     {ADDR_AND_LEN_OF_ARRAY(s5k3l8xxm3q_video_setting), PNULL,
                      0, .width = VIDEO_WIDTH, .height = VIDEO_HEIGHT,
                      .xclk_to_sensor = EX_MCLK,
                      .image_format = SENSOR_IMAGE_FORMAT_RAW},

                     {ADDR_AND_LEN_OF_ARRAY(s5k3l8xxm3q_preview_setting), PNULL,
                      0, .width = PREVIEW_WIDTH, .height = PREVIEW_HEIGHT,
                      .xclk_to_sensor = EX_MCLK,
                      .image_format = SENSOR_IMAGE_FORMAT_RAW},

                     {ADDR_AND_LEN_OF_ARRAY(s5k3l8xxm3q_snapshot_setting),
                      PNULL, 0, .width = SNAPSHOT_WIDTH,
                      .height = SNAPSHOT_HEIGHT, .xclk_to_sensor = EX_MCLK,
                      .image_format = SENSOR_IMAGE_FORMAT_RAW}}}

        /*If there are multiple modules,please add here*/
};

static SENSOR_TRIM_T s_s5k3l8xxm3q_resolution_trim_tab[VENDOR_NUM] = {
    {.module_id = MODULE_QTECH,
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

static SENSOR_REG_T s5k3l8xxm3q_shutter_reg[] = {
    {0x0202, SENSOR_MIN_SHUTTER},
};

static struct sensor_i2c_reg_tab s5k3l8xxm3q_shutter_tab = {
    .settings = s5k3l8xxm3q_shutter_reg,
    .size = ARRAY_SIZE(s5k3l8xxm3q_shutter_reg),
};

static SENSOR_REG_T s5k3l8xxm3q_again_reg[] = {
    {0x0204, SENSOR_BASE_GAIN},
};

static struct sensor_i2c_reg_tab s5k3l8xxm3q_again_tab = {
    .settings = s5k3l8xxm3q_again_reg,
    .size = ARRAY_SIZE(s5k3l8xxm3q_again_reg),
};

static SENSOR_REG_T s5k3l8xxm3q_dgain_reg[] = {

};

static struct sensor_i2c_reg_tab s5k3l8xxm3q_dgain_tab = {
    .settings = s5k3l8xxm3q_dgain_reg,
    .size = ARRAY_SIZE(s5k3l8xxm3q_dgain_reg),
};

static SENSOR_REG_T s5k3l8xxm3q_frame_length_reg[] = {
    {0x0340, PREVIEW_FRAME_LENGTH},
};

static struct sensor_i2c_reg_tab s5k3l8xxm3q_frame_length_tab = {
    .settings = s5k3l8xxm3q_frame_length_reg,
    .size = ARRAY_SIZE(s5k3l8xxm3q_frame_length_reg),
};

static struct sensor_aec_i2c_tag s5k3l8xxm3q_aec_info = {
    .slave_addr = (I2C_SLAVE_ADDR >> 1),
    .addr_bits_type = SENSOR_I2C_REG_16BIT,
    .data_bits_type = SENSOR_I2C_VAL_16BIT,
    .shutter = &s5k3l8xxm3q_shutter_tab,
    .again = &s5k3l8xxm3q_again_tab,
    .dgain = &s5k3l8xxm3q_dgain_tab,
    .frame_length = &s5k3l8xxm3q_frame_length_tab,
};

static SENSOR_STATIC_INFO_T s_s5k3l8xxm3q_static_info[VENDOR_NUM] = {
    {.module_id = MODULE_QTECH,
     .static_info = {.f_num = 200,
                     .focal_length = 354,
                     .max_fps = 0,
                     .max_adgain = 15 * 2,
                     .ois_supported = 0,
                     .pdaf_supported = 0,
                     .exp_valid_frame_num = 1,
                     .clamp_level = 64,
                     .adgain_valid_frame_num = 0,
                     .fov_info = {{4.614f, 3.444f}, 4.222f}}}
    /*If there are multiple modules,please add here*/
};

static SENSOR_MODE_FPS_INFO_T s_s5k3l8xxm3q_mode_fps_info[VENDOR_NUM] = {
    {.module_id = MODULE_QTECH,
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

static struct sensor_module_info s_s5k3l8xxm3q_module_info_tab[VENDOR_NUM] = {
    {.module_id = MODULE_QTECH,
     .module_info = {.major_i2c_addr = I2C_SLAVE_ADDR >> 1,
                     .minor_i2c_addr = I2C_SLAVE_ADDR >> 1,

                     .reg_addr_value_bits = SENSOR_I2C_REG_16BIT |
                                            SENSOR_I2C_VAL_16BIT |
                                            SENSOR_I2C_FREQ_400,

                     .avdd_val = SENSOR_AVDD_2800MV,
                     .iovdd_val = SENSOR_AVDD_1800MV,
                     .dvdd_val = SENSOR_AVDD_1200MV,

                     .image_pattern = SENSOR_IMAGE_PATTERN_RAWRGB_GB,

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

static struct sensor_ic_ops s_s5k3l8xxm3q_ops_tab;
struct sensor_raw_info *s_s5k3l8xxm3q_mipi_raw_info_ptr =
    &s_s5k3l8xxm3q_mipi_raw_info;

SENSOR_INFO_T g_s5k3l8xxm3q_mipi_raw_info = {
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
    .identify_code = {{.reg_addr = s5k3l8xxm3q_PID_ADDR,
                       .reg_value = s5k3l8xxm3q_PID_VALUE},
                      {.reg_addr = s5k3l8xxm3q_VER_ADDR,
                       .reg_value = s5k3l8xxm3q_VER_VALUE}},

    .source_width_max = SNAPSHOT_WIDTH,
    .source_height_max = SNAPSHOT_HEIGHT,
    .name = (cmr_s8 *)SENSOR_NAME,
    .image_format = SENSOR_IMAGE_FORMAT_RAW,

    .module_info_tab = s_s5k3l8xxm3q_module_info_tab,
    .module_info_tab_size = ARRAY_SIZE(s_s5k3l8xxm3q_module_info_tab),

    .resolution_tab_info_ptr = s_s5k3l8xxm3q_resolution_tab_raw,
    .sns_ops = &s_s5k3l8xxm3q_ops_tab,
    .raw_info_ptr = &s_s5k3l8xxm3q_mipi_raw_info_ptr,

    .video_tab_info_ptr = NULL,
    .sensor_version_info = (cmr_s8 *)"s5k3l8xxm3q_v1",
};
