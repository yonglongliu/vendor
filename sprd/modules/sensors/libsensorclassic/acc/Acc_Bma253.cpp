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
#include <sys/select.h>
#include <cutils/log.h>

#include "AccSensor.h"

#define ACC_SENSOR_RANGE		GRAVITY_EARTH*4.0f	//-4G~4G
//depends on sensor chip output data register valid bits numbers,BMA253 12bits
#define ACC_SENSOR_SENSITIVITY		((ACC_SENSOR_RANGE *2) /4096.0f) //3    //3.91mg/LSB
#define BMA_ACC_INPUT_NAME "accelerometer"
#define SYSFS_PATH_CLASS_INPUT "/sys/class/input/input"
#define SYSFS_NODE_BMA_ENABLE "enable"
#define SYSFS_NODE_BMA_DELAY "delay"
#define ACC_UNIT_CONVERSION(value) ((value)*ACC_SENSOR_SENSITIVITY)

/*****************************************************************************/
static struct sensor_t sSensorList[] = {
	{"BMA253 3-axis Accelerometer",
	"Bosch Sensortec",
	1,
	SENSORS_ACCELERATION_HANDLE,
	SENSOR_TYPE_ACCELEROMETER,
	ACC_SENSOR_RANGE,
	ACC_SENSOR_SENSITIVITY,
	0.145f,
	8000, // us fastest is 100Hz
	0,
	0,
	SENSOR_STRING_TYPE_ACCELEROMETER,
	"",
	200000,//unused
	SENSOR_FLAG_CONTINUOUS_MODE,
	{}},
};

AccSensor::AccSensor()
    : SensorBase(NULL, BMA_ACC_INPUT_NAME),
      mEnabled(0),
      mDelay(-1),
      mLayout(0),
      mInputReader(32),
      mHasPendingEvent(false),
      input_sysfs_path_len(0),
      mSensorCoordinate() {
	mPendingEvent.version = sizeof(sensors_event_t);
	mPendingEvent.sensor = ID_A;
	mPendingEvent.type = SENSOR_TYPE_ACCELEROMETER;
	memset(mPendingEvent.data, 0, sizeof(mPendingEvent.data));

	int InputNum = getInputNum(BMA_ACC_INPUT_NAME);
	ALOGE_IF(InputNum < 0, "couldn't find '%s' input device InputNum=%d",SYSFS_PATH_CLASS_INPUT, InputNum);
	sprintf(input_sysfs_path, "%s%d",
				SYSFS_PATH_CLASS_INPUT, InputNum);
	setInitialState();
}

AccSensor::~AccSensor() {
  if (mEnabled) {
    setEnable(0, 0);
  }
}

int AccSensor::setInitialState() {
  struct input_absinfo absinfo;
  int clockid = CLOCK_BOOTTIME;


  if (!ioctl(data_fd, EVIOCGABS(EVENT_TYPE_ACCEL_X), &absinfo)) {
    mPendingEvent.acceleration.x = ACC_UNIT_CONVERSION(absinfo.value);
  }
  if (!ioctl(data_fd, EVIOCGABS(EVENT_TYPE_ACCEL_Y), &absinfo)) {
    mPendingEvent.acceleration.y = ACC_UNIT_CONVERSION(absinfo.value);
  }
  if (!ioctl(data_fd, EVIOCGABS(EVENT_TYPE_ACCEL_Z), &absinfo)) {
    mPendingEvent.acceleration.z = ACC_UNIT_CONVERSION(absinfo.value);
  }

  if (!ioctl(data_fd, EVIOCSCLOCKID, &clockid)) {
    ALOGD("AccSensor: set EVIOCSCLOCKID = %d\n", clockid);
  } else {
    ALOGE("AccSensor: set EVIOCSCLOCKID failed \n");
  }

  return 0;
}

bool AccSensor::hasPendingEvents() const { return mHasPendingEvent; }
static int enabled_count = 0;
int AccSensor::setEnable(int32_t handle, int enabled) {
	int err = 0;
	char writeBuffer[8];
	int writeBytes;
	char sysfsEnable[PATH_MAX];

	/* handle check */
	if (handle != ID_A) {
		ALOGE("AccSensor: Invalid handle (%d)", handle);
		return -EINVAL;
	}

	ALOGD("AccSensor: handle=%d,enabled=%d\n", handle, enabled);
	sprintf(sysfsEnable, "%s/%s",
				input_sysfs_path, SYSFS_NODE_BMA_ENABLE);
	if (enabled) {
		  setInitialState();
		writeBytes = sprintf(writeBuffer, "%d\n", 1) + 1;
		mEnabled = 1;
		enabled_count += 1;
	} else {
		writeBytes = sprintf(writeBuffer, "%d\n", 0) + 1;
		mEnabled = 0;
		if(enabled_count == 0){
		ALOGE("AccSensor: enabled_count already equals to 0,skip!\n");
		} else {
		enabled_count -= 1;
		}
	}

	if( (!mEnabled) && enabled_count ){
	ALOGE("AccSensor: mEnabled=0 while enabled_count!=0,skip!\n");
	} else {
	err = write_sys_attribute(sysfsEnable, writeBuffer, writeBytes);
	}

	ALOGD("AccSensor: mEnabled = %d,enabled_count = %d\n", mEnabled,enabled_count);
  return err;
}

