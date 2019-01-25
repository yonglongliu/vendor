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
#include "sensor_s5k3p8sm_mipi_raw.h"

#define s5k3p8sm_i2c_read_otp(addr) s5k3p8sm_i2c_read_otp_set(handle, addr)

#define LOG_TAG "s5k3p8sm_drv_mipi_raw"

#define RES_TAB_RAW   s_s5k3p8sm_resolution_Tab_RAW
#define RES_TRIM_TAB  s_s5k3p8sm_Resolution_Trim_Tab
#define STATIC_INFO   s_s5k3p8sm_static_info
#define FPS_INFO      s_s5k3p8sm_mode_fps_info
#define MIPI_RAW_INFO g_s5k3p8sm_mipi_raw_info
#define MODULE_INFO   s_s5k3p8sm_module_info_tab

static cmr_int s5k3p8sm_drv_get_raw_info(cmr_handle handle);

static cmr_int s5k3p8sm_drv_init_fps_info(cmr_handle handle) {
    cmr_int rtn = SENSOR_SUCCESS;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("E");
    struct sensor_fps_info *fps_info = sns_drv_cxt->fps_info;
    struct sensor_trim_tag *trim_info = sns_drv_cxt->trim_tab_info;
    struct sensor_static_info *static_info = sns_drv_cxt->static_info;

    if (!fps_info->is_init) {
        uint32_t i, modn, tempfps = 0;
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
                      tempfps, trim_info[i].frame_line, trim_info[i].line_time);
            SENSOR_LOGI("mode %d,max_fps: %d ", i, fps_info->sensor_mode_fps[i].max_fps);
            SENSOR_LOGI("is_high_fps: %d,highfps_skip_num %d",
                fps_info->sensor_mode_fps[i].is_high_fps,
                fps_info->sensor_mode_fps[i].high_fps_skip_num);
        }
        fps_info->is_init = 1;
    }
    SENSOR_LOGI("X");
    return rtn;
}

static cmr_int s5k3p8sm_drv_identify(cmr_handle handle, cmr_int param) {
#define S5K3P8SM_PID_VALUE 0x3108
#define S5K3P8SM_PID_ADDR 0x0000
#define S5K3P8SM_VER_VALUE 0xb001
#define S5K3P8SM_VER_ADDR 0x0002
    cmr_u16 pid_value = 0x00;
    cmr_u16 ver_value = 0x00;
    cmr_int ret_value = SENSOR_FAIL;
    cmr_u16 i = 0;
    cmr_u16 ret;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("identify E:\n");

    pid_value = hw_sensor_read_reg(sns_drv_cxt->hw_handle, S5K3P8SM_PID_ADDR);
    ver_value = hw_sensor_read_reg(sns_drv_cxt->hw_handle, S5K3P8SM_VER_ADDR);

    if (S5K3P8SM_PID_VALUE == pid_value) {
        SENSOR_LOGI("PID = %x, VER = %x", pid_value, ver_value);
        if (S5K3P8SM_VER_VALUE == ver_value) {
            SENSOR_LOGI("this is S5K3P8SM sensor !");
            ret_value = s5k3p8sm_drv_get_raw_info(handle);
            if (SENSOR_SUCCESS != ret_value) {
                SENSOR_LOGE("SENSOR_S5K3P8SM: the module is unknow error !");
            }
#if defined(CONFIG_CAMERA_OIS_FUNC)
            for (i = 0; i < 3; i++) {
                ret = ois_pre_open(handle);
                if (ret == 0)
                    break;
                usleep(20 * 1000);
            }
            if (ret == 0)
                OpenOIS(handle);
#endif
            s5k3p8sm_drv_init_fps_info(handle);
        } else {
            SENSOR_LOGE(
                "SENSOR_S5K3P8SM: Identify this is hm%x%x sensor !", pid_value,
                ver_value);
            return ret_value;
        }
    } else {
        SENSOR_LOGE(
            "SENSOR_S5K3P8SM: identify fail,pid_value=%d, ver_value = %d",
            pid_value, ver_value);
    }

    return ret_value;
}

