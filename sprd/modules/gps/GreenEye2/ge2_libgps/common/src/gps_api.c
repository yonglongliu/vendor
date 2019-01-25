#include <sys/socket.h> 
#include <net/if.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "gps_common.h"
#include "gps_cmd.h"
#include "gps_engine.h"
#include "gps_api.h"

extern char data_status;
extern int imsi_status;
extern int cid_status;
extern int simcard;
extern char fix_flag;
extern char *inet_ntoa(struct in_addr in);
extern GpsNiNotification gnotification[1];
extern GNSSPosition_t GnssPos;

unsigned char IMSI[IMSI_MAXLEN];
unsigned char MSISDN[MSISDN_MAXLEN];
static unsigned char supl_response = 0;
CgLocationId_t  current_info;
CgPositionParam_t gLocationInfo[1];
char mode_type = 0;

int wait_for_complete(char flag)
{
    int  t = 0,rc = 0;
   
    E("wait for complete , flag is %d",flag);
    imsi_status = 0;
    cid_status = 0;
    send_agps_cmd(flag);     //return true flag should cost some ms times

    if(flag == IMSI_GET_CMD)
    {
        while(imsi_status != STATUS_OK)
        {           
            usleep(10000);
            t++;
            if(t > 50)
            {
                rc = -1;
                break;
            }
        }
    }
    if(flag == CELL_LOCATION_GET_CMD)
    {
        while(cid_status != STATUS_OK)
        {           
            usleep(10000);
            t++;
            if(t > 50)
            {
                rc = -1;
                break;
            }
        }
    }
    if(rc == -1)
        E("the phone sim info can't get!!!,flag is %d",flag);

    return rc;
}


void set_dest_value(unsigned char *dest,char *src,int lenth)
{
    if(lenth == 1) *dest = *src - '0';
    if(lenth == 2) *dest = (*src - '0')*10 + *(src + 1) - '0';
    if(lenth == 3) *dest = (*src - '0')*100 + (*(src + 1) - '0')*10 + *(src + 2) - '0';

}

void convert_ip_type(char *src,unsigned char *dest)
{
    int i = 0,k=0,len = 0;
    char *tmp1,*tmp2;
    tmp1 = memchr(src,'.',strlen(src));
    i = tmp1 - src;

    set_dest_value(dest,src,i);
    tmp1 = tmp1 + 1;
    len = strlen(src)-i-1;
    tmp2 = memchr(tmp1,'.',len);
    i = tmp2 - tmp1;
    set_dest_value(dest + 1,tmp1,i);
    len = len - i - 1;

    tmp2 = tmp2 + 1;
    tmp1 = memchr(tmp2,'.',len);
    i = tmp1 - tmp2;
    set_dest_value(dest + 2,tmp2,i);

    len = len - i - 1;
    tmp1 = tmp1+1;
    set_dest_value(dest + 3,tmp1,len);
}

