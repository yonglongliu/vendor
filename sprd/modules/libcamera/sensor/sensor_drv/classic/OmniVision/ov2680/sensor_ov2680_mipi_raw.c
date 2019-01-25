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

#include "sensor_ov2680_mipi_raw.h"

#undef  LOG_TAG
#define LOG_TAG "ov2680_mipi_raw"

#define FPS_INFO       s_ov2680_mode_fps_info
#define STATIC_INFO    s_ov2680_static_info
#define MIPI_RAW_INFO  g_ov2680_mipi_raw_info
#define RES_TRIM_TAB   s_ov2680_Resolution_Trim_Tab
#define VIDEO_INFO     s_ov2680_video_info
#define MODULE_INFO    s_ov2680_module_info_tab
#define RES_TAB_RAW        s_ov2680_resolution_tab_raw

static cmr_int ov2680_drv_set_video_mode(cmr_handle handle, cmr_uint param) {
    SENSOR_REG_T_PTR sensor_reg_ptr;
    cmr_u16 i = 0x00;
    cmr_u32 mode = 0;
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    if (param >= SENSOR_VIDEO_MODE_MAX)
        return 0;

    if(sns_drv_cxt->ops_cb.get_mode) {
        ret = sns_drv_cxt->ops_cb.get_mode(handle, &mode);
        if (SENSOR_SUCCESS != ret) {
            SENSOR_LOGI("fail.");
            return SENSOR_FAIL;
        }
    }

    if (PNULL == VIDEO_INFO[mode].setting_ptr) {
        SENSOR_LOGI("fail.");
        return SENSOR_FAIL;
    }

    sensor_reg_ptr = (SENSOR_REG_T_PTR)&VIDEO_INFO[mode].setting_ptr[param];
    if (PNULL == sensor_reg_ptr) {
        SENSOR_LOGI("fail.");
        return SENSOR_FAIL;
    }

    for (i = 0x00; (0xffff != sensor_reg_ptr[i].reg_addr) ||
                   (0xff != sensor_reg_ptr[i].reg_value); i++) {
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, sensor_reg_ptr[i].reg_addr,
                        sensor_reg_ptr[i].reg_value);
    }

    SENSOR_LOGI("%lu", param);
    return 0;
}

static cmr_int ov2680_drv_init_fps_info(cmr_handle handle) {
    cmr_int rtn = SENSOR_SUCCESS;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    struct sensor_fps_info *fps_info = sns_drv_cxt->fps_info;
    struct sensor_trim_tag *trim_info = sns_drv_cxt->trim_tab_info;
    struct sensor_static_info *static_info = sns_drv_cxt->static_info;
    SENSOR_LOGI("E:");

    if (!fps_info->is_init) {
        cmr_u32 i, modn, tempfps = 0;
        SENSOR_LOGI("start init");
        for (i = 0; i < SENSOR_MODE_MAX; i++) {
            // max fps should be multiple of 30,it calulated from line_time and
            // frame_line
            tempfps = trim_info[i].line_time * trim_info[i].frame_line;
            if (0 != tempfps) {
                tempfps = 1000000000 / tempfps;
                modn = tempfps / 30;
                if (tempfps > modn * 30)
                    modn++;
                fps_info->sensor_mode_fps[i].max_fps = modn * 30;
                if (fps_info->sensor_mode_fps[i].max_fps > 30) {
                    fps_info->sensor_mode_fps[i].is_high_fps = 1;
                    fps_info->sensor_mode_fps[i].high_fps_skip_num =
                        fps_info->sensor_mode_fps[i].max_fps / 30;
                }
                if (fps_info->sensor_mode_fps[i].max_fps >
                    static_info->max_fps) {
                    static_info->max_fps = fps_info->sensor_mode_fps[i].max_fps;
                }
            }
            SENSOR_LOGI("mode %d,tempfps %d,frame_len %d,line_time: %d ", i,
                        tempfps, trim_info[i].frame_line, trim_info[i].line_time);
            SENSOR_LOGI("mode %d,max_fps: %d ", i,
                        fps_info->sensor_mode_fps[i].max_fps);
            SENSOR_LOGI("is_high_fps: %d,highfps_skip_num %d",
                fps_info->sensor_mode_fps[i].is_high_fps,
                fps_info->sensor_mode_fps[i].high_fps_skip_num);
        }
        fps_info->is_init = 1;
    }
    SENSOR_LOGV("X");
    return rtn;
}

