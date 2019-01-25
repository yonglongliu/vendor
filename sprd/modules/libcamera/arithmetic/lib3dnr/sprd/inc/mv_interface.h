#ifndef _3DNR_MV_INTERFACE_H_
#define _3DNR_MV_INTERFACE_H_

#include <stdint.h>
#include <utils/Log.h>
#include "cmr_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum {
	THREAD_NUM_ONE = 1,
	THREAD_NUM_MAX,
} THREAD_NUMBER;

//preview or capture judge
typedef enum c3dnr_mode_type
{
	MODE_PREVIEW = 0,
	MODE_CAPTURE
}c3dnr_mode_e;

#define DEBUG_STR     "L %d, %s: "
#define DEBUG_ARGS    __LINE__,__FUNCTION__

#define BL_LOGV(format,...) ALOGV(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)

#define BL_LOGE(format,...) ALOGE(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)

#define BL_LOGI(format,...) ALOGI(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)

#define BL_LOGW(format,...) ALOGW(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)
void ProjectMatching(cmr_s32 *xProjRef, cmr_s32 *yProjRef, cmr_s32 *xProjIn, cmr_s32 *yProjIn, cmr_u32 width,
		     cmr_u32 height, cmr_s8 *pOffset, cmr_u32 num, cmr_s32 *extra);
void ProjectMatching_pre(cmr_s32 *xProjRef, cmr_s32 *yProjRef, cmr_s32 *xProjIn, cmr_s32 *yProjIn, cmr_u32 width,
             cmr_u32 height, cmr_s8  *pOffset, cmr_u32 num, cmr_s32 *extra);
cmr_s32 DNR_init_threadPool();
cmr_s32 DNR_destroy_threadPool();
void IntegralProjection1D_process(cmr_u8 *img, cmr_u32 w, cmr_u32 h, cmr_s32 *xProjIn, cmr_s32 *yProjIn,
				  cmr_s32 *extra, c3dnr_mode_e mode);

#ifdef __cplusplus
}
#endif

#endif //_3DNR_MV_INTERFACE_H_
