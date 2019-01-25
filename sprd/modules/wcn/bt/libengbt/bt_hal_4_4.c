/******************************************************************************
 *
 *  Copyright (C) 2017 Spreadtrum Corporation
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

/******************************************************************************
 *
 *  Filename:      bt_hal.c
 *
 *  Description:   Spreadtrum NPI Test HAL implementation
 *
 ******************************************************************************/

#undef LOG_TAG
#define LOG_TAG "bt_npi"


#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>
#include <ctype.h>
#include <sys/prctl.h>
#include <sys/capability.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <private/android_filesystem_config.h>

#include <hardware/hardware.h>
#include <hardware/bluetooth.h>
#include <dlfcn.h>
#include <utils/Log.h>
#include <semaphore.h>

#include "btif_sprd.h"
#include "bt_hal.h"

#define BTD(param, ...) ALOGD("HAL %s "param, __FUNCTION__, ## __VA_ARGS__)
#define BTE(param, ...) ALOGE("HAL %s "param, __FUNCTION__, ## __VA_ARGS__)
#define UNUSED(x) (void)(x)


#if defined(__LP64__)
static const char kBluetoothLibraryName[] = "/system/lib64/hw/bluetooth.default.so";
#else
static const char kBluetoothLibraryName[] = "/system/lib/hw/bluetooth.default.so";
#endif

const bt_interface_t* bt_iface;
const hw_module_t* module;
const bluetooth_device_t* bt_device;
static hw_device_t* device;
static void *bt_handle;

static bt_status_t status;

static uint8_t bt_enabled = 0;

static sem_t semt_wait;
static pthread_mutex_t mutex_lock;


static const btsprd_interface_t *sBluetoothSprdInterface = NULL;

static bthal_callbacks_t *bthal_callbacks = NULL;

 static btsprd_callbacks_t bt_sprd_callbacks = {
    sizeof(bt_sprd_callbacks),
    NULL
};

typedef struct {
    int ret;
    void *data;
} callback_response_t;

typedef struct {
    const char *name;
    sem_t semaphore;
    callback_response_t res;
} callback_data_t;

callback_data_t callback_data[] = {
    {.name = "bt_rf_get_path"},
};


#define WAIT(callback) callback_wait(#callback)

#define CALLBACK_RET(callback, ret, data) callback_report(#callback, ret, data)

#ifndef CASE_RETURN_STR
#define CASE_RETURN_STR(const) case const: return #const;
#endif


static void callback_init(void)
{
    int i;
    for (i = 0; i < (int)(sizeof(callback_data) / sizeof(callback_data[0])); i++) {
        sem_init(&callback_data[i].semaphore, 0, 0);
    }
}

callback_data_t *get_callback_data(const char *name) {
    int i;
    for (i = 0; i < (int)(sizeof(callback_data) / sizeof(callback_data[0])); i++) {
        if (callback_data[i].name && !strcmp(name, callback_data[i].name)) {
            return &callback_data[i];
        }
    }
    return NULL;
}

static inline callback_response_t *callback_wait(char *callback)
{
    callback_data_t *callback_data = get_callback_data(callback);
    callback_data->res.ret = 1;
    callback_data->res.data = NULL;
    sem_wait(&callback_data->semaphore);
    return &callback_data->res;
}

static inline void callback_report(char *callback, int ret, void *data)
{
    callback_data_t *callback_data = get_callback_data(callback);
    callback_data->res.ret = ret;
    callback_data->res.data = data;
    sem_post(&callback_data->semaphore);
}

static void callback_cleanup(void)
{
    int i;
    for (i = 0; i < (int)(sizeof(callback_data) / sizeof(callback_data[0])); i++) {
         sem_destroy(&callback_data[i].semaphore);
    }
}

static const char* dump_bt_status(bt_status_t status)
{
    switch(status)
    {
        CASE_RETURN_STR(BT_STATUS_SUCCESS)
        CASE_RETURN_STR(BT_STATUS_FAIL)
        CASE_RETURN_STR(BT_STATUS_NOT_READY)
        CASE_RETURN_STR(BT_STATUS_NOMEM)
        CASE_RETURN_STR(BT_STATUS_BUSY)
        CASE_RETURN_STR(BT_STATUS_UNSUPPORTED)

        default:
            return "unknown status code";
    }
}

