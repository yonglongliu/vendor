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
#define LOG_TAG "isp_blk_bdn"
#include "isp_blocks_cfg.h"

static cmr_u32 _pm_bdn_convert_param(void *dst_param, cmr_u32 strength_level,  cmr_u32 mode_flag, cmr_u32 scene_flag)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_s32 i = 0;
	cmr_u32 total_offset_units = 0;
	cmr_u32 *multi_nr_map_ptr = PNULL;
	struct isp_bdn_param *dst_ptr = (struct isp_bdn_param*)dst_param;
	struct sensor_bdn_level* bdn_param = PNULL;

	if (SENSOR_MULTI_MODE_FLAG != dst_ptr->nr_mode_setting) {
		bdn_param = (struct sensor_bdn_level*)(dst_ptr->param_ptr);
	} else {
		multi_nr_map_ptr = (cmr_u32 *)dst_ptr->scene_ptr;
		total_offset_units = _pm_calc_nr_addr_offset(mode_flag, scene_flag, multi_nr_map_ptr);
		bdn_param = (struct sensor_bdn_level*)((cmr_u8 *)dst_ptr->param_ptr + total_offset_units * dst_ptr->level_num * sizeof(struct sensor_bdn_level));
	}

	strength_level = PM_CLIP(strength_level, 0, dst_ptr->level_num - 1);
	if (bdn_param != NULL) {
		for (i = 0; i < 10; i++) {
			dst_ptr->cur.dis[i][0] = (bdn_param[strength_level].diswei_tab[i][0]&0xFF) | ((bdn_param[strength_level].diswei_tab[i][2]&0xFF) << 8)
				| ((bdn_param[strength_level].diswei_tab[i][4]&0xFF) << 16) | ((bdn_param[strength_level].diswei_tab[i][8]&0xFF) << 24);
			dst_ptr->cur.dis[i][1] = (bdn_param[strength_level].diswei_tab[i][10]&0xFF) | ((bdn_param[strength_level].diswei_tab[i][18]&0xFF) << 8);
		}

		for (i = 0; i < 10; i++) {
			dst_ptr->cur.ran[i][0] = (bdn_param[strength_level].ranwei_tab[i][0]&0xFF) | ((bdn_param[strength_level].ranwei_tab[i][1]&0xFF) << 8)
								| ((bdn_param[strength_level].ranwei_tab[i][2]&0xFF) << 16) | ((bdn_param[strength_level].ranwei_tab[i][3]&0xFF) << 24);
			dst_ptr->cur.ran[i][1] = (bdn_param[strength_level].ranwei_tab[i][4]&0xFF) | ((bdn_param[strength_level].ranwei_tab[i][5]&0xFF) << 8)
								| ((bdn_param[strength_level].ranwei_tab[i][6]&0xFF) << 16) | ((bdn_param[strength_level].ranwei_tab[i][7]&0xFF) << 24);
			dst_ptr->cur.ran[i][2] = (bdn_param[strength_level].ranwei_tab[i][8]&0xFF) | ((bdn_param[strength_level].ranwei_tab[i][9]&0xFF) << 8)
								| ((bdn_param[strength_level].ranwei_tab[i][10]&0xFF) << 16) | ((bdn_param[strength_level].ranwei_tab[i][11]&0xFF) << 24);
			dst_ptr->cur.ran[i][3] = (bdn_param[strength_level].ranwei_tab[i][12]&0xFF) | ((bdn_param[strength_level].ranwei_tab[i][13]&0xFF) << 8)
								| ((bdn_param[strength_level].ranwei_tab[i][14]&0xFF) << 16) | ((bdn_param[strength_level].ranwei_tab[i][15]&0xFF) << 24);
			dst_ptr->cur.ran[i][4] = (bdn_param[strength_level].ranwei_tab[i][16]&0xFF) | ((bdn_param[strength_level].ranwei_tab[i][17]&0xFF) << 8)
								| ((bdn_param[strength_level].ranwei_tab[i][18]&0xFF) << 16) | ((bdn_param[strength_level].ranwei_tab[i][19]&0xFF) << 24);
			dst_ptr->cur.ran[i][5] = (bdn_param[strength_level].ranwei_tab[i][20]&0xFF) | ((bdn_param[strength_level].ranwei_tab[i][21]&0xFF) << 8)
								| ((bdn_param[strength_level].ranwei_tab[i][22]&0xFF) << 16) | ((bdn_param[strength_level].ranwei_tab[i][23]&0xFF) << 24);
			dst_ptr->cur.ran[i][6] = (bdn_param[strength_level].ranwei_tab[i][24]&0xFF) | ((bdn_param[strength_level].ranwei_tab[i][25]&0xFF) << 8)
								| ((bdn_param[strength_level].ranwei_tab[i][26]&0xFF) << 16) | ((bdn_param[strength_level].ranwei_tab[i][27]&0xFF) << 24);
			dst_ptr->cur.ran[i][7] = (bdn_param[strength_level].ranwei_tab[i][28]&0xFF) | ((bdn_param[strength_level].ranwei_tab[i][29]&0xFF) << 8)
								| ((bdn_param[strength_level].ranwei_tab[i][30]&0xFF) << 16);
		}

		dst_ptr->cur.radial_bypass = bdn_param[strength_level].radial_bypass;
		dst_ptr->cur.addback = bdn_param[strength_level].addback;

		dst_ptr->cur.offset_x = bdn_param[strength_level].center.x;
		dst_ptr->cur.offset_y = bdn_param[strength_level].center.y;
		dst_ptr->cur.squ_x2 = bdn_param[strength_level].delta_sqr.x;
		dst_ptr->cur.squ_y2 = bdn_param[strength_level].delta_sqr.y;
		dst_ptr->cur.coef = bdn_param[strength_level].coeff.p1;
		dst_ptr->cur.coef2 = bdn_param[strength_level].coeff.p2;
		dst_ptr->cur.offset = bdn_param[strength_level].dis_sqr_offset;

		dst_ptr->cur.start_pos_x = 0x0;
		dst_ptr->cur.start_pos_y = 0x0;
		dst_ptr->cur.bypass = bdn_param[strength_level].bypass;
	}

	return rtn;

}

