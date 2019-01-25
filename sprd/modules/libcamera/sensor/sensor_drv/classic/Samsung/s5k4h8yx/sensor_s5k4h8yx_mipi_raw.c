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
#include"sensor_s5k4h8yx_mipi_raw.h"
#define LOG_TAG "s5k4h8yx_mipi_raw"

#define MIPI_RAW_INFO  g_s5k4h8yx_mipi_raw_info
#define RES_TRIM_TAB   s_s5k4h8yx_resolution_trim_tab
#define FPS_INFO       s_s5k4h8yx_mode_fps_info
#define STATIC_INFO    s_s5k4h8yx_static_info
#define VIDEO_INFO     s_s5k4h8yx_video_info
#define MODULE_INFO    s_s5k4h8yx_module_info_tab
#define RES_TAB_RAW    s_s5k4h8yx_resolution_tab_raw

static cmr_int s5k4h8yx_drv_set_video_mode(cmr_handle handle,
                                                       cmr_uint param) {
    SENSOR_REG_T_PTR sensor_reg_ptr;
    cmr_u16 i = 0;
    cmr_u32 mode = 0;
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    if (param >= SENSOR_VIDEO_MODE_MAX)
        return 0;

    if(sns_drv_cxt->ops_cb.get_mode)
        ret = sns_drv_cxt->ops_cb.get_mode(sns_drv_cxt->caller_handle, &mode);
        if (SENSOR_SUCCESS != ret) {
            SENSOR_LOGI("get mode fail.");
            return ret;
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

    SENSOR_LOGI("%ld", param);
    return 0;
}

#if 0
static cmr_uint s5k4h8yx_drv_GetResolutionTrimTab(cmr_handle handle,
                                                    cmr_uint param) {
    SENSOR_LOGI("0x%lx", (cmr_uint)ss5k4h8yx_drv_Resolution_Trim_Tab);
    return (cmr_uint)ss5k4h8yx_drv_Resolution_Trim_Tab;
}
#endif
static cmr_int s5k4h8yx_drv_power_on(cmr_handle handle,
                                           cmr_uint power_on) {
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    struct module_cfg_info *module_info = sns_drv_cxt->module_info;

    SENSOR_AVDD_VAL_E dvdd_val = module_info->dvdd_val;
    SENSOR_AVDD_VAL_E avdd_val = module_info->avdd_val;
    SENSOR_AVDD_VAL_E iovdd_val = module_info->iovdd_val;
    BOOLEAN power_down = MIPI_RAW_INFO.power_down_level;
    BOOLEAN reset_level = MIPI_RAW_INFO.reset_pulse_level;

    cmr_u8 pid_value = 0x00;

    SENSOR_LOGI("In");
    if (SENSOR_TRUE == power_on) {
        hw_sensor_set_reset_level(sns_drv_cxt->hw_handle, reset_level);
        hw_sensor_power_down(sns_drv_cxt->hw_handle, power_down);
        hw_sensor_set_voltage(sns_drv_cxt->hw_handle, dvdd_val, avdd_val, iovdd_val);
        usleep(1);
        hw_sensor_power_down(sns_drv_cxt->hw_handle, !power_down);
        hw_sensor_set_reset_level(sns_drv_cxt->hw_handle, !reset_level);
        usleep(1000);
        hw_sensor_set_mclk(sns_drv_cxt->hw_handle, SENSOR_DEFALUT_MCLK);
        usleep(1000);
    } else {
        hw_sensor_set_mclk(sns_drv_cxt->hw_handle, SENSOR_DISABLE_MCLK);
        hw_sensor_set_reset_level(sns_drv_cxt->hw_handle, reset_level);
        hw_sensor_power_down(sns_drv_cxt->hw_handle, power_down);
        hw_sensor_set_voltage(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED,
                                 SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
    }

    SENSOR_LOGI("Power_On(1:on, 0:off): %ld,reset_level %d, dvdd_val %d",
                                        power_on, reset_level, dvdd_val);
    return SENSOR_SUCCESS;
}

#if 0
static cmr_u32 s5k4h8yx_drv_GetMaxFrameLine(cmr_handle handle,
                                          cmr_u32 index) {
    cmr_u32 max_line = 0x00;
    SENSOR_TRIM_T_PTR trim_ptr = ss5k4h8yx_drv_Resolution_Trim_Tab;

    max_line = trim_ptr[index].frame_line;
    s_current_default_line_time = trim_ptr[index].line_time;

    return max_line;
}
#endif
static cmr_int s5k4h8yx_drv_init_mode_fps_info(cmr_handle handle) {
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
    SENSOR_LOGI("X");
    return rtn;
}

static cmr_int s5k4h8yx_drv_get_raw_info(cmr_handle handle) {
    cmr_int rtn = SENSOR_SUCCESS;
#if 0
    void *handlelib;
    handlelib = dlopen("libcamsensortuning.so", RTLD_NOW);
    if (handlelib == NULL) {
        char const *err_str = dlerror();
        ALOGE("dlopen error%s", err_str ? err_str : "unknown");
    }

    /* Get the address of the struct hal_module_info. */
    const char *sym = S5K4H8YX_RAW_PARAM_AS_STR;

    s_s5k4h8yx_mipi_raw_info_ptr = (oem_module_t *)dlsym(handlelib, sym);
    if (s_s5k4h8yx_mipi_raw_info_ptr == NULL) {
        SENSOR_LOGI("load: couldn't find symbol %s", sym);
    } else {
        SENSOR_LOGI("link symbol success");
    }
#endif
    return rtn;
}

static cmr_int s5k4h8yx_drv_identify(cmr_handle handle,
                                              cmr_uint param) {
#define S5K4H8YX_PID_VALUE 0x4088
#define S5K4H8YX_PID_ADDR 0x0000
#define S5K4H8YX_VER_VALUE 0xc001
#define S5K4H8YX_VER_ADDR 0x0002
    cmr_u16 pid_value = 0x00;
    cmr_u16 ver_value = 0x00;
    cmr_int ret_value = SENSOR_FAIL;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("E:\n");

    pid_value = hw_sensor_read_reg(sns_drv_cxt->hw_handle, S5K4H8YX_PID_ADDR);
    ver_value = hw_sensor_read_reg(sns_drv_cxt->hw_handle, S5K4H8YX_VER_ADDR);
    SENSOR_LOGI("Identify: PID = %x, VER = %x", pid_value,
                ver_value);

    if (S5K4H8YX_PID_VALUE == pid_value) {
        if (S5K4H8YX_VER_VALUE == ver_value) {
            ret_value = SENSOR_SUCCESS;
            SENSOR_LOGI("this is %s sensor !",SENSOR_NAME);
           /* ret_value = s5k4h8yx_drv_get_raw_info(handle);
            if (SENSOR_SUCCESS != ret_value) {
                SENSOR_LOGI("the module is unknow error !");
            }*/
            s5k4h8yx_drv_init_mode_fps_info(handle);
        } else {
            SENSOR_LOGI("Identify this is hm%x%x sensor !",
                        pid_value, ver_value);
            return ret_value;
        }
    }

    return ret_value;
}

static cmr_int s5k4h8yx_drv_ex_write_exposure(cmr_handle handle,
                                                           cmr_uint param) {
    cmr_int ret_value = SENSOR_SUCCESS;
    cmr_u16 exposure_line = 0x00;
    cmr_u16 dummy_line = 0x00;
    cmr_u16 size_index = 0x00;
    struct sensor_ex_exposure *ex = (struct sensor_ex_exposure *)param;
    SENSOR_IC_CHECK_PTR(ex);

    exposure_line = ex->exposure;
    dummy_line = ex->dummy;
    size_index = ex->size_index;

    ret_value = s5k4h8yx_drv_write_exp_dummy(handle, exposure_line, dummy_line,
                                          size_index);

    return ret_value;
}

static cmr_int s5k4h8yx_drv_write_exposure(cmr_handle handle,
                                                      cmr_uint param) {
    cmr_int ret_value = SENSOR_SUCCESS;
    cmr_u32 expsure_line = 0x00;
    cmr_u32 dummy_line = 0x00;
    cmr_u32 size_index = 0x00;

    expsure_line = param & 0xffff;
    dummy_line = (0 >> 0x10) & 0x0fff;
    size_index = (param >> 0x1c) & 0x0f;

    ret_value =
        s5k4h8yx_drv_write_exp_dummy(handle, expsure_line, dummy_line, size_index);

    return ret_value;
}

// cmr_u32 s_af_step=0x00;
static cmr_int s5k4h8yx_drv_write_exp_dummy(cmr_handle handle,
                                                    cmr_u16 expsure_line,
                                                    cmr_u16 dummy_line,
                                                    cmr_u16 size_index) {
    cmr_int ret_value = SENSOR_SUCCESS;
    cmr_u16 frame_len_cur = 0x00;
    cmr_u16 frame_len = 0x00;
    cmr_u16 max_frame_len = 0x00;
    cmr_u32 linetime = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("write_exposure line:%d, dummy:%d, size_index:%d",
                            expsure_line, dummy_line, size_index);
    max_frame_len = sns_drv_cxt->trim_tab_info[size_index].frame_line;
    if (expsure_line < 2) {
        expsure_line = 2;
    }
    sns_drv_cxt->frame_length_def = max_frame_len;

    frame_len = expsure_line + dummy_line;
    frame_len = frame_len > (expsure_line + 8) ? frame_len : (expsure_line + 8);
    frame_len = (frame_len > max_frame_len) ? frame_len : max_frame_len;
    if (0x00 != (0x01 & frame_len)) {
        frame_len += 0x01;
    }

    frame_len_cur = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0340) & 0xffff;

    if (frame_len_cur != frame_len) {
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0340, frame_len & 0xffff);
    }
    sns_drv_cxt->frame_length = frame_len;

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0202, expsure_line & 0xffff);
    sns_drv_cxt->exp_line = expsure_line;
    linetime = sns_drv_cxt->trim_tab_info[size_index].line_time;

    if(sns_drv_cxt->ops_cb.set_exif_info) {
        sns_drv_cxt->ops_cb.set_exif_info(sns_drv_cxt->caller_handle,
                          SENSOR_EXIF_CTRL_EXPOSURETIME, sns_drv_cxt->exp_line);
    } else {
        sns_drv_cxt->exif_info.exposure_line = sns_drv_cxt->exp_line;
    }
    sns_drv_cxt->exp_time = sns_drv_cxt->exp_line * linetime / 1000;
    sns_drv_cxt->sensor_ev_info.preview_shutter = sns_drv_cxt->exp_line;

    return ret_value;
}

