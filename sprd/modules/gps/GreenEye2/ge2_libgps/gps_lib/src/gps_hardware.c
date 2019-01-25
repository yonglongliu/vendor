#include <termios.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <cutils/properties.h>

#include "gps_common.h"
#include "gps_engine.h"
#include "gps_cmd.h"
#include "gnss_libgps_api.h"

static const char *GPS_EPHE = "$PCAGPS,gpsephe,";
static const char *GPS_TIME = "$PCAGPS,gpstime,";
static const char *GPS_LOC  = "$PCAGPS,gpslocn,";
static const char *A_END  = "$PCSUPL,aend\r\n";
static const char *S_END  = "$PCSUPL,send\r\n";
static const char *E_ASSIST  = "$PCSUPL,assist,";
static const char *E_SUPL_READY  = "$PCSUPL,suplready,";
static const char *MSA_POS  = "$PCSUPL,niposition,";


static char *close_cmd ="$PCGDC,SHUTDOWN,1,*1\r\n"; 

extern TGNSS_DataProcessor_t gGPSStream;

unsigned long send_length = 0;
unsigned long read_length = 0;

static void  set_fdattr(int fd )
{
    struct termios termios;
    int    status;
    char stop_bits = 0;
    int baud = B3000000;

    GpsState *s = _gps_state;

   tcflush(fd, TCIOFLUSH);
   tcgetattr(fd, &termios);

   if(GREENEYE2 == s->hardware_id) // it's real uart 
   {
       termios.c_oflag &= ~OPOST;
       termios.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
       termios.c_cflag &= ~(CSIZE | PARENB |CRTSCTS);//CRTSCTS
       termios.c_cflag |= (CS8 |CRTSCTS );//CRTSCTS
       cfmakeraw(&termios);
       cfsetispeed(&termios, baud); //B115200 
    }
    else
    {
        cfmakeraw(&termios);
        termios.c_cflag |= (CRTSCTS | stop_bits);
        /* set input/output baudrate */
        cfsetospeed(&termios, baud);
        cfsetispeed(&termios, baud);
    }

	status = tcsetattr(fd, TCSANOW, &termios);
	if  (status != 0)
    {
    	E("the set UART error:%s\r\n",strerror(errno));
    }
	tcflush(fd, TCIOFLUSH);
}

static int openSerialPort(const char* pSerialName)
{
	GpsState *s = _gps_state;
	int ret = 0;
	char time = 3;

	do 
	{		   
		if ((s->fd = open(pSerialName, O_RDWR | O_NOCTTY)) >= 0)			 
		break;
		usleep(100000);
	} while (time--);

	
	if (s->fd < 0)
	{
	    E("%s: no gps Hardware detected, errno = %d, %s", __FUNCTION__, errno, strerror(errno));
		ret = -1;
	}
	//it add debug information 
	D("openSerialPort name %s ,fd=%d,ret =%d\r\n", pSerialName,s->fd, ret);

       return ret;
}


void close_engine(GpsState* s)
{
    int ret = 0;
    struct termios termios;

   	tcflush(s->fd, TCIOFLUSH);

    if(s->hardware_id == GREENEYE2)
    {
        tcgetattr(s->fd, &termios);
        termios.c_cflag &= ~(CRTSCTS);// turn off CRTSCTS
        cfmakeraw(&termios);
        tcsetattr(s->fd, TCSANOW, &termios);
    }
	ret = close(s->fd);
	D("close_engine:%d, ret:%d\n",s->fd,ret);
	s->fd = -1;
	s->gps_open = false;
}

/*--------------------------------------------------------------------------
	Function:  gps_hardware_open

	Description:
		it  open  the device and power On device 
	Parameters:
	 void: 
	Return: SUCCESS:0 ; FAILED : others 
--------------------------------------------------------------------------*/
int gps_hardware_open(void)
{
    int rc = 0;
	GpsState *s = _gps_state;

    D("gps_hardware_open enter");
	
	if(s->fd < 0)// it has not open 
	{
		rc = openSerialPort(s->serialName);
		if(0 != rc)// the serial open error 
		{
			return rc;
		}
	}
	//init the serial port 
	set_fdattr(s->fd);
	if(s->gps_open == false)
	{
	    s->gps_open = true;
	}
    SignalEvent(&(s->threadsync));
    D("SignalEvent \r\n");
    return rc;
}

/*--------------------------------------------------------------------------
	Function:  gps_hardware_close

	Description:
		it  close the device and power off device 
	Parameters:
	 void: 
	Return: SUCCESS:0 ; FAILED : others 
--------------------------------------------------------------------------*/
int gps_hardware_close(void)
{
    int rc = 0;
    GpsState *s = _gps_state;

	if(s->gps_open == false)
	{
		return rc;
	}
	close_engine(s);

    return rc;
}

/*--------------------------------------------------------------------------
	Function:  gps_screenStatusRead

	Description:
		it  read  the serial port by asynchronous mothod 
	Parameters:
	      pGPSState: point the gps device 
	Return: void
--------------------------------------------------------------------------*/
void gps_screenStatusRead(GpsState *s)
{
	char value[16];
	int readLen = 0;
	int i = 0;
	int ret = 0;

	if(s->powerctrlfd < 0)
 	{
 		E("gps_screenStatusRead fd not open\n");
 		return;
 	}
	memset(value,0,sizeof(value));
	readLen = read(s->powerctrlfd,value,sizeof(value));
	if(readLen > 0) 
	{
		if(!memcmp(value,"gnss_resume",11))
		{
			D("####SCREEN ON \n");
			s->screen_off = 0;
		}
		else if(!memcmp(value,"gnss_sleep ",11))
		{
			D("####SCREEN OFF \n");
			s->screen_off = 1;
		}
		else
		{
			E("gps_screenStatusRead unkown data: read length=%d\n", readLen);
		}

	}
	else
	{	
 		E("gps_screenStatusRead fail (%s)\n", strerror(errno));
	}   
}