static void adapter_state_changed(bt_state_t state)
{
    BTD("%s", (state == BT_STATE_OFF)?"OFF":"ON");
    if (state == BT_STATE_ON) {
        bt_enabled = 1;
    } else {
        bt_enabled = 0;
    }
    sem_post(&semt_wait);
}

bt_callbacks_t bt_callbacks = {
    sizeof(bt_callbacks_t),
    adapter_state_changed,
    NULL,
    NULL,
    NULL, /* device_found_cb */
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL, /* dut_mode_recv_cb */
    NULL, /* le_test_mode_cb */
    NULL,
    NULL
};

static int bt_lib_load(void)
{
    const char *id = BT_STACK_MODULE_ID;

    bt_handle = dlopen(kBluetoothLibraryName, RTLD_NOW);
    if (!bt_handle) {
        char const *err_str = dlerror();
        BTE("%s", err_str ? err_str : "error unknown");
        goto error;
    }

    const char *sym = HAL_MODULE_INFO_SYM_AS_STR;
    module = (struct hw_module_t *)dlsym(bt_handle, sym);
    if (!module) {
        BTE("%s", sym);
        goto error;
    }

    if (strcmp(id, module->id) != 0) {
        BTE("id=%s does not match HAL module ID: %s", id, module->id);
        goto error;
    }

    BTD("loaded HAL id=%s path=%s hmi=%p handle=%p",
        id, kBluetoothLibraryName, module, bt_handle);

    return 0;

error:
    if (bt_handle)
        dlclose(bt_handle);

    return -EINVAL;
}

static int bt_lib_unload(void)
{
    int err = 0;
    if (bt_handle)
        dlclose(bt_handle);
    return err;
}

static int btif_test_init(void)
{
    sBluetoothSprdInterface = (btsprd_interface_t *) bt_iface->get_profile_interface(BT_PROFILE_SPRD_ID);
    if (!sBluetoothSprdInterface) {
        BTE("Failed to get Bluetooth Test Interface");
        return -1;
    }
    sBluetoothSprdInterface->init(&bt_sprd_callbacks);
    return 0;
}

static int bt_init(void)
{

    BTD("%s", __func__);
    if (bt_lib_load()) {
        BTE("faild to load Bluetooth library");
        return -1;
    }

    if (module->methods->open(module, BT_HARDWARE_MODULE_ID, &device)) {
        BTE("faild to open the Bluetooth module");
        return -1;
    }

    bt_device = (bluetooth_device_t*)device;
    bt_iface = bt_device->get_bluetooth_interface();

    status = bt_iface->init(&bt_callbacks);
    if (status != BT_STATUS_SUCCESS) {
        BTE("faild to initialize Bluetooth stack");
        return -1;
    }

    btif_test_init();

    sem_init(&semt_wait, 0, 0);
    pthread_mutex_init(&mutex_lock, NULL);
    return 0;
}

static void bt_cleanup(void)
{
    BTD();
    bt_iface->cleanup();
    sleep(1);
    BTE("sleep 1 second");
    bt_lib_unload();
}

static void bt_callbacks_init(bthal_callbacks_t* cb)
{
    bthal_callbacks = cb;
}

static int bt_enable(bthal_callbacks_t* callbacks)
{
    struct timeval time_now;
    struct timespec act_timeout;
    BTD();
    bt_callbacks_init(callbacks);
    if (bt_enabled) {
        BTE("bluetooth is already enabled");
        return 0;
    }

    pthread_mutex_lock(&mutex_lock);

    if (bt_init() < 0) {
        pthread_mutex_unlock(&mutex_lock);
        BTE("bt_init failed");
        return -1;
    }

    status = bt_iface->enable();
    if (status != BT_STATUS_SUCCESS)
        BTE("HAL REQUEST FAILED status : %d (%s)", status, dump_bt_status(status));

    gettimeofday(&time_now, NULL);
    act_timeout.tv_sec = time_now.tv_sec + 8;
    act_timeout.tv_nsec = time_now.tv_usec * 1000;

    if (sem_timedwait(&semt_wait, &act_timeout) <0 ) {
        BTE("%s timeout", __func__);
    }
    pthread_mutex_unlock(&mutex_lock);

    if (!bt_enabled)
        return -1;

    return 0;
}

