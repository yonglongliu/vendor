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
#define LOG_TAG "isp_blk_cce"
#include "isp_blocks_cfg.h"

cmr_s32 _pm_cce_adjust_hue_saturation(struct isp_cce_param * cce_param, cmr_u32 hue, cmr_u32 saturation)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 cce_coef_index = 0;
	cmr_u16 cce_coef[3] = { 0 };
	cmr_u32 cur_idx = 0;
	struct isp_rgb_gains rgb_gain = { 0x00, 0x00, 0x00, 0x00 };
	struct isp_cce_param *dst_cce_ptr = (struct isp_cce_param *)cce_param;
	cmr_u32 i = 0x00;
	cmr_u16 src_matrix[9] = { 0 };
	cmr_u16 dst_matrix[9] = { 0 };

	ISP_LOGV("target: hue = %d, sat= % d", hue, saturation);

	rtn = isp_hue_saturation_2_gain(hue, saturation, &rgb_gain);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to get hue saturation gain !");
		return ISP_ERROR;
	}

	cce_coef_index = ISP_CCE_COEF_COLOR_CAST;
	dst_cce_ptr->cce_coef[cce_coef_index][0] = rgb_gain.gain_r;
	dst_cce_ptr->cce_coef[cce_coef_index][1] = rgb_gain.gain_g;
	dst_cce_ptr->cce_coef[cce_coef_index][2] = rgb_gain.gain_b;

	dst_cce_ptr->cur_level[0] = hue;
	dst_cce_ptr->cur_level[1] = saturation;

	for (i = 0; i < 3; i++) {
		cmr_u32 color_cast_coef = dst_cce_ptr->cce_coef[ISP_CCE_COEF_COLOR_CAST][i];
		cmr_u32 gain_offset_coef = dst_cce_ptr->cce_coef[ISP_CCE_COEF_GAIN_OFFSET][i];

		cce_coef[i] = gain_offset_coef * color_cast_coef / ISP_FIX_8BIT_UNIT;
	}

	ISP_LOGV("multiply: %d, %d, %d", cce_coef[0], cce_coef[1], cce_coef[2]);

	cur_idx = dst_cce_ptr->cur_idx;

	for (i = 0; i < 9; ++i) {
		src_matrix[i] = (cmr_u16) dst_cce_ptr->cce_tab[cur_idx].matrix[i];
	}

	isp_cce_adjust(src_matrix, cce_coef, dst_matrix, ISP_FIX_10BIT_UNIT);

	for (i = 0; i < 9; ++i) {
		dst_cce_ptr->cur.matrix[i] = (cmr_u32) dst_matrix[i];
	}

	dst_cce_ptr->cur.y_offset = dst_cce_ptr->cce_tab[cur_idx].y_offset;
	dst_cce_ptr->cur.u_offset = dst_cce_ptr->cce_tab[cur_idx].u_offset;
	dst_cce_ptr->cur.v_offset = dst_cce_ptr->cce_tab[cur_idx].v_offset;

	return rtn;
}

cmr_s32 _pm_cce_adjust_gain_offset(struct isp_cce_param * cce_param, cmr_u16 r_gain, cmr_u16 g_gain, cmr_u16 b_gain)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 cce_coef_index = 0;
	cmr_u16 cce_coef[3] = { 0 };
	struct isp_cce_param *dst_cce_ptr = (struct isp_cce_param *)cce_param;
	cmr_u32 i = 0x00;
	cmr_u32 cur_idx = 0;
	cmr_u16 src_matrix[9] = { 0 };
	cmr_u16 dst_matrix[9] = { 0 };

	ISP_LOGV("target: gain: %d, %d, %d", r_gain, g_gain, b_gain);

	if (0 == r_gain || 0 == g_gain || 0 == b_gain) {
		r_gain = ISP_FIX_8BIT_UNIT;
		g_gain = ISP_FIX_8BIT_UNIT;
		b_gain = ISP_FIX_8BIT_UNIT;
	}

	cce_coef_index = ISP_CCE_COEF_GAIN_OFFSET;
	dst_cce_ptr->cce_coef[cce_coef_index][0] = r_gain;
	dst_cce_ptr->cce_coef[cce_coef_index][1] = g_gain;
	dst_cce_ptr->cce_coef[cce_coef_index][2] = b_gain;

	for (i = 0; i < 3; i++) {
		cmr_u32 color_cast_coef = dst_cce_ptr->cce_coef[ISP_CCE_COEF_COLOR_CAST][i];
		cmr_u32 gain_offset_coef = dst_cce_ptr->cce_coef[ISP_CCE_COEF_GAIN_OFFSET][i];

		cce_coef[i] = gain_offset_coef * color_cast_coef / ISP_FIX_8BIT_UNIT;
	}

	cur_idx = dst_cce_ptr->cur_idx;

	for (i = 0; i < 9; ++i) {
		src_matrix[i] = (cmr_u16) dst_cce_ptr->cce_tab[cur_idx].matrix[i];
	}

	isp_cce_adjust(src_matrix, cce_coef, dst_matrix, ISP_FIX_10BIT_UNIT);

	for (i = 0; i < 9; ++i) {
		dst_cce_ptr->cur.matrix[i] = (cmr_u32) dst_matrix[i];
	}

	dst_cce_ptr->cur.y_offset = dst_cce_ptr->cce_tab[cur_idx].y_offset;
	dst_cce_ptr->cur.u_offset = dst_cce_ptr->cce_tab[cur_idx].u_offset;
	dst_cce_ptr->cur.v_offset = dst_cce_ptr->cce_tab[cur_idx].v_offset;

	return rtn;
}

