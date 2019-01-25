/******************************************************************************
 * alwrapper_3a.c
 *
 *  Created on: 2015/12/05
 *      Author: MarkTseng
 *  Latest update: 2016/5/31
 *      Reviser: MarkTseng
 *  Comments:
 *       This c file is mainly used for AP framework to:
 *       1. Dispatch ISP stats to seperated stats
 *       2. Get DL seuqence by opmode
 *       3. Set DL sequence to ISP (Altek)
 ******************************************************************************/


#include <math.h>
#include <string.h>
#include <sys/time.h>

#include "mtype.h"
#include "frmwk_hw3a_event_type.h"
#include "hw3a_stats.h"
#include "alwrapper_3a.h"
#include "alwrapper_3a_errcode.h"

#ifndef LOCAL_NDK_BUILD
#include "isp_common_types.h"
#endif


#define _ENABLE_STATS_PRE_ALLOC_BUF 1
/******************************************************************************
 * function prototype
 ******************************************************************************/


/******************************************************************************
 * Function code
 ******************************************************************************/

/*  for AF ctrl layer */
/*
 * API name: al3awrapper_dispatchhw3astats_af
 * This API used for copying stats data from HW ISP(Altek) to AF buffer, but without further patching
 * Framework should call al3AWrapper_DispatchHW3A_XXStats in certain thread for patching, after patching completed, send event
 * to XX Ctrl layer, prepare for process
 * param alisp_metadata[In] :   meta data address from ISP driver, passing via AP framework
 * param alisp_metadata_af[Out] : AF stats buffer addr, should be arranged via AF ctrl/3A ctrl layer
 * param udsof_idx[In] : current SOF index, should be from ISP driver layer
 * return: error code
 */
uint32 al3awrapper_dispatchhw3astats_af( void * alisp_metadata, struct isp_drv_meta_af_t * alisp_metadata_af, uint32 udsof_idx )
{
	uint32 ret = ERR_WRP_SUCCESS;
	uint8 *paddrlocal;
	struct isp_drv_meta_t pispmeta;
	struct isp_drv_meta_af_t *pispmetaaf;
	uint16  uwValidstats_Num =0;

	uint8 *ptempaddr;
	uint32 offset_start, offset, padding;

	struct timeval systemtime, systemtime_e;
	uint64  time_s, time_e;
	uint64  time_1, time_2;

	/* NULL pointer protection */
	if ( alisp_metadata == NULL )
		return ERR_WRP_EMPTY_METADATA;

	/* store timestamp at beginning, for AF usage */
	gettimeofday(&systemtime, NULL);
	time_s = systemtime.tv_sec * 1000000 + systemtime.tv_usec;

	paddrlocal = (uint8 *)alisp_metadata;
	/* by byte parsing, use casting would cause some compiler padding shift issue */
	/* 0. common Info */
	offset = 0;
	//memcpy( &pispmeta.umagicnum        , paddrlocal, 4 );
	pispmeta.umagicnum = (paddrlocal[offset] + (paddrlocal[offset+1]<<8) + (paddrlocal[offset+2]<<16) + (paddrlocal[offset+3]<<24));
	//paddrlocal+= 4;
	offset+= 4;
	//memcpy( &pispmeta.uhwengineid      , paddrlocal, 2 );
	pispmeta.uhwengineid = ((paddrlocal[offset]) + (paddrlocal[offset+1]<<8));
	//paddrlocal+= 2;
	offset+= 2;
	//memcpy( &pispmeta.uframeidx        , paddrlocal, 2 );
	pispmeta.uframeidx = ((paddrlocal[offset]) + (paddrlocal[offset+1]<<8));
	//paddrlocal+= 2;
	offset+= 2;
	//memcpy( &pispmeta.uchecksum        , paddrlocal, 4 );
	pispmeta.uchecksum = (paddrlocal[offset] + (paddrlocal[offset+1]<<8) + (paddrlocal[offset+2]<<16) + (paddrlocal[offset+3]<<24));
	//paddrlocal+= 4;
	offset+= 4;

	/* check HW3A stats's type ID */
	if ( (pispmeta.umagicnum & 0xFF000000) != HW3A_VERSION_NUMBER_AF_TYPEID ) {
		ret =  ERR_WRP_MISMACTHED_AFSTATS_VER;	/* keep error code only in early phase, if need return immediatedly then return here */
		return ret;
	}

	/*   1. af stats info  */
	//memcpy( &pispmeta.uaftag           , paddrlocal, 2 );
	pispmeta.uaftag = ((paddrlocal[offset]) + (paddrlocal[offset+1]<<8));
	//paddrlocal+= 2;
	offset+= 2;
	//memcpy( &pispmeta.uaftokenid       , paddrlocal, 2 );
	pispmeta.uaftokenid = ((paddrlocal[offset]) + (paddrlocal[offset+1]<<8));
	//paddrlocal+= 2;
	offset+= 2;
	//memcpy( &pispmeta.uafstatssize     , paddrlocal, 4 );
	pispmeta.uafstatssize = (paddrlocal[offset] + (paddrlocal[offset+1]<<8) + (paddrlocal[offset+2]<<16) + (paddrlocal[offset+3]<<24));
	//paddrlocal+= 4;
	offset+= 4;
	//memcpy( &pispmeta.uafstatsaddr     , paddrlocal, 4 );
	pispmeta.uafstatsaddr = (paddrlocal[offset] + (paddrlocal[offset+1]<<8) + (paddrlocal[offset+2]<<16) + (paddrlocal[offset+3]<<24));
	//paddrlocal+= 4;
	offset+= 4;

	/* check HW3A stats define version with magic number, which should be maintained & update at each ISP release */
	if ( (pispmeta.umagicnum & 0x00FFFFFF) != HW3A_MAGIC_NUMBER_VERSION )
		ret =  ERR_WRP_MISMACTHED_STATS_VER;	/* keep error code only in early phase, if need return immediatedly then return here */

	gettimeofday(&systemtime, NULL);
	time_1 = systemtime.tv_sec * 1000000 + systemtime.tv_usec;

	/* check data validity, if data is empty, reset buffer to AF lib, avoid corrupt data to crash system  */
	if ( (pispmeta.uafstatsaddr == 0 || pispmeta.uafstatssize == 0  ) && alisp_metadata_af != NULL ) {
		memset( alisp_metadata_af, 0, sizeof( struct  isp_drv_meta_af_t ));
	}

	gettimeofday(&systemtime, NULL);
	time_2 = systemtime.tv_sec * 1000000 + systemtime.tv_usec;

	/* parsing AF if AF pointer is valid */
	if ( alisp_metadata_af == NULL )
		;	/* do nothing, even if HW3A stats passing valid data */
	else if ( pispmeta.uafstatssize != 0 && pispmeta.uafstatsaddr != 0 ) {
		pispmetaaf = (struct isp_drv_meta_af_t *)alisp_metadata_af;
		/* udpate common info */
		pispmetaaf->umagicnum = pispmeta.umagicnum;
		pispmetaaf->uhwengineid = pispmeta.uhwengineid;
		pispmetaaf->uframeidx = pispmeta.uframeidx;

		/* update af info */
		pispmetaaf->uaftokenid = pispmeta.uaftokenid;
		pispmetaaf->uafstatssize = pispmeta.uafstatssize;
		pispmetaaf->upseudoflag = 0;

		memcpy( &pispmetaaf->systemtime, &systemtime, sizeof(struct timeval));
		pispmetaaf->udsys_sof_idx = udsof_idx;

		/* retriving af info of stats */
		paddrlocal = (uint8 *)alisp_metadata + pispmeta.uafstatsaddr;
		offset = 0;
		//memcpy( &alisp_metadata_af->af_stats_info.udpixelsperblocks  , paddrlocal, 4 );
		alisp_metadata_af->af_stats_info.udpixelsperblocks = (paddrlocal[offset] + (paddrlocal[offset+1]<<8) + (paddrlocal[offset+2]<<16) + (paddrlocal[offset+3]<<24));
		//paddrlocal+= 4;
		offset+= 4;
		//memcpy( &alisp_metadata_af->af_stats_info.udbanksize         , paddrlocal, 4 );
		alisp_metadata_af->af_stats_info.udbanksize = (paddrlocal[offset] + (paddrlocal[offset+1]<<8) + (paddrlocal[offset+2]<<16) + (paddrlocal[offset+3]<<24));
		//paddrlocal+= 4;
		offset+= 4;
		//memcpy( &alisp_metadata_af->af_stats_info.ucvalidblocks      , paddrlocal, 1 );
		alisp_metadata_af->af_stats_info.ucvalidblocks = paddrlocal[offset];
		//paddrlocal+= 1;
		offset+= 1;
		//memcpy( &alisp_metadata_af->af_stats_info.ucvalidbanks       , paddrlocal, 1 );
		alisp_metadata_af->af_stats_info.ucvalidbanks = paddrlocal[offset];
		//paddrlocal+= 1;
		offset+= 1;

		/* copy af stats to indicated buffer address, make 8 alignment */
		offset_start = pispmeta.uafstatsaddr + offset;  /* offset is accumulation value of af info of stats */
		padding = (offset_start % 8 == 0 ) ? 0:(8- (offset_start % 8));
		offset_start = offset_start + padding;

		/* allocate memory buffer base on meta size of AF stats */
		if ( alisp_metadata_af->uafstatssize > HW3A_AF_STATS_BUFFER_SIZE ) {
			return ERR_WRP_ALLOCATE_BUFFER;
		}

		/* shift to data addr */
		ptempaddr = (uint8 *)alisp_metadata + offset_start;

		alisp_metadata_af->b_isstats_byaddr = 0;

#ifdef _ENABLE_STATS_PRE_ALLOC_BUF
#if ( _ENABLE_STATS_PRE_ALLOC_BUF )
		alisp_metadata_af->b_isstats_byaddr = 1;
#endif
#endif

		if ( alisp_metadata_af->b_isstats_byaddr == 0 ) {
			/* copy data to buffer addr */
			memcpy( (void *)alisp_metadata_af->paf_stats , (void *)ptempaddr, alisp_metadata_af->uafstatssize );
		} else {
			/* assign by addr if buffer is allocated from 3A ctrl layer  */
			alisp_metadata_af->puc_af_stats = (uint8 *)ptempaddr;
		}

		uwValidstats_Num++;
	} else { /* set pseudo flag */
		pispmetaaf = (struct isp_drv_meta_af_t *)alisp_metadata_af;
		/* udpate common info */
		pispmetaaf->umagicnum = pispmeta.umagicnum;
		pispmetaaf->uhwengineid = pispmeta.uhwengineid;
		pispmetaaf->uframeidx = pispmeta.uframeidx;

		/* update af info */
		pispmetaaf->uaftokenid = pispmeta.uaftokenid;
		pispmetaaf->uafstatssize = pispmeta.uafstatssize;
		pispmetaaf->upseudoflag = 1;

		memcpy( &pispmetaaf->systemtime, &systemtime, sizeof(struct timeval));
		pispmetaaf->udsys_sof_idx = udsof_idx;

		if ( pispmeta.uafstatsaddr != 0) {
			/* retriving af info of stats */
			paddrlocal = (uint8 *)alisp_metadata + pispmeta.uafstatsaddr;
			offset = 0;
			//memcpy( &alisp_metadata_af->af_stats_info.udpixelsperblocks  , paddrlocal, 4 );
			alisp_metadata_af->af_stats_info.udpixelsperblocks = (paddrlocal[offset] + (paddrlocal[offset+1]<<8) + (paddrlocal[offset+2]<<16) + (paddrlocal[offset+3]<<24));
			//paddrlocal+= 4;
			offset+= 4;
			//memcpy( &alisp_metadata_af->af_stats_info.udbanksize         , paddrlocal, 4 );
			alisp_metadata_af->af_stats_info.udbanksize = (paddrlocal[offset] + (paddrlocal[offset+1]<<8) + (paddrlocal[offset+2]<<16) + (paddrlocal[offset+3]<<24));
			//paddrlocal+= 4;
			offset+= 4;
			//memcpy( &alisp_metadata_af->af_stats_info.ucvalidblocks      , paddrlocal, 1 );
			alisp_metadata_af->af_stats_info.ucvalidblocks = paddrlocal[offset];
			//paddrlocal+= 1;
			offset+= 1;
			//memcpy( &alisp_metadata_af->af_stats_info.ucvalidbanks       , paddrlocal, 1 );
			alisp_metadata_af->af_stats_info.ucvalidbanks = paddrlocal[offset];
		}

		pispmetaaf->b_isstats_byaddr = 0;
	}

	if ( uwValidstats_Num == 0 )
		return ERR_WRP_EMPTY_METADATA;

	gettimeofday(&systemtime_e, NULL);
	time_e = systemtime_e.tv_sec * 1000000 + systemtime_e.tv_usec;

#ifndef LOCAL_NDK_BUILD
	ISP_LOGI("al3awrapper_dispatchhw3astats process total_time:%d,commom_t:%d,memsetAF:%d,af:%d\r\n", (uint32)(time_e - time_s),(uint32)(time_1 - time_s),(uint32)(time_2 - time_1),(uint32)(time_e - time_2));
#endif


	return ret;
}

