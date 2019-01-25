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

#if defined(CONFIG_CAMERA_RT_REFOCUS)

#define LOG_TAG "cmr_refocus"

#include "cmr_msg.h"
#include "cmr_ipm.h"
#include "cmr_common.h"
#include "SprdOEMCamera.h"
#include "../../arithmetic/inc/alRnBRT.h"
#include "cutils/properties.h"

typedef int (*altek_refocus_getVerison)(void *a_pOutBuf, int a_dInBufMaxSize);
typedef int (*altek_refocus_int)(void *const a_pInData, int a_dInSize,
                                 int a_dInImgW, int a_dInImgH,
                                 void *a_pInOTPBuf, int a_dInOTPSize);
typedef int (*altek_refocus_function)(unsigned char *a_pucOutYCC444,
                                      unsigned char *a_pucInYCCNV21Buf,
                                      unsigned char *a_puwInDisparityBuf,
                                      unsigned short a_uwInDisparityW,
                                      unsigned short a_uwInDisparityH,
                                      unsigned short a_uwInX,
                                      unsigned short a_uwInY);
typedef int (*altek_refocus_finalize)();

struct refoucs_ops {
    altek_refocus_getVerison getVesrion;
    altek_refocus_int init;
    altek_refocus_function function;
    altek_refocus_finalize finalize;
};

struct class_refocus {
    struct ipm_common common;
    cmr_handle thread_handle;
    cmr_uint is_busy;
    cmr_uint is_inited;
    cmr_int ops_init_ret;
    void *alloc_addr;
    cmr_uint mem_size;
    cmr_uint frame_cnt;
    cmr_uint frame_total_num;
    struct ipm_frame_in frame_in;
    struct ipm_frame_out frame_out;
    ipm_callback frame_cb;
};

struct refocus_init_parameter {
    struct img_size refocus_size;
    struct img_otp_data otp_data;
};

struct refocus_start_parameter {
    void *frame_data;
    ipm_callback frame_cb;
    cmr_handle caller_handle;
    void *private_data;
    cmr_uint depth_map_with;
    cmr_uint depth_map_height;
    void *depth_map_data;
    cmr_uint touch_x;
    cmr_uint touch_y;
};

static cmr_int refocus_open(cmr_handle ipm_handle, struct ipm_open_in *in,
                            struct ipm_open_out *out,
                            cmr_handle *out_class_handle);
static cmr_int refocus_close(cmr_handle class_handle);
static cmr_int refocus_transfer_frame(cmr_handle class_handle,
                                      struct ipm_frame_in *in,
                                      struct ipm_frame_out *out);
static cmr_int refocus_pre_proc(cmr_handle class_handle);
static cmr_int refocus_post_proc(cmr_handle class_handle);
static cmr_int refocus_start(cmr_handle class_handle,
                             struct refocus_start_parameter *param);
static cmr_uint check_size_data_invalid(struct img_size *refocus_size);
static cmr_int refocus_call_init(struct class_refocus *class_handle,
                                 const struct refocus_init_parameter *parm);
static cmr_uint refocus_is_busy(struct class_refocus *class_handle);
static void refocus_set_busy(struct class_refocus *class_handle,
                             cmr_uint is_busy);
static cmr_int refocus_thread_create(struct class_refocus *class_handle);
static cmr_int refocus_thread_proc(struct cmr_msg *message, void *private_data);
int32_t refocus_save_to_file(cmr_u32 index, cmr_u32 img_fmt, cmr_u32 width,
                             cmr_u32 height, cmr_u32 touchX, cmr_u32 touchY,
                             void *addr);
int32_t refocus_output_save_to_file(cmr_u32 index, cmr_u32 img_fmt,
                                    cmr_u32 width, cmr_u32 height,
                                    cmr_u32 touchX, cmr_u32 touchY, void *addr);

int32_t dBokehImgW = 0;
int32_t dBokehImgH = 0;
int32_t refocus_frame_num = 0;
int32_t no_callback = 1;

static struct refoucs_ops altek_refoucs_ops = {

#if defined(CONFIG_CAMERA_RT_REFOCUS)
    alRnBRT_VersionInfo_Get, alRnBRT_Init,
    (altek_refocus_function)alRnBRT_ReFocus, alRnBRT_Close
#else
    NULL, NULL, NULL
#endif
};

