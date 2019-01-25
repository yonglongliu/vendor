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

#include "sensor_ov13850_mipi_raw.h"

#undef LOG_TAG
#define LOG_TAG "ov13850_mipi_raw"

#define STATIC_INFO s_ov13850_static_info
#define FPS_INFO s_ov13850_mode_fps_info
#define MIPI_RAW_INFO g_ov13850_mipi_raw_info
#define RES_TRIM_TAB s_ov13850_resolution_trim_tab
#define VIDEO_INFO s_ov13850_video_info
#define MODULE_INFO s_ov13850_module_info_tab
#define RES_TAB_RAW s_ov13850_resolution_tab_raw

static cmr_int ov13850_drv_set_video_mode(cmr_handle handle, cmr_uint param) {
    SENSOR_REG_T_PTR sensor_reg_ptr;
    cmr_u16 i = 0x00;
    cmr_u32 mode = 0;
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    if (param >= SENSOR_VIDEO_MODE_MAX)
        return 0;

    if (sns_drv_cxt->ops_cb.get_mode) {
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
                   (0xff != sensor_reg_ptr[i].reg_value);
         i++) {
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, sensor_reg_ptr[i].reg_addr,
                            sensor_reg_ptr[i].reg_value);
    }

    SENSOR_LOGI("0x%lx", param);
    return 0;
}

static cmr_int ov13850_drv_power_on(cmr_handle handle, cmr_uint power_on) {
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    struct module_cfg_info *module_info = sns_drv_cxt->module_info;

    SENSOR_AVDD_VAL_E dvdd_val = module_info->dvdd_val;
    SENSOR_AVDD_VAL_E avdd_val = module_info->avdd_val;
    SENSOR_AVDD_VAL_E iovdd_val = module_info->iovdd_val;
    BOOLEAN power_down = MIPI_RAW_INFO.power_down_level;
    BOOLEAN reset_level = MIPI_RAW_INFO.reset_pulse_level;

    if (SENSOR_TRUE == power_on) {
        hw_sensor_set_mclk(sns_drv_cxt->hw_handle, SENSOR_DISABLE_MCLK);
        hw_sensor_set_voltage(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED,
                              SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
        hw_sensor_set_reset_level(sns_drv_cxt->hw_handle, reset_level);
        hw_sensor_power_down(sns_drv_cxt->hw_handle, power_down);
        usleep(10 * 1000);

        hw_sensor_set_iovdd_val(sns_drv_cxt->hw_handle, iovdd_val);
        usleep(1000);
        hw_sensor_set_avdd_val(sns_drv_cxt->hw_handle, avdd_val);
        usleep(1000);
        hw_sensor_set_dvdd_val(sns_drv_cxt->hw_handle, dvdd_val);

        usleep(10 * 1000);
        hw_sensor_power_down(sns_drv_cxt->hw_handle, !power_down);
        usleep(1000);
        hw_sensor_set_reset_level(sns_drv_cxt->hw_handle, !reset_level);
        usleep(20 * 1000);
        hw_sensor_set_mclk(sns_drv_cxt->hw_handle, SENSOR_DEFALUT_MCLK);
        usleep(20 * 1000);

    } else {
        usleep(4 * 1000);
        hw_sensor_set_mclk(sns_drv_cxt->hw_handle, SENSOR_DISABLE_MCLK);
        usleep(1000);
        hw_sensor_set_reset_level(sns_drv_cxt->hw_handle, reset_level);
        hw_sensor_power_down(sns_drv_cxt->hw_handle, power_down);
        usleep(1000);
        hw_sensor_set_dvdd_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED);
        usleep(1000);
        hw_sensor_set_iovdd_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED);
        hw_sensor_set_avdd_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED);
    }
    SENSOR_LOGI("Power_On(1:on, 0:off): %ld", power_on);
    return SENSOR_SUCCESS;
}

