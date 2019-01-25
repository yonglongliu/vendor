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

#include "AkmOri.h"
#include "Ori_akm099xx.h"

#define MSENSOR                                            0x83
#define MSENSOR_IOCTL_MSENSOR_ENABLE             _IOW(MSENSOR, 0x55, int)
#define MSENSOR_IOCTL_OSENSOR_ENABLE             _IOW(MSENSOR, 0x56, int)
#define AKM099XX_IOC_SET_DELAY                   _IOW(MSENSOR, 0x29, short)
#define ECOMPASS_IOC_GET_DELAY                   _IOR(MSENSOR, 0x1d, int)

/*****************************************************************************/
static struct sensor_t sSensorList[] = {
	{
		"AKM099XX Magnetic field sensor",
		"akm099xx",
		1,
		SENSORS_MAGNETIC_FIELD_HANDLE,
		SENSOR_TYPE_MAGNETIC_FIELD, 
		65535.0f,
		CONVERT_M, 
		0.35f, 
		50000, 
		0, 
		0, 
		SENSOR_STRING_TYPE_MAGNETIC_FIELD,
		"",
		10000,
		SENSOR_FLAG_CONTINUOUS_MODE,
		{}
	},
	{
		"MEMSIC Orientation sensor",
		"akm099xx",
		1, 
		SENSORS_ORIENTATION_HANDLE,
		SENSOR_TYPE_ORIENTATION, 
		360.0f,
		CONVERT_O, 
		0.495f, 
		50000, 
		0, 
		0, 
		SENSOR_STRING_TYPE_ORIENTATION,
		"",
		10000,
		SENSOR_FLAG_CONTINUOUS_MODE,
		{}
	},
};

static float offset_value[3] = {0 ,0 ,0};

OriSensor::OriSensor()
: SensorBase("/dev/AKM099XX", "compass"),
	enabled(0),
	mPendingMask(0),
	mMaEnabled(0),
	mOrEnabled(0),
	mInputReader(32)
{

	memset(mPendingEvents, 0, sizeof(mPendingEvents));

	mPendingEvents[MagneticField].version = sizeof(sensors_event_t);
	mPendingEvents[MagneticField].sensor = ID_M;
	mPendingEvents[MagneticField].type = SENSOR_TYPE_MAGNETIC_FIELD;
	mPendingEvents[MagneticField].magnetic.status = SENSOR_STATUS_ACCURACY_HIGH;

	mPendingEvents[Orientation  ].version = sizeof(sensors_event_t);
	mPendingEvents[Orientation  ].sensor = ID_O;
	mPendingEvents[Orientation  ].type = SENSOR_TYPE_ORIENTATION;
	mPendingEvents[Orientation  ].orientation.status = SENSOR_STATUS_ACCURACY_HIGH;

	for (int i=0 ; i<__numSensors ; i++)
	{
		mEnabled[i] = 0;
		mDelay[i] = 50000000; // 50 ms by default
	}

	open_device();
	AKMInitLib(0x05,0x00);
	AKMCaliApiSetOffset(offset_value);
}

OriSensor::~OriSensor() {
}

int OriSensor::setEnable(int32_t handle, int en)
{
	int what = -1;
	int cmd = -1;
	int err = 0;
	int newState  = en ? 1 : 0;

	int clockid = CLOCK_BOOTTIME;
	if (!ioctl(data_fd, EVIOCSCLOCKID, &clockid))
	{
		ALOGD("OriSensor: set EVIOCSCLOCKID = %d\n", clockid);
	}
	else
	{
		ALOGE("OriSensor: set EVIOCSCLOCKID failed \n");
	}

	what = handle2id(handle);

	if (what >= __numSensors)
		return -EINVAL;

	if ((newState<<what) != (enabled & (1<<what))) {
		switch (what) {
			case MagneticField:
				cmd = MSENSOR_IOCTL_MSENSOR_ENABLE;
				mMaEnabled = en;
				break;

			case Orientation:
				cmd = MSENSOR_IOCTL_MSENSOR_ENABLE;
				mOrEnabled = en;
				break;
			default:
				ALOGE("AKM099XX unknown cmd \n");
				return -EINVAL;

		}  
		ALOGE("OriSensor XXXXX cmd = %d,newState = %d ,what = %d \n",cmd,newState,what);
 
		int flags = newState;
		if( !newState && (mMaEnabled||mOrEnabled) ){
			err = 0;
		ALOGE("OriSensor (mMaEnabled||mOrEnabled)!=0,skip!\n");
		}else{
		err = ioctl(dev_fd, cmd, &flags);
		if (err)
		{
			ALOGE("OriSensor akm099xx compass set enable failed\n");   
		}
		err = err<0 ? -errno : 0;
		}

		if (!err) {
			enabled &= ~(1<<what);
			enabled |= (uint32_t(flags)<<what);
		}   
	}   
	return err;

}

