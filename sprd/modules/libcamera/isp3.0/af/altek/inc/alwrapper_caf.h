/********************************************************************************
 * al3awrapper_af.h
 *
 *  Created on: 2015/12/06
 *      Author: ZenoKuo
 *  Latest update: 2016/2/26
 *      Reviser: ZenoKuo
 *  Comments:
********************************************************************************/

#define _WRAPPER_CAF_VER 0.8050

#define CAF_CONFIG_ID 201
#define CAF_MAX_COLUMN_NUM  (16)
#define CAF_MAX_ROW_NUM (30)
#define CAF_MAX_BLOCK_NUM (CAF_MAX_COLUMN_NUM*CAF_MAX_ROW_NUM)
#define CAF_MAX_SIZE_RATIO  (100)
#define CAF_MAX_OFFSET_RATIO  (99)

/*
 * caf_time_stamp_t
 *
 * time_stamp_sec: time stamp second.
 * time_stamp_us: time stamp microsecond.
 */
#pragma pack(push, 4)
struct caf_time_stamp_t {
	uint32 time_stamp_sec;
	uint32 time_stamp_us;
};
#pragma pack(pop)

/*
 * lib_caf_stats_t
 * caf_token_id:	valid setting number, which same when doing AF configuration, for 3A libs synchronization
 * frame_id:	frame id when hw3a compute stats.
 * valid_column_num:   Number of valid column which hw output
 * valid_row_num:        Number of valid row which hw output
 * time_stamp:	time when hw3a compute stats. unit: ms
 * fv[CAF_MAX_BLOCK_NUM]:		focus value;
 */
#pragma pack(push, 4)
struct lib_caf_stats_t {
	uint16 caf_token_id;
	uint16 frame_id;
	uint8  valid_column_num;
	uint8  valid_row_num;
	struct caf_time_stamp_t time_stamp;
	uint32 fv[CAF_MAX_BLOCK_NUM];
};

#pragma pack(push, 4)
struct lib_caf_stats_config_t {
	uint8		roi_left_ratio;
	uint8		roi_top_ratio;
	uint8		roi_width_ratio;
	uint8		roi_height_ratio;
	uint8		num_blk_hor;
	uint8		num_blk_ver;
};
#pragma pack(pop)

/*
 * API name: al3awrapperaf_getcafcfg
 * This API is used for query caf ISP config before calling al3awrapperaf_updateispconfig_af
 * param isp_config[out]: input buffer, API would manage parameter and return via this pointer
 * return: error code
 */
uint32 al3awrapper_caf_transform_cfg(struct lib_caf_stats_config_t *lib_config,struct alhw3a_af_cfginfo_t *isp_config);

/*
 * API name: al3awrapper_dispatch_caf_stats
 * This API used for patching HW3A stats from ISP(Altek) for CAF libs, after patching completed, AF ctrl should prepare patched
 * stats to AF libs
 * param isp_meta_data[In]: patched data after calling al3awrapper_dispatch_caf_stats, used for patch AF stats for CAF lib
 * param caf_ref_config[In]: CAF config for reference, used to limit valid banks
 * param alaf_stats[Out]: result patched AF stats
 * return: error code
 */
uint32 al3awrapper_dispatch_caf_stats(void *isp_meta_data, void *caf_ref_config, void *caf_stats );

/*
 * API name: al3awrapperaf_get_version
 * This API would return labeled version of wrapper
 * fWrapVersion[out], return current wapper version
 * return: error code
 */
uint32 al3awrappercaf_get_version(float *wrap_version);
