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
#ifndef _SENSOR_GC2375_MIPI_RAW_H_
#define _SENSOR_GC2375_MIPI_RAW_H_

#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"

#include "parameters/sensor_gc8024_raw_param_main.c"

#define VENDOR_NUM 1

#define CAMERA_IMAGE_180
#define SENSOR_NAME "gc8024"
#define I2C_SLAVE_ADDR 0x6e /* 8bit slave address*/

#define GC8024_PID_ADDR 0xf0
#define GC8024_PID_VALUE 0x80
#define GC8024_VER_ADDR 0xf1
#define GC8024_VER_VALUE 0x24

/* sensor parameters begin */
/* effective sensor output image size */
#define SNAPSHOT_WIDTH 3264
#define SNAPSHOT_HEIGHT 2448
#define PREVIEW_WIDTH 1632
#define PREVIEW_HEIGHT 1224

/*Raw Trim parameters*/
#define SNAPSHOT_TRIM_X 0
#define SNAPSHOT_TRIM_Y 0
#define SNAPSHOT_TRIM_W 3264
#define SNAPSHOT_TRIM_H 2448
#define PREVIEW_TRIM_X 0
#define PREVIEW_TRIM_Y 0
#define PREVIEW_TRIM_W 1632
#define PREVIEW_TRIM_H 1224
/*Mipi output*/
#define LANE_NUM 2
#define RAW_BITS 10

#define SNAPSHOT_MIPI_PER_LANE_BPS 720
#define PREVIEW_MIPI_PER_LANE_BPS 360

/*line time unit: 1ns*/
#define SNAPSHOT_LINE_TIME 53333 // 26674
#define PREVIEW_LINE_TIME 106000 // 26674

/* frame length*/
#define SNAPSHOT_FRAME_LENGTH 4230 // 623
#define PREVIEW_FRAME_LENGTH 2160  // 623

/* please ref your spec */
#define FRAME_OFFSET 0
#define SENSOR_MAX_GAIN 0x200 // 8x
#define SENSOR_BASE_GAIN 0x40
#define SENSOR_MIN_SHUTTER 4

/* please ref your spec
 * 1 : average binning
 * 2 : sum-average binning
 * 4 : sum binning
 */
#define BINNING_FACTOR 2

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

//#define IMAGE_NORMAL_MIRROR
#define IMAGE_H_MIRROR
//#define IMAGE_V_MIRROR
//#define IMAGE_HV_MIRROR

#ifdef IMAGE_NORMAL_MIRROR
#define MIRROR 0xd4
#define PRE_STARTY 0x03
#define PRE_STARTX 0x07
#define CAP_STARTY 0x03
#define CAP_STARTX 0x09
#endif

#ifdef IMAGE_H_MIRROR
#define MIRROR 0xd5
#define PRE_STARTY 0x03
#define PRE_STARTX 0x02
#define CAP_STARTY 0x03
#define CAP_STARTX 0x06
#endif

#ifdef IMAGE_V_MIRROR
#define MIRROR 0xd6
#define PRE_STARTY 0x04
#define PRE_STARTX 0x07
#define CAP_STARTY 0x08
#define CAP_STARTX 0x09
#endif

#ifdef IMAGE_HV_MIRROR
#define MIRROR 0xd7
#define PRE_STARTY 0x04
#define PRE_STARTX 0x02
#define CAP_STARTY 0x08
#define CAP_STARTX 0x06
#endif
static uint8_t PreorCap = 0; // pre:0  cap:1
static uint8_t gainlevel = 0;

const uint8_t Val28[2][4] = {
    {0x9f, 0xb0, 0xc0, 0xdf}, // preview
    {0x1f, 0x30, 0x40, 0x5f}, // capture
};

typedef struct gc8024_private_data{
    /* pre:0  cap:1 */
    uint8_t PreorCap;
    uint8_t gainlevel;
}PRIVATE_DATA;
/*==============================================================================
 * Description:
 * global variable
 *============================================================================*/
static struct hdr_info_t s_hdr_info = {
    2000000 / SNAPSHOT_LINE_TIME, /*min 5fps*/
    SNAPSHOT_FRAME_LENGTH - FRAME_OFFSET, SENSOR_BASE_GAIN};
static uint32_t s_current_default_frame_length = PREVIEW_FRAME_LENGTH;
static struct sensor_ev_info_t s_sensor_ev_info = {
    PREVIEW_FRAME_LENGTH - FRAME_OFFSET, SENSOR_BASE_GAIN,PREVIEW_FRAME_LENGTH};

