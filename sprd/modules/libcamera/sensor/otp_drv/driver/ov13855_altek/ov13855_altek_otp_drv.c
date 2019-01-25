#include "ov13855_altek_otp_drv.h"

/** ov13855_altek_section_data_checksum:
 *    @buff: address of otp buffer
 *    @offset: the start address of the section
 *    @data_count: data count of the section
 *    @check_sum_offset: the section checksum offset
 *Return: unsigned char.
 **/
static cmr_int _ov13855_altek_i2c_read(void *otp_drv_handle,
                                       uint16_t slave_addr,
                                       uint16_t memory_addr,
                                       uint8_t *memory_data) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    uint8_t cmd_val[5] = {0x00};
    uint16_t cmd_len = 0;

    cmd_val[0] = memory_addr >> 8;
    cmd_val[1] = memory_addr & 0xff;
    cmd_len = 2;

    ret = hw_Sensor_ReadI2C(otp_cxt->hw_handle, slave_addr,
                            (uint8_t *)&cmd_val[0], cmd_len);
    if (OTP_CAMERA_SUCCESS == ret) {
        *memory_data = cmd_val[0];
    }
    return ret;
}

static cmr_int _ov13855_altek_i2c_write(void *otp_drv_handle,
                                        uint16_t slave_addr,
                                        uint16_t memory_addr,
                                        uint16_t memory_data) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    uint8_t cmd_val[5] = {0x00};
    uint16_t cmd_len = 0;

    cmd_val[0] = memory_addr >> 8;
    cmd_val[1] = memory_addr & 0xff;
    cmd_val[2] = memory_data;
    cmd_len = 3;
    ret = hw_Sensor_WriteI2C(otp_cxt->hw_handle, slave_addr,
                             (uint8_t *)&cmd_val[0], cmd_len);

    return ret;
}
static cmr_int _ov13855_altek_section_checksum(unsigned char *buf,
                                               unsigned int offset,
                                               unsigned int data_count,
                                               unsigned int check_sum_offset) {
    uint32_t ret = OTP_CAMERA_SUCCESS;
    uint32_t i = 0, sum = 0;

    OTP_LOGI("in");
    for (i = offset; i < offset + data_count; i++) {
        sum += buf[i];
    }
    if (((sum % 256)) == buf[check_sum_offset]) {
        ret = OTP_CAMERA_SUCCESS;
    } else {
        ret = OTP_CAMERA_FAIL;
    }
    OTP_LOGI("out: offset:0x%x, sum:0x%x, checksum:0x%x", check_sum_offset, sum,
             buf[check_sum_offset]);
    return ret;
}

static cmr_int _ov13855_altek_buffer_init(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    cmr_int otp_len;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    /*include random and golden lsc otp data,add reserve*/
    otp_len = sizeof(otp_format_data_t) + LSC_FORMAT_SIZE + OTP_RESERVE_BUFFER +
              DUAL_DATA_SIZE;
    otp_format_data_t *otp_data =
        (otp_format_data_t *)sensor_otp_get_formatted_buffer(
            otp_len, otp_cxt->sensor_id);
    if (NULL == otp_data) {
        OTP_LOGE("malloc otp data buffer failed.\n");
        ret = OTP_CAMERA_FAIL;
    }
    otp_cxt->otp_data = otp_data;
    OTP_LOGI("out");
    return ret;
}

static cmr_int _ov13855_altek_parse_ae_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_section_info_t *ae_cali_dat = &(otp_cxt->otp_data->ae_cali_dat);

    /*TODO*/

    cmr_u8 *ae_src_dat = otp_cxt->otp_raw_data.buffer + AE_INFO_OFFSET;

    // for ae calibration
    ae_cali_dat->rdm_info.buffer = ae_src_dat;
    ae_cali_dat->rdm_info.size = AE_INFO_CHECKSUM - AE_INFO_OFFSET;
    ae_cali_dat->gld_info.buffer = NULL;
    ae_cali_dat->gld_info.size = 0;

    /*END*/

    OTP_LOGI("out");
    return ret;
}

