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
 * V6.0
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

#define CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
#ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
#include "../af_zzz.h"
#endif
#include "parameters/sensor_ov8856s_raw_param_main.c"

#define CAMERA_IMAGE_180

#define SENSOR_NAME "ov8856s_mipi_raw"
#define I2C_SLAVE_ADDR 0x20 // 0x6c    /* 8bit slave address*/

#define ov8856s_PID_ADDR 0x300B
#define ov8856s_PID_VALUE 0x88
#define ov8856s_VER_ADDR 0x300C
#define ov8856s_VER_VALUE 0x5A

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
#define LANE_NUM 4
#define RAW_BITS 10

#define SNAPSHOT_MIPI_PER_LANE_BPS 720
#define PREVIEW_MIPI_PER_LANE_BPS 720

/*line time unit: 0.001us*/
#define SNAPSHOT_LINE_TIME 13410
#define PREVIEW_LINE_TIME 13410

/* frame length*/
#define SNAPSHOT_FRAME_LENGTH 2482
#define PREVIEW_FRAME_LENGTH 2482

/* please ref your spec */
#define FRAME_OFFSET 6
#define SENSOR_MAX_GAIN 0x7ff // 16 multiple
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
#define SUPPORT_AUTO_FRAME_LENGTH 0

/*delay 1 frame to write sensor gain*/
//#define GAIN_DELAY_1_FRAME

/* sensor parameters end */

/* isp parameters, please don't change it*/
#define ISP_BASE_GAIN 0x80

/* please don't change it */
#define EX_MCLK 24

#define IMAGE_NORMAL_MIRROR
//#define IMAGE_H_MIRROR
//#define IMAGE_V_MIRROR
//#define IMAGE_HV_MIRROR

/*==============================================================================
 * Description:
 * global variable
 *============================================================================*/
static uint32_t s_current_default_frame_length = PREVIEW_FRAME_LENGTH;
static struct sensor_ev_info_t s_sensor_ev_info = {
    PREVIEW_FRAME_LENGTH - FRAME_OFFSET, SENSOR_BASE_GAIN,
    PREVIEW_FRAME_LENGTH};

//#define FEATURE_OTP    /*OTP function switch*/

#ifdef FEATURE_OTP
#include "parameters/sensor_ov8856s_darling_otp.c"
#else
#include "parameters/ov8856s_darling_otp.h"
#endif

static SENSOR_IOCTL_FUNC_TAB_T s_ov8856s_ioctl_func_tab;
struct sensor_raw_info *s_ov8856s_mipi_raw_info_ptr = &s_ov8856s_mipi_raw_info;
/*//delay 200ms
{SENSOR_WRITE_DELAY, 200},
*/

static const SENSOR_REG_T ov8856s_init_setting[] = {
    // 100 99 1632x1224
    {0x0103, 0x01}, {0x0100, 0x00}, {0x0302, 0x3c}, {0x0303, 0x01},
    {0x031e, 0x0c}, {0x3000, 0x00}, {0x300e, 0x00}, {0x3010, 0x00},
    {0x3015, 0x84}, {0x3018, 0x72}, {0x3033, 0x24}, {0x3500, 0x00},
    {0x3501, 0x4c}, {0x3502, 0xe0}, {0x3503, 0x08}, {0x3505, 0x83},
    {0x3508, 0x01}, {0x3509, 0x80}, {0x350c, 0x00}, {0x350d, 0x80},
    {0x350e, 0x04}, {0x350f, 0x00}, {0x3510, 0x00}, {0x3511, 0x02},
    {0x3512, 0x00}, {0x3600, 0x72}, {0x3601, 0x40}, {0x3602, 0x30},
    {0x3610, 0xc5}, {0x3611, 0x58}, {0x3612, 0x5c}, {0x3613, 0x5a},
    {0x3614, 0x60}, {0x3628, 0xff}, {0x3629, 0xff}, {0x362a, 0xff},
    {0x3633, 0x10}, {0x3634, 0x10}, {0x3635, 0x10}, {0x3636, 0x10},
    {0x3663, 0x08}, {0x3669, 0x34}, {0x366e, 0x08}, {0x3706, 0x86},
    {0x370b, 0x7e}, {0x3714, 0x27}, {0x3730, 0x12}, {0x3733, 0x10},
    {0x3764, 0x00}, {0x3765, 0x00}, {0x3769, 0x62}, {0x376a, 0x2a},
    {0x376b, 0x30}, {0x3780, 0x00}, {0x3781, 0x24}, {0x3782, 0x00},
    {0x3783, 0x23}, {0x3798, 0x2f}, {0x37a1, 0x60}, {0x37a8, 0x6a},
    {0x37ab, 0x3f}, {0x37c2, 0x14}, {0x37c3, 0xf1}, {0x37c9, 0x80},
    {0x37cb, 0x03}, {0x37cc, 0x0a}, {0x37cd, 0x16}, {0x37ce, 0x1f},
    {0x3800, 0x00}, {0x3801, 0x00}, {0x3802, 0x00}, {0x3803, 0x0c},
    {0x3804, 0x0c}, {0x3805, 0xdf}, {0x3806, 0x09}, {0x3807, 0xa3},
    {0x3808, 0x06}, {0x3809, 0x60}, {0x380a, 0x04}, {0x380b, 0xc8},
    {0x380c, 0x07}, {0x380d, 0x8c}, {0x380e, 0x04}, {0x380f, 0xde},
    {0x3810, 0x00}, {0x3811, 0x08}, {0x3812, 0x00}, {0x3813, 0x02},
    {0x3814, 0x03}, {0x3815, 0x01}, {0x3816, 0x00}, {0x3817, 0x00},
    {0x3818, 0x00}, {0x3819, 0x00}, {0x3820, 0x90}, {0x3821, 0x67},
    {0x382a, 0x03}, {0x382b, 0x01}, {0x3830, 0x06}, {0x3836, 0x02},
    {0x3862, 0x04}, {0x3863, 0x08}, {0x3cc0, 0x33}, {0x3d85, 0x17},
    {0x3d8c, 0x73}, {0x3d8d, 0xde}, {0x4001, 0xe0}, {0x4003, 0x40},
    {0x4008, 0x00}, {0x4009, 0x05}, {0x400f, 0x80}, {0x4010, 0xf0},
    {0x4011, 0xff}, {0x4012, 0x02}, {0x4013, 0x01}, {0x4014, 0x01},
    {0x4015, 0x01}, {0x4042, 0x00}, {0x4043, 0x80}, {0x4044, 0x00},
    {0x4045, 0x80}, {0x4046, 0x00}, {0x4047, 0x80}, {0x4048, 0x00},
    {0x4049, 0x80}, {0x4041, 0x03}, {0x404c, 0x20}, {0x404d, 0x00},
    {0x404e, 0x20}, {0x4203, 0x80}, {0x4307, 0x30}, {0x4317, 0x00},
    {0x4503, 0x08}, {0x4601, 0x80}, {0x4816, 0x53}, {0x481b, 0x58},
    {0x481f, 0x27}, {0x4837, 0x16}, {0x5000, 0x77}, {0x5001, 0x0a},
    {0x5004, 0x04}, {0x502e, 0x03}, {0x5030, 0x41}, {0x5795, 0x00},
    {0x5796, 0x10}, {0x5797, 0x10}, {0x5798, 0x73}, {0x5799, 0x73},
    {0x579a, 0x00}, {0x579b, 0x28}, {0x579c, 0x00}, {0x579d, 0x16},
    {0x579e, 0x06}, {0x579f, 0x20}, {0x57a0, 0x04}, {0x57a1, 0xa0},
    {0x5780, 0x14}, {0x5781, 0x0f}, {0x5782, 0x44}, {0x5783, 0x02},
    {0x5784, 0x01}, {0x5785, 0x01}, {0x5786, 0x00}, {0x5787, 0x04},
    {0x5788, 0x02}, {0x5789, 0x0f}, {0x578a, 0xfd}, {0x578b, 0xf5},
    {0x578c, 0xf5}, {0x578d, 0x03}, {0x578e, 0x08}, {0x578f, 0x0c},
    {0x5790, 0x08}, {0x5791, 0x04}, {0x5792, 0x00}, {0x5793, 0x52},
    {0x5794, 0xa3}, {0x59f8, 0x3d}, // update from AM11a, for LENC h-band issue
    {0x5a08, 0x02}, {0x5b00, 0x02}, {0x5b01, 0x10}, {0x5b02, 0x03},
    {0x5b03, 0xcf}, {0x5b05, 0x6c}, {0x5e00, 0x00},
    //{0x0100, 0x01},

};

