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
 * V6.0
 */
/*History
*Date                  Modification                                 Reason
*
*/
#include "sensor_s5k3l8xxm3_mipi_raw.h"

#define RES_TAB_RAW   s_s5k3l8xxm3_resolution_tab_raw
#define RES_TRIM_TAB  s_s5k3l8xxm3_resolution_trim_tab
#define STATIC_INFO   s_s5k3l8xxm3_static_info
#define FPS_INFO      s_s5k3l8xxm3_mode_fps_info
#define MIPI_RAW_INFO g_s5k3l8xxm3_mipi_raw_info
#define MODULE_INFO   s_s5k3l8xxm3_module_info_tab

/*==============================================================================
 * Description:
 * write group-hold on to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void s5k3l8xxm3_group_hold_on(cmr_handle handle) {
    SENSOR_LOGI("E");
    SENSOR_IC_CHECK_HANDLE_VOID(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0104, 0x0100);
}

/*==============================================================================
 * Description:
 * write group-hold off to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void s5k3l8xxm3_group_hold_off(cmr_handle handle) {
    SENSOR_LOGI("E");

    SENSOR_IC_CHECK_HANDLE_VOID(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0104, 0x0000);
}

/*==============================================================================
 * Description:
 * read gain from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static cmr_u16 s5k3l8xxm3_read_gain(cmr_handle handle) {
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    return sns_drv_cxt->sensor_ev_info.preview_gain;
}

/*==============================================================================
 * Description:
 * write gain to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void s5k3l8xxm3_write_gain(cmr_handle handle, float gain) {
    float real_gain = gain;
    float a_gain = 0;
    float d_gain = 0;

    SENSOR_IC_CHECK_HANDLE_VOID(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    if ((cmr_u32)real_gain <= 16 * 32) {
        a_gain = real_gain;
        d_gain = 256;
    } else {
        a_gain = 16 * 32;
        d_gain = 256.0 * real_gain / a_gain;
        SENSOR_LOGI("_s5k3l8xxm3: real_gain:0x%x, a_gain: 0x%x, d_gain: 0x%x",
                    (cmr_u32)real_gain, (cmr_u32)a_gain, (cmr_u32)d_gain);
        if ((cmr_u32)d_gain > 256 * 256)
            d_gain = 256 * 256; // d_gain < 256x
    }

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x204, (cmr_u32)a_gain);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x20e, (cmr_u32)d_gain);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x210, (cmr_u32)d_gain);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x212, (cmr_u32)d_gain);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x214, (cmr_u32)d_gain);

    // s5k3l8xxm3_group_hold_off(handle);
}

/*==============================================================================
 * Description:
 * read frame length from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static cmr_u16 s5k3l8xxm3_read_frame_length(cmr_handle handle) {
    cmr_u16 frame_length = 0;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    frame_length = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0340);
    sns_drv_cxt->sensor_ev_info.preview_framelength = frame_length;

    return frame_length;
}

/*==============================================================================
 * Description:
 * write frame length to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void s5k3l8xxm3_write_frame_length(cmr_handle handle,
                                                      cmr_u32 frame_len) {
    SENSOR_IC_CHECK_HANDLE_VOID(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0340, frame_len);
}

/*==============================================================================
 * Description:
 * read shutter from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static cmr_u16 s5k3l8xxm3_read_shutter(cmr_handle handle) {
    cmr_u16 shutter = 0;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    shutter= hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0202);
    sns_drv_cxt->sensor_ev_info.preview_shutter = shutter;

    return shutter;
}

/*==============================================================================
 * Description:
 * write shutter to sensor registers
 * please pay attention to the frame length
 * please modify this function acording your spec
 *============================================================================*/
static void s5k3l8xxm3_write_shutter(cmr_handle handle, cmr_u32 shutter) {
    SENSOR_IC_CHECK_HANDLE_VOID(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0202, shutter);
}

/*==============================================================================
 * Description:
 * write exposure to sensor registers and get current shutter
 * please pay attention to the frame length
 * please don't change this function if it's necessary
 *============================================================================*/
