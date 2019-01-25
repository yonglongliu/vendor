/*
* Copyright (C) 2012 The Android Open Source Project8u7
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
 * V4.0
 */

#define LOG_TAG "c2390_mipi_raw"
#include "sensor_c2390_mipi_raw.h"

#define FPS_INFO s_c2390_mode_fps_info
#define STATIC_INFO s_c2390_static_info
#define VIDEO_INFO s_c2390_video_info
#define MODULE_INFO s_c2390_module_info_tab
#define RES_TAB_RAW s_c2390_resolution_tab_raw
#define RES_TRIM_TAB s_c2390_resolution_trim_tab
#define MIPI_RAW_INFO g_c2390_mipi_raw_info

/*==============================================================================
 * Description:
 * set video mode
 *
 *============================================================================*/
static cmr_int c2390_drv_set_video_mode(cmr_handle handle, cmr_uint param) {
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

    return ret;
}

/*==============================================================================
 * Description:
 * calculate fps for every sensor mode according to frame_line and line_time
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int c2390_drv_init_fps_info(cmr_handle handle) {
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
    SENSOR_LOGV("X");
    return rtn;
}

static cmr_int c2390_drv_get_static_info(cmr_handle handle, cmr_u32 *param) {
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
        c2390_drv_init_fps_info(handle);
    }
    ex_info = (struct sensor_ex_info *)param;
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

static cmr_int c2390_drv_get_fps_info(cmr_handle handle, cmr_u32 *param) {
    cmr_int rtn = SENSOR_SUCCESS;
    SENSOR_MODE_FPS_T *fps_info = (SENSOR_MODE_FPS_T *)param;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(fps_info);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    struct sensor_fps_info *fps_data = sns_drv_cxt->fps_info;

    // make sure have inited fps of every sensor mode.
    if (!fps_data->is_init) {
        c2390_drv_init_fps_info(handle);
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

/*==============================================================================
 * Description:
 * write group-hold on to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void c2390_group_hold_on(void) { SENSOR_LOGV("E"); }

/*==============================================================================
 * Description:
 * write group-hold off to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void c2390_group_hold_off(void) { SENSOR_LOGV("E"); }

/*==============================================================================
 * Description:
 * read gain from sensor registers
 * please modify this function acording your spec
 *============================================================================*/

static cmr_u16 c2390_drv_read_gain(cmr_handle handle) {
    cmr_u16 gain_h = 0;
    cmr_u16 gain_l = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    gain_h = 0;
    gain_l = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0205) & 0xff;

    return ((gain_h << 8) | gain_l);
}