static const SENSOR_REG_T ov8856s_preview_setting[] = {
    // mipi bit rate 720Mbps, //1632x1224, 30fps, frame_length 2482,line time
    // 13.41us

    //{0x0100, 0x00},
    {0x366e, 0x08},
    {0x3714, 0x27},
    {0x37c2, 0x14},
    {0x3808, 0x06},
    {0x3809, 0x60},
    {0x380a, 0x04},
    {0x380b, 0xc8},
    {0x380e, 0x09},
    {0x380f, 0xb2},
    {0x3811, 0x02},
    {0x3813, 0x02},
    {0x3814, 0x03},

#ifdef IMAGE_NORMAL_MIRROR
    {0x3820, 0x90},
    {0x3821, 0x67},
    {0x502e, 0x03},
    {0x5001, 0x0a},
    {0x5004, 0x04},
    {0x376b, 0x30},
#endif
#ifdef IMAGE_H_MIRROR
    {0x3820, 0x90},
    {0x3821, 0x61},
    {0x502e, 0x03},
    {0x5001, 0x0a},
    {0x5004, 0x00},
    {0x376b, 0x30},
#endif
#ifdef IMAGE_V_MIRROR
    {0x3820, 0x96},
    {0x3821, 0x67},
    {0x502e, 0x00},
    {0x5001, 0x0e},
    {0x5004, 0x04},
    {0x376b, 0x36},
#endif
#ifdef IMAGE_HV_MIRROR
    {0x3820, 0x96},
    {0x3821, 0x61},
    {0x502e, 0x00},
    {0x5001, 0x0e},
    {0x5004, 0x00},
    {0x376b, 0x36},
#endif
    /*=============flip mirror end=====================*/
    {0x382a, 0x03},
    {0x4009, 0x05},
    {0x5795, 0x00},
    {0x5796, 0x10},
    {0x5797, 0x10},
    {0x5798, 0x73},
    {0x5799, 0x73},
    {0x579b, 0x28},
    {0x579d, 0x16},
    {0x579e, 0x06},
    {0x579f, 0x20},
    {0x57a0, 0x04},
    {0x57a1, 0xa0},
    //{0x0100, 0x01},
};

static const SENSOR_REG_T ov8856s_snapshot_setting[] = {

    // mipi bit rate 720Mbps,3264x2448,  30fps, frame_length 2482,line time
    // 13.41us
    // {0x0100,0x00},
    {0x366e, 0x10},
    {0x3714, 0x23},
    {0x37c2, 0x04},
    {0x3808, 0x0c},
    {0x3809, 0xc0},
    {0x380a, 0x09},
    {0x380b, 0x90},
    {0x380e, 0x09},
    {0x380f, 0xb2},
    {0x3811, 0x10},
    {0x3813, 0x04},
    {0x3814, 0x01},

/*=============flip mirror start=====================*/

#ifdef IMAGE_NORMAL_MIRROR
    {0x3820, 0x80},
    {0x3821, 0x46},
    {0x502e, 0x03},
    {0x5001, 0x0a},
    {0x5004, 0x04},
    {0x376b, 0x30},
#endif
#ifdef IMAGE_H_MIRROR
    {0x3820, 0x80},
    {0x3821, 0x40},
    {0x502e, 0x03},
    {0x5001, 0x0a},
    {0x5004, 0x00},
    {0x376b, 0x30},
#endif
#ifdef IMAGE_V_MIRROR
    {0x3820, 0x86},
    {0x3821, 0x46},
    {0x502e, 0x00},
    {0x5001, 0x0e},
    {0x5004, 0x04},
    {0x376b, 0x36},
#endif
#ifdef IMAGE_HV_MIRROR
    {0x3820, 0x86},
    {0x3821, 0x40},
    {0x502e, 0x00},
    {0x5001, 0x0e},
    {0x5004, 0x00},
    {0x376b, 0x36},
#endif

    /*=============flip mirror end=====================*/

    {0x382a, 0x01},
    {0x4009, 0x0b},
    {0x5795, 0x02},
    {0x5796, 0x20},
    {0x5797, 0x20},
    {0x5798, 0xd5},
    {0x5799, 0xd5},
    {0x579b, 0x50},
    {0x579d, 0x2c},
    {0x579e, 0x0c},
    {0x579f, 0x40},
    {0x57a0, 0x09},
    {0x57a1, 0x40},
    //{0x0100,0x01},

};

static SENSOR_REG_TAB_INFO_T s_ov8856s_resolution_tab_raw[SENSOR_MODE_MAX] = {
    {ADDR_AND_LEN_OF_ARRAY(ov8856s_init_setting), 0, 0, EX_MCLK,
     SENSOR_IMAGE_FORMAT_RAW},
    {ADDR_AND_LEN_OF_ARRAY(ov8856s_preview_setting), PREVIEW_WIDTH,
     PREVIEW_HEIGHT, EX_MCLK, SENSOR_IMAGE_FORMAT_RAW},
    {ADDR_AND_LEN_OF_ARRAY(ov8856s_snapshot_setting), SNAPSHOT_WIDTH,
     SNAPSHOT_HEIGHT, EX_MCLK, SENSOR_IMAGE_FORMAT_RAW},
};

