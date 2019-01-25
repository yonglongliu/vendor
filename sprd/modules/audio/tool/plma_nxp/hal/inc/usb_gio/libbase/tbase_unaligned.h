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
 *  Module:       tbase_unaligned.h
 *  Description:
 *     unaligned integer access
 ************************************************************************/

#ifndef __tbase_unaligned_h__
#define __tbase_unaligned_h__


/*
	This file defines the following macros and inline functions.
	For better documentation the macros are shown as function prototypes.

* Read an integer from a (possibly) unaligned address.
  The returned integer has native CPU endianess.

	little endian memory layout:
	T_UINT16 TB_READ_UNALIGNED_LE16(const T_UINT8* addr); 
	T_UINT32 TB_READ_UNALIGNED_LE32(const T_UINT8* addr); 
	T_UINT64 TB_READ_UNALIGNED_LE64(const T_UINT8* addr); 

	big endian memory layout:
	T_UINT16 TB_READ_UNALIGNED_BE16(const T_UINT8* addr); 
	T_UINT32 TB_READ_UNALIGNED_BE32(const T_UINT8* addr); 
	T_UINT64 TB_READ_UNALIGNED_BE64(const T_UINT8* addr); 

	memory layout corresponds to CPU native format:
	T_UINT16 TB_READ_UNALIGNED_UINT16(const T_UINT8* addr); 
	T_UINT32 TB_READ_UNALIGNED_UINT32(const T_UINT8* addr); 
	T_UINT64 TB_READ_UNALIGNED_UINT64(const T_UINT8* addr); 


* Write an integer to a (possibly) unaligned address.
	The integer provided in val has native CPU endianess.
	
	little endian memory layout:
	void TB_WRITE_UNALIGNED_LE16(const T_UINT8* addr, T_UINT16 val);
	void TB_WRITE_UNALIGNED_LE32(const T_UINT8* addr, T_UINT32 val);
	void TB_WRITE_UNALIGNED_LE64(const T_UINT8* addr, T_UINT64 val);

	big endian memory layout:
	void TB_WRITE_UNALIGNED_BE16(const T_UINT8* addr, T_UINT16 val);
	void TB_WRITE_UNALIGNED_BE32(const T_UINT8* addr, T_UINT32 val);
	void TB_WRITE_UNALIGNED_BE64(const T_UINT8* addr, T_UINT64 val);

	memory layout corresponds to CPU native format:
	void TB_WRITE_UNALIGNED_UINT16(const T_UINT8* addr, T_UINT16 val);
	void TB_WRITE_UNALIGNED_UINT32(const T_UINT8* addr, T_UINT32 val);
	void TB_WRITE_UNALIGNED_UINT64(const T_UINT8* addr, T_UINT64 val);

*/


//
// little endian memory layout
//

static 
TB_INLINE 
T_UINT16 
TB_READ_UNALIGNED_LE16(TB_PACKED_PTR const void* addr)
{
	const T_UINT8* p = (const T_UINT8*)addr;
	return (
		( ((T_UINT16)p[0]) ) |
		( ((T_UINT16)p[1]) << 8 )
	);
}

static 
TB_INLINE 
T_UINT32 
TB_READ_UNALIGNED_LE32(TB_PACKED_PTR const void* addr)
{
	const T_UINT8* p = (const T_UINT8*)addr;
	return (
		( ((T_UINT32)p[0]) ) |
		( ((T_UINT32)p[1]) << 8 ) |
		( ((T_UINT32)p[2]) << 16 ) |
		( ((T_UINT32)p[3]) << 24 )
	);
}

static 
TB_INLINE 
T_UINT64 
TB_READ_UNALIGNED_LE64(TB_PACKED_PTR const void* addr)
{
	const T_UINT8* p = (const T_UINT8*)addr;
	return (
		( ((T_UINT64)p[0]) ) |
		( ((T_UINT64)p[1]) << 8 ) |
		( ((T_UINT64)p[2]) << 16 ) |
		( ((T_UINT64)p[3]) << 24 ) |
		( ((T_UINT64)p[4]) << 32 ) |
		( ((T_UINT64)p[5]) << 40 ) |
		( ((T_UINT64)p[6]) << 48 ) |
		( ((T_UINT64)p[7]) << 56 )
	);
}


//
// big endian memory layout
//

static 
TB_INLINE 
T_UINT16 
TB_READ_UNALIGNED_BE16(TB_PACKED_PTR const void* addr)
{
	const T_UINT8* p = (const T_UINT8*)addr;
	return (
		( ((T_UINT16)p[0]) << 8 ) |
		( ((T_UINT16)p[1]) )
	);
}

static 
TB_INLINE 
T_UINT32 
TB_READ_UNALIGNED_BE32(TB_PACKED_PTR const void* addr)
{
	const T_UINT8* p = (const T_UINT8*)addr;
	return (
		( ((T_UINT32)p[0]) << 24 ) |
		( ((T_UINT32)p[1]) << 16 ) |
		( ((T_UINT32)p[2]) << 8 ) |
		( ((T_UINT32)p[3]) )
	);
}

