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
#include <stdlib.h>
#include <string.h> 
#include <memory.h>
#include "flash.h"
#include "ddr.h"
#include "eng_diag.h"
#include "eng_modules.h"
#include "engopt.h"

#define ENG_STREND "\r\n"

static int get_flashddrsize(char *req,char *rsp)
{
//ddr

	char ddr_size_str[5]={0};
	int retddr=-2;
	int ddr_size_int=0;
	int emmc_size=0;
	int nand_size=0;
//test  the size of the DDR
	memset(ddr_size_str, 0, 5);
	retddr = sprd_get_ddrsize(ddr_size_str);
	if (retddr != 0) {
		ENG_LOG("/proc/sprd_dmc/property not exist\n");
		sprintf(rsp, "%s%s", "sprd_dmc/property no exist ", ENG_STREND);
		return 0;
	} else if (retddr == 0) {
		ddr_size_int = atoi(ddr_size_str);  //MB
	}

//test the size of the emmc/nand
	nand_size = sprd_get_nandsize();
	emmc_size = sprd_get_emmcsize();
	
	if(nand_size != -1) {
		ENG_LOG("read the nand size success \n");
		sprintf(rsp, "%dM+%dM%s", nand_size, ddr_size_int, ENG_STREND);
		return 0;
	} else if(emmc_size != -1) {
		ENG_LOG("read the emmc size success \n");
		sprintf(rsp, "%dG+%dM%s", emmc_size, ddr_size_int, ENG_STREND);
		return 0;
	} else
		sprintf(rsp, "%s%s", "emmc/nand size read fail ", ENG_STREND);

	return 0;
}

void register_this_module(struct eng_callback * reg)
{
	ALOGD("file:%s, func:%s\n", __FILE__, __func__);
	sprintf(reg->at_cmd, "%s", "AT+EMMCDDRSIZE");
	reg->eng_linuxcmd_func = get_flashddrsize;
	ALOGD("module cmd:%s\n", reg->at_cmd);
}
