/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: $ $Date: $
 *
 * Name:        MD5 calulator  
 *
 * File:        auth_digcalc.h
 *
 * Description: helper functions to aid in calulating values when performing Md5 hash
 *
 * Author:      
 */
#ifndef __AUTH_DIGCALC_H__
#define __AUTH_DIGCALC_H__

#ifdef __cplusplus
extern "C" {
#endif

#define HASHLEN 16
typedef char HASH[HASHLEN];
#define HASHHEXLEN 32
typedef char HASHHEX[HASHHEXLEN+1];

/* calculate H(A1) as per HTTP Digest spec */
void DigestCalcHA1(
    char * pszAlg,
    char * pszUserName,
    char * pszRealm,
    char * pszPassword,
    int    pwdLen,
    char * pszNonce,
    char * pszCNonce,
    HASHHEX SessionKey
);

/* calculate request-digest/response-digest as per HTTP Digest spec */
void DigestCalcResponse(
    HASHHEX HA1,           /* H(A1) */
    char * pszNonce,       /* nonce from server */
    char * pszNonceCount,  /* 8 hex digits */
    char * pszCNonce,      /* client nonce */
    char * pszQop,         /* qop-value: "", "auth", "auth-int" */
    char * pszMethod,      /* method from the request */
    char * pszDigestUri,   /* requested URL */
    HASHHEX HEntity,       /* H(entity body) if qop="auth-int" */
    HASHHEX Response      /* request-digest or response-digest */
);

#ifdef __cplusplus
}
#endif

#endif

