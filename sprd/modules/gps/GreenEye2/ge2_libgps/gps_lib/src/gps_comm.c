#include <sys/types.h>  
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/stat.h>  
#include <errno.h>  
#include <fcntl.h>  
#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <time.h>
#include <utils/Log.h>
#include <cutils/properties.h>
#include <sys/epoll.h>
#include <termios.h>
#include <cutils/sockets.h>
#include <poll.h>

#include "gps_cmd.h"
#include "gps_common.h"
#include "gps_engine.h"
#include "gnss_log.h"

#define JAN61980      44244
#define JAN11901      15385

extern char gps_log_enable;

extern char *gps_idle_on ;
extern char *gps_idle_off;
static FILE *nmea_fp;
static char *gps_to_nmea;

#ifdef GNSS_ANDROIDN
#ifdef GNSS_INTEGRATED
char *libgps_version = "GE2_16A_W17.42.01,2017.10.20,Android N sharkle debug\r\n";
#else //GNSS_INTEGRATED
char *libgps_version = "GE2_16A_W17.42.01,2017.10.20,Android N GE2 debug\r\n";
#endif //GNSS_INTEGRATED
#else
#ifdef GNSS_LCS
char *libgps_version = "GE2_16A_W17.42.01,2017.10.20  Android6.0,common native lcs  \r\n";
#else
char *libgps_version = "GE2_16A_W17.42.01,2017.10.20  Android6.0,common native \r\n";
#endif
#endif 

static FILE *gUart_fd= NULL;
static FILE *table = NULL;
char logbuf[GNSS_LOG_BUUFER_MAX];
int         gLogType = 0;
int         gWriteFlag = 0;
int         gDropCount = 0;

static char data_capture_flag = 0;
char data_exist_flag = 0;
static FILE *datafp = NULL;
char fname[64] = {0};

typedef struct SCgTime
{
        unsigned short year;		/*< year */
        unsigned short month;		/*< month (0-11) */
        unsigned short day;		/*< day (0-31) */
        unsigned short hour;		/*< hour (0-23) */
        unsigned short minute;		/*< minute (0-59) */
        unsigned short second;		/*< second (0-59) */
        unsigned int  microsec;	/*< microseconds */
} TCgTime;

static const long month_day[2][13] = {
       {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365},
       {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366}
};

static int TimeSystemToGps(TCgTime *aTime, gpstime *pGpsTime)
{
    unsigned int yday, mjd, leap;
    double in_seconds = 0.0, sec_per_week = 0.0, fmjd = 0.0;
    double daysFromEpoc = 0.0;


    in_seconds = (double)aTime->second + ((double)aTime->microsec / (double)1000000.0) ;
    leap = (aTime->year%4 == 0);
    yday = month_day[leap][aTime->month] + aTime->day + 1;

    mjd = ((aTime->year - 1901)/4)*1461 + ((aTime->year - 1901)%4)*365 + yday - 1 + JAN11901;
    fmjd = ((in_seconds/(double)60 + aTime->minute)/(double)60 + aTime->hour)/(double)24;

    pGpsTime->weekno = (unsigned short)((mjd - JAN61980)/7);

    daysFromEpoc = (mjd - JAN61980) - (pGpsTime->weekno*7.0) + fmjd;
    sec_per_week  = daysFromEpoc * 86400;

    pGpsTime->mscount = (unsigned int)(sec_per_week * 1000);
  
    return 0;
}

int OsTimeSystemGet( gpstime *aGpsTime)
{
	struct timespec t = {0};
	TCgTime time = {0};
	struct tm *tm1 = NULL;
	time_t tsec = {0};

	clock_gettime(CLOCK_REALTIME, &t);
	tsec = t.tv_sec;
	tm1 = gmtime(&tsec);

	time.year  = tm1->tm_year + 1900;
	time.month = tm1->tm_mon;
	time.day = tm1->tm_mday - 1;
	time.hour  = tm1->tm_hour;
	time.minute = tm1->tm_min;
	time.second = tm1->tm_sec;
	time.microsec = (t.tv_nsec/1000);

	TimeSystemToGps(&time, aGpsTime);
	
	return 0;
}
void get_sys_time(void)
{
	struct timespec t;
	struct tm *tm1 = NULL;
	time_t tsec = {0};
	TCgTime time;
	
	clock_gettime(CLOCK_REALTIME, &t);
	tsec = t.tv_sec;
	tm1 = gmtime(&tsec);

	time.year  = tm1->tm_year + 1900;
	time.month = tm1->tm_mon; // The number of months since January, in the range 0 to 11
	time.day = tm1->tm_mday - 1;// The day of the month, in the range 1 to 31
	time.hour  = tm1->tm_hour + 8;
	time.minute = tm1->tm_min;
	time.second = tm1->tm_sec;
   fprintf(nmea_fp,"time is:%d-%2d-%2d---%2d:%2d:%2d\n",
	time.year,time.month+1,time.day+1,time.hour,time.minute,time.second);
}
void store_nmea_log(char lnum)
{
    struct stat temp;
    GpsState*  state = _gps_state;  

    if(access("/data/gnss/nmea.log",0) == -1)
    {
        temp.st_size = 0;
    }
    else
    {
        stat("/data/gnss/nmea.log", &temp);
    }

    if(temp.st_size >= 20971520)
    {
        if(nmea_fp != NULL)
        { 
            fclose(nmea_fp);
            nmea_fp = NULL;
        }
        gps_to_nmea = "the nmea log is now over 20M,so overrun it\n";                    
        nmea_fp = fopen("/data/gnss/nmea.log","w+");
        if(nmea_fp != NULL)
            fwrite(gps_to_nmea,1,strlen(gps_to_nmea),nmea_fp);
    }
    else if(nmea_fp == NULL)
    {
        D("create nmea log file now");
        nmea_fp = fopen("/data/gnss/nmea.log","a+");   //apend open
    }

    if(nmea_fp != NULL)
    {
        switch(lnum){
        case log_init:
            gps_to_nmea = "=========>>>>>>>>>gps init is now set\r\n";
            get_sys_time();
            fwrite(gps_to_nmea,1,strlen(gps_to_nmea),nmea_fp);
            fflush(nmea_fp);
            D("libgps init : \n");
            break;
        case log_start:
            get_sys_time();
            gps_to_nmea = "=========>>>>>>>>>gps start is now set\r\n";
            fwrite(gps_to_nmea,1,strlen(gps_to_nmea),nmea_fp);			
            fprintf(nmea_fp," =========>>>>>>>>>gps Real send IDLEOFF is:%d:\n",state->IdlOffCount);
            fwrite(libgps_version,1,strlen(libgps_version),nmea_fp);
            fflush(nmea_fp);
            D("libgps start : %s\n",libgps_version);
            break;
        case log_stop:
            get_sys_time();
            gps_to_nmea = "=========>>>>>>>>>gps stop is now set\r\n";
            fwrite(gps_to_nmea,1,strlen(gps_to_nmea),nmea_fp);
            fprintf(nmea_fp,"=========>>>>>>>>>gps Real send IDLON is:%d:\n",state->IdlOnCount);
            fclose(nmea_fp);
            nmea_fp = NULL;
            D("libgps stop  \n");
            break;
        case log_cleanup:
            get_sys_time();
            gps_to_nmea = "=========>>>>>>>>>gps cleanup is now set\r\n";
            fwrite(gps_to_nmea,1,strlen(gps_to_nmea),nmea_fp);
            D("libgps cleanup \n");
            fclose(nmea_fp);
            nmea_fp = NULL;
            break;
        case log_nothing:
        default:
            return;
    }
  }

}

void nmea_save_nmea_data(NmeaReader*  r)
{
    if((gps_log_enable == 1) && (nmea_fp != NULL))
    {
        fwrite(r->in,r->pos,1,nmea_fp);
        fflush(nmea_fp);
    }
}


int  gps_power_Statusync(int value)
{
    GpsState* s = _gps_state;
    int ret = 0;

    D("gps_power_Statusync enter");

	switch(value)
	{
		case IDLE_ON_SUCCESS:
		{
          
            if(s->powerState == DEVICE_IDLED)
            {
                D("idle on success! \r\n");
                s->powerErrorNum = 0;
            }
            else
            {
                if(s->powerState == DEVICE_IDLING)
                {
                  ThreadWaitCond(s,2000);                  
                }
                //it need to repeat it again when powererrornum < 3
                if((s->powerState == DEVICE_IDLING)&& (s->powerErrorNum < POWER_ERROR_NUMBER_MAX))
    			{
    			    ret = 1; 
                    s->powerErrorNum++;
    				//gps_sendData(IDLEON_STAT, gps_idle_on);
                    E("idle on fail,maybe some wrong \r\n");
    			}
            }       
                        
			break;
		}
		case IDLE_OFF_SUCCESS:
		{
			if(s->powerState == DEVICE_WORKED)
			{
                D("idle off success \r\n");
                s->powerErrorNum = 0;
			}
            else
            {
                if(s->powerState == DEVICE_WORKING)
                {
                    ThreadWaitCond(s,200); 
                }  
                //it need to repeat it again when powererrornum < 3
                if((s->powerState == DEVICE_WORKING)&& (s->powerErrorNum < POWER_ERROR_NUMBER_MAX))
                {
    			   ret = 1;
                   s->powerErrorNum++;
                   E("idle off fail,maybe some wrong \r\n");
                }  
            }
			break;
		}
        case POWER_WAKEUP:
        {
            if(s->powerState == DEVICE_IDLED)
            {
                D("it wakeup success");
                s->powerErrorNum = 0;
            }
            else
            {
                if(s->powerState == DEVICE_WAKEUPING)
                {
                    if(4 != ((s->powerErrorNum)%5))
                    {
                        ThreadWaitCond(s,10); 
                    }
                    else
                    {
                        ThreadWaitCond(s,2000);
                    }
                }
                
                if(s->powerState == DEVICE_WAKEUPING)
                {
                     ret = 1;
                     s->powerErrorNum++;
                     if(0 == ((s->powerErrorNum)%100))
                     E("wake up fail %d,so try again",s->powerErrorNum);
                }
            }
            break;
        }
        case POWER_SLEEPING:
            if(s->powerState == DEVICE_SLEEP)
            {
                D("it sleep success");
                s->powerErrorNum = 0;
            }
            else
            {
                if(s->powerState == DEVICE_SLEEPING)
                {
                    ThreadWaitCond(s,200); 
                }  
                //it need to repeat it again when powererrornum < 3
                if((s->powerState == DEVICE_SLEEPING)&& (s->powerErrorNum < POWER_ERROR_NUMBER_MAX))
                {
    			   ret = 1;
                   s->powerErrorNum++;
                   gps_notiySleepDevice(s);
                   E("it sleep  fail,maybe some wrong \r\n");
                }  
            }
            
            break;
		
		default:// we dont't care the  power on event 
		        E("the gps_power_Statusync value =%d \r\n",value);
			break;
	
	}
    return ret;
}