static cmr_int ov13850_drv_identify(cmr_handle handle, cmr_uint param) {
#define ov13850_PID_VALUE 0xD8
#define ov13850_PID_ADDR 0x300A
#define ov13850_VER_VALUE 0x50
#define ov13850_VER_ADDR 0x300B

    cmr_u8 pid_value = 0x00;
    cmr_u8 ver_value = 0x00;
    cmr_int ret_value = SENSOR_FAIL;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("SENSOR_ov13850: mipi raw identify\n");

    SENSOR_LOGI("<tang>: sensor_id: %x\n",
                hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x302a));

    pid_value = hw_sensor_read_reg(sns_drv_cxt->hw_handle, ov13850_PID_ADDR);
    if (ov13850_PID_VALUE == pid_value) {
        ver_value =
            hw_sensor_read_reg(sns_drv_cxt->hw_handle, ov13850_VER_ADDR);
        SENSOR_LOGI("SENSOR_ov13850: Identify: PID = %x, VER = %x", pid_value,
                    ver_value);
        if (ov13850_VER_VALUE == ver_value) {
            SENSOR_LOGI("SENSOR_ov13850: this is ov13850 sensor !");
            // Sensor_ov13850_InitRawTuneInfo();
            ret_value = SENSOR_SUCCESS;
        } else {
            SENSOR_LOGI("SENSOR_ov13850: Identify this is OV%x%x sensor !",
                        pid_value, ver_value);
        }
    } else {
        SENSOR_LOGI("SENSOR_ov13850: identify fail,pid_value=%d", pid_value);
    }

    return ret_value;
}

static cmr_u32 ov13850_drv_get_shutter(cmr_handle handle) {
    // read shutter, in number of line period
    cmr_u32 shutter = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    shutter = (hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x03500) & 0x0f);
    shutter =
        (shutter << 8) + hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x3501);
    shutter = (shutter << 4) +
              (hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x3502) >> 4);

    return shutter;
}

static cmr_int ov13850_drv_set_shutter(cmr_handle handle, cmr_uint shutter) {
    // write shutter, in number of line period
    cmr_u16 temp = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    shutter = shutter & 0xffff;
    temp = shutter & 0x0f;
    temp = temp << 4;
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3502, temp);
    temp = shutter & 0xfff;
    temp = temp >> 4;
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3501, temp);
    temp = (shutter >> 12) & 0xf;
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3500, temp);

    return 0;
}

static cmr_u32 ov13850_drv_get_vts(cmr_handle handle) {
    // read VTS from register settings
    cmr_u32 VTS;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    VTS = hw_sensor_read_reg(sns_drv_cxt->hw_handle,
                             0x380e); // total vertical size[15:8] high byte
    VTS = ((VTS & 0x7f) << 8) +
          hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x380f);

    return VTS;
}

static cmr_int ov13850_drv_set_vts(cmr_handle handle, cmr_uint VTS) {
    // write VTS to registers
    cmr_u16 temp = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    temp = VTS & 0xff;
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x380f, temp);
    temp = (VTS >> 8) & 0x7f;
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x380e, temp);

    return 0;
}

// sensor gain
static cmr_u16 ov13850_drv_get_sensor_gain16(cmr_handle handle) {
    // read sensor gain, 16 = 1x
    cmr_u16 gain16 = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    gain16 = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x350a) & 0x03;
    gain16 = (gain16 << 8) + hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x350b);
    gain16 = ((gain16 >> 4) + 1) * ((gain16 & 0x0f) + 16);

    return gain16;
}

static cmr_u16 ov13850_drv_set_sensor_gain16(cmr_handle handle, int gain16) {
    // write sensor gain, 16 = 1x
    int gain_reg = 0;
    cmr_u16 temp = 0;
    int i = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    if (gain16 > 0x7ff) {
        gain16 = 0x7ff;
    }

    for (i = 0; i < 5; i++) {
        if (gain16 > 31) {
            gain16 = gain16 / 2;
            gain_reg = gain_reg | (0x10 << i);
        } else
            break;
    }
    gain_reg = gain_reg | (gain16 - 16);
    temp = gain_reg & 0xff;
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x350b, temp);
    temp = gain_reg >> 8;
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x350a, temp);
    return 0;
}

