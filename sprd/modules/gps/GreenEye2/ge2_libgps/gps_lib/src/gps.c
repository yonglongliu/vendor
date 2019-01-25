///============================================================================
/// Copyright 2012-2014  spreadtrum  --
/// This program be used, duplicated, modified or distributed
/// pursuant to the terms and conditions of the Apache 2 License.
/// ---------------------------------------------------------------------------
/// file gps_lib.c
/// for converting NMEA to Android like APIs
/// ---------------------------------------------------------------------------
/// zhouxw mod 20130920,version 1.00,include test,need add supl so on...
///============================================================================
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <math.h>
#include <cutils/sockets.h>
#include <cutils/properties.h>
#include <termios.h>
#include <hardware_legacy/power.h>
#include <math.h>

//for socket
#include <sys/un.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "gps_cmd.h"
#include "gps_common.h"
#include "gps_engine.h"
#include "gnss_libgps_api.h"
#include "gnssdaemon_client.h"
#include "gnss_log.h"
//if remove debug,mod this

extern const void* gps_get_extension(const char* name);
extern void gps_daemon_thread(void*  arg);


extern int daemon_state;
extern char gps_clear_nmea;
extern CgPositionParam_t gLocationInfo[1];
extern char mode_type;
extern TGNSS_DataProcessor_t gGPSStream;

extern int gPosMethod;

extern unsigned int g_startmode_flg;
extern char  g_llh_time_flg;

int gSUPLRspType = 0;
char thread_status = thread_init; 
char fix_flag = 0;
GpsState  _gps_state[1];
GpsNiNotification gnotification[1];

static IMG_INFO_T gnssImgInfo[2];
COMBINE_INFO_T combineInfo;
char gps_log_enable = 1;

/*Begin: mode related*/
/*GNSS tool test mode:default 0: RF_GPS, 1: RF_GLO, 2: RF_BD*/
enum {
    EUT_RF_GPS=0,
    EUT_RF_GL,
    EUT_RF_BD,
};
enum gps_pc_mode{
	GPS_WARM_START	  = 1,
	GPS_PS_START	  = 100,

	GPS_COLD_START	  = 125,

	GPS_LOG_ENABLE	  = 136,
	GPS_START_SI      = 192,
	GPS_STOP_SI       = 448,
	GPS_START_MOLR    = 449,
	GPS_LOG_DISABLE   = 520,
	GPS_HOT_START	  = 1024,
	GPS_GPS_ONLY      = 4100,
	GPS_GL_ONLY       = 4104,
	GPS_BDS_ONLY      = 4108,
	GPS_GPS_GL        = 4112,
	GPS_GPS_BDS       = 4116,
	GPS_LTE_EN        = 4120,
	GPS_LTE_DIS       = 4128,
	GPS_SET_ASSERT    = 4129,
	GPS_TCXO          = 50001,  /*TCXO stability test*/
	GPS_CW_CN0        = 50002,  /*CW CN0 test*/
	GPS_RF_GPS        = 50003,  /*RF data tool GPS mode*/
	GPS_TSX_TEMP	  = 50004,  /*TSX TEMP test*/
	GPS_NEW_TCXO	  = 50005,  /*TCXO new stability test*/
	GPS_RF_GL    	  = 50006,  /*RF data tool GLONASS mode*/
	GPS_RF_BD         = 50007,  /*RF data tool BEIDOU mode*/

	GPS_FAC_START	  = 65535,  /*factory start*/
};

/*end*/

static char *cold_cmd = "$PCGDC,CS*55\r\n";
static char *warm_cmd = "$PCGDC,WS*55\r\n";
static char *hot_cmd = "$PCGDC,HS*55\r\n";
static char *fac_cmd = "$PCGDC,FAC*55\r\n";
static GpsStatus gstate;
extern int gpsTow;
static int gps_mode = 0;
static GNSSVelocity_t GnssVel;
GNSSPosition_t GnssPos;
static int days_per_month_no_leap[] =
    {31,28,31,30,31,30,31,31,30,31,30,31};
static int days_per_month_leap[] =
    {31,29,31,30,31,30,31,31,30,31,30,31};
static char cer_path[128];
static TSX_TEMP_T tsx_temp_get;

#ifdef GNSS_REG_WR
int parse_register_value(char *buf,unsigned short lenth)
{
	GpsState*	s = _gps_state;
	unsigned int addr, value;

    addr = value = 0;
    if(lenth != 2*sizeof(unsigned int))
    {
        E("the error data length: %ud",lenth);
    }
    else
    {
        memcpy(&addr,buf,sizeof(unsigned int));
        memcpy(&value,buf + sizeof(unsigned int),sizeof(unsigned int));
    }

	D("REG_WR parse_register_value: addr=%x, value=%x\n",addr,value);

	if(addr == s->rRegAddr)
	{
		s->rRegValue = value;
		SignalEvent(&(s->readsync));   // post signal
	}
	
	return 0;
}

int app_write_register(unsigned int addr,unsigned int value)
{
	GpsState*	s = _gps_state;

	D("REG_WR app_write_register: addr=%x, value=%x\n",addr,value);

	s->wRegAddr = addr;
	s->wRegValue = value;
	
	gps_sendData(WRITE_REG,NULL); 
	
	return 0;
}

unsigned int app_read_register(unsigned int addr)
{
	GpsState*	s = _gps_state;
	unsigned int value;

	D("REG_WR app_read_register: addr=%x\n",addr);

	s->rRegAddr = addr;
	s->rRegValue = 0;
	gps_sendData(READ_REG,NULL); 

	WaitEvent(&(s->readsync), 1500);//wait signal

	value = s->rRegValue;

	D("REG_WR app_read_register: addr=%x, value=%x\n",addr,value);

	return value;
}
#endif

void get_tsx_temp(unsigned char *buf, unsigned short length)
{
	GpsState*	s = _gps_state;

	if(length != sizeof(TSX_TEMP_T))
	{
		E("the error data length: %ud", length);
	}
	else
	{
		memcpy(&tsx_temp_get, buf, sizeof(TSX_TEMP_T));
	}
#ifdef GNSS_INTEGRATED
	D("get_tsx_temp: temp=%f, osctemp =%f\n",
	    tsx_temp_get.temprature, tsx_temp_get.temprature_osc);
#else
	D("get_tsx_temp: temp=%f\n", tsx_temp_get.temprature);
#endif
	s->tsx_temp = tsx_temp_get.temprature;
	SignalEvent(&(s->readsync));   // post signal
}

char* engpc_read_tsxtemp()
{
	GpsState*	s = _gps_state;
	int ret = 0;
	int i = 0;
	char* tempbuf;
        int tsxdatalength = 40;

	/*cali mode get temp after gnss worked. just wait*/
	for (i=0; i<18; i++) {
		if(s->powerState == DEVICE_WORKED)
			break;
		usleep(100000);
	}
	memset(&tsx_temp_get, 0, sizeof(TSX_TEMP_T));
	gps_sendData(REQUEST_TSXTEMP, NULL);

	for(i=0; i<4; i++)
	{
		ret = WaitEvent(&(s->readsync), 500);//wait signal
		if(ret == 0)
		{
			break;
		}
	}

	tempbuf = malloc(tsxdatalength);
	if (tempbuf == NULL) {
		D("engpc_read_tsxtemp malloc error\n");
	} else {
		memset(tempbuf, 0, tsxdatalength);
#ifdef GNSS_INTEGRATED
		D("engpc_read_tsxtemp: i= %d, tsxtemp=%f, osc=%f\n",
			i, tsx_temp_get.temprature, tsx_temp_get.temprature_osc);
		sprintf(tempbuf,"%f,OSCTEMP=%f",tsx_temp_get.temprature,
			tsx_temp_get.temprature_osc);
#else
		D("engpc_read_tsxtemp: i= %d, tsxtemp=%f\n", i, tsx_temp_get.temprature);
		sprintf(tempbuf,"%f",tsx_temp_get.temprature);
#endif
	}
	return tempbuf;
}

/*========================here is for cp&up begin============================*/
void clear_agps_data()
{
	int flag = COMMAND;
    GpsState*  s = _gps_state;
	s->cmd_flag = COLD_START_MODE;
	gps_sendData(flag, cold_cmd);
	return;
}

/*========================here is for cp&up begin============================*/
static int is_leap_year(int year)
{
    if ((year%400) == 0)
        return 1;
    if ((year%100) == 0)
        return 0;
    if ((year%4) == 0)
        return 1;
    return 0;
}
static int number_of_leap_years_in_between(int from, int to)
{
    int n_400y, n_100y, n_4y;
    n_400y = to/400 - from/400;
    n_100y = to/100 - from/100;
    n_4y = to/4 - from/4;
    return (n_4y - n_100y + n_400y);
}

static time_t utc_mktime(struct tm *_tm)
{
    time_t t_epoch=0;
    int m; 
    int *days_per_month;
    if (is_leap_year(_tm->tm_year+1900))
        days_per_month = days_per_month_leap;
    else
        days_per_month = days_per_month_no_leap;
    t_epoch += (_tm->tm_year - 70)*SECONDS_PER_NORMAL_YEAR; 
    t_epoch += number_of_leap_years_in_between(1970,_tm->tm_year+1900-1) * SECONDS_PER_DAY;
    for (m=0; m<_tm->tm_mon; m++) {
        t_epoch += days_per_month[m]*SECONDS_PER_DAY;
    }
    t_epoch += (_tm->tm_mday-1)*SECONDS_PER_DAY;
    t_epoch += _tm->tm_hour*SECONDS_PER_HOUR;
    t_epoch += _tm->tm_min*SECONDS_PER_MIN;
    t_epoch += _tm->tm_sec;
    return t_epoch;

}


static int str2int( const char*  p, const char*  end )
{
    int   result = 0;
    int   len    = end - p;

    for ( ; len > 0; len--, p++ )
    {
        int  c;

        if (p >= end)
            goto Fail;

        c = *p - '0';
        if ((unsigned)c >= 10)
            goto Fail;

        result = result*10 + c;
    }
    return  result;

Fail:
    return -1;
}

static int strhex2int( const char*  p, const char*  end )
{
    int   result = 0;
    int   len    = end - p;

    for ( ; len > 0; len--, p++ )
    {
        int  c;

        if (p >= end)
            goto Fail;
        if ((*p >= 'a') && (*p <= 'f'))
            c = *p - 'a' + 10;
        else if ((*p >= 'A') && (*p <= 'F'))
            c = *p - 'A' + 10;
        else if ((*p >= '0') && (*p <= '9'))
            c = *p - '0';
        else
            goto Fail;

        result = result*0x10 + c;
    }
    return  result;

Fail:
    return -1;
}

static double str2float( const char*  p, const char*  end )
{
    int   result = 0;
    int   len    = end - p;
    char  temp[16];

    if (len >= (int)sizeof(temp))
        return 0.;

    memcpy( temp, p, len );
    temp[len] = 0;
    return strtod( temp, NULL );
}

static double convert_from_hhmm( Token  tok )
{
    double  val     = str2float(tok.p, tok.end);
    int     degrees = (int)(floor(val) / 100);
    double  minutes = val - degrees*100.;
    double  dcoord  = degrees + minutes / 60.0;
    return dcoord;
}


static void  nmea_reader_init( NmeaReader*  r )
{
    memset( r, 0, sizeof(*r) );

    r->pos      = 0;
    r->overflow = 0;
    r->utc_year = -1;
    r->utc_mon  = -1;
    r->utc_day  = -1;
    r->location_callback = NULL;
    r->sv_status_callback = NULL;
    r->svinfo_flags = 0;

    r->fix.size = sizeof(GpsLocation);
    r->svstatus.size = sizeof(GpsSvStatus);
#ifdef GNSS_ANDROIDN
	r->gnsssv_status_callback = NULL;
	r->gnss_used_in_fix_nums = 0;
    r->gnss_svstatus.size = sizeof(GnssSvStatus);
#endif 
}

static void nmea_reader_set_callback( NmeaReader*  r, GpsCallbacks  *callbacks)
{
    if(callbacks != NULL)
    {
        r->location_callback = callbacks->location_cb;
        r->sv_status_callback = callbacks->sv_status_cb;
#ifdef GNSS_ANDROIDN
        r->gnsssv_status_callback = callbacks->gnss_sv_status_cb;
#endif
        E("nmea_reader_set_callback is enter,fix is %d sv_status_callback =%p",r->fix.flags,r->sv_status_callback);
        r->fix.flags = 0;
        if (callbacks->location_cb != NULL && r->fix.flags != 0)
        {
            D("%s: sending latest fix to new callback", __FUNCTION__);
            r->location_callback( &r->fix );
            r->fix.flags = 0;
        }
    }
    else
    {
        r->location_callback  = NULL;
        r->sv_status_callback  = NULL;
#ifdef GNSS_ANDROIDN
		r->gnsssv_status_callback = NULL;
#endif 
    }
}

static int nmea_tokenizer_init( NmeaTokenizer*  t, const char*  p, const char*  end )
{
    int    count = 0;

    // the initial '$' is optional
    if (p < end && p[0] == '$')
        p += 1;

    // remove trailing newline
    if (end > p && end[-1] == '\n') {
        end -= 1;
        if (end > p && end[-1] == '\r')
            end -= 1;
    }

    // get rid of checksum at the end of the sentecne
    if (end >= p+3 && end[-3] == '*') {
        end -= 3;
    }

    while (p < end) {
        const char*  q = p;

        q = memchr(p, ',', end-p);
        if (q == NULL)
            q = end;

        if (q >= p) {
            if (count < MAX_NMEA_TOKENS) {
                t->tokens[count].p   = p;
                t->tokens[count].end = q;
                count += 1;
            }
        }
        if (q < end)
            q += 1;

        p = q;
    }

    t->count = count;
    return count;
}

static Token  nmea_tokenizer_get( NmeaTokenizer*  t, int  index )
{
    Token  tok;
    static const char*  dummy = "";

    if (index < 0 || index >= t->count) {
        tok.p = tok.end = dummy;
    } else
        tok = t->tokens[index];

    return tok;
}

static int nmea_reader_update_time( NmeaReader*  r, Token  tok )
{
    int        hour, minute;
    double     seconds;
    struct tm  tm;
    time_t     fix_time;

    if (tok.p + 6 > tok.end)
        return -1;

    if (r->utc_year < 0) {
        // no date yet, get current one
        time_t  now = time(NULL);
        gmtime_r( &now, &tm );
        r->utc_year = tm.tm_year + 1900;
        r->utc_mon  = tm.tm_mon + 1;
        r->utc_day  = tm.tm_mday;
    }

    hour    = str2int(tok.p,   tok.p+2);
    minute  = str2int(tok.p+2, tok.p+4);
    seconds = str2float(tok.p+4, tok.end);

    tm.tm_hour = hour;
    tm.tm_min  = minute;
    tm.tm_sec  = (int) seconds;
    tm.tm_year = r->utc_year - 1900;
    tm.tm_mon  = r->utc_mon - 1;
    tm.tm_mday = r->utc_day;

    fix_time = utc_mktime( &tm );
    r->fix.timestamp = (long long)fix_time * 1000;
//will be update time stamp
    memset(&gLocationInfo->time,0x0,sizeof(TcgTimeStamp));
    gLocationInfo->time.wYear = tm.tm_year + 1900;
    gLocationInfo->time.wMonth = tm.tm_mon + 1;
    gLocationInfo->time.wDay = tm.tm_mday;
    gLocationInfo->time.wHour = tm.tm_hour;
    gLocationInfo->time.wMinute = tm.tm_min;
    gLocationInfo->time.wSecond = tm.tm_sec;		
    return 0;
}

static int nmea_reader_update_latlong( NmeaReader*  r,
     Token  latitude, char   latitudeHemi,Token   longitude, char   longitudeHemi )
{
    double   lat, lon;
    Token    tok;

    tok = latitude;
    if (tok.p + 6 > tok.end) {
        D("latitude is too short: '%.*s'", (int)(tok.end-tok.p), tok.p);
        return -1;
    }
    lat = convert_from_hhmm(tok);
    if (latitudeHemi == 'S')
        lat = -lat;

    tok = longitude;
    if (tok.p + 6 > tok.end) {
        D("longitude is too short: '%.*s'", (int)(tok.end-tok.p), tok.p);
        return -1;
    }
    lon = convert_from_hhmm(tok);
    if (longitudeHemi == 'W')
        lon = -lon;

    r->fix.flags    |= GPS_LOCATION_HAS_LAT_LONG;
    r->fix.latitude  = lat;
    r->fix.longitude = lon;
//will update pos 
    gLocationInfo->pos.latitude = r->fix.latitude;
    gLocationInfo->pos.longitude = r->fix.longitude;

    return 0;
}