/*--------------------------------------------------------------------------
	Function:  gps_serialPortRead

	Description:
		it  read  the serial port by asynchronous mothod 
	Parameters:
	 pGPSState: point the gps device 
	Return: void
--------------------------------------------------------------------------*/
void gps_serialPortRead(GpsState *pGPSState)
{
    int readLen = 0;
	int i = 0;
	int ret = 0;
	GpsState *s = _gps_state;
       struct timeval tvold;
       struct timeval tvnew;

	 if(pGPSState->fd < 0)
 	{
 		E("gps_serialPortRead rd not open \r\n");
 		usleep(20000);
 		return;
 	}
	 //D("gps_serialPortRead read before ");
	readLen = read( pGPSState->fd, pGPSState->readBuffer, SERIAL_READ_BUUFER_MAX-1);
     pGPSState->readCount++;
	if (readLen > 0) 
	{
       	read_length = read_length + readLen;   //send length
	    if(read_length > GPS_RECV_MAX_LEN) 
		{
			read_length = 0;
		}
        gettimeofday(&tvold, NULL);
		ret =  GNSS_ParseOnePacket(&gGPSStream, pGPSState->readBuffer,readLen);
        gettimeofday(&tvnew, NULL);
        i = tvnew.tv_sec - tvold.tv_sec;
        if( i > 2)
        {
            E("%s   GNSS_ParseOnePacket  consume too times :%d \r\n",__FUNCTION__,i);
        }

	}
	else
	{	
		usleep(20000);
	}
    
    //D("gps_serialPortRead read len =%d",readLen);
}


 /*--------------------------------------------------------------------------
	 Function:	gps_serialPortWrite
 
	 Description:
		 it sync write the serial port r 
	 Parameters:
	  pGPSState: point the gps device 
	 Return: void
 --------------------------------------------------------------------------*/
void  gps_serialPortWrite(GpsState *pGPSState)
{
    int writeLen = 0;
	int dataLen = 0;
	int totalLen = 0;
	unsigned int readPos = pGPSState->serialWrite.ReadPosition;
	char* pData = &pGPSState->serialWrite.pBuffer[readPos];

	if(pGPSState->fd < 0)
 	{
 		E("gps_serialPortWrite rd not open \r\n");
 		return;
 	}

	dataLen = pGPSState->serialWrite.DataLen; 
	fcntl(pGPSState->fd, F_SETFL, O_RDWR | O_NOCTTY);
	// it need to optimize by data size 
	if(pGPSState->serialWrite.WritePosition  > readPos)
	{
		totalLen = write(pGPSState->fd,pData,dataLen); 
	}
	else // it need twice copy data 
	{	
		int len = pGPSState->serialWrite.BufferSize - readPos;
		
		writeLen = write(pGPSState->fd,pData,len);  // copy data the readpos ------bufferend 
		if(writeLen == len)
		{   // copy data the bufferhead -----witepose -1 
             len = write(pGPSState->fd,pGPSState->serialWrite.pBuffer,(dataLen -len)); 
             totalLen = dataLen;			
		}
		else
		{
             totalLen = writeLen;
		}
	
	}

	if(totalLen)
	{
		pGPSState->serialWrite.ReadPosition = (readPos + totalLen +1)%(pGPSState->serialWrite.BufferSize);
		pGPSState->serialWrite.DataLen = dataLen - totalLen;
	}
	fcntl(pGPSState->fd, F_SETFL, O_RDWR | O_NOCTTY | O_NONBLOCK);
	
}




/*--------------------------------------------------------------------------
    Function:  gps_writeSerialData

    Description:
        it   send data to device by sync method 
    Parameters:
     s : point the gps device 
     len : the send data len 
    Return: it real send data len  
--------------------------------------------------------------------------*/
int gps_writeSerialData(GpsState* s,int len)
{
    int lenth = 0,i;

    if(s->fd < 0)
    {
        E("fd not open success,return");
        return -1;
    }
    if(DEVICE_SLEEP == s->powerState)
    {
        E(" the power is not wakeup");
        return lenth;
    }
	send_length = send_length + len;   //send length
	if(send_length > GPS_SEND_MAX_LEN)
		send_length = 0;
    fcntl(s->fd, F_SETFL, O_RDWR | O_NOCTTY);
    lenth = write(s->fd,s->writeBuffer,len); 
    fcntl(s->fd, F_SETFL, O_RDWR | O_NOCTTY | O_NONBLOCK);
    return lenth;
}

