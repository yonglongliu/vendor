
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

#define QMAX981_HAL_DRIVER_USING_IOCTL

#define QMAX981_ACC_DEV_PATH_NAME    "/dev/qmax981"
#define QMAX981_ACC_INPUT_NAME  "accelerometer"

#define QMAX981_ACC_IOCTL_BASE                   77
/** The following define the IOCTL command values via the ioctl macros */
#define QMAX981_ACC_IOCTL_SET_DELAY              _IOW(QMAX981_ACC_IOCTL_BASE, 0, int)
#define QMAX981_ACC_IOCTL_GET_DELAY              _IOR(QMAX981_ACC_IOCTL_BASE, 1, int)
#define QMAX981_ACC_IOCTL_SET_ENABLE             _IOW(QMAX981_ACC_IOCTL_BASE, 2, int)
#define QMAX981_ACC_IOCTL_GET_ENABLE             _IOR(QMAX981_ACC_IOCTL_BASE, 3, int)

//#define QMAX981_ACC_STEP_COUNT
#ifdef QMAX981_ACC_STEP_COUNT
#define QMAX981_ACC_IOCTL_SET_STEPCOUNT_ENABLE		_IOW(QMAX981_ACC_IOCTL_BASE,5,int)
#define QMAX981_ACC_IOCTL_GET_STEPCOUNT_ENABLE		_IOR(QMAX981_ACC_IOCTL_BASE,6,int)
#endif


#define ACC_UNIT_CONVERSION(value) ((value) * GRAVITY_EARTH / (1024.0f))

static int64_t  previous_timestamp2 = 0;
/*****************************************************************************/
static struct sensor_t sSensorList[] = {
        {
			"QST QMAX981 3-axis Accelerometer",
			 "QST",
			 1, 
			 SENSORS_ACCELERATION_HANDLE,
			 SENSOR_TYPE_ACCELEROMETER, 
			 (GRAVITY_EARTH * 2.0f),
			 (GRAVITY_EARTH) / 1024.0f,
			 0.145f,
			 5000,
			 0,
			 0,
			 SENSOR_STRING_TYPE_ACCELEROMETER,
			 "",
			 1000000,
			 SENSOR_FLAG_CONTINUOUS_MODE,
			 {}
		},
#ifdef QMAX981_ACC_STEP_COUNT		 	 
		{
			"QST QMAX981 Step Counter",
			"QST",
			1,
			SENSORS_STEP_COUNTER_HANDLE,
			SENSOR_TYPE_STEP_COUNTER,
			65535.0f,
			1.0f,
			1.0f,
			0,
			0,
			0,
			SENSOR_STRING_TYPE_STEP_COUNTER,
			"",
			0,
			SENSOR_FLAG_ON_CHANGE_MODE,
			{}
		},
#endif
};

AccSensor::AccSensor() :
        SensorBase(QMAX981_ACC_DEV_PATH_NAME, QMAX981_ACC_INPUT_NAME),
                mEnabled(0), mDelay(-1), mInputReader(32), mHasPendingEvent(false),
                mSensorCoordinate()
{
        mPendingEvent.version = sizeof(sensors_event_t);
        mPendingEvent.sensor = ID_A;
        mPendingEvent.type = SENSOR_TYPE_ACCELEROMETER;
        memset(mPendingEvent.data, 0, sizeof(mPendingEvent.data));

#ifdef QMAX981_ACC_STEP_COUNT		
	 mPendingEvent.version = sizeof(sensors_event_t);
        mPendingEvent.sensor = ID_STEP;
        mPendingEvent.type = SENSOR_TYPE_STEP_COUNTER;
        memset(mPendingEvent.data, 0, sizeof(mPendingEvent.data));
#endif
        open_device();

    ALOGD("AccSensor::AccSensor(Acc_QMAX981.cpp): construct called");
}

AccSensor::~AccSensor()
{
        if (mEnabled) {
                setEnable(0, 0);
        }

        close_device();

    ALOGD("AccSensor::~AccSensor(Acc_QMAX981.cpp): destroy called");
}

