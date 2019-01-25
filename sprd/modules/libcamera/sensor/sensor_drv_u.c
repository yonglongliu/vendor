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
#define LOG_TAG "sns_drv_u"
#define ATRACE_TAG (ATRACE_TAG_CAMERA | ATRACE_TAG_HAL)

#include <cutils/trace.h>
#include <utils/Log.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>

#include "hw_sensor_drv.h"
#include "sensor_cfg.h"
#include "sensor_drv_u.h"

#if !(defined(CONFIG_CAMERA_ISP_VERSION_V3) ||                                 \
      defined(CONFIG_CAMERA_ISP_VERSION_V4))
#include "isp_cali_interface.h"
#include "isp_param_file_update.h"
#endif

#define SENSOR_CTRL_MSG_QUEUE_SIZE 10
#define SPRD_DUAL_OTP_SIZE 230

#define SENSOR_CTRL_EVT_BASE (CMR_EVT_SENSOR_BASE + 0x800)
#define SENSOR_CTRL_EVT_INIT (SENSOR_CTRL_EVT_BASE + 0x0)
#define SENSOR_CTRL_EVT_EXIT (SENSOR_CTRL_EVT_BASE + 0x1)
#define SENSOR_CTRL_EVT_SETMODE (SENSOR_CTRL_EVT_BASE + 0x2)
#define SENSOR_CTRL_EVT_SETMODONE (SENSOR_CTRL_EVT_BASE + 0x3)
#define SENSOR_CTRL_EVT_CFGOTP (SENSOR_CTRL_EVT_BASE + 0x4)
#define SENSOR_CTRL_EVT_STREAM_CTRL (SENSOR_CTRL_EVT_BASE + 0x5)

/**---------------------------------------------------------------------------*
 **                         Local Variables                                   *
 **---------------------------------------------------------------------------*/
struct sensor_drv_context s_local_sensor_cxt[4];
/**---------------------------------------------------------------------------*
 **                         Local Functions                                   *
 **---------------------------------------------------------------------------*/

static void sensor_set_status(struct sensor_drv_context *sensor_cxt,
                              SENSOR_ID_E sensor_id);

static cmr_int sensor_ctrl_thread_proc(struct cmr_msg *message, void *p_data);
static cmr_int sensor_open(struct sensor_drv_context *sensor_cxt,
                           cmr_u32 sensor_id);
static cmr_int sensor_set_mode_msg(struct sensor_drv_context *sensor_cxt,
                                   cmr_u32 mode, cmr_u32 is_inited);
static cmr_int sensor_set_mode(struct sensor_drv_context *sensor_cxt,
                               cmr_u32 mode, cmr_u32 is_inited);

static cmr_int sensor_stream_off(struct sensor_drv_context *sensor_cxt);

static cmr_int sensor_stream_ctrl(struct sensor_drv_context *sensor_cxt,
                                  cmr_u32 on_off);

static cmr_int sensor_init_defaul_exif(struct sensor_drv_context *sensor_cxt);

static void sensor_rid_save_sensor_info(struct sensor_drv_context *sensor_cxt);

static void sensor_save_info_for_checkXtool(struct sensor_drv_context *sensor_cxt);
static cmr_int sensor_hw_read_i2c(cmr_handle sns_module_handle,
                                  cmr_u16 slave_addr, cmr_u8 *cmd,
                                  cmr_u16 cmd_length);

static cmr_int sensor_hw_write_i2c(cmr_handle sns_module_handle,
                                   cmr_u16 slave_addr, cmr_u8 *cmd,
                                   cmr_u16 cmd_length);

static cmr_int sensor_muti_i2c_write(cmr_handle sns_module_handle, void *param);

static cmr_int sensor_ic_create(struct sensor_drv_context *sensor_cxt,
                                cmr_u32 sensor_id);

static cmr_int sensor_ic_delete(struct sensor_drv_context *sensor_cxt);

static void *sensor_ic_get_data(struct sensor_drv_context *sensor_cxt,
                                cmr_uint cmd);

static cmr_int sensor_ic_identify(struct sensor_drv_context *sensor_cxt,
                                  cmr_u32 sensor_id);

static cmr_int sensor_ic_ex_write_exposure(cmr_handle handle, cmr_uint param);

static cmr_int sensor_ic_write_ae_value(cmr_handle handle, cmr_u32 param);

static cmr_int sensor_ic_write_gain(cmr_handle handle, cmr_u32 param);

static cmr_int sensor_ic_read_aec_info(cmr_handle handle, void *param);

static cmr_int sensor_af_init(cmr_handle sns_module_handle);

static cmr_int sensor_af_deinit(cmr_handle sns_module_handle);

static cmr_int sensor_af_set_pos(cmr_handle sns_module_handle, cmr_u32 pos);

static cmr_int sensor_af_get_pos(cmr_handle sns_module_handle, cmr_u16 *pos);

static cmr_int sensor_af_get_pos_info(cmr_handle sns_module_handle,
                                      struct sensor_vcm_info *info);

static cmr_int sensor_otp_module_init(struct sensor_drv_context *sensor_cxt);

static cmr_int sensor_otp_module_deinit(struct sensor_drv_context *sensor_cxt);

static cmr_int sensor_otp_process(struct sensor_drv_context *sensor_cxt,
                                  uint8_t cmd, uint8_t sub_cmd, void *data);
extern uint32_t isp_raw_para_update_from_file(SENSOR_INFO_T *sensor_info_ptr,
                                              SENSOR_ID_E sensor_id);

LOCAL cmr_int sensor_otp_rw_ctrl(struct sensor_drv_context *sensor_cxt,
                                 uint8_t cmd, uint8_t sub_cmd, void *data);

extern uint32_t isp_raw_para_update_from_file(SENSOR_INFO_T *sensor_info_ptr,
                                              SENSOR_ID_E sensor_id);
cmr_int sensor_set_raw_infor(struct sensor_drv_context *sensor_cxt,
                             cmr_u8 vendor_id);

void sensor_set_cxt_common(struct sensor_drv_context *sensor_cxt) {
    SENSOR_ID_E sensor_id = 0;

    SENSOR_DRV_CHECK_ZERO_VOID(sensor_cxt);
    sensor_id = sensor_cxt->sensor_register_info.cur_id;
#ifndef CONFIG_CAMERA_SENSOR_MULTI_INSTANCE_SUPPORT
    s_local_sensor_cxt[0] = *(struct sensor_drv_context *)sensor_cxt;
#else
    if (sensor_id == 0)
        s_local_sensor_cxt[0] = *sensor_cxt;
    else if (sensor_id == 1)
        s_local_sensor_cxt[1] = *sensor_cxt;
    else if (sensor_id == 2)
        s_local_sensor_cxt[2] = *sensor_cxt;
    else if (sensor_id == 3)
        s_local_sensor_cxt[3] = *sensor_cxt;
#endif

    return;
}

void *sensor_get_dev_cxt(void) { return (void *)&s_local_sensor_cxt[0]; }

void *sensor_get_dev_cxt_Ex(cmr_u32 camera_id) {
    struct sensor_drv_context *p_sensor_cxt = NULL;
    if (camera_id > CAMERA_ID_MAX)
        return NULL;
#ifndef CONFIG_CAMERA_SENSOR_MULTI_INSTANCE_SUPPORT
    p_sensor_cxt = &s_local_sensor_cxt[0];
#else
    if (camera_id == 0)
        p_sensor_cxt = &s_local_sensor_cxt[0];
    else if (camera_id == 1)
        p_sensor_cxt = &s_local_sensor_cxt[1];
    else if (camera_id == 2)
        p_sensor_cxt = &s_local_sensor_cxt[2];
    else if (camera_id == 3)
        p_sensor_cxt = &s_local_sensor_cxt[3];
#endif

    return (void *)p_sensor_cxt;
}

cmr_int sensor_get_flash_level(struct sensor_drv_context *sensor_cxt,
                               struct sensor_flash_level *level) {
    int ret = SENSOR_SUCCESS;
    SENSOR_DRV_CHECK_ZERO(sensor_cxt);

    ret = hw_sensor_get_flash_level(sensor_cxt->hw_drv_handle, level);
    if (0 != ret) {
        SENSOR_LOGE("_Sensor_Device_GetFlashLevel failed, ret=%d \n", ret);
        ret = -1;
    }

    return ret;
}

void sensor_power_on(struct sensor_drv_context *sensor_cxt, cmr_u32 power_on) {
    ATRACE_BEGIN(__FUNCTION__);

    struct sensor_ic_ops *sns_ops = PNULL;

    SENSOR_DRV_CHECK_ZERO_VOID(sensor_cxt);
    SENSOR_DRV_CHECK_ZERO_VOID(sensor_cxt->sensor_info_ptr);
    sns_ops = sensor_cxt->sensor_info_ptr->sns_ops;

    SENSOR_LOGI("E:power_on = %d", power_on);
    /*when use the camera vendor functions, the sensor_cxt should be set at
     * first */
    sensor_set_cxt_common(sensor_cxt);
    if (sns_ops && sns_ops->power) {
        sns_ops->power(sensor_cxt->sns_ic_drv_handle, power_on);
    }

    ATRACE_END();
}

void sensor_set_export_Info(struct sensor_drv_context *sensor_cxt) {
    SENSOR_VIDEO_INFO_T *video_info_ptr = PNULL;
    cmr_u32 i = 0;

    SENSOR_LOGI("E");
    SENSOR_DRV_CHECK_ZERO_VOID(sensor_cxt);
    SENSOR_DRV_CHECK_ZERO_VOID(sensor_cxt->sensor_info_ptr);

    SENSOR_EXP_INFO_T *exp_info_ptr = &sensor_cxt->sensor_exp_info;

    /*get current sensor common configure information*/
    SENSOR_INFO_T *sns_info = sensor_cxt->sensor_info_ptr;
    struct sensor_ic_ops *sns_ops = sensor_cxt->sensor_info_ptr->sns_ops;
    SENSOR_MATCH_T *module = sensor_cxt->current_module;

    /*get curent sensor module configure information*/
    SENSOR_REG_TAB_INFO_T *res_info_ptr =
        sensor_ic_get_data(sensor_cxt, SENSOR_CMD_GET_RESOLUTION);
    struct sensor_trim_tag *res_trim_ptr =
        sensor_ic_get_data(sensor_cxt, SENSOR_CMD_GET_TRIM_TAB);
    struct module_cfg_info *mod_cfg_info =
        sensor_ic_get_data(sensor_cxt, SENSOR_CMD_GET_MODULE_CFG);

    SENSOR_MEMSET(exp_info_ptr, 0x00, sizeof(SENSOR_EXP_INFO_T));
    exp_info_ptr->name = sns_info->name;
    exp_info_ptr->image_format = sns_info->image_format;
    exp_info_ptr->image_pattern = mod_cfg_info->image_pattern;

    /*the high 3bit will be the phase(delay sel)*/
    exp_info_ptr->pclk_polarity = (sns_info->hw_signal_polarity & 0x01);
    exp_info_ptr->vsync_polarity = ((sns_info->hw_signal_polarity >> 2) & 0x1);
    exp_info_ptr->hsync_polarity = ((sns_info->hw_signal_polarity >> 4) & 0x1);
    exp_info_ptr->pclk_delay = ((sns_info->hw_signal_polarity >> 5) & 0x07);

    if (NULL != sns_info->raw_info_ptr) {
        exp_info_ptr->raw_info_ptr =
            (struct sensor_raw_info *)*sns_info->raw_info_ptr;
    }

    if ((NULL != exp_info_ptr->raw_info_ptr) &&
        (NULL != exp_info_ptr->raw_info_ptr->resolution_info_ptr)) {
        exp_info_ptr->raw_info_ptr->resolution_info_ptr->image_pattern =
            mod_cfg_info->image_pattern;
    }

    exp_info_ptr->source_width_max = sns_info->source_width_max;
    exp_info_ptr->source_height_max = sns_info->source_height_max;

    exp_info_ptr->environment_mode = sns_info->environment_mode;
    exp_info_ptr->image_effect = sns_info->image_effect;
    exp_info_ptr->wb_mode = sns_info->wb_mode;
    exp_info_ptr->step_count = sns_info->step_count;
    exp_info_ptr->ext_info_ptr = sns_info->ext_info_ptr;

    if (mod_cfg_info) {
        exp_info_ptr->preview_skip_num = mod_cfg_info->preview_skip_num;
        exp_info_ptr->capture_skip_num = mod_cfg_info->capture_skip_num;
        exp_info_ptr->flash_capture_skip_num =
            mod_cfg_info->flash_capture_skip_num;
        exp_info_ptr->mipi_cap_skip_num = mod_cfg_info->mipi_cap_skip_num;
        exp_info_ptr->preview_deci_num = mod_cfg_info->preview_deci_num;
        exp_info_ptr->change_setting_skip_num =
            mod_cfg_info->change_setting_skip_num;
        exp_info_ptr->video_preview_deci_num =
            mod_cfg_info->video_preview_deci_num;
        exp_info_ptr->threshold_eb = mod_cfg_info->threshold_eb;
        exp_info_ptr->threshold_mode = mod_cfg_info->threshold_mode;
        exp_info_ptr->threshold_start = mod_cfg_info->threshold_start;
        exp_info_ptr->threshold_end = mod_cfg_info->threshold_end;
    }
    exp_info_ptr->sns_ops = sns_info->sns_ops;

    for (i = SENSOR_MODE_COMMON_INIT; i < SENSOR_MODE_MAX; i++) {
        if ((PNULL != res_info_ptr[i].sensor_reg_tab_ptr) ||
            ((0x00 != res_info_ptr[i].width) &&
             (0x00 != res_info_ptr[i].width))) {
            if (SENSOR_IMAGE_FORMAT_JPEG == res_info_ptr[i].image_format) {
                exp_info_ptr->sensor_image_type = SENSOR_IMAGE_FORMAT_JPEG;
            }
            exp_info_ptr->sensor_mode_info[i].mode = i;
            exp_info_ptr->sensor_mode_info[i].width = res_info_ptr[i].width;
            exp_info_ptr->sensor_mode_info[i].height = res_info_ptr[i].height;
            if ((PNULL != res_trim_ptr) &&
                (0x00 != res_trim_ptr[i].trim_width) &&
                (0x00 != res_trim_ptr[i].trim_height)) {
                exp_info_ptr->sensor_mode_info[i].trim_start_x =
                    res_trim_ptr[i].trim_start_x;
                exp_info_ptr->sensor_mode_info[i].trim_start_y =
                    res_trim_ptr[i].trim_start_y;
                exp_info_ptr->sensor_mode_info[i].trim_width =
                    res_trim_ptr[i].trim_width;
                exp_info_ptr->sensor_mode_info[i].trim_height =
                    res_trim_ptr[i].trim_height;
                exp_info_ptr->sensor_mode_info[i].line_time =
                    res_trim_ptr[i].line_time;
                exp_info_ptr->sensor_mode_info[i].bps_per_lane =
                    res_trim_ptr[i].bps_per_lane;
                exp_info_ptr->sensor_mode_info[i].frame_line =
                    res_trim_ptr[i].frame_line;
            } else {
                exp_info_ptr->sensor_mode_info[i].trim_start_x = 0x00;
                exp_info_ptr->sensor_mode_info[i].trim_start_y = 0x00;
                exp_info_ptr->sensor_mode_info[i].trim_width =
                    res_info_ptr[i].width;
                exp_info_ptr->sensor_mode_info[i].trim_height =
                    res_info_ptr[i].height;
            }

            /*scaler trim*/
            if ((PNULL != res_trim_ptr) &&
                (0x00 != res_trim_ptr[i].scaler_trim.w) &&
                (0x00 != res_trim_ptr[i].scaler_trim.h)) {
                exp_info_ptr->sensor_mode_info[i].scaler_trim =
                    res_trim_ptr[i].scaler_trim;
            } else {
                exp_info_ptr->sensor_mode_info[i].scaler_trim.x = 0x00;
                exp_info_ptr->sensor_mode_info[i].scaler_trim.y = 0x00;
                exp_info_ptr->sensor_mode_info[i].scaler_trim.w =
                    exp_info_ptr->sensor_mode_info[i].trim_width;
                exp_info_ptr->sensor_mode_info[i].scaler_trim.h =
                    exp_info_ptr->sensor_mode_info[i].trim_height;
            }

            if (SENSOR_IMAGE_FORMAT_MAX != sns_info->image_format) {
                exp_info_ptr->sensor_mode_info[i].image_format =
                    sns_info->image_format;
            } else {
                exp_info_ptr->sensor_mode_info[i].image_format =
                    res_info_ptr[i].image_format;
            }
        } else {
            exp_info_ptr->sensor_mode_info[i].mode = SENSOR_MODE_MAX;
        }

        if (PNULL != sns_info->video_tab_info_ptr) {
            video_info_ptr = &sns_info->video_tab_info_ptr[i];
            if (PNULL != video_info_ptr) {
                cmr_copy((void *)&exp_info_ptr->sensor_video_info[i],
                         (void *)video_info_ptr, sizeof(SENSOR_VIDEO_INFO_T));
            }
        }

        if ((NULL != exp_info_ptr->raw_info_ptr) &&
            (NULL != exp_info_ptr->raw_info_ptr->resolution_info_ptr)) {
            exp_info_ptr->raw_info_ptr->resolution_info_ptr->tab[i].start_x =
                exp_info_ptr->sensor_mode_info[i].trim_start_x;
            exp_info_ptr->raw_info_ptr->resolution_info_ptr->tab[i].start_y =
                exp_info_ptr->sensor_mode_info[i].trim_start_y;
            exp_info_ptr->raw_info_ptr->resolution_info_ptr->tab[i].width =
                exp_info_ptr->sensor_mode_info[i].trim_width;
            exp_info_ptr->raw_info_ptr->resolution_info_ptr->tab[i].height =
                exp_info_ptr->sensor_mode_info[i].trim_height;
            exp_info_ptr->raw_info_ptr->resolution_info_ptr->tab[i].line_time =
                exp_info_ptr->sensor_mode_info[i].line_time;
            exp_info_ptr->raw_info_ptr->resolution_info_ptr->tab[i].frame_line =
                exp_info_ptr->sensor_mode_info[i].frame_line;
        }

        if ((NULL != exp_info_ptr->raw_info_ptr) &&
            (NULL != exp_info_ptr->raw_info_ptr->ioctrl_ptr)) {
            struct sensor_raw_ioctrl *ioctl =
                exp_info_ptr->raw_info_ptr->ioctrl_ptr;
            ioctl->caller_handler = sensor_cxt;
            ioctl->set_focus = sensor_af_set_pos;
            ioctl->get_pos = sensor_af_get_pos_info;
            ioctl->set_exposure = sensor_ic_write_ae_value;
            ioctl->set_gain = sensor_ic_write_gain;
            ioctl->ext_fuc = NULL;
#if defined(CONFIG_CAMERA_ISP_DIR_2_1) | defined(CONFIG_CAMERA_ISP_DIR_2_4)
            if (module && module->af_dev_info.af_drv_entry) {
                ioctl->set_pos = sensor_af_set_pos;
                ioctl->get_otp = NULL;
                ioctl->get_motor_pos = sensor_af_get_pos;
                ioctl->set_motor_bestmode = NULL;
                ioctl->get_test_vcm_mode = NULL;
                ioctl->set_test_vcm_mode = NULL;
            } else {
                ioctl->set_pos = NULL;
                ioctl->get_motor_pos = NULL;
                CMR_LOGE("AF device driver has problem,please double check "
                         "it.module:%p,af_dev:%p",
                         module,
                         module ? module->af_dev_info.af_drv_entry : NULL);
            }
#endif
#ifdef SBS_MODE_SENSOR
            char value1[PROPERTY_VALUE_MAX];
            property_get("persist.camera.sbs.mode", value1, "0");
            if (!strcmp(value1, "slave")) {
                ioctl->set_pos = NULL;
                ioctl->get_motor_pos = NULL;
                ioctl->set_focus = NULL;
            }
#endif
            ioctl->write_i2c = sensor_hw_write_i2c;
            // exp_info_ptr->raw_info_ptr->ioctrl_ptr->read_i2c =
            // Sensor_ReadI2C;
            ioctl->ex_set_exposure = sensor_ic_ex_write_exposure;
            ioctl->read_aec_info = sensor_ic_read_aec_info;
            ioctl->write_aec_info = sensor_muti_i2c_write;
            ioctl->sns_ioctl = sensor_drv_ioctl;
        }
        // now we think sensor output width and height are equal to sensor
        // trim_width and trim_height
        exp_info_ptr->sensor_mode_info[i].out_width =
            exp_info_ptr->sensor_mode_info[i].trim_width;
        exp_info_ptr->sensor_mode_info[i].out_height =
            exp_info_ptr->sensor_mode_info[i].trim_height;
    }
    if (mod_cfg_info) {
        exp_info_ptr->sensor_interface = mod_cfg_info->sensor_interface;
        exp_info_ptr->change_setting_skip_num =
            mod_cfg_info->change_setting_skip_num;
        exp_info_ptr->horizontal_view_angle =
            mod_cfg_info->horizontal_view_angle;
        exp_info_ptr->vertical_view_angle = mod_cfg_info->vertical_view_angle;
        exp_info_ptr->sensor_version_info = sns_info->sensor_version_info;
    }
    SENSOR_LOGI("X");
}

