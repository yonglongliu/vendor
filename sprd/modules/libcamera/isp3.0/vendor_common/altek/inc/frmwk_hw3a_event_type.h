/******************************************************************************
 * File name: frmwk_hw3a_event_type.h
 * Create Date:
 *
 * Comment:
 * Describe common difinition of HW3A event
 *
 *****************************************************************************/

#ifndef _AL_FRMWK_HW3A_EVENT_TYPE_H_
#define _AL_FRMWK_HW3A_EVENT_TYPE_H_

/******************************************************************************
 * Include files
 *****************************************************************************/
#include "mtype.h"


/******************************************************************************
 * Macro definitions
 *****************************************************************************/

#define MAX_RUNTIME_AE_DEBUG_DATA       (3072)
#define MAX_RUNTIME_AWB_DEBUG_DATA      (2048)
#define MAX_RUNTIME_AF_DEBUG_DATA       (8192)
#define MAX_AE_DEBUG_ARRAY_NUM          MAX_RUNTIME_AE_DEBUG_DATA
#define MAX_AE_COMMON_EXIF_DATA         (1024)

/******************************************************************************
 * Static declarations
 *****************************************************************************/


/******************************************************************************
 * Type declarations
 *****************************************************************************/
#ifdef __cplusplus
extern "C"
{
#endif

enum ae_antiflicker_mode_t {
	ANTIFLICKER_50HZ,
	ANTIFLICKER_60HZ,
	ANTIFLICKER_OFF,
};

enum antiflicker_ui_mode_t {
	ANTIFLICKER_UI_MODE_AUTO = 0,
	ANTIFLICKER_UI_MODE_FIXED_50HZ,
	ANTIFLICKER_UI_MODE_FIXED_60HZ,
	ANTIFLICKER_UI_MODE_OFF,
};

enum color_order_mode_t {
	COLOR_RGGB,
	COLOR_GRGB,
	COLOR_GBGR,
	COLOR_BGGR,
};

enum al3a_operation_mode {
	AL3A_OPMODE_PREVIEW = 0,
	AL3A_OPMODE_VIDEO,
	AL3A_OPMODE_SNAPSHOT,
	AL3A_OPMODE_MAX_MODE
};

enum al3a_roi_type {
	AL3A_ROI_DEFAULT = 0,
	AL3A_ROI_FACE ,
	AL3A_ROI_TOUCH,
	AL3A_ROI_MAX
};

enum al3a_fe_flash_stat {
	AL3A_FLASH_OFF = 0,
	AL3A_FLASH_PRE,
	AL3A_FLASH_MAIN,
	AL3A_FLASH_RAMPUP,
	AL3A_FLASH_RELEASE,
};

/* flash setting mode for flash */
enum al3a_fe_ui_flash_mode {
	AL3A_UI_FLASH_OFF = 0,
	AL3A_UI_FLASH_AUTO,
	AL3A_UI_FLASH_ON,
	AL3A_UI_FLASH_TORCH,
	AL3A_UI_FLASH_MAX,
};

/* MWB Mode */
enum awb_mode_type_t {
	AL3A_WB_MODE_OFF = 0,
	AL3A_WB_MODE_AUTO,
	AL3A_WB_MODE_INCANDESCENT,
	AL3A_WB_MODE_FLUORESCENT,
	AL3A_WB_MODE_WARM_FLUORESCENT,
	AL3A_WB_MODE_DAYLIGHT,
	AL3A_WB_MODE_CLOUDY_DAYLIGHT,
	AL3A_WB_MODE_TWILIGHT,
	AL3A_WB_MODE_SHADE,
	AL3A_WB_MODE_MANUAL_CT,
	AL3A_WB_MODE_UNDEF
};

/*
 *
 *  Enumrator for EXIF debug switch
 *  @EXIF_DEBUG_MASK_OFF : turn off all exif debug embedding
 *  @EXIF_DEBUG_MASK_AE :  turn on AE exif debug embedding
 *  @EXIF_DEBUG_MASK_AWB :  turn on AWB exif debug embedding
 *  @EXIF_DEBUG_MASK_AF :  turn on AF exif debug embedding
 *  @EXIF_DEBUG_MASK_AS :  turn on auto scene exif debug embedding
 *  @EXIF_DEBUG_MASK_AFD : turn on anti-flicker exif debug embedding
 *  @EXIF_DEBUG_MASK_UDEF : turn on user defined exif debug embedding (3rd party info)
 *
 */
enum exif_debug_mask_mode {
	EXIF_DEBUG_MASK_OFF     = 0,
	EXIF_DEBUG_MASK_AE      = 1 ,
	EXIF_DEBUG_MASK_AWB     = 1 << 1,
	EXIF_DEBUG_MASK_AF      = 1 << 2,
	EXIF_DEBUG_MASK_AS      = 1 << 3,
	EXIF_DEBUG_MASK_AFD     = 1 << 4,
	EXIF_DEBUG_MASK_FRMWK   = 1 << 5,
	EXIF_DEBUG_MASK_ISP     = 1 << 6,  /* For ISP debug */
	EXIF_DEBUG_MASK_UDEF    = 1 << 7,
};

enum ae_state_t {
	AE_NOT_INITED = 0,
	AE_RUNNING,
	AE_CONVERGED,
	AE_ROI_CONVERGED,
	AE_PRECAP_CONVERGED,
	AE_LMT_CONVERGED,
	AE_DISABLED,
	AE_LOCKED,
	AE_STABLE_LOCK,
	AE_STATE_MAX
} ;

/* flash running status */
enum fe_state_t {
	AE_EST_WITH_LED_OFF,
	AE_EST_WITH_LED_PRE_DONE,
	AE_EST_WITH_LED_RUNNING,
	AE_EST_WITH_LED_DONE,
	AE_EST_WITH_LED_REQUEST_LED_OFF,    /* reserved */
	AE_EST_WITH_LED_RECOVER_LV_EXP,       /* reserved */
	AE_EST_WITH_LED_RECOVER_LV_EXP_DONE,   /* reserved */

	AE_EST_WITH_LED_STATE_MAX
};

/* AWB States */
enum awb_states_type_t {
	AL3A_WB_STATE_UNSTABLE = 0,
	AL3A_WB_STATE_STABLE,
	AL3A_WB_STATE_LOCK,
	AL3A_WB_STATE_PREPARE_UNDER_FLASHON_DONE,
	AL3A_WB_STATE_UNDER_FLASHON_AWB_DONE,
};

/*
enum flicker_state_t {
	FLICKER_NOT_INITED = 0,
	FLICKER_RUNNING,
	FLICKER_DISABLED,
	FLICKER_LOCKED,
	FLICKER_STATE_MAX
} ;
*/

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct wbgain_data_t {
	uint16 r_gain;              /* scale 256 */
	uint16 g_gain;              /* scale 256 */
	uint16 b_gain;              /* scale 256 */
};
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct calibration_data_t {
	uint32 minISO;
	uint32 calib_r_gain;        /* scale 1000 */
	uint32 calib_g_gain;        /* scale 1000 */
	uint32 calib_b_gain;        /* scale 1000 */
};
#pragma pack(pop)  /* restore old alignment setting from stack */


#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct exposure_data_t {
	uint32  ad_gain;            /* sensor AD gain */
	uint32  isp_gain;           /* ISP D gain, reserved */
	uint32  exposure_time;      /* exposure time */
	uint32  exposure_line;      /* exposure line */
	uint32  ISO;
	uint32  ExifISO;
};
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct flash_control_data_t {
	int16  ucDICTotal_idx;      /* driving IC total index, -1 : invalid index */
	int16  ucDIC1_idx;          /* driving IC LED1 index, -1 : invalid index */
	int16  ucDIC2_idx;          /* driving IC LED2 index, -1 : invalid index */

	float  fLEDTotalCurrent;   /* -1 : invalid index */
	float  fLED1Current;       /* -1 : invalid index */
	float  fLED2Current;       /* -1 : invalid index */

};
#pragma pack(pop)  /* restore old alignment setting from stack */


