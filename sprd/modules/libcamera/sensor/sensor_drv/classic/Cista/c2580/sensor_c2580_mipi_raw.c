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

#define LOG_TAG "c2580_mipi_raw"
#include "sensor_c2580_mipi_raw.h"

#define FPS_INFO s_c2580_mode_fps_info
#define STATIC_INFO s_c2580_static_info
#define VIDEO_INFO s_c2580_video_info
#define MODULE_INFO s_c2580_module_info_tab
#define RES_TAB_RAW s_c2580_resolution_tab_raw
#define RES_TRIM_TAB s_c2580_resolution_trim_tab
#define MIPI_RAW_INFO g_c2580_mipi_raw_info

static cmr_int c2580_drv_set_video_mode(cmr_handle handle, cmr_u32 param) {
    SENSOR_REG_T_PTR sensor_reg_ptr;
    cmr_u16 i = 0x00;
    cmr_u32 mode = 0;
    cmr_int ret = SENSOR_SUCCESS;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    if (param >= SENSOR_VIDEO_MODE_MAX)
        return 0;

    if (sns_drv_cxt->ops_cb.get_mode)
        ret = sns_drv_cxt->ops_cb.get_mode(sns_drv_cxt->caller_handle, &mode);
    if (SENSOR_SUCCESS != ret) {
        SENSOR_LOGI("get mode fail.");
        return ret;
    }

    if (PNULL == VIDEO_INFO[mode].setting_ptr) {
        SENSOR_LOGI("fail. mode=%d", mode);
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

    SENSOR_LOGI("0x%02x", param);
    return ret;
}

/*==============================================================================
 * Description:
 * calculate fps for every sensor mode according to frame_line and line_time
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int c2580_drv_init_fps_info(cmr_handle handle) {
    cmr_int rtn = SENSOR_SUCCESS;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    struct sensor_fps_info *fps_info = sns_drv_cxt->fps_info;
    struct sensor_trim_tag *trim_info = sns_drv_cxt->trim_tab_info;
    struct sensor_static_info *static_info = sns_drv_cxt->static_info;

    SENSOR_LOGV("E");
    if (!fps_info->is_init) {
        cmr_u32 i, modn, tempfps = 0;
        SENSOR_LOGI("start init");
        for (i = 0; i < SENSOR_MODE_MAX; i++) {
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
    SENSOR_LOGV("X");
    return rtn;
}

static cmr_int c2580_drv_get_static_info(cmr_handle handle, cmr_u32 *param) {
    cmr_int rtn = SENSOR_SUCCESS;
    struct sensor_ex_info *ex_info = (struct sensor_ex_info *)param;
    cmr_u32 up = 0;
    cmr_u32 down = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(ex_info);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    struct sensor_fps_info *fps_info = sns_drv_cxt->fps_info;
    struct sensor_static_info *static_info = sns_drv_cxt->static_info;
    struct module_cfg_info *module_info = sns_drv_cxt->module_info;
    if (!(fps_info && static_info && module_info)) {
        SENSOR_LOGE("error:null pointer checked.return");
        return SENSOR_FAIL;
    }

    // make sure we have get max fps of all settings.
    if (!fps_info->is_init) {
        c2580_drv_init_fps_info(handle);
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
    sensor_ic_print_static_info((cmr_s8 *)SENSOR_NAME, ex_info);

    return rtn;
}

static cmr_int c2580_drv_get_fps_info(cmr_handle handle, cmr_u32 *param) {
    cmr_int rtn = SENSOR_SUCCESS;
    SENSOR_MODE_FPS_T *fps_info = (SENSOR_MODE_FPS_T *)param;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(fps_info);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    struct sensor_fps_info *fps_data = sns_drv_cxt->fps_info;

    // make sure have inited fps of every sensor mode.
    if (!fps_data->is_init) {
        c2580_drv_init_fps_info(handle);
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

static cmr_int c2580_drv_set_init(cmr_handle handle) {
    cmr_u16 i;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_REG_T *sensor_reg_ptr = (SENSOR_REG_T *)c2580_1600_1200_setting;
    usleep(100 * 1000);

    for (i = 0; i < sizeof(c2580_1600_1200_setting) /
                        sizeof(c2580_1600_1200_setting[0]);
         i++) {
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, sensor_reg_ptr[i].reg_addr,
                            sensor_reg_ptr[i].reg_value);
    }

    SENSOR_LOGV("X");

    return 0;
}

static cmr_int c2580_drv_stream_on(cmr_handle handle, cmr_uint param) {
    int temp;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    // c2580_drv_set_init(handle);
    temp = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0100);

    temp = temp | 0x01;

    SENSOR_LOGI("StreamOn: 0x%x", temp);

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0100, temp);
    // usleep(100*1000);

    return 0;
}
static cmr_int c2580_drv_get_brightness(cmr_handle handle, cmr_u32 *param) {
    cmr_u16 i = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    // hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe,0x01);
    *param = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x3f09);
    SENSOR_LOGI("SENSOR: get_brightness: lumma = 0x%x\n", *param);
    // hw_sensor_write_reg(sns_drv_cxt->hw_handle, handle,0xfe,0x00);
    return 0;
}

static cmr_int c2580_drv_access_val(cmr_handle handle, cmr_uint param) {
    cmr_u32 rtn = SENSOR_SUCCESS;
    SENSOR_VAL_T *param_ptr = (SENSOR_VAL_T *)param;
    cmr_u16 tmp;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_HANDLE(param_ptr);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("param_ptr->type=%x", param_ptr->type);
    switch (param_ptr->type) {
    case SENSOR_VAL_TYPE_GET_BV:
        rtn = c2580_drv_get_brightness(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_GET_STATIC_INFO:
        rtn = c2580_drv_get_static_info(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_GET_FPS_INFO:
        rtn = c2580_drv_get_fps_info(handle, param_ptr->pval);
        break;
    default:
        break;
    }

    SENSOR_LOGV("X");

    return rtn;
}

static cmr_int c2580_drv_stream_off(cmr_handle handle, cmr_uint param) {
    int temp;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("Stop");

    temp = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0100);

    temp = temp & 0xfe;

    SENSOR_LOGI("SENSOR_c2580: StreamOff: 0x%x", temp);

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0100, temp);
    usleep(150 * 1000);

    return 0;
}

static int c2580_drv_get_gain16(cmr_handle handle) {
    int gain16;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    gain16 = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0205);

    return gain16;
}

static int c2580_drv_set_gain16(cmr_handle handle, int gain16) {
    int temp;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("write_gain:0x%x", gain16);

    temp = gain16 & 0xff;
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0205, temp);

    return 0;
}
static int c2580_drv_get_shutter(cmr_handle handle) {
    // read shutter, in number of line period
    int shutter;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    shutter = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0202);
    shutter =
        (shutter << 8) + hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0203);

    return shutter;
}

static int c2580_drv_set_shutter(cmr_handle handle, int shutter) {
    int temp;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("shutter:%d", shutter);

    temp = shutter >> 8;
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0202, temp);

    temp = shutter & 0xff;
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0203, temp);

    return 0;
}

static int c2580_drv_get_vts(cmr_handle handle) {
    int VTS;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    VTS = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0340);
    VTS = (VTS << 8) + hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0341);

    return VTS;
}

static int c2580_drv_set_vts(cmr_handle handle, int VTS) {
    // write VTS to registers
    int temp;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("write_vts:%d", VTS);

    temp = VTS >> 8;
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0340, temp);

    temp = VTS & 0xff;
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0341, temp);

    return 0;
}

static int c2580_drv_grouphold_on(cmr_handle handle) {
    SENSOR_LOGI("E");
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    /*hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3400,0x00);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3404,0x05);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xe000,0x02);//es H  0xE002
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xe001,0x02);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xe003,0x02);//es L   0xE005
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xe004,0x03);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xe006,0x02);//gain    0xE008
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xe007,0x05);

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xe009,0x03);//vts H  0xE00B
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xe00A,0x40);*/
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xe00B,
                        (c2580_drv_get_vts(handle) >> 8) & 0xFF);
    /*hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xe00C,0x03);//vts L  0xE00E
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xe00D,0x41);*/
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xe00E,
                        c2580_drv_get_vts(handle) & 0xFF);
    return 0;
}
static int c2580_drv_grouphold_off(cmr_handle handle) {
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x340F, 0x20); // fast write
    SENSOR_LOGI("vts:0x%x, shutter:0x%x, gain: 0x%x", c2580_drv_get_vts(handle),
                c2580_drv_get_shutter(handle), c2580_drv_get_gain16(handle));

    return 0;
}

