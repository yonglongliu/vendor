/*
 *Copyright 2014 NXP Semiconductors
 *
 *Licensed under the Apache License, Version 2.0 (the "License");
 *you may not use this file except in compliance with the License.
 *You may obtain a copy of the License at
 *            
 *http://www.apache.org/licenses/LICENSE-2.0
 *             
 *Unless required by applicable law or agreed to in writing, software
 *distributed under the License is distributed on an "AS IS" BASIS,
 *WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *See the License for the specific language governing permissions and
 *limitations under the License.
 */

/************************************************************************
 *  Module:       tbase_swap.h
 *  Description:
 *     swap bytes helpers                
 ************************************************************************/

#ifndef __tbase_swap_h__
#define __tbase_swap_h__

#if !defined(TBASE_COMPILER_DETECTED)
	#error Compiler not detected. tbase_platform must be included before.
#endif


/*
	This file defines the following macros.
	For better documentation the macros are shown as function prototypes.
	
	Reverse byte ordering of integers. These macros map to platform specific intrinsics if possible.
	T_UINT16 TB_SWAP_BYTES_16(T_UINT16 x)
	T_UINT32 TB_SWAP_BYTES_32(T_UINT32 x)
	T_UINT64 TB_SWAP_BYTES_64(T_UINT64 x)

	Reverse byte ordering of integers. These macros do never map to intrinsics but are based on expressions.
	These macros are to be used if x is a constant or literal, or in case of struct initializer lists, etc.
	T_UINT16 TB_SWAP_BYTES_16_exp(T_UINT16 x)
	T_UINT32 TB_SWAP_BYTES_32_exp(T_UINT32 x)
	T_UINT64 TB_SWAP_BYTES_64_exp(T_UINT64 x)


	These macros are used for little endian to native CPU format conversion.
	_exp variants are to be used if x is a constant or literal.
	T_UINT16 T_Cpu_from_LE_UINT16(T_LE_UINT16 x)
	T_UINT32 T_Cpu_from_LE_UINT32(T_LE_UINT32 x)
	T_UINT64 T_Cpu_from_LE_UINT64(T_LE_UINT64 x)
	T_UINT16 T_Cpu_from_LE_UINT16_exp(T_LE_UINT16 x)
	T_UINT32 T_Cpu_from_LE_UINT32_exp(T_LE_UINT32 x)
	T_UINT64 T_Cpu_from_LE_UINT64_exp(T_LE_UINT64 x)
	
	These macros are used for native CPU format to little endian conversion.
	_exp variants are to be used if x is a constant or literal.
	T_LE_UINT16 T_LE_UINT16_from_Cpu(T_UINT16 x)
	T_LE_UINT32 T_LE_UINT32_from_Cpu(T_UINT32 x)
	T_LE_UINT64 T_LE_UINT64_from_Cpu(T_UINT64 x)
	T_LE_UINT16 T_LE_UINT16_from_Cpu_exp(T_UINT16 x)
	T_LE_UINT32 T_LE_UINT32_from_Cpu_exp(T_UINT32 x)
	T_LE_UINT64 T_LE_UINT64_from_Cpu_exp(T_UINT64 x)

	These macros are used for big endian to native CPU format conversion.
	_exp variants are to be used if x is a constant or literal.
	T_UINT16 T_Cpu_from_BE_UINT16(T_BE_UINT16 x)
	T_UINT32 T_Cpu_from_BE_UINT32(T_BE_UINT32 x)
	T_UINT64 T_Cpu_from_BE_UINT64(T_BE_UINT64 x)
	T_UINT16 T_Cpu_from_BE_UINT16_exp(T_BE_UINT16 x)
	T_UINT32 T_Cpu_from_BE_UINT32_exp(T_BE_UINT32 x)
	T_UINT64 T_Cpu_from_BE_UINT64_exp(T_BE_UINT64 x)
	
	These macros are used for native CPU format to big endian conversion.
	_exp variants are to be used if x is a constant or literal.
	T_BE_UINT16 T_BE_UINT16_from_Cpu(T_UINT16 x)
	T_BE_UINT32 T_BE_UINT32_from_Cpu(T_UINT32 x)
	T_BE_UINT64 T_BE_UINT64_from_Cpu(T_UINT64 x)
	T_BE_UINT16 T_BE_UINT16_from_Cpu_exp(T_UINT16 x)
	T_BE_UINT32 T_BE_UINT32_from_Cpu_exp(T_UINT32 x)
	T_BE_UINT64 T_BE_UINT64_from_Cpu_exp(T_UINT64 x)
*/


