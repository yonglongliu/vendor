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
#include "sensor_ov5640_mipi_raw.h"

#undef LOG_TAG
#define LOG_TAG "ov5640_mipi_raw"

#define STATIC_INFO s_ov5640_static_info
#define FPS_INFO s_ov5640_mode_fps_info
#define MIPI_RAW_INFO g_ov5640_mipi_raw_info
#define RES_TRIM_TAB s_ov5640_Resolution_Trim_Tab
#define MODULE_INFO s_ov5640_module_info_tab
#define RES_TAB_RAW s_ov5640_resolution_Tab_RAW

static cmr_int ov5640_drv_power_on(cmr_handle handle, cmr_uint power_on) {
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    struct module_cfg_info *module_info = sns_drv_cxt->module_info;
    cmr_handle hw_handle = sns_drv_cxt->hw_handle;

    SENSOR_AVDD_VAL_E dvdd_val = module_info->dvdd_val;
    SENSOR_AVDD_VAL_E avdd_val = module_info->avdd_val;
    SENSOR_AVDD_VAL_E iovdd_val = module_info->iovdd_val;
    BOOLEAN power_down = MIPI_RAW_INFO.power_down_level;
    BOOLEAN reset_level = MIPI_RAW_INFO.reset_pulse_level;

    if (SENSOR_TRUE == power_on) {
        hw_sensor_power_down(hw_handle, power_down);
        // Open power
        // Sensor_SetMonitorVoltage(SENSOR_AVDD_2800MV);
        hw_sensor_set_voltage(hw_handle, dvdd_val, avdd_val, iovdd_val);
        usleep(20 * 1000);
        hw_sensor_set_mclk(hw_handle, SENSOR_DEFALUT_MCLK);
        usleep(10 * 1000);
        hw_sensor_power_down(hw_handle, !power_down);
        // Reset sensor
        hw_sensor_set_reset_level(hw_handle, reset_level);
    } else {
        hw_sensor_power_down(hw_handle, power_down);
        hw_sensor_set_mclk(hw_handle, SENSOR_DISABLE_MCLK);
        hw_sensor_set_voltage(hw_handle, SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED,
                              SENSOR_AVDD_CLOSED);
        // Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
    }
    SENSOR_PRINT("Power_On(1:on, 0:off): %d", power_on);
    return SENSOR_SUCCESS;
}

static cmr_int ov5640_drv_identify(cmr_handle handle, cmr_uint param) {
#define ov5640_PID_VALUE 0x56
#define ov5640_PID_ADDR 0x300A
#define ov5640_VER_VALUE 0x40
#define ov5640_VER_ADDR 0x300B

    cmr_u8 pid_value = 0x00;
    cmr_u8 ver_value = 0x00;
    cmr_u32 ret_value = SENSOR_FAIL;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_PRINT("SENSOR_OV5640: mipi raw identify\n");

    pid_value = hw_sensor_read_reg(sns_drv_cxt->hw_handle, ov5640_PID_ADDR);

    if (ov5640_PID_VALUE == pid_value) {
        ver_value = hw_sensor_read_reg(sns_drv_cxt->hw_handle, ov5640_VER_ADDR);
        SENSOR_LOGI("SENSOR_OV5640: Identify: PID = %x, VER = %x", pid_value,
                    ver_value);
        if (ov5640_VER_VALUE == ver_value) {
            // Sensor_InitRawTuneInfo();
            ret_value = SENSOR_SUCCESS;
            SENSOR_LOGI("SENSOR_OV5640: this is ov5640 mipi raw sensor !");
        } else {
            SENSOR_LOGI("SENSOR_OV5640: Identify this is OV%x%x sensor !",
                        pid_value, ver_value);
        }
    } else {
        SENSOR_LOGE("SENSOR_OV5640: identify fail,pid_value=%d", pid_value);
    }

    return ret_value;
}

static cmr_int ov5640_drv_write_exposure(cmr_handle handle, cmr_uint param) {
    cmr_int ret_value = SENSOR_SUCCESS;
    cmr_u16 expsure_line = 0x00;
    cmr_u16 dummy_line = 0x00;
    cmr_u16 value = 0x00;
    cmr_u16 value0 = 0x00;
    cmr_u16 value1 = 0x00;
    cmr_u16 value2 = 0x00;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    expsure_line = param & 0xffff;
    dummy_line = (param >> 0x10) & 0xffff;

    SENSOR_LOGI("write_exposure line:%d, dummy:%d", expsure_line, dummy_line);

    value = (expsure_line << 0x04) & 0xff;
    ret_value = hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3502, value);
    value = (expsure_line >> 0x04) & 0xff;
    ret_value = hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3501, value);
    value = (expsure_line >> 0x0c) & 0x0f;
    ret_value = hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3500, value);

    return ret_value;
}

