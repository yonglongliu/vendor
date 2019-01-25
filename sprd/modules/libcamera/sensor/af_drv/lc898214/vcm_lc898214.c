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
#include "vcm_lc898214.h"

#if 0
uint32_t vcm_LC898214_get_pose_dis(SENSOR_HW_HANDLE handle, uint32_t *up2h, uint32_t *h2down)
{
	*up2h = POSE_UP_HORIZONTAL;
	*h2down = POSE_DOWN_HORIZONTAL;

	return 0;
}
#endif

static int lc898214_drv_create(struct af_drv_init_para *input_ptr,
                               cmr_handle *sns_af_drv_handle) {
    cmr_int ret = AF_SUCCESS;
    struct sns_af_drv_cxt *af_drv_cxt = NULL;
    CHECK_PTR(input_ptr);
    ret = af_drv_create(input_ptr, sns_af_drv_handle);
    if (ret != AF_SUCCESS) {
        ret = AF_FAIL;
    } else {
        af_drv_cxt = (struct sns_af_drv_cxt *)*sns_af_drv_handle;
        af_drv_cxt->private = (void *)malloc(sizeof(struct lc898214_pri_data));
        cmr_bzero(af_drv_cxt->private, sizeof(struct lc898214_pri_data));
        ret = _lc898214_drv_init(*sns_af_drv_handle);
        if (ret != AF_SUCCESS)
            ret = AF_FAIL;
    }
    return ret;
}
static int lc898214_drv_delete(cmr_handle sns_af_drv_handle, void *param) {
    cmr_int ret = AF_SUCCESS;
    struct sns_af_drv_cxt *af_drv_cxt = sns_af_drv_handle;
    CHECK_PTR(sns_af_drv_handle);

    if (af_drv_cxt->private) {
        free(af_drv_cxt->private);
        af_drv_cxt->private = NULL;
    }
    ret = af_drv_delete(sns_af_drv_handle, param);
    return ret;
}
/*==============================================================================
 * Description:
 * calculate vcm driver dac code and write to vcm driver;
 *
 * Param: ISP write dac code
 *============================================================================*/
static int lc898214_drv_set_pos(cmr_handle sns_af_drv_handle, uint32_t pos) {
    uint32_t ret_value = AF_SUCCESS;
    uint8_t cmd_val[3] = {0x00};
    struct sns_af_drv_cxt *af_drv_cxt =
        (struct sns_af_drv_cxt *)sns_af_drv_handle;
    CHECK_PTR(sns_af_drv_handle);
    struct lc898214_pri_data *pri_data = af_drv_cxt->private;
    CHECK_PTR(pri_data);

    uint16_t cmd_len = 0;
    uint32_t time_out = 0;

    uint16_t slave_addr = (0xE4 >> 1); // LC898214_VCM_SLAVE_ADDR;
    int i;

    if ((int32_t)pos < 0)
        pos = 0;
    else if ((int32_t)pos > 0x3FF)
        pos = 0x3FF;
    af_drv_cxt->current_pos = pos & 0x3FF;
    CMR_LOGI("set position %d", pos);
    pri_data->rCode = pos;
    uint16_t Min_Pos = 0, Max_Pos = 1023;
    uint16_t ivalue2;
    uint16_t temp;
    double k = 1.0 * ((pri_data->Hall_Max) - (pri_data->Hall_Min)) /
               (Max_Pos - Min_Pos);
    temp = pri_data->Hall_Min;
    int32_t iK = (int32_t)(k * 100);
    double k2 = iK / 100.0;

    uint16_t a = (uint16_t)((pos * k2) + temp);
    ivalue2 = a;
    pri_data->HallValue = ivalue2 & 0xfff0;

    if ((int16_t)ivalue2 < (int16_t)pri_data->Hall_Min) {
        ivalue2 = (uint16_t)(pri_data->Hall_Min);
    }
    if ((int16_t)ivalue2 > (int16_t)pri_data->Hall_Max) {
        ivalue2 = (uint16_t)(pri_data->Hall_Max);
    }
    // BOOL bRet = I2C_Write(SlaveID, 0xA0 ,ivalue2, 2);
    cmd_len = 2;
    cmd_val[0] = 0xA0;
    cmd_val[1] = ((ivalue2 & 0xff00) >> 8); // pos&0xff00>>8;//ivalue2;//TODO
    cmd_val[2] = ivalue2 & 0xff;            // pos&0xff00>>8;//ivalue2;//TODO
    ret_value = hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, slave_addr,
                                   (uint8_t *)&cmd_val[0], cmd_len);
    cmd_len = 2;
    cmd_val[0] = 0xA1;
    cmd_val[1] = ivalue2 & 0xff; // pos&0xf0;//ivalue2;//TODO
    ret_value = hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, slave_addr,
                                   (uint8_t *)&cmd_val[0], cmd_len);
    CMR_LOGV("set position set_pos %d SlvCh 0x%04x 0x%04x", pos, ivalue2,
            ret_value);

    uint16_t SlvCh;
    for (i = 0; i < 30; i++) {
        SlvCh = hw_sensor_grc_read_i2c(af_drv_cxt->hw_handle, slave_addr, 0x8F,
                                       BITS_ADDR8_REG8);
        // ret_value
        // =hw_sensor_grc_read_i2c(af_drv_cxt->hw_handle,slave_addr,0xA0,
        // BITS_ADDR8_REG16);
        usleep(50);
        if (SlvCh == 0x00) {
            break;
        }
    }
    // CMR_LOGI("set position 1 set_pos %d SlvCh 0x%04x 0x%04x", pos, ivalue2,
    // ret_value);
    return ret_value;
}

