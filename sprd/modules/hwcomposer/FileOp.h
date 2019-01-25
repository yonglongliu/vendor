/*
 * Copyright (C) 2007 The Android Open Source Project
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

#ifndef ANDROID_FILEOP_H
#define ANDROID_FILEOP_H

#include <cutils/log.h>

#include <malloc.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>

namespace android {

class FileOp
{
public:
    inline FileOp() { }
    inline ~FileOp() { }

	int SetFPS(unsigned int fps)
	{
		char value[20]; /* large enough for millions of years */
		//char devicepath[] = "/sys/devices/sprd-adf/dispc0/dynamic_fps";
		//char devicepath[] = "/sys/devices/platform/soc/soc:mm/d3200000.dispc/vsync_rate_report";
		char devicepath[] = "/sys/class/display/dispc0/vsync_rate_report";

		snprintf(value, sizeof(value), "%u", fps);
		return write_value(devicepath, value);
	}

private:
	int write_value(const char *file, const char *value)
	{
		int to_write, written, ret, fd;

		fd = TEMP_FAILURE_RETRY(open(file, O_WRONLY));
		if (fd < 0) {
			ALOGE("open file faile fd < 0");
			return -errno;
		}

		to_write = strlen(value) + 1;
		written = TEMP_FAILURE_RETRY(write(fd, value, to_write));
		if (written == -1) {
			ALOGE("openfile write faile return value -1");
			ret = -errno;
		} else if (written != to_write) {
			/* even though EAGAIN is an errno value that could be set
			   by write() in some cases, none of them apply here.  So, this return
			   value can be clearly identified when debugging and suggests the
			   caller that it may try to call vibrator_on() again */
			ret = -EAGAIN;
			ALOGE("openfile write faile written != to_write");
		} else {
			ret = 0;
		}

		errno = 0;
		close(fd);

		return ret;
	}

};

}; // namespace android

#endif // ANDROID_FILEOP_H