static cmr_int ov13850_drv_write_exposure(cmr_handle handle, cmr_uint param) {
    cmr_int ret_value = SENSOR_SUCCESS;
    cmr_u16 expsure_line = 0x00;
    cmr_u16 dummy_line = 0x00;
    cmr_u16 size_index = 0x00;
    cmr_u16 frame_len = 0x00;
    cmr_u16 frame_len_cur = 0x00;
    cmr_u16 max_frame_len = 0x00;
    cmr_u16 value = 0x00;
    cmr_u16 value0 = 0x00;
    cmr_u16 value1 = 0x00;
    cmr_u16 value2 = 0x00;
    cmr_u32 linetime = 0;

    expsure_line = param & 0xffff;
    dummy_line = (param >> 0x10) & 0x0fff;
    size_index = (param >> 0x1c) & 0x0f;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    struct sensor_trim_tag *trim_info = sns_drv_cxt->trim_tab_info;

    SENSOR_LOGI("write_exposure line:%d, dummy:%d, size_index:%d", expsure_line,
                dummy_line, size_index);

    max_frame_len = trim_info[size_index].frame_line;
    if (0x00 != max_frame_len) {
        frame_len = ((expsure_line + 8) > max_frame_len) ? (expsure_line + 8)
                                                         : max_frame_len;

        frame_len_cur = ov13850_drv_get_vts(handle);

        SENSOR_LOGI("frame_len: %d,   frame_len_cur:%d\n", frame_len,
                    frame_len_cur);

        if (frame_len_cur != frame_len) {
            ov13850_drv_set_vts(handle, frame_len);
        }
    }
    ov13850_drv_set_shutter(handle, expsure_line);

    sns_drv_cxt->exp_line = expsure_line;
    linetime = trim_info[size_index].line_time;
    sns_drv_cxt->exp_time = expsure_line * linetime / 10;

    if (sns_drv_cxt->ops_cb.set_exif_info) {
        sns_drv_cxt->ops_cb.set_exif_info(sns_drv_cxt->caller_handle,
                                          SENSOR_EXIF_CTRL_EXPOSURETIME,
                                          expsure_line);
    } else {
        sns_drv_cxt->exif_info.exposure_line = expsure_line;
    }

    return ret_value;
}

static cmr_int ov13850_drv_write_gain(cmr_handle handle, cmr_uint param) {
    cmr_int ret_value = SENSOR_SUCCESS;
    cmr_u16 value = 0x00;
    cmr_u32 real_gain = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    real_gain = param >> 3;
    if (real_gain > 0xf8) {
        real_gain = 0xf8;
    }

    SENSOR_LOGI("real_gain:0x%x, param: 0x%x", real_gain, param);

    value = real_gain & 0xff;
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x350b, value);
    value = (real_gain >> 8) & 0x7;
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x350a, value);

    return ret_value;
}

static cmr_int ov13850_drv_read_gain(cmr_handle handle, cmr_uint param) {
    cmr_int rtn = SENSOR_SUCCESS;
    cmr_u16 value = 0x00;
    cmr_u32 gain = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    value = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x350b); /*0-7*/
    gain = value & 0xff;
    value = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x350a); /*8*/
    gain |= (value << 0x08) & 0x300;

    sns_drv_cxt->sensor_gain.gain = (int)gain;

    SENSOR_LOGI("gain: 0x%x", gain);

    return rtn;
}

static cmr_int ov13850_drv_read_otp_gain(cmr_handle handle, cmr_u32 *param) {
    cmr_int rtn = SENSOR_SUCCESS;
    cmr_u16 value = 0x00;
    cmr_u32 gain = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param);

    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    value = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x350b); /*0-7*/
    gain = value & 0xff;
    value = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x350a); /*8*/
    gain |= (value << 0x08) & 0x300;

    *param = gain;

    return rtn;
}

