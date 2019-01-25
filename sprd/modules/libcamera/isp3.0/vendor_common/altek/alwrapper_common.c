/******************************************************************************
 * alwrapper_common.c
 *
 *  Created on: 2016/03/18
 *      Author: FishHuang
 *  Latest update:
 *      Reviser:
 *  Comments:
 *       This c file is mainly used for AP framework to:
 *       1. dispatch common HW, such as IRP(ISP)
 *       2. dispatch bin file from tuning tool
 ******************************************************************************/

#include <string.h>
#include "mtype.h"

#include "alwrapper_common.h"
#include "alwrapper_common_errcode.h"
#include "alwrapper_scene_setting.h"

/******************************************************************************
 * function prototype
 ******************************************************************************/


/******************************************************************************
 * Function code
 ******************************************************************************/
/*
 * brief Separate 3ABin
 * param a_pc3ABinAdd [IN], pointer of 3A Bin
 * param a_pcAEAdd     [OUT], location pointer of AE bin
 * param a_pcAFAdd     [OUT], location pointer of AF bin
 * param a_pcAWBAdd    [OUT], location pointer of AWB bin
 * return None
 */
void al3awrapper_com_separate_3a_bin(uint8* a_pc3abinadd, uint8** a_pcaeadd, uint8** a_pcafadd, uint8** a_pcawbadd)
{
	struct header_info *t_header_info;
	t_header_info = (struct header_info *)a_pc3abinadd;

	*a_pcaeadd = a_pc3abinadd + t_header_info->uwlocation1;
	*a_pcafadd = a_pc3abinadd + t_header_info->uwlocation2;
	*a_pcawbadd = a_pc3abinadd + t_header_info->uwlocation3;
}

/*
 * brief Separate 3ABin type2 API, add return size info
 * param a_pc3ABinAdd [IN], pointer of 3A Bin
 * param uw_bin_size  [IN], total bin size of 3A Bin file
 * param a_bin1_info [OUT], seperated bin information (including addr, size)
 * return None
 */
uint32 al3awrapper_com_separate_3a_bin_type2(uint8* a_pc3abinadd, uint32 uw_bin_size , struct bin1_sep_info* a_bin1_info )
{
	uint32 ret = ERR_WRP_COMM_SUCCESS;
	/* parameter validity check */
	if ( a_pc3abinadd == NULL || a_bin1_info == NULL )
		return ERR_WRP_COMM_INVALID_ADDR;

	struct header_info *t_header_info;
	t_header_info = (struct header_info *)a_pc3abinadd;

	/* size over boundry check  */
	if ( t_header_info->uwlocation1 > uw_bin_size)
		return ERR_WRP_COMM_INVALID_SIZEINFO_1;
	else if ( t_header_info->uwlocation2 > uw_bin_size)
		return ERR_WRP_COMM_INVALID_SIZEINFO_2;
	else if ( t_header_info->uwlocation3 > uw_bin_size)
		return ERR_WRP_COMM_INVALID_SIZEINFO_3;

	/* seperated size validity check */
	if ( t_header_info->uwlocation2 < t_header_info->uwlocation1 )
		return ERR_WRP_COMM_INVALID_SIZEINFO_1;
	else if ( t_header_info->uwlocation3 < t_header_info->uwlocation2 )
		return ERR_WRP_COMM_INVALID_SIZEINFO_2;
	else if ( uw_bin_size < t_header_info->uwlocation3 )
		return ERR_WRP_COMM_INVALID_SIZEINFO_3;

	a_bin1_info->puc_ae_bin_addr  = a_pc3abinadd + t_header_info->uwlocation1;
	a_bin1_info->puc_awb_bin_addr = a_pc3abinadd + t_header_info->uwlocation2;
	a_bin1_info->puc_af_bin_addr  = a_pc3abinadd + t_header_info->uwlocation3;

	a_bin1_info->uw_ae_bin_size  = t_header_info->uwlocation2 - t_header_info->uwlocation1;
	a_bin1_info->uw_awb_bin_size = t_header_info->uwlocation3 - t_header_info->uwlocation2;
	a_bin1_info->uw_af_bin_size  = uw_bin_size - t_header_info->uwlocation3;
	return ret;
}