static cmr_int _ov13855_altek_parse_module_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    cmr_u16 vendor_id;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_section_info_t *module_dat = &(otp_cxt->otp_data->module_dat);
    /*begain read raw data, save module info */
    /*TODO*/
    cmr_u8 *module_info = NULL;

    module_info = otp_cxt->otp_raw_data.buffer + MODULE_INFO_OFFSET;
    vendor_id = module_info[0];
    module_dat->rdm_info.buffer = module_info;
    module_dat->rdm_info.size = MODULE_INFO_CHECKSUM - MODULE_INFO_OFFSET;
    module_dat->gld_info.buffer = NULL;
    module_dat->gld_info.size = 0;

    if (vendor_id != MODULE_VENDOR_ID) {
        ret = OTP_CAMERA_FAIL;
    }

    OTP_LOGI("out");
    return ret;
}

static cmr_int _ov13855_altek_parse_af_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;

    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_section_info_t *af_cali_dat = &(otp_cxt->otp_data->af_cali_dat);

    /*TODO*/

    cmr_u8 *af_src_dat = otp_cxt->otp_raw_data.buffer + AF_INFO_OFFSET;
    ret = _ov13855_altek_section_checksum(
        otp_cxt->otp_raw_data.buffer, AF_INFO_OFFSET,
        AF_INFO_CHECKSUM - AF_INFO_OFFSET, AF_INFO_CHECKSUM);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("auto focus checksum error,parse failed");
        return ret;
    }
    af_cali_dat->rdm_info.buffer = af_src_dat;
    af_cali_dat->rdm_info.size = AF_INFO_CHECKSUM - AF_INFO_OFFSET;
    af_cali_dat->gld_info.buffer = NULL;
    af_cali_dat->gld_info.size = 0;

    OTP_LOGI("out");
    return ret;
}

static cmr_int _ov13855_altek_parse_oc_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_section_info_t *opt_dst = &(otp_cxt->otp_data->opt_center_dat);
    ret = _ov13855_altek_section_checksum(
        otp_cxt->otp_raw_data.buffer, OPTICAL_INFO_OFFSET,
        LSC_INFO_CHECKSUM - OPTICAL_INFO_OFFSET, LSC_INFO_CHECKSUM);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGI("oc otp data checksum error,parse failed.");
        return ret;
    }
    cmr_u8 *opt_src = otp_cxt->otp_raw_data.buffer + OPTICAL_INFO_OFFSET;
    opt_dst->rdm_info.buffer = opt_src;
    opt_dst->rdm_info.size = LSC_INFO_OFFSET - OPTICAL_INFO_OFFSET;
    opt_dst->gld_info.buffer = NULL;
    opt_dst->gld_info.size = 0;

    OTP_LOGI("out");
    return ret;
}
static cmr_int _ov13855_altek_parse_awb_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    cmr_uint data_count_rdm = 0;
    cmr_uint data_count_gld = 0;

    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_section_info_t *awb_cali_dat = &(otp_cxt->otp_data->awb_cali_dat);

    /*TODO*/

    cmr_u8 *awb_src_dat = otp_cxt->otp_raw_data.buffer + AWB_INFO_OFFSET;

    ret = _ov13855_altek_section_checksum(
        otp_cxt->otp_raw_data.buffer, AWB_INFO_OFFSET,
        AWB_INFO_CHECKSUM - AWB_INFO_OFFSET, AWB_INFO_CHECKSUM);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("awb otp data checksum error,parse failed");
        return ret;
    }
    OTP_LOGI("awb section count:0x%x", AWB_SECTION_NUM);
    data_count_rdm = AWB_SECTION_NUM * AWB_INFO_SIZE;
    data_count_gld = AWB_SECTION_NUM * (sizeof(awb_target_packet_t));
    awb_cali_dat->rdm_info.buffer = awb_src_dat;
    awb_cali_dat->rdm_info.size = data_count_rdm;
    awb_cali_dat->gld_info.buffer = golden_awb;
    awb_cali_dat->gld_info.size = data_count_gld;

    OTP_LOGI("out");
    return ret;
}

