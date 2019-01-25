
/*
* Copyright (C) 2010 The Android Open Source Project
* Copyright (C) 2012-2015, The Linux Foundation. All rights reserved.
*
* Not a Contribution, Apache license notifications and license are retained
* for attribution purposes only.
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


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <system/audio.h>
#include <cutils/log.h>
#include <cutils/str_parms.h>
#include <tinyalsa/asoundlib.h>
#include "smart_amp.h"
#include "smart_amp_isc.h"


#define LOG_TAG "audio_hw_smart_amp"
#define LOG_NDEBUG 0
#define AUDIO_DUMP_WRITE(fd,buffer)     (fd> 0) ? write(fd, buffer, strlen(buffer)): ALOGI("%s\n",buffer);

#define  SMART_AMP_DUMP_BUFFER_MAX_SIZE     64

typedef struct {
    int channel;
    int format;
}PCM_CFG_INFO;

typedef struct {
    void *pData; /* 2chanel + 16bit + 20 sample */
    int useSize;
}SMART_AMP_BUFFER;

typedef struct {
    void *pData;
    int useSize;
}STRAM_BUFFER;

typedef struct smart_amp_ctl {
    STRAM_BUFFER writebuffer;
    SMART_AMP_BUFFER inBuffer;
    SMART_AMP_BUFFER outBuffer;
    SMART_AMP_STREAM_INFO streaminfo;
    struct ISC_MMODE_MSPK_T *isc_mmode_mspk_ptr;
    int flagDump;
    int CfgMode;
    BOOST_MODE boostMode;
    int setVoltageMv;
    struct mixer_ctl *boost_ctl;
}SMART_AMP_CTL;



volatile int amp_log_level = 3;
#define LOG_V(...)  ALOGV_IF(amp_log_level >= 5,__VA_ARGS__);
#define LOG_D(...)  ALOGD_IF(amp_log_level >= 4,__VA_ARGS__);
#define LOG_I(...)  ALOGI_IF(amp_log_level >= 3,__VA_ARGS__);
#define LOG_W(...)  ALOGW_IF(amp_log_level >= 2,__VA_ARGS__);
#define LOG_E(...)  ALOGE_IF(amp_log_level >= 1,__VA_ARGS__);

#define size_t unsigned int
#define SIZE_OF_BUFFER                 9620 * 2
#define AUDIO_CHANNEL_STEREO           3
#define AUDIO_CHANNEL_MONO             1
#define SMART_AMP_PROCESS_LENGTH       80
#define SMART_AMP_QKS                  26
#define SMART_MONITOR_FILE_1            "data/local/media/smart_amp_monitor_1.bin"
#define SMART_MONITOR_FILE_2            "data/local/media/smart_amp_monitor_1.bin"
#define SMART_AMP_BATTERY_STR         "battery_voltage"
#define ISC_DEFAULT_MODE              "handsfree"

//code optimization
#define CODE_ADVANCE      1

#if 0
struct ISC_ALL_T {
    int a;
    int b;
    int c;
};

struct ISC_ALL_T* init_ISC(uint8 *para_in, uint8 *monitor_out)
{
    return (struct ISC_ALL_T*)malloc(100);
}

int32 close_ISC(struct ISC_ALL_T *isc_all_ptr)
{
    free(isc_all_ptr);
    return 0;
}
int32 sigpath_proc_ISC(struct ISC_ALL_T *isc_all_ptr, int32 *input, int32 *output)
{
    memcpy(output, input, 80);
    return 0;
}
#endif


