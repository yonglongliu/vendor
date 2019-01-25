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
#define LOG_TAG "afl_ctrl"
#include "afl_ctrl.h"
#include "deflicker.h"
#include <cutils/properties.h>
#include <math.h>
#include "isp_pm.h"

#define ISP_AFL_BUFFER_LEN                   (3120 * 4 * 61)
#define ISP_SET_AFL_THR                      "isp.afl.thr"

#define AFLCTRL_EVT_BASE            0x2000
#define AFLCTRL_EVT_INIT            AFLCTRL_EVT_BASE
#define AFLCTRL_EVT_DEINIT          (AFLCTRL_EVT_BASE + 1)
#define AFLCTRL_EVT_IOCTRL          (AFLCTRL_EVT_BASE + 2)
#define AFLCTRL_EVT_PROCESS         (AFLCTRL_EVT_BASE + 3)
#define AFLCTRL_EVT_EXIT            (AFLCTRL_EVT_BASE + 4)

static cmr_s32 afl_check_handle(cmr_handle handle)
{
	struct isp_anti_flicker_cfg *cxt = (struct isp_anti_flicker_cfg *)handle;

	if (NULL == cxt) {
		ISP_LOGE("fail to check handle");
		return ISP_ERROR;
	}

	return ISP_SUCCESS;
}

static cmr_s32 _set_afl_thr(cmr_s32 * thr)
{
#ifdef WIN32
	return -1;
#else
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 temp = 0;
	char temp_thr[4] = { 0 };
	cmr_s32 i = 0, j = 0;
	char value[PROPERTY_VALUE_MAX];

	if (NULL == thr) {
		ISP_LOGE("fail to check thr,thr is NULL!");
		return -1;
	}

	property_get(ISP_SET_AFL_THR, value, "/dev/close_afl");
	ISP_LOGV(" _set_afl_thr:%s", value);

	if (strcmp(value, "/dev/close_afl")) {
		for (i = 0; i < 9; i++) {
			for (j = 0; j < 3; j++) {
				temp_thr[j] = value[3 * i + j];
			}
			temp_thr[j] = '\0';
			ISP_LOGV("temp_thr:%c, %c, %c", temp_thr[0], temp_thr[1], temp_thr[2]);
			temp = atoi(temp_thr);
			*thr++ = temp;

			if (0 == temp) {
				rtn = -1;
				ISP_LOGE("fail to check temp");
				break;
			}
		}
	} else {
		rtn = -1;
		return rtn;
	}
	return rtn;
#endif
}

static cmr_s32 aflctrl_save_to_file(cmr_s32 height, cmr_s32 * addr, cmr_u32 cnt)
{
	cmr_s32 i = 0;
	cmr_s32 *ptr = addr;
	FILE *fp = NULL;
	char file_name[100] = { 0 };
	char tmp_str[100];

	if (NULL == addr) {
		ISP_LOGE("fail to check param,param is NULL!");
		return -1;
	}

	ISP_LOGV("addr %p line num %d", ptr, height);

	strcpy(file_name, CAMERA_DUMP_PATH);
	sprintf(tmp_str, "%d", cnt);
	strcat(file_name, tmp_str);
	strcat(file_name, "_afl_statistic.txt");
	ISP_LOGV("file name %s", file_name);

	//fp = fopen(file_name, "wb");
	fp = fopen(file_name, "w+");
	if (NULL == fp) {
		ISP_LOGE("fail to open file: %s \n", file_name);
		return -1;
	}
	//fwrite((void*)ptr, 1, height * sizeof(cmr_s32), fp);
	for (i = 0; i < height; i++) {
		fprintf(fp, "%08x\n", *ptr);
		ptr++;
	}

	if (NULL != fp) {
		fclose(fp);
	}

	return 0;
}

