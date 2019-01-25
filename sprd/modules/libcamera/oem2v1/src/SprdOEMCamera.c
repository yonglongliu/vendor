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
#define LOG_TAG "OEMCam"

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#if (MINICAMERA != 1)
#include <math.h>
#endif
#include "cmr_oem.h"
#include "SprdOEMCamera.h"
#include "cmr_img_debug.h"

cmr_int camera_init(cmr_u32 camera_id, camera_cb_of_type callback,
                    void *client_data, cmr_uint is_autotest,
                    cmr_handle *camera_handle, void *cb_of_malloc,
                    void *cb_of_free) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    if (!callback || !client_data || !camera_handle) {
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    oem_init_log_level();

    CMR_LOGI("camera id %d autotest %ld", camera_id, is_autotest);
    ret = camera_local_int(camera_id, callback, client_data, is_autotest,
                           camera_handle, cb_of_malloc, cb_of_free);
    if (ret) {
        ret = -CMR_CAMERA_FAIL;
        CMR_LOGE("failed to init camera %ld", ret);
    } else {
        CMR_LOGI("camera handle 0x%lx", (cmr_uint)*camera_handle);
    }

    camera_lls_enable(*camera_handle, 0);
    camera_set_lls_shot_mode(*camera_handle, 0);
    camera_vendor_hdr_enable(*camera_handle, 0);
exit:
    return ret;
}

cmr_int camera_deinit(cmr_handle camera_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    if (!camera_handle) {
        ret = -CMR_CAMERA_INVALID_PARAM;
        CMR_LOGE("param is null");
        goto exit;
    }
    camera_local_deinit(camera_handle);
exit:
    return ret;
}

cmr_int camera_release_frame(cmr_handle camera_handle, enum camera_data data,
                             cmr_uint index) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)camera_handle;

    if (!camera_handle) {
        CMR_LOGE("camera handle is null");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    CMR_LOGI("release data %d  index %ld", data, index);
    switch (data) {
    case CAMERA_PREVIEW_DATA:
    case CAMERA_VIDEO_DATA:
        ret = cmr_preview_release_frame(cxt->prev_cxt.preview_handle,
                                        cxt->camera_id, index);
        break;
    case CAMERA_SNAPSHOT_DATA:
        ret = cmr_snapshot_release_frame(cxt->snp_cxt.snapshot_handle, index);
        break;
    default:
        CMR_LOGI("don't support %d", data);
        break;
    }
    if (ret) {
        CMR_LOGE("failed to release frame ret %ld", ret);
    }
exit:
    CMR_LOGI("done %ld", ret);
    return ret;
}

cmr_int camera_set_param(cmr_handle camera_handle, enum camera_param_type id,
                         cmr_uint param) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)camera_handle;
    struct setting_cmd_parameter setting_param;

    if (!camera_handle) {
        CMR_LOGE("camera handle is null");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    ret = camera_local_set_param(camera_handle, id, param);
exit:
    return ret;
}

cmr_int camera_start_preview(cmr_handle camera_handle,
                             enum takepicture_mode mode) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    if (!camera_handle) {
        CMR_LOGE("camera handle is null");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    ret = camera_local_start_preview(camera_handle, mode, CAMERA_PREVIEW);
    if (ret) {
        CMR_LOGE("failed to start preview %ld", ret);
    }
exit:
    CMR_LOGI("done");
    return ret;
}

