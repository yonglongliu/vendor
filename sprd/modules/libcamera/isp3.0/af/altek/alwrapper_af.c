/********************************************************************************
 * al3awrapper_af.c
 *
 *  Created on: 2015/12/06
 *      Author: ZenoKuo
 *  Latest update: 2016/5/13
 *      Reviser: Allenwang
 *  Comments:
 *       This c file is mainly used for AP framework to:
 *       1. Query HW3A config setting
 *       2. Set HW3A via AP IPS driver of altek
 *       3. translate AP framework index from al3A to framework define, such as scene mode, etc.
 *       4. packed output report from AE lib to framework
 *       5. translate input parameter from AP framework to AP
********************************************************************************/

/* test build in local */

#include "mtype.h"
#include "frmwk_hw3a_event_type.h"
#include "hw3a_stats.h"
/* AF lib define */
#include "allib_af.h"
#include "allib_af_errcode.h"

/* Wrapper define */
#include "alwrapper_3a.h"
#include "alwrapper_af.h"
#include "alwrapper_af_errcode.h"

#include <math.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <android/log.h>

/* Debug only */
#if 0
#define WRAP_LOG(...) __android_log_print(ANDROID_LOG_DEBUG, "af_wrapper", __VA_ARGS__)
#else
#define WRAP_LOG(...) do { } while(0)
#endif


/* VCM inf step*/
#define VCM_INF_STEP_ADDR_OFFSET (1714)
/*VCM macro step*/
#define VCM_MACRO_STEP_ADDR_OFFSET (1715)
/*VCM calibration inf distance in mm.*/
#define VCM_INF_STEP_CALIB_DISTANCE (20000)
/*VCM calibration macro distance in mm.*/
#define VCM_MACRO_STEP_CALIB_DISTANCE (700)
/* f-number, ex. f2.0, then input 2.0*/
#define MODULE_F_NUMBER (2.0)
/* focal length in mm*/
#define MODULE_FOCAL_LENGTH (4400)
/* mechanical limitation top position (macro side) in step.*/
#define VCM_INF_STEP_MECH_TOP (1023)
/*mechanical limitation bottom position(inf side) in step.*/
#define VCM_INF_STEP_MECH_BOTTOM (0)
/* time consume of lens from moving to stable in ms. */
#define VCM_MOVING_STATBLE_TIME (20)

#define AF_DEFAULT_AFBLK_TH {\
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,\
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,\
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,\
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
#define AF_DEFAULT_THLUT {0, 0, 0, 0}
#define AF_DEFAULT_MASK {0, 0, 0, 1, 1, 2}
#define AF_DEFAULT_LUT {   0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,\
			    11, 12, 13, 14, 15, 16, 17, 18, 19, 20,\
			    21, 22, 23, 24, 25, 26, 27, 28, 29, 30,\
			    31, 32, 33, 34, 35, 36, 37, 38, 39, 40,\
			    41, 42, 43, 44, 45, 46, 47, 48, 49, 50,\
			    51, 52, 53, 54, 55, 56, 57, 58, 59, 60,\
			    61, 62, 63, 64, 65, 66, 67, 68, 69, 70,\
			    71, 72, 73, 74, 75, 76, 77, 78, 79, 80,\
			    81, 82, 83, 84, 85, 86, 87, 88, 89, 90,\
			    91, 92, 93, 94, 95, 96, 97, 98, 99,100,\
			   101,102,103,104,105,106,107,108,109,110,\
			   111,112,113,114,115,116,117,118,119,120,\
			   121,122,123,124,125,126,127,128,129,130,\
			   131,132,133,134,135,136,137,138,139,140,\
			   141,142,143,144,145,146,147,148,149,150,\
			   151,152,153,154,155,156,157,158,159,160,\
			   161,162,163,164,165,166,167,168,169,170,\
			   171,172,173,174,175,176,177,178,179,180,\
			   181,182,183,184,185,186,187,188,189,190,\
			   191,192,193,194,195,196,197,198,199,200,\
			   201,202,203,204,205,206,207,208,209,210,\
			   211,212,213,214,215,216,217,218,219,220,\
			   221,222,223,224,225,226,227,228,229,230,\
			   231,232,233,234,235,236,237,238,239,240,\
			   241,242,243,244,245,246,247,248,249,250,\
			   251,252,253,254,255,255,255,255}

/*
 * API name: al3AWrapper_dispatch_af_stats
 * This API used for patching HW3A stats from ISP(Altek) for AF libs(Altek), after patching completed, AF ctrl should prepare patched
 * stats to AF libs
 * param isp_meta_data[In]: patched data after calling al3AWrapper_DispatchHW3AStats, used for patch AF stats for AF lib
 * param alaf_stats[Out]: result patched AF stats
 * return: error code
 */