static void _calculate_hdr_exposure(cmr_handle handle, int capture_gain16,
                                    int capture_VTS, int capture_shutter) {
    // c2580_drv_grouphold_on();

    // write capture gain
    c2580_drv_set_gain16(handle, capture_gain16);

    // write capture shutter
    if (capture_shutter > (capture_VTS - c2580_ES_OFFSET)) {
        capture_VTS = capture_shutter + c2580_ES_OFFSET;
        c2580_drv_set_vts(handle, capture_VTS);
    }
    c2580_drv_set_shutter(handle, capture_shutter);

    // c2580_drv_grouphold_off();
}

static cmr_int c2580_drv_power_on(cmr_handle handle, cmr_uint power_on) {
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
        hw_sensor_set_iovdd_val(sns_drv_cxt->hw_handle, iovdd_val);
        usleep(2000);
        hw_sensor_set_avdd_val(sns_drv_cxt->hw_handle, avdd_val);
        hw_sensor_power_down(sns_drv_cxt->hw_handle, !power_down);
        usleep(2000);
        // step 3 reset pin high
        hw_sensor_set_reset_level(sns_drv_cxt->hw_handle, !reset_level);
        usleep(2 * 1000);
        // step 4 xvclk
        hw_sensor_set_mclk(sns_drv_cxt->hw_handle, SENSOR_DEFALUT_MCLK);
        usleep(4 * 1000);
    } else {
        usleep(4 * 1000);
        // step 1 reset and PWDN
        hw_sensor_set_reset_level(sns_drv_cxt->hw_handle, reset_level);
        usleep(2000);
        hw_sensor_power_down(sns_drv_cxt->hw_handle, power_down);
        usleep(2000);
        // step 4 xvclk
        hw_sensor_set_mclk(sns_drv_cxt->hw_handle, SENSOR_DISABLE_MCLK);
        usleep(5000);
        // step 5 AVDD IOVDD
        hw_sensor_set_avdd_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED);
        usleep(2000);
        hw_sensor_set_iovdd_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED);
    }
    SENSOR_LOGI("Power_On(1:on, 0:off): %ld", power_on);
    return SENSOR_SUCCESS;
}