static int nmea_reader_update_altitude( NmeaReader*  r,
           Token   altitude)
{
    double  alt;
    Token   tok = altitude;

    if (tok.p >= tok.end)
        return -1;

    r->fix.flags   |= GPS_LOCATION_HAS_ALTITUDE;
    r->fix.altitude = str2float(tok.p, tok.end);
//will update altitue
    gLocationInfo->pos.altitude = r->fix.altitude;
    return 0;
}



static int nmea_reader_update_accuracy ( NmeaReader*  r,
      Token    hdop)
{ 

     r->fix.flags   |= GPS_LOCATION_HAS_ACCURACY;
      //r->fix.accuracy  has assign in the NmeaEncode 
     D("%s the accuracy is %f ",__FUNCTION__,r->fix.accuracy );
 #if 0
    double  alt;
    Token   tok = hdop;

    if (tok.p >= tok.end)
        return -1;

    r->fix.flags   |= GPS_LOCATION_HAS_ACCURACY;
    r->fix.accuracy = str2float(tok.p, tok.end)*10.0;
    srand(time(NULL)); 
    
    if(( r->fix.accuracy > 12) &&( r->fix.accuracy < 60))
    {
        r->fix.accuracy = 6 + rand()%7;
    }
    else if( r->fix.accuracy > 60)
    {
        r->fix.accuracy = 12 + rand()%6;
    }
#endif
    return 0;
}

static int nmea_reader_handle_pstat(NmeaReader *r, NmeaTokenizer *tzer)
{
    GpsState* s = _gps_state;
    int prn;
    float snr;
    int nline;
    Token tok;

	E(" nmea_reader_handle_pstat enter,nmeareader point:%p\r\n",r);

    tok=nmea_tokenizer_get(tzer,1);
    nline = str2int(tok.p, tok.end);
	if(nline == 1)
	{
		memset(&sv_rawinfo, 0, sizeof(RawGpsSvInfo));
	}

    tok=nmea_tokenizer_get(tzer,2);
    prn = str2int(tok.p, tok.end);

    tok=nmea_tokenizer_get(tzer,13);
	snr = str2float(tok.p, tok.end);
	if(sv_rawinfo.num_svs >= 49) // the sv_list array max 50
	{
		E("sv_rawinfo.num_svs too large");
		return 0;
	}
	sv_rawinfo.sv_list[sv_rawinfo.num_svs].prn = prn;
	sv_rawinfo.sv_list[sv_rawinfo.num_svs].snr = snr;
	sv_rawinfo.num_svs++;

    return 0;
}

static int nmea_reader_handle_gsv(NmeaReader *r, NmeaTokenizer *tzer)
{
    int lines;
    int nline;
    int nsat;
	int i = 0;
    Token tok;
    GpsSvInfo  *svinfo;
#ifdef GNSS_ANDROIDN
    GnssSvInfo  *gnss_svinfo;
#endif
	
	//D(" nmea_reader_handle_gsv enter\r\n");
    
    tok=nmea_tokenizer_get(tzer,1);
    lines = str2int(tok.p, tok.end);

    tok=nmea_tokenizer_get(tzer,2);
    nline = str2int(tok.p, tok.end);

    tok=nmea_tokenizer_get(tzer,3);
    if (nline == 1)
    {
        r->svstatus.num_svs = 0;
#ifdef GNSS_ANDROIDN
		r->gnss_svstatus.num_svs = 0;
#endif
    }

    for (nsat=1; nsat <= 4;nsat++) {
            tok=nmea_tokenizer_get(tzer,4*nsat);
            if (!TOKEN_LEN(tok))
                continue;
            //if (str2int(tok.p,tok.end)>32)
            //    continue;
			if(r->svstatus.num_svs >= GPS_MAX_SVS)
			{
				E("svstatus num over or equal %d",GPS_MAX_SVS);
				break;
			}
            svinfo=&r->svstatus.sv_list[r->svstatus.num_svs++];
            svinfo->size = sizeof(GpsSvInfo);
            svinfo->prn=str2int(tok.p,tok.end);
            tok=nmea_tokenizer_get(tzer,4*nsat+1);
            svinfo->elevation=str2float(tok.p,tok.end);
            tok=nmea_tokenizer_get(tzer,4*nsat+2);
            svinfo->azimuth=str2float(tok.p,tok.end);
            tok=nmea_tokenizer_get(tzer,4*nsat+3);
            svinfo->snr=str2float(tok.p,tok.end);
#ifndef GNSS_ANDROIDN         
            if(sv_rawinfo.num_svs >= 1)
			{
				for(i = 0;i<sv_rawinfo.num_svs;i++)
				{
					if(svinfo->prn == sv_rawinfo.sv_list[i].prn)
					{
						svinfo->snr = sv_rawinfo.sv_list[i].snr;
						break;
					}
				}
			}
#endif//GNSS_ANDROIDN 
    }

#ifdef GNSS_ANDROIDN
	// GNSS
	for(nsat=1; nsat <= 4;nsat++)
	{
		tok=nmea_tokenizer_get(tzer,4*nsat);
		if (!TOKEN_LEN(tok))
			continue;

		gnss_svinfo = &r->gnss_svstatus.gnss_sv_list[r->gnss_svstatus.num_svs++];
		gnss_svinfo->size = sizeof(GnssSvInfo);
		gnss_svinfo->svid = str2int(tok.p,tok.end);
		tok = nmea_tokenizer_get(tzer,4*nsat+1);
		gnss_svinfo->elevation = str2float(tok.p,tok.end);
		tok = nmea_tokenizer_get(tzer,4*nsat+2);
		gnss_svinfo->azimuth = str2float(tok.p,tok.end);
		tok = nmea_tokenizer_get(tzer,4*nsat+3);
		gnss_svinfo->c_n0_dbhz = str2float(tok.p,tok.end);
		gnss_svinfo->constellation = GNSS_CONSTELLATION_GPS;  // GPS constellation

		for(i=0;i<r->gnss_used_in_fix_nums;i++)
		{
			if(gnss_svinfo->svid == r->gnss_used_in_fix_mask[i])
			{
				gnss_svinfo->flags = GNSS_SV_FLAGS_HAS_EPHEMERIS_DATA | GNSS_SV_FLAGS_HAS_ALMANAC_DATA | GNSS_SV_FLAGS_USED_IN_FIX;
				break;
			}
		}

		if(i >= r->gnss_used_in_fix_nums)
		{
			if(gnss_svinfo->c_n0_dbhz != 0)
			{
				gnss_svinfo->flags = GNSS_SV_FLAGS_HAS_EPHEMERIS_DATA | GNSS_SV_FLAGS_HAS_ALMANAC_DATA;
			}
			else
			{
				gnss_svinfo->flags = GNSS_SV_FLAGS_NONE;
			}
		}
	}
#endif //GNSS_ANDROIDN
#if 0
    if (lines == nline)
    {
        r->svinfo_flags |= SVINFO_GOT_SV_INFO_FLAG;
         E("set SVINFO_GOT_SV_INFO_FLAG &&&");
     }
 #endif 
    return 0;
}

static int nmea_reader_handle_glgsv(NmeaReader *r, NmeaTokenizer *tzer)
{
    int lines;
    int nline;
    int nsat;
    Token tok;
	int i = 0;
    GpsSvInfo  *svinfo;
#ifdef GNSS_ANDROIDN
    GnssSvInfo  *gnss_svinfo;
#endif
  
	if(r->svstatus.num_svs >= GPS_MAX_SVS)
	{
		E("num svs over or equal %d",GPS_MAX_SVS);
		return 0;
	}

    tok=nmea_tokenizer_get(tzer,1);
    lines = str2int(tok.p, tok.end);

    tok=nmea_tokenizer_get(tzer,2);
    nline = str2int(tok.p, tok.end);
	
    tok=nmea_tokenizer_get(tzer,3);

    // GpsSvStatus
    for (nsat=1; nsat <= 4;nsat++) {
            tok=nmea_tokenizer_get(tzer,4*nsat);
            if (!TOKEN_LEN(tok))
                continue;
            //if (str2int(tok.p,tok.end)>32)
               // continue;
			if(r->svstatus.num_svs >= GPS_MAX_SVS)
			{
				E("svstatus over or equal num %d",GPS_MAX_SVS);
				break;
			}
            svinfo=&r->svstatus.sv_list[r->svstatus.num_svs++];
            svinfo->size = sizeof(GpsSvInfo);
            svinfo->prn=str2int(tok.p,tok.end);
            tok=nmea_tokenizer_get(tzer,4*nsat+1);
            svinfo->elevation=str2float(tok.p,tok.end);
            tok=nmea_tokenizer_get(tzer,4*nsat+2);
            svinfo->azimuth=str2float(tok.p,tok.end);
            tok=nmea_tokenizer_get(tzer,4*nsat+3);
            svinfo->snr=str2float(tok.p,tok.end);
#ifndef GNSS_ANDROIDN      
            if(sv_rawinfo.num_svs >= 1)
			{
				for(i = 0;i<sv_rawinfo.num_svs;i++)
				{
					if(svinfo->prn == sv_rawinfo.sv_list[i].prn)
					{
						svinfo->snr = sv_rawinfo.sv_list[i].snr;
						break;
					}
				}
			}
#endif //GNSS_ANDROIDN 
            
    }

#ifdef GNSS_ANDROIDN  
    // GnssSvStatus
	for(nsat=1; nsat <= 4;nsat++)
	{
		tok=nmea_tokenizer_get(tzer,4*nsat);
		if (!TOKEN_LEN(tok))
			continue;
		gnss_svinfo = &r->gnss_svstatus.gnss_sv_list[r->gnss_svstatus.num_svs++];
		gnss_svinfo->size = sizeof(GnssSvInfo);
		gnss_svinfo->svid = str2int(tok.p,tok.end);
		tok = nmea_tokenizer_get(tzer,4*nsat+1);
		gnss_svinfo->elevation = str2float(tok.p,tok.end);
		tok = nmea_tokenizer_get(tzer,4*nsat+2);
		gnss_svinfo->azimuth = str2float(tok.p,tok.end);
		tok = nmea_tokenizer_get(tzer,4*nsat+3);
		gnss_svinfo->c_n0_dbhz = str2float(tok.p,tok.end);
		gnss_svinfo->constellation = GNSS_CONSTELLATION_GLONASS;  // GLONASS constellation

		for(i=0;i<r->gnss_used_in_fix_nums;i++)
		{
			if(gnss_svinfo->svid == r->gnss_used_in_fix_mask[i])
			{
				gnss_svinfo->flags = GNSS_SV_FLAGS_HAS_EPHEMERIS_DATA | GNSS_SV_FLAGS_HAS_ALMANAC_DATA | GNSS_SV_FLAGS_USED_IN_FIX;
				break;
			}
		}

		if(i >= r->gnss_used_in_fix_nums)
		{
			if(gnss_svinfo->c_n0_dbhz != 0)
			{
				gnss_svinfo->flags = GNSS_SV_FLAGS_HAS_EPHEMERIS_DATA | GNSS_SV_FLAGS_HAS_ALMANAC_DATA;
			}
			else
			{
				gnss_svinfo->flags = GNSS_SV_FLAGS_NONE;
			}
		}
	}
#endif //GNSS_ANDROIDN 

#if 0
    if (lines == nline)
    {
        r->svinfo_flags |= SVINFO_GOT_SV_INFO_FLAG;
         E("set SVINFO_GOT_SV_INFO_FLAG &&&");
     }
#endif 
    return 0;
}



static int nmea_reader_handle_bdgsv(NmeaReader *r, NmeaTokenizer *tzer)
{
    int lines;
    int nline;
    int nsat;
    Token tok;
	int i = 0;
    GpsSvInfo  *svinfo;
#ifdef GNSS_ANDROIDN
    GnssSvInfo *gnss_svinfo;
#endif

	if(r->svstatus.num_svs >= GPS_MAX_SVS)
	{
		E("num bds svs over or equal %d",GPS_MAX_SVS);
		return 0;
	}    

    tok=nmea_tokenizer_get(tzer,1);
    lines = str2int(tok.p, tok.end);

    tok=nmea_tokenizer_get(tzer,2);
    nline = str2int(tok.p, tok.end);
	

    tok=nmea_tokenizer_get(tzer,3);

	// GpsSvStatus, deprecated, to be removed in the next Android release
    for (nsat=1; nsat <= 4;nsat++) {
            
            tok=nmea_tokenizer_get(tzer,4*nsat);
            if (!TOKEN_LEN(tok))
                continue;
            //prevent svstatus 
			if(r->svstatus.num_svs >= GPS_MAX_SVS)
			{
				E("svstatus num over or equal %d",GPS_MAX_SVS);
				break;
			}
            svinfo=&r->svstatus.sv_list[r->svstatus.num_svs++];
            svinfo->size = sizeof(GpsSvInfo);
            svinfo->prn=str2int(tok.p,tok.end);
            tok=nmea_tokenizer_get(tzer,4*nsat+1);
            svinfo->elevation=str2float(tok.p,tok.end);
            tok=nmea_tokenizer_get(tzer,4*nsat+2);
            svinfo->azimuth=str2float(tok.p,tok.end);
            tok=nmea_tokenizer_get(tzer,4*nsat+3);
            svinfo->snr=str2float(tok.p,tok.end);
#ifndef GNSS_ANDROIDN
            if(sv_rawinfo.num_svs >= 1)
			{
				for(i = 0;i<sv_rawinfo.num_svs;i++)
				{
					if(svinfo->prn == sv_rawinfo.sv_list[i].prn)
					{
						svinfo->snr = sv_rawinfo.sv_list[i].snr;
						break;
					}
				}
			}
#endif //GNSS_ANDROIDN

    }
#ifdef GNSS_ANDROIDN
	// GnssSvStatus
    for (nsat=1; nsat <= 4;nsat++)
	{
		tok = nmea_tokenizer_get(tzer,4*nsat);
		if (!TOKEN_LEN(tok))
			continue;

		gnss_svinfo = &r->gnss_svstatus.gnss_sv_list[r->gnss_svstatus.num_svs++];
		gnss_svinfo->size = sizeof(GnssSvInfo);
		gnss_svinfo->svid = str2int(tok.p,tok.end);
		tok = nmea_tokenizer_get(tzer,4*nsat+1);
		gnss_svinfo->elevation = str2float(tok.p,tok.end);
		tok = nmea_tokenizer_get(tzer,4*nsat+2);
		gnss_svinfo->azimuth = str2float(tok.p,tok.end);
		tok = nmea_tokenizer_get(tzer,4*nsat+3);
		gnss_svinfo->c_n0_dbhz = str2float(tok.p,tok.end);
		gnss_svinfo->constellation = GNSS_CONSTELLATION_BEIDOU;  // Beidou constellation

		for(i=0;i<r->gnss_used_in_fix_nums;i++)
		{
			if(gnss_svinfo->svid == r->gnss_used_in_fix_mask[i])
			{
				gnss_svinfo->flags = GNSS_SV_FLAGS_HAS_EPHEMERIS_DATA | GNSS_SV_FLAGS_HAS_ALMANAC_DATA | GNSS_SV_FLAGS_USED_IN_FIX;
				break;
			}
		}

		if(i >= r->gnss_used_in_fix_nums)
		{
			if(gnss_svinfo->c_n0_dbhz != 0)
			{
				gnss_svinfo->flags = GNSS_SV_FLAGS_HAS_EPHEMERIS_DATA | GNSS_SV_FLAGS_HAS_ALMANAC_DATA;
			}
			else
			{
				gnss_svinfo->flags = GNSS_SV_FLAGS_NONE;
			}
		}

    }
#endif //GNSS_ANDROIDN
    return 0;
}

static int nmea_reader_update_bearing( NmeaReader*  r, Token  bearing )
{
    double  alt;
    Token   tok = bearing;

    if (tok.p >= tok.end)
        return -1;

    r->fix.flags   |= GPS_LOCATION_HAS_BEARING;
    r->fix.bearing  = str2float(tok.p, tok.end);
//update bearing
    gLocationInfo->velocity.bearing = r->fix.bearing;
    return 0;
}


