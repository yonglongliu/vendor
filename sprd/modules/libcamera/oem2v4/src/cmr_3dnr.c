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

#ifdef CONFIG_CAMERA_3DNR_CAPTURE

#define LOG_TAG "cmr_3dnr"
#include <cutils/trace.h>
#include "cmr_msg.h"
#include "cmr_ipm.h"
#include "cmr_common.h"
#include "cmr_sensor.h"
#include "cmr_oem.h"
#include "interface.h"
#include <cutils/properties.h>
#include "isp_mw.h"

struct thread_3dnr_info {
    cmr_handle class_handle;
    struct ipm_frame_in in;
    struct ipm_frame_out out;
};

struct class_3dnr_pre { // 3dnr pre
    struct ipm_common common;
    cmr_uint mem_size;
    cmr_uint width;
    cmr_uint height;
    cmr_uint is_inited;
    cmr_handle threednr_thread;
    struct img_addr dst_addr;
    ipm_callback reg_cb;
    struct ipm_frame_in frame_in;
    cmr_uint pre_buf_phy[PRE_3DNR_NUM];
    cmr_uint pre_buf_vir[PRE_3DNR_NUM];
    cmr_s32 pre_buf_fd[PRE_3DNR_NUM];
    cmr_u32 g_num;
    cmr_uint is_stop;
};

struct class_3dnr {
    struct ipm_common common;
    cmr_u8 *alloc_addr[CAP_3DNR_NUM];
    struct thread_3dnr_info g_info_3dnr[CAP_3DNR_NUM];
    cmr_uint mem_size;
    cmr_uint width;
    cmr_uint height;
    cmr_uint small_width;
    cmr_uint small_height;
    cmr_uint is_inited;
    cmr_handle threednr_thread;
    struct img_addr dst_addr;
    ipm_callback reg_cb;
    struct ipm_frame_in frame_in;
    struct c3dnr_buffer out_buf;
    cmr_uint out_buf_phy;
    cmr_uint out_buf_vir;
    cmr_uint small_buf_phy[CAP_3DNR_NUM];
    cmr_uint small_buf_vir[CAP_3DNR_NUM];
    cmr_s32 out_buf_fd;
    cmr_s32 small_buf_fd[CAP_3DNR_NUM];
    sem_t sem_3dnr;
    cmr_u32 g_num;
    cmr_uint is_stop;
};

#define CHECK_HANDLE_VALID(handle)                                             \
    do {                                                                       \
        if (!handle) {                                                         \
            return -CMR_CAMERA_INVALID_PARAM;                                  \
        }                                                                      \
    } while (0)

#define IMAGE_FORMAT "YVU420_SEMIPLANAR"
#define CAMERA_3DNR_MSG_QUEUE_SIZE 5

#define CMR_EVT_3DNR_BASE (CMR_EVT_IPM_BASE + 0X100)
#define CMR_EVT_3DNR_INIT (CMR_EVT_3DNR_BASE + 0)
#define CMR_EVT_3DNR_START (CMR_EVT_3DNR_BASE + 1)
#define CMR_EVT_3DNR_EXIT (CMR_EVT_3DNR_BASE + 2)
#define CMR_EVT_3DNR_SAVE_FRAME (CMR_EVT_3DNR_BASE + 3)

static cmr_int threednr_open(cmr_handle ipm_handle, struct ipm_open_in *in,
                             struct ipm_open_out *out,
                             cmr_handle *class_handle);
static cmr_int threednr_close(cmr_handle class_handle);
static cmr_int threednr_transfer_frame(cmr_handle class_handle,
                                       struct ipm_frame_in *in,
                                       struct ipm_frame_out *out);
static cmr_int threednr_post_proc(cmr_handle class_handle);
static cmr_int threednr_open_prev(cmr_handle ipm_handle, struct ipm_open_in *in,
                                  struct ipm_open_out *out,
                                  cmr_handle *class_handle);
static cmr_int threednr_close_prev(cmr_handle class_handle);
static cmr_int threednr_transfer_prev_frame(cmr_handle class_handle,
                                            struct ipm_frame_in *in,
                                            struct ipm_frame_out *out);
static cmr_int req_3dnr_do(cmr_handle class_handle, struct img_addr *dst_addr,
                           struct img_size frame_size);
static cmr_int threednr_process_thread_proc(struct cmr_msg *message,
                                            void *private_data);
static cmr_int threednr_thread_create(struct class_3dnr *class_handle);
static cmr_int threednr_thread_destroy(struct class_3dnr *class_handle);
static cmr_int threednr_save_frame(cmr_handle class_handle,
                                   struct ipm_frame_in *in);
static cmr_int req_3dnr_save_frame(cmr_handle class_handle,
                                   struct ipm_frame_in *in);