LOCAL void sensor_clean_info(struct sensor_drv_context *sensor_cxt) {
    SENSOR_REGISTER_INFO_T_PTR sensor_register_info_ptr = PNULL;

    SENSOR_DRV_CHECK_ZERO_VOID(sensor_cxt);
    sensor_register_info_ptr = &sensor_cxt->sensor_register_info;
    sensor_cxt->sensor_mode[SENSOR_MAIN] = SENSOR_MODE_MAX;
    sensor_cxt->sensor_mode[SENSOR_SUB] = SENSOR_MODE_MAX;
    sensor_cxt->sensor_mode[SENSOR_DEVICE2] = SENSOR_MODE_MAX;
    sensor_cxt->sensor_mode[SENSOR_DEVICE3] = SENSOR_MODE_MAX;
    sensor_cxt->sensor_info_ptr = PNULL;
    sensor_cxt->sensor_isInit = SENSOR_FALSE;
    sensor_cxt->sensor_index[SENSOR_MAIN] = 0xFF;
    sensor_cxt->sensor_index[SENSOR_SUB] = 0xFF;
    sensor_cxt->sensor_index[SENSOR_DEVICE2] = 0xFF;
    sensor_cxt->sensor_index[SENSOR_DEVICE3] = 0xFF;
    sensor_cxt->sensor_index[SENSOR_ATV] = 0xFF;
    SENSOR_MEMSET(&sensor_cxt->sensor_exp_info, 0x00,
                  sizeof(SENSOR_EXP_INFO_T));
    sensor_register_info_ptr->cur_id = SENSOR_ID_MAX;
    return;
}

LOCAL cmr_int sensor_set_id(struct sensor_drv_context *sensor_cxt,
                            SENSOR_ID_E sensor_id) {
    SENSOR_REGISTER_INFO_T_PTR sensor_register_info_ptr = PNULL;

    sensor_register_info_ptr = &sensor_cxt->sensor_register_info;
    sensor_register_info_ptr->cur_id = sensor_id;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    SENSOR_LOGI("id %d,is_register_sensor %ld", sensor_id,
                sensor_cxt->is_register_sensor);
#if 0
    if (1 == sensor_cxt->is_register_sensor) {
        if ((SENSOR_MAIN == sensor_id) && (1 == sensor_cxt->is_main_sensor))
            return SENSOR_SUCCESS;
        if ((SENSOR_SUB == sensor_id) && (0 == sensor_cxt->is_main_sensor))
            return SENSOR_SUCCESS;
        if ((SENSOR_DEVICE2 == sensor_id) && (1 == sensor_cxt->is_main2_sensor))
            return SENSOR_SUCCESS;
        if ((SENSOR_DEVICE3 == sensor_id) && (0 == sensor_cxt->is_main2_sensor))
            return SENSOR_SUCCESS;
    }
#endif
    if ((SENSOR_MAIN <= sensor_id) || (SENSOR_DEVICE3 >= sensor_id)) {
        if (SENSOR_SUB == sensor_id) {
            if ((1 == sensor_cxt->is_register_sensor) &&
                (1 == sensor_cxt->is_main_sensor)) {
                hw_sensor_i2c_deinit(sensor_cxt->hw_drv_handle, SENSOR_MAIN);
            }
            sensor_cxt->is_main_sensor = 0;
        } else if (SENSOR_MAIN == sensor_id) {
            if ((1 == sensor_cxt->is_register_sensor) &&
                (0 == sensor_cxt->is_main_sensor)) {
                hw_sensor_i2c_deinit(sensor_cxt->hw_drv_handle, SENSOR_SUB);
            }
            sensor_cxt->is_main_sensor = 1;
        } else if (SENSOR_DEVICE3 == sensor_id) {
            if ((1 == sensor_cxt->is_register_sensor) &&
                (1 == sensor_cxt->is_main2_sensor)) {
                hw_sensor_i2c_deinit(sensor_cxt->hw_drv_handle, SENSOR_DEVICE2);
            }
            sensor_cxt->is_main2_sensor = 0;
        } else if (SENSOR_DEVICE2 == sensor_id) {
            if ((1 == sensor_cxt->is_register_sensor) &&
                (0 == sensor_cxt->is_main2_sensor)) {
                hw_sensor_i2c_deinit(sensor_cxt->hw_drv_handle, SENSOR_DEVICE3);
            }
            sensor_cxt->is_main2_sensor = 1;
        }
        sensor_cxt->is_register_sensor = 0;
        if (hw_sensor_i2c_init(sensor_cxt->hw_drv_handle, sensor_id)) {
            if (SENSOR_MAIN == sensor_id) {
                sensor_cxt->is_main_sensor = 0;
            } else if (SENSOR_DEVICE2 == sensor_id) {
                sensor_cxt->is_main2_sensor = 0;
            }
            SENSOR_LOGE("add I2C error");
            return SENSOR_FAIL;
        } else {
            SENSOR_LOGV("add I2C OK.");
            sensor_cxt->is_register_sensor = 1;
        }
    }

    return SENSOR_SUCCESS;
}

LOCAL SENSOR_ID_E sensor_get_cur_id(struct sensor_drv_context *sensor_cxt) {
    SENSOR_REGISTER_INFO_T_PTR sensor_register_info_ptr = PNULL;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    sensor_register_info_ptr = &sensor_cxt->sensor_register_info;
    SENSOR_LOGV("id %d", sensor_register_info_ptr->cur_id);
    return (SENSOR_ID_E)sensor_register_info_ptr->cur_id;
}

LOCAL void sensor_i2c_init(struct sensor_drv_context *sensor_cxt,
                           SENSOR_ID_E sensor_id) {
    SENSOR_REGISTER_INFO_T_PTR sensor_register_info_ptr = PNULL;

    sensor_register_info_ptr = PNULL;
    SENSOR_INFO_T **sensor_info_tab_ptr = PNULL;
    SENSOR_INFO_T *sensor_info_ptr = PNULL;
    cmr_u32 i2c_clock = 100000;
    cmr_u32 set_i2c_clock = 0;

    SENSOR_DRV_CHECK_ZERO_VOID(sensor_cxt);
    sensor_register_info_ptr = &sensor_cxt->sensor_register_info;
    sensor_register_info_ptr->cur_id = sensor_id;

    if (hw_sensor_i2c_init(sensor_cxt->hw_drv_handle, sensor_id)) {
        SENSOR_LOGE("SENSOR: add I2C driver error");
        return;
    } else {
        SENSOR_LOGI("SENSOR: add I2C driver OK");
        sensor_cxt->is_register_sensor = 1;
    }

    SENSOR_LOGI("sensor_id=%d, is_register_sensor=%ld", sensor_id,
                sensor_cxt->is_register_sensor);
}

LOCAL cmr_int sensor_i2c_deinit(struct sensor_drv_context *sensor_cxt,
                                SENSOR_ID_E sensor_id) {
    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    if (1 == sensor_cxt->is_register_sensor) {
        if ((SENSOR_MAIN <= sensor_id) || (SENSOR_DEVICE3 >= sensor_id)) {
            hw_sensor_i2c_deinit(sensor_cxt->hw_drv_handle, sensor_id);
            sensor_cxt->is_register_sensor = 0;
            SENSOR_LOGI("delete I2C %d driver OK", sensor_id);
        }
    } else {
        SENSOR_LOGI("delete I2C %d driver OK", SENSOR_ID_MAX);
    }

    return SENSOR_SUCCESS;
}

LOCAL cmr_u32 sensor_identify_search(struct sensor_drv_context *sensor_cxt,
                                     SENSOR_ID_E sensor_id) {
    cmr_u32 sensor_index = 0;
    SENSOR_MATCH_T *module_tab = PNULL;
    cmr_u32 tab_length = 0x00;
    SENSOR_INFO_T *sensor_info_ptr = PNULL;
    cmr_u32 retValue = SENSOR_FAIL;
    SENSOR_REGISTER_INFO_T_PTR register_info =
        &sensor_cxt->sensor_register_info;

    SENSOR_LOGI("search all sensor...");
    module_tab = sensor_get_module_tab(sensor_cxt->is_autotest, sensor_id);
    tab_length = sensor_get_tab_length(sensor_cxt->is_autotest, sensor_id) -
                 SENSOR_ONE_I2C;

    /* search the sensor in the table*/
    for (sensor_index = 0x00; sensor_index < tab_length; sensor_index++) {
        sensor_info_ptr = module_tab[sensor_index].sensor_info;
        SENSOR_LOGI("index:%d", sensor_index);
        if (NULL == sensor_info_ptr) {
            SENSOR_LOGW("%d info of Sensor table %d is null", sensor_index,
                        sensor_id);
            continue;
        }

        sensor_cxt->sensor_info_ptr = sensor_info_ptr;
        sensor_cxt->current_module = (void *)&module_tab[sensor_index];
        sensor_cxt->module_list[sensor_id] = (void *)&module_tab[sensor_index];

        retValue = sensor_ic_identify(sensor_cxt, sensor_id);
        if (SENSOR_SUCCESS == retValue) {
            if (SENSOR_ATV != sensor_get_cur_id(sensor_cxt))
                sensor_cxt->sensor_index[sensor_id] = sensor_index;
            break;
        }
    }
    if (SCI_TRUE == register_info->is_register[sensor_id]) {
        SENSOR_LOGI("SENSOR TYPE of %d indentify OK", (cmr_u32)sensor_id);
        sensor_cxt->sensor_param_saved = SCI_TRUE;
    } else {
        SENSOR_LOGI("SENSOR TYPE of %d indentify failed!", (cmr_u32)sensor_id);
    }
    //------------------------------------------------------------------------
    return retValue;
}

LOCAL cmr_u32 sensor_identify_strsearch(struct sensor_drv_context *sensor_cxt,
                                        SENSOR_ID_E sensor_id) {
    cmr_u32 sensor_index = 0;
    SENSOR_INFO_T *sensor_info_ptr = PNULL;
    cmr_u32 retValue = SENSOR_FAIL;
    SENSOR_MATCH_T *module_tab = PNULL;
    SENSOR_REGISTER_INFO_T_PTR register_info =
        &sensor_cxt->sensor_register_info;

    sensor_index = sensor_get_match_index(sensor_cxt->is_autotest, sensor_id);
    if (sensor_index == 0xFF) {
        return retValue;
    }

    module_tab = sensor_get_module_tab(sensor_cxt->is_autotest, sensor_id);
    sensor_info_ptr = module_tab[sensor_index].sensor_info;

    if (NULL == sensor_info_ptr) {
        SENSOR_LOGE("%d info of Sensor table %d is null", sensor_index,
                    sensor_id);
        return retValue;
    }
    sensor_cxt->sensor_info_ptr = sensor_info_ptr;
    sensor_cxt->current_module = (void *)&module_tab[sensor_index];
    sensor_cxt->module_list[sensor_id] = (void *)&module_tab[sensor_index];

    retValue = sensor_ic_identify(sensor_cxt, sensor_id);
    if (SENSOR_SUCCESS == retValue) {
        if (SENSOR_ATV != sensor_get_cur_id(sensor_cxt))
            sensor_cxt->sensor_index[sensor_id] = sensor_index;
    }

    if (SCI_TRUE == register_info->is_register[sensor_id]) {
        SENSOR_LOGI("SENSOR TYPE of %d indentify OK", (cmr_u32)sensor_id);
        sensor_cxt->sensor_param_saved = SCI_TRUE;
    } else {
        SENSOR_LOGI("SENSOR TYPE of %d indentify failed!", (cmr_u32)sensor_id);
    }

    return retValue;
}

// sensor_get_ma
static cmr_int sensor_get_match_info(struct sensor_drv_context *sensor_cxt,
                                     cmr_u32 sensor_id) {
    cmr_u32 ret = SENSOR_FAIL;
    cmr_u32 drv_idx = 0;
    SENSOR_REGISTER_INFO_T_PTR register_info = PNULL;
    SENSOR_MATCH_T *module_tab = PNULL;
    SENSOR_INFO_T *sns_info_ptr = PNULL;
    cmr_u32 tab_length = 0x00;

    SENSOR_LOGI("sensor identifing %d", sensor_id);
    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    register_info = &sensor_cxt->sensor_register_info;

    if( sensor_id >= SENSOR_ATV) {
        SENSOR_LOGI("invalid id=%d",sensor_id);
        return SENSOR_FAIL;
    }
    if (SCI_TRUE == register_info->is_register[sensor_id]) {
        SENSOR_LOGI("drv has loaded");
        return SENSOR_SUCCESS;
    }

    drv_idx = sensor_cxt->sensor_index[sensor_id];
    if (0xFF == drv_idx) {
        SENSOR_LOGE("invalid drv_idx=%d", drv_idx);
        return SENSOR_FAIL;
    }

    module_tab = sensor_get_module_tab(0, sensor_id);
    tab_length = sensor_get_tab_length(0, sensor_id);
    SENSOR_LOGI("sensor_id=%d, tab_length=%d, drv_idx=%d",
                sensor_id,tab_length,drv_idx);
    if (NULL == module_tab || tab_length == 0 ||drv_idx >= tab_length) {
        SENSOR_LOGE("no table, id=%d, idx=%d",sensor_id,drv_idx);
        return SENSOR_FAIL;
    }

    sns_info_ptr = module_tab[drv_idx].sensor_info;
    if (NULL == sns_info_ptr) {
        SENSOR_LOGW("no sns drv, id=%d,idx=%d", sensor_id, drv_idx);
        return SENSOR_FAIL;
    }

    sensor_cxt->sensor_list_ptr[sensor_id] = sns_info_ptr;
    sensor_cxt->module_list[sensor_id] = (void *)&module_tab[drv_idx];
    sensor_cxt->sensor_info_ptr = sns_info_ptr;
    sensor_cxt->current_module = (void *)&module_tab[drv_idx];

    return SENSOR_SUCCESS;
}

static cmr_int sensor_get_module_cfg_info(struct sensor_drv_context *sensor_cxt,
                                          cmr_u32 sensor_id,
                                          struct module_cfg_info **cfg_info) {
    cmr_int ret = SENSOR_FAIL;
    cmr_u32 i = 0, tab_size = 0;
    SENSOR_MATCH_T *module_info = NULL;
    struct sensor_module_info *mod_cfg_tab = NULL;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    SENSOR_DRV_CHECK_ZERO(sensor_cxt->sensor_info_ptr);

    tab_size = sensor_cxt->sensor_info_ptr->module_info_tab_size;
    mod_cfg_tab = sensor_cxt->sensor_info_ptr->module_info_tab;
    module_info = sensor_cxt->current_module;
    SENSOR_LOGI("p1:%p,p2:%p", module_info, mod_cfg_tab);

    if (mod_cfg_tab && module_info) {
        SENSOR_LOGI("tab_size:%d,%d,%d", tab_size, sizeof(mod_cfg_tab),
                    sizeof(mod_cfg_tab[0]));
        // tab_size = 3;
        for (i = 0; i < tab_size; i++) {
            *cfg_info = &mod_cfg_tab[i].module_info;
            SENSOR_LOGI("mid1:%d,mid2:%d,index:%d", module_info->module_id,
                        mod_cfg_tab[i].module_id, i);
            if (mod_cfg_tab[i].module_id == module_info->module_id) {
                return SENSOR_SUCCESS;
            }
        }
    } else {
        SENSOR_LOGE("ERROR:module info or configure info is NULL\n");
    }

    return ret;
}
/**
 * NOTE: when you start a identify transaction processing
 *       you must restore runtime context.
 *
 *   sensor_cxt->sensor_info_ptr = sensor_cxt->sensor_list_ptr[sensor_id];
 *   sensor_cxt->current_module = sensor_cxt->module_list[sensor_id];
 *
 **/
static cmr_int sensor_ic_identify(struct sensor_drv_context *sensor_cxt,
                                  cmr_u32 sensor_id) {
/*  
steps:
1. create the obj of sensor IC driver.
1. configure I2C bus, sensor ID, I2C clock, slave addr, reg addr lenth, data length.
2. power on sensor IC.
3. identify the PID & VID of the sensor IC.
4. power off sensor IC.
5. destroy the obj of sensor IC driver.
*/

    cmr_int ret = SENSOR_SUCCESS;
    struct hw_drv_cfg_param hw_drv_cfg;
    struct sensor_ic_ops *sns_ops = PNULL;
    struct module_cfg_info *mod_cfg_info = PNULL;
    struct sensor_ic_drv_init_para sns_init_para;
    SENSOR_REGISTER_INFO_T_PTR register_info = PNULL;

    SENSOR_DRV_CHECK_PTR(sensor_cxt);
    SENSOR_DRV_CHECK_PTR(sensor_cxt->sensor_info_ptr);
    register_info = &sensor_cxt->sensor_register_info;
    sns_ops = sensor_cxt->sensor_info_ptr->sns_ops;

    ret = sensor_get_module_cfg_info(sensor_cxt, sensor_id, &mod_cfg_info);
    if (SENSOR_SUCCESS != ret) {
        SENSOR_LOGE("get module config info failed");
        return ret;
    }
    sensor_cxt->i2c_addr = mod_cfg_info->major_i2c_addr;
    /* create sensor ic handle */
    ret = sensor_ic_create(sensor_cxt, sensor_id);
    if (SENSOR_SUCCESS != ret) {
        SENSOR_LOGE("sensor ic handle create failed!");
        sensor_ic_delete(sensor_cxt);
        return ret;
    }

try: /*sensor has backup addr*/
        if (sns_ops && sns_ops->identify) {

            /*init i2c*/
            hw_drv_cfg.i2c_bus_config = mod_cfg_info->reg_addr_value_bits;
            hw_sensor_drv_cfg(sensor_cxt->hw_drv_handle, &hw_drv_cfg);
            sensor_i2c_init(sensor_cxt, sensor_id);
            SENSOR_LOGI("i2c_addr:0x%x", sensor_cxt->i2c_addr);
            hw_sensor_i2c_set_addr(sensor_cxt->hw_drv_handle,
                                   sensor_cxt->i2c_addr);
            hw_sensor_i2c_set_clk(sensor_cxt->hw_drv_handle);

            /*Make sure kernel get match i2c-client*/
            sensor_set_id(sensor_cxt, sensor_id);
            sensor_set_status(sensor_cxt, sensor_id);
            sensor_power_on(sensor_cxt, SCI_TRUE); /*power on*/

            ret = sns_ops->identify(sensor_cxt->sns_ic_drv_handle,
                                    SENSOR_ZERO_I2C);
            if (SENSOR_SUCCESS == ret) {
                /**if the following is SCI_FALSE,that is,now is in identify
                 *process
                 * should delete sensor ic handle
                 **/
                if (register_info->is_register[sensor_id] != SCI_TRUE) {
                    sensor_power_on(sensor_cxt, SCI_FALSE);
                    sensor_i2c_deinit(sensor_cxt, sensor_id);
                    sensor_ic_delete(sensor_cxt);
                }
                sensor_cxt->sensor_list_ptr[sensor_id] =
                    sensor_cxt->sensor_info_ptr;
                register_info->is_register[sensor_id] = SCI_TRUE;
                register_info->img_sensor_num++;
            } else {
                // register_info->is_register[sensor_id] = SCI_FALSE;
                sensor_power_on(sensor_cxt, SCI_FALSE);
                if ((sensor_cxt->i2c_addr != mod_cfg_info->minor_i2c_addr) &&
                    mod_cfg_info->minor_i2c_addr != 0x00) {
                    sensor_cxt->i2c_addr = mod_cfg_info->minor_i2c_addr;
                    SENSOR_LOGI("use backup i2c address,try again!");
                    goto try
                        ;
                }
                SENSOR_LOGI("identify failed!");
                sensor_ic_delete(sensor_cxt);
                return SENSOR_FAIL;
            }
        }

    return ret;
}

