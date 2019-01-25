/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 5864 $ $Date: 2008-04-09 11:09:10 -0400 (Wed, 09 Apr 2008) $
 *
 */

/*
 * Guidelines for queue usage:
 * 1. Queue IDs and Queue group IDs cannot be shared across tasks/threads.
 * 2. Each task/thread must create its own set of queues and queue groups
 * 3. All tasks/threads wishing to use a queue must first create it in their own
 *    context.
 * 4. Queues are identified by names and not by OSAL q IDs. OSAL q ID for unique
 *    queue is not unique across threads/tasks
 * 5. Kernel space (for vtspr) does not implement message queue groups
 * 5. Kernel space (for vtspr) does not implement message queue timeouts
 */

#ifndef _OSAL_MSG_H_
#define _OSAL_MSG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "osal_types.h"
#include "osal_constant.h"
#include "osal_platform.h"

#define OSAL_MSG_GROUP_MAX_QUEUES (16)
#define OSAL_MSG_SIZE_MAX         (4096)
#define OSAL_MSG_PATH_NAME_SIZE_MAX (256)

/* 
 * Declaring OSAL_msgQPathName as extern so that 
 * it can be accessed across files 
 */
extern char OSAL_msgQPathName[OSAL_MSG_PATH_NAME_SIZE_MAX];

typedef void    *OSAL_MsgQId;
typedef void    *OSAL_MsgQGrpId;

OSAL_MsgQId OSAL_msgQCreate(
    char   *name_ptr,
    vint    srcModId,
    vint    dstModId,
    vint    msgStructId,
    uint32  maxMsgs,
    uint32  lenOfMsg,
    uint32  flags);

OSAL_Status OSAL_msgQDelete(
    OSAL_MsgQId qId);

OSAL_Status OSAL_msgQSend(
    OSAL_MsgQId   qId,
    void         *buffer_ptr,
    uint32        octets,
    uint32        msTimeout,
    OSAL_Boolean *timeout_ptr);

int32 OSAL_msgQRecv(
    OSAL_MsgQId   qId,
    void         *buffer_ptr,
    uint32        octets,
    uint32        msTimeout,
    OSAL_Boolean *timeout_ptr);

OSAL_Status OSAL_msgQGrpCreate(
    OSAL_MsgQGrpId *g_ptr);

OSAL_Status OSAL_msgQGrpDelete(
    OSAL_MsgQGrpId *g_ptr);

OSAL_Status OSAL_msgQGrpAddQ(
    OSAL_MsgQGrpId *g_ptr,
    OSAL_MsgQId     qId);

int32 OSAL_msgQGrpRecv(
    OSAL_MsgQGrpId *g_ptr,
    void           *buffer_ptr,
    uint32          octets,
    uint32          msTimeout,
    OSAL_MsgQId    *qId_ptr,
    OSAL_Boolean   *timeout_ptr);

OSAL_Status OSAL_msgQSimpPut(
    OSAL_MsgQId qId,
    int32       val,
    uint32      msTimeout);

OSAL_Status OSAL_msgQSimpGet(
    OSAL_MsgQId  qId,
    int32       *val_ptr,
    uint32       msTimeout);

OSAL_Status OSAL_msgQSimpDelete(
    OSAL_MsgQId qId);

OSAL_MsgQId OSAL_msgQSimpInit(
    int size);


#ifdef __cplusplus
}
#endif

#endif
