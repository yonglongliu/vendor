
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
#define ATRACE_TAG (ATRACE_TAG_CAMERA | ATRACE_TAG_HAL)
#define LOG_TAG "cmr_3dnr_sw"
#include <dlfcn.h>
#include "cmr_msg.h"
#include "cmr_ipm.h"
#include "cmr_common.h"
#include "cmr_sensor.h"
#include "cmr_oem.h"
#include "3dnr_interface.h"
#include <cutils/properties.h>
#include "isp_mw.h"

typedef struct c3dn_io_info {
    c3dnr_buffer_t image[3];
    cmr_u32 width;
    cmr_u32 height;
    cmr_s8 mv_x;
    cmr_s8 mv_y;

    cmr_u8 blending_no;
} c3dnr_io_info_t;

struct thread_3dnr_info {
    cmr_handle class_handle;
    struct ipm_frame_in in;
    struct ipm_frame_out out;
};
struct small_buf_info {
    cmr_uint small_buf_phy[PRE_SW_3DNR_RESERVE_NUM];
    cmr_uint small_buf_vir[PRE_SW_3DNR_RESERVE_NUM];
    cmr_s32 small_buf_fd[PRE_SW_3DNR_RESERVE_NUM];
    cmr_uint used[PRE_SW_3DNR_RESERVE_NUM];
};
typedef struct preview_smallbuf_node {
    uint8_t *buf_vir;
    uint8_t *buf_phy;
    cmr_int buf_fd;
    struct preview_smallbuf_node *next;
} preview_smallbuf_node_t;
typedef struct preview_smallbuf_queue {
    struct preview_smallbuf_node *head;
    struct preview_smallbuf_node *tail;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} preview_smallbuf_queue_t;
struct class_3dnr_pre { // 3dnr pre
    struct ipm_common common;
    cmr_uint mem_size;
    cmr_uint width;
    cmr_uint height;
    cmr_uint small_width;
    cmr_uint small_height;
    cmr_uint is_inited;
    cmr_handle threednr_prevthread;
    struct img_addr dst_addr;
    ipm_callback reg_cb;
    struct ipm_frame_in frame_in;
    // ***** for output data **************
    cmr_uint pre_buf_phy[PRE_SW_3DNR_RESERVE_NUM];
    cmr_uint pre_buf_vir[PRE_SW_3DNR_RESERVE_NUM];
    cmr_s32 pre_buf_fd[PRE_SW_3DNR_RESERVE_NUM];
// ************************************
// ***** for preview small image ******
#if 0
    cmr_uint small_buf_phy[PRE_SW_3DNR_RESERVE_NUM];
    cmr_uint small_buf_vir[PRE_SW_3DNR_RESERVE_NUM];
    cmr_s32 small_buf_fd[PRE_SW_3DNR_RESERVE_NUM];
#else
    struct preview_smallbuf_queue small_buf_queue;
    struct small_buf_info small_buf_info;
#endif
    // ************************************
    cmr_u32 g_num;
    cmr_uint is_stop;
    c3dnr_buffer_t *out_image;
    sem_t sem_3dnr;
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
    union c3dnr_buffer out_buf;
    cmr_uint out_buf_phy;
    cmr_uint out_buf_vir;
    cmr_uint small_buf_phy[CAP_3DNR_NUM];
    cmr_uint small_buf_vir[CAP_3DNR_NUM];
    cmr_s32 out_buf_fd;
    cmr_s32 small_buf_fd[CAP_3DNR_NUM];
    sem_t sem_3dnr;
    cmr_u32 g_num;
    cmr_u32 g_totalnum;
    cmr_uint is_stop;
};
typedef struct process_pre_3dnr_info {
    struct ipm_frame_in in;
    struct ipm_frame_out out;
    struct preview_smallbuf_node smallbuf_node;
    struct prev_threednr_info threednr_info;
} process_pre_3dnr_info_t;
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

#define CMR_EVT_3DNR_PREV_INIT (CMR_EVT_3DNR_BASE + 4)
#define CMR_EVT_3DNR_PREV_START (CMR_EVT_3DNR_BASE + 5)
#define CMR_EVT_3DNR_PREV_EXIT (CMR_EVT_3DNR_BASE + 6)

// static uint8_t *pbig;
// static uint8_t *psmall;
// static uint8_t *pvideo;

static int slope_tmp[6] = {4};
static int sigma_tmp[6] = {4};
static int threthold[4][6] = {{2, 6, 9, 9, 9, 9},
                              {3, 6, 9, 9, 9, 9},
                              {3, 6, 9, 9, 9, 9},
                              {4, 6, 9, 9, 9, 9}};
static int slope[4][6] = {
    {5, 6, 9, 9, 9, 9},
    {5, 6, 9, 9, 9, 9},
    {5, 6, 9, 9, 9, 9},
    {4, 6, 9, 9, 9, 9},
};

uint16_t g_SearchWindow_x = 0;
uint16_t g_SearchWindow_y = 0;

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
static cmr_int threednr_process_prevthread_proc(struct cmr_msg *message,
                                                void *private_data);
static cmr_int threednr_prevthread_create(struct class_3dnr_pre *class_handle);
static cmr_int threednr_prevthread_destroy(struct class_3dnr_pre *class_handle);
static cmr_int threednr_thread_create(struct class_3dnr *class_handle);
static cmr_int threednr_thread_destroy(struct class_3dnr *class_handle);
static cmr_int threednr_save_frame(cmr_handle class_handle,
                                   struct ipm_frame_in *in);
cmr_int threednr_start_scale(cmr_handle oem_handle, struct img_frm *src,
                             struct img_frm *dst);
static cmr_int req_3dnr_save_frame(cmr_handle class_handle,
                                   struct ipm_frame_in *in);
static cmr_int save_yuv(char *filename, char *buffer, uint32_t width,
                        uint32_t height);
static cmr_int get_free_smallbuffer(void *class_handle, cmr_u32 *cur_frm_index);
static cmr_int
req_3dnr_preview_frame(cmr_handle class_handle, struct ipm_frame_in *in,
                       struct ipm_frame_out *out,
                       struct prev_threednr_info *threednr_info,
                       struct preview_smallbuf_node *small_buf_node);
static cmr_int
init_queue_preview_smallbuffer(struct preview_smallbuf_queue *psmall_buf_queue);
static cmr_int
dequeue_preview_smallbuffer(struct preview_smallbuf_queue *psmall_buf_queue,
                            struct preview_smallbuf_node *pnode);
static cmr_int
queue_preview_smallbufer(struct preview_smallbuf_queue *psmall_buf_queue,
                         struct preview_smallbuf_node *pnode);
static cmr_int deinit_queue_preview_smallbufer(
    struct preview_smallbuf_queue *psmall_buf_queue);

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
    return ret;
}

