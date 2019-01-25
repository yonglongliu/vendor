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
#define LOG_TAG "isp_blk_cmc10"
#include "isp_blocks_cfg.h"

cmr_s32 _pm_cmc10_init(void *dst_cmc10_param, void *src_cmc10_param, void *param1, void *param_ptr2)
{
	cmr_u32 i = 0;
	cmr_u32 j = 0;
	cmr_u32 index = 0;
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_cmc10_param *dst_ptr = (struct isp_cmc10_param *)dst_cmc10_param;
	struct sensor_cmc10_param *src_ptr = (struct sensor_cmc10_param *)src_cmc10_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param_ptr2);

	dst_ptr->cur_idx_info.x0 = src_ptr->cur_idx.x0;
	dst_ptr->cur_idx_info.x1 = src_ptr->cur_idx.x1;
	dst_ptr->cur_idx_info.weight0 = src_ptr->cur_idx.weight0;
	dst_ptr->cur_idx_info.weight1 = src_ptr->cur_idx.weight1;
	index = src_ptr->cur_idx.x0;

	for (i = 0; i < SENSOR_CMC_NUM; i++) {
		for (j = 0; j < SENSOR_CMC_POINT_NUM; j++) {
			dst_ptr->matrix[i][j] = src_ptr->matrix[i][j];
		}
	}

	dst_ptr->cur.bypass = header_ptr->bypass;

	for (j = 0; j < SENSOR_CMC_POINT_NUM; j++) {
		dst_ptr->cur.matrix.val[j] = src_ptr->matrix[index][j];
	}

	header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_cmc10_adjust(struct isp_cmc10_param * cmc_ptr, cmr_u32 is_reduce)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u16 *cmc_matrix_x0 = NULL;
	cmr_u16 *cmc_matrix_x1 = NULL;
	cmr_u32 alpha_x0 = 0x00;
	cmr_u32 alpha_x1 = 0x00;
	void *src_matrix[2] = { NULL };
	cmr_u16 weight[2] = { 0 };
	cmr_u16 interp_result[SENSOR_CMC_POINT_NUM] = { 0 };
	cmr_u32 i = 0;

	if ((cmc_ptr->cur_idx_info.x0 >= ISP_CMC_NUM) || (cmc_ptr->cur_idx_info.x1 >= ISP_CMC_NUM)) {
		rtn = ISP_ERROR;
		return rtn;
	}

	src_matrix[0] = cmc_ptr->matrix[cmc_ptr->cur_idx_info.x0];
	src_matrix[1] = cmc_ptr->matrix[cmc_ptr->cur_idx_info.x1];
	weight[0] = cmc_ptr->cur_idx_info.weight0;
	weight[1] = cmc_ptr->cur_idx_info.weight1;

	/*interplate alg. */
	rtn = isp_interp_data(interp_result, src_matrix, weight, 9, ISP_INTERP_INT14);
	if (ISP_SUCCESS != rtn) {
		rtn = ISP_ERROR;
		return rtn;
	}

	if (ISP_ONE == is_reduce) {
		isp_cmc_adjust_4_reduce_saturation(interp_result, cmc_ptr->result_cmc, cmc_ptr->reduce_percent);
	} else {
		memcpy(cmc_ptr->result_cmc, interp_result, sizeof(cmc_ptr->result_cmc));
	}

	for (i = 0; i < SENSOR_CMC_POINT_NUM; i++) {
		cmc_ptr->cur.matrix.val[i] = cmc_ptr->result_cmc[i];
	}

	return rtn;
}