static 
TB_INLINE 
T_UINT64 
TB_READ_UNALIGNED_BE64(TB_PACKED_PTR const void* addr)
{
	const T_UINT8* p = (const T_UINT8*)addr;
	return (
		( ((T_UINT64)p[0]) << 56 ) |
		( ((T_UINT64)p[1]) << 48 ) |
		( ((T_UINT64)p[2]) << 40 ) |
		( ((T_UINT64)p[3]) << 32 ) |
		( ((T_UINT64)p[4]) << 24 ) |
		( ((T_UINT64)p[5]) << 16 ) |
		( ((T_UINT64)p[6]) << 8 ) |
		( ((T_UINT64)p[7]) )
	);
}



//
// little endian memory layout
//

static 
TB_INLINE 
void
TB_WRITE_UNALIGNED_LE16(TB_PACKED_PTR void* addr, T_UINT16 val)
{
	T_UINT8* p = (T_UINT8*)addr;
	p[0] = (T_UINT8)(val);
	p[1] = (T_UINT8)(val >> 8);
}

static 
TB_INLINE 
void
TB_WRITE_UNALIGNED_LE32(TB_PACKED_PTR void* addr, T_UINT32 val)
{
	T_UINT8* p = (T_UINT8*)addr;
	p[0] = (T_UINT8)(val);
	p[1] = (T_UINT8)(val >> 8);
	p[2] = (T_UINT8)(val >> 16);
	p[3] = (T_UINT8)(val >> 24);
}

static 
TB_INLINE 
void
TB_WRITE_UNALIGNED_LE64(TB_PACKED_PTR void* addr, T_UINT64 val)
{
	T_UINT8* p = (T_UINT8*)addr;
	p[0] = (T_UINT8)(val);
	p[1] = (T_UINT8)(val >> 8);
	p[2] = (T_UINT8)(val >> 16);
	p[3] = (T_UINT8)(val >> 24);
	p[4] = (T_UINT8)(val >> 32);
	p[5] = (T_UINT8)(val >> 40);
	p[6] = (T_UINT8)(val >> 48);
	p[7] = (T_UINT8)(val >> 56);
}


//
// big endian memory layout
//

static 
TB_INLINE 
void
TB_WRITE_UNALIGNED_BE16(TB_PACKED_PTR void* addr, T_UINT16 val)
{
	T_UINT8* p = (T_UINT8*)addr;
	p[0] = (T_UINT8)(val >> 8);
	p[1] = (T_UINT8)(val);
}

static 
TB_INLINE 
void
TB_WRITE_UNALIGNED_BE32(TB_PACKED_PTR void* addr, T_UINT32 val)
{
	T_UINT8* p = (T_UINT8*)addr;
	p[0] = (T_UINT8)(val >> 24);
	p[1] = (T_UINT8)(val >> 16);
	p[2] = (T_UINT8)(val >> 8);
	p[3] = (T_UINT8)(val);
}

static 
TB_INLINE 
void
TB_WRITE_UNALIGNED_BE64(TB_PACKED_PTR void* addr, T_UINT64 val)
{
	T_UINT8* p = (T_UINT8*)addr;
	p[0] = (T_UINT8)(val >> 56);
	p[1] = (T_UINT8)(val >> 48);
	p[2] = (T_UINT8)(val >> 40);
	p[3] = (T_UINT8)(val >> 32);
	p[4] = (T_UINT8)(val >> 24);
	p[5] = (T_UINT8)(val >> 16);
	p[6] = (T_UINT8)(val >> 8);
	p[7] = (T_UINT8)(val);
}



//
// native CPU memory layout
//

#if defined(TBASE_LITTLE_ENDIAN)

#define TB_READ_UNALIGNED_UINT16(addr)		TB_READ_UNALIGNED_LE16(addr)
#define TB_READ_UNALIGNED_UINT32(addr)		TB_READ_UNALIGNED_LE32(addr)
#define TB_READ_UNALIGNED_UINT64(addr)		TB_READ_UNALIGNED_LE64(addr)

#define TB_WRITE_UNALIGNED_UINT16(addr, val)		TB_WRITE_UNALIGNED_LE16((addr), (val))
#define TB_WRITE_UNALIGNED_UINT32(addr, val)		TB_WRITE_UNALIGNED_LE32((addr), (val))
#define TB_WRITE_UNALIGNED_UINT64(addr, val)		TB_WRITE_UNALIGNED_LE64((addr), (val))

#elif defined(TBASE_BIG_ENDIAN)

#define TB_READ_UNALIGNED_UINT16(addr)		TB_READ_UNALIGNED_BE16(addr)
#define TB_READ_UNALIGNED_UINT32(addr)		TB_READ_UNALIGNED_BE32(addr)
#define TB_READ_UNALIGNED_UINT64(addr)		TB_READ_UNALIGNED_BE64(addr)

#define TB_WRITE_UNALIGNED_UINT16(addr, val)		TB_WRITE_UNALIGNED_BE16((addr), (val))
#define TB_WRITE_UNALIGNED_UINT32(addr, val)		TB_WRITE_UNALIGNED_BE32((addr), (val))
#define TB_WRITE_UNALIGNED_UINT64(addr, val)		TB_WRITE_UNALIGNED_BE64((addr), (val))

#else
#error Endianess not defined.
#endif



#endif  /* __tbase_unaligned_h__ */

/*************************** EOF **************************************/
