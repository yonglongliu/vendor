#ifndef __WLNPI_H__
#define __WLNPI_H__

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>

#include <linux/socket.h>
#include <linux/pkt_sched.h>
#include <netlink/object-api.h>

#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>
#include <netlink/netlink.h>
#include <endian.h>
#include <linux/types.h>

#define ETH_ALEN 					(6)
#define IWNPI_SSID_LEN              (32)
#define IWNPI_ASSOC_RESP_DATA_LEN   (280)

typedef unsigned char u8;
typedef unsigned short u16;

struct chan_t
{
	u16 index;
	u8 primary_chan;
	u8 center_chan;
};

enum wlan_nl_commands
{
	WLAN_NL_CMD_UNSPEC,
	WLAN_NL_CMD_NPI,
	WLAN_NL_CMD_GET_INFO,
	WLAN_NL_CMD_MAX,
};
enum wlan_nl_attrs
{
	WLAN_NL_ATTR_UNSPEC,
	WLAN_NL_ATTR_DEVINDEX,
	WLAN_NL_ATTR_COMMON_USR_TO_DRV,
	WLAN_NL_ATTR_COMMON_DRV_TO_USR,
	WLAN_NL_ATTR_MAX,
};

enum WLNPI_CMD_LIST
{
	WLNPI_CMD_START,
	WLNPI_CMD_STOP,
	WLNPI_CMD_SET_MAC,
	WLNPI_CMD_GET_MAC,
	WLNPI_CMD_SET_MAC_FILTER,
	WLNPI_CMD_GET_MAC_FILTER,  // 5
	WLNPI_CMD_SET_CHANNEL,
	WLNPI_CMD_GET_CHANNEL,
	WLNPI_CMD_GET_RSSI,
	WLNPI_CMD_SET_TX_MODE,
	WLNPI_CMD_GET_TX_MODE,  // 10
	WLNPI_CMD_SET_RATE,
	WLNPI_CMD_GET_RATE,
	WLNPI_CMD_SET_BAND,
	WLNPI_CMD_GET_BAND,
	WLNPI_CMD_SET_BW,  // 15
	WLNPI_CMD_GET_BW,
	WLNPI_CMD_SET_PKTLEN,
	WLNPI_CMD_GET_PKTLEN,
	WLNPI_CMD_SET_PREAMBLE,
	WLNPI_CMD_SET_GUARD_INTERVAL,  // 20
	WLNPI_CMD_GET_GUARD_INTERVAL,
	WLNPI_CMD_SET_BURST_INTERVAL,
	WLNPI_CMD_GET_BURST_INTERVAL,
	WLNPI_CMD_SET_PAYLOAD,
	WLNPI_CMD_GET_PAYLOAD,  // 25
	WLNPI_CMD_SET_TX_POWER,
	WLNPI_CMD_GET_TX_POWER,
	WLNPI_CMD_SET_TX_COUNT,
	WLNPI_CMD_GET_RX_OK_COUNT,
	WLNPI_CMD_TX_START,  // 30
	WLNPI_CMD_TX_STOP,
	WLNPI_CMD_RX_START,
	WLNPI_CMD_RX_STOP,
	WLNPI_CMD_GET_REG,
	WLNPI_CMD_SET_REG,  // 35
	WLNPI_CMD_SIN_WAVE,
	WLNPI_CMD_LNA_ON,
	WLNPI_CMD_LNA_OFF,
	WLNPI_CMD_GET_LNA_STATUS,
	WLNPI_CMD_SET_WLAN_CAP,  //40
	WLNPI_CMD_GET_WLAN_CAP,
	WLNPI_CMD_GET_CONN_AP_INFO,
	WLNPI_CMD_GET_MCS_INDEX,
	/* AutoRate */
	WLNPI_CMD_SET_AUTORATE_FLAG,
	WLNPI_CMD_GET_AUTORATE_FLAG, //45
	WLNPI_CMD_SET_AUTORATE_PKTCNT,
	WLNPI_CMD_GET_AUTORATE_PKTCNT,
	WLNPI_CMD_SET_AUTORATE_RETCNT,
	WLNPI_CMD_GET_AUTORATE_RETCNT,
	WLNPI_CMD_ROAM,				//50
	WLNPI_CMD_WMM_PARAM,
	WLNPI_CMD_ENG_MODE,//52
	WLNPI_CMD_RSV1_MARLIN_MARLIN2,//For Marlin/Marlin2 don't chang this item
	WLNPI_CMD_RSV2_MARLIN_MARLIN2,//For Marlin/Marlin2 don't chang this item
	WLNPI_CMD_RSV3_MARLIN_MARLIN2,//For Marlin/Marlin2 don't chang this item
	WLNPI_CMD_RSV4_MARLIN_MARLIN2,//For Marlin/Marlin2 don't chang this item
	WLNPI_CMD_RSV5_MARLIN_MARLIN2,//For Marlin/Marlin2 don't chang this item
	WLNPI_CMD_RSV6_MARLIN_MARLIN2,//For Marlin/Marlin2 don't chang this item
	WLNPI_CMD_RSV7_MARLIN_MARLIN2,//For Marlin/Marlin2 don't chang this item
	WLNPI_CMD_RSV8_MARLIN_MARLIN2,//For Marlin/Marlin2 don't chang this item
	WLNPI_CMD_RSV9_MARLIN_MARLIN2,//For Marlin/Marlin2 don't chang this item
	WLNPI_CMD_RSV10_MARLIN_MARLIN2,//For Marlin/Marlin2 don't chang this item
	WLNPI_CMD_RSV11_MARLIN_MARLIN2,//For Marlin/Marlin2 don't chang this item
	WLNPI_CMD_RSV12_MARLIN_MARLIN2,//For Marlin/Marlin2 don't chang this item
	WLNPI_CMD_GET_DEV_SUPPORT_CHANNEL = 65,

