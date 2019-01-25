/********************************************************************************
 * allib_af.h
 *
 *  Created on: 2015/12/06
 *      Author: ZenoKuo
 *  Latest update: 2016/2/26
 *      Reviser: AllenWang
 *  Comments:
********************************************************************************/
#ifndef _ALAFLIB_H_
#define _ALAFLIB_H_

#include "mtype.h"

#define ALAF_EXIF_INFO_SIZE (2060)
#define ALAF_DEBUG_INFO_SIZE (15500)
#define ALAF_MAX_STATS_ROI_NUM (5)
#define MAX_STATS_COLUMN_NUM (16)
#define MAX_STATS_ROW_NUM (9)
#define ALAF_MAX_STATS_NUM (MAX_STATS_COLUMN_NUM*MAX_STATS_ROW_NUM)
#define ALAF_MAX_ZONE (9)
#define MAX_TRIGGER_DATA_SIZE (128)

//#define NEW_AF_ALGORITHM
#ifdef NEW_AF_ALGORITHM
/// ALTEK_MODIFIED >>>
#define ALAF_MAX_HIST_NUM (256)
/// ALTEK_MODIFIED <<<
#endif /* NEW_AF_ALGORITHM */

//#define IIR_PARAM_ENABLE

/*
 * allib_af_version_t
 *
 * m_uw_main_ver:	Main version
 * m_uw_sub_ver:	Sub version
 */
#pragma pack(push, 4)
struct allib_af_version_t {
	uint16 m_uw_main_ver;
	uint16 m_uw_sub_ver;
};
#pragma pack(pop)


/*
 * allib_af_time_stamp_t
 *
 * time_stamp_sec: time stamp second.
 * time_stamp_us: time stamp microsecond.
 */
#pragma pack(push, 4)
struct allib_af_time_stamp_t {
	uint32 time_stamp_sec;
	uint32 time_stamp_us;
};
#pragma pack(pop)

/*
 * allib_af_input_move_lens_cb_t
 *
 * uc_device_id: devices ID.
 * *move_lens_cb: callback function.
 * p_handle: 			handler from HAL, only have to do is input to callback function.
 *
 * lens working status. to info alAFLib.
 */
#pragma pack(push, 4)
struct allib_af_input_move_lens_cb_t {
	uint8 uc_device_id;
	void *p_handle;
	uint8 (*move_lens_cb)(void *p_handle, int16 w_vcm_dac, uint8 uc_device_id);
};
#pragma pack(pop)

/*
 * allib_af_lens_status
 *
 * LENS_STOP:				lens is in stop status.
 * LENS_MOVING:			lens is moving now.
 * LENS_MOVE_DONE:	lens is move done which change from MOVING.
 *
 * lens working status. to info alAFLib.
 */
#pragma pack(push, 4)
enum allib_af_lens_status {
	LENS_STOP = 1,
	LENS_MOVING,
	LENS_MOVE_DONE,
	LENS_MAX
};
#pragma pack(pop)

/*
 * allib_af_input_lens_info_t
 *
 * lens_pos_updated:true, if lens position is update.
 * lens_pos:current position
 * lens_status:current lens status
 * manual_lens_pos_dac:	input lens vcm dac value to alAFLib
 *
 * set image buffer to alAFLib. the sub-cam buf only avalible when sub-cam supported.
 */
#pragma pack(push, 4)
struct allib_af_input_lens_info_t {
	uint8 lens_pos_updated;
	uint16 lens_pos;
	enum allib_af_lens_status lens_status;
	int16 manual_lens_pos_dac;
};
#pragma pack(pop)

/*
 * allib_af_input_sof_id_t
 *
 * sof_time:			system time when updating sof id.
 * sof_frame_id:	sof frame id.
 *
 * send sof id number and system time at sof updating moment.
 */
#pragma pack(push, 4)
struct allib_af_input_sof_id_t {

	struct allib_af_time_stamp_t	sof_time;
	uint32			sof_frame_id;

};
#pragma pack(pop)

/*
 * allib_af_input_image_buf_t
 *
 * frame_ready:	true if img is ready.
 * img_time:	system time when DL image.
 * p_main_cam:	pointer to image buf from main-cam
 * p_sub_cam:	pointer to image buf from sub-cam, set to null if sub-cam not support.
 *
 * set image buffer to alAFLib. the sub-cam buf only avalible when sub-cam supported.
 */
#pragma pack(push, 4)
struct allib_af_input_image_buf_t {
	int32 			frame_ready;
	struct allib_af_time_stamp_t	img_time;
	uint8 			*p_main_cam;
	uint8 			*p_sub_cam;
};
#pragma pack(pop)

/*
 * allib_af_input_from_calib_t
 *
 * inf_step:	VCM inf step
 * macro_step:	VCM macro step
 *
 * inf_distance:	VCM calibration inf distance in mm.
 * macro_distance:	VCM calibration macro distance in mm.
 *
 * mech_top:	mechanical limitation top position (macro side) in step.
 * mech_bottom:	mechanical limitation bottom position(inf side) in step.
 * lens_move_stable_time: time consume of lens from moving to stable in ms.
 *
 * extend_calib_ptr:	extra lib calibraion data, if not support, set to null.
 * extend_calib_data_size: size of extra lib calibration data, if not support set to zero.
 *
 * data from OTP must input to alAFLib
 */
#pragma pack(push, 4)
struct allib_af_input_from_calib_t {
	int16 inf_step;
	int16 macro_step;
	int32 inf_distance;
	int32 macro_distance;
	int16 mech_top;
	int16 mech_bottom;
	uint8 lens_move_stable_time;
	void *extend_calib_ptr;
	int32 extend_calib_data_size;
};
#pragma pack(pop)

/*
 * allib_af_input_module_info_t
 *
 * f_number:	f-number, ex. f2.0, then input 2.0
 * focal_lenth:	focal length in um
 *
 * the data should settled before initialing.
 */
#pragma pack(push, 4)
struct allib_af_input_module_info_t {
	float f_number;
	float focal_lenth;
	int32 cam_layout_type;
};
#pragma pack(pop)

