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
#include <string.h>
//#include <sys/time.h>
//#include <sys/ioctl.h>
//#include <sys/types.h>

//#include "eng_diag.h"
#include "eng_modules.h"

#if defined(ANDROID)
#include <android/log.h>
#include <utils/Log.h>

#undef LOG_TAG
#define LOG_TAG "sprd_efuse_hal"

#define LOGI(fmt, args...) \
  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, fmt, ##args)
#define LOGD(fmt, args...) \
  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##args)
#define LOGE(fmt, args...) \
  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, fmt, ##args)
#endif



int get_cplc_from_prodnv(char* cplc)
{
	const char* sNfcCplcFilePath = "/productinfo/nfccplc.txt";
	const int cplc_len = 104;
	if(!cplc)
	{
		printf("cplc buff not ready\n");
		return -1;
	}
	
	FILE *pFile = fopen(sNfcCplcFilePath, "r");
	if(!pFile)
	{
		perror("open /productinfo/nfccplc.txt ");
		return -2;
	}
	
	if(fgets(cplc, cplc_len + 1, pFile) == 0)
	{
		perror("get cplc from prodnv fail. ");
		fclose(pFile);
		pFile = 0;
		return -3;
	}
	
	fclose(pFile);
	pFile = 0;
	
	return 0;
}

int get_Nfc_Cplc_for_engpc(char *req, char *cplc)
{
	req = req;
	return get_cplc_from_prodnv(cplc);
}

void register_this_module(struct eng_callback * reg)
{
	printf("file:%s, func:%s\n", __FILE__, __func__);
	sprintf(reg->at_cmd, "%s", "AT+GETNFCCPLC");
	reg->eng_linuxcmd_func = get_Nfc_Cplc_for_engpc;
	printf("module cmd:%s\n", reg->at_cmd);
}