LOCAL cmr_int sensor_identify(struct sensor_drv_context *sensor_cxt,
                              SENSOR_ID_E sensor_id) {
    cmr_int ret = SENSOR_SUCCESS;
    cmr_u32 sensor_index = 0;
    SENSOR_INFO_T *sensor_info_ptr = PNULL;
    cmr_u32 retValue = SCI_FALSE;
    SENSOR_REGISTER_INFO_T_PTR register_info = PNULL;

    SENSOR_LOGI("sensor identifing %d", sensor_id);
    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    register_info = &sensor_cxt->sensor_register_info;

    /*when use the camera vendor functions, the sensor_cxt should be set at
     * first */
    sensor_set_cxt_common(sensor_cxt);

    // if already identified
    if (SCI_TRUE == register_info->is_register[sensor_id]) {
        SENSOR_LOGI("sensor identified");
        return SCI_TRUE;
    }

    if (sensor_cxt->sensor_identified && (SENSOR_ATV != sensor_id)) {
        ret = sensor_get_match_info(sensor_cxt, sensor_id);
        if (SENSOR_SUCCESS != ret) { // NULL
            // sensor_i2c_deinit(sensor_cxt, sensor_id);
            goto IDENTIFY_SEARCH;
        }
        ret = sensor_ic_identify(sensor_cxt, sensor_id);
        if (SENSOR_SUCCESS != ret) { // NULL
            goto IDENTIFY_SEARCH;
        }
    }

IDENTIFY_SEARCH:
    if (sensor_cxt->sensor_identified == SCI_FALSE &&
        ((strlen(CAMERA_SENSOR_TYPE_BACK) != 0) ||
         (strlen(CAMERA_SENSOR_TYPE_FRONT) != 0) ||
         (strlen(CAMERA_SENSOR_TYPE_BACK_EXT) != 0))) {
        retValue = sensor_identify_strsearch(sensor_cxt, sensor_id);
        if (retValue == SENSOR_SUCCESS) {
            return retValue;
        }
    }
    retValue = sensor_identify_search(sensor_cxt, sensor_id);
    return retValue;
}

static void sensor_set_status(struct sensor_drv_context *sensor_cxt,
                              SENSOR_ID_E sensor_id) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_u32 i = 0, ret = 0;
    cmr_u32 rst_lvl = 0;
    SENSOR_REGISTER_INFO_T_PTR register_info = PNULL;
    struct module_cfg_info mod_cfg_info;
    SENSOR_INFO_T *sensor_info_ptr = NULL;

    SENSOR_DRV_CHECK_ZERO_VOID(sensor_cxt);
    register_info = &sensor_cxt->sensor_register_info;

    /*pwdn all the sensor to avoid confilct as the sensor output*/
    SENSOR_LOGV("1");
    for (i = 0; i <= SENSOR_DEVICE3; i++) {
        if (i == sensor_id) {
            continue;
        }
        if (SENSOR_TRUE == register_info->is_register[i]) {
            sensor_set_id(sensor_cxt, i);
            sensor_info_ptr = sensor_cxt->sensor_list_ptr[i];
            sensor_set_cxt_common(sensor_cxt);
#ifdef CONFIG_CAMERA_RT_REFOCUS
            if (i == SENSOR_SUB) {
                hw_sensor_set_reset_level(
                    sensor_cxt->hw_drv_handle,
                    (cmr_u32)sensor_info_ptr->reset_pulse_level);
                usleep(10 * 1000);
                SENSOR_LOGI("Sensor_sleep of id %d", i);
            }
#endif
        }
    }
    sensor_set_id(sensor_cxt, sensor_id); /**/

    ATRACE_END();
}

// cmr_int sensor_register(struct sensor_drv_context *sensor_cxt,
cmr_int sns_load_drv(struct sensor_drv_context *sensor_cxt,
                        cmr_u32 sensor_id) {
    ATRACE_BEGIN(__FUNCTION__);
    cmr_int ret = SENSOR_SUCCESS;

    // cmr_u32 sensor_index = 0;
    // SENSOR_MATCH_T *module_tab = PNULL;
    // cmr_u32 tab_length = 0x00;
    // SENSOR_INFO_T *sensor_info_ptr = PNULL;
    SENSOR_REGISTER_INFO_T_PTR register_info = PNULL;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    register_info = &sensor_cxt->sensor_register_info;

    if (sensor_cxt->sensor_identified) {
        ret = sensor_get_match_info(sensor_cxt, sensor_id);
        SENSOR_LOGI("ret:%ld,sensor_id:%d", ret, sensor_id);
        if (SENSOR_SUCCESS != ret) {
            register_info->is_register[sensor_id] = SCI_FALSE;
            return ret;
        }
        register_info->is_register[sensor_id] = SCI_TRUE;
        register_info->img_sensor_num++;
    }

    ATRACE_END();
    return ret;
}

LOCAL cmr_int sensor_load_idx_inf_file(struct sensor_drv_context *sensor_cxt) {
    FILE *fp;
    cmr_u8 sensor_idx[SENSOR_PARAM_NUM];
    cmr_u32 len = 0;
    cmr_u32 i;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    cmr_bzero(sensor_idx, SENSOR_PARAM_NUM);

    /* open and read the sensor idx file.*/
    fp = fopen(SENSOR_PARA, "rb+");
    if (NULL == fp) {
        fp = fopen(SENSOR_PARA, "wb+");
        if (NULL == fp) {
            SENSOR_LOGE(" failed to open file %s open error:%s ", SENSOR_PARA, strerror(errno));
        }
    } else {
        len = fread(sensor_idx, 1, SENSOR_PARAM_NUM, fp);
        SENSOR_LOGI("rd sns idx file, len %d, 8Bytes:%x,%x,%x,%x;%x,%x,%x,%x ", len,
                    sensor_idx[0], sensor_idx[1], sensor_idx[2],
                    sensor_idx[3], sensor_idx[4], sensor_idx[5],
                    sensor_idx[6], sensor_idx[7]);
    }
    if (NULL != fp)
        fclose(fp);

    /* check the mark and parser the idx data. */
    /*    sensor_parser_idx_mark(sensor_cxt, sensor_param); */
    if ((SIGN_0 == sensor_idx[0]) && (SIGN_1 == sensor_idx[1]) &&
        (SIGN_2 == sensor_idx[2]) && (SIGN_3 == sensor_idx[3])) {
        sensor_cxt->sensor_identified = SCI_TRUE;
        for (i = 0; i < 4; i++) {
            sensor_cxt->sensor_index[i] = sensor_idx[4 + i];
        }
    } else {
        for (i = 0; i < 4; i++) {
            /* 0xff means invalid idx.*/
            sensor_cxt->sensor_index[i] = 0xff;
        }
        sensor_cxt->sensor_identified = SCI_FALSE;
    }
    return SENSOR_SUCCESS;

}

LOCAL cmr_int sensor_save_idx_inf_file(struct sensor_drv_context *sensor_cxt) {

    FILE *fp;
    cmr_u8 idx_with_mark[SENSOR_PARAM_NUM];
    cmr_u32 i;
    cmr_u8 *id_mark = idx_with_mark;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    cmr_bzero(idx_with_mark, sizeof(idx_with_mark));

    if (sensor_cxt->sensor_param_saved) {
        SENSOR_LOGI("index is %d,%d.", sensor_cxt->sensor_index[SENSOR_MAIN],
                                       sensor_cxt->sensor_index[SENSOR_SUB]);
        /* generate the idx mark and idx data. */
        //sensor_generate_idx_mark(sensor_cxt, sns_idx_with_mark);
        *id_mark++ = SIGN_0;
        *id_mark++ = SIGN_1;
        *id_mark++ = SIGN_2;
        *id_mark++ = SIGN_3;
        for (i = 0; i < 4; i++) {
            *id_mark++ = sensor_cxt->sensor_index[i];
        }

        fp = fopen(SENSOR_PARA, "wb+");
        if (NULL == fp) {
            SENSOR_LOGE(" failed to open file %s open error:%s ", SENSOR_PARA, strerror(errno));
        } else {
            fwrite(idx_with_mark, 1, SENSOR_PARAM_NUM, fp);
            fclose(fp);
        }
    }
    return SENSOR_SUCCESS;
}

cmr_int Sensor_set_calibration(cmr_u32 value) {
    struct sensor_drv_context *sensor_cxt =
        (struct sensor_drv_context *)sensor_get_dev_cxt();
    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    sensor_cxt->is_calibration = value;
    return SENSOR_SUCCESS;
}

void _sensor_calil_lnc_param_recover(struct sensor_drv_context *sensor_cxt,
                                     SENSOR_INFO_T *sensor_info_ptr) {
#if !(defined(CONFIG_CAMERA_ISP_VERSION_V3) ||                                 \
      defined(CONFIG_CAMERA_ISP_VERSION_V4))
    cmr_u32 i = 0;
    cmr_u32 index = 0;
    cmr_u32 length = 0;
    cmr_uint addr = 0;
    struct sensor_raw_fix_info *raw_fix_info_ptr = PNULL;
    struct sensor_raw_info *raw_info_ptr = PNULL;

    raw_info_ptr = (struct sensor_raw_info *)(*(sensor_info_ptr->raw_info_ptr));

    SENSOR_DRV_CHECK_ZERO_VOID(sensor_cxt);

    if (PNULL != raw_info_ptr) {
        raw_fix_info_ptr = raw_info_ptr->fix_ptr;
        if (PNULL != raw_fix_info_ptr) {
            for (i = 0; i < 8; i++) {
                if (sensor_cxt->lnc_addr_bakup[i][1]) {

                    free((void *)sensor_cxt->lnc_addr_bakup[i][1]);
                    sensor_cxt->lnc_addr_bakup[i][1] = 0;
                    index = sensor_cxt->lnc_addr_bakup[i][0];  /*index*/
                    length = sensor_cxt->lnc_addr_bakup[i][3]; /*length*/
                    addr =
                        sensor_cxt->lnc_addr_bakup[i][2]; /*original address*/

                    raw_fix_info_ptr->lnc.map[index][0].param_addr =
                        (cmr_u16 *)addr;
                    raw_fix_info_ptr->lnc.map[index][0].len = length;
                    sensor_cxt->lnc_addr_bakup[i][0] = 0;
                    sensor_cxt->lnc_addr_bakup[i][1] = 0;
                    sensor_cxt->lnc_addr_bakup[i][2] = 0;
                    sensor_cxt->lnc_addr_bakup[i][3] = 0;
                }
            }
        } else {
            for (i = 0; i < 8; i++) {
                if (sensor_cxt->lnc_addr_bakup[i][1]) {
                    free((void *)sensor_cxt->lnc_addr_bakup[i][1]);
                }
            }
            cmr_bzero((void *)&sensor_cxt->lnc_addr_bakup[0][0],
                      sizeof(sensor_cxt->lnc_addr_bakup));
        }
    } else {
        for (i = 0; i < 8; i++) {
            if (sensor_cxt->lnc_addr_bakup[i][1]) {
                free((void *)sensor_cxt->lnc_addr_bakup[i][1]);
            }
        }
        cmr_bzero((void *)&sensor_cxt->lnc_addr_bakup[0][0],
                  sizeof(sensor_cxt->lnc_addr_bakup));
    }

    sensor_cxt->is_calibration = 0;
    SENSOR_LOGI("test: is_calibration: %d", sensor_cxt->is_calibration);
#endif
}

cmr_int _sensor_cali_lnc_param_update(struct sensor_drv_context *sensor_cxt,
                                      cmr_s8 *cfg_file_dir,
                                      SENSOR_INFO_T *sensor_info_ptr,
                                      SENSOR_ID_E sensor_id) {
    UNUSED(sensor_id);
    cmr_u32 rtn = SENSOR_SUCCESS;
#if !(defined(CONFIG_CAMERA_ISP_VERSION_V3) ||                                 \
      defined(CONFIG_CAMERA_ISP_VERSION_V4))
    const cmr_s8 *sensor_name = sensor_info_ptr->name;
    FILE *fp = PNULL;
    cmr_s8 file_name[80] = {0};
    cmr_s8 *file_name_ptr = 0;
    cmr_u32 str_len = 0;
    cmr_int file_pos = 0;
    cmr_u32 file_size = 0;
    cmr_s8 *data_ptr;
    cmr_s32 i, j;
    cmr_u16 *temp_buf_16 = PNULL;
    cmr_u32 width;
    cmr_u32 height;
    cmr_u32 index = 0;
    struct sensor_trim_tag *trim_ptr = PNULL;
    struct sensor_ic_ops *sns_ops = PNULL;
    struct sensor_raw_fix_info *raw_fix_info_ptr = PNULL;

    if (SENSOR_IMAGE_FORMAT_RAW != sensor_info_ptr->image_format) {
        rtn = SENSOR_FAIL;
        goto cali_lnc_param_update_exit;
    }

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    if (PNULL == sensor_cxt->sensor_info_ptr) {
        SENSOR_LOGE("No sensor info!");
        return -1;
    }
    sns_ops = sensor_cxt->sensor_info_ptr->sns_ops;
    str_len = sprintf(file_name, "%ssensor_%s", cfg_file_dir, sensor_name);
    file_name_ptr = (cmr_s8 *)&file_name[0] + str_len;

    /*LNC DATA Table*/
    temp_buf_16 = (cmr_u16 *)malloc(128 * 1024 * 2);
    if (!temp_buf_16) {
        rtn = SENSOR_FAIL;
        goto cali_lnc_param_update_exit;
    }

    trim_ptr = sensor_ic_get_data(sensor_cxt, SENSOR_CMD_GET_TRIM_TAB);
    raw_fix_info_ptr =
        ((struct sensor_raw_info *)(*(sensor_info_ptr->raw_info_ptr)))->fix_ptr;
    i = 1;
    while (1) {
        height = trim_ptr[i].trim_height;
        width = trim_ptr[i].trim_width;
        if ((0 == height) || (0 == width)) {
            break;
        }

        sprintf(file_name_ptr, "_lnc_%d_%d_%d_rdm.dat", width, height, (i - 1));

        fp = fopen(file_name, "rb");
        if (0 == fp) {
            SENSOR_LOGW("does not find calibration file");
            i++;
            continue;
        }

        fseek(fp, 0L, SEEK_END);
        file_pos = ftell(fp);
        if (file_pos >= 0) {
            file_size = (cmr_u32)file_pos;
        } else {
            fclose(fp);
            free(temp_buf_16);
            temp_buf_16 = NULL;
            SENSOR_LOGE("file pointers error!");
            rtn = SENSOR_FAIL;
            goto cali_lnc_param_update_exit;
        }
        fseek(fp, 0L, SEEK_SET);

        fread(temp_buf_16, 1, file_size, fp);
        fclose(fp);

        if (file_size != raw_fix_info_ptr->lnc.map[i - 1][0].len) {
            SENSOR_LOGI("file size dis-match, do not replace, w:%d, h:%d, ori: "
                        "%d, now:%d/n",
                        width, height, raw_fix_info_ptr->lnc.map[i - 1][0].len,
                        file_size);
        } else {
            if (sensor_cxt->lnc_addr_bakup[index][1]) {
                free((void *)sensor_cxt->lnc_addr_bakup[index][1]);
                sensor_cxt->lnc_addr_bakup[index][1] = 0;
            }
            sensor_cxt->lnc_addr_bakup[index][1] = (cmr_uint)malloc(file_size);
            if (0 == sensor_cxt->lnc_addr_bakup[index][1]) {
                rtn = SENSOR_FAIL;
                SENSOR_LOGW("malloc failed i = %d", i);
                goto cali_lnc_param_update_exit;
            }
            cmr_bzero((void *)sensor_cxt->lnc_addr_bakup[index][1], file_size);

            sensor_cxt->lnc_addr_bakup[index][0] = i - 1;
            sensor_cxt->lnc_addr_bakup[index][2] =
                (cmr_uint)raw_fix_info_ptr->lnc.map[i - 1][0]
                    .param_addr; /*save the original address*/
            sensor_cxt->lnc_addr_bakup[index][3] = file_size;
            data_ptr = (cmr_s8 *)sensor_cxt->lnc_addr_bakup[index][1];
            raw_fix_info_ptr->lnc.map[i - 1][0].param_addr =
                (cmr_u16 *)data_ptr;
            cmr_copy(data_ptr, temp_buf_16, file_size);
            index++;
            SENSOR_LOGI("replace finished");
        }
        i++;
    }

    if (temp_buf_16) {
        free((void *)temp_buf_16);
        temp_buf_16 = 0;
    }
    return rtn;

cali_lnc_param_update_exit:

    if (temp_buf_16) {
        free((void *)temp_buf_16);
        temp_buf_16 = 0;
    }

    _sensor_calil_lnc_param_recover(sensor_cxt, sensor_info_ptr);
#endif
    return rtn;
}