static cmr_int aflctrl_process(struct isp_anti_flicker_cfg *cxt, struct afl_proc_in *in_ptr,
			       struct afl_ctrl_proc_out *out_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	cmr_int ret = 0;
	cmr_s32 thr[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	struct isp_awb_statistic_info *ae_stat_ptr = NULL;
	cmr_u32 cur_flicker = 0;
	cmr_u32 cur_exp_flag = 0;
	cmr_s32 ae_exp_flag = 0;
	cmr_u32 i = 0;
	cmr_int flag = 0;
	cmr_s32 *addr = NULL;
	cmr_u32 normal_50hz_thrd = 0;
	cmr_u32 lowlight_50hz_thrd = 0;
	cmr_u32 normal_60hz_thrd = 0;
	cmr_u32 lowlight_60hz_thrd = 0;
	cmr_int bypass = 0;
	UNUSED(out_ptr);
	struct isp_antiflicker_param *afl_param = NULL;
#ifdef CONFIG_ISP_2_2
	struct isp_pm_param_data param_data;
	struct isp_pm_ioctl_input input = { NULL, 0 };
	struct isp_pm_ioctl_output output = { NULL, 0 };
	memset(&param_data, 0, sizeof(param_data));
#endif
	cmr_s32 algo_width;
	cmr_s32 algo_height;
	void *afl_stat = NULL;

	if (!cxt || !in_ptr) {
		ISP_LOGE("fail to check param is NULL!");
		goto exit;
	}

	afl_stat = malloc(in_ptr->private_len);
	memcpy(afl_stat, in_ptr->private_data, in_ptr->private_len);

	ae_stat_ptr = in_ptr->ae_stat_ptr;
	cur_flicker = in_ptr->cur_flicker;
	cur_exp_flag = in_ptr->cur_exp_flag;
	ae_exp_flag = in_ptr->ae_exp_flag;
	addr = (cmr_s32 *) (cmr_uint) in_ptr->vir_addr;

#ifdef CONFIG_ISP_2_2
	BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_ISP_SETTING, ISP_BLK_ANTI_FLICKER, NULL, 0);
	ret = isp_pm_ioctl(in_ptr->handle_pm, ISP_PM_CMD_GET_SINGLE_SETTING, &input, &output);
	if (ISP_SUCCESS == ret && 1 == output.param_num) {
		afl_param = (struct isp_antiflicker_param *)output.param_data->data_ptr;
		normal_50hz_thrd = afl_param->normal_50hz_thrd;
		lowlight_50hz_thrd = afl_param->lowlight_50hz_thrd;
		normal_60hz_thrd = afl_param->normal_60hz_thrd;
		lowlight_60hz_thrd = afl_param->lowlight_60hz_thrd;
	} else {
		normal_50hz_thrd = 280;
		lowlight_50hz_thrd = 100;
		normal_60hz_thrd = 140;
		lowlight_60hz_thrd = 100;
	}
#else
	if (1 == in_ptr->pm_param_num) {
		afl_param = in_ptr->afl_param_ptr;
		normal_50hz_thrd = afl_param->normal_50hz_thrd;
		lowlight_50hz_thrd = afl_param->lowlight_50hz_thrd;
		normal_60hz_thrd = afl_param->normal_60hz_thrd;
		lowlight_60hz_thrd = afl_param->lowlight_60hz_thrd;
	} else {
		normal_50hz_thrd = 280;
		lowlight_50hz_thrd = 100;
		normal_60hz_thrd = 140;
		lowlight_60hz_thrd = 100;
	}
#endif

	if (cur_exp_flag) {
		if (cur_flicker) {
			ret = _set_afl_thr(thr);
			if (0 == ret) {
				ISP_LOGV("%d %d %d %d %d %d %d %d %d",
					 thr[0], thr[1], thr[2], thr[3], thr[4],
					 thr[5], thr[6], thr[7], thr[8]);
				ISP_LOGV("60Hz setting working");
			} else {
				thr[0] = 200;
				thr[1] = 20;
				thr[2] = 160;
				thr[3] = (ae_exp_flag == 1) ? lowlight_60hz_thrd : normal_60hz_thrd;
				thr[4] = 100;
				thr[5] = 4;
				thr[6] = (cxt->version == 1) ? 50 : 30;
				thr[7] = 20;
				thr[8] = 120;
				ISP_LOGV("60Hz using default threshold");
			}
		} else {
			ret = _set_afl_thr(thr);
			if (0 == ret) {
				ISP_LOGV("%d %d %d %d %d %d %d %d %d",
					 thr[0], thr[1], thr[2], thr[3], thr[4],
					 thr[5], thr[6], thr[7], thr[8]);
				ISP_LOGV("50Hz setting working");
			} else {
				thr[0] = 200;
				thr[1] = 20;
				thr[2] = 160;
				thr[3] = (ae_exp_flag == 1) ? lowlight_50hz_thrd : normal_50hz_thrd;
				thr[4] = 100;
				thr[5] = 4;
				thr[6] = (cxt->version == 1) ? 50 : 30;
				thr[7] = 20;
				thr[8] = 120;
				ISP_LOGV("50Hz using default threshold");
			}
		}

		if (cxt->version) {
			algo_width = 640;
			algo_height = 480;
		} else {
			algo_width = cxt->width;
			algo_height = cxt->height;
		}

#ifdef CONFIG_ISP_2_3
		for (i = 0; i < cxt->frame_num; i++) {
			if (cur_flicker) {
				flag = antiflcker_sw_process_v2p2(algo_width,
								  algo_height, addr, 0, thr[0], thr[1],
								  thr[2], thr[3], thr[4], thr[5], thr[6],
								  thr[7], thr[8],
								  (cmr_s32 *)ae_stat_ptr->r_info,
								  (cmr_s32 *)ae_stat_ptr->g_info,
								  (cmr_s32 *)ae_stat_ptr->b_info);
				ISP_LOGV("flag %ld %s", flag, "60Hz");
			} else {
				flag = antiflcker_sw_process_v2p2(algo_width,
								  algo_height, addr, 1, thr[0], thr[1],
								  thr[2], thr[3], thr[4], thr[5], thr[6],
								  thr[7], thr[8],
								  (cmr_s32 *)ae_stat_ptr->r_info,
								  (cmr_s32 *)ae_stat_ptr->g_info,
								  (cmr_s32 *)ae_stat_ptr->b_info);
				ISP_LOGV("flag %ld %s", flag, "50Hz");
			}
			if (flag)
				break;

			if (cxt->version)
				addr += algo_height;
			else
				addr += cxt->vheight;
		}
#else
		for (i = 0; i < cxt->frame_num; i++) {
			if (cur_flicker) {
				flag = antiflcker_sw_process(algo_width,
							     algo_height, addr, 0, thr[0], thr[1],
							     thr[2], thr[3], thr[4], thr[5], thr[6],
							     thr[7], thr[8],
							     (cmr_s32 *)ae_stat_ptr->r_info,
							     (cmr_s32 *)ae_stat_ptr->g_info,
							     (cmr_s32 *)ae_stat_ptr->b_info);
				ISP_LOGV("flag %ld %s", flag, "60Hz");
			} else {
				flag = antiflcker_sw_process(algo_width,
							     algo_height, addr, 1, thr[0], thr[1],
							     thr[2], thr[3], thr[4], thr[5], thr[6],
							     thr[7], thr[8],
							     (cmr_s32 *)ae_stat_ptr->r_info,
							     (cmr_s32 *)ae_stat_ptr->g_info,
							     (cmr_s32 *)ae_stat_ptr->b_info);
				ISP_LOGV("flag %ld %s", flag, "50Hz");
			}
			if (flag)
				break;

			if (cxt->version)
				addr += algo_height;
			else
				addr += cxt->vheight;
		}
#endif
	}

	pthread_mutex_lock(&cxt->status_lock);

	cxt->flag = flag;
	cxt->cur_flicker = cur_flicker;

	pthread_mutex_unlock(&cxt->status_lock);

	if (cxt->afl_set_cb) {
		cxt->afl_set_cb(cxt->caller_handle, ISP_AFL_SET_STATS_BUFFER, afl_stat, NULL);
	}

	if (in_ptr->afl_mode > AE_FLICKER_60HZ)
		bypass = 0;
	else
		bypass = 1;

	if (cxt->afl_set_cb) {
		if (cxt->version)
			cxt->afl_set_cb(cxt->caller_handle, ISP_AFL_NEW_SET_BYPASS, &bypass, NULL);
		else
			cxt->afl_set_cb(cxt->caller_handle, ISP_AFL_SET_BYPASS, &bypass, NULL);
	}

exit:
	if (afl_stat)
		free(afl_stat);

	ISP_LOGV("done %ld", rtn);
	return rtn;
}

