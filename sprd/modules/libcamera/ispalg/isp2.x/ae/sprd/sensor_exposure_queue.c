/*
 * Copyright (C) 2015 The Android Open Source Project
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
#include "sensor_exposure_queue.h"
#include "cmr_log.h"

#ifndef UNUSED
#define     UNUSED(param)  (void)(param)
#endif

#ifndef MAX
#define MAX(_a,_b)  ((_a)>(_b)?(_a):(_b))
#endif

#define S_Q_LENGTH (10)

struct q_head {
	cmr_u32 num;
	struct q_item *queue_ptr;
};

struct q_item_index {
	cmr_s32 idx_exp_l;
	cmr_s32 idx_exp_t;
	cmr_s32 idx_dmy;
	cmr_s32 idx_sensor_gain;
	cmr_s32 idx_isp_gain;
};

/*in rolling shutter, exposure time valid number is always 2, and sensor valid number maybe is 1 or 2,
 *isp gain is always is 1;
 *So exp_valid_num >= sensor_gain_valid_num >= isp_gain_valid_num;
 */
struct queue_context {
	cmr_s32 exp_valid_num;
	cmr_s32 sensor_gain_valid_num;
	cmr_s32 isp_gain_valid_num;
	cmr_s32 max_valid_num;

	cmr_s32 exp_valid_offset;	/*according to max validata num */
	cmr_s32 sensor_gain_valid_offset;	/*according to max validata num */
	cmr_s32 isp_gain_valid_offset;	/*according to max validata num */

	struct q_item_index cur_idx;
	struct q_item_index write_idx;
	struct q_item_index actual_idx;
	struct q_head q_pool;
};

static void s_q_increase_idx(struct queue_context *q_cxt, struct q_item_index *idx, cmr_s32 increase_step)
{
	UNUSED(q_cxt);

	idx->idx_exp_l += increase_step;
	idx->idx_exp_l = (idx->idx_exp_l + S_Q_LENGTH) % S_Q_LENGTH;

	idx->idx_exp_t += increase_step;
	idx->idx_exp_t = (idx->idx_exp_t + S_Q_LENGTH) % S_Q_LENGTH;

	idx->idx_sensor_gain += increase_step;
	idx->idx_sensor_gain = (idx->idx_sensor_gain + S_Q_LENGTH) % S_Q_LENGTH;

	idx->idx_isp_gain += increase_step;
	idx->idx_isp_gain = (idx->idx_isp_gain + S_Q_LENGTH) % S_Q_LENGTH;

	idx->idx_dmy += increase_step;
	idx->idx_dmy = (idx->idx_dmy + S_Q_LENGTH) % S_Q_LENGTH;
}

static void s_q_update_actual_idx(struct queue_context *q_cxt, struct q_item_index *cur_idx, struct q_item_index *actual_idx)
{
	UNUSED(q_cxt);

	actual_idx->idx_exp_l = cur_idx->idx_exp_l - q_cxt->max_valid_num;
	actual_idx->idx_exp_l = (actual_idx->idx_exp_l + S_Q_LENGTH) % S_Q_LENGTH;

	actual_idx->idx_exp_t = cur_idx->idx_exp_t - q_cxt->max_valid_num;
	actual_idx->idx_exp_t = (actual_idx->idx_exp_t + S_Q_LENGTH) % S_Q_LENGTH;

	actual_idx->idx_sensor_gain = cur_idx->idx_sensor_gain - q_cxt->max_valid_num;
	actual_idx->idx_sensor_gain = (actual_idx->idx_sensor_gain + S_Q_LENGTH) % S_Q_LENGTH;

	actual_idx->idx_isp_gain = cur_idx->idx_isp_gain - q_cxt->max_valid_num;
	actual_idx->idx_isp_gain = (actual_idx->idx_isp_gain + S_Q_LENGTH) % S_Q_LENGTH;

	actual_idx->idx_dmy = cur_idx->idx_dmy - q_cxt->max_valid_num;
	actual_idx->idx_dmy = (actual_idx->idx_dmy + S_Q_LENGTH) % S_Q_LENGTH;
}

