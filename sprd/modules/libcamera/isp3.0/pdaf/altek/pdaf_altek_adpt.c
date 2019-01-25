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
#define LOG_TAG "alk_adpt_pdaf"

#include <dlfcn.h>
#include <math.h>
#include <utils/Timers.h>
#include "isp_type.h"
#include "pdaf_ctrl.h"
#include "isp_adpt.h"
#include "hw3a_stats.h"
#include "cutils/properties.h"
#include "isp_mlog.h"
#include "alPDAF_err.h"
#include "alPDAF.h"
#include "PDExtract.h" /* for typ3 */
#include "PDExtract2.h" /* for type2 */

#undef FEATURE_DL_OPEN

#define PDLIB_PATH "libalPDAF.so"
//#define PDEXTRACT_LIBPATH "libalPDExtract.so"
#define PDEXTRACT_LIBPATH "libalPDExtract.so"
#define PDEXTRACT_TYPE2_LIBPATH "libalpdextract2.so"
#define PD_REG_OUT_SIZE 352
#define PD_OTP_PACK_SIZE 550

struct pdaf_altek_lib_ops {
	cmr_s32 (*init)(void *a_pInPDPackData, void *a_pInOTPData, int a_dInOTPSize, void *a_pInTuningPara);
	cmr_s32 (*calc)(float *a_pfInPDValue, void *a_pOutPDReg, void *a_pInImageBuf_left,
				void *a_pInImageBuf_right, unsigned short a_uwInWidth, unsigned short a_uwInHeight,
				alGE_RECT a_tInWOI, DataBit a_tInbit, PDInReg *a_tInPDReg);
	cmr_s32 (*deinit)(void);
	cmr_s32 (*reset)(void);
};

struct pdaf_type3_extract_ops {
	cmr_s32 (*getsize)(struct altek_pdaf_info *PDSensorInfo, alPD_RECT *InputROI,
					unsigned short RawFileWidth, unsigned short RawFileHeight,
					unsigned short *PDDataWidth, unsigned short *PDDataHeight);
	cmr_s32 (*extract)(struct altek_pdaf_info *PDSensorInfo, cmr_u8 *RawFile, alPD_RECT *InputROI, cmr_u16 RawFileWidth,
					cmr_u16 RawFileHeight, cmr_u16 *PDOutLeft, cmr_u16 *PDOutRight,unsigned short *PDDataWidth, unsigned short *PDDataHeight);
};

struct pdaf_type2_extract_ops {
	cmr_s32 (*get_version)(void *version, int size);
	cmr_s32 (*extract)(unsigned char *tail_data, struct altek_pd_size pd_size,
			   enum vendor sensor_module,
			   unsigned short *pd_left, unsigned short *pd_right);
};

struct pdaf_altek_lib_api {
	cmr_s32 (*pdaf_altek_version)(void *a_pOutBuf, int a_dInBufMaxSize);
};

struct pdaf_type2_info {
	cmr_u32 data_format;
	cmr_u32 width;
	cmr_u32 height;
	cmr_u32 pd_size;
};

struct pdaf_altek_context {
	cmr_u32 camera_id;
	cmr_u8 pdaf_support;
	cmr_u8 pd_open;
	cmr_u8 is_busy;
	cmr_u32 frame_id;
	cmr_u32 token_id;
	cmr_u8 pd_enable;
	cmr_u8 extract_enable;
	cmr_handle caller_handle;
	cmr_handle altek_lib_handle;
	cmr_handle extract_lib_handle;
	struct pdaf_altek_lib_api lib_api;
	struct pdaf_altek_lib_ops ops;
	struct pdaf_type3_extract_ops extract3_ops;
	struct pdaf_type2_extract_ops extract2_ops;
	struct pdaf_ctrl_cb_ops_type cb_ops;
	cmr_int (*pd_set_buffer) (struct pd_frame_in *cb_param);
	struct altek_pdaf_info pd_info;
	void *tuning_param[ISP_INDEX_MAX];
	struct pdaf_type2_info type2_info;
	void *pd_left;
	void *pd_right;
	alPD_RECT roi;
	PDInReg pd_reg_in;
	cmr_u8 pd_reg_out[PD_REG_OUT_SIZE];
	struct isp3a_pdaf_altek_report_t report_data;
	cmr_u8 pdotp_pack_data[PD_OTP_PACK_SIZE];
	cmr_u8 vendor;
	//struct pdaf_ctrl_init_in init_in_param;
};

/************************************ INTERNAK DECLARATION ************************************/


/************************************ INTERNAK FUNCTION ***************************************/

static cmr_int load_pdaltek_library(cmr_handle adpt_handle)
{
	cmr_int ret = -ISP_ERROR;
	struct pdaf_altek_context *cxt = (struct pdaf_altek_context *)adpt_handle;

	ISP_CHECK_HANDLE_VALID(adpt_handle);

#ifdef FEATURE_DL_OPEN
	cxt->altek_lib_handle = dlopen(PDLIB_PATH, RTLD_NOW);
	if (!cxt->altek_lib_handle) {
		ISP_LOGE("failed to dlopen");
		goto error_dlopen;
	}
	cxt->lib_api.pdaf_altek_version = dlsym(cxt->altek_lib_handle, "alPDAF_VersionInfo_Get");
	if (!cxt->lib_api.pdaf_altek_version) {
		ISP_LOGE("failed to dlsym version");
		goto error_dlsym;
	}
	cxt->ops.init = dlsym(cxt->altek_lib_handle, "alPDAF_Initial");
	if (!cxt->ops.init) {
		ISP_LOGE("failed to dlsym init");
		goto error_dlsym;
	}

	cxt->ops.calc = dlsym(cxt->altek_lib_handle, "alPDAF_Calculate");
	if (!cxt->ops.calc) {
		ISP_LOGE("failed to dlsym calc");
		goto error_dlsym;
	}
	cxt->ops.deinit = dlsym(cxt->altek_lib_handle, "alPDAF_Close");
	if (!cxt->ops.deinit) {
		ISP_LOGE("failed to dlsym deinit");
		goto error_dlsym;
	}
	cxt->ops.reset = dlsym(cxt->altek_lib_handle, "alPDAF_Reset");
	if (!cxt->ops.reset) {
		ISP_LOGE("failed to dlsym reset");
		goto error_dlsym;
	}

	return 0;
error_dlsym:
	dlclose(cxt->altek_lib_handle);
	cxt->altek_lib_handle = NULL;
error_dlopen:
	return ret;
#else
	cxt->lib_api.pdaf_altek_version = alPDAF_VersionInfo_Get;
	cxt->ops.init = alPDAF_Initial;
	cxt->ops.calc = alPDAF_Calculate;
	cxt->ops.deinit = alPDAF_Close;
	cxt->ops.reset = alPDAF_Reset;
	return ISP_SUCCESS;
#endif
}