static int pcm_mixer_to_mono_from_stereo(int16_t *pBuffer, int32_t samples)
{
    int rc = 0;
    int i = 0;
    int16_t *tmp_buf = pBuffer;

    if ((NULL == pBuffer) || (2 >  samples)) {
        LOG_E("%s error: argin is error!", __FUNCTION__);
        rc = -1;
        goto error;
    }

#if CODE_ADVANCE
    int16_t *ptr_y = tmp_buf;
    int16_t *ptr_x = pBuffer;
    int32_t samples_n = samples >> 1;
    int32_t cnt4 = samples_n >> 2;
    int32_t res4 = samples_n - (cnt4 << 2);
    // process multiple of 4
    while (cnt4--)
    {
        *ptr_y++ =  (*(ptr_x + 0 ) + *(ptr_x + 1)) >> 1;
        ptr_x += 2;
        *ptr_y++ =  (*(ptr_x + 0 ) + *(ptr_x + 1)) >> 1;
        ptr_x += 2;
        *ptr_y++ =  (*(ptr_x + 0 ) + *(ptr_x + 1)) >> 1;
        ptr_x += 2;
        *ptr_y++ =  (*(ptr_x + 0 ) + *(ptr_x + 1)) >> 1;
        ptr_x += 2;
    }
    // process remainder of 4
    while (res4--)
    {
        *ptr_y++ =  (*(ptr_x + 0 ) + *(ptr_x + 1)) >> 1;
        ptr_x += 2;
    }
#else
    for (i = 0; i < (samples / 2); i++) {
        tmp_buf[i] = (pBuffer[2 * i + 1] + pBuffer[2 * i]) / 2;
    }

#endif

    return rc;

error:
    return rc;
}

static int pcm_mixer_to_stereo_from_mono(int16_t *pSrcBuffer, int16_t *pDesBuffer, int32_t samples)
{
    int rc = 0;
    int i = 0;
    int16_t *tmp_buf = pDesBuffer;

    if ((NULL == pSrcBuffer) || (NULL == pDesBuffer) || (1  > samples)) {
        LOG_E("%s error: argin is error!", __FUNCTION__);
        rc = -1;
        goto error;
    }
#if CODE_ADVANCE
    int16_t *ptr_src = pSrcBuffer;
    int16_t *ptr_des = pDesBuffer;
    int32_t cnt4 = samples >> 2;
    int32_t res4 = samples - (cnt4 << 2);
    // process multiple of 4
    while (cnt4--)
    {
        *ptr_des++ =  *ptr_src;
        *ptr_des++ =  *ptr_src ++;
        *ptr_des++ =  *ptr_src;
        *ptr_des++ =  *ptr_src ++;
        *ptr_des++ =  *ptr_src;
        *ptr_des++ =  *ptr_src ++;
        *ptr_des++ =  *ptr_src;
        *ptr_des++ =  *ptr_src ++;
    }
    // process remainder of 4
    while (res4--)
    {
        *ptr_des++ =  *ptr_src;
        *ptr_des++ =  *ptr_src ++;
    }

#else
    for (i = 0; i < samples; i++) {
        tmp_buf[2 * i] = pSrcBuffer[i];
        tmp_buf[2 * i + 1] = pSrcBuffer[i];
    }
#endif
    return rc;

error:
    return rc;
}

static int memcpy_to_i32_q26_from_i16(int32_t *pDes, const int16_t *pSrc, size_t count)
{
    int32_t index = 0;
    int32_t rc = 0;

    if ((NULL == pDes) || (NULL == pSrc) || (0 == count)) {
        LOG_E("%s error: argin is error !", __FUNCTION__);
        rc = -1;
        goto error;
    }
#if CODE_ADVANCE
    int16_t *ptr_src = pSrc;
    int32_t *ptr_des = pDes;
    int32_t cnt4 = count >> 2;
    int32_t res4 = count - (cnt4 << 2);
    // process multiple of 4
    while (cnt4--)
    {
           *ptr_des++ =  (int32_t)(*ptr_src++) << (SMART_AMP_QKS - 15);
           *ptr_des++ =  (int32_t)(*ptr_src++) << (SMART_AMP_QKS - 15);
           *ptr_des++ =  (int32_t)(*ptr_src++) << (SMART_AMP_QKS - 15);
           *ptr_des++ =  (int32_t)(*ptr_src++) << (SMART_AMP_QKS - 15);
    }
    // process remainder of 4
    while (res4--)
    {
           *ptr_des++ =  (int32_t)(*ptr_src++) << (SMART_AMP_QKS - 15);
    }
#else
    while (count) {
        pDes[index] = ((int32_t)pSrc[index]) << (SMART_AMP_QKS - 15);
        index++;
        count --;
    }
#endif

    LOG_D("%s success : index = %d, count =%d", __FUNCTION__, index, count);
    return rc;

error:
    return rc;
}

