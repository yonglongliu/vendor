/******************************************************************************
 *  File name: AL_AE.h
 *  Create Date:
 *
 *  Comment:
 *  Describe common difinition of AE algo type definition
 *  interface
 *
 *****************************************************************************/

#ifndef _AL_AE_H_
#define _AL_AE_H_

/******************************************************************************
 * Include files
 *****************************************************************************/
#include "pthread.h"
#include "frmwk_hw3a_event_type.h"
#include "hw3a_stats.h"

/******************************************************************************
 * Macro definitions
 *****************************************************************************/


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

/*
 *@typedef ae_metering_mode_type_t
 *@brief AE metering mode
 */
enum ae_metering_mode_type_t {
	AE_METERING_AVERAGE,
	AE_METERING_CENTERWT,
	AE_METERING_SPOTWT,
	AE_METERING_USERDEF_WT,
	AE_METERING_MAX_MODE
};

/*
 *@typedef ae_iso_mode_t
 *@brief AE ISO speed mode
 */
enum ae_iso_mode_t {
	AE_ISO_AUTO = 0,
	AE_ISO_100,
	AE_ISO_200,
	AE_ISO_400,
	AE_ISO_800,
	AE_ISO_1600,
	AE_ISO_3200,
	AE_ISO_6400,
	AE_ISO_12800,
	AE_ISO_USRDEF,
	AE_ISO_MAX
};

/*
 *@typedef ae_ev_mode_t
 *@brief AE EV compensation mode
 */
enum ae_ev_mode_t {
	AE_UIEVCOMP_USRDEF = 0,
	AE_EV_LEVEL_1,
	AE_EV_LEVEL_2,
	AE_EV_LEVEL_3,
	AE_EV_LEVEL_4,
	AE_EV_LEVEL_5,
	AE_EV_LEVEL_6,
	AE_EV_LEVEL_7,
	AE_EV_LEVEL_8,
	AE_EV_LEVEL_9,
	AE_EV_LEVEL_10,
	AE_EV_LEVEL_11,
	AE_EV_LEVEL_12,
	AE_EV_LEVEL_13,
	AE_EV_LEVEL_14,
	AE_EV_LEVEL_15,
	AE_EV_LEVEL_16,
	AE_EV_LEVEL_17,
	AE_EV_LEVEL_18,
	AE_EV_LEVEL_19,
	AE_EV_LEVEL_20,
	AE_EV_LEVEL_21,
	AE_EV_LEVEL_22,
	AE_EV_LEVEL_23,
	AE_EV_LEVEL_24,
	AE_EV_LEVEL_25,
	AE_EV_LEVEL_26,
	AE_EV_LEVEL_27,
	AE_EV_LEVEL_28,
	AE_EV_LEVEL_29,
	AE_EV_LEVEL_30,
	AE_EV_LEVEL_31,
	AE_EV_LEVEL_32,
	AE_EV_LEVEL_33,
	AE_EV_LEVEL_34,
	AE_EV_LEVEL_35,
	AE_EV_LEVEL_36,
	AE_EV_LEVEL_37,
	AE_EV_LEVEL_38,
	AE_EV_LEVEL_39,
	AE_EV_LEVEL_40,
	AE_EV_LEVEL_41,
	AE_EV_MAX,
};

/*
 *@typedef ae_gain_mode_t
 *@brief AE AD gain mode
 */
enum ae_gain_mode_t {
	AE_ADGAIN_AUTO = 0,
	AE_ADGAIN_100,
	AE_ADGAIN_200,
	AE_ADGAIN_400,
	AE_ADGAIN_800,
	AE_ADGAIN_1600,
	AE_ADGAIN_3200,
	AE_ADGAIN_6400,
	AE_ADGAIN_12800,
	AE_ADGAIN_USRDEF,
	AE_ADGAIN_MAX
};

/*
 *@typedef ae_flash_st_t
 *@brief Flash (HW) status (off/on)
 */
enum ae_flash_st_t {
	AE_FLASH_OFF,		/* flash HW status is switched off, no matter is torch off, or main-flash off, or pre-flash off */
	AE_FLASH_ON,		/* flash HW status is switched on */
	AE_FLASH_CANCEL	/* time out */
};

/*
 *@typedef ae_script_mode_t
 *@brief Script mode (off/on, AE/FE)
 */
enum ae_script_mode_t {
	/* normal control  */
	AE_SCRIPT_OFF,	/* turn off script mode  */
	AE_SCRIPT_ON,	/* turn on AE script mode  */
	FE_SCRIPT_ON,	/* turn on FE script mode  */

	/*  special control */
	AE_SCRIPT_PAUSE,	/* used when special case, pause current script running until resume or OFF command send  */
	AE_SCRIPT_RESUME,	/* resume script mode when current script mode under pause status, no used for ON/OFF mode  */
	AE_SCRIPT_MAX
};

/*
 *@typedef ae_script_st_t
 *@brief AE script status
 */
enum ae_script_st_t {
	/* normal control  */
	SCRIPT_IDLE,	/* Init */
	SCRIPT_WAIT_EXPOSURE_VALID,
	SCRIPT_WAIT_LED_TURNON,
	SCRIPT_WAIT_LED_TURNOFF,
	SCRIPT_LOG_MSG,
	SCRIPT_WAIT_NEXT_ROUND,
	SCRIPT_DONE
};

/*
 *@typedef ae_sensorctrl_test_flag_t
 *@brief ae sensor ctrl test flag
 */
enum ae_sensorctrl_test_flag_t {
	SENSOR_TEST_OFF = 0,
	SENSOR_TEST_AUTO_ON,
	SENSOR_TEST_MANUAL_ON,
	SENSOR_TEST_MAX
};

