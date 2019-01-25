#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <math.h>

#include <cutils/log.h>
#include <cutils/properties.h>

#include "gps_ril_cmd.h"
#include "agps_api.h"
#include "gps_api.h"
#include "gps_common.h"
#include "gps_cmd.h"
#include "gps_engine.h"
#include "gnssdaemon_client.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define  LOG_TAG  "LIBGPS_INTERFACE"

extern char fix_flag;
#ifndef GNSS_LCS
extern unsigned long int libgps_c_plane_stack_network_ulreq(agps_cp_ul_pdu_params_t *pParams);
extern unsigned long int libgps_lcsverificationresp(unsigned long notifId,unsigned long present, 
                                                    unsigned long verificationResponse);
extern unsigned long int lcs_molr(agps_cp_molr_params_t *agps_molr_params);
extern unsigned long int lcs_pos(agps_cp_lcs_pos_params_t *cp_agps_pos_params);
#endif 



void CG_SendGlonassEph(int count);

CgHplmnInfo_t hpinfo; 

#define MSISDN_LEN        7

typedef struct TIMEPOS
{
    int timeflag;
    int locflag;
    TCgRxNTime RxTime;
    TCgAgpsPosition RxLocation;
}AGNSSTIMEPOS;
static AGNSSTIMEPOS timepos_0;
static TCgAgpsEphemerisCollection_t gnsseph;
static GLONASS_EPHEMERIS glonass_eph[28];
static TCmdData_t Pak;
static IONO_PARAM iono;

CG_SO_SETAPI int CG_closePhoneItf(void) 
{
	return 0;
}

int CG_deliverAssistData_RefTime(int sid, TCgRxNTime *apTime)
{
	msg_to_libgps(SEND_TIME,0,(void *)apTime,0);
	D("CG_deliverAssistData_RefTime sid:%d [weekNo:%d][second:%ud]\n",sid,apTime->weekNo,(unsigned int)apTime->second);
	return 0;
}
int CG_deliverAssistData_RefLoc(int sid, void * apPos, TCgRxNTime * apTime)
{
	msg_to_libgps(SEND_LOCTION,0,apPos,0);
    D("%s sid:%d,system: %ud",__FUNCTION__,sid,(unsigned int)apTime->sysTime);
	return 0;
}
int CG_deliverAssistData_NavMdl(int sid, TCgAgpsEphemerisCollection_t* apEphemerisCollection)
{
	msg_to_libgps(SEND_MDL,0,apEphemerisCollection,0);
	
	TCgAgpsEphemerisCollection_t *ephCollection = (TCgAgpsEphemerisCollection_t *)apEphemerisCollection;
	int i;

	for (i = 0; i < ephCollection->count; i++)
	{
		if (ephCollection->satellite[i].SatID != 0)
		{
			D(" sid: %d Received ephemeris for Sat %d exp time [WN:%d==TOE:%d] Status [%d]\n",
			sid,
			ephCollection->satellite[i].SatID,ephCollection->satellite[i].week_no, 
			ephCollection->satellite[i].toe, ephCollection->satellite[i].SatelliteStatus);
		}
	}
	return 0;
}

int CG_deliverAssistData_AcquisAssist(int sid, TCgAgpsAcquisAssist_t *apAcquisAssist)
{
	D("%s sid:%d\n",__FUNCTION__,sid);
	msg_to_libgps(SEND_ACQUIST_ASSIST,0,apAcquisAssist,0);
	return 0;
}

int CG_getPosition(TcgTimeStamp *pTime, GNSSLocationCoordinates_t *pLastPos, GNSSVelocity_t *pVelocity)
{
	D("CG_getPosition\n");
	int ret = 0;
	CgPositionParam_t locationinfo[1];
	memset(locationinfo,0,sizeof(CgPositionParam_t));
	ret = msg_to_libgps(GET_POSITION,(unsigned char *)locationinfo,0,0);
	if(ret == 1) return ret;
	
	memcpy(pTime,&locationinfo->time,sizeof(TcgTimeStamp));
    pVelocity->velType = GNSSVelocityType_horizontalWithVerticalVelocityAndUncertainty;
    pVelocity->choice.VelUncertainty.bearing = (long)locationinfo->velocity.bearing;
    pVelocity->choice.VelUncertainty.horizontalSpeed = (long)locationinfo->velocity.horizontalSpeed;
    pVelocity->choice.VelUncertainty.verticalDirection = locationinfo->velocity.verticalDirect;
    pVelocity->choice.VelUncertainty.verticalSpeed = (long)locationinfo->velocity.verticalSpeed;
    pVelocity->choice.VelUncertainty.horizontalUncertaintySpeed = 0;
    pVelocity->choice.VelUncertainty.verticalUncertaintySpeed = 0;

    transGEOPos(&locationinfo->pos,pLastPos);

	D("[wYear:%d][wMonth:%d][wDay:%d][wHour:%d][wMinute:%d][seconds:%d]\n",
	pTime->wYear,pTime->wMonth,pTime->wDay,pTime->wHour,pTime->wMinute,pTime->wSecond);
	//D("[latitude:%f][longitude:%f][altitude:%f]\n",pLastPos->latitude,pLastPos->longitude,pLastPos->altitude);

	return ret;
}

void CG_suplReadyReceivePosInfo(int sid,char rspType)
{
	D("CG_suplReadyReceivePosInfo\n");
	msg_to_libgps(SEND_SUPL_READY,0,&sid,&rspType);
}

void CG_SuplNIPosMethod(char NIPosMethod)
{
    D("%s called NIPosMethod:%d",__FUNCTION__ ,NIPosMethod);
	return;
}

void gnss_send_eph(void)
{
	int temp_count = 0;
	if( (16 < gnsseph.count) && (gnsseph.count<33) )
	{
		temp_count = gnsseph.count;
	}
	gps_sendData(EPHEMERIS,(const char *)&gnsseph);
	usleep(200000);
	if ((16 < temp_count) && (temp_count < 33))
	{
		D("send two times eph ,temp_count:%d",temp_count);
		gnsseph.count = temp_count -16;
		memcpy((const char *)&gnsseph.satellite,
			(const char *)&	gnsseph.satellite[16],
			sizeof(TCgAgpsEphemeris_t)*gnsseph.count );
		gps_sendData(EPHEMERIS, (const char *)&gnsseph);
		usleep(200000);
	}
}
int CG_deliverAssistData_End(int sid)
{
    GpsState*  s = _gps_state;
	D("CG_deliverAssistData_End sid:%d,will send assist data\n",sid);
	if(s->cmcc == 0)
	{
        msg_to_libgps(SEND_DELIVER_ASSIST_END,0,0,0);
        return 0;
	}

    if(agpsdata->gpsEphValid)
    {
        D("send gps data,num is %d",agpsdata->gpsNum);
        agpsdata->gpsEphValid = 0;
        gnsseph.count = agpsdata->gpsNum;
        agpsdata->gpsNum = 0;
        gnss_send_eph();
    }
    if(agpsdata->glonassEphValid)
    {
		usleep(200000);
        D("send glonass data,num is %d",agpsdata->glonassNum);
        agpsdata->glonassEphValid = 0;
        CG_SendGlonassEph(agpsdata->glonassNum);
        agpsdata->glonassNum = 0;
    }
	return 0;
}

CG_SO_SETAPI void CG_SUPL_NI_END(int sid)
{
    D("%s called sid:%d",__FUNCTION__ ,sid);
    
    msg_to_libgps(SEND_SUPL_END,0,0,0);
    return;
}

int CG_deliverMSAPosition(int sid, TCgAgpsPosition *msaPosition,TcgVelocity  *agpsVelocity)
{
    D("%s called sid:%d",__FUNCTION__ ,sid);
	msg_to_libgps(SEND_MSA_POSITION,0,(void *)msaPosition,(void *)agpsVelocity);
	return 0;
}

CG_SO_SETAPI int CG_getMSISDN(unsigned char* pBuf, unsigned short usLenBuf) 
{
    D("%s: Len:%u",__FUNCTION__,usLenBuf);
	pBuf[0] = 0x31;
	pBuf[1] = 0x77;
	pBuf[2] = 0x24;
	pBuf[3] = 0x23;
	pBuf[4] = 0x31;
	pBuf[5] = 0x03;
	pBuf[6] = 0x00;
	pBuf[7] = 0x00;
	return 1;
}

CG_SO_SETAPI int CG_getIMSI(unsigned char* pBuf, unsigned short usLenBuf) 
{
	unsigned char imsitmp[20] = {0};
	int i = 0,ret = 0;

	ret = msg_to_libgps(IMSI_GET_CMD, imsitmp,0,0);
	
	if(ret == 1)
	return 1;

	D("CG_getIMSI: imsi is %s, LEN: %u",imsitmp,usLenBuf);
	
	imsitmp[15] = '\0';
	for(i=0;i<15;i++) imsitmp[i] = imsitmp[i] - '0';
	
	for(i=0; i<8; i++)
	{
		if(i == 7)
		{
			pBuf[i] = (0xf<<4) + imsitmp[2*i];
		}
		else
		{
			pBuf[i] = (imsitmp[2*i+1]<<4) + imsitmp[2*i];
		}
		//D("%d",pBuf[i]);
	}
	
	return 0;
}

CG_SO_SETAPI int CG_getLocationId(CgLocationId_t *pLocationId) 
{
	int ret = 0;
	D("CG_getLocationId enter");

	ret = msg_to_libgps(CELL_LOCATION_GET_CMD, (unsigned char *)pLocationId,0,0);
	if(ret == 1)
	return ret;
	
	D("present is %d",pLocationId->cellInfo.present);

	return 0;
}

CG_SO_SETAPI unsigned char CG_showNotifVerif(CG_e_NotificationType aNotificationType)
{
	unsigned char ret = 1;

    D("%s  enter notifType: %d",__FUNCTION__,aNotificationType);
	msg_to_libgps(VERIF_GET_CMD,&ret,0,0);
	return ret;
}

CG_SO_SETAPI unsigned long CG_getGsmCellInformation(CgGsmCellInformation_t * pGsmCellInfo) 
{
    D("%s refMCC:%ld refMNC:%ld",__FUNCTION__,pGsmCellInfo->refMCC,pGsmCellInfo->refMNC);
	return 0;
}

CG_SO_SETAPI unsigned long CG_getCdmaCellInformation(CgCdmaCellInformation_t * pCdmaCellInfo) 
{
    D("%s  enter refSID: %ld",__FUNCTION__,pCdmaCellInfo->refSID);
	return 0;
}

CG_SO_SETAPI unsigned long CG_getWcdmaCellInformation(CgWcdmaCellInformation_t * pWcdmaCellInfo) 
{
    D("%s  enter refMCC: %ld refMNC: %ld ",__FUNCTION__,pWcdmaCellInfo->refMCC,pWcdmaCellInfo->refMNC);
	return 0;
}

CG_SO_SETAPI unsigned long CG_getPresentCellInfo(CgCellInfo_PR * apCellInfo) 
{
    D("%s  enter CellInfo: %d ",__FUNCTION__,*apCellInfo);
	return 0;
}

CG_SO_SETAPI CG_SETId_PR CG_getSETID_Type(void)
{
	unsigned char ret = 1;

	D("CG_getSETID_Type enter\n");
	msg_to_libgps(DATA_GET_CMD,&ret,0,0);
	return ret;
}

CG_SO_SETAPI int CG_registerWAPandSMS(AgnssNiNotification_t *notification)
{
	D("CG_registerWAPandSMS\n");
	msg_to_libgps(SMS_GET_CMD,(unsigned char *)notification,0,0);
	return 0;
}

CG_SO_SETAPI unsigned long CG_registerWAPandSMS_cp(AgnssNiNotification_t *notification)
{
	D("CG_registerWAPandSMS_cp\n");
	msg_to_libgps(SMS_CP_CMD,(unsigned char *)notification,0,0);
	return 0;
}
/*get Historical Report from engine*/
int CG_readyRecvHistoricReport(int sid, CgHistoricReporting_t *historicReporting, TCgSetQoP_t *qop)
{
    D("%s, sid=%d,history report type:%d,qopflag:%d\n",__FUNCTION__,\
      sid,historicReporting->allowedRptType,qop->flag);
	return 0;
}

/* Deliver Location result of si location request of another set */
/* If msaPosition/agpsVelocity is NULL, information is not present*/
int CG_deliverLocOfAnotherSet(int sid, TCgAgpsPosition *msaPosition, TcgVelocity  *agpsVelocity)
{
    D("%s,sid:%d,latitude:%f,longitude:%f,altitude:%f,verticalSpeed:%f,verticalDirect:%d",\
        __FUNCTION__,sid,msaPosition->latitude,msaPosition->longitude,msaPosition->altitude,\
        agpsVelocity->verticalSpeed,agpsVelocity->verticalDirect);
	return 0;
}

#define WIFI_PATH "/data/gnss/supl/wifi.xml"
#define WIFI_GET_CMD 19

