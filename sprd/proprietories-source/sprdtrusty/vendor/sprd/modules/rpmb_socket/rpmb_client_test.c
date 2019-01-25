/*
 * Copyright (c) 2017, Spreadtrum.com.
 * All rights reserved.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "rpmb_client.h"


static uint8_t key_byte[] =  {
    0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xa,0xb,0xc,0xd,0xe,0xf,
    0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xa,0xb,0xc,0xd,0xe,0xf};


int main(void)
{
    int ret = -1;

	ret = is_wr_rpmb_key();
	printf("is_wr_rpmb_key() return %d \n", ret);

	ret = -1;
	ret = wr_rpmb_key(key_byte, sizeof(key_byte));
	printf("wr_rpmb_key() return %d \n", ret);

	ret = -1;
	ret = run_storageproxyd();
	printf("run_storageproxyd() return %d \n", ret);
}
