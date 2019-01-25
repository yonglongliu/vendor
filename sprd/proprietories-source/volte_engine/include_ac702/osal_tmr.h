/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 7023 $ $Date: 2008-07-17 11:31:01 -0400 (Thu, 17 Jul 2008) $
 *
 */

#ifndef _OSAL_TMR_H_
#define _OSAL_TMR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "osal_types.h"
#include "osal_constant.h"
#include "osal_platform.h"

typedef void *              OSAL_TmrId;
typedef int32               OSAL_TmrReturn;
typedef void *              OSAL_TmrArg;
typedef OSAL_TmrReturn    (*OSAL_TmrPtr)(OSAL_TmrArg);


OSAL_TmrId OSAL_tmrCreate(
    void);

OSAL_Status OSAL_tmrDelete(
    OSAL_TmrId tId);

OSAL_Status OSAL_tmrStart(
    OSAL_TmrId    tId,
    OSAL_TmrPtr   fx_ptr,
    OSAL_TmrArg   arg,
    uint32        msTimeout);

OSAL_Status OSAL_tmrPeriodicStart(
    OSAL_TmrId    tId,
    OSAL_TmrPtr   fx_ptr,
    OSAL_TmrArg   arg,
    uint32        msTimeout);

OSAL_Status OSAL_tmrStop(
    OSAL_TmrId tId);

OSAL_Status OSAL_tmrIsRunning(
    OSAL_TmrId tId);

OSAL_Status OSAL_tmrInterrupt(
    OSAL_TmrId tId);

#ifdef __cplusplus
}
#endif

#endif
