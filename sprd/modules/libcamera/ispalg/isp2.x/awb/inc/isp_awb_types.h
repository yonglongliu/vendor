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
#ifndef _ISP_AWB_TYPES_
#define _ISP_AWB_TYPES_

#ifdef WIN32
#include <stdlib.h>
#include <stdio.h>
#include "sci_types.h"
#else
#include <linux/types.h>
#include <sys/types.h>
#include <utils/Log.h>
#include "isp_type.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define AWB_SUCCESS				0
#define AWB_E_NO_WHITE_BLOCK 			1
#define AWB_ERROR				255

#define AWB_MAX_INSTANCE_NUM			4
#define AWB_MAX_PIECEWISE_NUM			16

#define AWB_TRUE	1
#define AWB_FALSE	0

	typedef void *awb_handle_t;

	enum awb_cmd {
		AWB_CMD_SET_BASE = 0x1000,
		AWB_CMD_SET_INIT_PARAM = (AWB_CMD_SET_BASE + 1),
		AWB_CMD_GET_BASE = 0x2000,
		AWB_CMD_GET_INIT_PARAM = (AWB_CMD_GET_BASE + 1),
		AWB_CMD_GET_CALC_DETAIL = (AWB_CMD_GET_BASE + 2),

		AWB_CMD_UNKNOWN = 0xffffffff
	};

	struct awb_size {
		cmr_u16 w;
		cmr_u16 h;
	};

	struct awb_range {
		cmr_u16 min;
		cmr_u16 max;
	};

	struct awb_point {
		cmr_u16 x;
		cmr_u16 y;
	};

	struct awb_rect {
		struct awb_point begin;
		struct awb_point end;
	};

	struct awb_gain {
		cmr_u16 r;
		cmr_u16 g;
		cmr_u16 b;
	};

	struct awb_save_gain {
		cmr_u16 r;
		cmr_u16 g;
		cmr_u16 b;
		cmr_u16 ct;
	};
	struct awb_linear_func {
		cmr_s32 a;
		cmr_s32 b;
		cmr_u32 shift;
	};

	struct awb_sample {
		/* input value of the sample point */
		cmr_s32 x;
		/* output value of the sample point */
		cmr_s32 y;
	};

/* piecewise linear function */
	struct awb_piecewise_func {
		/* sample points of the piecewise linear function, the x value
		   of the samples must be in an ascending sort  order */
		struct awb_sample samples[AWB_MAX_PIECEWISE_NUM];
		/* number of the samples */
		cmr_u16 num;
		cmr_u16 base;
	};

/* scanline */
	struct awb_scanline {
		cmr_u16 y;
		cmr_u16 x0;
		cmr_u16 x1;
	};

/* scanline of the window */
	struct awb_win_scanline {
		/* scanline number */
		cmr_u16 num;
		struct awb_scanline scanline;
	};

#ifdef __cplusplus
}
#endif
#endif
