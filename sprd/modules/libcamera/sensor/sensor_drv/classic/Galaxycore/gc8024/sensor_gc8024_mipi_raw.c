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
 * V5.0
 */
/*History
*Date                  Modification                                 Reason
*
*/
#define LOG_TAG "sensor_gc8024"
#include "sensor_gc8024_mipi_raw.h"

#define VIDEO_INFO s_gc8024_video_info
#define FPS_INFO s_gc8024_mode_fps_info
#define RES_TRIM_TAB s_gc8024_resolution_trim_tab
#define STATIC_INFO s_gc8024_static_info
#define MIPI_RAW_INFO g_gc8024_mipi_raw_info
#define MODULE_INFO s_gc8024_module_info_tab
#define RES_TAB_RAW s_gc8024_resolution_tab_raw

/*==============================================================================
 * Description:
 * set video mode
 *
 *============================================================================*/
static cmr_int gc8024_set_video_mode(cmr_handle handle, cmr_uint param) {
    SENSOR_REG_T_PTR sensor_reg_ptr;
    cmr_u16 i = 0x00;
    cmr_u32 mode = 0;
    cmr_int ret = SENSOR_SUCCESS;

    SENSOR_IC_CHECK_PTR(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    if (param >= SENSOR_VIDEO_MODE_MAX)
        return 0;
    if (sns_drv_cxt->ops_cb.get_mode) {
        ret = sns_drv_cxt->ops_cb.get_mode(sns_drv_cxt->caller_handle, &mode);
        if (SENSOR_SUCCESS != ret) {
            SENSOR_LOGI("get mode fail.");
            return ret;
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

    return ret;
}

/*==============================================================================
 * Description:
 * calculate fps for every sensor mode according to frame_line and line_time
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int gc8024_drv_init_fps_info(cmr_handle handle) {
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

static cmr_int gc8024_drv_get_static_info(cmr_handle handle, cmr_uint *param) {
    cmr_int rtn = SENSOR_SUCCESS;
    struct sensor_ex_info *ex_info;
    cmr_u32 up = 0;
    cmr_u32 down = 0;

    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param);
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
        gc8024_drv_init_fps_info(handle);
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

static cmr_int gc8024_drv_get_fps_info(cmr_handle handle, cmr_uint *param) {
    cmr_int rtn = SENSOR_SUCCESS;
    SENSOR_MODE_FPS_T *fps_info;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param);

    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    struct sensor_fps_info *fps_data = sns_drv_cxt->fps_info;

    // make sure have inited fps of every sensor mode.
    if (!fps_data->is_init) {
        gc8024_drv_init_fps_info(handle);
    }
    fps_info = (SENSOR_MODE_FPS_T *)param;
    uint32_t sensor_mode = fps_info->mode;
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
#if 0
/*==============================================================================
 * Description:
 * get default frame length
 *
 *============================================================================*/
static cmr_u32 gc8024_drv_get_default_frame_length(cmr_u32 mode) {
    return RES_TRIM_TAB[mode].frame_line;
}
#endif
/*==============================================================================
 * Description:
 * write group-hold on to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void gc8024_drv_group_hold_on(void) {
    SENSOR_LOGI("E");

    // hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xYYYY, 0xff);
}

/*==============================================================================
 * Description:
 * write group-hold off to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void gc8024_drv_group_hold_off(void) {
    SENSOR_LOGI("E");

    // hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xYYYY, 0xff);
}

/*==============================================================================
 * Description:
 * read gain from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static cmr_u16 gc8024_drv_read_gain(void) {
    return s_sensor_ev_info.preview_gain;
}

/*==============================================================================
 * Description:
 * write gain to sensor registers
 * please modify this function acording your spec
 *============================================================================*/

#define ANALOG_GAIN_1 64   // 1.00x
#define ANALOG_GAIN_2 91   // 1.42x
#define ANALOG_GAIN_3 127  // 1.98x
#define ANALOG_GAIN_4 181  // 2.82x
#define ANALOG_GAIN_5 252  // 3.94x
#define ANALOG_GAIN_6 357  // 5.57x
#define ANALOG_GAIN_7 508  // 7.94x
#define ANALOG_GAIN_8 717  // 11.21x
#define ANALOG_GAIN_9 1012 // 15.81x

static void gc8024_drv_write_gain(cmr_handle handle, cmr_uint gain) {
    cmr_u16 temp = 0x00;
    cmr_u8 value = 0;

    SENSOR_IC_CHECK_HANDLE_VOID(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    PRIVATE_DATA *pri_data =
        sizeof(PRIVATE_DATA) > 4
            ? (PRIVATE_DATA *)sns_drv_cxt->privata_data.buffer
            : (PRIVATE_DATA *)&sns_drv_cxt->privata_data.data;
    SENSOR_IC_CHECK_PTR_VOID(pri_data);

    value = (hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0xfa) >> 7) & 0x01;

    if (value == 1)
        pri_data->PreorCap = 0;
    else
        pri_data->PreorCap = 1;
    if (SENSOR_MAX_GAIN < gain)
        gain = SENSOR_MAX_GAIN;
    if (gain < SENSOR_BASE_GAIN)
        gain = SENSOR_BASE_GAIN;
    if ((ANALOG_GAIN_1 <= gain) && (gain < ANALOG_GAIN_2)) {
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x21, 0x0b);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xd8, 0x20);
        // analog gain
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xb6, 0x00);
        temp = gain;
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xb1, temp >> 6);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xb2, (temp << 2) & 0xfc);
        pri_data->gainlevel = 0;
    } else if ((ANALOG_GAIN_2 <= gain) && (gain < ANALOG_GAIN_3)) {
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x21, 0x0b);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xd8, 0x20);
        // analog gain
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xb6, 0x01);
        temp = 64 * gain / ANALOG_GAIN_2;
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xb1, temp >> 6);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xb2, (temp << 2) & 0xfc);
        pri_data->gainlevel = 1;
    } else if ((ANALOG_GAIN_3 <= gain) && (gain < ANALOG_GAIN_4)) {
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x21, 0x10);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xd8, 0x20);
        // analog gain
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xb6, 0x02);
        temp = 64 * gain / ANALOG_GAIN_3;
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xb1, temp >> 6);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xb2, (temp << 2) & 0xfc);
        pri_data->gainlevel = 1;
    } else if ((ANALOG_GAIN_4 <= gain) && (gain < ANALOG_GAIN_5)) {
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x21, 0x10);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xd8, 0x20);
        // analog gain
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xb6, 0x03);
        temp = 64 * gain / ANALOG_GAIN_4;
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xb1, temp >> 6);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xb2, (temp << 2) & 0xfc);
        pri_data->gainlevel = 2;
    } else if ((ANALOG_GAIN_5 <= gain) && (gain < ANALOG_GAIN_6)) {
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x21, 0x10);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xd8, 0x18);
        // analog gain
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xb6, 0x04);
        temp = 64 * gain / ANALOG_GAIN_5;
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xb1, temp >> 6);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xb2, (temp << 2) & 0xfc);
        pri_data->gainlevel = 2;
    } else if ((ANALOG_GAIN_6 <= gain) && (gain < ANALOG_GAIN_7)) {
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x21, 0x12);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xd8, 0x12);
        // analog gain
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xb6, 0x05);
        temp = 64 * gain / ANALOG_GAIN_6;
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xb1, temp >> 6);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xb2, (temp << 2) & 0xfc);
        pri_data->gainlevel = 3;
    } else if ((ANALOG_GAIN_7 <= gain) && (gain < ANALOG_GAIN_8)) {
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x21, 0x12);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xd8, 0x12);
        // analog gain
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xb6, 0x06);
        temp = 64 * gain / ANALOG_GAIN_7;
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xb1, temp >> 6);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xb2, (temp << 2) & 0xfc);
        pri_data->gainlevel = 3;
    } else if ((ANALOG_GAIN_8 <= gain) && (gain < ANALOG_GAIN_9)) {
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x21, 0x12);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xd8, 0x12);
        // analog gain
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xb6, 0x07);
        temp = 64 * gain / ANALOG_GAIN_8;
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xb1, temp >> 6);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xb2, (temp << 2) & 0xfc);
        pri_data->gainlevel = 3;
    } else {
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x21, 0x12);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xd8, 0x12);
        // analog gain
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xb6, 0x08);
        temp = 64 * gain / ANALOG_GAIN_9;
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xb1, temp >> 6);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xb2, (temp << 2) & 0xfc);
        pri_data->gainlevel = 3;
    }

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x28,
                        Val28[pri_data->PreorCap][pri_data->gainlevel]);
}

