/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __IBOKEH_H__
#define __IBOKEH_H__
#if (defined WIN32 || defined REALVIEW)
#define JNIEXPORT
#else
#include <jni.h>
#endif

#ifdef WIN32
#define GraphicBuffer	unsigned char
#else
//#include "GraphicBuffer.h"
//using namespace android;
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	int width;  // image width
	int height; // image height
	int depth_width;
	int depth_height;
	int SmoothWinSize;//odd number
	int ClipRatio; // RANGE 1:64
	int Scalingratio;//2,4,6,8
	int DisparitySmoothWinSize;//odd number
	//int HistMinNumberRatio;
	//int UpdatedMaxMinDiff;
} InitParams;
#if 0
typedef struct {
	int F_number; // 1 ~ 20
	int sel_x; /* The point which be touched */
	int sel_y; /* The point which be touched */
	unsigned char *DisparityImage;
} WeightParams;
#endif
JNIEXPORT int iBokehInit(void **handle, InitParams *params);
JNIEXPORT int iBokehDeinit(void *handle);
JNIEXPORT int iBokehCreateWeightMap(void *handle, WeightParams_t *params);
JNIEXPORT int iBokehBlurImage(void *handle, void *Src_YUV, void *Output_YUV);

#ifdef __cplusplus
}
#endif

#endif
