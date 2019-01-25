/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 10367 $ $Date: 2009-09-22 03:57:18 +0800 (Tue, 22 Sep 2009) $
 *
 */


#ifndef _VTSP_STREAM_H_
#define _VTSP_STREAM_H_

#include "osal.h"

#ifdef __cplusplus
extern "C" {
#endif

VTSP_Return VTSP_streamStart(
    uvint            infc,
    VTSP_Stream     *stream_ptr);
VTSP_Return VTSP_streamEnd(
    uvint           infc,
    uvint           streamId);
VTSP_Return VTSP_streamModify(
    uvint            infc,
    VTSP_Stream     *stream_ptr);
VTSP_Return VTSP_streamModifyDir(
    uvint           infc,
    uvint           streamId,
    VTSP_StreamDir  dir);
VTSP_Return VTSP_streamModifyEncoder(
    uvint            infc,
    uvint            streamId,
    uvint            coder);

VTSP_Return VTSP_streamSendRfc4733(
    uvint             infc,
    uvint             streamId,
    VTSP_Rfc4733Type  type,
    void             *rfc4733Data_ptr);

VTSP_Return VTSP_streamModifyConf(
    uvint           infc,
    uvint           streamId,
    uint32          confMask);
VTSP_Return VTSP_streamQuery(
    uvint           infc,
    uvint           streamId);
VTSP_Return VTSP_streamTone(
    uvint           infc,
    uvint           streamId,
    uvint           templateTone,
    uvint           repeat,
    uint32          maxTime);
VTSP_Return VTSP_streamToneQuad(
    uvint           infc,
    uvint           streamId,
    uvint           templateQuad,
    uvint           repeat,
    uint32          maxTime);
VTSP_Return VTSP_streamToneSequence(
    uvint           infc,
    uvint           streamId,
    uvint          *toneId_ptr,
    uvint           numToneIds,
    uint32          control,
    uint32          repeat);
VTSP_Return VTSP_streamToneQuadSequence(
    uvint           infc,
    uvint           streamId,
    uvint          *toneId_ptr,
    uvint           numToneIds,
    uint32          control,
    uint32          repeat);
VTSP_Return VTSP_streamToneStop(
    uvint           infc,
    uvint           streamId);
VTSP_Return VTSP_streamToneQuadStop(
    uvint           infc,
    uvint           streamId);

#ifdef __cplusplus
}
#endif

#endif