static cmr_u16 s5k3l8xxm3_write_exposure_dummy(cmr_handle handle,
                                                cmr_u32 shutter,
                                                cmr_u32 dummy_line,
                                                cmr_u16 size_index) {
    cmr_u32 dest_fr_len = 0;
    cmr_u32 cur_fr_len = 0;
    cmr_u32 fr_len = 0;

    // s5k3l8xxm3_group_hold_on(handle);
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    fr_len = sns_drv_cxt->frame_length_def;

    if (1 == SUPPORT_AUTO_FRAME_LENGTH)
        goto write_sensor_shutter;

    dummy_line = dummy_line > FRAME_OFFSET ? dummy_line : FRAME_OFFSET;
    dest_fr_len =
        ((shutter + dummy_line) > fr_len) ? (shutter + dummy_line) : fr_len;
    sns_drv_cxt->frame_length = dest_fr_len;

    cur_fr_len = s5k3l8xxm3_read_frame_length(handle);

    if (shutter < SENSOR_MIN_SHUTTER)
        shutter = SENSOR_MIN_SHUTTER;

    if (dest_fr_len != cur_fr_len)
        s5k3l8xxm3_write_frame_length(handle, dest_fr_len);
write_sensor_shutter:
    /* write shutter to sensor registers */
    sns_drv_cxt->sensor_ev_info.preview_shutter = shutter;
    s5k3l8xxm3_write_shutter(handle, shutter);

#ifdef GAIN_DELAY_1_FRAME
    usleep(dest_fr_len * PREVIEW_LINE_TIME / 10);
#endif

    return SENSOR_SUCCESS;
}

/*==============================================================================
 * Description:
 * sensor power on
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int s5k3l8xxm3_power_on(cmr_handle handle, cmr_u32 power_on) {
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
        hw_sensor_set_avdd_val(sns_drv_cxt->hw_handle, avdd_val);
        hw_sensor_set_dvdd_val(sns_drv_cxt->hw_handle, dvdd_val);
        hw_sensor_set_iovdd_val(sns_drv_cxt->hw_handle, iovdd_val);
        hw_sensor_power_down(sns_drv_cxt->hw_handle, !power_down);
        hw_sensor_set_reset_level(sns_drv_cxt->hw_handle,!reset_level);
        usleep(20);
        hw_sensor_set_mclk(sns_drv_cxt->hw_handle, EX_MCLK);
        hw_sensor_set_mipi_level(sns_drv_cxt->hw_handle, 0);
    } else {
        hw_sensor_set_mipi_level(sns_drv_cxt->hw_handle,1);
        hw_sensor_set_mclk(sns_drv_cxt->hw_handle, SENSOR_DISABLE_MCLK);
        hw_sensor_set_reset_level(sns_drv_cxt->hw_handle,reset_level);
        hw_sensor_power_down(sns_drv_cxt->hw_handle,power_down);
        hw_sensor_set_dvdd_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED);
        hw_sensor_set_avdd_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED);
        hw_sensor_set_iovdd_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED);
    }
    SENSOR_LOGI("(1:on, 0:off): %d", power_on);
    return SENSOR_SUCCESS;
}

/*==============================================================================
 * Description:
 * calculate fps for every sensor mode according to frame_line and line_time
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int s5k3l8xxm3_init_fps_info(cmr_handle handle) {
    cmr_int rtn = SENSOR_SUCCESS;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("E");
    struct sensor_fps_info *fps_info = sns_drv_cxt->fps_info;
    struct sensor_trim_tag *trim_info = sns_drv_cxt->trim_tab_info;
    struct sensor_static_info *static_info = sns_drv_cxt->static_info;

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
                if (fps_info->sensor_mode_fps[i].max_fps > static_info->max_fps) {
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
                fps_info->sensor_mode_fps[i]
                    .high_fps_skip_num);
        }
        fps_info->is_init = 1;
    }
    SENSOR_LOGI("X");
    return rtn;
}

static const struct pd_pos_info s5k3l8xxm3_pd_pos_r[] = {
    {28, 35}, {44, 39}, {64, 39}, {80, 35}, {32, 47}, {48, 51},
    {60, 51}, {76, 47}, {32, 71}, {48, 67}, {60, 67}, {76, 71},
    {28, 83}, {44, 79}, {64, 79}, {80, 83},
};

static const struct pd_pos_info s5k3l8xxm3_pd_pos_l[] = {
    {28, 31}, {44, 35}, {64, 35}, {80, 31}, {32, 51}, {48, 55},
    {60, 55}, {76, 51}, {32, 67}, {48, 63}, {60, 63}, {76, 67},
    {28, 87}, {44, 83}, {64, 83}, {80, 87},
};

static cmr_int s5k3l8xxm3_get_pdaf_info(cmr_handle handle,
                                         cmr_u32 *param) {
    cmr_int rtn = SENSOR_SUCCESS;
    struct sensor_pdaf_info *pdaf_info = NULL;
    cmr_u16 pd_pos_r_size = 0;
    cmr_u16 pd_pos_l_size = 0;

    /*TODO*/
    SENSOR_IC_CHECK_PTR(param);
    pdaf_info = (struct sensor_pdaf_info *)param;
    pd_pos_r_size = NUMBER_OF_ARRAY(s5k3l8xxm3_pd_pos_r);
    pd_pos_l_size = NUMBER_OF_ARRAY(s5k3l8xxm3_pd_pos_l);
    if (pd_pos_r_size != pd_pos_l_size) {
        SENSOR_LOGE(
            "s5k3l8xxm3_pd_pos_r size not match s5k3l8xxm3_pd_pos_l");
        return -1;
    }

    pdaf_info->pd_offset_x = 24;
    pdaf_info->pd_offset_y = 24;
    pdaf_info->pd_pitch_x = 64;
    pdaf_info->pd_pitch_y = 64;
    pdaf_info->pd_density_x = 16;
    pdaf_info->pd_density_y = 16;
    pdaf_info->pd_block_num_x = 65;
    pdaf_info->pd_block_num_y = 48;
    pdaf_info->pd_pos_size = pd_pos_r_size;
    pdaf_info->pd_pos_r = (struct pd_pos_info *)s5k3l8xxm3_pd_pos_r;
    pdaf_info->pd_pos_l = (struct pd_pos_info *)s5k3l8xxm3_pd_pos_l;

    return rtn;
}