/*
 *@typedef ae_converge_level_type_t
 *@brief AE converge speed level
 */
enum ae_converge_level_type_t {
	AE_CONVERGE_NORMAL,	/* normal speed */
	AE_CONVERGE_DIRECT,	/* fastest speed */
	AE_CONVERGE_FAST,	/* fast speed */
	AE_CONVERGE_SMOOTH,	/* slower than normal, recommand for video or face  */
	AE_CONVERGE_SLOW,	/* slower than smooth, recommand for video or face  */
	AE_CONVERGE_SPEED_LVMAX
};

/*
 *@typedef ae_capture_mode_t
 *@brief Capture mode
 */
enum ae_capture_mode_t {
	CAPTURE_MODE_AUTO,
	CAPTURE_MODE_ZSL,
	CAPTURE_MODE_HDR,		/* ZSL HDR */
	CAPTURE_MODE_VIDEO,
	CAPTURE_MODE_VIDEO_HDR,
	CAPTURE_MODE_BRACKET,		/* ZSL AE bracket */
	CATPURE_MODE_DUALCAM_SYNC,
	CAPTURE_MODE_DUALCAM_ASYNC,	/* Fusion */
	CAPTURE_MODE_FASTSHOT,
	CAPTURE_MODE_FASTSHOT_DUALCAM,
	CAPTURE_MODE_MAX
};

/*
 *@typedef ae_scene_mode_t
 *@brief Scene mode
 */
enum ae_scene_mode_t {
	SCENE_MODE_AUTO,
	SCENE_MODE_PROGRAM,
	SCENE_MODE_PORTRAIT,
	SCENE_MODE_LANDSCAPE,
	SCENE_MODE_NIGHT,
	SCENE_MODE_NIGHT_PORTRAIT,
	SCENE_MODE_BEACH,
	SCENE_MODE_SNOW,
	SCENE_MODE_SUNSET,
	SCENE_MODE_FLOWER,
	SCENE_MODE_TEXT,
	SCENE_MODE_BACKLIGHT,
	SCENE_MODE_FIREWORK,
	SCENE_MODE_PANORAMA,
	SCENE_MODE_SPORT,
	SCENE_MODE_PARTY,
	SCENE_MODE_ANTI_SHAKE,
	SCENE_MODE_CANDLELIGHT,
	SCENE_MODE_HDR,			/* for HDR algoritm , should have some priority AE (highkey priority, lowkey priority) */
	SCENE_MODE_BARCODE,		/* this scene mode may same as text scene mode  */
	SCENE_MODE_MAX
};

/*
 *@typedef ae_set_param_type_t
 *@brief Set param command ID
 */
enum ae_set_param_type_t {
	AE_SET_PARAM_INVALID_ENTRY,	/* invalid entry start */
	/*  Initial setting   */
	AE_SET_PARAM_INIT_SETTING,	/* avoid zero param to be valid */
	AE_SET_PARAM_OTP_WB_DAT,	/* update OTP calibration data */

	/*  Basic command   */
	AE_SET_PARAM_ENABLE,
	AE_SET_PARAM_LOCKAE,
	AE_SET_PARAM_UI_EVCOMP,
	AE_SET_PARAM_UI_EVCOMP_LEVEL,
	AE_SET_PARAM_3RD_EVCOMP,
	AE_SET_PARAM_TARTGETMEAN,
	AE_SET_PARAM_METERING_MODE,
	AE_SET_PARAM_ISO_MODE,
	AE_SET_PARAM_SCENE_MODE,
	AE_SET_PARAM_FLASH_MODE,	/* auto, force-fill, foce-off, refer to al3a_fe_ui_flash_mode */
	AE_SET_PARAM_CAPTURE_MODE,	/* ZSL, non-ZSL */
	AE_SET_PARAM_BRACKET_MODE,
	AE_SET_PARAM_ANTIFLICKERMODE,
	AE_SET_PARAM_SOF_NOTIFY,	/* inform AE lib current SOF changed  */
	AE_SET_PARAM_ISP_DGAIN_SOF_NOTIFY,	/* inform AE lib current SOF changed  */

	AE_SET_PARAM_FPS,
	AE_SET_PARAM_ROI,		/*  touch ROI info  */
	AE_SET_PARAM_FACE_ROI,		/* face ROI info */
	AE_SET_PARAM_METERING_TABLE,
	AE_SET_PARAM_DIGITAL_ZOOM,

	AE_SET_PARAM_RESET_ROI_ACK,	/* reset ROI ack status once framework received ae_report_update_t->ae_roi_change_st = 1, after reset, next output would receive ae_report_update_t->ae_roi_change_st = 0 */

	AE_SET_PARAM_CONVERGE_SPD,	/* control AE converge speed */
	AE_SET_PARAM_ENABLE_DEBUG_REPORT,	/* enable AE debug 3K report reporting */

	/* Outer update, ex: AWB, AF, motion, etc.  */
	AE_SET_PARAM_AWB_INFO,		/* set AWB update information (From 3A AWB) */
	AE_SET_PARAM_AF_INFO,		/* set AF update information  (From 3A AWB) */
	AE_SET_PARAM_GYRO_INFO,		/* gyro status */
	AE_SET_PARAM_MOTION_INFO,	/* in frame object motion status */
	AE_SET_PARAM_SENSOR_INFO,	/* used for normal mode preview sensor info */
	AE_SET_PARAM_CAPTURE_SENSOR_INFO,	/* used for capture mode sensor info */

