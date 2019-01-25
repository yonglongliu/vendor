/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 7023 $ $Date: 2008-07-17 11:31:01 -0400 (Thu, 17 Jul 2008) $
 *
 */

#ifndef _OSAL_LOG_H_
#define _OSAL_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "osal_types.h"
#include "osal_constant.h"
#include "osal_platform.h"

#define OSAL_MAX_DIAG_LOG_LENGTH     256
 /*
  * Diagnostic event queue name and its depth.
  */
#define OSAL_DIAG_EVENT_QUEUE_NAME   "diag-event"
#define OSAL_DIAG_EVENT_QUEUE_DEPTH  16

typedef struct {
    char        message[OSAL_MAX_DIAG_LOG_LENGTH];
    int         msgType;
} OSAL_diagMsg;

OSAL_Status OSAL_logMsg(
    const char *format_ptr,
    ...)
#ifdef __GNUC__
__attribute__ ((format (printf, 1, 2)));
#else
;
#endif

OSAL_Status OSAL_logString(
    char *string_ptr);

#ifdef __cplusplus
}
#endif

#endif
