#ifndef GPS_COMMON_H
#define GPS_COMMON_H
#include <errno.h>
#include "agps_api.h"
#include <pthread.h>
#include <stdio.h>
#include <cutils/log.h>
#include <cutils/properties.h>
#include <stdlib.h>
#include <unistd.h>    
#include <sys/stat.h>
#include <string.h>  
#include <sys/types.h>
#include <semaphore.h>
#include <time.h>
#include <signal.h>
#include <hardware_legacy/power.h>
#include <fcntl.h>
#ifdef GNSS_LCS
#include "lcs_type.h"
#include "lcs_agps.h"
#else
#include "agps_type.h"
#include "agnss_type.h"
#endif
#include "gnss_libgps_api.h"
#include "gps_api.h"
#include "ge2_type.h"
#include "gps_cmd.h"
#include "gps.h"

#ifdef GNSS_INTEGRATED
#define  GNSS_SIPC_NAME_LEN	28
#endif

#define  WIFI_FUNC   0


#define  GPS_DEBUG   0    
#define  GPS_LTE     0
#define AGPS_PARAM_CONFIG
//#define AGPS_LTE 1
#define GNSS_REG_WR

#define UPDATE_BIN  0

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define  LOG_TAG  "LIBGPS"

#if GPS_DEBUG
#define  D(...)   LOGD(__VA_ARGS__)
#else
#define  E(...)   ALOGE(__VA_ARGS__)
#define  D(...)   ALOGD(__VA_ARGS__)

#endif

#define CTRL_MODE 1
#define SUPL_MODE 0
#ifdef GNSS_LCS
#define CG_SO_SETAPI
#endif 
#define  MAX_NMEA_TOKENS  32
#define  NMEA_MAX_SIZE  1023
#define TOKEN_LEN(tok)  (tok.end>tok.p?tok.end-tok.p:0)
#define SERIAL_READ_BUUFER_MAX       (1024) //todo 
#define SERIAL_WRITE_BUUFER_MAX      (2048) //todo 
#define GNSS_LOG_BUUFER_MAX          (2048) //todo 

#define GPS_RECV_MAX_LEN                 (4294967200UL)
#define GPS_SEND_MAX_LEN                 (4294967200UL)

#define POWER_ERROR_NUMBER_MAX        (9)
#define SEND_REPEAT_COUNT_MAX         (3)
#define MAX(__a__, __b__) 	((__a__) > (__b__) ? (__a__) : (__b__))
#define MIN(__a__, __b__) 	((__a__) > (__b__) ? (__b__) : (__a__))

//for boot add by zxw
#define START_DOWNLOAD_FD1 0x01
#define START_DOWNLOAD_GE2 0x02
#define CLOSE_DEVICE       0x03
#define DEVICE_EXCEPTION   0x04
#define DOWNLOAD_FAIL      0x05
#define GET_GNSS_STATUS    0x06

#define GNSS_SOCKET_NAME "gnss_server"
#define WCND_SOCKET_NAME    "wcnd" 

#define GNSS_DOWNLOAD_TIMEOUT  (12) // 6 seconds 



#define WCN_CMD_REBOOT_GNSS		"rebootgnss"
#define WCN_CMD_DUMP_GNSS		"dumpgnss"
#define WCN_CMD_START_GNSS		"startgnss"
#define WCN_CMD_STOP_GNSS			"stopgnss"
#define WCN_RESP_REBOOT_GNSS		"rebootgnss-ok"
#define WCN_RESP_DUMP_GNSS		"dumpgnss-ok"
#define WCN_RESP_START_GNSS		"startgnss-ok"
#define WCN_RESP_STOP_GNSS		"stopgnss-ok"
#define WCN_RESP_STATE_GNSS		"gnssstate-ok"

#define GET_XML 0x00
#define SET_XML 0x01
#define SUPL_XML_PATH           "/data/gnss/supl/supl.xml"
#define CONFIG_XML_PATH         "/data/gnss/config/config.xml"
#define RAWOBS_PATH             "/data/gnss/lte/raw.obs"
#define GE2_FDL_PATH            "/data/gnss/bin/gnssfdl.bin"


