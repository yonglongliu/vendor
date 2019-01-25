
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <getopt.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "wifi_eut_hdlr.h"
#include "eng_diag.h"
#include "eng_modules.h"
#include "engopt.h"
#include "vlog.h"
#include "cutils/properties.h"

static int get_eut_cmd_index(int eut_cmd_id);

#define SET_ATTR_FUNC(attr) \
	static int set_##attr (int eut_cmd_id, int data) \
	{ \
		int index = get_eut_cmd_index(eut_cmd_id); \
		return index < 0 ? -1 : !(eut_cmd[index].attr = data); \
	}

#define GET_ATTR_FUNC(attr) \
	static int get_##attr (int eut_cmd_id) \
	{ \
		int index = get_eut_cmd_index(eut_cmd_id); \
		return index < 0 ? -1 : eut_cmd[index].attr; \
	}

/* new rate table for marlin3 */
static struct wifi_rate_t g_wifi_rate_table[] = {
    {0, "DSSS-1"}, /*1M_Long*/  {1, "DSSS-2"}, /*2M_Long*/ {2, "DSSS-2S"}, /*2M_Short*/ {3, "CCK-5.5"}, /*5.5M_Long*/ {4, "CCK-5.5S"}, /*5.5M_Short*/
    {5, "CCK-11"}, /*11M_Long*/  {6, "CCK-11S"}, /*11M_Short*/  {7, "OFDM-6"}, /*6M*/ {8, "OFDM-9"}, /*9M*/ {9, "OFDM-12"}, /*12M*/
    {10, "OFDM-18"}, /*18M*/ {11, "OFDM-24"}, /*24M*/ {12, "OFDM-36"}, /*36M*/ {13, "OFDM-48"}, /*48M*/ {14, "OFDM-54"}, /*54M*/
    {15, "MCS-0"}, /*HT_MCS0*/ {16, "MCS-1"}, /*HT_MCS1*/ {17, "MCS-2"}, /*HT_MCS2*/ {18, "MCS-3"},  /*HT_MCS3*/ {19, "MCS-4"},  /*HT_MCS4*/
    {20, "MCS-5"},  /*HT_MCS5*/ {21, "MCS-6"},  /*HT_MCS6*/ {22, "MCS-7"}, /*HT_MCS7*/ {23, "MCS-8"}, /*HT_MCS8*/{24, "MCS-9"}, /*HT_MCS9*/
    {25, "MCS-10"}, /*HT_MCS10*/ {26, "MCS-11"}, /*HT_MCS11*/ {27, "MCS-12"}, /*HT_MCS12*/ {28, "MCS-13"}, /*HT_MCS13*/ {29, "MCS-14"}, /*HT_MCS14*/
    {30, "MCS-15"}, /*HT_MCS15*/ {31, "MCS0_1SS"}, /*VHT_MCS0_1SS*/ {32, "MCS1_1SS"}, /*VHT_MCS1_1SS*/ {33, "MCS2_1SS"}, /*VHT_MCS2_1SS*/ {34, "MCS3_1SS"}, /*VHT_MCS3_1SS*/
    {35, "MCS4_1SS"}, /*VHT_MCS4_1SS*/ {36, "MCS5_1SS"}, /*VHT_MCS5_1SS*/ {37, "MCS6_1SS"}, /*VHT_MCS6_1SS*/ {38, "MCS7_1SS"}, /*VHT_MCS7_1SS*/ {39, "MCS8_1SS"}, /*VHT_MCS8_1SS*/
    {40, "MCS9_1SS"}, /*VHT_MCS9_1SS*/ {41, "MCS0_2SS"}, /*VHT_MCS0_2SS*/ {42, "MCS1_2SS"}, /*VHT_MCS1_2SS*/ {43, "MCS2_2SS"}, /*VHT_MCS2_2SS*/ {44, "MCS3_2SS"}, /*VHT_MCS3_2SS*/
    {45, "MCS4_2SS"}, /*VHT_MCS4_2SS*/ {46, "MCS5_2SS"}, /*VHT_MCS5_2SS*/ {47, "MCS6_2SS"}, /*VHT_MCS6_2SS*/ {48, "MCS7_2SS"}, /*VHT_MCS7_2SS*/ {49, "MCS8_2SS"}, /*VHT_MCS8_2SS*/
    {50, "MCS9_2SS"}, /*VHT_MCS9_2SS*/ {-1, "null"}
};

/* rate table-1 for marlin2, it will replace by array g_wifi_rate_table for marlin3 */
static struct wifi_rate_t g_wifi_rate_table_1[] = {
    {1, "DSSS-1"},   {2, "DSSS-2"},   {5, "CCK-5.5"},  {11, "CCK-11"},
    {6, "OFDM-6"},   {9, "OFDM-9"},   {12, "OFDM-12"}, {18, "OFDM-18"},
    {24, "OFDM-24"}, {36, "OFDM-36"}, {48, "OFDM-48"}, {54, "OFDM-54"},
    {7, "MCS-0"},    {13, "MCS-1"},   {19, "MCS-2"},   {26, "MCS-3"},
    {39, "MCS-4"},   {52, "MCS-5"},   {58, "MCS-6"},   {65, "MCS-7"},
    {-1, "null"}
};