#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct flash_report_data_t {
	struct wbgain_data_t   flash_gain;
	struct wbgain_data_t   flash_gain_led2;    /* to AWB */
	uint32          flash_ratio;        /* to AWB */
	uint32          flash_ratio_led2;   /* to AWB */

	uint32          LED1_CT;            /* uints: K */
	uint32          LED2_CT;            /* uints: K */

};
#pragma pack(pop)  /* restore old alignment setting from stack */



#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct ae_report_update_t {
	/* broadcase to other ISP/3A/sensor module from current 3A module */

	/* common stats info */
	uint32 hw3a_frame_id;               /* frame ID when HW3a updated */
	uint32 sys_sof_index;               /* current SOF ID, should be maintained by framework */
	uint32 hw3a_ae_block_nums;          /* total blocks */
	uint32 hw3a_ae_block_totalpixels;   /* total pixels in each blocks */
	uint32 hw3a_ae_stats[256];          /* HW3a AE stats addr */

	/* AE runing status */
	uint32 targetmean;                  /* converge basic target mean */
	uint32 curmean;                     /* current AE weighted mean (after weighting mask) */
	uint32 avgmean;                     /* current AE average mean  (weighting mask all 1.0),
					     * used for background report, which would be update during AE locked status */
	uint32 center_mean2x2;              /* current center mean in 2x2 blocks */
	int32   bv_val;                     /* current BV value */
	int32   BG_BVResult;                /* BV value from average mean, updated even at AE locked
					     * (not update when AE disabled);  */

	int16  ae_converged;                /* AE converged flag */
	uint8   ae_need_flash_flg;          /* flash needed flag */

	enum ae_state_t  ae_LibStates;           /* AE running status, refer to ae_state_t */
	enum fe_state_t   ae_FlashStates;        /* Flash AE status, refer to fe_state_t */

	/* common exposure paraeter to sensor */
	uint32 sensor_ad_gain;              /* sensor A/D gain setting */
	uint32 isp_d_gain;                  /* ISP additional D gain supported, need ISP to generate HW3a stats, reserved */
	uint32 exposure_time;               /* current exposure time */
	uint32 exposure_line;               /* current exposure line */
	uint32 exposure_max_line;           /* max exposure line in current sensor mode */
	uint32 ae_cur_iso;                  /* current ISO speed */
	uint32  udExifISO;			/* EXIF ISO speed */


	/* AE control status */
	int16  ae_roi_change_st;            /* 0 : no response to ROI change, 1: ROI changed and taken,
					     * this reset should be performed via framework once framework ack this message status */
	enum ae_antiflicker_mode_t  flicker_mode;/* current flicker mode, should be set via set_param API */
	uint16 ae_metering_mode;            /* refer to ae_metering_mode_type_t */
	uint16 ae_script_mode;              /* refer to ae_script_mode_t */

	uint16 DigitalZoom;                 /* store current valid digital zoom */

	int16  bv_delta;                    /* delta BV from converge */
	uint16  cur_fps;                    /* current FPS, from framework control information */

	struct exposure_data_t    flash_off_exp_dat;
	struct exposure_data_t    snapshot_exp_dat;

	/* hdr control */
	uint32  hdr_exp_ratio;              /* sacle by 100, control HDR sensor long/short exposure ratio */
	uint32  hdr_ae_long_exp_line;       /* sensor */
	uint32  hdr_ae_long_exp_time;       /* appplication */
	uint32  hdr_ae_long_ad_gain;        /* sensor */
	uint32  hdr_ae_short_exp_line;      /* sensor */
	uint32  hdr_ae_short_exp_time;      /* application */
	uint32  hdr_ae_short_ad_gain;       /* sensor */

	/* extreme color info */
	uint32    ae_extreme_green_cnt;     /* green color count */
	uint32    ae_extreme_blue_cnt;      /* blue color count */
	uint32    ae_extreme_color_cnt;     /* pure color count */

	int32    ae_non_comp_bv_val;        /* current non compensated BV value, would be refered by AWB */

	/* flash output report */
	struct flash_report_data_t      preflash_report;       /* to AWB */
	struct flash_report_data_t      mainflash_report;      /* to AWB */

	struct flash_control_data_t   preflash_ctrldat;        /* to Framework */
	struct flash_control_data_t   mainflash_ctrldat;       /* to framework */

	/* AE state machine, would be implemented by framework layer of Ae interface */
	uint32             ae_Frmwkstates;              /* store states of framework depend state machine define */

	/* common exif info for Altek exif reader */
	uint32  ae_commonexif_valid_size;
	void* ae_commonexif_data;

	uint8   ucIsEnableAeDebugReport;             /* 0: disable debug report, 1: enable debug report */
	/* debug message for altek advanced debug reader info */
	uint32  ae_debug_valid_size;
	void* ae_debug_data;

};
#pragma pack(pop)  /* restore old alignment setting from stack */


