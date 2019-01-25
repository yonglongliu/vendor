/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "awb_adpt"
#define ATRACE_TAG (ATRACE_TAG_CAMERA | ATRACE_TAG_HAL)
#include <cutils/trace.h>

#include "awb_ctrl.h"
#include "awb_sprd_adpt.h"
#include "awb.h"
#include "isp_adpt.h"
#include "dlfcn.h"
#include <cutils/properties.h>

#include "isp_awb_queue.h"
#include "isp_debug.h"

#include <utils/Timers.h>

#define AWB_CTRL_MAGIC_BEGIN		0xe5a55e5a
#define AWB_CTRL_MAGIC_END		0x5e5ae5a5
#define AWB_CTRL_RESOLUTION_NUM 	8
#define AWB_CTRL_SCENEMODE_NUM	10

#define AWB_GAIN_PARAM_FILE_NAME_CAMERASERVER "/data/misc/cameraserver/awb.file"
#define AWB_GAIN_PARAM_FILE_NAME_MEDIA "/data/misc/media/awb.file"

#define AWB_CTRL_TRUE			1
#define AWB_CTRL_FALSE			0
#define AWB_CTRL_LOCKMODE 1
#define AWB_CTRL_UNLOCKMODE 0
#define AWB_CTRL_SAFE_FREE(_p) \
do { \
	if (NULL != (_p)) {\
		free(_p); \
		_p = NULL; \
	} \
}while(0)

#define __MIN(a,b) (((a)<(b))?(a):(b))

char libawb_path[][20] = {
	"libawb.so",
	"libawb1.so",
	"libawb_v2.so",
	"libawb_v3.so",
	"libawb_v4.so",
	"libawb_v5.so",
};

struct awbsprd_lib_ops {
	void *(*awb_init_v1) (struct awb_init_param * init_param, struct awb_rgb_gain * gain);
	 cmr_s32(*awb_calc_v1) (void *awb_handle, struct awb_calc_param * calc_param, struct awb_calc_result * calc_result);
	 cmr_s32(*awb_ioctrl_v1) (void *awb_handle, cmr_s32 cmd, void *param);
	 cmr_s32(*awb_deinit_v1) (void *awb_handle);
	 cmr_s32(*awb_sync_gain) (struct awb_sync_info * sync_info, cmr_u32 gain_r_master, cmr_u32 gain_g_master, cmr_u32 gain_b_master,
							  cmr_u32 * gain_r_slave, cmr_u32 * gain_g_slave, cmr_u32 * gain_b_slave);
};

struct awb_gain_queue {
	void *ct;
	void *r_gain;
	void *g_gain;
	void *b_gain;
	void *weight;

	cmr_u32 size;
};

struct awb_ctrl_cxt {
	/*must be the first one */
	cmr_u32 magic_begin;
	/*awb status lock */
	pthread_mutex_t status_lock;
	/*initialize parameter */
	struct awb_ctrl_init_param init_param;
	struct awb_init_param awb_init_param;
	/*camera id */
	cmr_u32 camera_id;			/* 0: back camera, 1: front camera */
	/*work mode */
	cmr_u32 work_mode;			/* 0: preview, 1:capture, 2:video */
	cmr_u32 param_index;		/* tuning param index */
	cmr_u32 last_enable;		/* record nonzsl video stop */
	cmr_u32 snap_lock;			/* record lock awb frames after snapshot    */
	/*white balance mode: auto or manual */
	enum awb_ctrl_wb_mode wb_mode;
	/*scene mode */
	enum awb_ctrl_scene_mode scene_mode;
	/*format of statistic image */
	enum awb_ctrl_stat_img_format stat_img_format;
	/*statistic image size */
	struct awb_ctrl_size stat_img_size;
	struct awb_ctrl_size stat_win_size;

	cmr_u32 flash_update_awb;
	struct awb_ctrl_opt_info otp_info;
	/*flash info */
	struct awb_flash_info flash_info;
	/* ctrl touch flash awb */
	cmr_u32 flash_pre_state;
	/*current gain */
	struct awb_ctrl_gain cur_gain;
	/*output gain */
	struct awb_ctrl_gain output_gain;
	/*output ct */
	cmr_u32 output_ct;
	/*recover gain */
	struct awb_ctrl_gain recover_gain;
	/*recover ct */
	cmr_u32 recover_ct;
	/*recover awb mode */
	enum awb_ctrl_wb_mode recover_mode;
	/*awb lock info */
	struct awb_ctrl_lock_info lock_info;
	/*current ct */
	cmr_u32 cur_ct;
	/*algorithm handle */
	void *alg_handle;
	void *lib_handle;
	struct awbsprd_lib_ops lib_ops;
	struct third_lib_info *lib_info;
	struct awb_gain_queue gain_queue;
	cmr_u8 *log;
	cmr_u32 size;
	cmr_u32 frame_count;

	/*
	 * for dual camera sync
	 */
	cmr_u8 sensor_role;
	cmr_u32 is_multi_mode;
	func_isp_br_ioctrl ptr_isp_br_ioctrl;

	struct awb_ctrl_calc_result awb_result;

	/*must be the last one */
	cmr_u32 magic_end;
};

static cmr_u32 _awb_get_gain(struct awb_ctrl_cxt *cxt, void *param);

static void _deinit_gain_queue(struct awb_gain_queue *queue)
{
	if (0 != queue->ct) {
		queue_deinit(queue->ct);
		queue->ct = 0;
	}

	if (0 != queue->r_gain) {
		queue_deinit(queue->r_gain);
		queue->r_gain = 0;
	}

	if (0 != queue->g_gain) {
		queue_deinit(queue->g_gain);
		queue->g_gain = 0;
	}

	if (0 != queue->b_gain) {
		queue_deinit(queue->b_gain);
		queue->b_gain = 0;
	}

	if (0 != queue->weight) {
		queue_deinit(queue->weight);
		queue->weight = 0;
	}
}

static void _gain_queue_add(struct awb_gain_queue *queue, struct awb_ctrl_gain *gain, cmr_u16 ct, cmr_u32 weight)
{
	if (0 != queue->ct)
		queue_add(queue->ct, ct);

	if (0 != queue->r_gain)
		queue_add(queue->r_gain, gain->r);

	if (0 != queue->g_gain)
		queue_add(queue->g_gain, gain->g);

	if (0 != queue->b_gain)
		queue_add(queue->b_gain, gain->b);

	if (0 != queue->weight)
		queue_add(queue->weight, weight);
}

static void _gain_queue_clear(struct awb_gain_queue *queue)
{
	if (0 != queue->ct)
		queue_clear(queue->ct);

	if (0 != queue->r_gain)
		queue_clear(queue->r_gain);

	if (0 != queue->g_gain)
		queue_clear(queue->g_gain);

	if (0 != queue->b_gain)
		queue_clear(queue->b_gain);

	if (0 != queue->weight)
		queue_clear(queue->weight);
}

static cmr_s32 _init_gain_queue(struct awb_gain_queue *queue, cmr_u32 size)
{
	queue->ct = queue_init(size);
	if (0 == queue->ct)
		goto ERROR_EXIT;

	queue->r_gain = queue_init(size);
	if (0 == queue->r_gain)
		goto ERROR_EXIT;

	queue->g_gain = queue_init(size);
	if (0 == queue->g_gain)
		goto ERROR_EXIT;

	queue->b_gain = queue_init(size);
	if (0 == queue->b_gain)
		goto ERROR_EXIT;

	queue->weight = queue_init(size);
	if (0 == queue->weight)
		goto ERROR_EXIT;

	return AWB_SUCCESS;

  ERROR_EXIT:
	_deinit_gain_queue(queue);
	return AWB_ERROR;
}

static void _gain_queue_average(struct awb_gain_queue *queue, struct awb_ctrl_gain *gain, cmr_u32 * ct)
{
	if (0 == queue->weight)
		return;

	if (0 != queue->ct)
		*ct = queue_weighted_average(queue->ct, queue->weight);

	if (0 != queue->r_gain)
		gain->r = queue_weighted_average(queue->r_gain, queue->weight);

	if (0 != queue->g_gain)
		gain->g = queue_weighted_average(queue->g_gain, queue->weight);

	if (0 != queue->b_gain)
		gain->b = queue_weighted_average(queue->b_gain, queue->weight);
}

static cmr_u32 _check_handle(awb_ctrl_handle_t handle)
{
	cmr_u32 rtn = AWB_CTRL_SUCCESS;
	struct awb_ctrl_cxt *cxt = (struct awb_ctrl_cxt *)handle;

	if (NULL == cxt) {
		ISP_LOGE("fail to get awb cxt");
		return AWB_CTRL_ERROR;
	}

	if (AWB_CTRL_MAGIC_BEGIN != cxt->magic_begin || AWB_CTRL_MAGIC_END != cxt->magic_end) {
		ISP_LOGE("fail to get invalid magic, begin = 0x%x, end = 0x%x", cxt->magic_begin, cxt->magic_end);
		return AWB_CTRL_ERROR;
	}

	return rtn;
}

static cmr_u32 _check_init_param(struct awb_ctrl_init_param *param)
{
	cmr_u32 rtn = AWB_CTRL_SUCCESS;
	UNUSED(param);
	return rtn;
}

struct awb_save_gain s_save_awb_param[4] = { {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} };

