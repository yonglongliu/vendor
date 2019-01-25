#include "gc5024_common_otp_drv.h"
#define SENSOR_OTP
/** gc5024_common_section_data_checksum:
 *    @buff: address of otp buffer
 *    @offset: the start address of the section
 *    @data_count: data count of the section
 *    @check_sum_offset: the section checksum offset
 *Return: unsigned char.
 **/
static cmr_int _gc5024_common_i2c_read_8bit(void *otp_drv_handle,
                                            uint16_t slave_addr,
                                            uint16_t memory_addr,
                                            uint8_t *memory_data) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    uint8_t cmd_val[5] = {0x00};
    uint16_t cmd_len = 0;

    // cmd_val[0] = memory_addr>>8;
    cmd_val[0] = memory_addr & 0xff;
    cmd_len = 1;

    ret = hw_Sensor_ReadI2C(otp_cxt->hw_handle, slave_addr,
                            (uint8_t *)&cmd_val[0], cmd_len);
    if (OTP_CAMERA_SUCCESS == ret) {
        *memory_data = cmd_val[0];
    }
    return ret;
}

static cmr_int _gc5024_common_i2c_write_8bit(void *otp_drv_handle,
                                             uint16_t slave_addr,
                                             uint16_t memory_addr,
                                             uint16_t memory_data) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    uint8_t cmd_val[5] = {0x00};
    uint16_t cmd_len = 0;

    cmd_val[0] = memory_addr & 0xff;
    cmd_val[1] = memory_data;
    cmd_len = 2;
    ret = hw_Sensor_WriteI2C(otp_cxt->hw_handle, slave_addr,
                             (uint8_t *)&cmd_val[0], cmd_len);

    return ret;
}

static cmr_u8 _gc5024_common_read_otp(cmr_handle otp_drv_handle, cmr_u8 addr)

{
    cmr_u8 value[2] = {0, 0};

    CHECK_PTR(otp_drv_handle);
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    struct sensor_ic_drv_cxt *sns_drv_cxt =
        (struct sensor_ic_drv_cxt *)otp_drv_handle;

    _gc5024_common_i2c_write_8bit(otp_drv_handle, SENSOR_I2C_ADDR, 0xfe, 0x00);
    _gc5024_common_i2c_write_8bit(otp_drv_handle, SENSOR_I2C_ADDR, 0xd5, addr);
    usleep(1 * 1000);
    _gc5024_common_i2c_write_8bit(otp_drv_handle, SENSOR_I2C_ADDR, 0xf3, 0x20);
    usleep(1 * 1000);
    _gc5024_common_i2c_read_8bit(otp_drv_handle, SENSOR_I2C_ADDR, 0xd7, value);

    return value[0];
}

static cmr_int _gc5024_common_otp_enable(cmr_handle otp_drv_handle,
                                         otp_state state)

