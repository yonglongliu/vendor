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

#ifndef INCLUDE_SENSORS_SENSORLOCALLOGDEF_H_
#define INCLUDE_SENSORS_SENSORLOCALLOGDEF_H_

/* Log enablers, each of these independent */
#if 0
#define PROCESS_VERBOSE (0) /* process log messages */
#define EXTRA_VERBOSE   (0) /* verbose log messages */
#define SYSFS_VERBOSE   (1) /* log sysfs interactions as cat/echo for repro purpose on a shell */
#define FUNC_ENTRY      (0) /* log entry in all one-time functions */

/* Note that enabling this logs may affect performance */
#define HANDLER_ENTRY   (0) /* log entry in all handler functions */
#define ENG_VERBOSE     (0) /* log some a lot more info about the internals */
#define INPUT_DATA      (0) /* log the data input from the events */
#define HANDLER_DATA    (0) /* log the data fetched from the handlers */
#endif
extern uint32_t DBG_VERBOSE;
extern uint32_t FUNC_ENTRY;
extern uint32_t INPUT_DATA;
extern uint32_t SYSFS_VERBOSE;
extern uint32_t DUMP_DATA;
extern uint32_t PROCESS_VERBOSE;
extern uint32_t HANDLER_ENTRY;
extern uint32_t ENG_VERBOSE;

#define SENSOR_HUB_TAG  "SPRD"
#define SH_LOG(fmt, args...)    ALOGD("%s[%d]%s::" fmt, SENSOR_HUB_TAG, __LINE__, __FUNCTION__, ##args);
#define SH_ERR(fmt, args...)    ALOGE("%s[%d]%s:[Err]:" fmt, SENSOR_HUB_TAG, __LINE__, __FUNCTION__, ##args);
#define SH_DBG(fmt, args...)    ALOGD_IF(DBG_VERBOSE, "%s[%d]%s:" fmt, SENSOR_HUB_TAG, __LINE__, __FUNCTION__, ##args);
#define SH_FUNC(fmt, args...)   ALOGD_IF(FUNC_ENTRY, "%s[%d]%s:" fmt, SENSOR_HUB_TAG, __LINE__, __FUNCTION__, ##args);
#define SH_DATA(fmt, args...)   ALOGD_IF(INPUT_DATA, "%s[%d]%s:" fmt, SENSOR_HUB_TAG, __LINE__, __FUNCTION__, ##args);
#define SH_SYSFS(fmt, args...)  ALOGD_IF(SYSFS_VERBOSE, "%s[%d]%s:" fmt, SENSOR_HUB_TAG, __LINE__, __FUNCTION__, ##args);
#define SH_DUMP(fmt, args...)   ALOGD_IF(DUMP_DATA, fmt, ##args);
#endif  // INCLUDE_SENSORS_SENSORLOCALLOGDEF_H_