static struct eut_cmd_t eut_cmd[] = {
	/*fixme: should be load/unload wifi driver before/after iwnpi command*/
	{
		.at_cmd = "EUT,%d\r\n",
		.at_rsp_fmt = NULL,
		/* 0: stop, 1: start */
		.npi_cmd = "iwnpi wlan0 %s",
		.eut_cmd_id = WIFI_EUT_CMD_SET_EUT_MODE,
		.ext_data = WIFI_EUT_EXIT,
	},
	{
		.at_cmd = "EUT?\r\n",
		.at_rsp_fmt = "+SPWIFITEST:EUT=%d",
		.npi_cmd = NULL,
		.eut_cmd_id = WIFI_EUT_CMD_GET_EUT_MODE,
	},
	{
		.at_cmd = "ANT,%d\r\n",
		.at_rsp_fmt = NULL,
		.npi_cmd = "iwnpi wlan0 set_chain %d",
		.eut_cmd_id = WIFI_EUT_CMD_SET_ANT,
	},
	{
		.at_cmd = "ANT?\r\n",
		.at_rsp_fmt = "+SPWIFITEST:ANT=%d",
		.npi_cmd = "iwnpi wlan0 get_chain",
		.eut_cmd_id = WIFI_EUT_CMD_GET_ANT,
	},
	{
		.at_cmd = "BAND,%d\r\n",
		.at_rsp_fmt = NULL,
		.npi_cmd = "iwnpi wlan0 set_band %d",
		.eut_cmd_id = WIFI_EUT_CMD_SET_BAND,
	},
	{
		.at_cmd = "BAND?\r\n",
		.at_rsp_fmt = "+SPWIFITEST:BAND=%d",
		.npi_cmd = "iwnpi wlan0 get_band",
		.eut_cmd_id = WIFI_EUT_CMD_GET_BAND,
	},
	{
		.at_cmd = "BW,%d\r\n",
		.at_rsp_fmt = NULL,
		.npi_cmd = "iwnpi wlan0 set_cbw %d",
		.eut_cmd_id = WIFI_EUT_CMD_SET_BW,
	},
	{
		.at_cmd = "BW?\r\n",
		.at_rsp_fmt = "+SPWIFITEST:BW=%d",
		.npi_cmd = "iwnpi wlan0 get_cbw",
		.eut_cmd_id = WIFI_EUT_CMD_GET_BW,
	},
	{
		.at_cmd = "SBW,%d\r\n",
		.at_rsp_fmt = NULL,
		.npi_cmd = "iwnpi wlan0 set_sbw %d",
		.eut_cmd_id = WIFI_EUT_CMD_SET_SBW,
	},
	{
		.at_cmd = "SBW?\r\n",
		.at_rsp_fmt = "+SPWIFITEST:SBW=%d",
		.npi_cmd = "iwnpi wlan0 get_sbw",
		.eut_cmd_id = WIFI_EUT_CMD_GET_SBW,
	},
	{
		.at_cmd = "CH,%d,%d\r\n",
		.at_rsp_fmt = NULL,
		.npi_cmd = "iwnpi wlan0 set_channel %d %d",
		.eut_cmd_id = WIFI_EUT_CMD_SET_CHANNEL,
	},
	{
		.at_cmd = "CH?\r\n",
		.at_rsp_fmt = "+SPWIFITEST:CH=%d,%d",
		.npi_cmd = "iwnpi wlan0 get_channel",
		.eut_cmd_id = WIFI_EUT_CMD_GET_CHANNEL,
	},
	{
		.at_cmd = "RATE,%s\r\n",
		.at_rsp_fmt = NULL,
		.npi_cmd = "iwnpi wlan0 set_rate %d",
		.eut_cmd_id = WIFI_EUT_CMD_SET_RATE,
	},
	{
		.at_cmd = "RATE?\r\n",
		.at_rsp_fmt = "+SPWIFITEST:RATE=%s",
		.npi_cmd = "iwnpi wlan0 get_rate",
		.eut_cmd_id = WIFI_EUT_CMD_GET_RATE,
	},
	{
		.at_cmd = "PREAMBLE,%d\r\n",
		.at_rsp_fmt = NULL,
		.npi_cmd = "iwnpi wlan0 set_preamble %d",
		.eut_cmd_id = WIFI_EUT_CMD_SET_PREAMBLE,
	},
	{
		.at_cmd = "PAYLOAD,%d\r\n",
		.at_rsp_fmt = NULL,
		.npi_cmd = "iwnpi wlan0 set_payload %d",
		.eut_cmd_id = WIFI_EUT_CMD_SET_PAYLOAD,
	},
	{
		.at_cmd = "PAYLOAD?\r\n",
		.at_rsp_fmt = "+SPWIFITEST:PAYLOAD=%d",
		.npi_cmd = "iwnpi wlan0 get_payload",
		.eut_cmd_id = WIFI_EUT_CMD_GET_PAYLOAD,
	},
	{
		.at_cmd = "PKTLEN,%d\r\n",
		.at_rsp_fmt = NULL,
		.npi_cmd = "iwnpi wlan0 set_pkt_len %d",
		.eut_cmd_id = WIFI_EUT_CMD_SET_PKT_LEN,
	},
	{
		.at_cmd = "PKTLEN?\r\n",
		.at_rsp_fmt = "+SPWIFITEST:PKTLEN=%d",
		.npi_cmd = "iwnpi wlan0 get_pkt_len",
		.eut_cmd_id = WIFI_EUT_CMD_GET_PKT_LEN,
	},
	{
		.at_cmd = "GUARDINTERVAL,%d\r\n",
		.at_rsp_fmt = NULL,
		.npi_cmd = "iwnpi wlan0 set_gi %d",
		.eut_cmd_id = WIFI_EUT_CMD_SET_GUARDINTER,
	},
	{
		.at_cmd = "GUARDINTERVAL?\r\n",
		.at_rsp_fmt = "+SPWIFITEST:GUARDINTERVAL=%d",
		.npi_cmd = "iwnpi wlan0 get_gi",
		.eut_cmd_id = WIFI_EUT_CMD_GET_GUARDINTER,
	},
	{
		.at_cmd = "TXPWRLV,%d\r\n",
		.at_rsp_fmt = NULL,
		.npi_cmd = "iwnpi wlan0 set_tx_power %d",
		.eut_cmd_id = WIFI_EUT_CMD_SET_TX_POWER,
	},
	{
		.at_cmd = "TXPWRLV?\r\n",
		.at_rsp_fmt = "+SPWIFITEST:TXPWRLV=%d",
		.npi_cmd = "iwnpi wlan0 get_tx_power",
		.eut_cmd_id = WIFI_EUT_CMD_GET_TX_POWER,
	},
	/* more than one iwnpi command to be run
	 * CW mode(id = 0):
	 *     1.iwnpi wlan0 tx_stop
	 *     2.iwnpi wlan0 sin_wave
	 * other modes:(duty cycle/carrier suppressioon/local leakage)
	 *     1.iwnpi wlan0 set_tx_mode
	 */
	{
		.at_cmd = "TXMODE,%d\r\n",
		.at_rsp_fmt = NULL,
		.npi_cmd = "iwnpi wlan0 set_tx_mode %d",
		.eut_cmd_id = WIFI_EUT_CMD_SET_TX_MODE,
	},
	{
		.at_cmd = "TXMODE?\r\n",
		.at_rsp_fmt = "+SPWIFITEST:TXMODE=%d",
		.npi_cmd = "iwnpi wlan0 get_tx_mode",
		.eut_cmd_id = WIFI_EUT_CMD_GET_TX_MODE,
	},
	/* more than one iwnpi command to be run
	 * param1 = open:
	 *     1.iwnpi wlan0 set_tx_count pkt_count
	 *     2.iwnpi wlan0 tx_start
	 * param1 = close:
	 *     1.iwnpi wlan0 tx_stop
	 */
	{
		.at_cmd = "TX,%d,%d,%d\r\n",
		.at_rsp_fmt = NULL,
		.npi_cmd = NULL,
		.eut_cmd_id = WIFI_EUT_CMD_SET_TX,
	},
	{
		.at_cmd = "TX?\r\n",
		.at_rsp_fmt = "+SPWIFITEST:TX=%d",
		.npi_cmd = NULL,
		.eut_cmd_id = WIFI_EUT_CMD_GET_TX,
	},
	{
		.at_cmd = "DECODEMODE,%d\r\n",
		.at_rsp_fmt = NULL,
		.npi_cmd = "iwnpi wlan0 set_fec %d",
		.eut_cmd_id = WIFI_EUT_CMD_SET_DECODEMODE,
	},
	{
		.at_cmd = "DECODEMODE?\r\n",
		.at_rsp_fmt = "+SPWIFITEST:DECODEMODE=%d",
		.npi_cmd = "iwnpi wlan0 get_fec",
		.eut_cmd_id = WIFI_EUT_CMD_GET_DECODEMODE,
	},
	/* fixme: not implement */
	{
		.at_cmd = "MACFILTER,%d\r\n",
		.at_rsp_fmt = NULL,
		.npi_cmd = NULL,
		.eut_cmd_id = WIFI_EUT_CMD_SET_MAC_FILTER,
	},
	/* fixme: not implement */
	{
		.at_cmd = "MACFILTER?\r\n",
		.at_rsp_fmt = NULL,
		.npi_cmd = "iwnpi wlan0 get_macfilter",
		.eut_cmd_id = WIFI_EUT_CMD_GET_MAC_FILTER,
	},
	{
		.at_cmd = "LNA,%d\r\n",
		.at_rsp_fmt = NULL,
		.npi_cmd = "iwnpi wlan0 %s",
		.eut_cmd_id = WIFI_EUT_CMD_SET_LNA_STATUS,
	},
	{
		.at_cmd = "LNA?\r\n",
		.at_rsp_fmt = "+SPWIFITEST:LNA=%d",
		.npi_cmd = "iwnpi wlan0 lna_status",
		.eut_cmd_id = WIFI_EUT_CMD_GET_LNA_STATUS,
	},
	{
		.at_cmd = "RX,%d\r\n",
		.at_rsp_fmt = NULL,
		.npi_cmd = "iwnpi wlan0 %s",
		.eut_cmd_id = WIFI_EUT_CMD_SET_RX,
	},
	{
		.at_cmd = "RX?\r\n",
		.at_rsp_fmt = "+SPWIFITEST:RX=%d",
		.npi_cmd = NULL,
		.eut_cmd_id = WIFI_EUT_CMD_GET_RX,
	},
	{
		.at_cmd = "RSSI?\r\n",
		.at_rsp_fmt = "+SPWIFITEST:RSSI=%d",
		.npi_cmd = "iwnpi wlan0 get_rssi",
		.eut_cmd_id = WIFI_EUT_CMD_GET_RSSI,
	},
	{
		.at_cmd = "RXPACKCOUNT?\r\n",
		.at_rsp_fmt = "+SPWIFITEST:RXPACKCOUNT=%d,%d%d",
		.npi_cmd = "iwnpi wlan0 get_rx_ok",
		.eut_cmd_id = WIFI_EUT_CMD_GET_RXPACKCOUNT,
	},
	{
		.at_cmd = "XTXCTL,%d\r\n",
		.npi_cmd = "iwnpi wlan0 %s",
		.eut_cmd_id = WIFI_EUT_CMD_PRIV_TX_CTRL,
	},
	{
		.at_cmd = "XSINWAVE,%d\r\n",
		.npi_cmd = "iwnpi wlan0 sin_wave",
		.eut_cmd_id = WIFI_EUT_CMD_PRIV_SIN_WAVE,
	},
	{
		.at_cmd = "XSET_TX_COUNT,%d\r\n",
		.npi_cmd = "iwnpi wlan0 set_tx_count %d",
		.eut_cmd_id = WIFI_EUT_CMD_PRIV_SET_TX_COUNT,
	},
	/* mac address */
	{
		.at_cmd = "MAC,%02X%02X%02X%02X%02X%02X\r\n",
		.npi_cmd = NULL,
		.eut_cmd_id = WIFI_EUT_CMD_SET_MAC,
	},
	{
		.at_cmd = "MAC?\r\n",
		.npi_cmd = NULL,
		.eut_cmd_id = WIFI_EUT_CMD_GET_MAC,
	},
	/* invalid for marlin3 */
	{
		.at_cmd = "NETMODE,%d\r\n",
		.npi_cmd = NULL,
		.eut_cmd_id = WIFI_EUT_CMD_SET_NETMODE,
	},
	{
		.at_cmd = NULL,
		.eut_cmd_id = WIFI_EUT_CMD_DEFAULT,
	}
};