static cmr_int unload_pdaltek_library(cmr_handle adpt_handle)
{
	struct pdaf_altek_context *cxt = (struct pdaf_altek_context *)adpt_handle;

	ISP_CHECK_HANDLE_VALID(adpt_handle);
	if (cxt->altek_lib_handle) {
		dlclose(cxt->altek_lib_handle);
		cxt->altek_lib_handle = NULL;
	}
	return 0;
}

static cmr_int load_pdaltek_extract_library(cmr_handle adpt_handle)
{
	cmr_int ret = -ISP_ERROR;
	struct pdaf_altek_context *cxt = (struct pdaf_altek_context *)adpt_handle;

	ISP_CHECK_HANDLE_VALID(adpt_handle);
	switch (cxt->pdaf_support) {
	case SENSOR_PDAF_TYPE2_ENABLE:
		cxt->extract_lib_handle = dlopen(PDEXTRACT_TYPE2_LIBPATH, RTLD_NOW);
		if (!cxt->extract_lib_handle) {
			ISP_LOGE("failed to dlopen");
			goto error_dlopen;
		}
		cxt->extract2_ops.get_version = dlsym(cxt->extract_lib_handle, "altek_extract_getversion");
		if (!cxt->extract2_ops.get_version) {
			ISP_LOGE("failed to dlsym get version");
			goto error_dlsym;
		}
		cxt->extract2_ops.extract = dlsym(cxt->extract_lib_handle, "altek_extract_type2");
		if (!cxt->extract2_ops.extract) {
			ISP_LOGE("failed to dlsym extract");
			goto error_dlsym;
		}
		break;
	case SENSOR_PDAF_TYPE3_ENABLE:
		cxt->extract_lib_handle = dlopen(PDEXTRACT_LIBPATH, RTLD_NOW);
		if (!cxt->extract_lib_handle) {
			ISP_LOGE("failed to dlopen");
			goto error_dlopen;
		}
		cxt->extract3_ops.extract = dlsym(cxt->extract_lib_handle, "alPDExtract_Run");
		if (!cxt->extract3_ops.extract) {
			ISP_LOGE("failed to dlsym extract");
			goto error_dlsym;
		}
		cxt->extract3_ops.getsize = dlsym(cxt->extract_lib_handle, "alPDExtract_GetSize");
		if (!cxt->extract3_ops.getsize) {
			ISP_LOGE("failed to dlsym getsize");
			goto error_dlsym;
		}
		break;
	default:
		break;
	}

	return 0;
error_dlsym:
	dlclose(cxt->extract_lib_handle);
	cxt->extract_lib_handle = NULL;
error_dlopen:
	return ret;
}

static cmr_int unload_pdaltek_extract_library(cmr_handle adpt_handle)
{
	struct pdaf_altek_context *cxt = (struct pdaf_altek_context *)adpt_handle;

	ISP_CHECK_HANDLE_VALID(adpt_handle);
	if (cxt->extract_lib_handle) {
		dlclose(cxt->extract_lib_handle);
		cxt->extract_lib_handle = NULL;
	}
	return 0;
}

static cmr_int pdafaltek_adpt_get_version(cmr_handle adpt_handle)
{
	struct pdaf_altek_context *cxt = (struct pdaf_altek_context *)adpt_handle;
	cmr_int ret = -ISP_ERROR;
	cmr_u8 version[256] = { 0 };
	char extract_version[256] = { 0 };

	ISP_CHECK_HANDLE_VALID(adpt_handle);
	ret = cxt->lib_api.pdaf_altek_version(&version, sizeof(version));
	if (SENSOR_PDAF_TYPE2_ENABLE == cxt->pdaf_support)
		cxt->extract2_ops.get_version(extract_version, sizeof(extract_version));

	ISP_LOGI("cal version %s extract ver:%s", version, extract_version);
	return ret;
}