static cmr_int save_yuv(char *filename, char *buffer, uint32_t width,
                        uint32_t height);

static struct class_ops threednr_ops_tab_info = {threednr_open, threednr_close,
                                                 threednr_transfer_frame, NULL,
                                                 threednr_post_proc};

struct class_tab_t threednr_tab_info = {
    &threednr_ops_tab_info,
};

static struct class_ops threednr_prev_ops_tab_info = {
    threednr_open_prev, threednr_close_prev, threednr_transfer_prev_frame, NULL,
    threednr_post_proc};

struct class_tab_t threednr_prev_tab_info = {
    &threednr_prev_ops_tab_info,
};

cmr_int isp_ioctl_for_3dnr(cmr_handle isp_handle, c3dnr_io_info_t *io_info) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct isp_3dnr_info isp_3dnr;

    isp_3dnr.image[0].buffer = io_info->image[0].bufferY;
    isp_3dnr.image[0].fd = io_info->image[0].fd;
    isp_3dnr.image[1].buffer = io_info->image[1].bufferY;
    isp_3dnr.image[1].fd = io_info->image[1].fd;
    isp_3dnr.image[2].buffer = io_info->image[2].bufferY;
    isp_3dnr.image[2].fd = io_info->image[2].fd;
    isp_3dnr.width = io_info->width;
    isp_3dnr.height = io_info->height;
    isp_3dnr.mv_x = io_info->mv_x;
    isp_3dnr.mv_y = io_info->mv_y;
    isp_3dnr.blending_no = io_info->blending_no;

    ret = isp_ioctl(isp_handle, ISP_CTRL_POST_3DNR, (void *)&isp_3dnr);

    return ret;
}

static cmr_int threednr_open(cmr_handle ipm_handle, struct ipm_open_in *in,
                             struct ipm_open_out *out,
                             cmr_handle *class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_3dnr *threednr_handle = NULL;
    cmr_uint size;
    cmr_int i = 0;
    struct c3dnr_param_info param;
    struct ipm_context_t *ipm_cxt = (struct ipm_context_t *)ipm_handle;
    cmr_handle oem_handle = NULL;
    struct ipm_init_in *ipm_in = (struct ipm_init_in *)&ipm_cxt->init_in;
    struct camera_context *cam_cxt = NULL;
    struct isp_context *isp_cxt = NULL;
    cmr_u32 buf_size;
    cmr_u32 buf_num;
    cmr_u32 small_buf_size;
    cmr_u32 small_buf_num;

    CMR_LOGI("E");

    if (!out || !in || !ipm_handle || !class_handle) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }

    threednr_handle = (struct class_3dnr *)malloc(sizeof(struct class_3dnr));
    if (!threednr_handle) {
        CMR_LOGE("No mem!");
        return CMR_CAMERA_NO_MEM;
    }

    cmr_bzero(threednr_handle, sizeof(struct class_3dnr));

    sem_init(&threednr_handle->sem_3dnr, 0, 1);

    out->format = IMG_FMT_YCBCR420;
    out->total_frame_number = CAP_3DNR_NUM;

    size = (cmr_uint)(in->frame_size.width * in->frame_size.height * 3 / 2);

    CMR_LOGD("in->frame_size.width = %d,in->frame_size.height = %d",
             in->frame_size.width, in->frame_size.height);

    threednr_handle->common.ipm_cxt = (struct ipm_context_t *)ipm_handle;
    threednr_handle->common.class_type = IPM_TYPE_3DNR;
    threednr_handle->common.receive_frame_count = 0;
    threednr_handle->common.save_frame_count = 0;
    threednr_handle->common.ops = &threednr_ops_tab_info;

    threednr_handle->mem_size = size;

    threednr_handle->height = in->frame_size.height;
    threednr_handle->width = in->frame_size.width;
    threednr_handle->small_height = threednr_handle->height / 2;
    threednr_handle->small_width = threednr_handle->width / 2;
    threednr_handle->reg_cb = in->reg_cb;
    threednr_handle->g_num = 0;
    threednr_handle->is_stop = 0;

    ret = threednr_thread_create(threednr_handle);
    if (ret) {
        CMR_LOGE("3dnr error: create thread");
        goto free_all;
    }

    oem_handle = threednr_handle->common.ipm_cxt->init_in.oem_handle;
    cam_cxt = (struct camera_context *)oem_handle;
    isp_cxt = (struct isp_context *)&(cam_cxt->isp_cxt);
    buf_size = threednr_handle->width * threednr_handle->height * 3 / 2;
    buf_num = 1;
    small_buf_size =
        threednr_handle->small_width * threednr_handle->small_height * 3 / 2;
    small_buf_num = CAP_3DNR_NUM;
    if (NULL != cam_cxt->hal_malloc) {
        if (0 !=
            cam_cxt->hal_malloc(
                CAMERA_SNAPSHOT_3DNR_DST, &buf_size, &buf_num,
                &threednr_handle->out_buf_phy, &threednr_handle->out_buf_vir,
                &threednr_handle->out_buf_fd, cam_cxt->client_data)) {
            CMR_LOGE("Fail to alloc mem for 3DNR out");
        } else {
            CMR_LOGD("OK to alloc mem for 3DNR out");
        }
        if (0 !=
            cam_cxt->hal_malloc(
                CAMERA_SNAPSHOT_3DNR, &small_buf_size, &small_buf_num,
                (cmr_uint *)threednr_handle->small_buf_phy,
                (cmr_uint *)threednr_handle->small_buf_vir,
                threednr_handle->small_buf_fd, cam_cxt->client_data)) {
            CMR_LOGE("Fail to malloc buffers for small image");
        } else {
            CMR_LOGD("OK to malloc buffers for small image");
        }
    } else {
        CMR_LOGE("cam_cxt->hal_malloc is NULL");
    }
    for (int i = 0; i < 2; i++) {
        memset(&param.out_img[i], 0, sizeof(struct c3dnr_buffer));
    }

    threednr_handle->out_buf.bufferY =
        (unsigned char *)threednr_handle->out_buf_vir;
    threednr_handle->out_buf.bufferU =
        threednr_handle->out_buf.bufferY +
        threednr_handle->width * threednr_handle->height;
    threednr_handle->out_buf.bufferV = threednr_handle->out_buf.bufferU;
    threednr_handle->out_buf.fd = threednr_handle->out_buf_fd;
    param.orig_width = threednr_handle->width;
    param.orig_height = threednr_handle->height;
    param.small_width = threednr_handle->small_width;
    param.small_height = threednr_handle->small_height;
    param.total_frame_num = CAP_3DNR_NUM;
    param.poutimg = &threednr_handle->out_buf;
    param.isp_handle = isp_cxt->isp_handle;
    param.isp_ioctrl = isp_ioctl_for_3dnr;
    if (NULL == param.isp_handle) {
        CMR_LOGE("isp_handle is null");
    }
    ret = threednr_init(&param);
    if (ret < 0) {
        CMR_LOGE("Fail to call threednr_init");
    } else {
        CMR_LOGD("ok to call threednr_init");
    }

    *class_handle = (cmr_handle)threednr_handle;
    CMR_LOGI("X");

    return ret;

