#define LOG_TAG "bu64241gwz"
#include "bu64241gwz.h"

static uint32_t _BU64241GWZ_set_motor_bestmode(cmr_handle sns_af_drv_handle) {
    struct sns_af_drv_cxt *af_drv_cxt =
        (struct sns_af_drv_cxt *)sns_af_drv_handle;
    CHECK_PTR(sns_af_drv_handle);

    uint16_t A_code = 80, B_code = 90;
    uint8_t rf = 0x0F, slew_rate = 0x02, stt = 0x01, str = 0x02;
    uint8_t cmd_val[2]; // set
    cmd_val[0] = 0xcc;
    cmd_val[1] = (rf << 3) | slew_rate;
    hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, (0x18 >> 1),
                       (uint8_t *)&cmd_val[0], 2);
    usleep(200);
    cmd_val[0] = 0xd4 | (A_code >> 8);
    cmd_val[1] = A_code & 0xff;
    hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, (0x18 >> 1),
                       (uint8_t *)&cmd_val[0], 2);
    usleep(200);
    cmd_val[0] = 0xdc | (B_code >> 8);
    cmd_val[1] = B_code & 0xff;
    hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, (0x18 >> 1),
                       (uint8_t *)&cmd_val[0], 2);
    usleep(200);
    cmd_val[0] = 0xe4;
    cmd_val[1] = (str << 5) | stt;
    hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, (0x18 >> 1),
                       (uint8_t *)&cmd_val[0], 2);

    CMR_LOGI("VCM mode set");
    return 0;
}

static uint32_t _BU64241GWZ_get_test_vcm_mode(cmr_handle sns_af_drv_handle) {
    struct sns_af_drv_cxt *af_drv_cxt =
        (struct sns_af_drv_cxt *)sns_af_drv_handle;
    CHECK_PTR(sns_af_drv_handle);

    uint16_t A_code = 80, B_code = 90, C_code = 0;
    uint8_t rf = 0x0F, slew_rate = 0x02, stt = 0x01, str = 0x02;
    uint8_t cmd_val[2];

    FILE *fp = NULL;
    fp = fopen("/data/misc/media/cur_vcm_info.txt", "wb");

    // read
    cmd_val[0] = 0xcc;
    hw_Sensor_ReadI2C(af_drv_cxt->hw_handle, (0x18 >> 1),
                      (uint8_t *)&cmd_val[0], 2);
    rf = cmd_val[1] >> 3;
    slew_rate = cmd_val[1] & 0x03;
    usleep(200);
    cmd_val[0] = 0xd4;
    hw_Sensor_ReadI2C(af_drv_cxt->hw_handle, (0x18 >> 1),
                      (uint8_t *)&cmd_val[0], 2);
    A_code = (cmd_val[0] & 0x03) << 8;
    A_code = A_code + cmd_val[1];
    usleep(200);
    cmd_val[0] = 0xdc;
    hw_Sensor_ReadI2C(af_drv_cxt->hw_handle, (0x18 >> 1),
                      (uint8_t *)&cmd_val[0], 2);
    B_code = (cmd_val[0] & 0x03) << 8;
    B_code = B_code + cmd_val[1];
    usleep(200);
    cmd_val[0] = 0xc4;
    hw_Sensor_ReadI2C(af_drv_cxt->hw_handle, (0x18 >> 1),
                      (uint8_t *)&cmd_val[0], 2);
    C_code = (cmd_val[0] & 0x03) << 8;
    C_code = C_code + cmd_val[1];
    usleep(200);
    cmd_val[0] = 0xe4;
    hw_Sensor_ReadI2C(af_drv_cxt->hw_handle, (0x18 >> 1),
                      (uint8_t *)&cmd_val[0], 2);
    str = cmd_val[1] >> 5;
    stt = cmd_val[1] & 0x1f;
    CMR_LOGI("VCM A_code B_code rf slew_rate stt str pos,%d %d %d %d %d %d %d",
             A_code, B_code, rf, slew_rate, stt, str, C_code);

    fprintf(fp,
            "VCM A_code B_code rf slew_rate stt str pos,%d %d %d %d %d %d %d",
            A_code, B_code, rf, slew_rate, stt, str, C_code);
    fflush(fp);
    fclose(fp);
    return 0;
}