#define SUPL_ETC_PATH          "/system/etc/supl.xml"
#define CONFIG_ETC_PATH        "/system/etc/config.xml"
#define SPREADORBIT_ETC_PATH   "/system/etc/jpleph.405"
#define RAWOBS_ETC_PATH         "/system/etc/raw.obs"
#define GE2_FIRMWARE_ETC_PATH   "/system/etc/gnssmodem.bin"
#define GE2_FDL_ETC_PATH        "/system/etc/gnssfdl.bin"
#define AGNSS_CER_ETC_PATH      "/system/etc/spirentroot.cer"
#define COMBINE_PATH            "/system/etc/combined_gnssmodem.bin"
#ifdef GNSS_INTEGRATED
//SHARKLE/PIK2 
#define SHARKLE_PIKL2_GNSS_POWER_DEV   "/sys/class/misc/gnss_common_ctl/gnss_power_enable"
#define SHARKLE_PIKL2_GNSS_SUBSYS_DEV   "/sys/class/misc/gnss_common_ctl/gnss_subsys"
#define SHARKLE_PIKL2_GNSS_DUMP_DEV   "/sys/class/misc/gnss_common_ctl/gnss_dump"
#define SHARKLE_PIKL2_GNSS_STATUS_DEV   "/sys/class/misc/gnss_common_ctl/gnss_status"
#define INTERGRATED_GNSS_STATUS_POWEROFF (0)
#define INTERGRATED_GNSS_STATUS_POWERON  (1)
#define INTERGRATED_GNSS_STATUS_ASSERT  (2)
#define INTERGRATED_GNSS_STATUS_POWEROFF_GOING  (3)
#define INTERGRATED_GNSS_STATUS_POWERON_GOING  (4)

#define SHARKLE_PIKL2_PARTITION_NMAE_LEN  (128)

#define RF_DATA_RAWCOUNT  (3072) //12K /4
#else
#define RF_DATA_RAWCOUNT   (16384) //64k/4
#endif

#define SUPL_TYPE         0
#define CONFIG_TYPE       1
#define SPREADORBIT_TYPE  2
#define FIRMWARE_TYPE     3
#define FDL_TYPE          4
#define RAWOBS_TYPE       5
#define CER_TYPE          6

#define GPS_GLONASS       0
#define GPS_BDS           1
#define GLONASS_IMG       3
#define BDS_IMG           4

#ifdef GNSS_INTEGRATED
#define GPS_GLONASS_IMG   16
#define GPS_BDS_IMG       17
#endif

#define PI 3.14159265358979323846
#define WGS_AXIS_A  6378137.0
#define WGS_E1_SQR  0.006694379990141317
#define EARTH_R 6371.004
#define ABS(x)  (((x) > 0 ? (x): -(x)))

//agps session protection macro and constaint define
#define AGPS_SESSION_START   (1)
#define AGPS_SESSION_STOP    (2)
#define AGPS_SESSION_TIMEOUT (3)

#define AGPS_GLONASS_EPH_MAX_COUNT  (28)   //28
#define AGPS_SESSION_TIMEOU_SECONDS (3600) //one hour
#define GPS_WEEK_NO_SECONDS         (604800) //one hour

#define TRACE_OFFSET              544*1024
  
#define FORMAT_MAX_LENTH   8
#define LINE_MAX_NUM       2000
#define XML_FILE_MAX_MUM   (256)

#define ASSERT_TYPE_UNKOWN   0
#define ASSERT_TYPE_MANUAL   1
#define ASSERT_TYPE_FIRMWARE 2

//modem slog define socket server and port 
#define LOCAL_HOST "127.0.0.1"
#define SLOG_MODEM_SERVER_PORT 4321




typedef struct
{
    float freq;
    float temprature;
}TFTable;
#ifdef GNSS_INTEGRATED
typedef struct
{
    int freq;
    int temprature;
    int temprature_osc;
}TSX_DATA_T;
#define TSX_DATA_GROUP_NUM 4
typedef struct
{
	double temprature;
	double temprature_osc;
}TSX_TEMP_T;

typedef struct
{
	/*TFTable tsxTable[200];*/
	int temprature_diff;
	double c0;
	double c1;
	double c2;
	double c3;
	double c1_Osc;
	double c2_Osc;
	double c3_Osc;
	double invP[49]; /* 7*7 */
	unsigned int validFlag_remain[8];
	unsigned int validFlag[4][21];
}TsxData;
#else
typedef struct
{
	int freq;
	int temprature;
}TSX_DATA_T;
#define TSX_DATA_GROUP_NUM 2
typedef struct
{
	double temprature;
}TSX_TEMP_T;

