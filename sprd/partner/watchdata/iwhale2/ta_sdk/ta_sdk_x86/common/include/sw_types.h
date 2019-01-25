/**************************************************************
                Watchdata Platform Development Center
                       Watchdata Confidential
            Copyright (c) Watchdata System CO., Ltd. 2013 -
                        All Rights Reserved
                       http://www.watchdata.com
**************************************************************/
/*
 * Create Date:
 * 
 * Modify Record:
 *     <author>      <date>           <version>     <desc>
 *------------------------------------------------------------
 *	   dengping.niu  2013/11/25        1.0		    Create the file.
 */

/*
 * @file    : sw_types.h
 * @ingroup : common
 * @brief   : 通用参数类型定义
 * @author  : dengping.niu
 * @version   1.0
 * @date      2013/11/25
 */
#ifndef _TEE_TYPES_H_
#define _TEE_TYPES_H_


#include "stdlib.h"
#include "math.h"
#include "time.h"
/**
 * @brief 基本类型定义
 */

typedef unsigned char  uint8_t;
typedef   signed char  int8_t;
typedef unsigned short uint16_t;
typedef   signed short  int16_t;
typedef unsigned int   uint32_t;
typedef   signed int    int32_t;

typedef uint32_t        size_t;

/**
 * @brief 常量定义
 */
#ifndef NULL
#define NULL	((void*)0)
#endif

int printf(const char*, ...);

#if 1
#define err_log(fmt,arg...)	{\
					printf("[ERR][Func: %s() Line: %d]", __func__, __LINE__);\
					printf(fmt,##arg);\
					printf("\n");\
				}
#else
#define err_log(fmt,arg...)
#endif

#endif /*_TEE_TYPES_H_*/