static cmr_int aflctrl_ctrl_thr_proc(struct cmr_msg *message, void *p_data)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_anti_flicker_cfg *cxt = (struct isp_anti_flicker_cfg *)p_data;

	if (!message || !p_data) {
		ISP_LOGE("fail to check param");
		goto exit;
	}
	ISP_LOGV("message.msg_type 0x%x, data %p", message->msg_type, message->data);

	switch (message->msg_type) {
	case AFLCTRL_EVT_PROCESS:
		rtn = aflctrl_process(cxt, (struct afl_proc_in *)message->data, &cxt->proc_out);
		break;
	case AFLCTRL_EVT_DEINIT:
		ISP_LOGV("msg done");
		break;
	default:
		ISP_LOGE("fail to proc, don't support msg 0x%x", message->msg_type);
		break;
	}

exit:
	ISP_LOGV("done %ld", rtn);
	return rtn;
}

static cmr_int aflctrl_create_thread(struct isp_anti_flicker_cfg *cxt)
{
	cmr_int rtn = ISP_SUCCESS;

	rtn = cmr_thread_create(&cxt->thr_handle, ISP_THREAD_QUEUE_NUM, aflctrl_ctrl_thr_proc, (void *)cxt);
	if (rtn) {
		ISP_LOGE("fail to create ctrl thread ");
		rtn = -ISP_ERROR;
		goto exit;
	}
	rtn = cmr_thread_set_name(cxt->thr_handle, "aflctrl");
	if (CMR_MSG_SUCCESS != rtn) {
		ISP_LOGE("fail to set aflctrl name");
		rtn = CMR_MSG_SUCCESS;
	}
exit:
	ISP_LOGI("afl_ctrl thread rtn %ld", rtn);
	return rtn;
}

