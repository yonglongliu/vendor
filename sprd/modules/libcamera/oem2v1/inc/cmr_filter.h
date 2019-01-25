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
#ifndef _CMR_FILTER_H_
#define _CMR_FILTER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "cmr_common.h"
#include "cmr_oem.h"

struct class_filter {
    struct ipm_common common;
    cmr_uint width;
    cmr_uint height;
    cmr_uint is_inited;
    cmr_handle handle;
    struct img_addr dst_addr;
    ipm_callback reg_cb;
    struct ipm_frame_in frame_in;
    cmr_uint filter_type;
    struct filter_ops *filter_ops;
    sem_t sem;
    int vendor; // 0:sprd, 1:arcsoft;
};

struct filter_pic_data {
    cmr_uint width;
    cmr_uint height;
    cmr_uint last_width;
    cmr_uint last_height;
    struct img_addr *dst_addr;
};

struct filter_ops {
    cmr_int (*create)(cmr_handle *handle, cmr_uint type, cmr_uint width,
                      cmr_uint height);
    cmr_int (*doeffect)(struct class_filter *filter_handle,
                        struct filter_pic_data *pic_data);
    cmr_int (*destroy)(cmr_handle handle);
};

#ifdef __cplusplus
}
#endif

#endif