free_all:
    if (NULL != threednr_handle)
        free(threednr_handle);
    return CMR_CAMERA_NO_MEM;
}

static cmr_int threednr_close(cmr_handle class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_3dnr *threednr_handle = (struct class_3dnr *)class_handle;
    cmr_int i;
    cmr_handle oem_handle = NULL;
    struct camera_context *cam_cxt = NULL;

    CMR_LOGI("E");
    CHECK_HANDLE_VALID(threednr_handle);

    oem_handle = threednr_handle->common.ipm_cxt->init_in.oem_handle;
    cam_cxt = (struct camera_context *)oem_handle;
    threednr_handle->is_stop = 1;

    threednr_cancel();
    CMR_LOGI("OK to threednr_cancel");

    ret = threednr_deinit();
    if (ret) {
        CMR_LOGE("threednr_deinit timeout or failed");
        sem_destroy(&threednr_handle->sem_3dnr);
        goto exit;
    }

    CMR_LOGI("OK to threednr_deinit");

    sem_destroy(&threednr_handle->sem_3dnr);

    if (cam_cxt->hal_free == NULL) {
        CMR_LOGE("cam_cxt->hal_free is NULL");
        goto exit;
    }

    ret = cam_cxt->hal_free(
                        CAMERA_SNAPSHOT_3DNR_DST, &threednr_handle->out_buf_phy,
                        &threednr_handle->out_buf_vir, &threednr_handle->out_buf_fd, 1,
                        cam_cxt->client_data);
    if (ret) {
        CMR_LOGE("Fail to free the output buffer");
    }

    ret = cam_cxt->hal_free(CAMERA_SNAPSHOT_3DNR,
                            (cmr_uint *)threednr_handle->small_buf_phy,
                            (cmr_uint *)threednr_handle->small_buf_vir,
                            threednr_handle->small_buf_fd, CAP_3DNR_NUM,
                            cam_cxt->client_data);
    if (ret) {
        CMR_LOGE("Fail to free the small image buffers");
    }

exit:
    ret = threednr_thread_destroy(threednr_handle);
    if (ret) {
        CMR_LOGE("3dnr failed to destroy 3dnr thread");
    }

    sem_post(&cam_cxt->threednr_proc_sm);

    if (NULL != threednr_handle)
        free(threednr_handle);

    CMR_LOGI("X");

    return ret;
}

