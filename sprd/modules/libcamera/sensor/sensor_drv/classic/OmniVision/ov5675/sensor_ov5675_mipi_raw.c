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
#define LOG_TAG "ov5675_mipi_raw"
#include "sensor_ov5675_mipi_raw.h"

#define FPS_INFO s_ov5675_mode_fps_info
#define STATIC_INFO s_ov5675_static_info
#define VIDEO_INFO s_ov5675_video_info
#define MODULE_INFO s_ov5675_module_info_tab
#define RES_TAB_RAW s_ov5675_resolution_tab_raw
#define RES_TRIM_TAB s_ov5675_resolution_trim_tab
#define RES_TAB_RAW_NEW s_ov5675_resolution_tab_raw_new
#define RES_TRIM_TAB_NEW s_ov5675_resolution_trim_tab_new
#define MIPI_RAW_INFO g_ov5675_mipi_raw_info

static EXIF_SPEC_PIC_TAKING_COND_T s_ov5675_exif_info;

/*==============================================================================
 * Description:
 * set video mode
 *============================================================================*/
static cmr_int ov5675_drv_set_video_mode(cmr_handle handle, cmr_uint param) {
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_REG_T_PTR sensor_reg_ptr = PNULL;
    cmr_u16 i = 0x00;
    cmr_u32 mode = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    if (param >= SENSOR_VIDEO_MODE_MAX)
        return SENSOR_FAIL;

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
    return ret;
}

/*==============================================================================
 * Description:
 * calculate fps for every sensor mode according to frame_line and line_time
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int ov5675_drv_init_fps_info(cmr_handle handle) {
    cmr_int rtn = SENSOR_SUCCESS;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

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
    SENSOR_LOGI("X:");
    return rtn;
}

static cmr_int ov5675_drv_get_static_info(cmr_handle handle, cmr_u32 *param) {
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
        ov5675_drv_init_fps_info(handle);
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

    memcpy(&ex_info->fov_info, &static_info->fov_info,
           sizeof(static_info->fov_info));

    sensor_ic_print_static_info((cmr_s8 *)SENSOR_NAME, ex_info);

    return rtn;
}

static cmr_int ov5675_drv_get_fps_info(cmr_handle handle, cmr_u32 *param) {
    cmr_int rtn = SENSOR_SUCCESS;
    SENSOR_MODE_FPS_T *fps_info;

    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    struct sensor_fps_info *fps_data = sns_drv_cxt->fps_info;

    // make sure have inited fps of every sensor mode.
    if (!fps_data->is_init) {
        ov5675_drv_init_fps_info(handle);
    }
    SENSOR_IC_CHECK_PTR(param);
    fps_info = (SENSOR_MODE_FPS_T *)param;
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
 * get default frame length
 *
 *============================================================================*/
#if 0
static uint32_t ov5675_drv_get_default_frame_length(cmr_handle handle,
                                                                  cmr_u32 mode) {
    return RES_TRIM_TAB[mode].frame_line;
}
#endif
/*==============================================================================
 * Description:
 * write group-hold on to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void ov5675_drv_group_hold_on(cmr_handle handle) {
    UNUSED(handle);
    SENSOR_LOGI("E");

    // Sensor_WriteReg(0x3208, 0x001);
}

/*==============================================================================
 * Description:
 * write group-hold off to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void ov5675_drv_group_hold_off(cmr_handle handle) {
    UNUSED(handle);
    SENSOR_LOGI("E");

    // Sensor_WriteReg(0x3208, 0x10);
    // Sensor_WriteReg(0x3208, 0xa0);
}

/*==============================================================================
 * Description:
 * write gain to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void ov5675_drv_write_gain(cmr_handle handle, cmr_uint gain) {
    SENSOR_IC_CHECK_HANDLE_VOID(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    if (SENSOR_MAX_GAIN < gain)
        gain = SENSOR_MAX_GAIN;
    cmr_u16 value = 0;

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3208, 0x00);

    value = (gain >> 8) & 0x1f;
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3508, value);

    value = gain & 0xff;
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3509, value);

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3208, 0x10);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3208, 0xa0);
}

/*==============================================================================
 * Description:
 * read frame length from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static cmr_u16 ov5675_drv_read_frame_length(cmr_handle handle) {
    cmr_u16 frame_len_h = 0;
    cmr_u16 frame_len_l = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    frame_len_h = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x380E) & 0xff;
    frame_len_l = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x380F) & 0xff;

    return ((frame_len_h << 8) | frame_len_l);
}

/*==============================================================================
 * Description:
 * write frame length to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void ov5675_drv_write_frame_length(cmr_handle handle,
                                          cmr_u32 frame_len) {
    SENSOR_IC_CHECK_HANDLE_VOID(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x380E,
                        (frame_len >> 8) & 0xff);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x380F, frame_len & 0xff);
}

/*==============================================================================
 * Description:
 * write shutter to sensor registers
 * please pay attention to the frame length
 * please modify this function acording your spec
 *============================================================================*/
