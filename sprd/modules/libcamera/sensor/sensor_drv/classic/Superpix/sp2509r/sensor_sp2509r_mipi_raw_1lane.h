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

#include "parameters/sensor_sp2509r_raw_param_main.c"

//#define FEATURE_OTP

#define NOT_SUPPORT_VIDEO

#define VENDOR_NUM 1
#define SENSOR_NAME				"sp2509r_mipi_raw"
#define I2C_SLAVE_ADDR			0x7a //0x6c

#define sp2509r_PID_ADDR			0x02
#define sp2509r_PID_VALUE			0x25
#define sp2509r_VER_ADDR			0x03
#define sp2509r_VER_VALUE			0x09


/* sensor parameters begin */
/* effective sensor output image size */
#define VIDEO_WIDTH				1280
#define VIDEO_HEIGHT			720
#define PREVIEW_WIDTH			1600
#define PREVIEW_HEIGHT			1200
#define SNAPSHOT_WIDTH			1600 
#define SNAPSHOT_HEIGHT			1200

/*Raw Trim parameters*/
#define VIDEO_TRIM_X			0
#define VIDEO_TRIM_Y			0
#define VIDEO_TRIM_W			VIDEO_WIDTH
#define VIDEO_TRIM_H			VIDEO_HEIGHT
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

#define VIDEO_MIPI_PER_LANE_BPS	  	  720  /* 2*Mipi clk */
#define PREVIEW_MIPI_PER_LANE_BPS	  720  /* 2*Mipi clk */
#define SNAPSHOT_MIPI_PER_LANE_BPS	  720  /* 2*Mipi clk */

/*line time unit: 0.1ns*/
#define VIDEO_LINE_TIME		  	  26300
#define PREVIEW_LINE_TIME		  26300
#define SNAPSHOT_LINE_TIME		  26300

/* frame length*/
#define VIDEO_FRAME_LENGTH			1234
#define PREVIEW_FRAME_LENGTH		1234
#define SNAPSHOT_FRAME_LENGTH		1234

/* please ref your spec */
#define FRAME_OFFSET			0
#define SENSOR_MAX_GAIN			0xf8
#define SENSOR_BASE_GAIN		0x10
#define SENSOR_MIN_SHUTTER		4

/* please ref your spec
 * 1 : average binning
 * 2 : sum-average binning
 * 4 : sum binning
 */
#define BINNING_FACTOR			1

/* please ref spec
 * 1: sensor auto caculate
 * 0: driver caculate
 */
#define SUPPORT_AUTO_FRAME_LENGTH	0

/*delay 1 frame to write sensor gain*/
//#define GAIN_DELAY_1_FRAME

/* sensor parameters end */

/* isp parameters, please don't change it*/
#define ISP_BASE_GAIN			0x80

/* please don't change it */
#define EX_MCLK				24


/*==============================================================================
 * Description:
 * register setting
 *============================================================================*/

static const SENSOR_REG_T sp2509r_init_setting[] = {
{0xfd,0x00},
{0x2f,0x0c},       //2015.12.24
{0x34,0x00},
{0x35,0x21},
{0x30,0x1d},
{0x33,0x05},
{0xfd,0x01},
{0x44,0x00},
{0x2a,0x4c},
{0x2b,0x1e},      // ZH15.12.24
{0x2c,0x60},
{0x25,0x11},      //2015.12.24
{0x03,0x07},
{0x04,0x85},
{0x09,0x00},
{0x0a,0x02},      //2015.12.24
{0x06,0x0a},
{0x24,0xf0},
{0x01,0x01},
{0xfb,0x73},
{0xfd,0x01},
{0x16,0x04},
{0x1c,0x09},
{0x21,0x46},
{0x6c,0x00},
{0x6b,0x00},
{0x84,0x00},
{0x85,0x10},
{0x86,0x10},
{0x12,0x04},       // ZH15.12.24
{0x13,0x40},       //2015.12.24
{0x11,0x20},
{0x33,0x40},       //2015.12.24
{0xd0,0x03},
{0xd1,0x01},
{0xd2,0x00},       //2015.12.24
{0xd3,0x01},
{0xd4,0x20},       //2015.12.24
{0x51,0x14},       //2015.12.24
{0x52,0x10},       //2015.12.24
{0x55,0x30},
{0x58,0x10},
{0x71,0x10},
{0x6f,0x40},
{0x75,0x60},        //2015.12.24
{0x76,0x10},      // ZH15.12.24
{0x8a,0x22},
{0x8b,0x22},
{0x19,0x71},
{0x29,0x01},
{0xfd,0x01},
{0x9d,0xea},
{0xa0,0x29},//05 lxl151231
{0xa1,0x04},
{0xad,0x62},
{0xae,0x00},
{0xaf,0x85},//85 lxl151231
{0xb1,0x01},
//{0xac,0x01},//mipi_en
{0xfd,0x01},
{0xfc,0x10},
{0xfe,0x10},
{0xf9,0x00},
{0xfa,0x00},
{0xfd,0x01},
{0x8e,0x06},
{0x8f,0x40},
{0x90,0x04},
{0x91,0xb0},  
{0x45,0x01},
{0x46,0x00},
{0x47,0x6c},
{0x48,0x03},
{0x49,0x8b},
{0x4a,0x00},
{0x4b,0x07},
{0x4c,0x04},
{0x4d,0xb7},
//{0x3f,0x03},
{0x3f,0x00},
};

