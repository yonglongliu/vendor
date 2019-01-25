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

#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <utils/Log.h>

#include "eng_diag.h"
#include "eng_modules.h"
#include "engopt.h"

#include "bt_npi.h"
#include "bt_cus.h"
#include "bt_eut.h"

#undef LOG_TAG
#define LOG_TAG "bt_eut"


#define BTD(param, ...) {ALOGD(param, ## __VA_ARGS__);}

static bt_eut_action_t  bt_eut_action[] = {


    {NULL, NULL, NULL},
};

static int bt_npi_cmd_callback(char *diag_cmd, char *at_rsp)
{
	char *p = diag_cmd + sizeof(MSG_HEAD_T) + 1;
    unsigned int length = strlen(p);
    static char buf[1024];

    if (length > sizeof(buf)) {
        BTD("out of memory");
        return -1;
    }

    memcpy(buf, p, length);
    buf[length - 1] = 0;
    BTD("[%s], len: %d", buf, strlen(buf));
    bt_npi_parse(EUT_BT_MODULE_INDEX, buf, at_rsp);
	return 0;
}


static int ble_npi_cmd_callback(char *diag_cmd, char *at_rsp)
{
	char *p = diag_cmd + sizeof(MSG_HEAD_T) + 1;
    unsigned int length = strlen(p);
    static char buf[1024];

    if (length > sizeof(buf)) {
        BTD("out of memory");
        return -1;
    }

    memcpy(buf, p, length);
    buf[length - 1] = 0;
    BTD("[%s], len: %d", buf, strlen(buf));
    bt_npi_parse(EUT_BLE_MODULE_INDEX, buf, at_rsp);
	return 0;
}

static int bt_cus_getbtstatus_callback(char *diag_cmd, char *at_rsp)
{
	char *p = diag_cmd + sizeof(MSG_HEAD_T) + 1;
    unsigned int length = strlen(p);
    static char buf[1024];

    if (length > sizeof(buf)) {
        BTD("out of memory");
        return -1;
    }

    memcpy(buf, p, length);
    buf[length - 1] = 0;
    BTD("[%s], len: %d", buf, strlen(buf));
    get_bt_status_parse(buf, at_rsp);
    return 0;
}


void register_this_module_ext(struct eng_callback *reg, int *num)
{
    int moudles_num = 0;
    BTD("%s : for BT EUT", __func__);

    sprintf((reg + moudles_num)->at_cmd, "%s", BT_NPI_ATKEY);
    (reg + moudles_num)->eng_linuxcmd_func = bt_npi_cmd_callback;
    moudles_num++;

    sprintf((reg + moudles_num)->at_cmd, "%s", BLE_NPI_ATKEY);
    (reg + moudles_num)->eng_linuxcmd_func = ble_npi_cmd_callback;
    moudles_num++;


    sprintf((reg + moudles_num)->at_cmd, "%s", BT_CUS_GETBTSTATUS_ATKEY);
    (reg + moudles_num)->eng_linuxcmd_func = bt_cus_getbtstatus_callback;
    moudles_num++;


    *num = moudles_num;
    BTD("%s %d register complete", __func__, moudles_num);
}