static int nmea_reader_update_date( NmeaReader*  r, Token  date, Token  time )
{
    Token  tok = date;
    int    day, mon, year;

    if (tok.p + 6 != tok.end) {
        D("date not properly formatted: '%.*s'", (int)(tok.end-tok.p), tok.p);
        return -1;
    }
    day  = str2int(tok.p, tok.p+2);
    mon  = str2int(tok.p+2, tok.p+4);
    year = str2int(tok.p+4, tok.p+6) + 2000;

    if ((day|mon|year) < 0) {
        D("date not properly formatted: '%.*s'", (int)(tok.end-tok.p), tok.p);
        return -1;
    }

    r->utc_year  = year;
    r->utc_mon   = mon;
    r->utc_day   = day;

    return nmea_reader_update_time( r, time );
}

static int nmea_reader_update_speed( NmeaReader* r, Token speed )
{
    double  alt;
    Token   tok = speed;

    if (tok.p >= tok.end)
        return -1;

    r->fix.flags   |= GPS_LOCATION_HAS_SPEED;
    r->fix.speed    = str2float(tok.p, tok.end);
//update vec horizon
    gLocationInfo->velocity.horizontalSpeed = r->fix.speed;
    return 0;
}




static int  nmea_reader_handle_pglor_sat(NmeaReader *r, NmeaTokenizer *tzer)
{
    Token tok;
    int indx;

    for (indx=2;indx<tzer->count;indx+=3) {
        int prn;
        uint32_t sv_flags;
        uint32_t sv_mask;
        tok=nmea_tokenizer_get(tzer,indx);
        if (!TOKEN_LEN(tok))
            continue;
        prn = str2int(tok.p,tok.end);
        D("prn str(%.*s) d(%d)",(int)(tok.end-tok.p),tok.p,prn);
        if (prn > 32)
            continue;
        sv_mask = 1 << (prn-1);
        tok=nmea_tokenizer_get(tzer,indx+2);
        sv_flags = strhex2int(tok.p,tok.end);
        D("svflags str(%.*s) d(%X)",(int)(tok.end-tok.p),tok.p,sv_flags);
        switch(sv_flags&0x30) {
            case PGLOR_SAT_SV_EPH_SRC_BE:
            case PGLOR_SAT_SV_EPH_SRC_CBEE:
            case PGLOR_SAT_SV_EPH_SRC_SBEE:
               // r->svstatus.ephemeris_mask |= sv_mask;
                break;
            default:
                break;
        }
    }
    r->svinfo_flags |= SVINFO_GOT_SV_USED_FLAG;
    r->svinfo_flags |= SVINFO_GOT_EPH_INFO_FLAG;
	D("Set svinfo_flags SV_USED_FLAG & EPH_INFO_FLAG\r\n");
    return 0;
}

static int nmea_reader_handle_gsa(NmeaReader *r, NmeaTokenizer *tzer)
{
    int i,prn;
    Token tok_fixStatus,tok_accuracy,  tok_prn;

    tok_fixStatus   = nmea_tokenizer_get(tzer, 2);

    if ((tok_fixStatus.p[0] != '\0' && tok_fixStatus.p[0] != '1') && (fix_flag == 1))
    {
        tok_accuracy = nmea_tokenizer_get(tzer, 15);
        nmea_reader_update_accuracy(r, tok_accuracy);   

        for(i = 3; i <=14; i++)
        {
			tok_prn  = nmea_tokenizer_get(tzer, i);
			prn = str2int(tok_prn.p, tok_prn.end);
			r->svstatus.used_in_fix_mask |= (1 << (prn-1));
#ifdef GNSS_ANDROIDN
			r->gnss_used_in_fix_mask[r->gnss_used_in_fix_nums] = prn;
			r->gnss_used_in_fix_nums++;
#endif 
        }
    }
	else
	{
		memset(&(r->svstatus.used_in_fix_mask),0,sizeof(r->svstatus.used_in_fix_mask));  
		memset(&(r->svstatus.ephemeris_mask),0,sizeof(r->svstatus.ephemeris_mask)); 
		memset(&(r->svstatus.almanac_mask),0,sizeof(r->svstatus.almanac_mask));
	}
    return 0;
}



static int msa_update_latlong( NmeaReader*  r,
     Token  latitude, char   latitudeHemi,Token   longitude, char   longitudeHemi )
{
    double   lat, lon;
    Token    tok;

    tok = latitude;
    lat = str2float(latitude.p,latitude.end);
    if (latitudeHemi == 'S')
        lat = -lat;

    tok = longitude;
    if (tok.p + 6 > tok.end) {
        D("longitude is too short: '%.*s'", (int)(tok.end-tok.p), tok.p);
        return -1;
    }
    lon = str2float(longitude.p,longitude.end);
    if (longitudeHemi == 'W')
        lon = -lon;

    r->fix.flags    |= GPS_LOCATION_HAS_LAT_LONG;
    r->fix.latitude  = lat;
    r->fix.longitude = lon;
    return 0;
}


static void parse_gga_token(NmeaTokenizer *tzer,NmeaReader*  r)
{
    Token  tok_time          = nmea_tokenizer_get(tzer,1);
    Token  tok_latitude      = nmea_tokenizer_get(tzer,2);
    Token  tok_latitudeHemi  = nmea_tokenizer_get(tzer,3);
    Token  tok_longitude     = nmea_tokenizer_get(tzer,4);
    Token  tok_longitudeHemi = nmea_tokenizer_get(tzer,5);
    Token  tok_accuracy      = nmea_tokenizer_get(tzer,8);
    Token  tok_altitude      = nmea_tokenizer_get(tzer,9);

    nmea_reader_update_time(r, tok_time);
    nmea_reader_update_latlong(r, tok_latitude,
                              tok_latitudeHemi.p[0],
                              tok_longitude,
                              tok_longitudeHemi.p[0]);
    nmea_reader_update_altitude(r, tok_altitude);
    nmea_reader_update_accuracy(r, tok_accuracy);
}
//char data_trig = 0; 
//extern void SUPLReady(int *sid,char* rspType);

void transGNSSPos(TCgAgpsPosition *pPos,GNSSPosition_t *pPosOut)
{
    GNSSEllipsoidPointWithAltitudeAndUncertaintyEllipsoid_t *pEll;
    pPosOut->posEstimate.coordType = GNSSLocationCoordinatesType_ellipsoidPointWithAltitudeAndUncertaintyEllipsoid;
    pEll =  &(pPosOut->posEstimate.choice.ellPointAltitudeEllips);
    if(pPos->latitude < 0)
    {
        pEll->latitudeSign = GNSSLatitudeSign_south;
    }
	else
	{
        pEll->latitudeSign = GNSSLatitudeSign_north;
	}
    pEll->degreesLatitude = ((fabs(pPos->latitude) / 90) * 0x800000);
    pEll->degreesLongitude = (pPos->longitude / 360) * 0x1000000;
    pEll->altitude = abs(pPos->altitude);
    if(pPos->latitude < 0)
    {
        pEll->altitudeDirection = GNSSAltitudeDirection_depth;

    }
	else
	{
        pEll->altitudeDirection = GNSSAltitudeDirection_height;
	}
	D("latitudesign is %d,altitudedirection is %d",pEll->latitudeSign,pEll->altitudeDirection);
	if((agpsdata->uncertaintySemiMajor > 0) && (agpsdata->uncertaintySemiMajor < 127))
	{
    	pEll->uncertaintySemiMajor = agpsdata->uncertaintySemiMajor;
	}
	else if(agpsdata->uncertaintySemiMajor <= 0)
	{
		pEll->uncertaintySemiMajor = 1;
	}
	else if(agpsdata->uncertaintySemiMajor >= 127)
	{
		pEll->uncertaintySemiMajor = 126;
	}

	if((agpsdata->uncertaintySemiMinor > 0) && (agpsdata->uncertaintySemiMinor < 127))
	{
    	pEll->uncertaintySemiMinor = agpsdata->uncertaintySemiMinor;
	}
	else if(agpsdata->uncertaintySemiMinor <= 0)
	{
		pEll->uncertaintySemiMinor = 1;
	}
	else if(agpsdata->uncertaintySemiMinor >= 127)
	{
		pEll->uncertaintySemiMinor = 126;
	}


	if((agpsdata->orientationMajorAxis > 0) && (agpsdata->orientationMajorAxis < 89))
	{
    	pEll->orientationMajorAxis = agpsdata->orientationMajorAxis;
	}
	else if(agpsdata->orientationMajorAxis <= 0)
	{
		pEll->orientationMajorAxis = 1;
	}
	else if(agpsdata->orientationMajorAxis >= 89)
	{
		pEll->orientationMajorAxis = 88;
	}

	if((agpsdata->uncertaintyAltitude > 0) && (agpsdata->uncertaintyAltitude < 127))
	{
    	pEll->uncertaintyAltitude = agpsdata->uncertaintyAltitude;
	}
	else if(agpsdata->uncertaintyAltitude <= 0)
	{
		pEll->uncertaintyAltitude = 1;
	}
	else if(agpsdata->uncertaintyAltitude >= 127)
	{
		pEll->uncertaintyAltitude = 126;
	}
    pEll->confidence = 68;           
}
static int parse_rmc_token(NmeaTokenizer *tzer,NmeaReader*  r)
{
	int ret = 0;
	int result = 0;
	int sid = 0; 
	GpsState *s = _gps_state;

	Token  tok_time          = nmea_tokenizer_get(tzer,1);
	Token  tok_fixStatus     = nmea_tokenizer_get(tzer,2);
	Token  tok_latitude      = nmea_tokenizer_get(tzer,3);
	Token  tok_latitudeHemi  = nmea_tokenizer_get(tzer,4);
	Token  tok_longitude     = nmea_tokenizer_get(tzer,5);
	Token  tok_longitudeHemi = nmea_tokenizer_get(tzer,6);
	Token  tok_speed         = nmea_tokenizer_get(tzer,7);
	Token  tok_bearing       = nmea_tokenizer_get(tzer,8);
	Token  tok_date          = nmea_tokenizer_get(tzer,9);

	memset(&(r->svstatus.used_in_fix_mask),0,sizeof(r->svstatus.used_in_fix_mask));  
	memset(&(r->svstatus.ephemeris_mask),0,sizeof(r->svstatus.ephemeris_mask)); 
	memset(&(r->svstatus.almanac_mask),0,sizeof(r->svstatus.almanac_mask));
#ifdef GNSS_ANDROIDN
	memset(r->gnss_used_in_fix_mask, 0, 32*sizeof(int));
	r->gnss_used_in_fix_nums = 0;
#endif 
		
     D("in RMC, fixStatus=%c", tok_fixStatus.p[0]);
     if (tok_fixStatus.p[0] == 'A')
     {

         nmea_reader_update_date( r, tok_date, tok_time );

         nmea_reader_update_latlong( r, tok_latitude,
                                   tok_latitudeHemi.p[0],
                                   tok_longitude,
                                   tok_longitudeHemi.p[0] );

         nmea_reader_update_bearing( r, tok_bearing );
         nmea_reader_update_speed  ( r, tok_speed );
         fix_flag = 1;
         //D("fix lat is %f,long is %f\n",r->fix.latitude,r->fix.longitude);
		 sid = agpsdata->readySid;

		if(s->cmcc == 1)
		{
			D("sid %d,mode_type %d,curstate:%d",sid,mode_type,agpsdata->cur_state);
		}
         if((sid != 0) && (mode_type < 2) && (agpsdata->cur_state < 10))
         {
              E("begin to call location loc");
              GnssVel.velType = GNSSVelocityType_horizontalWithVerticalVelocityAndUncertainty;
              GnssVel.choice.VelUncertainty.bearing = (long)gLocationInfo->velocity.bearing;
              GnssVel.choice.VelUncertainty.horizontalSpeed = (long)gLocationInfo->velocity.horizontalSpeed;
              GnssVel.choice.VelUncertainty.verticalDirection = gLocationInfo->velocity.verticalDirect;
              GnssVel.choice.VelUncertainty.verticalSpeed = (long)gLocationInfo->velocity.verticalSpeed;
              GnssVel.choice.VelUncertainty.horizontalUncertaintySpeed = 0;
              GnssVel.choice.VelUncertainty.verticalUncertaintySpeed = 0;
              GnssPos.positionData = agpsdata->gnssidReq;
              transGNSSPos(&gLocationInfo->pos,&GnssPos);
			  GnssPos.posTime.gnssTOD = gpsTow;
			  GnssPos.posTime.gnssTimeID = GNSSTimeID_GPS;
#ifdef GNSS_LCS
              result = gnss_lcsInjectPos(sid,LCS_DOMAIN_UP,&GnssPos,&GnssVel,gpsTow);
#else
	          result = SPRD_gnssLocationInfo(sid, &GnssPos, &GnssVel);
#endif             
              if(0 != result)
              {
                 E(" gnss userplan location  error happen");
              }
              if(agpsdata->ni_status && (sid == agpsdata->ni_sid))
               {
                  agpsdata->ni_sid = 0;
               }
               else if(sid == agpsdata->si_sid)
               {
                  agpsdata->si_sid = 0;
               }
    
              mode_type = 0;
              agpsdata->readySid = 0;
         }
         if((sid != 0) && (mode_type < 2) && (agpsdata->cur_state >= 10))
         {
              E("begin to call location loc");
              GnssVel.velType = GNSSVelocityType_horizontalWithVerticalVelocityAndUncertainty;
              GnssVel.choice.VelUncertainty.bearing = (long)gLocationInfo->velocity.bearing;
              GnssVel.choice.VelUncertainty.horizontalSpeed = (long)gLocationInfo->velocity.horizontalSpeed;
              GnssVel.choice.VelUncertainty.verticalDirection = gLocationInfo->velocity.verticalDirect;
              GnssVel.choice.VelUncertainty.verticalSpeed = (long)gLocationInfo->velocity.verticalSpeed;
              GnssVel.choice.VelUncertainty.horizontalUncertaintySpeed = 0;
              GnssVel.choice.VelUncertainty.verticalUncertaintySpeed = 0;
              GnssPos.positionData = agpsdata->gnssidReq;
              transGNSSPos(&gLocationInfo->pos,&GnssPos);
			  GnssPos.posTime.gnssTOD = gpsTow;
			  GnssPos.posTime.gnssTimeID = GNSSTimeID_GPS;
#ifdef GNSS_LCS
			  result = gnss_lcsInjectPos(sid,LCS_DOMAIN_CP,&GnssPos,&GnssVel,gpsTow);
#else
              result = ctrlplane_SPRD_gnssLocationInfo(sid,&GnssPos, &GnssVel,gpsTow);
#endif 		
              if(0 != result)
              {
                 E("gnss control plan location error happen");
              }
              if(agpsdata->ni_status)
               {
                  agpsdata->ni_sid = 0;
               }
               else
               {
                  agpsdata->si_sid = 0;
               }
              mode_type = 0;
			  agpsdata->readySid = 0;
         }

     }
    return ret;
}

static void parse_gds_token(NmeaTokenizer *tzer,NmeaReader*  r)
{
    if(gSUPLRspType != RSP_MSA) return;
    Token  mtok_msa = nmea_tokenizer_get(tzer,1);
    if(memcmp(mtok_msa.p,"MSA",3)) return;

    Token  mtok_latitude = nmea_tokenizer_get(tzer,2);
    Token  mtok_latitudeHemi = nmea_tokenizer_get(tzer,3);
    Token  mtok_longitude = nmea_tokenizer_get(tzer,4);
    Token  mtok_longitudeHemi = nmea_tokenizer_get(tzer,5);
    Token  maltitude         = nmea_tokenizer_get(tzer,6);
    Token  mtok_speed = nmea_tokenizer_get(tzer,7);
    Token  mtok_bearing = nmea_tokenizer_get(tzer,8);

    D("msa is enter now\n");
    msa_update_latlong( r, mtok_latitude,
    mtok_latitudeHemi.p[0],
    mtok_longitude,
    mtok_longitudeHemi.p[0] );
    r->fix.flags   |= GPS_LOCATION_HAS_ALTITUDE;
    r->fix.altitude = str2float(maltitude.p, maltitude.end);
    nmea_reader_update_bearing( r, mtok_bearing );
    nmea_reader_update_speed  ( r, mtok_speed );
    D("MSA lat is %f,long is %f,altitude is %f,bearing is %f,speed is %f\n",
    r->fix.latitude,r->fix.longitude,r->fix.altitude,r->fix.bearing,r->fix.speed);

    r->location_callback( &r->fix );
    r->location_callback( &r->fix );
    r->location_callback( &r->fix );

}
static void parse_lor_token(NmeaTokenizer *tzer,NmeaReader*  r)
{
     Token tok;
     tok = nmea_tokenizer_get(tzer,1);
     D("sentence id $PGLOR,%.*s ", (int)(tok.end-tok.p), tok.p);
     if (TOKEN_LEN(tok) >= 3) {
         if (!memcmp(tok.p,"SAT",3) )
             nmea_reader_handle_pglor_sat(r, tzer);
     }
}