static cmr_int s5k3l8xxm3_get_static_info(cmr_handle handle,
                                           cmr_u32 *param) {
    cmr_int rtn = SENSOR_SUCCESS;
    struct sensor_ex_info *ex_info = (struct sensor_ex_info *)param;;
    cmr_u32 up = 0;
    cmr_u32 down = 0;
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
        s5k3l8xxm3_init_fps_info(handle);
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
    sensor_ic_print_static_info("s_s5k3l8xxm3",ex_info);

    return rtn;
}

static cmr_int s5k3l8xxm3_get_fps_info(cmr_handle handle, cmr_u32 *param) {
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_MODE_FPS_T *fps_info = (SENSOR_MODE_FPS_T *)param;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(fps_info);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    struct sensor_fps_info *fps_data = sns_drv_cxt->fps_info;

    // make sure have inited fps of every sensor mode.
    if (!fps_data->is_init) {
        s5k3l8xxm3_init_fps_info(handle);
    }

    cmr_u32 sensor_mode = fps_info->mode;
    fps_info->max_fps = fps_data->sensor_mode_fps[sensor_mode].max_fps;
    fps_info->min_fps = fps_data->sensor_mode_fps[sensor_mode].min_fps;
    fps_info->is_high_fps = fps_data->sensor_mode_fps[sensor_mode].is_high_fps;
    fps_info->high_fps_skip_num = fps_data->sensor_mode_fps[sensor_mode]
            .high_fps_skip_num;
    SENSOR_LOGI("mode %d, max_fps: %d", fps_info->mode, fps_info->max_fps);
    SENSOR_LOGI("min_fps: %d", fps_info->min_fps);
    SENSOR_LOGI("is_high_fps: %d", fps_info->is_high_fps);
    SENSOR_LOGI("high_fps_skip_num: %d", fps_info->high_fps_skip_num);

    return ret;
}

