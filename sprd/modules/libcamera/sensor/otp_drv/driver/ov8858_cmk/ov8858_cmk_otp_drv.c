#include "ov8858_cmk_otp_drv.h"
#include <cutils/properties.h>

/** ov8858_cmk_section_checksum:
 *    @buff: address of otp buffer
 *    @offset: the start address of the section
 *    @data_count: data count of the section
 *    @check_sum_offset: the section checksum offset
 *Return: unsigned char.
 **/
static cmr_int _ov8858_cmk_section_checksum(cmr_u8 *buf, cmr_uint offset,
                                            cmr_uint data_count,
                                            cmr_uint check_sum_offset) {
    cmr_s32 ret = OTP_CAMERA_SUCCESS;
    cmr_u32 i = 0, sum = 0;

    OTP_LOGV("in");
    for (i = offset; i < offset + data_count; i++) {
        sum += buf[i];
    }
    if ((sum % 256) == buf[check_sum_offset]) {
        ret = OTP_CAMERA_SUCCESS;
    } else {
        ret = CMR_CAMERA_FAIL;
    }
    OTP_LOGI("out: offset:%ld, checksum:%d buf: %d", check_sum_offset, sum,
             buf[check_sum_offset]);
    return ret;
}

static cmr_int _ov8858_cmk_buffer_init(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    cmr_int otp_len;
    cmr_u8 *otp_data = NULL;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    /*include random and golden lsc otp data,add reserve*/
    otp_len = sizeof(otp_format_data_t) + LSC_FORMAT_SIZE + OTP_RESERVE_BUFFER;
    otp_data = sensor_otp_get_formatted_buffer(otp_len, otp_cxt->sensor_id);
    if (NULL == otp_data) {
        OTP_LOGE("malloc otp data buffer failed.\n");
        ret = CMR_CAMERA_FAIL;
    }

    otp_cxt->otp_data = (otp_format_data_t *)otp_data;
    OTP_LOGV("out");
    return ret;
}
static cmr_int _ov8858_cmk_parse_module_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_section_info_t *module_dat = &(otp_cxt->otp_data->module_dat);
    cmr_u8 *module_info = NULL;

    /*begain read raw data, save module info */
    module_info = (cmr_u8 *)(otp_cxt->otp_raw_data.buffer + MODULE_INFO_OFFSET);
    module_dat->rdm_info.buffer = module_info;
    module_dat->rdm_info.size = MODULE_INFO_CHECKSUM - MODULE_INFO_OFFSET;
    module_dat->gld_info.buffer = NULL;
    module_dat->gld_info.size = 0;

    return ret;
}
static cmr_int _ov8858_cmk_parse_af_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;

    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_section_info_t *af_cali_dat = &(otp_cxt->otp_data->af_cali_dat);
    cmr_u8 *af_src_dat = otp_cxt->otp_raw_data.buffer + AF_INFO_OFFSET;
    ret = _ov8858_cmk_section_checksum(
        otp_cxt->otp_raw_data.buffer, AF_INFO_OFFSET,
        AF_INFO_CHECKSUM - AF_INFO_OFFSET, AF_INFO_CHECKSUM);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("auto focus checksum error,parse failed");
        return ret;
    } else {
        af_cali_dat->rdm_info.buffer = af_src_dat;
        af_cali_dat->rdm_info.size = AF_INFO_CHECKSUM - AF_INFO_OFFSET;
        af_cali_dat->gld_info.buffer = NULL;
        af_cali_dat->gld_info.size = 0;
    }
    OTP_LOGV("out");
    return ret;
}

static cmr_int _ov8858_cmk_parse_awb_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    cmr_uint data_count_rdm = 0;
    cmr_uint data_count_gld = 0;

    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_section_info_t *awb_cali_dat = &(otp_cxt->otp_data->awb_cali_dat);
    cmr_u8 *awb_src_dat = otp_cxt->otp_raw_data.buffer + AWB_INFO_OFFSET;

    ret = _ov8858_cmk_section_checksum(
        otp_cxt->otp_raw_data.buffer, AWB_INFO_OFFSET,
        AWB_INFO_CHECKSUM - AWB_INFO_OFFSET, AWB_INFO_CHECKSUM);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("awb otp data checksum error,parse failed");
        return ret;
    } else {
        data_count_rdm = AWB_SECTION_NUM * AWB_INFO_SIZE;
        data_count_gld = AWB_SECTION_NUM * (sizeof(awb_target_packet_t));
        awb_cali_dat->rdm_info.buffer = awb_src_dat;
        awb_cali_dat->rdm_info.size = data_count_rdm;
        awb_cali_dat->gld_info.buffer = golden_awb;
        awb_cali_dat->gld_info.size = data_count_gld;
    }
    OTP_LOGV("out");
    return ret;
}