{
    cmr_int ret = OTP_CAMERA_SUCCESS;
    uint8_t otp_clk, otp_en;
    CHECK_PTR(otp_drv_handle);
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    struct sensor_ic_drv_cxt *sns_drv_cxt =
        (struct sensor_ic_drv_cxt *)otp_drv_handle;

    _gc5024_common_i2c_write_8bit(otp_drv_handle, SENSOR_I2C_ADDR, 0xfe, 0x00);
    _gc5024_common_i2c_write_8bit(otp_drv_handle, SENSOR_I2C_ADDR, 0xfe, 0x00);
    _gc5024_common_i2c_write_8bit(otp_drv_handle, SENSOR_I2C_ADDR, 0xfe, 0x00);
    _gc5024_common_i2c_write_8bit(otp_drv_handle, SENSOR_I2C_ADDR, 0xf7, 0x01);
    _gc5024_common_i2c_write_8bit(otp_drv_handle, SENSOR_I2C_ADDR, 0xf8, 0x0e);
    _gc5024_common_i2c_write_8bit(otp_drv_handle, SENSOR_I2C_ADDR, 0xf9, 0xae);
    _gc5024_common_i2c_write_8bit(otp_drv_handle, SENSOR_I2C_ADDR, 0xfa, 0x84);
    _gc5024_common_i2c_write_8bit(otp_drv_handle, SENSOR_I2C_ADDR, 0xfc, 0xae);

    uint8_t otp_read_buf[2];
    _gc5024_common_i2c_read_8bit(otp_drv_handle, SENSOR_I2C_ADDR, 0xfa,
                                 &otp_read_buf[0]);
    _gc5024_common_i2c_read_8bit(otp_drv_handle, SENSOR_I2C_ADDR, 0xd4,
                                 &otp_read_buf[1]);
    otp_clk = otp_read_buf[0];
    otp_en = otp_read_buf[1];

    if (state) {
        otp_clk = otp_clk | 0x10;
        otp_en = otp_en | 0x80;
        usleep(5 * 1000);

        _gc5024_common_i2c_write_8bit(otp_drv_handle, SENSOR_I2C_ADDR, 0xfa,
                                      otp_clk); // 0xfa[6]:OTP_CLK_en
        _gc5024_common_i2c_write_8bit(otp_drv_handle, SENSOR_I2C_ADDR, 0xd4,
                                      otp_en); // 0xd4[7]:OTP_en

        OTP_LOGI("gc5024_OTP: Enable OTP!\n");
    } else {
        otp_en = otp_en & 0x7f;
        otp_clk = otp_clk & 0xef;
        usleep(5 * 1000);

        _gc5024_common_i2c_write_8bit(otp_drv_handle, SENSOR_I2C_ADDR, 0xd4,
                                      otp_en);
        _gc5024_common_i2c_write_8bit(otp_drv_handle, SENSOR_I2C_ADDR, 0xfa,
                                      otp_clk);

        OTP_LOGI("gc5024_OTP: Disable OTP!\n");
    }
    return ret;
}

static cmr_int _gc5024_common_section_checksum(unsigned char *buf,
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
    OTP_LOGI("_gc5024_otp_section_checks: offset:0x%x, sum:0x%x, checksum:0x%x",
             check_sum_offset, sum, buf[check_sum_offset]);
    return ret;
}

static cmr_int _gc5024_common_buffer_init(cmr_handle otp_drv_handle) {
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

static cmr_int _gc5024_common_parse_ae_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_section_info_t *ae_cali_dat = &(otp_cxt->otp_data->ae_cali_dat);
    /*TODO*/

    /*END*/

    OTP_LOGI("out");
    return ret;
}

static cmr_int _gc5024_common_parse_module_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    cmr_u32 moule_id = 0;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_section_info_t *module_dat = &(otp_cxt->otp_data->module_dat);
    /*begain read raw data, save module info */
    /*TODO*/
    cmr_u8 *flag = NULL;
    cmr_u8 *module_info_group = NULL;
    cmr_u8 index;

    flag = otp_cxt->otp_raw_data.buffer + MODULE_INFO_GROUP_FLAG_OFFSET;

    OTP_LOGI("gc5024_OTP : flag = 0x%x\n", *flag);

    if (*flag & 0x40) {
        index = 2;
        module_info_group = otp_cxt->otp_raw_data.buffer +
                            MODULE_INFO_GROUP_FLAG_OFFSET +
                            (0x40 - MODULE_INFO_GROUP_FLAG_OFFSET) / 8;

    } else if (*flag & 0x10) {
        index = 1;
        module_info_group = otp_cxt->otp_raw_data.buffer +
                            MODULE_INFO_GROUP_FLAG_OFFSET +
                            (0x08 - MODULE_INFO_GROUP_FLAG_OFFSET) / 8;
    } else {
        index = 0;
        module_info_group = NULL;
    }

    if (index > 0 && module_info_group) {
        moule_id = module_info_group[0];
        module_dat->rdm_info.buffer = module_info_group;
        module_dat->rdm_info.size = MODULE_INFO_SIZE;
        module_dat->rdm_info.buffer = NULL;
        module_dat->rdm_info.size = 0;
    } else {
        OTP_LOGI("gc5024_OTP : module info is invalid = 0x%x\n", *flag);
    }
    /*
    if (moule_id != MODULE_ID) {
        ret=OTP_CAMERA_FAIL;
    }
    */

    OTP_LOGI("out");
    return ret;
}

