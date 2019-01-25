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
 *  Module:       tbase_utils.h
 *  Description:
 *     base utilities                
 ************************************************************************/

#ifndef __tbase_utils_h__
#define __tbase_utils_h__

#ifndef TB_DEBUG
#error TB_DEBUG is not defined.
#endif

/* Calculate the byte offset of a member within a struct. */
/* mapped to the standard macro offsetof from stddef.h */
/* NOTE: This cannot be used for C++ classes, except with the MS compiler. */
/*lint -emacro(413, TB_OFFSET_OF_MEMBER) */
/*lint -emacro(413, offsetof) */
/*lint -emacro(545, offsetof) */
#ifdef _lint
/* always use standard offsetof operator for lint */
#define TB_OFFSET_OF_MEMBER(structType, memberName) offsetof(structType, memberName)
#else
#ifdef TBASE_COMPILER_GNU
/* GNU supports a special intrinsic that should be used to avoid warnings with TB_COMPILE_TIME_ASSERT */
#define TB_OFFSET_OF_MEMBER(structType, memberName) __builtin_offsetof(structType, memberName)
#else
/* standard offsetof operator, typically a macro in stddef.h */
#define TB_OFFSET_OF_MEMBER(structType, memberName) offsetof(structType, memberName)
#endif
#endif


/* Calculate the pointer to the base of a surrounding struct given its type */
/* and a pointer to a field within the struct. */
/* NOTE: This cannot be used for C++ classes. */
/*lint -emacro(413, TB_BASE_POINTER_FROM_MEMBER) */
/*lint -emacro(826, TB_BASE_POINTER_FROM_MEMBER) */
#define TB_BASE_POINTER_FROM_MEMBER(memberPtr, structType, memberName) \
    ( (structType*)(((char*)(memberPtr)) - TB_OFFSET_OF_MEMBER(structType,memberName)) )


/* minimum of two values */
#define TB_MIN(a,b)	( ((a)<(b)) ? (a) : (b) )

/* maximum of two values */
#define TB_MAX(a,b)	( ((a)>(b)) ? (a) : (b) )

/* limit a value to a specified range */
#define TB_MINMAX(x,min,max)  TB_MIN(max,TB_MAX(min,x))

/* return the absolute value of a signed char, short or int */
#define TB_ABS(x)	( ((x) < 0) ? -(x) : (x) )


/* number of elements of an array */
#define TB_ARRAY_ELEMENTS(a)	( sizeof(a)/sizeof(a[0]) )

/* pointer behind the array */
#define TB_ARRAY_LIMIT(a)		( &a[TB_ARRAY_ELEMENTS(a)] )


/* zero a struct instance, typically a local variable */
#define TB_ZERO_STRUCT(s)   TbZeroMemory(&(s),sizeof(s))

/* zero an array instance, typically a local variable */
#define TB_ZERO_ARRAY(a)    TbZeroMemory((a),sizeof(a))



/* macro to generate bit masks */
#define TB_BITMASK(bitnb)		( 0x1U<<(bitnb) )

/* test a single bit */
#define TB_TEST_BIT(val,bitnb)		( ((val) & TB_BITMASK(bitnb)) != 0 )
/* set a single bit */
#define TB_SET_BIT(val,bitnb)			{ ((val) |= TB_BITMASK(bitnb)); }
/* clear a single bit */
#define TB_CLEAR_BIT(val,bitnb)		{ ((val) &= ~TB_BITMASK(bitnb)); }



/* This macro generates a magic DWORD (four character code). */
/* byte order in memory is: c1 c2 c3 c4 */
#define TB_FOUR_CHAR_DWORD_BE(c1,c2,c3,c4)	( ((T_UINT32)(c4))|(((T_UINT32)(c3))<<8)|(((T_UINT32)(c2))<<16)|(((T_UINT32)(c1))<<24) )
#define TB_FOUR_CHAR_DWORD_LE(c1,c2,c3,c4)	( ((T_UINT32)(c1))|(((T_UINT32)(c2))<<8)|(((T_UINT32)(c3))<<16)|(((T_UINT32)(c4))<<24) )
#if defined(TBASE_BIG_ENDIAN)
#define TB_FOUR_CHAR_DWORD(c1,c2,c3,c4)	TB_FOUR_CHAR_DWORD_BE(c1,c2,c3,c4)
#else
#define TB_FOUR_CHAR_DWORD(c1,c2,c3,c4)	TB_FOUR_CHAR_DWORD_LE(c1,c2,c3,c4)
#endif