	/*  Update flash stats, command   */
	AE_SET_PARAM_PRE_FLASHAE_EST,		/* Prepare to do AE with flash */
	AE_SET_PARAM_FLASHAE_EST,
	AE_SET_PARAM_RESET_FLASH_EST,		/* reset flash exposure report */
	AE_SET_PARAM_FLASH_ST,			/* inform AE flash HW stats (on, off, timeout) */
	AE_SET_PARAM_FORCE_FLASHEST_CONVERGE,	/* used when framework need to force abort current converging (running time expired), use this command to abort current converging */

	AE_SET_PARAM_PRECAP_START,		/* precapture start */

	AE_SET_PARAM_SENSOR_HDR_MODE,		/*  sensor HDR mode, which should be handled by wrapper to parsing to 16x16 Y stats (such as Sony IMX214 sensor) */

	/*  Manual control command */
	AE_SET_PARAM_MANUAL_ISO,
	AE_SET_PARAM_MANUAL_EXPTIME,
	AE_SET_PARAM_MANAUL_GAIN,
	AE_SET_PARAM_MANAUL_ISP_GAIN,		/* reserved */
	AE_SET_PARAM_RELOAD_SETFILE,

	/* script test command */
	AE_SET_PARAM_SET_SCRIPT_MODE,		/* set mode on/off, which would be used for AE linearity check, FE response DVT */
	AE_SET_PARAM_SET_SCRIPT_PARAM,		/* update script param, should be set before turn on script mode */

	AE_SET_PARAM_RESET_STREAM_INFO,

	AE_SET_PARAM_REGEN_EXP_INFO,		/* for sensor mode change, UI/Ctrl force to reset sensor info, this set command let AE re-generate exposure time/line/gain again based on updated information */

	/* for dual camera sync usage   */
	AE_SET_PARAM_OTP_WB_DAT_SLV,	       /* update OTP calibration data, for dual cam sync mode */
	AE_SET_PARAM_SENSOR_INFO_SLV,            /* update slave sensor info */
	AE_SET_PARAM_CAPTURE_SENSOR_INFO_SLV,     /* update slave sensor info */
	AE_SET_PARAM_SOF_NOTIFY_SLV,              /* update slave SOF notify */
	AE_SET_PARAM_SYNC_MODE,                       /* update sync mdoe flag */
	AE_SET_PARAM_ENGINEER_MODE,                       /* Turn on/off Engineer mode,1:on;0:off */
	AE_SET_PARAM_SENSOR_CTRL_TEST,               /* Turn on/off test mode,and config max 4 sets of test exposure param*/
	AE_SET_PARAM_SCENE_SETTING,               /* Set Scene UIEVcomp,Pcurve node,exposure priority,manual exposure param*/
	AE_SET_PARAM_MAX
};

/*
 *@typedef ae_get_param_type_t
 *@brief Get param command ID
 */
enum ae_get_param_type_t {
	AE_GET_PARAM_INVALID_ENTRY,
	AE_GET_EXPOSURE_PARAM,
	AE_GET_INIT_EXPOSURE_PARAM,
	AE_GET_ALHW3A_CONFIG,
	AE_GET_CURRENT_CALIB_WB,
	AE_GET_CURRENT_WB,
	AE_GET_SCRIPT_MODE,
	AE_GET_DUAL_FLASH_ST,
	AE_GET_ISO_ADGAIN_FACTOR,
	AE_GET_ISO_FROM_ADGAIN,
	AE_GET_COMMON_EXIF,
	AE_GET_FULL_DEBUG,
	AE_GET_IQINFO_TO_ISP,

	AE_GET_PARAM_MAX
} ;

/*
 *@typedef ae_sensor_info_t
 *@brief Sensor infomation
 */
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
struct ae_sensor_info_t {
	uint32 min_fps;		/* scale 100, if 16.0 FPS, input 1600 as max FPS, suggest value: 1600 */
	uint32 max_fps;		/* scale 100, if 30.0 FPS, input 3000 as max FPS, suggest value: 3000 */
	uint32 min_line_cnt;	/* minimun exposure line, suggest value 1~ 4 (follow sensor FAE suggestion) */
	uint32 max_line_cnt;	/* maximun exposure line, suggest value 65535 (follow sensor FAE suggestion), which corresponding to min FPS setting
				*  if min FPS = 16, means max exposure time = 1/16 = 62.5 ms
				*  if 1 line = 20 us, max line count = 62.5ms / 20us = 3125 lines
				*/

	uint32 exposuretime_per_exp_line_ns;	/* optional, used for more precise control, if set 0 , would use min FPS vs max line count to calculate
						*automatically
						* note: use ns as units to keep precision, which would have difference for long exposure
						* ex: min_fps = 15.99, max line = 3000
						*       1 line = 1s  * 100/1599 / 3000 = 1000000000 (ns) *100 / 1599/3000
						*                =  20.846 (us) = 20846 (ns)
						*/

	uint32 min_gain;	/* scale 100, if 1.0x, set 100, suggest value: 100 (1.0x is most setting for every sensor) */
	uint32 max_gain;	/* scale 100, if 128.0x, set 12800, suggest value: 9600 (around ISO 6400) */

	/* sensor info, cropped size  */
	uint32 sensor_left;	/* if crop sensor mode is disable, this value should not be 0, units: pixel */
	uint32 sensor_top;	/* if crop sensor mode is disable, this value should not be 0, units: pixel */
	uint32 sensor_width;	/* if crop sensor mode is disable, this value should same as sensor mode original RAW size */
	uint32 sensor_height;	/* if crop sensor mode is disable, this value should same as sensor mode  original RAW size */

