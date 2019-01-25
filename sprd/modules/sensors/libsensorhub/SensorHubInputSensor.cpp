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
#include <linux/input.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include "SensorHubInputSensor.h"

#define WORKROUND_FACEUPDOWN
#define CHANGE_TO_OLD_FORMAT

int SensorHubInputSensor::cali_firmware_flag = 0;

SensorHubInputSensor::SensorHubInputSensor(const shub_drv_cfg_t & drvCfg)
    : SensorBase(drvCfg, 0),
    mInputReader(NULL),
    mDynamicLogel(NULL),
    mSensorList(drvCfg.sensorList),
    mTotalNumber(drvCfg.listSize),
    mSensorInfo(NULL),
    mHasPendingEvent(0),
    mEnabled(0) {
    mSensorInfo = (pSensorInfo_t)calloc(1, (mTotalNumber + 1) * sizeof(SensorInfo_t));
    if (mSensorInfo) {
        for (int i = 0; i < mTotalNumber; i++) {
            mSensorInfo[i].mEvents.version   = sizeof(sensors_event_t);
            mSensorInfo[i].mEvents.sensor     = mSensorList[i].handle;
            mSensorInfo[i].mEvents.type       = mSensorList[i].type;
            mSensorInfo[i].mEvents.acceleration.status = SENSOR_STATUS_ACCURACY_HIGH;

            mSensorInfo[i].pSensorList       = &mSensorList[i];

            mSensorInfo[i].enabled   = 0;
            mSensorInfo[i].flush     = 0;
            mSensorInfo[i].period_ns = 0;
            mSensorInfo[i].timeout   = 0;
            SH_LOG("init %s\n", mSensorList[i].name);
        }
        mSensorInfo[mTotalNumber].enabled = 0;
        mSensorInfo[mTotalNumber].mEvents.version = META_DATA_VERSION;
        mSensorInfo[mTotalNumber].mEvents.sensor  = 0;
        mSensorInfo[mTotalNumber].mEvents.type    = SENSOR_TYPE_META_DATA;
        mSensorInfo[mTotalNumber].mEvents.meta_data.what = META_DATA_FLUSH_COMPLETE;
    } else {
        SH_ERR("Calloc memory [%" PRIuLEAST32"] failed\n", (uint32_t)((mTotalNumber + 1) * sizeof(SensorInfo_t)));
    }

    /* init input read buffer */
#ifdef EVENT_DYNAMIC_ADAPT
    if (0 == drvMode) {
        mInputReader = new InputEventCircularReader(10, sizeof(input_event));
    } else {
        mInputReader = new InputEventCircularReader(10, sizeof(iio_event));
    }
#else /* EVENT_DYNAMIC_ADAPT */
    mInputReader = new InputEventCircularReader(10);
#endif /* EVENT_DYNAMIC_ADAPT */

    mDynamicLogel = new SensorHubDynamicLogel(0);

#ifdef SENSORHUB_EXIST_OR_NOT
    int data[2];  // 1(handle)+1(pad)
    data[0] = 0;
    data[1] = 0xEF;
    sendCmdtoDriver(efCmdUpdateHubStatus, data);
#endif /* SENSORHUB_EXIST_OR_NOT */

    downloadFirmware();
}

SensorHubInputSensor::~SensorHubInputSensor() {
    if (!mEnabled.isEmpty()) {
        setEnable(0, 0);
    }
    delete mInputReader;
    free(mSensorInfo);
}

int SensorHubInputSensor::setInitialState() {
    return 0;
}

int SensorHubInputSensor::downloadFirmware(void) {
    char data[MAX_STRING_SIZE];
    readDatafromDriver(efCmdSendOpCode, data, MAX_STRING_SIZE);
    //sendCaliFirmware();
    return 0;
}

pSensorInfo_t SensorHubInputSensor::getSensorInfo(int32_t handle) {
    int idx = 0;

    for (idx = 0; idx < mTotalNumber; idx++) {
        if (mSensorInfo[idx].pSensorList->handle == handle) {
            SH_DBG("found sensor handle (%d) name: %s", mSensorInfo[idx].pSensorList->handle, mSensorInfo[idx].pSensorList->name);
            return &mSensorInfo[idx];
        }
    }

    SH_ERR("Nothing to find for handle[%d]", handle);

    return NULL;
}

