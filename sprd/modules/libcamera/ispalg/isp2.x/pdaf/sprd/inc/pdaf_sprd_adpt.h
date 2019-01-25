/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _SPRD_PDAF_ADPT_H
#define _SPRD_PDAF_ADPT_H

#include <utils/Timers.h>
#include <pthread.h>
#include "pd_algo.h"

#define IMAGE_WIDTH 4160
#define IMAGE_HEIGHT 3072
#define BEGIN_X_0 24
#define BEGIN_Y_0 24
#define BEGIN_X_1 64
#define BEGIN_Y_1 32
#define BEGIN_X_2 24
#define BEGIN_Y_2 24
#define ROI_X_0 1048
#define ROI_Y_0 792
#define ROI_X_1 1088
#define ROI_Y_1 800
#define ROI_X_2 1048
#define ROI_Y_2 792
#define ROI_Width 2048
#define ROI_Height 1536
#define SENSOR_ID_0 0
#define SENSOR_ID_1 1
#define SENSOR_ID_2 2
#define SENSOR_ID_2_BLOCK_W 64
#define SENSOR_ID_2_BLOCK_H 64
#define PD_REG_OUT_SIZE	352
#define PD_OTP_PACK_SIZE	550

typedef struct {
	short int m_wLeft;
	short int m_wTop;
	short int m_wWidth;
	short int m_wHeight;
} alGE_RECT;

typedef struct {
	short int m_wLeft;
	short int m_wTop;
	short int m_wWidth;
	short int m_wHeight;
} alPD_RECT;

//static cmr_u16 Reg[256];

typedef struct {
	cmr_s32 dSensorID;			//0:for SamSung 1: for Sony
	double uwInfVCM;
	double uwMacroVCM;
} SensorInfo;

typedef struct {
	/*sensor information */
	SensorInfo tSensorInfo;
	cmr_s32 dcurrentVCM;
	// BV value
	float dBv;
	// black offset
	cmr_s32 dBlackOffset;
	cmr_u8 ucPrecision;
} PDInReg;

struct pdaf_timestamp {
	cmr_u32 sec;
	cmr_u32 usec;
};

struct sprd_pdaf_report_t {
	cmr_u32 token_id;
	cmr_u32 frame_id;
	char enable;
	struct pdaf_timestamp time_stamp;
	float pd_value;
	cmr_u32 pd_reg_size;
};

#endif