uint32 al3awrapper_dispatchhw3a_afstats(void *isp_meta_data,void *alaf_stats)
{
	uint32 ret = ERR_WPR_AF_SUCCESS;
	struct isp_drv_meta_af_t *p_meta_data_af;
	struct allib_af_hw_stats_t *p_patched_stats;
	uint32 total_blocks;
	uint32 bank_size;
	uint16 i = 0,j = 0,blocks, banks, index;
	uint8  *stats;
	uint32 *stats_addr_32;
	uint64 *stats_addr_64;
	int64   addr_diff;
	uint8 *add_stat_32, *add_stat_64;
	uint32  udoffset;

	WRAP_LOG("al3awrapper_dispatchhw3a_afstats start");

	/* check input parameter validity*/
	if (isp_meta_data == NULL) {
		WRAP_LOG("ERR_WRP_AF_EMPTY_METADATA");
		return ERR_WRP_AF_EMPTY_METADATA;
	}

	if (alaf_stats == NULL) {
		WRAP_LOG("ERR_WRP_AF_INVALID_INPUT_PARAM");
		return ERR_WRP_AF_INVALID_INPUT_PARAM;
	}

	p_meta_data_af = (struct isp_drv_meta_af_t *)isp_meta_data;
	p_patched_stats = (struct allib_af_hw_stats_t *)alaf_stats;
	WRAP_LOG("isp_meta_data: %p, alaf_stats: %p, p_meta_data_af->upseudoflag: %d", isp_meta_data, alaf_stats, p_meta_data_af->upseudoflag);

	if (p_meta_data_af->upseudoflag) {
		WRAP_LOG("ERR_WRP_AF_PSEUDO_DATA");
		return ERR_WRP_AF_PSEUDO_DATA;
	}

	total_blocks = p_meta_data_af->af_stats_info.ucvalidblocks * p_meta_data_af->af_stats_info.ucvalidbanks;
	bank_size = p_meta_data_af->af_stats_info.udbanksize;
	blocks = p_meta_data_af->af_stats_info.ucvalidblocks;
	banks = p_meta_data_af->af_stats_info.ucvalidbanks;
	WRAP_LOG("total_blocks: %d, bank_size: %d, blocks: %d, banks: %d", total_blocks, bank_size, blocks, banks);

	if (total_blocks == 0) {
		WRAP_LOG("ERR_WRP_AF_INVALID_INPUT_PARAM with total blocks: %d", total_blocks);
		return ERR_WRP_AF_INVALID_INPUT_PARAM;
	}

	if (blocks > MAX_STATS_COLUMN_NUM)
		blocks = MAX_STATS_COLUMN_NUM;

	if (banks > MAX_STATS_ROW_NUM)
		banks = MAX_STATS_ROW_NUM;

#ifdef NEW_AF_ALGORITHM
/// ALTEK_MODIFIED >>>
	p_patched_stats->hw_fv.valid_column_num = blocks;
	p_patched_stats->hw_fv.valid_row_num = banks;
/// ALTEK_MODIFIED <<<
#else
	p_patched_stats->valid_column_num = blocks;
	p_patched_stats->valid_row_num = banks;
#endif /* NEW_AF_ALGORITHM */

	index = 0;

	WRAP_LOG("uhwengineid: %d", p_meta_data_af->uhwengineid);

	if (p_meta_data_af->b_isstats_byaddr == 1) {
		if (p_meta_data_af->puc_af_stats == NULL)
			return ERR_WRP_AF_INVALID_STATS_ADDR;
		stats = p_meta_data_af->puc_af_stats;
	} else
		stats = p_meta_data_af->paf_stats;

	/* AP3AMGR_HW3A_A_0 and AP3AMGR_HW3A_B_0, see the AP3AMgr.h*/
	if (AL3A_HW3A_DEV_ID_A_0 == p_meta_data_af->uhwengineid) {
		for (j = 0; j < banks; j++) {
			stats_addr_32 = (uint32 *)(stats) + j * bank_size / 4;
			stats_addr_64 = (uint64 *)stats_addr_32;
			add_stat_32 = stats + j * bank_size;
			add_stat_64 = add_stat_32;

			for (i = 0; i < blocks; i++) {
				index = i + j * blocks;
#if 0
				p_patched_stats->cnt_hor[index] = stats_addr_32[1];
				p_patched_stats->cnt_ver[index] = stats_addr_32[0];
				p_patched_stats->filter_value1[index] = stats_addr_64[3];
				p_patched_stats->filter_value2[index] = stats_addr_64[4];
				p_patched_stats->y_factor[index] = stats_addr_64[1];
				p_patched_stats->fv_hor[index] = stats_addr_32[5];
				p_patched_stats->fv_ver[index] = stats_addr_32[4];
#else
#ifdef NEW_AF_ALGORITHM
				udoffset = 4;
/// ALTEK_MODIFIED >>>
				p_patched_stats->hw_fv.cnt_hor[index] = (add_stat_32[udoffset] + (add_stat_32[udoffset+1]<<8) + (add_stat_32[udoffset+2]<<16) + (add_stat_32[udoffset+3]<<24));
/// ALTEK_MODIFIED <<<

				udoffset = 0;
/// ALTEK_MODIFIED >>>
				p_patched_stats->hw_fv.cnt_ver[index] = (add_stat_32[udoffset] + (add_stat_32[udoffset+1]<<8) + (add_stat_32[udoffset+2]<<16) + (add_stat_32[udoffset+3]<<24));
/// ALTEK_MODIFIED <<<

				udoffset = 3 * 8;
/// ALTEK_MODIFIED >>>
				p_patched_stats->hw_fv.filter_value1[index] = ((uint64)(add_stat_64[udoffset]) + ((uint64)(add_stat_64[udoffset+1])<<8) + ((uint64)(add_stat_64[udoffset+2])<<16) + ((uint64)(add_stat_64[udoffset+3])<<24) +
				        ((uint64)(add_stat_64[udoffset+4])<<32) + ((uint64)(add_stat_64[udoffset+5])<<40) + ((uint64)(add_stat_64[udoffset+6])<<48) + ((uint64)(add_stat_64[udoffset+7])<<56));
/// ALTEK_MODIFIED <<<

				udoffset = 4 * 8;
/// ALTEK_MODIFIED >>>
				p_patched_stats->hw_fv.filter_value2[index] = ((uint64)(add_stat_64[udoffset]) + ((uint64)(add_stat_64[udoffset+1])<<8) + ((uint64)(add_stat_64[udoffset+2])<<16) + ((uint64)(add_stat_64[udoffset+3])<<24) +
				        ((uint64)(add_stat_64[udoffset+4])<<32) + ((uint64)(add_stat_64[udoffset+5])<<40) + ((uint64)(add_stat_64[udoffset+6])<<48) + ((uint64)(add_stat_64[udoffset+7])<<56));
/// ALTEK_MODIFIED <<<

				udoffset = 1 * 8;
/// ALTEK_MODIFIED >>>
				p_patched_stats->hw_fv.y_factor[index] = ((uint64)(add_stat_64[udoffset]) + ((uint64)(add_stat_64[udoffset+1])<<8) + ((uint64)(add_stat_64[udoffset+2])<<16) + ((uint64)(add_stat_64[udoffset+3])<<24) +
				        ((uint64)(add_stat_64[udoffset+4])<<32) + ((uint64)(add_stat_64[udoffset+5])<<40) + ((uint64)(add_stat_64[udoffset+6])<<48) + ((uint64)(add_stat_64[udoffset+7])<<56));
/// ALTEK_MODIFIED <<<

				udoffset = 5 * 4;
/// ALTEK_MODIFIED >>>
				p_patched_stats->hw_fv.fv_hor[index] = (add_stat_32[udoffset] + (add_stat_32[udoffset+1]<<8) + (add_stat_32[udoffset+2]<<16) + (add_stat_32[udoffset+3]<<24));
/// ALTEK_MODIFIED <<<

				udoffset = 4 * 4;
/// ALTEK_MODIFIED >>>
				p_patched_stats->hw_fv.fv_ver[index] = (add_stat_32[udoffset] + (add_stat_32[udoffset+1]<<8) + (add_stat_32[udoffset+2]<<16) + (add_stat_32[udoffset+3]<<24));
/// ALTEK_MODIFIED <<<
#else
				udoffset = 4;
				p_patched_stats->cnt_hor[index] = (add_stat_32[udoffset] + (add_stat_32[udoffset+1]<<8) + (add_stat_32[udoffset+2]<<16) + (add_stat_32[udoffset+3]<<24));

				udoffset = 0;
				p_patched_stats->cnt_ver[index] = (add_stat_32[udoffset] + (add_stat_32[udoffset+1]<<8) + (add_stat_32[udoffset+2]<<16) + (add_stat_32[udoffset+3]<<24));

				udoffset = 3 * 8;
				p_patched_stats->filter_value1[index] = ((uint64)(add_stat_64[udoffset]) + ((uint64)(add_stat_64[udoffset+1])<<8) + ((uint64)(add_stat_64[udoffset+2])<<16) + ((uint64)(add_stat_64[udoffset+3])<<24) +
				                                        ((uint64)(add_stat_64[udoffset+4])<<32) + ((uint64)(add_stat_64[udoffset+5])<<40) + ((uint64)(add_stat_64[udoffset+6])<<48) + ((uint64)(add_stat_64[udoffset+7])<<56));

				udoffset = 4 * 8;
				p_patched_stats->filter_value2[index] = ((uint64)(add_stat_64[udoffset]) + ((uint64)(add_stat_64[udoffset+1])<<8) + ((uint64)(add_stat_64[udoffset+2])<<16) + ((uint64)(add_stat_64[udoffset+3])<<24) +
				                                        ((uint64)(add_stat_64[udoffset+4])<<32) + ((uint64)(add_stat_64[udoffset+5])<<40) + ((uint64)(add_stat_64[udoffset+6])<<48) + ((uint64)(add_stat_64[udoffset+7])<<56));

				udoffset = 1 * 8;
				p_patched_stats->y_factor[index] = ((uint64)(add_stat_64[udoffset]) + ((uint64)(add_stat_64[udoffset+1])<<8) + ((uint64)(add_stat_64[udoffset+2])<<16) + ((uint64)(add_stat_64[udoffset+3])<<24) +
				                                    ((uint64)(add_stat_64[udoffset+4])<<32) + ((uint64)(add_stat_64[udoffset+5])<<40) + ((uint64)(add_stat_64[udoffset+6])<<48) + ((uint64)(add_stat_64[udoffset+7])<<56));

				udoffset = 5 * 4;
				p_patched_stats->fv_hor[index] = (add_stat_32[udoffset] + (add_stat_32[udoffset+1]<<8) + (add_stat_32[udoffset+2]<<16) + (add_stat_32[udoffset+3]<<24));

				udoffset = 4 * 4;
				p_patched_stats->fv_ver[index] = (add_stat_32[udoffset] + (add_stat_32[udoffset+1]<<8) + (add_stat_32[udoffset+2]<<16) + (add_stat_32[udoffset+3]<<24));
#endif /* NEW_AF_ALGORITHM */
#endif
				/*40 Bytes(HW3A AF Block Size) = 10 * uint32(4 Bytes)*/
				stats_addr_32 += 10;
				/*40 Bytes(HW3A AF Block Size) = 5 * uint64(8 Bytes)*/
				stats_addr_64 += 5;

				add_stat_32 += 40;
				add_stat_64 += 40;

#ifdef NEW_AF_ALGORITHM
/// ALTEK_MODIFIED >>>
				WRAP_LOG("fv_hor[%d]: %d", index, p_patched_stats->hw_fv.fv_hor[index]);
/// ALTEK_MODIFIED <<<
#else
				WRAP_LOG("fv_hor[%d]: %d", index, p_patched_stats->fv_hor[index]);
#endif /* NEW_AF_ALGORITHM */
			}
		}
	} else if(AL3A_HW3A_DEV_ID_B_0 == p_meta_data_af->uhwengineid) {
		for (j = 0; j < banks; j++) {
			stats_addr_32 = (uint32 *)(stats) + j * bank_size / 4;
			add_stat_32 = stats + j* bank_size;

			for (i = 0; i < blocks; i++) {
				index = i + j * banks;
#if 0
				p_patched_stats->cnt_hor[index] = stats_addr_32[1];
				p_patched_stats->cnt_ver[index] = stats_addr_32[0];
				p_patched_stats->filter_value1[index] = 0;
				p_patched_stats->filter_value2[index] = 0;
				p_patched_stats->y_factor[index] = stats_addr_32[2];
				p_patched_stats->fv_hor[index] = stats_addr_32[4];
				p_patched_stats->fv_ver[index] = stats_addr_32[3];
#else
#ifdef NEW_AF_ALGORITHM
				udoffset = 4;
/// ALTEK_MODIFIED >>>
				p_patched_stats->hw_fv.cnt_hor[index] = (add_stat_32[udoffset] + (add_stat_32[udoffset+1]<<8) + (add_stat_32[udoffset+2]<<16) + (add_stat_32[udoffset+3]<<24));
/// ALTEK_MODIFIED <<<

				udoffset = 0;
/// ALTEK_MODIFIED >>>
				p_patched_stats->hw_fv.cnt_ver[index] = (add_stat_32[udoffset] + (add_stat_32[udoffset+1]<<8) + (add_stat_32[udoffset+2]<<16) + (add_stat_32[udoffset+3]<<24));
/// ALTEK_MODIFIED <<<

/// ALTEK_MODIFIED >>>
				p_patched_stats->hw_fv.filter_value1[index] = 0;

				p_patched_stats->hw_fv.filter_value2[index] = 0;
/// ALTEK_MODIFIED <<<

				udoffset = 2 * 4;
/// ALTEK_MODIFIED >>>
				p_patched_stats->hw_fv.y_factor[index] = (add_stat_32[udoffset] + (add_stat_32[udoffset+1]<<8) + (add_stat_32[udoffset+2]<<16) + (add_stat_32[udoffset+3]<<24));
/// ALTEK_MODIFIED <<<

				udoffset = 4 * 4;
/// ALTEK_MODIFIED >>>
				p_patched_stats->hw_fv.fv_hor[index] = (add_stat_32[udoffset] + (add_stat_32[udoffset+1]<<8) + (add_stat_32[udoffset+2]<<16) + (add_stat_32[udoffset+3]<<24));
/// ALTEK_MODIFIED <<<

				udoffset = 3 * 4;
/// ALTEK_MODIFIED >>>
				p_patched_stats->hw_fv.fv_ver[index] = (add_stat_32[udoffset] + (add_stat_32[udoffset+1]<<8) + (add_stat_32[udoffset+2]<<16) + (add_stat_32[udoffset+3]<<24));
/// ALTEK_MODIFIED <<<
#else
				udoffset = 4;
				p_patched_stats->cnt_hor[index] = (add_stat_32[udoffset] + (add_stat_32[udoffset+1]<<8) + (add_stat_32[udoffset+2]<<16) + (add_stat_32[udoffset+3]<<24));

				udoffset = 0;
				p_patched_stats->cnt_ver[index] = (add_stat_32[udoffset] + (add_stat_32[udoffset+1]<<8) + (add_stat_32[udoffset+2]<<16) + (add_stat_32[udoffset+3]<<24));

				p_patched_stats->filter_value1[index] = 0;
				p_patched_stats->filter_value2[index] = 0;

				udoffset = 2 * 4;
				p_patched_stats->y_factor[index] = (add_stat_32[udoffset] + (add_stat_32[udoffset+1]<<8) + (add_stat_32[udoffset+2]<<16) + (add_stat_32[udoffset+3]<<24));

				udoffset = 4 * 4;
				p_patched_stats->fv_hor[index] = (add_stat_32[udoffset] + (add_stat_32[udoffset+1]<<8) + (add_stat_32[udoffset+2]<<16) + (add_stat_32[udoffset+3]<<24));

				udoffset = 3 * 4;
				p_patched_stats->fv_ver[index] = (add_stat_32[udoffset] + (add_stat_32[udoffset+1]<<8) + (add_stat_32[udoffset+2]<<16) + (add_stat_32[udoffset+3]<<24));
#endif /* NEW_AF_ALGORITHM */
#endif
				/*24 Bytes(HW3A AF Block Size) = 6 * uint32(4 Bytes)*/
				stats_addr_32 += 6;

				add_stat_32 += 24;

#ifdef NEW_AF_ALGORITHM
/// ALTEK_MODIFIED >>>
				WRAP_LOG("fv_hor[%d]: %d", index, p_patched_stats->hw_fv.fv_hor[index]);
/// ALTEK_MODIFIED <<<
#else
				WRAP_LOG("fv_hor[%d]: %d", index, p_patched_stats->fv_hor[index]);
#endif /* NEW_AF_ALGORITHM */
			}
		}
	} else {
		WRAP_LOG("ERR_WRP_AF_INVALID_ENGINE");
		return ERR_WRP_AF_INVALID_ENGINE;
	}

#ifdef NEW_AF_ALGORITHM
/// ALTEK_MODIFIED >>>
	p_patched_stats->hw_fv.af_token_id = p_meta_data_af->uaftokenid;
	p_patched_stats->hw_fv.hw3a_frame_id = p_meta_data_af->uframeidx;
	p_patched_stats->hw_fv.time_stamp.time_stamp_sec = p_meta_data_af->systemtime.tv_sec;
	p_patched_stats->hw_fv.time_stamp.time_stamp_us = p_meta_data_af->systemtime.tv_usec;
/// ALTEK_MODIFIED <<<
#else
	p_patched_stats->af_token_id = p_meta_data_af->uaftokenid;
	p_patched_stats->hw3a_frame_id = p_meta_data_af->uframeidx;
	p_patched_stats->time_stamp.time_stamp_sec = p_meta_data_af->systemtime.tv_sec;
	p_patched_stats->time_stamp.time_stamp_us = p_meta_data_af->systemtime.tv_usec;
#endif

	return ERR_WPR_AF_SUCCESS;
}