cmr_int camera_stop_preview(cmr_handle camera_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    if (!camera_handle) {
        CMR_LOGE("camera handle is null");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    ret = camera_local_stop_preview(camera_handle);
    if (ret) {
        CMR_LOGE("failed to stop preview %ld", ret);
    }
exit:
    CMR_LOGI("done");
    return ret;
}

cmr_int camera_start_autofocus(cmr_handle camera_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    if (!camera_handle) {
        CMR_LOGE("camera handle is null");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    ret = camera_local_start_focus(camera_handle);
    if (ret) {
        CMR_LOGE("failed to start focus %ld", ret);
    }

exit:
    CMR_LOGI("done");
    return ret;
}

cmr_int camera_cancel_autofocus(cmr_handle camera_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    if (!camera_handle) {
        CMR_LOGE("camera handle is null");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    ret = camera_local_cancel_focus(camera_handle);
    if (ret) {
        CMR_LOGE("failed to stop focus %ld", ret);
    }

exit:
    CMR_LOGI("done");
    return ret;
}

// when autoFocus, hal set caf mode to af in oem and isp,but in fact,the af mode
// is always caf which app and hal know this
cmr_int camera_transfer_caf_to_af(cmr_handle camera_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    ret = camera_local_transfer_caf_to_af(camera_handle);

    return ret;
}

// when cancelFocus, hal set af mode to caf in oem and isp,but in fact,the af
// mode is always caf which app and hal know this
cmr_int camera_transfer_af_to_caf(cmr_handle camera_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    ret = camera_local_transfer_af_to_caf(camera_handle);

    return ret;
}

cmr_int camera_cancel_takepicture(cmr_handle camera_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    if (!camera_handle) {
        CMR_LOGE("camera handle is null");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    ret = camera_local_stop_snapshot(camera_handle);
    if (ret) {
        CMR_LOGE("failed to cancel snapshot %ld", ret);
    }
exit:
    CMR_LOGI("done");
    return ret;
}

cmr_int camera_take_picture(cmr_handle camera_handle,
                            enum takepicture_mode cap_mode) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    if (!camera_handle) {
        CMR_LOGE("camera handle is null");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    ret = camera_local_start_snapshot(camera_handle, cap_mode, CAMERA_SNAPSHOT);
    if (ret) {
        CMR_LOGE("failed to start snapshot %ld", ret);
    }

exit:
    CMR_LOGI("done");
    return ret;
}

cmr_int camera_get_sn_trim(cmr_handle camera_handle, cmr_u32 mode,
                           cmr_uint *trim_x, cmr_uint *trim_y, cmr_uint *trim_w,
                           cmr_uint *trim_h, cmr_uint *width,
                           cmr_uint *height) // fot hal2.0
{
    UNUSED(camera_handle);
    UNUSED(mode);
    UNUSED(trim_x);
    UNUSED(trim_y);
    UNUSED(mode);
    UNUSED(trim_x);
    UNUSED(trim_y);
    UNUSED(trim_w);
    UNUSED(trim_h);
    UNUSED(width);
    UNUSED(height);

    cmr_int ret = CMR_CAMERA_SUCCESS;

    return ret;
}

cmr_int camera_set_mem_func(cmr_handle camera_handle, void *cb_of_malloc,
                            void *cb_of_free, void *private_data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)camera_handle;

    CMR_LOGI("0x%lx 0x%lx 0x%lx 0x%lx", (cmr_uint)camera_handle,
             (cmr_uint)cb_of_malloc, (cmr_uint)cb_of_free,
             (cmr_uint)private_data);
    if (!camera_handle || !cb_of_malloc || !cb_of_free) {
        CMR_LOGE("param error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    cxt->hal_malloc = cb_of_malloc;
    cxt->hal_free = cb_of_free;
    cxt->hal_mem_privdata = private_data;
exit:
    return ret;
}

cmr_int camera_get_redisplay_data(
    cmr_handle camera_handle, cmr_s32 output_fd, cmr_uint output_addr,
    cmr_uint output_vir_addr, cmr_uint output_width, cmr_uint output_height,
    cmr_s32 input_fd, cmr_uint input_addr_y, cmr_uint input_addr_uv,
    cmr_uint input_vir_addr, cmr_uint input_width, cmr_uint input_height) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    CMR_LOGI("0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx",
             (cmr_uint)camera_handle, (cmr_uint)output_addr,
             (cmr_uint)output_width, (cmr_uint)output_height,
             (cmr_uint)input_addr_y, (cmr_uint)input_addr_uv,
             (cmr_uint)input_width, (cmr_uint)input_height);

    if (!camera_handle || !output_width || !output_height || !input_width ||
        !input_height) {
        CMR_LOGE("param error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    ret = camera_local_redisplay_data(
        camera_handle, output_fd, output_addr, output_vir_addr, output_width,
        output_height, input_fd, input_addr_y, input_addr_uv, input_vir_addr,
        input_width, input_height);
    if (ret) {
        CMR_LOGE("failed to redisplay %ld", ret);
    }
exit:
    return ret;
}

cmr_int camera_is_change_size(cmr_handle camera_handle, cmr_u32 cap_width,
                              cmr_u32 cap_height, cmr_u32 preview_width,
                              cmr_u32 preview_height, cmr_u32 video_width,
                              cmr_u32 video_height, cmr_uint *is_change) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)camera_handle;
    struct preview_context *prev_cxt;

    if (!camera_handle || !is_change) {
        CMR_LOGE("param error 0x%lx 0x%lx", (cmr_uint)camera_handle,
                 (cmr_uint)is_change);
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    prev_cxt = &cxt->prev_cxt;
    if (PREVIEWING ==
        cmr_preview_get_status(cxt->prev_cxt.preview_handle, cxt->camera_id)) {
        if (preview_width != prev_cxt->size.width ||
            preview_height != prev_cxt->size.height) {
            *is_change = 1;
            CMR_LOGI("need to change size");
            goto exit;
        }

        if (video_width != prev_cxt->video_size.width ||
            video_height != prev_cxt->video_size.height) {
            *is_change = 1;
            CMR_LOGI("video need to change size");
            goto exit;
        }

        if (CAMERA_ZSL_MODE == cxt->snp_cxt.snp_mode) {
            struct snapshot_context *snp_cxt = &cxt->snp_cxt;
            if (snp_cxt->request_size.width != cap_width ||
                snp_cxt->request_size.height != cap_height) {
                CMR_LOGI("need to change size");
                *is_change = 1;
            }
        }
    }
exit:
    if (is_change != NULL)
        CMR_LOGI("done %ld", *is_change);
    return ret;
}

cmr_int camera_get_preview_rect(cmr_handle camera_handle, cmr_uint *rect_x,
                                cmr_uint *rect_y, cmr_uint *rect_width,
                                cmr_uint *rect_height) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct img_rect rect;

    if (!camera_handle || !rect_x || !rect_y || !rect_width || !rect_height) {
        CMR_LOGE("param error 0x%lx 0x%lx 0x%lx", (cmr_uint)camera_handle,
                 (cmr_uint)rect_width, (cmr_uint)rect_height);
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    ret = camera_local_get_prev_rect(camera_handle, &rect);
    if (ret) {
        CMR_LOGE("failed %ld", ret);
    } else {
        *rect_x = rect.start_x;
        *rect_y = rect.start_y;
        *rect_width = rect.width;
        *rect_height = rect.height;
    }
exit:
    CMR_LOGI("done %ld", ret);
    return ret;
}

cmr_int camera_get_zsl_capability(cmr_handle camera_handle,
                                  cmr_uint *is_support, cmr_uint *max_width,
                                  cmr_uint *max_height) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    if (!camera_handle || !is_support || !max_width || !max_height) {
        CMR_LOGE("error 0x%lx 0x%lx 0x%lx 0x%lx", (cmr_uint)camera_handle,
                 (cmr_uint)is_support, (cmr_uint)max_width,
                 (cmr_uint)max_height);
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    ret = camera_local_get_zsl_info(camera_handle, is_support, max_width,
                                    max_height);

exit:
    CMR_LOGI("done %ld", ret);
    return ret;
}

cmr_int camera_get_sensor_info_for_raw(cmr_handle camera_handle,
                                       struct sensor_mode_info *mode_info) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    if (!camera_handle || !mode_info) {
        CMR_LOGE("error 0x%lx 0x%lx", (cmr_uint)camera_handle,
                 (cmr_uint)mode_info);
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    ret = camera_get_sensor_mode_info(camera_handle, mode_info);

exit:
    CMR_LOGI("done %ld", ret);
    return ret;
}

cmr_int camera_get_sensor_trim(cmr_handle camera_handle,
                               struct img_rect *sn_trim) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    if (!camera_handle || !sn_trim) {
        CMR_LOGE("error 0x%lx 0x%lx", (cmr_uint)camera_handle,
                 (cmr_uint)sn_trim);
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    ret = camera_get_senor_mode_trim(camera_handle, sn_trim);

exit:
    CMR_LOGV("done %ld", ret);
    return ret;
}

cmr_int camera_get_sensor_trim2(cmr_handle camera_handle,
                                struct img_rect *sn_trim) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    if (!camera_handle || !sn_trim) {
        CMR_LOGE("error 0x%lx 0x%lx", (cmr_uint)camera_handle,
                 (cmr_uint)sn_trim);
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    ret = camera_get_senor_mode_trim2(camera_handle, sn_trim);

exit:
    CMR_LOGV("done %ld", ret);
    return ret;
}

cmr_uint camera_get_preview_rot_angle(cmr_handle camera_handle) {
    cmr_uint ret = CMR_CAMERA_SUCCESS;

    if (!camera_handle) {
        CMR_LOGE("error 0x%lx", (cmr_uint)camera_handle);
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    ret = camera_get_preview_angle(camera_handle);

exit:
    CMR_LOGI("done %ld", ret);
    return ret;
}

cmr_uint camera_get_sensor_exif_info(cmr_handle camera_handle,
                                     struct exif_info *exif_info) {
    cmr_uint ret = CMR_CAMERA_SUCCESS;

    if (!camera_handle || !exif_info) {
        CMR_LOGE("error 0x%lx info=0x%lx", (cmr_uint)camera_handle,
                 (cmr_uint)exif_info);
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    ret = camera_get_exif_info(camera_handle, exif_info);

exit:
    CMR_LOGI("done %ld", ret);
    return ret;
}

cmr_uint camera_get_sensor_result_exif_info(
    cmr_handle camera_handle,
    struct exif_spec_pic_taking_cond_tag *exif_pic_info) {
    cmr_uint ret = CMR_CAMERA_SUCCESS;

    if (!camera_handle || !exif_pic_info) {
        CMR_LOGE("error 0x%lx info=0x%lx", (cmr_uint)camera_handle,
                 (cmr_uint)exif_pic_info);
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    ret = camera_get_result_exif_info(camera_handle, exif_pic_info);

exit:
    CMR_LOGV("done %ld", ret);
    return ret;
}

/*
* get dcam iommu status
* return val:
*    0:    has iommu;
*    else: no iommu
*/
cmr_s32 camera_get_iommu_status(cmr_handle camera_handle) {
    return camera_local_get_iommu_status(camera_handle);
}

cmr_int camera_get_rolling_shutter(cmr_handle camera_handle,
                                   cmr_s64 *rolling_shutter_skew) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    if (!camera_handle || !rolling_shutter_skew) {
        CMR_LOGE("error 0x%lx rolling_shutter_skew=0x%lx",
                 (cmr_uint)camera_handle, (cmr_uint)rolling_shutter_skew);
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    ret = camera_get_rolling_shutter_skew_value(camera_handle,
                                                rolling_shutter_skew);
exit:
    return ret;
}

void camera_fd_enable(cmr_handle camera_handle, cmr_u32 is_enable) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)camera_handle;

    CMR_LOGV("%d", is_enable);
    if (camera_handle) {
        cxt->is_support_fd = is_enable;
    } else {
        CMR_LOGE("camera handle is null");
        ret = -CMR_CAMERA_INVALID_PARAM;
    }
}

void camera_lls_enable(cmr_handle camera_handle, cmr_u32 is_enable) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)camera_handle;

    CMR_LOGI("%d", is_enable);
    if (camera_handle) {
        cxt->is_lls_enable = is_enable;
    } else {
        CMR_LOGE("camera handle is null");
        ret = -CMR_CAMERA_INVALID_PARAM;
    }
}

cmr_int camera_is_lls_enabled(cmr_handle camera_handle) {
    cmr_int ret_val = 0;
    struct camera_context *cxt = (struct camera_context *)camera_handle;

    if (camera_handle) {
        ret_val = cxt->is_lls_enable;
    }

    return ret_val;
}

void camera_vendor_hdr_enable(cmr_handle camera_handle, cmr_u32 is_enable) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)camera_handle;

    CMR_LOGI("%d", is_enable);
    if (camera_handle) {
        cxt->is_vendor_hdr = is_enable;
    } else {
        CMR_LOGE("camera handle is null");
        ret = -CMR_CAMERA_INVALID_PARAM;
    }
}

cmr_int camera_is_vendor_hdr(cmr_handle camera_handle) {
    cmr_int ret_val = 0;
    struct camera_context *cxt = (struct camera_context *)camera_handle;

    if (camera_handle) {
        ret_val = cxt->is_vendor_hdr;
    }

    return ret_val;
}

void camera_set_lls_shot_mode(cmr_handle camera_handle, cmr_u32 is_enable) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)camera_handle;

    CMR_LOGI("%d", is_enable);
    if (camera_handle) {
        cxt->lls_shot_mode = is_enable;
    } else {
        CMR_LOGE("camera handle is null");
        ret = -CMR_CAMERA_INVALID_PARAM;
    }
}

