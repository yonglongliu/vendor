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
#include "sensor_gc0310_mipi.h"

#define VIDEO_MODE_TAB gc0310_video_mode_tab
#define YUV_INFO       g_GC0310_MIPI_yuv_info
#define RES_TRIM_TAB   s_gc0310_resolution_trim_tab
#define MODULE_INFO    s_GC0310_module_info_tab
#define RES_TAB_YUV    s_gc0310_resolution_tab_yuv

static cmr_int gc0310_drv_power_on(cmr_handle handle, cmr_uint power_on) {
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    struct module_cfg_info *module_info = sns_drv_cxt->module_info;

    SENSOR_AVDD_VAL_E dvdd_val = module_info->dvdd_val;
    SENSOR_AVDD_VAL_E avdd_val = module_info->avdd_val;
    SENSOR_AVDD_VAL_E iovdd_val = module_info->iovdd_val;
    BOOLEAN power_down = YUV_INFO.power_down_level;
    BOOLEAN reset_level = YUV_INFO.reset_pulse_level;

    if (SENSOR_TRUE == power_on) {
        hw_sensor_power_down(sns_drv_cxt->hw_handle, !power_down);
        usleep(1000);
        hw_sensor_set_iovdd_val(sns_drv_cxt->hw_handle, iovdd_val);
        usleep(2000);
        hw_sensor_set_avdd_val(sns_drv_cxt->hw_handle, avdd_val);
        usleep(1000);
        hw_sensor_set_mclk(sns_drv_cxt->hw_handle, SENSOR_DEFALUT_MCLK);
        usleep(1000);
        hw_sensor_power_down(sns_drv_cxt->hw_handle, power_down);
        usleep(1000);
        hw_sensor_power_down(sns_drv_cxt->hw_handle, !power_down);
    } else {
        hw_sensor_power_down(sns_drv_cxt->hw_handle, power_down);
        usleep(1000);
        hw_sensor_set_mclk(sns_drv_cxt->hw_handle, SENSOR_DISABLE_MCLK);
        usleep(1000);
        hw_sensor_set_avdd_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED);
        usleep(2000);
        hw_sensor_set_iovdd_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED);
        usleep(1000);
    }
    return SENSOR_SUCCESS;
}

static cmr_int gc0310_drv_identify(cmr_handle handle, cmr_uint param) {
#define GC0310_MIPI_PID_ADDR1 0xf0
#define GC0310_MIPI_PID_ADDR2 0xf1
#define GC0310_MIPI_SENSOR_ID 0xa310
    UNUSED(param);
    cmr_u16 sensor_id = 0;
    cmr_u8 pid_value = 0;
    cmr_u8 ver_value = 0;
    cmr_u16 i;

    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    for (i = 0; i < 3; i++) {
        sensor_id = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 
                                       GC0310_MIPI_PID_ADDR1) << 8;
        sensor_id |= hw_sensor_read_reg(sns_drv_cxt->hw_handle, 
                                        GC0310_MIPI_PID_ADDR2);
        SENSOR_LOGI("%s sensor_id gc0310 is %x\n", __func__, sensor_id);
        if (sensor_id == GC0310_MIPI_SENSOR_ID) {
            SENSOR_LOGI("sensor is GC0310_MIPI\n");
            return SENSOR_SUCCESS;
        }
    }

    return SENSOR_FAIL;
}

static cmr_int gc0310_drv_get_brightness(cmr_handle handle,
                                                   cmr_u32 *param) {
    cmr_u16 i = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x01);
    *param = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x14);
    SENSOR_LOGI("SENSOR: get_brightness: lumma = 0x%x\n", *param);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x00);
    return 0;
}

static cmr_int gc0310_drv_access_val(cmr_handle handle,
                                             cmr_uint param) {
    cmr_int rtn = SENSOR_SUCCESS;
    SENSOR_VAL_T *param_ptr = (SENSOR_VAL_T *)param;
    cmr_u16 tmp;

    SENSOR_LOGI("E param_ptr = %p", param_ptr);
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param_ptr);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("param_ptr->type=%x", param_ptr->type);
    switch (param_ptr->type) {
    case SENSOR_VAL_TYPE_GET_BV:
        rtn = gc0310_drv_get_brightness(handle, param_ptr->pval);
        break;
    default:
        break;
    }

    SENSOR_LOGI("X");

    return rtn;
}