#ifdef NEW_AF_ALGORITHM
/// ALTEK_MODIFIED >>>
uint32 al3awrapper_dispatchhw3a_yhist(void *isp_yhist,void *alaf_yhist)
{
	uint32 ret = ERR_WPR_AF_SUCCESS;
	struct al3awrapper_stats_yhist_t *p_yhist;
	struct allib_af_hw_stats_t *p_patched_stats;

	/* check input parameter validity*/
	if (isp_yhist == NULL) {
		WRAP_LOG("ERR_WRP_AF_EMPTY_METADATA");
		return ERR_WRP_AF_EMPTY_METADATA;
	}

	if (alaf_yhist == NULL) {
		WRAP_LOG("ERR_WRP_AF_INVALID_INPUT_PARAM");
		return ERR_WRP_AF_INVALID_INPUT_PARAM;
	}

	p_yhist = (struct al3awrapper_stats_yhist_t *)isp_yhist;
	p_patched_stats = (struct allib_af_hw_stats_t *)alaf_yhist;

	p_patched_stats->y_hist.b_is_stats_byaddr = p_yhist->b_is_stats_byaddr;
	p_patched_stats->y_hist.ustructuresize = p_yhist->ustructuresize;
	p_patched_stats->y_hist.umagicnum = p_yhist->umagicnum;
	p_patched_stats->y_hist.uhwengineid = p_yhist->uhwengineid;
	p_patched_stats->y_hist.uframeidx = p_yhist->uframeidx;
	p_patched_stats->y_hist.u_yhist_tokenid = p_yhist->u_yhist_tokenid;
	p_patched_stats->y_hist.u_yhist_statssize = p_yhist->u_yhist_statssize;
	p_patched_stats->y_hist.systemtime.time_stamp_sec = p_yhist->systemtime.tv_sec;
	p_patched_stats->y_hist.systemtime.time_stamp_us = p_yhist->systemtime.tv_usec;
	p_patched_stats->y_hist.udsys_sof_idx = p_yhist->udsys_sof_idx;

	/* switched by stats addr or fix array */
	if ( p_patched_stats->y_hist.b_is_stats_byaddr ) {
		p_patched_stats->y_hist.pt_hist_y = p_yhist->pt_hist_y;
	} else
		memcpy(p_patched_stats->y_hist.hist_y, p_yhist->hist_y, sizeof(p_yhist->hist_y));

	return ERR_WPR_AF_SUCCESS;
}
/// ALTEK_MODIFIED <<<
#endif /* NEW_AF_ALGORITHM */