static cmr_int s5k4h8yx_drv_write_gain(cmr_handle handle,
                                                cmr_uint param) {
    cmr_int ret_value = SENSOR_SUCCESS;
    float real_gain = 0;
    float a_gain = 0;
    float d_gain = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    real_gain = 1.0f * param * 0x0020 / 0x80;

    SENSOR_LOGI("real_gain:0x%x, param: %ul", (cmr_u32)real_gain, param);
    sns_drv_cxt->sensor_ev_info.preview_gain = param;

    if ((cmr_u32)real_gain <= 16 * 32) {
        a_gain = real_gain;
        d_gain = 256;
    } else {
        a_gain = 16 * 32;
        d_gain = 256.0 * real_gain / a_gain;
        SENSOR_LOGI("real_gain:0x%x, a_gain: 0x%x, d_gain: 0x%x",
                    (cmr_u32)real_gain, (cmr_u32)a_gain, (cmr_u32)d_gain);
        if ((cmr_u32)d_gain > 256 * 256)
            d_gain = 256 * 256;
    }

    ret_value = hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x204, (cmr_u32)a_gain); // 0x100);//a_gain);
    ret_value = hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x20e, (cmr_u32)d_gain);
    ret_value = hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x210, (cmr_u32)d_gain);
    ret_value = hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x212, (cmr_u32)d_gain);
    ret_value = hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x214, (cmr_u32)d_gain);

    return ret_value;
}