static int memcpy_to_i16_from_i32_q26(int16_t *pDes, const int32_t *pSrc, size_t count)
{
    int32_t index = 0;
    int32_t rc = 0;

    if ((NULL == pDes) || (NULL == pSrc) || (0 == count)) {
        LOG_E("%s error: argin is error !", __FUNCTION__);
        rc = -1;
        goto error;
    }

#if CODE_ADVANCE
    int32_t *ptr_src = pSrc;
    int16_t *ptr_des = pDes;
    int32_t cnt4 = count >> 2;
    int32_t res4 = count - (cnt4<<2);
    // process multiple of 4
    while (cnt4--)
    {
        *ptr_des++ = (int16_t)((*ptr_src++) >> (SMART_AMP_QKS - 15));
        *ptr_des++ = (int16_t)((*ptr_src++) >> (SMART_AMP_QKS - 15));
        *ptr_des++ = (int16_t)((*ptr_src++) >> (SMART_AMP_QKS - 15));
        *ptr_des++ = (int16_t)((*ptr_src++) >> (SMART_AMP_QKS - 15));
    }
    // process remainder of 4
    while (res4--)
    {
        *ptr_des++ = (int16_t)((*ptr_src++) >> (SMART_AMP_QKS - 15));
    }
#else
    while (count) {
        pDes[index] = (int16_t)(pSrc[index] >> (SMART_AMP_QKS - 15));
        index ++;
        count --;
    }
#endif

    LOG_D("%s success : index = %d, count =%d", __FUNCTION__, index, count);
    return rc;

error:
    return rc;
}

static bool is_support_stream(SMART_AMP_STREAM_INFO *pInfo)
{
    if (AUDIO_CHANNEL_OUT_STEREO == pInfo->channel &&  AUDIO_FORMAT_PCM_16_BIT== pInfo->format) {
        return true;
    }
    else {
       return false;
    }
}

SMART_AMP_CTL *init_smart_amp(char *amp_para_path, SMART_AMP_STREAM_INFO *pInfo)
{
    int rc = 0;
    SMART_AMP_CTL *pCtl = NULL;
    char *pMonitor[2] = {SMART_MONITOR_FILE_1, SMART_MONITOR_FILE_2};

    pCtl = (SMART_AMP_CTL *)malloc(sizeof(SMART_AMP_CTL));

    if ((NULL == pCtl) || NULL == pInfo || NULL == amp_para_path) {
        LOG_E("%s create SMART_AMP_CTL is fail!", __FUNCTION__);
        goto error;
    }

    memset(pCtl, 0, sizeof(SMART_AMP_CTL));

    if (!is_support_stream(pInfo)){
        goto error;
    }

    pCtl->inBuffer.pData= (void *)malloc(SMART_AMP_PROCESS_LENGTH);
    memset(pCtl->inBuffer.pData, 0, SMART_AMP_PROCESS_LENGTH);
    pCtl->writebuffer.pData = (int *)malloc(SIZE_OF_BUFFER + SMART_AMP_PROCESS_LENGTH);
    memset(pCtl->writebuffer.pData, 0, SIZE_OF_BUFFER+SMART_AMP_PROCESS_LENGTH);
    pCtl->outBuffer.pData= (void *)malloc(SMART_AMP_PROCESS_LENGTH);
    memset(pCtl->outBuffer.pData, 0, SMART_AMP_PROCESS_LENGTH);

    pCtl->streaminfo.channel = pInfo->channel;
    pCtl->streaminfo.format = pInfo->format;
    pCtl->streaminfo.mixer = pInfo->mixer;
    pCtl->flagDump = pInfo->flagDump;

    if ((NULL == pCtl->inBuffer.pData) || (NULL == pCtl->writebuffer.pData) || (NULL == pCtl->outBuffer.pData) ) {
        LOG_E("%s malloc buffer is fail!!", __FUNCTION__);
        goto error;
    }

    //@TODO: Load Smart AMP para config file
    rc = init_ISC(&pCtl->isc_mmode_mspk_ptr, amp_para_path, pMonitor);
    if (0 != rc) {
        LOG_E("%s rc = %d call init_ISC is fail! ", __FUNCTION__, rc);
        goto error;
    }
    return pCtl;

error:
    if (pCtl->inBuffer.pData) {
        free(pCtl->inBuffer.pData);
    }

    if (pCtl->writebuffer.pData) {
        free(pCtl->writebuffer.pData);
    }

    if (pCtl->outBuffer.pData) {
        free(pCtl->outBuffer.pData);
    }

    if (pCtl) {
        memset(pCtl, 0, sizeof(SMART_AMP_CTL));
        free(pCtl);
        pCtl = NULL;
    }
    return NULL;
}

