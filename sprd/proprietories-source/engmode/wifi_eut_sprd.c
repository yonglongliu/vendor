#include <stdlib.h>
#include <stdio.h>
#include "eut_opt.h"
#include <fcntl.h>
#include "engopt.h"

#include "wifi_eut_sprd.h"
#include "cutils/properties.h"

#define NUM_ELEMS(x) (sizeof(x) / sizeof(x[0]))
#define wifi_eut_debug

#ifdef wifi_eut_debug

#define rsp_debug(rsp_)                               \
  do {                                                \
    ENG_LOG("%s(), rsp###:%s\n", __FUNCTION__, rsp_); \
  } while (0);

#define wifi_status_dump()                                             \
  do {                                                                 \
    ENG_LOG("%s(), eut_enter = %d, tx_start = %d, rx_start = %d\n",    \
            __FUNCTION__, g_wifi_data.eut_enter, g_wifi_data.tx_start, \
            g_wifi_data.rx_start);                                     \
  } while (0);

#else

#define rsp_debug(rsp_)

#endif

static unsigned int g_wifi_tx_count = 0;

static WIFI_ELEMENT g_wifi_data;

/********************************************************************
*   name   get_wcn_hw_type
*   ---------------------------
*   description: get wcn hardware type
*   ----------------------------
*   para        IN/OUT      type            note
*   none
*   ----------------------------------------------------
*   return
*   a enum type digit portray wcn hardware chip
*   ------------------
*   other:
********************************************************************/
static int get_wcn_hw_type(void) {
  char buf[PROPERTY_VALUE_MAX] = {0};

  if (g_wifi_data.wcn_type != WCN_HW_INVALID)
    goto ok;
  property_get(WCN_HARDWARE_PRODUCT, buf, "invalid");
  if (0 == strcmp(buf, "marlin")) {
    g_wifi_data.wcn_type = WCN_HW_MARLIN;
  } else if (0 == strcmp(buf, "marlin2")) {
    g_wifi_data.wcn_type = WCN_HW_MARLIN2;
  } else if (0 == strcmp(buf, "marlin3")) {
    g_wifi_data.wcn_type = WCN_HW_MARLIN3;
  } else if (0 == strcmp(buf, "rs2351")) {
    g_wifi_data.wcn_type = WCN_HW_RS2351;
  } else {
    g_wifi_data.wcn_type = WCN_HW_INVALID;
  }

ok:
  ENG_LOG("ADL %s(), wcn hardware type is [%s], id = %d.\n", __func__, buf, g_wifi_data.wcn_type);
  return g_wifi_data.wcn_type;
}

/********************************************************************
*   name   get_android_version
*   ---------------------------
*   description: get android version
*   ----------------------------
*   para        IN/OUT      type            note
*   none
*   ----------------------------------------------------
*   return
*   a integer number: 8 stand for AndroidO(8.0)
*                     else stand for Other Version(< 8.0)
*   ------------------
*   other:
********************************************************************/
static ANDROID_VERSION get_android_version(void) {
  char buf[PROPERTY_VALUE_MAX] = {0};

if (g_wifi_data.and_ver != ANDROID_VER_INVALID)
    goto ok;
  property_get(ANDROID_VER_PROP, buf, "invalid");
  if (NULL != strstr(buf, "8.0")) {
    g_wifi_data.and_ver = ANDROID_VER_8;
  } else {
    g_wifi_data.and_ver = ANDROID_VER_7;
  }

ok:
  ENG_LOG("ADL %s(), android Version is [%s], and ver id = %d.\n", __func__, buf, g_wifi_data.and_ver);
  return g_wifi_data.and_ver;
}

static WIFI_CHANNEL g_wifi_channel_table[] = {
    /* 2.4G 20M */
    {"1-2412MHz(bgn)", 1, 1}, {"2-2417MHz(bgn)", 2, 2}, {"3-2422MHz(bgn)", 3, 3}, {"4-2427MHz(bgn)", 4, 4},
    {"5-2432MHz(bgn)", 5, 5}, {"6-2437MHz(bgn)", 6, 6}, {"7-2442MHz(bgn)", 7, 7}, {"8-2447MHz(bgn)", 8, 8},
    {"9-2452MHz(bgn)", 9, 9}, {"10-2457MHz(bgn)", 10, 10}, {"11-2462MHz(bgn)", 11, 11}, {"12-2467MHz(bgn)", 12, 12},
    {"13-2472MHz(bgn)", 13, 13}, {"14-2484MHz(bgn)", 14, 14},
    /* 2.4G 40M */
    {"1-2422MHz(n40H)", 1, 3}, {"2-2427MHz(n40H)", 2, 4}, {"3-2432MHz(n40H)", 3, 5}, {"4-2437MHz(n40H)", 4, 6},
    {"5-2442MHz(n40H)", 5, 7}, {"6-2447MHz(n40H)", 6, 8}, {"7-2452MHz(n40H)", 7, 9}, {"8-2457MHz(n40H)", 8, 10},
    {"9-2462MHz(n40H)", 9, 11}, {"5-2422MHz(n40L)", 5, 3}, {"6-2427MHz(n40L)", 6, 4}, {"7-2432MHz(n40L)", 7, 5},
    {"8-2437MHz(n40L)", 8, 6},  {"9-2442MHz(n40L)", 9, 7}, {"10-2447MHz(n40L)", 10, 8}, {"11-2452MHz(n40L)", 11, 9},
    {"12-2457MHz(n40L)", 12, 10}, {"13-2462MHz(n40L)", 13, 11},
    /* 5G 20M */
    {"36-5180MHz(a/n/ac)", 36, 36}, {"40-5200MHz(a/n/ac)", 40, 40}, {"44-5220MHz(a/n/ac)", 44, 44},
    {"48-5240MHz(a/n/ac)", 48, 48}, {"52-5260MHz(a/n/ac)", 52, 52}, {"56-5280MHz(a/n/ac)", 56, 56},
    {"60-5300MHz(a/n/ac)", 60, 60}, {"64-5320MHz(a/n/ac)", 64, 64}, {"100-5500MHz(a/n/ac)", 100, 100},
    {"104-5520MHz(a/n/ac)", 104, 104}, {"108-5540MHz(a/n/ac)", 108, 108}, {"112-5560MHz(a/n/ac)", 112, 112},
    {"116-5580MHz(a/n/ac)", 116, 116}, {"120-5600MHz(a/n/ac)", 120, 120}, {"124-5620MHz(a/n/ac)", 124, 124},
    {"128-5640MHz(a/n/ac)", 128, 128}, {"132-5660MHz(a/n/ac)", 132, 132}, {"136-5680MHz(a/n/ac)", 136, 136},
    {"140-5700MHz(a/n/ac)", 140, 140}, {"144-5720MHz(a/n/ac)", 144, 144}, {"149-5745MHz(a/n/ac)", 149, 149},
    {"153-5765MHz(a/n/ac)", 153, 153}, {"157-5785MHz(a/n/ac)", 157, 157}, {"161-5805MHz(a/n/ac)", 161, 161},
    {"165-5825MHz(a/n/ac)", 165, 165},
    /* 5G 40M */
    {"36-5190MHz(n/ac40H)", 36, 38}, {"40-5190MHz(n/ac40L)", 40, 38}, {"44-5230MHz(n/ac40H)", 44, 46},
    {"48-5230MHz(n/ac40L)", 48, 46}, {"52-5270MHz(n/ac40H)", 52, 54}, {"56-5270MHz(n/ac40L)", 56, 54},
    {"60-5310MHz(n/ac40H)", 60, 62}, {"64-5310MHz(n/ac40L)", 64, 62}, {"100-5510MHz(n/ac40H)", 100, 102},
    {"104-5510MHz(n/ac40L)", 104, 102}, {"108-5550MHz(n/ac40H)", 108, 110}, {"112-5550MHz(n/ac40L)", 112, 110},
    {"116-5590MHz(n/ac40H)", 116, 118}, {"120-5590MHz(n/ac40L)", 120, 118}, {"124-5630MHz(n/ac40H)", 124, 126},
    {"128-5630MHz(n/ac40L)", 128, 126}, {"132-5670MHz(n/ac40H)", 132, 134}, {"136-5670MHz(n/ac40L)", 136, 134},
    {"140-5710MHz(n/ac40H)", 140, 142}, {"144-5710MHz(n/ac40L)", 144, 142}, {"149-5755MHz(n/ac40H)", 149, 151},
    {"153-5755MHz(n/ac40L)", 153, 151}, {"157-5795MHz(n/ac40H)", 157, 159}, {"161-5795MHz(n/ac40L)", 161, 159},
    /* 5G 80M */
    {"36-5210MHz(ac80H)", 36, 42}, {"40-5210MHz(ac80H)", 40, 42}, {"44-5210MHz(ac80L)", 44, 42}, {"48-5210MHz(ac80L)", 48, 42},
    {"52-5290MHz(ac80H)", 52, 58}, {"56-5290MHz(ac80H)", 56, 58}, {"60-5290MHz(ac80L)", 60, 58}, {"64-5290MHz(ac80L)", 64, 58},
    {"100-5530MHz(ac80H)", 100, 106}, {"104-5530MHz(ac80H)", 104, 106}, {"108-5530MHz(ac80L)", 108, 106}, {"112-5530MHz(ac80L)", 112, 106},
    {"116-5610MHz(ac80H)", 116, 122}, {"120-5610MHz(ac80H)", 120, 122}, {"124-5610MHz(ac80L)", 124, 122}, {"128-5610MHz(ac80L)", 128, 122},
    {"132-5690MHz(ac80H)", 132, 138}, {"136-5690MHz(ac80H)", 136, 138}, {"140-5690MHz(ac80L)", 140, 138}, {"144-5690MHz(ac80L)", 144, 138},
    {"149-5775MHz(ac80H)", 149, 155}, {"153-5775MHz(ac80H)", 153, 155}, {"157-5775MHz(ac80L)", 157, 155}, {"161-5775MHz(ac80L)", 161, 155}
};

