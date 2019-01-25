#ifndef __WIFI_EUT_HDLR_H__
#define __WIFI_EUT_HDLR_H__

struct eut_cmd_t {
	int  eut_cmd_id;

	/* ext_data: depend on each command */
	int  ext_data;

	/* android version*/
	int  android_version;
	int  hw_product_type;

	char *at_cmd;
	char *at_rsp_fmt;
	char *npi_cmd;
};

struct wifi_rate_t {
	int  index;
	char *type_name;
};

enum {
HW_PRODUCT_UNKNOWN = 0,
HW_PRODUCT_MARLIN = 1,
HW_PRODUCT_MARLIN2 = 2,
HW_PRODUCT_MARLIN3 = 3,
HW_PRODUCT_MAX,
};

#define MAX_SIZE 8
#define WIFI_EUT_KEYWORD "AT+SPWIFITEST="
#define WIFI_EUT_KEYWORD_LEN 14

#define WCN_HARDWARE_PRODUCT "ro.wcn.hardware.product"

#define AT_CMD_RESP_ERR "+SPWIFITEST:ERR=error"
#define AT_CMD_RESP_OK  "+SPWIFITEST:OK"

#define NV_MAC_ADDR_PATH       "/productinfo/wifimac.txt"
#define CURRENT_MAC_ADDR_PATH  "/data/misc/wifi/wifimac.txt"

#define WIFI_EUT_EXIT  0
#define WIFI_EUT_ENTER 1

#define WIFI_TX_STOP  0
#define WIFI_TX_START 1

#define WIFI_RX_STOP  0
#define WIFI_RX_START 1

#define EUT_PROCESS_NONE  0
#define EUT_PROCESS_DONE  1

#define WIFI_EUT_CMD_DEFAULT         0
#define WIFI_EUT_CMD_SET_EUT_MODE    1
#define WIFI_EUT_CMD_GET_EUT_MODE    2
#define WIFI_EUT_CMD_SET_ANT         3
#define WIFI_EUT_CMD_GET_ANT         4
#define WIFI_EUT_CMD_SET_BAND        5
#define WIFI_EUT_CMD_GET_BAND        6
#define WIFI_EUT_CMD_SET_BW          7
#define WIFI_EUT_CMD_GET_BW          8
#define WIFI_EUT_CMD_SET_SBW         9
#define WIFI_EUT_CMD_GET_SBW         10
#define WIFI_EUT_CMD_SET_CHANNEL     11
#define WIFI_EUT_CMD_GET_CHANNEL     12
#define WIFI_EUT_CMD_SET_RATE        13
#define WIFI_EUT_CMD_GET_RATE        14
#define WIFI_EUT_CMD_SET_PAYLOAD     15
#define WIFI_EUT_CMD_GET_PAYLOAD     16
#define WIFI_EUT_CMD_SET_PKT_LEN     17
#define WIFI_EUT_CMD_GET_PKT_LEN     18
#define WIFI_EUT_CMD_SET_TX_POWER    19
#define WIFI_EUT_CMD_GET_TX_POWER    20
#define WIFI_EUT_CMD_SET_TX_MODE     21
#define WIFI_EUT_CMD_GET_TX_MODE     22
#define WIFI_EUT_CMD_SET_TX          23
#define WIFI_EUT_CMD_GET_TX          24
#define WIFI_EUT_CMD_SET_DECODEMODE  25
#define WIFI_EUT_CMD_GET_DECODEMODE  26
#define WIFI_EUT_CMD_SET_MAC_FILTER  27
#define WIFI_EUT_CMD_GET_MAC_FILTER  28
#define WIFI_EUT_CMD_SET_LNA_STATUS  29
#define WIFI_EUT_CMD_GET_LNA_STATUS  30
#define WIFI_EUT_CMD_SET_RX          31
#define WIFI_EUT_CMD_GET_RX          32

#define WIFI_EUT_CMD_SET_PREAMBLE    33
#define WIFI_EUT_CMD_SET_GUARDINTER  34
#define WIFI_EUT_CMD_GET_GUARDINTER  35
#define WIFI_EUT_CMD_GET_RSSI        36
#define WIFI_EUT_CMD_GET_RXPACKCOUNT 37
#define WIFI_EUT_CMD_SET_NETMODE     38
#define WIFI_EUT_CMD_SET_MAC         39
#define WIFI_EUT_CMD_GET_MAC         40

/* Private command that are not set by at command directly */
#define WIFI_EUT_CMD_PRIV_TX_CTRL       64
#define WIFI_EUT_CMD_PRIV_SIN_WAVE      65
#define WIFI_EUT_CMD_PRIV_SET_TX_COUNT  66
/* end private command */
#endif