/*  for 3A ctrl layer */
/*
 * API name: al3awrapper_dispatchhw3astats
 * This API used for copying stats data from HW ISP(Altek) to seperated buffer, but without further patching
 * Framework should call al3AWrapper_DispatchHW3A_XXStats in certain thread for patching, after patching completed, send event
 * to XX Ctrl layer, prepare for process
 * param alisp_metadata[In] :   meta data address from ISP driver, passing via AP framework
 * param alisp_metadata_ae[Out] : AE stats buffer addr, should be arranged via AE ctrl/3A ctrl layer
 * param alisp_metadata_awb[Out] : AWB stats buffer addr, should be arranged via AWB ctrl/3A ctrl layer
 * param alisp_metadata_af[Out] : AF stats buffer addr, should be arranged via AF ctrl/3A ctrl layer
 * param alisp_metadata_yhist[Out] : YHist stats buffer addr, should be arranged via 3A ctrl layer
 * param alisp_metadata_antif[Out] : AntiFlicker stats buffer addr, should be arranged via anti-flicker ctrl/3A ctrl layer
 * param alisp_metadata_subsample[Out] : subsample buffer addr, should be arranged via AF ctrl/3A ctrl layer
 * param udsof_idx[In] : current SOF index, should be from ISP driver layer
 * return: error code
 */
uint32 al3awrapper_dispatchhw3astats( void * alisp_metadata, struct isp_drv_meta_ae_t* alisp_metadata_ae,
                                      struct isp_drv_meta_awb_t* alisp_metadata_awb, struct isp_drv_meta_af_t * alisp_metadata_af,
                                      struct isp_drv_meta_yhist_t * alisp_metadata_yhist, struct isp_drv_meta_antif_t * alisp_metadata_antif,
                                      struct isp_drv_meta_subsample_t * alisp_metadata_subsample, uint32 udsof_idx  )
{
	uint32 ret = ERR_WRP_SUCCESS;
	uint8 *paddrlocal;
	struct isp_drv_meta_t pispmeta;
	struct isp_drv_meta_ae_t *pispmetaae;
	struct isp_drv_meta_awb_t *pispmetaawb;
	struct isp_drv_meta_af_t *pispmetaaf;
	struct isp_drv_meta_yhist_t *pispmetayhist;
	struct isp_drv_meta_antif_t *pispmetaantif;
	struct isp_drv_meta_subsample_t *pispmetasubsample;
	uint16  uwValidstats_Num =0;

	uint8 *ptempaddr;
	uint32 offset_start, offset, padding;

	struct timeval systemtime, systemtime_e;
	uint64  time_s, time_e;
	uint64  time_1, time_2;
	uint64  time_3, time_4;
	uint64  time_5, time_6;
	uint64  time_7, time_8 ,time_9;

	/* NULL pointer protection */
	if ( alisp_metadata == NULL )
		return ERR_WRP_EMPTY_METADATA;

	/* store timestamp at beginning, for AF usage */
	gettimeofday(&systemtime, NULL);
	time_s = systemtime.tv_sec * 1000000 + systemtime.tv_usec;

	paddrlocal = (uint8 *)alisp_metadata;
	/* by byte parsing, use casting would cause some compiler padding shift issue */
	/* 0. common Info */
	offset = 0;
	//memcpy( &pispmeta.umagicnum        , paddrlocal, 4 );
	pispmeta.umagicnum = (paddrlocal[offset] + (paddrlocal[offset+1]<<8) + (paddrlocal[offset+2]<<16) + (paddrlocal[offset+3]<<24));
	//paddrlocal+= 4;
	offset+= 4;
	//memcpy( &pispmeta.uhwengineid      , paddrlocal, 2 );
	pispmeta.uhwengineid = ((paddrlocal[offset]) + (paddrlocal[offset+1]<<8));
	//paddrlocal+= 2;
	offset+= 2;
	//memcpy( &pispmeta.uframeidx        , paddrlocal, 2 );
	pispmeta.uframeidx = ((paddrlocal[offset]) + (paddrlocal[offset+1]<<8));
	//paddrlocal+= 2;
	offset+= 2;
	//memcpy( &pispmeta.uchecksum        , paddrlocal, 4 );
	pispmeta.uchecksum = (paddrlocal[offset] + (paddrlocal[offset+1]<<8) + (paddrlocal[offset+2]<<16) + (paddrlocal[offset+3]<<24));
	//paddrlocal+= 4;
	offset+= 4;

	/* check HW3A stats's type ID */
	if ( (pispmeta.umagicnum & 0xFF000000) != HW3A_VERSION_NUMBER_TYPEID ) {
		ret =  ERR_WRP_MISMACTHED_ISPSTATS_VER;	/* keep error code only in early phase, if need return immediatedly then return here */
		return ret;
	}

