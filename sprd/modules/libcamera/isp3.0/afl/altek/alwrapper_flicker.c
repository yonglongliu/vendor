/******************************************************************************
 *  File name: alwrapper_flicker.c
 *
 *  Created on: 2016/01/05
 *      Author: Hubert Huang
 *  Latest update: 2016/5/3
 *      Reviser: MarkTseng
 *  Comments:
 *       This c file is mainly used for AP framework to:
 *       1. Query HW3A config setting
 *       2. Set HW3A via AP IPS driver of altek
 *       3. translate AP framework index from al3A to framework define, such as scene mode, etc.
 *       4. packed output report from AE lib to framework
 *       5. translate input parameter from AP framework to AP
 *       6. Patch stats from ISP
 *       7. WOI inform to AP
 *       8. Translate flicker enum to/from lib value
 ******************************************************************************/

#include <math.h>
#include <string.h>
#include "mtype.h"
#include "hw3a_stats.h"
/* Wrapper define */
#include "alwrapper_3a.h"
#include "alwrapper_flicker.h"
#include "alwrapper_flicker_errcode.h"
/* Flicker lib define */
#include "allib_flicker.h"
/* include AP log header */
#ifndef LOCAL_NDK_BUILD
#include "isp_common_types.h"
#endif

/* for Flicker ctrl layer */
/*
 * API name: al3AWrapper_DispatchHW3A_FlickerStats
 * This API used for patching HW3A stats from ISP(Altek) for Flicker libs(Altek), after patching completed, Flicker ctrl should prepare patched
 * stats to Flicker libs
 * param alISP_MetaData_Flicker[In]: patched data after calling al3AWrapper_DispatchHW3AStats, used for patch Flicker stats for Flicker lib
 * param alWrappered_Flicker_Dat[Out]: result patched Flicker stats
 * param aFlickerLibCallback[in]: callback lookup table, must passing correct table into this API for querying HW3A config
 * param flicker_runtimeDat[in]: Flicker lib runtime buffer after calling init, must passing correct addr to into this API
 * return: error code
 */
uint32 al3awrapper_dispatchhw3a_flickerstats( struct isp_drv_meta_antif_t * alisp_metadata_flicker, struct al3awrapper_stats_flicker_t * alwrappered_flicker_dat, struct alflickerruntimeobj_t *aflickerlibcallback, void * flicker_runtimedat  )
{
	uint32 ret = ERR_WPR_FLICKER_SUCCESS;
	struct isp_drv_meta_antif_t *pmetadata_flicker;
	struct al3awrapper_stats_flicker_t *ppatched_flickerdat;
	uint8 *stats;

	UNUSED(aflickerlibcallback);
	/* check input parameter validity */
	if ( alisp_metadata_flicker == NULL )
		return ERR_WRP_FLICKER_EMPTY_METADATA;
	if ( alwrappered_flicker_dat == NULL || flicker_runtimedat == NULL )
		return ERR_WRP_FLICKER_INVALID_INPUT_PARAM;

	pmetadata_flicker = (struct isp_drv_meta_antif_t *)alisp_metadata_flicker;
	ppatched_flickerdat = (struct al3awrapper_stats_flicker_t *)alwrappered_flicker_dat;

	if(pmetadata_flicker->b_isstats_byaddr == 1) {
		if ( pmetadata_flicker->puc_antif_stats == NULL )
			return ERR_WRP_FLICKER_INVALID_STATS_ADDR;
		stats = (uint8 *)pmetadata_flicker->puc_antif_stats;
	} else
		stats = (uint8 *)pmetadata_flicker->pantif_stats;

	/* update sturcture size, this would be double checked in flicker libs */
	ppatched_flickerdat->ustructuresize = sizeof( struct al3awrapper_stats_flicker_t );
	/* store patched data/common info/ae info from wrapper */
	ppatched_flickerdat->umagicnum           = pmetadata_flicker->umagicnum;
	ppatched_flickerdat->uhwengineid         = pmetadata_flicker->uhwengineid;
	ppatched_flickerdat->uframeidx           = pmetadata_flicker->uframeidx;
	ppatched_flickerdat->uantiftokenid       = pmetadata_flicker->uantiftokenid;
	ppatched_flickerdat->uantifstatssize     = pmetadata_flicker->uantifstatssize;

	/* store frame & timestamp */
	memcpy( &ppatched_flickerdat->systemtime, &pmetadata_flicker->systemtime, sizeof(struct timeval));
	ppatched_flickerdat->udsys_sof_idx       = pmetadata_flicker->udsys_sof_idx;

	/* check size validity, if over defined max number, return error  */
	if ( pmetadata_flicker->uantifstatssize <= HW3A_ANTIF_STATS_BUFFER_SIZE )
		memcpy( ppatched_flickerdat->pantif_stats, stats, pmetadata_flicker->uantifstatssize );
	else {
		/* AP platform log for invalid flicker stats  */
#ifndef LOCAL_NDK_BUILD
		ISP_LOGE("invalid statssize:%d", pmetadata_flicker->uantifstatssize);
#endif
		ppatched_flickerdat->uantifstatssize = 0;
		return ERR_WRP_FLICKER_INVALID_STATS_SIZE;
	}

	return ret;
}