SET_ATTR_FUNC(ext_data)
SET_ATTR_FUNC(hw_product_type)
SET_ATTR_FUNC(android_version)

GET_ATTR_FUNC(ext_data)
GET_ATTR_FUNC(hw_product_type)
GET_ATTR_FUNC(android_version)

static int get_eut_cmd_index(int eut_cmd_id)
{
	int i;
	for (i = 0; eut_cmd[i].at_cmd != NULL; i++)
		if (eut_cmd_id == eut_cmd[i].eut_cmd_id)
			return i;
	return -1;
}

/* descript: To check if the driver has benn loaded
 * param:
 *     mod_name: module name without extension
 * return value:
 *     1: drvier has been loaded
 *     0: not exist
 *    -1: error
 */

static int is_module_exist(char *mod_name)
{
	FILE *fp;
	char line[256];
	char module[64];
	char *p = module;
	static int is_driver_loaded;


	while(*mod_name && *mod_name != '.')
		*p++ = *mod_name++;

	*p = 0;

	fp = fopen("/proc/modules", "re");
	if (fp == NULL) {
		ENG_LOG("wifi_eut: open /proc/modules failed");
		return -1;
	}

	while (fgets(line, sizeof(line), fp)) {
		/* module has been loaded */
		if (!strncmp(module, line, strlen(module)))
			return 1;
	}

	fclose(fp);
	return 0;
}