static SENSOR_TRIM_T s_ov8856s_resolution_trim_tab[SENSOR_MODE_MAX] = {
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
    {PREVIEW_TRIM_X,
     PREVIEW_TRIM_Y,
     PREVIEW_TRIM_W,
     PREVIEW_TRIM_H,
     PREVIEW_LINE_TIME,
     PREVIEW_MIPI_PER_LANE_BPS,
     PREVIEW_FRAME_LENGTH,
     {0, 0, PREVIEW_TRIM_W, PREVIEW_TRIM_H}},
    {SNAPSHOT_TRIM_X,
     SNAPSHOT_TRIM_Y,
     SNAPSHOT_TRIM_W,
     SNAPSHOT_TRIM_H,
     SNAPSHOT_LINE_TIME,
     SNAPSHOT_MIPI_PER_LANE_BPS,
     SNAPSHOT_FRAME_LENGTH,
     {0, 0, SNAPSHOT_TRIM_W, SNAPSHOT_TRIM_H}},
};

/*==============================================================================
 * Description:
 * sensor all info
 * please modify this variable acording your spec
 *============================================================================*/
SENSOR_INFO_T g_ov8856s_mipi_raw_info = {
    /* salve i2c write address */
    (I2C_SLAVE_ADDR >> 1),
    /* salve i2c read address */
    (I2C_SLAVE_ADDR >> 1),
    /*bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit */
    SENSOR_I2C_REG_16BIT | SENSOR_I2C_VAL_8BIT | SENSOR_I2C_FREQ_400,
    /* bit2: 0:negative; 1:positive -> polarily of horizontal synchronization
     * signal
     * bit4: 0:negative; 1:positive -> polarily of vertical synchronization
     * signal
     * other bit: reseved
     */
    SENSOR_HW_SIGNAL_PCLK_P | SENSOR_HW_SIGNAL_VSYNC_P |
        SENSOR_HW_SIGNAL_HSYNC_P,
    /* preview mode */
    SENSOR_ENVIROMENT_NORMAL | SENSOR_ENVIROMENT_NIGHT,
    /* image effect */
    SENSOR_IMAGE_EFFECT_NORMAL | SENSOR_IMAGE_EFFECT_BLACKWHITE |
        SENSOR_IMAGE_EFFECT_RED | SENSOR_IMAGE_EFFECT_GREEN |
        SENSOR_IMAGE_EFFECT_BLUE | SENSOR_IMAGE_EFFECT_YELLOW |
        SENSOR_IMAGE_EFFECT_NEGATIVE | SENSOR_IMAGE_EFFECT_CANVAS,

    /* while balance mode */
    0,
    /* bit[0:7]: count of step in brightness, contrast, sharpness, saturation
     * bit[8:31] reseved
     */
    7,
    /* reset pulse level */
    SENSOR_LOW_PULSE_RESET,
    /* reset pulse width(ms) */
    50,
    /* 1: high level valid; 0: low level valid */
    SENSOR_LOW_LEVEL_PWDN,
    /* count of identify code */
    1,
    /* supply two code to identify sensor.
     * for Example: index = 0-> Device id, index = 1 -> version id
     * customer could ignore it.
     */
    {{ov8856s_PID_ADDR, ov8856s_PID_VALUE},
     {ov8856s_VER_ADDR, ov8856s_VER_VALUE}},
    /* voltage of avdd */
    SENSOR_AVDD_2800MV,
    /* max width of source image */
    SNAPSHOT_WIDTH,
    /* max height of source image */
    SNAPSHOT_HEIGHT,
    /* name of sensor */
    (cmr_s8 *)SENSOR_NAME,
    /* define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
     * if set to SENSOR_IMAGE_FORMAT_MAX here,
     * image format depent on SENSOR_REG_TAB_INFO_T
     */
    SENSOR_IMAGE_FORMAT_RAW,
    /*  pattern of input image form sensor */
    SENSOR_IMAGE_PATTERN_RAWRGB_B,
    /* point to resolution table information structure */
    s_ov8856s_resolution_tab_raw,
    /* point to ioctl function table */
    &s_ov8856s_ioctl_func_tab,
    /* information and table about Rawrgb sensor */
    &s_ov8856s_mipi_raw_info_ptr,
    /* extend information about sensor
     * like &g_ov8856s_ext_info
     */
    NULL,
    /* voltage of iovdd */
    SENSOR_AVDD_1800MV,
    /* voltage of dvdd */
    SENSOR_AVDD_1200MV,
    /* skip frame num before preview */
    1,
    /* skip frame num before capture */
    1,
    /* skip frame num for flash capture */
    6,
    /* skip frame num on mipi cap */
    0,
    /* deci frame num during preview */
    0,
    /* deci frame num during video preview */
    0,
    0,
    0,
    0,
    0,
    0,
    {SENSOR_INTERFACE_TYPE_CSI2, LANE_NUM, RAW_BITS, 0},
    0,
    /* skip frame num while change setting */
    1,
    /* horizontal  view angle*/
    65,
    /* vertical view angle*/
    60,
    (cmr_s8 *)"ov8856s_v1"};

static SENSOR_STATIC_INFO_T s_ov8856s_static_info = {
    220, // f-number,focal ratio
    250, // focal_length;
    0,   // max_fps,max fps of sensor's all settings,it will be calculated from
         // sensor mode fps
    16 * 2, // max_adgain,AD-gain
    0,      // ois_supported;
    0,      // pdaf_supported;
    1,      // exp_valid_frame_num;N+2-1
    64,     // clamp_level,black level
    1,      // adgain_valid_frame_num;N+1-1
};

static SENSOR_MODE_FPS_INFO_T s_ov8856s_mode_fps_info = {
    0, // is_init;
    {{SENSOR_MODE_COMMON_INIT, 0, 1, 0, 0},
     {SENSOR_MODE_PREVIEW_ONE, 0, 1, 0, 0},
     {SENSOR_MODE_SNAPSHOT_ONE_FIRST, 0, 1, 0, 0},
     {SENSOR_MODE_SNAPSHOT_ONE_SECOND, 0, 1, 0, 0},
     {SENSOR_MODE_SNAPSHOT_ONE_THIRD, 0, 1, 0, 0},
     {SENSOR_MODE_PREVIEW_TWO, 0, 1, 0, 0},
     {SENSOR_MODE_SNAPSHOT_TWO_FIRST, 0, 1, 0, 0},
     {SENSOR_MODE_SNAPSHOT_TWO_SECOND, 0, 1, 0, 0},
     {SENSOR_MODE_SNAPSHOT_TWO_THIRD, 0, 1, 0, 0}}};

/*==============================================================================
 * Description:
 * get default frame length
 *
 *============================================================================*/