/*==============================================================================
 * Description:
 * read frame length from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static cmr_u16 gc8024_drv_read_frame_length(void) {
    cmr_u16 frame_len_h = 0;
    cmr_u16 frame_len_l = 0;

    //	frame_len_h = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0xYYYY) & 0xff;
    //	frame_len_l = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0xYYYY) & 0xff;

    return ((frame_len_h << 8) | frame_len_l);
}

/*==============================================================================
 * Description:
 * write frame length to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void gc8024_drv_write_frame_length(cmr_handle handle,
                                          cmr_uint frame_len) {
    //	hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xYYYY, (frame_len >> 8) &
    // 0xff);
    //	hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xYYYY, frame_len & 0xff);
}

/*==============================================================================
 * Description:
 * write shutter to sensor registers
 * please pay attention to the frame length
 * please modify this function acording your spec
 *============================================================================*/
static void gc8024_drv_write_shutter(cmr_handle handle, cmr_uint shutter) {
    SENSOR_IC_CHECK_HANDLE_VOID(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    // if (!shutter)
    //     shutter = 1; /* avoid 0 */
    if (shutter < 1)
        shutter = 1;
    if (shutter > 8191)
        shutter = 8191;

    shutter = shutter * 4;

    // swicth txlow
    if (shutter <= 300) {
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x00);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x30, 0xfc);
    } else {
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x00);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x30, 0xf8);
    }

    // Update Shutter
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x00);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x04, (shutter)&0xFF);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x03, (shutter >> 8) & 0x7F);
}