static void _awb_save_gain(struct awb_save_gain *cxt, cmr_u32 num)
{
	cmr_u32 i = 0;
	FILE *fp = NULL;
	char version[1024];
	property_get("ro.build.version.release", version, "");

	if (version[0] > '6') {
		fp = fopen(AWB_GAIN_PARAM_FILE_NAME_CAMERASERVER, "wb");
		if (fp) {
			for (i = 0; i < num; ++i) {
				ISP_LOGV("[%d]: %d, %d, %d, %d\n", i, cxt[i].r, cxt[i].g, cxt[i].b, cxt[i].ct);
			}
			fwrite((char *)cxt, 1, num * sizeof(struct awb_save_gain), fp);
			fclose(fp);
			fp = NULL;
		}
	} else {
		fp = fopen(AWB_GAIN_PARAM_FILE_NAME_MEDIA, "wb");
		if (fp) {
			for (i = 0; i < num; ++i) {
				ISP_LOGV("[%d]: %d, %d, %d, %d\n", i, cxt[i].r, cxt[i].g, cxt[i].b, cxt[i].ct);
			}
			fwrite((char *)cxt, 1, num * sizeof(struct awb_save_gain), fp);
			fclose(fp);
			fp = NULL;
		}
	}
}

static void _awb_read_gain(struct awb_save_gain *cxt, cmr_u32 num)
{
	cmr_u32 i = 0;
	FILE *fp = NULL;
	char version[1024];
	property_get("ro.build.version.release", version, "");

	if (version[0] > '6') {
		fp = fopen(AWB_GAIN_PARAM_FILE_NAME_CAMERASERVER, "rb");
		if (fp) {
			memset((void *)cxt, 0, sizeof(struct awb_save_gain) * num);
			fread((char *)cxt, 1, num * sizeof(struct awb_save_gain), fp);
			fclose(fp);
			fp = NULL;

			for (i = 0; i < num; ++i) {
				ISP_LOGV("[%d]: %d, %d, %d, %d\n", i, cxt[i].r, cxt[i].g, cxt[i].b, cxt[i].ct);
			}
		}
	} else {
		fp = fopen(AWB_GAIN_PARAM_FILE_NAME_MEDIA, "rb");
		if (fp) {
			memset((void *)cxt, 0, sizeof(struct awb_save_gain) * num);
			fread((char *)cxt, 1, num * sizeof(struct awb_save_gain), fp);
			fclose(fp);
			fp = NULL;

			for (i = 0; i < num; ++i) {
				ISP_LOGV("[%d]: %d, %d, %d, %d\n", i, cxt[i].r, cxt[i].g, cxt[i].b, cxt[i].ct);
			}
		}
	}
}

static cmr_u32 _awb_set_wbmode(struct awb_ctrl_cxt *cxt, void *in_param)
{
	cmr_u32 rtn = AWB_CTRL_SUCCESS;
	cmr_u32 awb_mode = *(cmr_u32 *) in_param;

	cxt->wb_mode = awb_mode;

	if (cxt->wb_mode == 0 && cxt->last_enable == 0) {
		cxt->recover_gain.r = cxt->cur_gain.r;
		cxt->recover_gain.g = cxt->cur_gain.g;
		cxt->recover_gain.b = cxt->cur_gain.b;
		cxt->recover_ct = cxt->cur_ct;
		cxt->recover_mode = cxt->wb_mode;
	}
	ISP_LOGV("debug wbmode changed!");

	return rtn;
}

static cmr_u32 _awb_get_wbmode(struct awb_ctrl_cxt *cxt, void *out_param)
{
	cmr_u32 rtn = AWB_CTRL_SUCCESS;

	*(cmr_u32 *) out_param = cxt->wb_mode;
	ISP_LOGV("AWB get mode = %d", *(cmr_u32 *) out_param);
	return rtn;
}

static cmr_u32 _awb_set_workmode(struct awb_ctrl_cxt *cxt, void *in_param)
{
	cmr_u32 rtn = AWB_CTRL_SUCCESS;
	cmr_u32 work_mode = *(cmr_u32 *) in_param;

	cxt->work_mode = work_mode;
	return rtn;
}

static cmr_u32 _awb_set_recgain(struct awb_ctrl_cxt *cxt, void *param)
{
	UNUSED(param);
	cmr_u32 rtn = AWB_CTRL_SUCCESS;
	struct awb_gain awb_gain = { 0x0, 0x0, 0x0 };

	rtn = _awb_get_gain(cxt, (void *)&awb_gain);

	cxt->recover_gain.r = awb_gain.r;
	cxt->recover_gain.g = awb_gain.g;
	cxt->recover_gain.b = awb_gain.b;

	cxt->recover_mode = cxt->wb_mode;
	cxt->recover_ct = cxt->cur_ct;

	ISP_LOGV("pre flashing mode = %d", cxt->flash_info.flash_mode);

	ISP_LOGV("FLASH_TAG: awb flash recover gain = (%d, %d, %d), recover mode = %d", cxt->recover_gain.r, cxt->recover_gain.g, cxt->recover_gain.b, cxt->recover_mode);

	if (cxt->wb_mode == 0 && cxt->last_enable == 0 && cxt->flash_info.flash_enable == 0) {
		s_save_awb_param[cxt->camera_id].r = awb_gain.r;
		s_save_awb_param[cxt->camera_id].g = awb_gain.g;
		s_save_awb_param[cxt->camera_id].b = awb_gain.b;
		s_save_awb_param[cxt->camera_id].ct = cxt->cur_ct;
		_awb_save_gain(&s_save_awb_param[0], sizeof(s_save_awb_param) / sizeof(struct awb_save_gain));
	}
	return rtn;

}

static cmr_u32 _awb_get_gain(struct awb_ctrl_cxt *cxt, void *param)
{
	cmr_u32 rtn = AWB_CTRL_SUCCESS;
	struct awb_gain *awb_result = (struct awb_gain *)param;

	awb_result->r = cxt->output_gain.r;
	awb_result->g = cxt->output_gain.g;
	awb_result->b = cxt->output_gain.b;

	ISP_LOGV("_awb_get_gain = (%d,%d,%d)", awb_result->r, awb_result->g, awb_result->b);

	return rtn;

}

static cmr_u32 _awb_get_result_info(struct awb_ctrl_cxt *cxt, void *param)
{
	cmr_u32 rtn = AWB_CTRL_SUCCESS;
	struct awb_ctrl_calc_result *awb_result = (struct awb_ctrl_calc_result *)param;

	memcpy(awb_result, &cxt->awb_result, sizeof(struct awb_ctrl_calc_result));

	return rtn;

}

static cmr_u32 _awb_get_cur_gain(struct awb_ctrl_cxt *cxt, void *param)
{
	cmr_u32 rtn = AWB_CTRL_SUCCESS;
	struct awb_gain *awb_result = (struct awb_gain *)param;

	awb_result->r = cxt->cur_gain.r;
	awb_result->g = cxt->cur_gain.g;
	awb_result->b = cxt->cur_gain.b;

	ISP_LOGV("_awb_get_cur_gain = (%d,%d,%d)", awb_result->r, awb_result->g, awb_result->b);

	return rtn;

}

static cmr_u32 _awb_get_pix_cnt(struct awb_ctrl_cxt *cxt, void *param)
{
	cmr_u32 rtn = AWB_CTRL_SUCCESS;
	struct isp_size *in_ptr = (struct isp_size *)param;

	cxt->init_param.stat_win_size.w = in_ptr->w / 2 / 32 * 2;
	cxt->init_param.stat_win_size.h = in_ptr->h / 2 / 32 * 2;

	return rtn;
}

static cmr_u32 _awb_get_stat_size(struct awb_ctrl_cxt *cxt, void *param)
{
	cmr_u32 rtn = AWB_CTRL_SUCCESS;
	struct awb_size *stat_size = (struct awb_size *)param;

	stat_size->w = cxt->init_param.stat_img_size.w;
	stat_size->h = cxt->init_param.stat_img_size.h;

	ISP_LOGV("_awb_get_stat_size = (%d,%d)", stat_size->w, stat_size->h);

	return rtn;
}

static cmr_u32 _awb_get_ct(struct awb_ctrl_cxt *cxt, void *param)
{
	cmr_u32 rtn = AWB_CTRL_SUCCESS;
	cmr_u32 *ct = (cmr_u32 *) param;

	*ct = cxt->output_ct;

	ISP_LOGV("_awb_get_ct = %d", cxt->output_ct);

	return rtn;
}

static cmr_u32 _awb_get_recgain(struct awb_ctrl_cxt *cxt, void *param)
{
	UNUSED(param);
	cmr_u32 rtn = AWB_CTRL_SUCCESS;
	struct awb_ctrl_gain awb_gain = { 0x0, 0x0, 0x0 };

	awb_gain.r = cxt->recover_gain.r;
	awb_gain.g = cxt->recover_gain.g;
	awb_gain.b = cxt->recover_gain.b;

	cxt->output_gain.r = cxt->recover_gain.r;
	cxt->output_gain.g = cxt->recover_gain.g;
	cxt->output_gain.b = cxt->recover_gain.b;
	cxt->output_ct = cxt->recover_ct;

	cxt->cur_gain.r = awb_gain.r;
	cxt->cur_gain.g = awb_gain.g;
	cxt->cur_gain.b = awb_gain.b;
	cxt->cur_ct = cxt->recover_ct;
	cxt->wb_mode = cxt->recover_mode;

	ISP_LOGV("after flashing mode = %d", cxt->flash_info.flash_mode);

	ISP_LOGV("FLASH_TAG: awb flash end  gain = (%d, %d, %d), recover mode = %d", cxt->cur_gain.r, cxt->cur_gain.g, cxt->cur_gain.b, cxt->wb_mode);

	return rtn;

}

