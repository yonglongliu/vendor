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
#define LOG_TAG "isp_dev_access"

#include "isp_dev_access.h"
#include "ae_ctrl_types.h"
#include "af_ctrl.h"
#include "isp_alg_fw.h"
//#define ISP_DEFAULT_CFG_FOR_BRING_UP

cmr_int isp_dev_statis_buf_malloc(cmr_handle isp_dev_handle, struct isp_statis_mem_info * in_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;
	struct isp_statis_mem_info *statis_mem_info = &cxt->statis_mem_info;
	cmr_uint type = 0;
	cmr_s32 fds[2];
	cmr_uint kaddr[2];

	statis_mem_info->isp_lsc_mem_size = in_ptr->isp_lsc_mem_size;
	statis_mem_info->isp_lsc_mem_num = in_ptr->isp_lsc_mem_num;
	statis_mem_info->isp_lsc_physaddr = in_ptr->isp_lsc_physaddr;
	statis_mem_info->isp_lsc_virtaddr = in_ptr->isp_lsc_virtaddr;
	statis_mem_info->lsc_mfd = in_ptr->lsc_mfd;

	if (statis_mem_info->isp_statis_alloc_flag == 0) {
		statis_mem_info->buffer_client_data = in_ptr->buffer_client_data;
		statis_mem_info->cb_of_malloc = in_ptr->cb_of_malloc;
		statis_mem_info->cb_of_free = in_ptr->cb_of_free;
		statis_mem_info->isp_statis_mem_size = (ISP_AEM_STATIS_BUF_SIZE
							+ ISP_AFM_STATIS_BUF_SIZE + ISP_AFL_STATIS_BUF_SIZE + ISP_PDAF_STATIS_BUF_SIZE + ISP_BINNING_STATIS_BUF_SIZE) * 5;
		statis_mem_info->isp_statis_mem_num = 1;
		if (statis_mem_info->cb_of_malloc) {
			isp_cb_of_malloc cb_malloc = in_ptr->cb_of_malloc;
			if (-EINVAL == cb_malloc(CAMERA_ISP_STATIS,
				  &statis_mem_info->isp_statis_mem_size,
				  &statis_mem_info->isp_statis_mem_num,
				  kaddr,
				  &statis_mem_info->isp_statis_u_addr,
				  //&statis_mem_info->statis_mfd,
				  fds,
				  statis_mem_info->buffer_client_data)) {
					ISP_LOGE("fail to malloc statis_bq buffer");
					return ISP_ALLOC_ERROR;

				  }
		} else {
			ISP_LOGE("fail to malloc statis_bq buffer");
			return ISP_PARAM_NULL;
		}
		statis_mem_info->isp_statis_k_addr[0] = kaddr[0];
		statis_mem_info->isp_statis_k_addr[1] = kaddr[1];
		statis_mem_info->statis_mfd = fds[0];
		statis_mem_info->statis_buf_dev_fd = fds[1];
		statis_mem_info->isp_statis_alloc_flag = 1;
	}

	return rtn;
}

cmr_int isp_dev_trans_addr(cmr_handle isp_dev_handle)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;
	struct isp_statis_mem_info *statis_mem_info = &cxt->statis_mem_info;
	struct isp_file *file = NULL;
	struct isp_statis_buf_input isp_statis_buf;
	struct isp_statis_buf_input isp_2d_lsc_buf;

	memset(&isp_statis_buf, 0x00, sizeof(isp_statis_buf));
	memset(&isp_2d_lsc_buf, 0x00, sizeof(isp_2d_lsc_buf));
	file = (struct isp_file *)(cxt->isp_driver_handle);
	file->reserved = (void *)statis_mem_info->isp_lsc_virtaddr;
	/*set buffer to kernel */
	isp_2d_lsc_buf.buf_size = statis_mem_info->isp_lsc_mem_size;
	isp_2d_lsc_buf.buf_num = statis_mem_info->isp_lsc_mem_num;
	isp_2d_lsc_buf.phy_addr = statis_mem_info->isp_lsc_physaddr;
	isp_2d_lsc_buf.vir_addr = statis_mem_info->isp_lsc_virtaddr;
	isp_2d_lsc_buf.mfd = statis_mem_info->lsc_mfd;

	isp_statis_buf.buf_size = statis_mem_info->isp_statis_mem_size;
	isp_statis_buf.buf_num = statis_mem_info->isp_statis_mem_num;
	isp_statis_buf.kaddr[0] = statis_mem_info->isp_statis_k_addr[0];
	isp_statis_buf.kaddr[1] = statis_mem_info->isp_statis_k_addr[1];
	isp_statis_buf.vir_addr = statis_mem_info->isp_statis_u_addr;
	isp_statis_buf.buf_flag = 0;
	isp_statis_buf.mfd = statis_mem_info->statis_mfd;
	isp_statis_buf.dev_fd = statis_mem_info->statis_buf_dev_fd;
	/*isp lsc addr transfer */
	isp_u_2d_lsc_transaddr(cxt->isp_driver_handle, &isp_2d_lsc_buf);

	rtn = isp_dev_access_ioctl(isp_dev_handle, ISP_DEV_SET_STSTIS_BUF, &isp_statis_buf, NULL);

	return rtn;
}