static WIFI_CHANNEL *match_channel_table_by_name(const char *name) {
  int i;
  WIFI_CHANNEL *ret = NULL;

  for (i = 0; i < (int)NUM_ELEMS(g_wifi_channel_table); i++) {
    if ((strlen(name) - 1 == strlen(g_wifi_channel_table[i].name)) && (NULL != strstr(name, g_wifi_channel_table[i].name))) {
      ret = &g_wifi_channel_table[i];
      break;
    }
  }
  return ret;
}

static WIFI_CHANNEL *match_channel_table_by_chn(const int primary_channel, const int center_channel) {
  int i;
  WIFI_CHANNEL *ret = NULL;

  for (i = 0; i < (int)NUM_ELEMS(g_wifi_channel_table); i++) {
    if (center_channel == g_wifi_channel_table[i].center_channel && primary_channel == g_wifi_channel_table[i].primary_channel) {
      ret = &g_wifi_channel_table[i];
      break;
    }
  }
  return ret;
}

// older rate table for marlin2, it will replace by array g_wifi_rate_table as marlin3
static WIFI_RATE g_wifi_rate_table_old[] = {
    {1, "DSSS-1"},   {2, "DSSS-2"},   {5, "CCK-5.6"},  {11, "CCK-11"},
    {6, "OFDM-6"},   {9, "OFDM-9"},   {12, "OFDM-12"}, {18, "OFDM-18"},
    {24, "OFDM-24"}, {36, "OFDM-36"}, {48, "OFDM-48"}, {54, "OFDM-54"},
    {7, "MCS-0"},    {13, "MCS-1"},   {19, "MCS-2"},   {26, "MCS-3"},
    {39, "MCS-4"},   {52, "MCS-5"},   {58, "MCS-6"},   {65, "MCS-7"},
    {-1, "null"}
};

// new rate table for marlin3, it will be used for marlin2
static WIFI_RATE g_wifi_rate_table[] = {
    {0, "DSSS-1"}, /*1M_Long*/  {1, "DSSS-2"}, /*2M_Long*/ {2, "DSSS-2S"}, /*2M_Short*/ {3, "CCK-5.5"}, /*5.5M_Long*/ {4, "CCK-5.5S"}, /*5.5M_Short*/
    {5, "CCK-11"}, /*11M_Long*/  {6, "CCK-11S"}, /*11M_Short*/  {7, "OFDM-6"}, /*6M*/ {8, "OFDM-9"}, /*9M*/ {9, "OFDM-12"}, /*12M*/
    {10, "OFDM-18"}, /*18M*/ {11, "OFDM-24"}, /*24M*/ {12, "OFDM-36"}, /*36M*/ {13, "OFDM-48"}, /*48M*/ {14, "OFDM-54"}, /*54M*/
    {15, "MCS-0"}, /*HT_MCS0*/ {16, "MCS-1"}, /*HT_MCS1*/ {17, "MCS-2"}, /*HT_MCS2*/ {18, "MCS-3"},  /*HT_MCS3*/ {19, "MCS-4"},  /*HT_MCS4*/
    {20, "MCS-5"},  /*HT_MCS5*/ {21, "MCS-6"},  /*HT_MCS6*/ {22, "MCS-7"}, /*HT_MCS7*/ {23, "MCS-8"}, /*HT_MCS8*/{24, "MCS-9"}, /*HT_MCS9*/
    {25, "MCS-10"}, /*HT_MCS10*/ {26, "MCS-11"}, /*HT_MCS11*/ {27, "MCS-12"}, /*HT_MCS12*/ {28, "MCS-13"}, /*HT_MCS13*/ {29, "MCS-14"}, /*HT_MCS14*/
    {30, "MCS-15"}, /*HT_MCS15*/ {31, "VHT_MCS0_1SS"}, /*VHT_MCS0_1SS*/ {32, "VHT_MCS1_1SS"}, /*VHT_MCS1_1SS*/ {33, "VHT_MCS2_1SS"}, /*VHT_MCS2_1SS*/ {34, "VHT_MCS3_1SS"}, /*VHT_MCS3_1SS*/
    {35, "VHT_MCS4_1SS"}, /*VHT_MCS4_1SS*/ {36, "VHT_MCS5_1SS"}, /*VHT_MCS5_1SS*/ {37, "VHT_MCS6_1SS"}, /*VHT_MCS6_1SS*/ {38, "VHT_MCS7_1SS"}, /*VHT_MCS7_1SS*/ {39, "VHT_MCS8_1SS"}, /*VHT_MCS8_1SS*/
    {40, "VHT_MCS9_1SS"}, /*VHT_MCS9_1SS*/ {41, "VHT_MCS0_2SS"}, /*VHT_MCS0_2SS*/ {42, "VHT_MCS1_2SS"}, /*VHT_MCS1_2SS*/ {43, "VHT_MCS2_2SS"}, /*VHT_MCS2_2SS*/ {44, "VHT_MCS3_2SS"}, /*VHT_MCS3_2SS*/
    {45, "VHT_MCS4_2SS"}, /*VHT_MCS4_2SS*/ {46, "VHT_MCS5_2SS"}, /*VHT_MCS5_2SS*/ {47, "VHT_MCS6_2SS"}, /*VHT_MCS6_2SS*/ {48, "VHT_MCS7_2SS"}, /*VHT_MCS7_2SS*/ {49, "VHT_MCS8_2SS"}, /*VHT_MCS8_2SS*/
    {50, "VHT_MCS9_2SS"}, /*VHT_MCS9_2SS*/ {-1, "null"}
};

static int mattch_rate_table_str(const char *name) {
  WIFI_RATE *rate_ptr = NULL;

  if (WCN_HW_MARLIN3 == get_wcn_hw_type()) {   //if wcn chip is marlin3
    rate_ptr = g_wifi_rate_table;
  } else {
    rate_ptr = g_wifi_rate_table_old;
  }

  while (rate_ptr->rate != -1) {
    if (strlen(name) - 1 == strlen(rate_ptr->name) && NULL != strstr(name, rate_ptr->name)) {
      return rate_ptr->rate;
    }
    rate_ptr++;
  }
  return -1;
}

static char *mattch_rate_table_index(const int rate) {
  WIFI_RATE *rate_ptr = NULL;

  if (WCN_HW_MARLIN3 == get_wcn_hw_type()) {   //if wcn chip is marlin3
    rate_ptr = g_wifi_rate_table;
  } else {
    rate_ptr = g_wifi_rate_table_old;
  }

  while (rate_ptr->rate != -1) {
    if (rate == rate_ptr->rate) {
      return rate_ptr->name;
    }
    rate_ptr++;
  }
  return NULL;
}

/********************************************************************
*   name   get_iwnpi_ret_line
*   ---------------------------
*   description: get string result from iwnpi when after Chip's command
*   ----------------------------
*   para        IN/OUT      type            note
*   outstr      OUT         const char *    store string remove "ret: " and ":end"
*   out_len     IN          const int       the max length of out_str
*   flag        INT         const char *    find string included this flag,
*                                           if flag = null, only get one data
*   ----------------------------------------------------
*   return
*   if flag=null, return one digit from ret: %d:end
*   if flag!=null, return value must > 0, stand for the length of string saved in *out_str
*   ------------------
*   other:
********************************************************************/
static int get_iwnpi_ret_line(const char *flag, const char *out_str, const int out_len) {
  FILE *fp = NULL;
  char *str1 = NULL;
  char *str2 = NULL;
  int ret = WIFI_INT_INVALID_RET;
  unsigned long len = 0;
  char buf[TMP_BUF_SIZE] = {0};

  if (NULL != flag && NULL == out_str) {
    ENG_LOG("%s(), flag(%s) out str is NULL, return -100.\n", __FUNCTION__, flag);
    return WIFI_INT_INVALID_RET;
  }

  ENG_LOG("ADL entry %s(), flag = %s.", __func__, flag);
  if (NULL == (fp = fopen(TMP_FILE, "r+"))) {
    ENG_LOG("no %s\n", TMP_FILE);
    return WIFI_INT_INVALID_RET;
  }

  while (!feof(fp)) {
    fgets(buf, TMP_BUF_SIZE, fp);
    ENG_LOG("ADL %s(), buf = %s", __func__, buf);

    if (NULL == flag) {   //only for return one data, format is "ret: %d:end"
      if (sscanf(buf, IWNPI_RET_ONE_DATA, &ret) > 0)
        break;
    } else if (NULL != flag && NULL != strstr(buf, flag)) {
      str1 = strstr(buf, IWNPI_RET_BEGIN);
      str2 = strstr(buf, IWNPI_RET_END);
      if ((NULL != str1) && (NULL != str2)) {
        len = (unsigned long)str2 - (unsigned long)str1 - strlen(IWNPI_RET_BEGIN);
        if (len > 0) {
          len = out_len > len ? len : out_len;
          memset(out_str, 0x00, out_len);
          memcpy(out_str, str1 + strlen(IWNPI_RET_BEGIN), len);
          ret = len;
          break;
        }
      }
    }
    memset(buf, 0x00, TMP_BUF_SIZE);
  }
  fclose(fp);

  ENG_LOG("ADL leaving %s(), out_str = %s, ret = %d", __func__, out_str, ret);
  return ret;
}