static struct class_ops refocus_ops_tab_info = {
    refocus_open,     refocus_close,     refocus_transfer_frame,
    refocus_pre_proc, refocus_post_proc,
};

struct class_tab_t refocus_tab_info = {
    &refocus_ops_tab_info,
};

#define CMR_REFOCUS_LIMIT_SIZE (320 * 240)
#define CMR_EVT_REFOCUS_START (1 << 19)
#define CMR_EVT_REFOCUS_EXIT (1 << 20)
#define CMR_EVT_REFOCUS_INIT (1 << 21)
#define CMR__EVT_REFOCUS_MASK_BITS                                             \
    (cmr_u32)(CMR_EVT_REFOCUS_START | CMR_EVT_REFOCUS_EXIT |                   \
              CMR_EVT_REFOCUS_INIT)

#define CAMERA_REFOCUS_MSG_QUEUE_SIZE 5
#define IMAGE_FORMAT "YVU420_SEMIPLANAR"

#define CHECK_HANDLE_VALID(handle)                                             \
    do {                                                                       \
        if (!handle) {                                                         \
            return CMR_CAMERA_INVALID_PARAM;                                   \
        }                                                                      \
    } while (0)

#define REFOCUS_PTR_NUB 200
static void *refocus_ptr[REFOCUS_PTR_NUB] = {NULL};
static cmr_int refocus_open(cmr_handle ipm_handle, struct ipm_open_in *in,
                            struct ipm_open_out *out,
                            cmr_handle *out_class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_refocus *refocus_handle = NULL;
    struct img_size *refocus_size;
    struct refocus_init_parameter param;

    if (!out || !in || !ipm_handle || !out_class_handle) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }

    refocus_handle =
        (struct class_refocus *)malloc(sizeof(struct class_refocus));
    if (!refocus_handle) {
        CMR_LOGE("No mem!");
        return CMR_CAMERA_NO_MEM;
    }

    cmr_bzero(refocus_handle, sizeof(struct class_refocus));

    refocus_handle->common.ipm_cxt = (struct ipm_context_t *)ipm_handle;
    refocus_handle->common.class_type = IPM_TYPE_REFOCUS;
    refocus_handle->common.ops = &refocus_ops_tab_info;
    refocus_handle->frame_cb = in->reg_cb;
    refocus_handle->mem_size =
        in->frame_size.height * in->frame_size.width * 3 / 2;
    refocus_handle->frame_total_num = in->frame_cnt;
    refocus_handle->frame_cnt = 0;

    CMR_LOGD("mem_size = 0x%ld ,in->otp_data->otp_size "
             "0x%x,in->otp_data->otp_ptr %p",
             refocus_handle->mem_size, in->otp_data.otp_size,
             in->otp_data.otp_ptr);

    param.refocus_size = in->frame_size;

    ret = refocus_thread_create(refocus_handle);
    if (ret) {
        CMR_LOGE("failed to create thread.");
        goto free_refocus_handle;
    }

    param.refocus_size = in->frame_size;
    param.otp_data.otp_size = in->otp_data.otp_size;
    param.otp_data.otp_ptr = in->otp_data.otp_ptr;

    ret = refocus_call_init(refocus_handle, &param);
    if (ret) {
        CMR_LOGE("failed to init fd");
        refocus_close(refocus_handle);
    } else {
        *out_class_handle = (cmr_handle)refocus_handle;
    }

    return ret;

free_refocus_handle:
    if (refocus_handle) {
        free(refocus_handle);
        refocus_handle = NULL;
    }
    return ret;
}

static cmr_int refocus_close(cmr_handle class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_refocus *refocus_handle = (struct class_refocus *)class_handle;
    CMR_MSG_INIT(message);

    CHECK_HANDLE_VALID(refocus_handle);

    message.msg_type = CMR_EVT_REFOCUS_EXIT;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    ret = cmr_thread_msg_send(refocus_handle->thread_handle, &message);
    if (ret) {
        CMR_LOGE("send msg fail");
        goto out;
    }

    if (refocus_handle->thread_handle) {
        cmr_thread_destroy(refocus_handle->thread_handle);
        refocus_handle->thread_handle = 0;
        refocus_handle->is_inited = 0;
    }

    if (refocus_handle) {
        free(refocus_handle);
        refocus_handle = NULL;
    }

out:
    return ret;
}