int deinit_smart_amp(SMART_AMP_CTL *pCtl)
{
    int rc = 0;
    if (NULL == pCtl) {
        LOG_E("%s error: ctl is NULL !", __FUNCTION__);
        rc = -1;
        goto error;
    }

    if (pCtl->inBuffer.pData) {
        free(pCtl->inBuffer.pData);
    }

    if (pCtl->writebuffer.pData) {
        free(pCtl->writebuffer.pData);
    }

    if (pCtl->outBuffer.pData) {
        free(pCtl->outBuffer.pData);
    }

     //@TODO: close smart amp
     rc = close_ISC(pCtl->isc_mmode_mspk_ptr);

    if (pCtl) {
        memset(pCtl, 0, sizeof(SMART_AMP_CTL));
        free(pCtl);
        pCtl = NULL;
    }
    return rc;

error:
    LOG_E("%s error: is fail!", __FUNCTION__);
    return rc;
}


int smart_amp_process(void *pInBuf,  int size, void **pOutBuf, int *pOutSize, SMART_AMP_CTL *pCtl)
{
    int rc = 0;
    int loop = 0;
    int tmpSize = 0;
    int writeSize = 0;
    void *pTmpInBuffer =NULL;
    void *pTmpOutBuffer = NULL;
    void *pWriteBuffer  = NULL;
    void *pTmpBuffer = NULL;

    if ((NULL == pInBuf) || (NULL == pCtl) || (NULL == pOutBuf)  || (NULL == pOutSize)) {
        LOG_E("%s error: argin is NULL", __FUNCTION__);
        rc = -1;
        goto error;
    }

    pTmpBuffer = pInBuf;
    pTmpInBuffer = pCtl->inBuffer.pData;
    pTmpOutBuffer = pCtl->outBuffer.pData;
    pWriteBuffer = pCtl->writebuffer.pData;

    int totalSize = size + pCtl->inBuffer.useSize;
    tmpSize = totalSize;

    if (tmpSize > SIZE_OF_BUFFER + SMART_AMP_PROCESS_LENGTH) {
        LOG_E("%s error: memory hasn't enough useSize = %d size = %d!", __FUNCTION__, pCtl->inBuffer.useSize, size);
        rc = -2;
        goto error;
    }

    if (tmpSize >=  SMART_AMP_PROCESS_LENGTH) { /*use catch and argin*/
        /*fill in buffer*/
        pTmpInBuffer = pCtl->inBuffer.pData + pCtl->inBuffer.useSize;
        memcpy(pTmpInBuffer,  pTmpBuffer, SMART_AMP_PROCESS_LENGTH - pCtl->inBuffer.useSize);

        pTmpInBuffer =pCtl->inBuffer.pData;
        pTmpBuffer = pTmpBuffer + (SMART_AMP_PROCESS_LENGTH - pCtl->inBuffer.useSize);

        for (loop = 0; loop < totalSize / SMART_AMP_PROCESS_LENGTH; loop ++) {
            if (pCtl->flagDump) {
                static FILE *before_file = NULL;
                if (NULL == before_file) {
                    before_file = fopen("data/local/media/dump_smart_amp_process_before.pcm", "wb");
                } else {
                    fwrite(pTmpInBuffer, SMART_AMP_PROCESS_LENGTH, 1, before_file);
                }
            }
            //update in buffer pointer
            pcm_mixer_to_mono_from_stereo((int16_t *)pTmpInBuffer, SMART_AMP_PROCESS_LENGTH / 2);
            memcpy_to_i32_q26_from_i16((int32_t *)pTmpOutBuffer, (const int16_t *)pTmpInBuffer, SMART_AMP_PROCESS_LENGTH /4);

            // AMP Process
            sigpath_proc_mono_ISC(pCtl->isc_mmode_mspk_ptr, (int32_t *)pTmpOutBuffer, (int32_t *)pTmpInBuffer);

            memcpy_to_i16_from_i32_q26((int16_t *)pTmpOutBuffer, (const int32_t *)pTmpInBuffer, SMART_AMP_PROCESS_LENGTH /4);
            pcm_mixer_to_stereo_from_mono((int16_t *)pTmpOutBuffer, (int16_t *)pTmpInBuffer, SMART_AMP_PROCESS_LENGTH /4);

            if (pCtl->flagDump) {
                static FILE *after_file = NULL;
                if (NULL == after_file) {
                    after_file = fopen("data/local/media/dump_smart_amp_process_after.pcm", "wb");
                } else {
                    fwrite(pTmpInBuffer, SMART_AMP_PROCESS_LENGTH, 1, after_file);
                }
            }

            if ((writeSize + SMART_AMP_PROCESS_LENGTH) <= SIZE_OF_BUFFER + SMART_AMP_PROCESS_LENGTH) {
                memcpy(pWriteBuffer, pTmpInBuffer, SMART_AMP_PROCESS_LENGTH);
                pWriteBuffer = pWriteBuffer + SMART_AMP_PROCESS_LENGTH;
                writeSize = writeSize + SMART_AMP_PROCESS_LENGTH;
            } else {
                LOG_E("%s error: loop memory hasn't enough !", __FUNCTION__);
                rc = -3;
                goto error;
            }
            tmpSize -= SMART_AMP_PROCESS_LENGTH;
            memcpy(pTmpInBuffer, pTmpBuffer, SMART_AMP_PROCESS_LENGTH);
            pTmpBuffer += SMART_AMP_PROCESS_LENGTH;
        }
        memcpy(pTmpInBuffer, pTmpBuffer, tmpSize);
        pCtl->inBuffer.useSize = tmpSize;

        *pOutBuf = pCtl->writebuffer.pData;
        *pOutSize = totalSize - tmpSize;

    } else { /*fill in catch*/
        pTmpInBuffer = pCtl->inBuffer.pData + pCtl->inBuffer.useSize;
        memcpy(pTmpInBuffer,  pTmpBuffer, size);
        pCtl->inBuffer.useSize += size;
        *pOutBuf = NULL;
        *pOutSize = 0;
    }

    return 0;
error:
    LOG_E("%s error: is fail rc = %d!", __FUNCTION__, rc);
    return rc;
}