/*
 * brief Separate ShadongIRPBin
 * param a_pcshadingirpbinadd [IN], pointer of ShadingIRP Bin
 * param a_pcshadingadd      [OUT], location pointer of Shading bin
 * param a_pcirpadd          [OUT], location pointer of IRP bin
 * return None
 */
void al3awrapper_com_separate_shadingirp_bin(uint8* a_pcshadingirpbinadd, uint8** a_pcshadingadd, uint8** a_pcirpadd)
{
	struct header_info *t_header_info;
	t_header_info = (struct header_info *)a_pcshadingirpbinadd;

	*a_pcshadingadd = a_pcshadingirpbinadd + t_header_info->uwlocation1;
	*a_pcirpadd = a_pcshadingirpbinadd + t_header_info->uwlocation2;
}

/*
 * brief Separate ShadongIRPBin type2 API, add return size info
 * param a_pcshadingirpbinadd [IN], pointer of 3A Bin
 * param uw_bin_size  [IN], total bin size of 3A Bin file
 * param a_bin2_info [OUT], seperated bin information (including addr, size)
 * return None
 */
uint32 al3awrapper_com_separate_shadingirp_bin_type2(uint8* a_pcshadingirpbinadd, uint32 uw_bin_size , struct bin2_sep_info* a_bin2_info )
{
	int32 ret = ERR_WRP_COMM_SUCCESS;
	/* parameter validity check */
	if ( a_pcshadingirpbinadd == NULL || a_bin2_info == NULL )
		return ERR_WRP_COMM_INVALID_ADDR;

	struct header_info *t_header_info;
	t_header_info = (struct header_info *)a_pcshadingirpbinadd;

	/* size over boundry check  */
	if ( t_header_info->uwlocation1 > uw_bin_size)
		return ERR_WRP_COMM_INVALID_SIZEINFO_1;
	else if ( t_header_info->uwlocation2 > uw_bin_size)
		return ERR_WRP_COMM_INVALID_SIZEINFO_2;

	/* seperated size validity check */
	if ( t_header_info->uwlocation2 < t_header_info->uwlocation1 )
		return ERR_WRP_COMM_INVALID_SIZEINFO_1;
	else if ( uw_bin_size < t_header_info->uwlocation2 )
		return ERR_WRP_COMM_INVALID_SIZEINFO_2;

	a_bin2_info->puc_shading_bin_addr  = a_pcshadingirpbinadd + t_header_info->uwlocation1;
	a_bin2_info->puc_irp_bin_addr      = a_pcshadingirpbinadd + t_header_info->uwlocation2;

	a_bin2_info->uw_shading_bin_size  = t_header_info->uwlocation2 - t_header_info->uwlocation1;
	a_bin2_info->uw_irp_bin_size      = uw_bin_size - t_header_info->uwlocation2;

	return ret;
}

/*
 * API name: al3awrapper_com_getversion
 * This API would return labeled version of wrapper
 * fWrapVersion[out], return current wapper version
 * return: error code
 */
UINT32 al3awrapper_com_getversion( float *fwrapversion )
{
	UINT32 ret = ERR_WRP_COMM_SUCCESS;
	*fwrapversion = _WRAPPER_COMM_VER;
	return ret;
}


/*
 * brief Separate function bin API, add return size info
 * param a_cfunction [IN], function number
 * param a_pcFunctionBin [IN], pointer of function addr
 * param uw_IDX_number  [IN], input idx number
 * param a_pcBinAddr [OUT], seperated function addr
 * param a_pwBinSize [OUT], seperated function size
 * return None
 */