cmr_int camera_get_lls_shot_mode(cmr_handle camera_handle) {
    cmr_int ret_val = 0;
    struct camera_context *cxt = (struct camera_context *)camera_handle;

    if (camera_handle) {
        ret_val = cxt->lls_shot_mode;
    }

    return ret_val;
}

void camera_fd_start(cmr_handle camera_handle, cmr_u32 param) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)camera_handle;

    if (camera_handle) {
        cxt->fd_on_off = param;
        ret = camera_local_fd_start(camera_handle);
        if (ret) {
            CMR_LOGE("fail to start fd %ld", ret);
        }
    } else {
        CMR_LOGE("camera handle is null");
        ret = -CMR_CAMERA_INVALID_PARAM;
    }
}

void camera_flip_enable(cmr_handle camera_handle, cmr_u32 param) {
    struct camera_context *cxt = (struct camera_context *)camera_handle;

    CMR_LOGI("%d", param);
    if (camera_handle) {
        cxt->flip_on = param;
    } else {
        CMR_LOGE("camera handle is null");
    }
}

cmr_int camera_is_need_stop_preview(cmr_handle camera_handle) {
    UNUSED(camera_handle);
    cmr_int ret = CMR_CAMERA_SUCCESS;

    return ret;
}

cmr_int camera_takepicture_process(cmr_handle camera_handle,
                                   cmr_uint src_phy_addr, cmr_uint src_vir_addr,
                                   cmr_u32 width, cmr_u32 height) {
    UNUSED(camera_handle);
    UNUSED(src_phy_addr);
    UNUSED(src_vir_addr);
    UNUSED(width);
    UNUSED(height);

    cmr_int ret = CMR_CAMERA_SUCCESS;

    return ret;
}

