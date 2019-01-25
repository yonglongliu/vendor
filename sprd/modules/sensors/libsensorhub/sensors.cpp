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

#include <hardware/sensors.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <math.h>
#include <poll.h>
#include <pthread.h>
#include <stdlib.h>

#include <linux/input.h>

#include <utils/Atomic.h>
#include <cutils/log.h>

#include "sensors.h"

#include "SensorHubInputSensor.h"
#include "sensors/SensorConfigDrv.h"
/*****************************************************************************/


/*****************************************************************************/
/* The SENSORS Module */
#include "sensors/SensorConfigFeature.h"

static int open_sensors(const struct hw_module_t* module, const char* id,
                        struct hw_device_t** device);


static int sensors__get_sensors_list(struct sensors_module_t* module,
                                     struct sensor_t const** list) {
    SH_LOG("module id[%s] [%d]", module->common.id, shubGetListSize());
    *list = shubGetList();
    return shubGetListSize();
}

static int sensors__set_operation_mode(unsigned int mode) {
        SH_LOG("operation mode is [%d]\n", mode);
        return -EINVAL;
}

static struct hw_module_methods_t sensors_module_methods = {
        .open = open_sensors
};

struct sensors_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .version_major = 1,
        .version_minor = 0,
        .id =  SENSORS_HARDWARE_MODULE_ID,
        .name = "Sensor module",
        .author = "Sensor Platforms Inc.",
        .methods = &sensors_module_methods,
        .dso = 0,
        .reserved = {0}
    },
    .get_sensors_list = sensors__get_sensors_list,
    .set_operation_mode = sensors__set_operation_mode,
};

struct sensors_poll_context_t {
    struct sensors_poll_device_1 device;  // must be first

        sensors_poll_context_t();
        ~sensors_poll_context_t();
    int activate(int handle, int enabled);
    int setDelay(int handle, int64_t delay_ns);
    int pollEvents(sensors_event_t* data, int count);
    int batch(int handle, int flags, int64_t period_ns, int64_t timeout);
    int flush(int handle);

 private:
    static const char WAKE_MESSAGE = 'W';
    int mWritePipeFd;
    int numSensorDrivers;
    size_t numFds;
    size_t wake;  // numFds - 1;
    struct pollfd* mPollFds = NULL;
    SensorBase** mSensors = NULL;

    int handleToDriver(int handle) const {
        int idxdrv = 0;
        int idxlist = 0;
        pshub_cfg_t cfg = shubGetDrvConfig();

        if (cfg) {
            for (idxdrv = 0; idxdrv < numSensorDrivers; idxdrv++) {
                for (idxlist = 0; idxlist < cfg->cfg[idxdrv].listSize; idxlist++) {
                    if (handle == cfg->cfg[idxdrv].sensorList[idxlist].handle) {
                        SH_DBG("found sensor named '%s', handle = %d",
                                cfg->cfg[idxdrv].sensorList[idxlist].name, handle);
                        return (int)idxdrv;
                    }
                }
            }
        }

        SH_ERR("Nothing to be found for handle '%d', return err", handle);

        return -EINVAL;
    }
};

sensors_poll_context_t::sensors_poll_context_t() {
    int result;
    int idx = 0;
    pshub_cfg_t cfg = shubGetDrvConfig();

    if (cfg) {
        SH_LOG("GetDrvConfig cfgSize = %d", cfg->cfgSize);

        numSensorDrivers = cfg->cfgSize;
        numFds  = numSensorDrivers + 1;
        wake = numSensorDrivers;  // numFds - 1;

        mSensors = (SensorBase **)malloc(sizeof(SensorBase *) * numSensorDrivers);
        mPollFds = (struct pollfd*)malloc(sizeof(struct pollfd) * numFds);

        if (mSensors && mPollFds) {
            for (idx = 0; idx < numSensorDrivers; idx++) {
                SH_LOG("GetDrvConfig ctlName = %s, drvName = %s",
                        cfg->cfg[idx].ctlName, cfg->cfg[idx].drvName);

                mSensors[idx] = new SensorHubInputSensor((cfg->cfg[idx]));

                mPollFds[idx].fd = mSensors[idx]->getFd();
                mPollFds[idx].events = POLLIN;
                mPollFds[idx].revents = 0;
            }

            int wakeFds[2];
            result = pipe(wakeFds);
            ALOGE_IF(result < 0, "error creating wake pipe (%s)", strerror(errno));
            fcntl(wakeFds[0], F_SETFL, O_NONBLOCK);  // @@ read should probably be blocking but since
            fcntl(wakeFds[1], F_SETFL, O_NONBLOCK);  // we are using 'poll' it doesn't matter
            mWritePipeFd = wakeFds[1];

            mPollFds[wake].fd = wakeFds[0];
            mPollFds[wake].events = POLLIN;
            mPollFds[wake].revents = 0;
        } else {
            SH_ERR("Malloc 'SensorBase' and 'struct pollfd' failed, return");
        }
    } else {
        SH_ERR("GetDrvConfig failed, return");
    }
}

sensors_poll_context_t::~sensors_poll_context_t() {
    for (int i = 0 ; i < numSensorDrivers ; i++) {
        delete mSensors[i];
    }
    close(mPollFds[wake].fd);
    close(mWritePipeFd);
    free(mSensors);
    free(mPollFds);
}

