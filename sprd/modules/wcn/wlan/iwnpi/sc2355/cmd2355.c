/*
 * Authors:<jay.yang@spreadtrum.com>
 * Owner:
 */
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>

#include "wlnpi.h"

#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>
#include <netlink/netlink.h>
#include <endian.h>
#include <linux/types.h>

#include <android/log.h>
#include <utils/Log.h>


#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG     "WLNPI"

#define ENG_LOG  ALOGD

#define WLNPI_RESULT_BUF_LEN        (256)

#define IWNPI_EXEC_TMP_FILE         ("/productinfo/iwnpi_exec_data.log")

#define WLAN_EID_SUPP_RATES         (1)
#define WLAN_EID_HT_CAP             (45)
#define WLAN_EID_VENDOR_SPECIFIC    (221)
#define WPS_IE_VENDOR_TYPE          (0x0050f204)

#define IWNPI_CONN_MODE_MANAGED     ("Managed")
#define IWNPI_CONN_MODE_OTHER       ("Other Mode")
#define IWNPI_DEFAULT_CHANSPEC      ("2.4G")
#define IWNPI_DEFAULT_BANDWIDTH     ("20MHz")

static int wlnpi_ap_info_print_capa(const char *data);
static int wlnpi_ap_info_print_support_rate(const char *data);
static char *wlnpi_get_rate_by_phy(int phy);
static char *wlnpi_bss_get_ie(const char *bss, char ieid);
static char *iwnpi_bss_get_vendor_ie(const char *bss, int vendor_type);
static int wlnpi_ap_info_print_ht_capa(const char *data);
static int wlnpi_ap_info_print_ht_mcs(const char *data);
static int wlnpi_ap_info_print_wps(const char *data);
//static int iwnpi_hex_dump(unsigned char *name, unsigned short nLen, unsigned char *pData, unsigned short len);

struct chan_t g_chan_list[] ={
	/*2.4G--20M*/
	{0,1,1},{1,2,2},{2,3,3},{3,4,4},{4,5,5},
	{5,6,6},{6,7,7},{7,8,8},{8,9,9},{9,10,10},
	{10,11,11},{11,12,12},{12,13,13},{13,14,14},
	/*2.4G---40M*/
	{14,1,3},{15,2,4},{16,3,5},{17,4,6},{18,5,7},
	{19,6,8},{20,7,9},{21,8,10},{22,9,11},{23,5,3},
	{24,6,4},{25,7,5},{26,8,6},{27,9,7},{28,10,8},
	{29,11,9},{30,12,10},{31,13,11},
	/*5G---20M*/
	{32,36,36},{33,40,40},{34,44,44},{35,48,48},{36,52,52},
	{37,56,56},{38,60,60},{39,64,64},{40,100,100},{41,104,104},
	{42,108,108},{43,112,112},{44,116,116},{45,120,120},{46,124,124},
	{47,128,128},{48,132,132},{49,136,136},{50,140,140},{51,144,144},
	{52,149,149},{53,153,153},{54,157,157},{55,161,161},{56,165,165},
	/*5G---40M*/
	{57,36,38},{58,40,38},{59,44,46},{60,48,46},{61,52,54},
	{62,56,54},{63,60,62},{64,64,62},{65,100,102},{66,104,102},
	{67,108,110},{68,112,110},{69,116,118},{70,120,118},{71,124,126},
	{72,128,126},{73,132,134},{74,136,134},{75,140,142},{76,144,142},
	{77,149,151},{78,153,151},{79,157,159},{80,161,159},
	/*5G---80M*/
	{81,36,42},{82,40,42},{83,44,42},{84,48,42},{85,52,58},
	{86,56,58},{87,60,58},{88,64,58},{89,100,106},{90,104,106},
	{91,108,106},{92,112,106},{93,116,122},{94,120,122},{95,124,122},
	{96,128,122},{97,132,138},{98,136,138},{99,140,138},{100,144,138},
	{101,149,155},{102,153,155},{103,157,155},{104,161,155},
};


/********************************************************************
*   name   iwnpi_get_be16
*   ---------------------------
*   description:
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
static short iwnpi_get_be16(const char *a)
{
    return (a[0] << 8) | a[1];
}

/********************************************************************
*   name   iwnpi_get_le16
*   ---------------------------
*   description:
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
static short iwnpi_get_le16(const char *a)
{
    return (a[1] << 8) | a[0];
}

/********************************************************************
*   name   iwnpi_get_be32
*   ---------------------------
*   description:
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
static int iwnpi_get_be32(const char *a)
{
    return (a[0] << 24) | (a[1] << 16) | (a[2] << 8) | a[3];
}

bool is_digital(const char *str)
{
    char first = *str;
    const char *tmp;

    if (first != '-' && first != '+' && (first < '0' || first > '9')) {
	return false;
    }

    tmp = str + 1;
    while(*tmp) {
	if (*tmp < '0' || *tmp > '9')
	    return false;
	tmp++;
    }
    return true;
}

static iwnpi_rate_table g_rate_table[] =
{
    {0x82, "1[b]"},
    {0x84, "2[b]"},
    {0x8B, "5.5[b]"},
    {0x0C, "6"},
    {0x12, "9"},
    {0x96, "11[b]"},
    {0x18, "12"},
    {0x24, "18"},
    {0x30, "24"},
    {0x48, "36"},
    {0x60, "48"},
    {0x6C, "54"},

    /* 11g */
    {0x02, "1[b]"},
    {0x04, "2[b]"},
    {0x0B, "5.5[b]"},
    {0x16, "11[b]"},
};

int mac_addr_a2n(unsigned char *mac_addr, char *arg)
{
    int i;

    for (i = 0; i < ETH_ALEN; i++)
    {
        int temp;
        char *cp = strchr(arg, ':');
        if (cp) {
            *cp = 0;
            cp++;
        }
        if (sscanf(arg, "%x", &temp) != 1)
            return -1;
        if (temp < 0 || temp > 255)
            return -1;

        mac_addr[i] = temp;
        if (!cp)
            break;
        arg = cp;
    }
    if (i < ETH_ALEN - 1)
        return -1;

    return 0;
}

int wlnpi_cmd_no_argv(int argc, char **argv,  unsigned char *s_buf, int *s_len )
{
    if(argc > 0)
        return -1;
    *s_len = 0;
    return 0;
}

int wlnpi_show_only_status(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
    return 0;
}

int wlnpi_show_only_int_ret(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
    ENG_LOG("ADL entry %s(), r_len = %d", __func__, r_len);

    if(4 != r_len)
    {
        printf("%s() err\n", __func__);
        return -1;
    }
    printf("ret: %d :end\n",  *( (unsigned int *)(r_buf+0) )  );

    ENG_LOG("ADL leaving %s(), return 0", __func__);

    return 0;
}

/*-----CMD ID:0-----------*/
int wlnpi_cmd_start(int argc, char **argv,  unsigned char *s_buf, int *s_len )
{
	if(1== argc)
		mac_addr_a2n(s_buf, argv[0]);
	else
		memcpy(s_buf,  &(g_wlnpi.mac[0]),  6 );

	*s_len = 6;
    return 0;
}

/*-----CMD ID:2-----------*/
int wlnpi_cmd_set_mac(int argc, char **argv,  unsigned char *s_buf, int *s_len )
{
    if(1 != argc)
        return -1;
    mac_addr_a2n(s_buf, argv[0]);
    *s_len = 6;
    return 0;
}

/*-----CMD ID:3-----------*/
int wlnpi_show_get_mac(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
    int i, ret, p;

    ENG_LOG("ADL entry %s(), r_len = %d", __func__,r_len);

    printf("ret: mac: %02x:%02x:%02x:%02x:%02x:%02x :end\n", r_buf[0], r_buf[1], r_buf[2],r_buf[3],r_buf[4], r_buf[5]);

    ENG_LOG("ADL leaving %s(), mac = %02x:%02x:%02x:%02x:%02x:%02x, return 0", __func__, r_buf[0], r_buf[1], r_buf[2],r_buf[3],r_buf[4], r_buf[5]);

    return 0;
}

/*-----CMD ID:4-----------*/
/********************************************************************
*   name   wlnpi_cmd_set_macfilter
*   ---------------------------
*   description:
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_cmd_set_macfilter(int argc, char **argv,  unsigned char *s_buf, int *s_len )
{
    char *err;

    ENG_LOG("ADL entry %s(), argc = %d, argv[0] = %s", __func__, argc, argv[0]);
    if(1 != argc)
    {
        ENG_LOG("ADL %s(), argc is ERROR, return -1", __func__);
        return -1;
    }

    *((unsigned int *)s_buf) = strtol(argv[0], &err, 10);
    if(err == argv[0])
    {
        ENG_LOG("ADL %s(), strtol is ERROR, return -1", __func__);
        return -1;
    }

    *s_len = 1;

    ENG_LOG("ADL leaving %s(), s_buf[0] = %d, s_len = %d", __func__, s_buf[0], *s_len);
    return 0;
}

/*-----CMD ID:5-----------*/
/********************************************************************
*   name   wlnpi_show_get_macfilter
*   ---------------------------
*   description:
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_show_get_macfilter(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
    unsigned char macfilter = 0;
    ENG_LOG("ADL entry %s(), r_len = %d", __func__, r_len);

    if(1 != r_len)
    {
        printf("get_macfilter err\n");
        ENG_LOG("ADL leaving %s(), r_len is ERROR, return -1", __func__);

        return -1;
    }

    macfilter = *( (unsigned char *)(r_buf+0) ) ;
    printf("ret: %d :end\n", macfilter);

    ENG_LOG("ADL leaving %s(), macfilter = %d, return 0", __func__, macfilter);

    return 0;
}

/*-----CMD ID:6-----------*/
int wlnpi_cmd_set_channel(int argc, char **argv,  unsigned char *s_buf, int *s_len )
{
	u8 count;
    char *err;
    if(2 != argc)
	{
		ENG_LOG("both primary & center channel should be set\n");
        return -1;
	}
    *((unsigned int *)s_buf) =   strtol(argv[0], &err, 10);
	*((unsigned int *)(s_buf+1)) =   strtol(argv[1], &err, 10);
	for(count = 0; count < (sizeof(g_chan_list)/sizeof(struct chan_t)); count++)
	{
		if ((*s_buf == g_chan_list[count].primary_chan) &&
			*(s_buf + 1) == g_chan_list[count].center_chan)
			break;
	}
	if (count == sizeof(g_chan_list)/sizeof(struct chan_t))
	{
		 ENG_LOG(" primary or center channel is invalid\n");
		 return -1;
	}

    if(err == argv[0])
        return -1;
    *s_len = 2;
    return 0;
}

/*-----CMD ID:7-----------*/
int wlnpi_show_get_channel(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
    unsigned char primary20 = 0;
    unsigned char center = 0;

    ENG_LOG("ADL entry %s(), r_len = %d", __func__, r_len);

    if(2 != r_len)
    {
        printf("get_channel err\n");
        ENG_LOG("ADL leaving %s(), r_len is ERROR, return -1", __func__);
        return -1;
    }

    primary20 =  *( (unsigned char *)(r_buf+0) );
    center =  *((unsigned char *)(r_buf+1));
    printf("ret: primary_channel:%d,center_channel:%d :end\n", primary20, center);

    ENG_LOG("ADL leaving %s(), primary20 = %d, center=%d  return 0", __func__, primary20, center);

    return 0;
}

/*-----CMD ID:8-----------*/
int wlnpi_show_get_rssi(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
    char rssi = 0;
    FILE *fp = NULL;
    char tmp[64] = {0x00};

    ENG_LOG("ADL entry %s(), r_len = %d", __func__, r_len);

    if(1 != r_len)
    {
        printf("get_rssi err\n");
        ENG_LOG("ADL leaving %s(), r_len is ERROR, return -1", __func__);

        return -1;
    }

    rssi = *((char *)r_buf);
    printf("ret: %d:end\n", rssi );
    ENG_LOG("%s rssi is : %d \n",__func__, rssi);
    snprintf(tmp, 64, "rssi is : %d", rssi);

    if(NULL != (fp = fopen(IWNPI_EXEC_TMP_FILE, "w+"))) {
        int write_cnt = 0;

        write_cnt = fputs(tmp, fp);
        ENG_LOG("ADL %s(), callED fputs(%s), write_cnt = %d", __func__, tmp, write_cnt);
        fclose(fp);
    }
    else {
	ENG_LOG("%s open file error\n", __func__);
    }
    return 0;
}

/*-----CMD ID:9-----------*/
/********************************************************************
*   name   wlnpi_cmd_set_tx_mode
*   ---------------------------
*   description:
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   para:0：duty cycle；1：carrier suppressioon；2：local leakage
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_cmd_set_tx_mode(int argc, char **argv,  unsigned char *s_buf, int *s_len )
{
    char *err;

    ENG_LOG("ADL entry %s(), argc = %d, argv[0] = %s", __func__, argc, argv[0]);
    if(1 != argc)
    {
        ENG_LOG("ADL %s(), argc is ERROR, return -1", __func__);
        return -1;
    }

    *((unsigned int *)s_buf) = strtol(argv[0], &err, 10);
    if(err == argv[0])
    {
        ENG_LOG("ADL %s(), strtol is ERROR, return -1", __func__);
        return -1;
    }

	if(*s_buf > 2)
	{
		ENG_LOG("index value:%d exceed define value", *s_buf);
		return -1;
	}

    *s_len = 1;

    ENG_LOG("ADL leaving %s(), s_buf[0] = %d, s_len = %d", __func__, s_buf[0], *s_len);
    return 0;
}

/*-----CMD ID:10-----------*/
/********************************************************************
*   name   wlnpi_show_get_tx_mode
*   ---------------------------
*   description:
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_show_get_tx_mode(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
    unsigned char tx_mode = 0;
    ENG_LOG("ADL entry %s(), r_len = %d", __func__, r_len);

    if(1 != r_len)
    {
        printf("get_tx_mode err\n");
        ENG_LOG("ADL leaving %s(), r_len is ERROR, return -1", __func__);

		return -1;
    }

    tx_mode = *( (unsigned char *)(r_buf+0) ) ;
	if(tx_mode > 2)
	{
		ENG_LOG("index value:%d exceed define value", tx_mode);
		return -1;
	}
    printf("ret: %d :end\n", tx_mode);

    ENG_LOG("ADL leaving %s(), tx_mode = %d, return 0", __func__, tx_mode);

	return 0;
}

/*-----CMD ID:11-----------*/
int wlnpi_cmd_set_rate(int argc, char **argv,  unsigned char *s_buf, int *s_len )
{
    char *err;

    ENG_LOG("ADL entry %s(), argc = %d, argv[0] = %s", __func__, argc, argv[0]);

	if(1 != argc)
        return -1;

    *((unsigned int *)s_buf) =   strtol(argv[0], &err, 10);
    if(err == argv[0])
        return -1;

	/*rate index should between 0 and 50*/
	if(*s_buf > 50)
	{
		ENG_LOG("rate index value:%d exceed design value\n", *s_buf);
		return -1;
	}

    *s_len = 1;

    ENG_LOG("ADL leaving %s(), s_buf[0] = %d, s_len = %d", __func__, s_buf[0], *s_len);
    return 0;
}