	/* sensor info, original size  */
	uint32 sensor_fullwidth;	/* sensor mode original RAW size */
	uint32 sensor_fullheight;	/* sensor mode original RAW size */

	/* OIS feature */
	uint8  ois_supported;

	/* Fn number, should refer to lens spec */
	float f_number;		/* if aperture = 2.0, here input 2.0 */
};
#pragma pack(pop)  /* restore old alignment setting from stack  */

/*
 *@typedef ae_gyro_info_t
 *@brief Gyro info
 */
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
struct ae_gyro_info_t {
	uint8    param_mode_int;	/* true: interger, false: float */
	uint32  uinfo[3];
	float   finfo[3];
};
#pragma pack(pop)  /* restore old alignment setting from stack  */

/*
 *@typedef ae_awb_info_t
 *@brief AWB information for AE
 */
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
struct ae_awb_info_t {
	uint32  color_temp;
	uint16  gain_r;	/* 256 base */
	uint16  gain_g;	/* 256 base */
	uint16  gain_b;	/* 256 base */

	struct wbgain_data_t awb_balanced_wb;	/* used for flash estimation */

	uint16  awb_state;
};
#pragma pack(pop)  /* restore old alignment setting from stack  */

/*
 *@typedef ae_af_info_t
 *@brief AF information for AE
 */
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
struct ae_af_info_t {
	uint32 af_state;
	uint32 af_distance;
};
#pragma pack(pop)  /* restore old alignment setting from stack  */

/*
 *@typedef ae_motion_info_t
 *@brief Motion detection information for AE
 */
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
struct ae_motion_info_t {
	uint16  motion_state;    // 0: steady without significant motion, 1: slight motion, 2: heavy motion
	uint32  motion_value_global;   // in screen overall motion
	uint32  motion_value_local;     // in scrren object motion
};
#pragma pack(pop)  /* restore old alignment setting from stack  */

/*
 *@typedef ae_single_expo_set_t
 *@brief Exposure parameter of single node
 */
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
struct ae_single_expo_set_t {
	uint32  ad_gain;
	uint32  isp_dgain;
	uint32  exp_linecount;
	uint32  exp_time;
	uint32  ISO;
	uint32  ExifISO;
};
#pragma pack(pop)  /* restore old alignment setting from stack  */


/*
 *@typedef ae_deubg_info_t
 *@brief debug size and ptr
 */
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
struct ae_deubg_info_t {
	uint32  size;
	void* ptr;
};
#pragma pack(pop)  /* restore old alignment setting from stack  */


#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
struct ae_iq_info_to_isp_t {
	uint8  valid_flg;  /* 0:AE Not Process yet,1:AE already process */
	uint8  reserved[MAX_AE_IQ_INFO_RESERVED_SIZE];  /*reserved 16 bytes*/
};
#pragma pack(pop)  /* restore old alignment setting from stack  */

/*
 *@typedef ae_exposure_param_t
 *@brief Exposure parameter
 */
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
struct ae_exposure_param_t {
	uint32  targetmean;
	uint32  curYmean;

	/* bracket parameter  */
	uint8     valid_exp_num;
	struct ae_single_expo_set_t  bracket_expo[AL_MAX_EXP_ENTRIES];

	/* Single shot */
	struct ae_single_expo_set_t  still_expo;

	/*  preview  */
	struct ae_single_expo_set_t  cur_expo;

	/* initial */
	struct ae_single_expo_set_t  init_expo;

	/* flash off report  */
	struct ae_single_expo_set_t  flashoff_expo;	/* would be reset after calling  set param : AE_SET_PARAM_RESET_FLASH_EST */

	/* flash output report  */			/* would be reset after calling  set param : AE_SET_PARAM_RESET_FLASH_EST */
	int32    isneedflash;	/* 1: need flash, 0: without flash */
	struct flash_control_data_t   preflash_ctrldat;	/* to Framework */
	struct flash_control_data_t   mainflash_ctrldat;	/* to framework */

	int32   bv_val;		/* scale 1000 */
	uint32   ISO;
	enum ae_metering_mode_type_t ae_metering_mode;
};
#pragma pack(pop)  /* restore old alignment setting from stack  */

/*
 *@typedef ae_init_exposure_param_t
 *@brief Exposure parameter of intial value (first time to open camera)
 */
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
struct ae_init_exposure_param_t {
	uint16  ad_gain;	/* scale 100 */
	uint32  isp_dgain;	/* scale 100 */
	uint32  exp_linecount;
	uint32  exp_time;	/* for initial, exposure time is only for reference, exposure line would be more precise  */
	int32   bv_val;		/* scale 1000 */
};
#pragma pack(pop)  /* restore old alignment setting from stack  */

/*
 *@typedef ae_iso_adgain_info_t
 *@brief Translate AD gain vs ISO
 */
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
struct ae_iso_adgain_info_t {
	uint32    ad_gain;	/* scale 100 */
	uint32    ISO;
};
#pragma pack(pop)  /* restore old alignment setting from stack  */

/*
 *@typedef ae_exposure_bracket_param_t
 *@brief bracket expousre parameter
 */
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
struct ae_exposure_bracket_param_t {
	int32    valid_exp_num;
	int32    bracket_evComp[AL_MAX_EXP_ENTRIES];	/* scale 1000, should corresponding to valid_exp_num, max number follow AL_MAX_EXP_ENTRIES define */
};
#pragma pack(pop)  /* restore old alignment setting from stack  */

/*
 *@typedef ae_sof_notify_param_t
 *@brief SOF information
 */
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
struct ae_sof_notify_param_t {
	uint32 sys_sof_index;	 /* current SOF index of AP framework */

