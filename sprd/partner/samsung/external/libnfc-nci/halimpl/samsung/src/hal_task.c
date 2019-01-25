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
 *   Version: 2.0
 *
 */

#include <hardware/nfc.h>
#include <string.h>
#include <malloc.h>

#include "osi.h"
#include "hal.h"
#include "hal_msg.h"
#include "util.h"
#include "device.h"

#include <cutils/properties.h>

static void nfc_hal_state_switch(tNFC_HAL_MSG *msg, eHAL_STATE state)
{
    tNFC_HAL_MSG *new;

    new = (tNFC_HAL_MSG*)OSI_mem_get(HAL_EVT_SIZE);
    if (!new)
    {
        OSI_loge("Failed to memory allocate!");
        nfc_stack_cback(HAL_NFC_ERROR_EVT, HAL_NFC_STATUS_OK);
        return;
    }

    nfc_hal_info.state = state;
    memcpy(new, msg, sizeof(HAL_EVT_SIZE));
    OSI_queue_put(nfc_hal_info.msg_q, (void *)new);
}

void hal_sleep(void *param)
{
    device_sleep();
}

void hal_update_sleep_timer(void)
{
    device_wakeup();

    /* workaround for double timer */
    if (nfc_hal_info.flag & HAL_FLAG_MASK_USING_TIMER)
    {
        OSI_logt("double timer");
        return;
    }

    OSI_timer_start(nfc_hal_info.sleep_timer, nfc_hal_info.cfg.sleep_timeout,
            (tOSI_TIMER_CALLBACK) hal_sleep, NULL);
}

int __send_to_device(uint8_t* data, size_t len)
{
    hal_update_sleep_timer();
    if (nfc_hal_info.nci_last_pkt)
        memcpy((void *)nfc_hal_info.nci_last_pkt, (void *)data, len);

    return device_write(data, len);
}

/* START [H15052701] */
int check_force_fw_update_mode()
{
    uint8_t force_update_mode =0;
    char check_update[PROPERTY_VALUE_MAX] = {0};

    if( property_get("nfc.fw.downloadmode_force", check_update, "") )
        sscanf(check_update, "%lu", &force_update_mode);

    OSI_logd ("force_update_mode : %d", force_update_mode);

    return (uint8_t)force_update_mode;
}
/* END [H15052701] */

void nfc_hal_open_sm(tNFC_HAL_MSG *msg)
{
    tNFC_FW_PKT *fw_pkt = &msg->fw_packet;
    tNFC_HAL_FW_INFO *fw = &nfc_hal_info.fw_info;
    tNFC_HAL_FW_BL_INFO *bl = &fw->bl_info;
    size_t ret;

    switch (msg->event)
    {
    case HAL_EVT_OPEN:
#ifndef NFC_HAL_DO_NOT_USE_BOOTLOADER
        device_set_mode(NFC_DEV_MODE_BOOTLOADER);
        OSI_logd("Get Bootloader information.");
        ret = nfc_fw_send_cmd(FW_CMD_GET_BOOTINFO, NULL, 0);
        break;

    case HAL_EVT_READ:
        fw_read_payload(msg);
        if (fw_pkt->code != FW_RET_SUCCESS)
        {
            OSI_loge("Failed to get bootloader information. Return code: 0x%02X",
                    fw_pkt->code);
            nfc_stack_cback(HAL_NFC_OPEN_CPLT_EVT, HAL_NFC_STATUS_FAILED);
            break;
        }

        // store bootloader information from received data packet.
        memcpy(bl->version, fw_pkt->payload, sizeof(bl->version));
        FROM_LITTLE_ARRY(bl->sector_size, &fw_pkt->payload[4], 2);
        FROM_LITTLE_ARRY(bl->page_size, &fw_pkt->payload[6], 2);
        FROM_LITTLE_ARRY(bl->frame_max_size, &fw_pkt->payload[8], 2);
        FROM_LITTLE_ARRY(bl->hw_buffer_size, &fw_pkt->payload[10], 2);
        // search product information for using bootloader.
        bl->product = product_map(bl->version, &bl->base_address);
        OSI_logd("Bootloader: Version %02x.%02x.%02x.%02x(N%d), Sector size: %d, Page Size: %d",
                bl->version[0], bl->version[1], bl->version[2], bl->version[3],
                (bl->product >> 8), bl->sector_size, bl->page_size);

        if (bl->product != SNFC_UNKNOWN) {
            OSI_logd("bl->product 0x%03X!!: %s", bl->product, get_product_name(bl->product));
        }
        else
        {
            OSI_logd("F/W Direct Mode!");

            // Get Base Address from libnfc-sec-hal.conf file.
            if (get_config_int(cfg_name_table[CFG_FW_BASE_ADDRESS], &bl->base_address))
                OSI_logd("F/W Base Address : 0x%x", bl->base_address);
            else
            {
            msg->event = HAL_EVT_COMPLETE_FAILED;
            nfc_hal_state_switch(msg, HAL_STATE_OPEN);
                OSI_loge("Doesn't support 'F/W Direct Mode'.");
            break;
            }

        }

        if(SNFC_N8 <= bl->version[0]) {
            FROM_LITTLE_ARRY(bl->base_address, &fw_pkt->payload[12], 2);
        }
#ifndef FWU_APK
        hal_fw_get_image_path();
#endif
        ret = 0;
        if (get_config_int(cfg_name_table[CFG_UPDATE_MODE], (int *)&ret))
            OSI_logd("F/W Update mode is %d", ret);

        nfc_hal_info.state = HAL_STATE_POSTINIT;
        nfc_hal_info.flag &= ~HAL_FLAG_FORCE_FW_UPDATE;
/*force update for libnfc-sec-hal.conf */
// Start [S150831001] APIs for RF Tuning
        if( (ret == FW_UPDATE_BY_FORCE_VER) || (check_force_fw_update_mode()==1) )
// End [S150831001] APIs for RF Tuning
        {
            OSI_logd("NFC need to update FW!(force update)");
            fw_force_update(NULL);
            break;
        }
/*force update for libnfc-sec-hal.conf */
        device_set_mode(NFC_DEV_MODE_ON);

        if(bl->product < SNFC_N7)
        {
        hal_nci_send_reset();
        OSI_timer_start(nfc_hal_info.nci_timer, 1000,
                    (tOSI_TIMER_CALLBACK) fw_force_update, NULL);
        }
        else
        {
            nfc_hal_info.state = HAL_STATE_FW;
            nfc_hal_info.fw_info.state = FW_W4_NCI_PROP_FW_CFG;
            nfc_hal_info.flag |= HAL_FLAG_CLK_SET;

            /* Clock Rewrite support to N7. */
            hal_nci_send_prop_fw_cfg(bl->product);
            OSI_timer_start(nfc_hal_info.nci_timer, 1000,
                        (tOSI_TIMER_CALLBACK) fw_force_update, NULL);
        }
            break;

    case HAL_EVT_COMPLETE:
#endif
        device_set_mode(NFC_DEV_MODE_ON);
        nfc_hal_info.state = HAL_STATE_POSTINIT;
        nfc_stack_cback(HAL_NFC_OPEN_CPLT_EVT, HAL_NFC_STATUS_OK);
        break;

    case HAL_EVT_COMPLETE_FAILED:
        device_set_mode(NFC_DEV_MODE_OFF);
        nfc_stack_cback(HAL_NFC_OPEN_CPLT_EVT, HAL_NFC_STATUS_FAILED);
        break;

    case HAL_EVT_TERMINATE:
        //TODO: terminate
        break;
    default:
        break;
    }
}