/*
 * allib_af_input_init_info_t
 *
 * module_info:	info of module spec
 * calib_data:		calibration data
 *
 * the data should settled before initialing.
 */
#pragma pack(push, 4)
struct allib_af_input_init_info_t {
	struct allib_af_input_module_info_t	module_info;
	struct allib_af_input_from_calib_t	calib_data;
};
#pragma pack(pop)

/* HAF third-party calib-param */
/* if there is external HAF lib */

#pragma pack(push, 4)
struct allib_af_input_initial_set_t {
	void*	p_initial_set_data;
};
#pragma pack(pop)

#pragma pack(push, 4)
struct allib_af_input_update_bin_t {
	void*	p_bin_data;
};
#pragma pack(pop)
/*
 * allib_af_hybrid_type
 *
 * alAFLIB_HYBRID_TYPE_PD:	Phase detection.
 *
 * alAFLib support hybrid af type
 */

#pragma pack(push, 4)
enum allib_af_hybrid_type {
	alAFLIB_HYBRID_TYPE_PD = 1,
	alAFLIB_HYBRID_TYPE_LASER = 1 << 1,
	alAFLIB_HYBRID_TYPE_IAF = 1 << 2,
	alAFLIB_HYBRID_TYPE_MAX
};
#pragma pack(pop)

/*
 * allib_af_input_enable_hybrid_t
 * enable_hybrid:	enable hybrid AF
 * type:		hybrid af type.
 *
 * haf setting data, if support.
 */
#pragma pack(push, 4)
struct allib_af_input_enable_hybrid_t {
	uint8	enable_hybrid;
	enum allib_af_hybrid_type	type;
	struct allib_af_version_t  pd_lib_version;
};
#pragma pack(pop)

/*
 * allib_af_get_data_info_t
 * addr:	data address.
 * size:	data size.
 *
 */
#pragma pack(push, 4)
struct allib_af_get_data_info_t {
	void*	addr;
	uint32	size;
};
#pragma pack(pop)
/*
 * allib_af_get_pd_info_t
 * pd_range:	pd confidence.
 *
 */
#pragma pack(push, 4)
struct allib_af_get_pd_info_t {
	uint8	update_pd_info;
	uint32	pd_range;

};
#pragma pack(pop)
/*
 * allib_af_input_focus_mode_type
 *
 * input af mode which should be set from HAL control.
 * alAFLib_AF_MODE_OFF 	: 1 AF turn off
 * alAFLib_AF_MODE_AUTO 	: 2 Auto mode
 * alAFLib_AF_MODE_MACRO   : 3 Macro mode
 * alAFLib_AF_MODE_CONTINUOUS_VIDEO   : 4 continues video mode
 * alAFLib_AF_MODE_CONTINUOUS_PICTURE : 5 continues liveview picture.
 * alAFLib_AF_MODE_EDOF,              : 6 set focus lens position by HAL
 * alAFLib_AF_MODE_MANUAL,            : 7 manual mode.
 * alAFLib_AF_MODE_HYBRID_AUTO,       : 8 hybrid af auto mode, if haf qualify
 * alAFLib_AF_MODE_HYBRID_CONTINUOUS_VIDEO   : 9 hybrid af continues video mode, if haf qualify
 * alAFLib_AF_MODE_HYBRID_CONTINUOUS_PICTURE : 10 hybrid af continues liveview picture mode, if haf qualify
 * alAFLib_AF_MODE_TUNING,                   : 11 Special mode for tuning.
 * alAFLib_AF_MODE_NOT_SUPPORTED,            : 12 not support.
 * alAFLib_AF_MODE_MAX
 */
#pragma pack(push, 4)
enum allib_af_input_focus_mode_type {
	alAFLib_AF_MODE_OFF = 1,
	alAFLib_AF_MODE_AUTO,
	alAFLib_AF_MODE_MACRO,
	alAFLib_AF_MODE_CONTINUOUS_VIDEO,
	alAFLib_AF_MODE_CONTINUOUS_PICTURE,
	alAFLib_AF_MODE_EDOF,
	alAFLib_AF_MODE_MANUAL,
	alAFLib_AF_MODE_HYBRID_AUTO,
	alAFLib_AF_MODE_HYBRID_CONTINUOUS_VIDEO,
	alAFLib_AF_MODE_HYBRID_CONTINUOUS_PICTURE,
	alAFLib_AF_MODE_TUNING,
	alAFLib_AF_MODE_NOT_SUPPORTED,
	alAFLib_AF_MODE_MAX
};
#pragma pack(pop)

/*
 * allib_af_roi_type
 *
 * alAFLib_ROI_TYPE_NORMAL:	ROI info for continuous AF.
 * alAFLib_ROI_TYPE_TOUCH:		ROI info for touch AF.
 * alAFLib_ROI_TYPE_FACE:		Input face detected roi info
 *
 * alAFLib support roi type
 */
#pragma pack(push, 4)
enum allib_af_roi_type {
	alAFLib_ROI_TYPE_NORMAL = 0,
	alAFLib_ROI_TYPE_TOUCH,
	alAFLib_ROI_TYPE_FACE,
};
#pragma pack(pop)

/*
 * allib_af_roi_t
 *
 * uw_top:	top-left x position
 * uw_left:	top-left y position
 * uw_dx:	crop width
 * uw_dy:	crop height
 *
 * alAFLib roi info.
 */
#pragma pack(push, 4)
struct allib_af_roi_t {
	uint16 uw_top;
	uint16 uw_left;
	uint16 uw_dx;
	uint16 uw_dy;
};
#pragma pack(pop)

/*
 * allib_af_img_t
 *
 * uw_width:	img width
 * uw_height:	img height
 *
 * image info structure.
 */
#pragma pack(push, 4)
struct allib_af_img_t {
	uint16 uw_width;
	uint16 uw_height;
};
#pragma pack(pop)

/*
 * allib_af_crop_t
 *
 * uwx:	top-left x position
 * uwy:	top-left y position
 * dx:	crop width
 * dy:	crop height
 *
 * img crop info struct.
 */