cmr_int threednr_open_prev(cmr_handle ipm_handle, struct ipm_open_in *in,
                           struct ipm_open_out *out, cmr_handle *class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_3dnr_pre *threednr_prev_handle = NULL;
    cmr_uint size;
    cmr_int i = 0;
    struct c3dnr_param_info param;
    cmr_handle oem_handle = NULL;
    struct camera_context *cam_cxt = NULL;
    struct preview_context *prev_cxt = NULL;
    struct isp_context *isp_cxt = NULL;
    cmr_u32 buf_size;
    cmr_u32 buf_num;
    cmr_u32 small_buf_size;
    cmr_u32 small_buf_num;

    CMR_LOGI("E");

    if (!out || !in || !ipm_handle || !class_handle) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }

    threednr_prev_handle =
        (struct class_3dnr_pre *)malloc(sizeof(struct class_3dnr_pre));
    if (!threednr_prev_handle) {
        CMR_LOGE("No mem!");
        return CMR_CAMERA_NO_MEM;
    }

    cmr_bzero(threednr_prev_handle, sizeof(struct class_3dnr_pre));

    size = (cmr_uint)(in->frame_size.width * in->frame_size.height * 3 / 2);

    CMR_LOGD("in->frame_size.width = %d,in->frame_size.height = %d",
             in->frame_size.width, in->frame_size.height);

    threednr_prev_handle->common.ipm_cxt = (struct ipm_context_t *)ipm_handle;
    threednr_prev_handle->common.class_type = IPM_TYPE_3DNR_PRE;
    threednr_prev_handle->common.ops = &threednr_prev_ops_tab_info;

    oem_handle = threednr_prev_handle->common.ipm_cxt->init_in.oem_handle;
    cam_cxt = (struct camera_context *)oem_handle;
    prev_cxt = (struct preview_context *)&cam_cxt->prev_cxt;

    threednr_prev_handle->mem_size = size;
    threednr_prev_handle->height = in->frame_size.height;
    threednr_prev_handle->width = in->frame_size.width;
    threednr_prev_handle->is_stop = 0;

    isp_cxt = (struct isp_context *)&(cam_cxt->isp_cxt);
    buf_size =
        threednr_prev_handle->width * threednr_prev_handle->height * 3 / 2;
    buf_num = PRE_3DNR_NUM;
    if (NULL != cam_cxt->hal_malloc) {
        if (0 !=
            cam_cxt->hal_malloc(CAMERA_PREVIEW_3DNR, &buf_size, &buf_num,
                                threednr_prev_handle->pre_buf_phy,
                                threednr_prev_handle->pre_buf_vir,
                                threednr_prev_handle->pre_buf_fd,
                                cam_cxt->client_data)) {
            CMR_LOGE("Fail to alloc mem for 3DNR preview");
        } else {
            CMR_LOGD("OK to alloc mem for 3DNR preview");
        }
    } else {
        CMR_LOGE("cam_cxt->hal_malloc is NULL");
    }

    param.orig_width = threednr_prev_handle->width;
    param.orig_height = threednr_prev_handle->height;
    param.small_width = threednr_prev_handle->width;
    param.small_height = threednr_prev_handle->height;
    param.total_frame_num = PRE_3DNR_NUM;
    for (int i = 0; i < PRE_3DNR_NUM; i++) {
        param.out_img[i].bufferY =
            (uint8_t *)threednr_prev_handle->pre_buf_vir[i];
        param.out_img[i].bufferU =
            param.out_img[i].bufferY +
            threednr_prev_handle->width * threednr_prev_handle->height;
        param.out_img[i].bufferV = param.out_img[i].bufferU;
        param.out_img[i].fd = threednr_prev_handle->pre_buf_fd[i];
    }
    param.poutimg = &param.out_img[0];
    param.isp_handle = isp_cxt->isp_handle;
    param.isp_ioctrl = isp_ioctl_for_3dnr;
    if (NULL == param.isp_handle) {
        CMR_LOGE("isp_handle is null");
    }

    ret = threednr_init(&param);
    if (ret < 0) {
        CMR_LOGE("Fail to call preview threednr_init");
    } else {
        CMR_LOGD("ok to call preview threednr_init");
    }
    *class_handle = (cmr_handle)threednr_prev_handle;

    CMR_LOGI("X");

    return ret;
}

