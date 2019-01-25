/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 5312 $ $Date: 2008-02-18 19:05:41 -0600 (Mon, 18 Feb 2008) $
 */

#ifndef _OSAL_STRING_H_
#define _OSAL_STRING_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "osal_types.h"
#include "osal_constant.h"
#include "osal_platform.h"

char* OSAL_strcpy(
    char        *dst_ptr,
    const char  *src_ptr);

char* OSAL_strncpy(
    char       *dst_ptr,
    const char *src_ptr,
    vint        size);

vint OSAL_strcmp(
    const char *str1_ptr,
    const char *str2_ptr);

vint OSAL_strcasecmp(
    const char *str1_ptr, 
    const char *str2_ptr);

vint OSAL_strncmp(
    const char *str1_ptr,
    const char *str2_ptr,
    vint        size);

vint OSAL_strncasecmp(
    const char *str1_ptr, 
    const char *str2_ptr,
    vint        size);

vint OSAL_strlen(
    const char *str_ptr);

vint OSAL_snprintf(
    char       *buf_ptr,
    vint        size,
    const char *format_ptr,
    ...)
#ifdef __GNUC__
__attribute__ ((format (printf, 3, 4)));
#else
;
#endif

char* OSAL_strchr(
    const char *str_ptr,
    vint        c);

char* OSAL_strtok(
    char *str_ptr,
    const char *sub_ptr);

char* OSAL_strscan(
    const char *str_ptr,
    const char *sub_ptr);

char* OSAL_strnscan(
    const char *str_ptr,
    vint        size,
    const char *sub_ptr);

char* OSAL_strncasescan(
    const char *str_ptr,
    vint        size,
    const char *sub_ptr);

vint OSAL_atoi(
    const char *num_ptr);

vint OSAL_itoa(
    vint    value,
    char   *str_ptr,
    vint    strLen);

vint OSAL_htoi(
    const char *num_ptr);

vint OSAL_itoh(
    vint    value,
    char   *str_ptr,
    vint    strLen);

uint32 OSAL_strtoul(
    const char *str_ptr, 
    char      **end_ptr, 
    vint        base);

void OSAL_strAsciiToUnicode(
    char *unicode_ptr,
    char *ascii_ptr,
    int   szUnicode);

void OSAL_strUnicodeToAscii(
    char *ascii_ptr,
    char *unicode_ptr,
    int   szAscii);

#ifdef __cplusplus
}
#endif

#endif