int camera_get_postprocess_capture_size(cmr_u32 camera_id, cmr_u32 *mem_size) {
    int ret = CMR_CAMERA_SUCCESS;

    ret = camera_get_postproc_capture_size(camera_id, mem_size);
    return ret;
}

uint32_t camera_get_size_align_page(uint32_t size) { return size; }

cmr_int camera_fast_ctrl(cmr_handle camera_handle, enum fast_ctrl_mode mode,
                         cmr_u32 param) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)camera_handle;

    if (!camera_handle) {
        CMR_LOGE("camera handle is null");
        ret = -CMR_CAMERA_INVALID_PARAM;
    }

    switch (mode) {
    case CAMERA_FAST_MODE_FD:
        cxt->fd_on_off = param;
        ret = camera_local_fd_start(camera_handle);
        if (ret) {
            CMR_LOGE("fail to start fd %ld", ret);
        }
        break;

    default:
        break;
    }

    return ret;
}

cmr_int camera_start_preflash(cmr_handle camera_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    if (!camera_handle) {
        CMR_LOGE("camera handle is null");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    ret = camera_local_pre_flash(camera_handle);
    if (ret) {
        CMR_LOGE("failed to cancel snapshot %ld", ret);
    }
exit:
    CMR_LOGI("done");
    return ret;
}

cmr_int camera_get_viewangle(cmr_handle camera_handle,
                             struct sensor_view_angle *view_angle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    if (!camera_handle || !view_angle) {
        CMR_LOGE("Invalid param error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    ret = camera_local_get_viewangle(camera_handle, view_angle);
    if (ret) {
        CMR_LOGE("failed %ld", ret);
    }

exit:
    CMR_LOGI("done %ld", ret);
    return ret;
}
cmr_int camera_set_preview_buffer(cmr_handle camera_handle,
                                  cmr_uint src_phy_addr, cmr_uint src_vir_addr,
                                  cmr_s32 fd) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    if (!camera_handle || !src_vir_addr) {
        CMR_LOGE("Invalid param error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    ret = camera_local_set_preview_buffer(camera_handle, src_phy_addr,
                                          src_vir_addr, fd);
    if (ret) {
        CMR_LOGE("failed %ld", ret);
    }
exit:
    CMR_LOGV("done %ld", ret);
    return ret;
}
cmr_int camera_set_video_buffer(cmr_handle camera_handle, cmr_uint src_phy_addr,
                                cmr_uint src_vir_addr, cmr_s32 fd) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    if (!camera_handle || !fd || !src_vir_addr) {
        CMR_LOGE("Invalid param error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    ret = camera_local_set_video_buffer(camera_handle, src_phy_addr,
                                        src_vir_addr, fd);
    if (ret) {
        CMR_LOGE("failed %ld", ret);
    }
exit:
    CMR_LOGI("done %ld", ret);
    return ret;
}
cmr_int camera_set_zsl_buffer(cmr_handle camera_handle, cmr_uint src_phy_addr,
                              cmr_uint src_vir_addr, cmr_s32 fd) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    if (!camera_handle || !fd || !src_vir_addr) {
        CMR_LOGE("Invalid param error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    ret = camera_local_set_zsl_buffer(camera_handle, src_phy_addr, src_vir_addr,
                                      fd);
    if (ret) {
        CMR_LOGE("failed %ld", ret);
    }
exit:
    CMR_LOGV("done %ld", ret);
    return ret;
}
cmr_int camera_set_video_snapshot_buffer(cmr_handle camera_handle,
                                         cmr_uint src_phy_addr,
                                         cmr_uint src_vir_addr, cmr_s32 fd) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    if (!camera_handle || !fd || !src_vir_addr) {
        CMR_LOGE("Invalid param error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    ret = camera_local_set_video_snapshot_buffer(camera_handle, src_phy_addr,
                                                 src_vir_addr, fd);
    if (ret) {
        CMR_LOGE("failed %ld", ret);
    }
exit:
    CMR_LOGV("done %ld", ret);
    return ret;
}
cmr_int camera_set_zsl_snapshot_buffer(cmr_handle camera_handle,
                                       cmr_uint src_phy_addr,
                                       cmr_uint src_vir_addr, cmr_s32 fd) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    if (!camera_handle || !src_vir_addr) {
        CMR_LOGE("Invalid param error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    ret = camera_local_set_zsl_snapshot_buffer(camera_handle, src_phy_addr,
                                               src_vir_addr, fd);
    if (ret) {
        CMR_LOGE("failed %ld", ret);
    }
exit:
    CMR_LOGV("done %ld", ret);
    return ret;
}

cmr_int camera_zsl_snapshot_need_pause(cmr_handle camera_handle,
                                       cmr_int *flag) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    if (!camera_handle || !flag) {
        CMR_LOGE("Invalid param error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    ret = camera_local_zsl_snapshot_need_pause(camera_handle, flag);
    if (ret) {
        CMR_LOGE("failed %ld", ret);
    }
exit:
    CMR_LOGV("done %ld", ret);
    return ret;
}

cmr_int camera_get_isp_handle(cmr_handle camera_handle,
                              cmr_handle *isp_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)camera_handle;

    if (!camera_handle || !isp_handle) {
        ret = -CMR_CAMERA_INVALID_PARAM;
        CMR_LOGE("param is null, camera handle 0x%p, isp_ptr 0x%p",
                 camera_handle, isp_handle);
    } else {
        *isp_handle = cxt->isp_cxt.isp_handle;
    }

    return ret;
}

cmr_int camera_get_isp_info(cmr_handle camera_handle, void **addr, int *size) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    ret = camera_local_get_isp_info(camera_handle, addr, size);

    return ret;
}

void camera_start_burst_notice(cmr_handle camera_handle) {
    camera_local_start_burst_notice(camera_handle);
}

void camera_end_burst_notice(cmr_handle camera_handle) {
    camera_local_end_burst_notice(camera_handle);
}

cmr_int camera_get_gain_thrs(cmr_handle camera_handle, cmr_u32 *is_over_thrs) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)camera_handle;
    struct setting_context *setting_cxt = &cxt->setting_cxt;
    ret = cmr_sensor_get_gain_thrs(cxt->sn_cxt.sensor_handle, cxt->camera_id,
                                   is_over_thrs);

    return ret;
}