typedef struct
{
	TFTable tsxTable[200];
	double c0;
	double c1;
	double c2;
	double c3;
	double invP[16];
	unsigned int validFlag_remain[8];
	unsigned int validFlag[4];
}TsxData;
#endif
typedef struct ttf{
	unsigned int weekno;
	unsigned int mscount;
}gpstime;

typedef struct {
    int     prn;
    float   snr;
} RawSvData;

typedef struct {
    int     	num_svs;
    RawSvData   sv_list[50];
} RawGpsSvInfo;
RawGpsSvInfo sv_rawinfo;

//end
typedef enum DevicepowerState
{
	DEVICE_DOWNLOADING = 1,
	DEVICE_DOWNLOADED,
	DEVICE_POWERONING,
	DEVICE_POWERONED,
	DEVICE_WORKING,
	DEVICE_WORKED,
	DEVICE_IDLING,
	DEVICE_IDLED,
	DEVICE_SLEEPING,
	DEVICE_SLEEP,
    DEVICE_WAKEUPING,
    DEVICE_RESET,
	DEVICE_POWEROFFING,
	DEVICE_POWEROFFED,
	DEVICE_ABNORMAL
}TDevicepowerState;

typedef struct {
    const char*  p;
    const char*  end;
} Token;


typedef struct {
    int     count;
    Token   tokens[ MAX_NMEA_TOKENS ];
} NmeaTokenizer;


struct timeout_actions_elem {
    int inuse;
    int id;
    time_t tout;
    void (*timeout_cb)(int, void *data);
    void *data;
}; 

typedef struct {
    int     pos;
    int     overflow;
    int     utc_year;
    int     utc_mon;
    int     utc_day;
    GpsLocation  fix;
    GpsSvStatus  svstatus;
    int svinfo_flags;
    gps_location_callback  location_callback;
    gps_sv_status_callback  sv_status_callback;
#ifdef GNSS_ANDROIDN	
	GnssSvStatus  gnss_svstatus;
	int gnss_used_in_fix_mask[32];  // save the svid of svs used in fix
	int gnss_used_in_fix_nums;  // count the svs used in fix
	gnss_sv_status_callback gnsssv_status_callback;
#endif //GNSS_ANDROIDN 
    char    in[ NMEA_MAX_SIZE+1 ];
} NmeaReader;

typedef enum{
   LIBGPSTYPE,
   SUPLTYPE,
   CONTROLTYPE,
   ENGINETYPE
}GPSTYPE;

typedef enum
{
    DUMP_RECV_START_UNKOWN,
    DUMP_RECV_CODE_START,
    DUMP_RECV_CODE_END,//it qual data start
    DUMP_RECV_DATA_END,
    DUMP_RECV_MAX,
}DUMP_RecvState_e;

//supl data 
typedef enum
{
    SUPL_NET_RELEASE = 0,
    SUPL_NET_REQ = 1,
    SUPL_NET_ENABLE = 2
}SUPL_NetState_e;


