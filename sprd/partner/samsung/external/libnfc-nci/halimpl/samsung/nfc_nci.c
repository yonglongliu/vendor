/*
 *    Copyright (C) 2013 SAMSUNG S.LSI
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at:
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 *   Author: Woonki Lee <woonki84.lee@samsung.com>
 *
 */

#define LOG_TAG "NfcNciHal"
#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <hardware/hardware.h>
#include <hardware/nfc.h>
#include <cutils/log.h>
#include <stdlib.h>

#include "hal.h"

/* Close an opened nfc device instance */
static int nfc_close (hw_device_t *dev)
{
    int ret = 0;
    free (dev);

    nfc_hal_deinit();

    return ret;
}

static int nfc_open (const hw_module_t* module, const char* name, hw_device_t** device)
{
    ALOGD ("%s: enter; name=%s", __FUNCTION__, name);
    int ret = 0; //0 is ok; -1 is error

    if (strcmp (name, NFC_NCI_CONTROLLER) == 0)
    {
        struct nfc_nci_device *dev = calloc (1, sizeof(struct nfc_nci_device));

        // Common hw_device_t fields
        dev->common.tag = HARDWARE_DEVICE_TAG;
        dev->common.version = 0x00010000; // [31:16] major, [15:0] minor
        dev->common.module = (struct hw_module_t*) module;
        dev->common.close = nfc_close;

        // NCI HAL method pointers
        dev->open               = nfc_hal_open;
        dev->write              = nfc_hal_write;
        dev->core_initialized   = nfc_hal_core_initialized;
        dev->pre_discover       = nfc_hal_pre_discover;
        dev->close              = nfc_hal_close;
        dev->control_granted    = nfc_hal_control_granted;
        dev->power_cycle        = nfc_hal_power_cycle;

        // Copy in
        *device = (hw_device_t*) dev;

        /* Initialize HAL */
        ret = nfc_hal_init();
    }
    else
    {
        ret = -EINVAL;
    }
    ALOGD ("%s: exit %d", __FUNCTION__, ret);
    return ret;
}


static struct hw_module_methods_t nfc_module_methods =
{
    .open = nfc_open,
};


struct nfc_nci_module_t HAL_MODULE_INFO_SYM =
{
    .common =
    {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = 0x0100, // [15:8] major, [7:0] minor (1.0)
        .hal_api_version = 0x00, // 0 is only valid value
        .id = NFC_NCI_HARDWARE_MODULE_ID,
        .name = "Default NFC NCI HW HAL",
        .author = "The Android Open Source Project",
        .methods = &nfc_module_methods,
    },
};
