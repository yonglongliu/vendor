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

#include "parameters/sensor_gc2375a_raw_param_main.c"

//#define FEATURE_OTP

//#define NOT_SUPPORT_VIDEO
#define SYNC_MAST_CAMAER_FIX_20FPS

#define VENDOR_NUM 1
#define SENSOR_NAME "gc2375a_mipi_raw"
#define I2C_SLAVE_ADDR 0x6e

#define gc2375a_PID_ADDR 0xF0
#define gc2375a_PID_VALUE 0x23
#define gc2375a_VER_ADDR 0xF1
#define gc2375a_VER_VALUE 0x75

/* sensor parameters begin */
/* effective sensor output image size */
#define VIDEO_WIDTH 1600
#define VIDEO_HEIGHT 1200
#define PREVIEW_WIDTH 1600
#define PREVIEW_HEIGHT 1200
#define SNAPSHOT_WIDTH 1600
#define SNAPSHOT_HEIGHT 1200

/*Raw Trim parameters*/
#define VIDEO_TRIM_X 0
#define VIDEO_TRIM_Y 0
#define VIDEO_TRIM_W VIDEO_WIDTH
#define VIDEO_TRIM_H VIDEO_HEIGHT
#define PREVIEW_TRIM_X 0
#define PREVIEW_TRIM_Y 0
#define PREVIEW_TRIM_W PREVIEW_WIDTH
#define PREVIEW_TRIM_H PREVIEW_HEIGHT
#define SNAPSHOT_TRIM_X 0
#define SNAPSHOT_TRIM_Y 0
#define SNAPSHOT_TRIM_W SNAPSHOT_WIDTH
#define SNAPSHOT_TRIM_H SNAPSHOT_HEIGHT

/*Mipi output*/
#define LANE_NUM 1
#define RAW_BITS 10

#define HIGHT_DUMMY 1224

#ifdef SYNC_MAST_CAMAER_FIX_20FPS
#define VIDEO_MIPI_PER_LANE_BPS 576    /* 2*Mipi clk */
#define PREVIEW_MIPI_PER_LANE_BPS 576  /* 2*Mipi clk */
#define SNAPSHOT_MIPI_PER_LANE_BPS 576 /* 2*Mipi clk */

/*line time unit: 0.1ns*/
#define VIDEO_LINE_TIME 31167
#define PREVIEW_LINE_TIME 31167
#define SNAPSHOT_LINE_TIME 31167

/* frame length*/
#define VIDEO_FRAME_LENGTH 1604
#define PREVIEW_FRAME_LENGTH 1604 // 1240
//#define SNAPSHOT_FRAME_LENGTH		1204 //25.8fps
#define SNAPSHOT_FRAME_LENGTH 1604 // 20fps

#define INIT_SHUTTER 0x3C3

#else
#define VIDEO_MIPI_PER_LANE_BPS 624   /* 2*Mipi clk */
#define PREVIEW_MIPI_PER_LANE_BPS 624 /* 2*Mipi clk */
#define SNAPSHOT_MIPI_PER_LANE_BPS 624 /* 2*Mipi clk */

/*line time unit: 0.1ns*/
#define VIDEO_LINE_TIME 26600
#define PREVIEW_LINE_TIME 26600
#define SNAPSHOT_LINE_TIME 26600

/* frame length*/
#define VIDEO_FRAME_LENGTH 1240
#define PREVIEW_FRAME_LENGTH 1240 // 1240
//#define SNAPSHOT_FRAME_LENGTH		1240 //25.8fps
#define SNAPSHOT_FRAME_LENGTH 1240 // 20fps
#define INIT_SHUTTER 0x465
#endif

/* frame length*/
#define VIDEO_FRAME_LENGTH 1240
#define PREVIEW_FRAME_LENGTH 1255 // 1240
//#define SNAPSHOT_FRAME_LENGTH		1240 //25.8fps
#define SNAPSHOT_FRAME_LENGTH 1604 // 20fps

