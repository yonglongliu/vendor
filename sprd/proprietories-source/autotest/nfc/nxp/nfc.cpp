#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <time.h>
#include <errno.h>

#include <cutils/log.h>
#include "diag.h"
#include <debug.h>
//#include "autotest_modules.h"

#include <hardware/hardware.h>
#include <hardware/nfc.h>
#include "nfc.h"

void OSI_delay (uint32_t timeout);

static nfc_event_t last_event = HAL_NFC_ERROR_EVT;
static nfc_status_t last_evt_status = HAL_NFC_STATUS_FAILED;
void hal_context_callback(nfc_event_t event, nfc_status_t event_status)
{
    last_event = event;
    last_evt_status = event_status;
    ALOGD("nfc context callback event=%u, status=%u\n", event, event_status);
}

static uint16_t last_recv_data_len = 0;
static uint8_t last_rx_buff[260]={0};
void hal_context_data_callback (uint16_t data_len, uint8_t* p_data)
{
    ALOGD("nfc data callback len=%d\n", data_len);
    last_recv_data_len = data_len;
    memcpy(last_rx_buff, p_data, data_len);
}

    //int ret = 0; //0 means success
    const struct hw_module_t* hw_module = NULL;
    nfc_nci_device_t* nfc_device = NULL;

    int max_try = 0;
    uint8_t nci_core_reset_cmd[4]={0x20,0x00,0x01,0x01};  //1. 15. 18. 20000101
    uint8_t nci_initializ_cmd[3]={0x20,0x01,0x00};    //2. 16. 19. 200100
    uint8_t nci_rf_txldo_cmd[3] = {0x2F, 0x02, 0x00};  //3. 2F0200 NCI SYSTEM ACT PROPRIETARY EXT
    uint8_t nci_rf_txldo_cmd2[4] = {0x2F, 0x00, 0x01, 0x00};  //4. 2F000100  ·µ»Ø8×Ö½Ú»òÕß258×Ö½Ú
    uint8_t nci_cmd[8] = {0x20, 0x02, 0x05, 0x01, 0xA0, 0x03, 0x01, 0x08}; //5. 20020501A0030108  ·µ»Ø4002020000
    uint8_t nci_cmd2[15] = {0x20, 0x02, 0x0C, 0x01, 0xA0, 0x26, 0x08, 0x20, 0x41, 0xA3, 0x01, 0x88, 0x01, 0xE2, 0x02}; //6. 20020C01A026082041A3018801E202 //4002020000
    uint8_t nci_cmd3[15] = {0x20, 0x02, 0x0C, 0x01, 0xA0, 0x20, 0x08, 0x08, 0x52, 0xA2, 0x82, 0x30, 0x01, 0xE1, 0x02};//7. 20020C01A020080852A2823001E102 ·µ»Ø4002020000
    uint8_t core_set_config_cmd_1[18] = {0x20,0x02,0x0F,0x01,0xA0,0x0E,0x0B,0x11,0x01,0x01,0x31,0x00,0x00,0x00,0x02,0x00,0x01,0x0C};
    //8. 20020F01A00E0B110101310000002000100C TVDD configuration - Config 1 //·µ»Ø4002020000

    uint8_t nci_rx_cmd[13] = {0x20, 0x02, 0x0A, 0x01, 0xA0, 0x0D, 0x06, 0x34, 0x44, 0x55, 0x0A, 0x00, 0x00}; //9. //RM Reception on type A - RX gain ·µ»Ø4002020000
    uint8_t nci_min_cmd[13] = {0x20, 0x02, 0x0A, 0x01, 0xA0, 0x0D, 0x06, 0x34, 0x2D, 0xDC, 0x50, 0x03, 0x00}; //10. //RM Reception on type A - Min level  ·µ»Ø4002020000
    uint8_t nci_lpcd_cmd[8] = {0x20, 0x02, 0x05, 0x01, 0xA0, 0x40, 0x01, 0x00}; //11. LPCD off ·µ»Ø4002020000

    uint8_t nci_dpc_cmd1[94] = {0x20,0x02,0x5B,0x01,0xA0,0x0B,0x57,0x08,0x08,0x90,0x78,0x0F,0x4E,0x00,0x3D,0x95,0x00,0x00,0x3D,0x9F,0x00,0x00,0x50,0x9F,0x00,0x00,0x59,0x9F,0x00,0x00,0x5A,0x9F,0x00,0x00,0x64,0x9F,0x00,0x00,0x65,0x9F,0x00,0x00,0x6E,0x9F,0x00,0x00,0x72,0x9F,0x00,0x00,0x79,0x1F,0x00,0x00,0x7B,0x1F,0x00,0x00,0x84,0x1F,0x00,0x00,0x86,0x1F,0x00,0x00,0x8F,0x1F,0x00,0x00,0x91,0x1F,0x00,0x00,0x9A,0x1F,0x00,0x00,0xA1,0x9F,0x00,0x00,0xA7,0x1F,0x00,0x00,0xB0,0x1F,0x00,0x00,0xB9,0x1F,0x00,0x00};//12. DPC disable 1 ·µ»Ø4002020000
    uint8_t nci_dpc_cmd2[95] = {0x20,0x02,0x5C,0x01,0xA0,0x0B,0x58,0x08,0x88,0x90,0x78,0x0F,0x4E,0x32,0x00,0x3D,0x95,0x00,0x00,0x3D,0x9F,0x00,0x00,0x50,0x9F,0x00,0x00,0x59,0x9F,0x00,0x00,0x5A,0x9F,0x00,0x00,0x64,0x9F,0x00,0x00,0x65,0x9F,0x00,0x00,0x6E,0x9F,0x00,0x00,0x72,0x9F,0x00,0x00,0x79,0x1F,0x00,0x00,0x7B,0x1F,0x00,0x00,0x84,0x1F,0x00,0x00,0x86,0x1F,0x00,0x00,0x8F,0x1F,0x00,0x00,0x91,0x1F,0x00,0x00,0x9A,0x1F,0x00,0x00,0xA1,0x1F,0x00,0x00,0xA7,0x1F,0x00,0x00,0xB0,0x1F,0x00,0x00,0xB9,0x1F,0x00,0x00};//13. DPC disable 2 ·µ»Ø4002020500

    uint8_t nci_cmd4[5] = {0x2F, 0x3D, 0x02, 0x01, 0x80}; //17. //Current Measurement
    uint8_t nci_cmd5[6] = {0x20, 0x03, 0x03, 0x01, 0xA0, 0x3F};//20.
    uint8_t nci_cmd6[7] = {0x20, 0x02, 0x04, 0x01, 0x21, 0x01, 0x00};//21.
    uint8_t nci_cmd7[4] = {0x2F, 0x00, 0x01, 0x00};//22
    uint8_t nci_cmd8[7] = {0x21, 0x00, 0x04, 0x01, 0x04, 0x01, 0x02};//23. //·µ»Ø41000100
    uint8_t discover_cmd[6] = {0x21,0x03,0x03,0x01,0x00,0x01};//24. 210303010001 //·µ»Ø41030100
    uint8_t nci_dlma_cmd[217] = {0x20,0x02,0xD6,0x01,0xA0,0x34,0xD2,0x23,0x04,0x18,0x07,0x40,0x00,0x20,0x40,0x00,0xBE,0x23,0x60,0x00,0x2B,0x13,0x40,0x00,0xB8,0x21,0x60,0x00,0x38,0x35,0x00,0x00,0x18,0x46,0x08,0x00,0xDE,0x54,0x08,0x02,0x00,0x00,0x08,0x02,0x00,0x00,0x08,0x02,0x00,0x00,0x08,0x02,0x00,0x00,0x08,0x02,0x00,0x00,0x08,0x02,0x00,0x00,0x08,0x02,0x00,0x00,0x48,0x01,0x00,0x00,0x08,0x83,0x00,0x00,0x08,0x01,0x00,0x00,0xC8,0x02,0x00,0x00,0xC8,0x00,0x00,0x00,0x88,0x02,0x00,0x00,0x48,0x02,0x00,0x00,0xB8,0x00,0x00,0x00,0x68,0x00,0x00,0x00,0x18,0x00,0x00,0x00,0x08,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x07,0x00,0x20,0x40,0x00,0xBE,0x23,0x60,0x00,0x2B,0x13,0x40,0x00,0xB8,0x21,0x60,0x00,0x38,0x35,0x00,0x00,0x18,0x46,0x08,0x00,0xDE,0x54,0x08,0x02,0x00,0x00,0x08,0x02,0x00,0x00,0x08,0x02,0x00,0x00,0x08,0x02,0x00,0x00,0x08,0x02,0x00,0x00,0x08,0x82,0x00,0x00,0x48,0x01,0x00,0x00,0x08,0x03,0x00,0x00,0x00,0x01,0x00,0x00,0xc8,0x02,0x00,0x00,0xC8,0x00,0x00,0x00,0x88,0x02,0x00,0x00,0x48,0x02,0x00,0x00,0xB8,0x00,0x00,0x00,0x68,0x00,0x00,0x00,0x18,0x00,0x00,0x00,0x08,0x02,0x00,0x00,0x00,0x00};
    //14. 2002D601A034D2230418074000204000BE2360002B134000B82160003835000018460800DE5408020000080200000802000008020000080200000802000008020000480100000803000008010000C8020000C80000008802000048020000B80000006800000018000000080200000000000000000700204000BE2360002B134000B82160003835000018460800DE5408020000080200000802000008020000080200000802000008020000480100000803000008010000C8020000C80000008802000048020000B80000006800000018000000080200000000


