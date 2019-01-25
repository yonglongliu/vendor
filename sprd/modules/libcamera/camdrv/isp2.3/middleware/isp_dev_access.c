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
//#define ISP_DEFAULT_CFG_FOR_BRING_UP

struct isp_dev_access_context {
	cmr_handle evt_alg_handle;
	isp_evt_cb isp_event_cb;
	cmr_handle isp_driver_handle;
	struct isp_statis_mem_info statis_mem_info;
};

cmr_int isp_get_statis_buf_vir_addr(cmr_handle isp_dev_handle, struct isp_statis_info *in_ptr, cmr_uint *u_addr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	if (!isp_dev_handle) {
		ret = ISP_PARAM_ERROR;
		return ret;
	}

	*u_addr = cxt->statis_mem_info.isp_statis_u_addr + in_ptr->addr_offset;

	ISP_LOGV("u_addr = 0x%lx, isp_statis_u_addr = 0x%lx, addr_offset = 0x%x",
		*u_addr, cxt->statis_mem_info.isp_statis_u_addr, in_ptr->addr_offset);

	return ret;
}

cmr_int isp_dev_statis_buf_malloc(cmr_handle isp_dev_handle, struct isp_statis_mem_info *in_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;
	struct isp_statis_mem_info *statis_mem_info = &cxt->statis_mem_info;
	cmr_s32 fds[2];
	cmr_uint kaddr[2];
	cmr_uint stats_buffer_size = 0;

	statis_mem_info->isp_lsc_mem_size = in_ptr->isp_lsc_mem_size;
	statis_mem_info->isp_lsc_mem_num = in_ptr->isp_lsc_mem_num;
	statis_mem_info->isp_lsc_physaddr = in_ptr->isp_lsc_physaddr;
	statis_mem_info->isp_lsc_virtaddr = in_ptr->isp_lsc_virtaddr;
	statis_mem_info->lsc_mfd = in_ptr->lsc_mfd;

	if (in_ptr->statis_valid & ISP_STATIS_VALID_AEM)
		stats_buffer_size += ISP_AEM_STATIS_BUF_SIZE;
	if (in_ptr->statis_valid & ISP_STATIS_VALID_AFM)
		stats_buffer_size += ISP_AFM_STATIS_BUF_SIZE;
	if (in_ptr->statis_valid & ISP_STATIS_VALID_AFL)
		stats_buffer_size += ISP_AFL_STATIS_BUF_SIZE;
	if (in_ptr->statis_valid & ISP_STATIS_VALID_BINNING)
		stats_buffer_size += ISP_BINNING_STATIS_BUF_SIZE;
	if (in_ptr->statis_valid & ISP_STATIS_VALID_HIST)
		stats_buffer_size += ISP_HIST_STATIS_BUF_SIZE;
	if (in_ptr->statis_valid & ISP_STATIS_VALID_HIST2)
		stats_buffer_size += ISP_HIST2_STATIS_BUF_SIZE;
	if (in_ptr->statis_valid & ISP_STATIS_VALID_PDAF)
		stats_buffer_size += ISP_PDAF_STATIS_BUF_SIZE;
	if (in_ptr->statis_valid & ISP_STATIS_VALID_EBD)
		stats_buffer_size += ISP_EBD_STATIS_BUF_SIZE;

	if (statis_mem_info->isp_statis_alloc_flag == 0) {
		statis_mem_info->buffer_client_data = in_ptr->buffer_client_data;
		statis_mem_info->cb_of_malloc = in_ptr->cb_of_malloc;
		statis_mem_info->cb_of_free = in_ptr->cb_of_free;
		statis_mem_info->isp_statis_mem_size = stats_buffer_size * 5;
		statis_mem_info->isp_statis_mem_num = 1;
		statis_mem_info->statis_valid = in_ptr->statis_valid;
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
			return -ISP_PARAM_NULL;
		}

		statis_mem_info->isp_statis_k_addr[0] = kaddr[0];
		statis_mem_info->isp_statis_k_addr[1] = kaddr[1];
		statis_mem_info->statis_mfd = fds[0];
		statis_mem_info->statis_buf_dev_fd = fds[1];
		statis_mem_info->isp_statis_alloc_flag = 1;
	}

	return ret;
}

