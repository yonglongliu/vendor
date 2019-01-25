#ifndef __ALRNB_LIBRARY_HEADER_
#define __ALRNB_LIBRARY_HEADER_

/*
PROPRIETARY STATEMENT:
THIS FILE (OR DOCUMENT) CONTAINS INFORMATION CONFIDENTIAL AND
PROPRIETARY TO ALTEK CORPORATION AND SHALL NOT BE REPRODUCED
OR TRANSFERRED TO OTHER FORMATS OR DISCLOSED TO OTHERS.
ALTEK DOES NOT EXPRESSLY OR IMPLIEDLY GRANT THE RECEIVER ANY
RIGHT INCLUDING INTELLECTUAL PROPERTY RIGHT TO USE FOR ANY
SPECIFIC PURPOSE, AND THIS FILE (OR DOCUMENT) DOES NOT
CONSTITUTE A FEEDBACK TO ANY KIND.
*/

#include "alGE.h"

#define alRnBRT_ERR_MODULE 0x71000 // 462848
#define alRnBRT_ERR_CODE alGE_ERR_CODE
#define alRnBRT_ERR_SUCCESS alGE_ERR_SUCCESS
#define alRnBRT_ERR_INIT_DIM_FAIL (alRnBRT_ERR_MODULE + 0x00000001) // 462849
#define alRnBRT_ERR_INVALID_TABLE (alRnBRT_ERR_MODULE + 0x00000002) // 462850
#define alRnBRT_ERR_BUFFER_SIZE_TOO_SMALL                                      \
    (alRnBRT_ERR_MODULE + 0x00000003)                                  // 462851
#define alRnBRT_ERR_BUFFER_IS_NULL (alRnBRT_ERR_MODULE + 0x00000004)   // 462852
#define alRnBRT_ERR_INVALID_BASELINE (alRnBRT_ERR_MODULE + 0x00000005) // 462853
#define alRnBRT_ERR_INVALID_EXPOSURE_INFO                                      \
    (alRnBRT_ERR_MODULE + 0x00000006) // 462854
#define alRnBRT_ERR_INVALID_IMG_RATIO                                          \
    (alRnBRT_ERR_MODULE + 0x00000007) // 462855
#define alRnBRT_ERR_INVALID_DEPTH_IMG_RATIO                                    \
    (alRnBRT_ERR_MODULE + 0x00000008)                                  // 462856
#define alRnBRT_ERR_INVALID_F_NUMBER (alRnBRT_ERR_MODULE + 0x00000009) // 462857
#define alRnBRT_ERR_INVALID_BLUR_STRENGTH                                      \
    (alRnBRT_ERR_MODULE + 0x0000000a) // 462858
#define alRnBRT_ERR_INVALID_COORDINATE                                         \
    (alRnBRT_ERR_MODULE + 0x0000000b) // 462859
#define alRnBRT_ERR_UNREGISTERED_DEPTH_MAP                                     \
    (alRnBRT_ERR_MODULE + 0x0000000c) // 462860

#ifdef __cplusplus
extern "C" {
#endif

alRnBRT_ERR_CODE alRnBRT_PackDataTransform(void *a_tOutALCalData,
                                           int *a_dOutALCalDataSize,
                                           void *a_pInOTPBuf, int a_dInOTPSize);

alRnBRT_ERR_CODE alRnBRT_Init(void *const a_pInData, int a_dInSize,
                              int a_dInImgW, int a_dInImgH, void *a_pInOTPBuf,
                              int a_dInOTPSize);

alRnBRT_ERR_CODE alRnBRT_Close();

alRnBRT_ERR_CODE alRnBRT_VersionInfo_Get(void *a_pOutBuf, int a_dInBufMaxSize);

alRnBRT_ERR_CODE alRnBRT_ReFocus(unsigned char *a_pucOutYCCNV21,
                                 unsigned char *a_pucInYCCNV21Buf,
                                 unsigned char *a_puwInDisparityBuf,
                                 unsigned short a_uwInDisparityW,
                                 unsigned short a_uwInDisparityH,
                                 unsigned short a_uwInX,
                                 unsigned short a_uwInY);

#ifdef __cplusplus
}
#endif

#endif //__ALRNB_LIBRARY_HEADER_