static void read_param_from_file(char* parafile) {
    //char parafile[] = "/data/misc/cameraserver/bst_tdns_settings_image.txt";
    FILE *pFile = fopen(parafile, "rt");
    char line[256];

    if (pFile == NULL) {
        CMR_LOGE("open setting file  %s failed.\n", parafile);
    } else {
        fgets(line, 256, pFile);
        char ss[256];

        while (!feof(pFile)) {
            sscanf(line, "%s", ss);

            if (!strcmp(ss, "-th0")) {
                sscanf(line, "%s %d %d %d %d %d %d", ss, &threthold[0][0],
                       &threthold[0][1], &threthold[0][2],
                       &threthold[0][3], &threthold[0][4],
                       &threthold[0][5]);
            } else if (!strcmp(ss, "-th1")) {
                sscanf(line, "%s %d %d %d %d %d %d", ss, &threthold[1][0],
                       &threthold[1][1], &threthold[1][2],
                       &threthold[1][3], &threthold[1][4],
                       &threthold[1][5]);
            } else if (!strcmp(ss, "-th2")) {
                sscanf(line, "%s %d %d %d %d %d %d", ss, &threthold[2][0],
                       &threthold[2][1], &threthold[2][2],
                       &threthold[2][3], &threthold[2][4],
                       &threthold[2][5]);
            } else if (!strcmp(ss, "-th3")) {
                sscanf(line, "%s %d %d %d %d %d %d", ss, &threthold[3][0],
                       &threthold[3][1], &threthold[3][2],
                       &threthold[3][3], &threthold[3][4],
                       &threthold[3][5]);
            } else if (!strcmp(ss, "-sl0")) {
                sscanf(line, "%s %d %d %d %d %d %d", ss, &slope[0][0],
                       &slope[0][1], &slope[0][2], &slope[0][3],
                       &slope[0][4], &slope[0][5]);
            } else if (!strcmp(ss, "-sl1")) {
                sscanf(line, "%s %d %d %d %d %d %d", ss, &slope[1][0],
                       &slope[1][1], &slope[1][2], &slope[1][3],
                       &slope[1][4], &slope[1][5]);
            } else if (!strcmp(ss, "-sl2")) {
                sscanf(line, "%s %d %d %d %d %d %d", ss, &slope[2][0],
                       &slope[2][1], &slope[2][2], &slope[2][3],
                       &slope[2][4], &slope[2][5]);
            } else if (!strcmp(ss, "-sl3")) {
                sscanf(line, "%s %d %d %d %d %d %d", ss, &slope[3][0],
                       &slope[3][1], &slope[3][2], &slope[3][3],
                       &slope[3][4], &slope[3][5]);
            } else if (!strcmp(ss, "-srx"))
                sscanf(line, "%s %d", ss, &g_SearchWindow_x);
            else if (!strcmp(ss, "-sry"))
                sscanf(line, "%s %d", ss, &g_SearchWindow_y);

            fgets(line, 256, pFile);
        }
    }
    if (pFile != NULL)
        fclose(pFile);
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
    char flag[PROPERTY_VALUE_MAX];

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

    property_get("3dnr_setting_from_file", flag, "0");
    if (!strcmp(flag, "1")) {
        char cap_parafile[] = "/data/misc/cameraserver/bst_tdns_settings_image_cap.txt";
        read_param_from_file(cap_parafile);
    }

    out->total_frame_number = CAP_3DNR_NUM;

    cmr_bzero(threednr_handle, sizeof(struct class_3dnr));
    sem_init(&threednr_handle->sem_3dnr, 0, 1);
    size = (cmr_uint)(in->frame_size.width * in->frame_size.height * 3 / 2);
    threednr_handle->common.ipm_cxt = (struct ipm_context_t *)ipm_handle;
    threednr_handle->common.class_type = IPM_TYPE_3DNR;
    threednr_handle->common.ops = &threednr_ops_tab_info;
    threednr_handle->common.receive_frame_count = 0;
    threednr_handle->common.save_frame_count = 0;
    threednr_handle->common.ops = &threednr_ops_tab_info;

    threednr_handle->mem_size = size;

    threednr_handle->height = in->frame_size.height;
    threednr_handle->width = in->frame_size.width;
    if ((threednr_handle->width * 10) / threednr_handle->height <= 13) {
        threednr_handle->small_height = CMR_3DNR_4_3_SMALL_HEIGHT;
        threednr_handle->small_width = CMR_3DNR_4_3_SMALL_WIDTH;
    } else if ((threednr_handle->width * 10) / threednr_handle->height <= 18) {
        threednr_handle->small_height = CMR_3DNR_16_9_SMALL_HEIGHT;
        threednr_handle->small_width = CMR_3DNR_16_9_SMALL_WIDTH;
    } else {
        CMR_LOGE("incorrect 3dnr small image mapping, using 16*9 as the "
                 "default setting");
        threednr_handle->small_height = CMR_3DNR_16_9_SMALL_HEIGHT;
        threednr_handle->small_width = CMR_3DNR_16_9_SMALL_WIDTH;
    }
    threednr_handle->reg_cb = in->reg_cb;
    threednr_handle->g_num = 0;
    threednr_handle->is_stop = 0;
    threednr_handle->g_totalnum = 0;
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
    threednr_handle->out_buf.cpu_buffer.bufferY =
        (unsigned char *)threednr_handle->out_buf_vir;
    threednr_handle->out_buf.cpu_buffer.bufferU =
        threednr_handle->out_buf.cpu_buffer.bufferY +
        threednr_handle->width * threednr_handle->height;
    threednr_handle->out_buf.cpu_buffer.bufferV =
        threednr_handle->out_buf.cpu_buffer.bufferU;
    param.orig_width = threednr_handle->width;
    param.orig_height = threednr_handle->height;
    param.small_width = threednr_handle->small_width;
    param.small_height = threednr_handle->small_height;
    param.total_frame_num = CAP_3DNR_NUM;
    param.gain = in->adgain;

    if (!strcmp(flag, "1")) {
        param.SearchWindow_x = g_SearchWindow_x;
        param.SearchWindow_y = g_SearchWindow_y;
    } else {
        param.SearchWindow_x = 21;
        param.SearchWindow_y = 21;
    }
    CMR_LOGD("set SearWindow : %d, %d", param.SearchWindow_x, param.SearchWindow_y);

    param.low_thr = 100;
    param.ratio = 2;
    param.sigma_tmp = sigma_tmp;
    param.slope_tmp = slope_tmp;
    param.threthold = threthold;
    param.slope = slope;
    param.yuv_mode = 0;       // NV12?
    param.thread_num_acc = 4; // 2 | (1 << 4) | (2 << 6) |(1<<12);
    param.thread_num = 4;     // 2 | (1<<4) | (2<<6) | (1<<12);
    param.recur_str = -1;
    param.control_en = 0x0;
    param.preview_cpyBuf = 1;
    param.poutimg = NULL;
    ret = threednr_init(&param);
    if (ret < 0) {
        CMR_LOGE("Fail to call threednr_init");
    } else {
        CMR_LOGD("ok to call threednr_init");
    }
    *class_handle = threednr_handle;
    CMR_LOGD("X");
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

    threednr_handle->is_stop = 1;
    // threednr_cancel();
    CMR_LOGI("OK to threednr_cancel");

    ret = threednr_deinit();
    if (ret) {
        CMR_LOGE("3dnr failed to threednr_deinit");
    }
    sem_wait(&threednr_handle->sem_3dnr);
    oem_handle = threednr_handle->common.ipm_cxt->init_in.oem_handle;
    cam_cxt = (struct camera_context *)oem_handle;
    sem_post(&threednr_handle->sem_3dnr);

    ret = threednr_thread_destroy(threednr_handle);
    if (ret) {
        CMR_LOGE("3dnr failed to destroy 3dnr thread");
    }

    sem_destroy(&threednr_handle->sem_3dnr);
    if (NULL != threednr_handle)
        free(threednr_handle);

    sem_post(&cam_cxt->threednr_proc_sm);

    CMR_LOGI("X");
    return ret;
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
    union c3dnr_buffer big_buf, small_buf;
    struct img_frm *src, dst;
    cmr_u32 cur_frm;
    char filename[128];

    if (threednr_handle->is_stop) {
        CMR_LOGE("threednr_handle is stop");
        goto exit;
    }
    sem_wait(&threednr_handle->sem_3dnr);

    cur_frm = threednr_handle->common.save_frame_count;
    CMR_LOGD("wait sem. cur_frm: %d", cur_frm);
    CMR_LOGD("ipm_frame_in.private_data 0x%lx", (cmr_int)in->private_data);
    if (NULL == in->private_data) {
        CMR_LOGE("private_data is ptr of camera_context, now is null");
        goto exit;
    }

    addr = &in->dst_frame.addr_vir;
    size = in->src_frame.size;
#if 1
    ret = req_3dnr_save_frame(threednr_handle, in);
    if (ret != CMR_CAMERA_SUCCESS) {
        CMR_LOGE("req_dnr_save_frame fail");
        goto exit;
    }
#endif
    oem_handle = threednr_handle->common.ipm_cxt->init_in.oem_handle;
    if (threednr_handle->is_stop) {
        CMR_LOGE("threednr_handle is stop");
        goto exit;
    }

    {
        char flag[PROPERTY_VALUE_MAX];
        property_get("3dnr_save_capture_frame", flag, "0");
        if (!strcmp(flag, "1")) { // save input image.
            CMR_LOGI("save pic: %d, threednr_handle->g_num: %d.", cur_frm,
                     threednr_handle->g_num);
            sprintf(filename,
                    "/data/misc/cameraserver/big_in_%dx%d_index_%d.yuv",
                    threednr_handle->width, threednr_handle->height, cur_frm);
            save_yuv(filename, (char *)in->src_frame.addr_vir.addr_y,
                     threednr_handle->width, threednr_handle->height);
            sprintf(filename,
                    "/data/misc/cameraserver/small_in_%dx%d_index_%d.yuv",
                    threednr_handle->small_width, threednr_handle->small_height,
                    cur_frm);
            save_yuv(filename, (char *)in->src_frame.addr_vir.addr_y +
                                   threednr_handle->width *
                                       threednr_handle->height * 3 / 2,
                     threednr_handle->small_width,
                     threednr_handle->small_height);
        }
    }

    // call 3dnr function
    CMR_LOGD("Call the threednr_function(). before. cnt: %d",
             threednr_handle->common.save_frame_count);
    big_buf.gpu_buffer.handle =
        out->private_data; //(unsigned char *)in->src_frame.addr_vir.addr_y;
    small_buf.cpu_buffer.bufferY =
        (unsigned char *)in->src_frame.addr_vir.addr_y +
        threednr_handle->width * threednr_handle->height * 3 / 2;
    small_buf.cpu_buffer.bufferU =
        small_buf.cpu_buffer.bufferY +
        threednr_handle->small_width * threednr_handle->small_height;
    small_buf.cpu_buffer.bufferV = small_buf.cpu_buffer.bufferU;
    CMR_LOGV("Call the threednr_function().big Y: %p, small Y: %p."
             " ,threednr_handle->is_stop %ld",
             big_buf.cpu_buffer.bufferY, small_buf.cpu_buffer.bufferY,
             threednr_handle->is_stop);

    if (threednr_handle->is_stop) {
        CMR_LOGE("threednr_handle is stop");
        goto exit;
    }
    /*CMR_LOGI("add before smallbuf addry:%x , big_buf addry:%x ,
    size:%d,%d", small_buf.cpu_buffer.bufferY , big_buf.cpu_buffer.bufferY,
    threednr_handle->width , threednr_handle->height);*/
    ret = threednr_function(&small_buf, &big_buf);
    if (ret < 0) {
        CMR_LOGE("Fail to call the threednr_function");
    }
    /*CMR_LOGI("add after threednr_function smallbuf addry:%x , big_buf
       addry:%x , size:%d,%d", small_buf.cpu_buffer.bufferY ,
       big_buf.cpu_buffer.bufferY,
        threednr_handle->width , threednr_handle->height);*/
    if (threednr_handle->is_stop) {
        CMR_LOGE("threednr_handle is stop");
        goto exit;
    }

    {
        char flag[PROPERTY_VALUE_MAX];
        property_get("3dnr_save_capture_frame", flag, "0");
        if (!strcmp(flag, "1")) { // save output image.
            sprintf(
                filename,
                "/data/misc/cameraserver/%dx%d_3dnr_handle_frame_index%d.yuv",
                threednr_handle->width, threednr_handle->height, cur_frm);
            save_yuv(filename, (char *)in->dst_frame.addr_vir.addr_y,
                     threednr_handle->width, threednr_handle->height);
        }
    }

    if (CAP_3DNR_NUM == threednr_handle->common.save_frame_count) {
        cmr_bzero(&out->dst_frame, sizeof(struct img_frm));
        oem_handle = threednr_handle->common.ipm_cxt->init_in.oem_handle;
        threednr_handle->frame_in = *in;
        // CMR_LOGI("add all 3dnr frame is ready");
        ret = req_3dnr_do(threednr_handle, addr, size);
    }
exit:
    CMR_LOGD("post sem");
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

static volatile uint8_t *ptemp, *ptemp1, *ptemp2, *ptemp3;
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
    // struct isp_context *isp_cxt = NULL;
    cmr_u32 buf_size;
    cmr_u32 buf_num;
    cmr_u32 small_buf_size;
    cmr_u32 small_buf_num;
    struct preview_smallbuf_node smallbuff_node;
    char flag[PROPERTY_VALUE_MAX];

    CMR_LOGI("E");
    if (!out || !in || !ipm_handle || !class_handle) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }

    property_get("3dnr_setting_from_file", flag, "0");
    if (!strcmp(flag, "1")) {
        char pre_parafile[] = "/data/misc/cameraserver/bst_tdns_settings_image_pre.txt";
        read_param_from_file(pre_parafile);
    }

    threednr_prev_handle =
        (struct class_3dnr_pre *)malloc(sizeof(struct class_3dnr_pre));
    if (!threednr_prev_handle) {
        CMR_LOGE("No mem!");
        return CMR_CAMERA_NO_MEM;
    }

    cmr_bzero(threednr_prev_handle, sizeof(struct class_3dnr_pre));
    ret = threednr_prevthread_create(threednr_prev_handle);
    if (ret != CMR_CAMERA_SUCCESS) {
        CMR_LOGE("threednr_prevthread_create failed");
        return CMR_CAMERA_FAIL;
    }
    size = (cmr_uint)(in->frame_size.width * in->frame_size.height * 3 / 2);

    CMR_LOGD("in->frame_size.width = %d,in->frame_size.height = %d",
             in->frame_size.width, in->frame_size.height);

    sem_init(&threednr_prev_handle->sem_3dnr, 0, 1);
    threednr_prev_handle->reg_cb = in->reg_cb;
    threednr_prev_handle->common.ipm_cxt = (struct ipm_context_t *)ipm_handle;
    threednr_prev_handle->common.class_type = IPM_TYPE_3DNR_PRE;
    threednr_prev_handle->common.ops = &threednr_prev_ops_tab_info;

    oem_handle = threednr_prev_handle->common.ipm_cxt->init_in.oem_handle;
    cam_cxt = (struct camera_context *)oem_handle;
    prev_cxt = (struct preview_context *)&cam_cxt->prev_cxt;

    init_queue_preview_smallbuffer(&(threednr_prev_handle->small_buf_queue));

    threednr_prev_handle->mem_size = size;
    threednr_prev_handle->height = in->frame_size.height;
    threednr_prev_handle->width = in->frame_size.width;
    threednr_prev_handle->small_height = in->frame_size.height / 4;
    threednr_prev_handle->small_width = in->frame_size.width / 4;
    threednr_prev_handle->is_stop = 0;
    threednr_prev_handle->g_num = 0;
    for (int i = 0; i < PRE_SW_3DNR_RESERVE_NUM; i++) {
        threednr_prev_handle->small_buf_info.used[i] = 0;
    }
    // isp_cxt = (struct isp_context *)&(cam_cxt->isp_cxt);
    buf_size =
        threednr_prev_handle->width * threednr_prev_handle->height * 3 / 2;
    buf_num = PRE_SW_3DNR_RESERVE_NUM;
    small_buf_size = threednr_prev_handle->small_width *
                     threednr_prev_handle->small_height * 3 / 2;
    small_buf_num = PRE_SW_3DNR_RESERVE_NUM;

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
        if (0 !=
            cam_cxt->hal_malloc(
                CAMERA_PREVIEW_SCALE_3DNR, &small_buf_size, &small_buf_num,
                (cmr_uint *)threednr_prev_handle->small_buf_info.small_buf_phy,
                (cmr_uint *)threednr_prev_handle->small_buf_info.small_buf_vir,
                threednr_prev_handle->small_buf_info.small_buf_fd,
                cam_cxt->client_data)) {
            CMR_LOGE("Fail to malloc buffers for small image");
        } else {
            CMR_LOGD("OK to malloc buffers for small image");
        }
    } else {
        CMR_LOGE("cam_cxt->hal_malloc is NULL");
    }
    ptemp = malloc(threednr_prev_handle->width * threednr_prev_handle->height *
                   3 / 2);
    ptemp2 = malloc(threednr_prev_handle->width * threednr_prev_handle->height *
                    3 / 2);
    ptemp1 = malloc(threednr_prev_handle->small_width *
                    threednr_prev_handle->small_height * 3 / 2);
    ptemp3 = malloc(threednr_prev_handle->width * threednr_prev_handle->height *
                    3 / 2);
    for (int i = 0; i < PRE_SW_3DNR_RESERVE_NUM; i++) {
        smallbuff_node.buf_vir =
            (uint8_t *)threednr_prev_handle->small_buf_info.small_buf_vir[i];
        smallbuff_node.buf_phy =
            (uint8_t *)threednr_prev_handle->small_buf_info.small_buf_phy[i];
        smallbuff_node.buf_fd =
            threednr_prev_handle->small_buf_info.small_buf_fd[i];
        queue_preview_smallbufer(&(threednr_prev_handle->small_buf_queue),
                                 &(smallbuff_node));
    }
    param.orig_width = threednr_prev_handle->width;
    param.orig_height = threednr_prev_handle->height;
    param.small_width = threednr_prev_handle->small_width;
    param.small_height = threednr_prev_handle->small_height;
    param.total_frame_num = PRE_3DNR_NUM;
    // need release
    threednr_prev_handle->out_image =
        (union c3dnr_buffer *)malloc(sizeof(union c3dnr_buffer));
    threednr_prev_handle->out_image->cpu_buffer.bufferY =
        (uint8_t *)threednr_prev_handle->pre_buf_vir[0];
    threednr_prev_handle->out_image->cpu_buffer.bufferU =
        threednr_prev_handle->out_image->cpu_buffer.bufferY +
        threednr_prev_handle->width * threednr_prev_handle->height;
    threednr_prev_handle->out_image->cpu_buffer.bufferV =
        threednr_prev_handle->out_image->cpu_buffer.bufferU;
    threednr_prev_handle->out_image->cpu_buffer.fd =
        threednr_prev_handle->pre_buf_fd[0];
    param.poutimg = NULL; // threednr_prev_handle->out_image;
    param.gain = 16;

    if (!strcmp(flag, "1")) {
        param.SearchWindow_x = g_SearchWindow_x;
        param.SearchWindow_y = g_SearchWindow_y;
    } else {
        param.SearchWindow_x = 7;
        param.SearchWindow_y = 7;
    }
    CMR_LOGD("set SearWindow : %d, %d", param.SearchWindow_x, param.SearchWindow_y);

    param.low_thr = 100;
    param.ratio = 2;
    param.sigma_tmp = sigma_tmp;
    param.slope_tmp = slope_tmp;
    param.threthold = threthold;
    param.slope = slope;
    param.yuv_mode = 0; // NV12?
    param.thread_num_acc = 2 | (1 << 4) | (2 << 6) | (1 << 12);
    param.thread_num = 2 | (1 << 4) | (2 << 6) | (1 << 12);
    param.recur_str = 3;
    param.control_en = 0x0;
    param.preview_cpyBuf = 1;
    // param.videoflag = 1;
    /*CMR_LOGI("add small width:%d ,height:%d , big width:%d ,height:%d",
             param.small_width, param.small_height, param.orig_width,
             param.orig_height);*/
    ret = threednr_init(&param);
    if (ret < 0) {
        CMR_LOGE("Fail to call preview threednr_init");
    } else {
        CMR_LOGD("ok to call preview threednr_init");
    }
    *class_handle = (cmr_handle)threednr_prev_handle;

    // pbig = (uint8_t*)
    // malloc(threednr_prev_handle->height*threednr_prev_handle->width*3/2);
    // psmall = (uint8_t*)
    // malloc(threednr_prev_handle->small_height*threednr_prev_handle->small_width*3/2);
    // pvideo = (uint8_t*)
    // malloc(threednr_prev_handle->height*threednr_prev_handle->width*3/2);
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
    ret = threednr_prevthread_destroy(threednr_prev_handle);
    if (ret) {
        CMR_LOGE("3dnr failed to destroy 3dnr prev thread");
    }

    CMR_LOGI("OK to threednr_cancel for preview");
    ret = threednr_deinit();
    if (ret) {
        CMR_LOGE("3dnr preview failed to threednr_deinit");
    }

    sem_destroy(&threednr_prev_handle->sem_3dnr);
    oem_handle = threednr_prev_handle->common.ipm_cxt->init_in.oem_handle;
    cam_cxt = (struct camera_context *)oem_handle;

    if (NULL != cam_cxt->hal_malloc) {
        if (0 !=
            cam_cxt->hal_free(CAMERA_PREVIEW_3DNR,
                              threednr_prev_handle->pre_buf_phy,
                              threednr_prev_handle->pre_buf_vir,
                              threednr_prev_handle->pre_buf_fd,
                              PRE_SW_3DNR_RESERVE_NUM, cam_cxt->client_data)) {
            CMR_LOGE("Fail to free the preview output buffer");
        } else {
            CMR_LOGD("OK to free thepreview  output buffer");
        }

        if (0 !=
            cam_cxt->hal_free(
                CAMERA_PREVIEW_SCALE_3DNR,
                (cmr_uint *)threednr_prev_handle->small_buf_info.small_buf_phy,
                (cmr_uint *)threednr_prev_handle->small_buf_info.small_buf_vir,
                threednr_prev_handle->small_buf_info.small_buf_fd,
                PRE_SW_3DNR_RESERVE_NUM, cam_cxt->client_data)) {
            CMR_LOGE("Fail to free the small image buffers");
        }
    } else {
        CMR_LOGD("cam_cxt->hal_free is NULL");
    }

    if (threednr_prev_handle->out_image)
        free(threednr_prev_handle->out_image);

    deinit_queue_preview_smallbufer(&(threednr_prev_handle->small_buf_queue));
    if (NULL != threednr_prev_handle)
        free(threednr_prev_handle);
    CMR_LOGI("X");

    return ret;
}
cmr_int threednr_transfer_prev_frame(cmr_handle class_handle,
                                     struct ipm_frame_in *in,
                                     struct ipm_frame_out *out) {

    struct img_frm src, dst;
    cmr_u32 cur_frm_idx;
    int ret;
    struct class_3dnr_pre *threednr_prev_handle =
        (struct class_3dnr_pre *)class_handle;
    struct prev_threednr_info *pthreednr_info;
    struct preview_smallbuf_node smallbuf_node;
    //    int ret = get_free_smallbuffer(class_handle ,
    //    &cur_frm_idx);//threednr_prev_handle->g_num%PRE_SW_3DNR_RESERVE_NUM;
    if (!in) {
        CMR_LOGE("incorrect parameter");
        return -1;
    }

    ret = dequeue_preview_smallbuffer(&(threednr_prev_handle->small_buf_queue),
                                      &smallbuf_node);
    if (ret != 0) {
        CMR_LOGE(
            "add threednr_transfer_prev_frame failed no free small buffer");
        return -1;
    }
    // CMR_LOGI("add get_free_smallbuffer cur_frm_idx is id:%d" ,
    // cur_frm_idx);
    src = in->src_frame;
    src.data_end.y_endian = 0;
    src.data_end.uv_endian = 0;
    src.rect.start_x = 0;
    src.rect.start_y = 0;
    src.rect.width = threednr_prev_handle->width;
    src.rect.height = threednr_prev_handle->height;
    memcpy(&dst, &src, sizeof(struct img_frm));
    dst.buf_size = threednr_prev_handle->small_width *
                   threednr_prev_handle->small_height * 3 / 2;
    dst.rect.start_x = 0;
    dst.rect.start_y = 0;
    dst.rect.width = threednr_prev_handle->small_width;
    dst.rect.height = threednr_prev_handle->small_height;
    dst.size.width = threednr_prev_handle->small_width;
    dst.size.height = threednr_prev_handle->small_height;
    dst.addr_phy.addr_y =
        (cmr_uint)smallbuf_node
            .buf_phy; // threednr_prev_handle->small_buf_info.small_buf_phy[cur_frm_idx];
    dst.addr_phy.addr_u =
        dst.addr_phy.addr_y +
        threednr_prev_handle->small_width * threednr_prev_handle->small_height;
    dst.addr_phy.addr_v = dst.addr_phy.addr_u;
    dst.addr_vir.addr_y =
        (cmr_uint)smallbuf_node
            .buf_vir; // threednr_prev_handle->small_buf_info.small_buf_vir[cur_frm_idx];
    dst.addr_vir.addr_u =
        dst.addr_vir.addr_y +
        threednr_prev_handle->small_width * threednr_prev_handle->small_height;
    dst.addr_vir.addr_v = dst.addr_vir.addr_u;
    dst.fd =
        smallbuf_node
            .buf_fd; // threednr_prev_handle->small_buf_info.small_buf_fd[cur_frm_idx];
    int64_t time = systemTime(CLOCK_MONOTONIC);

    ret = threednr_start_scale(
        threednr_prev_handle->common.ipm_cxt->init_in.oem_handle, &src, &dst);
    if (ret) {
        CMR_LOGE("Fail to call threednr_start_scale");
        goto exit;
    }
    in->dst_frame = dst;
    ((struct prev_threednr_info *)(in->private_data))->frm_smallpreview = dst;
    pthreednr_info = in->private_data;
    ret = req_3dnr_preview_frame(class_handle, in, out, pthreednr_info,
                                 &smallbuf_node /*cur_frm_idx*/);
//    threednr_prev_handle->g_num++;
exit:
    return ret;
}
cmr_int threednr_process_prev_frame(cmr_handle class_handle,
                                    struct ipm_frame_in *in,
                                    struct ipm_frame_out *out)

