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
#define LOG_TAG "isp_mw"
#define ATRACE_TAG (ATRACE_TAG_CAMERA | ATRACE_TAG_HAL)

#include <stdlib.h>
#include <cutils/trace.h>
#include "cutils/properties.h"
#include <unistd.h>
#include "isp_mw.h"
#include "isp_3a_fw.h"
#include "isp_dev_access.h"
#include "isp_3a_adpt.h"
/**************************************** MACRO DEFINE *****************************************/
#define ISP_MW_FILE_NAME_LEN          100


/************************************* INTERNAL DATA TYPE ***************************************/
struct isp_mw_tunng_file_info {
	void *isp_3a_addr;
	void *isp_shading_addr;
	cmr_u32 isp_3a_size;
	cmr_u32 isp_shading_size;
	cmr_u32 scene_bin_size;
	void *ae_tuning_addr;
	void *awb_tuning_addr;
	void *af_tuning_addr;
	void *scene_bin_addr;
	struct bin2_sep_info isp_dev_bin_info;
	void *isp_caf_addr;
	cmr_u32 isp_caf_size;
	void *isp_pdaf_addr;
	cmr_u32 isp_pdaf_size;
	struct sensor_otp_iso_awb_info iso_awb_info;
};

struct isp_mw_pdaf_info {
	void *pdaf_cbc_addr;
	cmr_u32 pdaf_cbc_size;
	cmr_u8  pdaf_supported;
};

struct isp_mw_context {
	cmr_u32 camera_id;
	cmr_u16 is_refocus;
	cmr_u32 is_inited;
	cmr_u32 idx;
	void    *isp_onebin_addr;
	cmr_u32 isp_onebin_size;
	void    *isp_onebin_slv_addr;
	cmr_u32 isp_onebin_slv_size;
	cmr_handle caller_handle;
	cmr_handle isp_3a_handle;
	cmr_handle isp_dev_handle;
	cmr_malloc alloc_cb;
	cmr_free   free_cb;
	proc_callback caller_callback;
	struct isp_init_param input_param;
	struct isp_mw_tunng_file_info tuning_bin[ISP_INDEX_MAX];
	struct isp_mw_tunng_file_info tuning_bin_slv[ISP_INDEX_MAX];
	struct isp_mw_pdaf_info pdaf_info;
};

static cmr_int ispmw_create_thread(cmr_handle isp_mw_handle);
static void ispmw_dev_evt_cb(cmr_int evt, void *data, void *privdata);
/*************************************INTERNAK FUNCTION ***************************************/
cmr_int ispmw_create_thread(cmr_handle isp_mw_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;

	UNUSED(isp_mw_handle);
	return ret;
}

void ispmw_dev_evt_cb(cmr_int evt, void *data, void *privdata)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp_mw_context                       *cxt = (struct isp_mw_context *)privdata;

	if (!privdata) {
		ISP_LOGE("error,handle is NULL");
		goto exit;
	}

	switch (evt) {
	case ISP3A_DEV_STATICS_DONE:
	case ISP3A_DEV_SENSOR_SOF:
		ret = isp_3a_fw_receive_data(cxt->isp_3a_handle, evt, data);
		break;
	default:
		break;
	}
exit:
	ISP_LOGI("done");
}

void ispmw_dev_buf_cfg_evt_cb(cmr_handle isp_handle, isp_buf_cfg_evt_cb grab_event_cb)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp_mw_context                       *cxt = (struct isp_mw_context *)isp_handle;

	isp_dev_access_cfg_buf_evt_reg(cxt->isp_dev_handle, cxt->caller_handle, grab_event_cb);
}

cmr_int ispmw_parse_tuning_onebin(cmr_handle isp_mw_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp_mw_context                       *cxt = (struct isp_mw_context *)isp_mw_handle;
	struct bin1_sep_info                        a_bin1_info;
	struct bin2_sep_info                        a_bin2_info;
	struct bin3_sep_info                        a_bin3_info;
	cmr_int                                     i = 0;

	for (i=0; i < ISP_INDEX_MAX; i++) {
		ret =isp_separate_one_bin(( uint8 *) cxt->isp_onebin_addr,cxt->isp_onebin_size,
					  i,
					  &a_bin1_info,
					  &a_bin2_info,
					  &a_bin3_info);
		if (ret) {
			ISP_LOGE("failed to separate one bin ret=%ld ", ret);
		}
		cxt->tuning_bin[i].ae_tuning_addr = a_bin1_info.puc_ae_bin_addr;
		cxt->tuning_bin[i].awb_tuning_addr = a_bin1_info.puc_awb_bin_addr;
		cxt->tuning_bin[i].af_tuning_addr = a_bin1_info.puc_af_bin_addr;
		cxt->tuning_bin[i].isp_caf_addr = a_bin3_info.puc_caf_bin_addr;
		cxt->tuning_bin[i].isp_caf_size = a_bin3_info.uw_caf_bin_size;
		cxt->tuning_bin[i].isp_pdaf_addr = a_bin3_info.puc_pdaf_bin_addr;
		cxt->tuning_bin[i].isp_pdaf_size = a_bin3_info.uw_pdaf_bin_size;
		cxt->tuning_bin[i].isp_dev_bin_info.puc_shading_bin_addr = a_bin2_info.puc_shading_bin_addr;
		cxt->tuning_bin[i].isp_dev_bin_info.uw_shading_bin_size = a_bin2_info.uw_shading_bin_size;
		cxt->tuning_bin[i].isp_dev_bin_info.puc_irp_bin_addr = a_bin2_info.puc_irp_bin_addr;
		cxt->tuning_bin[i].isp_dev_bin_info.uw_irp_bin_size = a_bin2_info.uw_irp_bin_size;
		cxt->tuning_bin[i].scene_bin_addr = a_bin3_info.puc_scene_bin_addr;
		cxt->tuning_bin[i].scene_bin_size = a_bin3_info.uw_scene_bin_size;
		ISP_LOGI("onebin i=%ld ae_addr %p awb_addr %p af_addr %p caf_addr %p pdaf_addr %p scene_addr %p shading_addr %p shading_size %d irp_addr %p irp_size %d",
			 i,
			 cxt->tuning_bin[i].ae_tuning_addr,
			 cxt->tuning_bin[i].awb_tuning_addr,
			 cxt->tuning_bin[i].af_tuning_addr,
			 cxt->tuning_bin[i].isp_caf_addr,
			 cxt->tuning_bin[i].isp_pdaf_addr,
			 cxt->tuning_bin[i].scene_bin_addr,
			 cxt->tuning_bin[i].isp_dev_bin_info.puc_shading_bin_addr,
			 cxt->tuning_bin[i].isp_dev_bin_info.uw_shading_bin_size,
			 cxt->tuning_bin[i].isp_dev_bin_info.puc_irp_bin_addr,
			 cxt->tuning_bin[i].isp_dev_bin_info.uw_irp_bin_size );
	}
	return ret;
}