static uint32_t ov8856s_get_default_frame_length(SENSOR_HW_HANDLE handle,
                                                 uint32_t mode) {
    return s_ov8856s_resolution_trim_tab[mode].frame_line;
}

/*==============================================================================
 * Description:
 * write group-hold on to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void ov8856s_group_hold_on(SENSOR_HW_HANDLE handle) {
    SENSOR_PRINT("E");
}

/*==============================================================================
 * Description:
 * write group-hold off to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void ov8856s_group_hold_off(SENSOR_HW_HANDLE handle) {
    SENSOR_PRINT("E");
}

/*==============================================================================
 * Description:
 * read gain from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint16_t ov8856s_read_gain(SENSOR_HW_HANDLE handle) {
    return s_sensor_ev_info.preview_gain;
}

/*==============================================================================
 * Description:
 * write gain to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void ov8856s_write_gain(SENSOR_HW_HANDLE handle, uint32_t gain) {
    float gain_a = gain;
    float gain_d = 0x400;

    if (SENSOR_MAX_GAIN < (uint16_t)gain_a) {

        gain_a = SENSOR_MAX_GAIN;
        gain_d = gain * 0x400 / gain_a;
        if ((uint16_t)gain_d > 0x2 * 0x400 - 1)
            gain_d = 0x2 * 0x400 - 1;
    }
    // Sensor_WriteReg(0x320a, 0x01);

    // group 1:all other registers( gain)
    Sensor_WriteReg(0x3208, 0x01);

    Sensor_WriteReg(0x3508, ((uint16_t)gain_a >> 8) & 0x07);
    Sensor_WriteReg(0x3509, (uint16_t)gain_a & 0xff);
    Sensor_WriteReg(0x5019, ((uint16_t)gain_d >> 8) & 0x07);
    Sensor_WriteReg(0x501a, (uint16_t)gain_d & 0xff);
    Sensor_WriteReg(0x501b, ((uint16_t)gain_d >> 8) & 0x07);
    Sensor_WriteReg(0x501c, (uint16_t)gain_d & 0xff);
    Sensor_WriteReg(0x501d, ((uint16_t)gain_d >> 8) & 0x07);
    Sensor_WriteReg(0x501e, (uint16_t)gain_d & 0xff);
    Sensor_WriteReg(0x501f, ((uint16_t)gain_d >> 8) & 0x07);
    Sensor_WriteReg(0x5020, (uint16_t)gain_d & 0xff);
    Sensor_WriteReg(0x3208, 0x11);
    Sensor_WriteReg(0x3208, 0xA1);
}

/*==============================================================================
 * Description:
 * read frame length from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint16_t ov8856s_read_frame_length(SENSOR_HW_HANDLE handle) {
    uint32_t frame_len;

    frame_len = Sensor_ReadReg(0x380e) & 0xff;
    frame_len = frame_len << 8 | (Sensor_ReadReg(0x380f) & 0xff);
    s_sensor_ev_info.preview_framelength = frame_len;

    return s_sensor_ev_info.preview_framelength;
}

/*==============================================================================
 * Description:
 * write frame length to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void ov8856s_write_frame_length(SENSOR_HW_HANDLE handle,
                                       uint32_t frame_len) {
    Sensor_WriteReg(0x380e, (frame_len >> 8) & 0xff);
    Sensor_WriteReg(0x380f, frame_len & 0xff);
    s_sensor_ev_info.preview_framelength = frame_len;
}

/*==============================================================================
 * Description:
 * read shutter from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t ov8856s_read_shutter(SENSOR_HW_HANDLE handle) {
    uint32_t value = 0x00;
    uint8_t shutter_l = 0x00;
    uint8_t shutter_m = 0x00;
    uint8_t shutter_h = 0x00;

    shutter_l = Sensor_ReadReg(0x3502);
    // value=(shutter>>0x04)&0x0f;
    shutter_m = Sensor_ReadReg(0x3501);
    // value+=(shutter&0xff)<<0x04;
    shutter_h = Sensor_ReadReg(0x3500);
    // value+=(shutter&0x0f)<<0x0c;
    value = ((shutter_h & 0x0f) << 12) + (shutter_m << 4) +
            ((shutter_l >> 4) & 0x0f);
    s_sensor_ev_info.preview_shutter = value; // shutter;
    return s_sensor_ev_info.preview_shutter;
}

/*==============================================================================
 * Description:
 * write shutter to sensor registers
 * please pay attention to the frame length
 * please modify this function acording your spec
 *============================================================================*/
static void ov8856s_write_shutter(SENSOR_HW_HANDLE handle, uint32_t shutter) {
    uint16_t value = 0x00;
    value = (shutter << 0x04) & 0xff;
    Sensor_WriteReg(0x3502, value);
    value = (shutter >> 0x04) & 0xff;
    Sensor_WriteReg(0x3501, value);
    value = (shutter >> 0x0c) & 0x0f;
    Sensor_WriteReg(0x3500, value);
    s_sensor_ev_info.preview_shutter = shutter;
}

/*==============================================================================
 * Description:
 * write exposure to sensor registers and get current shutter
 * please pay attention to the frame length
 * please don't change this function if it's necessary
 *============================================================================*/
static uint16_t ov8856s_write_exposure_dummy(SENSOR_HW_HANDLE handle,
                                             uint32_t shutter,
                                             uint32_t dummy_line,
                                             uint16_t size_index) {
    uint32_t dest_fr_len = 0;
    uint32_t cur_fr_len = 0;
    uint32_t fr_len = s_current_default_frame_length;

    // ov8856s_group_hold_on(handle);

    if (1 == SUPPORT_AUTO_FRAME_LENGTH)
        goto write_sensor_shutter;

    dummy_line = dummy_line > FRAME_OFFSET ? dummy_line : FRAME_OFFSET;
    dest_fr_len =
        ((shutter + dummy_line) > fr_len) ? (shutter + dummy_line) : fr_len;

    cur_fr_len = ov8856s_read_frame_length(handle);

    if (shutter < SENSOR_MIN_SHUTTER)
        shutter = SENSOR_MIN_SHUTTER;

    if (dest_fr_len != cur_fr_len)
        ov8856s_write_frame_length(handle, dest_fr_len);
write_sensor_shutter:
    /* write shutter to sensor registers */
    s_sensor_ev_info.preview_shutter = shutter;
    ov8856s_write_shutter(handle, shutter);

#ifdef GAIN_DELAY_1_FRAME
    usleep(dest_fr_len * PREVIEW_LINE_TIME / 10);
#endif

    return SENSOR_SUCCESS;
}