static cmr_int ov2680_drv_power_on(cmr_handle handle, cmr_uint power_on) {
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    struct module_cfg_info *module_info = sns_drv_cxt->module_info;

    SENSOR_AVDD_VAL_E dvdd_val = module_info->dvdd_val;
    SENSOR_AVDD_VAL_E avdd_val = module_info->avdd_val;
    SENSOR_AVDD_VAL_E iovdd_val = module_info->iovdd_val;
    BOOLEAN power_down = MIPI_RAW_INFO.power_down_level;
    BOOLEAN reset_level = MIPI_RAW_INFO.reset_pulse_level;

    if (SENSOR_TRUE == power_on) {
        hw_sensor_power_down(sns_drv_cxt->hw_handle, power_down);
        hw_sensor_set_reset_level(sns_drv_cxt->hw_handle, reset_level);
        hw_sensor_set_mclk(sns_drv_cxt->hw_handle, SENSOR_DEFALUT_MCLK);
        usleep(1000);
        hw_sensor_set_avdd_val(sns_drv_cxt->hw_handle, avdd_val);
        hw_sensor_set_iovdd_val(sns_drv_cxt->hw_handle, iovdd_val);
        usleep(1000);
        hw_sensor_power_down(sns_drv_cxt->hw_handle, !power_down);
        usleep(3 * 1000);
        hw_sensor_set_reset_level(sns_drv_cxt->hw_handle, !reset_level);
    } else {
        hw_sensor_set_mclk(sns_drv_cxt->hw_handle, SENSOR_DISABLE_MCLK);
        usleep(1000);
        hw_sensor_power_down(sns_drv_cxt->hw_handle, power_down);
        hw_sensor_set_reset_level(sns_drv_cxt->hw_handle, reset_level);
        usleep(500);
        hw_sensor_set_avdd_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED);
        hw_sensor_set_iovdd_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED);
    }
    SENSOR_LOGI("(1:on, 0:off): %lu", power_on);
    return SENSOR_SUCCESS;
}

static cmr_int ov2680_drv_identify(cmr_handle handle, cmr_uint param) {
#define ov2680_PID_VALUE 0x26
#define ov2680_PID_ADDR 0x300a
#define ov2680_VER_VALUE 0x80
#define ov2680_VER_ADDR 0x300b
    UNUSED(param);
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    cmr_u8 pid_value = 0x00;
    cmr_u8 ver_value = 0x00;
    cmr_u32 ret_value = SENSOR_FAIL;

    SENSOR_LOGI("SENSOR_ov2680: mipi raw identify\n");

    pid_value = hw_sensor_read_reg(sns_drv_cxt->hw_handle, ov2680_PID_ADDR);

    if (ov2680_PID_VALUE == pid_value) {
        ver_value = hw_sensor_read_reg(sns_drv_cxt->hw_handle, ov2680_VER_ADDR);
        SENSOR_LOGI("PID = %x, VER = %x", pid_value, ver_value);
        if (ov2680_VER_VALUE == ver_value) {
            ov2680_drv_init_fps_info(handle);
            ret_value = SENSOR_SUCCESS;
            SENSOR_LOGI("this is ov2680 sensor !");
        } else {
            SENSOR_LOGI("SENSOR_ov2680: Identify this is OV%x%x sensor !",
                        pid_value, ver_value);
        }
    } else {
        SENSOR_LOGI("identify fail,pid_value=%d", pid_value);
    }

    return ret_value;
}

