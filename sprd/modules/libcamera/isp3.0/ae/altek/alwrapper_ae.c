/******************************************************************************
 * alwrapper_ae.c
 *
 *  Created on: 2015/12/06
 *      Author: MarkTseng
 *  Latest update: 2017/01/18
 *      Reviser: HanTseng
 *  Comments:
 *       This c file is mainly used for AP framework to:
 *       1. Query HW3A config setting
 *       2. Set HW3A via AP IPS driver of altek
 *       3. translate AP framework index from al3A to framework define, such as scene mode, etc.
 *       4. packed output report from AE lib to framework
 *       5. translate input parameter from AP framework to AP
 *       6. Patch stats from ISP
 *       7. WOI inform to AP
 ******************************************************************************/

#include "mtype.h"
#include "frmwk_hw3a_event_type.h"
#include "hw3a_stats.h"
/* AE lib define */
#include "allib_ae.h"
#include "allib_ae_errcode.h"

/* Wrapper define */
#include "alwrapper_3a.h"
#include "alwrapper_ae.h"   /* include wrapper AE define */
#include "alwrapper_ae_errcode.h"
#include "alwrapper_scene_setting.h"

#include <math.h>
#include <string.h>

#include <stdio.h>
#include <stdlib.h>



/* for AE ctrl layer */
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
uint32 al3awrapper_dispatchhw3a_aestats( struct isp_drv_meta_ae_t * alisp_metadata_ae, struct al3awrapper_stats_ae_t * alwrappered_ae_dat, struct alaeruntimeobj_t *aaelibcallback, void * ae_runtimedat  )
{
	uint32 ret = ERR_WPR_AE_SUCCESS;
	struct isp_drv_meta_ae_t *pmetadata_ae;
	uint8 *stats;
	struct al3awrapper_stats_ae_t *ppatched_aedat;
	struct al3awrapper_ae_wb_gain_t wb_gain;
	uint32 udtotalblocks;
	uint32 udpixelsperblocks;
	uint32 udbanksize;
	uint32 udoffset;
	UINT16 i,j,blocks, banks, index;
	struct ae_get_param_t localparam;
	uint32 *pt_stats_r, *pt_stats_g, *pt_stats_b, *pt_stats_y;

	/* check input parameter validity */
	if ( alisp_metadata_ae == NULL )
		return ERR_WRP_AE_EMPTY_METADATA;
	if ( alwrappered_ae_dat == NULL || ae_runtimedat == NULL || aaelibcallback == NULL )
		return ERR_WRP_AE_INVALID_INPUT_PARAM;

	pmetadata_ae = (struct isp_drv_meta_ae_t *)alisp_metadata_ae;
	ppatched_aedat = (struct al3awrapper_stats_ae_t *)alwrappered_ae_dat;

	/* pseudo flag processing */
	if ( pmetadata_ae->upseudoflag == 1 ) {
		/* update sturcture size, this would be double checked in AE libs */
		ppatched_aedat->ustructuresize = sizeof( struct al3awrapper_stats_ae_t );

		ppatched_aedat->umagicnum           = pmetadata_ae->umagicnum;
		ppatched_aedat->uhwengineid         = pmetadata_ae->uhwengineid;
		ppatched_aedat->uframeidx           = pmetadata_ae->uframeidx;
		ppatched_aedat->uaetokenid          = pmetadata_ae->uaetokenid;
		ppatched_aedat->uaestatssize        = pmetadata_ae->uaestatssize;
		ppatched_aedat->udpixelsperblocks   = pmetadata_ae->ae_stats_info.udpixelsperblocks;
		ppatched_aedat->udbanksize          = pmetadata_ae->ae_stats_info.udbanksize;
		ppatched_aedat->ucvalidblocks       = pmetadata_ae->ae_stats_info.ucvalidblocks;
		ppatched_aedat->ucvalidbanks        = pmetadata_ae->ae_stats_info.ucvalidbanks;
		ppatched_aedat->upseudoflag         = pmetadata_ae->upseudoflag;

		/* store to stats info, translate to unit base 1000 (= 1.0x) */
		ppatched_aedat->stat_wb_2y.r = 1000;
		ppatched_aedat->stat_wb_2y.g = 1000;
		ppatched_aedat->stat_wb_2y.b = 1000;

		/* store frame & timestamp */
		memcpy( &ppatched_aedat->systemtime, &pmetadata_ae->systemtime, sizeof(struct timeval));
		ppatched_aedat->udsys_sof_idx       = pmetadata_ae->udsys_sof_idx;
		ppatched_aedat->udisp_dgain       = pmetadata_ae->udisp_dgain;

		/* reset flag */
		ppatched_aedat->bisstatsbyaddr = FALSE;
		/* patch stats to defined array */
		pt_stats_r = (uint32 *)(ppatched_aedat->statsr);
		pt_stats_g = (uint32 *)(ppatched_aedat->statsg);
		pt_stats_b = (uint32 *)(ppatched_aedat->statsb);
		pt_stats_y = (uint32 *)(ppatched_aedat->statsy);

		return ret;
	}


	/* check if stats from allocated buffer */
	if ( pmetadata_ae->b_isstats_byaddr == 1 ) {
		if ( pmetadata_ae->puc_ae_stats == NULL )
			return ERR_WRP_AE_INVALID_STATS_ADDR;
		stats = (uint8 *)pmetadata_ae->puc_ae_stats;
	} else
		stats = (uint8 *)pmetadata_ae->pae_stats;

	/* update sturcture size, this would be double checked in AE libs */
	ppatched_aedat->ustructuresize = sizeof( struct al3awrapper_stats_ae_t );

	udtotalblocks = pmetadata_ae->ae_stats_info.ucvalidblocks * pmetadata_ae->ae_stats_info.ucvalidbanks;
	udbanksize = pmetadata_ae->ae_stats_info.udbanksize;
	blocks = pmetadata_ae->ae_stats_info.ucvalidblocks;
	banks = pmetadata_ae->ae_stats_info.ucvalidbanks;

	/* check data AE blocks validity */
	if ( udtotalblocks > AL_MAX_AE_STATS_NUM )
		return ERR_WRP_AE_INVALID_BLOCKS;

	udpixelsperblocks = pmetadata_ae->ae_stats_info.udpixelsperblocks;

	if (0 == udpixelsperblocks)
		return ERR_WRP_AE_INVALID_PIXEL_PER_BLOCKS;

	/* query AWB from AE libs */
	memset( &localparam, 0, sizeof(struct ae_get_param_t) );
	localparam.ae_get_param_type = AE_GET_CURRENT_CALIB_WB;		/* ask AE lib for HW3A setting */
	ret = aaelibcallback->get_param( &localparam, ae_runtimedat );
	if ( ret != ERR_WPR_AE_SUCCESS )
		return ret;

	/* check WB validity, calibration WB is high priority */
	if ( localparam.para.calib_data.calib_r_gain == 0 ||  localparam.para.calib_data.calib_g_gain == 0 || localparam.para.calib_data.calib_b_gain == 0 ) {  /* calibration gain from OTP, scale 1000, uint32 type */
		memset( &localparam, 0, sizeof(struct ae_get_param_t) );
		localparam.ae_get_param_type = AE_GET_CURRENT_WB;  /* ask AE lib for HW3A setting */
		ret = aaelibcallback->get_param( &localparam, ae_runtimedat );
		if ( ret != ERR_WPR_AE_SUCCESS )
			return ret;

		if ( localparam.para.wb_data.r_gain == 0 || localparam.para.wb_data.g_gain == 0 || localparam.para.wb_data.b_gain == 0  )   /* WB from AWB, scale 256, UINT16 type */
			return ERR_WRP_AE_INVALID_INPUT_WB;
		else {
			wb_gain.r = (uint32)(localparam.para.wb_data.r_gain);
			wb_gain.g = (uint32)(localparam.para.wb_data.g_gain);
			wb_gain.b = (uint32)(localparam.para.wb_data.b_gain);

			/* debug printf, removed for release version */
			// printf( "al3AWrapper_DispatchHW3A_AEStats get LV WB R/G/B : (%d, %d, %d) \r\n", wb_gain.r, wb_gain.g, wb_gain.b );
		}
	} else {
		wb_gain.r = (uint32)(localparam.para.calib_data.calib_r_gain *256/localparam.para.calib_data.calib_g_gain);
		wb_gain.g = (uint32)(localparam.para.calib_data.calib_g_gain *256/localparam.para.calib_data.calib_g_gain);
		wb_gain.b = (uint32)(localparam.para.calib_data.calib_b_gain *256/localparam.para.calib_data.calib_g_gain);

		/* debug printf, removed for release version */
		// printf( "al3AWrapper_DispatchHW3A_AEStats get OTP WB R/G/B : (%d, %d, %d) \r\n", wb_gain.r, wb_gain.g, wb_gain.b );
	}

	/* store to stats info, translate to unit base 1000 (= 1.0x) */
	ppatched_aedat->stat_wb_2y.r = wb_gain.r*1000/256;
	ppatched_aedat->stat_wb_2y.g = wb_gain.g*1000/256;
	ppatched_aedat->stat_wb_2y.b = wb_gain.b*1000/256;

	wb_gain.r = (uint32)(wb_gain.r *  0.299 + 0.5);   /*  76.544: 0.299 * 256. */
	wb_gain.g = (uint32)(wb_gain.g *  0.587 + 0.5);  /* 150.272: 0.587 * 256. , Combine Gr Gb factor */
	wb_gain.b = (uint32)(wb_gain.b *  0.114 + 0.5);  /*  29.184: 0.114 * 256. */

	/* debug printf, removed for release version */
	// printf( "al3AWrapper_DispatchHW3A_AEStats get patching WB R/G/B : (%d, %d, %d) \r\n", wb_gain.r, wb_gain.g, wb_gain.b );

	udoffset =0;
	index = 0;
	ppatched_aedat->ucstatsdepth = 10;

	/* store patched data/common info/ae info from Wrapper */
	ppatched_aedat->umagicnum           = pmetadata_ae->umagicnum;
	ppatched_aedat->uhwengineid         = pmetadata_ae->uhwengineid;
	ppatched_aedat->uframeidx           = pmetadata_ae->uframeidx;
	ppatched_aedat->uaetokenid          = pmetadata_ae->uaetokenid;
	ppatched_aedat->uaestatssize        = pmetadata_ae->uaestatssize;
	ppatched_aedat->udpixelsperblocks   = udpixelsperblocks;
	ppatched_aedat->udbanksize          = udbanksize;
	ppatched_aedat->ucvalidblocks       = blocks;
	ppatched_aedat->ucvalidbanks        = banks;
	ppatched_aedat->upseudoflag         = pmetadata_ae->upseudoflag;

	/* debug printf, removed for release version */
	// printf( "al3AWrapper_DispatchHW3A_AEStats VerNum: %d, HWID: %d, Frmidx: %d, TokID: %d, Size: %d, PPB: %d, BZ: %d, ValidBlock: %d, ValidBank:%d, SFlag:%d \r\n",
	// ppatched_aedat->umagicnum, ppatched_aedat->uhwengineid, ppatched_aedat->uframeidx, ppatched_aedat->uaetokenid, ppatched_aedat->uaestatssize,
	// ppatched_aedat->udpixelsperblocks, ppatched_aedat->udbanksize, ppatched_aedat->ucvalidblocks, ppatched_aedat->ucvalidbanks, ppatched_aedat->upseudoflag  );

	/* store frame & timestamp */
	memcpy( &ppatched_aedat->systemtime, &pmetadata_ae->systemtime, sizeof(struct timeval));
	ppatched_aedat->udsys_sof_idx       = pmetadata_ae->udsys_sof_idx;
	ppatched_aedat->udisp_dgain       = pmetadata_ae->udisp_dgain;


	/* debug printf, removed for release version */
	// printf( "al3AWrapper_DispatchHW3A_AEStats SOF idx :%d \r\n", ppatched_aedat->udsys_sof_idx  );

	/* patching R/G/G/B from WB or calibration WB to Y/R/G/B, */
	/* here is the sample which use array passing insetad of pointer passing of prepared buffer from AE ctrl layer */
	/* SPRD suggest use by addr of pre-allocated buffer as default value */
	ppatched_aedat->bisstatsbyaddr = TRUE;

	if ( ppatched_aedat->bisstatsbyaddr == FALSE ) {
		/* patch stats to defined array */
		pt_stats_r = (uint32 *)(ppatched_aedat->statsr);
		pt_stats_g = (uint32 *)(ppatched_aedat->statsg);
		pt_stats_b = (uint32 *)(ppatched_aedat->statsb);
		pt_stats_y = (uint32 *)(ppatched_aedat->statsy);
	} else {
		/* patch stats to pre-allocated buffer addr */
		pt_stats_r = (uint32 *)(ppatched_aedat->ptstatsr);
		pt_stats_g = (uint32 *)(ppatched_aedat->ptstatsg);
		pt_stats_b = (uint32 *)(ppatched_aedat->ptstatsb);
		pt_stats_y = (uint32 *)(ppatched_aedat->ptstatsy);

		if ( (pt_stats_r == NULL) || (pt_stats_g == NULL) || (pt_stats_b == NULL) || (pt_stats_y == NULL) ) {
			/* reset flag */
			ppatched_aedat->bisstatsbyaddr = FALSE;
			/* patch stats to defined array */
			pt_stats_r = (uint32 *)(ppatched_aedat->statsr);
			pt_stats_g = (uint32 *)(ppatched_aedat->statsg);
			pt_stats_b = (uint32 *)(ppatched_aedat->statsb);
			pt_stats_y = (uint32 *)(ppatched_aedat->statsy);
		}

	}

	for ( j =0; j <banks; j++ ) {
		udoffset = udbanksize*j;
		for ( i = 0; i <blocks; i++ ) {
			/* due to data from HW, use direct address instead of casting */
			pt_stats_r[index] = ( stats[udoffset] + (stats[udoffset+1]<<8) + (stats[udoffset+2]<<16) + (stats[udoffset+3]<<24) )/udpixelsperblocks;  /* 10 bits */
			pt_stats_g[index] = (( ( stats[udoffset+4] + (stats[udoffset+5]<<8) + (stats[udoffset+6]<<16) + (stats[udoffset+7]<<24) ) +
			                       ( stats[udoffset+8] + (stats[udoffset+9]<<8) + (stats[udoffset+10]<<16) + (stats[udoffset+11]<<24) ) )/udpixelsperblocks)>>1; /* 10 bits */
			pt_stats_b[index] = ( stats[udoffset+12] + (stats[udoffset+13]<<8) + (stats[udoffset+14]<<16) + (stats[udoffset+15]<<24) )/udpixelsperblocks;    /* 10 bits */

			/* calculate Y */
			pt_stats_y[index] = ( pt_stats_r[index]* wb_gain.r + pt_stats_g[index] * wb_gain.g +
			                      pt_stats_b[index] * wb_gain.b ) >> 8;  /* 10 bits */

			/* debug printf, removed for release version */
			// printf( "al3awrapper_dispatchhw3a_aestats stats[%d] y/r/g/b: %d, %d, %d, %d \r\n", index,
			//         pt_stats_y[index],pt_stats_r[index], pt_stats_g[index], pt_stats_b[index] );

			index++;
			udoffset += 16;
		}
	}
	return ret;
}