static uint32_t _BU64241GWZ_set_test_vcm_mode(cmr_handle sns_af_drv_handle,
                                              char *vcm_mode) {
    struct sns_af_drv_cxt *af_drv_cxt =
        (struct sns_af_drv_cxt *)sns_af_drv_handle;
    CHECK_PTR(sns_af_drv_handle);

    uint16_t A_code = 80, B_code = 90;
    uint8_t rf = 0x0F, slew_rate = 0x02, stt = 0x01, str = 0x02;
    uint8_t cmd_val[2];
    char *p1 = vcm_mode;

    while (*p1 != '~' && *p1 != '\0')
        p1++;
    *p1++ = '\0';
    A_code = atoi(vcm_mode);
    vcm_mode = p1;
    while (*p1 != '~' && *p1 != '\0')
        p1++;
    *p1++ = '\0';
    B_code = atoi(vcm_mode);
    vcm_mode = p1;
    while (*p1 != '~' && *p1 != '\0')
        p1++;
    *p1++ = '\0';
    rf = atoi(vcm_mode);
    vcm_mode = p1;
    while (*p1 != '~' && *p1 != '\0')
        p1++;
    *p1++ = '\0';
    slew_rate = atoi(vcm_mode);
    vcm_mode = p1;
    while (*p1 != '~' && *p1 != '\0')
        p1++;
    *p1++ = '\0';
    stt = atoi(vcm_mode);
    vcm_mode = p1;
    while (*p1 != '~' && *p1 != '\0')
        p1++;
    *p1++ = '\0';
    str = atoi(vcm_mode);
    CMR_LOGI("VCM A_code B_code rf slew_rate stt str 1nd,%d %d %d %d %d %d",
             A_code, B_code, rf, slew_rate, stt, str);
    // set
    cmd_val[0] = 0xcc;
    cmd_val[1] = (rf << 3) | slew_rate;
    hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, (0x18 >> 1),
                       (uint8_t *)&cmd_val[0], 2);
    usleep(200);
    cmd_val[0] = 0xd4 | (A_code >> 8);
    cmd_val[1] = A_code & 0xff;
    hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, (0x18 >> 1),
                       (uint8_t *)&cmd_val[0], 2);
    usleep(200);
    cmd_val[0] = 0xdc | (B_code >> 8);
    cmd_val[1] = B_code & 0xff;
    hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, (0x18 >> 1),
                       (uint8_t *)&cmd_val[0], 2);
    usleep(200);
    cmd_val[0] = 0xe4;
    cmd_val[1] = (str << 5) | stt;
    hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, (0x18 >> 1),
                       (uint8_t *)&cmd_val[0], 2);
    // read
    cmd_val[0] = 0xcc;
    hw_Sensor_ReadI2C(af_drv_cxt->hw_handle, (0x18 >> 1),
                      (uint8_t *)&cmd_val[0], 2);
    rf = cmd_val[1] >> 3;
    slew_rate = cmd_val[1] & 0x03;
    usleep(200);
    cmd_val[0] = 0xd4;
    hw_Sensor_ReadI2C(af_drv_cxt->hw_handle, (0x18 >> 1),
                      (uint8_t *)&cmd_val[0], 2);
    A_code = (cmd_val[0] & 0x03) << 8;
    A_code = A_code + cmd_val[1];
    usleep(200);
    cmd_val[0] = 0xdc;
    hw_Sensor_ReadI2C(af_drv_cxt->hw_handle, (0x18 >> 1),
                      (uint8_t *)&cmd_val[0], 2);
    B_code = (cmd_val[0] & 0x03) << 8;
    B_code = B_code + cmd_val[1];
    usleep(200);
    cmd_val[0] = 0xe4;
    hw_Sensor_ReadI2C(af_drv_cxt->hw_handle, (0x18 >> 1),
                      (uint8_t *)&cmd_val[0], 2);
    str = cmd_val[1] >> 5;
    stt = cmd_val[1] & 0x1f;
    CMR_LOGI("VCM A_code B_code rf slew_rate stt str 2nd,%d %d %d %d %d %d",
             A_code, B_code, rf, slew_rate, stt, str);
    return 0;
}

static int BU64241GWZ_drv_create(struct af_drv_init_para *input_ptr,
                                 cmr_handle *sns_af_drv_handle) {
    cmr_int ret = AF_SUCCESS;
    CHECK_PTR(input_ptr);
    ret = af_drv_create(input_ptr, sns_af_drv_handle);
    if (ret != AF_SUCCESS) {
        ret = AF_FAIL;
    } else {
        ret = _BU64241GWZ_drv_init(*sns_af_drv_handle);
        if (ret != AF_SUCCESS)
            ret = AF_FAIL;
    }
    return ret;
}

