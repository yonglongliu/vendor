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
#define LOG_TAG "lib_ctrl"
#include "isp_com.h"
#include "awb_ctrl.h"
#include "ae_sprd_adpt.h"
#include "af_ctrl.h"
//#include "awb_al_ctrl.h"
#include "awb_sprd_adpt.h"
#include "sensor_raw.h"
#include "af_log.h"
#include "lib_ctrl.h"
//#include "sp_af_ctrl.h"
#include <dlfcn.h>
//#include "ALC_AF_Ctrl.h"

#ifdef CONFIG_USE_ALC_AE
#include "ae_alc_ctrl.h"
#endif

struct awb_lib_fun awb_lib_fun;
struct ae_lib_fun ae_lib_fun;
struct af_lib_fun af_lib_fun;
//struct al_awb_thirdlib_fun al_awb_thirdlib_fun;

char *al_libversion_choice(cmr_u32 version_id)
{
	ISP_LOGV("E");
	switch (version_id) {
	case AL_AWB_LIB_VERSION_0:
		return "/system/lib/libAl_Awb_ov13850r2a.so";
	case AL_AWB_LIB_VERSION_1:
		return "/system/lib/libAl_Awb_t4kb3.so";
	}

	return NULL;
}

#if 0
cmr_u32 al_awb_lib_open(cmr_u32 version_id)
{
	void *handle;
	char *AWB_LIB;
	ISP_LOGE("E");
	AWB_LIB = al_libversion_choice(version_id);
	handle = dlopen(AWB_LIB, RTLD_NOW);

	if (NULL == handle) {
		ISP_LOGE("dlopen get handle error\n");
		return ISP_ERROR;
	}

	al_awb_thirdlib_fun.AlAwbInterfaceInit = (void *)dlsym(handle, "AlAwbInterfaceInit");
	if (NULL == al_awb_thirdlib_fun.AlAwbInterfaceInit) {
		ISP_LOGE("dlsym AlAwbInterfaceInit error\n");
		goto load_error;
	}
	al_awb_thirdlib_fun.AlAwbInterfaceDestroy = (void *)dlsym(handle, "AlAwbInterfaceDestroy");
	if (NULL == al_awb_thirdlib_fun.AlAwbInterfaceDestroy) {
		ISP_LOGE("dlsym AlAwbInterfaceDestroy error\n");
		goto load_error;
	}
	al_awb_thirdlib_fun.AlAwbInterfaceReset = (void *)dlsym(handle, "AlAwbInterfaceReset");
	if (NULL == al_awb_thirdlib_fun.AlAwbInterfaceReset) {
		ISP_LOGE("dlsym AlAwbInterfaceReset error\n");
		goto load_error;
	}
	al_awb_thirdlib_fun.AlAwbInterfaceMain = (void *)dlsym(handle, "AlAwbInterfaceMain");
	if (NULL == al_awb_thirdlib_fun.AlAwbInterfaceMain) {
		ISP_LOGE("dlsym AlAwbInterfaceMain error\n");
		goto load_error;
	}
	al_awb_thirdlib_fun.AlAwbInterfaceSendCommand = (void *)dlsym(handle, "AlAwbInterfaceSendCommand");
	if (NULL == al_awb_thirdlib_fun.AlAwbInterfaceSendCommand) {
		ISP_LOGE("dlsym AlAwbInterfaceSendCommand error\n");
	}
	al_awb_thirdlib_fun.AlAwbInterfaceShowVersion = (void *)dlsym(handle, "AlAwbInterfaceShowVersion");
	if (NULL == al_awb_thirdlib_fun.AlAwbInterfaceShowVersion) {
		ISP_LOGE("dlsym AlAwbInterfaceShowVersion error\n");
		goto load_error;
	}
	ISP_LOGE("awb_thirdlib_fun.AlAwbInterfaceInit= %p", al_awb_thirdlib_fun.AlAwbInterfaceInit);
	ISP_LOGE("AWB_LIB= %s", AWB_LIB);

	return ISP_SUCCESS;

  load_error:

	if (NULL != handle) {
		dlclose(handle);
		handle = NULL;
	}
	return ISP_ERROR;

}
#endif
cmr_u32 isp_awblib_init(struct sensor_libuse_info * libuse_info, struct awb_lib_fun * awb_lib_fun)
{
	cmr_u32 rtn = AWB_CTRL_SUCCESS;
	struct third_lib_info awb_lib_info;
	cmr_u32 awb_producer_id = 0;
	cmr_u32 awb_lib_version = 0;
	UNUSED(awb_lib_fun);
	ISP_LOGI("E");
	if (libuse_info) {
		awb_lib_info = libuse_info->awb_lib_info;
		awb_producer_id = awb_lib_info.product_id;
		awb_lib_version = awb_lib_info.version_id;
		ISP_LOGV("awb_producer_id= ____0x%x", awb_producer_id);
		ISP_LOGV("awb_lib_version= ____0x%x", awb_lib_version);
	}

	switch (awb_producer_id) {
	case SPRD_AWB_LIB:
		switch (awb_lib_version) {
		case AWB_LIB_VERSION_0:
			//awb_lib_fun->awb_ctrl_init            = awb_sprd_ctrl_init;
			//awb_lib_fun->awb_ctrl_deinit          = awb_sprd_ctrl_deinit;
			//awb_lib_fun->awb_ctrl_calculation     = awb_sprd_ctrl_calculation;
			//awb_lib_fun->awb_ctrl_ioctrl          = awb_sprd_ctrl_ioctrl;
			break;
		case AWB_LIB_VERSION_1:
		default:
			ISP_LOGE("fail to check param, version = 0x%x", awb_lib_version);
			rtn = AWB_CTRL_ERROR;
		}
		break;

	default:
		ISP_LOGE("fail to check param, lib producer id = 0x%x", awb_producer_id);
		rtn = AWB_CTRL_ERROR;
	}

	return rtn;
}