static cmr_u32 _awb_set_lock(struct awb_ctrl_cxt *cxt, void *param)
#if 1
{
	UNUSED(param);
	cmr_u32 rtn = AWB_CTRL_SUCCESS;

	cxt->lock_info.lock_num += 1;

	ISP_LOGV("AWB_TEST _awb_set_lock0: luck=%d, mode=%d", cxt->lock_info.lock_num, cxt->lock_info.lock_mode);

	if (0 != cxt->lock_info.lock_num) {

		cxt->lock_info.lock_mode = AWB_CTRL_LOCKMODE;

		cxt->lock_info.lock_gain.r = cxt->output_gain.r;
		cxt->lock_info.lock_gain.g = cxt->output_gain.g;
		cxt->lock_info.lock_gain.b = cxt->output_gain.b;

		cxt->lock_info.lock_ct = cxt->output_ct;
	}
	ISP_LOGV("AWB_TEST _awb_set_lock1: luck=%d, mode:%d   rgb[%d %d %d]", cxt->lock_info.lock_num, cxt->lock_info.lock_mode, cxt->output_gain.r, cxt->output_gain.g, cxt->output_gain.b);
	return rtn;

}

#else
{
	cmr_u32 rtn = AWB_CTRL_SUCCESS;

	cxt->lock_info.lock_num += 1;

	ISP_LOGV("AWB_TEST _awb_set_lock0: luck=%d, unluck=%", cxt->lock_info.lock_num, cxt->lock_info.unlock_num);

	if (0 == cxt->lock_info.lock_gain.r && 0 == cxt->lock_info.lock_gain.g && 0 == cxt->lock_info.lock_gain.b) {

		cxt->lock_info.lock_gain.r = cxt->output_gain.r;
		cxt->lock_info.lock_gain.g = cxt->output_gain.g;
		cxt->lock_info.lock_gain.b = cxt->output_gain.b;

		cxt->lock_info.lock_ct = cxt->output_ct;
	}
	ISP_LOGV("AWB_TEST _awb_set_lock1: luck=%d, unluck=%, mode:%d", cxt->lock_info.lock_num, cxt->lock_info.unlock_num, cxt->lock_info.lock_mode);
	return rtn;

}
#endif

static cmr_u32 _awb_get_unlock(struct awb_ctrl_cxt *cxt, void *param)
#if 1
{
	UNUSED(param);
	cmr_u32 rtn = AWB_CTRL_SUCCESS;
	ISP_LOGV("AWB_TEST _awb_get_unlock0: lock_num=%d, mode:=%d", cxt->lock_info.lock_num, cxt->lock_info.lock_mode);
	if (0 != cxt->lock_info.lock_num) {
		cxt->lock_info.lock_num -= 1;
	}

	if (0 == cxt->lock_info.lock_num) {
		cxt->lock_info.lock_mode = AWB_CTRL_UNLOCKMODE;
	}

	ISP_LOGV("AWB_TEST _awb_get_unlock1: lock_num=%d, mode:=%d", cxt->lock_info.lock_num, cxt->lock_info.lock_mode);

	return rtn;
}
#else
{
	cmr_u32 rtn = AWB_CTRL_SUCCESS;

	cxt->lock_info.unlock_num += 1;
	if (0 == cxt->lock_info.lock_num)
		rtn = AWB_CTRL_ERROR;

	ISP_LOGV("AWB_TEST _awb_get_unlock0: luck=%d, unluck=%d", cxt->lock_info.lock_num, cxt->lock_info.unlock_num);

	if (0 != cxt->lock_info.lock_num && cxt->lock_info.lock_num != cxt->lock_info.unlock_num)
		cxt->lock_info.lock_mode = AWB_CTRL_LOCKMODE;

	ISP_LOGV("AWB_TEST _awb_get_unlock1: luck=%d, unluck=%, mode:=%d", cxt->lock_info.lock_num, cxt->lock_info.unlock_num, cxt->lock_info.lock_mode);

	return rtn;

}
#endif

static cmr_u32 _awb_flash_snopshot_recovery(struct awb_ctrl_cxt *cxt, void *param)
{
	UNUSED(param);
	cmr_u32 rtn = AWB_CTRL_SUCCESS;

	cxt->flash_info.main_flash_enable = 1;
	cxt->lock_info.lock_flash_frame = 10;

	return rtn;
}

static cmr_u32 _awb_set_flash_status(struct awb_ctrl_cxt *cxt, void *param)
{
	UNUSED(param);
	cmr_u32 rtn = AWB_CTRL_SUCCESS;
	enum awb_ctrl_flash_status *flash_status = (enum awb_ctrl_flash_status *)param;

	cxt->flash_info.flash_status = *flash_status;

	if (cxt->flash_info.flash_status == 4) {
		cxt->flash_update_awb = 0;
	} else {
		cxt->flash_update_awb = 1;
	}

	if (cxt->flash_info.flash_status == 2) {
		cxt->flash_pre_state = 10;
	} else {
		cxt->flash_pre_state = 0;
	}

	ISP_LOGV("flashing status = %d", cxt->flash_info.flash_status);

	return rtn;

}

static cmr_u32 _awb_set_tuning_param(struct awb_ctrl_cxt *cxt, void *param0)
{
	cmr_u32 rtn = AWB_CTRL_SUCCESS;
	UNUSED(cxt);
	rtn = _check_init_param(param0);
	if (AWB_CTRL_SUCCESS != rtn) {
		ISP_LOGE("fail to check init param");
		return AWB_CTRL_ERROR;
	}

	return rtn;
}

static cmr_u32 _awb_get_data_type(struct awb_ctrl_cxt *cxt, void *param)
{
	cmr_u32 rtn = AWB_CTRL_SUCCESS;

	if (NULL == cxt) {
		ISP_LOGE("fail to get awb cxt");
		rtn = AWB_CTRL_ERROR;
		goto exit;
	}

	if (NULL == param) {
		ISP_LOGE("param is null");
		rtn = AWB_CTRL_ERROR;
		goto exit;
	}

	cmr_s32 *data_type = (cmr_s32 *)param;
	*data_type = cxt->awb_init_param.tuning_param.stat_type;

	ISP_LOGV("awb_get_data_type = %d", *data_type);
  exit:
	return rtn;
}

static cmr_u32 awbsprd_unload_lib(struct awb_ctrl_cxt *cxt)
{
	cmr_u32 rtn = AWB_CTRL_SUCCESS;

	if (NULL == cxt) {
		ISP_LOGE("fail to load awb lib param");
		rtn = AWB_CTRL_ERROR;
		goto exit;
	}

	if (cxt->lib_handle) {
		dlclose(cxt->lib_handle);
		cxt->lib_handle = NULL;
	}

  exit:
	return rtn;
}

static cmr_u32 awbsprd_load_lib(struct awb_ctrl_cxt *cxt)
{
	cmr_u32 rtn = AWB_CTRL_SUCCESS;
	cmr_u32 v_count = 0;
	cmr_u32 v_id = 0;

	if (NULL == cxt) {
		ISP_LOGE("fail to load awb lib param");
		rtn = AWB_CTRL_ERROR;
		goto exit;
	}

	v_id = cxt->lib_info->version_id;
	v_count = sizeof(libawb_path) / sizeof(libawb_path[0]);
	if (v_id >= v_count) {
		ISP_LOGE("fail to get awb lib version id,  version_id: %d", v_id);
		rtn = AWB_CTRL_ERROR;
		goto exit;
	}
	ISP_LOGI("awb lib v_count:%d, version_id:%d, version_name:%s", v_count, v_id, libawb_path[v_id]);

	cxt->lib_handle = dlopen(libawb_path[v_id], RTLD_NOW);
	if (!cxt->lib_handle) {
		ISP_LOGE("fail to dlopen awb lib");
		rtn = AWB_CTRL_ERROR;
		goto exit;
	}

	cxt->lib_ops.awb_init_v1 = dlsym(cxt->lib_handle, "awb_init_v1");
	if (!cxt->lib_ops.awb_init_v1) {
		ISP_LOGE("fail to dlsym awb_init");
		rtn = AWB_CTRL_ERROR;
		goto load_error;
	}

	cxt->lib_ops.awb_calc_v1 = dlsym(cxt->lib_handle, "awb_calc_v1");
	if (!cxt->lib_ops.awb_calc_v1) {
		ISP_LOGE("fail to dlsym awb_calculation");
		rtn = AWB_CTRL_ERROR;
		goto load_error;
	}

	cxt->lib_ops.awb_sync_gain = dlsym(cxt->lib_handle, "awb_sync_gain");
	if (!cxt->lib_ops.awb_sync_gain) {
		ISP_LOGE("fail to dlsym awb_sync_gain");
		rtn = AWB_CTRL_ERROR;
		goto load_error;
	}

	cxt->lib_ops.awb_deinit_v1 = dlsym(cxt->lib_handle, "awb_deinit_v1");
	if (!cxt->lib_ops.awb_deinit_v1) {
		ISP_LOGE("fail to dlsym awb_deinit");
		rtn = AWB_CTRL_ERROR;
		goto load_error;
	}

	cxt->lib_ops.awb_ioctrl_v1 = dlsym(cxt->lib_handle, "awb_ioctrl_v1");
	if (!cxt->lib_ops.awb_ioctrl_v1) {
		ISP_LOGE("fail to dlsym awb_ioctrl");
		rtn = AWB_CTRL_ERROR;
		goto load_error;
	}

	return AWB_CTRL_SUCCESS;
  load_error:
	awbsprd_unload_lib(cxt);
  exit:
	return rtn;
}

