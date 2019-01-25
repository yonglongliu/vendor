/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "isp_pm"

#ifdef WIN32
#include <malloc.h>
#include <memory.h>
#include <string.h>
#endif
#include "ae_tuning_type.h"
#include "isp_pm.h"
#include "isp_blocks_cfg.h"
#include "isp_pm_com_type.h"
#include "isp_drv.h"
#include "cmr_types.h"

#define ISP_PM_BUF_NUM     10
#define ISP_PM_MAGIC_FLAG  0xFFEE5511

char nr_param_name[ISP_BLK_TYPE_MAX][32] = {
	"pdaf_correction",
	"bayer_nr",
	"vst",
	"ivst",
	"rgb_dither",
	"bpc",
	"grgb",
	"cfai",
	"rgb_afm",
	"cce_uvdiv",
	"pre_3dnr",
	"cap_3dnr",
	"yuv_precdn",
	"uv_cdn",
	"uv_postcdn",
	"ynr",
	"ee",
	"iircnr",
	"yuv_noisefilter",
};

char nr_mode_name[MAX_MODE_NUM][8] = {
	"common",
	"prv_0",
	"prv_1",
	"prv_2",
	"prv_3",
	"cap_0",
	"cap_1",
	"cap_2",
	"cap_3",
	"video_0",
	"video_1",
	"video_2",
	"video_3",
	"main",
};

char nr_scene_name[MAX_SCENEMODE_NUM][16] = {
	"normal",
	"night",
	"sport",
	"portrait",
	"landscape",
	"panorama",
};

struct isp_pm_buffer {
	cmr_u32 count;
	struct isp_data_info buf_array[ISP_PM_BUF_NUM];
};

struct isp_pm_blk_status {
	cmr_u32 update_flag;	/*0: not update 1: update */
	cmr_u32 block_id;	/*need update block_id */
};

struct isp_pm_mode_update_status {
	cmr_u32 blk_num;	/*need update total numbers */
	struct isp_pm_blk_status pm_blk_status[ISP_TUNE_BLOCK_MAX];
};

struct isp_pm_tune_merge_param {
	cmr_u32 mode_num;
	struct isp_pm_mode_param **tune_merge_para;
};

struct isp_pm_context {
	cmr_u32 magic_flag;
	pthread_mutex_t pm_mutex;
	cmr_u32 mode_num;
	cmr_u32 cxt_num;
	struct isp_pm_buffer buffer;
	struct isp_pm_param_data temp_param_data[ISP_TUNE_BLOCK_MAX];
	struct isp_pm_param_data *tmp_param_data_ptr;
	struct isp_context *active_cxt_ptr;
	struct isp_context cxt_array[ISP_TUNE_MODE_MAX];	/*preview,capture,video and so on */
	struct isp_pm_mode_param *active_mode;
	struct isp_pm_mode_param *merged_mode_array[ISP_TUNE_MODE_MAX];	/*new preview/capture/video mode param */
	struct isp_pm_mode_param *tune_mode_array[ISP_TUNE_MODE_MAX];	/*bakup isp tuning parameter, come frome sensor tuning file */
	cmr_u32 param_source;
	cmr_u32 cur_mode_id;
};

struct isp_pm_set_param {
	cmr_u32 id;
	cmr_u32 cmd;
	void *data;
	cmr_u32 size;
};

struct isp_pm_get_param {
	cmr_u32 id;
	cmr_u32 cmd;
};

struct isp_pm_get_result {
	void *data_ptr;
	cmr_u32 size;
};

struct isp_pm_write_param {
	cmr_u32 id;
	void *data;
	cmr_u32 size;
};

static cmr_s32 isp_pm_handle_check(cmr_handle handle)
{
	struct isp_pm_context *cxt_ptr = (struct isp_pm_context *)handle;

	if (PNULL == cxt_ptr) {
		ISP_LOGE("fail to get valid isp pm handle");
		return ISP_ERROR;
	}

	if (ISP_PM_MAGIC_FLAG != cxt_ptr->magic_flag) {
		ISP_LOGE("fail to get valid isp pm magic flag");
		return ISP_ERROR;
	}

	return ISP_SUCCESS;
}

static cmr_handle isp_pm_context_create(void)
{
	struct isp_pm_context *cxt_ptr = PNULL;

	cxt_ptr = (struct isp_pm_context *)malloc(sizeof(struct isp_pm_context));
	if (PNULL == cxt_ptr) {
		ISP_LOGE("fail to malloc");
		return cxt_ptr;
	}

	memset((void *)cxt_ptr, 0x00, sizeof(struct isp_pm_context));

	cxt_ptr->magic_flag = ISP_PM_MAGIC_FLAG;

	pthread_mutex_init(&cxt_ptr->pm_mutex, NULL);

	return (cmr_handle) cxt_ptr;
}

static void isp_pm_check_param(cmr_u32 id, cmr_u32 * update_flag)
{
	switch (id) {
	case ISP_BLK_PDAF_CORRECT:
	case ISP_BLK_NLM:
	case ISP_BLK_RGB_DITHER:
	case ISP_BLK_BPC:
	case ISP_BLK_GRGB:
	case ISP_BLK_CFA:
	case ISP_BLK_RGB_AFM:
	case ISP_BLK_UVDIV:
	case ISP_BLK_3DNR_PRE:
	case ISP_BLK_3DNR_CAP:
	case ISP_BLK_YUV_PRECDN:
	case ISP_BLK_UV_CDN:
	case ISP_BLK_UV_POSTCDN:
	case ISP_BLK_YNR:
	case ISP_BLK_EDGE:
	case ISP_BLK_IIRCNR_IIR:
	case ISP_BLK_YUV_NOISEFILTER:

	case ISP_BLK_BINNING4AWB:
		{
			*update_flag = ISP_PARAM_FROM_TOOL;
			break;
		}
	default:
		break;
	}
}

static cmr_s32 isp_pm_context_init(cmr_handle handle)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 i = 0, blk_num = 0, id = 0, offset = 0;
	cmr_u32 update_flag = 0;
	void *blk_ptr = PNULL;
	void *param_data_ptr = PNULL;
	intptr_t isp_cxt_start_addr = 0;
	struct isp_block_operations *ops = PNULL;
	struct isp_block_cfg *blk_cfg_ptr = PNULL;
	struct isp_context *isp_cxt_ptr = PNULL;
	struct isp_pm_context *pm_cxt_ptr = PNULL;
	struct isp_pm_block_header *blk_header_array = PNULL;
	struct isp_pm_block_header *blk_header_ptr = PNULL;
	struct isp_pm_mode_param *mode_param_ptr = PNULL;

	pm_cxt_ptr = (struct isp_pm_context *)handle;
	isp_cxt_ptr = (struct isp_context *)pm_cxt_ptr->active_cxt_ptr;
	mode_param_ptr = (struct isp_pm_mode_param *)pm_cxt_ptr->active_mode;

	if (isp_cxt_ptr == PNULL ||  mode_param_ptr == PNULL) {
		ISP_LOGE("Invalid pm context ptr.");
		return ISP_PARAM_NULL;
	}

	blk_header_array = (struct isp_pm_block_header *)mode_param_ptr->header;

	blk_num = mode_param_ptr->block_num;
	for (i = 0; i < blk_num; i++) {
		id = blk_header_array[i].block_id;
		blk_cfg_ptr = isp_pm_get_block_cfg(id);
		blk_header_ptr = &blk_header_array[i];
		if (pm_cxt_ptr->param_source) {
			isp_pm_check_param(id, &update_flag);
			if (0 != update_flag) {
				update_flag = 0;
				continue;
			}
		}
		if ((PNULL != blk_cfg_ptr) && (PNULL != blk_header_ptr)) {
			if (blk_cfg_ptr->ops) {
				ops = blk_cfg_ptr->ops;
				if (ops->init) {
					if (blk_header_ptr->size > 0) {
						isp_cxt_start_addr = (intptr_t) isp_cxt_ptr;
						offset = blk_cfg_ptr->offset;
						blk_ptr = (void *)(isp_cxt_start_addr + offset);
						param_data_ptr = (void *)blk_header_ptr->absolute_addr;
						if ((PNULL == blk_ptr) || (PNULL == param_data_ptr)
						    || (PNULL == blk_header_ptr)) {
							ISP_LOGE("fail to get valid param : blk_addr:%p, param:%p, header:%p, resolution:%p",
								 blk_ptr, param_data_ptr, blk_header_ptr, &mode_param_ptr->resolution);
							rtn = ISP_ERROR;
						} else {
							ops->init(blk_ptr, param_data_ptr, blk_header_ptr, &mode_param_ptr->resolution);
						}
					} else {
						ISP_LOGW("param size is warning: size:%d, id:0x%x, i:%d\n", blk_header_ptr->size, id, i);
					}
				}
			}
		}
	}

	isp_cxt_ptr->is_validate = ISP_ONE;

	return rtn;
}

static cmr_s32 isp_pm_context_update(cmr_handle handle, struct isp_pm_mode_param *org_mode_param)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 i = 0, j = 0, blk_num = 0, id = 0, offset = 0;
	void *blk_ptr = PNULL;
	void *param_data_ptr = PNULL;
	intptr_t isp_cxt_start_addr = 0;
	struct isp_block_operations *ops = PNULL;
	struct isp_block_cfg *blk_cfg_ptr = PNULL;
	struct isp_context *isp_cxt_ptr = PNULL;
	struct isp_pm_context *pm_cxt_ptr = PNULL;
	struct isp_pm_block_header *blk_header_array = PNULL;
	struct isp_pm_block_header *blk_header_ptr = PNULL;
	struct isp_pm_mode_param *mode_param_ptr = PNULL;

	pm_cxt_ptr = (struct isp_pm_context *)handle;
	isp_cxt_ptr = (struct isp_context *)pm_cxt_ptr->active_cxt_ptr;
	mode_param_ptr = (struct isp_pm_mode_param *)pm_cxt_ptr->active_mode;
	blk_header_array = (struct isp_pm_block_header *)mode_param_ptr->header;

	blk_num = mode_param_ptr->block_num;
	for (i = 0; i < blk_num; i++) {
		id = blk_header_array[i].block_id;
		blk_cfg_ptr = isp_pm_get_block_cfg(id);
		blk_header_ptr = &blk_header_array[i];
		if ((PNULL != blk_cfg_ptr) && (PNULL != blk_header_ptr)) {
			if (blk_cfg_ptr->ops) {
				ops = blk_cfg_ptr->ops;
				if (ops->init) {
					if (blk_header_ptr->size > 0) {
						isp_cxt_start_addr = (intptr_t) isp_cxt_ptr;
						offset = blk_cfg_ptr->offset;
						blk_ptr = (void *)(isp_cxt_start_addr + offset);
						param_data_ptr = (void *)blk_header_ptr->absolute_addr;
						if ((PNULL == blk_ptr) || (PNULL == param_data_ptr)
						    || (PNULL == blk_header_ptr)) {
							ISP_LOGE("fail to get valid param: blk_addr:%p, param:%p, header:%p, resolution:%p",
								 blk_ptr, param_data_ptr, blk_header_ptr, &mode_param_ptr->resolution);
							rtn = ISP_ERROR;
						} else {
							for (j = 0; j < org_mode_param->block_num; j++) {
								if (org_mode_param->header[j].block_id == blk_header_ptr->block_id) {
									break;
								}
							}
							if (j < org_mode_param->block_num) {
								if (blk_header_ptr->source_flag != org_mode_param->header[j].source_flag) {
									ops->init(blk_ptr, param_data_ptr, blk_header_ptr, &mode_param_ptr->resolution);
								}
							} else {
								ops->init(blk_ptr, param_data_ptr, blk_header_ptr, &mode_param_ptr->resolution);
							}
						}
					} else {
						ISP_LOGW("param size is warning: size:%d, id:0x%x, i:%d\n", blk_header_ptr->size, id, i);
					}
				}
			}
		}
	}

	isp_cxt_ptr->is_validate = ISP_ONE;

	return rtn;
}

