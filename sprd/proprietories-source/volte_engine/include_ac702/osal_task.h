/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 7023 $ $Date: 2008-07-17 11:31:01 -0400 (Thu, 17 Jul 2008) $
 *
 */

#ifndef _OSAL_TASK_H_
#define _OSAL_TASK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "osal_types.h"
#include "osal_constant.h"
#include "osal_platform.h"

typedef void *                  OSAL_TaskId;
typedef int32                   OSAL_TaskReturn;
typedef void *                  OSAL_TaskArg;
typedef OSAL_TaskReturn       (*OSAL_TaskPtr)(OSAL_TaskArg);

OSAL_TaskId OSAL_taskCreate(
    char          *name_ptr,
    OSAL_TaskPrio  priority,
    uint32         stackByteSize,
    OSAL_TaskPtr   fx_ptr,
    OSAL_TaskArg   arg);

OSAL_Status OSAL_taskDelete(
    OSAL_TaskId tId);

OSAL_Status  OSAL_taskDelay(
    uint32 msTimeout);

OSAL_Status OSAL_taskSendSignal(
    OSAL_TaskId tId, int signo);

OSAL_Status OSAL_taskRegisterSignal(
    int signo, void (*handler)(int));
#ifdef __cplusplus
}
#endif

#endif