/********************************************************************
*   name   get_iwnpi_exec_status
*   ---------------------------
*   description: get iwnpi status
*   ----------------------------
*   para        IN/OUT      type            note
*   void
*   ----------------------------------------------------
*   return
*   0: successfully
*   !0: failed
*   ------------------
*   other:
********************************************************************/
static int get_iwnpi_exec_status(void) {
  char buf[TMP_BUF_SIZE] = {0};
  int sta = -1;

  if (get_iwnpi_ret_line(IWNPI_RET_STA_FLAG, buf, TMP_BUF_SIZE) > 0)
    sscanf(buf, "status %d", &sta);
  ENG_LOG("ADL leaving %s(), get status = %d", __func__, sta);
  return sta;
}

/********************************************************************
*   name   get_result_from_iwnpi
*   ---------------------------
*   description: get int type result of Chip's command
*   ----------------------------
*   para        IN/OUT      type            note
*
*   ----------------------------------------------------
*   return
*   -100:error
*   other:return value
*   ------------------
*   other:
********************************************************************/
static inline int get_result_from_iwnpi(void) {
  return get_iwnpi_ret_line(NULL, NULL, 0);
}

static inline int wifi_enter_eut_mode(void) {
  return g_wifi_data.eut_enter;
}

/********************************************************************
*   name   exec_cmd
*   --------------------------------------------------
*   description: send cmd to iwnpi
*   --------------------------------------------------
*   para        IN/OUT      type            note
*   cmd         IN          const char *    execute cmd
*   ----------------------------------------------------
*   return
*   0: successfully
*   other: failed
*   ---------------------------------------------------
*   other:
********************************************************************/
static inline int exec_cmd(const char *cmd) {
  int ret = system(cmd);
  ENG_LOG("%s() [%s] %s!!\n", __func__, cmd, (ret == 0 ? "successfully" : strerror(ret)));
  return ret;
}

/********************************************************************
*   name   exec_iwnpi_cmd_for_status
*   --------------------------------------------------
*   description: send cmd to iwnpi, and return status
*   --------------------------------------------------
*   para        IN/OUT      type            note
*   cmd         IN          const char *    execute cmd
*   ----------------------------------------------------
*   return
*   0: successfully
*   other: failed
*   ---------------------------------------------------
*   other:
********************************************************************/
static inline int exec_iwnpi_cmd_for_status(const char *cmd) {
  int ret = -1;
  return ((ret = exec_cmd(cmd)) == 0 ? get_iwnpi_exec_status() : ret);
}

static inline int return_ok(const char *rsp) {
  strcpy(rsp, EUT_WIFI_OK);
  rsp_debug(rsp);
  return 0;
}

static inline int return_err(const char *rsp) {
  sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
  rsp_debug(rsp);
  return -1;
}

static int start_wifieut(char *rsp) {
  int ret;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN] = {0};

  ENG_LOG("ADL entry %s(), line = %d", __func__, __LINE__);

  if (wifi_enter_eut_mode() == 1)
    goto ok;

#ifdef SPRD_WCN_MARLIN
  ret = exec_cmd("connmgr_cli wcn poweron");
#endif
  ret = exec_cmd("rmmod sprdwl_ng.ko");

  sprintf(cmd, "insmod %s", WIFI_DRIVER_MODULE_PATH);
  if (0 != exec_cmd(cmd))
    goto err;

#if 0 /*must be close cose in NON-SIG mode */
    system("ifconfig wlan0 up");
#endif

  sprintf(cmd, "iwnpi wlan0 start > %s", TMP_FILE);
  if (0 != exec_iwnpi_cmd_for_status(cmd))
    goto err;

ok:
  memset(&g_wifi_data, 0x00, sizeof(WIFI_ELEMENT));
  g_wifi_data.eut_enter = 1;
  /* call init(), must be eut_enter is 1 */
  wifi_eut_init();
  return return_ok(rsp);

err:
  g_wifi_data.eut_enter = 0;
  return return_err(rsp);
}

static int stop_wifieut(char *rsp) {
  int ret;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN] = {0};

  ENG_LOG("ADL entry %s(), line = %d", __func__, __LINE__);

  wifi_status_dump();
  if (1 == g_wifi_data.tx_start) {
    sprintf(cmd, "iwnpi wlan0 tx_stop > %s", TMP_FILE);
    if (0 != exec_cmd(cmd))
      goto err;
  }

  if (1 == g_wifi_data.rx_start) {
    sprintf(cmd, "iwnpi wlan0 rx_stop > %s", TMP_FILE);
    if (0 != exec_cmd(cmd))
      goto err;
  }

  sprintf(cmd, "iwnpi wlan0 stop > %s", TMP_FILE);
  if (0 != exec_iwnpi_cmd_for_status(cmd))
    goto err;

#if 0
    exec_cmd("ifconfig wlan0 down");
#endif
  exec_cmd("rmmod sprdwl_ng.ko");
#ifdef SPRD_WCN_MARLIN
  ret = exec_cmd("connmgr_cli wcn poweroff ");
#endif
  memset(&g_wifi_data, 0, sizeof(WIFI_ELEMENT));
  return return_ok(rsp);

err:
  return return_err(rsp);
}

int wifi_eut_set(int cmd, char *rsp) {
  ENG_LOG("entry %s(), cmd = %d", __func__, cmd);
  if (cmd == 1) {
    start_wifieut(rsp);
  } else if (cmd == 0) {
    stop_wifieut(rsp);
  } else {
    ENG_LOG("%s(), cmd don't support", __func__);
    sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
  }
  return 0;
}

int wifi_eut_get(char *rsp) {
  sprintf(rsp, "%s%d", EUT_WIFI_REQ, g_wifi_data.eut_enter);
  rsp_debug(rsp);
  return 0;
}

int wifi_rate_set(char *string, char *rsp) {
  int ret = -1;
  int rate = -1;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN] = {0};

  if (wifi_enter_eut_mode() == 0)
    goto err;
  rate = mattch_rate_table_str(string);
  ENG_LOG("%s(), %s rate:%d", __FUNCTION__, string, rate);
  if (-1 == rate) goto err;

  sprintf(cmd, "iwnpi wlan0 set_rate %d > %s", rate, TMP_FILE);
  if (0 != exec_iwnpi_cmd_for_status(cmd))
    goto err;

  g_wifi_data.rate = rate;
  return return_ok(rsp);

err:
  return return_err(rsp);
}

int wifi_rate_get(char *rsp) {
  int ret = WIFI_INT_INVALID_RET;
  char *str = NULL;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN] = {0};

  ENG_LOG("%s()...\n", __FUNCTION__);
  if (wifi_enter_eut_mode() == 0)
    goto err;

  sprintf(cmd, "iwnpi wlan0 get_rate > %s", TMP_FILE);
  if (0 != exec_cmd(cmd))
    goto err;

  ret = get_result_from_iwnpi();
  if (WIFI_INT_INVALID_RET == ret) {
    ENG_LOG("iwnpi wlan0 get_rate err_code:%d", ret);
    goto err;
  }
  g_wifi_data.rate = ret;
  str = mattch_rate_table_index(g_wifi_data.rate);
  if (NULL == str) {
    ENG_LOG("%s(), don't mattch rate", __FUNCTION__);
    goto err;
  }

  sprintf(rsp, "%s%s", WIFI_RATE_REQ_RET, str);
  rsp_debug(rsp);
  return 0;

err:
  sprintf(rsp, "%s%s", WIFI_RATE_REQ_RET, "null");
  rsp_debug(rsp);
  return -1;
}