cmr_int isp_dev_set_interface(struct isp_interface_param_v1 * in_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	if (!in_ptr) {
		rtn = ISP_PARAM_ERROR;
		goto exit;
	}

	rtn = isp_set_comm_param((cmr_handle) in_ptr);
	ISP_TRACE_IF_FAIL(rtn, ("isp_set_comm_param error"));

	rtn = isp_set_slice_size((cmr_handle) in_ptr);
	ISP_TRACE_IF_FAIL(rtn, ("isp_set_slice_size error"));

	rtn = isp_set_fetch_param((cmr_handle) in_ptr);
	ISP_TRACE_IF_FAIL(rtn, ("isp_set_fetch_param error"));

	rtn = isp_set_store_param((cmr_handle) in_ptr);
	ISP_TRACE_IF_FAIL(rtn, ("isp_set_store_param error"));

	rtn = isp_set_dispatch((cmr_handle) in_ptr);
	ISP_TRACE_IF_FAIL(rtn, ("isp_set_dispatch error"));

	rtn = isp_set_arbiter((cmr_handle) in_ptr);
	ISP_TRACE_IF_FAIL(rtn, ("isp_set_arbiter error"));

exit:
	return rtn;
}

cmr_int isp_dev_start(cmr_handle isp_dev_handle, struct isp_interface_param_v1 * in_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;
	struct isp_dev_cfa_info cfa_param;
	struct isp_dev_cce_info cce_param;

	isp_u_fetch_raw_transaddr(cxt->isp_driver_handle, &in_ptr->fetch.fetch_addr);

	rtn = isp_get_fetch_addr(in_ptr, &in_ptr->fetch);
	ISP_RETURN_IF_FAIL(rtn, ("isp get fetch addr error"));

#ifdef ISP_DEFAULT_CFG_FOR_BRING_UP

	rtn = isp_get_cfa_default_param(in_ptr, &cfa_param);
	ISP_RETURN_IF_FAIL(rtn, ("isp get cfa default param error"));

	isp_u_cfa_block(cxt->isp_driver_handle, (void *)&cfa_param);
	ISP_RETURN_IF_FAIL(rtn, ("isp cfg cfa error"));
#endif

	isp_u_fetch_block(cxt->isp_driver_handle, (void *)&in_ptr->fetch);
	ISP_RETURN_IF_FAIL(rtn, ("isp cfg fetch error"));

	rtn = isp_u_store_block(cxt->isp_driver_handle, (void *)&in_ptr->store);
	ISP_RETURN_IF_FAIL(rtn, ("isp cfg store error"));

	rtn = isp_u_dispatch_block(cxt->isp_driver_handle, (void *)&in_ptr->dispatch);
	ISP_RETURN_IF_FAIL(rtn, ("isp cfg dispatch error"));

	isp_u_arbiter_block(cxt->isp_driver_handle, (void *)&in_ptr->arbiter);
	ISP_RETURN_IF_FAIL(rtn, ("isp cfg arbiter error"));

	isp_cfg_comm_data(cxt->isp_driver_handle, (void *)&in_ptr->com);

	isp_cfg_slice_size(cxt->isp_driver_handle, (void *)&in_ptr->slice);

	return rtn;
}

cmr_u32 isp_dev_access_chip_id(cmr_handle isp_dev_handle)
{
	cmr_u32 rtn = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	rtn = isp_dev_get_chip_id(cxt->isp_driver_handle);

	return rtn;
}

cmr_int isp_dev_raw_afm_statistic_r6p9(cmr_handle isp_dev_handle, void *statics)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	rtn = isp_u_raw_afm_statistic_r6p9(cxt->isp_driver_handle, statics);
	return rtn;
}

