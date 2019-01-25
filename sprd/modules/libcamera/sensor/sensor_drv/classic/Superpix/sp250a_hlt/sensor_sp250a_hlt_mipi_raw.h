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
 /*History
 *Date                  Modification                                 Reason
 *
 */

#ifndef _SENSOR_SP250a_HLT_MIPI_RAW_H_
#define _SENSOR_SP250a_HLT_MIPI_RAW_H_


#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"

#include "parameters/sensor_sp250a_hlt_raw_param_main.c"

//#define FEATURE_OTP
#define VENDOR_NUM 1
#define SENSOR_NAME				"sp250a_hlt_mipi_raw"
#define I2C_SLAVE_ADDR			0x78    /* 8bit slave address*/
#define BINNING_FACTOR 1
#define sp250a_hlt_PID_ADDR				0x02
#define sp250a_hlt_PID_VALUE			0x25
#define sp250a_hlt_VER_ADDR				0x03
#define sp250a_hlt_VER_VALUE			0x0a

/* sensor parameters begin */

/* effective sensor output image size */
#define PREVIEW_WIDTH			1600
#define PREVIEW_HEIGHT			1200
#define SNAPSHOT_WIDTH			1600 
#define SNAPSHOT_HEIGHT			1200

/*Raw Trim parameters*/
#define PREVIEW_TRIM_X			0
#define PREVIEW_TRIM_Y			0
#define PREVIEW_TRIM_W			PREVIEW_WIDTH
#define PREVIEW_TRIM_H			PREVIEW_HEIGHT
#define SNAPSHOT_TRIM_X			0
#define SNAPSHOT_TRIM_Y			0
#define SNAPSHOT_TRIM_W			SNAPSHOT_WIDTH
#define SNAPSHOT_TRIM_H			SNAPSHOT_HEIGHT

/*Mipi output*/
#define LANE_NUM			1
#define RAW_BITS			10

#define PREVIEW_MIPI_PER_LANE_BPS	  840  /* 2*Mipi clk */
#define SNAPSHOT_MIPI_PER_LANE_BPS	  840  /* 2*Mipi clk */
                                 
/*line time unit: 1ns*/
#define PREVIEW_LINE_TIME		  26900
#define SNAPSHOT_LINE_TIME		  26900
                                 
/* frame length*/                
#define PREVIEW_FRAME_LENGTH		1229
#define SNAPSHOT_FRAME_LENGTH		1229

/* please ref your spec */
#define FRAME_OFFSET			0
#define SENSOR_MAX_GAIN			0xa0 //max  gain ; notice: if param's max gain is changed ,please you change sensor_max_gain !
#define SENSOR_BASE_GAIN		0x10 
#define SENSOR_MIN_SHUTTER		1

/* please ref your spec
 * 1 : average binning
 * 2 : sum-average binning
 * 4 : sum binning
 */

/* please ref spec
 * 1: sensor auto caculate
 * 0: driver caculate
 */
#define SUPPORT_AUTO_FRAME_LENGTH	1
/* sensor parameters end */

/* isp parameters, please don't change it*/

#define ISP_BASE_GAIN 0x80


/* please don't change it */
#define EX_MCLK				24
#define IMAGE_NORMAL_MIRROR

/*==============================================================================
 * Description:
 * register setting
 *============================================================================*/

static const SENSOR_REG_T sp250a_hlt_init_setting[] = {
{0xfd,0x01},
//{0x03,0x01}, //1base 
//{0x04,0x73}, 
{0x24,0xff},
{0x01,0x01},
{0x11,0x30}, //rst_num1
{0x33,0x50}, //rst_num2  
{0x1c,0x0c}, //[0]double shutter disable
{0x1e,0x80}, //rcn_dds_8lsb
{0x29,0x80}, //scnt_dds_8lsb
{0x2a,0xda}, //adc range 835mv,rgcol 1.5v,rgcnt 0.9v   //ea        
{0x2c,0x60}, //high 8bit, pldo 2.57v
{0x21,0x26}, //pcp rst 3.3v, pcp tx 3.9v
{0x25,0x13}, //[4]bl_en, ipix 1.336uA
{0x27,0x00}, //two dds mode enable  
{0x55,0x10},
{0x66,0x36},
{0x68,0x28},
{0x72,0x50}, 
{0x58,0x2a},  
{0x75,0x60},
{0x76,0x05},    
{0x51,0x30}, //pd reset restg
{0x52,0x2a}, //pd reset tg

{0x3f,0x03},
{0x27,0x00},
{0x9d,0x96},
            
{0xd0,0x03},
{0xd1,0x01},  
{0xd2,0xd0},
{0xd3,0x02},            
{0xd4,0x40},  

{0xfb,0x7b},   
{0xf0,0x00},  
{0xf1,0x00},
{0xf2,0x00}, 
{0xf3,0x00},
        
{0xa1,0x04},
//{0xac,0x01},
{0xb1,0x01},



{0xfd,0x02},
{0x1d,0x01},
{0x8a,0x0f},

{0xfd,0x01},
};