#pragma pack(push, 4)
struct allib_af_crop_t {
	uint16 uwx;
	uint16 uwy;
	uint16 dx;
	uint16 dy;
};
#pragma pack(pop)

/*
 * allib_af_input_roi_info_t
 *
 * roi_updated:	does roi update
 * type:				input roi type, refer to al_af_lib_roi_type
 * frame_id:		frame id correspond to current roi
 * num_roi:			number of roi are qualify, max number is ALAF_MAX_STATS_ROI_NUM.
 * roi:					array of roi info, refer to type alAFLib_roi_t
 * weight:			weighting for each roi.
 * src_img_sz:	the source image which is refered by current input roi.
 *
 * roi info
 */
#pragma pack(push, 4)
struct allib_af_input_roi_info_t {
	uint8            roi_updated;
	enum allib_af_roi_type	type;
	uint32          frame_id;
	uint32          num_roi;
	struct allib_af_roi_t   roi[ALAF_MAX_STATS_ROI_NUM];
	uint32          weight[ALAF_MAX_STATS_ROI_NUM];
	struct allib_af_img_t	src_img_sz;
};
#pragma pack(pop)

/*
 * allib_af_input_sensor_info_t
 *
 * preview_img_sz: 	size of live view image
 * sensor_crop_info:sensor crop image size info.
 * actuator_info: 	info from actuator, refer to alAFLib_actuator_info_t
 * sensor info
 */
#pragma pack(push, 4)
struct allib_af_input_sensor_info_t {
	struct allib_af_img_t preview_img_sz;
	struct allib_af_crop_t sensor_crop_info;
};
#pragma pack(pop)

/*
 * allib_af_input_isp_info_t
 *
 * liveview_img_sz: size of live view image
 *
 * isp info
 */
#pragma pack(push, 4)
struct allib_af_input_isp_info_t {
	struct allib_af_img_t liveview_img_sz;
};
#pragma pack(pop)

/*
 * allib_af_input_aec_info_t
 *
 * ae_settled:	true for ae report converged.
 * cur_intensity: current intensity.
 * target_intensity: target intensity.
 * brightness: 	current brightness.
 * cur_gain: 	gain.
 * exp_time: 	exposure time.
 * preview_fr:preview frame rate in fps.
 */
#pragma pack(push, 4)
struct allib_af_input_aec_info_t {
	uint8 ae_settled;
	float cur_intensity;
	float target_intensity;
	int16	brightness;
	uint32 avg_intensity;
	uint32 center_intensity;
	float cur_gain;
	float exp_time;
	int32 preview_fr;
};
#pragma pack(pop)

/*
 * allib_af_input_awb_info_t
 *
 * awb_ready:	true for awb report ready.
 * p_awb_report:	pointer to awb report, if not support, point to null.
 *
 * pointer awb report which is from awb library.
 */
#pragma pack(push, 4)
struct allib_af_input_awb_info_t {
	uint8 awb_ready;
	void *p_awb_report;
};
#pragma pack(pop)

/*
 * allib_af_input_gyro_info_t
 *
 * gyro_enable:		true for gyro available.
 * gyro_ready:		true for gyro set data ready.
 * gyro_value[3]: 	gyro value for different direction.
 *
 * Gyro data
 */
#pragma pack(push, 4)
struct allib_af_input_gyro_info_t {
	uint8 gyro_enable;
	uint8 gyro_ready;
	int32 gyro_value[3];
};
#pragma pack(pop)

/*
 * allib_af_input_gravity_vector_t
 *
 * gravity_enable:	true for gravity available.
 * gravity_ready:	true for gravity set data ready.
 * g_vector[3]:		gravity value.
 *
 * Gravity data
 */
#pragma pack(push, 4)
struct allib_af_input_gravity_vector_t {
	uint8	gravity_enable;
	uint8 	gravity_ready;
	float g_vector[3];
};
#pragma pack(pop)

#ifdef NEW_AF_ALGORITHM
/// ALTEK_MODIFIED >>>
#pragma pack(push, 4)
enum allib_af_process_type {
	alAFLIB_PROCESS_UNKNOWN,
	alAFLIB_PROCESS_FV,
	alAFLIB_PROCESS_YHIST,
	alAFLIB_PROCESS_MAX
};
#pragma pack(pop)

#pragma pack(push, 4)
struct allib_af_hw_fv_t {
	uint16 af_token_id;
	uint16 curr_frame_id;
	uint32 hw3a_frame_id;
	uint8  valid_column_num;
	uint8  valid_row_num;
	struct allib_af_time_stamp_t time_stamp;
	uint32 fv_hor[ALAF_MAX_STATS_NUM];
	uint32 fv_ver[ALAF_MAX_STATS_NUM];
	uint64 filter_value1[ALAF_MAX_STATS_NUM];
	uint64 filter_value2[ALAF_MAX_STATS_NUM];
	uint64 y_factor[ALAF_MAX_STATS_NUM];
	uint32 cnt_hor[ALAF_MAX_STATS_NUM];
	uint32 cnt_ver[ALAF_MAX_STATS_NUM];
};
#pragma pack(pop)

#pragma pack(push, 4)
struct allib_af_yhist_t {
	uint32 ustructuresize;		/* here for confirmation */
	/* Common info */
	uint32 umagicnum;
	uint16 uhwengineid;
	uint16 uframeidx;			/* HW3a_frame_idx */
	/* yhist info */
	uint16 u_yhist_tokenid;
	uint32 u_yhist_statssize;
	/* framework time/frame idx info */
	struct allib_af_time_stamp_t systemtime;
	uint32 udsys_sof_idx;
	/* yhist stats */
	uint8  b_is_stats_byaddr;			/* true: use addr to passing stats, flase: use array define */
	uint32 hist_y[ALAF_MAX_HIST_NUM];	/* Y histogram accumulate pixel number */
	void* pt_hist_y;					/* store stats Y, each element should be uint32, 256 elements */
};
#pragma pack(pop)  /* restore old alignment setting from stack */
/*
 * the definition is depends on isp (hw3a), please confirm the value defined if isp changed
 * ALAF_MAX_STATS_NUM:	corresponds to hw3a definition (AL_MAX_AF_STATS_NUM)
 * ALAF_MAX_ZONE:		coresp. to (A3CTRL_A_AF_MAX_BLOCKS)
 */

