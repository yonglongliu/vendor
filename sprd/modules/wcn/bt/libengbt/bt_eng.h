/**
 * Copyright (C) 2016 Spreadtrum Communications Inc.
  **/
#ifndef BT_LIBENGBT_BT_ENG_H_
#define BT_LIBENGBT_BT_ENG_H_

#include <utils/Log.h>
#include <android/log.h>


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



#define DUT_MODE_ENABLE "1"
#define DUT_MODE_DISABLE "0"

#define CMD_BT_ON_STR "bt_on"
#define CMD_BT_OFF_STR "bt_off"
#define CMD_DUT_MODE_STR "dut_mode_configure"
#define CMD_DUT_STATUS_STR "eut_status"
#define CMD_DUT_RECV_DATA "dut_recv_data"
#define CMD_NONSIG_TX_MODE_STR "set_nosig_tx_testmode"
#define CMD_NONSIG_RX_MODE_STR "set_nosig_rx_testmode"
#define CMD_NONSIG_RX_RECV_DATA_STR "set_nosig_rx_recv_data"
#define CMD_NONSIG_RX_RECV_DATA_LE_STR "set_nosig_rx_recv_data_le"

#define CMD_LE_ENHANCED_REVEIVER_TEST_STR "le_enhanced_receiver_test"
#define CMD_LE_ENHANCED_TRANSMITTER_TEST_STR "le_enhanced_transmitter_test"
#define CMD_LE_TEST_END_STR "le_test_end"
#define CMD_SET_RF_PATH_STR "set_rf_path"



typedef int (*func_t)(int argc, char **argv);

typedef struct {
    char *cmd;
    func_t func;
} btif_eng_t;




int engpc_bt_get_nonsig_rx_data(uint16_t le, char *buf, uint16_t buf_len, uint16_t *read_len);

int engpc_bt_set_nonsig_tx_testmode(uint16_t enable, uint16_t is_le, uint16_t pattern,
                                    uint16_t channel, uint16_t pac_type, uint16_t pac_len,
                                    uint16_t power_type, uint16_t power_value, uint16_t pac_cnt);

int engpc_bt_set_nonsig_rx_testmode(uint16_t enable, uint16_t is_le, uint16_t pattern,
                                    uint16_t channel, uint16_t pac_type, uint16_t rx_gain,
                                    bt_bdaddr_t addr);

int engpc_bt_le_enhanced_receiver(uint8_t channel, uint8_t phy, uint8_t modulation_index);
int engpc_bt_le_enhanced_transmitter(uint8_t channel, uint8_t length, uint8_t payload, uint8_t phy);
int engpc_bt_le_test_end(void);

int engpc_bt_set_rf_path(uint8_t path);
int engpc_bt_get_rf_path(void);


int engpc_bt_dut_mode_send(uint16_t opcode, uint8_t *buf, uint8_t len);
static int bt_str2bd(const char *str, bt_bdaddr_t *addr);

#endif  // BT_LIBENGBT_BT_ENG_H_