static cmr_int _ov13855_altek_parse_lsc_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_section_info_t *lsc_dst = &(otp_cxt->otp_data->lsc_cali_dat);

    /*TODO*/

    ret = _ov13855_altek_section_checksum(
        otp_cxt->otp_raw_data.buffer, OPTICAL_INFO_OFFSET,
        LSC_INFO_CHECKSUM - OPTICAL_INFO_OFFSET, LSC_INFO_CHECKSUM);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("lsc checksum error,parse failed");
        return ret;
    }

    cmr_u8 *rdm_dst = otp_cxt->otp_raw_data.buffer + LSC_INFO_OFFSET;
    lsc_dst->rdm_info.buffer = rdm_dst;
    lsc_dst->rdm_info.size = LSC_INFO_CHECKSUM - LSC_INFO_OFFSET;
    lsc_dst->gld_info.buffer = golden_lsc;
    lsc_dst->gld_info.size = LSC_INFO_CHECKSUM - LSC_INFO_OFFSET;

    OTP_LOGI("out");
    return ret;
}

static cmr_int _ov13855_altek_parse_pdaf_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    /*TODO*/

    cmr_u8 *pdaf_src_dat = otp_cxt->otp_raw_data.buffer + PDAF_INFO_OFFSET;
    ret = _ov13855_altek_section_checksum(
        otp_cxt->otp_raw_data.buffer, PDAF_INFO_OFFSET,
        PDAF_INFO_CHECKSUM - PDAF_INFO_OFFSET, PDAF_INFO_CHECKSUM);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGI("pdaf otp data checksum error,parse failed.\n");
        return ret;
    } else {
        otp_cxt->otp_data->pdaf_cali_dat.rdm_info.buffer = pdaf_src_dat;
        otp_cxt->otp_data->pdaf_cali_dat.rdm_info.size =
            PDAF_INFO_CHECKSUM - PDAF_INFO_OFFSET;
        otp_cxt->otp_data->pdaf_cali_dat.gld_info.buffer = NULL;
        otp_cxt->otp_data->pdaf_cali_dat.gld_info.size = 0;
    }

    /*END*/

    OTP_LOGI("out");
    return ret;
}

static cmr_int _ov13855_altek_parse_dualcam_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;

    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    /*TODO*/

    /*dualcam data*/
    cmr_u8 *dualcam_src_dat = otp_cxt->otp_raw_data.buffer + DUAL_INFO_OFFSET;
    ret = _ov13855_altek_section_checksum(
        otp_cxt->otp_raw_data.buffer, DUAL_INFO_OFFSET,
        DUAL_INFO_CHECKSUM - DUAL_INFO_OFFSET, DUAL_INFO_CHECKSUM);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGI("dualcam otp data checksum error,parse failed.\n");
        otp_cxt->otp_data->dual_cam_cali_dat.rdm_info.buffer = NULL;
        otp_cxt->otp_data->dual_cam_cali_dat.rdm_info.size = 0;
        return ret;
    } else {
        otp_cxt->otp_data->dual_cam_cali_dat.rdm_info.buffer = dualcam_src_dat;
        otp_cxt->otp_data->dual_cam_cali_dat.rdm_info.size =
            DUAL_INFO_CHECKSUM - DUAL_INFO_OFFSET;
    }
    otp_cxt->otp_data->dual_cam_cali_dat.gld_info.buffer = NULL;
    otp_cxt->otp_data->dual_cam_cali_dat.gld_info.size = 0;

    OTP_LOGI("out");
    return ret;
}

static cmr_int _ov13855_altek_parse_altek_ip_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;

    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    cmr_u8 *altek_src_dat = otp_cxt->otp_raw_data.buffer + ALTEK_INFO_OFFSET;
    ret = _ov13855_altek_section_checksum(
        otp_cxt->otp_raw_data.buffer, ALTEK_INFO_OFFSET,
        ALTEK_INFO_CHECKSUM - ALTEK_INFO_OFFSET, ALTEK_INFO_CHECKSUM);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGI("altek otp data checksum error,parse failed.\n");
        otp_cxt->otp_data->third_cali_dat.rdm_info.buffer = NULL;
        otp_cxt->otp_data->third_cali_dat.rdm_info.size = 0;
        return ret;
    } else {
        otp_cxt->otp_data->third_cali_dat.rdm_info.buffer = altek_src_dat;
        otp_cxt->otp_data->third_cali_dat.rdm_info.size =
            ALTEK_INFO_CHECKSUM - ALTEK_INFO_OFFSET;
    }
    otp_cxt->otp_data->third_cali_dat.gld_info.buffer = NULL;
    otp_cxt->otp_data->third_cali_dat.gld_info.size = 0;
    OTP_LOGI("out");
    return ret;
}