/*
 * allib_af_hw_stats_t
 * af_token_id:	valid setting number, which same when doing AF configuration, for 3A libs synchronization
 * curr_frame_id:	frame id when send
 * hw3a_frame_id:	frame id when hw3a compute stats.
 * valid_column_num:   Number of valid column which hw output
 * valid_row_num:        Number of valid row which hw output
 * time_stamp:	time when hw3a compute stats. unit: ms
 * fv_hor[ALAF_MAX_STATS_NUM]:		focus value horizontal;
 * fv_ver[ALAF_MAX_STATS_NUM]:		focus value vertical;
 * y_factor[ALAF_MAX_STATS_NUM]:		Arrays with the sums for the intensity.
 * cnt_hor[ALAF_MAX_STATS_NUM]:		Counts for the horizontal focus value above the threshold
 * cnt_ver[ALAF_MAX_STATS_NUM]:		Counts for the vertical focus value above the threshold
 */
#pragma pack(push, 4)
struct allib_af_hw_stats_t {
	enum allib_af_process_type type;
	union {
		struct allib_af_hw_fv_t hw_fv;
		struct allib_af_yhist_t y_hist;
	};
};
#pragma pack(pop)
/// ALTEK_MODIFIED <<<
#else
#pragma pack(push, 4)
struct allib_af_hw_stats_t {
	uint16 af_token_id;
	uint16 curr_frame_id;
	uint32 hw3a_frame_id;
	uint8  valid_column_num;
	uint8  valid_row_num;
	struct allib_af_time_stamp_t time_stamp;
	uint32 fv_hor[ALAF_MAX_STATS_NUM];
	uint32 fv_ver[ALAF_MAX_STATS_NUM];
	uint64 filter_value1[ALAF_MAX_STATS_NUM];
	uint64 filter_value2[ALAF_MAX_STATS_NUM];
	uint64 y_factor[ALAF_MAX_STATS_NUM];
	uint32 cnt_hor[ALAF_MAX_STATS_NUM];
	uint32 cnt_ver[ALAF_MAX_STATS_NUM];
};
#pragma pack(pop)
#endif /* NEW_AF_ALGORITHM */
/*
 * allib_af_input_hwaf_info_t
 * hw_stats_ready:        af_stats data from hw is ready
 * max_num_stats:         setting param from hw defined, corrsp. to ALAF_MAX_STATS_NUM
 * max_num_zone:		setting param from hw defined, corrsp. to A3CTRL_A_AF_MAX_BLOCKS
 * max_num_banks:         setting param from hw defined, corrsp. to A3CTRL_A_AF_MAX_BANKS
 * hw_stats:		hw3a stats, refer to alAFLib_hw_stats_t.
 */
#pragma pack(push, 4)
struct allib_af_input_hwaf_info_t {
	uint8 hw_stats_ready;
	uint16 max_num_stats;
	uint8 max_num_zone;
	uint8 max_num_banks;
	struct allib_af_hw_stats_t hw_stats;
};
#pragma pack(pop)

/*
 * allib_af_input_altune_t
 *
 * cbaf_tuning_enable:	enable cabf tuning, if necessary.
 * scdet_tuning_enable: 	enable cabf tuning, if necessary.
 * haf_tuning_enable:	enable cabf tuning, if necessary.
 *
 * p_cbaf_tuning_ptr:	linke to cbaf tuning table header, ptr to cbaf tuning table, if enable.
 * p_scdet_tuning_ptr:	link to scdet tuning table header, ptr to scdet tuning table, if enable.
 * p_haf_tuning_ptr:	need to include extend tuning table from provider, ptr to extend haf lib tuning table, if enable.
 *
 * To set alAF tuning data in.
 */
#pragma pack(push, 4)
struct allib_af_input_altune_t {
	uint8 cbaf_tuning_enable;
	uint8 scdet_tuning_enable;
	uint8 haf_tuning_enable;
	void *p_cbaf_tuning_ptr;
	void *p_scdet_tuning_ptr;
	void *p_haf_tuning_ptr;
};
#pragma pack(pop)

/*
 * allib_af_input_special_event_type
 *
 * alAFLib_ROI_READ_DONE:	special event type, reference to alAFLib_input_special_event_type
 * alAFLib_AE_IS_LOCK:	true for word meaning.
 * alAFLib_AF_STATS_CONFIG_UPDATED: After token ID of the input AF stats was the same as true for word meaning.
 */
#pragma pack(push, 4)
enum allib_af_input_special_event_type {
	alAFLib_ROI_READ_DONE,
	alAFLib_AE_IS_LOCK,
	alAFLib_AF_STATS_CONFIG_UPDATED,
	alAFLib_SPECIAL_MAX
};
#pragma pack(pop)

/*
 * allib_af_input_special_event
 *
 * sys_time:	input system time when set event.
 * type:		special event type, reference to alAFLib_input_special_event_type
 * flag:		true for word meaning.
 */
#pragma pack(push, 4)
struct allib_af_input_special_event {
	struct allib_af_time_stamp_t	sys_time;
	enum allib_af_input_special_event_type type;
	uint8 flag;
};
#pragma pack(pop)

/*
 * allib_af_pd_stats_t
 * defocus:	defocus
 * conf_value:	confidence
 * pd_value:	phase different
 */
#pragma pack(push, 4)
struct allib_af_pd_stats_t {
	int32 defocus;
	uint32 conf_value;
	float pd_value;
};
#pragma pack(pop)

/*
 * allib_af_input_pd_info_t
 * token_id:	valid setting number, which same when doing AF configuration, for 3A libs synchronization
 * frame_id:	frame id when send
 * enable:	is PD enable?.
 * time_stamp:	time when hw3a compute stats. unit: ms
 * pd_stats[ALAF_MAX_STATS_NUM]: PD Statistics data;
 * roi:		PD use roi info of extend data.
 * src_img_sz:	roi reference PD source img size of extend data.
 * extend_data_ptr:	extra lib data, if not support, set to null.
 * extend_data_size: size of extra lib data, if not support set to zero.
 */

