/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
ver: 1.0
*/
#define LOG_TAG "af_bu64297"
#include "af_bu64297gwz.h"

#if 0
uint32_t bu64297gwz_get_pose_dis(SENSOR_HW_HANDLE handle, uint32_t *up2h, uint32_t *h2down)
{
	*up2h = POSE_UP_HORIZONTAL;
	*h2down = POSE_DOWN_HORIZONTAL;

	return 0;
}
#endif

static int bu64297gwz_drv_create(struct af_drv_init_para *input_ptr,
                                 cmr_handle *sns_af_drv_handle) {
    cmr_int ret = AF_SUCCESS;
    CHECK_PTR(input_ptr);
    ret = af_drv_create(input_ptr, sns_af_drv_handle);
    if (ret != AF_SUCCESS) {
        ret = AF_FAIL;
    } else {
        ret = _bu64297gwz_drv_init(*sns_af_drv_handle);
        if (ret != AF_SUCCESS)
            ret = AF_FAIL;
    }

    return ret;
}

static int bu64297gwz_drv_delete(cmr_handle sns_af_drv_handle, void *param) {
    cmr_int ret = AF_SUCCESS;
    CHECK_PTR(sns_af_drv_handle);
    ret = af_drv_delete(sns_af_drv_handle, param);
    return ret;
}

static int bu64297gwz_drv_set_pos(cmr_handle sns_af_drv_handle, uint16_t pos) {
    uint32_t ret_value = SENSOR_SUCCESS;
    uint8_t cmd_val[2] = {0x00};
    struct sns_af_drv_cxt *af_drv_cxt =
        (struct sns_af_drv_cxt *)sns_af_drv_handle;
    CHECK_PTR(sns_af_drv_handle);
    int32_t target_code = 0;

    if ((int32_t)pos < 0)
        pos = 0;
    else if ((int32_t)pos > 0x3FF)
        pos = 0x3FF;
    target_code = pos & 0x3FF;
    CMR_LOGI("%d", target_code);
    cmd_val[0] = ((target_code >> 8) & 0x03) | 0xC4;
    cmd_val[1] = target_code & 0xff;
    ret_value =
        hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, bu64297gwz_VCM_SLAVE_ADDR,
                           (uint8_t *)&cmd_val[0], 2);
    return ret_value;
}

static int bu64297gwz_drv_get_pos(cmr_handle sns_af_drv_handle, uint16_t *pos) {
    uint8_t cmd_val[2] = {0xc4, 0x00};
    struct sns_af_drv_cxt *af_drv_cxt =
        (struct sns_af_drv_cxt *)sns_af_drv_handle;
    CHECK_PTR(sns_af_drv_handle);

    hw_Sensor_ReadI2C(af_drv_cxt->hw_handle, (0x18 >> 1),
                      (uint8_t *)&cmd_val[0],
                      (2 << 16) | SENSOR_I2C_VAL_16BIT | SENSOR_I2C_VAL_8BIT);
    *pos = (cmd_val[0] & 0x03) << 8;
    *pos += cmd_val[1];
    CMR_LOGI("vcm pos %d", *pos);
    return 0;
}
static int bu64297gwz_drv_ioctl(cmr_handle sns_af_drv_handle, enum sns_cmd cmd,
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

struct sns_af_drv_entry bu64297gwz_drv_entry = {
    .motor_avdd_val = SENSOR_AVDD_3000MV,
    .default_work_mode = 0,
    .af_ops =
        {
            .create = bu64297gwz_drv_create,
            .delete = bu64297gwz_drv_delete,
            .set_pos = bu64297gwz_drv_set_pos,
            .get_pos = bu64297gwz_drv_get_pos,
            .ioctl = bu64297gwz_drv_ioctl,
        },
};

static int _bu64297gwz_drv_init(cmr_handle sns_af_drv_handle) {
    struct sns_af_drv_cxt *af_drv_cxt =
        (struct sns_af_drv_cxt *)sns_af_drv_handle;
    CHECK_PTR(sns_af_drv_handle);

    uint8_t cmd_val[2] = {0x00};
    uint16_t slave_addr = bu64297gwz_VCM_SLAVE_ADDR;
    uint16_t cmd_len = 0;
    uint32_t ret_value = SENSOR_SUCCESS;

    cmd_val[0] = 0xcc;
    cmd_val[1] = 0x02; // 0x4b;
    cmd_len = 2;
    ret_value = hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, slave_addr,
                                   (uint8_t *)&cmd_val[0], cmd_len);

    cmd_val[0] = 0xd4;
    cmd_val[1] = 0x96;
    cmd_len = 2;
    ret_value = hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, slave_addr,
                                   (uint8_t *)&cmd_val[0], cmd_len);

    cmd_val[0] = 0xdc;
    cmd_val[1] = 0x03;
    cmd_len = 2;
    ret_value = hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, slave_addr,
                                   (uint8_t *)&cmd_val[0], cmd_len);

    cmd_val[0] = 0xe4;
    cmd_val[1] = 0x02;
    cmd_len = 2;
    ret_value = hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, slave_addr,
                                   (uint8_t *)&cmd_val[0], cmd_len);

    return ret_value;
}