static cmr_int _gc5024_common_parse_af_data(cmr_handle otp_drv_handle) {
#if 0
    cmr_int ret = OTP_CAMERA_SUCCESS;

    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    afcalib_data_t *af_cali_dat = &(otp_cxt->otp_data->af_cali_dat);

	/*TODO*/
	cmr_u8 *af_src_dat = otp_cxt->otp_raw_data.buffer + AF_INFO_OFFSET;
	ret = _gc5024_common_section_checksum(
		otp_cxt->otp_raw_data.buffer, AF_INFO_OFFSET,
		AF_INFO_CHECKSUM - AF_INFO_OFFSET, AF_INFO_CHECKSUM);
	if (OTP_CAMERA_SUCCESS != ret) {
		OTP_LOGE("auto focus checksum error,parse failed");
		return ret;
	}
	af_cali_dat->infinity_dac = (af_src_dat[1] << 8) | af_src_dat[0];
	af_cali_dat->macro_dac = (af_src_dat[3] << 8) | af_src_dat[2];
	af_cali_dat->afc_direction = af_src_dat[4];

	/*END*/
    OTP_LOGD("af_infinity:0x%x,af_macro:0x%x,afc_direction:0x%x",
             af_cali_dat->infinity_dac, af_cali_dat->macro_dac,
             af_cali_dat->afc_direction);
    OTP_LOGI("out");
    return ret;
#endif
    return 0;
}

static cmr_int _gc5024_common_parse_oc_data(cmr_handle otp_drv_handle) {
#if 0
    cmr_int ret = OTP_CAMERA_SUCCESS;

    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    optical_center_t *opt_dst = &(otp_cxt->otp_data->opt_center_dat);

	/*TODO*/
	ret = _gc5024_common_section_checksum(
        otp_cxt->otp_raw_data.buffer, OPTICAL_INFO_OFFSET,
        LSC_INFO_CHECKSUM - OPTICAL_INFO_OFFSET, LSC_INFO_CHECKSUM);

	if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGI("oc otp data checksum error,parse failed.");
		return ret;
	}
	
	cmr_u8 *opt_src = otp_cxt->otp_raw_data.buffer + OPTICAL_INFO_OFFSET;
		opt_dst->R.x = (opt_src[1] << 8) | opt_src[0];
		opt_dst->R.y = (opt_src[3] << 8) | opt_src[2];
		opt_dst->GR.x = (opt_src[5] << 8) | opt_src[4];
		opt_dst->GR.y = (opt_src[7] << 8) | opt_src[6];
		opt_dst->GB.x = (opt_src[9] << 8) | opt_src[8];
		opt_dst->GB.y = (opt_src[11] << 8) | opt_src[10];
		opt_dst->B.x = (opt_src[13] << 8) | opt_src[12];
		opt_dst->B.y = (opt_src[15] << 8) | opt_src[14];

	/*END*/
  
	OTP_LOGD("optical_center:\n R=(0x%x,0x%x)\n GR=(0x%x,0x%x)\n "
				"GB=(0x%x,0x%x)\n B=(0x%x,0x%x)",
				opt_dst->R.x, opt_dst->R.y, opt_dst->GR.x, opt_dst->GR.y,
				opt_dst->GB.x, opt_dst->GB.y, opt_dst->B.x, opt_dst->B.y);
  
    OTP_LOGI("out");
    return ret;
#endif

    return 0;
}