static cmr_s32 isp_pm_context_deinit(cmr_handle handle)
{
	cmr_s32 rtn = ISP_SUCCESS;
	void *blk_ptr = PNULL;
	void *param_data_ptr = PNULL;
	cmr_u32 i = 0, j = 0, offset = 0;
	cmr_u32 blk_num = 0, id = 0;
	intptr_t isp_cxt_start_addr = 0;
	struct isp_block_operations *ops = PNULL;
	struct isp_block_cfg *blk_cfg_ptr = PNULL;
	struct isp_context *isp_cxt_ptr = PNULL;
	struct isp_pm_context *pm_cxt_ptr = PNULL;
	struct isp_pm_block_header *blk_header_array = PNULL;
	struct isp_pm_block_header *blk_header_ptr = PNULL;
	struct isp_pm_mode_param *mode_param_ptr = PNULL;

	pm_cxt_ptr = (struct isp_pm_context *)handle;

	isp_cxt_ptr = (struct isp_context *)pm_cxt_ptr->active_cxt_ptr;
	isp_cxt_start_addr = (intptr_t) isp_cxt_ptr;
	mode_param_ptr = (struct isp_pm_mode_param *)pm_cxt_ptr->active_mode;
	blk_header_array = (struct isp_pm_block_header *)mode_param_ptr->header;
	blk_num = mode_param_ptr->block_num;
	for (i = 0; i < blk_num; i++) {
		id = blk_header_array[i].block_id;
		blk_cfg_ptr = isp_pm_get_block_cfg(id);
		blk_header_ptr = &blk_header_array[i];
		if ((PNULL != blk_cfg_ptr) && (PNULL != blk_header_ptr)) {
			if (blk_cfg_ptr->ops) {
				ops = blk_cfg_ptr->ops;
				if (ops->deinit) {
					offset = blk_cfg_ptr->offset;
					blk_ptr = (void *)(isp_cxt_start_addr + offset);
					if (PNULL != blk_ptr) {
						ops->deinit(blk_ptr);
					} else {
						ISP_LOGE("fail to get valid blk_addr");
					}
				}
			}
		}
	}

	for (j = 0; j < ISP_TUNE_MODE_MAX; j++) {
		if (PNULL == pm_cxt_ptr->tune_mode_array[j]) {
			continue;
		}
		mode_param_ptr = (struct isp_pm_mode_param *)pm_cxt_ptr->tune_mode_array[j];
		blk_header_array = (struct isp_pm_block_header *)mode_param_ptr->header;
		isp_cxt_ptr = (struct isp_context *)&pm_cxt_ptr->cxt_array[j];
		isp_cxt_start_addr = (intptr_t) isp_cxt_ptr;
		blk_num = mode_param_ptr->block_num;
		for (i = 0; i < blk_num; i++) {
			id = blk_header_array[i].block_id;
			blk_cfg_ptr = isp_pm_get_block_cfg(id);
			blk_header_ptr = &blk_header_array[i];
			if ((PNULL != blk_cfg_ptr) && (PNULL != blk_header_ptr)) {
				if (blk_cfg_ptr->ops) {
					ops = blk_cfg_ptr->ops;
					if (ops->deinit) {
						offset = blk_cfg_ptr->offset;
						blk_ptr = (void *)(isp_cxt_start_addr + offset);
						if (PNULL != blk_ptr) {
							ops->deinit(blk_ptr);
						} else {
							ISP_LOGV("blk_addr is null erro");
						}
					}
				}
			}
		}
	}

	return rtn;
}

static struct isp_pm_block_header *isp_pm_get_block_header(struct isp_pm_mode_param *pm_mode_ptr, cmr_u32 id, cmr_s32 * index)
{
	cmr_u32 i = 0, blk_num = 0;
	struct isp_pm_block_header *header_ptr = PNULL;
	struct isp_pm_block_header *blk_header = (struct isp_pm_block_header *)pm_mode_ptr->header;

	blk_num = pm_mode_ptr->block_num;

	*index = ISP_TUNE_BLOCK_MAX;

	for (i = 0; i < blk_num; i++) {
		if (id == blk_header[i].block_id) {
			break;
		}
	}

	if (i < blk_num) {
		header_ptr = (struct isp_pm_block_header *)&blk_header[i];
		*index = i;
	}

	return header_ptr;
}

static struct isp_pm_mode_param *isp_pm_get_mode(struct isp_pm_tune_merge_param tune_merge_para, cmr_u32 id)
{
	cmr_u32 i = 0, mode_num = 0;
	struct isp_pm_mode_param *mode_param_ptr = PNULL;

	mode_num = tune_merge_para.mode_num;
	for (i = 0; i < mode_num; i++) {
		if (PNULL == tune_merge_para.tune_merge_para[i]) {
			continue;
		}
		if (id == tune_merge_para.tune_merge_para[i]->mode_id) {
			break;
		}
	}

	if (i < mode_num) {
		mode_param_ptr = (struct isp_pm_mode_param *)(tune_merge_para.tune_merge_para[i]);
	}

	return mode_param_ptr;
}

static struct isp_context *isp_pm_get_context(struct isp_pm_context *pm_cxt_ptr, cmr_u32 id)
{
	cmr_u32 i = 0, num = 0;
	struct isp_context *isp_cxt_ptr = PNULL;

#if 0
	num = pm_cxt_ptr->cxt_num;

	for (i = 0; i < num; i++) {
		if (id == pm_cxt_ptr->cxt_array[i].mode_id) {
			break;
		}
	}

	if (i < num) {
		isp_cxt_ptr = (struct isp_context *)&pm_cxt_ptr->cxt_array[i];
	}
#else
	isp_cxt_ptr = (struct isp_context *)&pm_cxt_ptr->cxt_array[ISP_MODE_ID_PRV_0];
	isp_cxt_ptr->mode_id = id;

#endif
	return isp_cxt_ptr;
}

static cmr_s32 isp_pm_active_mode_init(cmr_handle handle, cmr_u32 mode_id)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pm_context *pm_cxt_ptr = (struct isp_pm_context *)handle;
	struct isp_pm_tune_merge_param merged_mode_param;

	memset((void *)&merged_mode_param, 0x00, sizeof(struct isp_pm_tune_merge_param));
	merged_mode_param.tune_merge_para = pm_cxt_ptr->merged_mode_array;
	merged_mode_param.mode_num = pm_cxt_ptr->mode_num;

	pm_cxt_ptr->active_mode = isp_pm_get_mode(merged_mode_param, mode_id);
	if (PNULL == pm_cxt_ptr->active_mode) {
		ISP_LOGE("fail to get active mode");
		rtn = ISP_ERROR;
		return rtn;
	}

	pm_cxt_ptr->active_cxt_ptr = isp_pm_get_context(pm_cxt_ptr, mode_id);
	if (PNULL == pm_cxt_ptr->active_cxt_ptr) {
		ISP_LOGE("fail to get active cxt ");
		rtn = ISP_ERROR;
		return rtn;
	}

	return rtn;
}

static void isp_pm_mode_list_deinit(cmr_handle handle)
{
	cmr_u32 i = 0;
	struct isp_pm_context *cxt_ptr = (struct isp_pm_context *)handle;

	for (i = 0; i < ISP_TUNE_MODE_MAX; i++) {
		if (cxt_ptr->tune_mode_array[i]) {
			free(cxt_ptr->tune_mode_array[i]);
			cxt_ptr->tune_mode_array[i] = PNULL;
		}
		if (cxt_ptr->merged_mode_array[i]) {
			free(cxt_ptr->merged_mode_array[i]);
			cxt_ptr->merged_mode_array[i] = PNULL;
		}
	}

	if (PNULL != cxt_ptr->tmp_param_data_ptr) {
		free(cxt_ptr->tmp_param_data_ptr);
		cxt_ptr->tmp_param_data_ptr = PNULL;
	}
}

static cmr_u32 isp_calc_nr_addr_offset(cmr_u32 mode_flag,
		cmr_u32 * one_multi_mode_ptr,
		cmr_u32 * basic_offset_units_ptr)
{

	cmr_u32 rtn = ISP_SUCCESS;

	cmr_u32 basic_offset_units = 0;
	cmr_u32 i = 0, j = 0;

	if (mode_flag > (MAX_MODE_NUM - 1))
		return ISP_ERROR;
	if (!one_multi_mode_ptr)
		return ISP_ERROR;
	for (i = 0; i < mode_flag; i++) {
		ISP_LOGV("nr_mode_flag[%d]=%x\n",i,one_multi_mode_ptr[i]);
		if (one_multi_mode_ptr[i]) {
			for (j = 0; j < MAX_SCENEMODE_NUM; j++) {
				basic_offset_units += (one_multi_mode_ptr[i] >> j) & 0x01;
			}
		}
	}
	*basic_offset_units_ptr = basic_offset_units;

	return rtn;
}

