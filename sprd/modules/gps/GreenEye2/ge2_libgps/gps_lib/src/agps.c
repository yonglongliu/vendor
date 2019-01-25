#include <cutils/sockets.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <math.h>
#include <sys/time.h>
#include <signal.h>
#include <strings.h>

#include <hardware_legacy/power.h>

#include "gps_common.h"
#include "gps_cmd.h"
#include "gps_api.h"
#include "gps_engine.h"
//-------------------this is for libgps call supl function begin--------------//
extern CgPositionParam_t gLocationInfo[1];
extern char fix_flag;
extern unsigned char IMSI[IMSI_MAXLEN];
extern unsigned char MSISDN[MSISDN_MAXLEN];   //this value should copy to state global
extern CgLocationId_t  current_info;
extern char socket_status;
extern void agps_msg_thread(void *arg);
extern int gps_adingData(TCmdData_t*pPak);
extern GNSSReqAssData_t AssistData;

AgpsData_t agpsdata[1];   //in future,this struct will be large as GpsState
char data_status = 1;
int simcard = 1;

static char agps_first = 0;
int gPosMethod = RSP_AUTONOMOUS;
// SessionID
static int sidBase = 100;
static TCgSetQoP_t SetQoP[1];
static TcgAreaEventParams_t EventParam[1];
static struct itimerval delay,ndelay;
//static int geo_times = 0;

static int time_ps = 0;
static int triggered = 0;
static double distance_old = 0;
static JavaDataType javadata;
static CgSetPosTechnologys_t  PosTechnologys[1];
static CgSetEventTriggerCap_t EventTriggerCap[1];
static setReportingCap_t	  paramReportingCap[1];
static void assist_ril_send_ni_message(uint8_t *msg, size_t sz);
static int agps_lcsinit = 0;
int get_session_id(int si)
{
    int sid ;

    sidBase = sidBase + 100;
    if(sidBase > 10000)
    {
        sidBase = 100;
    }
    if(si)//LCS_DOMAIN_UP
    {
        sid = sidBase;
    }
    else // ni 
    {
        sid = sidBase + 10000;// ni sid > 10000
    }
	return sid;
}

unsigned int CG_gotSUPLInit(unsigned char *pSUPLINIT_Data, unsigned short SUPLINIT_Len) 
{
    D("RXN_gotSUPLINIT called \n");
    int upSid = 0;
    upSid = get_session_id(0);     // get  up ni SessionID
	agpsdata->ni_sid = upSid;
    D("CG_gotSUPLInit: session id is : %d\n", upSid);
#ifdef GNSS_LCS
     gnss_lcsContrlEcid(upSid,LCS_DOMAIN_UP,LCS_NWLR_ECID,1);
     gnss_lcsgotSUPLINIT(upSid, pSUPLINIT_Data, SUPLINIT_Len);
#else
	 RXN_gotSUPLINIT(upSid, pSUPLINIT_Data, SUPLINIT_Len);
#endif
    return 0;
}

unsigned int CG_setPosMode(int aPosMethod)
{

#ifdef GNSS_LCS
     gnss_lcssetPosMode(aPosMethod);
#else
	 RXN_setPosMode(aPosMethod);
#endif
    gPosMethod = aPosMethod;	
    return 0;
}

#if 0
void talk_to_java(void *arg) 
{
    int ss,err,client;
    struct sockaddr_in server_addr,client_addr;
    int size;
    socklen_t  lenth;

    D("%s arg:%p\n",__FUNCTION__,arg);
    sem_init(&javadata.JavaEvent,0,0);
    lenth = sizeof(struct sockaddr);
    
    ss = socket(AF_INET,SOCK_STREAM,0);
    if(ss < 0)
    {
        E("socket fail\n");
        return;
    }
    bzero(&server_addr,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(7000);
    err = bind(ss,(struct sockaddr *)&server_addr,sizeof(server_addr));
    if(err < 0)
    {
        E("bind fail\n");
        close(ss);
        return;
    }
    err = listen(ss,3);
    if(err < 0)
    {
        E("listen fail\n");
        close(ss);
        return;
    }
    while((client = accept(ss,(struct sockaddr*)&client_addr,&lenth)) >= 0)
    {
        D("client is connect,server will talk to java\n");
        while(1)
        {
            sem_wait(&javadata.JavaEvent); 
            D("===>>>>will send to java data is %s\n",javadata.buff);
            size = write(client,javadata.buff,javadata.len);
            if(size <= 0) 
			{
				E("server_start:recvfail\n");
				if((size < 0) && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))
				continue;
				break;          
			}
			bzero(javadata.buff,sizeof(javadata.buff));
        }
    }
    return;
}
#endif

int AGPS_End_Session(void)
{
	GpsState*  s = _gps_state;
	
	E("=====>>>>>>supl end ===========");
	if((agpsdata->ni_status == 1) || (agpsdata->cp_begin == 1))
	{
		if(agpsdata->ni_status == 1)
		{
			D("clear ni_status");
			agpsdata->ni_status = 0;
		}
		if(agpsdata->cp_begin == 1)
		{
			D("clear cp_begin");
			agpsdata->cp_begin = 0;
		}
		D("AGPS_End_Session gps_stop");
		gps_stop();
		agpsdata->cur_state = MSB_SI;
	}

	if(agpsdata->peroid_mode == 1)
	{
		D("clear peroid_mode");
		agpsdata->peroid_mode = 0;
		gps_stop();    //should idle on set,cs maybe needed
		agpsdata->cur_state = MSB_SI;
	}

	if(0 == s->cmcc)
	s->agps_session = AGPS_SESSION_STOP;
	return 0; 
}

void deliver_msa(TCgAgpsPosition *pPosition,TcgVelocity *pvelocity)
{
	agpsdata->msaflag = 1;
	agpsdata->msalatitude = pPosition->latitude;
	agpsdata->msalongitude = pPosition->longitude;
	agpsdata->msaaltitude = pPosition->altitude;
	memcpy(&agpsdata->Velocity,pvelocity,sizeof(TcgVelocity));
}
/*--------------------------------------------------------------------------
    Function:  Rad_Calc

    Description:
        it Calculate the cycle lat and long 
        
     
    Return: double,distance between two gps point,km 
    samble:
	double MLatA = 60.001234,MLatB = 61.00346,MLonA = 121.3749,MLonB = 121.3849;
----------------------------------------------------------------------------------*/
double Rad_Calc(double MLatA,double MLonA,double MLatB,double MLonB)
{
    double ci,distance;
	D("calc:(%f,%f) to (%f,%f)",MLatA,MLonA,MLatB,MLonB);
	MLatA = 90 - MLatA;
	MLatB = 90 - MLatB;
	ci = sin(MLatA)*sin(MLatB)*cos(MLonA-MLonB) + cos(MLatA)*cos(MLatB);
	distance = EARTH_R*acos(ci)*PI/180;
    D("Distance is %f\n",distance);
    return distance*1000;
}
/*--------------------------------------------------------------------------
    Function:  Poly_Calc

    Description:
        it Calculate the Poly lat and long 
        
     
    Return: 1--inside poly,trigger geofence;0--outside poly 

----------------------------------------------------------------------------------*/
int Poly_Calc(double ALon,double ALat, TCgAgpsPosition *point,int num)
{
    int iSum, iCount, iIndex,ret;
    double dLon1, dLon2, dLat1, dLat2, dLon;

    ret = 0;
    if(num < 3) 
    {
        return ret;
    }

    iSum = 0;
    iCount = num;
    for(iIndex = 0; iIndex < iCount - 1; iIndex++)
    {
        if(iIndex == iCount - 1)
        {
            dLon1 = point[iIndex].longitude;
            dLat1 = point[iIndex].latitude;
            dLon2 = point[0].longitude;
            dLat2 = point[0].latitude;
        }
        else
        {
            dLon1 = point[iIndex].longitude;
            dLat1 = point[iIndex].latitude;
            dLon2 = point[iIndex + 1].longitude;
            dLat2 = point[iIndex + 1].latitude;
        }
        if(((ALat >= dLat1) && (ALat < dLat2)) || ((ALat >= dLat2) && (ALat < dLat1)))
        {
            if(abs(dLat1 - dLat2) > 0)
            {
                dLon = dLon1 - ((dLon1 -dLon2)*(dLat1 -ALat))/(dLat1 - dLat2);
                if (dLon < ALon)
                {
                    iSum++;
                }
            }
        }

    }
    D("isum is %d",iSum);
    if (iSum % 2 != 0)  //isum is odd,means point in poly.
    {
        ret = 1;
    }
    return ret;
}

/*--------------------------------------------------------------------------
    Function:  Ellipse_Calc

    Description:
        it Calculate the ellipse lat and long 
        
     
    Return: 1--inside ellipse,trigger geofence;0--outside ellipse 

----------------------------------------------------------------------------------*/
int Ellipse_Calc(double ALon,double ALat, TcgEllipticalArea_t *ellip)
{
    int ret = 0;
    double ALonc0 = 0,ALatc0 = 0,ALatc1 = 0,ALonc1 = 0;
    double c = 0,cx = 0,cy = 0,diff = 0,templac = 0.0;
    
    templac = (double)sin(ellip->coordinate.latitude)*sin(ellip->coordinate.latitude);
    
    c = sqrt((ellip->semiMajor*ellip->semiMajor) - (ellip->semiMinor*ellip->semiMinor));
    cy = c*sin(ellip->angle*PI/180);
    cx = c*cos(ellip->angle*PI/180);
    cy = fabs(cy);
    cx = fabs(cx);
    diff = (180*cy)/(1000*PI*EARTH_R);
    ALatc0 = ellip->coordinate.latitude + diff;
    ALatc1 = ellip->coordinate.latitude - diff;
    diff = (180*cx)/(1000*PI*EARTH_R);
    diff = acos((cos(diff) - templac)/(1 - templac));
    ALonc0 = ellip->coordinate.longitude + diff;
    ALonc1 = ellip->coordinate.longitude - diff;
    //east longtitude , north lat
    cx = Rad_Calc(ALat,ALon,ALatc0,ALonc0);
    cy = Rad_Calc(ALat,ALon,ALatc1,ALonc1);
    D("cx=%f,cy=%f",cx,cy);
    if(cx + cy > ellip->semiMajor*2)
    {
        ret = 0;
        D("point is outside ellipse");
    }
    else
    {
        ret = 1;
        D("point is inside ellipse");    
    }
    //now we get c0,c1
    return ret;
}
/*--------------------------------------------------------------------------
    Function:   get_event_type

    Description:
         get geofence event type
        
    Return: event type
----------------------------------------------------------------------------------*/
TcgAreaEventType_e get_event_type(char *type)
{
	int sum = 0;
	int ret = 0;
	if(type == NULL)
	{
		E("get event type is null");
		return ret;
	}
	sum = type[0]+type[1]+type[2];
	switch(sum)
	{
		case 'N'+'O'+'T':
			ret = TAET_NOTHING;
			break;
		case 'E'+'N'+'T':
			ret = TAET_ENTERING;
			break;
		case 'I'+'N'+'S':
			ret = TAET_INSIDE;
			break;
		case 'O'+'U'+'T':
			ret = TAET_OUTSIDE;
			break;
		case 'L'+'E'+'A':
			ret = TAET_LEAVING;
			break;
		default:
			break;
	
	}
	D("get event type is %d",ret);
	return ret;
}