static const SENSOR_REG_T sp250a_hlt_preview_setting[] = {

};
  
static const SENSOR_REG_T sp250a_hlt_snapshot_setting[] = {
};


static struct sensor_res_tab_info s_sp250a_hlt_resolution_tab_raw[VENDOR_NUM] = {
	{
      .module_id = MODULE_SUNNY,
      .reg_tab = {
        {ADDR_AND_LEN_OF_ARRAY(sp250a_hlt_init_setting), PNULL, 0,
        .width = 0, .height = 0,
        .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW},
		
        {ADDR_AND_LEN_OF_ARRAY(sp250a_hlt_preview_setting), PNULL, 0,
        .width = PREVIEW_WIDTH, .height = PREVIEW_HEIGHT,
        .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW},

#if 0      
    {ADDR_AND_LEN_OF_ARRAY(sp250a_hlt_com_mipi_raw2), 0, 0, 24,
     SENSOR_IMAGE_FORMAT_RAW},
//{ADDR_AND_LEN_OF_ARRAY(sp250a_hlt_800X600_mipi_raw), 800, 600, 24,
// SENSOR_IMAGE_FORMAT_RAW},
#if 0//def CONFIG_CAMERA_DUAL_SYNC
    {ADDR_AND_LEN_OF_ARRAY(sp250a_hlt_1600X1200_altek_mipi_raw), 1600, 1200, 24,
     SENSOR_IMAGE_FORMAT_RAW},
#else
    {ADDR_AND_LEN_OF_ARRAY(sp250a_hlt_1600X1200_mipi_raw2), 1600, 1200, 24,
     SENSOR_IMAGE_FORMAT_RAW},
#endif
    {PNULL, 0, 0, 0, 0, 0},
    //{PNULL, 0, 800, 600, 24, SENSOR_IMAGE_FORMAT_RAW},
    //{ADDR_AND_LEN_OF_ARRAY(sp250a_hlt_640x480_mipi_raw), 640, 480, 24,
    // SENSOR_IMAGE_FORMAT_RAW},
    //{ADDR_AND_LEN_OF_ARRAY(sp250a_hlt_1600x1200_mipi_raw), 1600, 1200, 24,
    // SENSOR_IMAGE_FORMAT_RAW},

    {PNULL, 0, 0, 0, 0, 0},
    {PNULL, 0, 0, 0, 0, 0},
    {PNULL, 0, 0, 0, 0, 0},
    {PNULL, 0, 0, 0, 0, 0},
    {PNULL, 0, 0, 0, 0, 0},
    {PNULL, 0, 0, 0, 0, 0}}
#endif
  }}
/*If there are multiple modules,please add here*/
};

static SENSOR_TRIM_T s_sp250a_hlt_resolution_trim_tab[VENDOR_NUM] = {
	{
     .module_id = MODULE_SUNNY,
     .trim_info = {
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
    //{0, 0, 800, 600, 51800, 330, 644, {0, 0, 800, 600}},
    {0, 0, 1600, 1200, 26900, 840, 1229, {0, 0, 1600, 1200}}, // 25757.6ns
#if 0    
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}}
#endif
	}}

    /*If there are multiple modules,please add here*/

};