#if 0
static cmr_u32 s5k3p8sm_drv_GetMaxFrameLine(cmr_u32 index) {
    uint32_t max_line = 0x00;
    SENSOR_TRIM_T_PTR trim_ptr = RES_TRIM_TAB;

    max_line = trim_ptr[index].frame_line;

    return max_line;
}
#endif
static cmr_int s5k3p8sm_drv_write_exp_dummy(cmr_handle handle, 
                 cmr_u16 expsure_line, cmr_u16 dummy_line, cmr_u16 size_index)
{
    cmr_int ret_value = SENSOR_SUCCESS;
    cmr_u32 frame_len_cur = 0x00;
    cmr_u32 frame_len = 0x00;
    cmr_u32 max_frame_len = 0x00;
    cmr_u32 linetime = 0;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_PRINT("exposure_line:%d, dummy:%d, size_index:%d",
                         expsure_line, dummy_line, size_index);
    max_frame_len = sns_drv_cxt->trim_tab_info[size_index].frame_line;
    if (expsure_line < 3) {
        expsure_line = 3;
    }

    frame_len = expsure_line + dummy_line;
    frame_len = (frame_len > (uint32_t)(expsure_line + 5))
                    ? frame_len
                    : (uint32_t)(expsure_line + 5);
    frame_len = (frame_len > max_frame_len) ? frame_len : max_frame_len;
    if (0x00 != (0x01 & frame_len)) {
        frame_len += 0x01;
    }

    frame_len_cur = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0340);

    if (frame_len_cur != frame_len) {
        ret_value = hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0340, frame_len);
    }

    ret_value = hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x202, expsure_line);

    sns_drv_cxt->exp_line = expsure_line;
    linetime = sns_drv_cxt->trim_tab_info[size_index].line_time;
    if(sns_drv_cxt->ops_cb.set_exif_info) {
        sns_drv_cxt->ops_cb.set_exif_info(sns_drv_cxt->caller_handle,
                                  SENSOR_EXIF_CTRL_EXPOSURETIME, expsure_line);
    } else {
        sns_drv_cxt->exif_info.exposure_time = expsure_line;
        sns_drv_cxt->exif_info.exposure_time = expsure_line * linetime / 1000;
    }

    return ret_value;
}

static cmr_int s5k3p8sm_drv_write_exposure(cmr_handle handle, cmr_int param) {
    cmr_int ret_value = SENSOR_SUCCESS;
    cmr_u32 expsure_line = 0x00;
    cmr_u32 dummy_line = 0x00;
    cmr_u32 size_index = 0x00;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    expsure_line = param & 0xffff;
    dummy_line = (param >> 0x10) & 0x0fff;
    size_index = (param >> 0x1c) & 0x0f;

    SENSOR_PRINT("line:%d, dummy:%d, size_index:%d",
                   expsure_line, dummy_line, size_index);

    ret_value =
        s5k3p8sm_drv_write_exp_dummy(handle, expsure_line, dummy_line, size_index);

    sns_drv_cxt->sensor_ev_info.preview_shutter = sns_drv_cxt->exp_line;

    return ret_value;
}

#define OTP_LSC_INFO_LEN 1658

static cmr_u8 s5k3p3_opt_lsc_data[OTP_LSC_INFO_LEN];
static struct sensor_otp_cust_info s5k3p3_otp_info;

static cmr_u8 s5k3p8sm_i2c_read_otp_set(cmr_handle handle, cmr_u16 addr) {
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    return hw_sensor_grc_read_i2c(sns_drv_cxt->hw_handle, 
                                  0xA0 >> 1, addr, BITS_ADDR16_REG8);
}

static int s5k3p8sm_otp_init(cmr_handle handle) { return 0; }