int get_ip(CgIPAddress_t *ipAddr,char *name,int lenth)
{
        int inet_sock;
        struct ifreq ifr;
        char *data;

        ipAddr->type = CgIPAddress_V4;

        inet_sock = socket(AF_INET, SOCK_DGRAM, 0);
        if(inet_sock < 0)
        {
            E("create socket is fail,return");
            return 1;
        }
        strncpy(ifr.ifr_name, name,lenth);
        ifr.ifr_name[lenth] = '\0';
        if (ioctl(inet_sock, SIOCGIFADDR, &ifr) <  0)
        {
            E("get ip fail,name is %s,error %s",name,strerror(errno));
		close(inet_sock);
		return 1;
        }
        data = (char *)inet_ntoa(((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr);

        E("%s\n", (char *)inet_ntoa(((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr));

        convert_ip_type(data,ipAddr->choice.ipv4Address);
        close(inet_sock);
        return 0;
}
//=======================================this is right get net info======================
static int inet_sock;
int get_netstat(char *name,int lenth)
{
	struct ifreq ifr;
	char *data = name;
	int i,len = lenth;
    int nameLen= strlen(name);
	D("lenth of name is %d",nameLen);
	if(lenth < 2 || nameLen < 2)
	{
	    D("devname lenth is too small");
	    return 1;
	}
	for(i = 0; i < nameLen; i++)
	{
	    if(name[i] == ' ')
	    {
	        len = len -1;
	        data = data + 1;
	    }
	}
	strncpy(ifr.ifr_name, data,len);
	ifr.ifr_name[len] = '\0';
	if (ioctl(inet_sock, SIOCGIFADDR, &ifr) <  0)
	{
		return 1;
	}
	memset(name,0,lenth);
	memcpy(name,ifr.ifr_name,len);
	E("%s net is connected\n",ifr.ifr_name);
	
	return 0;
}

int get_netinfo(int *val,char *dev_name)
{
	int fd,len,i;
	static char data[4096];
	char *ptr,*begin;

	*val = 0;
	fd = open("/proc/net/dev",O_NOCTTY | O_RDONLY | O_NONBLOCK);
	if(fd < 0) 
	{
		E("fd is null,return\n");
		return 1;
	}
	inet_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(inet_sock < 0) 
	{
		E("inet_sock is fail,return\n");
		close(fd);
		return 1;
	}
    len = read(fd,data,sizeof(data));
	E("len is %d\n",len);
	if(len > 4000)
	{
	    E("get net/dev data too long,fail");
	    close(inet_sock);
        close(fd);
        return 0;
	}
    for(ptr = data;ptr < data+len; ptr++)
	{
		ptr = memchr(ptr,':',data+len-ptr-1);
		if(ptr == NULL)
		{
			E("find token over\n");
			break;
		}

		for(i = 0; i < ptr-data; i++)
		{
			if(*(ptr-i) == '\n')
			{
				begin = ptr-i;
				break;
			}
		}
		if(i-1 > 15)
		{
		    E("get net device name is too long,fail");
		    close(inet_sock);
	        close(fd);
	        return 0;
		}
		memcpy(dev_name,ptr-i+1,i-1);
		D("dev_name:%s,lenth is %d,strlen is %zu\n",dev_name,i-1,strlen(dev_name));
	    if(strstr(dev_name,"lo") != NULL)
	    {
	        continue;
	    }
		if(!get_netstat(dev_name,i-1))
		{
			E("network is ok\n");
			*val = 1;
			break;
		}

	}
	close(inet_sock);
	close(fd);
	return strlen(dev_name);
}

unsigned short MSAPosition(TCgAgpsPosition *msaPosition,TcgVelocity  *agpsVelocity)
{
#if 0
    int len;
    char *buf;
    len = sizeof(TCgAgpsPosition) + sizeof(TcgVelocity);
    buf = malloc(len);
    if(buf == NULL)
    {
        E("malloc is fail");
        return 1;
    }
    memcpy(buf,msaPosition,sizeof(TCgAgpsPosition));
    memcpy(buf+sizeof(TCgAgpsPosition),agpsVelocity,sizeof(TcgVelocity));
    gps_sendData(MSA_POSITION,(const char *)buf);
    free(buf);
#else
    deliver_msa(msaPosition,agpsVelocity);
#endif
    return 0;
}


void SUPLReady(int *sid,char* rspType)
{
    E("suplready is enter,type is %d",*rspType);
	E("suplready sid is %d",*sid);
    mode_type = *rspType;
	agpsdata->readySid = *sid;
	if(mode_type == 2)
	{
		E("SUPLReady type is msa");
	    gps_sendData(SUPL_READY,rspType);
	}
	if(mode_type == 17)
	{
		if(agpsdata->ni_status == 1)
		{
			agpsdata->cur_state = GEO_MSA_NI; 
		}
		else
		{
			agpsdata->cur_state = GEO_MSA_SI; 
		}
	}
	if(mode_type == 18)
	{
		if(agpsdata->ni_status == 1)
		{
			agpsdata->cur_state = GEO_MSB_NI; 
		}
		else
		{
			agpsdata->cur_state = GEO_MSB_SI; 
		}
	}
	if((mode_type == 17) || (mode_type == 18))
	{
		start_geofence_timer();
	}
}

unsigned short GetPosition(unsigned char *pbuf)
{
    unsigned short ret = 0;
	E("===>>>getposition:fixflag=%d",fix_flag);
    if(fix_flag == 1)
    {
		GnssPos.positionData = agpsdata->gnssidReq;
		transGNSSPos(&gLocationInfo->pos,&GnssPos);
		GnssPos.posTime.gnssTOD = gpsTow;
		GnssPos.posTime.gnssTimeID = GNSSTimeID_GPS;
        memcpy(pbuf, &GnssPos, sizeof(GNSSPosition_t));
        ret = 0;
    }
    else
        ret = 1;
    return ret;
}


////////////////////////////////////////////////////////////////////////////////

int msg_to_libgps(char cmd, unsigned char *pbuf,void *ext1,void *ext2) 
{
  E("supl msg set,rec cmd is %d\n",cmd);
  int ret = 0,lenth = 0;
  int value = 0;
  char dev_name[128];
  GpsState*  s = _gps_state;
  
  switch(cmd) 
  {
    case MSISDN_GET_CMD:
       // wait_for_complete(MSISDN_CMD);
        if(simcard == 0) return 1;
		memcpy(pbuf, MSISDN, MSISDN_MAXLEN);
		break;
    case IMSI_GET_CMD:       
        wait_for_complete(IMSI_CMD);
        if(simcard == 0) return 1;
        E("imsi is %s",IMSI);
        memcpy(pbuf, IMSI, IMSI_MAXLEN);
        break;

   case CELL_LOCATION_GET_CMD:
#if 1
        if(simcard == 0) return 1;
        wait_for_complete(CID_CMD);
#else

		current_info.cellInfo.present = CgCellInfo_PR_gsmCell;
		current_info.cellInfo.choice.gsmCell.refMCC = 460; 
		current_info.cellInfo.choice.gsmCell.refMNC = 0; 
		current_info.cellInfo.choice.gsmCell.refLAC = 6334;  
		current_info.cellInfo.choice.gsmCell.refCI = 37282; 
#endif
        E("CELL_LOCATION_GET_CMD present:%d\n",current_info.cellInfo.present);
		if((current_info.cellInfo.choice.gsmCell.refMCC == 0) && (current_info.cellInfo.choice.gsmCell.refMNC == 0) 
		&& (current_info.cellInfo.choice.gsmCell.refLAC == 0) && (current_info.cellInfo.choice.gsmCell.refCI == 0) )
		{
			return 1;
		}
        memcpy(pbuf, &current_info, sizeof(current_info));
        break;	

   case VERIF_GET_CMD:	
        E("supl response is not used!!");
        if(pbuf)
        *pbuf = supl_response;
        break;	

   case SMS_GET_CMD:
        if(pbuf != NULL)
        {
            memcpy(gnotification, pbuf, sizeof(GpsNiNotification));
            wait_for_complete(SMS_CMD);
        }
        break;

   case DATA_GET_CMD:
        D("message(DATA_GET_CMD): data_status=%d\n",data_status);
		if(s->nokiaee_enable == 1)
		{
			data_status = 1;
		}
        if(pbuf)
        *pbuf = data_status;
        break;

   case SEND_TIME:
        //if(s->cmcc == 1)
        //gps_sendData(TIME,(const char *)ext1);
        break;

   case SEND_LOCTION:
        //if(s->cmcc == 1)
        //gps_sendData(LOCATION,(const char *)ext1);
        break;

   case SEND_MDL:
        //if(s->cmcc == 1)
         //gps_sendData(EPHEMERIS,(const char *)ext1);
        break;
   case IP_GET_CMD:
        E("IP will get");   //not support ip get
        memset(dev_name,'\0',sizeof(dev_name));
		lenth = get_netinfo(&value,dev_name);
		if(value == 1)
        	ret = get_ip((CgIPAddress_t *)pbuf,dev_name,lenth);
		else
			E("get ip fail for network is not connected");
        break;
   case WIFI_INFO:
        E("get wifi cmd");
		ret = 1;
#if WIFI_FUNC
        ret  = assist_get_wifi_info();
#endif
        if(pbuf)
        *pbuf = ret;
        break;

   case SEND_ACQUIST_ASSIST:
        E("send acquist assist,will be used in future");
		break;

   case SEND_DELIVER_ASSIST_END:
        E("we will send assist end");
		break;

   case SEND_SUPL_END:
        E("we will send supl end");
        send_agps_cmd(AGPS_END); 
		break;

   case SEND_MSA_POSITION:
		MSAPosition(ext1,ext2);
		break;

   case SEND_SUPL_READY:
	   	SUPLReady(ext1,ext2);
	   	break;

  case GET_POSITION:
		ret = GetPosition(pbuf);    //we will get position from nmea
	  	break;

   default:
		E("Unknow CMD\n");
		break;
}

   return ret;	
}