static cmr_int s5k3l8xxm3_set_sensor_close_flag(cmr_handle handle) {
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    sns_drv_cxt->is_sensor_close = 1;

    return ret;
}
/*==============================================================================
 * Description:
 * cfg otp setting
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int s5k3l8xxm3_access_val(cmr_handle handle, cmr_int param) {
    cmr_int ret = SENSOR_FAIL;
    SENSOR_VAL_T *param_ptr = (SENSOR_VAL_T *)param;

    if (!param_ptr) {
#ifdef FEATURE_OTP
        if (PNULL != s_s5k3l8xxm3_raw_param_tab_ptr->cfg_otp) {
            ret = s_s5k3l8xxm3_raw_param_tab_ptr->cfg_otp(
                handle, s_s5k3l8xxm3_otp_info_ptr);
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

    SENSOR_LOGI("sensor s5k3l8xxm3: param_ptr->type=%x", param_ptr->type);

    switch (param_ptr->type) {
    case SENSOR_VAL_TYPE_INIT_OTP:
        ret = s5k3l8xxm3_otp_init(handle);
        break;
    case SENSOR_VAL_TYPE_READ_OTP:
        ret = s5k3l8xxm3_otp_read(handle, param_ptr);
        break;
    case SENSOR_VAL_TYPE_PARSE_OTP:
        ret = s5k3l8xxm3_parse_otp(handle, param_ptr);
        if (ret != 0)
            ret = s5k3l8xxm3_otp_read(handle, param_ptr);
        break;
    case SENSOR_VAL_TYPE_WRITE_OTP:
        // rtn = _s5k3l8xxm3_write_otp(handle, (cmr_u32)param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_SHUTTER:
        *((cmr_u32 *)param_ptr->pval) = s5k3l8xxm3_read_shutter(handle);
        break;
    case SENSOR_VAL_TYPE_READ_OTP_GAIN:
        *((cmr_u32 *)param_ptr->pval) = s5k3l8xxm3_read_gain(handle);
        break;
    case SENSOR_VAL_TYPE_GET_STATIC_INFO:
        ret = s5k3l8xxm3_get_static_info(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_GET_FPS_INFO:
        ret = s5k3l8xxm3_get_fps_info(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_GET_PDAF_INFO:
        ret = s5k3l8xxm3_get_pdaf_info(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_SET_SENSOR_CLOSE_FLAG:
        ret = s5k3l8xxm3_set_sensor_close_flag(handle);
        break;
    default:
        break;
    }
    ret = SENSOR_SUCCESS;

    return ret;
}

/*==============================================================================
 * Description:
 * identify sensor id
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int s5k3l8xxm3_identify(cmr_handle handle, cmr_u32 param) {
    cmr_u16 pid_value = 0x00;
    cmr_u16 ver_value = 0x00;
    cmr_int ret_value = SENSOR_FAIL;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("mipi raw identify");

    pid_value = hw_sensor_read_reg(sns_drv_cxt->hw_handle, s5k3l8xxm3_PID_ADDR);

    if (s5k3l8xxm3_PID_VALUE == pid_value) {
        ver_value = hw_sensor_read_reg(sns_drv_cxt->hw_handle, s5k3l8xxm3_VER_ADDR);
        SENSOR_LOGI("Identify: PID = %x, VER = %x", pid_value, ver_value);
        if (s5k3l8xxm3_VER_VALUE == ver_value) {
            SENSOR_LOGI("this is s5k3l8xxm3 sensor");
#ifdef FEATURE_OTP
            /*if read otp info failed or module id mismatched ,identify failed
             * ,return SENSOR_FAIL ,exit identify*/
            if (PNULL != s_s5k3l8xxm3_raw_param_tab_ptr->identify_otp) {
                SENSOR_LOGI("identify module_id=0x%x",
                             s_s5k3l8xxm3_raw_param_tab_ptr->param_id);
                // set default value
                memset(s_s5k3l8xxm3_otp_info_ptr, 0x00,
                       sizeof(struct otp_info_t));
                ret_value = s_s5k3l8xxm3_raw_param_tab_ptr->identify_otp(
                    handle, s_s5k3l8xxm3_otp_info_ptr);
                if (SENSOR_SUCCESS == ret_value) {
                    SENSOR_LOGI(
                        "identify otp sucess! module_id=0x%x, module_name=%s",
                        s_s5k3l8xxm3_raw_param_tab_ptr->param_id, MODULE_NAME);
                } else {
                    SENSOR_LOGI("identify otp fail! exit identify");
                    return ret_value;
                }
            } else {
                SENSOR_LOGI("no identify_otp function!");
            }