cmr_int isp_dev_trans_addr(cmr_handle isp_dev_handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;
	struct isp_statis_mem_info *statis_mem_info = &cxt->statis_mem_info;
	struct isp_file *file = NULL;
	struct isp_statis_buf_input isp_statis_buf;
	struct isp_statis_buf_input lsc_2d_buf;

	memset(&isp_statis_buf, 0x00, sizeof(isp_statis_buf));
	memset(&lsc_2d_buf, 0x00, sizeof(lsc_2d_buf));
	file = (struct isp_file *)(cxt->isp_driver_handle);
	file->dcam_lsc_vaddr = (void *)statis_mem_info->isp_lsc_virtaddr;

	/*set buffer to kernel */
	lsc_2d_buf.buf_size = statis_mem_info->isp_lsc_mem_size;
	lsc_2d_buf.buf_num = statis_mem_info->isp_lsc_mem_num;
	lsc_2d_buf.phy_addr = statis_mem_info->isp_lsc_physaddr;
	lsc_2d_buf.vir_addr = statis_mem_info->isp_lsc_virtaddr;
	lsc_2d_buf.mfd = statis_mem_info->lsc_mfd;
	ISP_LOGV("dcam lsc buf_size:0x%x, buf_num:%d, phy_addr:0x%lx, vir_addr:0x%lx, mfd:%ld",
			lsc_2d_buf.buf_size,
			lsc_2d_buf.buf_num,
			lsc_2d_buf.phy_addr,
			lsc_2d_buf.vir_addr,
			lsc_2d_buf.mfd);

	isp_statis_buf.buf_size = statis_mem_info->isp_statis_mem_size;
	isp_statis_buf.buf_num = statis_mem_info->isp_statis_mem_num;
	isp_statis_buf.kaddr[0] = statis_mem_info->isp_statis_k_addr[0];
	isp_statis_buf.kaddr[1] = statis_mem_info->isp_statis_k_addr[1];
	isp_statis_buf.vir_addr = statis_mem_info->isp_statis_u_addr;
	isp_statis_buf.buf_flag = 0;
	isp_statis_buf.statis_valid = statis_mem_info->statis_valid;
	isp_statis_buf.mfd = statis_mem_info->statis_mfd;
	isp_statis_buf.dev_fd = statis_mem_info->statis_buf_dev_fd;

	/*lsc addr transfer */
	dcam_u_2d_lsc_transaddr(cxt->isp_driver_handle, &lsc_2d_buf);

	ret = isp_dev_access_ioctl(isp_dev_handle, ISP_DEV_SET_STSTIS_BUF, &isp_statis_buf, NULL);

	return ret;
}

cmr_int isp_dev_set_interface(struct isp_drv_interface_param *in_ptr)
{
	cmr_int ret = ISP_SUCCESS;

	if (!in_ptr) {
		ret = ISP_PARAM_ERROR;
		goto exit;
	}

	ret = isp_set_comm_param((cmr_handle) in_ptr);
	ISP_TRACE_IF_FAIL(ret, ("fail to isp_set_comm_param"));

	ret = isp_set_slice_size((cmr_handle) in_ptr);
	ISP_TRACE_IF_FAIL(ret, ("fail to isp_set_slice_size"));

	ret = isp_set_fetch_param((cmr_handle) in_ptr);
	ISP_TRACE_IF_FAIL(ret, ("fail to isp_set_fetch_param"));

	ret = isp_set_store_param((cmr_handle) in_ptr);
	ISP_TRACE_IF_FAIL(ret, ("fail to isp_set_store_param"));

	ret = isp_set_dispatch((cmr_handle) in_ptr);
	ISP_TRACE_IF_FAIL(ret, ("fail to isp_set_dispatch"));

	ret = isp_set_arbiter((cmr_handle) in_ptr);
	ISP_TRACE_IF_FAIL(ret, ("fail to isp_set_arbiter"));

exit:
	return ret;
}