void  nmea_reader_parse( NmeaReader*  r )
{
    NmeaTokenizer  tzer[1];
    Token          tok;
    GpsState*  s = _gps_state;    
    int nmea_end = 0;
    int sum_check = 0;

    if (r->pos < 9) {
        E("nmea lenth below 9,discarded.");
        return;
    }

	//===================nmea log read=====================//
	if(!((r->in[0] == '$') && (r->in[1] == 'P') && (r->in[2] == 'S')))
	{
		//s->callbacks.nmea_cb(r->fix.timestamp,r->in,r->pos);
		savenmealog2Buffer(r->in,r->pos);
	}
	//====================end===========================//
	//if((r->in[0] != '$') || ((r->in[1] != 'G') && (r->in[1] != 'B')))
	if((r->in[0] != '$') || ((r->in[1] != 'G') && (r->in[1] != 'B') && (r->in[1] != 'P') && ((r->in[1] == 'P')||(r->in[2] != 'S'))))
	{
		//E("int nmea log,token '$G' or '$B' not found");
		return;
	}
    nmea_tokenizer_init(tzer, r->in, r->in + r->pos);

    tok = nmea_tokenizer_get(tzer, 0);
    if (tok.p + 5 > tok.end) {
        D("sentence id '%.*s' too short, ignored.", (int)(tok.end-tok.p), tok.p);
        return;
    }
	//E("&&&&& p0= %c, p1= %c \r\n",*(tok.p),*(tok.p+1));

    tok.p += 2;

    sum_check = *tok.p + *(tok.p + 1) + *(tok.p + 2);
    /*-------------------------------------
		GGA is 207
		GSV is 240
		GSA is 219
		RMC is 226
		WER is 238
		GDS is 222
		CMD is 212
		LOR is 237
		log is 322
		DMP is 225
    -------------------------------------*/
    switch(sum_check)
    {
        case ('G'+'G'+'A'):
            parse_gga_token(tzer,r);
            break;
        case ('G'+'S'+'A'):
            nmea_reader_handle_gsa(r,tzer);
            break;
        case ('G'+'S'+'V'):
			if(!memcmp(r->in,"$GLGSV",6))
			{				
				nmea_reader_handle_glgsv(r, tzer);
				break;
			}
			if(!memcmp(r->in,"$BDGSV",6))
			{
				nmea_reader_handle_bdgsv(r, tzer);
				break;
			}
            nmea_reader_handle_gsv(r, tzer);
            break;
        case ('R'+'M'+'C'):
 			 r->svstatus.num_svs = 0;   //clear svstatus
#ifdef GNSS_ANDROIDN
			 r->gnss_svstatus.num_svs = 0;
#endif 
             parse_rmc_token(tzer,r);
             break;
        case ('G'+'D'+'S'):
            parse_gds_token(tzer,r);
            break;
        case ('L'+'O'+'R'):
            parse_lor_token(tzer,r);
            break;
        case ('V'+'T'+'G'):
            nmea_end = 1;
			r->svinfo_flags |= SVINFO_GOT_SV_INFO_FLAG;
            break;
		case ('T'+'A'+'T'):
			if(!memcmp(r->in,"$PSTAT",6))
			{
				nmea_reader_handle_pstat(r, tzer);
			}
			break;
        default:
            break;
    }
	if(s->ref == 0)
	{
		D("ref==0 not location callback");
		return;
	}
    if(s->logswitch)	
   {
        D("r->svinfo_flags = 0x%x, \r\n",r->svinfo_flags);
    }
    if ((r->svinfo_flags & SVINFO_GOT_EPH_INFO_FLAG) ||
            (r->svinfo_flags & SVINFO_GOT_SV_INFO_FLAG) ||
            (r->svinfo_flags & SVINFO_GOT_SV_USED_FLAG)) {

#ifndef GNSS_ANDROIDN
        if (r->sv_status_callback) {
            D(" svstatus size =%zu, numsvs =%d",r->svstatus.size, r->svstatus.num_svs);
            r->sv_status_callback( &r->svstatus );
            r->svinfo_flags = 0;
			memset(&sv_rawinfo, 0, sizeof(RawGpsSvInfo));
        }
    
#else 
		// gnss_sv_status_callback
		if (r->gnsssv_status_callback) {
			D("gnss_svstatus num_svs=%d", r->gnss_svstatus.num_svs);
			r->gnsssv_status_callback(&r->gnss_svstatus);
			r->svinfo_flags = 0;
		}
#endif //GNSS_ANDROIDN        
    }
    if((nmea_end == 1) && (r->fix.flags & GPS_LOCATION_HAS_LAT_LONG) &&
           (r->fix.flags & GPS_LOCATION_HAS_ALTITUDE) &&
           (r->fix.flags & GPS_LOCATION_HAS_SPEED) &&
           (r->fix.flags & GPS_LOCATION_HAS_BEARING)) {

	    D("callback flag is %d",fix_flag);
        if ((r->location_callback) && (fix_flag == 1)) 
        {	
            r->fix.speed = r->fix.speed*0.51417;
            if(s->logswitch)	
            {   
                D("nmea_reader_parse [gSUPLRspType:%d][gPosMethod:%d]",gSUPLRspType,gPosMethod);
            }
            r->location_callback( &r->fix );
            r->fix.flags = 0;            
        }
        else {
            D("no callback, keeping data until needed !");
        }

    }

}

 void  nmea_reader_addc( NmeaReader*  r, int  c )
{
    if (r->overflow) {
        r->overflow = (c != '\n');
        return;
    }

    if (r->pos >= (int) sizeof(r->in)-1 ) {
        r->overflow = 1;
        r->pos      = 0;
        return;
    }

    r->in[r->pos] = (char)c;
    r->pos       += 1;

    if ((r->pos > 2) && (c == '\n') && (r->in[r->pos-2] == '\r')){
        nmea_reader_parse(r);
        r->pos = 0;
    }
}

void test_gps_mode(GpsState *s)
{
	int flag = COMMAND;
	int ret = 0;

	if(gps_mode > 0)
	{
		usleep(100000);
		switch(gps_mode)
		{
			case GPS_WARM_START: //warm start
				if(s->cmcc)
				{
					D("set gps_mode=1,warm start now");
					s->cmd_flag = WARM_START_MODE;
					gps_sendData(flag, warm_cmd);
				}
				#if 0
				D("set gps_mode=1,warm start now");
				s->cmd_flag = WARM_START_MODE;
				if(s->hotTestFlag)
				{
					s->hotTestFlag = 0;
					release_wake_lock("GNSSLOCK");
					D("warm start release gnsslock now");
				}
				gps_sendData(flag, warm_cmd);
				#endif
				break;

			case GPS_COLD_START:
				 if(s->cmcc)
				 {
				     D("set gps_mode=125,cold start now");
                     s->cmd_flag = COLD_START_MODE;
				     gps_sendData(flag, cold_cmd);
				 }
				#if 0
				D("set gps_mode=125,cold start now");
				s->cmd_flag = COLD_START_MODE;
				if(0 == s->hotTestFlag)
				{
					s->hotTestFlag = 1;
					acquire_wake_lock(PARTIAL_WAKE_LOCK, "GNSSLOCK");
					D("cold start get gnsslock now");
				}
				g_startmode_flg = 1;
				gps_sendData(flag, cold_cmd);
				#endif
				break;

			case GPS_LOG_ENABLE:
				D("enable log save");
				gps_log_enable = 1;
				break;

			case GPS_START_SI:
				D("test_gps_mode: start si period session");
				start_agps_si(GPS_START_SI);
				break;

			case GPS_STOP_SI:
				D("test_gps_mode: stop si peroid session");
				stop_agps_si();
#ifdef GNSS_LCS
				gnss_lcsContrlEcid(agpsdata->si_sid,
					LCS_DOMAIN_UP,LCS_NWLR_ECID,0);
				gnss_lcscancelTriggerSession(agpsdata->si_sid);
#else
				RXN_cancelTriggerSession(agpsdata->si_sid);
#endif
				break;

			case GPS_LOG_DISABLE:
				D("disable log save");
				gps_log_enable = 0;
				break;

			case GPS_HOT_START:
				#if 0
				D("set gps_mode=1024,hot start now");
				s->cmd_flag = HOT_START_MODE;
				if(s->hotTestFlag)
				{
					s->hotTestFlag = 0;
					release_wake_lock("GNSSLOCK");
					D("hot start get gnsslock now");
				}
				gps_sendData(flag, hot_cmd);
				#endif
				break;

			case GPS_GPS_ONLY:
				D("gps only mode");
				config_xml_ctrl(SET_XML,"CP-MODE","001",CONFIG_TYPE);
				s->cpmode = 0x1;  //101 gps+glonass
				break;
			case GPS_GL_ONLY:
				D("glonass only mode");
				config_xml_ctrl(SET_XML,"CP-MODE","100",CONFIG_TYPE);
				s->cpmode = 0x4;  //101 glonass only mode
				break;
			case GPS_BDS_ONLY:
				D("bds only mode");
				config_xml_ctrl(SET_XML,"CP-MODE","010",CONFIG_TYPE);
				s->cpmode = 0x2;  //101 bds only mode
				break;
			case GPS_GPS_GL:
				D("gps and glonass mode");
				config_xml_ctrl(SET_XML,"CP-MODE","101",CONFIG_TYPE);
				s->cpmode = 0x5;  //gps and glonass mode
				break;
			case GPS_GPS_BDS:
				D("gps and bds mode");
				config_xml_ctrl(SET_XML,"CP-MODE","011",CONFIG_TYPE);
				s->cpmode = 0x3;  //101 gps and bds mode
				break;
			case 34813:
				D("set gps_mode=34813,cold start now");
				s->cmd_flag = COLD_START_MODE;
				if(0 == s->hotTestFlag)
				{
					s->hotTestFlag = 1;
					acquire_wake_lock(PARTIAL_WAKE_LOCK, "GNSSLOCK");
					D("cold start get gnsslock now");
				}
				g_startmode_flg = 1;
				gps_sendData(flag, cold_cmd);
				break;
			case 34815:
				D("set gps_mode=34815 fac test");
				s->cmd_flag = FAC_START_MODE;
				if(s->hotTestFlag)
				{
					s->hotTestFlag = 0;
					release_wake_lock("GNSSLOCK");
					D("fact start get gnsslock now");
				}
				gps_sendData(flag, fac_cmd);
				break;
			case GPS_FAC_START:
				#if 0
				D("set gps_mode=65535 fac test");
				s->cmd_flag = FAC_START_MODE;
				if(s->hotTestFlag)
				{
					s->hotTestFlag = 0;
					release_wake_lock("GNSSLOCK");
					D("fact start get gnsslock now");
				}
				gps_sendData(flag, fac_cmd);
				#endif
				break;

			case GPS_TCXO:
				D("TCXO test mode");
                		s->etuMode = 1;
				gps_sendData(EUT_TEST_TCXO,NULL);
				break;
			case GPS_CW_CN0:
				D("CW CN0 test mode");
                		s->etuMode = 1;
				s->cw_enable = 1;   // HW_CW
				s->watchdog = 0;
				gps_sendData(EUT_TEST_CW,NULL);
				break;
			case GPS_RF_GPS:
			case GPS_RF_GL:
			case GPS_RF_BD:
				{
					char temp[1]={0};
					if(gps_mode == GPS_RF_GPS) {
						temp[0] = EUT_RF_GPS;
					}
					else if(gps_mode == GPS_RF_GL) {
						temp[0] = EUT_RF_GL;
					}
					else {
						temp[0] = EUT_RF_BD;
					}
                			D("rf data tool mode");
                			s->etuMode = 1;
                			s->rftool_enable = 1;
					s->watchdog = 0;
					gps_sendData(RF_DATA_CAPTURE, temp);
                }
				break;
			case GPS_TSX_TEMP:
				D("tsx temp test mode");
                s->etuMode = 1;
				if(access("/productinfo/tsx.dat", F_OK) == 0)
				{
					ret = remove("/productinfo/tsx.dat");
					if(ret < 0)
					{
						E("fail to remove tsx.dat: errno=%d, strerror(errno)=%s", errno, strerror(errno));
					}
				}
				else
				{
					E("fail to access tsx.dat: errno=%d, strerror(errno)=%s", errno, strerror(errno));
				}
				config_xml_ctrl(SET_XML,"TSX-ENABLE","TRUE",CONFIG_TYPE);    // TSX_TEMP
				gps_sendData(EUT_TEST_TSX,NULL);
				break;
            case GPS_NEW_TCXO:
                D("TCXO/TSX dia new test");
                s->etuMode = 1;
                gps_sendData(EUT_TEST_TCXO_TSX, NULL);
                break;
 
			default:
				//assert(0);
				break;
		}
		gps_mode = 0;
		s->cmd_flag = 0;
        if(s->deleteAidingDataBit)
        {
            gps_sendData(SET_DELETE_AIDING_DATA_BIT,NULL);
        }
		//s->cpmode = 0;
	}	 
}

static void saveMemDedug(char *buf,int length)
{
    static FILE* fMemDump = NULL;
    static int revLen = 0;
    
    if(fMemDump)
    {      
        fwrite(buf,length,1,fMemDump); 
        fflush(fMemDump); 
        revLen += length;
    }
    else
    {
        fMemDump = fopen("/data/gnss/debug.mem","wb+");
        if(!fMemDump)
        {
            E("saveMemDedug create gnssdump error %s\r\n",strerror(errno));
            return;
        }
        else
        {
            D("it create saveMemDedug file");
            fwrite(buf,length,1,fMemDump); 
            fflush(fMemDump);
            revLen += length;
        }
         D("it writelen= 0x%x  saveMemDedug file",revLen);
    }
}

void memDumpAll(int fd)
{
    int n;
    char data[256];
    char codeEnd[64] = "ge2_dump352k_finish";
    char codeStart[64] = "ge2_start_dump";
    char allEnd[64] = "ge2_memdump_finish";
    DUMP_RecvState_e RevState = DUMP_RECV_START_UNKOWN;
    int   codeEndLen = strlen(codeEnd) + 1;
    int   codeStartLen = strlen(codeStart)+ 1;
    int totalLen = 0;
    char *pStart = NULL; 
    GpsState*  s = _gps_state;
   
    memset(data, 0,256);

    do
    {
        if(fd > 0)
        {
            n = read(fd, data, sizeof(data)-1);
            if(n > 0)//only debug 
            {
                saveMemDedug(data,n);
            }
        }
        else
        {
            break;
        }
        switch(RevState)
        {
            case DUMP_RECV_START_UNKOWN:
            {
                pStart = strstr(data, codeStart);
                if(pStart)
                {
                    D("start find head %d ,codeStartLen = %d",n,codeStartLen);
                    RevState = DUMP_RECV_CODE_START;
                    if(n > codeStartLen)
                    {
                        gps_saveCodedump((data+codeStartLen),(n-codeStartLen));
                        totalLen += (n-codeStartLen);
                    }
                }
                break;
            }
            case DUMP_RECV_CODE_START:
            {
                if((totalLen + n) < CODEDUMP_MAX_SIZE)
                {
                    gps_saveCodedump(data,n);
                    totalLen += n;
                }
                else //code + rsp + data
                {
                    pStart = strstr(data, codeEnd);
                    if(pStart)
                    {
                        if(pStart == data)
                        {
                            D("find the code finsh 1!");
                            if(n > codeEndLen)
                            {
                                 D("the first data len %d",(n - codeEndLen));
                                 gps_saveDatadump((pStart +codeEndLen),(n - codeEndLen));
                            }
                        }
                        else 
                        {
                            D("the left code len %d",(int)(pStart -data));
                            gps_saveCodedump(data,(pStart -data));
                            if(n > (pStart -data + codeEndLen))
                            {
                                gps_saveDatadump((pStart +codeEndLen),(n - (pStart + codeEndLen -data )));
                            }
                            
                        }
                        totalLen += (n - codeEndLen);
                        RevState = DUMP_RECV_CODE_END;
                    }
                    else  //the last  packet code 
                    {
                        gps_saveCodedump(data,n);
                        totalLen += n;
                    }

                }                
                break;
            }
            case DUMP_RECV_CODE_END:
            {
                
                totalLen += n;
                gps_saveDatadump(data,n); 
                if(totalLen > (DUMP_TOTAL_SIZE -1))
                RevState = DUMP_RECV_DATA_END;    
                break;
            }
            case DUMP_RECV_DATA_END:
            {
                pStart = strstr(data, allEnd);

                if(pStart)
                {
                    D("all memdupm ok ");
                    fd= -1;
                }
                break;
            }
            default:
             D("the dump data error branch");
                break;
        }   
    }while( (n > 0)  && (fd > 0));
    D("memDumpAll function end ");
}
 