static int s5k3p8sm_otp_read_data(cmr_handle handle) {
    cmr_u16 i = 0;
    cmr_u8 high_val = 0;
    cmr_u8 low_val = 0;
    cmr_u32 checksum = 0;
    struct sensor_single_otp_info *single_otp = NULL;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    single_otp = &s5k3p3_otp_info.single_otp;
    single_otp->program_flag = s5k3p8sm_i2c_read_otp(0x0000);
    SENSOR_LOGI("program_flag = %d", single_otp->program_flag);
    if (1 != single_otp->program_flag) {
        SENSOR_LOGE("failed to read otp or the otp is wrong data");
        return -1;
    }
    checksum += single_otp->program_flag;
    single_otp->module_info.year = s5k3p8sm_i2c_read_otp(0x0001);
    checksum += single_otp->module_info.year;
    single_otp->module_info.month = s5k3p8sm_i2c_read_otp(0x0002);
    checksum += single_otp->module_info.month;
    single_otp->module_info.day = s5k3p8sm_i2c_read_otp(0x0003);
    checksum += single_otp->module_info.day;
    single_otp->module_info.mid = s5k3p8sm_i2c_read_otp(0x0004);
    checksum += single_otp->module_info.mid;
    single_otp->module_info.lens_id = s5k3p8sm_i2c_read_otp(0x0005);
    checksum += single_otp->module_info.lens_id;
    single_otp->module_info.vcm_id = s5k3p8sm_i2c_read_otp(0x0006);
    checksum += single_otp->module_info.vcm_id;
    single_otp->module_info.driver_ic_id = s5k3p8sm_i2c_read_otp(0x0007);
    checksum += single_otp->module_info.driver_ic_id;

    high_val = s5k3p8sm_i2c_read_otp(0x0010);
    checksum += high_val;
    low_val = s5k3p8sm_i2c_read_otp(0x0011);
    checksum += low_val;
    single_otp->iso_awb_info.iso = (high_val << 8 | low_val);
    high_val = s5k3p8sm_i2c_read_otp(0x0012);
    checksum += high_val;
    low_val = s5k3p8sm_i2c_read_otp(0x0013);
    checksum += low_val;
    single_otp->iso_awb_info.gain_r = (high_val << 8 | low_val);
    high_val = s5k3p8sm_i2c_read_otp(0x0014);
    checksum += high_val;
    low_val = s5k3p8sm_i2c_read_otp(0x0015);
    checksum += low_val;
    single_otp->iso_awb_info.gain_g = (high_val << 8 | low_val);
    high_val = s5k3p8sm_i2c_read_otp(0x0016);
    checksum += high_val;
    low_val = s5k3p8sm_i2c_read_otp(0x0017);
    checksum += low_val;
    single_otp->iso_awb_info.gain_b = (high_val << 8 | low_val);

    for (i = 0; i < OTP_LSC_INFO_LEN; i++) {
        s5k3p3_opt_lsc_data[i] = s5k3p8sm_i2c_read_otp(0x0020 + i);
        checksum += s5k3p3_opt_lsc_data[i];
    }

    single_otp->lsc_info.lsc_data_addr = s5k3p3_opt_lsc_data;
    single_otp->lsc_info.lsc_data_size = sizeof(s5k3p3_opt_lsc_data);

    single_otp->af_info.flag = s5k3p8sm_i2c_read_otp(0x06A0);
    if (0 == single_otp->af_info.flag)
        SENSOR_LOGE("af otp is wrong");

    checksum = 0; //+= s5k3p3_otp_info.af_info.flag;
    /* cause checksum, skip af flag */
    low_val = s5k3p8sm_i2c_read_otp(0x06A1);
    checksum += low_val;
    high_val = s5k3p8sm_i2c_read_otp(0x06A2);
    checksum += high_val;
    single_otp->af_info.infinite_cali = (high_val << 8 | low_val);
    low_val = s5k3p8sm_i2c_read_otp(0x06A3);
    checksum += low_val;
    high_val = s5k3p8sm_i2c_read_otp(0x06A4);
    checksum += high_val;
    single_otp->af_info.macro_cali = (high_val << 8 | low_val);

    single_otp->checksum = s5k3p8sm_i2c_read_otp(0x06A5);
    SENSOR_LOGI("checksum = 0x%x s5k3p3_otp_info.checksum = 0x%x", checksum,
                 single_otp->checksum);

    if ((checksum % 0xff) != single_otp->checksum) {
        SENSOR_LOGE("checksum error!");
        single_otp->program_flag = 0;
        return -1;
    }

    return 0;
}

static cmr_int s5k3p8sm_otp_read(cmr_handle handle,
                                       SENSOR_VAL_T *param) {
    struct sensor_otp_cust_info *otp_info = NULL;
    SENSOR_LOGI("E");
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param);

    s5k3p8sm_otp_read_data(handle);
    otp_info = &s5k3p3_otp_info;

    if (1 != otp_info->single_otp.program_flag) {
        SENSOR_LOGE("otp error");
        param->pval = NULL;
        return -1;
    } else {
        param->pval = (void *)otp_info;
    }
    SENSOR_LOGI("param->pval = %p", param->pval);

    return 0;
}