/*==============================================================================
 * Description:
 * sensor power on
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t ov8856s_power_on(SENSOR_HW_HANDLE handle, uint32_t power_on) {
    SENSOR_AVDD_VAL_E dvdd_val = g_ov8856s_mipi_raw_info.dvdd_val;
    SENSOR_AVDD_VAL_E avdd_val = g_ov8856s_mipi_raw_info.avdd_val;
    SENSOR_AVDD_VAL_E iovdd_val = g_ov8856s_mipi_raw_info.iovdd_val;
    BOOLEAN power_down = g_ov8856s_mipi_raw_info.power_down_level;
    BOOLEAN reset_level = g_ov8856s_mipi_raw_info.reset_pulse_level;

    if (SENSOR_TRUE == power_on) {
        Sensor_PowerDown(power_down);
        Sensor_SetResetLevel(reset_level);
        usleep(10 * 1000);
        Sensor_SetAvddVoltage(avdd_val);
        Sensor_SetDvddVoltage(dvdd_val);
        Sensor_SetIovddVoltage(iovdd_val);
        usleep(10 * 1000);
        Sensor_PowerDown(!power_down);
        Sensor_SetResetLevel(!reset_level);
        usleep(10 * 1000);
        Sensor_SetMCLK(EX_MCLK);
        Sensor_SetMIPILevel(1);

#ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
        Sensor_SetMonitorVoltage(SENSOR_AVDD_2800MV);
        usleep(5 * 1000);
        zzz_init(2);
#else
        Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
#endif

    } else {
        Sensor_SetMIPILevel(0);
#ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
        zzz_deinit(2);
        Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
#endif

        Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
        usleep(10 * 1000);
        Sensor_SetResetLevel(reset_level);
        Sensor_PowerDown(power_down);
        usleep(10 * 1000);
        Sensor_SetAvddVoltage(SENSOR_AVDD_CLOSED);
        Sensor_SetDvddVoltage(SENSOR_AVDD_CLOSED);
        Sensor_SetIovddVoltage(SENSOR_AVDD_CLOSED);
    }
    SENSOR_PRINT("(1:on, 0:off): %d", power_on);
    return SENSOR_SUCCESS;
}

/*==============================================================================
 * Description:
 * calculate fps for every sensor mode according to frame_line and line_time
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t ov8856s_init_mode_fps_info(SENSOR_HW_HANDLE handle) {
    uint32_t rtn = SENSOR_SUCCESS;
    SENSOR_PRINT("ov8856s_init_mode_fps_info:E");
    if (!s_ov8856s_mode_fps_info.is_init) {
        uint32_t i, modn, tempfps = 0;
        SENSOR_PRINT("ov8856s_init_mode_fps_info:start init");
        for (i = 0; i < NUMBER_OF_ARRAY(s_ov8856s_resolution_trim_tab); i++) {
            // max fps should be multiple of 30,it calulated from line_time and
            // frame_line
            tempfps = s_ov8856s_resolution_trim_tab[i].line_time *
                      s_ov8856s_resolution_trim_tab[i].frame_line;
            if (0 != tempfps) {
                tempfps = 1000000000 / tempfps;
                modn = tempfps / 30;
                if (tempfps > modn * 30)
                    modn++;
                s_ov8856s_mode_fps_info.sensor_mode_fps[i].max_fps = modn * 30;
                if (s_ov8856s_mode_fps_info.sensor_mode_fps[i].max_fps > 30) {
                    s_ov8856s_mode_fps_info.sensor_mode_fps[i].is_high_fps = 1;
                    s_ov8856s_mode_fps_info.sensor_mode_fps[i]
                        .high_fps_skip_num =
                        s_ov8856s_mode_fps_info.sensor_mode_fps[i].max_fps / 30;
                }
                if (s_ov8856s_mode_fps_info.sensor_mode_fps[i].max_fps >
                    s_ov8856s_static_info.max_fps) {
                    s_ov8856s_static_info.max_fps =
                        s_ov8856s_mode_fps_info.sensor_mode_fps[i].max_fps;
                }
            }
            SENSOR_PRINT("mode %d,tempfps %d,frame_len %d,line_time: %d ", i,
                         tempfps, s_ov8856s_resolution_trim_tab[i].frame_line,
                         s_ov8856s_resolution_trim_tab[i].line_time);
            SENSOR_PRINT("mode %d,max_fps: %d ", i,
                         s_ov8856s_mode_fps_info.sensor_mode_fps[i].max_fps);
            SENSOR_PRINT(
                "is_high_fps: %d,highfps_skip_num %d",
                s_ov8856s_mode_fps_info.sensor_mode_fps[i].is_high_fps,
                s_ov8856s_mode_fps_info.sensor_mode_fps[i].high_fps_skip_num);
        }
        s_ov8856s_mode_fps_info.is_init = 1;
    }
    SENSOR_PRINT("ov8856s_init_mode_fps_info:X");
    return rtn;
}

static uint32_t ov8856s_get_static_info(SENSOR_HW_HANDLE handle,
                                        uint32_t *param) {
    uint32_t rtn = SENSOR_SUCCESS;
    struct sensor_ex_info *ex_info;
    uint32_t up = 0;
    uint32_t down = 0;
    // make sure we have get max fps of all settings.
    if (!s_ov8856s_mode_fps_info.is_init) {
        ov8856s_init_mode_fps_info(handle);
    }
    ex_info = (struct sensor_ex_info *)param;
    ex_info->f_num = s_ov8856s_static_info.f_num;
    ex_info->focal_length = s_ov8856s_static_info.focal_length;
    ex_info->max_fps = s_ov8856s_static_info.max_fps;
    ex_info->max_adgain = s_ov8856s_static_info.max_adgain;
    ex_info->ois_supported = s_ov8856s_static_info.ois_supported;
    ex_info->pdaf_supported = s_ov8856s_static_info.pdaf_supported;
    ex_info->exp_valid_frame_num = s_ov8856s_static_info.exp_valid_frame_num;
    ex_info->clamp_level = s_ov8856s_static_info.clamp_level;
    ex_info->adgain_valid_frame_num =
        s_ov8856s_static_info.adgain_valid_frame_num;
    ex_info->preview_skip_num = g_ov8856s_mipi_raw_info.preview_skip_num;
    ex_info->capture_skip_num = g_ov8856s_mipi_raw_info.capture_skip_num;
    ex_info->name = (cmr_s8 *)g_ov8856s_mipi_raw_info.name;
    ex_info->sensor_version_info = (cmr_s8 *)g_ov8856s_mipi_raw_info.sensor_version_info;
    // vcm_dw9800_get_pose_dis(handle, &up, &down);
    ex_info->pos_dis.up2hori = up;
    ex_info->pos_dis.hori2down = down;
    SENSOR_PRINT("f_num: %d", ex_info->f_num);
    SENSOR_PRINT("max_fps: %d", ex_info->max_fps);
    SENSOR_PRINT("max_adgain: %d", ex_info->max_adgain);
    SENSOR_PRINT("ois_supported: %d", ex_info->ois_supported);
    SENSOR_PRINT("pdaf_supported: %d", ex_info->pdaf_supported);
    SENSOR_PRINT("exp_valid_frame_num: %d", ex_info->exp_valid_frame_num);
    SENSOR_PRINT("clam_level: %d", ex_info->clamp_level);
    SENSOR_PRINT("adgain_valid_frame_num: %d", ex_info->adgain_valid_frame_num);
    SENSOR_PRINT("sensor name is: %s", ex_info->name);
    SENSOR_PRINT("sensor version info is: %s", ex_info->sensor_version_info);

    return rtn;
}

static uint32_t ov8856s_get_fps_info(SENSOR_HW_HANDLE handle, uint32_t *param) {
    uint32_t rtn = SENSOR_SUCCESS;
    SENSOR_MODE_FPS_T *fps_info;
    // make sure have inited fps of every sensor mode.
    if (!s_ov8856s_mode_fps_info.is_init) {
        ov8856s_init_mode_fps_info(handle);
    }
    fps_info = (SENSOR_MODE_FPS_T *)param;
    uint32_t sensor_mode = fps_info->mode;
    fps_info->max_fps =
        s_ov8856s_mode_fps_info.sensor_mode_fps[sensor_mode].max_fps;
    fps_info->min_fps =
        s_ov8856s_mode_fps_info.sensor_mode_fps[sensor_mode].min_fps;
    fps_info->is_high_fps =
        s_ov8856s_mode_fps_info.sensor_mode_fps[sensor_mode].is_high_fps;
    fps_info->high_fps_skip_num =
        s_ov8856s_mode_fps_info.sensor_mode_fps[sensor_mode].high_fps_skip_num;
    SENSOR_PRINT("mode %d, max_fps: %d", fps_info->mode, fps_info->max_fps);
    SENSOR_PRINT("min_fps: %d", fps_info->min_fps);
    SENSOR_PRINT("is_high_fps: %d", fps_info->is_high_fps);
    SENSOR_PRINT("high_fps_skip_num: %d", fps_info->high_fps_skip_num);

    return rtn;
}

/*==============================================================================
 * Description:
 * cfg otp setting
 * please modify this function acording your spec
 *============================================================================*/
