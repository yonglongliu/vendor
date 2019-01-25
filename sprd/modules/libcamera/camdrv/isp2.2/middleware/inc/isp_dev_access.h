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
#ifndef _ISP_DEV_ACCESS_H_
#define _ISP_DEV_ACCESS_H_

#include "isp_drv.h"

enum isp_dev_access_ctrl_cmd {
	ISP_DEV_SET_AE_MONITOR,
	ISP_DEV_SET_AE_MONITOR_WIN,
	ISP_DEV_SET_AE_MONITOR_BYPASS,
	ISP_DEV_SET_AE_STATISTICS_MODE,
	ISP_DEV_GET_AE_SYSTEM_TIME,
	ISP_DEV_GET_AF_MONITOR_WIN_NUM,
	ISP_DEV_SET_AF_MONITOR,
	ISP_DEV_SET_AF_MONITOR_WIN,
	ISP_DEV_SET_AF_BYPASS,
	ISP_DEV_SET_AF_WORK_MODE,
	ISP_DEV_SET_AF_SKIP_NUM,
	ISP_DEV_SET_AF_IIR_CFG,
	ISP_DEV_SET_AF_MODULES_CFG,
	ISP_DEV_SET_AE_SHIFT,
	ISP_DEV_SET_RGB_GAIN,
	ISP_DEV_SET_AFL_BLOCK,
	ISP_DEV_SET_AFL_NEW_BLOCK,
	ISP_DEV_RAW_AEM_BYPASS,
	ISP_DEV_RAW_AFM_BYPASS,
	ISP_DEV_RESET,
	ISP_DEV_STOP,
	ISP_DEV_ENABLE_IRQ,
	ISP_DEV_SET_STSTIS_BUF,
	ISP_DEV_SET_PDAF_CFG_PARAM,
	ISP_DEV_SET_PDAF_PPI_INFO,
	ISP_DEV_SET_PDAF_BYPASS,
	ISP_DEV_SET_PDAF_WORK_MODE,
	ISP_DEV_SET_PDAF_EXTRACTOR_BYPASS,
	ISP_DEV_SET_PDAF_ROI,
	ISP_DEV_SET_PDAF_SKIP_NUM,
	ISP_DEV_SET_AFL_CFG_PARAM,
	ISP_DEV_SET_AFL_BYPASS,
	ISP_DEV_SET_AFL_NEW_CFG_PARAM,
	ISP_DEV_SET_AFL_NEW_BYPASS,
	ISP_DEV_GET_AF_SYSTEM_TIME,
	ISP_DEV_POST_3DNR, //for post 3dnr
	ISP_DEV_CMD_MAX
};

struct isp_dev_access_context {
	cmr_handle evt_alg_handle;
	isp_evt_cb isp_event_cb;
	pthread_t monitor_handle;
	cmr_handle isp_driver_handle;
	struct isp_statis_mem_info statis_mem_info;
	struct isp_ops ops;
};

cmr_int isp_dev_raw_afm_type1_statistic(cmr_handle isp_dev_handle, void *statics);
cmr_int isp_dev_raw_afm_statistic_r6p9(cmr_handle isp_dev_handle, void *statics);
cmr_u32 isp_dev_access_chip_id(cmr_handle isp_dev_handle);
cmr_int isp_dev_statis_buf_malloc(cmr_handle isp_dev_handle, struct isp_statis_mem_info *in_ptr);
cmr_int isp_dev_trans_addr(cmr_handle isp_dev_handle);
cmr_int isp_dev_set_interface(struct isp_interface_param_v1 *in_ptr);
cmr_int isp_dev_start(cmr_handle isp_dev_handle, struct isp_interface_param_v1 *in_ptr);
cmr_int isp_dev_anti_flicker_bypass(cmr_handle isp_dev_handle, cmr_int bypass);
cmr_int isp_dev_anti_flicker_new_bypass(cmr_handle isp_dev_handle, cmr_int bypass);
cmr_int isp_dev_comm_shadow(cmr_handle isp_dev_handle, cmr_int shadow);
cmr_int isp_dev_lsc_update(cmr_handle isp_dev_handle, cmr_int flag);
cmr_int isp_dev_awb_gain(cmr_handle isp_dev_handle, cmr_u32 r, cmr_u32 g, cmr_u32 b);
cmr_int isp_dev_cfg_block(cmr_handle isp_dev_handle, void *data_ptr, cmr_int data_id);
void isp_dev_access_evt_reg(cmr_handle isp_dev_handle, isp_evt_cb isp_event_cb, void *privdata);
cmr_int isp_dev_access_init(cmr_s32 fd, cmr_handle * isp_dev_handle);
cmr_int isp_dev_access_deinit(cmr_handle isp_handler);
cmr_int isp_dev_access_capability(cmr_handle isp_dev_handle, enum isp_capbility_cmd cmd, void *param_ptr);
cmr_int isp_dev_access_ioctl(cmr_handle isp_dev_handle, cmr_int cmd, void *param0, void *param1);
void isp_dev_statis_info_proc(cmr_handle isp_dev_handle, void *param_ptr);
void isp_dev_irq_info_proc(cmr_handle isp_dev_handle, void *param_ptr);

#endif
