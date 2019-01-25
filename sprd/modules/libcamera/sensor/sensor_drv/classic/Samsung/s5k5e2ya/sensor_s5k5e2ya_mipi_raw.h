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
#ifndef SENSOR_S5K5E2YA_MIPI_RAW_H_
#define SENSOR_S5K5E2YA_MIPI_RAW_H_

#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"

#if defined(CONFIG_CAMERA_ISP_VERSION_V3) ||                                   \
    defined(CONFIG_CAMERA_ISP_VERSION_V4)
#include "parameters/sensor_s5k5e2ya_raw_param_main.c"
#else
#include "parameters/sensor_s5k5e2ya_raw_param.c"
#endif

#define CAMERA_IMAGE_180

#define VENDOR_NUM 1 /*very important*/

#define SENSOR_NAME "s5k5e2ya_mipi_raw"
#define I2C_SLAVE_ADDR 0x20 /* 8bit slave address*/

#define s5k5e2ya_PID_ADDR 0x00
#define s5k5e2ya_PID_VALUE 0x5E
#define s5k5e2ya_VER_ADDR 0x01
#define s5k5e2ya_VER_VALUE 0x20

/* sensor parameters begin */
/* effective sensor output image size */
#define SNAPSHOT_WIDTH 2560
#define SNAPSHOT_HEIGHT 1920
#define PREVIEW_WIDTH 2560
#define PREVIEW_HEIGHT 1920

/*Raw Trim parameters*/
#define SNAPSHOT_TRIM_X 0
#define SNAPSHOT_TRIM_Y 0
#define SNAPSHOT_TRIM_W 2560
#define SNAPSHOT_TRIM_H 1920
#define PREVIEW_TRIM_X 0
#define PREVIEW_TRIM_Y 0
#define PREVIEW_TRIM_W 2560
#define PREVIEW_TRIM_H 1920

/*Mipi output*/
#define LANE_NUM 2
#define RAW_BITS 10

#define SNAPSHOT_MIPI_PER_LANE_BPS 860 /* 2*Mipi clk */
#define PREVIEW_MIPI_PER_LANE_BPS 860  /* 2*Mipi clk */

/*line time unit: 0.1us*/
#define SNAPSHOT_LINE_TIME 16200
#define PREVIEW_LINE_TIME 16200

/* frame length*/
#define SNAPSHOT_FRAME_LENGTH 0x80B
#define PREVIEW_FRAME_LENGTH 0x80B

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

/* isp parameters, please don't change it*/
#if defined(CONFIG_CAMERA_ISP_VERSION_V3) ||                                   \
    defined(CONFIG_CAMERA_ISP_VERSION_V4)
#define ISP_BASE_GAIN 0x80
#else
#define ISP_BASE_GAIN 0x10
#endif
/* please don't change it */
#define EX_MCLK 24