cmr_int _sensor_cali_awb_param_update(cmr_s8 *cfg_file_dir,
                                      SENSOR_INFO_T *sensor_info_ptr,
                                      SENSOR_ID_E sensor_id) {
    UNUSED(sensor_id);

    cmr_int rtn = 0;
#if !(defined(CONFIG_CAMERA_ISP_VERSION_V3) ||                                 \
      defined(CONFIG_CAMERA_ISP_VERSION_V4))
    const cmr_s8 *sensor_name = sensor_info_ptr->name;
    FILE *fp = PNULL;
    cmr_s8 file_name[80] = {0};
    cmr_s8 buf[256] = {0x00};
    cmr_s8 *file_name_ptr = 0;
    cmr_u32 str_len = 0;
    cmr_int file_pos = 0;
    cmr_u32 file_size = 0;
    struct isp_bayer_ptn_stat_t *stat_ptr = PNULL;
    struct sensor_cali_info *cali_info_ptr = PNULL;
    struct sensor_raw_tune_info *raw_tune_info_ptr = PNULL;

    if (SENSOR_IMAGE_FORMAT_RAW != sensor_info_ptr->image_format) {
        return SENSOR_FAIL;
    }
    raw_tune_info_ptr =
        (struct sensor_raw_tune_info *)(((struct sensor_raw_info *)(*(
                                             sensor_info_ptr->raw_info_ptr)))
                                            ->tune_ptr);
    cali_info_ptr = (struct sensor_cali_info *)&(
        ((struct sensor_raw_info *)(*(sensor_info_ptr->raw_info_ptr)))
            ->cali_ptr->awb.cali_info);

    str_len = sprintf(file_name, "%ssensor_%s", cfg_file_dir, sensor_name);
    file_name_ptr = (cmr_s8 *)&file_name[0] + str_len;

    sprintf(file_name_ptr, "_awb_rdm.dat");

    SENSOR_LOGI("file_name: %s", file_name);
    fp = fopen(file_name, "rb");
    if (0 == fp) {
        SENSOR_LOGI("does not find calibration file");

        cali_info_ptr->r_sum = 1024;
        cali_info_ptr->b_sum = 1024;
        cali_info_ptr->gr_sum = 1024;
        cali_info_ptr->gb_sum = 1024;

        cali_info_ptr = (struct sensor_cali_info *)&(
            ((struct sensor_raw_info *)(*(sensor_info_ptr->raw_info_ptr)))
                ->cali_ptr->awb.golden_cali_info);
        cali_info_ptr->r_sum = 1024;
        cali_info_ptr->b_sum = 1024;
        cali_info_ptr->gr_sum = 1024;
        cali_info_ptr->gb_sum = 1024;
        rtn = SENSOR_SUCCESS;
        return rtn;
    } else {
        fseek(fp, 0L, SEEK_END);
        file_pos = ftell(fp);
        if (file_pos >= 0) {
            file_size = (cmr_u32)file_pos;
        } else {
            fclose(fp);
            SENSOR_LOGE("file pointers error!");
            return SENSOR_FAIL;
        }

        fseek(fp, 0L, SEEK_SET);
        fread(buf, 1, file_size, fp);
        fclose(fp);

        stat_ptr = (struct isp_bayer_ptn_stat_t *)&buf[0];
        cali_info_ptr->r_sum = stat_ptr->r_stat;
        cali_info_ptr->b_sum = stat_ptr->b_stat;
        cali_info_ptr->gr_sum = stat_ptr->gr_stat;
        cali_info_ptr->gb_sum = stat_ptr->gb_stat;

        rtn = SENSOR_SUCCESS;
    }

    cmr_bzero(&file_name[0], sizeof(file_name));
    cmr_bzero(&buf[0], sizeof(buf));
    str_len = sprintf(file_name, "%ssensor_%s", cfg_file_dir, sensor_name);
    file_name_ptr = (cmr_s8 *)&file_name[0] + str_len;

    sprintf(file_name_ptr, "_awb_gldn.dat");

    SENSOR_LOGI("file_name: %s", file_name);
    cali_info_ptr = (struct sensor_cali_info *)&(
        ((struct sensor_raw_info *)(*(sensor_info_ptr->raw_info_ptr)))
            ->cali_ptr->awb.golden_cali_info);
    fp = fopen(file_name, "rb");
    if (0 == fp) {
        SENSOR_LOGI("does not find calibration file");

        cali_info_ptr->r_sum = 1024;
        cali_info_ptr->b_sum = 1024;
        cali_info_ptr->gr_sum = 1024;
        cali_info_ptr->gb_sum = 1024;

        cali_info_ptr = (struct sensor_cali_info *)&(
            ((struct sensor_raw_info *)(*(sensor_info_ptr->raw_info_ptr)))
                ->cali_ptr->awb.cali_info);
        cali_info_ptr->r_sum = 1024;
        cali_info_ptr->b_sum = 1024;
        cali_info_ptr->gr_sum = 1024;
        cali_info_ptr->gb_sum = 1024;

        rtn = SENSOR_SUCCESS;
        return rtn;
    } else {
        fseek(fp, 0L, SEEK_END);
        file_pos = ftell(fp);
        if (file_pos >= 0) {
            file_size = (cmr_u32)file_pos;
        } else {
            fclose(fp);
            SENSOR_LOGE("file pointers error!");
            return SENSOR_FAIL;
        }
        fseek(fp, 0L, SEEK_SET);

        fread(buf, 1, file_size, fp);
        fclose(fp);

        stat_ptr = (struct isp_bayer_ptn_stat_t *)&buf[0];
        cali_info_ptr->r_sum = stat_ptr->r_stat;
        cali_info_ptr->b_sum = stat_ptr->b_stat;
        cali_info_ptr->gr_sum = stat_ptr->gr_stat;
        cali_info_ptr->gb_sum = stat_ptr->gb_stat;

        rtn = SENSOR_SUCCESS;
    }
#endif
    return rtn;
}

cmr_int _sensor_cali_flashlight_param_update(cmr_s8 *cfg_file_dir,
                                             SENSOR_INFO_T *sensor_info_ptr,
                                             SENSOR_ID_E sensor_id) {
    UNUSED(sensor_id);

    cmr_int rtn = 0;
#if !(defined(CONFIG_CAMERA_ISP_VERSION_V3) ||                                 \
      defined(CONFIG_CAMERA_ISP_VERSION_V4))
    const cmr_s8 *sensor_name = sensor_info_ptr->name;
    FILE *fp = PNULL;
    cmr_s8 file_name[80] = {0};
    cmr_s8 buf[256] = {0x00};
    cmr_s8 *file_name_ptr = 0;
    cmr_u32 str_len = 0;
    cmr_int file_pos = 0;
    cmr_u32 file_size = 0;
    struct isp_bayer_ptn_stat_t *stat_ptr = PNULL;
    struct sensor_cali_info *cali_info_ptr = PNULL;
    struct sensor_raw_tune_info *raw_tune_info_ptr = PNULL;

    if (SENSOR_IMAGE_FORMAT_RAW != sensor_info_ptr->image_format) {
        return SENSOR_FAIL;
    }
    raw_tune_info_ptr =
        (struct sensor_raw_tune_info *)(((struct sensor_raw_info *)(*(
                                             sensor_info_ptr->raw_info_ptr)))
                                            ->tune_ptr);
    cali_info_ptr = (struct sensor_cali_info *)&(
        ((struct sensor_raw_info *)(*(sensor_info_ptr->raw_info_ptr)))
            ->cali_ptr->flashlight.cali_info);

    str_len = sprintf(file_name, "%ssensor_%s", cfg_file_dir, sensor_name);
    file_name_ptr = (cmr_s8 *)&file_name[0] + str_len;

    sprintf(file_name_ptr, "_flashlight_rdm.dat");

    SENSOR_LOGI("file_name: %s", file_name);
    fp = fopen(file_name, "rb");
    if (0 == fp) {
        SENSOR_LOGI("does not find calibration file");

        cali_info_ptr->r_sum = 1024;
        cali_info_ptr->b_sum = 1024;
        cali_info_ptr->gr_sum = 1024;
        cali_info_ptr->gb_sum = 1024;

        cali_info_ptr = (struct sensor_cali_info *)&(
            ((struct sensor_raw_info *)(*(sensor_info_ptr->raw_info_ptr)))
                ->cali_ptr->flashlight.golden_cali_info);
        cali_info_ptr->r_sum = 1024;
        cali_info_ptr->b_sum = 1024;
        cali_info_ptr->gr_sum = 1024;
        cali_info_ptr->gb_sum = 1024;

        rtn = SENSOR_SUCCESS;
        return rtn;
    } else {
        fseek(fp, 0L, SEEK_END);
        file_pos = ftell(fp);
        if (file_pos >= 0) {
            file_size = (cmr_u32)file_pos;
        } else {
            fclose(fp);
            SENSOR_LOGE("file pointers error!");
            return SENSOR_FAIL;
        }
        fseek(fp, 0L, SEEK_SET);

        fread(buf, 1, file_size, fp);
        fclose(fp);

        stat_ptr = (struct isp_bayer_ptn_stat_t *)&buf[0];
        cali_info_ptr->r_sum = stat_ptr->r_stat;
        cali_info_ptr->b_sum = stat_ptr->b_stat;
        cali_info_ptr->gr_sum = stat_ptr->gr_stat;
        cali_info_ptr->gb_sum = stat_ptr->gb_stat;

        rtn = SENSOR_SUCCESS;
    }

    cmr_bzero(&file_name[0], sizeof(file_name));
    cmr_bzero(&buf[0], sizeof(buf));
    str_len = sprintf(file_name, "%ssensor_%s", cfg_file_dir, sensor_name);
    file_name_ptr = (cmr_s8 *)&file_name[0] + str_len;

    sprintf(file_name_ptr, "_flashlight_gldn.dat");

    SENSOR_LOGI("file_name: %s", file_name);
    cali_info_ptr = (struct sensor_cali_info *)&(
        ((struct sensor_raw_info *)(*(sensor_info_ptr->raw_info_ptr)))
            ->cali_ptr->flashlight.golden_cali_info);
    fp = fopen(file_name, "rb");
    if (0 == fp) {
        SENSOR_LOGI("does not find calibration file");

        cali_info_ptr->r_sum = 1024;
        cali_info_ptr->b_sum = 1024;
        cali_info_ptr->gr_sum = 1024;
        cali_info_ptr->gb_sum = 1024;

        cali_info_ptr = (struct sensor_cali_info *)&(
            ((struct sensor_raw_info *)(*(sensor_info_ptr->raw_info_ptr)))
                ->cali_ptr->flashlight.cali_info);
        cali_info_ptr->r_sum = 1024;
        cali_info_ptr->b_sum = 1024;
        cali_info_ptr->gr_sum = 1024;
        cali_info_ptr->gb_sum = 1024;

        rtn = SENSOR_SUCCESS;
        return rtn;
    } else {
        fseek(fp, 0L, SEEK_END);

        file_pos = ftell(fp);
        if (file_pos >= 0) {
            file_size = (cmr_u32)file_pos;
        } else {
            fclose(fp);
            SENSOR_LOGE("file pointers error!");
            return SENSOR_FAIL;
        }
        fseek(fp, 0L, SEEK_SET);

        fread(buf, 1, file_size, fp);
        fclose(fp);

        stat_ptr = (struct isp_bayer_ptn_stat_t *)&buf[0];
        cali_info_ptr->r_sum = stat_ptr->r_stat;
        cali_info_ptr->b_sum = stat_ptr->b_stat;
        cali_info_ptr->gr_sum = stat_ptr->gr_stat;
        cali_info_ptr->gb_sum = stat_ptr->gb_stat;

        rtn = SENSOR_SUCCESS;
    }
#endif
    return rtn;
}

cmr_int _sensor_cali_load_param(struct sensor_drv_context *sensor_cxt,
                                cmr_s8 *cfg_file_dir,
                                SENSOR_INFO_T *sensor_info_ptr,
                                SENSOR_ID_E sensor_id) {
#if !(defined(CONFIG_CAMERA_ISP_VERSION_V3) ||                                 \
      defined(CONFIG_CAMERA_ISP_VERSION_V4))
    cmr_int rtn = 0;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    if (1 != sensor_cxt->is_calibration) { /*for normal*/

        rtn = _sensor_cali_lnc_param_update(sensor_cxt, cfg_file_dir,
                                            sensor_info_ptr, sensor_id);
        if (rtn) {

            return SENSOR_FAIL;
        }
        rtn = _sensor_cali_flashlight_param_update(cfg_file_dir,
                                                   sensor_info_ptr, sensor_id);
        if (rtn) {
            return SENSOR_FAIL;
        }
        rtn = _sensor_cali_awb_param_update(cfg_file_dir, sensor_info_ptr,
                                            sensor_id);
        if (rtn) {
            return SENSOR_FAIL;
        }
    } else { /*for calibration*/
        struct sensor_cali_info *cali_info_ptr = PNULL;

        /*for awb calibration*/
        cali_info_ptr = (struct sensor_cali_info *)&(
            ((struct sensor_raw_info *)(*(sensor_info_ptr->raw_info_ptr)))
                ->cali_ptr->awb.cali_info);
        cali_info_ptr->r_sum = 1024;
        cali_info_ptr->b_sum = 1024;
        cali_info_ptr->gr_sum = 1024;
        cali_info_ptr->gb_sum = 1024;

        cali_info_ptr = (struct sensor_cali_info *)&(
            ((struct sensor_raw_info *)(*(sensor_info_ptr->raw_info_ptr)))
                ->cali_ptr->awb.golden_cali_info);
        cali_info_ptr->r_sum = 1024;
        cali_info_ptr->b_sum = 1024;
        cali_info_ptr->gr_sum = 1024;
        cali_info_ptr->gb_sum = 1024;

        /*for flash  calibration*/
        cali_info_ptr = (struct sensor_cali_info *)&(
            ((struct sensor_raw_info *)(*(sensor_info_ptr->raw_info_ptr)))
                ->cali_ptr->flashlight.cali_info);
        cali_info_ptr->r_sum = 1024;
        cali_info_ptr->b_sum = 1024;
        cali_info_ptr->gr_sum = 1024;
        cali_info_ptr->gb_sum = 1024;

        cali_info_ptr = (struct sensor_cali_info *)&(
            ((struct sensor_raw_info *)(*(sensor_info_ptr->raw_info_ptr)))
                ->cali_ptr->flashlight.golden_cali_info);
        cali_info_ptr->r_sum = 1024;
        cali_info_ptr->b_sum = 1024;
        cali_info_ptr->gr_sum = 1024;
        cali_info_ptr->gb_sum = 1024;
    }
#endif
    return SENSOR_SUCCESS;
}

LOCAL cmr_int sensor_create_ctrl_thread(struct sensor_drv_context *sensor_cxt) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);

    SENSOR_LOGI("is_inited %ld", sensor_cxt->ctrl_thread_cxt.is_inited);

    if (!sensor_cxt->ctrl_thread_cxt.is_inited) {
        pthread_mutex_init(&sensor_cxt->ctrl_thread_cxt.sensor_mutex, NULL);
        sem_init(&sensor_cxt->ctrl_thread_cxt.sensor_sync_sem, 0, 0);

        ret = cmr_thread_create(&sensor_cxt->ctrl_thread_cxt.thread_handle,
                                SENSOR_CTRL_MSG_QUEUE_SIZE,
                                sensor_ctrl_thread_proc, (void *)sensor_cxt);
        if (ret) {
            SENSOR_LOGE("send msg failed!");
            ret = CMR_CAMERA_FAIL;
            goto end;
        }

        sensor_cxt->ctrl_thread_cxt.is_inited = 1;
    }

end:
    if (ret) {
        sem_destroy(&sensor_cxt->ctrl_thread_cxt.sensor_sync_sem);
        pthread_mutex_destroy(&sensor_cxt->ctrl_thread_cxt.sensor_mutex);
        sensor_cxt->ctrl_thread_cxt.is_inited = 0;
    }

    SENSOR_LOGV("ret %ld", ret);
    return ret;
}

LOCAL cmr_int sensor_ctrl_thread_proc(struct cmr_msg *message, void *p_data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 evt = 0, on_off = 0;
    struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)p_data;
    otp_ctrl_cmd_t *otp_ctrl_data = NULL;

    if (!message || !p_data) {
        return CMR_CAMERA_INVALID_PARAM;
    }

    evt = (cmr_u32)message->msg_type;
    SENSOR_LOGI("evt %d", evt);

    switch (evt) {
    case SENSOR_CTRL_EVT_INIT:
        /*common control info config*/
        SENSOR_LOGI("INIT DONE!");
        break;

    case SENSOR_CTRL_EVT_SETMODE: {
        cmr_u32 mode = (cmr_u32)message->sub_msg_type;
        cmr_u32 is_inited = (cmr_u32)((cmr_uint)message->data);
        usleep(100);
        sensor_set_mode(sensor_cxt, mode, is_inited);
    } break;

    case SENSOR_CTRL_EVT_SETMODONE:
        SENSOR_LOGI("SENSOR_CTRL_EVT_SETMODONE OK");
        break;

    case SENSOR_CTRL_EVT_CFGOTP:
        otp_ctrl_data = (otp_ctrl_cmd_t *)message->data;
        sensor_otp_process(sensor_cxt, otp_ctrl_data->cmd,
                           otp_ctrl_data->sub_cmd, otp_ctrl_data->data);
        break;
    case SENSOR_CTRL_EVT_STREAM_CTRL:
        on_off = (cmr_u32)message->sub_msg_type;
        sensor_stream_ctrl(sensor_cxt, on_off);
        break;

    case SENSOR_CTRL_EVT_EXIT:
        /*common control info clear*/
        SENSOR_LOGI("EXIT DONE!");
        break;

    default:
        SENSOR_LOGE("jpeg:not correct message");
        break;
    }

    return ret;
}

LOCAL cmr_int
sensor_destroy_ctrl_thread(struct sensor_drv_context *sensor_cxt) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);

    SENSOR_LOGI("is_inited %ld", sensor_cxt->ctrl_thread_cxt.is_inited);

    if (sensor_cxt->ctrl_thread_cxt.is_inited) {
        message.msg_type = SENSOR_CTRL_EVT_EXIT;
        message.sync_flag = CMR_MSG_SYNC_PROCESSED;
        ret = cmr_thread_msg_send(sensor_cxt->ctrl_thread_cxt.thread_handle,
                                  &message);
        if (ret) {
            SENSOR_LOGE("send msg failed!");
        }

        ret = cmr_thread_destroy(sensor_cxt->ctrl_thread_cxt.thread_handle);
        sensor_cxt->ctrl_thread_cxt.thread_handle = 0;

        sem_destroy(&sensor_cxt->ctrl_thread_cxt.sensor_sync_sem);
        pthread_mutex_destroy(&sensor_cxt->ctrl_thread_cxt.sensor_mutex);
        sensor_cxt->ctrl_thread_cxt.is_inited = 0;
    }

    return ret;
}
/**
 *  sensor context init
 **/
cmr_int sensor_context_init(struct sensor_drv_context *sensor_cxt,
                            cmr_u32 sensor_id, cmr_uint is_autotest) {
    ATRACE_BEGIN(__FUNCTION__);
    cmr_int ret_val = SENSOR_FAIL;
    SENSOR_DRV_CHECK_ZERO(sensor_cxt);

    cmr_bzero((void *)sensor_cxt, sizeof(struct sensor_drv_context));
    sensor_init_log_level();

    sensor_cxt->fd_sensor = SENSOR_FD_INIT;
    sensor_cxt->i2c_addr = 0xff;
    sensor_cxt->is_autotest = is_autotest;

    sensor_clean_info(sensor_cxt);
    ret_val = sensor_init_defaul_exif(sensor_cxt);

    ATRACE_END();

    return ret_val;
}

cmr_int sensor_context_deinit(struct sensor_drv_context *sensor_cxt) {
    cmr_int ret_val = SENSOR_FAIL;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    sensor_destroy_ctrl_thread(sensor_cxt);

    sensor_stream_off(sensor_cxt);
    hw_sensor_mipi_deinit(sensor_cxt->hw_drv_handle);
    if (1 == sensor_cxt->is_register_sensor) {
        if (1 == sensor_cxt->is_main_sensor) {
            hw_sensor_i2c_deinit(sensor_cxt->hw_drv_handle, SENSOR_MAIN);
        } else {
            hw_sensor_i2c_deinit(sensor_cxt->hw_drv_handle, SENSOR_SUB);
        }
        if (1 == sensor_cxt->is_main2_sensor) {
            hw_sensor_i2c_deinit(sensor_cxt->hw_drv_handle, SENSOR_DEVICE2);
        } else {
            hw_sensor_i2c_deinit(sensor_cxt->hw_drv_handle, SENSOR_DEVICE3);
        }
        sensor_cxt->is_register_sensor = 0;
        sensor_cxt->is_main_sensor = 0;
        sensor_cxt->is_main2_sensor = 0;
    }

    if (SENSOR_TRUE == sensor_cxt->sensor_isInit) {
        if (SENSOR_IMAGE_FORMAT_RAW ==
            sensor_cxt->sensor_info_ptr->image_format) {
            if (0 == sensor_cxt->is_calibration) {
                _sensor_calil_lnc_param_recover(sensor_cxt,
                                                sensor_cxt->sensor_info_ptr);
            }
        }
        sensor_power_on(sensor_cxt, SENSOR_FALSE);
    }
    sensor_ic_delete(sensor_cxt);
    hw_sensor_drv_delete(sensor_cxt->hw_drv_handle);
    sensor_cxt->sensor_hw_handler = NULL;
    sensor_cxt->hw_drv_handle = NULL;
    sensor_cxt->sensor_isInit = SENSOR_FALSE;
    sensor_cxt->sensor_mode[SENSOR_MAIN] = SENSOR_MODE_MAX;
    sensor_cxt->sensor_mode[SENSOR_SUB] = SENSOR_MODE_MAX;
    sensor_cxt->sensor_mode[SENSOR_DEVICE2] = SENSOR_MODE_MAX;
    sensor_cxt->sensor_mode[SENSOR_DEVICE3] = SENSOR_MODE_MAX;
    SENSOR_LOGI("X");
    return ret_val;
}