{
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_3dnr_pre *threednr_prev_handle =
        (struct class_3dnr_pre *)class_handle;
    cmr_u32 frame_in_cnt;
    struct img_addr *addr;
    struct img_size size;
    cmr_handle oem_handle;
    cmr_u32 sensor_id = 0;
    cmr_u32 threednr_enable = 0;
    union c3dnr_buffer big_buf, small_buf, video_buf;
    struct img_frm *src, dst;
    cmr_u32 cur_frm_idx;
    char filename[128];
    struct c3dnr_pre_inparam preview_param;

    if (!class_handle || !in || !out) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    sem_wait(&threednr_prev_handle->sem_3dnr);
    int64_t time_1 = systemTime(CLOCK_MONOTONIC);

    addr = &in->dst_frame.addr_vir;
    size = in->src_frame.size;

    oem_handle = threednr_prev_handle->common.ipm_cxt->init_in.oem_handle;
    if (threednr_prev_handle->is_stop) {
        return CMR_CAMERA_FAIL;
    }
    int64_t time = systemTime(CLOCK_MONOTONIC);

    {
        char flag[PROPERTY_VALUE_MAX];
        static int index = 0;
        property_get("post_3dnr_save_scl_data", flag, "0");
        if (!strcmp(flag, "1")) { // save input image.
            CMR_LOGI("save pic: %d, threednr_prev_handle->g_num: %d.", index,
                     threednr_prev_handle->g_num);
            sprintf(filename,
                    "/data/misc/cameraserver/scl_in_%dx%d_index_%d.yuv",
                    threednr_prev_handle->width, threednr_prev_handle->height,
                    index);
            save_yuv(filename, (char *)in->src_frame.addr_vir.addr_y,
                     threednr_prev_handle->width, threednr_prev_handle->height);
            sprintf(filename,
                    "/data/misc/cameraserver/scl_out_%dx%d_index_%d.yuv",
                    threednr_prev_handle->small_width,
                    threednr_prev_handle->small_height, index);
            save_yuv(filename, (char *)in->dst_frame.addr_vir.addr_y,
                     threednr_prev_handle->small_width,
                     threednr_prev_handle->small_height);
            index++;
        }
    }