/* helper macro, used to convert enum constants to string values */
//lint -esym(773, TB_ENUMSTR)
//lint -esym(773, TB_ENUMSTR_W)
#define TB_ENUMSTR(e) (x==(e)) ? #e
#define TB_ENUMSTR_W(e) (x==(e)) ? L#e

/* helper macro: convert number to string */
#define TB_TO_STRING(x)     TB_TO_STRING_impl(x)
#define TB_TO_STRING_impl(x)   #x


/* lint complaints about 'Suspicious use of &' for these macros */
/*lint -emacro(545, TB_IS_ALIGNED) */
/*lint -emacro(545, TB_ALIGN_UP) */
/*lint -emacro(545, TB_ALIGN_DOWN) */

/* returns true if the given value is aligned to a 'size' boundary, size MUST BE a power of 2 */
#define TB_IS_ALIGNED(val,size)	( (((size_t)(val)) & ((size)-1)) == 0 )

/* round up a value to the next 'size' boundary, size MUST BE a power of 2 */
#define TB_ALIGN_UP(val,size)		( ((val) + ((size)-1)) & ~((size)-1) )

/* round down a value to the next 'size' boundary, size MUST BE a power of 2 */
#define TB_ALIGN_DOWN(val,size)	( (val) & ~((size)-1) )

/* round up val to the next integer multiple of mul */
#define TB_ROUND_UP(val, mul)    ( (((val) + ((mul)-1)) / (mul)) * (mul) )

/* round down val to the next integer multiple of mul */
#define TB_ROUND_DOWN(val, mul)   ( ((val) / (mul)) * (mul) )

/* divide dividend by divisor, round up to next integer, DOES NOT work for negative values */
#define TB_DIVIDE_ROUND_UP(dividend,divisor) ( ((dividend) + ((divisor)-1)) / (divisor) )

/* divide dividend by divisor, round mathematically, DOES NOT work for negative values */
#define TB_DIVIDE_ROUND_MATH(dividend,divisor) ( ((dividend) + ((divisor)/2)) / (divisor) )


/* Extract single bytes from larger integer types. */
/* should be used with unsigned integers only */
/* byte0 is the least significant byte, byte3 is the most significant byte */
/* lint might complain about 'Constant expression evaluates to 0 in operation '&'' if the result is 0 */
/*lint -emacro(778, TB_BYTE0) */
/*lint -emacro(778, TB_BYTE1) */
/*lint -emacro(778, TB_BYTE2) */
/*lint -emacro(778, TB_BYTE3) */
/* lint might complain about 'Excessive shift value (precision x shifted right by y)' when using constants/enums */
/*lint -emacro(572, TB_BYTE0) */
/*lint -emacro(572, TB_BYTE1) */
/*lint -emacro(572, TB_BYTE2) */
/*lint -emacro(572, TB_BYTE3) */
#define TB_BYTE0(x)			( (T_UINT8)((x) & 0xFF) )
#define TB_BYTE1(x)			( (T_UINT8)(((x)>>8) & 0xFF) )
#define TB_BYTE2(x)			( (T_UINT8)(((x)>>16) & 0xFF) )
#define TB_BYTE3(x)			( (T_UINT8)(((x)>>24) & 0xFF) )

/* Build larger integer types from single bytes. */
/* byte0 is the least significant byte, byte3 is the most significant byte */
#define TB_UINT16_FROM_BYTES(byte0, byte1) \
						( \
							( ((T_UINT16)(T_UINT8)(byte0)) ) | \
							( ((T_UINT16)(T_UINT8)(byte1)) << 8 ) \
						)
						