	WLNPI_CMD_GET_PA_INFO = 66,
	WLNPI_CMD_GET_TX_STATUS,
	WLNPI_CMD_GET_RX_STATUS,
	WLNPI_CMD_GET_STA_LUT_STATUS,
	WLNPI_CMD_START_PKT_LOG,
	WLNPI_CMD_STOP_PKT_LOG,
	WLNPI_CMD_RSV1,
	WLNPI_CMD_RSV2,
	WLNPI_CMD_RSV3,
	WLNPI_CMD_RSV4,
	WLNPI_CMD_RSV5,
	WLNPI_CMD_RSV6 = 77,
	WLNPI_CMD_SET_CHAIN,//78
	WLNPI_CMD_GET_CHAIN,
	WLNPI_CMD_SET_SBW,//80
	WLNPI_CMD_GET_SBW,
	WLNPI_CMD_SET_AMPDU_ENABLE,
	WLNPI_CMD_GET_AMPDU_ENABLE,
	WLNPI_CMD_SET_AMPDU_COUNT,
	WLNPI_CMD_GET_AMPDU_COUNT, //85
	WLNPI_CMD_SET_STBC,
	WLNPI_CMD_GET_STBC,
	WLNPI_CMD_SET_FEC,
	WLNPI_CMD_GET_FEC,
	WLNPI_CMD_GET_RX_SNR, //90
	WLNPI_CMD_SET_FORCE_TXGAIN,
	WLNPI_CMD_SET_FORCE_RXGAIN,
	WLNPI_CMD_SET_BFEE_ENABLE,
	WLNPI_CMD_SET_DPD_ENABLE,
	WLNPI_CMD_SET_AGC_LOG_ENABLE, //95
	WLNPI_CMD_SET_EFUSE,
	WLNPI_CMD_GET_EFUSE,
	WLNPI_CMD_SET_TXS_CALIB_RESULT,
	WLNPI_CMD_GET_TXS_TEMPERATURE,
	WLNPI_CMD_SET_CBANK_REG, //100
	WLNPI_CMD_SET_11N_GREEN_FIELD,
	WLNPI_CMD_GET_11N_GREEN_FIELD,
	WLNPI_CMD_ATENNA_COUPLING,
	WLNPI_CMD_SET_FIX_TX_RF_GAIN,
	WLNPI_CMD_SET_FIX_TX_PA_BIAS, //105
	WLNPI_CMD_SET_FIX_TX_DVGA_GAIN,
	WLNPI_CMD_SET_FIX_RX_LNA_GAIN,
	WLNPI_CMD_SET_FIX_RX_VGA_GAIN,
	WLNPI_CMD_SET_MAC_BSSID,
	WLNPI_CMD_GET_MAC_BSSID, //110
	WLNPI_CMD_FORCE_TX_RATE,
	WLNPI_CMD_FORCE_TX_POWER = 112,
	WLNPI_CMD_ENABLE_FW_LOOP_BACK,
	WLNPI_CMD_RSV114 = 114,
	WLNPI_CMD_SET_PROTECTION_MODE = 115,
	WLNPI_CMD_GET_PROTECTION_MODE,
	WLNPI_CMD_SET_RTS_THRESHOLD,
	WLNPI_CMD_SET_AUTORATE_DEBUG,
	WLNPI_CMD_SET_PM_CTL = 119,
	WLNPI_CMD_GET_PM_CTL = 120,
    /* Max */
    WLNPI_CMD_MAX,
};

enum GET_STATUS_SUBTYPE {
	GET_RECONNECT = 1,
	GET_STATUS_MAX,
};

struct wlan_nl_sock_state
{
	struct nl_sock *sock;
	int nl_id;
	int nl_cmd_id;
};

typedef struct
{
	struct nl_sock *sock;
	unsigned int devindex;
	int nl_id;
	int nl_cmd_id;
	unsigned char mac[6];
}wlnpi_t;

