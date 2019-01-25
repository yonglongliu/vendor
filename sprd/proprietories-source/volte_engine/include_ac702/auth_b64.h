/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: $ $Date: $
 *
 * Name:        Plain Text base 64 encoder/decoder 
 *
 * File:        auth_b64.h
 *
 * Description: Encoder and decoder functions for converting strings to base64
 *
 * Author:      
 */
 

#ifndef __AUTH_B64_H__
#define __AUTH_B64_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Macro definitions: */

#define b64is7bit(c)  ((c) > 0x7f ? 0 : 1)  /* Valid 7-Bit ASCII character? */
#define b64blocks(l) (((l) + 2) / 3 * 4 + 1)/* Length rounded to 4 byte block. */
#define b64octets(l)  ((l) / 4  * 3 + 1)    /* Length rounded to 3 byte octet. */

int b64encode(char *s, char *t, int size); /* Encodes a string to Base64. */
int b64decode(char *s, int size, char *t); /* Decodes a string to ASCII. */

#ifdef __cplusplus
}
#endif

#endif