static unsigned long ov8856s_access_val(SENSOR_HW_HANDLE handle,
                                        unsigned long param) {
    uint32_t ret = SENSOR_SUCCESS;
    SENSOR_VAL_T *param_ptr = (SENSOR_VAL_T *)param;

    if (!param_ptr) {
        return ret;
    }

    SENSOR_PRINT("sensor ov8856s: param_ptr->type=%x", param_ptr->type);

    switch (param_ptr->type) {
    case SENSOR_VAL_TYPE_INIT_OTP:
        ret = ov8856s_otp_init(handle);
        break;
    case SENSOR_VAL_TYPE_READ_OTP:
        ret = ov8856s_otp_read(handle, param_ptr);
        break;
    case SENSOR_VAL_TYPE_PARSE_OTP:
        ret = ov8856s_parse_otp(handle, param_ptr);
        break;
    case SENSOR_VAL_TYPE_SHUTTER:
        *((uint32_t *)param_ptr->pval) = ov8856s_read_shutter(handle);
        break;
    case SENSOR_VAL_TYPE_READ_OTP_GAIN:
        *((uint32_t *)param_ptr->pval) = ov8856s_read_gain(handle);
        break;
    case SENSOR_VAL_TYPE_GET_STATIC_INFO:
        ret = ov8856s_get_static_info(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_GET_FPS_INFO:
        ret = ov8856s_get_fps_info(handle, param_ptr->pval);
        break;
    default:
        break;
    }
    ret = SENSOR_SUCCESS;

    return ret;
}

/*==============================================================================
 * Description:
 * identify sensor id
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t ov8856s_identify(SENSOR_HW_HANDLE handle, uint32_t param) {
    uint16_t pid_value = 0x00;
    uint16_t ver_value = 0x00;
    uint32_t ret_value = SENSOR_FAIL;

    SENSOR_PRINT("mipi raw identify");

    pid_value = Sensor_ReadReg(ov8856s_PID_ADDR);

    if (ov8856s_PID_VALUE == pid_value) {
        ver_value = Sensor_ReadReg(ov8856s_VER_ADDR);
        SENSOR_PRINT("Identify: PID = %x, VER = %x", pid_value, ver_value);
        if (ov8856s_VER_VALUE == ver_value) {
            SENSOR_PRINT_HIGH("this is ov8856s sensor");

#ifdef FEATURE_OTP
            /*if read otp info failed or module id mismatched ,identify failed
             * ,return SENSOR_FAIL ,exit identify*/
            if (PNULL != s_ov8856s_raw_param_tab_ptr->identify_otp) {
                SENSOR_PRINT("identify module_id=0x%x",
                             s_ov8856s_raw_param_tab_ptr->param_id);
                // set default value
                memset(s_ov8856s_otp_info_ptr, 0x00, sizeof(struct otp_info_t));
                ret_value = s_ov8856s_raw_param_tab_ptr->identify_otp(
                    handle, s_ov8856s_otp_info_ptr);

                if (SENSOR_SUCCESS == ret_value) {
                    SENSOR_PRINT(
                        "identify otp sucess! module_id=0x%x, module_name=%s",
                        s_ov8856s_raw_param_tab_ptr->param_id, MODULE_NAME);
                } else {
                    SENSOR_PRINT("identify otp fail! exit identify");
                    return ret_value;
                }
            } else {
                SENSOR_PRINT("no identify_otp function!");
            }

#endif
            ov8856s_init_mode_fps_info(handle);
            ret_value = SENSOR_SUCCESS;

        } else {
            SENSOR_PRINT_HIGH("Identify this is %x%x sensor", pid_value,
                              ver_value);
        }
    } else {
        SENSOR_PRINT_HIGH("sensor identify fail, pid_value = %x", pid_value);
    }

    return ret_value;
}

/*==============================================================================
 * Description:
 * get resolution trim
 *
 *============================================================================*/
static unsigned long ov8856s_get_resolution_trim_tab(SENSOR_HW_HANDLE handle,
                                                     uint32_t param) {
    return (unsigned long)s_ov8856s_resolution_trim_tab;
}

/*==============================================================================
 * Description:
 * before snapshot
 * you can change this function if it's necessary
 *============================================================================*/
