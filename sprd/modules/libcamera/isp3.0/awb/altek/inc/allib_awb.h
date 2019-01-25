/******************************************************************************
 * File name: allib_awb.h
 * Create Date:
 *
 * Comment:
 * Describe common difinition of alAWBLib algo type definition / interface
 *****************************************************************************/

#ifndef _ALTEK_AWB_LIB_
#define _ALTEK_AWB_LIB_

/* Include files */
#include "pthread.h"
#include "mtype.h"
#include "hw3a_stats.h"
#include "allib_awb_errcode.h"
#include "frmwk_hw3a_event_type.h"

/* Macro definitions */

/* Static declarations */


/* Type declarations */
enum allib_awb_lightsource_t {
	alawb_lightsource_d75 = 0,      /* about 7500K */
	alawb_lightsource_d65,          /* about 6500K */
	alawb_lightsource_d55,          /* about 5500K */
	alawb_lightsource_cwf,          /* about 4000K */
	alawb_lightsource_t_light,      /* about 3800K */
	alawb_lightsource_a_light,      /* about 2850K */
	alawb_lightsource_h_light,      /* about 2380K */
	alawb_lightsource_flash,        /* flash light */
	alawb_lightsource_unknown
};

enum allib_awb_response_type_t {
	alawb_response_stable = 0,
	alawb_response_quick_act,
	alawb_response_direct,
	alawb_response_clear,
	alawb_response_unknow
};

enum allib_awb_response_level_t {
	alawb_response_level_normal = 0,
	alawb_response_level_slow,
	alawb_response_level_fast,
	alawb_response_level_unknown
};

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct allib_awb_awb_mode_setting_t {
	enum awb_mode_type_t        wbmode_type;            /* 0: Auto, others are MWB type */
	uint32                      manual_ct;              /* manual color temperature */
};
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct allib_awb_faceinfo_param_t {
	uint8                       flag_face;
	uint16                      x;
	uint16                      y;
	uint16                      w;
	uint16                      h;
	uint16                      frame_w;
	uint16                      frame_h;
};
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct allib_awb_response_setting_t {
	enum allib_awb_response_type_t   response_type;          /* AWB Stable type */
	enum allib_awb_response_level_t  response_level;         /* AWB Stable level */
};
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct allib_awb_ae_param_setting_t {
	enum ae_state_t             ae_state;               /* referrence to alAELib AE states */
	enum fe_state_t             fe_state;               /* referrence to alAELib FE states */
	int16                       ae_converge;
	int32                       bv;
	int32                       non_comp_bv;            /* non compensated bv value */
	uint32                      iso;                    /* ISO Speed */
	struct flash_report_data_t  flash_param_preview;
	struct flash_report_data_t  flash_param_capture;
};
#pragma pack(pop)  /* restore old alignment setting from stack */

enum allib_awb_awb_flow_setting_t {
	alawb_flow_none = 0,
	alawb_flow_lock,
	alawb_flow_bypass,
};

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct allib_awb_manual_flow_setting_t {
	enum allib_awb_awb_flow_setting_t manual_setting;         /* manual lock flag */
	struct wbgain_data_t              manual_wbgain;          /* manual lock wb gain */
	uint16                            manual_ct;              /* manual lock color temperature */
};
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct allib_awb_initial_data_t {
	struct wbgain_data_t        initial_wbgain;
	struct wbgain_data_t        initial_wbgain_balanced;
	uint16                      color_temperature;
};
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct allib_awb_debug_data_t {
	uint32                      data_size;
	void                        *data_addr;
};
#pragma pack(pop)  /* restore old alignment setting from stack */

enum allib_awb_set_flash_states_t {
	alawb_set_flash_none = 0,
	alawb_set_flash_prepare_under_flashon,
	alawb_set_flash_under_flashon,
	alawb_set_flash_under_flashon_capture,
};

enum allib_awb_awb_debug_type_t {
	alawb_dbg_none = 0,
	alawb_dbg_enable_log,
	alawb_dbg_manual,
	alawb_dbg_manual_flowCheck,
};

