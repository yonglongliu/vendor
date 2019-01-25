/*----------------------------------------------------------------------------------------------
*
* This file is ArcSoft's property. It contains ArcSoft's trade secret, proprietary and
* confidential information.
*
* The information and code contained in this file is only for authorized ArcSoft employees
* to design, create, modify, or review.
*
* DO NOT DISTRIBUTE, DO NOT DUPLICATE OR TRANSMIT IN ANY FORM WITHOUT PROPER AUTHORIZATION.
*
* If you are not an intended recipient of this file, you must not copy, distribute, modify,
* or take any action in reliance on it.
*
* If you have received this file in error, please immediately notify ArcSoft and
* permanently delete the original and any copy of any file and any printout thereof.
*
*-------------------------------------------------------------------------------------------------*/
/*
* BeautyShot_Image_Algorithm.h
*
* Reference:
*
* Description:
*
* Created by
*/

#ifndef ____BEAUTYSHOT_IMAGE_ALGORITHM_H_20150805____
#define ____BEAUTYSHOT_IMAGE_ALGORITHM_H_20150805____

#include "amcomdef.h"
#include "ammem.h"
#include "asvloffscreen.h"
#include "AISFReferenceInter.h"
#include "AISFCommonDef.h"
#include "asvloffscreen.h"
#include "abstypes.h"

#ifdef ENABLE_DLL_EXPORT
#define ARC_BS_API __declspec(dllexport)
#else
#define ARC_BS_API
#endif

class BeautyShot_Image_Algorithm : public AISF_NS::AISFReferenceInter
{
public:
	/*
	 *The unique ID of the BeautyShot_Image_Algorithm SDK, used in Create_BeautyShot_Image_Algorithm().
	 */
	static const MInt32 CLASSID = 0;
	/*
	 * Initialize the BeautyShot_Image_Algorithm SDK.
	 */
	virtual MRESULT	Init()=0;
	/*
	 * Uninitialize the BeautyShot_Image_Algorithm SDK. MUST correspond with Init().
	 */
	virtual MRESULT	UnInit()=0;

	/*
	 * Get the BeautyShot_Image_Algorithm SDK's version.
	 */
	virtual const MPBASE_Version *GetVersion()=0;


	/*
	 * Set/Get feature level to/from the BeautyShot_Image_Algorithm SDK
	 * nFeatureKey: feature key defined in abstypes.h
	 * nLevel: intensity of the feature
	 */
	virtual MVoid SetFeatureLevel(MInt32 nFeatureKey, MInt32 nLevel)=0;
	virtual MInt32 GetFeatureLevel(MInt32 nFeatureKey)=0;

	/*
	 * Process the current image with according parameters.
	 * pImgIn: Pointer of input image data.
	 * pImgOut: Pointer of output image data, it can be the same as pImgIn.
	 * pFacesIn: The detected faces info of input image. If it is MNull, BeautyShot_Image_Algorithm will detect faces inside.
	 * pFacesOut: The output faces info. If it is MNull, BeautyShot_Image_Algorithm does not output faces info.
	 */
	virtual MRESULT Process(LPASVLOFFSCREEN pImgIn, LPASVLOFFSCREEN pImgOut, ABS_TFaces *pFacesIn, ABS_TFaces *pFacesOut)=0;
};


/*
 * The function exported by the dynamic so, which is used to create the BeautyShot_Image_Algorithm SDK object.
 * Parameters:
 *   [IN] ClassID : The unique class ID, defined as BeautyShot_Image_Algorithm::CLASSID.
 *   [OUT] ppalgorithm: The BeautyShot_Image_Algorithm SDK object will be created in this function with reference count as 1.
 *                and shall be released by calling (*ppalgorithm)->Release() when it is not used anymore.
 * Return:
 *   MOK is succeeded, others are failed.
 */
#ifdef __cplusplus
extern "C" {
#endif

ARC_BS_API MInt32 Create_BeautyShot_Image_Algorithm(const MInt32 ClassID,
											BeautyShot_Image_Algorithm** ppalgorithm);

#ifdef __cplusplus
}
#endif


#endif	//____BEAUTYSHOT_IMAGE_ALGORITHM_H_20150805____
