/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 19429 $ $Date: 2012-12-22 03:07:07 +0800 (Sat, 22 Dec 2012) $
 *
 */

#ifndef _OSAL_TIME_H_
#define _OSAL_TIME_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "osal_types.h"
#include "osal_constant.h"
#include "osal_platform.h"

typedef struct {
    uint32 sec;  /* seconds */
    uint32 usec; /* microseconds */
} OSAL_TimeVal;

typedef struct {
    int sec;     /* seconds */
    int min;     /* minutes */
    int hour;    /* hours */
    int mday;    /* day of the month */
    int mon;     /* month */
    int year;    /* year */
    int wday;    /* day of the week */
    int yday;    /* day in the year */
    int isdst;   /* daylight saving time */
} OSAL_TimeLocal;

OSAL_Status OSAL_timeGetTimeOfDay(
    OSAL_TimeVal *timeVal_ptr);

OSAL_Status OSAL_timeGetISO8601(
    uint8 *buff_ptr, 
    vint *size_ptr);

OSAL_Status OSAL_timeLocalTime(
    OSAL_TimeLocal *timeLocal_ptr);

#ifdef __cplusplus
}
#endif

#endif