/* Report */
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct allib_awb_output_data_t {
	struct report_update_t              report_3a_update;          /* update awb information */
	uint32                              hw3a_curframeidx;          /* hw3a frame Index */
	uint32                              sys_cursof_frameidx;       /* sof frame Index */
	uint16                              awb_update;                /* valid awb to update or not */
	enum awb_mode_type_t                awb_mode;                  /* WB mode, 0: Auto, others are MWB type */
	struct wbgain_data_t                wbgain;                    /* WB gain for final result */
	struct wbgain_data_t                wbgain_balanced;           /* All balanced WB gain, for shading or else */
	struct wbgain_data_t                wbgain_capture;            /* WB gain for single shot */
	struct wbgain_data_t                wbgain_flash_off;          /* WB gain for flash control, flash off status, all-balanced */
	uint32                              color_temp;                /* (major) color temperature */
	uint32                              color_temp_capture;        /* one shot color temperature */
	uint32                              color_temp_flash_off;      /* flash off color temperature */
	enum allib_awb_lightsource_t        light_source;              /* light source */
	uint16                              awb_decision;              /* simple scene detect */
	uint8                               flag_shading_on;           /* 0: False, 1: True */
	enum awb_states_type_t              awb_states;                /* alAWBLib states */
	enum allib_awb_awb_debug_type_t     awb_debug_mask;            /* awb debug mask, can print different information with different mask */
	uint32                              awb_exif_data_size;        /* awb exif data size */
	void                                *awb_exif_data;            /* awb exif data, Structure1, about 340bytes */
	uint32                              awb_debug_data_size;       /* awb debug data size */
	void                                *awb_debug_data;           /* awb debug data, Structure2, about 10240bytes [TBD] */
};
#pragma pack(pop)  /* restore old alignment setting from stack */

/* Set / Get Param */
enum allib_awb_set_parameter_type_t {
	alawb_set_param_camera_calib_data = 1,
	alawb_set_param_tuning_file,
	alawb_set_param_sys_sof_frame_idx,
	alawb_set_param_awb_mode_setting,
	alawb_set_param_response_setting,
	alawb_set_param_update_ae_report,
	alawb_set_param_update_af_report,
	alawb_set_param_dzoom_factor,
	alawb_set_param_face_info_setting,
	alawb_set_param_flag_shading,
	alawb_set_param_awb_debug_mask,
	alawb_set_param_manual_flow,
	alawb_set_param_test_fix_patten,
	alawb_set_param_state_under_flash,
	alawb_set_param_identity_id,

	alawb_set_param_slave_calib_data,
	alawb_set_param_slave_tuning_file,
	alawb_set_param_slave_iso_speed,

	alawb_set_param_engineer_manual_wbgain,
	alawb_set_param_engineer_manual_balance_wbgain,
	alawb_set_param_engineer_manual_color_temp,

	alawb_set_param_max
};

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct allib_awb_set_parameter_t {
	enum allib_awb_set_parameter_type_t         type;
	union {
		struct calibration_data_t               awb_calib_data;         /* alawb_set_param_camera_calib_data / alawb_set_param_slave_calib_data */
		void                                    *tuning_file;           /* alawb_set_param_tuning_file / alawb_set_param_slave_tuning_file */
		uint32                                  sys_sof_frame_idx;      /* alawb_set_param_sys_sof_frame_idx */
		struct allib_awb_awb_mode_setting_t     awb_mode_setting;       /* alawb_set_param_awb_mode_setting */
		struct allib_awb_response_setting_t     awb_response_setting;   /* alawb_set_param_response_setting */
		struct allib_awb_ae_param_setting_t     ae_report_update;       /* alawb_set_param_update_ae_report */
		struct af_report_update_t               af_report_update;       /* alawb_set_param_update_af_report */
		float                                   dzoom_factor;           /* alawb_set_param_dzoom_factor */
		struct allib_awb_faceinfo_param_t       face_info;              /* alawb_set_param_face_info_setting */
		uint8                                   flag_shading;           /* alawb_set_param_flag_shading */
		enum allib_awb_awb_debug_type_t         awb_debug_mask;         /* alawb_set_param_awb_debug_mask */
		struct allib_awb_manual_flow_setting_t  awb_manual_flow;        /* alawb_set_param_manual_flow */
		uint8                                   test_fix_patten;        /* alawb_set_param_test_fix_patten */
		enum allib_awb_set_flash_states_t       state_under_flash;      /* alawb_set_param_state_under_flash */
		uint16                                  slave_iso_speed;        /* alawb_set_param_slave_iso_speed */
		uint8                                   identity_id;            /* alawb_set_param_identity_id */
		struct wbgain_data_t                    manual_wbgain;          /* alawb_set_param_engineer_manual_wbgain / alawb_set_param_engineer_manual_balance_wbgain */
		uint16                                  manual_color_temp;      /* alawb_set_param_engineer_manual_color_temp */
	}   para;
};
#pragma pack(pop)  /* restore old alignment setting from stack */