static cmr_int s5k3p8sm_parse_otp(cmr_handle handle,
                                        SENSOR_VAL_T *param) {
    cmr_u8 *buff = NULL;
    struct sensor_single_otp_info *single_otp = NULL;
    cmr_u16 i = 0;
    cmr_u16 j = 0;
    cmr_u8 high_val = 0;
    cmr_u8 low_val = 0;
    cmr_u32 checksum = 0;

    SENSOR_LOGI("E");
    if (NULL == param->pval) {
        SENSOR_LOGE("s5k3p8sm_parse_otp is NULL data");
        return -1;
    }
    buff = param->pval;

    if (1 != buff[0]) {
        SENSOR_LOGE("s5k3p8sm_parse_otp is wrong data");
        param->pval = NULL;
        return -1;
    } else {
        single_otp = &s5k3p3_otp_info.single_otp;
        single_otp->program_flag = buff[i++];
        SENSOR_LOGI("program_flag = %d", single_otp->program_flag);

        checksum += single_otp->program_flag;
        single_otp->module_info.year = buff[i++];
        checksum += single_otp->module_info.year;
        single_otp->module_info.month = buff[i++];
        checksum += single_otp->module_info.month;
        single_otp->module_info.day = buff[i++];
        checksum += single_otp->module_info.day;
        single_otp->module_info.mid = buff[i++];
        checksum += single_otp->module_info.mid;
        single_otp->module_info.lens_id = buff[i++];
        checksum += single_otp->module_info.lens_id;
        single_otp->module_info.vcm_id = buff[i++];
        checksum += single_otp->module_info.vcm_id;
        single_otp->module_info.driver_ic_id = buff[i++];
        checksum += single_otp->module_info.driver_ic_id;

        high_val = buff[i++];
        checksum += high_val;
        low_val = buff[i++];
        checksum += low_val;
        single_otp->iso_awb_info.iso = (high_val << 8 | low_val);
        high_val = buff[i++];
        checksum += high_val;
        low_val = buff[i++];
        checksum += low_val;
        single_otp->iso_awb_info.gain_r = (high_val << 8 | low_val);
        high_val = buff[i++];
        checksum += high_val;
        low_val = buff[i++];
        checksum += low_val;
        single_otp->iso_awb_info.gain_g = (high_val << 8 | low_val);
        high_val = buff[i++];
        checksum += high_val;
        low_val = buff[i++];
        checksum += low_val;
        single_otp->iso_awb_info.gain_b = (high_val << 8 | low_val);

        for (j = 0; j < OTP_LSC_INFO_LEN; j++) {
            s5k3p3_opt_lsc_data[j] = buff[i++];
            checksum += s5k3p3_opt_lsc_data[j];
        }
        single_otp->lsc_info.lsc_data_addr = s5k3p3_opt_lsc_data;
        single_otp->lsc_info.lsc_data_size = sizeof(s5k3p3_opt_lsc_data);

        single_otp->af_info.flag = buff[i++];
        if (0 == single_otp->af_info.flag)
            SENSOR_LOGE("af otp is wrong");

        checksum = 0; //+= s5k3p3_otp_info.af_info.flag;
        /* cause checksum, skip af flag */
        low_val = buff[i++];
        checksum += low_val;
        high_val = buff[i++];
        checksum += high_val;
        single_otp->af_info.infinite_cali = (high_val << 8 | low_val);
        low_val = buff[i++];
        checksum += low_val;
        high_val = buff[i++];
        checksum += high_val;
        single_otp->af_info.macro_cali = (high_val << 8 | low_val);

        single_otp->checksum = buff[i++];
        SENSOR_LOGI(
            "checksum = 0x%x s5k3p3_otp_info.checksum = 0x%x,r=%d,g=%d,b=%d",
            checksum, single_otp->checksum, single_otp->iso_awb_info.gain_r,
            single_otp->iso_awb_info.gain_g, single_otp->iso_awb_info.gain_b);

        if ((checksum % 0xff) != single_otp->checksum) {
            SENSOR_LOGE("checksum error!");
            single_otp->program_flag = 0;
            param->pval = NULL;
            return -1;
        }
        param->pval = (void *)&s5k3p3_otp_info;
    }
    SENSOR_LOGI("param->pval = %p", param->pval);

    return 0;
}

static cmr_int s5k3p8sm_drv_ex_write_exposure(cmr_handle handle,
                                                 cmr_int param) {
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

    ret = s5k3p8sm_drv_write_exp_dummy(handle, exposure_line, dummy_line, size_index);

    sns_drv_cxt->sensor_ev_info.preview_shutter = sns_drv_cxt->exp_line;

    return ret;
}

static cmr_int s5k3p8sm_drv_update_gain(cmr_handle handle, cmr_int param) {
    cmr_int ret_value = SENSOR_SUCCESS;
    uint32_t real_gain = 0;
    uint32_t a_gain = 0;
    uint32_t d_gain = 0;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    real_gain = param >> 2;

    SENSOR_PRINT("SENSOR_S5K3P8SM: real_gain:%d, param: %ld", real_gain, param);

    if (real_gain <= 16 * 32) {
        a_gain = real_gain;
        d_gain = 256;
    } else {
        a_gain = 16 * 32;
        d_gain = 256.0 * real_gain / a_gain;
        SENSOR_LOGI("s5k3p8sm_drv: real_gain:0x%x, a_gain: 0x%x, d_gain: 0x%x",
                    (uint32_t)real_gain, (uint32_t)a_gain, (uint32_t)d_gain);
        if ((uint32_t)d_gain > 256 * 256)
            d_gain = 256 * 256; // d_gain < 256x
    }

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x204, a_gain);

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x20e, d_gain);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x210, d_gain);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x212, d_gain);
    ret_value = hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x214, d_gain);

    SENSOR_LOGI("a_gain:0x%x, d_gain: 0x%x", a_gain, d_gain);

    return ret_value;
}

static cmr_int s5k3p8sm_drv_write_gain(cmr_handle handle, cmr_s32 param) {
    cmr_int ret = SENSOR_SUCCESS;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    SENSOR_LOGI("param: %ld", param);

    ret = s5k3p8sm_drv_update_gain(handle, param);
    sns_drv_cxt->sensor_ev_info.preview_gain = param;

    return ret;
}

static cmr_int s5k3p8sm_drv_read_gain(cmr_handle handle, cmr_u32 param) {
    cmr_int rtn = SENSOR_SUCCESS;
    cmr_u32 again = 0;
    cmr_u32 dgain = 0;
    cmr_u32 gain = 0;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    again = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0204);
    dgain = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0210);
    gain = again * dgain;

    SENSOR_LOGI("gain: 0x%x", gain);
    return rtn;
}