cmr_int afl_ctrl_init(cmr_handle * isp_afl_handle, struct afl_ctrl_init_in * input_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_anti_flicker_cfg *cxt = NULL;

	if (!input_ptr || !isp_afl_handle) {
		rtn = ISP_PARAM_NULL;
		goto exit;
	}
	*isp_afl_handle = NULL;
	rtn = antiflcker_sw_init();
	if (rtn) {
		ISP_LOGE("fail to do antiflcker_sw_init");
		return ISP_ERROR;
	}

	cxt = (struct isp_anti_flicker_cfg *)malloc(sizeof(struct isp_anti_flicker_cfg));
	if (NULL == cxt) {
		ISP_LOGE("fail to do:malloc");
		return ISP_ERROR;
	}
	memset((void *)cxt, 0x00, sizeof(*cxt));
	pthread_mutex_init(&cxt->status_lock, NULL);

	cxt->bypass = 0;
	cxt->skip_frame_num = 1;
	cxt->mode = 0;
	cxt->line_step = 0;
	cxt->frame_num = 3;	//1~15
	cxt->vheight = input_ptr->size.h;
	cxt->start_col = 0;
	cxt->end_col = input_ptr->size.w - 1;
	cxt->vir_addr = (cmr_int) input_ptr->vir_addr;
	cxt->afl_set_cb = input_ptr->afl_set_cb;
	cxt->caller_handle = input_ptr->caller_handle;
	cxt->version = input_ptr->version;
	rtn = aflctrl_create_thread(cxt);
exit:
	if (rtn) {
		if (cxt) {
			free((void *)cxt);
			cxt = NULL;
		}
	} else {
		*isp_afl_handle = (void *)cxt;
	}
	ISP_LOGI("done %ld", rtn);
	return rtn;
}

cmr_int afl_ctrl_cfg(isp_handle isp_afl_handle)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_anti_flicker_cfg *cxt = (struct isp_anti_flicker_cfg *)isp_afl_handle;;
	struct isp_dev_anti_flicker_info afl_info;
	cxt->vheight = cxt->height;
	cxt->start_col = 0;
	cxt->end_col = cxt->width - 1;

	afl_info.bypass = 0;
	afl_info.skip_frame_num = 0;
	afl_info.mode = 0;
	afl_info.line_step = 0;
	afl_info.frame_num = 3;
	afl_info.start_col = 0;
	afl_info.end_col = cxt->width - 1;
	afl_info.vheight = cxt->height;
	afl_info.img_size.height = cxt->height;
	afl_info.img_size.width = cxt->width;

	if (cxt->afl_set_cb) {
		cxt->afl_set_cb(cxt->caller_handle, ISP_AFL_SET_CFG_PARAM, &afl_info, NULL);
	}
	ISP_LOGI("done %ld", rtn);
	return rtn;
}

