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
#define LOG_TAG "isp_blk_1d_lsc"
#include "isp_blocks_cfg.h"

cmr_s32 _pm_1d_lsc_init(void *dst_lnc_param, void *src_lnc_param, void *param1, void *param2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 i = 0, j = 0, index = 0;
	struct isp_1d_lsc_param *dst_ptr = (struct isp_1d_lsc_param *)dst_lnc_param;
	struct sensor_1d_lsc_param *src_ptr = (struct sensor_1d_lsc_param *)src_lnc_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param2);

	memset((void *)&dst_ptr->cur, 0x00, sizeof(dst_ptr->cur));
	memcpy((void *)dst_ptr->map, (void *)src_ptr->map, sizeof(dst_ptr->map));
	dst_ptr->cur.bypass = header_ptr->bypass;

	dst_ptr->cur_index_info.x0 = src_ptr->cur_idx.x0;
	dst_ptr->cur_index_info.x1 = src_ptr->cur_idx.x1;
	dst_ptr->cur_index_info.weight0 = src_ptr->cur_idx.weight0;
	dst_ptr->cur_index_info.weight1 = src_ptr->cur_idx.weight1;

	for (i = 0; i < SENSOR_LENS_NUM; ++i) {
		for (j = 0; j < SENSOR_LNC_RC_NUM; ++j) {
			dst_ptr->map[i].curve_distcptn[j].center_pos.x = src_ptr->map[i].curve_distcptn[j].center_pos.x;
			dst_ptr->map[i].curve_distcptn[j].center_pos.y = src_ptr->map[i].curve_distcptn[j].center_pos.y;

			dst_ptr->map[i].curve_distcptn[j].rlsc_init_r = src_ptr->map[i].curve_distcptn[j].rlsc_init_r;
			dst_ptr->map[i].curve_distcptn[j].rlsc_init_r2 = src_ptr->map[i].curve_distcptn[j].rlsc_init_r2;
			dst_ptr->map[i].curve_distcptn[j].rlsc_init_dr2 = src_ptr->map[i].curve_distcptn[j].rlsc_init_dr2;
		}
		dst_ptr->map[i].rlsc_radius_step = src_ptr->map[i].rlsc_radius_step;
	}

	index = src_ptr->cur_idx.x0;

	dst_ptr->cur.radius_step = src_ptr->map[index].rlsc_radius_step;
	dst_ptr->cur.center_r0c0_col_x = src_ptr->map[index].curve_distcptn[0].center_pos.x;
	dst_ptr->cur.center_r0c0_row_y = src_ptr->map[index].curve_distcptn[0].center_pos.y;
	dst_ptr->cur.center_r0c1_col_x = src_ptr->map[index].curve_distcptn[1].center_pos.x;
	dst_ptr->cur.center_r0c1_row_y = src_ptr->map[index].curve_distcptn[1].center_pos.y;
	dst_ptr->cur.center_r1c0_col_x = src_ptr->map[index].curve_distcptn[2].center_pos.x;
	dst_ptr->cur.center_r1c0_row_y = src_ptr->map[index].curve_distcptn[2].center_pos.y;
	dst_ptr->cur.center_r1c1_col_x = src_ptr->map[index].curve_distcptn[3].center_pos.x;
	dst_ptr->cur.center_r1c1_row_y = src_ptr->map[index].curve_distcptn[3].center_pos.y;

	dst_ptr->cur.init_r_r0c1 = src_ptr->map[index].curve_distcptn[0].rlsc_init_r;
	dst_ptr->cur.init_r_r0c0 = src_ptr->map[index].curve_distcptn[1].rlsc_init_r;
	dst_ptr->cur.init_r_r1c1 = src_ptr->map[index].curve_distcptn[2].rlsc_init_r;
	dst_ptr->cur.init_r_r1c0 = src_ptr->map[index].curve_distcptn[3].rlsc_init_r;

	dst_ptr->cur.init_r2_r0c0 = src_ptr->map[index].curve_distcptn[0].rlsc_init_r2;
	dst_ptr->cur.init_r2_r0c1 = src_ptr->map[index].curve_distcptn[1].rlsc_init_r2;
	dst_ptr->cur.init_r2_r1c0 = src_ptr->map[index].curve_distcptn[2].rlsc_init_r2;
	dst_ptr->cur.init_r2_r1c1 = src_ptr->map[index].curve_distcptn[3].rlsc_init_r2;

	dst_ptr->cur.init_dr2_r0c0 = src_ptr->map[index].curve_distcptn[0].rlsc_init_dr2;
	dst_ptr->cur.init_dr2_r0c1 = src_ptr->map[index].curve_distcptn[1].rlsc_init_dr2;
	dst_ptr->cur.init_dr2_r1c0 = src_ptr->map[index].curve_distcptn[2].rlsc_init_dr2;
	dst_ptr->cur.init_dr2_r1c1 = src_ptr->map[index].curve_distcptn[3].rlsc_init_dr2;
	header_ptr->is_update = header_ptr->is_update | ISP_PM_BLK_LSC_UPDATE_MASK_PARAM;
#if __WORDSIZE == 64
	dst_ptr->cur.data_ptr[0] = (cmr_uint) (src_ptr->data_area) & 0xffffffff;
	dst_ptr->cur.data_ptr[1] = (cmr_uint) (src_ptr->data_area) >> 32;
#else
	dst_ptr->cur.data_ptr[0] = (cmr_uint) (src_ptr->data_area);
	dst_ptr->cur.data_ptr[1] = 0;
#endif
	return rtn;
}

cmr_s32 _pm_1d_lsc_set_param(void *lnc_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_1d_lsc_param *dst_lnc_ptr = PNULL;
	struct isp_pm_block_header *lnc_header_ptr = (struct isp_pm_block_header *)param_ptr1;
	dst_lnc_ptr = (struct isp_1d_lsc_param *)lnc_param;

	lnc_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_LSC_BYPASS:
		dst_lnc_ptr->cur.bypass = *((cmr_u32 *) param_ptr0);
		lnc_header_ptr->is_update |= ISP_PM_BLK_LSC_UPDATE_MASK_PARAM;
		break;

	case ISP_PM_BLK_SMART_SETTING:
		break;

	default:
		lnc_header_ptr->is_update = ISP_ZERO;
		break;
	}

	return rtn;
}

cmr_s32 _pm_1d_lsc_get_param(void *lnc_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	struct isp_1d_lsc_param *lnc_ptr = (struct isp_1d_lsc_param *)lnc_param;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->id = ISP_BLK_1D_LSC;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void *)&lnc_ptr->cur;
		param_data_ptr->data_size = sizeof(lnc_ptr->cur);
		*update_flag &= (~ISP_PM_BLK_LSC_UPDATE_MASK_PARAM);
		break;

	case ISP_PM_BLK_LSC_BYPASS:
		param_data_ptr->data_ptr = (void *)&lnc_ptr->cur.bypass;
		param_data_ptr->data_size = sizeof(lnc_ptr->cur.bypass);
		break;

	default:
		break;
	}

	return rtn;
}