static void ov5675_drv_write_shutter(cmr_handle handle, cmr_u32 shutter) {
    SENSOR_IC_CHECK_HANDLE_VOID(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    cmr_u16 value = 0x00;
    cmr_u16 ov_shutter = (shutter >> 1);
    value = (ov_shutter << 0x04) & 0xff;
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3502, value);

    value = (ov_shutter >> 0x04) & 0xff;
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3501, value);

    value = (ov_shutter >> 0x0c) & 0x0f;
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3500, value);
}

/*==============================================================================
 * Description:
 * write exposure to sensor registers and get current shutter
 * please pay attention to the frame length
 * please don't change this function if it's necessary
 *============================================================================*/
static uint16_t ov5675_drv_update_exposure(cmr_handle handle, uint32_t shutter,
                                           uint32_t dummy_line) {
    uint32_t dest_fr_len = 0;
    uint32_t cur_fr_len = 0;
    uint32_t fr_len = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    // ov5675_group_hold_on(handle);
    fr_len = sns_drv_cxt->frame_length_def;
    if (1 == SUPPORT_AUTO_FRAME_LENGTH)
        goto write_sensor_shutter;

    dest_fr_len = ((shutter + dummy_line + FRAME_OFFSET) > fr_len)
                      ? (shutter + dummy_line + FRAME_OFFSET)
                      : fr_len;

    cur_fr_len = ov5675_drv_read_frame_length(handle);

    if (shutter < SENSOR_MIN_SHUTTER)
        shutter = SENSOR_MIN_SHUTTER;

    if (dest_fr_len != cur_fr_len)
        ov5675_drv_write_frame_length(handle, dest_fr_len);
write_sensor_shutter:
    /* write shutter to sensor registers */
    ov5675_drv_write_shutter(handle, shutter);

    if (sns_drv_cxt->ops_cb.set_exif_info) {
        sns_drv_cxt->ops_cb.set_exif_info(
            sns_drv_cxt->caller_handle, SENSOR_EXIF_CTRL_EXPOSURETIME, shutter);
    }

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
static cmr_int ov5675_drv_power_on(cmr_handle handle, cmr_uint power_on) {
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    struct module_cfg_info *module_info = sns_drv_cxt->module_info;

    SENSOR_AVDD_VAL_E dvdd_val = module_info->dvdd_val;
    SENSOR_AVDD_VAL_E avdd_val = module_info->avdd_val;
    SENSOR_AVDD_VAL_E iovdd_val = module_info->iovdd_val;
    BOOLEAN power_down = MIPI_RAW_INFO.power_down_level;
    BOOLEAN reset_level = MIPI_RAW_INFO.reset_pulse_level;

    if (SENSOR_TRUE == power_on) {
        hw_sensor_set_reset_level(sns_drv_cxt->hw_handle, reset_level);
        usleep(1 * 1000);
        hw_sensor_set_voltage(sns_drv_cxt->hw_handle, dvdd_val, avdd_val,
                              iovdd_val);
        hw_sensor_set_avdd_val(sns_drv_cxt->hw_handle, avdd_val);
        hw_sensor_set_dvdd_val(sns_drv_cxt->hw_handle, dvdd_val);
        hw_sensor_set_iovdd_val(sns_drv_cxt->hw_handle, iovdd_val);
        usleep(1 * 1000);
        hw_sensor_set_reset_level(sns_drv_cxt->hw_handle, !reset_level);
        usleep(1 * 1000);
        hw_sensor_set_mclk(sns_drv_cxt->hw_handle, EX_MCLK);
#if defined(CONFIG_DUAL_MODULE)
        hw_sensor_set_mipi_level(sns_drv_cxt->hw_handle, 0);
#endif
    } else {
        hw_sensor_set_mclk(sns_drv_cxt->hw_handle, SENSOR_DISABLE_MCLK);
        hw_sensor_set_reset_level(sns_drv_cxt->hw_handle, reset_level);
        hw_sensor_set_dvdd_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED);
        usleep(1 * 1000);
        hw_sensor_set_avdd_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED);
        usleep(1 * 1000);
        hw_sensor_set_iovdd_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED);
