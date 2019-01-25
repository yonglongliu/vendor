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

#include <hardware/nfc.h>
#include <openssl/sha.h>
//[START] WR for EVT_Transaction to under FW V1.1.13
#include <unistd.h>
//[END] WR for EVT_Transaction to under FW V1.1.13
#include <string.h>
#include <malloc.h>

#include "osi.h"
#include "hal.h"
#include "hal_msg.h"
#include "util.h"
#include "device.h"
#include "product.h"

size_t __send_to_bootloader(eNFC_FW_BLTYPE type, uint8_t code,
                                        uint8_t *data, size_t len)
{
    tNFC_FW_PKT pkt;
    size_t ret;

    if (len > FW_DATA_PAYLOAD_MAX)
    {
        OSI_loge("Large paramLen (MAX:%d, Request:%d)", FW_DATA_PAYLOAD_MAX, len);
        return -1;
    }

    pkt.type = nfc_hal_info.fw_info.seq_no | type;
    nfc_hal_info.fw_info.seq_no ^= 0x80;
    pkt.code = code;
    pkt.len = (uint16_t)len;   // 2bytes, little endian

    memcpy(FW_PAYLOAD(&pkt), data, len);
    ret = (size_t)device_write((uint8_t*)&pkt, FW_HDR_SIZE + len);
    if (ret != FW_HDR_SIZE + len)
        OSI_loge("Failed to send header");

    return ret;
}

size_t nfc_fw_send_cmd(eNFC_FW_BLCMD cmd, uint8_t *param, size_t paramLen)
{
    return __send_to_bootloader(FW_MSG_CMD, cmd, param, paramLen) - FW_HDR_SIZE;
}

int nfc_fw_send_data(uint8_t *data, int len)
{
    return __send_to_bootloader(FW_MSG_DATA, 0, data, len) - FW_HDR_SIZE;
}

int fw_read_payload(tNFC_HAL_MSG *msg)
{
    tNFC_FW_PKT *fw_pkt = &msg->fw_packet;
    uint8_t len_high;
    int ret;

    // Read high length byte
    ret = device_read(&len_high, 1);
    if (ret != 1)
    {
        OSI_mem_free((tOSI_MEM_HANDLER)msg);
        OSI_loge("Failed to read length(high) of bootloader cmd");
        return ret;
    }
    fw_pkt->len += len_high << 8;

    // Read payload
    ret = device_read(fw_pkt->payload, fw_pkt->len);
    if (ret != (int)fw_pkt->len)
    {
        OSI_mem_free((tOSI_MEM_HANDLER)msg);
        OSI_loge("Failed to read payload");
        return ret;
    }

    data_trace("Recv", NCI_HDR_SIZE + 1 + ret, msg->param);
    return ret;
}

int nfc_fw_cal_next_frame_length(tNFC_HAL_FW_INFO *fw, int max)
{
    int len;
    int sector = fw->target_sector;
    int size = (int)fw->bl_info.sector_size;
    unsigned int cur = fw->image_info.cur;

    len = ((sector + 1) * size) - cur;

    if (len < 0) return len;
    return (len > max) ? max : len;
}

bool nfc_fw_getver_from_image(tNFC_HAL_FW_INFO *fw, char *file_name)
{
    FILE *file;
    unsigned int tmp;

    OSI_logt("!");

    if ((file = fopen(file_name, "rb")) == NULL)
    {
        OSI_loge("Failed to open F/W file (file: %s", file_name);
        return false;
    }

    // File size check
    fseek(file, 0, SEEK_END);
    tmp = (unsigned int)ftell(file);
    if (tmp < 24)   // TODO: FILE header size
    {
        OSI_loge("Invalid file format!!");
        fclose(file);
        return false;
    }

    // version 0x0C
    fseek(file, 0x10, SEEK_SET);
    fread(&fw->image_ver, 4, 1, file);
    fclose(file);

    return true;
}