static const SENSOR_REG_T
    s_sp250a_hlt_preview_size_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
    /*video mode 0: ?fps*/
    {{.reg_addr = 0xffff, .reg_value = 0xff}},
    /* video mode 1:?fps*/
    {{.reg_addr = 0xffff, .reg_value = 0xff}},
    /* video mode 2:?fps*/
    {{.reg_addr = 0xffff, .reg_value = 0xff}},
    /* video mode 3:?fps*/
    {{.reg_addr = 0xffff, .reg_value = 0xff}},
};


static SENSOR_STATIC_INFO_T s_sp250a_hlt_static_info[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .static_info = {
        .f_num = 200,
        .focal_length = 354,
        .max_fps = 30,
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


static SENSOR_MODE_FPS_INFO_T s_sp250a_hlt_mode_fps_info[VENDOR_NUM] = {
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

static const SENSOR_REG_T
    s_sp250a_hlt_capture_size_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
    /*video mode 0: ?fps*/
    {{.reg_addr = 0xffff, .reg_value = 0xff}},
    /* video mode 1:?fps*/
    {{.reg_addr = 0xffff, .reg_value = 0xff}},
    /* video mode 2:?fps*/
    {{.reg_addr = 0xffff, .reg_value = 0xff}},
    /* video mode 3:?fps*/
    {{.reg_addr = 0xffff, .reg_value = 0xff}},
};

static SENSOR_VIDEO_INFO_T s_sp250a_hlt_video_info[SENSOR_MODE_MAX] = {
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{30, 30, 270, 90}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
     (SENSOR_REG_T **)s_sp250a_hlt_preview_size_video_tab},
    {{{2, 5, 338, 1000}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
     (SENSOR_REG_T **)s_sp250a_hlt_capture_size_video_tab},
};


static struct sensor_module_info s_sp250a_hlt_module_info_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .module_info = {
         .major_i2c_addr = I2C_SLAVE_ADDR >> 1,
         .minor_i2c_addr = I2C_SLAVE_ADDR >> 1,

         .reg_addr_value_bits = SENSOR_I2C_REG_8BIT | SENSOR_I2C_VAL_8BIT |
                                SENSOR_I2C_FREQ_100,

         .avdd_val = SENSOR_AVDD_2800MV,
         .iovdd_val = SENSOR_AVDD_1800MV,
         .dvdd_val = SENSOR_AVDD_1800MV,

         .image_pattern = SENSOR_IMAGE_PATTERN_RAWRGB_R,

         .preview_skip_num = 3,
         .capture_skip_num = 1,
         .flash_capture_skip_num = 2,
         .mipi_cap_skip_num = 0,
         .preview_deci_num = 0,
         .video_preview_deci_num = 0,

         .threshold_eb = 0,
         .threshold_mode = 0,
         .threshold_start = 0,
         .threshold_end = 0,

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

static struct sensor_ic_ops s_sp250a_hlt_ops_tab;
struct sensor_raw_info *s_sp250a_hlt_mipi_raw_info_ptr = &s_sp250a_hlt_mipi_raw_info;


/*==============================================================================
 * Description:
 * sensor all info
 * please modify this variable acording your spec
 *============================================================================*/
SENSOR_INFO_T g_sp250a_hlt_mipi_raw_info = {
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
    .identify_code =
        {{ .reg_addr = sp250a_hlt_PID_ADDR, .reg_value = sp250a_hlt_PID_VALUE},
         { .reg_addr = sp250a_hlt_VER_ADDR, .reg_value = sp250a_hlt_VER_VALUE}},

    .source_width_max = SNAPSHOT_WIDTH,
    .source_height_max = SNAPSHOT_HEIGHT,
    .name = (cmr_s8 *)SENSOR_NAME,
    .image_format = SENSOR_IMAGE_FORMAT_RAW,

    .module_info_tab = s_sp250a_hlt_module_info_tab,
    .module_info_tab_size = ARRAY_SIZE(s_sp250a_hlt_module_info_tab),

    .resolution_tab_info_ptr = s_sp250a_hlt_resolution_tab_raw,
    .sns_ops = &s_sp250a_hlt_ops_tab,
    .raw_info_ptr = &s_sp250a_hlt_mipi_raw_info_ptr,

    .video_tab_info_ptr = NULL,
    .sensor_version_info = (cmr_s8 *)"sp250a_4",
};

#endif