static cmr_int _gc5024_common_parse_awb_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;

    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_section_info_t *awb_cali_dat = &(otp_cxt->otp_data->awb_cali_dat);

    /*TODO*/

    cmr_u8 *flag = NULL;
    cmr_u8 *awb_info_group = NULL;
    cmr_u8 index;

    flag = otp_cxt->otp_raw_data.buffer + MODULE_INFO_GROUP_FLAG_OFFSET;

    if (*flag & 0x04) {
        index = 2;
        awb_info_group = otp_cxt->otp_raw_data.buffer +
                         MODULE_INFO_GROUP_FLAG_OFFSET +
                         (0x88 - MODULE_INFO_GROUP_FLAG_OFFSET) / 8;
    } else if (*flag & 0x01) {
        index = 1;
        awb_info_group = otp_cxt->otp_raw_data.buffer +
                         MODULE_INFO_GROUP_FLAG_OFFSET +
                         (0x78 - MODULE_INFO_GROUP_FLAG_OFFSET) / 8;
    } else {
        index = 0;
        awb_info_group = NULL;
    }

    if (index > 0 && awb_info_group) {
        OTP_LOGI("gc5024_OTP_WB group%d is Valid !!\n", index);
        awb_cali_dat->rdm_info.buffer = awb_info_group;
        awb_cali_dat->rdm_info.size = AWB_INFO_SIZE;
    }
    /*
        // base ratio
        awb_cali_dat->awb_rdm_info[0].GrGb_ratio = 0x100;
        // golden
        awb_cali_dat->awb_gld_info[0].rg_ratio = RG_TYPICAL;
        awb_cali_dat->awb_gld_info[0].bg_ratio = BG_TYPICAL;
        awb_cali_dat->awb_gld_info[0].GrGb_ratio = 0x100;
    */
    OTP_LOGI("out");
    return ret;
}

static cmr_int _gc5024_common_parse_lsc_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_section_info_t *lsc_dst = &(otp_cxt->otp_data->lsc_cali_dat);

    return 0;

/*TODO*/
#if 0
	ret = _gc5024_common_section_checksum(
        otp_cxt->otp_raw_data.buffer, OPTICAL_INFO_OFFSET,
        LSC_INFO_CHECKSUM - OPTICAL_INFO_OFFSET, LSC_INFO_CHECKSUM);

	if (OTP_CAMERA_SUCCESS != ret) {
		OTP_LOGE("lsc checksum error,parse failed");
		return ret;
	}

	/*random data*/
	memcpy(rdm_dst, otp_cxt->otp_raw_data.buffer +LSC_INFO_OFFSET,
           LSC_INFO_CHECKSUM - LSC_INFO_OFFSET);
    lsc_dst->lsc_calib_random.length = LSC_INFO_CHECKSUM - LSC_INFO_OFFSET;

	/*END*/

    /*gold data*/
    memcpy(gld_dst, golden_lsc, LSC_INFO_CHECKSUM - LSC_INFO_OFFSET);
    lsc_dst->lsc_calib_golden.length = LSC_INFO_CHECKSUM - LSC_INFO_OFFSET;
	
		   
    OTP_LOGI("out");
    return ret;
#endif
}

static cmr_int _gc5024_common_parse_pdaf_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    /*TODO*/
    return 0;
#if 0
	cmr_u8 *pdaf_src_dat = otp_cxt->otp_raw_data.buffer + PDAF_INFO_OFFSET;
	ret = _gc5024_common_section_checksum(
	   otp_cxt->otp_raw_data.buffer, PDAF_INFO_OFFSET,
	   PDAF_INFO_CHECKSUM - PDAF_INFO_OFFSET, PDAF_INFO_CHECKSUM);
	if (OTP_CAMERA_SUCCESS != ret) {
	   OTP_LOGI("pdaf otp data checksum error,parse failed.\n");
	   return ret;
	} else {
	   otp_cxt->otp_data->pdaf_cali_dat.buffer = pdaf_src_dat;
	   otp_cxt->otp_data->pdaf_cali_dat.size =
		   PDAF_INFO_CHECKSUM - PDAF_INFO_OFFSET;
	}
	/*END*/

    OTP_LOGI("out");
    return ret;
#endif
}

static cmr_int _gc5024_common_parse_dualcam_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;

    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    /*TODO*/
    return 0;
#if 0
	cmr_u8 *dualcam_src_dat = otp_cxt->otp_raw_data.buffer + DUAL_INFO_OFFSET;
	otp_cxt->otp_data->dual_cam_cali_dat.buffer = dualcam_src_dat;
	otp_cxt->otp_data->dual_cam_cali_dat.size = DUAL_INFO_CHECKSUM - DUAL_INFO_OFFSET;

	/*END*/

    OTP_LOGI("out");
    return ret;
#endif
}