#if defined(CONFIG_DUAL_MODULE)
        hw_sensor_set_mipi_level(sns_drv_cxt->hw_handle, 1);
#endif
    }
    SENSOR_LOGI("(1:on, 0:off): %ld", power_on);
    return SENSOR_SUCCESS;
}

/*==============================================================================
 * Description:
 * Initialize Exif Info
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int ov5675_drv_init_exif_info(cmr_handle handle,
                                         void **exif_info_in /*in*/) {
    cmr_int ret = SENSOR_FAIL;
    EXIF_SPEC_PIC_TAKING_COND_T *exif_ptr = NULL;
    *exif_info_in = NULL;
    SENSOR_IC_CHECK_HANDLE(handle);

    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    ret = sensor_ic_get_init_exif_info(sns_drv_cxt, &exif_ptr);
    SENSOR_IC_CHECK_PTR(exif_ptr);
    *exif_info_in = exif_ptr;

    SENSOR_LOGI("Start");
    /*aperture = numerator/denominator */
    /*fnumber = numerator/denominator */
    exif_ptr->valid.FNumber = 1;
    exif_ptr->FNumber.numerator = 14;
    exif_ptr->FNumber.denominator = 5;

    exif_ptr->valid.ApertureValue = 1;
    exif_ptr->ApertureValue.numerator = 14;
    exif_ptr->ApertureValue.denominator = 5;
    exif_ptr->valid.MaxApertureValue = 1;
    exif_ptr->MaxApertureValue.numerator = 14;
    exif_ptr->MaxApertureValue.denominator = 5;
    exif_ptr->valid.FocalLength = 1;
    exif_ptr->FocalLength.numerator = 289;
    exif_ptr->FocalLength.denominator = 100;

    return ret;
}

/*==============================================================================
 * Description:
 * identify sensor id
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int ov5675_drv_identify(cmr_handle handle, cmr_uint param) {
    cmr_u16 pid_value = 0x00;
    cmr_u16 ver_value = 0x00;
    cmr_int ret_value = SENSOR_FAIL;
    UNUSED(param);
    SENSOR_IC_CHECK_HANDLE(handle);

    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("mipi raw identify");
    pid_value = hw_sensor_read_reg(sns_drv_cxt->hw_handle, ov5675_PID_ADDR);

    if (ov5675_PID_VALUE == pid_value) {
        ver_value = hw_sensor_read_reg(sns_drv_cxt->hw_handle, ov5675_VER_ADDR);
        SENSOR_LOGI("Identify: PID = %x, VER = %x", pid_value, ver_value);
        if (ov5675_VER_VALUE == ver_value) {
            SENSOR_LOGI("this is ov5675 sensor");
            ov5675_drv_init_fps_info(handle);
            ret_value = SENSOR_SUCCESS;
        } else {
            SENSOR_LOGI("Identify this is %x%x sensor", pid_value, ver_value);
        }
    } else {
        SENSOR_LOGE("sensor identify fail, pid_value = %x", pid_value);
    }
    return ret_value;
}

/*==============================================================================
 * Description:
 * before snapshot
 * you can change this function if it's necessary
 *============================================================================*/