int smart_amp_get_battery_voltage(long int *pBatteryVoltage, char *pFileNodePath)
{
    int rc = 0;
    char r_buffer[128] = {0};
    FILE *pBatteryFile = NULL;

    pBatteryFile = fopen(pFileNodePath, "r");
    if (NULL == pBatteryFile) {
        LOG_E("%s error: open file is fail!", __FUNCTION__);
        rc = -1;
        goto error;
    }

    if  (fread(r_buffer, 1, sizeof(r_buffer), pBatteryFile)) {
        *pBatteryVoltage = strtol(r_buffer, NULL, 10);
    }else {
        LOG_E("%s error: read file is fail!", __FUNCTION__);
        rc = -2;
        goto error;
    }

    if (pBatteryFile) {
        fclose(pBatteryFile);
        pBatteryFile = NULL;
    }
    return rc;

error:
    LOG_E("%s error: rc = %d", __FUNCTION__, rc);
    if (pBatteryFile) {
        fclose(pBatteryFile);
        pBatteryFile = NULL;
    }
    return rc;
}

int smart_amp_set_battery_voltage(const char *kvpairs)
{
    struct str_parms *parms = NULL;
    char value[128];
    long int battery_voltage = 0;
    int rc = 0;
    LOG_D("%s in", __FUNCTION__);

    if ((NULL == kvpairs)) {
        LOG_E("%s error: argin is NULL!", __FUNCTION__);
        rc = -1;
        goto error;
    }
    parms = str_parms_create_str(kvpairs);

    rc = str_parms_get_str(parms, SMART_AMP_BATTERY_STR, value, sizeof(value));
    if (0 <= rc) {
        battery_voltage = strtol(value, NULL, 10);
        rc = input_battery_voltage_ISC((int)battery_voltage);
        if (0 != rc) {
            LOG_E("%s call set_battery_voltage_ISC is fail! rc = %d",
                __FUNCTION__, rc);
            goto error;
        }
        LOG_D("%s battery_voltage = %ld", __FUNCTION__, battery_voltage);
    }
    str_parms_destroy(parms);
    parms = NULL;
    LOG_D("%s out", __FUNCTION__);
    return rc;

error:
    LOG_E("%s error: rc = %d", __FUNCTION__, rc);
    if (parms) {
        str_parms_destroy(parms);
        parms = NULL;
    }
    return rc;
}