/*
 * API name: al3awrapperaf_translatefocusmodetoaptype
 * This API used for translating AF lib focus mode define to framework
 * param focus_mode[In] :   Focus mode define of AF lib (Altek)
 * return: Focus mode define for AP framework
 */
uint32 al3awrapperaf_translatefocusmodetoaptype(uint32 focus_mode)
{
	uint32 ret_focus_mode;

	switch(focus_mode) {
	/*
	* here just sample code, need to be implement by Framework define
	      case alAFLib_AF_MODE_AUTO:
		  ret_focus_mode = FOCUS_MODE_AUTO;
		  break;
	      case alAFLib_AF_MODE_MANUAL:
		  ret_focus_mode = FOCUS_MODE_MANUAL;
		  break;
	      case alAFLib_AF_MODE_MACRO:
		  ret_focus_mode = FOCUS_MODE_MACRO;
		  break;
	      case alAFLib_AF_MODE_OFF:
		  ret_focus_mode = FOCUS_MODE_OFF;
		  break;
	      case alAFLib_AF_MODE_OFF:
		  ret_focus_mode = FOCUS_MODE_FIXED;
		  break;
	      case alAFLib_AF_MODE_EDOF:
		  ret_focus_mode = FOCUS_MODE_EDOF;
		  break;
	*/
	default:
		ret_focus_mode = 0;
		break;
	}

	return ret_focus_mode;
}