#endif
            s5k3l8xxm3_init_fps_info(handle);
            ret_value = SENSOR_SUCCESS;

        } else {
            SENSOR_LOGI("Identify this is %x%x sensor", pid_value,
                              ver_value);
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
static cmr_int s5k3l8xxm3_before_snapshot(cmr_handle handle,
                                           cmr_u32 param) {
    cmr_u32 cap_shutter = 0;
    cmr_u32 prv_shutter = 0;
    cmr_u32 gain = 0;
    cmr_u32 cap_gain = 0;
    cmr_u32 capture_mode = param & 0xffff;
    cmr_u32 preview_mode = (param >> 0x10) & 0xffff;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;


    cmr_u32 prv_linetime = sns_drv_cxt->trim_tab_info[preview_mode].line_time;
    cmr_u32 cap_linetime = sns_drv_cxt->trim_tab_info[capture_mode].line_time;

    sns_drv_cxt->frame_length_def = sns_drv_cxt->trim_tab_info[capture_mode].frame_line;
    SENSOR_LOGI("capture_mode = %d", capture_mode);

    if (preview_mode == capture_mode) {
        cap_shutter = sns_drv_cxt->sensor_ev_info.preview_shutter;
        cap_gain = sns_drv_cxt->sensor_ev_info.preview_gain;
        goto snapshot_info;
    }

    prv_shutter = sns_drv_cxt->sensor_ev_info.preview_shutter;
    gain = sns_drv_cxt->sensor_ev_info.preview_gain;
    if(sns_drv_cxt->ops_cb.set_mode)
        sns_drv_cxt->ops_cb.set_mode(sns_drv_cxt->caller_handle, capture_mode);
    if(sns_drv_cxt->ops_cb.set_mode_wait_done) 
        sns_drv_cxt->ops_cb.set_mode_wait_done(sns_drv_cxt->caller_handle);

    cap_shutter = prv_shutter * prv_linetime / cap_linetime;

    cap_shutter = s5k3l8xxm3_write_exposure_dummy(handle, cap_shutter, 0, 0);
    cap_gain = gain;
    s5k3l8xxm3_write_gain(handle, cap_gain);
    SENSOR_LOGI("preview_shutter = 0x%x, preview_gain = 0x%x",
                 sns_drv_cxt->sensor_ev_info.preview_shutter,
                 sns_drv_cxt->sensor_ev_info.preview_gain);

    SENSOR_LOGI("capture_shutter = 0x%x, capture_gain = 0x%x", cap_shutter,
                 cap_gain);
snapshot_info:
    if(sns_drv_cxt->ops_cb.set_exif_info)
        sns_drv_cxt->ops_cb.set_exif_info(sns_drv_cxt->caller_handle,
                          SENSOR_EXIF_CTRL_EXPOSURETIME, cap_shutter);

    return SENSOR_SUCCESS;
}

/*==============================================================================
 * Description:
 * get the shutter from isp
 * please don't change this function unless it's necessary
 *============================================================================*/
static cmr_int s5k3l8xxm3_write_exposure(cmr_handle handle,
                                               cmr_uint param) {
    cmr_int ret = SENSOR_SUCCESS;
    cmr_u16 exposure_line = 0x00;
    cmr_u16 dummy_line = 0x00;
    cmr_u16 size_index = 0x00;
    struct sensor_ex_exposure *ex = (struct sensor_ex_exposure *)param;

    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    exposure_line = ex->exposure;
    dummy_line = ex->dummy;
    size_index = ex->size_index;

    SENSOR_LOGI("size_index=%d, exposure_line = %d, dummy_line=%d", size_index,
                 exposure_line, dummy_line);
    sns_drv_cxt->frame_length_def = sns_drv_cxt->trim_tab_info[size_index].frame_line;
    sns_drv_cxt->line_time_def = sns_drv_cxt->trim_tab_info[size_index].line_time;

    ret = s5k3l8xxm3_write_exposure_dummy(handle, exposure_line,
                                                dummy_line, size_index);

    return ret;
}

/*==============================================================================
 * Description:
 * get the parameter from isp to real gain
 * you mustn't change the funcion !
 *============================================================================*/
static cmr_int isp_to_real_gain(cmr_handle handle, cmr_u32 param) {
    cmr_u32 real_gain = 0;

    real_gain = param;

    return real_gain;
}

/*==============================================================================
 * Description:
 * write gain value to sensor
 * you can change this function if it's necessary
 *============================================================================*/
static cmr_int s5k3l8xxm3_write_gain_value(cmr_handle handle,
                                            cmr_uint param) {
    cmr_int ret = SENSOR_SUCCESS;
    float real_gain = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    param = param < SENSOR_BASE_GAIN ? SENSOR_BASE_GAIN : param;

    real_gain = (float)param * SENSOR_BASE_GAIN / ISP_BASE_GAIN * 1.0;

    SENSOR_LOGI("real_gain = %f", real_gain);

    sns_drv_cxt->sensor_ev_info.preview_gain = real_gain;
    s5k3l8xxm3_write_gain(handle, real_gain);

    return ret;
}

static struct sensor_reg_tag s5k3l8xxm3_shutter_reg[] = {
    {0x0202, 0},
};

static struct sensor_i2c_reg_tab s5k3l8xxm3_shutter_tab = {
    .settings = s5k3l8xxm3_shutter_reg,
    .size = ARRAY_SIZE(s5k3l8xxm3_shutter_reg),
};

static struct sensor_reg_tag s5k3l8xxm3_again_reg[] = {
    {0x0204, 0},
};

static struct sensor_i2c_reg_tab s5k3l8xxm3_again_tab = {
    .settings = s5k3l8xxm3_again_reg, .size = ARRAY_SIZE(s5k3l8xxm3_again_reg),
};

static struct sensor_reg_tag s5k3l8xxm3_dgain_reg[] = {
    {0x020e, 0}, {0x0210, 0}, {0x0212, 0}, {0x0214, 0},
};

struct sensor_i2c_reg_tab s5k3l8xxm3_dgain_tab = {
    .settings = s5k3l8xxm3_dgain_reg, .size = ARRAY_SIZE(s5k3l8xxm3_dgain_reg),
};

static struct sensor_reg_tag s5k3l8xxm3_frame_length_reg[] = {
    {0x0340, 0},
};

static struct sensor_i2c_reg_tab s5k3l8xxm3_frame_length_tab = {
    .settings = s5k3l8xxm3_frame_length_reg,
    .size = ARRAY_SIZE(s5k3l8xxm3_frame_length_reg),
};

static struct sensor_aec_i2c_tag s5k3l8xxm3_aec_info = {
    .slave_addr = (I2C_SLAVE_ADDR >> 1),
    .addr_bits_type = SENSOR_I2C_REG_16BIT,
    .data_bits_type = SENSOR_I2C_VAL_16BIT,
    .shutter = &s5k3l8xxm3_shutter_tab,
    .again = &s5k3l8xxm3_again_tab,
    .dgain = &s5k3l8xxm3_dgain_tab,
    .frame_length = &s5k3l8xxm3_frame_length_tab,
};

static cmr_u16 s5k3l8xxm3_calc_exposure(cmr_handle handle,
                                         cmr_u32 shutter, cmr_u32 dummy_line,
                                         cmr_u16 mode,
                                         struct sensor_aec_i2c_tag *aec_info) {
    cmr_u32 dest_fr_len = 0;
    cmr_u32 cur_fr_len = 0;
    cmr_u32 fr_len = 0;
    cmr_int offset = 0;
    float fps = 0.0;
    float line_time = 0.0;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    fr_len = sns_drv_cxt->frame_length_def;

    dummy_line = dummy_line > FRAME_OFFSET ? dummy_line : FRAME_OFFSET;
    dest_fr_len =
        ((shutter + dummy_line) > fr_len) ? (shutter + dummy_line) : fr_len;
    sns_drv_cxt->frame_length = dest_fr_len;

    cur_fr_len = s5k3l8xxm3_read_frame_length(handle);

    if (shutter < SENSOR_MIN_SHUTTER)
        shutter = SENSOR_MIN_SHUTTER;
    line_time = sns_drv_cxt->trim_tab_info[mode].line_time;
    if (cur_fr_len > shutter) {
        fps = 1000000.0 / (cur_fr_len * line_time);
    } else {
        fps = 1000000.0 / ((shutter + dummy_line) * line_time);
    }
    SENSOR_LOGI("sync fps = %f", fps);

    aec_info->frame_length->settings[0].reg_value = dest_fr_len;
    aec_info->shutter->settings[0].reg_value = shutter;
    return shutter;
}

static void s5k3l8xxm3_calc_gain(float gain,
                                 struct sensor_aec_i2c_tag *aec_info) {
    cmr_u32 ret_value = SENSOR_SUCCESS;
    cmr_u16 value = 0x00;
    float real_gain = gain;
    float a_gain = 0;
    float d_gain = 0;
    uint8_t i = 0;

    if ((cmr_u32)real_gain <= 16 * 32) {
        a_gain = real_gain;
        d_gain = 256;
    } else {
        a_gain = 16 * 32;
        d_gain = 256.0 * real_gain / a_gain;
        SENSOR_LOGI("_s5k3l8xxm3: real_gain:0x%x, a_gain: 0x%x, d_gain: 0x%x",
                    (cmr_u32)real_gain, (cmr_u32)a_gain, (cmr_u32)d_gain);
        if ((cmr_u32)d_gain > 256 * 256)
            d_gain = 256 * 256; // d_gain < 256x
    }

    aec_info->again->settings[0].reg_value = (cmr_u16)a_gain;
    for (i = 0; i < aec_info->dgain->size; i++)
        aec_info->dgain->settings[i].reg_value = (cmr_u16)d_gain;
}

static cmr_int s5k3l8xxm3_read_aec_info(cmr_handle handle, void *param) {
    cmr_int ret = SENSOR_SUCCESS;
    struct sensor_aec_reg_info *info = (struct sensor_aec_reg_info *)param;
    cmr_u16 exposure_line = 0x00;
    cmr_u16 dummy_line = 0x00;
    cmr_u16 mode = 0x00;
    float real_gain = 0;
    cmr_u32 gain = 0;
    cmr_u16 frame_interval = 0x00;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    struct sensor_trim_tag *trim_info = sns_drv_cxt->trim_tab_info;

    info->aec_i2c_info_out = &s5k3l8xxm3_aec_info;

    exposure_line = info->exp.exposure;
    dummy_line = info->exp.dummy;
    mode = info->exp.size_index;

    frame_interval = (cmr_u16)(((exposure_line + dummy_line) *
                     (trim_info[mode].line_time)) / 1000000);
    SENSOR_LOGI(
        "mode = %d, exposure_line = %d, dummy_line= %d, frame_interval= %d ms",
        mode, exposure_line, dummy_line, frame_interval);

    sns_drv_cxt->frame_length_def = trim_info[mode].frame_line;
    sns_drv_cxt->line_time_def = trim_info[mode].line_time;

    sns_drv_cxt->sensor_ev_info.preview_shutter = s5k3l8xxm3_calc_exposure(
        handle, exposure_line, dummy_line, mode, &s5k3l8xxm3_aec_info);

    gain = info->gain < SENSOR_BASE_GAIN ? SENSOR_BASE_GAIN : info->gain;
    real_gain = (float)info->gain * SENSOR_BASE_GAIN / ISP_BASE_GAIN * 1.0;
    s5k3l8xxm3_calc_gain(real_gain, &s5k3l8xxm3_aec_info);
    return ret;
}

/*==============================================================================
 * Description:
 * write parameter to frame sync
 * please add your frame sync function to this function
 *============================================================================*/

static cmr_int _s5k3l8xxm3_SetMaster_FrameSync(cmr_handle handle,
                                                     cmr_u32 param) {
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x30c2, 0x0100);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x30c4, 0x0100);
    return 0;
}