int AccSensor::setDelay(int32_t handle, int64_t delay_ns) {
  int err = 0;
  int ms;
  char sysfsEnable[PATH_MAX];
  char writeBuffer[8];

  /* handle check */
  if (handle != ID_A) {
    ALOGE("AccSensor: Invalid handle (%d)", handle);
    return -EINVAL;
  }
  ALOGE("AccSensor: mDelay(%lld) delay_ns(%lld)", mDelay, delay_ns);
  if (mDelay != delay_ns) {
	ms = delay_ns /1000000;
	ALOGE("AccSensor: ms(%d) ", ms);
	int writeBytes = sprintf(writeBuffer, "%d\n", ms) + 1;
	sprintf(sysfsEnable, "%s/%s",
				input_sysfs_path, SYSFS_NODE_BMA_DELAY);
	err = write_sys_attribute(sysfsEnable, writeBuffer, writeBytes);
	if (err != 0) {
	    ALOGE("AccSensor:setDelay fail (%s)", strerror(errno));
	    return -errno;
	}
	mDelay = delay_ns;
  }
  return err;
}

int64_t AccSensor::getDelay(int32_t handle) {
  return (handle == ID_A) ? mDelay : 0;
}

int AccSensor::getEnable(int32_t handle) {
  return (handle == ID_A) ? mEnabled : 0;
}

int AccSensor::readEvents(sensors_event_t *data, int count) {
  if (count < 1) return -EINVAL;

  if (mHasPendingEvent) {
    mHasPendingEvent = false;
    mPendingEvent.timestamp = getTimestamp();
    *data = mPendingEvent;
    return mEnabled ? 1 : 0;
  }

  ssize_t n = mInputReader.fill(data_fd);
  if (n < 0) return n;

  int numEventReceived = 0;
  input_event const *event;
  static float acc_latest_x;
  static float acc_latest_y;
  static float acc_latest_z;

  while (count && mInputReader.readEvent(&event)) {
    int type = event->type;
    if (type == EV_ABS) {
      float value = event->value;
      if (event->code == EVENT_TYPE_ACCEL_X) {
        acc_latest_x = ACC_UNIT_CONVERSION(value);
      } else if (event->code == EVENT_TYPE_ACCEL_Y) {
        acc_latest_y = ACC_UNIT_CONVERSION(value);
      } else if (event->code == EVENT_TYPE_ACCEL_Z) {
        acc_latest_z = ACC_UNIT_CONVERSION(value);
      }
    } else if (type == EV_SYN) {
      mPendingEvent.timestamp = timevalToNano(event->time);
      if (mEnabled || enabled_count) {
        mPendingEvent.acceleration.x = acc_latest_x;
        mPendingEvent.acceleration.y = acc_latest_y;
        mPendingEvent.acceleration.z = acc_latest_z;
        mSensorCoordinate.coordinate_data_convert(mPendingEvent.acceleration.v,
                                                  INSTALL_DIR);
        *data++ = mPendingEvent;
        count--;
        numEventReceived++;
      }
    } else {
      ALOGE("AccSensor: unknown event (type=%d, code=%d)", type, event->code);
    }
    mInputReader.next();
  }

  return numEventReceived;
}

int AccSensor::getInputNum(const char *InputName)
{
    int num = -1;
    char filename[64];
    int err;
    char ev_name[64];
    int fd;

    for (int i = 0; i < 100; i++) {
        sprintf(filename, "/dev/input/event%d", i);
        fd = open(filename, O_RDONLY);
        if (fd >= 0) {
            err = ioctl(fd, EVIOCGNAME(sizeof(ev_name) - 1), &ev_name);
            ALOGD("get input device ev_name: %s", ev_name);
            if (err < 1)
                ev_name[0] = '\0';
            if (0 == strcmp(ev_name, InputName)) {
                num = i;
                close(fd);
                break;
            }
            close(fd);
        } else
	ALOGE("AccSensor: open %s failedi fd=%d\n",filename, fd);
    }
    return num;
}

int AccSensor::populateSensorList(struct sensor_t *list) {
  memcpy(list, sSensorList, sizeof(struct sensor_t) * numSensors);
  return numSensors;
}