/*--------------------------------------------------------------------------
    Function:  gps_sendCommnd

    Description:
        it  encode and send  start-module   request  
    Parameters:
     flag: start module  request flag 
    Return: the write data length  
--------------------------------------------------------------------------*/
static int gps_sendCommnd(int flag)
{
	GpsState*	s = _gps_state;
	TCmdData_t packet;
	int len,writeLen ;
	char measure_string[32];

	len = writeLen = 0;
	E("send command enter,subtype is %d",flag);
	packet.type  = GNSS_LIBGPS_SET_PARA_TYPE;

	switch(flag)
	{
		case COMMAND:
		{
			packet.subType = GNSS_LIBGPS_SET_WORKMODE_SUBTYPE;
			packet.length = 4;
			memcpy(packet.buff,&(s->cmd_flag),sizeof(int));
			break;
		}
		case SET_CPMODE:
		{
			packet.subType = GNSS_LIBGPS_SET_GNSS_SUBTYPE;
			packet.length = 4;
			memcpy(packet.buff,&(s->cpmode),sizeof(int));
			break;
		}
		case SET_OUTTYPE:
		{
			memset(measure_string,0,sizeof(measure_string));
		    config_xml_ctrl(GET_XML,"MEASURE-REPORT",measure_string,CONFIG_TYPE);
		    if(!memcmp(measure_string,"TRUE",strlen("TRUE")))
		    {
		        D("open measurement report switch");
				s->outtype = (s->outtype|0x02);
		    }
			D("outypte is 0x%x",s->outtype);
			packet.subType = GNSS_LIBGPS_SET_OUTPUT_PROTOCOL_SUBTYPE;
			packet.length = 4;
			memcpy(packet.buff,&(s->outtype),sizeof(int));
			break;
		}
		case CLEAR_CONFIG:
		{
			packet.subType = GNSS_LIBGPS_CLEAR_FLASH_CONFIG_SUBTYPE;
			packet.length = 0;
			break;
		}
		case SET_INTERVAL:
		{	// CTS_TEST
			packet.subType = GNSS_LIBGPS_SET_REPORT_INTERVAL;
			packet.length = 4;
			memcpy(packet.buff,&(s->min_interval),sizeof(int));
			break;
		}
		case SET_LTE_ENABLE:
			packet.subType = GNSS_LIBGPS_SET_LTE_SWITCH;
			packet.length = 4;
			memcpy(packet.buff,&(s->lte_open),sizeof(int));
		    break;
		case SET_NET_MODE:
		    D("SET_NET_MODE: inform firmware of network status:%d",s->connected);
		    packet.subType = GNSS_LIBGPS_SET_NETWORK_STATUS;
			packet.length = 4;
		    if((agpsdata->agps_enable == 0)&&(s->nokiaee_enable == 0))
		    {
		        D("SET_NET_MODE: agps and nokia ee are all closed.");
			    memcpy(packet.buff,&len,sizeof(int));
			}
		    else
		    {
			    memcpy(packet.buff,&(s->connected),sizeof(int));
			}
		    break;		
		case SET_CMCC_MODE:
			packet.subType = GNSS_LIBGPS_SET_CMCC_SWITCH;
			packet.length = 4;
			memcpy(packet.buff,&(s->cmcc),sizeof(int));
		    break;
		case SET_REALEPH:
			packet.subType = GNSS_LIBGPS_SET_REALEPH_STATUS;
			packet.length = 4;
			memcpy(packet.buff,&(s->realeph),sizeof(int));
		    break;	
		case SET_WIFI_STATUS:
			packet.subType = GNSS_LIBGPS_SET_WIFI_STATUS;
			packet.length = 4;
			memcpy(packet.buff,&(agpsdata->wifiStatus),sizeof(int));
		    break;		
		case SET_ASSERT_STATUS:
			packet.subType = GNSS_LIBGPS_SET_ASSERT_STATUS;
			packet.length = 0;
		    break;
		case SET_DELETE_AIDING_DATA_BIT:
			packet.subType = GNSS_LIBGPS_SET_DELETE_AIDING_DATA_BIT;
			packet.length = 4;
			memcpy(packet.buff,&(s->deleteAidingDataBit),sizeof(unsigned int));
            break;
		case SET_BASEBAND_MODE:
			packet.subType = GNSS_LIBGPS_SETMODE_SUBTYPE;
			packet.length = 4;
			memcpy(packet.buff,&(s->baseband_mode),sizeof(int));
			break;
		default:
			break;
	}
    
    pthread_mutex_lock(&s->writeMutex);
	len = GNSS_EncodeOnePacket(&packet, s->writeBuffer, SERIAL_WRITE_BUUFER_MAX);

	if(len > 0)
	{
		writeLen = gps_writeSerialData(s,len);   //gps_writeData2Buffer
	}
	else
	{
		E("gps_sendCommnd->GNSS_EncodeOnePacket error \r\n ");
	}
    pthread_mutex_unlock(&s->writeMutex);
	return writeLen;
}


/*--------------------------------------------------------------------------
    Function:  gps_powerRequest

    Description:
        it  encode and send  power   request  
    Parameters:
     flag: power request flag 
    Return: the write data length  
--------------------------------------------------------------------------*/
static int gps_powerRequest(int flag)
{
	GpsState*	s = _gps_state;
	TCmdData_t packet;
	int len ,writeLen;
    gpstime Gtime;    
    
    len = writeLen = 0;

	//first check it can send data ?  todo 	
	packet.type = GNSS_LIBGPS_POWER_REQUEST_TYPE;
	packet.length = 0;
	
	switch(flag) //it should add POWERON to do 
	{
		case IDLEON_STAT:
		{
			packet.subType = GNSS_LIBGPS_POWER_IDLEON_REQ_SUBTYPE;
			break;
		}
		case IDLEOFF_STAT:
		{
			if(s->first_idleoff == 0)
		    {
		        s->first_idleoff = 1;
		        packet.length = sizeof(gpstime);
		        OsTimeSystemGet(&Gtime);
		        D("first idleoff,time:weekno=%d,mscount=%d",Gtime.weekno,Gtime.mscount);
		        memcpy(packet.buff,&Gtime,packet.length);
		    }
			packet.subType = GNSS_LIBGPS_POWER_IDLEOFF_REQ_SUBTYPE;
			break;
		}
		case SHUTDOWN:
		{
			packet.subType = GNSS_LIBGPS_POWER_SHUTDOWN_REQ_SUBTYPE;
			break;
		}
		case POWERON:
		{
			packet.subType = GNSS_LIBGPS_POWER_ON_REQ_SUBTYPE;
			break;
		}
		case CHIPRESET:
		{
			packet.subType = GNSS_LIBGPS_POWER_RESET_REQ_SUBTYPE;
			break;
		}
		default:
			break;
	}

    pthread_mutex_lock(&s->writeMutex);
	len = GNSS_EncodeOnePacket( &packet, s->writeBuffer, SERIAL_WRITE_BUUFER_MAX);
	if(len > 0)
	{
		writeLen = gps_writeSerialData(s,len);     //gps_writeData2Buffer
	}
	else
	{
		E("gps_powerRequest->GNSS_EncodeOnePacket error \r\n ");
	}
    pthread_mutex_unlock(&s->writeMutex);
	D("gps_powerRequestsssss-> send power request =%d  writelen=%d",packet.subType,writeLen);

	return writeLen;
	
}

