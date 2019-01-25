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
#include "sensor_gc030a_raw_param_common.c"
#include "sensor_gc030a_raw_param_prv_0.c"
#include "sensor_gc030a_raw_param_cap_0.c"
#include "sensor_gc030a_raw_param_video_0.c"

/* End Include */

//#endif


/************************************************************************/


/* IspToolVersion=R1.17.0501 */


/* Capture Sizes:
	640x480
*/


/************************************************************************/


static struct sensor_raw_resolution_info_tab s_gc030a_trim_info=
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


static struct sensor_raw_ioctrl s_gc030a_ioctrl=
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


static struct sensor_version_info s_gc030a_version_info=
{
	0x00070005,
	{
		{
			0x33306367,
			0x00006130,
			0x00000000,
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


static uint32_t s_gc030a_libuse_info[]=
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


static struct sensor_raw_info s_gc030a_mipi_raw_info=
{
	&s_gc030a_version_info,
	{
		{s_gc030a_tune_info_common, sizeof(s_gc030a_tune_info_common)},
		{s_gc030a_tune_info_prv_0, sizeof(s_gc030a_tune_info_prv_0)},
		{NULL, 0},
		{NULL, 0},
		{NULL, 0},
		{s_gc030a_tune_info_cap_0, sizeof(s_gc030a_tune_info_cap_0)},
		{NULL, 0},
		{NULL, 0},
		{NULL, 0},
		{s_gc030a_tune_info_video_0, sizeof(s_gc030a_tune_info_video_0)},
		{NULL, 0},
		{NULL, 0},
		{NULL, 0},
	},
	&s_gc030a_trim_info,
	&s_gc030a_ioctrl,
	(struct sensor_libuse_info *)s_gc030a_libuse_info,
	{
		&s_gc030a_fix_info_common,
		&s_gc030a_fix_info_prv_0,
		NULL,
		NULL,
		NULL,
		&s_gc030a_fix_info_cap_0,
		NULL,
		NULL,
		NULL,
		&s_gc030a_fix_info_video_0,
		NULL,
		NULL,
		NULL,
	},
	{
		{s_gc030a_common_tool_ui_input, sizeof(s_gc030a_common_tool_ui_input)},
		{s_gc030a_prv_0_tool_ui_input, sizeof(s_gc030a_prv_0_tool_ui_input)},
		{NULL, 0},
		{NULL, 0},
		{NULL, 0},
		{s_gc030a_cap_0_tool_ui_input, sizeof(s_gc030a_cap_0_tool_ui_input)},
		{NULL, 0},
		{NULL, 0},
		{NULL, 0},
		{s_gc030a_video_0_tool_ui_input, sizeof(s_gc030a_video_0_tool_ui_input)},
		{NULL, 0},
		{NULL, 0},
		{NULL, 0},
	},
	{
		&s_gc030a_nr_scene_map_param,
		&s_gc030a_nr_level_number_map_param,
		&s_gc030a_default_nr_level_map_param,
	},
};
