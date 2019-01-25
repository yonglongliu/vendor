/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 1381 $ $Date: 2006-12-05 14:44:53 -0800 (Tue, 05 Dec 2006) $
 */

#ifndef _OSAL_SELECT_H_
#define _OSAL_SELECT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "osal_types.h"
#include "osal_constant.h"
#include "osal_platform.h"

/*
 * Timeout structure
 */
typedef struct {
    uint32 sec;
    uint32 usec;
} OSAL_SelectTimeval;

/*
 * Function prototypes.
 */

OSAL_Status OSAL_select(
    OSAL_SelectSet     *readSet_ptr,
    OSAL_SelectSet     *writeSet_ptr,
    OSAL_SelectTimeval *timeout_ptr,
    OSAL_Boolean       *isTimeout_ptr);

OSAL_Status OSAL_selectAddId(
    OSAL_SelectId  *id_ptr,
    OSAL_SelectSet *set_ptr);

OSAL_Status OSAL_selectRemoveId(
    OSAL_SelectId  *id_ptr,
    OSAL_SelectSet *set_ptr);

OSAL_Status OSAL_selectSetInit(
    OSAL_SelectSet *set_ptr);

OSAL_Status OSAL_selectIsIdSet(
    OSAL_SelectId   *id_ptr,
    OSAL_SelectSet  *set_ptr,
    OSAL_Boolean    *isSet_ptr);

OSAL_Status OSAL_selectGetTime(
     OSAL_SelectTimeval *timeout_ptr);

#ifdef __cplusplus
}
#endif

#endif