/*==============================================================================
 * Description:
 * write gain to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void c2390_drv_write_gain(cmr_handle handle, cmr_u32 gain) {
    cmr_u32 sensor_gain = 0x00;
    cmr_u32 reg0216 = 0x00;
    cmr_u32 reg0217 = 0x00;
    cmr_u16 sleep = 0;
    SENSOR_IC_CHECK_HANDLE_VOID(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("gain = %x\n", gain);
    if (gain > SENSOR_MAX_GAIN)
        gain = SENSOR_MAX_GAIN;

    sleep = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0100) & 0xff;

    if (sleep == 0) {
        if (gain < SENSOR_BASE_GAIN) {
            hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0205, 0x00);
            hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x328e, 0x20);
            hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0216, 0x01);
            hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0217, 0x00);
        } else if (gain <= 0x100) // <16x
        {
            sensor_gain = (((gain - 16) / 16) << 4) + gain % 16;
            hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0205, sensor_gain);
            hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x328e, 0x20);
            hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0216, 0x01);
            hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0217, 0x00);
        } else if (gain <= 0x200) // <32x
        {
            sensor_gain = (((gain / 2 - 16) / 16) << 4) + (gain / 2) % 16;
            hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0205, sensor_gain);
            hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x328e,
                                0x10); // night mode x2
            hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0216, 0x01);
            hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0217, 0x00);
        } else // >= 32x
        {
            reg0216 = (gain / SENSOR_BASE_GAIN) >> 5;
            reg0217 = (gain % (SENSOR_BASE_GAIN * 32)) / 2;
            hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0205, 0xf0);
            hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x328e,
                                0x10); // night mode x2
            hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0216, reg0216);
            hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0217, reg0217);
        }
    } else {
        if (gain < SENSOR_BASE_GAIN) {
            hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xe062, 0x00);
            hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xe065, 0x20);
            hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xe068, 0x01);
            hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xe06b, 0x00);
            hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x340f, 0x12);
        } else if (gain <= 0x100) // <16x
        {
            sensor_gain = (((gain - 16) / 16) << 4) + gain % 16;
            hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xe062, sensor_gain);
            hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xe065, 0x20);
            hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xe068, 0x01);
            hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xe06b, 0x00);
            hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x340f, 0x12);
        } else if (gain <= 0x200) // <32x
        {
            sensor_gain = (((gain / 2 - 16) / 16) << 4) + (gain / 2) % 16;
            hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xe062, sensor_gain);
            hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xe065,
                                0x10); // night mode x2
            hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xe068, 0x01);
            hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xe06b, 0x00);
            hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x340f, 0x12);
        } else // >= 32x
        {
            reg0216 = (gain / SENSOR_BASE_GAIN) >> 5;
            reg0217 = (gain % (SENSOR_BASE_GAIN * 32)) / 2;
            hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xe062, 0xf0);
            hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xe065,
                                0x10); // night mode x2
            hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xe068, reg0216);
            hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xe06b, reg0217);
            hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x340f, 0x12);
        }
    }
    SENSOR_LOGI("sensor_gain = %d\n", sensor_gain);
}

/*==============================================================================
 * Description:
 * read frame length from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static cmr_u16 c2390_drv_read_frame_length(cmr_handle handle) {
    cmr_u16 frame_len_h = 0;
    cmr_u16 frame_len_l = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    frame_len_h = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0340) & 0xff;
    frame_len_l = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0341) & 0xff;

    return ((frame_len_h << 8) | frame_len_l);
}

/*==============================================================================
 * Description:
 * write frame length to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void c2390_drv_write_frame_length(cmr_handle handle, cmr_u32 frame_len) {
    SENSOR_IC_CHECK_HANDLE_VOID(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0340,
                        (frame_len >> 8) & 0xff);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0341, frame_len & 0xff);
}

/*==============================================================================
 * Description:
 * read shutter from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static cmr_u32 c2390_drv_read_shutter(cmr_handle handle) {
    cmr_u16 shutter_h = 0;
    cmr_u16 shutter_l = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    shutter_h = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0202) & 0xff;
    shutter_l = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0203) & 0xff;

    return (shutter_h << 8) | shutter_l;
}

/*==============================================================================
 * Description:
 * write shutter to sensor registers
 * please pay attention to the frame length
 * please modify this function acording your spec
 *============================================================================*/
static void c2390_drv_write_shutter(cmr_handle handle, cmr_u32 shutter) {
    SENSOR_IC_CHECK_HANDLE_VOID(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0202, (shutter >> 8) & 0xff);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0203, shutter & 0xff);
    SENSOR_LOGI("shutter %d", shutter);
}

/*==============================================================================
 * Description:
 * write exposure to sensor registers and get current shutter
 * please pay attention to the frame length
 * please don't change this function if it's necessary
 *============================================================================*/
