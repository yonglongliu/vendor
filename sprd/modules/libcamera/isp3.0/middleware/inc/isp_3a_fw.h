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
#ifndef _ISP_3A_FW_H_
#define _ISP_3A_FW_H_
/*----------------------------------------------------------------------------*
**				Dependencies					*
**---------------------------------------------------------------------------*/

#include <sys/types.h>
#include "isp_type.h"
#include "isp_mw.h"
#include "isp_common_types.h"

#define ISP3A_HW3A_MAX_DLSQL_NUM	20

struct isp_3a_fw_init_in {
	cmr_handle caller_handle;
	cmr_u16 is_refocus;
	cmr_handle dev_access_handle;
	cmr_u32 camera_id;
	cmr_u32 idx_num;
	proc_callback isp_mw_callback;
	void *setting_param_ptr;
	void *setting_param_ptr_slv;
	struct isp_bin_info bin_info[ISP_INDEX_MAX];
	struct isp_bin_info bin_info_slv[ISP_INDEX_MAX];
	struct isp_size size;
	struct isp_data_info calibration_param;
	struct isp_lib_config af_config;
	struct isp_lib_config awb_config;
	struct isp_lib_config ae_config;
	struct isp_lib_config afl_config;
	struct isp_lib_config pdaf_config;
	void *sensor_lsc_golden_data;
	struct isp_ops ops;
	struct isp_sensor_ex_info ex_info;
	struct isp_sensor_ex_info ex_info_slv; // slave sensor
	struct sensor_otp_cust_info *otp_data;
	struct sensor_otp_cust_info *otp_data_slv;
	struct sensor_dual_otp_info *dual_otp;
	struct sensor_pdaf_info *pdaf_info;
};



/*
struct isp_3a_fw_postproc_in {
	cmr_u32 src_avail_height;
	cmr_u32 src_slice_height;
	cmr_u32 dst_slice_height;
	struct isp_img_frm src_frame;
	struct isp_img_frm dst_frame;
};
*/

struct isp_3a_fw_postproc_out {
	cmr_u32 out_height;
};

struct isp_3a_dld_sequence {
	cmr_u8 uc_sensor_id;//ucAHBSensoreID;
	cmr_u8 uc_is_single_3amode;//ucIsSingle3AMode;
	cmr_u8 ucpreview_basic_dsdseqlen;//ucPreview_Baisc_DldSeqLength;
	cmr_u8 ucpreview_adv_dldseqlen;//ucPreview_Adv_DldSeqLength;
	cmr_u8 ucfastcoverge_basic_dldseqlen;//ucFastConverge_Baisc_DldSeqLength;
	cmr_u8 aucpreview_basic_dldseq[ISP3A_HW3A_MAX_DLSQL_NUM];//aucPreview_Baisc_DldSeq[HW3A_MAX_DLSQL_NUM];
	cmr_u8 aucpreview_adv_dldseq[ISP3A_HW3A_MAX_DLSQL_NUM];//aucPreview_Adv_DldSeq[HW3A_MAX_DLSQL_NUM];
	cmr_u8 aucfastconverge_basic_dldseq[ISP3A_HW3A_MAX_DLSQL_NUM];//aucFastConverge_Baisc_DldSeq[HW3A_MAX_DLSQL_NUM];
};

struct isp_3a_cfg_param {
	struct isp3a_awb_hw_cfg awb_cfg;
	struct isp3a_ae_hw_cfg ae_cfg;
	struct isp3a_af_hw_cfg af_cfg;
	struct isp3a_afl_hw_cfg afl_cfg;
//	struct isp3a_yhis_hw_cfg yhis_cfg;
//	struct isp3a_subsample_hw_cfg subsample_cfg;
	struct isp_awb_gain awb_gain;
	struct isp_awb_gain awb_gain_balanced;
};

struct isp_3a_get_dld_in {
	cmr_u32 op_mode;
};

cmr_int isp_3a_fw_init(struct isp_3a_fw_init_in *input_ptr, cmr_handle *isp_3a_handle);
cmr_int isp_3a_fw_deinit(cmr_handle isp_3a_handle);
cmr_int isp_3a_fw_capability(cmr_handle isp_3a_handle, enum isp_capbility_cmd cmd, void *param_ptr);
cmr_int isp_3a_fw_ioctl(cmr_handle isp_3a_handle, enum isp_ctrl_cmd cmd, void *param_ptr);
cmr_int isp_3a_fw_start(cmr_handle isp_3a_handle, struct isp_video_start *param_ptr);
cmr_int isp_3a_fw_set_tuning_mode(cmr_handle isp_3a_handle, struct isp_video_start *input_ptr);
cmr_int isp_3a_fw_stop(cmr_handle isp_3a_handle);
cmr_int isp_3a_fw_receive_data(cmr_handle isp_3a_handle, cmr_int evt, void *data);
cmr_int isp_3a_fw_get_cfg(cmr_handle isp_3a_handle, struct isp_3a_cfg_param *data);
cmr_int isp_3a_fw_get_iso_speed(cmr_handle isp_3a_handle, cmr_u32 *hw_iso_speed);
cmr_int isp_3a_fw_get_dldseq(cmr_handle isp_3a_handle, struct isp_3a_get_dld_in *input, struct isp_3a_dld_sequence *data);
cmr_int isp_3a_fw_get_awb_gain(cmr_handle isp_3a_handle, struct isp_awb_gain *gain, struct isp_awb_gain *gain_balanced);
cmr_int isp_3a_fw_set_tuning_mode(cmr_handle isp_3a_handle, struct isp_video_start *param_ptr);
/**---------------------------------------------------------------------------*/

#endif