static cmr_int _ov8858_cmk_parse_lsc_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("in");

    cmr_u32 i = 0;
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    otp_section_info_t *lsc_dst = &(otp_cxt->otp_data->lsc_cali_dat);
    otp_section_info_t *opt_dst = &(otp_cxt->otp_data->opt_center_dat);

    ret = _ov8858_cmk_section_checksum(
        otp_cxt->otp_raw_data.buffer, OPTICAL_INFO_OFFSET,
        LSC_INFO_CHECKSUM - OPTICAL_INFO_OFFSET, LSC_INFO_CHECKSUM);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGI("lsc otp data checksum error,parse failed.\n");
    } else {
        /*optical center data*/
        cmr_u8 *opt_src = otp_cxt->otp_raw_data.buffer + OPTICAL_INFO_OFFSET;
        opt_dst->rdm_info.buffer = opt_src;
        opt_dst->rdm_info.size = LSC_INFO_OFFSET - OPTICAL_INFO_OFFSET;
        opt_dst->gld_info.buffer = NULL;
        opt_dst->gld_info.size = 0;

        /*lsc data*/
        cmr_u8 *rdm_dst = otp_cxt->otp_raw_data.buffer + LSC_INFO_OFFSET;
        lsc_dst->rdm_info.buffer = rdm_dst;
        lsc_dst->rdm_info.size = LSC_INFO_CHECKSUM - LSC_INFO_OFFSET;
        lsc_dst->gld_info.buffer = golden_lsc;
        lsc_dst->gld_info.size = LSC_INFO_CHECKSUM - LSC_INFO_OFFSET;
    }

    OTP_LOGV("out");
    return ret;
}

static cmr_int _ov8858_cmk_parse_pdaf_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;

    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    cmr_u8 *pdaf_src_dat = otp_cxt->otp_raw_data.buffer + PDAF_INFO_OFFSET;
    ret = _ov8858_cmk_section_checksum(
        otp_cxt->otp_raw_data.buffer, PDAF_INFO_OFFSET,
        PDAF_INFO_CHECKSUM - PDAF_INFO_OFFSET, PDAF_INFO_CHECKSUM);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGI("pdaf otp data checksum error,parse failed.\n");
        return ret;
    } else {
        otp_cxt->otp_data->pdaf_cali_dat.rdm_info.buffer = pdaf_src_dat;
        otp_cxt->otp_data->pdaf_cali_dat.rdm_info.size =
            PDAF_INFO_CHECKSUM - PDAF_INFO_OFFSET;
        otp_cxt->otp_data->pdaf_cali_dat.rdm_info.buffer = NULL;
        otp_cxt->otp_data->pdaf_cali_dat.rdm_info.size = 0;
    }

    OTP_LOGV("out");
    return ret;
}

static int _ov8858_cmk_parse_ae_data(void *otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_section_info_t *ae_cali_dat = &(otp_cxt->otp_data->ae_cali_dat);
    cmr_u8 *ae_src_dat = otp_cxt->otp_raw_data.buffer + AE_INFO_OFFSET;

    ae_cali_dat->rdm_info.buffer = ae_src_dat;
    ae_cali_dat->rdm_info.size = AE_INFO_CHECKSUM - AE_INFO_OFFSET;
    ae_cali_dat->gld_info.buffer = NULL;
    ae_cali_dat->gld_info.size = 0;

    return ret;
}