static cmr_int c2580_drv_identify(cmr_handle handle, cmr_uint param) {
#define c2580_PID_VALUE 0x02
#define c2580_PID_ADDR 0x0000
#define c2580_VER_VALUE 0x01
#define c2580_VER_ADDR 0x0001

    cmr_u8 pid_value = 0x00;
    cmr_u8 ver_value = 0x00;
    cmr_u32 ret_value = SENSOR_FAIL;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("E:");

    pid_value = hw_sensor_read_reg(sns_drv_cxt->hw_handle, c2580_PID_ADDR);
    if (c2580_PID_VALUE == pid_value) {
        ver_value = hw_sensor_read_reg(sns_drv_cxt->hw_handle, c2580_VER_ADDR);
        SENSOR_LOGI("PID = %x, VER = %x", pid_value, ver_value);
        if (c2580_VER_VALUE == ver_value) {
            SENSOR_LOGI("this is c2580 sensor !");
            ret_value = SENSOR_SUCCESS;
        } else {
            SENSOR_LOGI("this is c%x%x sensor !", pid_value, ver_value);
        }
    } else {
        SENSOR_LOGE("identify fail,pid_value=%d", pid_value);
    }

    return ret_value;
}

static cmr_int c2580_drv_write_exposure(cmr_handle handle, cmr_uint param) {
    cmr_int ret_value = SENSOR_SUCCESS;
    cmr_u16 expsure_line = 0x00;
    cmr_u16 dummy_line = 0x00;
    cmr_u16 size_index = 0x00;
    cmr_u16 frame_len = 0x00;
    cmr_u16 frame_len_cur = 0x00;
    cmr_u16 max_frame_len = 0x00;

    expsure_line = param & 0xffff;
    dummy_line = (param >> 0x10) & 0x0fff;
    size_index = (param >> 0x1c) & 0x0f;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("exposure line:%d, dummy:%d, size_index:%d", expsure_line,
                dummy_line, size_index);

    // group hold on
    // c2580_drv_grouphold_on();

    max_frame_len = sns_drv_cxt->trim_tab_info[size_index].frame_line;

    if (0x00 != max_frame_len) {
        frame_len = ((expsure_line + c2580_ES_OFFSET) > max_frame_len)
                        ? (expsure_line + c2580_ES_OFFSET)
                        : max_frame_len;
        frame_len = (frame_len + 1) >> 1 << 1;
        frame_len_cur = c2580_drv_get_vts(handle);
        SENSOR_LOGI("frame_len_cur:%d, frame_len:%d,", frame_len_cur,
                    frame_len);

        if (frame_len_cur != frame_len) {
            ret_value = c2580_drv_set_vts(handle, frame_len);
            if (SENSOR_SUCCESS != ret_value) {
                SENSOR_LOGE("set vts failed");
                return SENSOR_FAIL;
            }
        }
    }

    ret_value = c2580_drv_set_shutter(handle, expsure_line);

    return ret_value;
}