double s2double(char *buf,int len)
{
    int   result = 0;
    char  temp[16];

    if (len >= (int)sizeof(temp))
        return 0.;

    memcpy( temp, buf, len );
    temp[len] = 0;
    return strtod( temp, NULL );
}

double convert_location(double  val)
{
    int     degrees = (int)(floor(val)/100);
    double  minutes = val - degrees*100.;
    double  dcoord  = degrees + minutes/60.0;
    return dcoord;
}
/*--------------------------------------------------------------------------
    Function:  geofence_handler_ni

    Description:
        start geofence ni session
        
    Return: void 
----------------------------------------------------------------------------------*/
void geofence_handler_ni(int signo)
{
	double distance = 0;
	double curlac = 0, curlong = 0;
	int hitted = 0;
	static int time_ps_old = 0;
	static int measure_req = 0;
	static int moving = 0;

	D("time current is %d second,signo =%d",time_ps + 30,signo);
	
	time_ps++;
	if(time_ps_old > 0)
	{
		if((time_ps - time_ps_old < 62) && (time_ps < 212))
		{
			D("trigged happened,wait for 60s");
			return;
		}
		else
		{
			time_ps_old = 0;
		}
	}
	if((time_ps < 212) && (triggered < agpsdata->numfixed))
	{
		if(agpsdata->cur_state == GEO_MSA_NI)
		{

			D("msa ni should request msa position");

			if(time_ps == 1)
			{
				send_agps_cmd(GEO_MSA_TRIGGER);   //geofence msa request)
			}
			if((agpsdata->msaflag == 0) && (time_ps < 210))
			{
				D("get msa data from ge2");
				if((measure_req == 0) && (time_ps%10 == 0))
				{
					D("=====>>>>>msa geofence request");
					send_agps_cmd(GEO_MSA_TRIGGER);   //geofence msa request
					measure_req = 1;
				}
				return;
			}
			if(time_ps >= 210)
			{
				return;
			}
			measure_req = 0;
			agpsdata->msaflag = 0;
			curlac = agpsdata->msalatitude;
			curlong = agpsdata->msalongitude;
		}
		else
		{
			curlac = gLocationInfo->pos.latitude;
			curlong = gLocationInfo->pos.longitude;
		}
		D("current lac is %f,long is %f",curlac,curlong);
		if((fix_flag == 1) && (curlac != 0.0))
		{
			D("check trigger condition,type is %d",agpsdata->type);
			distance = Rad_Calc(agpsdata->geolatitude,agpsdata->geolongitude,curlac,curlong);
			switch(agpsdata->type)
			{
				case TAET_ENTERING:
					//if((distance < agpsdata->radis - 15) && (distance_old > agpsdata->radis))
					if((distance < agpsdata->radis - 100) && (moving == 0))
					{
						hitted = 1;
						moving = 1;
					}
					if(distance > agpsdata->radis)
					{
						moving = 0;
					}
					break;
				case TAET_INSIDE:
					if(distance < agpsdata->radis)
					{
						hitted = 1;
					}
					break;
				case TAET_OUTSIDE:
					if(distance > agpsdata->radis)
					{
						hitted = 1;
					}
					break;
				case TAET_LEAVING:
					//if((distance > agpsdata->radis + 10) && (distance_old < agpsdata->radis))
					if((distance > agpsdata->radis + 100) && (moving == 0))
					{
						moving = 1;
						hitted = 1;
					}
					if(distance > agpsdata->radis)
					{
						moving = 0;
					}
					break;
				default:
					break;
			}
			//if(distance < agpsdata->radis)
			if(hitted == 1)
			{
				triggered++;
				time_ps_old = time_ps;
				send_agps_cmd(GEO_TRIGGER);   //geofence triggered
			}
		}
		distance_old = distance;
	}
	else
	{
		moving = 0;
		measure_req = 0;
		agpsdata->msaflag = 0;
		triggered = 0;
		distance_old = 0;
		time_ps = 0;
		memset(&ndelay,0,sizeof(struct itimerval));
		setitimer(ITIMER_REAL, &ndelay, NULL);
		send_agps_cmd(GEO_SESSION_END);   //geofence triggered
	}
	
}

void geofence_task_ni(TcgAreaEventParams_t *pParam)
{
	//memcpy(EventParam,pParam,sizeof(TcgAreaEventParams_t));
	agpsdata->type = pParam->type;
	agpsdata->radis = pParam->geoTargetArea.areaArray[0].choice.circular.radius;
	agpsdata->times = (pParam->stopTime)/(pParam->intervalTime);
	agpsdata->numfixed = pParam->maxTimes;
	agpsdata->ni_interval = pParam->intervalTime;
	agpsdata->geolatitude = pParam->geoTargetArea.areaArray[0].choice.circular.coordinate.latitude;
	agpsdata->geolongitude = pParam->geoTargetArea.areaArray[0].choice.circular.coordinate.longitude;
	
	return;
}
/*--------------------------------------------------------------------------
    Function:  geofence_handler_si

    Description:
        start geofence si session
        
    Return: void 
----------------------------------------------------------------------------------*/
void geofence_handler_si(int signo)
{
	double distance = 0;
	double curlac = 0, curlong = 0;
	int hitted = 0;
	static int si_time_ps_old = 0;
	static int si_measure_req = 0;
    static int simoving = 0;
	D("time current is %d second,signo= %d ",time_ps + 30,signo);

	time_ps++;
	if(si_time_ps_old > 0)
	{
		if((time_ps - si_time_ps_old < 62) && (time_ps < 212))
		{
			D("trigged happened,wait for 60s");
			return;
		}
		else
		{
			si_time_ps_old = 0;
		}
	}
	if((time_ps < 212) && (triggered < agpsdata->numfixed))
	{
		D("fix flag is %d",fix_flag);

		if(agpsdata->cur_state == GEO_MSA_SI)
		{
			D("msa si should request msa position");

			if(agpsdata->msaflag == 0)
			{
				D("get msa data from ge2");
				if((si_measure_req == 0) && (time_ps < 210))
				{
					send_agps_cmd(GEO_MSA_TRIGGER);   //geofence msa request
					si_measure_req = 1;
				}
				return;
			}
			if(time_ps >= 210)
			{
				return;
			}
			si_measure_req = 0;
			agpsdata->msaflag = 0;
			curlac = agpsdata->msalatitude;
			curlong = agpsdata->msalongitude;
		}
		else
		{
			curlac = gLocationInfo->pos.latitude;
			curlong = gLocationInfo->pos.longitude;
		}


		if((fix_flag == 1) && (curlac != 0))
		{
			D("check trigger condition,type is %d,curlac=%f,curlong=%f",EventParam->type,curlac,curlong);
			distance = Rad_Calc(agpsdata->geolatitude,agpsdata->geolongitude,curlac,curlong);
			switch(EventParam->type)
			{
				case TAET_ENTERING:
					if((distance < agpsdata->radis - 100) && (simoving == 0))
					{
					    javadata.len = sprintf(javadata.buff,"$PSPRD,11,1,GEOFENCE ENTERING EVENT:lac %f,long %f",curlac,curlong);
						hitted = 1;
						simoving = 1;
					}
					if(distance > agpsdata->radis)
					{
						simoving = 0;
					}
					break;
				case TAET_INSIDE:
					if(distance < agpsdata->radis)
					{
						javadata.len = sprintf(javadata.buff,"$PSPRD,11,1,GEOFENCE INSIDE EVENT:lac %f,long %f",curlac,curlong);
						hitted = 1;
					}
					break;
				case TAET_OUTSIDE:
					if(distance > agpsdata->radis)
					{
						javadata.len = sprintf(javadata.buff,"$PSPRD,11,1,GEOFENCE OUTSIDE EVENT:lac %f,long %f",curlac,curlong);
						hitted = 1;
					}
					break;
				case TAET_LEAVING:				
					if((distance > agpsdata->radis + 100) && (simoving == 0))
					{
						javadata.len = sprintf(javadata.buff,"$PSPRD,11,1,GEOFENCE LEAVING EVENT:lac %f,long %f",curlac,curlong);
						simoving = 1;
						hitted = 1;
					}
					if(distance > agpsdata->radis)
					{
						simoving = 0;
					}
					break;
				default:
					break;
			}
			if(hitted == 1)
			{
				si_time_ps_old = time_ps;
				triggered++;
				config_xml_ctrl(SET_XML,"GEO-TRIGGER","TRUE",SUPL_TYPE);
				D("signal java thread to report geofence event");
				//sem_post(&javadata.JavaEvent);
			}

		}
		distance_old = distance;
	}
	else
	{
		si_measure_req = 0;
		agpsdata->msaflag = 0;
		time_ps = 0;
		D("geofence session over");
		triggered = 0;
		distance_old = 0;
		memset(&delay,0,sizeof(struct itimerval));
		setitimer(ITIMER_REAL, &delay, NULL);
		send_agps_cmd(GEO_SESSION_END);   //geofence triggered
	}
}
void stop_agps_si(void)
{
	D("stop agps si session");
	if(agpsdata->cur_state == GEO_MSB_SI)
	{
		D("mode is geo msb si");
		setitimer(ITIMER_REAL, &delay, NULL);
		AGPS_End_Session();
	}
	return;
}
/*--------------------------------------------------------------------------
    Function:  start_geofence_timer

    Description:
        start agps si session,area and peroid
        
    Return: void 
----------------------------------------------------------------------------------*/
void start_geofence_timer(void)
{
	D("start geofence timer enter");
	if(agpsdata->ni_status == 1)
	{
		time_ps = 0;
		memset(&ndelay,0,sizeof(struct itimerval));
		setitimer(ITIMER_REAL, &ndelay, NULL);
		ndelay.it_value.tv_sec = 30;
		ndelay.it_value.tv_usec = 0;
		ndelay.it_interval.tv_sec = 1; //agpsdata->ni_interval;
		ndelay.it_interval.tv_usec = 0;
		signal(SIGALRM, geofence_handler_ni);
		setitimer(ITIMER_REAL, &ndelay, NULL);
	}
	else
	{
		time_ps = 0;
		memset(&delay,0,sizeof(struct itimerval));
		setitimer(ITIMER_REAL, &delay, NULL);
		delay.it_value.tv_sec = 30; //EventParam->startTime;
		delay.it_value.tv_usec = 0;
		delay.it_interval.tv_sec = 1;  //EventParam->intervalTime;
		delay.it_interval.tv_usec = 0;
		signal(SIGALRM, geofence_handler_si);
		setitimer(ITIMER_REAL, &delay, NULL);

	}
}

