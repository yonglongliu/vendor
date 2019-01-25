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
#ifndef _ISP_AWB_QUEUE_H_
#define _ISP_AWB_QUEUE_H_

#include "isp_awb_types.h"
#include <malloc.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AWBL_MAX_QUEUE_SIZE	64

	typedef void *queue_handle_t;

	struct awbl_cyc_queue {
		cmr_u32 q[AWBL_MAX_QUEUE_SIZE];
		cmr_u32 size;
		cmr_u32 cur_index;
		cmr_s32 gradient;
	};

//queue functions
	cmr_s32 _initQueue(struct awbl_cyc_queue *queue, cmr_u32 size);

	void _addToCycQueue(struct awbl_cyc_queue *queue, cmr_u32 value);

	cmr_s32 _isQueueFull(struct awbl_cyc_queue *queue);

	cmr_u32 _calcAvgValueOfQueue(struct awbl_cyc_queue *queue);

	cmr_s32 _calcDeltaValueOfQueue(struct awbl_cyc_queue *queue);

	queue_handle_t queue_init(cmr_u32 size);
	void queue_add(queue_handle_t queue, cmr_u32 value);
	cmr_u32 queue_average(queue_handle_t queue);
	cmr_u32 queue_max(queue_handle_t queue);
	cmr_u32 queue_min(queue_handle_t queue);
	cmr_u32 queue_delta(queue_handle_t queue);
	cmr_u32 queue_statis(queue_handle_t queue, cmr_u32 statis_value);
	cmr_u32 queue_weighted_average(queue_handle_t queue_value, queue_handle_t queue_weight);
	void queue_deinit(queue_handle_t queue);
	void queue_clear(queue_handle_t queue);

#ifdef __cplusplus
}
#endif
#endif