static cmr_u16 c2390_drv_update_exposure(cmr_handle handle, cmr_u32 shutter,
                                         cmr_u32 dummy_line) {
    cmr_u32 dest_fr_len = 0;
    cmr_u32 cur_fr_len = 0;
    cmr_u32 fr_len = 0;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    fr_len = sns_drv_cxt->frame_length_def;
    // c2390_group_hold_on();

    if (1 == SUPPORT_AUTO_FRAME_LENGTH)
        goto write_sensor_shutter;

    dummy_line = dummy_line > FRAME_OFFSET ? dummy_line : FRAME_OFFSET;
    dest_fr_len =
        ((shutter + dummy_line) > fr_len) ? (shutter + dummy_line) : fr_len;

    cur_fr_len = c2390_drv_read_frame_length(handle);

    if (shutter < SENSOR_MIN_SHUTTER)
        shutter = SENSOR_MIN_SHUTTER;

    if (dest_fr_len != cur_fr_len)
        c2390_drv_write_frame_length(handle, dest_fr_len);
write_sensor_shutter:
    /* write shutter to sensor registers */
    c2390_drv_write_shutter(handle, shutter);
    return shutter;
}

/*==============================================================================
 * Description:
 * sensor power on
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int c2390_drv_power_on(cmr_handle handle, cmr_uint power_on) {
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    struct module_cfg_info *module_info = sns_drv_cxt->module_info;

    SENSOR_AVDD_VAL_E dvdd_val = module_info->dvdd_val;
    SENSOR_AVDD_VAL_E avdd_val = module_info->avdd_val;
    SENSOR_AVDD_VAL_E iovdd_val = module_info->iovdd_val;
    BOOLEAN power_down = MIPI_RAW_INFO.power_down_level;
    BOOLEAN reset_level = MIPI_RAW_INFO.reset_pulse_level;

    if (SENSOR_TRUE == power_on) {
        hw_sensor_power_down(sns_drv_cxt->hw_handle, !power_down);
        hw_sensor_set_reset_level(sns_drv_cxt->hw_handle, reset_level);
        usleep(1 * 1000);
        hw_sensor_set_iovdd_val(sns_drv_cxt->hw_handle, iovdd_val);
        usleep(1 * 1000);
        hw_sensor_set_avdd_val(sns_drv_cxt->hw_handle, avdd_val);
        hw_sensor_set_dvdd_val(sns_drv_cxt->hw_handle, dvdd_val);
        hw_sensor_set_monitor_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_3300MV);
        usleep(1 * 1000);
        // hw_sensor_set_mclk(sns_drv_cxt->hw_handle, SENSOR_DEFALUT_MCLK);
        usleep(1 * 1000);
        hw_sensor_power_down(sns_drv_cxt->hw_handle, power_down);
        hw_sensor_set_reset_level(sns_drv_cxt->hw_handle, !reset_level);
        usleep(1 * 1000);
        hw_sensor_set_mclk(sns_drv_cxt->hw_handle, SENSOR_DEFALUT_MCLK);
        usleep(1 * 1000);
    } else {
#ifdef DYNA_BLC_ENABLE
        BLC_FLAG = 0;
#endif
        hw_sensor_set_mclk(sns_drv_cxt->hw_handle, SENSOR_DISABLE_MCLK);
        usleep(1 * 1000);
        hw_sensor_set_reset_level(sns_drv_cxt->hw_handle, reset_level);
        hw_sensor_power_down(sns_drv_cxt->hw_handle, !power_down);
        usleep(1 * 1000);
        hw_sensor_set_avdd_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED);
        hw_sensor_set_dvdd_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED);
        usleep(1 * 1000);
        hw_sensor_set_iovdd_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED);
    }
    SENSOR_LOGI("(1:on, 0:off): %ld", power_on);
    return SENSOR_SUCCESS;
}

/*==============================================================================
 * Description:
 * identify sensor id
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int c2390_drv_identify(cmr_handle handle, cmr_uint param) {
    cmr_u8 pid_value = 0x05;
    cmr_u8 ver_value = 0x01;
    cmr_int ret_value = SENSOR_FAIL;

    SENSOR_LOGI("E");
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    pid_value = hw_sensor_read_reg(sns_drv_cxt->hw_handle, c2390_PID_ADDR);

    if (c2390_PID_VALUE == pid_value) {
        ver_value = hw_sensor_read_reg(sns_drv_cxt->hw_handle, c2390_VER_ADDR);
        SENSOR_LOGI("Identify: PID = %x, VER = %x", pid_value, ver_value);
        if (c2390_VER_VALUE == ver_value) {
            ret_value = SENSOR_SUCCESS;
            SENSOR_LOGI("this is c2390 sensor");
        } else {
            SENSOR_LOGI("Identify this is %x%x sensor", pid_value, ver_value);
        }
    } else {
        SENSOR_LOGE("identify fail, pid_value = %x", pid_value);
    }

    return ret_value;
}

/*==============================================================================
 * Description:
 * before snapshot
 * you can change this function if it's necessary
 *============================================================================*/