static cmr_int ov5640_drv_write_gain(cmr_handle handle, cmr_uint param) {
    cmr_int ret_value = SENSOR_SUCCESS;
    cmr_u16 value = 0x00;
    cmr_u32 real_gain = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    real_gain = ((param & 0xf) + 16) * (((param >> 4) & 0x01) + 1) *
                (((param >> 5) & 0x01) + 1);
    real_gain = real_gain * (((param >> 6) & 0x01) + 1) *
                (((param >> 7) * 0x01) + 1) * (((param >> 8) * 0x01) + 1);

    SENSOR_LOGI("real_gain:0x%x, param: 0x%x", real_gain, param);

    value = real_gain & 0xff;
    ret_value =
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x350b, value); /*0-7*/
    value = (real_gain >> 0x08) & 0x03;
    ret_value =
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x350a, value); /*8*/

    return ret_value;
}

static cmr_int ov5640_drv_read_gain(cmr_handle handle, cmr_u32 param) {
    cmr_u32 rtn = SENSOR_SUCCESS;
    cmr_u16 value = 0x00;
    cmr_u32 gain = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    value = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x350b); /*0-7*/
    gain = value & 0xff;
    value = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x350a); /*8*/
    gain |= (value << 0x08) & 0x300;

    sns_drv_cxt->sensor_gain.gain = gain;

    SENSOR_LOGI("gain: 0x%x", gain);

    return rtn;
}

static cmr_int ov5640_drv_set_ev(cmr_handle handle, cmr_uint param) {
    cmr_int rtn = SENSOR_SUCCESS;
    SENSOR_EXT_FUN_T_PTR ext_ptr = (SENSOR_EXT_FUN_T_PTR)param;
    cmr_u16 value = 0x00;
    cmr_u32 gain = 0;
    cmr_u32 ev = ext_ptr->param;

    SENSOR_LOGI("param: 0x%x", ev);
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    gain = sns_drv_cxt->sensor_gain.gain;
    gain = (gain * ext_ptr->param) >> 0x06;

    value = gain & 0xff;
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x350b, value); /*0-7*/
    value = (gain >> 0x08) & 0x03;
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x350a, value); /*8*/

    return rtn;
}