/*--------------------------------------------------------------------------
    Function:  gps_adingData

    Description:
        it  encode and send  ading  data 
        
    Parameters:
     pPak: point the  ading data information  
    Return: the write data length  
--------------------------------------------------------------------------*/
int gps_adingData(TCmdData_t*pPak)
{
    GpsState *s = _gps_state;
    int writeLen = 0, len = 0;
    TCmdData_t *packet = pPak;
    int ret =0;

    if(DEVICE_SLEEP == s->powerState)
    {
        E("GE2 has sleep so drops data \r\n ");
        return ret; 
    }
    
    pthread_mutex_lock(&s->writeMutex);
    memset(s->writeBuffer,0,SERIAL_WRITE_BUUFER_MAX);
    len = GNSS_EncodeOnePacket(packet, s->writeBuffer, SERIAL_WRITE_BUUFER_MAX);
    if(len > 0)
    {
	    writeLen = gps_writeSerialData(s,len);
    }
    else
    {
	    E("gps_powerRequest->GNSS_EncodeOnePacket error \r\n ");
    }
    pthread_mutex_unlock(&s->writeMutex);
    //gps_restorestate(s);
    return writeLen;
}

/*--------------------------------------------------------------------------
    Function:  gps_getEpheLen

    Description:
        it  get the  all the Ephemeris data 
        
    Parameters:
     pBuf: point the data buffer 
    Return: the Ephemeris data length  
--------------------------------------------------------------------------*/
static int gps_getEpheLen(const char *pBuf)
{
    TCgAgpsEphemerisCollection_t *ephe;
    int i,len = 0;

    if(pBuf != NULL)
	{
		ephe = (TCgAgpsEphemerisCollection_t *)pBuf;
	}
    else
	{
		return -1;
	}
	
    for(i = 0; i< MAX_SATS_NUMBERS; i++)
    {
        if(ephe->satellite[i].toe == 0)
        {
             len = 5 + i*(sizeof(TCgAgpsEphemeris_t));
             break;
        }
    }
    return len;
}
/*--------------------------------------------------------------------------
    Function:  adjustAdingData

    Description:
        it  adjust Ading data, such as location,time and EPHEMERIS,these data will 
        be send to engine.
        
    Parameters:
		type: Ading data type  
		src: supl send buffer
		dest:send to engine buffer

    Return: 
		int type, success for 0

--------------------------------------------------------------------------*/
static TEphemeris_t EPHval;
static GPS_EPHEMERIS_PACKED EphPacked;
static WLLH_TYPE PosTime;
static char set_init_cmcc = 0;

static int adjustAdingData(int type,const char *src, TCmdData_t *dest)
{
	GpsState *s = _gps_state;
    TCgAgpsEphemerisCollection_t *ephe;
	TCgRxNTime *time;
    TCgAgpsPosition *loc;
	unsigned int agpsmode = 1;
	int lenth = 0,i;

    if((src == NULL) || (dest == NULL))
    {
		E("src or dest null,return");
        return -1;
    }
    switch(type)
    {
		case EPHEMERIS:
            {
				E("SEND GE2 EPHEMERIS");
				ephe = (TCgAgpsEphemerisCollection_t *)src;
				E("ephe transform begin,svid count is %d",ephe->count);
				if(ephe->count > 16)
				{
					E("ephe count is too large,maybe some wrong");
					ephe->count = 16;
				}
				memcpy(dest->buff,&agpsmode,sizeof(int));
				for(i = 0; i < ephe->count; i++)
                {
					memset(&EPHval,0,sizeof(TEphemeris_t));
					memset(&EphPacked,0,sizeof(GPS_EPHEMERIS_PACKED));
					Trans_Ephpacked(&(ephe->satellite[i]),&EPHval);
					EncodeSvData(&EPHval, &EphPacked);
					if(s->use_nokiaee == 1)
					{
						EphPacked.flag = 3;  // 11  bit1 means aiding data from nokia ee
					}
					else
					{
						EphPacked.flag = 1;  // 01
					}
					EphPacked.svid = ephe->satellite[i].SatID;		
					//E("svid i:word[2]=%d,word[10]=%d,word[11]=%d",EphPacked.svid,EphPacked.word[2],EphPacked.word[10],EphPacked.word[11]);	
	
	
					memcpy(dest->buff+i*sizeof(GPS_EPHEMERIS_PACKED)+4,&EphPacked,sizeof(GPS_EPHEMERIS_PACKED));
				}
				dest->length = sizeof(GPS_EPHEMERIS_PACKED)*ephe->count+4;
            	dest->type = GNSS_LIBGPS_AIDING_DATA_TYPE;
            	dest->subType = GNSS_LIBGPS_AIDING_GPSEPHEMERIS_SUBTYPE;
				//adjust data for ge2;
			}
        	break;
		case TIME:
            {
				E("SEND GE2 TIME");
				time = (TCgRxNTime*)src;
				PosTime.weekNo = time->weekNo;
				PosTime.second = time->second;
				PosTime.secondFrac = time->secondFrac;
				if((time->weekNo > 0) && (s->cmcc == 1) && (set_init_cmcc == 0))
				{
				    D("now cmcc mode will set to ge2");
				    set_init_cmcc = 1;
				    gps_sendData(SET_CMCC_MODE, NULL); 
				}
				//E("SEND TIME OVER,weekno is %d",PosTime.weekNo);
				return 1;
				//adjust data for ge2;
			}
        	break;
		case LOCATION:
            {
				D("SEND GE2:time:week=%d,second=%d,secondfrac=%d"
                  ,PosTime.weekNo,PosTime.second,PosTime.secondFrac);
				loc = (TCgAgpsPosition *)src;
				dest->length = sizeof(WLLH_TYPE);
            	dest->type = GNSS_LIBGPS_AIDING_DATA_TYPE;
            	dest->subType = GNSS_LIBGPS_AIDING_POSVELTIME_SUBTYPE;
			    PosTime.lat = loc->latitude;
			    PosTime.lon = loc->longitude;
			    PosTime.hae = loc->altitude;
			    D("SEND GE2:location:lat=%f,lon=%f",PosTime.lat,PosTime.lon);
				memcpy(dest->buff,&PosTime,sizeof(WLLH_TYPE));
				//adjust data for ge2;
			}
        	break;
		default:
        	break;
    }
    return 0;
}

