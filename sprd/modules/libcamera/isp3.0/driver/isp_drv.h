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
#ifndef _ISP_DRV_H_
#define _ISP_DRV_H_

#ifndef WIN32
#include <sys/types.h>
#include <utils/Log.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>
#endif
#include <utils/Timers.h>
#include "isp_type.h"
#include "sprd_isp_altek.h"
#include "debug_structure.h"

typedef void*           isp_handle;
typedef void (*isp_evt_cb)(cmr_int evt, void *data, cmr_u32 data_len, void *privdata);

enum isp_drv_evt {
	ISP_DRV_STATISTICE,
	ISP_DRV_AF_STATISTICE,
	ISP_DRV_SENSOR_SOF,
	ISP_DRV_RAW10,
	ISP_DRV_CFG_BUF,
	ISP_DEV_EVT_MAX
};

struct isp_dev_init_info {
	cmr_u32         camera_id;
	cmr_u32         width;
	cmr_u32         height;
	cmr_malloc      alloc_cb;
	cmr_free        free_cb;
	cmr_handle      mem_cb_handle;
	void            *setting_param_list_ptr[3];//0:back,1:front,2:dual back,
	uint32          shading_bin_size;
	uint8           *shading_bin_addr;
	uint32          shading_bin_offset;
	uint32          irp_bin_size;
	uint8           *irp_bin_addr;
	uint32          irp_bin_offset;
	uint32          cbc_bin_size;
	uint8           *cbc_bin_addr;
	uint32          cbc_bin_offset;
	cmr_u8          pdaf_supported;
};

struct isp_img_fd {
	cmr_u32         y;
	cmr_u32         u;
	cmr_u32         v;
};

struct isp_statis_info {
	struct isp_statis_frame_output  statis_frame;
	nsecs_t                         timestamp;
	cmr_uint                        statis_cnt;
	cmr_uint                        irq_statis_cnt;
};

struct isp_irq_node {
	uint32_t                     irq_val0;
	uint32_t                     sof_idx;
	int32_t                      ret_val;
	struct sprd_isp_time         time_stamp;
};