static int lc898214_drv_ioctl(cmr_handle sns_af_drv_handle, enum sns_cmd cmd,
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

struct sns_af_drv_entry lc898214_drv_entry = {
    .motor_avdd_val = SENSOR_AVDD_2800MV,
    .default_work_mode = 2,
    .af_ops =
        {
            .create = lc898214_drv_create,
            .delete = lc898214_drv_delete,
            .set_pos = lc898214_drv_set_pos,
            .get_pos = NULL,
            .ioctl = lc898214_drv_ioctl,
        },
};

static int _lc898214_drv_init(cmr_handle sns_af_drv_handle) {
    uint32_t ret_value = AF_SUCCESS;

    struct sns_af_drv_cxt *af_drv_cxt =
        (struct sns_af_drv_cxt *)sns_af_drv_handle;
    CHECK_PTR(sns_af_drv_handle);
    struct lc898214_pri_data *pri_data = af_drv_cxt->private;
    CHECK_PTR(pri_data);

    uint8_t cmd_val[2] = {0x00};
    uint8_t cmd_val_1[3] = {0x00};
    uint16_t slave_addr = LC898214_VCM_SLAVE_ADDR;
    uint16_t cmd_len = 0;
    uint16_t SlaveID = (0xE4 >> 1);
    uint16_t EepromSlaveID = (0xE6 >> 1);

    uint16_t SlvCh = 0xff;
    uint16_t EepCh = 0xff;
    int i;

    CMR_LOGV("E");
    for (i = 0; i < 30; i++) {
        SlvCh = hw_sensor_grc_read_i2c(af_drv_cxt->hw_handle, SlaveID, 0xF0,
                                       BITS_ADDR8_REG8);
        usleep(50);
        if (SlvCh == 0x42) {
            break;
        }
    }
    usleep(100);
    cmd_len = 2;

    cmd_val[0] = 0x87;
    cmd_val[1] = 0x00;
    ret_value = hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, SlaveID,
                                   (uint8_t *)&cmd_val[0], cmd_len);

    usleep(20);
    uint16_t Max_H = 0xff, Max_L = 0xff, Min_H = 0xff, Min_L = 0xff;
    Max_H = hw_sensor_grc_read_i2c(af_drv_cxt->hw_handle, EepromSlaveID, 0x38,
                                   BITS_ADDR8_REG8);
    usleep(50);
    Max_L = hw_sensor_grc_read_i2c(af_drv_cxt->hw_handle, EepromSlaveID, 0x39,
                                   BITS_ADDR8_REG8);
    usleep(50);
    Min_H = hw_sensor_grc_read_i2c(af_drv_cxt->hw_handle, EepromSlaveID, 0x3A,
                                   BITS_ADDR8_REG8);
    usleep(50);
    Min_L = hw_sensor_grc_read_i2c(af_drv_cxt->hw_handle, EepromSlaveID, 0x3B,
                                   BITS_ADDR8_REG8);
    usleep(50);
    uint16_t Max = (uint16_t)((Max_H << 8) | Max_L);
    uint16_t Min = (uint16_t)((Min_H << 8) | Min_L);
    pri_data->Hall_Max = (int16_t)Max; //(int16_t)Max;//0x6000
    pri_data->Hall_Min = (int16_t)Min; //(int16_t)Min; //0x9000
    CMR_LOGV("DriverIC Init Max_H	%d %d %d %d! %d %d", Max_H, Max_L,
            Min_H, Min_L, Max, Min);

    // I2C_Write(SlaveID, 0xE0, 0x01, 0);
    cmd_val[0] = 0xE0;
    cmd_val[1] = 0x01;
    ret_value = hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, SlaveID,
                                   (uint8_t *)&cmd_val[0], cmd_len);
    for (i = 0; i < 30; i++) {
        EepCh = hw_sensor_grc_read_i2c(af_drv_cxt->hw_handle, SlaveID, 0xE0,
                                       BITS_ADDR8_REG8);
        usleep(50);
        if (EepCh == 0x00) {
            break;
        }
    }
    if ((SlvCh != 0x42) || (EepCh != 0x00)) {
        CMR_LOGE("DriverIC Init Err!");
        return -1;
    }
    //	cmd_val_1[0] = 0xA0;
    //	cmd_val_1[1] = ((pri_data->Hall_Min) >> 8);
    //	cmd_val_1[2] = ((pri_data->Hall_Min) & 0xff);
    // ret_value =
    // hw_Sensor_WriteI2C(af_drv_cxt->hw_handle,SlaveID,(uint8_t*)&cmd_val_1[0],
    // 3);
    // I2C_Write(SlaveID, 0xA0, Hall_Min, 2);
    pri_data->HallValue = pri_data->Hall_Min;
    pri_data->rCode = 0;

    return ret_value;
}
