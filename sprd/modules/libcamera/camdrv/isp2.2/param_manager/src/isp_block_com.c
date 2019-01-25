/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define LOG_TAG "isp_blk_com"
#include "isp_blocks_cfg.h"
#include "cmr_types.h"

#define array_offset(type, member) (intptr_t)(&((type*)0)->member)

#define ISP_PM_LEVEL_DEFAULT 3
#define ISP_PM_CCE_DEFAULT 0

#define ISP_NR_AUTO_MODE_BIT (0x01 << ISP_SCENEMODE_AUTO)
#define ISP_NR_NIGHT_MODE_BIT (0x01 << ISP_SCENEMODE_NIGHT)
#define ISP_NR_SPORT_MODE_BIT (0x01 << ISP_SCENEMODE_SPORT)
#define ISP_NR_PORTRAIT_MODE_BIT (0x01 << ISP_SCENEMODE_PORTRAIT)
#define ISP_NR_LANDSPACE_MODE_BIT (0x01 << ISP_SCENEMODE_LANDSCAPE)
#define ISP_NR_PANORAMA_MODE_BIT (0x01 << ISP_SCENEMODE_PANORAMA)
#define ISP_NR_SNOW_MODE_BIT (0x01 << ISP_SCENEMODE_SNOW)
#define ISP_NR_FIREWORK_MODE_BIT (0x01 << ISP_SCENEMODE_FIREWORK)
#define ISP_NR_DUSK_MODE_BIT (0x01 << ISP_SCENEMODE_DUSK)
#define ISP_NR_AUTUMN_MODE_BIT (0x01 << ISP_SCENEMODE_AUTUMN)
#define ISP_NR_TEXT_MODE_BIT (0x01 << ISP_SCENEMODE_TEXT)
#define ISP_NR_BACKLIGHT_MODE_BIT (0x01 << ISP_SCENEMODE_BACKLIGHT)
#define ISP_NR_12_MODE_BIT (0x01 << 12)
#define ISP_NR_13_MODE_BIT (0x01 << 13)
#define ISP_NR_14_MODE_BIT (0x01 << 14)
#define ISP_NR_15_MODE_BIT (0x01 << 15)
cmr_u8 nr_tool_flag[17] = { 0 };

cmr_u32 scene_mode_matrix[MAX_SCENEMODE_NUM] = {
	ISP_NR_AUTO_MODE_BIT,
	ISP_NR_NIGHT_MODE_BIT,
	ISP_NR_SPORT_MODE_BIT,
	ISP_NR_PORTRAIT_MODE_BIT,
	ISP_NR_LANDSPACE_MODE_BIT,
	ISP_NR_PANORAMA_MODE_BIT,
	ISP_NR_SNOW_MODE_BIT,
	ISP_NR_FIREWORK_MODE_BIT,
	ISP_NR_DUSK_MODE_BIT,
	ISP_NR_AUTUMN_MODE_BIT,
	ISP_NR_TEXT_MODE_BIT,
	ISP_NR_BACKLIGHT_MODE_BIT,
	ISP_NR_12_MODE_BIT,
	ISP_NR_13_MODE_BIT,
	ISP_NR_14_MODE_BIT,
	ISP_NR_15_MODE_BIT,
};

cmr_u32 sensor_mode_matrix[MAX_MODE_NUM] = {
	ISP_MODE_ID_COMMON,
	ISP_MODE_ID_PRV_0,
	ISP_MODE_ID_PRV_0,
	ISP_MODE_ID_PRV_0,
	ISP_MODE_ID_PRV_0,
	ISP_MODE_ID_CAP_0,
	ISP_MODE_ID_CAP_0,
	ISP_MODE_ID_CAP_0,
	ISP_MODE_ID_CAP_0,
	ISP_MODE_ID_VIDEO_0,
	ISP_MODE_ID_VIDEO_0,
	ISP_MODE_ID_VIDEO_0,
	ISP_MODE_ID_VIDEO_0,
};

cmr_u32 _pm_calc_nr_addr_offset(cmr_u32 mode_flag, cmr_u32 scene_flag, cmr_u32 * one_multi_mode_ptr)
{
	cmr_u32 offset_units = 0, offset_units_remain = 0;
	cmr_u32 i = 0, j = 0, quotient = 0, remainder = 0;

	if ((mode_flag > (MAX_MODE_NUM - 1)) || (scene_flag > (MAX_SCENEMODE_NUM - 1)))
		return 0;
	if (!one_multi_mode_ptr)
		return 0;

	/* current mode */
	if (one_multi_mode_ptr[mode_flag] & scene_mode_matrix[scene_flag]) {
		offset_units = mode_flag * MAX_SCENEMODE_NUM + scene_flag;
	}
	/* current mode normal scene */
	else if (one_multi_mode_ptr[mode_flag] & ISP_NR_AUTO_MODE_BIT) {
		offset_units = mode_flag * MAX_SCENEMODE_NUM;
	}
	/* class mode current scene  */
	else if (one_multi_mode_ptr[sensor_mode_matrix[mode_flag]] & scene_mode_matrix[scene_flag]) {
		offset_units = sensor_mode_matrix[mode_flag] * MAX_SCENEMODE_NUM + scene_flag;
	}
	/* class mode normal scene */
	else if (one_multi_mode_ptr[sensor_mode_matrix[mode_flag]] & ISP_NR_AUTO_MODE_BIT) {
		offset_units = sensor_mode_matrix[mode_flag] * MAX_SCENEMODE_NUM;
	}
	/* noraml mode current scene  */
	else if (one_multi_mode_ptr[ISP_MODE_ID_COMMON] & scene_mode_matrix[scene_flag]) {
		offset_units = scene_flag;
	}
	/* noraml mode normal scene  */
	else if (one_multi_mode_ptr[ISP_MODE_ID_COMMON] & ISP_NR_AUTO_MODE_BIT) {
		offset_units = 0;
	} else {
		ISP_LOGE("fail to find multi NR version\n");
		offset_units = 0;
	}
	ISP_LOGV("offset_units = %d)",offset_units);

	quotient = offset_units / MAX_SCENEMODE_NUM;
	remainder = offset_units % MAX_SCENEMODE_NUM;
	/* calc whole block */
	for (i = 0; i < quotient; i++) {
		for (j = 0; j < MAX_SCENEMODE_NUM; j++) {
			offset_units_remain += (one_multi_mode_ptr[i] >> j) & 0x01;
		}
	}
	/* calc remaind fraction */
	for (j = 0; j < remainder; j++) {
		offset_units_remain += (one_multi_mode_ptr[quotient] >> j) & 0x01;
	}
	/* sensor mode and scene mode setting for multi nr function */
	ISP_LOGV("mode_flag,scene_flag,offset_units_remain(%d,%d,%d)",mode_flag, scene_flag, offset_units_remain);
	return offset_units_remain;
}

