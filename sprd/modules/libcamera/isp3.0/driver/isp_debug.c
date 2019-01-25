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

#define LOG_TAG "isp_debug"

#include <stdio.h>
#include <string.h>
#include "isp_common_types.h"
#include "isp_debug.h"

cmr_int isp_debug_save_to_file(struct isp_img_info *img_info, cmr_u32 img_fmt, struct isp_irq_info *irq_info)
{
	cmr_int                      ret = 0;
	char                         file_name[40];
	struct isp_img_addr          *addr = NULL;
	cmr_u32                      index;
	FILE                         *fp = NULL;

	if (!img_info || !irq_info) {
		ret = -1;
		ISP_LOGE("get parameter is NULL.");
		goto exit;
	}

	addr = malloc(sizeof(struct isp_img_addr));
	if (!addr) {
		ret = -1;
		ISP_LOGE("alloc addr error.");
		goto exit;
	}

	index = irq_info->frm_index;

	addr->addr_y = irq_info->yaddr_vir;
	addr->addr_u = irq_info->uaddr_vir;
	addr->addr_v = irq_info->vaddr_vir;

	ISP_LOGI("index %d format %d width %d heght %d u_addr 0x%lx",
		 index, img_fmt, img_info->width, img_info->height, addr->addr_u);

	cmr_bzero(file_name, sizeof(file_name));
	snprintf(file_name, sizeof(file_name), "/data/misc/media/%d_%dX%d",
		 index, img_info->width, img_info->height);

	if (ISP_DATA_TYPE_YUV420 == img_fmt ||
	    ISP_DATA_TYPE_YUV422 == img_fmt) {
		strcat(file_name, "_y.raw");
		ISP_LOGI("file name %s", file_name);
		fp = fopen(file_name, "wb");
		if (NULL == fp) {
			ISP_LOGE("can not open file: %s", file_name);
			ret = -1;
			goto exit;
		}

		fwrite((void *)addr->addr_y, 1, img_info->width * img_info->height, fp);

		cmr_bzero(file_name, sizeof(file_name));
		snprintf(file_name, sizeof(file_name), "/data/misc/media/%d_%dX%d_uv.raw",
			 index, img_info->width, img_info->height);
		ISP_LOGI("file name %s", file_name);
		fp = fopen(file_name, "wb");
		if (NULL == fp) {
			ISP_LOGE("can not open file: %s", file_name);
			ret = -1;
			goto exit;
		}
		fwrite((void *)(addr->addr_y + img_info->width * img_info->height), 1,
		       img_info->width * img_info->height / 2, fp);
	} else if (ISP_DATA_TYPE_JPEG == img_fmt) {
		strcat(file_name, ".jpg");
		ISP_LOGI("file name %s", file_name);

		fp = fopen(file_name, "wb");
		if (NULL == fp) {
			ISP_LOGE("can not open file: %s", file_name);
			ret = -1;
			goto exit;
		}
		fwrite((void *)addr->addr_y, 1, img_info->width * img_info->height * 2, fp);
	} else if (ISP_DATA_TYPE_RAW == img_fmt) {
		strcat(file_name, "_mipi.raw");
		ISP_LOGI("file name %s", file_name);

		fp = fopen(file_name, "wb");
		if (NULL == fp) {
			ISP_LOGE("can not open file: %s", file_name);
			ret = -1;
			goto exit;
		}
		fwrite((void *)addr->addr_y, 1, (uint32_t)(img_info->width * img_info->height * 5 / 4), fp);
	} else if (ISP_DATA_TYPE_RAW2 == img_fmt) {
		strcat(file_name, "_mipi2.raw");
		ISP_LOGI("file name %s", file_name);

		fp = fopen(file_name, "wb");
		if (NULL == fp) {
			ISP_LOGE("can not open file: %s", file_name);
			ret = -1;
			goto exit;
		}

		fwrite((void *)addr->addr_y, 1,
		       (uint32_t)(((img_info->width * 4 / 3 + 7) >> 3) << 3) * img_info->height, fp);
	}
	fclose(fp);
exit:
	if (addr)
		free(addr);

	return ret;
}

cmr_int isp_debug_statistic_save_to_file(struct isp_img_info *img_info, struct isp_statis_frame_output *statis)
{
	cmr_int                      ret = 0;
	char                         file_name[40];
	FILE                         *fp = NULL;

	if (!img_info || !statis) {
		ret = -1;
		ISP_LOGE("get parameter is NULL.");
		goto exit;
	}

	cmr_bzero(file_name, sizeof(file_name));
	snprintf(file_name, sizeof(file_name), "/data/misc/media/statisic_%dX%d.log",
			img_info->width, img_info->height);

	ISP_LOGI("file name %s", file_name);
	fp = fopen(file_name, "a+");

	if (NULL == fp) {
		ISP_LOGI("can not open file: %s", file_name);
		ret = -1;
		goto exit;
	}

	fseek(fp, 0, SEEK_END);
	fwrite((void *)(statis->vir_addr), 1, statis->buf_size, fp);
	fclose(fp);

exit:
	return ret;
}