int wifi_channel_set_marlin3(char *ch_note, char *rsp) {
  int ret = -1;
  WIFI_CHANNEL *chn = NULL;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN] = {0x00};

  if (wifi_enter_eut_mode() == 0)
    goto err;

  chn = match_channel_table_by_name(ch_note);
  if (chn == NULL)
    goto err;

  ENG_LOG("%s(), %s: prim_chn = %d, cen_chn = %d.", __FUNCTION__, ch_note, chn->primary_channel, chn->center_channel);

  sprintf(cmd, "iwnpi wlan0 set_channel %d %d > %s", chn->primary_channel, chn->center_channel, TMP_FILE);
  if (0 != exec_iwnpi_cmd_for_status(cmd))
    goto err;

  g_wifi_data.marlin3_channel = chn;
  return return_ok(rsp);

err:
  g_wifi_data.marlin3_channel = NULL;
  return return_err(rsp);
}

int wifi_channel_get_marlin3(char *rsp) {
  int ret = -1;
  int primary_channel = 0;
  int center_channel = 0;
  WIFI_CHANNEL *chn = NULL;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN] = {0x00};

  if (wifi_enter_eut_mode() == 0)
    goto err;

  sprintf(cmd, "iwnpi wlan0 get_channel > %s", TMP_FILE);
  if (0 != exec_cmd(cmd))
    goto err;

  memset(cmd, 0, WIFI_EUT_COMMAND_MAX_LEN);
  ret = get_iwnpi_ret_line(IWNPI_RET_CHN_FLAG, cmd, WIFI_EUT_COMMAND_MAX_LEN);
  if (ret <= 0) {
    ENG_LOG("%s(), get_channel run err[%d].\n", __FUNCTION__, ret);
    goto err;
  }
  sscanf(cmd, "primary_channel:%d,center_channel:%d\n", &primary_channel, &center_channel);
  chn = match_channel_table_by_chn(primary_channel, center_channel);
  ENG_LOG("%s(), get_channel-> %s [%d, %d].\n", __FUNCTION__, (chn != NULL ? chn->name : "null"), primary_channel, center_channel);
  if (chn == NULL)
    goto err;
  g_wifi_data.marlin3_channel = chn;
  sprintf(rsp, "%s%s", WIFI_CHANNEL_REQ_RET, chn->name);
  rsp_debug(rsp);
  return 0;

err:
  g_wifi_data.marlin3_channel = NULL;
  sprintf(rsp, "%s%s", WIFI_CHANNEL_REQ_RET, "null");
  rsp_debug(rsp);
  return -1;
}

int wifi_channel_set(char *ch_note, char *rsp) {
  int ret = -1;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN] = {0x00};

  if (wifi_channel_set_marlin3(ch_note, rsp) == 0)
    return 0;

  int ch = atoi(ch_note);
  ENG_LOG("ADL entry %s(), ch = %d", __func__, ch);
  if (wifi_enter_eut_mode() == 0)
    goto err;

  if ((ch < 1) || (ch > 14)) {
    ENG_LOG("%s(), channel num err\n", __func__);
    goto err;
  }

  sprintf(cmd, "iwnpi wlan0 set_channel %d > %s", ch, TMP_FILE);
  if (0 != exec_iwnpi_cmd_for_status(cmd))
    goto err;

  g_wifi_data.channel = ch;
  return return_ok(rsp);

err:
  g_wifi_data.channel = 0;
  return return_err(rsp);
}

int wifi_channel_get(char *rsp) {
  if (wifi_channel_get_marlin3(rsp) == 0)
    return 0;

  ENG_LOG("%s(), channel:%d\n", __FUNCTION__, g_wifi_data.channel);
  if (0 == g_wifi_data.channel) {
    goto err;
  }

  sprintf(rsp, "%s%d", WIFI_CHANNEL_REQ_RET, g_wifi_data.channel);
  rsp_debug(rsp);
  return 0;

err:
  return return_err(rsp);
}

int wifi_txgainindex_set(int index, char *rsp) {
  int ret = -1;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN] = {0};

  if (wifi_enter_eut_mode() == 0)
    goto err;

  ENG_LOG("%s(), index:%d\n", __FUNCTION__, index);
  sprintf(cmd, "iwnpi wlan0 set_tx_power %d > %s", index, TMP_FILE);
  if (0 != exec_iwnpi_cmd_for_status(cmd))
    goto err;

  g_wifi_data.txgainindex = index;
  return return_ok(rsp);

err:
  g_wifi_data.txgainindex = -1;
  return return_err(rsp);
}

int wifi_txgainindex_get(char *rsp) {
 /* int ret = -1;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN] = {0};
  int level_a, level_b;

  wifi_status_dump();
  if (wifi_enter_eut_mode() == 0)
    goto err;
  sprintf(cmd, "iwnpi wlan0 get_tx_power > %s", TMP_FILE);
  if (0 != exec_cmd(cmd))
    goto err;

  memset(cmd, 0, sizeof(cmd));
  ret = get_iwnpi_ret_line(cmd, IWNPI_RET_BEGIN, IWNPI_RET_END);
  if (ret <= 0) {
    ENG_LOG("%s(), get_tx_power run err\n", __FUNCTION__);
    goto err;
  }
  sscanf(cmd, "level_a:%d,level_b:%d\n", &level_a, &level_b);
  sprintf(rsp, "%s%d,%d", WIFI_TXGAININDEX_REQ_RET, level_a, level_b);
  rsp_debug(rsp);
  return 0;
err: */
  return return_err(rsp);
}

static int wifi_tx_start(char *rsp) {
  int ret;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN] = {0};

  ENG_LOG("ADL entry %s()", __func__);

  wifi_status_dump();
  if (1 == g_wifi_data.tx_start) {
    ENG_LOG("ADL %s(), tx_start is aleady set to 1, goto ok.", __func__);
    goto ok;
  }

  if (1 == g_wifi_data.rx_start) {
    ENG_LOG("ADL %s(), Rx_start is 1, goto err. RX_START is 1", __func__);
    goto err;
  }
  /*we can not set tx on in cw mode*/
  if (1 == g_wifi_data.sin_wave_start) {
    ENG_LOG("ADL %s(), sin_wave_start is 1, goto err. SIN_WAVE_start is 1",
            __func__);
    goto err;
  }

  sprintf(cmd, "iwnpi wlan0 set_tx_count %d > %s", g_wifi_tx_count, TMP_FILE);
  if (0 != exec_iwnpi_cmd_for_status(cmd))
    goto err;

  /* reset to default, must be 0 */
  g_wifi_tx_count = 0;

  sprintf(cmd, "iwnpi wlan0 tx_start > %s", TMP_FILE);
  if (0 != exec_iwnpi_cmd_for_status(cmd))
    goto err;

ok:
  g_wifi_data.tx_start = 1;
  return return_ok(rsp);

err:
  g_wifi_data.tx_start = 0;
  return return_err(rsp);
}

static int wifi_tx_stop(char *rsp) {
  int ret;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN] = {0};

  ENG_LOG("ADL entry %s()", __func__);

  wifi_status_dump();

  if (0 == g_wifi_data.tx_start && 0 == g_wifi_data.sin_wave_start) {
    goto ok;
  }

  if (1 == g_wifi_data.rx_start) {
    goto err;
  }

  sprintf(cmd, "iwnpi wlan0 tx_stop > %s", TMP_FILE);
  if (0 != exec_iwnpi_cmd_for_status(cmd))
    goto err;

ok:
  g_wifi_data.tx_start = 0;
  g_wifi_data.sin_wave_start = 0;
  return return_ok(rsp);

err:
  g_wifi_data.tx_start = 0;
  g_wifi_data.sin_wave_start = 0;
  return return_err(rsp);
}

int wifi_tx_set(int command_code, int mode, int pktcnt, char *rsp) {
  ENG_LOG("ADL entry %s(), command_code = %d, mode = %d, pktcnt = %d", __func__,
          command_code, mode, pktcnt);

  if (wifi_enter_eut_mode() == 0)
    goto err;

  if ((1 == command_code) && (0 == g_wifi_data.rx_start)) {
    if (0 == mode) {
      /* continues */
      ENG_LOG("ADL %s(), set g_wifi_tx_count is 0", __func__);
      g_wifi_tx_count = 0;
    } else if (1 == mode) {
      if (pktcnt > 0) {
        ENG_LOG("ADL %s(), set g_wifi_tx_count is %d", __func__, pktcnt);
        g_wifi_tx_count = pktcnt;
      } else {
        ENG_LOG("ADL %s(), pktcnt is ERROR, pktcnt = %d", __func__, pktcnt);
        goto err;
      }
    }

    if (-1 == wifi_tx_start(rsp))
      goto err;
  } else if (0 == command_code) {
    wifi_tx_stop(rsp);
  } else {
    goto err;
  }

  return return_ok(rsp);

err:
  return return_err(rsp);
}

int wifi_tx_get(char *rsp) {
  sprintf(rsp, "%s%d", EUT_WIFI_TX_REQ, g_wifi_data.tx_start);
  rsp_debug(rsp);
  return 0;
}