static cmr_int c2580_drv_write_gain(cmr_handle handle, cmr_uint param) {
    cmr_int ret_value = SENSOR_SUCCESS;
    cmr_u32 isp_gain = 0x00;
    cmr_u32 sensor_gain = 0x00;
    SENSOR_LOGI("write_gain:0x%lx", param);

    isp_gain = ((param & 0xf) + 16) * (((param >> 4) & 0x01) + 1) *
               (((param >> 5) & 0x01) + 1);
    isp_gain = isp_gain * (((param >> 6) & 0x01) + 1) *
               (((param >> 7) & 0x01) + 1) * (((param >> 8) & 0x01) + 1);

    // sensor_gain=(((isp_gain-16)/16)<<4)+isp_gain%16;
    sensor_gain = (((isp_gain - 16) / 16) << 4) + (isp_gain - 16) % 16;

    ret_value = c2580_drv_set_gain16(handle, sensor_gain);

    // group hold off
    // c2580_drv_grouphold_off();
    return ret_value;
}

static cmr_int c2580_drv_before_snapshot(cmr_handle handle, cmr_uint param) {
    cmr_u32 capture_exposure, preview_maxline;
    cmr_u32 capture_maxline, preview_exposure;
    cmr_u32 capture_mode = param & 0xffff;
    cmr_u32 preview_mode = (param >> 0x10) & 0xffff;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    PRIVATE_DATA *pri_data = (PRIVATE_DATA *)sns_drv_cxt->privata_data.buffer;

    cmr_u32 prv_linetime = sns_drv_cxt->trim_tab_info[preview_mode].line_time;
    cmr_u32 cap_linetime = sns_drv_cxt->trim_tab_info[capture_mode].line_time;

    SENSOR_LOGI("mode: 0x%lx", param);

    if (preview_mode == capture_mode) {
        SENSOR_LOGI("prv mode equal to capmode");
        goto CFG_INFO;
    }

    preview_exposure = c2580_drv_get_shutter(handle);
    preview_maxline = c2580_drv_get_vts(handle);

    if (sns_drv_cxt->ops_cb.set_mode)
        sns_drv_cxt->ops_cb.set_mode(sns_drv_cxt->caller_handle, capture_mode);
    if (sns_drv_cxt->ops_cb.set_mode_wait_done)
        sns_drv_cxt->ops_cb.set_mode_wait_done(sns_drv_cxt->caller_handle);

    capture_maxline = c2580_drv_get_vts(handle);

    capture_exposure = preview_exposure * prv_linetime * 2 / cap_linetime;

    if (0 == capture_exposure) {
        capture_exposure = 1;
    }

    SENSOR_LOGI("preview_exposure:%d, capture_exposure:%d, capture_maxline: %d",
                preview_exposure, capture_exposure, capture_maxline);
    // c2580_drv_grouphold_on();
    if (capture_exposure > (capture_maxline - c2580_ES_OFFSET)) {
        capture_maxline = capture_exposure + c2580_ES_OFFSET;
        capture_maxline = (capture_maxline + 1) >> 1 << 1;
        c2580_drv_set_vts(handle, capture_maxline);
    }

    c2580_drv_set_shutter(handle, capture_exposure);
// c2580_drv_grouphold_off();
CFG_INFO:
    pri_data->cap_shutter = c2580_drv_get_shutter(handle);
    pri_data->cap_vts = c2580_drv_get_vts(handle);
    pri_data->gain = c2580_drv_get_gain16(handle);

    if (sns_drv_cxt->ops_cb.set_exif_info) {
        sns_drv_cxt->ops_cb.set_exif_info(sns_drv_cxt->caller_handle,
                                          SENSOR_EXIF_CTRL_EXPOSURETIME,
                                          pri_data->cap_shutter);
    } else {
        sns_drv_cxt->exif_info.exposure_time = pri_data->cap_shutter;
    }

    return SENSOR_SUCCESS;
}