static uint32_t ov8856s_before_snapshot(SENSOR_HW_HANDLE handle,
                                        uint32_t param) {
    uint32_t cap_shutter = 0;
    uint32_t prv_shutter = 0;
    uint32_t gain = 0;
    uint32_t cap_gain = 0;
    uint32_t capture_mode = param & 0xffff;
    uint32_t preview_mode = (param >> 0x10) & 0xffff;

    uint32_t prv_linetime =
        s_ov8856s_resolution_trim_tab[preview_mode].line_time;
    uint32_t cap_linetime =
        s_ov8856s_resolution_trim_tab[capture_mode].line_time;

    s_current_default_frame_length =
        ov8856s_get_default_frame_length(handle, capture_mode);
    SENSOR_PRINT("capture_mode = %d", capture_mode);

    if (preview_mode == capture_mode) {
        cap_shutter = s_sensor_ev_info.preview_shutter;
        cap_gain = s_sensor_ev_info.preview_gain;
        goto snapshot_info;
    }

    prv_shutter = s_sensor_ev_info.preview_shutter; // ov8856s_read_shutter();
    gain = s_sensor_ev_info.preview_gain;           // ov8856s_read_gain();

    Sensor_SetMode(capture_mode);
    Sensor_SetMode_WaitDone();

    cap_shutter = prv_shutter * prv_linetime / cap_linetime; //* BINNING_FACTOR;
                                                             /*
                                                                     while (gain >= (2 * SENSOR_BASE_GAIN)) {
                                                                             if (cap_shutter * 2 > s_current_default_frame_length)
                                                                                     break;
                                                                             cap_shutter = cap_shutter * 2;
                                                                             gain = gain / 2;
                                                                     }
                                                             */
    cap_shutter = ov8856s_write_exposure_dummy(handle, cap_shutter, 0, 0);
    cap_gain = gain;
    ov8856s_write_gain(handle, cap_gain);
    SENSOR_PRINT("preview_shutter = 0x%x, preview_gain = 0x%x",
                 s_sensor_ev_info.preview_shutter,
                 s_sensor_ev_info.preview_gain);

    SENSOR_PRINT("capture_shutter = 0x%x, capture_gain = 0x%x", cap_shutter,
                 cap_gain);
snapshot_info:

    Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, cap_shutter);

    return SENSOR_SUCCESS;
}

/*==============================================================================
 * Description:
 * get the shutter from isp
 * please don't change this function unless it's necessary
 *============================================================================*/
static unsigned long ov8856s_write_exposure(SENSOR_HW_HANDLE handle,
                                            unsigned long param) {
    uint32_t ret_value = SENSOR_SUCCESS;
    uint16_t exposure_line = 0x00;
    uint16_t dummy_line = 0x00;
    uint16_t size_index = 0x00;
    uint16_t frame_interval = 0x00;
    struct sensor_ex_exposure *ex = (struct sensor_ex_exposure *)param;

    if (!param) {
        SENSOR_PRINT_ERR("param is NULL !!!");
        return ret_value;
    }

    exposure_line = ex->exposure;
    dummy_line = ex->dummy;
    size_index = ex->size_index;
    frame_interval =
        (uint16_t)(((exposure_line + dummy_line) *
                    (s_ov8856s_resolution_trim_tab[size_index].line_time)) /
                   1000000);
    SENSOR_PRINT(
        "mode = %d, exposure_line = %d, dummy_line= %d, frame_interval= %d ms",
        size_index, exposure_line, dummy_line, frame_interval);
    s_current_default_frame_length =
        ov8856s_get_default_frame_length(handle, size_index);

    ret_value = ov8856s_write_exposure_dummy(handle, exposure_line, dummy_line,
                                             size_index);

    return ret_value;
}

/*==============================================================================
 * Description:
 * get the parameter from isp to real gain
 * you mustn't change the funcion !
 *============================================================================*/
static uint32_t isp_to_real_gain(SENSOR_HW_HANDLE handle, uint32_t param) {
    uint32_t real_gain = 0;

    real_gain = param;

    return real_gain;
}

/*==============================================================================
 * Description:
 * write gain value to sensor
 * you can change this function if it's necessary
 *============================================================================*/
static uint32_t ov8856s_write_gain_value(SENSOR_HW_HANDLE handle,
                                         unsigned long param) {
    uint32_t ret_value = SENSOR_SUCCESS;
    float real_gain = 0;

    // real_gain = isp_to_real_gain(handle,param);
    param = param < SENSOR_BASE_GAIN ? SENSOR_BASE_GAIN : param;

    real_gain = (float)1.0f * param * SENSOR_BASE_GAIN / ISP_BASE_GAIN;

    SENSOR_PRINT("real_gain = %f", real_gain);

    s_sensor_ev_info.preview_gain = real_gain;
    ov8856s_write_gain(handle, real_gain);

    return ret_value;
}

static struct sensor_reg_tag ov8856s_shutter_reg[] = {
    {0x3502, 0}, {0x3501, 0}, {0x3500, 0},
};

static struct sensor_i2c_reg_tab ov8856s_shutter_tab = {
    .settings = ov8856s_shutter_reg, .size = ARRAY_SIZE(ov8856s_shutter_reg),
};

static struct sensor_reg_tag ov8856s_again_reg[] = {
    {0x3208, 0x01}, {0x3508, 0x00}, {0x3509, 0x00},
};

static struct sensor_i2c_reg_tab ov8856s_again_tab = {
    .settings = ov8856s_again_reg, .size = ARRAY_SIZE(ov8856s_again_reg),
};

static struct sensor_reg_tag ov8856s_dgain_reg[] = {
    {0x5019, 0}, {0x501a, 0}, {0x501b, 0}, {0x501c, 0},    {0x501d, 0},
    {0x501e, 0}, {0x501f, 0}, {0x5020, 0}, {0x3208, 0x11}, {0x3208, 0xA1},
};

struct sensor_i2c_reg_tab ov8856s_dgain_tab = {
    .settings = ov8856s_dgain_reg, .size = ARRAY_SIZE(ov8856s_dgain_reg),
};

static struct sensor_reg_tag ov8856s_frame_length_reg[] = {
    {0x380e, 0}, {0x380f, 0},
};

static struct sensor_i2c_reg_tab ov8856s_frame_length_tab = {
    .settings = ov8856s_frame_length_reg,
    .size = ARRAY_SIZE(ov8856s_frame_length_reg),
};

static struct sensor_aec_i2c_tag ov8856s_aec_info = {
    .slave_addr = (I2C_SLAVE_ADDR >> 1),
    .addr_bits_type = SENSOR_I2C_REG_16BIT,
    .data_bits_type = SENSOR_I2C_VAL_8BIT,
    .shutter = &ov8856s_shutter_tab,
    .again = &ov8856s_again_tab,
    .dgain = &ov8856s_dgain_tab,
    .frame_length = &ov8856s_frame_length_tab,
};