static cmr_int ov13850_drv_before_snapshot(cmr_handle handle, cmr_uint param) {
    cmr_u8 ret_l, ret_m, ret_h;
    cmr_u32 capture_exposure, preview_maxline;
    cmr_u32 capture_maxline, preview_exposure;
    cmr_u32 capture_mode = param & 0xffff;
    cmr_u32 preview_mode = (param >> 0x10) & 0xffff;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("mode: 0x%08x", param);

    cmr_u32 prv_linetime = sns_drv_cxt->trim_tab_info[preview_mode].line_time;
    cmr_u32 cap_linetime = sns_drv_cxt->trim_tab_info[capture_mode].line_time;

    if (preview_mode == capture_mode) {
        SENSOR_LOGI("prv mode equal to capmode");
        goto CFG_INFO;
    }

    ret_h = (cmr_u8)hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x3500);
    ret_m = (cmr_u8)hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x3501);
    ret_l = (cmr_u8)hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x3502);
    preview_exposure = (ret_h << 12) + (ret_m << 4) + (ret_l >> 4);

    ret_h = (cmr_u8)hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x380e);
    ret_l = (cmr_u8)hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x380f);
    preview_maxline = (ret_h << 8) + ret_l;

    if (sns_drv_cxt->ops_cb.set_mode)
        sns_drv_cxt->ops_cb.set_mode(sns_drv_cxt->caller_handle, capture_mode);
    if (sns_drv_cxt->ops_cb.set_mode_wait_done)
        sns_drv_cxt->ops_cb.set_mode_wait_done(sns_drv_cxt->caller_handle);

    SENSOR_LOGI("prv_linetime = %d cap_linetime = %d\n", prv_linetime,
                cap_linetime);

    if (prv_linetime == cap_linetime) {
        SENSOR_LOGI("prvline equal to capline");
        // goto CFG_INFO;
    }

    ret_h = (cmr_u8)hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x380e);
    ret_l = (cmr_u8)hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x380f);
    capture_maxline = (ret_h << 8) + ret_l;

    capture_exposure = preview_exposure * prv_linetime / cap_linetime;

    if (0 == capture_exposure) {
        capture_exposure = 1;
    }
    SENSOR_LOGI("capture_exposure = %d   capture_maxline = %d\n",
                capture_exposure, capture_maxline);

    if (capture_exposure > (capture_maxline - 4)) {
        capture_maxline = capture_exposure + 4;
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
    usleep(200 * 1000);

CFG_INFO:
    sns_drv_cxt->exp_line = ov13850_drv_get_shutter(handle);
    *((cmr_u32 *)sns_drv_cxt->privata_data.data) = ov13850_drv_get_vts(handle);
    ov13850_drv_read_gain(handle, capture_mode);
    sns_drv_cxt->exp_time = sns_drv_cxt->exp_line * cap_linetime / 10;

    if (sns_drv_cxt->ops_cb.set_exif_info) {
        sns_drv_cxt->ops_cb.set_exif_info(sns_drv_cxt->caller_handle,
                                          SENSOR_EXIF_CTRL_EXPOSURETIME,
                                          sns_drv_cxt->exp_line);
    } else {
        sns_drv_cxt->exif_info.exposure_line = sns_drv_cxt->exp_line;
    }

    return SENSOR_SUCCESS;
}

static cmr_int ov13850_drv_after_snapshot(cmr_handle handle, cmr_uint param) {
    SENSOR_LOGI("after_snapshot mode:%ld", param);
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    if (sns_drv_cxt->ops_cb.set_mode) {
        sns_drv_cxt->ops_cb.set_mode(sns_drv_cxt->caller_handle,
                                     (cmr_u32)param);
    }

    return SENSOR_SUCCESS;
}