static int bt_disable(void)
{
    struct timeval time_now;
    struct timespec act_timeout;

    BTD();
    if (!bt_enabled) {
        BTE("Bluetooth is already disabled");
        return 0;
    }

    pthread_mutex_lock(&mutex_lock);
    status = bt_iface->disable();
    if (status != BT_STATUS_SUCCESS)
        BTE("HAL REQUEST FAILED status : %d (%s)", status, dump_bt_status(status));

    gettimeofday(&time_now, NULL);
    act_timeout.tv_sec = time_now.tv_sec + 8;
    act_timeout.tv_nsec = time_now.tv_usec * 1000;

    if (sem_timedwait(&semt_wait, &act_timeout) <0 ) {
        BTE("%s timeout", __func__);
    }
    pthread_mutex_unlock(&mutex_lock);

    if (bt_enabled) {
        bt_cleanup();
        return -1;
    }

    bt_cleanup();

    return 0;
}

static int bt_dut_mode_configure(uint8_t mode)
{
    BTD("%s", __func__);
    if (!bt_enabled) {
        BTE("bluetooth must be enabled for test_mode to work.");
        return -1;
    }

    status = bt_iface->dut_mode_configure(mode);
    if (status != BT_STATUS_SUCCESS)
        BTE("HAL REQUEST FAILED status : %d (%s)", status, dump_bt_status(status));
    return status;
}

static int bt_dut_mode_send(uint16_t opcode, uint8_t* buf, uint8_t len)
{
    if (bt_iface)
        return bt_iface->dut_mode_send(opcode, buf, len);
    return -1;
}

static int bt_le_test_mode(uint16_t opcode, uint8_t* buf, uint8_t len)
{
    if (bt_iface)
        return bt_iface->le_test_mode(opcode, buf, len);
    return -1;
}

static void handle_nonsig_rx_data(tSPRD_VSC_CMPL *p)
{
    bt_status_t status;
    uint8_t result,rssi;
    uint32_t pkt_cnt,pkt_err_cnt;
    uint32_t bit_cnt,bit_err_cnt;
    uint8_t *buf;

    BTD("%s", __FUNCTION__);
    BTD("opcode = 0x%X", p->opcode);
    BTD("param_len = 0x%X", p->param_len);

    if (p->param_len != 18) {
       status =  BT_STATUS_FAIL;
       bthal_callbacks->nonsig_test_rx_recv_cb(status, 0, 0, 0, 0, 0);
       return;
    }

    buf = p->p_param_buf;
    result = *buf;
    rssi = *(buf + 1);
    pkt_cnt = *(uint32_t *)(buf + 2);
    pkt_err_cnt = *(uint32_t *)(buf + 6);
    bit_cnt = *(uint32_t *)(buf + 10);
    bit_err_cnt = *(uint32_t *)(buf + 14);

    BTD("ret:0x%X, rssi:0x%X, pkt_cnt:0x%X, pkt_err_cnt:0x%X, bit_cnt:0x%X, pkt_err_cnt:0x%X",
        result, rssi, pkt_cnt, pkt_err_cnt, bit_cnt, bit_err_cnt);

    if(result == 0)
        status = BT_STATUS_SUCCESS;
    else
        status = BT_STATUS_FAIL;

    bthal_callbacks->nonsig_test_rx_recv_cb(status,
        rssi, pkt_cnt, pkt_err_cnt, bit_cnt, bit_err_cnt);
}

static void dut_vsc_cback(tSPRD_VSC_CMPL *p)
{
    BTD("%s, opcode: 0x%04X", __FUNCTION__, p->opcode);

    switch (p->opcode) {
        case NONSIG_RX_GETDATA:
        case NONSIG_LE_RX_GETDATA: {
                handle_nonsig_rx_data(p);
            }
            break;

        case HCI_DUT_GET_RF_PATH: {
                uint8_t *path = malloc(1);
                *path = *(uint8_t*)p;
                CALLBACK_RET(bt_rf_get_path, 0, path);
            }
            break;

        default:
            break;
    }
}