cmr_int ispmw_get_tuning_onebin(cmr_handle isp_mw_handle, const cmr_s8 *sensor_name)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp_mw_context                       *cxt = (struct isp_mw_context *)isp_mw_handle;
	FILE                                        *fp = NULL;
	cmr_u8                                      file_name[ISP_MW_FILE_NAME_LEN];

	/* get onebin bin */
	ISP_LOGI("sensor_name %s camera_id =%d", sensor_name, cxt->camera_id);
	sprintf((void *)&file_name[0], "/system/lib/tuning/%s_tuning.bin", sensor_name);
	ISP_LOGV("sensor_name %s", &file_name[0]);
	fp = fopen((void *)&file_name[0], "rb");
	if (NULL == fp) {
		ISP_LOGE("failed to open tuning bin");
		ret = -ISP_ERROR;
		goto exit;
	}
	fseek(fp, 0, SEEK_END);
	cxt->isp_onebin_size = ftell(fp);
	if (0 == cxt->isp_onebin_size) {
		fclose(fp);
		ret = -ISP_ERROR;
		goto exit;
	}
	fseek(fp, 0, SEEK_SET);
	cxt->isp_onebin_addr = malloc(cxt->isp_onebin_size);
	if (NULL == cxt->isp_onebin_addr) {
		fclose(fp);
		ISP_LOGE("failed to malloc");
		ret = -ISP_ERROR;
		goto exit;
	}
	if (cxt->isp_onebin_size != fread(cxt->isp_onebin_addr, 1, cxt->isp_onebin_size, fp)) {
		fclose(fp);
		ISP_LOGE("failed to read one bin");
		ret = -ISP_ERROR;
		goto exit;
	}
	fclose(fp);

exit:
	if (ret) {
		if (cxt->isp_onebin_addr) {
			free(cxt->isp_onebin_addr);
			cxt->isp_onebin_addr = NULL;
			cxt->isp_onebin_size = 0;
		}
	} else {
		ISP_LOGI("one bin size = %d",cxt->isp_onebin_size);
	}
	return ret;
}

