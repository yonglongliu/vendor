#define LOG_TAG "sns_ic_drv"
#include "sensor_ic_drv.h"
#include "cmr_common.h"

/**
* There are only four Camera,so far,If there are more than four
* cameras in the future,you should change the camera nums*/
static EXIF_SPEC_PIC_TAKING_COND_T *exif_info_ptr[4] = {NULL, NULL, NULL, NULL};
/*common interface*/
cmr_int sensor_ic_drv_create(struct sensor_ic_drv_init_para *init_param,
                             cmr_handle *sns_ic_drv_handle) {
    cmr_int ret = SENSOR_IC_SUCCESS;
    struct sensor_ic_drv_cxt *sns_drv_cxt = NULL;
    SENSOR_IC_CHECK_HANDLE(init_param);

    *sns_ic_drv_handle = NULL;
    sns_drv_cxt =
        (struct sensor_ic_drv_cxt *)malloc(sizeof(struct sensor_ic_drv_cxt));
    if (!sns_drv_cxt) {
        SENSOR_LOGE("malloc failed,create sensor ic instance failed!");
        ret = SENSOR_IC_FAILED;
    } else {
        cmr_bzero(sns_drv_cxt, sizeof(*sns_drv_cxt));
        sns_drv_cxt->hw_handle = init_param->hw_handle;
        sns_drv_cxt->caller_handle = init_param->caller_handle;
        sns_drv_cxt->sensor_id = init_param->sensor_id;
        sns_drv_cxt->module_id = init_param->module_id;
        /*init ops*/
        sns_drv_cxt->ops_cb.set_exif_info = init_param->ops_cb.set_exif_info;
        sns_drv_cxt->ops_cb.get_exif_info = init_param->ops_cb.get_exif_info;
        sns_drv_cxt->ops_cb.set_mode = init_param->ops_cb.set_mode;
        sns_drv_cxt->ops_cb.set_mode_wait_done =
            init_param->ops_cb.set_mode_wait_done;
        sns_drv_cxt->ops_cb.get_mode = init_param->ops_cb.get_mode;
        sns_drv_cxt->exif_malloc = 0;

        *sns_ic_drv_handle = (cmr_handle)sns_drv_cxt;
    }
    CMR_LOGI("sns_drv_cxt:%p", sns_drv_cxt);
    return ret;
}

cmr_int sensor_ic_drv_delete(cmr_handle handle, void *param) {
    cmr_int ret = SENSOR_IC_SUCCESS;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    if (sns_drv_cxt->exif_ptr && 1 == sns_drv_cxt->exif_malloc) {
        free(sns_drv_cxt->exif_ptr);
        exif_info_ptr[sns_drv_cxt->sensor_id] = NULL;
    }
    SENSOR_LOGV("in");
    if (handle) {
        free(handle);
    } else {
        SENSOR_LOGE("sensor_ic_handle is NULL,no need release.");
    }
    SENSOR_LOGV("out");
    return ret;
}

cmr_int sensor_ic_get_private_data(cmr_handle handle, cmr_uint cmd,
                                   void **param) {
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param);

    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    if (cmd >= SENSOR_CMD_GET_MAX) {
        SENSOR_LOGE("invalid cmd, cmd:%lu", cmd);
        return SENSOR_IC_FAILED;
    }
    switch (cmd) {
    case SENSOR_CMD_GET_TRIM_TAB:
        *param = sns_drv_cxt->trim_tab_info;
        break;
    case SENSOR_CMD_GET_RESOLUTION:
        *param = sns_drv_cxt->res_tab_info;
        break;
    case SENSOR_CMD_GET_MODULE_CFG:
        *param = sns_drv_cxt->module_info;
        break;
    case SENSOR_CMD_GET_EXIF:
        if (sns_drv_cxt->exif_ptr)
            *param = sns_drv_cxt->exif_ptr;
        /*will be used in the future*/
        // else
        //    *param = &sns_drv_cxt->exif_info;
        break;
    default:
        SENSOR_LOGE("not support cmd:%lu", cmd);
    }
    return SENSOR_IC_SUCCESS;
}