struct isp_nr_param_update_info {
	char *sensor_name;
	cmr_u32 sensor_mode;
	cmr_u32 param_type;
	cmr_uint *nr_param_ptr;
	cmr_u32 size_of_per_unit;
	cmr_uint *multi_nr_scene_map_ptr;
};
static cmr_s32 isp_nr_param_update(struct isp_nr_param_update_info *nr_param_update_ptr)
{
	cmr_u32 rtn = ISP_SUCCESS;
	cmr_s32 ret;
	cmr_u32 basic_offset_units = 0;
	cmr_u32 *one_multi_mode_ptr = PNULL;
	cmr_u8 *nr_param_ptr = PNULL;
	cmr_u8 *nr_param_start_ptr = PNULL;
	cmr_u32 size_of_per_unit = 0;

	cmr_u32 scene_number = 0;
	cmr_u32 sensor_mode = 0;
	char *sensor_name = PNULL;
	cmr_u32 param_type = 0;
	FILE *fp = NULL;
	char filename[80];
	if (nr_param_update_ptr->sensor_mode > (MAX_MODE_NUM - 1)) {
		ISP_LOGE("Incorrect nr sensor mode: %d", nr_param_update_ptr->sensor_mode);
		return ISP_ERROR;
	}
	if (nr_param_update_ptr->param_type > (ISP_BLK_TYPE_MAX - 1)) {
		ISP_LOGE("Incorrect nr param_type: %d", nr_param_update_ptr->param_type);
		return ISP_ERROR;
	}
	if (PNULL == nr_param_update_ptr->nr_param_ptr) {
		ISP_LOGE("fail to get valid nr param address ,type(%d)!", nr_param_update_ptr->param_type);
		return ISP_ERROR;
	}
	one_multi_mode_ptr = (cmr_u32 *) nr_param_update_ptr->multi_nr_scene_map_ptr;
	nr_param_ptr = (cmr_u8 *) nr_param_update_ptr->nr_param_ptr;
	size_of_per_unit = nr_param_update_ptr->size_of_per_unit;
	param_type = nr_param_update_ptr->param_type;
	sensor_mode = nr_param_update_ptr->sensor_mode;
	sensor_name = nr_param_update_ptr->sensor_name;

	rtn = isp_calc_nr_addr_offset(sensor_mode, one_multi_mode_ptr, &basic_offset_units);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("isp_calc_nr_addr_offset failed, error: %d", rtn);
		return ISP_ERROR;
	}

	nr_param_ptr += basic_offset_units * size_of_per_unit;

	/* calc current block */
	if (one_multi_mode_ptr[sensor_mode]) {
		for (scene_number = 0; scene_number < MAX_SCENEMODE_NUM; scene_number++) {

			if ((one_multi_mode_ptr[sensor_mode] >> scene_number) & 0x01) {
				sprintf(filename, "%s%s_%s_%s_%s_param.bin", CAMERA_DUMP_PATH,
					sensor_name, nr_mode_name[sensor_mode],
					nr_scene_name[scene_number], nr_param_name[param_type]);
				if (0 != access(filename, R_OK)) {
					ISP_LOGV("param access %s not exist", filename);
				} else {
					if (NULL != (fp = fopen(filename, "rb"))) {
						ISP_LOGV("param open %s, succeed!", filename);
						ret = fread((void *)nr_param_ptr, 1, size_of_per_unit, fp);
						if (ret < 0) {
							ISP_LOGE("fail to read .");
							fclose(fp);
							rtn = ret;
							return rtn;
						}
						fclose(fp);
					} else {
						ISP_LOGV("param open %s, not succeed!",filename);
					}
				}
				nr_param_ptr += size_of_per_unit;
			}
		}
	}
	return rtn;
}

static cmr_s32 isp_pm_mode_list_init(cmr_handle handle,
				     struct isp_pm_init_input *input,
				     struct isp_pm_init_output *output)
{
	cmr_u32 rtn = ISP_SUCCESS;

	cmr_u32 max_num = 0;
	cmr_u32 data_area_size = 0;
	cmr_u32 i = 0, j = 0, size = 0, offset = 0;
	cmr_u32 k = 0;
	cmr_u32 add_ae_len = 0, add_awb_len = 0, add_lnc_len = 0;
	cmr_u32 extend_offset = 0;
	struct sensor_raw_fix_info *fix_data_ptr = PNULL;
	struct sensor_nr_fix_info *nr_fix_ptr = PNULL;
	struct sensor_nr_scene_map_param *nr_scene_map_ptr = PNULL;
	struct sensor_nr_level_map_param *nr_level_number_ptr = PNULL;
	struct sensor_nr_level_map_param *nr_default_level_ptr = PNULL;

	cmr_u8 *src_data_ptr = PNULL;
	cmr_u8 *dst_data_ptr = PNULL;
	struct isp_mode_param *src_mod_ptr = PNULL;
	struct isp_pm_mode_param *dst_mod_ptr = PNULL;
	struct isp_block_header *src_header = PNULL;
	struct isp_pm_block_header *dst_header = PNULL;
	struct isp_pm_context *cxt_ptr = (struct isp_pm_context *)handle;
	struct isp_nr_param_update_info nr_param_update_info;
	cmr_u32 isp_blk_nr_type = ISP_BLK_TYPE_MAX;
	intptr_t nr_set_addr = 0;
	cmr_u32 nr_set_size = 0;
	struct nr_set_group_unit *nr_ptr = PNULL;
	cmr_u32 multi_nr_flag = 0;

	cxt_ptr->mode_num = input->num;

	nr_fix_ptr = input->nr_fix_info;
	if (PNULL != nr_fix_ptr) {
		multi_nr_flag = SENSOR_MULTI_MODE_FLAG;
	} else {
		multi_nr_flag = SENSOR_DEFAULT_MODE_FLAG;
	}

	if (output)
		output->multi_nr_flag = multi_nr_flag;

	if (strlen((char *)input->sensor_name)) {
		nr_param_update_info.sensor_name = (char *)input->sensor_name;
	} else {
		ISP_LOGE("fail to get  sensor name from Tune file!");
	}
	nr_scene_map_ptr = (struct sensor_nr_scene_map_param *)(nr_fix_ptr->nr_scene_ptr);
	nr_level_number_ptr = (struct sensor_nr_level_map_param *)(nr_fix_ptr->nr_level_number_ptr);
	nr_default_level_ptr = (struct sensor_nr_level_map_param *)(nr_fix_ptr->nr_default_level_ptr);

	if (nr_scene_map_ptr == PNULL || nr_level_number_ptr == PNULL
		|| nr_default_level_ptr == PNULL) {
		ISP_LOGE("NR map info is null!\n");
		rtn = ISP_ERROR;
		goto _mode_list_init_error_exit;
	}

	nr_param_update_info.multi_nr_scene_map_ptr = (cmr_uint *) & (nr_scene_map_ptr->nr_scene_map[0]);

