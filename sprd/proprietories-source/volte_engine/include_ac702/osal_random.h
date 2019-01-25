/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL  
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE  
 * APPLIES: "COPYRIGHT 2004-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"  
 *  
 * $D2Tech$ $Rev: 988 $ $Date: 2006-11-02 15:47:08 -0800 (Thu, 02 Nov 2006) $
 * 
 */

#ifndef _OSAL_RANDOM_H_
#define _OSAL_RANDOM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "osal_types.h"
#include "osal_constant.h"
#include "osal_platform.h"

void OSAL_randomReseed(
    void);

void OSAL_randomGetOctets(
    char *buf_ptr,
    int   size);

void OSAL_randomGetOctetsReEntrant(
    uint32 *seed_ptr,
    char   *buf_ptr,
    int     size);

#ifdef __cplusplus
}
#endif

#endif