static cmr_int ov2680_drv_write_exp_dummy(cmr_handle handle,
                                                   cmr_u16 expsure_line,
                                                   cmr_u16 dummy_line,
                                                   cmr_u16 size_index) {
    cmr_int ret_value = SENSOR_SUCCESS;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    cmr_u16 frame_len = 0x00;
    cmr_u16 frame_len_cur = 0x00;
    cmr_u16 max_frame_len = 0x00;
    cmr_u16 value = 0x00;
    cmr_u16 value0 = 0x00;
    cmr_u16 value1 = 0x00;
    cmr_u16 value2 = 0x00;
    cmr_u16 frame_interval = 0x00;
    cmr_u16 offset = 0;
    struct sensor_trim_tag *trim_info = sns_drv_cxt->trim_tab_info;

    frame_interval = (cmr_u16)(((expsure_line + dummy_line) *
                    trim_info[size_index].line_time) / 1000000);
    SENSOR_LOGI("current mode = %d, exposure_line = %d, dummy_line= %d, "
                "frame_interval= %d ms",
                size_index, expsure_line, dummy_line, frame_interval);

    max_frame_len = trim_info[size_index].frame_line;

    if (0x00 != max_frame_len) {
        if (dummy_line > FRAME_OFFSET)
            offset = dummy_line;
        else
            offset = FRAME_OFFSET;
        frame_len = ((expsure_line + offset) > max_frame_len)
                        ? (expsure_line + offset)
                        : max_frame_len;

        if (0x00 != (0x01 & frame_len)) {
            frame_len += 0x01;
        }

        frame_len_cur = (hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x380e) & 0xff) << 8;
        frame_len_cur |= hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x380f) & 0xff;

        if (frame_len_cur != frame_len) {
            value = (frame_len)&0xff;
            ret_value = hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x380f, value);
            value = (frame_len >> 0x08) & 0xff;
            ret_value = hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x380e, value);
        }
    }

    value = (expsure_line << 0x04) & 0xff;
    ret_value = hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3502, value);
    value = (expsure_line >> 0x04) & 0xff;
    ret_value = hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3501, value);
    value = (expsure_line >> 0x0c) & 0x0f;
    ret_value = hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3500, value);

    return ret_value;
}

static cmr_int ov2680_drv_write_exposure(cmr_handle handle,
                                           cmr_uint param) {
    cmr_int ret_value = SENSOR_SUCCESS;
    cmr_u32 expsure_line = 0x00;
    cmr_u32 dummy_line = 0x00;
    cmr_u32 size_index = 0x00;
    SENSOR_IC_CHECK_HANDLE(handle);

    expsure_line = param & 0xffff;
    dummy_line = (param >> 0x10) & 0x0fff;
    size_index = (param >> 0x1c) & 0x0f;

    ret_value =
       ov2680_drv_write_exp_dummy(handle, expsure_line, dummy_line, size_index);

    return ret_value;
}

static cmr_int ov2680_drv_ex_write_exposure(cmr_handle handle,
                                              cmr_uint param) {
    cmr_int ret_value = SENSOR_SUCCESS;
    cmr_u16 exposure_line = 0x00;
    cmr_u16 dummy_line = 0x00;
    cmr_u16 size_index = 0x00;
    struct sensor_ex_exposure *ex = (struct sensor_ex_exposure *)param;

    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(ex );

    exposure_line = ex->exposure;
    dummy_line = ex->dummy;
    size_index = ex->size_index;

    ret_value =
       ov2680_drv_write_exp_dummy(handle, exposure_line, dummy_line, size_index);

    return ret_value;
}

static cmr_int ov2680_drv_write_gain(cmr_handle handle,
                                       cmr_uint param) {
    cmr_int ret_value = SENSOR_SUCCESS;
    cmr_u16 value = 0x00;
    cmr_u32 real_gain = 0;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    real_gain = param >> 3;
    if (real_gain > 0x7ff) {
        real_gain = 0x7ff;
    }

    SENSOR_LOGI("SENSOR_ov2680: real_gain:0x%x, param: %lu", real_gain, param);

    value = real_gain & 0xff;
    ret_value = hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x350b, value); /*0-7*/
    value = (real_gain >> 0x08) & 0x07;
    ret_value = hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x350a, value); /*8*/

    return ret_value;
}

static struct sensor_reg_tag ov2680_shutter_reg[] = {
    {0x3502, 0}, {0x3501, 0}, {0x3500, 0},
};

static struct sensor_i2c_reg_tab ov2680_shutter_tab = {
    .settings = ov2680_shutter_reg, .size = ARRAY_SIZE(ov2680_shutter_reg),
};

static struct sensor_reg_tag ov2680_again_reg[] = {
    {0x350b, 0}, {0x350a, 0},
};

static struct sensor_i2c_reg_tab ov2680_again_tab = {
    .settings = ov2680_again_reg, .size = ARRAY_SIZE(ov2680_again_reg),
};

static struct sensor_reg_tag ov2680_dgain_reg[] = {};

struct sensor_i2c_reg_tab ov2680_dgain_tab = {
    .settings = ov2680_dgain_reg, .size = ARRAY_SIZE(ov2680_dgain_reg),
};

