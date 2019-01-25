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
#define LOG_TAG "vcm_zc524"
#include "vcm_zc524.h"

#if 0
uint32_t vcm_zc524_get_pose_dis(SENSOR_HW_HANDLE handle, uint32_t *up2h, uint32_t *h2down)
{
	*up2h = POSE_UP_HORIZONTAL;
	*h2down = POSE_DOWN_HORIZONTAL;

	return 0;
}
#endif

static int vcm_zc524_drv_create(struct af_drv_init_para *input_ptr,
                                cmr_handle *sns_af_drv_handle) {
    cmr_int ret = AF_SUCCESS;
    CMR_LOGV("create");
    CHECK_PTR(input_ptr);
    ret = af_drv_create(input_ptr, sns_af_drv_handle);
    if (ret != AF_SUCCESS) {
        ret = AF_FAIL;
    } else {
        ret = _vcm_zc524_set_mode(*sns_af_drv_handle);
        if (ret != AF_SUCCESS)
            ret = AF_FAIL;
    }
    CMR_LOGV("done");

    return ret;
}

static int vcm_zc524_drv_delete(cmr_handle sns_af_drv_handle, void *param) {
    cmr_int ret = AF_SUCCESS;
    CHECK_PTR(sns_af_drv_handle);
    ret = af_drv_delete(sns_af_drv_handle, param);
    return ret;
}

static int vcm_zc524_drv_set_pos(cmr_handle sns_af_drv_handle, uint16_t pos) {
    uint8_t cmd_val[2] = {0x00};
    uint32_t ret_value = AF_SUCCESS;

    struct sns_af_drv_cxt *af_drv_cxt =
        (struct sns_af_drv_cxt *)sns_af_drv_handle;
    CHECK_PTR(sns_af_drv_handle);

    uint16_t slave_addr = zc524_VCM_SLAVE_ADDR;
    uint16_t cmd_len = 0;
    uint32_t time_out = 0;

    if ((int32_t)pos < 0)
        pos = 0;
    else if ((int32_t)pos > 0x3FF)
        pos = 0x3FF;
    af_drv_cxt->current_pos = pos & 0x3FF;

    CMR_LOGI("set position %d", pos);
    cmd_len = 2;
    cmd_val[0] = (pos >> 4) & 0x3F;
    cmd_val[1] = ((pos & 0xFF) << 4) & 0xF0;
    ret_value = hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, slave_addr,
                                   (uint8_t *)&cmd_val[0], cmd_len);

    return ret_value;
}

static int vcm_zc524_drv_get_pos_info(struct sensor_vcm_info *info) {
    CHECK_PTR(info);

    if ((int32_t)(info->pos) < 0)
        info->pos = 0;
    else if ((int32_t)(info->pos) > 0x3FF)
        info->pos = 0x3FF;

    info->slave_addr = zc524_VCM_SLAVE_ADDR;
    info->cmd_len = 2;
    info->cmd_val[0] = ((info->pos) >> 4) & 0x3F;
    info->cmd_val[1] = (((info->pos) & 0xFF) << 4) & 0xF0;

    return AF_SUCCESS;
}

static int vcm_zc524_drv_ioctl(cmr_handle sns_af_drv_handle, enum sns_cmd cmd,
                               void *param) {
    uint32_t ret_value = AF_SUCCESS;
    struct sns_af_drv_cxt *af_drv_cxt =
        (struct sns_af_drv_cxt *)sns_af_drv_handle;
    CHECK_PTR(sns_af_drv_handle);
    CMR_LOGI("cmd is ", cmd);
    switch (cmd) {
    case CMD_SNS_AF_SET_BEST_MODE:
        break;
    case CMD_SNS_AF_GET_TEST_MODE:
        break;
    case CMD_SNS_AF_SET_TEST_MODE:
        break;
    case CMD_SNS_AF_GET_POS_INFO:
        vcm_zc524_drv_get_pos_info(param);
    default:
        break;
    }
    return ret_value;
}

struct sns_af_drv_entry vcm_zc524_drv_entry = {
    .motor_avdd_val = SENSOR_AVDD_2800MV,
    .default_work_mode = 2,
    .af_ops =
        {
            .create = vcm_zc524_drv_create,
            .delete = vcm_zc524_drv_delete,
            .set_pos = vcm_zc524_drv_set_pos,
            .get_pos = NULL,
            .ioctl = vcm_zc524_drv_ioctl,
        },
};

static int _vcm_zc524_set_mode(cmr_handle sns_af_drv_handle) {
    struct sns_af_drv_cxt *af_drv_cxt =
        (struct sns_af_drv_cxt *)sns_af_drv_handle;
    CHECK_PTR(sns_af_drv_handle);

    uint8_t cmd_val[2] = {0x00};
    uint8_t mode = 0;
    uint16_t slave_addr = zc524_VCM_SLAVE_ADDR;
    uint16_t dsc_reg_addr = 0x80;
    uint16_t vdp_addr = 0x81;
    uint32_t ret = AF_SUCCESS;
    uint16_t cmd_len = 0;

    cmd_len = 2;

    if (af_drv_cxt->af_work_mode) {
        mode = af_drv_cxt->af_work_mode;
    } else {
        mode = vcm_zc524_drv_entry.default_work_mode;
    }
    CMR_LOGI("mode: %d", mode);

    usleep(100);
    switch (mode) {
    case 1:
        break;

    case 2: {
        // ret = hw_sensor_grc_read_i2c(af_drv_cxt->hw_handle,EepromSlaveID,
        // 0x3B,
        // BITS_ADDR8_REG8);
        cmd_val[0] = 0xEC;
        cmd_val[1] = 0xAB;
        ret = hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, slave_addr,
                                 (uint8_t *)&cmd_val[0], cmd_len);

        if (ret) {
            SENSOR_LOGI("SENSOR_vcm: start control fail!");
        }

        cmd_val[0] = 0x80;
        cmd_val[1] = 0x0C;
        ret = hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, slave_addr,
                                 (uint8_t *)&cmd_val[0], cmd_len);

        if (ret) {
            SENSOR_LOGI("SENSOR_vcm: _zc524_SRCInit 1 fail!");
        }

        cmd_val[0] = 0x81;
        cmd_val[1] = 0x64;
        ret = hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, slave_addr,
                                 (uint8_t *)&cmd_val[0], cmd_len);

        if (ret) {
            SENSOR_LOGI("SENSOR_vcm: _zc524_SRCInit 2 fail!");
        }

        cmd_val[0] = 0xDC;
        cmd_val[1] = 0x59;
        ret = hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, slave_addr,
                                 (uint8_t *)&cmd_val[0], cmd_len);

        if (ret) {
            SENSOR_LOGI("SENSOR_vcm: finish control fail!");
        }
        usleep(200);

#if 0
    {
      int i;
      for (i = 0; i < 10; i++) {
        vcm_zc524_drv_set_pos(sns_af_drv_handle, 0);
        usleep(50000);
        vcm_zc524_drv_set_pos(sns_af_drv_handle, 0x3FF);
        usleep(50000);
      }
    }
#endif

    } break;
    default:
        break;
    }
    return ret;
}
