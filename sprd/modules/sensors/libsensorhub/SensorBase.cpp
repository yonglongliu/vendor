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

#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <poll.h>
#include <unistd.h>
#include <dirent.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <sys/select.h>
#include <linux/input.h>
#include <cutils/log.h>
#include <utils/Timers.h>
#include <utils/SystemClock.h>

#include "SensorBase.h"
#include <sensors/SensorLocalLogdef.h>
/*****************************************************************************/
/*
 * define macro begin
 * */
#define INT32_CHAR_LEN      12
#define IIO_MAX_BUFF_SIZE   8192
#define IIO_BUF_SIZE_RETRY  8
/*
 * define macro end
 * */
SensorBase::SensorBase(const shub_drv_cfg_t & drvCfg, int defer = 0)
    : udevName(drvCfg.ctlName),
      uinputName(drvCfg.drvName),
      ctlMode(drvCfg.ctlMode),
      drvMode(drvCfg.drvMode),
      devFd(-1),
      dataFd(-1) {
    if (!defer) {
        if (uinputName) {
            dataFd = openDataDevice(uinputName);
        }
        if (udevName) {
            devFd = openCtlDevice();
        }
    }

    SH_LOG("SensorBase::SensorBase dataFd = %d, devFd = %d", dataFd, devFd);
}

SensorBase::~SensorBase() {
    closeDataDevice();
    closeCtlDevice();
}

int SensorBase::setDelay(int32_t handle, int64_t ns) {
    SH_DBG("handle[%d], ns[%" SCNd64"]", handle, ns);
    return 0;
}

/**
 * as specified in hardware/sensors.h
     * When timeout is not 0:
     *   If successful, 0 is returned.
     *   If the specified sensor doesn't support batch mode, return retu.
     *   If the specified sensor's trigger-mode is one-shot, return -EINVAL.
     *   If WAKE_UPON_FIFO_FULL is specified and the specified sensor's internal
     *   FIFO is too small to store at least 10 seconds worth of data at the
     *   given rate, -EINVAL is returned. Note that as stated above, this has to
     *   be determined at compile time, and not based on the state of the
     *   system.
     *   If some other constraints above cannot be satisfied, return -EINVAL.
     *
     * When timeout is 0:
     *   The caller will never set the wake_upon_fifo_full flag.
     *   The function must succeed, and batch mode must be deactivated.
  */
int SensorBase::batch(int handle, int flags, int64_t period_ns, int64_t timeout) {
    SH_DBG("handle[%d], flags[%d], period_ns[%" SCNd64"]", handle, flags, period_ns);
    if (timeout !=0) {
        return -EINVAL;
    } else {
        return 0;
    }
}

int SensorBase::flush(int32_t handle) {
    SH_DBG("handle[%d]", handle);
    return 0;
}



bool SensorBase::hasPendingEvents() const {
    return false;
}


/*
 * Basic function begin
 * */

/*
 * Basic function: time
 * */
int64_t SensorBase::getTimestamp() {
    struct timespec ts;
    int result;
    int64_t timestamp;

    result = clock_gettime(CLOCK_BOOTTIME, &ts);
    if (result == 0) {
        timestamp = seconds_to_nanoseconds(ts.tv_sec) + ts.tv_nsec;
        return timestamp;
    } else {
        return 0;
    }
}

/*
 * Basic functin: XXX
 * */
int SensorBase::find_type_by_name(const char *name, const char *type) {
    const struct dirent *ent;
    int number, numstrlen;
    FILE *nameFile;
    DIR *dp;
    char thisname[30];
    char *filename;
    int filenamelength;
    static const char *iio_dir = "/sys/bus/iio/devices/";

    dp = opendir(iio_dir);
    if (dp == NULL) {
        SH_ERR("opendir %s failed", iio_dir);
        return -ENODEV;
    }

    while (ent = readdir(dp), ent != NULL) {
        if (strcmp(ent->d_name, ".") != 0 &&
                strcmp(ent->d_name, "..") != 0 &&
                strlen(ent->d_name) > strlen(type) &&
                strncmp(ent->d_name, type, strlen(type)) == 0) {
            numstrlen = sscanf(ent->d_name + strlen(type), "%d", &number);

            /* verify the next character is not a colon */
            if (strncmp(ent->d_name + strlen(type) + numstrlen, ":", 1) != 0) {
                filenamelength = strlen(iio_dir) + strlen(type) + numstrlen + 6;
                filename = (char *)malloc(filenamelength);
                if (filename == NULL) {
                    SH_ERR("malloc failed");
                    return -ENOMEM;
                }
                snprintf(filename, filenamelength, "%s%s%d/name", iio_dir, type, number);
                nameFile = fopen(filename, "r");
                if (!nameFile) {
                    SH_ERR("fopen %s failed!", filename);
                    continue;
                }

                free(filename);
                fscanf(nameFile, "%s", thisname);
                if (strcmp(name, thisname) == 0) {
                    return number;
                }
                fclose(nameFile);
            }
        }
    }
    return -ENODEV;
}