/*------------------------------------------------------------
    Function:  gps_sendHeartTest

    Description:
        it  encode and send  heart test request  
    Parameters: none
--------------------------------------------------------------*/
static void  gps_sendHeartTest(GpsState* pGPsState)
{
	TCmdData_t packet;
	int len,writeLen ;
    pid_t pid, ppid,tid;

	memset(&packet,0,sizeof(TCmdData_t));
	//first check it can send data ?  todo 	
	packet.type    = GNSS_LIBGPS_DATA_TYPE;
	packet.subType = GNSS_LIBGPS_TEST_SUBTYPE;
	packet.length = 0;
	pthread_mutex_lock(&pGPsState->writeMutex);
	len = GNSS_EncodeOnePacket( &packet, pGPsState->writeBuffer, SERIAL_WRITE_BUUFER_MAX);

	if(len > 0)
	{
		writeLen = gps_writeSerialData(pGPsState,len);   //gps_writeData2Buffer
		pthread_mutex_lock(&pGPsState->mutex);
		pGPsState->waitHeartRsp =1;
		pGPsState->HeartCount++;
		pthread_mutex_unlock(&pGPsState->mutex);

		pid = getpid();
		ppid = getppid();
		tid = gettid();
		D("pid =%d, ppid =%d,tid =%d",pid,ppid,tid);
		D("gps_sendHeartTest HeartCount =%d\r\n",pGPsState->HeartCount);
	}
	else
	{
		E("gps_sendHeartTest error in send heart tet \r\n ");
	}
	pthread_mutex_unlock(&pGPsState->writeMutex);
	return ;
}


/*------------------------------------------------------------
    Function:  gps_check device abornaml state 

    Description:
        it  encode and send  heart test request  
    Parameters: none
--------------------------------------------------------------*/
static int  gps_abnormalState(GpsState* pGPsState,char magtic,speed_t speed)
{
    int n;
    char data[128];
    char * pStart;
    char* pEnd;
    struct termios termios;
    int    status = -1;
    int fd = 0;
    int count = 0;
   
    //it's block read if(s->fd < 0)
    fd = open(pGPsState->serialName, O_RDWR | O_NOCTTY);
    if(fd < 0)
    {
        E("the open UART error:%s\r\n",strerror(errno));
        return status;
    }
    pGPsState->fd = fd;
    memset(data, 0,128);
    tcflush(fd, TCIOFLUSH);
    tcgetattr(fd, &termios);
    termios.c_oflag &= ~OPOST;
    termios.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    termios.c_cflag &= ~(CSIZE | PARENB );
    termios.c_cflag |= (CS8 |CRTSCTS);
    cfmakeraw(&termios);
    cfsetispeed(&termios, speed); 
    status = tcsetattr(fd, TCSANOW, &termios);
    if(status != 0)
    {
        E("the set ttys3 error:%s\r\n",strerror(errno));
    }
    tcflush(fd,TCIOFLUSH);
    n = read(fd, data, sizeof(data)-1);
    pStart = data;
    pEnd  = &data[n-1];
    if(n > 0)
    {
        pStart = strchr(data,magtic);
        if(pStart)
        {
            D("\r\n count = %d,pStart =%p, pEnd=%p \r\n",count,pStart,pEnd); 
            while(pStart < pEnd)
            {
                if(*pStart == magtic)
                {
                    count++;
                }
                else
                {
                    count = 0;
                }
                pStart++;
            }
            if(count > 5)
            {
                status = 0;
            }
            else
            {
                status = -1;
            }
        }
        
        D("\r\n gps_abnormalState read len = %d, \r\n",n);       
    }
    else
    {
        status = -1;
    }
    return status;
}

void createDump(GpsState* pState)
{
	int ret = 0;
	int dataLen = 0;
	char data[64];
	char zygote_value[128];
	int ret_zy;

#ifdef GNSS_INTEGRATED
	gnss_request_memdumpl();
	return;
#endif
	ret_zy = property_get("init.svc.zygote",zygote_value,NULL);
	D("vts test get zygote prop %s ,ret_zy %d \r\n",zygote_value, ret_zy);
	if (ret_zy > 0) {
		if (strncmp(zygote_value, "stopped", sizeof("stopped")) == 0){
			D("%s,it is   vts test, so do not creatDump return\n",__FUNCTION__);
			return;
		}else{
			D("%s,it is not vts test\n",__FUNCTION__);
		}
	}
    acquire_wake_lock(PARTIAL_WAKE_LOCK, "GNSSLOCK");
    D("createDump close hardware \r\n");
    gps_hardware_close();
    //it's request download fdl
    D("start download fdl \r\n");
    ret = gps_devieManger(START_DOWNLOAD_FD1);
    if(!ret)
    {     
        D("the download fdl success!\r\n");
        ret = gps_abnormalState(pState,0x66,B2000000);
        D("gps_abnormalState after status = %d\r\n",ret);
        if(!ret)
        { 
            memset(data, 0x8e,64);
            dataLen = write(pState->fd, data, 8);
            if(dataLen > 2)
            {   
                D("send 0x8e to fdl ack!\r\n");
                memDumpAll(pState->fd);
            }
            ret = 0;
        }
    } 
    else
    {
        E("gps_devieManger download fail!\r\n");
    }
    release_wake_lock("GNSSLOCK");
}

int recoverDevice(GpsState* pState)
{
    int reqDownCount = 3;
    int ret = 0;

    D("%s enter!",__FUNCTION__);

    acquire_wake_lock(PARTIAL_WAKE_LOCK, "GNSSLOCK");
    fcntl(pState->fd, F_SETFL, O_RDWR | O_NOCTTY | O_NONBLOCK);
    gps_hardware_close();
    gps_setPowerState(DEVICE_DOWNLOADING);
    
    while(reqDownCount)
    {
        D("recoverDevice reqDownCount =%d download GE2",reqDownCount);
        ret = gps_devieManger(START_DOWNLOAD_GE2);  
        if(0 == ret)
        {
            gps_setPowerState(DEVICE_DOWNLOADED);
            break;// it's ok 
        }
        reqDownCount--;
    }
    if(ret)
    {
        E("%s recover download eror!",__FUNCTION__);
        release_wake_lock("GNSSLOCK");
        return ret;
    }
    //GE2 auto enter sleep mode,so AP should set sleep status
    gps_setPowerState(DEVICE_SLEEP);
    ret = gps_hardware_open(); 
    sleep(1);

    if(ret == 0)
    {
         ret = gps_wakeupDevice(pState); 
         if(ret == 0)
        {
             update_tsx_param();
             gps_sendData(SET_OUTTYPE,NULL);
             D("update assist data begin");
             update_assist_data(pState);
             D("update assist data end");
        }
         else
        {
             E("%s gps_wakeupDevice eror, and don't recover again",__FUNCTION__);
        }
    }
    else
    {
         E("%s gps_hardware_open eror!",__FUNCTION__);
    }
    release_wake_lock("GNSSLOCK");
    return ret;
}


static void watchdog( void*  arg )
{
    GpsState*   s = (GpsState*) arg;
	extern unsigned long send_length;
	extern unsigned long read_length;
	extern void udp_setup(GpsState *s);
    int dataLen = 0;
    char data[64];
    int status = 0;
    char wifi[128];
    int wifiold = 0;
    
	int watchdog_open = 1;
    
	while(watchdog_open)
	{
	    D("watchdog_open PowerState:%s readcount:%d\r\n",gps_PowerStateName(s->powerState),s->readCount);
	    if((s->powerState > DEVICE_DOWNLOADED) && 
           (s->powerState < DEVICE_POWEROFFING))//DEVICE_SLEEP
        {
            if(s->gps_flag == CMD_START)
            {
                D("libgps monitor Work,send byte is %ld,read byte is %ld" \
                  ,send_length,read_length);
                s->keepIdleTime = 0; 
                memset(wifi,0,sizeof(wifi));
                wifiold = agpsdata->wifiStatus;
                if(property_get("init.svc.p2p_supplicant",wifi,NULL) > 0)
                {
                    if(!memcmp(wifi,"running",strlen("running")))
                    {
                        D("wifi is open");
                        agpsdata->wifiStatus = 1;
                    }
                    else if(!memcmp(wifi,"stopped",strlen("stopped")))
                    {
                        agpsdata->wifiStatus = 0;
                    }
                }
                if(wifiold != agpsdata->wifiStatus)
                {
                    gps_sendData(SET_WIFI_STATUS,NULL);         
                }
                if(DEVICE_RESET == s->powerState)
                {
#ifndef GNSS_INTEGRATED

                    if(0 == recoverDevice(s))
                    {
                        s->HeartCount = 0;
                        s->started = 0;
                        fix_flag = 0;
                        gps_mode = GPS_HOT_START;//hot-start
                        gps_state_start(s);
                    }
                    else
                    {
                        D("recoverDevice fail,so return \r\n");
                        return;
                    }
#endif
                }
            }
            else if((s->gps_flag == CMD_STOP)|| (s->gps_flag == CMD_INIT))
            {
                if((s->sleepFlag)&& (DEVICE_IDLED == s->powerState))
                {
                
                    s->keepIdleTime++;
                    D("libgps monitor, idltime =%d second\r\n",
                        (s->keepIdleTime * GNSS_MONITOR_INTERVAL_TIME));

                    if(s->keepIdleTime > (s->sleepTimer>>1))
                    {                        
                        if(DEVICE_SLEEP != s->powerState)
                        {
                            gps_notiySleepDevice(s);
                            usleep(10000);
                        }
                        if(s->hotTestFlag)
                        {
                            s->hotTestFlag = 0;
                            release_wake_lock("GNSSLOCK");
                            D("release_wake_lock GNSSLOCK ");
                        }
                        s->keepIdleTime = 0;
                    }
                    
                    if(s->screen_off == 1)
                    {
                        if((s->postlog)||(s->cmcc))//auto test 
                        {
                            D("auto test ,so don't send cp sleep");
                        }
                        else
                        {
                            if(!s->hotTestFlag)
                            {
                                if(DEVICE_SLEEP != s->powerState)
                                {
                                    D("it send sleep in  screen off ");
                                    gps_notiySleepDevice(s);
                                    usleep(10000);
                                    s->keepIdleTime = 0;
                                }
                                s->keepIdleTime = 0;
                            }
                            else
                            {
                                D("it can't send sleep in  screen off when last cold-start");
                            }
                        }
                        
                    }

                } 
                else if((s->sleepFlag)&& (DEVICE_RESET == s->powerState))
                {
#ifndef GNSS_INTEGRATED
                    if(0== recoverDevice(s))
                    {
                        s->HeartCount = 0;
                        s->keepIdleTime = 0;
                        fix_flag = 0;
                        gps_mode = GPS_HOT_START;//hot-start
                        if(s->postlog)
                        {
                            D("auto test ,so don't send cp sleep");
                        }
                        else
                        {
                            gps_notiySleepDevice(s);
                            usleep(20000);
                        }
                    }
                    else
                    {
                         D("recoverDevice fail,so return \r\n");
                         return;
                    }
#endif
                }
            }
            else
            {
                D("libgps monitor, PowerState: %s \r\n",gps_PowerStateName(s->powerState));
                s->keepIdleTime = 0;
            }
            if((s->watchdog)&& (s->fd > 0)&& (!s->happenAssert)&&
                (DEVICE_SLEEP!= s->powerState)&&(DEVICE_RESET!= s->powerState))
            {
                D("pid =%d, ppid =%d,tid =%d",getpid(),getppid(),gettid());
                if(DEVICE_SLEEPING == s->powerState){ 
                    gps_power_Statusync(POWER_SLEEPING);
                }
                if(DEVICE_SLEEP != s->powerState){
                    gps_sendHeartTest(s);
                }
                if(s->HeartCount > HEART_COUNT_MAX)
                {
                    struct timeval rcv_timeout;
                    rcv_timeout.tv_sec = 5;
                    rcv_timeout.tv_usec = 0;
                    gnss_notify_abnormal(WCND_SOCKET_NAME,&rcv_timeout,"heartcount-trigger");
                    if(!s->release) //userdebug version 
                    {   
                        D("the romcode send 0x55, ge2 crash!\r\n");
						gps_setPowerState(DEVICE_ABNORMAL);
#ifdef GNSS_INTEGRATED
                        gnss_request_memdumpl();
#else
                        createDump(s);
#endif
                        s->HeartCount = 0; 
                    }
                  else
                    {
                         D("%s,gps_wakeupDevice  failed ",__FUNCTION__);
                         gps_setPowerState(DEVICE_RESET);                      
                    }
                }

            }
        }
        if((agpsdata->udp_stat == 0) && ((s->cmcc == 1)||(agpsdata->udp_enable == TRUE)))    //&& (agps_mode == SUPL_MODE))
        {
            D("setup udp thread in watchdog thread");
            udp_setup(s);
        }
#ifndef GNSS_INTEGRATED
        //the reset device timeout ,so get status every time 
        if(DEVICE_DOWNLOADING == s->powerState)
        {
            status  = gps_devieManger(GET_GNSS_STATUS);
            if(status)
            {
                 D("%s  get gnss_download  status failed ",__FUNCTION__);
            }
            else
            {
                gps_notiySleepDevice(s);
                usleep(20000);
                D("%s  get gnss_download  status success ",__FUNCTION__);
            }
            
        }
 #endif
		sleep(GNSS_MONITOR_INTERVAL_TIME);
	}
	return;
}

static void gps_read_thread( void*  arg )
{
    GpsState*   s = (GpsState*) arg;
    int rc;
    fd_set rfds;
    int fd = -1;
    struct timeval tv;
    int maxFd;

    thread_status = thread_run;
    nmea_reader_init( s->NmeaReader );
    nmea_reader_set_callback(s->NmeaReader,&s->callbacks); 
	GNSS_InitParsePacket(&gGPSStream);
    D("gps_read_thread , before threadsync wait");
	WaitEvent(&(s->threadsync), EVENT_WAIT_FOREVER);//wait signal
    D("gps_read_thread received WaitEvent");
    
    while(1)
    {          
        if((s->powerState > DEVICE_DOWNLOADING)
           &&(s->powerState < DEVICE_POWEROFFED))
        {   
            fd = s->fd;
            if(fd < 0)
            {
                D("WaitEvent Read event before\r\n");
                WaitEvent(&(s->threadsync), EVENT_WAIT_FOREVER);//wait signal
                D("WaitEvent Read event after \r\n");
            }
            else
            { 
                FD_ZERO(&rfds);
                FD_SET(fd, &rfds);             
                if(s->powerctrlfd > 0 && s->gps_flag != CMD_CLEAN)
                {
                    FD_SET(s->powerctrlfd, &rfds);
                    maxFd = MAX(fd,s->powerctrlfd);
                }
                else
                {
                    maxFd = s->fd;
                }
                if(  s->gps_flag == CMD_START)
                {
                    tv.tv_sec = 1;
                    tv.tv_usec = 0;
                }
                else
                {
                    tv.tv_sec = 0;
                    tv.tv_usec = 10000;
                }
                rc = select(maxFd + 1, &rfds, NULL, NULL, &tv);
                if(rc < 0)
                {
                    E("select fd:%d error :%d: %s:\r\n", fd, errno,strerror(errno));
                    if(errno == EINTR)
                    {
                         continue;
                    }
                    else
                    {
                        usleep(50000);
                        continue;
                    }
                }
                
                if(FD_ISSET(fd, &rfds))
                {
                    gps_serialPortRead(s);
                }
			if((s->powerctrlfd > 0)&&(FD_ISSET(s->powerctrlfd, &rfds)))
			{
				D("ap will sleep,send cp sleep,gps_flag:%d, sleepflag:%d, powerstate: %d",s->gps_flag,s->sleepFlag,s->powerState);

				gps_screenStatusRead(s);
				if((CMD_START != s->gps_flag) && (s->sleepFlag) && (DEVICE_IDLED == s->powerState) && (s->screen_off == 1))
				{
				    if((s->postlog)||(s->cmcc))
                    {
                        D("auto test ,so don't send cp sleep");
                    }
                    else
                    {
                        if(!s->hotTestFlag)
                        {
                            D("gps_screenStatusRead Read SCREEN off and send sleep");
                            gps_notiySleepDevice(s);
                            usleep(30000);
                          
                        }
                        else
                        {
                             D("gps_screenStatusRead Read SCREEN off but last cold-start");
                        }
                    }
					s->keepIdleTime = 0;
				}
			}
                
            }
        }
        else
        {
            sleep(1);
        }
	}

}