int sensors_poll_context_t::activate(int handle, int enabled) {
    int index = handleToDriver(handle);
    if (index < 0) return index;

    int err =  mSensors[index]->setEnable(handle, enabled);
    ALOGE_IF(err != 0, "Sensor-activate failed (%d)", err);
    if (enabled && !err) {
        const char wakeMessage(WAKE_MESSAGE);
        int result = write(mWritePipeFd, &wakeMessage, 1);
        ALOGE_IF(result < 0, "error sending wake message (%s)", strerror(errno));
    }
    return err;
}

int sensors_poll_context_t::setDelay(int handle, int64_t delay_ns) {
    int index = handleToDriver(handle);
    if (index < 0) return index;

    int err = mSensors[index]->setDelay(handle, delay_ns);
    ALOGE_IF(err < 0, "set delay failed (%d)", err);

    return err;
}

int sensors_poll_context_t::pollEvents(sensors_event_t* data, int count) {
    int nbEvents = 0;
    int n = 0;
    do {
        // see if we have some leftover from the last poll()
        for (int i = 0 ; count && i < numSensorDrivers ; i++) {
            SensorBase* const sensor(mSensors[i]);
            SH_DATA("mPollFds[%d].revents = %d", i, mPollFds[i].revents);
            if ((mPollFds[i].revents & POLLIN) || (sensor->hasPendingEvents())) {
                int nb = sensor->readEvents(data, count);
                if (nb < count) {
                    // no more data for this sensor
                    mPollFds[i].revents = 0;
                }
        if (nb >=0) {
                    count -= nb;
                    nbEvents += nb;
                    data += nb;
                }
            }
        }

        if (count) {
            // we still have some room, so try to see if we can get
            // some events immediately or just wait if we don't have
            // anything to return

            //  n = poll(mPollFds, numFds, nbEvents ? 0 : -1);
            n = TEMP_FAILURE_RETRY(poll(mPollFds, numFds, nbEvents ? 0 : -1));
            if (n < 0) {
                SH_ERR("poll() failed (%s)", strerror(errno));
                return -errno;
            }
            if (mPollFds[wake].revents & POLLIN) {
                char msg;
                int result = read(mPollFds[wake].fd, &msg, 1);
                ALOGE_IF(result < 0, "error reading from wake pipe (%s)", strerror(errno));
                ALOGE_IF(msg != WAKE_MESSAGE, "unknown message on wake queue (0x%02x)", int(msg));
                mPollFds[wake].revents = 0;
            }
        }
        // if we have events and space, go read them
    } while (n && count);
    return nbEvents;
}

int sensors_poll_context_t::batch(int handle, int flags, int64_t period_ns, int64_t timeout) {
    int index = handleToDriver(handle);
    if (index < 0) return index;

    int err = mSensors[index]->batch(handle, flags, period_ns, timeout);
    ALOGE_IF(err < 0, "batch failed (%d)", err);

    return err;
}

int sensors_poll_context_t::flush(int handle) {
    int index = handleToDriver(handle);
    if (index < 0) return index;

    int err = mSensors[index]->flush(handle);
    ALOGE_IF(err < 0, "flush failed (%d)", err);

    return err;
}


/*****************************************************************************/

static int poll__close(struct hw_device_t *dev) {
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    if (ctx) {
        delete ctx;
    }
    return 0;
}

static int poll__activate(struct sensors_poll_device_t *dev,
        int handle, int enabled) {
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->activate(handle, enabled);
}

static int poll__setDelay(struct sensors_poll_device_t *dev,
        int handle, int64_t ns) {
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->setDelay(handle, ns);
}

static int poll__poll(struct sensors_poll_device_t *dev,
        sensors_event_t* data, int count) {
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->pollEvents(data, count);
}

static int poll__batch(struct sensors_poll_device_1 *dev,
                      int handle, int flags, int64_t period_ns, int64_t timeout) {
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->batch(handle, flags, period_ns, timeout);
}

static int poll__flush(struct sensors_poll_device_1* dev, int handle) {
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->flush(handle);
}

/*****************************************************************************/

/** Open a new instance of a sensor device using name */
static int open_sensors(const struct hw_module_t* module, const char* id,
                        struct hw_device_t** device) {
    int status = -EINVAL;
    SH_LOG("id[%s]", id);
    /*
     * Write Opcode first
     * */
    SensorHubOpcodeExtrator();

    sensors_poll_context_t *dev = new sensors_poll_context_t();

    memset(&dev->device, 0, sizeof(sensors_poll_device_t));

    dev->device.common.tag = HARDWARE_DEVICE_TAG;
    dev->device.common.version  = SENSORS_DEVICE_API_VERSION_1_3;
    dev->device.common.module   = const_cast<hw_module_t*>(module);
    dev->device.common.close    = poll__close;
    dev->device.activate        = poll__activate;
    dev->device.setDelay        = poll__setDelay;
    dev->device.poll            = poll__poll;
    dev->device.batch           = poll__batch;
    dev->device.flush           = poll__flush;

    *device = &dev->device.common;
    status = 0;

    return status;
}