static cmr_int pdafaltek_adpt_set_pd_info(cmr_handle adpt_handle, struct sensor_pdaf_info *pd_info)
{
	struct pdaf_altek_context *cxt = (struct pdaf_altek_context *)adpt_handle;
	cmr_int ret = -ISP_ERROR;

	ISP_CHECK_HANDLE_VALID(adpt_handle);

	if (!pd_info) {
		ret = -ISP_PARAM_NULL;
		return ret;
	}
	cxt->pd_info.pd_offset_x = pd_info->pd_offset_x;
	cxt->pd_info.pd_offset_y = pd_info->pd_offset_y;
	cxt->pd_info.pd_pitch_x = pd_info->pd_pitch_x;
	cxt->pd_info.pd_pitch_y = pd_info->pd_pitch_y;
	cxt->pd_info.pd_density_x = pd_info->pd_density_x;
	cxt->pd_info.pd_density_y = pd_info->pd_density_y;
	cxt->pd_info.pd_block_num_x = pd_info->pd_block_num_x;
	cxt->pd_info.pd_block_num_y = pd_info->pd_block_num_y;
	cxt->pd_info.pd_pos_size = pd_info->pd_pos_size;
	cxt->pd_info.pd_pos_r = (struct altek_pos_info *)pd_info->pd_pos_r;
	cxt->pd_info.pd_pos_l = (struct altek_pos_info *)pd_info->pd_pos_l;

	cxt->type2_info.data_format = pd_info->type2_info.data_format;
	cxt->type2_info.width = pd_info->type2_info.width;
	cxt->type2_info.height = pd_info->type2_info.height;
	ISP_LOGI("pd_density_x %d, pd_block_num_x %d, pd_block_num_y %d type2: df %d w %d h %d",
		 cxt->pd_info.pd_density_x, cxt->pd_info.pd_block_num_x, cxt->pd_info.pd_block_num_y,
		 cxt->type2_info.data_format, cxt->type2_info.width, cxt->type2_info.height);

	return 0;
}

static cmr_int pdafaltek_adpt_set_open(cmr_handle adpt_handle, struct pdaf_ctrl_param_in *in)
{
	cmr_int ret = -ISP_ERROR;
	struct pdaf_altek_context *cxt = (struct pdaf_altek_context *)adpt_handle;

	if (!in) {
		ISP_LOGE("init param is null");
		ret = ISP_PARAM_NULL;
		return ret;
	}
	ISP_CHECK_HANDLE_VALID(adpt_handle);
	cxt->pd_set_buffer = in->pd_set_buffer;
	cxt->pd_open = 1;
	ISP_LOGI("pd open %p", cxt->pd_set_buffer);
	return 0;
}

static cmr_int pdafaltek_adpt_set_close(cmr_handle adpt_handle, struct pdaf_ctrl_param_in *in)
{
	cmr_int ret = -ISP_ERROR;
	struct pdaf_altek_context *cxt = (struct pdaf_altek_context *)adpt_handle;

	ISP_CHECK_HANDLE_VALID(adpt_handle);
	UNUSED(in);
	cxt->pd_set_buffer = NULL;
	cxt->pd_open = 0;
	ISP_LOGI("pd close");
	return 0;
}

static cmr_int pdafaltek_adpt_set_config(cmr_handle adpt_handle, struct pdaf_ctrl_param_in *in)
{
	cmr_int ret = -ISP_ERROR;
	struct pdaf_altek_context *cxt = (struct pdaf_altek_context *)adpt_handle;

	ISP_CHECK_HANDLE_VALID(adpt_handle);
	if (!in && !in->pd_config) {
		ISP_LOGE("init param is null, input_ptr is %p", in);
		ret = ISP_PARAM_NULL;
		return ret;
	}

	cxt->token_id = in->pd_config->token_id;
	ISP_LOGI("token id %d, type: %x", cxt->token_id, in->pd_config->type);
	if (in->pd_config->type & ISP3A_PD_CONFIG_ENABLE) {
		cxt->pd_enable = in->pd_config->pd_enable;
		ISP_LOGI("pd enable %d", cxt->pd_enable);
	}
	if (in->pd_config->type & ISP3A_PD_CONFIG_ROI) {
		//cxt->roi.m_wLeft = in->pd_config->pd_roi.start_x;
		//cxt->roi.m_wTop = in->pd_config->pd_roi.start_y;
		//cxt->roi.m_wWidth = in->pd_config->pd_roi.width;
		//cxt->roi.m_wHeight = in->pd_config->pd_roi.height;
		ISP_LOGV("set roi %d %d %d %d", in->pd_config->pd_roi.start_x, in->pd_config->pd_roi.start_y,
					in->pd_config->pd_roi.width, in->pd_config->pd_roi.height);
	}
	if (in->pd_config->type & ISP3A_PD_CONFIG_RESET) {
		cxt->frame_id = 0;
		ret = cxt->ops.reset();
		ISP_LOGI("pd reset");
	}

	return 0;
}

static cmr_int pdafaltek_adpt_set_enable(cmr_handle adpt_handle, struct pdaf_ctrl_param_in *in)
{
	cmr_int ret = -ISP_ERROR;
	struct pdaf_altek_context *cxt = (struct pdaf_altek_context *)adpt_handle;

	ISP_CHECK_HANDLE_VALID(adpt_handle);
	if (!in) {
		ISP_LOGE("init param is null");
		ret = ISP_PARAM_NULL;
		return ret;
	}
	cxt->pd_enable = in->pd_enable;
	ISP_LOGI("pd enable %d", cxt->pd_enable);

	return 0;
}

static cmr_int pdafaltek_adpt_get_busy(cmr_handle adpt_handle, struct pdaf_ctrl_param_out *out)
{
	cmr_int ret = -ISP_ERROR;
	struct pdaf_altek_context *cxt = (struct pdaf_altek_context *)adpt_handle;

	ISP_CHECK_HANDLE_VALID(adpt_handle);

	out->is_busy = cxt->is_busy;
	return 0;
}

static cmr_int pdafaltek_libops_init(cmr_handle adpt_handle)
{
	cmr_int ret = -ISP_ERROR;
	struct pdaf_altek_context *cxt = (struct pdaf_altek_context *)adpt_handle;

	ISP_CHECK_HANDLE_VALID(adpt_handle);
	ret = load_pdaltek_library(cxt);
	if (ret) {
		ISP_LOGE("failed to load library");
		goto error_load_lib;
	}

	ret = load_pdaltek_extract_library(cxt);
	if (ret) {
		ISP_LOGE("failed to load extract library");
		goto error_load_extract_lib;
	}

	return ret;

error_load_extract_lib:
	unload_pdaltek_library(cxt);
error_load_lib:
	return ret;
}