typedef struct {
	//global state value
	int      init;
	int      first;   
	int      first_idleoff;
	int      fd;
	int      powerctrlfd;
	int      gnsslogfd;
    int      socet_client;//DAEMON socket client 
	int      slog_switch;
	int      slogmodem_fd;
	int      hardware_id;
    int      getetcconfig;
    unsigned char  sleepCount;
    unsigned char  wakeupCount;
#ifndef GNSS_INTEGRATED
    char serialName[22];//fd  string name
#else
    char serialName[GNSS_SIPC_NAME_LEN + 2];//fd  string name
#endif
	char supl_path[64];
	char binPath[128];
	//	int readBuffersize;
	unsigned char readBuffer[SERIAL_READ_BUUFER_MAX];
	unsigned char writeBuffer[SERIAL_WRITE_BUUFER_MAX];
	unsigned char gnsslogBuffer[GNSS_LOG_BUUFER_MAX];
	int  writeoffsize;
	int  sendLen;
	int  sendFlag; 
	TCircular_buffer serialWrite;
	pthread_mutex_t  writeMutex; 
	NmeaReader  NmeaReader[1];
	int      ref;
	//for set mode param
	GpsPositionRecurrence   recurrence;
	int      min_interval;
	int      preferred_accuracy;
	int      preferred_time;
	int      fix_frequency; 
	//for supl server and port
	char     suplhost[80];
	int      suplport;
	//for callback
	GpsCallbacks         callbacks;
	AGpsCallbacks        agps_callbacks;
	GpsNiCallbacks       ni_callbacks;
	AGpsRilCallbacks     agps_ril_callbacks;
	GpsXtraCallbacks     xtra_callbacks;
	GpsGeofenceCallbacks geofence_callbacks;
	GpsMeasurementCallbacks measurement_callbacks;
	GpsNavigationMessageCallbacks navigation_message_callbacks;
	//for data conn,zhouxw add
	int  connected;
	int  type;
	int  avaiable;
	int  wifi_ok;
	int  m_capabilities;
	int  gps_flag; 
	int  gps_open;
    int  started;
	int  fix_stat;
	int  logswitch;
	int  workmode;
	//thread for gps
	pthread_t  thread;
	pthread_t  ril_thread;
	pthread_t  daemon_thread;
	pthread_t  lte_thread;
	pthread_t  watchdog_thread;
	pthread_t  lcs_clientthread; 
	pthread_t  udp_thread;
	//pthread_t  data_capture_thread;
	pthread_t  gnss_log_thread;
	//phread mutex init
	pthread_mutex_t    mutex; 
	pthread_mutex_t    gnsslogmutex;
    pthread_mutex_t    socketmutex;
	pthread_mutex_t    saveMutex;
	pthread_cond_t cond;
	sem_t threadsync;  //gps
	sem_t gpsdsem;  //gpsd event wait
	sem_t readsync;
	sem_t vesionsync;
    sem_t gnssLogsync;
	//control sock
	int     control[2];
	int     asock[2];
	//for print debug,can remove
	int     cmd_flag;
	int     cpmode;
	int     outtype;
	int     lte_open;	
	int     lte_running;
	int     nokiaee_enable;  // NOKIA EE switch status, 1: enable  0:disable
	int     use_nokiaee;     // NOKIA EE use status, 1: use  0:nouse
	int     realeph;
	int     cmcc;
	int     IdlOnCount;
	int     IdlOffCount;
	int     HeartCount;
    int     readCount;
	int     waitHeartRsp;
	int     keepIdleTime;
	int     postlog;
	int     watchdog;
	int     imgmode;
	int     sleepFlag;
	int     sleepTimer ;
	int     happenAssert; 
    int     assertType;
	//it don't send sleep command in  5 minuter when laster cold-start
	int     hotTestFlag;
	//these is for power value
	TDevicepowerState powerState;// the device power state 
	int     powerErrorNum;
	sem_t powersync;  //power state sync
	FILE*  fFlashCfg;
	//GNSS_REG_WR
	unsigned int wRegAddr;
	unsigned int wRegValue;
	unsigned int rRegAddr;
	unsigned int rRegValue;
	int  screen_off;    // SCREENOFF
	int  release; 
	//AGPS Timeout protection
    int supldata;
	int agps_session;
	time_t lastreftime;
	time_t lastrefeph;
	unsigned int deleteAidingDataBit;
	int tsx_enable;
	int tsxIsdeliver;
    int etuMode;
	double tsx_temp;   // TSXTEMP
	int    cw_enable;	// HW_CW
	int rftool_enable;
	int float_cn_enable;
	int baseband_mode;
#ifdef GNSS_ANDROIDN
	GnssSystemInfo gnss_system_info;
#endif //GNSS_ANDROIDN 
	unsigned int combineOffset;
	int network_status;
} GpsState;

#define MSB_SI       0
#define MSB_NI       1
#define MSA_SI       2
#define MSA_NI       3
#define GEO_MSB_SI   4
#define GEO_MSB_NI   5
#define GEO_MSA_SI   6
#define GEO_MSA_NI   7
#define PEROID_SI    8
#define PEROID_NI    9
#define CP_SI        10
#define CP_NI        11


typedef struct
{
    char prn;
    short message_id;
    short submessage_id;
    unsigned char data[40];
}navigation_type;

