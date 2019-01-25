/******************************************************************************
*  File name: allib_3a_ver.h
*  Create Date:
*
*  Comment:
*  3A managed version
 ******************************************************************************/

#include "mtype.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define _ALLIB_3A_VERSION 0.1650

/*
* API name: allib_3a_getversion
* This API would return labeled version of 3A lib managed version
* fVersion[out], return current version
* return: error code
*/
uint32 allib_3a_getversion( float *fVersion );

#ifdef __cplusplus
}  // extern "C"
#endif