/**
 *	Copyright (c) 2016 Spreadtrum Technologies, Inc.
 *	All Rights Reserved.
 *	Confidential and Proprietary - Spreadtrum Technologies, Inc.
 **/
#define LOG_TAG "s5k3l8xxm3_qtech_otp_drv"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <utils/Log.h>
#include "sensor_drv_u.h"
#include "otp_common.h"
#include "s5k3l8xxm3_qtech_golden_otp.h"
#include "cmr_sensor_info.h"

/*Module Vendor ID*/
#define MODULE_VENDOR_ID 0x06

/*I2C slave address setting*/
#define OTP_I2C_ADDR (0xA8 >> 1)
#define SENSOR_I2C_ADDR (0x5A >> 1)

/*OTP space setting*/
#define OTP_START_ADDR 0x0000
#define OTP_END_ADDR 0x01FFF
#define OTP_RAW_DATA_LEN (OTP_END_ADDR - OTP_START_ADDR + 1)

/*module base info*/
#define MODULE_INFO_GROUP_FLAG_OFFSET 0x0000 /*for Sensor OTP memory*/
#define MODULE_INFO_OFFSET 0x0000
#define MODULE_INFO_END_OFFSET 0x000F
#define MODULE_INFO_CHECKSUM 0x000F
/*AWB*/
#define AWB_INFO_GROUP_FLAG_OFFSET 0x0000 /*for Sensor OTP memory*/
#define AWB_INFO_OFFSET 0x0016
#define AWB_INFO_END_OFFSET 0x0022
#define AWB_INFO_CHECKSUM 0x0022
#define AWB_INFO_SIZE 6
#define AWB_SECTION_NUM 1
/*AF*/
#define AF_INFO_GROUP_FLAG_OFFSET 0x0000 /*for Sensor OTP memory*/
#define AF_INFO_OFFSET 0x0010
#define AF_INFO_END_OFFSET 0x0015
#define AF_INFO_CHECKSUM 0x0015
/*AE*/
#define AE_INFO_OFFSET 0x0D0D
#define AE_INFO_END_OFFSET 0x0D25
#define AE_INFO_CHECKSUM 0x0D25
/*Lens shading calibration*/
#define LSC_INFO_GROUP_FLAG_OFFSET 0x0000 /*for Sensor OTP memory*/
#define LSC_INFO_OFFSET 0x0033
#define LSC_INFO_END_OFFSET 0x0B8B
#define LSC_INFO_CHECKSUM 0x0B8B
#define LSC_INFO_CHANNEL_SIZE 726
#define LSC_DATA_SIZE 2920

/*optical center data*/
#define OPTICAL_INFO_OFFSET 0x0023

/*dualcamera data calibration*/
#define DUAL_INFO_OFFSET 0x0D26
#define DUAL_INFO_END_OFFSET 0xE0C
#define DUAL_INFO_CHECKSUM 0xE0C
#define DUAL_DATA_SIZE (DUAL_INFO_END_OFFSET - DUAL_INFO_OFFSET)

/*PDAF*/
#define PDAF_INFO_OFFSET 0x0B8C
#define PDAF_INFO_END_OFFSET 0x0D0C
#define PDAF_INFO_CHECKSUM 0x0D0C

/*reserved data*/
#define RES_INFO_OFFSET 0x0E0D
#define RES_INFO_END_OFFSET 0x0FFF
#define RES_INFO_CHECKSUM 0x0FFE
#define RES_DATA_SIZE (RES_INFO_END_OFFSET - RES_INFO_OFFSET)
/**/

#define GAIN_WIDTH 23
#define GAIN_HEIGHT 18
#define LSC_FORMAT_SIZE ((LSC_INFO_END_OFFSET - LSC_INFO_OFFSET) * 2)

static cmr_int s5k3l8xxm3_qtech_otp_drv_create(otp_drv_init_para_t *input_para,
                                               cmr_handle *sns_af_drv_handle);
static cmr_int s5k3l8xxm3_qtech_otp_drv_delete(cmr_handle otp_drv_handle);
static cmr_int s5k3l8xxm3_qtech_otp_drv_read(cmr_handle otp_drv_handle,
                                             void *p_params);
static cmr_int s5k3l8xxm3_qtech_otp_drv_write(cmr_handle otp_drv_handle,
                                              void *p_params);
static cmr_int s5k3l8xxm3_qtech_otp_drv_parse(cmr_handle otp_drv_handle,
                                              void *p_params);
static cmr_int s5k3l8xxm3_qtech_otp_drv_calibration(cmr_handle otp_drv_handle);
static cmr_int s5k3l8xxm3_qtech_otp_drv_ioctl(cmr_handle otp_drv_handle,
                                              cmr_uint cmd, void *p_params);

otp_drv_entry_t
    s5k3l8xxm3_qtech_drv_entry =
        {
            .otp_cfg =
                {
                    .cali_items =
                        {
                            .is_self_cal =
                                TRUE,        /*TRUE:OC calibration on,FALSE; OC
                                                calibration off*/
                            .is_awbc = TRUE, /* TRUE: support awb calibration,
                                                FALSE: Not support awb
                                                calibration */
                            .is_awbc_self_cal = FALSE, /*TRUE: Sensor side
                                                          calibration, FALSE:
                                                          ISP Side calibration*/
                            .is_lsc = TRUE, /* TRUE: support lsc calibration,
                                               FALSE: Not support lsc
                                               calibration */
                            .is_lsc_self_cal =
                                FALSE, /*TRUE: Sensor side calibration, FALSE:
                                          ISP Side calibration*/
                            .is_pdafc = FALSE,         /* TRUE: support pdaf
                                                          calibration, FALSE: Not
                                                          support pdaf calibration */
                            .is_pdaf_self_cal = FALSE, /*TRUE: Sensor side
                                                          calibration, FALSE:
                                                          ISP Side calibration*/
                            .is_afc = FALSE,
                            .is_af_self_cal = FALSE,
                            .is_dul_camc = FALSE, /* TRUE: support dual camera
                                                     calibration, FALSE: Not
                                                     support dual camera
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
                            .full_img_width = 4208,
                            /*the height of the stream the sensor can output*/
                            .full_img_height = 3120,
                            .lsc_otp_grid = 96,
                            .gain_width = GAIN_WIDTH,
                            .gain_height = GAIN_HEIGHT,
                        },
                },
            .otp_ops =
                {
                    .sensor_otp_create = s5k3l8xxm3_qtech_otp_drv_create,
                    .sensor_otp_delete = s5k3l8xxm3_qtech_otp_drv_delete,
                    .sensor_otp_read = s5k3l8xxm3_qtech_otp_drv_read,
                    .sensor_otp_write = s5k3l8xxm3_qtech_otp_drv_write,
                    .sensor_otp_parse = s5k3l8xxm3_qtech_otp_drv_parse,
                    .sensor_otp_calibration =
                        s5k3l8xxm3_qtech_otp_drv_calibration,
                    .sensor_otp_ioctl =
                        s5k3l8xxm3_qtech_otp_drv_ioctl, /*expend*/
                },
};