enum allib_awb_get_parameter_type_t {
	alawb_get_param_init_isp_config = 1,
	alawb_get_param_init_setting,
	alawb_get_param_wbgain,
	alawb_get_param_wbgain_balanced,
	alawb_get_param_wbgain_flash_off,
	alawb_get_param_color_temperature,
	alawb_get_param_light_source,
	alawb_get_param_awb_decision,
	alawb_get_param_manual_flow,
	alawb_get_param_test_fix_patten,
	alawb_get_param_awb_states,
	alawb_get_param_debug_data_exif,
	alawb_get_param_debug_data_full,
	alawb_get_param_max
};

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct allib_awb_get_parameter_t {
	enum allib_awb_get_parameter_type_t         type;
	uint32                                      hw3a_curframeidx;
	uint32                                      sys_cursof_frameidx;
	union {
		struct alhw3a_awb_cfginfo_t             *awb_hw_config;         /* alawb_get_param_init_isp_config */
		struct allib_awb_initial_data_t         awb_init_data;          /* alawb_get_param_init_setting */
		struct wbgain_data_t                    wbgain;                 /* alawb_get_param_wbgain */
		struct wbgain_data_t                    wbgain_balanced;        /* alawb_get_param_wbgain_balanced */
		struct wbgain_data_t                    wbgain_flash_off;       /* alawb_get_param_wbgain_flash_off */
		uint32                                  color_temp;             /* alawb_get_param_color_temperature */
		uint16                                  light_source;           /* alawb_get_param_light_source */
		uint16                                  awb_decision;           /* alawb_get_param_awb_decision */
		struct allib_awb_manual_flow_setting_t  awb_manual_flow;        /* alawb_get_param_manual_flow */
		uint8                                   test_fix_patten;        /* alawb_get_param_test_fix_patten */
		enum awb_states_type_t                  awb_states;             /* alawb_get_param_awb_states */
		struct allib_awb_debug_data_t           debug_data;             /* alawb_get_param_debug_data_exif & full */
	}   para;
};
#pragma pack(pop)  /* restore old alignment setting from stack */


/* public APIs */
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct allib_awb_lib_version_t {
	uint16 major_version;
	uint16 minor_version;
};
#pragma pack(pop)  /* restore old alignment setting from stack */

void allib_awb_getlib_version(struct allib_awb_lib_version_t *awb_libversion);

typedef uint32 (*allib_awb_init_func)(void *awb_obj);
typedef uint32 (*allib_awb_deinit_func)(void *awb_obj);
typedef uint32 (*allib_awb_set_param_func)(struct allib_awb_set_parameter_t *param, void *awb_dat);
typedef uint32 (*allib_awb_get_param_func)(struct allib_awb_get_parameter_t *param, void *awb_dat);
typedef uint32 (*allib_awb_estimation_func)(void *hw3a_stats_data, void *awb_dat, struct allib_awb_output_data_t *awb_output);
typedef uint32 (*allib_awb_match_func)(void *awb_dat, struct allib_awb_output_data_t *match_output);

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct allib_awb_runtime_obj_t {
	pthread_mutex_t             mutex_obj;
	uint32                      obj_verification;
	void                        *awb;
	allib_awb_init_func         initial;
	allib_awb_deinit_func       deinit;
	allib_awb_set_param_func    set_param;
	allib_awb_get_param_func    get_param;
	allib_awb_estimation_func   estimation;
	allib_awb_match_func        match;
};
#pragma pack(pop)  /* restore old alignment setting from stack */

/* Return: TRUE: loading with no error , FALSE: false loading function APIs address. */
uint8 allib_awb_loadfunc(struct allib_awb_runtime_obj_t *awb_run_obj);

#endif /*_ALTEK_AWB_LIB_ */