static const SENSOR_REG_T s5k5e2ya_init_setting[] = {
    // Reset for operation
    {0x0100, 0x00}, // stream off

    {0x0101, 0x00}, //[1:0] mirror and vertical flip
    // Clock Setting
    {0x0305, 0x06}, // 05 //PLLP (def:5)
    {0x0306, 0x00},
    {0x0307, 0xe0}, // B3 //PLLM (def:CCh 204d --> B3h 179d)
    {0x3C1F, 0x00}, // PLLS
    {0x30CC, 0x30}, //[7:4] dphy_band_ctrl

    {0x0820,
     0x03}, // requested link bit rate mbps : (def:3D3h 979d --> 35Bh 859d)
    {0x0821, 0x80},
    {0x3C1C, 0x58}, // dbr_div

    // Size Setting
    {0x0340,
     0x08}, // frame_length_lines : def. 990d (--> 3C8 Mimnimum 22 lines)
    {0x0341, 0x0b},
    {0x0342, 0x0B}, // line_length_pck : def. 2900d
    {0x0343, 0x54},

    {0x0344, 0x00}, // x_addr_start
    {0x0345, 0x08},
    {0x0346, 0x00}, // y_addr_start
    {0x0347, 0x08},
    {0x0348, 0x0A}, // x_addr_end : def. 2575d
    {0x0349, 0x07},
    {0x034A, 0x07}, // y_addr_end : def. 1936d
    {0x034B, 0x8F},
    {0x034C, 0x0a}, // x_output size : def. 1288d
    {0x034D, 0x00},
    {0x034E, 0x07}, // y_output size : def. 968d
    {0x034F, 0x80},

    // Digital Binning
    {0x0900, 0x00}, // 2x2 Binning
    {0x0901, 0x20},
    {0x0387, 0x01},

    // Integration time
    {0x0202, 0x02}, // coarse integration
    {0x0203, 0x00},
    {0x0200, 0x0A}, // fine integration (AA8h --> AC4h)
    {0x0201, 0xC4},

    // Analog Timing Tuning (0923)
    {0x3000, 0x04}, // ct_ld_start
    {0x3002, 0x03}, // ct_sl_start
    {0x3003, 0x04}, // ct_sl_margin
    {0x3004, 0x02}, // ct_rx_start
    {0x3005, 0x00}, // ct_rx_margin (MSB)
    {0x3006, 0x10}, // ct_rx_margin (LSB)
    {0x3007, 0x03}, // ct_tx_start
    {0x3008, 0x55}, // ct_tx_width
    {0x3039, 0x00}, // cintc1_margin
    {0x303A, 0x00}, // cintc2_margin
    {0x303B, 0x00}, // offs_sh
    {0x3009, 0x05}, // ct_srx_margin
    {0x300A, 0x55}, // ct_stx_width
    {0x300B, 0x38}, // ct_dstx_width
    {0x300C, 0x10}, // ct_stx2dstx
    {0x3012, 0x05}, // ct_cds_start
    {0x3013, 0x00}, // ct_s1s_start
    {0x3014, 0x22}, // ct_s1s_end
    {0x300E, 0x79}, // ct_s3_width
    {0x3010, 0x68}, // ct_s4_width
    {0x3019, 0x03}, // ct_s4d_start
    {0x301A, 0x00}, // ct_pbr_start
    {0x301B, 0x06}, // ct_pbr_width
    {0x301C, 0x00}, // ct_pbs_start
    {0x301D, 0x22}, // ct_pbs_width
    {0x301E, 0x00}, // ct_pbr_ob_start
    {0x301F, 0x10}, // ct_pbr_ob_width
    {0x3020, 0x00}, // ct_pbs_ob_start
    {0x3021, 0x00}, // ct_pbs_ob_width
    {0x3022, 0x0A}, // ct_cds_lim_start
    {0x3023, 0x1E}, // ct_crs_start
    {0x3024, 0x00}, // ct_lp_hblk_cds_start (MSB)
    {0x3025, 0x00}, // ct_lp_hblk_cds_start (LSB)
    {0x3026, 0x00}, // ct_lp_hblk_cds_end (MSB)
    {0x3027, 0x00}, // ct_lp_hblk_cds_end (LSB)
    {0x3028, 0x1A}, // ct_rmp_off_start
    {0x3015, 0x00}, // ct_rmp_rst_start (MSB)
    {0x3016, 0x84}, // ct_rmp_rst_start (LSB)
    {0x3017, 0x00}, // ct_rmp_sig_start (MSB)
    {0x3018, 0xA0}, // ct_rmp_sig_start (LSB)
    {0x302B, 0x10}, // ct_cnt_margin
    {0x302C, 0x0A}, // ct_rmp_per
    {0x302D, 0x06}, // ct_cnt_ms_margin1
    {0x302E, 0x05}, // ct_cnt_ms_margin2
    {0x302F, 0x0E}, // rst_mx
    {0x3030, 0x2F}, // sig_mx
    {0x3031, 0x08}, // ct_latch_start
    {0x3032, 0x05}, // ct_latch_width
    {0x3033, 0x09}, // ct_hold_start
    {0x3034, 0x05}, // ct_hold_width
    {0x3035, 0x00}, // ct_lp_hblk_dbs_start (MSB)
    {0x3036, 0x00}, // ct_lp_hblk_dbs_start (LSB)
    {0x3037, 0x00}, // ct_lp_hblk_dbs_end (MSB)
    {0x3038, 0x00}, // ct_lp_hblk_dbs_end (LSB)
    {0x3088, 0x06}, // ct_lat_lsb_offset_start1
    {0x308A, 0x08}, // ct_lat_lsb_offset_end1
    {0x308C, 0x05}, // ct_lat_lsb_offset_start2
    {0x308E, 0x07}, // ct_lat_lsb_offset_end2
    {0x3090, 0x06}, // ct_conv_en_offset_start1
    {0x3092, 0x08}, // ct_conv_en_offset_end1
    {0x3094, 0x05}, // ct_conv_en_offset_start2
    {0x3096, 0x21}, // ct_conv_en_offset_end2

    // CDS
    {0x3099, 0x0E}, // cds_option ([3]:crs switch disable, s3,s4 strengthx16)
    {0x3070, 0x10}, // comp1_bias (default:77)
    {0x3085, 0x11}, // comp1_bias (gain1~4)
    {0x3086, 0x01}, // comp1_bias (gain4~8)

    // RMP
    {0x3064, 0x00}, // Multiple sampling(gainx8,x16)
    {0x3062, 0x08}, // off_rst

    // DBR
    {0x3061, 0x11}, // dbr_tune_rd (default :08, 0E 3.02V)  3.1V
    {0x307B, 0x20}, // dbr_tune_rgsl (default :08)

    // Bias sampling
    {0x3068, 0x00}, // RMP BP bias sampling [0]: Disable
    {0x3074, 0x00}, // Pixel bias sampling [2]:Default L
    {0x307D, 0x00}, // VREF sampling [0] : Disable
    {0x3045, 0x01}, // ct_opt_l1_start
    {0x3046, 0x05}, // ct_opt_l1_width
    {0x3047, 0x78},

    // Smart PLA
    {0x307F, 0xB1}, // RDV_OPTION[5:4], RG default high
    {0x3098, 0x01}, // CDS_OPTION[16] SPLA-II enable
    {0x305C, 0xF6}, // lob_extension[6]

    {0x306B, 0x10}, // [3]bls_stx_en disable
    {0x3063, 0x27}, // ADC_SAT 490mV(19h) --> 610mV(2Fh) --> 570mV
    {0x320C, 0x07}, // ADC_MAX (def:076Ch --> 0700h)
    {0x320D, 0x00},
    {0x3400, 0x01}, // GAS bypass 0x01->0x00
    {0x3235, 0x49}, // L/F-ADLC on
    {0x3233, 0x00}, // D-pedestal L/F ADLC off (1FC0h)
    {0x3234, 0x00},
    {0x3300, 0x0C}, // BPC bypass  //0x0d->0x0c
    {0x0204, 0x00}, // Analog gain x1
    {0x0205, 0x20},
    {0x3931, 0x02},//vod  427-->470mv
    {0x3932, 0x60},//slew step down

    // streaming ON
    //{0x0100,0x01},
};