static cmr_int ov13850_drv_init_exif_info(cmr_handle handle, void **param) {
    cmr_int ret = SENSOR_FAIL;
    EXIF_SPEC_PIC_TAKING_COND_T *sexif = NULL;

    SENSOR_IC_CHECK_PTR(param);
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    ret = sensor_ic_get_init_exif_info(sns_drv_cxt, &sexif);
    SENSOR_IC_CHECK_PTR(sexif);
    *param = sexif;

    sexif->ExposureTime.numerator = sns_drv_cxt->exp_time;
    sexif->ExposureTime.denominator = 1000000;

    /*you can add other init data,here*/
    return ret;
}

static cmr_int ov13850_drv_stream_on(cmr_handle handle, cmr_uint param) {
    SENSOR_LOGI("StreamOn E:");

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    hw_sensor_write_reg(
        sns_drv_cxt->hw_handle, 0x3009,
        (hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x3009) | 0x60));
    SENSOR_LOGI(" driver_capability :0x%x ",
                hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x3009));

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0100, 0x01);

    return 0;
}

static cmr_int ov13850_drv_stream_off(cmr_handle handle, cmr_uint param) {
    SENSOR_LOGI("StreamOff");
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0100, 0x00);
    usleep(100 * 1000);

    return 0;
}

static int ov13850_drv_get_gain16(cmr_handle handle) {
    // read gain, 16 = 1x
    int gain16;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    gain16 = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x350a) & 0x03;
    gain16 = (gain16 << 8) + hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x350b);

    return gain16;
}

static cmr_int ov13850_drv_set_gain16(cmr_handle handle, int gain16) {
    // write gain, 16 = 1x
    int temp;
    gain16 = gain16 & 0x3ff;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    temp = gain16 & 0xff;
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x350b, temp);

    temp = gain16 >> 8;
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x350a, temp);

    return 0;
}

static void _calculate_hdr_exposure(cmr_handle handle, int capture_gain16,
                                    int capture_VTS, int capture_shutter) {
    // write capture gain
    ov13850_drv_set_gain16(handle, capture_gain16);

    // write capture shutter
    /*if (capture_shutter > (capture_VTS - 4)) {
            capture_VTS = capture_shutter + 4;
            OV5640_set_VTS(capture_VTS);
    }*/
    ov13850_drv_set_shutter(handle, capture_shutter);
}

static cmr_int ov13850_drv_set_ev(cmr_handle handle, cmr_uint param) {
    cmr_int rtn = SENSOR_SUCCESS;
    SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR)param;

    cmr_u16 value = 0x00;

    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(ext_ptr);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    cmr_u32 ev = ext_ptr->param;
    cmr_u32 gain = sns_drv_cxt->sensor_gain.gain;
    cmr_u32 vts = *((cmr_u32 *)sns_drv_cxt->privata_data.data);
    cmr_u16 exp_line = sns_drv_cxt->exp_line;

    SENSOR_LOGI("param: 0x%x", ext_ptr->param);

    switch (ev) {
    case SENSOR_HDR_EV_LEVE_0:
        _calculate_hdr_exposure(handle, gain, vts, exp_line / 4);
        break;
    case SENSOR_HDR_EV_LEVE_1:
        _calculate_hdr_exposure(handle, gain, vts, exp_line);
        break;
    case SENSOR_HDR_EV_LEVE_2:
        _calculate_hdr_exposure(handle, gain, vts, exp_line * 4);
        break;
    default:
        break;
    }
    usleep(50 * 1000);
    return rtn;
}

