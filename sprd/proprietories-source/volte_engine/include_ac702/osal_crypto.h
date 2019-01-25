/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev$ $Date$
 *
 */

 /*
 * to wrap around many popular crypto functions in openssl or isc lib 
 * (DNS BIND) 
 */

#ifndef _OSAL_CRYPTO_H_
#define _OSAL_CRYPTO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "osal_types.h"
#include "osal_constant.h"
#include "osal_platform.h"

#define OSAL_CRYPTO_SHA_LBLOCK              16
#define OSAL_CRYPTO_SHA_CBLOCK              (SHA_LBLOCK*4)
#define OSAL_CRYPTO_SHA256_CBLOCK           (SHA_LBLOCK*4)
#define OSAL_CRYPTO_SHA512_CBLOCK           (SHA_LBLOCK*8)
#define OSAL_CRYPTO_SHA_DIGEST_LENGTH       20
#define OSAL_CRYPTO_SHA224_DIGEST_LENGTH    28
#define OSAL_CRYPTO_SHA256_DIGEST_LENGTH    32
#define OSAL_CRYPTO_SHA384_DIGEST_LENGTH    48
#define OSAL_CRYPTO_SHA512_DIGEST_LENGTH    64

/* message digest algo to be used in HMAC */
typedef enum {
    OSAL_CRYPTO_MD_ALGO_NONE = 0,
    OSAL_CRYPTO_MD_ALGO_SHA256,
    OSAL_CRYPTO_MD_ALGO_MD5,
} OSAL_CryptoMdAlgo;

typedef void *OSAL_CryptoCtxId;

OSAL_CryptoCtxId OSAL_cryptoAllocCtx(void);

OSAL_Status OSAL_cryptoFreeCtx(
        OSAL_CryptoCtxId ctxId);

/* SHA256 algo */
OSAL_Status OSAL_cryptoSha256Init(
        OSAL_CryptoCtxId ctx);

OSAL_Status OSAL_cryptoSha256Update(
        OSAL_CryptoCtxId ctx,
        const void      *data,
        size_t           len);

OSAL_Status OSAL_cryptoSha256Final(
        OSAL_CryptoCtxId ctx,
        unsigned char   *md);

unsigned char *OSAL_cryptoSha256(
        const unsigned char *d,
        size_t               n,
        unsigned char       *md);

/* HMAC functions using specified md algo */
OSAL_Status OSAL_cryptoHmacInit(
        OSAL_CryptoCtxId  ctx,
        const void       *key,
        size_t            keyLen,
        OSAL_CryptoMdAlgo mdAlgo);

OSAL_Status OSAL_cryptoHmacUpdate(
        OSAL_CryptoCtxId ctx,
        const void      *data,
        size_t           len);

OSAL_Status OSAL_cryptoHmacFinal(
        OSAL_CryptoCtxId ctx,
        unsigned char   *md,
        size_t          *mdLen_ptr);

OSAL_Status OSAL_cryptoHmacCleanup(
        OSAL_CryptoCtxId ctx);

unsigned char *OSAL_cryptoHmac(
        const void          *key,
        size_t               keyLen,
        OSAL_CryptoMdAlgo    mdAlgo,
        const unsigned char *d,
        size_t               n,
        unsigned char       *md,
        size_t              *mdLen_ptr);

/* base64 encode/decode */
int OSAL_cryptoB64Encode(
        char *s,
        char *t,
        int   size);

int OSAL_cryptoB64Decode(
        char *s,
        int   size,
        char *t);

#ifdef __cplusplus
}
#endif

#endif