int OriSensor::getEnable(int32_t handle)
{
        int id = handle2id(handle);
        if (id >= 0) {
                return enabled;
         } else {
                return 0;
         }
}

int64_t OriSensor::getDelay(int32_t handle)
{

        int id = handle2id(handle);
        if (id > 0) {
                return mDelay[id];
        } else {
                return 0;
        }
}

int OriSensor::setDelay(int32_t handle, int64_t ns)
{
	int what = -1;

	what = handle2id(handle);

	if (what >= __numSensors)
		return -EINVAL;

	if (ns < 0)
		return -EINVAL;

	mDelay[what] = ns;

	if (enabled) {
		uint64_t wanted = -1LLU;
		for (int i=0 ; i<__numSensors ; i++) {
			if (enabled & (1<<i)) {
				uint64_t ns = mDelay[i];
				wanted = wanted < ns ? wanted : ns;
			}
		}
		int delay = int64_t(wanted) / 1000000;

		ALOGE("OriSensor AKM099XX delay time = %d\n", delay);
		if (ioctl(dev_fd, AKM099XX_IOC_SET_DELAY, &delay)) {
			return -errno;
		}
	}
	return 0;
}

int OriSensor::readEvents(sensors_event_t* data, int count)
{
	int numEventReceived = 0;
	input_event const* event;

	int64_t ts = 0;
	float ucalibmag[3] = {0,0,0};
	int64_t status = 0;
	float calibmag[3];
	float magbias[3];
	float ori_value[3];
	int8_t lv;

	if (count < 1)
		return -EINVAL;

	ssize_t n = mInputReader.fill(data_fd);

	if (n < 0)
		return n;

	while (count && mInputReader.readEvent(&event))
	{
		int type = event->type;
		if (type == EV_ABS) {
			processEvent(event->code, event->value);
			mInputReader.next();
		} else if (type == EV_SYN) {
			int64_t time = timevalToNano(event->time);
			if(mMaEnabled || mOrEnabled)
			{
				if(mMaEnabled)
				{
					AKMDoCaliApi(time, mag_value, 0,calibmag, magbias, &lv);
					mPendingEvents[MagneticField].magnetic.x = calibmag[0];
					mPendingEvents[MagneticField].magnetic.y = calibmag[1];
					mPendingEvents[MagneticField].magnetic.z = calibmag[2];
				}

				if(mOrEnabled)
				{
					acc_value[0] = acc_value[0] * 65536;
					acc_value[1] = acc_value[1] * 65536;
					acc_value[2] = acc_value[2] * 65536;

					AKMCaliApiSetAccData(time, acc_value, 0);
					GetOrientation(ori_value);
					mPendingEvents[Orientation].orientation.azimuth =ori_value[0]/65536;
					mPendingEvents[Orientation].orientation.pitch = ori_value[1]/65536;
					mPendingEvents[Orientation].orientation.roll = ori_value[2]/65536;
					mPendingMask |= 1<<Orientation;	
				}
			}
			for (int j=0 ; count && mPendingMask && j<__numSensors ; j++) {
				if (mPendingMask & (1<<j)) {
					mPendingMask &= ~(1<<j);
					mPendingEvents[j].timestamp = time;
					if (enabled & (1<<j)) {
						*data++ = mPendingEvents[j];
						count--;
						numEventReceived++;
					}
				}
			}
			if (!mPendingMask) {
				mInputReader.next();
			}
		} else {
			ALOGE("AKM099XXSensor: unknown event (type=%d, code=%d)",
					type, event->code);
			mInputReader.next();
		}
	}

	return numEventReceived;
}

int OriSensor::setAccel(sensors_event_t* data)
{
	acc_value[0] = data->acceleration.x;
	acc_value[1] = data->acceleration.y;
	acc_value[2] = data->acceleration.z;
	
	return 0;
}

int OriSensor::handle2id(int32_t handle)
{
	ALOGE("OriSensor AKM099XX:handle2id handle = %d \n",handle);
	switch (handle) {
		case ID_A:
			return Accelerometer;
		case ID_M:
			return MagneticField;
		case ID_O:
			return Orientation;
		default:
		ALOGE("AKM099XX: unknown handle (%d)", handle);
		return -EINVAL;
	}
}

void OriSensor::processEvent(int code, int value)
{
	switch (code) {
		case EVENT_TYPE_MAGV_X:
			mPendingMask |= 1<<MagneticField;
			mag_value[0] = value;
			break;
		case EVENT_TYPE_MAGV_Y:
			mPendingMask |= 1<<MagneticField;
			mag_value[1] = -value;
			break;
		case EVENT_TYPE_MAGV_Z:
			mPendingMask |= 1<<MagneticField;
			mag_value[2] = -value;
			break;
	  default:
	  	ALOGE("unkonw event type\n");
	}
}

int OriSensor::populateSensorList(struct sensor_t *list)
{
        memcpy(list, sSensorList, sizeof(struct sensor_t) * numSensors);
        return numSensors;
}