static cmr_int refocus_transfer_frame(cmr_handle class_handle,
                                      struct ipm_frame_in *in,
                                      struct ipm_frame_out *out) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_refocus *refocus_handle = (struct class_refocus *)class_handle;
    cmr_uint frame_cnt;
    cmr_u32 is_busy = 0;
    struct refocus_start_parameter param;

    if (!in || !class_handle) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }

    // frame_cnt   = ++refocus_handle->frame_cnt;
    refocus_frame_num = in->frame_cnt;

    if (frame_cnt < refocus_handle->frame_total_num) {
        CMR_LOGD("This is fd 0x%ld frame. need the 0x%ld frame,", frame_cnt,
                 refocus_handle->frame_total_num);
        return ret;
    }

    is_busy = refocus_is_busy(refocus_handle);
    CMR_LOGV("fd is_busy =%d,frame_cnt %d", is_busy, frame_cnt);

    if (!is_busy) {
        refocus_handle->frame_cnt = 0;
        refocus_handle->frame_in = *in;

        param.frame_data = (void *)in->src_frame.addr_vir.addr_y;
        param.touch_x = in->touch_x;
        param.touch_y = in->touch_y;
        param.depth_map_with = in->depth_map.width;
        param.depth_map_height = in->depth_map.height;
        param.depth_map_data = (void *)in->depth_map.depth_map_ptr;
        param.frame_cb = refocus_handle->frame_cb;
        param.caller_handle = in->caller_handle;
        param.private_data = in->private_data;
        CMR_LOGV("in %param.depth_map_data %p. touch %d %d", in,
                 param.depth_map_data, in->touch_x, in->touch_y);

        // memcpy(refocus_handle->alloc_addr, (void
        // *)in->src_frame.addr_vir.addr_y, refocus_handle->mem_size);

        ret = refocus_start(class_handle, &param);
        if (ret) {
            CMR_LOGE("send msg fail");
            goto out;
        }

        if (refocus_handle->frame_cb) {
            if (out != NULL) {
                cmr_bzero(out, sizeof(struct ipm_frame_out));
            }
        } else {
            if (out != NULL) {
                out = &refocus_handle->frame_out;
            } else {
                CMR_LOGE("sync err,out parm can't NULL.");
            }
        }
    }
out:
    return ret;
}

static cmr_int refocus_pre_proc(cmr_handle class_handle) {
    UNUSED(class_handle);

    cmr_int ret = CMR_CAMERA_SUCCESS;

    /*no need to do*/

    return ret;
}
static cmr_int refocus_post_proc(cmr_handle class_handle) {
    UNUSED(class_handle);

    cmr_int ret = CMR_CAMERA_SUCCESS;

    /*no need to do*/

    return ret;
}

static cmr_int refocus_start(cmr_handle class_handle,
                             struct refocus_start_parameter *param) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_refocus *refocus_handle = (struct class_refocus *)class_handle;
    cmr_u32 is_busy = 0;
    CMR_MSG_INIT(message);

    if (!class_handle || !param) {
        CMR_LOGE("parameter is NULL. fail");
        return CMR_CAMERA_INVALID_PARAM;
    }

    message.data = (void *)malloc(sizeof(struct refocus_start_parameter));
    if (NULL == message.data) {
        CMR_LOGE("NO mem, Fail to alloc memory for msg data");
        return CMR_CAMERA_NO_MEM;
    }

    memcpy(message.data, param, sizeof(struct refocus_start_parameter));

    message.msg_type = CMR_EVT_REFOCUS_START;
    message.alloc_flag = 1;

    if (refocus_handle->frame_cb) {
        // message.sync_flag = CMR_MSG_SYNC_RECEIVED;
        message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    } else {
        message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    }

    ret = cmr_thread_msg_send(refocus_handle->thread_handle, &message);
    if (ret) {
        CMR_LOGE("send msg fail");
        ret = CMR_CAMERA_FAIL;
    }

    return ret;
}

static cmr_uint check_size_data_invalid(struct img_size *refocus_size) {
    cmr_int ret = -CMR_CAMERA_FAIL;

    if (NULL != refocus_size) {
        if ((refocus_size->width) && (refocus_size->height)) {
            ret = CMR_CAMERA_SUCCESS;
        }
    }

    return ret;
}