static cmr_int gc0310_drv_set_brightness(cmr_handle handle,
                                                   cmr_int level) {
    cmr_u16 i;
    SENSOR_REG_T *reg_tab = NULL;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    reg_tab = brightness_tab[sns_drv_cxt->module_id] + 2 * level;

    if (level > 6)
        return 0;

    for (i = 0; (0xFF != reg_tab[i].reg_addr) &&
                (0xFF != reg_tab[i].reg_value); i++) {
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, reg_tab[i].reg_addr,
                             reg_tab[i].reg_value);
    }

    return 0;
}

static cmr_int gc0310_drv_init(cmr_handle handle) {
    cmr_u16 i;
    SENSOR_REG_T *reg_tab = (SENSOR_REG_T *)GC0310_MIPI_YUV_COMMON2;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("gx ");
    for (i = 0; (0xFF != reg_tab[i].reg_addr) &&
                (0xFF != reg_tab[i].reg_value); i++) {
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, reg_tab[i].reg_addr,
                             reg_tab[i].reg_value);
    }
    SENSOR_LOGI("X");

    return 0;
}

static cmr_int gc0310_drv_set_video_mode(cmr_handle handle,
                                                   cmr_int mode) {
    cmr_u16 i = 0x00;
    SENSOR_REG_T_PTR reg_tab = NULL;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    reg_tab = video_mode_tab[sns_drv_cxt->module_id] + 18*mode;

    SENSOR_LOGI("SENSOR: set_video_mode: mode = %lu\n", mode);
    if (mode > 1 || mode == 0)
        return 0;

    for (i = 0x00; (0xff != reg_tab[i].reg_addr) ||
                   (0xff != reg_tab[i].reg_value); i++) {
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, reg_tab[i].reg_addr,
                             reg_tab[i].reg_value);
    }

    return 0;
}

static cmr_int gc0310_drv_set_ev(cmr_handle handle,
                                        cmr_uint level) {
    cmr_u16 i;
    SENSOR_REG_T *reg_tab = NULL;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    reg_tab = ev_tab[sns_drv_cxt->module_id] + 4*level;

    if (level > 6)
        return 0;

    for (i = 0; (0xFF != reg_tab[i].reg_addr) ||
                (0xFF != reg_tab[i].reg_value); i++) {
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, reg_tab[i].reg_addr,
                             reg_tab[i].reg_value);
    }
    SENSOR_LOGI("set ev");

    return 0;
}

static cmr_int gc0310_drv_set_anti_flicker(cmr_handle handle,
                                                  cmr_uint param) {
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    switch (param) {
    case FLICKER_50HZ:
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x00);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x05, 0x02); // hb
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x06, 0xd1);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x07, 0x00); // vb
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x08, 0x22);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x01);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x25, 0x00); // step
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x26, 0x6a);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x27, 0x02); // level1
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x28, 0x12);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x29, 0x03); // level2
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x2a, 0x50);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x2b, 0x05); // 6e8//level3 640
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x2c, 0xcc);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x2d, 0x07); // level4
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x2e, 0x74);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x00);
        break;

    case FLICKER_60HZ:
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x00);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x05, 0x02);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x06, 0x60);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x07, 0x00);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x08, 0x58);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x01);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x25, 0x00); // anti-flicker step [11:8]
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x26, 0x60); // anti-flicker step [7:0]

        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x27, 0x02); // exp level 0  14.28fps
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x28, 0x40);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x29, 0x03); // exp level 1  12.50fps
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x2a, 0x60);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x2b, 0x06); // exp level 2  6.67fps
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x2c, 0x00);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x2d, 0x08); // exp level 3  5.55fps
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x2e, 0x40);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x00);
        break;

    default:
        break;
    }

    return 0;
}

static cmr_int gc0310_drv_set_awb(cmr_handle handle, cmr_int mode) {
    cmr_u8 awb_en_value;
    cmr_u16 i;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_REG_T *reg_tab = awb_tab[sns_drv_cxt->module_id] + 6*mode;

    if (mode > 6)
        return 0;

    for (i = 0; (0xFF != reg_tab[i].reg_addr) ||
                (0xFF != reg_tab[i].reg_value); i++) {
        hw_sensor_write_reg(sns_drv_cxt->hw_handle,
                            reg_tab[i].reg_addr, reg_tab[i].reg_value);
    }

    return 0;
}