	uint32 exp_linecount;
	uint32 exp_time;	/* in us, 1.0s should set 1000000 */
	uint32 exp_adgain;	/* scale 100 */
	int32  ImageBV;		/* this data would be calculated automatically, no need to input by framework */

	uint16 cuFPS;		/* scale 100, ex: 16.0 FPS --> 1600*/
};
#pragma pack(pop)  /* restore old alignment setting from stack  */

/*
 *@typedef ae_isp_dgain_sof_notify_param_t
 *@brief ISP Dgain SOF information
 */
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
struct ae_isp_dgain_sof_notify_param_t {
	uint32 sys_sof_index;	 /* current SOF index of AP framework */
	uint32 isp_dgain;	/* scale 100 */
};
#pragma pack(pop)  /* restore old alignment setting from stack  */

/*
 *@typedef ae_script_param_t
 *@brief script of flash script mode
 */
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
struct ae_script_param_t {
	uint8  bScriptMode;	/* current script mode status, TRUE: turned on, FALSE: turned off*/
	uint32 udmaxscriptcnt;	/* used for framework to know how many loop remains */
	uint32 udcurrentscriptcnt;	/* used for framework to know current loop count, could be used to make indicator of processing bar ( x% completed )*/
	uint8  bisneedflashTurnOn;	/* this would indicate flash turn on request, framework need to turn on flag when set to on (1), if value is 0, means script mode request turn off flash */
	uint32 udcurrentscriptexptime;	/* this value is script recorded exposure time, in us , 1.0s means should set 1000000 us */
	uint32 udcurrentscriptexpline;	/* this value is script recorded exposure line, in line (min exposure line would follow script define, neglect sensor setting) */
	uint32 udcurrentscriptgain;	/* this value is script recorded exposure ad gain ( min 1.0x), 1.0x mean should set 100 */

	/* flash script request parameter */
	uint32 udled1index;
	float fled1current;	/* ex: 87.5 mA*/
	uint32 udled2index;
	float fled2current;	/* ex: 87.5 mA*/
	uint32 udTotalIndex;

	uint8   ucFlashMode;	/* 0: not defined,  1: torch mode, 2: main-flash mode */
	enum ae_script_st_t scriptstate;  /*idle,  running , success done or fail(return error msg)*/
};
#pragma pack(pop)  /* restore old alignment setting from stack  */

/*
 *@typedef ae_script_mode_data_t
 *@brief script of AE script mode
 */
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
struct ae_script_mode_data_t {
	enum ae_script_mode_t  script_mode;
	struct ae_script_param_t  script_param;
};
#pragma pack(pop)  /* restore old alignment setting from stack  */

/*
 *@typedef ae_set_parameter_init_t
 *@brief AE initial setting
 */
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
struct ae_set_parameter_init_t {
	/*  tuning parameter setting  */
	void   *ae_setting_data;	/* ae setting file */

	/* initial UI setting */
	enum al3a_operation_mode  al_3aop_mode;
	enum ae_iso_mode_t  isolevel;
	uint32 udISOSpeed;	/* if ISO level set userdef, could input manually here */
	uint32  ae_scene_mode;	/* refer to ae_scene_mode_t */
	uint32  capture_mode;	/* refer to ae_capture_mode_t */
	enum ae_antiflicker_mode_t  ae_set_antibaning_mode;
	int32 ae_ui_evcomp;	/* +2 ~ -2 EV, scale 1000 */
	int32 ae_3rd_evcomp;	/* +2 ~ -2 EV, scale 1000 */
	enum ae_metering_mode_type_t  ae_metering_mode;		/* initial value suggest use: AE_METERING_INTELLIWT */
	enum al3a_fe_ui_flash_mode  flash_mode;			/* initial value suggest use: AL3A_UI_FLASH_AUTO */

	/*  basic control param  */
	uint8 ae_enable;		/* for initial, should be true */
	uint8 ae_lock;		/* for initial, should be false */

	/*  sensor info setting, RAW size, fps , etc. */
	struct ae_sensor_info_t    preview_sensor_info;		/* normal preview mode setting */
	struct ae_sensor_info_t    capture_sensor_info;		/* if need to use capture mode (use capture exposure table or special mode usage) */

	/* initial AWB info */
	struct ae_awb_info_t    ae_awb_info;

	/* OTP data */
	struct calibration_data_t   ae_calib_wb_gain;			/* if no calibration data is prepared, set {0.0, 0.0, 0.0, 0.0} would let AE lib use its default value
								 * but remember to set OTP data via set_param, refer to command AE_SET_PARAM_OPT_WB_DAT */
	/* Identity ID, used for recognize channel of 3A control for each camera channel */
	uint8  identity_id;       /* default 0, should be assigned by AP framework */

};
#pragma pack(pop)  /* restore old alignment setting from stack  */

/*
 *@typedef ae_engineer_mode_param_t
 *@brief ae engineer mode param
 */
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct ae_engineer_mode_param_t {
	uint32  engineer_type;		 /* see ae_engineer_mode_type_t */
	uint8   ucengineermode;      /* 0:Off,1:on */
};
#pragma pack(pop)  /* restore old alignment setting from stack */


/*
 *@typedef ae_timegain_sync_test_t
 *@brief ae time&gain sync test
 */
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct ae_timegain_sync_test_t {
	uint32 exptime[AL_AE_SENSORTEST_MAXNODE];
	uint32 gain[AL_AE_SENSORTEST_MAXNODE];
	enum ae_sensorctrl_test_flag_t  flag;  /*0:off ,1:auto on,2Manual on*/
	uint8 validnode;
};
#pragma pack(pop)  /* restore old alignment setting from stack */