static cmr_int refocus_call_init(struct class_refocus *class_handle,
                                 const struct refocus_init_parameter *parm) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    CMR_MSG_INIT(message);

    message.data = malloc(sizeof(struct refocus_init_parameter));
    if (NULL == message.data) {
        CMR_LOGE("NO mem, Fail to alloc memory for msg data");
        ret = CMR_CAMERA_NO_MEM;
        goto out;
    }

    message.alloc_flag = 1;
    memcpy(message.data, parm, sizeof(struct refocus_init_parameter));

    message.msg_type = CMR_EVT_REFOCUS_INIT;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    ret = cmr_thread_msg_send(class_handle->thread_handle, &message);
    if (CMR_CAMERA_SUCCESS != ret) {
        CMR_LOGE("msg send fail");
        ret = CMR_CAMERA_FAIL;
        goto free_all;
    }

    return ret | class_handle->ops_init_ret;

free_all:
    free(message.data);
out:
    return ret;
}

static cmr_uint refocus_is_busy(struct class_refocus *class_handle) {
    cmr_int is_busy = 0;

    if (NULL == class_handle) {
        return is_busy;
    }

    is_busy = class_handle->is_busy;

    return is_busy;
}

static void refocus_set_busy(struct class_refocus *class_handle,
                             cmr_uint is_busy) {
    if (NULL == class_handle) {
        return;
    }

    class_handle->is_busy = is_busy;
}

static cmr_int refocus_thread_create(struct class_refocus *class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    CMR_MSG_INIT(message);

    CHECK_HANDLE_VALID(class_handle);

    if (!class_handle->is_inited) {
        ret = cmr_thread_create(&class_handle->thread_handle,
                                CAMERA_REFOCUS_MSG_QUEUE_SIZE,
                                refocus_thread_proc, (void *)class_handle);
        if (ret) {
            CMR_LOGE("send msg failed!");
            ret = CMR_CAMERA_FAIL;
            goto end;
        }
        ret = cmr_thread_set_name(class_handle->thread_handle, "refocus");
        if (CMR_MSG_SUCCESS != ret) {
            CMR_LOGE("fail to set thr name");
            ret = CMR_MSG_SUCCESS;
        }

        class_handle->is_inited = 1;
    } else {
        CMR_LOGI("fd is inited already");
    }

end:
    return ret;
}