/*--------------------------------------------------------------------------
    Function:  start_agps_si

    Description:
        start agps si session,area and peroid
        
    Return: void 
----------------------------------------------------------------------------------*/
void start_agps_si(int mode)
{
	int start_time = 0,stop_time = 0,interval = 0,ret = 0;
	char *START = "PERIODIC-START-TIME";
	char *STOP  = "PERIODIC-STOP-TIME";
	char *INTER = "PERIODIC-MIN-INTERVAL";
	char *H_ACC = "HORIZON-ACCURACY";
	char *V_ACC = "VERTICAL-ACCURACY";
	char *L_AGE ="LOC-AGE";
	char *DELAY = "DELAY";
	char *TR_TYPE = "TRIGGER-TYPE";
	char value[32];

	ret = get_xml_value(START, value);
	if(ret == 0)
	{
		start_time = s2int(value,strlen(value));
		D("start_period_si: start time is %d",start_time);
	}
	
	memset(value,0,32);
	ret = get_xml_value(STOP, value);
	if(ret == 0)
	{
		stop_time = s2int(value,strlen(value));
		D("start_period_si: stop time is %d",stop_time);
	}
	
	memset(value,0,32);
	ret = get_xml_value(INTER, value);
	if(ret == 0)
	{
		interval = s2int(value,strlen(value));
		D("start_period_si: interval time is %d",interval);
	}  

	memset(SetQoP,0,sizeof(TCgSetQoP_t));
	memset(value,0,32);
	ret = get_xml_value(H_ACC, value);
	if(ret == 0)
	{
		SetQoP->QoP_HorAcc = s2int(value,strlen(value));
		if(SetQoP->QoP_HorAcc != 0)
		SetQoP->flag = SetQoP->flag | 0x01;
		
		D("start_period_si: QoP_HorAcc is %d",SetQoP->QoP_HorAcc);
	}
	
	memset(value,0,32);
	ret = get_xml_value(V_ACC, value);
	if(ret == 0)
	{
		SetQoP->QoP_VerAcc = s2int(value,strlen(value));
		if(SetQoP->QoP_VerAcc != 0)
		SetQoP->flag = SetQoP->flag | 0x02;
		
		D("start_period_si: QoP_VerAcc is %d",SetQoP->QoP_VerAcc);
	}
	
	memset(value,0,32);
	ret = get_xml_value(L_AGE, value);
	if(ret == 0)
	{
		SetQoP->QoP_MaxLocAge = s2int(value,strlen(value));
		if(SetQoP->QoP_MaxLocAge != 0)
		SetQoP->flag = SetQoP->flag | 0x04;
		
		D("start_period_si: QoP_MaxLocAge is %d",SetQoP->QoP_MaxLocAge);
	}
	
	memset(value,0,32);
	ret = get_xml_value(DELAY, value);
	if(ret == 0)
	{
		SetQoP->QoP_Delay = s2int(value,strlen(value));
		if(SetQoP->QoP_Delay != 0)
		SetQoP->flag = SetQoP->flag | 0x08;
		
		D("start_period_si: QoP_Delay is %d",SetQoP->QoP_Delay);
	}

	agpsdata->si_sid = get_session_id(1);    // SessionID
//get current trigger type , area or period
	memset(value,0,32);
	ret = get_xml_value(TR_TYPE, value);
	if(ret == 0)
	{
		if(!memcmp(value,"AREA",strlen("AREA")))
			mode = 410;
	}

	if(mode == 192)
	{
		D("start period si: session id is : %d\n", agpsdata->si_sid);
#ifdef GNSS_LCS
    gnss_lcsContrlEcid(agpsdata->si_sid,LCS_DOMAIN_UP,LCS_NWLR_ECID,1);
    gnss_lcsstartSIPeriodicSessionForGnss(agpsdata->si_sid, &AssistData, SetQoP,start_time, stop_time, interval, NULL);
#else
	SPRD_startSIPeriodicSessionForGnss(agpsdata->si_sid, &AssistData, SetQoP,start_time, stop_time, interval, NULL);
#endif	
	}
	if(mode == 410)   //geofence area
	{
		unsigned short flagAssistanceRequired;
		char *Atype = "AREA-TYPE";
		char *Ainval = "AREA-MIN-INTERVAL";
		char *Astart = "AREA-START-TIME";
		char *Astop = "AREA-STOP-TIME";
		char *Amax = "MAX-NUM";
		char *Aradis = "GEORADIUS";
		char *Alat = "GEO-LAT";
		char *Alon = "GEO-LON";
		char *Asign = "SIGN";
		char lat_sign = 0;
		

		agpsdata->cur_state = GEO_MSB_SI;   //this is very important

		flagAssistanceRequired =  CG_SUPL_DATA_TYPE_REFERENCE_LOCATION |
									CG_SUPL_DATA_TYPE_NAVIGATION_MODEL |
									CG_SUPL_DATA_TYPE_REFERENCE_TIME;
		memset(value,0,32);
		ret = get_xml_value(Atype, value);
		if(ret == 0)
		{
			EventParam->type = get_event_type(value);
		}
		memset(value,0,32);
		ret = get_xml_value(Ainval, value);
		if(ret == 0)
		{
			EventParam->intervalTime = s2int(value,strlen(value));
			D("event param:inval %ld",EventParam->intervalTime);
		}
		memset(value,0,32);
		ret = get_xml_value(Astart, value);
		if(ret == 0)
		{
			EventParam->startTime = s2int(value,strlen(value));
			D("event param:start time %ld",EventParam->startTime);
		}
		memset(value,0,32);
		ret = get_xml_value(Astop, value);
		if(ret == 0)
		{
			EventParam->stopTime = s2int(value,strlen(value));
			D("event param:stop time %ld",EventParam->stopTime);
		}
		memset(value,0,32);
		ret = get_xml_value(Amax, value);
		if(ret == 0)
		{
			EventParam->maxTimes = s2int(value,strlen(value));
			agpsdata->numfixed = EventParam->maxTimes;			
			D("event param:maxTimes %ld",EventParam->maxTimes);
		}
		EventParam->isPosEst = true;
		EventParam->geoTargetArea.count = 1;
		EventParam->geoTargetArea.areaArray[0].areaType = TGAT_CIRCULAR;
		memset(value,0,32);
		ret = get_xml_value(Aradis, value);
		if(ret == 0)
		{
			EventParam->geoTargetArea.areaArray[0].choice.circular.radius = s2int(value,strlen(value));
			agpsdata->radis = EventParam->geoTargetArea.areaArray[0].choice.circular.radius;
			D("event param:radius %ld",EventParam->geoTargetArea.areaArray[0].choice.circular.radius);
		}
		memset(value,0,32);
		ret = get_xml_value(Alat, value);
		if(ret == 0)
		{
			EventParam->geoTargetArea.areaArray[0].choice.circular.coordinate.latitude = s2double(value,strlen(value));
			agpsdata->geolatitude = EventParam->geoTargetArea.areaArray[0].choice.circular.coordinate.latitude;
			D("event param:latitude %f",agpsdata->geolatitude);
		}
		else
		{
			agpsdata->geolatitude = 0;
		}

		memset(value,0,32);
		ret = get_xml_value(Alon, value);
		if(ret == 0)
		{
			EventParam->geoTargetArea.areaArray[0].choice.circular.coordinate.longitude = s2double(value,strlen(value));
			agpsdata->geolongitude = EventParam->geoTargetArea.areaArray[0].choice.circular.coordinate.longitude;
			D("event param:longtitude %f",agpsdata->geolongitude );
		}
		else
		{
			agpsdata->geolongitude = 0;
		}
		memset(value,0,32);
		ret = get_xml_value(Asign, value);
		if(ret == 0)
		{
			if(memcmp(value,"NORTH",strlen("NORTH")))
			{
				lat_sign = 1;
			    agpsdata->north = 0;
			}
			else
			{
				lat_sign = 0;
			    agpsdata->north = 1;
			}
		}
		if((EventParam->stopTime - EventParam->startTime > 0) && (EventParam->intervalTime > 0))
		{
			D("Time check ok");
		}
		else
		{
			D("Time check fail");
			return;
		}
		if(lat_sign == 1)
			EventParam->geoTargetArea.areaArray[0].choice.circular.coordinate.latitude = -agpsdata->geolatitude;
#ifdef GNSS_LCS
            gnss_lcsContrlEcid(agpsdata->si_sid,LCS_DOMAIN_UP,LCS_NWLR_ECID,1);
    		gnss_lcsstartSIAreaEventSessionForGnss(agpsdata->si_sid,&AssistData,SetQoP,EventParam);
#else
	        SPRD_startSIAreaEventSessionForGnss(agpsdata->si_sid,&AssistData,SetQoP,EventParam);
#endif			
		
		
	
	}
}

#ifdef GNSS_LCS
void gnss_lcscp_molr(void)
{
	char value[32];
	char *m_addr = "EXTERNAL-ADDRESS";
	char *m_addr_enable = "ExtAddr Enable";
	char *m_position_method = "MPM";    // Gavin add 1103
	GpsState*  s = _gps_state;
	const U16 flagAssistanceRequested = CG_LOCATION | CG_TIME | CG_NAVIGATION_MODEL;
	unsigned short unflagAssistanceRequired = CG_SUPL_DATA_TYPE_REFERENCE_LOCATION | 
		                                  	CG_SUPL_DATA_TYPE_REFERENCE_TIME |
		                                  	CG_SUPL_DATA_TYPE_NAVIGATION_MODEL;
	TCgSetQoP_t Qop ;
    CgThirdPartyList_t thirdPartyList;
    CgThirdParty_t thirdParty; 

	memset(&Qop,0,sizeof(TCgSetQoP_t));
	gps_start();   //try start engine
	s->cmcc = 1;
	memset(value,0,sizeof(value));
	get_xml_value(m_position_method, value);
	D("start_cp_molr: MOLR position method is <%s>",value);
    agpsdata->si_sid = get_session_id(1);
    AssistData.gpsAssistanceRequired = unflagAssistanceRequired;
    thirdPartyList.CgThirdPartyNums = 1;
    thirdPartyList.pCgThirdParty = &thirdParty;
    thirdParty.present = CgThirdParty_PR_uri;
	if(memcmp(value,"LOCATION",8) == 0)
	{
		D("start_cp_molr: MOLR position method of locationEstimate.\n");
		get_xml_value(m_addr_enable, value);
		if(memcmp(value,"FALSE",5))
		{										
			memset(&thirdParty,'\0',sizeof(CgThirdParty_t));
			memset(value,0,sizeof(value));
			get_xml_value(m_addr, value);
			memcpy(&thirdParty.choice.uri,value,17);
			
			//D("start_cp_molr: read external number is %s",cpthird.str);
			D("start_cp_molr: ctrlplane si will be run,%s",thirdParty.choice.uri);
            gnss_lcsContrlEcid(agpsdata->si_sid,LCS_DOMAIN_CP,LCS_NWLR_ECID,1);
            gnss_lcsstartSISessionForGnss(agpsdata->si_sid,LCS_DOMAIN_CP,CP_SI_LocReqType_locationEstimate,
                                       &AssistData,&Qop,&thirdPartyList);
		}
		else
		{
			D("start_cp_molr: external number enable is false");		
            gnss_lcsContrlEcid(agpsdata->si_sid,LCS_DOMAIN_CP,LCS_NWLR_ECID,1);
            gnss_lcsstartSISessionForGnss(agpsdata->si_sid,LCS_DOMAIN_CP,CP_SI_LocReqType_locationEstimate,
                                       &AssistData,&Qop,NULL);
		}
	}
	else
	{
		D("start_cp_molr: MOLR position method of AssistanceData.\n");
		get_xml_value(m_addr_enable, value);
		if(memcmp(value,"FALSE",5))
		{									
			memset(&thirdParty,'\0',sizeof(CgThirdParty_t));
			memset(value,0,sizeof(value));
			get_xml_value(m_addr, value);
			memcpy(&thirdParty.choice.uri,value,17);
			//D("start_cp_molr: read external number is %s",cpthird.str);
			D("start_cp_molr:ctrlplane si will be run,%s",thirdParty.choice.uri);
            gnss_lcsContrlEcid(agpsdata->si_sid,LCS_DOMAIN_CP,LCS_NWLR_ECID,1);
            gnss_lcsstartSISessionForGnss(agpsdata->si_sid,LCS_DOMAIN_CP,CP_SI_LocReqType_AssistanceDataOnly,
                                       &AssistData,&Qop,&thirdPartyList);
		}
		else
		{ 
			D("start_cp_molr: external number enable is false");
            gnss_lcsContrlEcid(agpsdata->si_sid,LCS_DOMAIN_CP,LCS_NWLR_ECID,1);
            gnss_lcsstartSISessionForGnss(agpsdata->si_sid,LCS_DOMAIN_CP,CP_SI_LocReqType_AssistanceDataOnly,
                                       &AssistData,&Qop,NULL);
		}	
	}
	
	return;
}

