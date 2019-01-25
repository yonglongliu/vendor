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

#include "isp_mlog.h"
#include <cutils/properties.h>
#include <stdarg.h>

#define MLOG_BUF_SIZE 1024
#define MLOG_FILE_NAME_SIZE 80

void isp_mlog(enum mlog_file_type name, const char *format, ...)
{
	char value[PROPERTY_VALUE_MAX];
	va_list ap;
	char buf[MLOG_BUF_SIZE];
	FILE *fp = NULL;
	char file_name[MLOG_FILE_NAME_SIZE] = {0};

	switch(name) {
	case AF_FILE:
		property_get(AF_SAVE_MLOG_STR, value, "no");
		sprintf(file_name, "/data/mlog/%s.txt", "af");
		break;
	case AWB_FILE:
		property_get(AWB_SAVE_MLOG_STR, value, "no");
		sprintf(file_name, "/data/mlog/%s.txt", "awb");
		break;
	case AE_FILE:
		property_get(AE_SAVE_MLOG_STR, value, "no");
		sprintf(file_name, "/data/mlog/%s.txt", "ae");
		break;
	case IRP_FILE:
		property_get(IRP_SAVE_MLOG_STR, value, "no");
		sprintf(file_name, "/data/mlog/%s.txt", "irp");
		break;
	case SHADING_FILE:
		property_get(SHADING_SAVE_MLOG_STR, value, "no");
		sprintf(file_name, "/data/mlog/%s.txt", "shading");
		break;
	default:
		break;
	}

	if (!strcmp(value, "save")) {
		fp = fopen(file_name, "wb");
		if (fp) {
			memset(buf, 0, sizeof(buf));
			va_list ap;
			va_start(ap, format);
			vsnprintf(buf, MLOG_BUF_SIZE, format, ap);
			va_end(ap);
			fwrite((void*)buf, 1, strlen(buf), fp);
			fclose(fp);
			fp = NULL;
		}
	}
}