void nfc_hal_postinit_sm(tNFC_HAL_MSG *msg)
{
    tNFC_NCI_PKT *pkt = &msg->nci_packet;
    tNFC_HAL_FW_INFO *fw = &nfc_hal_info.fw_info;
    tNFC_HAL_FW_BL_INFO *bl = &fw->bl_info;

    switch (msg->event)
    {
    case HAL_EVT_CORE_INIT:
#ifndef NFC_HAL_DO_NOT_USE_BOOTLOADER
        if (!(nfc_hal_info.flag & HAL_FLAG_FORCE_FW_UPDATE))
        {
            if (nfc_fw_getver_from_image(fw, nfc_hal_info.cfg.fw_file))
            {
                memcpy(&fw->fw_ver, NCI_MF_INFO(pkt), NCI_MF_INFO_SIZE);
                if (nfc_fw_force_update_check(fw))
                {
                    OSI_logd("NFC need to update!!");

                    nfc_hal_info.fw_info.state = FW_ENTER_UPDATE_MODE;
                    nfc_hal_state_switch(msg, HAL_STATE_FW);
                    break;
                }
                OSI_logd("Current F/W does not need to update!");
            }
        }
#endif

        nfc_hal_info.vs_info.state = VS_INIT;
        nfc_hal_state_switch(msg, HAL_STATE_VS);
        break;

    case HAL_EVT_WRITE:
        if (NCI_GID(pkt) == NCI_GID_CORE)
        {
            if (NCI_OID(pkt) == NCI_CORE_RESET)
            {
                if (nfc_hal_info.flag & HAL_FLAG_ALREADY_RESET)
                    goto complete;

                nfc_hal_info.flag |= HAL_FLAG_W4_CORE_RESET_RSP;
                OSI_timer_start(nfc_hal_info.nci_timer, 1000,
                            (tOSI_TIMER_CALLBACK) fw_force_update, NULL);
                OSI_logd("set flag to 0x%06X", nfc_hal_info.flag);
            }
            else if (NCI_OID(pkt) == NCI_CORE_INIT)
            {
                if (nfc_hal_info.flag & HAL_FLAG_ALREADY_INIT)
                    goto complete;

                nfc_hal_info.flag |= HAL_FLAG_W4_CORE_INIT_RSP;
                OSI_timer_start(nfc_hal_info.nci_timer, 1000,
                            (tOSI_TIMER_CALLBACK) nci_init_timeout, NULL);
                OSI_logd("set flag to 0x%06X", nfc_hal_info.flag);
            }
        }
        hal_nci_send(&msg->nci_packet);
        break;

    case HAL_EVT_READ:
        nci_read_payload(msg);
        if (NCI_GID(pkt) == NCI_GID_CORE)
        {
            if (NCI_OID(pkt) == NCI_CORE_RESET)
            {
                OSI_logd("Respond CORE_RESET_RSP");
                nfc_hal_info.flag &= ~HAL_FLAG_W4_CORE_RESET_RSP;
                nfc_hal_info.flag |= HAL_FLAG_ALREADY_RESET;
            }
            else if (NCI_OID(pkt) == NCI_CORE_INIT)
            {
                OSI_logd("Respond CORE_INIT_RSP");
                nfc_hal_info.flag &= ~HAL_FLAG_W4_CORE_INIT_RSP;
                nfc_hal_info.flag |= HAL_FLAG_ALREADY_INIT;
            }
            OSI_timer_stop(nfc_hal_info.nci_timer);
        }
        util_nci_analyzer(pkt);
        nfc_data_callback(&msg->nci_packet);
        break;

    case HAL_EVT_COMPLETE:
    complete:
        nfc_hal_info.flag |= HAL_FLAG_NTF_TRNS_ERROR | HAL_FLAG_RETRY_TRNS;
        nfc_hal_info.state = HAL_STATE_SERVICE;
        nfc_stack_cback(HAL_NFC_POST_INIT_CPLT_EVT, HAL_NFC_STATUS_OK);
        break;
    case HAL_EVT_COMPLETE_FAILED:
        nfc_stack_cback(HAL_NFC_POST_INIT_CPLT_EVT, HAL_NFC_STATUS_FAILED);
        break;

    case HAL_EVT_TERMINATE:
        //TODO: terminate
        break;
    default:
        break;
    }
}