/*-----CMD ID:12-----------*/
int wlnpi_show_get_rate(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
    unsigned char rate = 0;

    ENG_LOG("ADL entry %s(), r_len = %d", __func__, r_len);

    if(1 != r_len)
    {
        printf("get_rate err\n");
        ENG_LOG("ADL leaving %s(), r_len is ERROR, return -1", __func__);

        return -1;
    }

    rate = *( (unsigned char *)r_buf );
    printf("ret: %d :end\n", rate);

    ENG_LOG("ADL leaving %s(), rate = %d, return 0", __func__, rate);

    return 0;
}

/*-----CMD ID:13-----------*/
/******************************************************************
*   name   wlnpi_cmd_set_band
*   ---------------------------
*   description:
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_cmd_set_band(int argc, char **argv,  unsigned char *s_buf, int *s_len )
{
    char *err;

    ENG_LOG("ADL entry %s(), argc = %d, argv[0] = %s", __func__, argc, argv[0]);
    if(1 != argc)
    {
        return -1;
    }

    *((unsigned int *)s_buf) = strtol(argv[0], &err, 10);
    if(err == argv[0])
    {
        return -1;
    }

    *s_len = 1;

    ENG_LOG("ADL leaving %s()", __func__);
    return 0;
}

/*-----CMD ID:14-----------*/
/********************************************************************
*   name   wlnpi_show_get_band
*   ---------------------------
*   description:
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_show_get_band(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
    unsigned int band = 0;

    ENG_LOG("ADL entry %s(), r_len = %d", __func__, r_len);

    if(1 != r_len)
    {
        printf("get_band err \n");
        ENG_LOG("ADL leaving %s(), r_len is ERROR, return -1", __func__);

        return -1;
    }

    band = *( (unsigned char *)(r_buf+0) ) ;
    printf("ret: %d :end\n", band);

    ENG_LOG("ADL leaving %s(), band = %d, return 0", __func__, band);

    return 0;
}

/*-----CMD ID:15-----------*/
/******************************************************************
*   name   wlnpi_cmd_set_cbw
*   ---------------------------
*   description: set channel bandwidth
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_cmd_set_cbw(int argc, char **argv,  unsigned char *s_buf, int *s_len )
{
    char *err;

    ENG_LOG("ADL entry %s(), argc = %d, argv[0] = %s", __func__, argc, argv[0]);
    if(1 != argc)
    {
        return -1;
    }

    *((unsigned int *)s_buf) = strtol(argv[0], &err, 10);
    if(err == argv[0])
    {
        return -1;
    }

	if(*s_buf > 4)
	{
		ENG_LOG("index value:%d exceed define", *s_buf);
	}

    *s_len = 1;

    ENG_LOG("ADL leaving %s()", __func__);
    return 0;
}

/*-----CMD ID:16-----------*/
/********************************************************************
*   name   wlnpi_show_get_bandwidth
*   ---------------------------
*   description: get channel bandwidth
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_show_get_cbw(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
    unsigned int bandwidth = 0;

    ENG_LOG("ADL entry %s(), r_len = %d", __func__, r_len);

    if(1 != r_len)
    {
        printf("get_bandwidth err\n");
        ENG_LOG("ADL leaving %s(), r_len is ERROR, return -1", __func__);

        return -1;
    }

    bandwidth = *( (unsigned char *)(r_buf+0) );
    printf("ret: %d :end\n", bandwidth);

    ENG_LOG("ADL leaving %s(), bandwidth = %d, return 0", __func__, bandwidth);

    return 0;
}

/*-----CMD ID:17-----------*/
/******************************************************************
*   name   wlnpi_cmd_set_pkt_length
*   ---------------------------
*   description:
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_cmd_set_pkt_length(int argc, char **argv,  unsigned char *s_buf, int *s_len )
{
    char *err;

    ENG_LOG("ADL entry %s(), argc = %d, argv[0] = %s", __func__, argc, argv[0]);

    if(1 != argc)
    {
        return -1;
    }

    *((unsigned int *)s_buf) = strtol(argv[0], &err, 10);
    *s_len = 2;
    if(err == argv[0])
    {
        return -1;
    }

    ENG_LOG("ADL leaving %s()", __func__);

    return 0;
}

/*-----CMD ID:18-----------*/
/********************************************************************
*   name   wlnpi_show_get_pkt_length
*   ---------------------------
*   description:
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_show_get_pkt_length(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
    unsigned short pktlen = 0;

    ENG_LOG("ADL entry %s(), r_len = %d", __func__, r_len);

    if(2 != r_len)
    {
        printf("get_pkt_length err\n");
        ENG_LOG("ADL leaving %s(), r_len is ERROR, return -1", __func__);

        return -1;
    }

    pktlen = *( (unsigned short *)(r_buf+0) ) ;
    printf("ret: %d :end\n", pktlen);

    ENG_LOG("ADL leaving %s(), pktlen = %d, return 0", __func__, pktlen);

    return 0;
}

/*-----CMD ID:19-----------*/
/*******************************************************************
*   name   wlnpi_cmd_set_preamble
*   ---------------------------
*   description:
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_cmd_set_preamble(int argc, char **argv,  unsigned char *s_buf, int *s_len )
{
    char *err;

    ENG_LOG("ADL entry %s(), argc = %d, argv[0] = %s", __func__, argc, argv[0]);
    if(1 != argc)
    {
        return -1;
    }

    *((unsigned int *)s_buf) = strtol(argv[0], &err, 10);
    if(err == argv[0])
    {
        return -1;
    }
	if(*s_buf > 4)
	{
		 ENG_LOG("index value:%d exceed define!", *s_buf);
		 return -1;
	}

    *s_len = 1;

    ENG_LOG("ADL leaving %s()", __func__);
    return 0;
}

/*-----CMD ID:21-----------*/
/******************************************************************
*   name   wlnpi_cmd_set_guard_interval
*   ---------------------------
*   description:
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_cmd_set_guard_interval(int argc, char **argv,  unsigned char *s_buf, int *s_len )
{
    char *err;

    ENG_LOG("ADL entry %s(), argc = %d, argv[0] = %s", __func__, argc, argv[0]);
    if(1 != argc)
    {
        ENG_LOG("ADL %s(), argc is ERROR, return -1", __func__);
        return -1;
    }

    *((unsigned int *)s_buf) = strtol(argv[0], &err, 10);
    if(err == argv[0])
    {
        ENG_LOG("ADL %s(), strtol is ERROR, return -1", __func__);
        return -1;
    }
	if(*s_buf >1)
	{
		ENG_LOG("gi index:%d not equal 0 or 1", *s_buf);
		return -1;
	}

    *s_len = 1;

    ENG_LOG("ADL leaving %s()", __func__);
    return 0;
}

/*-----CMD ID:21-----------*/
/********************************************************************
*   name   wlnpi_show_get_guard_interval
*   ---------------------------
*   description:
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_show_get_guard_interval(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
    unsigned char gi = 0;
    ENG_LOG("ADL entry %s(), r_len = %d", __func__, r_len);

    if(1 != r_len)
    {
        printf("get_payload err\n");
        ENG_LOG("ADL leaving %s(), r_len is ERROR, return -1", __func__);

        return -1;
    }

    gi = *( (unsigned char *)(r_buf+0) ) ;
    printf("ret: %d :end\n", gi);

    ENG_LOG("ADL leaving %s(), gi = %d, return 0", __func__, gi);

    return 0;
}

/*-----CMD ID:24-----------*/
/*******************************************************************
*   name   wlnpi_cmd_set_payload
*   ---------------------------
*   description:
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_cmd_set_payload(int argc, char **argv,  unsigned char *s_buf, int *s_len )
{
    char *err;

    ENG_LOG("ADL entry %s(), argc = %d, argv[0] = %s", __func__, argc, argv[0]);
    if(1 != argc)
    {
        return -1;
    }

    *((unsigned int *)s_buf) = strtol(argv[0], &err, 10);
    if(err == argv[0])
    {
        return -1;
    }

    *s_len = 1;

    ENG_LOG("ADL leaving %s()", __func__);
    return 0;
}

/*-----CMD ID:25-----------*/
/********************************************************************
*   name   wlnpi_show_get_payload
*   ---------------------------
*   description:
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_show_get_payload(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
    unsigned int result = 0;

    ENG_LOG("ADL entry %s(), r_len = %d", __func__, r_len);

    if(1 != r_len)
    {
        printf("get_payload err\n");
        ENG_LOG("ADL leaving %s(), r_len is ERROR, return -1", __func__);

        return -1;
    }

    result = *( (unsigned char *)(r_buf+0) ) ;
    printf("ret: %d :end\n", result);

    ENG_LOG("ADL leaving %s(), result = %d, return 0", __func__, result);

    return 0;
}

/*-----CMD ID:26-----------*/
int wlnpi_cmd_set_tx_power(int argc, char **argv,  unsigned char *s_buf, int *s_len )
{
    char *err;
    if(1 != argc)
        return -1;
    *((unsigned int *)s_buf) =   strtol(argv[0], &err, 10);
    if(err == argv[0])
        return -1;
    *s_len = 1;
    return 0;
}

/*-----CMD ID:27-----------*/
int wlnpi_show_get_tx_power(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
    unsigned char TSSI;

    ENG_LOG("ADL entry %s(), r_len = %d", __func__, r_len);

    if(1 != r_len)
    {
        printf("get_tx_power err\n");
        ENG_LOG("ADL leaving %s(), r_len is ERROR, return -1", __func__);

        return -1;
    }

    TSSI = r_buf[0];
    printf("ret: %d :end\n", TSSI);

    ENG_LOG("ADL leaving %s(), TSSI:%d return 0", __func__, TSSI);

    return 0;
}

/*-----CMD ID:28-----------*/
int wlnpi_cmd_set_tx_count(int argc, char **argv,  unsigned char *s_buf, int *s_len )
{
    char *err;
    if(1 != argc)
        return -1;
    *s_buf =   strtol(argv[0], &err, 10);
    *s_len = 4;
    if(err == argv[0])
        return -1;
    return 0;
}

/*-----CMD ID:29-----------*/
int wlnpi_show_get_rx_ok(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
    FILE *fp = NULL;
    char ret_buf[WLNPI_RESULT_BUF_LEN] = {0x00};
    unsigned int rx_end_count = 0;
    unsigned int rx_err_end_count = 0;
    unsigned int fcs_faiil_count = 0;

    ENG_LOG("ADL entry %s(), r_len = %d", __func__, r_len);

    if(12 != r_len)
    {
        printf("get_rx_ok err\n");
        ENG_LOG("ADL leaving %s(), r_len is ERROR, return -1", __func__);

        return -1;
    }

    rx_end_count = *((unsigned int *)(r_buf+0));
    rx_err_end_count = *((unsigned int *)(r_buf+4));
    fcs_faiil_count = *((unsigned int *)(r_buf+8));

    snprintf(ret_buf, WLNPI_RESULT_BUF_LEN, "ret: reg value: rx_end_count=%d rx_err_end_count=%d fcs_fail_count=%d :end\n", rx_end_count, rx_err_end_count, fcs_faiil_count);

    if(NULL != (fp = fopen(IWNPI_EXEC_TMP_FILE, "w+")))
    {
        int write_cnt = 0;

        write_cnt = fputs(ret_buf, fp);
        ENG_LOG("ADL %s(), callED fputs(%s), write_cnt = %d", __func__, ret_buf, write_cnt);

        fclose(fp);
    }

    printf("%s", ret_buf);
    ENG_LOG("ADL leaving %s(), rx_end_count=%d rx_err_end_count=%d fcs_fail_count=%d, return 0", __func__, rx_end_count, rx_err_end_count, fcs_faiil_count);

    return 0;
}

/*-----CMD ID:34-----------*/
int wlnpi_cmd_get_reg(int argc, char **argv,  unsigned char *s_buf, int *s_len )
{
    unsigned char  *type  = s_buf;
    unsigned int   *addr  = (unsigned int *)(s_buf + 1);
    unsigned int   *count = (unsigned int *)(s_buf + 5);
    char *err;
    if((argc < 2)|| (argc > 3) || (!argv) )
        return -1;
    if(0 == strncmp(argv[0], "mac", 3) )
    {
        *type = 0;
    }
    else if(0 == strncmp(argv[0], "phy0", 4) )
    {
        *type = 1;
    }
    else if(0 == strncmp(argv[0], "phy1", 4) )
    {
        *type = 2;
    }
    else if(0 == strncmp(argv[0], "rf",  2) )
    {
        *type = 3;
    }
    else
    {
        return -1;
    }

    *addr =   strtol(argv[1], &err, 16);
    if(err == argv[1])
    {
        return -1;
    }

    ENG_LOG("ADL %s(), argv[2] addr = %p", __func__, argv[2]);
    if (NULL == argv[2] || 0 == strlen(argv[2]))
    {
        /* if argv[2] is NULL or null string, set count to default value, which value is 1 */
        ENG_LOG("ADL %s(), argv[2] is null, set count to 1", __func__);
        *count = 1;
    }
    else
    {
        *count =  strtol(argv[2], &err, 10);
        if(err == argv[2])
        {
            /* if exec strtol function is error, set count to default value, which value is 1 */
            ENG_LOG("ADL %s(), exec strtol(argv[2]) is error, set count to 1", __func__);
            *count = 1;
        }
    }

    if (*count >= 5)
    {
        ENG_LOG("ADL %s(), *count is too large, *count = %d, set count to 5", __func__, *count);
        *count = 5;
    }
    ENG_LOG("ADL %s(), *count = %d", __func__, *count);

    *s_len = 9;
    return 0;
}

/*-----CMD ID:34-----------*/
#define WLNPI_GET_REG_MAX_COUNT            (20)
int wlnpi_show_get_reg(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
    int i, ret, p;
    FILE *fp = NULL;
    char str[256] = {0};
    char ret_buf[WLNPI_RESULT_BUF_LEN] = {0x00};

    ENG_LOG("ADL entry %s(), r_len = %d", __func__, r_len);

    for(i=0, p =0; (i < r_len/4) && (i < WLNPI_GET_REG_MAX_COUNT); i++)
    {
	if (!(i % 4) && i) {
	    ret = sprintf((str +  p), "\n");
	    p = p + ret;
	}
	ret = sprintf((str +  p),  "0x%08X\t",  *((int *)(r_buf + i*4)));
	p = p + ret;
    }

    snprintf(ret_buf, WLNPI_RESULT_BUF_LEN, "reg values is :%s\n", str);

    if(NULL != (fp = fopen(IWNPI_EXEC_TMP_FILE, "w+")))
    {
        int write_cnt = 0;

        write_cnt = fputs(ret_buf, fp);
        ENG_LOG("ADL %s(), callED fputs(%s), write_cnt = %d", __func__, ret_buf, write_cnt);

        fclose(fp);
    }

    printf("%s", ret_buf);

    ENG_LOG("ADL leaving %s(), str = %s, return 0", __func__, str);

    return 0;
}