/* description: load driver
 * return value:
 *     0: success
 *    -1: error
 */
static int load_driver(char *path)
{
	int ret;
	int fd;
	void *addr;
	struct stat st;
	char *mod_name;
	char cmd[128];

	if (path == NULL) {
		ENG_LOG("wifi_eut: %s path is NULL", __func__);
		return -1;
	}

	mod_name = strrchr(path, '/');
	if (mod_name == NULL)
		return -1;

	sprintf(cmd, "insmod %s", path);
	ret = system(cmd);
	if (ret < 0){
		ENG_LOG("wifi_eut: load drv(%s) failed, ret = %d\n", path, ret);
		return -1;
	}

	ENG_LOG("wifi_eut: load drv(%s) success, ret = %d", path, ret);
	return 0;
}

static int get_rate_index(int prod_type, char *modulation)
{
	int i;
	struct wifi_rate_t *rate_tab;

	if (modulation == NULL)
		return -1;

	switch(prod_type) {
	case HW_PRODUCT_MARLIN3:
		rate_tab = g_wifi_rate_table;
		break;

	default:
		rate_tab = g_wifi_rate_table_1;
		break;
	}

	for (i = 0; rate_tab[i].index != -1; i++) {
		if (strcmp(modulation, rate_tab[i].type_name) == 0)
			break;
	}
	return rate_tab[i].index;
}

static char *get_modulation_name(int prod_type, int index)
{
	int i;
	struct wifi_rate_t *rate_tab;

	if (index < 0)
		return "null";

	switch(prod_type) {
	case HW_PRODUCT_MARLIN3:
		rate_tab = g_wifi_rate_table;
		break;

	default:
		rate_tab = g_wifi_rate_table_1;
		break;
	}

	for (i = 0; rate_tab[i].index != -1; i++) {
		if (index == rate_tab[i].index)
			break;
	}
	return rate_tab[i].type_name;
}

/* at_cmd: at command ascii
 * npi_cmd_buf: buf that return iwnpi command
 * return value:
 *     return -1 if parse failed, else return 0;
 */
static int atcmd_to_npi(int eut_cmd_id, char *at_cmd, char *npi_cmd_buf)
{
	int ret;
	int ch1,ch2;
	char str[32];
	int index, val;
	int rate_index;

	index = get_eut_cmd_index(eut_cmd_id);
	if (index < 0)
		return -1;

	switch (eut_cmd_id) {
	case WIFI_EUT_CMD_SET_EUT_MODE:
		ret = sscanf(at_cmd, eut_cmd[index].at_cmd, &val);
		if (ret != 1) {
			ENG_LOG("wifi_eut: %s %d\n", __func__, __LINE__);
			return -1;
		}

		if (val != 0 && val != 1) {
			ENG_LOG("wifi_eut: %s %d\n", __func__, __LINE__);
			return -1;
		}

		set_ext_data(eut_cmd_id, val);
		sprintf(npi_cmd_buf, eut_cmd[index].npi_cmd, (val == 1) ? "start" : "stop");
		break;

	case WIFI_EUT_CMD_SET_LNA_STATUS:
		ret = sscanf(at_cmd, eut_cmd[index].at_cmd, &val);
		if (ret != 1) {
			ENG_LOG("wifi_eut: %s %d\n", __func__, __LINE__);
			return -1;
		}

		if (val != 0 && val != 1) {
			ENG_LOG("wifi_eut: %s %d\n", __func__, __LINE__);
			return -1;
		}

		set_ext_data(eut_cmd_id, val);
		sprintf(npi_cmd_buf, eut_cmd[index].npi_cmd, (val == 1) ? "lna_on" : "lna_off");
		break;

	case WIFI_EUT_CMD_SET_CHANNEL:
		ret = sscanf(at_cmd, "CH,%d,%d\r\n", &ch1, &ch2);
		if (ret == 2) {
			sprintf(npi_cmd_buf, "iwnpi wlan0 set_channel %d %d", ch1, ch2);
		} else if (ret == 1) {
			if (get_hw_product_type(eut_cmd_id) == HW_PRODUCT_MARLIN3)
				sprintf(npi_cmd_buf, "iwnpi wlan0 set_channel %d %d", ch1, ch1);
			else
				sprintf(npi_cmd_buf, "iwnpi wlan0 set_channel %d", ch1);
		} else {
			ENG_LOG("wifi_eut: %s %d, chn counts = %d\n", __func__, __LINE__, ret);
			return -1;
		}

		break;

	case WIFI_EUT_CMD_SET_RATE:
		ret = sscanf(at_cmd, eut_cmd[index].at_cmd, str);
		if (ret != 1) {
			ENG_LOG("wifi_eut: %s %d\n", __func__, __LINE__);
			return -1;
		}

		rate_index = get_rate_index(get_hw_product_type(eut_cmd_id), str);
		ENG_LOG("wifi_eut: %s %d, rate = %s, rate_index = %d\n",
				__func__, __LINE__, str, rate_index);
		if (rate_index < 0) {
			return -1;
		}

		sprintf(npi_cmd_buf, eut_cmd[index].npi_cmd, rate_index);
		break;

	case WIFI_EUT_CMD_PRIV_TX_CTRL:
		ret = sscanf(at_cmd, eut_cmd[index].at_cmd, &val);
		if (ret != 1) {
			ENG_LOG("wifi_eut: %s %d\n", __func__, __LINE__);
			return -1;
		}

		set_ext_data(eut_cmd_id, val);
		sprintf(npi_cmd_buf, eut_cmd[index].npi_cmd, val == 1 ? "tx_start" : "tx_stop");
		break;

	/* compound command, handled out of this function */
	case WIFI_EUT_CMD_SET_TX:
	case WIFI_EUT_CMD_GET_TX:
		break;

	case WIFI_EUT_CMD_SET_MAC_FILTER:
	case WIFI_EUT_CMD_SET_DECODEMODE:
		ENG_LOG("wifi_eut: %s %d\n", __func__, __LINE__);
		return -1;

	/* invalid in marlin3 */
	case WIFI_EUT_CMD_SET_NETMODE:
		ret = sscanf(at_cmd, eut_cmd[index].at_cmd, &val);
		if (ret != 1) {
			ENG_LOG("wifi_eut: %s %d\n", __func__, __LINE__);
			return -1;
		}

		set_ext_data(eut_cmd_id, val);
		break;

	case WIFI_EUT_CMD_SET_RX:
		ret = sscanf(at_cmd, eut_cmd[index].at_cmd, &val);
		if (ret != 1) {
			ENG_LOG("wifi_eut: %s %d\n", __func__, __LINE__);
			return -1;
		}

		if (val != 0 && val != 1)
			return -1;

		set_ext_data(eut_cmd_id, val);
		sprintf(npi_cmd_buf, eut_cmd[index].npi_cmd, val == 1 ? "rx_start" : "rx_stop");
		break;

	case WIFI_EUT_CMD_SET_TX_MODE:
		ret = sscanf(at_cmd, eut_cmd[index].at_cmd, &val);
		if (ret != 1) {
			ENG_LOG("wifi_eut: %s %d\n", __func__, __LINE__);
			return -1;
		}

		eut_cmd[index].ext_data = val;

		/* CW mode */
		if (val == 0)
			return 0;

		sprintf(npi_cmd_buf, eut_cmd[index].npi_cmd, val);
		break;

	default:
		if (eut_cmd[index].at_cmd == NULL)
			return -1;

		ret = sscanf(at_cmd, eut_cmd[index].at_cmd, &val);
		if (ret != 1) {
			ENG_LOG("wifi_eut: %s %d\n", __func__, __LINE__);
			return -1;
		}

		sprintf(npi_cmd_buf, eut_cmd[index].npi_cmd, val);
		break;
	}
	return 0;
}