cmr_s32 _pm_cmc10_set_param(void *cmc10_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_cmc10_param *cmc10_ptr = (struct isp_cmc10_param *)cmc10_param;
	struct isp_pm_block_header *cmc10_header_ptr = (struct isp_pm_block_header *)param_ptr1;

	switch (cmd) {
	case ISP_PM_BLK_CMC10_BYPASS:
		cmc10_ptr->cur.bypass = *((cmr_u32 *) param_ptr0);
		cmc10_header_ptr->is_update = ISP_ONE;
		break;

	case ISP_PM_BLK_CMC10:
		memcpy(cmc10_ptr->cur.matrix.val, param_ptr0, 9 * sizeof(cmr_u16));
		cmc10_header_ptr->is_update = ISP_ONE;
		break;

	case ISP_PM_BLK_SMART_SETTING:
		{
			struct smart_block_result *block_result = (struct smart_block_result *)param_ptr0;
			struct isp_weight_value cmc_value = { {0}, {0} };
			cmr_u32 is_reduce = ISP_ZERO;
			struct isp_range val_range = { 0, 0 };
			cmr_u32 update = 0;

			if (NULL == block_result) {
				return ISP_ERROR;
			}

			if (0 == block_result->update) {
				return rtn;
			}

			if (ISP_SMART_CMC == block_result->smart_id) {

				struct isp_weight_value *weight_value = NULL;

				val_range.min = 0;
				val_range.max = ISP_CMC_NUM - 1;
				rtn = _pm_check_smart_param(block_result, &val_range, 1, ISP_SMART_Y_TYPE_WEIGHT_VALUE);

				if (ISP_SUCCESS != rtn) {
					ISP_LOGE("fail to check pm smart param !");
					return rtn;
				}

				weight_value = (struct isp_weight_value *)block_result->component[0].fix_data;

				struct isp_weight_value *bv_value = &weight_value[0];
				struct isp_weight_value *ct_value[2] = { &weight_value[1], &weight_value[2] };

				cmr_u16 bv_result[SENSOR_CMC_POINT_NUM] = { 0 };
				cmr_u16 ct_result[2][SENSOR_CMC_POINT_NUM] = { {0}, {0} };
				cmr_s32 i;
				for (i = 0; i < 2; i++) {
					void *src_matrix[2] = { NULL };
					src_matrix[0] = cmc10_ptr->matrix[ct_value[i]->value[0]];
					src_matrix[1] = cmc10_ptr->matrix[ct_value[i]->value[1]];

					cmr_u16 weight[2] = { 0 };
					weight[0] = ct_value[i]->weight[0];
					weight[1] = ct_value[i]->weight[1];
					weight[0] = weight[0] / (SMART_WEIGHT_UNIT / 16) * (SMART_WEIGHT_UNIT / 16);
					weight[1] = SMART_WEIGHT_UNIT - weight[0];

					isp_interp_data(ct_result[i], src_matrix, weight, 9, ISP_INTERP_INT14);
				}

				void *src_matrix[2] = { NULL };
				cmr_u16 flash_result[SENSOR_CMC_POINT_NUM] = { 0 };
				cmr_u16 tmp_result[SENSOR_CMC_POINT_NUM] = { 0 };
				src_matrix[0] = &ct_result[0][0];
				src_matrix[1] = &ct_result[1][0];

				cmr_u16 weight[2] = { 0 };
				weight[0] = bv_value->weight[0];
				weight[1] = bv_value->weight[1];
				weight[0] = weight[0] / (SMART_WEIGHT_UNIT / 16) * (SMART_WEIGHT_UNIT / 16);
				weight[1] = SMART_WEIGHT_UNIT - weight[0];

				isp_interp_data(bv_result, src_matrix, weight, 9, ISP_INTERP_INT14);

				if (block_result->component[0].flash == 1) {
					src_matrix[0] = cmc10_ptr->matrix[weight_value[3].value[0]];
					src_matrix[1] = cmc10_ptr->matrix[weight_value[3].value[1]];
					weight[0] = weight_value[3].weight[0];
					weight[1] = weight_value[3].weight[1];
					weight[0] = weight[0] / (SMART_WEIGHT_UNIT / 16) * (SMART_WEIGHT_UNIT / 16);
					weight[1] = SMART_WEIGHT_UNIT - weight[0];
					isp_interp_data(flash_result, src_matrix, weight, 9, ISP_INTERP_INT14);

					memcpy(tmp_result, bv_result, sizeof(cmc10_ptr->result_cmc));

					src_matrix[0] = &flash_result[0];
					src_matrix[1] = &tmp_result[0];
					weight[0] = weight_value[4].weight[0];
					weight[1] = weight_value[4].weight[1];
					weight[0] = weight[0] / (SMART_WEIGHT_UNIT / 16) * (SMART_WEIGHT_UNIT / 16);
					weight[1] = SMART_WEIGHT_UNIT - weight[0];
					isp_interp_data(bv_result, src_matrix, weight, 9, ISP_INTERP_INT14);
				}

				memcpy(cmc10_ptr->result_cmc, bv_result, sizeof(cmc10_ptr->result_cmc));
				for (i = 0; i < SENSOR_CMC_POINT_NUM; i++) {
					cmc10_ptr->cur.matrix.val[i] = cmc10_ptr->result_cmc[i];
				}

				cmc10_header_ptr->is_update = ISP_ONE;

			} else if (ISP_SMART_SATURATION_DEPRESS == block_result->smart_id) {

				val_range.min = 1;
				val_range.max = 255;
				rtn = _pm_check_smart_param(block_result, &val_range, 1, ISP_SMART_Y_TYPE_VALUE);
				if (ISP_SUCCESS != rtn) {
					ISP_LOGE("fail to check pm smart param !");
					return rtn;
				}

				cmc10_ptr->reduce_percent = block_result->component[0].fix_data[0];
				is_reduce = 1;
				update = 1;

				_pm_cmc10_adjust(cmc10_ptr, is_reduce);
				cmc10_header_ptr->is_update = ISP_ONE;
			}
		}
		break;

	default:
		break;
	}

	ISP_LOGV("ISP_SMART: cmd = %d, is_update =%d, reduce_percent=%d", cmd, cmc10_header_ptr->is_update, cmc10_ptr->reduce_percent);

	return rtn;
}

cmr_s32 _pm_cmc10_get_param(void *cmc10_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_cmc10_param *cmc10_ptr = (struct isp_cmc10_param *)cmc10_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->id = ISP_BLK_CMC10;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &cmc10_ptr->cur;
		param_data_ptr->data_size = sizeof(cmc10_ptr->cur);
		*update_flag = 0;
		break;

      case ISP_PM_BLK_CMC10:
		param_data_ptr->data_ptr = cmc10_ptr->cur.matrix.val;
		param_data_ptr->data_size = 9*sizeof(cmr_u16);
		*update_flag = 0;
		break;

	default:
		break;
	}

	return rtn;
}