//#define FEATURE_OTP    /*OTP function switch*/

#ifdef FEATURE_OTP
#include "parameters/sensor_gc8024_yyy_otp.c"
static struct otp_info_t *s_gc8024_otp_info_ptr = &s_gc8024_gcore_otp_info;
static struct raw_param_info_tab *s_gc8024_raw_param_tab_ptr =
    &s_gc8024_gcore_raw_param_tab; /*otp function interface*/
#endif

static struct sensor_ic_ops s_gc8024_ops_tab;

static struct sensor_raw_info *s_gc8024_mipi_raw_info_ptr =
    &s_gc8024_mipi_raw_info;
static EXIF_SPEC_PIC_TAKING_COND_T s_gc8024_exif_info;

/*//delay 200ms
{SENSOR_WRITE_DELAY, 200},
*/
static const SENSOR_REG_T gc8024_init_setting[] = {};

static const SENSOR_REG_T gc8024_preview_setting[] = {
    /*sys*/
    {0xfe, 0x00},
    {0xfe, 0x00},
    {0xfe, 0x00},
    {0xf7, 0x95},
    {0xf8, 0x08},
    {0xf9, 0x00},
    {0xfa, 0x84},
    {0xfc, 0xce},

    /*Analog*/
    {0xfe, 0x00},
    {0x03, 0x08},
    {0x04, 0xca},
    {0x05, 0x02},
    {0x06, 0x1c},
    {0x07, 0x00},
    {0x08, 0x10},
    {0x09, 0x00},
    {0x0a, 0x14},
    {0x0b, 0x00},
    {0x0c, 0x10},
    {0x0d, 0x09},
    {0x0e, 0x9c},
    {0x0f, 0x0c},
    {0x10, 0xd0},
    {0x17, MIRROR},
    {0x18, 0x02},
    {0x19, 0x0b},
    {0x1a, 0x19},
    {0x1c, 0x0c},
    {0x1d, 0x11}, // add 20160527
    {0x21, 0x12},
    {0x23, 0xb0},
    {0x28, 0xdf},
    {0x29, 0xd4},
    {0x2f, 0x4c},
    {0x30, 0xf8},
    {0xcd, 0x9a},
    {0xce, 0xfd},
    {0xd0, 0xd2},
    {0xd1, 0xa8},
    {0xd3, 0x35},
    {0xd8, 0x20},
    {0xda, 0x03},
    {0xdb, 0x4e},
    {0xdc, 0xb3},
    {0xde, 0x40},
    {0xe1, 0x1a},
    {0xe2, 0x00},
    {0xe3, 0x71},
    {0xe4, 0x78},
    {0xe5, 0x44},
    {0xe6, 0xdf},
    {0xe8, 0x02},
    {0xe9, 0x01},
    {0xea, 0x01},
    {0xeb, 0x02},
    {0xec, 0x02},
    {0xed, 0x01},
    {0xee, 0x01},
    {0xef, 0x02},

    /*ISP*/
    {0x80, 0x50},
    {0x88, 0x03},
    {0x89, 0x03},

    /*scaler mode*/
    {0x66, 0x3c},

    /*window*/
    {0x90, 0x01},
    {0x92, PRE_STARTY}, // crop y
    {0x94, PRE_STARTX}, // crop x
    {0x95, 0x04},
    {0x96, 0xc8},
    {0x97, 0x06},
    {0x98, 0x60},

    /*gain*/
    {0xfe, 0x01},
    {0x50, 0x00},
    {0x51, 0x08},
    {0x52, 0x10},
    {0x53, 0x18},
    {0x54, 0x19},
    {0x55, 0x1a},
    {0x56, 0x1b},
    {0x57, 0x1c},
    {0x58, 0x3c},
    {0xfe, 0x00},
    {0xb0, 0x48},
    {0xb1, 0x01},
    {0xb2, 0x00},
    {0xb6, 0x00},

    /*blk*/
    {0x40, 0x22},
    {0x41, 0x20},
    {0x42, 0x10},
    {0x4e, 0x00},
    {0x4f, 0x3c},
    {0x60, 0x00},
    {0x61, 0x80},
    {0x69, 0x03},
    {0x6c, 0x00},
    {0x6d, 0x0f},

    /*dark offset*/
    {0x35, 0x30},
    {0x36, 0x00},

    /*dark sun*/
    {0x37, 0xf0},
    {0x38, 0x80},
    {0x3b, 0xf0},
    {0x3d, 0x00},

    /*DD*/
    {0xfe, 0x01},
    {0xc2, 0x03},
    {0xc3, 0x00},
    {0xc4, 0xd8},
    {0xc5, 0x00},

    // new ob-setting
    // Scaler+Binning
    {0xfe, 0x00},
    {0x43, 0x06},
    {0xfe, 0x01},
    {0xbf, 0x41},
    {0xfe, 0x00},
    {0x45, 0x3f},
    {0x46, 0x3f},
    {0x47, 0x3f},
    {0x48, 0x3f},
    {0x49, 0x3f},
    {0x4a, 0x3f},
    {0x4b, 0x3f},
    {0x4c, 0x3f},
    //{0x37,0x00},
    {0xc0, 0x00},
    {0xc1, 0x00},
    {0xc2, 0x00},
    {0xc3, 0x00},
    {0xc4, 0x00},
    {0xc5, 0x00},
    {0xc6, 0x00},
    {0xc7, 0xff},
    {0xc8, 0x00},
    {0xfe, 0x01},
    {0x5b, 0x00},
    {0x5c, 0x00},
    {0x5d, 0x00},
    {0x5e, 0x00},
    {0x5f, 0x00},
    {0x60, 0x00},
    {0x61, 0x00},
    {0x62, 0xff},
    {0x63, 0x00},

    {0xfe, 0x00},

    /// end///

    /*mipi*/
    {0xfe, 0x03},
    {0x10, 0x01},
    {0x01, 0x07},
    {0x02, 0x34},
    {0x03, 0x13},
    {0x04, 0xf0},
    {0x06, 0x80},
    {0x11, 0x2b},
    {0x12, 0xf8},
    {0x13, 0x07},
    {0x15, 0x00},
    {0x16, 0x09},
    {0x18, 0x01},
    {0x19, 0x00},
    {0x1a, 0x00},
    {0x21, 0x10},
    {0x22, 0x02},
    {0x23, 0x10},
    {0x24, 0x02},
    {0x25, 0x12},
    {0x26, 0x04},
    {0x29, 0x02},
    {0x2a, 0x0b}, // 0a},
    {0x2b, 0x04},
    {0xfe, 0x00},
};