cmr_int ispmw_parse_tuning_onebin_slv(cmr_handle isp_mw_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp_mw_context                       *cxt = (struct isp_mw_context *)isp_mw_handle;
	struct bin1_sep_info                        a_bin1_info;
	struct bin2_sep_info                        a_bin2_info;
	struct bin3_sep_info                        a_bin3_info;
	cmr_int                                     i = 0;

	for (i = 0; i < ISP_INDEX_MAX; i++) {
		ret = isp_separate_one_bin((uint8 *)cxt->isp_onebin_slv_addr,
					   cxt->isp_onebin_slv_size,
					   i,
					   &a_bin1_info,
					   &a_bin2_info,
					   &a_bin3_info);
		if (ret) {
			ISP_LOGE("failed to separate one bin ret=%ld", ret);
		}
		cxt->tuning_bin_slv[i].ae_tuning_addr = a_bin1_info.puc_ae_bin_addr;
		cxt->tuning_bin_slv[i].awb_tuning_addr = a_bin1_info.puc_awb_bin_addr;
		cxt->tuning_bin_slv[i].af_tuning_addr = a_bin1_info.puc_af_bin_addr;
		cxt->tuning_bin_slv[i].isp_caf_addr = a_bin3_info.puc_caf_bin_addr;
		cxt->tuning_bin_slv[i].isp_caf_size = a_bin3_info.uw_caf_bin_size;
		cxt->tuning_bin_slv[i].isp_pdaf_addr = a_bin3_info.puc_pdaf_bin_addr;
		cxt->tuning_bin_slv[i].isp_pdaf_size = a_bin3_info.uw_pdaf_bin_size;
		cxt->tuning_bin_slv[i].isp_dev_bin_info.puc_shading_bin_addr = a_bin2_info.puc_shading_bin_addr;
		cxt->tuning_bin_slv[i].isp_dev_bin_info.uw_shading_bin_size = a_bin2_info.uw_shading_bin_size;
		cxt->tuning_bin_slv[i].isp_dev_bin_info.puc_irp_bin_addr = a_bin2_info.puc_irp_bin_addr;
		cxt->tuning_bin_slv[i].isp_dev_bin_info.uw_irp_bin_size = a_bin2_info.uw_irp_bin_size;
		cxt->tuning_bin_slv[i].scene_bin_addr = a_bin3_info.puc_scene_bin_addr;
		cxt->tuning_bin_slv[i].scene_bin_size = a_bin3_info.uw_scene_bin_size;
		ISP_LOGV("onebin i=%ld ae_addr %p awb_addr=%p af_addr=%p caf_addr=%p pdaf_addr=%p shading_addr=%p shading_size=0x%x irp_addr=%p irp_size=0x%x",
			 i,
			 cxt->tuning_bin_slv[i].ae_tuning_addr,
			 cxt->tuning_bin_slv[i].awb_tuning_addr,
			 cxt->tuning_bin_slv[i].af_tuning_addr,
			 cxt->tuning_bin_slv[i].isp_caf_addr,
			 cxt->tuning_bin_slv[i].isp_pdaf_addr,
			 cxt->tuning_bin_slv[i].isp_dev_bin_info.puc_shading_bin_addr,
			 cxt->tuning_bin_slv[i].isp_dev_bin_info.uw_shading_bin_size,
			 cxt->tuning_bin_slv[i].isp_dev_bin_info.puc_irp_bin_addr,
			 cxt->tuning_bin_slv[i].isp_dev_bin_info.uw_irp_bin_size );
	}
	return ret;
}

cmr_int ispmw_get_tuning_onebin_slv(cmr_handle isp_mw_handle, const cmr_s8 *sensor_name)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp_mw_context                       *cxt = (struct isp_mw_context *)isp_mw_handle;
	FILE                                        *fp = NULL;
	cmr_u8                                      file_name[ISP_MW_FILE_NAME_LEN];

	ISP_LOGI("sensor_name %s camera_id =%d", sensor_name, cxt->camera_id);
	sprintf((void *)&file_name[0], "/system/lib/tuning/%s_tuning.bin", sensor_name);
	ISP_LOGV("sensor_name %s", &file_name[0]);
	fp = fopen((void *)&file_name[0], "rb");
	if (NULL == fp) {
		ISP_LOGE("failed to open tuning bin");
		ret = -ISP_ERROR;
		goto exit;
	}
	fseek(fp, 0, SEEK_END);
	cxt->isp_onebin_slv_size = ftell(fp);
	if (0 == cxt->isp_onebin_slv_size) {
		fclose(fp);
		ret = -ISP_ERROR;
		goto exit;
	}
	fseek(fp, 0, SEEK_SET);
	cxt->isp_onebin_slv_addr = malloc(cxt->isp_onebin_slv_size);
	if (NULL == cxt->isp_onebin_slv_addr) {
		fclose(fp);
		ISP_LOGE("failed to malloc");
		ret = -ISP_ERROR;
		goto exit;
	}
	if (cxt->isp_onebin_slv_size != fread(cxt->isp_onebin_slv_addr, 1, cxt->isp_onebin_slv_size, fp)) {
		fclose(fp);
		ISP_LOGE("failed to read one bin");
		ret = -ISP_ERROR;
		goto exit;
	}
	fclose(fp);

exit:
	if (ret) {
		if (cxt->isp_onebin_slv_addr) {
			free(cxt->isp_onebin_slv_addr);
			cxt->isp_onebin_slv_addr = NULL;
			cxt->isp_onebin_slv_size = 0;
		}
	} else {
		ISP_LOGI("one bin size = %d",cxt->isp_onebin_slv_size);
	}
	return ret;
}

cmr_int ispmw_get_pdaf_cbc_bin(cmr_handle isp_mw_handle, const cmr_s8 *sensor_name)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp_mw_context                       *cxt = (struct isp_mw_context *)isp_mw_handle;
	FILE                                        *fp = NULL;
	cmr_u8                                      file_name[ISP_MW_FILE_NAME_LEN];

	/* get caf tuning bin */
	sprintf((void *)&file_name[0], "/system/lib/tuning/%s_cbc.bin", sensor_name);
	fp = fopen((void *)&file_name[0], "rb");
	if (NULL == fp) {
		ISP_LOGE("failed to open caf bin");
		goto exit;
	}
	fseek(fp, 0, SEEK_END);
	cxt->pdaf_info.pdaf_cbc_size = ftell(fp);
	if (0 == cxt->pdaf_info.pdaf_cbc_size) {
		fclose(fp);
		goto exit;
	}
	fseek(fp, 0, SEEK_SET);
	cxt->pdaf_info.pdaf_cbc_addr = malloc(cxt->pdaf_info.pdaf_cbc_size);
	if (NULL == cxt->pdaf_info.pdaf_cbc_addr) {
		fclose(fp);
		ISP_LOGE("failed to malloc");
		goto exit;
	}
	if (cxt->pdaf_info.pdaf_cbc_size != fread(cxt->pdaf_info.pdaf_cbc_addr,
						  1,
						  cxt->pdaf_info.pdaf_cbc_size, fp)) {
		fclose(fp);
		ISP_LOGE("failed to read caf bin");
		ret = -ISP_ERROR;
		goto exit;
	}
	fclose(fp);