static cmr_int _ov8858_cmk_parse_dualcam_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;

    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    // otp_section_info_t *af_cali_dat = &(otp_cxt->otp_data->af_cali_dat);

    /*dualcam data*/
    cmr_u8 *dualcam_src_dat = otp_cxt->otp_raw_data.buffer + DUAL_INFO_OFFSET;
    cmr_u8 *vcm_src_dat = otp_cxt->otp_raw_data.buffer + VCM_INFO_OFFSET;
    ret = _ov8858_cmk_section_checksum(
        otp_cxt->otp_raw_data.buffer, DUAL_INFO_OFFSET,
        DUAL_INFO_CHECKSUM - DUAL_INFO_OFFSET, DUAL_INFO_CHECKSUM);

    switch (ret) {
    case OTP_CAMERA_SUCCESS:
        /*af_cali_dat->vcm_step = 0;
        af_cali_dat->vcm_step_min = vcm_src_dat[0];
        af_cali_dat->vcm_step_max = vcm_src_dat[1];*/
        otp_cxt->otp_data->dual_cam_cali_dat.rdm_info.buffer = dualcam_src_dat;
        otp_cxt->otp_data->dual_cam_cali_dat.rdm_info.size =
            DUAL_DATA_SIZE - VCM_DATA_SIZE;
        break;
    default:
        ret = _ov8858_cmk_section_checksum(
            otp_cxt->otp_raw_data.buffer, DUAL_INFO_OFFSET,
            DUAL_INFO_CHECKSUM - DUAL_INFO_OFFSET - VCM_DATA_SIZE,
            DUAL_INFO_CHECKSUM - VCM_DATA_SIZE);
        if (OTP_CAMERA_SUCCESS != ret) {
            OTP_LOGI("dualcamera otp data checksum error,parse failed.\n");
            return ret;
        } else {
            OTP_LOGI("vcm data is not in otp data.\n");
            // af_cali_dat->vcm_step = -1;
            otp_cxt->otp_data->dual_cam_cali_dat.rdm_info.buffer =
                dualcam_src_dat;
            otp_cxt->otp_data->dual_cam_cali_dat.rdm_info.size =
                DUAL_DATA_SIZE - VCM_DATA_SIZE;
        }
    }

    OTP_LOGV("out");
    return ret;
}

static cmr_int _ov8858_cmk_awb_calibration(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    OTP_LOGV("in");
    CHECK_PTR(otp_drv_handle);

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    /*
        otp_calib_items_t *cal_items =
            &(ov8858_cmk_drv_entry.otp_cfg.cali_items);
        awbcalib_data_t *awb_cali_dat = &(otp_cxt->otp_data->awb_cali_dat);
        int rg, bg, R_gain, G_gain, B_gain, Base_gain, temp, i;

        // calculate G gain

        R_gain = awb_cali_dat->awb_gld_info[0].rg_ratio * 1000 /
                 awb_cali_dat->awb_rdm_info[0].rg_ratio;
        B_gain = awb_cali_dat->awb_gld_info[0].bg_ratio * 1000 /
                 awb_cali_dat->awb_rdm_info[0].bg_ratio;
        G_gain = 1000;

        if (R_gain < 1000 || B_gain < 1000) {
            if (R_gain < B_gain)
                Base_gain = R_gain;
            else
                Base_gain = B_gain;
        } else {
            Base_gain = G_gain;
        }
        if (Base_gain != 0) {
            R_gain = 0x400 * R_gain / (Base_gain);
            B_gain = 0x400 * B_gain / (Base_gain);
            G_gain = 0x400 * G_gain / (Base_gain);
        } else {
            OTP_LOGE("awb parse problem!");
        }
        OTP_LOGI("r_Gain=0x%x,g_Gain=0x%x,b_Gain=0x%x\n", R_gain, G_gain,
       B_gain);

        if (cal_items->is_awbc_self_cal) {
            OTP_LOGD("Do wb calibration local");
        }*/
    OTP_LOGV("out");
    return ret;
}

static cmr_int _ov8858_cmk_lsc_calibration(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    OTP_LOGV("in");
    CHECK_PTR(otp_drv_handle);

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    OTP_LOGV("out");
    return ret;
}

static cmr_int _ov8858_cmk_pdaf_calibration(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    OTP_LOGI("in");
    CHECK_PTR(otp_drv_handle);

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    OTP_LOGV("out");
    return ret;
}

/*==================================================
*                External interface
====================================================*/

