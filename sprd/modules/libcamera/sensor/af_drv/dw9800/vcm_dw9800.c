/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "vcm_dw9800"
#include "vcm_dw9800.h"

static uint32_t _dw9800_set_motor_bestmode(cmr_handle sns_af_drv_handle) {
    struct sns_af_drv_cxt *af_drv_cxt =
        (struct sns_af_drv_cxt *)sns_af_drv_handle;
    CHECK_PTR(sns_af_drv_handle);

    uint8_t cmd_val[2];

    // set
    cmd_val[0] = 0x02;
    cmd_val[1] = 0x02;
    hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, DW9800_VCM_SLAVE_ADDR,
                       (uint8_t *)&cmd_val[0], 2);
    usleep(200);
    cmd_val[0] = 0x06;
    cmd_val[1] = 0x80;
    hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, DW9800_VCM_SLAVE_ADDR,
                       (uint8_t *)&cmd_val[0], 2);
    usleep(200);
    cmd_val[0] = 0x07;
    cmd_val[1] = 0x75;
    hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, DW9800_VCM_SLAVE_ADDR,
                       (uint8_t *)&cmd_val[0], 2);
    usleep(200 * 1000);

    return 0;
}

static uint32_t _dw9800_get_test_vcm_mode(cmr_handle sns_af_drv_handle) {
    struct sns_af_drv_cxt *af_drv_cxt =
        (struct sns_af_drv_cxt *)sns_af_drv_handle;
    CHECK_PTR(sns_af_drv_handle);

    uint8_t ctrl, mode, freq;
    uint8_t pos1, pos2;
    uint8_t cmd_val[2];

    FILE *fp = NULL;
    fp = fopen("/data/misc/cameraserver/cur_vcm_info.txt", "wb");
    // read
    cmd_val[0] = 0x02;
    cmd_val[1] = 0x02;
    hw_Sensor_ReadI2C(af_drv_cxt->hw_handle, DW9800_VCM_SLAVE_ADDR,
                      (uint8_t *)&cmd_val[0], 1);
    ctrl = cmd_val[0];
    usleep(200);
    cmd_val[0] = 0x06;
    cmd_val[1] = 0x06;
    hw_Sensor_ReadI2C(af_drv_cxt->hw_handle, DW9800_VCM_SLAVE_ADDR,
                      (uint8_t *)&cmd_val[0], 1);
    mode = cmd_val[0];
    usleep(200);
    cmd_val[0] = 0x07;
    cmd_val[1] = 0x07;
    hw_Sensor_ReadI2C(af_drv_cxt->hw_handle, DW9800_VCM_SLAVE_ADDR,
                      (uint8_t *)&cmd_val[0], 1);
    freq = cmd_val[0];

    // read
    cmd_val[0] = 0x03;
    cmd_val[1] = 0x03;
    hw_Sensor_ReadI2C(af_drv_cxt->hw_handle, DW9800_VCM_SLAVE_ADDR,
                      (uint8_t *)&cmd_val[0], 1);
    pos1 = cmd_val[0];
    usleep(200);
    cmd_val[0] = 0x04;
    cmd_val[1] = 0x04;
    hw_Sensor_ReadI2C(af_drv_cxt->hw_handle, DW9800_VCM_SLAVE_ADDR,
                      (uint8_t *)&cmd_val[0], 1);
    pos2 = cmd_val[0];

    fprintf(fp, "VCM ctrl mode freq pos ,%d %d %d %d", ctrl, mode, freq,
            (pos1 << 8) + pos2);
    fclose(fp);
    return 0;
}

static uint32_t _dw9800_set_test_vcm_mode(cmr_handle sns_af_drv_handle,
                                          char *vcm_mode) {
    struct sns_af_drv_cxt *af_drv_cxt =
        (struct sns_af_drv_cxt *)sns_af_drv_handle;
    CHECK_PTR(sns_af_drv_handle);

    uint8_t ctrl, mode, freq;
    uint8_t pos1, pos2;
    uint8_t cmd_val[2];
    char *p1 = vcm_mode;

    while (*p1 != '~' && *p1 != '\0')
        p1++;
    *p1++ = '\0';
    ctrl = atoi(vcm_mode);
    vcm_mode = p1;
    while (*p1 != '~' && *p1 != '\0')
        p1++;
    *p1++ = '\0';
    mode = atoi(vcm_mode);
    vcm_mode = p1;
    while (*p1 != '~' && *p1 != '\0')
        p1++;
    *p1++ = '\0';
    freq = atoi(vcm_mode);
    CMR_LOGI("VCM ctrl mode freq pos 1nd,%d %d %d", ctrl, mode, freq);
    // set
    cmd_val[0] = 0x02;
    cmd_val[1] = ctrl;
    hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, DW9800_VCM_SLAVE_ADDR,
                       (uint8_t *)&cmd_val[0], 2);
    usleep(200);
    cmd_val[0] = 0x06;
    cmd_val[1] = mode;
    hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, DW9800_VCM_SLAVE_ADDR,
                       (uint8_t *)&cmd_val[0], 2);
    usleep(200);
    cmd_val[0] = 0x07;
    cmd_val[1] = freq;
    hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, DW9800_VCM_SLAVE_ADDR,
                       (uint8_t *)&cmd_val[0], 2);
    usleep(200 * 1000);
    // read
    cmd_val[0] = 0x02;
    cmd_val[1] = 0x02;
    hw_Sensor_ReadI2C(af_drv_cxt->hw_handle, DW9800_VCM_SLAVE_ADDR,
                      (uint8_t *)&cmd_val[0], 1);
    ctrl = cmd_val[0];
    usleep(200);
    cmd_val[0] = 0x06;
    cmd_val[1] = 0x06;
    hw_Sensor_ReadI2C(af_drv_cxt->hw_handle, DW9800_VCM_SLAVE_ADDR,
                      (uint8_t *)&cmd_val[0], 1);
    mode = cmd_val[0];
    usleep(200);
    cmd_val[0] = 0x07;
    cmd_val[1] = 0x07;
    hw_Sensor_ReadI2C(af_drv_cxt->hw_handle, DW9800_VCM_SLAVE_ADDR,
                      (uint8_t *)&cmd_val[0], 1);
    freq = cmd_val[0];
    CMR_LOGI("VCM ctrl mode freq pos 2nd,%d %d %d", ctrl, mode, freq);
    return 0;
}

