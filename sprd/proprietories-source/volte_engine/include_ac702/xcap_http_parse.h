/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 7102 $ $Date: 2008-07-23 16:03:18 -0500 (Wed, 23 Jul 2008) $
 *
 */

/** \file
 * \brief HTTP reply parsing helper functions.
 *  
 * API in this file is used to parse HTTP data received from a HTTP server.
 * This API is used on HTML data received in XCAP_Evt.
 */

#ifndef _XCAP_HTTP_PARSE_H_
#define _XCAP_HTTP_PARSE_H_

#ifdef __cplusplus
extern "C" {
#endif

/** \struct XCAP_HttpParseHdrObj
 * \brief XCAP parsed header placement structure.
 * 
 * A call to function #XCAP_httpParseHeader will fill this object with parsed
 * HTTP header.
 */
typedef struct {
    int  contentLength;         /**< Content-Length field of HTTP header */
    int  finalCode;             /**< Final HTTP code */
    char etag[32 + 1];          /**< ETag field in HTTP header */
    char contentType[128 + 1];  /**< Content-Type value of HTTP header field */
} XCAP_HttpParseHdrObj;

/*
 * Function prototypes.
 */

/** \fn int XCAP_httpParseHeader(
 *          char                 *buf_ptr,
 *          int                   bufSz,
 *          XCAP_HttpParseHdrObj *obj_ptr)
 * \brief HTTP header parsing function.
 *
 * This function parses HTTP header received from server. Header is received in
 * XCAP_Evt.hdr_ptr in a valid XCAP event received by calling #XCAP_getEvt.
 * @param buf_ptr Passed as XCAP_Evt.hdr_ptr
 * @param bufSz Size of the buffer pointer by buf_ptr. Since buf_ptr is NULL
 * terminated, user can determine length of string by NULL char detection.
 * @param obj_ptr Pointer to a struct where parsed buf_ptr data will be placed.
 * @return 0: failed, 1: passed
 */
int XCAP_httpParseHeader(
    char                 *buf_ptr,
    int                   bufSz,
    XCAP_HttpParseHdrObj *obj_ptr);

#ifdef __cplusplus
}
#endif

#endif