static int bt_set_nonsig_tx_testmode(uint16_t enable,
    uint16_t le, uint16_t pattern, uint16_t channel,
    uint16_t pac_type, uint16_t pac_len, uint16_t power_type,
    uint16_t power_value, uint16_t pac_cnt)
{
    uint16_t opcode;
    BTD("%s", __FUNCTION__);

    BTD("enable  : %X", enable);
    BTD("le      : %X", le);

    BTD("pattern : %X", pattern);
    BTD("channel : %X", channel);
    BTD("pac_type: %X", pac_type);
    BTD("pac_len : %X", pac_len);
    BTD("power_type   : %X", power_type);
    BTD("power_value  : %X", power_value);
    BTD("pac_cnt      : %X", pac_cnt);

    if(enable){
        opcode = le ? NONSIG_LE_TX_ENABLE : NONSIG_TX_ENABLE;
    }else{
        opcode = le ? NONSIG_LE_TX_DISABLE : NONSIG_TX_DISABLE;
    }

    if(enable){
        uint8_t buf[11];
        memset(buf,0x0,sizeof(buf));

        buf[0] = (uint8_t)pattern;
        buf[1] = (uint8_t)channel;
        buf[2] = (uint8_t)(pac_type & 0x00FF);
        buf[3] = (uint8_t)((pac_type & 0xFF00) >> 8);
        buf[4] = (uint8_t)(pac_len & 0x00FF);
        buf[5] = (uint8_t)((pac_len & 0xFF00) >> 8);
        buf[6] = (uint8_t)power_type;
        buf[7] = (uint8_t)(power_value & 0x00FF);
        buf[8] = (uint8_t)((power_value & 0xFF00) >> 8);
        buf[9] = (uint8_t)(pac_cnt & 0x00FF);
        buf[10] = (uint8_t)((pac_cnt & 0xFF00) >> 8);

        BTD("send hci cmd, opcode = 0x%X", opcode);
        sBluetoothSprdInterface->vendor_cmd_send(opcode, sizeof(buf), buf, dut_vsc_cback);
    }else{/* disable */
        BTD("send hci cmd, opcode = 0x%X",opcode);
        sBluetoothSprdInterface->vendor_cmd_send(opcode, 0, NULL, dut_vsc_cback);
    }

    return 0;
}


static int bt_set_nonsig_rx_testmode(uint16_t enable,
    uint16_t le, uint16_t pattern, uint16_t channel,
    uint16_t pac_type,uint16_t rx_gain, bt_bdaddr_t addr)
{
    uint16_t opcode;
    BTD("%s",__FUNCTION__);

    BTD("enable  : %X",enable);
    BTD("le      : %X",le);

    BTD("pattern : %d",pattern);
    BTD("channel : %d",channel);
    BTD("pac_type: %d",pac_type);
    BTD("rx_gain : %d",rx_gain);
    BTD("addr    : %02X:%02X:%02X:%02X:%02X:%02X",
        addr.address[0],addr.address[1],addr.address[2],
        addr.address[3],addr.address[4],addr.address[5]);

    if(enable){
        opcode = le ? NONSIG_LE_RX_ENABLE : NONSIG_RX_ENABLE;
    }else{
        opcode = le ? NONSIG_LE_RX_DISABLE : NONSIG_RX_DISABLE;
    }

    if(enable){
        uint8_t buf[11];
        memset(buf,0x0,sizeof(buf));

        buf[0] = (uint8_t)pattern;
        buf[1] = (uint8_t)channel;
        buf[2] = (uint8_t)(pac_type & 0x00FF);
        buf[3] = (uint8_t)((pac_type & 0xFF00) >> 8);
        buf[4] = (uint8_t)rx_gain;
        buf[5] = addr.address[5];
        buf[6] = addr.address[4];
        buf[7] = addr.address[3];
        buf[8] = addr.address[2];
        buf[9] = addr.address[1];
        buf[10] = addr.address[0];
        sBluetoothSprdInterface->vendor_cmd_send(opcode, sizeof(buf), buf, dut_vsc_cback);
    }else{
        sBluetoothSprdInterface->vendor_cmd_send(opcode, 0, NULL, dut_vsc_cback);
    }

    return 0;
}

