/*
 * Copyright (C) 2012 Spreadtrum Communications Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <getopt.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include "eng_diag.h"
#include "eng_modules.h"

#if defined(ANDROID)
#include <android/log.h>
#include <utils/Log.h>

#undef LOG_TAG
#define LOG_TAG "sprd_dpu_hal"

#define LOGI(fmt, args...) \
  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, fmt, ##args)
#define LOGD(fmt, args...) \
  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##args)
#define LOGE(fmt, args...) \
  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, fmt, ##args)
#endif

#define DEV_NODE "sys/class/display/panel0/name"
#define DATA_NODE "/data/lcdid"
#define UID_LENGTH 32

static int panel_id_file_read(char *buffer, int len)
{
    int fd = -1, ret = 0;

    if (buffer == NULL)
        return -1;

    fd = open(DEV_NODE, O_RDWR);
    if (fd < 0) {
        ALOGE("%s()->Line:%d; read info(%d) data error, retcode = %d; \n",
                __FUNCTION__, __LINE__, ret);
	fd = open(DATA_NODE, O_RDWR);
	ret = read(fd, buffer, len);
    	close(fd);
        return ret;
    }

    ret = read(fd, buffer, len);
    if (ret <= 0) {
        ALOGE("%s()->Line:%d; read info(%d) data error, retcode = %d; \n",
                __FUNCTION__, __LINE__, ret);
        close(fd);
        return EINVAL;
    }

    close(fd);
    return ret;

}

int panel_id_read_for_engpc (char *req, char *uid)
{
	int i = 0;
	int ret = 0;

	char panel_info[UID_LENGTH] = {0};

	ret = panel_id_file_read(panel_info, sizeof(panel_info));

	while(panel_info[i] != '\n') {
		i++;
	}
	panel_info[i] = '\0';

	memcpy(uid, panel_info, ret);

	return 0;
}

void register_this_module(struct eng_callback * reg)
{
	ALOGD("file:%s, func:%s\n", __FILE__, __func__);
	sprintf(reg->at_cmd, "%s", "AT+LCDID");
	reg->eng_linuxcmd_func = panel_id_read_for_engpc;
	ALOGD("module cmd:%s\n", reg->at_cmd);
}