static const SENSOR_REG_T s5k5e2ya_preview_setting[] = {};

static const SENSOR_REG_T s5k5e2ya_snapshot_setting[] = {};

static struct sensor_res_tab_info s_s5k5e2ya_resolution_tab_raw[VENDOR_NUM] = {
    {
      .module_id = MODULE_SUNNY,
      .reg_tab = {
        {ADDR_AND_LEN_OF_ARRAY(s5k5e2ya_init_setting), PNULL, 0,
        .width = 0, .height = 0,
        .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW},

        {ADDR_AND_LEN_OF_ARRAY(s5k5e2ya_preview_setting), PNULL, 0,
        .width = PREVIEW_WIDTH, .height = PREVIEW_HEIGHT,
        .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW},

        {ADDR_AND_LEN_OF_ARRAY(s5k5e2ya_snapshot_setting), PNULL, 0,
        .width = SNAPSHOT_WIDTH, .height = SNAPSHOT_HEIGHT,
        .xclk_to_sensor = 24, .image_format = SENSOR_IMAGE_FORMAT_RAW}}
    }
/*If there are multiple modules,please add here*/
};

static SENSOR_TRIM_T s_s5k5e2ya_resolution_trim_tab[VENDOR_NUM] = {
    {
     .module_id = MODULE_SUNNY,
     .trim_info = {
       {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
       {.trim_start_x = PREVIEW_TRIM_X,.trim_start_y = PREVIEW_TRIM_Y,
        .trim_width = PREVIEW_TRIM_W,.trim_height = PREVIEW_TRIM_H,
        .line_time = PREVIEW_LINE_TIME,.bps_per_lane = PREVIEW_MIPI_PER_LANE_BPS,
        .frame_line = PREVIEW_FRAME_LENGTH,
        .scaler_trim = {.x = 0, .y = 0, .w = PREVIEW_TRIM_W, .h = PREVIEW_TRIM_H}},

       {.trim_start_x = SNAPSHOT_TRIM_X,.trim_start_y = SNAPSHOT_TRIM_Y,
        .trim_width = SNAPSHOT_TRIM_W,.trim_height = SNAPSHOT_TRIM_H,
        .line_time = SNAPSHOT_LINE_TIME,.bps_per_lane = SNAPSHOT_MIPI_PER_LANE_BPS,
        .frame_line = SNAPSHOT_FRAME_LENGTH,
        .scaler_trim = {.x = 0, .y = 0, .w = SNAPSHOT_TRIM_W, .h = SNAPSHOT_TRIM_H}},
      }}
    /*If there are multiple modules,please add here*/
};


static const SENSOR_REG_T
    s_s5k5e2ya_preview_size_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
        /*video mode 0: ?fps */
        {{0xffff, 0xff}},
        /* video mode 1:?fps */
        {{0xffff, 0xff}},
        /* video mode 2:?fps */
        {{0xffff, 0xff}},
        /* video mode 3:?fps */
        {{0xffff, 0xff}}};