	// Data D Gain
	offset += 16;

	/*  1. ae  stats info */
	//memcpy( &pispmeta.uaetag           , paddrlocal, 2 );
	pispmeta.uaetag = ((paddrlocal[offset]) + (paddrlocal[offset+1]<<8));
	//paddrlocal+= 2;
	offset+= 2;
	//memcpy( &pispmeta.uaetokenid       , paddrlocal, 2 );
	pispmeta.uaetokenid = ((paddrlocal[offset]) + (paddrlocal[offset+1]<<8));
	//paddrlocal+= 2;
	offset+= 2;
	//memcpy( &pispmeta.uaestatssize     , paddrlocal, 4 );
	pispmeta.uaestatssize = (paddrlocal[offset] + (paddrlocal[offset+1]<<8) + (paddrlocal[offset+2]<<16) + (paddrlocal[offset+3]<<24));
	//paddrlocal+= 4;
	offset+= 4;
	//memcpy( &pispmeta.uaestatsaddr     , paddrlocal, 4 );
	pispmeta.uaestatsaddr = (paddrlocal[offset] + (paddrlocal[offset+1]<<8) + (paddrlocal[offset+2]<<16) + (paddrlocal[offset+3]<<24));
	//paddrlocal+= 4;
	offset+= 4;

	/*   2. awb stats info */
	//memcpy( &pispmeta.uawbtag          , paddrlocal, 2 );
	pispmeta.uawbtag = ((paddrlocal[offset]) + (paddrlocal[offset+1]<<8));
	//paddrlocal+= 2;
	offset+= 2;
	//memcpy( &pispmeta.uawbtokenid      , paddrlocal, 2 );
	pispmeta.uawbtokenid = ((paddrlocal[offset]) + (paddrlocal[offset+1]<<8));
	//paddrlocal+= 2;
	offset+= 2;
	//memcpy( &pispmeta.uawbstatssize    , paddrlocal, 4 );
	pispmeta.uawbstatssize = (paddrlocal[offset] + (paddrlocal[offset+1]<<8) + (paddrlocal[offset+2]<<16) + (paddrlocal[offset+3]<<24));
	//paddrlocal+= 4;
	offset+= 4;
	//memcpy( &pispmeta.uawbstatsaddr    , paddrlocal, 4 );
	pispmeta.uawbstatsaddr = (paddrlocal[offset] + (paddrlocal[offset+1]<<8) + (paddrlocal[offset+2]<<16) + (paddrlocal[offset+3]<<24));
	//paddrlocal+= 4;
	offset+= 4;

	/*   3. af stats info  */
	//memcpy( &pispmeta.uaftag           , paddrlocal, 2 );
	pispmeta.uaftag = ((paddrlocal[offset]) + (paddrlocal[offset+1]<<8));
	//paddrlocal+= 2;
	offset+= 2;
	//memcpy( &pispmeta.uaftokenid       , paddrlocal, 2 );
	pispmeta.uaftokenid = ((paddrlocal[offset]) + (paddrlocal[offset+1]<<8));
	//paddrlocal+= 2;
	offset+= 2;
	//memcpy( &pispmeta.uafstatssize     , paddrlocal, 4 );
	pispmeta.uafstatssize = (paddrlocal[offset] + (paddrlocal[offset+1]<<8) + (paddrlocal[offset+2]<<16) + (paddrlocal[offset+3]<<24));
	//paddrlocal+= 4;
	offset+= 4;
	//memcpy( &pispmeta.uafstatsaddr     , paddrlocal, 4 );
	pispmeta.uafstatsaddr = (paddrlocal[offset] + (paddrlocal[offset+1]<<8) + (paddrlocal[offset+2]<<16) + (paddrlocal[offset+3]<<24));
	//paddrlocal+= 4;
	offset+= 4;

	/*   4. y-hist stats info  */
	//memcpy( &pispmeta.uyhisttag        , paddrlocal, 2 );
	pispmeta.uyhisttag = ((paddrlocal[offset]) + (paddrlocal[offset+1]<<8));
	//paddrlocal+= 2;
	offset+= 2;
	//memcpy( &pispmeta.uyhisttokenid    , paddrlocal, 2 );
	pispmeta.uyhisttokenid = ((paddrlocal[offset]) + (paddrlocal[offset+1]<<8));
	//paddrlocal+= 2;
	offset+= 2;
	//memcpy( &pispmeta.uyhiststatssize  , paddrlocal, 4 );
	pispmeta.uyhiststatssize = (paddrlocal[offset] + (paddrlocal[offset+1]<<8) + (paddrlocal[offset+2]<<16) + (paddrlocal[offset+3]<<24));
	//paddrlocal+= 4;
	offset+= 4;
	//memcpy( &pispmeta.uyhiststatsaddr  , paddrlocal, 4 );
	pispmeta.uyhiststatsaddr = (paddrlocal[offset] + (paddrlocal[offset+1]<<8) + (paddrlocal[offset+2]<<16) + (paddrlocal[offset+3]<<24));
	//paddrlocal+= 4;
	offset+= 4;

	/*   5. anfi-flicker stats info */
	//memcpy( &pispmeta.uantiftag        , paddrlocal, 2 );
	pispmeta.uantiftag = ((paddrlocal[offset]) + (paddrlocal[offset+1]<<8));
	//paddrlocal+= 2;
	offset+= 2;
	//memcpy( &pispmeta.uantiftokenid    , paddrlocal, 2 );
	pispmeta.uantiftokenid = ((paddrlocal[offset]) + (paddrlocal[offset+1]<<8));
	//paddrlocal+= 2;
	offset+= 2;
	//memcpy( &pispmeta.uantifstatssize  , paddrlocal, 4 );
	pispmeta.uantifstatssize = (paddrlocal[offset] + (paddrlocal[offset+1]<<8) + (paddrlocal[offset+2]<<16) + (paddrlocal[offset+3]<<24));
	//paddrlocal+= 4;
	offset+= 4;
	//memcpy( &pispmeta.uantifstatsaddr  , paddrlocal, 4 );
	pispmeta.uantifstatsaddr = (paddrlocal[offset] + (paddrlocal[offset+1]<<8) + (paddrlocal[offset+2]<<16) + (paddrlocal[offset+3]<<24));
	//paddrlocal+= 4;
	offset+= 4;

	/*   6. subsample image info   */
	//memcpy( &pispmeta.usubsampletag        , paddrlocal, 2 );
	pispmeta.usubsampletag = ((paddrlocal[offset]) + (paddrlocal[offset+1]<<8));
	//paddrlocal+= 2;
	offset+= 2;
	//memcpy( &pispmeta.usubsampletokenid    , paddrlocal, 2 );
	pispmeta.usubsampletokenid = ((paddrlocal[offset]) + (paddrlocal[offset+1]<<8));
	//paddrlocal+= 2;
	offset+= 2;
	//memcpy( &pispmeta.usubsamplestatssize  , paddrlocal, 4 );
	pispmeta.usubsamplestatssize = (paddrlocal[offset] + (paddrlocal[offset+1]<<8) + (paddrlocal[offset+2]<<16) + (paddrlocal[offset+3]<<24));
	//paddrlocal+= 4;
	offset+= 4;
	//memcpy( &pispmeta.usubsamplestatsaddr  , paddrlocal, 4 );
	pispmeta.usubsamplestatsaddr = (paddrlocal[offset] + (paddrlocal[offset+1]<<8) + (paddrlocal[offset+2]<<16) + (paddrlocal[offset+3]<<24));
	//paddrlocal+= 4;
	offset+= 4;

	/* check HW3A stats define version with magic number, which should be maintained & update at each ISP release */
	if ( (pispmeta.umagicnum & 0x00FFFFFF) != HW3A_MAGIC_NUMBER_VERSION )
		ret =  ERR_WRP_MISMACTHED_STATS_VER;	/* keep error code only in early phase, if need return immediatedly then return here */

	gettimeofday(&systemtime, NULL);
	time_1 = systemtime.tv_sec * 1000000 + systemtime.tv_usec;



	/* check data validity, if data is empty, reset buffer to 3A lib, avoid corrupt data to crash system  */
	if ( ( pispmeta.uaestatsaddr == 0 || pispmeta.uaestatssize == 0  )   && alisp_metadata_ae != NULL ) {
		memset( alisp_metadata_ae , 0, sizeof( struct isp_drv_meta_ae_t ));
	}
	if ( (pispmeta.uawbstatsaddr == 0 || pispmeta.uawbstatssize == 0  )  &&  alisp_metadata_awb != NULL ) {
		memset( alisp_metadata_awb , 0, sizeof( struct  isp_drv_meta_awb_t ));
	}
	if ( (pispmeta.uafstatsaddr == 0 || pispmeta.uafstatssize == 0  ) && alisp_metadata_af != NULL ) {
		memset( alisp_metadata_af, 0, sizeof( struct  isp_drv_meta_af_t ));
	}
	if ( (pispmeta.uyhiststatsaddr == 0 || pispmeta.uyhiststatssize == 0  ) && alisp_metadata_yhist != NULL ) {
		memset( alisp_metadata_yhist, 0, sizeof( struct  isp_drv_meta_yhist_t ));
	}

