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
#ifndef _ISP_VIDEO_H
#define _ISP_VIDEO_H

#include <stdio.h>

#define TOOL_HW_VER 0x101
#define TOOL_SW_VER 0x10
#define TOOL_DEFAULT_VER (TOOL_HW_VER<<16)|(TOOL_SW_VER)

#define ISP_TOOL_YUV422_2FRAME (1<<0)
#define ISP_TOOL_YUV420_2FRAME (1<<1)
#define ISP_TOOL_NORMAL_RAW10 (1<<2)
#define ISP_TOOL_MIPI_RAW10 (1<<3)
#define ISP_TOOL_JPG (1<<4)
#define ISP_TOOL_YVU422_2FRAME (1<<5)
#define ISP_TOOL_YVU420_2FRAME (1<<6)

enum {
	REG_START_PREVIEW = 1,
	REG_STOP_PREVIEW,
	REG_TAKE_PICTURE,
	REG_SET_PARAM,
	REG_CTRL_FLASH,
	REG_MAX,
};
extern char raw_filename[200];
extern cmr_u32 tool_fmt_pattern;
extern cmr_u8 nr_tool_flag[17];
extern struct denoise_param_update nr_update_param;

cmr_s32 ispvideo_RegCameraFunc(cmr_u32 cmd, cmr_s32(*func) (cmr_u32, cmr_u32));
void send_img_data(cmr_u32 format, cmr_u32 width, cmr_u32 height, char *imgptr, cmr_s32 imagelen);
void send_capture_data(cmr_u32 format, cmr_u32 width, cmr_u32 height, char *ch0_ptr, cmr_s32 ch0_len, char *ch1_ptr, cmr_s32 ch1_len, char *ch2_ptr, cmr_s32 ch2_len);
void startispserver(cmr_u32 cam_id);
void stopispserver();
void setispserver(void *handle);
void send_capture_complete_msg();
void *ispvideo_GetIspHandle(void);
#endif