static cmr_int pdafaltek_libops_deinit(cmr_handle adpt_handle)
{
	cmr_int ret = -ISP_ERROR;

	ISP_CHECK_HANDLE_VALID(adpt_handle);
	ret = unload_pdaltek_extract_library(adpt_handle);
	ret = unload_pdaltek_library(adpt_handle);

	return ret;
}

cmr_int pdafaltek_get_pd_pack_bin(cmr_handle adpt_handle, const cmr_s8 *sensor_name)
{
	cmr_int ret = ISP_SUCCESS;
	struct pdaf_altek_context *cxt = (struct pdaf_altek_context *)adpt_handle;
	FILE *fp = NULL;
	cmr_u8 file_name[100];

	/* get pd pack bin */
	ISP_LOGI("sensor_name %s", sensor_name);
	sprintf((void *)&file_name[0], "/system/lib/tuning/%s_pdotp.bin", sensor_name);
	fp = fopen((void *)&file_name[0], "rb");
	if (NULL == fp) {
		ISP_LOGI("failed to open pdotp bin");
		goto exit;
	}
	fread(cxt->pdotp_pack_data, 1, PD_OTP_PACK_SIZE, fp);
	fclose(fp);
exit:
	return ret;
}

static cmr_int pdafaltek_adpt_init(void *in, void *out, cmr_handle *adpt_handle)
{
	cmr_int ret = -ISP_ERROR;
	struct pdaf_ctrl_init_in *in_p = (struct pdaf_ctrl_init_in *)in;
	struct pdaf_ctrl_init_out *out_p = (struct pdaf_ctrl_init_out *)out;
	struct pdaf_altek_context *cxt = NULL;
	struct sensor_otp_af_info *otp_af_info = NULL;
	cmr_u32 pd_in_size = 0;
	cmr_u32 i = 0;

	out_p->init_success = 0;

	if (!in_p || !adpt_handle) {
		ISP_LOGE("init param %p is null !!!", in_p);
		ret = ISP_PARAM_NULL;
		return ret;
	}

	if (/*NULL == in_p->pdaf_otp.otp_data || */NULL == in_p->pd_info) {
		ISP_LOGE("failed to get pd data");
		ret = ISP_PARAM_NULL;
		return ret;
	}

	*adpt_handle = NULL;
	cxt = (struct pdaf_altek_context *)malloc(sizeof(*cxt));
	if (NULL == cxt) {
		ISP_LOGE("failed to malloc cxt");
		ret = -ISP_ALLOC_ERROR;
		goto exit;
	}

	cmr_bzero(cxt, sizeof(*cxt));
	cxt->caller_handle = in_p->caller_handle;
	cxt->camera_id = in_p->camera_id;
	cxt->pdaf_support = in_p->pdaf_support;
	cxt->cb_ops = in_p->pdaf_ctrl_cb_ops;
	if (SENSOR_PDAF_TYPE3_ENABLE == in_p->pdaf_support)
		cxt->extract_enable = 1;
	ret = pdafaltek_adpt_set_pd_info(cxt, in_p->pd_info);
	if (ret) {
		ISP_LOGE("failed to set pd info");
		goto exit;
	}
	pd_in_size = in_p->pd_info->pd_block_num_x * in_p->pd_info->pd_block_num_y *
			in_p->pd_info->pd_density_x * in_p->pd_info->pd_density_y * sizeof(cmr_u16);

	ISP_LOGI("pd_in_size = %d", pd_in_size);
	if (0 == pd_in_size) {
		ISP_LOGE("failed to get pd size");
		ret = -ISP_PARAM_ERROR;
		goto exit;
	}
	cxt->pd_left = malloc(pd_in_size);
	if (NULL == cxt->pd_left) {
		ISP_LOGE("failed to malloc");
		ret = -ISP_ALLOC_ERROR;
		goto exit;
	}
	cxt->pd_right = malloc(pd_in_size);
	if (NULL == cxt->pd_right) {
		ISP_LOGE("failed to malloc");
		ret = -ISP_ALLOC_ERROR;
		goto exit;
	}

	cmr_bzero(cxt->pd_left, pd_in_size);
	cmr_bzero(cxt->pd_right, pd_in_size);
	otp_af_info = (struct sensor_otp_af_info *) in_p->af_otp.otp_data;
	if (otp_af_info) {
		cxt->pd_reg_in.tSensorInfo.uwInfVCM = (double)otp_af_info->infinite_cali;
		cxt->pd_reg_in.tSensorInfo.uwMacroVCM = (double)otp_af_info->macro_cali;
	} else {
		/* defaut value from altek */
		cxt->pd_reg_in.tSensorInfo.uwInfVCM = 401.0;//294;
		cxt->pd_reg_in.tSensorInfo.uwMacroVCM = 634.0;//570;
	}
	ISP_LOGI("infinite = %lf, macro = %lf",
			 cxt->pd_reg_in.tSensorInfo.uwInfVCM,
			 cxt->pd_reg_in.tSensorInfo.uwMacroVCM);

	if ((SENSOR_VENDOR_SS_BEGIN < in_p->pd_info->vendor_type) &&
	    (SENSOR_VENDOR_SS_END > in_p->pd_info->vendor_type)) {
		cxt->pd_reg_in.tSensorInfo.dSensorID = 0;
		cxt->vendor = 0;
	} else if ((SENSOR_VENDOR_IMX_BEGIN < in_p->pd_info->vendor_type) &&
		   (SENSOR_VENDOR_IMX_END > in_p->pd_info->vendor_type)) {
		cxt->pd_reg_in.tSensorInfo.dSensorID = 1;
		cxt->vendor = 1;
	} else {
		ISP_LOGE("failed to match pdaf support lib");
		goto exit;
	}

	ret = pdafaltek_libops_init(cxt);
	if (ret) {
		ISP_LOGE("failed to init library and ops");
		goto exit;
	}

	/* show version */
	pdafaltek_adpt_get_version(cxt);

	//pdafaltek_get_pd_pack_bin(cxt, (const cmr_s8 *)in_p->name);
	/* init lib */
	for (i = 0; i < ISP_INDEX_MAX; i++) {
		cxt->tuning_param[i] = in_p->tuning_param[i];
	}
	ISP_LOGI("otp ptr %p size %ld", in_p->pdaf_otp.otp_data, in_p->pdaf_otp.size);
	ret = cxt->ops.init(in_p->pdaf_otp.otp_data, NULL, 0, in_p->tuning_param[0]);
	if (ret) {
		ISP_LOGE("failed to init lib %ld", ret);
		goto error_lib_init;
	}

	*adpt_handle = (cmr_handle)cxt;
	cxt->is_busy = 0;
	cxt->report_data.pd_reg_out = (void *)cxt->pd_reg_out;
	cxt->report_data.pd_reg_size = PD_REG_OUT_SIZE;
	out_p->init_success = 1;
	return ret;

error_lib_init:
	pdafaltek_libops_deinit(cxt);
exit:
	if (cxt) {
		if (cxt->pd_right) {
			free(cxt->pd_right);
			cxt->pd_right = NULL;
		}
		if (cxt->pd_left) {
			free(cxt->pd_left);
			cxt->pd_left = NULL;
		}
		free(cxt);
		cxt = NULL;
	}
	return ret;
}