cmr_s32 _pm_cce_adjust(struct isp_cce_param * cce_param)
{
	cmr_s32 rtn0 = ISP_SUCCESS;
	cmr_s32 rtn = ISP_SUCCESS;

	cmr_s32 hue = cce_param->cur_level[0];
	cmr_s32 saturation = cce_param->cur_level[1];
	cmr_u16 r_gain = cce_param->cce_coef[ISP_CCE_COEF_GAIN_OFFSET][0];
	cmr_u16 g_gain = cce_param->cce_coef[ISP_CCE_COEF_GAIN_OFFSET][1];
	cmr_u16 b_gain = cce_param->cce_coef[ISP_CCE_COEF_GAIN_OFFSET][2];

	if (0 != cce_param->is_specialeffect) {
		ISP_LOGE("fail to adjust cce for specialeffect");
		return ISP_SUCCESS;
	}

	rtn0 = _pm_cce_adjust_hue_saturation(cce_param, hue, saturation);
	if (ISP_SUCCESS != rtn0) {
		ISP_LOGE("fail to adjust pm cce  hue saturation ");
		rtn = rtn0;
	}

	rtn0 = _pm_cce_adjust_gain_offset(cce_param, r_gain, g_gain, b_gain);
	if (ISP_SUCCESS != rtn0) {
		ISP_LOGE("fail to adjust pm cce gain offset ");
		rtn = rtn0;
	}

	return rtn;

}