int select_smart_amp_config(SMART_AMP_CTL *pCtl, ISC_MODE mode) {
    int rc = 0;
    if (NULL == pCtl) {
        LOG_E("%s  ArgIn Is NULL!", __FUNCTION__);
        rc = -1;
        goto error;
    }
    if (mode == pCtl->CfgMode) {
        LOG_I("%s  CfgMode and select mode is same CfgMode:%d mode:%d ", __FUNCTION__, pCtl->CfgMode, mode);
        return rc;
    }
    LOG_I("%s set mode to %d ", __FUNCTION__, mode);
    switch (mode) {
        case HANDSFREE: {
            rc = config_para_ISC(pCtl->isc_mmode_mspk_ptr, "Handsfree");
            if (0 != rc) {
                LOG_E("%s call config_para_ISC (handsfree) is fail rc = %d!", __FUNCTION__, rc);
                goto error;
            } else {
                break;
            }
        }
        case HEADFREE: {
            rc = config_para_ISC(pCtl->isc_mmode_mspk_ptr, "Headfree");
            if (0 != rc) {
                LOG_E("%s call config_para_ISC (headfree) is fail rc = %d!", __FUNCTION__, rc);
                goto error;
            } else {
                break;
            }
        }
        default :{
            LOG_E("%s mode Is error! mode:%d", __FUNCTION__, mode);
            rc = -2;
            goto error;
        }
    }
    pCtl->CfgMode = mode;

    // setVoltageMv is zero, there are don't set setVoltageMv, so smart amp
    // Voltage may is config init value, or latest "smart_amp_set_battery_voltage_uv"
    // to set value
    if (0 != pCtl->setVoltageMv) {
        rc = input_battery_voltage_ISC(pCtl->setVoltageMv);
        if (0 != rc) {
            LOG_E("%s call input_battery_voltage_ISC \is fail! rc = %d",
                __FUNCTION__, rc);
            goto error;
        }
    } else {
        LOG_I("%s input_battery_voltage_ISC setVoltageMv = %d so don't set valtage to isc"\
            , __FUNCTION__, pCtl->setVoltageMv);
    }
    return rc;
error:
    LOG_E("%s is fail rc = %d!", __FUNCTION__, rc);
    return rc;
}

