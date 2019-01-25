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

#ifndef ARCSOFT_DUALCAM_IMG_REFOCUS_H_
#define ARCSOFT_DUALCAM_IMG_REFOCUS_H_

#ifdef ARC_DCIRDLL_EXPORTS
#ifdef PLATFORM_LINUX
#define ARCDCIR_API __attribute__((visibility("default")))
#else
#define ARCDCIR_API __declspec(dllexport)
#endif
#else
#define ARCDCIR_API
#endif

#include "asvloffscreen.h"
#include "merror.h"
#include "arcsoft_dualcam_common_refocus.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _tag_ARC_DCIR_FACE_PARAM {
    PMRECT prtFaces; // [in]  The array of faces
    MInt32
        *pi32FaceAngles; // [in]  The array of corresponding face orientations
    MInt32 i32FacesNum;  // [in]  The number of faces
} ARC_DCIR_FACE_PARAM, *LPARC_DCIR_FACE_PARAM;

typedef struct _tag_ARC_DCIR_PARAM {
    MInt32 i32ImgDegree; // [in]  The degree of input images
    MFloat fMaxFOV;      // [in]  The maximum camera FOV among horizontal and
                         // vertical in degree
    ARC_DCIR_FACE_PARAM
    faceParam; // [in]  The information of faces in the left image
} ARC_DCIR_PARAM, *LPARC_DCIR_PARAM;

ARCDCIR_API const MPBASE_Version *ARC_DCIR_GetVersion();

typedef struct _tag_ARC_DCIR_REFOCUS_PARAM {
    MPOINT
    ptFocus; // [in]  The focus point to decide which region should be clear
    MInt32 i32BlurIntensity; // [in]  The intensity of blur,Range is [0,100],
                             // default as 50.
} ARC_DCIR_REFOCUS_PARAM, *LPARC_DCIR_REFOCUS_PARAM;

ARCDCIR_API MRESULT
ARC_DCIR_GetDefaultParam(   // return MOK if success, otherwise fail
    LPARC_DCIR_PARAM pParam // [out] The default parameter of algorithm engine
    );

/************************************************************************
 * the following functions for Arcsoft disparity data
 ************************************************************************/
#define ARC_DCIR_NORMAL_MODE 0x01
#define ARC_DCIR_POST_REFOCUS_MODE 0x02

ARCDCIR_API MRESULT ARC_DCIR_Init( // return MOK if success, otherwise fail
    MHandle *phHandle, // [in/out] The algorithm engine will be initialized by
                       // this API
    MInt32 i32Mode     // [in]  The mode of algorithm engine
    );

ARCDCIR_API MRESULT ARC_DCIR_Uninit( // return MOK if success, otherwise fail
    MHandle *phHandle // [in/out] The algorithm engine will be un-initialized by
                      // this API
    );
ARCDCIR_API MRESULT
ARC_DCIR_SetCameraImageInfo( // return MOK if success, otherwise fail
    MHandle hHandle,         // [in]  The algorithm engine
    LPARC_REFOCUSCAMERAIMAGE_PARAM pParam // [in]  Camera and image information
    );

ARCDCIR_API MRESULT
ARC_DCIR_SetCaliData(          // return MOK if success, otherwise fail
    MHandle hHandle,           // [in]  The algorithm engine
    LPARC_DC_CALDATA pCaliData // [in]   Calibration Data
    );

ARCDCIR_API MRESULT
ARC_DCIR_CalcDisparityData(     // return MOK if success, otherwise fail
    MHandle hHandle,            // [in]  The algorithm engine
    LPASVLOFFSCREEN pMainImg,   // [in]  The offscreen of main image input.
                                // Generally, Left image or Wide image as Main.
    LPASVLOFFSCREEN pAuxImg,    // [in]  The offscreen of auxiliary image input.
    LPARC_DCIR_PARAM pDCIRParam // [in]  The parameters for algorithm engine
    );

ARCDCIR_API MRESULT
ARC_DCIR_GetDisparityDataSize( // return MOK if success, otherwise fail
    MHandle hHandle,           // [in]  The algorithm engine
    MInt32 *pi32Size           // [out] The size of disparity map
    );

ARCDCIR_API MRESULT
ARC_DCIR_GetDisparityData( // return MOK if success, otherwise fail
    MHandle hHandle,       // [in]  The algorithm engine
    MVoid *pDisparityData  // [out] The data of disparity map
    );

ARCDCIR_API MRESULT ARC_DCIR_Process( // return MOK if success, otherwise fail
    MHandle hHandle,                  // [in]  The algorithm engine
    MVoid *pDisparityData,            // [in]  The data of disparity data
    MInt32 i32DisparityDataSize,      // [in]  The size of disparity data
    LPASVLOFFSCREEN pMainImg,         // [in]  The off-screen of main image
    LPARC_DCIR_REFOCUS_PARAM
        pRFParam,           // [in]  The parameter for refocusing image
    LPASVLOFFSCREEN pDstImg // [out]  The off-screen of result image
    );

ARCDCIR_API MRESULT ARC_DCIR_Reset( // return MOK if success, otherwise fail
    MHandle hHandle // [in/out] The algorithm engine will be reset by this API
    );

#ifdef __cplusplus
}
#endif

#endif /* ARCSOFT_DUALCAM_IMG_REFOCUS_H_ */