/*
 * API name: al3AWrapperAE_UpdateOTP2AELib
 * This API is used for set correct HW3A config (Altek ISP) to generate correct AE stats
 * param aCalibWBGain[in]: calibration data from OTP
 * param alAERuntimeObj_t[in]: callback lookup table, must passing correct table into this API for querying HW3A config
 * param ae_output[in]: returned output result after calling ae lib estimation
 * param ae_runtimedat[in]: AE lib runtime buffer after calling init, must passing correct addr to into this API
 * return: error code
 */
uint32 al3awrapperae_updateotp2aelib( struct calib_wb_gain_t acalibwbgain,  struct alaeruntimeobj_t *aaelibcallback, struct ae_output_data_t *ae_output , void * ae_runtimedat )
{
	uint32 ret = ERR_WPR_AE_SUCCESS;
	struct ae_set_param_t localparam;    /* local parameter set */

	if ( acalibwbgain.r == 0 || acalibwbgain.g == 0 || acalibwbgain.b == 0 )
		return ERR_WRP_AE_INVALID_INPUT_WB;

	memset( &localparam, 0, sizeof(struct ae_set_param_content_t) );
	localparam.ae_set_param_type = AE_SET_PARAM_OTP_WB_DAT;  /* ask AE lib for HW3A setting */
	localparam.set_param.ae_calib_wb_gain.calib_r_gain = acalibwbgain.r;   /* scale 1000 base, if source is already scaled by 1000, direcly passing to AE lib */
	localparam.set_param.ae_calib_wb_gain.calib_g_gain = acalibwbgain.g;   /* scale 1000 base, if source is already scaled by 1000, direcly passing to AE lib */
	localparam.set_param.ae_calib_wb_gain.calib_b_gain = acalibwbgain.b;   /* scale 1000 base, if source is already scaled by 1000, direcly passing to AE lib */
	localparam.set_param.ae_calib_wb_gain.minISO        = acalibwbgain.minISO;    /*  from calibration result, indicate minimun ISO which could be used */

	if ( localparam.set_param.ae_calib_wb_gain.minISO == 0 )
		localparam.set_param.ae_calib_wb_gain.minISO   = _DEFAULT_WRAPPER_MINISO;

	ret = aaelibcallback->set_param( &localparam, ae_output, ae_runtimedat );
	if ( ret != ERR_WPR_AE_SUCCESS )
		return ret;

	return ret;
}