cmr_int
camera_set_sensor_info_to_af(cmr_handle camera_handle,
                             struct cmr_af_aux_sensor_info *sensor_info) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    ret = camera_isp_set_sensor_info_to_af(camera_handle, sensor_info);
    return ret;
}

/**add for 3d capture to reset reprocessing capture size begin*/
cmr_int camera_set_reprocess_picture_size(cmr_handle camera_handle,
                                          cmr_uint is_reprocessing,
                                          cmr_u32 camera_id, cmr_u32 width,
                                          cmr_u32 height) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    ret = camera_local_set_cap_size(camera_handle, is_reprocessing, camera_id,
                                    width, height);
    return ret;
}
/**add for 3d capture to reset reprocessing capture size end*/

cmr_int camera_get_sensor_max_fps(cmr_handle camera_handle, cmr_u32 camera_id,
                                  cmr_u32 *max_fps) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    if (NULL == camera_handle || NULL == max_fps) {
        CMR_LOGE("input param is null!");
        ret = CMR_CAMERA_INVALID_PARAM;
        return ret;
    }
    ret = cmr_get_sensor_max_fps(camera_handle, camera_id, max_fps);
    return ret;
}

cmr_int camera_snapshot_is_need_flash(cmr_handle oem_handle, cmr_u32 camera_id,
                                      cmr_u32 *is_need_flash) {
    camera_local_snapshot_is_need_flash(oem_handle, camera_id, is_need_flash);
    return 0;
}