/* descript: parse at command to get the eut command id and iwnpi command
 * param:
 *     cmd: at command
 *     argv: buffer that return iwnpi param
 * return value:
 *     -1 if parse failed, else return eut command id
 */
int at2npi(char *cmd, char *argv[])
{
	int val;
	int ret;
	int index, i;
	int is_query_cmd = 0; /*set data or get data*/
	char *p, *chr;
	int cmd_match;
	static char buff[256];

	if (cmd == NULL || argv == NULL) {
		ENG_LOG("wifi_eut: %s %d\n", __func__, __LINE__);
		return -1;
	}

	ENG_LOG("wifi_eut: %s cmd = %s", __func__, cmd);

	/* match cmd format: ATCOMAND?\r\n */
	chr = strchr(cmd, '?');
	if (chr != NULL)
		is_query_cmd = 1;
	else {
		/* match cmd format: ATCOMAND,%d\r\n */
		chr = strchr(cmd, ',');
		if (chr == NULL) {
			/* match cmd format: ATCOMAND\r\n */
			chr = strchr(cmd, '\n');
			if (chr == NULL)
				return -1;
		}
	}

	for (index = 0, cmd_match = 0; eut_cmd[index].at_cmd != NULL; index++)
	{
		/* compare keywords: "EUT?"/"EUT," etc */
		if (!strncmp(eut_cmd[index].at_cmd, cmd, chr - cmd + 1)) {
			cmd_match = 1;
			break;
		}
	}

	if (!cmd_match)
		return -1;

	memset(buff, 0x0, sizeof(buff));
	if (is_query_cmd) {
		if (eut_cmd[index].npi_cmd != NULL)
			strcpy(buff, eut_cmd[index].npi_cmd);
	} else {
		ret = atcmd_to_npi(eut_cmd[index].eut_cmd_id, cmd, buff);
		if (ret < 0)
			return -1;
	}

	if (buff[0] != 0) {
		for (i = 0, p = buff; ; i++, p = NULL) {
			argv[i] = strtok(p, " ");
			if (argv[i] == NULL)
				break;

			ENG_LOG("wifi_eut: argv[%d] = %s\n", i, argv[i]);
		}
	}
	return eut_cmd[index].eut_cmd_id;
}

static void strip_white_space(char *str)
{
	int i, j;
	for (i = 0, j = 0; str[j] != 0; j++) {
		if (str[j] != ' ') {
			if (i != j) {
				str[i] = str[j];
			}

			i++;
		}
	}

	str[i] = 0;
}

static int fmt_npi_output2(int eut_cmd_id, char *output, char *rsp)
{
	int ret;
	int result;
	char *rate;
	int index;
	int ch1, ch2;
	int rx_count, rx_err, rx_fail;

	ret = sscanf(output, "ret:%d:end", &result);
	if (ret == 1) {
		switch (eut_cmd_id) {
		case WIFI_EUT_CMD_GET_CHANNEL:
			if (rsp != NULL)
				sprintf(rsp, "+SPWIFITEST:CH=%d", result);
			break;

		case WIFI_EUT_CMD_GET_RATE:
			if (rsp != NULL) {
				rate = get_modulation_name(get_hw_product_type(eut_cmd_id), result);
				ENG_LOG("wifi_eut: rate = %s, index = %d\n", rate, result);
				sprintf(rsp, "+SPWIFITEST:RATE=%s", rate);
			}

			break;

		case WIFI_EUT_CMD_GET_DECODEMODE:
		case WIFI_EUT_CMD_GET_GUARDINTER:
		case WIFI_EUT_CMD_GET_PKT_LEN:
		case WIFI_EUT_CMD_GET_PAYLOAD:
		case WIFI_EUT_CMD_GET_SBW:
		case WIFI_EUT_CMD_GET_TX_MODE:
		case WIFI_EUT_CMD_GET_RSSI:
		case WIFI_EUT_CMD_GET_BAND:
		case WIFI_EUT_CMD_GET_TX_POWER:
		case WIFI_EUT_CMD_GET_LNA_STATUS:
		case WIFI_EUT_CMD_GET_BW:
		case WIFI_EUT_CMD_GET_ANT:
			index = get_eut_cmd_index(eut_cmd_id);
			if (index < 0) {
				ENG_LOG("wifi_eut: %s %d, WIFI_EUT_CMD is out of range\n", __func__, __LINE__);
				return -1;
			}
			if (eut_cmd[index].at_rsp_fmt == NULL) {
				ENG_LOG("wifi_eut: %s %d ret = %d\n", __func__, __LINE__, ret);
				return -1;
			}

			if (rsp != NULL)
				sprintf(rsp, eut_cmd[index].at_rsp_fmt, result);

			break;

		default:
			if (result != 0)
				return -1;

			if (rsp != NULL)
				strcpy(rsp, AT_CMD_RESP_OK);

			ENG_LOG("wifi_eut: %s %d default handle(eut_cmd_id = %d)\n",
					__func__, __LINE__, eut_cmd_id);
			break;
		}
	} else {
		switch (eut_cmd_id) {
		case WIFI_EUT_CMD_GET_CHANNEL:
			ret = sscanf(output, "ret:primary_channel:%d,center_channel:%d:end", &ch1, &ch2);
			if (ret != 2) {
				ENG_LOG("wifi_eut: %s %d ret = %d\n", __func__, __LINE__, ret);
				return -1;
			}

			if (rsp != NULL)
				sprintf(rsp, "+SPWIFITEST:CH=%d,%d", ch1, ch2);

			break;

		case WIFI_EUT_CMD_GET_RXPACKCOUNT:
			ret = sscanf(output, "ret:regvalue:rx_end_count=%drx_err_end_count=%dfcs_fail_count=%d:end", &rx_count, &rx_err, &rx_fail);
			if (ret != 3) {
				ENG_LOG("wifi_eut: %s %d ret = %d\n", __func__, __LINE__, ret);
				return -1;
			}

			if (rsp != NULL)
				sprintf(rsp, "+SPWIFITEST:RXPACKCOUNT=%d,%d,%d", rx_count, rx_err, rx_fail);

			break;

		default:
			return -1;
		}
	}
	return 0;
}

