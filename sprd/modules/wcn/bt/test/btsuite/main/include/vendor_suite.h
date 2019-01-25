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

#pragma once

#include <stdbool.h>
#include "bt_types.h"
#include "hcidefs.h"

typedef struct {
    uint16_t opcode;
    uint16_t param_len;
    uint8_t* p_param_buf;
} hci_cmd_complete_t;

typedef void(hci_cmd_complete_cb)(hci_cmd_complete_t *p);

typedef struct {
    size_t size;
    int (*enable)(void);
    void (*disable)(void);
    void (*hci_cmd_send)(uint16_t opcode, size_t len, uint8_t* buf, hci_cmd_complete_cb *p);
    int (*dut_mode_enable)(void);
    void (*dut_mode_disable)(void);
    int (*read_local_address)(BD_ADDR address);
} bt_vendor_suite_interface_t;

extern const bt_vendor_suite_interface_t BT_VENDOR_SUITE_INTERFACE;