#else
void start_cp_molr(void)
{
	char value[32];
	char *m_addr = "EXTERNAL-ADDRESS";
	char *m_addr_enable = "ExtAddr Enable";
	char *m_position_method = "MPM";    // Gavin add 1103
	GpsState*  s = _gps_state;
	const U16 flagAssistanceRequested = CG_LOCATION | CG_TIME | CG_NAVIGATION_MODEL;
	unsigned short unflagAssistanceRequired = CG_SUPL_DATA_TYPE_REFERENCE_LOCATION | 
		                                  	CG_SUPL_DATA_TYPE_REFERENCE_TIME |
		                                  	CG_SUPL_DATA_TYPE_NAVIGATION_MODEL;
	TCgSetQoP_t Qop ;
	CpThirdPartyAddr_t cpthird;

	memset(&Qop,0,sizeof(TCgSetQoP_t));
	gps_start();   //try start engine
#ifndef GNSS_LCS
	set_cp_callbacks();
#endif 
	memset(value,0,sizeof(value));
	get_xml_value(m_position_method, value);
	D("start_cp_molr: MOLR position method is <%s>",value);
	s->cmcc = 1;
	if(memcmp(value,"LOCATION",8) == 0)
	{
		D("start_cp_molr: MOLR position method of locationEstimate.\n");
		get_xml_value(m_addr_enable, value);
		if(memcmp(value,"FALSE",5))
		{										
			memset(&cpthird.str,'\0',18);
			memset(value,0,sizeof(value));
			get_xml_value(m_addr, value);
			memcpy(&cpthird.str,value,17);
			D("start_cp_molr: read external number is %s",cpthird.str);
			D("start_cp_molr: ctrlplane si will be run");
			ctrlplane_RXN_startSISession(CP_SI_LocReqType_locationEstimate,
			unflagAssistanceRequired, &Qop,&cpthird);
		}
		else
		{
			D("start_cp_molr: external number enable is false");
			ctrlplane_RXN_startSISession(CP_SI_LocReqType_locationEstimate, unflagAssistanceRequired, &Qop,NULL);
		}
	}
	else
	{
		D("start_cp_molr: MOLR position method of AssistanceData.\n");
		get_xml_value(m_addr_enable, value);
		if(memcmp(value,"FALSE",5))
		{										
			memset(&cpthird.str,'\0',18);
			memset(value,0,sizeof(value));
			get_xml_value(m_addr, value);
			memcpy(&cpthird.str,value,17);
			D("start_cp_molr: read external number is %s",cpthird.str);
			D("start_cp_molr:ctrlplane si will be run");
			ctrlplane_RXN_startSISession(CP_SI_LocReqType_AssistanceDataOnly,
			flagAssistanceRequested, &Qop,&cpthird);
		}
		else
		{ 
			D("start_cp_molr: external number enable is false");
			ctrlplane_RXN_startSISession(CP_SI_LocReqType_AssistanceDataOnly, flagAssistanceRequested, &Qop,NULL);
		}	
	}
	
	return;
}
#endif


void get_type_id(uint32_t flags) 
{
	GpsState*  s = _gps_state;

	memset(IMSI, 0, IMSI_MAXLEN);
	memset(MSISDN, 0, MSISDN_MAXLEN);
	E("get type id flags: %d\n", flags);
	E("memset IMSI and MSISDN\n");
	
	E("Start callback to set id\n");

	if(!s->agps_ril_callbacks.request_setid) {
		D("Have no ril callbacks\n");
	} else {
		s->agps_ril_callbacks.request_setid(flags);
	}

}

void get_ri_locationid() 
{
    GpsState*  s = _gps_state;
    if(!s->agps_ril_callbacks.request_refloc) {
        D("Have no ril callbacks\n");
    } 
    else {
       s->agps_ril_callbacks.request_refloc(0);     //here 0 maybe wrong in future.
    }
}

int s2int(char *p, char len)
{
    unsigned char i = 0, c = 0;
    int result = 0;
    for (i = 0 ; i < len; i++ )
    {
        if((p[i] < '0') || (p[i] > '9'))
        continue;

        c = p[i] - '0';
        if ((unsigned)c >= 10)
        return 1;
        result = result*10 + c;
    }

    return  result;

}


/*=====================set supl and cp param====================*/
void setGnssCapability(int systemid)
{
    GNSSCapability_t  cap;
    char mode_string[32];
    GpsState*  s = _gps_state;
    
    D("%s: enter systemid:%d\n",__FUNCTION__,systemid);

    memset(mode_string,0,sizeof(mode_string));
    if(!config_xml_ctrl(GET_XML,"CP-MODE",mode_string,CONFIG_TYPE))
    {
        D("in getresponse,get cp-mode in xml is %s",mode_string);   
        s->cpmode = (mode_string[0]-'0')*4 + (mode_string[1]-'0')*2 + mode_string[2]-'0'; 
    }
	if(s->cpmode == 1)
	{
	    D("gps only,not set capbility");
	    return;
	}
    memset(&cap, 0, sizeof(GNSSCapability_t));
    cap.posMethodList.gnssNum = 2;
    cap.posMethodList.posMethod[0].gnssID = SUPL_GNSS_ID_GALILEO;
    cap.posMethodList.posMethod[0].gnssPosMode = GNSS_POS_MODE_MSA|GNSS_POS_MODE_MSB|GNSS_POS_MODE_AUTO;
    cap.posMethodList.posMethod[0].gnssSignal = (1<<GNSS_SIGNAL_GALILEO_E1)|(1<<GNSS_SIGNAL_GALILEO_E5a);

    if((s->cpmode == 4) || (s->cpmode == 5))
    {
        cap.posMethodList.posMethod[1].gnssID = SUPL_GNSS_ID_GLONASS;
        cap.posMethodList.posMethod[1].gnssPosMode = GNSS_POS_MODE_MSA|GNSS_POS_MODE_MSB|GNSS_POS_MODE_AUTO; 
        cap.posMethodList.posMethod[1].gnssSignal = (1<<GNSS_SIGNAL_GLONASS_G1)|(1<<GNSS_SIGNAL_GLONASS_G2)|(1<<GNSS_SIGNAL_GLONASS_G3);
    }
    else
    {
        cap.posMethodList.posMethod[1].gnssID = SUPL_GNSS_ID_BDS;
        cap.posMethodList.posMethod[1].gnssPosMode = GNSS_POS_MODE_MSA|GNSS_POS_MODE_MSB|GNSS_POS_MODE_AUTO; 
        cap.posMethodList.posMethod[1].gnssSignal = (1<<GNSS_SIGNAL_BDS_B1I);
    }
#ifndef GNSS_LCS
    if(systemid == 0)
    {
        SPRD_setGnssCapability(&cap);
    }
    else
    {
	    ctrlplane_SPRD_setGnssCapability(&cap);
    }
#endif
}

void set_assdata(void)
{
    GpsState*  s = _gps_state;
    memset(&AssistData,0,sizeof(GNSSReqAssData_t));
    AssistData.gpsAssistanceRequired = CG_SUPL_DATA_TYPE_REFERENCE_LOCATION |
										 CG_SUPL_DATA_TYPE_NAVIGATION_MODEL |
										 CG_SUPL_DATA_TYPE_REFERENCE_TIME;
										 
	AssistData.refLocation = 1;	
	AssistData.commonAssFlag = 1;
	AssistData.refTime = 1;
	AssistData.ionosphericModel = 1;	
	
	AssistData.earthOrienationPara = 1;  
	if((s->cpmode == 4) || (s->cpmode == 5))
	{
	    AssistData.genericAssData[0].gnssID = SUPL_GNSS_ID_GLONASS;   //maybe this can be
        AssistData.genericAssData[0].navModelFlag = 1;
        AssistData.genericAssData[0].auxiliaryInfo = 1;
        AssistData.gnssNum = 1;    //1 to 2
    }
    else
    {
        AssistData.genericAssData[0].gnssID = SUPL_GNSS_ID_BDS;   //maybe this can be
        AssistData.genericAssData[0].navModelFlag = 1;
        AssistData.genericAssData[0].auxiliaryInfo = 1;
        AssistData.gnssNum = 1;    //1 to 2
    }
}

