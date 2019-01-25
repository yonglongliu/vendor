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

#ifndef _ISP_MLOG_H_
#define _ISP_MLOG_H_
#include "isp_type.h"
#define AF_SAVE_MLOG_STR       "persist.sys.isp.af.mlog" /*save/no*/
#define AE_SAVE_MLOG_STR       "persist.sys.isp.ae.mlog" /*save/no*/
#define AWB_SAVE_MLOG_STR      "persist.sys.isp.awb.mlog" /*save/no*/
#define IRP_SAVE_MLOG_STR      "persist.sys.isp.irp.mlog" /*save/no*/
#define SHADING_SAVE_MLOG_STR  "persist.sys.isp.shading.mlog" /*save/no*/

enum mlog_file_type {
	AF_FILE,
	AWB_FILE,
	AE_FILE,
	IRP_FILE,
	SHADING_FILE,
};

void isp_mlog(enum mlog_file_type name,const char *format, ...);
#endif
