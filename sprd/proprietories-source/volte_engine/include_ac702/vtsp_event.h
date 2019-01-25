/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 10367 $ $Date: 2009-09-22 03:57:18 +0800 (Tue, 22 Sep 2009) $
 *
 */


#ifndef _VTSP_EVENT_H_
#define _VTSP_EVENT_H_

#include "osal.h"

#ifdef __cplusplus
extern "C" {
#endif

VTSP_Return VTSP_infcLineStatus(
    uvint           infc);
VTSP_Return VTSP_getEvent(
    vint            infc,
    VTSP_EventMsg  *msg_ptr,
    uint32          timeout);

VTSP_Return VTSP_putEvent(
    vint            infc,
    VTSP_EventMsg  *msg_ptr,
    uint32          timeout);

#ifdef __cplusplus
}
#endif

#endif