static cmr_u32 awb_get_debug_info(struct awb_ctrl_cxt *cxt, void *result)
{
	cmr_u32 rtn = AWB_SUCCESS;
	struct debug_awb_param *param = (struct debug_awb_param *)result;

	param->version = AWB_DEBUG_VERSION_ID;
	param->r_gain = cxt->output_gain.r;
	param->g_gain = cxt->output_gain.g;
	param->b_gain = cxt->output_gain.b;
	param->cur_ct = cxt->output_ct;
	param->cur_awb_mode = cxt->wb_mode;
	param->cur_work_mode = cxt->work_mode;
	/* awb calibration of golden sensor */
	param->golden_r = cxt->otp_info.gldn_stat_info.r;
	param->golden_g = cxt->otp_info.gldn_stat_info.g;
	param->golden_b = cxt->otp_info.gldn_stat_info.b;
	/* awb calibration of random sensor */
	param->random_r = cxt->otp_info.rdm_stat_info.r;
	param->random_g = cxt->otp_info.rdm_stat_info.g;
	param->random_b = cxt->otp_info.rdm_stat_info.b;
	return rtn;
}

static cmr_u32 _awb_get_flash_ct_table(struct awb_ctrl_cxt *cxt, void *result)
{
	cmr_u32 rtn = AWB_SUCCESS;
	struct awb_ct_table *param = (struct awb_ct_table *)result;
	rtn = cxt->lib_ops.awb_ioctrl_v1(cxt->alg_handle, AWB_IOCTRL_GET_CTTABLE20, param);
	return rtn;
}

static cmr_u32 awb_get_debug_info_for_display(struct awb_ctrl_cxt *cxt, void *result)
{
	cmr_u32 rtn = AWB_SUCCESS;
	struct debug_awb_display_param *emParam = (struct debug_awb_display_param *)result;
	emParam->version = AWB_DEBUG_VERSION_ID;
	emParam->r_gain = cxt->output_gain.r;
	emParam->g_gain = cxt->output_gain.g;
	emParam->b_gain = cxt->output_gain.b;
	emParam->cur_ct = cxt->output_ct;
	emParam->cur_awb_mode = cxt->wb_mode;
	emParam->cur_work_mode = cxt->work_mode;
	/* awb calibration of golden sensor */
	emParam->golden_r = cxt->otp_info.gldn_stat_info.r;
	emParam->golden_g = cxt->otp_info.gldn_stat_info.g;
	emParam->golden_b = cxt->otp_info.gldn_stat_info.b;
	/* awb calibration of random sensor */
	emParam->random_r = cxt->otp_info.rdm_stat_info.r;
	emParam->random_g = cxt->otp_info.rdm_stat_info.g;
	emParam->random_b = cxt->otp_info.rdm_stat_info.b;
	return rtn;
}

cmr_u32 _awb_parser_otp_info(struct awb_ctrl_init_param * param)
{
	cmr_u32 rtn = AWB_CTRL_SUCCESS;
	cmr_u8 *awb_rdm_otp_data = NULL;
	cmr_u16 awb_rdm_otp_len = 0;
	cmr_u16 *awb_golden_otp_data = NULL;
	cmr_u16 awb_golden_otp_len = 0;
	cmr_u32 otp_map_version = 0;
	cmr_u8 *module_info = NULL;
	cmr_u8 *awb_golden_otp_data_v1 = NULL;
	struct sensor_otp_section_info *awb_otp_info_ptr = NULL;
	struct sensor_otp_section_info *module_info_ptr = NULL;

	if (NULL != param->otp_info_ptr) {
		if (param->otp_info_ptr->otp_vendor == OTP_VENDOR_SINGLE) {
			awb_otp_info_ptr = param->otp_info_ptr->single_otp.iso_awb_info;
			module_info_ptr = param->otp_info_ptr->single_otp.module_info;
			ISP_LOGV("pass awb otp, single cam");
		} else if (param->otp_info_ptr->otp_vendor == OTP_VENDOR_SINGLE_CAM_DUAL || param->otp_info_ptr->otp_vendor == OTP_VENDOR_DUAL_CAM_DUAL) {
			if (param->is_master == 1) {
				awb_otp_info_ptr = param->otp_info_ptr->dual_otp.master_iso_awb_info;
				module_info_ptr = param->otp_info_ptr->dual_otp.master_module_info;
				ISP_LOGV("pass awb otp, dual cam master");
			} else {
				awb_otp_info_ptr = param->otp_info_ptr->dual_otp.slave_iso_awb_info;
				module_info_ptr = param->otp_info_ptr->dual_otp.slave_module_info;
				ISP_LOGV("pass awb otp, dual cam slave");
			}
		} else {
			awb_otp_info_ptr = NULL;
			module_info_ptr = NULL;
			ISP_LOGE("awb otp otp_vendor = %d", param->otp_info_ptr->otp_vendor);
		}
	} else {
		awb_otp_info_ptr = NULL;
		module_info_ptr = NULL;
		ISP_LOGE("awb otp info ptr is NULL");
	}

	if (NULL != awb_otp_info_ptr && NULL != module_info_ptr) {
		module_info = (cmr_u8 *) module_info_ptr->rdm_info.data_addr;

		if (NULL != module_info) {
			if ((module_info[4] == 0 && module_info[5] == 1)
				|| (module_info[4] == 0 && module_info[5] == 2)
				|| (module_info[4] == 0 && module_info[5] == 3)
				|| (module_info[4] == 0 && module_info[5] == 4)
				|| (module_info[4] == 1 && module_info[5] == 0 && (module_info[0] != 0x53 || module_info[1] != 0x50 || module_info[2] != 0x52 || module_info[3] != 0x44))
				|| (module_info[4] == 2 && module_info[5] == 0)
				|| (module_info[4] == 3 && module_info[5] == 0)
				|| (module_info[4] == 4 && module_info[5] == 0)) {
				ISP_LOGV("awb otp map v0.4");
				otp_map_version = 4;
				awb_rdm_otp_data = (cmr_u8 *) awb_otp_info_ptr->rdm_info.data_addr;
				awb_rdm_otp_len = awb_otp_info_ptr->rdm_info.data_size;
				awb_golden_otp_data = (cmr_u16 *) awb_otp_info_ptr->gld_info.data_addr;
				awb_golden_otp_len = awb_otp_info_ptr->gld_info.data_size;
			} else if (module_info[4] == 1 && module_info[5] == 0 && module_info[0] == 0x53 && module_info[1] == 0x50 && module_info[2] == 0x52 && module_info[3] == 0x44) {
				ISP_LOGV("awb otp map v1.0");
				otp_map_version = 1;
				awb_rdm_otp_data = (cmr_u8 *) awb_otp_info_ptr->rdm_info.data_addr + 1;
				awb_rdm_otp_len = awb_otp_info_ptr->rdm_info.data_size;
				awb_golden_otp_data_v1 = (cmr_u8 *) awb_otp_info_ptr->rdm_info.data_addr + 1 + 6;
				awb_golden_otp_len = awb_otp_info_ptr->rdm_info.data_size;
			} else {
				ISP_LOGE("awb otp map version error");
				awb_rdm_otp_data = NULL;
				awb_golden_otp_data = NULL;
				awb_golden_otp_data_v1 = NULL;
			}
		} else {
			ISP_LOGE("awb module_info is NULL");
			awb_rdm_otp_data = NULL;
			awb_golden_otp_data = NULL;
			awb_golden_otp_data_v1 = NULL;
		}

		if (NULL != awb_rdm_otp_data && 0 != awb_rdm_otp_len) {
			param->otp_info.rdm_stat_info.r = (awb_rdm_otp_data[1] << 8) | awb_rdm_otp_data[0];
			param->otp_info.rdm_stat_info.g = (awb_rdm_otp_data[3] << 8) | awb_rdm_otp_data[2];
			param->otp_info.rdm_stat_info.b = (awb_rdm_otp_data[5] << 8) | awb_rdm_otp_data[4];
			ISP_LOGV("awb otp random [%d %d %d ]", param->otp_info.rdm_stat_info.r, param->otp_info.rdm_stat_info.g, param->otp_info.rdm_stat_info.b);
		} else {
			param->otp_info.rdm_stat_info.r = 0;
			param->otp_info.rdm_stat_info.g = 0;
			param->otp_info.rdm_stat_info.b = 0;
			ISP_LOGE("awb_rdm_otp_data = %p, awb_rdm_otp_len = %d. Parser fail", awb_rdm_otp_data, awb_rdm_otp_len);
		}

		if (otp_map_version == 4 && NULL != awb_golden_otp_data && 0 != awb_golden_otp_len) {
			param->otp_info.gldn_stat_info.r = awb_golden_otp_data[0];
			param->otp_info.gldn_stat_info.g = awb_golden_otp_data[1];
			param->otp_info.gldn_stat_info.b = awb_golden_otp_data[2];
			ISP_LOGV("awb otp golden [%d %d %d]", param->otp_info.gldn_stat_info.r, param->otp_info.gldn_stat_info.g, param->otp_info.gldn_stat_info.b);
		} else if (otp_map_version == 1 && NULL != awb_golden_otp_data_v1 && 0 != awb_golden_otp_len) {
			param->otp_info.gldn_stat_info.r = (awb_golden_otp_data_v1[1] << 8) | awb_golden_otp_data_v1[0];
			param->otp_info.gldn_stat_info.g = (awb_golden_otp_data_v1[3] << 8) | awb_golden_otp_data_v1[2];
			param->otp_info.gldn_stat_info.b = (awb_golden_otp_data_v1[5] << 8) | awb_golden_otp_data_v1[4];
			ISP_LOGV("awb otp golden [%d %d %d]", param->otp_info.gldn_stat_info.r, param->otp_info.gldn_stat_info.g, param->otp_info.gldn_stat_info.b);
		} else {
			param->otp_info.gldn_stat_info.r = 0;
			param->otp_info.gldn_stat_info.g = 0;
			param->otp_info.gldn_stat_info.b = 0;
			ISP_LOGE("awb_golden_otp_data = %p, awb_golden_otp_len = %d. Parser fail", awb_golden_otp_data, awb_golden_otp_len);
		}
	} else {
		ISP_LOGE("awb awb_otp_info_ptr = %p, module_info_ptr = %p. Parser fail !", awb_otp_info_ptr, module_info_ptr);
		param->otp_info.rdm_stat_info.r = 0;
		param->otp_info.rdm_stat_info.g = 0;
		param->otp_info.rdm_stat_info.b = 0;
		param->otp_info.gldn_stat_info.r = 0;
		param->otp_info.gldn_stat_info.g = 0;
		param->otp_info.gldn_stat_info.b = 0;
	}

	return rtn;
}

