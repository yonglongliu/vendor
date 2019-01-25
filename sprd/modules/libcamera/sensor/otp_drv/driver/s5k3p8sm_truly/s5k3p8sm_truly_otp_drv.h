/**
 *	Copyright (c) 2016 Spreadtrum Technologies, Inc.
 *	All Rights Reserved.
 *	Confidential and Proprietary - Spreadtrum Technologies, Inc.
 **/
#define LOG_TAG "s5k3p8sm_otp_drv"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <utils/Log.h>
#include "sensor_drv_u.h"
#include "otp_common.h"
#include "s5k3p8sm_truly_golden_otp.h"
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
#define MODULE_INFO_END_OFFSET 0x0007

/**/
#define AWB_INFO_OFFSET 0x0010
#define AWB_INFO_END_OFFSET 0x0017
#define AWB_INFO_SIZE 8
#define AWB_SECTION_NUM 1
/**/
#define AF_INFO_OFFSET 0x06A0
#define AF_INFO_END_OFFSET 0x06A4
#define AF_INFO_CHECKSUM  0x06A5

/*Lens shading calibration*/
#define LSC_INFO_OFFSET 0x0020
#define LSC_INFO_END_OFFSET 0x0699
#define LSC_DATA_SIZE       1658

/*Dual camera iso,lsc calibration offset*/
#define MASTER_ISO_INFO_OFFSET 0x1065
#define MASTER_LSC_INFO_OFFSET 0x106d
#define MASTER_LSC_INFO_SIZE  1658
#define SLAVE_ISO_INFO_OFFSET 0x16e7
#define SLAVE_LSC_INFO_OFFSET 0x16ef
#define SLAVE_LSC_INFO_SIZE 400

#define LSC_INFO_CHANNEL_SIZE 726
#define LSC_INFO_CHECKSUM 0x0b8b
/*PDAF*/
#define PDAF_INFO_OFFSET 0x0700
#define PDAF_INFO_END_OFFSET 0x0927
#define PDAF_INFO_CHECKSUM 0x0701
/*SPC*/
#define SPC_INFO_OFFSET 0x0A2B
#define SPC_INFO_END_OFFSET 0x0AA9
#define SPC_INFO_CHECKSUM 0x0AAA
/**/
#define TOTAL_CHECKSUM_OFFSET 0x0AAB
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

static cmr_int _s5k3p8sm_truly_section_checksum(cmr_u8 *buf, cmr_uint first,
                                              cmr_uint last, cmr_uint position);
static cmr_int _s5k3p8sm_truly_buffer_init(cmr_handle otp_drv_handle);
static cmr_int _s5k3p8sm_truly_parse_awb_data(cmr_handle otp_drv_handle);
static cmr_int _s5k3p8sm_truly_parse_lsc_data(cmr_handle otp_drv_handle);
static cmr_int _s5k3p8sm_truly_parse_af_data(cmr_handle otp_drv_handle);
static cmr_int _s5k3p8sm_truly_parse_pdaf_data(cmr_handle otp_drv_handle);

static cmr_int _s5k3p8sm_truly_awb_calibration(cmr_handle otp_drv_handle);
static cmr_int _s5k3p8sm_truly_lsc_calibration(cmr_handle otp_drv_handle);
static cmr_int _s5k3p8sm_truly_pdaf_calibration(cmr_handle otp_drv_handle);

static cmr_int s5k3p8sm_truly_otp_drv_create(otp_drv_init_para_t *input_para,
                                            cmr_handle* sns_af_drv_handle);
static cmr_int s5k3p8sm_truly_otp_drv_delete(cmr_handle otp_drv_handle);
static cmr_int s5k3p8sm_truly_otp_drv_read(cmr_handle otp_drv_handle, void *p_data);
static cmr_int s5k3p8sm_truly_otp_drv_write(cmr_handle otp_drv_handle, void *p_data);
static cmr_int s5k3p8sm_truly_otp_drv_parse(cmr_handle otp_drv_handle, void *P_params);
static cmr_int s5k3p8sm_truly_otp_drv_calibration(cmr_handle otp_drv_handle);
static cmr_int s5k3p8sm_truly_otp_drv_ioctl(cmr_handle otp_drv_handle,
                                          cmr_uint cmd, void *params);

otp_drv_entry_t s5k3p8sm_truly_drv_entry = {
    .otp_cfg =
        {
            .cali_items =
                {
                    .is_self_cal = FALSE,
                    .is_dul_camc = FALSE,
                    .is_awbc = FALSE,
                    .is_lsc = FALSE,
                    .is_pdafc = FALSE,
                },
            .base_info_cfg =
                {
                    .is_lsc_drv_decompression = FALSE,
                    .compress_flag = OTP_COMPRESSED_FLAG,
                    .image_width = 4640,
                    .image_height = 3488,
                    .grid_width = 23,
                    .grid_height = 18,
                    .gain_width = GAIN_WIDTH,
                    .gain_height = GAIN_HEIGHT,
                },
        },
    .otp_ops =
        {
            .sensor_otp_create = s5k3p8sm_truly_otp_drv_create,
            .sensor_otp_delete = s5k3p8sm_truly_otp_drv_delete,
            .sensor_otp_read = s5k3p8sm_truly_otp_drv_read,
            .sensor_otp_write = s5k3p8sm_truly_otp_drv_write,
            .sensor_otp_parse = s5k3p8sm_truly_otp_drv_parse,
            .sensor_otp_calibration = s5k3p8sm_truly_otp_drv_calibration,
            .sensor_otp_ioctl = s5k3p8sm_truly_otp_drv_ioctl, /*expend*/
        },
};