/*
 * API name: al3AWrapperAE_UpdateOTP2AELib
 * This API is used for set correct HW3A config (Altek ISP) to generate correct AE stats
 * param aAEReport[in]: ae report from AE update
 * return: error code
 */
uint32 al3awrapper_updateaestate( struct ae_report_update_t* aaereport )
{
	uint32 ret = ERR_WPR_AE_SUCCESS;

	UNUSED(aaereport);
	/* state should follow framework define */
	/* initial state, before AE running, state should be non-initialed state */


	/* converged state, after converge for over 2 ~ x run, should be converge state */


	/* running state, when lib running, not converged, should be running state */


	/* lock state, if framework (or other lib, such as AF) request AE to pause running, would be locked state */


	/* disable state, only when framework disable AE lib, state would be disable state */

	return ret;
}

/*
 * API name: al3AWrapperAE_TranslateSceneModeFromAELib2AP
 * This API used for translating AE lib scene define to framework
 * param aSceneMode[In] :   scene mode define of AE lib (Altek)
 * return: scene mode define for AP framework
 */
uint32 al3awrapperae_translatescenemodefromaelib2ap( uint32 ascenemode )
{
	uint32 retapscenemode;

	/* here just sample code, need to be implement by Framework define */
	switch (ascenemode) {
	case 0:  /* scene 0 */
		retapscenemode = 0;
		break;
	default:
		retapscenemode = 0;
		break;
	}
	return retapscenemode;
}

