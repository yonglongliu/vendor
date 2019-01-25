/******************************************************************************
 * allib_3a_ver.c
 *
 *  Created on: 2016/3/30
 *      Author: MarkTseng
 *  Latest update:
 *      Reviser:
 *  Comments:
 *       This c file is mainly used for AP framework to:
 *       1. Get release 3A managed version
 ******************************************************************************/

#include "mtype.h"
#include "allib_3a_ver.h"
#include "allib_3a_ver_errcode.h"

/*
* API name: allib_3a_getversion
* This API would return labeled version of 3A lib managed version
* fVersion[out], return current version
* return: error code
*/
uint32 allib_3a_getversion( float *fVersion )
{
	uint32 ret = ERR_3ALIB_VER_SUCCESS;

	if ( fVersion == NULL )
		return ERR_3ALIB_VER_NULL_PTR;
	*fVersion = _ALLIB_3A_VERSION;

	return ret;
}