/* START [H15052702] */
static void nfc_hal_fw_send_fw_status(unsigned char status)
{
    tNFC_NCI_PKT fw_status;

    fw_status.oct0 = 0x6f;
    fw_status.oid = 0xFF;
    fw_status.len = 1;
    fw_status.payload[0] = status;
    OSI_loge("nfc_hal_fw_send_fw_status, status: 0x%02X", status);
    nfc_data_callback(&fw_status);
}
static void nfc_hal_fw_timeout(void *param)
{
    OSI_loge("FW Update fail!!");
    nfc_hal_fw_send_fw_status(FW_UPDATE_STATUS_ERROR);
}
/* END [H15052702] */

void nfc_hal_fw_sm(tNFC_HAL_MSG *msg)
{
    tNFC_FW_PKT *fw_pkt = &msg->fw_packet;
    tNFC_NCI_PKT *pkt = &msg->nci_packet;
    tNFC_HAL_FW_INFO *fw = &nfc_hal_info.fw_info;
    tNFC_HAL_FW_IMAGE_INFO *img = &fw->image_info;
    tNFC_HAL_FW_BL_INFO *bl = &fw->bl_info;

    uint8_t param[10] = {0x00};
    unsigned int len;

    switch (msg->event)
    {
    case HAL_EVT_CORE_INIT:
        device_set_mode(NFC_DEV_MODE_BOOTLOADER);

    case HAL_EVT_READ:
        OSI_timer_stop(nfc_hal_info.nci_timer);    /* [H16011102] */

        if (fw->state & FW_W4_FW_RSP)
        {
            OSI_timer_start(nfc_hal_info.nci_timer, 5000,
                    (tOSI_TIMER_CALLBACK) nfc_hal_fw_timeout, NULL);    /* [H16011102] */
            fw_read_payload(msg);
        }
        if (fw->state & FW_W4_NCI_RSP)
        {
            nci_read_payload(msg);
            util_nci_analyzer(pkt);
        }

        // responsed succeess?
        if (fw->state & FW_W4_FW_RSP && fw_pkt->code != FW_RET_SUCCESS)
        {
            OSI_loge("F/W bootloader mode failed. FW state: %d, Return code: 0x%02X",
                    fw->state, fw_pkt->code);
            if (fw->image_info.signature) {
                free(fw->image_info.signature);
                free(fw->image_info.rawData);
                free(fw->image_info.hashCode);

                nfc_hal_fw_send_fw_status(FW_UPDATE_STATUS_ERROR);    /* [H15052702] */
            }
            break;
        }

        switch (fw->state)
        {
        case FW_ENTER_UPDATE_MODE:
            if (nfc_fw_getsecinfo_from_image(fw, nfc_hal_info.cfg.fw_file, bl->version[NFC_HAL_BL_VER_KEYINFO]))
            {
                TO_LITTLE_ARRY(&param[0], img->hashCodeLen, 2);
                TO_LITTLE_ARRY(&param[2], img->signatureLen, 2);
                nfc_fw_send_cmd(FW_CMD_ENTER_UPDATEMODE, param, 4);
                fw->state = FW_W4_ENTER_UPDATEMODE_RSP;
            }
            else
            {
                OSI_loge("failed to get secure information from F/W image");
                // TODO: error handle
            }
            break;

        case FW_W4_ENTER_UPDATEMODE_RSP:
            OSI_logd("Enter F/W Updating mode");
            nfc_hal_fw_send_fw_status(FW_UPDATE_STATUS_START);    /* [H15052702] */
            OSI_logd("HASH check...");
            nfc_fw_send_data(img->hashCode, img->hashCodeLen);
            fw->state = FW_W4_HASH_DATA_RSP;
            break;

        case FW_W4_HASH_DATA_RSP:
            OSI_logd("HASH checking is succeed");

            OSI_logd("signature check...");
            nfc_fw_send_data(img->signature, img->signatureLen);
            fw->state = FW_W4_SIGN_DATA_RSP;
            break;

        case FW_W4_SIGN_DATA_RSP:
            OSI_logd("signature checking is succeed");

            img->cur = 0;
            fw->target_sector = 0;
            TO_LITTLE_ARRY(param, bl->base_address, 4);
            OSI_logd("sector update");
            nfc_fw_send_cmd(FW_CMD_UPDATE_SECT, param, 4);
            fw->state = FW_W4_UPDATE_SECT_RSP;
            break;

        case FW_W4_UPDATE_SECT_RSP:
            OSI_logd("%d sector is selected", fw->target_sector);

            len = nfc_fw_cal_next_frame_length(fw, FW_DATA_PAYLOAD_MAX);
            img->cur += nfc_fw_send_data(img->rawData + img->cur, len);
            fw->state = FW_W4_FW_DATA_RSP;
            break;

        case FW_W4_FW_DATA_RSP:
            if (img->cur == img->rawDataLen)
            // complete
            {
                nfc_fw_send_cmd(FW_CMD_COMPLETE_UPDATE_MODE, NULL, 0);
                fw->state = FW_W4_COMPLETE_RSP;
            }
            else if (img->cur < (unsigned int)((fw->target_sector+1)*bl->sector_size))
            // next data
            {
                len = nfc_fw_cal_next_frame_length(fw, FW_DATA_PAYLOAD_MAX);
                img->cur += nfc_fw_send_data(img->rawData + img->cur, len);
            }
            else
            // next sector
            {
                fw->target_sector++;
                TO_LITTLE_ARRY(param,
                        bl->base_address+(fw->target_sector*bl->sector_size), 4);
                OSI_logd("sector update");
                nfc_fw_send_cmd(FW_CMD_UPDATE_SECT, param, 4);
                fw->state = FW_W4_UPDATE_SECT_RSP;

                OSI_logd("F/W Updating...%6d/%6d (%3.0f%%)",
                        img->cur, (int)img->rawDataLen,
                        ((double)img->cur/(double)img->rawDataLen*100));

            }
            break;

        case FW_W4_COMPLETE_RSP:
            OSI_logd("F/W Update complete");
            free(fw->image_info.signature);
            free(fw->image_info.rawData);
            free(fw->image_info.hashCode);
            device_set_mode(NFC_DEV_MODE_OFF);

            device_set_mode(NFC_DEV_MODE_ON);
            nfc_hal_fw_send_fw_status(FW_UPDATE_STATUS_COMPLETED);
            OSI_logd("Get RF clock source type from CFG file and send it!");
            hal_nci_send_prop_fw_cfg(bl->product);
            fw->state = FW_W4_NCI_PROP_FW_CFG;
            break;

        case FW_W4_NCI_PROP_FW_CFG:
            if (NCI_MT(pkt) != NCI_MT_RSP
                || NCI_GID(pkt) != NCI_GID_PROP
                || NCI_OID(pkt) != NCI_PROP_FW_CFG)
            {
                OSI_logd("Not matched rsponse!! we expect NCI_PROP_FW_CFG_RSP");
            }
            else
            {
                if (NCI_STATUS(pkt) != NCI_STATUS_OK
                    && NCI_STATUS(pkt) != NCI_STATUS_E_SYNTAX
                    && NCI_STATUS(pkt) != NCI_CLOCK_STATUS_SYNTAX_ERROR
                    && NCI_STATUS(pkt) != NCI_CLOCK_STATUS_MISMATCHED
                    && NCI_STATUS(pkt) != NCI_CLOCK_STATUS_FULL)
                {
                    OSI_loge("Failed to config FW, status: %d", NCI_STATUS(pkt));
                    break;
                }
                else
                {
                    switch(NCI_STATUS(pkt))
                    {
                        case NCI_STATUS_OK:
                            OSI_logd("RF clock setup success!!");
                            break;

                        case NCI_STATUS_E_SYNTAX:
                    OSI_logd("This version of bootloader does not support FW_CFG!!");
                    OSI_logd("  you should check your RF clock source by yourself");
                            nfc_hal_fw_send_fw_status(FW_UPDATE_STATUS_ERROR);    /* [H15052702] */
                            break;

                        case NCI_CLOCK_STATUS_SYNTAX_ERROR:
                            OSI_loge("Command Length Error!");
                            break;

                        case NCI_CLOCK_STATUS_MISMATCHED:
                            OSI_loge("Clock Mismatch!");
                            break;

                        case NCI_CLOCK_STATUS_FULL:
                            OSI_loge("Please, Check the Clock Value. Current Clock Val : %d", pkt->payload[1]);
                            break;
                    }
                }

                if( bl->product >= SNFC_N7 )
                {
                    if(NCI_STATUS(pkt) == NCI_CLOCK_STATUS_FULL)
                    {
                        nfc_hal_state_switch(msg, HAL_STATE_FW);
                        fw_force_update(NULL);
                        break;
                    }
                    else if(NCI_STATUS(pkt) == NCI_CLOCK_STATUS_MISMATCHED)
                    {
                        device_set_mode(NFC_DEV_MODE_OFF);
                        device_set_mode(NFC_DEV_MODE_ON);

                            nfc_hal_state_switch(msg, HAL_STATE_FW);
                        fw->state = FW_W4_NCI_PROP_FW_CFG;
                        hal_nci_send_prop_fw_cfg(bl->product);
                            break;
                    }

                    if( (nfc_hal_info.flag & HAL_FLAG_CLK_SET)
                     || ((nfc_hal_info.flag & HAL_FLAG_FORCE_FW_UPDATE)
                         && !(nfc_hal_info.flag & (HAL_FLAG_W4_CORE_RESET_RSP|HAL_FLAG_W4_CORE_INIT_RSP))) )
                  {
                        msg->event = HAL_EVT_COMPLETE;
                        nfc_hal_state_switch(msg, HAL_STATE_OPEN);
                        nfc_hal_info.flag &= ~HAL_FLAG_CLK_SET;
                }
                else
                {
                        hal_nci_send_reset();
                        fw->state = FW_W4_NCI_RESET_RSP;
                    }
                    break;
                }

                if ((nfc_hal_info.flag & HAL_FLAG_FORCE_FW_UPDATE)
                    && !(nfc_hal_info.flag & (HAL_FLAG_W4_CORE_RESET_RSP|HAL_FLAG_W4_CORE_INIT_RSP)))
                {   /* F/W is updated before sending CORE_RESET_CMD; go back to open sm*/
                    msg->event = HAL_EVT_COMPLETE;
                    nfc_hal_state_switch(msg, HAL_STATE_OPEN);
                }
                else
                {
                    /* F/W is updated after sending CORE_RESET_CMD */
                    hal_nci_send_reset();
                    fw->state = FW_W4_NCI_RESET_RSP;
                }
            }
            break;

        case FW_W4_NCI_RESET_RSP:
            if (NCI_MT(pkt) != NCI_MT_RSP
                || NCI_GID(pkt) != NCI_GID_CORE
                || NCI_OID(pkt) != NCI_CORE_RESET
                || NCI_STATUS(pkt) != NCI_STATUS_OK)
            {
                OSI_logd("Failed NCI CORE_RESET after F/W updating.");
                nfc_stack_cback(HAL_NFC_POST_INIT_CPLT_EVT, HAL_NFC_STATUS_FAILED);
            }
            else
            {
                OSI_logd("OK! NCI CORE_RESET after F/W updating.");
                nfc_hal_info.flag &= ~HAL_FLAG_W4_CORE_RESET_RSP;

                if (nfc_hal_info.flag & HAL_FLAG_W4_CORE_RESET_RSP)
                {   /* F/W is updated after sending CORE_RESET_CMD */
                    nfc_hal_info.state = HAL_STATE_POSTINIT;
                    nfc_data_callback(pkt);
                    break;
                }

                OSI_timer_stop(nfc_hal_info.nci_timer);
                fw->state = FW_W4_NCI_INIT_RSP;
                OSI_timer_start(nfc_hal_info.nci_timer, 1000,
                            (tOSI_TIMER_CALLBACK) nci_init_timeout, NULL);
                hal_nci_send_init();
            }
            break;

        case FW_W4_NCI_INIT_RSP:
            if (NCI_MT(pkt) != NCI_MT_RSP
                || NCI_GID(pkt) != NCI_GID_CORE
                || NCI_OID(pkt) != NCI_CORE_INIT
                || NCI_STATUS(pkt) != NCI_STATUS_OK)
            {
                OSI_logd("Failed NCI CORE_INIT after F/W updating.");
                nfc_stack_cback(HAL_NFC_POST_INIT_CPLT_EVT, HAL_NFC_STATUS_FAILED);
                break;
            }

            nfc_hal_info.flag &= ~HAL_FLAG_W4_CORE_INIT_RSP;
            OSI_logd("OK! NCI CORE_INIT after F/W updating.");

            nfc_data_callback(pkt);
            if (nfc_hal_info.flag & HAL_FLAG_W4_CORE_INIT_RSP)
            {   /* F/W is updated after CORE_INIT_CMD */
                nfc_hal_info.state = HAL_STATE_POSTINIT;
                break;
            }

            /* F/W is updated after CORE_INIT_RSP */
            fw->state = 0;
            nfc_hal_info.vs_info.state = VS_INIT;
            nfc_hal_state_switch(msg, HAL_STATE_VS);
            break;
        }
        break;
    default:
        OSI_loge("Unexpected event [%d]", msg->event);
        break;
    }
}