static struct sensor_reg_tag ov2680_frame_length_reg[] = {
    {0x380f, 0x0e}, {0x380e, 0x05},
};

static struct sensor_i2c_reg_tab ov2680_frame_length_tab = {
    .settings = ov2680_frame_length_reg,
    .size = ARRAY_SIZE(ov2680_frame_length_reg),
};

static struct sensor_aec_i2c_tag ov2680_aec_info = {
    .slave_addr = ov2680_I2C_ADDR_W,
    .addr_bits_type = SENSOR_I2C_REG_16BIT,
    .data_bits_type = SENSOR_I2C_VAL_8BIT,
    .shutter = &ov2680_shutter_tab,
    .again = &ov2680_again_tab,
    .dgain = &ov2680_dgain_tab,
    .frame_length = &ov2680_frame_length_tab,
};

static cmr_int ov2680_drv_calc_exposure(cmr_handle handle,
                                     cmr_u32 expsure_line, cmr_u32 dummy_line,
                                     cmr_u16 size_index,
                                     struct sensor_aec_i2c_tag *aec_info) {
    cmr_int ret_value = SENSOR_SUCCESS;
    cmr_u16 frame_len = 0x00;
    cmr_u16 frame_len_cur = 0x00;
    cmr_u16 max_frame_len = 0x00;
    cmr_u16 value = 0x00;
    cmr_u16 value0 = 0x00;
    cmr_u16 value1 = 0x00;
    cmr_u16 value2 = 0x00;
    cmr_u16 frame_interval = 0x00;
    int32_t offset = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    struct sensor_trim_tag *trim_info = sns_drv_cxt->trim_tab_info;

    frame_interval = (cmr_u16)(((expsure_line + dummy_line) *
                    trim_info[size_index].line_time) / 1000000);
    SENSOR_LOGI("current mode = %d, exposure_line = %d, dummy_line= %d, "
                "frame_interval= %d ms",
                size_index, expsure_line, dummy_line, frame_interval);

    max_frame_len = trim_info[size_index].frame_line;

    if (0x00 != max_frame_len) {
        if (dummy_line > FRAME_OFFSET)
            offset = dummy_line;
        else
            offset = FRAME_OFFSET;
        frame_len = ((expsure_line + offset) > max_frame_len)
                        ? (expsure_line + offset)
                        : max_frame_len;

        if (0x00 != (0x01 & frame_len)) {
            frame_len += 0x01;
        }

        frame_len_cur = (hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x380e) & 0xff) << 8;
        frame_len_cur |= hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x380f) & 0xff;

        value = (frame_len)&0xff;
        aec_info->frame_length->settings[0].reg_value = value;
        value = (frame_len >> 0x08) & 0xff;
        aec_info->frame_length->settings[1].reg_value = value;
    }

    value = (expsure_line << 0x04) & 0xff;
    aec_info->shutter->settings[0].reg_value = value;
    value = (expsure_line >> 0x04) & 0xff;
    aec_info->shutter->settings[1].reg_value = value;
    value = (expsure_line >> 0x0c) & 0x0f;
    aec_info->shutter->settings[2].reg_value = value;

    return 0;
}

static void ov2680_drv_calc_gain(cmr_u32 gain,
                             struct sensor_aec_i2c_tag *aec_info) {
    cmr_u16 value = 0x00;
    cmr_u32 real_gain = 0;
    SENSOR_IC_CHECK_PTR_VOID(aec_info);
    real_gain = gain >> 3;
    if (real_gain > 0x3ff) {
        real_gain = 0x3ff;
    }

    SENSOR_LOGI("SENSOR_ov2680: real_gain:0x%x", real_gain);

    value = real_gain & 0xff;
    aec_info->again->settings[0].reg_value = value;
    value = (real_gain >> 0x08) & 0x03;
    aec_info->again->settings[1].reg_value = value;
}

static cmr_int ov2680_drv_read_aec_info(cmr_handle handle, cmr_uint param) {
    cmr_int ret_value = SENSOR_SUCCESS;
    cmr_u16 exposure_line = 0x00;
    cmr_u16 dummy_line = 0x00;
    cmr_u16 mode = 0x00;
    cmr_u32 gain = 0;
    struct sensor_aec_reg_info *info = (struct sensor_aec_reg_info *)param;

    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(info);

    info->aec_i2c_info_out = &ov2680_aec_info;

    exposure_line = info->exp.exposure;
    dummy_line = info->exp.dummy;
    mode = info->exp.size_index;

    ov2680_drv_calc_exposure(handle, exposure_line, dummy_line, mode,
                         &ov2680_aec_info);

    ov2680_drv_calc_gain(info->gain, &ov2680_aec_info);
    return ret_value;
}