uint32 al3awrapper_com_separate_function_bin(uint8 a_cfunction, uint8* a_pcFunctionBin, uint32 uw_IDX_number, uint8 **a_pcBinAddr, uint32 *a_pwBinSize)
{
	uint32 *ptr_bin_addr;
	uint32 *ptr_bin_list;
	uint32 *ptr_bin_size;
	struct functionbin_header_info *functionHeader;

	/* parameter validity check */
	if( NULL == a_pcFunctionBin || NULL == a_pcBinAddr || NULL == a_pwBinSize )
		return ERR_WRP_COMM_INVALID_ADDR;

	if( a_cfunction > WRAPPER_COMM_BIN_NUMBER )
		return ERR_WRP_COMM_INVALID_FUNCTION_IDX;

	functionHeader = (struct functionbin_header_info*) a_pcFunctionBin;

	/* header size check */
	if( sizeof(struct functionbin_header_info) != functionHeader->HeaderSize )
		return ERR_WRP_COMM_INVALID_FUNCTION(a_cfunction);

	/* IDX number check */
	if( uw_IDX_number > functionHeader->Index_Table_Number ||
	    functionHeader->Index_Table_Offset > functionHeader->TotalSize ||
	    functionHeader->Bin_Size_Table_Offset > functionHeader->TotalSize )
		return ERR_WRP_COMM_INVALID_FUNCTION(a_cfunction);

	ptr_bin_addr = (uint32*)(a_pcFunctionBin + functionHeader->Index_Table_Offset);
	ptr_bin_list = (uint32*)(a_pcFunctionBin + functionHeader->Bin_List_Offset);
	ptr_bin_size = (uint32*)(a_pcFunctionBin + functionHeader->Bin_Size_Table_Offset);

	*a_pcBinAddr = (a_pcFunctionBin + *(ptr_bin_addr + uw_IDX_number));
	*a_pwBinSize = *(ptr_bin_size + *(ptr_bin_list + uw_IDX_number));

	return ERR_WRP_COMM_SUCCESS;
}

/*
 * brief Separate one bin API to structure bin1_sep_info & bin2_sep_info
 * param a_pcbinadd [IN], pointer of bin addr
 * param uw_bin_size  [IN], total bin size of 3A Bin file
 * param uw_IDX_number [IN], current IDX
 * param a_bin1_info [OUT], seperated bin information (including addr, size)
 * param a_bin2_info [OUT], seperated bin information (including addr, size)
 * return None
 */
