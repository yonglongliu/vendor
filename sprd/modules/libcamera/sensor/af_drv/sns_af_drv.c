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
#define LOG_TAG "sns_af_drv"
#include "sns_af_drv.h"

int af_drv_create(struct af_drv_init_para *input_ptr,
                  cmr_handle *sns_af_drv_handle) {
    cmr_int ret = AF_SUCCESS;
    struct sns_af_drv_cxt *af_drv_cxt = NULL;
    CHECK_PTR(input_ptr);
    CMR_LOGV("in");
    *sns_af_drv_handle = NULL;
    af_drv_cxt = (struct sns_af_drv_cxt *)malloc(sizeof(struct sns_af_drv_cxt));
    if (!af_drv_cxt) {
        ret = AF_FAIL;
    } else {
        cmr_bzero(af_drv_cxt, sizeof(*af_drv_cxt));
        af_drv_cxt->hw_handle = input_ptr->hw_handle;
        *sns_af_drv_handle = (cmr_handle)af_drv_cxt;
    }
    CMR_LOGV("out");
    return ret;
}

int af_drv_delete(cmr_handle sns_af_drv_handle, void *param) {
    cmr_int ret = AF_SUCCESS;
    CMR_LOGV("in");
    if (sns_af_drv_handle) {
        free(sns_af_drv_handle);
    } else {
        CMR_LOGE("af_drv_handle is NULL,no need release.");
    }
    CMR_LOGV("out");
    return ret;
}