static uint16_t s5k3p8sm_drv_get_shutter(cmr_handle handle) {
    uint16_t shutter;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    shutter = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0202) & 0xffff;

    return shutter;
}

static cmr_int s5k3p8sm_drv_before_snapshot(cmr_handle handle,
                                              cmr_int param) {
    cmr_u8 ret_l, ret_m, ret_h;
    cmr_u32 capture_exposure, preview_maxline;
    cmr_u32 capture_maxline, preview_exposure;
    cmr_u32 capture_mode = param & 0xffff;
    cmr_u32 preview_mode = (param >> 0x10) & 0xffff;
    cmr_u16 exposure_line = 0;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    cmr_u32 prv_linetime = sns_drv_cxt->trim_tab_info[preview_mode].line_time;
    cmr_u32 cap_linetime = sns_drv_cxt->trim_tab_info[capture_mode].line_time;
    cmr_u32 frame_len = 0x00;
    cmr_u32 gain = 0;

    SENSOR_LOGI("mode: 0x%08lx,capture_mode:%d", param, capture_mode);

    if (preview_mode == capture_mode) {
        SENSOR_LOGI("prv mode equal to capmode");
        goto CFG_INFO;
    }

    if(sns_drv_cxt->ops_cb.set_mode)
        sns_drv_cxt->ops_cb.set_mode(sns_drv_cxt->caller_handle, capture_mode);
    if(sns_drv_cxt->ops_cb.set_mode_wait_done) 
        sns_drv_cxt->ops_cb.set_mode_wait_done(sns_drv_cxt->caller_handle);

    preview_exposure = sns_drv_cxt->sensor_ev_info.preview_shutter;
    gain = sns_drv_cxt->sensor_ev_info.preview_gain;

    capture_exposure = preview_exposure * prv_linetime / cap_linetime;

    SENSOR_LOGI("prev_exp=%d,cap_exp=%d,gain=%d",
                 preview_exposure, capture_exposure, gain);

    s5k3p8sm_drv_write_exp_dummy(handle, capture_exposure, 0, capture_mode);
    s5k3p8sm_drv_update_gain(handle, gain);

CFG_INFO:
    exposure_line = s5k3p8sm_drv_get_shutter(handle);

    if(sns_drv_cxt->ops_cb.set_exif_info) {
        sns_drv_cxt->ops_cb.set_exif_info(sns_drv_cxt->caller_handle,
                                  SENSOR_EXIF_CTRL_EXPOSURETIME, exposure_line);
        sns_drv_cxt->ops_cb.set_exif_info(sns_drv_cxt->caller_handle,
                                  SENSOR_EXIF_CTRL_APERTUREVALUE, 20);
        sns_drv_cxt->ops_cb.set_exif_info(sns_drv_cxt->caller_handle,
                                  SENSOR_EXIF_CTRL_MAXAPERTUREVALUE, 20);
        sns_drv_cxt->ops_cb.set_exif_info(sns_drv_cxt->caller_handle,
                                  SENSOR_EXIF_CTRL_FNUMBER, 20);
    } else {
        sns_drv_cxt->exif_info.exposure_line = exposure_line;
        sns_drv_cxt->exif_info.aperture_value = 29;
        sns_drv_cxt->exif_info.max_aperture_value = 20;
        sns_drv_cxt->exif_info.numerator = 20;
    }
    sns_drv_cxt->exp_line = exposure_line;
    sns_drv_cxt->exp_time = exposure_line * cap_linetime / 1000;

    return SENSOR_SUCCESS;
}

static cmr_int s5k3p8sm_drv_get_raw_info(cmr_handle handle) {
    cmr_int rtn = SENSOR_SUCCESS;
    struct raw_param_info_tab *tab_ptr =
        (struct raw_param_info_tab *)s_s5k3p8sm_raw_param_tab;
    cmr_u32 param_id ,i = 0x00;

    /*read param id from sensor omap*/
    param_id = S5K3P8SM_RAW_PARAM_COM;

    for (i = 0x00;; i++) {
        if (RAW_INFO_END_ID == tab_ptr[i].param_id) {
            SENSOR_LOGI("s5k3p8sm_drv_get_raw_info end");
            break;
        } else if (PNULL != tab_ptr[i].identify_otp) {
            if (SENSOR_SUCCESS == tab_ptr[i].identify_otp(handle, 0)) {
                // ss5k3p8sm_drv_mipi_raw_info_ptr = tab_ptr[i].info_ptr;
                SENSOR_LOGI("s5k3p8sm_drv_get_raw_info success");
                break;
            }
        }
    }

    return rtn;
}

static cmr_int s5k3p8sm_drv_init_exif_info(cmr_handle handle, void**exif_info_in/*in*/) {
    cmr_int ret = SENSOR_FAIL;
    EXIF_SPEC_PIC_TAKING_COND_T *exif_ptr = NULL;
    *exif_info_in = NULL;
    SENSOR_IC_CHECK_HANDLE(handle);

    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
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

    exif_ptr->ExposureTime.denominator = 1000000;

    return ret;
}