static void s_q_update_write_idx(struct queue_context *q_cxt, struct q_item_index *cur_idx, struct q_item_index *write_idx)
{
	UNUSED(q_cxt);

	/*init write-index: exp_line/exp_time/dummy_line */
	write_idx->idx_exp_l = cur_idx->idx_exp_l + q_cxt->exp_valid_offset;
	write_idx->idx_exp_l = (write_idx->idx_exp_l + S_Q_LENGTH) % S_Q_LENGTH;

	write_idx->idx_exp_t = cur_idx->idx_exp_t + q_cxt->exp_valid_offset;
	write_idx->idx_exp_t = (write_idx->idx_exp_t + S_Q_LENGTH) % S_Q_LENGTH;

	write_idx->idx_dmy = cur_idx->idx_dmy + q_cxt->exp_valid_offset;
	write_idx->idx_dmy = (write_idx->idx_dmy + S_Q_LENGTH) % S_Q_LENGTH;

	/*init write-index: sensor gain */
	write_idx->idx_sensor_gain = cur_idx->idx_sensor_gain + q_cxt->sensor_gain_valid_offset;
	write_idx->idx_sensor_gain = (write_idx->idx_sensor_gain + S_Q_LENGTH) % S_Q_LENGTH;

	/*init write-index: isp gain */
	write_idx->idx_isp_gain = cur_idx->idx_isp_gain + q_cxt->isp_gain_valid_offset;
	write_idx->idx_isp_gain = (write_idx->idx_isp_gain + S_Q_LENGTH) % S_Q_LENGTH;
}

static void s_q_add_item(struct queue_context *q_cxt, struct q_item *cur_item, struct q_item_index *idx)
{
	q_cxt->q_pool.queue_ptr[idx->idx_exp_l].exp_line = cur_item->exp_line;
	q_cxt->q_pool.queue_ptr[idx->idx_exp_t].exp_time = cur_item->exp_time;
	q_cxt->q_pool.queue_ptr[idx->idx_dmy].dumy_line = cur_item->dumy_line;
	q_cxt->q_pool.queue_ptr[idx->idx_sensor_gain].sensor_gain = cur_item->sensor_gain;
	q_cxt->q_pool.queue_ptr[idx->idx_isp_gain].isp_gain = cur_item->isp_gain;
}

static void s_q_get_item(struct queue_context *q_cxt, struct q_item_index *idx, struct q_item *item)
{
	item->exp_time = q_cxt->q_pool.queue_ptr[idx->idx_exp_t].exp_time;
	item->exp_line = q_cxt->q_pool.queue_ptr[idx->idx_exp_l].exp_line;
	item->dumy_line = q_cxt->q_pool.queue_ptr[idx->idx_dmy].dumy_line;
	item->sensor_gain = q_cxt->q_pool.queue_ptr[idx->idx_sensor_gain].sensor_gain;
	item->isp_gain = q_cxt->q_pool.queue_ptr[idx->idx_isp_gain].isp_gain;
}

cmr_s32 s_q_init(cmr_handle q_handle, struct s_q_init_in *in, struct s_q_init_out *out)
{
	cmr_s32 ret = 0;
	struct queue_context *q_cxt = NULL;
	cmr_u32 i = 0;

	if ((NULL == in) || (NULL == out) || (NULL == out)) {
		ret = -1;
		return ret;
	}

	q_cxt = (struct queue_context *)q_handle;
	for (i = 0; i < q_cxt->q_pool.num; ++i) {
		q_cxt->q_pool.queue_ptr[i].exp_line = in->exp_line;
		q_cxt->q_pool.queue_ptr[i].exp_time = in->exp_time;
		q_cxt->q_pool.queue_ptr[i].dumy_line = in->dmy_line;
		q_cxt->q_pool.queue_ptr[i].sensor_gain = in->sensor_gain;
		q_cxt->q_pool.queue_ptr[i].isp_gain = in->isp_gain;
	}

	/*init cur-index */
	q_cxt->cur_idx.idx_exp_l = 0;
	q_cxt->cur_idx.idx_exp_t = 0;
	q_cxt->cur_idx.idx_sensor_gain = 0;
	q_cxt->cur_idx.idx_isp_gain = 0;
	q_cxt->cur_idx.idx_dmy = 0;

	/*init actual index */
	s_q_update_actual_idx(q_cxt, &q_cxt->cur_idx, &q_cxt->actual_idx);
	s_q_get_item(q_cxt, &q_cxt->write_idx, &out->actual_item);
	/*init write-index */
	s_q_update_write_idx(q_cxt, &q_cxt->cur_idx, &q_cxt->write_idx);
	s_q_get_item(q_cxt, &q_cxt->write_idx, &out->write_item);
	/*update cur index flag */
	s_q_increase_idx(q_cxt, &q_cxt->cur_idx, 1);

	return ret;
}

