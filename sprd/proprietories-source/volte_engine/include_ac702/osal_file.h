/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 23244 $ $Date: 2013-12-02 16:27:24 +0800 (Mon, 02 Dec 2013) $
 *
 */

#ifndef _OSAL_FILE_H_
#define _OSAL_FILE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "osal_types.h"
#include "osal_constant.h"
#include "osal_platform.h"

typedef enum {
    OSAL_FILE_SEEK_SET = 1, /* The offset is set to offset bytes. */
    OSAL_FILE_SEEK_CUR = 2, /* The offset is set to its current location plus
                             * offset bytes. */
    OSAL_FILE_SEEK_END = 4, /* The offset is set to the size of the file plus
                             *  offset bytes. */
} OSAL_FileSeekType;

typedef enum {
    OSAL_FILE_O_CREATE = 1,  /* If the file does not exist it will be created.*/
    OSAL_FILE_O_APPEND = 2,  /* The file is opened in append mode. Before each
                              * write the file offset is positioned at the end
                              * of the file */
    OSAL_FILE_O_RDONLY = 4,  /* Open for reading only. */
    OSAL_FILE_O_WRONLY = 8,  /* Open for writing only. */
    OSAL_FILE_O_RDWR   = 16, /* Open for reading and writing. The result is
                              * undefined if this flag is applied to a FIFO. */
    OSAL_FILE_O_TRUNC  = 32, /*If the file exists and is a regular file, and 
                               the file is successfully opened O_RDWR or 
                               O_WRONLY, its length is truncated to 0 and the 
                               mode and owner are unchanged. */
    OSAL_FILE_O_NDELAY = 64,
} OSAL_FileFlags;

OSAL_Status OSAL_fileOpen(
    OSAL_FileId   *fileId_ptr,
    const char    *pathname_ptr,
    OSAL_FileFlags flags,
    uint32         mode);

OSAL_Status OSAL_fileClose(OSAL_FileId *fileId_ptr);

OSAL_Status OSAL_fileRead(
    OSAL_FileId   *fileId_ptr,
    void          *buf_ptr,
    vint          *size_ptr);

OSAL_Status OSAL_fileWrite(
    OSAL_FileId *fileId_ptr,
    void        *buf_ptr,
    vint        *size_ptr);

OSAL_Status OSAL_fileSeek(
    OSAL_FileId      *fileId_ptr,
    vint             *offset_ptr,
    OSAL_FileSeekType type);

OSAL_Status OSAL_fileGetSize(OSAL_FileId *fileId_ptr, vint *size_ptr);

OSAL_Boolean OSAL_fileExists(const char *pathname_ptr);

OSAL_Status OSAL_fileDelete(
    const char    *pathname_ptr);

OSAL_Status OSAL_fileFifoCreate(
    const char    *pathname_ptr,
    uint32         numMsgs,
    uint32         numBytes);

OSAL_Status OSAL_fileFifoDelete(
    const char    *pathname_ptr);

#ifdef __cplusplus
}
#endif

#endif
