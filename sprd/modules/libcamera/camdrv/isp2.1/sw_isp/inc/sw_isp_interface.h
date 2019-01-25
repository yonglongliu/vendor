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
#ifndef _SWISP_INTERFACE_H
#define _SWISP_INTERFACE_H
#include "sprd_realtimebokeh.h"
#ifdef __cplusplus
extern "C" {
#endif
void* init_sw_isp(struct soft_isp_init_param* initparam);
int sw_isp_process(void* handle , uint16_t *img_data, uint8_t* outputyuv , void**aem_addr , int32_t* aem_size);
int sw_isp_update_param(void* handle , struct soft_isp_block_param *isp_blockparam);
void deinit_sw_isp(void* handle);
void load_img(const char* filename , int width ,int height , uint16_t *buffer);
#ifdef __cplusplus
}
#endif

#endif