	for (i = 0; i < input->num; i++) {
		extend_offset = 0;
		nr_param_update_info.sensor_mode = i;
		src_mod_ptr = (struct isp_mode_param *)input->tuning_data[i].data_ptr;
		if (PNULL == src_mod_ptr)
			continue;
		fix_data_ptr = input->fix_data[i];
		if (PNULL == fix_data_ptr) {
			ISP_LOGE("fix_data_info ptr for mode %d is null!\n", i);
			rtn = ISP_ERROR;
			goto _mode_list_init_error_exit;
		}

		data_area_size = src_mod_ptr->size - sizeof(struct isp_mode_param);
		size = data_area_size + sizeof(struct isp_pm_mode_param);

		add_ae_len = fix_data_ptr->ae.ae_param.ae_len;
		add_lnc_len = fix_data_ptr->lnc.lnc_param.lnc_len;
		add_awb_len = fix_data_ptr->awb.awb_param.awb_len;
		size += add_ae_len + add_lnc_len + add_awb_len;

		nr_ptr = (struct nr_set_group_unit *)&(fix_data_ptr->nr.nr_set_group);

		for (k = 0; k < sizeof(struct sensor_nr_set_group_param) / sizeof(struct nr_set_group_unit); k++) {
			if (PNULL != nr_ptr[k].nr_ptr) {
				size += sizeof(struct sensor_nr_simple_header_param);
			}
		}

		if (PNULL != cxt_ptr->tune_mode_array[i]) {
			free(cxt_ptr->tune_mode_array[i]);
			cxt_ptr->tune_mode_array[i] = PNULL;
		}
		cxt_ptr->tune_mode_array[i] = (struct isp_pm_mode_param *)malloc(size);
		if (PNULL == cxt_ptr->tune_mode_array[i]) {
			ISP_LOGE("fail to malloc, i=%d", i);
			rtn = ISP_ERROR;
			goto _mode_list_init_error_exit;
		}
		memset((void *)cxt_ptr->tune_mode_array[i], 0x00, size);

		size = sizeof(src_mod_ptr->block_header)/sizeof(src_mod_ptr->block_header[0]);
		if (src_mod_ptr->block_num > size) {
			ISP_LOGW("block_num(%d) is larger than block_header size for mode %d",
				src_mod_ptr->block_num, i);
		}

		dst_mod_ptr = (struct isp_pm_mode_param *)cxt_ptr->tune_mode_array[i];
		src_header = (struct isp_block_header *)src_mod_ptr->block_header;
		dst_header = (struct isp_pm_block_header *)dst_mod_ptr->header;
		for (j = 0; j < src_mod_ptr->block_num; j++) {
			dst_header[j].is_update = 0;
			dst_header[j].source_flag = i;	/*data source is common,preview,capture or video */
			dst_header[j].bypass = src_header[j].bypass;
			dst_header[j].block_id = src_header[j].block_id;
			dst_header[j].param_id = src_header[j].param_id;
			dst_header[j].version_id = src_header[j].version_id;
			dst_header[j].size = src_header[j].size;

			offset = src_header[j].offset - sizeof(struct isp_mode_param);
			offset = offset + sizeof(struct isp_pm_mode_param);
			src_data_ptr = (cmr_u8 *) ((intptr_t) src_mod_ptr + src_header[j].offset);
			dst_data_ptr = (cmr_u8 *) ((intptr_t) dst_mod_ptr + offset + extend_offset);
			dst_header[j].absolute_addr = (void *)dst_data_ptr;
			memcpy((void *)dst_data_ptr, (void *)src_data_ptr, src_header[j].size);
			memcpy((void *)dst_header[j].name, (void *)src_header[j].block_name, sizeof(dst_header[j].name));

			switch (src_header[j].block_id) {
			case ISP_BLK_AE_NEW:{
					extend_offset += add_ae_len;
					dst_header[j].size = src_header[j].size + add_ae_len;
					memcpy((void *)(dst_data_ptr + sizeof(struct ae_param_tmp_001)), (void *)(fix_data_ptr->ae.ae_param.ae), add_ae_len);
					memcpy((void *)(dst_data_ptr + sizeof(struct ae_param_tmp_001) + add_ae_len),
					       (void *)(src_data_ptr + sizeof(struct ae_param_tmp_001)), (src_header[j].size - sizeof(struct ae_param_tmp_001)));
				}
				break;
			case ISP_BLK_AWB_NEW:{
				}
				break;
			case ISP_BLK_2D_LSC:{
					extend_offset += add_lnc_len;
					dst_header[j].size = src_header[j].size + add_lnc_len;
					memcpy((void *)(dst_data_ptr + src_header[j].size), (void *)(fix_data_ptr->lnc.lnc_param.lnc), add_lnc_len);
				}
				break;
			case ISP_BLK_PDAF_CORRECT:{
					isp_blk_nr_type = ISP_BLK_PDAF_CORRECT_T;
					nr_set_addr = (intptr_t) (fix_data_ptr->nr.nr_set_group.pdaf_correct);
					nr_set_size = sizeof(struct sensor_pdaf_correction_level);
				}
				break;
			case ISP_BLK_NLM:{
					nr_param_update_info.param_type = ISP_BLK_NLM_T;
					nr_param_update_info.nr_param_ptr = (cmr_uint *) (fix_data_ptr->nr.nr_set_group.nlm);
					nr_param_update_info.size_of_per_unit = sizeof(struct sensor_nlm_level) * nr_level_number_ptr->nr_level_map[ISP_BLK_NLM_T];
					isp_nr_param_update(&nr_param_update_info);

					*((cmr_u32 *) dst_data_ptr) = (cmr_u32) nr_level_number_ptr->nr_level_map[ISP_BLK_NLM_T];
					dst_data_ptr += sizeof(cmr_u32);
					*((cmr_u32 *) dst_data_ptr) = (cmr_u32) nr_default_level_ptr->nr_level_map[ISP_BLK_NLM_T];
					dst_data_ptr += sizeof(cmr_u32);
					*((cmr_u32 *) dst_data_ptr) = (cmr_u32) multi_nr_flag;
					dst_data_ptr += sizeof(cmr_u32);
					*((intptr_t *) dst_data_ptr) = (intptr_t) (&(nr_scene_map_ptr->nr_scene_map[0]));
					dst_data_ptr += sizeof(cmr_u32);
					*((intptr_t *) dst_data_ptr) = (intptr_t) fix_data_ptr->nr.nr_set_group.nlm;
					dst_data_ptr += sizeof(intptr_t);

					nr_param_update_info.param_type = ISP_BLK_VST_T;
					nr_param_update_info.nr_param_ptr = (cmr_uint *) (fix_data_ptr->nr.nr_set_group.vst);
					nr_param_update_info.size_of_per_unit = sizeof(struct sensor_vst_level) * nr_level_number_ptr->nr_level_map[ISP_BLK_VST_T];
					isp_nr_param_update(&nr_param_update_info);
					*((intptr_t *) dst_data_ptr) = (intptr_t) fix_data_ptr->nr.nr_set_group.vst;
					dst_data_ptr += sizeof(intptr_t);

					nr_param_update_info.param_type = ISP_BLK_IVST_T;
					nr_param_update_info.nr_param_ptr = (cmr_uint *) (fix_data_ptr->nr.nr_set_group.ivst);
					nr_param_update_info.size_of_per_unit = sizeof(struct sensor_ivst_level) * nr_level_number_ptr->nr_level_map[ISP_BLK_IVST_T];
					isp_nr_param_update(&nr_param_update_info);
					*((intptr_t *) dst_data_ptr) = (intptr_t) fix_data_ptr->nr.nr_set_group.ivst;
					dst_data_ptr += sizeof(intptr_t);

					extend_offset += 3 * sizeof(struct sensor_nr_simple_header_param);
					dst_header[j].size = 3 * sizeof(struct sensor_nr_simple_header_param);
				}
				break;
			case ISP_BLK_RGB_DITHER:{
					isp_blk_nr_type = ISP_BLK_RGB_DITHER_T;
					nr_set_addr = (intptr_t) (fix_data_ptr->nr.nr_set_group.rgb_dither);
					nr_set_size = sizeof(struct sensor_rgb_dither_level);
				}
				break;
			case ISP_BLK_BPC:{
					isp_blk_nr_type = ISP_BLK_BPC_T;
					nr_set_addr = (intptr_t) (fix_data_ptr->nr.nr_set_group.bpc);
					nr_set_size = sizeof(struct sensor_bpc_level);
				}
				break;
			case ISP_BLK_GRGB:{
					isp_blk_nr_type = ISP_BLK_GRGB_T;
					nr_set_addr = (intptr_t) (fix_data_ptr->nr.nr_set_group.grgb);
					nr_set_size = sizeof(struct sensor_grgb_level);
				}
				break;
			case ISP_BLK_CFA:{
					isp_blk_nr_type = ISP_BLK_CFA_T;
					nr_set_addr = (intptr_t) (fix_data_ptr->nr.nr_set_group.cfa);
					nr_set_size = sizeof(struct sensor_cfa_param_level);
				}
				break;
			case ISP_BLK_RGB_AFM:{
					isp_blk_nr_type = ISP_BLK_RGB_AFM_T;
					nr_set_addr = (intptr_t) (fix_data_ptr->nr.nr_set_group.rgb_afm);
					nr_set_size = sizeof(struct sensor_rgb_afm_level);
				}
				break;
			case ISP_BLK_UVDIV:{
					isp_blk_nr_type = ISP_BLK_UVDIV_T;
					nr_set_addr = (intptr_t) (fix_data_ptr->nr.nr_set_group.uvdiv);
					nr_set_size = sizeof(struct sensor_cce_uvdiv_level);
				}
				break;
			case ISP_BLK_3DNR_PRE:{
					isp_blk_nr_type = ISP_BLK_3DNR_PRE_T;
					nr_set_addr = (intptr_t) (fix_data_ptr->nr.nr_set_group.nr3d_pre);
					nr_set_size = sizeof(struct sensor_3dnr_level);
				}
				break;
			case ISP_BLK_3DNR_CAP:{
					isp_blk_nr_type = ISP_BLK_3DNR_CAP_T;
					nr_set_addr = (intptr_t) (fix_data_ptr->nr.nr_set_group.nr3d_cap);
					nr_set_size = sizeof(struct sensor_3dnr_level);
				}
				break;
			case ISP_BLK_YUV_PRECDN:{
					isp_blk_nr_type = ISP_BLK_YUV_PRECDN_T;
					nr_set_addr = (intptr_t) (fix_data_ptr->nr.nr_set_group.yuv_precdn);
					nr_set_size = sizeof(struct sensor_yuv_precdn_level);
				}
				break;
			case ISP_BLK_UV_CDN:{
					isp_blk_nr_type = ISP_BLK_CDN_T;
					nr_set_addr = (intptr_t) (fix_data_ptr->nr.nr_set_group.cdn);
					nr_set_size = sizeof(struct sensor_uv_cdn_level);
				}
				break;
			case ISP_BLK_UV_POSTCDN:{
					isp_blk_nr_type = ISP_BLK_POSTCDN_T;
					nr_set_addr = (intptr_t) (fix_data_ptr->nr.nr_set_group.postcdn);
					nr_set_size = sizeof(struct sensor_uv_postcdn_level);
				}
				break;
			case ISP_BLK_YNR:{
					isp_blk_nr_type = ISP_BLK_YNR_T;
					nr_set_addr = (intptr_t) (fix_data_ptr->nr.nr_set_group.ynr);
					nr_set_size = sizeof(struct sensor_ynr_level);
				}
				break;
			case ISP_BLK_EDGE:{
					isp_blk_nr_type = ISP_BLK_EDGE_T;
					nr_set_addr = (intptr_t) (fix_data_ptr->nr.nr_set_group.edge);
					nr_set_size = sizeof(struct sensor_ee_level);
				}
				break;
			case ISP_BLK_IIRCNR_IIR:{
					isp_blk_nr_type = ISP_BLK_IIRCNR_T;
					nr_set_addr = (intptr_t) (fix_data_ptr->nr.nr_set_group.iircnr);
					nr_set_size = sizeof(struct sensor_iircnr_level);
				}
				break;
			case ISP_BLK_YUV_NOISEFILTER:{
					isp_blk_nr_type = ISP_BLK_YUV_NOISEFILTER_T;
					nr_set_addr = (intptr_t) (fix_data_ptr->nr.nr_set_group.yuv_noisefilter);
					nr_set_size = sizeof(struct sensor_yuv_noisefilter_level);
				}
				break;
			default:
				break;
			}
			if (src_header[j].block_id == ISP_BLK_PDAF_CORRECT
			    || src_header[j].block_id == ISP_BLK_RGB_DITHER
			    || src_header[j].block_id == ISP_BLK_BPC
			    || src_header[j].block_id == ISP_BLK_GRGB
			    || src_header[j].block_id == ISP_BLK_CFA
			    || src_header[j].block_id == ISP_BLK_RGB_AFM
			    || src_header[j].block_id == ISP_BLK_UVDIV
			    || src_header[j].block_id == ISP_BLK_3DNR_PRE
			    || src_header[j].block_id == ISP_BLK_3DNR_CAP
			    || src_header[j].block_id == ISP_BLK_YUV_PRECDN
			    || src_header[j].block_id == ISP_BLK_UV_CDN
			    || src_header[j].block_id == ISP_BLK_UV_POSTCDN
			    || src_header[j].block_id == ISP_BLK_YNR
			    || src_header[j].block_id == ISP_BLK_EDGE
			    || src_header[j].block_id == ISP_BLK_IIRCNR_IIR
			    || src_header[j].block_id == ISP_BLK_YUV_NOISEFILTER) {
				nr_param_update_info.param_type = isp_blk_nr_type;
				nr_param_update_info.nr_param_ptr = (cmr_uint *) nr_set_addr;
				nr_param_update_info.size_of_per_unit = nr_set_size * nr_level_number_ptr->nr_level_map[isp_blk_nr_type];
				isp_nr_param_update(&nr_param_update_info);

				*((cmr_u32 *) dst_data_ptr) = (cmr_u32) nr_level_number_ptr->nr_level_map[isp_blk_nr_type];
				dst_data_ptr += sizeof(cmr_u32);
				*((cmr_u32 *) dst_data_ptr) = (cmr_u32) nr_default_level_ptr->nr_level_map[isp_blk_nr_type];
				dst_data_ptr += sizeof(cmr_u32);
				*((cmr_u32 *) dst_data_ptr) = (cmr_u32) multi_nr_flag;
				dst_data_ptr += sizeof(cmr_u32);
				*((intptr_t *) dst_data_ptr) = (intptr_t) (&(nr_scene_map_ptr->nr_scene_map[0]));
				dst_data_ptr += sizeof(intptr_t);
				*((intptr_t *) dst_data_ptr) = (intptr_t) nr_set_addr;
				dst_data_ptr += sizeof(intptr_t);
				extend_offset += sizeof(struct sensor_nr_simple_header_param);
				dst_header[j].size = sizeof(struct sensor_nr_simple_header_param);
			}
		}

		if (max_num < src_mod_ptr->block_num)
			max_num = src_mod_ptr->block_num;

		dst_mod_ptr->block_num = src_mod_ptr->block_num;
		dst_mod_ptr->mode_id = src_mod_ptr->mode_id;
		dst_mod_ptr->resolution.w = src_mod_ptr->width;
		dst_mod_ptr->resolution.h = src_mod_ptr->height;
		dst_mod_ptr->fps = src_mod_ptr->fps;
		memcpy((void *)dst_mod_ptr->mode_name, (void *)src_mod_ptr->mode_name, sizeof(src_mod_ptr->mode_name));
		ISP_LOGV("mode[%d]: param modify_time :%d", i, src_mod_ptr->reserved[0]);

		if (PNULL != cxt_ptr->merged_mode_array[i]) {
			free(cxt_ptr->merged_mode_array[i]);
			cxt_ptr->merged_mode_array[i] = PNULL;
		}

		cxt_ptr->merged_mode_array[i] = (struct isp_pm_mode_param *)malloc(sizeof(struct isp_pm_mode_param));
		if (PNULL == cxt_ptr->merged_mode_array[i]) {
			ISP_LOGE("fail to  malloc, i=%d", i);
			rtn = ISP_ERROR;
			goto _mode_list_init_error_exit;
		}
		memset((void *)cxt_ptr->merged_mode_array[i], 0x00, sizeof(struct isp_pm_mode_param));
	}