/*==============================================================================
 * Description:
 * calculate fps for every sensor mode according to frame_line and line_time
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int ov5640_drv_init_fps_info(cmr_handle handle) {
    cmr_int rtn = SENSOR_SUCCESS;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    struct sensor_fps_info *fps_info = sns_drv_cxt->fps_info;
    struct sensor_trim_tag *trim_info = sns_drv_cxt->trim_tab_info;
    struct sensor_static_info *static_info = sns_drv_cxt->static_info;

    SENSOR_LOGI("E");
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
                        tempfps, trim_info[i].frame_line,
                        trim_info[i].line_time);
            SENSOR_LOGI("mode %d,max_fps: %d ", i,
                        fps_info->sensor_mode_fps[i].max_fps);
            SENSOR_LOGI("is_high_fps: %d,highfps_skip_num %d",
                        fps_info->sensor_mode_fps[i].is_high_fps,
                        fps_info->sensor_mode_fps[i].high_fps_skip_num);
        }
        fps_info->is_init = 1;
    }
    SENSOR_LOGI("X");
    return rtn;
}

static cmr_int ov5640_drv_get_static_info(cmr_handle handle, cmr_u32 *param) {
    cmr_int rtn = SENSOR_SUCCESS;
    struct sensor_ex_info *ex_info = (struct sensor_ex_info *)param;
    cmr_u32 up = 0;
    cmr_u32 down = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(ex_info);
    SENSOR_IC_CHECK_PTR(param);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    struct sensor_fps_info *fps_info = sns_drv_cxt->fps_info;
    struct sensor_static_info *static_info = sns_drv_cxt->static_info;
    struct module_cfg_info *module_info = sns_drv_cxt->module_info;

    // make sure we have get max fps of all settings.
    if (!fps_info->is_init) {
        ov5640_drv_init_fps_info(handle);
    }
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

    ex_info->pos_dis.up2hori = up;
    ex_info->pos_dis.hori2down = down;
    sensor_ic_print_static_info(SENSOR_NAME, ex_info);

    return rtn;
}

static cmr_int ov5640_drv_get_fps_info(cmr_handle handle, cmr_u32 *param) {
    cmr_int rtn = SENSOR_SUCCESS;
    SENSOR_MODE_FPS_T *fps_info = (SENSOR_MODE_FPS_T *)param;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(fps_info);
    SENSOR_IC_CHECK_PTR(param);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    struct sensor_fps_info *fps_data = sns_drv_cxt->fps_info;

    // make sure have inited fps of every sensor mode.
    if (!fps_data->is_init) {
        ov5640_drv_init_fps_info(handle);
    }
    cmr_u32 sensor_mode = fps_info->mode;
    fps_info->max_fps = fps_data->sensor_mode_fps[sensor_mode].max_fps;
    fps_info->min_fps = fps_data->sensor_mode_fps[sensor_mode].min_fps;
    fps_info->is_high_fps = fps_data->sensor_mode_fps[sensor_mode].is_high_fps;
    fps_info->high_fps_skip_num =
        fps_data->sensor_mode_fps[sensor_mode].high_fps_skip_num;
    SENSOR_LOGI("mode %d, max_fps: %d", fps_info->mode, fps_info->max_fps);
    SENSOR_LOGI("min_fps: %d", fps_info->min_fps);
    SENSOR_LOGI("is_high_fps: %d", fps_info->is_high_fps);
    SENSOR_LOGI("high_fps_skip_num: %d", fps_info->high_fps_skip_num);

    return rtn;
}

static cmr_int ov5640_drv_ext_func(cmr_handle handle, cmr_uint ctl_param) {
    cmr_int rtn = SENSOR_SUCCESS;
    SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR)ctl_param;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(ext_ptr);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    switch (ext_ptr->cmd) {
    case 10:
        rtn = ov5640_drv_set_ev(handle, ctl_param);
        break;
    default:
        break;
    }

    return rtn;
}

static cmr_int ov5640_drv_before_snapshot(cmr_handle handle, cmr_uint param) {
    cmr_u32 cap_mode = (param >> CAP_MODE_BITS);
    cmr_u8 ret_l, ret_m, ret_h;
    cmr_u32 capture_exposure, preview_maxline;
    cmr_u32 capture_maxline, preview_exposure;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_PRINT("%d,%d.", cap_mode, param);
    param = param & 0xffff;
    cmr_u32 prv_linetime =
        sns_drv_cxt->trim_tab_info[SENSOR_MODE_PREVIEW_ONE].line_time;
    cmr_u32 cap_linetime = sns_drv_cxt->trim_tab_info[param].line_time;

    if (SENSOR_MODE_PREVIEW_ONE >= param) {
        ov5640_drv_read_gain(handle, 0x00);
        SENSOR_PRINT("prvmode equal to capmode");
        return SENSOR_SUCCESS;
    }

    ret_h = (cmr_u8)hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x3500);
    ret_m = (cmr_u8)hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x3501);
    ret_l = (cmr_u8)hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x3502);
    preview_exposure = (ret_h << 12) + (ret_m << 4) + (ret_l >> 4);

    ret_h = (cmr_u8)hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x380e);
    ret_l = (cmr_u8)hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x380f);
    preview_maxline = (ret_h << 8) + ret_l;

    if (sns_drv_cxt->ops_cb.set_mode)
        sns_drv_cxt->ops_cb.set_mode(sns_drv_cxt->caller_handle, param);

    if (prv_linetime == cap_linetime) {
        SENSOR_PRINT("prvline equal to capline");
        return SENSOR_SUCCESS;
    }

    ret_h = (cmr_u8)hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x380e);
    ret_l = (cmr_u8)hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x380f);
    capture_maxline = (ret_h << 8) + ret_l;
    capture_exposure = preview_exposure * prv_linetime / cap_linetime;

    if (0 == capture_exposure) {
        capture_exposure = 1;
    }

    ret_l = ((unsigned char)capture_exposure) << 4;
    ret_m = (unsigned char)(capture_exposure >> 4) & 0xff;
    ret_h = (unsigned char)(capture_exposure >> 12);

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3502, ret_l);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3501, ret_m);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3500, ret_h);

    ov5640_drv_read_gain(handle, 0x00);

    if (sns_drv_cxt->ops_cb.set_exif_info) {
        sns_drv_cxt->ops_cb.set_exif_info(sns_drv_cxt->caller_handle,
                                          SENSOR_EXIF_CTRL_EXPOSURETIME,
                                          capture_exposure);
    }

    return SENSOR_SUCCESS;
}

static cmr_int ov5640_drv_after_snapshot(cmr_handle handle, cmr_uint param) {
    SENSOR_PRINT("after_snapshot mode:%d", param);
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    if (sns_drv_cxt->ops_cb.set_mode)
        sns_drv_cxt->ops_cb.set_mode(sns_drv_cxt->caller_handle,
                                     (cmr_u32)param);

    return SENSOR_SUCCESS;
}

static cmr_int ov5640_drv_stream_on(cmr_handle handle, cmr_uint param) {
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    SENSOR_PRINT("StreamOn");

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3008, 0x02);

    return 0;
}

static cmr_int ov5640_drv_stream_off(cmr_handle handle, cmr_uint param) {
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_PRINT("StreamOff");

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3008, 0x42);
    usleep(100 * 1000);

    return 0;
}

/*==============================================================================
 * Description:
 * cfg otp setting
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int ov5640_drv_access_val(cmr_handle handle, cmr_uint param) {
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_VAL_T *param_ptr = (SENSOR_VAL_T *)param;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param_ptr);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("param_ptr->type=%x", param_ptr->type);
    switch (param_ptr->type) {
    case SENSOR_VAL_TYPE_GET_STATIC_INFO:
        ret = ov5640_drv_get_static_info(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_GET_FPS_INFO:
        ret = ov5640_drv_get_fps_info(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_SET_SENSOR_CLOSE_FLAG:
        ret = sns_drv_cxt->is_sensor_close = 1;
        break;
    default:
        break;
    }

    return ret;
}

static cmr_int
ov5640_drv_handle_create(struct sensor_ic_drv_init_para *init_param,
                         cmr_handle *sns_ic_drv_handle) {
    cmr_int ret = SENSOR_SUCCESS;
    struct sensor_ic_drv_cxt *sns_drv_cxt = NULL;

    ret = sensor_ic_drv_create(init_param, sns_ic_drv_handle);
    if (SENSOR_SUCCESS != ret)
        return SENSOR_FAIL;
    sns_drv_cxt = *sns_ic_drv_handle;

    sensor_ic_set_match_module_info(sns_drv_cxt, ARRAY_SIZE(MODULE_INFO),
                                    MODULE_INFO);
    sensor_ic_set_match_resolution_info(sns_drv_cxt, ARRAY_SIZE(RES_TAB_RAW),
                                        RES_TAB_RAW);
    sensor_ic_set_match_trim_info(sns_drv_cxt, ARRAY_SIZE(RES_TRIM_TAB),
                                  RES_TRIM_TAB);
    sensor_ic_set_match_static_info(sns_drv_cxt, ARRAY_SIZE(STATIC_INFO),
                                    STATIC_INFO);
    sensor_ic_set_match_fps_info(sns_drv_cxt, ARRAY_SIZE(FPS_INFO), FPS_INFO);

    /*init exif info,this will be deleted in the future*/
    ov5640_drv_init_fps_info(sns_drv_cxt);
    /*add private here*/
    return ret;
}