static const SENSOR_REG_T sp2509r_preview_setting[] = {

};

static const SENSOR_REG_T sp2509r_snapshot_setting[] = {


};

static const SENSOR_REG_T sp2509r_video_setting[] = {
	

};

static struct sensor_res_tab_info s_sp2509r_resolution_tab_raw[VENDOR_NUM] = {
	{
      .module_id = MODULE_SUNNY,
      .reg_tab = {
        {ADDR_AND_LEN_OF_ARRAY(sp2509r_init_setting), PNULL, 0,
        .width = 0, .height = 0,
        .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW},
#ifndef NOT_SUPPORT_VIDEO
		{ADDR_AND_LEN_OF_ARRAY(sp2509r_video_setting), PNULL, 0,
        .width = VIDEO_WIDTH, .height = VIDEO_HEIGHT,
        .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW},
		
        {ADDR_AND_LEN_OF_ARRAY(sp2509r_preview_setting), PNULL, 0,
        .width = PREVIEW_WIDTH, .height = PREVIEW_HEIGHT,
        .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW},
#endif
        {ADDR_AND_LEN_OF_ARRAY(sp2509r_snapshot_setting), PNULL, 0,
        .width = SNAPSHOT_WIDTH, .height = SNAPSHOT_HEIGHT,
        .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW}
		}
	}

	/*If there are multiple modules,please add here*/
};