static cmr_int ov5675_drv_before_snapshot(cmr_handle handle, cmr_uint param) {
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
    // ov5675_get_default_frame_length(handle, capture_mode);
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

    cap_shutter = ov5675_drv_update_exposure(handle, cap_shutter, 0);
    cap_gain = gain;
    ov5675_drv_write_gain(handle, cap_gain);
    SENSOR_LOGI("preview_shutter = %d, preview_gain = %f",
                sns_drv_cxt->sensor_ev_info.preview_shutter,
                sns_drv_cxt->sensor_ev_info.preview_gain);

    SENSOR_LOGI("capture_shutter = %d, capture_gain = %d", cap_shutter,
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
        sns_drv_cxt->exif_info.exposure_line = cap_shutter;
    }

    return SENSOR_SUCCESS;
}

/*==============================================================================
 * Description:
 * get the shutter from isp
 * please don't change this function unless it's necessary
 *============================================================================*/
static cmr_int ov5675_drv_write_exposure(cmr_handle handle, cmr_uint param) {
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
    // ov5675_get_default_frame_length(handle, mode);

    sns_drv_cxt->sensor_ev_info.preview_shutter =
        ov5675_drv_update_exposure(handle, exposure_line, dummy_line);

    return ret_value;
}

/*==============================================================================
 * Description:
 * get the parameter from isp to real gain
 * you mustn't change the funcion !
 *============================================================================*/
static cmr_u32 isp_to_real_gain(cmr_handle handle, uint32_t param) {
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
static cmr_int ov5675_drv_write_gain_value(cmr_handle handle, cmr_uint param) {
    cmr_int ret_value = SENSOR_SUCCESS;
    cmr_u32 real_gain = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    real_gain = isp_to_real_gain(handle, param);

    real_gain = real_gain * SENSOR_BASE_GAIN / ISP_BASE_GAIN;

    SENSOR_LOGI("real_gain = %d", real_gain);

    sns_drv_cxt->sensor_ev_info.preview_gain = real_gain;
    ov5675_drv_write_gain(handle, real_gain);

    return ret_value;
}

/*==============================================================================
 * Description:
 * increase gain or shutter for hdr
 *
 *============================================================================*/
static void ov5675_drv_increase_hdr_exposure(cmr_handle handle,
                                             cmr_u8 ev_multiplier) {
    struct hdr_info_t *hdr_info = NULL;
    cmr_u32 gain = 0;

    SENSOR_IC_CHECK_HANDLE_VOID(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    hdr_info = &sns_drv_cxt->hdr_info;

    cmr_u32 shutter_multiply =
        hdr_info->capture_max_shutter / hdr_info->capture_shutter;

    if (0 == shutter_multiply)
        shutter_multiply = 1;

    if (shutter_multiply >= ev_multiplier) {
        ov5675_drv_update_exposure(
            handle, hdr_info->capture_shutter * ev_multiplier, 0);
        ov5675_drv_write_gain(handle, hdr_info->capture_gain);
    } else {
        gain = hdr_info->capture_gain * ev_multiplier / shutter_multiply;
        ov5675_drv_update_exposure(
            handle, hdr_info->capture_shutter * shutter_multiply, 0);
        ov5675_drv_write_gain(handle, gain);
    }
}

/*==============================================================================
 * Description:
 * decrease gain or shutter for hdr
 *
 *============================================================================*/
static void ov5675_drv_decrease_hdr_exposure(cmr_handle handle,
                                             cmr_u8 ev_divisor) {
    cmr_u16 gain_multiply = 0;
    cmr_u32 shutter = 0;
    struct hdr_info_t *hdr_info = NULL;

    SENSOR_IC_CHECK_HANDLE_VOID(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    hdr_info = &sns_drv_cxt->hdr_info;

    gain_multiply = hdr_info->capture_gain / SENSOR_BASE_GAIN;

    if (gain_multiply >= ev_divisor) {
        ov5675_drv_update_exposure(handle, hdr_info->capture_shutter, 0);
        ov5675_drv_write_gain(handle, hdr_info->capture_gain / ev_divisor);

    } else {
        shutter = hdr_info->capture_shutter * gain_multiply / ev_divisor;
        ov5675_drv_update_exposure(handle, shutter, 0);
        ov5675_drv_write_gain(handle, hdr_info->capture_gain / gain_multiply);
    }
}

/*==============================================================================
 * Description:
 * set hdr ev
 * you can change this function if it's necessary
 *============================================================================*/
static cmr_int ov5675_drv_set_hdr_ev(cmr_handle handle, cmr_uint param) {
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR)param;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(ext_ptr);

    cmr_s32 ev = ext_ptr->param;
    cmr_u8 ev_divisor, ev_multiplier;

    switch (ev) {
    case SENSOR_HDR_EV_LEVE_0:
        ev_divisor = 2;
        ov5675_drv_decrease_hdr_exposure(handle, ev_divisor);
        break;
    case SENSOR_HDR_EV_LEVE_1:
        ev_multiplier = 1;
        ov5675_drv_increase_hdr_exposure(handle, ev_multiplier);
        break;
    case SENSOR_HDR_EV_LEVE_2:
        ev_multiplier = 2;
        ov5675_drv_increase_hdr_exposure(handle, ev_multiplier);
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
static cmr_int ov5675_drv_ext_func(cmr_handle handle, cmr_uint param) {
    cmr_int rtn = SENSOR_SUCCESS;
    SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR)param;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(ext_ptr);

    SENSOR_LOGI("E:cmd: %d", ext_ptr->cmd);
    switch (ext_ptr->cmd) {
    case SENSOR_EXT_EV:
        rtn = ov5675_drv_set_hdr_ev(handle, param);
        break;
    default:
        break;
    }

    return rtn;
}

static cmr_int ov5675_drv_set_frame_sync(cmr_handle handle, cmr_uint param) {
    SENSOR_LOGI("E");
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    /* setting from ov5675 */
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3663, 0x20);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3002, 0x21);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3823, 0x70);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3824, 0x00);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3825, 0xbb);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3826, 0x0f);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3827, 0x90);
    return 0;
}

