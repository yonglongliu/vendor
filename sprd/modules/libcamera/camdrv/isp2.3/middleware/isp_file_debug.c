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

#define LOG_TAG "isp_file"

#include <cutils/properties.h>
#include "cmr_types.h"
#include "cmr_log.h"
#include "isp_type.h"
#include "isp_drv.h"

struct af_debug_info_t {
	FILE *dac_fp;
	FILE *stats_fp;
	cmr_s32 vcm_progressive_dac;
	cmr_s32 user_dac_irq_cnt;
	cmr_s32 user_stats_irq_cnt;
};

struct ae_debug_info_t {
	FILE *stats_fp;
	cmr_u32 grap_frame_cnt;
};

struct ebd_debug_info_t {
	FILE *stats_fp;
};

struct isp_file_context {
	struct ae_debug_info_t ae_debug_info;
	struct ebd_debug_info_t ebd_debug_info;
};

static cmr_s32 g_isp_open_cnt;

static FILE *ispfile_open_file(char* name, cmr_s32 open_cnt)
{
	FILE *fp = NULL;
	char file_name[100] = { 0 };

	sprintf(file_name, CAMERA_DATA_FILE"/%s_%d.txt", name, open_cnt);

	fp = fopen(file_name, "w+");
	if (NULL ==fp)
		ISP_LOGE("fail to open file");

	return fp;
}

static cmr_int ispfile_close_file(FILE *fp)
{
	fclose(fp);

	return 0;
}

static cmr_int ispfile_ae_init(cmr_handle handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_file_context *cxt = (struct isp_file_context *)handle;

	cxt->ae_debug_info.stats_fp = ispfile_open_file("aem_stats", g_isp_open_cnt);

	return ret;
}

static cmr_int ispfile_ae_deinit(cmr_handle handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_file_context *cxt = (struct isp_file_context *)handle;

	if (!cxt) {
		ISP_LOGE("fail to get cxt NUL");
		goto exit;
	}

	ispfile_close_file(cxt->ae_debug_info.stats_fp);

exit:
	return ret;
}

cmr_int isp_file_ae_save_stats(cmr_handle *handle,
			       cmr_u32 *r_info, cmr_u32 *g_info, cmr_u32 *b_info,
			       cmr_u32 size)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_file_context *cxt = (struct isp_file_context *)handle;
	cmr_u64 sum_r = 0;
	cmr_u64 sum_g = 0;
	cmr_u64 sum_b = 0;
	cmr_u32 i = 0;

	if (!cxt) {
		ISP_LOGV("fail to get cxt NUL");
		goto exit;
	}

	for (i = 0; i < size; i++) {
		sum_r += r_info[i];
		sum_g += g_info[i];
		sum_b += b_info[i];
	}

	if (cxt->ae_debug_info.grap_frame_cnt < 5) {
#if __WORDSIZE == 64
		fprintf(cxt->ae_debug_info.stats_fp,
			"R: 0x%016lx G: 0x%016lx B: 0x%016lx\n",
			sum_r, sum_g, sum_b);
#else
		fprintf(cxt->ae_debug_info.stats_fp,
			"R: 0x%016llx G: 0x%016llx B: 0x%016llx\n",
			sum_r, sum_g, sum_b);
#endif
	}

	cxt->ae_debug_info.grap_frame_cnt++;
exit:
	return ret;
}

static cmr_int ispfile_ebd_init(cmr_handle handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_file_context *cxt = (struct isp_file_context *)handle;

	cxt->ebd_debug_info.stats_fp =
		ispfile_open_file("embed_line_stats", g_isp_open_cnt);

	return ret;
}

static cmr_int ispfile_ebd_deinit(cmr_handle handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_file_context *cxt = (struct isp_file_context *)handle;

	if (!cxt) {
		ISP_LOGE("fail to get cxt NULL");
		goto exit;
	}

	ispfile_close_file(cxt->ebd_debug_info.stats_fp);

exit:
	return ret;
}

cmr_int isp_file_ebd_save_info(cmr_handle *handle, void *info)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_file_context *cxt = (struct isp_file_context *)handle;
	cmr_u32 i = 0;
	cmr_uint u_addr = 0;
	struct isp_statis_info *statis_info = (struct isp_statis_info *)info;

	u_addr = statis_info->vir_addr;
	cmr_u8 *emb = (cmr_u8 *)u_addr;

	if (!cxt) {
		ISP_LOGV("fail to get cxt NUL");
		goto exit;
	}
	fprintf(cxt->ebd_debug_info.stats_fp, "id: %04d 0x%04x ",
		statis_info->frame_id + 1,
		statis_info->frame_id + 1);

	for (i = 0; i < 20; i++) {
		fprintf(cxt->ebd_debug_info.stats_fp,
			"0x%02x ", emb[i]);
	}
	fprintf(cxt->ebd_debug_info.stats_fp, "\n");

exit:
	return ret;
}

cmr_int isp_file_init(cmr_handle *handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_file_context *cxt = NULL;
	char value[PROPERTY_VALUE_MAX] = { 0x00 };

	*handle = NULL;
	property_get("persist.sys.camera.ispfp.debug", value, "0");
	if (1 != atoi(value)) {
		goto exit;
	}

	cxt = (struct isp_file_context *)malloc(sizeof(struct isp_file_context));
	if (!cxt) {
		ISP_LOGE("fail to malloc");
		ret = ISP_ALLOC_ERROR;
		goto exit;
	}
	memset(cxt, 0, sizeof(*cxt));

	ispfile_ae_init(cxt);
	ispfile_ebd_init(cxt);
	ISP_LOGI("g_isp_open_cnt %d", g_isp_open_cnt);

	g_isp_open_cnt++;
	*handle = (cmr_handle)cxt;
exit:
	return ret;
}

cmr_int isp_file_deinit(cmr_handle handle)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct isp_file_context *cxt = (struct isp_file_context *)handle;

	if (!cxt) {
		ISP_LOGV("fail to get cxt pointer");
		goto exit;
	}

	ispfile_ae_deinit(cxt);
	ispfile_ebd_deinit(cxt);
	free((void *)cxt);
	cxt = NULL;

exit:
	ISP_LOGV("done %d", ret);
	return ret;
}