static cmr_int c2390_drv_before_snapshot(cmr_handle handle, cmr_uint param) {
    cmr_u32 cap_shutter = 0;
    cmr_u32 prv_shutter = 0;
    cmr_u32 gain = 0;
    cmr_u32 cap_gain = 0;
    cmr_u32 capture_mode = param & 0xffff;
    cmr_u32 preview_mode = (param >> 0x10) & 0xffff;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    cmr_u32 prv_linetime = sns_drv_cxt->trim_tab_info[preview_mode].line_time;
    cmr_u32 cap_linetime = sns_drv_cxt->trim_tab_info[capture_mode].line_time;

    sns_drv_cxt->frame_length_def =
        sns_drv_cxt->trim_tab_info[capture_mode].frame_line;

    SENSOR_LOGI("capture_mode = %d", capture_mode);

    if (preview_mode == capture_mode) {
        cap_shutter = sns_drv_cxt->sensor_ev_info.preview_shutter;
        cap_gain = sns_drv_cxt->sensor_ev_info.preview_gain;
        goto snapshot_info;
    }

    prv_shutter = sns_drv_cxt->sensor_ev_info.preview_shutter;
    gain = sns_drv_cxt->sensor_ev_info.preview_gain;

    if (sns_drv_cxt->ops_cb.set_mode)
        sns_drv_cxt->ops_cb.set_mode(sns_drv_cxt->caller_handle, capture_mode);
    if (sns_drv_cxt->ops_cb.set_mode_wait_done)
        sns_drv_cxt->ops_cb.set_mode_wait_done(sns_drv_cxt->caller_handle);

    cap_shutter = prv_shutter * prv_linetime / cap_linetime;

    cap_shutter = c2390_drv_update_exposure(handle, cap_shutter, 0);
    cap_gain = gain;
    c2390_drv_write_gain(handle, cap_gain);
    SENSOR_LOGI("preview_shutter = 0x%x, preview_gain = %f",
                sns_drv_cxt->sensor_ev_info.preview_shutter,
                sns_drv_cxt->sensor_ev_info.preview_gain);

    SENSOR_LOGI("capture_shutter = 0x%x, capture_gain = 0x%x", cap_shutter,
                cap_gain);
snapshot_info:
    sns_drv_cxt->hdr_info.capture_shutter = cap_shutter;
    sns_drv_cxt->hdr_info.capture_gain = cap_gain;
    /* limit HDR capture min fps to 10;
     * MaxFrameTime = 1000000*0.1us;
     */
    sns_drv_cxt->hdr_info.capture_max_shutter = 1000000 / cap_linetime;

    if (sns_drv_cxt->ops_cb.set_exif_info) {
        sns_drv_cxt->ops_cb.set_exif_info(sns_drv_cxt->caller_handle,
                                          SENSOR_EXIF_CTRL_EXPOSURETIME,
                                          cap_shutter);
    } else {
        sns_drv_cxt->exif_info.exposure_time = cap_shutter;
    }

    return SENSOR_SUCCESS;
}