/*
 *@typedef ae_sceneparam_setting_t
 *@brief scene relation param
 */
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct ae_sceneparam_setting_t {
	int32   scene_ui_evcomp;
	uint8   pcurve_idx;
	uint8   exposure_priority;
	uint32  manual_exptime;
	uint32  manual_gain;
};
#pragma pack(pop)  /* restore old alignment setting from stack */

/*
 *@typedef ae_set_param_content_t
 *@brief parameter body of set param
 */
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
struct ae_set_param_content_t {
	/* initial setting, basic setting related */
	struct ae_set_parameter_init_t  ae_initial_setting;
	struct ae_sensor_info_t    normal_sensor_info;		/* normal mode, preview sensor info */
	struct ae_sensor_info_t    capture_sensor_info;

	struct calibration_data_t   ae_calib_wb_gain;

	void   *ae_setting_data;	/* this data should be parsed via wrapper to seperated AE setting data, check basic alignment and read error, if no error, passing to lib directly */

	/* UI command  */
	enum ae_metering_mode_type_t  ae_metering_mode;
	enum al3a_operation_mode  al_3aop_mode;	/* mode change */

	enum ae_iso_mode_t  isolevel;		/* ISO level, if set AD gain level, ISO level should be set to auto (follow manual AD gain) */
	uint32 manual_iso;			/* user DEF level , user define ISO speed */
	enum ae_gain_mode_t  adgainlevel;	/* AD gain level, if set ISO level, AD gain level should be set to auto ( follow manual ISO) */
	uint32 manual_adgain;			/* manual AD gain, sensor AD gain, scale 100 */
	enum ae_gain_mode_t  ispgainlevel;	/* reserved */
	uint32 manual_ispgain;			/* reserved, manual ISP gain, ISP D gain, scale 100 */
	uint32 exptime;				/* reserved */
	uint32 manual_exptime;			/* manual exposure time, unit: us */

	enum ae_antiflicker_mode_t  ae_set_antibaning_mode;
	uint32 ae_targetmean;
	enum ae_ev_mode_t  aeui_evcomp_level;
	int32 ae_ui_evcomp;	/* +2 ~ -2 EV, scale 1000 */
	int32 ae_3rd_evcomp;	/* +2 ~ -2 EV, scale 1000 */
	uint32 manualFPSmode;	/* reserved item */
	uint32 max_fps;
	uint32 min_fps;

	enum ae_converge_level_type_t   converge_speedlv;

	uint16 uwDigitalZoom;	/* 100: 1.0, 200: scale by 2x, default value 100 if never set digital zoom */

	struct rect_roi_config_t  ae_set_roi_setting;	/* should base on RAW croped domain */
	struct rect_roi_config_t  ae_set_face_roi_setting;	/* should base on RAW croped domain */

	uint8  redeye_mode;	/* reserved, red eye flash */
	enum ae_scene_mode_t  ae_scene_mode;	/* refer to ae_scene_mode_t */
	uint8 ae_enable;
	uint8 ae_lock;

	uint8 ae_enableDebugLog;

	enum ae_capture_mode_t  capture_mode;	/* refer to ae_capture_mode_t */
	uint8  longexp_mode;	/* reserved */
	uint8  hdr_sensor_mode;	/* ture: turned on, false: turned off */

	uint8 uuserdef_meteringtbl[AL_MAX_AE_STATS_NUM];	/* user define metering table */

	/* calibration related */
	enum ae_script_mode_t   script_mode;	/* for calibration used, framework should indicate current calibration mode for AE or for FE */
	void   *ae_script_setting_data;		/* This item may including in ae_setting_data of aec_initial_setting , seperated it for special used  */

	/* capture command  */
	struct ae_exposure_bracket_param_t   ae_bracket_param;

	/* frame based update command  */
	struct ae_sof_notify_param_t  sof_notify_param;		/* current sof notify param, should be updated for each SOF received */
	/* frame based update command  */
	struct ae_isp_dgain_sof_notify_param_t  isp_dgain_sof_notify_param;		/* current sof notify param, should be updated for each SOF received */

	struct ae_gyro_info_t   ae_gyro_info;
	struct ae_awb_info_t    ae_awb_info;
	struct ae_af_info_t     ae_af_info;
	struct ae_motion_info_t  ae_motion_info;

	/* flash-LED related control command */
	enum ae_flash_st_t    ae_flash_st;	/* store flash status */
	enum al3a_fe_ui_flash_mode  flash_mode;	/* UI flash setting mode, forcefill, auto, off, torch, etc. */

	/* advanced control command  */
	uint8  ucforceconvegeswt;		/* 0: normal AE, 1: always report converge in each
						 * AE state (such as prepare, do with flash, nromal AE), switch would be hold untill release via set_parmeter again */

	/*  for dual cam sync command  */
	uint8  us_dual_sync_mode;     /* 0: normal single sensor mode, 1: turn on sync mode, for tagging only */
	struct ae_sensor_info_t    slave_sensor_info;              /* for slave sensor info, used for dual cam sync mode */
	struct ae_sensor_info_t    slave_capture_sensor_info;              /* for slave sensor info, used for dual cam sync mode */
	struct calibration_data_t   ae_calib_wb_gain_slv_sensor;              /* for slave OTP, used for dual cam sync mode */
	struct ae_sof_notify_param_t  sof_notify_param_slv_sensor;   /* current sof notify param, should be updated for each SOF received */

