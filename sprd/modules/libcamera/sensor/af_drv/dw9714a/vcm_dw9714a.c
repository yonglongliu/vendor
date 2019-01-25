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
#define LOG_TAG "vcm_dw9714a"
#include "vcm_dw9714a.h"

static int dw9714a_drv_create(struct af_drv_init_para *input_ptr,
                              cmr_handle *sns_af_drv_handle) {
    cmr_int ret = AF_SUCCESS;
    CHECK_PTR(input_ptr);
    ret = af_drv_create(input_ptr, sns_af_drv_handle);
    if (ret != AF_SUCCESS) {
        ret = AF_FAIL;
    } else {
        ret = _dw9714a_drv_set_mode(*sns_af_drv_handle);
        if (ret != AF_SUCCESS)
            ret = AF_FAIL;
    }

    return ret;
}
static int dw9714a_drv_delete(cmr_handle sns_af_drv_handle, void *param) {
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
static int dw9714a_drv_set_pos(cmr_handle sns_af_drv_handle, uint16_t pos) {
    uint32_t ret_value = AF_SUCCESS;
    struct sns_af_drv_cxt *af_drv_cxt =
        (struct sns_af_drv_cxt *)sns_af_drv_handle;
    CHECK_PTR(sns_af_drv_handle);

    uint16_t slave_addr = DW9714A_VCM_SLAVE_ADDR;
    uint16_t cmd_len = 0;
    uint8_t cmd_val[2] = {0x00};

    cmd_val[0] = ((pos & 0xfff0) >> 4) & 0x3f;
    cmd_val[1] = ((pos & 0x0f) << 4) & 0xf0;
    cmd_len = 2;
    ret_value = hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, slave_addr,
                                   (uint8_t *)&cmd_val[0], cmd_len);

    CMR_LOGI("vcm_dw9714A_set_position: _write_af, ret =  %d, param = %d,  "
             "MSL:%x, LSL:%x\n",
             ret_value, pos, cmd_val[0], cmd_val[1]);

    return ret_value;
}

static int dw9714a_drv_ioctl(cmr_handle sns_af_drv_handle, enum sns_cmd cmd,
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

struct sns_af_drv_entry dw9714a_drv_entry = {
    .motor_avdd_val = SENSOR_AVDD_2800MV,
    .default_work_mode = 2,
    .af_ops =
        {
            .create = dw9714a_drv_create,
            .delete = dw9714a_drv_delete,
            .set_pos = dw9714a_drv_set_pos,
            .get_pos = NULL,
            .ioctl = dw9714a_drv_ioctl,
        },
};

static int _dw9714a_drv_set_mode(cmr_handle sns_af_drv_handle) {
    struct sns_af_drv_cxt *af_drv_cxt =
        (struct sns_af_drv_cxt *)sns_af_drv_handle;
    CHECK_PTR(sns_af_drv_handle);

    uint8_t cmd_val[2] = {0x00};
    uint16_t slave_addr = DW9714A_VCM_SLAVE_ADDR;
    uint16_t cmd_len = 0;
    uint32_t ret_value = AF_SUCCESS;
    int i = 0;
    uint32_t mode = 0;

    if (af_drv_cxt->af_work_mode) {
        mode = af_drv_cxt->af_work_mode;
    } else {
        mode = dw9714a_drv_entry.default_work_mode;
    }

    CMR_LOGI("vcm_dw9714A_init: _DW9714A_SRCInit: mode = %d\n", mode);
    switch (mode) {
    case 1:
        break;
    case 2: {
        cmd_val[0] = 0xec;
        cmd_val[1] = 0xa3;
        cmd_len = 2;
        ret_value = hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, slave_addr,
                                       (uint8_t *)&cmd_val[0], cmd_len);

        cmd_val[0] = 0xa1;
        cmd_val[1] = 0x0e;
        cmd_len = 2;
        ret_value = hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, slave_addr,
                                       (uint8_t *)&cmd_val[0], cmd_len);

        cmd_val[0] = 0xf2;
        cmd_val[1] = 0x90;
        cmd_len = 2;
        ret_value = hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, slave_addr,
                                       (uint8_t *)&cmd_val[0], cmd_len);

        cmd_val[0] = 0xdc;
        cmd_val[1] = 0x51;
        cmd_len = 2;
        ret_value = hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, slave_addr,
                                       (uint8_t *)&cmd_val[0], cmd_len);
    } break;

    case 3:
        break;
    }
    return ret_value;
}