/*==============================================================================
 * Description:
 * get the shutter from isp
 * please don't change this function unless it's necessary
 *============================================================================*/
static cmr_int c2390_drv_write_exposure(cmr_handle handle, cmr_u32 param) {
    cmr_int ret_value = SENSOR_SUCCESS;
    cmr_u16 exposure_line = 0x00;
    cmr_u16 dummy_line = 0x00;
    cmr_u16 mode = 0x00;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    exposure_line = param & 0xffff;
    dummy_line = (param >> 0x10) & 0xfff; /*for cits frame rate test*/
    mode = (param >> 0x1c) & 0x0f;

    SENSOR_LOGI("current mode = %d, exposure_line = %d, dummy_line=%d", mode,
                exposure_line, dummy_line);
    sns_drv_cxt->frame_length_def = sns_drv_cxt->trim_tab_info[mode].frame_line;

    sns_drv_cxt->sensor_ev_info.preview_shutter =
        c2390_drv_update_exposure(handle, exposure_line, dummy_line);

    return ret_value;
}

static cmr_int c2390_drv_write_exposure_ex(cmr_handle handle, cmr_uint param) {
    cmr_int ret_value = SENSOR_SUCCESS;
    cmr_u16 exposure_line = 0x00;
    cmr_u16 dummy_line = 0x00;
    cmr_u16 mode = 0x00;
    struct sensor_ex_exposure *ex = (struct sensor_ex_exposure *)param;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(ex);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    exposure_line = ex->exposure;
    dummy_line = ex->dummy;
    mode = ex->size_index;

    SENSOR_LOGI("current mode = %d, exposure_line = %d, dummy_line=%d", mode,
                exposure_line, dummy_line);
    sns_drv_cxt->frame_length_def = sns_drv_cxt->trim_tab_info[mode].frame_line;

    sns_drv_cxt->sensor_ev_info.preview_shutter =
        c2390_drv_update_exposure(handle, exposure_line, dummy_line);

    return ret_value;
}
/*==============================================================================
 * Description:
 * get the parameter from isp to real gain
 * you mustn't change the funcion !
 *============================================================================*/
static cmr_u32 isp_to_real_gain(cmr_u32 param) {
    cmr_u32 real_gain = 0;

#if defined(CONFIG_CAMERA_ISP_VERSION_V3) ||                                   \
    defined(CONFIG_CAMERA_ISP_VERSION_V4)
    real_gain = param;
#else
    real_gain = ((param & 0xf) + 16) * (((param >> 4) & 0x01) + 1);
    real_gain =
        real_gain * (((param >> 5) & 0x01) + 1) * (((param >> 6) & 0x01) + 1);
    real_gain =
        real_gain * (((param >> 7) & 0x01) + 1) * (((param >> 8) & 0x01) + 1);
    real_gain =
        real_gain * (((param >> 9) & 0x01) + 1) * (((param >> 10) & 0x01) + 1);
    real_gain = real_gain * (((param >> 11) & 0x01) + 1);
#endif

    return real_gain;
}

/*==============================================================================
 * Description:
 * write gain value to sensor
 * you can change this function if it's necessary
 *============================================================================*/
static cmr_int c2390_drv_write_gain_value(cmr_handle handle, cmr_uint param) {
    cmr_int ret_value = SENSOR_SUCCESS;
    cmr_u32 real_gain = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    real_gain = isp_to_real_gain(param);

    real_gain = real_gain * SENSOR_BASE_GAIN / ISP_BASE_GAIN;

    SENSOR_LOGI("real_gain = 0x%x", real_gain);

    sns_drv_cxt->sensor_ev_info.preview_gain = real_gain;
    c2390_drv_write_gain(handle, real_gain);

    return ret_value;
}

/*==============================================================================
 * Description:
 * increase gain or shutter for hdr
 *
 *============================================================================*/