static int wifi_rx_start(char *rsp) {
  int ret;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN] = {0};

  ENG_LOG("ADL entry %s(), rx_start = %d, tx_start = %d", __func__,
          g_wifi_data.rx_start, g_wifi_data.tx_start);

  wifi_status_dump();
  if (1 == g_wifi_data.rx_start) goto ok;
  if (1 == g_wifi_data.tx_start) goto err;

  sprintf(cmd, "iwnpi wlan0 rx_start > %s", TMP_FILE);
  if (0 != exec_iwnpi_cmd_for_status(cmd))
    goto err;

ok:
  g_wifi_data.rx_start = 1;
  return return_ok(rsp);

err:
  g_wifi_data.rx_start = 0;
  return return_err(rsp);
}

static int wifi_rx_stop(char *rsp) {
  int ret;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN] = {0};

  ENG_LOG("ADL entry %s(), rx_start = %d, tx_start = %d", __func__,
          g_wifi_data.rx_start, g_wifi_data.tx_start);

  wifi_status_dump();
  if (0 == g_wifi_data.rx_start) goto ok;
  if (1 == g_wifi_data.tx_start) goto err;
  sprintf(cmd, "iwnpi wlan0 rx_stop > %s", TMP_FILE);
  if (0 != exec_iwnpi_cmd_for_status(cmd))
    goto err;

ok:
  g_wifi_data.rx_start = 0;
  return return_ok(rsp);

err:
  g_wifi_data.rx_start = 0;
  return return_err(rsp);
}

int wifi_rx_set(int command_code, char *rsp) {
  ENG_LOG("ADL entry %s(), command_code = %d", __func__, command_code);

  if (wifi_enter_eut_mode() == 0)
    goto err;

  if ((command_code == 1) && (0 == g_wifi_data.tx_start)) {
    wifi_rx_start(rsp);
  } else if (command_code == 0) {
    wifi_rx_stop(rsp);
  } else {
    goto err;
  }
  return return_ok(rsp);

err:
  return return_err(rsp);
}

int wifi_rx_get(char *rsp) {
  ENG_LOG("%s()...\n", __FUNCTION__);
  sprintf(rsp, "%s%d", EUT_WIFI_RX_REQ, g_wifi_data.rx_start);
  rsp_debug(rsp);
  return 0;
}

int wifi_rssi_get(char *rsp) {
  int ret = WIFI_INT_INVALID_RET;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN] = {0};

  wifi_status_dump();
  if (1 == g_wifi_data.tx_start) {
    goto err;
  }
  sprintf(cmd, "iwnpi wlan0 get_rssi > %s", TMP_FILE);
  if (0 != exec_cmd(cmd))
    goto err;

  ret = get_result_from_iwnpi();
  if (WIFI_INT_INVALID_RET == ret) {
    ENG_LOG("iwnpi wlan0 get_rssi cmd err_code:%d", ret);
    goto err;
  }

  sprintf(rsp, "%s%d", EUT_WIFI_RSSI_REQ_RET, ret);
  rsp_debug(rsp);
  return 0;
err:
  return return_err(rsp);
}

int wifi_rxpktcnt_get(char *rsp) {
  int ret = -1;
  RX_PKTCNT *cnt = NULL;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN] = {0};

  ENG_LOG("ADL entry %s()", __func__);

  wifi_status_dump();
  if ((1 == g_wifi_data.tx_start)) {
    goto err;
  }

  sprintf(cmd, "iwnpi wlan0 get_rx_ok > %s", TMP_FILE);
  if (0 != exec_cmd(cmd))
    goto err;

  cnt = &(g_wifi_data.cnt);
  memset(cmd, 0x00, TMP_BUF_SIZE);
  ret = get_iwnpi_ret_line(IWNPI_RET_REG_FLAG, cmd, WIFI_EUT_COMMAND_MAX_LEN);
  ENG_LOG("ADL %s(), callED get_iwnpi_ret_line(), ret = %d", __func__, ret);
  if (ret <= 0) {
    ENG_LOG("%s, no iwnpi\n", __FUNCTION__);
    goto err;
  }
  sscanf(cmd, "reg value: rx_end_count=%d rx_err_end_count=%d fcs_fail_count=%d ",
               &(cnt->rx_end_count), &(cnt->rx_err_end_count),
               &(cnt->fcs_fail_count));

  sprintf(rsp, "%s%d,%d,%d", EUT_WIFI_RXPKTCNT_REQ_RET,
          g_wifi_data.cnt.rx_end_count, g_wifi_data.cnt.rx_err_end_count,
          g_wifi_data.cnt.fcs_fail_count);
  rsp_debug(rsp);
  return 0;
err:
  return return_err(rsp);
}

/********************************************************************
*   name   wifi_eut_init
*   ---------------------------
*   description: init some global's value when enter EUT mode
*   ----------------------------
*
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
static int wifi_eut_init(void) {
  int ret = -1;
  char rsp[WIFI_EUT_COMMAND_RSP_MAX_LEN + 1] = {0x00};

  ret = wifi_channel_set(((get_wcn_hw_type() == WCN_HW_MARLIN3) ? WIFI_EUT_DEFAULT_MARLIN3_CHANNEL : WIFI_EUT_DEFAULT_CHANNEL), rsp);
  rsp_debug(rsp);
  return ret;
}

/********************************************************************
*   name   wifi_lna_set
*   ---------------------------
*   description: Set lna switch of WLAN Chip
*   ----------------------------
*   para        IN/OUT      type            note
*   lna_on_off  IN          int             lna's status
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wifi_lna_set(int lna_on_off, char *rsp) {
  int ret = -1;
  int rate = -1;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN] = {0x00};

  ENG_LOG("ADL entry %s(), lna_on_off = %d", __func__, lna_on_off);

  if (wifi_enter_eut_mode() == 0)
    goto err;

  if (1 == lna_on_off) {
    sprintf(cmd, "iwnpi wlan0 lna_on > %s", TMP_FILE);
  } else if (0 == lna_on_off) {
    sprintf(cmd, "iwnpi wlan0 lna_off > %s", TMP_FILE);
  } else {
    ENG_LOG("%s(), lna_on_off is ERROR", __func__);
    goto err;
  }

  if (0 != exec_iwnpi_cmd_for_status(cmd))
    goto err;

  return return_ok(rsp);

err:
  return return_err(rsp);
}

/********************************************************************
*   name   wifi_lna_get
*   ---------------------------
*   description: get Chip's LNA status by iwnpi command
*   ----------------------------
*   para        IN/OUT      type            note
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wifi_lna_get(char *rsp) {
  int ret = WIFI_INT_INVALID_RET;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN] = {0x00};

  ENG_LOG("ADL entry %s()", __func__);
  wifi_status_dump();

  if (wifi_enter_eut_mode() == 0)
    goto err;

  sprintf(cmd, "iwnpi wlan0 lna_status > %s", TMP_FILE);
  if (0 != exec_cmd(cmd))
    goto err;

  ret = get_result_from_iwnpi();
  if (WIFI_INT_INVALID_RET == ret) {
    ENG_LOG("iwnpi wlan0 lna_status cmd, err_code = %d", ret);
    goto err;
  }

  sprintf(rsp, "%s%d", WIFI_LNA_STATUS_REQ_RET, ret);
  rsp_debug(rsp);
  return 0;
err:
  return return_err(rsp);
}

/********************************************************************
*   name   wifi_band_set
*   ---------------------------
*   description: set Chip's band(2.4G/5G) by iwnpi command set_band
*   ----------------------------
*   para        IN/OUT      type            note
*   band        IN          wifi_band       band number.
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*   band:2.4G/5G
*
********************************************************************/
int wifi_band_set(wifi_band band, char *rsp) {
  int ret = -1;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN] = {0x00};

  ENG_LOG("ADL entry %s(), band = %d", __func__, band);
  if (WCN_HW_MARLIN3 == get_wcn_hw_type()) {
    ENG_LOG("ADL entry %s(),marlin3 isn't support set band", __func__);
    goto err;
  }

  if (wifi_enter_eut_mode() == 0)
    goto err;

  if (!(WIFI_BAND_2_4G == band || WIFI_BAND_5G == band)) {
    ENG_LOG("%s(), band num is err, band = %d", __func__, band);
    goto err;
  }

  snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 set_band %d > %s", band,
           TMP_FILE);
  if (0 != exec_iwnpi_cmd_for_status(cmd))
    goto err;

  return return_ok(rsp);
err:
  return return_err(rsp);
}