	size = max_num * cxt_ptr->mode_num * sizeof(struct isp_pm_param_data);
	cxt_ptr->tmp_param_data_ptr = (struct isp_pm_param_data *)malloc(size);
	if (PNULL == cxt_ptr->tmp_param_data_ptr) {
		ISP_LOGE("fail to  malloc");
		rtn = ISP_ERROR;
		goto _mode_list_init_error_exit;
	}
	memset((void *)cxt_ptr->tmp_param_data_ptr, 0x00, size);

	return rtn;

_mode_list_init_error_exit:

	isp_pm_mode_list_deinit(handle);

	return rtn;
}

static cmr_s32 isp_pm_mode_param_convert(
			struct isp_pm_mode_param *next_mode_in,
			struct isp_pm_mode_param *active_mode_in,
			struct isp_pm_mode_update_status *mode_convert_status)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 block_num_active = 0;
	cmr_u32 block_num_next = 0;
	cmr_u32 i = 0, j = 0, count = 0;
	cmr_s32 smart_index = -1;
	void *smart_absolute_addr = NULL;

	if (NULL == next_mode_in || NULL == active_mode_in) {
		rtn = ISP_ERROR;
		ISP_LOGE("fail to  get valid param address :%p %p", next_mode_in, active_mode_in);
		return rtn;
	}

	block_num_active = active_mode_in->block_num;
	block_num_next = next_mode_in->block_num;

	memset(mode_convert_status, 0x00, sizeof(struct isp_pm_mode_update_status));

	for (i = 0; i < block_num_active; i++) {
		for (j = 0; j < block_num_next; j++) {
			if (active_mode_in->header[i].block_id == next_mode_in->header[j].block_id) {
				if (next_mode_in->header[j].source_flag != active_mode_in->header[i].source_flag) {
					mode_convert_status->pm_blk_status[count].block_id = active_mode_in->header[i].block_id;
					mode_convert_status->pm_blk_status[count].update_flag = 1;
					if (ISP_BLK_SMART == active_mode_in->header[i].block_id) {
						smart_index = count;
						smart_absolute_addr = active_mode_in->header[i].absolute_addr;
					}
					count++;
				}
				break;
			} else {
				continue;
			}
		}
	}

	if (smart_index >= 0 && NULL != smart_absolute_addr) {
		struct isp_smart_param *smart_param = (struct isp_smart_param *)smart_absolute_addr;

		ISP_LOGV("update smart related blocks");

		for (i = 0; i < smart_param->block_num; i++) {
			cmr_u32 block_id = smart_param->block[i].block_id;

			if (block_id >= ISP_BLK_EXT || 0 == block_id || 0 == smart_param->block[i].enable)
				continue;

			mode_convert_status->pm_blk_status[count].block_id = block_id;
			mode_convert_status->pm_blk_status[count].update_flag = 1;
			count++;

			ISP_LOGV("[%d], id=0x%x update", i, block_id);
		}
	}

	mode_convert_status->blk_num = count;

	return rtn;
}

static cmr_s32 isp_pm_mode_common_to_other(
		struct isp_pm_mode_param *mode_common_in,
		struct isp_pm_mode_param *mode_other_list_out)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 i = 0, j = 0, temp_block_num = 0;
	cmr_u32 common_block_num = 0, other_block_num = 0;

	if (NULL == mode_common_in || NULL == mode_other_list_out) {
		rtn = ISP_ERROR;
		ISP_LOGE("fail to  get valid param :%p %p", mode_common_in, mode_other_list_out);
		return rtn;
	}

	common_block_num = mode_common_in->block_num;
	other_block_num = mode_other_list_out->block_num;

	if (common_block_num < other_block_num) {
		rtn = ISP_ERROR;
		ISP_LOGE("fail to  get valid block num : %d %d", common_block_num, other_block_num);
		return rtn;
	}

	temp_block_num = other_block_num;
	for (i = 0; i < common_block_num; i++) {
		for (j = 0; j < other_block_num; j++) {
			if (mode_common_in->header[i].block_id == mode_other_list_out->header[j].block_id) {
				mode_other_list_out->header[j].source_flag = mode_other_list_out->mode_id;
				break;
			}
		}
		if (j == other_block_num) {
			memcpy((cmr_u8 *) & mode_other_list_out->header[temp_block_num],
					(cmr_u8 *) & mode_common_in->header[i],
					sizeof(struct isp_pm_block_header));
			mode_other_list_out->header[temp_block_num].source_flag = mode_common_in->mode_id;
			temp_block_num++;
		}
	}

	mode_other_list_out->block_num = temp_block_num;

	return rtn;
}

static cmr_s32 isp_pm_layout_param_and_init(cmr_handle handle)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 i = 0, counts = 0, mode_count = 0;
	struct isp_pm_context *pm_cxt_ptr = (struct isp_pm_context *)handle;

	if (NULL == pm_cxt_ptr) {
		ISP_LOGE("fail to  get valid cxt ptr ");
		rtn = ISP_ERROR;
		return rtn;
	}

	mode_count = ISP_TUNE_MODE_MAX;

	for (i = ISP_MODE_ID_COMMON; i < mode_count; i++) {
		if (PNULL == (cmr_u8 *) pm_cxt_ptr->tune_mode_array[i]) {
			continue;
		}
		memcpy((cmr_u8 *) pm_cxt_ptr->merged_mode_array[i],
				(cmr_u8 *) pm_cxt_ptr->tune_mode_array[i],
				sizeof(struct isp_pm_mode_param));
	}

	for (i = ISP_MODE_ID_PRV_0; i < mode_count; i++) {
		if (PNULL == pm_cxt_ptr->merged_mode_array[i]) {
			continue;
		}
		rtn = isp_pm_mode_common_to_other(
				pm_cxt_ptr->merged_mode_array[ISP_MODE_ID_COMMON],
				pm_cxt_ptr->merged_mode_array[i]);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGE("fail to change mode");
			rtn = ISP_ERROR;
			break;
		}

		if (0 == pm_cxt_ptr->param_source) {
			memset((void *)&pm_cxt_ptr->cxt_array[counts], 0x00, sizeof(pm_cxt_ptr->cxt_array[counts]));
		}
		pm_cxt_ptr->cxt_array[counts].mode_id = pm_cxt_ptr->merged_mode_array[i]->mode_id;
		counts++;
	}

	pm_cxt_ptr->cxt_num = counts;

	return rtn;
}

static cmr_u32 isp_pm_active_mode_param_update(struct isp_context *isp_cxt_ptr,
		struct isp_pm_mode_param *mode_param,
		struct isp_pm_mode_update_status *mode_convert_status)
{
	cmr_u32 rtn = ISP_SUCCESS;
	cmr_u32 i = 0, id = 0, offset = 0;
	cmr_u32 blcok_num_active = 0;
	cmr_u32 blcok_num_next = 0;
	intptr_t isp_cxt_start_addr = 0;
	cmr_s32 tmp_idx = 0;
	void *blk_ptr = PNULL;
	void *param_data_ptr = PNULL;
	struct isp_block_cfg *blk_cfg_ptr = PNULL;
	struct isp_block_operations *ops = PNULL;
	struct isp_pm_block_header *blk_header_ptr = PNULL;

	if ((PNULL == isp_cxt_ptr)
	    || (PNULL == mode_param)
	    || (PNULL == mode_convert_status)) {
		ISP_LOGE("fail to  get valid param ptr :%p %p %p", isp_cxt_ptr, mode_param, mode_convert_status);
		rtn = ISP_ERROR;
		return rtn;
	}

	for (i = 0; i < mode_convert_status->blk_num; i++) {
		if (mode_convert_status->pm_blk_status[i].update_flag) {
			id = mode_convert_status->pm_blk_status[i].block_id;
			blk_cfg_ptr = isp_pm_get_block_cfg(id);
			blk_header_ptr = isp_pm_get_block_header(mode_param, id, &tmp_idx);
			if ((PNULL != blk_cfg_ptr) && (PNULL != blk_header_ptr)) {
				if (blk_cfg_ptr->ops) {
					ops = blk_cfg_ptr->ops;
					isp_cxt_start_addr = (intptr_t) isp_cxt_ptr;	/*get isp context start address */
					offset = blk_cfg_ptr->offset;
					blk_ptr = (void *)(isp_cxt_start_addr + offset);	/*current block struct address; */
					param_data_ptr = (void *)blk_header_ptr->absolute_addr;	/*current block struct address; */
					if (ops->reset) {
						ops->reset(blk_ptr, blk_cfg_ptr->param_size);
					}
					if (ops->init) {
						ops->init(blk_ptr, param_data_ptr, blk_header_ptr, &mode_param->resolution);
					}
				}
			}
		}

	}

