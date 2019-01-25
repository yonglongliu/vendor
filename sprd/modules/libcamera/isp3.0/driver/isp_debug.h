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
#ifndef _ISP_DEBUG_H_
#define _ISP_DEBUG_H_

#include "sprd_isp_altek.h"
#include "cmr_types.h"

enum isp_data_type {
	ISP_DATA_TYPE_YUV422 = 0,
	ISP_DATA_TYPE_YUV420,
	ISP_DATA_TYPE_YVU420,
	ISP_DATA_TYPE_YUV420_3PLANE,
	ISP_DATA_TYPE_RAW,
	ISP_DATA_TYPE_RAW2,
	ISP_DATA_TYPE_RGB565,
	ISP_DATA_TYPE_RGB666,
	ISP_DATA_TYPE_RGB888,
	ISP_DATA_TYPE_JPEG,
	ISP_DATA_TYPE_YV12,
	ISP_DATA_TYPE_MAX
};

struct isp_img_addr {
	cmr_uint                        addr_y;
	cmr_uint                        addr_u;
	cmr_uint                        addr_v;
};

struct isp_img_info {
	cmr_u32                         width;
	cmr_u32                         height;
};

cmr_int isp_debug_save_to_file(struct isp_img_info *img_info, cmr_u32 img_fmt, struct isp_irq_info *irq_info);
cmr_int isp_debug_statistic_save_to_file(struct isp_img_info *img_info, struct isp_statis_frame_output *statis);

#endif /*_ISP_DEBUG_H_*/
