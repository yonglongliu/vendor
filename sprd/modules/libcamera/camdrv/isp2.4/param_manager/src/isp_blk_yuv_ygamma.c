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
#define LOG_TAG "isp_blk_ygamc"
#include "isp_blocks_cfg.h"

cmr_s32 _pm_yuv_ygamma_init(void *dst_gamc_param, void *src_gamc_param, void *param1, void *param_ptr2)
{
	cmr_u32 i = 0;
	cmr_u32 j = 0;
	cmr_u32 index = 0;
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_yuv_ygamma_param *dst_ptr = (struct isp_yuv_ygamma_param *)dst_gamc_param;
	struct sensor_y_gamma_param *src_ptr = (struct sensor_y_gamma_param *)src_gamc_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param_ptr2);

	for (i = 0; i < SENSOR_GAMMA_NUM; i++) {
		for (j = 0; j < SENSOR_GAMMA_POINT_NUM; j++) {
			dst_ptr->curve_tab[i].points[j].x = src_ptr->curve_tab[i].points[j].x;
			dst_ptr->curve_tab[i].points[j].y = src_ptr->curve_tab[i].points[j].y;
		}
	}

	dst_ptr->cur_idx = src_ptr->cur_idx;
	dst_ptr->cur.bypass = header_ptr->bypass;
	index = src_ptr->cur_idx;

	for (j = 0; j < ISP_PINGPANG_YUV_YGAMMA_NUM; j++) {
		if (j < ISP_PINGPANG_YUV_YGAMMA_NUM - 1) {
			dst_ptr->cur.nodes[j].node_x = (src_ptr->curve_tab[index].points[j * 2].x + src_ptr->curve_tab[index].points[j * 2 + 1].x) >> 1;
			dst_ptr->cur.nodes[j].node_y = (src_ptr->curve_tab[index].points[j * 2].y + src_ptr->curve_tab[index].points[j * 2 + 1].y) >> 1;
		} else {
			dst_ptr->cur.nodes[j].node_x = src_ptr->curve_tab[index].points[j * 2 - 1].x;
			dst_ptr->cur.nodes[j].node_y = src_ptr->curve_tab[index].points[j * 2 - 1].y;
		}
	}

	for (i = 0; i < MAX_SPECIALEFFECT_NUM; i++) {
		for (j = 0; j < SENSOR_GAMMA_POINT_NUM; j++) {
			dst_ptr->specialeffect_tab[i].points[j].x = src_ptr->specialeffect[i].points[j].x;
			dst_ptr->specialeffect_tab[i].points[j].y = src_ptr->specialeffect[i].points[j].y;
		}
	}

	header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_yuv_ygamma_set_param(void *gamc_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_yuv_ygamma_param *ygamma_ptr = (struct isp_yuv_ygamma_param *)gamc_param;
	struct isp_pm_block_header *gamc_header_ptr = (struct isp_pm_block_header *)param_ptr1;

	switch (cmd) {
	case ISP_PM_BLK_YGAMMA_BYPSS:
		ygamma_ptr->cur.bypass = *((cmr_u32 *) param_ptr0);
		gamc_header_ptr->is_update = ISP_ONE;
		break;

	case ISP_PM_BLK_YGAMMA:
		{
			cmr_u32 gamma_idx = *((cmr_u32 *) param_ptr0);
			cmr_u32 i;

			ygamma_ptr->cur_idx_weight.x0 = gamma_idx;
			ygamma_ptr->cur_idx_weight.x1 = gamma_idx;
			ygamma_ptr->cur_idx_weight.weight0 = 256;
			ygamma_ptr->cur_idx_weight.weight1 = 0;
			for (i = 0; i < ISP_PINGPANG_YUV_YGAMMA_NUM; i++) {
				if (i < ISP_PINGPANG_YUV_YGAMMA_NUM - 1) {
					ygamma_ptr->cur.nodes[i].node_x =
					    (ygamma_ptr->curve_tab[gamma_idx].points[i * 2].x + ygamma_ptr->curve_tab[gamma_idx].points[i * 2 + 1].x) >> 1;
					ygamma_ptr->cur.nodes[i].node_y =
					    (ygamma_ptr->curve_tab[gamma_idx].points[i * 2].y + ygamma_ptr->curve_tab[gamma_idx].points[i * 2 + 1].y) >> 1;
				} else {
					ygamma_ptr->cur.nodes[i].node_x = ygamma_ptr->curve_tab[gamma_idx].points[i * 2 - 1].x;
					ygamma_ptr->cur.nodes[i].node_y = ygamma_ptr->curve_tab[gamma_idx].points[i * 2 - 1].y;
				}
			}
			gamc_header_ptr->is_update = ISP_ONE;
		}

		break;

	case ISP_PM_BLK_SPECIAL_EFFECT:
		{
			cmr_u32 idx = *((cmr_u32 *) param_ptr0);
			if (0 == idx) {
				cmr_s32 j = 0;
				for (j = 0; j < ISP_PINGPANG_YUV_YGAMMA_NUM; j++) {
					if (j < ISP_PINGPANG_YUV_YGAMMA_NUM - 1) {
						ygamma_ptr->cur.nodes[j].node_x =
						    (ygamma_ptr->curve_tab[ygamma_ptr->cur_idx].points[j * 2].x +
						     ygamma_ptr->curve_tab[ygamma_ptr->cur_idx].points[j * 2 + 1].x) >> 1;
						ygamma_ptr->cur.nodes[j].node_y =
						    (ygamma_ptr->curve_tab[ygamma_ptr->cur_idx].points[j * 2].y +
						     ygamma_ptr->curve_tab[ygamma_ptr->cur_idx].points[j * 2 + 1].y) >> 1;
					} else {
						ygamma_ptr->cur.nodes[j].node_x = ygamma_ptr->curve_tab[ygamma_ptr->cur_idx].points[j * 2 - 1].x;
						ygamma_ptr->cur.nodes[j].node_y = ygamma_ptr->curve_tab[ygamma_ptr->cur_idx].points[j * 2 - 1].y;
					}
				}
			} else {
				cmr_s32 j = 0;
				for (j = 0; j < ISP_PINGPANG_YUV_YGAMMA_NUM; j++) {
					if (j < ISP_PINGPANG_YUV_YGAMMA_NUM - 1) {
						ygamma_ptr->cur.nodes[j].node_x =
						    (ygamma_ptr->specialeffect_tab[idx].points[j * 2].x + ygamma_ptr->specialeffect_tab[idx].points[j * 2 + 1].x) >> 1;
						ygamma_ptr->cur.nodes[j].node_y =
						    (ygamma_ptr->specialeffect_tab[idx].points[j * 2].y + ygamma_ptr->specialeffect_tab[idx].points[j * 2 + 1].y) >> 1;
					} else {
						ygamma_ptr->cur.nodes[j].node_x = ygamma_ptr->specialeffect_tab[idx].points[j * 2 - 1].x;
						ygamma_ptr->cur.nodes[j].node_y = ygamma_ptr->specialeffect_tab[idx].points[j * 2 - 1].y;
					}
				}
			}
			gamc_header_ptr->is_update = ISP_ONE;
		}
		break;

	case ISP_PM_BLK_SMART_SETTING:
		{
			struct smart_block_result *block_result = (struct smart_block_result *)param_ptr0;
			struct isp_weight_value *weight_value = NULL;
			struct isp_weight_value gamc_value = { {0}, {0} };
			struct isp_range val_range = { 0, 0 };
			cmr_s32 i;

			val_range.min = 0;
			val_range.max = SENSOR_GAMMA_NUM - 1;
			rtn = _pm_check_smart_param(block_result, &val_range, 1, ISP_SMART_Y_TYPE_WEIGHT_VALUE);
			if (ISP_SUCCESS != rtn) {
				ISP_LOGE("fail to check pm smart param !");
				return rtn;
			}

			weight_value = (struct isp_weight_value *)block_result->component[0].fix_data;
			gamc_value = *weight_value;

			gamc_value.weight[0] = gamc_value.weight[0] / (SMART_WEIGHT_UNIT / 16)
			    * (SMART_WEIGHT_UNIT / 16);
			gamc_value.weight[1] = SMART_WEIGHT_UNIT - gamc_value.weight[0];
			if (gamc_value.value[0] != ygamma_ptr->cur_idx_weight.x0 || gamc_value.weight[0] != ygamma_ptr->cur_idx_weight.weight0) {

				void *src[2] = { NULL };
				void *dst = NULL;
				src[0] = &ygamma_ptr->curve_tab[gamc_value.value[0]].points[0].x;
				src[1] = &ygamma_ptr->curve_tab[gamc_value.value[1]].points[0].x;
				dst = &ygamma_ptr->final_curve;

				rtn = isp_interp_data(dst, src, gamc_value.weight, SENSOR_GAMMA_POINT_NUM * 2, ISP_INTERP_UINT16);
				if (ISP_SUCCESS == rtn) {
					ygamma_ptr->cur_idx_weight.x0 = weight_value->value[0];
					ygamma_ptr->cur_idx_weight.x1 = weight_value->value[1];
					ygamma_ptr->cur_idx_weight.weight0 = weight_value->weight[0];
					ygamma_ptr->cur_idx_weight.weight1 = weight_value->weight[1];

					if (ygamma_ptr->cur.bypass) {
						gamc_header_ptr->is_update = ISP_ZERO;
					} else {
						gamc_header_ptr->is_update = ISP_ONE;
					}

					for (i = 0; i < ISP_PINGPANG_YUV_YGAMMA_NUM; i++) {
						if (i < ISP_PINGPANG_YUV_YGAMMA_NUM - 1) {
							ygamma_ptr->cur.nodes[i].node_x =
							    (ygamma_ptr->final_curve.points[i * 2].x + ygamma_ptr->final_curve.points[i * 2 + 1].x) >> 1;
							ygamma_ptr->cur.nodes[i].node_y =
							    (ygamma_ptr->final_curve.points[i * 2].y + ygamma_ptr->final_curve.points[i * 2 + 1].y) >> 1;
						} else {
							ygamma_ptr->cur.nodes[i].node_x = ygamma_ptr->final_curve.points[i * 2 - 1].x;
							ygamma_ptr->cur.nodes[i].node_y = ygamma_ptr->final_curve.points[i * 2 - 1].y;
						}
					}
				}
			}
		}
		break;

	default:
		break;
	}

	return rtn;
}

cmr_s32 _pm_yuv_ygamma_get_param(void *gamc_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_yuv_ygamma_param *gamc_ptr = (struct isp_yuv_ygamma_param *)gamc_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->id = ISP_BLK_Y_GAMMC;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &gamc_ptr->cur;
		param_data_ptr->data_size = sizeof(gamc_ptr->cur);
		*update_flag = 0;
		break;

	default:
		break;
	}

	return rtn;
}