exit:
	if (ret) {
		if (cxt->pdaf_info.pdaf_cbc_addr) {
			free(cxt->pdaf_info.pdaf_cbc_addr);
			cxt->pdaf_info.pdaf_cbc_addr = NULL;
			cxt->pdaf_info.pdaf_cbc_size = 0;
		}
	} else {
		ISP_LOGI("pdaf_cbc_addr = %p, size = %d",
			cxt->pdaf_info.pdaf_cbc_addr, cxt->pdaf_info.pdaf_cbc_size);
	}

	return ret;
}

cmr_int ispmw_put_pdaf_cbc_bin(cmr_handle isp_mw_handle)
{
	struct isp_mw_context                       *cxt = (struct isp_mw_context *)isp_mw_handle;

	if (cxt->pdaf_info.pdaf_cbc_addr) {
		free(cxt->pdaf_info.pdaf_cbc_addr);
		cxt->pdaf_info.pdaf_cbc_addr = NULL;
		cxt->pdaf_info.pdaf_cbc_size = 0;
	}

	return 0;
}

cmr_int ispmw_put_tuning_bin(cmr_handle isp_mw_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp_mw_context                       *cxt = (struct isp_mw_context *)isp_mw_handle;

	if (cxt->isp_onebin_addr) {
		free(cxt->isp_onebin_addr);
		cxt->isp_onebin_addr = NULL;
		cxt->isp_onebin_size = 0;
	}
	if (cxt->isp_onebin_slv_addr) {
		free(cxt->isp_onebin_slv_addr);
		cxt->isp_onebin_slv_addr = NULL;
		cxt->isp_onebin_slv_size = 0;
	}

	return ret;
}

cmr_int isp_parse_bin_otp(cmr_handle isp_mw_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp_mw_context                       *cxt = (struct isp_mw_context *)isp_mw_handle;
	struct sensor_otp_iso_awb_info *otp_ptr = NULL;
	cmr_u8 bin_otp_offset = 114;

	ISP_CHECK_HANDLE_VALID(isp_mw_handle);

	if (cxt->tuning_bin[0].isp_dev_bin_info.puc_shading_bin_addr
		&& cxt->tuning_bin[0].isp_dev_bin_info.uw_shading_bin_size >= (bin_otp_offset + sizeof(struct sensor_otp_iso_awb_info))) {
		/*for bin otp data: shading addr offset +114*/
		otp_ptr = (struct sensor_otp_iso_awb_info *)(cxt->tuning_bin[0].isp_dev_bin_info.puc_shading_bin_addr + bin_otp_offset);
		cxt->tuning_bin[0].iso_awb_info.iso = otp_ptr->iso;
		cxt->tuning_bin[0].iso_awb_info.gain_r = otp_ptr->gain_r;
		cxt->tuning_bin[0].iso_awb_info.gain_g = otp_ptr->gain_g;
		cxt->tuning_bin[0].iso_awb_info.gain_b = otp_ptr->gain_b;
		ISP_LOGI("bin otp data iso=%d, r=%d,g=%d,b=%d",
			 cxt->tuning_bin[0].iso_awb_info.iso,
			 cxt->tuning_bin[0].iso_awb_info.gain_r,
			 cxt->tuning_bin[0].iso_awb_info.gain_g,
			 cxt->tuning_bin[0].iso_awb_info.gain_b);
	}
	if (cxt->is_refocus) {
		if (cxt->tuning_bin_slv[0].isp_dev_bin_info.puc_shading_bin_addr
		    && cxt->tuning_bin_slv[0].isp_dev_bin_info.uw_shading_bin_size >= (bin_otp_offset + sizeof(struct sensor_otp_iso_awb_info))) {
			/*for bin otp data: shading addr offset +114*/
			otp_ptr = (struct sensor_otp_iso_awb_info *)(cxt->tuning_bin_slv[0].isp_dev_bin_info.puc_shading_bin_addr + bin_otp_offset);
			cxt->tuning_bin_slv[0].iso_awb_info.iso = otp_ptr->iso;
			cxt->tuning_bin_slv[0].iso_awb_info.gain_r = otp_ptr->gain_r;
			cxt->tuning_bin_slv[0].iso_awb_info.gain_g = otp_ptr->gain_g;
			cxt->tuning_bin_slv[0].iso_awb_info.gain_b = otp_ptr->gain_b;
			ISP_LOGI("bin slv otp data iso=%d, r=%d,g=%d,b=%d",
				 cxt->tuning_bin_slv[0].iso_awb_info.iso,
				 cxt->tuning_bin_slv[0].iso_awb_info.gain_r,
				 cxt->tuning_bin_slv[0].iso_awb_info.gain_g,
				 cxt->tuning_bin_slv[0].iso_awb_info.gain_b);
		}
	}

	return ISP_SUCCESS;
}

