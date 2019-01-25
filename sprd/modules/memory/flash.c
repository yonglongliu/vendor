/*
 * Copyright (C) 2018 Spreadtrum Communications Inc.
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
#include "emmc.h"
#include <dlfcn.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <utils/Log.h>
#include <math.h> 
#include "engopt.h"

int sprd_get_nandsize(void)
{
	int fd;
	int cur_row = 2;
	char  buffer[64]={0};
	int read_len = 0;
	float size = 0;
	char *endptr;

	if (0 == access("/sys/block/mtdblock4",F_OK)){
		fd = open("/sys/block/mtdblock4/size",O_RDONLY);
		if(fd < 0){
			ENG_LOG("nand card no exist\n");
			return -1;
		}
		read_len = read(fd,buffer,sizeof(buffer));
		if(read_len <= 0){
			ENG_LOG("nand card no read\n");
			return -1;
		}
		size = strtoul(buffer,&endptr,0);
		close(fd);
		ENG_LOG("sys/block/mtdblock4/size value = %f, read_len = %d ", size, read_len);
		ENG_LOG("%s %4.2f MB", "nand capcity:",(size/2/1024));
		size=ceil(size/2/1024);
		return (int)size;
	}else{
		ENG_LOG("nand cannot access");
		return -1;
	}

}


int sprd_get_emmcsize(void)
{
	int fd;
	int cur_row = 2;
	char  buffer[64]={0},temp[64]={0};
	int read_len = 0;
	float size = 0;
	char *endptr;
	if (0 == access("/sys/block/mmcblk0",F_OK)){
		fd = open("/sys/block/mmcblk0/size",O_RDONLY);
		if(fd < 0){
			ENG_LOG("emmc card no exist\n");
			return -1;
		}
		read_len = read(fd,buffer,sizeof(buffer));
		if(read_len <= 0){
			ENG_LOG("emmc card no read\n");
			return -1;
		}
		size = strtoul(buffer,&endptr,0);
		close(fd);
		ENG_LOG("sys/block/mmcblk0/size value = %f, read_len = %d ", size, read_len);
		ENG_LOG("%s %4.2f GB", TEXT_EMMC_CAPACITY,(size/2/1024/1024));
		size=ceil(size/2/1024/1024);
		return (int)size;
	}else{
		ENG_LOG("emmc cannot access");
		return -1;
	}

}

