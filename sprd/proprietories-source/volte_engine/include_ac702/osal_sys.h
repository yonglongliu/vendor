/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL  
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE  
 * APPLIES: "COPYRIGHT 2004-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"  
 *  
 * $D2Tech$ $Rev: 28791 $ $Date: 2014-09-11 15:28:46 +0800 (Thu, 11 Sep 2014) $
 * 
 */

#ifndef _OSAL_SYS_H_
#define _OSAL_SYS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "osal_types.h"
#include "osal_constant.h"
#include "osal_platform.h"

typedef void *OSAL_ProcessId;

OSAL_ProcessId OSAL_sysExecute(
    char *exeName_ptr,
    char *argList_ptr);

OSAL_Status OSAL_sysTerminate(
    OSAL_ProcessId procId);
    
OSAL_Status OSAL_sysAcquireWakelock(
    void);

OSAL_Status OSAL_sysReleaseWakelock(
    void);

int OSAL_sysGetPid(
    void);

#ifdef __cplusplus
}
#endif

#endif
