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

#include "isp_awb_queue.h"
/**---------------------------------------------------------------------------*
**				Micro Define					*
**----------------------------------------------------------------------------*/
#define ABS(_x)   ((_x) < 0 ? -(_x) : (_x))
/**---------------------------------------------------------------------------*
**				Data Structures					*
**---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
**				extend Variables and function			*
**---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
**				Local Variables					*
**---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
**				Constant Variables					*
**---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
**				Constant Variables				*
**---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
** 				Public Function Prototypes				*
**---------------------------------------------------------------------------*/
cmr_s32 _initQueue(struct awbl_cyc_queue *queue, cmr_u32 size)
{
	if (NULL == queue || size > AWBL_MAX_QUEUE_SIZE) {
		return -1;
	}

	memset(queue->q, 0, sizeof(cmr_u32) * AWBL_MAX_QUEUE_SIZE);

	queue->size = size;
	queue->cur_index = 0;
	queue->gradient = 0;

	return 0;
}

void _addToCycQueue(struct awbl_cyc_queue *queue, cmr_u32 value)
{
	if (NULL == queue)
		return;

	cmr_u32 cur_index = queue->cur_index;
	cmr_u32 *q = queue->q;

	//the queue is full, drop the first one
	if (cur_index == queue->size) {
		cmr_u32 i = 0;

		for (i = 1; i < queue->size; i++) {
			q[i - 1] = q[i];
		}

		cur_index = queue->size - 1;
		q[cur_index] = value;
	} else {
		q[cur_index++] = value;
		queue->cur_index = cur_index;
	}
}

cmr_s32 _isQueueFull(struct awbl_cyc_queue *queue)
{
	if (NULL == queue)
		return -1;

	if (queue->cur_index < queue->size)
		return 0;
	else
		return 1;
}

cmr_u32 _calcAvgValueOfQueue(struct awbl_cyc_queue * queue)
{
	cmr_u32 avg = 0;
	cmr_u32 sum = 0;
	cmr_s32 i = 0;
	cmr_u32 *q = NULL;
	cmr_s32 size = 0;

	if (NULL == queue)
		return 0;

	q = queue->q;

	size = (queue->cur_index < queue->size)
		? queue->cur_index : queue->size;

	if (0 == size)
		return 0;

	for (i = 0; i < size; i++) {
		sum += q[i];
	}

	avg = sum / size;

	return avg;
}

cmr_u32 _calcMaxValueOfQueue(struct awbl_cyc_queue * queue)
{
	cmr_s32 size = 0;
	cmr_s32 i = 0;
	cmr_u32 max = 0;
	cmr_u32 *q = NULL;

	if (NULL == queue)
		return 0;

	q = queue->q;

	size = (queue->cur_index < queue->size) ? queue->cur_index : queue->size;

	if (0 == size)
		return 0;

	for (i = 0; i < size; i++) {
		if (q[i] > max) {
			max = q[i];
		}
	}

	return max;
}

cmr_u32 _calcMinValueOfQueue(struct awbl_cyc_queue * queue)
{
	cmr_s32 size = 0;
	cmr_s32 i = 0;
	cmr_u32 min = 0;
	cmr_u32 *q = NULL;

	if (NULL == queue)
		return 0;

	q = queue->q;

	size = (queue->cur_index < queue->size) ? queue->cur_index : queue->size;

	if (0 == size)
		return 0;

	for (i = 0; i < size; i++) {
		if (q[i] < min) {
			min = q[i];
		}
	}

	return min;
}

cmr_s32 _calcDeltaValueOfQueue(struct awbl_cyc_queue * queue)
{
	cmr_s32 i = 0;
	cmr_u32 *q = NULL;
	cmr_s32 size = 0;
	cmr_s32 delta = 0;

	if (NULL == queue)
		return 0;

	q = queue->q;

	size = (queue->cur_index < queue->size)
		? queue->cur_index : queue->size;

	if (size < 2)
		return 0;

	for (i = 1; i < size; i++) {
		delta += (cmr_s32) q[i] - (cmr_s32) q[i - 1];
	}

	return delta;
}