static void  gps_state_done( GpsState*  s )
{
    // tell the thread to quit, and wait for it
    char   cmd = CMD_QUIT;
    void*  dummy;
    usleep(100000);   //100ms, wait libgps thread run
    if(thread_status == thread_run)
    {
        write( s->control[0], &cmd, 1 );
        pthread_join(s->thread, &dummy);
    }
    if(s->control[0] >= 0) { close( s->control[0] ); s->control[0] = -1;}
    if(s->control[1] >= 0) { close( s->control[1] ); s->control[1] = -1;}
  
    s->init = 0;
    D("%s: clear init \r\n",__FUNCTION__);
}


static void  gps_state_init( GpsState*  state )
{
    int  ret;
    char time = 0;
    state->first       =  1;
    state->control[0] = -1;
    state->control[1] = -1;
    state->fd         = -1;  
    state->powerctrlfd = -1;
    state->recurrence = GPS_POSITION_RECURRENCE_PERIODIC;
    state->min_interval = 1;
    state->preferred_accuracy = 50;
    state->preferred_time = 1;
    state->gnsslogfd = -1;
    state->sleepCount = 0;
    state->wakeupCount = 0;
	
    D("%s enter",__FUNCTION__);
    if( socketpair( AF_LOCAL, SOCK_STREAM, 0, state->control ) < 0 ) 
    {
        E("could not create thread control socket pair: %s", strerror(errno));
        goto Fail;
    }
    if (CreateEvent(&(state->threadsync), "threadsync") != 0)
    E("CreateEvent threadsync is fail,should got it");
    if (CreateEvent(&(state->gpsdsem), "gpsdsem") != 0)
    E("CreateEvent gpsdsem is fail,should got it");

	if (CreateEvent(&(state->readsync), "readsync") != 0)
	{
		E("CreateEvent readsync is fail,should got it");
	}
	if (CreateEvent(&(state->vesionsync), "vesionsync") != 0)
	{
		E("CreateEvent vesionsync is fail,should got it");
	}	
    if (CreateEvent(&(state->gnssLogsync), "gnssLogsync") != 0)
    {
        E("CreateEvent gnssLogsync is fail,should got it");
    }   

    state->m_capabilities = GPS_CAPABILITY_MSA \
		                    |GPS_CAPABILITY_MSB \
		                    |GPS_CAPABILITY_ON_DEMAND_TIME \
		                    |GPS_CAPABILITY_SCHEDULING;
							
	state->m_capabilities  |= GPS_CAPABILITY_MEASUREMENTS;
#ifndef GNSS_ANDROIDN	
	
	state->m_capabilities  |= GPS_CAPABILITY_NAV_MESSAGES;
#endif //GNSS_ANDROIDN	

    D("state->m_capabilities is %d\n", state->m_capabilities);
    state->callbacks.set_capabilities_cb(state->m_capabilities);
#if 0 //GNSS_ANDROIDN
	state->gnss_system_info.year_of_hw = 2016;
	state->gnss_system_info.size = sizeof(GnssSystemInfo);
	state->callbacks.set_system_info_cb(&(state->gnss_system_info));
#endif //GNSS_ANDROIDN

	state->lte_thread = state->callbacks.create_thread_cb( "lte",SoonerFix_PredictThread, state);  
	if ( !state->lte_thread ) 
	{
		E("could not create gps lte_thread: %s", strerror(errno));
	}
	usleep(20000);
	

    state->thread = state->callbacks.create_thread_cb( "libgps", gps_read_thread, state );
    if ( !state->thread ) 
    {
	    E("could not create gps thread: %s", strerror(errno));
        goto Fail;
    }
    while((thread_status != thread_run) && (time < 5))
    {
	    time++;
	    usleep(25000);
    }
    state->daemon_thread = state->callbacks.create_thread_cb( "libgps_daemon", gps_daemon_thread, state );
    if ( !state->daemon_thread ) 
    {
	    E("could not create gps daemon thread: %s", strerror(errno));
        goto Fail;
    }
    state->gnss_log_thread = state->callbacks.create_thread_cb( "lib_gnss_log", gnss_log_thread, state );
    if ( !state->gnss_log_thread ) 
    {
	    E("could not create gnss log  thread: %s", strerror(errno));
        goto Fail;
    }
    time = 0;
    while((daemon_state != thread_run) && (time < 5))
    {
	    time++;
	    usleep(25000);
    }
    D("%s create gps_daemon_thread success\r\n", __FUNCTION__);


    state->watchdog_thread = state->callbacks.create_thread_cb( "watchdog", watchdog, state );
    if ( !state->watchdog_thread ) 
    {
		  E("could not create gps watchdog thread: %s", strerror(errno));
        goto Fail;
    }
    state->lcs_clientthread = state->callbacks.create_thread_cb( "lcs_client", gnssdaemon_client_thread, state );
    // to do 
    //add control power ctrl fd 
    if(state->powerctrlfd < 0)
    {
#ifndef GNSS_INTEGRATED       
        state->powerctrlfd = open("/dev/power_ctl",O_RDWR);
#else
        state->powerctrlfd = open("/dev/gnss_pmnotify_ctl",O_RDWR);
#endif 
        
        if(state->powerctrlfd < 0) 
        {
            E("open /dev/power_ctl or gnss_pmnotify_ctl error: %s",strerror(errno));
        }

    }
    D("%s create watchdog success\r\n", __FUNCTION__);

    if(state->gnsslogfd < 0)
    {
        state->gnsslogfd = open("/dev/gnss_dbg",O_RDWR);
        if(state->gnsslogfd < 0) 
        {
            E("open /dev/gnss_dbg error: %s",strerror(errno));
        }
        else
        {
            D("open /dev/gnss_dbg success \n");
        }
    }
    //init agps timeout protection para
    state->lastreftime = 0;
    state->lastrefeph= 0;
    return;

Fail:
    E("gps init create thread is fail!\n");
    gps_state_done( state );
}

/************************************ zhouxw add for gps init mod *******************************/
/////////////////////////////////////////////////////////////////////////////////////////////////
//===============================================================================================//

static int checkthread(void)
{
    GpsState*  state = _gps_state;
    int ret = 0;
    char time = 0;
    if(thread_status != thread_run){
        state->thread = state->callbacks.create_thread_cb( "libgps", gps_read_thread, state );
        if ( !state->thread )
        { 
            E("could not create gps thread: %s", strerror(errno));
            ret = 1;
        }

        while((thread_status != thread_run) && (time < 5))
        {
            time++;
            usleep(25000);
        }
    }

    return ret;
}

void setup_file(int type)
{
    GpsState*  state = _gps_state;
    FILE *fp;
    FILE *xmlfp;
    char *buf;
    struct stat xmlstat;
	char *etc_path = NULL;
	char *setup_path = NULL;

	switch(type)
	{
		case SUPL_TYPE:
			etc_path = state->supl_path;
			setup_path = SUPL_XML_PATH;
			break;
		case CONFIG_TYPE:
			etc_path = CONFIG_ETC_PATH;
			setup_path = CONFIG_XML_PATH;
			break;
        case FDL_TYPE:
			etc_path = GE2_FDL_ETC_PATH;
			setup_path = GE2_FDL_PATH;
			break;
		case RAWOBS_TYPE:
			etc_path = RAWOBS_ETC_PATH;
			setup_path = RAWOBS_PATH;
			break;	
		case CER_TYPE:
			etc_path = AGNSS_CER_ETC_PATH;
			setup_path = cer_path;
			break;		
		default:
			break;
	}
	if((setup_path == NULL) || (etc_path == NULL))
	{
		E("etc path or setup path is null");
		return;
	}
	D("setup file etc-path = %s,setup-path = %s",etc_path,setup_path);
    if(access(etc_path,0) != -1)
    {
        stat(etc_path, &xmlstat);
        fp = fopen(etc_path,"rb");
        if(fp == NULL)
        {
            E("etc fp open fail,error is %s",strerror(errno));
            return;
        }
        buf = calloc(xmlstat.st_size ,1);
		if(buf == NULL){
            E("calloc  memory failed,error is %s",strerror(errno));
			fclose(fp);
			return;
		}
        fread(buf,xmlstat.st_size,1,fp);
        xmlfp = fopen(setup_path,"wb+");
        if(xmlfp != NULL)
        {
            fwrite(buf,xmlstat.st_size,1,xmlfp);
            fclose(xmlfp);
            E("====>>>>>>copy type xml data from etc to type:%d",type);
        }
        else
        {
             E("====>>>>>>setup_file setup_path error is %s\r\n",strerror(errno));
        }
        chmod(setup_path,0666);
        free(buf);
        fclose(fp);
    }
    else
    {
        E("setup etc path is not exist,type is %d",type);
    }
}

void setup_directory(void)
{
    if(access("/data/gnss",0) == -1)
    {
        mkdir("/data/gnss",0774);
    }
    if(access("/data/gnss/lte",0) == -1)
    {
        mkdir("/data/gnss/lte",0774);
    }
    if(access("/data/gnss/config",0) == -1)
    {
        mkdir("/data/gnss/config",0774);
    }
    if(access("/data/gnss/supl",0) == -1)
    {
        mkdir("/data/gnss/supl",0774);
    }
   if(access("/data/gnss/bin",0) == -1)
    {
        mkdir("/data/gnss/bin",0774);
    }
}

int query_server_mode(void)
{
	char *server_string = "SERVER-ADDRESS";
	char *port_string = "SERVER-PORT";
	char server_value[64];
	unsigned short port_value = 0;
	char value[64];
	int server_flag = 0;
	int port_flag = 0;
	int ret = 0;

	memset(value,0,sizeof(value));
	ret = get_xml_value(server_string, value);
	if(ret == 0)
	{
		memset(server_value,'\0',64);
		memcpy(server_value,value,strlen(value));

		D("%s: server addr is %s\n", __func__, server_value);

		if(!memcmp(server_value, "localhost", strlen("localhost")))
		{
			server_flag = 1;
		}
	}
	else
	{
		return 1;
	}

	memset(value,0,sizeof(value));
	ret = get_xml_value(port_string, value);
	if(ret == 0)
	{
		port_value = s2int(value,strlen(value));
		D("%s: port is %d\n", __func__, port_value);
		if((server_flag == 1)&&(port_value == 31275))
		{
			return 1;  // NOKIA EE mode
		}
		else
		{
			return 0;  // AGPS mode
		}
	}
	else
	{
		return 1;
	}
}

//stat function can't work normal 
int compare_file_tag(char *dname,char *sname)
{

	int ret = 1;
    int len;
    int tagcount = 0;
    FILE *fdame = NULL;
    FILE *fsname = NULL;
    char dbuf[XML_FILE_MAX_MUM],sbuf[XML_FILE_MAX_MUM];
    char * tagsbegin = NULL;
    char * tagsend   = NULL;
    char *tagdbegin  = NULL;
    char * tagdend   = NULL;
	size_t minilen = strlen("<GNSS>");
    
    fdame = fopen(dname, "r");
    fsname = fopen(sname, "r");
    
    if(NULL == fdame ||NULL== fsname)
    {  
        if(fdame)
        {
            fclose(fdame);
        }
        if(fsname)
        {
	    fclose(fsname);
        }
	E("%s: open dname fail, error is %s\n",__FUNCTION__,strerror(errno));
	return 0;
    }
    
    memset(dbuf,0,XML_FILE_MAX_MUM);
    memset(sbuf,0,XML_FILE_MAX_MUM);
    while(fgets(sbuf,XML_FILE_MAX_MUM,fsname)!= NULL)
    {
		if(strlen(sbuf) < minilen)
		{
			E("read config.xml error:%s",sbuf);
			continue;
		}
        if(fgets(dbuf,XML_FILE_MAX_MUM,fdame)!= NULL)
        {
			while(strlen(dbuf) < minilen)
			{
				E("read config.xml error:%s",sbuf);
				memset(dbuf,0,XML_FILE_MAX_MUM);
				if(fgets(dbuf,XML_FILE_MAX_MUM,fdame) != NULL)
				{
					E("get another lines from config");
					continue;
				}
				else
				{
                    E("config xml is end,some error in it");
                    return 0;
				}
			}
            //get system xml tag 
            tagsbegin = strstr(sbuf,"NAME");
            tagsend = strstr(sbuf,"VALUE");
            if(tagsbegin && tagsend)
            {
                len = tagsend - tagsbegin;
                //get data xml tag
                tagdbegin = strstr(dbuf,"NAME");
                tagdend = strstr(dbuf,"VALUE");
                if(tagdbegin && tagdend)
                {
                    //compare the tag 
                    if(0 != strncmp(tagsbegin,tagdbegin, len))
                    {
                        E("tag not same:len:%d :%s,%s",len,sbuf,dbuf);
                        ret = 0; 
                        break;
                    }
                    else
                    {
                        tagcount++;
                    }
                 }
                else
                {
                    E("data can't get tag %s,%s",sbuf,dbuf);
                    ret = 0; 
                    break;
                }
            } 
            else
            {
                D("the system xml sentence not include tag :%s",sbuf);
            }

        }
        else
        {
            E("%s: data can't get data tag same as system %s\n",__FUNCTION__,sbuf);
            ret = 0; 
            break;
        }
        
        memset(dbuf,0,XML_FILE_MAX_MUM);
        memset(sbuf,0,XML_FILE_MAX_MUM);
    }
    D("%s: get same tag count:%d\n",__FUNCTION__,tagcount);
    fclose(fdame);
    fclose(fsname);
	return ret;
}


