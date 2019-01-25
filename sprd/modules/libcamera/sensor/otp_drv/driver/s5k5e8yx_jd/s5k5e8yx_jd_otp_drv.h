/**
 *      Copyright (c) 2016 Spreadtrum Technologies, Inc.
 *      All Rights Reserved.
 *     Confidential and Proprietary - Spreadtrum Technologies, Inc.
 **/
#define LOG_TAG "s5k5e8yx_jd_otp_drv"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <utils/Log.h>
#include "sensor_drv_u.h"
#include "otp_common.h"
#include "s5k5e8yx_jd_golden_otp.h"
#include "cmr_sensor_info.h"

#define GT24C64A_I2C_ADDR 0xA0 >> 1
#define OTP_START_ADDR 0x0000
#define OTP_END_ADDR 0x0FFF

#define OTP_LEN 8192
#define GAIN_WIDTH 23
#define GAIN_HEIGHT 18

/*module base info*/
#define MODULE_INFO_OFFSET 0x0000
#define MODULE_INFO_CHECK_SUM 0x0050
#define MODULE_INFO_CHECKSUM 0x0050
/*AF*/
#define AF_INFO_OFFSET 0x0051
#define AF_INFO_END_OFFSET 0x005B
#define AF_INFO_CHECKSUM 0x005B
/*AWB*/
#define AWB_INFO_OFFSET 0x005C
#define AWB_INFO_END_OFFSET 0x0069
#define AWB_INFO_CHECKSUM 0x0069
#define AWB_INFO_SIZE 6
#define AWB_SECTION_NUM 2
/*OPTICAL*/
#define OPTICAL_INFO_OFFSET 0x006A //
#define LSC_INFO_OFFSET 0x0080
#define LSC_INFO_END_OFFSET 0x0AC0 // 0B8B
#define LSC_INFO_CHANNEL_SIZE 442 // 726
#define LSC_INFO_CHECKSUM 0x0AC0 // 0B8B

/*PDAF*/
#define PDAF_INFO_OFFSET 0x0AC1 // 0B8C
#define PDAF_INFO_END_OFFSET 0x0C42 // 0x081C//0D0C
#define PDAF_INFO_CHECKSUM 0x0C42 // 0D0C
/*SPC*/
#define SPC_INFO_OFFSET 0x0C43
#define SPC_INFO_END_OFFSET 0x0CC3
#define SPC_INFO_CHECKSUM 0x0CC3

/*Lens shading calibration*/

#define AF_60CM_POS 0x0FFC
/*reserve*/
#define RES_INFO_OFFSET 0xCC4 // E0D
#define RES_INFO_END_OFFSET 0x0FFF
#define RES_INFO_CHECKSUM 0x0FFF
#define RES_DATA_SIZE 1633 // 497
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

static cmr_int _s5k5e8yx_jd_section_checksum(cmr_u8 *buf, cmr_uint first,
                                             cmr_uint last, cmr_uint position);
static cmr_int _s5k5e8yx_jd_buffer_init(cmr_handle otp_drv_handle);
static cmr_int _s5k5e8yx_jd_parse_awb_data(cmr_handle otp_drv_handle);
static cmr_int _s5k5e8yx_jd_parse_lsc_data(cmr_handle otp_drv_handle);
static cmr_int _s5k5e8yx_jd_parse_af_data(cmr_handle otp_drv_handle);
static cmr_int _s5k5e8yx_jd_parse_pdaf_data(cmr_handle otp_drv_handle);

static cmr_int _s5k5e8yx_jd_awb_calibration(cmr_handle otp_drv_handle);
static cmr_int _s5k5e8yx_jd_lsc_calibration(cmr_handle otp_drv_handle);
static cmr_int _s5k5e8yx_jd_pdaf_calibration(cmr_handle otp_drv_handle);

static cmr_int s5k5e8yx_jd_otp_drv_create(otp_drv_init_para_t *input_para,
                                          cmr_handle *sns_af_drv_handle);
static cmr_int s5k5e8yx_jd_otp_drv_delete(cmr_handle otp_drv_handle);
static cmr_int s5k5e8yx_jd_otp_drv_read(cmr_handle otp_drv_handle,
                                        void *p_data);
static cmr_int s5k5e8yx_jd_otp_drv_write(cmr_handle otp_drv_handle,
                                         void *p_data);
static cmr_int s5k5e8yx_jd_otp_drv_parse(cmr_handle otp_drv_handle,
                                         void *P_params);
static cmr_int s5k5e8yx_jd_otp_drv_calibration(cmr_handle otp_drv_handle);
static cmr_int s5k5e8yx_jd_otp_drv_ioctl(cmr_handle otp_drv_handle,
                                         cmr_uint cmd, void *params);

otp_drv_entry_t s5k5e8yx_jd_otp_entry = {
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
                    .full_img_width = 2592,
                    .full_img_height = 1944,
                    .lsc_otp_grid = 96,
                    .gain_width = GAIN_WIDTH,
                    .gain_height = GAIN_HEIGHT,
                },
        },
    .otp_ops =
        {
            .sensor_otp_create = s5k5e8yx_jd_otp_drv_create,
            .sensor_otp_delete = s5k5e8yx_jd_otp_drv_delete,
            .sensor_otp_read = s5k5e8yx_jd_otp_drv_read,
            .sensor_otp_write = s5k5e8yx_jd_otp_drv_write,
            .sensor_otp_parse = s5k5e8yx_jd_otp_drv_parse,
            .sensor_otp_calibration = s5k5e8yx_jd_otp_drv_calibration,
            .sensor_otp_ioctl = s5k5e8yx_jd_otp_drv_ioctl, /*expend*/
        },
};