/*-----CMD ID:35-----------*/
int wlnpi_cmd_set_reg(int argc, char **argv,  unsigned char *s_buf, int *s_len )
{
    unsigned char  *type  = s_buf;
    unsigned int   *addr  = (unsigned int *)(s_buf + 1);
    unsigned int   *value = (unsigned int *)(s_buf + 5);
    char *err;
    if((argc < 2)|| (argc > 3) || (!argv) )
        return -1;
    if(0 == strncmp(argv[0], "mac", 3) )
    {
        *type = 0;
    }
    else if(0 == strncmp(argv[0], "phy0", 4) )
    {
        *type = 1;
    }
    else if(0 == strncmp(argv[0], "phy1", 4) )
    {
        *type = 2;
    }
    else if(0 == strncmp(argv[0], "rf",  2) )
    {
        *type = 3;
    }
    else
    {
        return -1;
    }
    *addr =   strtol(argv[1], &err, 16);
    if(err == argv[1])
        return -1;

    *value =  strtoul(argv[2], &err, 16);
    if(err == argv[2])
        return -1;
    *s_len = 9;
    return 0;
}

/*-----CMD ID:39-----------*/
int wlnpi_show_get_lna_status(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
    FILE *fp = NULL;
    char ret_buf[WLNPI_RESULT_BUF_LEN+1] = {0x00};
    unsigned char status = 0;

    ENG_LOG("ADL entry %s(), r_eln = %d", __func__, r_len);
    status = r_buf[0];

    snprintf(ret_buf, WLNPI_RESULT_BUF_LEN, "ret: %d :end\n", status);
    printf("%s", ret_buf);

    if(NULL != (fp = fopen(IWNPI_EXEC_TMP_FILE, "w+")))
    {
        int write_cnt = 0;

        write_cnt = fputs(ret_buf, fp);
        ENG_LOG("ADL %s(), write_cnt = %d ret_buf %s", __func__, write_cnt, ret_buf);

        fclose(fp);
    }

    ENG_LOG("ADL leaving %s(), status = %d", __func__, status);
    return 0;
}

/*-----CMD ID:40-----------*/
int wlnpi_cmd_set_wlan_cap(int argc, char **argv,  unsigned char *s_buf, int *s_len )
{
    char *err;
    if(1 != argc)
        return -1;
    *(unsigned int *)s_buf = strtol(argv[0], &err, 10);
    *s_len = 4;
    if(err == argv[0])
        return -1;
    return 0;
}

/*-----CMD ID:41-----------*/
int wlnpi_show_get_wlan_cap(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
    unsigned int cap = 0;

    ENG_LOG("ADL entry %s(), r_len = %d", __func__, r_len);

    if(4 != r_len)
    {
        printf("get_rssi err\n");
        ENG_LOG("ADL leaving %s(), r_len is ERROR, return -1", __func__);

        return -1;
    }

    cap = *(unsigned int *)r_buf;
    printf("ret: %d:end\n", cap );

    ENG_LOG("ADL leaving %s(), cap = %d, return 0", __func__, cap);

    return 0;
}
int wlnpi_cmd_get_reconnect(int argc, char **argv, unsigned char *s_buf, int *s_len)
{
    *s_buf = (unsigned char)GET_RECONNECT;
    *s_len = 1;
    ENG_LOG("ADL entry %s(), subtype = %d, len = %d\n", __func__, *s_buf, *s_len);
    return 0;
}

/*-----CMD ID:42-----------*/
int wlnpi_show_reconnect(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
    unsigned int retcnt = 0;
    FILE *fp = NULL;
    char tmp[64] = {0x00};
    retcnt = *((unsigned int *)(r_buf));

    snprintf(tmp, 64, "reconnect : %d", retcnt);
    ENG_LOG("%s tmp is:%s reconnect result : %d, r_len : %d\n",__func__, tmp, retcnt, r_len);

    if(NULL != (fp = fopen(IWNPI_EXEC_TMP_FILE, "w+"))) {
        int write_cnt = 0;

	write_cnt = fputs(tmp, fp);
        ENG_LOG("ADL %s(), callED fputs(%s) write_cnt = %d", __func__, tmp, write_cnt);
        fclose(fp);
    }else {
	ENG_LOG("%s open file error\n",__func__);
    }

    return 0;
}
/*-----CMD ID:42-----------*/
/********************************************************************
*   name   wlnpi_show_get_connected_ap_info
*   ---------------------------
*   description: get connected ap info
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_show_get_connected_ap_info(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
    assoc_resp resp_ies = {0x00};

    ENG_LOG("ADL entry %s(), r_len = %d", __func__, r_len);

    //iwnpi_hex_dump((unsigned char *)"r_buf:\n", strlen("r_buf:\n"), (unsigned char *)r_buf, r_len);

    if (NULL != r_buf)
    {
        memcpy(&resp_ies, r_buf, r_len);
    }
    else
    {
        ENG_LOG("ADL levaling %s(), resp_ies == NULL!!!", __func__);
        return -1;
    }

    if (!resp_ies.connect_status)
    {
        printf("not connected AP.\n");
        ENG_LOG("ADL levaling %s(), connect_status = %d", __func__, resp_ies.connect_status);

        return 0;
    }

    printf("Current Connected Ap info: \n");

    /* SSID */
    printf("SSID: %s\n", resp_ies.ssid);

    /* connect mode */
    printf("Mode: %s\t", (resp_ies.conn_mode ? IWNPI_CONN_MODE_OTHER : IWNPI_CONN_MODE_MANAGED));

    /* RSSI */
    printf("RSSI: %d dBm\t", resp_ies.rssi);

    /* SNR */
    printf("SNR: %d dB\t", resp_ies.snr);

    /* noise */
    printf("noise: %d dBm\n", resp_ies.noise);

    /* Flags: FromBcn RSSI on-channel Channel */
    printf("Flags: FromBcn RSSI on-channel Channel: %d\n", resp_ies.channel);

    /* BSSID */
    printf("BSSID: %02X:%02X:%02X:%02X:%02X:%02X\n", resp_ies.bssid[0], resp_ies.bssid[1], resp_ies.bssid[2], resp_ies.bssid[3], resp_ies.bssid[4], resp_ies.bssid[5]);

    /* capability */
    wlnpi_ap_info_print_capa(resp_ies.assoc_resp_info);

    /* Support Rates */
    wlnpi_ap_info_print_support_rate(resp_ies.assoc_resp_info);

    /* HT Capable: */
    {
        printf("\n");
        printf("HT Capable:\n");
        printf("Chanspec: %s\t", IWNPI_DEFAULT_CHANSPEC);
        printf("Channel: %d Primary channel: %d\t", resp_ies.channel, resp_ies.channel);
        printf("BandWidth: %s\n", IWNPI_DEFAULT_BANDWIDTH);

        /* HT Capabilities */
        {
            wlnpi_ap_info_print_ht_capa(resp_ies.assoc_resp_info);
            wlnpi_ap_info_print_ht_mcs(resp_ies.assoc_resp_info);
        }
    }

    /* wps */
    wlnpi_ap_info_print_wps(resp_ies.assoc_resp_info);

    printf("\n");

    ENG_LOG("ADL leaving %s(), return 0", __func__);

    return 0;
}

/********************************************************************
*   name   wlnpi_ap_info_print_capa
*   ---------------------------
*   description:
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
static int wlnpi_ap_info_print_capa(const char *data)
{
    short capability = data[2];

    ENG_LOG("ADL entry %s()", __func__);

    capability = iwnpi_get_le16(&data[2]);
    ENG_LOG("ADL %s(), capability = 0x%x", __func__, capability);

    printf("\n");
    printf("Capability:\n");
    if (capability & 1)
    {
        printf("ESS Type Network.\t");
    }
    else if (capability >> 1 & 1)
    {
        printf("IBSS Type Network.\t");
    }

    if (capability >> 2 & 1)
    {
        printf("CF Pollable.\t");
    }

    if (capability >> 3 & 1)
    {
        printf("CF Poll Requested.\t");
    }

    if (capability >> 4 & 1)
    {
        printf("Privacy Enabled.\t");
    }

    if (capability >> 5 & 1)
    {
        printf("Short Preamble.\t");
    }

    if (capability >> 6 & 1)
    {
        printf("PBCC Allowed.\t");
    }

    if (capability >> 7 & 1)
    {
        printf("Channel Agility Used.\t");
    }

    if (capability >> 8 & 1)
    {
        printf("Spectrum Mgmt Enabled.\t");
    }

    if (capability >> 9 & 1)
    {
        printf("QoS Supported.\t");
    }

    if (capability >> 10 & 1)
    {
        printf("G Mode Short Slot Time.\t");
    }

    if (capability >> 11 & 1)
    {
        printf("APSD Supported.\t");
    }

    if (capability >> 12 & 1)
    {
        printf("Radio Measurement.\t");
    }

    if (capability >> 13 & 1)
    {
        printf("DSSS-OFDM Allowed.\t");
    }

    if (capability >> 14 & 1)
    {
        printf("Delayed Block Ack Allowed.\t");
    }

    if (capability >> 15 & 1)
    {
        printf("Immediate Block Ack Allowed.\t");
    }

    printf("\n");
    ENG_LOG("ADL levaling %s()", __func__);
    return 0;
}

/********************************************************************
*   name   wlnpi_ap_info_print_wps
*   ---------------------------
*   description:
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
static int wlnpi_ap_info_print_wps(const char *data)
{
    char    *vendor_ie = NULL;
    char    wps_version = 0;
    char    wifi_protected_setup = 0;

    ENG_LOG("ADL entry %s()", __func__);

    vendor_ie = iwnpi_bss_get_vendor_ie(data, WPS_IE_VENDOR_TYPE);
    if (NULL == vendor_ie)
    {
        ENG_LOG("ADL %s(), get vendor failed. return 0", __func__);
        return 0;
    }

    //iwnpi_hex_dump("vendor:", strlen("vendor:"), (unsigned char *)vendor_ie, 0x16);

    ENG_LOG("ADL %s(), length = %d\n", __func__, vendor_ie[1]);
    wps_version = vendor_ie[10];
    wifi_protected_setup = vendor_ie[15];

    printf("\nWPS:\t");
    printf("0x%02x \t", wps_version);

    if (2 == wifi_protected_setup)
    {
        printf("Configured.");
    }
    else if (3 == wifi_protected_setup)
    {
        printf("AP.");
    }

    printf("\n");

    ENG_LOG("ADL levaling %s()", __func__);
    return 0;
}

/********************************************************************
*   name   wlnpi_ap_info_print_ht_mcs
*   ---------------------------
*   description:
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
static int wlnpi_ap_info_print_ht_mcs(const char *data)
{
    char    *ht_capa_ie = NULL;
    char    spatial_stream1 = 0;
    char    spatial_stream2 = 0;
    char    spatial_stream3 = 0;
    char    spatial_stream4 = 0;

    ht_capa_ie = wlnpi_bss_get_ie(data, WLAN_EID_HT_CAP);
    if (NULL == ht_capa_ie)
    {
        printf("error. get mcs failed.\n");
        ENG_LOG("ADL %s(), get mcs failed. return -1", __func__);

        return -1;
    }

    //iwnpi_hex_dump("ht_capability:", strlen("ht_capability:"), (unsigned char *)ht_capa_ie, 0x16);

    spatial_stream1 = ht_capa_ie[5];
    spatial_stream2 = ht_capa_ie[6];
    spatial_stream3 = ht_capa_ie[7];
    spatial_stream4 = ht_capa_ie[8];

    //printf("spatial1 = 0x%x, spatial2 = 0x%x\n", spatial_stream1, spatial_stream2);

    printf("\nSupported MCS:\n");

    if (spatial_stream1 >> 0 & 1)
    {
        printf("0 ");
    }

    if (spatial_stream1 >> 1 & 1)
    {
        printf("1 ");
    }

    if (spatial_stream1 >> 2 & 1)
    {
        printf("2 ");
    }

    if (spatial_stream1 >> 3 & 1)
    {
        printf("3 ");
    }

    if (spatial_stream1 >> 4 & 1)
    {
        printf("4 ");
    }

    if (spatial_stream1 >> 5 & 1)
    {
        printf("5 ");
    }

    if (spatial_stream1 >> 6 & 1)
    {
        printf("6 ");
    }

    if (spatial_stream1 >> 7 & 1)
    {
        printf("7");
    }

    /* spatial2 */
    if (spatial_stream2 >> 0 & 1)
    {
        printf("8 ");
    }

    if (spatial_stream2 >> 1 & 1)
    {
        printf("9 ");
    }

    if (spatial_stream2 >> 2 & 1)
    {
        printf("10 ");
    }

    if (spatial_stream2 >> 3 & 1)
    {
        printf("11 ");
    }

    if (spatial_stream2 >> 4 & 1)
    {
        printf("12 ");
    }

    if (spatial_stream2 >> 5 & 1)
    {
        printf("13 ");
    }

    if (spatial_stream2 >> 6 & 1)
    {
        printf("14 ");
    }

    if (spatial_stream2 >> 7 & 1)
    {
        printf("15");
    }

    /* spatial3 */
    if (spatial_stream3 >> 0 & 1)
    {
        printf("16 ");
    }

    if (spatial_stream3 >> 1 & 1)
    {
        printf("17 ");
    }

    if (spatial_stream3 >> 2 & 1)
    {
        printf("18 ");
    }

    if (spatial_stream3 >> 3 & 1)
    {
        printf("19 ");
    }

    if (spatial_stream3 >> 4 & 1)
    {
        printf("20 ");
    }

    if (spatial_stream3 >> 5 & 1)
    {
        printf("21 ");
    }

    if (spatial_stream3 >> 6 & 1)
    {
        printf("22 ");
    }

    if (spatial_stream3 >> 7 & 1)
    {
        printf("23");
    }

    /* spatial4 */
    if (spatial_stream4 >> 0 & 1)
    {
        printf("24 ");
    }

    if (spatial_stream4 >> 1 & 1)
    {
        printf("25 ");
    }

    if (spatial_stream4 >> 2 & 1)
    {
        printf("26 ");
    }

    if (spatial_stream4 >> 3 & 1)
    {
        printf("27 ");
    }

    if (spatial_stream4 >> 4 & 1)
    {
        printf("28 ");
    }

    if (spatial_stream4 >> 5 & 1)
    {
        printf("29 ");
    }

    if (spatial_stream4 >> 6 & 1)
    {
        printf("30 ");
    }

    if (spatial_stream4 >> 7 & 1)
    {
        printf("31 ");
    }

    printf("\n");

    return 0;
}