cmr_s32 _pm_bdn_init(void *dst_bdn_param, void *src_bdn_param, void* param1, void* param2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_bdn_param *dst_ptr = (struct isp_bdn_param *)dst_bdn_param;
	struct isp_pm_nr_header_param *src_ptr = (struct isp_pm_nr_header_param *)src_bdn_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param2);

	memset((void*)&dst_ptr->cur,0x00,sizeof(dst_ptr->cur));

	dst_ptr->cur.bypass = header_ptr->bypass;
	dst_ptr->cur_level = src_ptr->default_strength_level;
	dst_ptr->level_num = src_ptr->level_number;
	dst_ptr->param_ptr = src_ptr->param_ptr;
	dst_ptr->scene_ptr = src_ptr->multi_nr_map_ptr;
	dst_ptr->nr_mode_setting = src_ptr->nr_mode_setting;

	rtn=_pm_bdn_convert_param(dst_ptr, dst_ptr->cur_level, ISP_MODE_ID_COMMON, ISP_SCENEMODE_AUTO);
	dst_ptr->cur.bypass |=  header_ptr->bypass;
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("ISP_PM_BDN_CONVERT_PARAM: error!");
		return rtn;
	}

	header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_bdn_set_param(void *bdn_param, cmr_u32 cmd, void* param_ptr0, void* param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_bdn_param *bdn_ptr = (struct isp_bdn_param*)bdn_param;
	struct isp_pm_block_header *bdn_header_ptr = (struct isp_pm_block_header*)param_ptr1;

	bdn_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_BDN_BYPASS:
		bdn_ptr->cur.bypass = *((cmr_u32*)param_ptr0);
	break;

	case ISP_PM_BLK_BDN_RADIAL_BYPASS:
	{
		cmr_u32 bypass = *((cmr_u32*)param_ptr0);
		bdn_ptr->cur.radial_bypass = bypass;
	}
	break;

	case ISP_PM_BLK_BDN_STRENGTH_LEVEL:
	{
		cmr_u32 *level_ptr  = (cmr_u32*)param_ptr0;
		bdn_ptr->cur.ran_level = level_ptr[0];
		bdn_ptr->cur.dis_level = level_ptr[1];
	}
	break;

	case ISP_PM_BLK_SMART_SETTING:
	{
		struct smart_block_result *block_result = (struct smart_block_result*)param_ptr0;
		struct isp_range val_range;
		cmr_u32 cur_level[2] = {0};

		bdn_header_ptr->is_update = ISP_ZERO;
		val_range.min = 0;
		val_range.max = 255;

		rtn = _pm_check_smart_param(block_result, &val_range, 2, ISP_SMART_Y_TYPE_VALUE);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGE("ISP_PM_BLK_SMART_SETTING: wrong param !");
			return rtn;
		}

		cur_level[0] = (cmr_u32)block_result->component[0].fix_data[0];
		cur_level[1] = (cmr_u32)block_result->component[1].fix_data[0];

		if (cur_level[0] != bdn_ptr->cur.ran_level || cur_level[1] != bdn_ptr->cur.dis_level || nr_tool_flag[2] ||
			block_result->mode_flag_changed) {
			bdn_ptr->cur.ran_level = cur_level[0];
			bdn_ptr->cur.dis_level = cur_level[1];
			bdn_header_ptr->is_update = ISP_ONE;
			nr_tool_flag[2] = 0;
			block_result->mode_flag_changed = 0;
			rtn=_pm_bdn_convert_param(bdn_ptr, bdn_ptr->cur.ran_level,  block_result->mode_flag, block_result->scene_flag);
			bdn_ptr->cur.bypass |=  bdn_header_ptr->bypass;
			if (ISP_SUCCESS != rtn) {
				ISP_LOGE("ISP_PM_BDN_CONVERT_PARAM: error!");
				return rtn;
			}
		}
	}
	break;

	default:
		bdn_header_ptr->is_update = ISP_ZERO;
	break;
	}

	ISP_LOGI("ISP_SMART: cmd=%d, update=%d, cur_level[0]=%d, cur_level[1]=%d", cmd, bdn_header_ptr->is_update,
					bdn_ptr->cur.ran_level, bdn_ptr->cur.dis_level);

	return rtn;
}

cmr_s32 _pm_bdn_get_param(void *bdn_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_bdn_param *bdn_ptr = (struct isp_bdn_param*)bdn_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_BDN;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void*)&bdn_ptr->cur;
		param_data_ptr->data_size = sizeof(bdn_ptr->cur);
		*update_flag = 0;
	break;

	default:
	break;
	}

	return rtn;
}