/*
 * API name: al3awrapperaf_translatefocusmodetoaflibtype
 * This API used for translating framework focus mode to AF lib define
 * focus_mode[In] :   Focus mode define of AP framework define
 * return: Focus mode define for AF lib (Altek)
 */
uint32 al3awrapperaf_translatefocusmodetoaflibtype(uint32 focus_mode)
{
	uint32 ret_focus_mode;

	switch(focus_mode) {
	/*
	 here just sample code, need to be implement by Framework define

	case FOCUS_MODE_AUTO:
	case FOCUS_MODE_CONTINUOUS_PICTURE:
	case FOCUS_MODE_CONTINUOUS_VIDEO:
	    ret_focus_mode = alAFLib_AF_MODE_AUTO;
	    break;

	case FOCUS_MODE_MANUAL:
	case FOCUS_MODE_INFINITY:
	    ret_focus_mode = alAFLib_AF_MODE_MANUAL;
	    break;

	case FOCUS_MODE_MACRO:
	    ret_focus_mode = alAFLib_AF_MODE_MACRO;
	    break;

	case FOCUS_MODE_OFF:
	    ret_focus_mode = alAFLib_AF_MODE_OFF;
	    break;
	case FOCUS_MODE_FIXED:
	    ret_focus_mode = alAFLib_AF_MODE_OFF;
	    break;
	case FOCUS_MODE_EDOF:
	    ret_focus_mode = alAFLib_AF_MODE_EDOF;
	    break;

	*/
	default:
		ret_focus_mode = alAFLib_AF_MODE_NOT_SUPPORTED;
		break;
	}

	return ret_focus_mode;
}

/*
 * API name: al3awrapperaf_translatecalibdattoaflibtype
 * This API used for translating EEPROM data to AF lib define
 * eeprom_addr[In] :   EEPROM data address
 * AF_Calib_Dat[Out] :   Altek data format
 * return: Error code
 */
uint32 al3awrapperaf_translatecalibdattoaflibtype(void *eeprom_addr,struct allib_af_input_init_info_t *af_Init_data)
{
	if(NULL == eeprom_addr)
		return ERR_WRP_AF_EMPTY_EEPROM_ADDR;

	if(NULL == af_Init_data)
		return ERR_WRP_AF_EMPTY_INIT_ADDR;

	uint32 *eeprom_addr_32 = (uint32 *)eeprom_addr;

	af_Init_data->calib_data.inf_step = eeprom_addr_32[VCM_INF_STEP_ADDR_OFFSET];
	af_Init_data->calib_data.macro_step = eeprom_addr_32[VCM_MACRO_STEP_ADDR_OFFSET];
	af_Init_data->calib_data.inf_distance = VCM_INF_STEP_CALIB_DISTANCE;
	af_Init_data->calib_data.macro_distance = VCM_MACRO_STEP_CALIB_DISTANCE;
	af_Init_data->module_info.f_number = MODULE_F_NUMBER;
	af_Init_data->module_info.focal_lenth = MODULE_FOCAL_LENGTH;
	af_Init_data->calib_data.mech_top = VCM_INF_STEP_MECH_TOP;
	af_Init_data->calib_data.mech_bottom = VCM_INF_STEP_MECH_BOTTOM;
	af_Init_data->calib_data.lens_move_stable_time = VCM_MOVING_STATBLE_TIME;
	/* extra lib calibration data, if not support, set to null.*/
	af_Init_data->calib_data.extend_calib_ptr = NULL;
	/* size of extra lib calibration data, if not support set to zero.*/
	af_Init_data->calib_data.extend_calib_data_size = 0;

	return ERR_WPR_AF_SUCCESS;
}

