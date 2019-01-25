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
#include <sprd_efuse_hw.h>
#include "eng_diag.h"
#include "eng_modules.h"

#if defined(ANDROID)
#include <android/log.h>
#include <utils/Log.h>

#undef LOG_TAG
#define LOG_TAG "sprd_efuse_hal"

#undef DIS_EFUSE_DEBUG
#define DIS_EFUSE_DEBUG 1

#define DEV_UIDVAL "/sys/class/misc/sprd_otp_ap_efuse/uidval"
#define EFUSE_OK                 (0)
#define EFUSE_INPUT_PARAM_ERR    (-1)
#define EFUSE_OPEN_FAILED_ERR    (-2)
#define EFUSE_READ_FAILED_ERR    (-3)
#define EFUSE_WRITE_FAILED_ERR   (-4)
#define EFUSE_LSEEK_FAILED_ERR   (-4)

#define UID_LENGTH   (50)

#define LOGI(fmt, args...) \
  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, fmt, ##args)
#define LOGD(fmt, args...) \
  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##args)
#define LOGE(fmt, args...) \
  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, fmt, ##args)
#endif

static int uidval_file_read(unsigned char *read_data)
{
    int fd = -1, ret = EFUSE_OK;
    off_t curpos, offset;

    if (read_data == NULL)
        return EFUSE_INPUT_PARAM_ERR;

    fd = open(DEV_UIDVAL, O_RDONLY);
    if (fd < 0) {
        ALOGE("%s()->Line:%d; %s open error, fd = %d. ERRORNO:%S\n",
                __FUNCTION__, __LINE__, DEV_UIDVAL, fd, errno);
        return EFUSE_OPEN_FAILED_ERR;
    }

    curpos = lseek(fd, 0, SEEK_CUR);
    if (curpos == -1) {
        ALOGE("%s()->Line:%d; lseek error\n", __FUNCTION__, __LINE__);
        close(fd);
        return EFUSE_LSEEK_FAILED_ERR;
    }

    ret = read(fd, read_data, sizeof(unsigned char)* UID_LENGTH);
    if (ret <= 0) {
        ALOGE("%s()->Line:%d; read file ata error, retcode = %d; \n",
                __FUNCTION__, __LINE__, ret);
        close(fd);
        return EFUSE_READ_FAILED_ERR;
    }

    close(fd);
    return ret;
}

int file_uid_read_value(unsigned char *uid, int count)
{
    int len = 0;

    ALOGD("%s()->Line:%d; count = %d \n", __FUNCTION__, __LINE__, count);

    if ((NULL == uid) || (count < 1))
        return EFUSE_INPUT_PARAM_ERR;

    uidval_file_read(uid);
	len = strlen(uid);

    ALOGD("%s()->Line:%d; uid = %s, len = %d \n",
            __FUNCTION__, __LINE__, uid, (len-1));
    return (len - 1);
}


int efuse_uid_read_for_engpc (char *req, char *uid)
{
	int count = 0xFFFF;// This may be modified
	file_uid_read_value(uid, count);
	return 0;
}

void register_this_module(struct eng_callback * reg)
{
	ALOGD("file:%s, func:%s\n", __FILE__, __func__);
	sprintf(reg->at_cmd, "%s", "AT+GETDYNAMICUID");
	reg->eng_linuxcmd_func = efuse_uid_read_for_engpc;
	ALOGD("module cmd:%s\n", reg->at_cmd);
}

