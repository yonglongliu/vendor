/******************************************************************************
 *  File name: alwrapper_yhist.h
 *  Create Date:
 *
 *  Comment:
 *
 *
 ******************************************************************************/

#ifndef _AL_3AWRAPPER_FLICKER_H_
#define _AL_3AWRAPPER_FLICKER_H_

#include "mtype.h"
/* ISP Framework define */
#include "frmwk_hw3a_event_type.h"
#include "hw3a_stats.h"
/* Wrapper define */
#include "alwrapper_3a.h"

#define _WRAPPER_YHIST_VER 0.8050

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * API name: al3awrapper_dispatchhw3a_yhiststats
 * This API used for patching HW3A stats from ISP(Altek) of Y-histogram, after patching completed, AF ctrl layer should passing to CAF lib
 * param alisp_metadata_yhist[In]: patched data after calling al3AWrapper_DispatchHW3AStats, used for patch yhist stats
 * param al3awrapper_stats_yhist_t[Out]: result patched yhist stats
 * return: error code
 */
uint32 al3awrapper_dispatchhw3a_yhiststats( struct isp_drv_meta_yhist_t * alisp_metadata_yhist, struct al3awrapper_stats_yhist_t * alwrappered_yhist_dat );

/*
 * API name: al3awrapper_yhist_getdefaultcfg
 * This API is used for query default ISP config before updating ISP config of Yhist stats engine
 * param ayhistconfig[out]: input buffer, API would manage parameter and return via this pointer
 * return: error code
 */
uint32 al3awrapper_yhist_getdefaultcfg( struct alhw3a_yhist_cfginfo_t* ayhistconfig);

/*
 * API name: al3awrapper_yhist_getversion
 * This API would return labeled version of wrapper
 * fWrapVersion[out], return current wapper version
 * return: error code
 */
uint32 al3awrapper_yhist_getversion( float *fwrapversion );

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // _AL_3AWRAPPER_FLICKER_H_