static cmr_int ov2680_drv_read_gain(cmr_handle handle, cmr_uint param) {
    cmr_int rtn = SENSOR_SUCCESS;
    cmr_u16 value = 0x00;
    cmr_u32 gain = 0;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    value = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x350b); /*0-7*/
    gain = value & 0xff;
    value = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x350a); /*8*/
    gain |= (value << 0x08) & 0x300;

    //s_ov2680_gain = (int)gain;
    sns_drv_cxt->sensor_gain.gain= (int)gain;

    SENSOR_LOGI(" gain: 0x%x", gain);

    return rtn;
}

static cmr_int ov2680_drv_set_gain16(cmr_handle handle, int gain16) {
    // write gain, 16 = 1x
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    int temp;
    gain16 = gain16 & 0x3ff;

    temp = gain16 & 0xff;
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x350b, temp);

    temp = gain16 >> 8;
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x350a, temp);
    SENSOR_LOGI("gain %d", gain16);

    return 0;
}

static cmr_int ov2680_drv_set_shutter(cmr_handle handle, int shutter) {
    // write shutter, in number of line period
    int temp;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    shutter = shutter & 0xffff;

    temp = shutter & 0x0f;
    temp = temp << 4;
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3502, temp);
    //Sensor_WriteReg(0x3502, temp);

    temp = shutter & 0xfff;
    temp = temp >> 4;
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3501, temp);

    temp = shutter >> 12;
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3500, temp);

    SENSOR_LOGI("shutter %d", shutter);

    return 0;
}

static void ov2680_drv_calculate_hdr_exposure(cmr_handle handle, int capture_gain16,
                                                         int capture_VTS, int capture_shutter) {
    SENSOR_IC_CHECK_HANDLE_VOID(handle);
    ov2680_drv_set_gain16(handle, capture_gain16);

    ov2680_drv_set_shutter(handle, capture_shutter);
}

static cmr_int ov2680_drv_set_ev(cmr_handle handle, cmr_uint param) {
    cmr_int rtn = SENSOR_SUCCESS;
    cmr_u32 ev = param;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    PRIVATE_DATA *pri_data = sizeof(PRIVATE_DATA) > 4?
                             (PRIVATE_DATA *)sns_drv_cxt->privata_data.buffer:
                             (PRIVATE_DATA *)&sns_drv_cxt->privata_data.data;
    SENSOR_IC_CHECK_PTR(pri_data);

    //SENSOR_LOGI("_ov2680_SetEV param: %lu,0x%x,0x%x,0x%x", param,
    //            s_ov2680_gain, s_capture_VTS, s_capture_shutter);
    SENSOR_LOGI("param: %lu,0x%x,0x%x,0x%x", param, sns_drv_cxt->sensor_gain.gain,
                 pri_data->VTS, sns_drv_cxt->exp_line);

    switch (ev) {
    case SENSOR_HDR_EV_LEVE_0:
        ov2680_drv_calculate_hdr_exposure(handle, sns_drv_cxt->sensor_gain.gain / 2,
                               pri_data->VTS, sns_drv_cxt->exp_line / 2);
        break;
    case SENSOR_HDR_EV_LEVE_1:
        ov2680_drv_calculate_hdr_exposure(handle, sns_drv_cxt->sensor_gain.gain,
                               pri_data->VTS, sns_drv_cxt->exp_line);
        break;
    case SENSOR_HDR_EV_LEVE_2:
        ov2680_drv_calculate_hdr_exposure(handle, sns_drv_cxt->sensor_gain.gain,
                               pri_data->VTS, sns_drv_cxt->exp_line * 4);
        break;
    default:
        break;
    }
    return rtn;
}

static cmr_int ov2680_drv_ext_func(cmr_handle handle, cmr_uint ctl_param) {
    cmr_int rtn = SENSOR_SUCCESS;
    SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR)ctl_param;

    SENSOR_IC_CHECK_PTR(ext_ptr);
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("cmd %d", ext_ptr->cmd);
    switch (ext_ptr->cmd) {
    case SENSOR_EXT_EV:
        rtn = ov2680_drv_set_ev(handle, ext_ptr->param);
        break;
    default:
        break;
    }

    return rtn;
}

