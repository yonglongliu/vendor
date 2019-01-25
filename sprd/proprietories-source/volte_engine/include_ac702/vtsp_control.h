/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 10665 $ $Date: 2009-10-30 02:14:50 +0800 (Fri, 30 Oct 2009) $
 *
 */


#ifndef _VTSP_CONTROL_H_
#define _VTSP_CONTROL_H_

#include "osal.h"

#ifdef __cplusplus
extern "C" {
#endif

VTSP_Return VTSP_toneLocal(
    uvint  infc,
    uvint  templateTone,
    uvint  repeat,
    uint32 maxTime);

VTSP_Return VTSP_toneLocalSequence(
    uvint   infc,
    uvint  *toneId_ptr,
    uvint   numToneIds,
    uint32  control,
    uint32  repeat);

VTSP_Return VTSP_toneQuadLocal(
    uvint           infc,
    uvint           templateQuad,
    uvint           repeat,
    uint32          maxTime);

VTSP_Return VTSP_toneQuadLocalSequence(
    uvint   infc,
    uvint  *toneId_ptr,
    uvint   numToneIds,
    uint32  control,
    uint32  repeat);

VTSP_Return VTSP_toneLocalStop(
    uvint infc);

VTSP_Return VTSP_toneQuadLocalStop(
    uvint infc);

VTSP_Return VTSP_detect(
    uvint  infc,
    uint16 detectMask);

VTSP_Return VTSP_ring(
    uvint         infc,
    uvint         templateRing,
    uvint         numRings,
    uvint         maxTime,
    VTSP_CIDData *cid_ptr);

VTSP_Return VTSP_ringStop(
    uvint infc);

VTSP_Return VTSP_infcControlGain(
    uvint infc,
    vint  gainTx,
    vint  gainRx);

VTSP_Return VTSP_infcControlIO(
    uvint               infc,
    VTSP_ControlInfcIO  control,
    int32               value);

VTSP_Return VTSP_cidDataPackInit(
    VTSP_CIDData *cid_ptr);

VTSP_Return VTSP_cidDataPack(
    VTSP_CIDDataFields  field,
    char               *string_ptr,
    VTSP_CIDData       *cid_ptr);

VTSP_Return VTSP_cidOffhook(
    uvint         infc,
    VTSP_CIDData *cid_ptr);

VTSP_Return VTSP_cidGet(
    uvint         infc,
    VTSP_CIDData *cid_ptr);

VTSP_Return VTSP_infcControlLine(
    uvint                infc,
    VTSP_ControlInfcLine control);

VTSP_Return VTSP_cidDataUnpack(
    VTSP_CIDDataFields   field,
    char                *string_ptr,
    VTSP_CIDData        *cid_ptr);

VTSP_Return VTSP_cidDataUnpackDtmf(
    VTSP_CIDDataFields   field,
    char                *string_ptr,
    VTSP_CIDData        *cid_ptr);

VTSP_Return VTSP_cidTx(
    uvint             infc);

#ifdef __cplusplus
}
#endif

#endif