static void gps_config(GpsState* pState)
{
    int ret = 0;
    char lte_string[16];
    char post_string[16];
    char watchdog_string[16];
    char sleep_string[16];
    char img_string[16];
    char log_string[16];
    char partition[128];
    char buildType[128];
    char gnssmode[128];
    char andoridversion[128];
    char tsx_string[16];    // TSX_TEMP
    char rftool_string[16]; 
    char value[16];
    char tmp_server[64];

    memset(partition,0,sizeof(partition));
    memset(buildType,0,sizeof(buildType));
    memset(cer_path,0,sizeof(cer_path));
    memset(lte_string,0,sizeof(lte_string));
    memset(gnssmode,0,sizeof(gnssmode));
    memset(andoridversion,0,sizeof(andoridversion));
    setup_directory();

    if(access("/productinfo/txdata.txt", F_OK) == 0)
    {
        pState->tsx_enable = 1;
    }
    else
    {
        pState->tsx_enable = 0;
    }
    pState->getetcconfig = 0;

    if(access(CONFIG_XML_PATH,0) == -1)
    {
        setup_file(CONFIG_TYPE);
    }
    else
    {
        if(compare_file_tag(CONFIG_XML_PATH,CONFIG_ETC_PATH)==0)
        {
	    D("/data/gnss/config/config.xml size is zero\n");
	    if(pState->cmcc==0)
	    {
                D("setup config.xml from etc");
	         setup_file(CONFIG_TYPE);
	    }
	 }
	 else
	{
#ifndef GNSS_INTEGRATED
	    memset(pState->serialName,0,20);
	    ret = config_xml_ctrl(GET_XML,"UART-NAME",pState->serialName,CONFIG_TYPE);
	    if(ret == 0)
	    {
	        if(memcmp(pState->serialName,"/dev/ttyS3",strlen("/dev/ttyS3"))!=0)
		{
		    D("/data/gnss/config/config.xml can not read right value\n");
		    setup_file(CONFIG_TYPE);
		}
	    }
	    else
	    {
	        D("/data/gnss/config/config.xml read fail\n");
		setup_file(CONFIG_TYPE);
	    }
#else
		memset(pState->serialName,0,GNSS_SIPC_NAME_LEN);
		ret = config_xml_ctrl(GET_XML,"STTY-NAME",pState->serialName,CONFIG_TYPE);
		if(ret == 0)
		{
			if(memcmp(pState->serialName,"/dev/sttygnss0",strlen("/dev/sttygnss0"))!=0)
			{
				D("/data/gnss/config/config.xml can not read right value\n");
				setup_file(CONFIG_TYPE);
			}
		}
		else
		{
			D("/data/gnss/config/config.xml read fail\n");
			setup_file(CONFIG_TYPE);
		}
#endif
	}
     }

	memset(pState->supl_path, 0, 64);
	ret = config_xml_ctrl(GET_XML, "SUPL-PATH", pState->supl_path, CONFIG_TYPE);
	if(ret == 0)
	{
		if(access(pState->supl_path, 0) == -1)
		{
			memset(pState->supl_path, 0, 64);
			strncpy(pState->supl_path, "/vendor/etc/supl.xml", sizeof("/vendor/etc/supl.xml"));
		}
	}
	else
	{
		strncpy(pState->supl_path, "/vendor/etc/supl.xml", sizeof("/vendor/etc/supl.xml"));
	}
	D("gps_config: supl path: %s", pState->supl_path);

    if(access(SUPL_XML_PATH,0) == -1)
    {
        setup_file(SUPL_TYPE);
    }
	else
	{
		memset(value, 0, 16);
		ret = get_xml_value("SETID",value);
		if(ret == 0)
		{
			if(strcmp(value,"IMSI")!=0)
			{
				D("/data/gnss/supl/supl.xml can not read right value\n");
				setup_file(SUPL_TYPE);
			}
		}
		else
		{
			D("/data/gnss/supl/supl.xml read fail\n");
			setup_file(SUPL_TYPE);
		}
	}
	memset(tmp_server,0,sizeof(tmp_server));
	ret = get_xml_value("SERVER-ADDRESS", tmp_server);
	if(ret==0 && (!memcmp(tmp_server,"www.spirent",strlen("www.spirent"))||
       !memcmp(tmp_server,"slp.rs.de",strlen("slp.rs.de"))))
	{
	    D("spirent or slp server is set");
	    pState->cmcc = 1;
        acquire_wake_lock(PARTIAL_WAKE_LOCK, "GNSSLOCK");
        D("CMCC  get gnsslock now");
	}
   

    if((!get_xml_value("SUPL-CER", cer_path)) && (access(cer_path,0) == -1))
    {
        D("cerpath is not exist,copy from etc to %s",cer_path);
        setup_file(CER_TYPE);
    }
	else
	{
		if((!get_xml_value("SUPL-CER", cer_path))&&(access(cer_path,0) == 0))
		{
			setup_file(CER_TYPE);
		}
	}
    //protect the config.xml desotry 
    if(compare_file_tag(CONFIG_XML_PATH,CONFIG_ETC_PATH)==0&& pState->cmcc==0)
    {
        pState->getetcconfig = 1;
    }
    
    memset(lte_string,0,16);
    ret = config_xml_ctrl(GET_XML,"SPREADORBIT-ENABLE",lte_string,CONFIG_TYPE);
    if(ret == 0)
    {
        D("config xml get lte enable is %s",lte_string);
        if(!memcmp(lte_string,"TRUE",strlen("TRUE")))
        {
            pState->lte_open = 1;
            D("lte is open now");
        }
        else
        {
            pState->lte_open = 0;
            D("lte is close now");
        }
    }

	//Read NOKIA-EE status from config.xml
    memset(value,0,16);
	pState->nokiaee_enable = 0;
	pState->use_nokiaee = 0;
    ret = config_xml_ctrl(GET_XML,"NOKIA-EE",value,CONFIG_TYPE);
    if(ret == 0)
    {
        D("%s: NOKIA-EE value is %s",__func__,value);
        if(!memcmp(value,"TRUE",strlen("TRUE")))
        {
			D("NOKIA EE enable");
			pState->nokiaee_enable = 1;
        }
    }
	
#ifndef GNSS_INTEGRATED
	memset(pState->serialName,0,20);
	ret = config_xml_ctrl(GET_XML,"UART-NAME",pState->serialName,CONFIG_TYPE);
	if(ret == 0)
	{
		D("get the uart  name %s\n",pState->serialName);
	}
	else
	{
		E("get from config xml is fail, default ttyS3");
		strncpy(pState->serialName,"/dev/ttyS3",sizeof("/dev/ttyS3"));	
	}
#else
	memset(pState->serialName,0,GNSS_SIPC_NAME_LEN);
	ret = config_xml_ctrl(GET_XML,"STTY-NAME",pState->serialName,CONFIG_TYPE);
	if(ret == 0)
	{
		D("get the uart  name %s\n",pState->serialName);
		if(memcmp(pState->serialName,"/dev/sttygnss0",strlen("/dev/sttygnss0"))!=0)
		{
			D("/data/gnss/config/config.xml can not read right value\n");
		}
	}
	else
	{
		E("get from config xml is fail, default ttyS3");
		strncpy(pState->serialName,"/dev/sttygnss0",sizeof("/dev/sttygnss0"));	
	}
#endif

	//add get post-log 
    
    memset(post_string,0,16);
	ret = config_xml_ctrl(GET_XML,"POST-ENABLE",post_string,CONFIG_TYPE);
	if(ret == 0)
	{
        D("config xml get post log enable is %s",post_string);
        if(!memcmp(post_string,"TRUE",strlen("TRUE")))
        {
            pState->postlog= 1;
            D("post log set now");
        }
        else
        {
            pState->postlog = 0;
            D("post log close now");
        }
	}
    //add watch dog  turn on /off 
    memset(watchdog_string,0,16);
	ret = config_xml_ctrl(GET_XML,"APWDG-ENABLE",watchdog_string,CONFIG_TYPE);
	if(ret == 0)
	{
        D("config xml get ap watchdog enable is %s",watchdog_string);
        if(!memcmp(watchdog_string,"TRUE",strlen("TRUE")))
        {
            pState->watchdog = 1;
            D("ap  watchdog set now");
        }
        else
        {
            pState->watchdog = 0;
            D("ap watchdog close now");
        }
	}
    //add seelp/warkup 
    memset(sleep_string,0,16);
     ret = config_xml_ctrl(GET_XML,"SLEEP-ENABLE",sleep_string,CONFIG_TYPE);
     if(ret == 0)
     {
         D("config xml get cp sleep/wakeup enable is %s",sleep_string);
         if(!memcmp(sleep_string,"TRUE",strlen("TRUE")))
         {
            memset(sleep_string,0,16);
            ret = config_xml_ctrl(GET_XML,"SLEEP-TIMER",sleep_string,CONFIG_TYPE);
            if(ret == 0)
            {
                
                D("sleep timer %d",atoi(sleep_string));
                pState->sleepTimer = atoi(sleep_string);
            }
            else  //set default interval timer to sleep
            {
                pState->sleepTimer = GNSS_ILDE_KEEPTIME_MAX;
                D("sleep timer set default %d",pState->sleepTimer);
            }
             pState->sleepFlag = 1;
             D(" sleep/wakeup set now");
         }
         else
         {
             pState->sleepFlag = 0;
             pState->sleepTimer = 0;
             D("sleep/wakeup close now");
         }
     }
     else//default value 
    {
        D("gps_config default sleep:enable and timeout 150s ");
        pState->sleepFlag = 1;
        pState->sleepTimer = GNSS_ILDE_KEEPTIME_MAX;
    }
    
	if(property_get("ro.build.type",buildType,NULL) > 0)
	{
		D("build type :[%s]",buildType);
		if(0 == strncmp(buildType,"user",sizeof("user")))
		{
			pState->release =1;
		}
		else
		{
			pState->release = 0;
		}
	}
	else
	{
		E("read ro.build.type error");
	}

    
    property_get("ro.ge2.partition",partition,NULL);
    property_get("ro.ge2.mode",gnssmode,NULL);
    property_get("ro.build.version.release",andoridversion,NULL);
    D("ro.ge2.partition:%s,ro.ge2.mode:%s android:%s",partition,gnssmode,andoridversion);

    if(0 == strncmp(partition,"true", sizeof("true")))
    {
        if(0 == strncmp(andoridversion,"5.1", sizeof("5.1")))
        {     
            char* pDownreq ="startgnss path=/dev/block/platform/sdio_emmc/by-name/temppartition";
            memcpy(pState->binPath,pDownreq,strlen(pDownreq)); 
        }
        else
        {
            char* pDownreq ="startgnss path=/dev/block/platform/sprd-sdhci.3/by-name/gnssmodem";
            memcpy(pState->binPath,pDownreq,strlen(pDownreq)); 
        } 
    }
    else
    {
        memset(img_string,0,16);
        ret = config_xml_ctrl(GET_XML,"GPS-IMG-MODE",img_string,CONFIG_TYPE);
        if(ret == 0)
        {
            D("config xml get img-mode is %s",img_string);
            if(!memcmp(img_string,"GNSSBDMODEM",strlen("GNSSBDMODEM")))
            {
                pState->imgmode = BDS_IMG;
                D("imgmode is bds");
            }
            else
            {
                pState->imgmode = GLONASS_IMG;
                D("imgmode is glonass");
            }
        }
        if(0 == strncmp(gnssmode,"gps_beidou", sizeof("gps_beidou")) || (pState->imgmode == GPS_BDS))
        {
            char* pDownreq ="startgnss path=/vendor/etc/gnssbdmodem.bin";
            memcpy(pState->binPath,pDownreq,strlen(pDownreq)); 
        }
        else    
        {
            char* pDownreq ="startgnss path=/vendor/etc/gnssmodem.bin";
            memcpy(pState->binPath,pDownreq,strlen(pDownreq)); 
        }
    }

	pState->slogmodem_fd = 0;
    read_slog_status();
    memset(log_string,0,sizeof(log_string));
    config_xml_ctrl(GET_XML,"LOG-ENABLE",log_string,CONFIG_TYPE);
    if(!memcmp(log_string,"TRUE",strlen("TRUE")))
    {
        D("open ge2 log switch");
        pState->logswitch = 1;
    }
    if(!memcmp(log_string,"FALSE",strlen("FALSE")))
    {
		  D("close ge2 log switch");
	      pState->logswitch = 0;
    }
    
    if(pState->slog_switch || pState->postlog || pState->logswitch)
    {
        pState->outtype = 1;
    }
    else
    {
        pState->outtype = 0;
    }


	memset(value, 0, 16);
	ret = config_xml_ctrl(GET_XML, "FLOAT-CN0", value, CONFIG_TYPE);
	if(ret == 0)
	{
		D("config xml get float CN0 enable: %s", value);
		if(!memcmp(value, "TRUE", strlen("TRUE")))
		{
			pState->float_cn_enable = 1;
		}
		else
		{
			pState->float_cn_enable = 0;
		}
	}
	else
	{
		pState->float_cn_enable = 0;
	}
    
	memset(value, 0, 16);
	ret = config_xml_ctrl(GET_XML, "BASEBAND-MODE", value, CONFIG_TYPE);
	if(ret == 0)
	{
		D("config xml get BASEBAND-MODE enable: %s", value);
		if(!memcmp(value, "TRUE", strlen("TRUE")))
		{
			pState->baseband_mode = 1;
		}
		else
		{
			pState->baseband_mode = 0;
		}
	}
	else
	{
		pState->baseband_mode = 0;
	}
}

static void get_img_info(void)
{
	FILE *fp;

    if(access(COMBINE_PATH,0) == -1)
    {
        E("cannot seek combine location:%s",strerror(errno));
        return;
    }
	fp = fopen(COMBINE_PATH,"r");
    if(fp == NULL)
    {
        printf("fp is null\n");
        return ;
    }
    fseek(fp,12 ,SEEK_SET);
    fread(gnssImgInfo[0].tag,1,4,fp);
    fread(&gnssImgInfo[0].offset,4,1,fp);
    fread(&gnssImgInfo[0].size,4,1,fp);
 
    fread(gnssImgInfo[1].tag,1,4,fp);
    fread(&gnssImgInfo[1].offset,4,1,fp);
    fread(&gnssImgInfo[1].size,4,1,fp);

	combineInfo.combineValid = 1;
	if(!memcmp(gnssImgInfo[0].tag,"MLGL",4))
	{
		combineInfo.gloffset = gnssImgInfo[0].offset + TRACE_OFFSET;
		combineInfo.bdoffset = gnssImgInfo[1].offset + TRACE_OFFSET;
	}
	else if(!memcmp(gnssImgInfo[0].tag,"MLBD",4))
	{
		combineInfo.bdoffset = gnssImgInfo[0].offset + TRACE_OFFSET;
		combineInfo.gloffset = gnssImgInfo[1].offset + TRACE_OFFSET;
	}
	D("bdoffset is %x,gloffset is %x",combineInfo.bdoffset,combineInfo.gloffset);
	fclose(fp);
	return;
}
void sig_handler_pipe(int signo)
{
	D("%s signo  = %d",__func__,signo);
	return;

}
void sig_handler_abrt(int signo)
{
	D("%s signo  = %d",__func__,signo);
	return;

}
static int gps_init(GpsCallbacks* callbacks)
{
	GpsState*  s = _gps_state;
	int ret = 0;
	int reqCountMax = 3;
	int versionReqMax = 3;
	static int sendAssist = 0;
	int i =0;
    char img_string[16];
    static int img_old = 0;
    static int first_download = 0;
    struct timeval rcv_timeout; 
    
    rcv_timeout.tv_sec = 5;
    rcv_timeout.tv_usec = 0;

	D("gps_init  enter \r\n");
	signal(SIGPIPE, sig_handler_pipe);
	signal(SIGABRT, sig_handler_abrt);
	if (!s->first)
	{
		s->callbacks = *callbacks;
		pthread_mutex_init(&s->mutex,NULL);
		pthread_mutex_init(&s->writeMutex,NULL);
        pthread_mutex_init(&s->socketmutex,NULL);
        pthread_mutex_init(&s->gnsslogmutex,NULL);
		ThreadCreateCond(s,NULL);
#ifdef GNSS_INTEGRATED
        s->hardware_id = SHARKLE_PIKE2_GREENEYE2;//0
#else
        s->hardware_id = GREENEYE2;  
#endif
		gps_config(s);
		get_img_info();
        s->gps_flag = CMD_INIT;
		InitializeCircularBuffer(&s->serialWrite,SERIAL_WRITE_BUUFER_MAX); //maybe shouldn't malloc memory
        gps_state_init(s);
        if(s->hardware_id == GREENEYE2)
        {
            gps_setPowerState(DEVICE_SLEEP);
            ret  = gps_devieManger(GET_GNSS_STATUS);
            if(ret)
            {
                 D("%s first gnss_download failed ",__FUNCTION__);
                 gnss_notify_abnormal(WCND_SOCKET_NAME,&rcv_timeout,"donwload-trigger");
                 gps_setPowerState(DEVICE_DOWNLOADING);
                 return ret;
            }
        }
        else//SHARKLE 
        {
            ;//ret = sharkle_gnss_ready(1);
        }
    }
    else
    {
        if(s->hardware_id == GREENEYE2)
        {
            if(CMD_CLEAN != s->gps_flag)
            {
                ThreadWaitCond(s,200);  
            }
            D("gps_init powerstate should %d(real:%d),\r\n",DEVICE_SLEEP,s->powerState);
            if(s->powerState != DEVICE_SLEEP)
            {
                if(!s->release)
                {
                    E("%s gps power status not sync ,so return",__FUNCTION__);
                    return ret;
                }
                else
                {
                    ret  = gps_devieManger(GET_GNSS_STATUS);
                    if(ret)
                    {
                         D("%s  get gnss_download  status failed ",__FUNCTION__);
                         return ret;
                    }
                 }     
            }
        }
    }
    agpsdata->cur_state = MSB_SI;   //adjust msb si to default
    if(s->init)//it avoid the higer layer error
    {
        D("gps_init has init \r\n");
        return ret;
    }

    if((s->imgmode > 0) && (first_download))
    {
        memset(img_string,0,16);
        ret = config_xml_ctrl(GET_XML,"GPS-IMG-MODE",img_string,CONFIG_TYPE);
        if(ret == 0)
        {
            D("config xml get img-mode is %s",img_string);
            if(!memcmp(img_string,"GNSSBDMODEM",strlen("GNSSBDMODEM")))
            {
                char* pDownreq ="startgnss path=/vendor/etc/gnssbdmodem.bin";
                s->imgmode = BDS_IMG;
                D("imgmode is bds");
                memset(s->binPath,0,sizeof(s->binPath));
                memcpy(s->binPath,pDownreq,strlen(pDownreq));
            }
            else
            {
                char* pDownreq ="startgnss path=/vendor/etc/gnssmodem.bin";
                s->imgmode = GLONASS_IMG;
                D("imgmode is glonass");
                memset(s->binPath,0,sizeof(s->binPath));           
                memcpy(s->binPath,pDownreq,strlen(pDownreq));
            }
        }

    }
    if(s->imgmode == GLONASS_IMG)
	{
		combineInfo.offset = combineInfo.gloffset;
	}
	else
	{
		combineInfo.offset = combineInfo.bdoffset;
	}
    img_old = s->imgmode;
    first_download = 1;
#ifndef GNSS_INTEGRATED
    gps_setPowerState(DEVICE_SLEEP);
    gps_hardware_open();
#endif
    if(!s->sleepFlag)
    {
        ret = gps_wakeupDevice(s);
        if(ret)// wakeup fail
        {
            gnss_notify_abnormal(WCND_SOCKET_NAME,&rcv_timeout,"init-sleep-trigger"); 
            if(!s->release)
            {
                gps_setPowerState(DEVICE_ABNORMAL);
                createDump(s);
                D("%s wake up fail , so return",__FUNCTION__);
                return ret;
            }
            else
            {
                D("%s,gps_wakeupDevice  failed ",__FUNCTION__);
                gps_setPowerState(DEVICE_RESET);  
            }
        }
    }
    
    if((0 == sendAssist)&&(GREENEYE2== s->hardware_id))
    {
        if(DEVICE_SLEEP == s->powerState)
        {
            ret = gps_wakeupDevice(s);
            if(ret)
            {
                gnss_notify_abnormal(WCND_SOCKET_NAME,&rcv_timeout,"sleep-trigger"); 
                if(!s->release)
                {
                    gps_setPowerState(DEVICE_ABNORMAL);
                    createDump(s);
                    D("%s wake up fail before update assist data , so return",__FUNCTION__);
                    return ret;
                }
                else
                {
                    E("%s,gps_wakeupDevice  failed ",__FUNCTION__);
                    gps_setPowerState(DEVICE_RESET);  
                }
            }
        }
        
        if(!s->tsxIsdeliver)
        {
            if(s->tsx_enable == 1)
            {
                s->tsxIsdeliver = 1;
                update_tsx_param();
            }
        }
        gps_sendData(SET_OUTTYPE,NULL); 
        gps_sendData(SET_BASEBAND_MODE,NULL); 
        gps_sendData(REQUEST_VERSION,NULL);
        while(versionReqMax)
		{
			if(WaitEvent(&(s->vesionsync), 500) == 0)   //wait 500ms
			{	
				D("received version now");
				break;
			}
			D("wait 500ms,not get version");
			gps_sendData(REQUEST_VERSION,NULL);
			versionReqMax--;
		}	
		if(versionReqMax == 0)
		{
			E("==============>>>>>>>>>>>>>>can't get ge2 version!!!");
			return -1;
		}
        D("cmd ready is received,begin update assist data");
        update_assist_data(s);
        sendAssist =1;
        D("update assist data end");
    }


    if((s->postlog) ||(s->cmcc))
    {
        D("auto test ,so don't send cp sleep");
    }
    else
    {
        if(s->sleepFlag&&(DEVICE_IDLED == s->powerState))
        {            
            gps_notiySleepDevice(s);
            usleep(10000); 

        }
    }
    
    s->ref = 0;
    s->init =  1;
    s->IdlOnCount = 0;
    s->IdlOffCount = 0;
    s->HeartCount = 0;
    s->keepIdleTime = 0;
    s->happenAssert = 0;
    s->hotTestFlag = 0;
    s->readCount = 0;
    s->gps_flag = CMD_INIT;    // CMD_STOP
    s->etuMode = 0;
     s->supldata = SUPL_NET_RELEASE;
    ret = 0;
    return ret;
}