static int dw9800_drv_create(struct af_drv_init_para *input_ptr,
                             cmr_handle *sns_af_drv_handle) {
    cmr_int ret = AF_SUCCESS;
    CHECK_PTR(input_ptr);
    ret = af_drv_create(input_ptr, sns_af_drv_handle);
    if (ret != AF_SUCCESS) {
        ret = AF_FAIL;
    } else {
        ret = _dw9800_drv_init(*sns_af_drv_handle);
        if (ret != AF_SUCCESS)
            ret = AF_FAIL;
    }
    CMR_LOGV("af_drv_handle:%p", *sns_af_drv_handle);
    return ret;
}

static int dw9800_drv_delete(cmr_handle sns_af_drv_handle, void *param) {
    cmr_int ret = AF_SUCCESS;
    CHECK_PTR(sns_af_drv_handle);
    ret = af_drv_delete(sns_af_drv_handle, param);
    return ret;
}

static int dw9800_drv_set_pos(cmr_handle sns_af_drv_handle, uint16_t pos) {
    uint8_t cmd_val[2] = {0};
    struct sns_af_drv_cxt *af_drv_cxt =
        (struct sns_af_drv_cxt *)sns_af_drv_handle;
    CHECK_PTR(sns_af_drv_handle);
    // set pos
    cmd_val[0] = 0x03;
    cmd_val[1] = (pos >> 8) & 0x03;
    hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, DW9800_VCM_SLAVE_ADDR,
                       (uint8_t *)&cmd_val[0], 2);

    cmd_val[0] = 0x04;
    cmd_val[1] = pos & 0xff;
    hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, DW9800_VCM_SLAVE_ADDR,
                       (uint8_t *)&cmd_val[0], 2);
    CMR_LOGI("set_position:0x%x", pos);
    return AF_SUCCESS;
}

static int dw9800_drv_get_pos(cmr_handle sns_af_drv_handle, uint16_t *pos) {
    uint8_t cmd_val[2];
    struct sns_af_drv_cxt *af_drv_cxt =
        (struct sns_af_drv_cxt *)sns_af_drv_handle;
    CHECK_PTR(sns_af_drv_handle);
    // read
    CMR_LOGV("handle:%p", sns_af_drv_handle);
    cmd_val[0] = 0x03;
    hw_Sensor_ReadI2C(af_drv_cxt->hw_handle, DW9800_VCM_SLAVE_ADDR,
                      (uint8_t *)&cmd_val[0], 1);
    *pos = (cmd_val[0] & 0x03) << 8;
    usleep(200);
    cmd_val[0] = 0x04;
    hw_Sensor_ReadI2C(af_drv_cxt->hw_handle, DW9800_VCM_SLAVE_ADDR,
                      (uint8_t *)&cmd_val[0], 1);
    *pos += cmd_val[0];
    CMR_LOGI("get_position:0x%x", *pos);

    return AF_SUCCESS;
}
static int dw9800_drv_ioctl(cmr_handle sns_af_drv_handle, enum sns_cmd cmd,
                            void *param) {
    uint32_t ret_value = AF_SUCCESS;
    struct sns_af_drv_cxt *af_drv_cxt =
        (struct sns_af_drv_cxt *)sns_af_drv_handle;
    CHECK_PTR(sns_af_drv_handle);
    switch (cmd) {
    case CMD_SNS_AF_SET_BEST_MODE:
        _dw9800_set_motor_bestmode(sns_af_drv_handle);
        break;
    case CMD_SNS_AF_GET_TEST_MODE:
        _dw9800_get_test_vcm_mode(sns_af_drv_handle);
        break;
    case CMD_SNS_AF_SET_TEST_MODE:
        _dw9800_set_test_vcm_mode(sns_af_drv_handle, param);
        break;
    default:
        break;
    }
    return ret_value;
}

struct sns_af_drv_entry dw9800_drv_entry = {
    .motor_avdd_val = SENSOR_AVDD_2800MV,
    .default_work_mode = 0,
    .af_ops =
        {
            .create = dw9800_drv_create,
            .delete = dw9800_drv_delete,
            .set_pos = dw9800_drv_set_pos,
            .get_pos = dw9800_drv_get_pos,
            .ioctl = dw9800_drv_ioctl,
        },
};

static int _dw9800_drv_init(cmr_handle sns_af_drv_handle) {
    struct sns_af_drv_cxt *af_drv_cxt =
        (struct sns_af_drv_cxt *)sns_af_drv_handle;
    CHECK_PTR(sns_af_drv_handle);

    uint8_t cmd_val[2] = {0x00};
    uint16_t slave_addr = 0;
    uint16_t cmd_len = 0;
    uint32_t ret_value = SENSOR_SUCCESS;

    slave_addr = DW9800_VCM_SLAVE_ADDR;
    CMR_LOGV("E");
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
