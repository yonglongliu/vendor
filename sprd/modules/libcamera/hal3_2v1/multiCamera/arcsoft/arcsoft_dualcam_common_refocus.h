/*******************************************************************************
Copyright(c) ArcSoft, All right reserved.

This file is ArcSoft's property. It contains ArcSoft's trade secret, proprietary
and confidential information.

The information and code contained in this file is only for authorized ArcSoft
employees to design, create, modify, or review.

DO NOT DISTRIBUTE, DO NOT DUPLICATE OR TRANSMIT IN ANY FORM WITHOUT PROPER
AUTHORIZATION.

If you are not an intended recipient of this file, you must not copy,
distribute, modify, or take any action in reliance on it.

If you have received this file in error, please immediately notify ArcSoft and
permanently delete the original and any copy of any file and any printout
thereof.
*******************************************************************************/

#ifndef DUALCAMREFOCUS_INC_ARCSOFT_DUALCAM_COMMON_REFOCUS_H_
#define DUALCAMREFOCUS_INC_ARCSOFT_DUALCAM_COMMON_REFOCUS_H_

#ifdef ARC_DCIRDLL_EXPORTS
#ifdef PLATFORM_LINUX
#define ARCDCOOM_API __attribute__((visibility("default")))
#else
#define ARCDCOOM_API __declspec(dllexport)
#endif
#else
#define ARCDCOOM_API
#endif
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _tagARC_DC_CALDATA {
    MVoid *pCalibData;       // [in]  The data of camera's calibration
    MInt32 i32CalibDataSize; // [in]  The size of camera's calibration data

} ARC_DC_CALDATA, *LPARC_DC_CALDATA;

typedef struct _tagARC_REFOCUSCAMERAIMAGE_PARAM {
    MInt32 i32LeftFullWidth;   // Width of Left full image by ISP. Left as Main.
    MInt32 i32LeftFullHeight;  // Height of Left full image by ISP.
    MInt32 i32RightFullWidth;  // Width of Right full image by ISP. Right as
                               // Auxiliary.
    MInt32 i32RightFullHeight; // Height of Right full image by ISP.
} ARC_REFOCUSCAMERAIMAGE_PARAM, *LPARC_REFOCUSCAMERAIMAGE_PARAM;

#ifdef __cplusplus
}
#endif
#endif /* DUALCAMREFOCUS_INC_ARCSOFT_DUALCAM_COMMON_REFOCUS_H_ */
