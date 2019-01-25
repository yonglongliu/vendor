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
#include "sensor_ov13850r2a_raw_param_common.c"
#include "sensor_ov13850r2a_raw_param_prv_0.c"
#include "sensor_ov13850r2a_raw_param_prv_1.c"
#include "sensor_ov13850r2a_raw_param_cap_0.c"
#include "sensor_ov13850r2a_raw_param_cap_1.c"
#include "sensor_ov13850r2a_raw_param_video_0.c"
#include "sensor_ov13850r2a_raw_param_video_1.c"
#include "sensor_ov13850r2a_raw_param_video_2.c"

/* End Include */

//#endif


/************************************************************************/


/* IspToolVersion=1.15.46.2 */


/* Capture Sizes:
	4160x3120,2080x1560,1280x720
*/


/************************************************************************/


static struct sensor_raw_resolution_info_tab s_ov13850r2a_trim_info=
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


static struct sensor_raw_ioctrl s_ov13850r2a_ioctrl=
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
	0
};


/************************************************************************/


static struct sensor_version_info s_ov13850r2a_version_info=
{
	0x00070005,
	{
		{
			0x3331766F,
			0x72303538,
			0x00006132,
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


static uint32_t s_ov13850r2a_libuse_info[]=
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


static struct sensor_raw_info s_ov13850r2a_mipi_raw_info=
{
	&s_ov13850r2a_version_info,
	{
		{s_ov13850r2a_tune_info_common, sizeof(s_ov13850r2a_tune_info_common)},
		{s_ov13850r2a_tune_info_prv_0, sizeof(s_ov13850r2a_tune_info_prv_0)},
		{s_ov13850r2a_tune_info_prv_1, sizeof(s_ov13850r2a_tune_info_prv_1)},
		{NULL, 0},
		{NULL, 0},
		{s_ov13850r2a_tune_info_cap_0, sizeof(s_ov13850r2a_tune_info_cap_0)},
		{s_ov13850r2a_tune_info_cap_1, sizeof(s_ov13850r2a_tune_info_cap_1)},
		{NULL, 0},
		{NULL, 0},
		{s_ov13850r2a_tune_info_video_0, sizeof(s_ov13850r2a_tune_info_video_0)},
		{s_ov13850r2a_tune_info_video_1, sizeof(s_ov13850r2a_tune_info_video_1)},
		{s_ov13850r2a_tune_info_video_2, sizeof(s_ov13850r2a_tune_info_video_2)},
		{NULL, 0},
	},
	&s_ov13850r2a_trim_info,
	&s_ov13850r2a_ioctrl,
	(struct sensor_libuse_info *)s_ov13850r2a_libuse_info,
	{
		&s_ov13850r2a_fix_info_common,
		&s_ov13850r2a_fix_info_prv_0,
		&s_ov13850r2a_fix_info_prv_1,
		NULL,
		NULL,
		&s_ov13850r2a_fix_info_cap_0,
		&s_ov13850r2a_fix_info_cap_1,
		NULL,
		NULL,
		&s_ov13850r2a_fix_info_video_0,
		&s_ov13850r2a_fix_info_video_1,
		&s_ov13850r2a_fix_info_video_2,
		NULL,
	},
	{
		{s_ov13850r2a_common_tool_ui_input, sizeof(s_ov13850r2a_common_tool_ui_input)},
		{s_ov13850r2a_prv_0_tool_ui_input, sizeof(s_ov13850r2a_prv_0_tool_ui_input)},
		{s_ov13850r2a_prv_1_tool_ui_input, sizeof(s_ov13850r2a_prv_1_tool_ui_input)},
		{NULL, 0},
		{NULL, 0},
		{s_ov13850r2a_cap_0_tool_ui_input, sizeof(s_ov13850r2a_cap_0_tool_ui_input)},
		{s_ov13850r2a_cap_1_tool_ui_input, sizeof(s_ov13850r2a_cap_1_tool_ui_input)},
		{NULL, 0},
		{NULL, 0},
		{s_ov13850r2a_video_0_tool_ui_input, sizeof(s_ov13850r2a_video_0_tool_ui_input)},
		{s_ov13850r2a_video_1_tool_ui_input, sizeof(s_ov13850r2a_video_1_tool_ui_input)},
		{s_ov13850r2a_video_2_tool_ui_input, sizeof(s_ov13850r2a_video_2_tool_ui_input)},
		{NULL, 0},
	},
	{
		&s_ov13850r2a_nr_scene_map_param,
		&s_ov13850r2a_nr_level_number_map_param,
		&s_ov13850r2a_default_nr_level_map_param,
	},
};