static const SENSOR_REG_T
    s_s5k5e2ya_capture_size_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
        /*video mode 0: ?fps */
        {{0xffff, 0xff}},
        /* video mode 1:?fps */
        {{0xffff, 0xff}},
        /* video mode 2:?fps */
        {{0xffff, 0xff}},
        /* video mode 3:?fps */
        {{0xffff, 0xff}}};

static SENSOR_VIDEO_INFO_T s_s5k5e2ya_video_info[SENSOR_MODE_MAX] = {
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{30, 30, 270, 90}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
     (SENSOR_REG_T **)s_s5k5e2ya_preview_size_video_tab},
    {{{2, 5, 338, 1000}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
     (SENSOR_REG_T **)s_s5k5e2ya_capture_size_video_tab},
};

static SENSOR_STATIC_INFO_T s_s5k5e2ya_static_info[VENDOR_NUM] = {
    {
     .module_id = MODULE_SUNNY,
     .static_info = {
         .f_num = 280,
         .focal_length = 346,
         .max_fps = 0,
         .max_adgain = 16 * 256,
         .ois_supported = 0,
         .pdaf_supported = 0,
         .exp_valid_frame_num = 1,
         .clamp_level = 64,
         .adgain_valid_frame_num = 1,
         .fov_info = {{4.614f, 3.444f}, 4.222f}}
    }
    /*If there are multiple modules,please add here*/
};

