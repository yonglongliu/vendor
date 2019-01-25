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

#ifndef SENSORHUBINPUTSENSOR_H_
#define SENSORHUBINPUTSENSOR_H_

#include <stdint.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <utils/BitSet.h>

#include "sensors.h"
#include "SensorBase.h"
#include "InputEventReader.h"
#include "SensorHubDynamicLogel.h"


/*
 * define struct begin
 * */
typedef enum {
    efCmdsetEnable = 0,
    efCmdgetEnable,
    efCmdsetDelay,
    efCmdbatch,
    efCmdflush,
    efCmdUpdateHubStatus,
    efCmdSendOpCode
}func_cmd;

#define NS         (int64_t)1000000ll
#define US         1000ll
#define MAX_STRING_SIZE         255

typedef struct {
    sensors_event_t mEvents;
    int enabled;
    int flush;
    int64_t period_ns;
    int64_t timeout;
    const struct sensor_t *pSensorList;
}SensorInfo_t, *pSensorInfo_t;
/*
 * define struct end
 * */

/*
 * instantiate one these objects for every SensorHub provided
 * sensor you're interested in
 */
class SensorHubInputSensor : public SensorBase {
 public:
        explicit SensorHubInputSensor(const shub_drv_cfg_t & drvCfg);
        virtual ~SensorHubInputSensor();

        virtual int readEvents(sensors_event_t* data, int count);
        virtual bool hasPendingEvents() const;
        virtual int setDelay(int32_t handle, int64_t ns);
        virtual int setEnable(int32_t handle, int enabled);
        virtual int getEnable(int32_t handle);
        virtual int batch(int handle, int flags, int64_t period_ns, int64_t timeout);
        virtual int flush(int handle);

 protected:
        sensors_event_t * processEventInput(pseudo_event const* incomingEvent);
        sensors_event_t * processEventIIO(pseudo_event const* incomingEvent);
        sensors_event_t * processEvent(pseudo_event const* incomingEvent);
        sensors_event_t * processTypeAEvent(int handle, pmSenMsgTpA_t data);
        sensors_event_t * processTypeBEvent(int handle, pmSenMsgTpB_t data);
        sensors_event_t * processFlushEvent(int handle, pmSenMsgTpA_t data);
        sensors_event_t * processHubInit(int handle, pmSenMsgTpA_t data);
        pSensorInfo_t getSensorInfo(int32_t handle);

        int sendCmdtoDriverIoctl(int operate, int* para);
        int sendCmdtoDriverSysfs(int operate, int* para);
        int sendCmdtoDriver(int operate, int* para);
        int readDatafromDriverSysfs(int operate, char *data , int len);
        int readDatafromDriver(int operate, char *data , int len);
        int sendCaliFirmware();
        unsigned short shub_data_checksum(unsigned char *data, unsigned short out_len);
 private:
        SensorHubInputSensor();
        SensorHubInputSensor(const SensorHubInputSensor&);
        void operator=(const SensorHubInputSensor&);
        void checkOneShotMode(int handle, pSensorInfo_t pinfo);
        int setInitialState();
        int downloadFirmware(void);
        int converHandleToType(int handle, int *data);
        int64_t convertTimeToAp(int64_t time);

        InputEventCircularReader *mInputReader;
        SensorHubDynamicLogel * mDynamicLogel;

        sensor_t *mSensorList;
        int mTotalNumber;
        pSensorInfo_t mSensorInfo;
        android::BitSet64 mHasPendingEvent;
        android::BitSet64 mEnabled;
        char input_sysfs_path[PATH_MAX];
        int input_sysfs_path_len;
        static int cali_firmware_flag;
};

/*****************************************************************************/
#endif  // SENSORHUBINPUTSENSOR_H_