static cmr_u16 ov2680_drv_get_shutter(cmr_handle handle) {
    cmr_u16 shutter = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    shutter = (hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x03500) & 0x0f);
    shutter = (shutter << 8) + hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x3501);
    shutter = (shutter << 4) + (hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x3502) >> 4);

    return shutter;
}

static int ov2680_drv_get_vts(cmr_handle handle) {
    int VTS;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    VTS = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x380e);
    VTS = (VTS << 8) + hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x380f);

    return VTS;
}

static cmr_int ov2680_drv_before_snapshot(cmr_handle handle,
                                                   cmr_uint param) {
    cmr_u8 ret_l, ret_m, ret_h;
    cmr_u32 capture_exposure, preview_maxline;
    cmr_u32 capture_maxline, preview_exposure;
    cmr_u32 capture_mode = param & 0xffff;
    cmr_u32 preview_mode = (param >> 0x10) & 0xffff;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    PRIVATE_DATA *pri_data = sizeof(PRIVATE_DATA) > 4?
                             sns_drv_cxt->privata_data.buffer:
                             &sns_drv_cxt->privata_data.data;
    SENSOR_IC_CHECK_PTR(pri_data);
    cmr_u32 prv_linetime = sns_drv_cxt->trim_tab_info[preview_mode].line_time;
    cmr_u32 cap_linetime = sns_drv_cxt->trim_tab_info[capture_mode].line_time;

    SENSOR_LOGI("BeforeSnapshot mode: %lu", param);

    if (preview_mode == capture_mode) {
        SENSOR_LOGI("prv mode equal to capmode");
        goto CFG_INFO;
    }
    ret_h = (cmr_u8)hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x3500);
    ret_m = (cmr_u8)hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x3501);
    ret_l = (cmr_u8)hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x3502);
    preview_exposure = (ret_h << 12) + (ret_m << 4) + (ret_l >> 4);

    ret_h = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x380e);
    ret_l = (cmr_u8)hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x380f);
    preview_maxline = (ret_h << 8) + ret_l;

    if(sns_drv_cxt->ops_cb.set_mode)
        sns_drv_cxt->ops_cb.set_mode(sns_drv_cxt->caller_handle, capture_mode);
    if(sns_drv_cxt->ops_cb.set_mode_wait_done)
    sns_drv_cxt->ops_cb.set_mode_wait_done(sns_drv_cxt->caller_handle);

    if (prv_linetime == cap_linetime) {
        SENSOR_LOGI("prvline equal to capline");
        goto CFG_INFO;
    }

    ret_h = (cmr_u8)hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x380e);
    ret_l = (cmr_u8)hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x380f);
    capture_maxline = (ret_h << 8) + ret_l;

    capture_exposure = preview_exposure * prv_linetime / cap_linetime;

    if (0 == capture_exposure) {
        capture_exposure = 1;
    }

    if (capture_exposure > (capture_maxline - 4)) {
        capture_maxline = capture_exposure + 4;
        capture_maxline = (capture_maxline + 1) >> 1 << 1;
        ret_l = (unsigned char)(capture_maxline & 0x0ff);
        ret_h = (unsigned char)((capture_maxline >> 8) & 0xff);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x380e, ret_h);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x380f, ret_l);
    }

    ret_l = ((unsigned char)capture_exposure & 0xf) << 4;
    ret_m = (unsigned char)((capture_exposure & 0xfff) >> 4) & 0xff;
    ret_h = (unsigned char)(capture_exposure >> 12);

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3502, ret_l);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3501, ret_m);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3500, ret_h);

CFG_INFO:
    sns_drv_cxt->exp_line = ov2680_drv_get_shutter(handle);
    pri_data->VTS = ov2680_drv_get_vts(handle);
    ov2680_drv_read_gain(handle, capture_mode);

    if(sns_drv_cxt->ops_cb.set_exif_info){
        sns_drv_cxt->ops_cb.set_exif_info(sns_drv_cxt->caller_handle,
                SENSOR_EXIF_CTRL_EXPOSURETIME, sns_drv_cxt->exp_line);
    } else {
        sns_drv_cxt->exif_info.exposure_line = sns_drv_cxt->exp_line;
    }

    return SENSOR_SUCCESS;
}