static cmr_int _ov13855_altek_awb_calibration(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    OTP_LOGI("in");
    CHECK_PTR(otp_drv_handle);

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_section_info_t *awb_cali_dat = &(otp_cxt->otp_data->awb_cali_dat);

    /*TODO*/

    /*END*/

    OTP_LOGI("out");
    return ret;
}
static cmr_int _ov13855_altek_lsc_calibration(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    OTP_LOGI("in");
    CHECK_PTR(otp_drv_handle);
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_section_info_t *lsc_dst = &(otp_cxt->otp_data->lsc_cali_dat);

    /*TODO*/

    /*END*/
    OTP_LOGI("out");
    return ret;
}

static int _ov13855_altek_pdaf_calibration(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    OTP_LOGI("in");
    CHECK_PTR(otp_drv_handle);

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    OTP_LOGI("out");
    return ret;
}

static cmr_int _ov13855_altek_split_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_calib_items_t *cal_items =
        &(ov13855_altek_drv_entry.otp_cfg.cali_items);

    ret = _ov13855_altek_parse_module_data(otp_drv_handle);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("parse module info failed");
        return ret;
    }
    if (cal_items->is_self_cal) {
        ret = _ov13855_altek_parse_oc_data(otp_drv_handle);
        if (OTP_CAMERA_SUCCESS != ret) {
            OTP_LOGE("parse oc data failed");
            return ret;
        }
    }

    if (cal_items->is_afc) {
        ret = _ov13855_altek_parse_af_data(otp_drv_handle);
        if (OTP_CAMERA_SUCCESS != ret) {
            OTP_LOGE("parse af data failed");
            return ret;
        }
    }

    if (cal_items->is_awbc) {
        ret = _ov13855_altek_parse_awb_data(otp_drv_handle);
        if (OTP_CAMERA_SUCCESS != ret) {
            OTP_LOGE("parse awb data failed");
            return ret;
        }
    }

    if (cal_items->is_lsc) {
        ret = _ov13855_altek_parse_lsc_data(otp_drv_handle);
        if (OTP_CAMERA_SUCCESS != ret) {
            OTP_LOGE("parse lsc data failed");
            return ret;
        }
    }

    if (cal_items->is_pdafc) {
        ret = _ov13855_altek_parse_pdaf_data(otp_drv_handle);
        if (OTP_CAMERA_SUCCESS != ret) {
            OTP_LOGE("parse pdaf data failed");
            return ret;
        }
    }

    if (cal_items->is_dul_camc) {
        ret = _ov13855_altek_parse_ae_data(otp_drv_handle);
        if (OTP_CAMERA_SUCCESS != ret) {
            OTP_LOGE("parse ae data failed");
            return ret;
        }
        ret = _ov13855_altek_parse_dualcam_data(otp_drv_handle);
        if (OTP_CAMERA_SUCCESS != ret) {
            OTP_LOGE("parse dualcam data failed");
            return ret;
        }
        ret = _ov13855_altek_parse_altek_ip_data(otp_drv_handle);
        if (OTP_CAMERA_SUCCESS != ret) {
            OTP_LOGE("parse altek dualcam data failed");
            return ret;
        }
    }

    OTP_LOGI("out");
    return ret;
};

