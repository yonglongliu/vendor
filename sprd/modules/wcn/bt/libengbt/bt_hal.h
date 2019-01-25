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

#ifndef BT_LIBENGBT_BT_HAL_SPRD_H_
#define BT_LIBENGBT_BT_HAL_SPRD_H_

#include "bdroid_buildcfg.h"

#define HCI_GRP_VENDOR_SPECIFIC (0x3F << 10) /* 0xFC00 */
#define HCI_GRP_BLE_CMDS                (0x08 << 10)

#define NONSIG_TX_ENABLE (0x00D1 | HCI_GRP_VENDOR_SPECIFIC)
#define NONSIG_TX_DISABLE (0x00D2 | HCI_GRP_VENDOR_SPECIFIC)
#define NONSIG_RX_ENABLE (0x00D3 | HCI_GRP_VENDOR_SPECIFIC)
#define NONSIG_RX_GETDATA (0x00D4 | HCI_GRP_VENDOR_SPECIFIC)
#define NONSIG_RX_DISABLE (0x00D5 | HCI_GRP_VENDOR_SPECIFIC)

#define NONSIG_LE_TX_ENABLE (0x00D6 | HCI_GRP_VENDOR_SPECIFIC)
#define NONSIG_LE_TX_DISABLE (0x00D7 | HCI_GRP_VENDOR_SPECIFIC)
#define NONSIG_LE_RX_ENABLE (0x00D8 | HCI_GRP_VENDOR_SPECIFIC)
#define NONSIG_LE_RX_GETDATA (0x00D9 | HCI_GRP_VENDOR_SPECIFIC)
#define NONSIG_LE_RX_DISABLE (0x00DA | HCI_GRP_VENDOR_SPECIFIC)

#define HCI_DUT_SET_TXPWR (0x00E1 | HCI_GRP_VENDOR_SPECIFIC)
#define HCI_DUT_SET_RXGIAN (0x00E2 | HCI_GRP_VENDOR_SPECIFIC)
#define HCI_DUT_GET_RXDATA (0x00E3 | HCI_GRP_VENDOR_SPECIFIC)

#define HCI_LE_RECEIVER_TEST_OPCODE (0x201D)
#define HCI_LE_TRANSMITTER_TEST_OPCODE (0x201E)
#define HCI_LE_END_TEST_OPCODE (0x201F)

#define HCI_BLE_RECEIVER_TEST (0x001D | HCI_GRP_BLE_CMDS)
#define HCI_BLE_TRANSMITTER_TEST (0x001E | HCI_GRP_BLE_CMDS)
#define HCI_BLE_ENHANCED_RECEIVER_TEST (0x0033 | HCI_GRP_BLE_CMDS)
#define HCI_BLE_ENHANCED_TRANSMITTER_TEST (0x0034 | HCI_GRP_BLE_CMDS)
#define HCI_BLE_TEST_END                (0x001F | HCI_GRP_BLE_CMDS)

#define HCI_DUT_SET_RF_PATH (0x00DB | HCI_GRP_VENDOR_SPECIFIC)
#define HCI_DUT_GET_RF_PATH (0x00DC | HCI_GRP_VENDOR_SPECIFIC)


typedef void (*nonsig_test_rx_recv_callback)(bt_status_t status, uint8_t rssi, uint32_t pkt_cnt,
                                                uint32_t pkt_err_cnt,uint32_t bit_cnt,uint32_t bit_err_cnt);


typedef struct {
    /** set to sizeof(btav_callbacks_t) */
    size_t      size;
    nonsig_test_rx_recv_callback nonsig_test_rx_recv_cb;
    dut_mode_recv_callback dut_mode_recv_cb;
    le_test_mode_callback le_test_mode_cb;
} bthal_callbacks_t;



typedef struct {

    size_t          size;
    int (*enable)(bthal_callbacks_t* callbacks);
    int (*disable)(void);
    int (*dut_mode_configure)(uint8_t mode);
    int (*dut_mode_send)(uint16_t opcode, uint8_t *buf, uint8_t len);
    int (*le_test_mode)(uint16_t opcode, uint8_t *buf, uint8_t len);
    uint8_t (*is_enable)(void);
    int (*set_nonsig_tx_testmode)(uint16_t enable, uint16_t le,
                        uint16_t pattern, uint16_t channel, uint16_t pac_type,
                        uint16_t pac_len, uint16_t power_type, uint16_t power_value,
                        uint16_t pac_cnt);
    int (*set_nonsig_rx_testmode)(uint16_t enable, uint16_t le, uint16_t pattern, uint16_t channel,
                        uint16_t pac_type,uint16_t rx_gain, bt_bdaddr_t addr);
    int (*get_nonsig_rx_data)(uint16_t le);
    int (*le_enhanced_receiver)(uint8_t channel, uint8_t phy, uint8_t modulation_index);
    int (*le_enhanced_transmitter)(uint8_t channel, uint8_t length, uint8_t payload, uint8_t phy);
    int (*le_test_end)(void);
    int (*set_rf_path)(uint8_t path);
    int (*get_rf_path)(void);
} bt_test_kit_t;

const bt_test_kit_t *bt_test_kit_get_interface(void);


#endif  // BT_LIBENGBT_BT_ENG_SPRD_H_