static cmr_int s5k4h8yx_drv_before_snapshot(cmr_handle handle,
                                                       cmr_uint param) {
    cmr_u8 ret_l, ret_m, ret_h;
    cmr_u32 capture_exposure, preview_maxline;
    cmr_u32 capture_maxline, preview_exposure;
    cmr_u32 capture_mode = param & 0xffff;
    cmr_u32 preview_mode = (param >> 0x10) & 0xffff;
    cmr_u32 gain = 0;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    cmr_u32 prv_linetime = sns_drv_cxt->trim_tab_info[preview_mode].line_time;
    cmr_u32 cap_linetime = sns_drv_cxt->trim_tab_info[capture_mode].line_time;

    SENSOR_LOGI("mode: %ul", param);

    if (preview_mode == capture_mode) {
        SENSOR_LOGI("prv mode equal to capmode");
        goto CFG_INFO;
    }

    preview_exposure = s5k4h8yx_drv_get_shutter(handle);
    preview_maxline = s5k4h8yx_drv_get_vts(handle);

    if(sns_drv_cxt->ops_cb.set_mode)
        sns_drv_cxt->ops_cb.set_mode(sns_drv_cxt->caller_handle, capture_mode);
    if(sns_drv_cxt->ops_cb.set_mode_wait_done)
        sns_drv_cxt->ops_cb.set_mode_wait_done(sns_drv_cxt->caller_handle);

    gain = sns_drv_cxt->sensor_ev_info.preview_gain;

    /*if (prv_linetime == cap_linetime) {
        SENSOR_LOGI("prvline equal to capline");
        goto CFG_INFO;
    }*/

    capture_maxline = s5k4h8yx_drv_get_vts(handle);

    capture_exposure = preview_exposure * prv_linetime / cap_linetime;

    SENSOR_LOGI("BeforeSnapshot: capture_exposure 0x%x, capture_maxline "
                "0x%x,preview_maxline 0x%x, preview_exposure 0x%x",
                capture_exposure, capture_maxline, preview_maxline,
                preview_exposure);

    if (0 == capture_exposure) {
        capture_exposure = 1;
    }

    if (capture_exposure > (capture_maxline - 4)) {
        capture_maxline = capture_exposure + 4;
        s5k4h8yx_drv_set_vts(handle, capture_maxline);
    }

    s5k4h8yx_drv_set_shutter(handle, capture_exposure);
    s5k4h8yx_drv_write_gain(handle, gain);

CFG_INFO:
    // s_capture_shutter = s5k4h8yx_drv_get_shutter(handle);
    *((cmr_u32*)sns_drv_cxt->privata_data.data) = s5k4h8yx_drv_get_vts(handle);
    s5k4h8yx_drv_read_gain(handle, &gain);

    if(sns_drv_cxt->ops_cb.set_exif_info) {
        sns_drv_cxt->ops_cb.set_exif_info(sns_drv_cxt->caller_handle,
                          SENSOR_EXIF_CTRL_EXPOSURETIME, sns_drv_cxt->exp_line);
        sns_drv_cxt->ops_cb.set_exif_info(sns_drv_cxt->caller_handle,
                          SENSOR_EXIF_CTRL_APERTUREVALUE, 20);
        sns_drv_cxt->ops_cb.set_exif_info(sns_drv_cxt->caller_handle,
                          SENSOR_EXIF_CTRL_MAXAPERTUREVALUE, 20);
        sns_drv_cxt->ops_cb.set_exif_info(sns_drv_cxt->caller_handle,
                          SENSOR_EXIF_CTRL_FNUMBER, 20);
    } else {
        sns_drv_cxt->exif_info.exposure_line = sns_drv_cxt->exp_line;
    }
    sns_drv_cxt->exp_time = sns_drv_cxt->exp_time * cap_linetime / 1000;

#if 0
    Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, s_capture_shutter);
    Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_APERTUREVALUE, 20);
    Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_MAXAPERTUREVALUE, 20);
    Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_FNUMBER, 20);
    s_exposure_time = s_capture_shutter * cap_linetime / 1000;