#pragma pack(push, 4)
struct allib_af_input_pd_info_t {
	uint16 token_id;
	uint32 frame_id;
	uint8 enable;
	struct allib_af_time_stamp_t time_stamp;
	void *extend_data_ptr;
	uint32 extend_data_size;
};
#pragma pack(pop)

#pragma pack(push, 4)
struct allib_af_input_tirgger_info_t {
	uint32 frame_id;
	int8 ac_trigger_data[MAX_TRIGGER_DATA_SIZE];
};
#pragma pack(pop)

#pragma pack(push, 4)
struct allib_af_eeprom_data_t {
	uint8 *buff;
	uint32 num_bytes;
};
#pragma pack(pop)

#pragma pack(push, 4)
struct allib_af_dual_cam_share_info_t {
	void *af_status;
	int32 (*get_vcm_step)(void *af_status);
	int32 (*get_af_requirement)(void *af_status, int32 type, uint32 data);
};
#pragma pack(pop)

#pragma pack(push, 4)
struct allib_af_input_tube_info_t {
	struct allib_af_eeprom_data_t eeprom_data;
	int32 current_session;
	int32 main_session_id;
	int32 sub_session_id;
	struct allib_af_dual_cam_share_info_t share_info;
};
#pragma pack(pop)

#pragma pack(push, 4)
struct allib_af_stream_crop_t {
	uint32 ud_x;
	uint32 ud_y;
	uint32 ud_dx;
	uint32 ud_dy;
};
#pragma pack(pop)

/*
 * allib_af_set_param_type
 * the type of which parameter setin to alAFLib.
 * alAFLIB_SET_PARAM_UNKNOWN,		// 0 //         No type input
 * alAFLIB_SET_PARAM_SET_CALIBDATA,	// 1 //         Set OTP, calibration info and module info from hardware
 * alAFLIB_SET_PARAM_SET_SETTING_FILE,	// 2 //         send initial af library setting data
 * alAFLIB_SET_PARAM_INIT,			// 3 //         Initial alAFLib when open camera, allocate af thread data
 * alAFLIB_SET_PARAM_AF_START,		// 4 //         Except CAF, start AF scan process once. TAF
 * alAFLIB_SET_PARAM_CANCEL_FOCUS,		// 5 //         Cancel current AF process. Interrupt focusing
 * alAFLIB_SET_PARAM_FOCUS_MODE,		// 6 //         Set AF focus mode
 * alAFLIB_SET_PARAM_UPDATE_LENS_INFO,	// 7 //        when lens status change, update lens info to inform alAFLib.
 * alAFLIB_SET_PARAM_RESET_LENS_POS,	// 8 //         reset lens position to default.
 * alAFLIB_SET_PARAM_SET_LENS_MOVE_CB,	// 9 //         Set Lens move callback function when initialization
 * alAFLIB_SET_PARAM_UPDATE_MANUAL_POS_DAC,// 10 //         update dac value of manual focus. ori: alAFLIB_SET_PARAM_SET_MANUAL_FOCUS_DIST,
 * alAFLIB_SET_PARAM_SET_ROI,		// 11 //         Set Region of interest info, e.q. face roi, Top- Left x,y position and ROI's width Height
 * alAFLIB_SET_PARAM_TUNING_ENABLE,	// 12 //         Enable Tuning, the tuning parameter is send into by tuning header alAFLIB_SET_PARAM_UPDATA_TUNING_PTR
 * alAFLIB_SET_PARAM_UPDATE_TUNING_PTR,	// 13 //         TODO, Update tuning header when enable tuning
 * alAFLIB_SET_PARAM_UPDATE_AEC_INFO,	// 14 //         Update AEC info to alAFLib
 * alAFLIB_SET_PARAM_UPDATE_AWB_INFO,	// 15 //         Update AWB indo to alAFLib
 * alAFLIB_SET_PARAM_UPDATE_SENSOR_INFO,	// 16 //         Update sensor info.
 * alAFLIB_SET_PARAM_UPDATE_GYRO_INFO,	// 17 //         Update Gyro Info.
 * alAFLIB_SET_PARAM_UPDATE_GRAVITY_VECTOR,// 18 //         Update gravity vector, if gravity is avaliable
 * alAFLIB_SET_PARAM_UPDATE_ISP_INFO,	// 19 //         Update ISP Info
 * alAFLIB_SET_PARAM_UPDATE_SOF,		// 20 //         Update Start of frame(SOF) number, if new image comes
 * alAFLIB_SET_PARAM_WAIT_FOR_FLASH,	// 21 //         Inform alAFLib, it is waiting AE converge
 * alAFLIB_SET_PARAM_LOCK_CAF,		// 22 //         To Luck CAF process, when capture, mode change, and so on.
 * alAFLIB_SET_PARAM_RESET_CAF,		// 23 //         To reset CAF
 * alAFLIB_SET_PARAM_HYBIRD_AF_ENABLE,	// 24 //         Enable Hybrid AF
 * alAFLIB_SET_PARAM_SET_IMGBUF,		// 25 //         Set Image Buffer into alAFLib
 * alAFLIB_SET_PARAM_UPDATE_BIN,	// 26 //        Set alAFLib to default setting. The current setting will be remove, and no longer to recover until restart camera and load such setting file.
 * alAFLIB_SET_PARAM_SPECIAL_EVENT,	// 27 //        To inform alAFLib if event change status. Event Type please reference to alAFLib_input_special_event_type
 * alAFLIB_SET_PARAM_UPDATE_PD_INFO      // 28 //        Update PD Info.
 * alAFLIB_SET_PARAM_MAX			// 29 //	0xff
 */