/*------------------------------------------------------------------------------*
*					public functions			*
*-------------------------------------------------------------------------------*/
/* awb_sprd_ctrl_init--
*@ handle: instance
*@ param: input param
*@ result: output result
*@ return:
*@           AWB_CTRL_INVALID_HANDLE: failed
*@	     others: awb ctrl handle
*/
awb_ctrl_handle_t awb_sprd_ctrl_init(void *in, void *out)
{
	cmr_u32 rtn = AWB_CTRL_SUCCESS;
	struct awb_ctrl_init_param *param = (struct awb_ctrl_init_param *)in;
	struct awb_ctrl_init_result *result = (struct awb_ctrl_init_result *)out;
	struct awb_ctrl_cxt *cxt = NULL;
	struct sensor_otp_awb_info otp_info;

	if (NULL == param || NULL == result) {
		ISP_LOGE("fail to init awb, invalid param: param=%p, result=%p", param, result);
		goto ERROR_EXIT;
	}

	cxt = (struct awb_ctrl_cxt *)malloc(sizeof(struct awb_ctrl_cxt));
	if (NULL == cxt) {
		ISP_LOGE("fail to malloc awb ctrl cxt");
		goto ERROR_EXIT;
	}
	memset(cxt, 0, sizeof(struct awb_ctrl_cxt));
	cxt->lib_info = &param->lib_param;

	rtn = awbsprd_load_lib(cxt);
	if (AWB_CTRL_SUCCESS != rtn) {
		ISP_LOGE("fail to load awb lib");
		goto ERROR_EXIT;
	}

	cxt->snap_lock = 0;			// recovery snapshot awb continus frames
	cxt->last_enable = 0;
	cxt->flash_info.flash_enable = 0;
	cxt->lock_info.lock_flash_frame = 0;
	cxt->flash_info.main_flash_enable = 0;
	cxt->magic_begin = AWB_CTRL_MAGIC_BEGIN;
	cxt->magic_end = AWB_CTRL_MAGIC_END;
	cxt->flash_update_awb = 1;
	cxt->flash_pre_state = 0;

	cxt->sensor_role = param->sensor_role;
	cxt->is_multi_mode = param->is_multi_mode;
	cxt->ptr_isp_br_ioctrl = param->ptr_isp_br_ioctrl;
	ISP_LOGI("is_multi_mode=%d\n", param->is_multi_mode);

	// paser awb otp info
	_awb_parser_otp_info(param);
	pthread_mutex_init(&cxt->status_lock, NULL);

	cxt->stat_img_size = param->stat_img_size;
	cxt->awb_init_param.stat_w = param->stat_img_size.w;
	cxt->awb_init_param.stat_h = param->stat_img_size.h;
	cxt->awb_init_param.otp_random_r = param->otp_info.rdm_stat_info.r;
	cxt->awb_init_param.otp_random_g = param->otp_info.rdm_stat_info.g;
	cxt->awb_init_param.otp_random_b = param->otp_info.rdm_stat_info.b;
	cxt->awb_init_param.otp_golden_r = param->otp_info.gldn_stat_info.r;
	cxt->awb_init_param.otp_golden_g = param->otp_info.gldn_stat_info.g;
	cxt->awb_init_param.otp_golden_b = param->otp_info.gldn_stat_info.b;

	memcpy(&cxt->awb_init_param.tuning_param, param->tuning_param, sizeof(cxt->awb_init_param.tuning_param));

	struct awbParaGenIn *awb_ui_data = (struct awbParaGenIn *)&cxt->awb_init_param.tuning_param.ui_data[0];
	cxt->awb_init_param.otp_golden_r = awb_ui_data->otp_golden_r;
	cxt->awb_init_param.otp_golden_g = awb_ui_data->otp_golden_g;
	cxt->awb_init_param.otp_golden_b = awb_ui_data->otp_golden_b;
	if ((cxt->awb_init_param.otp_golden_r == 0) || (cxt->awb_init_param.otp_golden_g == 0) || (cxt->awb_init_param.otp_golden_b == 0)) {
		cxt->awb_init_param.otp_golden_r = param->otp_info.gldn_stat_info.r;
		cxt->awb_init_param.otp_golden_g = param->otp_info.gldn_stat_info.g;
		cxt->awb_init_param.otp_golden_b = param->otp_info.gldn_stat_info.b;
	}

	cxt->otp_info.gldn_stat_info.r = cxt->awb_init_param.otp_golden_r;
	cxt->otp_info.gldn_stat_info.g = cxt->awb_init_param.otp_golden_g;
	cxt->otp_info.gldn_stat_info.b = cxt->awb_init_param.otp_golden_b;
	cxt->otp_info.rdm_stat_info.r = param->otp_info.rdm_stat_info.r;
	cxt->otp_info.rdm_stat_info.g = param->otp_info.rdm_stat_info.g;
	cxt->otp_info.rdm_stat_info.b = param->otp_info.rdm_stat_info.b;

	if (cxt->awb_init_param.tuning_param.skip_frame_num > 8) {
		cxt->awb_init_param.tuning_param.skip_frame_num = 0;
	}
	if ((cxt->awb_init_param.tuning_param.calc_interval_num == 0) || (cxt->awb_init_param.tuning_param.calc_interval_num > 10)) {
		cxt->awb_init_param.tuning_param.calc_interval_num = 1;
	}
	if ((cxt->awb_init_param.tuning_param.smooth_buffer_num <= 2) || (cxt->awb_init_param.tuning_param.smooth_buffer_num > 64)) {
		cxt->awb_init_param.tuning_param.smooth_buffer_num = 8;
	}
	struct awb_rgb_gain awb_gain;
	cxt->alg_handle = cxt->lib_ops.awb_init_v1(&cxt->awb_init_param, &awb_gain);

	cmr_u32 smooth_buffer_num = cxt->awb_init_param.tuning_param.smooth_buffer_num;
	_init_gain_queue(&cxt->gain_queue, smooth_buffer_num);

	result->gain.r = awb_gain.r_gain;
	result->gain.g = awb_gain.g_gain;
	result->gain.b = awb_gain.b_gain;
	result->ct = awb_gain.ct;

	cxt->cur_gain.r = result->gain.r;
	cxt->cur_gain.g = result->gain.g;
	cxt->cur_gain.b = result->gain.b;
	cxt->cur_ct = result->ct;
	cxt->camera_id = param->camera_id;

	cxt->output_gain.r = result->gain.r;
	cxt->output_gain.g = result->gain.g;
	cxt->output_gain.b = result->gain.b;
	cxt->output_ct = result->ct;

	otp_info.otp_golden_r = cxt->otp_info.gldn_stat_info.r;
	otp_info.otp_golden_g = cxt->otp_info.gldn_stat_info.g;
	otp_info.otp_golden_b = cxt->otp_info.gldn_stat_info.b;
	otp_info.otp_random_r = cxt->otp_info.rdm_stat_info.r;
	otp_info.otp_random_g = cxt->otp_info.rdm_stat_info.g;
	otp_info.otp_random_b = cxt->otp_info.rdm_stat_info.b;

#ifndef CONFIG_ISP_2_2
	if (cxt->ptr_isp_br_ioctrl != NULL) {
		rtn = cxt->ptr_isp_br_ioctrl(cxt->camera_id, SET_OTP_AWB, &otp_info, NULL);
	}
#endif
	_awb_read_gain(&s_save_awb_param[0], sizeof(s_save_awb_param) / sizeof(struct awb_save_gain));

	if (0 != s_save_awb_param[cxt->camera_id].r && 0 != s_save_awb_param[cxt->camera_id].g && 0 != s_save_awb_param[cxt->camera_id].b) {
		cxt->output_gain.r = s_save_awb_param[cxt->camera_id].r;
		cxt->output_gain.g = s_save_awb_param[cxt->camera_id].g;
		cxt->output_gain.b = s_save_awb_param[cxt->camera_id].b;
		cxt->output_ct = s_save_awb_param[cxt->camera_id].ct;
	}

	ISP_LOGV("AWB init: (%d,%d,%d), AWB_OTP rr = %d, rg = %d, rb = %d, gr = %d, gg = %d, gb = %d", cxt->output_gain.r, cxt->output_gain.g, cxt->output_gain.b,
			 cxt->awb_init_param.otp_random_r, cxt->awb_init_param.otp_random_g, cxt->awb_init_param.otp_random_b, cxt->awb_init_param.otp_golden_r, cxt->awb_init_param.otp_golden_g,
			 cxt->awb_init_param.otp_golden_b);

	pthread_mutex_init(&cxt->status_lock, NULL);
	ISP_LOGI("done");
	return (awb_ctrl_handle_t) cxt;

  ERROR_EXIT:
	awbsprd_unload_lib(cxt);
	AWB_CTRL_SAFE_FREE(cxt);
	ISP_LOGE("fail to init awb %d", rtn);
	return AWB_CTRL_INVALID_HANDLE;
}

