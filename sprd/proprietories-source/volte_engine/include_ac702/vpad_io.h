/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision$ $Date$
 */

#ifndef _VPAD_IO_H_
#define _VPAD_IO_H_

#include <vpr_comm.h>
#define VPAD_IO_MESSAGE_SIZE_MAX             (sizeof(VPR_Comm))

#ifndef VPAD_IO_DEBUG
#define VPAD_ioDbgPrintf(fmt, args...)
#else
#define VPAD_ioDbgPrintf(fmt, args...) \
         OSAL_logMsg("[%s:%d] " fmt, __FUNCTION__, __LINE__, ## args)
#endif

OSAL_Status VPAD_ioInit();

void VPAD_ioDestroy();

OSAL_Status VPAD_ioReadDevice(
    void *buf_ptr,
    vint  size);

OSAL_Status VPAD_ioWriteDevice(
    void *buf_ptr,
    vint *size_ptr);

#endif // _VPAD_IO_H_
