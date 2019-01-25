/*
 * alog_port.h
 *
 *  Created on: 2017Äê10ÔÂ13ÈÕ
 *      Author: edwin.yang
 */

#ifndef ALOG_PORT_H_
#define ALOG_PORT_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WIN32
#ifndef LOG_TAG
#define LOG_TAG NULL
#endif

#ifndef CONDITION
#define CONDITION(cond) (__builtin_expect((cond)!=0, 0))
#endif

// ---------------------------------------------------------------------
#ifndef android_printLog
#define android_printLog(prio, tag, fmt...) \
        __android_log_print(prio, tag, fmt)
#endif

/*
 * Log macro that allows you to specify a number for the priority.
 */
#ifndef LOG_PRI
#define LOG_PRI(priority, tag, ...) \
        android_printLog(priority, tag, __VA_ARGS__)
#endif

// ---------------------------------------------------------------------
#ifndef ALOG
#define ALOG(priority, tag, ...) \
        LOG_PRI(ANDROID_##priority, tag, __VA_ARGS__)
#endif

#ifndef ALOGE
#define ALOGE(...) ((void)ALOG(LOG_ERROR, LOG_TAG, __VA_ARGS__))
#endif

/*
 * Simplified macro to send a verbose log message using the current LOG_TAG.
 */
#ifndef ALOGV_IF
#define ALOGV_IF(cond, ...) \
        ( (CONDITION(cond)) \
                ? ((void)ALOG(LOG_VERBOSE, LOG_TAG, __VA_ARGS__)) \
                        : (void)0 )
#endif

/*
 * Simplified macro to send a debug log message using the current LOG_TAG.
 */
#ifndef ALOGD_IF
#define ALOGD_IF(cond, ...) \
        ( (CONDITION(cond)) \
                ? ((void)ALOG(LOG_DEBUG, LOG_TAG, __VA_ARGS__)) \
                        : (void)0 )
#endif

/*
 * Simplified macro to send an info log message using the current LOG_TAG.
 */
#ifndef ALOGI_IF
#define ALOGI_IF(cond, ...) \
        ( (CONDITION(cond)) \
                ? ((void)ALOG(LOG_INFO, LOG_TAG, __VA_ARGS__)) \
                        : (void)0 )
#endif

/*
 * Simplified macro to send a warning log message using the current LOG_TAG.
 */
#ifndef ALOGW_IF
#define ALOGW_IF(cond, ...) \
        ( (CONDITION(cond)) \
                ? ((void)ALOG(LOG_WARN, LOG_TAG, __VA_ARGS__)) \
                        : (void)0 )
#endif

/*
 * Simplified macro to send an error log message using the current LOG_TAG.
 */
#ifndef ALOGE_IF
#define ALOGE_IF(cond, ...) \
        ( (CONDITION(cond)) \
                ? ((void)ALOG(LOG_ERROR, LOG_TAG, __VA_ARGS__)) \
                        : (void)0 )
#endif

/*android*/
enum {
    ALGO_LOG_LEVEL_OVER_LOGE = 1,
    ALGO_LOG_LEVEL_OVER_LOGW = 2,
    ALGO_LOG_LEVEL_OVER_LOGI = 3,
    ALGO_LOG_LEVEL_OVER_LOGD = 4,
    ALGO_LOG_LEVEL_OVER_LOGV = 5
};

extern uint32_t g_algo_log_level;
#define ALGO_LOG(fmt, args...) ALOGE("%d, %s: " fmt, __LINE__, __FUNCTION__, ##args)
#define ALGO_LOGE(fmt, args...)  ALOGE("%d, %s: " fmt, __LINE__, __FUNCTION__, ##args)
#define ALGO_LOGW(fmt, args...) ALOGW_IF(g_algo_log_level >= ALGO_LOG_LEVEL_OVER_LOGW, "%d, %s: " fmt, __LINE__, __FUNCTION__, ##args)
#define ALGO_LOGI(fmt, args...) ALOGI_IF(g_algo_log_level >= ALGO_LOG_LEVEL_OVER_LOGI, "%d, %s: " fmt, __LINE__, __FUNCTION__, ##args)
#define ALGO_LOGD(fmt, args...) ALOGD_IF(g_algo_log_level >= ALGO_LOG_LEVEL_OVER_LOGD, "%d, %s: " fmt, __LINE__, __FUNCTION__, ##args)
#define ALGO_LOGV(fmt, args...) ALOGV_IF(g_algo_log_level >= ALGO_LOG_LEVEL_OVER_LOGV, "%d, %s: " fmt, __LINE__, __FUNCTION__, ##args)
#else
#define ALGO_LOGW printf
#define ALGO_LOG printf
#define ALGO_LOGI printf
#define ALGO_LOGE printf
#define ALGO_LOGV printf
#define ALGO_LOGD printf
#define ALOGE printf
#define ALGO_LOGX printf
#endif
#ifdef __cplusplus
}
#endif
#endif /* ALOG_PORT_H_ */