cmr_u32 isp_aelib_init(struct sensor_libuse_info * libuse_info, struct ae_lib_fun * ae_lib_fun)
{
	cmr_u32 rtn = AE_SUCCESS;
	struct third_lib_info ae_lib_info;
	cmr_u32 ae_producer_id = 0;
	cmr_u32 ae_lib_version = 0;

	ISP_LOGI("E");
	if (libuse_info) {
		ae_lib_info = libuse_info->ae_lib_info;
		ae_producer_id = ae_lib_info.product_id;
		ae_lib_version = ae_lib_info.version_id;
		ISP_LOGI("ae_producer_id= ____0x%x", ae_producer_id);
	}

	ae_lib_fun->product_id = ae_producer_id;
	ae_lib_fun->version_id = ae_lib_version;

	switch (ae_producer_id) {
	case SPRD_AE_LIB:
		switch (ae_lib_version) {
		case AE_LIB_VERSION_0:
			break;
		case AE_LIB_VERSION_1:
		default:
			ISP_LOGE("fail to check param, ae invalid lib version = 0x%x", ae_lib_version);
			rtn = AE_ERROR;
		}
		break;
	default:
		ISP_LOGE("fail to check param, lib producer id = 0x%x", ae_producer_id);
		rtn = AE_ERROR;
	}

	return rtn;
}

