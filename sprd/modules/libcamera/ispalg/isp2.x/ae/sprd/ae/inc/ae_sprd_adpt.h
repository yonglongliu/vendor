/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _AE_CTRL_V2_H_
#define _AE_CTRL_V2_H_

#include "ae_tuning_type.h"
#include "ae_ctrl_types.h"

#ifdef __cplusplus
extern "C" {
#endif

	cmr_handle ae_sprd_init(cmr_handle in_param, cmr_handle out_param);
	cmr_s32 ae_sprd_deinit(cmr_handle handler, cmr_handle in_param, cmr_handle out_param);
	cmr_s32 ae_sprd_calculation(cmr_handle handler, cmr_handle in_param, cmr_handle out_param);
	cmr_s32 ae_sprd_io_ctrl(cmr_handle handler, cmr_s32 cmd, cmr_handle in_param, cmr_handle out_param);

#ifdef __cplusplus
}
#endif
#endif