	return rtn;
}

static struct isp_data_info *isp_pm_buffer_alloc(struct isp_pm_buffer *buff_ptr, cmr_u32 size)
{
	cmr_u32 idx = 0;
	struct isp_data_info *data_ptr = PNULL;

	if ((size > 0) && (buff_ptr->count < ISP_PM_BUF_NUM)) {
		idx = buff_ptr->count;
		data_ptr = &buff_ptr->buf_array[idx];
		data_ptr->data_ptr = (void *)malloc(size);
		if (PNULL == data_ptr->data_ptr) {
			ISP_LOGE("fail to malloc , size: %d", size);
			data_ptr = PNULL;
			return data_ptr;
		}
		memset((void *)data_ptr->data_ptr, 0x00, size);
		data_ptr->size = size;
		buff_ptr->count++;
	}

	return data_ptr;
}

static cmr_s32 isp_pm_buffer_free(struct isp_pm_buffer *buffer_ptr)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 i = 0;

	for (i = 0; i < buffer_ptr->count; i++) {
		if (PNULL != buffer_ptr->buf_array[i].data_ptr) {
			free(buffer_ptr->buf_array[i].data_ptr);
			buffer_ptr->buf_array[i].data_ptr = PNULL;
			buffer_ptr->buf_array[i].size = 0;
		}
	}

	return rtn;
}

static cmr_s32 isp_pm_set_block_param(struct isp_pm_context *pm_cxt_ptr,
		struct isp_pm_param_data *param_data_ptr)
{
	cmr_s32 rtn = ISP_SUCCESS;
	void *blk_ptr = PNULL;
	intptr_t cxt_start_addr = 0;
	cmr_u32 id = 0, cmd = 0, offset = 0, size = 0;
	cmr_s32 tmp_idx = 0;
	struct isp_block_operations *ops = PNULL;
	struct isp_block_cfg *blk_cfg_ptr = PNULL;
	struct isp_pm_block_header *blk_header_ptr = PNULL;

	id = param_data_ptr->id;
	blk_cfg_ptr = isp_pm_get_block_cfg(id);
	blk_header_ptr = isp_pm_get_block_header(pm_cxt_ptr->active_mode, id, &tmp_idx);
	if ((PNULL != blk_cfg_ptr) && (PNULL != blk_header_ptr)) {
		if (blk_cfg_ptr->ops) {
			cmd = param_data_ptr->cmd;
			cxt_start_addr = (intptr_t) pm_cxt_ptr->active_cxt_ptr;
			offset = blk_cfg_ptr->offset;
			ops = blk_cfg_ptr->ops;
			blk_ptr = (void *)(cxt_start_addr + offset);
			if ((PNULL == blk_ptr)
			    || (PNULL == blk_header_ptr)) {
				rtn = ISP_ERROR;
				ISP_LOGE("fail to  get valid param : blk_addr:%p, param data_ptr:%p, header:%p",
					blk_ptr, param_data_ptr, blk_header_ptr);
			} else {
				ops->set(blk_ptr, cmd, param_data_ptr->data_ptr, blk_header_ptr);
			}
		}
	} else {
		ISP_LOGE("fail to  get validated id , id=%d", id);
	}

	return rtn;
}

static cmr_s32 isp_pm_set_mode(cmr_handle handle, cmr_u32 mode_id)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_context *isp_cxt_ptr = PNULL;
	struct isp_pm_context *pm_cxt_ptr = (struct isp_pm_context *)handle;
	struct isp_pm_mode_param *next_mode_param = PNULL;
	struct isp_pm_tune_merge_param merged_mode_param;

	if (ISP_TUNE_MODE_MAX < mode_id) {
		ISP_LOGE("fail to  get valid mode id , id=%d\n", mode_id);
		rtn = ISP_ERROR;
		return rtn;
	}

	merged_mode_param.tune_merge_para = pm_cxt_ptr->merged_mode_array;
	merged_mode_param.mode_num = pm_cxt_ptr->mode_num;

	next_mode_param = isp_pm_get_mode(merged_mode_param, mode_id);
	if (PNULL == next_mode_param) {
		ISP_LOGE("fail to  get pm mode");
		rtn = ISP_ERROR;
		goto _pm_set_mode_error_exit;
	}
	pm_cxt_ptr->active_mode = next_mode_param;

	isp_cxt_ptr = isp_pm_get_context(pm_cxt_ptr, mode_id);
	if (PNULL == isp_cxt_ptr) {
		ISP_LOGE("fail to  get pm cxt ");
		rtn = ISP_ERROR;
		goto _pm_set_mode_error_exit;
	}
	pm_cxt_ptr->active_cxt_ptr = isp_cxt_ptr;

	rtn = isp_pm_context_init(handle);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("isp_pm_context_init failed.");
	}

_pm_set_mode_error_exit:

	return rtn;
}

static cmr_s32 isp_pm_change_mode(cmr_handle handle, cmr_u32 mode_id)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_context *isp_cxt_ptr = PNULL;
	struct isp_pm_context *pm_cxt_ptr = (struct isp_pm_context *)handle;
	struct isp_pm_mode_param *next_mode_param = PNULL;
	struct isp_pm_tune_merge_param merged_mode_param;
	struct isp_pm_mode_param *org_mode_param = PNULL;

	if (ISP_TUNE_MODE_MAX < mode_id) {
		ISP_LOGE("fail to  get valid  mode id , id=%d\n", mode_id);
		rtn = ISP_ERROR;
		return rtn;
	}

	org_mode_param = pm_cxt_ptr->active_mode;
	merged_mode_param.tune_merge_para = pm_cxt_ptr->merged_mode_array;
	merged_mode_param.mode_num = pm_cxt_ptr->mode_num;

	next_mode_param = isp_pm_get_mode(merged_mode_param, mode_id);
	if (PNULL == next_mode_param) {
		ISP_LOGE("fail to  get pm mode");
		rtn = ISP_ERROR;
		goto _pm_set_mode_error_exit;
	}
	pm_cxt_ptr->active_mode = next_mode_param;

	isp_cxt_ptr = isp_pm_get_context(pm_cxt_ptr, mode_id);
	if (PNULL == isp_cxt_ptr) {
		ISP_LOGE("fail to  get pm cxt ");
		rtn = ISP_ERROR;
		goto _pm_set_mode_error_exit;
	}
	pm_cxt_ptr->active_cxt_ptr = isp_cxt_ptr;

	isp_pm_context_update(handle, org_mode_param);

_pm_set_mode_error_exit:

	return rtn;
}

static cmr_s32 isp_pm_set_param(cmr_handle handle, enum isp_pm_cmd cmd, void *param_ptr)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pm_context *pm_cxt_ptr = PNULL;

	if ((PNULL == handle) || (PNULL == param_ptr)) {
		ISP_LOGE("fail to  get valid param: %p %p", handle, param_ptr);
		return ISP_ERROR;
	}

	pm_cxt_ptr = (struct isp_pm_context *)handle;

	switch (cmd) {
	case ISP_PM_CMD_SET_MODE:
		{
			cmr_u32 mode_id = *((cmr_u32 *) param_ptr);
			rtn = isp_pm_change_mode(handle, mode_id);
		}
		break;
	case ISP_PM_CMD_SET_AWB:
	case ISP_PM_CMD_SET_AE:
	case ISP_PM_CMD_SET_AF:
	case ISP_PM_CMD_SET_SMART:
	case ISP_PM_CMD_SET_OTHERS:
	case ISP_PM_CMD_SET_SCENE_MODE:
	case ISP_PM_CMD_SET_SPECIAL_EFFECT:
		{
			struct isp_pm_ioctl_input *ioctrl_input_ptr = (struct isp_pm_ioctl_input *)param_ptr;
			struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)ioctrl_input_ptr->param_data_ptr;

			if (PNULL == param_data_ptr) {
				rtn = ISP_ERROR;
				ISP_LOGE("fail to  get valid  param");
				return rtn;
			}

			rtn = isp_pm_set_block_param(pm_cxt_ptr, param_data_ptr);
		}
		break;
	case ISP_PM_CMD_ALLOC_BUF_MEMORY:
		{
			cmr_u32 id = 0, size = 0;
			struct isp_data_info *data_ptr = PNULL;
			struct isp_buffer_size_info *buffer_size_ptr = PNULL;
			struct isp_pm_param_data param_data;
			struct isp_pm_memory_init_param memory_param;
			struct isp_pm_ioctl_input *ioctrl_input_ptr = (struct isp_pm_ioctl_input *)param_ptr;
			struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)ioctrl_input_ptr->param_data_ptr;

			if (PNULL == param_data_ptr) {
				rtn = ISP_ERROR;
				ISP_LOGE("fail to  get valid param data, cmd: 0x%x\n", cmd);
				return rtn;
			}
			buffer_size_ptr = (struct isp_buffer_size_info *)param_data_ptr->data_ptr;
			if (PNULL == buffer_size_ptr) {
				rtn = ISP_ERROR;
				ISP_LOGE("fail to  get valid param data, cmd:0x%x\n", cmd);
				return rtn;
			}

			size = buffer_size_ptr->count_lines * buffer_size_ptr->pitch;
			data_ptr = (struct isp_data_info *)isp_pm_buffer_alloc(&pm_cxt_ptr->buffer, size);
			if (PNULL == data_ptr) {
				ISP_LOGE("fail to alloc buffer memory");
				return rtn;
			}
			memory_param.buffer = *data_ptr;
			memory_param.size_info = *buffer_size_ptr;
			param_data = *param_data_ptr;
			param_data.data_ptr = (void *)&memory_param;
			param_data.data_size = sizeof(struct isp_pm_memory_init_param);
			param_data.cmd = ISP_PM_BLK_MEMORY_INIT;
			rtn = isp_pm_set_block_param(pm_cxt_ptr, &param_data);
		}
		break;
	case ISP_PM_CMD_SET_PARAM_SOURCE:
		{
			cmr_u32 *param_source = (cmr_u32 *) param_ptr;
			pm_cxt_ptr->param_source = *param_source;
		}
		break;
	default:
		break;
	}

	return rtn;
}

