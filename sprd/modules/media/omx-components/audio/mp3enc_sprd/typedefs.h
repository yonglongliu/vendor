/***********************************************
** File Name:     typedefs.h
** Author:        Fei Xie
** Date:          11/09/01
**
** Description:
** Note:
** CopyRight:     SpreadTrum Communications CORP. All Rights Reserved

  ************************************************
  **           Edit History
  ** Name          Date             Descripotion
  ** fei.xie       2001-11-09       create
  ** Xiaoyu Wang	 2007-09-03       Migrate to STiMi
  ** Michael Chen	 2007-09-10       Migrate to T-DMB
************************************************/

#ifndef TYPEDEFS_H

/****************MACRO define*****************************************************/

#define TYPEDEFS_H


//#define MP3_ENC_TEST_DATA


#define MAX_32 (word32)	0x7fffffffL
#define MIN_32 (word32)	0x80000000L

#define MAX_16 (word16)	0x7fff
#define MIN_16 (word16)	0x8000




typedef enum
{
    FALSE = 0,
		TRUE  = 1
} boolean;

typedef signed char word8;
typedef short word16;
typedef long word32;
typedef int flag;

typedef char      int8;
typedef short int int16;
typedef long  int int32;
#ifdef WIN32
typedef __int64        int64;

//////////////////////////////////////////////////////////////////////////
typedef double          FLOAT;
#else
typedef long long int64;
#endif
typedef unsigned char      uint8;
typedef unsigned short int uint16;
typedef unsigned long  int uint32;

typedef unsigned char BOOLEAN;

#define REG(rigister)	*(volatile uint16*)(rigister)

#define HIGH_16BIT(value)    ((uint32)value>>16)&0xffff
#define LOW_16BIT(value)    ((uint32)value)&0xffff
#define M32_16_rs(r32,r16,rs) ((((r32)>>16)*(r16)+((((r32)&65535)*(r16))>>16))>>(rs))
#define M32_16(r32,r16) (((r32)>>16)*(r16)+((((r32)&65535)*(r16))>>16))

/* temp use for unknown parameters in coding stage*/
#define UNKNOWN 	0

/* for those parameters which are no-use in current stage */
#define NO_USE		0

/* for those parameters which will change before DMA start */
//#define NO_INIT		0

#define TBD			0

#define CMD_CONTROL_FLOW

#define POWER_SAVING

#define LDPC_INT_PROTECT

//#define USB_LOG_USE
//#define UART_LOG_USE
//#define MP3_ENC_TEST_DATA
extern int32 g_frame;
#define FRAME_NUMBER 163
#include <stdio.h>
extern FILE *fp_out;
//#define MP3_ENC_BIT_WRITE_IMPROVE 
#define MP3_ENC_32BIT


#ifdef MP3_ENC_32BIT
#define MP3_ENC_BIT_LEFT 32
#else
#define MP3_ENC_BIT_LEFT 8
#endif
#endif