cmr_int isp_dev_raw_afm_type1_statistic(cmr_handle isp_dev_handle, void *statics)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	rtn = isp_u_raw_afm_type1_statistic(cxt->isp_driver_handle, statics);
	return rtn;
}

cmr_int isp_dev_anti_flicker_bypass(cmr_handle isp_dev_handle, cmr_int bypass)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	rtn = isp_u_anti_flicker_bypass(cxt->isp_driver_handle, (void *)&bypass);

	return rtn;
}

cmr_int isp_dev_anti_flicker_new_bypass(cmr_handle isp_dev_handle, cmr_int bypass)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	rtn = isp_u_anti_flicker_new_bypass(cxt->isp_driver_handle, (void *)&bypass);

	return rtn;
}

cmr_int isp_dev_cfg_block(cmr_handle isp_dev_handle, void *data_ptr, cmr_int data_id)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	rtn = isp_cfg_block(cxt->isp_driver_handle, data_ptr, data_id);

	return rtn;
}

cmr_int isp_dev_lsc_update(cmr_handle isp_dev_handle, cmr_int flag)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	ISP_LOGV("driver handle is %p", cxt->isp_driver_handle);

	rtn = isp_u_2d_lsc_param_update(cxt->isp_driver_handle, flag);

	return rtn;
}

cmr_int isp_dev_awb_gain(cmr_handle isp_dev_handle, cmr_u32 r, cmr_u32 g, cmr_u32 b)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	ret = isp_u_awbc_gain(cxt->isp_driver_handle, r, g, b);

	return ret;
}

cmr_int isp_dev_comm_shadow(cmr_handle isp_dev_handle, cmr_int shadow)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	rtn = isp_u_comm_shadow_ctrl(cxt->isp_driver_handle, shadow);

	return rtn;
}

void isp_dev_access_evt_reg(cmr_handle isp_dev_handle, isp_evt_cb isp_event_cb, void *privdata)
{
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	cxt->isp_event_cb = isp_event_cb;
	cxt->evt_alg_handle = privdata;
}

void isp_dev_statis_info_proc(cmr_handle isp_dev_handle, void *param_ptr)
{
	struct sprd_img_statis_info *irq_info;
	struct isp_statis_info *statis_info = NULL;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;
	struct isp_alg_fw_context *isp_alg_fw_cxt = (struct isp_alg_fw_context *)(cxt->evt_alg_handle);
	irq_info = (struct sprd_img_statis_info *)param_ptr;

	statis_info = malloc(sizeof(*statis_info));

	statis_info->phy_addr = irq_info->phy_addr;
	statis_info->vir_addr = irq_info->vir_addr;
	statis_info->kaddr[0] = irq_info->kaddr[0];
	statis_info->kaddr[1] = irq_info->kaddr[1];
	statis_info->irq_property = irq_info->irq_property;
	statis_info->buf_size = irq_info->buf_size;
	statis_info->mfd = irq_info->mfd;

	if (isp_alg_fw_cxt->is_multi_mode == ISP_DUAL_NORMAL) {
		// TODO: change this after kernel header modified
		statis_info->sec = 0;//irq_info->sec;
		statis_info->usec = 0;//irq_info->usec;
		statis_info->monoboottime = 0;//irq_info->monoboottime;
	}

	ISP_LOGV("got one frame statis paddr 0x%x vaddr 0x%x property 0x%d", statis_info->phy_addr, statis_info->vir_addr, statis_info->irq_property);
	if (irq_info->irq_property == IRQ_AEM_STATIS) {
		if (cxt->isp_event_cb) {
			(*cxt->isp_event_cb) (ISP_CTRL_EVT_AE, statis_info, (void *)cxt->evt_alg_handle);
		}
	} else if (irq_info->irq_property == IRQ_AFM_STATIS) {
		if (cxt->isp_event_cb) {
			(*cxt->isp_event_cb) (ISP_CTRL_EVT_AF, statis_info, (void *)cxt->evt_alg_handle);
		}
	} else if (irq_info->irq_property == IRQ_AFL_STATIS) {
		if (cxt->isp_event_cb) {
			(*cxt->isp_event_cb) (ISP_PROC_AFL_DONE, statis_info, (void *)cxt->evt_alg_handle);
		}
	} else if (irq_info->irq_property == IRQ_BINNING_STATIS) {
		if (cxt->isp_event_cb) {
			(*cxt->isp_event_cb) (ISP_CTRL_EVT_BINNING, statis_info, (void *)cxt->evt_alg_handle);
		}
	} else if (irq_info->irq_property == IRQ_PDAF_STATIS) {
		if (cxt->isp_event_cb) {
			(*cxt->isp_event_cb) (ISP_CTRL_EVT_PDAF, statis_info, (void *)cxt->evt_alg_handle);
		}
	} else {
		free((void *)statis_info);
		statis_info = NULL;
	}

}

