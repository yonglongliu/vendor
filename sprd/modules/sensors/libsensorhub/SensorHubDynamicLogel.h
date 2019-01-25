/*
 * Copyright (C) 2008 The Android Open Source Project
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

#ifndef SENSORHUBDYNAMICLOGEL_H_
#define SENSORHUBDYNAMICLOGEL_H_

#include <stdint.h>
#include <sys/types.h>
#include <cutils/log.h>
#include <sensors/SensorLocalLogdef.h>

#define MAX_FLAG_CTL 8
#define MAX_LOG_STRING_SIZE 255

typedef struct {
    uint8_t Cmd;
    uint8_t Length;
    uint8_t debug_data[5];
    uint32_t udata[MAX_FLAG_CTL];
}mSenMsgLogCtl_t, *pmSenMsgLogCtl_t;

class SensorHubDynamicLogel {
    /* Array of pointers */
    uint32_t *ctlFlag[MAX_FLAG_CTL];

 public:
    explicit SensorHubDynamicLogel(uint32_t flags);
    ~SensorHubDynamicLogel();
    ssize_t dynamicLogerCtl(pmSenMsgLogCtl_t event);
    ssize_t dynamicLogerDumpData(int, const char * format, ...);
};

#endif  // SENSORHUBDYNAMICLOGEL_H_
