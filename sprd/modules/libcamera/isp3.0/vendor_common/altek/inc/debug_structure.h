/******************************************************************************
 *  File name: debug_structure.h
 *  Latest Update Date:2017/7/17
 *
 *  Comment:
 *  Describe structure definition of debug information
 *****************************************************************************/
#include "mtype.h"

#define DEBUG_STRUCT_VERSION               (508)

#define MAX_AEFE_DEBUG_SIZE_STRUCT1        (781)
#define MAX_AEFE_DEBUG_SIZE_STRUCT2        (1109)
#define MAX_FLICKER_DEBUG_SIZE_STRUCT1     (20)
#define MAX_FLICKER_DEBUG_SIZE_STRUCT2     (1296)
#define MAX_AWB_DEBUG_SIZE_STRUCT1         (438)
#define MAX_AWB_DEBUG_SIZE_STRUCT2         (10240)
#define MAX_AF_DEBUG_SIZE_STRUCT1          (2060)
#define MAX_AF_DEBUG_SIZE_STRUCT2          (15500)
#define MAX_BIN1_DEBUG_SIZE_STRUCT2        (30000)
#define MAX_BIN2_DEBUG_SIZE_STRUCT2        (1137144)
#define START_STR_SIZE                     (32)
#define END_STR_SIZE                       (24)
#define PROJECT_NAME_SIZE                  (16)
#define OTP_LSC_DATA_SIZE                  (1658)
#define OTP_CURRENT_MODULE_RESERVED1       (10)
#define OTP_CURRENT_MODULE_RESERVED2       (8)
#define OTP_CURRENT_MODULE_MASTER_CHIP_ID  (16)
#define SHADING_DEBUG_VERSION_NUMBER       (12)
#define SHADING_DEBUG_RESERVED_SIZE        (18)
#define IRP_TUNING_DEBUG_IDX               (3)
#define IRP_TUNING_DEBUG_INFO              (6)
#define IRP_TUNING_DEBUG_BYPASS            (5)
#define IRP_TUNING_DEBUG_CCM               (9)
#define IRP_TUNING_DEBUG_PARA_ADDR         (56)
#define IRP_TUNING_DEBUG_RESERVED          (5)
#define IRP_TUNING_DEBUG_BVTH              (4)
#define MAX_IRP_GAMMA_TONE_SIZE            (1027)
#define ISP_SW_DEBUG_RESERVED_SIZE         (59)
#define OTHER_DEBUG_RESERVED_SIZE          (11)
#define DEBUG_INFO1_RESERVED_SIZE          (109)
enum e_scinfo_color_order {
	E_SCINFO_COLOR_ORDER_RG = 0,
	E_SCINFO_COLOR_ORDER_GR,
	E_SCINFO_COLOR_ORDER_GB,
	E_SCINFO_COLOR_ORDER_BG
};

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
struct otp_report_debug1 {