/* description: parse the npi command output, and format the at command resp
 * param:
 *    eut_cmd_id: command id
 *    data: npi command output
 *    rsp: the buffer that response at command
 * return value:
 *    0: success
 *   -1: failed
 */
int fmt_npi_output(int eut_cmd_id, char *data, char *rsp)
{
	int i;
	int status, ret;
	char *output[MAX_SIZE];
	char *start, *end;
	int num = 0;

	for (num = 0, start = data; num < MAX_SIZE; num++){
		end = strchr(start, 0x0a);
		if (end == NULL)
			break;

		output[num] = strndup(start, end - start);
		ENG_LOG("wifi_eut: output[%d] = %s\n", num, output[num]);
		strip_white_space(output[num]);
		start = end + 1;
	}

	/* output[0] = ret:status%d:end */
	ret = sscanf(output[0], "ret:status%d:end", &status);
	if (ret != 1) {
		ENG_LOG("wifi_eut: %s %d(ret = %d, status = %d)\n",
				__func__, __LINE__, ret, status);
		ret = -1;
		goto out;
	}

	if (status != 0) {
		ret = -1;
		goto out;
	}

	switch (num) {
	case 1: /* one line output only */
		if (rsp != NULL)
			strcpy(rsp, AT_CMD_RESP_OK);

		ret = 0;
		break;

	case 2:
		ret = fmt_npi_output2(eut_cmd_id, output[1], rsp);
		break;

	default:
		ret = -1;
		break;
	}

out:
	for (i = 0; i < num; i++)
		free(output[i]);
	return ret;
}

/*
 * return value:
 *    0: success
 *   -1: failed
 */
static int run_npi_command(int eut_cmd_id, char *argv[], char *resp)
{
	char *p;
	int len, ret;
	int pfd[2];
	pid_t pid;
	char buf[256];


	if (argv == NULL) {
		ENG_LOG("wifi_eut: %s %d\n", __func__, __LINE__);
		return -1;
	}

	if (pipe(pfd) == -1) {
		ENG_LOG("wifi_eut: create pipe failed\n");
		return -1;
	}

	pid = fork();
	if (pid < 0)
	{
		ENG_LOG("wifi_eut: %s fork failed\n", __func__);
		close(pfd[0]);
		close(pfd[1]);
		return -1;
	}

	if (pid == 0) {
		close(pfd[0]);
		dup2(pfd[1], STDOUT_FILENO);
		execvp("/vendor/bin/iwnpi", argv);
		printf("child exec iwnpi failed\n");
		exit(0);
	} else {
		close(pfd[1]);
		memset(buf, 0x0, sizeof(buf));

		p = buf;
		len = sizeof(buf);

		//fixme: only read 256 bytes max
		while (1) {
			ret = read(pfd[0], p, len);
			if (ret <= 0)
				break;

			len -= ret;
			if (len <= 0)
				break;

			p += ret;
		}

		close(pfd[0]);
		ret = fmt_npi_output(eut_cmd_id, buf, resp);
		wait(NULL);
	}
	return ret;
}

int set_mac_addr(char *mac)
{
	int fd;
	int ret;
	char mac_addr[32] = {0};

	(void *) mac_addr;
	fd = open(NV_MAC_ADDR_PATH, O_CREAT | O_WRONLY, 0666);
	if (fd < 0)
		return -1;

	sprintf(mac_addr, "%02x:%02x:%02x:%02x:%02x:%02x",
			mac[0], mac[1], mac[2], mac[3], mac[4],mac[5]);

	ENG_LOG("wifi_eut: %s MAC=%s", __func__, mac_addr);
	ret = write(fd, mac_addr, strlen(mac_addr));
	close(fd);

	if (ret < 0)
		return -1;

	fd = open(CURRENT_MAC_ADDR_PATH, O_CREAT | O_WRONLY, 0666);
	if (fd < 0)
		return -1;

	ret = write(fd, mac_addr, strlen(mac_addr));
	close(fd);
	if (ret < 0)
		return -1;

	return 0;
}

static struct eut_cmd_t *at2eut(char *cmd)
{
	int index;
	char *chr;

	chr = strchr(cmd, '?');
	if (chr == NULL) {
		/* match cmd format: ATCOMAND,%d\r\n */
		chr = strchr(cmd, ',');
		if (chr == NULL) {
			/* match cmd format: ATCOMAND\r\n */
			chr = strchr(cmd, '\n');
			if (chr == NULL)
				return NULL;
		}
	}

	for (index = 0; eut_cmd[index].at_cmd != NULL; index++) {
		/* compare keywords: "EUT?"/"EUT," etc */
		if (!strncmp(eut_cmd[index].at_cmd, cmd, chr - cmd + 1)) {
			return &eut_cmd[index];
		}
	}

	return NULL;
}

int get_mac_addr(char *mac_addr)
{
	int fd;
	int ret;
	char buf[32];

	fd = open(NV_MAC_ADDR_PATH, O_RDONLY);
	if (fd < 0)
		return -1;

	if (read(fd, buf, sizeof(buf)) < 0) {
		close(fd);
		return -1;
	}

	close(fd);
	ENG_LOG("wifi_eut: %s MAC=%s\n", __func__, buf);
	ret = sscanf(buf, "%02x:%02x:%02x:%02x:%02x:%02x",
			&mac_addr[0], &mac_addr[1], &mac_addr[2], &mac_addr[3], &mac_addr[4], &mac_addr[5]);

	if (ret != 6)
		return -1;

	return 0;
}