#endif
    return SENSOR_SUCCESS;
}

static cmr_int s5k4h8yx_drv_after_snapshot(cmr_handle handle,
                                                       cmr_uint param) {
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    SENSOR_LOGI("after_snapshot mode:%ld", param);
    if(sns_drv_cxt->ops_cb.set_mode)
        ret = sns_drv_cxt->ops_cb.set_mode(sns_drv_cxt->caller_handle,(cmr_u32)param);

    return ret;
}

static cmr_int s5k4h8yx_drv_get_exif_info(cmr_handle handle,
                                                      void **param) {
    cmr_int ret = SENSOR_FAIL;
    EXIF_SPEC_PIC_TAKING_COND_T *sexif = NULL;
    *param = NULL;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    ret = sensor_ic_get_init_exif_info(sns_drv_cxt, &sexif);
    SENSOR_IC_CHECK_PTR(sexif);

    sexif->ExposureTime.numerator = sns_drv_cxt->exp_time;
    sexif->ExposureTime.denominator = 1000000;
    *param = (void*)sexif;
    return 0;
}

static cmr_int s5k4h8yx_drv_stream_on(cmr_handle handle,
                                                cmr_uint param) {
    UNUSED(param);
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    SENSOR_LOGI("E:");

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0100, 0x0103);
#if 1
    char value1[PROPERTY_VALUE_MAX];
    property_get("debug.camera.test.mode", value1, "0");
    if (!strcmp(value1, "1")) {
        SENSOR_LOGI("enable test mode");
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0600, 0x0002);
    }