int AccSensor::setInitialState()
{
        struct input_absinfo absinfo;
        int clockid = CLOCK_BOOTTIME;

        if (mEnabled) {
                if (!ioctl(data_fd, EVIOCGABS(EVENT_TYPE_ACCEL_X), &absinfo)) {
                        mPendingEvent.acceleration.x = ACC_UNIT_CONVERSION(absinfo.value);
                }
                if (!ioctl(data_fd, EVIOCGABS(EVENT_TYPE_ACCEL_Y), &absinfo)) {
                        mPendingEvent.acceleration.y = ACC_UNIT_CONVERSION(absinfo.value);
                }
                if (!ioctl(data_fd, EVIOCGABS(EVENT_TYPE_ACCEL_Z), &absinfo)) {
                        mPendingEvent.acceleration.z = ACC_UNIT_CONVERSION(absinfo.value);
                }
        }
		
	    if (!ioctl(data_fd, EVIOCSCLOCKID, &clockid))
        {
            ALOGD("AccSensor: set EVIOCSCLOCKID = %d\n", clockid);
        }
        else
        {
            ALOGE("AccSensor: set EVIOCSCLOCKID failed \n");
        }	
			
        return 0;
}

bool AccSensor::hasPendingEvents() const
{
        return mHasPendingEvent;
}

int AccSensor::setEnable(int32_t handle, int enabled)
{
        int err = 0;
        int opDone = 0;

#ifndef QMAX981_HAL_DRIVER_USING_IOCTL
    char buffer[2];
        buffer[0] = '\0';
        buffer[1] = '\0';
#endif

        /* handle check */
        if ((handle != ID_A))
//#ifdef QMAX981_ACC_STEP_COUNT
//			||(handle != ID_STEP)
//#endif
	{
                ALOGE("AccSensor: Invalid handle (%d)", handle);
                return -EINVAL;
        }

#ifdef QMAX981_HAL_DRIVER_USING_IOCTL
if (handle == ID_A)
{
        if (mEnabled <= 0) {
                if (enabled) {
                        ALOGD("AccSensor:enable ioctl");
                        err = ioctl(dev_fd, QMAX981_ACC_IOCTL_SET_ENABLE,
                                  &enabled);
                        opDone = 1;
                        ALOGD("AccSensor:enable ioctl done");
                }
        } else if (mEnabled == 1) {
                if (!enabled) {
                        ALOGD("AccSensor:enable ioctl false");
                        err = ioctl(dev_fd, QMAX981_ACC_IOCTL_SET_ENABLE,
                                  &enabled);
                        opDone = 1;
                }
    }
}
#ifdef QMAX981_ACC_STEP_COUNT
else
{
	      if (mEnabled <= 0) {
                if (enabled) {
                        ALOGD("AccSensor:enable ioctl");
                        err = ioctl(dev_fd, QMAX981_ACC_IOCTL_SET_STEPCOUNT_ENABLE,
                                  &enabled);
                        opDone = 1;
                        ALOGD("AccSensor:enable ioctl done");
                }
        } else if (mEnabled == 1) {
                if (!enabled) {
                        ALOGD("AccSensor:enable ioctl false");
                        err = ioctl(dev_fd, QMAX981_ACC_IOCTL_SET_STEPCOUNT_ENABLE,
                                  &enabled);
                        opDone = 1;
                }
    }
}
#endif
#else
        if (mEnabled<= 0) {
                if(enabled) buffer[0] = '1';
        } else if (mEnabled == 1) {
                if(!enabled) buffer[0] = '0';
        }

    if (buffer[0] != '\0') {
                err = write_sys_attribute(sysfs_enable, buffer, 1);
        opDone = 1;
    }
#endif
        if (err != 0) {
                ALOGE("AccSensor: IOCTL failed (%s)", strerror(errno));
                return err;
        }
        if (opDone) {
                ALOGD("AccSensor: Control set %d", enabled);
                setInitialState();
        }

        if (enabled) {
                mEnabled++;
                if (mEnabled > 32767)
                        mEnabled = 32767;
        } else {
                mEnabled--;
                if (mEnabled < 0)
                        mEnabled = 0;
        }
    if(mEnabled == 0)
	{
		previous_timestamp2 = 0;
	}
		ALOGD("AccSensor(Acc_QMAX981.cpp): mEnabled = %d", mEnabled);

        return err;
}