bool nfc_fw_getsecinfo_from_image(tNFC_HAL_FW_INFO *fw, char *file_name, uint8_t keyType)
{
    tNFC_HAL_FW_IMAGE_INFO *img = &fw->image_info;
    FILE *file;
    struct {
        uint8_t release_ver[12];
        uint8_t fw_ver[8];
        uint32_t sig_addr;
        uint32_t sig_size;
        uint32_t img_addr;
        uint32_t sector_num;
        uint32_t custom_sig_addr;
        uint32_t custom_sig_size;
    } header;
    double dataLen;

    OSI_logt("!");

    if ((file = fopen(file_name, "rb")) == NULL)
    {
        OSI_loge("Failed to open F/W file (file: %s)", file_name);
        return false;
    }

    fseek(file, 0, SEEK_SET);
    fread(&header, sizeof(header), 1, file);

    if (keyType == 0) // General key
        img->signatureLen = header.sig_size;
    else // Customer key
        img->signatureLen = header.custom_sig_size;

    img->signature = (uint8_t *)malloc(img->signatureLen);
    if (!(img->signature))
    {
        OSI_loge("Failed to memory allocation(sig) %ld", img->signatureLen);
        fclose(file);
        return false;
    }

    dataLen = header.sector_num * fw->bl_info.sector_size;
    if (dataLen > (double)0xFFFFFFFF)
    {
        OSI_loge("Too large length: %f", dataLen);
        img->rawData = NULL;
    }
    else
    {
        img->rawDataLen = dataLen;
        img->rawData = (uint8_t *)malloc(img->rawDataLen);
    }

    if (!(img->rawData))
    {
        OSI_loge("Failed to memory allocation(data) %ld", img->rawDataLen);
        free(img->signature);
        fclose(file);
        return false;
    }

    if (keyType == 0) // General key
        fseek(file, header.sig_addr, SEEK_SET);
    else // Customer key
        fseek(file, header.custom_sig_addr, SEEK_SET);
    fread(img->signature, img->signatureLen, 1, file);

    fseek(file, header.img_addr, SEEK_SET);
    fread(img->rawData, img->rawDataLen, 1, file);

    fclose(file);

    // get hash
    if(SNFC_N8 > fw->bl_info.version[0])
        img->hashCodeLen = SHA_DIGEST_LENGTH;
    else
        img->hashCodeLen = SHA256_DIGEST_LENGTH;

    img->hashCode = (uint8_t *)malloc(img->hashCodeLen);
    if (!(img->rawData))
    {
        OSI_loge("Failed to memory allocation");
        free(img->signature);
        free(img->rawData);
        return false;
    }

    {/* open ssl */
    if(SNFC_N8 > fw->bl_info.version[0]) {
        SHA_CTX sha_ctx;
        SHA1_Init(&sha_ctx);
        SHA1_Update(&sha_ctx, img->rawData, img->rawDataLen);
        SHA1_Final(img->hashCode, &sha_ctx);
    } else {
        SHA256_CTX sha_ctx;
        SHA256_Init(&sha_ctx);
        SHA256_Update(&sha_ctx, img->rawData, img->rawDataLen);
        SHA256_Final(img->hashCode, &sha_ctx);
    }
    }

    return true;
}