/* awb_sprd_ctrl_deinit--
*@ handle: instance
*@ param: input param
*@ result: output result
*@ return:
*@           0: successful
*@	     others: failed
*/
cmr_s32 awb_sprd_ctrl_deinit(void *handle, void *in, void *out)
{
	UNUSED(in);
	UNUSED(out);
	cmr_s32 rtn = AWB_CTRL_SUCCESS;
	struct awb_ctrl_cxt *cxt = (struct awb_ctrl_cxt *)handle;

	rtn = _check_handle(handle);

	if (AWB_CTRL_SUCCESS != rtn) {
		ISP_LOGE("fail to deinit awb");
		rtn = AWB_CTRL_ERROR;
		goto EXIT;
	}

	_deinit_gain_queue(&cxt->gain_queue);

	pthread_mutex_destroy(&cxt->status_lock);

	rtn = cxt->lib_ops.awb_deinit_v1(cxt->alg_handle);
	if (AWB_CTRL_SUCCESS != rtn) {
		ISP_LOGE("fail to deinit awb_v1 \n");
	}

	rtn = awbsprd_unload_lib(cxt);
	if (AWB_CTRL_SUCCESS != rtn) {
		ISP_LOGE("fail to unload awb lib");
		goto EXIT;
	}

	memset(cxt, 0, sizeof(*cxt));
	AWB_CTRL_SAFE_FREE(cxt);

	ISP_LOGI("done");
	return rtn;
  EXIT:
	memset(cxt, 0, sizeof(*cxt));
	AWB_CTRL_SAFE_FREE(cxt);
	ISP_LOGE("fail to deinit awb");
	return rtn;
}

