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

#ifndef MAX
#define MAX(x, y) (((x) > (y))?(x):(y))
#endif

struct seq_q_head {
	cmr_u32 cur_idx;
	cmr_u32 max_num;
	struct seq_cell *cell_ptr;
};

struct seq_cxt {
	cmr_u32 queue_num;
	struct seq_init_in init_in_param;
	struct seq_q_head write_q;
	struct seq_q_head actual_q;

	cmr_u32 max_valid_num;
	cmr_u32 is_first_exp;
	cmr_u32 valid_offset_num;

	struct seq_cell pre_cell;
};

static cmr_int seq_get_cur_actual_element(struct seq_cxt *cxt_ptr, struct seq_cell *out_ptr)
{
	cmr_int ret = -1;

	cmr_u32 frame_id = 0;
	cmr_u32 cur_idx = 0;


	if (NULL == cxt_ptr
		|| NULL == out_ptr) {
		goto exit;
	}

	cur_idx = cxt_ptr->actual_q.cur_idx;
	*out_ptr = *(cxt_ptr->actual_q.cell_ptr + cur_idx);
	return 0;
exit:
	return ret;
}

static cmr_int seq_push_actual_element(struct seq_cxt *cxt_ptr, cmr_u32 is_add_idx, cmr_u32 offset, struct seq_cell *in_ptr)
{
	cmr_int ret = -1;

	cmr_u32 frame_id = 0;
	cmr_u32 cur_idx = 0;
	cmr_u32 offset_idx = 0;


	if (NULL == cxt_ptr
		|| NULL == in_ptr) {
		goto exit;
	}

	cur_idx = cxt_ptr->actual_q.cur_idx;
	offset_idx = cur_idx + offset;

	offset_idx = offset_idx % cxt_ptr->actual_q.max_num;
	if (in_ptr->dummy)
		(cxt_ptr->actual_q.cell_ptr + offset_idx)->dummy = in_ptr->dummy;
	if (in_ptr->exp_time)
		(cxt_ptr->actual_q.cell_ptr + offset_idx)->exp_time = in_ptr->exp_time;
	if (in_ptr->exp_line)
		(cxt_ptr->actual_q.cell_ptr + offset_idx)->exp_line = in_ptr->exp_line;
	if (in_ptr->gain)
		(cxt_ptr->actual_q.cell_ptr + offset_idx)->gain = in_ptr->gain;

	if (is_add_idx) {
		cur_idx++;
		cur_idx = cur_idx % cxt_ptr->actual_q.max_num;
		cxt_ptr->actual_q.cur_idx = cur_idx;
	}
	return 0;
exit:
	return ret;
}

static cmr_int seq_get_cur_write_element(struct seq_cxt *cxt_ptr, struct seq_cell *out_ptr)
{
	cmr_int ret = -1;

	cmr_u32 frame_id = 0;
	cmr_u32 cur_idx = 0;


	if (NULL == cxt_ptr
		|| NULL == out_ptr) {
		goto exit;
	}

	cur_idx = cxt_ptr->write_q.cur_idx;
	*out_ptr = *(cxt_ptr->write_q.cell_ptr + cur_idx);
	return 0;
exit:
	return ret;
}

static cmr_int seq_push_write_element(struct seq_cxt *cxt_ptr, cmr_u32 is_add_idx, cmr_u32 offset, struct seq_cell *in_ptr)
{
	cmr_int ret = -1;

	cmr_u32 frame_id = 0;
	cmr_u32 cur_idx = 0;
	cmr_u32 offset_idx = 0;


	if (NULL == cxt_ptr
		|| NULL == in_ptr) {
		goto exit;
	}

	cur_idx = cxt_ptr->write_q.cur_idx;
	offset_idx = cur_idx + offset;

	offset_idx = offset_idx % cxt_ptr->write_q.max_num;
	if (in_ptr->dummy)
		(cxt_ptr->write_q.cell_ptr + offset_idx)->dummy = in_ptr->dummy;
	if (in_ptr->exp_time)
		(cxt_ptr->write_q.cell_ptr + offset_idx)->exp_time = in_ptr->exp_time;
	if (in_ptr->exp_line)
		(cxt_ptr->write_q.cell_ptr + offset_idx)->exp_line = in_ptr->exp_line;
	if (in_ptr->gain)
		(cxt_ptr->write_q.cell_ptr + offset_idx)->gain = in_ptr->gain;

	if (is_add_idx) {
		cur_idx++;
		cur_idx = cur_idx % cxt_ptr->write_q.max_num;
		cxt_ptr->write_q.cur_idx = cur_idx;
	}
	return 0;
exit:
	return ret;
}