/*************************************EXTERNAL FUNCTION ***************************************/
cmr_int isp_init(struct isp_init_param *input_ptr, cmr_handle *isp_handle)
{
	ATRACE_BEGIN(__func__);

	cmr_int                                     ret = ISP_SUCCESS;
	cmr_int                                     i = 0;
	struct isp_3a_fw_init_in                    isp3a_input;
	struct isp_dev_init_in                      isp_dev_input;
	struct isp_mw_context                       *cxt = NULL;

	isp_init_log_level();

	if (!input_ptr || !isp_handle) {
		ISP_LOGE("init param is null,input_ptr is 0x%lx,isp_handle is 0x%lx", (cmr_uint)input_ptr, (cmr_uint)isp_handle);
		ret = ISP_PARAM_NULL;
		goto exit;
	}

	*isp_handle = NULL;
	cxt = (struct isp_mw_context *)malloc(sizeof(struct isp_mw_context));
	if (NULL == cxt) {
		ISP_LOGE("failed to malloc");
		ret = ISP_ALLOC_ERROR;
		goto exit;
	}
	cmr_bzero(cxt, sizeof(*cxt));

	ATRACE_BEGIN("isp_get_and_parse_tuning_bin");
	ISP_LOGI("init param name %s", input_ptr->ex_info.name);
	cxt->input_param.ex_info.name = input_ptr->ex_info.name;
	ret = ispmw_get_tuning_onebin((cmr_handle)cxt, (const cmr_s8 *)input_ptr->ex_info.name);
	if (ret) {
		goto exit;
	}
	if (input_ptr->ex_info.af_supported) {
		if (SENSOR_PDAF_TYPE3_ENABLE == input_ptr->ex_info.pdaf_supported) {
			ret = ispmw_get_pdaf_cbc_bin((cmr_handle)cxt,
						     (const cmr_s8 *)input_ptr->ex_info.name);
			if (ret) {
				ISP_LOGE("failed to get pdaf cbc bin");
				goto exit;
			}
		}
	}
	ret = ispmw_parse_tuning_onebin((cmr_handle)cxt);
	if (ret) {
		goto exit;
	}

	if (input_ptr->is_refocus && (0 == input_ptr->camera_id || 1 == input_ptr->camera_id) && input_ptr->ex_info_slv.name) {
		ret = ispmw_get_tuning_onebin_slv((cmr_handle)cxt, (const cmr_s8 *)input_ptr->ex_info_slv.name);
		if (ret) {
			goto exit;
		}
		ret = ispmw_parse_tuning_onebin_slv((cmr_handle)cxt);
		if (ret) {
			goto exit;
		}
	}

	ATRACE_END();

	cxt->camera_id = input_ptr->camera_id;
	cxt->is_refocus = input_ptr->is_refocus;
	cxt->caller_handle = input_ptr->oem_handle;
	cxt->caller_callback = input_ptr->ctrl_callback;
	cxt->alloc_cb = input_ptr->alloc_cb;
	cxt->free_cb = input_ptr->free_cb;

	cmr_bzero(&isp_dev_input, sizeof(isp_dev_input));
	isp_dev_input.camera_id = input_ptr->camera_id;
	isp_dev_input.caller_handle = (cmr_handle)cxt;
	isp_dev_input.mem_cb_handle = input_ptr->oem_handle;
	isp_dev_input.shading_bin_addr = cxt->tuning_bin[0].isp_dev_bin_info.puc_shading_bin_addr;
	isp_dev_input.shading_bin_size = cxt->tuning_bin[0].isp_dev_bin_info.uw_shading_bin_size;
	isp_dev_input.irp_bin_addr = cxt->tuning_bin[0].isp_dev_bin_info.puc_irp_bin_addr;
	isp_dev_input.irp_bin_size = cxt->tuning_bin[0].isp_dev_bin_info.uw_irp_bin_size;
	isp_dev_input.pdaf_cbcp_bin_addr = cxt->pdaf_info.pdaf_cbc_addr;
	isp_dev_input.pdaf_cbc_bin_size = cxt->pdaf_info.pdaf_cbc_size;
	isp_dev_input.pdaf_supported = input_ptr->ex_info.pdaf_supported;
	memcpy(&isp_dev_input.init_param, input_ptr, sizeof(struct isp_init_param));
	ISP_LOGI("cbc bin addr is %p size 0x%x", (cmr_u32 *)isp_dev_input.pdaf_cbcp_bin_addr,
		isp_dev_input.pdaf_cbc_bin_size);
	ret = isp_dev_access_init(&isp_dev_input, &cxt->isp_dev_handle);
	if (ret) {
		goto exit;
	}

	isp_parse_bin_otp((cmr_handle)cxt);

	cmr_bzero(&isp3a_input, sizeof(isp3a_input));
	isp3a_input.setting_param_ptr = input_ptr->setting_param_ptr;
	isp3a_input.isp_mw_callback = input_ptr->ctrl_callback;
	isp3a_input.caller_handle = input_ptr->oem_handle;
	isp3a_input.dev_access_handle = cxt->isp_dev_handle;
	isp3a_input.camera_id = input_ptr->camera_id;
	cxt->idx = 0;
	isp3a_input.idx_num = cxt->idx;
	for (i = 0; i < ISP_INDEX_MAX; i++) {
		isp3a_input.bin_info[i].ae_addr = cxt->tuning_bin[i].ae_tuning_addr;
		isp3a_input.bin_info[i].awb_addr = cxt->tuning_bin[i].awb_tuning_addr;
		isp3a_input.bin_info[i].af_addr = cxt->tuning_bin[i].af_tuning_addr;
		isp3a_input.bin_info[i].scene_addr = cxt->tuning_bin[i].scene_bin_addr;
		isp3a_input.bin_info[i].isp_pdaf_addr = cxt->tuning_bin[i].isp_pdaf_addr;
		ISP_LOGV("i=%ld ae_addr %p awb=%p  af_addr=%p pdaf_addr=%p scene=%p",
			i,
			isp3a_input.bin_info[i].ae_addr,
			isp3a_input.bin_info[i].awb_addr,
			isp3a_input.bin_info[i].af_addr,
			isp3a_input.bin_info[i].isp_pdaf_addr,
			isp3a_input.bin_info[i].scene_addr
			);
	}
	isp3a_input.ops = input_ptr->ops;
	isp3a_input.bin_info[0].isp_shading_addr = cxt->tuning_bin[0].isp_shading_addr;
	isp3a_input.bin_info[0].isp_shading_size = cxt->tuning_bin[0].isp_shading_size;
	isp3a_input.bin_info[cxt->idx].isp_caf_addr = cxt->tuning_bin[cxt->idx].isp_caf_addr;
	isp3a_input.bin_info[cxt->idx].isp_caf_size = cxt->tuning_bin[cxt->idx].isp_caf_size;
	isp3a_input.ex_info = input_ptr->ex_info;
	isp3a_input.otp_data = input_ptr->otp_data;
	isp3a_input.dual_otp= input_ptr->dual_otp;
	isp3a_input.pdaf_info = input_ptr->pdaf_info;
	isp3a_input.bin_info[0].otp_data_addr = &cxt->tuning_bin[0].iso_awb_info;
	if (input_ptr->is_refocus) {
		isp3a_input.is_refocus = input_ptr->is_refocus;
		isp3a_input.otp_data_slv = input_ptr->otp_data_slv;
		isp3a_input.bin_info_slv[0].otp_data_addr = &cxt->tuning_bin_slv[0].iso_awb_info;
		for (i = 0; i < ISP_INDEX_MAX; i++) {
			//isp3a_input.bin_info_slv[i].ae_addr = cxt->tuning_bin_slv[i].ae_tuning_addr;
			isp3a_input.bin_info_slv[i].awb_addr = cxt->tuning_bin_slv[i].awb_tuning_addr;
			//isp3a_input.bin_info_slv[i].af_addr = cxt->tuning_bin_slv[i].af_tuning_addr;
		}
		isp3a_input.setting_param_ptr_slv = input_ptr->setting_param_ptr_slv;
		isp3a_input.ex_info_slv = input_ptr->ex_info_slv;
	}

	ret = isp_3a_fw_init(&isp3a_input, &cxt->isp_3a_handle);
exit:
	if (ret) {
		if (cxt) {
			ispmw_put_tuning_bin(cxt);
			ispmw_put_pdaf_cbc_bin(cxt);
			ret = isp_3a_fw_deinit(cxt->isp_3a_handle);
			if (ret)
				ISP_LOGE("failed to deinit 3a fw %ld", ret);
			ret = isp_dev_access_deinit(cxt->isp_dev_handle);
			if (ret)
				ISP_LOGE("failed to deinit access %ld", ret);
			free((void *)cxt);
		}
	} else {
		cxt->is_inited = 1;
		*isp_handle = (cmr_handle)cxt;
	}
	ISP_LOGI("done %ld", ret);
	ATRACE_END();
	return ret;
}