int AccSensor::setDelay(int32_t handle, int64_t delay_ns)
{
        int err = 0;
        int ms;

#ifndef QMAX981_HAL_DRIVER_USING_IOCTL
        char buffer[32];
        int bytes;
#endif

        /* handle check */
        if (handle != ID_A) {
                ALOGE("AccSensor: Invalid handle (%d)", handle);
                return -EINVAL;
        }

        if (mDelay != delay_ns) {
                ms = delay_ns / 1000000;
#ifdef QMAX981_HAL_DRIVER_USING_IOCTL
                if (ioctl(dev_fd, QMAX981_ACC_IOCTL_SET_DELAY, &ms)) {
#else
                bytes = sprintf(buffer, "%d", ms);
                if (write_sys_attribute(sysfs_poll, buffer, bytes)){
#endif
                        return -errno;
                }
                mDelay = delay_ns;
        }

        ALOGD("AccSensor(Acc_QMAX981.cpp): delay = %d", delay_ns);

        return err;
}

int64_t AccSensor::getDelay(int32_t handle)
{
        return (handle == ID_A) ? mDelay : 0;
}

int AccSensor::getEnable(int32_t handle)
{
        return (handle == ID_A) ? mEnabled : 0;
}

int AccSensor::readEvents(sensors_event_t * data, int count)
{
        if (count < 1)
                return -EINVAL;

        if (mHasPendingEvent) {
                mHasPendingEvent = false;
                mPendingEvent.timestamp = getTimestamp();
                *data = mPendingEvent;
                return mEnabled ? 1 : 0;
        }

        ssize_t n = mInputReader.fill(data_fd);
        if (n < 0)
                return n;

        int numEventReceived = 0;
        input_event const *event;
        static float acc_latest_x;
        static float acc_latest_y;
        static float acc_latest_z;
#ifdef QMAX981_ACC_STEP_COUNT			
	 static float acc_latest_stepcount;
#endif

        while (count && mInputReader.readEvent(&event)) {
                int type = event->type;
                if (type == EV_ABS) {
                        float value = event->value;
                        if (event->code == EVENT_TYPE_ACCEL_X) {
                                //mPendingEvent.acceleration.x = ACC_UNIT_CONVERSION(value);
                                acc_latest_x = ACC_UNIT_CONVERSION(value);
                        } else if (event->code == EVENT_TYPE_ACCEL_Y) {
                                //mPendingEvent.acceleration.y = ACC_UNIT_CONVERSION(value);
                                acc_latest_y = ACC_UNIT_CONVERSION(value);
                        } else if (event->code == EVENT_TYPE_ACCEL_Z) {
                                //mPendingEvent.acceleration.z = ACC_UNIT_CONVERSION(value);
                                acc_latest_z = ACC_UNIT_CONVERSION(value);
                        }
					#ifdef QMAX981_ACC_STEP_COUNT						
							else if (event->code == ABS_RX){
									acc_latest_stepcount   = ACC_UNIT_CONVERSION(value);
						}
					#endif		
                } else if (type == EV_SYN) {
                        mPendingEvent.timestamp = timevalToNano(event->time);
                        if (mEnabled) {
                                mPendingEvent.acceleration.x = acc_latest_x;
                                mPendingEvent.acceleration.y = acc_latest_y;
                                mPendingEvent.acceleration.z = acc_latest_z;
							#ifdef QMAX981_ACC_STEP_COUNT									
								mPendingEvent.u64.step_counter = acc_latest_stepcount;		
							#endif
                                mSensorCoordinate.coordinate_data_convert(
                                                mPendingEvent.acceleration.v, INSTALL_DIR);
											if((previous_timestamp2>0) && (mPendingEvent.timestamp>previous_timestamp2) && (mDelay>0))
				{
					int loop_cnt, i;
					int64_t temp_time;
					loop_cnt = (2*(mPendingEvent.timestamp-previous_timestamp2) + mDelay)/(2*mDelay);
					previous_timestamp2 = mPendingEvent.timestamp;
					if(loop_cnt > 0)
					{
						temp_time = mPendingEvent.timestamp;
						for(i=loop_cnt; i>0; i--)
						{
							mPendingEvent.timestamp = temp_time - (i-1)*mDelay;
                                *data++ = mPendingEvent;
                                count--;
                                numEventReceived++;
									if(count <= 0)
							{
								break;
							}
						}
					}
					else
					{
						*data++ = mPendingEvent;
						count--;
						numEventReceived++;
					}
				}
				else
				{
					*data++ = mPendingEvent;
					count--;
					numEventReceived++;
					previous_timestamp2 = mPendingEvent.timestamp;
				}
                        }
                } else {
                        ALOGE("AccSensor: unknown event (type=%d, code=%d)",
                              type, event->code);
                }
                mInputReader.next();
        }

        return numEventReceived;
}

int AccSensor::populateSensorList(struct sensor_t *list)
{
        memcpy(list, sSensorList, sizeof(struct sensor_t) * numSensors);
        return numSensors;
}