	uint8 current_module_calistatus;  /* 00:fail, 01:success */
	uint8 current_module_year;  /* eg: 2015 0x0f */
	uint8 current_module_month;  /* eg: jan 0x01 */
	uint8 current_module_day;  /* eg: 1st 0x01 */
	uint8 current_module_module_house_id;  /* 0x02(truly) */
	uint8 current_module_lens_id;
	uint8 current_module_vcm_id;
	uint8 current_module_driver_ic;
	uint16 current_module_iso;  /* miniso */
	uint16 current_module_r_gain;  /* iso_gain_r */
	uint16 current_module_g_gain;  /* iso_gain_g */
	uint16 current_module_b_gain;  /* iso_gain_b */
	uint16 current_module_infinity;
	uint16 current_module_macro;
	uint8 current_module_af_flag;  /* 00:fail, 01:success */
	uint8 current_module_version;
	uint16 current_master_module_iso;
	uint16 current_master_module_r_gain;
	uint16 current_master_module_g_gain;
	uint16 current_master_module_b_gain;
	uint16 current_slave_module_iso;
	uint16 current_slave_module_r_gain;
	uint16 current_slave_module_g_gain;
	uint16 current_slave_module_b_gain;
	uint8 current_module_master_chip_id[PROJECT_NAME_SIZE];
	uint16 current_module_project_id;
	uint8 current_module_eeprom_map_ver;
	uint8 current_module_reserved[OTP_CURRENT_MODULE_RESERVED1];  /* Set 0x00 */
	uint8 total_check_sum;
};
#pragma pack(pop)  /* restore old alignment setting from stack  */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
struct otp_report_debug2 {
	uint8 current_module_calistatus;  /* 00:fail, 01:success */
	uint8 current_module_year;  /* eg: 2015 0x0f */
	uint8 current_module_month;  /* eg: jan 0x01 */
	uint8 current_module_day;  /* eg: 1st 0x01 */
	uint8 current_module_module_house_id;  /* 0x02(truly) */
	uint8 current_module_lens_id;
	uint8 current_module_vcm_id;
	uint8 current_module_driver_ic_id;
	uint16 current_module_iso;  /* miniso */
	uint16 current_module_r_gain;  /* iso_gain_r */
	uint16 current_module_g_gain;  /* iso_gain_g */
	uint16 current_module_b_gain;  /* iso_gain_b */
	uint8 current_module_lsc[OTP_LSC_DATA_SIZE];  /* Set 0x00 */
	uint16 current_module_infinity;
	uint16 current_module_macro;
	uint8 current_module_af_flag;  /* 00:fail, 01:success */
	uint8 current_module_version;
	uint16 current_master_module_iso;
	uint16 current_master_module_r_gain;
	uint16 current_master_module_g_gain;
	uint16 current_master_module_b_gain;
	uint8 current_master_module_lsc[OTP_LSC_DATA_SIZE];  /* Set 0x00 */
	uint16 current_slave_module_iso;
	uint16 current_slave_module_r_gain;
	uint16 current_slave_module_g_gain;
	uint16 current_slave_module_b_gain;
	uint8 current_slave_module_lsc[OTP_LSC_DATA_SIZE];  /* Set 0x00 */
	uint8 current_module_master_chip_id[OTP_CURRENT_MODULE_MASTER_CHIP_ID];  /* Set 0x00 */
	uint16 current_module_project_id;
	uint8 current_module_eeprom_map_ver;
	uint8 current_module_reserved[OTP_CURRENT_MODULE_RESERVED2];  /* Set 0x00 */
	uint8 total_check_sum;  /* LSC Data */
};
#pragma pack(pop)  /* restore old alignment setting from stack  */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
struct raw_img_debug {
	uint16 raw_width;  /* CFA Width */
	uint16 raw_height;  /* CFA Height */
	uint8 raw_format;  /* altek RAW10, android RAW10, packed10, unpacted10 */
	uint8 raw_bit_number;  /* 8bit or 10 bit */
	uint8 mirror_flip;  /* no, mirror, flip or mirror+flip */
	enum e_scinfo_color_order raw_color_order;  /* BGGR */
};
#pragma pack(pop)  /* restore old alignment setting from stack  */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
struct shading_debug {
	uint16 uwbalancedawbgain_r;  /* (10-bit), 1x: 256, balanced wb gain for shading */
	uint16 uwbalancedawbgain_g;  /* (10-bit), 1x: 256, balanced wb gain for shading */
	uint16 uwbalancedawbgain_b;  /* (10-bit), 1x: 256, balanced wb gain for shading */
	uint8 orientation;
	uint8 ucsensorid;  /* rear sensor */
	uint8 ucotpstatus;
	uint8 otpversion;
	uint16 shading_width;
	uint16 shading_height;
	uint16 hrgain;
	uint16 mrgain;
	uint16 lrgain;
	uint16 currentrgain;
	uint16 currentbgain;
	uint16 currentprojx;
	uint16 currentprojy;
	uint8 rprun;
	uint8 bprun;
	uint8 ext_ucorientation;
	uint8 ext_ucsensorid;
	uint16 ext_hrgain;  /* front sensor */
	uint16 ext_mrgain;  /* front sensor */
	uint16 ext_lrgain;  /* front sensor */
	uint16 ext_currentprojx;  /* front sensor */
	uint16 ext_currentprojy;  /* front sensor */
	uint8 ext_otpstatus;  /* front sensor */
	uint8 ext_rprun;  /* front sensor */
	uint8 ext_bprun;  /* front sensor */
	uint8 ucshadtableflag;
	uint8 versionnumber[SHADING_DEBUG_VERSION_NUMBER];  /* The version of ShadingInfo_XXX.bin (created date+time. ex: 201601122130) */
	uint32 udtuningversion;  /* the version of shading fine-tune data (user define) */
	uint16 uwisostep;
	uint8 reserved[SHADING_DEBUG_RESERVED_SIZE];
};
#pragma pack(pop)  /* restore old alignment setting from stack  */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
struct irp_tuning_debug {