void nfc_hal_vs_sm(tNFC_HAL_MSG *msg)
{
    tNFC_HAL_VS_INFO *vs = &nfc_hal_info.vs_info;
    tNFC_HAL_FW_BL_INFO *bl = &nfc_hal_info.fw_info.bl_info;
    tNFC_NCI_PKT *pkt = &msg->nci_packet;
    tNFC_NCI_PKT vs_pkt;
    uint8_t check_sum[2];
    int size;
/* START [20150604] RF Rewrite */
    static int count = 0;
/* END [20150604] RF Rewrite */

// Start [S15083101] APIs for RF Tuning
    size_t ret = 0;
// End [S15083101] APIs for RF Tuning

    if (msg->event != HAL_EVT_READ
        && msg->event != HAL_EVT_CORE_INIT)
    {
        OSI_loge("Unexpected event [%d]", msg->event);
        return;
    }

    if (vs->state != VS_INIT)
    {
        nci_read_payload(msg);
        util_nci_analyzer(pkt);
    }

    switch (vs->state)
    {
    case VS_INIT:
        vs->rfregid = 0;
        vs->propid = 0;
        vs->propcnt = get_config_count(cfg_name_table[CFG_NCI_PROP]);
        OSI_logd("Configure file has %d NCI_PROP", vs->propcnt);

        if (hal_vs_is_rfreg_file(vs)) {
            OSI_logd("Configure file has rfreg file: %s", vs->rfreg_file);
            OSI_logd("Get RF register version.");
            if(SNFC_N8 <= bl->version[0])
                hal_vs_nci_send(NCI_PROP_GET_OPTION_META_OID, NULL, 0);
            else
                hal_vs_nci_send(NCI_PROP_GET_RFREG_VER, NULL, 0);
            vs->check_sum = 0;
            vs->state = VS_W4_GET_RFREG_VER_RSP;
            break;
        }

        /* If solution has RF register file, HAL does not use hal configuration file,
           but it does not has the file, HAL checks hal configruation file to set RF reg */
        vs->rfregcnt = get_config_count(cfg_name_table[CFG_RF_REG]);
        OSI_logd("Configure file has %d RF reg config", vs->rfregcnt);
        vs->state = VS_W4_CFG_RFREG_RSP;
        goto vs_send_rfreg_from_cfg;

    /* start from rf register file */
    case VS_W4_GET_RFREG_VER_RSP:
        if (NCI_MT(pkt) != NCI_MT_RSP
            || NCI_GID(pkt) != NCI_GID_PROP
            || NCI_STATUS(pkt) != NCI_STATUS_OK)
        {
            if (NCI_STATUS(pkt) == NCI_STATUS_E_SYNTAX)
            {
                // F/W does not support RF reg setting.
                OSI_loge("Failed NCI PROP_GET_RFREG_VER (SYNTAX error)");
                goto vs_send_prop;
            }
            else if(NCI_OID(pkt) != NCI_PROP_GET_OPTION_META_OID)
                OSI_loge("Failed NCI PROP_GET_RFREG_VER (OID error)");
        }

        OSI_logd("Got RF register version");

// Start [S15083101] APIs for RF Tuning
        ret = 0;
        if (get_config_int(cfg_name_table[CFG_UPDATE_MODE], (int *)&ret))
            OSI_logd("F/W Update mode is %d", ret);
		
            if(ret == 0x03)
	    {
                OSI_logd("Update RF register file.");
		hal_vs_nci_send(NCI_PROP_START_RFREG, NULL, 0);
		vs->state = VS_W4_START_RFREG_RSP;
		break;		
	    }
// END [S15083101] APIs for RF Tuning

        if (!hal_vs_check_rfreg_update(vs, pkt))
        {
            OSI_logd("Do not need to update RF registers.");
            goto vs_send_prop;
        }

        OSI_logd("Start setting RF register!!");
        hal_vs_nci_send(NCI_PROP_START_RFREG, NULL, 0);
        vs->state = VS_W4_START_RFREG_RSP;
        break;

    case VS_W4_START_RFREG_RSP:
        if (NCI_MT(pkt) != NCI_MT_RSP
            || NCI_GID(pkt) != NCI_GID_PROP
            || NCI_OID(pkt) != NCI_PROP_START_RFREG
            || NCI_STATUS(pkt) != NCI_STATUS_OK)
        {
            OSI_loge("Failed to start RF register!");
            goto vs_send_prop;
        }
        vs->state = VS_W4_FILE_RFREG_RSP;

    case VS_W4_FILE_RFREG_RSP:
        if (NCI_MT(pkt) != NCI_MT_RSP
            || NCI_GID(pkt) != NCI_GID_PROP
            || NCI_OID(pkt) != NCI_PROP_SET_RFREG
            || NCI_STATUS(pkt) != NCI_STATUS_OK)
        {
            if (NCI_OID(pkt) != NCI_PROP_START_RFREG)
            {
                OSI_loge("Failed to set RF reg id: %d!", vs->rfregid-1);
                hal_vs_nci_send(NCI_PROP_STOP_RFREG, NULL, 0);
                vs->state = VS_W4_STOP_RFREG_RSP;
                break;
            }
        }

        if (hal_vs_rfreg_update(vs)) //is continue?
            break;

        OSI_logd("Set RF register version");
        hal_vs_nci_send(NCI_PROP_SET_RFREG_VER, vs->rfreg_version, 8);
        vs->state = VS_W4_SET_RFREG_VER_RSP;
        break;

    case VS_W4_SET_RFREG_VER_RSP:
        if (NCI_MT(pkt) != NCI_MT_RSP
            || NCI_GID(pkt) != NCI_GID_PROP
            || NCI_OID(pkt) != NCI_PROP_SET_RFREG_VER
            || NCI_STATUS(pkt) != NCI_STATUS_OK)
        {
            OSI_loge("Failed to set RF version!");
        }

        OSI_logd("Stop setting RF register!");
        OSI_logd("RF register check sum is 0x%04X", vs->check_sum&0xFFFF);
        TO_LITTLE_ARRY(check_sum, vs->check_sum&0xFFFF, 2);
        hal_vs_nci_send(NCI_PROP_STOP_RFREG, check_sum, 2);
        vs->state = VS_W4_STOP_RFREG_RSP;
        break;

    case VS_W4_STOP_RFREG_RSP:
/* START [20150604] RF Rewrite */
        if (NCI_STATUS(pkt) != NCI_STATUS_OK
            || NCI_MT(pkt) != NCI_MT_RSP
            || NCI_GID(pkt) != NCI_GID_PROP
            || NCI_OID(pkt) != NCI_PROP_STOP_RFREG)
/* END [20150604] RF Rewrite */
        {
            OSI_loge("Failed to stop RF register!");
/* START [20150604] RF Rewrite */
            if (count < 2) {
                vs->rfregid = 0;
                vs->check_sum = 0;
                OSI_logd("Restart setting RF register!!");
                hal_vs_nci_send(NCI_PROP_START_RFREG, NULL, 0);
                vs->state = VS_W4_START_RFREG_RSP;
                count++;
                break;
            }
/* END [20150604] RF Rewrite */

        }
        else
        {
            OSI_logd("Success RF register set!!");
        }
        goto vs_send_prop;
    /* end from rf register file */

    case VS_W4_CFG_RFREG_RSP:
        if (NCI_MT(pkt) != NCI_MT_RSP
            || NCI_GID(pkt) != NCI_GID_PROP
            || NCI_OID(pkt) != NCI_PROP_SET_RFREG
            || NCI_STATUS(pkt) != NCI_STATUS_OK)
        {
            OSI_loge("Failed NCI PROP_RFREG id: %d", vs->rfregid);
            // It isn't critical
        }

        OSI_logd("RF %d is setted", vs->rfregid);

    vs_send_rfreg_from_cfg:
        if (vs->rfregcnt > vs->rfregid) {
            vs->rfregid++;
            size = nfc_hal_vs_get_rfreg(vs->rfregid, &vs_pkt);
            if (size > 0 )
            {
                OSI_logd("send rf register (id:%d)", vs->rfregid);
                hal_nci_send(&vs_pkt);
                break;
            }
        }
        goto vs_send_prop;

    case VS_W4_PROP_RSP:
        if (NCI_MT(pkt) != NCI_MT_RSP
            || NCI_GID(pkt) != NCI_GID_PROP
            || NCI_OID(pkt) != vs->lastoid
            || NCI_STATUS(pkt) != NCI_STATUS_OK)
        {
            OSI_loge("Failed NCI PROP(%d) OID: 0x%02X, response: %02X",
                vs->propid, (unsigned int)vs->lastoid, (unsigned int)NCI_STATUS(pkt));
            // It isn't critical
        }

        vs->propid++;

    vs_send_prop:
        if (vs->propcnt > vs->propid)
        {
            memset(&vs_pkt, 0, sizeof(tNFC_NCI_PKT));
            size = hal_vs_get_prop(vs->propid, &vs_pkt);
            vs->lastoid = NCI_OID(&vs_pkt);
            hal_nci_send(&vs_pkt);
            vs->state = VS_W4_PROP_RSP;
            break;
        }

        /* Workaround: Initialization flash of LMRT
         * TODO: Only for version 2.2.4
         */
        {
            hal_nci_send_clearLmrt();
            vs->state = VS_W4_COMPLETE;
            break;
        }
        /* End WA */

    case VS_W4_COMPLETE:
        OSI_logd("Vendor Specific is complete.");
        msg->event = HAL_EVT_COMPLETE;
        nfc_hal_state_switch(msg, HAL_STATE_POSTINIT);
        break;
    default:
        OSI_loge("Unexpected event [%d]", msg->event);
        break;
    }
}