void set_supl_param(void)
{
	int ret = 0,version = 1,num = 0;
	int vt = 0,nt = 0,cer_ok = 1;
	char value[32];
	char tmp_server[64];
	unsigned short tmp_sPort = 7275;
	char *m_serverAddress = "supl.nokia.com";
	//char *m_cmccServer = "www.spirent-lcs.com";
	char *m_server = "SERVER-ADDRESS";
	char *m_port = "SERVER-PORT";
	char *m_version = "SUPL-VERSION";
	char *ntime = "NOTIFY-TIMEOUT";
	char *vtime = "VERIFY-TIMEOUT";
	char *cer_enable = "CER-VERIFY";
	char *cerpath = "SUPL-CER";
	char *ecid_enable = "ECID-ENABLE";
	#ifdef AGPS_PARAM_CONFIG
	int hash_v = 0;
	unsigned char realtime_v = TRUE;
	unsigned char quasirealtime_v = TRUE;
	unsigned char batch_v = TRUE;
	unsigned int maxNumPositions_v = 60;
	unsigned int maxNumMeasurements_v = 60;
	unsigned int periodicMinInt_v = 10;
	unsigned int periodicMaxInt_v = 60;
	char *hash_Algorithm = "HASH-ALGO";     // AUTO:0, SHA1:1, SHA256:2
	char *realtime = "REAL-TIME";   //  TRUE     FALSE
	char *quasirealtime = "QUASIREAL-TIME";   //  TRUE    
	char *batch = "BATCH";   //  TRUE
	char *maxNumPositions = "MAXNUM-POS";   //  60
	char *maxNumMeasurements = "MAXNUM-MSR";   //  60
	char *periodicMinInt = "PERIODIC-MININT";   //  10
	char *periodicMaxInt = "PERIODIC-MAXINT";   //  60
	#endif
	char *tls_enable = "TLS-ENABLE";
	CgSetCapability_t *pSetCapability;
	GpsState*  s = _gps_state;
    
	D("set_supl_param enter");
	memcpy(tmp_server,m_serverAddress,strlen(m_serverAddress));
	memset(value,0,sizeof(value));
	setGnssCapability(0);
	set_assdata();
	ret = get_xml_value(m_version, value);
	if(ret == 0)
	{
		version = s2int(value + 1,1);
		D("set_supl_param: supl version is %d\n",version);
#ifdef GNSS_LCS
    	gnss_lcschangserver(version,0,0);
#else
	RXN_chgVersion(version,0,0);
#endif
		memset(value,0,sizeof(value));
	}
	else
	{
#ifdef GNSS_LCS
    	gnss_lcschangserver(2,0,0);
#else
		RXN_chgVersion(2,0,0);
#endif
	}
	
	ret = get_xml_value(m_server, value);
	if(ret == 0)
	{
		memset(tmp_server,'\0',64);
		memcpy(tmp_server,value,strlen(value));

		D("set_supl_param: supl server is %s\n",tmp_server);
		memset(value,0,sizeof(value));
	}
	 
	ret = get_xml_value(m_port, value);
	if(ret == 0)
	{
		tmp_sPort = s2int(value,strlen(value));
		D("set_supl_param: supl port is %d\n",tmp_sPort);
	}

#ifdef GNSS_LCS
    gnss_lcshost_port(tmp_server,0, tmp_sPort);
#else
	if(s->use_nokiaee == 1)
	{
		RXN_setH_SLP("localhost",0, 31275);
	}
	else
	{
		RXN_setH_SLP(tmp_server,0, tmp_sPort);
	}
#endif
	//below is for setcap
	memset(value,0,sizeof(value));
	ret = get_xml_value(ntime, value);
	if(ret == 0)
	{
		nt = s2int(value,strlen(value));
		D("set_supl_param: ntime is %d\n",nt);
	}

	memset(value,0,sizeof(value));
	ret = get_xml_value(vtime, value);
	if(ret == 0)
	{
		vt = s2int(value,strlen(value));
		D("set_supl_param: vtime  is %d\n",vt);
	}
	 
	memset(value,0,sizeof(value));
	ret = get_xml_value(cer_enable, value);
	if((ret == 0) && (!memcmp(value,"FALSE",5)))
	{
		cer_ok = 0;
		D("set_supl_param: cer enable is false\n");
	}
	
	#ifdef AGPS_PARAM_CONFIG
	// hash algorithm
	memset(value,0,sizeof(value));
	ret = get_xml_value(hash_Algorithm, value);
	if(ret == 0)
	{
		if(!memcmp(value,"AUTO",4))
		{
			hash_v = 0;
			D("set_supl_param: hash algorithm =>> AUTO\n");
		}
		else if(!memcmp(value,"SHA1",4))
		{
			hash_v = 1;
			D("set_supl_param: hash algorithm =>> SHA1\n");
		}
		else if(!memcmp(value,"SHA256",6))
		{
			hash_v = 2;
			D("set_supl_param: hash algorithm =>> SHA256\n");
		}
		else
		{
			hash_v = 0;
			D("set_supl_param: hash algorithm =>> default, AUTO\n");
		}
	}

	//real time
	memset(value,0,sizeof(value));
	ret = get_xml_value(realtime, value);
	if(ret == 0)
	{
		if(!memcmp(value,"FALSE",5))
		{
			realtime_v = FALSE;
			D("set_supl_param: real time is FALSE(%d) \n", realtime_v);
		}
		else if(!memcmp(value,"TRUE",4))
		{
			realtime_v = TRUE;
			D("set_supl_param: real time is TRUE(%d) \n", realtime_v);
		}
		else
		{
			realtime_v = TRUE;
			D("set_supl_param: real time is INVALID(%d) \n", realtime_v);
		}
	}

	//quasi real time
	memset(value,0,sizeof(value));
	ret = get_xml_value(quasirealtime, value);
	if(ret == 0)
	{
		if(!memcmp(value,"FALSE",5))
		{
			quasirealtime_v = FALSE;
			D("set_supl_param: quasi real time is FALSE(%d) \n", quasirealtime_v);
		}
		else if(!memcmp(value,"TRUE",4))
		{
			quasirealtime_v = TRUE;
			D("set_supl_param: quasi real time is TRUE(%d) \n", quasirealtime_v);
		}
		else
		{
			quasirealtime_v = TRUE;
			D("set_supl_param: quasi real time is INVALID(%d) \n", quasirealtime_v);
		}
	}

	//batch
	memset(value,0,sizeof(value));
	ret = get_xml_value(batch, value);
	if(ret == 0)
	{
		if(!memcmp(value,"FALSE",5))
		{
			batch_v = FALSE;
			D("set_supl_param: batch is FALSE(%d) \n", batch_v);
		}
		else if(!memcmp(value,"TRUE",4))
		{
			batch_v = TRUE;
			D("set_supl_param: batch is TRUE(%d) \n", batch_v);
		}
		else
		{
			batch_v = TRUE;
			D("set_supl_param: batch is INVALID(%d) \n", batch_v);
		}
	}

	// maxNumPositions
	memset(value,0,sizeof(value));
	ret = get_xml_value(maxNumPositions, value);
	if(ret == 0)
	{
		maxNumPositions_v = s2int(value,strlen(value));
		D("set_supl_param: maxNumPositions(%d)\n",maxNumPositions_v);
	}

	// maxNumMeasurements
	memset(value,0,sizeof(value));
	ret = get_xml_value(maxNumMeasurements, value);
	if(ret == 0)
	{
		maxNumMeasurements_v = s2int(value,strlen(value));
		D("set_supl_param: maxNumMeasurements(%d)\n",maxNumMeasurements_v);
	}

	// periodicMinInt
	memset(value,0,sizeof(value));
	ret = get_xml_value(periodicMinInt, value);
	if(ret == 0)
	{
		periodicMinInt_v = s2int(value,strlen(value));
		D("set_supl_param: periodicMinInt(%d)\n",periodicMinInt_v);
	}

	// periodicMaxInt
	memset(value,0,sizeof(value));
	ret = get_xml_value(periodicMaxInt, value);
	if(ret == 0)
	{
		periodicMaxInt_v = s2int(value,strlen(value));
		D("set_supl_param: periodicMaxInt(%d)\n",periodicMaxInt_v);
	}
	#endif  //AGPS_PARAM_CONFIG

	pSetCapability = calloc(sizeof(CgSetCapability_t),1);
	if(pSetCapability == NULL)
	{
		E("set_supl_param: malloc pSetCapability fail \n");
		return;
	}

	pSetCapability->pPosTechnologys = PosTechnologys;
	pSetCapability->pEventTriggerCap = EventTriggerCap;
	//pSetCapability->pEventTriggerCap = NULL;
	pSetCapability->pPosTechnologys->agpsSETassisted = true;
	pSetCapability->pPosTechnologys->agpsSETBased    = true;
    pSetCapability->pPosTechnologys->agnssMsa = true;
    pSetCapability->pPosTechnologys->agnssMsb = true;
    pSetCapability->pPosTechnologys->autonomousGnss = true;
	pSetCapability->pPosTechnologys->autonomousGPS   = true;
	pSetCapability->pPosTechnologys->aFLT            = false;
	pSetCapability->pPosTechnologys->eotd            = false;
	pSetCapability->pPosTechnologys->otdoa           = false;
	memset(value,0,sizeof(value));
	ret = get_xml_value(ecid_enable, value);
	if(ret == 0)
	{
		if(!memcmp(value,"TRUE",4))
		{
			D("ecid is true");
			pSetCapability->pPosTechnologys->ecid  = true;
		}
		else
		{
			D("ecid is false");
			pSetCapability->pPosTechnologys->ecid  = false;
		}	
	}	


	pSetCapability->pEventTriggerCap->ellipticalArea            = false;
	pSetCapability->pEventTriggerCap->polygonArea               = false;
	pSetCapability->pEventTriggerCap->maxNumGeoAreaSupported    = 3;

	pSetCapability->pEventTriggerCap->maxAreaIdListSupported    = 6;
	pSetCapability->pEventTriggerCap->maxAreaIdSupportedPerList = 256;
	pSetCapability->notifVerifTimeout = vt;

	pSetCapability->notifTimeout = nt;
	pSetCapability->suplCertPathAvail = cer_ok;
	
	if(cer_ok == 1)
	{
		ret = get_xml_value(cerpath, pSetCapability->SuplCertPath);
		D("set_supl_param: cerpath is %s",pSetCapability->SuplCertPath);
	}

	pSetCapability->supportedGPSAssistanceData =  0x130;
	#ifdef AGPS_PARAM_CONFIG
	pSetCapability->verHashType = hash_v;
	pSetCapability->pReportingCap = paramReportingCap;
	pSetCapability->pReportingCap->periodicMinInt = periodicMinInt_v;
	pSetCapability->pReportingCap->periodicMaxInt = periodicMaxInt_v;
	pSetCapability->pReportingCap->repMode.realtime = realtime_v;
	pSetCapability->pReportingCap->repMode.quasirealtime= quasirealtime_v;
	pSetCapability->pReportingCap->repMode.batch= batch_v;
	pSetCapability->pReportingCap->batchRepCap.maxNumPositions = maxNumPositions_v;
	pSetCapability->pReportingCap->batchRepCap.maxNumMeasurements= maxNumMeasurements_v;

	#endif  //AGPS_PARAM_CONFIG
	memset(value,0,sizeof(value));
	ret = get_xml_value(tls_enable, value);
	if(ret == 0)
	{
		if((!memcmp(value,"TRUE",4))&&(s->use_nokiaee == 0))
		{
			D("tls enable is true");
			pSetCapability->isTlsEnabled  = true;
		}
		else
		{
			D("tls enable is false");
			pSetCapability->isTlsEnabled = false;
		}
	}
#ifdef GNSS_LCS
    gnss_lcsupcfgSetCapability(pSetCapability);
#else
	RXN_cfgSetCapability(pSetCapability);
#endif	
	free(pSetCapability);

	return;
}