/*==============================================================================
 * Description:
 * mipi stream on
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int s5k3l8xxm3_stream_on(cmr_handle handle, cmr_u32 param) {
    SENSOR_LOGI("E");
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0100, 0x0100);

    return 0;
}

/*==============================================================================
 * Description:
 * mipi stream off
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int s5k3l8xxm3_stream_off(cmr_handle handle, cmr_u32 param) {
    SENSOR_LOGI("E");
    unsigned char value;
    unsigned int sleep_time = 0, frame_time;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    value = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0100);

    if (value == 0x01) {
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0100, 0x00);
        if (!sns_drv_cxt->is_sensor_close) {
            usleep(50 * 1000);
        }
    } else {
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0100, 0x00);
    }

    sns_drv_cxt->is_sensor_close = 0;
    SENSOR_LOGI("X sleep_time=%dus", sleep_time);
    return 0;
}

static cmr_int s5k3l8xxm3_drv_handle_create(
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

    sns_drv_cxt->frame_length_def = PREVIEW_FRAME_LENGTH;
    sns_drv_cxt->sensor_ev_info.preview_shutter = PREVIEW_FRAME_LENGTH - FRAME_OFFSET;
    sns_drv_cxt->sensor_ev_info.preview_gain = SENSOR_BASE_GAIN;
    sns_drv_cxt->sensor_ev_info.preview_framelength = PREVIEW_FRAME_LENGTH;

    /*add private here*/
    return ret;
}