/*==============================================================================
 * Description:
 * write exposure to sensor registers and get current shutter
 * please pay attention to the frame length
 * please don't change this function if it's necessary
 *============================================================================*/
static cmr_u16 gc8024_drv_update_exposure(cmr_handle handle, cmr_u32 shutter,
                                          cmr_u32 dummy_line) {
    cmr_u32 dest_fr_len = 0;
    cmr_u32 cur_fr_len = 0;
    cmr_u32 fr_len = 0;

    SENSOR_IC_CHECK_PTR(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    fr_len = sns_drv_cxt->frame_length_def;

    if (1 == SUPPORT_AUTO_FRAME_LENGTH)
        goto write_sensor_shutter;

    dest_fr_len = ((shutter + dummy_line + FRAME_OFFSET) > fr_len)
                      ? (shutter + dummy_line + FRAME_OFFSET)
                      : fr_len;

    cur_fr_len = gc8024_drv_read_frame_length();

    if (shutter < SENSOR_MIN_SHUTTER)
        shutter = SENSOR_MIN_SHUTTER;

    if (dest_fr_len != cur_fr_len)
        gc8024_drv_write_frame_length(handle, dest_fr_len);
write_sensor_shutter:
    /* write shutter to sensor registers */
    gc8024_drv_write_shutter(handle, shutter);
#ifdef GAIN_DELAY_1_FRAME
    usleep(dest_fr_len * PREVIEW_LINE_TIME / 10);
#endif
    return shutter;
}

/*==============================================================================
 * Description:
 * sensor power on
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int gc8024_drv_power_on(cmr_handle handle, cmr_uint power_on) {
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    struct module_cfg_info *module_info = sns_drv_cxt->module_info;

    SENSOR_AVDD_VAL_E dvdd_val = module_info->dvdd_val;
    SENSOR_AVDD_VAL_E avdd_val = module_info->avdd_val;
    SENSOR_AVDD_VAL_E iovdd_val = module_info->iovdd_val;
    BOOLEAN power_down = MIPI_RAW_INFO.power_down_level;
    BOOLEAN reset_level = MIPI_RAW_INFO.reset_pulse_level;

    if (SENSOR_TRUE == power_on) {
        hw_sensor_power_down(sns_drv_cxt->hw_handle, power_down);
        usleep(12 * 1000);
        hw_sensor_set_iovdd_val(sns_drv_cxt->hw_handle, iovdd_val);
        usleep(1 * 1000);
        hw_sensor_set_dvdd_val(sns_drv_cxt->hw_handle, dvdd_val);
        usleep(1 * 1000);

        hw_sensor_set_avdd_val(sns_drv_cxt->hw_handle, avdd_val);
        usleep(1 * 1000);
        hw_sensor_set_mclk(sns_drv_cxt->hw_handle, EX_MCLK);
        usleep(1 * 1000);
        hw_sensor_power_down(sns_drv_cxt->hw_handle, !power_down);
        usleep(1 * 1000);
        hw_sensor_set_reset_level(sns_drv_cxt->hw_handle, !reset_level);
    } else {
        hw_sensor_power_down(sns_drv_cxt->hw_handle, power_down);
        hw_sensor_set_reset_level(sns_drv_cxt->hw_handle, reset_level);
        hw_sensor_set_mclk(sns_drv_cxt->hw_handle, SENSOR_DISABLE_MCLK);
        hw_sensor_set_avdd_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED);
        hw_sensor_set_dvdd_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED);
        hw_sensor_set_iovdd_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED);
        hw_sensor_power_down(sns_drv_cxt->hw_handle, !power_down);
    }
    SENSOR_LOGI("(1:on, 0:off): %ld", power_on);
    return SENSOR_SUCCESS;
}

/*==============================================================================
 * Description:
 * cfg otp setting
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int gc8024_drv_access_val(cmr_handle handle, cmr_uint param) {
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_VAL_T *param_ptr = (SENSOR_VAL_T *)param;
    SENSOR_IC_CHECK_PTR(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    if (!param_ptr) {
#ifdef FEATURE_OTP
        if (PNULL != s_gc8024_raw_param_tab_ptr->cfg_otp) {
            ret = s_gc8024_raw_param_tab_ptr->cfg_otp(s_gc8024_otp_info_ptr);
            // checking OTP apply result
            if (SENSOR_SUCCESS != ret) {
                SENSOR_LOGI("apply otp failed");
            }
        } else {
            SENSOR_LOGI("no update otp function!");
        }
#endif
        return ret;
    }

    SENSOR_LOGI("sensor gc8024: param_ptr->type=%x", param_ptr->type);

    switch (param_ptr->type) {
    case SENSOR_VAL_TYPE_SHUTTER:
        //*((uint32_t *)param_ptr->pval) = gc8024_read_shutter();
        *((uint32_t *)param_ptr->pval) =
            sns_drv_cxt->sensor_ev_info.preview_shutter;
        break;
    case SENSOR_VAL_TYPE_READ_OTP_GAIN:
        //*((uint32_t *)param_ptr->pval) = gc8024_read_gain();
        *((uint32_t *)param_ptr->pval) =
            sns_drv_cxt->sensor_ev_info.preview_gain;
        break;
    case SENSOR_VAL_TYPE_GET_STATIC_INFO:
        ret = gc8024_drv_get_static_info(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_GET_FPS_INFO:
        ret = gc8024_drv_get_fps_info(handle, param_ptr->pval);
        break;
    default:
        break;
    }

    return ret;
}

/*==============================================================================
 * Description:
 * Initialize Exif Info
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int gc8024_drv_init_exif_info(cmr_handle handle, void **param) {
    cmr_int ret = SENSOR_FAIL;
    EXIF_SPEC_PIC_TAKING_COND_T *sexif = NULL;

    SENSOR_IC_CHECK_PTR(param);
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    ret = sensor_ic_get_init_exif_info(sns_drv_cxt, &sexif);
    SENSOR_IC_CHECK_PTR(sexif);
    *param = sexif;
    /*you can add other init data,here*/
    return ret;
}