void isp_dev_irq_info_proc(cmr_handle isp_dev_handle, void *param_ptr)
{
	struct sprd_irq_info *irq_info;
	struct isp_statis_info *statis_info = NULL;

	statis_info = malloc(sizeof(*statis_info));

	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;
	irq_info = (struct sprd_irq_info *)param_ptr;

	statis_info->irq_property = irq_info->irq_property;

	if (irq_info->irq_property == IRQ_DCAM_SOF) {
		if (cxt->isp_event_cb) {
			(cxt->isp_event_cb) (ISP_CTRL_EVT_SOF, statis_info, (void *)cxt->evt_alg_handle);
		}
	} else if (irq_info->irq_property == IRQ_RAW_CAP_DONE) {
		if (cxt->isp_event_cb) {
			(cxt->isp_event_cb) (ISP_CTRL_EVT_TX, NULL, (void *)cxt->evt_alg_handle);
		}
	} else {
		free((void *)statis_info);
		statis_info = NULL;
	}

}

cmr_int isp_dev_access_init(cmr_s32 fd, cmr_handle * isp_dev_handle)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = NULL;
	pthread_attr_t attr;

	cxt = (struct isp_dev_access_context *)malloc(sizeof(struct isp_dev_access_context));
	if (NULL == cxt) {
		ISP_LOGE("fail to malloc");
		rtn = ISP_ERROR;
		goto exit;
	}
	*isp_dev_handle = NULL;
	memset((void *)cxt, 0x00, sizeof(struct isp_dev_access_context));

	rtn = isp_dev_open(fd, &cxt->isp_driver_handle);

	if (rtn) {
		ISP_LOGE("fail to open isp dev!");
		goto exit;
	}
exit:
	if (rtn) {
		if (cxt) {
			free((void *)cxt);
			cxt = NULL;
		}
	} else {
		*isp_dev_handle = (cmr_handle) cxt;
	}

	ISP_LOGI("done %ld", rtn);

	return rtn;
}

cmr_int isp_dev_access_deinit(cmr_handle isp_handler)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_handler;
	struct isp_statis_mem_info *statis_mem_info = &cxt->statis_mem_info;
	cmr_uint type = 0;
	void *dummy;
	ISP_CHECK_HANDLE_VALID(isp_handler);

	if (statis_mem_info->isp_statis_alloc_flag == 1) {
		type = CAMERA_ISP_STATIS;

		if (statis_mem_info->cb_of_free) {
			isp_cb_of_free cb_free = statis_mem_info->cb_of_free;
			cb_free(type, (cmr_uint *)statis_mem_info->isp_statis_k_addr, &statis_mem_info->isp_statis_u_addr, &statis_mem_info->statis_mfd,
				statis_mem_info->isp_statis_mem_num, statis_mem_info->buffer_client_data);
		}

		statis_mem_info->isp_statis_alloc_flag = 0;
	}
	isp_dev_close(cxt->isp_driver_handle);
	if (cxt) {
		free((void *)cxt);
		cxt = NULL;
	}

	ISP_LOGI("done %ld", rtn);

	return rtn;
}

cmr_int isp_dev_access_capability(cmr_handle isp_dev_handle, enum isp_capbility_cmd cmd, void *param_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	if (!isp_dev_handle || !param_ptr) {
		rtn = ISP_PARAM_ERROR;
		goto exit;
	}

	switch (cmd) {
	case ISP_VIDEO_SIZE:{
			struct isp_video_limit *size_ptr = param_ptr;
			rtn = isp_u_capability_continue_size(cxt->isp_driver_handle, &size_ptr->width, &size_ptr->height);
			break;
		}
	case ISP_CAPTURE_SIZE:{
			struct isp_video_limit *size_ptr = param_ptr;
			rtn = isp_u_capability_single_size(cxt->isp_driver_handle, &size_ptr->width, &size_ptr->height);
			break;
		}
	case ISP_REG_VAL:
		rtn = isp_dev_reg_fetch(cxt->isp_driver_handle, 0, (cmr_u32 *) param_ptr, 0x1000);
		break;
	default:
		break;
	}

exit:
	return rtn;
}