	struct ae_hw_config_t ae_hw_config;  /* reserved,  sensor info, hw3a info */
	struct ae_engineer_mode_param_t ae_engineer_mode_param;
	struct ae_timegain_sync_test_t ae_sensorctrl_test_param;
	struct ae_sceneparam_setting_t ae_scene_param;

	uint8  ae_manual_wtb_enable_flg;    /*True:Set weight table by AP,False:use tuning bin table*/
};
#pragma pack(pop)  /* restore old alignment setting from stack  */

/*
 *@typedef ae_set_param_t
 *@brief parameter body of set param
 */
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
struct ae_set_param_t {
	enum  ae_set_param_type_t  ae_set_param_type;
	uint8  bforceupdateflg;		/* strongerly suggest not use this flag, default suggest : flase, if turn on, this set parameter would directly impact current buffer & shadow buffer
					 * shadow buffer normally would be update once when AE lib estimation/processing API is calling */
	struct ae_set_param_content_t    set_param;
};
#pragma pack(pop)  /* restore old alignment setting from stack  */

/*
 *@typedef ae_get_param_t
 *@brief parameter body of get param
 */
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
struct ae_get_param_t {
	enum ae_get_param_type_t   ae_get_param_type;
	union {
		struct ae_exposure_param_t  ae_get_expo_param;
		struct ae_init_exposure_param_t  ae_get_init_expo_param;
		struct alhw3a_ae_cfginfo_t  alhw3a_aeconfig;
		struct calibration_data_t   calib_data;
		struct wbgain_data_t        wb_data;
		struct ae_script_mode_data_t  curscriptmodedata;
		uint8   ucdualledstats;		/* 0: undefined, 1: single LED, 2: dual LED */
		uint32  udadgainx100periso100gain;	/* if value = 100 , mean ISO 100 = 100 (AD gain = 1.0x ), if value = 200, mean ISO 100 = 200 (AD gain = 2.0x) */
		struct ae_iso_adgain_info_t   iso_adgain_info;
		struct ae_deubg_info_t commom_exif;
		struct ae_deubg_info_t full_debug;
		struct ae_iq_info_to_isp_t ae_iq_info;
	} para;
};
#pragma pack(pop)  /* restore old alignment setting from stack  */

/*
 *@typedef ae_config_info_t
 *@brief parameter body of set param command for HW info config
 */
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
struct ae_config_info_t {
	struct ae_hw_config_t  ae_hw_config_set;	/* sensor, HW3A config info */
	void *ae_initial_set_param;		/* initial setting */
} ;
#pragma pack(pop)  /* restore old alignment setting from stack  */

/*
 *@typedef ae_output_data_t
 *@brief parameter body of AE output result
 */
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
struct ae_output_data_t {
	struct report_update_t rpt_3a_update;   // store each module report update result

	/* common stats info  */
	uint32 hw3a_frame_id;		/* frame ID when HW3a updated */
	uint32 sys_sof_index;		/* current SOF ID, should be maintained by framework */
	uint32 hw3a_ae_block_nums;	/* total blocks */
	uint32 hw3a_ae_block_totalpixels;	/* total pixels in each blocks */
	uint8  hw3a_ae_block_w;		/*block width*/
	uint8  hw3a_ae_block_h;		/*block height*/
	void*  pthw3a_ae_stats;		/* store HW3A Y data */
	int32  hw3a_cur_mask[AL_MAX_AE_STATS_NUM];	/* store HW3A running mask of AE */

	/* AE runing status */
	uint32  ae_targetmean;
	uint32  ae_default_target;		/*from tuning param or set cmd param*/
	uint32  ae_cur_mean;
	uint32  ae_nonwt_mean;		/* average mean */
	uint32  ae_center_mean2x2;	/* center 2x2 mean */
	int32   bvresult;		/* index for current brightness level */
	int32   bg_bvresult;		/* BV value from average mean, updated even at AE locked (not update when AE disabled) */
	INT16   ae_converged;
	uint8   ae_need_flash_flg;	/* flash needed flag */

	enum ae_state_t  ae_est_status;	/* mapping to ae_LibStates */
	enum fe_state_t  fe_est_status;	/* mapping to ae_FlashStates */

	/* AE current exposure result, From SOF notify */
	uint32  udCur_ISO;		/* current SOF ISO speed */
	uint32  udCur_exposure_time;	/* current SOF exposure time */
	uint32  udCur_exposure_line;	/* current SOF exposure line */
	uint32  udCur_sensor_ad_gain;	/* current SOF AD gain */
	uint32  udCur_ISP_D_gain;	/*  reserved, for stats from ISP platform used  */
	uint16  uwCur_fps;		/* current FPS, from framework control information */

	/* Next AE exposure parameter */
	uint32  udISO;			/* current ISO speed */
	uint32  udExifISO;			/* EXIF ISO speed */
	uint32  udexposure_time;
	uint32  udexposure_line;
	uint32  udsensor_ad_gain;
	uint32  udISP_D_gain;		/*  reserved, for stats from ISP platform used  */
	int32   bv_delta;
	int32   ae_non_comp_bv_val;	/* current non compensated BV value, would be refered by AWB */
	int32   ae_nextBV;

	struct exposure_data_t    flash_off_exp_dat;
	struct exposure_data_t    snapshot_exp_dat;
	struct exposure_data_t    bracket_HDR_exp_dat[AL_MAX_EXP_ENTRIES];

