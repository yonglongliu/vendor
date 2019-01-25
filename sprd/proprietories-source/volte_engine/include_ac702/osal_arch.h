/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL  
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE  
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"  
 *  
 * $D2Tech$ $Rev: 988 $ $Date: 2006-11-02 15:47:08 -0800 (Thu, 02 Nov 2006) $
 * 
 */

#ifndef _OSAL_ARCH_H_
#define _OSAL_ARCH_H_

#include "osal_types.h"
#include "osal_constant.h"
#include "osal_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

OSAL_INLINE uint32 OSAL_archCountGet(
    void);

OSAL_INLINE void OSAL_archCountInit(
    void);

OSAL_INLINE uint32 OSAL_archCountDelta(
        uint32 start,
        uint32 stop);

OSAL_INLINE uint32 OSAL_archCountCyclesToKHz(
    uint32 count);

#ifdef __cplusplus
}
#endif

#endif