int smart_amp_get_battery_delta(int *pDelta)
{
    int rc = 0;
    int32_t delta = 0;

    if (NULL == pDelta) {
        LOG_E("%s argin is NULL !", __FUNCTION__);
        rc = -1;
        goto error;
    }

    rc = get_delta_battery_voltage_mv_ISC(&delta);
    if (0 != rc) {
        LOG_E("%s call:get_delta_battery_voltage_mv_ISC is fail! rc = %d!", __FUNCTION__, rc);
        goto error;
    }

    LOG_D("%s get delta:%d mv", __FUNCTION__, delta);
    *pDelta = delta;
    return 0;

error:
    LOG_E("%s is fail rc = %d!", __FUNCTION__, rc);
    return rc;
}

int smart_amp_set_battery_voltage_uv(SMART_AMP_CTL *pCtl, int batteryVoltage)
{
    int rc = 0;
    batteryVoltage = batteryVoltage / 1000;
    rc = input_battery_voltage_ISC((int)batteryVoltage);
    if (0 != rc) {
        LOG_E("%s call smart_amp_direct_set_battery_voltage is fail! rc = %d",
            __FUNCTION__, rc);
        goto error;
    }
    pCtl->setVoltageMv = batteryVoltage;
    return rc;

error:
    LOG_E("%s error: rc = %d", __FUNCTION__, rc);
    return rc;
}

int smart_amp_set_boost(SMART_AMP_CTL *pCtl, BOOST_MODE mode)
{
    int rc = 0;
    int mixer_val = 0;
    const char *smartamp_boost_ctl = "SmartAmp Boost";
    if ((NULL == pCtl) || mode >=BOOST_MODE_INVALID) {
        LOG_E("%s argin is error!", __FUNCTION__);
        rc = -1;
        goto error;
    }
    if (mode == pCtl->boostMode) {
        return rc;
    }
    switch (mode) {
        case BOOST_BYPASS: {
            mixer_val = 0;
        }break;
        case BOOST_5V: {
            mixer_val = 1;
        }break;
        default: {
            LOG_E("%s argin is error!", __FUNCTION__);
            rc = -2;
            goto error;
        }
    }
    if (NULL == pCtl->boost_ctl) {
        pCtl->boost_ctl = mixer_get_ctl_by_name(pCtl->streaminfo.mixer, smartamp_boost_ctl);
        if (NULL == pCtl->boost_ctl) {
            LOG_E("%s get mixer ctl is fail!", __FUNCTION__);
            rc = -3;
            goto error;
        } else {
            mixer_ctl_set_value(pCtl->boost_ctl, 0, mixer_val);
        }
    } else {
        mixer_ctl_set_value(pCtl->boost_ctl, 0, mixer_val);
    }
    pCtl->boostMode = mode;
    return rc;

error:
    LOG_E("%s goto error! rc = %d", __FUNCTION__, rc);
    return rc;
}

void smart_amp_dump(int fd, SMART_AMP_CTL *pCtl){
    char buffer[SMART_AMP_DUMP_BUFFER_MAX_SIZE]={0};
    if(NULL!=pCtl){
        snprintf(buffer,(SMART_AMP_DUMP_BUFFER_MAX_SIZE-1),
            "SmartAmp outBuffer:%p size:%d "
            "inBuffer:%p size:%d "
            "inBuffer:%p size:%d\n",
            pCtl->outBuffer.pData,
            pCtl->outBuffer.useSize,
            pCtl->inBuffer.pData,
            pCtl->inBuffer.useSize,
            pCtl->writebuffer.pData,
            pCtl->writebuffer.useSize);
        AUDIO_DUMP_WRITE(fd,buffer);
        memset(buffer,0,sizeof(buffer));

        snprintf(buffer,(SMART_AMP_DUMP_BUFFER_MAX_SIZE-1),
            "SmartAmp boostMode:%d "
            "Voltage:%d "
            "boost_ctl:%p "
            "mspk_ptr:0x%x "
            "CfgMode:0x%x\n",
            pCtl->boostMode,
            pCtl->setVoltageMv,
            pCtl->boost_ctl,
            pCtl->isc_mmode_mspk_ptr,
            pCtl->CfgMode);
        AUDIO_DUMP_WRITE(fd,buffer);
    }
}