/**
 * NOTE:when open camera,you should do the following steps.
   1.register all sensor for the following first open sensor.
   2.if first open sensor failed, identify module list to save sensor index.
   3.try open sensor second time.
   4.if 3 steps failed,sensor open failed.
 **/
cmr_int sensor_open_common(struct sensor_drv_context *sensor_cxt,
                           cmr_u32 sensor_id, cmr_uint is_autotest) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret_val = SENSOR_FAIL;
    cmr_u32 sensor_num = 0;

    SENSOR_LOGI("0, start,id %d autotest %ld", sensor_id, is_autotest);
    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    /* init ctx, exif, load sensor file, open hw driver with sensor id.*/
    ret_val = sensor_context_init(sensor_cxt, sensor_id, is_autotest);
    if (SENSOR_SUCCESS != ret_val) {
        /*Will never come here*/
        return ret_val;
    }
    /* create sensor process thread. */
    ret_val = sensor_create_ctrl_thread(sensor_cxt);
    if (SENSOR_SUCCESS != ret_val) {
        SENSOR_LOGE("Failed to create sensor ctrl thread");
        goto init_exit;
    }

    /* create the hw obj of kernel driver. */
    struct hw_drv_init_para input_ptr;
    cmr_int fd_sensor = SENSOR_FD_INIT;
    cmr_handle hw_drv_handle = NULL;

    input_ptr.sensor_id = sensor_id;
    input_ptr.caller_handle = sensor_cxt;
    fd_sensor = hw_sensor_drv_create(&input_ptr, &hw_drv_handle);
    if ( (SENSOR_FD_INIT == fd_sensor) || (NULL == hw_drv_handle) ) {
        SENSOR_LOGE("sns_device_init %d error, return", sensor_id);
        ret_val = SENSOR_FAIL;
        goto init_exit;
    }
    sensor_cxt->fd_sensor = fd_sensor;
    sensor_cxt->hw_drv_handle = hw_drv_handle;
    sensor_cxt->sensor_hw_handler = hw_drv_handle;

    /*load all the sensor ICs' info according the indexs stored in sensor idx file*/
    sensor_load_idx_inf_file(sensor_cxt);
    if (sensor_cxt->sensor_identified) {
        if (SENSOR_SUCCESS == sns_load_drv(sensor_cxt, SENSOR_MAIN)){
            sensor_num++;
        }
#ifndef CONFIG_DCAM_SENSOR_NO_FRONT_SUPPORT
        if (SENSOR_SUCCESS == sns_load_drv(sensor_cxt, SENSOR_SUB)) {
            sensor_num++;
        }
#endif
#ifdef CONFIG_DCAM_SENSOR2_SUPPORT
        if (SENSOR_SUCCESS == sns_load_drv(sensor_cxt, SENSOR_DEVICE2)) {
            sensor_num++;
        }
#endif
#ifdef CONFIG_DCAM_SENSOR3_SUPPORT
        if (SENSOR_SUCCESS == sns_load_drv(sensor_cxt, SENSOR_DEVICE3)) {
            sensor_num++;
        }
#endif
        SENSOR_LOGI("1 is identify, register OK");
        /*first open sensor*/
        ret_val = sensor_open(sensor_cxt, sensor_id);
        if (ret_val != SENSOR_SUCCESS) {
            SENSOR_LOGI("first open sensor failed,start identify");
			sensor_cxt->sensor_identified = SCI_FALSE; 
			sensor_cxt->sensor_param_saved = SCI_FALSE; 
			sensor_cxt->sensor_register_info.is_register[sensor_id] = SCI_FALSE; 
			CMR_LOGI("set sensor info false");
        }
    }

    /* scan the devices in cfg list and find out the correct sensor driver */
    if ((!sensor_cxt->sensor_identified) || (ret_val != SENSOR_SUCCESS)) {
        sensor_num = 0;
        SENSOR_LOGI("register sensor fail, start identify");
        if (sensor_identify(sensor_cxt, SENSOR_MAIN))
            sensor_num++;
#ifndef CONFIG_DCAM_SENSOR_NO_FRONT_SUPPORT
        if (sensor_identify(sensor_cxt, SENSOR_SUB))
            sensor_num++;
#endif
#ifdef CONFIG_DCAM_SENSOR2_SUPPORT
        if (sensor_identify(sensor_cxt, SENSOR_DEVICE2))
            sensor_num++;
#endif
#ifdef CONFIG_DCAM_SENSOR3_SUPPORT
        if (sensor_identify(sensor_cxt, SENSOR_DEVICE3))
            sensor_num++;
#endif
        ret_val = sensor_open(sensor_cxt, sensor_id);
    }
    sensor_cxt->sensor_identified = SCI_TRUE;

    sensor_save_idx_inf_file(sensor_cxt);
    sensor_rid_save_sensor_info(sensor_cxt);
    sensor_save_info_for_checkXtool(sensor_cxt);

    if (sensor_cxt->sensor_info_ptr &&
        SENSOR_IMAGE_FORMAT_RAW == sensor_cxt->sensor_info_ptr->image_format) {
        if (SENSOR_SUCCESS == ret_val) {
            ret_val =
                _sensor_cali_load_param(sensor_cxt, (cmr_s8 *)CALI_FILE_DIR,
                                        sensor_cxt->sensor_info_ptr, sensor_id);
            if (ret_val) {
                SENSOR_LOGW("load cali data failed!! rtn:%ld", ret_val);
                goto init_exit;
            }
        }
    }

    SENSOR_LOGI("total camera number %d", sensor_num);

init_exit:
    if (SENSOR_SUCCESS != ret_val) {
        sensor_destroy_ctrl_thread(sensor_cxt);
        hw_sensor_drv_delete(hw_drv_handle);
        sensor_cxt->hw_drv_handle = NULL;
        sensor_cxt->sensor_hw_handler = NULL;
    }

    ATRACE_END();
    return ret_val;
}

// cmr_int sensor_is_init_common(struct sensor_drv_context *sensor_cxt) {
cmr_int sensor_is_init_common(struct sensor_drv_context *sensor_cxt) {
    ATRACE_BEGIN(__FUNCTION__);
    SENSOR_DRV_CHECK_ZERO(sensor_cxt);

    ATRACE_END();
    return sensor_cxt->sensor_isInit;
}

cmr_int read_txt_file(const char *file_name, void *data) {
    FILE *pf = fopen(file_name, "r");
    uint32_t read_byte = 0;
    cmr_u8 *otp_data = (cmr_u8 *)data;

    if (NULL == pf) {
        SENSOR_LOGE("dualotp read failed!");
        goto exit;
    }

    if (NULL == otp_data) {
        SENSOR_LOGE("dualotp data malloc failed!");
        fclose(pf);
        goto exit;
    }

    while (!feof(pf)) {
        fscanf(pf, "%d\n", otp_data);
        otp_data += 4;
        read_byte += 4;
    }
    fclose(pf);
    SENSOR_LOGI("dualotp read_bytes=%d ", read_byte);

exit:
    return read_byte;
}

LOCAL cmr_int sensor_write_dualcam_otpdata(
    struct sensor_drv_context *sensor_cxt, cmr_u32 sensor_id) {

    cmr_u32 ret_val = SENSOR_FAIL;
    cmr_u16 num_byte = SPRD_DUAL_OTP_SIZE;
    char value[PROPERTY_VALUE_MAX];

    SENSOR_LOGI("write dualotp ");
    property_get("debug.dualcamera.write.otp", value, "false");
    if (!strcmp(value, "true") && (sensor_id == 0)) {
        const char *psPath_OtpData = "data/misc/cameraserver/otp.txt";

        otp_params_t pdata;
        char *dual_data = (char *)malloc(num_byte);
        int otp_ret = read_txt_file(psPath_OtpData, dual_data);
        pdata.buffer = dual_data;
        pdata.num_bytes = num_byte;
        int i = 0;
        SENSOR_LOGI("read dualotp bin,len = %d", otp_ret);
        if (otp_ret > 0) {
            int ret = sensor_otp_rw_ctrl(sensor_cxt, OTP_WRITE_DATA, 0,
                                         (void *)&pdata);
        }
        free(dual_data);
        SENSOR_LOGI("free dualotp file");
    }

    return ret_val;
}

static cmr_int sensor_open(struct sensor_drv_context *sensor_cxt,
                           cmr_u32 sensor_id) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_u32 ret_val = SENSOR_FAIL;
    cmr_u32 is_inited = 0;
    SENSOR_REGISTER_INFO_T_PTR register_info = PNULL;
    SENSOR_MATCH_T *module = NULL;
    struct sensor_ic_ops *sns_ops = PNULL;
    struct module_cfg_info *mod_cfg_info = PNULL;
    struct hw_drv_cfg_param hw_drv_cfg;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    /*restore the context*/
    sensor_cxt->sensor_info_ptr = sensor_cxt->sensor_list_ptr[sensor_id];
    sensor_cxt->current_module = sensor_cxt->module_list[sensor_id];

    register_info = &sensor_cxt->sensor_register_info;
    sns_ops = sensor_cxt->sensor_info_ptr->sns_ops;

    if (1 == sensor_cxt->is_autotest) {
        is_inited = 1;
    }

    if (SENSOR_TRUE == register_info->is_register[sensor_id]) {
        sensor_cxt->sensor_isInit = SENSOR_TRUE;

        ret_val = sensor_ic_identify(sensor_cxt, sensor_id);
        if (SENSOR_SUCCESS != ret_val) {
            sensor_cxt->sensor_isInit = SENSOR_FALSE;
            return SENSOR_FAIL;
        }

        if (SENSOR_SUCCESS != sensor_set_mode_msg(sensor_cxt,
                                                  SENSOR_MODE_COMMON_INIT,
                                                  is_inited)) {
            SENSOR_LOGE("Sensor set init mode error!");
            sensor_i2c_deinit(sensor_cxt, sensor_id);
            ret_val = SENSOR_FAIL;
        }
        /*       if (SENSOR_SUCCESS == ret_val) {
                   if (SENSOR_SUCCESS != sensor_set_mode_msg(sensor_cxt,
                                                             SENSOR_MODE_PREVIEW_ONE,
                                                             is_inited)) {
                       SENSOR_LOGE("Sensor set init mode error!");
                       sensor_i2c_deinit(sensor_cxt, sensor_id);
                       ret_val = SENSOR_FAIL;
                   }
               }*/

        sensor_af_init(sensor_cxt);
        sensor_otp_module_init(sensor_cxt);
        module = sensor_cxt->current_module;
        if ((SENSOR_IMAGE_FORMAT_RAW ==
             sensor_cxt->sensor_info_ptr->image_format) &&
            module && module->otp_drv_info) {
            /*if property debug.dualcamera.write.otp is false do nothing*/
            sensor_write_dualcam_otpdata(sensor_cxt, sensor_id);
            sensor_otp_rw_ctrl(sensor_cxt, OTP_READ_PARSE_DATA, 0, NULL);
#if 1 // def CONFIG_ISP_TUNING_PARAM_UPDATE
            sensor_otp_ops_t *otp_ops = PNULL;
            cmr_u8 vendor_id = 0;
            otp_ops = &module->otp_drv_info->otp_ops;
            otp_ops->sensor_otp_ioctl(sensor_cxt->otp_drv_handle,
                                      CMD_SNS_OTP_GET_VENDOR_ID,
                                      (void *)&vendor_id);
            sensor_set_raw_infor(sensor_cxt, vendor_id);
#endif
        } else {
            SENSOR_LOGE("don't support otp:mod:%p,otp_drv:%p", module,
                        module ? module->otp_drv_info : NULL);
        }
        sensor_set_export_Info(sensor_cxt);

        sensor_cxt->stream_on = 1;
        sensor_stream_off(sensor_cxt);
        SENSOR_LOGI("open success");
    } else {
        SENSOR_LOGE("Sensor not register, open failed, sensor_id = %d",
                    sensor_id);
    }

    if ((SENSOR_SUCCESS == ret_val) && (1 == sensor_cxt->is_autotest)) {
        sensor_set_mode_done_common(sensor_cxt);
    }

    ATRACE_END();
    return ret_val;
}

LOCAL cmr_int sensor_set_mode_msg(struct sensor_drv_context *sensor_cxt,
                                  cmr_u32 mode, cmr_u32 is_inited) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    message.msg_type = SENSOR_CTRL_EVT_SETMODE;
    message.sub_msg_type = mode;
    message.sync_flag = CMR_MSG_SYNC_NONE;
    message.data = (void *)((cmr_uint)is_inited);
    ret = cmr_thread_msg_send(sensor_cxt->ctrl_thread_cxt.thread_handle,
                              &message);
    if (ret) {
        SENSOR_LOGE("send msg failed!");
        return CMR_CAMERA_FAIL;
    }

    return ret;
}

cmr_int sensor_set_mode_done_common(cmr_handle sns_module_handle) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct sensor_drv_context *sensor_cxt =
        (struct sensor_drv_context *)sns_module_handle;
    SENSOR_DRV_CHECK_ZERO(sensor_cxt);

    /*the set mode function can be async control*/
    message.msg_type = SENSOR_CTRL_EVT_SETMODONE;
    message.sync_flag = CMR_MSG_SYNC_RECEIVED;
    ret = cmr_thread_msg_send(sensor_cxt->ctrl_thread_cxt.thread_handle,
                              &message);
    if (ret) {
        SENSOR_LOGE("send msg failed!");
        return CMR_CAMERA_FAIL;
    }
    return ret;
}

LOCAL cmr_int sensor_set_mode(struct sensor_drv_context *sensor_cxt,
                              cmr_u32 mode, cmr_u32 is_inited) {
    ATRACE_BEGIN(__FUNCTION__);
    cmr_int rtn;
    cmr_u32 mclk;

    SENSOR_REG_TAB_INFO_T *res_info_ptr = PNULL;
    struct sensor_trim_tag *res_trim_ptr = PNULL;
    struct module_cfg_info *mod_cfg_info = PNULL;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    SENSOR_DRV_CHECK_ZERO(sensor_cxt->sensor_info_ptr);

    mod_cfg_info = sensor_ic_get_data(sensor_cxt, SENSOR_CMD_GET_MODULE_CFG);
    res_trim_ptr = sensor_ic_get_data(sensor_cxt, SENSOR_CMD_GET_TRIM_TAB);
    res_info_ptr = sensor_ic_get_data(sensor_cxt, SENSOR_CMD_GET_RESOLUTION);

    SENSOR_LOGI("mode = %d.", mode);
    if (SENSOR_FALSE == sensor_cxt->sensor_isInit) {
        SENSOR_LOGE("sensor has not init");
        return SENSOR_OP_STATUS_ERR;
    }

    if (sensor_cxt->sensor_mode[sensor_get_cur_id(sensor_cxt)] == mode) {
        SENSOR_LOGI("The sensor mode as before");
    } else {
        if (PNULL != res_info_ptr[mode].sensor_reg_tab_ptr) {
            mclk = res_info_ptr[mode].xclk_to_sensor;
            hw_sensor_set_mclk(sensor_cxt->hw_drv_handle, mclk);

            hw_Sensor_SendRegTabToSensor(sensor_cxt->hw_drv_handle,
                                         &res_info_ptr[mode]);
            sensor_cxt->sensor_mode[sensor_get_cur_id(sensor_cxt)] = mode;
        } else {
            SENSOR_LOGI("No this resolution information !!!");
        }
    }
#if 0
    if (is_inited) {
        if (SENSOR_INTERFACE_TYPE_CSI2 == mod_cfg_info->sensor_interface.type)
            /*stream off first for MIPI sensor switch*/
            if (SENSOR_SUCCESS == sensor_stream_off(sensor_cxt)) {
            }
    }
#endif
    ATRACE_END();
    return SENSOR_SUCCESS;
}

cmr_int sensor_set_mode_common(cmr_handle sns_module_handle, cmr_u32 mode) {
    cmr_int ret = 0;
    struct sensor_drv_context *sensor_cxt =
        (struct sensor_drv_context *)sns_module_handle;
    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    ret = sensor_set_mode_msg(sensor_cxt, mode, 1);
    return ret;
}

cmr_int sensor_get_mode_common(cmr_handle sns_module_handle, cmr_u32 *mode) {
    struct sensor_drv_context *sensor_cxt =
        (struct sensor_drv_context *)sns_module_handle;
    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    SENSOR_DRV_CHECK_ZERO(mode);
    if (SENSOR_FALSE == sensor_cxt->sensor_isInit) {
        SENSOR_LOGE("sensor has not init");
        return SENSOR_OP_STATUS_ERR;
    }
    *mode = sensor_cxt->sensor_mode[sensor_get_cur_id(sensor_cxt)];
    return SENSOR_SUCCESS;
}

cmr_int sensor_stream_on(struct sensor_drv_context *sensor_cxt) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int err = 0xff;
    cmr_u32 param = 0;
    SENSOR_IOCTL_FUNC_PTR stream_on_func = PNULL;
    struct sensor_ic_ops *sns_ops = PNULL;

    SENSOR_LOGI("E");

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    SENSOR_DRV_CHECK_ZERO(sensor_cxt->sensor_info_ptr);

    sns_ops = sensor_cxt->sensor_info_ptr->sns_ops;
    if (sns_ops && sns_ops->ext_ops[SENSOR_IOCTL_STREAM_ON].ops) {
        stream_on_func = sns_ops->ext_ops[SENSOR_IOCTL_STREAM_ON].ops;
    } else {
        SENSOR_LOGE("get stream_on function failed!");
        return SENSOR_FAIL;
    }

    if (!sensor_cxt->sensor_isInit) {
        SENSOR_LOGE("X: sensor has not been initialized");
        return SENSOR_FAIL;
    }

    if (!sensor_cxt->stream_on) {
        param = sensor_cxt->bypass_mode;
        err = stream_on_func(sensor_cxt->sns_ic_drv_handle, param);
    }
    if (0 == err) {
        sensor_cxt->stream_on = 1;
    }
    SENSOR_LOGI("X");
    ATRACE_END();
    return err;
}

cmr_int sensor_stream_ctrl_common(struct sensor_drv_context *sensor_cxt,
                                  cmr_u32 on_off) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);

    /*the set mode function can be async control*/
    message.msg_type = SENSOR_CTRL_EVT_STREAM_CTRL;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    message.sub_msg_type = on_off;
    ret = cmr_thread_msg_send(sensor_cxt->ctrl_thread_cxt.thread_handle,
                              &message);
    if (ret) {
        SENSOR_LOGE("send msg failed!");
        return CMR_CAMERA_FAIL;
    }
    return ret;
}

