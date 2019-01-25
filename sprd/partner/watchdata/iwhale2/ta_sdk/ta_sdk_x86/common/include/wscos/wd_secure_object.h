/**************************************************************
                Watchdata Platform Development Center
                       Watchdata Confidential
            Copyright (c) Watchdata System CO., Ltd. 2013 -
                        All Rights Reserved
                       http://www.watchdata.com
**************************************************************/
/*
 * Create Date: 2013/2/12
 *
 * Modify Record:
 *     <author>      <date>           <version>     <desc>
 *------------------------------------------------------------
 *	    wd_pf        2013/2/12        1.0		     Create the file.
 */
/*
 * @file    : gp_tee_time.h
 * @ingroup : gp_api
 * @brief   :
 * @author  : wd_pf
 * @version : 1.0
 * @date    : 2013/2/12
 */
#ifndef __WD_SECURE_OBJECT_H__
#define __WD_SECURE_OBJECT_H__

#include "tee_internal_api.h"

/**
 * @brief 
 */
typedef enum{ 
    WD_SO_CONTEXT_DEVICE = 0x00000000,
    WD_SO_CONTEXT_SP,
    WD_SO_CONTEXT_TA,
}WD_SO_CONTEXT_T;

/**
 * @brief 
 */
typedef enum{ 
    WD_SO_LC_PERMANENT = 0x00000000,
    WD_SO_LC_POWERCYCLE,
    WD_SO_LC_SESSION,
}WD_SO_LC_T;
/**
 * @brief 
 *
 * @param timeout
 *
 * @return 
 */
TEE_Result wd_so_wrap_object(const void *src, size_t plain_len, size_t encrypted_len, void *dest, size_t *dest_len, WD_SO_CONTEXT_T context, WD_SO_LC_T life_cycle, TEE_UUID *consumer, unsigned int flags);

/**
 * @brief 
 *
 * @param timeout
 *
 * @return 
 */
TEE_Result wd_so_unwrap_object(void *src, size_t src_len, void *dest, size_t *dest_len, unsigned int flags);
#endif