static cmr_int refocus_thread_proc(struct cmr_msg *message,
                                   void *private_data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_refocus *class_handle = (struct class_refocus *)private_data;
    cmr_int evt = 0;
    cmr_uint mem_size = 0;
    void *addr = 0;
    cmr_int altek_refocus_ret = 0;
    cmr_int otp_size = 8192;
    void *otp_ptr = NULL;
    void *refocus_out_buff_ptr = NULL;
    cmr_int face_num = 0;
    cmr_int k, min_refocus;
    char *psPath_otpData = "data/misc/cameraserver/otp.bin";
    char *psPath_depthData = "data/misc/cameraserver/depth.bin";
    struct refocus_init_parameter *init_param;
    struct refocus_start_parameter *start_param;
    cmr_s8 value[PROPERTY_VALUE_MAX];
    cmr_s8 value1[PROPERTY_VALUE_MAX];

    if (!message || !class_handle) {
        CMR_LOGE("parameter is fail");
        return CMR_CAMERA_INVALID_PARAM;
    }

    evt = (cmr_u32)(message->msg_type & CMR__EVT_REFOCUS_MASK_BITS);

    switch (evt) {
    case CMR_EVT_REFOCUS_INIT: {
        struct refocus_init_parameter *init_param =
            (struct refocus_init_parameter *)message->data;
        if (!check_size_data_invalid(&init_param->refocus_size)) {
            if (altek_refoucs_ops.getVesrion) {
                char acVersion[256];
                altek_refoucs_ops.getVesrion(acVersion, 256);
                CMR_LOGI("[REFOCUS] alRnBRT_VersionInfo_Get %s", acVersion);
            }
            if (altek_refoucs_ops.init &&
                (init_param->refocus_size.width *
                     init_param->refocus_size.height >=
                 CMR_REFOCUS_LIMIT_SIZE)) {

                if (init_param->otp_data.otp_ptr != NULL) {
                    save_file(psPath_otpData, init_param->otp_data.otp_ptr,
                              init_param->otp_data.otp_size);
                    CMR_LOGI("[REFOCUS] init_param.otp_ptr %p, "
                             "init_param.otp_size %d",
                             init_param->otp_data.otp_ptr,
                             init_param->otp_data.otp_size);
                    CMR_LOGI("[REFOCUS] w/h %d %d, "
                             "init_param.otp_data->otp_ptr %p, "
                             "init_param.otp_data->otp_size 0x%x",
                             init_param->refocus_size.width,
                             init_param->refocus_size.height,
                             init_param->otp_data.otp_ptr,
                             init_param->otp_data.otp_size);
                    dBokehImgW = init_param->refocus_size.width;
                    dBokehImgH = init_param->refocus_size.height;
                    ret = altek_refoucs_ops.init(
                        NULL, 0, init_param->refocus_size.width,
                        init_param->refocus_size.height,
                        init_param->otp_data.otp_ptr,
                        init_param->otp_data.otp_size);
                    if (alRnBRT_ERR_SUCCESS != ret) {
                        class_handle->ops_init_ret = ret;
                        CMR_LOGE("[REFOCUS] init fail. ret 0x%x", ret);
                    } else {
                        CMR_LOGI("[REFOCUS] init done.");
                    }
                } else {
#if 0
									   char *psPath_OTP= "data/misc/cameraserver/packet.otp";
									   init_param->otp_data.otp_size = 0x2000;
									   char *otp_data = malloc(init_param->otp_data.otp_size);
									   read_file(psPath_otpData,  otp_data, init_param->otp_data.otp_size);
									   CMR_LOGI("[REFOCUS] read_file %s",psPath_otpData);
									   CMR_LOGI("[REFOCUS] w/h %d %d, init_param.otp_data->otp_ptr %p, init_param.otp_data->otp_size 0x%x", init_param->refocus_size.width, init_param->refocus_size.height,init_param->otp_data.otp_ptr,init_param->otp_data.otp_size);
									   dBokehImgW = init_param->refocus_size.width;
									   dBokehImgH = init_param->refocus_size.height;
									   ret = altek_refoucs_ops.init(NULL,0,init_param->refocus_size.width, init_param->refocus_size.height, otp_data, init_param->otp_data.otp_size);
									   if(alRnBRT_ERR_SUCCESS !=ret){
										   class_handle->ops_init_ret = ret;
										   CMR_LOGE("[REFOCUS] init fail. ret 0x%x",ret);
									   } else {
										   CMR_LOGI("[REFOCUS] init done.");
									   }
									   if(otp_data){
										   free(otp_data);
										   otp_data = NULL;
									   }
#endif
                }

            } else {
                class_handle->ops_init_ret = -CMR_CAMERA_FAIL;
            }
        }
    } break;

    case CMR_EVT_REFOCUS_START:
        CMR_PRINT_TIME;
        start_param = (struct refocus_start_parameter *)message->data;

        if (NULL == start_param) {
            CMR_LOGE("parameter fail");
            break;
        }

        refocus_set_busy(class_handle, 1);

        /*start call lib function*/
        CMR_LOGV("refocus detect start");

        if (altek_refoucs_ops.function && start_param->depth_map_data != NULL) {

            unsigned char *pucDepth = (unsigned char *)malloc(
                start_param->depth_map_with * start_param->depth_map_height);

            if (start_param->depth_map_data == NULL) {
#if 0
							   read_file(psPath_depthData,pucDepth,start_param->depth_map_with*start_param->depth_map_height);
							   CMR_LOGI("[REFOCUS] read_file %s",psPath_depthData);
#endif
                no_callback = 1;
            } else {
                memcpy(pucDepth, start_param->depth_map_data,
                       start_param->depth_map_with *
                           start_param->depth_map_height);
                no_callback = 0;
            }

            unsigned char *pucInputBufferNV21 =
                (unsigned char *)malloc(dBokehImgW * dBokehImgH * 3 / 2);
            memcpy(pucInputBufferNV21, start_param->frame_data,
                   dBokehImgW * dBokehImgH * 3 / 2);

            unsigned char *pucOutYCCNV21 =
                (unsigned char *)malloc(dBokehImgW * dBokehImgH * 3 / 2);

            if (start_param->touch_x == 0 || start_param->touch_y == 0) {
                start_param->touch_x = dBokehImgW / 2;
                start_param->touch_y = dBokehImgH / 2;
            }
            CMR_LOGD("pucInputBufferNV21 %p,pucDepth %p "
                     "start_param->depth_map_data %p,%d, %d, %d, %d,dBokehImgW "
                     "%d dBokehImgH %d",
                     pucInputBufferNV21, pucDepth, start_param->depth_map_data,
                     start_param->depth_map_with, start_param->depth_map_height,
                     start_param->touch_x, start_param->touch_y, dBokehImgW,
                     dBokehImgH);
            altek_refocus_ret = altek_refoucs_ops.function(
                pucOutYCCNV21, pucInputBufferNV21, pucDepth,
                start_param->depth_map_with, start_param->depth_map_height,
                start_param->touch_x, start_param->touch_y);
            if (altek_refocus_ret == 0) {
                memcpy(start_param->frame_data, pucOutYCCNV21,
                       dBokehImgW * dBokehImgH * 3 / 2);
                CMR_LOGV("&(class_handle->frame_out.dst_frame.addr_vir) %p "
                         "class_handle->frame_out.dst_frame.addr_vir 0x%x, "
                         "start_param->frame_data %p",
                         &(class_handle->frame_out.dst_frame.addr_vir),
                         class_handle->frame_out.dst_frame.addr_vir.addr_y,
                         start_param->frame_data);
            }

            /*debug info*/
            property_get("debug.camera.save.refocus", value, "0");
            property_get("persist.camera.save.refocus", value1, "0");
            if (atoi(value) == 1 && altek_refocus_ret == 0) {
                if (refocus_frame_num % 100 == 0 && refocus_frame_num != 0) {
                    refocus_save_to_file(
                        refocus_frame_num, IMG_DATA_TYPE_RAW,
                        start_param->depth_map_with,
                        start_param->depth_map_height, start_param->touch_x,
                        start_param->touch_y, start_param->depth_map_data);
                    refocus_save_to_file(
                        refocus_frame_num, IMG_DATA_TYPE_YUV420, dBokehImgW,
                        dBokehImgH, start_param->touch_x, start_param->touch_y,
                        pucInputBufferNV21);
                    refocus_output_save_to_file(
                        refocus_frame_num, IMG_DATA_TYPE_YUV420, dBokehImgW,
                        dBokehImgH, start_param->touch_x, start_param->touch_y,
                        pucOutYCCNV21);
                }
            } else if (atoi(value) == 1 && altek_refocus_ret != 0) {
                CMR_LOGE("refocus function fail,0x%x", altek_refocus_ret);
                refocus_save_to_file(refocus_frame_num, IMG_DATA_TYPE_RAW,
                                     start_param->depth_map_with,
                                     start_param->depth_map_height,
                                     start_param->touch_x, start_param->touch_y,
                                     start_param->depth_map_data);
            } else if (atoi(value) == 2 || atoi(value1) == 2) {
                memcpy(start_param->frame_data, start_param->depth_map_data,
                       start_param->depth_map_with *
                           start_param->depth_map_height);
                memset(start_param->frame_data +
                           start_param->depth_map_with *
                               start_param->depth_map_height,
                       128, start_param->depth_map_with *
                                start_param->depth_map_height * 1 / 2);
            }

            if (pucDepth != NULL) {
                free(pucDepth);
                pucDepth = NULL;
            }
            if (pucInputBufferNV21 != NULL) {
                free(pucInputBufferNV21);
                pucInputBufferNV21 = NULL;
            }
            if (pucOutYCCNV21 != NULL) {
                free(pucOutYCCNV21);
                pucOutYCCNV21 = NULL;
            }

            /*callback*/
            if (class_handle->frame_cb && no_callback == 0) {
                class_handle->frame_out.private_data =
                    start_param->private_data;
                class_handle->frame_out.caller_handle =
                    start_param->caller_handle;
                class_handle->frame_cb(IPM_TYPE_REFOCUS,
                                       &class_handle->frame_out);
            }
        }
        refocus_set_busy(class_handle, 0);
        CMR_PRINT_TIME;
        break;

    case CMR_EVT_REFOCUS_EXIT:
        if (altek_refoucs_ops.finalize && !class_handle->ops_init_ret) {
            if (0 != altek_refoucs_ops.finalize()) {
                CMR_LOGE("[REFOCUS] deinit fail");
            } else {
                CMR_LOGI("[REFOCUS] deinit done");
            }
        }
        class_handle->ops_init_ret = 0;
        break;

    default:
        break;
    }

    return ret;
}
int32_t refocus_save_to_file(cmr_u32 index, cmr_u32 img_fmt, cmr_u32 width,
                             cmr_u32 height, cmr_u32 touchX, cmr_u32 touchY,
                             void *addr) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    char file_name[100];
    char tmp_str[10];
    FILE *fp = NULL;
    uint32_t write_bytes = 0;

    CMR_LOGI("index %d format %d width %d heght %d", index, img_fmt, width,
             height);

    cmr_bzero(file_name, 100);
    strcpy(file_name, CAMERA_DUMP_PATH);
    sprintf(tmp_str, "%d", width);
    strcat(file_name, tmp_str);
    strcat(file_name, "X");
    sprintf(tmp_str, "%d", height);
    strcat(file_name, tmp_str);

    strcat(file_name, "_");
    sprintf(tmp_str, "%d", touchX);
    strcat(file_name, tmp_str);
    strcat(file_name, "X");
    sprintf(tmp_str, "%d", touchY);
    strcat(file_name, tmp_str);
    CMR_LOGI("file name %s", file_name);

    if (IMG_DATA_TYPE_YUV420 == img_fmt) {
        strcat(file_name, "_");
        sprintf(tmp_str, "%d", index);
        strcat(file_name, tmp_str);
        strcat(file_name, ".yuv");
        CMR_LOGI("file name %s", file_name);
        fp = fopen(file_name, "wb");
        if (NULL == fp) {
            CMR_LOGI("can not open file: %s \n", file_name);
            return 0;
        }
        write_bytes = fwrite(addr, 1, width * height * 3 / 2, fp);
    } else if (IMG_DATA_TYPE_RAW == img_fmt) {
        strcat(file_name, "_");
        sprintf(tmp_str, "%d", index);
        strcat(file_name, tmp_str);
        strcat(file_name, ".mipi_raw");
        CMR_LOGI("file name %s", file_name);
        fp = fopen(file_name, "wb");
        if (NULL == fp) {
            CMR_LOGI("can not open file: %s \n", file_name);
            return 0;
        }
        write_bytes = fwrite(addr, 1, width * height, fp);
    }

    fclose(fp);

    return write_bytes;
}
int32_t refocus_output_save_to_file(cmr_u32 index, cmr_u32 img_fmt,
                                    cmr_u32 width, cmr_u32 height,
                                    cmr_u32 touchX, cmr_u32 touchY,
                                    void *addr) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    char file_name[100];
    char tmp_str[10];
    FILE *fp = NULL;
    uint32_t write_bytes = 0;

    CMR_LOGI("index %d format %d width %d heght %d", index, img_fmt, width,
             height);

    cmr_bzero(file_name, 100);
    strcpy(file_name, CAMERA_DUMP_PATH);
    sprintf(tmp_str, "%d", width);
    strcat(file_name, tmp_str);
    strcat(file_name, "X");
    sprintf(tmp_str, "%d", height);
    strcat(file_name, tmp_str);

    strcat(file_name, "_");
    sprintf(tmp_str, "%d", touchX);
    strcat(file_name, tmp_str);
    strcat(file_name, "X");
    sprintf(tmp_str, "%d", touchY);
    strcat(file_name, tmp_str);
    CMR_LOGI("file name %s", file_name);

    if (IMG_DATA_TYPE_YUV420 == img_fmt) {
        strcat(file_name, "_");
        sprintf(tmp_str, "%d", index);
        strcat(file_name, tmp_str);
        strcat(file_name, "_out");
        strcat(file_name, ".yuv");
        CMR_LOGI("file name %s", file_name);
        fp = fopen(file_name, "wb");
        if (NULL == fp) {
            CMR_LOGI("can not open file: %s \n", file_name);
            return 0;
        }
        write_bytes = fwrite(addr, 1, width * height * 3 / 2, fp);
    } else if (IMG_DATA_TYPE_RAW == img_fmt) {
        strcat(file_name, "_");
        sprintf(tmp_str, "%d", index);
        strcat(file_name, tmp_str);
        strcat(file_name, ".mipi_raw");
        CMR_LOGI("file name %s", file_name);
        fp = fopen(file_name, "wb");
        if (NULL == fp) {
            CMR_LOGI("can not open file: %s \n", file_name);
            return 0;
        }
        write_bytes = fwrite(addr, 1, width * height, fp);
    }

    fclose(fp);

    return write_bytes;
}

#endif