/*
 * API name: al3awrapperaf_translateroitoaflibtype
 * This API used for translating ROI info to AF lib define
 * frame_id[In] :   Current frame id
 * roi_info[Out] :   Altek data format
 * return: Error code
 */
uint32 al3awrapperaf_translateroitoaflibtype(unsigned int frame_id,struct allib_af_input_roi_info_t *roi_info)
{
	/*Sample code for continuous AF default setting in sensor raw size  1280*960 , 30% * 30% crop*/
	roi_info->roi_updated = TRUE;
	roi_info->type = alAFLib_ROI_TYPE_NORMAL;
	roi_info->frame_id = frame_id;
	roi_info->num_roi = 1;
	roi_info->roi[0].uw_left = 422;
	roi_info->roi[0].uw_top = 317;
	roi_info->roi[0].uw_dx = 422;
	roi_info->roi[0].uw_dy = 317;
	roi_info->weight[0] = 1;
	roi_info->src_img_sz.uw_width = 1280;
	roi_info->src_img_sz.uw_height = 960;

	return ERR_WPR_AF_SUCCESS;
}

/*
 * API name: al3awrapperaf_updateispconfig_af
 * This API is used for query ISP config before calling al3awrapperaf_updateispconfig_af
 * param lib_config[in]: AF stats config info from AF lib
 * param isp_config[out]: input buffer, API would manage parameter and return via this pointer
 * return: error code
 */
uint32 al3awrapperaf_updateispconfig_af(struct allib_af_out_stats_config_t *lib_config,struct alhw3a_af_cfginfo_t *isp_config)
{
	uint32 ret = ERR_WPR_AF_SUCCESS;
	uint32 udTemp = 0;
	WRAP_LOG("al3awrapperaf_updateispconfig_af start ID: %d", lib_config->token_id);

	isp_config->tokenid = lib_config->token_id;

	isp_config->tafregion.uwblknumx = lib_config->num_blk_hor;
	WRAP_LOG("uwblknumx: %d", isp_config->tafregion.uwblknumx);

	isp_config->tafregion.uwblknumy = lib_config->num_blk_ver;
	WRAP_LOG("uwblknumy: %d", isp_config->tafregion.uwblknumy);

	if (0 == lib_config->src_img_sz.uw_width)
		return ERR_WRP_AF_ZERO_SRC_IMG_WIDTH;

	if (0 == lib_config->src_img_sz.uw_height)
		return ERR_WRP_AF_ZERO_SRC_IMG_HEIGHT;

	udTemp = 100 * ((uint32)(lib_config->roi.uw_dx)) / ((uint32)(lib_config->src_img_sz.uw_width));
	WRAP_LOG("uw_dx: %d, uw_width: %d", lib_config->roi.uw_dx, lib_config->src_img_sz.uw_width);

	isp_config->tafregion.uwsizeratiox = (uint16)udTemp;
	WRAP_LOG("uwsizeratiox: %d", isp_config->tafregion.uwsizeratiox);

	udTemp = 100 * ((uint32)(lib_config->roi.uw_dy)) / ((uint32)(lib_config->src_img_sz.uw_height));
	WRAP_LOG("uw_dy: %d, uw_height: %d", lib_config->roi.uw_dy, lib_config->src_img_sz.uw_height);

	isp_config->tafregion.uwsizeratioy = (uint16)udTemp;
	WRAP_LOG("uwsizeratioy: %d", isp_config->tafregion.uwsizeratioy);

	udTemp = 100 * ((uint32)(lib_config->roi.uw_left)) / ((uint32)(lib_config->src_img_sz.uw_width));
	WRAP_LOG("uw_left: %d, uw_width: %d", lib_config->roi.uw_left, lib_config->src_img_sz.uw_width);

	isp_config->tafregion.uwoffsetratiox = (uint16)udTemp;
	WRAP_LOG("uwoffsetratiox: %d", isp_config->tafregion.uwoffsetratiox);

	udTemp = 100 * ((uint32)(lib_config->roi.uw_top)) / ((uint32)(lib_config->src_img_sz.uw_height));
	WRAP_LOG("uw_top: %d, uw_height: %d", lib_config->roi.uw_top, lib_config->src_img_sz.uw_height);

	isp_config->tafregion.uwoffsetratioy = (uint16)udTemp;
	WRAP_LOG("uwoffsetratioy: %d", isp_config->tafregion.uwoffsetratioy);

	isp_config->benableaflut = lib_config->b_enable_af_lut;
	WRAP_LOG("benableaflut: %d", isp_config->benableaflut);

	memcpy(isp_config->auwlut, lib_config->auw_lut, sizeof(uint16) * 259);
	memcpy(isp_config->auwaflut, lib_config->auw_af_lut, sizeof(uint16) * 259);
	memcpy(isp_config->aucweight, lib_config->auc_weight, sizeof(uint8) * 6);

	isp_config->uwsh = lib_config->uw_sh;
	WRAP_LOG("uwsh: %d", isp_config->uwsh);

	isp_config->ucthmode = lib_config->uc_th_mode;
	WRAP_LOG("ucthmode: %d", isp_config->ucthmode);

	memcpy(isp_config->aucindex, lib_config->auc_index, sizeof(uint8) * 82);
	memcpy(isp_config->auwth, lib_config->auw_th, sizeof(uint16) * 4);
	memcpy(isp_config->pwtv, lib_config->pw_tv, sizeof(uint16) * 4);

	isp_config->udafoffset = lib_config->ud_af_offset;
	WRAP_LOG("udafoffset: %d", isp_config->udafoffset);

	isp_config->baf_py_enable = lib_config->b_af_py_enable;
	WRAP_LOG("baf_py_enable: %d", isp_config->baf_py_enable);

	isp_config->baf_lpf_enable = lib_config->b_af_lpf_enable;
	WRAP_LOG("baf_lpf_enable: %d", isp_config->baf_lpf_enable);

	switch (lib_config->n_filter_mode) {
	case MODE_51:
		isp_config->nfiltermode = HW3A_MF_51_MODE;
		break;

	case MODE_31:
		isp_config->nfiltermode = HW3A_MF_31_MODE;
		break;

	case DISABLE:
	default:
		isp_config->nfiltermode = HW3A_MF_DISABLE;
		break;
	}

	WRAP_LOG("nfiltermode: %d", isp_config->nfiltermode);

	isp_config->ucfilterid = lib_config->uc_filter_id;
	WRAP_LOG("ucfilterid: %d", isp_config->ucfilterid);

	isp_config->uwlinecnt = lib_config->uw_line_cnt;
	WRAP_LOG("uwlinecnt: %d",isp_config->uwlinecnt);

#ifdef IIR_PARAM_ENABLE
	isp_config->tlpf.udb0 = lib_config->t_lpf.udb0;
	isp_config->tlpf.udb1 = lib_config->t_lpf.udb1;
	isp_config->tlpf.udb2 = lib_config->t_lpf.udb2;
	isp_config->tlpf.udb3 = lib_config->t_lpf.udb3;
	isp_config->tlpf.udb4 = lib_config->t_lpf.udb4;
	isp_config->tlpf.udb5 = lib_config->t_lpf.udb5;
	isp_config->tlpf.udb6 = lib_config->t_lpf.udb6;
	isp_config->tlpf.udshift = lib_config->t_lpf.udshift;

	isp_config->atiir[0].binitbyuser = lib_config->at_iir[0].binitbyuser;
	isp_config->atiir[0].udp = lib_config->at_iir[0].udp;
	isp_config->atiir[0].udq = lib_config->at_iir[0].udq;
	isp_config->atiir[0].udr = lib_config->at_iir[0].udr;
	isp_config->atiir[0].uds = lib_config->at_iir[0].uds;
	isp_config->atiir[0].udt = lib_config->at_iir[0].udt;
	isp_config->atiir[0].udabsshift = lib_config->at_iir[0].udabsshift;
	isp_config->atiir[0].udth = lib_config->at_iir[0].udth;
	isp_config->atiir[0].udinita = lib_config->at_iir[0].udinita;
	isp_config->atiir[0].udinitb = lib_config->at_iir[0].udinitb;

	isp_config->atiir[1].binitbyuser = lib_config->at_iir[1].binitbyuser;
	isp_config->atiir[1].udp = lib_config->at_iir[1].udp;
	isp_config->atiir[1].udq = lib_config->at_iir[1].udq;
	isp_config->atiir[1].udr = lib_config->at_iir[1].udr;
	isp_config->atiir[1].uds = lib_config->at_iir[1].uds;
	isp_config->atiir[1].udt = lib_config->at_iir[1].udt;
	isp_config->atiir[1].udabsshift = lib_config->at_iir[1].udabsshift;
	isp_config->atiir[1].udth = lib_config->at_iir[1].udth;
	isp_config->atiir[1].udinita = lib_config->at_iir[1].udinita;
	isp_config->atiir[1].udinitb = lib_config->at_iir[1].udinitb;

	isp_config->tpseudoy.udwr = lib_config->t_pseudoy.udwr;
	isp_config->tpseudoy.udwgr = lib_config->t_pseudoy.udwgr;
	isp_config->tpseudoy.udwgb = lib_config->t_pseudoy.udwgb;
	isp_config->tpseudoy.udwb = lib_config->t_pseudoy.udwb;
	isp_config->tpseudoy.udshift = lib_config->t_pseudoy.udshift;
	isp_config->tpseudoy.udoffset = lib_config->t_pseudoy.udoffset;
	isp_config->tpseudoy.udgain = lib_config->t_pseudoy.udgain;
#endif

	return ret;
}