cmr_u32 isp_aflib_init(struct sensor_libuse_info * libuse_info, struct af_lib_fun * af_lib_fun)
{
	cmr_u32 rtn = AF_SUCCESS;
	struct third_lib_info af_lib_info;
	cmr_u32 af_producer_id = 0;
	cmr_u32 af_lib_version = 0;

	ISP_LOGI("E");
	if (libuse_info) {
		af_lib_info = libuse_info->af_lib_info;
		af_producer_id = af_lib_info.product_id;
		af_lib_version = af_lib_info.version_id;
		ISP_LOGI("af_producer_id= ____0x%x", af_producer_id);
	}
	ISP_LOGV("af_producer_id= ____0x%x", af_producer_id);
	memset(af_lib_fun, 0, sizeof(struct af_lib_fun));
	switch (af_producer_id) {
	case SPRD_AF_LIB:
		switch (af_lib_version) {
		case AF_LIB_VERSION_0:
			break;
		case AF_LIB_VERSION_1:
		default:
			ISP_LOGE("fail to check param, lib version = 0x%x", af_lib_version);
			rtn = AF_ERROR;
		}
		break;
	case SFT_AF_LIB:
		switch (af_lib_version) {
		case AF_LIB_VERSION_0:
#if 0
			af_lib_fun->af_init_interface = sft_af_init;
			af_lib_fun->af_calc_interface = sft_af_calc;
			af_lib_fun->af_deinit_interface = sft_af_deinit;
			af_lib_fun->af_ioctrl_interface = sft_af_ioctrl;
			af_lib_fun->af_ioctrl_set_flash_notice = sft_af_ioctrl_set_flash_notice;
			af_lib_fun->af_ioctrl_get_af_value = sft_af_ioctrl_get_af_value;
			af_lib_fun->af_ioctrl_burst_notice = sft_af_ioctrl_burst_notice;
			af_lib_fun->af_ioctrl_set_af_mode = sft_af_ioctrl_set_af_mode;
			af_lib_fun->af_ioctrl_get_af_mode = sft_af_ioctrl_get_af_mode;
			af_lib_fun->af_ioctrl_ioread = sft_af_ioctrl_ioread;
			af_lib_fun->af_ioctrl_iowrite = sft_af_ioctrl_iowrite;
			af_lib_fun->af_ioctrl_set_fd_update = sft_af_ioctrl_set_fd_update;
			af_lib_fun->af_ioctrl_af_start = sft_af_ioctrl_af_start;
			af_lib_fun->af_ioctrl_set_isp_start_info = sft_af_ioctrl_set_isp_start_info;
			af_lib_fun->af_ioctrl_set_isp_stop_info = sft_af_ioctrl_set_isp_stop_info;
			af_lib_fun->af_ioctrl_set_ae_awb_info = sft_af_ioctrl_set_ae_awb_info;
			af_lib_fun->af_ioctrl_get_af_cur_pos = sft_af_ioctrl_get_af_cur_pos;
			af_lib_fun->af_ioctrl_set_af_pos = sft_af_ioctrl_set_af_pos;
			af_lib_fun->af_ioctrl_set_af_bypass = sft_af_ioctrl_set_af_bypass;
			af_lib_fun->af_ioctrl_set_af_stop = sft_af_ioctrl_set_af_stop;
			af_lib_fun->af_ioctrl_set_af_param = sft_af_ioctrl_set_af_param;
#endif
			break;
		case AF_LIB_VERSION_1:
		default:
			ISP_LOGE("fail to check param, lib version = 0x%x", af_lib_version);
			rtn = AF_ERROR;
		}
		break;
	case ALC_AF_LIB:
		switch (af_lib_version) {
		case AF_LIB_VERSION_0:
#if 0
			af_lib_fun->af_init_interface = alc_af_init;
			af_lib_fun->af_calc_interface = alc_af_calc;
			af_lib_fun->af_deinit_interface = alc_af_deinit;
			af_lib_fun->af_ioctrl_interface = alc_af_ioctrl;
			af_lib_fun->af_ioctrl_set_af_mode = alc_af_ioctrl_set_af_mode;
			af_lib_fun->af_ioctrl_set_fd_update = alc_af_ioctrl_set_fd_update;
			af_lib_fun->af_ioctrl_af_start = alc_af_ioctrl_af_start;
			af_lib_fun->af_ioctrl_set_isp_start_info = alc_af_ioctrl_set_isp_start_info;
			af_lib_fun->af_ioctrl_set_ae_awb_info = alc_af_ioctrl_set_ae_awb_info;
#endif
		case AF_LIB_VERSION_1:
		default:
			ISP_LOGE("fail to check param, lib version = 0x%x", af_lib_version);
		}
		break;
	default:
		ISP_LOGE("fail to check param, lib producer id = 0x%x", af_producer_id);
		rtn = AF_ERROR;
	}

	return rtn;
}

cmr_u32 aaa_lib_init(isp_ctrl_context * handle, struct sensor_libuse_info * libuse_info)
{
	memset(&awb_lib_fun, 0x00, sizeof(awb_lib_fun));
	memset(&ae_lib_fun, 0x00, sizeof(ae_lib_fun));
	memset(&af_lib_fun, 0x00, sizeof(af_lib_fun));

	isp_awblib_init(libuse_info, &awb_lib_fun);
	handle->awb_lib_fun = &awb_lib_fun;
	isp_aelib_init(libuse_info, &ae_lib_fun);
	handle->ae_lib_fun = &ae_lib_fun;
	isp_aflib_init(libuse_info, &af_lib_fun);
	handle->af_lib_fun = &af_lib_fun;

	return 0;
}