static cmr_int pdafaltek_adpt_deinit(cmr_handle adpt_handle)
{
	struct pdaf_altek_context *cxt = (struct pdaf_altek_context *)adpt_handle;
	cmr_int ret = ISP_SUCCESS;

	ISP_LOGI("cxt = %p", cxt);
	if (cxt) {
		/* deinit lib */
		ret = cxt->ops.deinit();
		ret = pdafaltek_libops_deinit(cxt);
		if (cxt->pd_right) {
			free(cxt->pd_right);
			cxt->pd_right = NULL;
		}
		if (cxt->pd_left) {
			free(cxt->pd_left);
			cxt->pd_left = NULL;
		}
		cmr_bzero(cxt, sizeof(*cxt));
		free(cxt);
		cxt = NULL;
	}

	return ret;
}

cmr_int pdafaltek_dump_file(cmr_u32 index, cmr_u32 img_fmt, cmr_u32 width, cmr_u32 height, void *addr, cmr_u32 vcm)
{
	cmr_int ret = -ISP_ERROR;
	char file_name[80];
	char tmp_str[10];
	FILE *fp = NULL;

	ISP_LOGI("index %d format %d width %d heght %d", index, img_fmt, width, height);

	cmr_bzero(file_name, 80);
	strcpy(file_name, "/data/mlog/");
	sprintf(tmp_str, "%d", width);
	strcat(file_name, tmp_str);
	strcat(file_name, "X");
	sprintf(tmp_str, "%d", height);
	strcat(file_name, tmp_str);

	if (PDAF_DATA_TYPE_RAW == img_fmt) {
		strcat(file_name, "_");
		sprintf(tmp_str, "%d", index);
		strcat(file_name, tmp_str);
		strcat(file_name, ".mipi_raw");
		ISP_LOGI("file name %s", file_name);

		fp = fopen(file_name, "wb");
		if (NULL == fp) {
			ISP_LOGI("can not open file: %s", file_name);
			return 0;
		}

		fwrite((void *)addr, 1, (cmr_u32)(width * height * 5 / 4), fp);
		fclose(fp);
	} else if (PDAF_DATA_TYPE_LEFT == img_fmt) {
		strcat(file_name, "_");
		sprintf(tmp_str, "%d", index);
		strcat(file_name, tmp_str);
		strcat(file_name, "_");
		sprintf(tmp_str, "%d", vcm);
		strcat(file_name, tmp_str);
		strcat(file_name, ".left");
		ISP_LOGI("file name %s", file_name);

		fp = fopen(file_name, "wb");
		if (NULL == fp) {
			ISP_LOGI("can not open file: %s", file_name);
			return 0;
		}

		fwrite((void *)addr, 1, (cmr_u32)(width * height * 2), fp);
		fclose(fp);
	} else if (PDAF_DATA_TYPE_RIGHT == img_fmt) {
		strcat(file_name, "_");
		sprintf(tmp_str, "%d", index);
		strcat(file_name, tmp_str);
		strcat(file_name, "_");
		sprintf(tmp_str, "%d", vcm);
		strcat(file_name, tmp_str);
		strcat(file_name, ".right");
		ISP_LOGI("file name %s", file_name);

		fp = fopen(file_name, "wb");
		if (NULL == fp) {
			ISP_LOGI("can not open file: %s", file_name);
			return 0;
		}

		fwrite((void *)addr, 1, (cmr_u32)(width * height * 2), fp);
		fclose(fp);
	} else if (PDAF_DATA_TYPE_OUT == img_fmt) {
		strcat(file_name, "_");
		sprintf(tmp_str, "%d", index);
		strcat(file_name, tmp_str);
		strcat(file_name, ".out");
		ISP_LOGI("file name %s", file_name);

		fp = fopen(file_name, "wb");
		if (NULL == fp) {
			ISP_LOGI("can not open file: %s", file_name);
			return 0;
		}

		fwrite((void *)addr, 1, PD_REG_OUT_SIZE, fp);
		fclose(fp);
	} else if (PDAF_DATA_TYPE_RAW_OUT == img_fmt) {
		strcat(file_name, "_");
		sprintf(tmp_str, "%d", index);
		strcat(file_name, tmp_str);
		strcat(file_name, ".raw");
		ISP_LOGI("file name %s", file_name);
		fp = fopen(file_name, "wb");
		if (NULL == fp) {
			ISP_LOGI("can not open file: %s", file_name);
			return 0;
		}
		fwrite((void *)addr, 1, (cmr_u32)(width * height * 5 / 4), fp);
		fclose(fp);
	}
	return 0;
}

