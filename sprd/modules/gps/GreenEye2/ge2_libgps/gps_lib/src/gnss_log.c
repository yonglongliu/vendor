
#include <errno.h>  
#include <fcntl.h>  
#include <stdio.h>  
#include "gps_common.h"
#include "gnss_log.h"


static FILE *slog_fd;



/*--------------------------------------------------------------------------
    Function:  savenmealog2Buffer

    Description:
        it only save nmea  log  to temp buffer when it need report callbacks.nmea_cb  
    Parameters:
     pdata: point the  nma log 
     len : the one nmea log datalen  

     notes:  the  input data length <= 2048  
     
    Return: void
--------------------------------------------------------------------------*/
void savenmealog2Buffer(const char*pdata, int len)
{
	GpsState*   s = _gps_state;
    int datalen = sizeof(logbuf);
    int count = GNSS_LOG_MAX_RETRY;
    
    //D("%s input datalen : %d ",__FUNCTION__,len);
    if((pdata == NULL)|| (len > datalen))
    {
        E("%s input datalen : %d > 512",__FUNCTION__,len);
        return;
    }
    while(count)
    {
        if(gWriteFlag == 0)
        {
            break;
        }
        else
        {
            //SignalEvent(&s->gnssLogsync); 
            usleep(1500);//1ms 
            count--;
        }
    }
    if(gWriteFlag)
    {
        gDropCount++;
        //gWriteFlag = 1; 
        D("%s will drop count:%d gnss log :%s",__FUNCTION__,gDropCount,logbuf);
		return;
    }
    pthread_mutex_lock(&s->gnsslogmutex);
    memset(logbuf,0,sizeof(logbuf));
    memcpy(logbuf,pdata,len);
    gLogType = GNSS_TYPE_LOG_NMEA;//it only save file 
    //gWriteFlag = 0;
    pthread_mutex_unlock(&s->gnsslogmutex);
    SignalEvent(&s->gnssLogsync); 
    usleep(1000);//release read thread 
}


/*--------------------------------------------------------------------------
    Function:  copygnsslog2Sendbuffer

    Description:
         it copy gnss log from tmep buffer to send buffer  
    Parameters: void
     
    Return: void
--------------------------------------------------------------------------*/
void copygnsslog2Sendbuffer()
{
	GpsState*   s = _gps_state;
    int dataLen = 0;
    
    memset(s->gnsslogBuffer,0,GNSS_LOG_BUUFER_MAX);
    pthread_mutex_lock(&s->gnsslogmutex);
    dataLen = strlen(logbuf);
    if(dataLen < GNSS_LOG_BUUFER_MAX)
    {
        memcpy(s->gnsslogBuffer,logbuf,strlen(logbuf));
    }
    else
    {
        D("copygnsslog2Sendbuffer dataLen > 2048");
    }
    //gWriteFlag = 1;
    //memset(logbuf,0,sizeof(logbuf));
    pthread_mutex_unlock(&s->gnsslogmutex);

}

 /*--------------------------------------------------------------------------
     Function:  gnss_rf_capture_data
 
     Description:
          it read rf data from /data/gnss/yyyy-mm-dd-mmin--so on.txt
     Parameters: void
      
     Return: void
 --------------------------------------------------------------------------*/
 void gnss_rf_capture_data(void )
{
	GpsState*   s = _gps_state;
	FILE *fp = NULL;
	char data[MAXLINE];
	char result[100];
	char head[16]="$GPGGA,";
	int ret = 0;

	if(data_exist_flag == 1)
	{
		D("read: %s\n", fname);
		if ((fp = fopen (fname, "r")) == NULL)
		{
			perror ("File open error!\n");
		}
		else
		{
			while ((fgets(data, MAXLINE, fp)) != NULL)
			{
				//D("capture_thread line\n");
				memset(result,0,100);
				strcpy(result,head);
				strcat(result,data);
				s->callbacks.nmea_cb(s->NmeaReader[0].fix.timestamp, (const char*)data, strlen(data));
			}
			//E("capture_thread error : <%s>",strerror(errno));
			data_exist_flag = 0;
			fclose(fp);
			D("capture_thread end\n");
		}
	}
	usleep(2000);

}

/*--------------------------------------------------------------------------
    Function:  saveLocallog

    Description:
        it gnss  log  to local dir : /data/gnss/gnss.log 
    Parameters:
     data: point the  gnss  log 
     len : the one  gnss  log  datalen  

     notes:  the  input data length <= 2048  
     
    Return: void
--------------------------------------------------------------------------*/
void saveLocallog(const char  *data,short len)
{
    GpsState*  state = _gps_state;  
    char *log_path = "/data/gnss/gnss.log";

    if((slog_fd == NULL))
    {
        D("create nmea log file now");
        slog_fd = fopen(log_path,"a+");
    }
    if(slog_fd != NULL)
    {
        fwrite(data,len,1,slog_fd);
        fflush(slog_fd);
    }
}