/********************************************************************
*   name   wlnpi_ap_info_print_ht_capa
*   ---------------------------
*   description:
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
static int wlnpi_ap_info_print_ht_capa(const char *data)
{
    char    *ht_capa_ie = NULL;
    short   ht_capa = 0;

    ENG_LOG("ADL entry %s()", __func__);

    ht_capa_ie = wlnpi_bss_get_ie(data, WLAN_EID_HT_CAP);
    if (NULL == ht_capa_ie)
    {
        printf("error. get support_rate failed.\n");
        ENG_LOG("ADL %s(), get support_rate failed. return -1", __func__);

        return -1;
    }

    //iwnpi_hex_dump("ht_capability:", strlen("ht_capability:"), (unsigned char *)ht_capa_ie, 16);

    ENG_LOG("ADL %s(), len = %d\n", __func__, ht_capa_ie[1]);

    ht_capa = iwnpi_get_le16(&ht_capa_ie[2]);

    ENG_LOG("ADL %s(), HT Capabilities = 0x%x\n", __func__, ht_capa);

    printf("\n");
    printf("HT Capabilities:\n");

    if (ht_capa >> 0 & 1)
    {
        printf("LDPC Coding Capability.\n");
    }

    if (ht_capa >> 1 & 1)
    {
        printf("20MHz and 40MHz Operation is Supported.\n");
    }

    if (ht_capa >> 2 & 3)
    {
        printf("Spatial Multiplexing Enabled.\n");
    }

    if (ht_capa >> 4 & 1)
    {
        printf("Can receive PPDUs with HT-Greenfield format.\n");
    }

    if (ht_capa >> 5 & 1)
    {
        printf("Short GI for 20MHz.\n");
    }

    if (ht_capa >> 6 & 1)
    {
        printf("Short GI for 40MHz.\n");
    }

    if (ht_capa >> 7 & 1)
    {
        printf("Transmitter Support Tx STBC.\n");
    }

    if (ht_capa >> 8 & 3)/*2 bit*/
    {
        printf("Rx STBC Support.\n");
    }

    if (ht_capa >> 10 & 1)
    {
        printf("Support HT-Delayed BlockAck Operation.\n");
    }

    if (ht_capa >> 11 & 1)
    {
        printf("Maximal A-MSDU size.\n");
    }

    if (ht_capa >> 12 & 1)
    {
        printf("BSS Allow use of DSSS/CCK Rates @40MHz\n");
    }

    if (ht_capa >> 13 & 1)
    {
        printf("Device/BSS Support use of PSMP.\n");
    }

    if (ht_capa >> 14 & 1)
    {
        printf("AP allow use of 40MHz Transmissions In Neighboring BSSs.\n");
    }

    if (ht_capa >> 15 & 1)
    {
        printf("L-SIG TXOP Protection Support.\n");
    }

    ENG_LOG("ADL leval %s()", __func__);
    return 0;
}

/********************************************************************
*   name   wlnpi_ap_info_print_support_rate
*   ---------------------------
*   description:
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
static int wlnpi_ap_info_print_support_rate(const char *data)
{
    char i = 0;
    char length = 0;
    char *support_rate_ie = NULL;

    ENG_LOG("ADL entry %s()", __func__);

    //iwnpi_hex_dump("data:", strlen("data:"), (unsigned char *)data, 100);
    support_rate_ie = wlnpi_bss_get_ie(data, WLAN_EID_SUPP_RATES);
    if (NULL == support_rate_ie)
    {
        printf("error. get support_rate failed.\n");
        ENG_LOG("ADL %s(), get support_rate failed. return -1", __func__);

        return -1;
    }

    //iwnpi_hex_dump("support rate:", strlen("support rate:"), (unsigned char *)support_rate_ie, 10);

    length = support_rate_ie[1];
    ENG_LOG("ADL %s(), length = %d\n", __func__, length);

    printf("\n");
    printf("Support Rates:\n");

    while (i < length)
    {
        printf("%s  ", wlnpi_get_rate_by_phy(support_rate_ie[2 + i]));
        i++;
    }

    printf("\n");

    ENG_LOG("ADL levaling %s()", __func__);

    return 0;
}

/********************************************************************
*   name   wlnpi_get_rate_by_phy
*   ---------------------------
*   description:
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
static char *wlnpi_get_rate_by_phy(int phy)
{
    unsigned char i = 0;
    char id_num = sizeof(g_rate_table) / sizeof(iwnpi_rate_table);

    for (i = 0; i < id_num; i++)
    {
        if (phy == g_rate_table[i].phy_rate)
        {
            ENG_LOG("\nADL %s(), rate: phy = 0x%2x, str = %s\n", __func__, phy, g_rate_table[i].str_rate);
            return g_rate_table[i].str_rate;
        }
    }

    ENG_LOG("ADL %s(), not match rate, phy = 0x%x", __func__, phy);
    return "";
}

/********************************************************************
*   name   wlnpi_bss_get_ie
*   ---------------------------
*   description:
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
static char *wlnpi_bss_get_ie(const char *bss, char ieid)
{
    short ie_len = 0;
    const char *end, *pos;

    /* get length of ies */
    ie_len = bss[0] + bss[1];
    ENG_LOG("%s(), ie_len = %d\n", __func__, ie_len);

    pos = (const char *) (bss + 2 + 6);/* 6 is capability's length + status code length + AID length */
    end = pos + ie_len - 6;

    while (pos + 1 < end)
    {
        if (pos + 2 + pos[1] > end)
        {
            break;
        }

        if (pos[0] == ieid)
        {
            return (char *)pos;
        }

        pos += 2 + pos[1];
    }

    return NULL;
}

/********************************************************************
*   name   iwnpi_bss_get_vendor_ie
*   ---------------------------
*   description:
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
static char *iwnpi_bss_get_vendor_ie(const char *bss, int vendor_type)
{
    short ie_len = 0;
    const char *end, *pos;

    /* get length of ies */
    ie_len = bss[0] + bss[1];

    pos = (const char *)(bss + 2+6);
    end = pos + ie_len - 6;

    while (pos + 1 < end)
    {
        if (pos + 2 + pos[1] > end)
        {
            break;
        }

        if (pos[0] == WLAN_EID_VENDOR_SPECIFIC && pos[1] >= 4 && vendor_type == iwnpi_get_be32(&pos[2]))
        {
            return (char *)pos;
        }
        pos += 2 + pos[1];
    }

    return NULL;
}

/*-----CMD ID:43-----------*/
/********************************************************************
*   name   wlnpi_show_get_mcs_index
*   ---------------------------
*   description:
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_show_get_mcs_index(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
    unsigned int mcs_index = 0;

    ENG_LOG("ADL entry %s(), r_len = %d", __func__, r_len);

    if(1 != r_len)
    {
        printf("get msc index err \n");
        ENG_LOG("ADL leaving %s(), r_len is ERROR, return -1", __func__);

        return -1;
    }

    mcs_index = *( (unsigned char *)(r_buf+0) ) ;
    printf("mcs index: %d\n", mcs_index);

    ENG_LOG("ADL leaving %s(), mcs_index = %d, return 0", __func__, mcs_index);

    return 0;
}

/*-----CMD ID:44-----------*/
/********************************************************************
*   name   wlnpi_cmd_set_ar
*   ---------------------------
*   description: set autorate's flag and index
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_cmd_set_ar(int argc, char **argv, unsigned char *s_buf, int *s_len )
{
    char *endptr = NULL;
    unsigned char   *ar_flag  = (unsigned char *)(s_buf + 0);
    unsigned char   *ar_index = (unsigned char *)(s_buf + 1);
    unsigned char   *ar_sgi   = (unsigned char *)(s_buf + 2);

    ENG_LOG("ADL entry %s(), argc = %d", __func__, argc);

    if((argc < 1)|| (argc > 4) || (!argv))
    {
        ENG_LOG("ADL leaving %s(), argc ERR.", __func__);
        return -1;
    }

    *ar_flag = strtoul(argv[0], &endptr, 10);
    if(*endptr != '\0')
    {
        ENG_LOG("ADL leaving %s(), strtol(ar_flag) ERR.", __func__);
        return -1;
    }

    if (1 == *ar_flag)
    {
        *ar_index = strtoul(argv[1], &endptr, 10);
        if(*endptr != '\0')
        {
            ENG_LOG("ADL leaving %s(), strtol(ar_index) ERR.", __func__);
            return -1;
        }

        *ar_sgi = strtoul(argv[2], &endptr, 10);
        if(*endptr != '\0')
        {
            ENG_LOG("ADL leaving %s(), strtol(ar_sgi) ERR.", __func__);
            return -1;
        }
    }
    else if (0 == *ar_flag)
    {
        *ar_index = 0;
        *ar_sgi = 0;
    }
    else
    {
        ENG_LOG("ADL leaving %s(), ar_flag is invalid. *ar_flag = %d", __func__, *ar_flag);
        return -1;
    }

    *s_len = 3;

    ENG_LOG("ADL leaving %s(), ar_flag = %d, ar_index = %d, *s_len = %d", __func__, *ar_flag, *ar_index, *s_len);

    return 0;
}


/*-----CMD ID:45-----------*/
/********************************************************************
*   name   wlnpi_show_get_ar
*   ---------------------------
*   description: set autorate's flag and autorate index
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_show_get_ar(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
    unsigned char ar_flag = 0;
    unsigned char ar_index = 0;

    ENG_LOG("ADL entry %s(), r_len = %d", __func__, r_len);

    ar_flag  = *(unsigned char *)(r_buf + 0);
    ar_index = *(unsigned char *)(r_buf + 1);

    printf("ret: %d, %d :end\n", ar_flag, ar_index);

    ENG_LOG("ADL leaving %s(), ar_flag = %d, ar_index= %d, return 0", __func__, ar_flag, ar_index);

    return 0;
}

/*-----CMD ID:46-----------*/
/********************************************************************
*   name   wlnpi_cmd_set_ar_pktcnt
*   ---------------------------
*   description: set autorate's pktcnt
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_cmd_set_ar_pktcnt(int argc, char **argv,  unsigned char *s_buf, int *s_len )
{
    char *err;

    ENG_LOG("ADL entry %s(), argc = %d", __func__, argc);

    if(1 != argc)
    {
        ENG_LOG("ADL leaving %s(), argc error. return -1", __func__);
        return -1;
    }

    *((unsigned int *)s_buf) = strtoul(argv[0], &err, 10);
    if(err == argv[0])
    {
        ENG_LOG("ADL leaving %s(), strtol error. return -1", __func__);
        return -1;
    }

    *s_len = 4;

    ENG_LOG("ADL leaving %s()", __func__);
    return 0;
}

/*-----CMD ID:47-----------*/
/********************************************************************
*   name   wlnpi_show_get_ar_pktcnt
*   ---------------------------
*   description: get autorate's pktcnt
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_show_get_ar_pktcnt(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
    unsigned int pktcnt = 0;
    ENG_LOG("ADL entry %s(), r_len = %d", __func__, r_len);

    if(4 != r_len)
    {
        printf("get_ar_pktcnt err\n");
        ENG_LOG("ADL leaving %s(), r_len is ERROR, return -1", __func__);
        return -1;
    }

    pktcnt = *((unsigned int *)(r_buf+0));
    printf("ret: %d :end\n", pktcnt);

    ENG_LOG("ADL leaving %s(), pktcnt = %d, return 0", __func__, pktcnt);

    return 0;
}

/*-----CMD ID:48-----------*/
/********************************************************************
*   name   wlnpi_cmd_set_ar_retcnt
*   ---------------------------
*   description: set autorate's retcnt
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_cmd_set_ar_retcnt(int argc, char **argv,  unsigned char *s_buf, int *s_len )
{
    char *err;

    ENG_LOG("ADL entry %s(), argc = %d", __func__, argc);

    if(1 != argc)
    {
        ENG_LOG("ADL leaving %s(), argc error. return -1", __func__);
        return -1;
    }

    *((unsigned int *)s_buf) = strtoul(argv[0], &err, 10);
    if(err == argv[0])
    {
        ENG_LOG("ADL leaving %s(), strtol error. return -1", __func__);
        return -1;
    }

    *s_len = 4;

    ENG_LOG("ADL leaving %s()", __func__);
    return 0;
}

/*-----CMD ID:49-----------*/
/********************************************************************
*   name   wlnpi_show_get_ar_retcnt
*   ---------------------------
*   description: get autorate's retcnt
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_show_get_ar_retcnt(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
    unsigned int retcnt = 0;
    ENG_LOG("ADL entry %s(), r_len = %d", __func__, r_len);

    if(4 != r_len)
    {
        printf("get_ar_retcnt err\n");
        ENG_LOG("ADL leaving %s(), r_len is ERROR, return -1", __func__);
        return -1;
    }

    retcnt = *((unsigned int *)(r_buf+0));
    printf("ret: %d :end\n", retcnt);

    ENG_LOG("ADL leaving %s(), retcnt = %d, return 0", __func__, retcnt);

    return 0;
}

/*-----CMD ID:51-----------*/
int wlnpi_cmd_roam(int argc, char **argv,  unsigned char *s_buf, int *s_len)
{
    char *err;
    int index = 0;
    unsigned char *tmp = s_buf;
	int length;

    if(3 != argc)
        return -1;

    *tmp = (unsigned char)strtol(argv[0], &err, 10);
    if(err == argv[0])
        return -1;

    *s_len = 1;
    tmp = tmp + 1;

    mac_addr_a2n(tmp, argv[1]);
    *s_len += 6;
    tmp = tmp + 6;

    length = strlen(argv[2]) + 1;
    strncpy((char *)tmp, argv[2], length);
    *s_len += length;

    ENG_LOG("roam  channel:%d mac:%02x:%02x:%02x:%02x:%02x:%02x ssid:%s\n",s_buf[0],
	    s_buf[1],s_buf[2],s_buf[3],s_buf[4],s_buf[5],s_buf[6],s_buf +7);

    return 0;
}