static cmr_int ov2680_drv_after_snapshot(cmr_handle handle,
                                           cmr_uint param) {
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    SENSOR_LOGI("mode:%ld", param);

    if(sns_drv_cxt->ops_cb.set_mode)
       ret = sns_drv_cxt->ops_cb.set_mode(sns_drv_cxt->caller_handle, (cmr_u32)param);

    return ret;
}

static cmr_int ov2680_drv_stream_on(cmr_handle handle,
                                           cmr_uint param) {
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    SENSOR_LOGI("StreamOn");

#ifdef CONFIG_CAMERA_RT_REFOCUS
    al3200_mini_ctrl(param);
    al3200_mini_ctrl(1);
#endif
    usleep(100 * 1000);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0100, 0x01);
#if 1
    char value1[PROPERTY_VALUE_MAX];
    property_get("debug.camera.test.mode", value1, "0");
    if (!strcmp(value1, "1")) {
        SENSOR_LOGI("enable test mode");
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x5080, 0x80);
    }
#endif

    return 0;
}

static cmr_int ov2680_drv_stream_off(cmr_handle handle,
                                            cmr_uint param) {
    UNUSED(param);
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    SENSOR_LOGI("StreamOff");

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0100, 0x00);
    usleep(100 * 1000);

    return 0;
}

static cmr_int ov2680_drv_get_static_info(cmr_handle handle,
                                                     cmr_u32* param) {
    cmr_int rtn = SENSOR_SUCCESS;
    struct sensor_ex_info *ex_info = (struct sensor_ex_info *)param;

    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(ex_info);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    struct sensor_fps_info *fps_info = sns_drv_cxt->fps_info;
    struct sensor_static_info *static_info = sns_drv_cxt->static_info;
    struct module_cfg_info *module_info = sns_drv_cxt->module_info;
    if(!(fps_info && static_info && module_info)) {
        SENSOR_LOGE("error:null pointer checked.return");
        return SENSOR_FAIL;
    }

    // make sure we have get max fps of all settings.
    if (!fps_info->is_init) {
        ov2680_drv_init_fps_info(handle);
    }
    ex_info = (struct sensor_ex_info *)param;
    SENSOR_IC_CHECK_PTR(ex_info);

    ex_info->f_num = static_info->f_num;
    ex_info->focal_length = static_info->focal_length;
    ex_info->max_fps = static_info->max_fps;
    ex_info->max_adgain = static_info->max_adgain;
    ex_info->ois_supported = static_info->ois_supported;
    ex_info->pdaf_supported = static_info->pdaf_supported;
    ex_info->exp_valid_frame_num = static_info->exp_valid_frame_num;
    ex_info->clamp_level = static_info->clamp_level;
    ex_info->adgain_valid_frame_num = static_info->adgain_valid_frame_num;
    ex_info->preview_skip_num = module_info->preview_skip_num;
    ex_info->capture_skip_num = module_info->capture_skip_num;
    ex_info->name = (cmr_s8 *)MIPI_RAW_INFO.name;
    ex_info->sensor_version_info = (cmr_s8 *)MIPI_RAW_INFO.sensor_version_info;
    memcpy(&ex_info->fov_info, &static_info->fov_info, sizeof(static_info->fov_info));

    sensor_ic_print_static_info(SENSOR_NAME, ex_info);

    return rtn;
}

static cmr_int ov2680_drv_get_fps_info(cmr_handle handle, cmr_u32*param) {
    cmr_int rtn = SENSOR_SUCCESS;
    SENSOR_MODE_FPS_T *fps_info = (SENSOR_MODE_FPS_T *)param;;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param);

    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    struct sensor_fps_info *fps_data = sns_drv_cxt->fps_info;

    // make sure have inited fps of every sensor mode.
    if (!fps_data->is_init) {
        ov2680_drv_init_fps_info(handle);
    }
    uint32_t sensor_mode = fps_info->mode;
    fps_info->max_fps = fps_data->sensor_mode_fps[sensor_mode].max_fps;
    fps_info->min_fps = fps_data->sensor_mode_fps[sensor_mode].min_fps;
    fps_info->is_high_fps = fps_data->sensor_mode_fps[sensor_mode].is_high_fps;
    fps_info->high_fps_skip_num = fps_data->sensor_mode_fps[sensor_mode].high_fps_skip_num;
    SENSOR_LOGI(" mode %d, max_fps: %d", fps_info->mode, fps_info->max_fps);
    SENSOR_LOGI("min_fps: %d", fps_info->min_fps);
    SENSOR_LOGI("is_high_fps: %d", fps_info->is_high_fps);
    SENSOR_LOGI("high_fps_skip_num: %d", fps_info->high_fps_skip_num);

    return rtn;
}