/* description: process specical eut command
 * param:
 *     eut_cmd_id: eut command id
 *     at_cmd: at command
 *     at_rsp: response buffer
 *
 * return value:
 *     EUT_PROCESS_DONE: All commands have been processed correctly, and should not run any iwnpi command after this
 *     EUT_PROCESS_NONE: No special command to be processed.
 */
int process_special_eut_command(int eut_cmd_id, char *at_cmd, char *at_rsp)
{
	int ret;
	int index, cmd_id;
	int stat, mode, pkt_count;
	char *drv_path;
	char buf[64];
	char *argv[8];

	switch (eut_cmd_id) {
	case WIFI_EUT_CMD_GET_EUT_MODE:
		stat = get_ext_data(eut_cmd_id);
		if (stat != 0 && stat != 1) {
			ENG_LOG("wifi_eut: Unknown mode (line:%d)", __LINE__);
			goto err;
			}

		sprintf(at_rsp, "+SPWIFITEST:EUT=%d", stat);
		return EUT_PROCESS_DONE;

	case WIFI_EUT_CMD_SET_TX_MODE:
		/* CW mode */
		if (get_ext_data(eut_cmd_id) == 0) {
			/* iwnpi wlan0 tx_stop */
			if (get_ext_data(WIFI_EUT_CMD_GET_TX) == WIFI_TX_START) {
				ENG_LOG("tx status is : start, need stop it first\n");
				cmd_id = at2npi("XTXCTL,0\r\n", argv);
				if (cmd_id < 0)
					goto err;

				ret = run_npi_command(cmd_id, argv, NULL);
				if (ret < 0)
					goto err;
			}
			/* iwnpi wlan0 sin_wave */
			cmd_id = at2npi("XSINWAVE,1\r\n", argv);
			if (cmd_id < 0)
				goto err;

			ret = run_npi_command(cmd_id, argv, NULL);
			if (ret < 0)
				goto err;

			strcpy(at_rsp, AT_CMD_RESP_OK);
			return EUT_PROCESS_DONE;
		}

		break;

	case WIFI_EUT_CMD_SET_TX:
		index = get_eut_cmd_index(eut_cmd_id);
		if (index == -1)
			goto err;

		ret = sscanf(at_cmd, eut_cmd[index].at_cmd, &stat, &mode, &pkt_count);
		if (ret < 0) {
			ENG_LOG("wifi_eut: %s %d ret = %d\n", __func__, __LINE__, ret);
			goto err;
		}

		/* turn off tx*/
		if (stat == 0) {
			cmd_id = at2npi("XTXCTL,0\r\n", argv);
			if (cmd_id < 0) {
				ENG_LOG("wifi_eut: %s %d\n", __func__, __LINE__);
				goto err;
			}

			if (run_npi_command(cmd_id, argv, NULL) < 0)
				goto err;

			set_ext_data(WIFI_EUT_CMD_GET_TX, WIFI_TX_STOP);
			strcpy(at_rsp, AT_CMD_RESP_OK);

			return EUT_PROCESS_DONE;

		} else if (stat == 1) {
			switch (ret) {
			case 2:
			case 3:
				/*mode == 0; continue mode*/
				if (mode == 0)
					pkt_count = 0;
				else {
					if (ret != 3)
						goto err;
				}

				sprintf(buf, "XSET_TX_COUNT,%d\r\n", pkt_count);
				cmd_id = at2npi(buf, argv);
				if (cmd_id < 0)
					goto err;

				if (run_npi_command(cmd_id, argv, NULL) < 0)
					goto err;

				/* ignore break */
			case 1:
				cmd_id = at2npi("XTXCTL,1\r\n", argv);
				if (cmd_id < 0)
					goto err;

				if (run_npi_command(cmd_id, argv, NULL) < 0)
					goto err;

				set_ext_data(WIFI_EUT_CMD_GET_TX, WIFI_TX_START);
				strcpy(at_rsp, AT_CMD_RESP_OK);
				return EUT_PROCESS_DONE;


			default:
				ENG_LOG("wifi_eut: %s %d, ret = %d\n", __func__, __LINE__, ret);
				goto err;
			}
		} else {
			goto err;
		}

		break;

	/* Enter EUT mode*/
	case WIFI_EUT_CMD_SET_EUT_MODE:
		// TODO: power on cp2 for marlin
		if (get_ext_data(eut_cmd_id) == WIFI_EUT_ENTER) {
			switch (get_hw_product_type(eut_cmd_id)) {
			case HW_PRODUCT_MARLIN:
				drv_path = "lib/modules/sprdwl.ko";
				break;
			default:
				drv_path = "lib/modules/sprdwl_ng.ko";
			}

			if (load_driver(drv_path) < 0)
				goto err;
		}

		break;

	case WIFI_EUT_CMD_SET_NETMODE:
		strcpy(at_rsp, AT_CMD_RESP_OK);
		return EUT_PROCESS_DONE;

	case WIFI_EUT_CMD_GET_RX:
		sprintf(at_rsp, "+SPWIFITEST:RX=%d", get_ext_data(eut_cmd_id));
		return EUT_PROCESS_DONE;

	case WIFI_EUT_CMD_GET_TX:
		sprintf(at_rsp, "+SPWIFITEST:TX=%d", get_ext_data(eut_cmd_id));
		return EUT_PROCESS_DONE;

	default:
		break;
	}
	return EUT_PROCESS_NONE;

err:
	strcpy(at_rsp, AT_CMD_RESP_ERR);
	return EUT_PROCESS_DONE;
}

/* description: do some special process after iwnpi command
 * return value:
 *     0: success
 *    -1: error
 */

