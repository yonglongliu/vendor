/******************************************************************************
 * File name: alwrapper_awb.h
 * Create Date:
 * Comment:
 *
 *****************************************************************************/

#ifndef _AL_3AWRAPPER_AWB_H_
#define _AL_3AWRAPPER_AWB_H_

#include "mtype.h"
/* ISP Framework define */
#include "frmwk_hw3a_event_type.h"
#include "hw3a_stats.h"
/* AWB lib define */
#include "allib_awb.h"
#include "allib_awb_errcode.h"
/* Wrapper define */
#include "alwrapper_3a.h"
/* Scene define */
#include "alwrapper_scene_setting.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define _WRAPPER_AWB_VER        0.8090

#define AWB_STATS_MAX_BLOCKS    (64)
#define AWB_STATS_MAX_BANKS     (48)

/*
 * API name: al3awrapperawb_setotpcalibration
 * This API is used for setting stylish file to alAWBLib
 * param calib_data[in]: calibration data, type: FLOAT32
 * param aAWBLibCallback[in]: callback lookup table, must passing correct table into this API for setting calibration data
 * return: error code
 */
uint32 al3awrapperawb_setotpcalibration(struct calibration_data_t *calib_data , struct allib_awb_runtime_obj_t *aawblibcallback);

/*
 * API name: al3awrapperawb_getdefaultcfg
 * This API is used for query default ISP config before calling al3awrapperawb_queryispconfig_awb,
 * which default config use without correct OTP
 * param aAWBConfig[out]: input buffer, API would manage parameter and return via this pointer
 * return: error code
 */
uint32 al3awrapperawb_getdefaultcfg(struct alhw3a_awb_cfginfo_t *aawbconfig);

/*
 * API name: al3awrapperawb_queryispconfig_awb
 * This API is used for query ISP config before calling al3awrapperawb_updateispconfig_awb
 * param aAWBConfig[out]: API would manage parameter and return via this pointer
 * param aAWBLibCallback[in]: callback lookup table, must passing correct table into this API for querying HW3A config
 * return: error code
 */
uint32 al3awrapperawb_queryispconfig_awb(struct alhw3a_awb_cfginfo_t *aawbconfig, struct allib_awb_runtime_obj_t *aawblibcallback);

/*
 * API name: al3awrapperawb_updateispconfig_awb
 * This API is used for set correct HW3A config (Altek ISP) to generate correct AWB stats
 * param a_ucSensor[in]: AHB sensor ID
 * param aAWBConfig[in]: setting after query via calling al3awrapperawb_queryispconfig_awb
 * return: error code
 */
uint32 al3awrapperawb_updateispconfig_awb(uint8 a_ucSensor, struct alhw3a_awb_cfginfo_t *aawbconfig);

/*
 * API name: al3awrapperawb_translatescenemodefromawblib2ap
 * This API used for translating AWB lib scene define to framework
 * param aSceneMode[In] :   scene mode define of AWB lib (Altek)
 * return: scene mode define for AP framework
 */
uint32 al3awrapperawb_translatescenemodefromawblib2ap(uint32 ascenemode);

/*
 * API name: al3awrapperawb_translatescenemodefromap2awblib
 * This API used for translating framework scene mode to AWB lib define
 * aSceneMode[In] :   scene mode define of AP framework define
 * return: scene mode define for AWB lib (Altek)
 */
uint32 al3awrapperawb_translatescenemodefromap2awblib(uint32 ascenemode);

/*
 * API name: al3awrapper_dispatchhw3a_awbstats
 * This API used for patching HW3A stats from ISP(Altek) for AWB libs(Altek),
 * after patching completed, AWB ctrl should prepare patched
 * stats to AWB libs
 * param alISP_MetaData_AWB[In]: patched data after calling al3AWrapper_DispatchHW3AStats,
 *                               used for patch AWB stats for AWB lib
 * param alWrappered_AWB_Dat[Out]: result patched AWB stats
 * return: error code
 */
uint32 al3awrapper_dispatchhw3a_awbstats(void *alisp_metadata_awb, void *alwrappered_awb_dat);

/*
 * API name: al3awrapper_dispatchhw3a_awb_stats_rgbdata
 * This API used for patching HW3A stats from ISP(Altek) for third party libary,
 * after patching completed, AWB ctrl should prepare patched
 * stats to AWB libs
 * param alISP_MetaData_AWB[In]: patched data after calling al3AWrapper_DispatchHW3AStats,
 *                               used for patch AWB stats for AWB lib
 * param alWrappered_AWB_Dat[Out]: result AWB stats
 * return: error code
 */
uint32 al3awrapper_dispatchhw3a_awb_stats_rgbdata(void *alisp_metadata_awb, void *alwrappered_awb_dat);

/*
 * API name: al3awrapperawb_settuningfile
 * This API is used for setting Tuning file to alAWBLib
 * param setting_file[in]: file address pointer to setting file [TBD, need confirm with 3A data file format
 * param aAWBLibCallback[in]: callback lookup table, must passing correct table into this API for setting file
 * return: error code
 */
uint32 al3awrapperawb_settuningfile(void *setting_file, struct allib_awb_runtime_obj_t *aawblibcallback);


/*
 * API name: al3awrapperawb_setscenesetting
 * This API is used for setting scene event file to alAWBLib
 * param setting_info[in]: scene info addr
 * param aAWBLibCallback[in]: callback lookup table, must passing correct table into this API for setting file
 * return: error code
 */
uint32 al3awrapperawb_setscenesetting(void *setting_info, struct allib_awb_runtime_obj_t *aawblibcallback);



/*
 * API name: al3awrapperawb_getversion
 * This API would return labeled version of wrapper
 * fWrapVersion[out], return current wapper version
 * return: error code
 */
uint32 al3awrapperawb_getversion(float *fwrapversion);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* _AL_3AWRAPPER_AWB_H_ */