#ifdef NEW_AF_ALGORITHM
/// ALTEK_MODIFIED >>>
/*
 * API name: al3awrapperaf_updateispconfig_yhis
 * This API is used for translate ISP config
 * param lib_config[in]: AF stats config info from AF lib
 * param isp_config[out]: input buffer, API would manage parameter and return via this pointer
 * return: error code
 */
uint32 al3awrapperaf_updateispconfig_yhis(struct allib_af_out_stats_config_t *lib_config, struct alhw3a_yhist_cfginfo_t *isp_config)
{
	uint32 ret = ERR_WPR_AF_SUCCESS;
	uint32 udTemp = 0;
	WRAP_LOG("al3awrapperaf_updateispconfig_yhis start ID: %d", lib_config->token_id);

	isp_config->tokenid = lib_config->token_id;

	isp_config->tyhistregion.uwblknumx = lib_config->num_blk_hor;
	WRAP_LOG("uwblknumx: %u", isp_config->tyhistregion.uwblknumx);

	isp_config->tyhistregion.uwblknumy = lib_config->num_blk_ver;
	WRAP_LOG("uwblknumy: %u", isp_config->tyhistregion.uwblknumy);

	if (0 == lib_config->src_img_sz.uw_width)
		return ERR_WRP_AF_ZERO_SRC_IMG_WIDTH;

	if (0 == lib_config->src_img_sz.uw_height)
		return ERR_WRP_AF_ZERO_SRC_IMG_HEIGHT;

	udTemp = 100 * ((uint32)(lib_config->roi.uw_dx)) / ((uint32)(lib_config->src_img_sz.uw_width));
	WRAP_LOG("uw_dx: %u, uw_width: %u", lib_config->roi.uw_dx, lib_config->src_img_sz.uw_width);

	isp_config->tyhistregion.uwborderratiox = (uint16)udTemp;
	WRAP_LOG("uwborderratiox: %u", isp_config->tyhistregion.uwborderratiox);

	udTemp = 100 * ((uint32)(lib_config->roi.uw_dy)) / ((uint32)(lib_config->src_img_sz.uw_height));
	WRAP_LOG("uw_dy: %u, uw_height: %u", lib_config->roi.uw_dy, lib_config->src_img_sz.uw_height);

	isp_config->tyhistregion.uwborderratioy = (uint16)udTemp;
	WRAP_LOG("uwborderratioy: %u", isp_config->tyhistregion.uwborderratioy);

	udTemp = 100 * ((uint32)(lib_config->roi.uw_left)) / ((uint32)(lib_config->src_img_sz.uw_width));
	WRAP_LOG("uw_left: %u, uw_width: %u", lib_config->roi.uw_left, lib_config->src_img_sz.uw_width);

	isp_config->tyhistregion.uwoffsetratiox = (uint16)udTemp;
	WRAP_LOG("uwoffsetratiox: %u", isp_config->tyhistregion.uwoffsetratiox);

	udTemp = 100 * ((uint32)(lib_config->roi.uw_top)) / ((uint32)(lib_config->src_img_sz.uw_height));
	WRAP_LOG("uw_top: %u, uw_height: %u", lib_config->roi.uw_top, lib_config->src_img_sz.uw_height);

	isp_config->tyhistregion.uwoffsetratioy = (uint16)udTemp;
	WRAP_LOG("uwoffsetratioy: %u", isp_config->tyhistregion.uwoffsetratioy);

	return ret;
}
/// ALTEK_MODIFIED <<<
#endif /* NEW_AF_ALGORITHM */