/*
 * API name: al3AWrapperAE_TranslateSceneModeFromAP2AELib
 * This API used for translating framework scene mode to AE lib define
 * aSceneMode[In] :   scene mode define of AP framework define
 * return: scene mode define for AE lib (Altek)
 */
uint32 al3awrapperae_translatescenemodefromap2aelib( uint32 ascenemode )
{
	uint32 retaescenemode;

	/* here just sample code, need to be implement by Framework define */
	switch (ascenemode) {
	case 0:  /* scene 0 */
		retaescenemode = 0;
		break;
	default:
		retaescenemode = 0;
		break;
	}
	return retaescenemode;
}

/*
 * API name: al3AWrapperAE_QueryISPConfig_AE
 * This API is used for query ISP config before calling al3AWrapperAE_UpdateISPConfig_AE
 * param aAEConfig[out]: API would manage parameter and return via this pointer
 * param aaelibcallback[in]: callback lookup table, must passing correct table into this API for querying HW3A config
 * param ae_runtimedat[in]: AE lib runtime buffer after calling init, must passing correct addr to into this API
 * return: error code
 */
uint32 al3awrapperae_queryispconfig_ae( struct alhw3a_ae_cfginfo_t* aaeconfig, struct alaeruntimeobj_t *aaelibcallback, void * ae_runtimedat )
{
	uint32 ret = ERR_WPR_AE_SUCCESS;
	struct ae_get_param_t localparam;

	if ( aaeconfig == NULL || ae_runtimedat == NULL )
		return ERR_WRP_AE_INVALID_INPUT_PARAM;

	memset( &localparam, 0, sizeof(struct ae_get_param_t) );
	localparam.ae_get_param_type = AE_GET_ALHW3A_CONFIG;  /* query AE lib for HW3A setting */

	/* send command to AE lib, query setting parameter */
	ret = aaelibcallback->get_param( &localparam, ae_runtimedat );
	if ( ret != ERR_WPR_AE_SUCCESS )
		return ret;

	memcpy( aaeconfig, &localparam.para.alhw3a_aeconfig, sizeof( struct alhw3a_ae_cfginfo_t ) );  /* copy data, prepare for return */
	return ret;
}