int  epoll_register( int  epoll_fd, int  fd )
{
    struct epoll_event  ev;
    int  ret, flags;

    /* important: make the fd non-blocking */
    flags = fcntl(fd, F_GETFL);
    if(fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        E("the %s fcntl set O_NONBLOCK fail",__FUNCTION__);
    }

    ev.events  = EPOLLIN;
    ev.data.fd = fd;
    do {
        ret = epoll_ctl( epoll_fd, EPOLL_CTL_ADD, fd, &ev );
    } while (ret < 0 && errno == EINTR);
    return ret;
}

int  epoll_deregister( int  epoll_fd, int  fd )
{
    int  ret;
    do 
    {
        ret = epoll_ctl( epoll_fd, EPOLL_CTL_DEL, fd, NULL );
    } while (ret < 0 && errno == EINTR);
    return ret;
}

void send_agps_cmd(char cmd)
{
    int   ret;  
    GpsState* s = _gps_state;
	int times = 0;
    do { 
		ret = write( s->asock[0], &cmd, 1); 
		times++;
	}
    while (ret < 0 && errno == EINTR && times < 3);
    E("send agps cmd to server");
    if (ret != 1)
        E("%s:send CMD_STOP command fail: ret=%d: %s",__FUNCTION__, ret, strerror(errno));
}



static void  gps_getPowerResponse(TCmdData_t* pPacket)
{
    GpsState* s = _gps_state;

	
	D("gps_getPowerResponse	subtype = %d\r\n",pPacket->subType);
    switch(pPacket->subType)
    {
  	    case GNSS_LIBGPS_POWERON_STATUS_SUCCESS:
		gps_setPowerState(DEVICE_POWERONED);
		break;

  	    case GNSS_LIBGPS_IDLON_STATUS_SUCCESS:
		gps_setPowerState(DEVICE_IDLED);
		break;

  	    case GNSS_LIBGPS_IDLOFF_STATUS_SUCCESS:
		gps_setPowerState(DEVICE_WORKED);
		break;

        case GNSS_LIBGPS_SHUTDOWN_STATUS_SUCCESS:
		gps_setPowerState(DEVICE_POWEROFFED);
		break;

            case GNSS_LIBGPS_RESET_STATUS_SUCCESS:
		gps_setPowerState(DEVICE_POWERONED);
		break;

  	    case GNSS_LIBGPS_POWERON_STATUS_FAILED:
	    case GNSS_LIBGPS_IDLON_STATUS_FAILED:
        case GNSS_LIBGPS_IDLOFF_STATUS_FAILED:
        case GNSS_LIBGPS_RESET_STATUS_FAILED:
        case GNSS_LIBGPS_SHUTDOWN_STATUS_FAILED:
		s->powerErrorNum++;
		break;

	    default:
		E("gps_getPowerResponse  subtype = %d\r\n",pPacket->subType);
		break;
    }
    ThreadSignalCond(s);
}

static void  gps_getNotified(TCmdData_t* pPacket)
{
    GpsState* s = _gps_state;
    unsigned char  state;
    
    if(GNSS_NOTIFIED_DEVICE_STATE_SUBTYPE == pPacket->subType)
    {
        if(pPacket->length == 2)
        {
            state = pPacket->buff[0];
            if(state == GNSS_DEVICE_READY_STATE)
            {
                D("gps_getNotified Device %d Ready ",pPacket->buff[1]);
                gps_setPowerState(DEVICE_IDLED);
                ThreadSignalCond(s);
            }
            else if(state == GNSS_DEVICE_SLEEP_STATE)
            {
                D("gps_getNotified sleep %d success",pPacket->buff[1]);
                gps_setPowerState(DEVICE_SLEEP);
                
#ifdef GNSS_INTEGRATED
                sharkle_gnss_ready(0);
#endif
                ThreadSignalCond(s);
            }
        }
        else
        {
            E("gps_getNotified data len error");
        }
    }
}



int get_one_nmea(GpsState* s,unsigned char *buf,int len)
{
	int pos = 0;
	int i = 0;
	int temp = len;
	//E("get one nmea enter");
	while(!((buf[i] == '\r') && (buf[i+1] == '\n')))
	{
		s->NmeaReader[0].in[i] = buf[i];
		i++;	
		temp--;	
		if(temp <= 0)
		{
			return 0;
		}
	}
	s->NmeaReader[0].in[i] = buf[i];
	if(s->logswitch)
	{
	    D("get one nmea,buf[i]=%d",s->NmeaReader[0].in[i]);
	}
    s->NmeaReader[0].in[i+1] = buf[i+1];
	return i+2;
}


void parseNmea(unsigned char *buf,int len)
{
    GpsState*  s = _gps_state;  
	unsigned char *temp = buf;
	//it only save modem slog and local log 
    if((0 == s->postlog)&&((s->slog_switch)||((0 == s->release) && (s->logswitch))))
    {
        //D("the nmea log len = %d\r\n",len);
	    preprocessGnssLog(buf,len,GNSS_LIBGPS_NMEA_SUBTYPE);
    }

	while(len > 9)
	{
		//E("parse nmea enter");
		memset(s->NmeaReader[0].in,0,(NMEA_MAX_SIZE+1));
		s->NmeaReader[0].pos = get_one_nmea(s,temp,len);
		if(s->NmeaReader[0].pos == 0)
		{
			E("read last nema log is not complete");
			break;
		}
		temp = temp + s->NmeaReader[0].pos;
		len = len - s->NmeaReader[0].pos;
		//D("%s \r\n",s->NmeaReader[0].in);
		nmea_reader_parse(s->NmeaReader);
	}
    return;
}

void parsePstat(unsigned char *buf,int len)
{
    GpsState*  s = _gps_state;
	unsigned char *temp;
	unsigned char tempdata[128];
	int length = 120;

	if(memcmp(buf,"$PSTAT",6) != 0)
	{
		return;
	}

	D("the pstat log len = %d\r\n",len);

	memcpy(tempdata, buf, length);
	tempdata[length++] = '\r';
	tempdata[length++] = '\n';
	len = length;

	temp = tempdata;

	//s->callbacks.nmea_cb(s->NmeaReader[0].fix.timestamp, (const char*)temp, (int)len);
    savenmealog2Buffer((const char*)temp, len);
	while(len > 9)
	{
		memset(s->NmeaReader[0].in,0,(NMEA_MAX_SIZE+1));
		s->NmeaReader[0].pos = get_one_nmea(s,temp,len);
		if(s->NmeaReader[0].pos == 0)
		{
			E("read last pstat log is not complete");
			break;
		}
		temp = temp + s->NmeaReader[0].pos;
		len = len - s->NmeaReader[0].pos;
		if(s->logswitch)
		{
		    D("%s \r\n",s->NmeaReader[0].in);
		}
		nmea_reader_parse(s->NmeaReader);
	}
    return;
}

void parseMSA(unsigned char *buf,int len)
{
    E("will parse msa");

	if(!set_measurement_data((const unsigned char*)buf,len))
		send_agps_cmd(MEASURE_CMD); 
}

//it create tcp/ip common socket fd , used in kaios 
static int socket_tcp_client()
{
    int socket_fd = -1;
    struct sockaddr_in addr;
    int ret = 0;
    
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    if(-1 != socket_fd)
    {
        memset(&addr, 0, sizeof addr);
        addr.sin_family = AF_INET;
        addr.sin_port = htons(SLOG_MODEM_SERVER_PORT);
        ret = inet_pton(AF_INET, LOCAL_HOST, &addr.sin_addr);
        if(ret > 0)
        {
            ret = connect(socket_fd, (const struct sockaddr *)&addr, sizeof addr);
            if(0 == ret)
            {
                D("create slogmodem client socket success %d",socket_fd);
            }
            else
            {
                close(socket_fd);
                socket_fd = -1;
                E("can't connect slogmodem client socket, %s",strerror(errno));
            }  
        }
        else
        {
            close(socket_fd);
            socket_fd = -1;
            E("can't inet_pton Fail to convert to network adress, %s",strerror(errno));
        }
    }
    else
    {
        E("can't create slogmodem client socket, %s",strerror(errno));
    }
    return socket_fd;

}