static void c2390_drv_increase_hdr_exposure(cmr_handle handle,
                                            cmr_u8 ev_multiplier) {
    cmr_u32 shutter_multiply = 0;
    cmr_u32 gain = 0;
    SENSOR_IC_CHECK_HANDLE_VOID(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    struct hdr_info_t *hdr_info = &sns_drv_cxt->hdr_info;

    shutter_multiply =
        hdr_info->capture_max_shutter / hdr_info->capture_shutter;

    if (0 == shutter_multiply)
        shutter_multiply = 1;

    if (shutter_multiply >= ev_multiplier) {
        c2390_drv_update_exposure(handle,
                                  hdr_info->capture_shutter * ev_multiplier, 0);
        c2390_drv_write_gain(handle, hdr_info->capture_gain);
    } else {
        gain = hdr_info->capture_gain * ev_multiplier / shutter_multiply;
        c2390_drv_update_exposure(
            handle, hdr_info->capture_shutter * shutter_multiply, 0);
        c2390_drv_write_gain(handle, gain);
    }
}

/*==============================================================================
 * Description:
 * decrease gain or shutter for hdr
 *
 *============================================================================*/
static void c2390_drv_decrease_hdr_exposure(cmr_handle handle,
                                            cmr_u8 ev_divisor) {
    cmr_u16 gain_multiply = 0;
    cmr_u32 shutter = 0;
    SENSOR_IC_CHECK_HANDLE_VOID(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    struct hdr_info_t *hdr_info = &sns_drv_cxt->hdr_info;

    gain_multiply = hdr_info->capture_gain / SENSOR_BASE_GAIN;

    if (gain_multiply >= ev_divisor) {
        c2390_drv_update_exposure(handle, hdr_info->capture_shutter, 0);
        c2390_drv_write_gain(handle, hdr_info->capture_gain / ev_divisor);

    } else {
        shutter = hdr_info->capture_shutter * gain_multiply / ev_divisor;
        c2390_drv_update_exposure(handle, shutter, 0);
        c2390_drv_write_gain(handle, hdr_info->capture_gain / gain_multiply);
    }
}

/*==============================================================================
 * Description:
 * set hdr ev
 * you can change this function if it's necessary
 *============================================================================*/
static cmr_int c2390_drv_set_hdr_ev(cmr_handle handle, cmr_uint param) {
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR)param;

    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(ext_ptr);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    cmr_u32 ev = ext_ptr->param;
    cmr_u8 ev_divisor, ev_multiplier;

    switch (ev) {
    case SENSOR_HDR_EV_LEVE_0:
        ev_divisor = 2;
        c2390_drv_decrease_hdr_exposure(handle, ev_divisor);
        break;
    case SENSOR_HDR_EV_LEVE_1:
        ev_multiplier = 2;
        c2390_drv_increase_hdr_exposure(handle, ev_multiplier);
        break;
    case SENSOR_HDR_EV_LEVE_2:
        ev_multiplier = 1;
        c2390_drv_increase_hdr_exposure(handle, ev_multiplier);
        break;
    default:
        break;
    }
    return ret;
}

/*==============================================================================
 * Description:
 * extra functoin
 * you can add functions reference SENSOR_EXT_FUNC_CMD_E which from
 *sensor_drv_u.h
 *============================================================================*/
static cmr_int c2390_drv_ext_func(cmr_handle handle, cmr_uint param) {
    cmr_int rtn = SENSOR_SUCCESS;
    SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR)param;

    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(ext_ptr);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("ext_ptr->cmd: %d", ext_ptr->cmd);
    switch (ext_ptr->cmd) {
    case SENSOR_EXT_EV:
        rtn = c2390_drv_set_hdr_ev(handle, param);
        break;
    default:
        break;
    }

    return rtn;
}

