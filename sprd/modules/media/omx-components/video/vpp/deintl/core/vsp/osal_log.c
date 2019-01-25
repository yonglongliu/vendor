/******************************************************************************
 ** File Name:    osal_log.c                                             *
 ** Author:       Xiaowei.Luo, Leon.Li                                                 *
 ** DATE:         10/15/2014                                                  *
 ** Copyright:    2014 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 10/15/2014    Xiaowei.Luo     Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include <utils/Log.h>
#include "osal_log.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

#define DLOG_TAG "VSP_DEINT"

static int component_debug_level = -1;

#define SLOG_VA(priority, tag, fmt, args) \
	__android_log_vprint(ANDROID_##priority, tag, fmt, args)

void OSAL_Log(_LOG_LEVEL logLevel, const char *msg, ...)
{
    va_list argptr;
//SLOG_VA (LOG_DEBUG, DLOG_TAG, "component_debug_level %d",component_debug_level );
    //  if (component_debug_level < 0)
    {
        component_debug_level = getenv("SPRD_DEBUG") ? atoi(getenv("SPRD_DEBUG")) : OMX_DEBUG_LEVEL;
        //	SLOG_VA (LOG_DEBUG, DLOG_TAG, "component_debug_level %d",component_debug_level );
    }

    {
     ///   if ((int)logLevel > component_debug_level) return;
    }

    va_start(argptr, msg);

    switch (logLevel) {
    case _LOG_TRACE:
        SLOG_VA (LOG_DEBUG, DLOG_TAG, msg, argptr);
        break;
    case _LOG_DEBUG:
        SLOG_VA (LOG_DEBUG, DLOG_TAG, msg, argptr);
        break;

    case _LOG_INFO:
        SLOG_VA (LOG_INFO, DLOG_TAG, msg, argptr);
        break;
    case _LOG_WARNING:
        SLOG_VA (LOG_WARN, DLOG_TAG, msg, argptr);
        break;
    case _LOG_ERROR:
        SLOG_VA (LOG_ERROR, DLOG_TAG, msg, argptr);
        break;
    default:
        SLOG_VA (LOG_DEBUG, DLOG_TAG, msg, argptr);
        break;
    }

    va_end(argptr);
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End