/*-----CMD ID:51-----------*/
int wlnpi_cmd_set_wmm(int argc, char **argv, unsigned char *s_buf, int *s_len)
{
    char *err;
    unsigned char *tmp = s_buf;
    int type, cwmin, cwmax, aifs, txop;

    if(5 != argc) {
	ENG_LOG("Error ! para num is : %d, required 5\n", argc);
	return -1;
    }

    /*set be/bk/vi/vo */
    if (strcmp(argv[0], "be") == 0) {
	ENG_LOG("it is be \n");
	type = 0;
    } else if(strcmp(argv[0], "bk") ==0) {
	ENG_LOG("it is bk \n");
	type = 1;
    } else if(strcmp(argv[0], "vi") ==0) {
	ENG_LOG("it is vi \n");
	type = 2;
    } else if(strcmp(argv[0], "vo") ==0) {
	ENG_LOG("it is vo \n");
	type = 3;
    } else {
	ENG_LOG("invalid para : %s \n", argv[0]);
	return -1;
    }
    *tmp = type;
    *s_len = 1;
    tmp += 1;
    if (!is_digital(argv[1])) {
	ENG_LOG("Invalid cwmin para : %s\n", argv[1]);
	return -1;
    }

    /*set cwmin*/
    cwmin = strtol(argv[1], &err, 10);
    if (err == argv[1]) {
	ENG_LOG("get cwmin failed \n");
	return -1;
    }
    if (cwmin < 0 || cwmin > 255) {
	ENG_LOG("invalid cwmin value : %d \n", cwmin);
	return -1;
    }
    *tmp = cwmin;
    *s_len += 1;
    tmp += 1;

    /*set cwmax*/
    if (!is_digital(argv[2])) {
	ENG_LOG("Invalid cwmax para : %s\n",argv[2]);
	return -1;
    }
    cwmax = strtol(argv[2], &err, 10);
    if (err == argv[2]) {
	ENG_LOG("get cwmax failed \n");
	return -1;
    }
    if (cwmax < 0 || cwmax > 255) {
	ENG_LOG("invalid cwmax value : %d \n", cwmax);
	return -1;
    }
    *tmp = cwmax;
    *s_len += 1;
    tmp += 1;

    if (cwmin > cwmax) {
	ENG_LOG("cwmin larger than cwmax\n");
	return -1;
    }

    /*set aifs*/
    if (!is_digital(argv[3])) {
	ENG_LOG("Invalid aifs para : %s\n",argv[3]);
	return -1;
    }
    aifs = strtol(argv[3], &err, 10);
    if (err == argv[3]) {
	ENG_LOG("get aifs failed\n");
	return -1;
    }
    if (aifs < 0 || aifs > 255) {
	ENG_LOG("invalid aifs value : %d \n", aifs);
	return -1;
    }
    *tmp = aifs;
    *s_len += 1;
    tmp += 1;

    /*set txop*/
    if (!is_digital(argv[4])) {
	ENG_LOG("Invalid txop para : %s\n", argv[4]);
	return -1;
    }
    txop = strtol(argv[4], &err, 10);
    if (err == argv[4]) {
	ENG_LOG("get txop failed\n");
	return -1;
    }
    if (txop < 0 || txop > 65535) {
	ENG_LOG("invalid txop value : %d \n", txop);
	return -1;
    }
    *(unsigned short *)tmp = txop;
    *s_len += 2;

    printf("show para,len : %d \n", *s_len);
    printf("be|bk|vo|vi : %d \n", *s_buf);
    printf("cwmin : %d \n", *(s_buf + 1));
    printf("cwmax : %d \n", *(s_buf + 2));
    printf("aifs  : %d \n", *(s_buf + 3));
    printf("txop  : %d \n", *(unsigned short *)(s_buf + 4));

    return 0;
}

/*-----CMD ID:52-----------*/
int wlnpi_cmd_set_eng_mode(int argc, char **argv, unsigned char *s_buf, int *s_len)
{
    int value;
    unsigned char *p = s_buf;
    char *err;

#define SET_CCA_STATE  1
#define GET_CCA_STATE  2
#define SET_SCAN_STATE 3
#define GET_SCAN_STATE 4

    ENG_LOG("ADL entry %s(), argc = %d , argv[0] = %s", __func__, argc, argv[0]);

    if((1 != argc && 2 != argc) || p == NULL)
        return -1;

	/* command id (1: Adaptive Mode) */
	value = strtoul(argv[0], &err, 10);
	if (err == argv[0])
	    return -1;

	*p++ = value;

	switch (value)
	{
		case SET_CCA_STATE:
		case SET_SCAN_STATE:
			/* data length */
			*p++ = 1;

			/* data (0: off, 1: on) */
			value = strtoul(argv[1], &err, 10);
			if (err == argv[1])
			    return -1;

			*p = value;
			*s_len = 3;
			break;

		default:
			*s_len = 1;
			break;
	}

    ENG_LOG("ADL leaving %s()", __func__);
    return 0;
}

/*-----CMD ID:55-----------*/
/******************************************************************
*   name   wlnpi_cmd_start_pkt_log
*   ---------------------------
*   description:
*   ----------------------------
*   para        IN/OUT      type            note
*   u16 buffer_num, u16 duration(default : 0)
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_cmd_start_pkt_log(int argc, char **argv,  unsigned char *s_buf, int *s_len )
{
    char **err = NULL;
	unsigned short *buffer_num = (unsigned short *)s_buf;
    unsigned short *duration = (unsigned short *)(s_buf + 2);

    ENG_LOG("ADL entry %s(), argc = %d", __func__, argc);

    if(2 != argc)
    {
		ENG_LOG("ADL leaving %s(), argc error. return -1", __func__);
		return -1;
    }

    *buffer_num = strtoul(argv[0], err, 10);
    if(err)
    {
		ENG_LOG("ADL leaving %s(), strtol error. return -1", __func__);
		return -1;
    }

    *duration = strtoul(argv[1], err, 10);
    if(err)
    {
		ENG_LOG("ADL leaving %s(), strtol error. return -1", __func__);
		return -1;
    }

    *s_len = 4;

    ENG_LOG("ADL leaving %s()", __func__);
    return 0;
}

/*-----CMD ID:78-----------*/
/********************************************************************
*   name   wlnpi_cmd_set_chain
*   ---------------------------
*   description:
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*	bit 0: Primary channel
*	bit 1: Second channel
*
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_cmd_set_chain(int argc, char **argv, unsigned char *s_buf, int *s_len)
{
	char *err;

	ENG_LOG("ADL entry %s(), argc = %d, argv[0] = %s", __func__, argc, argv[0]);
	if(1 != argc)
	{
		return -1;
	}

	*((unsigned int *)s_buf) = strtol(argv[0], &err, 10);
	if(err == argv[0])
	{
		return -1;
	}
	if((*s_buf >3) || (0 == *s_buf))
	{
		ENG_LOG("index value:%d, not equal to 1,2,3", *s_buf);
		return -1;
	}

	*s_len = 1;

	ENG_LOG("ADL leaving %s()", __func__);
	return 0;
}

/*-----CMD ID:79-----------*/
/********************************************************************
*   name   wlnpi_show_get_chain
*   ---------------------------
*   description:
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_show_get_chain(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
	unsigned char chain = 255;

	ENG_LOG("ADL entry %s(), r_len = %d", __func__, r_len);

	if(1 != r_len)
	{
		printf("get_chain err\n");
		ENG_LOG("ADL leaving %s(), r_len is ERROR, return -1", __func__);

		return -1;
	}

	chain = *((unsigned char *)r_buf);
	if((chain > 3) || (0 == chain))
	{
		printf("get_chain err,not equal to 1,2,3\n");
		return -1;
	}

	printf("ret: %d :end\n", chain);

	ENG_LOG("ADL leaving %s(), chain = %d, return 0", __func__, chain);

	return 0;
}

/*-----CMD ID:80-----------*/
/********************************************************************
*   name   wlnpi_cmd_set_sbw
*   ---------------------------
*   description:
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   para: 
*   0 stand for 20M
*   1 stand for 40M
*   2 stand for 80M
*   3 stand for 160M
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_cmd_set_sbw(int argc, char **argv, unsigned char *s_buf, int *s_len)
{
	char *err;

	ENG_LOG("ADL entry %s(), argc = %d, argv[0] = %s", __func__, argc, argv[0]);
	if(1 != argc)
	{
		return -1;
	}

	*((unsigned int *)s_buf) = strtol(argv[0], &err, 10);
	if(err == argv[0])
	{
		return -1;
	}
	if(*s_buf > 3)
	{
		ENG_LOG("index value:%d, exceed define!", *s_buf);
		return -1;
	}

	*s_len = 1;

	ENG_LOG("ADL leaving %s()", __func__);
	return 0;
}
/*-----CMD ID:81-----------*/
/********************************************************************
*   name   wlnpi_show_get_sbw
*   ---------------------------
*   description:
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_show_get_sbw(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
	unsigned char sbw = 255;

	ENG_LOG("ADL entry %s(), r_len = %d", __func__, r_len);

	if(1 != r_len)
	{
		printf("get_sbw err\n");
		ENG_LOG("ADL leaving %s(), r_len is ERROR, return -1", __func__);

		return -1;
	}

	sbw = *( (unsigned char *)r_buf );
	if(sbw > 3)
	{
		printf("get_sbw err\n");
		return -1;
	}

	printf("ret: %d :end\n", sbw);

	ENG_LOG("ADL leaving %s(), chain = %d, return 0", __func__, sbw);

	return 0;
}
/*-----CMD ID:88-----------*/
/********************************************************************
*   name   wlnpi_cmd_set_fec
*   ---------------------------
*   description:
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   para: 0 stand for BCC 1 stand for LDPC
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_cmd_set_fec(int argc, char **argv, unsigned char *s_buf, int *s_len)
{
	char *err;

	ENG_LOG("ADL entry %s(), argc = %d, argv[0] = %s", __func__, argc, argv[0]);
	if(1 != argc)
	{
		return -1;
	}

	*((unsigned int *)s_buf) = strtol(argv[0], &err, 10);
	if(err == argv[0])
	{
		return -1;
	}
	if((0 != *s_buf) &&(1 != *s_buf))
	{
		ENG_LOG("index value:%d, not equal to 0 or 1", *s_buf);
		return -1;
	}

	*s_len = 1;

	ENG_LOG("ADL leaving %s()", __func__);
	return 0;
}

/*-----CMD ID:89-----------*/
/********************************************************************
*   name   wlnpi_show_get_fec
*   ---------------------------
*   description:
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_show_get_fec(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
	unsigned char fec = 255;

	ENG_LOG("ADL entry %s(), r_len = %d", __func__, r_len);

	if(1 != r_len)
	{
		printf("get_fec err\n");
		ENG_LOG("ADL leaving %s(), r_len is ERROR, return -1", __func__);

		return -1;
	}

	fec = *( (unsigned char *)r_buf );
	if((0 != fec) &&(1 != fec))
	{
		printf("get_fec err\n");
		return -1;
	}

	printf("ret: %d :end\n", fec);

	ENG_LOG("ADL leaving %s(), fec = %d, return 0", __func__, fec);

	return 0;
}

/*-----CMD ID:115-----------*/
/********************************************************************
*   name   wlnpi_cmd_set_prot_mode
*   ---------------------------
*   description: set protection mode
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_cmd_set_prot_mode(int argc, char **argv,  unsigned char *s_buf, int *s_len)
{
	char **err = NULL;
	unsigned char *mode = s_buf;
	unsigned char *prot_mode = s_buf + 1;
	int val;

	ENG_LOG("ADL entry %s(), argc = %d", __func__, argc);

	if(2 != argc)
	{
		ENG_LOG("ADL leaving %s(), argc error. return -1", __func__);
		return -1;
	}

	*mode = strtoul(argv[0], err, 10);
	if(err)
	{
	ENG_LOG("ADL leaving %s(), strtol error. return -1", __func__);
	return -1;
	}

	val = strtoul(argv[1], err, 10);
	if (err || val > 0xff)
	{
		ENG_LOG("prot_mode = %d err", val);
	return -1;
	}

	*prot_mode = val;

	*s_len = 2;

	ENG_LOG("ADL leaving %s()", __func__);
	return 0;
}

/*-----CMD ID:116-----------*/
/********************************************************************
*   name   wlnpi_cmd_get_prot_mode
*   ---------------------------
*   description: get protection mode
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_cmd_get_prot_mode(int argc, char **argv,  unsigned char *s_buf, int *s_len)
{
	char **err = NULL;
	unsigned char *mode = s_buf;
	int val;

    ENG_LOG("ADL entry %s(), argc = %d", __func__, argc);

	if(1 != argc)
	{
		ENG_LOG("ADL leaving %s(), argc error. return -1", __func__);
		return -1;
	}

	val = strtoul(argv[0], err, 10);
	if(err && val > 0xff)
	{
		ENG_LOG("ADL leaving %s(), strtol error. return -1", __func__);
		return -1;
	}

	*mode = val;
	*s_len = 1;

	ENG_LOG("ADL leaving %s()", __func__);
	return 0;
}

/*-----CMD ID:116-----------*/
/********************************************************************
*   name   wlnpi_show_get_prot_mode
*   ---------------------------
*   description: show protection mode
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_show_get_prot_mode(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
	unsigned int retcnt = 0;
	ENG_LOG("ADL entry %s(), r_len = %d", __func__, r_len);


	if (r_len == 1)
	{
		retcnt = *((unsigned char*)(r_buf+0));
	}
	else if (r_len == 4)
	{
		retcnt = *((unsigned int *)(r_buf+0));
	}
	else
	{
		printf("get_ar_retcnt err\n");
		ENG_LOG("ADL leaving %s(), r_len is ERROR, return -1", __func__);
		return -1;
	}

	printf("ret: %d :end\n", retcnt);

	ENG_LOG("ADL leaving %s(), retcnt = %d, return 0", __func__, retcnt);

	return 0;
}

/*-----CMD ID:117-----------*/
/********************************************************************
 *   name   wlnpi_cmd_set_threshold
 *   ---------------------------
 *   description: set threshold rts frag
 *   ----------------------------
 *   para        IN/OUT      type            note
 *   ----------------------------------------------------
 *   return
 *   0:exec successful
 *   -1:error
 *   ------------------
 *   other:
 *
 ********************************************************************/
int wlnpi_cmd_set_threshold(int argc, char **argv,  unsigned char *s_buf, int *s_len)
{
    char **err = NULL;
	unsigned char *mode = s_buf;
    unsigned int *rts = (unsigned int *)(s_buf + 1);
    unsigned int *frag = (unsigned int *)(s_buf + 5);

    ENG_LOG("ADL entry %s(), argc = %d", __func__, argc);

    if(3 != argc)
    {
		ENG_LOG("ADL leaving %s(), argc error. return -1", __func__);
		return -1;
    }

    *mode = strtoul(argv[0], err, 10);
    if(err)
    {
		ENG_LOG("ADL leaving %s(), strtol error. return -1", __func__);
		return -1;
    }

    *rts = strtoul(argv[1], err, 10);
    if(err)
    {
		ENG_LOG("ADL leaving %s(), strtol error. return -1", __func__);
		return -1;
    }

    *frag = strtoul(argv[2], err, 10);
    if(err)
    {
		ENG_LOG("ADL leaving %s(), strtol error. return -1", __func__);
		return -1;
    }
    //iwnpi_hex_dump("D:", 2, s_buf, 9);
    *s_len = 9;

    ENG_LOG("ADL leaving %s()", __func__);
    return 0;
}

