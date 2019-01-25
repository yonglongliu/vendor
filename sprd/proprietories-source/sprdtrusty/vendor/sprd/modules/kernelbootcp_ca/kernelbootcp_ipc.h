/*
 * Copyright (C) 2012 The Android Open Source Project
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

#pragma once

#define KERNELBOOTCP_PORT "com.android.trusty.kernelbootcp"
#define KERNELBOOTCP_MAX_BUFFER_LENGTH 4096

// Commands
enum secureboot_command {
    KERNELBOOTCP_BIT                = 1,
    KERNELBOOTCP_REQ_SHIFT          = 1,

    KERNEL_BOOTCP_VERIFY_ALL        = (0 << KERNELBOOTCP_REQ_SHIFT),
    KERNEL_BOOTCP_UNLOCK_DDR        = (1 << KERNELBOOTCP_REQ_SHIFT),
};

#ifdef __ANDROID__

/**
 * kernelbootcp_message - Serial header for communicating with KBC server
 * @cmd: the command, one of kernelbootcp_command.
 * @payload: start of the serialized command specific payload
 */
struct kernelbootcp_message {
    uint32_t cmd;
    uint8_t  payload[0];
};

#endif