cmr_s32 PM_CLIP(cmr_s32 x, cmr_s32 bottom, cmr_s32 top)
{
	cmr_s32 val = 0;

	if (x < bottom) {
		val = bottom;
	} else if (x > top) {
		val = top;
	} else {
		val = x;
	}

	return val;
}

cmr_s32 _is_print_log()
{
	char value[PROPERTY_VALUE_MAX] = { 0 };
	cmr_u32 is_print = 0;

	property_get("debug.camera.isp.pm", value, "0");

	if (!strcmp(value, "1")) {
		is_print = 1;
	}

	return is_print;
}

cmr_s32 _pm_check_smart_param(struct smart_block_result * block_result, struct isp_range * range, cmr_u32 comp_num, cmr_u32 type)
{
	cmr_u32 i = 0;

	if (NULL == block_result) {
		ISP_LOGE("fail to check valid pointer\n");
		return ISP_ERROR;
	}

	if (comp_num != block_result->component_num) {
		ISP_LOGE("fail to component num : %d (%d)\n", block_result->component_num, comp_num);
		return ISP_ERROR;
	}

	if (0 == block_result->update) {
		ISP_LOGV("do not need update\n");
		return ISP_ERROR;
	}

	for (i = 0; i < comp_num; i++) {
		if (type != block_result->component[0].y_type) {
			ISP_LOGE("fail to block type : %d (%d)\n", block_result->component[0].y_type, type);
			return ISP_ERROR;
		}

		if (ISP_SMART_Y_TYPE_VALUE == type) {
			cmr_s32 value = block_result->component[i].fix_data[0];

			if (value < range->min || value > range->max) {
				ISP_LOGE("fail to value range: %d ([%d, %d])\n", value, range->min, range->max);
				return ISP_ERROR;
			}
		} else if (ISP_SMART_Y_TYPE_WEIGHT_VALUE == type) {
			struct isp_weight_value *weight_value = (struct isp_weight_value *)block_result->component[i].fix_data;

			if ((cmr_s32) weight_value->value[0] < range->min || (cmr_s32) weight_value->value[0] > range->max) {
				ISP_LOGE("fail to value range: %d ([%d, %d])\n", weight_value->value[0], range->min, range->max);
				return ISP_ERROR;
			}

			if ((cmr_s32) weight_value->value[1] < range->min || (cmr_s32) weight_value->value[1] > range->max) {
				ISP_LOGE("fail to value range: %d ([%d, %d])\n", weight_value->value[1], range->min, range->max);
				return ISP_ERROR;
			}
		}
	}

	return ISP_SUCCESS;
}

cmr_s32 _pm_common_rest(void *blk_addr, cmr_u32 size)
{
	cmr_s32 rtn = ISP_SUCCESS;
	memset((void *)blk_addr, 0x00, size);
	return rtn;
}

cmr_u32 _pm_get_lens_grid_mode(cmr_u32 grid)
{
	cmr_u32 mode = 0x00;

	switch (grid) {
	case 16:
		mode = 0;
		break;

	case 32:
		mode = 1;
		break;

	case 64:
		mode = 2;
		break;

	default:
		ISP_LOGE("fail to get lens grid");
		break;
	}

	return mode;
}

cmr_u16 _pm_get_lens_grid_pitch(cmr_u32 grid_pitch, cmr_u32 width, cmr_u32 flag)
{
	cmr_u16 pitch = ISP_SUCCESS;

	if (0 == grid_pitch) {
		return pitch;
	}

	if (ISP_ZERO != ((width / ISP_TWO - ISP_ONE) % grid_pitch)) {
		pitch = ((width / ISP_TWO - ISP_ONE) / grid_pitch + ISP_TWO);
	} else {
		pitch = ((width / ISP_TWO - ISP_ONE) / grid_pitch + ISP_ONE);
	}

	if (ISP_ONE == flag) {
		pitch += 2;
	}

	return pitch;
}

cmr_u32 _ispLog2n(cmr_u32 index)
{
	cmr_u32 index_num = index;
	cmr_u32 i = 0x00;

	for (i = 0x00; index_num > 1; i++) {
		index_num >>= 0x01;
	}

	return i;
}
