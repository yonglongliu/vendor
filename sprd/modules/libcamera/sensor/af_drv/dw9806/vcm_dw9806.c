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
#define LOG_TAG "vcm_dw9806"
#include "vcm_dw9806.h"

static int dw9806_drv_create(struct af_drv_init_para *input_ptr,
                             cmr_handle *sns_af_drv_handle) {
    cmr_int ret = AF_SUCCESS;
    CHECK_PTR(input_ptr);
    ret = af_drv_create(input_ptr, sns_af_drv_handle);
    if (ret != AF_SUCCESS) {
        ret = AF_FAIL;
    } else {
        ret = _dw9806_drv_set_mode(*sns_af_drv_handle);
        if (ret != AF_SUCCESS)
            ret = AF_FAIL;
    }

    return ret;
}

static int dw9806_drv_delete(cmr_handle sns_af_drv_handle, void *param) {
    cmr_int ret = AF_SUCCESS;
    CHECK_PTR(sns_af_drv_handle);
    ret = af_drv_delete(sns_af_drv_handle, param);
    return ret;
}

/*==============================================================================
 * Description:
 * calculate vcm driver dac code and write to vcm driver;
 *
 * Param: ISP write dac code
 *============================================================================*/
static int dw9806_drv_set_pos(cmr_handle sns_af_drv_handle, uint32_t pos) {
    uint32_t ret_value = AF_SUCCESS;
    uint8_t cmd_val[2] = {0x00};
    struct sns_af_drv_cxt *af_drv_cxt =
        (struct sns_af_drv_cxt *)sns_af_drv_handle;
    CHECK_PTR(sns_af_drv_handle);

    uint16_t slave_addr = DW9806_VCM_SLAVE_ADDR;
    uint16_t cmd_len = 0;
    uint32_t time_out = 0;

    cmd_len = 2;
    cmd_val[0] = 0x03;
    cmd_val[1] = (pos & 0x300) >> 8;
    ret_value = hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, slave_addr,
                                   (uint8_t *)&cmd_val[0], cmd_len);
    cmd_val[0] = 0x04;
    cmd_val[1] = (pos & 0xff);
    ret_value = hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, slave_addr,
                                   (uint8_t *)&cmd_val[0], cmd_len);
    return ret_value;
}

static int dw9806_drv_ioctl(cmr_handle sns_af_drv_handle, enum sns_cmd cmd,
                            void *param) {
    uint32_t ret_value = AF_SUCCESS;
    struct sns_af_drv_cxt *af_drv_cxt =
        (struct sns_af_drv_cxt *)sns_af_drv_handle;
    CHECK_PTR(sns_af_drv_handle);
    switch (cmd) {
    case CMD_SNS_AF_SET_BEST_MODE:
        break;
    case CMD_SNS_AF_GET_TEST_MODE:
        break;
    case CMD_SNS_AF_SET_TEST_MODE:
        break;
    default:
        break;
    }
    return ret_value;
}

struct sns_af_drv_entry dw9806_drv_entry = {
    .motor_avdd_val = SENSOR_AVDD_2800MV,
    .default_work_mode = 2,
    .af_ops =
        {
            .create = dw9806_drv_create,
            .delete = dw9806_drv_delete,
            .set_pos = dw9806_drv_set_pos,
            .get_pos = NULL,
            .ioctl = dw9806_drv_ioctl,
        },
};

static int _dw9806_drv_set_mode(cmr_handle sns_af_drv_handle) {
    uint32_t ret_value = AF_SUCCESS;

    struct sns_af_drv_cxt *af_drv_cxt =
        (struct sns_af_drv_cxt *)sns_af_drv_handle;
    CHECK_PTR(sns_af_drv_handle);

    uint8_t mode = 0;
    uint8_t cmd_val[6] = {0x00};
    uint16_t slave_addr = DW9806_VCM_SLAVE_ADDR;
    uint16_t cmd_len = 0;

    if (af_drv_cxt->af_work_mode) {
        mode = af_drv_cxt->af_work_mode;
    } else {
        mode = dw9806_drv_entry.default_work_mode;
    }
    CMR_LOGI("mode: %d", mode);

    switch (mode) {
    case 1:
        break;

    case 2: {
        cmd_len = 2;
        cmd_val[0] = 0x02;
        cmd_val[1] = 0x01;
        hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, slave_addr,
                           (uint8_t *)&cmd_val[0], cmd_len);
        usleep(1000);
        cmd_val[0] = 0x02;
        cmd_val[1] = 0x00;
        hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, slave_addr,
                           (uint8_t *)&cmd_val[0], cmd_len);
        usleep(1000);
        cmd_val[0] = 0x02;
        cmd_val[1] = 0x02;
        hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, slave_addr,
                           (uint8_t *)&cmd_val[0], cmd_len);
        usleep(1000);
        cmd_val[0] = 0x06;
        cmd_val[1] = 0x61;
        hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, slave_addr,
                           (uint8_t *)&cmd_val[0], cmd_len);
        usleep(1000);
        cmd_val[0] = 0x07;
        cmd_val[1] = 0x1c;
        hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, slave_addr,
                           (uint8_t *)&cmd_val[0], cmd_len);
        usleep(1000);
    } break;

    case 3:
        break;
    }

    return ret_value;
}
