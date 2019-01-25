/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 24709 $ $Date: 2014-02-20 17:27:12 +0800 (Thu, 20 Feb 2014) $
 */

#ifndef _EZXML_LIST_H_
#define _EZXML_LIST_H_

#include <osal.h>

typedef OSAL_SemId EZXML_SemId;

typedef struct EZXML_Entry{
    union {
        struct {
            uvint  objType;
            void  *data __attribute__ ((packed));
        }e; 
        struct {
            struct EZXML_Entry *pNext;
            struct EZXML_Entry *pPrev;
        }p;
    }u;
    
#ifdef EZXML_DEBUG_LOG
    uint32    inUse;
#endif
} EZXML_ListEntry;

typedef struct {
    EZXML_ListEntry *pHead;
    EZXML_ListEntry *pTail;
    vint            isBackwards;
    EZXML_SemId       lock;
    char           *listName;
    int             cnt;
} EZXML_List;

void EZXML_initEntry(
    EZXML_ListEntry *pEntry);

void EZXML_initList(
    EZXML_List    *pDLL);

EZXML_ListEntry * EZXML_dequeue(
    EZXML_List *pDLL);

int EZXML_enqueue(
    EZXML_List *pDLL, 
    EZXML_ListEntry *pEntry);


#endif
