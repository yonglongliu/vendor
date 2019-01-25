/******************************************************************************
*  File name: alwrapper_common.h
*  Create Date:
*
*  Comment:
*  Common wrapper, handle outside 3A behavior, such ae seperate bin file
 ******************************************************************************/

#include "mtype.h"  /* same as altek 3A lib released */
#include "alwrapper_scene_setting.h"

#define _WRAPPER_COMM_VER 0.8070

#define WRAPPER_COMM_BIN_NUMBER                 (9)
#define WRAPPER_COMM_BIN_PROJECT_STRLENGTH      (32)

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct header_info {
	int8 cbintag[20];
	uint32 uwtotalsize;
	uint32 uwversion1;
	uint32 uwversion2;
	uint32 uwlocation1;
	uint32 uwlocation2;
	uint32 uwlocation3;
};
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct bin1_sep_info {
	uint32 uw_ae_bin_size;
	uint8* puc_ae_bin_addr;
	uint32 uw_awb_bin_size;
	uint8* puc_awb_bin_addr;
	uint32 uw_af_bin_size;
	uint8* puc_af_bin_addr;

	/* Not used right now, reserved it for furture use */
	uint32 uw_flicker_bin_size;
	uint8* puc_flicker_bin_addr;
};
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct bin2_sep_info {
	uint32 uw_shading_bin_size;
	uint8* puc_shading_bin_addr;
	uint32 uw_irp_bin_size;
	uint8* puc_irp_bin_addr;
};
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct bin3_sep_info {
	uint32 uw_caf_bin_size;
	uint8* puc_caf_bin_addr;

	uint32 uw_pdaf_bin_size;
	uint8* puc_pdaf_bin_addr;

	uint32 uw_scene_bin_size;
	uint8* puc_scene_bin_addr;
};
#pragma pack(pop)  /* restore old alignment setting from stack */

enum onebin_function_t {
	AF = 0,
	AE,
	Flicker,
	SHADING,
	AWB,
	CAF,
	IRP,
	SpecialEffect,
	TONE,
	CCM,
	LUT,
	Scene_Setting,
	PDAF,
	MAX,
};

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(1)    /* new alignment setting */
struct functionbin_header_info {
	uint32 HeaderSize;
	uint32 TotalSize;
	uint32 MultiTuning;
	uint32 Index_Table_Number;
	uint32 Index_Table_Offset;
	uint32 Bin_Number;
	uint32 Bin_List_Offset;
	uint32 Bin_Size_Table_Offset;
	uint32 Bin_FileName_Table_Offset;
	uint32 Bin_FileName_Array_Length;
	uint32 Bin_Start;
	uint32 Project;
};
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(1)    /* new alignment setting */
struct onebin_header_info {
	uint32 HeaderSize;
	uint32 TotalSize;
	uint32 TotalBinNum;
	uint32 Current_Bin_Num;
	uint32 Multi_Tuning;

	uint32 IDX_Number;
	uint32 IDX_Info_Offset;
	uint32 IDX_Name_Offset;
	uint32 FunctionBin_Table_Number;
	uint32 FunctionBin_Type_Offset;
	uint32 FunctionBin_Size_Offset;
	uint32 FunctionBin_Table_Offset;
	uint32 Bin_Start;
	uint32 Project;

	uint32 Prj_StrLength;
	int8 Prj_ProjectName[WRAPPER_COMM_BIN_PROJECT_STRLENGTH];
	int8 Prj_SensorName[WRAPPER_COMM_BIN_PROJECT_STRLENGTH];
	int8 Prj_Assembly[WRAPPER_COMM_BIN_PROJECT_STRLENGTH];
	int8 Prj_Date[WRAPPER_COMM_BIN_PROJECT_STRLENGTH];
	int8 Prj_Tuner[WRAPPER_COMM_BIN_PROJECT_STRLENGTH];

	uint32 Tuning_Version;
	uint32 Version_Major;
	uint32 Version_Minor;
};
#pragma pack(pop)  /* restore old alignment setting from stack */

/*
 * brief Separate 3ABin
 * param a_pc3ABinAdd [IN], pointer of 3A Bin
 * param a_pcAEAdd     [OUT], location pointer of AE bin
 * param a_pcAFAdd     [OUT], location pointer of AF bin
 * param a_pcAWBAdd    [OUT], location pointer of AWB bin
 * return None
 */
void al3awrapper_com_separate_3a_bin(uint8* a_pc3abinadd, uint8** a_pcaeadd, uint8** a_pcafadd, uint8** a_pcawbadd);

/*
 * brief Separate ShadongIRPBin
 * param a_pcShadingIRPBinAdd [IN], pointer of ShadingIRP Bin
 * param a_pcShadingAdd      [OUT], location pointer of Shading bin
 * param a_pcIRPAdd          [OUT], location pointer of IRP bin
 * return None
 */
void al3awrapper_com_separate_shadingirp_bin(uint8* a_pcshadingirpbinadd, uint8** a_pcshadingadd, uint8** a_pcirpadd);


/*
 * brief Separate 3ABin type2 API, add return size info
 * param a_pc3ABinAdd [IN], pointer of 3A Bin
 * param uw_bin_size  [IN], total bin size of 3A Bin file
 * param a_bin1_info [OUT], seperated bin information (including addr, size)
 * return None
 */
uint32 al3awrapper_com_separate_3a_bin_type2(uint8* a_pc3abinadd, uint32 uw_bin_size , struct bin1_sep_info* a_bin1_info );

/*
 * brief Separate ShadongIRPBin type2 API, add return size info
 * param a_pcshadingirpbinadd [IN], pointer of 3A Bin
 * param uw_bin_size  [IN], total bin size of 3A Bin file
 * param a_bin2_info [OUT], seperated bin information (including addr, size)
 * return None
 */
uint32 al3awrapper_com_separate_shadingirp_bin_type2(uint8* a_pcshadingirpbinadd, uint32 uw_bin_size , struct bin2_sep_info* a_bin2_info );

/*
 * API name: al3awrapper_com_getversion
 * This API would return labeled version of wrapper
 * fWrapVersion[out], return current wapper version
 * return: error code
 */
UINT32 al3awrapper_com_getversion( float *fwrapversion );

/*
 * brief Separate one bin API to structure bin1_sep_info & bin2_sep_info
 * param a_pcbinadd [IN], pointer of bin addr
 * param uw_bin_size  [IN], total bin size of 3A Bin file
 * param uw_IDX_number [IN], current IDX
 * param a_bin1_info [OUT], seperated bin information (including addr, size)
 * param a_bin2_info [OUT], seperated bin information (including addr, size)
 * return None
 */
uint32   al3awrapper_com_separate_one_bin(uint8* a_pcbinadd, uint32 uw_bin_size, uint32 uw_IDX_number, struct bin1_sep_info* a_bin1_info, struct bin2_sep_info* a_bin2_info, struct bin3_sep_info* a_bin3_info);

/*
 * API name: al3awrapper_com_separate_scenesetting
 * This API is used to obtain scene setting info from scene setting file
 * param scene[in]: scene mode
 * param setting_file[in]: file address pointer to scene setting file w/ IDX
 * param scene_info[out]: output scene setting info, including scene mode and addr
 * return: error code
 */
uint32   al3awrapper_com_separate_scenesetting(enum scene_setting_list_t scene, void *setting_file, struct scene_setting_info *scene_info);