int gps_SendRequest(GpsState *s,int flag)
{
	TCmdData_t packet;
	int len = 0, writeLen = 0;

	D("gps_sendrequest enter,flag is 0x%x",flag);

	packet.type = GNSS_LIBGPS_REQUEST_TYPE;

	switch(flag)
	{
		case REQUEST_VERSION:
			packet.subType = GNSS_LIBGPS_REQUEST_VERSION_SUBTYPE;
			packet.length = 0;
			break;
		case REQUEST_CHIP:
			packet.subType = GNSS_LIBGPS_REQUEST_CHIP_SUBTYPE;
			packet.length = 0;
			break;
		case REQUEST_GPSTIME:
			packet.subType = GNSS_LIBGPS_REQUEST_GPSTIME_SUBTYPE;
			packet.length = 0;
			break;
		case REQUEST_CONSTELLATION:
			packet.subType = GNSS_LIBGPS_REQUEST_CONSTELLATION_SUBTYPE;
			packet.length = 0;
			break;
		case REQUEST_TSXTEMP:
			packet.subType = GNSS_LIBGPS_REQUEST_TSXTEMP_SUBTYPE;    // TSXTEMP
			packet.length = 0;
			break;
		default:
			E("Not support request flag \r\n ");
			return 0;
	}
	D("request type %d,subtype %d",packet.type,packet.subType);

	pthread_mutex_lock(&s->writeMutex);
	len = GNSS_EncodeOnePacket( &packet, s->writeBuffer, SERIAL_WRITE_BUUFER_MAX);
	if(len > 0)
	{
		writeLen = gps_writeSerialData(s,len); 
	}
	else
	{
		E("gps_SendRequest->GNSS_EncodeOnePacket error \r\n ");
	}
	pthread_mutex_unlock(&s->writeMutex);

	return writeLen;
}

int gps_SendNotify(GpsState *s,int flag)
{
	TCmdData_t packet;
	int len ,writeLen;
    
    len = writeLen = 0;
    pthread_mutex_lock(&s->writeMutex);
	//first check it can send data ?  todo 	
	packet.type = GNSS_LIBGPS_NOTIFIED_TYPE;
	packet.subType = GNSS_NOTIFIED_DEVICE_STATE_SUBTYPE;
	packet.length = 2;
    if(SLEEP_NOTIFY == flag)
    {
        if(DEVICE_SLEEP == s->powerState)
        {
             D("the state is sleep ,so don't send sleep again");
             pthread_mutex_unlock(&s->writeMutex);
             return writeLen;
        }
        else
        {
            packet.buff[0] = GNSS_DEVICE_SLEEP_STATE;
            packet.buff[1] = s->sleepCount++;
        }
    }
    else if(WAKEUP_NOTIFY == flag)
    {   
        packet.buff[0] = GNSS_DEVICE_READY_STATE;
        packet.buff[1] = s->wakeupCount++;
    }
    else
    {
        E("it not supply notify now");
    }
	D("request type %d,subtype %d",packet.type,packet.subType);
	len = GNSS_EncodeOnePacket( &packet, s->writeBuffer, SERIAL_WRITE_BUUFER_MAX);
	if(len > 0)
	{
		writeLen = gps_writeSerialData(s,len);     //gps_writeData2Buffer
		if(SLEEP_NOTIFY == flag){
			s->powerState = DEVICE_SLEEPING;
		}
	}
	else
	{
		E("gps_SendNotify->GNSS_EncodeOnePacket error \r\n ");
	}
    pthread_mutex_unlock(&s->writeMutex);
    return writeLen;
}

#if 1//def TEST_MODE
static int gps_eut_config(int flag, unsigned char value)
{
	GpsState*	s = _gps_state;
	TCmdData_t packet;
	int len,writeLen ;

	len = writeLen = 0;
	E("config eut,subtype is %d",flag);
	packet.type  = GNSS_LIBGPS_EUT_TYPE;

	switch(flag)
	{
		case EUT_TEST_CW:
		{
			packet.subType = GNSS_LIBGPS_EUT_CW_SUBTYPE;
			packet.length = 0;
			break;
		}
		case EUT_TEST_TSX:
		{
			packet.subType = GNSS_LIBGPS_EUT_TSXTEMP_SUBTYPE;
			packet.length = 0;
			break;
		}
		case EUT_TEST_TCXO:
		{
			packet.subType = GNSS_LIBGPS_EUT_TCXO_SUBTYPE;
			packet.length = 0;
			break;
		}
		case EUT_TEST_MIDBAND:
		{
			packet.subType = GNSS_LIBGPS_EUT_MIDBAND_SUBTYPE;
			packet.length = 0;
			break;
		}
		case RF_DATA_CAPTURE:
			packet.subType = GNSS_LIBGPS_EUT_RFDATA_CAP_SUBTYPE;
			packet.length = 1;
			packet.buff[0] = value; /*set RF test mode: 0,gps;1,glonass,2,bd*/
			break;
        case EUT_TEST_TCXO_TSX:
            {
                packet.subType = GNSS_LIBGPS_EUT_TCXO_SUBTYPE;
                packet.length = 1;
                packet.buff[0] = 1; //set   TCXO_TSX
                E("gps_eut_config-> EUT_TEST_TCXO_TSX  \r\n ");
                break;
            }
		default:
			break;
	}
	
	pthread_mutex_lock(&s->writeMutex);
	len = GNSS_EncodeOnePacket(&packet, s->writeBuffer, SERIAL_WRITE_BUUFER_MAX);

	if(len > 0)
	{
		writeLen = gps_writeSerialData(s,len);   //gps_writeData2Buffer
	}
	else
	{
		E("gps_eut_config->GNSS_EncodeOnePacket error \r\n ");
	}
	pthread_mutex_unlock(&s->writeMutex);
	
	return writeLen;
}
#endif