static cmr_int ov8858_cmk_otp_drv_create(otp_drv_init_para_t *input_para,
                                         cmr_handle *sns_af_drv_handle) {
    return sensor_otp_drv_create(input_para, sns_af_drv_handle);
}

static cmr_int ov8858_cmk_otp_drv_delete(cmr_handle otp_drv_handle) {
    return sensor_otp_drv_delete(otp_drv_handle);
}

static cmr_int ov8858_cmk_otp_drv_read(cmr_handle otp_drv_handle, void *param) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    cmr_u8 cmd_val[3];
    cmr_uint i = 0;
    char value[255];
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_params_t *otp_raw_data = &(otp_cxt->otp_raw_data);
    otp_params_t *p_data = (otp_params_t *)param;

    if (!otp_raw_data->buffer) {
        otp_raw_data->buffer =
            sensor_otp_get_raw_buffer(OTP_LEN, otp_cxt->sensor_id);
        if (NULL == otp_raw_data->buffer) {
            OTP_LOGE("malloc otp raw buffer failed\n");
            ret = OTP_CAMERA_FAIL;
            goto exit;
        }
        otp_raw_data->num_bytes = OTP_LEN;
        _ov8858_cmk_buffer_init(otp_drv_handle);
    }

    if (sensor_otp_get_buffer_state(otp_cxt->sensor_id)) {
        OTP_LOGI("otp raw data has read before,return directly");
        if (p_data) {
            p_data->buffer = otp_raw_data->buffer;
            p_data->num_bytes = otp_raw_data->num_bytes;
        }
        goto exit;
    }
    /*	for (i = 0; i < 8192; i++) {
            cmd_val[0] = ((OTP_START_ADDR + i) >> 8) & 0xff;
            cmd_val[1] = (OTP_START_ADDR + i) & 0xff;
            hw_sensor_read_i2c(otp_cxt->hw_handle, GT24C64A_I2C_ADDR,
                               (cmr_u8 *)&cmd_val[0], 2);
            otp_raw_data->buffer[i] = cmd_val[0];
        }*/
    /*in burst mode,otp data read from kernel one time*/
    //	memset(otp_raw_data->buffer, 0, 8192);
    otp_raw_data->buffer[0] = 0x00;
    otp_raw_data->buffer[1] = 0x00;
    ret = hw_sensor_read_i2c(otp_cxt->hw_handle, 0xb0 >> 1,
                             (cmr_u8 *)otp_raw_data->buffer,
                             SENSOR_I2C_REG_16BIT | OTP_LEN << 16);
    OTP_LOGI("malloc otp raw buffer OTP_LEN %d\n", OTP_LEN);

exit:
    property_get("debug.camera.save.otp.raw.data", value, "0");
    if (atoi(value) == 1) {
        if (sensor_otp_dump_raw_data(otp_raw_data->buffer, OTP_LEN,
                                     otp_cxt->dev_name))
            OTP_LOGE("dump failed");
    }
    OTP_LOGV("X");
    return ret;
}