	gettimeofday(&systemtime, NULL);
	time_2 = systemtime.tv_sec * 1000000 + systemtime.tv_usec;


	if ( (pispmeta.uantifstatsaddr == 0 || pispmeta.uantifstatssize == 0  ) && alisp_metadata_antif != NULL ) {
		memset( alisp_metadata_antif, 0, sizeof( struct  isp_drv_meta_antif_t ));
#ifndef LOCAL_NDK_BUILD
		//ISP_LOGI("al3awrapper_dispatchhw3astats anti-flicker invalid addr: %d , statssize:%d", pispmeta.uantifstatsaddr, pispmeta.uantifstatssize );
#endif
	}

	gettimeofday(&systemtime, NULL);
	time_3 = systemtime.tv_sec * 1000000 + systemtime.tv_usec;

	if ( (pispmeta.usubsamplestatsaddr == 0  || pispmeta.usubsamplestatssize == 0  ) && alisp_metadata_subsample != NULL ) {
		memset( alisp_metadata_subsample, 0, sizeof( struct  isp_drv_meta_subsample_t ));
	}

	gettimeofday(&systemtime, NULL);
	time_4= systemtime.tv_sec * 1000000 + systemtime.tv_usec;


	/* parsing AE if AE pointer is valid */
	if ( alisp_metadata_ae == NULL )
		; /* do nothing, even if HW3A stats passing valid data */
	else if ( pispmeta.uaestatssize != 0 && pispmeta.uaestatsaddr != 0 ) {
		pispmetaae = (struct isp_drv_meta_ae_t *)alisp_metadata_ae;
		/* udpate common info */
		pispmetaae->umagicnum = pispmeta.umagicnum;
		pispmetaae->uhwengineid = pispmeta.uhwengineid;
		pispmetaae->uframeidx = pispmeta.uframeidx;

		/* update ae info */
		pispmetaae->uaetokenid = pispmeta.uaetokenid;
		pispmetaae->uaestatssize = pispmeta.uaestatssize;
		pispmetaae->upseudoflag  = 0;

		memcpy( &pispmetaae->systemtime, &systemtime, sizeof(struct timeval));
		pispmetaae->udsys_sof_idx = udsof_idx;
		pispmetaae->udisp_dgain = 100;

		/* retriving AE info of stats */
		paddrlocal = (uint8 *)alisp_metadata + pispmeta.uaestatsaddr;
		offset = 0;
		//memcpy( &alisp_metadata_ae->ae_stats_info.udpixelsperblocks  , paddrlocal, 4 );
		alisp_metadata_ae->ae_stats_info.udpixelsperblocks = (paddrlocal[offset] + (paddrlocal[offset+1]<<8) + (paddrlocal[offset+2]<<16) + (paddrlocal[offset+3]<<24));
		//paddrlocal+= 4;
		offset+=4;
		//memcpy( &alisp_metadata_ae->ae_stats_info.udbanksize         , paddrlocal, 4 );
		alisp_metadata_ae->ae_stats_info.udbanksize = (paddrlocal[offset] + (paddrlocal[offset+1]<<8) + (paddrlocal[offset+2]<<16) + (paddrlocal[offset+3]<<24));
		//paddrlocal+= 4;
		offset+=4;
		//memcpy( &alisp_metadata_ae->ae_stats_info.ucvalidblocks      , paddrlocal, 1 );
		alisp_metadata_ae->ae_stats_info.ucvalidblocks = paddrlocal[offset];
		//paddrlocal+= 1;
		offset+=1;
		//memcpy( &alisp_metadata_ae->ae_stats_info.ucvalidbanks       , paddrlocal, 1 );
		alisp_metadata_ae->ae_stats_info.ucvalidbanks = paddrlocal[offset];
		//paddrlocal+= 1;
		offset+=1;

		/* copy AE stats to indicated buffer address, make 8 alignment */
		offset_start = pispmeta.uaestatsaddr + offset;		/* 10 is accumulation value of ae info of stats, 4+4+1+1 */
		padding = (offset_start % 8 == 0 ) ? 0:(8- (offset_start % 8));
		offset_start = offset_start + padding;

		/* allocate memory buffer base on meta size of AE stats */
		if ( alisp_metadata_ae->uaestatssize > HW3A_AE_STATS_BUFFER_SIZE ) {
			return ERR_WRP_ALLOCATE_BUFFER;
		}

		/* shift to data addr */
		ptempaddr = (uint8 *)alisp_metadata + offset_start;

		alisp_metadata_ae->b_isstats_byaddr = 0;

		/* check pre-allocated buffer validity */
#ifdef _ENABLE_STATS_PRE_ALLOC_BUF
#if ( _ENABLE_STATS_PRE_ALLOC_BUF )
		alisp_metadata_ae->b_isstats_byaddr = 1;
#endif
#endif

		if ( alisp_metadata_ae->b_isstats_byaddr == 0 ) {
			/* copy data to buffer addr */
			memcpy( (void *)alisp_metadata_ae->pae_stats , (void *)ptempaddr, alisp_metadata_ae->uaestatssize  );
		} else {
			/* assign by addr if buffer is allocated from 3A ctrl layer  */
			alisp_metadata_ae->puc_ae_stats = (uint8 *)ptempaddr;
		}

		uwValidstats_Num++;
	} else { /* set pseudo flag, for AE lib progressive runing */
		pispmetaae = (struct isp_drv_meta_ae_t *)alisp_metadata_ae;
		/* udpate common info */
		pispmetaae->umagicnum = pispmeta.umagicnum;
		pispmetaae->uhwengineid = pispmeta.uhwengineid;
		pispmetaae->uframeidx = pispmeta.uframeidx;

		/* update ae info */
		pispmetaae->uaetokenid = pispmeta.uaetokenid;
		pispmetaae->uaestatssize = pispmeta.uaestatssize;
		pispmetaae->upseudoflag = 1;

		memcpy( &pispmetaae->systemtime, &systemtime, sizeof(struct timeval));
		pispmetaae->udsys_sof_idx = udsof_idx;
		pispmetaae->udisp_dgain = 100;


		if ( pispmeta.uaestatsaddr != 0 ) {
			/* retriving AE info of stats */
			paddrlocal = (uint8 *)alisp_metadata + pispmeta.uaestatsaddr;
			offset = 0;
			//memcpy( &alisp_metadata_ae->ae_stats_info.udpixelsperblocks  , paddrlocal, 4 );
			alisp_metadata_ae->ae_stats_info.udpixelsperblocks = (paddrlocal[offset] + (paddrlocal[offset+1]<<8) + (paddrlocal[offset+2]<<16) + (paddrlocal[offset+3]<<24));
			//paddrlocal+= 4;
			offset+=4;
			//memcpy( &alisp_metadata_ae->ae_stats_info.udbanksize         , paddrlocal, 4 );
			alisp_metadata_ae->ae_stats_info.udbanksize = (paddrlocal[offset] + (paddrlocal[offset+1]<<8) + (paddrlocal[offset+2]<<16) + (paddrlocal[offset+3]<<24));
			//paddrlocal+= 4;
			offset+=4;
			//memcpy( &alisp_metadata_ae->ae_stats_info.ucvalidblocks      , paddrlocal, 1 );
			alisp_metadata_ae->ae_stats_info.ucvalidblocks = paddrlocal[offset];
			//paddrlocal+= 1;
			offset+=1;
			//memcpy( &alisp_metadata_ae->ae_stats_info.ucvalidbanks       , paddrlocal, 1 );
			alisp_metadata_ae->ae_stats_info.ucvalidbanks = paddrlocal[offset];

		} else {
			alisp_metadata_ae->ae_stats_info.udpixelsperblocks = 3000;
			alisp_metadata_ae->ae_stats_info.udbanksize = 512;
			alisp_metadata_ae->ae_stats_info.ucvalidblocks = 16;
			alisp_metadata_ae->ae_stats_info.ucvalidbanks = 16;
		}

		/* no valid stats data  */
		pispmetaae->b_isstats_byaddr = 0;

	}


	gettimeofday(&systemtime, NULL);
	time_5= systemtime.tv_sec * 1000000 + systemtime.tv_usec;