#ifdef GNSS_REG_WR
static int gnss_register_wr(int flag)
{
	GpsState*	s = _gps_state;
	TCmdData_t packet;
	int len,writeLen ;

	len = writeLen = 0;
	E("gnss_register_wr,subtype is %d",flag);
	packet.type  = GNSS_LIBGPS_REG_WR_TYPE;

	switch(flag)
	{
		case WRITE_REG:
		{
			packet.subType = GNSS_LIBGPS_REG_WRITE_SUBTYPE;
			packet.length = 8;
			memcpy(packet.buff,&(s->wRegAddr),sizeof(unsigned int));
			memcpy(packet.buff + sizeof(unsigned int), &(s->wRegValue), sizeof(unsigned int));
			break;
		}
		case READ_REG:
		{
			packet.subType = GNSS_LIBGPS_REG_READ_SUBTYPE;
			packet.length = 4;
			memcpy(packet.buff, &(s->rRegAddr), sizeof(unsigned int));
			break;
		}
		default:
			break;
	}
	
	pthread_mutex_lock(&s->writeMutex);
	len = GNSS_EncodeOnePacket(&packet, s->writeBuffer, SERIAL_WRITE_BUUFER_MAX);

	if(len > 0)
	{
		writeLen = gps_writeSerialData(s,len);   //gps_writeData2Buffer
	}
	else
	{
		E("gnss_register_wr->GNSS_EncodeOnePacket error \r\n ");
	}
	pthread_mutex_unlock(&s->writeMutex);
	
	return writeLen;
}
#endif

/*--------------------------------------------------------------------------
    Function:  gps_wakeupDevice

    Description:
        it  wakeup device before  send data to ge2 device   
    Parameters: gps device  
    Return: 
		 1 : it  wake up fail  0: wake up success 

--------------------------------------------------------------------------*/
int gps_wakeupDevice(GpsState *s )
{
    int wakeup_timeout=0;
    int wakeuping =1;
    int ret = 0;
    int i = 0;
    
    //first wakeup GE2 device 
	if((s->sleepFlag)&&(DEVICE_SLEEP == s->powerState))
	{
		gps_hardware_open();
		gps_setPowerState(DEVICE_WAKEUPING);
		while(wakeuping)
		{
			for(i = 0;i < 3; i++)
			{
				ret = gps_SendNotify(s,WAKEUP_NOTIFY);
                if(ret < 0)
                {
                    break;
                }
			}
			wakeuping = gps_power_Statusync(POWER_WAKEUP);
			if(wakeuping)
			{
				wakeup_timeout++;
			}
			if(wakeup_timeout == 30)
			{
			    break;
			}
		}
        //it can't wakeup device,so must download firmware 
        if(wakeuping)
        {
        
          
            ret = 1;
        }
        else
        {   
            s->powerErrorNum = 0;
            ret = 0;
        }

	}
    else
    {
        ret = 0;
    }
    return ret;
}

/*--------------------------------------------------------------------------
    Function:  gps_notiySleepDevice

    Description:
        it  sleep device before  send data to ge2 device   
    Parameters: gps device  
    Return: 

--------------------------------------------------------------------------*/
void  gps_notiySleepDevice(GpsState *s )
{
    int sleepCount = 3;
    int ret = 0;
    int i = 0;

    if(DEVICE_IDLED != s->powerState)
    {
        E(" send sleep command  the powerState should DEVICE_IDLED ,but  powerState: %s",gps_PowerStateName(s->powerState));
        return;
    }
    ret = gps_SendNotify(s,SLEEP_NOTIFY);
    if(ret){
        D("%s send sleep cmd to GNSS.", __FUNCTION__);
    }else{
        D("%s  GNSS already sleep.", __FUNCTION__);
    }

}


void gps_restorestate(GpsState *s)
{
    if(CMD_START != s->gps_flag)
    {
        if(s->postlog)
        {
            D("auto test ,so don't send cp sleep");
        }
        else
        {
            if((s->sleepFlag)&&(s->screen_off)&&(DEVICE_SLEEP != s->powerState))
            {
                gps_notiySleepDevice(s);
            }
        }
        s->keepIdleTime = 0; 
    }
}