cmr_int camera_set_3dnr_video_mode(cmr_handle camera_handle,
                                   cmr_uint is_3dnr_video) {
    camera_set_3dnr_video(camera_handle, is_3dnr_video);
    return 0;
}

cmr_uint camera_get_sensor_otp_info(cmr_handle camera_handle,
                                    struct sensor_otp_cust_info *otp_info) {
    cmr_uint ret = CMR_CAMERA_SUCCESS;

    if (!camera_handle || !otp_info) {
        CMR_LOGE("error 0x%lx info=%p", (cmr_uint)camera_handle, otp_info);
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    ret = camera_get_otpinfo(camera_handle, otp_info);
    CMR_LOGV("otp_info data_ptr %p size %d", otp_info->total_otp.data_ptr,
             otp_info->total_otp.size);

exit:
    CMR_LOGI("done %ld", ret);
    return ret;
}

cmr_uint camera_get_sensor_vcm_step(cmr_handle camera_handle, cmr_u32 camera_id,
                                    cmr_u32 *vcm_step) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    if (NULL == camera_handle) {
        CMR_LOGE("input param is null!");
        ret = CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    ret = cmr_get_sensor_vcm_step(camera_handle, camera_id, vcm_step);
    CMR_LOGI("vcm_step %d", *vcm_step);
    return ret;

exit:
    CMR_LOGI("done %ld", ret);
    return ret;
}

cmr_int camera_set_largest_picture_size(cmr_u32 camera_id, cmr_u16 width,
                                        cmr_u16 height) {
    cmr_int ret = 0;

    ret = camera_set_largest_pict_size(camera_id, width, height);

    return ret;
}

cmr_int dump_jpeg_file(void *virt_addr, unsigned int size, int width,
                       int height) {
    char str_buf[100];
    FILE *fp;
    char tmp_str[20] = {0};
    char datetime[100];
    time_t timep;
    struct tm *p;
    time(&timep);
    p = localtime(&timep);
    snprintf(datetime, sizeof(str_buf), "%04d%02d%02d%02d%02d%02d",
             (1900 + p->tm_year), (1 + p->tm_mon), p->tm_mday, p->tm_hour,
             p->tm_min, p->tm_sec);

    strcpy(str_buf, CAMERA_DUMP_PATH);
    strcat(datetime, "_");
    sprintf(tmp_str, "%d", width);
    strcat(datetime, tmp_str);
    strcat(datetime, "X");
    sprintf(tmp_str, "%d", height);
    strcat(datetime, tmp_str);
    strcat(str_buf, datetime);
    strcat(str_buf, ".jpg");

    CMR_LOGD("file name %s", str_buf);
    fp = fopen(str_buf, "ab+");
    if (NULL == fp) {
        printf("open %s failed\n", str_buf);
        goto exit;
    }
    fwrite((uint8_t *)virt_addr, 1, size, fp);
    fflush(fp);
    fclose(fp);
exit:
    return 0;
}

cmr_int camera_set_sensor_close_flag(cmr_handle camera_handle) {
    camera_local_set_sensor_close_flag(camera_handle);
    return 0;
}

cmr_int camera_start_capture(cmr_handle camera_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    if (!camera_handle) {
        CMR_LOGE("camera handle is null");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    ret = camera_local_start_capture(camera_handle);
    if (ret) {
        CMR_LOGE("failed to start snapshot %ld", ret);
        goto exit;
    }

exit:
    return ret;
}

cmr_int camera_stop_capture(cmr_handle camera_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    if (!camera_handle) {
        CMR_LOGE("camera handle is null");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    ret = camera_local_stop_capture(camera_handle);
    if (ret) {
        CMR_LOGE("failed to start snapshot %ld", ret);
        goto exit;
    }

exit:
    return ret;
}

cmr_int camera_ioctrl(cmr_handle handle, int cmd, void *param) {
    cmr_int ret = 0;

    switch (cmd) {
    case CAMERA_IOCTRL_SET_MULTI_CAMERAMODE: {
        multiCameraMode *camera_mode = (multiCameraMode *)param;
        CMR_LOGD("camera_mode %d", *camera_mode);
        camera_set_mem_multimode(*camera_mode);
        camera_set_oem_multimode(*camera_mode);
        break;
    }
    case CAMERA_IOCTRL_GET_SENSOR_LUMA: {
        ret = camera_local_get_cover(handle, (cmr_u32 *)param);
        break;
    }
    case CAMERA_IOCTRL_COVERED_SENSOR_STREAM_CTRL:
        CMR_LOGD("force sensor stream on %u", *(uint32_t *)param);
        ret = camera_stream_ctrl(handle, *(uint32_t *)param);
        break;
    case CAMERA_IOCTRL_GET_FULLSCAN_INFO: {
        ret = cmr_get_isp_af_fullscan(handle,
                                      (struct isp_af_fullscan_info *)param);
        break;
    }
    case CAMERA_IOCTRL_SET_AF_POS: {
        ret = cmr_set_af_pos(handle, *(uint32_t *)param);
        break;
    }
    case CAMERA_IOCTRL_SET_3A_BYPASS: {
        ret = cmr_set_3a_bypass(handle, *(uint32_t *)param);
        break;
    }
    case CAMERA_IOCTRL_GET_AE_FPS: {
        ret = cmr_get_ae_fps(handle, (cmr_u32 *)param);
        break;
    }
    case CAMERA_IOCTRL_SET_SNAPSHOT_TIMESTAMP: {
        ret = cmr_set_snapshot_timestamp(handle, *(int64_t *)param);
        break;
    }
    case CAMERA_IOCTRL_3DNR_VIDEOMODE: {
        ret = camera_set_3dnr_video(handle, *(cmr_uint *)param);
        break;
    }
    case CAMERA_IOCTRL_GET_CPP_CAPABILITY: {
        ret = camera_local_get_cpp_capability(
            handle, &(((struct img_size *)param)->width),
            &(((struct img_size *)param)->height));
        break;
    }
    case CAMERA_IOCTRL_THUMB_YUV_PROC: {
        ret = camera_set_thumb_yuv_proc(handle,
                                        (struct snp_thumb_yuv_param *)param);
        break;
    }
    case CAMERA_IOCTRL_JPEG_ENCODE_EXIF_PROC: {
        ret = camera_jpeg_encode_exif_simplify(handle,
                                               (struct enc_exif_param *)param);
        break;
    }
    case CAMERA_IOCTRL_SET_CAPTURE_FACE_BEAUTIFY: {
        ret = camera_local_set_capture_fb(handle, (cmr_u32 *)param);
        break;
    }
    case CAMERA_IOCTRL_DEBUG_IMG: {
        ret = cmr_img_debug(param, NULL);
        break;
    }
    case CAMERA_IOCTRL_GET_GRAB_CAPABILITY:
        ret = camera_get_grab_capability(handle,
                                         (struct cmr_path_capability *)param);
        break;
    default:
        break;
    }

    return ret;
}

cmr_int camera_reprocess_yuv_for_jpeg(cmr_handle camera_handle,
                                      enum takepicture_mode cap_mode,
                                      struct frm_info *frm_data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    if (!camera_handle) {
        CMR_LOGE("camera handle is null");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    ret =
        camera_local_reprocess_yuv_for_jpeg(camera_handle, cap_mode, frm_data);
    if (ret) {
        CMR_LOGE("failed to start snapshot %ld", ret);
    }

exit:
    CMR_LOGI("done");
    return ret;
}

#if defined(CONFIG_ISP_2_1)
cmr_int camera_get_focus_point(cmr_handle camera_handle, cmr_s32 *point_x,
                               cmr_s32 *point_y) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    return ret;
}

cmr_s32 camera_isp_sw_check_buf(cmr_handle camera_handle, cmr_uint *param_ptr) {
    cmr_s32 ret = CMR_CAMERA_SUCCESS;
    return ret;
}

cmr_int camera_isp_sw_proc(cmr_handle camera_handle,
                           struct soft_isp_frm_param *param_ptr) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    return ret;
}

cmr_int camera_raw_post_proc(cmr_handle camera_handle, struct img_frm *raw_buff,
                             struct img_frm *yuv_buff,
                             struct img_sbs_info *sbs_info) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    return ret;
}

cmr_int camera_get_tuning_param(cmr_handle camera_handle,
                                struct tuning_param_info *tuning_info) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    return ret;
}
#endif