/*==============================================================================
 * Description:
 * identify sensor id
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int gc8024_drv_identify(cmr_handle handle, cmr_uint param) {
    UNUSED(param);
    cmr_u16 pid_value = 0x00;
    cmr_u16 ver_value = 0x00;
    cmr_u16 ret_value = SENSOR_FAIL;

    SENSOR_LOGI("mipi raw identify");
    SENSOR_IC_CHECK_PTR(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    pid_value = hw_sensor_read_reg(sns_drv_cxt->hw_handle, GC8024_PID_ADDR);

    if (GC8024_PID_VALUE == pid_value) {
        ver_value = hw_sensor_read_reg(sns_drv_cxt->hw_handle, GC8024_VER_ADDR);
        SENSOR_LOGI("Identify: PID = %x, VER = %x", pid_value, ver_value);
        if (GC8024_VER_VALUE == ver_value) {
            SENSOR_LOGI("this is gc8024 sensor");

#ifdef FEATURE_OTP
            /*if read otp info failed or module id mismatched ,identify failed
             * ,return SENSOR_FAIL ,exit identify*/
            if (PNULL != s_gc8024_raw_param_tab_ptr->identify_otp) {
                SENSOR_LOGI("identify module_id=0x%x",
                            s_gc8024_raw_param_tab_ptr->param_id);
                // set default value
                memset(s_gc8024_otp_info_ptr, 0x00, sizeof(struct otp_info_t));
                ret_value = s_gc8024_raw_param_tab_ptr->identify_otp(
                    s_gc8024_otp_info_ptr);

                if (SENSOR_SUCCESS == ret_value) {
                    SENSOR_LOGI(
                        "identify otp sucess! module_id=0x%x, module_name=%s",
                        s_gc8024_raw_param_tab_ptr->param_id, MODULE_NAME);
                } else {
                    SENSOR_LOGI("identify otp fail! exit identify");
                    return ret_value;
                }
            } else {
                SENSOR_LOGI("no identify_otp function!");
            }