static int checkID_result=1;
int nfc_open(void)
{
    int ret =0;
    ALOGD("hw_get_module....");
    ret = hw_get_module (NFC_NCI_NXP_PN54X_HARDWARE_MODULE_ID, &hw_module);
    if (ret == 0)
    {
        ALOGD("OK\nopen module...");
        ret = nfc_nci_open(hw_module, &nfc_device);
        if (ret != 0){
            ALOGD ("fail.\n");
            return -1;
        }
    }
    ALOGD("OK\n");

    ALOGD("open nfc device...\n");
    nfc_device->open (nfc_device, hal_context_callback, hal_context_data_callback);

    max_try = 0;
    last_event = HAL_NFC_ERROR_EVT;
    last_evt_status = HAL_NFC_STATUS_FAILED;
    last_recv_data_len = 0;
    memset(last_rx_buff, 0, sizeof(last_rx_buff));
    while(last_event != HAL_NFC_OPEN_CPLT_EVT && last_recv_data_len != 6  ){
        OSI_delay(100);
        if(max_try ++ > 150){//FW will be downloaded if not forced, might take up to 15 seconds?
            ALOGD("open device timeout!\n");
            nfc_nci_close(nfc_device);
            return -1;
        }
    }
    if(last_recv_data_len == 6){ //No FW was downloaded, HAL sent reset but did not report HAL_NFC_OPEN_CPLT_EVT
        if(last_rx_buff[0] == 0x40 &&
            last_rx_buff[1] == 0x00 &&
            last_rx_buff[2] == 0x03 &&
            last_rx_buff[3] == 0x00 &&
            last_rx_buff[5] == 0x01)
            {
                ALOGD("HAL reset nfc device during open.\n");
                ret  = 0;
            } else{
                nfc_nci_close(nfc_device);
                ALOGD("HAL reported error reset response!\n");
                return -1;
            }
    } else if(last_evt_status == HAL_NFC_STATUS_OK){
        ALOGD("HAL reported open completed event!\n");
    } else{
        nfc_nci_close(nfc_device);
        ALOGD("open device failed\n");
        return -1;
    }
    ALOGD("OK\n");

    //Reset device
    if(last_recv_data_len == 0){
        ALOGD("reset nfc device nci_core_reset_cmd ...\n");
        max_try = 0;
        last_recv_data_len = 0;
        memset(last_rx_buff, 0, sizeof(last_rx_buff));
        nfc_device->write(nfc_device, sizeof(nci_core_reset_cmd), nci_core_reset_cmd);
        while(last_recv_data_len == 0){
            OSI_delay(10);
            if(max_try ++ > 500){
                ALOGD("reset device timeout\n");
                break;
            }
        }
        if(last_recv_data_len > 0)
        {
            ALOGD("nci_core_reset_cmd OK\n");
            ret  = 0;
        } else{
            ALOGD("failed to reset nfc device.\n");
            ret = -1;
        }
    }

    //Initialize device
    if(ret == 0){
        ALOGD("Initialize nfc device nci_initializ_cmd ...\n");
        max_try = 0;
        last_recv_data_len = 0;
        memset(last_rx_buff, 0, sizeof(last_rx_buff));
        nfc_device->write(nfc_device, sizeof(nci_initializ_cmd), nci_initializ_cmd);
        while(last_recv_data_len == 0){
            OSI_delay(10);
            if(max_try ++ > 500){
                ALOGD("initiaze device timeout\n");
                break;
            }
        }
        if(last_recv_data_len > 0)
        {
            ALOGD("nci_initializ_cmd OK\n");
            ret  = 0;
        } else{
            ALOGD("failed to initialze nfc device.\n");
            ret = -1;
        }
    }
    if(ret == 0){
        ALOGD("F/W version: %x.%x.%x\n", last_rx_buff[25],last_rx_buff[26], last_rx_buff[27]);
    }

    if(ret == 0){
        ALOGD("nci_rf_txldo_cmd...\n");
        max_try = 0;
        last_recv_data_len = 0;
        memset(last_rx_buff, 0, sizeof(last_rx_buff));
        nfc_device->write(nfc_device, sizeof(nci_rf_txldo_cmd), nci_rf_txldo_cmd);
        while(last_recv_data_len == 0){
            OSI_delay(10);
            if(max_try ++ > 500){
                ALOGD("initiaze device timeout\n");
                break;
            }
        }
        if(last_recv_data_len > 0)
        {
            ALOGD("nci_rf_txldo_cmd OK\n");
            checkID_result  = 0;
        } else{
            ALOGD("nci_rf_txldo_cmd failed .\n");
            checkID_result = 1;
        }
    }

    if(ret == 0){
        ALOGD("nci_rf_txldo_cmd2...\n");
        max_try = 0;
        last_recv_data_len = 0;
        memset(last_rx_buff, 0, sizeof(last_rx_buff));
        nfc_device->write(nfc_device, sizeof(nci_rf_txldo_cmd2), nci_rf_txldo_cmd2); //4F000100
        while(last_recv_data_len == 0){
            OSI_delay(10);
            if(max_try ++ > 500){
                ALOGD("initiaze device timeout\n");
                break;
            }
        }
        ALOGD("nci_rf_txldo_cmd2 last_recv_data_len=d%\n",last_recv_data_len);
        if(last_recv_data_len > 0)
        {
            ALOGD("nci_rf_txldo_cmd2 OK\n");
            checkID_result = 0;
        } else{
            ALOGD("failed to nci_rf_txldo_cmd2.\n");
            checkID_result = 1;
        }
    }
    return ret;
}