cmr_int threednr_close_prev(cmr_handle class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_3dnr_pre *threednr_prev_handle =
        (struct class_3dnr_pre *)class_handle;
    cmr_int i;
    cmr_handle oem_handle = NULL;
    struct camera_context *cam_cxt = NULL;

    CMR_LOGI("E");
    CHECK_HANDLE_VALID(threednr_prev_handle);

    threednr_prev_handle->is_stop = 1;
    threednr_cancel();
    CMR_LOGI("OK to threednr_cancel for preview");
    ret = threednr_deinit();
    if (ret) {
        CMR_LOGE("3dnr preview failed to threednr_deinit");
    }

    oem_handle = threednr_prev_handle->common.ipm_cxt->init_in.oem_handle;
    cam_cxt = (struct camera_context *)oem_handle;

    if (NULL != cam_cxt->hal_malloc) {
        if (0 !=
            cam_cxt->hal_free(CAMERA_PREVIEW_3DNR,
                              threednr_prev_handle->pre_buf_phy,
                              threednr_prev_handle->pre_buf_vir,
                              threednr_prev_handle->pre_buf_fd, PRE_3DNR_NUM,
                              cam_cxt->client_data)) {
            CMR_LOGE("Fail to free the preview output buffer");
        } else {
            CMR_LOGD("OK to free thepreview  output buffer");
        }
    } else {
        CMR_LOGD("cam_cxt->hal_free is NULL");
    }

    if (NULL != threednr_prev_handle)
        free(threednr_prev_handle);

    CMR_LOGI("X");

    return ret;
}

cmr_int threednr_transfer_prev_frame(cmr_handle class_handle,
                                     struct ipm_frame_in *in,
                                     struct ipm_frame_out *out)

{
    cmr_int ret = CMR_CAMERA_SUCCESS;
    c3dnr_buffer_t image;
    struct class_3dnr_pre *threednr_prev_handle =
        (struct class_3dnr_pre *)class_handle;

    image.bufferY = (unsigned char *)in->src_frame.addr_vir.addr_y;
    image.bufferU =
        image.bufferY + in->src_frame.size.width * in->src_frame.size.height;
    image.bufferV = image.bufferU;
    image.fd = in->src_frame.fd;
    CMR_LOGV(" in buf:Y  %p, U %p,fd 0x%x", image.bufferY, image.bufferU,
             image.fd);

    if (threednr_prev_handle->is_stop) {
        return ret;
    }

    ret = threednr_function_pre(&image, &image);
    if (ret) {
        CMR_LOGE("3dnr failed to threednr_pre_function");
    }

    return ret;
}

static cmr_int threednr_post_proc(cmr_handle class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_3dnr *threednr_handle = (struct class_3dnr *)class_handle;

    threednr_callback();
    CMR_LOGV("call back done ,threednr_handle->g_num %d",
             threednr_handle->g_num);
    return ret;
}

static cmr_int threednr_start_scale(cmr_handle oem_handle, struct img_frm *src,
                                    struct img_frm *dst) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    if (!oem_handle || !src || !dst) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    CMR_LOGD("src fd 0x%x , dst fd 0x%x", src->fd, dst->fd);
    CMR_LOGD("src 0x%lx 0x%lx , dst 0x%lx 0x%lx", src->addr_phy.addr_y,
             src->addr_phy.addr_u, dst->addr_phy.addr_y, dst->addr_phy.addr_u);
    CMR_LOGV(
        "src size %d %d dst size %d %d rect %d %d %d %d endian %d %d %d %d",
        src->size.width, src->size.height, dst->size.width, dst->size.height,
        src->rect.start_x, src->rect.start_y, src->rect.width, src->rect.height,
        src->data_end.y_endian, src->data_end.uv_endian, dst->data_end.y_endian,
        dst->data_end.uv_endian);

    ret = cmr_scale_start(cxt->scaler_cxt.scaler_handle, src, dst,
                          (cmr_evt_cb)NULL, NULL);
    if (ret) {
        CMR_LOGE("failed to start scaler, ret %ld", ret);
    }
exit:
    CMR_LOGI("done %ld", ret);

    ATRACE_END();
    return ret;
}

static cmr_int req_3dnr_save_frame(cmr_handle class_handle,
                                   struct ipm_frame_in *in) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_3dnr *threednr_handle = (struct class_3dnr *)class_handle;

    CMR_MSG_INIT(message);

    if (!class_handle || !in) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }

    threednr_handle->common.save_frame_count++;
    if (threednr_handle->common.save_frame_count > CAP_3DNR_NUM) {
        CMR_LOGE("cap cnt error,%ld", threednr_handle->common.save_frame_count);
        return CMR_CAMERA_FAIL;
    }

    message.data = (struct ipm_frame_in *)malloc(sizeof(struct ipm_frame_in));
    if (!message.data) {
        CMR_LOGE("No mem!");
        ret = CMR_CAMERA_NO_MEM;
        return ret;
    }
    memcpy(message.data, in, sizeof(struct ipm_frame_in));
    message.msg_type = CMR_EVT_3DNR_SAVE_FRAME;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    message.alloc_flag = 1;
    ret = cmr_thread_msg_send(threednr_handle->threednr_thread, &message);
    if (ret) {
        CMR_LOGE("Failed to send one msg to 3dnr thread");
        if (message.data) {
            free(message.data);
        }
    }
    return ret;
}