cmr_int isp_dev_start(cmr_handle isp_dev_handle, cmr_u32 mode_id, struct isp_drv_interface_param *in_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;
	struct isp_u_blocks_info dev_blocks_info;
#ifdef ISP_DEFAULT_CFG_FOR_BRING_UP
	struct isp_dev_cfa_info cfa_param;
#endif

	memset(&dev_blocks_info, 0x0, sizeof(dev_blocks_info));

	isp_u_fetch_raw_transaddr(cxt->isp_driver_handle, &in_ptr->fetch.fetch_addr);

	ret = isp_get_fetch_addr(in_ptr, &in_ptr->fetch);
	ISP_RETURN_IF_FAIL(ret, ("fail to get isp fetch addr"));

	if (mode_id >= ISP_MODE_ID_CAP_0 &&
			mode_id <= ISP_MODE_ID_CAP_3)
		dev_blocks_info.scene_id = 1;
	else
		dev_blocks_info.scene_id = 0;

#ifdef ISP_DEFAULT_CFG_FOR_BRING_UP
	ret = isp_get_cfa_default_param(in_ptr, &cfa_param);
	ISP_RETURN_IF_FAIL(ret, ("fail to get isp cfa default param"));

	dev_blocks_info.block_info = (void *)&in_ptr->cfa_param;
	ret = isp_u_cfa_block(cxt->isp_driver_handle, (void *)&dev_blocks_info);
	ISP_RETURN_IF_FAIL(ret, ("fail to cfg isp cfa"));
#endif
	dev_blocks_info.block_info = (void *)&in_ptr->fetch;
	ret = isp_u_fetch_block(cxt->isp_driver_handle, (void *)&dev_blocks_info);
	ISP_RETURN_IF_FAIL(ret, ("fail to cfg isp fetch"));

	dev_blocks_info.block_info = (void *)&in_ptr->store;
	ret = isp_u_store_block(cxt->isp_driver_handle, (void *)&dev_blocks_info);
	ISP_RETURN_IF_FAIL(ret, ("fail to cfg isp store"));

	dev_blocks_info.block_info = (void *)&in_ptr->dispatch;
	ret = isp_u_dispatch_block(cxt->isp_driver_handle, (void *)&dev_blocks_info);
	ISP_RETURN_IF_FAIL(ret, ("fail to cfg isp dispatch"));

	dev_blocks_info.block_info = (void *)&in_ptr->arbiter;
	ret = isp_u_arbiter_block(cxt->isp_driver_handle, (void *)&dev_blocks_info);
	ISP_RETURN_IF_FAIL(ret, ("fail to cfg isp arbiter"));

	dev_blocks_info.block_info = (void *)&in_ptr->com;
	isp_cfg_comm_data(cxt->isp_driver_handle, &dev_blocks_info);

	dev_blocks_info.block_info = (void *)&in_ptr->slice;
	isp_cfg_slice_size(cxt->isp_driver_handle, &dev_blocks_info);

	return ret;
}

cmr_int isp_dev_anti_flicker_new_bypass(cmr_handle isp_dev_handle, struct isp_u_blocks_info *block_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	ret = isp_u_anti_flicker_new_bypass(cxt->isp_driver_handle, (void *)block_ptr);

	return ret;
}

cmr_int isp_dev_cfg_block(cmr_handle isp_dev_handle, void *data_ptr, cmr_int data_id)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	ret = isp_cfg_block(cxt->isp_driver_handle, data_ptr, data_id);

	return ret;
}

cmr_int isp_dev_awb_gain(cmr_handle isp_dev_handle, struct isp_u_blocks_info *block_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;
	ret = isp_u_awbc_gain(cxt->isp_driver_handle, (void *)block_ptr);
	return ret;
}