static cmr_int s5k3p8sm_drv_stream_on(cmr_handle handle, cmr_s32 param) {
    cmr_s32 ret = SENSOR_SUCCESS;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("StreamOn E");

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0100, 0x0100);
#if 1
    char value1[PROPERTY_VALUE_MAX];
    property_get("debug.camera.test.mode", value1, "0");
    if (!strcmp(value1, "1")) {
        SENSOR_LOGD("enable test mode");
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0600, 0x0002);
    }
#endif

    SENSOR_LOGI("StreamOn out");

    return 0;
}

static cmr_int s5k3p8sm_drv_stream_off(cmr_handle handle, cmr_int param) {
    UNUSED(param);
    cmr_s32 ret = SENSOR_SUCCESS;
    uint16_t value;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    SENSOR_LOGI("StreamOff:E");

    value = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0100);
    ret = hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0100, 0x0000);
    if (value == 0x0100)
        usleep(50 * 1000);

    return ret;
}

static cmr_int s5k3p8sm_drv_com_Identify_otp(cmr_handle handle,
                                           void *param_ptr) {
    UNUSED(param_ptr);
    cmr_s32 rtn = SENSOR_FAIL;
    cmr_s32 param_id;

    SENSOR_LOGI("E:");

    /*read param id from sensor omap*/
    param_id = S5K3P8SM_RAW_PARAM_COM;

    if (S5K3P8SM_RAW_PARAM_COM == param_id) {
        rtn = SENSOR_SUCCESS;
    }

    return rtn;
}

static cmr_int s5k3p8sm_drv_power_on(cmr_handle handle, cmr_int power_on) {
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    struct module_cfg_info *module_info = sns_drv_cxt->module_info;
    SENSOR_AVDD_VAL_E dvdd_val = module_info->dvdd_val;
    SENSOR_AVDD_VAL_E avdd_val = module_info->avdd_val;
    SENSOR_AVDD_VAL_E iovdd_val = module_info->iovdd_val;
    BOOLEAN power_down = MIPI_RAW_INFO.power_down_level;
    BOOLEAN reset_level = MIPI_RAW_INFO.reset_pulse_level;
    uint8_t pid_value = 0x00;

    if (SENSOR_TRUE == power_on) {
        hw_sensor_set_reset_level(sns_drv_cxt->hw_handle, reset_level);
        hw_sensor_power_down(sns_drv_cxt->hw_handle, power_down);
        hw_sensor_set_voltage(sns_drv_cxt->hw_handle, dvdd_val,
                              avdd_val, iovdd_val);
        usleep(1);
        hw_sensor_set_reset_level(sns_drv_cxt->hw_handle, !reset_level);
        hw_sensor_power_down(sns_drv_cxt->hw_handle, !power_down);
        usleep(20);
        hw_sensor_set_mclk(sns_drv_cxt->hw_handle, SENSOR_DEFALUT_MCLK);
        usleep(1000);
    } else {
        hw_sensor_set_mclk(sns_drv_cxt->hw_handle, SENSOR_DISABLE_MCLK);
        hw_sensor_set_reset_level(sns_drv_cxt->hw_handle, reset_level);
        hw_sensor_power_down(sns_drv_cxt->hw_handle, power_down);
        hw_sensor_set_voltage(sns_drv_cxt->hw_handle,SENSOR_AVDD_CLOSED,
                              SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
    }

    SENSOR_LOGI("s5k3p8sm_drv_power_on(1:on, 0:off): %ld, "
                     "reset_level %d, dvdd_val %d",
                     power_on, reset_level, dvdd_val);
    return SENSOR_SUCCESS;
}

static cmr_int s5k3p8sm_drv_write_otp_gain(cmr_handle handle,
                                         cmr_u32 *param) {
    cmr_int ret_value = SENSOR_SUCCESS;
    uint16_t value = 0x00;
    float a_gain = 0;
    float d_gain = 0;
    float real_gain = 0.0f;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param);
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
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x204, (cmr_u32)a_gain);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x20e, (cmr_u32)d_gain);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x210, (cmr_u32)d_gain);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x212, (cmr_u32)d_gain);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x214, (cmr_u32)d_gain);

    SENSOR_LOGI("write_gain:0x%x\n", *param);

    return ret_value;
}

static cmr_int s5k3p8sm_drv_read_otp_gain(cmr_handle handle,
                                        uint32_t *param) {
    cmr_int ret = SENSOR_SUCCESS;
    cmr_u16 gain_a = 0;
    cmr_u16 gain_d = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    gain_a = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0204) & 0xffff;
    gain_d = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0210) & 0xffff;
    *param = gain_a * gain_d;

    SENSOR_LOGI("gain: %d", *param);

    return ret;
}

