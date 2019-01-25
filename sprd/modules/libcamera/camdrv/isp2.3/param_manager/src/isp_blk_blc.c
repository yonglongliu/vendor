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
#define LOG_TAG "isp_blk_blc"
#include "isp_blocks_cfg.h"

cmr_s32 _pm_blc_init(void *dst_blc_param, void *src_blc_param, void *param1, void *param_ptr2)
{
	cmr_u32 i = 0;
	cmr_u32 index = 0;
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_blc_param *dst_ptr = (struct isp_blc_param *)dst_blc_param;
	struct sensor_blc_param *src_ptr = (struct sensor_blc_param *)src_blc_param;
	struct isp_pm_block_header *blc_header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param_ptr2);

	for (i = 0; i < SENSOR_BLC_NUM; i++) {
		dst_ptr->offset[i].r = src_ptr->tab[i].r;
		dst_ptr->offset[i].gb = src_ptr->tab[i].gb;
		dst_ptr->offset[i].b = src_ptr->tab[i].b;
		dst_ptr->offset[i].gr = src_ptr->tab[i].gr;
	}
	dst_ptr->cur_idx.x0 = src_ptr->cur_idx.x0;
	dst_ptr->cur_idx.x1 = src_ptr->cur_idx.x1;
	dst_ptr->cur_idx.weight0 = src_ptr->cur_idx.weight0;
	dst_ptr->cur_idx.weight1 = src_ptr->cur_idx.weight1;
	index = src_ptr->cur_idx.x0;

	dst_ptr->cur.bypass = blc_header_ptr->bypass;
	dst_ptr->cur.b = dst_ptr->offset[index].b;
	dst_ptr->cur.gb = dst_ptr->offset[index].gb;
	dst_ptr->cur.gr = dst_ptr->offset[index].gr;
	dst_ptr->cur.r = dst_ptr->offset[index].r;
	blc_header_ptr->is_update = ISP_ONE;
	return rtn;
}

cmr_s32 _pm_blc_set_param(void *blc_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 index = 0;
	struct isp_blc_param *blc_ptr = (struct isp_blc_param *)blc_param;
	struct isp_pm_block_header *blc_header_ptr = (struct isp_pm_block_header *)param_ptr1;

	switch (cmd) {
	case ISP_PM_BLK_BLC_OFFSET:
		index = *((cmr_u32 *) param_ptr0);
		blc_ptr->cur.r = blc_ptr->offset[index].r;
		blc_ptr->cur.gr = blc_ptr->offset[index].gr;
		blc_ptr->cur.b = blc_ptr->offset[index].b;
		blc_ptr->cur.gb = blc_ptr->offset[index].gb;
		blc_header_ptr->is_update = ISP_ONE;
		break;

	case ISP_PM_BLK_BLC_BYPASS:
		blc_ptr->cur.bypass = *((cmr_u32 *) param_ptr0);
		blc_header_ptr->is_update = ISP_ONE;
		break;

	case ISP_PM_BLK_BLC_MODE:
		blc_ptr->cur.mode = *((cmr_u32 *) param_ptr0);
		blc_header_ptr->is_update = ISP_ONE;
		break;
	case ISP_PM_BLK_SMART_SETTING:
		{
			struct smart_block_result *block_result = (struct smart_block_result *)param_ptr0;
			struct isp_weight_value *weight_value = NULL;
			struct isp_weight_value blc_value = { {0}, {0} };
			struct isp_range val_range = { 0, 0 };

			val_range.min = 0;
			val_range.max = SENSOR_BLC_NUM - 1;
			rtn = _pm_check_smart_param(block_result, &val_range, 1, ISP_SMART_Y_TYPE_WEIGHT_VALUE);
			if (ISP_SUCCESS != rtn) {
				ISP_LOGE("fail to check pm  smart param!");
				return rtn;
			}

			weight_value = (struct isp_weight_value *)block_result->component[0].fix_data;
			blc_value = *weight_value;

			blc_value.weight[0] = blc_value.weight[0] / (SMART_WEIGHT_UNIT / 16)
			    * (SMART_WEIGHT_UNIT / 16);
			blc_value.weight[1] = SMART_WEIGHT_UNIT - blc_value.weight[0];

			if (blc_value.value[0] != blc_ptr->cur_idx.x0 || blc_value.weight[0] != blc_ptr->cur_idx.weight0) {

				blc_ptr->cur.r = (blc_ptr->offset[blc_value.value[0]].r * blc_value.weight[0] +
						  blc_ptr->offset[blc_value.value[1]].r * blc_value.weight[1] + SMART_WEIGHT_UNIT / 2) / SMART_WEIGHT_UNIT;
				blc_ptr->cur.gr = (blc_ptr->offset[blc_value.value[0]].gr * blc_value.weight[0] +
						   blc_ptr->offset[blc_value.value[1]].gr * blc_value.weight[1] + SMART_WEIGHT_UNIT / 2) / SMART_WEIGHT_UNIT;
				blc_ptr->cur.gb = (blc_ptr->offset[blc_value.value[0]].gb * blc_value.weight[0] +
						   blc_ptr->offset[blc_value.value[1]].gb * blc_value.weight[1] + SMART_WEIGHT_UNIT / 2) / SMART_WEIGHT_UNIT;
				blc_ptr->cur.b = (blc_ptr->offset[blc_value.value[0]].b * blc_value.weight[0] +
						  blc_ptr->offset[blc_value.value[1]].b * blc_value.weight[1] + SMART_WEIGHT_UNIT / 2) / SMART_WEIGHT_UNIT;
				blc_ptr->cur_idx.x0 = weight_value->value[0];
				blc_ptr->cur_idx.x1 = weight_value->value[1];
				blc_ptr->cur_idx.weight0 = weight_value->weight[0];
				blc_ptr->cur_idx.weight1 = weight_value->weight[1];
				blc_header_ptr->is_update = ISP_ONE;
			}
		}
		break;

	default:

		break;
	}

	return rtn;
}

cmr_s32 _pm_blc_get_param(void *blc_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_blc_param *blc_ptr = (struct isp_blc_param *)blc_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->id = ISP_BLK_BLC;
	param_data_ptr->cmd = cmd;
	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &blc_ptr->cur;
		param_data_ptr->data_size = sizeof(blc_ptr->cur);
		*update_flag = 0;
		break;

	case ISP_PM_BLK_BLC_BYPASS:
		param_data_ptr->data_ptr = &blc_ptr->cur.bypass;
		param_data_ptr->data_size = sizeof(blc_ptr->cur.bypass);
		break;

		//this is the test code.yongheng.lu add
	case ISP_PM_BLK_BLC_OFFSET_GB:
		param_data_ptr->data_ptr = &blc_ptr->cur.gb;
		param_data_ptr->data_size = sizeof(blc_ptr->cur.gb);
		break;

	default:
		break;
	}

	return rtn;
}