/*
 * API name: al3AWrapperAE_GetDefaultCfg
 * This API is used for query default ISP config before calling al3AWrapperAE_UpdateISPConfig_AE
 * param aAEConfig[out]: input buffer, API would manage parameter and return via this pointer
 * return: error code
 */
uint32 al3awrapperae_getdefaultcfg( struct alhw3a_ae_cfginfo_t* aaeconfig)
{
	uint32 ret = ERR_WPR_AE_SUCCESS;
	struct alhw3a_ae_cfginfo_t localparam;

	if ( aaeconfig == NULL )
		return ERR_WRP_AE_INVALID_INPUT_PARAM;

	localparam.tokenid = 0x01;
	localparam.taeregion.uwborderratiox = 100;   /* 100% use of current sensor cropped area */
	localparam.taeregion.uwborderratioy = 100;   /* 100% use of current sensor cropped area */
	localparam.taeregion.uwblknumx = 16;         /* fixed value for ae lib */
	localparam.taeregion.uwblknumy = 16;         /* fixed value for ae lib */
	localparam.taeregion.uwoffsetratiox = 0;     /* 0% offset of left of current sensor cropped area */
	localparam.taeregion.uwoffsetratioy = 0;     /* 0% offset of top of current sensor cropped area */

	memcpy( aaeconfig, &localparam, sizeof(struct alhw3a_ae_cfginfo_t ) );
	return ret;
}