uint32   al3awrapper_com_separate_one_bin(uint8* a_pcbinadd, uint32 uw_bin_size, uint32 uw_IDX_number, struct bin1_sep_info* a_bin1_info, struct bin2_sep_info* a_bin2_info, struct bin3_sep_info* a_bin3_info)
{
	int32   ret = ERR_WRP_COMM_SUCCESS;
	int32   result = ERR_WRP_COMM_SUCCESS;
	int8    position = -1;
	uint8   i, j;
	uint8   *addr;
	uint32  offset;
	uint32  *ptr_function_type;
	uint32  *ptr_function_size;
	uint32  *ptr_function_offset;
	int8    position_list[WRAPPER_COMM_BIN_NUMBER];
	struct onebin_header_info *t_header_info;

	/* This version wrapper support follows functions: */
	enum onebin_function_t Merged_Functions[WRAPPER_COMM_BIN_NUMBER] = {    AF,
	                                                                        AE,
	                                                                        AWB,
	                                                                        Flicker,
	                                                                        CAF,
	                                                                        PDAF,
	                                                                        Scene_Setting,
	                                                                        SHADING,
	                                                                        IRP
	                                                                   };

	/* parameter validity check */
	if (NULL == a_pcbinadd || NULL == a_bin1_info || NULL == a_bin2_info || NULL == a_bin3_info)
		return ERR_WRP_COMM_INVALID_ADDR;

	t_header_info = (struct onebin_header_info *) a_pcbinadd;

	/* IDX number check */
	if (uw_IDX_number > t_header_info->IDX_Number) {
		result = ERR_WRP_COMM_INVALID_IDX;
		uw_IDX_number = 0;
	}

	/* Bin format check */
	if( sizeof(struct onebin_header_info)   != t_header_info->HeaderSize ||
	    WRAPPER_COMM_BIN_PROJECT_STRLENGTH  != t_header_info->Prj_StrLength ||
	    WRAPPER_COMM_BIN_NUMBER             > t_header_info->FunctionBin_Table_Number)
		return ERR_WRP_COMM_INVALID_FORMAT;

	/* Function Type / Size / Offset */
	ptr_function_type = (uint32*)(a_pcbinadd + t_header_info->FunctionBin_Type_Offset);
	ptr_function_size = (uint32*)(a_pcbinadd + t_header_info->FunctionBin_Size_Offset);
	ptr_function_offset = (uint32*)(a_pcbinadd + t_header_info->FunctionBin_Table_Offset);

	/* Merged_Functions check */
	for(i = 0; i < WRAPPER_COMM_BIN_NUMBER; i++) {
		position = -1;
		for (j = 0; j < t_header_info->FunctionBin_Table_Number; j++) {
			if(ptr_function_type[j] == Merged_Functions[i]) {
				position = j;
				break;
			}
		}
		if( -1 == position )
			return ERR_WRP_COMM_MISSING_FUNCTION(i);
		else
			position_list[i] = position;
	}

	for(i = 0; i < WRAPPER_COMM_BIN_NUMBER; i++) {

		position = position_list[i];
		offset = ptr_function_offset[position];

		/* size over boundry check  */
		if( uw_bin_size < offset )
			return ERR_WRP_COMM_INVALID_SIZEINFO(i);

		addr = (a_pcbinadd + offset);

		switch (Merged_Functions[position]) {
		case AF:
			ret = al3awrapper_com_separate_function_bin(i, addr, uw_IDX_number, &(a_bin1_info->puc_af_bin_addr), &(a_bin1_info->uw_af_bin_size));
			break;
		case AE:
			ret = al3awrapper_com_separate_function_bin(i, addr, uw_IDX_number, &(a_bin1_info->puc_ae_bin_addr), &(a_bin1_info->uw_ae_bin_size));
			break;
		case AWB:
			ret = al3awrapper_com_separate_function_bin(i, addr, uw_IDX_number, &(a_bin1_info->puc_awb_bin_addr), &(a_bin1_info->uw_awb_bin_size));
			break;
		case Flicker:
			ret = al3awrapper_com_separate_function_bin(i, addr, uw_IDX_number, &(a_bin1_info->puc_flicker_bin_addr), &(a_bin1_info->uw_flicker_bin_size));
			break;
		case CAF:
			ret = al3awrapper_com_separate_function_bin(i, addr, uw_IDX_number, &(a_bin3_info->puc_caf_bin_addr), &(a_bin3_info->uw_caf_bin_size));
			break;
		case PDAF:
			ret = al3awrapper_com_separate_function_bin(i, addr, uw_IDX_number, &(a_bin3_info->puc_pdaf_bin_addr), &(a_bin3_info->uw_pdaf_bin_size));
			break;
		case Scene_Setting:
			ret = al3awrapper_com_separate_function_bin(i, addr, uw_IDX_number, &(a_bin3_info->puc_scene_bin_addr), &(a_bin3_info->uw_scene_bin_size));
			break;
		case SHADING:
			ret = al3awrapper_com_separate_function_bin(i, addr, uw_IDX_number, &(a_bin2_info->puc_shading_bin_addr), &(a_bin2_info->uw_shading_bin_size));
			break;
		case IRP:
			a_bin2_info->puc_irp_bin_addr = addr;
			a_bin2_info->uw_irp_bin_size = ptr_function_size[position];
			break;
		default:
			break;
		}

		if( ERR_WRP_COMM_SUCCESS != ret )
			return ret;

	}

	if( ERR_WRP_COMM_SUCCESS != result )
		return result;

	return ERR_WRP_COMM_SUCCESS;
}


/*
 * API name: al3awrapper_com_separate_scenesetting
 * This API is used to obtain scene setting info from scene setting file
 * param scene[in]: scene mode
 * param setting_file[in]: file address pointer to scene setting file w/ IDX
 * param scene_info[out]: output scene setting info, including scene mode and addr
 * return: error code
 */
uint32 al3awrapper_com_separate_scenesetting(enum scene_setting_list_t scene, void *setting_file, struct scene_setting_info *scene_info)
{
	uint32 ret = ERR_WRP_COMM_SUCCESS;
	short sceneIdx = (short)scene;
	char *addr;
	struct alwrapper_scene_setting_header *header;
	struct alwrapper_scene_setting *setting;

	if (setting_file == NULL || scene_info == NULL)
		return ERR_WRP_COMM_INVALID_ADDR;

	header = (struct alwrapper_scene_setting_header*)setting_file;
	if (header->ud_structure_size != sizeof(struct alwrapper_scene_setting_header) ||
	    header->ud_scene_number + 1 != Scene_Max)
		return ERR_WRP_COMM_INVALID_FORMAT;

	addr = (char*)setting_file;
	scene_info->uw_mode = (uint32)scene;
	if (Auto == scene)
		scene_info->puc_addr = NULL;
	else
		scene_info->puc_addr = (void*)(addr + header->ud_scene_offset + header->ud_scene_size * (sceneIdx - 1));
	return ret;
}