typedef struct{
	int cur_state;
	int old_state;
	int north;
	int east;
	int type;
	long numfixed;
	int times;
	long ni_interval;
	int si_sid;
	int ni_sid;
	int readySid;
	int msaflag;
	long radis;
	int agps_enable;
	int udp_stat;
	int udp_enable;
	int wifiStatus;
	double geolatitude; 
	double geolongitude; 
	double geoaltitude; 
	double msalatitude; 
	double msalongitude; 
	double msaaltitude; 
	TcgVelocity Velocity;
    //lte para data
    int mask; // sv bit lists 
    int weekno; 
    int tow; 
    int ni_status;
    int peroid_mode;
    int  cp_begin;
    int cp_ni_session;
    sem_t JavaEvent;
	int SensorStatus;
	int glonassNum;
	int gpsNum;
	int timeValid;
	int locValid;
	int gpsEphValid;
	int glonassEphValid;
	int bdsEphValid;
	unsigned int assistflag;
	unsigned char gnssidReq;
//uncertainty value
	int uncertaintySemiMajor;
    int uncertaintySemiMinor;
    int orientationMajorAxis;
    int uncertaintyAltitude;
	navigation_type navidata;
    int suplniLen;
    unsigned char suplnimsg[512];
}AgpsData_t;

typedef struct 
{
    unsigned char sv_id;
    unsigned char merge_time;
    unsigned short softbit_buf_read_index;
    unsigned short softbit_buf_write_index;
    char softbit_buf[1500];
}softbit_data;

typedef struct 
{
    unsigned char sv_id;
    unsigned char merge_time;
    unsigned short start_index;
    unsigned short len;
    char writebuf[300];
}softbit_receivebuf;

typedef struct 
{
    unsigned char sv_id;
    unsigned short write_index;
    char merge[60];
}softbit_sendbuf;

typedef struct javadata
{
    char buff[1024];
    int len;
    sem_t JavaEvent;
}JavaDataType;

typedef struct LLC
{
	unsigned int   weekNo;         /*< week number (from beginning of current week cycle) */
	unsigned int    second;          /*<  seconds (from beginning of current week no) */
	unsigned int    secondFrac;      /*<  seconds fraction (from beginning of current second) units: 1ns */
	DOUBLE lat;
	DOUBLE lon;
	DOUBLE hae;
} WLLH_TYPE;

typedef struct __TAgpsMsrElement_t 
{
    unsigned char  systemid;     //0-gps,1-glonass,2-beidou
    unsigned char  satelliteID;  // range: 0-63
    int            cNo;          // carrier noise ratio, range:(0..63)
    float          doppler;      // doppler, range:(-32768/5..32767/5)
    int            wholeChips;   // whole value of the code phase measurement, range:(0..1022)
    int            fracChips;    // fractional value of the code phase measurement, range:(0..1023),
    int            mpathInd;     // multipath indicator
    int            pseuRangeRMSErr; // index, range:(0..63)

	unsigned long long flags;
	long long	received_gps_tow_ns;
	long long	received_gps_tow_uncertainty_ns;
	double		pseudorange_m;
	double		pseudorange_uncertainty_m;
	float		carrier_frequency_hz; // default L1

	unsigned int carrier_count;
	double		carrier_phase;
	int 		bit_number; //Total bit number since midnight
	short		time_from_last_bit_ms; // how many ms in one bit
	double		elevation_deg;
	double		elevation_uncertainty_deg;
	double		azimuth_deg;
	double		azimuth_uncertainty_deg;
	int 		used_in_fix;
} TAgpsMsrElement_t;

typedef struct __TAgpsMsrSetList_t
{
    int                 gpsTOW;    // unit:ms
    unsigned char       listNums;  // range:0-16
    TAgpsMsrElement_t   msrElement[14];  // modify 16 to 14
}TAgpsMsrSetList_t;

typedef struct __TAgpsMsrElement_rawt
{
    unsigned char  systemid;     //0-gps,1-glonass,2-beidou
    unsigned char  satelliteID;  // range: 0-63
    int            cNo;          // carrier noise ratio, range:(0..63)
    float          doppler;      // doppler, range:(-32768/5..32767/5)
    int            wholeChips;   // whole value of the code phase measurement, range:(0..1022)
    int            fracChips;    // fractional value of the code phase measurement, range:(0..1023),
    int            mpathInd;     // multipath indicator
    int            pseuRangeRMSErr; // index, range:(0..63)
} TAgpsMsrElement_raw_t;