static const SENSOR_REG_T gc8024_snapshot_setting[] = {
    /*sys*/
    {0xfe, 0x00},
    {0xfe, 0x00},
    {0xfe, 0x00},
    {0xf7, 0x95},
    {0xf8, 0x08},
    {0xf9, 0x00},
    {0xfa, 0x09},
    {0xfc, 0xce},

    /*Analog*/
    {0xfe, 0x00},
    {0x03, 0x08},
    {0x04, 0xca},
    {0x05, 0x02},
    {0x06, 0x1c},
    {0x07, 0x00},
    {0x08, 0x10},
    {0x09, 0x00},
    {0x0a, 0x14},
    {0x0b, 0x00},
    {0x0c, 0x10},
    {0x0d, 0x09},
    {0x0e, 0x9c},
    {0x0f, 0x0c},
    {0x10, 0xd0},
    {0x17, MIRROR},
    {0x18, 0x02},
    {0x19, 0x0b},
    {0x1a, 0x19},
    {0x1c, 0x0c},
    {0x21, 0x12},
    {0x23, 0xb0},
    {0x28, 0x5f},
    {0x29, 0xd4},
    {0x2f, 0x4c},
    {0x30, 0xf8},
    {0xcd, 0x9a},
    {0xce, 0xfd},
    {0xd0, 0xd2},
    {0xd1, 0xa8},
    {0xd3, 0x35},
    {0xd8, 0x20},
    {0xda, 0x03},
    {0xdb, 0x4e},
    {0xdc, 0xb3},
    {0xde, 0x40},
    {0xe1, 0x1a},
    {0xe2, 0x00},
    {0xe3, 0x71},
    {0xe4, 0x78},
    {0xe5, 0x44},
    {0xe6, 0xdf},
    {0xe8, 0x02},
    {0xe9, 0x01},
    {0xea, 0x01},
    {0xeb, 0x02},
    {0xec, 0x02},
    {0xed, 0x01},
    {0xee, 0x01},
    {0xef, 0x02},

    /*ISP*/
    {0x80, 0x50},
    {0x88, 0x03},
    {0x89, 0x03},

    /*window*/
    {0x90, 0x01},
    {0x92, CAP_STARTY},
    {0x94, CAP_STARTX},
    {0x95, 0x09},
    {0x96, 0x90},
    {0x97, 0x0c},
    {0x98, 0xc0},

    /*gain*/
    {0xfe, 0x01},
    {0x50, 0x00},
    {0x51, 0x08},
    {0x52, 0x10},
    {0x53, 0x18},
    {0x54, 0x19},
    {0x55, 0x1a},
    {0x56, 0x1b},
    {0x57, 0x1c},
    {0x58, 0x3c},
    {0xfe, 0x00},

    {0xb0, 0x48},
    {0xb1, 0x01},
    {0xb2, 0x00},
    {0xb6, 0x00},

    /*blk*/
    {0x40, 0x22},
    {0x41, 0x20},
    {0x42, 0x10},
    {0x4e, 0x3c},
    {0x4f, 0x00},
    {0x60, 0x00},
    {0x61, 0x80},
    {0x69, 0x03},
    {0x6c, 0x00},
    {0x6d, 0x0f},

    /*dark offset*/
    {0x35, 0x30},
    {0x36, 0x00},

    /*dark sun*/
    {0x37, 0xf0},
    {0x38, 0x80},
    {0x3b, 0xf0},
    {0x3d, 0x00},

    /*dd*/
    {0xfe, 0x01},
    {0xc2, 0x03},
    {0xc3, 0x00},
    {0xc4, 0xd8},
    {0xc5, 0x00},

    // new ob-setting

    // Full Size
    {0xfe, 0x00},
    {0x43, 0x06},
    {0xfe, 0x01},
    {0xbf, 0x40},
    {0xfe, 0x00},
    {0x45, 0x3f},
    {0x46, 0x3f},
    {0x47, 0x3f},
    {0x48, 0x3f},
    {0x49, 0x3f},
    {0x4a, 0x3f},
    {0x4b, 0x3f},
    {0x4c, 0x3f},
    //{0x37,0x00},
    {0xc0, 0x00},
    {0xc1, 0x00},
    {0xc2, 0x00},
    {0xc3, 0x00},
    {0xc4, 0x00},
    {0xc5, 0xff},
    {0xc6, 0xff},
    {0xc7, 0xff},
    {0xc8, 0x00},
    {0xfe, 0x01},
    {0x5b, 0x00},
    {0x5c, 0x00},
    {0x5d, 0x00},
    {0x5e, 0x00},
    {0x5f, 0x00},
    {0x60, 0xff},
    {0x61, 0xff},
    {0x62, 0x00},
    {0x63, 0x00},

    {0xfe, 0x00},

    /// end////

    /*mipi*/
    {0xfe, 0x03},
    {0x10, 0x01},
    {0x01, 0x07},
    {0x02, 0x34},
    {0x03, 0x13},
    {0x04, 0xf0},
    {0x06, 0x80},
    {0x11, 0x2b},
    {0x12, 0xf0},
    {0x13, 0x0f},
    {0x15, 0x00},
    {0x16, 0x09},
    {0x18, 0x01},
    {0x19, 0x00},
    {0x1a, 0x00},
    {0x21, 0x10},
    {0x22, 0x05},
    {0x23, 0x30},
    {0x24, 0x02},
    {0x25, 0x15},
    {0x26, 0x08},
    {0x29, 0x06},
    {0x2a, 0x05}, // 04},
    {0x2b, 0x08},
    {0xfe, 0x00},
};

