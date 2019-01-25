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


#ifdef WIN32
#include "sensor_raw.h"
#endif


/************************************************************************/


/* IspToolVersion=1.1.1.3 */


/* Capture Sizes:
	4208x3120
*/


/************************************************************************/


static uint8_t s_s5k3p8sm_tune_info_common[]=
{
#if 1 /* version_id=0x00030002, mode_name=COMMON, mode_id=0, size=4208x3120 */
#endif /* SMART END */

};


/************************************************************************/



static uint8_t s_s5k3p8sm_tune_info_prv_0[]=
{
#if 1 /* version_id=0x00030002, mode_name=PRV_0, mode_id=1, size=4208x3120 */
#endif /* PRV_0 HEAD END */
#if 1 /* mode_name=PRV_0, block_name=PREGLBG, version_id=0, param_id=0 */
#endif /* PREGLBG END */

};


static uint8_t s_s5k3p8sm_tune_info_cap_0[]=
{
#if 1 /* version_id=0x00030002, mode_name=CAP_0, mode_id=5, size=4208x3120 */
#endif /* SMART END */

};


static uint8_t s_s5k3p8sm_tune_info_video_0[]=
{
#if 1 /* version_id=0x00030002, mode_name=VIDEO_0, mode_id=9, size=4208x3120 */
#endif /* AE END */

};

static struct sensor_raw_resolution_info_tab s_s5k3p8sm_trim_info=
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


static struct sensor_raw_ioctrl s_s5k3p8sm_ioctrl=
{
	0,
	0,
	0,
	0,
	0,
	0,
};


/************************************************************************/


static struct sensor_version_info s_s5k3p8sm_version_info=
{
	0x00030002,
	sizeof(struct sensor_version_info),
	0x00
};


/************************************************************************/


static struct sensor_raw_info s_s5k3p8sm_mipi_raw_info=
{
	&s_s5k3p8sm_version_info,

	{
		{s_s5k3p8sm_tune_info_common, sizeof(s_s5k3p8sm_tune_info_common)},
		{s_s5k3p8sm_tune_info_prv_0, sizeof(s_s5k3p8sm_tune_info_prv_0)},
		{NULL, 0},
		{NULL, 0},
		{NULL, 0},
		{s_s5k3p8sm_tune_info_cap_0, sizeof(s_s5k3p8sm_tune_info_cap_0)},
		{NULL, 0},
		{NULL, 0},
		{NULL, 0},
		{s_s5k3p8sm_tune_info_video_0, sizeof(s_s5k3p8sm_tune_info_video_0)},
		{NULL, 0},
		{NULL, 0},
		{NULL, 0},
	},

	&s_s5k3p8sm_trim_info,
	&s_s5k3p8sm_ioctrl
};


/************************************************************************/


/* common Normal AE0 Parameter:
	LineTime=187, MinLine=4, MaxGain=8.0, NormalMinFps50Hz=10.0, NormalMinFps60Hz=10.0
*/