bool nfc_fw_force_update_check(tNFC_HAL_FW_INFO *fw)
{
    int update_mode = 0;
    uint8_t bl_support = get_product_support_fw(fw->bl_info.product);

    OSI_logd("Image Version: %02x.%02x.%02x",
        fw->image_ver.major, fw->image_ver.build1, fw->image_ver.build2);
    OSI_logd("F/W   Version: %02x.%02x.%02x",
        fw->fw_ver.major, fw->fw_ver.build1, fw->fw_ver.build2);

    if ((bl_support != SNFC_UNKNOWN) && ((fw->image_ver.major & 0xF0) != bl_support))
    {
        OSI_loge("This product does not support %x.x.x version of fw. supporting fw version: %x.x.x",
                                                        ((fw->image_ver.major & 0xF0) >> 4), bl_support>>4);
        return false;
    }

    if (get_config_int(cfg_name_table[CFG_UPDATE_MODE], &update_mode))
        OSI_logd("F/W Update mode is %d", update_mode);

//[START] WR for EVT_Transaction to under FW V1.1.13
    if(fw->image_ver.major <= 0x11 && fw->image_ver.build2 < 0x0D)
    {
        OSI_logd("Under the FW v1.1.13 in the system ");
        /*check the hci_nv_file if existing */
        if(access("/data/nfc/nfaStorage.bin1",F_OK) < 0)
        {
            OSI_loge("nfaStorage.bin1 file is not existing");
            OSI_loge("Force FW update will be executed");
            return true;
        }
    }
//[END] WR for EVT_Transaction to under FW V1.1.13
    switch (update_mode) {
    case FW_UPDATE_BY_UPPER_VER: // Image > F/W
        if (fw->image_ver.major > fw->fw_ver.major)
            return true;
        else if (fw->image_ver.major < fw->fw_ver.major)
            return false;

        if (fw->image_ver.build1 > fw->fw_ver.build1)
            return true;
        else if (fw->image_ver.build1 < fw->fw_ver.build1)
            return false;

        if (fw->image_ver.build2 > fw->fw_ver.build2)
            return true;
        break;

    case FW_UPDATE_BY_FORCE_VER: // Force!
        return true;

    default: // Image != F/W
        if ((fw->image_ver.major != fw->fw_ver.major)
            || (fw->image_ver.build1 != fw->fw_ver.build1)
            || (fw->image_ver.build2 != fw->fw_ver.build2))
            return true;
    }

    return false;
}

void fw_force_update(void *param)
{
    // TODO: Timeout handle
    tNFC_HAL_FW_INFO *fw = &nfc_hal_info.fw_info;
    tNFC_HAL_FW_IMAGE_INFO *img = &fw->image_info;
    tNFC_HAL_FW_BL_INFO *bl = &fw->bl_info;
    uint8_t data[10] = {0x00};

    OSI_loge("Try to F/W update!");

    device_set_mode(NFC_DEV_MODE_BOOTLOADER);
    // TODO: error handle
    if (nfc_fw_getsecinfo_from_image(fw, nfc_hal_info.cfg.fw_file, bl->version[NFC_HAL_BL_VER_KEYINFO]))
    {
        TO_LITTLE_ARRY(&data[0], img->hashCodeLen, 2);
        TO_LITTLE_ARRY(&data[2], img->signatureLen, 2);
        nfc_fw_send_cmd(FW_CMD_ENTER_UPDATEMODE, data, 4);
        fw->state = FW_W4_ENTER_UPDATEMODE_RSP;
        nfc_hal_info.flag |= HAL_FLAG_FORCE_FW_UPDATE;
        nfc_hal_info.state = HAL_STATE_FW;
    }
}

bool hal_fw_get_image_path(void)
{
    tNFC_HAL_FW_INFO *fw = &nfc_hal_info.fw_info;
    tNFC_HAL_FW_BL_INFO *bl = &fw->bl_info;
    char fw_file[100];

    memset(fw_file, 0, sizeof(fw_file));
    strncpy(fw_file, cfg_name_table[CFG_FW_FILE], strlen(cfg_name_table[CFG_FW_FILE]));

    if (!get_config_string(fw_file, nfc_hal_info.cfg.fw_file, sizeof(nfc_hal_info.cfg.fw_file)))
    {
        if (!get_config_string(cfg_name_table[CFG_FW_FILE], nfc_hal_info.cfg.fw_file, sizeof(nfc_hal_info.cfg.fw_file)))
            return false;
    }
    OSI_logd("FW file : %s", nfc_hal_info.cfg.fw_file);
    return true;
}
