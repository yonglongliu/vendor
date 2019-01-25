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

static struct wifi_eut_t *get_eut(int eut_cmd_id);

#define SET_EXT_DATA(eut_id, data) \
do { \
	if (get_eut(eut_id) != NULL) \
		get_eut(eut_id)->ext_data = (data); \
} while(0)

#define SET_PRODUCT_TYPE(eut_id, type) \
do { \
	if (get_eut(eut_id) != NULL) \
		get_eut(eut_id)->wcn_product_type = (type); \
} while(0)

#define SET_ANDROID_VER(eut_id, ver) \
do { \
	if (get_eut(eut_id) != NULL) \
		get_eut(eut_id)->android_version = (ver); \
} while(0)

#define GET_EXT_DATA(eut_id) \
	get_eut(eut_id) != NULL ? get_eut(eut_id)->ext_data : -1

#define GET_PRODUCT_TYPE(eut_id) \
	get_eut(eut_id) != NULL ? get_eut(eut_id)->wcn_product_type : -1

#define GET_ANDROID_VER(eut_id) \
	get_eut(eut_id) != NULL ? get_eut(eut_id)->android_version : -1

/* new rate table for marlin3 */
static struct wifi_rate_t g_wifi_rate_table[] = {
    {0, "DSSS-1"},    /*1M_Long*/      {1, "DSSS-2"},    /*2M_Long*/      {2, "DSSS-2S"},   /*2M_Short*/
	{3, "CCK-5.5"},   /*5.5M_Long*/    {4, "CCK-5.5S"},  /*5.5M_Short*/   {5, "CCK-11"},    /*11M_Long*/
	{6, "CCK-11S"},   /*11M_Short*/    {7, "OFDM-6"},    /*6M*/           {8, "OFDM-9"},    /*9M*/
	{9, "OFDM-12"},   /*12M*/          {10, "OFDM-18"},  /*18M*/          {11, "OFDM-24"},  /*24M*/
	{12, "OFDM-36"},  /*36M*/          {13, "OFDM-48"},  /*48M*/          {14, "OFDM-54"},  /*54M*/
    {15, "MCS-0"},    /*HT_MCS0*/      {16, "MCS-1"},    /*HT_MCS1*/      {17, "MCS-2"},    /*HT_MCS2*/
	{18, "MCS-3"},    /*HT_MCS3*/      {19, "MCS-4"},    /*HT_MCS4*/      {20, "MCS-5"},    /*HT_MCS5*/
	{21, "MCS-6"},    /*HT_MCS6*/      {22, "MCS-7"},    /*HT_MCS7*/      {23, "MCS-8"},    /*HT_MCS8*/
	{24, "MCS-9"},    /*HT_MCS9*/      {25, "MCS-10"},   /*HT_MCS10*/     {26, "MCS-11"},   /*HT_MCS11*/
	{27, "MCS-12"},   /*HT_MCS12*/     {28, "MCS-13"},   /*HT_MCS13*/     {29, "MCS-14"},   /*HT_MCS14*/
    {30, "MCS-15"},   /*HT_MCS15*/     {31, "MCS0_1SS"}, /*VHT_MCS0_1SS*/ {32, "MCS1_1SS"}, /*VHT_MCS1_1SS*/
	{33, "MCS2_1SS"}, /*VHT_MCS2_1SS*/ {34, "MCS3_1SS"}, /*VHT_MCS3_1SS*/ {35, "MCS4_1SS"}, /*VHT_MCS4_1SS*/
	{36, "MCS5_1SS"}, /*VHT_MCS5_1SS*/ {37, "MCS6_1SS"}, /*VHT_MCS6_1SS*/ {38, "MCS7_1SS"}, /*VHT_MCS7_1SS*/
	{39, "MCS8_1SS"}, /*VHT_MCS8_1SS*/ {40, "MCS9_1SS"}, /*VHT_MCS9_1SS*/ {41, "MCS0_2SS"}, /*VHT_MCS0_2SS*/
	{42, "MCS1_2SS"}, /*VHT_MCS1_2SS*/ {43, "MCS2_2SS"}, /*VHT_MCS2_2SS*/ {44, "MCS3_2SS"}, /*VHT_MCS3_2SS*/
    {45, "MCS4_2SS"}, /*VHT_MCS4_2SS*/ {46, "MCS5_2SS"}, /*VHT_MCS5_2SS*/ {47, "MCS6_2SS"}, /*VHT_MCS6_2SS*/
	{48, "MCS7_2SS"}, /*VHT_MCS7_2SS*/ {49, "MCS8_2SS"}, /*VHT_MCS8_2SS*/
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

static struct wifi_eut_t wifi_eut[] = {
	/*fixme: should be load/unload wifi driver before/after iwnpi command*/
	{
		.at_pattern = "EUT,%d\r\n",
		.at_rsp_fmt = NULL,
		/* 0: stop, 1: start */
		.npi_cmd = "iwnpi wlan0 %s",
		.eut_cmd_id = WIFI_EUT_CMD_SET_EUT_MODE,
	},
	{
		.at_pattern = "EUT?\r\n",
		.at_rsp_fmt = "+SPWIFITEST:EUT=%d",
		.npi_cmd = NULL,
		.eut_cmd_id = WIFI_EUT_CMD_GET_EUT_MODE,
	},
	{
		.at_pattern = "ANT,%d\r\n",
		.at_rsp_fmt = NULL,
		.npi_cmd = "iwnpi wlan0 set_chain %d",
		.eut_cmd_id = WIFI_EUT_CMD_SET_ANT,
	},
	{
		.at_pattern = "ANT?\r\n",
		.at_rsp_fmt = "+SPWIFITEST:ANT=%d",
		.npi_cmd = "iwnpi wlan0 get_chain",
		.eut_cmd_id = WIFI_EUT_CMD_GET_ANT,
	},
	{
		.at_pattern = "BAND,%d\r\n",
		.at_rsp_fmt = NULL,
		.npi_cmd = "iwnpi wlan0 set_band %d",
		.eut_cmd_id = WIFI_EUT_CMD_SET_BAND,
	},
	{
		.at_pattern = "BAND?\r\n",
		.at_rsp_fmt = "+SPWIFITEST:BAND=%d",
		.npi_cmd = "iwnpi wlan0 get_band",
		.eut_cmd_id = WIFI_EUT_CMD_GET_BAND,
	},
	{
		.at_pattern = "BW,%d\r\n",
		.at_rsp_fmt = NULL,
		.npi_cmd = "iwnpi wlan0 set_cbw %d",
		.eut_cmd_id = WIFI_EUT_CMD_SET_BW,
	},
	{
		.at_pattern = "BW?\r\n",
		.at_rsp_fmt = "+SPWIFITEST:BW=%d",
		.npi_cmd = "iwnpi wlan0 get_cbw",
		.eut_cmd_id = WIFI_EUT_CMD_GET_BW,
	},
	{
		.at_pattern = "SBW,%d\r\n",
		.at_rsp_fmt = NULL,
		.npi_cmd = "iwnpi wlan0 set_sbw %d",
		.eut_cmd_id = WIFI_EUT_CMD_SET_SBW,
	},
	{
		.at_pattern = "SBW?\r\n",
		.at_rsp_fmt = "+SPWIFITEST:SBW=%d",
		.npi_cmd = "iwnpi wlan0 get_sbw",
		.eut_cmd_id = WIFI_EUT_CMD_GET_SBW,
	},
	{
		.at_pattern = "CH,%d,%d\r\n",
		.at_rsp_fmt = NULL,
		.npi_cmd = "iwnpi wlan0 set_channel %d %d",
		.eut_cmd_id = WIFI_EUT_CMD_SET_CHANNEL,
	},
	{
		.at_pattern = "CH?\r\n",
		.at_rsp_fmt = "+SPWIFITEST:CH=%d,%d",
		.npi_cmd = "iwnpi wlan0 get_channel",
		.eut_cmd_id = WIFI_EUT_CMD_GET_CHANNEL,
	},
	{
		.at_pattern = "RATE,%s\r\n",
		.at_rsp_fmt = NULL,
		.npi_cmd = "iwnpi wlan0 set_rate %d",
		.eut_cmd_id = WIFI_EUT_CMD_SET_RATE,
	},
	{
		.at_pattern = "RATE?\r\n",
		.at_rsp_fmt = "+SPWIFITEST:RATE=%s",
		.npi_cmd = "iwnpi wlan0 get_rate",
		.eut_cmd_id = WIFI_EUT_CMD_GET_RATE,
	},
	{
		.at_pattern = "PREAMBLE,%d\r\n",
		.at_rsp_fmt = NULL,
		.npi_cmd = "iwnpi wlan0 set_preamble %d",
		.eut_cmd_id = WIFI_EUT_CMD_SET_PREAMBLE,
	},
	{
		.at_pattern = "PAYLOAD,%d\r\n",
		.at_rsp_fmt = NULL,
		.npi_cmd = "iwnpi wlan0 set_payload %d",
		.eut_cmd_id = WIFI_EUT_CMD_SET_PAYLOAD,
	},
	{
		.at_pattern = "PAYLOAD?\r\n",
		.at_rsp_fmt = "+SPWIFITEST:PAYLOAD=%d",
		.npi_cmd = "iwnpi wlan0 get_payload",
		.eut_cmd_id = WIFI_EUT_CMD_GET_PAYLOAD,
	},
	{
		.at_pattern = "PKTLEN,%d\r\n",
		.at_rsp_fmt = NULL,
		.npi_cmd = "iwnpi wlan0 set_pkt_len %d",
		.eut_cmd_id = WIFI_EUT_CMD_SET_PKT_LEN,
	},
	{
		.at_pattern = "PKTLEN?\r\n",
		.at_rsp_fmt = "+SPWIFITEST:PKTLEN=%d",
		.npi_cmd = "iwnpi wlan0 get_pkt_len",
		.eut_cmd_id = WIFI_EUT_CMD_GET_PKT_LEN,
	},
	{
		.at_pattern = "GUARDINTERVAL,%d\r\n",
		.at_rsp_fmt = NULL,
		.npi_cmd = "iwnpi wlan0 set_gi %d",
		.eut_cmd_id = WIFI_EUT_CMD_SET_GUARDINTER,
	},
	{
		.at_pattern = "GUARDINTERVAL?\r\n",
		.at_rsp_fmt = "+SPWIFITEST:GUARDINTERVAL=%d",
		.npi_cmd = "iwnpi wlan0 get_gi",
		.eut_cmd_id = WIFI_EUT_CMD_GET_GUARDINTER,
	},
	{
		.at_pattern = "TXPWRLV,%d\r\n",
		.at_rsp_fmt = NULL,
		.npi_cmd = "iwnpi wlan0 set_tx_power %d",
		.eut_cmd_id = WIFI_EUT_CMD_SET_TX_POWER,
	},
	{
		.at_pattern = "TXPWRLV?\r\n",
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
		.at_pattern = "TXMODE,%d\r\n",
		.at_rsp_fmt = NULL,
		.npi_cmd = NULL,
		.eut_cmd_id = WIFI_EUT_CMD_SET_TX_MODE,
	},
	{
		.at_pattern = "TXMODE?\r\n",
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
		.at_pattern = "TX,%d,%d,%d\r\n",
		.at_rsp_fmt = NULL,
		.npi_cmd = NULL,
		.eut_cmd_id = WIFI_EUT_CMD_SET_TX,
	},
	{
		.at_pattern = "TX?\r\n",
		.at_rsp_fmt = "+SPWIFITEST:TX=%d",
		.npi_cmd = NULL,
		.eut_cmd_id = WIFI_EUT_CMD_GET_TX,
	},
	{
		.at_pattern = "DECODEMODE,%d\r\n",
		.at_rsp_fmt = NULL,
		.npi_cmd = "iwnpi wlan0 set_fec %d",
		.eut_cmd_id = WIFI_EUT_CMD_SET_DECODEMODE,
	},
	{
		.at_pattern = "DECODEMODE?\r\n",
		.at_rsp_fmt = "+SPWIFITEST:DECODEMODE=%d",
		.npi_cmd = "iwnpi wlan0 get_fec",
		.eut_cmd_id = WIFI_EUT_CMD_GET_DECODEMODE,
	},
	/* fixme: not implement */
	{
		.at_pattern = "MACFILTER,%d\r\n",
		.at_rsp_fmt = NULL,
		.npi_cmd = NULL,
		.eut_cmd_id = WIFI_EUT_CMD_SET_MAC_FILTER,
	},
	/* fixme: not implement */
	{
		.at_pattern = "MACFILTER?\r\n",
		.at_rsp_fmt = NULL,
		.npi_cmd = "iwnpi wlan0 get_macfilter",
		.eut_cmd_id = WIFI_EUT_CMD_GET_MAC_FILTER,
	},
	{
		.at_pattern = "LNA,%d\r\n",
		.at_rsp_fmt = NULL,
		.npi_cmd = "iwnpi wlan0 %s",
		.eut_cmd_id = WIFI_EUT_CMD_SET_LNA_STATUS,
	},
	{
		.at_pattern = "LNA?\r\n",
		.at_rsp_fmt = "+SPWIFITEST:LNA=%d",
		.npi_cmd = "iwnpi wlan0 lna_status",
		.eut_cmd_id = WIFI_EUT_CMD_GET_LNA_STATUS,
	},
	{
		.at_pattern = "RX,%d\r\n",
		.at_rsp_fmt = NULL,
		.npi_cmd = "iwnpi wlan0 %s",
		.eut_cmd_id = WIFI_EUT_CMD_SET_RX,
	},
	{
		.at_pattern = "RX?\r\n",
		.at_rsp_fmt = "+SPWIFITEST:RX=%d",
		.npi_cmd = NULL,
		.eut_cmd_id = WIFI_EUT_CMD_GET_RX,
	},
	{
		.at_pattern = "RSSI?\r\n",
		.at_rsp_fmt = "+SPWIFITEST:RSSI=%d",
		.npi_cmd = "iwnpi wlan0 get_rssi",
		.eut_cmd_id = WIFI_EUT_CMD_GET_RSSI,
	},
	{
		.at_pattern = "RXPACKCOUNT?\r\n",
		.at_rsp_fmt = "+SPWIFITEST:RXPACKCOUNT=%d,%d,%d",
		.npi_cmd = "iwnpi wlan0 get_rx_ok",
		.eut_cmd_id = WIFI_EUT_CMD_GET_RXPACKCOUNT,
	},
	/* invalid for marlin3 */
	{
		.at_pattern = "NETMODE,%d\r\n",
		.npi_cmd = NULL,
		.eut_cmd_id = WIFI_EUT_CMD_SET_NETMODE,
	},
	{
		.at_pattern = "MAC,%02X%02X%02X%02X%02X%02X\r\n",
		.npi_cmd = NULL,
		.eut_cmd_id = WIFI_EUT_CMD_SET_MAC,
	},
	{
		.at_pattern = "MAC?\r\n",
		.npi_cmd = NULL,
		.eut_cmd_id = WIFI_EUT_CMD_GET_MAC,
	},
	{
		.at_pattern = "CP2INFO?\r\n",
		.npi_cmd = NULL,
		.eut_cmd_id = WIFI_EUT_CMD_GET_CP2INFO,
	},
	{
		.at_pattern = NULL,
		.eut_cmd_id = WIFI_EUT_CMD_DEFAULT,
	}
};

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
	int i;
	FILE *fp;
	char line[256];
	char module[64];
	static int is_driver_loaded;


