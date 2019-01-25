/**
 *	Copyright (c) 2016 Spreadtrum Technologies, Inc.
 *	All Rights Reserved.
 *	Confidential and Proprietary - Spreadtrum Technologies, Inc.
 **/
#define LOG_TAG "gc5024_common_otp_drv"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <utils/Log.h>
#include "sensor_drv_u.h"
#include "otp_common.h"
#include "gc5024_common_golden_otp.h"
#include "cmr_sensor_info.h"

/*Module Vendor ID*/
#define MODULE_ID 0x00

/*I2C slave address setting*/
#define OTP_I2C_ADDR (0x6e >> 1)
#define SENSOR_I2C_ADDR (0x6e >> 1)

/*OTP space setting*/
#define OTP_START_ADDR 0x00
#define OTP_END_ADDR 0xa0
#define OTP_RAW_DATA_LEN ((OTP_END_ADDR - OTP_START_ADDR + 8) / 8)

/*module base info*/
#define MODULE_INFO_GROUP_FLAG_OFFSET 0x00 /*for Sensor OTP memory*/
#define MODULE_INFO_OFFSET 0x08
#define MODULE_INFO_END_OFFSET 0x70
#define MODULE_INFO_SECTION_NUM 2
#define MODULE_INFO_SIZE 8
/**********************************
// gc_stand otp no module_info_checksum
#define MODULE_INFO_CHECKSUM 0x00

***********************************/

/*AWB*/
#define AWB_INFO_GROUP_FLAG_OFFSET 0x00 /*for Sensor OTP memory*/
#define AWB_INFO_OFFSET 0x78
#define AWB_INFO_END_OFFSET 0x90

/*******************************
// gc_stand otp no awb_info_checksum
#define AWB_INFO_CHECKSUM 0x00
*******************************/

#define AWB_INFO_SIZE 2
#define AWB_SECTION_NUM 2
/*AF*/
#define AF_INFO_GROUP_FLAG_OFFSET 0x0000 /*for Sensor OTP memory*/
#define AF_INFO_OFFSET 0x0010
#define AF_INFO_END_OFFSET 0x0015
#define AF_INFO_CHECKSUM 0x0015
/*AE*/
#define AE_INFO_OFFSET 0x00
#define AE_INFO_END_OFFSET 0x000
#define AE_INFO_CHECKSUM 0x00
/*Lens shading calibration*/
#define LSC_INFO_GROUP_FLAG_OFFSET 0x00 /*for Sensor OTP memory*/
#define LSC_INFO_OFFSET 0x00
#define LSC_INFO_END_OFFSET 0x00
#define LSC_INFO_CHECKSUM 0x000
#define LSC_INFO_CHANNEL_SIZE 00
#define LSC_DATA_SIZE 0

/*optical center data*/
#define OPTICAL_INFO_OFFSET 0x0000

/*dualcamera data calibration*/
#define DUAL_INFO_OFFSET 0x000
#define DUAL_INFO_END_OFFSET 0x00
#define DUAL_INFO_CHECKSUM 0x00
#define DUAL_DATA_SIZE (DUAL_INFO_END_OFFSET - DUAL_INFO_OFFSET)

/*PDAF*/
#define PDAF_INFO_OFFSET 0x00
#define PDAF_INFO_END_OFFSET 0x00
#define PDAF_INFO_CHECKSUM 0x00

/*reserved data*/
#define RES_INFO_OFFSET 0x98
#define RES_INFO_END_OFFSET 0xa0
#define RES_INFO_CHECKSUM 0x00
#define RES_DATA_SIZE (RES_INFO_END_OFFSET - RES_INFO_OFFSET)
/**/

#define GAIN_WIDTH 23
#define GAIN_HEIGHT 18
#define LSC_FORMAT_SIZE ((LSC_INFO_END_OFFSET - LSC_INFO_OFFSET) * 2)

#define RG_TYPICAL 0xA6
#define BG_TYPICAL 0x9A
//#define INFO_ROM_START			0x08
//#define INFO_WIDTH       			0x07
//#define WB_ROM_START           		0x78
//#define WB_WIDTH         			0x02

typedef enum {
    otp_close = 0,
    otp_open,
} otp_state;

static cmr_int gc5024_common_otp_drv_create(otp_drv_init_para_t *input_para,
                                            cmr_handle *sns_af_drv_handle);
static cmr_int gc5024_common_otp_drv_delete(cmr_handle otp_drv_handle);
static cmr_int gc5024_common_otp_drv_read(cmr_handle otp_drv_handle,
                                          void *p_params);
static cmr_int gc5024_common_otp_drv_write(cmr_handle otp_drv_handle,
                                           void *p_params);
static cmr_int gc5024_common_otp_drv_parse(cmr_handle otp_drv_handle,
                                           void *p_params);
static cmr_int gc5024_common_otp_drv_calibration(cmr_handle otp_drv_handle);
static cmr_int gc5024_common_otp_drv_ioctl(cmr_handle otp_drv_handle,
                                           cmr_uint cmd, void *p_params);
static cmr_int _gc5024_common_otp_enable(cmr_handle otp_drv_handle,
                                         otp_state state);

otp_drv_entry_t gc5024_common_drv_entry = {
    .otp_cfg =
        {
            .cali_items =
                {
                    .is_self_cal = TRUE, /*TRUE: OC calibration on,FALSE: OC
                                            calibration off*/
                    .is_awbc = TRUE,     /* TRUE: support awb calibration,
                                            FALSE: Not support awb calibration
                                            */
                    .is_awbc_self_cal = TRUE, /*TRUE: Sensor side
                                                 calibration, FALSE: ISP
                                                 Side calibration*/
                    .is_lsc = FALSE,          /* TRUE: support lsc calibration,
                                                 FALSE: Not support lsc calibration
                                                 */
                    .is_lsc_self_cal = FALSE, /*TRUE: Sensor side
                                                 calibration, FALSE: ISP
                                                 Side calibration*/
                    .is_pdafc = FALSE,        /* TRUE: support pdaf calibration,
                                                 FALSE: Not support pdaf
                                                 calibration */
                    .is_pdaf_self_cal = FALSE, /*TRUE: Sensor side
                                                  calibration, FALSE: ISP
                                                  Side calibration*/
                    .is_afc = FALSE,
                    .is_af_self_cal = FALSE,
                    .is_dul_camc =
                        FALSE, /* TRUE: support dual camera  calibration,
                                  FALSE: Not support dual camera
                                  calibration */
                },
            .base_info_cfg =
                {
                    /*decompression on otp driver or isp*/
                    .is_lsc_drv_decompression = FALSE,
                    /*otp data compressed format,
                      should confirm with module fae*/
                    .compress_flag = GAIN_ORIGIN_BITS,
                    /*the width of the stream the sensor can output*/
                    .image_width = 2592,
                    /*the height of the stream the sensor can output*/
                    .image_height = 1944,
                    .grid_width = 23,
                    .grid_height = 18,
                    .gain_width = GAIN_WIDTH,
                    .gain_height = GAIN_HEIGHT,
                },
        },
    .otp_ops =
        {
            .sensor_otp_create = gc5024_common_otp_drv_create,
            .sensor_otp_delete = gc5024_common_otp_drv_delete,
            .sensor_otp_read = gc5024_common_otp_drv_read,
            .sensor_otp_write = gc5024_common_otp_drv_write,
            .sensor_otp_parse = gc5024_common_otp_drv_parse,
            .sensor_otp_calibration = gc5024_common_otp_drv_calibration,
            .sensor_otp_ioctl = gc5024_common_otp_drv_ioctl, /*expend*/
        },
};