/*-----CMD ID:119-----------*/
int wlnpi_cmd_set_pm_ctl(int argc, char **argv, unsigned char *s_buf, int *s_len)
{
    char *err;
    if(1 != argc)
        return -1;
    *(unsigned int *)s_buf = strtol(argv[0], &err, 10);
    *s_len = 4;
    if(err == argv[0])
        return -1;
    return 0;
}

/*-----CMD ID:120-----------*/
int wlnpi_show_get_pm_ctl(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
    unsigned int ctl = 0;

    ENG_LOG("ADL entry %s(), r_len = %d", __func__, r_len);

    if(4 != r_len)
    {
        printf("get_pm ctl err\n");
        ENG_LOG("ADL leaving %s(), r_len is ERROR, return -1", __func__);

        return -1;
    }

    ctl = *(unsigned int *)r_buf;
    printf("ret: %d:end\n", ctl);

    ENG_LOG("ADL leaving %s(), pm_ctl = %d, return 0", __func__, ctl);

    return 0;
}

int wlnpi_show_get_pa_infor(struct wlnpi_cmd_t *cmd,
			    unsigned char *r_buf,
			    int r_len)
{
	int len = 0;
	struct pa_info_struct *pa_info = (struct pa_info_struct *)r_buf;

	ENG_LOG("ADL entry %s(),id=%d, %s, r_len = %d",
		__func__, cmd->id, cmd->name, r_len);

	if ((1 > r_len) || (512 < r_len)) {
		printf("get_cp2_infor cmd_name:%s err\n", cmd->name);
		ENG_LOG("ADL leaving %s(), r_len is ERROR, return -1",
			__func__);
		return -1;
	}

	printf("pa status information:\r\n");
	printf("rx_giveup_cnt:%08x\r\n", pa_info->rx_giveup_cnt);
	printf("rx_drop_cnt:%08x\r\n", pa_info->rx_drop_cnt);
	printf("rx_filter_cnt_frmtype:%08x\r\n",
	       pa_info->rx_filter_cnt_frmtype);
	printf("rx_ce_err_info:%08x\r\n", pa_info->rx_ce_err_info);
	printf("rx_dup_cnt:%08x\r\n", pa_info->rx_dup_cnt);
	printf("rx_stage23_mpdu_cnt:%08x\r\n", pa_info->rx_stage23_mpdu_cnt);
	printf("rx_bitmap_chk_cnt:%08x\r\n", pa_info->rx_bitmap_chk_cnt);
	printf("rx_int_cnt:%08x\r\n", pa_info->rx_int_cnt);
	printf("rx_mpdu_debug_cnt:%08x\r\n", pa_info->rx_mpdu_debug_cnt);
	printf("rx_ce_debug:%08x\r\n", pa_info->rx_ce_debug);
	printf("rx_psdu_cnt:%08x\r\n", pa_info->rx_psdu_cnt);
	printf("rx_mpdu_cnt1:%08x\r\n", pa_info->rx_mpdu_cnt1);
	printf("rx_mpdu_cnt2:%08x\r\n", pa_info->rx_mpdu_cnt2);
	printf("rx_mac1_mpdu_bytecnt:%08x\r\n", pa_info->rx_mac1_mpdu_bytecnt);
	printf("rx_mac2_mpdu_bytecnt:%08x\r\n", pa_info->rx_mac2_mpdu_bytecnt);
	printf("rx_mac3_mpdu_bytecnt:%08x\r\n", pa_info->rx_mac3_mpdu_bytecnt);
	printf("rx_mac4_mpdu_bytecnt:%08x\r\n", pa_info->rx_mac4_mpdu_bytecnt);
	printf("rx_mac1_lastsuc_datafrm_tsf_l:%08x\r\n",
	       pa_info->rx_mac1_lastsuc_datafrm_tsf_l);
	printf("rx_mac2_lastsuc_datafrm_tsf_l:%08x\r\n",
		pa_info->rx_mac2_lastsuc_datafrm_tsf_l);
	printf("rx_mac3_lastsuc_datafrm_tsf_l:%08x\r\n",
		pa_info->rx_mac3_lastsuc_datafrm_tsf_l);
	printf("rx_mac4_lastsuc_datafrm_tsf_l:%08x\r\n",
		pa_info->rx_mac4_lastsuc_datafrm_tsf_l);
	printf("mib_cycle_cnt:%08x\r\n", pa_info->mib_cycle_cnt);
	printf("mib_rx_clear_cnt_20m:%08x\r\n", pa_info->mib_rx_clear_cnt_20m);
	printf("mib_rx_clear_cnt_40m:%08x\r\n", pa_info->mib_rx_clear_cnt_40m);
	printf("mib_rx_clear_cnt_80m:%08x\r\n", pa_info->mib_rx_clear_cnt_80m);
	printf("mib_rx_cycle_cnt:%08x\r\n", pa_info->mib_rx_cycle_cnt);
	printf("mib_myrx_uc_cycle_cnt:%08x\r\n",
		pa_info->mib_myrx_uc_cycle_cnt);
	printf("mib_myrx_bcmc_cycle_cnt:%08x\r\n",
		   pa_info->mib_myrx_bcmc_cycle_cnt);
	printf("mib_rx_idle_cnt:%08x\r\n", pa_info->mib_rx_idle_cnt);
	printf("mib_tx_cycle_cnt:%08x\r\n", pa_info->mib_tx_cycle_cnt);
	printf("mib_tx_idle_cnt:%08x\r\n", pa_info->mib_tx_idle_cnt);
	ENG_LOG("ADL leaving %s(), return 0", __func__);
	return 0;
}

int wlnpi_show_get_tx_status(struct wlnpi_cmd_t *cmd,
			     unsigned char *r_buf,
			     int r_len)
{
	int len = 0, i = 0;
	struct tx_status_struct *tx_status = (struct tx_status_struct *)r_buf;

	ENG_LOG("ADL entry %s(),id=%d, %s, r_len = %d",
		    __func__, cmd->id, cmd->name, r_len);

	if ((1 > r_len) || (512 < r_len)) {
		printf("get_cp2_infor cmd_name:%s err\n", cmd->name);
		ENG_LOG("ADL leaving %s(), r_len is ERROR, return -1",
			__func__);
		return -1;
	}

	printf("tx status information:\r\n");
	for (i = 0; i < 4; i++) {
		printf("tx_pkt_cnt[%d]:%08x\r\n", i,
		       tx_status->tx_pkt_cnt[i]);
	}
	for (i = 0; i < 4; i++) {
		printf("tx_suc_cnt[%d]:%08x\r\n", i,
		       tx_status->tx_suc_cnt[i]);
	}
	for (i = 0; i < 4; i++) {
		printf("tx_fail_cnt[%d]:%08x\r\n", i,
		       tx_status->tx_fail_cnt[i]);
	}
	for (i = 0; i < 2; i++) {
		printf("tx_fail_reason_cnt[%d]:%08x\r\n", i,
		       tx_status->tx_fail_reason_cnt[i]);
	}
	printf("tx_err_cnt:%08x\r\n", tx_status->tx_err_cnt);
	printf("rts_success_cnt:%08x\r\n", tx_status->rts_success_cnt);
	printf("rts_fail_cnt:%08x\r\n", tx_status->rts_fail_cnt);
	printf("retry_cnt:%08x\r\n", tx_status->retry_cnt);
	printf("ampdu_retry_cnt:%08x\r\n", tx_status->ampdu_retry_cnt);
	printf("ba_rx_fail_cnt:%08x\r\n", tx_status->ba_rx_fail_cnt);
	for (i = 0; i < 4; i++) {
		printf("color_num_sdio[%d]:%02x\r\n", i,
		       tx_status->color_num_sdio[i]);
	}
	for (i = 0; i < 4; i++) {
		printf("color_num_mac[%d]:%02x\r\n", i,
		       tx_status->color_num_mac[i]);
	}
	for (i = 0; i < 4; i++) {
		printf("color_num_txc[%d]:%02x\r\n", i,
		       tx_status->color_num_txc[i]);
	}

	ENG_LOG("ADL leaving %s(), return 0", __func__);

	return 0;
}

int wlnpi_show_get_rx_status(struct wlnpi_cmd_t *cmd,
			     unsigned char *r_buf,
			     int r_len)
{
	int len = 0, i = 0;
	struct rx_status_struct *rx_status = (struct rx_status_struct *)r_buf;

	ENG_LOG("ADL entry %s(),id=%d, %s, r_len = %d",
		__func__,cmd->id, cmd->name, r_len);

	if ((1 > r_len) || (512 < r_len)) {
		printf("get_cp2_infor cmd_name:%s err\n", cmd->name);
		ENG_LOG("ADL leaving %s(), r_len is ERROR, return -1", __func__);
		return -1;
	}

	printf("rx status information:\r\n");

	for (i = 0; i < 4; i++) {
		printf("rx_pkt_cnt[%d]:%08x\r\n", i,
		       rx_status->rx_pkt_cnt[i]);
	}
	for (i = 0; i < 4; i++) {
		printf("rx_retry_pkt_cnt[%d]:%08x\r\n", i,
		       rx_status->rx_retry_pkt_cnt[i]);
	}
	printf("rx_su_beamformed_pkt_cnt:%08x\r\n",
	       rx_status->rx_su_beamformed_pkt_cnt);
	printf("rx_mu_pkt_cnt:%08x\r\n",
	       rx_status->rx_mu_pkt_cnt);
	for (i = 0; i < 16; i++) {
		printf("rx_11n_mcs_cnt[%d]:%08x\r\n", i,
		       rx_status->rx_11n_mcs_cnt[i]);
	}
	for (i = 0; i < 16; i++) {
		printf("rx_11n_sgi_cnt[%d]:%08x\r\n", i,
		       rx_status->rx_11n_sgi_cnt[i]);
	}
	for (i = 0; i < 10; i++) {
		printf("rx_11ac_mcs_cnt[%d]:%08x\r\n", i,
		       rx_status->rx_11ac_mcs_cnt[i]);
	}
	for (i = 0; i < 10; i++) {
		printf("rx_11ac_sgi_cnt[%d]:%08x\r\n", i,
		       rx_status->rx_11ac_sgi_cnt[i]);
	}
	for (i = 0; i < 2; i++) {
		printf("rx_11ac_stream_cnt[%d]:%08x\r\n", i,
		       rx_status->rx_11ac_stream_cnt[i]);
	}
	for (i = 0; i < 3; i++) {
		printf("rx_bandwidth_cnt[%d]:%08x\r\n", i,
		       rx_status->rx_bandwidth_cnt[i]);
	}
	for (i = 0; i < 3; i++) {
		printf("last_rxdata_rssi1[%d]:%02x\r\n", i,
		       rx_status->rx_rssi[i].last_rxdata_rssi1);
		printf("last_rxdata_rssi2[%d]:%02x\r\n", i,
		       rx_status->rx_rssi[i].last_rxdata_rssi2);
		printf("last_rxdata_snr1[%d]:%02x\r\n", i,
		       rx_status->rx_rssi[i].last_rxdata_snr1);
		printf("last_rxdata_snr2[%d]:%02x\r\n", i,
		       rx_status->rx_rssi[i].last_rxdata_snr2);
		printf("last_rxdata_snr_combo[%d]:%02x\r\n", i,
		       rx_status->rx_rssi[i].last_rxdata_snr_combo);
		printf("last_rxdata_snr_l[%d]:%02x\r\n", i,
		       rx_status->rx_rssi[i].last_rxdata_snr_l);
	}
	for (i = 0; i < 5; i++) {
		printf("rxc_isr_cnt[%d]:%08x\r\n", i,
		       rx_status->rxc_isr_cnt[i]);
	}
	for (i = 0; i < 5; i++) {
		printf("rxq_buffer_rqst_isr_cnt[%d]:%08x\r\n", i,
		       rx_status->rxq_buffer_rqst_isr_cnt[i]);
	}
	for (i = 0; i < 5; i++) {
		printf("req_tgrt_bu_num[%d]:%04x\r\n", i,
		       rx_status->req_tgrt_bu_num[i]);
	}
	for (i = 0; i < 3; i++) {
		printf("rx_alloc_pkt_num[%d]:%04x\r\n", i,
		       rx_status->rx_alloc_pkt_num[i]);
	}
	ENG_LOG("ADL leaving %s(), return 0", __func__);
	return 0;
}

int wlnpi_show_get_sta_lut_status(struct wlnpi_cmd_t *cmd,
				  unsigned char *r_buf,
				  int r_len)
{
	int len = 0, i, j, k;

	ENG_LOG("ADL entry %s(),id=%d, %s, r_len = %d",
		__func__, cmd->id, cmd->name, r_len);

	if ((1 > r_len) || (512 < r_len)) {
		printf("get_cp2_infor cmd_name:%s err\n", cmd->name);
		ENG_LOG("ADL leaving %s(), r_len is ERROR, return -1",
			__func__);
		return -1;
	}

	j = r_len/96;

	printf("sta lut status infor, current sta lut count = %d:\r\n", j);
	for (i = 1; i <= j; i++) {
		printf("sta lut %d:\r\n", i);
		for(k = 0; k < 24; k++) 	{
			printf("Word%d:%02x%02x%02x%02x\r\n", k,
			       r_buf[len+3], r_buf[len+2],
			       r_buf[len+1], r_buf[len]);
			len += 4;
		}
	}
	ENG_LOG("ADL leaving %s(), return 0", __func__);
	return 0;
}