/*==============================================================================
 * Description:
 * mipi stream on
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int ov5675_drv_stream_on(cmr_handle handle, cmr_uint param) {
    UNUSED(param);
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("E:module_id=%d", sns_drv_cxt->module_id);
    if (sns_drv_cxt->module_id == MODULE_SUNNY)
        ov5675_drv_set_frame_sync(handle, 0);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0100, 0x01);

    return 0;
}

/*==============================================================================
 * Description:
 * mipi stream off
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int ov5675_drv_stream_off(cmr_handle handle, cmr_uint param) {

    UNUSED(param);
    unsigned char value;
    unsigned int sleep_time = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    SENSOR_LOGI("E:");

    value = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0100);
    if (value != 0x00) {
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0100, 0x00);
        if (!sns_drv_cxt->is_sensor_close) {
            sleep_time = 50 * 1000;
            usleep(sleep_time);
        }
    } else {
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0100, 0x00);
    }

    sns_drv_cxt->is_sensor_close = 0;
    SENSOR_LOGI("X:sleep_time=%dus", sleep_time);
    return 0;
}

static cmr_int ov5675_drv_access_val(cmr_handle handle, unsigned long param) {
    cmr_int rtn = SENSOR_SUCCESS;
    SENSOR_VAL_T *param_ptr = (SENSOR_VAL_T *)param;
    cmr_u16 tmp;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param_ptr);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("E: param_ptr = %p,type=%x", param_ptr, param_ptr->type);
    switch (param_ptr->type) {
    case SENSOR_VAL_TYPE_SHUTTER:
        *((cmr_u32 *)param_ptr->pval) =
            sns_drv_cxt->sensor_ev_info.preview_shutter;
        break;
    case SENSOR_VAL_TYPE_GET_STATIC_INFO:
        rtn = ov5675_drv_get_static_info(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_GET_FPS_INFO:
        rtn = ov5675_drv_get_fps_info(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_SET_SENSOR_CLOSE_FLAG:
        sns_drv_cxt->is_sensor_close = 1;
        break;
    default:
        break;
    }
    SENSOR_LOGV("X:");

    return rtn;
}

static cmr_int
ov5675_drv_handle_create(struct sensor_ic_drv_init_para *init_param,
                         cmr_handle *sns_ic_drv_handle) {
    cmr_int ret = SENSOR_SUCCESS;
    struct sensor_ic_drv_cxt *sns_drv_cxt = NULL;
    void *pri_data = NULL;
    char value[255];

    ret = sensor_ic_drv_create(init_param, sns_ic_drv_handle);
    sns_drv_cxt = *sns_ic_drv_handle;

    /*init hdr infomation*/
    sns_drv_cxt->hdr_info.capture_max_shutter = 2000000000 / SNAPSHOT_LINE_TIME;
    sns_drv_cxt->hdr_info.capture_shutter =
        SNAPSHOT_FRAME_LENGTH - FRAME_OFFSET;
    sns_drv_cxt->hdr_info.capture_gain = SENSOR_BASE_GAIN;

    sns_drv_cxt->sensor_ev_info.preview_shutter =
        PREVIEW_FRAME_LENGTH - FRAME_OFFSET;
    sns_drv_cxt->sensor_ev_info.preview_gain = SENSOR_BASE_GAIN;
    sns_drv_cxt->sensor_ev_info.preview_framelength = PREVIEW_FRAME_LENGTH;

    sns_drv_cxt->frame_length_def = PREVIEW_FRAME_LENGTH;

    sensor_ic_set_match_module_info(sns_drv_cxt, ARRAY_SIZE(MODULE_INFO),
                                    MODULE_INFO);
    property_get("debug.camera.setting", value, "0");
    if (atoi(value) == 1) {
        sensor_ic_set_match_resolution_info(
            sns_drv_cxt, ARRAY_SIZE(RES_TAB_RAW_NEW), RES_TAB_RAW_NEW);
        sensor_ic_set_match_trim_info(sns_drv_cxt, ARRAY_SIZE(RES_TRIM_TAB_NEW),
                                      RES_TRIM_TAB_NEW);
    } else {
        sensor_ic_set_match_resolution_info(
            sns_drv_cxt, ARRAY_SIZE(RES_TAB_RAW), RES_TAB_RAW);
        sensor_ic_set_match_trim_info(sns_drv_cxt, ARRAY_SIZE(RES_TRIM_TAB),
                                      RES_TRIM_TAB);
    }
    sensor_ic_set_match_static_info(sns_drv_cxt, ARRAY_SIZE(STATIC_INFO),
                                    STATIC_INFO);
    sensor_ic_set_match_fps_info(sns_drv_cxt, ARRAY_SIZE(FPS_INFO), FPS_INFO);

    /*init exif info,this will be deleted in the future*/
    ov5675_drv_init_exif_info(sns_drv_cxt, &sns_drv_cxt->exif_ptr);

    /*add private here*/
    return ret;
}