/*
 * API name: al3AWrapperFlicker_GetDefaultCfg
 * This API is used for query default ISP config before calling al3AWrapperFlicker_UpdateISPConfig_Flicker
 * param aFlickerConfig[out]: input buffer, API would manage parameter and return via this pointer
 * return: error code
 */
uint32 al3awrapperflicker_getdefaultcfg( struct alhw3a_flicker_cfginfo_t* aflickerconfig )
{
	uint32 ret = ERR_WPR_FLICKER_SUCCESS;
	struct alhw3a_flicker_cfginfo_t localParam;

	if ( aflickerconfig == NULL )
		return ERR_WRP_FLICKER_INVALID_INPUT_PARAM;

	localParam.tokenid = 0xFFFF;
	localParam.uwoffsetratiox = 0;
	localParam.uwoffsetratioy = 0;

	memcpy( aflickerconfig, &localParam, sizeof(struct alhw3a_flicker_cfginfo_t ) );
	return ret;
}

/*
 * API name: al3AWrapperFlicker_UpdateISPConfig_Flicker
 * This API is used for updating ISP config
 * param a_ucSensor[in]: AHB sensor ID
 * param aFlickerConfig[out]: input buffer, API would manage parameter and return via this pointer
 * return: error code
 */
uint32 al3awrapperflicker_updateispconfig_flicker( uint8 a_ucsensor, struct alhw3a_flicker_cfginfo_t* aflickerconfig )
{
	uint32 ret = ERR_WPR_FLICKER_SUCCESS;

	UNUSED(a_ucsensor);
	UNUSED(aflickerconfig);
	return ret;
}

/*
 * API name: al3AWrapperFlicker_QueryISPConfig_Flicker
 * This API is used for query ISP config before calling al3AWrapperFlicker_UpdateISPConfig_Flicker
 * param aFlickerConfig[out]: API would manage parameter and return via this pointer
 * param aFlickerLibCallback[in]: callback lookup table, must passing correct table into this API for querying HW3A config
 * param flicker_runtimeDat[in]: Flicker lib runtime buffer after calling init, must passing correct addr to into this API
 * return: error code
 */
uint32 al3awrapperflicker_queryispconfig_flicker( struct alhw3a_flicker_cfginfo_t* aflickerconfig, struct alflickerruntimeobj_t *aflickerlibcallback, void * flicker_runtimedat, struct raw_info *aflickersensorinfo )
{
	uint32 ret = ERR_WPR_FLICKER_SUCCESS;
	struct flicker_get_param_t localparam;
	struct flicker_output_data_t outputparam;
	struct flicker_set_param_t local_setparam;

	if ( aflickerconfig == NULL || flicker_runtimedat == NULL )
		return ERR_WRP_FLICKER_INVALID_INPUT_PARAM;

	memset( &localparam, 0, sizeof( struct flicker_get_param_t) );
	localparam.flicker_get_param_type = FLICKER_GET_ALHW3A_CONFIG;   /* ask Flicker lib for HW3A setting */

	/* send command to Flicker lib, query HW3A setting parameter */
	ret = aflickerlibcallback->get_param( &localparam, flicker_runtimedat );
	if ( ret != ERR_WPR_FLICKER_SUCCESS )
		return ret;

	/* Update raw info & Line time */
	local_setparam.set_param.rawsizex = aflickersensorinfo->sensor_raw_w;
	local_setparam.set_param.rawsizey = aflickersensorinfo->sensor_raw_h;
	local_setparam.set_param.line_time = aflickersensorinfo->line_time;
	local_setparam.flicker_set_param_type = FLICKER_SET_PARAM_RAW_SIZE;
	ret = aflickerlibcallback->set_param( &local_setparam, &outputparam, &flicker_runtimedat );
	if ( ret != ERR_WPR_FLICKER_SUCCESS )
		return ret;
	local_setparam.flicker_set_param_type = FLICKER_SET_PARAM_LINE_TIME;
	ret = aflickerlibcallback->set_param( &local_setparam, &outputparam, &flicker_runtimedat );
	if ( ret != ERR_WPR_FLICKER_SUCCESS )
		return ret;

	memcpy( aflickerconfig, &localparam.alhw3a_flickerconfig, sizeof( struct alhw3a_flicker_cfginfo_t ) );  /* copy data, prepare for return */

	return ret;
}

/*
 * API name: al3AWrapperAntiF_GetVersion
 * This API would return labeled version of wrapper
 * fWrapVersion[out], return current wapper version
 * return: error code
 */