struct wlnpi_cmd_t g_cmd_table[] =
{
	{
		/*-----CMD ID:0-----------*/
        .id    = WLNPI_CMD_START,
        .name  = "start",
        .help  = "start",
        .parse = wlnpi_cmd_start,
        .show  = wlnpi_show_only_status,
    },
	{
		/*-----CMD ID:1-----------*/
        .id    = WLNPI_CMD_STOP,
        .name  = "stop",
        .help  = "stop",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_only_status,
    },
    {
		/*-----CMD ID:2-----------*/
        .id    = WLNPI_CMD_SET_MAC,
        .name  = "set_mac",
        .help  = "set_mac <xx:xx:xx:xx:xx:xx>",
        .parse = wlnpi_cmd_set_mac,
        .show  = wlnpi_show_only_status,
    },
	{
		/*-----CMD ID:3-----------*/
        .id    = WLNPI_CMD_GET_MAC,
        .name  = "get_mac",
        .help  = "get_mac",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_get_mac,
    },
    {
        /*-----CMD ID:4-----------*/
        .id    = WLNPI_CMD_SET_MAC_FILTER,
        .name  = "set_macfilter",
        .help  = "set_macfilter <NUM>",
        .parse = wlnpi_cmd_set_macfilter,
        .show  = wlnpi_show_only_status,
    },
    {
        /*-----CMD ID:5-----------*/
        .id    = WLNPI_CMD_GET_MAC_FILTER,
        .name  = "get_macfilter",
        .help  = "get_macfilter",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_get_macfilter,
    },
	{
		/*-----CMD ID:6-----------*/
        .id    = WLNPI_CMD_SET_CHANNEL,
        .name  = "set_channel",
        .help  = "set_channel [primary20_CH][center_CH]",
        .parse = wlnpi_cmd_set_channel,
        .show  = wlnpi_show_only_status,
    },
    {
		/*-----CMD ID:7-----------*/
        .id    = WLNPI_CMD_GET_CHANNEL,
        .name  = "get_channel",
        .help  = "get_channel",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_get_channel,
    },
    {
		/*-----CMD ID:8-----------*/
        .id    = WLNPI_CMD_GET_RSSI,
        .name  = "get_rssi",
        .help  = "get_rssi",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_get_rssi,
    },
    {
        /*-----CMD ID:9-----------*/
        .id    = WLNPI_CMD_SET_TX_MODE,
        .name  = "set_tx_mode",
        .help  = "set_tx_mode [0:duty cycle|1:carrier suppressioon|2:local leakage]",
        .parse = wlnpi_cmd_set_tx_mode,
        .show  = wlnpi_show_only_status,
    },
    {
        /*-----CMD ID:10-----------*/
        .id    = WLNPI_CMD_GET_TX_MODE,
        .name  = "get_tx_mode",
        .help  = "get_tx_mode",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_get_tx_mode,
    },
    {
		/*-----CMD ID:11-----------*/
        .id    = WLNPI_CMD_SET_RATE,
        .name  = "set_rate",
        .help  = "set_rate <NUM>",
        .parse = wlnpi_cmd_set_rate,
        .show  = wlnpi_show_only_status,
    },
    {
		/*-----CMD ID:12-----------*/
        .id    = WLNPI_CMD_GET_RATE,
        .name  = "get_rate",
        .help  = "get_rate",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_get_rate,
    },
    {
        /*-----CMD ID:13-----------*/
        .id    = WLNPI_CMD_SET_BAND,
        .name  = "set_band",
        .help  = "set_band <NUM>",
        .parse = wlnpi_cmd_set_band,
        .show  = wlnpi_show_only_status,
    },
    {
        /*-----CMD ID:14-----------*/
        .id    = WLNPI_CMD_GET_BAND,
        .name  = "get_band",
        .help  = "get_band",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_get_band,
    },
    {
        /*-----CMD ID:15-----------*/
        .id    = WLNPI_CMD_SET_BW,
        .name  = "set_cbw",
        .help  = "set_cbw [0:20M|1:40M|2:80M|3:160M]",
        .parse = wlnpi_cmd_set_cbw,
        .show  = wlnpi_show_only_status,
    },
    {
        /*-----CMD ID:16-----------*/
        .id    = WLNPI_CMD_GET_BW,
        .name  = "get_cbw",
        .help  = "get_cbw",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_get_cbw,
    },
	{
        /*-----CMD ID:17-----------*/
        .id    = WLNPI_CMD_SET_PKTLEN,
        .name  = "set_pkt_len",
        .help  = "set_pkt_len <NUM>",
        .parse = wlnpi_cmd_set_pkt_length,
        .show  = wlnpi_show_only_status,
    },
    {
        /*-----CMD ID:18-----------*/
        .id    = WLNPI_CMD_GET_PKTLEN,
        .name  = "get_pkt_len",
        .help  = "get_pkt_len",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_get_pkt_length,
    },
    {
        /*-----CMD ID:19-----------*/
        .id    = WLNPI_CMD_SET_PREAMBLE,
        .name  = "set_preamble",
        .help  = "set_preamble [0:normal|1:cck|2:mixed|3:11n green|4:11ac]",
        .parse = wlnpi_cmd_set_preamble,
        .show  = wlnpi_show_only_status,
    },
    {
        /*-----CMD ID:20-----------*/
        .id    = WLNPI_CMD_SET_GUARD_INTERVAL,
        .name  = "set_gi",
        .help  = "set_gi [0:long|1:short]",
        .parse = wlnpi_cmd_set_guard_interval,
        .show  = wlnpi_show_only_status,
    },
    {
        /*-----CMD ID:21-----------*/
        .id    = WLNPI_CMD_GET_GUARD_INTERVAL,
        .name  = "get_gi",
        .help  = "get_gi",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_get_guard_interval,
    },
        /*-----CMD ID:22-----------*/
      /*.id    = WLNPI_CMD_SET_BURST_INTERVAL,*/
        /*-----CMD ID:23-----------*/
	/*  .id    = WLNPI_CMD_GET_BURST_INTERVAL,*/
    {
        /*-----CMD ID:24-----------*/
        .id    = WLNPI_CMD_SET_PAYLOAD,
        .name  = "set_payload",
        .help  = "set_payload <NUM>",
        .parse = wlnpi_cmd_set_payload,
        .show  = wlnpi_show_only_status,
    },
    {
        /*-----CMD ID:25-----------*/
        .id    = WLNPI_CMD_GET_PAYLOAD,
        .name  = "get_payload",
        .help  = "get_payload",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_get_payload,
    },
    {
		/*-----CMD ID:26-----------*/
        .id    = WLNPI_CMD_SET_TX_POWER,
        .name  = "set_tx_power",
        .help  = "set_tx_power <NUM>",
        .parse = wlnpi_cmd_set_tx_power,
        .show  = wlnpi_show_only_status,
    },
    {
		/*-----CMD ID:27-----------*/
        .id    = WLNPI_CMD_GET_TX_POWER,
        .name  = "get_tx_power",
        .help  = "get_tx_power",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_get_tx_power,
    },
    {
		/*-----CMD ID:28-----------*/
        .id    = WLNPI_CMD_SET_TX_COUNT,
        .name  = "set_tx_count",
        .help  = "set_tx_count <NUM>",
        .parse = wlnpi_cmd_set_tx_count,
        .show  = wlnpi_show_only_status,
    },
	{
		/*-----CMD ID:29-----------*/
        .id    = WLNPI_CMD_GET_RX_OK_COUNT,
        .name  = "get_rx_ok",
        .help  = "get_rx_ok",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_get_rx_ok,
    },
    {
		/*-----CMD ID:30-----------*/
        .id    = WLNPI_CMD_TX_START,
        .name  = "tx_start",
        .help  = "tx_start",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_only_status,
    },
	{
		/*-----CMD ID:31-----------*/
        .id    = WLNPI_CMD_TX_STOP,
        .name  = "tx_stop",
        .help  = "tx_stop",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_only_status,
    },
    {
		/*-----CMD ID:32-----------*/
        .id    = WLNPI_CMD_RX_START,
        .name  = "rx_start",
        .help  = "rx_start",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_only_status,
    },
    {
		/*-----CMD ID:33-----------*/
        .id    = WLNPI_CMD_RX_STOP,
        .name  = "rx_stop",
        .help  = "rx_stop",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_only_status,
    },
    {
		/*-----CMD ID:34-----------*/
        .id    = WLNPI_CMD_GET_REG,
        .name  = "get_reg",
        .help  = "get_reg <mac/phy0/phy1/rf> <addr_offset> [count]",
        .parse = wlnpi_cmd_get_reg,
        .show  = wlnpi_show_get_reg,
    },
    {
		/*-----CMD ID:35-----------*/
        .id    = WLNPI_CMD_SET_REG,
        .name  = "set_reg",
        .help  = "set_reg <mac/phy0/phy1/rf> <addr_offset> <value>",
        .parse = wlnpi_cmd_set_reg,
        .show  = wlnpi_show_only_status,
    },
    {
		/*-----CMD ID:36-----------*/
        .id    = WLNPI_CMD_SIN_WAVE,
        .name  = "sin_wave",
        .help  = "sin_wave",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_only_status,
    },
    {
		/*-----CMD ID:37-----------*/
        .id    = WLNPI_CMD_LNA_ON,
        .name  = "lna_on",
        .help  = "lna_on",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_only_status,
    },
	{
		/*-----CMD ID:38-----------*/
        .id    = WLNPI_CMD_LNA_OFF,
        .name  = "lna_off",
        .help  = "lna_off",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_only_status,
    },
    {
		/*-----CMD ID:39-----------*/
        .id    = WLNPI_CMD_GET_LNA_STATUS,
        .name  = "lna_status",
        .help  = "lna_status",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_get_lna_status,
    },
    {
		/*-----CMD ID:40-----------*/
        .id    = WLNPI_CMD_SET_WLAN_CAP,
        .name  = "set_wlan_cap",
        .help  = "set_wlan_cap <NUM>",
        .parse = wlnpi_cmd_set_wlan_cap,
        .show  = wlnpi_show_only_status,
    },
    {
		/*-----CMD ID:41-----------*/
        .id    = WLNPI_CMD_GET_WLAN_CAP,
        .name  = "get_wlan_cap",
        .help  = "get_wlan_cap",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_get_wlan_cap,
    },
    {
        /*-----CMD ID:42-----------*/
        .id    = WLNPI_CMD_GET_CONN_AP_INFO,
        .name  = "conn_status",
        .help  = "conn_status //get current connected ap's info",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_get_connected_ap_info,
    },
    {
        /*-----CMD ID:42-----------*/
        .id    = WLNPI_CMD_GET_CONN_AP_INFO,
        .name  = "get_reconnect",
        .help  = "get_reconnect //get reconnect times",
        .parse = wlnpi_cmd_get_reconnect,
        .show  = wlnpi_show_reconnect,
    },
    {
        /*-----CMD ID:43-----------*/
        .id    = WLNPI_CMD_GET_MCS_INDEX,
        .name  = "mcs",
        .help  = "mcs //get mcs index in use.",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_get_mcs_index,
    },
    {
         /*-----CMD ID:44-----------*/
        .id    = WLNPI_CMD_SET_AUTORATE_FLAG,
        .name  = "set_ar",
        .help  = "set_ar [flag:0|1] [index:NUM] [sgi:0|1] #set auto rate flag(on or off) and index.",
        .parse = wlnpi_cmd_set_ar,
        .show  = wlnpi_show_only_status,
    },
    {
         /*-----CMD ID:45-----------*/
        .id    = WLNPI_CMD_GET_AUTORATE_FLAG,
        .name  = "get_ar",
        .help  = "get_ar(get auto rate flag(on or off) and index)",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_get_ar,
    },
    {
         /*-----CMD ID:46-----------*/
        .id    = WLNPI_CMD_SET_AUTORATE_PKTCNT,
        .name  = "set_ar_pktcnt",
        .help  = "set_ar_pktcnt<NUM> //set auto rate pktcnt.",
        .parse = wlnpi_cmd_set_ar_pktcnt,
        .show  = wlnpi_show_only_status,
    },
    {
         /*-----CMD ID:47-----------*/
        .id    = WLNPI_CMD_GET_AUTORATE_PKTCNT,
        .name  = "get_ar_pktcnt",
        .help  = "get_ar_pktcnt//get auto rate pktcnt.",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_get_ar_pktcnt,
    },
    {
         /*-----CMD ID:48-----------*/
        .id    = WLNPI_CMD_SET_AUTORATE_RETCNT,
        .name  = "set_ar_retcnt",
        .help  = "set_ar_retcnt[NUM]//ar:auto rate",
        .parse = wlnpi_cmd_set_ar_retcnt,
        .show  = wlnpi_show_only_status,
    },
    {
         /*-----CMD ID:49-----------*/
        .id    = WLNPI_CMD_GET_AUTORATE_RETCNT,
        .name  = "get_ar_retcnt",
        .help  = "get_ar_pktcnt //get auto rate retcnt.",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_get_ar_retcnt,
    },
	 {
         /*-----CMD ID:50-----------*/
        .id    = WLNPI_CMD_ROAM,
        .name  = "roam",
        .help  = "roam <channel> <mac_addr> <ssid>",
        .parse = wlnpi_cmd_roam,
        .show  = wlnpi_show_only_status,
    },
    {
        /*-----CMD ID:51-----------*/
        .id    = WLNPI_CMD_WMM_PARAM,
        .name  = "set_wmm",
        .help  = "set_wmm [be|bk|vi|vo] {cwmin} {cwmax} {aifs} {txop}",
        .parse = wlnpi_cmd_set_wmm,
        .show  = wlnpi_show_only_status,
    },
    {
        /*-----CMD ID:52-----------*/
        .id    = WLNPI_CMD_ENG_MODE,
        .name  = "set_eng_mode",
        .help  = "set_eng_mode [1:set_cca|3:set_scan][0:off|1:on]",
        .parse = wlnpi_cmd_set_eng_mode,
        .show  = wlnpi_show_only_status,
    },
    {
        /*-----CMD ID:65-----------*/
        .id    = WLNPI_CMD_GET_DEV_SUPPORT_CHANNEL,
        .name  = "get_sup_ch",
        .help  = "get_sup_ch",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_only_status,
    },
	{
		/*-----CMD ID:66-----------*/
		.id    = WLNPI_CMD_GET_PA_INFO,
		.name  = "pa_info",
		.help  = "pa_info",
		.parse = wlnpi_cmd_no_argv,
		.show  = wlnpi_show_get_pa_infor,
	},
	{
		/*-----CMD ID:67-----------*/
		.id    = WLNPI_CMD_GET_TX_STATUS,
		.name  = "tx_status",
		.help  = "tx_status",
		.parse = wlnpi_cmd_no_argv,
		.show  = wlnpi_show_get_tx_status,
	},
	{
		/*-----CMD ID:68-----------*/
		.id    = WLNPI_CMD_GET_RX_STATUS,
		.name  = "rx_status",
		.help  = "rx_status",
		.parse = wlnpi_cmd_no_argv,
		.show  = wlnpi_show_get_rx_status,
	},
	{
		/*-----CMD ID:69-----------*/
		.id    = WLNPI_CMD_GET_STA_LUT_STATUS,
		.name  = "sta_lut_status",
		.help  = "sta_lut_status",
		.parse = wlnpi_cmd_no_argv,
		.show  = wlnpi_show_get_sta_lut_status,
	},
    {
        /*-----CMD ID:70-----------*/
        .id    = WLNPI_CMD_START_PKT_LOG,
        .name  = "start_pkt_log",
        .help  = "start_pkt_log [0-255] 0",
        .parse = wlnpi_cmd_start_pkt_log,
        .show  = wlnpi_show_only_status,
    },
    {
        /*-----CMD ID:71-----------*/
        .id    = WLNPI_CMD_STOP_PKT_LOG,
        .name  = "stop_pkt_log",
        .help  = "stop_pkt_log",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_only_status,
    },
	{
		/*-----CMD ID:78-----------*/
		.id    = WLNPI_CMD_SET_CHAIN,
		.name  = "set_chain",
		.help  = "set_chain [1:primary CH|2:Sec CH|3:MIMO]",
		.parse = wlnpi_cmd_set_chain,
		.show  = wlnpi_show_only_status,
	},
	{
		/*-----CMD ID:79-----------*/
		.id    = WLNPI_CMD_GET_CHAIN,
		.name  = "get_chain",
		.help  = "get_chain",
		.parse = wlnpi_cmd_no_argv,
		.show  = wlnpi_show_get_chain,
	},
	{
		/*-----CMD ID:80-----------*/
		.id    = WLNPI_CMD_SET_SBW,
		.name  = "set_sbw",
		.help  = "set_sbw [0:20M|1:40M|2:80M|3:160M]",
		.parse = wlnpi_cmd_set_sbw,
		.show  = wlnpi_show_only_status,
	},
	{
		/*-----CMD ID:81-----------*/
		.id    = WLNPI_CMD_GET_SBW,
		.name  = "get_sbw",
		.help  = "get_sbw",
		.parse = wlnpi_cmd_no_argv,
		.show  = wlnpi_show_get_sbw,
	},
	{
		/*-----CMD ID:82-----------*/
		.id    = WLNPI_CMD_SET_AMPDU_ENABLE,
		.name  = "set_ampdu_en",
		.help  = "set_ampdu_en",
		.parse = wlnpi_cmd_no_argv,
		.show  = wlnpi_show_only_status,
	},
	{
		/*-----CMD ID:83-----------*/
		.id    = WLNPI_CMD_GET_AMPDU_ENABLE,
		.name  = "get_ampdu_en",
		.help  = "get_ampdu_en",
		.parse = wlnpi_cmd_no_argv,
		.show  = wlnpi_show_only_status,
	},
	{
		/*-----CMD ID:84-----------*/
		.id    = WLNPI_CMD_SET_AMPDU_COUNT,
		.name  = "set_ampdu_cnt",
		.help  = "set_ampdu_cnt",
		.parse = wlnpi_cmd_no_argv,
		.show  = wlnpi_show_only_status,
	},
	{
		/*-----CMD ID:85-----------*/
		.id    = WLNPI_CMD_GET_AMPDU_COUNT,
		.name  = "get_ampdu_cnt",
		.help  = "get_ampdu_cnt",
		.parse = wlnpi_cmd_no_argv,
		.show  = wlnpi_show_only_status,
	},
	{
		/*-----CMD ID:86-----------*/
		.id    = WLNPI_CMD_SET_STBC,
		.name  = "set_stbc",
		.help  = "set_stbc",
		.parse = wlnpi_cmd_no_argv,
		.show  = wlnpi_show_only_status,
	},
	{
		/*-----CMD ID:87-----------*/
		.id    = WLNPI_CMD_GET_STBC,
		.name  = "get_stbc",
		.help  = "get_stbc",
		.parse = wlnpi_cmd_no_argv,
		.show  = wlnpi_show_only_status,
	},
	{
		/*-----CMD ID:88-----------*/
		.id    = WLNPI_CMD_SET_FEC,
		.name  = "set_fec",
		.help  = "set_fec [0:BCC|1:LDPC]",
		.parse = wlnpi_cmd_set_fec,
		.show  = wlnpi_show_only_status,
	},
	{
		/*-----CMD ID:89-----------*/
		.id    = WLNPI_CMD_GET_FEC,
		.name  = "get_fec",
		.help  = "get_fec",
		.parse = wlnpi_cmd_no_argv,
		.show  = wlnpi_show_get_fec,
	},
	{
		/*-----CMD ID:90-----------*/
		.id    = WLNPI_CMD_GET_RX_SNR,
		.name  = "get_rx_snr",
		.help  = "get_rx_snr",
		.parse = wlnpi_cmd_no_argv,
		.show  = wlnpi_show_only_status,
	},
	{
		/*-----CMD ID:91-----------*/
		.id    = WLNPI_CMD_SET_FORCE_TXGAIN,
		.name  = "set_force_txgain",
		.help  = "set_force_txgain",
		.parse = wlnpi_cmd_no_argv,
		.show  = wlnpi_show_only_status,
	},
	{
		/*-----CMD ID:92-----------*/
		.id    = WLNPI_CMD_SET_FORCE_RXGAIN,
		.name  = "set_force_rxgain",
		.help  = "set_force_rxgain",
		.parse = wlnpi_cmd_no_argv,
		.show  = wlnpi_show_only_status,
	},
	{
		/*-----CMD ID:93-----------*/
		.id    = WLNPI_CMD_SET_BFEE_ENABLE,
		.name  = "set_bfee_en",
		.help  = "set_bfee_en",
		.parse = wlnpi_cmd_no_argv,
		.show  = wlnpi_show_only_status,
	},
	{
		/*-----CMD ID:94-----------*/
		.id    = WLNPI_CMD_SET_DPD_ENABLE,
		.name  = "set_dpd_en",
		.help  = "set_dpd_en",
		.parse = wlnpi_cmd_no_argv,
		.show  = wlnpi_show_only_status,
	},
	{
		/*-----CMD ID:95-----------*/
		.id    = WLNPI_CMD_SET_AGC_LOG_ENABLE,
		.name  = "set_agc_log_en",
		.help  = "set_agc_log_en",
		.parse = wlnpi_cmd_no_argv,
		.show  = wlnpi_show_only_status,
	},
	{
		/*-----CMD ID:96-----------*/
		.id    = WLNPI_CMD_SET_EFUSE,
		.name  = "set_efuse",
		.help  = "set_efuse",
		.parse = wlnpi_cmd_no_argv,
		.show  = wlnpi_show_only_status,
	},
	{
		/*-----CMD ID:97-----------*/
		.id    = WLNPI_CMD_GET_EFUSE,
		.name  = "get_efuse",
		.help  = "get_efuse",
		.parse = wlnpi_cmd_no_argv,
		.show  = wlnpi_show_only_status,
	},
	{
		/*-----CMD ID:98-----------*/
		.id    = WLNPI_CMD_SET_TXS_CALIB_RESULT,
		.name  = "set_txs_cal_res",
		.help  = "set_txs_cal_res",
		.parse = wlnpi_cmd_no_argv,
		.show  = wlnpi_show_only_status,
	},
	{
		/*-----CMD ID:99-----------*/
		.id    = WLNPI_CMD_GET_TXS_TEMPERATURE,
		.name  = "get_txs_temp",
		.help  = "get_txs_temp",
		.parse = wlnpi_cmd_no_argv,
		.show  = wlnpi_show_only_status,
	},
	{
		/*-----CMD ID:100-----------*/
		.id    = WLNPI_CMD_SET_CBANK_REG,
		.name  = "set_cbank_reg",
		.help  = "set_cbank_reg",
		.parse = wlnpi_cmd_no_argv,
		.show  = wlnpi_show_only_status,
	},
	{
		/*-----CMD ID:101-----------*/
		.id    = WLNPI_CMD_SET_11N_GREEN_FIELD,
		.name  = "set_11n_green",
		.help  = "set_11n_green",
		.parse = wlnpi_cmd_no_argv,
		.show  = wlnpi_show_only_status,
	},
	{
		/*-----CMD ID:102-----------*/
		.id    = WLNPI_CMD_GET_11N_GREEN_FIELD,
		.name  = "get_11n_green",
		.help  = "get_11n_green",
		.parse = wlnpi_cmd_no_argv,
		.show  = wlnpi_show_only_status,
	},
	{
		/*-----CMD ID:103-----------*/
		.id    = WLNPI_CMD_ATENNA_COUPLING,
		.name  = "atenna_coup",
		.help  = "atenna_coup",
		.parse = wlnpi_cmd_no_argv,
		.show  = wlnpi_show_only_status,
	},
	{
		/*-----CMD ID:104-----------*/
		.id    = WLNPI_CMD_SET_FIX_TX_RF_GAIN,
		.name  = "set_tx_rf_gain",
		.help  = "set_tx_rf_gain",
		.parse = wlnpi_cmd_no_argv,
		.show  = wlnpi_show_only_status,
	},
	{
		/*-----CMD ID:105-----------*/
		.id    = WLNPI_CMD_SET_FIX_TX_PA_BIAS,
		.name  = "set_tx_pa_bias",
		.help  = "set_tx_pa_bias",
		.parse = wlnpi_cmd_no_argv,
		.show  = wlnpi_show_only_status,
	},
	{
		/*-----CMD ID:106-----------*/
		.id    = WLNPI_CMD_SET_FIX_TX_DVGA_GAIN,
		.name  = "set_tx_dvga_gain",
		.help  = "set_tx_dvga_gain",
		.parse = wlnpi_cmd_no_argv,
		.show  = wlnpi_show_only_status,
	},
	{
		/*-----CMD ID:107-----------*/
		.id    = WLNPI_CMD_SET_FIX_RX_LNA_GAIN,
		.name  = "set_rx_lna_gain",
		.help  = "set_rx_lna_gain",
		.parse = wlnpi_cmd_no_argv,
		.show  = wlnpi_show_only_status,
	},
	{
		/*-----CMD ID:108-----------*/
		.id    = WLNPI_CMD_SET_FIX_RX_VGA_GAIN,
		.name  = "set_rx_vga_gain",
		.help  = "set_rx_vga_gain",
		.parse = wlnpi_cmd_no_argv,
		.show  = wlnpi_show_only_status,
	},
	{
		/*-----CMD ID:109-----------*/
		.id    = WLNPI_CMD_SET_MAC_BSSID,
		.name  = "set_mac_bssid",
		.help  = "set_mac_bssid",
		.parse = wlnpi_cmd_no_argv,
		.show  = wlnpi_show_only_status,
	},
	{
		/*-----CMD ID:110-----------*/
		.id    = WLNPI_CMD_GET_MAC_BSSID,
		.name  = "get_mac_bssid",
		.help  = "get_mac_bssid",
		.parse = wlnpi_cmd_no_argv,
		.show  = wlnpi_show_only_status,
	},
	{
		/*-----CMD ID:111-----------*/
		.id    = WLNPI_CMD_FORCE_TX_RATE,
		.name  = "force_tx_rate",
		.help  = "force_tx_rate",
		.parse = wlnpi_cmd_no_argv,
		.show  = wlnpi_show_only_status,
	},
	{
		/*-----CMD ID:112-----------*/
		.id    = WLNPI_CMD_FORCE_TX_POWER,
		.name  = "force_tx_power",
		.help  = "force_tx_power",
		.parse = wlnpi_cmd_no_argv,
		.show  = wlnpi_show_only_status,
	},
	{
		/*-----CMD ID:113-----------*/
		.id    = WLNPI_CMD_ENABLE_FW_LOOP_BACK,
		.name  = "en_fw_loop",
		.help  = "en_fw_loop",
		.parse = wlnpi_cmd_no_argv,
		.show  = wlnpi_show_only_status,
	},
	{
		/*-----CMD ID:115-----------*/
		.id    = WLNPI_CMD_SET_PROTECTION_MODE,
		.name  = "set_prot_mode",
		.help  = "set_prot_mode [mode:1:sta|2:ap|4:p2p_dev|5:p2p_cli|6:p2p_go|7:ibss] [value]",
		.parse = wlnpi_cmd_set_prot_mode,
		.show  = wlnpi_show_only_status,
	},
	{
		/*-----CMD ID:116-----------*/
		.id    = WLNPI_CMD_GET_PROTECTION_MODE,
		.name  = "get_prot_mode",
		.help  = "get_prot_mode [mode:1:sta|2:ap|4:p2p_dev|5:p2p_cli|6:p2p_go|7:ibss]",
		.parse = wlnpi_cmd_get_prot_mode,
		.show  = wlnpi_show_get_prot_mode,
	},
	{
		/*-----CMD ID:117-----------*/
		.id    = WLNPI_CMD_SET_RTS_THRESHOLD,
		.name  = "set_threshold",
		.help  = "set_threshold [mode:1:sta|2:ap|4:p2p_dev|5:p2p_cli|6:p2p_go|7:ibss] [rts:] [flag:]",
		.parse = wlnpi_cmd_set_threshold,
		.show  = wlnpi_show_only_status,
	},
	{
		/*-----CMD ID:118-----------*/
		.id    = WLNPI_CMD_SET_AUTORATE_DEBUG,
		.name  = "set_ar_dbg",
		.help  = "set_ar_dbg",
		.parse = wlnpi_cmd_no_argv,
		.show  = wlnpi_show_only_status,
	},
	{
		/*-----CMD ID:119-----------*/
		.id    = WLNPI_CMD_SET_PM_CTL,
		.name  = "set_pm_ctl",
		.help  = "set_pm_ctl [int32]",
		.parse = wlnpi_cmd_set_pm_ctl,
		.show  = wlnpi_show_only_status,
	},
	{
		/*-----CMD ID:120-----------*/
		.id    = WLNPI_CMD_GET_PM_CTL,
		.name  = "get_pm_ctl",
		.help  = "get_pm_ctl",
		.parse = wlnpi_cmd_no_argv,
		.show  = wlnpi_show_get_pm_ctl,
	},
};

