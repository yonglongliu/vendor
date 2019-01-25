/******************************************************************************
 ** File Name:    vsp_deint.h                                                   *
 ** Author: *
 ** DATE:         5/4/2017                                                   *
 ** Copyright:    2017 Spreadtrum, Incorporated. All Rights Reserved.         *
 ** Description:  define data structures for vsp deint                      *
 *****************************************************************************/

#ifndef _VSP_DEINT_H_
#define _VSP_DEINT_H_

#include <string.h>
#include <utils/Log.h>

#define IMG_WIDTH 176
#define IMG_HEIGHT 144
#define INTERLEAVE 1
#define THRESHOLD 20
#define MAX_DECODE_FRAME_NUM 25

#define PUBLIC

typedef unsigned char BOOLEAN;
typedef unsigned char uint8;
typedef signed char int8;
typedef signed short int16;
typedef unsigned short uint16;
typedef signed int int32;
typedef unsigned int uint32;
typedef unsigned long		uint_32or64;

typedef struct DeintParams {
  uint32 width;
  uint32 height;

  uint8 interleave;
  uint8 threshold;

  uint32 y_len;
  uint32 c_len;
} DEINT_PARAMS_T;

typedef enum {
  SRC_FRAME_ADDR = 0,
  REF_FRAME_ADDR,
  DST_FRAME_ADDR,
  MAX_FRAME_NUM
} DEINT_BUF_TYPE;

#endif