static cmr_int s5k3l8xxm3_drv_handle_delete(cmr_handle handle, 
                                                        cmr_u32 *param) {
    cmr_int ret = SENSOR_SUCCESS;
    /*if has private data,you must release it here*/

    ret = sensor_ic_drv_delete(handle,param);
    return ret;
}

static cmr_int s5k3l8xxm3_drv_get_private_data(cmr_handle handle,
                                                     cmr_uint cmd, void**param){
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param);

    ret = sensor_ic_get_private_data(handle,cmd, param);
    return ret;
}

/*==============================================================================
 * Description:
 * all ioctl functoins
 * you can add functions reference SENSOR_IOCTL_FUNC_TAB_T from sensor_drv_u.h
 *
 * add ioctl functions like this:
 * .power = s5k3l8xxm3_power_on,
 *============================================================================*/
static struct sensor_ic_ops s_s5k3l8xxm3_ops_tab = {
    .create_handle = s5k3l8xxm3_drv_handle_create,
    .delete_handle = s5k3l8xxm3_drv_handle_delete,
    .get_data = s5k3l8xxm3_drv_get_private_data,
    .power  = s5k3l8xxm3_power_on,
    .identify = s5k3l8xxm3_identify,

    .write_gain_value = s5k3l8xxm3_write_gain_value,
    .ex_write_exp = s5k3l8xxm3_write_exposure,
    .read_aec_info = s5k3l8xxm3_read_aec_info,
    .ext_ops = {
        [SENSOR_IOCTL_BEFORE_SNAPSHOT].ops = s5k3l8xxm3_before_snapshot,
        [SENSOR_IOCTL_STREAM_ON].ops = s5k3l8xxm3_stream_on,
        [SENSOR_IOCTL_STREAM_OFF].ops = s5k3l8xxm3_stream_off,
        [SENSOR_IOCTL_ACCESS_VAL].ops = s5k3l8xxm3_access_val,
    }
};