int process_after_npi_command(int eut_cmd_id)
{
	char buf[32];
	char *module_name;

	switch (eut_cmd_id) {
		case WIFI_EUT_CMD_SET_EUT_MODE:
			/* Exit EUT mode: unload wifi driver */
			if (get_ext_data(eut_cmd_id) == WIFI_EUT_EXIT) {
				switch (get_hw_product_type(eut_cmd_id)) {
				case HW_PRODUCT_MARLIN:
					module_name = "sprdwl";
					break;

				default:
					module_name = "sprdwl_ng";
					break;
				}

				sprintf(buf, "rmmod %s", module_name);
				if (system(buf) < 0) {
					ENG_LOG("wifi_eut: %s %d\n", __func__, __LINE__);
					return -1;
				}

				/* exit eut mode successful */
				set_ext_data(WIFI_EUT_CMD_GET_EUT_MODE, WIFI_EUT_EXIT);
			} else {
				set_ext_data(WIFI_EUT_CMD_GET_EUT_MODE, WIFI_EUT_ENTER);
			}

			break;

		case WIFI_EUT_CMD_SET_RX:
			/* update rx status */
			set_ext_data(WIFI_EUT_CMD_GET_RX, get_ext_data(eut_cmd_id));
			break;

		default:
			break;
	}
	return 0;
}

/* description: process at command and return at response
 * param:
 *     diag_cmd: diag command from PC
 *     at_rsp: at command result response to PC
 * return :
 *     0: success
 *    -1: failure
 */


/* diag cmd format
 * +-----------------------+-------+------+
 * | head |   diag header  | data  | tail |
 * +------+----------------+-------+------+
 * |  7E  | SN + LEN + ... | ASCII |  7E  |
 * +------+----------------+-------+------+
 */
int wifi_eut_hdlr(char *diag_cmd, char *at_rsp)
{
	int ret;
	int eut_cmd_id;
	int hdr_len;
	char *argv[8], *at_cmd;
	int len;
	char mac[6];
	struct eut_cmd_t *eut;

	hdr_len = sizeof(MSG_HEAD_T);
	at_cmd = diag_cmd + 1 + hdr_len;
	len = strlen(at_cmd) - 1;
	at_cmd[len] = 0;

	if (strncmp(WIFI_EUT_KEYWORD, at_cmd, WIFI_EUT_KEYWORD_LEN) != 0)
		goto err;

	at_cmd += WIFI_EUT_KEYWORD_LEN;
	eut = at2eut(at_cmd);
	if (eut == NULL) {
		ENG_LOG("wifi_eut: %s, No at command matched\n", __func__);
		goto err;
	}
	eut_cmd_id = at2npi(at_cmd, argv);
	if (eut_cmd_id < 0) {
		ENG_LOG("wifi_eut: %s %d\n", __func__, __LINE__);
		goto err;
	}

	switch (eut_cmd_id) {
	case WIFI_EUT_CMD_SET_MAC:
		ret = sscanf(at_cmd, eut->at_cmd,
				&mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
		if (ret != 6)
			goto err;

		if(set_mac_addr(mac) < 0)
			goto err;

		strcpy(at_rsp, AT_CMD_RESP_OK);
		ENG_LOG("wifi_eut: resp = %s\n", at_rsp);
		return 0;

	case WIFI_EUT_CMD_GET_MAC:
		if(get_mac_addr(mac) < 0)
			goto err;

		sprintf(at_rsp, "+SPWIFITEST:MAC=%02x%02x%02x%02x%02x%02x",
				mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

		ENG_LOG("wifi_eut: resp = %s\n", at_rsp);
		return 0;

	default:
		break;
	}

	ENG_LOG("wifi_eut: eut_cmd_id = %d\n", eut_cmd_id);
	if (get_ext_data(WIFI_EUT_CMD_GET_EUT_MODE) == WIFI_EUT_EXIT) {
		if (eut_cmd_id != WIFI_EUT_CMD_SET_EUT_MODE &&
			eut_cmd_id != WIFI_EUT_CMD_GET_EUT_MODE)
		goto err;
	}

	ret = process_special_eut_command(eut_cmd_id, at_cmd, at_rsp);
	if (ret == EUT_PROCESS_DONE) {
		/* EUT_PROCESS_DONE means EUT command that has no iwnpi command or
		 * multi iwnpi commands to be run has been processed correctly,
		 * and at command response message has been format in at_rsp;
		 */
		ENG_LOG("wifi_eut: resp = %s\n", at_rsp);
		return 0;
	}

	ret = run_npi_command(eut_cmd_id, argv, at_rsp);
	if (ret < 0)
		goto err;

	ret = process_after_npi_command(eut_cmd_id);
	if (ret < 0)
		goto err;

	ENG_LOG("wifi_eut: resp = %s\n", at_rsp);
	return 0;

err:
	strcpy(at_rsp, AT_CMD_RESP_ERR);
	ENG_LOG("wifi_eut: resp = %s\n", at_rsp);
	return -1;
}

void wifi_eut_init()
{
	char name[PROPERTY_VALUE_MAX] = {0};
	int hw_product_type;

	property_get(WCN_HARDWARE_PRODUCT, name, "invalid");

	//fixme: just only support marlin/marlin2/marlin3
	if (strcmp("marlin", name) == 0)
		hw_product_type = HW_PRODUCT_MARLIN;
	else if (strcmp("marlin3", name) == 0)
		hw_product_type = HW_PRODUCT_MARLIN3;
	else
		hw_product_type = HW_PRODUCT_MARLIN2;
	/* depends on hw product type */
	set_hw_product_type(WIFI_EUT_CMD_SET_RATE, hw_product_type);
	set_hw_product_type(WIFI_EUT_CMD_GET_RATE, hw_product_type);
	set_hw_product_type(WIFI_EUT_CMD_SET_CHANNEL, hw_product_type);
	set_hw_product_type(WIFI_EUT_CMD_GET_CHANNEL, hw_product_type);

	/* set default eut mode: (0: EXIT EUT; 1: ENTER EUT) */
	set_ext_data(WIFI_EUT_CMD_GET_EUT_MODE, WIFI_EUT_EXIT);

	/* set default rx status: (0: rx_stop, 1: rx_start)*/
	set_ext_data(WIFI_EUT_CMD_GET_RX, WIFI_RX_STOP);

	/* set default tx status: (0: tx_stop, 1: tx_start)*/
	set_ext_data(WIFI_EUT_CMD_GET_TX, WIFI_TX_STOP);
}

void register_this_module(struct eng_callback * reg)
{
	ALOGD("file:%s, func:%s\n", __FILE__, __func__);
	wifi_eut_init();
	sprintf(reg->at_cmd, "%s", "AT+SPWIFITEST");
	reg->eng_linuxcmd_func = wifi_eut_hdlr;
	ALOGD("module cmd:%s\n", reg->at_cmd);
}