LOCAL cmr_int sensor_stream_ctrl(struct sensor_drv_context *sensor_cxt,
                                 cmr_u32 on_off) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = 0;
    cmr_u32 mode;
    struct module_cfg_info *mod_cfg_info = PNULL;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    mod_cfg_info = (struct module_cfg_info *)sensor_ic_get_data(
        sensor_cxt, SENSOR_CMD_GET_MODULE_CFG);
    SENSOR_LOGI("on_off %d", on_off);
    if (on_off == 1) {
        if (SENSOR_INTERFACE_TYPE_CSI2 == mod_cfg_info->sensor_interface.type) {
            mode = sensor_cxt->sensor_mode[sensor_get_cur_id(sensor_cxt)];
            struct hw_mipi_init_param init_param;
            init_param.lane_num =
                sensor_cxt->sensor_exp_info.sensor_interface.bus_width;
            init_param.bps_per_lane =
                sensor_cxt->sensor_exp_info.sensor_mode_info[mode].bps_per_lane;
            ret = hw_sensor_mipi_init(sensor_cxt->hw_drv_handle, init_param);
            if (ret) {
                SENSOR_LOGE("mipi initial failed ret %ld", ret);
            } else {
                SENSOR_LOGI("mipi initial ok");
            }
        }
        ret = sensor_stream_on(sensor_cxt);
    } else if (on_off == 2) {
        if (SENSOR_SUCCESS == ret) {
            if (SENSOR_INTERFACE_TYPE_CSI2 ==
                mod_cfg_info->sensor_interface.type) {
                struct hw_mipi_init_param init_param;
                mode = sensor_cxt->sensor_mode[sensor_get_cur_id(sensor_cxt)];
                init_param.lane_num =
                    sensor_cxt->sensor_exp_info.sensor_interface.bus_width;
                init_param.bps_per_lane =
                    sensor_cxt->sensor_exp_info.sensor_mode_info[mode]
                        .bps_per_lane;
                ret = hw_sensor_mipi_switch(sensor_cxt->hw_drv_handle,
                                            init_param);
            }
        }
    } else {
        ret = sensor_stream_off(sensor_cxt);
        if (SENSOR_SUCCESS == ret) {
            if (SENSOR_INTERFACE_TYPE_CSI2 ==
                mod_cfg_info->sensor_interface.type) {
                hw_sensor_mipi_deinit(sensor_cxt->hw_drv_handle);
                mode = sensor_cxt->sensor_mode[sensor_get_cur_id(sensor_cxt)];
            }
        }
    }

    ATRACE_END();
    return ret;
}

cmr_int sensor_stream_off(struct sensor_drv_context *sensor_cxt) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int err = 0xff;
    cmr_u32 param = 0;
    SENSOR_IOCTL_FUNC_PTR stream_off_func = PNULL;
    struct sensor_ic_ops *sns_ops = PNULL;

    SENSOR_LOGI("E");
    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    SENSOR_DRV_CHECK_ZERO(sensor_cxt->sensor_info_ptr);

    sns_ops = sensor_cxt->sensor_info_ptr->sns_ops;
    if (sns_ops && sns_ops->ext_ops[SENSOR_IOCTL_STREAM_OFF].ops) {
        stream_off_func = sns_ops->ext_ops[SENSOR_IOCTL_STREAM_OFF].ops;
    } else {
        SENSOR_LOGE("get stream_off function failed!");
        return SENSOR_FAIL;
    }

    if (!sensor_cxt->sensor_isInit) {
        SENSOR_LOGE("X: sensor has not been initialized");
        return SENSOR_FAIL;
    }

    /*when use the camera vendor functions, the sensor_cxt should be set at
     * first */
    sensor_set_cxt_common(sensor_cxt);

    if (PNULL != stream_off_func && sensor_cxt->stream_on == 1) {
        param = sensor_cxt->bypass_mode;
        err = stream_off_func(sensor_cxt->sns_ic_drv_handle, param);
    }
    sensor_cxt->stream_on = 0;

    SENSOR_LOGI("X");
    ATRACE_END();
    return err;
}

cmr_int sensor_get_info_common(struct sensor_drv_context *sensor_cxt,
                               SENSOR_EXP_INFO_T **sensor_exp_info_pptr) {
    if (PNULL == sensor_cxt) {
        SENSOR_LOGE("zero pointer ");
        return SENSOR_FAIL;
    }
    if (!sensor_cxt->sensor_isInit) {
        SENSOR_LOGE("sensor has not init");
        return SENSOR_FAIL;
    }

    SENSOR_LOGV("info = %p ", (void *)&sensor_cxt->sensor_exp_info);
    *sensor_exp_info_pptr = &sensor_cxt->sensor_exp_info;
    return SENSOR_SUCCESS;
}

SENSOR_EXP_INFO_T *Sensor_GetInfo(void) {
    struct sensor_drv_context *sensor_cxt =
        (struct sensor_drv_context *)sensor_get_dev_cxt();

    if (PNULL == sensor_cxt) {
        SENSOR_LOGE("zero pointer ");
        return PNULL;
    }
    if (!sensor_cxt->sensor_isInit) {
        SENSOR_LOGE("sensor has not init");
        return PNULL;
    }

    SENSOR_LOGI("info=%p ", (void *)&sensor_cxt->sensor_exp_info);
    return &sensor_cxt->sensor_exp_info;
}

SENSOR_EXP_INFO_T *Sensor_GetInfo_withid(cmr_u32 id) {
    struct sensor_drv_context *sensor_cxt =
        (struct sensor_drv_context *)sensor_get_dev_cxt_Ex(id);

    if (PNULL == sensor_cxt) {
        SENSOR_LOGE("zero pointer ");
        return PNULL;
    }
    if (!sensor_is_init_common(sensor_cxt)) {
        SENSOR_LOGI("sensor has not init");
        return PNULL;
    }

    SENSOR_LOGI("info=%lx ", (void *)&sensor_cxt->sensor_exp_info);
    return &sensor_cxt->sensor_exp_info;
}

/*********************************************************************************
 todo:
 now the sensor_close_common only support close all sensor, and the function
 should be
 updated when the hardware can supported multiple sensors;
 *********************************************************************************/
cmr_int sensor_close_common(struct sensor_drv_context *sensor_cxt,
                            cmr_u32 sensor_id) {
    ATRACE_BEGIN(__FUNCTION__);
    UNUSED(sensor_id);
    // SENSOR_REGISTER_INFO_T_PTR sensor_register_info_ptr = PNULL;

    SENSOR_LOGI("E");
    SENSOR_DRV_CHECK_ZERO(sensor_cxt);

    // sensor_register_info_ptr = &sensor_cxt->sensor_register_info;
    sensor_otp_module_deinit(sensor_cxt);
    sensor_af_deinit(sensor_cxt);
    sensor_context_deinit(sensor_cxt);

    ATRACE_END();
    return SENSOR_SUCCESS;
}

LOCAL cmr_int sensor_init_defaul_exif(struct sensor_drv_context *sensor_cxt) {
    EXIF_SPEC_PIC_TAKING_COND_T *exif_ptr = PNULL;
    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    exif_ptr = &sensor_cxt->default_exif;
    cmr_bzero(&sensor_cxt->default_exif, sizeof(EXIF_SPEC_PIC_TAKING_COND_T));

    SENSOR_LOGV("E");

    exif_ptr->valid.FNumber = 1;
    exif_ptr->FNumber.numerator = 14;
    exif_ptr->FNumber.denominator = 5;
    exif_ptr->valid.ExposureProgram = 1;
    exif_ptr->ExposureProgram = 0x04;
    exif_ptr->valid.ApertureValue = 1;
    exif_ptr->ApertureValue.numerator = 14;
    exif_ptr->ApertureValue.denominator = 5;
    exif_ptr->valid.MaxApertureValue = 1;
    exif_ptr->MaxApertureValue.numerator = 14;
    exif_ptr->MaxApertureValue.denominator = 5;
    exif_ptr->valid.FocalLength = 1;
    exif_ptr->FocalLength.numerator = 289;
    exif_ptr->FocalLength.denominator = 100;
    exif_ptr->valid.FileSource = 1;
    exif_ptr->FileSource = 0x03;
    exif_ptr->valid.ExposureMode = 1;
    exif_ptr->ExposureMode = 0x00;
    exif_ptr->valid.WhiteBalance = 1;
    exif_ptr->WhiteBalance = 0x00;
    exif_ptr->valid.Flash = 1;

    SENSOR_LOGV("X");
    return SENSOR_SUCCESS;
}

cmr_int sensor_set_exif_common(cmr_handle sns_module_handle, cmr_u32 cmdin,
                               cmr_u32 param) {
    struct sensor_drv_context *sensor_cxt =
        (struct sensor_drv_context *)sns_module_handle;
    SENSOR_EXIF_CTRL_E cmd = (SENSOR_EXIF_CTRL_E)cmdin;
    SENSOR_EXP_INFO_T_PTR sensor_info_ptr = NULL;
    EXIF_SPEC_PIC_TAKING_COND_T *sensor_exif_info_ptr = PNULL;
    void *exif_ptr = PNULL;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);

    sensor_get_info_common(sensor_cxt, &sensor_info_ptr);
    SENSOR_DRV_CHECK_ZERO(sensor_info_ptr);

    exif_ptr = (EXIF_SPEC_PIC_TAKING_COND_T *)sensor_ic_get_data(
        sensor_cxt, SENSOR_CMD_GET_EXIF);
    if (exif_ptr) {
        sensor_exif_info_ptr = (EXIF_SPEC_PIC_TAKING_COND_T *)exif_ptr;
    } else {
        SENSOR_LOGV("the fun is null, set it to default");
        sensor_exif_info_ptr = &sensor_cxt->default_exif;
    }

    switch (cmd) {
    case SENSOR_EXIF_CTRL_EXPOSURETIME: {
        enum sensor_mode img_sensor_mode =
            sensor_cxt->sensor_mode[sensor_get_cur_id(sensor_cxt)];
        cmr_u32 exposureline_time =
            sensor_info_ptr->sensor_mode_info[img_sensor_mode].line_time;
        cmr_u32 exposureline_num = param;
        cmr_u32 exposure_time = 0x00;
        cmr_int regen_exposure_time = 0x00;
        cmr_int orig_exposure_time = 0x00;

        exposure_time = exposureline_time * exposureline_num / 1000;
        orig_exposure_time = exposureline_time * exposureline_num;
        sensor_exif_info_ptr->valid.ExposureTime = 1;

        if (0x00 == exposure_time) {
            sensor_exif_info_ptr->valid.ExposureTime = 0;
        } else if (1000000 >= exposure_time) {
            sensor_exif_info_ptr->ExposureTime.numerator = 0x01;
            sensor_exif_info_ptr->ExposureTime.denominator =
                (1000000.00 / exposure_time + 0.5);
        } else {
            cmr_u32 second = 0x00;
            do {
                second++;
                exposure_time -= 1000000;
                if (1000000 >= exposure_time) {
                    break;
                }
            } while (1);
            sensor_exif_info_ptr->ExposureTime.denominator =
                1000000 / exposure_time;
            sensor_exif_info_ptr->ExposureTime.numerator =
                sensor_exif_info_ptr->ExposureTime.denominator * second;
        }
        regen_exposure_time = 1000000000ll *
                              sensor_exif_info_ptr->ExposureTime.numerator /
                              sensor_exif_info_ptr->ExposureTime.denominator;
// To check within range of CTS
        if (((orig_exposure_time - regen_exposure_time) > 100000) ||
            ((regen_exposure_time - orig_exposure_time) > 100000)) {
            sensor_exif_info_ptr->ExposureTime.denominator =
                (1000000.00 / exposure_time) * 1000 + 0.5;
            sensor_exif_info_ptr->ExposureTime.numerator = 1000;
        }
        break;
    }
    case SENSOR_EXIF_CTRL_FNUMBER:
        sensor_exif_info_ptr->valid.FNumber = 1;
        sensor_exif_info_ptr->FNumber.numerator = param;
        sensor_exif_info_ptr->FNumber.denominator = 10;
        break;
        break;
    case SENSOR_EXIF_CTRL_EXPOSUREPROGRAM:
        break;
    case SENSOR_EXIF_CTRL_SPECTRALSENSITIVITY:
        break;
    case SENSOR_EXIF_CTRL_ISOSPEEDRATINGS:
        if (param == 6) {
            /*6 = CAMERA_ISO_MAX*/
            sensor_exif_info_ptr->valid.ISOSpeedRatings = 0;
        } else {
            sensor_exif_info_ptr->valid.ISOSpeedRatings = 1;
        }
        sensor_exif_info_ptr->ISOSpeedRatings.count = 1;
        sensor_exif_info_ptr->ISOSpeedRatings.type = EXIF_SHORT;
        sensor_exif_info_ptr->ISOSpeedRatings.size = 2;
        cmr_copy((void *)&sensor_exif_info_ptr->ISOSpeedRatings.ptr[0],
                 (void *)&param, 2);
        break;
    case SENSOR_EXIF_CTRL_OECF:
        break;
    case SENSOR_EXIF_CTRL_SHUTTERSPEEDVALUE:
        break;
    case SENSOR_EXIF_CTRL_APERTUREVALUE:
        sensor_exif_info_ptr->valid.ApertureValue = 1;
        sensor_exif_info_ptr->ApertureValue.numerator = param;
        sensor_exif_info_ptr->ApertureValue.denominator = 10;
        break;
    case SENSOR_EXIF_CTRL_BRIGHTNESSVALUE: {
        sensor_exif_info_ptr->valid.BrightnessValue = 1;
        switch (param) {
        case 0:
        case 1:
        case 2:
            sensor_exif_info_ptr->BrightnessValue.numerator = 1;
            sensor_exif_info_ptr->BrightnessValue.denominator = 1;
            break;
        case 3:
            sensor_exif_info_ptr->BrightnessValue.numerator = 0;
            sensor_exif_info_ptr->BrightnessValue.denominator = 0;
            break;
        case 4:
        case 5:
        case 6:
            sensor_exif_info_ptr->BrightnessValue.numerator = 2;
            sensor_exif_info_ptr->BrightnessValue.denominator = 2;
            break;
        default:
            sensor_exif_info_ptr->BrightnessValue.numerator = 0xff;
            sensor_exif_info_ptr->BrightnessValue.denominator = 0xff;
            break;
        }
        break;
    } break;
    case SENSOR_EXIF_CTRL_EXPOSUREBIASVALUE:
        break;
    case SENSOR_EXIF_CTRL_MAXAPERTUREVALUE:
        sensor_exif_info_ptr->valid.MaxApertureValue = 1;
        sensor_exif_info_ptr->MaxApertureValue.numerator = param;
        sensor_exif_info_ptr->MaxApertureValue.denominator = 10;
        break;
    case SENSOR_EXIF_CTRL_SUBJECTDISTANCE:
        break;
    case SENSOR_EXIF_CTRL_METERINGMODE:
        break;
    case SENSOR_EXIF_CTRL_LIGHTSOURCE: {
        sensor_exif_info_ptr->valid.LightSource = 1;
        switch (param) {
        case 0:
            sensor_exif_info_ptr->LightSource = 0x00;
            break;
        case 1:
            sensor_exif_info_ptr->LightSource = 0x03;
            break;
        case 2:
            sensor_exif_info_ptr->LightSource = 0x0f;
            break;
        case 3:
            sensor_exif_info_ptr->LightSource = 0x0e;
            break;
        case 4:
            sensor_exif_info_ptr->LightSource = 0x02;
            break;
        case 5:
            sensor_exif_info_ptr->LightSource = 0x01;
            break;
        case 6:
            sensor_exif_info_ptr->LightSource = 0x0a;
            break;
        default:
            sensor_exif_info_ptr->LightSource = 0xff;
            break;
        }
        break;
    }
    case SENSOR_EXIF_CTRL_FLASH:
        sensor_exif_info_ptr->valid.Flash = 1;
        sensor_exif_info_ptr->Flash = param;
        break;
    case SENSOR_EXIF_CTRL_FOCALLENGTH:
        break;
    case SENSOR_EXIF_CTRL_SUBJECTAREA:
        break;
    case SENSOR_EXIF_CTRL_FLASHENERGY:
        break;
    case SENSOR_EXIF_CTRL_SPATIALFREQUENCYRESPONSE:
        break;
    case SENSOR_EXIF_CTRL_FOCALPLANEXRESOLUTION:
        break;
    case SENSOR_EXIF_CTRL_FOCALPLANEYRESOLUTION:
        break;
    case SENSOR_EXIF_CTRL_FOCALPLANERESOLUTIONUNIT:
        break;
    case SENSOR_EXIF_CTRL_SUBJECTLOCATION:
        break;
    case SENSOR_EXIF_CTRL_EXPOSUREINDEX:
        break;
    case SENSOR_EXIF_CTRL_SENSINGMETHOD:
        break;
    case SENSOR_EXIF_CTRL_FILESOURCE:
        break;
    case SENSOR_EXIF_CTRL_SCENETYPE:
        break;
    case SENSOR_EXIF_CTRL_CFAPATTERN:
        break;
    case SENSOR_EXIF_CTRL_CUSTOMRENDERED:
        break;
    case SENSOR_EXIF_CTRL_EXPOSUREMODE:
        break;

    case SENSOR_EXIF_CTRL_WHITEBALANCE:
        sensor_exif_info_ptr->valid.WhiteBalance = 1;
        if (param)
            sensor_exif_info_ptr->WhiteBalance = 1;
        else
            sensor_exif_info_ptr->WhiteBalance = 0;
        break;

    case SENSOR_EXIF_CTRL_DIGITALZOOMRATIO:
        break;
    case SENSOR_EXIF_CTRL_FOCALLENGTHIN35MMFILM:
        break;
    case SENSOR_EXIF_CTRL_SCENECAPTURETYPE: {
        sensor_exif_info_ptr->valid.SceneCaptureType = 1;
        switch (param) {
        case 0:
            sensor_exif_info_ptr->SceneCaptureType = 0x00;
            break;
        case 1:
            sensor_exif_info_ptr->SceneCaptureType = 0x03;
            break;
        default:
            sensor_exif_info_ptr->LightSource = 0xff;
            break;
        }
        break;
    }
    case SENSOR_EXIF_CTRL_GAINCONTROL:
        break;
    case SENSOR_EXIF_CTRL_CONTRAST: {
        sensor_exif_info_ptr->valid.Contrast = 1;
        switch (param) {
        case 0:
        case 1:
        case 2:
            sensor_exif_info_ptr->Contrast = 0x01;
            break;
        case 3:
            sensor_exif_info_ptr->Contrast = 0x00;
            break;
        case 4:
        case 5:
        case 6:
            sensor_exif_info_ptr->Contrast = 0x02;
            break;
        default:
            sensor_exif_info_ptr->Contrast = 0xff;
            break;
        }
        break;
    }
    case SENSOR_EXIF_CTRL_SATURATION: {
        sensor_exif_info_ptr->valid.Saturation = 1;
        switch (param) {
        case 0:
        case 1:
        case 2:
            sensor_exif_info_ptr->Saturation = 0x01;
            break;
        case 3:
            sensor_exif_info_ptr->Saturation = 0x00;
            break;
        case 4:
        case 5:
        case 6:
            sensor_exif_info_ptr->Saturation = 0x02;
            break;
        default:
            sensor_exif_info_ptr->Saturation = 0xff;
            break;
        }
        break;
    }
    case SENSOR_EXIF_CTRL_SHARPNESS: {
        sensor_exif_info_ptr->valid.Sharpness = 1;
        switch (param) {
        case 0:
        case 1:
        case 2:
            sensor_exif_info_ptr->Sharpness = 0x01;
            break;
        case 3:
            sensor_exif_info_ptr->Sharpness = 0x00;
            break;
        case 4:
        case 5:
        case 6:
            sensor_exif_info_ptr->Sharpness = 0x02;
            break;
        default:
            sensor_exif_info_ptr->Sharpness = 0xff;
            break;
        }
        break;
    }
    case SENSOR_EXIF_CTRL_DEVICESETTINGDESCRIPTION:
        break;
    case SENSOR_EXIF_CTRL_SUBJECTDISTANCERANGE:
        break;
    default:
        break;
    }
    return SENSOR_SUCCESS;
}

cmr_int sensor_update_isparm_from_file(struct sensor_drv_context *sensor_cxt,
                                       cmr_u32 sensor_id) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    if (!sensor_cxt) {
        return CMR_CAMERA_FAIL;
    }

    ret = isp_raw_para_update_from_file(sensor_cxt->sensor_info_ptr, sensor_id);
    return ret;
}

