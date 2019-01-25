/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 10367 $ $Date: 2009-09-22 03:57:18 +0800 (Tue, 22 Sep 2009) $
 *
 */


#ifndef _VTSP_FLOW_H_
#define _VTSP_FLOW_H_

#include "osal.h"

#ifdef __cplusplus
extern "C" {
#endif

vint VTSP_flowOpen(
    uvint infc,
    uvint streamId,
    uvint flowDir,
    uvint key,
    uvint recordCoder);

VTSP_Return VTSP_flowPlay(
    vint   flowId,
    uvint  coder,
    uvint  blockSize,
    void  *data_ptr,
    uint32 control,
    uint32 timeout);

VTSP_Return VTSP_flowPlaySil(
    vint   flowId,
    uvint  coder,
    uvint  blockSize,
    void  *data_ptr,
    uint32 duration,
    uint32 control,
    uint32 timeout);

VTSP_Return VTSP_flowClose(
    vint flowId);

VTSP_Return VTSP_flowAbort(
    vint   flowId,
    uint32 timeout);

vint VTSP_flowRecord(
    vint    flowId,
    uvint   blockSize,
    uvint  *coder,
    void   *data_ptr,
    uint32 *duration,
    uint32  timeout);

#ifdef __cplusplus
}
#endif

#endif

