/*
 *******************************************************************************
 * $Header$ * *  Copyright (c) 2016-2025 Spreadtrum Inc. All rights reserved. *
 *  +-----------------------------------------------------------------+ *  | THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY ONLY BE USED | *  | AND COPIED IN ACCORDANCE WITH THE TERMS AND CONDITIONS OF SUCH  | *  | A LICENSE AND WITH THE INCLUSION OF THE THIS COPY RIGHT NOTICE. | *  | THIS SOFTWARE OR ANY OTHER COPIES OF THIS SOFTWARE MAY NOT BE   |
 *  | PROVIDED OR OTHERWISE MADE AVAILABLE TO ANY OTHER PERSON. THE   |
 *  | OWNERSHIP AND TITLE OF THIS SOFTWARE IS NOT TRANSFERRED.        |
 *  |                                                                 | *  | THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT   | *  | ANY PRIOR NOTICE AND SHOULD NOT BE CONSTRUED AS A COMMITMENT BY | *  | SPREADTRUM INC.                                                 |
 *  +-----------------------------------------------------------------+ *
 * $History$ *
 ******************************************************************************* */  
	
/*!
 *******************************************************************************
 * Copyright 2016-2025 Spreadtrum, Inc. All rights reserved. * * \file
 * AF_Interface.h * * \brief * Interface for AF * * \date * 2016/01/03 * * \author
 * Galen Tsai * *
 ******************************************************************************* */ 
	
#ifndef __AFV1_INTERFACE_H__
#define  __AFV1_INTERFACE_H__
	
	// Interface Commands
	typedef enum _AF_IOCTRL_CMD { AF_IOCTRL_TRIGGER = 0, AF_IOCTRL_STOP, AF_IOCTRL_Get_Result, AF_IOCTRL_Record_Wins,
	AF_IOCTRL_Set_Hw_Wins,
	AF_IOCTRL_Record_Vcm_Pos, AF_IOCTRL_Get_Alg_Mode,
	AF_IOCTRL_Get_Bokeh_Result,
	AF_IOCTRL_Set_Time_Stamp, AF_IOCTRL_Set_Pre_Trigger_Data,
	AF_IOCTRL_Record_FV, AF_IOCTRL_Set_Dac_info,
	AF_IOCTRL_MAX, 
} AF_IOCTRL_CMD;
 
	// Interface Param Structure
	typedef struct _AF_Result {
	cmr_u32 AF_Result;
	cmr_u32 af_mode;
} AF_Result;
 typedef struct _AF_Roi {
	cmr_u32 index;
	cmr_u32 start_x;
	cmr_u32 start_y;
	cmr_u32 end_x;
	cmr_u32 end_y;
} AF_Roi;
 typedef struct _AF_HW_Wins {
	void *win_settings;
	cmr_u32 af_mode;
} AF_HW_Wins;
 typedef struct _AF_Timestamp {
	cmr_u32 type;
	cmr_u64 time_stamp;
} AF_Timestamp;
 
	// Interface
void *AF_init(AF_Ctrl_Ops * AF_Ops, af_tuning_block_param * af_tuning_data, haf_tuning_param_t * haf_tune_data, cmr_u32 * dump_info_len, char *sys_version);
cmr_u8 AF_deinit(void *handle);
cmr_u8 AF_Process_Frame(void *handle);
cmr_u8 AF_IOCtrl_process(void *handle, AF_IOCTRL_CMD cmd, void *param);

#endif	/*  */
