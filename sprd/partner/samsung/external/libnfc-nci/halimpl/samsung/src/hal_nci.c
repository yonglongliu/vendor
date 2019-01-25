/*
 *	  Copyright (C) 2013 SAMSUNG S.LSI
 *
 *	 Licensed under the Apache License, Version 2.0 (the "License");
 *	 you may not use this file except in compliance with the License.
 *	 You may obtain a copy of the License at:
 *
 *	 http://www.apache.org/licenses/LICENSE-2.0
 *
 *	 Unless required by applicable law or agreed to in writing, software
 *	 distributed under the License is distributed on an "AS IS" BASIS,
 *	 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	 See the License for the specific language governing permissions and
 *	 limitations under the License.
 *
 *	 Author: Woonki Lee <woonki84.lee@samsung.com>
 *	 Version: 2.0
 *
 */

#include <hardware/nfc.h>
#include <string.h>

#include "osi.h"
#include "hal.h"
#include "hal_msg.h"
#include "device.h"
#include "util.h"

bool DTA_STATUS = false;

int hal_nci_send(tNFC_NCI_PKT *pkt)
{
	size_t len = (size_t)(pkt->len + NCI_HDR_SIZE);
	int ret;

	ret = __send_to_device((uint8_t*)pkt, len);
	if (ret != (int)len
		/* workaround for retry; F/W I2C issue */
		&& (nfc_hal_info.flag & HAL_FLAG_NTF_TRNS_ERROR))
	{
		OSI_loge("NCI message send failed");
		OSI_logd("set flag to 0x%06X", nfc_hal_info.flag);
	}
	else
	{
		util_nci_analyzer(pkt);
	}

	return ret;
}

void hal_nci_send_reset(void)
{
	tNFC_NCI_PKT nci_pkt;

	memset(&nci_pkt, 0, sizeof(tNFC_NCI_PKT));
	nci_pkt.oct0 = NCI_MT_CMD | NCI_PBF_LAST | NCI_GID_CORE;
	nci_pkt.oid = NCI_CORE_RESET;
	nci_pkt.len = 0x01;
	nci_pkt.payload[0] = 0x01;	// Reset config

	hal_nci_send(&nci_pkt);
}

void hal_nci_send_init(void)
{
	tNFC_NCI_PKT nci_pkt;

	memset(&nci_pkt, 0, sizeof(tNFC_NCI_PKT));
	nci_pkt.oct0 = NCI_MT_CMD | NCI_PBF_LAST | NCI_GID_CORE;
	nci_pkt.oid = NCI_CORE_INIT;
	nci_pkt.len = 0x00;

	hal_nci_send(&nci_pkt);
}

/* Workaround: Initialization flash of LMRT */
void hal_nci_send_clearLmrt(void)
{
    tNFC_NCI_PKT nci_pkt;

    memset(&nci_pkt, 0, sizeof(tNFC_NCI_PKT));
    nci_pkt.oct0 = NCI_MT_CMD | NCI_PBF_LAST | NCI_GID_RF_MANAGE;
    nci_pkt.oid = 0x01; // RF_SET_LMRT
    nci_pkt.len = 0x02;
    nci_pkt.payload[0] = 0x00;
    nci_pkt.payload[1] = 0x00;

    hal_nci_send(&nci_pkt);
}
/* END WA */

void get_clock_info(int rev, int field_name, int *buffer)
{
	char rev_field[50] = {'\0',};
	int isRevField = 0;

    snprintf(rev_field, sizeof(rev_field), "%s_REV%d", cfg_name_table[field_name], rev);
	isRevField = get_config_count(rev_field);
	if (rev >= 0 &&  isRevField) {
		if (!get_config_int(rev_field, buffer))
			*buffer = 0;
	}
	else if (!get_config_int(cfg_name_table[field_name], buffer))
		*buffer = 0;
}

void hal_nci_send_prop_fw_cfg(uint8_t product)
{
	tNFC_NCI_PKT nci_pkt;
	int rev = get_hw_rev();

	memset(&nci_pkt, 0, sizeof(tNFC_NCI_PKT));
	nci_pkt.oct0 = NCI_MT_CMD | NCI_PBF_LAST | NCI_GID_PROP;
	nci_pkt.oid = NCI_PROP_FW_CFG;

	switch(productGroup(product))
	{
		case SNFC_N7:
		case SNFC_N8:
		case SNFC_N81:
            nci_pkt.len = 0x01;
            get_clock_info(rev, CFG_FW_CLK_SPEED, (int *)&nci_pkt.payload[0]);
            if(nci_pkt.payload[0] == 0xff)
                OSI_loge("Set a different value! Current Clock Speed Value : 0x%x", nci_pkt.payload[0]);
			break;

        default:
            nci_pkt.len = 0x03;
            get_clock_info(rev, CFG_FW_CLK_TYPE, (int *)&nci_pkt.payload[0]);
            get_clock_info(rev, CFG_FW_CLK_SPEED, (int *)&nci_pkt.payload[1]);
            get_clock_info(rev, CFG_FW_CLK_REQ, (int *)&nci_pkt.payload[2]);
            break;
	}

	hal_nci_send(&nci_pkt);
}

