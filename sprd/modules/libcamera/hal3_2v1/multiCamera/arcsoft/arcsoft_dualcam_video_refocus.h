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
#ifndef _ARCSOFT_DUALCAM_VID_REFOCUS_H_
#define _ARCSOFT_DUALCAM_VID_REFOCUS_H_

#ifdef ARC_VIDEOREFOCUSDLL_EXPORTS
#define ARC_DCVR_API __declspec(dllexport)
#else
#define ARC_DCVR_API
#endif

#include "asvloffscreen.h"
#include "merror.h"
#include "arcsoft_dualcam_common_refocus.h"

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************************
* This function is used to get version information of library
************************************************************************/
ARC_DCVR_API const MPBASE_Version *ARC_DCVR_GetVersion();

/************************************************************************
* This function is used to get default parameters of library
************************************************************************/
typedef struct _tag_ARC_DCVR_PARAM {
    MPOINT
    ptFocus; // [in]  The focus point to decide which region should be clear
    MInt32 i32BlurLevel; // [in]  The intensity of blur,range [1, 100]
    MBool bRefocusOn;    // [in]  Do Refocus or not
} ARC_DCVR_PARAM, *LPARC_DCVR_PARAM;

ARC_DCVR_API MRESULT
ARC_DCVR_GetDefaultParam(   // return MOK if success, otherwise fail
    LPARC_DCVR_PARAM pParam // [out] The default parameter of algorithm engin
    );

/************************************************************************
* the following functions for Arcsoft video refocus
************************************************************************/
ARC_DCVR_API MRESULT ARC_DCVR_Init( // return MOK if success, otherwise fail
    MHandle
        *phHandle // [out] The algorithm engine will be initialized by this API
    );
ARC_DCVR_API MRESULT
ARC_DCVR_SetCameraImageInfo( // return MOK if success, otherwise fail
    MHandle hHandle,         // [in]  The algorithm engine
    LPARC_REFOCUSCAMERAIMAGE_PARAM pParam // [in]  Camera and image information
    );

ARC_DCVR_API MRESULT
ARC_DCVR_SetCaliData(          // return MOK if success, otherwise fail
    MHandle hHandle,           // [in]  The algorithm engine
    LPARC_DC_CALDATA pCaliData // [in]   Calibration Data
    );
ARC_DCVR_API MRESULT
ARC_DCVR_SetImageDegree( // return MOK if success, otherwise fail
    MHandle hHandle,     // [in]  The algorithm engine
    MInt32 i32ImgDegree  // [in]  The degree of input images
    );

ARC_DCVR_API MRESULT ARC_DCVR_Uninit( // return MOK if success, otherwise fail
    MHandle *phHandle // [in/out] The algorithm engine will be un-initialized by
                      // this API
    );

ARC_DCVR_API MRESULT ARC_DCVR_Process( // return MOK if success, otherwise fail
    MHandle hHandle,                   // [in]  The algorithm engine
    LPASVLOFFSCREEN pMainImg, // [in]  The offscreen of main image input.
                              // Generally, Left or Wide as Main.
    LPASVLOFFSCREEN pAuxImg,  // [in]  The offscreen of auxiliary image input.
    LPASVLOFFSCREEN pDstImg,  // [out] The offscreen of result image
    LPARC_DCVR_PARAM pParam   // [in]  The parameters for algorithm engine
    );

#ifdef __cplusplus
}
#endif

#endif