/* awb_sprd_ctrl_calculation--
*@ handle: instance
*@ param: input param
*@ result: output result
*@ return:
*@           0: successful
*@	     others: failed
*/
cmr_s32 awb_sprd_ctrl_calculation(void *handle, void *in, void *out)
{
	cmr_s32 rtn = AWB_CTRL_SUCCESS;
	struct awb_ctrl_cxt *cxt = (struct awb_ctrl_cxt *)handle;
	struct awb_ctrl_calc_param param;
	struct awb_ctrl_calc_result result;
	UNUSED(out);

	if (NULL == in) {
		ISP_LOGE("fail to calc awb, invalid param: param=%p", in);
		return AWB_CTRL_ERROR;
	}

	memcpy(&param, in, sizeof(struct awb_ctrl_calc_param));
	memset(&result, 0, sizeof(struct awb_ctrl_calc_result));

	rtn = _check_handle(handle);
	if (AWB_CTRL_SUCCESS != rtn) {
		ISP_LOGE("fail to check handle");
		return AWB_CTRL_ERROR;
	}
#ifndef CONFIG_ISP_2_2
	if ((cxt->is_multi_mode == ISP_ALG_DUAL_SBS) || (cxt->is_multi_mode == ISP_ALG_DUAL_NORMAL)) {
		if ((!cxt->sensor_role) && (cxt->ptr_isp_br_ioctrl != NULL)) {
			struct awb_sync_info awb_sync;

			cxt->ptr_isp_br_ioctrl(cxt->camera_id - 2, GET_STAT_AWB_DATA, NULL, &awb_sync.stat_master_info);
			cxt->ptr_isp_br_ioctrl(cxt->camera_id, GET_STAT_AWB_DATA, NULL, &awb_sync.stat_slave_info);

			struct awb_ctrl_gain gain_master;
			struct awb_ctrl_gain gain_slave;
			cxt->ptr_isp_br_ioctrl(cxt->camera_id - 2, GET_GAIN_AWB_DATA, NULL, &gain_master);

			awb_sync.stat_master_info.height = 32;
			awb_sync.stat_master_info.width = 32;
			awb_sync.stat_slave_info.height = 32;
			awb_sync.stat_slave_info.width = 32;

			awb_sync.master_fov = 0;
			awb_sync.slave_fov = 0;
			awb_sync.master_pix_cnt = ((1600 / awb_sync.stat_master_info.width) * (1200 / awb_sync.stat_master_info.height)) / 4;
			awb_sync.slave_pix_cnt = ((1600 / awb_sync.stat_slave_info.width) * (1200 / awb_sync.stat_slave_info.height)) / 4;

			cmr_u32 ct;
			cxt->ptr_isp_br_ioctrl(cxt->camera_id - 2, GET_MATCH_AWB_DATA, NULL, &ct);

			struct sensor_otp_awb_info master_otp_info;
			struct sensor_otp_awb_info slave_otp_info;

			cxt->ptr_isp_br_ioctrl(cxt->camera_id - 2, GET_OTP_AWB, NULL, &master_otp_info);
			cxt->ptr_isp_br_ioctrl(cxt->camera_id, GET_OTP_AWB, NULL, &slave_otp_info);

			awb_sync.master_gldn_stat_info.r = master_otp_info.otp_golden_r;
			awb_sync.master_gldn_stat_info.g = master_otp_info.otp_golden_g;
			awb_sync.master_gldn_stat_info.b = master_otp_info.otp_golden_b;
			awb_sync.master_rdm_stat_info.r = master_otp_info.otp_random_r;
			awb_sync.master_rdm_stat_info.g = master_otp_info.otp_random_g;
			awb_sync.master_rdm_stat_info.b = master_otp_info.otp_random_b;

			awb_sync.slave_gldn_stat_info.r = slave_otp_info.otp_golden_r;
			awb_sync.slave_gldn_stat_info.g = slave_otp_info.otp_golden_g;
			awb_sync.slave_gldn_stat_info.b = slave_otp_info.otp_golden_b;
			awb_sync.slave_rdm_stat_info.r = slave_otp_info.otp_random_r;
			awb_sync.slave_rdm_stat_info.g = slave_otp_info.otp_random_g;
			awb_sync.slave_rdm_stat_info.b = slave_otp_info.otp_random_b;

			int ret = cxt->lib_ops.awb_sync_gain(&awb_sync, gain_master.r, gain_master.g, gain_master.b, &gain_slave.r, &gain_slave.g, &gain_slave.b);

			cxt->ptr_isp_br_ioctrl(cxt->camera_id, SET_GAIN_AWB_DATA, &gain_slave, NULL);

			result.gain.r = gain_slave.r;
			result.gain.g = gain_slave.g;
			result.gain.b = gain_slave.b;
			result.ct = ct;

			cxt->output_gain.r = result.gain.r;
			cxt->output_gain.g = result.gain.g;
			cxt->output_gain.b = result.gain.b;
			cxt->output_ct = result.ct;

			if (ret == 0) {
				result.update_gain = 1;
				return AWB_CTRL_SUCCESS;
			}
		}
	}
#endif

	cmr_u32 smooth_buffer_num = cxt->awb_init_param.tuning_param.smooth_buffer_num;
	cmr_u32 skip_frame_num = cxt->awb_init_param.tuning_param.skip_frame_num;
	cmr_u32 calc_interval_num = cxt->awb_init_param.tuning_param.calc_interval_num;

	if ((AWB_CTRL_SCENEMODE_AUTO == cxt->scene_mode) && (AWB_CTRL_WB_MODE_AUTO == cxt->wb_mode)) {
		cxt->frame_count++;
		if ((cxt->frame_count <= skip_frame_num) || ((cxt->frame_count > smooth_buffer_num) && (cxt->frame_count % calc_interval_num == 1)))	// for power saving, do awb calc once every two frames
		{
			result.gain.r = cxt->output_gain.r;
			result.gain.g = cxt->output_gain.g;
			result.gain.b = cxt->output_gain.b;
			result.ct = cxt->output_ct;

			result.log_awb.log = cxt->log;
			result.log_awb.size = cxt->size;

			return rtn;
		}
	}

	struct awb_calc_param calc_param;
	struct awb_calc_result calc_result;
	memset(&calc_param, 0x00, sizeof(calc_param));
	memset(&calc_result, 0x00, sizeof(calc_result));
	if (cxt->awb_init_param.tuning_param.stat_type) {
		calc_param.stat_img.r = param.stat_img.chn_img.r;
		calc_param.stat_img.g = param.stat_img.chn_img.g;
		calc_param.stat_img.b = param.stat_img.chn_img.b;
		calc_param.b_pix_cnt = (cxt->init_param.stat_win_size.w * cxt->init_param.stat_win_size.h) / 4;
		calc_param.g_pix_cnt = (cxt->init_param.stat_win_size.w * cxt->init_param.stat_win_size.h) / 4;
		calc_param.r_pix_cnt = (cxt->init_param.stat_win_size.w * cxt->init_param.stat_win_size.h) / 4;
		calc_param.stat_img_w = 32;
		calc_param.stat_img_h = 32;
	} else {
		calc_param.stat_img.r = param.stat_img_awb.chn_img.r;
		calc_param.stat_img.g = param.stat_img_awb.chn_img.g;
		calc_param.stat_img.b = param.stat_img_awb.chn_img.b;
		calc_param.stat_img_w = param.stat_width_awb;
		calc_param.stat_img_h = param.stat_height_awb;
		calc_param.r_pix_cnt = 1;
		calc_param.g_pix_cnt = 1;
		calc_param.b_pix_cnt = 1;
	}

	calc_param.bv = param.bv;
	calc_param.iso = param.ae_info.iso;

	memcpy(calc_param.matrix, param.matrix, 9 * sizeof(cmr_s32));
	memcpy(calc_param.gamma, param.gamma, 256);
	ATRACE_BEGIN(__FUNCTION__);
	cmr_u64 time0 = systemTime(CLOCK_MONOTONIC);
	rtn = cxt->lib_ops.awb_calc_v1(cxt->alg_handle, &calc_param, &calc_result);
	cmr_u64 time1 = systemTime(CLOCK_MONOTONIC);
	ATRACE_END();
	ISP_LOGV("AWB %dx%d: (%d,%d,%d) %dK, %dus", calc_param.stat_img_w, calc_param.stat_img_h, calc_result.awb_gain[0].r_gain, calc_result.awb_gain[0].g_gain,
			 calc_result.awb_gain[0].b_gain, calc_result.awb_gain[0].ct, (cmr_s32) ((time1 - time0) / 1000));

	result.gain.r = calc_result.awb_gain[0].r_gain;
	result.gain.g = calc_result.awb_gain[0].g_gain;
	result.gain.b = calc_result.awb_gain[0].b_gain;
	result.ct = calc_result.awb_gain[0].ct;
	result.pg_flag = calc_result.awb_gain[0].pg;
	result.green100 = calc_result.awb_gain[0].green100;
	result.log_awb.log = calc_result.log_buffer;
	result.log_awb.size = calc_result.log_size;
	result.update_gain = cxt->flash_update_awb;

	cxt->cur_gain.r = result.gain.r;
	cxt->cur_gain.g = result.gain.g;
	cxt->cur_gain.b = result.gain.b;
	cxt->cur_ct = result.ct;
	cxt->log = calc_result.log_buffer;
	cxt->size = calc_result.log_size;

	if (cxt->frame_count > smooth_buffer_num) {
		_gain_queue_add(&cxt->gain_queue, &cxt->cur_gain, cxt->cur_ct, 256);
		_gain_queue_average(&cxt->gain_queue, &cxt->output_gain, &cxt->output_ct);
	} else {
		cxt->output_gain = cxt->cur_gain;
		cxt->output_ct = cxt->cur_ct;
	}

	ISP_LOGV("AWB smooth output: (%d,%d,%d) %dK", cxt->output_gain.r, cxt->output_gain.g, cxt->output_gain.b, cxt->output_ct);

	//lock awb after snapshot
	if (cxt->snap_lock != 0) {
		cxt->output_gain.r = cxt->recover_gain.r;
		cxt->output_gain.g = cxt->recover_gain.g;
		cxt->output_gain.b = cxt->recover_gain.b;
		cxt->output_ct = cxt->recover_ct;
		cxt->snap_lock -= 1;
	}
	//scenemode & mwb change
	if (AWB_CTRL_SCENEMODE_AUTO == cxt->scene_mode) {
		cmr_u32 mawb_id = cxt->wb_mode;
		if (AWB_CTRL_WB_MODE_AUTO != cxt->wb_mode) {
			if (mawb_id == AWB_CTRL_AWB_MODE_OFF) {
				cxt->output_gain.r = 1024;
				cxt->output_gain.g = 1024;
				cxt->output_gain.b = 1024;
				cxt->output_ct = 5000;
			} else if ((mawb_id > 0) && (mawb_id < 10))	// return mwb by mwb mode id
			{
				cmr_s32 index = 0;

				cmr_s32 i;
				for (i = 0; i < cxt->awb_init_param.tuning_param.wbModeNum; i++) {
					if (mawb_id == cxt->awb_init_param.tuning_param.wbModeId[i]) {
						index = i;
						break;
					}
				}
				cxt->output_gain.r = cxt->awb_init_param.tuning_param.wbMode_gain[index].r_gain;
				cxt->output_gain.g = cxt->awb_init_param.tuning_param.wbMode_gain[index].g_gain;
				cxt->output_gain.b = cxt->awb_init_param.tuning_param.wbMode_gain[index].b_gain;
				cxt->output_ct = cxt->awb_init_param.tuning_param.wbMode_gain[index].ct;
			} else				// return mwb by ct, (100K <= ct < 10000K)
			{
				if (mawb_id >= 10000) {
					cmr_s32 index = 100;
					cxt->output_gain.r = cxt->awb_init_param.tuning_param.mwb_gain[index].r_gain;
					cxt->output_gain.g = cxt->awb_init_param.tuning_param.mwb_gain[index].g_gain;
					cxt->output_gain.b = cxt->awb_init_param.tuning_param.mwb_gain[index].b_gain;
					cxt->output_ct = cxt->awb_init_param.tuning_param.mwb_gain[index].ct;
				} else if (mawb_id < 100) {
					cmr_s32 index = 1;
					cxt->output_gain.r = cxt->awb_init_param.tuning_param.mwb_gain[index].r_gain;
					cxt->output_gain.g = cxt->awb_init_param.tuning_param.mwb_gain[index].g_gain;
					cxt->output_gain.b = cxt->awb_init_param.tuning_param.mwb_gain[index].b_gain;
					cxt->output_ct = cxt->awb_init_param.tuning_param.mwb_gain[index].ct;
				} else {
					cmr_u32 index1 = mawb_id / 100;
					cmr_u32 index2 = mawb_id / 100 + 1;
					cmr_u32 weight1 = index2 * 100 - mawb_id;
					cmr_u32 weight2 = mawb_id - index1 * 100;
					cxt->output_gain.r = (cxt->awb_init_param.tuning_param.mwb_gain[index1].r_gain * weight1 + cxt->awb_init_param.tuning_param.mwb_gain[index2].r_gain * weight2 + 50) / 100;
					cxt->output_gain.g = (cxt->awb_init_param.tuning_param.mwb_gain[index1].g_gain * weight1 + cxt->awb_init_param.tuning_param.mwb_gain[index2].g_gain * weight2 + 50) / 100;
					cxt->output_gain.b = (cxt->awb_init_param.tuning_param.mwb_gain[index1].b_gain * weight1 + cxt->awb_init_param.tuning_param.mwb_gain[index2].b_gain * weight2 + 50) / 100;
					cxt->output_ct = mawb_id;
				}
			}
			if ((cxt->otp_info.gldn_stat_info.r != 0) && (cxt->otp_info.gldn_stat_info.g != 0) && (cxt->otp_info.gldn_stat_info.b != 0)) {
				double otp_g_coef = (double)cxt->otp_info.rdm_stat_info.g / cxt->otp_info.gldn_stat_info.g;
				double otp_r_coef = (double)cxt->otp_info.rdm_stat_info.r / cxt->otp_info.gldn_stat_info.r;
				double otp_b_coef = (double)cxt->otp_info.rdm_stat_info.b / cxt->otp_info.gldn_stat_info.b;

				if (otp_g_coef != 0) {
					otp_r_coef = otp_r_coef / otp_g_coef;
					otp_b_coef = otp_b_coef / otp_g_coef;
					cxt->output_gain.r = cxt->output_gain.r * otp_r_coef;
					cxt->output_gain.b = cxt->output_gain.b * otp_b_coef;
				}
			}
		}
	} else {
		cmr_u32 scene_mode = cxt->scene_mode;
		if (AWB_CTRL_SCENEMODE_USER_0 == scene_mode) {
			cxt->output_gain.r = cxt->awb_init_param.tuning_param.mwb_gain[scene_mode].r_gain;
			cxt->output_gain.g = cxt->awb_init_param.tuning_param.mwb_gain[scene_mode].g_gain;
			cxt->output_gain.b = cxt->awb_init_param.tuning_param.mwb_gain[scene_mode].b_gain;
			cxt->output_ct = cxt->awb_init_param.tuning_param.mwb_gain[scene_mode].ct;
		}
	}

	//lock mode
	if (AWB_CTRL_LOCKMODE == cxt->lock_info.lock_mode && cxt->wb_mode == 0) {
		cxt->output_gain.r = cxt->lock_info.lock_gain.r;
		cxt->output_gain.g = cxt->lock_info.lock_gain.g;
		cxt->output_gain.b = cxt->lock_info.lock_gain.b;
		cxt->output_ct = cxt->lock_info.lock_ct;
	}
	//only pre flash after
	if (cxt->flash_pre_state != 0 && cxt->wb_mode == 0) {
		cxt->output_gain.r = cxt->recover_gain.r;
		cxt->output_gain.g = cxt->recover_gain.g;
		cxt->output_gain.b = cxt->recover_gain.b;
		cxt->output_ct = cxt->recover_ct;
		cxt->flash_pre_state -= 1;
	}
	//lock awb after flash
	if (cxt->flash_info.main_flash_enable == 1 && cxt->lock_info.lock_flash_frame != 0 && cxt->wb_mode == 0) {
		cxt->output_gain.r = cxt->recover_gain.r;
		cxt->output_gain.g = cxt->recover_gain.g;
		cxt->output_gain.b = cxt->recover_gain.b;
		cxt->output_ct = cxt->recover_ct;
		cxt->lock_info.lock_flash_frame -= 1;
	} else {
		cxt->flash_info.main_flash_enable = 0;
	}

//  ISP_LOGD("cxt->snap_lock =%d lock_mode =%d main_flash_enable =%d  lock_flash_frame =%d ",cxt->snap_lock,cxt->lock_info.lock_mode,cxt->flash_info.main_flash_enable,cxt->lock_info.lock_flash_frame);
	ISP_LOGV("AWB result : (%d,%d,%d) %dK", cxt->output_gain.r, cxt->output_gain.g, cxt->output_gain.b, cxt->output_ct);

	result.gain.r = cxt->output_gain.r;
	result.gain.g = cxt->output_gain.g;
	result.gain.b = cxt->output_gain.b;
	result.ct = cxt->output_ct;

	if ((cxt->is_multi_mode == ISP_ALG_DUAL_SBS) && (cxt->ptr_isp_br_ioctrl != NULL)) {
		cxt->ptr_isp_br_ioctrl(cxt->camera_id, SET_GAIN_AWB_DATA, &result.gain, NULL);
	}

	pthread_mutex_lock(&cxt->status_lock);

	memcpy(&cxt->awb_result, &result, sizeof(struct awb_ctrl_calc_result));

	pthread_mutex_unlock(&cxt->status_lock);

	return rtn;
}

