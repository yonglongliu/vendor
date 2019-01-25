/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 7023 $ $Date: 2008-07-17 11:31:01 -0400 (Thu, 17 Jul 2008) $
 *
 */

#ifndef _OSAL_COND_H_
#define _OSAL_COND_H_

#include "osal_types.h"
#include "osal_constant.h"
#include "osal_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

OSAL_Status OSAL_condApplicationExitRegister(
    void);

OSAL_Status OSAL_condApplicationExitWaitForCondition(
    void);

OSAL_Status OSAL_condApplicationExitUnregister(
    void);

#ifdef __cplusplus
}
#endif

#endif