    // call 3dnr function
    CMR_LOGV("Call the threednr_function(). before. cnt: %d, fd: 0x%x",
             threednr_prev_handle->common.save_frame_count, in->src_frame.fd);
    // memcpy(pbig , (unsigned char*)(in->src_frame.addr_vir.addr_y) ,
    // threednr_prev_handle->width * threednr_prev_handle->height*3/2);
    big_buf.cpu_buffer.bufferY =
        (unsigned char *)(in->src_frame.addr_vir.addr_y);
    big_buf.cpu_buffer.bufferU =
        big_buf.cpu_buffer.bufferY +
        threednr_prev_handle->width * threednr_prev_handle->height;
    big_buf.cpu_buffer.bufferV = big_buf.cpu_buffer.bufferU;
    big_buf.cpu_buffer.fd = in->src_frame.fd;
    // memcpy(psmall , (unsigned char *)in->dst_frame.addr_vir.addr_y ,
    // threednr_prev_handle->small_width *
    // threednr_prev_handle->small_height*3/2);
    small_buf.cpu_buffer.bufferY =
        (unsigned char *)
            in->dst_frame.addr_vir.addr_y; // small_buf_vir[cur_frm_idx];
    small_buf.cpu_buffer.bufferU =
        small_buf.cpu_buffer.bufferY +
        threednr_prev_handle->small_width * threednr_prev_handle->small_height;
    small_buf.cpu_buffer.bufferV = small_buf.cpu_buffer.bufferU;
    small_buf.cpu_buffer.fd =
        0; // threednr_prev_handle->small_buf_fd[cur_frm_idx];
    // if(0 != out->dst_frame.addr_vir.addr_y)
    // memcpy(pvideo , (unsigned char *)out->dst_frame.addr_vir.addr_y ,
    // threednr_prev_handle->width * threednr_prev_handle->height*3/2);
    video_buf.cpu_buffer.bufferY =
        (unsigned char *)out->dst_frame.addr_vir.addr_y;
    video_buf.cpu_buffer.bufferU =
        video_buf.cpu_buffer.bufferY +
        threednr_prev_handle->width * threednr_prev_handle->height;
    video_buf.cpu_buffer.bufferV = video_buf.cpu_buffer.bufferU;
    preview_param.gain = in->adgain;
    CMR_LOGD("Call the threednr_function().big Y: %p, 0x%x, small Y: %p, 0x%x, "
             "video Y: %p, 0x%x, adgain: %d"
             " ,threednr_prev_handle->is_stop %ld",
             big_buf.cpu_buffer.bufferY, big_buf.cpu_buffer.fd,
             small_buf.cpu_buffer.bufferY, small_buf.cpu_buffer.fd,
             video_buf.cpu_buffer.bufferY, video_buf.cpu_buffer.fd,
             preview_param.gain, threednr_prev_handle->is_stop);