/*
 * swap macros that are based on expressions,
 * result is calculated at compile time
 */
/*lint -emacro(648, TB_SWAP_BYTES_16_exp) */
#define	TB_SWAP_BYTES_16_exp(x) \
	(T_UINT16) \
	(((T_UINT16)(x)<<8) | \
	 ((T_UINT16)(x)>>8))

/*lint -emacro(648, TB_SWAP_BYTES_32_exp) */
#define	TB_SWAP_BYTES_32_exp(x) \
	(T_UINT32) \
	(((T_UINT32)(x)<<24) | \
	 (((T_UINT32)(x)<<8)&0x00FF0000UL) | \
	 (((T_UINT32)(x)>>8)&0x0000FF00UL) | \
	 ((T_UINT32)(x)>>24))

/*lint -emacro(648, TB_SWAP_BYTES_64_exp) */
#define	TB_SWAP_BYTES_64_exp(x) \
	(T_UINT64) \
	(((T_UINT64)(x) << 56) | \
	 (((T_UINT64)(x) << 40) & 0xff000000000000ULL) | \
	 (((T_UINT64)(x) << 24) & 0xff0000000000ULL) | \
	 (((T_UINT64)(x) << 8)  & 0xff00000000ULL) | \
	 (((T_UINT64)(x) >> 8)  & 0xff000000ULL) | \
	 (((T_UINT64)(x) >> 24) & 0xff0000ULL) | \
	 (((T_UINT64)(x) >> 40) & 0xff00ULL) | \
	 ((T_UINT64)(x)  >> 56))



/*
 * swapping macros (Microsoft specific)
 */
#ifdef TBASE_COMPILER_MICROSOFT
/* mapped to Microsoft specific intrinsics */
#include <stdlib.h>
#define	TB_SWAP_BYTES_16(x) _byteswap_ushort(x)
#define	TB_SWAP_BYTES_32(x) _byteswap_ulong(x)
#define	TB_SWAP_BYTES_64(x) _byteswap_uint64(x)
#endif

/* GNU 4.5 and later supports specific intrinsics */
#if defined(TBASE_COMPILER_GNU) && ( (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5) )
#define	TB_SWAP_BYTES_16(x) __builtin_bswap16(x)
#define	TB_SWAP_BYTES_32(x) __builtin_bswap32(x)
#define	TB_SWAP_BYTES_64(x) __builtin_bswap64(x)
#endif


/* 
 fall-back to expressions, if compiler is unknown. 
*/
#if !defined(TB_SWAP_BYTES_16)
#define	TB_SWAP_BYTES_16(x) TB_SWAP_BYTES_16_exp(x)
#endif

#if !defined(TB_SWAP_BYTES_32)
#define	TB_SWAP_BYTES_32(x) TB_SWAP_BYTES_32_exp(x)
#endif

#if !defined(TB_SWAP_BYTES_64)
#define	TB_SWAP_BYTES_64(x) TB_SWAP_BYTES_64_exp(x)
#endif



/*
 * conversion helpers
 * the _exp variants should be used for a constant x value, e.g. for struct initialization
 */

#if defined(TBASE_LITTLE_ENDIAN)

/* T_UINT16 -> T_LE_UINT16 */
#define T_LE_UINT16_from_Cpu(x)			(x)
#define T_LE_UINT16_from_Cpu_exp(x) (x)

/* T_UINT32 -> T_LE_UINT32 */
#define T_LE_UINT32_from_Cpu(x)			(x)
#define T_LE_UINT32_from_Cpu_exp(x) (x)

/* T_UINT64 -> T_LE_UINT64 */
#define T_LE_UINT64_from_Cpu(x)			(x)
#define T_LE_UINT64_from_Cpu_exp(x) (x)

/* T_LE_UINT16 -> T_UINT16 */
#define T_Cpu_from_LE_UINT16(x)			(x)
#define T_Cpu_from_LE_UINT16_exp(x) (x)

/* T_LE_UINT32 -> T_UINT32 */
#define T_Cpu_from_LE_UINT32(x)			(x)
#define T_Cpu_from_LE_UINT32_exp(x) (x)

/* T_LE_UINT64 -> T_UINT64 */
#define T_Cpu_from_LE_UINT64(x)			(x)
#define T_Cpu_from_LE_UINT64_exp(x) (x)


/* T_UINT16 -> T_BE_UINT16 */
#define T_BE_UINT16_from_Cpu(x)				TB_SWAP_BYTES_16(x)
#define T_BE_UINT16_from_Cpu_exp(x)		TB_SWAP_BYTES_16_exp(x)

