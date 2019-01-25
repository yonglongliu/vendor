/*
 * Copyright (C) 2017 spreadtrum.com
 */

#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include <sprdimgversion.h>



int main(void)
{
    unsigned int version = 3;
	int ret;

    sprd_set_imgversion(IMAGE_SYSTEM, version);
    sprd_set_imgversion(IMAGE_VENDOR, version);
    sprd_set_imgversion(IMAGE_L_MODEM, version);


    version = 0;
    ret = sprd_get_imgversion(IMAGE_SYSTEM, &version);
    fprintf(stderr, "IMAGE_SYSTEM version : %d, %d\n", version, ret);

    version = 0;
    ret = sprd_get_imgversion(IMAGE_VENDOR, &version);
    fprintf(stderr, "IMAGE_VENDOR version : %d, %d\n", version, ret);

    version = 0;
    ret = sprd_get_imgversion(IMAGE_L_MODEM, &version);
    fprintf(stderr, "IMAGE_L_MODEM version : %d, %d\n", version,ret);


    return 0;

}