    if (threednr_prev_handle->is_stop) {
        return CMR_CAMERA_FAIL;
    }
    int64_t time_2;

#if 0
	 FILE *fp;
     char filename[256];
     static int index = 0;
	 sprintf(filename , "/data/misc/cameraserver/%dx%d_preview_before_index%d.yuv" , threednr_prev_handle->width , threednr_prev_handle->height , index);
	 fp = fopen(filename , "wb");
     if(fp) {
         fwrite(big_buf.cpu_buffer.bufferY , 1 , threednr_prev_handle->width*threednr_prev_handle->height*3/2 , fp);
         fclose(fp);
     }
#endif
    static int index_total = 0;
    int cycle_index;
    static int64_t time_total = 0;
    cycle_index = index_total % 100;
    if (cycle_index == 0)
        time_total = systemTime(CLOCK_MONOTONIC);
    if (cycle_index == 99) {
        CMR_LOGI("3dnr effect costtime:%lld ms , index_total:%d",
                 (systemTime(CLOCK_MONOTONIC) - time_total) / 1000000,
                 index_total);
    }
    index_total++;
    time_2 = systemTime(CLOCK_MONOTONIC);

    char value[128];
    property_get("3dnrclose", value, "0");
    if (!strcmp(value, "0")) {
        if (video_buf.cpu_buffer.bufferY != NULL) {
            CMR_LOGV("add threednr_function_pre previewbuffer with video :%p , "
                     "small:%p , video buffer:%p",
                     big_buf.cpu_buffer.bufferY, small_buf.cpu_buffer.bufferY,
                     video_buf.cpu_buffer.bufferY);

            if ((small_buf.cpu_buffer.bufferY != NULL) &&
                (big_buf.cpu_buffer.bufferY != NULL))
                ret = threednr_function_pre(&small_buf, &big_buf, &video_buf,
                                            &preview_param);
            else {
                CMR_LOGE(
                    "preview or scale image is null, direct copy video buffer");
                memcpy(video_buf.cpu_buffer.bufferY, big_buf.cpu_buffer.bufferY,
                       threednr_prev_handle->width *
                           threednr_prev_handle->height * 3 / 2);
            }
        } else {
            static int index = 0;
            ptemp[0] = 1;
            property_get("3dnr_save", value, "0");
            FILE *fp;
            sprintf(filename,
                    "/data/misc/cameraserver/%dx%d_preview_index%d.yuv",
                    threednr_prev_handle->width, threednr_prev_handle->height,
                    index);
            if (!strcmp(value, "1")) {
                sprintf(filename,
                        "/data/misc/cameraserver/%dx%d_preview_index%d.yuv",
                        threednr_prev_handle->width,
                        threednr_prev_handle->height, index);
                fp = fopen(filename, "wb");
                if (fp) {
                    fwrite(big_buf.cpu_buffer.bufferY, 1,
                           threednr_prev_handle->width *
                               threednr_prev_handle->height * 3 / 2,
                           fp);
                    fclose(fp);
                }
                sprintf(filename,
                        "/data/misc/cameraserver/%dx%d_preview_index%d.yuv",
                        threednr_prev_handle->small_width,
                        threednr_prev_handle->small_height, index);
                fp = fopen(filename, "wb");

                if (fp) {
                    fwrite(small_buf.cpu_buffer.bufferY, 1,
                           threednr_prev_handle->small_width *
                               threednr_prev_handle->small_height * 3 / 2,
                           fp);
                    fclose(fp);
                }
            }

            if ((small_buf.cpu_buffer.bufferY != NULL) &&
                (big_buf.cpu_buffer.bufferY != NULL))
                ret = threednr_function_pre(&small_buf, &big_buf, NULL,
                                            &preview_param);

            if (!strcmp(value, "1")) {
                sprintf(
                    filename,
                    "/data/misc/cameraserver/%dx%d_preview_result_index%d.yuv",
                    threednr_prev_handle->width, threednr_prev_handle->height,
                    index);
                fp = fopen(filename, "wb");
                if (fp) {
                    fwrite(big_buf.cpu_buffer.bufferY, 1,
                           threednr_prev_handle->width *
                               threednr_prev_handle->height * 3 / 2,
                           fp);
                    fclose(fp);
                }
            }
            CMR_LOGV("add threednr_function_pre previewbuffer:%p , small:%p",
                     big_buf.cpu_buffer.bufferY, small_buf.cpu_buffer.bufferY);
            index++;
        }
    } else {
        CMR_LOGI("force to close 3dnr, only by pass");
        if (video_buf.cpu_buffer.bufferY != NULL)
            memcpy(video_buf.cpu_buffer.bufferY, big_buf.cpu_buffer.bufferY,
                   threednr_prev_handle->width * threednr_prev_handle->height *
                       3 / 2);
    }