static cmr_int _gc5024_common_awb_calibration(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    OTP_LOGI("in");
    CHECK_PTR(otp_drv_handle);

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    /*    awbcalib_data_t *awb_cali_dat = &(otp_cxt->otp_data->awb_cali_dat);

        cmr_u16 r_gain_current = 0, g_gain_current = 0, b_gain_current = 0,
                base_gain = 0;
        cmr_u16 r_gain = 128, g_gain = 128, b_gain = 128;
        cmr_u16 rg_typical, bg_typical;

        rg_typical = awb_cali_dat->awb_gld_info[0].rg_ratio; // RG_TYPICAL;
        bg_typical = awb_cali_dat->awb_gld_info[0].bg_ratio; // BG_TYPICAL;

        if (0x01 == (awb_cali_dat->wb_flag & 0x01)) {
            r_gain_current =
                256 * rg_typical / awb_cali_dat->awb_rdm_info[0].rg_ratio;
            b_gain_current =
                256 * bg_typical / awb_cali_dat->awb_gld_info[0].bg_ratio;
            g_gain_current = 256;

            base_gain =
                (r_gain_current < b_gain_current) ? r_gain_current :
    b_gain_current;
            base_gain = (base_gain < g_gain_current) ? base_gain :
    g_gain_current;
            OTP_LOGI("gc5024_OTP_UPDATE_AWB:r_gain_current = 0x%x ,
    b_gain_current "
                     "= 0x%x , base_gain = 0x%x \n",
                     r_gain_current, b_gain_current, base_gain);

            r_gain = 0x80 * r_gain_current / base_gain;
            g_gain = 0x80 * g_gain_current / base_gain;
            b_gain = 0x80 * b_gain_current / base_gain;
            OTP_LOGI("gc5024_OTP_UPDATE_AWB:r_gain = 0x%x , g_gain = 0x%x ,
    b_gain "
                     "= 0x%x \n",
                     r_gain, g_gain, b_gain);

    #if 0
                    r_gain=0xff;
                    g_gain=0xff;
                    b_gain=0x80;
    #endif

    #if 0
                    r_gain=0x80;
                    g_gain=0x80;
                    b_gain=0x80;
    #endif

            _gc5024_common_i2c_write_8bit(otp_drv_handle, SENSOR_I2C_ADDR, 0xfe,
                                          0x00);
            _gc5024_common_i2c_write_8bit(otp_drv_handle, SENSOR_I2C_ADDR, 0xb8,
                                          g_gain);
            _gc5024_common_i2c_write_8bit(otp_drv_handle, SENSOR_I2C_ADDR, 0xb9,
                                          g_gain);
            _gc5024_common_i2c_write_8bit(otp_drv_handle, SENSOR_I2C_ADDR, 0xba,
                                          r_gain);
            _gc5024_common_i2c_write_8bit(otp_drv_handle, SENSOR_I2C_ADDR, 0xbb,
                                          r_gain);
            _gc5024_common_i2c_write_8bit(otp_drv_handle, SENSOR_I2C_ADDR, 0xbc,
                                          b_gain);
            _gc5024_common_i2c_write_8bit(otp_drv_handle, SENSOR_I2C_ADDR, 0xbd,
                                          b_gain);
            _gc5024_common_i2c_write_8bit(otp_drv_handle, SENSOR_I2C_ADDR, 0xbe,
                                          g_gain);
            _gc5024_common_i2c_write_8bit(otp_drv_handle, SENSOR_I2C_ADDR, 0xbf,
                                          g_gain);
        }
    */

    OTP_LOGI("out");
    return ret;
}
static cmr_int _gc5024_common_lsc_calibration(cmr_handle otp_drv_handle) {
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

static int _gc5024_common_pdaf_calibration(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    OTP_LOGI("in");
    CHECK_PTR(otp_drv_handle);

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    OTP_LOGI("out");
    return ret;
}

static cmr_int _gc5024_common_split_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_calib_items_t *cal_items =
        &(gc5024_common_drv_entry.otp_cfg.cali_items);

    ret = _gc5024_common_parse_module_data(otp_drv_handle);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("parse module info failed");
        return ret;
    }

    if (cal_items->is_self_cal) {
        ret = _gc5024_common_parse_oc_data(otp_drv_handle);
        if (OTP_CAMERA_SUCCESS != ret) {
            OTP_LOGE("parse oc data failed");
            return ret;
        }
    }

    if (cal_items->is_afc) {
        ret = _gc5024_common_parse_af_data(otp_drv_handle);
        if (OTP_CAMERA_SUCCESS != ret) {
            OTP_LOGE("parse af data failed");
            return ret;
        }
    }

    if (cal_items->is_awbc) {
        ret = _gc5024_common_parse_awb_data(otp_drv_handle);
        if (OTP_CAMERA_SUCCESS != ret) {
            OTP_LOGE("parse awb data failed");
            return ret;
        }
    }

    if (cal_items->is_lsc) {
        ret = _gc5024_common_parse_lsc_data(otp_drv_handle);
        if (OTP_CAMERA_SUCCESS != ret) {
            OTP_LOGE("parse lsc data failed");
            return ret;
        }
    }

    if (cal_items->is_pdafc) {
        ret = _gc5024_common_parse_pdaf_data(otp_drv_handle);
        if (OTP_CAMERA_SUCCESS != ret) {
            OTP_LOGE("parse pdaf data failed");
            return ret;
        }
    }

    if (cal_items->is_dul_camc) {
        ret = _gc5024_common_parse_ae_data(otp_drv_handle);
        if (OTP_CAMERA_SUCCESS != ret) {
            OTP_LOGE("parse ae data failed");
            return ret;
        }
        ret = _gc5024_common_parse_dualcam_data(otp_drv_handle);
        if (OTP_CAMERA_SUCCESS != ret) {
            OTP_LOGE("parse dualcam data failed");
            return ret;
        }
    }

    OTP_LOGI("out");
    return ret;
};