uint32 al3awrapperantif_getversion( float *fwrapversion )
{
	uint32 ret = ERR_WPR_FLICKER_SUCCESS;

	*fwrapversion = _WRAPPER_ANTIF_VER;

	return ret;
}

/*
 * API name: al3awrapper_antif_get_flickermode
 * This API would return handled enum flicker mode (50 Hz or 60 Hz )
 * flicker_libresult[in], flicker result from flicker lib
 * flicker_mode[out], return current wapper version
 * return: error code
 */
uint32 al3awrapper_antif_get_flickermode( uint8 flicker_libresult, enum ae_antiflicker_mode_t *flicker_mode )
{
	uint32 ret = ERR_WPR_FLICKER_SUCCESS;

	if ( flicker_libresult > _DEFAULT_FLICKER_50HZ_TH_LOWER && flicker_libresult < _DEFAULT_FLICKER_50HZ_TH_UPPER )
		*flicker_mode = ANTIFLICKER_50HZ;
	else if ( flicker_libresult > _DEFAULT_FLICKER_60HZ_TH_LOWER && flicker_libresult < _DEFAULT_FLICKER_60HZ_TH_UPPER )
		*flicker_mode = ANTIFLICKER_60HZ;
	else
		return ERR_WRP_FLICKER_INVALID_INPUT_HZ;  /*  invalid input Hz range  */

	return ret;
}

/*
 * API name: al3awrapper_antif_set_flickermode
 * This API would help to parse set manual flicker mode to flicker lib
 * param flicker_mode[in], UI flicker mode (enum, Auto, 50 Hz or 60 Hz)
 * param aFlickerLibCallback[in]: callback lookup table, must passing correct table into this API for querying HW3A config
 * param flicker_runtimeDat[in]: Flicker lib runtime buffer after calling init, must passing correct addr to into this API
 * return: error code
 */
uint32 al3awrapper_antif_set_flickermode( enum antiflicker_ui_mode_t flicker_mode, struct alflickerruntimeobj_t *aflickerlibcallback, void * flicker_runtimedat )
{
	uint32 ret = ERR_WPR_FLICKER_SUCCESS;
	uint16 flicker_hz;
	struct flicker_output_data_t outputparam;   /* f set_param, no actual usage now, just cread instant in avoid of error when passing NULL to lib */
	struct flicker_set_param_t local_setparam;

	if ( flicker_mode == ANTIFLICKER_UI_MODE_AUTO )
		flicker_hz = _DEFAULT_FLICKER_LIBVALUE_AUTO;
	else if ( flicker_mode == ANTIFLICKER_UI_MODE_FIXED_50HZ )
		flicker_hz = _DEFAULT_FLICKER_LIBVALUE_50HZ;
	else if ( flicker_mode == ANTIFLICKER_UI_MODE_FIXED_60HZ )
		flicker_hz = _DEFAULT_FLICKER_LIBVALUE_60HZ;
	else if ( flicker_mode == ANTIFLICKER_UI_MODE_OFF )
		flicker_hz = _DEFAULT_FLICKER_LIBVALUE_OFF;
	else
		return ERR_WRP_FLICKER_INVALID_INPUT_HZ;  /*  invalid input Hz range, do nothing to flicker lib  */

	memset( &local_setparam, 0, sizeof(struct flicker_set_param_t) );
	local_setparam.flicker_set_param_type = FLICKER_SET_PARAM_MANUAL_MODE;   /* ask Flicker lib for HW3A setting */
	local_setparam.set_param.manual_mode = flicker_hz;

	ret = aflickerlibcallback->set_param( &local_setparam, &outputparam, flicker_runtimedat );
	if ( ret != ERR_WPR_FLICKER_SUCCESS )
		return ret;

	return ret;
}

/*
 * API name: al3awrapperflicker_reference_setting
 * This API would provide recommended fps setting
 * param flicker_mode[in], flicker_reference_input_setting_t
 * param aFlickerLibCallback[in]: flicker_reference_output_setting_t
 * return: error code
 */
uint32 al3awrapperflicker_reference_setting(struct flicker_reference_output_setting_t *out, struct flicker_reference_input_setting_t *in)
{
	uint32 ret = ERR_WPR_FLICKER_SUCCESS;
	uint16 max_fps, min_fps;
	double tmp_fps;

	if(in->flicker_freq == ANTIFLICKER_50HZ) {
		max_fps = 2500;
		min_fps = in->current_min_fps;
	} else if(in->flicker_freq == ANTIFLICKER_60HZ) {
		max_fps = 3000;
		min_fps = in->current_min_fps;
	} else {
		max_fps = 3000;
		min_fps = in->current_min_fps;
	}
	out->recmd_max_fps = max_fps;
	out->recmd_min_fps = min_fps;

	return ret;
}