static cmr_int save_yuv(char *filename, char *buffer, uint32_t width,
                        uint32_t height) {
    FILE *fp;
    fp = fopen(filename, "wb");
    if (fp) {
        fwrite(buffer, 1, width * height * 3 / 2, fp);
        fclose(fp);
        return 0;
    } else
        return -1;
}

void *thread_3dnr(void *p_data) {
    struct thread_3dnr_info *info = (struct thread_3dnr_info *)p_data;
    struct ipm_frame_in *in = &info->in;
    struct ipm_frame_out *out = &info->out;
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_3dnr *threednr_handle =
        (struct class_3dnr *)info->class_handle;
    cmr_u32 frame_in_cnt;
    struct img_addr *addr;
    struct img_size size;
    cmr_handle oem_handle;
    cmr_u32 sensor_id = 0;
    cmr_u32 threednr_enable = 0;
    struct c3dnr_buffer big_buf, small_buf;
    struct img_frm *src, dst;
    cmr_u32 cur_frm;
    char filename[128];

    sem_wait(&threednr_handle->sem_3dnr);

    cur_frm = threednr_handle->common.save_frame_count;
    CMR_LOGD("wait sem. cur_frm: %d", cur_frm);

    CMR_LOGI("ipm_frame_in.private_data 0x%lx", (cmr_int)in->private_data);
    addr = &in->dst_frame.addr_vir;
    size = in->src_frame.size;

    ret = req_3dnr_save_frame(threednr_handle, in);
    if (ret != CMR_CAMERA_SUCCESS) {
        CMR_LOGE("req_dnr_save_frame fail");
        return NULL;
    }

    oem_handle = threednr_handle->common.ipm_cxt->init_in.oem_handle;
    src = &in->src_frame;
    src->data_end.y_endian = 0;
    src->data_end.uv_endian = 0;
    src->rect.start_x = 0;
    src->rect.start_y = 0;
    src->rect.width = threednr_handle->width;
    src->rect.height = threednr_handle->height;
    memcpy(&dst, &in->src_frame, sizeof(struct img_frm));
    dst.buf_size =
        threednr_handle->small_width * threednr_handle->small_height * 3 / 2;
    dst.rect.start_x = 0;
    dst.rect.start_y = 0;
    dst.rect.width = threednr_handle->small_width;
    dst.rect.height = threednr_handle->small_height;
    dst.size.width = threednr_handle->small_width;
    dst.size.height = threednr_handle->small_height;
    dst.addr_phy.addr_y = threednr_handle->small_buf_phy[cur_frm];
    dst.addr_phy.addr_u =
        dst.addr_phy.addr_y +
        threednr_handle->small_width * threednr_handle->small_height;
    dst.addr_phy.addr_v = dst.addr_phy.addr_u;
    dst.addr_vir.addr_y = threednr_handle->small_buf_vir[cur_frm];
    dst.addr_vir.addr_u =
        dst.addr_vir.addr_y +
        threednr_handle->small_width * threednr_handle->small_height;
    dst.addr_vir.addr_v = dst.addr_vir.addr_u;
    dst.fd = threednr_handle->small_buf_fd[cur_frm];
    CMR_LOGV("Call the threednr_start_scale().src Y: 0x%lx, 0x%x, dst Y: "
             "0x%lx, 0x%x",
             src->addr_vir.addr_y, src->fd, dst.addr_vir.addr_y, dst.fd);

    if (threednr_handle->is_stop) {
        return NULL;
    }

    ret = threednr_start_scale(oem_handle, src, &dst);
    if (ret) {
        CMR_LOGE("Fail to call threednr_start_scale");
        return NULL;
    }

#if 1
    {
        char flag[PROPERTY_VALUE_MAX] = {'\0'};
        property_get("post_3dnr_save_scl_data", flag, "0");
        if (!strcmp(flag, "1")) { // save input image.
            CMR_LOGI("save pic: %d, threednr_handle->g_num: %d.", cur_frm,
                     threednr_handle->g_num);
            sprintf(filename,
                    "/data/misc/cameraserver/scl_in_%ldx%ld_index_%d.yuv",
                    threednr_handle->width, threednr_handle->height, cur_frm);
            save_yuv(filename, (char *)src->addr_vir.addr_y,
                     threednr_handle->width, threednr_handle->height);
            sprintf(filename,
                    "/data/misc/cameraserver/scl_out_%ldx%ld_index_%d.yuv",
                    threednr_handle->small_width, threednr_handle->small_height,
                    cur_frm);
            save_yuv(filename, (char *)dst.addr_vir.addr_y,
                     threednr_handle->small_width,
                     threednr_handle->small_height);
        }
    }
#endif

    // call 3dnr function
    CMR_LOGD("Call the threednr_function(). before. cnt: %ld, fd: 0x%x",
             threednr_handle->common.save_frame_count, in->src_frame.fd);
    big_buf.bufferY = (unsigned char *)in->src_frame.addr_vir.addr_y;
    big_buf.bufferU =
        big_buf.bufferY + threednr_handle->width * threednr_handle->height;
    big_buf.bufferV = big_buf.bufferU;
    big_buf.fd = in->src_frame.fd;
    small_buf.bufferY =
        (unsigned char *)threednr_handle->small_buf_vir[cur_frm];
    small_buf.bufferU =
        small_buf.bufferY +
        threednr_handle->small_width * threednr_handle->small_height;
    small_buf.bufferV = small_buf.bufferU;
    small_buf.fd = threednr_handle->small_buf_fd[cur_frm];
    CMR_LOGV("Call the threednr_function().big Y: %p, 0x%x, small Y: %p, 0x%x. "
             " ,threednr_handle->is_stop %ld",
             big_buf.bufferY, big_buf.fd, small_buf.bufferY, small_buf.fd,
             threednr_handle->is_stop);

    if (threednr_handle->is_stop) {
        return NULL;
    }

    ret = threednr_function(&small_buf, &big_buf);
    if (ret < 0) {
        CMR_LOGE("Fail to call the threednr_function");
    }

    if (CAP_3DNR_NUM == threednr_handle->common.save_frame_count) {
        cmr_bzero(&out->dst_frame, sizeof(struct img_frm));
        oem_handle = threednr_handle->common.ipm_cxt->init_in.oem_handle;
        threednr_handle->frame_in = *in;
        ret = req_3dnr_do(threednr_handle, addr, size);
    }

    CMR_LOGI("post sem");
    sem_post(&threednr_handle->sem_3dnr);

    return NULL;
}

