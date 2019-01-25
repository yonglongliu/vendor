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


/************************************************************************/


//#ifdef WIN32

#include "sensor_raw.h"

#define _NR_MAP_PARAM_
#include "isp_nr.h"
#undef _NR_MAP_PARAM_


/* Begin Include */
#include "sensor_sp250a_zrt_raw_param_common.c"
#include "sensor_sp250a_zrt_raw_param_prv_0.c"
#include "sensor_sp250a_zrt_raw_param_cap_0.c"
#include "sensor_sp250a_zrt_raw_param_video_0.c"

/* End Include */

//#endif


/************************************************************************/


/* IspToolVersion=R1.17.4601 */


/* Capture Sizes:
	1600x1200
*/


/************************************************************************/


static struct sensor_raw_resolution_info_tab s_sp250a_zrt_trim_info=
{
	0x00,
	{
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
	}
};


/************************************************************************/


static struct sensor_raw_ioctrl s_sp250a_zrt_ioctrl=
{
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0
};


/************************************************************************/


static struct sensor_version_info s_sp250a_zrt_version_info=
{
	0x00070005,
	{
		{
			0x35327073,
			0x7A5F6130,
			0x00007472,
			0x00000000,
			0x00000000,
			0x00000000,
			0x00000000,
			0x00000000
		}
	},
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000
};


/************************************************************************/


static uint32_t s_sp250a_zrt_libuse_info[]=
{
	0x00000000,0x00000000,0x00000000,0x00000001,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000001,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000
};


/************************************************************************/


static struct sensor_raw_info s_sp250a_zrt_mipi_raw_info=
{
	&s_sp250a_zrt_version_info,
	{
		{s_sp250a_zrt_tune_info_common, sizeof(s_sp250a_zrt_tune_info_common)},
		{s_sp250a_zrt_tune_info_prv_0, sizeof(s_sp250a_zrt_tune_info_prv_0)},
		{NULL, 0},
		{NULL, 0},
		{NULL, 0},
		{s_sp250a_zrt_tune_info_cap_0, sizeof(s_sp250a_zrt_tune_info_cap_0)},
		{NULL, 0},
		{NULL, 0},
		{NULL, 0},
		{s_sp250a_zrt_tune_info_video_0, sizeof(s_sp250a_zrt_tune_info_video_0)},
		{NULL, 0},
		{NULL, 0},
		{NULL, 0},
	},
	&s_sp250a_zrt_trim_info,
	&s_sp250a_zrt_ioctrl,
	(struct sensor_libuse_info *)s_sp250a_zrt_libuse_info,
	{
		&s_sp250a_zrt_fix_info_common,
		&s_sp250a_zrt_fix_info_prv_0,
		NULL,
		NULL,
		NULL,
		&s_sp250a_zrt_fix_info_cap_0,
		NULL,
		NULL,
		NULL,
		&s_sp250a_zrt_fix_info_video_0,
		NULL,
		NULL,
		NULL,
	},
	{
		{s_sp250a_zrt_common_tool_ui_input, sizeof(s_sp250a_zrt_common_tool_ui_input)},
		{s_sp250a_zrt_prv_0_tool_ui_input, sizeof(s_sp250a_zrt_prv_0_tool_ui_input)},
		{NULL, 0},
		{NULL, 0},
		{NULL, 0},
		{s_sp250a_zrt_cap_0_tool_ui_input, sizeof(s_sp250a_zrt_cap_0_tool_ui_input)},
		{NULL, 0},
		{NULL, 0},
		{NULL, 0},
		{s_sp250a_zrt_video_0_tool_ui_input, sizeof(s_sp250a_zrt_video_0_tool_ui_input)},
		{NULL, 0},
		{NULL, 0},
		{NULL, 0},
	},
	{
		&s_sp250a_zrt_nr_scene_map_param,
		&s_sp250a_zrt_nr_level_number_map_param,
		&s_sp250a_zrt_default_nr_level_map_param,
	},
};