cmr_int camera_set_gpu_mem_ops(cmr_handle camera_handle, void *cb_of_malloc,
                               void *cb_of_free) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    ret = camera_local_set_gpu_mem_ops(camera_handle, cb_of_malloc, cb_of_free);
    if (ret) {
        ret = -CMR_CAMERA_FAIL;
        CMR_LOGE("failed to set camera gpu callback %ld", ret);
    }

    return ret;
}

static oem_ops_t oem_module_ops = {
    camera_init, camera_deinit, camera_release_frame, camera_set_param,
    camera_start_preview, camera_stop_preview, camera_start_autofocus,
    camera_cancel_autofocus, camera_cancel_takepicture,
    // camera_safe_scale_th,
    NULL, camera_take_picture, camera_get_sn_trim, camera_set_mem_func,
    camera_get_redisplay_data, camera_is_change_size,
    camera_get_postprocess_capture_size, camera_get_preview_rect,
    camera_get_zsl_capability, camera_get_sensor_info_for_raw,
    camera_get_sensor_trim, camera_get_sensor_trim2,
    camera_get_preview_rot_angle, camera_fd_enable, camera_flip_enable,
    camera_fd_start, camera_is_need_stop_preview, camera_takepicture_process,
    camera_get_size_align_page, camera_fast_ctrl, camera_start_preflash,
    camera_get_viewangle, camera_get_sensor_exif_info,
    camera_get_sensor_result_exif_info, camera_get_iommu_status,
    camera_set_preview_buffer, camera_set_video_buffer, camera_set_zsl_buffer,
    camera_set_video_snapshot_buffer, camera_set_zsl_snapshot_buffer,
    camera_zsl_snapshot_need_pause, camera_get_isp_handle, camera_lls_enable,
    camera_is_lls_enabled, camera_vendor_hdr_enable, camera_is_vendor_hdr,
    camera_set_lls_shot_mode, camera_get_lls_shot_mode, camera_get_isp_info,
    camera_start_burst_notice, camera_end_burst_notice,
    camera_transfer_caf_to_af, camera_transfer_af_to_caf, dump_jpeg_file,
    camera_get_gain_thrs, camera_set_sensor_info_to_af,
    camera_get_sensor_max_fps, camera_snapshot_is_need_flash,
    camera_get_sensor_otp_info, camera_get_sensor_vcm_step,
    camera_set_sensor_close_flag, camera_set_reprocess_picture_size,
    camera_start_capture, camera_stop_capture,
    camera_set_largest_picture_size, camera_ioctrl,
    camera_reprocess_yuv_for_jpeg,
#if defined(CONFIG_ISP_2_1)
    camera_get_focus_point, camera_isp_sw_check_buf, camera_isp_sw_proc,
    camera_raw_post_proc, camera_get_tuning_param,
#endif
#if defined(CONFIG_ISP_2_3)
    camera_set_gpu_mem_ops,
#endif
    camera_get_rolling_shutter};

struct oem_module OEM_MODULE_INFO_SYM = {
    .tag = 0, .ops = &oem_module_ops, .dso = NULL};
