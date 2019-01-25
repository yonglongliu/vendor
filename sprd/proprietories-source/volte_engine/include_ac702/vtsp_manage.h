/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 10367 $ $Date: 2009-09-22 03:57:18 +0800 (Tue, 22 Sep 2009) $
 *
 */


#ifndef _VTSP_MANAGE_H_
#define _VTSP_MANAGE_H_

#include "osal.h"

#ifdef __cplusplus
extern "C" {
#endif

VTSP_QueryData *VTSP_query(
    void);

VTSP_Return VTSP_init(
    VTSP_Context       **context_ptr, 
    VTSP_ProfileHeader  *profile_ptr,
    VTSP_TaskConfig     *taskConfig_ptr);

VTSP_Return VTSP_config(
    VTSP_TemplateCode    templateCode, 
    uvint                templateId, 
    void                *data_ptr); 

VTSP_Return VTSP_configRing(
    uvint              templateId,
    VTSP_RingTemplate *templateData_ptr);

VTSP_Return VTSP_configTone(
    uvint              templateId,
    VTSP_ToneTemplate *templateData_ptr);

VTSP_Return VTSP_configToneQuad(
    uvint                  templateId,
    VTSP_ToneQuadTemplate *templateData_ptr);

VTSP_Return VTSP_start(void);

VTSP_Return VTSP_shutdown(void);

VTSP_Return VTSP_getVersion(
    VTSP_Version   *data_ptr);

#ifdef __cplusplus
}
#endif

#endif