static cmr_int ov2680_drv_access_val(cmr_handle handle,
                                             cmr_uint param) {
    cmr_int rtn = SENSOR_SUCCESS;
    SENSOR_VAL_T *param_ptr = (SENSOR_VAL_T *)param;
    cmr_u16 tmp;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param_ptr);

    SENSOR_LOGI("E: param_ptr = %p",
                param_ptr);

    SENSOR_LOGI("param_ptr->type=%x", param_ptr->type);
    switch (param_ptr->type) {
    case SENSOR_VAL_TYPE_GET_STATIC_INFO:
        rtn = ov2680_drv_get_static_info(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_GET_FPS_INFO:
        rtn = ov2680_drv_get_fps_info(handle, param_ptr->pval);
        break;
    default:
        break;
    }

    SENSOR_LOGI("X:");

    return rtn;
}

static cmr_int ov2680_drv_handle_create(
          struct sensor_ic_drv_init_para *init_param, cmr_handle* sns_ic_drv_handle) {
    cmr_int ret = SENSOR_SUCCESS;
    struct sensor_ic_drv_cxt * sns_drv_cxt = NULL;

    ret = sensor_ic_drv_create(init_param,sns_ic_drv_handle);
    if(SENSOR_SUCCESS != ret)
        return SENSOR_FAIL;
    sns_drv_cxt = *sns_ic_drv_handle;

    sensor_ic_set_match_module_info(sns_drv_cxt, ARRAY_SIZE(MODULE_INFO), MODULE_INFO);
    sensor_ic_set_match_resolution_info(sns_drv_cxt, ARRAY_SIZE(RES_TAB_RAW), RES_TAB_RAW);
    sensor_ic_set_match_trim_info(sns_drv_cxt, ARRAY_SIZE(RES_TRIM_TAB), RES_TRIM_TAB);
    sensor_ic_set_match_static_info(sns_drv_cxt, ARRAY_SIZE(STATIC_INFO), STATIC_INFO);
    sensor_ic_set_match_fps_info(sns_drv_cxt, ARRAY_SIZE(FPS_INFO), FPS_INFO);

    /*add private here*/
    return ret;
}

static cmr_int ov2680_drv_get_private_data(cmr_handle handle,
                                                     cmr_uint cmd, void**param){
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param);

    ret = sensor_ic_get_private_data(handle,cmd, param);
    return ret;
}

static cmr_int ov2680_drv_handle_delete(cmr_handle handle, void *param) {
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_IC_CHECK_HANDLE(handle);

    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    /*if has private data,you must release it here*/

    ret = sensor_ic_drv_delete(handle,param);
    return ret;
}

static struct sensor_ic_ops s_ov2680_ops_tab = {
    .create_handle = ov2680_drv_handle_create,
    .delete_handle = ov2680_drv_handle_delete,
    .get_data = ov2680_drv_get_private_data,
    .power = ov2680_drv_power_on,
    .identify = ov2680_drv_identify,
    .ex_write_exp = ov2680_drv_ex_write_exposure,
    .write_exp = ov2680_drv_write_exposure,
    .write_gain_value = ov2680_drv_write_gain,
    .read_aec_info = ov2680_drv_read_aec_info,

    /*the following interface called by oem layer*/
    .ext_ops = {
        [SENSOR_IOCTL_BEFORE_SNAPSHOT].ops = ov2680_drv_before_snapshot,
        [SENSOR_IOCTL_STREAM_ON].ops = ov2680_drv_stream_on,
        [SENSOR_IOCTL_STREAM_OFF].ops = ov2680_drv_stream_off,
        [SENSOR_IOCTL_EXT_FUNC].ops = ov2680_drv_ext_func,
        [SENSOR_IOCTL_VIDEO_MODE].ops = ov2680_drv_set_video_mode,
        [SENSOR_IOCTL_ACCESS_VAL].ops = ov2680_drv_access_val,
    }
};