static cmr_int pdafaltek_adpt_extract_type3(cmr_handle adpt_handle, struct pd_raw_info *pd_raw, alGE_RECT *pdroi)
{
	cmr_int ret = -ISP_ERROR;
	struct pdaf_altek_context *cxt = (struct pdaf_altek_context *)adpt_handle;
	cmr_u16 image_width;
	cmr_u16 image_height;
	char value[PROPERTY_VALUE_MAX];

	if (NULL == pd_raw->addr) {
		ISP_LOGE("init param is null !!!");
		ret = ISP_PARAM_NULL;
		return ret;
	}

	image_width = pd_raw->roi.image_width;
	image_height = pd_raw->roi.image_height;
	if (image_width == pd_raw->roi.trim_width
		&& image_height == pd_raw->roi.trim_height) {
		cxt->pd_info.pd_raw_crop_en = 0;
	} else {
		cxt->pd_info.pd_raw_crop_en = 1;
	}
	if (cxt->pd_info.pd_raw_crop_en) {
		cxt->roi.m_wLeft = pd_raw->roi.trim_start_x;
		cxt->roi.m_wTop = pd_raw->roi.trim_start_y;
		cxt->roi.m_wWidth = pd_raw->roi.trim_width;
		cxt->roi.m_wHeight = pd_raw->roi.trim_height;
	} else {
		cxt->roi.m_wLeft = pd_raw->roi.trim_width / 4;
		cxt->roi.m_wTop = pd_raw->roi.trim_height / 4;
		cxt->roi.m_wWidth = pd_raw->roi.trim_width / 2;
		cxt->roi.m_wHeight = pd_raw->roi.trim_height / 2;
	}

	property_get("debug.camera.dump.pdaf.raw", (char *)value, "0");
	if (atoi(value)) {
		pdafaltek_dump_file(cxt->frame_id, PDAF_DATA_TYPE_RAW,
				    pd_raw->roi.trim_width,
				    pd_raw->roi.trim_height,
				    pd_raw->addr, cxt->pd_reg_in.dcurrentVCM);
	}
	ISP_LOGV("pd addr %p, width %d, height %d,roi %d, %d, %d, %d,",
		 pd_raw->addr, image_width, image_height,
		 cxt->roi.m_wLeft, cxt->roi.m_wTop,
		 cxt->roi.m_wWidth, cxt->roi.m_wHeight);
	ISP_LOGV("pd_offset_x %d, pd_offset_y %d, pd_pitch_x %d, pd_pitch_y %d, pd_density_x %d, pd_density_y %d, pd_block_num_x %d, pd_block_num_y %d, pd_pos_size %d, pd_raw_crop_en %d",
			cxt->pd_info.pd_offset_x,cxt->pd_info.pd_offset_y,cxt->pd_info.pd_pitch_x, cxt->pd_info.pd_pitch_y,
			cxt->pd_info.pd_density_x,cxt->pd_info.pd_density_y,cxt->pd_info.pd_block_num_x,cxt->pd_info.pd_block_num_y,
			cxt->pd_info.pd_pos_size,cxt->pd_info.pd_raw_crop_en);
	ISP_LOGV("pd_pos_r.pd_pos_x %d pd_pos_r.pd_pos_y %d pd_pos_l.pd_pos_x %d pd_pos_l.pd_pos_y %d",
			cxt->pd_info.pd_pos_r->pd_pos_x, cxt->pd_info.pd_pos_r->pd_pos_y, cxt->pd_info.pd_pos_l->pd_pos_x, cxt->pd_info.pd_pos_l->pd_pos_y);

	ret = cxt->extract3_ops.extract(&cxt->pd_info,(cmr_u8 *)pd_raw->addr, &cxt->roi, image_width, image_height,
			(cmr_u16 *)cxt->pd_left, (cmr_u16 *)cxt->pd_right,(cmr_u16 *)&pdroi->m_wWidth, (cmr_u16 *)&pdroi->m_wHeight);
	ISP_LOGV("pd size %d, %d,", pdroi->m_wWidth, pdroi->m_wHeight);
	if (ret) {
		ISP_LOGE("failed to extract pd data %ld", ret);
		goto exit;
	}
	ISP_LOGV("pd left addr %p, right addr %p", cxt->pd_left, cxt->pd_right);

	property_get("debug.camera.dump.pdaf.data", (char *)value, "0");
	if (atoi(value)) {
		pdafaltek_dump_file(cxt->frame_id, PDAF_DATA_TYPE_LEFT,
				    pdroi->m_wWidth,
				    pdroi->m_wHeight,
				    cxt->pd_left, cxt->pd_reg_in.dcurrentVCM);

		pdafaltek_dump_file(cxt->frame_id, PDAF_DATA_TYPE_RIGHT,
				    pdroi->m_wWidth,
				    pdroi->m_wHeight,
				    cxt->pd_right, cxt->pd_reg_in.dcurrentVCM);
		pdafaltek_dump_file(cxt->frame_id, PDAF_DATA_TYPE_RAW_OUT,
				pd_raw->roi.trim_width,
				pd_raw->roi.trim_height,
				pd_raw->addr, cxt->pd_reg_in.dcurrentVCM);
	}
exit:
	return ret;
}