static cmr_int _ov13855_altek_compatible_convert(cmr_handle otp_drv_handle,
                                                 void *p_data) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    char value[255];
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_format_data_t *format_data = otp_cxt->otp_data;
    SENSOR_VAL_T *p_val = (SENSOR_VAL_T *)p_data;
    struct sensor_single_otp_info *single_otp = NULL;
    struct sensor_otp_cust_info *convert_data = NULL;

    convert_data = malloc(sizeof(struct sensor_otp_cust_info));
    cmr_bzero(convert_data, sizeof(*convert_data));
    single_otp = &convert_data->single_otp;
    /*otp vendor type*/
    convert_data->otp_vendor = OTP_VENDOR_DUAL_CAM_DUAL;
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
    property_get("persist.sys.cam.api.version", value, "0");
    convert_data->dual_otp.dual_flag = 1;
    if (atoi(value) == 0) {
        convert_data->dual_otp.data_3d.data_ptr =
            format_data->dual_cam_cali_dat.rdm_info.buffer;
        convert_data->dual_otp.data_3d.size =
            format_data->dual_cam_cali_dat.rdm_info.size;
        convert_data->dual_otp.data_3d.dualcam_cali_lib_type = OTP_CALI_SPRD;
    } else {
        convert_data->dual_otp.data_3d.data_ptr = otp_cxt->otp_raw_data.buffer;
        convert_data->dual_otp.data_3d.size = otp_cxt->otp_raw_data.num_bytes;
        convert_data->dual_otp.data_3d.dualcam_cali_lib_type = OTP_CALI_ALTEK;
    }
    otp_cxt->compat_convert_data = convert_data;
    p_val->pval = convert_data;
    p_val->type = SENSOR_VAL_TYPE_PARSE_OTP;
    OTP_LOGI("out");
    return ret;
}

/*==================================================
*                  External interface
====================================================*/

static cmr_int ov13855_altek_otp_drv_create(otp_drv_init_para_t *input_para,
                                            cmr_handle *sns_af_drv_handle) {
    OTP_LOGI("in");
    return sensor_otp_drv_create(input_para, sns_af_drv_handle);
}

static cmr_int ov13855_altek_otp_drv_delete(cmr_handle otp_drv_handle) {

    OTP_LOGI("in");
    return sensor_otp_drv_delete(otp_drv_handle);
}

static cmr_int ov13855_altek_otp_drv_read(cmr_handle otp_drv_handle,
                                          void *p_params) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    char value[255];
    CHECK_PTR(otp_drv_handle);
    // CHECK_PTR(p_params);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_params_t *otp_raw_data = &(otp_cxt->otp_raw_data);
    otp_params_t *p_data = (otp_params_t *)p_params;
    cmr_u8 *buffer = NULL;

    if (!otp_cxt->otp_raw_data.buffer) {
        /*when mobile power on , it will init*/
        otp_raw_data->buffer =
            sensor_otp_get_raw_buffer(OTP_RAW_DATA_LEN, otp_cxt->sensor_id);
        if (NULL == otp_raw_data->buffer) {
            OTP_LOGE("malloc otp raw buffer failed\n");
            ret = OTP_CAMERA_FAIL;
            goto exit;
        }
        otp_raw_data->num_bytes = OTP_RAW_DATA_LEN;
        _ov13855_altek_buffer_init(otp_drv_handle);
    }

    if (sensor_otp_get_buffer_state(otp_cxt->sensor_id)) {
        OTP_LOGI("otp raw data has read before,return directly");
        if (p_data) {
            p_data->buffer = otp_raw_data->buffer;
            p_data->num_bytes = otp_raw_data->num_bytes;
        }
        goto exit;
    }
    /*start read otp data one time*/
    /*TODO*/
    otp_raw_data->buffer[0] = 0;
    otp_raw_data->buffer[1] = 0;
    ret = hw_sensor_read_i2c(otp_cxt->hw_handle, OTP_I2C_ADDR,
                             (cmr_u8 *)otp_raw_data->buffer,
                             SENSOR_I2C_REG_16BIT | OTP_RAW_DATA_LEN << 16);
    /*END*/
    if (OTP_CAMERA_SUCCESS == ret) {
        if (ov13855_altek_drv_entry.otp_dep.is_depend_relation &&
            (ov13855_altek_drv_entry.otp_dep.depend_sensor_id !=
             otp_cxt->sensor_id)) {
            buffer = sensor_otp_get_raw_buffer(
                OTP_RAW_DATA_LEN,
                ov13855_altek_drv_entry.otp_dep.depend_sensor_id);
            if (buffer) {
                memcpy(buffer, otp_raw_data->buffer, OTP_RAW_DATA_LEN);
            } else {
                OTP_LOGE("get buffer is null");
            }
        }
        property_get("debug.camera.save.otp.raw.data", value, "0");
        if (atoi(value) == 1) {
            if (sensor_otp_dump_raw_data(otp_raw_data->buffer, OTP_RAW_DATA_LEN,
                                         otp_cxt->dev_name))
                OTP_LOGE("dump failed");
        }
    }