#pragma pack(push, 4)
enum allib_af_set_param_type {
	alAFLIB_SET_PARAM_UNKNOWN,
	alAFLIB_SET_PARAM_SET_CALIBDATA,
	alAFLIB_SET_PARAM_SET_SETTING_FILE,
	alAFLIB_SET_PARAM_INIT,
	alAFLIB_SET_PARAM_AF_START,
	alAFLIB_SET_PARAM_CANCEL_FOCUS,
	alAFLIB_SET_PARAM_FOCUS_MODE,
	alAFLIB_SET_PARAM_UPDATE_LENS_INFO,
	alAFLIB_SET_PARAM_RESET_LENS_POS,
	alAFLIB_SET_PARAM_SET_LENS_MOVE_CB,
	alAFLIB_SET_PARAM_UPDATE_MANUAL_POS_DAC,
	alAFLIB_SET_PARAM_SET_ROI,
	alAFLIB_SET_PARAM_TUNING_ENABLE,
	alAFLIB_SET_PARAM_UPDATE_TUNING_PTR,
	alAFLIB_SET_PARAM_UPDATE_AEC_INFO,
	alAFLIB_SET_PARAM_UPDATE_AWB_INFO,
	alAFLIB_SET_PARAM_UPDATE_SENSOR_INFO,
	alAFLIB_SET_PARAM_UPDATE_GYRO_INFO,
	alAFLIB_SET_PARAM_UPDATE_GRAVITY_VECTOR,
	alAFLIB_SET_PARAM_UPDATE_ISP_INFO,
	alAFLIB_SET_PARAM_UPDATE_SOF,
	alAFLIB_SET_PARAM_WAIT_FOR_FLASH,
	alAFLIB_SET_PARAM_LOCK_CAF,
	alAFLIB_SET_PARAM_RESET_CAF,
	alAFLIB_SET_PARAM_HYBIRD_AF_ENABLE,
	alAFLIB_SET_PARAM_SET_IMGBUF,
	alAFLIB_SET_PARAM_UPDATE_BIN,
	alAFLIB_SET_PARAM_SPECIAL_EVENT,
	alAFLIB_SET_PARAM_UPDATE_PD_INFO,
	alAFLIB_SET_PARAM_TRIG_INFO,
	alAFLIB_SET_PARAM_TUBE_CONFIG,
	alAFLIB_SET_PARAM_UPDATE_STREAM_CROP_INFO,
	alAFLIB_SET_PARAM_MAX
};
#pragma pack(pop)

/*
 * allib_af_input_set_param_t
 * To set info to alAFLib, refernce to type
 *
 * type:	set_param type to which you are going to setin alAFLib, refer to alAFLib_set_param_type
 * current_sof_id:	frame id from sof;
 * union{
 *	init_info:
 *	init_set:
 *	afctrl_initialized:	is af_ctrl laer initialized.
 *	focus_mode_type:	al_af_lib_input_focus_mode_type
 *	lens_info:
 *	move_lens:
 *	roi_info:
 *	sensor_info:
 *	isp_info:
 *	aec_info:		Need double check.
 *	awb_info:
 *	gyro_info:
 *	gravity_info:		g-sensor info
 *	al_af_tuning
 *	al_af_tuning_enable
 *	sof_id:
 *	lock_caf:
 *	wait_for_flash:
 *	special_event:
 *	haf_info:
 *	img_buf:
 *}u_set_data;
 */
#pragma pack(push, 4)
struct allib_af_input_set_param_t {
	enum allib_af_set_param_type type;
	uint32 current_sof_id;
	union {
		struct allib_af_input_init_info_t       init_info;
		struct allib_af_input_initial_set_t     init_set;
		uint8                            afctrl_initialized;
		int32                           focus_mode_type;
		struct allib_af_input_lens_info_t       lens_info;
		struct allib_af_input_move_lens_cb_t    move_lens;
		struct allib_af_input_roi_info_t        roi_info;
		struct allib_af_input_sensor_info_t     sensor_info;
		struct allib_af_input_isp_info_t        isp_info;
		struct allib_af_input_aec_info_t        aec_info;
		struct allib_af_input_awb_info_t        awb_info;
		struct allib_af_input_gyro_info_t       gyro_info;
		struct allib_af_input_gravity_vector_t  gravity_info;
		struct allib_af_input_altune_t          al_af_tuning;
		uint8                            al_af_tuning_enable;
		struct allib_af_input_sof_id_t             sof_id;
		uint8                            lock_caf;
		uint8                            wait_for_flash;
		struct allib_af_input_special_event   special_event;
		struct allib_af_input_enable_hybrid_t   haf_info;
		struct allib_af_input_image_buf_t	img_buf;
		struct allib_af_input_pd_info_t pd_info;
		struct allib_af_input_update_bin_t update_bin;
		struct allib_af_input_tirgger_info_t trig_info;
		struct allib_af_input_tube_info_t         tube_info;
		struct allib_af_stream_crop_t             stream_crop_info;
	} u_set_data;
};
#pragma pack(pop)

/*
 * allib_af_get_param_type
 *
 * alAFLIB_GET_PARAM_FOCUS_MODE:		get current focus mode
 * alAFLIB_GET_PARAM_DEFAULT_LENS_POS:	get default lens position (step)
 * alAFLIB_GET_PARAM_GET_CUR_LENS_POS:	get current lens position (step)
 * alAFLIB_GET_PARAM_DEBUG_INFO:		get Debug info memory address size
 * alAFLIB_GET_PARAM_EXIF_INFO:			get Exif info memory address size
 * alAFLIB_GET_PARAM_NOTHING:			set it if you don't need to get info from alAFlib this time.
 *
 * info type you want to get from alAFLib.
 */
#pragma pack(push, 4)
enum allib_af_get_param_type {
	alAFLIB_GET_PARAM_FOCUS_MODE = 0,
	alAFLIB_GET_PARAM_DEFAULT_LENS_POS,
	alAFLIB_GET_PARAM_GET_CUR_LENS_POS,
	alAFLIB_GET_PARAM_DEBUG_INFO,
	alAFLIB_GET_PARAM_EXIF_INFO,
	alAFLIB_GET_PARAM_NOTHING,
	alAFLIB_GET_PARAM_PD_INFO,
	alAFLIB_GET_PARAM_MAX
};
#pragma pack(pop)

