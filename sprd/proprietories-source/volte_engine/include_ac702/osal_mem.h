/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 7023 $ $Date: 2008-07-17 11:31:01 -0400 (Thu, 17 Jul 2008) $
 *
 */

#ifndef _OSAL_MEM_H_
#define _OSAL_MEM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "osal_types.h"
#include "osal_constant.h"
#include "osal_platform.h"

#define OSAL_MEM_ARG_STATIC_ALLOC   (0)
#define OSAL_MEM_ARG_DYNAMIC_ALLOC  (1)
void OSAL_memSet(
    void    *mem_ptr, 
    int32    value,   
    int32    size);

void OSAL_memCpy(
    void        *dest_ptr, 
    const void  *src_ptr,
    int32        size);

int OSAL_memCmp(
    const void *mem1_ptr,
    const void *mem2_ptr,
    int32       size);

void * OSAL_memAlloc(
    int32 numBytes,
    int32 memArg);

void * OSAL_memCalloc(
    int32 numElements,
    int32 elementSize,
    int32 memArg);

OSAL_Status OSAL_memFree(
    void  *mem_ptr,
    int32  memArg);

void OSAL_memMove(
    void        *dest_ptr,
    const void  *src_ptr,
    int32        size);

void * OSAL_memReAlloc(
    void  *mem_ptr,
    int32  numBytes,
    int32  memArg);

#ifdef __cplusplus
}
#endif

#endif