cmr_int create_3dnr_thread(struct thread_3dnr_info *info) {
    cmr_int rtn = 0;
    pthread_attr_t attr;
    pthread_t handle;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    rtn = pthread_create(&handle, &attr, thread_3dnr, info);
    if (rtn) {
        CMR_LOGE("Fail to create thread for thread_3dnr");
        return rtn;
    }

    pthread_attr_destroy(&attr);

    CMR_LOGD("done %ld", rtn);

    return rtn;
}

static cmr_int threednr_transfer_frame(cmr_handle class_handle,
                                       struct ipm_frame_in *in,
                                       struct ipm_frame_out *out) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    if (!out || !in || !class_handle) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }
    struct class_3dnr *threednr_handle = (struct class_3dnr *)class_handle;
    cmr_u32 frame_in_cnt;
    struct img_addr *addr;
    struct img_size size;
    cmr_handle oem_handle;
    cmr_u32 sensor_id = 0;
    cmr_u32 dnr_enable = 0;
    struct c3dnr_buffer big_buf, small_buf;
    cmr_u32 cur_num = threednr_handle->g_num;

    if (threednr_handle->is_stop) {
        return 0;
    }

    CMR_LOGD("get one frame, num %d", cur_num);
    threednr_handle->g_info_3dnr[cur_num].class_handle = class_handle;
    memcpy(&threednr_handle->g_info_3dnr[cur_num].in, in,
           sizeof(struct ipm_frame_in));
    memcpy(&threednr_handle->g_info_3dnr[cur_num].out, out,
           sizeof(struct ipm_frame_out));
    create_3dnr_thread(&threednr_handle->g_info_3dnr[cur_num]);
    threednr_handle->g_num++;
    threednr_handle->g_num = threednr_handle->g_num % CAP_3DNR_NUM;

    return 0;
}

static cmr_int req_3dnr_do(cmr_handle class_handle, struct img_addr *dst_addr,
                           struct img_size frame_size) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_3dnr *threednr_handle = (struct class_3dnr *)class_handle;

    CMR_MSG_INIT(message);

    if (!dst_addr || !class_handle) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }

    threednr_handle->dst_addr = *dst_addr;
    threednr_handle->width = frame_size.width;
    threednr_handle->height = frame_size.height;

    message.msg_type = CMR_EVT_3DNR_START;
    if (NULL != threednr_handle->reg_cb)
        message.sync_flag = CMR_MSG_SYNC_RECEIVED;
    else
        message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    ret = cmr_thread_msg_send(threednr_handle->threednr_thread, &message);
    if (ret) {
        CMR_LOGE("Failed to send one msg to 3dnr thread.");
    }

    return ret;
}