    if (ret < 0) {
        CMR_LOGE("Fail to call the threednr_function");
    }
    sem_post(&threednr_prev_handle->sem_3dnr);
exit:
    return ret;
}

static cmr_int threednr_post_proc(cmr_handle class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    return ret;
}

cmr_int threednr_start_scale(cmr_handle oem_handle, struct img_frm *src,
                             struct img_frm *dst) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    if (!oem_handle || !src || !dst) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    CMR_LOGV(
        "src size %d %d dst size %d %d rect %d %d %d %d endian %d %d %d %d",
        src->size.width, src->size.height, dst->size.width, dst->size.height,
        src->rect.start_x, src->rect.start_y, src->rect.width, src->rect.height,
        src->data_end.y_endian, src->data_end.uv_endian, dst->data_end.y_endian,
        dst->data_end.uv_endian);

    CMR_LOGD(
        "src fd: 0x%x, yaddr: 0x%x, fmt: %d dst fd: 0x%x, yaddr: 0x%x, fmt: %d",
        src->fd, src->addr_vir.addr_y, src->fmt, dst->fd, dst->addr_vir.addr_y,
        dst->fmt);
    ret = cmr_scale_start(cxt->scaler_cxt.scaler_handle, src, dst,
                          (cmr_evt_cb)NULL, NULL);
    if (ret) {
        CMR_LOGE("failed to start scaler, ret %ld", ret);
    }
