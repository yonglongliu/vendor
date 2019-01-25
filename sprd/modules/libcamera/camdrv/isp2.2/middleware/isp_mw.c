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

#include "isp_mw.h"
#include "cmr_msg.h"

/* new part */
#include "isp_dev_access.h"
#include "isp_alg_fw.h"

struct isp_mw_context {
	cmr_handle alg_fw_handle;
	cmr_handle dev_access_handle;
};

void ispmw_dev_buf_cfg_evt_cb(cmr_handle isp_handle, isp_buf_cfg_evt_cb grab_event_cb)
{
	UNUSED(isp_handle);
	UNUSED(grab_event_cb);
}

void isp_statis_evt_cb(cmr_int evt, void *data, void *privdata)
{
	struct isp_mw_context *mw_cxt = (struct isp_mw_context *)privdata;
	UNUSED(evt);

	isp_dev_statis_info_proc(mw_cxt->dev_access_handle, data);

}

void isp_irq_proc_evt_cb(cmr_int evt, void *data, void *privdata)
{
	struct isp_mw_context *mw_cxt = (struct isp_mw_context *)privdata;
	UNUSED(evt);

	ISP_LOGV("SOF:isp_irq_proc_evt_cb");
	isp_dev_irq_info_proc(mw_cxt->dev_access_handle, data);
}

static cmr_s32 _isp_check_video_param(struct isp_video_start *param_ptr)
{
	cmr_s32 rtn = ISP_SUCCESS;

	if ((ISP_ZERO != (param_ptr->size.w & ISP_ONE)) || (ISP_ZERO != (param_ptr->size.h & ISP_ONE))) {
		rtn = ISP_PARAM_ERROR;
		ISP_RETURN_IF_FAIL(rtn, ("fail to check video param input size: w:%d, h:%d error", param_ptr->size.w, param_ptr->size.h));
	}

	return rtn;
}

static cmr_s32 _isp_check_proc_start_param(struct ips_in_param *in_param_ptr)
{
	cmr_s32 rtn = ISP_SUCCESS;

	if ((ISP_ZERO != (in_param_ptr->src_frame.img_size.w & ISP_ONE)) || (ISP_ZERO != (in_param_ptr->src_frame.img_size.h & ISP_ONE))) {
		rtn = ISP_PARAM_ERROR;
		ISP_RETURN_IF_FAIL(rtn, ("ifail to check start param input size: w:%d, h:%d error", in_param_ptr->src_frame.img_size.w, in_param_ptr->src_frame.img_size.h));
	}

	return rtn;
}

static cmr_s32 _isp_check_proc_next_param(struct ipn_in_param *in_param_ptr)
{
	cmr_s32 rtn = ISP_SUCCESS;

	if ((ISP_ZERO != (in_param_ptr->src_slice_height & ISP_ONE)) || (ISP_ZERO != (in_param_ptr->src_avail_height & ISP_ONE))) {
		rtn = ISP_PARAM_ERROR;
		ISP_RETURN_IF_FAIL(rtn, ("fail to check param,input size:src_slice_h:%d,src_avail_h:%d error", in_param_ptr->src_slice_height, in_param_ptr->src_avail_height));
	}

	return rtn;
}

/* Public Function Prototypes */
cmr_int isp_init(struct isp_init_param * input_ptr, cmr_handle * isp_handler)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_mw_context *cxt = NULL;
	struct isp_alg_fw_init_in ispalg_input;

	isp_init_log_level();
	ISP_LOGV("E");
	cxt = (struct isp_mw_context *)malloc(sizeof(struct isp_mw_context));
	memset((void *)cxt, 0x00, sizeof(*cxt));
	if (!input_ptr || !isp_handler) {
		ISP_LOGE("fail to check init param,input_ptr is 0x%lx & handler is 0x%lx", (cmr_uint) input_ptr, (cmr_uint) isp_handler);
		rtn = ISP_PARAM_NULL;
		goto exit;
	}

	*isp_handler = NULL;

	rtn = isp_dev_access_init(input_ptr->dcam_fd, &cxt->dev_access_handle);
	if (rtn) {
		goto exit;
	}

	ispalg_input.dev_access_handle = cxt->dev_access_handle;
	ispalg_input.init_param = input_ptr;
	rtn = isp_alg_fw_init(&ispalg_input, &cxt->alg_fw_handle);