cmr_s32 _calcDeltaOfQueue(struct awbl_cyc_queue * queue)
{
	cmr_s32 size = 0;
	cmr_u32 delta = 0;
	cmr_u32 *q = NULL;
	if (NULL == queue)
		return 0;

	q = queue->q;

	size = (queue->cur_index < queue->size) ? queue->cur_index : queue->size;

	if (size < 2)
		return 0;

	delta = ABS((cmr_s32) q[size - 1] - (cmr_s32) q[size - 2]);

	return delta;
}

cmr_u32 _calcNumOfQueue(struct awbl_cyc_queue * queue, cmr_u32 statis_value)
{
	cmr_s32 size = 0;
	cmr_s32 i = 0;
	cmr_u32 *q = NULL;
	cmr_u32 num = 0;

	if (NULL == queue)
		return 0;
	q = queue->q;

	size = (queue->cur_index < queue->size) ? queue->cur_index : queue->size;
	if (0 == size)
		return 0;

	for (i = 0; i < size; i++) {
		if (statis_value == q[i]) {
			num++;
		}
	}

	return num;
}

cmr_u32 _calc_weighted_average(struct awbl_cyc_queue * queue_value, struct awbl_cyc_queue * queue_weight)
{
	cmr_u32 avg = 0;
	cmr_u32 sum = 0;
	cmr_s32 i = 0;
	cmr_u32 *qv = NULL;
	cmr_u32 *qw = NULL;
	cmr_s32 size_v = 0;
	cmr_s32 size_w = 0;
	cmr_u32 weight_sum = 0;

	if (NULL == queue_value || NULL == queue_weight)
		return 0;

	qv = queue_value->q;
	qw = queue_weight->q;

	size_v = (queue_value->cur_index < queue_value->size)
		? queue_value->cur_index : queue_value->size;

	size_w = (queue_weight->cur_index < queue_weight->size)
		? queue_weight->cur_index : queue_weight->size;

	if (size_v != size_w || 0 == size_w)
		return 0;

	for (i = 0; i < size_w; i++) {
		qw[i] = 256;
	}

	if (size_w > 1) {

		qw[0] = qw[0] - 60;
		qw[size_w - 1] = qw[size_w - 1] + 60;

	}

	for (i = 0; i < size_v; i++) {
		sum += qv[i] * qw[i];
		weight_sum += qw[i];
	}

	if (weight_sum > 0)
		avg = sum / weight_sum;

	return avg;
}

void _clear(struct awbl_cyc_queue *queue)
{
	if (NULL == queue)
		return;

	queue->cur_index = 0;
}

queue_handle_t queue_init(cmr_u32 size)
{
	struct awbl_cyc_queue *queue = NULL;

	if (0 == size || size > AWBL_MAX_QUEUE_SIZE)
		return 0;

	queue = (struct awbl_cyc_queue *)malloc(sizeof(*queue));
	if (NULL == queue)
		return 0;

	_initQueue(queue, size);

	return (queue_handle_t) queue;
}

void queue_add(queue_handle_t queue, cmr_u32 value)
{
	_addToCycQueue((struct awbl_cyc_queue *)queue, value);
}

cmr_u32 queue_average(queue_handle_t queue)
{
	return _calcAvgValueOfQueue((struct awbl_cyc_queue *)queue);
}

cmr_u32 queue_max(queue_handle_t queue)
{
	return _calcMaxValueOfQueue((struct awbl_cyc_queue *)queue);
}

cmr_u32 queue_min(queue_handle_t queue)
{
	return _calcMinValueOfQueue((struct awbl_cyc_queue *)queue);
}

cmr_u32 queue_delta(queue_handle_t queue)
{
	return _calcDeltaOfQueue((struct awbl_cyc_queue *)queue);
}

cmr_u32 queue_weighted_average(queue_handle_t queue_value, queue_handle_t queue_weight)
{
	return _calc_weighted_average((struct awbl_cyc_queue *)queue_value, (struct awbl_cyc_queue *)queue_weight);
}

cmr_u32 queue_statis(queue_handle_t queue, cmr_u32 statis_value)
{
	return _calcNumOfQueue((struct awbl_cyc_queue *)queue, statis_value);
}

void queue_clear(queue_handle_t queue)
{
	_clear((struct awbl_cyc_queue *)queue);
}

void queue_deinit(queue_handle_t queue)
{
	if (NULL != (struct awbl_cyc_queue *)queue) {
		free((struct awbl_cyc_queue *)queue);
		queue = 0;
	}
}

/**---------------------------------------------------------------------------*/