cmr_int aflnew_ctrl_cfg(isp_handle isp_afl_handle)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_anti_flicker_cfg *cxt = (struct isp_anti_flicker_cfg *)isp_afl_handle;
	struct isp_dev_anti_flicker_new_info afl_info_v3;

	cxt->vheight = cxt->height;
	cxt->start_col = 0;
	cxt->end_col = cxt->width;

	afl_info_v3.bypass = cxt->bypass;
	afl_info_v3.mode = cxt->mode;
	afl_info_v3.skip_frame_num = cxt->skip_frame_num;
	afl_info_v3.afl_stepx = (cmr_u64) cxt->width * 0x100000 / 640;
	afl_info_v3.afl_stepy = (cmr_u64) cxt->height * 0x100000 / 480;
	afl_info_v3.frame_num = cxt->frame_num;
	afl_info_v3.start_col = cxt->start_col;
	afl_info_v3.end_col = cxt->end_col;
	afl_info_v3.step_x_region = (cmr_u64) cxt->width * 0x100000 / 640;
	afl_info_v3.step_y_region = (cmr_u64) cxt->height * 0x100000 / 480;
	afl_info_v3.step_x_start_region = 0;
	afl_info_v3.step_x_end_region = cxt->width;

	afl_info_v3.img_size.width = cxt->width;
	afl_info_v3.img_size.height = cxt->height;

	if (cxt->afl_set_cb) {
		cxt->afl_set_cb(cxt->caller_handle, ISP_AFL_NEW_SET_CFG_PARAM, &afl_info_v3, NULL);
	}

	ISP_LOGI("done:stepx: 0x%x, y: 0x%x, x_region %x, y 0x%x, x_start_region 0x%x, x_end 0x%x",
		 afl_info_v3.afl_stepx, afl_info_v3.afl_stepy,
		 afl_info_v3.step_x_region, afl_info_v3.step_y_region,
		 afl_info_v3.step_x_start_region, afl_info_v3.step_x_end_region);
	return rtn;
}

static cmr_int aflctrl_destroy_thread(struct isp_anti_flicker_cfg *cxt)
{
	cmr_int rtn = ISP_SUCCESS;

	if (!cxt) {
		ISP_LOGE("fail to check param, in parm is NULL");
		rtn = ISP_ERROR;
		goto exit;
	}

	if (cxt->thr_handle) {
		rtn = cmr_thread_destroy(cxt->thr_handle);
		if (!rtn) {
			cxt->thr_handle = NULL;
		} else {
			ISP_LOGE("fail to destroy ctrl thread %ld", rtn);
		}
	}
exit:
	ISP_LOGI("done %ld", rtn);
	return rtn;
}

cmr_int afl_ctrl_deinit(cmr_handle * isp_afl_handle)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_anti_flicker_cfg *cxt = *isp_afl_handle;
	cmr_int bypass = 1;
	CMR_MSG_INIT(message);

	ISP_CHECK_HANDLE_VALID(isp_afl_handle);
	if (NULL == cxt) {
		ISP_LOGE("fail to check param, in parm is NULL");
		rtn = ISP_ERROR;
		return rtn;
	}

	message.msg_type = AFLCTRL_EVT_DEINIT;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	message.alloc_flag = 0;
	message.data = NULL;
	rtn = cmr_thread_msg_send(cxt->thr_handle, &message);
	if (rtn) {
		ISP_LOGE("fail to send msg to main thr %ld", rtn);
		goto exit;
	}

	if (cxt->afl_set_cb) {
		if (cxt->version)
			cxt->afl_set_cb(cxt->caller_handle, ISP_AFL_NEW_SET_BYPASS, &bypass, NULL);
		else
			cxt->afl_set_cb(cxt->caller_handle, ISP_AFL_SET_BYPASS, &bypass, NULL);
	}

	rtn = aflctrl_destroy_thread(cxt);
	if (rtn) {
		ISP_LOGE("fail to destroy aflctrl thread.");
		rtn = antiflcker_sw_deinit();
		if (rtn)
			ISP_LOGE("fail to do antiflcker deinit.");
		goto exit;
	}
	pthread_mutex_destroy(&cxt->status_lock);

	rtn = antiflcker_sw_deinit();
	if (rtn) {
		ISP_LOGE("fail to do antiflcker deinit.");
		return ISP_ERROR;
	}