static cmr_int pdafaltek_adpt_type3_process(cmr_handle adpt_handle, void *in, void *out)
{
	cmr_int ret = -ISP_ERROR;
	struct pdaf_altek_context *cxt = (struct pdaf_altek_context *)adpt_handle;
	struct pdaf_ctrl_process_in *proc_in = (struct pdaf_ctrl_process_in *)in;
	struct pdaf_ctrl_callback_in callback_in;
	alGE_RECT pdroi;
	DataBit bit;
	nsecs_t cal_time_begin = 0;
	nsecs_t cal_time_end = 0;
	char value[PROPERTY_VALUE_MAX];

	UNUSED(out);
	cmr_bzero(&pdroi, sizeof(pdroi));
	if (!in) {
		ISP_LOGE("init param %p is null !!!", in);
		ret = ISP_PARAM_NULL;
		return ret;
	}

	ISP_CHECK_HANDLE_VALID(adpt_handle);
	cmr_bzero(&callback_in, sizeof(callback_in));

	if (!cxt->pd_enable || !cxt->pd_open) {
		ISP_LOGI("pd enable %d, open %d", cxt->pd_enable, cxt->pd_open);
		ret = ISP_SUCCESS;
		goto exit;
	}
	cxt->is_busy = 1;

	cxt->pd_reg_in.dcurrentVCM = proc_in->dcurrentVCM;
	cxt->pd_reg_in.dBv = (float)proc_in->dBv / 1000.0;

	if (cxt->extract_enable)
		pdafaltek_adpt_extract_type3(cxt, &proc_in->pd_raw, &pdroi);

	switch (proc_in->bit) {
	case PD_DATA_BIT_8:
		bit = SRCIMG_8; // not support yet
		break;
	case PD_DATA_BIT_10:
		bit = SRCIMG_10;
		break;
	case PD_DATA_BIT_12:
		bit = SRCIMG_12;
		break;
	default:
		bit = SRCIMG_10;
		break;
	}

	ISP_LOGV("bit %d, roi %d, %d, %d, %d,",
			bit, pdroi.m_wLeft, pdroi.m_wTop, pdroi.m_wWidth, pdroi.m_wHeight);

	ISP_LOGV("dBv %f, vcm current %d",
			cxt->pd_reg_in.dBv, cxt->pd_reg_in.dcurrentVCM);

	ISP_LOGV("pd report data pd_value befor: %f, reg_out %p",
			cxt->report_data.pd_value, cxt->report_data.pd_reg_out);
	ISP_LOGV("pd left addr %p, right addr %p", cxt->pd_left, cxt->pd_right);

	ISP_LOGV("bit %d, roi %d, %d, %d, %d,",
			bit, pdroi.m_wLeft, pdroi.m_wTop, pdroi.m_wWidth, pdroi.m_wHeight);

	ISP_LOGV("pd_reg_in %p", &cxt->pd_reg_in);
	cal_time_begin = systemTime(CLOCK_MONOTONIC);
	ret = cxt->ops.calc(&cxt->report_data.pd_value,
			    cxt->report_data.pd_reg_out,
			    cxt->pd_left, cxt->pd_right,
			    pdroi.m_wWidth, pdroi.m_wHeight,
			    pdroi,
			    bit,
			    &cxt->pd_reg_in);
	if (ret) {
		ISP_LOGE("failed to calc pd data %ld", ret);
		goto exit;
	}
	cal_time_end = systemTime(CLOCK_MONOTONIC);
	ISP_LOGI("pd report data pd_value after: %f, cal delta %ld",
			cxt->report_data.pd_value,
			(cmr_uint)(cal_time_end - cal_time_begin) / 1000);

	property_get("debug.camera.dump.pdaf.out", (char *)value, "0");
	if (atoi(value)) {
		pdafaltek_dump_file(cxt->frame_id, PDAF_DATA_TYPE_OUT,
				    pdroi.m_wWidth,
				    pdroi.m_wHeight,
				    cxt->report_data.pd_reg_out,
				    cxt->pd_reg_in.dcurrentVCM);
	}
	cxt->report_data.token_id = cxt->token_id;
	cxt->report_data.frame_id = cxt->frame_id;
	cxt->report_data.enable = cxt->pd_enable;
	cxt->report_data.time_stamp.sec = proc_in->pd_raw.sec;
	cxt->report_data.time_stamp.usec = proc_in->pd_raw.usec;

	callback_in.proc_out.pd_report_data = cxt->report_data;
	/*call back to haf lib*/
	if (cxt->cb_ops.call_back)
		ret = (cxt->cb_ops.call_back)(cxt->caller_handle, &callback_in);

	cxt->frame_id++; //= proc_in->pd_raw.frame_id;

exit:
	if (cxt->pd_set_buffer)
		ret = (cxt->pd_set_buffer)(&proc_in->pd_raw.pd_in);

	cxt->is_busy = 0;
	return ret;
}

static cmr_int pdafaltek_adpt_extract_type2(cmr_handle adpt_handle, cmr_u8 *addr)
{
	cmr_int ret = -ISP_ERROR;
	struct pdaf_altek_context *cxt = (struct pdaf_altek_context *)adpt_handle;
	struct altek_pd_size pd_size_info;

	pd_size_info.total_width = cxt->type2_info.width;
	pd_size_info.total_height = cxt->type2_info.height;
	pd_size_info.left_width = cxt->type2_info.width;
	pd_size_info.left_height = cxt->type2_info.height / 2;
	pd_size_info.right_width = cxt->type2_info.width;
	pd_size_info.right_height = cxt->type2_info.height / 2;

	ret = cxt->extract2_ops.extract(addr, pd_size_info,
					cxt->vendor,
					(cmr_u16 *)cxt->pd_left,
					(cmr_u16 *)cxt->pd_right);

	if (ret) {
		ISP_LOGE("failed to extract pd data %ld", ret);
		goto exit;
	}
exit:
	return ret;
}