static cmr_int threednr_save_frame(cmr_handle class_handle,
                                   struct ipm_frame_in *in) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_uint y_size = 0;
    cmr_uint uv_size = 0;
    struct class_3dnr *threednr_handle = (struct class_3dnr *)class_handle;
    cmr_int frame_sn = 0;
    if (!class_handle || !in) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }

    if (threednr_handle->common.save_frame_count > CAP_3DNR_NUM) {
        CMR_LOGE("cap cnt error,%ld", threednr_handle->common.save_frame_count);
        return CMR_CAMERA_FAIL;
    }

    y_size = in->src_frame.size.height * in->src_frame.size.width;
    uv_size = in->src_frame.size.height * in->src_frame.size.width / 2;
    frame_sn = threednr_handle->common.save_frame_count - 1;
    if (frame_sn < 0) {
        CMR_LOGE("frame_sn error,%ld.", frame_sn);
        return CMR_CAMERA_FAIL;
    }

    CMR_LOGV(" 3dnr frame_sn %ld, y_addr 0x%lx", frame_sn,
             in->src_frame.addr_vir.addr_y);
    if (threednr_handle->mem_size >= in->src_frame.buf_size &&
        NULL != (void *)in->src_frame.addr_vir.addr_y) {
        threednr_handle->alloc_addr[frame_sn] =
            (cmr_u8 *)(in->src_frame.addr_vir.addr_y);
    } else {
        CMR_LOGE(" 3dnr:mem size:0x%lx,data y_size:0x%lx. 0x%lx",
                 threednr_handle->mem_size, y_size,
                 in->src_frame.addr_vir.addr_y);
    }
    return ret;
}

static cmr_int threednr_process_thread_proc(struct cmr_msg *message,
                                            void *private_data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_3dnr *class_handle = (struct class_3dnr *)private_data;
    cmr_u32 evt = 0;
    struct ipm_frame_out out;
    struct ipm_frame_in *in;

    if (!message || !class_handle) {
        CMR_LOGE("parameter is fail");
        return CMR_CAMERA_INVALID_PARAM;
    }

    evt = (cmr_u32)message->msg_type;

    switch (evt) {
    case CMR_EVT_3DNR_INIT:
        CMR_LOGI("3dnr thread inited.");
        break;
    case CMR_EVT_3DNR_SAVE_FRAME:
        CMR_LOGI("3dnr save frame");
        in = message->data;
        ret = threednr_save_frame(class_handle, in);
        if (ret != CMR_CAMERA_SUCCESS) {
            CMR_LOGE("3dnr save frame failed.");
        }
        break;
    case CMR_EVT_3DNR_START:
        CMR_LOGD("CMR_EVT_3DNR_START.");
        class_handle->common.receive_frame_count = 0;
        class_handle->common.save_frame_count = 0;
        out.private_data = class_handle->common.ipm_cxt->init_in.oem_handle;
        out.dst_frame = class_handle->frame_in.dst_frame;
        CMR_LOGD("out private_data 0x%lx", (cmr_int)out.private_data);
        CMR_LOGD("CMR_EVT_3DNR_START addr 0x%lx   %ld %ld",
                 class_handle->dst_addr.addr_y, class_handle->width,
                 class_handle->height);

        if (class_handle->reg_cb) {
            (class_handle->reg_cb)(IPM_TYPE_3DNR, &out);
        }

        break;
    case CMR_EVT_3DNR_EXIT:
        CMR_LOGD("3dnr thread exit.");
        break;
    default:
        break;
    }

    return ret;
}

static cmr_int threednr_thread_create(struct class_3dnr *class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    CMR_MSG_INIT(message);

    CHECK_HANDLE_VALID(class_handle);

    if (!class_handle->is_inited) {
        ret = cmr_thread_create(
            &class_handle->threednr_thread, CAMERA_3DNR_MSG_QUEUE_SIZE,
            threednr_process_thread_proc, (void *)class_handle);
        if (ret) {
            CMR_LOGE("send msg failed!");
            ret = CMR_CAMERA_FAIL;
            return ret;
        }
        class_handle->is_inited = 1;
    }

    return ret;
}

static cmr_int threednr_thread_destroy(struct class_3dnr *class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    CMR_MSG_INIT(message);

    CHECK_HANDLE_VALID(class_handle);

    if (class_handle->is_inited) {

        ret = cmr_thread_destroy(class_handle->threednr_thread);
        class_handle->threednr_thread = 0;

        class_handle->is_inited = 0;
    }

    return ret;
}

#endif
