/******************************************************************************
 *
 *  Copyright (C) 2016 Spreadtrum Corporation
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
 *  This is the stream state machine for the BRCM offloaded advanced audio.
 *
 ******************************************************************************/

#define LOG_TAG "bt_vnd_a2dp"
#define LOG_NDEBUG 0

#include <string.h>
#include <pthread.h>
#include <utils/Log.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <ctype.h>
#include <cutils/properties.h>
#include <stdlib.h>

#if (defined(SPRD_FEATURE_A2DPOFFLOAD) && SPRD_FEATURE_A2DPOFFLOAD == TRUE)
#include "bt_hci_bdroid.h"
#include "bt_vendor_sprd.h"
#include "bt_vendor_sprd_hci.h"
#include "bt_vendor_sprd_a2dp.h"
#include "bt_target.h"

#define BTA2DPDBG(param, ...) {ALOGD(param, ## __VA_ARGS__);}

/*****************************************************************************
** Constants and types
*****************************************************************************/

typedef enum
{
    SPRD_VND_A2DP_OFFLOAD_DISABLED,
    SPRD_VND_A2DP_OFFLOAD_TURNING_ON,
    SPRD_VND_A2DP_OFFLOAD_ENABLED,
    SPRD_VND_A2DP_OFFLOAD_TURNING_OFF,
} tSPRD_VND_A2DP_EVENT;