#endif

    return 0;
}

static cmr_int s5k4h8yx_drv_stream_off(cmr_handle handle,
                                               cmr_uint param) {
    UNUSED(param);
    uint16_t value;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("E");

    value = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0100);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0100, 0x0003);
    if (value == 0x0103)
        usleep(50 * 1000);

    return 0;
}

static cmr_u16 s5k4h8yx_drv_get_shutter(cmr_handle handle) {
    /* read shutter, in number of line period */
    cmr_u16 shutter = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

#if 1 // MP tool //!??
    shutter = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0202);

    return shutter;
#else
    return s_set_exposure;
#endif
}

static cmr_int s5k4h8yx_drv_set_shutter(cmr_handle handle,
                                                 cmr_u16 shutter) {
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    // hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x104, 0x01);
    // write shutter, in number of line period
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0202, shutter);
    // hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x104, 0x00);

    return 0;
}

static cmr_u32 s5k4h8yx_drv_get_gain16(cmr_handle handle) {
    // read gain, 16 = 1x
    cmr_u32 gain16 = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    gain16 =
       (256 * 16) / (256 - hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0157));

    return gain16;
}

static cmr_int s5k4h8yx_drv_set_gain16(cmr_handle handle, cmr_u32 gain16) {
    // write gain, 16 = 1x
    cmr_u16 temp = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    gain16 = gain16 & 0x3ff;

    if (gain16 < 16)
        gain16 = 16;
    if (gain16 > 170)
        gain16 = 170;

    temp = (256 * (gain16 - 16)) / gain16;
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0157, temp & 0xff);

    return 0;
}

// cmr_u32 s_af_step=0x00;
static cmr_int s5k4h8yx_drv_write_exposure_ev(cmr_handle handle,
                                                 cmr_uint param) {
    cmr_int ret_value = SENSOR_SUCCESS;
    cmr_u16 expsure_line = 0x00;
    cmr_u16 dummy_line = 0x00;
    cmr_u16 frame_len_cur = 0x00;
    cmr_u16 frame_len = 0x00;
    cmr_u16 size_index = 0x00;
    cmr_u16 max_frame_len = 0x00;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    expsure_line = param;
//    s_set_exposure = expsure_line;
    SENSOR_LOGI("expsure_line:%d, dummy:%d, size_index:%d",
                expsure_line, dummy_line, size_index);
    max_frame_len = 2480;
    if (expsure_line < 2) {
        expsure_line = 2;
    }

    frame_len = 2480;
    frame_len = frame_len > (expsure_line + 8) ? frame_len : (expsure_line + 8);
    if (0x00 != (0x01 & frame_len)) {
        frame_len += 0x01;
    }

    frame_len_cur = (hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0340)) & 0xffff;
    // frame_len_cur |= (hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0340)<<0x08)&0xff00;

    // ret_value = hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x104, 0x01);
    if (frame_len_cur != frame_len) {
        ret_value = hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0340, frame_len & 0xffff);
        // ret_value = hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0340, (frame_len >> 0x08) & 0xff);
    }

    ret_value = hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x202, expsure_line & 0xffff);
    // ret_value = hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x202, (expsure_line >> 0x08) & 0xff);
    // ret_value = hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x104, 0x00);
    return ret_value;
}