/********************************************************************
*   name   wifi_band_get
*   ---------------------------
*   description: Get Chip's band(2.4G/5G) by iwnpi command from wifi RF
*   ----------------------------
*   para        IN/OUT      type            note
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*   band:2.4G/5G
*
********************************************************************/
int wifi_band_get(char *rsp) {
  int ret = WIFI_INT_INVALID_RET;
  wifi_band band = 0;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN] = {0x00};

  ENG_LOG("ADL entry %s()", __func__);
  if (WCN_HW_MARLIN3 == get_wcn_hw_type()) {
    ENG_LOG("ADL entry %s(),marlin3 isn't support set band", __func__);
    goto err;
  }
  snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 get_band > %s",
           TMP_FILE);
  if (0 != exec_cmd(cmd))
    goto err;

  band = get_result_from_iwnpi();
  if (WIFI_INT_INVALID_RET == ret) {
    ENG_LOG("iwnpi wlan0 get_band cmd, err_code = %d", ret);
    goto err;
  }

  snprintf(rsp, WIFI_EUT_COMMAND_RSP_MAX_LEN, "%s%d", WIFI_BAND_REQ_RET, band);
  rsp_debug(rsp);
  return 0;

err:
  return return_err(rsp);
}

/********************************************************************
*   name   wifi_bandwidth_set
*   ---------------------------
*   description: set Chip's band width by iwnpi command set_cbw
*   ----------------------------
*   para        IN/OUT      type            note
*   band        IN          wifi_bandwidth  band width.
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*   20 40 80 160
********************************************************************/
int wifi_bandwidth_set(wifi_bandwidth band_width, char *rsp) {
  char cmd[WIFI_EUT_COMMAND_MAX_LEN] = {0x00};

  ENG_LOG("ADL entry %s(), band_width = %d", __func__, band_width);
  if (wifi_enter_eut_mode() == 0)
    goto err;

  if (band_width > WIFI_BANDWIDTH_160MHZ) {
    ENG_LOG("%s(), band num is err, band_width = %d", __func__, band_width);
    goto err;
  }

  snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 set_cbw %d > %s",
           band_width, TMP_FILE);
  if (0 != exec_iwnpi_cmd_for_status(cmd)) {
    memset(cmd, 0, WIFI_EUT_COMMAND_MAX_LEN);
    snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 set_bandwidth %d > %s",
           band_width, TMP_FILE);
    if (0 != exec_iwnpi_cmd_for_status(cmd))
      goto err;
  }

  return return_ok(rsp);

err:
  return return_err(rsp);
}

/********************************************************************
*   name   wifi_bandwidth_get
*   ---------------------------
*   description: Get Chip's band width by iwnpi command from wifi RF
*   ----------------------------
*   para        IN/OUT      type            note
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*   band width:20 40 80 160
*
********************************************************************/
int wifi_bandwidth_get(char *rsp) {
  int ret = WIFI_INT_INVALID_RET;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN] = {0x00};

  ENG_LOG("ADL entry %s()", __func__);

  snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 get_cbw > %s",
           TMP_FILE);
  if (0 != exec_cmd(cmd)) {
    memset(cmd, 0, WIFI_EUT_COMMAND_MAX_LEN);
    snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 get_bandwidth > %s",
           TMP_FILE);
    if (0 != exec_cmd(cmd))
      goto err;
  }

  ret = get_result_from_iwnpi();
  if (WIFI_INT_INVALID_RET == ret) {
    ENG_LOG("iwnpi wlan0 get_band cmd, err_code = %d", ret);
    goto err;
  }
  snprintf(rsp, WIFI_EUT_COMMAND_RSP_MAX_LEN, "%s%d", WIFI_BANDWIDTH_REQ_RET,
           ret);
  rsp_debug(rsp);
  return 0;

err:
  return return_err(rsp);
}

/********************************************************************
*   name   wifi_tx_power_level_set
*   ---------------------------
*   description: set Chip's tx_power by iwnpi command set_tx_power
*   ----------------------------
*   para        IN/OUT      type            note
*   tx_power    IN          int             tx power
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
*
********************************************************************/
int wifi_tx_power_level_set(int tx_power, char *rsp) {
  int ret = -1;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN] = {0x00};

  ENG_LOG("ADL entry %s(), tx_power = %d", __func__, tx_power);
  if (wifi_enter_eut_mode() == 0)
    goto err;

  if (tx_power < WIFI_TX_POWER_MIN_VALUE ||
      tx_power > WIFI_TX_POWER_MAX_VALUE) {
    ENG_LOG("%s(), tx_power's value is invalid, tx_power = %d", __func__,
            tx_power);
    goto err;
  }

  snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 set_tx_power %d > %s",
           tx_power, TMP_FILE);
  if (0 != exec_iwnpi_cmd_for_status(cmd))
    goto err;

  return return_ok(rsp);

err:
  return return_err(rsp);
}

/********************************************************************
*   name   wifi_tx_power_level_get
*   ---------------------------
*   description: get Chip's tx power leaving by iwnpi command get_tx_power
*   ----------------------------
*   para        IN/OUT      type            note
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
*
********************************************************************/
int wifi_tx_power_level_get(char *rsp) {
  int tssi = WIFI_INT_INVALID_RET;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN] = {0x00};

  ENG_LOG("ADL entry %s()", __func__);
  snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 get_tx_power > %s",
           TMP_FILE);
  if (0 != exec_cmd(cmd))
    goto err;

  tssi = get_result_from_iwnpi();
  if (tssi == WIFI_INT_INVALID_RET) {
    int level_a = 0, level_b = 0;
    memset(cmd, 0x00, WIFI_EUT_COMMAND_MAX_LEN);
    if (get_iwnpi_ret_line(IWNPI_RET_PWR_FLAG, cmd, WIFI_EUT_COMMAND_MAX_LEN) <= 0)
      goto err;
    sscanf(cmd, "level_a:%d,level_b:%d", &level_a, &level_b);
    snprintf(rsp, WIFI_EUT_COMMAND_RSP_MAX_LEN, "%s%d,%d",
           WIFI_TX_POWER_LEVEL_REQ_RET, level_a, level_b);
    rsp_debug(rsp);
    return 0;
  }
  snprintf(rsp, WIFI_EUT_COMMAND_RSP_MAX_LEN, "%s%d",
           WIFI_TX_POWER_LEVEL_REQ_RET, tssi);
  rsp_debug(rsp);
  return 0;

err:
  return return_err(rsp);
}

/********************************************************************
*   name   wifi_pkt_length_set
*   ---------------------------
*   description: set Chip's pkt length by iwnpi command set_pkt_len
*   ----------------------------
*   para        IN/OUT      type            note
*   pkt_len     IN          int             pkt length
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
*
********************************************************************/
int wifi_pkt_length_set(int pkt_len, char *rsp) {
  int ret = -1;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN] = {0x00};

  ENG_LOG("ADL entry %s(), pkt_len = %d", __func__, pkt_len);
  if (wifi_enter_eut_mode() == 0)
    goto err;

  if (pkt_len < WIFI_PKT_LENGTH_MIN_VALUE ||
      pkt_len > WIFI_PKT_LENGTH_MAX_VALUE) {
    ENG_LOG("%s(), tx_power's value is invalid, pkt_len = %d", __func__,
            pkt_len);
    goto err;
  }

  snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 set_pkt_len %d > %s",
           pkt_len, TMP_FILE);
  if (0 != exec_iwnpi_cmd_for_status(cmd)) {
    memset(cmd, 0, WIFI_EUT_COMMAND_MAX_LEN);
    snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 set_pkt_length %d > %s",
           pkt_len, TMP_FILE);
    if (0 != exec_iwnpi_cmd_for_status(cmd))
      goto err;
  }

  return return_ok(rsp);

err:
  return return_err(rsp);
}

/********************************************************************
*   name   wifi_pkt_length_get
*   ---------------------------
*   description: get Chip's pkt length by iwnpi command get_pkt_len
*   ----------------------------
*   para        IN/OUT      type            note
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
*
********************************************************************/
int wifi_pkt_length_get(char *rsp) {
  int pkt_length = WIFI_INT_INVALID_RET;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN] = {0x00};

  ENG_LOG("ADL entry %s()", __func__);

  snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 get_pkt_len > %s",
           TMP_FILE);
  if (0 != exec_cmd(cmd)) {
    memset(cmd, 0x00, WIFI_EUT_COMMAND_MAX_LEN);
    snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 get_pkt_length > %s",
           TMP_FILE);
    if (0 != exec_cmd(cmd))
      goto err;
  }

  pkt_length = get_result_from_iwnpi();
  if (WIFI_INT_INVALID_RET == pkt_length) {
    ENG_LOG("iwnpi wlan0 get_pkt_len cmd, err_code = %d", pkt_length);
    goto err;
  }

  snprintf(rsp, WIFI_EUT_COMMAND_RSP_MAX_LEN, "%s%d", WIFI_PKT_LENGTH_REQ_RET,
           pkt_length);
  rsp_debug(rsp);
  return 0;

err:
  return return_err(rsp);
}

