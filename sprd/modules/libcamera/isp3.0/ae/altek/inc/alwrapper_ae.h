/******************************************************************************
 *  File name: alwrapper_ae.h
 *  Create Date:
 *
 *  Comment:
 *
 *
 ******************************************************************************/
#ifndef _AL_3AWRAPPER_AE_H_
#define _AL_3AWRAPPER_AE_H_

#include "mtype.h"
/* ISP Framework define */
#include "frmwk_hw3a_event_type.h"
#include "hw3a_stats.h"
/* AE lib define */
#include "allib_ae.h"
#include "allib_ae_errcode.h"
/* Wrapper define */
#include "alwrapper_3a.h"

#define _WRAPPER_AE_VER 0.8110
#define _DEFAULT_WRAPPER_MINISO  50


#ifdef __cplusplus
extern "C"
{
#endif

/*
 * API name: al3AWrapperAE_QueryISPConfig_AE
 * This API is used for query ISP config before calling al3AWrapperAE_UpdateISPConfig_AE
 * param aAEConfig[out]: API would manage parameter and return via this pointer
 * param aaelibcallback[in]: callback lookup table, must passing correct table into this API for querying HW3A config
 * param ae_runtimedat[in]: AE lib runtime buffer after calling init, must passing correct addr to into this API
 * return: error code
 */
uint32 al3awrapperae_queryispconfig_ae( struct alhw3a_ae_cfginfo_t* aaeconfig, struct alaeruntimeobj_t *aaelibcallback, void * ae_runtimedat );

/*
 * API name: al3AWrapperAE_GetDefaultCfg
 * This API is used for query default ISP config before calling al3AWrapperAE_UpdateISPConfig_AE
 * param aAEConfig[out]: input buffer, API would manage parameter and return via this pointer
 * return: error code
 */
uint32 al3awrapperae_getdefaultcfg( struct alhw3a_ae_cfginfo_t* aaeconfig);

/*
 * API name: al3AWrapperAE_QueryISPConfig_AE
 * This API is used for query ISP config before calling al3AWrapperAE_UpdateISPConfig_AE
 * param a_ucSensor[in]: AHB sensor ID
 * param aAEConfig[out]: input buffer, API would manage parameter and return via this pointer
 * return: error code
 */
uint32 al3awrapperae_updateispconfig_ae( uint8 a_ucsensor, struct alhw3a_ae_cfginfo_t* aaeconfig );

/*
 * API name: al3AWrapperAE_UpdateOTP2AELib
 * This API is used for set correct HW3A config (Altek ISP) to generate correct AE stats
 * param aCalibWBGain[in]: calibration data from OTP
 * param alAERuntimeObj_t[in]: callback lookup table, must passing correct table into this API for querying HW3A config
 * param ae_output[in]: returned output result after calling ae lib estimation
 * param ae_runtimedat[in]: AE lib runtime buffer after calling init, must passing correct addr to into this API
 * return: error code
 */
uint32 al3awrapperae_updateotp2aelib( struct calib_wb_gain_t acalibwbgain, struct alaeruntimeobj_t *aaelibcallback, struct ae_output_data_t *ae_output , void * ae_runtimedat );

/*
 * API name: al3AWrapperAE_UpdateOTP2AELib
 * This API is used for set correct HW3A config (Altek ISP) to generate correct AE stats
 * param aAEReport[in]: ae report from AE update
 * return: error code
 */
uint32 al3awrapper_updateaestate( struct ae_report_update_t* aaereport );

/*
 * API name: al3AWrapperAE_TranslateSceneModeFromAELib2AP
 * This API used for translating AE lib scene define to framework
 * param aSceneMode[In] :   scene mode define of AE lib (Altek)
 * return: scene mode define for AP framework
 */
uint32 al3awrapperae_translatescenemodefromaelib2ap( uint32 ascenemode );

/*
 * API name: al3AWrapperAE_TranslateSceneModeFromAP2AELib
 * This API used for translating framework scene mode to AE lib define
 * aSceneMode[In] :   scene mode define of AP framework define
 * return: scene mode define for AE lib (Altek)
 */
uint32 al3awrapperae_translatescenemodefromap2aelib( uint32 ascenemode );

/*
 * API name: al3AWrapper_DispatchHW3A_AEStats
 * This API used for patching HW3A stats from ISP(Altek) for AE libs(Altek), after patching completed, AE ctrl should prepare patched
 * stats to AE libs
 * param alISP_MetaData_AE[In]: patched data after calling al3AWrapper_DispatchHW3AStats, used for patch AE stats for AE lib
 * param alWrappered_AE_Dat[Out]: result patched AE stats, must input WB gain
 * param aaelibcallback[in]: callback lookup table, must passing correct table into this API for querying HW3A config
 * param ae_runtimedat[in]: AE lib runtime buffer after calling init, must passing correct addr to into this API
 * return: error code
 */
uint32 al3awrapper_dispatchhw3a_aestats( struct isp_drv_meta_ae_t * alisp_metadata_ae, struct al3awrapper_stats_ae_t * alwrappered_ae_dat, struct alaeruntimeobj_t *aaelibcallback, void * ae_runtimedat  );


/*
 * API name: al3AWrapper_SetAELib_Param
 * This API should be interface between framework with AE lib, if framework define differ from libs, should be translated here
 * uDatatype[in], usually should be set_param type, such as set ROI, etc.
 * set_param[in], would be framework defined structure, casting case by case
 * ae_output[in], returned output result after calling ae lib estimation
 * param aaelibcallback[in]: callback lookup table, must passing correct table into this API for querying HW3A config
 * param ae_runtimedat[in]: AE lib runtime buffer after calling init, must passing correct addr to into this API
 * return: error code
 */
uint32 al3awrapper_setaelib_param( void * udatatype, void * set_param, struct alaeruntimeobj_t *aaelibcallback, struct ae_output_data_t *ae_output, void * ae_runtimedat );

/*
 * API name: al3AWrapper_SetAELib_Param
 * This API should be interface between framework with AE lib, if framework define differ from libs, should be translated here
 * uDatatype[in], usually should be set_param type, such as set ROI, etc.
 * set_param[out], would be framework defined structure, casting case by case
 * param aaelibcallback[in]: callback lookup table, must passing correct table into this API for querying HW3A config
 * param ae_runtimedat[in]: AE lib runtime buffer after calling init, must passing correct addr to into this API
 * return: error code
 */
uint32 al3awrapper_getaelib_param( void * udatatype, void * get_param, struct alaeruntimeobj_t *aaelibcallback, void * ae_runtimedat );

/*
 * API name: al3AWrapper_UpdateAESettingFile
 * This API should be interface between framework with AE lib, if framework define differ from libs, should be translated here
 * ae_set_file[in], handle of AE setting file, remapping here if neccessary
 * ae_output[in], returned output result after calling ae lib estimation
 * param aaelibcallback[in]: callback lookup table, must passing correct table into this API for querying HW3A config
 * param ae_runtimedat[in]: AE lib runtime buffer after calling init, must passing correct addr to into this API
 * return: error code
 */
uint32 al3awrapper_updateaesettingfile( void * ae_set_file, struct alaeruntimeobj_t *aaelibcallback, struct ae_output_data_t *ae_output, void * ae_runtimedat );

/*
 * API name: al3AWrapperAE_SetSceneSetting
 * This API is used for setting scene event file to alAELib
 * param setting_info[in]: scene info addr
 * param alAERuntimeObj_t[in]: callback lookup table, must passing correct table into this API for querying HW3A config
 * param ae_output[in]: returned output result after calling ae lib estimation
 * param ae_runtimedat[in]: AE lib runtime buffer after calling init, must passing correct addr to into this API
 * return: error code
 */
uint32 al3awrapperae_setscenesetting(void *setting_info, struct alaeruntimeobj_t *aaelibcallback, struct ae_output_data_t *ae_output , void * ae_runtimedat );

/*
 * API name: al3AWrapperAE_GetVersion
 * This API would return labeled version of wrapper
 * fWrapVersion[out], return current wapper version
 * return: error code
 */
uint32 al3awrapperae_getversion( float *fwrapversion );

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // _AL_3AWRAPPER_AE_H_