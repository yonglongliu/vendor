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

#define LOG_TAG "cmr_filter"

#include "cmr_ipm.h"
#include "cmr_filter.h"
#include <cutils/properties.h>

#define SPRD_FILTER_VERSION (0)
#define ARCSOFT_FILTER_VERSION (1)

#define CHECK_HANDLE_VALID(handle)                                             \
    do {                                                                       \
        if (!handle) {                                                         \
            return -CMR_CAMERA_INVALID_PARAM;                                  \
        }                                                                      \
    } while (0)

static cmr_int filter_open(cmr_handle ipm_handle, struct ipm_open_in *in,
                           struct ipm_open_out *out, cmr_handle *class_handle);
static cmr_int filter_close(cmr_handle class_handle);
static cmr_int filter_transfer_frame(cmr_handle class_handle,
                                     struct ipm_frame_in *in,
                                     struct ipm_frame_out *out);
static cmr_int filter_arithmetic_do(cmr_handle class_handle,
                                    struct img_addr *dst_addr, cmr_u32 width,
                                    cmr_u32 height);
static cmr_int filter_arithmetic_init(cmr_handle class_handle);
static cmr_int filter_arithmetic_deinit(cmr_handle class_handle);

#if (CONFIG_FILTER_VERSION == SPRD_FILTER_VERSION)
extern struct filter_ops sprd_filter_ops;
#elif(CONFIG_FILTER_VERSION == ARCSOFT_FILTER_VERSION)
extern struct filter_ops arcsoft_filter_ops;
#endif

static struct filter_ops *filter_ops_mode[] = {
#if (CONFIG_FILTER_VERSION == SPRD_FILTER_VERSION)
        [SPRD_FILTER_VERSION] = &sprd_filter_ops,
#elif(CONFIG_FILTER_VERSION == ARCSOFT_FILTER_VERSION)
        [ARCSOFT_FILTER_VERSION] = &arcsoft_filter_ops,
#endif
};

static struct class_ops filter_ops_tab_info = {
    filter_open, filter_close, filter_transfer_frame, NULL, NULL,
};

struct class_tab_t filter_tab_info = {
    &filter_ops_tab_info,
};

static cmr_int filter_open(cmr_handle ipm_handle, struct ipm_open_in *in,
                           struct ipm_open_out *out, cmr_handle *class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_filter *filter_handle = NULL;

    if (!out || !in || !ipm_handle || !class_handle) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }

    filter_handle = (struct class_filter *)malloc(sizeof(struct class_filter));
    if (!filter_handle) {
        CMR_LOGE("No mem!");
        return CMR_CAMERA_NO_MEM;
    }

    cmr_bzero(filter_handle, sizeof(struct class_filter));

    out->format = IMG_FMT_YCBCR420;
    out->total_frame_number = 1;

    CMR_LOGI("in->frame_size.width = %d,in->frame_size.height = %d",
             in->frame_size.width, in->frame_size.height);

    filter_handle->common.ipm_cxt = (struct ipm_context_t *)ipm_handle;
    filter_handle->common.class_type = IPM_TYPE_FILTER;
    filter_handle->common.receive_frame_count = 0;
    filter_handle->common.save_frame_count = 0;
    filter_handle->common.ops = &filter_ops_tab_info;

    filter_handle->height = in->frame_size.height;
    filter_handle->width = in->frame_size.width;
    filter_handle->reg_cb = in->reg_cb;

    filter_handle->vendor = CONFIG_FILTER_VERSION;
    if (filter_handle->vendor == 0xFF) {
        CMR_LOGE("Not algorithm support the filter mode");
        goto exit;
    }

    filter_handle->filter_ops = filter_ops_mode[filter_handle->vendor];
    CMR_LOGI("filter version %d, ops %p", filter_handle->vendor,
             filter_handle->filter_ops);

    ret = filter_arithmetic_init((cmr_handle)filter_handle);
    if (ret) {
        CMR_LOGE("failed to filter_arithmetic_init");
        goto exit;
    }
    filter_handle->is_inited = 1;
    sem_init(&filter_handle->sem, 0, 1);

    *class_handle = (cmr_handle)filter_handle;
    CMR_LOGI("x");

    return ret;

exit:
    if (NULL != filter_handle)
        free(filter_handle);
    return CMR_CAMERA_FAIL;
}

static cmr_int filter_close(cmr_handle class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_filter *filter_handle = (struct class_filter *)class_handle;
    CHECK_HANDLE_VALID(filter_handle);
    if (!filter_handle) {
        CMR_LOGE("filter_handle is null");
        return CMR_CAMERA_INVALID_PARAM;
    }
    CMR_LOGI("E");

    if (filter_handle->is_inited) {
        sem_wait(&filter_handle->sem);
        ret = filter_arithmetic_deinit(class_handle);
        if (ret) {
            CMR_LOGE("failed to deinit");
        }
        sem_post(&filter_handle->sem);
        sem_destroy(&filter_handle->sem);
    }

    filter_handle->is_inited = 0;
    free(filter_handle);
    CMR_LOGI("X");

    return ret;
}