static cmr_s32 isp_pm_get_single_block_param(struct isp_pm_mode_param *mode_param_in,
					     struct isp_context *isp_cxt_ptr,
					     struct isp_pm_ioctl_input *blk_info_in, cmr_u32 * param_count,
					     struct isp_pm_param_data *data_param_ptr, cmr_u32 * rtn_idx)
{
	cmr_s32 rtn = ISP_SUCCESS;
	intptr_t isp_cxt_start_addr = 0;
	void *blk_ptr = NULL;
	cmr_s32 tm_idx = 0;
	cmr_u32 i = 0, id = 0, cmd = 0, counts = 0, offset = 0, blk_num = 0;
	struct isp_pm_block_header *blk_header_ptr = PNULL;
	struct isp_pm_param_data *block_param_data_ptr = NULL;
	struct isp_block_operations *ops = PNULL;
	struct isp_block_cfg *blk_cfg_ptr = PNULL;

	if (blk_info_in->param_num > 1) {
		ISP_LOGE("fail to  get valid block num.only support one, num: %d\n", blk_info_in->param_num);
		rtn = ISP_ERROR;
		return rtn;
	}

	blk_num = blk_info_in->param_num;
	block_param_data_ptr = blk_info_in->param_data_ptr;
	for (i = 0; i < 1; i++) {	//just support get only one block info
		id = block_param_data_ptr[i].id;
		cmd = block_param_data_ptr[i].cmd;
		blk_header_ptr = isp_pm_get_block_header(mode_param_in, id, &tm_idx);
		blk_cfg_ptr = isp_pm_get_block_cfg(id);
		if ((NULL != blk_cfg_ptr) && (NULL != blk_header_ptr)) {
			isp_cxt_start_addr = (intptr_t) isp_cxt_ptr;
			offset = blk_cfg_ptr->offset;
			ops = blk_cfg_ptr->ops;
			blk_ptr = (void *)(isp_cxt_start_addr + offset);
			if (NULL != ops && NULL != ops->get) {
				ops->get(blk_ptr, cmd, &data_param_ptr[tm_idx], &blk_header_ptr->is_update);
				counts++;
			}
		} else {
			ISP_LOGE("Fail to get valid block for id:%x (%d),  header: %p,  cfg:  %p\n",
				id,  tm_idx, blk_header_ptr, blk_cfg_ptr);
			rtn = ISP_PARAM_ERROR;
			return rtn;
		}
		*rtn_idx = tm_idx;
	}
	*param_count = counts;

	return rtn;
}

static cmr_s32 isp_pm_get_param(cmr_handle handle, enum isp_pm_cmd cmd, void *in_ptr, void *out_ptr)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 blk_num = 0, id = 0, i = 0, j = 0;
	struct isp_pm_context *pm_cxt_ptr = PNULL;
	struct isp_pm_ioctl_output *result_ptr = PNULL;
	struct isp_pm_param_data *param_data_ptr = PNULL;
	struct isp_pm_mode_param *mode_param_ptr = PNULL;
	struct isp_pm_block_header *blk_header_array = PNULL;
	struct isp_pm_context *cxt_ptr = PNULL;
	struct isp_video_start *param_ptr = PNULL;
	cmr_u32 mod_num = 0;
	cmr_s32 *version_id = PNULL;

	if (PNULL == out_ptr) {
		ISP_LOGE("fail to  get valid output ptr");
		return ISP_PARAM_NULL;
	}

	if (ISP_PM_CMD_GET_MODEID_BY_FPS == cmd) {
		struct isp_pm_context *cxt_ptr = (struct isp_pm_context *)handle;
		*((cmr_s32 *) out_ptr) = ISP_MODE_ID_VIDEO_0;
		for (i = ISP_MODE_ID_VIDEO_0; i < ISP_MODE_ID_VIDEO_3; i++) {
			if (cxt_ptr->tune_mode_array[i] != NULL) {
				if (cxt_ptr->tune_mode_array[i]->fps == (cmr_u32) (*(cmr_s32 *) in_ptr)) {
					*((cmr_s32 *) out_ptr) = cxt_ptr->tune_mode_array[i]->mode_id;
					break;
				}
			}
		}
	} else if (ISP_PM_CMD_GET_MODEID_BY_RESOLUTION == cmd) {
		cxt_ptr = (struct isp_pm_context *)handle;
		param_ptr = in_ptr;
		if (param_ptr->work_mode == 0) {
			*((cmr_s32 *) out_ptr) = ISP_MODE_ID_PRV_0;
			for (i = ISP_MODE_ID_PRV_0; i < ISP_MODE_ID_PRV_3; i++) {
				if (cxt_ptr->tune_mode_array[i] != NULL) {
					if (cxt_ptr->tune_mode_array[i]->resolution.w == param_ptr->size.w) {
						*((cmr_s32 *) out_ptr) = cxt_ptr->tune_mode_array[i]->mode_id;
						cxt_ptr->cur_mode_id = cxt_ptr->tune_mode_array[i]->mode_id;
						break;
					}
				}
			}
		} else {
			*((cmr_s32 *) out_ptr) = ISP_MODE_ID_CAP_0;
			for (i = ISP_MODE_ID_CAP_0; i < ISP_MODE_ID_CAP_3; i++) {
				if (cxt_ptr->tune_mode_array[i] != NULL) {
					if (cxt_ptr->tune_mode_array[i]->resolution.w == param_ptr->size.w) {
						*((cmr_s32 *) out_ptr) = cxt_ptr->tune_mode_array[i]->mode_id;
						break;
					}
				}
			}
		}
	} else if (ISP_PM_CMD_GET_DV_MODEID_BY_RESOLUTION == cmd) {
		cxt_ptr = (struct isp_pm_context *)handle;
		param_ptr = in_ptr;
		if (param_ptr->dv_mode == 0) {
			ISP_LOGE("fail to  get valid dv mode\n");
		} else {
			*((cmr_s32 *) out_ptr) = ISP_MODE_ID_VIDEO_0;
			for (i = ISP_MODE_ID_VIDEO_0; i < ISP_MODE_ID_VIDEO_3; i++) {
				if (cxt_ptr->tune_mode_array[i] != NULL) {
					if (cxt_ptr->tune_mode_array[i]->resolution.w == param_ptr->size.w) {
						*((cmr_s32 *) out_ptr) = cxt_ptr->tune_mode_array[i]->mode_id;
						cxt_ptr->cur_mode_id = cxt_ptr->tune_mode_array[i]->mode_id;
						break;
					}
				}
			}
		}
	} else if ((ISP_PM_CMD_GET_ISP_SETTING == cmd) || (ISP_PM_CMD_GET_ISP_ALL_SETTING == cmd)) {
		void *blk_ptr = PNULL;
		cmr_u32 flag = 0, is_update = 0;
		cmr_u32 update_flag = 0;
		cmr_u32 offset = 0, counts = 0;
		intptr_t isp_cxt_start_addr = 0;
		struct isp_block_operations *ops = PNULL;
		struct isp_block_cfg *blk_cfg_ptr = PNULL;
		struct isp_context *isp_cxt_ptr = PNULL;
		struct isp_pm_block_header *blk_header_ptr = PNULL;

		if (ISP_PM_CMD_GET_ISP_SETTING == cmd) {
			flag = 0;
		} else if (ISP_PM_CMD_GET_ISP_ALL_SETTING == cmd) {
			flag = 1;
		}

		pm_cxt_ptr = (struct isp_pm_context *)handle;
		isp_cxt_ptr = (struct isp_context *)pm_cxt_ptr->active_cxt_ptr;
		mode_param_ptr = (struct isp_pm_mode_param *)pm_cxt_ptr->active_mode;
		blk_header_array = (struct isp_pm_block_header *)mode_param_ptr->header;
		param_data_ptr = (struct isp_pm_param_data *)pm_cxt_ptr->tmp_param_data_ptr;
		result_ptr = (struct isp_pm_ioctl_output *)out_ptr;

		blk_num = mode_param_ptr->block_num;
		for (i = 0; i < blk_num; i++) {
			id = blk_header_array[i].block_id;
			blk_cfg_ptr = isp_pm_get_block_cfg(id);
			blk_header_ptr = &blk_header_array[i];
			if ((PNULL != blk_cfg_ptr) && (PNULL != blk_header_ptr)) {
				is_update = 0;
				if ((ISP_ZERO != blk_header_ptr->is_update) || (ISP_ZERO != flag)) {
					is_update = 1;
				}

				if (is_update) {
					isp_cxt_start_addr = (intptr_t) isp_cxt_ptr;
					offset = blk_cfg_ptr->offset;
					ops = blk_cfg_ptr->ops;
					blk_ptr = (void *)(isp_cxt_start_addr + offset);
					if (ops != NULL) {
						if ((PNULL == blk_ptr) || (PNULL == param_data_ptr)) {
							ISP_LOGE("fail to  get valid param: blk_addr:%p, param_ptr:%p, isp_update:%d\n",
								 blk_ptr, param_data_ptr, blk_header_ptr->is_update);
						} else {
							ops->get(blk_ptr, ISP_PM_BLK_ISP_SETTING, param_data_ptr, &blk_header_ptr->is_update);
						}
					}
					if (param_data_ptr)
						param_data_ptr++;
					counts++;
				}

			} else {
				ISP_LOGV("no operation function:%d", id);
			}
		}
		result_ptr->param_data = pm_cxt_ptr->tmp_param_data_ptr;
		result_ptr->param_num = counts;
	} else if (ISP_PM_CMD_GET_SINGLE_SETTING == cmd) {
		cmr_u32 single_param_counts = 0;
		cmr_u32 blk_idx = 0;
		struct isp_pm_param_data *single_data_ptr = PNULL;

		pm_cxt_ptr = (struct isp_pm_context *)handle;
		single_data_ptr = pm_cxt_ptr->temp_param_data;
		result_ptr = (struct isp_pm_ioctl_output *)out_ptr;
		result_ptr->param_num = 0;
		result_ptr->param_data = PNULL;
		rtn = isp_pm_get_single_block_param(pm_cxt_ptr->active_mode,
						    pm_cxt_ptr->active_cxt_ptr, (struct isp_pm_ioctl_input *)in_ptr,
						    &single_param_counts, single_data_ptr, &blk_idx);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGE("fail to get single block param!");
			rtn = ISP_ERROR;
			return rtn;
		}
		result_ptr->param_data = &pm_cxt_ptr->temp_param_data[blk_idx];
		result_ptr->param_num = single_param_counts;	//always is one
	} else if (ISP_PM_CMD_GET_AE_VERSION_ID == cmd) {
		pm_cxt_ptr = (struct isp_pm_context *)handle;
		mod_num = 0;
		version_id = (cmr_s32 *) out_ptr;
		mod_num = pm_cxt_ptr->mode_num;
		for (j = 0; j < mod_num; j++) {
			mode_param_ptr = (struct isp_pm_mode_param *)pm_cxt_ptr->tune_mode_array[j];
			if (PNULL != mode_param_ptr) {
				blk_header_array = (struct isp_pm_block_header *)mode_param_ptr->header;
				blk_num = mode_param_ptr->block_num;
				for (i = 0; i < blk_num; i++) {
					if (ISP_BLK_AE_NEW == blk_header_array[i].block_id) {
						*version_id = blk_header_array[i].version_id;
					}
					if (-1 != *version_id)
						break;
				}
			} else {
				continue;
			}
			if (-1 != *version_id)
				break;
		}
	} else {
		void *awb_data_ptr = PNULL;
		cmr_u32 block_id = 0;
		cmr_u32 mod_num = 0;
		switch (cmd) {
		case ISP_PM_CMD_GET_INIT_AE:
			block_id = ISP_BLK_AE_NEW;
			break;
		case ISP_PM_CMD_GET_INIT_ALSC:
			block_id = ISP_BLK_ALSC;
			break;
		case ISP_PM_CMD_GET_INIT_AWB:
			block_id = ISP_BLK_AWB_NEW;
			break;
		case ISP_PM_CMD_GET_INIT_AF:
		case ISP_PM_CMD_GET_INIT_AF_NEW:
			block_id = ISP_BLK_AF_NEW;
			break;
		case ISP_PM_CMD_GET_INIT_SMART:
			block_id = ISP_BLK_SMART;
			break;
		case ISP_PM_CMD_GET_INIT_AFT:
			block_id = ISP_BLK_AFT;
			break;
		case ISP_PM_CMD_GET_THIRD_PART_INIT_SFT_AF:
			block_id = ISP_BLK_SFT_AF;
			break;
		case ISP_PM_CMD_GET_INIT_DUAL_FLASH:
			block_id = ISP_BLK_DUAL_FLASH;
			break;
		default:
			break;
		}
		pm_cxt_ptr = (struct isp_pm_context *)handle;
		result_ptr = (struct isp_pm_ioctl_output *)out_ptr;
		result_ptr->param_num = 0;
		result_ptr->param_data = PNULL;
		mod_num = pm_cxt_ptr->mode_num;
		param_data_ptr = (struct isp_pm_param_data *)pm_cxt_ptr->tmp_param_data_ptr;
		for (j = 0; j < mod_num; j++) {
			mode_param_ptr = (struct isp_pm_mode_param *)pm_cxt_ptr->tune_mode_array[j];
			if (PNULL != mode_param_ptr) {
				blk_header_array = (struct isp_pm_block_header *)mode_param_ptr->header;
				blk_num = mode_param_ptr->block_num;
				for (i = 0; i < blk_num; i++) {
					id = blk_header_array[i].block_id;
					if (block_id == id) {
						break;
					}
				}
				if (i < blk_num) {
					param_data_ptr->data_ptr = (void *)blk_header_array[i].absolute_addr;
					param_data_ptr->data_size = blk_header_array[i].size;
					param_data_ptr->user_data[0] = blk_header_array[i].bypass;
				} else {
					param_data_ptr->data_ptr = PNULL;
					param_data_ptr->data_size = 0;
				}
			} else {
				param_data_ptr->data_ptr = PNULL;
				param_data_ptr->data_size = 0;
			}

			param_data_ptr->cmd = cmd;
			param_data_ptr->mod_id = j;
			param_data_ptr->id = (j << 16) | block_id;	/*H-16: mode id L-16: block id */
			param_data_ptr++;
			result_ptr->param_num++;
		}
		result_ptr->param_data = (void *)pm_cxt_ptr->tmp_param_data_ptr;
	}

	return rtn;
}