typedef struct __TAgpsMsrSetList_rawt
{
    int                 gpsTOW;    // unit:ms
    unsigned char       listNums;  // range:0-16
    TAgpsMsrElement_raw_t   msrElement[14]; // todo  change 14
}TAgpsMsrSetList_raw_t;

typedef struct {
	int  bdsURAI;
	int  bdsToe;   /* 2^3  sec*/
	unsigned int  bdsAPowerHalf;  /*2^-19   meters^1/2*/
	unsigned int  bdsE;  /*2^-33  */
	int  bdsW;  /*2^-31   semi-circles */
	int  bdsDeltaN; /*2^-43   semi-circles/sec */
	int  bdsM0;  /*2^-31   semi-circles */
	int  bdsOmega0;  /*2^-31   semi-circles */
	int  bdsOmegaDot; /*2^-43   semi-circles/sec */
	int  bdsI0;  /*2^-31   semi-circles */
	int  bdsIDot;  /*2^-43   semi-circles/sec */
	int  bdsCuc;  /*2^-31  radians*/
	int  bdsCus;  /*2^-31  radians*/
	int  bdsCrc;  /*2^-6  meters*/
	int  bdsCrs;  /*2^-6  meters*/
	int  bdsCic;  /*2^-31  radians*/
	int  bdsCis;  /*2^-31  radians*/
} *PBDS_EPHEMERIS,BDS_EPHEMERIS;

typedef struct{
	int  bdsToc;  /* 2^3  seconds */
	int  bdsA0;   /* 2^-33  sec/sec */
	int  bdsA1;   /* 2^-50  sec/sec^2 */
	int  bdsA2;    /* 2^-66  seconds */
	int  bdsTgd1;  /* 10^-10  seconds */
} *PBDS_CLOCK_EPHEMERIS,BDS_CLOCK_EPHEMERIS;

typedef struct{
    char tag[4];
    unsigned int offset;
    unsigned int  size;
}IMG_INFO_T;

typedef struct{
    int combineValid;
	unsigned int offset;
	unsigned int bdoffset;
	unsigned int gloffset;
}COMBINE_INFO_T;

extern GpsState  _gps_state[1];
extern AgpsData_t agpsdata[1]; 
extern COMBINE_INFO_T combineInfo;
extern GpsData gps_msr;
extern char data_exist_flag;
extern char logbuf[GNSS_LOG_BUUFER_MAX];
extern int   gLogType;
extern int   gWriteFlag;
extern int    gDropCount;
extern CgPositionParam_t gLocationInfo[1];
extern char fname[64];
extern int gpsTow;
#ifdef GNSS_ANDROIDN
extern GnssData gnss_msr;
#endif //GNSS_ANDROIDN 
void deliver_msa(TCgAgpsPosition *pPosition,TcgVelocity *pvelocity);
int get_gps_state(GPSTYPE type);
void nmea_save_nmea_data(NmeaReader*  r);
void store_nmea_log(char lnum);
int gps_power_Statusync(int value);
int  epoll_register( int  epoll_fd, int  fd );
int  epoll_deregister( int  epoll_fd, int  fd );
void send_agps_cmd(char cmd);
int gps_writeSerialData(GpsState* s,int len);
int gps_hardware_open(void);
int gps_hardware_close(void);
unsigned int CG_setPosMode(int aPosMethod);
void set_supl_param(void);
void test_gps_mode(GpsState *s);
void  nmea_reader_addc( NmeaReader*  r, int  c );
void supl_init(void);
char assist_get_wifi_info(void);
int set_measurement_data(const unsigned char *buf,unsigned int len);
void set_ni_position(const char *buf);
unsigned short NI_SUPL_END(void);
void  gps_state_start(GpsState*  s);
int gps_start(void);
int gps_stop(void);
void gps_serialPortRead(GpsState *pGPSState);
void gps_sendData(int flag, const char* pBuf);
void gps_setPowerState(int state);
const char* gps_PowerStateName(int state);
int gps_SendNotify(GpsState *s,int flag);
void  gps_dispatch(TCmdData_t* pPacket);
int gps_devieManger(int flag);
void gps_saveDatadump(char *buf,int length);
int gps_saveCodedump( char *buf,int length);