static cmr_int c2580_drv_after_snapshot(cmr_handle handle, cmr_uint param) {
    SENSOR_LOGI("after_snapshot mode:%ld", param);
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    if (sns_drv_cxt->ops_cb.set_mode)
        sns_drv_cxt->ops_cb.set_mode(sns_drv_cxt->caller_handle, param);

    return SENSOR_SUCCESS;
}

static cmr_int c2580_drv_set_ev(cmr_handle handle, cmr_uint param) {
    cmr_int rtn = SENSOR_SUCCESS;
    SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR)param;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(ext_ptr);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    PRIVATE_DATA *pri_data = (PRIVATE_DATA *)sns_drv_cxt->privata_data.buffer;

    cmr_int gain = pri_data->gain;
    cmr_int vts = pri_data->cap_vts;
    cmr_int cap_shutter = pri_data->cap_shutter;

    cmr_u32 ev = ext_ptr->param;

    SENSOR_LOGI("param: 0x%x", ext_ptr->param);

    switch (ev) {
    case SENSOR_HDR_EV_LEVE_0:
        _calculate_hdr_exposure(handle, gain, vts, cap_shutter / 2);
        break;
    case SENSOR_HDR_EV_LEVE_1:
        _calculate_hdr_exposure(handle, gain, vts, cap_shutter);
        break;
    case SENSOR_HDR_EV_LEVE_2:
        _calculate_hdr_exposure(handle, gain, vts, cap_shutter * 2);
        break;
    default:
        break;
    }
    return rtn;
}

static cmr_int c2580_drv_save_load_exposure(cmr_handle handle, cmr_uint param) {
    cmr_int rtn = SENSOR_SUCCESS;
    SENSOR_EXT_FUN_PARAM_T_PTR sl_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR)param;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(sl_ptr);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    PRIVATE_DATA *pri_data = (PRIVATE_DATA *)sns_drv_cxt->privata_data.buffer;
    SENSOR_IC_CHECK_PTR(pri_data);

    cmr_u32 sl_param = sl_ptr->param;
    if (sl_param) {
        /*load exposure params to sensor*/
        SENSOR_LOGI("load shutter 0x%lx gain 0x%lx", pri_data->shutter,
                    pri_data->gain_bak);
        // c2580_drv_grouphold_on();
        c2580_drv_set_gain16(handle, pri_data->gain_bak);
        c2580_drv_set_shutter(handle, pri_data->shutter);
        // c2580_drv_grouphold_off();
    } else {
        /*ave exposure params from sensor*/
        pri_data->shutter = c2580_drv_get_shutter(handle);
        pri_data->gain_bak = c2580_drv_get_gain16(handle);
        SENSOR_LOGI("save shutter 0x%lx gain 0x%lx", pri_data->shutter,
                    pri_data->gain_bak);
    }
    return rtn;
}