static cmr_int pdafaltek_adpt_type2_process(cmr_handle adpt_handle, void *in, void *out)
{
	cmr_int ret = ISP_SUCCESS;
	struct pdaf_altek_context *cxt = (struct pdaf_altek_context *)adpt_handle;
	struct pdaf_ctrl_process_in *proc_in = (struct pdaf_ctrl_process_in *)in;
	UNUSED(out);
	alGE_RECT pdroi;
	nsecs_t cal_time_begin = 0;
	nsecs_t cal_time_end = 0;
	struct pdaf_ctrl_callback_in callback_in;
	cmr_u32 pd_width = 0;
	cmr_u32 pd_height = 0;

	memset(&pdroi, 0x00, sizeof(pdroi));
	memset(&callback_in, 0x00, sizeof(callback_in));

	if (!cxt->pd_enable || !cxt->pd_open) {
		ISP_LOGI("pd enable %d, open %d", cxt->pd_enable, cxt->pd_open);
		ret = ISP_SUCCESS;
		goto exit;
	}
	cxt->is_busy = 1;

	cxt->pd_reg_in.dcurrentVCM = proc_in->dcurrentVCM;
	cxt->pd_reg_in.dBv = (float)proc_in->dBv / 1000.0;

	cal_time_begin = systemTime(CLOCK_MONOTONIC);
	pdafaltek_adpt_extract_type2(cxt, proc_in->pd_raw.addr);
	cal_time_end = systemTime(CLOCK_MONOTONIC);
	ISP_LOGI("extract delta %ld", (cmr_uint)(cal_time_end - cal_time_begin) / 1000);

	pd_width = cxt->pd_info.pd_block_num_x * cxt->pd_info.pd_density_x;
	pd_height = cxt->pd_info.pd_block_num_y * cxt->pd_info.pd_density_y;
	pdroi.m_wLeft = pd_width / 4;
	pdroi.m_wTop =  pd_height / 4;
	pdroi.m_wWidth = pd_width / 2;
	pdroi.m_wHeight = pd_height / 2;

	cal_time_begin = systemTime(CLOCK_MONOTONIC);

	ret = cxt->ops.calc(&cxt->report_data.pd_value,
			    cxt->report_data.pd_reg_out,
			    cxt->pd_left, cxt->pd_right,
			    pd_width, pd_height,
			    pdroi,
			    SRCIMG_10,
			    &cxt->pd_reg_in);

	cal_time_end = systemTime(CLOCK_MONOTONIC);
	ISP_LOGI("cal delta %lu roi %d %d %d %d", (cmr_uint)(cal_time_end - cal_time_begin) / 1000,
		 pdroi.m_wLeft, pdroi.m_wTop, pdroi.m_wWidth, pdroi.m_wHeight);

	cxt->report_data.token_id = cxt->token_id;
	cxt->report_data.frame_id = cxt->frame_id;
	cxt->report_data.enable = cxt->pd_enable;
	cxt->report_data.time_stamp.sec = proc_in->pd_raw.sec;
	cxt->report_data.time_stamp.usec = proc_in->pd_raw.usec;

	callback_in.proc_out.pd_report_data = cxt->report_data;
	/*call back to haf lib*/
	if (cxt->cb_ops.call_back)
		ret = (cxt->cb_ops.call_back)(cxt->caller_handle, &callback_in);

	cxt->frame_id++; //= proc_in->pd_raw.frame_id;

exit:
	if (cxt->pd_set_buffer)
		ret = (cxt->pd_set_buffer)(&proc_in->pd_raw.pd_in);

	cxt->is_busy = 0;
	return ret;
}

static cmr_int pdafaltek_adpt_process(cmr_handle adpt_handle, void *in, void *out)
{
	cmr_int ret = -ISP_ERROR;
	struct pdaf_altek_context *cxt = (struct pdaf_altek_context *)adpt_handle;

	ISP_CHECK_HANDLE_VALID(adpt_handle);

	switch (cxt->pdaf_support) {
	case SENSOR_PDAF_TYPE2_ENABLE:
		ret = pdafaltek_adpt_type2_process(cxt, in, out);
		break;
	case SENSOR_PDAF_TYPE3_ENABLE:
		ret = pdafaltek_adpt_type3_process(cxt, in, out);
		break;
	}

	return ret;
}

static cmr_int pdafaltek_adpt_ioctrl(cmr_handle adpt_handle, cmr_int cmd,
				   void *in, void *out)
{
	cmr_int ret = -ISP_ERROR;

	ISP_CHECK_HANDLE_VALID(adpt_handle);
	ISP_LOGV("cmd = %ld", cmd);

	switch (cmd) {
	case PDAF_CTRL_CMD_SET_OPEN:
		ret = pdafaltek_adpt_set_open(adpt_handle, in);
		break;
	case PDAF_CTRL_CMD_SET_CLOSE:
		ret = pdafaltek_adpt_set_close(adpt_handle, in);
		break;
	case PDAF_CTRL_CMD_SET_CONFIG:
		ret = pdafaltek_adpt_set_config(adpt_handle, in);
		break;
	case PDAF_CTRL_CMD_SET_ENABLE:
		ret = pdafaltek_adpt_set_enable(adpt_handle, in);
		break;
	case PDAF_CTRL_CMD_GET_BUSY:
		ret = pdafaltek_adpt_get_busy(adpt_handle, out);
		break;
	default:
		ISP_LOGE("failed to case cmd = %ld", cmd);
		break;
	}

	return ret;
}

static struct adpt_ops_type pdaf_altek_adpt_ops = {
	.adpt_init = pdafaltek_adpt_init,
	.adpt_deinit = pdafaltek_adpt_deinit,
	.adpt_process = pdafaltek_adpt_process,
	.adpt_ioctrl = pdafaltek_adpt_ioctrl,
};

static struct isp_lib_config pdaf_altek_lib_info = {
	.product_id = 0,
	.version_id = 0,
	.product_name_low = "",
	.product_name_high = "",
};

struct adpt_register_type pdaf_altek_adpt_info = {
	.lib_type = ADPT_LIB_PDAF,
	.lib_info = &pdaf_altek_lib_info,
	.ops = &pdaf_altek_adpt_ops,
};