int nfc_set_discover_cmd(void)
{
    //core set config cmd 1
    {
        ALOGD("core set nci_cmd...\n");

        max_try = 0;
        last_recv_data_len = 0;
        memset(last_rx_buff, 0, sizeof(last_rx_buff));
        nfc_device->write(nfc_device, sizeof(nci_cmd), nci_cmd);
        while(last_recv_data_len == 0){
            OSI_delay(10);
            if(max_try ++ > 100){
                ALOGD("core set config cmd 1 timeout\n");
                break;
            }
        }
        if(last_recv_data_len > 0 )
        {
            ALOGD("nci_cmd OK\n");
            checkID_result = 0;
        } else{
            ALOGD("failed to core set config cmd 1.\n");
            checkID_result = 1;
        }
    }

    if(checkID_result == 0)
    {
        ALOGD("core set nci_cmd2...\n");

        max_try = 0;
        last_recv_data_len = 0;
        memset(last_rx_buff, 0, sizeof(last_rx_buff));
        nfc_device->write(nfc_device, sizeof(nci_cmd2), nci_cmd2);
        while(last_recv_data_len == 0){
            OSI_delay(10);
            if(max_try ++ > 100){
                ALOGD("core set config cmd 2 timeout\n");
                break;
            }
        }
        ALOGD("nci_cmd2 last_recv_data_len%d\n",last_recv_data_len);
        if(last_recv_data_len > 0)
        {
            ALOGD("nci_cmd2 OK\n");
            checkID_result = 0;
        } else{
            ALOGD("failed to core set config cmd 2.\n");
            checkID_result = 1;
        }
    }

    if(checkID_result == 0)
    {
        ALOGD("core set nci_cmd3...\n");

        max_try = 0;
        last_recv_data_len = 0;
        memset(last_rx_buff, 0, sizeof(last_rx_buff));
        nfc_device->write(nfc_device, sizeof(nci_cmd3), nci_cmd3);
        while(last_recv_data_len == 0){
            OSI_delay(10);
            if(max_try ++ > 100){
                ALOGD("core set config cmd 2 timeout\n");
                break;
            }
        }
        if(last_recv_data_len > 0)
        {
           ALOGD("nci_cmd3 OK\n");
           checkID_result = 0;
        } else{
            ALOGD("failed to nci_cmd3.\n");
            checkID_result = 1;
        }
    }

    if(checkID_result == 0)
    {
        ALOGD("core set config cmd core_set_config_cmd_1...\n");

        max_try = 0;
        last_recv_data_len = 0;
        memset(last_rx_buff, 0, sizeof(last_rx_buff));
        nfc_device->write(nfc_device, sizeof(core_set_config_cmd_1), core_set_config_cmd_1);
        while(last_recv_data_len == 0){
            OSI_delay(10);
            if(max_try ++ > 100){
                ALOGD("core set config cmd 2 timeout\n");
                break;
            }
        }
        if(last_recv_data_len > 0)
        {
            ALOGD("core_set_config_cmd_1 OK\n");
            checkID_result = 0;
        } else{
            ALOGD("failed to core set config cmd 1.\n");
            checkID_result = 1;
        }
    }

    if(checkID_result == 0)
    {
        ALOGD("core set nci_rx_cmd...\n");

        max_try = 0;
        last_recv_data_len = 0;
        memset(last_rx_buff, 0, sizeof(last_rx_buff));
        nfc_device->write(nfc_device, sizeof(nci_rx_cmd), nci_rx_cmd);
        while(last_recv_data_len == 0){
            OSI_delay(10);
            if(max_try ++ > 100){
                ALOGD("core set config cmd 2 timeout\n");
                break;
            }
        }
        if(last_recv_data_len > 0)
        {
            ALOGD("nci_rx_cmd OK\n");
            checkID_result = 0;
        } else{
            ALOGD("failed to nci_rx_cmd.\n");
            checkID_result = 1;
        }
    }

    if(checkID_result == 0)
    {
        ALOGD("core set nci_min_cmd...\n");

        max_try = 0;
        last_recv_data_len = 0;
        memset(last_rx_buff, 0, sizeof(last_rx_buff));
        nfc_device->write(nfc_device, sizeof(nci_min_cmd), nci_min_cmd);
        while(last_recv_data_len == 0){
            OSI_delay(10);
            if(max_try ++ > 100){
                ALOGD("core set config cmd 2 timeout\n");
                break;
            }
        }
        if(last_recv_data_len > 0)
        {
            ALOGD("nci_min_cmd OK\n");
            checkID_result = 0;
        } else{
            ALOGD("failed to nci_min_cmd.\n");
            checkID_result = 1;
        }
    }

    if(checkID_result == 0)
    {
        ALOGD("core set nci_lpcd_cmd...\n");

        max_try = 0;
        last_recv_data_len = 0;
        memset(last_rx_buff, 0, sizeof(last_rx_buff));
        nfc_device->write(nfc_device, sizeof(nci_lpcd_cmd), nci_lpcd_cmd);
        while(last_recv_data_len == 0){
            OSI_delay(10);
            if(max_try ++ > 100){
                ALOGD("core set config cmd 2 timeout\n");
                break;
            }
        }
        if(last_recv_data_len > 0)
        {
            ALOGD("nci_lpcd_cmd OK\n");
            checkID_result = 0;
        } else{
            ALOGD("failed to nci_lpcd_cmd.\n");
            checkID_result = 1;
        }
    }

    if(checkID_result == 0)
    {
        ALOGD("core set nci_dpc_cmd1...\n");

        max_try = 0;
        last_recv_data_len = 0;
        memset(last_rx_buff, 0, sizeof(last_rx_buff));
        nfc_device->write(nfc_device, sizeof(nci_dpc_cmd1), nci_dpc_cmd1);
        while(last_recv_data_len == 0){
            OSI_delay(10);
            if(max_try ++ > 100){
                ALOGD("core set config cmd 2 timeout\n");
                break;
            }
        }
        if(last_recv_data_len > 0)
        {
            ALOGD("nci_dpc_cmd1 OK\n");
            checkID_result = 0;
        } else{
            ALOGD("failed to nci_dpc_cmd1.\n");
            checkID_result = 1;
        }
    }

    if(checkID_result == 0)
    {
        ALOGD("core set nci_dpc_cmd2...\n");

        max_try = 0;
        last_recv_data_len = 0;
        memset(last_rx_buff, 0, sizeof(last_rx_buff));
        nfc_device->write(nfc_device, sizeof(nci_dpc_cmd2), nci_dpc_cmd2);
        while(last_recv_data_len == 0){
            OSI_delay(10);
            if(max_try ++ > 100){
                ALOGD("core set config cmd 2 timeout\n");
                break;
            }
        }
        if(last_recv_data_len > 0)
        {
            ALOGD("nci_dpc_cmd2 OK\n");
            checkID_result = 0;
        } else{
            ALOGD("failed to nci_dpc_cmd2.\n");
            checkID_result = 1;
        }
    }

    if(checkID_result == 0)
    {
        ALOGD("nci_dlma_cmd...\n");

        max_try = 0;
        last_recv_data_len = 0;
        memset(last_rx_buff, 0, sizeof(last_rx_buff));
        nfc_device->write(nfc_device, sizeof(nci_dlma_cmd), nci_dlma_cmd);
        while(last_recv_data_len == 0){
            OSI_delay(10);
            if(max_try ++ > 100){
                ALOGD("rf set listen mode routing cmd timeout\n");
                break;
            }
        }
        if(last_recv_data_len > 0)
        {
            ALOGD("nci_dlma_cmd OK\n");
            checkID_result = 0;
        } else{
            ALOGD("failed to nci_dlma_cmd.\n");
            checkID_result = 1;
        }
    }

    if(checkID_result == 0)
    {
        ALOGD("rf set listen mode routing cmd...\n");

        max_try = 0;
        last_recv_data_len = 0;
        memset(last_rx_buff, 0, sizeof(last_rx_buff));
        nfc_device->write(nfc_device, sizeof(nci_dlma_cmd), nci_dlma_cmd);
        while(last_recv_data_len == 0){
            OSI_delay(10);
            if(max_try ++ > 100){
                ALOGD("rf set listen mode routing cmd timeout\n");
                break;
            }
        }

        if(last_recv_data_len > 0)
        {
            ALOGD("nci_dlma_cmd OK\n");
            checkID_result = 0;
        } else{
            ALOGD("failed to nci_dlma_cmd.\n");
            checkID_result = 1;
        }
    }

    //Reset device
    if(checkID_result == 0){
        ALOGD("reset nfc device nci_core_reset_cmd ...\n");
        max_try = 0;
        last_recv_data_len = 0;
        memset(last_rx_buff, 0, sizeof(last_rx_buff));
        nfc_device->write(nfc_device, sizeof(nci_core_reset_cmd), nci_core_reset_cmd);
        while(last_recv_data_len == 0){
            OSI_delay(10);
            if(max_try ++ > 500){
                ALOGD("reset device timeout\n");
                break;
            }
        }
        if(last_recv_data_len > 0)
        {
            ALOGD("nci_core_reset_cmd OK\n");
            checkID_result  = 0;
        } else{
            ALOGD("failed to reset nfc device.\n");
            checkID_result = 1;
        }
    }

    if(checkID_result == 0){
        ALOGD("Initialize nfc device nci_initializ_cmd ...\n");
        max_try = 0;
        last_recv_data_len = 0;
        memset(last_rx_buff, 0, sizeof(last_rx_buff));
        nfc_device->write(nfc_device, sizeof(nci_initializ_cmd), nci_initializ_cmd);
        while(last_recv_data_len == 0){
            OSI_delay(10);
            if(max_try ++ > 500){
                ALOGD("initiaze device timeout\n");
                break;
            }
        }
        if(last_recv_data_len > 0)
        {
            ALOGD("nci_initializ_cmd OK\n");
            checkID_result  = 0;
        } else{
            ALOGD("failed to initialze nfc device.\n");
            checkID_result = 1;
        }
    }

    if(checkID_result == 0){
        ALOGD("nci_cmd4 ...\n");
        max_try = 0;
        last_recv_data_len = 0;
        memset(last_rx_buff, 0, sizeof(last_rx_buff));
        nfc_device->write(nfc_device, sizeof(nci_cmd4), nci_cmd4);
        while(last_recv_data_len == 0){
            OSI_delay(10);
            if(max_try ++ > 500){
                ALOGD("initiaze device timeout\n");
                break;
            }
        }
        if(last_recv_data_len > 0)
        {
            ALOGD("nci_cmd4 OK\n");
            checkID_result  = 0;
        } else{
            ALOGD("failed to nci_cmd4.\n");
            checkID_result = 1;
        }
    }

    //Reset device
    if(checkID_result == 0){
        ALOGD("reset nfc device nci_core_reset_cmd ...\n");
        max_try = 0;
        last_recv_data_len = 0;
        memset(last_rx_buff, 0, sizeof(last_rx_buff));
        nfc_device->write(nfc_device, sizeof(nci_core_reset_cmd), nci_core_reset_cmd);
        while(last_recv_data_len == 0){
            OSI_delay(10);
            if(max_try ++ > 500){
                ALOGD("reset device timeout\n");
                break;
            }
        }
        if(last_recv_data_len > 0)
        {
            ALOGD("nci_core_reset_cmd OK\n");
            checkID_result  = 0;
        } else{
            ALOGD("failed to reset nfc device.\n");
            checkID_result = 1;
        }
    }

    if(checkID_result == 0){
        ALOGD("Initialize nfc device nci_initializ_cmd ...\n");
        max_try = 0;
        last_recv_data_len = 0;
        memset(last_rx_buff, 0, sizeof(last_rx_buff));
        nfc_device->write(nfc_device, sizeof(nci_initializ_cmd), nci_initializ_cmd);
        while(last_recv_data_len == 0){
            OSI_delay(10);
            if(max_try ++ > 500){
                ALOGD("initiaze device timeout\n");
                break;
            }
        }
        if(last_recv_data_len > 0)
        {
            ALOGD("initialze nfc OK\n");
            checkID_result  = 0;
        } else{
            ALOGD("failed to initialze nfc device.\n");
            checkID_result = 1;
        }
    }

    if(checkID_result == 0){
        ALOGD("nci_cmd5 ...\n");
        max_try = 0;
        last_recv_data_len = 0;
        memset(last_rx_buff, 0, sizeof(last_rx_buff));
        nfc_device->write(nfc_device, sizeof(nci_cmd5), nci_cmd5);
        while(last_recv_data_len == 0){
            OSI_delay(10);
            if(max_try ++ > 500){
                ALOGD("initiaze device timeout\n");
                break;
            }
        }
        if(last_recv_data_len > 0)
        {
            ALOGD("nci_cmd5 OK\n");
            checkID_result  = 0;
        } else{
            ALOGD("failed to nci_cmd5.\n");
            checkID_result = 1;
        }
    }

    if(checkID_result == 0){
        ALOGD("nci_cmd6 ...\n");
        max_try = 0;
        last_recv_data_len = 0;
        memset(last_rx_buff, 0, sizeof(last_rx_buff));
        nfc_device->write(nfc_device, sizeof(nci_cmd6), nci_cmd6);
        while(last_recv_data_len == 0){
            OSI_delay(10);
            if(max_try ++ > 500){
                ALOGD("initiaze device timeout\n");
                break;
            }
        }
        if(last_recv_data_len > 0)
        {
            ALOGD("nci_cmd6 OK\n");
            checkID_result  = 0;
        } else{
            ALOGD("failed to nci_cmd6.\n");
            checkID_result = 1;
        }
    }

    if(checkID_result == 0){
        ALOGD("nci_rf_txldo_cmd2 ...\n");
        max_try = 0;
        last_recv_data_len = 0;
        memset(last_rx_buff, 0, sizeof(last_rx_buff));
        nfc_device->write(nfc_device, sizeof(nci_rf_txldo_cmd2), nci_rf_txldo_cmd2);
        while(last_recv_data_len == 0){
            OSI_delay(10);
            if(max_try ++ > 500){
                ALOGD("initiaze device timeout\n");
                break;
            }
        }
        if(last_recv_data_len > 0)
        {
            ALOGD("nci_rf_txldo_cmd2 OK\n");
            checkID_result  = 0;
        } else{
            ALOGD("failed to nci_rf_txldo_cmd2.\n");
            checkID_result = 1;
        }
    }

    if(checkID_result == 0){
        ALOGD("nci_cmd8 ...\n");
        max_try = 0;
        last_recv_data_len = 0;
        memset(last_rx_buff, 0, sizeof(last_rx_buff));
        nfc_device->write(nfc_device, sizeof(nci_cmd8), nci_cmd8);
        while(last_recv_data_len == 0){
            OSI_delay(10);
            if(max_try ++ > 500){
                ALOGD("initiaze device timeout\n");
                break;
            }
        }
        if(last_recv_data_len > 0)
        {
            ALOGD("nci_cmd8 OK\n");
            checkID_result  = 0;
        } else{
            ALOGD("failed to nci_cmd8.\n");
            checkID_result = 1;
        }
    }

    if(checkID_result == 0){
        ALOGD("rf discover...\n");
        max_try = 0;
        last_recv_data_len = 0;
        memset(last_rx_buff, 0, sizeof(last_rx_buff));
        nfc_device->write(nfc_device, sizeof(discover_cmd), discover_cmd);
        while(last_recv_data_len == 0){
            OSI_delay(10);
            if(max_try ++ > 100){
                ALOGD("discover cmd timeout\n");
                break;
            }
        }
        if(last_recv_data_len > 0)
        {
            ALOGD("discover_cmd OK\n");
            checkID_result = 0;
        } else{
            ALOGD("failed to set nfc to poll.\n");
            checkID_result = 1;
        }
    }
    return checkID_result;
}

