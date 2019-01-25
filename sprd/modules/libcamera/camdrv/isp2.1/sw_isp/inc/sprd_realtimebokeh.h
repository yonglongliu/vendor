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
#ifndef _SPRD_SW_ISP_H_
#define _SPRD_SW_ISP_H_
#include <sys/types.h>
//#include "cmr_types.h"
//#include "cmr_common.h"
#include "isp_mw.h"
#include "sprd_isp_r6p10.h"
#include "thread.h"
//#define SW_ISP_IMG_WIDTH  1600
//#define SW_ISP_IMG_HEIGHT 1200
//#define DEPTH_OUTPUT_WIDTH 960
//#define DEPTH_OUTPUT_HEIGHT 720
//#define PREVIEW_WIDTH 960
//#define PREVIEW_HEIGHT 720
//#define DEPTH_INPUT_WIDTH 960
//#define DEPTH_INPUT_HEIGHT 720
#define YNR_CONTROL_INTERNAL 1
#ifdef YNR_CONTROL_INTERNAL
typedef struct ynr_param_info
{
	uint32_t src_width;
	uint32_t src_height;
	uint32_t dst_width;
	uint32_t dst_height;
	int32_t src_buf_fd;
	int32_t dst_buf_fd;
}ynr_param_info_t;
typedef cmr_int (* ynr_ioctl_fun)(cmr_handle isp_handler,ynr_param_info_t *param_ptr);

#endif
typedef void (*soft_isp_evt_cb)(cmr_int evt, void *data, void *privdata);

enum soft_isp_evt_id {
	SOFT_IRQ_AEM_STATIS, //AEM done
	SOFT_BOKEH_DONE, //BOKEH done
	SOFT_BUFFER_RELEASE,
	SOFT_ISP_EVT_MAX,
};
typedef enum realtimebokeh_cmd {
	REALTIMEBOKEH_CMD_GET_ONLINECALBINFO,
	REALTIMEBOKEH_CMD_MAX,
}sprd_realtimebokeh_cmd;
struct soft_isp_init_param {
	cmr_handle isp_handle;
	soft_isp_evt_cb evt_cb;
	//sw isp info
	int32_t swisp_inputwidth;
	int32_t swisp_inputheight;
	int32_t swisp_inputwidthpitch;
	int32_t swisp_mainthreadnum;
	int32_t swisp_subthreadnum;
	//depth,bokeh info
	uint8_t* potp_buf;
	uint32_t otp_size;
#ifdef YNR_CONTROL_INTERNAL
	ynr_ioctl_fun ynr_control;
#endif
	uint32_t online_calb_width;
	uint32_t online_calb_height;
};

typedef struct soft_isp_startparam
{
	int32_t preview_width;
	int32_t preview_height;
	struct isp_img_frm s_yuv_depth; //sub sensor yuv after ynr maybe preview size 960x720
	struct isp_img_frm s_yuv_sw_out;
}soft_isp_startparam_t;

enum soft_isp_format {
	SOFT_ISP_DATA_NV21,
	SOFT_ISP_DATA_NORMAL_RAW10,
	SOFT_ISP_DATA_CSI2_RAW10,
	SOFT_ISP_DATA_FORMAT_MAX
};

struct soft_isp_block_param {
	int32_t blc_update;
	struct isp_dev_blc_info blc_info;
	int32_t rgb_gain_update;
	struct isp_dev_rgb_gain_info gain_info;
	int32_t lsc_update;
	struct isp_dev_2d_lsc_info lsc_info;
	int32_t aem_update;
	struct isp_dev_raw_aem_info aem_info;
	int32_t awb_update;
	struct isp_dev_awb_info awb_info;
	int32_t gamma_update;
	struct isp_dev_gamma_info gamma_info;
	int32_t rgb2y_update;
	struct isp_dev_rgb2y_info rgb2y_info;
	int32_t init_flag;
};

struct soft_isp_proc_param {
	//in param
	struct isp_img_frm raw; //raw sub sensor input raw data
	struct soft_isp_misc_img_frm m_yuv_pre; //preview input may be 960x720
	struct soft_isp_misc_img_frm m_yuv_bokeh; //bokeh result return to display
	struct soft_isp_block_param block; //isp block
	WeightParams_t weightparam; //weight param
	uint32_t af_status; //0:af done 1: af doing
	// out param
	cmr_int release_status; // 0: not used 1: using
	cmr_int bokeh_status; //0: valid 1: invalid
};

struct soft_isp_cb_info {
	cmr_uint vir_addr;
	uint32_t buf_size;
	struct isp_img_frm raw;
	struct soft_isp_misc_img_frm m_yuv_pre;
};
#if 0
typedef enum realtimebokeh_process_status
{
	REALTIMEBOKEH_STATUS_NORMAL,
	REALTIMEBOKEH_STATUS_NOBOKEH,
	REALTIMEBOKEH_STATUS_MISTAKESCEN,

}realbokeh_process_status;
#endif

#ifdef __cplusplus
extern "C" {
#endif

int sprd_realtimebokeh_init(struct soft_isp_init_param *param, void **handle);
int sprd_realtimebokeh_process(void* handle , struct soft_isp_proc_param *isp_param);
int sprd_realtimebokeh_deinit(void* handle);
#ifdef YNR_CONTROL_INTERNAL
void sprd_realtimebokeh_ynr_callback(void* handle);
#endif
int sprd_realtimebokeh_stop(void* handle);
int sprd_realtimebokeh_start(void* handle , struct soft_isp_startparam *startparam);
int sprd_realtimebokeh_ioctl(void *handle, sprd_realtimebokeh_cmd cmd , void *outinfo);
#ifdef __cplusplus
}
#endif

#endif