/*--------------------------------------------------------------------------
    Function:  gps_sendData

    Description:
        it  send data in sync communication ,it should try 3 times if it send failed ,
        or it may be hardware error 
        
    Parameters:
     flag: it message type  
     pBuf: point the data buffer 
    Return: void 
--------------------------------------------------------------------------*/
void  gps_sendData(int flag,const char *pBuf )
{
	GpsState *s = _gps_state;
	int len = 0;
	TCmdData_t Pak;
    int ret = 0;
    
	if((s->gps_flag != CMD_START) && ((flag == EPHEMERIS) || (flag == TIME) || (flag == LOCATION)))
	{
		E("agps data will not send,for gps is close");
		return;
	}

    if(SLEEP_NOTIFY != flag)
    {
        ret = gps_wakeupDevice(s);	
        if(ret)
        {
            struct timeval rcv_timeout;
            rcv_timeout.tv_sec = 5;
            rcv_timeout.tv_usec = 0;
            gnss_notify_abnormal(WCND_SOCKET_NAME,&rcv_timeout,"sleep-trigger");
            
            if(!s->release) //userdebug
            {
                gps_setPowerState(DEVICE_ABNORMAL);
                createDump(s);
                return ; 
            }
            else
            {
                D("%s,gps_wakeupDevice  failed ",__FUNCTION__);
                gps_setPowerState(DEVICE_RESET);
            }
            return;
        }
    }
    
	D("gps_sendData,flag is 0x%x",flag);
        
	switch(flag)
	{
		case COMMAND:
		case SET_CPMODE:
		case SET_OUTTYPE:
		case CLEAR_CONFIG:
		case SET_INTERVAL:		// CTS_TEST
		case SET_CMCC_MODE:
		case SET_LTE_ENABLE:
		case SET_NET_MODE:
		case SET_REALEPH:
		case SET_WIFI_STATUS:
		case SET_ASSERT_STATUS:
		case SET_DELETE_AIDING_DATA_BIT:
		case SET_BASEBAND_MODE:
		{
			len = gps_sendCommnd(flag);
			break;
		}
		case IDLEON_STAT:
		case IDLEOFF_STAT:
		case SHUTDOWN:
		case POWERON:
		case CHIPRESET:
		{
			len = gps_powerRequest(flag);
			break;	
		}
		case EPHEMERIS:
		case TIME:
		case LOCATION:
		{
			ret = 0;
            if(EPHEMERIS == flag)
            {
                usleep(100000);
            }
			ret = adjustAdingData(flag,pBuf,&Pak);
			if(!ret)
			{
				len = gps_adingData(&Pak);  
                if(EPHEMERIS == flag)
                {
                    usleep(200000);
                }
			} 
			break;
		}
		//it should rewrite the agps coontrol plan 
		case ASSIST_END:
		case SUPL_END:
		case ASSIST : 
		case MSA_POSITION:	
		case POSITION:
			break;
		case SUPL_READY:
			E("request msa data");
			memset(&Pak,0,sizeof(TCmdData_t));
			Pak.type = GNSS_LIBGPS_REQUEST_TYPE;
			Pak.subType = GNSS_LIBGPS_REQUEST_MEASUREMENT_SUBTYPE;
			Pak.length = 1;
			Pak.buff[0] = *pBuf;
			len = gps_adingData(&Pak);
			break;
		case REQUEST_VERSION:
		case REQUEST_CHIP:
		case REQUEST_GPSTIME:
		case REQUEST_CONSTELLATION:
			len = gps_SendRequest(s,flag);
			break;
		case REQUEST_TSXTEMP:
			{
				int i = 0;
				/*cali mode get temp sometimes happen send data before
				gps_start finish. it can't success. just wait*/
				for(i=0; i<5; i++){
					if(s->gps_open == true)
						break;
					usleep(300000);
				}
			}
			len = gps_SendRequest(s,flag);
			break; 
		case SLEEP_NOTIFY:
			len = gps_SendNotify(s,flag);
			break;
		case EUT_TEST_CW:
		case EUT_TEST_TSX:
		case EUT_TEST_TCXO:
		case EUT_TEST_MIDBAND:
		case EUT_TEST_TCXO_TSX:
			len = gps_eut_config(flag,0);
			break;
		case RF_DATA_CAPTURE:
			len = gps_eut_config(flag, *pBuf);
			break;
		#ifdef GNSS_REG_WR
		case WRITE_REG:
		case READ_REG:
			len = gnss_register_wr(flag);
			break;
		#endif
		default:
			E("gps_sendData flag = %d error \r\n",flag);
			break;
	}
	
     D("gps_sendData,send datalen  %d",len);
     //gps_restorestate(s);
}

int gps_sendTsxData(TCmdData_t* pPak)
{
    GpsState *s = _gps_state;
    int writeLen = 0, len = 0;
    TCmdData_t *packet = pPak;

    pthread_mutex_lock(&s->writeMutex);
    memset(s->writeBuffer,0,SERIAL_WRITE_BUUFER_MAX);
    len = GNSS_EncodeOnePacket(packet, s->writeBuffer, SERIAL_WRITE_BUUFER_MAX);
    if(len > 0)
    {
	    writeLen = gps_writeSerialData(s,len);
    }
    else
    {
	    E("gps_sendTsxData error \r\n ");
    }
    pthread_mutex_unlock(&s->writeMutex);
    return writeLen;
}

void update_tsx_param(void)
{
	GpsState *s = _gps_state;
	TCmdData_t Pak;
	FILE *fp1 = NULL;
	int ret = 0;
	int rcount = 0;
	TsxData cpTsxData;
	TSX_DATA_T apTsxData[TSX_DATA_GROUP_NUM];

	if(access("/productinfo/tsx.dat", F_OK) == 0)
	{
		D("update_tsx_param: /productinfo/tsx.dat");
		fp1 = fopen("/productinfo/tsx.dat", "r");
		if(fp1 != NULL)
		{
			memset(&cpTsxData, 0, sizeof(cpTsxData));
			rcount = fread(&cpTsxData, sizeof(TsxData), 1, fp1);
			fclose(fp1);
                        if (rcount <= 0) {
				D("update_tsx_param read fail: %d", rcount);
				return;
			}

			D("update_tsx_param: count is %d, c1=%lf, c2=%lf, c3=%lf",
				rcount, cpTsxData.c1,
				cpTsxData.c2, cpTsxData.c3);

			if((cpTsxData.c1 >= -0.35)&&(cpTsxData.c1 <= -0.1))
			{
				memset(&Pak, 0, sizeof(TCmdData_t));
				Pak.type = GNSS_LIBGPS_DATA_TYPE;
				Pak.subType = GNSS_LIBGPS_SET_CP_TSX_TEMP;
				Pak.length = sizeof(cpTsxData);
				memcpy(Pak.buff, &cpTsxData, sizeof(cpTsxData));
				gps_sendTsxData(&Pak);
				D("update_tsx_param: done");
				return;
			}
		}
	}

	if(access("/productinfo/txdata.txt", F_OK) == 0)
	{
		D("update_tsx_param: /productinfo/txdata.txt");
		fp1 = fopen("/productinfo/txdata.txt", "r");
		if(fp1 != NULL)
		{
			memset(apTsxData, 0, sizeof(apTsxData));
			rcount = fread(apTsxData, sizeof(TSX_DATA_T),
				TSX_DATA_GROUP_NUM, fp1);
			fclose(fp1);
                        if (rcount <= 0) {
                                D("update_tsx_param read fail: %d", rcount);
                                return;
                        }
#ifdef GNSS_INTEGRATED
			D("read ap tsx data: count is %d, \
				%d:%d:%d, %d:%d:%d, %d:%d:%d, %d:%d:%d\n",
				rcount,
				apTsxData[0].freq, apTsxData[0].temprature,
				apTsxData[0].temprature_osc,
				apTsxData[1].freq, apTsxData[1].temprature,
				apTsxData[1].temprature_osc,
				apTsxData[2].freq, apTsxData[2].temprature,
				apTsxData[2].temprature_osc,
				apTsxData[3].freq, apTsxData[3].temprature,
				apTsxData[3].temprature_osc);
#else
			D("read ap tsx data: %d:%d, %d:%d \n",
			    apTsxData[0].freq, apTsxData[0].temprature,
			    apTsxData[1].freq, apTsxData[1].temprature);
#endif
			memset(&Pak, 0, sizeof(TCmdData_t));
			Pak.type = GNSS_LIBGPS_DATA_TYPE;
			Pak.subType = GNSS_LIBGPS_SET_AP_TSX_TEMP;
			Pak.length = sizeof(apTsxData);
			memcpy(Pak.buff, apTsxData, sizeof(apTsxData));
			gps_sendTsxData(&Pak);
		}
	}
}

