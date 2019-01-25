/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2006 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29023 $ $Date: 2014-09-25 17:28:52 +0800 (Thu, 25 Sep 2014) $
 */

#ifndef _OSAL_TYPES_H_
#define _OSAL_TYPES_H_

/* 
 * Commonly used types
 */
typedef unsigned                uvint;
typedef int                     vint;
typedef char                    int8;
typedef unsigned char           uint8;
typedef short                   int16;
typedef unsigned short          uint16;
typedef int                     int32;
typedef unsigned int            uint32;
typedef long long               int64;
typedef unsigned long long      uint64;

/*
 * Maximum and Minimum values of the commonly used types
 */
#define MAX_INT8  (int8)(0x7F)
#define MIN_INT8  (int8)(0x80)
#define MAX_INT16 (int16)(0x7FFF)
#define MIN_INT16 (int16)(0x8000)
#define MAX_INT32 (int32)(0x7FFFFFFF)
#define MIN_INT32 (int32)(0x80000000)
#define MAX_INT64 (int64)(0x7FFFFFFFFFFFFFFF)
#define MIN_INT64 (int64)(0x8000000000000000)

#endif