static cmr_int dev_ae_set_statistics_mode(cmr_handle isp_dev_handle, cmr_int mode, cmr_u32 skip_number)
{

	cmr_int rtn = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	switch (mode) {
	case AE_STATISTICS_MODE_SINGLE:
		isp_u_3a_ctrl(cxt->isp_driver_handle, 1);
		isp_u_raw_aem_mode(cxt->isp_driver_handle, 0);
		isp_u_raw_aem_skip_num(cxt->isp_driver_handle, skip_number);
		break;
	case AE_STATISTICS_MODE_CONTINUE:
		isp_u_raw_aem_mode(cxt->isp_driver_handle, 1);
		isp_u_raw_aem_skip_num(cxt->isp_driver_handle, 0);
		break;
	default:
		break;
	}

	return 0;
}

static cmr_int dev_ae_set_rgb_gain(cmr_handle isp_dev_handle, cmr_u32 * rgb_gain_coeff)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;
	cmr_u32 rgb_gain_offset = 4096;
	struct isp_dev_rgb_gain_info gain_info;

	gain_info.bypass = 0;
	gain_info.global_gain = *rgb_gain_coeff;
	gain_info.r_gain = rgb_gain_offset;
	gain_info.g_gain = rgb_gain_offset;
	gain_info.b_gain = rgb_gain_offset;

	ISP_LOGV("d-gain--global_gain: %d\n", gain_info.global_gain);

	isp_u_rgb_gain_block(cxt->isp_driver_handle, &gain_info);

	return 0;
}

static cmr_int dev_set_af_monitor(cmr_handle isp_dev_handle, void *param0, cmr_u32 cur_envi)
{
	cmr_int rtn = ISP_SUCCESS;

	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;
	struct af_monitor_set *in_param = (struct af_monitor_set *)param0;

	UNUSED(cur_envi);
	isp_u_raw_afm_bypass(cxt->isp_driver_handle, 1);
	isp_u_raw_afm_skip_num_clr(cxt->isp_driver_handle, 1);
	isp_u_raw_afm_skip_num_clr(cxt->isp_driver_handle, 0);
	isp_u_raw_afm_skip_num(cxt->isp_driver_handle, in_param->skip_num);
	isp_u_raw_afm_bypass(cxt->isp_driver_handle, in_param->bypass);

	return rtn;
}