/*calculate hdr exposure*/
static void s5k4h8yx_drv_cal_hdr_exposure(cmr_handle handle,
                 int capture_gain16, int capture_VTS, int capture_shutter) {
    // write capture gain
    //s5k4h8yx_drv_set_gain16(capture_gain16);

    // write capture shutter
    /*if (capture_shutter > (capture_VTS - 4)) {
            capture_VTS = capture_shutter + 4;
            OV5640_set_VTS(capture_VTS);
    }*/
    //s5k4h8yx_drv_set_shutter(capture_shutter);
    s5k4h8yx_drv_write_exposure_ev(handle, capture_shutter);
}

static cmr_int s5k4h8yx_drv_set_ev(cmr_handle handle, cmr_uint param) {
    cmr_int rtn = SENSOR_SUCCESS;
    cmr_u16 value = 0x00;

    SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR)param;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(ext_ptr);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    cmr_u32 gain = sns_drv_cxt->sensor_gain.gain;
    cmr_u32 cap_shutter = sns_drv_cxt->exp_line;
    cmr_u32 cap_vts = *((cmr_u32*)sns_drv_cxt->privata_data.data);
    cmr_u32 ev = ext_ptr->param;

    cmr_u16 expsure_line = 0x00;
    cmr_u16 line_time = 0x00;
    cmr_u32 exp_delay_time = 0;

    line_time = 134;

    SENSOR_LOGI("ev: 0x%x s_capture_shutter %d", ev, cap_shutter);

    switch (ev) {
    case SENSOR_HDR_EV_LEVE_0:
        s5k4h8yx_drv_cal_hdr_exposure(handle, gain, cap_vts, cap_shutter / 4);
        break;
    case SENSOR_HDR_EV_LEVE_1:
        s5k4h8yx_drv_cal_hdr_exposure(handle, gain, cap_vts, cap_shutter);
        break;
    case SENSOR_HDR_EV_LEVE_2:
        s5k4h8yx_drv_cal_hdr_exposure(handle, gain, cap_vts, cap_shutter * 2);
        break;
    default:
        break;
    }

    expsure_line = cap_shutter;

    exp_delay_time = (expsure_line * line_time) / 10;
    exp_delay_time += 10000;

    if (50000 > exp_delay_time) {
        exp_delay_time = 50000;
    }

    usleep(exp_delay_time);

    return rtn;
}

static cmr_int s5k4h8yx_drv_ext_func(cmr_handle handle,
                                       cmr_uint ctl_param) {
    cmr_int rtn = SENSOR_SUCCESS;
    SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR)ctl_param;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(ext_ptr);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("0x%x", ext_ptr->cmd);

    switch (ext_ptr->cmd) {
    case SENSOR_EXT_FUNC_INIT:
        break;
    case SENSOR_EXT_FOCUS_START:
        break;
    case SENSOR_EXT_EXPOSURE_START:
        break;
    case SENSOR_EXT_EV:
        rtn = s5k4h8yx_drv_set_ev(handle, ctl_param);
        break;
    default:
        break;
    }
    return rtn;
}

static cmr_u16 s5k4h8yx_drv_get_vts(cmr_handle handle) {
    cmr_u16 VTS = 0;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    VTS = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0160);

    return VTS;
}

static cmr_int s5k4h8yx_drv_set_vts(cmr_handle handle, cmr_u16 VTS) {
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0160, (VTS & 0xffff));

    return 0;
}

static cmr_int s5k4h8yx_drv_read_gain(cmr_handle handle, cmr_u32 *param) {
    cmr_int rtn = SENSOR_SUCCESS;
    cmr_u32 gain = 0;
    cmr_u16 a_gain = 0;
    cmr_u16 d_gain = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    a_gain = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0204);
    d_gain = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0210);
    gain = a_gain * d_gain;

    sns_drv_cxt->sensor_gain.gain = gain;
    *param = gain;

    SENSOR_LOGI("gain: 0x%x", gain);

    return rtn;
}

