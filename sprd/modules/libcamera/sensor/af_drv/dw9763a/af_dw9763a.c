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
#define LOG_TAG "af_dw9763a"
#include "af_dw9763a.h"

/*==============================================================================
 * Description:
 * write code to vcm driver
 * you can change this function acording your spec if it's necessary
 * code: Dac code for vcm driver
 *============================================================================*/
static uint32_t _dw9763a_write_dac_code(cmr_handle sns_af_drv_handle, int32_t code)
{
    uint32_t ret_value = AF_SUCCESS;
    uint8_t cmd_val[2] = { 0x00 };
    struct sns_af_drv_cxt *af_drv_cxt = (struct sns_af_drv_cxt*)sns_af_drv_handle;
    CHECK_PTR(sns_af_drv_handle);

    uint16_t slave_addr = DW9763A_VCM_SLAVE_ADDR;
    uint16_t cmd_len = 0;

    if ((int32_t)code < 0)
        code = 0;
    else if ((int32_t)code > 0x3FF)
        code = 0x3FF;

    CMR_LOGV("%d", code);

    cmd_len = 2;
    cmd_val[0] = 0x03;
    cmd_val[1] = (code&0x300)>>8;
    ret_value = hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
    cmd_val[0] = 0x04;
    cmd_val[1] = (code&0xff);
    ret_value = hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, slave_addr,(uint8_t*)&cmd_val[0], cmd_len);

    return ret_value;
}

static int dw9763a_drv_create(struct af_drv_init_para *input_ptr, cmr_handle* sns_af_drv_handle)
{
    cmr_int ret = AF_SUCCESS;
    CHECK_PTR(input_ptr);
    ret = af_drv_create(input_ptr,sns_af_drv_handle);
    if (ret != AF_SUCCESS) {
        ret = AF_FAIL;
    } else {
        ret = _dw9763a_drv_set_mode(*sns_af_drv_handle);
        if (ret != AF_SUCCESS)
            ret = AF_FAIL;
    }

    return ret;
}

static int dw9763a_drv_delete(cmr_handle sns_af_drv_handle, void* param) {
    cmr_int ret = AF_SUCCESS;
    CHECK_PTR(sns_af_drv_handle);
    dw9763a_drv_set_pos(sns_af_drv_handle, 0);
    ret = af_drv_delete(sns_af_drv_handle,param);
    return ret;
}
/*==============================================================================
 * Description:
 * calculate vcm driver dac code and write to vcm driver;
 *
 * pos: ISP write dac code
 *============================================================================*/

static int dw9763a_drv_set_pos(cmr_handle sns_af_drv_handle, uint16_t pos)
{
    uint32_t ret_value = AF_SUCCESS;
    struct sns_af_drv_cxt *af_drv_cxt = (struct sns_af_drv_cxt*)sns_af_drv_handle;
    CHECK_PTR(sns_af_drv_handle);
    int32_t target_code = pos & 0x3FF;
    int32_t m_cur_dac_code = 0;

    m_cur_dac_code = af_drv_cxt->current_pos;
/*    if(!target_code) {
        while((m_cur_dac_code - target_code) >= MOVE_CODE_STEP_MAX){
            m_cur_dac_code = m_cur_dac_code - MOVE_CODE_STEP_MAX;
            _dw9763a_write_dac_code(sns_af_drv_handle, m_cur_dac_code);
            CMR_LOGI("dac_target_code = %d\n", m_cur_dac_code);
            usleep(WAIT_STABLE_TIME*1000);
        }
    }*/
    _dw9763a_write_dac_code(sns_af_drv_handle, target_code);

    af_drv_cxt->current_pos = target_code;
    CMR_LOGI("target_code = %d\n", target_code);

    return ret_value;
}