/* please ref your spec */
#define FRAME_OFFSET 0
#define SENSOR_MAX_GAIN 0xffff
#define SENSOR_BASE_GAIN 0x40
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

#ifdef IMAGE_NORMAL_MIRROR
#define MIRROR 0xd4
#define STARTY 0x01
#define STARTX 0x01
#define BLK_Select1_H 0x00
#define BLK_Select1_L 0x3c
#define BLK_Select2_H 0x00
#define BLK_Select2_L 0x03
#endif

#ifdef IMAGE_H_MIRROR
#define MIRROR 0xd5
#define STARTY 0x01
#define STARTX 0x00
#define BLK_Select1_H 0x00
#define BLK_Select1_L 0x3c
#define BLK_Select2_H 0x00
#define BLK_Select2_L 0x03
#endif

#ifdef IMAGE_V_MIRROR
#define MIRROR 0xd6
#define STARTY 0x02
#define STARTX 0x01
#define BLK_Select1_H 0x3c
#define BLK_Select1_L 0x00
#define BLK_Select2_H 0xc0
#define BLK_Select2_L 0x00
#endif

#ifdef IMAGE_HV_MIRROR
#define MIRROR 0xd7
#define STARTY 0x02
#define STARTX 0x00
#define BLK_Select1_H 0x3c
#define BLK_Select1_L 0x00
#define BLK_Select2_H 0xc0
#define BLK_Select2_L 0x00
#endif

/*==============================================================================
 * Description:
 * register setting
 *============================================================================*/

static const SENSOR_REG_T gc2375a_init_setting[] = {
    /*System*/
    {0xfe, 0x00},
    {0xfe, 0x00},
    {0xfe, 0x00},
    {0xf7, 0x01},
    {0xf8, 0x0b}, //{0xf8, 0x0c},
    {0xf9, 0x42},
    {0xfa, 0x88},
    {0xfc, 0x8e},
    {0xfe, 0x00},
    {0x88, 0x03},

    /*Analog*/
    {0x03, (INIT_SHUTTER >> 0x08) & 0xff},
    {0x04, (INIT_SHUTTER)&0xff},
    {0x05, 0x02},
    {0x06, 0xac}, //{0x06, 0x5a},

    {0x07, ((SNAPSHOT_FRAME_LENGTH - HIGHT_DUMMY) >> 0x08) & 0xff},
    {0x08, (SNAPSHOT_FRAME_LENGTH - HIGHT_DUMMY) & 0xff},

    {0x09, 0x00},
    {0x0a, 0x08},
    {0x0b, 0x00},
    {0x0c, 0x18},
    {0x0d, 0x04},
    {0x0e, 0xb8},
    {0x0f, 0x06},
    {0x10, 0x48},
    {0x17, MIRROR},
    {0x1c, 0x10},
    {0x1d, 0x13},
    {0x20, 0x0b},
    {0x21, 0x6d},
    {0x22, 0x0c},
    {0x25, 0xc1},
    {0x26, 0x0e},
    {0x27, 0x22},
    {0x29, 0x5f},
    {0x2b, 0x88},
    {0x2f, 0x12},
    {0x38, 0x86},
    {0x3d, 0x00},
    {0xcd, 0xa3},
    {0xce, 0x57},
    {0xd0, 0x09},
    {0xd1, 0xca},
    {0xd2, 0x34},
    {0xd3, 0xbb},
    {0xd8, 0x60},
    {0xe0, 0x08},
    {0xe1, 0x1f},
    {0xe4, 0xf8},
    {0xe5, 0x0c},
    {0xe6, 0x10},
    {0xe7, 0xcc},
    {0xe8, 0x02},
    {0xe9, 0x01},
    {0xea, 0x02},
    {0xeb, 0x01},

    /*Crop*/
    {0x90, 0x01},
    {0x92, STARTY},
    {0x94, STARTX},
    {0x95, 0x04},
    {0x96, 0xb0},
    {0x97, 0x06},
    {0x98, 0x40},

    /*BLK*/
    {0x18, 0x02},
    {0x1a, 0x18},
    {0x28, 0x00},
    {0x3f, 0x40},
    {0x40, 0x26},
    {0x41, 0x00},
    {0x43, 0x03},
    {0x4a, 0x00},
    {0x4e, BLK_Select1_H},
    {0x4f, BLK_Select1_L},
    {0x66, BLK_Select2_H},
    {0x67, BLK_Select2_L},

    /*Dark sun*/
    {0x68, 0x00},

    /*Gain*/
    {0xb0, 0x58},
    {0xb1, 0x01},
    {0xb2, 0x00},
    {0xb6, 0x00},

    /*MIPI*/
    {0xef, 0x00},
    {0xfe, 0x03},
    {0x01, 0x03},
    {0x02, 0x33},
    {0x03, 0x90},
    {0x04, 0x04},
    {0x05, 0x00},
    {0x06, 0x80},
    {0x11, 0x2b},
    {0x12, 0xd0},
    {0x13, 0x07},
    {0x15, 0x00},
    {0x21, 0x08},
    {0x22, 0x05},
    {0x23, 0x13},
    {0x24, 0x02},
    {0x25, 0x13},
    {0x26, 0x08},
    {0x29, 0x06},
    {0x2a, 0x08},
    {0x2b, 0x08},
    {0xfe, 0x00},
};