static cmr_int s5k4h8yx_drv_write_otp_gain(cmr_handle handle,
                                         cmr_u32 *param) {
    cmr_int ret_value = SENSOR_SUCCESS;
    cmr_u16 value = 0x00;
    float a_gain = 0;
    float d_gain = 0;
    float real_gain = 0.0f;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("write_gain:0x%x\n", *param);

    if ((*param / 4) > 0x200) {
        a_gain = 16 * 32;
        d_gain = (*param / 4.0) * 256 / a_gain;
        if (d_gain > 256 * 256)
            d_gain = 256 * 256;
    } else {
        a_gain = *param / 4;
        d_gain = 256;
    }
    ret_value = hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x204, (cmr_u32)a_gain);
    ret_value = hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x20e, (cmr_u32)d_gain);
    ret_value = hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x210, (cmr_u32)d_gain);
    ret_value = hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x212, (cmr_u32)d_gain);
    ret_value = hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x214, (cmr_u32)d_gain);

    return ret_value;
}

static cmr_int s5k4h8yx_drv_read_otp_gain(cmr_handle handle,
                                        cmr_u32 *param) {
    cmr_int rtn = SENSOR_SUCCESS;
    cmr_u32 gain = 0;
    cmr_u16 a_gain = 0;
    cmr_u16 d_gain = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    a_gain = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0204);
    d_gain = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0210);
    *param = a_gain * d_gain;
    SENSOR_LOGI("gain: %d", *param);

    return rtn;
}

static cmr_int s5k4h8yx_drv_get_fps_info(cmr_handle handle,
                                       cmr_u32 *param) {
    cmr_int rtn = SENSOR_SUCCESS;
    SENSOR_MODE_FPS_T *fps_info = (SENSOR_MODE_FPS_T *)param;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(fps_info);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    struct sensor_fps_info *fps_data = sns_drv_cxt->fps_info;

    // make sure have inited fps of every sensor mode.
    if (!fps_data->is_init) {
        s5k4h8yx_drv_init_mode_fps_info(handle);
    }
    cmr_u32 sensor_mode = fps_info->mode;
    fps_info->max_fps = fps_data->sensor_mode_fps[sensor_mode].max_fps;
    fps_info->min_fps = fps_data->sensor_mode_fps[sensor_mode].min_fps;
    fps_info->is_high_fps = fps_data->sensor_mode_fps[sensor_mode].is_high_fps;
    fps_info->high_fps_skip_num = fps_data->sensor_mode_fps[sensor_mode].high_fps_skip_num;

    SENSOR_LOGI("mode %d, max_fps: %d", fps_info->mode, fps_info->max_fps);
    SENSOR_LOGI("min_fps: %d", fps_info->min_fps);
    SENSOR_LOGI("is_high_fps: %d", fps_info->is_high_fps);
    SENSOR_LOGI("high_fps_skip_num: %d", fps_info->high_fps_skip_num);

    return rtn;
}

static cmr_int s5k4h8yx_drv_get_static_info(cmr_handle handle,
                                          cmr_u32 *param) {
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

    ex_info->f_num = static_info->f_num;
    ex_info->focal_length = static_info->focal_length;
    ex_info->max_fps = static_info->max_fps;
    ex_info->max_adgain = static_info->max_adgain;
    ex_info->ois_supported = static_info->ois_supported;
    ex_info->pdaf_supported = static_info->pdaf_supported;
    ex_info->exp_valid_frame_num = static_info->exp_valid_frame_num;
    ex_info->clamp_level = static_info->clamp_level;
    ex_info->adgain_valid_frame_num =
        static_info->adgain_valid_frame_num;
    ex_info->preview_skip_num = module_info->preview_skip_num;
    ex_info->capture_skip_num = module_info->capture_skip_num;
    ex_info->name = (cmr_s8 *)MIPI_RAW_INFO.name;
    ex_info->sensor_version_info = (cmr_s8 *)MIPI_RAW_INFO.sensor_version_info;

    memcpy(&ex_info->fov_info, &static_info->fov_info, sizeof(static_info->fov_info));

    sensor_ic_print_static_info(SENSOR_NAME, ex_info);

    return rtn;
}