cmr_int isp_dev_access_ioctl(cmr_handle isp_dev_handle, cmr_int cmd, void *param0, void *param1)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	switch (cmd) {
	case ISP_DEV_SET_AE_MONITOR:
		isp_u_raw_aem_skip_num(cxt->isp_driver_handle, *(cmr_u32 *) param0);
		break;
	case ISP_DEV_SET_AE_MONITOR_WIN:{
			struct ae_monitor_info *in_param = (struct ae_monitor_info *)param0;
			rtn = isp_u_raw_aem_offset(cxt->isp_driver_handle, in_param->trim.x, in_param->trim.y);
			rtn = isp_u_raw_aem_blk_size(cxt->isp_driver_handle, in_param->win_size.w, in_param->win_size.h);
			break;
		}
	case ISP_DEV_SET_AE_MONITOR_BYPASS:
		isp_u_raw_aem_bypass(cxt->isp_driver_handle, param0);
		break;
	case ISP_DEV_SET_AE_STATISTICS_MODE:
		rtn = dev_ae_set_statistics_mode(isp_dev_handle, *(cmr_int *) param0, *(cmr_u32 *) param1);
		break;
	case ISP_DEV_SET_RGB_GAIN:
		rtn = dev_ae_set_rgb_gain(isp_dev_handle, (cmr_u32*)param0);
		break;
	case ISP_DEV_GET_AE_SYSTEM_TIME:
	case ISP_DEV_GET_AF_SYSTEM_TIME:
		rtn = isp_u_capability_time(cxt->isp_driver_handle, (cmr_u32 *) param0, (cmr_u32 *) param1);
		break;
	case ISP_DEV_RESET:
		rtn = isp_dev_reset(cxt->isp_driver_handle);
		break;
	case ISP_DEV_STOP:
		rtn = isp_dev_stop(cxt->isp_driver_handle);
		break;
	case ISP_DEV_ENABLE_IRQ:
		rtn = isp_dev_enable_irq(cxt->isp_driver_handle, *(cmr_u32 *) param0);
		break;
	case ISP_DEV_SET_AFL_BLOCK:
		rtn = isp_u_anti_flicker_block(cxt->isp_driver_handle, param0);
		break;
	case ISP_DEV_SET_AFL_NEW_BLOCK:
		rtn = isp_u_anti_flicker_new_block(cxt->isp_driver_handle, param0);
		break;
	case ISP_DEV_RAW_AEM_BYPASS:
		rtn = isp_u_raw_aem_bypass(cxt->isp_driver_handle, param0);
		break;
	case ISP_DEV_RAW_AFM_BYPASS:
		rtn = isp_u_raw_afm_bypass(cxt->isp_driver_handle, *(cmr_u32 *) param0);
		break;
	case ISP_DEV_SET_AF_MONITOR:
		rtn = dev_set_af_monitor(cxt, param0, *(cmr_u32 *) param1);
		break;
	case ISP_DEV_SET_AF_MONITOR_WIN:
		rtn = isp_u_raw_afm_win(cxt->isp_driver_handle, param0);
		break;
	case ISP_DEV_GET_AF_MONITOR_WIN_NUM:
		rtn = isp_u_raw_afm_win_num(cxt->isp_driver_handle, (cmr_u32 *) param0);
		ISP_LOGV("af_win_num %d", *(cmr_u32 *) param0);
		break;
	case ISP_DEV_SET_STSTIS_BUF:
		rtn = isp_dev_set_statis_buf(cxt->isp_driver_handle, param0);
		break;
	case ISP_DEV_SET_PDAF_CFG_PARAM:
		rtn = isp_u_pdaf_block(cxt->isp_driver_handle, param0);
		break;
	case ISP_DEV_SET_PDAF_BYPASS:
		rtn = isp_u_pdaf_bypass(cxt->isp_driver_handle, param0);
		break;
	case ISP_DEV_SET_PDAF_WORK_MODE:
		rtn = isp_u_pdaf_work_mode(cxt->isp_driver_handle, param0);
		break;
	case ISP_DEV_SET_PDAF_SKIP_NUM:
		rtn = isp_u_pdaf_skip_num(cxt->isp_driver_handle, param0);
		break;
	case ISP_DEV_SET_PDAF_EXTRACTOR_BYPASS:
		rtn = isp_u_pdaf_extractor_bypass(cxt->isp_driver_handle, param0);
		break;
	case ISP_DEV_SET_PDAF_ROI:
		rtn = isp_u_pdaf_roi(cxt->isp_driver_handle, param0);
		break;
	case ISP_DEV_SET_PDAF_PPI_INFO:
		rtn = isp_u_pdaf_ppi_info(cxt->isp_driver_handle, param0);
		break;
	case ISP_DEV_SET_AFL_CFG_PARAM:
		rtn = isp_u_anti_flicker_block(cxt->isp_driver_handle, param0);
		break;
	case ISP_DEV_SET_AFL_NEW_CFG_PARAM:
		rtn = isp_u_anti_flicker_new_block(cxt->isp_driver_handle, param0);
		break;
	case ISP_DEV_SET_AFL_BYPASS:
		rtn = isp_u_anti_flicker_bypass(cxt->isp_driver_handle, param0);
		break;
	case ISP_DEV_SET_AFL_NEW_BYPASS:
		rtn = isp_u_anti_flicker_new_bypass(cxt->isp_driver_handle, param0);
		break;
	case ISP_DEV_SET_AE_SHIFT:
		rtn = isp_u_raw_aem_shift(cxt->isp_driver_handle, param0);
		break;
	case ISP_DEV_SET_AF_WORK_MODE:
		rtn = isp_u_raw_afm_mode(cxt->isp_driver_handle, *(cmr_u32 *)param0);
		break;
	case ISP_DEV_SET_AF_SKIP_NUM:
		rtn = isp_u_raw_afm_skip_num(cxt->isp_driver_handle, *(cmr_u32 *)param0);
		break;
	case ISP_DEV_SET_AF_IIR_CFG:
		rtn = isp_u_raw_afm_iir_nr_cfg(cxt->isp_driver_handle, param0);
		break;
	case ISP_DEV_SET_AF_MODULES_CFG:
		rtn = isp_u_raw_afm_modules_cfg(cxt->isp_driver_handle, param0);
		break;
	case ISP_DEV_POST_3DNR:
		rtn = isp_dev_3dnr(cxt->isp_driver_handle, param0);
	default:
		break;
	}
	return rtn;
}