/********************************************************************
*   name   wifi_tx_mode_set
*   ---------------------------
*   description: set Chip's tx mode by iwnpi command set_tx_mode
*   ----------------------------
*   para        IN/OUT      type            note
*   tx_mode     IN          wifi_tx_mode    tx mode
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
*
********************************************************************/
int wifi_tx_mode_set(wifi_tx_mode tx_mode, char *rsp) {
  int ret = -1;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN] = {0x00};

  ENG_LOG("ADL entry %s(), tx_mode = %d", __func__, tx_mode);
  if (wifi_enter_eut_mode() == 0)
    goto err;

  if (tx_mode > WIFI_TX_MODE_LOCAL_LEAKAGE) {
    ENG_LOG("%s(), tx_power's value is invalid, tx_mode = %d", __func__,
            tx_mode);
    goto err;
  }

  /* if tx mode is CW,must be tx stop when tx start is 1, rather, call sin_wave
   */
  if (WIFI_TX_MODE_CW == tx_mode) {
    ENG_LOG("ADL %s(), tx_mode is CW, tx_start = %d", __func__,
            g_wifi_data.tx_start);
    if (1 == g_wifi_data.tx_start) {
      snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 tx_stop > %s",
               TMP_FILE);
      if (0 != exec_iwnpi_cmd_for_status(cmd))
        goto err;

      ENG_LOG("ADL %s(), set tx_start to 0", __func__);
      g_wifi_data.tx_start = 0;
    }

    snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 sin_wave > %s",
             TMP_FILE);
    if (0 != exec_iwnpi_cmd_for_status(cmd))
      goto err;

    g_wifi_data.sin_wave_start = 1;
    ENG_LOG("ADL %s(), set sin_wave_start is 1.", __func__);

  } else { /* other mode, send value to CP2 */
    ENG_LOG("ADL %s(), tx_mode is other mode, mode = %d", __func__, tx_mode);
    snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 set_tx_mode %d > %s",
             tx_mode, TMP_FILE);
    if (0 != exec_iwnpi_cmd_for_status(cmd))
      goto err;
  }

  return return_ok(rsp);

err:
  return return_err(rsp);
}

/********************************************************************
*   name   wifi_tx_mode_get
*   ---------------------------
*   description: get Chip's tx mode by iwnpi command tx_mode
*   ----------------------------
*   para        IN/OUT      type            note
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
*
********************************************************************/
int wifi_tx_mode_get(char *rsp) {
  int ret = WIFI_INT_INVALID_RET;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN] = {0x00};

  ENG_LOG("ADL entry %s()", __func__);

  snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 get_tx_mode > %s",
           TMP_FILE);
  if (0 != exec_cmd(cmd))
    goto err;

  ret = get_result_from_iwnpi();
  if (WIFI_INT_INVALID_RET == ret) {
    ENG_LOG("iwnpi wlan0 get_tx_mode cmd, err_code = %d", ret);
    goto err;
  }
  snprintf(rsp, WIFI_EUT_COMMAND_RSP_MAX_LEN, "%s%d", WIFI_TX_MODE_REQ_RET,
           ret);
  rsp_debug(rsp);
  return 0;

err:
  return return_err(rsp);
}

/********************************************************************
*   name   wifi_preamble_set
*   ---------------------------
*   description: set Chip's preamble type by iwnpi command set_preamble
*   ----------------------------
*   para            IN/OUT      type                    note
*   preamble_type   IN          wifi_preamble_type      tx power
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
*
********************************************************************/
int wifi_preamble_set(wifi_preamble_type preamble_type, char *rsp) {
  int ret = -1;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN] = {0x00};

  ENG_LOG("ADL entry %s(), preamble_type = %d", __func__, preamble_type);
  if (wifi_enter_eut_mode() == 0)
    goto err;

  if (preamble_type > PREAMBLE_TYPE_80211N_GREEN_FIELD) {
    ENG_LOG("%s(), preamble's value is invalid, preamble_type = %d", __func__,
            preamble_type);
    goto err;
  }

  snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 set_preamble %d > %s",
           preamble_type, TMP_FILE);
  if (0 != exec_iwnpi_cmd_for_status(cmd))
    goto err;

  return return_ok(rsp);

err:
  return return_err(rsp);
}

/********************************************************************
*   name   wifi_preamble_get
*   ---------------------------
*   description: get Chip's preamble type by iwnpi command get_preamble
*   ----------------------------
*   para        IN/OUT      type            note
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
*
********************************************************************/
int wifi_preamble_get(char *rsp) {
  int ret = WIFI_INT_INVALID_RET;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN] = {0x00};

  ENG_LOG("ADL entry %s()", __func__);

  snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 get_preamble > %s",
           TMP_FILE);
  if (0 != exec_cmd(cmd))
    goto err;

  ret = get_result_from_iwnpi();
  if (WIFI_INT_INVALID_RET == ret) {
    ENG_LOG("iwnpi wlan0 get_tx_mode cmd, err_code = %d", ret);
    goto err;
  }
  snprintf(rsp, WIFI_EUT_COMMAND_RSP_MAX_LEN, "%s%d", WIFI_PREAMBLE_REQ_RET, ret);
  rsp_debug(rsp);
  return 0;

err:
  return return_err(rsp);
}

/********************************************************************
*   name   wifi_payload_set
*   ---------------------------
*   description: set Chip's payload by iwnpi command set_payload
*   ----------------------------
*   para            IN/OUT      type                    note
*   payload         IN          wifi_payload            payload
*   rsp             OUT         char *                  response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
*
********************************************************************/
int wifi_payload_set(wifi_payload payload, char *rsp) {
  int ret = -1;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN] = {0x00};

  ENG_LOG("ADL entry %s(), payload = %d", __func__, payload);
  if (wifi_enter_eut_mode() == 0)
    goto err;

  if (payload > PAYLOAD_RANDOM) {
    ENG_LOG("%s(), payload's value is invalid, payload = %d", __func__,
            payload);
    goto err;
  }

  snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 set_payload %d > %s",
           payload, TMP_FILE);
  if (0 != exec_iwnpi_cmd_for_status(cmd))
    goto err;

  return return_ok(rsp);

err:
  return return_err(rsp);
}

/********************************************************************
*   name   wifi_payload_get
*   ---------------------------
*   description: get Chip's payload by iwnpi command get_payload
*   ----------------------------
*   para        IN/OUT      type            note
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
*
********************************************************************/
int wifi_payload_get(char *rsp) {
  int ret = WIFI_INT_INVALID_RET;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN] = {0x00};

  ENG_LOG("ADL entry %s()", __func__);

  snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 get_payload > %s",
           TMP_FILE);
  if (0 != exec_cmd(cmd))
    goto err;

  ret = get_result_from_iwnpi();
  if (WIFI_INT_INVALID_RET == ret) {
    ENG_LOG("iwnpi wlan0 get_tx_mode cmd, err_code = %d", ret);
    goto err;
  }
  snprintf(rsp, WIFI_EUT_COMMAND_RSP_MAX_LEN, "%s%d", WIFI_PAYLOAD_REQ_RET, ret);
  rsp_debug(rsp);
  return 0;

err:
  return return_err(rsp);
}

/********************************************************************
*   name   wifi_guardinterval_set
*   ---------------------------
*   description: set Chip's guard interval by iwnpi command set_gi
*   ----------------------------
*   para            IN/OUT      type                    note
*   gi              IN          wifi_guard_interval     guard interval
*   rsp             OUT         char *                  response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
*
********************************************************************/
int wifi_guardinterval_set(wifi_guard_interval gi, char *rsp) {
  char cmd[WIFI_EUT_COMMAND_MAX_LEN] = {0x00};

  ENG_LOG("ADL entry %s(), gi = %d", __func__, gi);

  if (gi > GUARD_INTERVAL_MAX_VALUE) {
    ENG_LOG("%s(), guard interval's value is invalid, gi = %d", __func__, gi);
    goto err;
  }

  snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN,
           "iwnpi wlan0 set_gi %d > %s", gi, TMP_FILE);
  if (0 != exec_iwnpi_cmd_for_status(cmd)) {
    memset(cmd, 0, WIFI_EUT_COMMAND_MAX_LEN);
    snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN,
           "iwnpi wlan0 set_guard_interval %d > %s", gi, TMP_FILE);
    if (0 != exec_iwnpi_cmd_for_status(cmd))
      goto err;
  }

  return return_ok(rsp);

err:
  return return_err(rsp);
}

/********************************************************************
*   name   wifi_guardinterval_get
*   ---------------------------
*   description: get Chip's guard interval by iwnpi command get_gi
*   ----------------------------
*   para        IN/OUT      type            note
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
*
********************************************************************/
int wifi_guardinterval_get(char *rsp) {
  int ret = WIFI_INT_INVALID_RET;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN] = {0x00};

  ENG_LOG("ADL entry %s()", __func__);

  snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 get_gi > %s",
           TMP_FILE);
  if (0 != exec_cmd(cmd)) {
    memset(cmd, 0, WIFI_EUT_COMMAND_MAX_LEN);
    snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 get_guard_interval > %s",
           TMP_FILE);
    if (0 != exec_cmd(cmd))
      goto err;
  }

  ret = get_result_from_iwnpi();
  if (WIFI_INT_INVALID_RET == ret) {
    ENG_LOG("iwnpi wlan0 get_gi cmd, err_code = %d", ret);
    goto err;
  }

  snprintf(rsp, WIFI_EUT_COMMAND_RSP_MAX_LEN, "%s%d",
           WIFI_GUARDINTERVAL_REQ_RET, ret);
  rsp_debug(rsp);
  return 0;

