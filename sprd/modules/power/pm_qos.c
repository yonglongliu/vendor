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

#include "utils.h"
#include "pm_qos.h"

static int pm_qos_cpuidle_fd = -1;

int pm_qos_cpu_clear(const char *path, struct file *file)
{
    if (CC_UNLIKELY(path == NULL || file == NULL))
        return 0;

    if (pm_qos_cpuidle_fd > 0) {
        close(pm_qos_cpuidle_fd);
        pm_qos_cpuidle_fd = -1;
    }
    sprd_timer_settime(file->timer_id, 0);
    memset(&(file->stat), 0, sizeof(struct request_stat));

    return 1;
}

int pm_qos_cpu_set(int enable, int duration,const char *path, struct file *file)
{
    char buf[128] = {'\0'};
    struct timespec now;
    struct req_item *req_item = NULL;
    long long time_value;

    if (CC_UNLIKELY(path == NULL || file == NULL))
        return 0;

    ENTER("enable:%d, duration: %d, %s/%s: %s", enable, duration, path, file->name
        , file->value.target_value);

    memset(&now, 0, sizeof(now));
    snprintf(buf, sizeof(buf), "%s/%s", path, file->name);

    sort_request_for_file(enable, duration, file);
    if (DEBUG_V) {
        char time[20] = {'\0'};
        ALOGD(">>>>>>>>>>>>>>>>>>>>");
        ALOGD("%s:", buf);
        for (int i = 0; i < file->stat.count; i++) {
            sprd_strftime(time, sizeof(time), file->stat.items[i].duration_end_time);
            ALOGD("  value:%s, times:%d, end_time: %s", file->stat.items[i].value
                , file->stat.items[i].times, time);
        }
        ALOGD("<<<<<<<<<<<<<<<<<<<<");
    }

    if (file->stat.count <= 0) {
        ALOGD("ALL latency requests has been handled");
        pm_qos_cpu_clear(path, file);
        return 1;
    }

    // Set timer if the highest priority request has duration time
    clock_gettime(CLOCK_MONOTONIC, &now);
    req_item = &(file->stat.items[file->stat.count - 1]);
    time_value = calc_timespan_ms(now, req_item->duration_end_time);
    if (time_value > 0) {
        sprd_timer_settime(file->timer_id, time_value);
    }

    // If the highest priority request is the same with
    // current, don't need send the request to driver
    if(strlen(file->stat.current.value) != 0
        && file->comp((void *)req_item, (void *)(&(file->stat.current))) == 0)
        return 1;

    // Update current request
    memcpy(&(file->stat.current), req_item, sizeof(struct req_item));

    if (pm_qos_cpuidle_fd < 0) {
        pm_qos_cpuidle_fd = open(buf, O_RDWR);
        if (pm_qos_cpuidle_fd < 0) {
            ALOGE("open(%s) failed:%s", buf, strerror(errno));
            return 0;
        }
    }

    if (access(buf, F_OK) == 0) {
        int value = atoi(req_item->value);
        ALOGD("Set %s: %s", buf, req_item->value);
        write(pm_qos_cpuidle_fd, &value, sizeof(value));
    }

    return 1;
}