/*
 * API name: al3awrapper_updateafreport
 * This API is used for translate AF status  to generate AF report for AP
 * param lib_status[in]: af status from AF update
 * param report[out]: AF report for AP
 * return: error code
 */
uint32 al3awrapper_updateafreport(struct allib_af_out_status_t *lib_status,struct af_report_update_t *report)
{
	uint32 ret = ERR_WPR_AF_SUCCESS;

	if(NULL == lib_status)
		return ERR_WRP_AF_EMPTY_AF_STATUS;

	report->focus_done = lib_status->focus_done;
	report->status = lib_status->t_status;
	report->f_distance = lib_status->f_distance;

	return ret;
}

/*
 * API name: al3awrapperaf_translateaeinfotoaflibtype
 * This API used for translating AE info to AF lib define
 * ae_report_update_t[In] :   Iput AE data
 * struct allib_af_input_aec_info_t[Out] :   Altek data format
 * return: Error code
 */
uint32 al3awrapperaf_translateaeinfotoaflibtype(struct ae_report_update_t *ae_report,struct allib_af_input_aec_info_t *ae_info)
{
	if(NULL == ae_report)
		return ERR_WRP_AF_EMPTY_AE_INFO_INPUT;

	if(NULL == ae_info)
		return ERR_WRP_AF_EMPTY_AF_INFO_OUTPUT;

	ae_info->ae_settled = ae_report->ae_converged;
	ae_info->cur_intensity = (float)(ae_report->curmean);
	ae_info->target_intensity = (float)(ae_report->targetmean);

	if(ae_report->bv_val+5000 > 30000)
		ae_info->brightness = 30000;
	else if(ae_report->bv_val-5000 <-30000)
		ae_info->brightness =-30000;
	else
		ae_info->brightness = (short)(ae_report->bv_val+5000);

	ae_info->preview_fr = ae_report->cur_fps;
	ae_info->cur_gain = (float)(ae_report->sensor_ad_gain);

	return ERR_WPR_AF_SUCCESS;
}

/*
 * API name: al3awrapperaf_getdefaultcfg
 * This API is used for query default ISP config before calling al3awrapperaf_updateispconfig_af
 * param isp_config[out]: input buffer, API would manage parameter and return via this pointer
 * return: error code
 */
uint32 al3awrapperaf_getdefaultcfg(struct alhw3a_af_cfginfo_t *isp_config)
{
	uint32 ret = ERR_WPR_AF_SUCCESS;
	uint8 auc_af_blk_th_idx[82] = AF_DEFAULT_AFBLK_TH;
	uint16 auw_af_th_lut[4] = AF_DEFAULT_THLUT;
	uint8 auc_wei[6] = AF_DEFAULT_MASK;
	uint16 auw_af_lut[259] = AF_DEFAULT_LUT;

	if(isp_config == NULL)
		return ERR_WRP_AF_INVALID_INPUT_PARAM;

	isp_config->tokenid = 0x01;
	isp_config->tafregion.uwblknumx = 3;
	isp_config->tafregion.uwblknumy = 3;
	isp_config->tafregion.uwsizeratiox = 33;
	isp_config->tafregion.uwsizeratioy = 33;
	isp_config->tafregion.uwoffsetratiox = 33;
	isp_config->tafregion.uwoffsetratioy = 33;
	isp_config->benableaflut = FALSE;
	memcpy(isp_config->auwlut,auw_af_lut,sizeof(uint16)*259);
	memcpy(isp_config->auwaflut,auw_af_lut,sizeof(uint16)*259);
	memcpy(isp_config->aucweight,auc_wei,sizeof(uint8)*6);
	isp_config->uwsh = 0;
	isp_config->ucthmode = 0;
	memcpy(isp_config->aucindex,auc_af_blk_th_idx,sizeof(uint8)*82);
	memcpy(isp_config->auwth,auw_af_th_lut,sizeof(uint16)*4);
	memcpy(isp_config->pwtv,auw_af_th_lut,sizeof(uint16)*4);
	isp_config->udafoffset = 0xFFFF; // 0xFFFF = get value from sensor clamp level at driver layer
	isp_config->baf_py_enable = FALSE;
	isp_config->baf_lpf_enable = FALSE;
	isp_config->nfiltermode = HW3A_MF_DISABLE;
	isp_config->ucfilterid = 0;
	isp_config->uwlinecnt = 1;

	return ret;
}

/*
 * API name: al3awrapperaf_get_version
 * This API would return labeled version of wrapper
 * fWrapVersion[out], return current wapper version
 * return: error code
 */
uint32 al3awrapperaf_get_version(float *wrap_version)
{
	uint32 ret = ERR_WPR_AF_SUCCESS;

	*wrap_version = _WRAPPER_AF_VER;

	return ret;
}