void isp_dev_access_evt_reg(cmr_handle isp_dev_handle, isp_evt_cb isp_event_cb, void *privdata)
{
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	cxt->isp_event_cb = isp_event_cb;
	cxt->evt_alg_handle = privdata;
}

void isp_dev_statis_info_proc(cmr_handle isp_dev_handle, void *param_ptr)
{
	struct sprd_img_statis_info *irq_info = (struct sprd_img_statis_info *)param_ptr;
	struct isp_statis_info *statis_info = NULL;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	statis_info = malloc(sizeof(*statis_info));

	statis_info->phy_addr = irq_info->phy_addr;
	statis_info->vir_addr = irq_info->vir_addr;
	statis_info->addr_offset = irq_info->addr_offset;
	statis_info->kaddr[0] = irq_info->kaddr[0];
	statis_info->kaddr[1] = irq_info->kaddr[1];
	statis_info->irq_property = irq_info->irq_property;
	statis_info->buf_size = irq_info->buf_size;
	statis_info->mfd = irq_info->mfd;
	statis_info->frame_id = irq_info->frame_id;
	statis_info->sec = irq_info->sec;
	statis_info->usec = irq_info->usec;
	statis_info->dac_info = irq_info->dac_info;

	ISP_LOGV("got one frame stats offset 0x%x vaddr 0x%x property %d frame id %d timestamp %ds %dus",
		 statis_info->addr_offset, statis_info->vir_addr,
		 statis_info->irq_property,
		 statis_info->frame_id,
		 statis_info->sec, statis_info->usec);
	if (irq_info->irq_property == IRQ_AEM_STATIS) {
		if (cxt->isp_event_cb) {
			(*cxt->isp_event_cb) (ISP_CTRL_EVT_AE, statis_info, (void *)cxt->evt_alg_handle);
		}
	} else if (irq_info->irq_property == IRQ_AFM_STATIS) {
		if (cxt->isp_event_cb) {
			(*cxt->isp_event_cb) (ISP_CTRL_EVT_AF, statis_info, (void *)cxt->evt_alg_handle);
		}
	} else if (irq_info->irq_property == IRQ_DCAM_PULSE) {
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
	} else if (irq_info->irq_property == IRQ_EBD_STATIS) {
		if (cxt->isp_event_cb) {
			(*cxt->isp_event_cb) (ISP_CTRL_EVT_EBD, statis_info, (void *)cxt->evt_alg_handle);
		}
	} else {
		free((void *)statis_info);
		statis_info = NULL;
	}
}

void isp_dev_irq_info_proc(cmr_handle isp_dev_handle, void *param_ptr)
{
	struct sprd_irq_info *irq_info = (struct sprd_irq_info *)param_ptr;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	if (irq_info->irq_property == IRQ_DCAM_SOF) {
		if (cxt->isp_event_cb) {
			(cxt->isp_event_cb) (ISP_CTRL_EVT_SOF, NULL, (void *)cxt->evt_alg_handle);
		}
	} else if (irq_info->irq_property == IRQ_RAW_CAP_DONE) {
		if (cxt->isp_event_cb) {
			(cxt->isp_event_cb) (ISP_CTRL_EVT_TX, NULL, (void *)cxt->evt_alg_handle);
		}
	} else {
		ISP_LOGW("there is no irq_property");
	}
}

cmr_int isp_dev_access_init(cmr_s32 fd, cmr_handle *isp_dev_handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = NULL;

	cxt = (struct isp_dev_access_context *)malloc(sizeof(struct isp_dev_access_context));
	if (NULL == cxt) {
		ISP_LOGE("fail to malloc");
		ret = -ISP_ALLOC_ERROR;
		goto exit;
	}
	*isp_dev_handle = NULL;
	memset((void *)cxt, 0x00, sizeof(*cxt));

	ret = isp_dev_open(fd, &cxt->isp_driver_handle);

	if (ret) {
		ISP_LOGE("fail to open isp dev!");
		goto exit;
	}
exit:
	if (ret) {
		if (cxt) {
			free((void *)cxt);
			cxt = NULL;
		}
	} else {
		*isp_dev_handle = (cmr_handle) cxt;
	}

	ISP_LOGI("done %ld", ret);

	return ret;
}