exit:
	if (rtn) {
		if (cxt) {
			rtn = isp_alg_fw_deinit(cxt->alg_fw_handle);
			rtn = isp_dev_access_deinit(cxt->dev_access_handle);
			free((void *)cxt);
			cxt = NULL;
		}
	} else {
		*isp_handler = (isp_handle) cxt;
	}

	ISP_LOGI("done %ld", rtn);

	return rtn;
}

cmr_int isp_deinit(cmr_handle isp_handler)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_mw_context *cxt = (struct isp_mw_context *)isp_handler;

	ISP_CHECK_HANDLE_VALID(isp_handler);

	rtn = isp_alg_fw_deinit(cxt->alg_fw_handle);
	if (ISP_SUCCESS == rtn) {
		rtn = isp_dev_access_deinit(cxt->dev_access_handle);
	}

	if (NULL != cxt) {
		free(cxt);
		cxt = NULL;
	}

	ISP_LOGI("done %ld", rtn);

	return rtn;
}

cmr_int isp_capability(cmr_handle isp_handler, enum isp_capbility_cmd cmd, void *param_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_mw_context *cxt = (struct isp_mw_context *)isp_handler;

	switch (cmd) {
	case ISP_VIDEO_SIZE:
	case ISP_CAPTURE_SIZE:
	case ISP_REG_VAL:
		rtn = isp_dev_access_capability(cxt->dev_access_handle, cmd, param_ptr);
		break;
	case ISP_LOW_LUX_EB:
	case ISP_CUR_ISO:
	case ISP_CTRL_GET_AE_LUM:
		rtn = isp_alg_fw_capability(cxt->alg_fw_handle, cmd, param_ptr);
		break;
	default:
		break;
	}

	return rtn;
}

cmr_int isp_ioctl(cmr_handle isp_handler, enum isp_ctrl_cmd cmd, void *param_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_mw_context *cxt = (struct isp_mw_context *)isp_handler;

	if (NULL == cxt) {
		ISP_LOGE("fail to check isp handler");
		return ISP_PARAM_NULL;
	}

	rtn = isp_alg_fw_ioctl(cxt->alg_fw_handle, cmd, param_ptr, NULL);

	ISP_TRACE_IF_FAIL(rtn, ("fail to do isp_ioctl"));

	return rtn;
}

cmr_int isp_video_start(cmr_handle isp_handler, struct isp_video_start * param_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_mw_context *cxt = (struct isp_mw_context *)isp_handler;
	if (!isp_handler || !param_ptr) {
		rtn = ISP_PARAM_ERROR;
		goto exit;
	}

	rtn = isp_alg_fw_start(cxt->alg_fw_handle, param_ptr);

exit:
	ISP_LOGV("done %ld", rtn);
	return rtn;
}

cmr_int isp_video_stop(cmr_handle isp_handler)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_mw_context *cxt = (struct isp_mw_context *)isp_handler;

	if (!isp_handler) {
		rtn = ISP_PARAM_NULL;
		goto exit;
	}

	rtn = isp_alg_fw_stop(cxt->alg_fw_handle);

exit:
	return rtn;
}

cmr_int isp_proc_start(cmr_handle isp_handler, struct ips_in_param * in_ptr, struct ips_out_param * out_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_mw_context *cxt = (struct isp_mw_context *)isp_handler;
	UNUSED(out_ptr);

	if (NULL == cxt) {
		ISP_LOGE("fail to check isp handler");
		return ISP_PARAM_NULL;
	}

	rtn = _isp_check_proc_start_param(in_ptr);
	ISP_RETURN_IF_FAIL(rtn, ("fail to check init param"));

	rtn = isp_alg_proc_start(cxt->alg_fw_handle, in_ptr);

	return rtn;
}

cmr_int isp_proc_next(cmr_handle isp_handler, struct ipn_in_param * in_ptr, struct ips_out_param * out_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_mw_context *cxt = (struct isp_mw_context *)isp_handler;
	UNUSED(out_ptr);

	if (NULL == cxt) {
		ISP_LOGE("fail to check isp handler");
		return ISP_PARAM_NULL;
	}

	rtn = _isp_check_proc_next_param(in_ptr);
	ISP_RETURN_IF_FAIL(rtn, ("fail to check init param"));

	rtn = isp_alg_proc_next(cxt->alg_fw_handle, in_ptr);

	return rtn;
}

cmr_int isp_cap_buff_cfg(cmr_handle isp_handle, struct isp_img_param * buf_cfg)
{
	cmr_int ret = ISP_SUCCESS;
	UNUSED(isp_handle);
	UNUSED(buf_cfg);

	return ret;
}