/*
 * allib_af_input_get_param_t
 *
 * type:			refer to alAFLib_get_param_type.
 * sof_frame_id
 * uw_default_lens_pos:	current lens pos (step).
 * af_focus_mode:		current setted focused mode.
 * u_wdefault_lens_pos: 	default lens pos (step)
 * To inform upper layer (ex. af_wrapper) current AF status.
 * the type is get by get_param.
 */
#pragma pack(push, 4)
struct allib_af_input_get_param_t {
	enum allib_af_get_param_type type;
	uint32 sof_frame_id;
	union {
		enum allib_af_input_focus_mode_type	af_focus_mode;
		int16				w_current_lens_pos;
		uint16				uw_default_lens_pos;
		struct allib_af_get_data_info_t debug_data_info;
		struct allib_af_get_data_info_t exif_data_info;
		struct allib_af_get_pd_info_t pd_data_info;
	} u_get_data;
};
#pragma pack(pop)

/*
 * allib_af_status_type
 *
 * alAFLib_STATUS_INVALID :	alAF lib invalid.
 * alAFLib_STATUS_INIT	:	alAF lib finished initialization, ready for waitinf trigger.
 * alAFLib_STATUS_FOCUSED :	alAF lib finished one round focus process.
 * alAFLib_STATUS_UNKNOWN :	alAF lib is know in unknown status. AF is not busy.
 * alAFLib_STATUS_WARNING : alAF lib wrans result that may not focus. UI should show red box.
 * alAFLib_STATUS_AF_ABORT: alAF lib is aborted by self-protection
 * alAFLib_STATUS_FORCE_ABORT :alAF lib is aborted by AP cmd cancel focus
 * alAFLib_STATUS_FOCUSING:	alAF lib is busy know and doing focus process., the aec should be locked this time.
 *
 * To inform upper layer (ex. af_wrapper) current AF status.
 * the type is get by get_param.
 */
#pragma pack(push, 4)
enum allib_af_status_type {
	alAFLib_STATUS_INVALID = -1,
	alAFLib_STATUS_INIT = 0,
	alAFLib_STATUS_FOCUSED,

	/*	alAFLib_STATUS_UNKNOWN,*/
	alAFLib_STATUS_WARNING,
	alAFLib_STATUS_AF_ABORT,
	alAFLib_STATUS_FORCE_ABORT,
	alAFLib_STATUS_FOCUSING,
	alAFLib_STATUS_PD_SEARCH_START,
	alAFLib_STATUS_PD_SEARCH_DONE,
	alAFLib_STATUS_PD_SEARCH_STOP
};
#pragma pack(pop)

/*
 * allib_af_out_status_t
 * focus_done:	alAFLib focus process done.
 * t_status:	refer to alAFLib_status_type.
 * f_distance:	focus distance in mm this AF round.
 *
 * Some information of alAFLib, when receive trigger.
 */
#pragma pack(push, 4)
struct allib_af_out_status_t {
	uint8			focus_done;
	enum allib_af_status_type	t_status;
	float			f_distance;
};
#pragma pack(pop)

/*
 * typedef Median_filter_mode
 * brief med mode
 */
#pragma pack(push, 4)
enum allib_af_med_filter_mode {
	MODE_51,
	MODE_31,
	DISABLE,
};
#pragma pack(pop)

/*
 * @typedef allib_af_lpf_t
 * @brief AF LPF config info
 */
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct allib_af_lpf_t {
	uint32 udb0;
	uint32 udb1;
	uint32 udb2;
	uint32 udb3;
	uint32 udb4;
	uint32 udb5;
	uint32 udb6;
	uint32 udshift;
};
#pragma pack(pop)  /* restore old alignment setting from stack */

/*
 * @typedef allib_af_iir_t
 * @brief AF IIR config info
 */
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct allib_af_iir_t {
	uint8   binitbyuser;
	uint32 udp;
	uint32 udq;
	uint32 udr;
	uint32 uds;
	uint32 udt;
	uint32 udabsshift;
	uint32 udth;
	uint32 udinita;
	uint32 udinitb;
};
#pragma pack(pop)  /* restore old alignment setting from stack */

/*
 * @typedef allib_af_pseudoy_t
 * @brief AF PseudoY config info
 */
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct allib_af_pseudoy_t {
	uint32 udwr;
	uint32 udwgr;
	uint32 udwgb;
	uint32 udwb;
	uint32 udshift;
	uint32 udoffset;
	uint32 udgain;
};
#pragma pack(pop)  /* restore old alignment setting from stack */

/*
 * allib_af_out_stats_config_t
 * token_id
 * roi:		AF use roi info.
 * src_img_sz:	roi reference source img size, which is received from upper layer.
 * num_blk_ver:	vertical block number.
 * num_blk_hor:	horizontal block number.
 * b_enable_af_lut:	enable flag for tone mapping, set false would disable auwAFLUT
 * auw_lut:	tone mapping for common 3A, no longer used (default bypassing)
 * auw_af_lut:	tone mapping for AF only
 * auc_weight:	weight values when calculate the frequency.
 * uw_sh:		bit for shift
 * uc_th_mode:	configure 2 modes. Mode 1: First 82 blocks refer to Th, Tv from thelookup array. Mode 2: Every 4 blocks reference one element.
 * auc_index:	locate index in an 82 elements array and reference one of 4 stages Th, Tv.
 * auw_th:		thresholds for calculate the horizontal contrast.
 * pw_tv:		thresholds for calculate the vertical contrast.
 * ud_af_offset:	input data offset
 * b_af_py_enable:	turn pseudo Y on or off
 * b_af_lpf_enable:turn low pass filter on or off
 * n_filter_mode:	median filter mode, suggest use DISABLE
 * uc_filter_id:	median filter device ID, for AF, use 1
 * uw_line_cnt:	AF timing control anchor point
 *
 * feadback AF current use roi info and block number.
 */