	uint32 udqmerge_ver;
	uint32 udtool_ver;
	uint32 udtuning_ver;
	uint8 aucidx[IRP_TUNING_DEBUG_IDX];  /* ISP output preview, video and Still three scenario at the same time */
	uint8 ucsensorid;	/* sensor: 0 (rear 0)
				                 * sensor: 1 (rear1)
				                 * lite: 2 (front, no ycc nre)
				                 */
	uint16 uwsensortype;  /* user definition by sensor vendor/type serial number */
	uint8 ucsensormode;  /* for fr, video/preview size, depend on projects; default (fr + preview) */
	uint8 ucqualitypath;  /* normal quallity ;high quality */
	uint16 uwbayerscalarw;  /* bayer scalar width */
	uint16 uwbayerscalarh;  /* bayer scalar height */
	uint16 uwisp_outputw;  /* isp output width */
	uint16 uwisp_outputh;  /* bayer scalar height */
	uint8 abverifydebug[IRP_TUNING_DEBUG_INFO];  /* verify flag;HDR flag;RDN flag;NRE flag;Asharp flag;GEG flag */
	uint8 abfuncbypass[IRP_TUNING_DEBUG_BYPASS];  /* Function Bypass flag  */
	uint16 uwawbgain_r;  /* (10-bit), 1x: 256, wb gain for final result */
	uint16 uwawbgain_g;  /* (10-bit), 1x: 256, wb gain for final result */
	uint16 uwawbgain_b;  /* (10-bit), 1x: 256, wb gain for final result */
	uint16 uwblackoffset_r;  /* (10-bit) */
	uint16 uwblackoffset_g;  /* (10-bit) */
	uint16 uwblackoffset_b;  /* (10-bit) */
	uint16 uwisospeed;  /* iso speed for ss flow, but not used now */
	uint32 udcolortempature;
	int16 dae_bv; /*bv value for ss flow, but not used now*/
	int16 adbv_th_tone[IRP_TUNING_DEBUG_BVTH];  /* Tone table switch point by BV value */
	int16 adbv_th_ccm[IRP_TUNING_DEBUG_BVTH];  /* CCM switch point by BV value */
	int16 adbv_th_3dlut[IRP_TUNING_DEBUG_BVTH];  /* 3DLUT switch point by BV value */
	int16 adccm[IRP_TUNING_DEBUG_CCM];  /* ccm, 1x: 256 */
	uint32 audparaaddr[IRP_TUNING_DEBUG_PARA_ADDR];  /* every parameter ID address */
	uint32 audreserved[IRP_TUNING_DEBUG_RESERVED];  /* Reserved */
};
#pragma pack(pop)  /* restore old alignment setting from stack  */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
struct irp_gamma_tone {
	uint16 gamma_tone[MAX_IRP_GAMMA_TONE_SIZE];  /* 1. Generate / Tuning CCM to fit the target image color reproduction
						  * 2. Advanced tuning to achieve target camera style.
						  */
};
#pragma pack(pop)  /* restore old alignment setting from stack  */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
struct isp_sw_debug {
	uint8 ucSharpnessLevel;  /* UI: Sharpness, ie. -2 ~0(default)~ 2, step 0.5 */
	uint8 ucSaturationLevel;  /* UI: Saturation, ie. -2 ~0(default)~ 2, step 0.5 */
	uint8 ucContrastLevel;  /* UI: Contrast, ie. -2 ~0(default)~ 2, step 0.5 */
	uint8 ucBrightnessValue;  /* UI: Brightness, ie. -2 ~0(default)~ 2, step 0.5 */
	uint8 ucSpecialEffect;	/* UI: Special effect, ie. 0:Off (default),Blue point,
				 * Red-yellow point,Green point,Solarise,Posterise,Washed out,
				 * Warm vintage,Cold vintage,Black & white,Negative
				 */
	uint8 aucReserver[ISP_SW_DEBUG_RESERVED_SIZE];  /* Reserved */
};
#pragma pack(pop)  /* restore old alignment setting from stack  */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
struct other_debug {
	uint32 valid_ad_gain;  /* (scale 100) Valid exposure information */
	uint32 valid_exposure_time;  /* (Unit: us) Valid exposure information */
	uint32 valid_exposure_line;  /* Valid exposure information */
	uint32 fn_value;  /* Lens F-number(scale 1000), Fno 2.0 = 2000 */
	uint32 focal_length;  /* The focal length of lens, (scale 1000) 3097 => 3.097 mm */
	uint8 flash_flag;  /* 0: off, 1: preflash on, 2: mainflash on */
	uint8 sync_mode;   /*  0: off, 1: turn on */
	uint8 mirror;   /*  0: off, 1: turn on */
	uint8 orientation;  /* jpg orientation 0 : Horizontal_0(normal), 1 : Rotate_90_CW , 2 : Rotate_180_CW , 3 : Rotate_270_CW */
	uint8 reserved[OTHER_DEBUG_RESERVED_SIZE];  /* Reserved */
};
#pragma pack(pop)  /* restore old alignment setting from stack  */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
struct debug_info1 {
	int8 string1[START_STR_SIZE];/* ie. exif_str_g2v1 */
	int32 struct_version;
	uint16 main_ver_major;  /* 3ALib. Major Version */
	uint16 main_ver_minor;  /* 3ALib. Minor Version */
	uint16 isp_ver_major;
	uint16 isp_ver_minor;
	uint32 structure_size1;
	uint8 ae_fe_debug_info1[MAX_AEFE_DEBUG_SIZE_STRUCT1];	/* Update @ each report update
								         * ae_output_data_t -> rpt_3a_update -> ae_update-> ae_commonexif_data
								         * Real size:  ae_output_data_t->rpt_3a_update->ae_update->ae_commonexif_valid_size
								         */
	uint8 flicker_debug_info1[MAX_FLICKER_DEBUG_SIZE_STRUCT1];	/* Update @ each report update
									* flicker_output_data_t -> rpt_update -> flicker_update-> flicker_debug_info1(Exif data)
									* Real size:  flicker_output_data_t -> rpt_update -> flicker_update->flicker_debug_info1_valid_size
									*/
	uint8 awb_debug_info1[MAX_AWB_DEBUG_SIZE_STRUCT1];	/* Update @ each report update
								         * AWB debug_info(Exif data)  is ?œawb_debug_data_array??in the structure ?œalAWBLib_output_data_t??from alAWBRuntimeObj_t-> estimation
								         */
	uint8 af_debug_info1[MAX_AF_DEBUG_SIZE_STRUCT1];	/* Collect @ finish focus
								     * alAFLib_output_report_t->alAF_EXIF_data
								     * alAFLib_output_report_t->alAF_EXIF_data_size
								     */

