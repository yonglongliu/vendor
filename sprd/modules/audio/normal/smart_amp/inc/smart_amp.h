/*
* Copyright (C) 2010 The Android Open Source Project
* Copyright (C) 2012-2015, The Linux Foundation. All rights reserved.
*
* Not a Contribution, Apache license notifications and license are retained
* for attribution purposes only.
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

#ifndef __SMART_AMP_H__
#define __SMART_AMP_H__


#ifdef FLAG_VENDOR_ETC
#define SMART_INIT_BIN_FILE           "vendor/etc/smart_amp_init.bin"
#else
#define SMART_INIT_BIN_FILE           "system/etc/smart_amp_init.bin"
#endif

#define BATTERY_VOLTAGE_PATH          "/sys/class/power_supply/battery/voltage_now"
#define SMART_AMP_LOOP_CNT            60

typedef struct smart_amp_ctl SMART_AMP_CTL;

typedef struct {
    int channel;
    int format;
    int flagDump;
    struct mixer *mixer;
}SMART_AMP_STREAM_INFO;

typedef enum {
    HANDSFREE = 0x1,
    HEADFREE  = 0x2,
}ISC_MODE;

typedef enum {
    BOOST_BYPASS,
    BOOST_5V,
    BOOST_MODE_INVALID
}BOOST_MODE;

SMART_AMP_CTL *init_smart_amp(char *amp_para_path, SMART_AMP_STREAM_INFO *pInfo);
int deinit_smart_amp(SMART_AMP_CTL *pCtl);
int smart_amp_process(void *pInBuf,  int size, void **pOutBuf, int *pOutSize, SMART_AMP_CTL *pCtl);
int smart_amp_get_battery_voltage(long int *pBatteryVoltage, char *pFileNodePath);
int smart_amp_set_battery_voltage(const char *kvpairs);
int select_smart_amp_config(SMART_AMP_CTL *pCtl, ISC_MODE mode);
int smart_amp_get_battery_delta(int *pDelta);
int smart_amp_set_battery_voltage_uv(SMART_AMP_CTL *pCtl, int batteryVoltage);
int smart_amp_set_boost(SMART_AMP_CTL *pCtl, BOOST_MODE mode);
void smart_amp_dump(int fd, SMART_AMP_CTL *pCtl);
#endif //__SMART_AMP_H__