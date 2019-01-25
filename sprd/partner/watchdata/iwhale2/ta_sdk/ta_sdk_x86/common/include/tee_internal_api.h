/*
 * Copyright (c) 2014, STMicroelectronics International N.V.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* Based on GP TEE Internal API Specification Version 0.27 */
#ifndef TEE_INTERNAL_API_H
#define TEE_INTERNAL_API_H

#include <sw_types.h>
#include <tee_api.h>
#include <tee_api_defines.h>
#include <tee_api_types.h>
#include <tee_ta_api.h>
//#include <wdlog.h>
//#define SLogTrace(fmt,arg...)   {\
                printf("[SLogTrace][Line: %d]", __LINE__);\
                printf(fmt,##arg);\
                printf("\n");\
        }
//#define SLogError(fmt,arg...)   {\
                printf("{SLogError][Line: %d]", __LINE__);\
                printf(fmt,##arg);\
                printf("\n");\
        }
#define SLogTrace(...)
#define SLogError(...)
#define SLogWarning(...)
#define S_VAR_NOT_USED(x)

/*
 * @brief Definitions for using public or private key encryption for RSA
 */
#define SW_RSA_USE_PUBLIC 0x20000930
#define SW_RSA_USE_PRIVATE 0x20000931

#define TEE_Panic_t(error)   {err_log("%d", error); TEE_Panic(error); return error;}
#define TEE_Panic_v(error)   {err_log("%d", error); TEE_Panic(error); return;}

#define CHECK_ATTRIBUTE_TYPE(u32x) ((u32x<<2)>>31)

#define IS_REF_ATTRIBUTE(u32x) (CHECK_ATTRIBUTE_TYPE(u32x)==0)
#define IS_VALUE_ATTRIBUTE(u32x) (CHECK_ATTRIBUTE_TYPE(u32x)==1)


/**
 * @brief see Table5-5: Handle Flag Constants
 */
#define IS_HANDLE_INITIALIZED(u32x) ((u32x & TEE_HANDLE_FLAG_INITIALIZED) == TEE_HANDLE_FLAG_INITIALIZED)
#define SET_H_PERSISTENT(u32x) 		(u32x |= TEE_HANDLE_FLAG_PERSISTENT)
#define SET_H_INITIALIZED(u32x) 	(u32x |= TEE_HANDLE_FLAG_INITIALIZED)
#define SET_H_UNINITIALIZED(u32x) 	(u32x &= (~TEE_HANDLE_FLAG_INITIALIZED))
#define SET_H_KEY_SET(u32x) 		(u32x |= TEE_HANDLE_FLAG_KEY_SET)
#define SET_H_TWO_KEYS(u32x) 		(u32x |= TEE_HANDLE_FLAG_EXPECT_TWO_KEYS)

/**
 * @brief
 */
#define IS_ATT_PROTECTED(u32x) 	((u32x&0x10000000) == 0x10000000)
#define IS_P_OBJ(u32x) 			((u32x&0x00010000) == 0x00010000)
#define IS_T_OBJ(u32x) 			((u32x&0x00010000) == 0)

/**
 * @brief see Table5-3: Data Flag Constants
 */
#define IS_AR_READ(u32x) 		((u32x & TEE_DATA_FLAG_ACCESS_READ) == TEE_DATA_FLAG_ACCESS_READ)
#define IS_AR_WRITE(u32x) 		((u32x & TEE_DATA_FLAG_ACCESS_WRITE) == TEE_DATA_FLAG_ACCESS_WRITE)
#define IS_AR_WRITE_META(u32x) 	((u32x & TEE_DATA_FLAG_ACCESS_WRITE_META) == TEE_DATA_FLAG_ACCESS_WRITE_META)
#define IS_AR_SHARE_READ(u32x) 	((u32x & TEE_DATA_FLAG_SHARE_READ) == TEE_DATA_FLAG_SHARE_READ)
#define IS_AR_SHARE_WRITE(u32x) ((u32x & TEE_DATA_FLAG_SHARE_WRITE) == TEE_DATA_FLAG_SHARE_WRITE)
#define IS_AR_CREATE(u32x) 		((u32x & TEE_DATA_FLAG_CREATE) == TEE_DATA_FLAG_CREATE)
#define IS_AR_EXCLUSIVE(u32x) 	((u32x & TEE_DATA_FLAG_EXCLUSIVE) == TEE_DATA_FLAG_EXCLUSIVE)

#define TEE_PROPSET_CURRENT_TA	      (TEE_PropSetHandle)0xFFFFFFFF
#define TEE_PROPSET_CURRENT_CLIENT	  (TEE_PropSetHandle)0xFFFFFFFE
#define TEE_PROPSET_TEE_IMPLEMENTATION	  (TEE_PropSetHandle)0xFFFFFFFD

#define WD_PROPSET_CURRENT_TA	      (0xFFFFFFFF)
#define WD_PROPSET_CURRENT_CLIENT	  (0xFFFFFFFE)
#define WD_PROPSET_TEE_IMPLEMENTATION	  (0xFFFFFFFD)

typedef enum{
	GPD_TA_APPID = 0,
	GPD_TA_SINGLE_INSTANCE,
	GPD_TA_MULTI_SESSION,
	GPD_TA_INSTANCE_KEEP_ALIVE,
	GPD_TA_DATA_SIZE,
	GPD_TA_STACK_SIZE,
	GPD_TA_MAX_ITEMID,
}PROPERTY_ITEMID;

#endif