exit:
	if (NULL != cxt) {
		free((void *)cxt);
		*isp_afl_handle = NULL;
	}

	ISP_LOGI("done %ld", rtn);
	return rtn;
}

static cmr_u32 aflctrl_get_info(struct isp_anti_flicker_cfg *cxt, void *result)
{
	cmr_u32 rtn = ISP_SUCCESS;
	struct afl_ctrl_proc_out *param = (struct afl_ctrl_proc_out *)result;

	param->cur_flicker = cxt->cur_flicker;
	param->flag = cxt->flag;

	return rtn;
}

static cmr_u32 aflctrl_set_img_size(cmr_handle handle, void *in)
{
	cmr_u32 rtn = ISP_SUCCESS;
	struct isp_anti_flicker_cfg *cxt = (struct isp_anti_flicker_cfg *)handle;
	struct isp_size *size = (struct isp_size *)in;

	cxt->width = size->w;
	cxt->height = size->h;

	return rtn;
}

cmr_int afl_ctrl_ioctrl(cmr_handle handle, enum afl_io_ctrl_cmd cmd, void *in_ptr, void *out_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_anti_flicker_cfg *cxt = (struct isp_anti_flicker_cfg *)handle;
	UNUSED(out_ptr);

	rtn = afl_check_handle(handle);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to check handle");
		return ISP_ERROR;
	}

	pthread_mutex_lock(&cxt->status_lock);

	switch (cmd) {
	case AFL_GET_INFO:
		rtn = aflctrl_get_info(cxt, in_ptr);
		break;
	case AFL_SET_BYPASS:
		if (cxt->afl_set_cb) {
			if (cxt->version) {
				cxt->afl_set_cb(cxt->caller_handle, ISP_AFL_NEW_SET_BYPASS, in_ptr, NULL);
			} else {
				cxt->afl_set_cb(cxt->caller_handle, ISP_AFL_SET_BYPASS, in_ptr, NULL);
			}
		}
		break;
	case AFL_NEW_SET_BYPASS:
		if (cxt->afl_set_cb) {
			cxt->afl_set_cb(cxt->caller_handle, ISP_AFL_NEW_SET_BYPASS, in_ptr, NULL);
		}
		break;
	case AFL_SET_IMG_SIZE:
		rtn = aflctrl_set_img_size(cxt, in_ptr);
		break;
	default:
		ISP_LOGE("fail to get invalid cmd %d", cmd);
		rtn = ISP_ERROR;
		break;
	}
	pthread_mutex_unlock(&cxt->status_lock);

	return rtn;
}

cmr_int afl_ctrl_process(cmr_handle isp_afl_handle, struct afl_proc_in *in_ptr, struct afl_ctrl_proc_out *out_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct isp_anti_flicker_cfg *cxt = (struct isp_anti_flicker_cfg *)isp_afl_handle;

	UNUSED(out_ptr);
	if (!isp_afl_handle || !in_ptr) {
		rtn = ISP_PARAM_ERROR;
		goto exit;
	}
	ISP_LOGV("begin %ld", rtn);

	CMR_MSG_INIT(message);
	message.data = malloc(sizeof(*in_ptr));
	if (!message.data) {
		ISP_LOGE("fail to malloc msg");
		rtn = ISP_ALLOC_ERROR;
		goto exit;
	}
	memcpy(message.data, in_ptr, sizeof(*in_ptr));
	message.alloc_flag = 1;

	message.msg_type = AFLCTRL_EVT_PROCESS;
	message.sync_flag = CMR_MSG_SYNC_NONE;
	rtn = cmr_thread_msg_send(cxt->thr_handle, &message);

exit:
	ISP_LOGV("done %ld", rtn);
	return rtn;
}