static SENSOR_MODE_FPS_INFO_T s_s5k5e2ya_mode_fps_info[VENDOR_NUM] = {
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

static struct sensor_module_info s_s5k5e2ya_module_info_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .module_info = {
         .major_i2c_addr = I2C_SLAVE_ADDR >> 1 ,
         .minor_i2c_addr = I2C_SLAVE_ADDR >> 1 ,

         .reg_addr_value_bits = SENSOR_I2C_REG_16BIT | SENSOR_I2C_VAL_8BIT |
                                SENSOR_I2C_FREQ_400,

         .avdd_val = SENSOR_AVDD_2800MV,
         .iovdd_val = SENSOR_AVDD_1800MV,
         .dvdd_val = SENSOR_AVDD_1200MV,

         .image_pattern = SENSOR_IMAGE_PATTERN_RAWRGB_GR,

         .preview_skip_num = 1,
         .capture_skip_num = 1,
         .flash_capture_skip_num = 6,
         .mipi_cap_skip_num = 0,
         .preview_deci_num = 0,
         .video_preview_deci_num = 0,

         .sensor_interface = 
         {.type = SENSOR_INTERFACE_TYPE_CSI2, .bus_width = LANE_NUM,
          .pixel_width = RAW_BITS, .is_loose = 0},

         .change_setting_skip_num = 1,
         .horizontal_view_angle = 65,
         .vertical_view_angle = 60
      }
    }

/*If there are multiple modules,please add here*/
};

static struct sensor_ic_ops s5k5e2ya_ops_tab;
static struct sensor_raw_info *s_s5k5e2ya_mipi_raw_info_ptr =
                                   &s_s5k5e2ya_mipi_raw_info;
/*==============================================================================
 * Description:
 * sensor all info
 * please modify this variable acording your spec
 *============================================================================*/
SENSOR_INFO_T g_s5k5e2ya_mipi_raw_info = {
    .hw_signal_polarity = SENSOR_HW_SIGNAL_PCLK_P | SENSOR_HW_SIGNAL_VSYNC_P |
                          SENSOR_HW_SIGNAL_HSYNC_P,

    .environment_mode = SENSOR_ENVIROMENT_NORMAL | SENSOR_ENVIROMENT_NIGHT,

    .image_effect = SENSOR_IMAGE_EFFECT_NORMAL | SENSOR_IMAGE_EFFECT_BLACKWHITE |
                    SENSOR_IMAGE_EFFECT_RED | SENSOR_IMAGE_EFFECT_GREEN |
                    SENSOR_IMAGE_EFFECT_BLUE | SENSOR_IMAGE_EFFECT_YELLOW |
                    SENSOR_IMAGE_EFFECT_NEGATIVE | SENSOR_IMAGE_EFFECT_CANVAS,

    .wb_mode = 0,
    .step_count = 7,

    .reset_pulse_level = SENSOR_HIGH_PULSE_RESET,
    .reset_pulse_width = 50,
    .power_down_level = SENSOR_LOW_LEVEL_PWDN,
    .identify_count = 1,

    .identify_code = {{s5k5e2ya_PID_ADDR, s5k5e2ya_PID_VALUE},
                      {s5k5e2ya_VER_ADDR, s5k5e2ya_VER_VALUE}},

    .source_width_max = SNAPSHOT_WIDTH,
    .source_height_max = SNAPSHOT_HEIGHT,

    .name = (cmr_s8 *)SENSOR_NAME,
    .image_format = SENSOR_IMAGE_FORMAT_RAW,

    .resolution_tab_info_ptr = s_s5k5e2ya_resolution_tab_raw,
    .sns_ops = &s5k5e2ya_ops_tab,
    .raw_info_ptr = &s_s5k5e2ya_mipi_raw_info_ptr,
    .module_info_tab = s_s5k5e2ya_module_info_tab,
    .module_info_tab_size = ARRAY_SIZE(s_s5k5e2ya_module_info_tab),
    .ext_info_ptr = NULL,

    .sensor_version_info = (cmr_s8 *) "s5k5e2ya_v1",
};

#endif