static cmr_int s5k3p8sm_drv_get_static_info(cmr_handle handle,
                                          cmr_u32 *param) {
    cmr_int ret = SENSOR_SUCCESS;
    struct sensor_ex_info *ex_info;
    uint32_t up = 0;
    uint32_t down = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    struct sensor_fps_info *fps_info = sns_drv_cxt->fps_info;
    struct sensor_static_info *static_info = sns_drv_cxt->static_info;
    struct module_cfg_info *module_info = sns_drv_cxt->module_info;
    if(!(fps_info && static_info && module_info)) {
        SENSOR_LOGE("error:null pointer checked.return");
        return SENSOR_FAIL;
    }

    if (!fps_info->is_init) {
        s5k3p8sm_drv_init_fps_info(handle);
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

    memcpy(&ex_info->fov_info, &static_info->fov_info, sizeof(static_info->fov_info));

#if defined(CONFIG_CAMERA_OIS_FUNC)
    Ois_get_pose_dis(handle, &up, &down);
#endif
    ex_info->pos_dis.up2hori = up;
    ex_info->pos_dis.hori2down = down;
    sensor_ic_print_static_info("s5k3p8sm",ex_info);

    return ret;
}

static cmr_int s5k3p8sm_drv_get_fps_info(cmr_handle handle,
                                       uint32_t *param) {
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_MODE_FPS_T *fps_info;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    struct sensor_fps_info *fps_data = sns_drv_cxt->fps_info;

    if (!fps_data->is_init) {
        s5k3p8sm_drv_init_fps_info(handle);
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

    return ret;
}

static const struct pd_pos_info s5k3p8sm_drv_pd_pos_l[] = {
	{ 4,  7}, {20, 11}, {40, 11}, {56,  7}, { 8, 27}, {24, 31},
	{36, 31}, {52, 27}, { 8, 43}, {24, 39}, {36, 39}, {52, 43},
	{ 4, 63}, {20, 59}, {40, 59}, {56, 63},
};

static const struct pd_pos_info s5k3p8sm_drv_pd_pos_r[] = {
	{ 4, 11}, {20, 15}, {40, 15}, {56, 11}, { 8, 23}, {24, 27},
	{36, 27}, {52, 23}, { 8, 47}, {24, 43}, {36, 43}, {52, 47},
	{ 4, 59}, {20, 55}, {40, 55}, {56, 59},
};

static cmr_int s5k3p8sm_drv_get_pdaf_info(cmr_handle handle,
                                        cmr_u32 *param) {
    cmr_int ret = SENSOR_SUCCESS;
    struct sensor_pdaf_info *pdaf_info = NULL;
    cmr_u16 pd_pos_r_size = 0;
    cmr_u16 pd_pos_l_size = 0;

    SENSOR_IC_CHECK_PTR(param);

    pdaf_info = (struct sensor_pdaf_info *)param;
    pd_pos_r_size = NUMBER_OF_ARRAY(s5k3p8sm_drv_pd_pos_r);
    pd_pos_l_size = NUMBER_OF_ARRAY(s5k3p8sm_drv_pd_pos_l);
    if (pd_pos_r_size != pd_pos_l_size) {
        SENSOR_LOGE("size not match s5k3l8xxm3_pd_pos_l");
        return -1;
    }

    pdaf_info->pd_offset_x = 16;
    pdaf_info->pd_offset_y = 16;
    pdaf_info->pd_pitch_x = 64;
    pdaf_info->pd_pitch_y = 64;
    pdaf_info->pd_density_x = 4;
    pdaf_info->pd_density_y = 4;
    pdaf_info->pd_block_num_x = 72;
    pdaf_info->pd_block_num_y = 54;
    pdaf_info->pd_pos_size = pd_pos_r_size;
    pdaf_info->pd_pos_r = (struct pd_pos_info *)s5k3p8sm_drv_pd_pos_r;
    pdaf_info->pd_pos_l = (struct pd_pos_info *)s5k3p8sm_drv_pd_pos_l;
    pdaf_info->vendor_type = SENSOR_VENDOR_S5K3P8SM;
    pdaf_info->type2_info.data_type = 0x30;
    pdaf_info->type2_info.data_format = DATA_RAW10;
    pdaf_info->type2_info.width = 72 * 2;
    pdaf_info->type2_info.height = 54 * 16;
    pdaf_info->type2_info.pd_size = pdaf_info->type2_info.width * pdaf_info->type2_info.height * 10 / 8;

    return ret;
}

static cmr_int s5k3p8sm_drv_access_val(cmr_handle handle, cmr_int param) {
    cmr_int rtn = SENSOR_SUCCESS;
    SENSOR_VAL_T *param_ptr = (SENSOR_VAL_T *)param;
    uint16_t tmp;

    SENSOR_LOGI("param_ptr = %p", param_ptr);
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param_ptr);

    SENSOR_LOGI("param_ptr->type=%x", param_ptr->type);
    switch (param_ptr->type) {
    case SENSOR_VAL_TYPE_INIT_OTP:
      //  s5k3p8sm_otp_init(handle);
        break;
    case SENSOR_VAL_TYPE_SHUTTER:
        *((uint32_t *)param_ptr->pval) = s5k3p8sm_drv_get_shutter(handle);
        break;
    case SENSOR_VAL_TYPE_READ_OTP:
       // s5k3p8sm_otp_read(handle, param_ptr);
        break;
    case SENSOR_VAL_TYPE_PARSE_OTP:
        //s5k3p8sm_parse_otp(handle, param_ptr);
        break;
    case SENSOR_VAL_TYPE_WRITE_OTP:
        break;
    case SENSOR_VAL_TYPE_GET_RELOADINFO: {
    } break;
    case SENSOR_VAL_TYPE_GET_AFPOSITION:
        *(uint32_t *)param_ptr->pval = 0; // cur_af_pos;
        break;
    case SENSOR_VAL_TYPE_WRITE_OTP_GAIN:
        rtn = s5k3p8sm_drv_write_otp_gain(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_READ_OTP_GAIN:
        rtn = s5k3p8sm_drv_read_otp_gain(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_GET_STATIC_INFO:
        rtn = s5k3p8sm_drv_get_static_info(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_GET_FPS_INFO:
        rtn = s5k3p8sm_drv_get_fps_info(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_GET_PDAF_INFO:
        rtn = s5k3p8sm_drv_get_pdaf_info(handle, param_ptr->pval);
        break;
    default:
        break;
    }

    SENSOR_LOGI("X");

    return rtn;
}

static cmr_int s5k3p8sm_drv_handle_create(
          struct sensor_ic_drv_init_para *init_param, cmr_handle* sns_ic_drv_handle) {
    cmr_int ret = SENSOR_SUCCESS;
    struct sensor_ic_drv_cxt * sns_drv_cxt = NULL; 

    SENSOR_LOGI("E:");
    ret = sensor_ic_drv_create(init_param,sns_ic_drv_handle);
    if(SENSOR_IC_FAILED == ret) {
        SENSOR_LOGE("sensor instance create failed!");
        return ret;
    }
    sns_drv_cxt = *sns_ic_drv_handle;

    sensor_ic_set_match_module_info(sns_drv_cxt, ARRAY_SIZE(MODULE_INFO), MODULE_INFO);
    sensor_ic_set_match_resolution_info(sns_drv_cxt, ARRAY_SIZE(RES_TAB_RAW), RES_TAB_RAW);
    sensor_ic_set_match_trim_info(sns_drv_cxt, ARRAY_SIZE(RES_TRIM_TAB), RES_TRIM_TAB);
    sensor_ic_set_match_static_info(sns_drv_cxt, ARRAY_SIZE(STATIC_INFO), STATIC_INFO);
    sensor_ic_set_match_fps_info(sns_drv_cxt, ARRAY_SIZE(FPS_INFO), FPS_INFO);

    s5k3p8sm_drv_init_exif_info(sns_drv_cxt, &sns_drv_cxt->exif_ptr);

    SENSOR_LOGI("X:");

    return ret;
}

static cmr_s32 s5k3p8sm_drv_handle_delete(cmr_handle handle, uint32_t *param) {
    cmr_s32 ret = SENSOR_SUCCESS;
    /*if has private data,you must release it here*/
    SENSOR_LOGI("E:");
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    ret = sensor_ic_drv_delete(handle,param);
    SENSOR_LOGI("X:");

    return ret;
}

static cmr_int s5k3p8sm_drv_get_private_data(cmr_handle handle,
                                                     cmr_uint cmd, void**param){
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param);

    ret = sensor_ic_get_private_data(handle,cmd, param);
    return ret;
}

static struct sensor_ic_ops s5k3p8sm_ops_tab  = {
    .create_handle = s5k3p8sm_drv_handle_create,
    .delete_handle = s5k3p8sm_drv_handle_delete,
    .get_data = s5k3p8sm_drv_get_private_data,
    .power  = s5k3p8sm_drv_power_on,
    .identify = s5k3p8sm_drv_identify,

    .write_exp = s5k3p8sm_drv_write_exposure,
    .write_gain_value = s5k3p8sm_drv_write_gain,
    .ex_write_exp = s5k3p8sm_drv_ex_write_exposure,
    .read_aec_info = NULL,
    .ext_ops = {
        [SENSOR_IOCTL_BEFORE_SNAPSHOT].ops = s5k3p8sm_drv_before_snapshot,
        [SENSOR_IOCTL_STREAM_ON].ops = s5k3p8sm_drv_stream_on,
        [SENSOR_IOCTL_STREAM_OFF].ops = s5k3p8sm_drv_stream_off,
        //[SENSOR_IOCTL_CUS_FUNC_3].ops = s5k3p8sm_drv_get_exif_info,
        [SENSOR_IOCTL_ACCESS_VAL].ops = s5k3p8sm_drv_access_val,
    }
};

