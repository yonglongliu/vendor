/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL AND
 * PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12772 $ $Date: 2010-08-20 05:05:49 +0800 (Fri, 20 Aug 2010) $
 *
 */        
#ifndef _VTSP_PACKET_H_
#define _VTSP_PACKET_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Prototypes
 */

VTSP_Return VTSP_stunRecv(
    VTSP_Stun *stunMsg_ptr,
    uint32     timeout,
    vint       video);

VTSP_Return VTSP_stunSend(
    VTSP_Stun *stunMsg_ptr,
    uint32     timeout,
    vint       video);

#ifdef __cplusplus
}
#endif

#endif