	struct otp_report_debug1 otp_report_debug_info1;
	struct raw_img_debug raw_debug_info1;	/* Sensor/ISP, included into S_ISP_DEBUG_INFO of ISP API: u32 ISPDrv_GetOutputBuffer(u8 a_ucSensor,
						                     * u8 a_ucOutputImage, u32 a_pudOutputBuffer, S_SCENARIO_SIZE a_ptCropSize, S_ISP_DEBUG_INFO * a_ptIspDebugInfo)
						                     */
	struct shading_debug shading_debug_info1;	/* Included into S_ISP_DEBUG_INFO of ISP API: u32 ISPDrv_GetOutputBuffer(u8 a_ucSensor, u8 a_ucOutputImage,
							                     * u32 a_pudOutputBuffer, S_SCENARIO_SIZE a_ptCropSize, S_ISP_DEBUG_INFO * a_ptIspDebugInfo)
							                     */
	struct irp_tuning_debug irp_tuning_para_debug_info1;	/* Included into S_ISP_DEBUG_INFO of ISP API: u32 ISPDrv_GetOutputBuffer(u8 a_ucSensor, u8 a_ucOutputImage,
								                             * u32 a_pudOutputBuffer, S_SCENARIO_SIZE a_ptCropSize, S_ISP_DEBUG_INFO * a_ptIspDebugInfo)
								                             */
	struct isp_sw_debug sw_debug1;	/* Included into S_ISP_DEBUG_INFO of ISP API: u32 ISPDrv_GetOutputBuffer(u8 a_ucSensor,
					                 * u8 a_ucOutputImage, u32 a_pudOutputBuffer, S_SCENARIO_SIZE a_ptCropSize, S_ISP_DEBUG_INFO * a_ptIspDebugInfo)
					                 */
	struct other_debug other_debug_info1;
	int8 reserved[DEBUG_INFO1_RESERVED_SIZE];   /* Reserved =4096 - total size*/
	int8 end_string[END_STR_SIZE];  /* ie. end_exif_str_g2v1 */
};
#pragma pack(pop)  /* restore old alignment setting from stack  */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
struct debug_info2 {