cmr_int isp_deinit(cmr_handle isp_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp_mw_context                       *cxt = (struct isp_mw_context *)isp_handle;

	ISP_CHECK_HANDLE_VALID(isp_handle);

	ret = isp_3a_fw_deinit(cxt->isp_3a_handle);
	if (ret)
		ISP_LOGE("failed to deinit 3a fw %ld", ret);
	ret = isp_dev_access_deinit(cxt->isp_dev_handle);
	if (ret)
		ISP_LOGE("failed to deinit access %ld", ret);
	ispmw_put_tuning_bin(cxt);
	ispmw_put_pdaf_cbc_bin(cxt);
	free((void *)cxt);

	return ret;
}

cmr_int isp_capability(cmr_handle isp_handle, enum isp_capbility_cmd cmd, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp_mw_context                       *cxt = (struct isp_mw_context *)isp_handle;

	ISP_CHECK_HANDLE_VALID(isp_handle);

	if (!param_ptr) {
		ISP_LOGE("input is NULL");
		ret = ISP_PARAM_NULL;
		goto exit;
	}
	ISP_LOGI("CMD 0x%x", cmd);
	switch (cmd) {
	case ISP_VIDEO_SIZE:
	case ISP_CAPTURE_SIZE:
	case ISP_REG_VAL:
		ret = isp_dev_access_capability(cxt->isp_dev_handle, cmd, param_ptr);
		break;
	case ISP_LOW_LUX_EB:
	case ISP_CUR_ISO:
	case ISP_CTRL_GET_AE_LUM:
		ret = isp_3a_fw_capability(cxt->isp_3a_handle, cmd, param_ptr);
		break;
	case ISP_DENOISE_LEVEL:
	case ISP_DENOISE_INFO:
	default:
		break;
	}
exit:
	return ret;
}