static cmr_int _gc5024_common_compatible_convert(cmr_handle otp_drv_handle,
                                                 void *p_data) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
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
    convert_data->otp_vendor = OTP_VENDOR_SINGLE;
    /*otp raw data*/
    convert_data->total_otp.data_ptr = otp_cxt->otp_raw_data.buffer;
    convert_data->total_otp.size = otp_cxt->otp_raw_data.num_bytes;
    /*module data*/
    single_otp->module_info =
        (struct sensor_otp_section_info *)&format_data->module_dat;

    /*af convert*/
    single_otp->af_info =
        (struct sensor_otp_section_info *)&format_data->af_cali_dat;

    /*awb convert*/
    single_otp->iso_awb_info =
        (struct sensor_otp_section_info *)&format_data->awb_cali_dat;

    /*optical center*/
    single_otp->optical_center_info =
        (struct sensor_otp_section_info *)&format_data->opt_center_dat;

    /*lsc convert*/
    single_otp->lsc_info =
        (struct sensor_otp_section_info *)&format_data->lsc_cali_dat;

#ifdef SENSOR_OTP
#else
    otp_cxt->compat_convert_data = convert_data;
#endif
    p_val->pval = convert_data;
    p_val->type = SENSOR_VAL_TYPE_PARSE_OTP;
    OTP_LOGI("out");
    return ret;
}

/*==================================================
*                  External interface
====================================================*/

static cmr_int gc5024_common_otp_drv_create(otp_drv_init_para_t *input_para,
                                            cmr_handle *sns_af_drv_handle) {
    OTP_LOGI("in");
    return sensor_otp_drv_create(input_para, sns_af_drv_handle);
}

static cmr_int gc5024_common_otp_drv_delete(cmr_handle otp_drv_handle) {

    OTP_LOGI("in");
    return sensor_otp_drv_delete(otp_drv_handle);
}