cmr_int isp_dev_access_deinit(cmr_handle handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)handle;
	struct isp_statis_mem_info *statis_mem_info = &cxt->statis_mem_info;
	cmr_uint type = 0;

	ISP_CHECK_HANDLE_VALID(handle);
	ISP_LOGI("X");

	ret = isp_dev_mask_3a_int(cxt->isp_driver_handle);
	if (ret) {
		ISP_LOGW("fail to mask 3a but deinit can continue");
	}

	if (statis_mem_info->isp_statis_alloc_flag == 1) {
		type = CAMERA_ISP_STATIS;

		if (statis_mem_info->cb_of_free) {
			isp_cb_of_free cb_free = statis_mem_info->cb_of_free;

			cb_free(type, &statis_mem_info->isp_statis_k_addr[0],
				&statis_mem_info->isp_statis_u_addr,
				&statis_mem_info->statis_mfd,
				statis_mem_info->isp_statis_mem_num,
				statis_mem_info->buffer_client_data);
		}

		statis_mem_info->isp_statis_alloc_flag = 0;
	}
	isp_dev_close(cxt->isp_driver_handle);
	if (cxt) {
		free((void *)cxt);
		cxt = NULL;
	}

	ISP_LOGI("done %ld", ret);

	return ret;
}

cmr_int isp_dev_access_capability(cmr_handle isp_dev_handle, enum isp_capbility_cmd cmd, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	if (!isp_dev_handle || !param_ptr) {
		ret = ISP_PARAM_ERROR;
		goto exit;
	}

	switch (cmd) {
	case ISP_VIDEO_SIZE:{
			struct isp_video_limit *size_ptr = param_ptr;

			ret = isp_u_capability_continue_size(cxt->isp_driver_handle, &size_ptr->width, &size_ptr->height);
			break;
		}
	default:
		break;
	}

exit:
	return ret;
}

static cmr_int ispdev_access_ae_set_rgb_gain(cmr_handle isp_dev_handle, struct isp_u_blocks_info *block_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;
	cmr_u32 rgb_gain_offset = 4096;
	struct isp_dev_rgb_gain_info gain_info;

	gain_info.bypass = 0;
	gain_info.global_gain = block_ptr->rgb_gain_coeff;
	gain_info.r_gain = rgb_gain_offset;
	gain_info.g_gain = rgb_gain_offset;
	gain_info.b_gain = rgb_gain_offset;

	ISP_LOGV("d-gain--global_gain: %d\n", gain_info.global_gain);

	block_ptr->block_info = &gain_info;
	isp_u_rgb_gain_block(cxt->isp_driver_handle, (void *)block_ptr);

	return ret;
}

static cmr_int ispdev_access_set_af_monitor(cmr_handle isp_dev_handle, struct isp_u_blocks_info *block_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;
	struct isp_u_blocks_info afm_block_info;

	memset(&afm_block_info, 0x0, sizeof(afm_block_info));
	afm_block_info.scene_id = block_ptr->scene_id;

	afm_block_info.afm_info.bypass = 1;
	isp_u_raw_afm_bypass(cxt->isp_driver_handle, (void *)&afm_block_info);

	afm_block_info.clear = 1;
	isp_u_raw_afm_skip_num_clr(cxt->isp_driver_handle, (void *)&afm_block_info);

	afm_block_info.clear = 0;
	isp_u_raw_afm_skip_num_clr(cxt->isp_driver_handle, (void *)&afm_block_info);

	isp_u_raw_afm_skip_num(cxt->isp_driver_handle, (void *)block_ptr);
	isp_u_raw_afm_bypass(cxt->isp_driver_handle, (void *)block_ptr);

	return ret;
}

