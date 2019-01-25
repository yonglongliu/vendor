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
#define LOG_TAG "altek_adpt"

#include <stdlib.h>
#include "isp_common_types.h"
#include "alwrapper_3a.h"
#include "hw3a_stats.h"
#include "isp_3a_adpt.h"

#define STATS_OTHER_SIZE         (100)

cmr_int isp_dispatch_stats(void *isp_stats, void *ae_stats_buf, void *awb_stats_buf, void *af_stats_buf, void *yhist_stats_buf, void *antif_stats_buf, void *subsample, cmr_u32 sof_idx)
{
	cmr_int                                     ret = ISP_SUCCESS;

	ret = (cmr_int)al3awrapper_dispatchhw3astats(isp_stats,
			(struct isp_drv_meta_ae_t*)ae_stats_buf,
			(struct isp_drv_meta_awb_t*)awb_stats_buf,
			(struct isp_drv_meta_af_t *)af_stats_buf,
			(struct isp_drv_meta_yhist_t *)yhist_stats_buf,
			(struct isp_drv_meta_antif_t *)antif_stats_buf,
			(struct isp_drv_meta_subsample_t *)subsample,
			sof_idx);
	if (ret) {
		ISP_LOGE("failed to disptach stats %ld", ret);
	}
	ISP_LOGV("done %ld", ret);
	return ret;
}

cmr_int isp_dispatch_af_stats(void *isp_af_stats, void *af_stats_buf, cmr_u32 sof_idx)
{
	cmr_int                                     ret = ISP_SUCCESS;

	ret = (cmr_int)al3awrapper_dispatchhw3astats_af(isp_af_stats,
			(struct isp_drv_meta_af_t *)af_stats_buf,
			sof_idx);
	if (ret) {
		ISP_LOGE("failed to disptach af stats %ld", ret);
	}
	ISP_LOGV("done %ld", ret);
	return ret;
}

cmr_int isp_separate_3a_bin(void *bin, void **ae_tuning_buf, void **awb_tuning_buf, void **af_tuning_buf)
{
	cmr_int                                     ret = ISP_SUCCESS;

	al3awrapper_com_separate_3a_bin((uint8*)bin, (uint8**)ae_tuning_buf, (uint8**)af_tuning_buf, (uint8**)awb_tuning_buf);

	return ret;
}

cmr_int isp_separate_one_bin(uint8* bin, uint32 uw_bin_size, uint32 uw_IDX_number, struct bin1_sep_info* a_bin1_info, struct bin2_sep_info* a_bin2_info, struct bin3_sep_info* a_bin3_info)
{
	cmr_int                                     ret = ISP_SUCCESS;

	al3awrapper_com_separate_one_bin((uint8*)bin, uw_bin_size, uw_IDX_number, a_bin1_info, a_bin2_info, a_bin3_info);
	//al3awrapper_com_separate_one_bin((uint8*)bin, (uint8**)ae_tuning_buf, (uint8**)af_tuning_buf, (uint8**)awb_tuning_buf);
	return ret;
}
cmr_int isp_separate_scenesetting(enum scene_setting_list_t scene, void *setting_file, struct scene_setting_info *scene_info)
{
	cmr_int                             ret = ISP_SUCCESS;
	al3awrapper_com_separate_scenesetting( scene, setting_file, scene_info);
	return ret;
}
cmr_int isp_separate_drv_bin(void *bin, void **shading_buf, void **irp_buf)
{
	cmr_int                                     ret = ISP_SUCCESS;

	al3awrapper_com_separate_shadingirp_bin((uint8*)bin, (uint8**)shading_buf, (uint8**)irp_buf);
	return ret;
}

cmr_int isp_separate_drv_bin_2(void *bin, cmr_u32 bin_size, struct bin2_sep_info *bin_info)
{
	cmr_int                                     ret = ISP_SUCCESS;

	al3awrapper_com_separate_shadingirp_bin_type2((uint8*)bin, bin_size, bin_info);
	return ret;
}

cmr_int isp_get_stats_size(struct stats_buf_size_list *stats_buf_size)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct wrapper_hw3a_define_sizeinfo_t size;

	al3awrapper_get_defined_stats_size(&size);
	stats_buf_size->ae_stats_size = size.ud_alhw3a_ae_stats_size + STATS_OTHER_SIZE;
	stats_buf_size->awb_stats_size = size.ud_alhw3a_awb_stats_size + STATS_OTHER_SIZE;
	stats_buf_size->af_stats_size = size.ud_alhw3a_af_stats_size + STATS_OTHER_SIZE;
	stats_buf_size->yhist_stats_size = size.ud_alhw3a_yhist_stats_size + STATS_OTHER_SIZE;
	stats_buf_size->antif_stats_size = size.ud_alhw3a_antif_stats_size + STATS_OTHER_SIZE;
	stats_buf_size->subimg_stats_size = size.ud_alhw3a_subimg_stats_size + STATS_OTHER_SIZE;

	return ret;
}