	int8 string2[START_STR_SIZE];  /* ie. jpeg_str_g2v1 */
	uint32 structure_size2;

	uint8 ae_fe_debug_info2[MAX_AEFE_DEBUG_SIZE_STRUCT2];	/* Update @ each report update
								         * ae_output_data_t->rpt_3a_update->ae_update->ae_debug_data
								         * Real size: ae_output_data_t->rpt_3a_update->ae_update->ae_debug_valid_size
								         */

	uint8 flicker_debug_info2[MAX_FLICKER_DEBUG_SIZE_STRUCT2];	/* Update @ each report update
									* flicker_output_data_t -> rpt_update -> flicker_update-> flicker_debug_info1(Exif data)
									* Real size:  flicker_output_data_t -> rpt_update -> flicker_update->flicker_debug_info1_valid_size
									*/

	uint8 awb_debug_info2[MAX_AWB_DEBUG_SIZE_STRUCT2];	/* Update @ each report update
								         * AWB debug_info (Structure 2)  is ??awb_debug_data_full??in the structure ?œalAWBLib_output_data_t??from alAWBRuntimeObj_t-> estimation
								         */

	uint8 af_debug_info2[MAX_AF_DEBUG_SIZE_STRUCT2];	/* Collect @ finish focus
								     * alAFLib_output_report_t->alaf_debug_data
								     * alAFLib_output_report_t->alAF_debug_data_size
								     */
	struct otp_report_debug2 otp_report_debug_info2;
	struct irp_gamma_tone irp_tuning_para_debug_info2;	/* Included into S_ISP_DEBUG_INFO of ISP API: u32 ISPDrv_GetOutputBuffer(u8 a_ucSensor,
								                         * u8 a_ucOutputImage, u32 a_pudOutputBuffer, S_SCENARIO_SIZE a_ptCropSize, S_ISP_DEBUG_INFO * a_ptIspDebugInfo)
								                         */

	uint8 bin_file1[MAX_BIN1_DEBUG_SIZE_STRUCT2];	/* This is merged table data from tuning tool
							     * Include  AE Bin + AF Bin + AWB Bin File.
							     * Also refer to porting document
							     */

	uint8 bin_file2[MAX_BIN2_DEBUG_SIZE_STRUCT2];	/* This is merged table data from tuning tool
							     * Include  Shading + IRP Bin File.
							     * Also refer to porting document
							     */

	int8 end_string[END_STR_SIZE];  /* ie. end_jpeg_str_g2v1 */
};
#pragma pack(pop)  /* restore old alignment setting from stack  */