void  gps_state_start(GpsState*  s)
{
    char cmd = CMD_START;
    int  ret;

    do { ret=write( s->control[0], &cmd, 1 ); }
    while (ret < 0 && errno == EINTR);
    
    if (ret != 1)
    E("send cmd start fail\n");

}

int gps_start(void)
{
    GpsState*  s = _gps_state;

    int ret = 0;

    if (!s->init)
    {
        E("%s: called with uninitialized state", __FUNCTION__);
        //return -1;
    }
    //reset cs mode
    g_startmode_flg = 0;
    g_llh_time_flg  = 0;
    
    if(agpsdata->cur_state < 10 && s->init)
    {
        gstate.status = GPS_STATUS_ENGINE_ON;
        s->callbacks.status_cb(&gstate);
        
        gstate.status = GPS_STATUS_SESSION_BEGIN;
        s->callbacks.status_cb(&gstate);	
    }
    else
    {
        D("controlplane begin start");
        //s->ref = 0;
    }
    pthread_mutex_lock(&s->mutex);
    s->ref++;
    pthread_mutex_unlock(&s->mutex);
    //start navi  
    if(agpsdata->cp_begin == 1)
    {
        //s->cmcc = 1;
        //gps_sendData(SET_CMCC_MODE, NULL); 
        //acquire_wake_lock(PARTIAL_WAKE_LOCK, "GNSSLOCK");
        //usleep(500000);    //in cp mode,just sleep 500ms
        D("CMCC  get gnsslock now");
    }
    
    D("%s: called,ref is %d", __FUNCTION__,s->ref);
	if((agpsdata->ni_status == 1) || (agpsdata->peroid_mode == 1))      //peroid mode add,reset mode
	{
		gps_mode = GPS_COLD_START;
	}
    if(s->ref == 1) 
    {
		if(agpsdata->cp_begin == 1 && (s->cmcc ==1))
		{
			gps_mode = GPS_COLD_START;
			D("in cp mode,gps is first start,just send cold start");
		}
        gps_state_start(s);
		fix_flag = 0;
    }
    //fix_flag = 0;

    return 0;
}
static void gps_state_stop( GpsState*  s )
{
    char  cmd = CMD_STOP;
    int   ret;   
   
    do { ret = write( s->control[0], &cmd, 1); }
    while (ret < 0 && errno == EINTR);

    if (ret != 1)
    E("%s:send CMD_STOP command fail: ret=%d: %s",__FUNCTION__, ret, strerror(errno));

}
void gps_delay_stop(int signo)
{
    GpsState*  s = _gps_state;
    
    D("gps_delay_stop:s->ref is %d, %d",s->ref,signo);
    if ((s->ref == 0))//(s->gps_flag == CMD_START)(s->fd >= 0) && 
    {
        D("%s:send stop cmd",__FUNCTION__);
        gps_state_stop(s);
    }

}
unsigned int get_stop_delay(void)
{
    char value[32];
    unsigned int ret = 0,dtime = 0;
    
    memset(value,0,sizeof(value));
	ret = config_xml_ctrl(GET_XML,"STOP-TIMER",value,CONFIG_TYPE);
	if(ret == 0)
	{
		dtime = str2int(value,value+strlen(value));
	}
    else
    {
        D("can't get stop delay from config.xml");
    }
    return dtime;
}

int gps_stop(void)
{
    GpsState*  s = _gps_state;
    unsigned int delay = 0;

    s->watchdog = 1; //fixing watchdog close issue after rfdata/cw test
    s->gps_flag = CMD_STOP;// it must first ass
	if(s->ref == 0) return 0;
    if (!s->init)
    {
        E("%s: called with uninitialized state!!", __FUNCTION__);
        //return -1;
    }
    pthread_mutex_lock(&s->mutex);
    s->ref--;
    pthread_mutex_unlock(&s->mutex);
    D("gps_stop,s->ref is %d",s->ref);
    
    if(agpsdata->cur_state < 10 && s->init)
    { 
        gstate.status = GPS_STATUS_SESSION_END;
        s->callbacks.status_cb(&gstate);
	}
	
    delay = get_stop_delay();
    D("get stop delay is %d",delay);
    if(delay > 0)
    {
        //signal(SIGALRM,gps_delay_stop);
        gps_delay_stop(SIGALRM);
        //alarm(delay);
        sleep(delay);
    }
    else
    {
        if ((s->fd >= 0) &&(s->ref == 0))
        {
            D("before send stop command");
            gps_state_stop(s);
        }
        if(agpsdata->cur_state < 10 && s->init)
        { 
            gstate.status = GPS_STATUS_ENGINE_OFF; 
            s->callbacks.status_cb(&gstate);	
        }
    }
    return 0;
}

static void gps_state_clean( GpsState*  s )
{
    char  cmd = CMD_CLEAN;
    int   ret;  
   
    do { ret = write( s->control[0], &cmd, 1); }
    while (ret < 0 && errno == EINTR);

    if (ret != 1)
    D("%s:send CMD_STOP command fail: ret=%d: %s",__FUNCTION__, ret, strerror(errno));

}


static void  gps_cleanup(void)
{
    GpsState*  s = _gps_state;
   
    D("%s: called", __FUNCTION__);
	
    if (!s->init)
    {
        E("%s: called with uninitialized state!!", __FUNCTION__);
        return;
    }
    s->ref = 0;
    s->init = 0;
    D("%s: clear init \r\n",__FUNCTION__);
    gps_state_clean(s);
}

static int gps_inject_time(GpsUtcTime time, int64_t timeReference, int uncertainty)
{
    D("gps_inject_time ,time:%lld,timeref= %lld, uncert =%d \n",\
        (long long int)time,(long long int)timeReference,uncertainty);
    return 0;
}

static int  gps_inject_location(double latitude, double longitude, float accuracy)
{
    D("gps_inject_location latitude = %g,longitude= %g accuracy =%f\n",\
        latitude,longitude,accuracy);
    return 0;
}


static void gps_delete_aiding_data(GpsAidingData flags)
{
    GpsState*  s = _gps_state;
    s->deleteAidingDataBit = 0;

    if (!s->init)
    E("%s: called with uninitialized state !!", __FUNCTION__);
    
    D("gps_delete aiding data,flags=%d,before gps_mode=%d\n",flags,gps_mode);
    gps_mode = flags;

    if((gps_mode == GPS_TCXO)||(gps_mode == GPS_CW_CN0)||
        (gps_mode == GPS_RF_GPS)||(gps_mode == GPS_TSX_TEMP)||
        (gps_mode == GPS_NEW_TCXO)||(gps_mode == GPS_RF_GL)||
        (gps_mode == GPS_RF_BD))
    {
		return;
    }

    if(gps_mode == GPS_PS_START)
    {
    	config_xml_ctrl(SET_XML,"POST-ENABLE","TRUE",CONFIG_TYPE);
        s->postlog= 1;
        gps_mode = 0;
		return;
    }
    if(gps_mode == GPS_SET_ASSERT)
    {
        D("assert status is received");
        gps_sendData(SET_ASSERT_STATUS,NULL);
        gps_mode = 0;
		return;
    }
	if(gps_mode == GPS_START_SI)
	{
		D("up mode,will start period si");
		if(agpsdata->cur_state != PEROID_SI)
		{
		    D("up mode,start period si");
		    if(s->ref > 0)
		    {
		        E("test period mode,but gps already start,must stop first");
		        s->ref = 1;
		        gps_stop();	
		        usleep(100000);		    
		    }
		    
            test_gps_mode(s);
		    agpsdata->peroid_mode = 1;
		    agpsdata->cur_state = PEROID_SI;
		    gps_start();
		}
		return;
	}

	if((gps_mode == GPS_STOP_SI) && (agpsdata->cur_state < 10))
	{
		D("up mode,stop period si");
		agpsdata->cur_state = MSB_SI; 
		agpsdata->peroid_mode = 0;
		test_gps_mode(s);
		return;
	}
	
	if(gps_mode == GPS_START_MOLR)
	{
		D("cp mode,start molr");
		if(agpsdata->cur_state != CP_SI)
		{
		    agpsdata->cur_state = CP_SI;
#ifdef GNSS_LCS
                    gnss_lcscp_molr();               
#else
                    start_cp_molr();
#endif
		}
		return;
	}
	if((gps_mode == GPS_GPS_ONLY) || (gps_mode == GPS_GL_ONLY) ||
		(gps_mode == GPS_BDS_ONLY) || (gps_mode == GPS_GPS_GL)||
		(gps_mode == GPS_GPS_BDS))
	{
		//if(s->gps_flag == CMD_START)
		test_gps_mode(s);
		return;
	}
	if(gps_mode == GPS_LTE_EN)
	{
		E("lte open,write to xml");
		config_xml_ctrl(SET_XML,"SPREADORBIT-ENABLE","TRUE",CONFIG_TYPE);
		gps_mode = 0;
		s->lte_open = 1;
		gps_sendData(SET_LTE_ENABLE,NULL);
		return;
	}
	if(gps_mode == GPS_LTE_DIS)
	{
		E("lte off mode");
		config_xml_ctrl(SET_XML,"SPREADORBIT-ENABLE","FALSE",CONFIG_TYPE);
		gps_mode = 0;
		s->lte_open = 0;
		gps_sendData(SET_LTE_ENABLE, NULL);
		return;
	}

    s->deleteAidingDataBit = flags;
    //gps_sendData(SET_DELETE_AIDING_DATA_BIT,NULL);
}

static int gps_set_position_mode(GpsPositionMode mode, GpsPositionRecurrence recurrence,
            uint32_t min_interval, uint32_t preferred_accuracy, uint32_t preferred_time)
{
	unsigned int ret = 0;
	const char *xml_mode = "SUPL-MODE";
	char val[32];
	unsigned int xmode = 0;

	GpsState*  s = _gps_state;

	D("%s: called.", __FUNCTION__);

	if (!s->init) 
	{
		E("called with uninitialized state !!");
		return -1;
	}
	s->recurrence = recurrence;
	s->min_interval = min_interval ? min_interval : 1000;
	s->preferred_accuracy = preferred_accuracy ? preferred_accuracy : 50;
	s->preferred_time = preferred_time;
	agpsdata->agps_enable = mode;

	D("set_position_mode: %d, min_interval: %d \n",mode, min_interval);

	// CTS_TEST
	if(s->min_interval > 2000)
	{
		s->min_interval = 2000;  // limit for cts test
	}
	else if(s->min_interval < 1000)
	{
		s->min_interval = 1000;   // in case of non-standard value
	}
	s->min_interval = s->min_interval/1000;

	memset(val,0,32);
	ret = get_xml_value(xml_mode, val);
	if(!memcmp(val,"msa",3))
	{
		xmode = 2;
		E("xml mode is msa");
	}
	if(!memcmp(val,"msb",3))
	{
		xmode = 1;
		E("xml mode is msb");
	}

	if(xmode != mode)
	E("note:set mode from framwork is not equel xml set!");

	if(agpsdata->cur_state < 10)
	{
		CG_setPosMode(xmode);
	}
	return 0;
}


//the funcitons of _GpsInterface list  end 

static const GpsInterface  _GpsInterface = {
    size:                       sizeof(GpsInterface), 
    init: 			gps_init,
    start:			gps_start,
    stop:			gps_stop,
    cleanup:			gps_cleanup,
    inject_time:		gps_inject_time,
    inject_location:		gps_inject_location,
    delete_aiding_data:		gps_delete_aiding_data,
    set_position_mode:		gps_set_position_mode,
    get_extension: 		gps_get_extension,
};


const GpsInterface* gps_get_hardware_interface(struct gps_device_t* dev)
{
	D("Call gps_get_hardware_interface %p",dev);
	return &_GpsInterface;
}


static int open_gps(const struct hw_module_t* module, char const* name,
                    struct hw_device_t** device)
{
    D("Call open_gps name = %s",name);
    struct gps_device_t *gps_device = malloc(sizeof(struct gps_device_t));
    if (gps_device)
    {
        memset(gps_device, 0, sizeof(struct gps_device_t));
        gps_device->common.tag        = HARDWARE_DEVICE_TAG;
        gps_device->common.version    = 0;
        gps_device->common.module     = (struct hw_module_t*)module;
        gps_device->get_gps_interface = gps_get_hardware_interface;

        *device = (struct hw_device_t*)gps_device;
        return 0;
    }
    return 1; 
}

static struct hw_module_methods_t hw_module_methods = {
    .open = open_gps
};

struct hw_module_t HAL_MODULE_INFO_SYM = {
    .tag = HARDWARE_MODULE_TAG,
    .version_major = 1,
    .version_minor = 0,
    .id = GPS_HARDWARE_MODULE_ID,
    .name = "CG GPS Module",
    .author = "CellGuide",
    .methods = &hw_module_methods,
};

