/********************************************************************************
 * al3awrapper_caf.c
 *
 *  Created on: 2016/6/15
 *      Author: Marktseng
 *  Comments:
 *       This c file is mainly used for AP framework to:
 *       1. Query HW3A config setting
 *       2. Set HW3A via AP IPS driver of altek
********************************************************************************/

/* test build in local */

#include "mtype.h"
#include "frmwk_hw3a_event_type.h"
#include "hw3a_stats.h"

/* Wrapper define */
#include "alwrapper_3a.h"
#include "alwrapper_caf.h"
#include "alwrapper_caf_errcode.h"

#include <math.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <android/log.h>

/* Debug only */
#if 0
#define WRAP_LOG(...) __android_log_print(ANDROID_LOG_WARN, "caf_wrapper", __VA_ARGS__)
#else
#define WRAP_LOG(...) do { } while(0)
#endif

#define CAF_DEFAULT_AFBLK_TH {\
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,\
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,\
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,\
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
#define CAF_DEFAULT_THLUT {0, 0, 0, 0}
#define CAF_DEFAULT_MASK {0, 0, 0, 1, 1, 2}
#define CAF_DEFAULT_LUT {   0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,\
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
 * API name: al3awrapper_caf_transform_cfg
 * param lib_config[in]: input caf configuration
 * param isp_config[out]: output hwaf config for caf's request
 * return: error code
 */
uint32 al3awrapper_caf_transform_cfg(struct lib_caf_stats_config_t *lib_config,struct alhw3a_af_cfginfo_t *isp_config)
{
	uint32 ret = ERR_WPR_CAF_SUCCESS;
	uint8 auc_af_blk_th_idx[82] = CAF_DEFAULT_AFBLK_TH;
	uint16 auw_af_th_lut[4] = CAF_DEFAULT_THLUT;
	uint8 auc_wei[6] = CAF_DEFAULT_MASK;
	uint16 auw_af_lut[259] = CAF_DEFAULT_LUT;

	if(lib_config == NULL)
		return ERR_WRP_CAF_NULL_INPUT_PARAM;

	if(isp_config == NULL)
		return ERR_WRP_CAF_NULL_OUTPUT_PARAM;
	
	isp_config->tokenid = CAF_CONFIG_ID;

	if(CAF_MAX_COLUMN_NUM < lib_config->num_blk_hor)
		return ERR_WRP_CAF_INVALID_COLUMN_NUM;
	
	isp_config->tafregion.uwblknumx = lib_config->num_blk_hor;

	if(CAF_MAX_ROW_NUM < lib_config->num_blk_ver)
		return ERR_WRP_CAF_INVALID_ROW_NUM;

	isp_config->tafregion.uwblknumy = lib_config->num_blk_ver;

	if(CAF_MAX_SIZE_RATIO < lib_config->roi_width_ratio)
		return ERR_WRP_CAF_WRONG_ROI_CONFIG;
	
	isp_config->tafregion.uwsizeratiox = lib_config->roi_width_ratio;

	if(CAF_MAX_SIZE_RATIO < lib_config->roi_height_ratio)
		return ERR_WRP_CAF_WRONG_ROI_CONFIG;
	
	isp_config->tafregion.uwsizeratioy = lib_config->roi_height_ratio;

	if(CAF_MAX_OFFSET_RATIO < lib_config->roi_left_ratio)
		return ERR_WRP_CAF_WRONG_ROI_CONFIG;
	
	isp_config->tafregion.uwoffsetratiox = lib_config->roi_left_ratio;

	if(CAF_MAX_OFFSET_RATIO < lib_config->roi_top_ratio)
		return ERR_WRP_CAF_WRONG_ROI_CONFIG;
	
	isp_config->tafregion.uwoffsetratioy = lib_config->roi_top_ratio;
	
    WRAP_LOG("CAF config off_x:%d,off_y:%d,w:%d,h:%d \n", isp_config->tafregion.uwoffsetratiox, isp_config->tafregion.uwoffsetratioy,
    isp_config->tafregion.uwsizeratiox, isp_config->tafregion.uwsizeratioy );  
    
	isp_config->benableaflut = FALSE;
	memcpy(isp_config->auwlut,auw_af_lut,sizeof(uint16)*259);
	memcpy(isp_config->auwaflut,auw_af_lut,sizeof(uint16)*259);
	memcpy(isp_config->aucweight,auc_wei,sizeof(uint8)*6);
	isp_config->uwsh = 0;
	isp_config->ucthmode = 0;
	memcpy(isp_config->aucindex,auc_af_blk_th_idx,sizeof(uint8)*82);
	memcpy(isp_config->auwth,auw_af_th_lut,sizeof(uint16)*4);
	memcpy(isp_config->pwtv,auw_af_th_lut,sizeof(uint16)*4);
	isp_config->udafoffset = 0x80;
	isp_config->baf_py_enable = FALSE;
	isp_config->baf_lpf_enable = FALSE;
	isp_config->nfiltermode = HW3A_MF_DISABLE;
	isp_config->ucfilterid = 0;
	isp_config->uwlinecnt = 1;

	return ret;
}