cmr_int seq_init(cmr_u32 queue_num, struct seq_init_in *in_ptr, void **handle)
{
	cmr_int ret = -1;
	struct seq_cxt *cxt_ptr = NULL;
	cmr_int q_size = 0;


	if ((queue_num < 4)
		|| NULL == in_ptr
		|| NULL == handle) {
		goto exit;
	}
	*handle = NULL;
	cxt_ptr = (struct seq_cxt *)malloc(sizeof(*cxt_ptr));
	if (NULL == cxt_ptr) {
		goto exit;
	}
	memset(cxt_ptr, 0, sizeof(*cxt_ptr));

	cxt_ptr->queue_num = queue_num;
	cxt_ptr->init_in_param = *in_ptr;

	cxt_ptr->write_q.max_num = queue_num;

	q_size = queue_num * sizeof(struct seq_cell);
	cxt_ptr->write_q.cell_ptr = (struct seq_cell *)malloc(q_size);
	if (NULL == cxt_ptr->write_q.cell_ptr)
		goto exit;
	memset(cxt_ptr->write_q.cell_ptr, 0, q_size);

	cxt_ptr->actual_q.max_num = queue_num;
	cxt_ptr->actual_q.cell_ptr = (struct seq_cell *)malloc(q_size);
	if (NULL == cxt_ptr->actual_q.cell_ptr)
		goto exit;
	memset(cxt_ptr->actual_q.cell_ptr, 0, q_size);

	cxt_ptr->max_valid_num = MAX(in_ptr->exp_valid_num, in_ptr->gain_valid_num);

	if (in_ptr->exp_valid_num > in_ptr->gain_valid_num) {
		cxt_ptr->is_first_exp = 1;
		cxt_ptr->valid_offset_num = in_ptr->exp_valid_num - in_ptr->gain_valid_num;
	} else {
		cxt_ptr->valid_offset_num = in_ptr->gain_valid_num - in_ptr->exp_valid_num;
	}

	*handle = (void *)cxt_ptr;
	return 0;
exit:
	if (cxt_ptr->write_q.cell_ptr)
		free(cxt_ptr->write_q.cell_ptr);
	if (cxt_ptr->actual_q.cell_ptr)
		free(cxt_ptr->actual_q.cell_ptr);
	if (cxt_ptr)
		free(cxt_ptr);
	return ret;
}

cmr_int seq_deinit(void *handle)
{
	cmr_int ret = -1;
	struct seq_cxt *cxt_ptr = NULL;


	if (NULL == handle)
		goto exit;

	cxt_ptr = (struct seq_cxt *)handle;
	if (cxt_ptr->write_q.cell_ptr)
		free(cxt_ptr->write_q.cell_ptr);
	if (cxt_ptr->actual_q.cell_ptr)
		free(cxt_ptr->actual_q.cell_ptr);
	free(cxt_ptr);
	return 0;
exit:
	return ret;
}

cmr_int seq_reset(void *handle)
{
	cmr_int ret = 0;
	struct seq_cxt *cxt_ptr = NULL;
	cmr_int q_size = 0;

	cxt_ptr = (struct seq_cxt *)handle;

	if (NULL == cxt_ptr
			|| NULL == cxt_ptr->write_q.cell_ptr
			|| NULL == cxt_ptr->actual_q.cell_ptr) {
		goto exit;
	}

	q_size = cxt_ptr->queue_num * sizeof(struct seq_cell);
	memset(cxt_ptr->write_q.cell_ptr, 0, q_size);
	cxt_ptr->write_q.cur_idx = 0;
	memset(cxt_ptr->actual_q.cell_ptr, 0, q_size);
	cxt_ptr->actual_q.cur_idx = 0;

exit:
	return ret;
}

