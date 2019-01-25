/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 7023 $ $Date: 2008-07-17 11:31:01 -0400 (Thu, 17 Jul 2008) $
 *
 */

#ifndef _OSAL_SEM_H_
#define _OSAL_SEM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "osal_types.h"
#include "osal_constant.h"
#include "osal_platform.h"

typedef void *OSAL_SemId;

OSAL_SemId OSAL_semMutexCreate(
    void);

OSAL_SemId OSAL_semCountCreate(
    int32 initCount);

OSAL_SemId OSAL_semBinaryCreate(
    OSAL_SemBState initState);

OSAL_Status OSAL_semDelete(
    OSAL_SemId sId);

OSAL_Status OSAL_semAcquire(
    OSAL_SemId sId,
    uint32     msTimeout);

OSAL_Status OSAL_semGive(
    OSAL_SemId sId);

#ifdef __cplusplus
}
#endif

#endif