exit:
    OTP_LOGI("out");
    return ret;
}

static cmr_int ov13855_altek_otp_drv_write(cmr_handle otp_drv_handle,
                                           void *p_params) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    CHECK_PTR(p_params);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_params_t *otp_write_data = p_params;

    if (NULL != otp_write_data->buffer) {
        OTP_LOGI("write %s dev otp,buffer:0x%x,size:%d", otp_cxt->dev_name,
                 otp_write_data->buffer, otp_write_data->num_bytes);
        /*TODO*/

        /*END*/
    } else {
        OTP_LOGE("ERROR:buffer pointer is null");
        ret = OTP_CAMERA_FAIL;
    }
    OTP_LOGI("out");
    return ret;
}

static cmr_int ov13855_altek_otp_drv_parse(cmr_handle otp_drv_handle,
                                           void *p_params) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    // CHECK_PTR(p_params);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_base_info_cfg_t *base_info =
        &(ov13855_altek_drv_entry.otp_cfg.base_info_cfg);
    otp_params_t *otp_raw_data = &(otp_cxt->otp_raw_data);
    module_data_t *module_dat = &(otp_cxt->otp_data->module_dat);

    if (sensor_otp_get_buffer_state(otp_cxt->sensor_id)) {
        OTP_LOGI("otp has parse before,return directly");
        return ret;
    }

    if (!otp_raw_data->buffer) {
        OTP_LOGI("should read otp before parse");
        ret = ov13855_altek_otp_drv_read(otp_drv_handle, NULL);
        if (ret != OTP_CAMERA_SUCCESS) {
            return OTP_CAMERA_FAIL;
        }
    }

    /*begin read raw data, save module info */
    OTP_LOGI("drver has read otp raw data,start parsed.");

    ret = _ov13855_altek_split_data(otp_drv_handle);
    if (ret != OTP_CAMERA_SUCCESS) {
        return OTP_CAMERA_FAIL;
    }

    /*decompress lsc data if needed*/
    if ((base_info->compress_flag != GAIN_ORIGIN_BITS) &&
        base_info->is_lsc_drv_decompression == TRUE) {
        ret = sensor_otp_lsc_decompress(
            &ov13855_altek_drv_entry.otp_cfg.base_info_cfg,
            &otp_cxt->otp_data->lsc_cali_dat);
        if (ret != OTP_CAMERA_SUCCESS) {
            return OTP_CAMERA_FAIL;
        }
    }
    sensor_otp_set_buffer_state(otp_cxt->sensor_id, 1); /*read to memory*/

    OTP_LOGI("out");
    return ret;
}

static cmr_int ov13855_altek_otp_drv_calibration(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    otp_calib_items_t *cali_items =
        &(ov13855_altek_drv_entry.otp_cfg.cali_items);

    if (cali_items->is_pdafc && cali_items->is_pdaf_self_cal) {
        _ov13855_altek_pdaf_calibration(otp_drv_handle);
    }
    /*calibration at sensor or isp */
    if (cali_items->is_awbc && cali_items->is_awbc_self_cal) {
        _ov13855_altek_awb_calibration(otp_drv_handle);
    }

    if (cali_items->is_lsc && cali_items->is_awbc_self_cal) {
        _ov13855_altek_lsc_calibration(otp_drv_handle);
    }
    /*If there are other items that need calibration, please add to here*/

    OTP_LOGI("out");
    return ret;
}

/*just for expend*/
static cmr_int ov13855_altek_otp_drv_ioctl(cmr_handle otp_drv_handle,
                                           cmr_uint cmd, void *params) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    /*you can add you command*/
    switch (cmd) {
    case CMD_SNS_OTP_DATA_COMPATIBLE_CONVERT:
        _ov13855_altek_compatible_convert(otp_drv_handle, params);
        break;
    default:
        break;
    }
    OTP_LOGI("out");
    return ret;
}
