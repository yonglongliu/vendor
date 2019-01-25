/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 1381 $ $Date: 2006-12-05 14:44:53 -0800 (Tue, 05 Dec 2006) $
 */

#ifndef _VPR_H_
#define _VPR_H_

#define VPR_NETWORK_MODE_STRING_WIFI "wifi"
#define VPR_NETWORK_MODE_STRING_LTE  "lte"

#ifndef INCLUDE_VPR

#define VPR_startDaemon() (OSAL_SUCCESS)
#define VPR_stopDaemon()  (OSAL_SUCCESS)

#else

OSAL_Status VPR_startDaemon(void);
OSAL_Status VPR_stopDaemon(void);
OSAL_Status VPR_allocate(void);
OSAL_Status VPR_start(void);
void VPR_destory(void);
#endif
#endif
