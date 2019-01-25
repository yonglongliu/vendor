/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28342 $ $Date: 2014-08-19 17:37:28 +0800 (Tue, 19 Aug 2014) $
 */
#ifndef _EZXML_MEM_H_
#define _EZXML_MEM_H_

#include "_ezxml_list.h"

#define EZXML_PORT_LOCK_INIT       NULL

/* Maximum concurrent used memory size. */
#ifdef INCLUDE_TAPP
#define EZXML_MEM_POOL_SZ_8         (512)
#define EZXML_MEM_POOL_SZ_16        (512) 
#define EZXML_MEM_POOL_SZ_32        (512)
#define EZXML_MEM_POOL_SZ_64        (512)
#define EZXML_MEM_POOL_SZ_256       (512)
#define EZXML_MEM_POOL_SZ_512       (64)
#define EZXML_MEM_POOL_SZ_2048      (10)
#define EZXML_MEM_POOL_SZ_4096      (4)
#define EZXML_MEM_POOL_SZ_8192      (1)
#define EZXML_MEM_POOL_SZ_65536     (1)
#elif INCLUDE_NVRAM
#define EZXML_MEM_POOL_SZ_8         (8)
#define EZXML_MEM_POOL_SZ_16        (8) 
#define EZXML_MEM_POOL_SZ_32        (8)
#define EZXML_MEM_POOL_SZ_64        (32)
#define EZXML_MEM_POOL_SZ_256       (16)
#define EZXML_MEM_POOL_SZ_512       (8)
#define EZXML_MEM_POOL_SZ_2048      (1)
#define EZXML_MEM_POOL_SZ_4096      (0)
#define EZXML_MEM_POOL_SZ_8192      (0)
#define EZXML_MEM_POOL_SZ_65536     (0)
#else
/* For XML configuration file */
#define EZXML_MEM_POOL_SZ_8         (128)
#define EZXML_MEM_POOL_SZ_16        (128) 
#define EZXML_MEM_POOL_SZ_32        (128)
#define EZXML_MEM_POOL_SZ_64        (128)
#define EZXML_MEM_POOL_SZ_256       (8)
#define EZXML_MEM_POOL_SZ_512       (4)
#define EZXML_MEM_POOL_SZ_2048      (4)
#define EZXML_MEM_POOL_SZ_4096      (1)
#define EZXML_MEM_POOL_SZ_8192      (1)
#define EZXML_MEM_POOL_SZ_65536     (0)
#endif
/* enum of memory size type */
typedef enum {
    EZXML_MEM_TYPE_FIRST,
    EZXML_MEM_TYPE_8 = EZXML_MEM_TYPE_FIRST,
    EZXML_MEM_TYPE_16,
    EZXML_MEM_TYPE_32,
    EZXML_MEM_TYPE_64,
    EZXML_MEM_TYPE_256,
    EZXML_MEM_TYPE_512,
    EZXML_MEM_TYPE_2048,
    EZXML_MEM_TYPE_4096,
    EZXML_MEM_TYPE_8192,
    EZXML_MEM_TYPE_65536,
    EZXML_MEM_TYPE_LAST,
}EZXML_memType;

typedef struct {
    EZXML_memType ezMemType;
    vint        ezObjSize;
    vint        ezPoolSize;
    EZXML_List  ezPoolList; 
#ifdef EZXML_DEBUG_LOG
    vint        ezMemMinSz; 
                /*
                 * Record for pool usage. 
                 * It's minimum remaining pool size,  minus means miss count
                 */
    vint        ezMemCurSz; /* Current pool size */
#endif
}EZXML_MemPool;

vint EZXML_init(
    void);

void *EZXML_memPoolAlloc(
    EZXML_memType objType);

void EZXML_memPoolFree(
    void *mem_ptr);

void EZXML_destroy(
    void);

int  EZXML_semMutexCreate(EZXML_SemId *lock_ptr);

int  EZXML_semDelete(EZXML_SemId lock);

int  EZXML_semAcquire(EZXML_SemId lock);

int  EZXML_semGive(EZXML_SemId lock);

void * EZXML_memPoolReAlloc(
    void  *mem_ptr,
    vint   size);

void* EZXML_memAlloc(
    vint size,
    vint memArg);

void EZXML_memFree(
    void *mem_ptr,
    vint memArg);

void * EZXML_memReAlloc(
    void  *mem_ptr,
    vint   size,
    vint   memArg);

void EZXML_memLogReset(void);

char * EZXML_strdup(const char *s);

#endif
