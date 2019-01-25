/******************************************************************************
 *
 *  Copyright (C) 2014 Google, Inc.
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

#include <assert.h>
#include <cutils/properties.h>
#include <string.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>

#include "bt_types.h"

#include "vendor_h4.h"
#include "vendor_hci.h"
#include "vendor_hcif.h"
#include "vendor_utils.h"
#include "vendor_hcicmds.h"
#include "vendor_test.h"
#include "vendor_suite.h"
#include "hcidefs.h"

#include "buffer_allocator.h"


#include "osi/include/eager_reader.h"
#include "osi/include/osi.h"
#include "osi/include/log.h"
#include "osi/include/reactor.h"
#include "osi/include/thread.h"
#include "osi/include/future.h"
#include "osi/include/list.h"
#include "osi/include/fixed_queue.h"
#include "osi/include/semaphore.h"

static semaphore_t *address_sem;
static BD_ADDR local_addr;

static int enable(void)
{
    BTD("%s", __func__);
    address_sem = semaphore_new(0);

    hcif_start_up();
    hci_start_up();

    return 0;
}

static void disable(void)
{
    BTD("%s", __func__);

    hci_shut_down();
    hcif_shut_down();

    semaphore_free(address_sem);
}

static void hci_cmd_send(uint16_t opcode, size_t len, uint8_t* buf, hci_cmd_complete_cb *p)
{
    BTD("%s", __func__);
    BTM_SendHciCommand(opcode, len & 0xFF, buf, p);
}

static int dut_mode_enable(void)
{
    return bt_test_mode_enable();
}

static void dut_mode_disable(void)
{
    bt_test_mode_disable();
}

void read_local_address_cb(BD_ADDR address) {
    BTD("%s", __func__);
    memcpy(local_addr, address, sizeof(BD_ADDR));
    semaphore_post(address_sem);
}

static int read_local_address(BD_ADDR address) {
    btsnd_hcic_read_bd_addr();
    semaphore_wait(address_sem);
    memcpy(address, local_addr, sizeof(BD_ADDR));
    return 0;
}

const bt_vendor_suite_interface_t BT_VENDOR_SUITE_INTERFACE = {
    sizeof(bt_vendor_suite_interface_t),
    enable,
    disable,
    hci_cmd_send,
    dut_mode_enable,
    dut_mode_disable,
    read_local_address,
};