/*==============================================================================
 * Description:
 * mipi stream on
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int c2390_drv_stream_on(cmr_handle handle, cmr_uint param) {
    SENSOR_LOGI("E");
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    usleep(60 * 1000);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0003, 0x00);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0003, 0x00);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0309, 0x56);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0003, 0x00);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0003, 0x00);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0100, 0x01);
    usleep(30 * 1000);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0003, 0x00);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0003, 0x00);

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0003, 0x00);

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3108, 0x00);

    //	hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3108, 0xcd);

    /*delay*/
    usleep(20 * 1000);
    usleep(200 * 1000);
    // hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x328b, 0x03);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3108, 0xcd);

    return 0;
}

/*==============================================================================
 * Description:
 * mipi stream off
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int c2390_drv_stream_off(cmr_handle handle, cmr_uint param) {
    SENSOR_LOGI("E");
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0003, 0x00);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0003, 0x00);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0003, 0x00);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0100, 0x00);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0003, 0x00);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0003, 0x00);

    usleep(200 * 1000); // 100*1000 capture fail

    return 0;
}

/*==============================================================================
 * Description:
 * cfg otp setting
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int c2390_drv_access_val(cmr_handle handle, cmr_uint param) {
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_VAL_T *param_ptr = (SENSOR_VAL_T *)param;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param_ptr);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("param_ptr->type=%x", param_ptr->type);

    switch (param_ptr->type) {
    case SENSOR_VAL_TYPE_SHUTTER:
        *((cmr_u32 *)param_ptr->pval) = c2390_drv_read_shutter(handle);
        break;
    case SENSOR_VAL_TYPE_READ_OTP_GAIN:
        *((cmr_u32 *)param_ptr->pval) = c2390_drv_read_gain(handle);
        break;
    case SENSOR_VAL_TYPE_GET_STATIC_INFO:
        ret = c2390_drv_get_static_info(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_GET_FPS_INFO:
        ret = c2390_drv_get_fps_info(handle, param_ptr->pval);
        break;
    default:
        break;
    }

    return ret;
}

static cmr_int
c2390_drv_handle_create(struct sensor_ic_drv_init_para *init_param,
                        cmr_handle *sns_ic_drv_handle) {
    cmr_int ret = SENSOR_SUCCESS;
    struct sensor_ic_drv_cxt *sns_drv_cxt = NULL;

    ret = sensor_ic_drv_create(init_param, sns_ic_drv_handle);
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

    /*add private here*/
    return ret;
}

static cmr_int c2390_drv_handle_delete(cmr_handle handle, void *param) {
    cmr_int ret = SENSOR_SUCCESS;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    ret = sensor_ic_drv_delete(handle, param);
    return ret;
}

static cmr_int c2390_drv_get_private_data(cmr_handle handle, cmr_uint cmd,
                                          void **param) {
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param);

    ret = sensor_ic_get_private_data(handle, cmd, param);
    return ret;
}

static struct sensor_ic_ops s_c2390_ops_tab = {
    .create_handle = c2390_drv_handle_create,
    .delete_handle = c2390_drv_handle_delete,
    .get_data = c2390_drv_get_private_data,

    .power = c2390_drv_power_on,
    .identify = c2390_drv_identify,
    .ex_write_exp = c2390_drv_write_exposure_ex,
    .write_gain_value = c2390_drv_write_gain_value,

    .ext_ops =
        {
                [SENSOR_IOCTL_EXT_FUNC].ops = c2390_drv_ext_func,
                [SENSOR_IOCTL_BEFORE_SNAPSHOT].ops = c2390_drv_before_snapshot,
                [SENSOR_IOCTL_AFTER_SNAPSHOT].ops = NULL,
                [SENSOR_IOCTL_STREAM_ON].ops = c2390_drv_stream_on,
                [SENSOR_IOCTL_STREAM_OFF].ops = c2390_drv_stream_off,
                [SENSOR_IOCTL_ACCESS_VAL].ops = c2390_drv_access_val,
        }

};