void set_cp_param(void)
{
	int ret = 0,num = 0;
	int vt = 0,nt = 0,cer_ok = 1;
	char value[32];

	char *ntime = "NOTIFY-TIMEOUT";
	char *vtime = "VERIFY-TIMEOUT";
	char *cer_enable = "CER-VERIFY";
	char *cerpath = "SUPL-CER";
	char *ecid_enable = "ECID-ENABLE";
		
	#ifdef AGPS_PARAM_CONFIG
	int hash_v = 0;
	unsigned char realtime_v = TRUE;
	unsigned char quasirealtime_v = TRUE;
	unsigned char batch_v = TRUE;
	unsigned int maxNumPositions_v = 60;
	unsigned int maxNumMeasurements_v = 60;
	unsigned int periodicMinInt_v = 10;
	unsigned int periodicMaxInt_v = 60;
	char *hash_Algorithm = "HASH-ALGO";     // AUTO:0, SHA1:1, SHA256:2
	char *realtime = "REAL-TIME";   //  TRUE     FALSE
	char *quasirealtime = "QUASIREAL-TIME";   //  TRUE    
	char *batch = "BATCH";   //  TRUE
	char *maxNumPositions = "MAXNUM-POS";   //  60
	char *maxNumMeasurements = "MAXNUM-MSR";   //  60
	char *periodicMinInt = "PERIODIC-MININT";   //  10
	char *periodicMaxInt = "PERIODIC-MAXINT";   //  60

	#endif
	CgSetCapability_t *pSetCapability;
    setGnssCapability(1);
	E("will set controlplane capability");
	//below is for setcap
	memset(value,0,sizeof(value));
	ret = get_xml_value(ntime, value);
	if(ret == 0)
	{
		nt = s2int(value,strlen(value));
		D("set_supl_param: ntime is %d\n",nt);
	}

	memset(value,0,sizeof(value));
	ret = get_xml_value(vtime, value);
	if(ret == 0)
	{
		vt = s2int(value,strlen(value));
		D("set_supl_param: vtime  is %d\n",vt);
	}
	 
	memset(value,0,sizeof(value));
	ret = get_xml_value(cer_enable, value);
	if((ret == 0) && (!memcmp(value,"FALSE",5)))
	{
		cer_ok = 0;
		D("set_supl_param: cer enable is false\n");
	}
	
	#ifdef AGPS_PARAM_CONFIG
	// hash algorithm
	memset(value,0,sizeof(value));
	ret = get_xml_value(hash_Algorithm, value);
	if(ret == 0)
	{
		if(!memcmp(value,"AUTO",4))
		{
			hash_v = 0;
			D("set_supl_param: hash algorithm =>> AUTO\n");
		}
		else if(!memcmp(value,"SHA1",4))
		{
			hash_v = 1;
			D("set_supl_param: hash algorithm =>> SHA1\n");
		}
		else if(!memcmp(value,"SHA256",6))
		{
			hash_v = 2;
			D("set_supl_param: hash algorithm =>> SHA256\n");
		}
		else
		{
			hash_v = 0;
			D("set_supl_param: hash algorithm =>> default, AUTO\n");
		}
	}

	//real time
	memset(value,0,sizeof(value));
	ret = get_xml_value(realtime, value);
	if(ret == 0)
	{
		if(!memcmp(value,"FALSE",5))
		{
			realtime_v = FALSE;
			D("set_supl_param: real time is FALSE(%d) \n", realtime_v);
		}
		else if(!memcmp(value,"TRUE",4))
		{
			realtime_v = TRUE;
			D("set_supl_param: real time is TRUE(%d) \n", realtime_v);
		}
		else
		{
			realtime_v = TRUE;
			D("set_supl_param: real time is INVALID(%d) \n", realtime_v);
		}
	}

	//quasi real time
	memset(value,0,sizeof(value));
	ret = get_xml_value(quasirealtime, value);
	if(ret == 0)
	{
		if(!memcmp(value,"FALSE",5))
		{
			quasirealtime_v = FALSE;
			D("set_supl_param: quasi real time is FALSE(%d) \n", quasirealtime_v);
		}
		else if(!memcmp(value,"TRUE",4))
		{
			quasirealtime_v = TRUE;
			D("set_supl_param: quasi real time is TRUE(%d) \n", quasirealtime_v);
		}
		else
		{
			quasirealtime_v = TRUE;
			D("set_supl_param: quasi real time is INVALID(%d) \n", quasirealtime_v);
		}
	}

	//batch
	memset(value,0,sizeof(value));
	ret = get_xml_value(batch, value);
	if(ret == 0)
	{
		if(!memcmp(value,"FALSE",5))
		{
			batch_v = FALSE;
			D("set_supl_param: batch is FALSE(%d) \n", batch_v);
		}
		else if(!memcmp(value,"TRUE",4))
		{
			batch_v = TRUE;
			D("set_supl_param: batch is TRUE(%d) \n", batch_v);
		}
		else
		{
			batch_v = TRUE;
			D("set_supl_param: batch is INVALID(%d) \n", batch_v);
		}
	}

	// maxNumPositions
	memset(value,0,sizeof(value));
	ret = get_xml_value(maxNumPositions, value);
	if(ret == 0)
	{
		maxNumPositions_v = s2int(value,strlen(value));
		D("set_supl_param: maxNumPositions(%d)\n",maxNumPositions_v);
	}

	// maxNumMeasurements
	memset(value,0,sizeof(value));
	ret = get_xml_value(maxNumMeasurements, value);
	if(ret == 0)
	{
		maxNumMeasurements_v = s2int(value,strlen(value));
		D("set_supl_param: maxNumMeasurements(%d)\n",maxNumMeasurements_v);
	}

	// periodicMinInt
	memset(value,0,sizeof(value));
	ret = get_xml_value(periodicMinInt, value);
	if(ret == 0)
	{
		periodicMinInt_v = s2int(value,strlen(value));
		D("set_supl_param: periodicMinInt(%d)\n",periodicMinInt_v);
	}

	// periodicMaxInt
	memset(value,0,sizeof(value));
	ret = get_xml_value(periodicMaxInt, value);
	if(ret == 0)
	{
		periodicMaxInt_v = s2int(value,strlen(value));
		D("set_supl_param: periodicMaxInt(%d)\n",periodicMaxInt_v);
	}
	#endif  //AGPS_PARAM_CONFIG

	pSetCapability = calloc(sizeof(CgSetCapability_t),1);
	if(pSetCapability == NULL)
	{
		E("set_supl_param: malloc pSetCapability fail \n");
		return;
	}

	pSetCapability->pPosTechnologys = PosTechnologys;
	pSetCapability->pEventTriggerCap = EventTriggerCap;  //mod
	pSetCapability->pPosTechnologys->agpsSETassisted = true;
	pSetCapability->pPosTechnologys->agpsSETBased    = true;

	pSetCapability->pPosTechnologys->autonomousGPS   = true;
	pSetCapability->pPosTechnologys->aFLT            = false;
	
	pSetCapability->pPosTechnologys->eotd            = false;
	pSetCapability->pPosTechnologys->otdoa           = false;

	memset(value,0,sizeof(value));
	ret = get_xml_value(ecid_enable, value);
	if(ret == 0)
	{
		if(!memcmp(value,"TRUE",4))
		{
			D("ecid is true");
			pSetCapability->pPosTechnologys->ecid  = true;
		}
		else
		{
			D("ecid is false");
			pSetCapability->pPosTechnologys->ecid  = false;
		}	
	}	
	
	pSetCapability->pEventTriggerCap->ellipticalArea            = false;
	pSetCapability->pEventTriggerCap->polygonArea               = false;
	pSetCapability->pEventTriggerCap->maxNumGeoAreaSupported    = 3;

	pSetCapability->pEventTriggerCap->maxAreaIdListSupported    = 6;
	pSetCapability->pEventTriggerCap->maxAreaIdSupportedPerList = 256;


	pSetCapability->notifVerifTimeout = vt;

	pSetCapability->notifTimeout = nt;
	pSetCapability->suplCertPathAvail = cer_ok;
	
	if(cer_ok == 1)
	{
		ret = get_xml_value(cerpath, pSetCapability->SuplCertPath);
		D("set_supl_param: cerpath is %s",pSetCapability->SuplCertPath);
	}

	pSetCapability->supportedGPSAssistanceData =  0x130;
	#ifdef AGPS_PARAM_CONFIG
	pSetCapability->verHashType = hash_v;
	pSetCapability->pReportingCap = paramReportingCap;
	pSetCapability->pReportingCap->periodicMinInt = periodicMinInt_v;
	pSetCapability->pReportingCap->periodicMaxInt = periodicMaxInt_v;
	pSetCapability->pReportingCap->repMode.realtime = realtime_v;
	pSetCapability->pReportingCap->repMode.quasirealtime= quasirealtime_v;
	pSetCapability->pReportingCap->repMode.batch= batch_v;
	pSetCapability->pReportingCap->batchRepCap.maxNumPositions = maxNumPositions_v;
	pSetCapability->pReportingCap->batchRepCap.maxNumMeasurements= maxNumMeasurements_v;

	#endif  //AGPS_PARAM_CONFIG

#ifdef GNSS_LCS
    	gnss_lcscpcfgSetCapability(pSetCapability);
#else
		ctrlplane_RXN_cfgSetCapability(pSetCapability);
#endif

	
	free(pSetCapability);

	return;
}

/*==========================set supl and cp param end==============================*/
static void start_udp_thread(void *arg)
{
	int sin_len,i,port = 7275;  
	unsigned char message[256];  
	int length;
	int socket_descriptor = -1;  
	struct sockaddr_in sin; 
    
	D("start_udp_thread: waiting for data form sender arg: %p \n",arg);  

    memset(&sin,0,sizeof(sin));
	sin.sin_family = AF_INET;  
	sin.sin_addr.s_addr = htonl(INADDR_ANY);  
	sin.sin_port = htons(port);  
	sin_len = sizeof(sin);  
	while(socket_descriptor < 0)
	{
		socket_descriptor = socket(AF_INET,SOCK_DGRAM,0);  
		if(socket_descriptor < 0)
		{
			E("socket open fail,errno is %s\n",strerror(errno));
			sleep(1);
		}
	}
	
	bind(socket_descriptor,(struct sockaddr *)&sin,sizeof(sin));  
	D("start_udp_thread, before while");
	while(1)  
	{  
		length = recvfrom(socket_descriptor,message,sizeof(message),0,(struct sockaddr *)&sin,(socklen_t*)(&sin_len));  
		//D("response from server length is %d\n",length);  
		if(length > 0)
		{
			for(i = 0; i < length; i++)
			D("read data is:%d\n",message[i]);
			
			D("before call assist_ril_send_ni_message");
			assist_ril_send_ni_message(message, length);   //call ni message next
		}
	}  

	close(socket_descriptor);
	return;  
}

void udp_setup(GpsState *s)
{
	D("udp setup");
	agpsdata->udp_stat = 1;
	s->udp_thread = s->callbacks.create_thread_cb("udp-thread", start_udp_thread, s ); 
	if (!s->udp_thread ) 
    {
        E("could not create gps udp_thread: %s", strerror(errno));
    }	
    return;
}

#ifndef GNSS_LCS
static void agps_start()
{
	unsigned short port = 7275;
	char value[32];
	char *reset_func = "RESET-ENABLE";
	char *udp_func = "UDP-ENABLE";
	int ret = 0;

	D("agps_start: thread run \n");

	//if(agps_mode == CTRL_MODE)
	
	D("agps_start: control plane will start");        
	ctrlplane_task_enable();
	usleep(200000);
	set_cp_param();
	set_cp_callbacks();
	agent_thread();
	usleep(200000);

	D("agps_start: supl mode will start");
	RXN_startSUPLClient(port);      //this will never stop
	usleep(200000);

	set_supl_param();
	set_up_callbacks();
	
	return;
}
#endif

void check_agps_thread(GpsState *s)
{
    int time;
    if(socket_status != thread_run){
        time = 0;
        s->ril_thread = s->callbacks.create_thread_cb( "libgpssock", agps_msg_thread, s );     
        if ( !s->ril_thread ) 
        {
            D("could not create gps ril_thread: %s", strerror(errno));
         }
        while((socket_status != thread_run) && (time < 5))
        {
            time++;
            usleep(25000);
        }
    }

}

static int get_agps_mode(void)
{
	int mode = 0,ret = 0;
	char *cp_enable = "CONTROL-PLANE";
	char value[16];
	
	memset(value,0,sizeof(value));
	ret = get_xml_value(cp_enable, value);
	if((ret == 0) && (!memcmp(value,"TRUE",4)))
	{
		mode = CTRL_MODE;
	}
	else
	{
		mode = SUPL_MODE;
	}
	return mode;
}

static void start_agps_thread(GpsState *state)
{
   int time;
    if(agps_first == 1)
    {
        check_agps_thread(state);
        return;
    }
	agps_thread_init(state);
	//agps_mode = get_agps_mode();
    state->ril_thread = state->callbacks.create_thread_cb( "libagps",agps_msg_thread, state );     
    if ( !state->ril_thread ) 
    {
		E("could not create agps_msg_thread: %s", strerror(errno));
    }
    time = 0;
    while((socket_status != thread_run) && (time < 5))
    {
		time++;
		usleep(25000);
    }
#ifdef GNSS_LCS
    gnss_lcsstart();
#else
 	agps_start();
#endif
   
    agps_first = 1;

}
static void set_udp_state(void)
{
	char value[32];	
	int ret = 0;
	char *udp_func = "UDP-ENABLE";

	memset(value,0,sizeof(value));
	ret = get_xml_value(udp_func, value);
	if(ret == 0)
	{
		if(!memcmp(value,"FALSE",5))
		{
			agpsdata->udp_enable = FALSE;
			D("set_supl_param: udp is close \n");
		}
		else if(!memcmp(value,"TRUE",4))
		{
			agpsdata->udp_enable = TRUE;
			D("set_supl_param: udp is open \n");
		}
	}
}
static void assist_gps_init( AGpsCallbacks* callbacks )
{
    E("apgs init is begin");
    GpsState *s = _gps_state;
    s->agps_callbacks = *callbacks;
    agpsdata->agps_enable = 1;     //enable agps
    agpsdata->glonassNum = 0;
    agpsdata->gpsNum = 0;
    start_agps_thread(s);
	set_udp_state();
}