static int bt_get_nonsig_rx_data(uint16_t le)
{
    BTD("get_nonsig_rx_data LE=%d",le);
    uint16_t opcode;
    opcode = le ? NONSIG_LE_RX_GETDATA : NONSIG_RX_GETDATA;
    sBluetoothSprdInterface->vendor_cmd_send(opcode, 0, NULL, dut_vsc_cback);
    return 0;
}

static int bt_le_enhanced_receiver_cmd(uint8_t channel, uint8_t phy, uint8_t modulation_index)
{
    uint8_t buf[3];

    BTD("%s channel: 0x%02x, phy: 0x%02x, modulation_index: 0x%02x", __func__, channel, phy, modulation_index);
    buf[0] = channel;
    buf[1] = phy;
    buf[2] = modulation_index;
    sBluetoothSprdInterface->vendor_cmd_send(HCI_BLE_ENHANCED_RECEIVER_TEST, sizeof(buf), buf, dut_vsc_cback);
    return 0;
}

static int bt_le_enhanced_transmitter_cmd(uint8_t channel, uint8_t length, uint8_t payload, uint8_t phy)
{
    uint8_t buf[4];

    BTD("%s channel: 0x%02x, length: 0x%02x, payload: 0x%02x, phy: 0x%02x", __func__, channel, length, payload, phy);
    buf[0] = channel;
    buf[1] = length;
    buf[2] = payload;
    buf[3] = phy;
    sBluetoothSprdInterface->vendor_cmd_send(HCI_BLE_ENHANCED_TRANSMITTER_TEST, sizeof(buf), buf, dut_vsc_cback);
    return 0;
}

static int bt_le_test_end_cmd(void)
{

    BTD("%s", __func__);
    sBluetoothSprdInterface->vendor_cmd_send(HCI_BLE_TEST_END, 0, NULL, dut_vsc_cback);
    return 0;
}

static int bt_rf_set_path(uint8_t path)
{

    BTD("%s path: %u", __func__, path);
    sBluetoothSprdInterface->vendor_cmd_send(HCI_DUT_SET_RF_PATH, sizeof(path), &path, dut_vsc_cback);
    return 0;
}

static int bt_rf_get_path(void)
{
    int path;
    callback_response_t *res;

    BTD("%s wait to get path", __func__);
    sBluetoothSprdInterface->vendor_cmd_send(HCI_DUT_GET_RF_PATH, 0, NULL, dut_vsc_cback);
    res = WAIT(bt_rf_get_path);

    path = *((uint8_t*)res->data);
    free(res->data);
    res->data = NULL;

    BTD("%s retrun path: %u", __func__, path);
    return path;
}

static uint8_t is_enable(void)
{
    return bt_enabled;
}


static bt_test_kit_t bt_test_kit = {
    .size = sizeof(bt_test_kit_t),
    .enable = bt_enable,
    .disable = bt_disable,
    .dut_mode_configure = bt_dut_mode_configure,
    .dut_mode_send = bt_dut_mode_send,
    .le_test_mode = bt_le_test_mode,
    .is_enable = is_enable,
    .set_nonsig_tx_testmode = bt_set_nonsig_tx_testmode,
    .set_nonsig_rx_testmode = bt_set_nonsig_rx_testmode,
    .get_nonsig_rx_data = bt_get_nonsig_rx_data,
    .le_enhanced_receiver = bt_le_enhanced_receiver_cmd,
    .le_enhanced_transmitter = bt_le_enhanced_transmitter_cmd,
    .le_test_end = bt_le_test_end_cmd,
    .set_rf_path = bt_rf_set_path,
    .get_rf_path = bt_rf_get_path,
};

const bt_test_kit_t *bt_test_kit_get_interface(void) {
    return &bt_test_kit;
}

