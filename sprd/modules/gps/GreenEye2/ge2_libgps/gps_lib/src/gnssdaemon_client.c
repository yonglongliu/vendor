#include <cutils/sockets.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <math.h>
#include <sys/time.h>
#include <signal.h>
#include <hardware_legacy/power.h>

#include "gps_common.h"
#include "gps_cmd.h"
#include "gps_api.h"
#include "gps_engine.h"
//#include "agnss_type.h"
#include "gnssdaemon_client.h"


//#undef LOG_TAG
//#define LOG_TAG 	"GPSD_CLIENT"
/*
**********************************************************
                client function begin
***********************************************************
*/
void gnss_send2Daemon(GpsState* pState,char type,void* pData, unsigned short len)
{
    unsigned char data[GNSS_SOCKET_BUFFER_SIZE];
    unsigned char *pTmp = data;
    unsigned short sendLen = 0;
    int ret = 0;

    if(len > (GNSS_SOCKET_BUFFER_SIZE - 3))
    {
        E("gnss_send2Daemon input param error: len:%d",len);
        return;
    }
    //add lock
    pthread_mutex_lock(&pState->socketmutex);
    memset(data,0,GNSS_SOCKET_BUFFER_SIZE);
    *pTmp++ = type;
	*pTmp++ =  (unsigned char)((len) >> 8);
	*pTmp++ =  (unsigned char)(len);
    if(len > 0)
    {
        memcpy(pTmp,pData,len);
    }
    sendLen = len + GNSS_SOCKET_DATA_HEAD;
    
	ret = TEMP_FAILURE_RETRY(write(pState->socet_client,data,sendLen));
	if(ret < 0)
	{
		E("writes client_fd:%d fail (error:%s)",pState->socet_client, strerror(errno));
		pthread_mutex_unlock(&pState->socketmutex);
		return;
	}
	else
	{
		D("write sendLen:%d bytes to client_fd:%d ",sendLen, pState->socet_client);
	}
    //release lock 
    
    pthread_mutex_unlock(&pState->socketmutex);
    
}

static void gnss_setLteConfig(GpsState* pState,int flag)
{
    if(1 == flag)  
    {
      D("lte open,write to xml");
      config_xml_ctrl(SET_XML,"SPREADORBIT-ENABLE","TRUE",CONFIG_TYPE);
      pState->lte_open = 1;
    }
    else
    {
        D("lte close,write to xml");
        config_xml_ctrl(SET_XML,"SPREADORBIT-ENABLE","FALSE",CONFIG_TYPE);
        pState->lte_open = 0;
    }
}

#if 0
static void gnss_getLteConfig(GpsState* pState,int flag)
{
    char config = pState->lte_open;
    unsigned short len = 1;
    //send daemon notiy to do 
    gnss_send2Daemon(pState, GNSS_CONTROL_GET_LTE_TYPE,&config,len);
}
#endif
static void gnss_MolaSession(GpsState* pState,int flag)
{
    D("gnss cp mode pState point: %p,will start molr",pState);
    if(1 == flag)
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
    }
    else
    {
        // noting to do, control plan stop by agps
    }
}

static void gnss_peroidSession(GpsState* pState,int flag)
{
    if(1 == flag)
    {
		D("up mode,will start period si");
		if(agpsdata->cur_state != PEROID_SI)
		{
		    D("up mode,start period si");
		    if(pState->ref > 0)
		    {
		        E("test period mode,but gps already start,must stop first");
		        pState->ref = 1;
		        gps_stop();	
		        usleep(100000);		    
		    }
            start_agps_si(192);
		    agpsdata->peroid_mode = 1;
		    agpsdata->cur_state = PEROID_SI;
		    gps_start();
		}        
    }
    else
    {
        if(agpsdata->cur_state < 10)
        {
            D("up mode,stop period si");
            agpsdata->cur_state = MSB_SI; 
            agpsdata->peroid_mode = 0;
            stop_agps_si();
#ifdef GNSS_LCS
    gnss_lcsContrlEcid(agpsdata->si_sid,LCS_DOMAIN_UP,LCS_NWLR_ECID,0);
	gnss_lcscancelTriggerSession(agpsdata->si_sid); 
#else
	RXN_cancelTriggerSession(agpsdata->si_sid);    // SessionID
#endif
		
        }
    }
}