cmr_int isp_ioctl(cmr_handle isp_handle, enum isp_ctrl_cmd cmd, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp_mw_context                       *cxt = (struct isp_mw_context *)isp_handle;

	ISP_CHECK_HANDLE_VALID(isp_handle);

	ret = isp_3a_fw_ioctl(cxt->isp_3a_handle, cmd, param_ptr);

	return ret;
}

cmr_int isp_set_tuning_mode(cmr_handle isp_handle, struct isp_video_start *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp_mw_context                       *cxt = (struct isp_mw_context *)isp_handle;
	cmr_u32 tuning_mode;

	ISP_CHECK_HANDLE_VALID(isp_handle);
	ISP_LOGI("hal tuning mode %d!", param_ptr->tuning_mode);
	switch (param_ptr->tuning_mode) {
	case ISP_TUNING_PREVIEW_FULL:
	case ISP_TUNING_VIDEO_FULL:
	case ISP_TUNING_CAPTURE_FULL:
	case ISP_TUNING_HDR_FULL:
		 tuning_mode = ISP_INDEX_PREVIEW_FULL;
		break;
	case ISP_TUNING_PREVIEW_BINNING:
	case ISP_TUNING_VIDEO_BINNING:
	case ISP_TUNING_CAPTURE_BINNING:
	case ISP_TUNING_HDR_BINNING:
		tuning_mode = ISP_INDEX_PREVIEW_BINING;
		break;
	case ISP_TUNING_MULTILAYER_FULL:
		 tuning_mode= ISP_INDEX_STILL_FULL; /*full size of multilayer */
		break;
	case ISP_TUNING_MULTILAYER_BINNING:
		 tuning_mode = ISP_INDEX_STILL_BINING;
		break;
	case ISP_TUNING_SLOWVIDEO:
		 tuning_mode = ISP_INDEX_VIDEO_SLOW_MOTION;
		break;
	default:
		tuning_mode = ISP_INDEX_PREVIEW_FULL;
		break;
	}
	param_ptr->tuning_mode = tuning_mode;
	cxt->idx = param_ptr->tuning_mode;
	ISP_LOGV("converted tuning mode %d ", param_ptr->tuning_mode);
	return ret;
}

cmr_int isp_video_start(cmr_handle isp_handle, struct isp_video_start *param_ptr)
{
	ATRACE_BEGIN(__func__);

	cmr_int                                     ret = ISP_SUCCESS;
	struct isp_mw_context                       *cxt = (struct isp_mw_context *)isp_handle;
	struct isp_3a_cfg_param                     cfg_param;
	struct isp_3a_get_dld_in                    dld_in;
	struct isp_3a_dld_sequence                  dld_seq;
	struct isp_dev_access_start_in              dev_in;
	char                                        file_name[128];

	ISP_CHECK_HANDLE_VALID(isp_handle);

	if (NULL == param_ptr) {
		ISP_LOGE("error,input is NULL");
		goto exit;
	}
	ISP_LOGI("sensor %s size w, h %d, %d", cxt->input_param.ex_info.name,
		 param_ptr->size.w, param_ptr->size.h);
	isp_set_tuning_mode(cxt, param_ptr);
	ret = isp_3a_fw_start(cxt->isp_3a_handle, param_ptr);
	if (ret) {
		ISP_LOGE("failed to start 3a");
		goto exit;
	}

	ret = isp_3a_fw_get_cfg(cxt->isp_3a_handle, &dev_in.hw_cfg);
	if (ret) {
		ISP_LOGE("failed to get cfg");
		goto stop_exit;
	}
	ret = isp_3a_fw_get_iso_speed(cxt->isp_3a_handle, &dev_in.hw_iso_speed);
	if (ret) {
		ISP_LOGE("failed to get hw_iso_speed");
		goto stop_exit;
	}
	dld_in.op_mode = ISP3A_OPMODE_NORMALLV;
	ret = isp_3a_fw_get_dldseq(cxt->isp_3a_handle, &dld_in, &dev_in.dld_seq);
	if (ret) {
		ISP_LOGE("failed to get dldseq");
		goto stop_exit;
	}

	dev_in.common_in = *param_ptr;
	ret = isp_dev_access_start_multiframe(cxt->isp_dev_handle, &dev_in);
	isp_3a_fw_set_tuning_mode(cxt->isp_3a_handle, param_ptr);
	if (ISP_SUCCESS == ret) {
		goto exit;
	}
stop_exit:
	isp_3a_fw_stop(cxt->isp_3a_handle);
exit:
	ISP_LOGI("done %ld", ret);
	ATRACE_END();
	return ret;
}

cmr_int isp_video_stop(cmr_handle isp_handle)
{
	ATRACE_BEGIN(__func__);

	cmr_int                                     ret = ISP_SUCCESS;
	struct isp_mw_context                       *cxt = (struct isp_mw_context *)isp_handle;

	ISP_CHECK_HANDLE_VALID(isp_handle);

	ret = isp_dev_access_stop_multiframe(cxt->isp_dev_handle);
	ret = isp_3a_fw_stop(cxt->isp_3a_handle);

	ATRACE_END();
	return ret;
}