static cmr_int filter_transfer_frame(cmr_handle class_handle,
                                     struct ipm_frame_in *in,
                                     struct ipm_frame_out *out) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_filter *filter_handle = (struct class_filter *)class_handle;
    struct img_addr *addr;
    cmr_uint width = 0;
    cmr_uint height = 0;
    struct camera_context *cxt = (struct camera_context *)in->private_data;

    CMR_LOGI("E");

    if (!out || !in || !class_handle || !cxt) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }
    if (!filter_handle->is_inited) {
        return ret;
    }
    sem_wait(&filter_handle->sem);

    addr = &in->src_frame.addr_vir;
    width = in->src_frame.size.width;
    height = in->src_frame.size.height;
    filter_handle->filter_type = (int)out->private_data;

    char value[PROPERTY_VALUE_MAX];
    property_get("debug.dump.filter.frame", value, "null");
    if (!strcmp(value, "true")) {
        camera_save_yuv_to_file(0, IMG_DATA_TYPE_YUV420, width, height, addr);
    }
    CMR_LOGI("w=%d,h=%d,type=%d", width, height, filter_handle->filter_type);

    ret = filter_arithmetic_do(class_handle, addr, width, height);
    if (ret) {
        CMR_LOGE("failed to filter_arithmetic_do");
        goto exit;
    }

    out->dst_frame = in->src_frame;
    out->private_data = in->private_data;

    CMR_LOGI("x,private_data=%p,type=%d", out->private_data,
             filter_handle->filter_type);
exit:
    sem_post(&filter_handle->sem);
    return ret;
}

static cmr_int filter_arithmetic_init(cmr_handle class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    struct class_filter *filter_handle = (struct class_filter *)class_handle;
    CMR_LOGI("E");

    cmr_uint type = filter_handle->filter_type;
    cmr_uint width = filter_handle->width;
    cmr_uint height = filter_handle->height;

    if (filter_handle->filter_ops && filter_handle->filter_ops->create) {
        ret = filter_handle->filter_ops->create(&(filter_handle->handle), type,
                                                width, height);
        if (ret) {
            CMR_LOGE("failed to create");
        }
    } else {
        CMR_LOGE("filter_ops or create is null");
        ret = CMR_CAMERA_FAIL;
    }

    CMR_LOGI("x");

    return ret;
}

static cmr_int filter_arithmetic_deinit(cmr_handle class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_filter *filter_handle = (struct class_filter *)class_handle;
    CMR_LOGI("E");

    cmr_handle handle = filter_handle->handle;

    if (filter_handle->filter_ops && filter_handle->filter_ops->destroy) {
        if (handle) {
            ret = filter_handle->filter_ops->destroy(handle);
            if (ret) {
                CMR_LOGE("failed to destroy");
                goto exit;
            }
        } else {
            CMR_LOGW("filter handle is null");
        }
    } else {
        CMR_LOGE("filter_ops or destroy is null");
        ret = CMR_CAMERA_FAIL;
    }

    CMR_LOGI("x");
exit:
    return ret;
}

static cmr_int filter_arithmetic_do(cmr_handle class_handle,
                                    struct img_addr *dst_addr, cmr_u32 width,
                                    cmr_u32 height) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct timespec start_time, end_time;
    cmr_uint duration = 0;

    CMR_LOGI("E");
    struct filter_pic_data pic_data;
    struct class_filter *filter_handle = (struct class_filter *)class_handle;
    cmr_bzero(&start_time, sizeof(struct timespec));
    cmr_bzero(&end_time, sizeof(struct timespec));
    cmr_bzero(&pic_data, sizeof(struct filter_pic_data));

    pic_data.last_width = filter_handle->width;
    pic_data.last_height = filter_handle->height;
    pic_data.width = width;
    pic_data.height = height;
    pic_data.dst_addr = dst_addr;

    if (filter_handle->filter_ops && filter_handle->filter_ops->doeffect) {
        clock_gettime(CLOCK_BOOTTIME, &start_time);
        ret = filter_handle->filter_ops->doeffect(filter_handle, &pic_data);
        if (ret) {
            CMR_LOGE("failed to doeffect %ld", ret);
            goto exit;
        }
        clock_gettime(CLOCK_BOOTTIME, &end_time);
        duration = (end_time.tv_sec - start_time.tv_sec) * 1000 +
                   (end_time.tv_nsec - start_time.tv_nsec) / 1000000;
        CMR_LOGD("do effect time = %d", duration);
    } else {
        CMR_LOGE("filter_ops or doeffect is null");
        ret = CMR_CAMERA_FAIL;
    }

exit:
    return ret;
}