static cmr_int gc0310_drv_set_contrast(cmr_handle handle, cmr_int level) {
    cmr_u16 i;
    SENSOR_REG_T *reg_tab = NULL;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    reg_tab = contrast_tab[sns_drv_cxt->module_id] + 2*level;

    if (level > 6)
        return 0;

    for (i = 0; (0xFF != reg_tab[i].reg_addr) &&
                (0xFF != reg_tab[i].reg_value); i++) {
        hw_sensor_write_reg(sns_drv_cxt->hw_handle,
                            reg_tab[i].reg_addr, reg_tab[i].reg_value);
    }

    return 0;
}

static cmr_int gc0310_drv_set_saturation(cmr_handle handle,
                                    cmr_uint level) {
    cmr_u16 i;
    SENSOR_REG_T *reg_tab  = NULL;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    reg_tab = saturation_tab[sns_drv_cxt->module_id] + 3*level;

    if (level > 6)
        return 0;

    for (i = 0; (0xFF != reg_tab[i].reg_addr) &&
                (0xFF != reg_tab[i].reg_value); i++) {
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, reg_tab[i].reg_addr,
                             reg_tab[i].reg_value);
    }

    return 0;
}

static cmr_int gc0310_drv_set_preview_mode(cmr_handle handle,
                                                      cmr_int preview_mode) {
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("set_preview_mode: preview_mode = %ld\n", preview_mode);

    gc0310_drv_set_anti_flicker(handle, 0);
    switch (preview_mode) {
    case DCAMERA_ENVIRONMENT_NORMAL:
        // YCP_saturation
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x01);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3c, 0x20);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x00);
        SENSOR_LOGI("set_preview_mode: DCAMERA_ENVIRONMENT_NORMAL\n");
        break;

    case 1: // DCAMERA_ENVIRONMENT_NIGHT:
        // YCP_saturation
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x01);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3c, 0x30);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x00);
        SENSOR_LOGI("set_preview_mode: DCAMERA_ENVIRONMENT_NIGHT\n");
        break;

    case 3: // SENSOR_ENVIROMENT_PORTRAIT:
        // YCP_saturation
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xd1, 0x34);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xd2, 0x34);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x01);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3c, 0x20);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x00);
        SENSOR_LOGI("set_preview_mode: SENSOR_ENVIROMENT_PORTRAIT\n");
        break;

    case 4: // SENSOR_ENVIROMENT_LANDSCAPE://4
        // nightmode disable
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xd1, 0x4c);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xd2, 0x4c);

        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x01);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3c, 0x20);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x00);
        SENSOR_LOGI("set_preview_mode: SENSOR_ENVIROMENT_LANDSCAPE\n");
        break;

    case 2: // SENSOR_ENVIROMENT_SPORTS://2
        // nightmode disable
        // YCP_saturation
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xd1, 0x40);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xd2, 0x40);

        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x01);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3c, 0x20);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x00);
        SENSOR_LOGI("set_preview_mode: SENSOR_ENVIROMENT_SPORTS\n");
        break;

    default:
        break;
    }

    SENSOR_Sleep(10);
    return 0;
}

static cmr_int gc0310_drv_set_image_effect(cmr_handle handle,
                                                      cmr_int effect_type) {
    cmr_u16 i = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_REG_T *reg_tab = image_effect_tab[sns_drv_cxt->module_id] + 4*effect_type;
    if (effect_type > 7)
        return 0;

    for (i = 0; (0xFF != reg_tab[i].reg_addr) || (0xFF != reg_tab[i].reg_value); i++) {
        hw_sensor_write_reg_8bits(sns_drv_cxt->hw_handle,reg_tab[i].reg_addr,
                              reg_tab[i].reg_value);
    }

    return 0;
}

static cmr_int gc0310_drv_after_snapshot(cmr_handle handle,
                                                cmr_uint param) {
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    if(sns_drv_cxt->ops_cb.set_mode)
        ret = sns_drv_cxt->ops_cb.set_mode(sns_drv_cxt->caller_handle, param);

    return ret;
}

static cmr_int gc0310_drv_before_snapshot(cmr_handle handle,
                                       cmr_uint sensor_snapshot_mode) {
    cmr_int ret = SENSOR_SUCCESS;
    sensor_snapshot_mode &= 0xffff;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    if(sns_drv_cxt->ops_cb.set_mode)
        ret = sns_drv_cxt->ops_cb.set_mode(sns_drv_cxt->caller_handle, sensor_snapshot_mode);
    if(sns_drv_cxt->ops_cb.set_mode_wait_done)
        ret = sns_drv_cxt->ops_cb.set_mode_wait_done(sns_drv_cxt->caller_handle);

    switch (sensor_snapshot_mode) {
    case SENSOR_MODE_PREVIEW_ONE:
        SENSOR_LOGI("Capture VGA Size");
        break;
    case SENSOR_MODE_SNAPSHOT_ONE_FIRST:
    case SENSOR_MODE_SNAPSHOT_ONE_SECOND:
        break;
    default:
        break;
    }

    SENSOR_LOGI("Before Snapshot,ret=%d", ret);
    return ret;
}