#pragma pack(push, 4)
struct allib_af_out_stats_config_t {
	uint16		token_id;
	enum allib_af_roi_type	roi_type;
	struct allib_af_roi_t	roi;
	struct allib_af_img_t	src_img_sz;
	uint8		num_blk_ver;
	uint8		num_blk_hor;
	uint8		b_enable_af_lut;
	uint16 auw_lut[259];
	uint16 auw_af_lut[259];
	uint8 auc_weight[6];
	uint16 uw_sh;
	uint8 uc_th_mode;
	uint8 auc_index[82];
	uint16 auw_th[4];
	uint16 pw_tv[4];
	uint32 ud_af_offset;
	uint8 b_af_py_enable;
	uint8 b_af_lpf_enable;
	enum allib_af_med_filter_mode n_filter_mode;
	uint8 uc_filter_id;
	uint16 uw_line_cnt;
#ifdef IIR_PARAM_ENABLE
	struct allib_af_lpf_t t_lpf;
	struct allib_af_iir_t at_iir[2];
	struct allib_af_pseudoy_t t_pseudoy;
#endif
};
#pragma pack(pop)

/*
 * allib_af_pd_config_type
 * PD_STATE:		update pd state.
 * PD_ROI:	update roi info.
 */
#pragma pack(push, 4)
enum allib_af_pd_config_type {
	alAFLib_PD_CONFIG_ENABLE=1,
	alAFLib_PD_CONFIG_ROI=(1<<1),
	alAFLib_PD_CONFIG_RESET=(1<<2),
};
#pragma pack(pop)

/*
 * allib_af_out_pd_roi_info_t
 * roi:		AF use roi info.
 * src_img_sz:	roi reference source img size, which is received from upper layer.
 * num_blk_ver:	vertical block number.
 * num_blk_hor:	horizontal block number.
 */

#pragma pack(push, 4)
struct allib_af_out_pd_roi_info_t {
	struct allib_af_roi_t	roi;
	struct allib_af_img_t	src_img_sz;
	uint8 num_blk_ver;
	uint8 num_blk_hor;
};
#pragma pack(pop)

/*
 * allib_af_out_pd_config_t
 * token_id
 * pd_roi_info:		AF use roi info.
 * type:	pd config type
 * pd_state:    pd state upadte.
 * b_enable:	enable flag for PD calculation
 */

#pragma pack(push, 4)
struct allib_af_out_pd_config_t {
	uint16 token_id;
	uint8 b_enable;
	enum allib_af_pd_config_type type;
	int32 pd_enable;
	struct allib_af_out_pd_roi_info_t pd_roi_info;
};
#pragma pack(pop)

#pragma pack(push, 4)
enum allib_af_output_type {
	alAFLIB_OUTPUT_UNKNOW = 1 ,
	alAFLIB_OUTPUT_STATUS =( 1 << 1),
	alAFLIB_OUTPUT_STATS_CONFIG =( 1 << 2),
	alAFLIB_OUTPUT_PD_CONFIG =( 1 << 3),
	alAFLIB_OUTPUT_IMG_BUF_DONE =( 1 << 4),
	alAFLIB_OUTPUT_MOVE_LENS =( 1 << 5),
	alAFLIB_OUTPUT_DEBUG_INFO =( 1 << 6),
	alAFLIB_OUTPUT_ROI =( 1 << 7),
	alAFLIB_OUTPUT_MAX
};
#pragma pack(pop)

/*
 * allib_af_output_report_t
 * sof_frame_id:	sof frame id of this output report.
 * result:	AFLib result. TRUE : AF control should updated AF report and response; FALSE : Do nothing.
 * type:		refer to alAFLib_output_type
 * focus_status:	AFLib current status. if triggered.
 * stats_config:	ISP driver configuration for HW AF.
 * roi: 			Output focused roi.
 * al_af_debug_data:	AF lib debug information.
 * al_af_debug_data_size
 * wrap_result:	returns result, the result define in al3ALib_AF_ErrCode.h
 * param_result:	param_result returns the error result that only effect AF focus quality but do not make lib crash.
 *
 * it often occurs when set / get parameter behavior so naming it as param_result.
 */
#pragma pack(push, 4)
struct allib_af_output_report_t {
	uint32 sof_frame_id;
	uint32 result;
	enum allib_af_output_type type;
	struct allib_af_out_status_t focus_status;
	struct allib_af_out_stats_config_t stats_config;
	struct allib_af_out_pd_config_t pd_config;
	struct allib_af_roi_t roi;
	void *p_al_af_debug_data;
	uint32 al_af_debug_data_size;
	void *p_al_af_exif_data;
	uint32 al_af_exif_data_size;
	uint32 wrap_result;
	uint16 param_result;
};
#pragma pack(pop)

/* alAFLib API */

/* callback functions */
typedef uint8 (*allib_af_set_param_func)(struct allib_af_input_set_param_t *param, void *allib_af_out_obj, void *allib_af_runtime_obj);
typedef uint8 (*allib_af_get_param_func)(struct allib_af_input_get_param_t *param, void *allib_af_out_obj, void *allib_af_runtime_obj);
typedef uint8 (*allib_af_process_func)(void *allib_af_hw_stats, void *allib_af_out_obj, void *allib_af_runtime_obj);
typedef void* (*allib_af_intial_func)(void *allib_af_out_obj);
typedef uint8 (*allib_af_deinit_func)(void *allib_af_runtime_obj, void *allib_af_out_obj);
typedef void (*allib_af_callback_func)(struct allib_af_output_report_t *output, void *phandle);

/*
 * allib_af_ops_t
 * initial:	initialize alAFLib, when open camera or something else happened.
 * deinit:	close alAFLib, when close camera or...etc.
 * set_param:	set data to alAFlib.
 * get_param:	get info from alAFLib.
 * process:
 */
#pragma pack(push, 4)
struct allib_af_ops_t {
	allib_af_intial_func	initial;
	allib_af_deinit_func	deinit;
	allib_af_set_param_func set_param;
	allib_af_get_param_func get_param;
	allib_af_process_func	process;
};
#pragma pack(pop)

void allib_af_get_lib_ver(struct allib_af_version_t *allib_af_ver );
uint8 allib_af_load_func(struct allib_af_ops_t *allib_af_ops);

#endif
/* end _ALAFLIB_H_ */