	for (i = 0; *mod_name && *mod_name != '.'; i++) {
		if (i + 1 == sizeof(module))
			break;

		module[i] = *mod_name++;
	}

	module[i] = 0;

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

static int rmmod_drv(char *mod_name)
{
	int i;
	char cmd[64] = "rmmod";
	static int is_driver_loaded;

	i = strlen(cmd);
	cmd[i++] = 0x20; /* white space */

	while (*mod_name && *mod_name != '.') {
		cmd[i++] = *mod_name++;
		if (i + 1 == sizeof(cmd))
			break;
	}

	cmd[i] = '\0';

	ENG_LOG("wifi_eut: %s", cmd);
	if (system(cmd) < 0)
		return -1;

	return 0;
}

/* description: insmod driver
 * param:
 *     path: module path
 *
 * return value:
 *     0: success
 *    -1: error
 */
static int load_driver(char *path)
{
	int ret;
	int fd;
	void *addr;
	char *mod_name;
	char cmd[128];

	if (path == NULL) {
		ENG_LOG("wifi_eut: %s path is NULL", __func__);
		return -1;
	}

	mod_name = strrchr(path, '/');
	if (mod_name == NULL)
		return -1;

	if (is_module_exist(++mod_name) == 1) {
		if (rmmod_drv(mod_name) < 0)
			return -1;
	}

	sprintf(cmd, "insmod %s", path);
	ret = system(cmd);
	if (ret < 0){
		ENG_LOG("wifi_eut: load drv(%s) failed, ret = %d\n", path, ret);
		return -1;
	}

	ENG_LOG("wifi_eut: load drv(%s) success, ret = %d", path, ret);
	return 0;
}

/* description: Get the rate index
 * param:
 *     prod_type: wcn product type
 *     modulation: ascii code of modulation
 *
 * return value:
 *     return -1 if error, else return index;
 */
static int get_rate_index(int prod_type, char *modulation)
{
	int i;
	struct wifi_rate_t *rate_tab;

	if (modulation == NULL)
		return -1;

	switch(prod_type) {
	case WCN_PRODUCT_MARLIN3:
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
	case WCN_PRODUCT_MARLIN3:
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

/* description: Get wifi_eut_t struct
 * param:
 *     eut_cmd_id: eut_id
 *
 * return value:
 *     return NULL if error, else return eut struct;
 */
static struct wifi_eut_t *get_eut(int eut_cmd_id)
{
	int i;
	for (i = 0; wifi_eut[i].at_pattern != NULL; i++)
		if (eut_cmd_id == wifi_eut[i].eut_cmd_id)
			return &wifi_eut[i];
	return NULL;
}

/* description: translate at command into iwnpi command
 * param:
 *     eut: wifi_eut_t struct
 *     at_cmd: ascii code of at command
 *     npi_cmd_buf: write npi command into npi_cmd_buf
 *
 * return value:
 *     0: success
 *    -1: failed to get iwnpi command
 */
static int at2npi(struct wifi_eut_t *eut, char *at_cmd, char *npi_cmd_buf)
{
	int ret;
	int val;
	int ch1, ch2;
	char rate_buf[16];
	int rate_index;

	if (eut->npi_cmd == NULL)
		return -1;

	switch (eut->eut_cmd_id) {
	case WIFI_EUT_CMD_SET_EUT_MODE:
		ret = sscanf(at_cmd, eut->at_pattern, &val);
		if (ret != 1)
			return -1;

		if (val != 0 && val != 1)
			return -1;

		eut->ext_data = val;
		sprintf(npi_cmd_buf, eut->npi_cmd, val ? "start" : "stop");
		break;

	case WIFI_EUT_CMD_SET_RX:
		if (GET_EXT_DATA(WIFI_EUT_TX_STATUS) == WIFI_TX_START) {
			ENG_LOG("wifi_eut: ERR: TX STATUS is ON!");
			return -1;
		}

		ret = sscanf(at_cmd, eut->at_pattern, &val);
		if (ret != 1) {
			ENG_LOG("wifi_eut: %s %d\n", __func__, __LINE__);
			return -1;
		}

		if (val != 0 && val != 1)
			return -1;


		eut->ext_data = val;
		sprintf(npi_cmd_buf, eut->npi_cmd, val == 1 ? "rx_start" : "rx_stop");
		break;

	case WIFI_EUT_CMD_SET_CHANNEL:
		ret = sscanf(at_cmd, "CH,%d,%d\r\n", &ch1, &ch2);
		if (ret == 2) {
			sprintf(npi_cmd_buf, "iwnpi wlan0 set_channel %d %d", ch1, ch2);
		} else if (ret == 1) {
			if (eut->wcn_product_type == WCN_PRODUCT_MARLIN3)
				sprintf(npi_cmd_buf, "iwnpi wlan0 set_channel %d %d", ch1, ch1);
			else
				sprintf(npi_cmd_buf, "iwnpi wlan0 set_channel %d", ch1);
		} else
			return -1;

		break;


	case WIFI_EUT_CMD_SET_LNA_STATUS:
		ret = sscanf(at_cmd, eut->at_pattern, &val);
		if (ret != 1)
			return -1;

		if (val != 0 && val != 1)
			return -1;

		eut->ext_data = val;
		sprintf(npi_cmd_buf, eut->npi_cmd, (val == 1) ? "lna_on" : "lna_off");
		break;

	case WIFI_EUT_CMD_SET_RATE:
		ret = sscanf(at_cmd, eut->at_pattern, rate_buf);
		if (ret != 1) {
			ENG_LOG("wifi_eut: %s %d\n", __func__, __LINE__);
			return -1;
		}

		rate_index = get_rate_index(eut->wcn_product_type, rate_buf);
		ENG_LOG("wifi_eut: %s %d, rate = %s, rate_index = %d\n",
				__func__, __LINE__, rate_buf, rate_index);
		if (rate_index < 0) {
			return -1;
		}

		sprintf(npi_cmd_buf, eut->npi_cmd, rate_index);
		break;

	/* Only one paramter with int type */
	case WIFI_EUT_CMD_SET_PREAMBLE:
	case WIFI_EUT_CMD_SET_SBW:
	case WIFI_EUT_CMD_SET_BW:
	case WIFI_EUT_CMD_SET_BAND:
	case WIFI_EUT_CMD_SET_ANT:
	case WIFI_EUT_CMD_SET_PAYLOAD:
	case WIFI_EUT_CMD_SET_PKT_LEN:
	case WIFI_EUT_CMD_SET_GUARDINTER:
	case WIFI_EUT_CMD_SET_TX_POWER:
	case WIFI_EUT_CMD_SET_DECODEMODE:
		ret = sscanf(at_cmd, eut->at_pattern, &val);
		if (ret != 1)
			return -1;

		sprintf(npi_cmd_buf, eut->npi_cmd, val);
		break;

	case WIFI_EUT_CMD_GET_PAYLOAD:
	case WIFI_EUT_CMD_GET_RATE:
	case WIFI_EUT_CMD_GET_CHANNEL:
	case WIFI_EUT_CMD_GET_BW:
	case WIFI_EUT_CMD_GET_BAND:
	case WIFI_EUT_CMD_GET_ANT:
	case WIFI_EUT_CMD_GET_SBW:
	case WIFI_EUT_CMD_GET_PKT_LEN:
	case WIFI_EUT_CMD_GET_GUARDINTER:
	case WIFI_EUT_CMD_GET_TX_POWER:
	case WIFI_EUT_CMD_GET_TX_MODE:
	case WIFI_EUT_CMD_GET_DECODEMODE:
	case WIFI_EUT_CMD_GET_LNA_STATUS:
	case WIFI_EUT_CMD_GET_RXPACKCOUNT:
	case WIFI_EUT_CMD_GET_RSSI:
		strcpy(npi_cmd_buf, eut->npi_cmd);
		break;

	default:
		return -1;
	}

	return 0;
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

/* description: parse npi response and format to at response
 * param:
 *     eut: wifi_eut_t struct
 *     npi_output: npi result
 *     rsp: format at response into rsp buff
 *
 * return value:
 *     0: npi command execute success
 *    -1: npi command failed
 */
static int npi_result_to_at_resp(struct wifi_eut_t *eut, char *npi_output, char *rsp)
{
	int ret;
	int result;
	char *rate;
	int ch1, ch2;
	int rx_count, rx_err, rx_fail;
	char output[256] = {0};

	if (eut == NULL || rsp == NULL)
		return -1;

	strncpy(output, npi_output, sizeof(output));
	strip_white_space(output);

	ret = sscanf(output, "ret:%d:end", &result);
	if (ret == 1) {
		switch (eut->eut_cmd_id) {
		case WIFI_EUT_CMD_GET_CHANNEL:
			if (rsp != NULL)
				sprintf(rsp, "+SPWIFITEST:CH=%d", result);
			break;

		case WIFI_EUT_CMD_GET_RATE:
			if (rsp != NULL) {
				rate = get_modulation_name(eut->wcn_product_type, result);
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
			if (eut->at_rsp_fmt == NULL) {
				ENG_LOG("wifi_eut: %s %d ret = %d\n", __func__, __LINE__, ret);
				return -1;
			}

			sprintf(rsp, eut->at_rsp_fmt, result);

			break;

		default:
			if (result != 0)
				return -1;

			strcpy(rsp, AT_CMD_RESP_OK);

			ENG_LOG("wifi_eut: %s %d default handle(eut_cmd_id = %d)\n",
					__func__, __LINE__, eut->eut_cmd_id);
			break;
		}
	} else {
		switch (eut->eut_cmd_id) {
		case WIFI_EUT_CMD_GET_CHANNEL:
			ret = sscanf(output, "ret:primary_channel:%d,center_channel:%d:end", &ch1, &ch2);
			if (ret != 2) {
				ENG_LOG("wifi_eut: %s %d ret = %d\n", __func__, __LINE__, ret);
				return -1;
			}

			sprintf(rsp, "+SPWIFITEST:CH=%d,%d", ch1, ch2);

			break;

		case WIFI_EUT_CMD_GET_RXPACKCOUNT:
			ret = sscanf(output, "ret:regvalue:rx_end_count=%drx_err_end_count=%dfcs_fail_count=%d:end",
					&rx_count, &rx_err, &rx_fail);
			if (ret != 3) {
				ENG_LOG("wifi_eut: %s %d ret = %d\n", __func__, __LINE__, ret);
				return -1;
			}

			sprintf(rsp, "+SPWIFITEST:RXPACKCOUNT=%d,%d,%d", rx_count, rx_err, rx_fail);
			break;

		default:
			return -1;
		}
	}

	return 0;
}

/* description: read a line from npi response result
 * param:
 *    data: npi output result
 *    buf: read a line into buffer
 *    buf_len: buffer size

 * return value:
 *    On success, the number of bytes read is returned
 */
int read_line(char *data, char *buf, int buf_len)
{
	int n;
	char *ptr = data;

	if (data == NULL || buf == NULL)
		return 0;

	for (n = 0; *ptr != '\0' && *ptr != '\n'; n++) {
		if (n == buf_len)
			break;

		buf[n] = *ptr++;
	}

	buf[n] = '\0';

	if (*ptr == '\n')
		n++;

	if (n != 0)
		ENG_LOG("wifi_eut: output: %s", buf);

	return n;
}

int check_npi_status(char *npi_output)
{
	int ret;
	int status;
	char buf[64];

	strncpy(buf, npi_output, sizeof(buf));
	strip_white_space(buf);
	ret = sscanf(buf, "ret:status%d:end", &status);
	if (ret != 1) {
		ENG_LOG("wifi_eut: %s ret = %d, status = %d", __func__, ret, status);
		return -1;
	}

	return status;
}

/* description: run iwnpi command
 * param:
 *    npi_cmd: npi command
 *    npi_buf: npi response
 *    npi_buf_len: npi buffer size

 * return value:
 *    On success return 0, else return -1;
 */
static int run_npi_command(char *npi_cmd, char *npi_buf, int npi_buf_len)
{
	int i;
	char *p;
	char *argv[16] = {0};
	char npi[128] = {0};
	int len, ret;
	int pfd[2];
	pid_t pid;

	if (npi_cmd == NULL && npi_buf == NULL) {
		ENG_LOG("wifi_eut: %s %d\n", __func__, __LINE__);
		return -1;
	}

	if (pipe(pfd) == -1) {
		ENG_LOG("wifi_eut: create pipe failed\n");
		return -1;
	}

	ENG_LOG("wifi_eut: npi command: %s\n", npi_cmd);
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

		strcpy(npi, npi_cmd);
		for (i = 0, p = npi; i < 16; i++, p = NULL) {
			argv[i] = strtok(p, " ");
			if (argv[i] == NULL)
				break;
		}

		execvp("/vendor/bin/iwnpi", argv);
		printf("child exec iwnpi failed\n");
		exit(0);
	} else {
		close(pfd[1]);

		p = npi_buf;
		len = npi_buf_len;

		memset(npi_buf, 0x0, npi_buf_len);

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
		wait(NULL);
	}

	return 0;
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
			&mac_addr[0], &mac_addr[1], &mac_addr[2],
			&mac_addr[3], &mac_addr[4], &mac_addr[5]);

	if (ret != 6)
		return -1;

	return 0;
}

int get_cp2_info(char *buf, int buf_len)
{
	int i;
	int max_try = 10;
	int fd;
	int ret;

	fd = open(AT_CMD_INTF, O_RDWR);
	if (fd < 0) {
		ENG_LOG("wifi_eut: open %s faild, ret = %d", AT_CMD_INTF, fd);
		return -1;
	}

	write(fd, AT_CMD_GET_CP2_INFO, sizeof(AT_CMD_GET_CP2_INFO));

	do {
		ret = read(fd, buf, buf_len);
		max_try--;
	} while(ret == 0 && max_try > 0);

	close(fd);

	while(*buf) {
		if (*buf == 0x7e) {
			if (buf[1] == '\0') {
				*buf = '\0';
				break;
			}
			else
				*buf = ',';
		}

		buf++;
	}

	return 0;
}

/* description: check if eut has permission to execute
 * param:
 *     eut: wiif_eut_t struct
 *
 * return value:
 *     0: allow to execute
 *    -1: not allowed to execute
 */
static int permission_check(struct wifi_eut_t *eut)
{
	int ret = -1;

	if (GET_EXT_DATA(WIFI_EUT_MODE) == WIFI_EUT_ENTER)
		return 0;

	switch (eut->eut_cmd_id) {
	case WIFI_EUT_CMD_SET_EUT_MODE:
	case WIFI_EUT_CMD_GET_EUT_MODE:
	case WIFI_EUT_CMD_SET_MAC:
	case WIFI_EUT_CMD_GET_MAC:
	case WIFI_EUT_CMD_GET_CP2INFO:
		ret = 0;
		break;

	default:
		break;
	}

	return ret;
}

static struct wifi_eut_t *at2eut(char *cmd)
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

	for (index = 0; wifi_eut[index].at_pattern != NULL; index++) {
		/* compare keywords: "EUT?"/"EUT," etc */
		if (!strncmp(wifi_eut[index].at_pattern, cmd, chr - cmd + 1)) {
			return &wifi_eut[index];
		}
	}

	return NULL;
}

static int set_tx_mode(struct wifi_eut_t *eut, char *at_cmd)
{
	int ret;
	int mode;
	char npi_cmd[64];
	char npi_output[256];
	char line[128];

	ret = sscanf(at_cmd, eut->at_pattern, &mode);
	if (ret != 1) {
		ENG_LOG("wifi_eut: %s %d\n", __func__, __LINE__);
		return -1;
	}

	/* CW mode */
	if (mode == 0) {
		ret = run_npi_command("iwnpi wlan0 tx_stop", npi_output, sizeof(npi_output));
		if (ret < 0)
			return -1;

		if (read_line(npi_output, line, sizeof(line)) == 0)
			return -1;

		if (check_npi_status(line) < 0)
			return -1;


		ret = run_npi_command("iwnpi wlan0 sin_wave", npi_output, sizeof(npi_output));
		if (ret < 0)
			return -1;

		if (read_line(npi_output, line, sizeof(line)) == 0)
			return -1;

		if (check_npi_status(line) < 0)
			return -1;
	} else {
		sprintf(npi_cmd, "iwnpi wlan0 set_tx_mode %d", mode);
		ret = run_npi_command(npi_cmd, npi_output, sizeof(npi_output));
		if (ret < 0)
			return -1;

		if (read_line(npi_output, line, sizeof(line)) == 0)
			return -1;

		if (check_npi_status(line) < 0)
			return -1;
	}

	return 0;
}

int set_tx(struct wifi_eut_t *eut, char *at_cmd)
{
	int ret;
	char npi_cmd[64] = {0};
	char npi_output[256];
	char line[128];
	int stat, mode, pkt_count;

	if (GET_EXT_DATA(WIFI_EUT_RX_STATUS) == WIFI_RX_START) {
		ENG_LOG("wifi_eut: ERR: RX STATUS is ON!");
		return -1;
	}

	ret = sscanf(at_cmd, eut->at_pattern, &stat, &mode, &pkt_count);
	if (ret < 0)
		return -1;

	/* Turn off tx*/
	if (stat == 0) {
		ret = run_npi_command("iwnpi wlan0 tx_stop", npi_output, sizeof(npi_output));
		if (ret < 0) {
			ENG_LOG("wifi_eut: %s %d", __func__, __LINE__);
			return -1;
		}

		if (read_line(npi_output, line, sizeof(line)) == 0) {
			ENG_LOG("wifi_eut: %s %d", __func__, __LINE__);
			return -1;
		}

		if (check_npi_status(line) < 0) {
			ENG_LOG("wifi_eut: %s %d", __func__, __LINE__);
			return -1;
		}

		SET_EXT_DATA(WIFI_EUT_TX_STATUS, WIFI_TX_STOP);
	/* Turn on tx */
	} else if (stat == 1) {
		switch (ret) {
		case 2:
		case 3:
			/*mode == 0; continue mode*/
			if (mode == 0)
				pkt_count = 0;
			else {
				if (ret != 3)
					return -1;
			}

			sprintf(npi_cmd, "iwnpi wlan0 set_tx_count %d", pkt_count);
			ret = run_npi_command(npi_cmd, npi_output, sizeof(npi_output));
			if (ret < 0) {
				ENG_LOG("wifi_eut: %s %d", __func__, __LINE__);
				return -1;
			}

			if (read_line(npi_output, line, sizeof(line)) == 0) {
				ENG_LOG("wifi_eut: %s %d", __func__, __LINE__);
				return -1;
			}

			if (check_npi_status(line) < 0) {
				ENG_LOG("wifi_eut: %s %d", __func__, __LINE__);
				return -1;
			}

			/* ignore break */
		case 1:
			ret = run_npi_command("iwnpi wlan0 tx_start", npi_output, sizeof(npi_output));
			if (ret < 0) {
				ENG_LOG("wifi_eut: %s %d", __func__, __LINE__);
				return -1;
			}

			if (read_line(npi_output, line, sizeof(line)) == 0) {
				ENG_LOG("wifi_eut: %s %d", __func__, __LINE__);
				return -1;
			}

			if (check_npi_status(line) < 0) {
				ENG_LOG("wifi_eut: %s %d", __func__, __LINE__);
				return -1;
			}

			SET_EXT_DATA(WIFI_EUT_TX_STATUS, WIFI_TX_START);
			break;

		default:
			return -1;
		}
	}

	return 0;
}

static int set_eut_mode(struct wifi_eut_t *eut, char *at_cmd)
{
	int ret;
	char *out;
	char *drv_path;
	char *module_name;
	char buf[64];
	char npi_cmd[64];
	char npi_output[128] = {0};

	ret = at2npi(eut, at_cmd, npi_cmd);
	if (ret < 0)
		return -1;

	if (eut->ext_data == WIFI_EUT_ENTER) {
		switch (eut->wcn_product_type) {
			case WCN_PRODUCT_MARLIN:
				drv_path = MARLIN_DRIVER_PATH;
				break;

			default:
				drv_path = MARLIN3_DRIVER_PATH;
		}

		if (load_driver(drv_path) < 0)
			return -1;

		ret = run_npi_command(npi_cmd, npi_output, sizeof(npi_output));
		if (ret < 0)
			return -1;

		if (read_line(npi_output, buf, sizeof(buf)) == 0)
			return -1;

		if (check_npi_status(buf) < 0)
			return -1;

		SET_EXT_DATA(WIFI_EUT_MODE, WIFI_EUT_ENTER);
	} else if (eut->ext_data == WIFI_EUT_EXIT) {
		ret = run_npi_command(npi_cmd, npi_output, sizeof(npi_output));
		if (ret < 0)
			return -1;

		if (read_line(npi_output, buf, sizeof(buf)) == 0)
			return -1;

		if (check_npi_status(buf) < 0)
			return -1;


		switch (eut->wcn_product_type) {
		case WCN_PRODUCT_MARLIN:
			module_name = "sprdwl";
			break;

		default:
			module_name = "sprdwl_ng";
			break;
		}

		if (is_module_exist(module_name) == 1) {
			sprintf(buf, "rmmod %s", module_name);
			if (system(buf) < 0) {
				ENG_LOG("wifi_eut: %s %d\n", __func__, __LINE__);
				return -1;
			}
		}

		/* exit eut mode successful */
		SET_EXT_DATA(WIFI_EUT_MODE, WIFI_EUT_EXIT);
	} else {
		return -1;
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
	char *at_cmd;
	int len;
	struct wifi_eut_t *eut;
	char npi_cmd[64];
	char npi_output[512];
	char mac[6];
	char buff[256];
	char *pout;
	int npi_flag = 0;

	at_cmd = diag_cmd + sizeof(MSG_HEAD_T) + 1;
	len = strlen(at_cmd) - 1;
	at_cmd[len] = 0;

	ENG_LOG("wifi_eut: AT command = %s", at_cmd);
	if (strncmp(WIFI_EUT_KEYWORD, at_cmd, WIFI_EUT_KEYWORD_LEN) != 0) {
		ENG_LOG("wifi_eut: keywords not match(%s)", at_cmd);
		goto err;
	}

	at_cmd += WIFI_EUT_KEYWORD_LEN;

	eut = at2eut(at_cmd);
	if (eut == NULL) {
		ENG_LOG("wifi_eut: at2eut failed!(line:%d)\n", __LINE__);
		goto err;
	}

	ENG_LOG("wifi_eut: eut_cmd_id = %d", eut->eut_cmd_id);

	ret = permission_check(eut);
	if (ret < 0) {
		ENG_LOG("wifi_eut: permission denied(line:%d)\n", __LINE__);
		ENG_LOG("wifi_eut: WIFI EUT mode = %d, eut cmd id = %d\n",
				GET_EXT_DATA(WIFI_EUT_MODE), eut->eut_cmd_id);

		goto err;
	}

	switch (eut->eut_cmd_id) {
	case WIFI_EUT_CMD_SET_NETMODE:
		strcpy(at_rsp, AT_CMD_RESP_OK);
		break;

	case WIFI_EUT_CMD_GET_EUT_MODE:
		sprintf(at_rsp, "+SPWIFITEST:EUT=%d", eut->ext_data);
		break;

	case WIFI_EUT_CMD_SET_TX_MODE:
		if(set_tx_mode(eut, at_cmd) < 0)
			goto err;

		strcpy(at_rsp, AT_CMD_RESP_OK);
		break;

	case WIFI_EUT_CMD_SET_MAC:
		ret = sscanf(at_cmd, eut->at_pattern,
				&mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
		if (ret != 6)
			goto err;

		if(set_mac_addr(mac) < 0)
			goto err;

		strcpy(at_rsp, AT_CMD_RESP_OK);
		break;

	case WIFI_EUT_CMD_GET_MAC:
		if(get_mac_addr(mac) < 0)
			goto err;

		sprintf(at_rsp, "+SPWIFITEST:MAC=%02x%02x%02x%02x%02x%02x",
				mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

		break;

	case WIFI_EUT_CMD_GET_CP2INFO:
		if (get_cp2_info(buff, sizeof(buff)) < 0)
			goto err;

		sprintf(at_rsp, "+SPWIFITEST:CP2INFO,%s", buff);
		break;

	case WIFI_EUT_CMD_SET_TX:
		if (set_tx(eut, at_cmd) < 0)
			goto err;

		strcpy(at_rsp, AT_CMD_RESP_OK);
		break;

	case WIFI_EUT_CMD_SET_EUT_MODE:
		if (set_eut_mode(eut, at_cmd) < 0)
			goto err;

		strcpy(at_rsp, AT_CMD_RESP_OK);
		break;

	case WIFI_EUT_CMD_GET_TX:
		sprintf(at_rsp, "+SPWIFITEST:TX=%d", eut->ext_data);
		break;

	case WIFI_EUT_CMD_GET_RX:
		sprintf(at_rsp, "+SPWIFITEST:RX=%d", eut->ext_data);
		break;

	default:
		ret = at2npi(eut, at_cmd, npi_cmd);
		if (ret < 0)
			goto err;

		ret = run_npi_command(npi_cmd, npi_output, sizeof(npi_output));
		if (ret < 0)
			goto err;

		pout = npi_output;
		ret = read_line(pout, buff, sizeof(buff));
		if (ret == 0) {
			ENG_LOG("wifi_eut: read_line failed ret = %d (line:%d)", ret, __LINE__);
			goto err;
		}

		pout += ret;

		ret = check_npi_status(buff);
		if(ret < 0) {
			ENG_LOG("wifi_eut: npi status = %d (line:%d)", ret, __LINE__);
			goto err;
		}

		ret = read_line(pout, buff, sizeof(buff));
		if (ret == 0) {
			strcpy(at_rsp, AT_CMD_RESP_OK);
		} else {
			npi_flag = 0;
			do {
				if (strncmp("ret", buff, 3) == 0) {
					ret = npi_result_to_at_resp(eut, buff, at_rsp);
					if (!ret)
						npi_flag = 1;

					break;
				}

				pout += ret;
				ret = read_line(pout, buff, sizeof(buff));

			} while (ret > 0);

			if (npi_flag == 0)
				goto err;
		}

		if (eut->eut_cmd_id == WIFI_EUT_CMD_SET_RX)
			SET_EXT_DATA(WIFI_EUT_RX_STATUS, eut->ext_data);

		break;
	}

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
	int wcn_product_type;

	property_get(WCN_HARDWARE_PRODUCT, name, "invalid");

	//fixme: just only support marlin/marlin2/marlin3
	if (strcmp("marlin", name) == 0)
		wcn_product_type = WCN_PRODUCT_MARLIN;
	else if (strcmp("marlin2", name) == 0)
		wcn_product_type = WCN_PRODUCT_MARLIN2;
	else
		wcn_product_type = WCN_PRODUCT_MARLIN3;

	/* depends on hw product type */
	SET_PRODUCT_TYPE(WIFI_EUT_CMD_SET_RATE, wcn_product_type);
	SET_PRODUCT_TYPE(WIFI_EUT_CMD_GET_RATE, wcn_product_type);
	SET_PRODUCT_TYPE(WIFI_EUT_CMD_SET_CHANNEL, wcn_product_type);
	SET_PRODUCT_TYPE(WIFI_EUT_CMD_GET_CHANNEL, wcn_product_type);

	/* set default eut mode: (0: EXIT EUT; 1: ENTER EUT) */
	SET_EXT_DATA(WIFI_EUT_MODE, WIFI_EUT_EXIT);

	/* set default rx status: (0: rx_stop, 1: rx_start)*/
	SET_EXT_DATA(WIFI_EUT_RX_STATUS, WIFI_RX_STOP);

	/* set default tx status: (0: tx_stop, 1: tx_start)*/
	SET_EXT_DATA(WIFI_EUT_TX_STATUS, WIFI_TX_STOP);
}

void register_this_module(struct eng_callback * reg)
{
	ALOGD("file:%s, func:%s\n", __FILE__, __func__);

	wifi_eut_init();
	sprintf(reg->at_cmd, "%s", "AT+SPWIFITEST");
	reg->eng_linuxcmd_func = wifi_eut_hdlr;
	ALOGD("module cmd:%s\n", reg->at_cmd);
}