static int assist_gps_data_conn_open( const char* apn )
{
    D("Call assist_gps_data_conn_open");
    GpsState *s = _gps_state;

	//if((s->connected) && (SUPL_NET_REQ == s->supldata))
    //{
        D("Call assist_gps_data_conn_open connected and SignalEvent");
        s->supldata = SUPL_NET_ENABLE;
        send_agps_cmd(AGPS_NETWORK_CMD);
        //SignalEvent(&(s->readsync));   // post signal agps waite the network state
    //}


    if (apn != NULL) {

        D("DeviceName or APN : (%s)", apn);
    }
    else {
        D("DeviceName or APN is NULL");
        return -1;
    }
  
    return 0;
}



static int assist_gps_data_conn_closed(void)
{
	D("Call assist_gps_data_conn_closed");
	return 0;
}

static int assist_gps_data_conn_failed(void)
{
	GpsState *s = _gps_state;
    s->connected = 0;
    //SignalEvent(&(s->readsync));   // post signal agps waite the network state 
    E("assist_gps_data_conn_failed");
	return 0;
}

static int assist_gps_set_server( AGpsType type, const char* hostname, int port )
{
	D("assist_gps_set_server,name is %s,port is %d\n",hostname,port);
	GpsState *s = _gps_state;
	char buffer[8];
	int ret;

	if(AGPS_TYPE_SUPL == type)
	{
		strncpy(s->suplhost,hostname,sizeof(s->suplhost));
		s->suplhost[sizeof(s->suplhost)-1]='\0';
		s->suplport = port;
	}

	if(hostname != NULL) {
		//RXN_setH_SLP(hostname, 0, port);
		if(access("/system/etc/gps.conf", 0) == 0) {
			config_xml_ctrl(SET_XML,"SERVER-ADDRESS", (char*)hostname, SUPL_TYPE);
			ret = sprintf(buffer, "%d", port);
			config_xml_ctrl(SET_XML,"SERVER-PORT", buffer, SUPL_TYPE);
		}
	}

	return 0;
}

static int assist_gps_data_conn_open_with_apn_ip_type(const char* apn, ApnIpType apnIpType)
{ 
     GpsState*  s = _gps_state;

    D("%s apn:%s type:%d\n",__FUNCTION__,apn,apnIpType);
    //if((s->connected) && (SUPL_NET_REQ == s->supldata))
    //{
        D("Call assist_gps_data_conn_open_with_apn_ip_type connected and SignalEvent");
        //SignalEvent(&(s->readsync));   // post signal agps waite the network state
        send_agps_cmd(AGPS_NETWORK_CMD);
        s->supldata = SUPL_NET_ENABLE;
    //}

	return 0;
}

static const AGpsInterface sAssistGpsInterface =
{
    .size =                     sizeof(AGpsInterface),
    .init =                     assist_gps_init,
    .data_conn_open =           assist_gps_data_conn_open,
    .data_conn_closed =         assist_gps_data_conn_closed,
    .data_conn_failed =         assist_gps_data_conn_failed,
    .set_server =               assist_gps_set_server,
    .data_conn_open_with_apn_ip_type =		assist_gps_data_conn_open_with_apn_ip_type,   
};


static void gps_ni_init(GpsNiCallbacks *callbacks)
{
    D("NI init\n");
    GpsState*  s = _gps_state;
    s->ni_callbacks = *callbacks;
}


unsigned int gps_ni_send_respond(int notif_id, int user_response)
{
    unsigned int ret = 0;
    
	D("gps ni send respond,mode=%d,user_response=%d",agpsdata->cur_state,user_response);

	if(agpsdata->cur_state < 10)
	{
#ifdef GNSS_LCS
    ret = (unsigned int)gnss_lcsniResponse(notif_id,user_response,LCS_DOMAIN_UP);
#else
	RXN_niResponse(notif_id,user_response);
#endif		
	}
	else{
#ifdef GNSS_LCS
    ret = (unsigned int)gnss_lcsniResponse(notif_id,user_response,LCS_DOMAIN_CP); 
#else
    ret = ctrlplane_RXN_niResponse(notif_id,user_response);
#endif	
	}
	return ret;
}

static void gps_ni_respond(int notif_id, GpsUserResponseType user_response)
{
	D("gps_ni_respond enter");
    gps_ni_send_respond(notif_id, user_response);
}

static const  GpsNiInterface  sGpsNiInterface = {
    .size =                     sizeof(GpsNiInterface),
    .init =                     gps_ni_init,
    .respond =                	gps_ni_respond,
};

static void assist_ril_init(AGpsRilCallbacks *callbacks)
{
    D("RIL init\n");
    GpsState *s = _gps_state;
    if(!callbacks->request_refloc) {
		D("request_refloc is NULL");
	}
    if(!callbacks->request_setid) {
		D("request_setid is NULL");
	}
	
    s->agps_ril_callbacks = *callbacks;
    if(s->agps_ril_callbacks.request_setid) {
		D("s->agps_ril_callbacks.request_setid is not NULL");
	}
    if(s->agps_ril_callbacks.request_refloc) {
		D("s->agps_ril_callbacks.request_refloc is not NULL");
	}
#if WIFI_FUNC
    if(s->agps_ril_callbacks.request_wifi) {
                E("request wifi func is not null");
                s->wifi_ok = 0;
		
	}
#endif
}

static void assist_ril_ref_location(const AGpsRefLocation *agps_reflocation, size_t sz)
{
	GpsState *s = _gps_state;
	AGpsRefLocationCellID cellid = (AGpsRefLocationCellID)(agps_reflocation->u.cellID);

	if (!agps_reflocation) {
		E("agps_reflocation is NULL \n");
		return;
	}

	if (sz != sizeof(AGpsRefLocation)) {
		E("size of struct AGpsRefLocation defined in HAL does not equals to that from framework.\n");
		return;
	}

        memset(&current_info,0,sizeof(current_info));

	switch (agps_reflocation->type) {
		case AGPS_REF_LOCATION_TYPE_GSM_CELLID:
			D("type gsm enter,mcc=%d,mnc=%d,lac=%d,cid=%d\n",cellid.mcc,cellid.mnc,cellid.lac,cellid.cid);
			if(cellid.cid > 65535)
			{
				/*temp for lte bug */
				//cellid.cid = 0;
				D("gsm should switch to lte \n");
			    current_info.cellInfo.present = CgCellInfo_PR_lteCell;
			    current_info.cellInfo.choice.lteCell.refMCC = cellid.mcc;
			    current_info.cellInfo.choice.lteCell.refMNC = cellid.mnc;
			    current_info.cellInfo.choice.lteCell.refTac = cellid.lac;
			    current_info.cellInfo.choice.lteCell.refCellId = cellid.cid;
			    current_info.cellInfo.choice.lteCell.refPhysCellId = 0;  // phy cellid
			    current_info.cellInfo.choice.lteCell.refMNCDigits = 2;   // MNC length
				
			}
			else
			{
				D("type gsm enter\n");
				current_info.cellInfo.present = CgCellInfo_PR_gsmCell;
				current_info.cellInfo.choice.gsmCell.refMCC = cellid.mcc;
				current_info.cellInfo.choice.gsmCell.refMNC = cellid.mnc;
				current_info.cellInfo.choice.gsmCell.refLAC = cellid.lac;
				current_info.cellInfo.choice.gsmCell.refCI = cellid.cid ;
			}
			break;
		case AGPS_REF_LOCATION_TYPE_UMTS_CELLID:
			D("type UMTS enter,mcc=%d,mnc=%d,cid=%d\n",cellid.mcc,cellid.mnc,cellid.cid);
			current_info.cellInfo.present = CgCellInfo_PR_wcdmaCell;
			current_info.cellInfo.choice.wcdmaCell.refMCC = cellid.mcc; 
			current_info.cellInfo.choice.wcdmaCell.refMNC = cellid.mnc;   
			current_info.cellInfo.choice.wcdmaCell.refUC = cellid.cid;
			break;
		case AGPS_REF_LOCATION_TYPE_MAC:
			D("type WIFI enter\n");
			break;
#ifdef GNSS_ANDROIDN
		case AGPS_REF_LOCATION_TYPE_LTE_CELLID:
			D("type LTE enter,mcc=%d,mnc=%d,tac=%d,cid=%d,pcid=%d\n",cellid.mcc,cellid.mnc,cellid.tac,cellid.cid,cellid.pcid);
			current_info.cellInfo.present = CgCellInfo_PR_lteCell;
			current_info.cellInfo.choice.lteCell.refMCC = cellid.mcc;
			current_info.cellInfo.choice.lteCell.refMNC = cellid.mnc;
			current_info.cellInfo.choice.lteCell.refTac = cellid.tac;
			current_info.cellInfo.choice.lteCell.refCellId = cellid.cid;
			current_info.cellInfo.choice.lteCell.refPhysCellId = cellid.pcid;// phy cellid
			current_info.cellInfo.choice.lteCell.refMNCDigits = 2;
		 	break;
#endif
		default:
			E("cell type : %d not supported",agps_reflocation->type);
			return;
    }
}

static void assist_ril_set_id(AGpsSetIDType type, const char *setid)
{
    D("assist_ril_set_id\n");
    GpsState *s = _gps_state;

    if (!setid) {
        E("Not a valid setid");
        return;
    }
    simcard = 1;
    switch(type) {
        case AGPS_SETID_TYPE_IMSI:
	     E("type:AGPS_SETID_TYPE_IMSI");
            memcpy(IMSI,setid,strlen(setid));
            break;

        case AGPS_SETID_TYPE_MSISDN:
	     E("type:AGPS_SETID_TYPE_MSISDN");
            memcpy(MSISDN,setid,strlen(setid));
            break;

        case AGPS_SETID_TYPE_NONE:
            D("The phone may not have a SIM card.");
            simcard = 0;
	    return;

        default:
            E("Not a valid type as a set id");
            return;
    }
}

static void assist_ril_send_ni_message(uint8_t *msg, size_t sz)
{
    D("assist_ril_send_ni_message is enter\n");
    GpsState *s    = _gps_state;
    int ret = 0;
    
    if((NULL == msg)||(!s->init)) return;
    //call external_gps_start,
    agpsdata->ni_status = 1;
    if(s->ref > 0)
    {
        E("ni message begin,but gps is already running,ref=%d",s->ref);
        s->ref = 1;
        gps_stop();
        usleep(500000);   //here maybe not need
    }
    ret = gps_start();
    if(0 == ret)
    {
        ret = agps_request_data_conn(s);
        fix_flag = 0;
        if(0 == ret)
        {
            E("assist_ril_send_ni_message ->agps_request_data_conn ok,ni msg len:%zu",sz);
            if(sz < 512)
            {
                memset(agpsdata->suplnimsg,0,512);
                memcpy(agpsdata->suplnimsg,msg,sz);
                agpsdata->suplniLen = sz;
            }
            else
            {
                E("assist_ril_send_ni_message ->ni leng oversize 512 error");
            }
        }
        else
        {
            E("assist_ril_send_ni_message ->agps_request_data_conn error");
        }
    }
}