static cmr_int ispdev_access_set_slice_raw(cmr_handle isp_dev_handle, struct isp_raw_proc_info *info)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_u_blocks_info fetch_info;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	memset(&fetch_info, 0x0, sizeof(fetch_info));

	ret = isp_dev_set_slice_raw_info(cxt->isp_driver_handle, info);
	ISP_TRACE_IF_FAIL(ret, ("fail to slice raw info"));

	/* raw capture */
	fetch_info.scene_id = 1;
	fetch_info.fetch_start = ISP_ONE;
	ret = isp_u_fetch_start_isp(cxt->isp_driver_handle, (void *)&fetch_info);
	ISP_TRACE_IF_FAIL(ret, ("fail to fetch start isp"));

	return ret;
}

static cmr_int ispdev_access_aem_stats_info(cmr_handle isp_dev_handle,  struct isp_u_blocks_info *block_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;


	if (!cxt || !block_ptr) {
		ISP_LOGE("fail to get ptr:%p, %p", cxt, block_ptr);
		return -ISP_PARAM_NULL;
	}

	isp_u_raw_aem_block(cxt->isp_driver_handle, (void *)block_ptr);
	ISP_TRACE_IF_FAIL(ret, ("fail to set aem block."));

	/* set aem bypass */
	isp_u_raw_aem_bypass(cxt->isp_driver_handle, (void *)block_ptr);
	ISP_TRACE_IF_FAIL(ret, ("fail to set aem bypass."));

	/* set aem skip num */
	ret = isp_u_raw_aem_skip_num(cxt->isp_driver_handle, (void *)block_ptr);
	ISP_TRACE_IF_FAIL(ret, ("fail to set aem skip num."));

	/* set single mode :0 or continue mode: 1 */
	isp_u_raw_aem_mode(cxt->isp_driver_handle, (void *)block_ptr);
	ISP_TRACE_IF_FAIL(ret, ("fail to set aem mode."));

	/* set aem offset */
	ret = isp_u_raw_aem_offset(cxt->isp_driver_handle, (void *)block_ptr);
	ISP_TRACE_IF_FAIL(ret, ("fail to aem offset"));

	/* set aem blk_size */
	ret = isp_u_raw_aem_blk_size(cxt->isp_driver_handle, (void *)block_ptr);
	ISP_TRACE_IF_FAIL(ret, ("fail to aem blk"));

	return ret;
}

static cmr_int ispdev_access_get_timestamp(cmr_handle isp_dev_handle, struct isp_time *time)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;
	cmr_u32 sec = 0;
	cmr_u32 usec = 0;

	ret = isp_u_capability_time(cxt->isp_driver_handle, &sec, &usec);
	time->sec = sec;
	time->usec = usec;

	return ret;
}

