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

#include "common.h"
#include "utils.h"
#include "devfreq.h"

// Storage the frequency supported by kernel
static int devfreq_ddr_freqs[NUM_DEVFREQ_AVAILABLE_FREQ_MAX] = {0};

static int integer_compare(const void *aa,const void *bb)
{
    int a = *((int *)aa);
    int b = *((int *)bb);

    return (a - b);
}

/**
 * init_available_freqs:
 * @path: the path of available frequency node in /sys
 * @value: where the available freqs store
 * return: 1 if sucessfull, else 0
 */
static int init_available_freqs(const char *path, int *value)
{
    char buf[128] = {0};
    int fd = -1;
    int n = -1;
    char *item = NULL;
    int size = 0;
    int tmp[10] = {0};

    if (CC_UNLIKELY(path == NULL || value == NULL)) return 0;

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        ALOGE("Open %s fail: %s\n", path, strerror(errno));
        return 0;
    }
    n = read(fd, buf, sizeof(buf));
    if (n == -1) {
        ALOGE("Reading %s fail: %s\n", path, strerror(errno));
        close(fd);
        return 0;
    }
    close(fd);

    item = strtok(buf," ");
    while (item != NULL) {
        int d = atoi(item);
        if (d > 0) {
            tmp[size++] = d;
        }
        item = strtok(NULL, " ");
    }
    if (size == 0)
        return 0;

    qsort(tmp, size, sizeof(int), &integer_compare);

    size--;
    for (int i = NUM_DEVFREQ_AVAILABLE_FREQ_MAX-1; i > 0; i--) {
        value[i] = tmp[size];
        if (--size < 0)
            size = 0;
        ALOGD("%s: dfs_freqs[%d] = %d", __func__, i, value[i]);
    }

    return 1;
}

int devfreq_ddr_clear(const char *path, struct file *file)
{
    char buf[128] = {'\0'};
    char value[LEN_VALUE_MAX] = {'\0'};

    if (CC_UNLIKELY(path == NULL || file == NULL))
        return 0;

    snprintf(buf, sizeof(buf), "%s/%s", path, file->name);

    sprd_timer_settime(file->timer_id, 0);
    if ((strlen(file->stat.current.value) != 0) && (access(buf, F_OK) == 0)) {
        snprintf(value, sizeof(value), "%d %s", 0, file->stat.current.value);
        ALOGD("set %s: %s ", buf, value);
        sprd_write(buf, value);
    }
    memset(&(file->stat), 0, sizeof(struct request_stat));
    return 1;
}

int devfreq_ddr_set(int enable, int duration, const char *path, struct file *file)
{
    char buf[128] = {'\0'};
    char value[LEN_VALUE_MAX] = {'\0'};
    struct timespec now;
    struct req_item *req_item = NULL;
    long long time_value;

    if (CC_UNLIKELY(path == NULL || file == NULL))
        return 0;

    ENTER("enable:%d, duration: %d, %s/%s: %s", enable, duration, path, file->name
        , file->value.target_value);

    if (CC_UNLIKELY(devfreq_ddr_freqs[1] == 0)) {
        ALOGD("%s: Get available ddr freqs", __func__);
        if (init_available_freqs(PATH_DEVFREQ_DDR_FREQ_TABLE, devfreq_ddr_freqs) == 0)
            return 0;
    }

    if (strncmp(file->value.target_value, "max", 3) == 0) {
        snprintf(file->value.target_value, LEN_VALUE_MAX, "%d"
            , devfreq_ddr_freqs[NUM_DEVFREQ_AVAILABLE_FREQ_MAX - 1]);
    }

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
        devfreq_ddr_clear(path, file);
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

    if (access(buf, F_OK) == 0) {
        if (strlen(file->stat.current.value) != 0) {
            snprintf(value, sizeof(value), "%d %s", 0, file->stat.current.value);
            ALOGD("set %s: %s", buf, value);
            sprd_write(buf, value);
        }

        memcpy(&(file->stat.current), req_item, sizeof(struct req_item));
        snprintf(value, sizeof(value), "%d %s", 1, file->stat.current.value);
        ALOGD("set %s: %s ", buf, value);
        sprd_write(buf, value);
    } else {
        // Update current request
        memcpy(&(file->stat.current), req_item, sizeof(struct req_item));
    }

    return 1;
}