typedef struct wlnpi_cmd_t wlnpi_cmd_t___;
typedef  int (*P_FUNC_1)(int, char **,  unsigned char *, int * );
typedef  int (*P_FUNC_2)(struct wlnpi_cmd_t *, unsigned char *, int);
struct wlnpi_cmd_t
{
	char    *name;
	char    *help;
	P_FUNC_1 parse;
	P_FUNC_2 show;
	char     id;
};

typedef struct
{
	unsigned char  type;
	unsigned char  subtype;
	unsigned short len;
}WLNPI_CMD_HDR_T;

enum WLNPI_CMD_TYPE
{
	HOST_TO_MARLIN_CMD = 1,
	MARLIN_TO_HOST_REPLY  ,
};

typedef struct iwnpi_rate_table_t
{
    int  phy_rate;
    char *str_rate;
}iwnpi_rate_table;

typedef struct assoc_resp_t
{
    char connect_status;
    char ssid[IWNPI_SSID_LEN+1];
    char conn_mode;
    int  rssi;
    int  snr;
    int  noise;
    char channel;
    char bssid[ETH_ALEN];
    char assoc_resp_info[IWNPI_ASSOC_RESP_DATA_LEN];
} assoc_resp;

struct pa_info_struct
{
	unsigned int rx_giveup_cnt;
	unsigned int rx_drop_cnt;
	unsigned int rx_filter_cnt_frmtype;
	unsigned int rx_ce_err_info;
	unsigned int rx_dup_cnt;
	unsigned int rx_stage23_mpdu_cnt;
	unsigned int rx_bitmap_chk_cnt;
	unsigned int rx_int_cnt;
	unsigned int rx_mpdu_debug_cnt;
	unsigned int rx_ce_debug;
	unsigned int rx_psdu_cnt;
	unsigned int rx_mpdu_cnt1;
	unsigned int rx_mpdu_cnt2;
	unsigned int rx_mac1_mpdu_bytecnt;
	unsigned int rx_mac2_mpdu_bytecnt;
	unsigned int rx_mac3_mpdu_bytecnt;
	unsigned int rx_mac4_mpdu_bytecnt;
	unsigned int rx_mac1_lastsuc_datafrm_tsf_l;
	unsigned int rx_mac2_lastsuc_datafrm_tsf_l;
	unsigned int rx_mac3_lastsuc_datafrm_tsf_l;
	unsigned int rx_mac4_lastsuc_datafrm_tsf_l;
	unsigned int mib_cycle_cnt;
	unsigned int mib_rx_clear_cnt_20m;
	unsigned int mib_rx_clear_cnt_40m;
	unsigned int mib_rx_clear_cnt_80m;
	unsigned int mib_rx_cycle_cnt;
	unsigned int mib_myrx_uc_cycle_cnt;
	unsigned int mib_myrx_bcmc_cycle_cnt;
	unsigned int mib_rx_idle_cnt;
	unsigned int mib_tx_cycle_cnt;
	unsigned int mib_tx_idle_cnt;
};

struct tx_status_struct
{
	unsigned int tx_pkt_cnt[4];
	unsigned int tx_suc_cnt[4];
	unsigned int tx_fail_cnt[4];
	unsigned int tx_fail_reason_cnt[2];
	unsigned int tx_err_cnt;
	unsigned int rts_success_cnt;
	unsigned int rts_fail_cnt;
	unsigned int retry_cnt;
	unsigned int ampdu_retry_cnt;
	unsigned int ba_rx_fail_cnt;
	unsigned char color_num_sdio[4];
	unsigned char color_num_mac[4];
	unsigned char color_num_txc[4];
};

struct rx_rssi_struct {
	unsigned char last_rxdata_rssi1;
	unsigned char last_rxdata_rssi2;
	unsigned char last_rxdata_snr1;
	unsigned char last_rxdata_snr2;
	unsigned char last_rxdata_snr_combo;
	unsigned char last_rxdata_snr_l;
} __packed;

struct rx_status_struct
{
	unsigned int rx_pkt_cnt[4];
	unsigned int rx_retry_pkt_cnt[4];
	unsigned int rx_su_beamformed_pkt_cnt;
	unsigned int rx_mu_pkt_cnt;
	unsigned int rx_11n_mcs_cnt[16];
	unsigned int rx_11n_sgi_cnt[16];
	unsigned int rx_11ac_mcs_cnt[10];
	unsigned int rx_11ac_sgi_cnt[10];
	unsigned int rx_11ac_stream_cnt[2];
	unsigned int rx_bandwidth_cnt[3];
	struct rx_rssi_struct rx_rssi[3];
	unsigned int rxc_isr_cnt[5];
	unsigned int rxq_buffer_rqst_isr_cnt[5];
	unsigned short req_tgrt_bu_num[5];
	unsigned short rx_alloc_pkt_num[3];
} __packed;

extern wlnpi_t g_wlnpi;
extern struct wlnpi_cmd_t *match_cmd_table(char *name);

#endif