cmr_s32 _pm_cce_init(void *dst_cce_param, void *src_cce_param, void *param1, void *param2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 i = 0x00, j = 0, index = 0;
	struct sensor_cce_param *src_ptr = (struct sensor_cce_param *)src_cce_param;
	struct isp_cce_param *dst_ptr = (struct isp_cce_param *)dst_cce_param;
	struct isp_pm_block_header *cce_header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param2);

	memset((void *)&dst_ptr->cur, 0x00, sizeof(dst_ptr->cur));
	dst_ptr->is_specialeffect = 0;

	for (i = 0; i < SENSOR_CCE_NUM; ++i) {
		for (j = 0x00; j < 9; ++j) {
			dst_ptr->cce_tab[i].matrix[j] = src_ptr->tab[i].matrix[j];
		}
		dst_ptr->cce_tab[i].u_offset = src_ptr->tab[i].u_shift;
		dst_ptr->cce_tab[i].v_offset = src_ptr->tab[i].v_shift;
		dst_ptr->cce_tab[i].y_offset = src_ptr->tab[i].y_shift;
	}
	index = src_ptr->cur_idx;

	for (i = 0; i < MAX_SPECIALEFFECT_NUM; ++i) {
		for (j = 0; j < 9; ++j) {
			dst_ptr->specialeffect_tab[i].matrix[j] = src_ptr->specialeffect[i].matrix[j];
		}
		dst_ptr->specialeffect_tab[i].y_offset = src_ptr->specialeffect[i].y_shift;
		dst_ptr->specialeffect_tab[i].u_offset = src_ptr->specialeffect[i].u_shift;
		dst_ptr->specialeffect_tab[i].v_offset = src_ptr->specialeffect[i].v_shift;
	}

	for (i = 0; i < 9; ++i) {
		dst_ptr->cur.matrix[i] = src_ptr->tab[index].matrix[i];
	}

	dst_ptr->cur.u_offset = src_ptr->tab[index].u_shift;
	dst_ptr->cur.v_offset = src_ptr->tab[index].v_shift;
	dst_ptr->cur.y_offset = src_ptr->tab[index].y_shift;

	for (i = 0; i < 2; ++i) {
		for (j = 0; j < 3; ++j) {
			if (ISP_CCE_COEF_COLOR_CAST == i) {
				dst_ptr->cce_coef[i][j] = ISP_FIX_10BIT_UNIT;
			} else if (ISP_CCE_COEF_GAIN_OFFSET == i) {
				dst_ptr->cce_coef[i][j] = ISP_FIX_8BIT_UNIT;
			}
		}
	}

	dst_ptr->cur_idx = index;
	dst_ptr->prv_idx = 0;

	_pm_cce_adjust(dst_ptr);

	cce_header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_cce_set_param(void *cce_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_cce_param *dst_ptr = (struct isp_cce_param *)cce_param;
	struct isp_pm_block_header *cce_header_ptr = (struct isp_pm_block_header *)param_ptr1;

	switch (cmd) {
	case ISP_PM_BLK_CCE:
		{
			cmr_u32 idx = 0;
			cmr_u32 i = 0x00;
			idx = *((cmr_u32 *) param_ptr0);

			for (i = 0; i < 9; ++i) {
				dst_ptr->cur.matrix[i] = dst_ptr->cce_tab[idx].matrix[i];
			}

			dst_ptr->cur.y_offset = dst_ptr->cce_tab[idx].y_offset;
			dst_ptr->cur.u_offset = dst_ptr->cce_tab[idx].u_offset;
			dst_ptr->cur.v_offset = dst_ptr->cce_tab[idx].v_offset;

			dst_ptr->cur_idx = idx;

			dst_ptr->is_specialeffect = 0;

			_pm_cce_adjust(dst_ptr);
			cce_header_ptr->is_update = ISP_ONE;
		}
		break;

	case ISP_PM_BLK_SPECIAL_EFFECT:
		{
			cmr_u32 idx = *((cmr_u32 *) param_ptr0);
			dst_ptr->prv_idx = dst_ptr->cur_idx;
			dst_ptr->cur_idx = idx;
			if (0 == idx) {
				dst_ptr->is_specialeffect = 0;
				dst_ptr->cur = dst_ptr->cce_tab[dst_ptr->cur_idx];
			} else {
				dst_ptr->is_specialeffect = 1;
				dst_ptr->cur = dst_ptr->specialeffect_tab[dst_ptr->cur_idx];
			}
			_pm_cce_adjust(dst_ptr);
			cce_header_ptr->is_update = ISP_ONE;
		}
		break;

	case ISP_PM_BLK_SMART_SETTING:
		{
			struct isp_range val_range = { 0, 0 };
			struct smart_block_result *block_result = (struct smart_block_result *)param_ptr0;

			if (block_result->update == 0) {
				return rtn;
			}

			if (dst_ptr->is_specialeffect != 0) {
				/*special effect mode, do not adjust cce */
				return ISP_SUCCESS;
			}

			if ((0 == dst_ptr->cur_idx) && (dst_ptr->cur_idx != dst_ptr->prv_idx)) {
				cce_header_ptr->is_update = ISP_ONE;
			}

			if (ISP_SMART_COLOR_CAST == block_result->smart_id) {

				cmr_s32 adjust_hue = dst_ptr->cur_level[0];
				cmr_s32 adjust_saturation = dst_ptr->cur_level[1];

				val_range.min = 0;
				val_range.max = 360;
				rtn = _pm_check_smart_param(block_result, &val_range, 2, ISP_SMART_Y_TYPE_VALUE);
				if (ISP_SUCCESS != rtn) {
					ISP_LOGE("fail to check pm smart param !");
					return rtn;
				}

				adjust_hue = block_result->component[0].fix_data[0];
				adjust_saturation = block_result->component[1].fix_data[0];

				if (adjust_hue != dst_ptr->cur_level[0]
				    || adjust_saturation != dst_ptr->cur_level[1]) {

					rtn = _pm_cce_adjust_hue_saturation(dst_ptr, adjust_hue, adjust_saturation);
					if (ISP_SUCCESS == rtn)
						cce_header_ptr->is_update = ISP_ONE;
				}

			} else if (ISP_SMART_GAIN_OFFSET == block_result->smart_id) {
				cmr_u16 r_gain = 0;
				cmr_u16 g_gain = 0;
				cmr_u16 b_gain = 0;

				val_range.min = 0;
				val_range.max = 360;
				rtn = _pm_check_smart_param(block_result, &val_range, 3, ISP_SMART_Y_TYPE_VALUE);
				if (ISP_SUCCESS != rtn) {
					ISP_LOGE("fail to check pm smart param !");
					return rtn;
				}

				r_gain = block_result->component[0].fix_data[0];
				g_gain = block_result->component[1].fix_data[0];
				b_gain = block_result->component[2].fix_data[0];

				if (dst_ptr->cce_coef[ISP_CCE_COEF_GAIN_OFFSET][0] != r_gain ||
				    dst_ptr->cce_coef[ISP_CCE_COEF_GAIN_OFFSET][1] != g_gain || dst_ptr->cce_coef[ISP_CCE_COEF_GAIN_OFFSET][2] != b_gain) {

					rtn = _pm_cce_adjust_gain_offset(dst_ptr, r_gain, g_gain, b_gain);
					if (ISP_SUCCESS == rtn)
						cce_header_ptr->is_update = ISP_ONE;
				}
			}
		}
		break;

	default:
		break;
	}

	return rtn;
}

cmr_s32 _pm_cce_get_param(void *cce_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_cce_param *cce_ptr = (struct isp_cce_param *)cce_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->id = ISP_BLK_CCE;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		{
			param_data_ptr->data_ptr = (void *)&cce_ptr->cur;
			param_data_ptr->data_size = sizeof(cce_ptr->cur);
			*update_flag = 0;
		}
		break;

	default:
		break;
	}

	return rtn;
}