	/* parsing AWB if AWB pointer is valid */
	if ( alisp_metadata_awb == NULL )
		;	/* do nothing, even if HW3A stats passing valid data */
	else if ( pispmeta.uawbstatssize != 0 && pispmeta.uawbstatsaddr != 0 ) {
		pispmetaawb = (struct isp_drv_meta_awb_t *)alisp_metadata_awb;
		/* udpate common info */
		pispmetaawb->umagicnum = pispmeta.umagicnum;
		pispmetaawb->uhwengineid = pispmeta.uhwengineid;
		pispmetaawb->uframeidx = pispmeta.uframeidx;

		/* update awb info */
		pispmetaawb->uawbtokenid = pispmeta.uawbtokenid;
		pispmetaawb->uawbstatssize = pispmeta.uawbstatssize;
		pispmetaawb->upseudoflag  = 0;

		memcpy( &pispmetaawb->systemtime, &systemtime, sizeof(struct timeval));
		pispmetaawb->udsys_sof_idx = udsof_idx;

		/* retriving awb info of stats */
		paddrlocal = (uint8 *)alisp_metadata + pispmeta.uawbstatsaddr;
		offset = 0;
		//memcpy( &alisp_metadata_awb->awb_stats_info.udpixelsperblocks  , paddrlocal, 4 );
		alisp_metadata_awb->awb_stats_info.udpixelsperblocks = (paddrlocal[offset] + (paddrlocal[offset+1]<<8) + (paddrlocal[offset+2]<<16) + (paddrlocal[offset+3]<<24));
		//paddrlocal+= 4;
		offset+= 4;
		//memcpy( &alisp_metadata_awb->awb_stats_info.udbanksize         , paddrlocal, 4 );
		alisp_metadata_awb->awb_stats_info.udbanksize = (paddrlocal[offset] + (paddrlocal[offset+1]<<8) + (paddrlocal[offset+2]<<16) + (paddrlocal[offset+3]<<24));
		//paddrlocal+= 4;
		offset+= 4;
		//memcpy( &alisp_metadata_awb->awb_stats_info.uwsub_x            , paddrlocal, 2 );
		alisp_metadata_awb->awb_stats_info.uwsub_x = ((paddrlocal[offset]) + (paddrlocal[offset+1]<<8));
		//paddrlocal+= 2;
		offset+= 2;
		//memcpy( &alisp_metadata_awb->awb_stats_info.uwsub_y            , paddrlocal, 2 );
		alisp_metadata_awb->awb_stats_info.uwsub_y = ((paddrlocal[offset]) + (paddrlocal[offset+1]<<8));
		//paddrlocal+= 2;
		offset+= 2;
		//memcpy( &alisp_metadata_awb->awb_stats_info.uwwin_x            , paddrlocal, 2 );
		alisp_metadata_awb->awb_stats_info.uwwin_x = ((paddrlocal[offset]) + (paddrlocal[offset+1]<<8));
		//paddrlocal+= 2;
		offset+= 2;
		//memcpy( &alisp_metadata_awb->awb_stats_info.uwwin_y            , paddrlocal, 2 );
		alisp_metadata_awb->awb_stats_info.uwwin_y = ((paddrlocal[offset]) + (paddrlocal[offset+1]<<8));
		//paddrlocal+= 2;
		offset+= 2;
		//memcpy( &alisp_metadata_awb->awb_stats_info.uwwin_w            , paddrlocal, 2 );
		alisp_metadata_awb->awb_stats_info.uwwin_w = ((paddrlocal[offset]) + (paddrlocal[offset+1]<<8));
		//paddrlocal+= 2;
		offset+= 2;
		//memcpy( &alisp_metadata_awb->awb_stats_info.uwwin_h            , paddrlocal, 2 );
		alisp_metadata_awb->awb_stats_info.uwwin_h = ((paddrlocal[offset]) + (paddrlocal[offset+1]<<8));
		//paddrlocal+= 2;
		offset+= 2;
		//memcpy( &alisp_metadata_awb->awb_stats_info.uwtotalcount       , paddrlocal, 2 );
		alisp_metadata_awb->awb_stats_info.uwtotalcount = ((paddrlocal[offset]) + (paddrlocal[offset+1]<<8));
		//paddrlocal+= 2;
		offset+= 2;
		//memcpy( &alisp_metadata_awb->awb_stats_info.uwgrasscount       , paddrlocal, 2 );
		alisp_metadata_awb->awb_stats_info.uwgrasscount = ((paddrlocal[offset]) + (paddrlocal[offset+1]<<8));
		//paddrlocal+= 2;
		offset+= 2;
		//memcpy( &alisp_metadata_awb->awb_stats_info.ucvalidblocks      , paddrlocal, 1 );
		alisp_metadata_awb->awb_stats_info.ucvalidblocks = paddrlocal[offset];
		//paddrlocal+= 1;
		offset+= 1;
		//memcpy( &alisp_metadata_awb->awb_stats_info.ucvalidbanks       , paddrlocal, 1 );
		alisp_metadata_awb->awb_stats_info.ucvalidbanks = paddrlocal[offset];
		//paddrlocal+= 1;
		offset+= 1;
		alisp_metadata_awb->awb_stats_info.ucRGBFmt = paddrlocal[offset];
		offset += 1;

		/* copy AWB stats to indicated buffer address, make 8 alignment */
		offset_start = pispmeta.uawbstatsaddr + offset;		/* offset is accumulation value of awb info of stats */
		padding = (offset_start % 8 == 0 ) ? 0:(8- (offset_start % 8));
		offset_start = offset_start + padding;

		/* allocate memory buffer base on meta size of AWB stats */
		if ( alisp_metadata_awb->uawbstatssize > HW3A_AWB_STATS_BUFFER_SIZE  ) {
			return ERR_WRP_ALLOCATE_BUFFER;
		}

		/* shift to data addr */
		ptempaddr = (uint8 *)alisp_metadata + offset_start;

		alisp_metadata_awb->b_isstats_byaddr = 0;
		/* check pre-allocated buffer validity */
#ifdef _ENABLE_STATS_PRE_ALLOC_BUF
#if ( _ENABLE_STATS_PRE_ALLOC_BUF )
		alisp_metadata_awb->b_isstats_byaddr = 1;
#endif
#endif

		if ( alisp_metadata_awb->b_isstats_byaddr == 0 ) {
			/* copy data to buffer addr */
			memcpy( (void *)alisp_metadata_awb->pawb_stats , (void *)ptempaddr, alisp_metadata_awb->uawbstatssize );
		} else {
			/* assign by addr if buffer is allocated from 3A ctrl layer  */
			alisp_metadata_awb->puc_awb_stats = (uint8 *)ptempaddr;
		}

		uwValidstats_Num++;
	} else { /* set pseudo flag, for AWB lib progressive runing */
		pispmetaawb = (struct isp_drv_meta_awb_t *)alisp_metadata_awb;
		/* udpate common info */
		pispmetaawb->umagicnum = pispmeta.umagicnum;
		pispmetaawb->uhwengineid = pispmeta.uhwengineid;
		pispmetaawb->uframeidx = pispmeta.uframeidx;

		/* update awb info */
		pispmetaawb->uawbtokenid = pispmeta.uawbtokenid;
		pispmetaawb->uawbstatssize = pispmeta.uawbstatssize;
		pispmetaawb->upseudoflag  = 1;
		pispmetaawb->b_isstats_byaddr = 0;
		memcpy( &pispmetaawb->systemtime, &systemtime, sizeof(struct timeval));
		pispmetaawb->udsys_sof_idx = udsof_idx;

		if ( pispmeta.uawbstatsaddr != 0) {
			/* retriving awb info of stats */
			paddrlocal = (uint8 *)alisp_metadata + pispmeta.uawbstatsaddr;
			offset = 0;
			//memcpy( &alisp_metadata_awb->awb_stats_info.udpixelsperblocks  , paddrlocal, 4 );
			alisp_metadata_awb->awb_stats_info.udpixelsperblocks = (paddrlocal[offset] + (paddrlocal[offset+1]<<8) + (paddrlocal[offset+2]<<16) + (paddrlocal[offset+3]<<24));
			//paddrlocal+= 4;
			offset+= 4;
			//memcpy( &alisp_metadata_awb->awb_stats_info.udbanksize         , paddrlocal, 4 );
			alisp_metadata_awb->awb_stats_info.udbanksize = (paddrlocal[offset] + (paddrlocal[offset+1]<<8) + (paddrlocal[offset+2]<<16) + (paddrlocal[offset+3]<<24));
			//paddrlocal+= 4;
			offset+= 4;
			//memcpy( &alisp_metadata_awb->awb_stats_info.uwsub_x            , paddrlocal, 2 );
			alisp_metadata_awb->awb_stats_info.uwsub_x = ((paddrlocal[offset]) + (paddrlocal[offset+1]<<8));
			//paddrlocal+= 2;
			offset+= 2;
			//memcpy( &alisp_metadata_awb->awb_stats_info.uwsub_y            , paddrlocal, 2 );
			alisp_metadata_awb->awb_stats_info.uwsub_y = ((paddrlocal[offset]) + (paddrlocal[offset+1]<<8));
			//paddrlocal+= 2;
			offset+= 2;
			//memcpy( &alisp_metadata_awb->awb_stats_info.uwwin_x            , paddrlocal, 2 );
			alisp_metadata_awb->awb_stats_info.uwwin_x = ((paddrlocal[offset]) + (paddrlocal[offset+1]<<8));
			//paddrlocal+= 2;
			offset+= 2;
			//memcpy( &alisp_metadata_awb->awb_stats_info.uwwin_y            , paddrlocal, 2 );
			alisp_metadata_awb->awb_stats_info.uwwin_y = ((paddrlocal[offset]) + (paddrlocal[offset+1]<<8));
			//paddrlocal+= 2;
			offset+= 2;
			//memcpy( &alisp_metadata_awb->awb_stats_info.uwwin_w            , paddrlocal, 2 );
			alisp_metadata_awb->awb_stats_info.uwwin_w = ((paddrlocal[offset]) + (paddrlocal[offset+1]<<8));
			//paddrlocal+= 2;
			offset+= 2;
			//memcpy( &alisp_metadata_awb->awb_stats_info.uwwin_h            , paddrlocal, 2 );
			alisp_metadata_awb->awb_stats_info.uwwin_h = ((paddrlocal[offset]) + (paddrlocal[offset+1]<<8));
			//paddrlocal+= 2;
			offset+= 2;
			//memcpy( &alisp_metadata_awb->awb_stats_info.uwtotalcount       , paddrlocal, 2 );
			alisp_metadata_awb->awb_stats_info.uwtotalcount = ((paddrlocal[offset]) + (paddrlocal[offset+1]<<8));
			//paddrlocal+= 2;
			offset+= 2;
			//memcpy( &alisp_metadata_awb->awb_stats_info.uwgrasscount       , paddrlocal, 2 );
			alisp_metadata_awb->awb_stats_info.uwgrasscount= ((paddrlocal[offset]) + (paddrlocal[offset+1]<<8));
			//paddrlocal+= 2;
			offset+= 2;
			//memcpy( &alisp_metadata_awb->awb_stats_info.ucvalidblocks      , paddrlocal, 1 );
			alisp_metadata_awb->awb_stats_info.ucvalidblocks = paddrlocal[offset];
			//paddrlocal+= 1;
			offset+= 1;
			//memcpy( &alisp_metadata_awb->awb_stats_info.ucvalidbanks       , paddrlocal, 1 );
			alisp_metadata_awb->awb_stats_info.ucvalidbanks = paddrlocal[offset];
		}

	}