static cmr_int ov5675_drv_handle_delete(cmr_handle handle, void *param) {
    cmr_int ret = SENSOR_SUCCESS;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    ret = sensor_ic_drv_delete(handle, param);
    return ret;
}

static cmr_int ov5675_drv_get_private_data(cmr_handle handle, cmr_uint cmd,
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
static struct sensor_ic_ops s_ov5675_ops_tab = {
    .create_handle = ov5675_drv_handle_create,
    .delete_handle = ov5675_drv_handle_delete,
    .get_data = ov5675_drv_get_private_data,
    /*---------------------------------------*/
    .power = ov5675_drv_power_on,
    .identify = ov5675_drv_identify,
    .ex_write_exp = ov5675_drv_write_exposure,
    .write_gain_value = ov5675_drv_write_gain_value,

    .ext_ops = {
            [SENSOR_IOCTL_EXT_FUNC].ops = ov5675_drv_ext_func,
            [SENSOR_IOCTL_BEFORE_SNAPSHOT].ops = ov5675_drv_before_snapshot,
            [SENSOR_IOCTL_STREAM_ON].ops = ov5675_drv_stream_on,
            [SENSOR_IOCTL_STREAM_OFF].ops = ov5675_drv_stream_off,
            /** expand interface,if you want to add your sub cmd ,
             *  you can add it in enum {@SENSOR_IOCTL_VAL_TYPE}
             */
            [SENSOR_IOCTL_ACCESS_VAL].ops = ov5675_drv_access_val,
    }};