static void gnss_setWorkmodel(GpsState* pState,char mode)
{
    switch(mode)
    {
        case GNSS_GPS_WORKMODE:
        {
            D("gps only mode");
            config_xml_ctrl(SET_XML,"CP-MODE","001",CONFIG_TYPE);
            pState->cpmode = 0x1;  //001 gps
            break;
        }
        case GNSS_BD_WORKMODE:
        {
            D("bds only mode");
            config_xml_ctrl(SET_XML,"CP-MODE","010",CONFIG_TYPE);
            pState->cpmode = 0x2;  //010 bd only 
            break;
        }
        case GNSS_GPS_BD_WORKMODE:
        {
            config_xml_ctrl(SET_XML,"CP-MODE","011",CONFIG_TYPE);
            pState->cpmode = 0x3;  //011 gps+bd
            break;
        }
        case GNSS_GLONASS_WORKMODE:
        {
            config_xml_ctrl(SET_XML,"CP-MODE","100",CONFIG_TYPE);
            pState->cpmode = 0x4;  //101 gps+glonass
            break;
        }
        case GNSS_GPS_GLONASS_WORKMODE:
        {
            config_xml_ctrl(SET_XML,"CP-MODE","101",CONFIG_TYPE);
            pState->cpmode = 0x5;  //101 gps+glonass
            break;
        }
        default:
            E("gnss work mode error:%d",mode);
            break;
            
    }
}

//get command from gpsd 
static void gnss_getCommond(GpsState* pState,void* pData, int len)
{
    int type;
    unsigned short dataLen;
    unsigned char* pTmpData = (unsigned char*)pData;
    
    if((NULL == pData)|| len < GNSS_SOCKET_DATA_HEAD)
    {
        E("error happen point: %p, data len :%d",pData,len);
        return;
    }
    
    type = *pTmpData++; 
    dataLen = *pTmpData++;
    dataLen <<= 8;
    dataLen |= *pTmpData++;

    if(dataLen == (len -GNSS_SOCKET_DATA_HEAD))
    {
        D("the gnss_getCommond  type = %d\r\n",type);
        switch(type)
        {
            case GNSS_CONTROL_SET_WORKMODE_TYPE:
            {
                char flag = *pTmpData;

                gnss_setWorkmodel(pState,flag);
                break;
            }
            case GNSS_CONTROL_SET_LTE_TYPE:
            {
                char flag = *pTmpData;
                gnss_setLteConfig(pState,flag); 
                break;
            }
           #if 0
            case GNSS_CONTROL_GET_LTE_TYPE:
            {
                char flag = *pTmpData;
                gnss_getLteConfig(pState,flag); 
                break;
            }
           #endif
            case GNSS_CONTROL_SET_ASSERT_TYPE:                     
            {
                pState->assertType = ASSERT_TYPE_MANUAL;
                gps_sendData(SET_ASSERT_STATUS,NULL);
                break;
            }      
            case GNSS_CONTROL_SET_MOLA_TYPE:
            {
                char flag = *pTmpData;
                gnss_MolaSession(pState,flag);
                break;
            }
            case GNSS_CONTROL_SET_TRIGGER_TYPE:
            {
                char flag = *pTmpData;
                gnss_peroidSession(pState,flag);
                break;
            }
            case GNSS_CONTROL_SET_WIFI_TYPE:
            {
                D(" get wifi information ok");
                SignalEvent(&pState->gpsdsem);
                break;
            }
            case GNSS_CONTROL_SET_ANGRYGPS_AGPSMODE:
            {
                D(" get angrygps gpsmode information ok,mode is %d",*pTmpData);
                agpsdata->agps_enable = *pTmpData;
                SignalEvent(&pState->gpsdsem);
                break;
            }
            default:
            {
                break;
            }
        }
    }
    else
    {
        E("the socket rcv data error datalen : %d ,len :%d",dataLen,len);
    }

}

//daemon client thread 
void gnssdaemon_client_thread(void *arg) 
{ 
    int len = 0;
	char buff[GNSS_SOCKET_BUFFER_SIZE];
    struct timeval rcv_timeout;
	GpsState*  s = (GpsState*)arg;
    char flag = 0;

    rcv_timeout.tv_sec = 5;
    rcv_timeout.tv_usec = 0;

	s->socet_client = get_socket_fd(GNSS_LCS_SOCKET_NAME,&rcv_timeout);

	if(s->socet_client < 0)
    {
        E("open client fd fail:%s",strerror(errno));
        return;
    }
    else
    {   
        flag = 1; //connet server
        gnss_send2Daemon(s,GNSS_CONTROL_NOTIFIED_INITED_TYPE,&flag,1);
        D("$$$$ get socket client fd:%d",s->socet_client);
    }
    
	while(1)
	{
	    memset(buff,0,sizeof(buff));
	    len = read(s->socet_client,buff,sizeof(buff));
        if(len > 0)
        {
            gnss_getCommond(s,buff,len);
        }
        else
        {
            E("read socet_client error: %s",strerror(errno));
        }
	}

    
}

/*
**********************************************************
                client function end
***********************************************************
*/


