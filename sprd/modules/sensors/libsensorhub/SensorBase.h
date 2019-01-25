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

#ifndef SENSORBASE_H_
#define SENSORBASE_H_

#include <stdint.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <limits.h>

#include <hardware/sensors.h>
#include <sensors/SensorConfigFeature.h>

/*****************************************************************************/

struct sensors_event_t;

class SensorBase {
 protected:
        const char* udevName;
        const char* uinputName;
        char        input_name[PATH_MAX];
        int         ctlMode;
        int         drvMode;
        int         devFd;
        int         dataFd;

        static int64_t getTimestamp();
        static int64_t timevalToNano(timeval const& t) {
            return t.tv_sec*1000000000LL + t.tv_usec*1000;
        }

        int find_type_by_name(const char *name, const char *type);
        int sysfs_set_input_attr(char *devpath, const char *attr, char *value, int len);
        int sysfs_set_input_attr_by_int(char *devpath, const char *attr, int value);

        int openInputDataDevice(const char *inputName);
        int openIIODataDevice(const char *inputName);
        int openDataDevice(const char* dataName);
        int closeDataDevice();

        int openIOCtlDevice();
        int openSysfsCtlDevice();
        int openCtlDevice();
        int closeCtlDevice();

 public:
        SensorBase(const shub_drv_cfg_t &drvCfg, int defer);
        virtual ~SensorBase();

        virtual int readEvents(sensors_event_t* data, int count) = 0;
        virtual bool hasPendingEvents() const;
        virtual int getFd() const;
        virtual int setDelay(int32_t handle, int64_t ns);
        virtual int setEnable(int32_t handle, int enabled) = 0;
        virtual int getEnable(int32_t handle) = 0;
        virtual int batch(int handle, int flags, int64_t period_ns, int64_t timeout);
        virtual int flush(int32_t handle);
};

/*****************************************************************************/

#endif  // SENSORBASE_H_