int nci_read_payload(tNFC_HAL_MSG *msg)
{
	tNFC_NCI_PKT *pkt = &msg->nci_packet;
	int ret;

	ret = device_read(NCI_PAYLOAD(pkt), NCI_LEN(pkt));
	if (ret != (int)NCI_LEN(pkt))
	{
		OSI_mem_free((tOSI_MEM_HANDLER)msg);
		OSI_loge("Failed to read payload");
		return ret;
	}

	data_trace("Recv", NCI_HDR_SIZE + ret, msg->param);
	return ret;
}

void nci_init_timeout(void *param)
{
	tNFC_HAL_FW_INFO *fw = &nfc_hal_info.fw_info;
	tNFC_HAL_FW_BL_INFO *bl = &fw->bl_info;

	OSI_loge("NCI_INIT_RSP timeout!!");
	OSI_logd("Try send clk config!");
	device_set_mode(NFC_DEV_MODE_OFF);
	device_set_mode(NFC_DEV_MODE_ON);
	nfc_hal_info.state = HAL_STATE_FW;
	nfc_hal_info.fw_info.state = FW_W4_NCI_PROP_FW_CFG;
	hal_nci_send_prop_fw_cfg(bl->product);
}

bool nfc_hal_prehandler(tNFC_NCI_PKT *pkt)
{
    if (NCI_MT(pkt) == NCI_MT_NTF)
    {
#if 0
/* START [S15012201] - block flip cover in RF field */
        if (NCI_GID(pkt) == NCI_GID_PROP)
        //if (NCI_GID(pkt) == NCI_GID_EE_MANAGE)
        {
            if (NCI_OID(pkt) == NCI_CORE_RESET)
            {
                nfc_stack_cback(HAL_NFC_ERROR_EVT, HAL_NFC_STATUS_ERR_TRANSPORT);
                return false;
            }
        }
/* END [S15012201] - block flip cover in RF field */
#endif
        if (NCI_GID(pkt) == NCI_GID_PROP)
        {
            /* Again procedure. only for N3 isN3group */
            if (NCI_OID(pkt) == NCI_PROP_AGAIN)
            {
                if (nfc_hal_info.nci_last_pkt)
                {
                    OSI_logd("NFC requests sending last message again!");
                    hal_update_sleep_timer();
                    device_write((uint8_t*)nfc_hal_info.nci_last_pkt,
                                 (size_t)(nfc_hal_info.nci_last_pkt->len + NCI_HDR_SIZE));
                    return false;
                }
            }
        }
#if 0
        /* Patch for DTA Analog Poll(UND) */
        if (NCI_GID(pkt) == NCI_GID_RF_MANAGE)
        {
            if( NCI_OID(pkt) == 0x05)
            {
                if(DTA_STATUS)
                {
                    int i;
                    tNFC_NCI_PKT nci_pkt;
                    uint8_t t3t_poll[4] = {0xff, 0xff, 0x01, 0x0f};

                    nci_pkt.oct0 = NCI_MT_CMD | NCI_PBF_LAST | NCI_GID_RF_MANAGE;
                    nci_pkt.oid = 0x08;
                    nci_pkt.len = 0x04;

                    for(i=0; i<4; i++)
                        nci_pkt.payload[i] = t3t_poll[i];

                    hal_nci_send(&nci_pkt);
                }
            }
        }
#endif
    }

	if (NCI_MT(pkt) == NCI_MT_CMD)
	{
		if (NCI_GID(pkt) == NCI_GID_PROP)
		{
			if (NCI_OID(pkt) == NCI_PROP_WR_RESET)
			{
				hal_nci_send_reset();
				nfc_hal_info.flag |= HAL_FLAG_PROP_RESET;
				return false;
			}

			/* Override sleep timeout for DTA */
#define NCI_PROP_DTA_SLEEP_TIME 0x30
			if (NCI_OID(pkt) == NCI_PROP_DTA_SLEEP_TIME)
			{
				nfc_hal_info.cfg.sleep_timeout = 5000;
				OSI_logd("Sleep timeout is overrided to %d", nfc_hal_info.cfg.sleep_timeout);

				pkt->oct0 = NCI_MT_RSP | NCI_PBF_LAST | NCI_GID_PROP;
				nfc_data_callback(pkt);
				return false;
			}
		}
	}

	if (NCI_MT(pkt) == NCI_MT_RSP)
	{
		if (NCI_GID(pkt) == NCI_GID_CORE)
		{
			if (NCI_OID(pkt) == NCI_CORE_RESET)
			{
				pkt->oct0 = NCI_MT_RSP | NCI_PBF_LAST | NCI_GID_PROP;
				pkt->oid = NCI_PROP_WR_RESET;
			}
		}
#if 0
        /* Patch for DTA Analog Poll(All) */
        if (NCI_GID(pkt) == NCI_GID_PROP)
        {
#define NCI_PROP_DTA_ENABLED 0x3A
            if (NCI_OID(pkt) == NCI_PROP_DTA_ENABLED)
            {
                DTA_STATUS = true;
                OSI_logd("DTA Status RSP: %d", DTA_STATUS);
            }
        }
#endif
	}
	return true;
}