int64_t SensorHubInputSensor::convertTimeToAp(int64_t time) {
#ifdef UseSysTimeStamp
    SH_DATA("McuT[%d], SysTime[%lld]\n", time, getTimestamp());
    return getTimestamp();
#else
#if 1

    if (getTimestamp() < (time*US) && (((time*US) -getTimestamp()) < 2000000000ll)) {
      while(getTimestamp() < (time*US))
      {
        usleep(1000);
      }
    }

    return time * US;
#else
    int64_t RealTime = (getTimestamp() / US) & 0xFFFFFFFF00000000ll;
    /*Mcu Time = ms*/
    int64_t McuTime = ((int64_t)time) & 0x00000000FFFFFFFFll;

    SH_DATA("McuT[%d], SysTime[%lld], ReportTime[%lld]\n", time, getTimestamp(), (RealTime | McuTime)*NS);
    return (RealTime | McuTime) * US;
#endif
#endif
}

int SensorHubInputSensor::sendCmdtoDriverIoctl(int operate, int* para) {
    int rst = -1;

    SH_SYSFS("operate = %d, handle = %d, type = %d, para = %d",
            operate, para[0], para[1], para[2]);

    switch (operate) {
        case efCmdsetEnable:
            switch (para[0]) {
                case SENSOR_HANDLE_ACCELEROMETER:
                    rst = ioctl(devFd, ACCELEROMETER_IOCTL_SET_ENABLE, &para[1]);
                    break;

                case SENSOR_HANDLE_LIGHT:
                    rst = ioctl(devFd, LIGHT_IOCTL_SET_ENABLE, &para[1]);
                    break;

                case SENSOR_HANDLE_PROXIMITY:
                    rst = ioctl(devFd, PROXIMITY_IOCTL_SET_ENABLE, &para[1]);
                    break;
                default:
                    break;
            }
            break;

        case efCmdsetDelay:
            switch (para[0]) {
                case SENSOR_HANDLE_ACCELEROMETER:
                    SH_SYSFS("ACCELEROMETER_IOCTL_SET_DELAY %d", para[1]);
                    rst = ioctl(devFd, ACCELEROMETER_IOCTL_SET_DELAY, &para[1]);
                    break;

                case SENSOR_HANDLE_LIGHT:
                    SH_SYSFS("SENSOR_HANDLE_LIGHT %d", para[1]);
                    break;

                case SENSOR_HANDLE_PROXIMITY:
                    SH_SYSFS("SENSOR_HANDLE_PROXIMITY %d", para[1]);
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
    if (rst != 0) {
        SH_ERR("IOCTL failed (%s)", strerror(errno));
    }

    return rst;
}

int SensorHubInputSensor::sendCmdtoDriverSysfs(int operate, int* para) {
    int rst = 0;
    int fd  = 0;
    char buf[128] = {0};
    int64_t data = 0;
    /*
     * Just for debug begin
     * */
    char fixed_sysfs_path[PATH_MAX];
    int fixed_sysfs_path_len;

    strcpy(fixed_sysfs_path, udevName);
    fixed_sysfs_path_len = strlen(fixed_sysfs_path);
    /*
     * Just for debug end
     * */
    switch (operate) {
        case efCmdsetEnable:
            strcpy(&fixed_sysfs_path[fixed_sysfs_path_len], "enable");
            sprintf(buf, "%d %d\n", para[0], para[1]);
            break;
        case efCmdsetDelay:
            strcpy(&fixed_sysfs_path[fixed_sysfs_path_len], "delay");
            sprintf(buf, "%d %d %d\n", para[0], para[1], para[2]);
            break;
        case efCmdbatch:
            strcpy(&fixed_sysfs_path[fixed_sysfs_path_len], "batch");
            memcpy(&data, &para[3], sizeof(int64_t));
            sprintf(buf, "%d %d %d %" SCNd64"\n", para[0], para[1], para[2], data);
            break;
        case efCmdflush:
            strcpy(&fixed_sysfs_path[fixed_sysfs_path_len], "flush");
            sprintf(buf, "%d\n", para[0]);
            break;
        case efCmdUpdateHubStatus:
            strcpy(&fixed_sysfs_path[fixed_sysfs_path_len], "sensorhub");
            sprintf(buf, "%d\n", para[0]);
            break;
        default:
            break;
    }

    SH_SYSFS("write (%s) %s", fixed_sysfs_path, buf);
    fd = open(fixed_sysfs_path, O_RDWR);
    if (fd >= 0) {
        rst = write(fd, buf, sizeof(buf));
        if (rst < 0)
            SH_ERR("write (%s) to (%s) failed!", buf, fixed_sysfs_path);

        close(fd);
    } else {
        SH_ERR("%s: open (%s) failed!", __func__, fixed_sysfs_path);
        return -EINVAL;
    }

    return rst;
}

int SensorHubInputSensor::sendCmdtoDriver(int operate, int* para) {
    int rst = 0;

    switch (ctlMode) {
        case 0: /* IO Ctl */
            rst = sendCmdtoDriverIoctl(operate, para);
            break;
        case 1: /* Sysfs */
            rst = sendCmdtoDriverSysfs(operate, para);
            break;
        default:
            break;
    }

    rst = (rst < 0) ? rst : 0;
    return rst;
}

int SensorHubInputSensor::readDatafromDriverSysfs(int operate, char *data , int len) {
    char buff[MAX_STRING_SIZE] = {0};
    int fd = 0;
    int err = 0;

    if (len > MAX_STRING_SIZE)
        len = MAX_STRING_SIZE;
    /*
     * Just for debug begin
     * */
    char fixed_sysfs_path[PATH_MAX];
    int fixed_sysfs_path_len;

    strcpy(fixed_sysfs_path, udevName);
    fixed_sysfs_path_len = strlen(fixed_sysfs_path);
    /*
     * Just for debug end
     * */

    switch (operate) {
        case efCmdSendOpCode:
            strcpy(&fixed_sysfs_path[fixed_sysfs_path_len], "op_download");
            break;
        default:
            break;
    }

    fd = open(fixed_sysfs_path, O_RDWR);
    SH_SYSFS("read (%s)", fixed_sysfs_path);

    if (fd >= 0) {
        err = read(fd, buff, len);
        close(fd);
        if (err < 0) {
            SH_ERR("write(%s) failed!\n", buff);
            return err;
        }
        memcpy(data, buff, len);
    } else {
        SH_ERR("open file fail(%s) %s!\n", buff, strerror(errno));
        return -EINVAL;
    }

    return err;
}

int SensorHubInputSensor::readDatafromDriver(int operate, char *data , int len) {
    switch (ctlMode) {
        case 0: /* IO Ctl */
            break;
        case 1: /* Sysfs */
            readDatafromDriverSysfs(operate, data , len);
            break;
        default:
            break;
    }

    return 0;
}

unsigned short SensorHubInputSensor::shub_data_checksum(unsigned char *data,
	 unsigned short out_len)
{
    unsigned int sum = 0;
    unsigned short nAdd;

    if ((NULL == data) || (0 == out_len))
        return 0;

    while (out_len > 2) {
        nAdd = *data++;
        nAdd <<= 8;
        nAdd += *data++;
        sum += nAdd;
        out_len -= 2;
    }
    if (out_len == 2) {
        nAdd = *data++;
        nAdd <<= 8;
        nAdd += *data++;
        sum += nAdd;
    } else {
        nAdd = *data++;
        nAdd <<= 8;
        sum += nAdd;
    }
/*The carry is 2 at most, so we need 2 additions at most. */
    sum = (sum & 0xFFFF) + (sum >> 16);
    sum = (sum & 0xFFFF) + (sum >> 16);

    return (unsigned short)(~sum);
}

int SensorHubInputSensor::sendCaliFirmware() {
    using namespace std;

    long start = 0, end = 0, count = 0;
    char *buffer, flag_buffer[16] = {0};
	int i, fd, mag_cali_fd = -1, rst, flag;
    unsigned short mag_crc_check = 0;

    cali_firmware_flag++;
    SH_LOG(" %s, cali_firmware_flag = %d\r\n", __func__, cali_firmware_flag);

    ifstream fin;
    fin.open("/system/vendor/firmware/EXEC_CALIBRATE_MAG_IMAGE", ios::binary);

    if (!fin.is_open()) {
        SH_LOG("the cali library can't open\r\n'");
        return -EINVAL;
    } else {
        SH_LOG("open cali library success\r\n");
    }

    fd = open("/proc/pmic/cali_lib", O_RDWR);
    if (fd < 0) {
        SH_LOG("the cali_lib note can't open\r\n'");
        fin.close();
        return -EINVAL;
    }

    mag_cali_fd = open("/sys/class/sprd_sensorhub/sensor_hub/mag_cali_flag", O_RDWR);
    if (mag_cali_fd < 0) {
        SH_LOG("the mag_cali_flag note can't open\r\n'");
        fin.close();
        close(fd);
        return -EINVAL;
    }

    fin.seekg(0, ios::beg);
    start = fin.tellg();
    fin.seekg(0, ios::end);
    end = fin.tellg();
    SH_LOG("the cali library size is %ld \r\n", end - start);

    fin.seekg(0, ios::beg);
    count = end - start;

    buffer = new char [count];
    if (buffer != NULL) {
        fin.read(buffer, count);
    } else {
        SH_LOG("can't alloc buffer for mag library'\r\n'");
        fin.close();
        close(fd);
        close(mag_cali_fd);
        return -EINVAL;
    }

    for (i = 0; i < 8; i++) {
        flag_buffer[i] = buffer[i];
    }
    for (i = 0; i < 8; i++) {
        flag_buffer[i + 8] = *(buffer + count - 8 + i);
    }

    mag_crc_check = shub_data_checksum((unsigned char *)flag_buffer, 16);
    SH_LOG("the mag cali check sum = 0x%x\r\n", mag_crc_check);

    if (fd >= 0) {
        SH_LOG("open cali_lib success\r\n");
        for (i = 0; i < 5; i++) {
            rst = write(fd, buffer, count);
            if (rst != count) {
                SH_ERR("write (%s) to (%s) failed!", buffer, "/proc/pmic/cali_lib");
                continue;
            } else {
                flag = 1;
                if (mag_cali_fd >= 0) {
                    SH_LOG("the mag_cali_flag open success\r\n");
                    sprintf(flag_buffer, "%d %ld %hu", flag, count, mag_crc_check);
                    write(mag_cali_fd, flag_buffer, 16);
                    SH_LOG("mag cali send success, i = %d,  write %ld bytes, the rst = %d \r\n", i, count, rst);
                }
                break;
            }
        }
    }

    delete [] buffer;
    fin.close();
    close(fd);
    close(mag_cali_fd);

    return 0;
}

int SensorHubInputSensor::setEnable(int32_t handle, int enabled) {
    int data[3];  // 1(handle)+1(enable)+1(pad)
    int err = 0;
    int flags = !!enabled;
    pSensorInfo_t ptr = getSensorInfo(handle);

    if (NULL == ptr)
        return -EINVAL;

    if (flags != ptr->enabled) {
        //  ptr->enabled = flags;
        data[0] = handle;
        data[1] = flags;
        data[2] = 0xEF;
        err = sendCmdtoDriver(efCmdsetEnable, data);
        if (err == 0) {  //  0 means success
            ptr->enabled = flags;
        }
    } else {
        SH_ERR("Sensor %s has been %s", ptr->pSensorList->name, flags ? "Open" : "Close");
    }

    return err;
}

int SensorHubInputSensor::getEnable(int32_t handle) {
    pSensorInfo_t ptr = getSensorInfo(handle);
    if (NULL == ptr)
        return -EINVAL;

    return ptr->enabled;
}

bool SensorHubInputSensor::hasPendingEvents() const {
    return !mHasPendingEvent.isEmpty();
}

int SensorHubInputSensor::setDelay(int32_t handle, int64_t delay_ns) {
    int err = 0;
    pSensorInfo_t ptr = getSensorInfo(handle);
    if (NULL == ptr)
        return -EINVAL;

    int data[4];  // 2(handle+type)+1(delay_ms)+1(pad)

    data[0] = handle;
    data[2] = 0xEF;

    if (ptr->period_ns != delay_ns) {
        ptr->period_ns = delay_ns;
        data[1] = delay_ns / 1000000;
        err = sendCmdtoDriver(efCmdsetDelay, data);
    }
    batch(handle, 0, delay_ns, 0);

    return err;
}

int SensorHubInputSensor::batch(int32_t handle, int32_t flags, int64_t period_ns, int64_t timeout) {
    int err = 0;
    int idx = 0;
    pSensorInfo_t ptr = getSensorInfo(handle);
    if (NULL == ptr)
        return -EINVAL;

    int data[6];  // 1(handle)+6(flags+period_ns+timeout)+1(pad)

    data[0] = handle;
    data[1] = flags;
    data[5] = 0xEF;

    data[2] = (int)(period_ns/NS);
    if (data[2] < 10) {
        data[2] = 10;
    }

    timeout /= NS;
    memcpy(&data[3], &timeout, sizeof(int64_t));

    ptr->period_ns = period_ns;
    ptr->timeout = timeout;
    err = sendCmdtoDriver(efCmdbatch, data);

    return err;
}

int SensorHubInputSensor::flush(int32_t handle) {
    int err = 0;
    pSensorInfo_t ptr = getSensorInfo(handle);

    if (NULL == ptr)
        return -EINVAL;

    int data[2];  // 1(handle)+1(pad)

    data[0] = handle;
    data[1] = 0xEF;

    err = sendCmdtoDriver(efCmdflush, data);

    return err;
}

sensors_event_t * SensorHubInputSensor::processEventInput(pseudo_event const* incomingEvent) {
    struct input_event *pInputEvent = (struct input_event *)incomingEvent;
/*
 * find index of mSensorInfo[] begin
 * */
    pSensorInfo_t ptr = NULL;

    switch (pInputEvent->code) {
        case ABS_X:
        case ABS_Y:
        case ABS_Z:
            ptr = getSensorInfo(SENSOR_HANDLE_ACCELEROMETER);
            break;

        case ABS_MISC:
            ptr = getSensorInfo(SENSOR_HANDLE_LIGHT);
            break;

        case ABS_DISTANCE:
            ptr = getSensorInfo(SENSOR_HANDLE_PROXIMITY);
            break;
    }
/*
 * find index of mSensorInfo[] end
 * */
    switch (pInputEvent->code) {
#if 0
        case ABS_X: /* Sensor Acc */
            ptr->mEvents.acceleration.x = ACC_UNIT_CONVERSION(pInputEvent->value);
            break;

        case ABS_Y: /* Sensor Acc */
            ptr->mEvents.acceleration.y = ACC_UNIT_CONVERSION(pInputEvent->value);
            break;

        case ABS_Z: /* Sensor Acc */
            ptr->mEvents.acceleration.z = ACC_UNIT_CONVERSION(pInputEvent->value);
            break;
#else
        case ABS_X:
        case ABS_Y:
        case ABS_Z:
            ptr->mEvents.data[pInputEvent->code - ABS_X] = ACC_UNIT_CONVERSION(pInputEvent->value);
            break;
#endif
        case ABS_MISC: /* Sensor Light */
            ptr->mEvents.light = (float)pInputEvent->value;
            break;

        case ABS_DISTANCE: /* Sensor PROXIMITY */
            ptr->mEvents.distance = pInputEvent->value;
            break;

        default:
            SH_ERR("unknown input event code %d", pInputEvent->code);
            break;
    }

    return &ptr->mEvents;
}

sensors_event_t * SensorHubInputSensor::processEventIIO(pseudo_event const* incomingEvent) {
    pmSenMsgTpA_t ptr = (pmSenMsgTpA_t)incomingEvent;

    SH_FUNC("Cmd type = %d", ptr->Cmd);
    switch (ptr->Cmd) {
        case eHalCmd:
            SH_DBG("eHalCmd");
            break;
        case eHalLogCtl:
            SH_LOG("eHalLogCtl");
            mDynamicLogel->dynamicLogerCtl((pmSenMsgLogCtl_t)incomingEvent);
            break;
        case eHalSenData:
            SH_DATA("eHalSenData");
            if (ptr->Length == sizeof(mSenMsgTpB_t)) {
                return processTypeBEvent(ptr->HandleID, (pmSenMsgTpB_t)incomingEvent);
            } else {
                return processTypeAEvent(ptr->HandleID, (pmSenMsgTpA_t)ptr);
            }
            break;
        case eHalFlush:
            SH_DBG("eHalFlush");
            return processFlushEvent(ptr->HandleID, (pmSenMsgTpA_t)ptr);
        case eMcuReady:
            SH_DBG("eMcuReady");
            return NULL;
        case eCommonDriverReady:
            SH_DBG("eCommonDriverReady");
            return processHubInit(ptr->HandleID, (pmSenMsgTpA_t)ptr);
        default:
            SH_ERR("default, unknown type %d", ptr->Cmd);
            break;
    }

    return NULL;
}

void SensorHubInputSensor::checkOneShotMode(int handle, pSensorInfo_t pinfo)
{
    if((pinfo->pSensorList->flags & SENSOR_FLAG_ONE_SHOT_MODE)
        && ((pinfo->pSensorList->flags & SENSOR_FLAG_ON_CHANGE_MODE) == 0))
    {
        // If sensors is one shot mode, When sensors Trigger event, We need disable sensor to avoid sensors repot another event
        setEnable(handle, 0);
    }
}

sensors_event_t * SensorHubInputSensor::processTypeAEvent(int handle, pmSenMsgTpA_t data) {
    pSensorInfo_t ptr = getSensorInfo(handle);
    // SH_DATA("handle = %d", handle);

    if (ptr == NULL || ptr->enabled == 0)
        return NULL;

    /*convert Mcu Time To Ap Time*/
    ptr->mEvents.timestamp = convertTimeToAp(data->timestamp);

    switch (handle) {
        case SENSOR_HANDLE_STEP_COUNTER:
        case SENSOR_HANDLE_WAKE_UP_STEP_COUNTER:
            ptr->mEvents.u64.step_counter = data->pedometer;
            break;
        case SENSOR_HANDLE_HEART_RATE:
            ptr->mEvents.heart_rate.bpm = data->fdata[0];
            ptr->mEvents.heart_rate.status = data->status;
            break;
#ifdef WORKROUND_FACEUPDOWN
        case SENSOR_HANDLE_FACE_UP_DOWN:
            SH_DATA("FaceUpDown x:%f y:%f z:%f", data->fdata[0], data->fdata[1], data->fdata[2]);
            ptr->mEvents.type = SENSOR_TYPE_SPRD_FACE_UP_DOWN;
            ptr->mEvents.sensor = SENSOR_HANDLE_FACE_UP_DOWN;
            ptr->mEvents.data[0] = data->fdata[0];
            SH_LOG("FaceUpDown sensor:%s, value:%f, event:%d", ptr->pSensorList->name, ptr->mEvents.data[0], (handle - SENSOR_HANDLE_FACE_UP_DOWN));
            break;
        case SENSOR_HANDLE_WAKE_UP_FACE_UP_DOWN:
            SH_DATA("FaceUpDownWakeUp x:%f y:%f z:%f", data->fdata[0], data->fdata[1], data->fdata[2]);
            ptr->mEvents.type = SENSOR_TYPE_SPRD_FACE_UP_DOWN;
            ptr->mEvents.sensor = SENSOR_HANDLE_WAKE_UP_FACE_UP_DOWN;
            ptr->mEvents.data[0] = data->fdata[0];
            SH_LOG("FaceUpDownWakeUp sensor:%s, value:%f, event:%d", ptr->pSensorList->name, ptr->mEvents.data[0], (handle - SENSOR_HANDLE_WAKE_UP_FACE_UP_DOWN));
            break;
#endif /* WORKROUND_FACEUPDOWN */
#ifdef CHANGE_TO_OLD_FORMAT
	case SENSOR_HANDLE_SHAKE:
	case SENSOR_HANDLE_WAKE_UP_SHAKE:
	    SH_DATA("Shake x:%f y:%f z:%f", data->fdata[0], data->fdata[1], data->fdata[2]);
	    ptr->mEvents.data[0] = data->fdata[2];
	    ptr->mEvents.data[1] = 1;
	    ptr->mEvents.data[2] = data->fdata[0];
	    SH_LOG("Shake sensor:%s, value:%f, event:%d", ptr->pSensorList->name, ptr->mEvents.data[0], (handle - SENSOR_HANDLE_SHAKE));
	    break;
	case SENSOR_HANDLE_TAP:
	case SENSOR_HANDLE_WAKE_UP_TAP:
	    SH_DATA("Tap x:%f y:%f z:%f", data->fdata[0], data->fdata[1], data->fdata[2]);
	    ptr->mEvents.data[0] = 2;
	    ptr->mEvents.data[1] = data->fdata[2];
	    ptr->mEvents.data[2] = data->fdata[1];
	    SH_LOG("Tap sensor:%s, value:%f, event:%d", ptr->pSensorList->name, ptr->mEvents.data[0], (handle - SENSOR_HANDLE_TAP));
	    break;
	case SENSOR_HANDLE_FLIP:
	case SENSOR_HANDLE_WAKE_UP_FLIP:
	    SH_DATA("Filp x:%f y:%f z:%f", data->fdata[0], data->fdata[1], data->fdata[2]);
	    ptr->mEvents.data[0] = data->fdata[1];
	    ptr->mEvents.data[1] = 1;
	    ptr->mEvents.data[2] = data->fdata[0];
	    SH_LOG("Filp sensor:%s, value:%f, event:%d", ptr->pSensorList->name, ptr->mEvents.data[0], (handle - SENSOR_HANDLE_FLIP));
	    break;
	case SENSOR_HANDLE_TWIST:
	case SENSOR_HANDLE_WAKE_UP_TWIST:
	    SH_DATA("Twist x:%f y:%f z:%f", data->fdata[0], data->fdata[1], data->fdata[2]);
	    ptr->mEvents.data[0] = data->fdata[1];
	    ptr->mEvents.data[1] = data->fdata[0];
	    SH_LOG("Twist sensor:%s, value:%f, event:%d", ptr->pSensorList->name, ptr->mEvents.data[0], (handle - SENSOR_HANDLE_TWIST));
	    break;
	case SENSOR_HANDLE_POCKET_MODE:
	case SENSOR_HANDLE_WAKE_UP_POCKET_MODE:
	    SH_DATA("Pocket mode x:%f y:%f z:%f", data->fdata[0], data->fdata[1], data->fdata[2]);
	    ptr->mEvents.data[0] = data->fdata[0];
		ptr->mEvents.data[1] = 0;
	    SH_LOG("Pocket mode sensor:%s, value:%f, event:%d", ptr->pSensorList->name, ptr->mEvents.data[0], (handle - SENSOR_HANDLE_POCKET_MODE));
	    break;
#endif
        case SENSOR_HANDLE_PROXIMITY:
        case SENSOR_HANDLE_WAKE_UP_PROXIMITY:
            SH_LOG("Proximity sensor x:%f y:%f z:%f", data->fdata[0], data->fdata[1], data->fdata[2]);
            ptr->mEvents.orientation.status = data->status;
            memcpy(ptr->mEvents.data, data->fdata, sizeof(float) * 3);
            break;
        case SENSOR_HANDLE_ROTATION_VECTOR:
            SH_LOG("Handdle_rotation_vector  x:%f y:%f z:%f", data->fdata[0], data->fdata[1], data->fdata[2]);
            memcpy(ptr->mEvents.data, data->fdata, sizeof(float) * 3);
            ptr->mEvents.data[3] =
                1 - data->fdata[0] * data->fdata[0]
                - data->fdata[1] * data->fdata[1]
                - data->fdata[2] * data->fdata[2];
            ptr->mEvents.data[3] = (ptr->mEvents.data[3] > 0) ? (float)sqrt(ptr->mEvents.data[3]) : 0;
            break;
        case SENSOR_HANDLE_GAME_ROTATION_VECTOR:
            SH_LOG("Game_rotation_vector  x:%f y:%f z:%f", data->fdata[0], data->fdata[1], data->fdata[2]);
            memcpy(ptr->mEvents.data, data->fdata, sizeof(float) * 3);
            ptr->mEvents.data[3] =
                1 - data->fdata[0] * data->fdata[0]
                - data->fdata[1] * data->fdata[1]
                - data->fdata[2] * data->fdata[2];
            ptr->mEvents.data[3] = (ptr->mEvents.data[3] > 0) ? (float)sqrt(ptr->mEvents.data[3]) : 0;
            break;
        case SENSOR_HANDLE_GEOMAGNETIC_ROTATION_VECTOR:
            SH_LOG("Geomagnetic_rotation_vector  x:%f y:%f z:%f", data->fdata[0], data->fdata[1], data->fdata[2]);
            memcpy(ptr->mEvents.data, data->fdata, sizeof(float) * 3);
            ptr->mEvents.data[3] =
                1- data->fdata[0] * data->fdata[0]
                -data->fdata[1] * data->fdata[1]
                - data->fdata[2] * data->fdata[2];
            ptr->mEvents.data[3] = (ptr->mEvents.data[3] > 0) ? (float)sqrt(ptr->mEvents.data[3]) : 0;
            break;
        case MCU_REINITIAL:

            return NULL;
            break;

        default:
            ptr->mEvents.orientation.status = data->status;
            memcpy(ptr->mEvents.data, data->fdata, sizeof(float) * 3);
            break;
    }
    checkOneShotMode(handle, ptr);

    ALOGD_IF((ptr->pSensorList->flags >> 1) == (SENSOR_FLAG_ONE_SHOT_MODE >> 1), "%s triggle once", ptr->pSensorList->name);
    mDynamicLogel->dynamicLogerDumpData(!!(ptr->pSensorList->flags >> 1), "%s:\n\tdata: %f, %f, %f \ttimestamp:%lld\n",
            ptr->pSensorList->name,
            data->fdata[0],
            data->fdata[1],
            data->fdata[2],
            ptr->mEvents.timestamp);

    return &ptr->mEvents;
}

sensors_event_t * SensorHubInputSensor::processTypeBEvent(int handle, pmSenMsgTpB_t data) {
    pSensorInfo_t ptr = getSensorInfo(handle);
    // SH_DATA("handle = %d", handle);

    if (ptr == NULL || ptr->enabled == 0)
        return NULL;

    /*convert Mcu Time To Ap Time*/
    ptr->mEvents.timestamp = convertTimeToAp(data->timestamp);

    switch (handle) {
        default:
            memcpy(ptr->mEvents.data, data->fdata, sizeof(float) * 6);
            break;
    }
    checkOneShotMode(handle, ptr);

    ALOGD_IF((ptr->pSensorList->flags >> 1) == (SENSOR_FLAG_ONE_SHOT_MODE >> 1), "%s triggle once", ptr->pSensorList->name);
    mDynamicLogel->dynamicLogerDumpData(!!(ptr->pSensorList->flags >> 1), "%s:\n\tdata: %f, %f, %f, %f, %f, %f \ttimestamp:%lld\n",
            ptr->pSensorList->name,
            data->fdata[0],
            data->fdata[1],
            data->fdata[2],
            data->fdata[3],
            data->fdata[4],
            data->fdata[5],
            ptr->mEvents.timestamp);

    return &ptr->mEvents;
}

sensors_event_t * SensorHubInputSensor::processFlushEvent(int handle, pmSenMsgTpA_t data) {
    uint32_t flag = 0;
    pSensorInfo_t ptr = getSensorInfo(handle);

    SH_DATA("handle = %d, data handle = %d", handle, data->HandleID);

    if (ptr == NULL)
        return NULL;

#if 0
    if (ptr->pSensorList != NULL) {
        if (ptr->pSensorList->flags & SENSOR_FLAG_ONE_SHOT_MODE) {
            return NULL;
        }
    }
#endif

    mSensorInfo[mTotalNumber].enabled = 1;
    mSensorInfo[mTotalNumber].mEvents.meta_data.sensor = handle;
    mSensorInfo[mTotalNumber].mEvents.meta_data.what   = META_DATA_FLUSH_COMPLETE;
    mSensorInfo[mTotalNumber].mEvents.timestamp = getTimestamp();

    return &mSensorInfo[mTotalNumber].mEvents;
}

sensors_event_t * SensorHubInputSensor::processHubInit(int handle, pmSenMsgTpA_t data) {
    int idx = 0;

    SH_FUNC("In:%d, %d\n", handle, data->type);

    /*Initial Hub*/
    for (idx = 0; idx < mTotalNumber; idx++) {
        if (mSensorInfo[idx].enabled) {
            batch(mSensorInfo[idx].pSensorList->handle, 0, mSensorInfo[idx].period_ns, mSensorInfo[idx].timeout);
            setEnable(mSensorInfo[idx].pSensorList->handle, 0);
            setEnable(mSensorInfo[idx].pSensorList->handle, 1);
            flush(mSensorInfo[idx].pSensorList->handle);
        }
    }

    SH_FUNC("Out\n");
    return NULL;
}

sensors_event_t * SensorHubInputSensor::processEvent(pseudo_event const* incomingEvent) {
    sensors_event_t * pSEvent = NULL;

    switch (drvMode) {
        case 0: /* input */
            pSEvent = processEventInput(incomingEvent);
            break;
        case 1: /* IIO */
            pSEvent = processEventIIO(incomingEvent);
            break;
        default:
            break;
    }

    return pSEvent;
}

int SensorHubInputSensor::readEvents(sensors_event_t* data, int count) {
    sensors_event_t * pSensorEvent = NULL;
    SH_DATA("count = %d, dataFd = %d", count, dataFd);
    if (count < 1)
        return -EINVAL;
//    if (mEnabled.isEmpty()) {
//        return 0;
//    }

    ssize_t n = mInputReader->fill(dataFd);
    if (n < 0)
        return n;

    int numEventReceived = 0;
    pseudo_event const* event;

again:
    while (count && mInputReader->readEvent(&event)) {
        switch (drvMode) {
            case 0:
                {
                    struct input_event *pInputEvent = (struct input_event *)event;

                    if ((pInputEvent->type == EV_REL) || (pInputEvent->type == EV_ABS)) {
                        pSensorEvent = processEventInput(event);
                    } else if (pInputEvent->type == EV_SYN) {
                        /* Upper 32-bits of the timestamp comes in the EV_SYN value */
                        if (pSensorEvent != NULL) {
                            pSensorEvent->timestamp = timevalToNano(pInputEvent->time);
                            *data++ = *pSensorEvent;
                            count--;
                            numEventReceived++;
                        }
                    } else {
                        SH_ERR("unknown event (type=%d, code=%d)", pInputEvent->type, pInputEvent->code);
                    }
                }
                break;
            case 1:
                pSensorEvent = processEvent(event);
                if (pSensorEvent != NULL) {
                    *data++ = *pSensorEvent;
                    count--;
                    numEventReceived++;
                }
                break;
            default:
                break;
        }
        mInputReader->next();
    }

    /* if we didn't read a complete event, see if we can fill and
       try again instead of returning with nothing and redoing poll. */
    if (numEventReceived == 0 && mEnabled.isEmpty() == 0) {
        n = mInputReader->fill(dataFd);
        if (n)
            goto again;
    }

    return numEventReceived;
}
