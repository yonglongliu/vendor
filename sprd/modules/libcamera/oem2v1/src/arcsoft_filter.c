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

#define LOG_TAG "cmr_arcsoft_filter"
#include "cmr_filter.h"
#include "cmr_common.h"
#include "amipengine.h"
#include "merror.h"

static cmr_int arcsoft_filter_create_engine(cmr_handle *handle, cmr_uint type,
                                            cmr_uint width, cmr_uint height);
static cmr_int arcsoft_filter_destroy_engine(cmr_handle handle);
static cmr_int arcsoft_filter_doeffect(struct class_filter *filter_handle,
                                       struct filter_pic_data *pic_data);

struct filter_ops arcsoft_filter_ops = {
    arcsoft_filter_create_engine, arcsoft_filter_doeffect,
    arcsoft_filter_destroy_engine,
};

static cmr_int arcsoft_filter_create_engine(cmr_handle *handle, cmr_uint type,
                                            cmr_uint width, cmr_uint height) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    CMR_LOGD("X");
    return ret;
}

static cmr_int arcsoft_filter_destroy_engine(cmr_handle handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    CMR_LOGD("X");
    return ret;
}

static cmr_int arcsoft_filter_doeffect(struct class_filter *filter_handle,
                                       struct filter_pic_data *pic_data) {
    if (!filter_handle || !pic_data) {
        CMR_LOGE("para is null");
        return CMR_CAMERA_INVALID_PARAM;
    }
    CMR_LOGI("E");

    MRESULT res = MOK;
    cmr_int mEffectId = 0;
    mEffectId = filter_handle->filter_type;
    CMR_LOGD("mEffectId = %ld", mEffectId);

    int pic_width = pic_data->width;
    int pic_height = pic_data->height;

    if (mEffectId != 0 && pic_width > 0 && pic_height > 0) {
        unsigned char *yBuf = (unsigned char *)(pic_data->dst_addr->addr_y);
        unsigned char *uvBuf = (unsigned char *)(pic_data->dst_addr->addr_u);
        CMR_LOGD("picselfie %dx%d yBuf is %p, uvBuf is %p", pic_width,
                 pic_height, yBuf, uvBuf);

        MHandle engine = MNull;
        ASVLOFFSCREEN srcImg;
        MPixelInfo pixelinfo;
        MEffectParam effectPara;
        effectPara.dwEffectID = mEffectId;
        srcImg.i32Width = pic_width;
        srcImg.i32Height = pic_height;
        srcImg.u32PixelArrayFormat = ASVL_PAF_NV21;
        srcImg.pi32Pitch[0] = pic_width;
        srcImg.pi32Pitch[1] = pic_width;
        srcImg.pi32Pitch[2] = pic_width;
        srcImg.ppu8Plane[0] = yBuf;
        srcImg.ppu8Plane[1] = uvBuf;
        srcImg.ppu8Plane[2] = uvBuf;

        pixelinfo.dwPixelArrayFormat = srcImg.u32PixelArrayFormat;
        pixelinfo.lWidth = srcImg.i32Width;
        pixelinfo.lHeight = srcImg.i32Height;

        res = MIPCreateImageEngine(&effectPara, &pixelinfo, &engine);
        CMR_LOGD("MIPCreateImageEngine res = %ld", res);
        if (res != MOK) {
            CMR_LOGE("arcsoft fail to MIPCreateImageEngine");
            goto exit;
        }
        res = MIPDoEffect(engine, &srcImg);
        if (res != MOK) {
            CMR_LOGE("arcsoft fail to MIPDoEffect");
            goto exit;
        }
        CMR_LOGI("MIPDoEffect res = %ld", res);

        if (engine) {
            res = MIPDestroyImageEngine(engine);
            CMR_LOGD("MIPDestroyImageEngine res = %ld", res);
            if (res != MOK) {
                CMR_LOGE("arcsoft fail to MIPDestroyImageEngine");
                goto exit;
            }
        }
    }

    CMR_LOGI("X");
exit:
    return res;
}
