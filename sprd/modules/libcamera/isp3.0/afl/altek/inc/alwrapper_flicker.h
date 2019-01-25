/******************************************************************************
 *  File name: alwrapper_flicker.h
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
/* Flicker lib define */
#include "allib_flicker.h"
#include "allib_flicker_errcode.h"
/* Wrapper define */
#include "alwrapper_3a.h"


#define _WRAPPER_ANTIF_VER 0.8080

#define _DEFAULT_FLICKER_50HZ_TH_LOWER  (47)
#define _DEFAULT_FLICKER_50HZ_TH_UPPER  (53)
#define _DEFAULT_FLICKER_60HZ_TH_LOWER  (56)
#define _DEFAULT_FLICKER_60HZ_TH_UPPER  (64)
#define _DEFAULT_FLICKER_LIBVALUE_50HZ  (50)
#define _DEFAULT_FLICKER_LIBVALUE_60HZ  (60)
#define _DEFAULT_FLICKER_LIBVALUE_AUTO  (0)
#define _DEFAULT_FLICKER_LIBVALUE_OFF   (255)

#define _DEFAULT_FLICKER_MODE      (ANTIFLICKER_UI_MODE_FIXED_50HZ)

#ifdef __cplusplus
extern "C"
{
#endif

/**
\api name: al3awrapper_dispatchhw3a_flickerstats
\this api used for patching hw3a stats from isp(altek) for flicker libs(altek), after patching completed, flicker ctrl should prepare patched
\stats to flicker libs
\param alisp_metadata_flicker[in]: patched data after calling al3awrapper_dispatchhw3astats, used for patch flicker stats for flicker lib
\param alwrappered_flicker_dat[out]: result patched flicker stats
\param aflickerlibcallback[in]: callback lookup table, must passing correct table into this api for querying hw3a config
\param flicker_runtimedat[in]: flicker lib runtime buffer after calling init, must passing correct addr to into this api
\return: error code
*/
uint32 al3awrapper_dispatchhw3a_flickerstats( struct isp_drv_meta_antif_t * alisp_metadata_flicker, struct al3awrapper_stats_flicker_t * alwrappered_flicker_dat, struct alflickerruntimeobj_t *aflickerlibcallback, void * flicker_runtimedat  );


/**
\api name: al3awrapperflicker_getdefaultcfg
\this api is used for query default isp config before calling al3awrapperflicker_updateispconfig_flicker
\param aflickerconfig[out]: input buffer, api would manage parameter and return via this pointer
\return: error code
*/
uint32 al3awrapperflicker_getdefaultcfg( struct alhw3a_flicker_cfginfo_t* aflickerconfig );


/**
\api name: al3awrapperflicker_updateispconfig_flicker
\this api is used for updating isp config
\param a_ucsensor[in]: ahb sensor id
\param aflickerconfig[out]: input buffer, api would manage parameter and return via this pointer
\return: error code
*/
uint32 al3awrapperflicker_updateispconfig_flicker( uint8 a_ucsensor, struct alhw3a_flicker_cfginfo_t* aflickerconfig );


/**
\api name: al3awrapperflicker_queryispconfig_flicker
\this api is used for query isp config before calling al3awrapperflicker_updateispconfig_flicker
\param aflickerconfig[out]: api would manage parameter and return via this pointer
\param aflickerlibcallback[in]: callback lookup table, must passing correct table into this api for querying hw3a config
\param flicker_runtimedat[in]: flicker lib runtime buffer after calling init, must passing correct addr to into this api
\return: error code
*/
uint32 al3awrapperflicker_queryispconfig_flicker( struct alhw3a_flicker_cfginfo_t* aflickerconfig, struct alflickerruntimeobj_t *aflickerlibcallback, void * flicker_runtimedat, struct raw_info *aflickersensorinfo );

/**
\api name: al3awrapperantif_getversion
\this api would return labeled version of wrapper
\fwrapversion[out], return current wapper version
\return: error code
*/
uint32 al3awrapperantif_getversion( float *fwrapversion );

/**
\api name: al3awrapper_antif_get_flickermode
\this api would return handled enum flicker mode (50 hz or 60 hz )
\flicker_libresult[in], flicker result from flicker lib
\flicker_mode[out], return current wapper version
\return: error code
*/
uint32 al3awrapper_antif_get_flickermode( uint8 flicker_libresult, enum ae_antiflicker_mode_t *flicker_mode );

/**
 * API name: al3awrapper_antif_set_flickermode
 * This API would help to parse set manual flicker mode to flicker lib
 * param flicker_mode[in], UI flicker mode (enum, Auto, 50 Hz or 60 Hz)
 * param aFlickerLibCallback[in]: callback lookup table, must passing correct table into this API for querying HW3A config
 * param flicker_runtimeDat[in]: Flicker lib runtime buffer after calling init, must passing correct addr to into this API
 * return: error code
 */
uint32 al3awrapper_antif_set_flickermode( enum antiflicker_ui_mode_t flicker_mode, struct alflickerruntimeobj_t *aflickerlibcallback, void * flicker_runtimedat );

/**
 * API name: al3awrapperflicker_reference_setting
 * This API would provide recommended fps setting
 * param flicker_mode[in], flicker_reference_input_setting_t
 * param aFlickerLibCallback[in]: flicker_reference_output_setting_t
 * return: error code
 */
uint32 al3awrapperflicker_reference_setting(struct flicker_reference_output_setting_t *out, struct flicker_reference_input_setting_t *in);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // _AL_3AWRAPPER_FLICKER_H_