void  nmea_reader_parse( NmeaReader*  r );

void SoonerFix_PredictThread(void *arg);   //lte
int parse_lte_request(char *buf,unsigned short lenth);
int EncodeSvData(TEphemeris_t *eph, GPS_EPHEMERIS_PACKED *ephPacked);
int Trans_Ephpacked(TCgAgpsEphemeris_t *epacked,TEphemeris_t *eph);
#ifdef GNSS_LCS
void gnss_lcscp_molr(void);
#else
void start_cp_molr(void);
#endif

int get_session_id(int si);
void start_agps_si(int mode);
void stop_agps_si(void);
void agent_thread(void);
int Get_Msr(TcgAgpsMsrSetList_t *msrlist,GNSSVelocity_t *vel);
int gnssGetMsr(GNSSMsrSetList_t *msrlist,GNSSVelocity_t *vel);
void agps_thread_init(GpsState* s);
void clear_agps_data(void);
int AGPS_End_Session(void);
void SF_updateObsTask(char *buf,int lenth);
void start_geofence_timer(void);
void geofence_task_ni(TcgAreaEventParams_t *pParam);
void update_assist_data(GpsState* s);
int OsTimeSystemGet( gpstime *aGpsTime);
int GET_UBITS(unsigned int data, unsigned int pos, unsigned int len);
int get_netinfo(int *val,char *dev_name);
int parse_register_value(char *buf,unsigned short lenth);
unsigned int app_read_register(unsigned int addr);
void gps_screenStatusRead(GpsState *s);
int gps_wakeupDevice(GpsState* s);
void  gps_notiySleepDevice(GpsState *s );
void gps_restorestate(GpsState* s);
int gps_adingData(TCmdData_t *pPak);
int recoverDevice(GpsState* pState);
void createDump(GpsState* pState);
void preprocessGnssLog(unsigned char  *data,unsigned short len,unsigned char subType);
void writeGnsslogTokernel(unsigned char *data, unsigned short len);
void parseNmea(unsigned char *buf,int len);
void nmea_parse_cp(unsigned char *buff,unsigned short lenth);
void read_slog_status(void);
void update_tsx_param(void);
void get_tsx_temp(unsigned char *buf, unsigned short length);
int get_socket_fd(char* server,struct timeval* ptimeout);
int EncodeBD2EphemerisD2(PBDS_EPHEMERIS pEph, PBDS_CLOCK_EPHEMERIS pClockEph, GPS_EPHEMERIS_PACKED *ephPacked);
int EncodeBD2EphemerisD1(PBDS_EPHEMERIS pEph, PBDS_CLOCK_EPHEMERIS pClockEph, GPS_EPHEMERIS_PACKED *ephPacked);
void set_supl_server(void);
int EncodeGpsAlmanac(GPSAlmanacElementRRLP_t *pAlm,GPS_ALMANAC_PACKED *pGpsAlmPacked);
void compute_abs_timeout(struct timespec *ts, int ms);
void transGEOPos(TCgAgpsPosition *pPos,GNSSLocationCoordinates_t *pPosOut);
int s2int(char *p, char len);
void gnss_notify_abnormal(char* server,struct timeval* ptimeout,char* cause);

int msg_to_libgps(char cmd, unsigned char *pbuf,void *ext1,void *ext2);
char  parse_wifi_xml(void);
int CG_deliverAssDataIonoModel(int sid, GNSSIonosphericModel_t *pIonoModel);
int CG_deliverGNSSAssData(int sid, GNSSAssistData_t *pAssData);
int CG_deliverAssistData_End(int sid);
void set_cp_param(void);
int query_server_mode(void);
void agps_release_data_conn(GpsState*  s);
int agps_request_data_conn(GpsState*  s );
unsigned int CG_gotSUPLInit(unsigned char *pSUPLINIT_Data, unsigned short SUPLINIT_Len);
void transGNSSPos(TCgAgpsPosition *pPos,GNSSPosition_t *pPosOut);
#ifdef GNSS_INTEGRATED
int sharkle_gnss_switch_modules();
int sharkle_gnss_ready(int ctl ); 
int gnss_request_memdumpl();
#endif


#endif //GPS_COMMON_H