/*
 * API name: al3AWrapperAE_QueryISPConfig_AE
 * This API is used for query ISP config before calling al3AWrapperAE_UpdateISPConfig_AE
 * param a_ucSensor[in]: AHB sensor ID
 * param aAEConfig[out]: input buffer, API would manage parameter and return via this pointer
 * return: error code
 */
uint32 al3awrapperae_updateispconfig_ae( uint8 a_ucsensor, struct alhw3a_ae_cfginfo_t* aaeconfig )
{
	uint32 ret = ERR_WPR_AE_SUCCESS;
#ifdef LINK_ALTEK_ISP_DRV_DEFINE   /* only build when link to ISP driver define */
	ret = ISPDRV_AP3AMGR_SetAECfg( a_ucsensor, aaeconfig );
#else
	UNUSED(a_ucsensor);
	UNUSED(aaeconfig);
#endif
	return ret;
}

/*
 * API name: al3AWrapperAE_SetSceneSetting
 * This API is used for setting scene event file to alAELib
 * param setting_info[in]: scene info addr
 * param alAERuntimeObj_t[in]: callback lookup table, must passing correct table into this API for querying HW3A config
 * param ae_output[in]: returned output result after calling ae lib estimation
 * param ae_runtimedat[in]: AE lib runtime buffer after calling init, must passing correct addr to into this API
 * return: error code
 */
uint32 al3awrapperae_setscenesetting(void *setting_info, struct alaeruntimeobj_t *aaelibcallback, struct ae_output_data_t *ae_output , void * ae_runtimedat )
{
	uint32 ret = ERR_WPR_AE_SUCCESS;
	struct ae_set_param_t localparam;    /* local parameter set */
	struct scene_setting_info *info;
	struct alwrapper_scene_setting *setting;

	if (setting_info == NULL)
		return ERR_WRP_AE_INVALID_INPUT_PARAM;

	info = (struct scene_setting_info*)setting_info;
	if (info->uw_mode == Auto) {
		localparam.ae_set_param_type = AE_SET_PARAM_SCENE_SETTING;
		localparam.set_param.ae_scene_param.scene_ui_evcomp = 0;
		localparam.set_param.ae_scene_param.pcurve_idx = 0;
		localparam.set_param.ae_scene_param.exposure_priority = 0;
		localparam.set_param.ae_scene_param.manual_exptime = 0;
		localparam.set_param.ae_scene_param.manual_gain = 0;
		ret = aaelibcallback->set_param( &localparam, ae_output, ae_runtimedat );
	}
	if (info->uw_mode != Auto && info->puc_addr != NULL) {
		setting = (struct alwrapper_scene_setting*)info->puc_addr;

		localparam.ae_set_param_type = AE_SET_PARAM_SCENE_SETTING;
		localparam.set_param.ae_scene_param.scene_ui_evcomp = setting->d_ae_ev_comp;
		localparam.set_param.ae_scene_param.pcurve_idx = setting->d_ae_p_curve;
		localparam.set_param.ae_scene_param.exposure_priority = 0;
		localparam.set_param.ae_scene_param.manual_exptime = 0;
		localparam.set_param.ae_scene_param.manual_gain = 0;
		ret = aaelibcallback->set_param( &localparam, ae_output, ae_runtimedat );
	}
	return ret;
}

/*
 * API name: al3AWrapperAE_GetVersion
 * This API would return labeled version of wrapper
 * fWrapVersion[out], return current wapper version
 * return: error code
 */
uint32 al3awrapperae_getversion( float *fwrapversion )
{
	uint32 ret = ERR_WPR_AE_SUCCESS;

	*fwrapversion = _WRAPPER_AE_VER;

	return ret;
}

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