cmr_int sensor_ic_get_init_exif_info(cmr_handle handle, void **exif_info_in) {
    cmr_int ret = SENSOR_IC_SUCCESS;
    void *buffer = NULL;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    cmr_int sensor_num =
        sizeof(exif_info_ptr) / sizeof(EXIF_SPEC_PIC_TAKING_COND_T *);
    if (sns_drv_cxt->sensor_id >= sensor_num) {
        SENSOR_LOGE("sensor id is invalid,support sensor count:%d", sensor_num);
        ret = SENSOR_IC_FAILED;
        goto exit;
    }
    if (exif_info_ptr[sns_drv_cxt->sensor_id]) {
        *exif_info_in = (void *)exif_info_ptr[sns_drv_cxt->sensor_id];
    } else {
        buffer = malloc(sizeof(EXIF_SPEC_PIC_TAKING_COND_T));
        if (!buffer) {
            SENSOR_LOGE("malloc exif info mem failed");
            ret = SENSOR_IC_FAILED;
            goto exit;
        }
        sns_drv_cxt->exif_malloc = 1;
        exif_info_ptr[sns_drv_cxt->sensor_id] = buffer;
        memset(buffer, 0, sizeof(EXIF_SPEC_PIC_TAKING_COND_T));
        EXIF_SPEC_PIC_TAKING_COND_T *exif_ptr =
            (EXIF_SPEC_PIC_TAKING_COND_T *)buffer;
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

        *exif_info_in = buffer;
    }

exit:
    return ret;
}

cmr_int sensor_ic_set_match_static_info(cmr_handle handle, cmr_u16 vendor_num,
                                        void *static_info_ptr) {
    cmr_int ret = SENSOR_IC_SUCCESS;
    cmr_u32 i = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(static_info_ptr);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    SENSOR_STATIC_INFO_T *static_info = (SENSOR_STATIC_INFO_T *)static_info_ptr;
    SENSOR_LOGV("vendor_num:%d", vendor_num);

    for (i = 0; i < vendor_num; i++) {
        if (sns_drv_cxt->module_id == static_info[i].module_id) {
            sns_drv_cxt->static_info = &static_info[i].static_info;
            break;
        }
    }
    if (i == vendor_num) {
        SENSOR_LOGE("to the end of static info array,size:%d", vendor_num);
        ret = SENSOR_IC_FAILED;
    }
    SENSOR_LOGV("i:%d", i);

    return ret;
}

cmr_int sensor_ic_set_match_fps_info(cmr_handle handle, cmr_u16 vendor_num,
                                     void *fps_info_ptr) {
    cmr_int ret = SENSOR_IC_SUCCESS;
    cmr_u32 i = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(fps_info_ptr);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    SENSOR_MODE_FPS_INFO_T *fps_info = (SENSOR_MODE_FPS_INFO_T *)fps_info_ptr;
    SENSOR_LOGV("vendor_num:%d", vendor_num);

    for (i = 0; i < vendor_num; i++) {
        if (sns_drv_cxt->module_id == fps_info[i].module_id) {
            sns_drv_cxt->fps_info = &fps_info[i].fps_info;
            break;
        }
    }
    if (i == vendor_num) {
        SENSOR_LOGE("to the end of fps_info array,size:%d", vendor_num);
        ret = SENSOR_IC_FAILED;
    }
    SENSOR_LOGV("i:%d", i);

    return ret;
}