cmr_int sensor_get_exif_common(cmr_handle sns_module_handle, void **param) {
    struct sensor_ic_ops *sns_ops = PNULL;
    void *exif_ptr = PNULL;
    struct sensor_drv_context *sensor_cxt =
        (struct sensor_drv_context *)sns_module_handle;
    EXIF_SPEC_PIC_TAKING_COND_T **sensor_exif_info_pptr =
        (EXIF_SPEC_PIC_TAKING_COND_T **)param;
    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    SENSOR_DRV_CHECK_ZERO(sensor_exif_info_pptr);

    exif_ptr = (EXIF_SPEC_PIC_TAKING_COND_T *)sensor_ic_get_data(
        sensor_cxt, SENSOR_CMD_GET_EXIF);
    if (exif_ptr) {
        *sensor_exif_info_pptr = (EXIF_SPEC_PIC_TAKING_COND_T *)exif_ptr;
    } else {
        SENSOR_LOGV("the fun is null, set it to default");
        *sensor_exif_info_pptr = &sensor_cxt->default_exif;
    }
    return SENSOR_SUCCESS;
}


cmr_int sensor_set_raw_infor(struct sensor_drv_context *sensor_cxt,
                             cmr_u8 vendor_id) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    SENSOR_VAL_T val;
    struct sensor_ic_ops *sns_ops = PNULL;
    cmr_u32 sns_cmd = SENSOR_IOCTL_ACCESS_VAL;
    val.type = SENSOR_VAL_TYPE_SET_RAW_INFOR;
    val.pval = &vendor_id;

    SENSOR_LOGI("vendor id %x\n", vendor_id);
    sns_ops = sensor_cxt->sensor_info_ptr->sns_ops;
    if (sns_ops)
        ret = sns_ops->ext_ops[sns_cmd].ops(sensor_cxt->sns_ic_drv_handle,
                                            (cmr_uint)&val);
    return ret;
}

LOCAL cmr_int sensor_otp_rw_ctrl(struct sensor_drv_context *sensor_cxt,
                                 uint8_t cmd, uint8_t sub_cmd, void *data) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    otp_ctrl_cmd_t *otp_cmd = malloc(sizeof(otp_ctrl_cmd_t));
    if (!otp_cmd) {
        CMR_LOGE("otp cmd buffer alloc failed!");
        return -1;
    }
    otp_cmd->cmd = cmd;
    otp_cmd->sub_cmd = sub_cmd;
    otp_cmd->data = data;
    /*the set mode function can be async control*/
    message.msg_type = SENSOR_CTRL_EVT_CFGOTP;
    if (cmd == OTP_WRITE_DATA) {
        // message.sync_flag = CMR_MSG_SYNC_RECEIVED;
        message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    } else {
        message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    }
    message.alloc_flag = 1; /*alloc from sender*/
    message.data = (void *)otp_cmd;

    ret = cmr_thread_msg_send(sensor_cxt->ctrl_thread_cxt.thread_handle,
                              &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
        if (message.data) {
            free(message.data);
        }
        return CMR_CAMERA_FAIL;
    }
    return ret;
}

LOCAL cmr_int sensor_otp_process(struct sensor_drv_context *sensor_cxt,
                                 uint8_t cmd, uint8_t sub_cmd, void *data) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = 0;
    struct _sensor_val_tag param;
    SENSOR_MATCH_T *module = sensor_cxt->current_module;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    if (module && module->otp_drv_info && sensor_cxt->otp_drv_handle) {
        switch (cmd) {
        case OTP_READ_RAW_DATA:
            ret = module->otp_drv_info->otp_ops.sensor_otp_read(
                sensor_cxt->otp_drv_handle, data);
            break;
        case OTP_READ_PARSE_DATA:
            module->otp_drv_info->otp_ops.sensor_otp_read(
                sensor_cxt->otp_drv_handle, NULL);
            ret = module->otp_drv_info->otp_ops.sensor_otp_parse(
                sensor_cxt->otp_drv_handle, data);
            if (ret == CMR_CAMERA_SUCCESS) {
                ret = module->otp_drv_info->otp_ops.sensor_otp_calibration(
                    sensor_cxt->otp_drv_handle);
            } else {
                SENSOR_LOGE("otp prase failed");
                return ret;
            }
            break;
        case OTP_WRITE_DATA:
            ret = module->otp_drv_info->otp_ops.sensor_otp_write(
                sensor_cxt->otp_drv_handle, data);
            if (ret != CMR_CAMERA_SUCCESS)
                return ret;
            break;
        case OTP_IOCTL:
            ret = module->otp_drv_info->otp_ops.sensor_otp_ioctl(
                sensor_cxt->otp_drv_handle, sub_cmd, data);
            if (ret != CMR_CAMERA_SUCCESS)
                return ret;
            break;
        default:
            break;
        }
    } else {
#if 0
        if ((NULL != sensor_cxt->sensor_info_ptr) &&
            (NULL != sensor_cxt->sensor_info_ptr->ioctl_func_tab_ptr)) {
            param.type = SENSOR_VAL_TYPE_INIT_OTP;
            if (PNULL !=
                sensor_cxt->sensor_info_ptr->ioctl_func_tab_ptr->cfg_otp) {
                sensor_cxt->sensor_info_ptr->ioctl_func_tab_ptr->cfg_otp(
                    sensor_cxt->hw_drv_handle, (cmr_uint)&param);
            }
        } else {
            SENSOR_LOGE("invalid param failed!");
            return CMR_CAMERA_INVALID_PARAM;
        }
#endif
    }
    ATRACE_END();
    return ret;
}

#include <cutils/properties.h>

static void sensor_rid_save_sensor_info(struct sensor_drv_context *sensor_cxt) {
    char sensor_info[80] = {0};
    const char *const sensorInterface0 =
        "/sys/devices/virtual/misc/sprd_sensor/camera_sensor_name";
    ssize_t wr_ret;
    int fd;
    char value[PROPERTY_VALUE_MAX];

    property_get("persist.sys.sensor.id", value, "0");
    if (!strcmp(value, "trigger_srid")) {
        SENSOR_LOGI("srid E\n");
        if (sensor_cxt->sensor_list_ptr[SENSOR_MAIN] != NULL &&
            sensor_cxt->sensor_list_ptr[SENSOR_MAIN]->sensor_version_info !=
                NULL &&
            strlen((const char *)sensor_cxt->sensor_list_ptr[SENSOR_MAIN]
                       ->sensor_version_info) > 1)
            sprintf(
                sensor_info, "%s ",
                sensor_cxt->sensor_list_ptr[SENSOR_MAIN]->sensor_version_info);
        if (sensor_cxt->sensor_list_ptr[SENSOR_SUB] != NULL &&
            sensor_cxt->sensor_list_ptr[SENSOR_SUB]->sensor_version_info !=
                NULL &&
            strlen((const char *)sensor_cxt->sensor_list_ptr[SENSOR_SUB]
                       ->sensor_version_info) > 1)
            sprintf(
                sensor_info, "%s %s ", sensor_info,
                sensor_cxt->sensor_list_ptr[SENSOR_SUB]->sensor_version_info);
        if (sensor_cxt->sensor_list_ptr[SENSOR_DEVICE2] != NULL &&
            sensor_cxt->sensor_list_ptr[SENSOR_DEVICE2]->sensor_version_info !=
                NULL &&
            strlen((const char *)sensor_cxt->sensor_list_ptr[SENSOR_DEVICE2]
                       ->sensor_version_info) > 1)
            sprintf(sensor_info, "%s %s ", sensor_info,
                    sensor_cxt->sensor_list_ptr[SENSOR_DEVICE2]
                        ->sensor_version_info);
        if (sensor_cxt->sensor_list_ptr[SENSOR_DEVICE3] != NULL &&
            sensor_cxt->sensor_list_ptr[SENSOR_DEVICE3]->sensor_version_info !=
                NULL &&
            strlen((const char *)sensor_cxt->sensor_list_ptr[SENSOR_DEVICE3]
                       ->sensor_version_info) > 1)
            sprintf(sensor_info, "%s %s ", sensor_info,
                    sensor_cxt->sensor_list_ptr[SENSOR_DEVICE3]
                        ->sensor_version_info);
        SENSOR_LOGI("WRITE srid %s \n", sensor_info);

        fd = open(sensorInterface0, O_WRONLY | O_TRUNC);
        if (-1 == fd) {
            SENSOR_LOGE("Failed to open: sensorInterface");
            goto exit;
        }
        wr_ret = write(fd, sensor_info, strlen(sensor_info));
        if (-1 == wr_ret) {
            SENSOR_LOGE("WRITE FAILED \n");
            close(fd);
            goto exit;
        }
        property_set("sys.sensor.info", sensor_info);
        property_set("persist.sys.sensor.id", "0");
        close(fd);

    exit:
        SENSOR_LOGV("X");
    }
}

static void sensor_save_info_for_checkXtool(struct sensor_drv_context *sensor_cxt) {
    static cmr_int is_save_sensor_info = 0;
    const char *const check_x_tool_file =
        "/data/misc/media/check_x_tool.file";
    ssize_t wr_ret;
    int fd;
    FILE *fp;
    char sensor_info[128] = {0};
    if (!is_save_sensor_info) {
        SENSOR_LOGI("sensor_save_info_for_checkXtool +++");
        if (sensor_cxt->sensor_list_ptr[SENSOR_MAIN] != NULL &&
            sensor_cxt->sensor_list_ptr[SENSOR_MAIN]->sensor_version_info !=
                NULL &&
            strlen((const char *)sensor_cxt->sensor_list_ptr[SENSOR_MAIN]
                       ->sensor_version_info) > 1)
            sprintf(
                sensor_info, "cam0id:%s\ncam0size:%ldW\n",
                sensor_cxt->sensor_list_ptr[SENSOR_MAIN]->sensor_version_info,
                sensor_cxt->sensor_list_ptr[SENSOR_MAIN]->source_width_max*
                sensor_cxt->sensor_list_ptr[SENSOR_MAIN]->source_height_max
				/100000
				*10);
        if (sensor_cxt->sensor_list_ptr[SENSOR_SUB] != NULL &&
            sensor_cxt->sensor_list_ptr[SENSOR_SUB]->sensor_version_info !=
                NULL &&
            strlen((const char *)sensor_cxt->sensor_list_ptr[SENSOR_SUB]
                       ->sensor_version_info) > 1)
            sprintf(
                sensor_info, "%s\ncam1id:%s\ncam1size:%ldW\n", sensor_info,
                sensor_cxt->sensor_list_ptr[SENSOR_SUB]->sensor_version_info,
                sensor_cxt->sensor_list_ptr[SENSOR_SUB]->source_width_max*
                sensor_cxt->sensor_list_ptr[SENSOR_SUB]->source_height_max
				/100000
				*10);
        if (sensor_cxt->sensor_list_ptr[SENSOR_DEVICE2] != NULL &&
            sensor_cxt->sensor_list_ptr[SENSOR_DEVICE2]->sensor_version_info !=
                NULL &&
            strlen((const char *)sensor_cxt->sensor_list_ptr[SENSOR_DEVICE2]
                       ->sensor_version_info) > 1)
            sprintf(
                sensor_info, "%s\ncam2id:%s\ncam2size:%ldW\n", sensor_info,
                sensor_cxt->sensor_list_ptr[SENSOR_DEVICE2]->sensor_version_info,
                sensor_cxt->sensor_list_ptr[SENSOR_DEVICE2]->source_width_max*
                sensor_cxt->sensor_list_ptr[SENSOR_DEVICE2]->source_height_max
				/100000
				*10);
        if (sensor_cxt->sensor_list_ptr[SENSOR_DEVICE3] != NULL &&
            sensor_cxt->sensor_list_ptr[SENSOR_DEVICE3]->sensor_version_info !=
                NULL &&
            strlen((const char *)sensor_cxt->sensor_list_ptr[SENSOR_DEVICE3]
                       ->sensor_version_info) > 1)
            sprintf(
                sensor_info, "%s\ncam3id:%s\ncam3size:%ldW\n", sensor_info,
                sensor_cxt->sensor_list_ptr[SENSOR_DEVICE3]->sensor_version_info,
                sensor_cxt->sensor_list_ptr[SENSOR_DEVICE3]->source_width_max*
                sensor_cxt->sensor_list_ptr[SENSOR_DEVICE3]->source_height_max
				/100000
				*10);

        #if 0
        fd = open(check_x_tool_file, O_WRONLY | O_TRUNC);
        if (-1 == fd) {
            SENSOR_LOGE("Failed to open: check_x_tool_file");
            goto exit;
        }
        wr_ret = write(fd, sensor_info, strlen(sensor_info));
        if (-1 == wr_ret) {
            SENSOR_LOGE("WRITE FAILED \n");
            close(fd);
            goto exit;
        }
        close(fd);
        #else
        fp = fopen(check_x_tool_file, "wb+");
        if (NULL == fp) {
            SENSOR_LOGE(" failed to open file %s open error:%s ", check_x_tool_file, strerror(errno));
        } else {
            fwrite(sensor_info, sizeof(sensor_info), 1, fp);
            fclose(fp);
        }
        #endif
        is_save_sensor_info = 1;

   exit:
        SENSOR_LOGV("X");
    }
}
/*--------------------------AF INTERFACE-----------------------------*/

static cmr_int sensor_af_init(cmr_handle sns_module_handle) {
    cmr_int ret = SENSOR_SUCCESS;
    struct af_drv_init_para input_ptr;
    struct sns_af_drv_ops *af_ops = NULL;
    SENSOR_DRV_CHECK_ZERO(sns_module_handle);
    struct sensor_drv_context *sensor_cxt =
        (struct sensor_drv_context *)sns_module_handle;
    SENSOR_MATCH_T *module = sensor_cxt->current_module;

    sensor_cxt->sensor_info_ptr->focus_eb =
        (module->af_dev_info.af_drv_entry != NULL);
    SENSOR_LOGI("sensor %s is focus enable %d",
                sensor_cxt->sensor_info_ptr->name,
                sensor_cxt->sensor_info_ptr->focus_eb);

    {
        SENSOR_VAL_T val;
        struct sensor_ic_ops *sns_ops = PNULL;
        cmr_u32 sns_cmd = SENSOR_IOCTL_ACCESS_VAL;
        struct sensor_ex_info sn_ex_info_slv;
        memset(&sn_ex_info_slv, 0, sizeof(struct sensor_ex_info));

        val.type = SENSOR_VAL_TYPE_GET_STATIC_INFO;
        val.pval = &sn_ex_info_slv;

        sns_ops = sensor_cxt->sensor_info_ptr->sns_ops;
        if (sns_ops) {
            ret = sns_ops->ext_ops[sns_cmd].ops(sensor_cxt->sns_ic_drv_handle,
                                                (cmr_uint)&val);
            if (ret) {
                SENSOR_LOGI("get sensor ex info failed");
            }
            SENSOR_LOGI(
                "sensor %s fov physical size (%f, %f), focal_lengths %f",
                module->sn_name, sn_ex_info_slv.fov_info.physical_size[0],
                sn_ex_info_slv.fov_info.physical_size[1],
                sn_ex_info_slv.fov_info.focal_lengths);
            memcpy(&sensor_cxt->fov_info, &sn_ex_info_slv.fov_info,
                   sizeof(sn_ex_info_slv.fov_info));
        } else {
            SENSOR_LOGI("sns_ops null");
        }
    }

    if (module && module->af_dev_info.af_drv_entry &&
        (!sensor_cxt->af_drv_handle)) {
        af_ops = &module->af_dev_info.af_drv_entry->af_ops;
        hw_sensor_set_monitor_val(
            sensor_cxt->hw_drv_handle,
            module->af_dev_info.af_drv_entry->motor_avdd_val);
        SENSOR_LOGI("af power is %d",
                    module->af_dev_info.af_drv_entry->motor_avdd_val);
        if (af_ops->create) {
            input_ptr.af_work_mode = module->af_dev_info.af_work_mode;
            input_ptr.hw_handle = sensor_cxt->hw_drv_handle;
            ret = af_ops->create(&input_ptr, &sensor_cxt->af_drv_handle);
            if (SENSOR_SUCCESS != ret)
                return SENSOR_FAIL;
        }
    } else {
        SENSOR_LOGE("ERROR:module:%p,entry:%p,af_ops:%p", module,
                    module ? module->af_dev_info.af_drv_entry : NULL, af_ops);
        return SENSOR_FAIL;
    }
    return ret;
}

static cmr_int sensor_af_deinit(cmr_handle sns_module_handle) {
    cmr_int ret = SENSOR_SUCCESS;
    struct sns_af_drv_ops *af_ops = NULL;
    SENSOR_DRV_CHECK_ZERO(sns_module_handle);
    struct sensor_drv_context *sensor_cxt =
        (struct sensor_drv_context *)sns_module_handle;
    SENSOR_MATCH_T *module = sensor_cxt->current_module;

    if (module && module->af_dev_info.af_drv_entry &&
        sensor_cxt->af_drv_handle) {
        af_ops = &module->af_dev_info.af_drv_entry->af_ops;
        if (af_ops->delete) {
            ret = af_ops->delete (sensor_cxt->af_drv_handle, NULL);
            hw_sensor_set_monitor_val(sensor_cxt->hw_drv_handle,
                                      SENSOR_AVDD_CLOSED);
            sensor_cxt->af_drv_handle = NULL;
            if (SENSOR_SUCCESS != ret)
                return SENSOR_FAIL;
        }
    } else {
        SENSOR_LOGE("AF_DEINIT:Don't register af driver,return directly'");
        return SENSOR_FAIL;
    }
    SENSOR_LOGI("X:");
    return ret;
}

static cmr_int sensor_af_set_pos(cmr_handle sns_module_handle, cmr_u32 pos) {
    cmr_int ret = SENSOR_SUCCESS;
    struct sns_af_drv_ops *af_ops = NULL;
    SENSOR_DRV_CHECK_ZERO(sns_module_handle);
    struct sensor_drv_context *sensor_cxt =
        (struct sensor_drv_context *)sns_module_handle;
    SENSOR_MATCH_T *module = sensor_cxt->current_module;

    if (module && module->af_dev_info.af_drv_entry &&
        sensor_cxt->af_drv_handle) {
        af_ops = &module->af_dev_info.af_drv_entry->af_ops;
        if (af_ops->set_pos) {
            ret = af_ops->set_pos(sensor_cxt->af_drv_handle, (uint16_t)pos);
            if (SENSOR_SUCCESS != ret)
                return SENSOR_FAIL;
        } else {
            ret = SENSOR_FAIL;
            SENSOR_LOGE("set_pos is null please check your af driver.");
        }
    } else {
        SENSOR_LOGE("af driver not exist,return directly");
        return SENSOR_FAIL;
    }

    return ret;
}

static cmr_int sensor_af_get_pos(cmr_handle sns_module_handle, cmr_u16 *pos) {
    cmr_int ret = SENSOR_SUCCESS;
    struct sns_af_drv_ops *af_ops = NULL;
    SENSOR_DRV_CHECK_ZERO(sns_module_handle);
    struct sensor_drv_context *sensor_cxt =
        (struct sensor_drv_context *)sns_module_handle;
    SENSOR_MATCH_T *module = sensor_cxt->current_module;

    if (module && module->af_dev_info.af_drv_entry &&
        sensor_cxt->af_drv_handle) {
        af_ops = &module->af_dev_info.af_drv_entry->af_ops;
        if (af_ops->get_pos) {
            ret = af_ops->get_pos(sensor_cxt->af_drv_handle, pos);
            if (SENSOR_SUCCESS != ret)
                return SENSOR_FAIL;
        }
    } else {
        SENSOR_LOGE("af driver not exist,return directly");
        return SENSOR_FAIL;
    }
    return ret;
}

static cmr_int sensor_af_get_pos_info(cmr_handle sns_module_handle,
                                      struct sensor_vcm_info *info) {
    cmr_int ret = SENSOR_SUCCESS;
    struct sns_af_drv_ops *af_ops = NULL;
    SENSOR_DRV_CHECK_ZERO(sns_module_handle);
    struct sensor_drv_context *sensor_cxt =
        (struct sensor_drv_context *)sns_module_handle;
    SENSOR_MATCH_T *module = sensor_cxt->current_module;

    if (module && module->af_dev_info.af_drv_entry &&
        sensor_cxt->af_drv_handle) {
        af_ops = &module->af_dev_info.af_drv_entry->af_ops;
        if (af_ops->ioctl) {
            ret = af_ops->ioctl(sensor_cxt->af_drv_handle,
                                CMD_SNS_AF_GET_POS_INFO, (void *)info);
            if (SENSOR_SUCCESS != ret)
                return SENSOR_FAIL;
        }
    } else {
        SENSOR_LOGE("af driver not exist,return directly");
        return SENSOR_FAIL;
    }
    return ret;
}