static void assist_ril_update_network_state(int connected, int type, int roaming, const char* extra_info)
{		
    GpsState *s = _gps_state;
    D("%s: connected=%d,type=%d,roaming=%d,extra:%s\n",__func__,connected,type,roaming,extra_info);
	s->network_status = connected;

	if(s->nokiaee_enable == 1)
	{
		connected = 1;
	}

    s->connected = connected;
    s->type = type;
    data_status = connected;
#if 0
    if(s->connected)
    {
        SignalEvent(&(s->readsync));   // post signal agps waite the network state 
    }
#endif 

    if(agpsdata->cur_state < 10)
    {
#ifndef GNSS_LCS
		RXN_psStatus(connected);
#endif
	}

	if((agpsdata->agps_enable == 0)&&(s->nokiaee_enable == 0))
	{
	    D("agps and nokiaee all closed, so network status should be set disconnected.");
	    connected = 0;
	}
    
    return;
}

static void assist_ril_update_network_availability(int avaiable, const char* apn)
{
    GpsState *s = _gps_state;
    D("%s: avaiable=%d,apn=%s\n",__func__,avaiable,apn);
    s->avaiable = avaiable;
}

#if WIFI_FUNC
static void assist_ril_set_wifi_info(uint32_t flag, const char * mac_addr)
{
    GpsState *s = _gps_state;
    E("set wifi info is enter now,rec mac is %s",mac_addr);
    s->wifi_ok = 1;
}


char assist_get_wifi_info(void)
{
    char time = 0;
    GpsState *s = _gps_state;
    char flag = WIFI_CMD;

    send_agps_cmd(flag);

    while((time < 10) && (s->wifi_ok == 0))
    {
	time++;
	usleep(200000);
    }

    if(s->wifi_ok == 1)
    {
	s->wifi_ok = 0;
	return 0;
    }
   
    return 1;
}
#endif

static const AGpsRilInterface sAssistGpsRilInterface =
{
    .size =                 	   sizeof(AGpsRilInterface),
    .init =                 	   assist_ril_init,
    .set_ref_location =     	   assist_ril_ref_location,
    .set_set_id =           	   assist_ril_set_id,
#if WIFI_FUNC
    .set_wifi_info =               assist_ril_set_wifi_info,
#endif
    .ni_message =           	   assist_ril_send_ni_message,
    .update_network_state = 	   assist_ril_update_network_state,
    .update_network_availability = assist_ril_update_network_availability,
};
/*============================add interface begin========================*/
//this is xtra interface here;

static int gps_xtra_init(GpsXtraCallbacks* callbacks)
{
    GpsState*  s = _gps_state;
    if(callbacks == NULL)  return -1;
    if(callbacks->download_request_cb == NULL)
	D("callbacks's download_request_cb is null\n");
    if(callbacks->create_thread_cb == NULL)
	D("callbacks's create_thread_cb is null\n");
    s->xtra_callbacks = *callbacks;
    return 0;
}

static int  gps_inject_xtra_data( char* data, int length )
{
    D("gps_inject_xtra_data is enter, %s :%d\n",data,length);
    return 0;
}


static const GpsXtraInterface    sGpsXtraInterface =
{
   .size                 = 	sizeof(GpsXtraInterface),
   .init                 = 	gps_xtra_init,
   .inject_xtra_data     =      gps_inject_xtra_data,
};

//debug interface func
size_t gps_get_internal_state(char* buffer, size_t bufferSize)
{
     D("gps_get_internal_state %s, len =%zu\n",buffer,bufferSize);
     return 0;
}

static const GpsDebugInterface   sGpsDebugInterface =
{
   .size                 = 	sizeof(GpsDebugInterface),
   .get_internal_state   =      gps_get_internal_state,
};


void  gps_geofence_init(GpsGeofenceCallbacks* callbacks )
{
        GpsState*  s = _gps_state;
	if(callbacks == NULL){
	D("geo callback is null\n");
	return;
	}

	s->geofence_callbacks= *callbacks;
	return;

}
void gps_add_geofence_area(int32_t geofence_id, double latitude,
			double longitude, double radius_meters,
			int last_transition, int monitor_transitions,
			int notification_responsiveness_ms,
			int unknown_timer_ms)
{
	D("gps_add_geofence_area:%d,%f,%f,%f,%d,%d,%d,%d\n",\
        geofence_id,latitude,longitude,radius_meters,last_transition,\
        monitor_transitions,notification_responsiveness_ms,unknown_timer_ms);
	return;
}
void gps_pause_geofence(int32_t geofence_id)
{
    D("gps_pause_geofence geofence_id: %d \n",geofence_id);
    return;
}

void gps_resume_geofence(int32_t geofence_id, int monitor_transitions)
{
    D("gps_resume_geofence id = %d transitions= %d\n",geofence_id,monitor_transitions);
    return;

}

void gps_remove_geofence_area(int32_t geofence_id)
{

    D("gps_remove_geofence_area id: %d\n",geofence_id);
    return;

}

static const GpsGeofencingInterface sGpsGeoInterface =
{
   .size                 = 	sizeof(GpsGeofencingInterface),
   .init                 = 	gps_geofence_init,
   .add_geofence_area    =      gps_add_geofence_area,
   .pause_geofence       =      gps_pause_geofence,
   .resume_geofence      =      gps_resume_geofence,
   .remove_geofence_area =      gps_remove_geofence_area,
};

#ifdef GNSS_ANDROIDN
// GNSS_CONFIGURATION_INTERFACE
void gnss_configuration_update (const char* config_data, int32_t length)
{
     D("%s enter: %s,len:%d\n", __func__,config_data,length);
}

static const GnssConfigurationInterface sGnssConfigurationInterface =
{
	.size                 = sizeof(GnssConfigurationInterface),
	.configuration_update = gnss_configuration_update,
};
#endif //GNSS_ANDROIDN 

// GPS_NAVIGATION_MESSAGE_INTERFACE
int gps_navigation_message_start(GpsNavigationMessageCallbacks* callbacks)
{
    GpsState*  s = _gps_state;

    D("%s enter", __func__);
	if(callbacks == NULL){
	    E("GpsNavigationMessageCallbacks is null\n");
	    return GPS_NAVIGATION_MESSAGE_ERROR_GENERIC;
	}

	if(s->navigation_message_callbacks.navigation_message_callback != NULL)
	{
	    E("navigation navigation_message_callback message interface already init\n");
	    return GPS_NAVIGATION_MESSAGE_ERROR_ALREADY_INIT;
	}
#ifdef GNSS_ANDROIDN
    if(s->navigation_message_callbacks.gnss_navigation_message_callback != NULL)
        
	{
	    E("navigation gnss_navigation_message_callback  message interface already init\n");
	    return GPS_NAVIGATION_MESSAGE_ERROR_ALREADY_INIT;
	}
#endif //GNSS_ANDROIDN
	s->navigation_message_callbacks = *callbacks;
    return GPS_NAVIGATION_MESSAGE_OPERATION_SUCCESS;
}

void gps_navigation_message_close(void)
{
    GpsState*  s = _gps_state;
    D("%s enter", __func__);

	s->navigation_message_callbacks.navigation_message_callback = NULL;
#ifdef GNSS_ANDROIDN
	s->navigation_message_callbacks.gnss_navigation_message_callback = NULL;
#endif //GNSS_ANDROIDN

}

static const GpsNavigationMessageInterface sGpsNavigationMessageInterface =
{
	.size                 = sizeof(GpsNavigationMessageInterface),
	.init                 = gps_navigation_message_start,
	.close                = gps_navigation_message_close,
};

// GPS_MEASUREMENT_INTERFACE
int gps_measurement_init(GpsMeasurementCallbacks* callbacks)
{
    GpsState*  s = _gps_state;

    D("%s enter",__func__);
	if(callbacks == NULL){
	    E("GpsMeasurementCallbacks is null\n");
	    return GPS_MEASUREMENT_ERROR_GENERIC;
	}

	if(s->measurement_callbacks.measurement_callback != NULL)
	{
	    E("measurement measurement_callback interface already init.\n");
	    return GPS_MEASUREMENT_ERROR_ALREADY_INIT;
	}
#ifdef GNSS_ANDROIDN
    if(s->measurement_callbacks.gnss_measurement_callback != NULL)
    {
	    E("measurement gnss_measurement_callback interface already init.\n");
	    return GPS_MEASUREMENT_ERROR_ALREADY_INIT;
    }
#endif//GNSS_ANDROIDN
	s->measurement_callbacks = *callbacks;
    return GPS_MEASUREMENT_OPERATION_SUCCESS;
}

void gps_measurement_close(void)
{
    GpsState*  s = _gps_state;

    D("%s enter",__func__);
	s->measurement_callbacks.measurement_callback = NULL;
#ifdef GNSS_ANDROIDN
	s->measurement_callbacks.gnss_measurement_callback = NULL;
#endif//GNSS_ANDROIDN

}

static const GpsMeasurementInterface sGpsMeasurementInterface =
{
	.size                 = sizeof(GpsMeasurementInterface),
	.init                 = gps_measurement_init,
	.close                = gps_measurement_close,
};

const void* gps_get_extension(const char* name)
{	
	D("%s, interface name: %s", __func__, name);

	if (strcmp(name, AGPS_INTERFACE) == 0) {
		D("Set agps callback\n");
#ifdef GNSS_LCS
		if(0 == agps_lcsinit)
		{
			gnss_lcsinit();
			agps_lcsinit = 1;
		}
#endif 
		return &sAssistGpsInterface;
	}
	//For ni
	if (strcmp(name, GPS_NI_INTERFACE) == 0) {
		D("Set NI callback\n");
        return &sGpsNiInterface;
    	}
	//For ri
        if (strcmp(name,AGPS_RIL_INTERFACE) == 0) {
	    D("Set ril callback\n");
        return &sAssistGpsRilInterface;
        }
	//for xtra
        if (strcmp(name,GPS_XTRA_INTERFACE) == 0) {
	    	D("Set xtra callback\n");
            	return &sGpsXtraInterface;
        }
	//for geofence
        if (strcmp(name,GPS_GEOFENCING_INTERFACE) == 0) {
	    	D("Set geofence callback\n");
            	return &sGpsGeoInterface;
        }
	//for debug
        if (strcmp(name,GPS_DEBUG_INTERFACE) == 0) {
	    	D("Set debug callback\n");
            	return &sGpsDebugInterface;
        }
#ifdef GNSS_ANDROIDN
	if (strcmp(name,GNSS_CONFIGURATION_INTERFACE) == 0) {
		D("set configuration callback\n");
		return &sGnssConfigurationInterface;
	}
#endif //GNSS_ANDROIDN 
	if (strcmp(name,GPS_MEASUREMENT_INTERFACE) == 0) {
		D("set measurement callback\n");
		return &sGpsMeasurementInterface;
	}

	if (strcmp(name,GPS_NAVIGATION_MESSAGE_INTERFACE) == 0) {
		D("set navigation message callback\n");
		return &sGpsNavigationMessageInterface;
	}

	return NULL;
}