static cmr_int ov8858_cmk_otp_drv_write(cmr_handle otp_drv_handle,
                                        void *p_data) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    CHECK_PTR(p_data);
    OTP_LOGI(" dualotp write in");
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_params_t *otp_write_data = (otp_params_t *)p_data;

    char value[PROPERTY_VALUE_MAX];
    property_get("debug.dualcamera.write.otp", value, "false");

    if (!strcmp(value, "false")) {
        OTP_LOGI("do not set dual camera otp write property,directly return.");
        return ret;
    }
    /*for before write*/
    cmr_int bSum = 0;
    /*for after  write*/
    cmr_int aSum = 0;
    /*open write permission*/
    ret = hw_sensor_grc_write_i2c(otp_cxt->hw_handle, GT24C64A_I2C_WR_ADDR,
                                  0x0000, 0x00, BITS_ADDR16_REG8);
    sensor_otp_dump_raw_data(otp_write_data->buffer, DUAL_DATA_SIZE, "write");

    if (NULL != otp_write_data->buffer) {
        unsigned char *buffer = (unsigned char *)malloc(DUAL_DATA_SIZE);
        otp_write_data->reg_addr = DUAL_INFO_OFFSET;
        int i;

        for (i = 0; i < (int)otp_write_data->num_bytes; i++) {
            bSum += otp_write_data->buffer[i];
            hw_sensor_grc_write_i2c(otp_cxt->hw_handle, GT24C64A_I2C_ADDR,
                                    otp_write_data->reg_addr + i,
                                    otp_write_data->buffer[i],
                                    BITS_ADDR16_REG8);
            buffer[i] = hw_sensor_grc_read_i2c(
                otp_cxt->hw_handle, GT24C64A_I2C_ADDR,
                otp_write_data->reg_addr + i, BITS_ADDR16_REG8);

            aSum += buffer[i];
        }

        /*for check reg write successful*/
        if (bSum == aSum) {
            hw_sensor_grc_write_i2c(otp_cxt->hw_handle, GT24C64A_I2C_ADDR,
                                    DUAL_INFO_CHECKSUM, (aSum % 255 + 1),
                                    BITS_ADDR16_REG8);

        } else {
            ret = OTP_CAMERA_FAIL;
            OTP_LOGI("crc failed bSum = 0x%lx,aSum=0x%lx", bSum, aSum);
        }
        OTP_LOGI("write %s dev otp,buffer:0x%s,size:%d", otp_cxt->dev_name,
                 otp_write_data->buffer, otp_write_data->num_bytes);
        free(buffer);
    } else {
        OTP_LOGE("ERROR:buffer pointer is null");
        ret = OTP_CAMERA_FAIL;
    }

    /*close write permission*/
    ret = hw_sensor_grc_write_i2c(otp_cxt->hw_handle, GT24C64A_I2C_RD_ADDR,
                                  0x0000, 0x00, BITS_ADDR16_REG8);
    OTP_LOGV("out");
    return ret;
}

static cmr_int ov8858_cmk_otp_drv_parse(cmr_handle otp_drv_handle,
                                        void *params) {
    cmr_int ret = OTP_CAMERA_SUCCESS;

    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("in");
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_base_info_cfg_t *base_info =
        &(ov8858_cmk_drv_entry.otp_cfg.base_info_cfg);
    otp_params_t *otp_raw_data = &(otp_cxt->otp_raw_data);

    if (sensor_otp_get_buffer_state(otp_cxt->sensor_id)) {
        OTP_LOGI("otp has parse before,return directly");
        return ret;
    } else if (otp_raw_data->buffer) {
        /*begain read raw data, save module info */
        OTP_LOGI("drver has read otp raw data,start parsed.");
        _ov8858_cmk_parse_module_data(otp_drv_handle);
        _ov8858_cmk_parse_af_data(otp_drv_handle);
        _ov8858_cmk_parse_awb_data(otp_drv_handle);
        _ov8858_cmk_parse_lsc_data(otp_drv_handle);
        _ov8858_cmk_parse_pdaf_data(otp_drv_handle);
        _ov8858_cmk_parse_ae_data(otp_drv_handle);
        _ov8858_cmk_parse_dualcam_data(otp_drv_handle);

        /*decompress lsc data if needed*/
        if ((base_info->compress_flag != GAIN_ORIGIN_BITS) &&
            base_info->is_lsc_drv_decompression == TRUE) {
            ret = sensor_otp_lsc_decompress(
                &ov8858_cmk_drv_entry.otp_cfg.base_info_cfg,
                &otp_cxt->otp_data->lsc_cali_dat);
            if (ret != OTP_CAMERA_SUCCESS) {
                return OTP_CAMERA_FAIL;
            }
        }
        sensor_otp_set_buffer_state(otp_cxt->sensor_id, 1); /*read to memory*/
    } else {
        OTP_LOGE("should read otp before parse");
        return OTP_CAMERA_FAIL;
    }

    OTP_LOGV("out");
    return ret;
}

