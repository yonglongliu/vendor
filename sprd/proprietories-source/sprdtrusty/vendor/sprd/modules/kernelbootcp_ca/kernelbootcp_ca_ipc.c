/*
 * Copyright (C) 2015 The Android Open Source Project
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
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define LOG_TAG "TrustyKernelBootcp"
#include <cutils/log.h>

#include <trusty/tipc.h>

#include "kernelbootcp_ca_ipc.h"
#include "kernelbootcp_ipc.h"

#define TRUSTY_DEVICE_NAME "/dev/trusty-ipc-dev0"

static int handle_ = 0;

int trusty_kernelbootcp_connect()
{
    ALOGD("%s enter\n", __func__);

    int rc = tipc_connect(TRUSTY_DEVICE_NAME, KERNELBOOTCP_PORT);
    if (rc < 0) {
        ALOGE("tipc_connect() failed! \n");
        return rc;
    }
    handle_ = rc;
    ALOGD("handle = %d \n", handle_);
    return 0;
}

int trusty_kernelbootcp_call(uint32_t   cmd,
                             void      *in,
                             uint32_t   in_size,
                             uint8_t   *out,
                             uint32_t  *out_size)
{
    size_t    msg_size = 0;
    ssize_t   rc = 0;
    struct kernelbootcp_message *msg = NULL;

    ALOGD("%s enter in_size = %d out_size = %d\n",
          __func__, in_size, *out_size);
    if (handle_ == 0) {
        ALOGE("not connected\n");
        return -EINVAL;
    }
    msg_size = in_size + sizeof(struct kernelbootcp_message);
    msg = malloc(msg_size);
    msg->cmd = cmd;
    if (in_size > 0) {
        memcpy(msg->payload, in, in_size);
    }
    ALOGD("handle = %d msg_size = %d \n", handle_, msg_size);
    rc = write(handle_, msg, msg_size);
    free(msg);
    ALOGD("write rc = %d \n", rc);
    if (rc < 0) {
        ALOGE("failed to send cmd (%d) to %s: %s\n", cmd,
                KERNELBOOTCP_PORT, strerror(errno));
        return -errno;
    }
    rc = read(handle_, out, *out_size);
    ALOGD("read rc = %d \n", rc);
    if (rc < 0) {
        ALOGE("failed to retrieve response for cmd (%d) to %s: %s\n",
                cmd, KERNELBOOTCP_PORT, strerror(errno));
        return -errno;
    }
    if ((size_t) rc < sizeof(struct kernelbootcp_message)) {
        ALOGE("invalid response size (%d)\n", (int) rc);
        return -EINVAL;
    }
    msg = (struct kernelbootcp_message *) out;
    if ((cmd | KERNELBOOTCP_BIT) != msg->cmd) {
        ALOGE("invalid command (%d)", msg->cmd);
        return -EINVAL;
    }
    *out_size = ((size_t) rc) - sizeof(struct kernelbootcp_message);
    return rc;
}

void trusty_kernelbootcp_disconnect()
{
    ALOGD("%s enter\n", __func__);
    if (handle_ != 0) {
        tipc_close(handle_);
    }
}

int kernel_bootcp_unlock_ddr(void)
{
    int        result = 0;
    uint8_t   out_msg[16];
    uint32_t  outsize = 16;

    ALOGD("%s enter\n", __func__);
    result = trusty_kernelbootcp_connect();
    if (result != 0) {
        ALOGE("trusty_kernelbootcp_connect failed with ret = %d", result);
        return result;
    }
    result = trusty_kernelbootcp_call(KERNEL_BOOTCP_UNLOCK_DDR, NULL, 0,
                                      (uint8_t *)&out_msg, &outsize);

    trusty_kernelbootcp_disconnect();
    ALOGD("%s leave, ret = %d \n", __func__, result);
    return result;
}

void kbc_dump_table(KBC_LOAD_TABLE_S  *table)
{
    int           i = 0;
    KBC_IMAGE_S  *tmp_table = &(table->modem);

    for (i = 0; i < 4; i ++) {
        ALOGD("dump_table() len = %x maplen = %x addr = %llx \n",
              tmp_table->img_len, tmp_table->map_len, tmp_table->img_addr);
        tmp_table ++;
    }
    ALOGD("dump_table() flag = %d \n", table->flag);
    ALOGD("dump_table() is_packed = %d \n", table->is_packed);
}

int kernel_bootcp_verify_all(KBC_LOAD_TABLE_S  *table)
{
    int       result = 0;
    uint32_t  insize = sizeof(KBC_LOAD_TABLE_S);
    uint8_t   out_msg[16];
    uint32_t  outsize = 16;

    ALOGD("%s enter\n", __func__);
    if (NULL == table) {
        ALOGE("input para wrong!\n");
        return -1;
    }
    result = trusty_kernelbootcp_connect();
    if (result != 0) {
        ALOGE("trusty_kernelbootcp_connect failed with ret = %d", result);
        return result;
    }
//    kbc_dump_table(table);
    result = trusty_kernelbootcp_call(KERNEL_BOOTCP_VERIFY_ALL, (void *)table, insize,
                                      (uint8_t *)&out_msg, &outsize);
    trusty_kernelbootcp_disconnect();
    ALOGD("%s leave, ret = %d \n", __func__, result);
    return result;
}
