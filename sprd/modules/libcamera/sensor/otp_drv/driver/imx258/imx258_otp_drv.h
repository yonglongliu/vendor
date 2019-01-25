/**
 *	Copyright (c) 2016 Spreadtrum Technologies, Inc.
 *	All Rights Reserved.
 *	Confidential and Proprietary - Spreadtrum Technologies, Inc.
 **/
#define LOG_TAG "imx258_otp_drv"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <utils/Log.h>
#include "sensor_drv_u.h"
#include "otp_common.h"
#include "imx258_golden_otp.h"
#include "cmr_sensor_info.h"

#define GT24C64A_I2C_ADDR 0xA0 >> 1
#define OTP_START_ADDR 0x0000
#define OTP_END_ADDR 0x0FFF

#define OTP_LEN 8192
#define GAIN_WIDTH 23
#define GAIN_HEIGHT 18

#define WB_DATA_SIZE 8 * 2 /*Don't forget golden wb data*/
#define AF_DATA_SIZE 6
#define LSC_SRC_CHANNEL_SIZE 656
#define LSC_CHANNEL_SIZE 876

/*Don't forget golden lsc otp data*/
#define FORMAT_DATA_LEN                                                        \
    WB_DATA_SIZE + AF_DATA_SIZE + GAIN_WIDTH *GAIN_WIDTH * 4 * 2 * 2
/*module base info*/
#define MODULE_INFO_OFFSET 0x0000
#define MODULE_INFO_CHECKSUM 0x000F
/**/
#define AWB_INFO_OFFSET 0x0016
#define AWB_INFO_SIZE 6 /*byte*/
#define AWB_SECTION_NUM 2
#define AWB_INFO_CHECKSUM 0x0022
/**/
#define AF_INFO_OFFSET 0x0010
#define AF_INFO_SIZE 5 /*byte*/
#define AF_INFO_CHECKSUM 0x0015
/*optical center*/
#define OPTICAL_INFO_OFFSET 0x0023
/*Lens shading calibration*/
#define LSC_INFO_OFFSET 0x0033
#define LSC_INFO_CHANNEL_SIZE 726
#define LSC_INFO_CHECKSUM 0x0b8b
/*PDAF*/
#define PDAF_INFO_OFFSET 0x0b8c
#define PDAF_INFO_SIZE 384
#define PDAF_INFO_CHECKSUM 0x0d0c

#define LSC_GRID_SIZE 726
#define LSC_FORMAT_SIZE                                                        \
    GAIN_WIDTH *GAIN_HEIGHT * 2 * 4 * 2 /*include golden and random data*/
#define OTP_COMPRESSED_FLAG OTP_COMPRESSED_14BITS

typedef struct {
    unsigned char factory_id;
    unsigned char moule_id;
    unsigned char cali_version;
    unsigned char year;
    unsigned char month;
    unsigned char day;
    unsigned char sensor_id;
    unsigned char lens_id;
    unsigned char vcm_id;
    unsigned char drvier_ic_id;
    unsigned char ir_bg_id;
    unsigned char fw_ver;
    unsigned char af_cali_dir;
} module_info_t;

static cmr_int _imx258_section_checksum(unsigned char *buf, unsigned int first,
                                        unsigned int last,
                                        unsigned int position);
static cmr_int _imx258_buffer_init(cmr_handle otp_drv_handle);
static cmr_int _imx258_parse_awb_data(cmr_handle otp_drv_handle);
static cmr_int _imx258_parse_lsc_data(cmr_handle otp_drv_handle);
static cmr_int _imx258_parse_af_data(cmr_handle otp_drv_handle);
static cmr_int _imx258_parse_pdaf_data(cmr_handle otp_drv_handle);

static cmr_int _imx258_awb_calibration(cmr_handle otp_drv_handle);
static cmr_int _imx258_lsc_calibration(cmr_handle otp_drv_handle);
static cmr_int _imx258_pdaf_calibration(cmr_handle otp_drv_handle);

static cmr_int imx258_otp_drv_create(otp_drv_init_para_t *input_para,
                                     cmr_handle *sns_af_drv_handle);
static cmr_int imx258_otp_drv_delete(cmr_handle otp_drv_handle);
static cmr_int imx258_otp_drv_read(cmr_handle otp_drv_handle, void *p_data);
static cmr_int imx258_otp_drv_write(cmr_handle otp_drv_handle, void *p_data);
static cmr_int imx258_otp_drv_parse(cmr_handle otp_drv_handle, void *P_params);
static cmr_int imx258_otp_drv_calibration(cmr_handle otp_drv_handle);
static cmr_int imx258_otp_drv_ioctl(cmr_handle otp_drv_handle, int cmd,
                                    void *params);

otp_drv_entry_t imx258_drv_entry = {
    .otp_cfg =
        {
            .cali_items =
                {
                    .is_self_cal = TRUE,
                    .is_dul_camc = FALSE,
                    .is_awbc = TRUE,
                    .is_lsc = TRUE,
                    .is_pdafc = FALSE,
                },
            .base_info_cfg =
                {
                    .is_lsc_drv_decompression = FALSE,
                    .compress_flag = OTP_COMPRESSED_FLAG,
                    .full_img_width = 4208,
                    .full_img_height = 3120,
                    .lsc_otp_grid = 96,
                    .gain_width = GAIN_WIDTH,
                    .gain_height = GAIN_HEIGHT,
                },
        },
    .otp_ops =
        {
            .sensor_otp_create = imx258_otp_drv_create,
            .sensor_otp_delete = imx258_otp_drv_delete,
            .sensor_otp_read = imx258_otp_drv_read,
            .sensor_otp_write = imx258_otp_drv_write,
            .sensor_otp_parse = imx258_otp_drv_parse,
            .sensor_otp_calibration = imx258_otp_drv_calibration,
            .sensor_otp_ioctl = imx258_otp_drv_ioctl, /*expend*/
        },
};
