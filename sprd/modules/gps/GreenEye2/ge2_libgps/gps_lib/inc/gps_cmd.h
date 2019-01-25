#ifndef __GPS_CMD_H
#define __GPS_CMD_H

#define CG_ALMANAC			0x0001
#define CG_UTC_MODEL			0x0002
#define CG_IONIC_MODEL			0x0004
#define CG_DGPS_CORRECTION		0x0008
#define CG_LOCATION		        0x0010
#define CG_TIME		                0x0020
#define CG_ACQUISITION_ASSISTANCE	0x0040
#define CG_REAL_TIME_INTEGRITY		0x0080
#define CG_NAVIGATION_MODEL		0x0100

#define IMSI_MAXLEN   15
#define MSISDN_MAXLEN 11
#define SOCK_PATH "/data/gnss/supl/foo.sock"
#define SUPL_PATH "/data/gnss/supl/supl.sock"


#define CMDTTY   0  
#define GPSTEST  1
#define GPS_GPIO_INCLUDE 0

#define FIFO_SUPL "/data/gnss/supl/nififo"
#define SUPL_SOCK_PATH "/data/gnss/supl/supl.sock"
#define open_flag  1
#define close_flag 0
#define true 1
#define false 0  
//ge2 mode
#define GPS_BIT0        1
#define BDS_BIT1        2
#define GLONASS_BIT2    4
//output protocol
#define SPRD_MSG_BIT0   1
#define NMEA_LOG_BIT1   2
#define POST_MSG_BIT2   4
#define DEBUG_MSG_BIT3  8

#define HEART_COUNT_MAX 10
#define GNSS_MONITOR_INTERVAL_TIME (2) //unit second 
#define GNSS_ILDE_KEEPTIME_MAX     (150) 
#define CODEDUMP_MAX_SIZE (0x58000)
#define DATADUMP_MAX_SIZE (0x30000)
#define PCHANNELDUMP_MAX_SIZE (0x20000)
#define DUMP_TOTAL_SIZE   (0x88000)


#define _SIZE(x) (sizeof(x)/sizeof(x[0]))

//mod sv_used flag 2 to 4
#define SVINFO_GOT_EPH_INFO_FLAG   0x0001
#define SVINFO_GOT_SV_INFO_FLAG    0x0002
#define SVINFO_GOT_SV_USED_FLAG    0x0004   

#define IDLE_ON_SUCCESS    1001
#define IDLE_ON_FAIL      1002
#define IDLE_OFF_SUCCESS  1003
#define IDLE_OFF_FAIL     1004
#define POWEROFF          1005
#define POWER_WAKEUP      1006
#define POWER_SLEEPING    1007



#define SECONDS_PER_MIN (60)
#define SECONDS_PER_HOUR (60*SECONDS_PER_MIN)
#define SECONDS_PER_DAY  (24*SECONDS_PER_HOUR)
#define SECONDS_PER_NORMAL_YEAR (365*SECONDS_PER_DAY) 
#define SECONDS_PER_LEAP_YEAR (SECONDS_PER_NORMAL_YEAR+SECONDS_PER_DAY) 

#define PGLOR_SAT_SV_USED_IN_POSITION 0x4
#define PGLOR_SAT_SV_EPH_NOT_VALID 0x00
#define PGLOR_SAT_SV_EPH_SRC_BE 0x10
#define PGLOR_SAT_SV_EPH_SRC_CBEE 0x20
#define PGLOR_SAT_SV_EPH_SRC_SBEE 0x30

#define RESPONSE_MAXLEN 100
#define ENGINE_SEND_SI_SESSION  0x1
#define ENGINE_SEND_GPS_POSITION 0x2
#define ENGINE_SEND_GPS_MEASUREMENT 0x3

#define SHARKLE_PIKE2_GREENEYE2  (0x00)
#define GREENEYE2  (0x01)

//========================= agps command begin =========================//
#define START_SI         0x01
#define	MSISDN_CMD       0x02
#define	IMSI_CMD         0x03
#define	CID_CMD          0x04
#define	SMS_CMD          0x05
#define WIFI_CMD         0x06
#define MEASURE_CMD      0x07
#define NI_POSITION      0x08
#define AGPS_END         0x09
#define GEO_MSA_TRIGGER  0x0a
#define GEO_TRIGGER      0x10
#define GEO_SESSION_END  0x11
#define AGPS_LTE_CMD     0x12
#define NAVIGATION_CMD   0x13
#define AGPS_NETWORK_CMD 0x14

#define	LAST_CMD         0x20

#define STATUS_OK    0x01
#define STATUS_FAIL  0x00

#define NI_INIT      0x00
#define NI_RUNNING   0x01
#define NI_END       0x02

#define GNSS_POWER_IOCTL_BASE      'z'
#define GNSS_LNA_EN    _IO(GNSS_POWER_IOCTL_BASE, 0x06)
#define GNSS_LNA_DIS   _IO(GNSS_POWER_IOCTL_BASE, 0x07)




//================================ end =================================

typedef enum
{ 
    get_position_init = 0,
	get_position_comming = 1,
	get_position_done = 2
}egetpositionstate;
enum
{
	CMD_INIT  = 0,
	CMD_QUIT  = 0x18,
	CMD_START = 2,
	CMD_STOP  = 3,
	CMD_CLEAN = 4,
	CMD_ENTER = 5,
	CMD_NOTIFICATION = 6,
	CMD_WIFI = 7
};

enum
{
	STATUS_INIT  = 0,
	STATUS_START  = 1,
	STATUS_STOP = 2,
	STATUS_CLEAN  = 3,
};

enum
{
	log_init    = 0,
	log_start   = 1,
	log_stop    = 2,
	log_cleanup = 3,
	log_nothing = 4
};

enum
{
	thread_init = 0,
	thread_run  = 1,
	thread_stop = 2
};


#endif