/*--------------------------------------------------------------------------
    Function:  close_logsave

    Description:
        it closed the local gnss log file handle  
    Parameters:
     
    Return: void
--------------------------------------------------------------------------*/
void close_logsave(void)
{
    if(slog_fd != NULL)
    {
        fclose(slog_fd);
    }
}


/*--------------------------------------------------------------------------
    Function:  gnss_log_thread

    Description:
        it only save nmea  log  to temp buffer when it need report callbacks.nmea_cb  
    Parameters:
     pdata: point the  nma log 
     len : the one nmea log datalen  

     notes:  the  input data length <= 2048  
     
    Return: void
--------------------------------------------------------------------------*/
 void gnss_log_thread( void*  arg )
{
	GpsState*   s = (GpsState*) arg;
    int ret = 0;
    
    D("gnss_log_thread: enter");

	while(1)
	{
		gWriteFlag = 0;
		ret = WaitEvent(&(s->gnssLogsync), EVENT_WAIT_FOREVER);//wait signal
		gWriteFlag = 1;
        //D("s->etuMode:%d,s->postlog:%d,slog_switch:%d ,s->logswitch:%d",s->etuMode,s->postlog,s->slog_switch,s->logswitch);

         if(s->etuMode)//gnss tool test 
        {
             if(s->rftool_enable &&(GNSS_TYPE_LOG_RF == gLogType))
             {
                 gnss_rf_capture_data();
             }
             else
             {
                 copygnsslog2Sendbuffer();
                 //D("gnss_log_thread: etuMode datalen:%zu", strlen((const char *)s->gnsslogBuffer));
                 s->callbacks.nmea_cb(s->NmeaReader[0].fix.timestamp,(const char*)s->gnsslogBuffer,strlen((const char *)s->gnsslogBuffer));
             }
        }
        
		//it start ready gnss log or nmea and report the client 
		else if(s->postlog) //gnss tool or autotest  repoart  all gnss log to pc  
        {
            //if rf tool test ,it get data from file 
            copygnsslog2Sendbuffer();
            //D("gnss_log_thread: postlog datalen:%zu", strlen((const char *)s->gnsslogBuffer));
            s->callbacks.nmea_cb(s->NmeaReader[0].fix.timestamp,(const char*)s->gnsslogBuffer,strlen((const char *)s->gnsslogBuffer));
        }
        else if (s->slog_switch) //the modem slog open gnss log ,not pc tool 
        {
            //first it should transfer gnss log to modem log daemon 
            if(GNSS_TYPE_LOG_TRACE == gLogType)
            {
                copygnsslog2Sendbuffer();
                //D("gnss_log_thread: slog_switch trace  datalen:%zu", strlen((const char *)s->gnsslogBuffer));
                writeGnsslogTokernel((unsigned char*)s->gnsslogBuffer, (unsigned short)strlen((const char *)s->gnsslogBuffer));
            }
            else if(GNSS_TYPE_LOG_NMEA == gLogType)//it need report nmea log to client when nmea log
            {
                copygnsslog2Sendbuffer();
               // D("gnss_log_thread: slog_switch nmea  datalen:%zu", strlen((const char *)s->gnsslogBuffer));
                s->callbacks.nmea_cb(s->NmeaReader[0].fix.timestamp,(const char*)s->gnsslogBuffer,strlen((const char *)s->gnsslogBuffer));
            }

        }
        else // the pc tool and modem slog can't configuration , 
        {
            if((0 == s->release) && (s->logswitch)&&(GNSS_TYPE_LOG_TRACE == gLogType))// only userdebug pac and log switch on 
            {
                copygnsslog2Sendbuffer();
                //D("gnss_log_thread: logswitch trace  datalen:%zu", strlen((const char *)s->gnsslogBuffer));
                saveLocallog((const char*)s->gnsslogBuffer,( short)strlen((const char *)s->gnsslogBuffer));
            }
            else
            {
                copygnsslog2Sendbuffer();
                //it need report nmea log to client when nmea log 
                if(GNSS_TYPE_LOG_NMEA == gLogType)
                {
                    s->callbacks.nmea_cb(s->NmeaReader[0].fix.timestamp,(const char*)s->gnsslogBuffer,strlen((const char *)s->gnsslogBuffer));
                }
            }
            
        }
	}
}

