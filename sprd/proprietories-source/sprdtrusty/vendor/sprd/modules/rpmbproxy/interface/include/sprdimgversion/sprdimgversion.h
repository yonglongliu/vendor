/*
 * Copyright (C) 2017 spreadtrum.com
 */
#pragma once

typedef enum enAntiRBImageType {
	IMAGE_SYSTEM,
	IMAGE_VENDOR,
	IMAGE_L_MODEM,
	IMAGE_L_LDSP,
	IMAGE_L_LGDSP,
	IMAGE_PM_SYS,
	IMAGE_AGDSP,
	IMAGE_WCN,
	IMAGE_GPS,
	IMAGE_GPU,
	IMAGE_VBMETA,
	IMAGE_BOOT,
	IMAGE_RECOVERY,
} antirb_image_type;


/*
*@imgType   The image which need to get the version
*@swVersion return image version
*Return value: zero is ok
*
*/
int sprd_get_imgversion(antirb_image_type imgType, unsigned int* swVersion);

/*
*@imgType   The image which need to write the version
*@swVersion return image version
*Return value: zero is ok
*/
int sprd_set_imgversion(antirb_image_type imgType, unsigned int swVersion);