static cmr_int ov5640_drv_get_private_data(cmr_handle handle, cmr_uint cmd,
                                           void **param) {
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param);

    ret = sensor_ic_get_private_data(handle, cmd, param);
    return ret;
}

static cmr_int ov5640_drv_handle_delete(cmr_handle handle, void *param) {
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_IC_CHECK_HANDLE(handle);

    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    /*if has private data,you must release it here*/

    ret = sensor_ic_drv_delete(handle, param);
    return ret;
}

static struct sensor_ic_ops s_ov5640_ops_tab = {
    .create_handle = ov5640_drv_handle_create,
    .delete_handle = ov5640_drv_handle_delete,
    .get_data = ov5640_drv_get_private_data,
    .power = ov5640_drv_power_on,
    .identify = ov5640_drv_identify,

    .write_exp = ov5640_drv_write_exposure,
    .write_gain_value = ov5640_drv_write_gain,

    /*the following interface called by oem layer*/
    .ext_ops = {
            [SENSOR_IOCTL_BEFORE_SNAPSHOT].ops = ov5640_drv_before_snapshot,
            [SENSOR_IOCTL_STREAM_ON].ops = ov5640_drv_stream_on,
            [SENSOR_IOCTL_STREAM_OFF].ops = ov5640_drv_stream_off,
            [SENSOR_IOCTL_EXT_FUNC].ops = ov5640_drv_ext_func,
            [SENSOR_IOCTL_ACCESS_VAL].ops = ov5640_drv_access_val,
    }};