cmr_handle s_q_open(struct s_q_open_param * param)
{
	struct queue_context *q_cxt = NULL;

	q_cxt = (struct queue_context *)malloc(sizeof(struct queue_context));
	if (NULL == q_cxt) {
		ISP_LOGE("malloc cxt fail\n");
		goto q_init_error_exit;
	}

	memset((void *)q_cxt, 0, sizeof(struct queue_context));

	q_cxt->q_pool.queue_ptr = (struct q_item *)malloc(S_Q_LENGTH * sizeof(struct q_item));
	if (NULL == q_cxt->q_pool.queue_ptr) {
		ISP_LOGE("malloc pool fail\n");
		goto q_init_error_exit;
	}

	memset((void *)q_cxt->q_pool.queue_ptr, 0, S_Q_LENGTH * sizeof(struct q_item));
	q_cxt->q_pool.num = S_Q_LENGTH;

	q_cxt->exp_valid_num = param->exp_valid_num;
	q_cxt->sensor_gain_valid_num = param->sensor_gain_valid_num;
	q_cxt->isp_gain_valid_num = param->isp_gain_valid_num;

	q_cxt->max_valid_num = MAX(q_cxt->exp_valid_num, q_cxt->sensor_gain_valid_num);
	q_cxt->max_valid_num = MAX(q_cxt->max_valid_num, q_cxt->isp_gain_valid_num);

	q_cxt->exp_valid_offset = q_cxt->exp_valid_num - q_cxt->max_valid_num;
	q_cxt->sensor_gain_valid_offset = q_cxt->sensor_gain_valid_num - q_cxt->max_valid_num;
	q_cxt->isp_gain_valid_offset = q_cxt->isp_gain_valid_num - q_cxt->max_valid_num;

	ISP_LOGI("valid num: exp: %d, sensor gain: %d, isp gain: %d\n", q_cxt->exp_valid_num, q_cxt->sensor_gain_valid_num, q_cxt->isp_gain_valid_num);

	return (cmr_handle) q_cxt;

  q_init_error_exit:
	if (q_cxt) {
		if (q_cxt->q_pool.queue_ptr) {
			free(q_cxt->q_pool.queue_ptr);
			q_cxt->q_pool.queue_ptr = NULL;
		}
		free(q_cxt);
		q_cxt = NULL;
	}
	return q_cxt;
}

cmr_s32 s_q_put(cmr_handle q_handle, struct q_item * input_item, struct q_item * write_2_sensor, struct q_item * actual_item)
{
	cmr_s32 ret = 0;
	struct queue_context *q_cxt = NULL;

	if ((NULL == q_handle) || (NULL == input_item) || (NULL == write_2_sensor) || (NULL == actual_item)) {
		ISP_LOGE("cxt: %p, input: %p, write: %p, actual: %p\n", q_handle, input_item, write_2_sensor, actual_item);
		ret = -1;
		return ret;
	}

	q_cxt = (struct queue_context *)q_handle;
	s_q_add_item(q_cxt, input_item, &q_cxt->cur_idx);

	s_q_update_write_idx(q_cxt, &q_cxt->cur_idx, &q_cxt->write_idx);
	s_q_get_item(q_cxt, &q_cxt->write_idx, write_2_sensor);

	s_q_update_actual_idx(q_cxt, &q_cxt->cur_idx, &q_cxt->actual_idx);
	s_q_get_item(q_cxt, &q_cxt->actual_idx, actual_item);

	s_q_increase_idx(q_cxt, &q_cxt->cur_idx, 1);

	return ret;
}

cmr_s32 s_q_close(cmr_handle q_handle)
{
	cmr_s32 ret = 0;
	struct queue_context *cxt_ptr = NULL;

	if (NULL == q_handle) {
		ret = -1;
		ISP_LOGE("handle: %p\n", q_handle);
		return ret;
	}

	cxt_ptr = (struct queue_context *)q_handle;

	if (cxt_ptr->q_pool.queue_ptr) {
		free(cxt_ptr->q_pool.queue_ptr);
		cxt_ptr->q_pool.queue_ptr = NULL;
	}

	free(cxt_ptr);
	cxt_ptr = NULL;

	return ret;
}