static cmr_int s5k4h8yx_drv_access_val(cmr_handle handle,
                                                 cmr_uint param) {
    cmr_int rtn = SENSOR_SUCCESS;
    SENSOR_VAL_T *param_ptr = (SENSOR_VAL_T *)param;
    cmr_u16 tmp;

    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param_ptr);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("type =%x", param_ptr->type);
    switch (param_ptr->type) {
    case SENSOR_VAL_TYPE_SHUTTER:
        *((cmr_u32 *)param_ptr->pval) = s5k4h8yx_drv_get_shutter(handle);
        break;
    case SENSOR_VAL_TYPE_WRITE_OTP_GAIN:
        rtn = s5k4h8yx_drv_write_otp_gain(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_READ_OTP_GAIN:
        rtn = s5k4h8yx_drv_read_otp_gain(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_GET_GOLDEN_DATA:
//        rtn = s5k4h8yx_drv_get_golden_data(handle, (cmr_uint)param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_GET_GOLDEN_LSC_DATA:
        //*(cmr_u32*)param_ptr->pval = s5k4h8_lsc_golden_data;
        break;
    case SENSOR_VAL_TYPE_GET_STATIC_INFO:
        rtn = s5k4h8yx_drv_get_static_info(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_GET_FPS_INFO:
        rtn = s5k4h8yx_drv_get_fps_info(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_SET_SENSOR_CLOSE_FLAG:
        sns_drv_cxt->is_sensor_close = 1;
        rtn = SENSOR_SUCCESS;
        break;
    default:
        break;
    }

    SENSOR_LOGI("X:");

    return rtn;
}

static cmr_int s5k4h8yx_drv_handle_create(
          struct sensor_ic_drv_init_para *init_param, cmr_handle* sns_ic_drv_handle) {
    cmr_int ret = SENSOR_SUCCESS;
    struct sensor_ic_drv_cxt * sns_drv_cxt = NULL;
    void *pri_data = NULL;

    ret = sensor_ic_drv_create(init_param,sns_ic_drv_handle);
    sns_drv_cxt = *sns_ic_drv_handle;

    sensor_ic_set_match_module_info(sns_drv_cxt, ARRAY_SIZE(MODULE_INFO), MODULE_INFO);
    sensor_ic_set_match_resolution_info(sns_drv_cxt, ARRAY_SIZE(RES_TAB_RAW), RES_TAB_RAW);
    sensor_ic_set_match_trim_info(sns_drv_cxt, ARRAY_SIZE(RES_TRIM_TAB), RES_TRIM_TAB);
    sensor_ic_set_match_static_info(sns_drv_cxt, ARRAY_SIZE(STATIC_INFO), STATIC_INFO);
    sensor_ic_set_match_fps_info(sns_drv_cxt, ARRAY_SIZE(FPS_INFO), FPS_INFO);

    s5k4h8yx_drv_get_exif_info(*sns_ic_drv_handle,&sns_drv_cxt->exif_ptr);

    /*add private here*/
    return ret;
}

static cmr_int s5k4h8yx_drv_handle_delete(cmr_handle handle, cmr_int *param) {
    cmr_int ret = SENSOR_SUCCESS;
    /*if has private data,you must release it here*/
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    ret = sensor_ic_drv_delete(handle,param);
    return ret;
}

static cmr_int s5k4h8yx_drv_get_private_data(cmr_handle handle,
                                                     cmr_uint cmd, void**param){
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param);

    ret = sensor_ic_get_private_data(handle,cmd, param);
    return ret;
}


static struct sensor_ic_ops s5k4h8yx_ops_tab = {
    .create_handle = s5k4h8yx_drv_handle_create,
    .delete_handle = s5k4h8yx_drv_handle_delete,
    .get_data = s5k4h8yx_drv_get_private_data,
    .power = s5k4h8yx_drv_power_on,
    .identify = s5k4h8yx_drv_identify,
    .ex_write_exp = s5k4h8yx_drv_ex_write_exposure,
    .write_gain_value = s5k4h8yx_drv_write_gain,
    .ext_ops = {
        [SENSOR_IOCTL_EXT_FUNC].ops = s5k4h8yx_drv_ext_func,
        [SENSOR_IOCTL_VIDEO_MODE].ops = s5k4h8yx_drv_set_video_mode,
        [SENSOR_IOCTL_BEFORE_SNAPSHOT].ops = s5k4h8yx_drv_before_snapshot,
        [SENSOR_IOCTL_STREAM_ON].ops = s5k4h8yx_drv_stream_on,
        [SENSOR_IOCTL_STREAM_OFF].ops = s5k4h8yx_drv_stream_off,
        [SENSOR_IOCTL_ACCESS_VAL].ops = s5k4h8yx_drv_access_val,
    }
};

