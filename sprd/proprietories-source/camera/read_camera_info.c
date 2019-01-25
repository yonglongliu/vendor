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
#define LOG_TAG "sprd_camera_hal"

#define LOGI(fmt, args...) \
  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, fmt, ##args)
#define LOGD(fmt, args...) \
  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##args)
#define LOGE(fmt, args...) \
  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, fmt, ##args)
#endif

#define DEV_NODE "/data/misc/media/check_x_tool.file"

#define MAX_CAMERA_INFO_LEN 128

typedef enum {
  CMD_AT,
  CMD_DIAG,  /* handled by AP */
} cmd_type;

struct camera_cmd_str {
  cmd_type type;
  char *name;
  int (*cmd_hdlr)(char *, char *);
};

static int camera_sensor_file_read(char *buffer, int len)
{
    int fd = -1, ret = 0;

    if (buffer == NULL)
        return -1;

    fd = open(DEV_NODE, O_RDWR);
    if (fd < 0) {
        ALOGE("%s()->Line:%d; %s open error, fd = %d. \n",
                __FUNCTION__, __LINE__, DEV_NODE, fd);
        return EINVAL;
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

char *camera_info_parse(char *buffer, const char * str) {
	
    char *src,*dst;
	src = strstr(buffer, str);
	if (src == NULL)
		return NULL;
	dst = src + strlen(str);
 
    while(*src !='\n') {
        src++;
    }
    *src = '\0';
	return dst;
}

int read_cam0id_info (char *req, char *uid)
{
	char camera_info[MAX_CAMERA_INFO_LEN] = {0};
    char *info;

    camera_sensor_file_read(camera_info, sizeof(camera_info));
	info = camera_info_parse(camera_info, "cam0id:");
	strcpy(uid, info);

//	ALOGE("ywen:read_cam0id_info");
	return 0;
}

int read_cam0size_info (char *req, char *uid)
{
	char camera_info[MAX_CAMERA_INFO_LEN] = {0};
    char *info;

    camera_sensor_file_read(camera_info, sizeof(camera_info));
	info = camera_info_parse(camera_info, "cam0size:");
	strcpy(uid, info);
	
//	ALOGE("ywen:read_cam0size_info");
	return 0;
}

int read_cam1id_info (char *req, char *uid)
{
	char camera_info[MAX_CAMERA_INFO_LEN] = {0};
    char *info;

    camera_sensor_file_read(camera_info, sizeof(camera_info));
	info = camera_info_parse(camera_info, "cam1id:");
	strcpy(uid, info);
	
//	ALOGE("ywen:read_cam1id_info");
	return 0;
}
int read_cam1size_info (char *req, char *uid)
{
	char camera_info[MAX_CAMERA_INFO_LEN] = {0};
    char *info;

    camera_sensor_file_read(camera_info, sizeof(camera_info));
	info = camera_info_parse(camera_info, "cam1size:");
	strcpy(uid, info);
	
//	ALOGE("ywen:read_cam1size_info");
	return 0;
}

static struct camera_cmd_str cmd_array[] = {
    {CMD_AT, "AT+CAM0ID", read_cam0id_info},
    {CMD_AT, "AT+CAM0SIZE", read_cam0size_info},
    {CMD_AT, "AT+CAM1ID", read_cam1id_info},
    {CMD_AT, "AT+CAM1SIZE", read_cam1size_info},
};

void register_this_module_ext(struct eng_callback *reg, int *num)
{
	int moudles_num = 0; 
	int i = 0;
	ALOGD("file:%s, func:%s\n", __FILE__, __func__);

	moudles_num = sizeof(cmd_array)/sizeof(cmd_array[0]);
    for (i = 0; i < moudles_num; i++) {
		if (cmd_array[i].type == CMD_AT) {
			sprintf((reg+i)->at_cmd, "%s", cmd_array[i].name);
			(reg+i)->eng_linuxcmd_func = cmd_array[i].cmd_hdlr;
		}
	}

    *num = moudles_num;
    ALOGD("register_this_module_ext: %d - %d",*num, moudles_num);

}