cmr_int isp_proc_start(cmr_handle isp_handle, struct ips_in_param *input_ptr, struct ips_out_param *output_ptr)
{
	ATRACE_BEGIN(__func__);

	cmr_int                                     ret = ISP_SUCCESS;
	struct isp_mw_context                       *cxt = (struct isp_mw_context *)isp_handle;
	struct isp_dev_postproc_in                  dev_in;
	struct isp_dev_postproc_out                 dev_out;
	struct isp_3a_get_dld_in                    dld_in;
	struct isp_video_start                      proc_param;

	ISP_LOGE("isp_proc_start");
	UNUSED(output_ptr);

	proc_param.tuning_mode = ISP_TUNING_MULTILAYER_FULL;
	isp_set_tuning_mode(cxt, &proc_param);
	ret = isp_3a_fw_get_cfg(cxt->isp_3a_handle, &dev_in.hw_cfg);
	if (ret) {
		ISP_LOGE("failed to get cfg");
		goto exit;
	}
	ret = isp_3a_fw_get_iso_speed(cxt->isp_3a_handle, &dev_in.hw_iso_speed);
	if (ret) {
		ISP_LOGE("failed to get hw_iso_speed");
		goto exit;
	}
	dld_in.op_mode = ISP3A_OPMODE_NORMALLV;
	ret = isp_3a_fw_get_dldseq(cxt->isp_3a_handle, &dld_in, &dev_in.dldseq);
	if (ret) {
		ISP_LOGE("failed to get dldseq");
		goto exit;
	}
	ret = isp_3a_fw_get_awb_gain(cxt->isp_3a_handle, &dev_in.awb_gain, &dev_in.awb_gain_balanced);
	if (ret) {
		ISP_LOGE("failed to get awb gain");
		goto  exit;
	}

	dev_in.cap_mode = input_ptr->cap_mode;
	dev_in.input_ptr = input_ptr;
	memcpy(&dev_in.src_frame, &input_ptr->src_frame, sizeof(struct isp_img_frm));
	memcpy(&dev_in.dst_frame, &input_ptr->dst_frame, sizeof(struct isp_img_frm));
	memcpy(&dev_in.dst2_frame, &input_ptr->dst2_frame, sizeof(struct isp_img_frm));
	memcpy(&dev_in.resolution_info, &input_ptr->resolution_info, sizeof(struct isp_sensor_resolution_info));
	ret=  isp_3a_fw_set_tuning_mode(cxt->isp_3a_handle, &proc_param);
	ret = isp_dev_access_start_postproc(cxt->isp_dev_handle, &dev_in, &dev_out);

	ret = isp_3a_fw_stop(cxt->isp_3a_handle);
exit:
	ATRACE_END();
	return ret;
}

cmr_int isp_proc_next(cmr_handle isp_handle, struct ipn_in_param *input_ptr, struct ips_out_param *output_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp_mw_context                       *cxt = (struct isp_mw_context *)isp_handle;

	UNUSED(input_ptr);
	UNUSED(output_ptr);
	return ret;
}

cmr_int isp_cap_buff_cfg(cmr_handle isp_handle, struct isp_img_param *buf_cfg)
{
	ATRACE_BEGIN(__func__);

	cmr_int                    ret = ISP_SUCCESS;
	struct isp_mw_context      *cxt = (struct isp_mw_context *)isp_handle;
	cmr_u32                  i;
	struct isp_dev_img_param parm;

	if (NULL == buf_cfg || NULL == cxt->isp_dev_handle) {
		ISP_LOGE("Para invalid");
		return -1;
	}

	memset(&parm, 0, sizeof(struct isp_dev_img_param));
	parm.img_fmt = buf_cfg->img_fmt;
	parm.channel_id = buf_cfg->channel_id;
	parm.base_id = buf_cfg->base_id;
	parm.is_reserved_buf  = buf_cfg->is_reserved_buf;
	parm.count	 = buf_cfg->count;
	parm.flag         = buf_cfg->flag;
	for (i = 0; i < parm.count; i++) {
		parm.addr[i].chn0 = buf_cfg->addr[i].chn0;
		parm.addr[i].chn1 = buf_cfg->addr[i].chn1;
		parm.addr[i].chn2 = buf_cfg->addr[i].chn2;
		parm.addr_vir[i].chn0 = buf_cfg->addr_vir[i].chn0;
		parm.addr_vir[i].chn1 = buf_cfg->addr_vir[i].chn1;
		parm.addr_vir[i].chn2 = buf_cfg->addr_vir[i].chn2;
		parm.index[i]           = buf_cfg->index[i];
		parm.img_fd[i].y      = buf_cfg->img_fd[i].y;
		parm.img_fd[i].u      = buf_cfg->img_fd[i].u;
		parm.img_fd[i].v      = buf_cfg->img_fd[i].v;
	}
	parm.zsl_private      = buf_cfg->zsl_private;

	ret = isp_dev_access_cap_buf_cfg(cxt->isp_dev_handle, &parm);

	ATRACE_END();
	return ret;
}

cmr_int isp_drammode_takepic(cmr_handle isp_handle, cmr_u32 is_start)
{
	ATRACE_BEGIN(__func__);

	cmr_int                    ret = ISP_SUCCESS;
	struct isp_mw_context      *cxt = (struct isp_mw_context *)isp_handle;

	ret = isp_dev_access_drammode_takepic(cxt->isp_dev_handle, is_start);

	ATRACE_END();
	return ret;
}
