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
#ifndef _ISP_FILE_DEBUG_H_
#define _ISP_FILE_DEBUG_H_

cmr_int isp_file_ae_save_stats(cmr_handle *handle,
			       cmr_u32 *r_info, cmr_u32 *g_info, cmr_u32 *b_info,
			       cmr_u32 size);
cmr_int isp_file_ebd_save_info(cmr_handle *handle, void *info);

cmr_int isp_file_init(cmr_handle *handle);
cmr_int isp_file_deinit(cmr_handle handle);
#endif