static bt_vendor_op_a2dp_offload_t a2dp_offload_params;
static tCODEC_INFO_SBC codec_info;
#define MEDIA_HEADER_SIZE 12
static uint8_t media_header[MEDIA_HEADER_SIZE] = {0x80, 0x60, 0x00, 0x00
    , 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

#define HCI_SET_A2DPOFFLOAD_PARAMETER_SZIE 20

#define HCI_VSC_SET_A2DPOFFLOAD_PARAMETER  0xFCA7
#define HCI_VSC_SET_A2DPOFFLOAD_ENABLE  0xFCA8

tSPRD_VND_A2DP_EVENT a2dp_offload_state = SPRD_VND_A2DP_OFFLOAD_DISABLED;

/*******************************************************************************
** Local Utility Functions
*******************************************************************************/

static uint8_t sprd_vnd_a2dp_offload_enable(uint8_t enable);


static void log_bin_to_hexstr(uint8_t *bin, uint8_t binsz, const char *log_tag)
{
  char     *str, hex_str[]= "0123456789abcdef";
  uint8_t  i;

  str = (char *)malloc(binsz * 3);
  if (!binsz) {
    ALOGE("%s alloc failed", __FUNCTION__);
    return;
  }

  for (i = 0; i < binsz; i++) {
      str[(i * 3) + 0] = hex_str[(bin[i] >> 4) & 0x0F];
      str[(i * 3) + 1] = hex_str[(bin[i]     ) & 0x0F];
      str[(i * 3) + 2] = ' ';
  }
  str[(binsz * 3) - 1] = 0x00;
  ALOGD("%s %s", log_tag, str);
}

void sprd_vnd_a2dp_hci_uipc_cback(void *pmem)
{
    HC_BT_HDR    *p_evt_buf = (HC_BT_HDR *)pmem;
    uint8_t     *p, len, vsc_result, uipc_opcode;
    uint16_t    vsc_opcode, uipc_event;
    HC_BT_HDR    *p_buf = NULL;
    bt_vendor_op_result_t status = BT_VND_OP_RESULT_SUCCESS;

    p = (uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_LEN;
    len = *p;
    p = (uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_VSC;
    STREAM_TO_UINT16(vsc_opcode,p);
    vsc_result = *p++;

    log_bin_to_hexstr(((uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_VSC), len-1, __FUNCTION__);

    if (vsc_result != 0) {
        ALOGE("%s Failed VSC Op %04x", __FUNCTION__, vsc_opcode);
        status = BT_VND_OP_RESULT_FAIL;
    }
    else if (vsc_opcode == HCI_VSC_SET_A2DPOFFLOAD_PARAMETER) {
        sprd_vnd_a2dp_offload_enable(1);
    }
    else if (vsc_opcode == HCI_VSC_SET_A2DPOFFLOAD_ENABLE) {

        if (a2dp_offload_state == SPRD_VND_A2DP_OFFLOAD_TURNING_ON) {
            bt_vendor_cbacks->a2dp_offload_cb(BT_VND_OP_RESULT_SUCCESS, BT_VND_OP_A2DP_OFFLOAD_START,
                                              a2dp_offload_params.bta_av_handle);
            a2dp_offload_state = SPRD_VND_A2DP_OFFLOAD_ENABLED;

        } else if (a2dp_offload_state == SPRD_VND_A2DP_OFFLOAD_TURNING_OFF) {
            bt_vendor_cbacks->a2dp_offload_cb(BT_VND_OP_RESULT_SUCCESS, BT_VND_OP_A2DP_OFFLOAD_STOP,
                                              a2dp_offload_params.bta_av_handle);
            a2dp_offload_state = SPRD_VND_A2DP_OFFLOAD_DISABLED;
        }
    }

    /* Free the RX event buffer */
    bt_vendor_cbacks->dealloc(p_evt_buf);
}

static uint8_t sprd_vnd_a2dp_offload_enable(uint8_t enable)
{
    uint8_t *p, msg_req[HCI_CMD_MAX_LEN], tmp = 0;

    BTA2DPDBG("%s, %s", __FUNCTION__,
        enable ? "enable" : "disable");
    p = msg_req;
    UINT16_TO_STREAM(p, a2dp_offload_params.lm_handle);
    UINT8_TO_STREAM(p, enable ? 0x01 : 0x00);

    sprd_vnd_send_hci_vsc(HCI_VSC_SET_A2DPOFFLOAD_ENABLE, msg_req, (uint8_t)(p - msg_req), sprd_vnd_a2dp_hci_uipc_cback);
    return 0;
}

static uint8_t sprd_vnd_a2dp_offload_configure()
{
    uint8_t *p, msg_req[HCI_CMD_MAX_LEN], tmp = 0;

    BTA2DPDBG("%s", __FUNCTION__);

    p = msg_req;

    UINT16_TO_STREAM(p, a2dp_offload_params.lm_handle);
    UINT16_TO_STREAM(p, a2dp_offload_params.stream_mtu);
    UINT16_TO_STREAM(p, a2dp_offload_params.remote_cid);
    memcpy(p, media_header, sizeof(media_header));
    p += sizeof(media_header);

    if (codec_info.num_subbands == CODEC_INFO_SBC_SUBBAND_4)
        tmp = 0;
    else if (codec_info.num_subbands == CODEC_INFO_SBC_SUBBAND_8)
        tmp = 1;

    switch(codec_info.block_length) {
        case CODEC_INFO_SBC_BLOCK_4:
           tmp |= 0x00; break;
        case CODEC_INFO_SBC_BLOCK_8:
           tmp |= 0x02; break;
        case CODEC_INFO_SBC_BLOCK_12:
           tmp |= 0x04; break;
        case CODEC_INFO_SBC_BLOCK_16:
           tmp |= 0x06; break;
    }

    switch(codec_info.channel_mode) {
        case CODEC_INFO_SBC_CH_MONO:
           tmp |= 0x00; break;
        case CODEC_INFO_SBC_CH_DUAL:
           tmp |= 0x08; break;
        case CODEC_INFO_SBC_CH_STEREO:
           tmp |= 0x10; break;
        case CODEC_INFO_SBC_CH_JS:
           tmp |= 0x18; break;
    }

    switch(codec_info.sampling_freq) {
        case CODEC_INFO_SBC_SF_16K:
           tmp |= 0x00; break;
        case CODEC_INFO_SBC_SF_32K:
           tmp |= 0x20; break;
        case CODEC_INFO_SBC_SF_44K:
           tmp |= 0x40; break;
        case CODEC_INFO_SBC_SF_48K:
           tmp |= 0x60; break;
    }

    UINT8_TO_STREAM(p, tmp);
    UINT8_TO_STREAM(p, codec_info.bitpool_size);

    sprd_vnd_send_hci_vsc(HCI_VSC_SET_A2DPOFFLOAD_PARAMETER, msg_req, (uint8_t)(p - msg_req), sprd_vnd_a2dp_hci_uipc_cback);

    return 0;
}

static void sprd_vnd_map_a2d_uipc_codec_info(tCODEC_INFO_SBC *codec_info)
{
    switch(codec_info->sampling_freq) {
        case A2DP_SBC_IE_SAMP_FREQ_16:
            codec_info->sampling_freq = CODEC_INFO_SBC_SF_16K; break;
        case A2DP_SBC_IE_SAMP_FREQ_32:
            codec_info->sampling_freq = CODEC_INFO_SBC_SF_32K; break;
        case A2DP_SBC_IE_SAMP_FREQ_44:
            codec_info->sampling_freq = CODEC_INFO_SBC_SF_44K; break;
        case A2DP_SBC_IE_SAMP_FREQ_48:
            codec_info->sampling_freq = CODEC_INFO_SBC_SF_48K; break;

    }
    switch(codec_info->channel_mode) {
        case A2DP_SBC_IE_CH_MD_MONO:
            codec_info->channel_mode = CODEC_INFO_SBC_CH_MONO; break;
        case A2DP_SBC_IE_CH_MD_DUAL:
            codec_info->channel_mode = CODEC_INFO_SBC_CH_DUAL; break;
        case A2DP_SBC_IE_CH_MD_STEREO:
            codec_info->channel_mode = CODEC_INFO_SBC_CH_STEREO; break;
        case A2DP_SBC_IE_CH_MD_JOINT:
            codec_info->channel_mode = CODEC_INFO_SBC_CH_JS; break;
    }
    switch(codec_info->block_length) {
        case A2DP_SBC_IE_BLOCKS_4:
            codec_info->block_length = CODEC_INFO_SBC_BLOCK_4; break;
        case A2DP_SBC_IE_BLOCKS_8:
            codec_info->block_length = CODEC_INFO_SBC_BLOCK_8; break;
        case A2DP_SBC_IE_BLOCKS_12:
            codec_info->block_length = CODEC_INFO_SBC_BLOCK_12; break;
        case A2DP_SBC_IE_BLOCKS_16:
            codec_info->block_length = CODEC_INFO_SBC_BLOCK_16; break;
    }
    switch(codec_info->alloc_method) {
        case A2DP_SBC_IE_ALLOC_MD_S:
            codec_info->alloc_method = CODEC_INFO_SBC_ALLOC_SNR; break;
        case A2DP_SBC_IE_ALLOC_MD_L:
            codec_info->alloc_method = CODEC_INFO_SBC_ALLOC_LOUDNESS; break;
    }
    switch(codec_info->num_subbands) {
        case A2DP_SBC_IE_SUBBAND_4:
            codec_info->num_subbands = CODEC_INFO_SBC_SUBBAND_4; break;
        case A2DP_SBC_IE_SUBBAND_8:
            codec_info->num_subbands = CODEC_INFO_SBC_SUBBAND_8; break;
    }
}

static tA2DP_STATUS sprd_vnd_a2dp_parse_codec_info(tCODEC_INFO_SBC *parsed_info, uint8_t *codec_info)
{
    tA2DP_STATUS status = A2DP_SUCCESS;
    uint8_t   losc;
    uint8_t   mt;

    BTA2DPDBG("%s", __FUNCTION__);

    if( parsed_info == NULL || codec_info == NULL)
        status = A2DP_FAIL;
    else
    {
        losc    = *codec_info++;
        mt      = *codec_info++;
        /* If the function is called for the wrong Media Type or Media Codec Type */
        if(losc != A2DP_SBC_INFO_LEN || *codec_info != A2DP_MEDIA_CT_SBC)
            status = A2DP_WRONG_CODEC;
        else
        {
            codec_info++;
            parsed_info->sampling_freq = *codec_info & A2DP_SBC_IE_SAMP_FREQ_MSK;
            parsed_info->channel_mode  = *codec_info & A2DP_SBC_IE_CH_MD_MSK;
            codec_info++;
            parsed_info->block_length  = *codec_info & A2DP_SBC_IE_BLOCKS_MSK;
            parsed_info->num_subbands  = *codec_info & A2DP_SBC_IE_SUBBAND_MSK;
            parsed_info->alloc_method  = *codec_info & A2DP_SBC_IE_ALLOC_MD_MSK;
            codec_info += 2; /* MAX Bitpool */
            parsed_info->bitpool_size  = (*codec_info > SPRD_A2DP_OFFLOAD_MAX_BITPOOL) ?
                                         SPRD_A2DP_OFFLOAD_MAX_BITPOOL : (*codec_info);

            if(MULTI_BIT_SET(parsed_info->sampling_freq))
                status = A2DP_BAD_SAMP_FREQ;
            if(MULTI_BIT_SET(parsed_info->channel_mode))
                status = A2DP_BAD_CH_MODE;
            if(MULTI_BIT_SET(parsed_info->block_length))
                status = A2DP_BAD_BLOCK_LEN;
            if(MULTI_BIT_SET(parsed_info->num_subbands))
                status = A2DP_BAD_SUBBANDS;
            if(MULTI_BIT_SET(parsed_info->alloc_method))
                status = A2DP_BAD_ALLOC_METHOD;
            if(parsed_info->bitpool_size < A2DP_SBC_IE_MIN_BITPOOL || parsed_info->bitpool_size > A2DP_SBC_IE_MAX_BITPOOL )
                status = A2DP_BAD_MIN_BITPOOL;

            if(status == A2DP_SUCCESS)
                sprd_vnd_map_a2d_uipc_codec_info(parsed_info);

            BTA2DPDBG("%s STATUS %d parsed info : SampF %02x, ChnMode %02x, BlockL %02x, NSubB %02x, alloc %02x, bitpool %02x",
                __FUNCTION__, status, parsed_info->sampling_freq, parsed_info->channel_mode, parsed_info->block_length,
                parsed_info->num_subbands, parsed_info->alloc_method, parsed_info->bitpool_size);

        }
    }
    return status;
}

int sprd_vnd_a2dp_execute(bt_vendor_opcode_t opcode, void *ev_data)
{
    if (opcode == BT_VND_OP_A2DP_OFFLOAD_START) {

        memcpy(&a2dp_offload_params, (bt_vendor_op_a2dp_offload_t*)ev_data, sizeof(bt_vendor_op_a2dp_offload_t));

        if (a2dp_offload_state != SPRD_VND_A2DP_OFFLOAD_DISABLED) {
            BTA2DPDBG("a2dp was not disabled yet, return.");
            return -1;
        }

        a2dp_offload_state = SPRD_VND_A2DP_OFFLOAD_TURNING_ON;

        if (A2DP_SUCCESS != sprd_vnd_a2dp_parse_codec_info(
            &codec_info, (uint8_t *)a2dp_offload_params.codec_info)) {
            ALOGE("%s CodecConfig BT_VND_OP_A2DP_OFFLOAD_START FAILED", __FUNCTION__);
            bt_vendor_cbacks->a2dp_offload_cb(BT_VND_OP_RESULT_FAIL, BT_VND_OP_A2DP_OFFLOAD_START,
                                              a2dp_offload_params.bta_av_handle);
        } else {
            sprd_vnd_a2dp_offload_configure();
        }
    } else if (opcode == BT_VND_OP_A2DP_OFFLOAD_STOP) {

        if (a2dp_offload_state != SPRD_VND_A2DP_OFFLOAD_ENABLED) {
            BTA2DPDBG("a2dp was not enabled yet, return.");
            return -1;
        }
        a2dp_offload_state = SPRD_VND_A2DP_OFFLOAD_TURNING_OFF;
        sprd_vnd_a2dp_offload_enable(0);
    }

    return 0;
}
#endif