int SensorBase::sysfs_set_input_attr(char *devpath, const char *attr, char *value,
        int len) {
    char fname[PATH_MAX];
    int fd;
    snprintf(fname, sizeof(fname), "%s/%s", devpath, attr);
    fname[sizeof(fname) - 1] = '\0';
    fd = open(fname, O_WRONLY);
    if (fd < 0) {
        SH_ERR("open %s failed!", fname);
        return -errno;
    }

    if (write(fd, value, (size_t)len) < 0) {
        SH_ERR("write %s failed!", fname);
        close(fd);
        return -errno;
    }
    close(fd);
    return 0;
}

int SensorBase::sysfs_set_input_attr_by_int(char *devpath, const char *attr,
        int value) {
    char buf[INT32_CHAR_LEN];
    // int2a(buf, value, sizeof(buf));
    size_t n = snprintf(buf, sizeof(buf), "%d", value);
    if (n > sizeof(buf)) {
        return -1;
    }

    return sysfs_set_input_attr(devpath, attr, buf, sizeof(buf));
}

/*
 * Basic function: data
 * */
int SensorBase::openInputDataDevice(const char *inputName) {
    int fd = -1;
    const char *dirname = "/dev/input";
    char devname[PATH_MAX];
    char *filename;
    DIR *dir;
    struct dirent *de;

    dir = opendir(dirname);
    if (dir == NULL)
        return -1;
    snprintf(devname, sizeof(devname), "%s", dirname);
    // strcpy(devname, dirname); 2-29
    filename = devname + strlen(devname);
    *filename++ = '/';
    while ((de = readdir(dir))) {
        if (de->d_name[0] == '.' &&
           (de->d_name[1] == '\0' ||
           (de->d_name[1] == '.' && de->d_name[2] == '\0')))
            continue;
        snprintf(filename, PATH_MAX - strlen(devname), "%s", de->d_name);
        // strcpy(filename, de->d_name); 2-29
        fd = open(devname, O_RDONLY);
        if (fd >= 0) {
            char name[80];
            if (ioctl(fd, EVIOCGNAME(sizeof(name) - 1), &name) < 1) {
                name[0] = '\0';
            }
            if (!strcmp(name, inputName)) {
                snprintf(input_name, PATH_MAX, "%s", filename);
                // strcpy(input_name, filename); 2-29
                break;
            } else {
                close(fd);
                fd = -1;
            }
        }
    }
    closedir(dir);
    ALOGE_IF(fd < 0, "couldn't find '%s' input device", inputName);
    return fd;
}

