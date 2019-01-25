#ifndef LIBCAM_PORT_H
#define LIBCAM_PORT_H

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) & ((TYPE *)0)->MEMBER)
#endif

#include <stdint.h>
#include <signal.h>
#include "mtrace.h"

#define OS_PATH_SEPARATOR '/'

// typedef __int64 uint64_t;
// typedef __int64 int64_t;

#define exit(x)                                                                \
    {                                                                          \
        ALOGE("exit at %s,%s,%d", __FILE__, __FUNCTION__, __LINE__);           \
        return x;                                                              \
    }
#define sprd_isPerformanceTestable() 0
#define sprd_startPerfTracking(format, ...)                                    \
    ALOGV(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)
#define sprd_stopPerfTracking(format, ...)                                     \
    ALOGV(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)
#define sprd_perfInfo(format, ...)                                             \
    ALOGV(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)
#endif