	gettimeofday(&systemtime, NULL);
	time_6= systemtime.tv_sec * 1000000 + systemtime.tv_usec;


	/* parsing AF if AF pointer is valid */
	if ( alisp_metadata_af == NULL )
		;	/* do nothing, even if HW3A stats passing valid data */
	else if ( pispmeta.uafstatssize != 0 && pispmeta.uafstatsaddr != 0 ) {
		pispmetaaf = (struct isp_drv_meta_af_t *)alisp_metadata_af;
		/* udpate common info */
		pispmetaaf->umagicnum = pispmeta.umagicnum;
		pispmetaaf->uhwengineid = pispmeta.uhwengineid;
		pispmetaaf->uframeidx = pispmeta.uframeidx;

		/* update af info */
		pispmetaaf->uaftokenid = pispmeta.uaftokenid;
		pispmetaaf->uafstatssize = pispmeta.uafstatssize;
		pispmetaaf->upseudoflag = 0;

		memcpy( &pispmetaaf->systemtime, &systemtime, sizeof(struct timeval));
		pispmetaaf->udsys_sof_idx = udsof_idx;

		/* retriving af info of stats */
		paddrlocal = (uint8 *)alisp_metadata + pispmeta.uafstatsaddr;
		offset = 0;
		//memcpy( &alisp_metadata_af->af_stats_info.udpixelsperblocks  , paddrlocal, 4 );
		alisp_metadata_af->af_stats_info.udpixelsperblocks = (paddrlocal[offset] + (paddrlocal[offset+1]<<8) + (paddrlocal[offset+2]<<16) + (paddrlocal[offset+3]<<24));
		//paddrlocal+= 4;
		offset+= 4;
		//memcpy( &alisp_metadata_af->af_stats_info.udbanksize         , paddrlocal, 4 );
		alisp_metadata_af->af_stats_info.udbanksize = (paddrlocal[offset] + (paddrlocal[offset+1]<<8) + (paddrlocal[offset+2]<<16) + (paddrlocal[offset+3]<<24));
		//paddrlocal+= 4;
		offset+= 4;
		//memcpy( &alisp_metadata_af->af_stats_info.ucvalidblocks      , paddrlocal, 1 );
		alisp_metadata_af->af_stats_info.ucvalidblocks = paddrlocal[offset];
		//paddrlocal+= 1;
		offset+= 1;
		//memcpy( &alisp_metadata_af->af_stats_info.ucvalidbanks       , paddrlocal, 1 );
		alisp_metadata_af->af_stats_info.ucvalidbanks = paddrlocal[offset];
		//paddrlocal+= 1;
		offset+= 1;

		/* copy af stats to indicated buffer address, make 8 alignment */
		offset_start = pispmeta.uafstatsaddr + offset;  /* offset is accumulation value of af info of stats */
		padding = (offset_start % 8 == 0 ) ? 0:(8- (offset_start % 8));
		offset_start = offset_start + padding;

		/* allocate memory buffer base on meta size of AF stats */
		if ( alisp_metadata_af->uafstatssize > HW3A_AF_STATS_BUFFER_SIZE ) {
			return ERR_WRP_ALLOCATE_BUFFER;
		}

		/* shift to data addr */
		ptempaddr = (uint8 *)alisp_metadata + offset_start;

		alisp_metadata_af->b_isstats_byaddr = 0;

#ifdef _ENABLE_STATS_PRE_ALLOC_BUF
#if ( _ENABLE_STATS_PRE_ALLOC_BUF )
		alisp_metadata_af->b_isstats_byaddr = 1;
#endif
#endif

		if ( alisp_metadata_af->b_isstats_byaddr == 0 ) {
			/* copy data to buffer addr */
			memcpy( (void *)alisp_metadata_af->paf_stats , (void *)ptempaddr, alisp_metadata_af->uafstatssize );
		} else {
			/* assign by addr if buffer is allocated from 3A ctrl layer  */
			alisp_metadata_af->puc_af_stats = (uint8 *)ptempaddr;
		}

		uwValidstats_Num++;
	} else { /* set pseudo flag */
		pispmetaaf = (struct isp_drv_meta_af_t *)alisp_metadata_af;
		/* udpate common info */
		pispmetaaf->umagicnum = pispmeta.umagicnum;
		pispmetaaf->uhwengineid = pispmeta.uhwengineid;
		pispmetaaf->uframeidx = pispmeta.uframeidx;

		/* update af info */
		pispmetaaf->uaftokenid = pispmeta.uaftokenid;
		pispmetaaf->uafstatssize = pispmeta.uafstatssize;
		pispmetaaf->upseudoflag = 1;

		memcpy( &pispmetaaf->systemtime, &systemtime, sizeof(struct timeval));
		pispmetaaf->udsys_sof_idx = udsof_idx;

		if ( pispmeta.uafstatsaddr != 0) {
			/* retriving af info of stats */
			paddrlocal = (uint8 *)alisp_metadata + pispmeta.uafstatsaddr;
			offset = 0;
			//memcpy( &alisp_metadata_af->af_stats_info.udpixelsperblocks  , paddrlocal, 4 );
			alisp_metadata_af->af_stats_info.udpixelsperblocks = (paddrlocal[offset] + (paddrlocal[offset+1]<<8) + (paddrlocal[offset+2]<<16) + (paddrlocal[offset+3]<<24));
			//paddrlocal+= 4;
			offset+= 4;
			//memcpy( &alisp_metadata_af->af_stats_info.udbanksize         , paddrlocal, 4 );
			alisp_metadata_af->af_stats_info.udbanksize = (paddrlocal[offset] + (paddrlocal[offset+1]<<8) + (paddrlocal[offset+2]<<16) + (paddrlocal[offset+3]<<24));
			//paddrlocal+= 4;
			offset+= 4;
			//memcpy( &alisp_metadata_af->af_stats_info.ucvalidblocks      , paddrlocal, 1 );
			alisp_metadata_af->af_stats_info.ucvalidblocks = paddrlocal[offset];
			//paddrlocal+= 1;
			offset+= 1;
			//memcpy( &alisp_metadata_af->af_stats_info.ucvalidbanks       , paddrlocal, 1 );
			alisp_metadata_af->af_stats_info.ucvalidbanks = paddrlocal[offset];
		}

		pispmetaaf->b_isstats_byaddr = 0;
	}


	gettimeofday(&systemtime, NULL);
	time_7= systemtime.tv_sec * 1000000 + systemtime.tv_usec;


