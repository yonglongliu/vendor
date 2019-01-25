// ---------------------------------------------------------
// [CONFIDENTIAL]
// Copyright (c) 2016 Spreadtrum Corporation
// pd_algo.h
// ---------------------------------------------------------
#ifndef _PD_ALGO_
#define _PD_ALGO_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <isp_type.h>

#define PD_VERSION "PDAF_Algo_Ver: v1.16"
#define PD_PIXEL_NUM (24576)
#define PD_AREA_NUMBER (4)
#define PD_PIXEL_ALIGN_X (16)
#define PD_PIXEL_ALIGN_Y (32)
#define PD_PIXEL_ALIGN_HALF_X (7)
#define PD_PIXEL_ALIGN_HALF_Y (15)
#define PD_LINE_W_PIX (16)
#define PD_LINE_H_PIX (16)
#define PD_UNIT_W_SHIFT (4)
#define PD_UNIT_H_SHIFT (4)
#define PD_SLIDE_RANGE (33)		/* number of 33 means -16 to +16 */
#ifdef __cplusplus
extern "C" {
#endif							//__cplusplus

	typedef struct {
		cmr_s32 dX;
		cmr_s32 dY;
		float fRatio;
		cmr_u8 ucType;
	} PD_PixelInfo;

	typedef struct {
		cmr_s32 dBeginX;
		cmr_s32 dBeginY;
		cmr_s32 dImageW;
		cmr_s32 dImageH;
		cmr_s32 dAreaW;
		cmr_s32 dAreaH;
		cmr_s32 dDTCTEdgeTh;
		//PD Sensor Mode (0: Sony IMX258, 1:OV13855, 2:S5K3L8)
		cmr_s32 dSensorMode;
		//0:No need calibration data, 1:Calibrated by module house (OTP), 2: Calibrated by SPRD
		cmr_s32 dCalibration;
		//Only dCalibration = 1 to be effective
		void *OTPBuffer;
		cmr_s32 dOVSpeedup;
		//0: Normal, 1:Mirror+Flip
		int dSensorSetting;
	} PD_GlobalSetting;

	typedef void (*PDCALLBACK) (unsigned char *);

	cmr_s32 PD_Init(PD_GlobalSetting * a_pdGSetting);
	cmr_s32 PD_Do(cmr_u8 * raw, cmr_u8 * y, cmr_s32 a_dRectX, cmr_s32 a_dRectY, cmr_s32 a_dRectW, cmr_s32 a_dRectH, cmr_s32 a_dArea);
	cmr_s32 PD_DoType2(void *a_pInPhaseBuf_left, void *a_pInPhaseBuf_right, cmr_s32 a_dRectX, cmr_s32 a_dRectY, cmr_s32 a_dRectW, cmr_s32 a_dRectH, cmr_s32 a_dArea);
	cmr_s32 PD_SetCurVCM(cmr_s32 CurVCM);
	cmr_s32 PD_GetResult(cmr_s32 * a_pdConf, double *a_pdPhaseDiff, cmr_s32 * a_pdFrameID, cmr_s32 * a_pdDCCGain, cmr_s32 a_dArea);
	cmr_s32 PD_Uninit();
	cmr_s32 PD_PhaseFormatConverter(cmr_u8 * left_in, cmr_u8 * right_in, cmr_s32 * left_out, cmr_s32 * right_out, cmr_s32 a_dNumL, cmr_s32 a_dNumR);
	cmr_s32 PD_PhasePixelReorder(cmr_s32 * pBuf_pd_l, cmr_s32 * pBuf_pd_r, cmr_s32 * pPd_left_reorder, cmr_s32 * pPd_right_reorder, cmr_s32 buf_width, cmr_s32 buf_height);

	void FreeBuffer(cmr_u8 * raw);

#ifdef __cplusplus
}
#endif							//__cplusplus
#endif							/* _PD_ALGO_ */