void nfc_hal_service_sm(tNFC_HAL_MSG *msg)
{
    tNFC_NCI_PKT *pkt = &msg->nci_packet;
    tNFC_NCI_PKT *grant_pkt;

    switch (msg->event)
    {
    case HAL_EVT_WRITE:
        if (nfc_hal_prehandler(pkt))
        hal_nci_send(pkt);
        break;
    case HAL_EVT_READ:
        nci_read_payload(msg);
        util_nci_analyzer(pkt);
        hal_update_sleep_timer();
        if (nfc_hal_prehandler(pkt))
            nfc_data_callback(pkt);
        break;
    case HAL_EVT_CONTROL_GRANTED:
        nfc_hal_state_switch(msg, HAL_STATE_GRANTED);
        break;
    case HAL_EVT_TERMINATE:
        //TODO: terminate
        break;
    default:
        break;
    }
}

static void nfc_hal_grant_finish(void)
{
    nfc_stack_cback(HAL_NFC_RELEASE_CONTROL_EVT, HAL_NFC_STATUS_OK);
    nfc_hal_info.state = HAL_STATE_SERVICE;
    nfc_hal_info.grant_cback = NULL;
}

void nfc_hal_grant_sm(tNFC_HAL_MSG *msg)
{
    tNFC_NCI_PKT *pkt = &msg->nci_packet;
    uint8_t cback_ret = HAL_GRANT_FINISH;

    /* Granted mode is not need to SLEEP.
     * hal should pend granted mode just few time */
    switch (msg->event)
    {
    case HAL_EVT_READ:
        nci_read_payload(msg);
        util_nci_analyzer(pkt);
        cback_ret = nfc_hal_info.grant_cback(pkt);
        if (cback_ret == HAL_GRANT_FINISH)
            nfc_hal_grant_finish();

        if (cback_ret != HAL_GRANT_SEND_NEXT)
            break;

    case HAL_EVT_CONTROL_GRANTED:
        pkt = (tNFC_NCI_PKT *)OSI_queue_get(nfc_hal_info.nci_q);
        if (pkt)
        {
            // TODO: Should CLF respond?
            hal_nci_send(pkt);
            OSI_mem_free((tOSI_MEM_HANDLER)pkt);
        }
        else
            nfc_hal_grant_finish();

        break;

    case HAL_EVT_WRITE:
        OSI_loge("HAL is in granted mode!");
        break;
    }
}

