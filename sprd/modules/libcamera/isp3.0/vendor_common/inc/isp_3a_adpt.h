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

#ifndef _ISP_3A_ADPT_H_
#define _ISP_3A_ADPT_H_

#include "alwrapper_common.h"

struct stats_buf_size_list {
	cmr_u32 ae_stats_size;
	cmr_u32 awb_stats_size;
	cmr_u32 af_stats_size;
	cmr_u32 yhist_stats_size;
	cmr_u32 antif_stats_size;
	cmr_u32 subimg_stats_size;
};

cmr_int isp_dispatch_stats(void *isp_stats, void *ae_stats_buf, void *awb_stats_buf, void *af_stats_buf, void *yhist_stats_buf, void *antif_stats_buf, void *subsample, cmr_u32 sof_idx);
cmr_int isp_dispatch_af_stats(void *isp_af_stats, void *af_stats_buf, cmr_u32 sof_idx);
cmr_int isp_separate_3a_bin(void *bin, void **ae_tuning_buf, void **awb_tuning_buf, void **af_tuning_buf);
cmr_int isp_separate_one_bin(uint8* bin, uint32 uw_bin_size, uint32 uw_IDX_number, struct bin1_sep_info* a_bin1_info, struct bin2_sep_info* a_bin2_info, struct bin3_sep_info* a_bin3_info);
cmr_int isp_separate_scenesetting(enum scene_setting_list_t scene, void *setting_file, struct scene_setting_info *scene_info);
cmr_int isp_separate_drv_bin(void *bin, void **shading_buf, void **irp_buf);
cmr_int isp_separate_drv_bin_2(void *bin, cmr_u32 bin_size, struct bin2_sep_info *bin_info);
cmr_int isp_get_stats_size(struct stats_buf_size_list *stats_buf_size);
#endif