typedef struct test_mac{
    unsigned char   apMACAddress[6];
    signed short    apChannelFrequency; 
    signed short    apTransmitPower; 
}test_mac_t;

test_mac_t wifi_info[AGNSS_MAX_LID_SIZE];
static int jval = 0;
AgnssWlanCellList_t  *pwlan;

static int str2int( char *p)
{
    unsigned char i = 0, c = 0;
    int result = 0;
    int len = 0;
    char pos = 0;
    char *t = p;
    //E("t[0]=%c,t[1]=%c,t[2]=%c\n",*t,*(t+1),*(t+2));
    while((*t <= '9') && (*t >= '0'))
    {
        len++;
        t++;
        if(*t > '9' || *t < '0')
        break;
    }
    
    //E("len is %d,p[0]=%c,p[1]=%c\n",len,p[0],p[1]);
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
void set_mac_trans( char *srcmac, unsigned char * destmac)
{
	int i = 0, j = 0; 
	for(i = 0; i < 17; i++)
	{ 
		if(j > 5) break;
		
		if(srcmac[i] == ':')
		continue;
		
		if((srcmac[i] >= 'a') && (srcmac[i] <= 'f'))
		srcmac[i] = 10 + (srcmac[i] - 'a');
		
		if((srcmac[i+1] >= 'a') && (srcmac[i+1] <= 'f'))
		srcmac[i+1] = 10 + (srcmac[i+1] - 'a');

		if((srcmac[i] >= '0') && (srcmac[i] <= '9'))
		srcmac[i] = srcmac[i] - '0';
		
		if((srcmac[i+1] >= '0') && (srcmac[i+1] <= '9'))
		srcmac[i+1] = srcmac[i+1] - '0';
		
		destmac[j] = srcmac[i]*16 + srcmac[i+1];

		i++;
		j++;
	}
}

static int get_wifi_info(void)
{
	char ret = 0;
	FILE *fp;
    static char buff[256];
    int i = 0;
    int len = sizeof(buff)-1;
    
    jval = 0;
    memset(wifi_info,0,sizeof(wifi_info));
	fp = fopen(WIFI_PATH,"rb+");
	if(fp == NULL)
	{
		D("open wifi xml fail,error: %s\n",strerror(errno));
		return -1;
	}
    while(fgets(buff,sizeof(buff),fp) != NULL)
    {
        if(jval >= AGNSS_MAX_LID_SIZE) break;
        if(strstr(buff,":") == NULL) continue;
        //D("read line is %s\n",buff);
        set_mac_trans( buff, wifi_info[jval].apMACAddress);
        //D("mac[0]=%d,mac[1]=%d\n",wifi_info[jval].apMACAddress[0],wifi_info[jval].apMACAddress[1]);
        for(i = 17; i < len; i++)
        {
            if((wifi_info[jval].apChannelFrequency == 0) && (((buff[i] == 9) && 
            (buff[i+1] != 9)) || ((buff[i] == ' ') && (buff[i+1] != ' '))))
            {
                wifi_info[jval].apChannelFrequency = str2int(buff+i+1);
                i = i + 4;
                D("freq=%d\n",wifi_info[jval].apChannelFrequency); 
                continue;
            }
            if((wifi_info[jval].apChannelFrequency != 0) && (buff[i] == '-'))
            {           
                //E("src is %c\n",buff[i]);
                wifi_info[jval].apTransmitPower = 0-str2int(buff+i+1);
                D("power=%d\n",wifi_info[jval].apTransmitPower); 
                break;
            }
        }
        memset(buff,0,sizeof(buff));
        jval++;
    }
    fclose(fp);
    return 0;
}

void set_wlanapinfo(void)
{
     int i;
	 
     if(pwlan == NULL)
        return;
     pwlan->nbCellCount = jval - 1;
     pwlan->curCellInfo.apTransmitPower = wifi_info[0].apTransmitPower;
     pwlan->curCellInfo.apChannelFrequency = wifi_info[0].apChannelFrequency;
     memcpy(pwlan->curCellInfo.apMACAddress,wifi_info[0].apMACAddress,6);
     D("before , jval is %d",jval);
     for(i = 0;i < jval-1; i++)
     {
            pwlan->nbCellInfo[i].apTransmitPower = wifi_info[i+1].apTransmitPower;
            pwlan->nbCellInfo[i].apChannelFrequency = wifi_info[i+1].apChannelFrequency;
            memcpy(pwlan->nbCellInfo[i].apMACAddress,wifi_info[i+1].apMACAddress,6);  
     }
     for(i = 0; i < jval-1; i++)
     {
            D("freq:%d,power:%d\n",pwlan->nbCellInfo[i].apChannelFrequency,pwlan->nbCellInfo[i].apTransmitPower);        
            D("mac is:%x-%x-%x-%x-%x-%x\n",pwlan->nbCellInfo[i].apMACAddress[0],pwlan->nbCellInfo[i].apMACAddress[1],
						pwlan->nbCellInfo[i].apMACAddress[2],pwlan->nbCellInfo[i].apMACAddress[3],
						pwlan->nbCellInfo[i].apMACAddress[4],pwlan->nbCellInfo[i].apMACAddress[5]);    
      }
}
/*==================================wifi cmd=========================
    //struct stat lenth;
	//lenth.st_size = 0;
	//if(access(WIFI_PATH,0) != -1)
	//	stat(WIFI_PATH, &lenth);
	//else
	//	return 1;

	//len = (int)lenth.st_size;
	//D("lenth is %d\n",len);
============================================================================*/
char  parse_wifi_xml(void)
{
	char ret = 1;
    GpsState*  s = _gps_state;
    
	D("begin parse wifi xml");
   
    gnss_send2Daemon(s,GNSS_CONTROL_GET_WIFI_TYPE,NULL,0);
    if(WaitEvent(&(s->gpsdsem),1000) != 0)
    {
        E("wait wifi event fail");
        return ret;
    }
	get_wifi_info();
	set_wlanapinfo();

	return 0;
}

int CG_getWlanApInfo(AgnssWlanCellList_t *pCgWlanCellList)
{
	unsigned char ret = 1;
	int i;

    if(agpsdata->wifiStatus == 0)
    {
        E("wifi is close");
        return ret;
    }
	D("CG_getWlanApInfo enter\n");
	pwlan = pCgWlanCellList;
	memset(pCgWlanCellList,0xff,sizeof(AgnssWlanCellList_t));
	pCgWlanCellList->curCellInfo.apDeviceType = AGNSS_WLAN802_UNKNOWN;
	pCgWlanCellList->curCellInfo.apRoundTripDelayPresent = 0;
	pCgWlanCellList->curCellInfo.apReportedLocationPresent = 0;
	pCgWlanCellList->curCellInfo.flag = 0x43;
	for (i=0; i < AGNSS_MAX_LID_SIZE; i++)
	{
		pCgWlanCellList->nbCellInfo[i].apDeviceType = AGNSS_WLAN802_UNKNOWN;
		pCgWlanCellList->nbCellInfo[i].apRoundTripDelayPresent = 0;
		pCgWlanCellList->nbCellInfo[i].apReportedLocationPresent = 0;
		pCgWlanCellList->nbCellInfo[i].flag = 0x43;
	}

	//msg_to_libgps(WIFI_GET_CMD,&ret,0,0); 
	ret = parse_wifi_xml();
	if(ret == 1)
	{
	    E("get wifi info fail");
	}
	return ret;
}

int CG_getIPAddress(CgIPAddress_t *ipAddress)
{ 
	int ret = 0;
	D("CG_getIPAddress enter");
	//ret = msg_to_libgps(IP_GET_CMD,(unsigned char *)ipAddress,0,0); 
	ipAddress->type = CgIPAddress_V4;
	ipAddress->choice.ipv4Address[0] = 192;
	ipAddress->choice.ipv4Address[1] = 168;
	ipAddress->choice.ipv4Address[2] = 1;
	ipAddress->choice.ipv4Address[3] = 123;
	
	return ret;
}

int Get_Home_Plmn(int* mcc,int* mnc,int* mnc_digit)
{
	int ret = 0;
	if((mcc == NULL) || (mnc == NULL) || (mnc_digit == NULL))
	{
		D("home plmn ptr is null,fail");
		return -1;
	}
	msg_to_libgps(SIM_PLMN_CMD,(unsigned char *)mcc,(void *)mnc,(void *)mnc_digit); 
	D("read mcc is %d,mnc is %d",*mcc,*mnc);
	if(*mnc_digit < 2)
	ret = -1;
	
	return ret;
}

void CG_showPositionInfo(int sid)
{
    D("%s sid:%d",__FUNCTION__,sid);
	return;
}

/* AGPS deliver GEO fence related parameters */
void CG_deliverAreaEventParam(int sid, TcgAreaEventParams_t *pParam, TcgAreaIdLists_t *pLists)
{
	D("%s sid:%d,sizeof TcgAreaEventParams_t =%d,type=%d,isposEst=%d",\
        __FUNCTION__,sid,(int)sizeof(TcgAreaEventParams_t),pParam->type,pParam->isPosEst);
	D("deliverAreaEventParam,radis=%ld,interval=%lu,lat=%f,long=%f;Areacount:%lu",
	    pParam->geoTargetArea.areaArray[0].choice.circular.radius,pParam->intervalTime,
	    pParam->geoTargetArea.areaArray[0].choice.circular.coordinate.latitude,
	    pParam->geoTargetArea.areaArray[0].choice.circular.coordinate.longitude,
	    pLists->count);

	//agpsdata->geo_sid = sid;
	if(agpsdata->ni_status == 1)
	{
		D("==>>>ni area param set");
		geofence_task_ni(pParam);
	}	
	return;
}

BOOL CG_getPskForTls(TcgGbaOctetStr_t *pKs,         //OUT: Ks of the specific IMSI
	             TcgGbaOctetStr_t *pBtid,       //OUT: B-TID of the specific IMSI
	             TcgGbaOctetStr_t *pRand,       //OUT:
	             TcgGbaOctetStr_t *pImpi)
{
    if((NULL == pKs)||(NULL== pBtid)||(NULL == pRand)||(NULL == pImpi))
    {
        return 1;
    }
    
	return 0;
}

int CG_getNeighborLocationIdsInfo(AgnssNeighborLocationIdsList_t *NeighborLocationIdsList)
{
    
    D("%s enter, point:%p",__FUNCTION__,NeighborLocationIdsList);
    return 0;
}


int CG_getHplmnInfo(CgHplmnInfo_t *pHplmnInfo)
{
	D("CG_getHplmnInfo enter");
	char plmn[128];
	unsigned int i = 0,j;
	memset(plmn,0,sizeof(plmn));
	if(property_get("gsm.sim.operator.numeric",plmn,NULL) > 0)
	{
	    D("get hplmn :%s",plmn);
	    if(strlen(plmn) < 4)
	    {
	        D("get hplmn fail");
	        return 1;
	    }
	    for(i = 0; i < strlen(plmn); i++)
	    {
	        if(plmn[i] >= '0' && plmn[i] <= '9')
	            break;
	    }
	    D("first digit is %d",i);
	    for(j = 0; j < strlen(plmn)-i; j++)
	    {
	        plmn[j] = plmn[i+j];
	    }
	    pHplmnInfo->refMCC = (plmn[0]-'0')*100+(plmn[1]-'0')*10+plmn[2]-'0';
	    pHplmnInfo->refMNC = 0;
	    pHplmnInfo->refMNCDigits = strlen(plmn) - 3 - i;
        D("plmn len is %zu,mnc lenth is %d",strlen(plmn)-i,pHplmnInfo->refMNCDigits);
        if(pHplmnInfo->refMNCDigits == 2)
        {
            pHplmnInfo->refMNC = plmn[4]-'0' + (plmn[3]-'0')*10;
        }
        else if(pHplmnInfo->refMNCDigits == 3)
        {
            pHplmnInfo->refMNC = plmn[5]-'0' + (plmn[4]-'0')*10 + (plmn[3]-'0')*100;
        }
	}
	else
	{
	    D("get hplmn fail");
	    return 1;
	}		

	D("home mcc is %ld,mnc is %ld",pHplmnInfo->refMCC,pHplmnInfo->refMNC);
	if(pHplmnInfo->refMCC == 0) return 1;
	
	return 0;
}


   
// pEph point the aglonass eph; pTimePose : point the server time pos
static int GlonassEphIsValid(GLONASS_EPHEMERIS* pEph,AGNSSTIMEPOS* pTimePose)
{
    LLH llh_pos;
    KINEMATIC_INFO ecef_pos;
    KINEMATIC_INFO eph_pos;
    DOUBLE n;
    DOUBLE dx,dy,dz,p;
    DOUBLE Distance,R,S;
    DOUBLE SinE1;
    FLOAT el;
    int ret = 0;

    llh_pos.lat = pTimePose->RxLocation.latitude*PI/180;
    llh_pos.lon = pTimePose->RxLocation.longitude*PI/180;
    llh_pos.hae = pTimePose->RxLocation.altitude;
    n = WGS_AXIS_A/sqrt(1.0 -WGS_E1_SQR*sin(llh_pos.lat)*sin(llh_pos.lat));
    ecef_pos.x = (n + llh_pos.hae)*cos(llh_pos.lat)*cos(llh_pos.lon);
    ecef_pos.y = (n + llh_pos.hae)*cos(llh_pos.lat)*sin(llh_pos.lon);
    ecef_pos.z = (n*(1.0 - WGS_E1_SQR)+ llh_pos.hae)*sin(llh_pos.lat);

    //eph pos   
    dx = ecef_pos.x - pEph->x;
    dy = ecef_pos.y - pEph->y;
    dz = ecef_pos.z - pEph->z;
    p = pEph->x*pEph->x + pEph->y*pEph->y;
    Distance = sqrt(dx*dx + dy*dy + dz*dz);
    R = sqrt(p + ecef_pos.z *ecef_pos.z );
    S = ecef_pos.x*dx + (FLOAT)ecef_pos.y*dy;
    SinE1 = (-S - (FLOAT)ecef_pos.z*dz)/R/Distance;
    if(R < 1e-10 || ABS(SinE1) > 1.0)
    {
        el = (FLOAT)(PI/2);
        D("GlonassEphIsValid el1 = %f",el);
    }
    else
    {
        el = asin(SinE1);
        D("GlonassEphIsValid el2 = %f",el);
    }

    if(el > 0)
    {
        ret = 1;
    } 

   return ret;     
}

void CG_SendGlonassEph(int count)
{
    int ephLen = sizeof(GLONASS_EPHEMERIS);
    int i,j = 0;
    int packcount = 0;
    int ret = 0;
    GpsState*  s = _gps_state;
    unsigned int agpsmode = 1;    
    for(i = 0; i< count; i++)
    {
        if(!glonass_eph[i].flag)
        {
            D("glonass_eph count: %d",i);
            break;
        }
        
        if(j%7 == 0)
        {
            memset(&Pak,0,sizeof(TCmdData_t));  
            Pak.type = GNSS_LIBGPS_FLASH_TYPE;
            Pak.subType = GNSS_LIBGPS_FLASH_GLO_EPH1_SUBTYPE + packcount;
        }
        if(s->cmcc == 0)
        {
            ret = GlonassEphIsValid(&glonass_eph[i],&timepos_0);
        }
        else
        {
            ret = 1;
        }
        if(ret)
        {
            memcpy((Pak.buff+4+ephLen*j),&glonass_eph[i],ephLen);
            Pak.length += ephLen;
            j++;
            
            if(j%7 == 0)
            {
                D("GE2 send GLONASS EPH packet:%d, type = %d,subType = %d",
                packcount,Pak.type, Pak.subType);
				Pak.length = Pak.length+4;
				memcpy(Pak.buff,&agpsmode,4);  //for glonass agpsmode
                gps_adingData(&Pak); 
                usleep(200000);
                packcount++;
                Pak.length = 0;                        
                j = 0;
            }
        }
    }

    if(Pak.length > 0) //the last packet 
    {
        Pak.length = 7*ephLen+4;
		memcpy(Pak.buff,&agpsmode,4);
        gps_adingData(&Pak); 
        usleep(200000);
        D("GE2 send GLONASS packet:%d,subType = %d ,ephcount =%d",packcount,Pak.subType,j);
    }
    
}

void transBdsEph(PBDS_EPHEMERIS pBdsEph,GNSSNavModelBDSECEF_t *pBdsGnssEph)
{
    D("transbdseph is enter");
	pBdsEph->bdsURAI = pBdsGnssEph->bdsURAI;
	pBdsEph->bdsToe = pBdsGnssEph->bdsToe;  
	pBdsEph->bdsAPowerHalf = pBdsGnssEph->bdsAPowerHalf; 
	pBdsEph->bdsE = pBdsGnssEph->bdsE; 
	pBdsEph->bdsW = pBdsGnssEph->bdsW; 
	pBdsEph->bdsDeltaN = pBdsGnssEph->bdsDeltaN; 
	pBdsEph->bdsM0 = pBdsGnssEph->bdsM0;  
	pBdsEph->bdsOmega0 = pBdsGnssEph->bdsOmega0; 
	pBdsEph->bdsOmegaDot = pBdsGnssEph->bdsOmegaDot;
	pBdsEph->bdsI0 = pBdsGnssEph->bdsI0;  
	pBdsEph->bdsIDot = pBdsGnssEph->bdsIDot; 
	pBdsEph->bdsCuc = pBdsGnssEph->bdsCuc; 
	pBdsEph->bdsCus = pBdsGnssEph->bdsCus; 
	pBdsEph->bdsCrc = pBdsGnssEph->bdsCrc;
	pBdsEph->bdsCrs = pBdsGnssEph->bdsCrs;
	pBdsEph->bdsCic = pBdsGnssEph->bdsCic;
	pBdsEph->bdsCis = pBdsGnssEph->bdsCis;
}
void transBdsClock(PBDS_CLOCK_EPHEMERIS pBdsClock,GNSSBDSClockModel_t *pBdsGnssClock)
{
    D("transBdsClock is enter");
    pBdsClock->bdsToc = pBdsGnssClock->bdsToc;
    pBdsClock->bdsA0 = pBdsGnssClock->bdsA0;
	pBdsClock->bdsA1 = pBdsGnssClock->bdsA1;
	pBdsClock->bdsA2 = pBdsGnssClock->bdsA2;
	pBdsClock->bdsTgd1 = pBdsGnssClock->bdsTgd1; 
}
int BeidouAssData(GNSSGenericAssData_t *pbds)
{
	GpsState*  s = _gps_state;
    GNSSNavModelSat_t *pNav;
    static GPS_EPHEMERIS_PACKED BdsPacked;
    static BDS_EPHEMERIS BdsEph;
    static BDS_CLOCK_EPHEMERIS BdsClockEph;
    int i = 0;
    memset(&Pak,0,sizeof(TCmdData_t)); 
    D("beidou num %d",pbds->navModel->satNum);

    if(pbds->navModel->satNum > 14)// the firmware max 14  number  
    {  
        D("beidou num rejust the num 14");
        pbds->navModel->satNum = 14;
    }

	unsigned int agpsmode = 1;
	Pak.length = 4;
	memcpy(Pak.buff,&agpsmode,sizeof(int));
    for(i = 0; i < pbds->navModel->satNum; i++)
    {    
        pNav = &pbds->navModel->navModelSat[i];
        D("beidou svid is %d",pNav->svID);
        memset(&BdsPacked,0,sizeof(GPS_EPHEMERIS_PACKED));
        memset(&BdsEph,0,sizeof(BDS_EPHEMERIS));
        memset(&BdsClockEph,0,sizeof(BDS_CLOCK_EPHEMERIS));
        transBdsEph(&BdsEph,&pNav->orbitModel.choice.bdsECEF);
        transBdsClock(&BdsClockEph,&pNav->clockModel.choice.bdsClockModel);
        
        if(pNav->svID > 5)
        {
            EncodeBD2EphemerisD1(&BdsEph, &BdsClockEph, &BdsPacked);
        }
        else
        {
            EncodeBD2EphemerisD2(&BdsEph, &BdsClockEph, &BdsPacked);
        }

		if(s->use_nokiaee == 1)
		{
	        BdsPacked.flag = 3;  // 11    bit1 means aiding data form nokia ee
		}
		else
		{
	        BdsPacked.flag = 1;  // 01
		}
        BdsPacked.svid = pNav->svID;
        memcpy(Pak.buff+ 4 + i*sizeof(GPS_EPHEMERIS_PACKED),&BdsPacked,sizeof(GPS_EPHEMERIS_PACKED));
        Pak.length = Pak.length + sizeof(GPS_EPHEMERIS_PACKED);
        #if 0
        D("beidou:svid=%d,clockModel present=%d,orbitpresent=%d",
                pNav->svID,pNav->clockModel.present,pNav->orbitModel.present);   
        D("bdsToc=%ld,bdsA0=%ld,bdsA1=%ld,bdsA2=%ld,bdsTgd1=%ld",
        pNav->clockModel.choice.bdsClockModel.bdsToc,pNav->clockModel.choice.bdsClockModel.bdsA0,
        pNav->clockModel.choice.bdsClockModel.bdsA1,pNav->clockModel.choice.bdsClockModel.bdsA2,
        pNav->clockModel.choice.bdsClockModel.bdsTgd1);
        D("bdsURAI=%ld,bdsToe=%ld,bdsAPowerHalf=%ld,bdsE=%ld,bdsW=%ld,bdsDeltaN=%ld,bdsM0=%ld,bdsOmega0=%ld",
        pNav->orbitModel.choice.bdsECEF.bdsURAI,pNav->orbitModel.choice.bdsECEF.bdsToe,
        pNav->orbitModel.choice.bdsECEF.bdsAPowerHalf,pNav->orbitModel.choice.bdsECEF.bdsE,
        pNav->orbitModel.choice.bdsECEF.bdsW,pNav->orbitModel.choice.bdsECEF.bdsDeltaN,
        pNav->orbitModel.choice.bdsECEF.bdsM0,pNav->orbitModel.choice.bdsECEF.bdsOmega0);
        D("bdsOmegaDot=%ld,bdsI0=%ld,bdsIDot=%ld,bdsCuc=%ld,bdsCus=%ld,bdsCrc=%ld,bdsCrs=%ld,bdsCic=%ld,bdsCis=%ld,",
        pNav->orbitModel.choice.bdsECEF.bdsOmegaDot,pNav->orbitModel.choice.bdsECEF.bdsI0,
        pNav->orbitModel.choice.bdsECEF.bdsIDot,pNav->orbitModel.choice.bdsECEF.bdsCuc,
        pNav->orbitModel.choice.bdsECEF.bdsCus,pNav->orbitModel.choice.bdsECEF.bdsCrc,
        pNav->orbitModel.choice.bdsECEF.bdsCrs,pNav->orbitModel.choice.bdsECEF.bdsCic,
        pNav->orbitModel.choice.bdsECEF.bdsCis);
        #endif
    }   

    D("send bds data,lenth is %d",Pak.length);
    Pak.type = GNSS_LIBGPS_AIDING_DATA_TYPE;
    Pak.subType = GNSS_LIBGPS_AIDING_BDSEPHEMERIS_SUBTYPE;    
    gps_adingData(&Pak);
    return 0;
}

void send_cmcc_postime(void)
{
    WLLH_TYPE pakTime;
    if((timepos_0.timeflag == 1) && (timepos_0.locflag  == 1))
    {
        gps_sendData(TIME,(const char *)&timepos_0.RxTime);
        gps_sendData(LOCATION,(const char *)&timepos_0.RxLocation);
        timepos_0.timeflag = 0;
        timepos_0.locflag  = 0;
    }
    else if((timepos_0.timeflag == 1) && (timepos_0.locflag  == 0))  
    {
        memset(&pakTime,0,sizeof(WLLH_TYPE));
        memset(&Pak,0,sizeof(TCmdData_t));
        pakTime.weekNo = timepos_0.RxTime.weekNo;
        pakTime.second = timepos_0.RxTime.second;
        pakTime.secondFrac = timepos_0.RxTime.secondFrac;
        Pak.length = sizeof(WLLH_TYPE);
        Pak.type = GNSS_LIBGPS_AIDING_DATA_TYPE;
        Pak.subType = GNSS_LIBGPS_AIDING_POSVELTIME_SUBTYPE;
        D("SEND GE2:time:week=%d,second=%d,secondfrac=%d",pakTime.weekNo,pakTime.second,pakTime.secondFrac);
        memcpy(Pak.buff,&pakTime,sizeof(WLLH_TYPE));
        gps_adingData(&Pak);   //send time to firmware
        timepos_0.timeflag = 0;
    }
    else if((timepos_0.timeflag == 0) && (timepos_0.locflag  == 1)) 
    {
        memset(&pakTime,0,sizeof(WLLH_TYPE));
        memset(&Pak,0,sizeof(TCmdData_t));
        pakTime.lat = timepos_0.RxLocation.latitude;
        pakTime.lon = timepos_0.RxLocation.longitude;
        pakTime.hae = timepos_0.RxLocation.altitude;
        Pak.length = sizeof(WLLH_TYPE);
        Pak.type = GNSS_LIBGPS_AIDING_DATA_TYPE;
        Pak.subType = GNSS_LIBGPS_AIDING_POSVELTIME_SUBTYPE;
        D("SEND GE2:location:lat=%f,lon=%f",pakTime.lat,pakTime.lon);
        memcpy(Pak.buff,&pakTime,sizeof(WLLH_TYPE));
        gps_adingData(&Pak);   //send time to firmware
        timepos_0.locflag  = 0;
    }  
}

int CG_deliverGNSSAssData(int sid, GNSSAssistData_t *pAssData)
{
    int i = 0,gcount = 0,ephnum = 0;
    GNSSNavModelSat_t *pNav;
    GpsState*  s = _gps_state;
    GNSSGenericAssData_t *gpsAssist,*glonassAssist,*pbds;
    char rfchnl[24] = {1,-4,5,6,1,-4,5,6,-6,-7,0,-1,-2,-7,0,-1,4,-3,3,2,4,-3,3,2};
    
   // extern int gps_adingData(TCmdData_t *pPak);
    D("agps data received");
    gpsAssist = NULL;
    glonassAssist = NULL;
    pbds = NULL;
    
    if(pAssData == NULL)
    {
        E("pAssData is null");
        return 1;
    }
    //send_agps_cmd(GNSS_ASSIST_CMD);
    D("%s : sid:%dagps data 1",__FUNCTION__,sid);
    if(pAssData->commonAssData != NULL)
    {
        D("commonAssData is not null");
        if(pAssData->commonAssData->referenceTime != NULL)
        {
            D("agps time received,time present is %d",pAssData->commonAssData->referenceTime->flag);

            timepos_0.timeflag  = 1;
            if((unsigned int)(pAssData->commonAssData->referenceTime->flag&0x01) == 1)
            {
                D("time model is tow,uncertain is %lf",pAssData->commonAssData->referenceTime->tow.uncertainty);
                timepos_0.RxTime.weekNo = pAssData->commonAssData->referenceTime->tow.weekNo;
                timepos_0.RxTime.second = pAssData->commonAssData->referenceTime->tow.timeofweek;
                timepos_0.RxTime.secondFrac = pAssData->commonAssData->referenceTime->tow.towFrac;  
				if(pAssData->commonAssData->referenceTime->tow.timeID == GNSSTimeID_BDS)
				{
					D("bds time is received");
					timepos_0.RxTime.weekNo = timepos_0.RxTime.weekNo+1356;
					timepos_0.RxTime.second = timepos_0.RxTime.second+14;
				}
            }  
            else if((unsigned int)(pAssData->commonAssData->referenceTime->flag&0x02) == 2)
            {
                D("time model is tod,uncertain is %lf",pAssData->commonAssData->referenceTime->tod.uncertainty);
                timepos_0.RxTime.weekNo = pAssData->commonAssData->referenceTime->tod.dayNumber/7;
                timepos_0.RxTime.second = (pAssData->commonAssData->referenceTime->tod.dayNumber % 7)*86400+
                pAssData->commonAssData->referenceTime->tod.timeofDay;
                timepos_0.RxTime.secondFrac = pAssData->commonAssData->referenceTime->tod.todFrac;
				if(pAssData->commonAssData->referenceTime->tod.timeID == GNSSTimeID_BDS)
				{
					D("bds time is received");
					timepos_0.RxTime.weekNo = timepos_0.RxTime.weekNo+1356;
					timepos_0.RxTime.second = timepos_0.RxTime.second+14;
				}
            } 
            D("weekno=%u,second=%ud",timepos_0.RxTime.weekNo,(unsigned int)timepos_0.RxTime.second);
        }
        if((pAssData->commonAssData->referenceLoc != NULL) && 
		(pAssData->commonAssData->referenceLoc->coordType == GNSSLocationCoordinatesType_ellipsoidPointWithAltitudeAndUncertaintyEllipsoid))
        {
			D("begin parse location uncertainty value");
    		GNSSEllipsoidPointWithAltitudeAndUncertaintyEllipsoid_t *pEll;
			double temploc;
			double  uncertaintySemiMajor;
			double  uncertaintySemiMinor;
			double  orientationMajorAxis;
			double  uncertaintyAltitude;
			double  confidence;

			pEll = &(pAssData->commonAssData->referenceLoc->choice.ellPointAltitudeEllips);

            temploc = (double)pEll->degreesLatitude;
			temploc = (temploc/0x800000)*90;
            timepos_0.locflag  = 1;
			if(pEll->latitudeSign == GNSSLatitudeSign_south)
			{
            	timepos_0.RxLocation.latitude = -temploc;
			}
			else
			{
            	timepos_0.RxLocation.latitude = temploc;
			}	
			temploc = (double)pEll->degreesLongitude;
			temploc = (temploc/0x1000000)*360;
            timepos_0.RxLocation.longitude = temploc;
            timepos_0.RxLocation.altitude = pEll->altitude;
			uncertaintySemiMajor = pEll->uncertaintySemiMajor;
			uncertaintySemiMajor = 10*(pow(1.1,uncertaintySemiMajor)-1);
			uncertaintySemiMinor = pEll->uncertaintySemiMinor;
			uncertaintySemiMinor = 10*(pow(1.1,uncertaintySemiMinor)-1);
			orientationMajorAxis = pEll->orientationMajorAxis;
			//orientationMajorAxis = 45*(pow(1.025,orientationMajorAxis)-1);
			uncertaintyAltitude = pEll->uncertaintyAltitude;
			uncertaintyAltitude = 45*(pow(1.025,uncertaintyAltitude)-1);
			confidence = pEll->confidence;

			//D("agps location received,latitude is %f,longtitude is %f",timepos_0.RxLocation.latitude,timepos_0.RxLocation.longitude);
			D("agps location received");
			D("uncertaintySemiMajor:%lf,uncertaintySemiMinor:%lf,orientationMajorAxis:%lf,uncertaintyAltitude:%lf,confidence:%lf",
uncertaintySemiMajor,uncertaintySemiMinor,orientationMajorAxis,uncertaintyAltitude,confidence);

        }
		else
		{
			D("received location is null or type is error");
		}
    }
    if(timepos_0.RxTime.weekNo > 6000)
    {
        timepos_0.locflag = 0;
    }
    D("agps data 2");
    if((timepos_0.timeflag == 1) && (timepos_0.locflag  == 1) && (s->cmcc == 0))
    {
        s->lastreftime = time(NULL);
        if(AGPS_SESSION_START == s->agps_session || agpsdata->ni_status == 1 || 0 == s->agps_session)
        {
            gps_sendData(TIME,(const char *)&timepos_0.RxTime);
            gps_sendData(LOCATION,(const char *)&timepos_0.RxLocation);
            timepos_0.timeflag = 0;
            timepos_0.locflag  = 0;
        }
    }  
    else
    {
        E("network not send time and location together!!");
    }
    if(s->cmcc == 1)
    {
        send_cmcc_postime();
    }
    if(pAssData->genericAssDataList == NULL)
    {
        E("genericAssDataList is null");
        return 1;
    }

    if(pAssData->genericAssDataList->genericAssData[0].gnssID == SUPL_GNSS_ID_GPS )
    {
        D("gps set 0");
        gpsAssist = &pAssData->genericAssDataList->genericAssData[0];
    }
    else if(pAssData->genericAssDataList->genericAssData[0].gnssID == SUPL_GNSS_ID_GLONASS)
    {
        D("glonass set 0");
        glonassAssist = &pAssData->genericAssDataList->genericAssData[0];
    }
    else if(pAssData->genericAssDataList->genericAssData[0].gnssID == SUPL_GNSS_ID_BDS)
    {
        D("beidou set 0");
        pbds = &pAssData->genericAssDataList->genericAssData[0];
    }

    // 2
    if(pAssData->genericAssDataList->genericAssData[1].gnssID == SUPL_GNSS_ID_GPS )
    {
        D("gps set 1");
        gpsAssist = &pAssData->genericAssDataList->genericAssData[1];
    }
    else if(pAssData->genericAssDataList->genericAssData[1].gnssID == SUPL_GNSS_ID_GLONASS)
    {
        D("glonass set 1");
        glonassAssist = &pAssData->genericAssDataList->genericAssData[1];
    }
    else if(pAssData->genericAssDataList->genericAssData[1].gnssID == SUPL_GNSS_ID_BDS)
    {
        D("beidou set 1");
        pbds = &pAssData->genericAssDataList->genericAssData[1];
    }

	// 3
    if(pAssData->genericAssDataList->genericAssData[2].gnssID == SUPL_GNSS_ID_GPS )
    {
        D("gps set 2");
        gpsAssist = &pAssData->genericAssDataList->genericAssData[2];
    }
    else if(pAssData->genericAssDataList->genericAssData[2].gnssID == SUPL_GNSS_ID_GLONASS)
    {
        D("glonass set 2");
        glonassAssist = &pAssData->genericAssDataList->genericAssData[2];
    }
    else if(pAssData->genericAssDataList->genericAssData[2].gnssID == SUPL_GNSS_ID_BDS)
    {
        D("beidou set 2");
        pbds = &pAssData->genericAssDataList->genericAssData[2];
    }

    D("firmware workmode is %d",s->workmode);
    if((pbds != NULL) && (pbds->navModel != NULL) && (s->workmode == GPS_BDS))
    {
        D("begin send beidou assist data");
        BeidouAssData(pbds);
    }
    if((pbds != NULL) && (pbds->navModel == NULL))
	{
		E("bds is not null,but navmodel is null");
	}
    //memset(glonass_eph,0,sizeof(glonass_eph));
    if((glonassAssist == NULL) || (pAssData->genericAssDataList == NULL) || (glonassAssist->navModel == NULL))
    {
        E("glonass assist data is null");
    }
    else if(s->workmode == GPS_GLONASS)
    {   
        gcount = glonassAssist->navModel->satNum;
        D("glonass svid num is %d",gcount);
        //tk,n,P
        ephnum = agpsdata->glonassNum;
        for(i = 0; i < gcount; i++)
        {
            pNav = &glonassAssist->navModel->navModelSat[i];
            D("glonass:svid=%d,present=%d",pNav->svID,pNav->orbitModel.present);
			if(s->use_nokiaee == 1)
			{
				glonass_eph[i+ephnum].flag = 3;  // 11  bit1 means aiding data form nokia ee
			}
			else
			{
	            glonass_eph[i+ephnum].flag = 1;  // 01
			}
            
            if(pNav->svID < 25)
            {
                D("glonass only,not include auxiliaryInfo data");
                glonass_eph[i+ephnum].freq = rfchnl[pNav->svID-1];
                
            }
            else
            {
                E("error:glonass svid is over 25");
            }
            
            //glonass_eph[i].freq = tempauxi->auxInfo[i].gloChannelnumber;
            D("glonass freq=%d",glonass_eph[i+ephnum].freq);
            glonass_eph[i+ephnum].M = pNav->orbitModel.choice.glonassECEF.gloM;
            glonass_eph[i+ephnum].Ft = pNav->svHealth.choice.gloHealth.ft;
            glonass_eph[i+ephnum].n  = pNav->svID;
            glonass_eph[i+ephnum].Bn = pNav->svHealth.choice.gloHealth.bn;
            glonass_eph[i+ephnum].En = pNav->orbitModel.choice.glonassECEF.gloEn;
            glonass_eph[i+ephnum].tb = pNav->iod.choice.gloIOD.tb*60;
            //D("m=%d,ft=%d,bn=%d,en=%d,tb=%d",glonass_eph[i].M,glonass_eph[i].Ft,glonass_eph[i].Bn,glonass_eph[i].En,glonass_eph[i].tb);
            glonass_eph[i+ephnum].gamma = (double)pNav->clockModel.choice.glonassClockModel.gloGamma/(double)((long long)1<<40);
            //D("gamma is %f",glonass_eph[i].gamma);
            glonass_eph[i+ephnum].tn = (double)pNav->clockModel.choice.glonassClockModel.gloTau/(double)(1<<30);
            glonass_eph[i+ephnum].dtn = (double)pNav->clockModel.choice.glonassClockModel.gloDeltaTau/(double)(1<<30);
            glonass_eph[i+ephnum].x = (double)(pNav->orbitModel.choice.glonassECEF.gloX)/(double)(1<<11);
            glonass_eph[i+ephnum].x = glonass_eph[i+ephnum].x*1000.0;
            //D("glox %ld,x value is %f",pNav->orbitModel.choice.glonassECEF.gloX,glonass_eph[i].x);
            glonass_eph[i+ephnum].y = (double)(pNav->orbitModel.choice.glonassECEF.gloY)/(double)(1<<11);
            glonass_eph[i+ephnum].y = glonass_eph[i+ephnum].y*1000.0;
            //D("gloy %ld,y value is %f",pNav->orbitModel.choice.glonassECEF.gloY,glonass_eph[i].y);
            glonass_eph[i+ephnum].z = (double)(pNav->orbitModel.choice.glonassECEF.gloZ)/(double)(1<<11);
            glonass_eph[i+ephnum].z = glonass_eph[i+ephnum].z*1000.0;
            //D("gloz %ld,z value is %f",pNav->orbitModel.choice.glonassECEF.gloZ,glonass_eph[i].z);
            glonass_eph[i+ephnum].vx = (double)(pNav->orbitModel.choice.glonassECEF.gloXdot)/(double)(1<<20);
            glonass_eph[i+ephnum].vx = glonass_eph[i+ephnum].vx * 1000.0;
            glonass_eph[i+ephnum].vy = (double)(pNav->orbitModel.choice.glonassECEF.gloYdot)/(double)(1<<20);
            glonass_eph[i+ephnum].vy = glonass_eph[i+ephnum].vy * 1000.0;
            glonass_eph[i+ephnum].vz = (double)(pNav->orbitModel.choice.glonassECEF.gloZdot)/(double)(1<<20);
            glonass_eph[i+ephnum].vz = glonass_eph[i+ephnum].vz * 1000.0;
            glonass_eph[i+ephnum].ax = (double)(pNav->orbitModel.choice.glonassECEF.gloXdotdot*1000)/(double)(1<<30);
            glonass_eph[i+ephnum].ay = (double)(pNav->orbitModel.choice.glonassECEF.gloYdotdot*1000)/(double)(1<<30);
            glonass_eph[i+ephnum].az = (double)(pNav->orbitModel.choice.glonassECEF.gloZdotdot*1000)/(double)(1<<30);
            
        }
        if(s->cmcc == 0)
        {
            CG_SendGlonassEph(gcount);
        }
        else
        {
            agpsdata->glonassNum = agpsdata->glonassNum + gcount;
            agpsdata->glonassEphValid = 1;
        }
        
    }
    
    if((gpsAssist != NULL) && (pAssData->genericAssDataList != NULL) && (gpsAssist->navModel != NULL))
    {
        //memset(&gnsseph,0,sizeof(TCgAgpsEphemerisCollection_t));
        D("genericAssDataList gnssnum=%d,satnum=%d",pAssData->genericAssDataList->gnssNum,gpsAssist->navModel->satNum);
        gnsseph.count = gpsAssist->navModel->satNum;
        ephnum = agpsdata->gpsNum;
        for(i = 0; i < gnsseph.count; i++)
        {
            pNav = &gpsAssist->navModel->navModelSat[i];
            D("SVID:%d,clockModel present:%d,orbitpresent:%d",pNav->svID,pNav->clockModel.present,pNav->orbitModel.present);
            gnsseph.satellite[i+ephnum].SatID = pNav->svID;
            gnsseph.satellite[i+ephnum].SatelliteStatus = 0;    //test
            gnsseph.satellite[i+ephnum].week_no = timepos_0.RxTime.weekNo;
            gnsseph.satellite[i+ephnum].IODE = pNav->iod.choice.gpsIOD.iodc;
            gnsseph.satellite[i+ephnum].t_oc = pNav->clockModel.choice.navClockModel.navToc;
            gnsseph.satellite[i+ephnum].af0 = pNav->clockModel.choice.navClockModel.navaf0;
            gnsseph.satellite[i+ephnum].af1 = pNav->clockModel.choice.navClockModel.navaf1;
            gnsseph.satellite[i+ephnum].af2 = pNav->clockModel.choice.navClockModel.navaf2;
            gnsseph.satellite[i+ephnum].tgd = pNav->clockModel.choice.navClockModel.navTgd;
            gnsseph.satellite[i+ephnum].C_rs = pNav->orbitModel.choice.navKeplerianSet.navCrs;
            gnsseph.satellite[i+ephnum].C_is = pNav->orbitModel.choice.navKeplerianSet.navCis;
            gnsseph.satellite[i+ephnum].C_us = pNav->orbitModel.choice.navKeplerianSet.navCus;
            gnsseph.satellite[i+ephnum].C_rc = pNav->orbitModel.choice.navKeplerianSet.navCrc;
            gnsseph.satellite[i+ephnum].C_ic = pNav->orbitModel.choice.navKeplerianSet.navCic;
            gnsseph.satellite[i+ephnum].C_uc = pNav->orbitModel.choice.navKeplerianSet.navCuc;
            gnsseph.satellite[i+ephnum].OMEGA_0 = pNav->orbitModel.choice.navKeplerianSet.navOmegaA0;
            gnsseph.satellite[i+ephnum].I_0 = pNav->orbitModel.choice.navKeplerianSet.navI0;
            gnsseph.satellite[i+ephnum].SquareA = pNav->orbitModel.choice.navKeplerianSet.navAPowerHalf;
            gnsseph.satellite[i+ephnum].Idot = pNav->orbitModel.choice.navKeplerianSet.navIDot;
            gnsseph.satellite[i+ephnum].e = pNav->orbitModel.choice.navKeplerianSet.navE;
            gnsseph.satellite[i+ephnum].OMEGA_dot = pNav->orbitModel.choice.navKeplerianSet.navOmegaADot;
            gnsseph.satellite[i+ephnum].M_0 = pNav->orbitModel.choice.navKeplerianSet.navM0;
            gnsseph.satellite[i+ephnum].Del_n = pNav->orbitModel.choice.navKeplerianSet.navDeltaN;
            gnsseph.satellite[i+ephnum].omega = pNav->orbitModel.choice.navKeplerianSet.navOmega;
            gnsseph.satellite[i+ephnum].toe = pNav->orbitModel.choice.navKeplerianSet.navToe;
            //gnsseph.satellite[i]. = pNav->orbitModel.choice.navKeplerianSet.navFitFlag;
            //gnsseph.satellite[i]. = pNav->orbitModel.choice.navKeplerianSet.navURA;
        }
        
        if(s->cmcc)
        {
            agpsdata->gpsEphValid = 1;
            agpsdata->gpsNum = agpsdata->gpsNum + gnsseph.count;
            //gps_sendData(EPHEMERIS,(const char *)&gnsseph);  
        }
        else
        {
            s->lastrefeph = time(NULL);
            if(AGPS_SESSION_START == s->agps_session || agpsdata->ni_status == 1 || 0 == s->agps_session)
            {
                gnss_send_eph();
            }
        }

    }
	s->use_nokiaee = 0;
    return 0;
}

int CG_deliverAssDataIonoModel(int sid, GNSSIonosphericModel_t *pIonoModel)
{
    if(pIonoModel == NULL)
    {
        D("pIonoModel is null,return");
        return 1;
    }
    D("%s,sid:%d,iono model enter:kflag = %d, nflag = %d",__FUNCTION__,sid,pIonoModel->klobucharFlag,pIonoModel->neQuickFlag);
    D("a0=%ld,a1=%ld,a2=%ld,a3=%ld,b0=%ld,b1=%ld,b2=%ld,b3=%ld",
                                                pIonoModel->kloModel.alfa0,pIonoModel->kloModel.alfa1,
                                                pIonoModel->kloModel.alfa2,pIonoModel->kloModel.alfa3,
                                                pIonoModel->kloModel.beta0,pIonoModel->kloModel.beta1,
                                                pIonoModel->kloModel.beta2,pIonoModel->kloModel.beta3);
    iono.a0 = (float)((double)pIonoModel->kloModel.alfa0/(double)(1<<30));
    iono.a1 = (float)((double)pIonoModel->kloModel.alfa1/(double)(1<<27));
    iono.a2 = (float)((double)pIonoModel->kloModel.alfa2/(double)(1<<24));
    iono.a3 = (float)((double)pIonoModel->kloModel.alfa3/(double)(1<<24));
    iono.b0 = (float)(pIonoModel->kloModel.beta0*2048);
    iono.b1 = (float)(pIonoModel->kloModel.beta1*16384);
    iono.b2 = (float)(pIonoModel->kloModel.beta2*65536);
    iono.b3 = (float)(pIonoModel->kloModel.beta3*65536);
    iono.flag = 1;
    memset(&Pak,0,sizeof(TCmdData_t));	
	Pak.type = GNSS_LIBGPS_AIDING_DATA_TYPE;
	Pak.subType = GNSS_LIBGPS_AIDING_IONOSPHERE_SUBTYPE;
	Pak.length = sizeof(IONO_PARAM);
    memcpy(Pak.buff,&iono,Pak.length);
	gps_adingData(&Pak); 
    return 0;
}
/*================================ send assist end ============================*/

int CG_gnssReadyToRecvPosInfo(int sid, char rspType, GNSSPosReq_t  *pPosReq)
{
    D("%s,sid:%d,rspType:%d,posreq:%d",__FUNCTION__,sid,rspType,pPosReq->gnssPosTech);
    agpsdata->gnssidReq = pPosReq->gnssPosTech;
    msg_to_libgps(SEND_SUPL_READY,0,&sid,&rspType);
    return 0;
}

int CG_deliverLPPeAssData(int sid, LPPeProvAssData_t *pAssData)
{
    D("%s sid:%d,comProvFlag:%d;wlanProvFlag:%d",__FUNCTION__,sid,\
        pAssData->comProvFlag,pAssData->wlanProvFlag);
    return 0;
}

int CG_deliverAssDataAlmanac(int sid, GNSSAlmanacData_t *pAlmanac)
{
    GPSAlmanacRRLP_t *pgpsalm = NULL;
    int i = 0;
    static BLOCK_GPS_ALMANAC_SEND alm_send;
    alm_send.agps_mode = 1;
    if(pAlmanac == NULL)
    {
        E("pAlmanac is null,must return");
        return 1;
    }
    D("deliverAssDataAlmanac enter,sid = %d,present is %d",sid,pAlmanac->present);
    pgpsalm = &pAlmanac->choice.rrlpAlmanac.gpsAlmanac;
    D("num is %ld",pgpsalm->almanacElementNum);
    for(i = 0; i < pgpsalm->almanacElementNum; i++)
    {
    #if 1
        D("satelliteID=%ld,almanacE=%ld,almanacToa=%ld,almanacKsii=%ld,almanacOmegaDot=%ld,almanacSVhealth=%ld",
        pgpsalm->almanacList[i].satelliteID,pgpsalm->almanacList[i].almanacE,
        pgpsalm->almanacList[i].almanacToa,pgpsalm->almanacList[i].almanacKsii,
        pgpsalm->almanacList[i].almanacOmegaDot,pgpsalm->almanacList[i].almanacSVhealth);
        D("almanacAPowerHalf=%ld,almanacOmega0=%ld,almanacW=%ld,almanacM0=%ld,almanacAF0=%ld,almanacAF1=%ld",
        pgpsalm->almanacList[i].almanacAPowerHalf,pgpsalm->almanacList[i].almanacOmega0,
        pgpsalm->almanacList[i].almanacW,pgpsalm->almanacList[i].almanacM0,
        pgpsalm->almanacList[i].almanacAF0,pgpsalm->almanacList[i].almanacAF1);
    #endif
        EncodeGpsAlmanac(&pgpsalm->almanacList[i],&alm_send.gpsalm.GpsAlmPacked);
    }
    memset(&Pak,0,sizeof(TCmdData_t));	
	Pak.type = GNSS_LIBGPS_AIDING_DATA_TYPE;
	Pak.subType = GNSS_LIBGPS_AIDING_GPSALMANAC_SUBTYPE;
	Pak.length = sizeof(BLOCK_GPS_ALMANAC_SEND);
    memcpy(Pak.buff,&alm_send,Pak.length);
	gps_adingData(&Pak); 
    return 0;
}

#ifndef GNSS_LCS 
AgpsUpCallbacks_t sAgpsCallbacks = {
    sizeof(AgpsUpCallbacks_t),
    CG_closePhoneItf,
    CG_registerWAPandSMS,
    /* AGPS get SET ID */
    CG_getSETID_Type,
    CG_getIMSI,
    CG_getMSISDN,
    CG_getIPAddress,
    
    /* AGPS get wireless network information */
    CG_getLocationId,
    CG_getWlanApInfo,

    /* AGPS get GEO position of the SET */
    CG_getPosition,

    /* AGPS get TLS-PSK related keys */
    CG_getPskForTls,
    /* AGPS deliver assistance data */
    CG_deliverAssistData_RefTime,
    CG_deliverAssistData_RefLoc,
    CG_deliverAssistData_NavMdl,
    CG_deliverAssistData_AcquisAssist,
    CG_deliverLPPeAssData,
    CG_deliverAssistData_End,
    /* AGPS deliver position received from network */
    CG_deliverMSAPosition,
    CG_deliverLocOfAnotherSet,
    /* AGPS deliver GEO fence related parameters */

    CG_deliverAreaEventParam,
    /* AGPS send several notifications */
    CG_SUPL_NI_END,
    CG_suplReadyReceivePosInfo,
    CG_readyRecvHistoricReport,
    CG_showPositionInfo,
    CG_getNeighborLocationIdsInfo,
    CG_getHplmnInfo,   

    CG_deliverGNSSAssData,
    CG_gnssReadyToRecvPosInfo,
    CG_deliverAssDataIonoModel,
    CG_deliverAssDataAlmanac,
};
void set_up_callbacks(void)
{
	agpsapi_setAgpsUpCallbacks(&sAgpsCallbacks);
}
#endif //GNSS_LCS 
/*=========================================================================
************************controlplane begin*********************************
==========================================================================*/

CG_SO_SETAPI int CG_cpclosePhoneItf(void) 
{
	return 0;
}

BOOL CG_cpgetPskForTls(TcgGbaOctetStr_t *pKs,         //OUT: Ks of the specific IMSI
	             TcgGbaOctetStr_t *pBtid,       //OUT: B-TID of the specific IMSI
	             TcgGbaOctetStr_t *pRand,       //OUT:
	             TcgGbaOctetStr_t *pImpi)
{
    if((NULL == pKs)||(NULL== pBtid)||(NULL == pRand)||(NULL== pImpi))
    {
        E("%s,point is null ",__FUNCTION__);
    }

    return 0;
}
int CG_deliverAssistData_cpRefTime(int sid, TCgRxNTime *apTime)
{
    D("%s,sid:%d,systime:%ud",__FUNCTION__,sid,(unsigned int)apTime->sysTime);
	msg_to_libgps(SEND_TIME,0,(void *)apTime,0);
	return 0;
}

void CG_cpshowPositionInfo(int sid)
{
    D("%s,sid:%d",__FUNCTION__,sid);
	return;
}

int CG_deliverAssistData_cpRefLoc(int sid, void * apPos, TCgRxNTime * apTime)
{
    D("%s,sid:%d,apPos:%p,systime:%ud",__FUNCTION__,sid,apPos,(unsigned int)apTime->sysTime);
	msg_to_libgps(SEND_LOCTION,0,apPos,0);
	return 0;
}

void CG_cpdeliverAreaEventParam(int sid, TcgAreaEventParams_t *pParam, TcgAreaIdLists_t *pLists)
{
    D("this maybe never enter");
    D("%s,sid:%d,type:%d,area count:%ld",__FUNCTION__,sid,pParam->type,pLists->count);
	if(agpsdata->ni_status == 1)
	{
		D("==>>>ni area param set");
		geofence_task_ni(pParam);
	}	
	return;
}
int CG_deliverAssistData_cpNavMdl(int sid, TCgAgpsEphemerisCollection_t* apEphemerisCollection)
{
    int i;
    
    D("%s,sid:%d,ephemeris count:%d",__FUNCTION__,sid,apEphemerisCollection->count);
	msg_to_libgps(SEND_MDL,0,apEphemerisCollection,0);
	
	return 0;
}

int CG_deliverAssistData_cpAcquisAssist(int sid, TCgAgpsAcquisAssist_t *apAcquisAssist)
{
	D("%s sid:%d  eph count:%ud\n",__FUNCTION__,sid,apAcquisAssist->count);
	msg_to_libgps(SEND_ACQUIST_ASSIST,0,apAcquisAssist,0);
	return 0;
}

int CG_cpgetPosition(TcgTimeStamp *pTime, GNSSLocationCoordinates_t *pLastPos, GNSSVelocity_t *pVelocity)
{
	D("CG_cpgetPosition\n");
	int ret = 0;
	CgPositionParam_t locationinfo[1];
	memset(locationinfo,0,sizeof(CgPositionParam_t));

	if(fix_flag == 0)
	{
		ret = 1;
		D("Location is null ,so agps can't get position'\n");
		return ret;
	}
	else
	{
		ret = 0;
		D("Agps can get position'\n");
	}
	memcpy(pTime,&gLocationInfo->time,sizeof(TcgTimeStamp));
	pVelocity->velType = GNSSVelocityType_horizontalWithVerticalVelocityAndUncertainty;
	pVelocity->choice.VelUncertainty.bearing = (long)gLocationInfo->velocity.bearing;
	pVelocity->choice.VelUncertainty.horizontalSpeed = (long)gLocationInfo->velocity.horizontalSpeed;
	pVelocity->choice.VelUncertainty.verticalDirection = gLocationInfo->velocity.verticalDirect;
	pVelocity->choice.VelUncertainty.verticalSpeed = (long)gLocationInfo->velocity.verticalSpeed;
	pVelocity->choice.VelUncertainty.horizontalUncertaintySpeed = 0;
	pVelocity->choice.VelUncertainty.verticalUncertaintySpeed = 0;
	transGEOPos(&gLocationInfo->pos,pLastPos);

	D("[wYear:%d][wMonth:%d][wDay:%d][wHour:%d][wMinute:%d][seconds:%d]\n",
	pTime->wYear,pTime->wMonth,pTime->wDay,pTime->wHour,pTime->wMinute,pTime->wSecond);
	//D("[latitude:%f][longitude:%f][altitude:%f]\n",pLastPos->latitude,pLastPos->longitude,pLastPos->altitude);

	return ret;
}

void CG_cpsuplReadyReceivePosInfo(int sid,char rspType)
{
	D("CG_cpsuplReadyReceivePosInfo\n");
	msg_to_libgps(SEND_SUPL_READY,0,&sid,&rspType);
}

void CG_cpSuplNIPosMethod(char NIPosMethod)
{
	D("%s NIPosMethod:%d  \n",__FUNCTION__,NIPosMethod);
	return;
}

int CG_cpdeliverAssistData_End(int sid)
{
    GpsState*  s = _gps_state;
	D("CG_deliverAssistData_End sid:%d,will send assist data\n",sid);
	if(s->cmcc == 0)
	{
        msg_to_libgps(SEND_DELIVER_ASSIST_END,0,0,0);
        return 0;
	}

    if(agpsdata->gpsEphValid)
    {
        D("send gps data,num is %d",agpsdata->gpsNum);
        agpsdata->gpsEphValid = 0;
        gnsseph.count = agpsdata->gpsNum;
        agpsdata->gpsNum = 0;
        gnss_send_eph();
    }
    if(agpsdata->glonassEphValid)
    {
		usleep(200000);
        D("send glonass data,num is %d",agpsdata->glonassNum);
        agpsdata->glonassEphValid = 0;
        CG_SendGlonassEph(agpsdata->glonassNum);
        agpsdata->glonassNum = 0;
    }
	return 0;
}

CG_SO_SETAPI void CG_CP_END(int sid)
{
	D("%s, sid:%d\n",__FUNCTION__,sid);

    msg_to_libgps(SEND_SUPL_END,0,0,0);
    return;
}

int CG_cpdeliverMSAPosition(int sid, TCgAgpsPosition *msaPosition,TcgVelocity  *agpsVelocity)
{
	D("%s, sid:%d\n",__FUNCTION__,sid);
	msg_to_libgps(SEND_MSA_POSITION,0,(void *)msaPosition,(void *)agpsVelocity);
	return 0;
}

CG_SO_SETAPI int CG_cpgetMSISDN(unsigned char* pBuf, unsigned short usLenBuf) 
{
    D("%s, len:%d\n",__FUNCTION__,usLenBuf);

	pBuf[0] = 0x31;
	pBuf[1] = 0x77;
	pBuf[2] = 0x24;
	pBuf[3] = 0x23;
	pBuf[4] = 0x31;
	pBuf[5] = 0x03;
	pBuf[6] = 0x00;
	pBuf[7] = 0x00;
	return 1;
}

CG_SO_SETAPI int CG_cpgetIMSI(unsigned char* pBuf, unsigned short usLenBuf) 
{
	unsigned char imsitmp[20] = {0};
	int i = 0,ret = 0;

	ret = msg_to_libgps(IMSI_GET_CMD, imsitmp,0,0);
	
	if(ret == 1)
	return 1;

	D("CG_cpgetIMSI: imsi is %s,Len:%ud",imsitmp,usLenBuf);
	
	imsitmp[15] = '\0';
	for(i=0;i<15;i++) imsitmp[i] = imsitmp[i] - '0';
	
	for(i=0; i<8; i++)
	{
		if(i == 7)
		{
			pBuf[i] = (0xf<<4) + imsitmp[2*i];
		}
		else
		{
			pBuf[i] = (imsitmp[2*i+1]<<4) + imsitmp[2*i];
		}
		//D("%d",pBuf[i]);
	}
	
	return 0;
}

CG_SO_SETAPI int CG_cpgetLocationId(CgLocationId_t *pLocationId) 
{
	int ret = 0;
    
	D("CG_cpgetLocationId enter,status:%ld",pLocationId->status);
	ret = msg_to_libgps(CELL_LOCATION_GET_CMD, (unsigned char *)pLocationId,0,0);
	if(ret == 1)
	return ret;
	
	D("present is %d",pLocationId->cellInfo.present);

	return 0;
}

CG_SO_SETAPI unsigned char CG_cpshowNotifVerif(CG_e_NotificationType aNotificationType)
{
	unsigned char ret = 1;

    D("%s enter,NotificationType:%d",__FUNCTION__,aNotificationType);
	msg_to_libgps(VERIF_GET_CMD,&ret,0,0);
	return ret;
}

CG_SO_SETAPI unsigned long CG_cpgetGsmCellInformation(CgGsmCellInformation_t * pGsmCellInfo) 
{
    if(pGsmCellInfo)
    {
        D("%s enter",__FUNCTION__);
        return 0;
    }
    else
    {
        E("%s enter, the Point is NULL",__FUNCTION__);
        return 0;
    }
}

CG_SO_SETAPI unsigned long CG_cpgetCdmaCellInformation(CgCdmaCellInformation_t * pCdmaCellInfo) 
{
    if(pCdmaCellInfo)
    {
        D("%s enter",__FUNCTION__);
        return 0;
    }
    else
    {
        E("%s enter, the Point is NULL",__FUNCTION__);
        return 0; //t0do
    }
}

CG_SO_SETAPI unsigned long CG_cpgetWcdmaCellInformation(CgWcdmaCellInformation_t * pWcdmaCellInfo) 
{
    if(pWcdmaCellInfo)
    {
        D("%s enter",__FUNCTION__);
        return 0;
    }
    else
    {
        E("%s enter, the Point is NULL",__FUNCTION__);
        return 0; //todo 
    }
}

CG_SO_SETAPI unsigned long CG_cpgetPresentCellInfo(CgCellInfo_PR * apCellInfo) 
{
    if(apCellInfo)
    {
        D("%s enter",__FUNCTION__);
        return 0;
    }
    else
    {
        E("%s enter, the Point is NULL",__FUNCTION__);
        return 0; //todo 
    }
}

CG_SO_SETAPI CG_SETId_PR CG_cpgetSETID_Type(void)
{
	unsigned char ret = 1;

	D("CG_cpgetSETID_Type enter\n");
	msg_to_libgps(DATA_GET_CMD,&ret,0,0);
	return ret;
}

CG_SO_SETAPI int CG_cpregisterWAPandSMS(AgnssNiNotification_t *notification)
{
	D("CG_cpregisterWAPandSMS\n");
	msg_to_libgps(SMS_GET_CMD,(unsigned char *)notification,0,0);
	return 0;
}

CG_SO_SETAPI unsigned long CG_cpregisterWAPandSMS_cp(AgnssNiNotification_t *notification)
{
	D("CG_registerWAPandSMS_cp\n");
	msg_to_libgps(SMS_CP_CMD,(unsigned char *)notification,0,0);
	return 0;
}
/*get Historical Report from engine*/
int CG_cpreadyRecvHistoricReport(int sid, CgHistoricReporting_t *historicReporting, TCgSetQoP_t *qop)
{
    if((NULL==historicReporting)|| (NULL == qop))
    {
        E("%s enter, sid:%d,but point is null",__FUNCTION__,sid);
    }
    else
    {
        D("%s enter, sid:%d",__FUNCTION__,sid);
    }
	return 0;
}

/* Deliver Location result of si location request of another set */
/* If msaPosition/agpsVelocity is NULL, information is not present*/
int CG_cpdeliverLocOfAnotherSet(int sid, TCgAgpsPosition *msaPosition, TcgVelocity  *agpsVelocity)
{
    if((NULL ==msaPosition)||(NULL ==agpsVelocity))
    {
        E("%s enter, sid:%d, point is null ",__FUNCTION__,sid);
    }
	return 0;
}


int CG_cpgetWlanApInfo(AgnssWlanCellList_t *pCgWlanCellList)
{
    if(pCgWlanCellList)
    {
        D("%s enter",__FUNCTION__);
    }
	return 1;
}

int CG_cpgetIPAddress(CgIPAddress_t *ipAddress)
{ 
	int ret = 0;
	D("CG_cpgetIPAddress enter");
	ipAddress->type = CgIPAddress_V4;
	ipAddress->choice.ipv4Address[0] = 192;
	ipAddress->choice.ipv4Address[1] = 168;
	ipAddress->choice.ipv4Address[2] = 1;
	ipAddress->choice.ipv4Address[3] = 123;
	//ret =  msg_to_libgps(IP_GET_CMD,(unsigned char *)ipAddress,0,0); 
	return ret;
}

int Get_Home_cpPlmn(int* mcc,int* mnc,int* mnc_digit)
{
	int ret = 0;
	if((mcc == NULL) || (mnc == NULL) || (mnc_digit == NULL))
	{
		D("home plmn ptr is null,fail");
		return -1;
	}
	msg_to_libgps(SIM_PLMN_CMD,(unsigned char *)mcc,(void *)mnc,(void *)mnc_digit); 
	D("read mcc is %d,mnc is %d",*mcc,*mnc);
	if(*mnc_digit < 2)
	ret = -1;
	
	return ret;
}

#ifndef GNSS_LCS
int CG_cpSendUlMsg(agps_cp_ul_pdu_params_t *pParams)
{
    return libgps_c_plane_stack_network_ulreq(pParams);
}

int CG_cpSendVarificationRsp(unsigned long notifId,unsigned long present, unsigned long verificationResponse)
{
    return libgps_lcsverificationresp(notifId,present,verificationResponse);
}

int CG_cpMoLocationReq(agps_cp_molr_params_t *pParam)
{
    D("CG_cpMoLocationReq enter");
    lcs_molr(pParam);
    return 0;
}

int CG_cpLcsPos(agps_cp_lcs_pos_params_t *pParam)
{
    D("CG_cpLcsPos enter");
    lcs_pos(pParam);
    return 0;
}
#endif //GNSS_LCS

int CG_getMsr(int sid, TcgAgpsMsrSetList_t *pMsrSetList, GNSSVelocity_t *pVelocity)
{
    int ret = 0;

    D("%s enter, sid:%d",__FUNCTION__,sid);
    ret = Get_Msr(pMsrSetList,pVelocity);

    return ret;
}



int CG_cpMolrRsp(int errorNum,TCgAgpsPosition *Position, TcgVelocity  *agpsVelocity)
{
    if((NULL == Position)||(NULL == agpsVelocity))
    {
        E("CG_cpMolrRsp is enter in phone,errornum is %d\n",errorNum);
    }   
    
#ifdef GNSS_LCS
    gnss_lcsContrlEcid(agpsdata->si_sid,LCS_DOMAIN_CP,LCS_NWLR_ECID,0);
	gnss_lcsstopSiSessionReq(agpsdata->si_sid);
#else
     ctrlplane_RXN_stopSiSessionReq(agpsdata->si_sid);
#endif     
    
    return 0;
}
int CG_cpLcsPosRsp(agps_cp_pos_rsp_params_t *pRspParam)
{
    if(pRspParam)
    {
        D("CG_cpLcsPosRsp enter,type:%d",pRspParam->evtType);
    }
#ifdef GNSS_LCS
    gnss_lcsContrlEcid(agpsdata->si_sid,LCS_DOMAIN_CP,LCS_NWLR_ECID,0);
    gnss_lcsstopSiSessionReq(agpsdata->si_sid);
#else
    ctrlplane_RXN_stopSiSessionReq(agpsdata->si_sid);
#endif  //GNSS_LCS
    
    
    return 0;
}

int CG_cpdeliverLPPeAssData(int sid, LPPeProvAssData_t *pAssData)
{
    if(pAssData)
    {
        D("%s enter, sid:%d",__FUNCTION__,sid);
    }
    else
    {
        E("%s enter, sid:%d",__FUNCTION__,sid);
    }
    return 0;
}


int CG_cpdeliverGNSSAssData(int sid, GNSSAssistData_t *pAssData)
{
   int i = 0,gcount = 0,ephnum = 0;
    GNSSNavModelSat_t *pNav;
    GpsState*  s = _gps_state;
    GNSSGenericAssData_t *gpsAssist,*glonassAssist,*pbds;
    char rfchnl[24] = {1,-4,5,6,1,-4,5,6,-6,-7,0,-1,-2,-7,0,-1,4,-3,3,2,4,-3,3,2};
    
   // extern int gps_adingData(TCmdData_t *pPak);
    D("agps data received");
    gpsAssist = NULL;
    glonassAssist = NULL;
    pbds = NULL;
    
    if(pAssData == NULL)
    {
        E("pAssData is null");
        return 1;
    }
    //send_agps_cmd(GNSS_ASSIST_CMD);
    D("%s : sid:%dagps data 1",__FUNCTION__,sid);
    if(pAssData->commonAssData != NULL)
    {
        D("commonAssData is not null");
        if(pAssData->commonAssData->referenceTime != NULL)
        {
            D("agps time received,time present is %d",pAssData->commonAssData->referenceTime->flag);

            timepos_0.timeflag  = 1;
            if((unsigned int)(pAssData->commonAssData->referenceTime->flag&0x01) == 1)
            {
                D("time model is tow,uncertain is %lf",pAssData->commonAssData->referenceTime->tow.uncertainty);
                timepos_0.RxTime.weekNo = pAssData->commonAssData->referenceTime->tow.weekNo;
                timepos_0.RxTime.second = pAssData->commonAssData->referenceTime->tow.timeofweek;
                timepos_0.RxTime.secondFrac = pAssData->commonAssData->referenceTime->tow.towFrac;  
				if(pAssData->commonAssData->referenceTime->tow.timeID == GNSSTimeID_BDS)
				{
					D("bds time is received");
					timepos_0.RxTime.weekNo = timepos_0.RxTime.weekNo+1356;
					timepos_0.RxTime.second = timepos_0.RxTime.second+14;
				}
            }  
            else if((unsigned int)(pAssData->commonAssData->referenceTime->flag&0x02) == 2)
            {
                D("time model is tod,uncertain is %lf",pAssData->commonAssData->referenceTime->tod.uncertainty);
                timepos_0.RxTime.weekNo = pAssData->commonAssData->referenceTime->tod.dayNumber/7;
                timepos_0.RxTime.second = (pAssData->commonAssData->referenceTime->tod.dayNumber % 7)*86400+
                pAssData->commonAssData->referenceTime->tod.timeofDay;
                timepos_0.RxTime.secondFrac = pAssData->commonAssData->referenceTime->tod.todFrac;
				if(pAssData->commonAssData->referenceTime->tod.timeID == GNSSTimeID_BDS)
				{
					D("bds time is received");
					timepos_0.RxTime.weekNo = timepos_0.RxTime.weekNo+1356;
					timepos_0.RxTime.second = timepos_0.RxTime.second+14;
				}
            } 
            D("weekno=%u,second=%ud",timepos_0.RxTime.weekNo,(unsigned int)timepos_0.RxTime.second);
        }
        if((pAssData->commonAssData->referenceLoc != NULL) && 
		(pAssData->commonAssData->referenceLoc->coordType == GNSSLocationCoordinatesType_ellipsoidPointWithAltitudeAndUncertaintyEllipsoid))
        {
			D("begin parse location uncertainty value");
    		GNSSEllipsoidPointWithAltitudeAndUncertaintyEllipsoid_t *pEll;
			double temploc;
			double  uncertaintySemiMajor;
			double  uncertaintySemiMinor;
			double  orientationMajorAxis;
			double  uncertaintyAltitude;
			double  confidence;

			pEll = &(pAssData->commonAssData->referenceLoc->choice.ellPointAltitudeEllips);

            temploc = (double)pEll->degreesLatitude;
			temploc = (temploc/0x800000)*90;
            timepos_0.locflag  = 1;
			if(pEll->latitudeSign == GNSSLatitudeSign_south)
			{
            	timepos_0.RxLocation.latitude = -temploc;
			}
			else
			{
            	timepos_0.RxLocation.latitude = temploc;
			}	
			temploc = (double)pEll->degreesLongitude;
			temploc = (temploc/0x1000000)*360;
            timepos_0.RxLocation.longitude = temploc;
            timepos_0.RxLocation.altitude = pEll->altitude;
			uncertaintySemiMajor = pEll->uncertaintySemiMajor;
			uncertaintySemiMajor = 10*(pow(1.1,uncertaintySemiMajor)-1);
			uncertaintySemiMinor = pEll->uncertaintySemiMinor;
			uncertaintySemiMinor = 10*(pow(1.1,uncertaintySemiMinor)-1);
			orientationMajorAxis = pEll->orientationMajorAxis;
			//orientationMajorAxis = 45*(pow(1.025,orientationMajorAxis)-1);
			uncertaintyAltitude = pEll->uncertaintyAltitude;
			uncertaintyAltitude = 45*(pow(1.025,uncertaintyAltitude)-1);
			confidence = pEll->confidence;

			//D("agps location received,latitude is %f,longtitude is %f",timepos_0.RxLocation.latitude,timepos_0.RxLocation.longitude);
			D("agps location received");	
			D("uncertaintySemiMajor:%lf,uncertaintySemiMinor:%lf,orientationMajorAxis:%lf,uncertaintyAltitude:%lf,confidence:%lf",
uncertaintySemiMajor,uncertaintySemiMinor,orientationMajorAxis,uncertaintyAltitude,confidence);

        }
		else
		{
			D("received location is null or type is error");
		}
    }
    if(timepos_0.RxTime.weekNo > 6000)
    {
        timepos_0.locflag = 0;
    }
    D("agps data 2");
    if((timepos_0.timeflag == 1) && (timepos_0.locflag  == 1) && (s->cmcc == 0))
    {
        s->lastreftime = time(NULL);
        if(AGPS_SESSION_START == s->agps_session || 0 == s->agps_session)
        {
            gps_sendData(TIME,(const char *)&timepos_0.RxTime);
            gps_sendData(LOCATION,(const char *)&timepos_0.RxLocation);
            timepos_0.timeflag = 0;
            timepos_0.locflag  = 0;
        }
    }  
    else
    {
        E("network not send time and location together!!");
    }
    if(s->cmcc == 1)
    {
        send_cmcc_postime();
    }
    if(pAssData->genericAssDataList == NULL)
    {
        E("genericAssDataList is null");
        return 1;
    }

    if(pAssData->genericAssDataList->genericAssData[0].gnssID == SUPL_GNSS_ID_GPS )
    {
        D("gps set 0");
        gpsAssist = &pAssData->genericAssDataList->genericAssData[0];
    }
    else if(pAssData->genericAssDataList->genericAssData[0].gnssID == SUPL_GNSS_ID_GLONASS)
    {
        D("glonass set 0");
        glonassAssist = &pAssData->genericAssDataList->genericAssData[0];
    }
    else if(pAssData->genericAssDataList->genericAssData[0].gnssID == SUPL_GNSS_ID_BDS)
    {
        D("beidou set 0");
        pbds = &pAssData->genericAssDataList->genericAssData[0];
    }

    // 2
    if(pAssData->genericAssDataList->genericAssData[1].gnssID == SUPL_GNSS_ID_GPS )
    {
        D("gps set 1");
        gpsAssist = &pAssData->genericAssDataList->genericAssData[1];
    }
    else if(pAssData->genericAssDataList->genericAssData[1].gnssID == SUPL_GNSS_ID_GLONASS)
    {
        D("glonass set 1");
        glonassAssist = &pAssData->genericAssDataList->genericAssData[1];
    }
    else if(pAssData->genericAssDataList->genericAssData[1].gnssID == SUPL_GNSS_ID_BDS)
    {
        D("beidou set 1");
        pbds = &pAssData->genericAssDataList->genericAssData[1];
    }

	// 3
    if(pAssData->genericAssDataList->genericAssData[2].gnssID == SUPL_GNSS_ID_GPS )
    {
        D("gps set 2");
        gpsAssist = &pAssData->genericAssDataList->genericAssData[2];
    }
    else if(pAssData->genericAssDataList->genericAssData[2].gnssID == SUPL_GNSS_ID_GLONASS)
    {
        D("glonass set 2");
        glonassAssist = &pAssData->genericAssDataList->genericAssData[2];
    }
    else if(pAssData->genericAssDataList->genericAssData[2].gnssID == SUPL_GNSS_ID_BDS)
    {
        D("beidou set 2");
        pbds = &pAssData->genericAssDataList->genericAssData[2];
    }

    D("firmware workmode is %d",s->workmode);  
    if((pbds != NULL) && (pbds->navModel != NULL) && (s->workmode == GPS_BDS))
    {
        D("begin send beidou assist data");
        BeidouAssData(pbds);
    }
    if((pbds != NULL) && (pbds->navModel == NULL))
	{
		E("bds is not null,but navmodel is null");
	}
    
    //memset(glonass_eph,0,sizeof(glonass_eph));
    if((glonassAssist == NULL) || (pAssData->genericAssDataList == NULL) || (glonassAssist->navModel == NULL))
    {
        E("glonass assist data is null");
    }
    else if(s->workmode == GPS_GLONASS)
    {   
        gcount = glonassAssist->navModel->satNum;
        D("glonass svid num is %d",gcount);
        //tk,n,P
        ephnum = agpsdata->glonassNum;
        for(i = 0; i < gcount; i++)
        {
            pNav = &glonassAssist->navModel->navModelSat[i];
            D("glonass:svid=%d,present=%d",pNav->svID,pNav->orbitModel.present);
            glonass_eph[i+ephnum].flag = 1;
            
            if(pNav->svID < 25)
            {
                D("glonass only,not include auxiliaryInfo data");
                glonass_eph[i+ephnum].freq = rfchnl[pNav->svID-1];
                
            }
            else
            {
                E("error:glonass svid is over 25");
            }
            
            //glonass_eph[i].freq = tempauxi->auxInfo[i].gloChannelnumber;
            D("glonass freq=%d",glonass_eph[i+ephnum].freq);
            glonass_eph[i+ephnum].M = pNav->orbitModel.choice.glonassECEF.gloM;
            glonass_eph[i+ephnum].Ft = pNav->svHealth.choice.gloHealth.ft;
            glonass_eph[i+ephnum].n  = pNav->svID;
            glonass_eph[i+ephnum].Bn = pNav->svHealth.choice.gloHealth.bn;
            glonass_eph[i+ephnum].En = pNav->orbitModel.choice.glonassECEF.gloEn;
            glonass_eph[i+ephnum].tb = pNav->iod.choice.gloIOD.tb*60;
            //D("m=%d,ft=%d,bn=%d,en=%d,tb=%d",glonass_eph[i].M,glonass_eph[i].Ft,glonass_eph[i].Bn,glonass_eph[i].En,glonass_eph[i].tb);
            glonass_eph[i+ephnum].gamma = (double)pNav->clockModel.choice.glonassClockModel.gloGamma/(double)((long long)1<<40);
            //D("gamma is %f",glonass_eph[i].gamma);
            glonass_eph[i+ephnum].tn = (double)pNav->clockModel.choice.glonassClockModel.gloTau/(double)(1<<30);
            glonass_eph[i+ephnum].dtn = (double)pNav->clockModel.choice.glonassClockModel.gloDeltaTau/(double)(1<<30);
            glonass_eph[i+ephnum].x = (double)(pNav->orbitModel.choice.glonassECEF.gloX)/(double)(1<<11);
            glonass_eph[i+ephnum].x = glonass_eph[i+ephnum].x*1000.0;
            //D("glox %ld,x value is %f",pNav->orbitModel.choice.glonassECEF.gloX,glonass_eph[i].x);
            glonass_eph[i+ephnum].y = (double)(pNav->orbitModel.choice.glonassECEF.gloY)/(double)(1<<11);
            glonass_eph[i+ephnum].y = glonass_eph[i+ephnum].y*1000.0;
            //D("gloy %ld,y value is %f",pNav->orbitModel.choice.glonassECEF.gloY,glonass_eph[i].y);
            glonass_eph[i+ephnum].z = (double)(pNav->orbitModel.choice.glonassECEF.gloZ)/(double)(1<<11);
            glonass_eph[i+ephnum].z = glonass_eph[i+ephnum].z*1000.0;
            //D("gloz %ld,z value is %f",pNav->orbitModel.choice.glonassECEF.gloZ,glonass_eph[i].z);
            glonass_eph[i+ephnum].vx = (double)(pNav->orbitModel.choice.glonassECEF.gloXdot)/(double)(1<<20);
            glonass_eph[i+ephnum].vx = glonass_eph[i+ephnum].vx * 1000.0;
            glonass_eph[i+ephnum].vy = (double)(pNav->orbitModel.choice.glonassECEF.gloYdot)/(double)(1<<20);
            glonass_eph[i+ephnum].vy = glonass_eph[i+ephnum].vy * 1000.0;
            glonass_eph[i+ephnum].vz = (double)(pNav->orbitModel.choice.glonassECEF.gloZdot)/(double)(1<<20);
            glonass_eph[i+ephnum].vz = glonass_eph[i+ephnum].vz * 1000.0;
            glonass_eph[i+ephnum].ax = (double)(pNav->orbitModel.choice.glonassECEF.gloXdotdot*1000)/(double)(1<<30);
            glonass_eph[i+ephnum].ay = (double)(pNav->orbitModel.choice.glonassECEF.gloYdotdot*1000)/(double)(1<<30);
            glonass_eph[i+ephnum].az = (double)(pNav->orbitModel.choice.glonassECEF.gloZdotdot*1000)/(double)(1<<30);

            
        }
        if(s->cmcc == 0)
        {
            CG_SendGlonassEph(gcount);
        }
        else
        {
            agpsdata->glonassNum = agpsdata->glonassNum + gcount;
            agpsdata->glonassEphValid = 1;
        }
        
    }
    
    if((gpsAssist != NULL) && (pAssData->genericAssDataList != NULL) && (gpsAssist->navModel != NULL))
    {
        //memset(&gnsseph,0,sizeof(TCgAgpsEphemerisCollection_t));
        D("genericAssDataList gnssnum=%d,satnum=%d",pAssData->genericAssDataList->gnssNum,gpsAssist->navModel->satNum);
        gnsseph.count = gpsAssist->navModel->satNum;
        ephnum = agpsdata->gpsNum;
        for(i = 0; i < gnsseph.count; i++)
        {
            pNav = &gpsAssist->navModel->navModelSat[i];
            D("SVID:%d,clockModel present:%d,orbitpresent:%d",pNav->svID,pNav->clockModel.present,pNav->orbitModel.present);
            gnsseph.satellite[i+ephnum].SatID = pNav->svID;
            gnsseph.satellite[i+ephnum].SatelliteStatus = 0;    //test
            gnsseph.satellite[i+ephnum].week_no = timepos_0.RxTime.weekNo;
            gnsseph.satellite[i+ephnum].IODE = pNav->iod.choice.gpsIOD.iodc;
            gnsseph.satellite[i+ephnum].t_oc = pNav->clockModel.choice.navClockModel.navToc;
            gnsseph.satellite[i+ephnum].af0 = pNav->clockModel.choice.navClockModel.navaf0;
            gnsseph.satellite[i+ephnum].af1 = pNav->clockModel.choice.navClockModel.navaf1;
            gnsseph.satellite[i+ephnum].af2 = pNav->clockModel.choice.navClockModel.navaf2;
            gnsseph.satellite[i+ephnum].tgd = pNav->clockModel.choice.navClockModel.navTgd;
            gnsseph.satellite[i+ephnum].C_rs = pNav->orbitModel.choice.navKeplerianSet.navCrs;
            gnsseph.satellite[i+ephnum].C_is = pNav->orbitModel.choice.navKeplerianSet.navCis;
            gnsseph.satellite[i+ephnum].C_us = pNav->orbitModel.choice.navKeplerianSet.navCus;
            gnsseph.satellite[i+ephnum].C_rc = pNav->orbitModel.choice.navKeplerianSet.navCrc;
            gnsseph.satellite[i+ephnum].C_ic = pNav->orbitModel.choice.navKeplerianSet.navCic;
            gnsseph.satellite[i+ephnum].C_uc = pNav->orbitModel.choice.navKeplerianSet.navCuc;
            gnsseph.satellite[i+ephnum].OMEGA_0 = pNav->orbitModel.choice.navKeplerianSet.navOmegaA0;
            gnsseph.satellite[i+ephnum].I_0 = pNav->orbitModel.choice.navKeplerianSet.navI0;
            gnsseph.satellite[i+ephnum].SquareA = pNav->orbitModel.choice.navKeplerianSet.navAPowerHalf;
            gnsseph.satellite[i+ephnum].Idot = pNav->orbitModel.choice.navKeplerianSet.navIDot;
            gnsseph.satellite[i+ephnum].e = pNav->orbitModel.choice.navKeplerianSet.navE;
            gnsseph.satellite[i+ephnum].OMEGA_dot = pNav->orbitModel.choice.navKeplerianSet.navOmegaADot;
            gnsseph.satellite[i+ephnum].M_0 = pNav->orbitModel.choice.navKeplerianSet.navM0;
            gnsseph.satellite[i+ephnum].Del_n = pNav->orbitModel.choice.navKeplerianSet.navDeltaN;
            gnsseph.satellite[i+ephnum].omega = pNav->orbitModel.choice.navKeplerianSet.navOmega;
            gnsseph.satellite[i+ephnum].toe = pNav->orbitModel.choice.navKeplerianSet.navToe;
            //gnsseph.satellite[i]. = pNav->orbitModel.choice.navKeplerianSet.navFitFlag;
            //gnsseph.satellite[i]. = pNav->orbitModel.choice.navKeplerianSet.navURA;
        }
        
        if(s->cmcc)
        {
            agpsdata->gpsEphValid = 1;
            agpsdata->gpsNum = agpsdata->gpsNum + gnsseph.count;
            //gps_sendData(EPHEMERIS,(const char *)&gnsseph);  
        }
        else
        {
            s->lastrefeph = time(NULL);
            if(AGPS_SESSION_START == s->agps_session ||  0 == s->agps_session)
            {
                gnss_send_eph();
            }
        }

    }
    return 0;
}
/*================================ send assist end ============================*/

int CG_cpgnssReadyToRecvPosInfo(int sid, char rspType, GNSSPosReq_t  *pPosReq)
{
    D("%s sid:%d,agps data received,pos Tech:%d",__FUNCTION__,sid,pPosReq->gnssPosTech);
    agpsdata->gnssidReq = pPosReq->gnssPosTech;
    msg_to_libgps(SEND_SUPL_READY,0,&sid,&rspType);
    return 0;
}

int CG_cpdeliverAssDataIonoModel(int sid, GNSSIonosphericModel_t *pIonoModel)
{
    if(pIonoModel)
    {
        D("%s sid:%d,",__FUNCTION__,sid);
    }
    else
    {
        E("%s sid:%d,",__FUNCTION__,sid);
    }
    return 0;
}
int CG_cpgetNeighborLocationIdsInfo(AgnssNeighborLocationIdsList_t *NeighborLocationIdsList)
{
    if(NeighborLocationIdsList)
    {
        D("%s enter",__FUNCTION__);
    }
    else
    {
        E("%s enter",__FUNCTION__);
    }
    return 0;
}
int CG_cpdeliverAssDataAlmanac(int sid, GNSSAlmanacData_t *pAlmanac)
{
    D("%s enter sid:%d, almanac point:%p",__FUNCTION__,sid,pAlmanac);
    return 0;
}
#ifndef GNSS_LCS
AgpsCpCallbacks_t sCpCallbacks = {
    sizeof(AgpsCpCallbacks_t),
    CG_cpclosePhoneItf,
    CG_cpregisterWAPandSMS,
    /* AGPS get SET ID */
    CG_cpgetSETID_Type,
    CG_cpgetIMSI,
    CG_cpgetMSISDN,
    CG_cpgetIPAddress,
    
    /* AGPS get wireless network information */
    CG_cpgetLocationId,
    CG_cpgetWlanApInfo,
    CG_cpgetNeighborLocationIdsInfo,
    /* AGPS get GEO position of the SET */
    CG_cpgetPosition,
    CG_getMsr,
    /* AGPS get TLS-PSK related keys */
    CG_cpgetPskForTls,
    /* AGPS deliver assistance data */
    CG_deliverAssistData_cpRefTime,
    CG_deliverAssistData_cpRefLoc,
    CG_deliverAssistData_cpNavMdl,
    CG_deliverAssistData_cpAcquisAssist,
    CG_cpdeliverLPPeAssData,
    CG_cpdeliverAssistData_End,
    /* AGPS deliver position received from network */
    CG_cpdeliverMSAPosition,
    CG_cpdeliverLocOfAnotherSet,
    /* AGPS deliver GEO fence related parameters */
    CG_cpdeliverAreaEventParam,
    /* AGPS send several notifications */
    CG_CP_END,
    CG_cpsuplReadyReceivePosInfo,
    CG_cpreadyRecvHistoricReport,
    CG_cpshowPositionInfo,
    CG_cpSendUlMsg,
    CG_cpSendVarificationRsp,
    CG_cpMoLocationReq,    
    CG_cpMolrRsp,
    CG_cpLcsPos,  
    CG_cpLcsPosRsp,
    CG_cpdeliverGNSSAssData,
    CG_cpgnssReadyToRecvPosInfo,
    CG_cpdeliverAssDataIonoModel,
    CG_cpdeliverAssDataAlmanac,
};
void set_cp_callbacks(void)
{
    agpsapi_setAgpsCpCallbacks(&sCpCallbacks);
}
#endif //GNSS_LCS
//when aps session timeout ,it send last ref time
//location, eph
void agps_send_lastrefparam(int intervaltime)
{
	AGNSSTIMEPOS timepos_adjust;

	memset(&timepos_adjust, 0, sizeof(AGNSSTIMEPOS));
	memcpy(&timepos_adjust, &timepos_0, sizeof(AGNSSTIMEPOS));

	timepos_adjust.RxTime.second += intervaltime;

	if(timepos_adjust.RxTime.second > (GPS_WEEK_NO_SECONDS -1))
	{
		D("adjust weekno:%d, intervaltime: %d",timepos_adjust.RxTime.weekNo, intervaltime);
		timepos_adjust.RxTime.weekNo += 1;
		timepos_adjust.RxTime.second -= GPS_WEEK_NO_SECONDS;
	}

	gps_sendData(TIME, (const char *)&timepos_adjust.RxTime);
	gps_sendData(LOCATION, (const char *)&timepos_adjust.RxLocation);
	gnss_send_eph();
	CG_SendGlonassEph(AGPS_GLONASS_EPH_MAX_COUNT);
}