/* TASK */
void nfc_hal_task(void)
{
    tNFC_HAL_MSG *msg;
    eHAL_STATE old_st;

    OSI_logt("enter!");

    if (!nfc_hal_info.msg_task || !nfc_hal_info.nci_timer
            || !nfc_hal_info.msg_q || !nfc_hal_info.nci_q)
    {
        OSI_loge("msg_task = %d, nci_timer = %d, msg_q = %d, nci_q = %d", 
        nfc_hal_info.msg_task, nfc_hal_info.nci_timer, nfc_hal_info.msg_q, nfc_hal_info.nci_q);

        nfc_hal_deinit();
        OSI_loge("nfc_hal initialization is not succeeded.");
        nfc_stack_cback(HAL_NFC_ERROR_EVT, HAL_NFC_STATUS_FAILED);
        return;
    }

    while (OSI_task_isRun(nfc_hal_info.msg_task) == OSI_RUN)
    {
        msg = (tNFC_HAL_MSG *)OSI_queue_get_wait(nfc_hal_info.msg_q);
        if (!msg) continue;

        OSI_logd("Got a event: %s(%d)", event_to_string(msg->event), msg->event);
        if (msg->event == HAL_EVT_TERMINATE)
            break;

        OSI_logd("current state: %s", state_to_string(nfc_hal_info.state));
        old_st = nfc_hal_info.state;
        switch (nfc_hal_info.state)
        {
            case HAL_STATE_INIT:
            case HAL_STATE_DEINIT:
            case HAL_STATE_OPEN:
                nfc_hal_open_sm(msg);
                break;
            case HAL_STATE_FW:
                nfc_hal_fw_sm(msg);
                break;
            case HAL_STATE_VS:
                nfc_hal_vs_sm(msg);
                break;
            case HAL_STATE_POSTINIT:
                nfc_hal_postinit_sm(msg);
                break;
            case HAL_STATE_SERVICE:
                nfc_hal_service_sm(msg);
                break;
            case HAL_STATE_GRANTED:
                nfc_hal_grant_sm(msg);
                break;
            default:
                break;
        }
        OSI_mem_free((tOSI_MEM_HANDLER)msg);

        if (old_st != nfc_hal_info.state)
        {
            OSI_logd("hal state is changed: %s -> %s",
                state_to_string(old_st), state_to_string(nfc_hal_info.state));
        }
    }
    OSI_logt("exit!");
}