/*
 * API name: al3awrapper_dispatch_caf_stats
 * This API used for patching HW3A stats from ISP(Altek) for CAF libs, after patching completed, AF ctrl should prepare patched
 * stats to AF libs
 * param isp_meta_data[In]: patched data after calling al3awrapper_dispatch_caf_stats, used for patch AF stats for CAF lib
 * param caf_ref_config[In]: CAF config for reference, used to limit valid banks
 * param alaf_stats[Out]: result patched AF stats
 * return: error code
 */
uint32 al3awrapper_dispatch_caf_stats(void *isp_meta_data, void *caf_ref_config, void *caf_stats )
{
	uint32 ret = ERR_WPR_CAF_SUCCESS;
	struct isp_drv_meta_af_t *p_meta_data_af;
	struct lib_caf_stats_t *p_patched_stats;
	uint32 total_blocks;
	uint32 bank_size;
	uint16 i = 0,j = 0,blocks, banks, index;
    uint8  *stats;
	int64   addr_diff;
	uint8 *add_stat_32, *add_stat_64;    
    uint32  udoffset;
    struct lib_caf_stats_config_t *caf_fv_cfg = NULL;
    
    WRAP_LOG("al3awrapper_dispatchhw3a_cafstats start\n");
    
	/* check input parameter validity*/
	if(isp_meta_data == NULL) {
        WRAP_LOG("ERR_WRP_CAF_EMPTY_METADATA\n");
        return ERR_WRP_CAF_NULL_INPUT_PARAM;        
    }

	if(caf_stats == NULL) {
        WRAP_LOG("ERR_WRP_CAF_INVALID_OUTPUT_PARAM\n");
        return ERR_WRP_CAF_NULL_OUTPUT_PARAM;
    }
    
    if (caf_ref_config == NULL ) {
        WRAP_LOG("ERR_WRP_CAF_NULL_REF_CONFIG_PARAM\n");
        return ERR_WRP_CAF_NULL_REF_CONFIG_PARAM;
    }

	p_meta_data_af = (struct isp_drv_meta_af_t *)isp_meta_data;
	p_patched_stats = (struct lib_caf_stats_t *)caf_stats;
    caf_fv_cfg = (struct lib_caf_stats_config_t *)caf_ref_config;
    
	if(CAF_CONFIG_ID != p_meta_data_af->uaftokenid)
		return ERR_WRP_CAF_CONFIG_ID_MISMATCH;
	
	total_blocks = p_meta_data_af->af_stats_info.ucvalidblocks * p_meta_data_af->af_stats_info.ucvalidbanks;
	bank_size = p_meta_data_af->af_stats_info.udbanksize;
	blocks = p_meta_data_af->af_stats_info.ucvalidblocks;

	if(blocks > CAF_MAX_COLUMN_NUM)
		blocks = CAF_MAX_COLUMN_NUM;
		
	/* Cut to caf config blocks when valid blocks larger */
    if ( blocks > caf_fv_cfg->num_blk_hor )
        blocks = caf_fv_cfg->num_blk_hor;
    
	p_patched_stats->valid_column_num = blocks;
	banks = p_meta_data_af->af_stats_info.ucvalidbanks;
    
    WRAP_LOG("CAF valid blocks: %d, banks %d\n", blocks, banks);
    
	if(banks > CAF_MAX_ROW_NUM)
		banks = CAF_MAX_ROW_NUM;

    /* Cut to caf config banks when valid banks larger */
    if ( banks > caf_fv_cfg->num_blk_ver )
        banks = caf_fv_cfg->num_blk_ver;
    
	p_patched_stats->valid_row_num = banks;
	index = 0;

	if ( p_meta_data_af->b_isstats_byaddr == 1 ) {
		if ( p_meta_data_af->puc_af_stats == NULL )
			return ERR_WRP_CAF_NULL_INPUT_PARAM;
		stats = p_meta_data_af->puc_af_stats;
	} else
		stats = p_meta_data_af->paf_stats;    
    
    
	if(AL3A_HW3A_DEV_ID_A_0 == p_meta_data_af->uhwengineid) {
		for(j = 0; j < banks; j++) {


			add_stat_32 = stats + j* bank_size;
			add_stat_64 = add_stat_32;

			for(i = 0; i < blocks; i++) {
				index = i+j*blocks;
                
                udoffset = 5 * 4;
                
				p_patched_stats->fv[index] = ( add_stat_32[udoffset] + (add_stat_32[udoffset+1]<<8) + (add_stat_32[udoffset+2]<<16) + (add_stat_32[udoffset+3]<<24) );
				udoffset = 4 * 4;
                
                p_patched_stats->fv[index] += ( add_stat_32[udoffset] + (add_stat_32[udoffset+1]<<8) + (add_stat_32[udoffset+2]<<16) + (add_stat_32[udoffset+3]<<24) );
                
				add_stat_32 += 40;
				add_stat_64 += 40;           

                WRAP_LOG("CAF fv[%d] %d\n",index,p_patched_stats->fv[index]);                
			}
		}
	} else if(AL3A_HW3A_DEV_ID_B_0 == p_meta_data_af->uhwengineid) {
		for(j = 0; j < banks; j++) {

			add_stat_32 = stats + j* bank_size;
            
			for(i = 0; i < blocks; i++) {
				index = i+j*blocks;                
                
				udoffset = 4 * 4;
				p_patched_stats->fv[index] = ( add_stat_32[udoffset] + (add_stat_32[udoffset+1]<<8) + (add_stat_32[udoffset+2]<<16) + (add_stat_32[udoffset+3]<<24) );
				udoffset = 3 * 4;
				p_patched_stats->fv[index] += ( add_stat_32[udoffset] + (add_stat_32[udoffset+1]<<8) + (add_stat_32[udoffset+2]<<16) + (add_stat_32[udoffset+3]<<24) );
                
				add_stat_32 += 24;                
			}
		}
	} else {        
        WRAP_LOG("ERR_WRP_CAF_INVALID_ENGINE\n");
        return ERR_WRP_CAF_INVALID_ENGINE;
    }
		

	p_patched_stats->frame_id = p_meta_data_af->uframeidx;
	p_patched_stats->time_stamp.time_stamp_sec = p_meta_data_af->systemtime.tv_sec;
	p_patched_stats->time_stamp.time_stamp_us = p_meta_data_af->systemtime.tv_usec;
	return ERR_WPR_CAF_SUCCESS;
}

/*
 * API name: al3awrappercaf_get_version
 * This API would return labeled version of wrapper
 * fWrapVersion[out], return current wapper version
 * return: error code
 */
uint32 al3awrappercaf_get_version(float *wrap_version)
{
	uint32 ret = ERR_WPR_CAF_SUCCESS;

	*wrap_version = _WRAPPER_CAF_VER;

	return ret;
}