struct wlnpi_cmd_t *match_cmd_table(char *name)
{
    size_t          i;
    struct wlnpi_cmd_t *cmd = NULL;
    for(i=0; i< sizeof(g_cmd_table)/sizeof(struct wlnpi_cmd_t) ; i++)
    {
        if( 0 != strcmp(name, g_cmd_table[i].name) )
        {
            continue;
        }

        cmd = &g_cmd_table[i];
        break;
    }
    return cmd;
}

void do_help(void)
{
    int i, max;
    max = sizeof(g_cmd_table)/sizeof(struct wlnpi_cmd_t);
    for(i=0; i<max; i++)
    {
        printf("iwnpi wlan0 %s\n", g_cmd_table[i].help);
    }
    return;
}

/********************************************************************
*   name   iwnpi_hex_dump
*   ---------------------------
*   description:
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int iwnpi_hex_dump(unsigned char *name, unsigned short nLen, unsigned char *pData, unsigned short len)
{
    char *str;
    int i, p, ret;

    ENG_LOG("ADL entry %s(), len = %d", __func__, len);

    if (len > 1024)
        len = 1024;
    str = malloc(((len + 1) * 3 + nLen));
    memset(str, 0, (len + 1) * 3 + nLen);
    memcpy(str, name, nLen);
    if ((NULL == pData) || (0 == len))
    {
        printf("%s\n", str);
        free(str);
        return 0;
    }

    p = 0;
    for (i = 0; i < len; i++)
    {
        ret = sprintf((str + nLen + p), "%02x ", *(pData + i));
        p = p + ret;
    }
    ENG_LOG("%s\n\n", str);
    free(str);

    ENG_LOG("ADL levaling %s()", __func__);
    return 0;
}