err:
  return return_err(rsp);
}

/********************************************************************
*   name   wifi_mac_filter_set
*   ---------------------------
*   description: set Chip's mac filter by iwnpi command set_macfilter
*   ----------------------------
*   para            IN/OUT      type                    note
*   on_off          IN          int                     on or off mac filter
*function
*   mac             IN          const char*             mac addr as string
*format
*   rsp             OUT         char *                  response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
*
********************************************************************/
int wifi_mac_filter_set(int on_off, const char *mac, char *rsp) {
  char cmd[WIFI_EUT_COMMAND_MAX_LEN] = {0x00};
  char mac_str[WIFI_MAC_STR_MAX_LEN + 1] = {0x00};

  ENG_LOG("ADL entry %s(), on_off = %d, mac = %s", __func__, on_off, mac);
  if (wifi_enter_eut_mode() == 0)
    goto err;

  if (on_off < 0 || on_off > 1) {
    ENG_LOG("%s(), on_off's value is invalid, on_off = %d", __func__, on_off);
    goto err;
  }

  /* parese mac as string if on_off is 1 */
  if (1 == on_off) {
    unsigned char len = 0;

    if (strlen(mac) < (WIFI_MAC_STR_MAX_LEN + 2 + strlen("\r\n"))) {
      ENG_LOG("%s(), mac's length is invalid, len = %lu", __func__,
              strlen(mac));
      goto err;
    }

    if (mac && *mac == '\"') {
      /* skip first \" */
      mac++;
    }

    while (mac && *mac != '\"') {
      mac_str[len++] = *mac;
      mac++;
    }

    /* first, execute iwnpi wlan0 set_macfilter to set enable or disable */
    snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 set_macfilter %d > %s",
             on_off, TMP_FILE);
    if (0 != exec_iwnpi_cmd_for_status(cmd))
      goto err;

    /* second, execute iwnpi wlan0 set_mac set MAC assress */
    memset(cmd, 0x00, WIFI_EUT_COMMAND_MAX_LEN);
    snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 set_mac %s > %s",
             mac_str, TMP_FILE);
    if (0 != exec_cmd(cmd))
      goto err;

  } else if (0 == on_off) {
    snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 set_macfilter %d > %s",
             on_off, TMP_FILE);
    if (0 != exec_iwnpi_cmd_for_status(cmd))
      goto err;
  }

  return return_ok(rsp);

err:
  return return_err(rsp);
}

/********************************************************************
*   name   wifi_mac_filter_get
*   ---------------------------
*   description: get Chip's mac filter switch and mac addr if on, by iwnpi
*command get_macfilter
*   ----------------------------
*   para        IN/OUT      type            note
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
*
********************************************************************/
int wifi_mac_filter_get(char *rsp) {
  int ret = -1;
  int on_off = 0;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN] = {0x00};

  ENG_LOG("ADL entry %s()", __func__);

  /* inquery mac filter switch */
  snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 get_macfilter > %s",
           TMP_FILE);
  if (0 != exec_cmd(cmd))
    goto err;

  on_off = get_result_from_iwnpi();
  if (WIFI_INT_INVALID_RET == on_off) {
    ENG_LOG("iwnpi wlan0 lna_status cmd, result = %d", on_off);
    goto err;
  }

  if (1 == on_off) {
    char mac_str[WIFI_MAC_STR_MAX_LEN] = {0x00};

    /* the mac filter switch is on, so get MAC address by iwnpi get_mac command
     */
    memset(cmd, 0x00, WIFI_EUT_COMMAND_MAX_LEN);
    snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 get_mac > %s",
             TMP_FILE);
    if (0 != exec_cmd(cmd))
      goto err;

    ret = get_iwnpi_ret_line(IWNPI_RET_MAC_FLAG, mac_str, WIFI_MAC_STR_MAX_LEN);
    if (ret <= 0) {
      ENG_LOG("iwnpi wlan0 lna_status cmd, err_code = %d", ret);
      goto err;
    }

    snprintf(rsp, WIFI_EUT_COMMAND_RSP_MAX_LEN, "%s%d,\"%s\"",
             WIFI_MAC_FILTER_REQ_RET, on_off, mac_str);
  } else if (0 == on_off) {
    snprintf(rsp, WIFI_EUT_COMMAND_RSP_MAX_LEN, "%s%d", WIFI_MAC_FILTER_REQ_RET,
             on_off);
  } else {
    ENG_LOG("%s(), on_off's value is invalid, on_off = %d", __func__, on_off);
    goto err;
  }

  rsp_debug(rsp);
  return 0;

err:
  return return_err(rsp);
}

/********************************************************************
*   name   wifi_ant_get
*   ---------------------------
*   description: get Chip's chain by iwnpi *command get_chain
*   ----------------------------
*   para        IN/OUT      type            note
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
*
********************************************************************/
int wifi_ant_get(char *rsp) {
  int ret = WIFI_INT_INVALID_RET;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN] = {0};

  sprintf(cmd, "iwnpi wlan0 get_chain > %s", TMP_FILE);
  if (0 != exec_cmd(cmd))
    goto err;

  ret = get_result_from_iwnpi();
  if (ret < 0) {
    ENG_LOG("iwnpi wlan0 get_chain cmd, err_code:%d", ret);
    goto err;
  }

  g_wifi_data.chain = (WIFI_CHAIN)ret;
  sprintf(rsp, "%s%d", WIFI_ANT_REQ_RET, ret);
  rsp_debug(rsp);
  return 0;

err:
  g_wifi_data.chain = 0;
  return return_err(rsp);
}

/********************************************************************
*   name   wifi_ant_set
*   ---------------------------
*   description: set Chip's chain by iwnpi command set_chain
*   ----------------------------
*   para            IN/OUT      type                    note
*   chain           IN          int                     wifi chain
*   rsp             OUT         char *                  response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
*
********************************************************************/
int wifi_ant_set(int chain, char *rsp) {
  char cmd[WIFI_EUT_COMMAND_MAX_LEN] = {0};

  if (wifi_enter_eut_mode() == 0)
    goto err;
  ENG_LOG("%s(), chain:%d\n", __FUNCTION__, chain);
  if (chain != CHAIN_MIMO && chain != CHAIN_PRIMARY)
    goto err;

  sprintf(cmd, "iwnpi wlan0 set_chain %d > %s", chain, TMP_FILE);
  if (0 != exec_iwnpi_cmd_for_status(cmd))
    goto err;

  g_wifi_data.chain = chain;
  return return_ok(rsp);
err:
  g_wifi_data.chain = 0;
  return return_err(rsp);
}
/********************************************************************
*   name   wifi_decode_mode_get
*   ---------------------------
*   description: get Chip's decode mode by iwnpi *command get_fec
*   ----------------------------
*   para        IN/OUT      type            note
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
*
********************************************************************/
int wifi_decode_mode_get(char *rsp) {
  int ret;
  char cmd[WIFI_EUT_COMMAND_MAX_LEN] = {0};

  sprintf(cmd, "iwnpi wlan0 get_fec > %s", TMP_FILE);
  if (0 != exec_cmd(cmd))
    goto err;

  ret = get_result_from_iwnpi();
  if (ret < 0) {
    ENG_LOG("iwnpi wlan0 get_fec cmd  err_code:%d", ret);
    goto err;
  }

  g_wifi_data.decode_mode = (WIFI_DECODEMODE) ret;
  sprintf(rsp, "%s%d", WIFI_DECODEMODE_REQ_RET, ret);
  rsp_debug(rsp);
  return 0;
err:
  g_wifi_data.decode_mode = DM_BCC;
  return return_err(rsp);
}

/********************************************************************
*   name   wifi_decode_mode_set
*   ---------------------------
*   description: set Chip's decode mode by iwnpi command set_fec
*   ----------------------------
*   para            IN/OUT      type                    note
*   dec_mode        IN          int                     wifi decode mode
*   rsp             OUT         char *                  response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
*
********************************************************************/
int wifi_decode_mode_set(int dec_mode, char *rsp) {
  char cmd[WIFI_EUT_COMMAND_MAX_LEN] = {0};

  if (wifi_enter_eut_mode() == 0)
    goto err;

  ENG_LOG("%s(), dec_mode:%d\n", __FUNCTION__, dec_mode);

  if (dec_mode > DM_LDPC || dec_mode < DM_BCC)
    goto err;

  sprintf(cmd, "iwnpi wlan0 set_fec %d > %s", dec_mode, TMP_FILE);
  if (0 != exec_iwnpi_cmd_for_status(cmd))
    goto err;

  g_wifi_data.decode_mode = (WIFI_DECODEMODE)dec_mode;
  return return_ok(rsp);

err:
  g_wifi_data.decode_mode = DM_BCC;
  return return_err(rsp);
}
/********************************end of the file*************************************/