exit:
    CMR_LOGV("done %ld", ret);

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

static cmr_int threednr_transfer_frame(cmr_handle class_handle,
                                       struct ipm_frame_in *in,
                                       struct ipm_frame_out *out) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_3dnr *threednr_handle = (struct class_3dnr *)class_handle;
    cmr_u32 frame_in_cnt;
    struct img_addr *addr;
    struct img_size size;
    cmr_handle oem_handle;
    cmr_u32 sensor_id = 0;
    cmr_u32 dnr_enable = 0;
    union c3dnr_buffer big_buf, small_buf;
    cmr_u32 cur_num = threednr_handle->g_num;

    if (!out || !in || !class_handle) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }

    if (threednr_handle->is_stop) {
        return 0;
    }

    CMR_LOGD("get one frame, num %d, %d", cur_num, threednr_handle->g_totalnum);
    if (threednr_handle->g_totalnum < CAP_3DNR_NUM) {
        threednr_handle->g_info_3dnr[cur_num].class_handle = class_handle;
        memcpy(&threednr_handle->g_info_3dnr[cur_num].in, in,
               sizeof(struct ipm_frame_in));
        memcpy(&threednr_handle->g_info_3dnr[cur_num].out, out,
               sizeof(struct ipm_frame_out));
        create_3dnr_thread(&threednr_handle->g_info_3dnr[cur_num]);
    } else {
        CMR_LOGE("got more than %d 3dnr capture images, now got %d images",
                 CAP_3DNR_NUM, threednr_handle->g_totalnum);
    }
    threednr_handle->g_num++;
    threednr_handle->g_num = threednr_handle->g_num % CAP_3DNR_NUM;
    threednr_handle->g_totalnum++;

    return ret;
}