static cmr_s32 isp_pm_param_init_and_update(cmr_handle handle,
					    struct isp_pm_init_input *input,
					    struct isp_pm_init_output *output,
					    cmr_u32 mod_id)
{
	cmr_s32 rtn = ISP_SUCCESS;

	rtn = isp_pm_mode_list_init(handle, input, output);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to init mode list");
		return rtn;
	}

	rtn = isp_pm_layout_param_and_init(handle);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to init layout ");
		return rtn;
	}

	rtn = isp_pm_active_mode_init(handle, mod_id);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to init active mode ");
		return rtn;
	}

	rtn = isp_pm_set_mode(handle, mod_id);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to set mode");
		return rtn;
	}

	return rtn;
}

static void isp_pm_free(cmr_handle handle)
{
	if (PNULL != handle) {
		struct isp_pm_context *cxt_ptr = (struct isp_pm_context *)handle;

		cxt_ptr->magic_flag = ~cxt_ptr->magic_flag;
		pthread_mutex_destroy(&cxt_ptr->pm_mutex);

		isp_pm_context_deinit(handle);
		isp_pm_mode_list_deinit(handle);
		isp_pm_buffer_free(&cxt_ptr->buffer);

		free(handle);
	}
}

static cmr_s32 isp_pm_lsc_otp_param_update(cmr_handle handle, struct isp_pm_param_data *param_data)
{
	cmr_s32 rtn = ISP_SUCCESS;
	void *blk_ptr = PNULL;
	cmr_u32 i = 0, offset = 0;
	cmr_u32 mod_id = 0, id = 0;
	cmr_s32 tmp_idx = 0;
	intptr_t isp_cxt_start_addr = 0;
	struct isp_context *isp_cxt_ptr = PNULL;
	struct isp_block_cfg *blk_cfg = PNULL;
	struct isp_pm_block_header *blk_header_ptr = PNULL;
	struct isp_pm_context *pm_cxt_ptr = (struct isp_pm_context *)handle;

	for (i = ISP_MODE_ID_PRV_0; i < ISP_TUNE_MODE_MAX; ++i) {
		if (pm_cxt_ptr->merged_mode_array[i]) {
			mod_id = pm_cxt_ptr->merged_mode_array[i]->mode_id;
			isp_cxt_ptr = isp_pm_get_context(pm_cxt_ptr, mod_id);
			isp_cxt_start_addr = (intptr_t) isp_cxt_ptr;
			id = param_data->id;
			blk_cfg = isp_pm_get_block_cfg(id);
			blk_header_ptr = isp_pm_get_block_header(pm_cxt_ptr->merged_mode_array[i], id, &tmp_idx);
			if ((PNULL != blk_cfg)
			    && (PNULL != blk_cfg->ops)
			    && (PNULL != blk_cfg->ops->set)
			    && (PNULL != blk_header_ptr)) {
				offset = blk_cfg->offset;
				blk_ptr = (void *)(isp_cxt_start_addr + offset);
				blk_cfg->ops->set(blk_ptr, ISP_PM_BLK_LSC_OTP, param_data->data_ptr, blk_header_ptr);
			} else {
				ISP_LOGE("fail to get valid cfg, ops, set, header");
			}
		}
	}

	return rtn;
}

cmr_handle isp_pm_init(struct isp_pm_init_input * input, void *output)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_handle handle = PNULL;
	struct isp_pm_context *cxt_ptr;

	if (PNULL == input) {
		ISP_LOGE("fail to get valid input param");
		return PNULL;
	}

	handle = isp_pm_context_create();
	rtn = isp_pm_handle_check(handle);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to get valid handle");
		goto init_error_exit;
	}

	cxt_ptr = (struct isp_pm_context *)handle;
	pthread_mutex_lock(&cxt_ptr->pm_mutex);
	rtn = isp_pm_param_init_and_update(handle, input, output, ISP_MODE_ID_PRV_0);
	pthread_mutex_unlock(&cxt_ptr->pm_mutex);

	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to init & update");
		goto init_error_exit;
	}

	return handle;

init_error_exit:

	if (handle) {
		struct isp_pm_context *cxt_ptr = handle;

		isp_pm_free(handle);
		handle = PNULL;
	}

	return handle;
}

cmr_s32 isp_pm_ioctl(cmr_handle handle, enum isp_pm_cmd cmd, void *input, void *output)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pm_context *cxt_ptr = handle;

	rtn = isp_pm_handle_check(handle);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to get valid isp pm handel");
		rtn = ISP_ERROR;
		goto _ioctl_error_exit;
	}

	switch ((cmd & isp_pm_cmd_mask)) {
	case ISP_PM_CMD_SET_BASE:
		pthread_mutex_lock(&cxt_ptr->pm_mutex);
		rtn = isp_pm_set_param(handle, cmd, input);
		pthread_mutex_unlock(&cxt_ptr->pm_mutex);
		break;
	case ISP_PM_CMD_GET_BASE:
	case ISP_PM_CMD_GET_THIRD_PART_BASE:
		pthread_mutex_lock(&cxt_ptr->pm_mutex);
		rtn = isp_pm_get_param(handle, cmd, input, output);
		pthread_mutex_unlock(&cxt_ptr->pm_mutex);
		break;
	default:
		break;
	}

_ioctl_error_exit:

	return rtn;
}

cmr_s32 isp_pm_update(cmr_handle handle, enum isp_pm_cmd cmd, void *input, void *output)
{
	cmr_s32 rtn = ISP_SUCCESS;

	rtn = isp_pm_handle_check(handle);
	if (ISP_SUCCESS != rtn) {
		rtn = ISP_ERROR;
		ISP_LOGE("fail to get valid handle");
		goto isp_pm_update_error_exit;
	}

	struct isp_pm_context *cxt_ptr = (struct isp_pm_context *)handle;

	switch (cmd) {
	case ISP_PM_CMD_UPDATE_ALL_PARAMS:
		{
			pthread_mutex_lock(&cxt_ptr->pm_mutex);
			rtn = isp_pm_param_init_and_update(handle, input, output, cxt_ptr->cur_mode_id);
			pthread_mutex_unlock(&cxt_ptr->pm_mutex);
		}
		break;

	case ISP_PM_CMD_UPDATE_LSC_OTP:
		{
			pthread_mutex_lock(&cxt_ptr->pm_mutex);
			rtn = isp_pm_lsc_otp_param_update(handle, input);
			pthread_mutex_unlock(&cxt_ptr->pm_mutex);
		}
		break;
	default:
		break;
	}

	return rtn;

isp_pm_update_error_exit:
	return rtn;
}

cmr_s32 isp_pm_deinit(cmr_handle handle, void *input, void *output)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pm_context *cxt_ptr = handle;
	UNUSED(input);
	UNUSED(output);

	rtn = isp_pm_handle_check(handle);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to get valid handle, rtn: %d\n", rtn);

		return ISP_ERROR;
	}

	isp_pm_free(handle);

	handle = PNULL;

	return rtn;
}