#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct awb_report_update_t {
	/* broadcase to other ISP/3A/sensor module from current 3A module */
	uint32                  hw3a_frame_id;          /* hw3A frame ID, reported from HW3A stats */
	uint32                  sys_cursof_frameidx;    /* sof frame Index */
	uint16                  awb_update;             /* valid awb to update or not */
	enum awb_mode_type_t         awb_mode;               /* 0: Auto, others are MWB type */
	struct wbgain_data_t           wbgain;
	struct wbgain_data_t           wbgain_balanced;
	struct wbgain_data_t           wbgain_flash_off;       /* for flash contorl,
							 * stop updating uder set state_under_flash = TRUE
							 * (event: prepare_under_flash) */
	uint32                  color_temp;
	uint32                  color_temp_flash_off;   /* for flash contorl,
							 * stop updating uder set state_under_flash = TRUE
							 * (event: prepare_under_flash) */
	uint16                  light_source;           /* light source */
	uint16                  awb_decision;           /* simple scene detect [TBD] */
	enum awb_states_type_t       awb_states;
};
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct af_report_update_t {
	/* broadcase to other ISP/3A/sensor module from current 3A module */
	uint8      focus_done;
	uint16     status;        /* refer to af_status_type define */
	float f_distance;

};
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct flicker_report_update_t {
	//enum flicker_state_t  flicker_LibStates;           /* Flicker running status, refer to ae_state_t */
	uint8   IsEnableFlickerDebugReport;             /* 0: disable debug report, 1: enable debug report */

	/* common exif info for Altek exif reader */
	uint32  flicker_exif_valid_size;
	void* flicker_exif_data;

	/* debug message for altek advanced debug reader info */
	uint32  flicker_debug_valid_size;
	void* flicker_debug_data;
};
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct report_update_t {
	/* broadcast to other ISP/Modules */
	struct ae_report_update_t ae_update;      /* latest AE update */
	struct awb_report_update_t awb_update;    /* latest AWB update */
	struct af_report_update_t  af_update;     /* latest AF update */
	struct flicker_report_update_t flicker_update;      /* latest Flicker update */

};
#pragma pack(pop)  /* restore old alignment setting from stack */

#ifdef __cplusplus
}  /*extern "C" */
#endif

#endif /* _AL_FRMWK_HW3A_EVENT_TYPE_H_ */