cmr_int seq_put(void *handle, struct seq_item *in_est_ptr, struct seq_cell *out_actual_ptr, struct seq_cell *out_write_ptr)
{
	cmr_int ret = -1;
	struct seq_cxt *cxt_ptr = NULL;
	cmr_u32 cur_frame_id = 0;
	cmr_u32 skip_num = 0;
	cmr_u32 skip_offset_num = 0;
	cmr_u32 exp_valid_num = 0;
	cmr_u32 gain_valid_num = 0;
	cmr_u32 max_valid_num = 0;
	cmr_u32 offset_num = 0;
	cmr_u32 i = 0;

	struct seq_cell cur_write_cell;
	struct seq_cell push_write_cell;
	struct seq_cell push_nxt_write_cell;

	struct seq_cell cur_actual_cell;
	struct seq_cell push_nxt_actual_cell;


	if (NULL == handle
		|| NULL == in_est_ptr
		|| NULL == out_actual_ptr
		|| NULL == out_write_ptr) {
		goto exit;
	}
	cxt_ptr = (struct seq_cxt *)handle;

	cur_frame_id = in_est_ptr->cell.frame_id;
	if (SEQ_WORK_CAPTURE == in_est_ptr->work_mode)
		skip_num = cxt_ptr->init_in_param.capture_skip_num;
	else if (SEQ_WORK_PREVIEW == in_est_ptr->work_mode)
		skip_num = cxt_ptr->init_in_param.preview_skip_num;

	skip_offset_num = (skip_num > cxt_ptr->max_valid_num)?0:(cxt_ptr->max_valid_num - skip_num);
	if(skip_offset_num > 0) {
		skip_offset_num = skip_offset_num - 1;
	}
	exp_valid_num = cxt_ptr->init_in_param.exp_valid_num;
	gain_valid_num = cxt_ptr->init_in_param.gain_valid_num;
	max_valid_num = cxt_ptr->max_valid_num;

	memset(&push_nxt_actual_cell, 0, sizeof(push_nxt_actual_cell));
	memset(&push_nxt_write_cell, 0, sizeof(push_nxt_write_cell));

	if (cxt_ptr->init_in_param.idx_start_from == cur_frame_id) {
		for (i = skip_offset_num; i <= max_valid_num; i++) {
			push_nxt_actual_cell = in_est_ptr->cell;
			ret = seq_push_actual_element(cxt_ptr, 0, i, &push_nxt_actual_cell);
			if (ret)
				goto exit;
		}
	}

	ret = seq_get_cur_actual_element(cxt_ptr, &cur_actual_cell);
	if (ret)
		goto exit;

	ret = seq_get_cur_write_element(cxt_ptr, &cur_write_cell);
	if (ret)
		goto exit;

	offset_num = cxt_ptr->valid_offset_num;
	if (0 == offset_num) {
		/*same valid frame*/
		push_write_cell.frame_id = cur_frame_id;
		push_write_cell.dummy = in_est_ptr->cell.dummy;
		push_write_cell.exp_time = in_est_ptr->cell.exp_time;
		push_write_cell.exp_line = in_est_ptr->cell.exp_line;
		push_write_cell.gain = in_est_ptr->cell.gain;
	} else {
		/*different valid frame*/
		if (cxt_ptr->is_first_exp) {
			push_write_cell.frame_id = cur_frame_id;
			push_write_cell.dummy = in_est_ptr->cell.dummy;
			push_write_cell.exp_time = in_est_ptr->cell.exp_time;
			push_write_cell.exp_line = in_est_ptr->cell.exp_line;
			push_write_cell.gain = cur_write_cell.gain;

			push_nxt_write_cell.gain = in_est_ptr->cell.gain;

			if (cxt_ptr->pre_cell.exp_line == in_est_ptr->cell.exp_line) {
				push_write_cell.gain = push_nxt_write_cell.gain;
				for (i = 0; i < offset_num; i++) {
					ret = seq_push_write_element(cxt_ptr, 0, i, &push_nxt_write_cell);
					if (ret)
						goto exit;
				}
				for (i = gain_valid_num + 1; i < max_valid_num + 1; i++) {
					ret = seq_push_actual_element(cxt_ptr, 0, i, &push_nxt_write_cell);
					if (ret)
						goto exit;
				}
			}
		} else {
			push_write_cell.frame_id = cur_frame_id;
			push_write_cell.gain = in_est_ptr->cell.gain;
			push_write_cell.exp_line = cur_write_cell.exp_line;
			push_write_cell.exp_time = cur_write_cell.exp_time;
			push_write_cell.dummy = cur_write_cell.dummy;

			push_nxt_write_cell.exp_line = in_est_ptr->cell.exp_line;
			push_nxt_write_cell.exp_time = in_est_ptr->cell.exp_time;
			push_nxt_write_cell.dummy = in_est_ptr->cell.dummy;
		}
	}

	ret = seq_push_write_element(cxt_ptr, 1, offset_num, &push_nxt_write_cell);
	if (ret)
		goto exit;

	offset_num = max_valid_num + 1;
	push_nxt_actual_cell = in_est_ptr->cell;
	ret = seq_push_actual_element(cxt_ptr, 1, offset_num, &push_nxt_actual_cell);
	if (ret)
		goto exit;

	cxt_ptr->pre_cell = in_est_ptr->cell;


	*out_write_ptr = push_write_cell;
	*out_actual_ptr = cur_actual_cell;
	return 0;
exit:
	return ret;
}