/*--------------------------OTP INTERFACE-----------------------------*/
static cmr_int sensor_otp_module_init(struct sensor_drv_context *sensor_cxt) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    SENSOR_LOGI("in");
    SENSOR_MATCH_T *module = sensor_cxt->current_module;
    otp_drv_init_para_t input_para = {0, NULL, 0, 0};

    if (module && (module->otp_drv_info) && (!sensor_cxt->otp_drv_handle)) {
        input_para.hw_handle = sensor_cxt->hw_drv_handle;
        input_para.sensor_name = (char *)sensor_cxt->sensor_info_ptr->name;
        input_para.sensor_id = sensor_cxt->sensor_register_info.cur_id;
        input_para.sensor_ic_addr = sensor_cxt->i2c_addr;
        ret = module->otp_drv_info->otp_ops.sensor_otp_create(
            &input_para, &sensor_cxt->otp_drv_handle);
    } else {
        SENSOR_LOGE("error:Don't register otp_driver please double check! ");
    }
    SENSOR_LOGI("out");
    ATRACE_END();
    return ret;
}

static cmr_int sensor_otp_module_deinit(struct sensor_drv_context *sensor_cxt) {
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    SENSOR_LOGI("in");
    SENSOR_MATCH_T *module = sensor_cxt->current_module;

    if (module && (module->otp_drv_info) && sensor_cxt->otp_drv_handle) {
        ret = module->otp_drv_info->otp_ops.sensor_otp_delete(
            sensor_cxt->otp_drv_handle);
        sensor_cxt->otp_drv_handle = NULL;
    } else {
        SENSOR_LOGE("Don't has otp instance,no need to release.");
    }
    SENSOR_LOGI("out");
    return ret;
}

/*--------------------------HARDWARE INTERFACE-----------------------------*/
cmr_int sensor_hw_read_i2c(cmr_handle sns_module_handle, cmr_u16 slave_addr,
                           cmr_u8 *cmd, cmr_u16 cmd_length) {
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_DRV_CHECK_ZERO(sns_module_handle);
    struct sensor_drv_context *sensor_cxt =
        (struct sensor_drv_context *)sns_module_handle;
    ret = hw_sensor_read_i2c(sensor_cxt->hw_drv_handle, slave_addr, cmd,
                             cmd_length);
    return ret;
}

cmr_int sensor_hw_write_i2c(cmr_handle sns_module_handle, cmr_u16 slave_addr,
                            cmr_u8 *cmd, cmr_u16 cmd_length) {
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_DRV_CHECK_ZERO(sns_module_handle);
    struct sensor_drv_context *sensor_cxt =
        (struct sensor_drv_context *)sns_module_handle;
    ret = hw_sensor_write_i2c(sensor_cxt->hw_drv_handle, slave_addr, cmd,
                              cmd_length);
    return ret;
}

cmr_int sensor_muti_i2c_write(cmr_handle sns_module_handle, void *param) {
    cmr_int ret = SENSOR_SUCCESS;
    struct sensor_muti_aec_i2c_tag *aec_i2c_info =
        (struct sensor_muti_aec_i2c_tag *)param;
    SENSOR_DRV_CHECK_ZERO(sns_module_handle);
    SENSOR_IC_CHECK_PTR(aec_i2c_info);
    struct sensor_drv_context *sensor_cxt =
        (struct sensor_drv_context *)sns_module_handle;
    ret = hw_sensor_muti_i2c_write(sensor_cxt->hw_drv_handle, aec_i2c_info);
    return ret;
}

/*--------------------------SENSOR IC INTERFACE-----------------------------*/
static cmr_int sensor_ic_create(struct sensor_drv_context *sensor_cxt,
                                cmr_u32 sensor_id) {
    cmr_int ret = SENSOR_SUCCESS;
    struct sensor_ic_ops *sns_ops = PNULL;
    struct sensor_ic_drv_init_para sns_init_para;

    SENSOR_DRV_CHECK_PTR(sensor_cxt);
    SENSOR_DRV_CHECK_PTR(sensor_cxt->sensor_info_ptr);
    SENSOR_DRV_CHECK_PTR(sensor_cxt->current_module);

    sns_ops = sensor_cxt->sensor_info_ptr->sns_ops;

    if (sns_ops && sns_ops->create_handle) {
        sns_init_para.caller_handle = sensor_cxt;
        sns_init_para.hw_handle = sensor_cxt->hw_drv_handle;
        sns_init_para.module_id =
            ((SENSOR_MATCH_T *)sensor_cxt->current_module)->module_id;
        sns_init_para.sensor_id = sensor_id;
        sns_init_para.ops_cb.set_mode = sensor_set_mode_common;
        sns_init_para.ops_cb.get_mode = sensor_get_mode_common;
        sns_init_para.ops_cb.set_exif_info = sensor_set_exif_common;
        sns_init_para.ops_cb.get_exif_info = sensor_get_exif_common;
        sns_init_para.ops_cb.set_mode_wait_done = sensor_set_mode_done_common;
        ret = sns_ops->create_handle(&sns_init_para,
                                     &sensor_cxt->sns_ic_drv_handle);
        if (SENSOR_SUCCESS != ret)
            return ret;
    }

    return ret;
}

static cmr_int sensor_ic_delete(struct sensor_drv_context *sensor_cxt) {
    cmr_int ret = SENSOR_SUCCESS;
    struct sensor_ic_ops *sns_ops = PNULL;
    struct sensor_ic_drv_init_para sns_init_para;

    SENSOR_DRV_CHECK_PTR(sensor_cxt);
    SENSOR_DRV_CHECK_PTR(sensor_cxt->sensor_info_ptr);

    if (sensor_cxt->sns_ic_drv_handle) {
        sns_ops = sensor_cxt->sensor_info_ptr->sns_ops;
        if (sns_ops && sns_ops->delete_handle) {
            ret = sns_ops->delete_handle(sensor_cxt->sns_ic_drv_handle, NULL);
            if (SENSOR_SUCCESS != ret)
                return ret;
        }
    }
    sensor_cxt->sns_ic_drv_handle = PNULL;
    return ret;
}

static void *sensor_ic_get_data(struct sensor_drv_context *sensor_cxt,
                                cmr_uint cmd) {
    void *data = PNULL;
    struct sensor_ic_ops *sns_ops = PNULL;

    if (!sensor_cxt || (!sensor_cxt->sensor_info_ptr)) {
        SENSOR_LOGE("sensor_cxt is null return\n");
        return PNULL;
    }
    sns_ops = sensor_cxt->sensor_info_ptr->sns_ops;
    if (!sns_ops || !sns_ops->get_data) {
        SENSOR_LOGE("error:sensor ops is null\n");
        return PNULL;
    }

    switch (cmd) {
    case SENSOR_CMD_GET_TRIM_TAB:
        sns_ops->get_data(sensor_cxt->sns_ic_drv_handle,
                          SENSOR_CMD_GET_TRIM_TAB, &data);
        break;
    case SENSOR_CMD_GET_RESOLUTION:
        sns_ops->get_data(sensor_cxt->sns_ic_drv_handle,
                          SENSOR_CMD_GET_RESOLUTION, &data);
        break;
    case SENSOR_CMD_GET_MODULE_CFG:
        sns_ops->get_data(sensor_cxt->sns_ic_drv_handle,
                          SENSOR_CMD_GET_MODULE_CFG, &data);
        break;
    case SENSOR_CMD_GET_EXIF:
        sns_ops->get_data(sensor_cxt->sns_ic_drv_handle, SENSOR_CMD_GET_EXIF,
                          &data);
        break;
    default:
        SENSOR_LOGW("not support cmd:0x%lx", cmd);
    }

    return data;
}

cmr_int sensor_ic_write_gain(cmr_handle handle, cmr_u32 param) {
    cmr_int ret = SENSOR_SUCCESS;
    struct sensor_ic_ops *sns_ops = PNULL;
    struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)handle;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    SENSOR_DRV_CHECK_ZERO(sensor_cxt->sensor_info_ptr);

    sns_ops = sensor_cxt->sensor_info_ptr->sns_ops;

#ifdef SBS_MODE_SENSOR
    char value1[PROPERTY_VALUE_MAX];
    property_get("persist.camera.sbs.mode", value1, "0");
    if (!strcmp(value1, "slave")) {
#ifdef SBS_SENSOR_FRONT
        sns_ops = sensor_cxt->sensor_list_ptr[3]->sns_ops;
#else
        sns_ops = sensor_cxt->sensor_list_ptr[2]->sns_ops;
#endif
    }
#endif
    if (!(sns_ops && sns_ops->write_gain_value)) {
        CMR_LOGE("write_gain is NULL,return");
        return SENSOR_FAIL;
    }
    ret = sns_ops->write_gain_value(sensor_cxt->sns_ic_drv_handle, param);

    return ret;
}

static cmr_int sensor_ic_ex_write_exposure(cmr_handle handle, cmr_uint param) {
    cmr_int ret = SENSOR_SUCCESS;
    struct sensor_ic_ops *sns_ops = PNULL;
    struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)handle;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    SENSOR_DRV_CHECK_ZERO(sensor_cxt->sensor_info_ptr);

    sns_ops = sensor_cxt->sensor_info_ptr->sns_ops;
#ifdef SBS_MODE_SENSOR
    char value1[PROPERTY_VALUE_MAX];
    property_get("persist.camera.sbs.mode", value1, "0");
    if (!strcmp(value1, "slave")) {
#ifdef SBS_SENSOR_FRONT
        sns_ops = sensor_cxt->sensor_list_ptr[3]->sns_ops;
#else
        sns_ops = sensor_cxt->sensor_list_ptr[2]->sns_ops;
#endif
    }
#endif
    if (!(sns_ops && sns_ops->ex_write_exp)) {
        CMR_LOGE("ex_write_exp is NULL,return");
        return SENSOR_FAIL;
    }
    ret = sns_ops->ex_write_exp(sensor_cxt->sns_ic_drv_handle, param);

    return ret;
}

static cmr_int sensor_ic_write_ae_value(cmr_handle handle, cmr_u32 param) {
    cmr_int ret = SENSOR_SUCCESS;
    struct sensor_ic_ops *sns_ops = PNULL;
    struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)handle;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    SENSOR_DRV_CHECK_ZERO(sensor_cxt->sensor_info_ptr);

    sns_ops = sensor_cxt->sensor_info_ptr->sns_ops;
#ifdef SBS_MODE_SENSOR
    char value1[PROPERTY_VALUE_MAX];
    property_get("persist.camera.sbs.mode", value1, "0");
    if (!strcmp(value1, "slave")) {
#ifdef SBS_SENSOR_FRONT
        sns_ops = sensor_cxt->sensor_list_ptr[3]->sns_ops;
#else
        sns_ops = sensor_cxt->sensor_list_ptr[2]->sns_ops;
#endif
    }
#endif
    if (!(sns_ops && sns_ops->write_ae_value)) {
        CMR_LOGE("write_ae_value is NULL,return");
        return SENSOR_FAIL;
    }
    ret = sns_ops->write_ae_value(sensor_cxt->sns_ic_drv_handle, param);

    return ret;
}

static cmr_int sensor_ic_read_aec_info(cmr_handle handle, void *param) {
    cmr_int ret = SENSOR_SUCCESS;
    struct sensor_ic_ops *sns_ops = PNULL;
    struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)handle;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    SENSOR_DRV_CHECK_ZERO(sensor_cxt->sensor_info_ptr);

    sns_ops = sensor_cxt->sensor_info_ptr->sns_ops;
    if (!(sns_ops && sns_ops->read_aec_info)) {
        CMR_LOGE("ex_write_exp is NULL,return");
        return SENSOR_FAIL;
    }
    ret = sns_ops->read_aec_info(sensor_cxt->sns_ic_drv_handle, param);

    return ret;
}

static cmr_int sensor_ic_write_multi_ae_info(cmr_handle handle, void *param) {
    cmr_int ret = SENSOR_SUCCESS;
    cmr_handle sensor_handle;
    struct sensor_ic_ops *sns_ops = PNULL;
    struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)handle;
    struct sensor_multi_ae_info *ae_info = (struct sensor_multi_ae_info *)param;
    struct sensor_aec_reg_info aec_reg_info[CAMERA_ID_MAX];
    struct sensor_muti_aec_i2c_tag muti_aec_info;
    struct sensor_aec_i2c_tag *aec_info;
    cmr_u32 i, k, size = 0;
    cmr_u32 count = 0;
    struct sensor_reg_tag msettings[AEC_I2C_SETTINGS_MAX];
    struct sensor_reg_tag ssettings[AEC_I2C_SETTINGS_MAX];
    struct sensor_reg_tag *settings = NULL;
    cmr_u16 sensor_id[AEC_I2C_SENSOR_MAX];
    cmr_u16 i2c_slave_addr[AEC_I2C_SENSOR_MAX];
    cmr_u16 addr_bits_type[AEC_I2C_SENSOR_MAX];
    cmr_u16 data_bits_type[AEC_I2C_SENSOR_MAX];

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    SENSOR_DRV_CHECK_ZERO(sensor_cxt->sensor_info_ptr);
    SENSOR_DRV_CHECK_ZERO(param);

    memset(&aec_reg_info, 0x00, sizeof(aec_reg_info));
    memset(&muti_aec_info, 0x00, sizeof(muti_aec_info));
    memset(&aec_info, 0x00, sizeof(aec_info));

    count = ae_info[0].count;
    for (i = 0; i < count; i++) {
        sensor_handle = ae_info[i].handle;
        aec_reg_info[i].exp.exposure = ae_info[i].exp.exposure;
        aec_reg_info[i].exp.dummy = ae_info[i].exp.dummy;
        aec_reg_info[i].exp.size_index = ae_info[i].exp.size_index;
        aec_reg_info[i].gain = ae_info[i].gain;
        SENSOR_LOGV("read aec i %u count %d handle %p", i, count,
                    sensor_handle);
        ret = sensor_ic_read_aec_info(sensor_handle, (&aec_reg_info[i]));
        if (ret != CMR_CAMERA_SUCCESS)
            return ret;
    }

    muti_aec_info.sensor_id = sensor_id;
    muti_aec_info.i2c_slave_addr = i2c_slave_addr;
    muti_aec_info.addr_bits_type = addr_bits_type;
    muti_aec_info.data_bits_type = data_bits_type;

    for (k = 0; k < count; k++) {
        size = 0;
        aec_info = aec_reg_info[k].aec_i2c_info_out;
        sensor_id[k] = ae_info[k].camera_id;
        muti_aec_info.id_size = count;
        i2c_slave_addr[k] = aec_info->slave_addr;
        muti_aec_info.i2c_slave_len = count;
        addr_bits_type[k] = (cmr_u16)aec_info->addr_bits_type;
        muti_aec_info.addr_bits_type_len = count;
        data_bits_type[k] = (cmr_u16)aec_info->data_bits_type;
        muti_aec_info.data_bits_type_len = count;
        if (k == 0) {
            muti_aec_info.master_i2c_tab = msettings;
            settings = msettings;
        } else if (k == 1) {
            muti_aec_info.slave_i2c_tab = ssettings;
            settings = ssettings;
        }

        for (i = 0; i < aec_info->frame_length->size; i++) {
            settings[size].reg_addr =
                aec_info->frame_length->settings[i].reg_addr;
            settings[size++].reg_value =
                aec_info->frame_length->settings[i].reg_value;
        }
        for (i = 0; i < aec_info->shutter->size; i++) {
            settings[size].reg_addr = aec_info->shutter->settings[i].reg_addr;
            settings[size++].reg_value =
                aec_info->shutter->settings[i].reg_value;
        }
        for (i = 0; i < aec_info->again->size; i++) {
            settings[size].reg_addr = aec_info->again->settings[i].reg_addr;
            settings[size++].reg_value = aec_info->again->settings[i].reg_value;
        }
        for (i = 0; i < aec_info->dgain->size; i++) {
            settings[size].reg_addr = aec_info->dgain->settings[i].reg_addr;
            settings[size++].reg_value = aec_info->dgain->settings[i].reg_value;
        }
        if (k == 0) {
            muti_aec_info.msize =
                aec_info->shutter->size + aec_info->again->size +
                aec_info->dgain->size + aec_info->frame_length->size;
        } else if (k == 1) {
            muti_aec_info.ssize =
                aec_info->shutter->size + aec_info->again->size +
                aec_info->dgain->size + aec_info->frame_length->size;
        }
    }

    ret = sensor_muti_i2c_write(handle, &muti_aec_info);
    if (ret != CMR_CAMERA_SUCCESS)
        return ret;

    return ret;
}

cmr_int sensor_ic_ioctl(cmr_handle handle, enum sns_cmd cmd, void *param) {
    cmr_int ret = SENSOR_SUCCESS;
    struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)handle;

    SENSOR_DRV_CHECK_ZERO(handle);

    switch (cmd) {
    case CMD_SNS_IC_WRITE_MULTI_AE:
        ret = sensor_ic_write_multi_ae_info(handle, param);
        break;
    default:
        break;
    };

    return ret;
}

/*--------------------------COMMON INTERFACE-----------------------------*/

/** sensor_drv_ioctl:
 *  @sns_module_handle: sensor module handle.
 *  @cmd:   command that you want to do.
 *  @param: parameter data you want to transmit.
 *
 * NOTE:
 * This is an extended interface which contain sensor_ic,otp_drv,af_drv
 * if you want add some sub commands.please add them in
 *common/inc/cmr_sensor_info.h
 *
 * Return:
 * SENSOR_SUCCESS : ioctl success
 * SENSOR_FAIL : ioctl failed
 **/
cmr_int sensor_drv_ioctl(cmr_handle handle, enum sns_cmd cmd, void *param) {
    cmr_int ret = SENSOR_SUCCESS;
    struct sns_af_drv_ops *af_ops = PNULL;
    sensor_otp_ops_t *otp_ops = PNULL;
    SENSOR_MATCH_T *module = PNULL;
    SENSOR_DRV_CHECK_ZERO(handle);
    struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)handle;
    SENSOR_LOGI("E:cmd:0x%x", cmd);
    module = (SENSOR_MATCH_T *)sensor_cxt->current_module;
    SENSOR_DRV_CHECK_PTR(module);

    switch (cmd >> 8) {
    case CMD_SNS_OTP:
        if (module && (module->otp_drv_info) && sensor_cxt->otp_drv_handle) {
            otp_ops = &module->otp_drv_info->otp_ops;
            if (otp_ops && otp_ops->sensor_otp_ioctl) {
                ret = otp_ops->sensor_otp_ioctl(sensor_cxt->otp_drv_handle, cmd,
                                                param);
                if (ret != CMR_CAMERA_SUCCESS)
                    return ret;
            }
        } else {
            ret = SENSOR_FAIL;
            SENSOR_LOGE("otp driver,return directly");
        }
        break;
    case CMD_SNS_IC:
        ret = sensor_ic_ioctl(handle, cmd, param);
        break;
    case CMD_SNS_AF:
        if (module && module->af_dev_info.af_drv_entry &&
            sensor_cxt->af_drv_handle) {
            af_ops = &module->af_dev_info.af_drv_entry->af_ops;
            if (af_ops && af_ops->ioctl) {
                ret = af_ops->ioctl(sensor_cxt->af_drv_handle, cmd, param);
                if (SENSOR_SUCCESS != ret)
                    return SENSOR_FAIL;
            }
        } else {
            ret = SENSOR_FAIL;
            SENSOR_LOGE("af driver,return directly");
        }
        break;
    default:
        break;
    };

    SENSOR_LOGI("X:");
    return ret;
}
