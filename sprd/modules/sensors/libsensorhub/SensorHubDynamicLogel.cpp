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

#include "SensorHubDynamicLogel.h"

/* verbose log messages */
uint32_t DBG_VERBOSE = 0;
/* log entry in all one-time functions */
uint32_t FUNC_ENTRY = 0;
/* log the data input from the events */
uint32_t INPUT_DATA = 0;
/* log sysfs interactions as cat/echo for repro purpose on a shell */
uint32_t SYSFS_VERBOSE = 0;
/* log the data fetched from the handlers */
uint32_t DUMP_DATA = 0;
/* process log messages */
uint32_t PROCESS_VERBOSE = 0;
/* log entry in all handler functions */
uint32_t HANDLER_ENTRY = 0;
/* log some a lot more info about the internals */
uint32_t ENG_VERBOSE = 0;

SensorHubDynamicLogel::SensorHubDynamicLogel(uint32_t flags) {
    int idx = 0;

    ctlFlag[0] = &DBG_VERBOSE;
    ctlFlag[1] = &SYSFS_VERBOSE;
    ctlFlag[2] = &FUNC_ENTRY;
    ctlFlag[3] = &INPUT_DATA;
    ctlFlag[4] = &DUMP_DATA;
    ctlFlag[5] = &PROCESS_VERBOSE;
    ctlFlag[6] = &HANDLER_ENTRY;
    ctlFlag[7] = &ENG_VERBOSE;

    for (idx = 0; idx < MAX_FLAG_CTL; idx ++) {
        *ctlFlag[idx] = flags;
    }
}

SensorHubDynamicLogel::~SensorHubDynamicLogel() {
}

ssize_t SensorHubDynamicLogel::dynamicLogerCtl(pmSenMsgLogCtl_t event) {
    int idx = 0;

    for (idx = 0; idx < MAX_FLAG_CTL; idx++) {
        SH_LOG("udata[%d] = %d", idx, event->udata[idx]);
        *ctlFlag[idx] = event->udata[idx];
    }

    SH_LOG("DBG_VERBOSE[%d], FUNC_ENTRY[%d], INPUT_DATA[%d], SYSFS_VERBOSE[%d] DUMP_DATA[%d], PROCESS_VERBOSE[%d], HANDLER_ENTRY[%d], ENG_VERBOSE[%d]",
            DBG_VERBOSE, FUNC_ENTRY, INPUT_DATA, SYSFS_VERBOSE, DUMP_DATA, PROCESS_VERBOSE, HANDLER_ENTRY, ENG_VERBOSE);

    return 0;
}

ssize_t SensorHubDynamicLogel::dynamicLogerDumpData(int flag, const char * format, ...) {
    va_list argList;
    char buff[MAX_LOG_STRING_SIZE];
    ssize_t count = 0;
    int dumpOrNot = 0;

    dumpOrNot = (DUMP_DATA == 1) ? flag : DUMP_DATA;

    if (dumpOrNot) {
        va_start(argList, format);
        count = vsnprintf(buff, MAX_LOG_STRING_SIZE, format, argList);
        va_end(argList);

        SH_LOG("%s\n", buff);
    }

    return count;
}