#define TB_UINT32_FROM_BYTES(byte0, byte1, byte2, byte3) \
						( \
							( ((UINT32)(T_UINT8)(byte0)) ) | \
							( ((UINT32)(T_UINT8)(byte1)) << 8 ) | \
							( ((UINT32)(T_UINT8)(byte2)) << 16 ) | \
							( ((UINT32)(T_UINT8)(byte3)) << 24 ) \
						)

#define TB_UINT64_FROM_BYTES(byte0, byte1, byte2, byte3, byte4, byte5, byte6, byte7) \
						( \
							( ((UINT64)(T_UINT8)(byte0)) ) | \
							( ((UINT64)(T_UINT8)(byte1)) << 8 ) | \
							( ((UINT64)(T_UINT8)(byte2)) << 16 ) | \
							( ((UINT64)(T_UINT8)(byte3)) << 24 ) | \
							( ((UINT64)(T_UINT8)(byte4)) << 32 ) | \
							( ((UINT64)(T_UINT8)(byte5)) << 40 ) | \
							( ((UINT64)(T_UINT8)(byte6)) << 48 ) | \
							( ((UINT64)(T_UINT8)(byte7)) << 56 ) \
						)


/*
 * convert integer (range = 0..15) to ASCII character
 * The function returns a char from the range '0'..'9' or 'A'..'F'
 */
#define TB_HEX_UCASE_CHAR_FROM_INT(x)	( (char)((((x)&0xF) <= 9) ? ('0' + ((x)&0xF)) : (('A' - 10) + ((x)&0xF))) )

/*
 * convert integer (range = 0..15) to ASCII character
 * The function returns a char from the range '0'..'9' or 'a'..'f'
 */
#define TB_HEX_LCASE_CHAR_FROM_INT(x)	( (char)((((x)&0xF) <= 9) ? ('0' + ((x)&0xF)) : (('a' - 10) + ((x)&0xF))) )


/*
 * Convert a character from the range '0'..'9' or 'A'..'F' or 'a'..'f'
 * to its corresponding integer value (range 0..15).
 * The macro returns -1 if an invalid char is passed.
 */
#define TB_INT_FROM_HEX_CHAR(c)	( \
						((c)>='0' && (c)<='9') ? ((c) - '0') : \
						((c)>='A' && (c)<='F') ? ((c) - ('A' - 10)) :  \
						((c)>='a' && (c)<='f') ? ((c) - ('a' - 10)) :  -1 )



/*
 * extract BCD digit for power of 10
 * pwr = 1, 10, 100, 1000, ...
 */
#define TB_BCD_DIGIT(pwr, x)			( ((x)/(pwr)) % 10 )

/*
 * convert number to byte in BCD format, 
 * e.g. 0x0A converts to 0x10
 */
#define TB_BCD_BYTE_FROM_UINT(x)  ( (TB_BCD_DIGIT(10,(x)) << 4) | TB_BCD_DIGIT(1,(x)) )

/*
 * convert number in BCD format to uint
 * e.g. 0x10 converts to 0x0A
*/
#define TB_UINT_FROM_BCD_BYTE(x)  ( ((((x) >> 4) & 0xF) * 10) + ((x) & 0xF) )



/*
 * Returns true if the given character is a digit between 0..9
 */
#define TB_IS_DIGIT_CHAR(c)	( (c)>='0' && (c)<='9' )




/* macro to mark function arguments or variables unused, this will quieten lint and /W4 */
#define TB_UNUSED(x)			 { (void)x; }
#define TB_UNUSED_PARAM(x) { (void)x; }

/* macro to mark a variable unused, this will suppress warnings in release builds */
/* if ASSERTs are dummy macros*/
#if TB_DEBUG
#define TB_RELEASE_BUILD_UNUSED(x)
#else
#define TB_RELEASE_BUILD_UNUSED(x) { (void)x; }
#endif

#endif  /* __tbase_utils_h__ */

/*************************** EOF **************************************/