/* Print */
char *event_to_string(uint8_t event)
{
    switch (event)
    {
    case HAL_EVT_OPEN:
        return "HAL_EVT_OPEN";
    case HAL_EVT_CORE_INIT:
        return "HAL_EVT_CORE_INIT";
    case HAL_EVT_WRITE:
        return "HAL_EVT_WRITE";
    case HAL_EVT_READ:
        return "HAL_EVT_READ";
    case HAL_EVT_CONTROL_GRANTED:
        return "HAL_EVT_CONTROL_GRANTED";
    case HAL_EVT_TERMINATE:
        return "NFC_HAL_TERMINATE";
    case HAL_EVT_COMPLETE:
        return "NFC_HAL_COMPLETE";
    case HAL_EVT_COMPLETE_FAILED:
        return "NFC_HAL_COMPLETE_FAILED";
    }
    return "Uknown event.";
}

char *state_to_string(eHAL_STATE state)
{
    switch (state)
    {
    case HAL_STATE_INIT:
        return "INIT";
    case HAL_STATE_DEINIT:
        return "DEINIT";
    case HAL_STATE_OPEN:
        return "OPEN";
    case HAL_STATE_FW:
        return "FW_UPDATE";
    case HAL_STATE_VS:
        return "VENDOR_SPECIFIC";
    case HAL_STATE_POSTINIT:
        return "POST_INIT";
    case HAL_STATE_SERVICE:
        return "SERVICE";
    case HAL_STATE_GRANTED:
        return "GRANT";
    }
    return "Uknown state.";
}