int SensorBase::openIIODataDevice(const char *inputName) {
    int fd = -1;
    char buffer_access[PATH_MAX];
    int rate = 20, dev_num, enabled = 0, i;
    int rc = 0;
    int iio_buf_size;
    /* Add for compile begin */
    char fixed_sysfs_path[PATH_MAX];
    int fixed_sysfs_path_len;
    char mDevPath[PATH_MAX];
    char mTriggerName[PATH_MAX];
    /* Add for compile end */

    dev_num = find_type_by_name(inputName, "iio:device");
    if (dev_num < 0) {
        dev_num = 0;
    }

    snprintf(buffer_access, sizeof(buffer_access), "/dev/iio:device%d", dev_num);

    fd = open(buffer_access, O_RDWR);
    if (fd < 0)  {
        SH_ERR("open file '%s' failed: %s", buffer_access, strerror(errno));
    } else {
        SH_LOG("open file '%s' success! fd =%d", buffer_access, fd);
    }

    if (fd >= 0) {
        snprintf(fixed_sysfs_path, PATH_MAX, "%s", udevName);
        // strcpy(fixed_sysfs_path, udevName); 2-29
        fixed_sysfs_path_len = strlen(fixed_sysfs_path);

        snprintf(mDevPath, sizeof(mDevPath), "%s%s", fixed_sysfs_path, "iio");

        snprintf(mTriggerName, sizeof(mTriggerName), "%s-dev%d", inputName, dev_num);
    #if 0
        if (sysfs_set_input_attr_by_int(mDevPath, "buffer/length", IIO_MAX_BUFF_SIZE) < 0)
            SH_ERR("Set IIO buffer length failed: %s", strerror(errno));
    #endif
        // This is a piece of paranoia that retry for current_trigger
        for (i = 0; i < 5; i++) {
            rc = sysfs_set_input_attr(mDevPath, "trigger/current_trigger", mTriggerName, strlen(mTriggerName));
            if (rc < 0) {
                if (sysfs_set_input_attr_by_int(mDevPath, "buffer/enable", 1) < 0) {
                    SH_ERR("Set IIO buffer enable failed: %s", strerror(errno));
                }
                SH_ERR("Set current trigger failed: rc = %d, strerr() = %s", rc, strerror(errno));
            } else {
                SH_LOG("Set current trigger success!");
                break;
            }
        }

        iio_buf_size = IIO_MAX_BUFF_SIZE;
        for (i = 0; i < IIO_BUF_SIZE_RETRY; i++) {
            if (sysfs_set_input_attr_by_int(mDevPath, "buffer/length", IIO_MAX_BUFF_SIZE) < 0) {
                SH_ERR("Set IIO buffer length (%d) failed: %s", iio_buf_size, strerror(errno));
            } else {
                if (sysfs_set_input_attr_by_int(mDevPath, "buffer/enable", 1) < 0) {
                    SH_ERR("Set IIO buffer enable failed: %s, iio_buf_size = %d", strerror(errno), iio_buf_size);
                } else {
                    SH_LOG("Set IIO buffer length (%d) success!", iio_buf_size);
                    break;
                }
            }
            iio_buf_size /= 2;
        }
    }

    return fd;
}

int SensorBase::openDataDevice(const char* inputName) {
    switch (drvMode) {
        case 0: /* Input */
            dataFd = openInputDataDevice(inputName);
            break;

        case 1: /* IIO */
            dataFd = openIIODataDevice(inputName);
            break;

        default:
            SH_ERR("openDataDevice failed, Unknown type: %d", drvMode);
            break;
    }

    SH_LOG("drvMode = %d, uinputName = %s, dataFd = %d", drvMode, uinputName, dataFd);

    return dataFd;
}

int SensorBase::closeDataDevice() {

    if (dataFd >= 0) {
       close(dataFd);
       dataFd = -1;
       return 0;
    }
    return -1;
}

/*
 * Basic function: control
 * */
int SensorBase::openIOCtlDevice() {
    int fd = -1;

    if (udevName) {
        fd = open(udevName, O_RDONLY);
        ALOGE_IF(fd < 0, "Couldn't open %s (%s)", udevName, strerror(errno));
    }

    return fd;
}

int SensorBase::openSysfsCtlDevice() {
    int fd = -1;

    return fd;
}

int SensorBase::openCtlDevice() {
    switch (ctlMode) {
        case 0: /* IO ctl */
            devFd = openIOCtlDevice();
            break;

        case 1: /* Sysfs */
            devFd = openSysfsCtlDevice();
            break;

        default:
            SH_ERR("openIOCtlDevice failed, Unknown type: %d", ctlMode);
            break;
    }

    SH_LOG("ctlMode = %d, udevName = %s, devFd = %d", ctlMode, udevName, devFd);

    return devFd;
}

int SensorBase::closeCtlDevice() {
    if (devFd >= 0) {
        close(devFd);
        devFd = -1;
    }
    return 0;
}

int SensorBase::getFd() const {
    if (!uinputName) {
        return devFd;
    }
    return dataFd;
}