#endif
            ret_value = SENSOR_SUCCESS;
            SENSOR_LOGI("this is gc8024 sensor");
        } else {
            SENSOR_LOGI("Identify this is %x%x sensor", pid_value, ver_value);
        }
    } else {
        SENSOR_LOGE("sensor identify fail, pid_value = %x", pid_value);
    }

    return ret_value;
}
#if 0
/*==============================================================================
 * Description:
 * get resolution trim
 *
 *============================================================================*/
static cmr_uint gc8024_drv_get_trim_tab(cmr_handle handle, cmr_uint param) {
    return (cmr_uint)RES_TRIM_TAB;
}
#endif
/*==============================================================================
 * Description:
 * before snapshot
 * you can change this function if it's necessary
 *============================================================================*/
static cmr_int gc8024_drv_before_snapshot(cmr_handle handle, cmr_uint param) {
    uint32_t cap_shutter = 0;
    uint32_t prv_shutter = 0;
    uint32_t gain = 0;
    uint32_t cap_gain = 0;
    uint32_t capture_mode = param & 0xffff;
    uint32_t preview_mode = (param >> 0x10) & 0xffff;

    SENSOR_IC_CHECK_PTR(handle);
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

    cap_shutter = gc8024_drv_update_exposure(handle, cap_shutter, 0);
    cap_gain = gain;
    gc8024_drv_write_gain(handle, cap_gain);
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
static cmr_int gc8024_drv_write_exposure(cmr_handle handle, cmr_uint param) {
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
        gc8024_drv_update_exposure(handle, exposure_line, dummy_line);

    return ret_value;
}

/*==============================================================================
 * Description:
 * get the parameter from isp to real gain
 * you mustn't change the funcion !
 *============================================================================*/
static uint32_t isp_to_real_gain(cmr_uint param) {
    uint32_t real_gain = 0;

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
static cmr_int gc8024_drv_write_gain_value(cmr_handle handle, cmr_uint param) {
    cmr_int ret_value = SENSOR_SUCCESS;
    cmr_u32 real_gain = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    real_gain = isp_to_real_gain(param);

    real_gain = real_gain * SENSOR_BASE_GAIN / ISP_BASE_GAIN;

    SENSOR_LOGI("real_gain = 0x%x", real_gain);

    sns_drv_cxt->sensor_ev_info.preview_gain = real_gain;
    gc8024_drv_write_gain(handle, real_gain);

    return ret_value;
}

/*==============================================================================
 * Description:
 * increase gain or shutter for hdr
 *
 *============================================================================*/
static void gc8024_drv_increase_hdr_exposure(cmr_handle handle,
                                             cmr_u8 ev_multiplier) {
    struct hdr_info_t *hdr_info = NULL;
    cmr_u32 gain = 0;
    cmr_u32 shutter_multiply = 0;

    SENSOR_IC_CHECK_HANDLE_VOID(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    hdr_info = &sns_drv_cxt->hdr_info;

    shutter_multiply =
        hdr_info->capture_max_shutter / hdr_info->capture_shutter;

    if (0 == shutter_multiply)
        shutter_multiply = 1;

    if (shutter_multiply >= ev_multiplier) {
        gc8024_drv_update_exposure(
            handle, hdr_info->capture_shutter * ev_multiplier, 0);
        gc8024_drv_write_gain(handle, hdr_info->capture_gain);
    } else {
        gain = hdr_info->capture_gain * ev_multiplier / shutter_multiply;
        if (SENSOR_MAX_GAIN < gain)
            gain = SENSOR_MAX_GAIN;
        gc8024_drv_update_exposure(
            handle, hdr_info->capture_shutter * shutter_multiply, 0);
        gc8024_drv_write_gain(handle, gain);
    }
}

/*==============================================================================
 * Description:
 * decrease gain or shutter for hdr
 *
 *============================================================================*/
static void gc8024_drv_decrease_hdr_exposure(cmr_handle handle,
                                             cmr_u8 ev_divisor) {
    cmr_u16 gain_multiply = 0;
    cmr_u32 shutter = 0;
    struct hdr_info_t *hdr_info = NULL;

    SENSOR_IC_CHECK_HANDLE_VOID(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    hdr_info = &sns_drv_cxt->hdr_info;

    gain_multiply = hdr_info->capture_gain / SENSOR_BASE_GAIN;

    if (gain_multiply >= ev_divisor) {
        gc8024_drv_update_exposure(handle, hdr_info->capture_shutter, 0);
        gc8024_drv_write_gain(handle, hdr_info->capture_gain / ev_divisor);
    } else {
        shutter = hdr_info->capture_shutter * gain_multiply / ev_divisor;
        gc8024_drv_update_exposure(handle, shutter, 0);
        gc8024_drv_write_gain(handle, hdr_info->capture_gain / gain_multiply);
    }
}

/*==============================================================================
 * Description:
 * set hdr ev
 * you can change this function if it's necessary
 *============================================================================*/
static cmr_int gc8024_drv_set_hdr_ev(cmr_handle handle, cmr_uint param) {
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR)param;
    SENSOR_IC_CHECK_PTR(ext_ptr);

    cmr_u32 ev = ext_ptr->param;
    cmr_u8 ev_divisor, ev_multiplier;

    switch (ev) {
    case SENSOR_HDR_EV_LEVE_0:
        ev_divisor = 2;
        gc8024_drv_decrease_hdr_exposure(handle, ev_divisor);
        break;
    case SENSOR_HDR_EV_LEVE_1:
        ev_multiplier = 2;
        gc8024_drv_increase_hdr_exposure(handle, ev_multiplier);
        break;
    case SENSOR_HDR_EV_LEVE_2:
        ev_multiplier = 1;
        gc8024_drv_increase_hdr_exposure(handle, ev_multiplier);
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
static cmr_int gc8024_drv_ext_func(cmr_handle handle, cmr_uint param) {
    cmr_int rtn = SENSOR_SUCCESS;
    SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR)param;
    SENSOR_IC_CHECK_PTR(ext_ptr);

    SENSOR_LOGI("ext_ptr->cmd: %d", ext_ptr->cmd);
    switch (ext_ptr->cmd) {
    case SENSOR_EXT_EV:
        rtn = gc8024_drv_set_hdr_ev(handle, param);
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
static cmr_int gc8024_drv_stream_on(cmr_handle handle, cmr_uint param) {
    SENSOR_LOGI("E");
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x03);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x10, 0x91);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x00);

    return 0;
}

/*==============================================================================
 * Description:
 * mipi stream off
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int gc8024_drv_stream_off(cmr_handle handle, cmr_uint param) {
    SENSOR_LOGI("E");
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x03);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x10, 0x01);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x00);
    usleep(50 * 1000);

    return 0;
}

/**
 * gc8024_drv_handle_create- create gc8024 sensor ic instance.
 * @init_param: init_param transmit from caller.
 * @reg_addr: register address to read.
 *
 * NOTE: when open one camera instance,the interface will be called
 *       first. you can init sensor ic instance and static variables.
 **/
static cmr_int
gc8024_drv_handle_create(struct sensor_ic_drv_init_para *init_param,
                         cmr_handle *sns_ic_drv_handle) {
    cmr_int ret = SENSOR_SUCCESS;
    struct sensor_ic_drv_cxt *sns_drv_cxt = NULL;
    void *pri_data = NULL;
    cmr_u32 pri_data_size = sizeof(PRIVATE_DATA);

    ret = sensor_ic_drv_create(init_param, sns_ic_drv_handle);
    if (SENSOR_IC_FAILED == ret) {
        SENSOR_LOGE("sensor instance create failed!");
        return ret;
    }
    sns_drv_cxt = *sns_ic_drv_handle;

    if (pri_data_size > 4) {
        pri_data = (PRIVATE_DATA *)malloc(pri_data_size);
        if (pri_data)
            sns_drv_cxt->privata_data.buffer = (cmr_u8 *)pri_data;
        else
            SENSOR_LOGE("malloc private data failed: private_data_size:%d",
                        pri_data_size);
    }
    sns_drv_cxt->exif_ptr = (void *)&s_gc8024_exif_info;
    /*init hdr infomation*/
    sns_drv_cxt->hdr_info.capture_max_shutter = 2000000000 / SNAPSHOT_LINE_TIME;
    sns_drv_cxt->hdr_info.capture_shutter =
        SNAPSHOT_FRAME_LENGTH - FRAME_OFFSET;
    sns_drv_cxt->hdr_info.capture_gain = SENSOR_BASE_GAIN;

    sns_drv_cxt->sensor_ev_info.preview_shutter =
        PREVIEW_FRAME_LENGTH - FRAME_OFFSET;
    sns_drv_cxt->sensor_ev_info.preview_gain = SENSOR_BASE_GAIN;
    sns_drv_cxt->sensor_ev_info.preview_framelength = PREVIEW_FRAME_LENGTH;

    /*get match private data*/
    sensor_ic_set_match_module_info(sns_drv_cxt, ARRAY_SIZE(MODULE_INFO),
                                    MODULE_INFO);
    sensor_ic_set_match_resolution_info(sns_drv_cxt, ARRAY_SIZE(RES_TAB_RAW),
                                        RES_TAB_RAW);
    sensor_ic_set_match_trim_info(sns_drv_cxt, ARRAY_SIZE(RES_TRIM_TAB),
                                  RES_TRIM_TAB);
    sensor_ic_set_match_static_info(sns_drv_cxt, ARRAY_SIZE(STATIC_INFO),
                                    STATIC_INFO);
    sensor_ic_set_match_fps_info(sns_drv_cxt, ARRAY_SIZE(FPS_INFO), FPS_INFO);

    /*init static variables*/
    gc8024_drv_init_exif_info(sns_drv_cxt, &(sns_drv_cxt->exif_ptr));

    /*add private here*/
    return ret;
}

static cmr_int gc8024_drv_handle_delete(cmr_handle handle, void *param) {
    cmr_int ret = SENSOR_SUCCESS;
    cmr_u32 pri_data_size = sizeof(PRIVATE_DATA);

    /*if has private data,you must release it here*/
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    if ((pri_data_size > 4) && sns_drv_cxt->privata_data.buffer)
        free(sns_drv_cxt->privata_data.buffer);

    ret = sensor_ic_drv_delete(handle, param);
    return ret;
}

static cmr_int gc8024_drv_get_private_data(cmr_handle handle, cmr_uint cmd,
                                           void **param) {
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param);

    ret = sensor_ic_get_private_data(handle, cmd, param);
    return ret;
}

/*==============================================================================
 * Description:
 * all ioctl functoins
 * you can add functions reference SENSOR_IOCTL_FUNC_TAB_T from sensor_drv_u.h
 *
 * add ioctl functions like this:
 *============================================================================*/
static struct sensor_ic_ops s_gc8024_ops_tab = {
    .create_handle = gc8024_drv_handle_create,
    .delete_handle = gc8024_drv_handle_delete,
    .get_data = gc8024_drv_get_private_data,
    .power = gc8024_drv_power_on,
    .identify = gc8024_drv_identify,
    .ex_write_exp = gc8024_drv_write_exposure,
    .write_gain_value = gc8024_drv_write_gain_value,

    .ext_ops = {
            [SENSOR_IOCTL_EXT_FUNC].ops = gc8024_drv_ext_func,
            [SENSOR_IOCTL_BEFORE_SNAPSHOT].ops = gc8024_drv_before_snapshot,
            [SENSOR_IOCTL_STREAM_ON].ops = gc8024_drv_stream_on,
            [SENSOR_IOCTL_STREAM_OFF].ops = gc8024_drv_stream_off,
            [SENSOR_IOCTL_ACCESS_VAL].ops = gc8024_drv_access_val,
    }
};