//save dump into slogmodem
void save_dump_slogmodem(char* message, int length)
{
	GpsState *s = _gps_state;
	int wl = 0;
	int retry_count = 1;
	int ret = -1;

	D("%s: length=%d, dumpfile=%s", __func__, length, message);

	if(s->slogmodem_fd <= 0)
	{
        char kiaos[128];
	    ret = property_get("ro.moz.ril.numclients",kiaos,NULL);//NULL
	    
        if(ret > 0)//kiaos tcp socket 
        {
            D(" kiaos tcp socket");
            s->slogmodem_fd = socket_tcp_client();
        }
        else //android os socket 
        {
            D(" android socket");
            // connect slogmodem
		    s->slogmodem_fd = socket_local_client( "slogmodem", ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
        }


		if(s->slogmodem_fd < 0)
		{
			E("%s: connect slogmodem fail", __func__);
			return;
		}

		// set non block
		long flags = fcntl(s->slogmodem_fd,F_GETFL);
		flags |= O_NONBLOCK;
		ret = fcntl(s->slogmodem_fd,F_SETFL,flags);
		if(-1 == ret)
		{
			E("%s: set slogmodem socket to O_NONBLOCK error", __func__);
			close(s->slogmodem_fd);
			s->slogmodem_fd = 0;
			return;
		}
	}

	wl = write(s->slogmodem_fd, message, length);
	if(wl < 0)
	{
		E("%s: write to slogmodem socket error", __func__);
		close(s->slogmodem_fd);
		s->slogmodem_fd = 0;
		return;
	}

	//close(slogmodem_fd);
}

int gps_saveCodedump(char *buf,int length)
{
    static FILE* fMemDump = NULL;
    static int revLen = 0;
    int totalLen = 0; 
    totalLen = CODEDUMP_MAX_SIZE; 
    int ret = 0;

    if(fMemDump)
    {      
        fwrite(buf,length,1,fMemDump); 
        fflush(fMemDump);
        revLen += length;
        if(totalLen == revLen)
        {
            fclose(fMemDump);
            fMemDump = NULL;
            revLen = 0;
            ret = 1;
            D("memdump(code) end");
			save_dump_slogmodem("COPY_FILE GNSS DUMP /data/gnss/codedump.mem\n", strlen("COPY_FILE GNSS DUMP /data/gnss/codedump.mem\n"));
        }
        
    }
    else
    {
        fMemDump = fopen("/data/gnss/codedump.mem","wb+");
        if(!fMemDump)
        {
            E("%s create dump error %s\r\n", __func__, strerror(errno));
            return ret;
        }
        else
        {
            D("memdump(code) begin");
            fwrite(buf,length,1,fMemDump); 
            fflush(fMemDump);
			chmod("/data/gnss/codedump.mem", 0664);
            revLen += length;
        }
    }

    return ret;

}

void gps_saveDatadump(char *buf,int length)
{
    static FILE* fMemDump = NULL;
    static int revLen = 0;
    int totalLen = DATADUMP_MAX_SIZE;
    
    if(fMemDump)
    {      
        fwrite(buf,length,1,fMemDump); 
        fflush(fMemDump);
        revLen += length;
        if(totalLen == revLen)
        {
            fclose(fMemDump);
            fMemDump = NULL;
            revLen = 0;
            D("memdump(data) end");
			save_dump_slogmodem("COPY_FILE GNSS DUMP /data/gnss/datadump.mem\n", strlen("COPY_FILE GNSS DUMP /data/gnss/datadump.mem\n"));
        } 
    }
    else
    {
        fMemDump = fopen("/data/gnss/datadump.mem","wb+");
        if(!fMemDump)
        {
            E("%s create dump error %s\r\n", __func__, strerror(errno));
            return;
        }
        else
        {
            D("memdump(data) begin");
            fwrite(buf,length,1,fMemDump); 
            fflush(fMemDump);
			chmod("/data/gnss/datadump.mem", 0664);
            revLen += length;
        }
    }

}

void gps_savePchannelDump(char *buf,int length)
{
	static FILE* fMemDump = NULL;
	static int revLen = 0;
	int totalLen = PCHANNELDUMP_MAX_SIZE;
    GpsState *s = _gps_state;
    struct timeval rcv_timeout;

	if(fMemDump)
	{
		fwrite(buf,length,1,fMemDump);
		fflush(fMemDump);
		revLen += length;
		if(totalLen == revLen)
		{            
			fclose(fMemDump);
			fMemDump = NULL;
			revLen = 0;
			D("memdump(pchannel) end");
			save_dump_slogmodem("COPY_FILE GNSS DUMP /data/gnss/pchanneldump.mem\n", strlen("COPY_FILE GNSS DUMP /data/gnss/pchanneldump.mem\n")); 
            rcv_timeout.tv_sec = 5;
            rcv_timeout.tv_usec = 0;
            if(ASSERT_TYPE_MANUAL == s->assertType)
            {
                gnss_notify_abnormal(WCND_SOCKET_NAME,&rcv_timeout,"manual-trigger");
            }
            else
            {
                gnss_notify_abnormal(WCND_SOCKET_NAME,&rcv_timeout,"firmware-trigger");

            }
            if(0 == s->release)
			{
			    gps_setPowerState(DEVICE_ABNORMAL);
				s->HeartCount = 0;
			}
			else
			{
				D("%s, create assert and set reset ",__FUNCTION__);
				s->HeartCount = 0;
                gps_setPowerState(DEVICE_RESET);
			}
		}
	}
	else
	{
		fMemDump = fopen("/data/gnss/pchanneldump.mem","wb+");
		if(!fMemDump)
		{
			E("%s create pchanneldump error %s\r\n", __func__, strerror(errno));
			return;
		}
		else
		{
			D("memdump(pchannel) begin");
			fwrite(buf,length,1,fMemDump);
			fflush(fMemDump);
			chmod("/data/gnss/pchanneldump.mem", 0664);
			revLen += length;
		}
	}
}

void GPS_HeartCountRsp(GpsState* pGPSState)
{

    pthread_mutex_lock(&pGPSState->mutex);
    if(pGPSState->waitHeartRsp)
    {
        pGPSState->waitHeartRsp = 0;
        pGPSState->HeartCount = 0;
    }
    pthread_mutex_unlock(&pGPSState->mutex);
    D("&&& GPS_HeartCountRsp  HeartCount %d pid =%d,ppid =%d\r\n",
        pGPSState->HeartCount,getpid(),getppid());
}

void save_tsx_data(unsigned char *buf, int length)
{
	FILE* fp = NULL;
	TsxData cpTsxData;
	int ret = 0;

	D("save_tsx_data enter");

    if(length != sizeof(cpTsxData))
    {
        E("the struct TsxData size:%d is not suite data length:%d ",(int)sizeof(cpTsxData),length);
    }

	memcpy(&cpTsxData, buf, sizeof(cpTsxData));

	fp = fopen("/productinfo/tsx.dat", "w+");
	chmod("/productinfo/tsx.dat", 0666);
	if(fp != NULL)
	{
		ret = fwrite(&cpTsxData, sizeof(TsxData), 1, fp);
		fclose(fp);
	}
	else
	{
		E("save_tsx_data: fopen fail errno=%d, strerror(errno)=%s", errno, strerror(errno));
	}
}

void saveRFData(unsigned char  *data,unsigned short len)
{
	GpsState*  s = _gps_state;

	pthread_mutex_lock(&s->saveMutex);

	int wn = 0;
    if(NULL == datafp)
    {
	    datafp = fopen(fname,"a+");
    }

	wn = fwrite(data, len, 1, datafp);
	if(wn <= 0)
	{
		E("#####error :%s", strerror(errno));

		fflush(datafp);
		fclose(datafp);
		datafp = NULL;
		datafp = fopen(fname,"a+");
		wn = fwrite(data, len, 1, datafp);
	}

	//fflush(datafp);
	//fclose(datafp);
	//datafp = NULL;

	pthread_mutex_unlock(&s->saveMutex);
}




static void gps_getRFData(TCmdData_t *pPacket)
{
	char response[256];
	int i = 0;
	time_t t;
	struct tm *p;
    static int rawcount = 0; 
    GpsState*  s = _gps_state;
	//char fname[64] = {0};

	if(NULL == pPacket)
	{
		return;
	}

	//D("####data_capture_flag=%d",data_capture_flag);

	if(data_capture_flag == 1)
	{
	    rawcount++;
        saveRFData(pPacket->buff, pPacket->length);
        if( rawcount > RF_DATA_RAWCOUNT)//16384
        {
            if(!memcmp(&pPacket->buff[i],"DATA_CAPTURE_END",strlen("DATA_CAPTURE_END")))
            {
                D("####data capture end.");
                data_capture_flag = 0;
                data_exist_flag = 1;
                 rawcount = 0;
                fclose(datafp);
                datafp = NULL;   
                gLogType = GNSS_TYPE_LOG_RF;             
                SignalEvent(&s->gnssLogsync);
            }
            else
            {
                D("####data capture  should end, but not get the DATA_CAPTURE_END");
            }
        }

	}

	if(data_capture_flag == 0)
	{
		for(i = 0; i < pPacket->length; i++)
		{
			if(!memcmp(&pPacket->buff[i],"DATA_CAPTURE_BEGIN",strlen("DATA_CAPTURE_BEGIN")))
			{
				D("####data capture begin.");
				data_capture_flag = 1;
				data_exist_flag = 0;

				t = time(NULL);
				p = gmtime(&t);

				memset(fname,0,64);
				sprintf(fname, "data/gnss/%d-%d-%d_%d-%d-%d.txt",1900+p->tm_year,1+p->tm_mon, p->tm_mday, 8+p->tm_hour,p->tm_min,p->tm_sec);
				D("%s\n", fname);
                rawcount = 0;

				if(data_capture_flag == 1)
				{
					saveRFData(pPacket->buff, pPacket->length);
				}
				break;
			}
		}
	}

}

extern GpsData gps_msr;

static void  gps_getData(TCmdData_t* pPacket)
{
    GpsState* s = _gps_state;
    int i = 0,ret = 0;
    static int flagassert =0;
    static softbit_data softbit[34];
    unsigned int svid = 0,clearbit = 0;
    static softbit_receivebuf stempbuf;
    static softbit_sendbuf sendbuf;
    static char fia = 0;
    //static EphExt ephext;
	TCmdData_t Pak;
	int len = 0;
	
	if(fia == 0)
    {
        memset(softbit,0,sizeof(softbit));
        fia = 1;
    }
    if(NULL == pPacket)
    {
        E("get data,packet is null");
        return;
    }

    switch(pPacket->subType)
    {
        case GNSS_LIBGPS_LOG_SUBTYPE:
			if(s->float_cn_enable == 1)
			{
		        parsePstat(pPacket->buff, pPacket->length);
			}
			if(s->rftool_enable == 1)
			{
				gps_getRFData(pPacket);
			}
         #if 0
			else
			{
				if((s->postlog == 1) || (s->cw_enable == 1))	 // HW_CW
		        {
		           savenmealog2Buffer((const char*)pPacket->buff, (int)pPacket->length);
		        }
			}
          #endif
        	break;
            
        case GNSS_LIBGPS_NMEA_SUBTYPE:
            parseNmea(pPacket->buff,pPacket->length);
            break;
        case GNSS_LIBGPS_SOFTBIT_SEND_SUBTYPE:
            memset(&stempbuf,0,sizeof(softbit_receivebuf));
            memset(&Pak,0,sizeof(TCmdData_t));
            memcpy(&stempbuf,pPacket->buff,sizeof(softbit_receivebuf));
            D("===>>>received softbit request,svid:%d",stempbuf.sv_id);
            if(stempbuf.sv_id > 33)
            {
                E("request softbit not valid");
                break;
            }
            svid = stempbuf.sv_id;
            D("write_index before:%d",softbit[svid].softbit_buf_write_index);
            
            softbit[svid].merge_time = stempbuf.merge_time;
            softbit[svid].softbit_buf_write_index = stempbuf.start_index + stempbuf.len;
            if(softbit[svid].softbit_buf_write_index >= 1500)
            {
                E("svid %d write index is %d,should merge it",svid,softbit[svid].softbit_buf_write_index);
                len = 1500-stempbuf.start_index;
                memcpy(&softbit[svid].softbit_buf[stempbuf.start_index],stempbuf.writebuf,len);
                memcpy(&softbit[svid].softbit_buf[0],stempbuf.writebuf+len,stempbuf.len - len); 
                softbit[svid].softbit_buf_write_index = softbit[svid].softbit_buf_write_index - 1500;
            }
            else
            {
                memcpy(&softbit[svid].softbit_buf[stempbuf.start_index],stempbuf.writebuf,stempbuf.len);
            }
            D("write_after:%d",softbit[svid].softbit_buf_write_index);

            Pak.type = GNSS_LIBGPS_DATA_TYPE;
            Pak.subType = GNSS_LIBGPS_SOFTBIT_SEND_SUBTYPE;
            Pak.length = sizeof(softbit_sendbuf);
            sendbuf.sv_id = stempbuf.sv_id;
            sendbuf.write_index = softbit[svid].softbit_buf_write_index;
            if((softbit[svid].softbit_buf_write_index >= 1450) && (softbit[svid].merge_time == 0))  //1450 to 1449
            {
                D("next time will be merge,so should send merge data");
                memcpy(sendbuf.merge,&softbit[svid].softbit_buf[0],sizeof(sendbuf.merge));
                memcpy(Pak.buff,&sendbuf,Pak.length);
                gps_adingData(&Pak);
                break;
            }
            if(softbit[svid].merge_time != 0)
            {
                D("merge time is not zero,all data should merged");
                len = softbit[svid].softbit_buf_write_index;
                if(len <= 1440)
                { 
                    memcpy(sendbuf.merge,&softbit[svid].softbit_buf[len],sizeof(sendbuf.merge));
                }
                else
                {
                    memcpy(sendbuf.merge,&softbit[svid].softbit_buf[len],1500-len);
                    memcpy(sendbuf.merge+1500-len,&softbit[svid].softbit_buf[0],sizeof(sendbuf.merge)+len-1500);
                }
                memcpy(Pak.buff,&sendbuf,Pak.length);
                gps_adingData(&Pak);
            }
            break;
        case GNSS_LIBGPS_SOFTBIT_CLEAR_SUBTYPE:
            D("clear softbit svid is %d,clear type is %d",pPacket->buff[0],pPacket->buff[1]);
            if(pPacket->buff[0] > 32)
            {
                E("clear data svid is error");
                break;
            }
            memset(&softbit[pPacket->buff[0]],0,sizeof(softbit_data));
            break;
        case GNSS_LIBGPS_ASSERT_SUBTYPE:
			E("$$$$ assert %s\r\n",pPacket->buff);	
            {
			    //s->callbacks.nmea_cb(s->NmeaReader[0].fix.timestamp,
			                        //(const char*)pPacket->buff,
			                        //(int)pPacket->length); 
                savenmealog2Buffer((const char*)pPacket->buff,(int)pPacket->length);
            }
            break;
		case GNSS_LIBGPS_MEMDUMP_DATA_SUBTYPE:
            //userdebug and have not happen crash 
            if(0== s->happenAssert)
            {
                gps_saveDatadump((char*)pPacket->buff,pPacket->length);
            }
          break;
        case GNSS_LIBGPS_MEMDUMP_CODE_SUBTYPE:
#ifdef GNSS_INTEGRATED
            ret = gnss_request_memdumpl();
#else
            ret = gps_saveCodedump((char*)pPacket->buff,pPacket->length);
#endif

			break;
		case GNSS_LIBGPS_MEMDUMP_PCHANNEL_SUBTYPE:
			gps_savePchannelDump((char*)pPacket->buff,pPacket->length);
     
			break;
        case GNSS_LIBGPS_MSA_MEASUREMENT_SUBTYPE:
            if(0 == s->etuMode)
            {
                parseMSA(pPacket->buff,pPacket->length);
            }
            else
            {
                D("in ETU work, so don't deal with gnss measurement");
            }
            break;
        case GNSS_LIBGPS_OBS_SUBTYPE:
			if((s->lte_open == 1) && (s->lte_running == 1)&&(s->etuMode == 0))
			{
		        SF_updateObsTask((char*)pPacket->buff,(int)pPacket->length);

			}
            break;
        case GNSS_LIBGPS_NAVIGATION_SUBTYPE:
            if(0 == s->etuMode)
            {
                D("gnss navigation type is received");
                memset(&agpsdata->navidata,0,sizeof(navigation_type));
                if(pPacket->length == sizeof(navigation_type))
                {
                   D("begin save navigation data");
                   memcpy(&agpsdata->navidata,pPacket->buff,pPacket->length);
                   send_agps_cmd(NAVIGATION_CMD); 
                }
            }
            else
            {
                D("in ETU work, so don't deal with gnss navigation");
            }
            break;
        case GNSS_LIBGPS_GPSCLOCK_SUBTYPE:
            D("gnss gpsclock  is received");

            if(pPacket->length == sizeof(GpsClock))
            {
               D("begin save gpsclock data");
               D("clock leap_second:%d,bias_ns:%lf",gps_msr.clock.leap_second,gps_msr.clock.bias_ns);
               //memcpy(&gps_msr.clock,pPacket->buff,pPacket->length);
			   //gps_msr.clock.flags = 0x01;
			   //gps_msr.clock.full_bias_ns = 1.15*pow(10,18);
            }
			break;
        case GNSS_LIBGPS_TEST_SUBTYPE:
        {
            if(s->watchdog)
            {
                GPS_HeartCountRsp(s);
            }
            else
            {
                E("it should not recv the heart response!");
            }
            break;
        }
		case GNSS_LIBGPS_SAVE_TSX_TEMP:
			save_tsx_data(pPacket->buff, pPacket->length);
			break;
		case GNSS_LIBGPS_CALI_TSX_TEMP:
			get_tsx_temp(pPacket->buff, pPacket->length);
			break;
		case GNSS_LIBGPS_ELLIPES_SUBTYPE:
			if(pPacket->length == sizeof(int)*4)
			{
				memcpy(&agpsdata->orientationMajorAxis,pPacket->buff,sizeof(int));
				memcpy(&agpsdata->uncertaintySemiMajor,pPacket->buff+sizeof(int),sizeof(int));
				memcpy(&agpsdata->uncertaintySemiMinor,pPacket->buff+sizeof(int)*2,sizeof(int));
				memcpy(&agpsdata->uncertaintyAltitude,pPacket->buff+sizeof(int)*3,sizeof(int));
				D("ellipse subtype is received");
			}
			else
			{
				D("ellipse sbutype lenth is %d,maybe error",pPacket->length);
			}
        default:
            break;
    }
    return;
}

static void  gps_getAdiData(TCmdData_t * pPacket)
{
    GpsState* s = _gps_state;
	int ret = 0;
	
	if(NULL == pPacket)
	{
        return;
	}

	switch(pPacket->subType)
	{
		case GNSS_LIBGPS_AIDING_ASSISTANCE_SUBTYPE:
			D("start agps si session");
			if(pPacket->length == sizeof(int))
			{
			    memcpy(&agpsdata->assistflag,pPacket->buff,sizeof(int));
				D("assist request flag from firmware is 0x%x", agpsdata->assistflag);
				if(agpsdata->assistflag == 0x7)
				{
					agpsdata->assistflag = 0x134;
				}
			}
			else
			{
			    agpsdata->assistflag = 0x134;
			}
            send_agps_cmd(START_SI);
			break;
		default:
		{
			//assser(0);
			break;
		}
	}
	

}


static void gps_saveFlashCfg(TCmdData_t * pPacket)
{
    GpsState* s = _gps_state;
	int ret = 0;
	int offset = 0;
	
	if(NULL == pPacket)
	{
        return;
	}

	if(pPacket->subType < GNSS_LIBGPS_FLASH_MAX_LEN_SUBTYPE)
	{
		E("begin save flash data,subtype is %d,lenth is %d",pPacket->subType,pPacket->length);
		if(!s->fFlashCfg)
		{	
			//char tmp[GNSS_FLASH_BLOCK_SIZE];
			//int i = 0;
			
			//first open the file 
			if(access("/data/gnss/FlashCfg.cg",0) != -1)
			{
				s->fFlashCfg = fopen("/data/gnss/FlashCfg.cg","rb+");
			}				
			else
			{
				s->fFlashCfg = fopen("/data/gnss/FlashCfg.cg","wb+");
			}
			if(!s->fFlashCfg)
			{
				E("the create  flash error %s\r\n",strerror(errno));
				return;
			}
			#if 0
			memset(tmp,1,GNSS_FLASH_BLOCK_SIZE);
			while(i < GNSS_LIBGPS_FLASH_MAX_LEN_SUBTYPE)
			{
				offset += GNSS_FLASH_BLOCK_SIZE;
				WriteFile(s->fFlashCfg,tmp ,GNSS_FLASH_BLOCK_SIZE,offset);
				i++;
			}
			#endif
		}
		fseek(s->fFlashCfg,pPacket->subType * GNSS_FLASH_BLOCK_SIZE,SEEK_SET);
		fwrite(pPacket->buff,pPacket->length,1,s->fFlashCfg);   //lenth must limit by 2038
		fflush(s->fFlashCfg);
		//offset = pPacket->subType * GNSS_FLASH_BLOCK_SIZE;	
		//WriteFile(s->fFlashCfg, (unsigned char* )pPacket->buff,pPacket->length,offset);			 
	}
	else
	{
		E("the save flash cfg index error\r\n");
	}
}



/*--------------------------------------------------------------------------
	Function:  gps_getParaData

	Description:
		it  parse lte and some other para set and request 
	Parameters:
	 void: 
	Return: SUCCESS:0 ; FAILED : others 
--------------------------------------------------------------------------*/

static void gps_getParaData(TCmdData_t *pPacket)
{
    GpsState*  s = _gps_state;  
	if(NULL == pPacket)
	{
        return;
	}
	switch(pPacket->subType)
	{
		case GNSS_LIBGPS_SET_LTE_SUBTYPE:
			if((s->lte_open == 1) && (s->lte_running == 1))
			{
				D("lte request is received");
				parse_lte_request((char *)pPacket->buff,pPacket->length);
			}
			break;
		default:
			break;
    }
}

#ifdef GNSS_REG_WR
static void gps_getRegValue(TCmdData_t *pPacket)
{
	GpsState*  s = _gps_state;  
	if(NULL == pPacket)
	{
		return;
	}
	
	switch(pPacket->subType)
	{
		case GNSS_LIBGPS_REG_VALUE_SUBTYPE:
			D("register value received");
			parse_register_value((char *)pPacket->buff,pPacket->length);
			break;
		default:
			break;
    }
}
#endif
/*--------------------------------------------------------------------------
	Function:  gps_getResponse

	Description:
		it  parse response from ge2,these response is answer for libgps request. 
	Parameters:
 		packet in uart: 
	Return:void
--------------------------------------------------------------------------*/


static void gps_getResponse(TCmdData_t *pPacket)
{
	GpsState*  s = _gps_state;
	char response[256];
	char log_string[16];
	char imgmode[16];
	int cpmode = 0,i = 0;
	gpstime TT;
	memset(response,0,sizeof(response));
	E("getresponse subtype is %d",pPacket->subType);
	if(NULL == pPacket)
	{
        return;
	}
	switch(pPacket->subType)
	{
		case GNSS_LIBGPS_RESPONSE_VERSION_TYPE:
		    for(i = 0; i < pPacket->length; i++)
		    {
		        if(!memcmp(&pPacket->buff[i],"VERSION:HW",strlen("VERSION:HW")))
		        {
		            i = i + strlen("VERSION:HW") + 1;
		            memcpy(response,&pPacket->buff[i],pPacket->length-i-2);
                    D("gnss version get: %s,strlen is %zu",response,strlen(response));
                    config_xml_ctrl(SET_XML,"GE2-VERSION",response,CONFIG_TYPE); 
                    if(strstr(response, "GLO") != NULL)
                    {
                        memset(log_string,0,sizeof(log_string));
                        if(!config_xml_ctrl(GET_XML,"CP-MODE",log_string,CONFIG_TYPE))
                        {
                            D("in getresponse,get cp-mode in xml is %s",log_string);   
                            cpmode = (log_string[0]-'0')*4 + (log_string[1]-'0')*2 + log_string[2]-'0'; 
                        }
                        if(cpmode == 2 || cpmode == 3)
                        {
                            config_xml_ctrl(SET_XML,"CP-MODE","101",CONFIG_TYPE);  //gps+glonass
                        }
                        if(!config_xml_ctrl(GET_XML,"GPS-IMG-MODE",imgmode,CONFIG_TYPE) && (strstr(imgmode,"GNSSBDMODEM") != NULL))
                        {
                            config_xml_ctrl(SET_XML,"GPS-IMG-MODE","GNSSMODEM",CONFIG_TYPE);
                        }
                    }
                    else
                    {
                        memset(log_string,0,sizeof(log_string));
                        if(!config_xml_ctrl(GET_XML,"CP-MODE",log_string,CONFIG_TYPE))
                        {
                            D("in getresponse,get cp-mode in xml is %s",log_string);   
                            cpmode = (log_string[0]-'0')*4 + (log_string[1]-'0')*2 + log_string[2]-'0'; 
                        }
                        if(cpmode == 4 || cpmode == 5)
                        {
                            config_xml_ctrl(SET_XML,"CP-MODE","011",CONFIG_TYPE);  //gps+bds
                        }
                        if(!config_xml_ctrl(GET_XML,"GPS-IMG-MODE",imgmode,CONFIG_TYPE) && (strstr(imgmode,"GNSSMODEM") != NULL))
                        {
                            config_xml_ctrl(SET_XML,"GPS-IMG-MODE","GNSSBDMODEM",CONFIG_TYPE);
                        }
                        
                    }
					SignalEvent(&(s->vesionsync));
                    break;
		        }
		    }
		    break;
		case GNSS_LIBGPS_RESPONSE_CHIP_SUBTYPE:
			memcpy(response,pPacket->buff,pPacket->length);
			E("gnss chip id get,id is %s",response);
			break;
		case GNSS_LIBGPS_RESPONSE_GPSTIME_SUBTYPE:
			memcpy(&TT,pPacket->buff,pPacket->length);
			E("gps time get,gpstime weekno: %d,ms:%d",TT.weekno,TT.mscount);
			break;
		case GNSS_LIBGPS_RESPONSE_CONSTELLATION_SUBTYPE:
			memcpy(&cpmode,pPacket->buff,pPacket->length);
			E("gps constell get,value is 0x%x",cpmode);
			break;
		default:
			break;

	}
}

/*--------------------------------------------------------------------------
	Function:  getPartitionName

	Description:
		it  get gnss patiton name frorm the kernel driver device ,it must be  used after download 
	Parameters:
 		input :char* pdata   point the sting of partition name 
	Return: void  
--------------------------------------------------------------------------*/
#ifdef GNSS_INTEGRATED
static int  getPartitionName(char* pPartion)
{
    int fd ;
    int len ,ret;
    
    if(NULL ==pPartion) 
    {
        E("the input point is null %s",__FUNCTION__);
        return -1;
    }
    
    fd = open(SHARKLE_PIKL2_GNSS_SUBSYS_DEV, O_RDONLY);
    if (fd < 0) 
    {
        E("%s: open %s fail:%d,%s", __func__, SHARKLE_PIKL2_GNSS_SUBSYS_DEV,errno,strerror(errno));
        return -1;
    }
    len = read(fd,pPartion,SHARKLE_PIKL2_PARTITION_NMAE_LEN);

    if(len > 0)
    { 
        ret = 0;
    }
    else
    {
        E("%s: read %s fail error:%d,%s", __func__, SHARKLE_PIKL2_GNSS_SUBSYS_DEV,errno,strerror(errno));
        ret = -1;
    }
    close(fd);
    return ret;
}
#endif

int get_lines(int num,char *getbuf)
{
    int i = 0;
    char buf[256];
    char *temp;
    GpsState*  state = _gps_state; 
    
#ifndef GNSS_INTEGRATED
    if(table == NULL)
    {
        //first open the firmare bin 
		if(access(COMBINE_PATH,0) == -1)
		{
		    temp = strchr(state->binPath,'/');
		    D("bin path is %s",temp);
		    table = fopen(temp,"r");
		}
		else
		{
			D("bin path is %s",COMBINE_PATH);
			table = fopen(COMBINE_PATH,"r");
		}
        
        if(table == NULL)
        {
            D("open the firmare table bin fail,just return");
            return 1;
        }
        
    }
#else	
	if(table == NULL)
	{
        char partitionNane[128];
        int ret = 0;
        char* pGNSSName;
        int len = 0;

        memset(partitionNane,0,sizeof(partitionNane));
        ret = getPartitionName(partitionNane);
        if(0 == ret)
        {
            pGNSSName = strchr(partitionNane,'/');
            len = strlen(pGNSSName);
            if(0xA == pGNSSName[len -1])
            {
                pGNSSName[len -1] = 0; //delete the newline 
            }  
            D("the partion name:, %s",pGNSSName);
		    table = fopen(pGNSSName,"r");
        }
		//D("bin path is %s","/dev/block/platform/soc/soc:ap-ahb/20600000.sdio/by-name/wcnfdl");
		//table = fopen("/dev/block/platform/soc/soc:ap-ahb/20600000.sdio/by-name/wcnfdl","r");
		if(table == NULL)
		{
			D("open the firmare table bin fail:%d,%s,just return",errno,strerror(errno));
			return 1;
		}
	}
#endif
    //rewind(table);   
    //memset(buf,0,sizeof(buf));                           //search from begin or from TRACE_OFFSET
    if(combineInfo.combineValid == 1)
    {
		D("offset of combine is %d",combineInfo.offset);
		if(fseek(table,combineInfo.offset,SEEK_SET) != 0)
		{
		    E("cannot seek combine location:%s",strerror(errno));
		    return 1;
		}
    }
    else if(fseek(table,TRACE_OFFSET,SEEK_SET) != 0)
    {
        E("cannot seek location:%s",strerror(errno));
        return 1;
    }
    while(fgets(buf,sizeof(buf),table) != NULL)
    {
        //D("buf:%s",buf);
        i++;
        if(num == i)     //in vtable,num placed in order
        {  
            temp = strchr(buf,':');  //data placed like @xxx:
            if(temp != NULL && strlen(temp) < sizeof(buf))
            {
                memcpy(getbuf,temp+1,strlen(temp)-1);
                return 0;
            }
            else
            {
                E("cannot find num token");
                return 1;
            }
        }
        if(i > LINE_MAX_NUM)
        {
            E("find line num fail");
            return 1;
        }
    }
    return 1;
}

int transFormatString(char *pline,unsigned char *input,int lenth)
{
    int i = 0,j = 0,k = 0;
    int lineLen = strlen(pline);
    int start;
    int aint,readsize = 0;
    float bfloat;
    double cdouble;
    long long du64;
    char format[16];
    
    if(pline == NULL)
	{
		E("pline is null");
		return 0;
	}
    for(i = 0; (i < lineLen) && (readsize <= lenth); i++)
    {
		if(k > (int)(sizeof(logbuf)-1))
		{
			E("in transFormatString,strlen is error,k:%d",k);
			return 0;
		}
        if(pline[i] == '%')
        {
            start = i;
            for(j=i; j < lineLen-1; j++,i++)
            {
				if(k > (int)(sizeof(logbuf)-1))
				{
					E("in transFormatString,strlen is error,k:%d",k);
					return 0;
				}
                if((pline[j] == 'f') || (pline[j] == 'e'))   //float type,%f or %e
                {
                    if(pline[j+1] == 'f')               //%ff is double type
                    {
                        memset(format,0,sizeof(format));
                        memcpy(&cdouble,input+readsize,sizeof(double));
                        j++;
                        i++;
                        readsize = readsize + sizeof(double);
						if(j-start+1 >= (int)sizeof(format))
						{
							E("format is error,get length is %d",j-start+1);
							break;
						}
                        memcpy(&format,pline+start,j-start+1);
                        //D("ff type,format=%s",format);
                        //fprintf(gUart_fd,format,cdouble);
                        sprintf(&logbuf[k],format,cdouble);
                        k = strlen(logbuf);
                        break;
                    }
                    else
                    {
                        memcpy(&bfloat,input+readsize,sizeof(float));
                        readsize = readsize + sizeof(float);  
                        memset(format,0,sizeof(format)); 
			if(j-start+1 >= (int)sizeof(format))
			{
			    E("format is error,get length is %d",j-start+1);
			    break;
			}
                        memcpy(&format,pline+start,j-start+1);
                        //D("float type,format=%s,value=%f",format,bfloat);
                        //fprintf(gUart_fd,format,bfloat); 
                        sprintf(&logbuf[k],format,bfloat);
                        k = strlen(logbuf);
                        break;               
                    }
                }
                //include all int type;
                else if((pline[j] == 'i') || (pline[j] == 'u') || (pline[j] == 'd') || 
                           (pline[j] == 'x') || (pline[j] == 'X')|| (pline[j] == 'c')) 
                {
                    memcpy(&aint,input+readsize,sizeof(int));
                    readsize = readsize + sizeof(int);   
                    memset(format,0,sizeof(format)); 
					if(j-start+1 >= (int)sizeof(format))
					{
						E("format is error,get length is %d",j-start+1);
						break;
					}
                    memcpy(&format,pline+start,j-start+1);
                    //D("int type,format=%s",format);
                    //fprintf(gUart_fd,format,aint);    //transform type by fpirntf
                    sprintf(&logbuf[k],format,aint);
                    k = strlen(logbuf);
                    break;               
                }
                else if((pline[j] == 'l') && (pline[j+1] == 'l'))   //%lld or %llu or %llx or %llX,64 bit type
                {
                    j = j + 2;
                    i = i + 2;                                     
                    memcpy(&du64,input+readsize,sizeof(long long));
                    readsize = readsize + sizeof(long long); 
                    memset(format,0,sizeof(format)); 
		    if(j-start+1 >= (int)sizeof(format))
		    {
		        E("format is error,get length is %d",j-start+1);
			break;
		    }
                    memcpy(&format,pline+start,j-start+1);
                    //D("longlong type,format=%s",format);
                    //fprintf(gUart_fd,format,du64);
                    sprintf(&logbuf[k],format,du64);
                    k = strlen(logbuf); 
                    break;
                }
                else if((pline[j] == 'l') && ((pline[j+1] == 'f') || (pline[j+1] == 'e')))  //%lf or %le,just for double
                {
                    j++;
                    i++;
                    memcpy(&cdouble,input+readsize,sizeof(double));
                    readsize = readsize + sizeof(double); 
                    memset(format,0,sizeof(format)); 
		    if(j-start+1 >= (int)sizeof(format))
		    {
		        E("format is error,get length is %d",j-start+1);
			break;
		    }
                    memcpy(&format,pline+start,j-start+1);
                    //D("double type,format=%s",format);
                    //fprintf(gUart_fd,format,cdouble);
                    sprintf(&logbuf[k],format,cdouble);
                    k = strlen(logbuf); 
                    break;
                }
                else if(j-start > FORMAT_MAX_LENTH)
                {
                    E("can't get useful information in format");
                    //fwrite(&pline[start],j-start,1,gUart_fd); 
                    memcpy(&logbuf[k],&pline[start],j-start);
                    k = strlen(logbuf);
                    break;
                }
                //other case,% to type should remove.
            }
        }
        else if(pline[i] == '\\')    //don't parse it
        {
            if(pline[i+1] == 'n')
            {
			if(k > (int)(sizeof(logbuf)-2))
			{
				E("in transFormatString,strlen is error,k:%d",k);
				return 0;
			}
               //fprintf(gUart_fd,"\n");
               logbuf[k++] = '\r';
               logbuf[k++] = '\n';   //this is for '\n' or '\r' set to kernel and engpc
               break;
            }
            i++;
            continue;
        }
        else if(pline[i] == '\n' || pline[i] == '\r')
        {
            continue;
        }
        else
        {
            //fwrite(&pline[i],1,1,gUart_fd);
            logbuf[k++] = pline[i];
        }
    }
    return 0;
}

int cutString(char *pline)
{
	int i = 0, k = 0;
	int lineLen = strlen(pline);
    
	for(i = 0;i < lineLen; i++)
	{        
		if(pline[i] == '\\')
		{
			if(pline[i+1] == 'r') {
				//fprintf(gUart_fd, "\n");
				logbuf[k++] = '\r';
				logbuf[k++] = '\n';
				break;
			}
		}
		else
		{
			//fwrite(&pline[i], 1, 1, gUart_fd);
			logbuf[k++] = pline[i];
		}
	}
	return 0;
}



void preprocessGnssLog(unsigned char  *data,unsigned short len,unsigned char subType)
{
    char linebuf[300];
    char *ptemp = NULL;
    int num = 0,i;
    int recount = GNSS_LOG_MAX_RETRY;
    GpsState*  pState = _gps_state; 
    int bufferLen = sizeof(logbuf);

    if(len > bufferLen)
    {
        E("%s datalen:%d > 2048",__FUNCTION__,len);
    }
    while(recount && gWriteFlag)
    {      
        usleep(2000);//1ms 
        recount--;
    }
    if(gWriteFlag)
    {
        gDropCount++;
        //gWriteFlag = 1; 
        D("%s will drop count:%d gnss log :%s",__FUNCTION__,gDropCount,logbuf);
        usleep(1000);//release read thread    
		return;
    }
    
    if((GNSS_LIBGPS_ASSERT_SUBTYPE == subType) || (GNSS_LIBGPS_LOG_SUBTYPE == subType)||(GNSS_LIBGPS_NMEA_SUBTYPE == subType))
    {
		//writeGnsslogTokernel(data,len);

       // D("preprocessGnssLog 1 len =%d type :%d",len,subType);
	if(pState->etuMode)
	{
		D("preprocessGnssLog etuMode datalen:%d" ,len);
		if(1 == pState->cw_enable){
			pState->callbacks.nmea_cb(pState->NmeaReader[0].fix.timestamp,
				(const char*)data,len); //#bug 702699
		}
	}
	else
	{
		pthread_mutex_lock(&pState->gnsslogmutex);
		memset(logbuf,0,bufferLen);
		if(len > bufferLen)
			len = bufferLen;
		memcpy(logbuf,data,len);
		gLogType = GNSS_TYPE_LOG_TRACE;//it only save file

		pthread_mutex_unlock(&pState->gnsslogmutex);
		SignalEvent(&pState->gnssLogsync);
	}
        
    }
    else if(GNSS_LIBGPS_MINILOG_SUBTYPE == subType)
    {
	if(pState->etuMode)
        {
            return; 
        }
        //D("preprocessGnssLog 2 len =%d",len);
        memcpy(&num,data,sizeof(int));
        //D("num is %d",num);
        if(num > LINE_MAX_NUM)
        {
            E("line num:%d is too large,return",num);
            return;
        }

            memset(linebuf,'\0',sizeof(linebuf));
            pthread_mutex_lock(&pState->gnsslogmutex);            
            memset(logbuf,0,bufferLen);
            if(get_lines(num,linebuf) == 1)
            {
                E("cannot find line num,just discard it");
                pthread_mutex_unlock(&pState->gnsslogmutex);
                return;
            }
            //D("get line:%s",linebuf);

        if(strchr(linebuf,'%') != NULL)
        {
            //D("include var format");
            transFormatString(linebuf,data+sizeof(int),len-sizeof(int));
        }
        else
        {
			cutString(linebuf);
        }
        // it has put trace infor into logbug above code
        gLogType = GNSS_TYPE_LOG_TRACE;
        //gWriteFlag = 0;
        pthread_mutex_unlock(&pState->gnsslogmutex);
        SignalEvent(&pState->gnssLogsync); 
		//writeGnsslogTokernel((unsigned char*)logbuf, (unsigned short)strlen(logbuf));
    }
    else
    {
        TGNSS_Baseband_Log_t* pBasebandlog = (TGNSS_Baseband_Log_t*)data;
        int count = len/GNSS_LOG_BB_LEN;
      //D("preprocessGnssLog 3  len =%d",len);
        if(!pState->etuMode)
        {
            pthread_mutex_lock(&pState->gnsslogmutex);            
            memset(logbuf,0,bufferLen);
            sprintf(logbuf,"GNSS Baseband Log start len =%d,count=%d\r\n",len,count);
            while(count--)
            {
                sprintf(logbuf+strlen(logbuf),"type:%d,svid:%d,bbtime:%d,%d,%d,%d,%d,%d,%d,%d\r\n",
                pBasebandlog->type,pBasebandlog->svid,
                pBasebandlog->bbtime,pBasebandlog->data4,
                pBasebandlog->data5,pBasebandlog->data6,
                pBasebandlog->data7,pBasebandlog->data1,
                pBasebandlog->data2,pBasebandlog->data3);
                pBasebandlog++;
            }

            sprintf(logbuf+strlen(logbuf),"GNSS Baseband Log End\r\n");
            gLogType = GNSS_TYPE_LOG_TRACE;
            //gWriteFlag = 0;
            pthread_mutex_unlock(&pState->gnsslogmutex);
            SignalEvent(&pState->gnssLogsync); 
        }
	  //writeGnsslogTokernel((unsigned char*)logbuf, (unsigned short)strlen(logbuf));
    }
    usleep(1000);//release read thread    
}

void writeGnsslogTokernel(unsigned char *data, unsigned short len)
{
	GpsState*  s = _gps_state;
	int lenth = 0;

	if(s->gnsslogfd < 0)
	{
		//E("gnss log fd not open susccess");
		return ;
	}
	
	//fcntl(s->gnsslogfd, F_SETFL, O_RDWR | O_NOCTTY);   //zxw
	if(len > 0)
	{
		lenth = write(s->gnsslogfd,data,len); 
	}
	else
	{
		E("%s,len is 0",__func__);
	}
	//fcntl(s->gnsslogfd, F_SETFL, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if(lenth < 0 && errno != EINTR)
	{
		E("write gnsslogfd:%d fail (%s)\n",s->gnsslogfd,strerror(errno));
        close(s->gnsslogfd);
        s->gnsslogfd = open("/dev/gnss_dbg",O_RDWR);
        if(s->gnsslogfd < 0) 
        {
            E("open /dev/gnss_dbg error: %s",strerror(errno));
        }
        else
        {
            D("open /dev/gnss_dbg success \n");
        }
        
	}
}

/*--------------------------------------------------------------------------
	Function:  gps_dispatch

	Description:
		it  parse all dispatch from ge2
	Parameters:
	 packet in uart: 
	Return: void 
--------------------------------------------------------------------------*/
void  gps_dispatch(TCmdData_t* pPacket)
{
    GpsState*  state = _gps_state; 
	if(NULL == pPacket)
	{
        return;
	}
    //ap recevie the data ,so it should set heartcount 0 whatever response of  hardcount 
    state->HeartCount = 0;
    if( pPacket->length &&(GNSS_LIBGPS_DATA_TYPE == pPacket->type)
        &&((GNSS_LIBGPS_LOG_SUBTYPE == pPacket->subType)||
          (GNSS_LIBGPS_ASSERT_SUBTYPE == pPacket->subType)||
          (GNSS_LIBGPS_MINILOG_SUBTYPE == pPacket->subType)||
          (GNSS_LIBGPS_LOG_BB_SUBTYPE == pPacket->subType)))
	{
		if(0 == state->rftool_enable)
		{
			preprocessGnssLog(pPacket->buff,pPacket->length,pPacket->subType);
		}	
	}

	switch(pPacket->type)
	{
		case GNSS_LIBGPS_SET_PARA_TYPE:
		{
            E("===>>>>set para type");
			gps_getParaData(pPacket);
			break;
		}

		case GNSS_LIBGPS_RESPONSE_TYPE:
		{
			gps_getResponse(pPacket);
			break;
		}
		case GNSS_LIBGPS_NOTIFIED_TYPE:                     
		{
			gps_getNotified(pPacket);
			break;
		}
		case GNSS_LIBGPS_DATA_TYPE:
		{
			gps_getData(pPacket); 
			break;
		}
		case GNSS_LIBGPS_AIDING_DATA_TYPE:
		{
            if(state->etuMode)
            {
    			E("in ETU work ,so don't send agps request");
            }
            else
            {
    			E("agps aiding data received");
    			gps_getAdiData(pPacket);
            }
			break;
		}
		case GNSS_LIBGPS_POWER_RESPONSE_TYPE:
		{
            gps_getPowerResponse(pPacket);
			break;
		}
		case GNSS_LIBGPS_FLASH_TYPE:
		{
            if(state->etuMode)
            {
                D(" in ETU work ,so don't save flash information ");
            }
            else
            {            
    			D("gps save flash information ");
    			gps_saveFlashCfg(pPacket);
            }
			break;
		}
		case GNSS_LIBGPS_NMEA_TYPE:
		{
		    D("====>>>>>nmea data is received");
		    nmea_parse_cp(pPacket->buff,pPacket->length);
		    break;
		}
		#ifdef GNSS_REG_WR
		case GNSS_LIBGPS_REG_WR_TYPE:
		{
			D("obtain gnss register value from firmware\n");
			gps_getRegValue(pPacket);
			break;
		}
		#endif
		default:
		{
			E("the gps_dispatch  type error = %d\r\n",pPacket->type);
			break;
		}
	
	}
	
}

static int connect_download(void)
{
	int client_fd = -1;
	int retry_count = 20;
	struct timeval rcv_timeout; 

	client_fd = socket_local_client( GNSS_SOCKET_NAME,
	ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);

	while(client_fd < 0 && retry_count > 0)
	{
		retry_count--;
		E("%s: connect_download server %s, (error:%s)\n",__func__, GNSS_SOCKET_NAME,strerror(errno));
        
		usleep(100*1000);
		client_fd = socket_local_client( GNSS_SOCKET_NAME,
			ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
	}

	if(client_fd > 0)
	{
		rcv_timeout.tv_sec = 30;
		rcv_timeout.tv_usec = 0;
		if(setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, (char*)&rcv_timeout, sizeof(rcv_timeout)) < 0)
		{
			E("%s: set receive timeout fail\n",__func__);
		}
	}

	return client_fd;
}


static int close_download(int fd)
{
	close(fd);

	return 0;
}

static int DownloadFirmWare(int fd) 
{
	int n;
	char data[128];
    GpsState*  state = _gps_state; 

    D("DownloadFirmWare start TEMP_FAILURE_RETRY");
	//write reboot cmd
	int ret = TEMP_FAILURE_RETRY(write(fd, state->binPath, strlen(state->binPath)));
	if(ret < 0)
	{
		E("write %s bytes to client_fd:%d fail (error:%s)", state->binPath, fd, strerror(errno));
		return -1;
	}
	else
	{
		D("write %s bytes to client_fd:%d ", state->binPath, fd);
	}
    
	n = read(fd, data, sizeof(data)-1);
    
	if(0 == strncmp(data,WCN_RESP_START_GNSS, sizeof(WCN_RESP_START_GNSS)))
	{
		D("the download response ok ");
		ret = 0;
	}
	else
	{
		E("the download response error! ");
		ret = -1;
	}
	return ret;
}

static int GetGnssStatus(int fd) 
{
	int n;
	char data[128];
    GpsState*  state = _gps_state; 
    
    D("get gnss status from download modules ");
	//write reboot cmd
	int ret = TEMP_FAILURE_RETRY(write(fd, "startgnss gnss_status", sizeof("startgnss gnss_status")));
	if(ret < 0)
	{
		E("write %s bytes to client_fd:%d fail (error:%s)", "startgnss gnss_status", fd, strerror(errno));
		return -1;
	}
	else
	{
		D("write %s bytes to client_fd:%d ", "startgnss gnss_status", fd);
	}

    memset(data,0, 128);
	n = read(fd, data, sizeof(data)-1);
    
	if(0 == strncmp(data,WCN_RESP_START_GNSS, sizeof(WCN_RESP_START_GNSS)))
	{
		D("get gnss status download ok");
		ret = 0;
	}
	else
	{
		E("get gnss status download  error!");
		ret = -1;
	}
	return ret;
}


static int DownloadFD1(int fd) 
{
    int n;
    char data[128];

    
#ifdef ANDROIDL_PLATFORM
    char* pDownreq ="dumpgnss path=/dev/block/platform/sprd-sdhci.3/by-name/gnssfdl";  // android 4.4
#else
	char* pDownreq ="dumpgnss path=/vendor/etc/gnssfdl.bin";
#endif

    //write reboot cmd
    int ret = TEMP_FAILURE_RETRY(write(fd, pDownreq, strlen(pDownreq)));
    if(ret < 0)
    {
         E("write %s bytes to client_fd:%d fail (error:%s)", pDownreq, fd, strerror(errno));
         return -1;
    }
    else
    {
        E("write %s bytes to client_fd:%d ", pDownreq, fd);
    }
    
    n = read(fd, data, sizeof(data)-1);
     E("%s: get %d bytes %s\n", __func__, n, data);
    
    if(0 == strncmp(data,WCN_RESP_DUMP_GNSS, sizeof(WCN_RESP_DUMP_GNSS)))
    {
        D("the download fdl response ok ");
        ret = 0;
    }
    else
    {
        E("the download fdl response error! ");
        ret = -1;
    }

    return ret;

}

static int CloseGNSSDevice(int fd) 
{
    int n;
    char data[128];    

    //write close devices cmd
    int ret = TEMP_FAILURE_RETRY(write(fd, WCN_CMD_STOP_GNSS, strlen(WCN_CMD_STOP_GNSS)));
    if(ret < 0)
    {
        E("write %s bytes to client_fd:%d fail (error:%s)", WCN_CMD_STOP_GNSS, fd, strerror(errno));
        return -1;
    }
    // wait the download response 
    n = read(fd, data, sizeof(data)-1);
    E("%s: get %d bytes %s\n", __func__, n, data);
    
    if(0 == strncmp(data,WCN_RESP_STOP_GNSS, sizeof(WCN_RESP_STOP_GNSS)))
    {
       D("the close device  response ok ");
       ret = 0;
    }
    else
    {
       E("the download response error! ");
       ret = -1;
    }
    
    return ret; 
}

static int TryException(int fd)
{
    int n;
    char data[128];
    GpsState * pGpsState = _gps_state;
    struct termios termios;
    int    status;

    //write close devices cmd
    int ret = TEMP_FAILURE_RETRY(write(fd, WCN_CMD_DUMP_GNSS, strlen(WCN_CMD_DUMP_GNSS)));
    if(ret < 0)
    {
        E("write %s bytes to client_fd:%d fail (error:%s)\r\n", WCN_CMD_STOP_GNSS, fd, strerror(errno));
        return -1;
    }
    // wait the download response 
    n = read(fd, data, sizeof(data)-1);
    E("%s: get %d bytes %s\n", __func__, n, data);
    
    if(0 == strncmp(data,WCN_RESP_DUMP_GNSS, sizeof(WCN_RESP_DUMP_GNSS)))
    {
        E("TryExceptionDeal  response ok \r\n");
        pGpsState->fd = open(pGpsState->serialName, O_RDWR | O_NOCTTY);
        if(pGpsState->fd < 0)
        {
            return -1;
        }
        tcflush(pGpsState->fd, TCIOFLUSH);
        tcgetattr(pGpsState->fd, &termios);
        termios.c_oflag &= ~OPOST;
        termios.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
        termios.c_cflag &= ~(CSIZE | PARENB );//CRTSCTS
        termios.c_cflag |= (CS8 |CRTSCTS);//CRTSCTS
        cfmakeraw(&termios);
        cfsetispeed(&termios, B115200); 
        status = tcsetattr(fd, TCSANOW, &termios);
	    if(status != 0)
        {
    	    E("the set ttys3 error:%s\r\n",strerror(errno));
        }
	    tcflush(pGpsState->fd,TCIOFLUSH);
        gps_setPowerState(DEVICE_WORKED);
       ret = 0;
    }
    else
    {
       E("TryExceptionDeal response error! \r\n");
       ret = -1;
    }
    return ret;
}


int gps_devieManger(int flag)
{
    int download_fd,ret = 0;
    char data[128];
    int n;
	GpsState *s = _gps_state;

#ifdef GNSS_INTEGRATED
		return 0;  //for test
#endif
    download_fd = connect_download();

	if(download_fd < 0)
	{
		E("connect_download get fd error");
	    return -1;
	}

    switch(flag)
    {
        case GET_GNSS_STATUS:
        {
            ret = GetGnssStatus(download_fd);
            break;
        }
        case START_DOWNLOAD_GE2:
        {
            ret = DownloadFirmWare(download_fd);
            break;
        }
        case START_DOWNLOAD_FD1:
        {
            ret =DownloadFD1(download_fd);
            break;   
        }
        case CLOSE_DEVICE://cloed the devices 
        {
            ret = CloseGNSSDevice(download_fd);
            break;   
        }
        case DEVICE_EXCEPTION:
        {
            ret = TryException(download_fd);
        }
        default:
            E("gps_devieManger flag error =%d \r\n",flag);
            break;
    }
   	
    close_download(download_fd);
    return ret;
}

void read_slog_status(void)
{
	GpsState *s = _gps_state;
	char resp[10];
	int wl = 0, rl = 0;
	int retry_count = 1;
	int ret= -1;
	struct pollfd r_pollfd;
	struct timeval rcv_timeout;

	char zygote_value[128];
	int ret_zy;
	ret_zy = property_get("init.svc.zygote",zygote_value,NULL);
	D("vts test get zygote prop %s ,ret_zy %d slogmodem_fd %d\r\n",zygote_value, ret_zy, s->slogmodem_fd);
	if (ret_zy > 0) {
		if (strncmp(zygote_value, "stopped", sizeof("stopped")) == 0){
			D("%s,it is  vts test stop zygote so return slogmode function\n",__FUNCTION__);
			return;
		}else{
			D("%s,it is not vts test\n",__FUNCTION__);
		}
	}

	if(s->slogmodem_fd <= 0)
	{
        char kiaos[128];

         ret = property_get("ro.moz.ril.numclients",kiaos,NULL);//NULL
	    
        if(ret > 0)
        {
            
			E(" kiaos tcp socket");
            s->slogmodem_fd = socket_tcp_client();
        }
        else
        {
			E(" android os socket");
            // connect slogmodem
    		s->slogmodem_fd = socket_local_client( "slogmodem", ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
        }


		if(s->slogmodem_fd < 0)
		{
			E("connect slogmodem fail");
			return;
		}

		// set non block
		long flags = fcntl(s->slogmodem_fd,F_GETFL);
		flags |= O_NONBLOCK;
		ret = fcntl(s->slogmodem_fd,F_SETFL,flags);
		if(-1 == ret)
		{
			E("set slogmodem socket to O_NONBLOCK error");
			close(s->slogmodem_fd);
			s->slogmodem_fd = 0;
			return;
		}
	}

	// request log state
	wl = write(s->slogmodem_fd,"GET_LOG_STATE GNSS\n",19);
	if(wl < 0)
	{
		E("write GET_LOG_STATE to slogmodem socket error");
		close(s->slogmodem_fd);
		s->slogmodem_fd = 0;
		return;
	}

	// read status
	r_pollfd.fd = s->slogmodem_fd;
	r_pollfd.events = POLLIN;

	ret = poll(&r_pollfd,1,500);     // 500ms timeout

	if(( 1 == ret) && (r_pollfd.revents & POLLIN))
	{
		rl = read(s->slogmodem_fd, resp, 10);
		//D("slogmodem status:<%s>\n",resp);
		if(rl < 3)
		{
			E("read slogmodem socket error");
			close(s->slogmodem_fd);
			s->slogmodem_fd = 0;
			return;
		}
		else
		{
			if(0 == memcmp(resp,"OK ON\n", 6))     // OFF: (0 == memcmp(resp,"OK OFF\n", 7))
			{
				D("gnssslog status: slog open\n");
				s->slog_switch = 1;
			}
			else
			{
				D("gnssslog status: slog close\n");
				s->slog_switch = 0;
			}
		}
	}

	//close(slogmodem_fd);
}

// get socket client and set timeout 
int get_socket_fd(char* server,struct timeval* ptimeout)
{
	int client_fd = -1;
	int retry_count = 20;

	client_fd = socket_local_client(server,
	ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);

	while(client_fd < 0 && retry_count > 0)
	{
		retry_count--;
		E("%s: connect_download server %s, (error:%s)\n",__func__, server,strerror(errno));
		usleep(100*1000);
		client_fd = socket_local_client( server,
			ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
	}

	if(client_fd > 0)
	{
		if(setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, (char*)ptimeout, sizeof(struct timeval)) < 0)
		{
			E("%s: set receive timeout fail\n",__func__);
		}
	}

	return client_fd;
}

void gnss_notify_abnormal(char* server,struct timeval* ptimeout,char* cause)
{
    int fd = 0;
    int ret = 0;
    char causeError[64];
    
    fd = get_socket_fd(server,ptimeout);

     D("%s open client fd :%d",__func__,fd);

    if(fd < 0)
    {
        E("%s open client fd fail:%s",__func__,strerror(errno));
        return;
    }
    else
    {
        memset(causeError,0,sizeof(causeError));
        memcpy(causeError,"wcn gnsserror ",strlen("wcn gnsserror "));
        strcat(causeError,cause);
        ret = TEMP_FAILURE_RETRY(write(fd, causeError, strlen(causeError)));
        if(ret < 0)
        {
             E("write %s bytes to client_fd:%d fail (error:%s)",causeError, fd, strerror(errno));
             close(fd);
             return;
        }
        else
        {
            D("%s write %s to client_fd:%d ",__func__,causeError,fd);
        }
    }
    close(fd);
}

#ifdef GNSS_INTEGRATED

//it  request  memdup  
int gnss_request_memdumpl() 
{
    int fd;
    int ret = 0;
    int value = 1; //requestion 
    char buffer[8];
    GpsState*  state = _gps_state;
    struct timeval rcv_timeout;
	
    D("%s: enter ", __func__);

    fd = open(SHARKLE_PIKL2_GNSS_DUMP_DEV, O_WRONLY);

    if (fd < 0) 
    {
        E("%s: open %s fail:%d,%s", __func__, SHARKLE_PIKL2_GNSS_DUMP_DEV,errno,strerror(errno));
        return -1;
    }
    memset(buffer, 0, sizeof(buffer));
    sprintf(buffer, "%d", value);
    ret = write(fd, buffer, strlen(buffer));
    tcflush(fd, TCIOFLUSH);
    
    if (ret < 0) 
    {
        E("%s: write %s fail error:%d,%s", __func__, SHARKLE_PIKL2_GNSS_DUMP_DEV,errno,strerror(errno));
        close(fd);
        ret =  -1;
    }
    D("%s: memdump: ret:%d ", __func__,ret);
    save_dump_slogmodem("COPY_FILE GNSS DUMP /data/gnss/gnssdump.mem\n", strlen("COPY_FILE GNSS DUMP /data/gnss/gnssdump.mem\n"));

    rcv_timeout.tv_sec = 5;
    rcv_timeout.tv_usec = 0;
    if(ASSERT_TYPE_MANUAL == state->assertType)
    {
        gnss_notify_abnormal(WCND_SOCKET_NAME,&rcv_timeout,"manual-trigger");
    }
    else
    {
        gnss_notify_abnormal(WCND_SOCKET_NAME,&rcv_timeout,"firmware-trigger");
    }
    close(fd);
    state->happenAssert = 1;
    gps_hardware_close();
    gps_setPowerState(DEVICE_ABNORMAL);
    return ret;
}


//it  request power status 
//0 : poweroff
//1: power on 
int gnss_get_power_status() 
{
    int fd;
    int ret = 0;
    char value;
    

    fd = open(SHARKLE_PIKL2_GNSS_STATUS_DEV, O_RDONLY);

    if (fd < 0) 
    {
        E("%s: open %s fail:%d,%s", __func__, SHARKLE_PIKL2_GNSS_STATUS_DEV,errno,strerror(errno));
        return -1;
    }
    ret = read(fd, &value, sizeof(value));
    
    if (ret < 0) 
    {
        E("%s: read %s fail error:%d,%s", __func__, SHARKLE_PIKL2_GNSS_STATUS_DEV,errno,strerror(errno));
        close(fd);
        ret =  -1;
    }

	switch(value)
	{
		case '0':  //poweroff
			D("%s: gnss poweroff ", __func__);
			ret = INTERGRATED_GNSS_STATUS_POWEROFF;
			break;
		case '1':  //poweron
			D("%s: gnss poweron", __func__);
			ret = INTERGRATED_GNSS_STATUS_POWERON;
			break;
		case '2':  //assert
			D("%s: gnss poweron", __func__);
			ret = INTERGRATED_GNSS_STATUS_ASSERT;
			break;
		case '3':  //POWEROFF_GOING
			D("%s: gnss poweroff going", __func__);
			ret = INTERGRATED_GNSS_STATUS_POWEROFF_GOING;
			break;
		case '4':  //POWERON_GOING
			D("%s: gnss poweron going", __func__);
			ret = INTERGRATED_GNSS_STATUS_POWERON_GOING;
			break;
		default:
			D("%s: wrong gnss power state", __func__);
			ret =  -1;
			break;
	}
	close(fd);
	return ret;
}


//it  request power on/off /others status 
int gnss_set_power_control(int ctl) 
{
    int fd;
    int ret = 0;
    char buffer[8];
    
    D("%s: enter ", __func__);

    fd = open(SHARKLE_PIKL2_GNSS_POWER_DEV, O_WRONLY);

    if (fd < 0) 
    {
        E("%s: open %s fail:%d,%s", __func__, SHARKLE_PIKL2_GNSS_POWER_DEV,errno,strerror(errno));
        return -1;
    }
    memset(buffer, 0, sizeof(buffer));
    sprintf(buffer, "%d", ctl);
    ret = write(fd, buffer, strlen(buffer));
    tcflush(fd, TCIOFLUSH);
    
    if (ret < 0) 
    {
        E("%s: write %s fail error:%d,%s", __func__, SHARKLE_PIKL2_GNSS_POWER_DEV,errno,strerror(errno));
        close(fd);
        ret =  -1;
    }
    if(ctl)
    {
         D("%s: enable power on: ret:%d ", __func__,ret);
    }
    else
    {
        D("%s: enable power off: ret:%d ", __func__,ret);
    }
    close(fd);
    return ret;
}


int sharkle_gnss_switch_modules()
{
    int fd;
    int ret = 0;
    char buffer[16];
    int workmodules;
    GpsState*  state = _gps_state; 

    memset(buffer,0,16);
     
    ret = config_xml_ctrl(GET_XML,"GPS-IMG-MODE",buffer,CONFIG_TYPE);
    if(0 == ret)
    {
        D("config xml get img-mode is %s",buffer);
        if(!memcmp(buffer,"GNSSBDMODEM",strlen("GNSSBDMODEM")))
        {
            state->imgmode = BDS_IMG;
            workmodules = GPS_BDS_IMG;
        }
        else
        {
            state->imgmode = GLONASS_IMG;
            workmodules = GPS_GLONASS_IMG;
        }
    }
    else
    {
        state->imgmode = GLONASS_IMG;
        workmodules = GPS_GLONASS_IMG;
    }

    fd = open(SHARKLE_PIKL2_GNSS_SUBSYS_DEV,O_WRONLY);
     if (fd < 0) 
    {
        E("%s: open %s fail:%d,%s", __func__, SHARKLE_PIKL2_GNSS_SUBSYS_DEV,errno,strerror(errno));
        return -1;
    }
    memset(buffer, 0, sizeof(buffer));
    sprintf(buffer, "%d", workmodules);
    ret = write(fd, buffer, strlen(buffer));
    tcflush(fd, TCIOFLUSH);
    if (ret < 0) 
    {
        E("%s: write %s fail error:%d,%s", __func__, SHARKLE_PIKL2_GNSS_SUBSYS_DEV,errno,strerror(errno));
        close(fd);
        ret =  -1;
    }
    if(GPS_GLONASS_IMG == workmodules)
    {
        D("%s: switch  GPS_GLONASS_IMG ret:%d ", __func__,ret);
    }
    else
    {
       D("%s:  switch  GPS_BD_IMG: ret:%d ", __func__,ret);
    }
    close(fd);
    return ret;

}
//it  request power on/off /others status
int sharkle_gnss_ready(int ctl )
{
    int ret = 0;
    int i = 0;
    int versionReqMax = 10;//wait the GE2 ready
    char status;
    GpsState*  state = _gps_state;

    //first get the status
    status =  gnss_get_power_status();

    //set the status
    if(ctl)
	{
		for (i=0; i<6; i++)
		{
			if(INTERGRATED_GNSS_STATUS_POWEROFF_GOING == status)
			{
				usleep(200*1000);
				status = gnss_get_power_status();
				continue;
			}
			break;
		}
        if(INTERGRATED_GNSS_STATUS_POWEROFF == status)
        {
            gps_setPowerState(DEVICE_DOWNLOADING);
            
            ret = gnss_set_power_control(ctl);
            if(ret < 1)
            {
                E("%s poweron failed",__FUNCTION__);
                return -1;
            }
            else
            {
                ret = 0;
            }
            gps_setPowerState(DEVICE_DOWNLOADED);
            gps_setPowerState(DEVICE_IDLED);
            gps_hardware_open();
            if(0 == ret)
            {
                status = gnss_get_power_status();
                //wait verssion
                while(versionReqMax)
                {
                   if(WaitEvent(&(state->vesionsync), 200) == 0)   //wait 30ms
                   { 
                       D("received version now");
                       break;
                   }
                   versionReqMax--;
               }
               D("received version %d times",(10 - versionReqMax));
                gps_sendData(SET_BASEBAND_MODE,NULL);

                if(state->tsx_enable == 1)
                {
                    update_tsx_param();
                }
                update_assist_data(state);
            }
        //get the status when it download ready ,and real download
        }
        else
        {
            ret = 0;
            D("it has already download and poweron now");
        }
       
    }
    else
    {
        if(INTERGRATED_GNSS_STATUS_POWERON == status)//poweron 
        {
            if(DEVICE_SLEEP != state->powerState)
            {
                D("%s it can't poweroff when the powerstate at :%s",__FUNCTION__,gps_PowerStateName(state->powerState));
                return -1; 
            }
            ret = gnss_set_power_control(ctl);
            if(ret < 1)
            {
                E("%s poweroff failed",__FUNCTION__);
                return -1;
            }
            else
            {
                ret = 0;
                gps_hardware_close();
            }
        }
        else
        {
            D("it has already poweroff now");
        }
    }
    return ret;
}

#endif