	/* AE control status */
	INT16  ae_roi_change_st;	/* 0 : no response to ROI change, 1: ROI changed and taken, this reset should be performed via framework once framework ack this message status */
	enum ae_antiflicker_mode_t    flicker_mode;
	enum ae_metering_mode_type_t  ae_metering_mode;		/* AE metering mode */
	enum ae_script_mode_t  ae_script_mode;			/* if script mode is turned on, framework should switch to specical flow, monitor info changing of ae_script_info */
	uint16 DigitalZoom;		/* store current valid digital zoom */

	/*  hdr control */
	uint32  hdr_exp_ratio;		/* sacle by 100, control HDR sensor long/short exposure ratio */
	uint32  hdr_ae_long_exp_line;	/* to HDR sensor */
	uint32  hdr_ae_long_exp_time;	/* appplication */
	uint32  hdr_ae_long_ad_gain;	/* to HDR sensor */
	uint32  hdr_ae_short_exp_line;	/* to HDR sensor */
	uint32  hdr_ae_short_exp_time;	/* application */
	uint32  hdr_ae_short_ad_gain;	/* to HDR sensor */

	/* UI control parameter  */
	int32  ui_EV_comp;
	int32  ae_3rd_comp;

	/* AE Compensation */
	int32  bv_comp;
	int32  intae_comp;

	/* misc info */
	uint32    ae_extreme_green_cnt;
	uint32    ae_extreme_blue_cnt;
	uint32    ae_extreme_color_cnt;

	/* Exif reference */
	float  ae_Exif_Bv;
	float  ae_Exif_Av;
	float  ae_Exif_Tv;
	float  ae_Exif_Sv;
	struct ae_config_info_t  ae_config_info_param;

	/* flash reprot */
	struct flash_report_data_t      preflash_report;	/* to AWB */
	struct flash_report_data_t      mainflash_report;	/* to AWB */

	struct flash_control_data_t   preflash_ctrldat;	/* to Framework */
	struct flash_control_data_t   mainflash_ctrldat;	/* to Framework */

	uint16  ae_debug_valid_size;
	uint8*  ptae_debug_data;	/* refer to ae_report_update_t-->ae_debug_data */

	struct calibration_data_t  general_calib_data;
	uint32 udadgainx100periso100gain;	/* store ISO speed --> AD gain relation, ex: 100 --> ISO100 use AD gain 1.0 */

	/* request from 3A libs  */
	uint8   ae_request_HW3A_cfg;		/* if this status = true, AE Ctrl layer should call config wrapper again for setting */

	/* script mode related info/stats  */
	struct ae_script_param_t  ae_script_info;
};
#pragma pack(pop)  /* restore old alignment setting from stack  */



#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct ae_match_runtime_data_t {
	uint32  ISO;		              /* ISO speed */
	uint32  EXIF_ISO;              /*  Exif ISO speed */
	uint32  exposure_time;	/* exposure time */
	uint32  exposure_line;	/* exposure line */
	uint32  sensor_ad_gain;	/* AD gain */
	uint32  ISP_D_gain;	       /* reserved, for stats from ISP platform used  */
	uint16  uwCur_fps;		/* current FPS, from framework control information, copying from master camera module */
	int32    bv_val;                   /* current BV value, copy from master camera module */

	uint8    uc_sensor_mode;      /* 0: normal preview (master/slave), 1: capture mode ( master/slave), AP should indicate correct mode to get correct transform result  */
	uint8    uc_is_binning;         /* 0: FR, no binning, crop, 1: BIN mode */

};
#pragma pack(pop)  /* restore old alignment setting from stack */


/*
 *@typedef alaelib_version_t
 *@brief AE lib version number
 */
struct alaelib_version_t {
	uint16 major_version;
	uint16 minor_version;
} ;

/*
 *\brief get AE lib version number
 *\param AE_LibVersion [out], AE lib version number
 */
void allib_ae_getlib_version( struct alaelib_version_t* AE_LibVersion );

/* public function declaration */
typedef void (* allib_ae_intial )( void ** ae_init_buffer );
typedef void (* allib_ae_deinit )( void * ae_obj );
typedef uint32 (* allib_ae_set_param )( struct ae_set_param_t *param, struct ae_output_data_t *ae_output , void *ae_runtime_dat );
typedef uint32 (* allib_ae_get_param )( struct ae_get_param_t *param, void *ae_runtime_dat );
typedef uint32 (* allib_ae_estimation )( void * hw3a_stats_data, void *ae_runtime_dat, struct ae_output_data_t *ae_output );
typedef uint32 (* allib_ae_match_func )( struct ae_match_runtime_data_t *master_dat, void *ae_runtime_dat, struct ae_match_runtime_data_t *slave_dat );

/*
 *@typedef alaeruntimeobj_t
 *@brief AE lib function object (APIs address)
 */
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
struct alaeruntimeobj_t {
	pthread_mutex_t mutex_obj;
	uint32  identityid;
	void                 *ae;		/* runtime working buffer */
	allib_ae_intial      initial;
	allib_ae_deinit      deinit;
	allib_ae_set_param   set_param;
	allib_ae_get_param   get_param;
	allib_ae_estimation  estimation;
	allib_ae_match_func  sync_process;
};
#pragma pack(pop)  /* restore old alignment setting from stack  */

/*
 *\brief get AE lib API address
 *\param aec_run_obj [out], object of AE lib APIs address
 *\param identityID  [in],  framework tag for current instance
 *\return value , TRUE: loading with no error , FALSE: false loading function APIs address
 */
uint8 allib_ae_loadfunc( struct alaeruntimeobj_t *aec_run_obj, uint32 identityID );	/* return value true : success, flase: with error */

#ifdef __cplusplus
}  // extern "C"
#endif

#endif // _AL_AE_H_