static uint16_t ov8856s_calc_exposure(SENSOR_HW_HANDLE handle, uint32_t shutter,
                                      uint32_t dummy_line, uint16_t mode,
                                      struct sensor_aec_i2c_tag *aec_info) {
    uint32_t dest_fr_len = 0;
    uint32_t cur_fr_len = 0;
    uint32_t fr_len = s_current_default_frame_length;
    int32_t offset = 0;
    uint16_t value = 0x00;
    float fps = 0.0;
    float line_time = 0.0;

    dummy_line = dummy_line > FRAME_OFFSET ? dummy_line : FRAME_OFFSET;
    dest_fr_len =
        ((shutter + dummy_line) > fr_len) ? (shutter + dummy_line) : fr_len;

    // cur_fr_len = ov8856s_read_frame_length(handle);

    if (shutter < SENSOR_MIN_SHUTTER)
        shutter = SENSOR_MIN_SHUTTER;

    line_time = s_ov8856s_resolution_trim_tab[mode].line_time;
    fps = 1000000.0 / (dest_fr_len * line_time);

    SENSOR_PRINT("sync fps = %f", fps);
    aec_info->frame_length->settings[0].reg_value = (dest_fr_len >> 8) & 0xff;
    aec_info->frame_length->settings[1].reg_value = dest_fr_len & 0xff;
    value = (shutter << 0x04) & 0xff;
    aec_info->shutter->settings[0].reg_value = value;
    value = (shutter >> 0x04) & 0xff;
    aec_info->shutter->settings[1].reg_value = value;
    value = (shutter >> 0x0c) & 0x0f;
    aec_info->shutter->settings[2].reg_value = value;
    return shutter;
}

static void ov8856s_calc_gain(float gain, struct sensor_aec_i2c_tag *aec_info) {
    float gain_a = gain;
    float gain_d = 0x400;

    if (SENSOR_MAX_GAIN < (uint16_t)gain_a) {

        gain_a = SENSOR_MAX_GAIN;
        gain_d = gain * 0x400 / gain_a;
        if ((uint16_t)gain_d > 0x2 * 0x400 - 1)
            gain_d = 0x2 * 0x400 - 1;
    }
    // group 1:all other registers( gain)
    // Sensor_WriteReg(0x3208, 0x01);

    aec_info->again->settings[1].reg_value = ((uint16_t)gain_a >> 8) & 0x07;
    aec_info->again->settings[2].reg_value = (uint16_t)gain_a & 0xff;

    aec_info->dgain->settings[0].reg_value = ((uint16_t)gain_d >> 8) & 0x07;
    aec_info->dgain->settings[1].reg_value = (uint16_t)gain_d & 0xff;
    aec_info->dgain->settings[2].reg_value = ((uint16_t)gain_d >> 8) & 0x07;
    aec_info->dgain->settings[3].reg_value = (uint16_t)gain_d & 0xff;
    aec_info->dgain->settings[4].reg_value = ((uint16_t)gain_d >> 8) & 0x07;
    aec_info->dgain->settings[5].reg_value = (uint16_t)gain_d & 0xff;
    aec_info->dgain->settings[6].reg_value = ((uint16_t)gain_d >> 8) & 0x07;
    aec_info->dgain->settings[7].reg_value = (uint16_t)gain_d & 0xff;
}

static unsigned long ov8856s_read_aec_info(SENSOR_HW_HANDLE handle,
                                           unsigned long param) {
    unsigned long ret_value = SENSOR_SUCCESS;
    struct sensor_aec_reg_info *info = (struct sensor_aec_reg_info *)param;
    uint16_t exposure_line = 0x00;
    uint16_t dummy_line = 0x00;
    uint16_t mode = 0x00;
    float real_gain = 0;
    uint32_t gain = 0;
    uint16_t frame_interval = 0x00;

    info->aec_i2c_info_out = &ov8856s_aec_info;

    exposure_line = info->exp.exposure;
    dummy_line = info->exp.dummy;
    mode = info->exp.size_index;

    frame_interval =
        (uint16_t)(((exposure_line + dummy_line) *
                    (s_ov8856s_resolution_trim_tab[mode].line_time)) /
                   1000000);
    SENSOR_PRINT(
        "mode = %d, exposure_line = %d, dummy_line= %d, frame_interval= %d ms",
        mode, exposure_line, dummy_line, frame_interval);
    s_current_default_frame_length =
        ov8856s_get_default_frame_length(handle, mode);

    s_sensor_ev_info.preview_shutter = ov8856s_calc_exposure(
        handle, exposure_line, dummy_line, mode, &ov8856s_aec_info);

    gain = info->gain < SENSOR_BASE_GAIN ? SENSOR_BASE_GAIN : info->gain;
    real_gain = (float)info->gain * SENSOR_BASE_GAIN / ISP_BASE_GAIN * 1.0;
    ov8856s_calc_gain(real_gain, &ov8856s_aec_info);
    return ret_value;
}
#ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
/*==============================================================================
 * Description:
 * write parameter to vcm
 * please add your VCM function to this function
 *============================================================================*/
static uint32_t ov8856s_write_af(SENSOR_HW_HANDLE handle, uint32_t param) {
    return zzz_write_af(param);
}
#endif
unsigned long ov8856s_SetSlave_FrameSync(SENSOR_HW_HANDLE handle,
                                         unsigned long param) {
    Sensor_WriteReg(0x3000, 0x00); // bit 5 0 input 1 output 0x3003?
    Sensor_WriteReg(0x3823, 0x58);
    Sensor_WriteReg(0x3824, 0x00);
    Sensor_WriteReg(0x3825, 0x20);
    Sensor_WriteReg(0x3826, 0x00);
    Sensor_WriteReg(0x3827, 0x07);

    return 0;
}

/*==============================================================================
 * Description:
 * mipi stream on
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t ov8856s_stream_on(SENSOR_HW_HANDLE handle, uint32_t param) {
    SENSOR_PRINT("E");
    ov8856s_SetSlave_FrameSync(handle, param);

    Sensor_WriteReg(0x0100, 0x01);
    /*delay*/
    usleep(10 * 1000);

    return 0;
}

/*==============================================================================
 * Description:
 * mipi stream off
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t ov8856s_stream_off(SENSOR_HW_HANDLE handle, uint32_t param) {
    SENSOR_PRINT("E");

    Sensor_WriteReg(0x0100, 0x00);
    /*delay*/
    usleep(50 * 1000);

    return 0;
}

/*==============================================================================
 * Description:
 * all ioctl functoins
 * you can add functions reference SENSOR_IOCTL_FUNC_TAB_T from sensor_drv_u.h
 *
 * add ioctl functions like this:
 * .power = ov8856s_power_on,
 *============================================================================*/
static SENSOR_IOCTL_FUNC_TAB_T s_ov8856s_ioctl_func_tab = {
    .power = ov8856s_power_on,
    .identify = ov8856s_identify,
    .get_trim = ov8856s_get_resolution_trim_tab,
    .before_snapshort = ov8856s_before_snapshot,
    .ex_write_exp = ov8856s_write_exposure,
    .write_gain_value = ov8856s_write_gain_value,
#ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
    .af_enable = ov8856s_write_af,
#endif
    .stream_on = ov8856s_stream_on,
    .stream_off = ov8856s_stream_off,
    .cfg_otp = ov8856s_access_val,

    //.group_hold_on = ov8856s_group_hold_on,
    //.group_hold_of = ov8856s_group_hold_off,
    .read_aec_info = ov8856s_read_aec_info,
};
