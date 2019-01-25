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

#define LOG_TAG "cmr_sprd_filter"
#include "cmr_filter.h"
#include "cmr_common.h"
#include "imagefilter_interface.h"

static cmr_int sprd_filter_create_engine(cmr_handle *handle, cmr_uint type,
                                         cmr_uint width, cmr_uint height);
static cmr_int sprd_filter_destroy_engine(cmr_handle handle);
static cmr_int sprd_filter_doeffect(struct class_filter *filter_handle,
                                    struct filter_pic_data *pic_data);

struct filter_ops sprd_filter_ops = {
    sprd_filter_create_engine, sprd_filter_doeffect, sprd_filter_destroy_engine,
};

static cmr_int sprd_filter_create_engine(cmr_handle *handle, cmr_uint type,
                                         cmr_uint width, cmr_uint height) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    IFInitParam initPara;
    memset(&initPara, 0, sizeof(IFInitParam));
    initPara.imageType = NV21;
    initPara.imageWidth = width;
    initPara.imageHeight = height;
    initPara.paramPath = NULL;

    *handle = ImageFilterCreateEngine(&initPara);
    CMR_LOGD("X");
    return ret;
}

static cmr_int sprd_filter_destroy_engine(cmr_handle handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    if (!handle) {
        CMR_LOGE("handle is NULL");
        return CMR_CAMERA_INVALID_PARAM;
    }

    ret = ImageFilterDestroyEngine(handle);
    CMR_LOGD("res = %ld", ret);
    return ret;
}

static cmr_int sprd_filter_doeffect(struct class_filter *filter_handle,
                                    struct filter_pic_data *pic_data) {
    if (!filter_handle || !pic_data) {
        CMR_LOGE("para is null");
        return CMR_CAMERA_INVALID_PARAM;
    }

    CMR_LOGI("E");
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_uint type = filter_handle->filter_type;

    if (type != 0 && pic_data->width > 0 && pic_data->height > 0) {
        if ((pic_data->width != pic_data->last_width) ||
            (pic_data->height != pic_data->last_height)) {
            ret = sprd_filter_destroy_engine(filter_handle->handle);
            if (ret) {
                CMR_LOGE("sprd fail to destroy engine");
                goto exit;
            }
            ret = sprd_filter_create_engine(&(filter_handle->handle), type,
                                            pic_data->width, pic_data->height);
            if (ret) {
                CMR_LOGE("sprd fail to create engine");
                goto exit;
            }

            filter_handle->width = pic_data->width;
            filter_handle->height = pic_data->height;
        }

        IFImageData inputData, outputData;
        cmr_bzero(&inputData, sizeof(IFImageData));
        cmr_bzero(&outputData, sizeof(IFImageData));
        inputData.c1 = (void *)pic_data->dst_addr->addr_y;
        inputData.c2 = (void *)pic_data->dst_addr->addr_u;
        inputData.c3 = NULL;
        outputData.c1 = (void *)pic_data->dst_addr->addr_y;
        outputData.c2 = (void *)pic_data->dst_addr->addr_u;
        outputData.c3 = NULL;
        ret = ImageFilterRun(filter_handle->handle, &inputData, &outputData,
                             (IFFilterType)type, NULL);
        CMR_LOGI("ImageFilterRun res = %ld", ret);
        if (ret) {
            CMR_LOGE("failed to ImageFilterRun %ld", ret);
            goto exit;
        }
    }

    CMR_LOGI("X");
exit:
    return ret;
}
