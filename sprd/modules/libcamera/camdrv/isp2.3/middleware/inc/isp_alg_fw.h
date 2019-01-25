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
#ifndef _ISP_ALG_FW_H_
#define _ISP_ALG_FW_H_

#ifndef LOCAL_INCLUDE_ONLY
#error "Hi, This is only for camdrv."
#endif

#include "isp_mw.h"
#include "isp_blocks_cfg.h"
struct isp_alg_fw_init_in {
	cmr_handle dev_access_handle;
	struct isp_init_param *init_param;
};

cmr_int isp_alg_fw_init(struct isp_alg_fw_init_in *input_ptr, cmr_handle * isp_alg_handle);
cmr_int isp_alg_fw_deinit(cmr_handle isp_alg_handle);
cmr_int isp_alg_fw_ioctl(cmr_handle isp_alg_handle, enum isp_ctrl_cmd io_cmd, void *param_ptr);
cmr_int isp_alg_fw_start(cmr_handle isp_alg_handle, struct isp_video_start *in_ptr);
cmr_int isp_alg_fw_stop(cmr_handle isp_alg_handle);
cmr_int isp_alg_fw_proc_start(cmr_handle isp_alg_handle, struct ips_in_param *in_ptr);
cmr_int isp_alg_fw_proc_next(cmr_handle isp_alg_handle, struct ipn_in_param *in_ptr);
cmr_int isp_alg_fw_capability(cmr_handle isp_alg_handle, enum isp_capbility_cmd cmd, void *param_ptr);

#endif