int nfc_checkID(void)
{
    checkID_result = nfc_set_discover_cmd();

    //Ready to detect the tags from antenna
    if(checkID_result == 0){
        ALOGD("nfc is polling, please put a type 1/2/4 tag to the antenna...\n");
        max_try = 0;
        last_recv_data_len = 0;
        memset(last_rx_buff, 0, sizeof(last_rx_buff));
        while(last_recv_data_len == 0){
            OSI_delay(100);
            if(max_try ++ > 50){ //Wait max 30 seconds
                ALOGD("waiting tag timeout\n");
                break;
            }
        }
        if(last_recv_data_len > 2 && last_rx_buff[0] == 0x61 && last_rx_buff[1] == 0x05)
        {
            ALOGD("Tag detected.\n");
            checkID_result = 0;
        } else {
            ALOGD("failed to detect known tags.\n");
            checkID_result = 1;
        }
    }
    return checkID_result;
}

int nfc_close(void)
{
    int ret=0;
    ALOGD("Close device\n");
    ret = nfc_device->close (nfc_device);
    ret = nfc_nci_close(nfc_device);
    return ret;
}


void OSI_delay (uint32_t timeout)
{
    struct timespec delay;
    int err;

    delay.tv_sec = timeout / 1000;
    delay.tv_nsec = 1000 * 1000 * (timeout%1000);

    do
    {
        err = nanosleep(&delay, &delay);
    } while (err < 0 && errno == EINTR);
}