static SENSOR_TRIM_T s_sp2509r_resolution_trim_tab[VENDOR_NUM] = {
	{
     .module_id = MODULE_SUNNY,
     .trim_info = {
       {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
#ifndef NOT_SUPPORT_VIDEO	   
	   {.trim_start_x = VIDEO_TRIM_X, .trim_start_y = VIDEO_TRIM_Y,
        .trim_width = VIDEO_TRIM_W,   .trim_height = VIDEO_TRIM_H,
        .line_time = VIDEO_LINE_TIME, .bps_per_lane = VIDEO_MIPI_PER_LANE_BPS,
        .frame_line = VIDEO_FRAME_LENGTH,
        .scaler_trim = {.x = VIDEO_TRIM_X, .y = VIDEO_TRIM_Y, .w = VIDEO_TRIM_W, .h = VIDEO_TRIM_H}},
	   
	   {.trim_start_x = PREVIEW_TRIM_X, .trim_start_y = PREVIEW_TRIM_Y,
        .trim_width = PREVIEW_TRIM_W,   .trim_height = PREVIEW_TRIM_H,
        .line_time = PREVIEW_LINE_TIME, .bps_per_lane = PREVIEW_MIPI_PER_LANE_BPS,
        .frame_line = PREVIEW_FRAME_LENGTH,
        .scaler_trim = {.x = PREVIEW_TRIM_X, .y = PREVIEW_TRIM_Y, .w = PREVIEW_TRIM_W, .h = PREVIEW_TRIM_H}},
 #endif	      
	   {
        .trim_start_x = SNAPSHOT_TRIM_X, .trim_start_y = SNAPSHOT_TRIM_Y,
        .trim_width = SNAPSHOT_TRIM_W,   .trim_height = SNAPSHOT_TRIM_H,
        .line_time = SNAPSHOT_LINE_TIME, .bps_per_lane = SNAPSHOT_MIPI_PER_LANE_BPS,
        .frame_line = SNAPSHOT_FRAME_LENGTH,
        .scaler_trim = {.x = SNAPSHOT_TRIM_X, .y = SNAPSHOT_TRIM_Y, .w = SNAPSHOT_TRIM_W, .h = SNAPSHOT_TRIM_H}},
       }
	}

    /*If there are multiple modules,please add here*/

};

static SENSOR_REG_T sp2509r_shutter_reg[] = {
    {0xfd, 0x01}, 
	{0x03, 0}, 
	{0x04, 0}, 
	{0x01, 0x01}, 

};

static struct sensor_i2c_reg_tab sp2509r_shutter_tab = {
    .settings = sp2509r_shutter_reg, 
	.size = ARRAY_SIZE(sp2509r_shutter_reg),
};

static SENSOR_REG_T sp2509r_again_reg[] = {
	
    {0xfd, 0x01}, 
	{0x24, 0x00}, 
	{0x01, 0x01}, 
 
};

static struct sensor_i2c_reg_tab sp2509r_again_tab = {
    .settings = sp2509r_again_reg, 
	.size = ARRAY_SIZE(sp2509r_again_reg),
};

static SENSOR_REG_T sp2509r_dgain_reg[] = {
   
};

static struct sensor_i2c_reg_tab sp2509r_dgain_tab = {
    .settings = sp2509r_dgain_reg, 
	.size = ARRAY_SIZE(sp2509r_dgain_reg),
};

static SENSOR_REG_T sp2509r_frame_length_reg[] = {

};

static struct sensor_i2c_reg_tab sp2509r_frame_length_tab = {
    .settings = sp2509r_frame_length_reg,
    .size = ARRAY_SIZE(sp2509r_frame_length_reg),
};

static struct sensor_aec_i2c_tag sp2509r_aec_info = {
    .slave_addr = (I2C_SLAVE_ADDR >> 1),
    .addr_bits_type = SENSOR_I2C_REG_16BIT,
    .data_bits_type = SENSOR_I2C_VAL_8BIT,
    .shutter = &sp2509r_shutter_tab,
    .again = &sp2509r_again_tab,
    .dgain = &sp2509r_dgain_tab,
    .frame_length = &sp2509r_frame_length_tab,
};


static SENSOR_STATIC_INFO_T s_sp2509r_static_info[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .static_info = {
        .f_num = 200,
        .focal_length = 354,
        .max_fps = 0,
        .max_adgain = 15 * 2,
        .ois_supported = 0,
        .pdaf_supported = 0,
        .exp_valid_frame_num = 1,
        .clamp_level = 64,
        .adgain_valid_frame_num = 1,
        .fov_info = {{4.614f, 3.444f}, 4.222f}}
    }
    /*If there are multiple modules,please add here*/
};

static SENSOR_MODE_FPS_INFO_T s_sp2509r_mode_fps_info[VENDOR_NUM] = {
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

static struct sensor_module_info s_sp2509r_module_info_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .module_info = {
         .major_i2c_addr = I2C_SLAVE_ADDR >> 1,
         .minor_i2c_addr = I2C_SLAVE_ADDR >> 1,

         .reg_addr_value_bits = SENSOR_I2C_REG_8BIT | SENSOR_I2C_VAL_8BIT |
                                SENSOR_I2C_FREQ_100,

         .avdd_val = SENSOR_AVDD_2800MV,
         .iovdd_val = SENSOR_AVDD_1800MV,
         .dvdd_val = SENSOR_AVDD_CLOSED,

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

static struct sensor_ic_ops s_sp2509r_ops_tab;
struct sensor_raw_info *s_sp2509r_mipi_raw_info_ptr = &s_sp2509r_mipi_raw_info;

SENSOR_INFO_T g_sp2509r_mipi_raw_info = {
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
        {{ .reg_addr = sp2509r_PID_ADDR, .reg_value = sp2509r_PID_VALUE},
         { .reg_addr = sp2509r_VER_ADDR, .reg_value = sp2509r_VER_VALUE}},

    .source_width_max = SNAPSHOT_WIDTH,
    .source_height_max = SNAPSHOT_HEIGHT,
    .name = (cmr_s8 *)SENSOR_NAME,
    .image_format = SENSOR_IMAGE_FORMAT_RAW,

    .module_info_tab = s_sp2509r_module_info_tab,
    .module_info_tab_size = ARRAY_SIZE(s_sp2509r_module_info_tab),

    .resolution_tab_info_ptr = s_sp2509r_resolution_tab_raw,
    .sns_ops = &s_sp2509r_ops_tab,
    .raw_info_ptr = &s_sp2509r_mipi_raw_info_ptr,

    .video_tab_info_ptr = NULL,
    .sensor_version_info = (cmr_s8 *)"sp2509r_v1",
};