static const SENSOR_REG_T gc2375a_preview_setting[] = {
    {0xfe, 0x00},
};

static const SENSOR_REG_T gc2375a_snapshot_setting[] = {
    {0xfe, 0x00},

};

static const SENSOR_REG_T gc2375a_video_setting[] = {

};

static struct sensor_res_tab_info s_gc2375a_resolution_tab_raw[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .reg_tab =
         {{ADDR_AND_LEN_OF_ARRAY(gc2375a_init_setting), PNULL, 0, .width = 0,
           .height = 0, .xclk_to_sensor = EX_MCLK,
           .image_format = SENSOR_IMAGE_FORMAT_RAW},
#ifndef NOT_SUPPORT_VIDEO
          {ADDR_AND_LEN_OF_ARRAY(gc2375a_video_setting), PNULL, 0,
           .width = VIDEO_WIDTH, .height = VIDEO_HEIGHT,
           .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW},

          {ADDR_AND_LEN_OF_ARRAY(gc2375a_preview_setting), PNULL, 0,
           .width = PREVIEW_WIDTH, .height = PREVIEW_HEIGHT,
           .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW},
#endif
          {ADDR_AND_LEN_OF_ARRAY(gc2375a_snapshot_setting), PNULL, 0,
           .width = SNAPSHOT_WIDTH, .height = SNAPSHOT_HEIGHT,
           .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW}}}

    /*If there are multiple modules,please add here*/
};