static cmr_int c2580_drv_ext_func(cmr_handle handle, cmr_uint ctl_param) {
    cmr_int rtn = SENSOR_SUCCESS;
    SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR)ctl_param;

    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(ext_ptr);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    SENSOR_LOGI("0x%x", ext_ptr->cmd);

    switch (ext_ptr->cmd) {
    case SENSOR_EXT_FUNC_INIT:
        break;
    case SENSOR_EXT_FOCUS_START:
        break;
    case SENSOR_EXT_EXPOSURE_START:
        break;
    case SENSOR_EXT_EV:
        rtn = c2580_drv_set_ev(handle, ctl_param);
        break;

    case SENSOR_EXT_EXPOSURE_SL:
        rtn = c2580_drv_save_load_exposure(handle, ctl_param);
        break;

    default:
        break;
    }
    return rtn;
}

static cmr_int
c2580_drv_handle_create(struct sensor_ic_drv_init_para *init_param,
                        cmr_handle *sns_ic_drv_handle) {
    cmr_int ret = SENSOR_SUCCESS;
    struct sensor_ic_drv_cxt *sns_drv_cxt = NULL;
    cmr_u32 pri_data_size = sizeof(PRIVATE_DATA);
    void *pri_data = NULL;

    ret = sensor_ic_drv_create(init_param, sns_ic_drv_handle);
    sns_drv_cxt = *sns_ic_drv_handle;

    if (pri_data_size > 4) {
        pri_data = (PRIVATE_DATA *)malloc(pri_data_size);
        if (pri_data)
            sns_drv_cxt->privata_data.buffer = (cmr_u8 *)pri_data;
        else
            SENSOR_LOGE("size:%d", pri_data_size);
    }

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
    c2580_drv_init_fps_info(sns_drv_cxt);

    /*add private here*/
    return ret;
}

static cmr_int c2580_drv_handle_delete(cmr_handle handle, void *param) {
    cmr_int ret = SENSOR_SUCCESS;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    ret = sensor_ic_drv_delete(handle, param);
    return ret;
}

static cmr_int c2580_drv_get_private_data(cmr_handle handle, cmr_uint cmd,
                                          void **param) {
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param);

    ret = sensor_ic_get_private_data(handle, cmd, param);
    return ret;
}

// static static SENSOR_IOCTL_FUNC_TAB_T s_c2580_ioctl_func_tab = {
static struct sensor_ic_ops s_c2580_ops_tab = {
    .create_handle = c2580_drv_handle_create,
    .delete_handle = c2580_drv_handle_delete,
    .get_data = c2580_drv_get_private_data,

    .power = c2580_drv_power_on,
    .identify = c2580_drv_identify,
    .ex_write_exp = c2580_drv_write_exposure,
    .write_gain_value = c2580_drv_write_gain,

    .ext_ops =
        {
                [SENSOR_IOCTL_EXT_FUNC].ops = c2580_drv_ext_func,
                [SENSOR_IOCTL_BEFORE_SNAPSHOT].ops = c2580_drv_before_snapshot,
                [SENSOR_IOCTL_AFTER_SNAPSHOT].ops = c2580_drv_after_snapshot,
                [SENSOR_IOCTL_STREAM_ON].ops = c2580_drv_stream_on,
                [SENSOR_IOCTL_STREAM_OFF].ops = c2580_drv_stream_off,
                [SENSOR_IOCTL_ACCESS_VAL].ops = c2580_drv_access_val,
        }

};