static cmr_int gc5024_common_otp_drv_read(cmr_handle otp_drv_handle,
                                          void *p_params) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    int i;
    CHECK_PTR(otp_drv_handle);
    // CHECK_PTR(p_params);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_params_t *otp_raw_data = &(otp_cxt->otp_raw_data);
    otp_params_t *p_data = (otp_params_t *)p_params;

    _gc5024_common_otp_enable(otp_drv_handle, otp_open);
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
        _gc5024_common_buffer_init(otp_drv_handle);
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
    //_gc5024_common_otp_enable(otp_drv_handle, otp_open);

    for (i = 0; i < OTP_RAW_DATA_LEN; i++) {
        // ret=_gc5024_common_i2c_read_8bit(otp_drv_handle,OTP_I2C_ADDR,OTP_START_ADDR+i*8,buffer+i);
        otp_raw_data->buffer[i] =
            _gc5024_common_read_otp(otp_drv_handle, OTP_START_ADDR + i * 8);

        OTP_LOGI("otp address, value: [0x%x,0x%x]", OTP_START_ADDR + i * 8,
                 otp_raw_data->buffer[i]);
    }
    /*END*/

    sensor_otp_dump_raw_data(otp_cxt->otp_raw_data.buffer, OTP_RAW_DATA_LEN,
                             otp_cxt->dev_name);
exit:
    OTP_LOGI("out");
    return ret;
}

static cmr_int gc5024_common_otp_drv_write(cmr_handle otp_drv_handle,
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

static cmr_int gc5024_common_otp_drv_parse(cmr_handle otp_drv_handle,
                                           void *p_params) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    // CHECK_PTR(p_params);
    OTP_LOGI("gc5024_common_otp_drv_parse");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_base_info_cfg_t *base_info =
        &(gc5024_common_drv_entry.otp_cfg.base_info_cfg);
    otp_params_t *otp_raw_data = &(otp_cxt->otp_raw_data);

    if (sensor_otp_get_buffer_state(otp_cxt->sensor_id)) {
        OTP_LOGI("otp has parse before,return directly");
        return ret;
    }

    if (!otp_raw_data->buffer) {
        OTP_LOGI("should read otp before parse");
        ret = gc5024_common_otp_drv_read(otp_drv_handle, NULL);
        if (ret != OTP_CAMERA_SUCCESS) {
            return OTP_CAMERA_FAIL;
        }
    }

    /*begin read raw data, save module info */
    OTP_LOGI("drver has read otp raw data,start parsed.");

    ret = _gc5024_common_split_data(otp_drv_handle);
    if (ret != OTP_CAMERA_SUCCESS) {
        return OTP_CAMERA_FAIL;
    }

    /*decompress lsc data if needed*/
    if ((base_info->compress_flag != GAIN_ORIGIN_BITS) &&
        base_info->is_lsc_drv_decompression == TRUE) {
        ret = sensor_otp_lsc_decompress(
            &gc5024_common_drv_entry.otp_cfg.base_info_cfg,
            &otp_cxt->otp_data->lsc_cali_dat);
        if (ret != OTP_CAMERA_SUCCESS) {
            return OTP_CAMERA_FAIL;
        }
    }
    sensor_otp_set_buffer_state(otp_cxt->sensor_id, 1); /*read to memory*/

    _gc5024_common_otp_enable(otp_drv_handle, otp_close);

    OTP_LOGI("out");
    return ret;
}

static cmr_int gc5024_common_otp_drv_calibration(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    _gc5024_common_otp_enable(otp_drv_handle, otp_close);

    otp_calib_items_t *cali_items =
        &(gc5024_common_drv_entry.otp_cfg.cali_items);

    if (cali_items->is_pdafc && cali_items->is_pdaf_self_cal) {
        _gc5024_common_pdaf_calibration(otp_drv_handle);
    }
    /*calibration at sensor or isp */
    if (cali_items->is_awbc && cali_items->is_awbc_self_cal) {
        _gc5024_common_awb_calibration(otp_drv_handle);
    }

    if (cali_items->is_lsc && cali_items->is_awbc_self_cal) {
        _gc5024_common_lsc_calibration(otp_drv_handle);
    }
    /*If there are other items that need calibration, please add to here*/

    OTP_LOGI("out");
    return ret;
}

/*just for expend*/
static cmr_int gc5024_common_otp_drv_ioctl(cmr_handle otp_drv_handle,
                                           cmr_uint cmd, void *params) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    /*you can add you command*/
    switch (cmd) {
    case CMD_SNS_OTP_DATA_COMPATIBLE_CONVERT:
        _gc5024_common_compatible_convert(otp_drv_handle, params);
        break;
    default:
        break;
    }
    OTP_LOGI("out");
    return ret;
}