static cmr_int threednr_prevthread_create(struct class_3dnr_pre *class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    CMR_MSG_INIT(message);

    CHECK_HANDLE_VALID(class_handle);

    if (!class_handle->is_inited) {
        ret = cmr_thread_create(&class_handle->threednr_prevthread,
                                /*CAMERA_3DNR_MSG_QUEUE_SIZE*/ 8,
                                threednr_process_prevthread_proc,
                                (void *)class_handle);
        if (ret) {
            CMR_LOGE("send msg failed!");
            ret = CMR_CAMERA_FAIL;
            return ret;
        }
        ret = cmr_thread_set_name(class_handle->threednr_prevthread,
                                  "threednr_prv");
        if (CMR_MSG_SUCCESS != ret) {
            CMR_LOGE("fail to set 3dnr prev name");
            ret = CMR_MSG_SUCCESS;
        }
        message.msg_type = CMR_EVT_3DNR_PREV_INIT;
        message.sync_flag = CMR_MSG_SYNC_RECEIVED;
        ret = cmr_thread_msg_send(class_handle->threednr_prevthread, &message);
        if (ret) {
            CMR_LOGE("send msg failed!");
            ret = CMR_CAMERA_FAIL;

        } else
            class_handle->is_inited = 1;
    }

    return ret;
}
static cmr_int
threednr_prevthread_destroy(struct class_3dnr_pre *class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    CMR_MSG_INIT(message);

    CHECK_HANDLE_VALID(class_handle);

    if (class_handle->is_inited) {

        ret = cmr_thread_destroy(class_handle->threednr_prevthread);
        class_handle->threednr_prevthread = 0;

        class_handle->is_inited = 0;
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
        ret = cmr_thread_set_name(class_handle->threednr_thread, "threednr");
        if (CMR_MSG_SUCCESS != ret) {
            CMR_LOGE("fail to set 3dnr name");
            ret = CMR_MSG_SUCCESS;
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
static cmr_int
req_3dnr_preview_frame(cmr_handle class_handle, struct ipm_frame_in *in,
                       struct ipm_frame_out *out,
                       struct prev_threednr_info *threednr_info,
                       struct preview_smallbuf_node *psmall_buf_node) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_3dnr_pre *threednr_prev_handle =
        (struct class_3dnr_pre *)class_handle;
    struct process_pre_3dnr_info *ptemp;
    CMR_MSG_INIT(message);

    if (!class_handle || !in || !out) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }

    message.data = (struct process_pre_3dnr_info *)malloc(
        sizeof(struct process_pre_3dnr_info));
    if (!message.data) {
        CMR_LOGE("No mem!");
        ret = CMR_CAMERA_NO_MEM;
        return ret;
    }
    ptemp = message.data;
    memcpy(&ptemp->in, in, sizeof(struct ipm_frame_in));
    memcpy(&ptemp->out, out, sizeof(struct ipm_frame_out));
    ptemp->smallbuf_node = *psmall_buf_node;
    ptemp->threednr_info.frm_preview = in->src_frame;
    ptemp->threednr_info.frm_smallpreview = in->dst_frame;
    ptemp->threednr_info.frm_video = out->dst_frame;
    ptemp->threednr_info.framtype = threednr_info->framtype;
    ptemp->threednr_info.data = threednr_info->data;
    ptemp->threednr_info.camera_id = threednr_info->camera_id;
    ptemp->threednr_info.caller_handle = threednr_info->caller_handle;
    message.msg_type = CMR_EVT_3DNR_PREV_START;
    message.sync_flag = CMR_MSG_SYNC_RECEIVED;
    message.alloc_flag = 1;
    /*CMR_LOGI("add cmr_thread_msg_send threednr_prevthread:%p",
             threednr_prev_handle->threednr_prevthread);*/
    ret = cmr_thread_msg_send(threednr_prev_handle->threednr_prevthread,
                              &message);
    if (ret) {
        CMR_LOGE("Failed to send one msg to 3dnr thread");
        if (message.data) {
            free(message.data);
        }
    }
    return ret;
}
static cmr_int threednr_process_prevthread_proc(struct cmr_msg *message,
                                                void *private_data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_3dnr *class_handle = (struct class_3dnr *)private_data;
    cmr_u32 evt = 0;
    struct ipm_frame_out out;
    struct ipm_frame_in *in;
    struct process_pre_3dnr_info info;
    if (!message || !class_handle) {
        CMR_LOGE("parameter is fail");
        return CMR_CAMERA_INVALID_PARAM;
    }

    evt = (cmr_u32)message->msg_type;

    switch (evt) {
    case CMR_EVT_3DNR_PREV_INIT:
        CMR_LOGI("3dnr thread inited.");
        break;
    case CMR_EVT_3DNR_PREV_START:
        CMR_LOGV("CMR_EVT_3DNR_PREV_START.");
        struct class_3dnr_pre *threednr_prev_handle =
            (struct class_3dnr_pre *)class_handle;
        struct ipm_frame_out frame_out;
        info = *((struct process_pre_3dnr_info *)(message->data));
        threednr_process_prev_frame(class_handle, &info.in, &info.out);
        // threednr_prev_handle->small_buf_info.used[info.small_buf_index] = 0;
        queue_preview_smallbufer(&(threednr_prev_handle->small_buf_queue),
                                 &(info.smallbuf_node));
        struct prev_threednr_info threednr_info;
        // threednr_info.frm_preview = info.in.src_frame;
        threednr_info = info.threednr_info;
        frame_out.private_data = &threednr_info;
        frame_out.caller_handle = threednr_info.caller_handle;
        threednr_prev_handle->reg_cb(0, &frame_out);
        break;
    case CMR_EVT_3DNR_PREV_EXIT:
        CMR_LOGD("3dnr prev thread exit.");
        break;
    default:
        break;
    }

    return ret;
}

cmr_int threednr_pre_getinfo(void *class_handle,
                             struct threednr_pre_miscinfo *pre_info) {
    struct class_3dnr_pre *threednr_prev_handle =
        (struct class_3dnr_pre *)class_handle;
    pre_info->small_width = threednr_prev_handle->small_width;
    pre_info->small_height = threednr_prev_handle->small_height;
    pre_info->width = threednr_prev_handle->width;
    pre_info->height = threednr_prev_handle->height;
    pre_info->small_buf_vir =
        threednr_prev_handle->small_buf_info.small_buf_vir;
    pre_info->small_buf_phy =
        threednr_prev_handle->small_buf_info.small_buf_phy;
    pre_info->small_buf_fd =
        (cmr_s32 *)(threednr_prev_handle->small_buf_info.small_buf_fd);
    return CMR_CAMERA_SUCCESS;
}

static cmr_int init_queue_preview_smallbuffer(
    struct preview_smallbuf_queue *psmall_buf_queue) {
    psmall_buf_queue->head = NULL;
    psmall_buf_queue->tail = NULL;
    pthread_mutex_init(&psmall_buf_queue->mutex, NULL);
    pthread_cond_init(&psmall_buf_queue->cond, NULL);
    return 0;
}
static cmr_int
dequeue_preview_smallbuffer(struct preview_smallbuf_queue *psmall_buf_queue,
                            struct preview_smallbuf_node *pnode) {
    struct preview_smallbuf_node *ptemp;
    pthread_mutex_lock(&psmall_buf_queue->mutex);
    // CMR_LOGE("no free small buffer");
    while (psmall_buf_queue->head == NULL) {
        CMR_LOGE("no free small buffer");
        pthread_cond_wait(&psmall_buf_queue->cond, &psmall_buf_queue->mutex);
    }
    {
        *pnode = *(psmall_buf_queue->head);
        free(psmall_buf_queue->head);
        psmall_buf_queue->head = psmall_buf_queue->head->next;
        if (NULL == psmall_buf_queue->head) {
            psmall_buf_queue->tail = NULL;
        }
        // CMR_LOGI("add dequeue small buff vir addr:%p" , pnode->buf_vir);
        pthread_mutex_unlock(&psmall_buf_queue->mutex);
        return 0;
    }
}
static cmr_int
queue_preview_smallbufer(struct preview_smallbuf_queue *psmall_buf_queue,
                         struct preview_smallbuf_node *pnode) {
    struct preview_smallbuf_node *pnewnode =
        (struct preview_smallbuf_node *)malloc(
            sizeof(struct preview_smallbuf_node));
    CMR_LOGV("add new node:%p , smallbffqueue:%p", pnewnode, psmall_buf_queue);
    pthread_mutex_lock(&psmall_buf_queue->mutex);
    if ((NULL == pnewnode) || (NULL == pnode)) {
        CMR_LOGE("alloc pnewnode failed");
        pthread_mutex_unlock(&psmall_buf_queue->mutex);
        return -1;
    }
    *pnewnode = *pnode;
    pnewnode->next = NULL;
    if (NULL == psmall_buf_queue->head) {
        psmall_buf_queue->head = pnewnode;
        psmall_buf_queue->tail = pnewnode;
    } else {
        psmall_buf_queue->tail->next = pnewnode;
        psmall_buf_queue->tail = pnewnode;
    }
    pthread_cond_signal(&psmall_buf_queue->cond);
    pthread_mutex_unlock(&psmall_buf_queue->mutex);
    return 0;
}
static cmr_int deinit_queue_preview_smallbufer(
    struct preview_smallbuf_queue *psmall_buf_queue) {
    struct preview_smallbuf_node *pnode;
    pthread_mutex_lock(&psmall_buf_queue->mutex);
    pnode = psmall_buf_queue->head;
    while (pnode != NULL) {
        psmall_buf_queue->head = psmall_buf_queue->head->next;
        CMR_LOGV("add free pnode:%p , smallbffqueue:%p", pnode,
                 psmall_buf_queue);
        free(pnode);
        pnode = psmall_buf_queue->head;
    }

    psmall_buf_queue->tail = NULL;
    pthread_mutex_unlock(&psmall_buf_queue->mutex);
    pthread_mutex_destroy(&psmall_buf_queue->mutex);
    pthread_cond_destroy(&psmall_buf_queue->cond);
    return 0;
}
#if 0
cmr_int threednr_pre_addframenum(void *class_handle)
{
    struct class_3dnr_pre *threednr_prev_handle = (struct class_3dnr_pre *)class_handle;
    threednr_prev_handle->g_num++;
    return CMR_CAMERA_SUCCESS;
}
cmr_uint threednr_pre_get_gnum(void *class_handle)
{
    struct class_3dnr_pre *threednr_prev_handle = (struct class_3dnr_pre *)class_handle;
    return threednr_prev_handle->g_num;
}
#endif
#endif