static struct sensor_res_tab_info s_gc8024_resolution_tab_raw[VENDOR_NUM] = {
    {
      .module_id = MODULE_SUNNY,
      .reg_tab = {
        {ADDR_AND_LEN_OF_ARRAY(gc8024_init_setting), PNULL, 0,
        .width = 0, .height = 0,
        .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW},

        {ADDR_AND_LEN_OF_ARRAY(gc8024_snapshot_setting), PNULL, 0,
        .width = SNAPSHOT_WIDTH, .height = SNAPSHOT_HEIGHT,
        .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW}}
    }
/*If there are multiple modules,please add here*/
};

static SENSOR_TRIM_T s_gc8024_resolution_trim_tab[VENDOR_NUM] = {
    {
     .module_id = MODULE_SUNNY,
     .trim_info = {
       {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
       {
        .trim_start_x = SNAPSHOT_TRIM_X,.trim_start_y = SNAPSHOT_TRIM_Y,
        .trim_width = SNAPSHOT_TRIM_W,.trim_height = SNAPSHOT_TRIM_H,
        .line_time = SNAPSHOT_LINE_TIME,.bps_per_lane = SNAPSHOT_MIPI_PER_LANE_BPS,
        .frame_line = SNAPSHOT_FRAME_LENGTH,
        .scaler_trim = {.x = 0, .y = 0, .w = SNAPSHOT_TRIM_W, .h = SNAPSHOT_TRIM_H}},
      }}
    /*If there are multiple modules,please add here*/
};

static const SENSOR_REG_T
    s_gc8024_preview_size_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
        /*video mode 0: ?fps */
        {{0xffff, 0xff}},
        /* video mode 1:?fps */
        {{0xffff, 0xff}},
        /* video mode 2:?fps */
        {{0xffff, 0xff}},
        /* video mode 3:?fps */
        {{0xffff, 0xff}}};

