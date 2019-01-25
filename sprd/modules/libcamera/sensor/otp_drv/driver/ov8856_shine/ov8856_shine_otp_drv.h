/**
 *      Copyright (c) 2016 Spreadtrum Technologies, Inc.
 *      All Rights Reserved.
 *     Confidential and Proprietary - Spreadtrum Technologies, Inc.
 **/
#define LOG_TAG "ov8856_shine_otp_drv"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <utils/Log.h>
#include "sensor_drv_u.h"
#include "otp_common.h"
#include "ov8856_shine_golden_otp.h"
#include "cmr_sensor_info.h"

#define GT24C64A_I2C_ADDR 0xA0 >> 1
#define OTP_START_ADDR 0x0000
#define OTP_END_ADDR 0x0FFF

#define OTP_LEN 8192
#define GAIN_WIDTH 23
#define GAIN_HEIGHT 18

#define WB_DATA_SIZE 8 * 2 /*Don't forget truly wb data*/
#define AF_DATA_SIZE 6
#define LSC_SRC_CHANNEL_SIZE 656
#define LSC_CHANNEL_SIZE 876

/*Don't forget truly lsc otp data*/
#define FORMAT_DATA_LEN                                                        \
    WB_DATA_SIZE + AF_DATA_SIZE + GAIN_WIDTH *GAIN_WIDTH * 4 * 2 * 2
/*module base info*/
#define MODULE_INFO_OFFSET 0x0000
#define MODULE_INFO_CHECK_SUM 0x000F
#define MODULE_INFO_CHECKSUM 0x000F
/*AF*/
#define AF_INFO_OFFSET 0x0010
#define AF_INFO_END_OFFSET 0x0015
#define AF_INFO_CHECKSUM 0x0015
/*AWB*/
#define AWB_INFO_OFFSET 0x0016
#define AWB_INFO_END_OFFSET 0x0022
#define AWB_INFO_CHECKSUM 0x0022
#define AWB_INFO_SIZE 6
#define AWB_SECTION_NUM 2
/*OPTICAL*/
#define OPTICAL_INFO_OFFSET 0x0023
#define LSC_INFO_OFFSET 0x0033
#define LSC_INFO_END_OFFSET 0x071B	 //0B8B
#define LSC_INFO_CHANNEL_SIZE 442 //726
#define LSC_INFO_CHECKSUM 0x071B //0B8B

/*PDAF*/
#define PDAF_INFO_OFFSET 0x071C //0B8C
#define PDAF_INFO_END_OFFSET 0x089C//0x081C//0D0C
#define PDAF_INFO_CHECKSUM 0x089C //0D0C
/*SPC*/
#define SPC_INFO_OFFSET 0x089D
#define SPC_INFO_END_OFFSET 0x099C
#define SPC_INFO_CHECKSUM 0x099C

/*AE*/
#define AE_INFO_OFFSET 0x089D//0D0D
#define AE_INFO_END_OFFSET 0x099C//0x08AF//0D25
#define AE_INFO_CHECKSUM 0x099C//0D25
/*Lens shading calibration*/

/*dualcamera data calibration*/
#define DUAL_INFO_OFFSET 0x08B6//0D26
#define DUAL_INFO_END_OFFSET 0x099A //0E0A
#define VCM_INFO_OFFSET 0x099A //0E0A
#define DUAL_INFO_CHECKSUM 0x099C//E0C
#define DUAL_DATA_SIZE 230
#define VCM_DATA_SIZE 2

#define AF_60CM_POS 0x0FFC
/*reserve*/
#define RES_INFO_OFFSET 0x099D //E0D
#define RES_INFO_END_OFFSET 0x0FFF
#define RES_INFO_CHECKSUM 0x0FFE
#define RES_DATA_SIZE 1633//497
/**/
#define TOTAL_CHECKSUM_OFFSET 0x0FFF

#define LSC_GRID_SIZE 96 // 726
#define LSC_FORMAT_SIZE                                                        \
    GAIN_WIDTH *GAIN_HEIGHT * 2 * 4 * 2 /*include truly and random data*/
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

static cmr_int _ov8856_shine_section_checksum(cmr_u8 *buf, cmr_uint first,
                                               cmr_uint last,
                                               cmr_uint position);
static cmr_int _ov8856_shine_buffer_init(cmr_handle otp_drv_handle);
static cmr_int _ov8856_shine_parse_awb_data(cmr_handle otp_drv_handle);
static cmr_int _ov8856_shine_parse_lsc_data(cmr_handle otp_drv_handle);
static cmr_int _ov8856_shine_parse_af_data(cmr_handle otp_drv_handle);
static cmr_int _ov8856_shine_parse_pdaf_data(cmr_handle otp_drv_handle);

static cmr_int _ov8856_shine_awb_calibration(cmr_handle otp_drv_handle);
static cmr_int _ov8856_shine_lsc_calibration(cmr_handle otp_drv_handle);
static cmr_int _ov8856_shine_pdaf_calibration(cmr_handle otp_drv_handle);

static cmr_int ov8856_shine_otp_drv_create(otp_drv_init_para_t *input_para,
                                            cmr_handle *sns_af_drv_handle);
static cmr_int ov8856_shine_otp_drv_delete(cmr_handle otp_drv_handle);
static cmr_int ov8856_shine_otp_drv_read(cmr_handle otp_drv_handle,
                                          void *p_data);
static cmr_int ov8856_shine_otp_drv_write(cmr_handle otp_drv_handle,
                                           void *p_data);
static cmr_int ov8856_shine_otp_drv_parse(cmr_handle otp_drv_handle,
                                           void *P_params);
static cmr_int ov8856_shine_otp_drv_calibration(cmr_handle otp_drv_handle);
static cmr_int ov8856_shine_otp_drv_ioctl(cmr_handle otp_drv_handle,
                                           cmr_uint cmd, void *params);

otp_drv_entry_t ov8856_shine_otp_entry = {
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
                    .full_img_width = 3264,
                    .full_img_height = 2448,
                    .lsc_otp_grid = 96,
                    .gain_width = GAIN_WIDTH,
                    .gain_height = GAIN_HEIGHT,
                },
        },
    .otp_ops =
        {
            .sensor_otp_create = ov8856_shine_otp_drv_create,
            .sensor_otp_delete = ov8856_shine_otp_drv_delete,
            .sensor_otp_read = ov8856_shine_otp_drv_read,
            .sensor_otp_write = ov8856_shine_otp_drv_write,
            .sensor_otp_parse = ov8856_shine_otp_drv_parse,
            .sensor_otp_calibration = ov8856_shine_otp_drv_calibration,
            .sensor_otp_ioctl = ov8856_shine_otp_drv_ioctl, /*expend*/
        },
};