static SENSOR_TRIM_T s_gc2375a_resolution_trim_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .trim_info =
         {
             {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
#ifndef NOT_SUPPORT_VIDEO
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
#endif
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

static SENSOR_REG_T gc2375a_shutter_reg[] = {
    {0xfe, 0x00},
    {0x03, (INIT_SHUTTER >> 0x08) & 0xff},
    {0x04, (INIT_SHUTTER)&0xff},
};

static struct sensor_i2c_reg_tab gc2375a_shutter_tab = {
    .settings = gc2375a_shutter_reg, .size = ARRAY_SIZE(gc2375a_shutter_reg),
};

static SENSOR_REG_T gc2375a_again_reg[] = {
    {0xfe, 0x00}, {0x20, 0x00}, {0x22, 0}, {0x26, 0},
    {0xB6, 0},    {0xb1, 0},    {0xb2, 0},
};

static struct sensor_i2c_reg_tab gc2375a_again_tab = {
    .settings = gc2375a_again_reg, .size = ARRAY_SIZE(gc2375a_again_reg),
};

static SENSOR_REG_T gc2375a_dgain_reg[] = {

};

static struct sensor_i2c_reg_tab gc2375a_dgain_tab = {
    .settings = gc2375a_dgain_reg, .size = ARRAY_SIZE(gc2375a_dgain_reg),
};

static SENSOR_REG_T gc2375a_frame_length_reg[] = {
    /*
    {0xfe, 0x00},
    {0x07, ((SNAPSHOT_FRAME_LENGTH-HIGHT_DUMMY)>>0x08)&0xff},
    {0x08, ((SNAPSHOT_FRAME_LENGTH-HIGHT_DUMMY)&0xff},
    */
};

static struct sensor_i2c_reg_tab gc2375a_frame_length_tab = {
    .settings = gc2375a_frame_length_reg,
    .size = ARRAY_SIZE(gc2375a_frame_length_reg),
};

static struct sensor_aec_i2c_tag gc2375a_aec_info = {
    .slave_addr = (I2C_SLAVE_ADDR >> 1),
    .addr_bits_type = SENSOR_I2C_REG_8BIT,
    .data_bits_type = SENSOR_I2C_VAL_8BIT,
    .shutter = &gc2375a_shutter_tab,
    .again = &gc2375a_again_tab,
    .dgain = &gc2375a_dgain_tab,
    .frame_length = &gc2375a_frame_length_tab,
};

static SENSOR_STATIC_INFO_T s_gc2375a_static_info[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .static_info = {.f_num = 200,
                     .focal_length = 354,
                     .max_fps = 0,
                     .max_adgain = 15 * 2,
                     .ois_supported = 0,
                     .pdaf_supported = 0,
                     .exp_valid_frame_num = 1,
                     .clamp_level = 64,
                     .adgain_valid_frame_num = 1,
                     .fov_info = {{4.614f, 3.444f}, 4.222f}}}
    /*If there are multiple modules,please add here*/
};

static SENSOR_MODE_FPS_INFO_T s_gc2375a_mode_fps_info[VENDOR_NUM] = {
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

static struct sensor_module_info s_gc2375a_module_info_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .module_info = {.major_i2c_addr = I2C_SLAVE_ADDR >> 1,
                     .minor_i2c_addr = I2C_SLAVE_ADDR >> 1,

                     .reg_addr_value_bits = SENSOR_I2C_REG_8BIT |
                                            SENSOR_I2C_VAL_8BIT |
                                            SENSOR_I2C_FREQ_100,

                     .avdd_val = SENSOR_AVDD_2800MV,
                     .iovdd_val = SENSOR_AVDD_1800MV,
                     .dvdd_val = SENSOR_AVDD_1800MV,

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
                     .vertical_view_angle = 60}}

    /*If there are multiple modules,please add here*/
};

static struct sensor_ic_ops s_gc2375a_ops_tab;
struct sensor_raw_info *s_gc2375a_mipi_raw_info_ptr = &s_gc2375a_mipi_raw_info;

SENSOR_INFO_T g_gc2375a_mipi_raw_info = {
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
    .power_down_level = SENSOR_HIGH_LEVEL_PWDN,
    .identify_count = 1,
    .identify_code = {{.reg_addr = gc2375a_PID_ADDR,
                       .reg_value = gc2375a_PID_VALUE},
                      {.reg_addr = gc2375a_VER_ADDR,
                       .reg_value = gc2375a_VER_VALUE}},

    .source_width_max = SNAPSHOT_WIDTH,
    .source_height_max = SNAPSHOT_HEIGHT,
    .name = (cmr_s8 *)SENSOR_NAME,
    .image_format = SENSOR_IMAGE_FORMAT_RAW,

    .module_info_tab = s_gc2375a_module_info_tab,
    .module_info_tab_size = ARRAY_SIZE(s_gc2375a_module_info_tab),

    .resolution_tab_info_ptr = s_gc2375a_resolution_tab_raw,
    .sns_ops = &s_gc2375a_ops_tab,
    .raw_info_ptr = &s_gc2375a_mipi_raw_info_ptr,

    .video_tab_info_ptr = NULL,
    .sensor_version_info = (cmr_s8 *)"gc2375a_v1",
};