cmr_int sensor_ic_set_match_resolution_info(cmr_handle handle,
                                            cmr_u16 vendor_num, void *param) {
    cmr_int ret = SENSOR_IC_SUCCESS;
    cmr_u32 i = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param);

    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    struct sensor_res_tab_info *res_ptr = (struct sensor_res_tab_info *)param;
    SENSOR_LOGV("vendor_num:%d", vendor_num);

    for (i = 0; i < vendor_num; i++) {
        if (sns_drv_cxt->module_id == res_ptr[i].module_id) {
            sns_drv_cxt->res_tab_info =
                (struct sensor_reg_tab_info_tag *)&res_ptr[i].reg_tab;
            break;
        }
    }
    if (i == vendor_num) {
        SENSOR_LOGE("to the end of res_solu array,size:%d", vendor_num);
        ret = SENSOR_IC_FAILED;
    }
    SENSOR_LOGV("i:%d", i);

    return ret;
}

cmr_int sensor_ic_set_match_trim_info(cmr_handle handle, cmr_u16 vendor_num,
                                      void *param) {
    cmr_int ret = SENSOR_IC_SUCCESS;
    cmr_u32 i = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param);

    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    struct sensor_trim_info *trim_ptr = (struct sensor_trim_info *)param;
    SENSOR_LOGV("vendor_num:%d", vendor_num);

    for (i = 0; i < vendor_num; i++) {
        if (sns_drv_cxt->module_id == trim_ptr[i].module_id) {
            sns_drv_cxt->trim_tab_info =
                (struct sensor_trim_tag *)&trim_ptr[i].trim_info;
            break;
        }
    }
    if (i == vendor_num) {
        SENSOR_LOGE("to the end of trim array,size:%d", vendor_num);
        ret = SENSOR_IC_FAILED;
    }
    SENSOR_LOGV("i:%d", i);

    return ret;
}

cmr_int sensor_ic_set_match_module_info(cmr_handle handle, cmr_u16 vendor_num,
                                        void *module_info_ptr) {
    cmr_int ret = SENSOR_IC_SUCCESS;
    cmr_u32 i = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(module_info_ptr);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    struct sensor_module_info *module_info_tab =
        (struct sensor_module_info *)module_info_ptr;

    for (i = 0; i < vendor_num; i++) {
        sns_drv_cxt->module_info = &module_info_tab[i].module_info;
        if (sns_drv_cxt->module_id == module_info_tab[i].module_id)
            break;
    }
    if (i == vendor_num) {
        SENSOR_LOGE("to the end of module info array,size:%d", vendor_num);
        ret = SENSOR_IC_FAILED;
    }
    SENSOR_LOGV("i:%d", i);

    return ret;
}

void sensor_ic_print_static_info(cmr_s8 *log_tag,
                                 struct sensor_ex_info *ex_info) {
    if (ex_info == PNULL)
        return;
    SENSOR_LOGI("%s: f_num: %d", log_tag, ex_info->f_num);
    SENSOR_LOGI("%s: max_fps: %d", log_tag, ex_info->max_fps);
    SENSOR_LOGI("%s: max_adgain: %d", log_tag, ex_info->max_adgain);
    SENSOR_LOGI("%s: ois_supported: %d", log_tag, ex_info->ois_supported);
    SENSOR_LOGI("%s: pdaf_supported: %d", log_tag, ex_info->pdaf_supported);
    SENSOR_LOGI("%s: exp_valid_frame_num: %d", log_tag,
                ex_info->exp_valid_frame_num);
    SENSOR_LOGI("%s: clam_level: %d", log_tag, ex_info->clamp_level);
    SENSOR_LOGI("%s: adgain_valid_frame_num: %d", log_tag,
                ex_info->adgain_valid_frame_num);
    SENSOR_LOGI("%s: sensor name is: %s", log_tag, ex_info->name);
    SENSOR_LOGI("%s: sensor version info is: %s", log_tag,
                ex_info->sensor_version_info);
    SENSOR_LOGI("%s: up2hori: %d", log_tag, ex_info->pos_dis.up2hori);
    SENSOR_LOGI("%s: hori2down: %d", log_tag, ex_info->pos_dis.hori2down);
}