/*ISP driver API */
cmr_int isp_dev_init(struct isp_dev_init_info *init_param_ptr, isp_handle *handle);
cmr_int isp_dev_deinit(isp_handle handle);
cmr_int isp_dev_start(isp_handle handle);
cmr_int isp_dev_alloc_highiso_mem(isp_handle handle, struct isp_raw_data *buf, struct isp_img_size *size);
void isp_dev_evt_reg(isp_handle handle, isp_evt_cb isp_event_cb, void *privdata);
cmr_int isp_dev_stop(isp_handle handle);
cmr_int isp_dev_stream_on(isp_handle handle);
cmr_int isp_dev_stream_off(isp_handle handle);
cmr_int isp_dev_load_cbc(isp_handle handle);
cmr_int isp_dev_pdaf_supported(isp_handle handle);
cmr_int isp_dev_set_rawaddr(isp_handle handle, struct isp_raw_data *param);
cmr_int isp_dev_set_post_yuv_mem(isp_handle handle, struct isp_img_mem *param);
cmr_int isp_dev_set_fetch_src_buf(isp_handle handle, struct isp_img_mem *param);
cmr_int isp_dev_load_firmware(isp_handle handle, struct isp_init_mem_param *param);
cmr_int isp_dev_sel_iq_param_index(isp_handle handle, cmr_u32 tuning_idx);
cmr_int isp_dev_set_statis_buf(isp_handle handle, struct isp_statis_buf *param);
cmr_int isp_dev_get_statis_buf(isp_handle handle, struct isp_img_read_op *param);
cmr_int isp_dev_set_img_buf(isp_handle handle, struct isp_cfg_img_buf *param);
cmr_int isp_dev_get_img_buf(isp_handle handle, struct isp_img_read_op *param);
cmr_int isp_dev_set_img_param(isp_handle handle, struct isp_cfg_img_param *param);
cmr_int isp_dev_get_timestamp(isp_handle handle, cmr_u32 *sec, cmr_u32 *usec);
cmr_int isp_dev_drammode_takepic(isp_handle handle, cmr_u32 is_start);
cmr_int isp_dev_highiso_mode(isp_handle handle, struct isp_raw_data *param);
cmr_int isp_dev_cfg_scenario_info(isp_handle handle, struct scenario_info_ap *data);
cmr_int isp_dev_cfg_iso_speed(isp_handle handle, cmr_u32 *data);
cmr_int isp_dev_cfg_awb_gain(isp_handle handle, struct isp_awb_gain_info *data);
cmr_int isp_dev_cfg_awb_gain_balanced(isp_handle handle, struct isp_awb_gain_info *data);
cmr_int isp_dev_cfg_dld_seq(isp_handle handle, struct dld_sequence *data);
cmr_int isp_dev_cfg_3a_param(isp_handle handle, struct cfg_3a_info *data);
cmr_int isp_dev_cfg_ae_param(isp_handle handle, struct ae_cfg_info *data);
cmr_int isp_dev_cfg_af_param(isp_handle handle, struct af_cfg_info *data);
cmr_int isp_dev_cfg_awb_param(isp_handle handle, struct awb_cfg_info *data);
cmr_int isp_dev_cfg_yhis_param(isp_handle handle, struct yhis_cfg_info *data);
cmr_int isp_dev_cfg_sub_sample(isp_handle handle, struct subsample_cfg_info *data);
cmr_int isp_dev_cfg_anti_flicker(isp_handle handle, struct antiflicker_cfg_info *data);
cmr_int isp_dev_cfg_dld_seq_basic_prev(isp_handle handle, cmr_u32 size, cmr_u8 *data);
cmr_int isp_dev_cfg_dld_seq_adv_prev(isp_handle handle, cmr_u32 size, cmr_u8 *data);
cmr_int isp_dev_cfg_dld_seq_fast_converge(isp_handle handle, cmr_u32 size, cmr_u8 *data);
cmr_int isp_dev_cfg_sharpness(isp_handle handle, cmr_u32 mode);
cmr_int isp_dev_cfg_saturation(isp_handle handle, cmr_u32 mode);
cmr_int isp_dev_cfg_contrast(isp_handle handle, cmr_u32 mode);
cmr_int isp_dev_cfg_special_effect(isp_handle handle, cmr_u32 mode);
cmr_int isp_dev_cfg_brightness_gain(isp_handle handle, struct isp_brightness_gain *data);
cmr_int isp_dev_cfg_brightness_mode(isp_handle handle, cmr_u32 mode);
cmr_int isp_dev_cfg_color_temp(isp_handle handle, cmr_u32 mode);
cmr_int isp_dev_cfg_ccm(isp_handle handle, struct isp_iq_ccm_info *data);
cmr_int isp_dev_cfg_valid_adgain(isp_handle handle, cmr_u32 mode);
cmr_int isp_dev_cfg_exp_time(isp_handle handle, cmr_u32 mode);
cmr_int isp_dev_cfg_otp_info(isp_handle handle, struct isp_iq_otp_info *data);
cmr_int isp_dev_cfg_sof_info(isp_handle handle, struct isp_sof_cfg_info *data);
cmr_int isp_dev_cfg_d_gain(isp_handle handle, struct altek_d_gain_info *data);
cmr_int isp_dev_cfg_y_offset(isp_handle handle, cmr_u32 *data);
cmr_int isp_dev_capability_fw_size(isp_handle handle, cmr_int *size);
cmr_int isp_dev_capability_statis_buf_size(isp_handle handle, cmr_int *size);
cmr_int isp_dev_capability_dram_buf_size(isp_handle handle, cmr_int *size);
cmr_int isp_dev_capability_highiso_buf_size(isp_handle handle, cmr_int *size);
cmr_int isp_dev_capability_video_size(isp_handle handle, struct isp_img_size *size);
cmr_int isp_dev_capability_single_size(isp_handle handle, struct isp_img_size *size);
cmr_int isp_dev_get_isp_id(isp_handle handle, cmr_u32 *isp_id);
cmr_int isp_dev_get_user_cnt(isp_handle handle, cmr_s32 *cnt);
cmr_int isp_dev_get_iq_param(isp_handle handle, struct debug_info1 *info1, struct debug_info2 *info2);
cmr_int isp_dev_get_img_iq_param(isp_handle handle, struct debug_info1 *info1, struct debug_info2 *info2);
cmr_int isp_dev_set_capture_mode(isp_handle handle, cmr_u32 capture_mode);
cmr_int isp_dev_set_skip_num(isp_handle handle, cmr_u32 skip_num);
cmr_int isp_dev_set_deci_num(isp_handle handle, cmr_u32 deci_num);
cmr_int isp_dev_match_data_ctrl(isp_handle handle, struct isp_match_data_param *data);
cmr_int isp_dev_set_init_param(isp_handle *handle, struct isp_dev_init_param *init_param_ptr);
cmr_int isp_dev_cfg_cap_buf(isp_handle handle, struct isp_img_mem *param);
void isp_dev_buf_cfg_evt_reg(isp_handle handle, cmr_handle grab_handle, isp_evt_cb isp_event_cb);

#endif