/* awb_sprd_ctrl_ioctrl--
*@ handle: instance
*@ param: input param
*@ result: output result
*@ return:
*@           0: successful
*@	     others: failed
*/
cmr_s32 awb_sprd_ctrl_ioctrl(void *handle, cmr_s32 cmd, void *in, void *out)
{
//      UNUSED(out);
	cmr_s32 rtn = AWB_CTRL_SUCCESS;
	struct awb_ctrl_cxt *cxt = (struct awb_ctrl_cxt *)handle;

	rtn = _check_handle(handle);
	if (AWB_CTRL_SUCCESS != rtn) {
		ISP_LOGE("fail to check handle");
		return AWB_CTRL_ERROR;
	}

	pthread_mutex_lock(&cxt->status_lock);

	switch (cmd) {
	case AWB_CTRL_CMD_SET_WB_MODE:
		rtn = _awb_set_wbmode(cxt, in);
		break;

	case AWB_CTRL_CMD_GET_WB_MODE:
		rtn = _awb_get_wbmode(cxt, out);
		break;

	case AWB_CTRL_CMD_SET_WORK_MODE:
//      ISP_LOGV("AWB_CTRL_CMD_SET_WORK_MODE lock_mode = %d cxt->last_enable =%d cxt->flash_info.flash_enable =%d",cxt->lock_info.lock_mode,cxt->last_enable,cxt->flash_info.flash_enable);
		if (cxt->last_enable == 2 && cxt->flash_info.flash_enable == 0) {
			rtn = _awb_get_recgain(cxt, in);
			if (AWB_CTRL_SUCCESS != rtn) {
				ISP_LOGE("fail to _awb_get_recgain");
				return AWB_CTRL_ERROR;
			}
			cxt->snap_lock = 0;	//lock awb N frames after snapshot
			cxt->last_enable = 0;
		}
		cxt->flash_info.flash_enable = 0;
		rtn = _awb_set_workmode(cxt, in);
		break;

	case AWB_CTRL_CMD_GET_GAIN:
		rtn = _awb_get_gain(cxt, in);
		break;

	case AWB_CTRL_CMD_GET_CUR_GAIN:
		rtn = _awb_get_cur_gain(cxt, in);
		break;

	case AWB_CTRL_CMD_RESULT_INFO:
		rtn = _awb_get_result_info(cxt, in);
		break;

	case AWB_CTRL_CMD_FLASHING:
		ISP_LOGV("FLASH_TAG: AWB_CTRL_CMD_FLASHING");
		//rtn = _awb_set_flash_gain(cxt, in);
		break;

	case AWB_CTRL_CMD_FLASH_OPEN_M:
		ISP_LOGV("FLASH_TAG: AWB_CTRL_CMD_FLASH_OPEN_M");
		cxt->flash_info.flash_mode = AWB_CTRL_FLASH_MAIN;
		break;

	case AWB_CTRL_CMD_FLASH_OPEN_P:
		ISP_LOGV("FLASH_TAG: AWB_CTRL_CMD_FLASH_OPEN_P");
		cxt->flash_info.flash_mode = AWB_CTRL_FLASH_PRE;
		break;

	case AWB_CTRL_CMD_FLASH_CLOSE:
		ISP_LOGV("FLASH_TAG: AWB_CTRL_CMD_FLASH_CLOSE");
		if (cxt->wb_mode == 0) {
			if ((AWB_CTRL_FLASH_PRE == cxt->flash_info.flash_mode) || (AWB_CTRL_FLASH_MAIN == cxt->flash_info.flash_mode)) {
				rtn = _awb_get_recgain(cxt, in);
			}
		}
		cxt->flash_info.flash_mode = AWB_CTRL_FLASH_END;
		cxt->flash_info.flash_enable = AWB_CTRL_FLASH_END;

		break;

	case AWB_CTRL_CMD_FLASH_BEFORE_P:
		ISP_LOGV("FLASH_TAG: AWB_CTRL_CMD_FLASH_BEFORE_P");
		cxt->flash_info.flash_enable = 1;
		if (cxt->wb_mode == 0) {
			rtn = _awb_set_recgain(cxt, in);
		}
		break;

	case AWB_CTRL_CMD_LOCK:
		rtn = _awb_set_lock(cxt, in);
		break;

	case AWB_CTRL_CMD_UNLOCK:
		rtn = _awb_get_unlock(cxt, in);
		break;

	case AWB_CTRL_CMD_FLASH_SNOP:
		rtn = _awb_flash_snopshot_recovery(cxt, in);
		break;

	case AWB_CTRL_CMD_GET_STAT_SIZE:
		rtn = _awb_get_stat_size(cxt, in);
		break;

	case AWB_CTRL_CMD_GET_PIX_CNT:
		rtn = _awb_get_pix_cnt(cxt, in);
		break;

	case AWB_CTRL_CMD_GET_CT:
		rtn = _awb_get_ct(cxt, in);
		break;

	case AWB_CTRL_CMD_SET_FLASH_STATUS:
		rtn = _awb_set_flash_status(cxt, in);
		break;
	case AWB_CTRL_CMD_SET_UPDATE_TUNING_PARAM:
		rtn = _awb_set_tuning_param(cxt, in);
		break;

	case AWB_CTRL_CMD_GET_DEBUG_INFO:
		if (out) {
			rtn = awb_get_debug_info(cxt, out);
		}
		break;

	case AWB_CTRL_CMD_EM_GET_PARAM:
		rtn = awb_get_debug_info_for_display(cxt, out);
		break;

	case AWB_CTRL_CMD_VIDEO_STOP_NOTIFY:
//      ISP_LOGV("AWB_CTRL_CMD_VIDEO_STOP_NOTIFY  cxt->lock_info.lock_mode =%d  cxt->last_enable =%d  flash_mode =%d ",cxt->lock_info.lock_mode,cxt->last_enable,cxt->flash_info.flash_enable);
		if (cxt->last_enable == 0 && cxt->flash_info.flash_enable == 0) {
			rtn = _awb_set_recgain(cxt, in);
		}
		cxt->last_enable++;
		break;

	case AWB_CTRL_CMD_GET_CT_TABLE20:
		rtn = _awb_get_flash_ct_table(cxt, out);
		break;

	case AWB_CTRL_CMD_GET_DATA_TYPE:
		rtn = _awb_get_data_type(cxt, out);
		break;
	default:
		ISP_LOGE("fail to get invalid cmd %d", cmd);
		rtn = AWB_CTRL_ERROR;
		break;
	}

	pthread_mutex_unlock(&cxt->status_lock);
	return rtn;
}

struct adpt_ops_type awb_sprd_adpt_ops_ver1 = {
	.adpt_init = awb_sprd_ctrl_init,
	.adpt_deinit = awb_sprd_ctrl_deinit,
	.adpt_process = awb_sprd_ctrl_calculation,
	.adpt_ioctrl = awb_sprd_ctrl_ioctrl,
};
