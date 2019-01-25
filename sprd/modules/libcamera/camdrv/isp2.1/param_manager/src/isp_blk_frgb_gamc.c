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
#define LOG_TAG "isp_blk_frgb_gamc"
#include "isp_blocks_cfg.h"

cmr_s32 _pm_frgb_gamc_init(void *dst_gamc_param, void *src_gamc_param, void *param1, void *param_ptr2)
{
	cmr_u32 i = 0;
	cmr_u32 j = 0;
	cmr_u32 index = 0;
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_frgb_gamc_param *dst_ptr = (struct isp_frgb_gamc_param *)dst_gamc_param;
	struct sensor_frgb_gammac_param *src_ptr = (struct sensor_frgb_gammac_param *)src_gamc_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param_ptr2);

	if (1 == header_ptr->param_id) {
		for (i = 0; i < SENSOR_GAMMA_NUM; i++) {
			for (j = 0; j < SENSOR_GAMMA_POINT_NUM; j++) {
				dst_ptr->curve_tab[i].points_r[j].x = src_ptr->curve_tab[i].points_r[j].x;
				dst_ptr->curve_tab[i].points_r[j].y = src_ptr->curve_tab[i].points_r[j].y;
				dst_ptr->curve_tab[i].points_g[j].x = src_ptr->curve_tab[i].points_g[j].x;
				dst_ptr->curve_tab[i].points_g[j].y = src_ptr->curve_tab[i].points_g[j].y;
				dst_ptr->curve_tab[i].points_b[j].x = src_ptr->curve_tab[i].points_b[j].x;
				dst_ptr->curve_tab[i].points_b[j].y = src_ptr->curve_tab[i].points_b[j].y;
			}
		}
	} else {
		for (i = 0; i < SENSOR_GAMMA_NUM; i++) {
			for (j = 0; j < SENSOR_GAMMA_POINT_NUM; j++) {
				dst_ptr->curve_tab[i].points_r[j].x = src_ptr->curve_tab[i].points_r[j].x;
				dst_ptr->curve_tab[i].points_r[j].y = src_ptr->curve_tab[i].points_r[j].y;
				dst_ptr->curve_tab[i].points_g[j].x = src_ptr->curve_tab[i].points_r[j].x;
				dst_ptr->curve_tab[i].points_g[j].y = src_ptr->curve_tab[i].points_r[j].y;
				dst_ptr->curve_tab[i].points_b[j].x = src_ptr->curve_tab[i].points_r[j].x;
				dst_ptr->curve_tab[i].points_b[j].y = src_ptr->curve_tab[i].points_r[j].y;
			}
		}
	}

	dst_ptr->cur_idx.x0 = src_ptr->cur_idx_info.x0;
	dst_ptr->cur_idx.x1 = src_ptr->cur_idx_info.x1;
	dst_ptr->cur_idx.weight0 = src_ptr->cur_idx_info.weight0;
	dst_ptr->cur_idx.weight1 = src_ptr->cur_idx_info.weight1;
	index = src_ptr->cur_idx_info.x0;
	dst_ptr->cur.bypass = header_ptr->bypass;

	if (1 == header_ptr->param_id) {
		for (j = 0; j < ISP_PINGPANG_FRGB_GAMC_NUM; j++) {
			dst_ptr->cur.gamc_nodes.nodes_r[j].node_x = src_ptr->curve_tab[index].points_r[j].x;
			dst_ptr->cur.gamc_nodes.nodes_r[j].node_y = src_ptr->curve_tab[index].points_r[j].y;
			dst_ptr->cur.gamc_nodes.nodes_g[j].node_x = src_ptr->curve_tab[index].points_g[j].x;
			dst_ptr->cur.gamc_nodes.nodes_g[j].node_y = src_ptr->curve_tab[index].points_g[j].y;
			dst_ptr->cur.gamc_nodes.nodes_b[j].node_x = src_ptr->curve_tab[index].points_b[j].x;
			dst_ptr->cur.gamc_nodes.nodes_b[j].node_y = src_ptr->curve_tab[index].points_b[j].y;
		}
	} else {
		for (j = 0; j < ISP_PINGPANG_FRGB_GAMC_NUM; j++) {
			dst_ptr->cur.gamc_nodes.nodes_r[j].node_x = src_ptr->curve_tab[index].points_r[j].x;
			dst_ptr->cur.gamc_nodes.nodes_r[j].node_y = src_ptr->curve_tab[index].points_r[j].y;
			dst_ptr->cur.gamc_nodes.nodes_g[j].node_x = src_ptr->curve_tab[index].points_r[j].x;
			dst_ptr->cur.gamc_nodes.nodes_g[j].node_y = src_ptr->curve_tab[index].points_r[j].y;
			dst_ptr->cur.gamc_nodes.nodes_b[j].node_x = src_ptr->curve_tab[index].points_r[j].x;
			dst_ptr->cur.gamc_nodes.nodes_b[j].node_y = src_ptr->curve_tab[index].points_r[j].y;
		}
	}

	header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_frgb_gamc_set_param(void *gamc_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_frgb_gamc_param *gamc_ptr = (struct isp_frgb_gamc_param *)gamc_param;
	struct isp_pm_block_header *gamc_header_ptr = (struct isp_pm_block_header *)param_ptr1;

	switch (cmd) {
	case ISP_PM_BLK_GAMMA_BYPASS:
		gamc_ptr->cur.bypass = *((cmr_u32 *) param_ptr0);
		gamc_header_ptr->is_update = ISP_ONE;
		break;

	case ISP_PM_BLK_GAMMA:
		{
			cmr_u32 gamma_idx = *((cmr_u32 *) param_ptr0);
			cmr_u32 i;

			gamc_ptr->cur_idx.x0 = gamma_idx;
			gamc_ptr->cur_idx.x1 = gamma_idx;
			gamc_ptr->cur_idx.weight0 = 256;
			gamc_ptr->cur_idx.weight1 = 0;

			if (1 == gamc_header_ptr->param_id) {
				for (i = 0; i < ISP_PINGPANG_FRGB_GAMC_NUM; i++) {
					gamc_ptr->cur.gamc_nodes.nodes_r[i].node_x = gamc_ptr->curve_tab[gamma_idx].points_r[i].x;
					gamc_ptr->cur.gamc_nodes.nodes_r[i].node_y = gamc_ptr->curve_tab[gamma_idx].points_r[i].y;
					gamc_ptr->cur.gamc_nodes.nodes_g[i].node_x = gamc_ptr->curve_tab[gamma_idx].points_g[i].x;
					gamc_ptr->cur.gamc_nodes.nodes_g[i].node_y = gamc_ptr->curve_tab[gamma_idx].points_g[i].y;
					gamc_ptr->cur.gamc_nodes.nodes_b[i].node_x = gamc_ptr->curve_tab[gamma_idx].points_b[i].x;
					gamc_ptr->cur.gamc_nodes.nodes_b[i].node_y = gamc_ptr->curve_tab[gamma_idx].points_b[i].y;
				}
			} else {
				for (i = 0; i < ISP_PINGPANG_FRGB_GAMC_NUM; i++) {
					gamc_ptr->cur.gamc_nodes.nodes_r[i].node_x = gamc_ptr->curve_tab[gamma_idx].points_r[i].x;
					gamc_ptr->cur.gamc_nodes.nodes_r[i].node_y = gamc_ptr->curve_tab[gamma_idx].points_r[i].y;
					gamc_ptr->cur.gamc_nodes.nodes_g[i].node_x = gamc_ptr->curve_tab[gamma_idx].points_r[i].x;
					gamc_ptr->cur.gamc_nodes.nodes_g[i].node_y = gamc_ptr->curve_tab[gamma_idx].points_r[i].y;
					gamc_ptr->cur.gamc_nodes.nodes_b[i].node_x = gamc_ptr->curve_tab[gamma_idx].points_r[i].x;
					gamc_ptr->cur.gamc_nodes.nodes_b[i].node_y = gamc_ptr->curve_tab[gamma_idx].points_r[i].y;
				}

			}
			gamc_header_ptr->is_update = ISP_ONE;
		}

		break;

	case ISP_PM_BLK_GAMMA_CUR:
		{
			cmr_u32 i;
			memcpy((void *)&gamc_ptr->final_curve, param_ptr0, sizeof(gamc_ptr->final_curve));

			gamc_ptr->cur_idx.x0 = 0;
			gamc_ptr->cur_idx.x1 = 0;
			gamc_ptr->cur_idx.weight0 = 256;
			gamc_ptr->cur_idx.weight1 = 0;

			if (1 == gamc_header_ptr->param_id) {
				for (i = 0; i < ISP_PINGPANG_FRGB_GAMC_NUM; i++) {
					gamc_ptr->cur.gamc_nodes.nodes_r[i].node_x = gamc_ptr->final_curve.points_r[i].x;
					gamc_ptr->cur.gamc_nodes.nodes_r[i].node_y = gamc_ptr->final_curve.points_r[i].y;
					gamc_ptr->cur.gamc_nodes.nodes_g[i].node_x = gamc_ptr->final_curve.points_g[i].x;
					gamc_ptr->cur.gamc_nodes.nodes_g[i].node_y = gamc_ptr->final_curve.points_g[i].y;
					gamc_ptr->cur.gamc_nodes.nodes_b[i].node_x = gamc_ptr->final_curve.points_b[i].x;
					gamc_ptr->cur.gamc_nodes.nodes_b[i].node_y = gamc_ptr->final_curve.points_b[i].y;
				}
			} else {
				for (i = 0; i < ISP_PINGPANG_FRGB_GAMC_NUM; i++) {
					gamc_ptr->cur.gamc_nodes.nodes_r[i].node_x = gamc_ptr->final_curve.points_r[i].x;
					gamc_ptr->cur.gamc_nodes.nodes_r[i].node_y = gamc_ptr->final_curve.points_r[i].y;
					gamc_ptr->cur.gamc_nodes.nodes_g[i].node_x = gamc_ptr->final_curve.points_r[i].x;
					gamc_ptr->cur.gamc_nodes.nodes_g[i].node_y = gamc_ptr->final_curve.points_r[i].y;
					gamc_ptr->cur.gamc_nodes.nodes_b[i].node_x = gamc_ptr->final_curve.points_r[i].x;
					gamc_ptr->cur.gamc_nodes.nodes_b[i].node_y = gamc_ptr->final_curve.points_r[i].y;
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

			if (gamc_value.value[0] != gamc_ptr->cur_idx.x0 || gamc_value.weight[0] != gamc_ptr->cur_idx.weight0) {

				if (1 == gamc_header_ptr->param_id) {
					void *src_r[2] = { NULL };
					void *src_g[2] = { NULL };
					void *src_b[2] = { NULL };
					void *dst_r = NULL;
					void *dst_g = NULL;
					void *dst_b = NULL;
					src_r[0] = &gamc_ptr->curve_tab[gamc_value.value[0]].points_r[0].x;
					src_r[1] = &gamc_ptr->curve_tab[gamc_value.value[1]].points_r[0].x;
					src_g[0] = &gamc_ptr->curve_tab[gamc_value.value[0]].points_g[0].x;
					src_g[1] = &gamc_ptr->curve_tab[gamc_value.value[1]].points_g[0].x;
					src_b[0] = &gamc_ptr->curve_tab[gamc_value.value[0]].points_b[0].x;
					src_b[1] = &gamc_ptr->curve_tab[gamc_value.value[1]].points_b[0].x;
					dst_r = &gamc_ptr->final_curve.points_r;
					dst_g = &gamc_ptr->final_curve.points_g;
					dst_b = &gamc_ptr->final_curve.points_b;

					rtn = isp_interp_data(dst_r, src_r, gamc_value.weight, SENSOR_GAMMA_POINT_NUM * 2, ISP_INTERP_UINT16);
					if (ISP_SUCCESS != rtn) {
						ISP_LOGE("fail to interp gamc_r data\n");
						return rtn;
					}
					rtn = isp_interp_data(dst_g, src_g, gamc_value.weight, SENSOR_GAMMA_POINT_NUM * 2, ISP_INTERP_UINT16);
					if (ISP_SUCCESS != rtn) {
						ISP_LOGE("fail to interp gamc_g data\n");
						return rtn;
					}
					rtn = isp_interp_data(dst_b, src_b, gamc_value.weight, SENSOR_GAMMA_POINT_NUM * 2, ISP_INTERP_UINT16);
					if (ISP_SUCCESS == rtn) {
						gamc_ptr->cur_idx.x0 = weight_value->value[0];
						gamc_ptr->cur_idx.x1 = weight_value->value[1];
						gamc_ptr->cur_idx.weight0 = weight_value->weight[0];
						gamc_ptr->cur_idx.weight1 = weight_value->weight[1];
						gamc_header_ptr->is_update = ISP_ONE;

						for (i = 0; i < ISP_PINGPANG_FRGB_GAMC_NUM; i++) {
							gamc_ptr->cur.gamc_nodes.nodes_r[i].node_x = gamc_ptr->final_curve.points_r[i].x;
							gamc_ptr->cur.gamc_nodes.nodes_r[i].node_y = gamc_ptr->final_curve.points_r[i].y;
							gamc_ptr->cur.gamc_nodes.nodes_g[i].node_x = gamc_ptr->final_curve.points_g[i].x;
							gamc_ptr->cur.gamc_nodes.nodes_g[i].node_y = gamc_ptr->final_curve.points_g[i].y;
							gamc_ptr->cur.gamc_nodes.nodes_b[i].node_x = gamc_ptr->final_curve.points_b[i].x;
							gamc_ptr->cur.gamc_nodes.nodes_b[i].node_y = gamc_ptr->final_curve.points_b[i].y;
						}
					}
				} else {
					void *src[2] = { NULL };
					void *dst = NULL;
					src[0] = &gamc_ptr->curve_tab[gamc_value.value[0]].points_r[0].x;
					src[1] = &gamc_ptr->curve_tab[gamc_value.value[1]].points_r[0].x;
					dst = &gamc_ptr->final_curve.points_r;

					rtn = isp_interp_data(dst, src, gamc_value.weight, SENSOR_GAMMA_POINT_NUM * 2, ISP_INTERP_UINT16);
					if (ISP_SUCCESS == rtn) {
						gamc_ptr->cur_idx.x0 = weight_value->value[0];
						gamc_ptr->cur_idx.x1 = weight_value->value[1];
						gamc_ptr->cur_idx.weight0 = weight_value->weight[0];
						gamc_ptr->cur_idx.weight1 = weight_value->weight[1];
						gamc_header_ptr->is_update = ISP_ONE;

						for (i = 0; i < ISP_PINGPANG_FRGB_GAMC_NUM; i++) {
							gamc_ptr->cur.gamc_nodes.nodes_r[i].node_x = gamc_ptr->final_curve.points_r[i].x;
							gamc_ptr->cur.gamc_nodes.nodes_r[i].node_y = gamc_ptr->final_curve.points_r[i].y;
							gamc_ptr->cur.gamc_nodes.nodes_g[i].node_x = gamc_ptr->final_curve.points_r[i].x;
							gamc_ptr->cur.gamc_nodes.nodes_g[i].node_y = gamc_ptr->final_curve.points_r[i].y;
							gamc_ptr->cur.gamc_nodes.nodes_b[i].node_x = gamc_ptr->final_curve.points_r[i].x;
							gamc_ptr->cur.gamc_nodes.nodes_b[i].node_y = gamc_ptr->final_curve.points_r[i].y;
						}
					}
				}
			}
		}
		break;

	default:
		break;
	}
	ISP_LOGV("cmd=%d, update=%d, value=(%d, %d), weight=(%d, %d)\n", cmd, gamc_header_ptr->is_update,
		 gamc_ptr->cur_idx.x0, gamc_ptr->cur_idx.x1, gamc_ptr->cur_idx.weight0, gamc_ptr->cur_idx.weight1);

	return rtn;
}

cmr_s32 _pm_frgb_gamc_get_param(void *gamc_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_frgb_gamc_param *gamc_ptr = (struct isp_frgb_gamc_param *)gamc_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->id = ISP_BLK_RGB_GAMC;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &gamc_ptr->cur;
		param_data_ptr->data_size = sizeof(gamc_ptr->cur);
		*update_flag = 0;
		break;

	case ISP_PM_BLK_GAMMA:
		param_data_ptr->data_ptr = &gamc_ptr->cur.gamc_nodes;
		param_data_ptr->data_size = sizeof(gamc_ptr->cur.gamc_nodes);
		*update_flag = 0;
		break;

	case ISP_PM_BLK_GAMMA_TAB:
		param_data_ptr->data_ptr = &gamc_ptr->curve_tab[0];
		param_data_ptr->data_size = sizeof(gamc_ptr->curve_tab);
		break;

	default:
		break;
	}

	return rtn;
}
