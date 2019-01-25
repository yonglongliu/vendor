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
#ifndef _AWB_CTRL_TYPES_H_
#define _AWB_CTRL_TYPES_H_
/*------------------------------------------------------------------------------*
*				Dependencies					*
*-------------------------------------------------------------------------------*/
#include "awb_ctrl.h"

enum awb_ctrl_flash_mode {
	AWB_CTRL_FLASH_END = 0x0,
	AWB_CTRL_FLASH_PRE,
	AWB_CTRL_FLASH_MAIN,
	AWB_CTRL_FLASH_MODE_MAX
};

struct awb_flash_info {
	enum awb_ctrl_flash_mode flash_mode;
	struct isp_awb_gain gain_flash_off;
	enum awb_ctrl_flash_status flash_status;
	cmr_u32 ct_flash_off;
};

struct awb_ctrl_recover_param {
	struct isp_awb_gain gain;
	struct isp_awb_gain gain_balanced;
	cmr_u32 ct;
};
#endif