cmr_int isp_dev_access_ioctl(cmr_handle isp_dev_handle, cmr_int cmd, void *in, void *out)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	switch (cmd) {
	case ISP_DEV_SET_AE_STATS_MONITOR:
		ret = ispdev_access_aem_stats_info(cxt, in);
		break;
	case ISP_DEV_SET_RGB_GAIN:
		ret = ispdev_access_ae_set_rgb_gain(cxt, in);
		break;
	case ISP_DEV_GET_SYSTEM_TIME:
		ret = ispdev_access_get_timestamp(cxt, out);
		break;
	case ISP_DEV_RESET:
		ret = isp_dev_reset(cxt->isp_driver_handle);
		break;
	case ISP_DEV_SET_AFL_NEW_BLOCK:
		ret = isp_u_anti_flicker_new_block(cxt->isp_driver_handle, in);
		break;
	case ISP_DEV_RAW_AEM_BYPASS:
		ret = isp_u_raw_aem_bypass(cxt->isp_driver_handle, in);
		break;
	case ISP_DEV_RAW_AFM_BYPASS:
		ret = isp_u_raw_afm_bypass(cxt->isp_driver_handle, in);
		break;
	case ISP_DEV_SET_AF_MONITOR:
		ret = ispdev_access_set_af_monitor(cxt, in);
		break;
	case ISP_DEV_SET_AF_MONITOR_WIN:
		ret = isp_u_raw_afm_win(cxt->isp_driver_handle, in);
		break;
	case ISP_DEV_GET_AF_MONITOR_WIN_NUM:
		ret = isp_u_raw_afm_win_num(cxt->isp_driver_handle, out);
		break;
	case ISP_DEV_SET_STSTIS_BUF:
		ret = isp_dev_set_statis_buf(cxt->isp_driver_handle, in);
		break;
	case ISP_DEV_SET_PDAF_CFG_PARAM:
		ret = isp_u_pdaf_block(cxt->isp_driver_handle, in);
		break;
	case ISP_DEV_SET_PDAF_BYPASS:
		ret = isp_u_pdaf_bypass(cxt->isp_driver_handle, in);
		break;
	case ISP_DEV_SET_PDAF_WORK_MODE:
		ret = isp_u_pdaf_work_mode(cxt->isp_driver_handle, in);
		break;
	case ISP_DEV_SET_PDAF_SKIP_NUM:
		ret = isp_u_pdaf_skip_num(cxt->isp_driver_handle, in);
		break;
	case ISP_DEV_SET_PDAF_EXTRACTOR_BYPASS:
		ret = isp_u_pdaf_extractor_bypass(cxt->isp_driver_handle, in);
		break;
	case ISP_DEV_SET_PDAF_ROI:
		ret = isp_u_pdaf_roi(cxt->isp_driver_handle, in);
		break;
	case ISP_DEV_SET_PDAF_PPI_INFO:
		ret = isp_u_pdaf_ppi_info(cxt->isp_driver_handle, in);
		break;
	case ISP_DEV_SET_AFL_NEW_CFG_PARAM:
		ret = isp_u_anti_flicker_new_block(cxt->isp_driver_handle, in);
		break;
	case ISP_DEV_SET_AFL_NEW_BYPASS:
		ret = isp_u_anti_flicker_new_bypass(cxt->isp_driver_handle, in);
		break;
	case ISP_DEV_SET_BINNING_BYPASS:
		ret = isp_u_binning_bypass(cxt->isp_driver_handle, in);
		break;
	case ISP_DEV_SET_HIST_BYPASS:
		ret = isp_u_hist_bypass(cxt->isp_driver_handle, in);
		break;
	case ISP_DEV_SET_HIST2_BYPASS:
		ret = isp_u_hist2_bypass(cxt->isp_driver_handle, in);
		break;
	case ISP_DEV_SET_AE_SHIFT:
		ret = isp_u_raw_aem_shift(cxt->isp_driver_handle, in);
		break;
	case ISP_DEV_SET_AF_WORK_MODE:
		ret = isp_u_raw_afm_mode(cxt->isp_driver_handle, in);
		break;
	case ISP_DEV_SET_AF_SKIP_NUM:
		ret = isp_u_raw_afm_skip_num(cxt->isp_driver_handle, in);
		break;
	case ISP_DEV_SET_AF_IIR_CFG:
		ret = isp_u_raw_afm_iir_nr_cfg(cxt->isp_driver_handle, in);
		break;
	case ISP_DEV_SET_AF_MODULES_CFG:
		ret = isp_u_raw_afm_modules_cfg(cxt->isp_driver_handle, in);
		break;
	case ISP_DEV_POST_3DNR:
		ret = isp_dev_3dnr(cxt->isp_driver_handle, in);
		break;
	case ISP_DEV_SET_RAW_SLICE:
		ret = ispdev_access_set_slice_raw(cxt, in);
		break;
	default:
		break;
	}
	return ret;
}