static int dw9763a_drv_ioctl(cmr_handle sns_af_drv_handle, enum sns_cmd cmd, void* param)
{
    uint32_t ret_value = AF_SUCCESS;
    struct sns_af_drv_cxt *af_drv_cxt = (struct sns_af_drv_cxt*)sns_af_drv_handle;
    CHECK_PTR(sns_af_drv_handle);
    switch(cmd) {
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

struct sns_af_drv_entry dw9763a_drv_entry =
{
    .motor_avdd_val = SENSOR_AVDD_2800MV,
    .default_work_mode = 2,
    .af_ops =
    {
        .create  = dw9763a_drv_create,
        .delete  = dw9763a_drv_delete,
        .set_pos = dw9763a_drv_set_pos,
        .get_pos = NULL,
        .ioctl   = dw9763a_drv_ioctl,
    },
};

static int _dw9763a_drv_set_mode(cmr_handle sns_af_drv_handle)
{
    struct sns_af_drv_cxt *af_drv_cxt = (struct sns_af_drv_cxt*)sns_af_drv_handle;
    CHECK_PTR(sns_af_drv_handle);

    uint8_t cmd_val[2] = { 0x00 };
    uint16_t slave_addr = DW9763A_VCM_SLAVE_ADDR;
    uint16_t cmd_len = 0;
    uint32_t ret_value = AF_SUCCESS;
    uint32_t mode = 0;
    if(af_drv_cxt->af_work_mode) {
        mode = af_drv_cxt->af_work_mode;
    } else {
        mode = dw9763a_drv_entry.default_work_mode;
    }

    CMR_LOGI("mode = %d\n", mode);

    switch (mode) {
    case 1:
        /* When you use direct mode after power on, you don't need register set. Because, DLC disable is default.*/
        break;
    case 2:
        /*Power Down */
        cmd_val[0] = 0x02;
        cmd_val[1] = 0x01;
        cmd_len = 2;
        ret_value = hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, slave_addr, (uint8_t *) & cmd_val[0], cmd_len);

        /*Power On */
        cmd_val[0] = 0x02;
        cmd_val[1] = 0x00;
        cmd_len = 2;
        ret_value = hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, slave_addr, (uint8_t *) & cmd_val[0], cmd_len);

        usleep(200);

        /*Ringing Setting On */
        cmd_val[0] = 0x02;
        cmd_val[1] = 0x02;
        cmd_len = 2;
        ret_value = hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, slave_addr, (uint8_t *) & cmd_val[0], cmd_len);

        /*SAC mode selection */
        cmd_val[0] = 0x06;
        cmd_val[1] = 0x61;
        cmd_len = 2;
        ret_value = hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, slave_addr, (uint8_t *) & cmd_val[0], cmd_len);

        /*Tvib Setting */
        cmd_val[0] = 0x07;
        cmd_val[1] = 0x38;
        cmd_len = 2;
        ret_value = hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, slave_addr, (uint8_t *) & cmd_val[0], cmd_len);
        break;
    case 3:
        /*Protection off */
        cmd_val[0] = 0xec;
        cmd_val[1] = 0xa3;
        cmd_len = 2;
        ret_value = hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, slave_addr, (uint8_t *) & cmd_val[0], cmd_len);

        /*DLC and MCLK[1:0] setting */
        cmd_val[0] = 0xa1;
        cmd_val[1] = 0x05;
        cmd_len = 2;
        ret_value = hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, slave_addr, (uint8_t *) & cmd_val[0], cmd_len);

        /*T_SRC[4:0] setting */
        cmd_val[0] = 0xf2;
        /*for better performace, cmd_val[1][7:3] should be adjusted to matching with the Tvib of your camera VCM*/
        cmd_val[1] = 0x00;
        cmd_len = 2;
        ret_value = hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, slave_addr, (uint8_t *) & cmd_val[0], cmd_len);

        /*Protection on */
        cmd_val[0] = 0xdc;
        cmd_val[1] = 0x51;
        cmd_len = 2;
        ret_value = hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, slave_addr, (uint8_t *) & cmd_val[0], cmd_len);
        break;
    }

    return ret_value;
}