/*--------------------------------------------------------------------------
    Function:  gps_setPowerState

    Description:
        it update the device power state 
        
    Parameters:
     state: it should change the device state 
     
    Return: 1 success others failed  
--------------------------------------------------------------------------*/
void gps_setPowerState(int state)
{
	GpsState*	s = _gps_state;
    int ret = 0;

	//to do check the power state is right ?
	D("GNSS STATE :%s, set power: %s",gps_PowerStateName(s->powerState),gps_PowerStateName(state));
    
	switch(state)
    {
        case DEVICE_DOWNLOADING: //inital or abornal
        {
            s->powerState = state;
            break;
        }
        case DEVICE_DOWNLOADED:
        {
            if(DEVICE_DOWNLOADING == s->powerState)
            {
                s->powerState = state;
            }
            else
            {
                ret =1;
            }
            break;
        }
        case DEVICE_WORKING:
        {
            if(DEVICE_IDLED == s->powerState)
            {
                s->powerState = state;
            }
            else
            {
                ret =1;
            }
            break;
        }
        case DEVICE_WORKED:
        {
            if(DEVICE_WORKING == s->powerState)
            {
                s->powerState = state;
            }
            else
            {
                ret =1;
            }
            break;
        }
        case DEVICE_IDLING:
        {
            if((DEVICE_WORKED == s->powerState)||
                (DEVICE_SLEEP == s->powerState))
            {
                s->powerState = state;
            }
            else
            {
                ret =1;
            }
            break;
        }
        case DEVICE_IDLED:
        {
            if((DEVICE_IDLING == s->powerState)||
                (DEVICE_DOWNLOADED== s->powerState)||
                (DEVICE_WAKEUPING == s->powerState))
            {
                s->powerState = state;
            }
            else
            {
                ret =1;
            }
            break;
        }
        case DEVICE_WAKEUPING:
        {
           s->powerState = state;
            break;
        }
        case DEVICE_SLEEP:
        {
            if((DEVICE_WORKED != s->powerState)||
               (DEVICE_WORKING != s->powerState))
            {
                s->powerState = state;
                gps_hardware_close();
            }
            else
            {
                ret =1;
            }
            break;
        }
        case DEVICE_POWEROFFING:
        {
            s->powerState = state;
            break;
        }
        case DEVICE_POWEROFFED:
        {
            if(DEVICE_POWEROFFING == s->powerState)
            {
                s->powerState = state;
            }
            else
            {
                ret =1;
            }
            break;
        }
        case DEVICE_RESET:
        {
            s->powerState = state;
            break;
        }
        case DEVICE_ABNORMAL:
        {
           s->powerState = state;
           break;
        }
        default: 
            break;
    }
	
    if(ret)
    {
        E("GNSS STATE :%s, should not set: %s",\
          gps_PowerStateName(s->powerState),\
          gps_PowerStateName(state));
    }
    else
    {
        D("SET POWER State change : %s\r\n",gps_PowerStateName(state));
    }
}

const char* gps_PowerStateName(int state)
{
    switch(state)
    {
        case DEVICE_DOWNLOADING: return "GE2 DOWNLOADING";
        case DEVICE_DOWNLOADED:  return "GE2 DOWNLOADED";
        case DEVICE_POWERONING:  return "GE2 POWERING";
        case DEVICE_POWERONED:   return "GE2 POWERONED";
        case DEVICE_WORKING:     return "GE2 CP WORKING";  
        case DEVICE_WORKED:      return "GE2 CP WORKED"; 
        case DEVICE_IDLING:      return "GE2 CP IDLING";  
        case DEVICE_IDLED:       return "GE2 CP IDLED"; 
        case DEVICE_SLEEPING:   return "GE2 CP DEVICE_SLEEPING";  
        case DEVICE_SLEEP:       return "GE2 CP SLEEP";  
        case DEVICE_WAKEUPING:   return "GE2 CP DEVICE_WAKEUPING";   
        case DEVICE_RESET:       return "GE2 CP DEVICE_RESET";
        case DEVICE_POWEROFFING: return "GE2 POWEROFFING";  
        case DEVICE_POWEROFFED:  return "GE2 POWEROFFED"; 
        case DEVICE_ABNORMAL:    return "GE2 ABNORMAL";   
        default:                 return  "GE2 UNKOWN STATE";
    }  
}

