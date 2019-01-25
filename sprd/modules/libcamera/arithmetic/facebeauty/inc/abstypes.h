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
 * abstypes.h
 *
 *
 */

#ifndef _ABS_TYPES_H_
#define _ABS_TYPES_H_

#include "amcomdef.h"

#define ABS_MAX_FACE_NUM			32
#define ABS_MAX_COLOR_NUM 			4

/* Error Definitions */

// No Error.
#define ABS_OK						0
// Unknown error.
#define ABS_ERR_UNKNOWN				-1
// Invalid parameter, maybe caused by NULL pointer.
#define ABS_ERR_INVALID_INPUT		-2
// User abort.
#define ABS_ERR_USER_ABORT			-3
// Unsupported image format.
#define ABS_ERR_IMAGE_FORMAT		-101
// Image Size unmatched.
#define ABS_ERR_IMAGE_SIZE_UNMATCH	-102
// Out of memory.
#define ABS_ERR_ALLOC_MEM_FAIL		-201
// No face input while faces needed.
#define ABS_ERR_NOFACE_INPUT		-903

// Begin of feature key definitions

// Base value of feature key.
static const MInt32 FEATURE_BASE							= 200;

// Feature key for intensity of skin soften. Intensity from 0 to 100.
static const MInt32 FEATURE_FACE_SOFTEN_KEY 				= FEATURE_BASE+1;
// Feature key for intensity of skin bright. Intensity from 0 to 100.
static const MInt32 FEATURE_FACE_WHITEN_KEY 				= FEATURE_BASE+2;
// Feature key for intensity of eye enlargement. Intensity from 0 to 100.
static const MInt32 FEATURE_EYE_ENLARGEMENT_KEY 			= FEATURE_BASE+4;
// Feature key for intensity of face slender. Intensity from 0 to 100.
static const MInt32 FEATURE_FACE_SLENDER_KEY 				= FEATURE_BASE+5;



typedef struct _tagFaces {
	MRECT		prtFaces[ABS_MAX_FACE_NUM];			// The array of face rectangles
	MInt32		lFaceNum;							// Number of faces array
	MInt32		plFaceRolls[ABS_MAX_FACE_NUM];		// The array of face angles, in the range of [0, 360)
} ABS_TFaces;


#endif /* _ABS_TYPES_H_ */
