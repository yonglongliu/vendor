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
#include "amcomdef.h"
#include "amdisplay.h"
#include "asvloffscreen.h"

// For MIPDoEffect

#define MEID_SUNNY 0x0003
#define MEID_FOCUS 0x0005
#define MEID_WARM02 0x0006
#define MEID_CLASSIC 0x0008
#define MEID_MONO 0x000a
#define MEID_GOLDEN 0x000d
#define MEID_WARM01 0x000e
#define MEID_1995 0x000f
#define MEID_FILM01 0x0010
#define MEID_COOL 0x0015
#define MEID_YELLOW 0x0016
#define MEID_PAINTING 0x0017
#define MEID_FLASH 0x0019
#define MEID_WARM03 0x001D
#define MEID_TONAL 0x001E
#define MEID_LOMO 0x001F
#define MEID_VINTAGE 0x0020
#define MEID_FILM04 0x0021
#define MEID_TOKYO 0x0022
#define MEID_COOL02 0x0023
#define MEID_SUMMER 0x0024
#define MEID_BABY 0x0025
#define MEID_BLUES 0x0026
#define MEID_COOL01 0x0027

// For dwDirection
#define MDIRECTION_FLIP_HORIZONTAL 0x100

typedef struct __tag_effectparam {
    MDWord dwEffectID;
    MDWord dwDirection;
    MVoid *pEffect;
    MDWord dwParamSize;
} MEffectParam;

typedef struct __tag_pixelinfo {
    MDWord dwPixelArrayFormat;
    MInt32 lWidth;
    MInt32 lHeight;
} MPixelInfo;

#ifdef __cplusplus
extern "C" {
#endif

MRESULT MIPCreateImageEngine(MEffectParam *peffectparam, MPixelInfo *ppixelinfo,
                             MHandle *phandle);

MRESULT MIPDoEffect(MHandle handle, LPASVLOFFSCREEN pOffscreen);

MRESULT MIPDestroyImageEngine(MHandle handle);

#ifdef __cplusplus
};
#endif