static int BU64241GWZ_drv_delete(cmr_handle sns_af_drv_handle, void *param) {
    cmr_int ret = AF_SUCCESS;
    CHECK_PTR(sns_af_drv_handle);
    ret = af_drv_delete(sns_af_drv_handle, param);
    return ret;
}

static int BU64241GWZ_drv_set_pos(cmr_handle sns_af_drv_handle, uint16_t pos) {
    uint8_t cmd_val[2] = {((pos >> 8) & 0x03) | 0xc4, pos & 0xff};
    struct sns_af_drv_cxt *af_drv_cxt =
        (struct sns_af_drv_cxt *)sns_af_drv_handle;
    CHECK_PTR(sns_af_drv_handle);
    // set pos
    CMR_LOGI("BU64241GWZ_set_pos %d", pos);
    hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, (0x18 >> 1),
                       (uint8_t *)&cmd_val[0], 2);
    return 0;
}

static int BU64241GWZ_drv_get_pos(cmr_handle sns_af_drv_handle, uint16_t *pos) {
    uint8_t cmd_val[2] = {0xc4, 0x00};
    struct sns_af_drv_cxt *af_drv_cxt =
        (struct sns_af_drv_cxt *)sns_af_drv_handle;
    CHECK_PTR(sns_af_drv_handle);

    hw_Sensor_ReadI2C(af_drv_cxt->hw_handle, (0x18 >> 1),
                      (uint8_t *)&cmd_val[0], 2);
    *pos = (cmd_val[0] & 0x03) << 8;
    *pos += cmd_val[1];
    CMR_LOGI("vcm pos %d", *pos);
    return 0;
}

static int BU64241GWZ_drv_ioctl(cmr_handle sns_af_drv_handle, enum sns_cmd cmd,
                                void *param) {
    cmr_int ret = AF_SUCCESS;
    struct sns_af_drv_cxt *af_drv_cxt =
        (struct sns_af_drv_cxt *)sns_af_drv_handle;
    CHECK_PTR(sns_af_drv_handle);
    switch (cmd) {
    case CMD_SNS_AF_SET_BEST_MODE:
        _BU64241GWZ_set_motor_bestmode(sns_af_drv_handle);
        break;
    case CMD_SNS_AF_GET_TEST_MODE:
        _BU64241GWZ_get_test_vcm_mode(sns_af_drv_handle);
        break;
    case CMD_SNS_AF_SET_TEST_MODE:
        _BU64241GWZ_set_test_vcm_mode(sns_af_drv_handle, param);
        break;
    default:
        break;
    }
    return ret;
}

struct sns_af_drv_entry bu64241gwz_drv_entry = {
    .motor_avdd_val = SENSOR_AVDD_2800MV,
    .default_work_mode = 0,
    .af_ops =
        {
            .create = BU64241GWZ_drv_create,
            .delete = BU64241GWZ_drv_delete,
            .set_pos = BU64241GWZ_drv_set_pos,
            .get_pos = BU64241GWZ_drv_get_pos,
            .ioctl = BU64241GWZ_drv_ioctl,
        },
};

static int _BU64241GWZ_drv_init(cmr_handle sns_af_drv_handle) {
    struct sns_af_drv_cxt *af_drv_cxt =
        (struct sns_af_drv_cxt *)sns_af_drv_handle;
    CHECK_PTR(sns_af_drv_handle);

    uint8_t cmd_val[2] = {0x00};
    uint16_t slave_addr = 0;
    uint16_t cmd_len = 0;
    uint32_t ret_value = AF_SUCCESS;

    slave_addr = BU64241GWZ_VCM_SLAVE_ADDR;
    CMR_LOGI("E");
    cmd_len = 2;
    cmd_val[0] = 0x02;
    cmd_val[1] = 0x02;
    hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, slave_addr,
                       (uint8_t *)&cmd_val[0], cmd_len);
    usleep(1000);
    cmd_val[0] = 0x06;
    cmd_val[1] = 0x80;
    hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, slave_addr,
                       (uint8_t *)&cmd_val[0], cmd_len);
    usleep(1000);
    cmd_val[0] = 0x07;
    cmd_val[1] = 0x75;
    hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, slave_addr,
                       (uint8_t *)&cmd_val[0], cmd_len);

    return ret_value;
}