/* T_UINT32 -> T_BE_UINT32 */
#define T_BE_UINT32_from_Cpu(x)				TB_SWAP_BYTES_32(x)
#define T_BE_UINT32_from_Cpu_exp(x)		TB_SWAP_BYTES_32_exp(x)

/* T_BE_UINT64 -> T_UINT64 */
#define T_BE_UINT64_from_Cpu(x)				TB_SWAP_BYTES_64(x)
#define T_BE_UINT64_from_Cpu_exp(x)		TB_SWAP_BYTES_64_exp(x)

/* T_BE_UINT16 -> T_UINT16 */
#define T_Cpu_from_BE_UINT16(x)				TB_SWAP_BYTES_16(x)
#define T_Cpu_from_BE_UINT16_exp(x)		TB_SWAP_BYTES_16_exp(x)

/* T_BE_UINT32 -> T_UINT32 */
#define T_Cpu_from_BE_UINT32(x)				TB_SWAP_BYTES_32(x)
#define T_Cpu_from_BE_UINT32_exp(x)		TB_SWAP_BYTES_32_exp(x)

/* T_BE_UINT64 -> T_UINT64 */
#define T_Cpu_from_BE_UINT64(x)				TB_SWAP_BYTES_64(x)
#define T_Cpu_from_BE_UINT64_exp(x)		TB_SWAP_BYTES_64_exp(x)

#endif /* TBASE_LITTLE_ENDIAN */

#if defined(TBASE_BIG_ENDIAN)

/* T_UINT16 -> T_LE_UINT16 */
#define T_LE_UINT16_from_Cpu(x)				TB_SWAP_BYTES_16(x)
#define T_LE_UINT16_from_Cpu_exp(x)		TB_SWAP_BYTES_16_exp(x)

/* T_UINT32 -> T_LE_UINT32 */
#define T_LE_UINT32_from_Cpu(x)				TB_SWAP_BYTES_32(x)
#define T_LE_UINT32_from_Cpu_exp(x)		TB_SWAP_BYTES_32_exp(x)

/* T_UINT64 -> T_LE_UINT64 */
#define T_LE_UINT64_from_Cpu(x)				TB_SWAP_BYTES_64(x)
#define T_LE_UINT64_from_Cpu_exp(x)		TB_SWAP_BYTES_64_exp(x)

/* T_LE_UINT16 -> T_UINT16 */
#define T_Cpu_from_LE_UINT16(x)				TB_SWAP_BYTES_16(x)
#define T_Cpu_from_LE_UINT16_exp(x)		TB_SWAP_BYTES_16_exp(x)

/* T_LE_UINT32 -> T_UINT32 */
#define T_Cpu_from_LE_UINT32(x)				TB_SWAP_BYTES_32(x)
#define T_Cpu_from_LE_UINT32_exp(x)		TB_SWAP_BYTES_32_exp(x)

/* T_LE_UINT64 -> T_UINT64 */
#define T_Cpu_from_LE_UINT64(x)				TB_SWAP_BYTES_64(x)
#define T_Cpu_from_LE_UINT64_exp(x)		TB_SWAP_BYTES_64_exp(x)


/* T_UINT16 -> T_BE_UINT16 */
#define T_BE_UINT16_from_Cpu(x)			(x)
#define T_BE_UINT16_from_Cpu_exp(x) (x)

/* T_UINT32 -> T_BE_UINT32 */
#define T_BE_UINT32_from_Cpu(x)			(x)
#define T_BE_UINT32_from_Cpu_exp(x) (x)

/* T_BE_UINT64 -> T_UINT64 */
#define T_BE_UINT64_from_Cpu(x)			(x)
#define T_BE_UINT64_from_Cpu_exp(x) (x)

/* T_BE_UINT16 -> T_UINT16 */
#define T_Cpu_from_BE_UINT16(x)			(x)
#define T_Cpu_from_BE_UINT16_exp(x) (x)

/* T_BE_UINT32 -> T_UINT32 */
#define T_Cpu_from_BE_UINT32(x)			(x)
#define T_Cpu_from_BE_UINT32_exp(x) (x)

/* T_BE_UINT64 -> T_UINT64 */
#define T_Cpu_from_BE_UINT64(x)			(x)
#define T_Cpu_from_BE_UINT64_exp(x) (x)

#endif /* TBASE_BIG_ENDIAN */


#endif  /* __tbase_swap_h__ */

/*************************** EOF **************************************/
