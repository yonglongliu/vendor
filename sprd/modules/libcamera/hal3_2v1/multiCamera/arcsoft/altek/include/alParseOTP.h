#ifndef __ALPARSEOTP_LIBRARY_HEADER_
#define __ALPARSEOTP_LIBRARY_HEADER_

#include "AltekPara.h"
#include "alGE.h"

#define alParseOTP_ERR_MODULE                           0x42000
#define alParseOTP_ERR_CODE                             alGE_ERR_CODE
#define alParseOTP_ERR_SUCCESS                          alGE_ERR_SUCCESS
#define alParseOTP_ERR_OPEN_FAIL                        (alParseOTP_ERR_MODULE+0x00000001) //270337
#define alParseOTP_ERR_BUFFER_SIZE_TOO_SMALL            (alParseOTP_ERR_MODULE+0x00000002) //270338

#ifdef __cplusplus
extern "C" {
#endif

alParseOTP_ERR_CODE alParseOTP(void *a_pInOTPBuf, int a_dInOTPSize, int a_dInVCMStep, int a_dInCamLayout,
                               ALTEK_OUTPUT *a_tOutStruct);

alParseOTP_ERR_CODE alParseOTP_VersionInfo_Get(void *a_pOutBuf, int a_dInBufMaxSize);

#ifdef __cplusplus
}
#endif

#endif