static cmr_int ov8858_cmk_otp_drv_calibration(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("in");
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    otp_calib_items_t *cali_items = &(ov8858_cmk_drv_entry.otp_cfg.cali_items);

    if (cali_items) {
        if (cali_items->is_pdafc && cali_items->is_pdaf_self_cal)
            _ov8858_cmk_pdaf_calibration(otp_drv_handle);
        /*calibration at sensor or isp */
        if (cali_items->is_awbc && cali_items->is_awbc_self_cal)
            _ov8858_cmk_awb_calibration(otp_drv_handle);
        if (cali_items->is_lsc && cali_items->is_awbc_self_cal)
            _ov8858_cmk_lsc_calibration(otp_drv_handle);
    }
    /*If there are other items that need calibration, please add to here*/
    OTP_LOGV("Exit");
    return ret;
}

static cmr_int ov8858_cmk_compatible_convert(cmr_handle otp_drv_handle,
                                             void *p_data) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("in");
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_format_data_t *format_data = otp_cxt->otp_data;
    SENSOR_VAL_T *p_val = (SENSOR_VAL_T *)p_data;
    struct sensor_single_otp_info *single_otp = NULL;
    struct sensor_otp_cust_info *convert_data = NULL;

    convert_data = malloc(sizeof(struct sensor_otp_cust_info));
    if (NULL == convert_data) {
        OTP_LOGE("malloc otp convert_data failed.\n");
        return CMR_CAMERA_FAIL;
    }
    cmr_bzero(convert_data, sizeof(*convert_data));
    single_otp = &convert_data->single_otp;
    /*otp vendor type*/
    convert_data->otp_vendor = OTP_VENDOR_SINGLE_CAM_DUAL;
    /*otp raw data*/
    convert_data->total_otp.data_ptr = otp_cxt->otp_raw_data.buffer;
    convert_data->total_otp.size = otp_cxt->otp_raw_data.num_bytes;
    /*module data*/
    convert_data->dual_otp.master_module_info =
        (struct sensor_otp_section_info *)&format_data->module_dat;

    /*af convert*/
    convert_data->dual_otp.master_af_info =
        (struct sensor_otp_section_info *)&format_data->af_cali_dat;

    /*awb convert*/
    convert_data->dual_otp.master_iso_awb_info =
        (struct sensor_otp_section_info *)&format_data->awb_cali_dat;

    /*optical center*/
    convert_data->dual_otp.master_optical_center_info =
        (struct sensor_otp_section_info *)&format_data->opt_center_dat;

    /*lsc convert*/
    convert_data->dual_otp.master_lsc_info =
        (struct sensor_otp_section_info *)&format_data->lsc_cali_dat;

    /*ae convert*/
    convert_data->dual_otp.master_ae_info =
        (struct sensor_otp_section_info *)&format_data->ae_cali_dat;

    /*pdaf convert*/
    convert_data->dual_otp.master_pdaf_info =
        (struct sensor_otp_section_info *)&format_data->pdaf_cali_dat;

    /*dual camera*/
    convert_data->dual_otp.dual_flag = 1;
    convert_data->dual_otp.data_3d.data_ptr =
        otp_cxt->otp_raw_data.buffer + DUAL_INFO_OFFSET;
    convert_data->dual_otp.data_3d.size = DUAL_DATA_SIZE - VCM_DATA_SIZE;

    otp_cxt->compat_convert_data = convert_data;
    p_val->pval = convert_data;
    p_val->type = SENSOR_VAL_TYPE_PARSE_OTP;
    OTP_LOGV("out");
    return 0;
}
static cmr_int ov8858_cmk_get_vendor_id(cmr_handle otp_drv_handle,
                                        void *p_data) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    cmr_u8 *vendor_id = (cmr_u8 *)p_data;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("in");
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    *vendor_id = otp_cxt->otp_raw_data.buffer[0];
    OTP_LOGI("vendor_id %x", *vendor_id);
    return ret;
}

/*just for expend*/
static cmr_int ov8858_cmk_otp_drv_ioctl(cmr_handle otp_drv_handle, cmr_uint cmd,
                                        void *params) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("in");

    /*you can add you command*/
    switch (cmd) {
    case CMD_SNS_OTP_DATA_COMPATIBLE_CONVERT:
        ov8858_cmk_compatible_convert(otp_drv_handle, params);
        break;
    case CMD_SNS_OTP_GET_VENDOR_ID:
        ov8858_cmk_get_vendor_id(otp_drv_handle, params);
        break;
    default:
        break;
    }
    OTP_LOGV("out");
    return ret;
}
