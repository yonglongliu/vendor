/******************************************************************************
 *
 *  Copyright (C) 2018 Spreadtrum Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

#undef LOG_TAG
#define LOG_TAG "bt_cus"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <cutils/properties.h>
#include <utils/Log.h>

#include "bt_hal.h"
#include "bt_cus.h"
#ifdef OSI_COMPAT_ANROID_6_0
#elif OSI_COMPAT_ANROID_4_4_4
#else
#include "raw_address.h"
#endif


#define BTD(param, ...) ALOGD("%s "param, __FUNCTION__, ## __VA_ARGS__)
#define BTE(param, ...) ALOGE("%s "param, __FUNCTION__, ## __VA_ARGS__)


#define RESPONSE_DUMP(param) ALOGD("%s, response dump: %s\n", __FUNCTION__, param);

#define CASE_RETURN_STR(const) case const: return #const;


void get_bt_status_parse(char *buf, char *rsp) {
    int ret;
    bt_bdaddr_t addr;
    const bt_test_kit_t *bt_kit = bt_test_kit_get_interface();

    ret = bt_kit->enable(NULL);
    if (ret < 0) {
        BTE("%s enable bt faild", __func__);
        goto faild;
    }

    ret = bt_kit->read_local_address(&addr);
    if (ret < 0) {
        BTE("%s read local address faild", __func__);
        goto faild;
    }

    ret = bt_kit->disable();
    if (ret < 0) {
        BTE("%s disable bt faild", __func__);
        goto faild;
    }

    snprintf(rsp, 128, "bt HWaddr: %02x:%02x:%02x:%02x:%02x:%02x\r\n", addr.address[0], addr.address[1],
               addr.address[2], addr.address[3], addr.address[4], addr.address[5]);
    return;


faild:;
    snprintf(rsp, 128, "bt: error fetching interface information: Device not found\r\n");
    return;
}