/*==============================================================================
 * Description:
 * calculate fps for every sensor mode according to frame_line and line_time
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int ov13850_drv_init_fps_info(cmr_handle handle) {
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

static cmr_int ov13850_drv_get_static_info(cmr_handle handle, cmr_u32 *param) {
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
        ov13850_drv_init_fps_info(handle);
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

static cmr_int ov13850_drv_get_fps_info(cmr_handle handle, cmr_u32 *param) {
    cmr_int rtn = SENSOR_SUCCESS;
    SENSOR_MODE_FPS_T *fps_info = (SENSOR_MODE_FPS_T *)param;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(fps_info);
    SENSOR_IC_CHECK_PTR(param);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    struct sensor_fps_info *fps_data = sns_drv_cxt->fps_info;

    // make sure have inited fps of every sensor mode.
    if (!fps_data->is_init) {
        ov13850_drv_init_fps_info(handle);
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

static cmr_int ov13850_drv_ext_func(cmr_handle handle, cmr_uint ctl_param) {
    cmr_int rtn = SENSOR_SUCCESS;
    SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR)ctl_param;
    SENSOR_IC_CHECK_PTR(ext_ptr);

    SENSOR_LOGI("0x%x", ext_ptr->cmd);

    switch (ext_ptr->cmd) {
    case SENSOR_EXT_FUNC_INIT:
        break;
    case SENSOR_EXT_FOCUS_START:
        break;
    case SENSOR_EXT_EXPOSURE_START:
        break;
    case SENSOR_EXT_EV:
        rtn = ov13850_drv_set_ev(handle, ctl_param);
        break;
    default:
        break;
    }
    return rtn;
}

static cmr_int ov13850_drv_access_val(cmr_handle handle, cmr_uint param) {
    cmr_int rtn = SENSOR_SUCCESS;
    SENSOR_VAL_T *param_ptr = (SENSOR_VAL_T *)param;
    cmr_u16 tmp;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("E");

    SENSOR_LOGI("param_ptr->type=%x", param_ptr->type);
    switch (param_ptr->type) {
    case SENSOR_VAL_TYPE_SHUTTER:
        *((cmr_u32 *)param_ptr->pval) = ov13850_drv_get_shutter(handle);
        break;
    case SENSOR_VAL_TYPE_READ_OTP_GAIN:
        rtn = ov13850_drv_read_otp_gain(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_GET_STATIC_INFO:
        rtn = ov13850_drv_get_static_info(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_GET_FPS_INFO:
        rtn = ov13850_drv_get_fps_info(handle, param_ptr->pval);
        break;
    default:
        break;
    }

    SENSOR_LOGI("X");

    return rtn;
}

static cmr_int
ov13850_drv_handle_create(struct sensor_ic_drv_init_para *init_param,
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
    ov13850_drv_init_fps_info(sns_drv_cxt);
    /*add private here*/
    return ret;
}

static cmr_int ov13850_drv_get_private_data(cmr_handle handle, cmr_uint cmd,
                                            void **param) {
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param);

    ret = sensor_ic_get_private_data(handle, cmd, param);
    return ret;
}

static cmr_int ov13850_drv_handle_delete(cmr_handle handle, void *param) {
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_IC_CHECK_HANDLE(handle);

    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    /*if has private data,you must release it here*/

    ret = sensor_ic_drv_delete(handle, param);
    return ret;
}
static struct sensor_ic_ops s_ov13850_ops_tab = {
    .create_handle = ov13850_drv_handle_create,
    .delete_handle = ov13850_drv_handle_delete,
    .get_data = ov13850_drv_get_private_data,
    .power = ov13850_drv_power_on,
    .identify = ov13850_drv_identify,
    .write_exp = ov13850_drv_write_exposure,
    .write_gain_value = ov13850_drv_write_gain,

    /*the following interface called by oem layer*/
    .ext_ops = {
            [SENSOR_IOCTL_BEFORE_SNAPSHOT].ops = ov13850_drv_before_snapshot,
            [SENSOR_IOCTL_STREAM_ON].ops = ov13850_drv_stream_on,
            [SENSOR_IOCTL_STREAM_OFF].ops = ov13850_drv_stream_off,
            [SENSOR_IOCTL_EXT_FUNC].ops = ov13850_drv_ext_func,
            [SENSOR_IOCTL_VIDEO_MODE].ops = ov13850_drv_set_video_mode,
            [SENSOR_IOCTL_ACCESS_VAL].ops = ov13850_drv_access_val,
    }};