static cmr_int gc0310_drv_stream_on(cmr_handle handle, cmr_uint param) {
    SENSOR_LOGI("Start");

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x03);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x10, 0x94);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x00);

    return 0;
}

static cmr_int gc0310_drv_stream_off(cmr_handle handle, cmr_uint param) {
    SENSOR_LOGI("gx Stop 1");
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    gc0310_drv_init(handle);

    SENSOR_LOGI("gx Stop");
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x03);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x10, 0x84);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x00);

    return 0;
}

static cmr_int gc0310_drv_handle_create(
          struct sensor_ic_drv_init_para *init_param, cmr_handle* sns_ic_drv_handle) {
    cmr_int ret = SENSOR_SUCCESS;
    struct sensor_ic_drv_cxt * sns_drv_cxt = NULL;

    ret = sensor_ic_drv_create(init_param,sns_ic_drv_handle);
    if(SENSOR_SUCCESS != ret)
        return SENSOR_FAIL;
    sns_drv_cxt = *sns_ic_drv_handle;

    sensor_ic_set_match_module_info(sns_drv_cxt, ARRAY_SIZE(MODULE_INFO), MODULE_INFO);
    sensor_ic_set_match_resolution_info(sns_drv_cxt, ARRAY_SIZE(RES_TAB_YUV), RES_TAB_YUV);
    sensor_ic_set_match_trim_info(sns_drv_cxt, ARRAY_SIZE(RES_TRIM_TAB), RES_TRIM_TAB);
    /*add private here*/
    return ret;
}

static cmr_int gc0310_drv_handle_delete(cmr_handle handle, cmr_u32 *param) {
    cmr_int ret = SENSOR_SUCCESS;
    /*if has private data,you must release it here*/
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    if(sns_drv_cxt->privata_data.buffer)
        free(sns_drv_cxt->privata_data.buffer);

    ret = sensor_ic_drv_delete(handle,param);
    return ret;
}

static cmr_int gc0310_drv_get_private_data(cmr_handle handle,
                                                     cmr_uint cmd, void**param){
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param);

    ret = sensor_ic_get_private_data(handle,cmd, param);
    return ret;
}

static struct sensor_ic_ops s_gc0310_ops_tab = {
    .create_handle = gc0310_drv_handle_create,
    .delete_handle = gc0310_drv_handle_delete,
    .get_data = gc0310_drv_get_private_data,

    .power  = gc0310_drv_power_on,
    .identify = gc0310_drv_identify,

    .ext_ops = {
        [SENSOR_IOCTL_BRIGHTNESS].ops = gc0310_drv_set_brightness,
        [SENSOR_IOCTL_CONTRAST].ops = gc0310_drv_set_contrast,
        [SENSOR_IOCTL_SATURATION].ops = gc0310_drv_set_saturation,
        [SENSOR_IOCTL_PREVIEWMODE].ops = gc0310_drv_set_preview_mode, 
        [SENSOR_IOCTL_IMAGE_EFFECT].ops = gc0310_drv_set_image_effect,
        [SENSOR_IOCTL_SET_WB_MODE].ops = gc0310_drv_set_awb,
        [SENSOR_IOCTL_EXPOSURE_COMPENSATION].ops = gc0310_drv_set_ev,
        [SENSOR_IOCTL_ANTI_BANDING_FLICKER].ops = gc0310_drv_set_anti_flicker,
        [SENSOR_IOCTL_VIDEO_MODE].ops = gc0310_drv_set_video_mode, 
        [SENSOR_IOCTL_BEFORE_SNAPSHOT].ops = gc0310_drv_before_snapshot,
        [SENSOR_IOCTL_STREAM_ON].ops = gc0310_drv_stream_on,
        [SENSOR_IOCTL_STREAM_OFF].ops = gc0310_drv_stream_off,
        [SENSOR_IOCTL_ACCESS_VAL].ops = gc0310_drv_access_val,
    }
};