	/* parsing YHist if YHist pointer is valid */
	if ( alisp_metadata_yhist == NULL )
		;	/* do nothing, even if HW3A stats passing valid data */
	else if ( pispmeta.uyhiststatssize != 0 && pispmeta.uyhiststatsaddr != 0 ) {
		pispmetayhist = (struct isp_drv_meta_yhist_t *)alisp_metadata_yhist;
		/* udpate common info */
		pispmetayhist->umagicnum = pispmeta.umagicnum;
		pispmetayhist->uhwengineid = pispmeta.uhwengineid;
		pispmetayhist->uframeidx = pispmeta.uframeidx;

		/* update yhist info */
		pispmetayhist->uyhisttokenid = pispmeta.uyhisttokenid;
		pispmetayhist->uyhiststatssize = pispmeta.uyhiststatssize;

		memcpy( &pispmetayhist->systemtime, &systemtime, sizeof(struct timeval));
		pispmetayhist->udsys_sof_idx = udsof_idx;

		/* retriving yhist info of stats */
		paddrlocal = (uint8 *)alisp_metadata + pispmeta.uyhiststatsaddr;
		offset = 0;

		/* copy yhist stats to indicated buffer address, make 8 alignment */
		offset_start = pispmeta.uyhiststatsaddr + offset;	/* offset is accumulation value of yhist info of stats */
		padding = (offset_start % 8 == 0 ) ? 0:(8- (offset_start % 8));
		offset_start = offset_start + padding;

#ifndef LOCAL_NDK_BUILD
		//ISP_LOGI("al3awrapper_dispatchhw3astats y-hist statssize:%d", alisp_metadata_yhist->uyhiststatssize );
#endif

		/* allocate memory buffer base on meta size of YHist stats */
		if ( alisp_metadata_yhist->uyhiststatssize > HW3A_YHIST_STATS_BUFFER_SIZE ) {
			return ERR_WRP_ALLOCATE_BUFFER;
		}

		/* shift to data addr */
		ptempaddr = (uint8 *)alisp_metadata + offset_start;


#ifdef _ENABLE_STATS_PRE_ALLOC_BUF
#if ( _ENABLE_STATS_PRE_ALLOC_BUF )
		alisp_metadata_yhist->b_isstats_byaddr = 1;
#endif
#endif

		if ( alisp_metadata_yhist->b_isstats_byaddr == 0 ) {
			/* copy data to buffer addr */
			memcpy( (void *)alisp_metadata_yhist->pyhist_stats , (void *)ptempaddr, alisp_metadata_yhist->uyhiststatssize );
		} else {
			/* assign by addr if buffer is allocated from 3A ctrl layer  */
			alisp_metadata_yhist->puc_yhist_stats = (uint8 *)ptempaddr;
		}
		uwValidstats_Num++;

	}

	gettimeofday(&systemtime, NULL);
	time_8= systemtime.tv_sec * 1000000 + systemtime.tv_usec;


	/* parsing AntiF if AntiF pointer is valid */
	if ( alisp_metadata_antif == NULL )
		;	/* do nothing, even if HW3A stats passing valid data */
	else if ( pispmeta.uantifstatssize != 0 && pispmeta.uantifstatsaddr != 0 ) {
		pispmetaantif = (struct isp_drv_meta_antif_t *)alisp_metadata_antif;
		/* udpate common info */
		pispmetaantif->umagicnum = pispmeta.umagicnum;
		pispmetaantif->uhwengineid = pispmeta.uhwengineid;
		pispmetaantif->uframeidx = pispmeta.uframeidx;

		/* update antif info */
		pispmetaantif->uantiftokenid = pispmeta.uantiftokenid;
		pispmetaantif->uantifstatssize = pispmeta.uantifstatssize;

		memcpy( &pispmetaantif->systemtime, &systemtime, sizeof(struct timeval));
		pispmetaantif->udsys_sof_idx = udsof_idx;

		/* retriving antif info of stats */
		paddrlocal = (uint8 *)alisp_metadata + pispmeta.uantifstatsaddr;
		offset = 0;

		/* copy antif stats to indicated buffer address, make 8 alignment */
		offset_start = pispmeta.uantifstatsaddr + offset;	/* offset is accumulation value of antif info of stats */
		padding = (offset_start % 8 == 0 ) ? 0:(8- (offset_start % 8));
		offset_start = offset_start + padding;

		/* allocate memory buffer base on meta size of AntiF stats */
		if ( alisp_metadata_antif->uantifstatssize > HW3A_ANTIF_STATS_BUFFER_SIZE ) {
			return ERR_WRP_ALLOCATE_BUFFER;
		}

		/* shift to data addr */
		ptempaddr = (uint8 *)alisp_metadata + offset_start;


#ifdef _ENABLE_STATS_PRE_ALLOC_BUF
#if ( _ENABLE_STATS_PRE_ALLOC_BUF )
		alisp_metadata_antif->b_isstats_byaddr = 1;
#endif
#endif

		if ( alisp_metadata_antif->b_isstats_byaddr == 0 ) {
			/* copy data to buffer addr */
			memcpy( (void *)alisp_metadata_antif->pantif_stats , (void *)ptempaddr, alisp_metadata_antif->uantifstatssize );
		} else {
			/* assign by addr if buffer is allocated from 3A ctrl layer  */
			alisp_metadata_antif->puc_antif_stats = (uint8 *)ptempaddr;
		}
		uwValidstats_Num++;
	}

	gettimeofday(&systemtime, NULL);
	time_9= systemtime.tv_sec * 1000000 + systemtime.tv_usec;

	/* parsing Subsample if Subsample pointer is valid */
	if ( alisp_metadata_subsample == NULL )
		;	/* do nothing, even if HW3A stats passing valid data */
	else if ( pispmeta.usubsamplestatssize != 0 && pispmeta.usubsamplestatsaddr != 0 ) {
		pispmetasubsample = (struct isp_drv_meta_subsample_t *)alisp_metadata_subsample;
		/* udpate common info */
		pispmetasubsample->umagicnum = pispmeta.umagicnum;
		pispmetasubsample->uhwengineid = pispmeta.uhwengineid;
		pispmetasubsample->uframeidx = pispmeta.uframeidx;

		/* update subsample info */
		pispmetasubsample->usubsampletokenid = pispmeta.usubsampletokenid;
		pispmetasubsample->usubsamplestatssize = pispmeta.usubsamplestatssize;

		memcpy( &pispmetasubsample->systemtime, &systemtime, sizeof(struct timeval));
		pispmetasubsample->udsys_sof_idx = udsof_idx;

		/* retriving subsample info of stats */
		paddrlocal = (uint8 *)alisp_metadata + pispmeta.usubsamplestatsaddr;
		offset = 0;
		//memcpy( &alisp_metadata_subsample->subsample_stats_info.udbuffertotalsize  , paddrlocal, 4 );
		alisp_metadata_subsample->subsample_stats_info.udbuffertotalsize = (paddrlocal[offset] + (paddrlocal[offset+1]<<8) + (paddrlocal[offset+2]<<16) + (paddrlocal[offset+3]<<24));
		//paddrlocal+= 4;
		offset+= 4;
		//memcpy( &alisp_metadata_subsample->subsample_stats_info.ucvalidw           , paddrlocal, 2 );
		alisp_metadata_subsample->subsample_stats_info.ucvalidw = ((paddrlocal[offset]) + (paddrlocal[offset+1]<<8));
		//paddrlocal+= 2;
		offset+= 2;
		//memcpy( &alisp_metadata_subsample->subsample_stats_info.ucvalidh           , paddrlocal, 2 );
		alisp_metadata_subsample->subsample_stats_info.ucvalidh = ((paddrlocal[offset]) + (paddrlocal[offset+1]<<8));
		//paddrlocal+= 2;
		offset+= 2;

		/* copy subsample stats to indicated buffer address, make 8 alignment */
		offset_start = pispmeta.usubsamplestatsaddr + offset;	/* offset is accumulation value of subsample info of stats */
		padding = (offset_start % 8 == 0 ) ? 0:(8- (offset_start % 8));
		offset_start = offset_start + padding;

		/* allocate memory buffer base on meta size of Subsample stats */
		if ( alisp_metadata_subsample->usubsamplestatssize > HW3A_SUBIMG_STATS_BUFFER_SIZE ) {
			return ERR_WRP_ALLOCATE_BUFFER;
		}

		/* shift to data addr */
		ptempaddr = (uint8 *)alisp_metadata + offset_start;

#ifdef _ENABLE_STATS_PRE_ALLOC_BUF
#if ( _ENABLE_STATS_PRE_ALLOC_BUF )
		alisp_metadata_subsample->b_isstats_byaddr = 1;
#endif
#endif

		if ( alisp_metadata_subsample->b_isstats_byaddr == 0 ) {
			/* copy data to buffer addr */
			memcpy( (void *)alisp_metadata_subsample->psubsample_stats , (void *)ptempaddr, alisp_metadata_subsample->usubsamplestatssize );
		} else {
			/* assign by addr if buffer is allocated from 3A ctrl layer  */
			alisp_metadata_subsample->puc_subsample_stats = (uint8 *)ptempaddr;
		}
		uwValidstats_Num++;
	}

