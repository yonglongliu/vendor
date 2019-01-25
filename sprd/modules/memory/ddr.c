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
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <memory.h>

#define DDR_SIZE "/proc/sprd_dmc/property"

#include "engopt.h"
#include "ddr.h"
#include <fcntl.h>

int  sprd_get_ddrsize( char *ddr_size)
{
	char num[5]={0};
	int fd=-1; 
	memset(num, 0, 5);

	fd= open(DDR_SIZE, O_RDONLY);
	if( fd < 0 ) {
		ENG_LOG("open '%s' error\n", DDR_SIZE);
		return -1;
	}
	read(fd, ddr_size, 5);
	
	ENG_LOG(" getDDRsize(  ddr_size=%s MB \n", ddr_size);
	close(fd);
	return 0;
}
