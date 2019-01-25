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
#ifndef _ISP_DEBUG_H_
#define _ISP_DEBUG_H_

#include "isp_type.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DEBUG_FILE_BUF_SIZE	4096

	typedef void *debug_handle_t;

	extern debug_handle_t smart_debug_file_init(const char file_name[], const char open_mode[]);
	extern void smart_debug_file_deinit(debug_handle_t handle);
	extern cmr_u32 smart_debug_file_open(debug_handle_t handle);
	extern void smart_debug_file_close(debug_handle_t handle);
	extern void smart_debug_file_print(debug_handle_t handle, char *str);

#ifdef __cplusplus
}
#endif
#endif