	if ( uwValidstats_Num == 0 )
		return ERR_WRP_EMPTY_METADATA;

	gettimeofday(&systemtime_e, NULL);
	time_e = systemtime_e.tv_sec * 1000000 + systemtime_e.tv_usec;

#ifndef LOCAL_NDK_BUILD
	ISP_LOGI("al3awrapper_dispatchhw3astats process total_time:%d,commom_t:%d,memset3A:%d,memset_flk:%d,memset_sub:%d,ae_t:%d,wb:%d,af:%d,y_t_m:%d,flk_t_m:%d,sub_t_m:%d\r\n", (uint32)(time_e - time_s),(uint32)(time_1 - time_s),(uint32)(time_2 - time_1),
	         (uint32)(time_3 - time_2),(uint32)(time_4 - time_3),(uint32)(time_5 - time_4),(uint32)(time_6 - time_5),(uint32)(time_7 - time_6),(uint32)(time_8-time_7),(uint32)(time_9-time_8),(uint32)(time_e-time_9));
#endif


	return ret;
}

/* DL sequence query API */
/*
 * API name: al3AWrapper_GetCurrentDLSequence
 * Comments:
 * This API is used for AP, to set correct DL sequence setting (both for basic/advanced)
 * Parameter:
 * ucAHBSensoreID[In]: used for operating AHB HW channel
 * aDldSequence[Out]: prepared for setup to AHB HW, help to schedule correct 3A HW stats output
 * aIsSingle3AMode[In]: 0: DL all 3A (AE or AWB or AF ) stats at same frame, used for rear camera
 *                      1: DL single 3A (AE/AWB/AF) stats per frame, follow aDldSequence setting, used for front camera
 * opMode[In]: 0: normal LV, (default value)
 *             1: AF, flash AF
 *             2: flash AE
 * return: error code
 */
uint32 al3awrapper_getcurrentdlsequence( uint8 ucahbsensoreid, struct alisp_dldsequence_t* adldsequence, uint8 aissingle3amode, enum alisp_opmode_idx_t opmode )
{
	uint32 ret = ERR_WRP_SUCCESS;
	struct alisp_dldsequence_t* adlselist;

	if ( adldsequence == NULL )
		return ERR_WRP_INVALID_DL_PARAM;

	adlselist = (struct alisp_dldsequence_t* )adldsequence;

	adlselist->ucahbsensoreid = ucahbsensoreid;	/* fulfill AHB sensor index which would be used when calling al3AWrapper_SetDLSequence */
	UNUSED(aissingle3amode);
	switch ( opmode ) {
	case OPMODE_NORMALLV:
		/* W9 config */
		adlselist->ucpreview_baisc_dldseqlength = 4;
		adlselist->aucpreview_baisc_dldseq[0] = HA3ACTRL_B_DL_TYPE_AE;
		adlselist->aucpreview_baisc_dldseq[1] = HA3ACTRL_B_DL_TYPE_AWB;
		adlselist->aucpreview_baisc_dldseq[2] = HA3ACTRL_B_DL_TYPE_AF;
		adlselist->aucpreview_baisc_dldseq[3] = HA3ACTRL_B_DL_TYPE_AWB;
		/* W10 config */
		adlselist->ucpreview_adv_dldseqlength = 1;
		adlselist->aucpreview_adv_dldseq[0] = HA3ACTRL_B_DL_TYPE_AntiF;
		/* W9 */
		adlselist->ucfastconverge_baisc_dldseqlength = 0;
		adlselist->aucfastconverge_baisc_dldseq[0] = HA3ACTRL_B_DL_TYPE_NONE;
		break;
	case OPMODE_AF_FLASH_AF:
		/* W9 config */
		adlselist->ucpreview_baisc_dldseqlength = 2;
		adlselist->aucpreview_baisc_dldseq[0] = HA3ACTRL_B_DL_TYPE_AF;
		adlselist->aucpreview_baisc_dldseq[1] = HA3ACTRL_B_DL_TYPE_AF;
		/* W10 config */
		adlselist->ucpreview_adv_dldseqlength = 1;
		adlselist->aucpreview_adv_dldseq[0] = HA3ACTRL_B_DL_TYPE_Sub;

		adlselist->ucfastconverge_baisc_dldseqlength = 0;
		adlselist->aucfastconverge_baisc_dldseq[0] = HA3ACTRL_B_DL_TYPE_NONE;
		break;
	case OPMODE_FLASH_AE:
		/* W9 config */
		adlselist->ucpreview_baisc_dldseqlength = 2;
		adlselist->aucpreview_baisc_dldseq[0] = HA3ACTRL_B_DL_TYPE_AE;
		adlselist->aucpreview_baisc_dldseq[1] = HA3ACTRL_B_DL_TYPE_AWB;
		/* W10 config */
		adlselist->ucpreview_adv_dldseqlength = 1;
		adlselist->aucpreview_adv_dldseq[0] = HA3ACTRL_B_DL_TYPE_AntiF;

		adlselist->ucfastconverge_baisc_dldseqlength = 0;
		adlselist->aucfastconverge_baisc_dldseq[0] = HA3ACTRL_B_DL_TYPE_NONE;
		break;

	default:
		return ERR_WRP_INVALID_DL_OPMODE;
		break;
	}
	return ret;
}

/* should include ISPDRV_SetBasicPreivewDldSeq/ISPDRV_SetAdvPreivewDldSeq/ISPDRV_SetBasicFastConvergeDldSeq from ISP_Driver layer */
/*
 * API name: al3AWrapper_SetDLSequence
 * Commets:
 * This API would update to Altek ISP via ISP driver layer
 * AP should call al3AWrapper_GetCurrentDLSequence before set to HW
 * param aDldSequence[In]
 * return: error code
 */
uint32 al3awrapper_setdlsequence( struct alisp_dldsequence_t adldsequence )
{
	uint32 ret = ERR_WRP_SUCCESS;
	uint8 ucahbsensoreid, ucIsSingle3AMode;

	ucahbsensoreid = adldsequence.ucahbsensoreid;
#ifdef LINK_ALTEK_ISP_DRV_DEFINE   /* only build when link to ISP driver define */
	/* W9 config */
	ret = ISPDRV_SetBasicPreivewDldSeq( ucahbsensoreid, (uint8 *)(&adldsequence.aucpreview_baisc_dldseq[0]), adldsequence.ucpreview_baisc_dldseqlength );
	if ( ret!= ERR_WRP_SUCCESS )
		return ret;
	/* W10 Config */
	ret = ISPDRV_SetAdvPreivewDldSeq( ucahbsensoreid, (uint8 *)(&adldsequence.aucpreview_adv_dldseq[0]), adldsequence.ucpreview_adv_dldseqlength );
	if ( ret!= ERR_WRP_SUCCESS )
		return ret;
	/* Fast W9 config */
	ret = ISPDRV_SetBasicFastConvergeDldSeq( ucahbsensoreid, (uint8 *)(&adldsequence.aucpreview_baisc_dldseq[0]), adldsequence.ucpreview_baisc_dldseqlength );
	if ( ret!= ERR_WRP_SUCCESS )
		return ret;
#endif
	return ret;
}

/*
 * API name: al3AWrapper_GetVersion
 * This API would return labeled version of wrapper
 * fWrapVersion[out], return current wapper version
 * return: error code
 */
uint32 al3awrapper_getversion( float *fwrapversion )
{
	uint32 ret = ERR_WRP_SUCCESS;

	*fwrapversion = _WRAPPER_VER;

	return ret;
}

/*
 * API name: al3awrapper_get_define_stats_size
 * This API would return hw3a defined size
 * Exception: subimage is decided by AP buffer when set buffer allocation for sub-image engine
 * a_hw3a_define_size_info[out], return stats size of each HW engine (AE/AWB/etc.)
 * return: error code
 */
uint32 al3awrapper_get_defined_stats_size( struct wrapper_hw3a_define_sizeinfo_t * a_hw3a_define_size_info )
{
	uint32 ret = ERR_WRP_SUCCESS;

	if ( a_hw3a_define_size_info == NULL )
		return ERR_WRP_INVALID_INPUT_PARAM;

	a_hw3a_define_size_info->ud_alhw3a_ae_stats_size = sizeof ( struct isp_drv_meta_ae_t  );
	a_hw3a_define_size_info->ud_alhw3a_awb_stats_size = sizeof ( struct isp_drv_meta_awb_t  );
	a_hw3a_define_size_info->ud_alhw3a_af_stats_size = sizeof ( struct isp_drv_meta_af_t  );
	a_hw3a_define_size_info->ud_alhw3a_yhist_stats_size = sizeof ( struct isp_drv_meta_yhist_t  );
	a_hw3a_define_size_info->ud_alhw3a_antif_stats_size = sizeof ( struct isp_drv_meta_antif_t  );
	a_hw3a_define_size_info->ud_alhw3a_subimg_stats_size = sizeof ( struct isp_drv_meta_subsample_t  );

	return ret;
}