static const SENSOR_REG_T
    s_gc8024_capture_size_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
        /*video mode 0: ?fps */
        {{0xffff, 0xff}},
        /* video mode 1:?fps */
        {{0xffff, 0xff}},
        /* video mode 2:?fps */
        {{0xffff, 0xff}},
        /* video mode 3:?fps */
        {{0xffff, 0xff}}};

static SENSOR_VIDEO_INFO_T s_gc8024_video_info[SENSOR_MODE_MAX] = {
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{30, 30, 270, 90}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
     (SENSOR_REG_T **)s_gc8024_preview_size_video_tab},
    {{{2, 5, 338, 1000}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
     (SENSOR_REG_T **)s_gc8024_capture_size_video_tab},
};

static SENSOR_STATIC_INFO_T s_gc8024_static_info[VENDOR_NUM] = {
    {
     .module_id = MODULE_SUNNY,
     .static_info = {
         .f_num = 220,
         .focal_length = 346,
         .max_fps = 0,
         .max_adgain = 8,
         .ois_supported = 0,
         .pdaf_supported = 0,
         .exp_valid_frame_num = 1,
         .clamp_level = 64,
         .adgain_valid_frame_num = 1,
         .fov_info = {{4.614f, 3.444f}, 4.222f}}
    }
    /*If there are multiple modules,please add here*/
};

static SENSOR_MODE_FPS_INFO_T s_gc8024_mode_fps_info[VENDOR_NUM] = {
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

static struct sensor_module_info s_gc8024_module_info_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .module_info = {
         .major_i2c_addr = I2C_SLAVE_ADDR >> 1,
         .minor_i2c_addr = I2C_SLAVE_ADDR >> 1,

         .reg_addr_value_bits = SENSOR_I2C_REG_8BIT | SENSOR_I2C_VAL_8BIT |
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

         .sensor_interface = {
              .type = SENSOR_INTERFACE_TYPE_CSI2,
              .bus_width = LANE_NUM,
              .pixel_width = RAW_BITS,
              .is_loose = 0,
          },

         .change_setting_skip_num = 1,
         .horizontal_view_angle = 65,
         .vertical_view_angle = 60
      }
    }

/*If there are multiple modules,please add here*/
};

/*==============================================================================
 * Description:
 * sensor all info
 * please modify this variable acording your spec
 *============================================================================*/
SENSOR_INFO_T g_gc8024_mipi_raw_info = {
    .hw_signal_polarity = SENSOR_HW_SIGNAL_PCLK_P | SENSOR_HW_SIGNAL_VSYNC_P |
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
    .power_down_level = SENSOR_HIGH_LEVEL_PWDN,

    .identify_count = 1,
    .identify_code = {{GC8024_PID_ADDR, GC8024_PID_VALUE},
                      {GC8024_VER_ADDR, GC8024_VER_VALUE}},

    .source_width_max = SNAPSHOT_WIDTH,
    .source_height_max = SNAPSHOT_HEIGHT,
    .name = (cmr_s8 *)SENSOR_NAME,
    .image_format = SENSOR_IMAGE_FORMAT_RAW,

    .resolution_tab_info_ptr = s_gc8024_resolution_tab_raw,
    .sns_ops = &s_gc8024_ops_tab,
    .raw_info_ptr = &s_gc8024_mipi_raw_info_ptr,
    .ext_info_ptr = NULL,
    .module_info_tab = s_gc8024_module_info_tab,
    .module_info_tab_size = ARRAY_SIZE(s_gc8024_module_info_tab),

    .sensor_version_info = (cmr_s8 *) "gc8024_v1",
};

#endif
