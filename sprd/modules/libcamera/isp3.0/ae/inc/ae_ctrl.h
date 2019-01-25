/*
 * Copyright (C) 2015 The Android Open Source Project
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

#ifndef _AE_CTRL_H_
#define _AE_CTRL_H_
/*----------------------------------------------------------------------------*
 **				 Dependencies				*
 **---------------------------------------------------------------------------*/
#include "ae_ctrl_types.h"
/**---------------------------------------------------------------------------*
 **				 Compiler Flag				*
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif

cmr_int ae_ctrl_init(struct ae_ctrl_init_in *in_ptr, struct ae_ctrl_init_out *out_ptr, cmr_handle *handle);
cmr_int ae_ctrl_deinit(cmr_handle handle);
cmr_int ae_ctrl_ioctrl(cmr_handle handle, enum ae_ctrl_cmd cmd, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr);
cmr_int ae_ctrl_process(cmr_handle handle, struct ae_ctrl_proc_in *in_ptr, struct ae_ctrl_proc_out *out_ptr);

/**----------------------------------------------------------------------------*
**					Compiler Flag				**
**----------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
#endif

