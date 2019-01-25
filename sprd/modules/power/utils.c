/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <unistd.h>
#include <stdlib.h>
#include <linux/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <cutils/properties.h>
#include <cutils/compiler.h>

#define LOG_TAG "PowerHAL"
#include <utils/Log.h>

#include "utils.h"

long long calc_timespan_ms(struct timespec start, struct timespec end)
{
    long long diff_in_ms = 0;
    diff_in_ms += (end.tv_sec - start.tv_sec) * SEC_TO_MS;
    diff_in_ms += (end.tv_nsec - start.tv_nsec) / MS_TO_NS;
    return diff_in_ms;
}

// struct timespec format to %H:%M:%S.ms
void sprd_strftime(char *buf, int size, struct timespec ts)
{
    struct tm t;
    struct timespec now_monotonic;
    struct timespec now_real;

    memset(&now_monotonic, 0, sizeof(now_monotonic));
    memset(&now_real, 0, sizeof(now_real));
    memset(buf, 0, size);

    if (ts.tv_sec == 0 && ts.tv_nsec == 0) {
        snprintf(buf, sizeof(buf), "0");
        return;
    }

    clock_gettime(CLOCK_MONOTONIC, &now_monotonic);
    clock_gettime(CLOCK_REALTIME, &now_real);

    now_real.tv_sec += (ts.tv_sec - now_monotonic.tv_sec);
    now_real.tv_nsec += (ts.tv_nsec - now_monotonic.tv_nsec);

    if (now_real.tv_nsec < 0) {
        now_real.tv_sec -= (abs(now_real.tv_nsec)/1000000000L + 1);
        now_real.tv_nsec = 1000000000L - abs(now_real.tv_nsec)%1000000000L;
    } else if (now_real.tv_nsec >= 100000000L) {
        now_real.tv_sec += now_real.tv_nsec/1000000000L;
        now_real.tv_nsec = now_real.tv_nsec%1000000000L;
    }

    localtime_r(&now_real.tv_sec, &t);
    snprintf(buf, size, "%02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec);
    snprintf(buf + strlen(buf), size - strlen(buf), ".%03ld", now_real.tv_nsec/MS_TO_NS);
}

void sprd_write(const char *path, const char *s)
{
    int len;
    int fd = open(path, O_WRONLY);

    if (fd < 0) {
        ALOGE("Error opening %s: %s\n", path, strerror(errno));
        return;
    }

    len = write(fd, s, strlen(s));
    if (len < 0) {
        ALOGE("Error writing to %s: %s\n", path, strerror(errno));
    }

    close(fd);
}

int sprd_read(const char *path, char *s, int size)
{
    int count;
    int ret = 0;
    int fd = open(path, O_RDONLY);

    if (fd < 0) {
        ALOGE("Error opening %s: %s\n", path, strerror(errno));
        return -1;
    }

    if ((count = read(fd, s, size - 1)) < 0) {
        ALOGE("Error writing to %s: %s\n", path, strerror(errno));
        ret = -1;
    } else {
        if (s[count - 1] == '\n')
            s[count - 1] = '\0';
        else
            s[count] = '\0';
    }

    close(fd);

    return ret;
}

int get_string_default_value(char *path, char *buf, int size)
{
    if (path == NULL || buf == NULL) return 0;
    if (sprd_read(path, buf, size) == 0 && strlen(buf) > 0)
        return 1;

    return 0;
}

void sprd_timer_create(int signo, timer_t *timer_id, int tid)
{
    struct sigevent sev;

    sev.sigev_notify = SIGEV_THREAD_ID;
    sev.sigev_signo = signo;
    sev.sigev_value.sival_ptr = timer_id;
    sev.sigev_notify_thread_id = tid;
    if (timer_create(CLOCK_MONOTONIC, &sev, timer_id) == -1) {
        ALOGE("timer_create() fail");
    } else {
        ALOGD("%s: timer_id=0x%lx", __func__, *((long *)timer_id));
    }
}

void sprd_timer_settime(timer_t timer_id, long long value)
{
    struct itimerspec its;

    if (DEBUG_V)
        ALOGD("set timerid=0x%lx, value=%lld", (long)timer_id, value);

    memset(&its, 0, sizeof(struct itimerspec));
    its.it_value.tv_sec = value / SEC_TO_MS;
    its.it_value.tv_nsec = (value % SEC_TO_MS) * MS_TO_NS;

    timer_settime(timer_id, 0, &its, NULL);
}
