
#ifdef GNSS_LCS 

#include <dlfcn.h>
#include "lcs_mgt.h" 
#include "gps_common.h"


LcsMgtInterface_t* pgLCSHandle = NULL;
extern AgpsData_t agpsdata[1]; 
extern AgnssWlanCellList_t  *pwlan;

static int gnss_lcsrequestSession(LcsGnssInstrHeader_t  *ptHeader)
{
    GpsState *s = _gps_state;

    D("%s domain %d ",__FUNCTION__,ptHeader->domain);     

    if(LCS_DOMAIN_CP == ptHeader->domain)
    {
        agpsdata->cur_state = CP_NI;
		//s->cmcc = 1;
    }
    else
    {
        E("%s ,it should happen CP ,not :%d ",__FUNCTION__,ptHeader->domain);     
    }
    if(CMD_START != s->gps_flag)
    {
        D("cp payload,begin gps_start");
        
	agpsdata->cp_begin = 1;  
	agpsdata->cp_ni_session = 1;
	agpsdata->ni_status = 1;  
	gps_start();
	usleep(300000);
        gnss_lcsContrlEcid(agpsdata->ni_sid,ptHeader->domain,LCS_NWLR_ECID,1);
        
    }
    //start ecid 
    //agpsdata->ni_sid = ptHeader->sid;

    return 0;
}

static int gnss_lcsregisterWAPandSMS(LcsGnssInstrHeader_t  *ptHeader)
{
    int ret = 0;
    
    D("%s domain %d,sid:%d ",__FUNCTION__,ptHeader->domain,ptHeader->sid); 
    if(ptHeader->argsLen !=  GNSS_ARGS_SIZE_NI_NOTIF)
    {
        D("%s input param len error: %d",__FUNCTION__,ptHeader->argsLen); 
        return -1;
    }
	msg_to_libgps(SMS_GET_CMD,(unsigned char *)ptHeader->ptArgs,0,0);
	return 0;
}


static int gnss_lcsgetSetID(LcsGnssInstrHeader_t  *ptHeader)
{
    int ret = 0;
    
    D("%s domain %d,sid:%d ",__FUNCTION__,ptHeader->domain,ptHeader->sid); 
    if(ptHeader->argsLen !=  GNSS_ARGS_SIZE_GET_SETID)
    {
        D("%s input param len error: %d",__FUNCTION__,ptHeader->argsLen); 
        return -1;
    }
	msg_to_libgps(DATA_GET_CMD,ptHeader->ptArgs,0,0);
    D("%s  get setid:%d",__FUNCTION__,(unsigned char)(*ptHeader->ptArgs));
	return 0;
}

static int gnss_lcsgetIMSI(LcsGnssInstrHeader_t  *ptHeader) 
{
	unsigned char imsitmp[20] = {0};
	int i = 0,ret = 0;
    unsigned char* pBuf;
    LcsGnssUnivData_t * pMsg = (LcsGnssUnivData_t *)ptHeader->ptArgs;
    
    D("%s domain %d,sid:%d ",__FUNCTION__,ptHeader->domain,ptHeader->sid); 
    if(ptHeader->argsLen !=  GNSS_ARGS_SIZE_GET_IMSI)
    {
        D("%s input param len error: %d",__FUNCTION__,ptHeader->argsLen); 
        return -1;
    }

	ret = msg_to_libgps(IMSI_GET_CMD, imsitmp,0,0);
	
	if(ret == 1)
	return 1;

	D("gnss_lcsgetIMSI: imsi is %s",imsitmp);
	
	imsitmp[15] = '\0';
	for(i=0;i<15;i++) imsitmp[i] = imsitmp[i] - '0';

    pBuf = pMsg->ptData;
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
	pMsg->dataLen = 8;
	return 0;
}


static int gnss_lcsgetMSISDN(LcsGnssInstrHeader_t  *ptHeader)
{
	int i = 0,ret = 0;
    LcsGnssUnivData_t * pdata = (LcsGnssUnivData_t *)ptHeader->ptArgs;

    D("%s domain %d,sid= %d",__FUNCTION__,ptHeader->domain,ptHeader->sid); 
    if(ptHeader->argsLen != GNSS_ARGS_SIZE_GET_MSISDN)
    {
        D("%s input param len error: %d",__FUNCTION__,ptHeader->argsLen); 
        return -1;
    }

    if(pdata->dataLen > 7)
    {
        *pdata->ptData++ = 0x31;
        *pdata->ptData++ = 0x77;
        *pdata->ptData++ = 0x24;
        *pdata->ptData++ = 0x23;
        *pdata->ptData++ = 0x31;
        *pdata->ptData++ = 0x03;
        *pdata->ptData++ = 0x00;
        *pdata->ptData++ = 0x00;
    }

    return ret; 
    
}

int gnss_lcsgetHplmnInfo(LcsGnssInstrHeader_t  *ptHeader)
{
	char plmn[128];
	int i = 0, j = 0,ret = 0;
    GnssArgsGetHplmn_t *pHplmnInfo;  

    D("%s domain %d,sid= %d",__FUNCTION__,ptHeader->domain,ptHeader->sid); 
    if(ptHeader->argsLen !=  GNSS_ARGS_SIZE_GET_HPLMN)
    {
        D("%s input param len error: %d",__FUNCTION__,ptHeader->argsLen); 
        return -1;
    }
    pHplmnInfo =(GnssArgsGetHplmn_t*)ptHeader->ptArgs;
	memset(plmn,0,sizeof(plmn));
	if(property_get("gsm.sim.operator.numeric",plmn,NULL) > 0)
	{
	    D("get hplmn :%s",plmn);
	    if(strlen(plmn) < 4)
	    {
	        D("get hplmn fail");
	        return 1;
	    }
	    for(i = 0; i < (int)strlen(plmn); i++)
	    {
	        if(plmn[i] >= '0' && plmn[i] <= '9')
	            break;
	    }
	    D("first digit is %d",i);
	    for(j = 0; j < (int)strlen(plmn)-i; j++)
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
	    ret = -1;
	}		

	D("home mcc is %ld,mnc is %ld",pHplmnInfo->refMCC,pHplmnInfo->refMNC);
	
	return ret;
}


int gnss_lcsIPAddress(LcsGnssInstrHeader_t  *ptHeader)
{ 
	int ret = 0;
    GnssArgsGetIpAddr_t *ipAddress = (GnssArgsGetIpAddr_t *)ptHeader->ptArgs;

    D("%s domain %d,sid= %d ",__FUNCTION__,ptHeader->domain,ptHeader->sid); 
    if(ptHeader->argsLen != GNSS_ARGS_SIZE_GET_IPADDR)
    {
        D("%s input param len error: %d",__FUNCTION__,ptHeader->argsLen); 
        return -1;
    }
    
	ipAddress->type = CgIPAddress_V4;
	ipAddress->choice.ipv4Address[0] = 192;
	ipAddress->choice.ipv4Address[1] = 168;
	ipAddress->choice.ipv4Address[2] = 1;
	ipAddress->choice.ipv4Address[3] = 123;
	
	return ret;
}


int gnss_lcsWlanApInfo(LcsGnssInstrHeader_t  *ptHeader)
{
	int i, ret = 0;
    
    GnssArgsGetWlanCell_t *pCgWlanCellList = (GnssArgsGetWlanCell_t*)ptHeader->ptArgs;

    D("%s domain %d,sid= %d",__FUNCTION__,ptHeader->domain,ptHeader->sid); 
	
	return -1;  //add for wifi info get fail

    if(ptHeader->argsLen !=  GNSS_ARGS_SIZE_GET_WLANCELL)
    {
        D("%s input param len error: %d",__FUNCTION__,ptHeader->argsLen); 
        ret = -1;
        return ret;
    }
        
    if(agpsdata->wifiStatus == 0)
    {
        E("wifi is close");
        ret = -1;
        return ret;
    }
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

	ret = parse_wifi_xml();
	if(ret == 1)
	{
	    E("get wifi info fail");
	}
	return ret;
}


int gnss_lcsgetPosition(LcsGnssInstrHeader_t  *ptHeader)
{
	int ret = 0;
    TcgTimeStamp *pTime = NULL;
    GNSSPosition_t *pLastPos = NULL;
    GNSSVelocity_t *pVelocity = NULL;
    GnssArgsGetPos_t *pLocation = (GnssArgsGetPos_t*)ptHeader->ptArgs;
    GpsState*  s = _gps_state;
    
    D("%s domain %d,sid= %d",__FUNCTION__,ptHeader->domain,ptHeader->sid); 
    if(ptHeader->argsLen !=  GNSS_ARGS_SIZE_GET_POS)
    {
        D("%s input param len error: %d",__FUNCTION__,ptHeader->argsLen); 
        ret = -1;
        return ret;
    }
    if(0 == s->cmcc)
    {
        D("%s run real network",__FUNCTION__);
        return -1;
    }
    if(pLocation)
    {
        pTime = pLocation->ptTime;
        pLastPos = pLocation->ptPosition;
        pVelocity = pLocation->ptVelocity;
    }

    if((NULL != pLocation) && (NULL != pTime)&&
       (NULL != pLastPos) && (NULL != pVelocity))
    {
        ret = msg_to_libgps(GET_POSITION,(unsigned char *)pLastPos,0,0);
        if(ret == 1) 
        {
            ret = -1;
            return ret;
        }
        
        memcpy(pTime,&gLocationInfo->time,sizeof(TcgTimeStamp));
        pVelocity->velType = GNSSVelocityType_horizontalWithVerticalVelocityAndUncertainty;
        pVelocity->choice.VelUncertainty.bearing = (long)gLocationInfo->velocity.bearing;
        pVelocity->choice.VelUncertainty.horizontalSpeed = (long)gLocationInfo->velocity.horizontalSpeed;
        pVelocity->choice.VelUncertainty.verticalDirection = gLocationInfo->velocity.verticalDirect;
        pVelocity->choice.VelUncertainty.verticalSpeed = (long)gLocationInfo->velocity.verticalSpeed;
        pVelocity->choice.VelUncertainty.horizontalUncertaintySpeed = 0;
        pVelocity->choice.VelUncertainty.verticalUncertaintySpeed = 0;
       
        D("[wYear:%d][wMonth:%d][wDay:%d][wHour:%d][wMinute:%d][seconds:%d]\n",
        pTime->wYear,pTime->wMonth,pTime->wDay,pTime->wHour,pTime->wMinute,pTime->wSecond);
        
    }
    else
    {
         ret = -1;
         E("%s input point null",__FUNCTION__); 
    }
        
	return ret;
}



int gnss_lcsgetMsr(LcsGnssInstrHeader_t  *ptHeader)
{
    int ret = 0;
    GnssArgsGetMeas_t *pMeasure;
    GNSSMsrSetList_t *pMsrSetList = NULL;
    GNSSVelocity_t *pVelocity = NULL;  

    D("%s domain %d,sid:%d ",__FUNCTION__,ptHeader->domain,ptHeader->sid); 
    if(ptHeader->argsLen != GNSS_ARGS_SIZE_GET_MEAS)
    {
        D("%s input param len error: %d",__FUNCTION__,ptHeader->argsLen); 
        ret = -1;
        return ret;
    }

    pMeasure = (GnssArgsGetMeas_t*)ptHeader->ptArgs;

    if(pMeasure)
    {
        pMsrSetList = pMeasure->ptMsrSets;
        pVelocity = pMeasure->ptVelocity;

        if((NULL != pMsrSetList)&& (NULL != pVelocity))
        {
            ret = gnssGetMsr(pMsrSetList,pVelocity);
        }
        else
        {
            E("%s input point null error",__FUNCTION__); 
            ret = -1;
        }

    }
    else
    {
        E("%s input ptHeader->ptArgs point NULL",__FUNCTION__); 
        ret = -1;
    }

    return ret;
}


int  gnss_lcsshowPositionInfo(LcsGnssInstrHeader_t  *ptHeader)
{
    int ret = 0;
    
    D("%s domain %d,sid= %d",__FUNCTION__,ptHeader->domain,ptHeader->sid); 
    if(ptHeader->argsLen !=  GNSS_ARGS_SIZE_SHOW_POS_PD)
    {
        D("%s input param len error: %d",__FUNCTION__,ptHeader->argsLen); 
        ret = -1;
        return ret;
    }

	return ret;
}

int gnss_lcsSUPL_NI_END(LcsGnssInstrHeader_t  *ptHeader)
{
    int ret = 0;
    
    D("%s domain %d,sid= %d",__FUNCTION__,ptHeader->domain,ptHeader->sid); 
    if(ptHeader->argsLen != GNSS_ARGS_SIZE_NOTIF_SESSN_END)
    {
        D("%s input param len error: %d",__FUNCTION__,ptHeader->argsLen); 
        ret = -1;
        return ret;
    }
    //turnoff ecid function 
    gnss_lcsContrlEcid(ptHeader->sid,ptHeader->domain,LCS_NWLR_ECID,0);
    msg_to_libgps(SEND_SUPL_END,0,0,0);
    return ret;
}



int gnss_lcsdeliverPostion(LcsGnssInstrHeader_t  *ptHeader)
{
    int ret = 0;
    GnssArgsDlvrMsaPos_t* pPostion =(GnssArgsDlvrMsaPos_t*)ptHeader->ptArgs;
    
    D("%s domain %d,sid= %d",__FUNCTION__,ptHeader->domain,ptHeader->sid); 
    if(ptHeader->argsLen != GNSS_ARGS_SIZE_DLVR_MSA_POS)
    {
        D("%s input param len error: %d",__FUNCTION__,ptHeader->argsLen); 
        ret = -1;
        return ret;
    }

    if(LCS_DOMAIN_UP == ptHeader->domain)
    {
	    msg_to_libgps(SEND_MSA_POSITION,0,(void *)pPostion->ptPosition,(void *)pPostion->ptVelocity);
    }
    else if(LCS_DOMAIN_CP == ptHeader->domain) // it can delete it 
    {
        gnss_lcsContrlEcid(ptHeader->sid,ptHeader->domain,LCS_NWLR_ECID,0);
        gnss_lcsstopSiSessionReq(ptHeader->sid);
    }
    else
    {
        E("%s domain  error %d ",__FUNCTION__,ptHeader->domain); 
    }
    return ret;
    
}


//this function shourd delete 
int gnss_lcscpLcsPosRsp(LcsGnssInstrHeader_t  *ptHeader)
{
    int ret = 0;
    
    D("%s domain %d,sid= %d",__FUNCTION__,ptHeader->domain,ptHeader->sid); 
    if(ptHeader->argsLen !=  GNSS_ARGS_SIZE_DLVR_POS_RESP)
    {
        D("%s input param len error: %d",__FUNCTION__,ptHeader->argsLen); 
        ret = -1;
        return ret;
    }
    gnss_lcsContrlEcid(ptHeader->sid,ptHeader->domain,LCS_NWLR_ECID,0);
    gnss_lcsstopSiSessionReq(ptHeader->sid);
    return 0;
}


/* AGPS deliver GEO fence related parameters */
int gnss_lcsdeliverAreaEventParam(LcsGnssInstrHeader_t  *ptHeader)
{
    TcgAreaEventParams_t *pParam = NULL; 
    TcgAreaIdLists_t *pLists = NULL;
    GnssArgsDlvrAreaEvent_t * pAreaEvent;
    int ret = 0;

    D("%s domain %d,sid= %d",__FUNCTION__,ptHeader->domain,ptHeader->sid); 
    if(ptHeader->argsLen != GNSS_ARGS_SIZE_DLVR_AREA_EVENT)
    {
        D("%s input param len error: %d",__FUNCTION__,ptHeader->argsLen); 
        ret = -1;
        return ret;
    }
    pAreaEvent = (GnssArgsDlvrAreaEvent_t*)ptHeader->ptArgs;
    pParam = pAreaEvent->ptEvent;
    pLists = pAreaEvent->ptIds;
	D("%s sid:%d,sizeof TcgAreaEventParams_t =%d,type=%d,isposEst=%d",\
        __FUNCTION__,ptHeader->sid,(int)sizeof(TcgAreaEventParams_t),pParam->type,pParam->isPosEst);
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
	return ret;
}


int gnss_lcsdeliverAssDataIonoModel(LcsGnssInstrHeader_t  *ptHeader)
{
    int ret = 0;
    GNSSIonosphericModel_t *pIonoModel = (GNSSIonosphericModel_t*)ptHeader->ptArgs;

    
    D("%s domain %d,sid= %d",__FUNCTION__,ptHeader->domain,ptHeader->sid); 
    if(ptHeader->argsLen !=  GNSS_ARGS_SIZE_DLVR_IONO_MODEL)
    {
        D("%s input param len error: %d",__FUNCTION__,ptHeader->argsLen); 
        ret = -1;
        return ret;
    }

    ret = CG_deliverAssDataIonoModel(ptHeader->sid,pIonoModel);

    return ret;
}



static int gnss_lcsdeliverGNSSAssData(LcsGnssInstrHeader_t  *ptHeader)
{
    int i = 0,gcount = 0, ret = 0;
    GpsState*  s = _gps_state;
    GNSSAssistData_t *pAssData = (GNSSAssistData_t *)ptHeader->ptArgs;

   D("%s domain %d,sid= %d",__FUNCTION__,ptHeader->domain,ptHeader->sid); 
   if(ptHeader->argsLen !=  GNSS_ARGS_SIZE_DLVR_AD_GNSS)
   {
       D("%s input param len error: %d",__FUNCTION__,ptHeader->argsLen); 
       ret = -1;
       return ret;
   }

    ret =  CG_deliverGNSSAssData(ptHeader->sid,pAssData);
    
    return ret;
}


static int gnss_lcsdeliverAssistData_End(LcsGnssInstrHeader_t  *ptHeader)
{
    int ret = 0;
    
    D("%s domain %d,sid= %d",__FUNCTION__,ptHeader->domain,ptHeader->sid); 
    if(ptHeader->argsLen !=  GNSS_ARGS_SIZE_DLVR_AD_END)
    {
        D("%s input param len error: %d",__FUNCTION__,ptHeader->argsLen); 
        ret = -1;
        return ret;
    }
    ret = CG_deliverAssistData_End(ptHeader->sid);
    
	return ret;
}


static int gnss_lcsReadyToRecvPosInfo(LcsGnssInstrHeader_t  *ptHeader)
{
    int ret = 0;
    char rspType;
    GNSSPosReq_t  *pPosReq;
    GnssArgsReadyRecvMeasPos_t * pReadRecv = (GnssArgsReadyRecvMeasPos_t*)ptHeader->ptArgs; 
    
    D("%s domain %d,sid= %d",__FUNCTION__,ptHeader->domain,ptHeader->sid); 
    if(ptHeader->argsLen !=  GNSS_ARGS_SIZE_READY_RECV_MEAS_POS)
    {
        D("%s input param len error: %d",__FUNCTION__,ptHeader->argsLen); 
        ret = -1;
        return ret;
    }
    rspType = pReadRecv->rspType;
    pPosReq = pReadRecv->ptReq;
    agpsdata->gnssidReq = pPosReq->gnssPosTech;
	agpsdata->ni_sid = ptHeader->sid;
    D("rspType:%d,posreq:%d",rspType,pPosReq->gnssPosTech);
    msg_to_libgps(SEND_SUPL_READY,0,&(ptHeader->sid),&rspType);
    return 0;
}

static int gnss_lcsGetLoc(LcsGnssInstrHeader_t  *ptHeader)
{
	int ret = 0;
	GpsState *s = _gps_state;

    GnssArgsGetRefLoc_t * pMsg = (GnssArgsGetRefLoc_t *)ptHeader->ptArgs;
#if 0
	D("cmcc mode is %d",s->cmcc);
	if(s->cmcc == 1)
	{
		D("in cmcc mode,not get cid from libgps");
		return 1;
	}
#endif
    D("lcsgetloc is enter");
	ret = msg_to_libgps(CELL_LOCATION_GET_CMD, (unsigned char *)pMsg,0,0);
	
	return ret;	
}
/*--------------------------------------------------------------------------
    Function:  gnss_lcscallback

    Description:
        it  
        
    Parameters:int 
	 
    Return: 
		 1 :  0: 

--------------------------------------------------------------------------*/

static  int gnss_lcscallback(LcsGnssInstrHeader_t  *ptHeader)
{
    int ret = -1;
    
    if(NULL == ptHeader)
    {
        return ret;       
    }
    
    D("gnss_lcscallback: type:0x%x, domain:%d",ptHeader->instr,ptHeader->domain);
    switch(ptHeader->instr)
    {
        case GNSS_INSTR_START:
        {
            ret = gnss_lcsrequestSession(ptHeader);
            break;
        }
        case GNSS_INSTR_RESET_AD:
        {
            // noting to do 
            break;
        }
        case GNSS_INSTR_NI_NOTIF:
        {
            ret = gnss_lcsregisterWAPandSMS(ptHeader);
            break;
        }
        case GNSS_INSTR_GET_SETID:
        {
            ret = gnss_lcsgetSetID(ptHeader);
            break;
        }
        case GNSS_INSTR_GET_IMSI:
        {
            ret  = gnss_lcsgetIMSI(ptHeader);
            break;
        }
        case GNSS_INSTR_GET_MSISDN:
        {
            ret = gnss_lcsgetMSISDN(ptHeader);
            break;
        }
        case GNSS_INSTR_GET_HPLMN:
        {
            ret = gnss_lcsgetHplmnInfo(ptHeader);
            break;
        }
        case GNSS_INSTR_GET_IPADDR:
        {
            ret = gnss_lcsIPAddress(ptHeader);
            break;
        }
        case GNSS_INSTR_GET_WLANCELL:
        {
            ret = gnss_lcsWlanApInfo(ptHeader);
            break;
        }
        case GNSS_INSTR_GET_MEAS:
        {
            ret = gnss_lcsgetMsr(ptHeader);
            break;
        }
        case GNSS_INSTR_GET_POS:
        {
            ret = gnss_lcsgetPosition(ptHeader);
            break;
        }
        case GNSS_INSTR_SHOW_POS_PD:
        {
            ret = gnss_lcsshowPositionInfo(ptHeader);
            break;
        }
        case GNSS_INSTR_NOTIF_SESSN_END:
        {
            ret = gnss_lcsSUPL_NI_END(ptHeader);
            break;
        }
        case GNSS_INSTR_DLVR_MSA_POS:
        {
            ret = gnss_lcsdeliverPostion(ptHeader);
            break;
        }
        case GNSS_INSTR_DLVR_POS_RESP:
        {
            ret = gnss_lcscpLcsPosRsp(ptHeader);
            break;
        }
        case GNSS_INSTR_DLVR_AREA_EVENT:
        {
            ret = gnss_lcsdeliverAreaEventParam(ptHeader);
            break;
        }
        case GNSS_INSTR_DLVR_IONO_MODEL:
        {
            ret = gnss_lcsdeliverAssDataIonoModel(ptHeader);
            break;
        }
        case GNSS_INSTR_DLVR_AD_GNSS:
        {
            ret = gnss_lcsdeliverGNSSAssData(ptHeader);
            break;
        }
        case GNSS_INSTR_DLVR_AD_END:
        {
            ret = gnss_lcsdeliverAssistData_End(ptHeader);
            break;
        }
        case GNSS_INSTR_READY_RECV_MEAS_POS:
        {
            ret = gnss_lcsReadyToRecvPosInfo(ptHeader);
            break;
        }
        case GNSS_INSTR_GET_REFLOC:
        {
            ret = gnss_lcsGetLoc(ptHeader);
            break;
        }
        default://GNSS_INSTR_GET_TLSPSK
                //GNSS_INSTR_DLVR_AD_LPPE
                //GNSS_INSTR_READY_RECV_HIST_REPORT
            break;
    }
    D("gnss_lcscallback: ret:%d",ret);
    return ret;
}


int  gnss_lcshost_port(const char *pFQDN_H_SLP, CgIPAddress_t* pIpAddr, unsigned short Port)
{
    int ret = 0;
    LcsGnssInstrHeader_t  gnssmsg;
    LcsArgsSetHslp_t  address;

    address.ptFqdn =(unsigned char *)pFQDN_H_SLP;
    address.ptIpOfSlp = pIpAddr;
    address.port   = Port;

    gnssmsg.domain = LCS_DOMAIN_UP;
    gnssmsg.sid = agpsdata->si_sid; // it should creae sid from command 
    gnssmsg.instr = LCS_INSTR_SET_HSLP;
    gnssmsg.argsLen =  LCS_ARGS_SIZE_SET_HSLP;
    gnssmsg.ptArgs = (unsigned char*)(&address);
    ret = pgLCSHandle->exec(&gnssmsg);

    return ret;
}


    
int  gnss_lcschangserver(unsigned char Maj, unsigned char Min, 
                            unsigned char ServInd)
{
    int ret = 0;
    LcsGnssInstrHeader_t  gnssmsg;
    LcsArgsSetVersion_t version;
    memset(&version,0,sizeof(LcsArgsSetVersion_t)); 
    version.maj = Maj;
    version.min = Min;
    version.servInd = ServInd;
    gnssmsg.domain = LCS_DOMAIN_UP;
    gnssmsg.sid = agpsdata->si_sid; // it should creae sid from command 
    gnssmsg.instr = LCS_INSTR_SET_VERSION;
    gnssmsg.argsLen = LCS_ARGS_SIZE_SET_VERSION;
    gnssmsg.ptArgs = (unsigned char*)(&version);
    ret = pgLCSHandle->exec(&gnssmsg);

    return ret;
}


int gnss_lcscpcfgSetCapability(CgSetCapability_t * pSetCapability)
{
    int ret = 0;
    LcsGnssInstrHeader_t  gnssmsg;
    LcsArgsSetCapability_t glcsCap ;
    char mode_string[32];
    GpsState*  s = _gps_state;
    
    if((NULL == pSetCapability)||(NULL == pgLCSHandle))
    {
        E("%s : input param point null or lcshandle null ",__FUNCTION__);     
        return -1;
    }
    memset(mode_string,0,sizeof(mode_string));
    if(!config_xml_ctrl(GET_XML,"CP-MODE",mode_string,CONFIG_TYPE))
    {
        D("in getresponse,get cp-mode in xml is %s",mode_string);   
        s->cpmode = (mode_string[0]-'0')*4 + (mode_string[1]-'0')*2 + mode_string[2]-'0'; 
    }
	if(s->cpmode == 1)
	{
	    D("gps only,not set capbility");
	    memset(&glcsCap, 0, sizeof(LcsArgsSetCapability_t));
		memcpy(&(glcsCap.capability),pSetCapability,sizeof(CgSetCapability_t)); 
		glcsCap.posMethodList.gnssNum = 0;
	}
    else
	{
		memset(&glcsCap, 0, sizeof(LcsArgsSetCapability_t));
		memcpy(&(glcsCap.capability),pSetCapability,sizeof(CgSetCapability_t)); 
		glcsCap.posMethodList.gnssNum = 1;
		//glcsCap.posMethodList.posMethod[0].gnssID = SUPL_GNSS_ID_GALILEO;
		//glcsCap.posMethodList.posMethod[0].gnssPosMode = GNSS_POS_MODE_MSA|GNSS_POS_MODE_MSB|GNSS_POS_MODE_AUTO;
		//glcsCap.posMethodList.posMethod[0].gnssSignal = (1<<GNSS_SIGNAL_GALILEO_E1)|(1<<GNSS_SIGNAL_GALILEO_E5a);
		if((s->cpmode == 4) || (s->cpmode == 5))
		{
		    glcsCap.posMethodList.posMethod[0].gnssID = SUPL_GNSS_ID_GLONASS;
		    glcsCap.posMethodList.posMethod[0].gnssPosMode = GNSS_POS_MODE_MSA|GNSS_POS_MODE_MSB|GNSS_POS_MODE_AUTO; 
		    glcsCap.posMethodList.posMethod[0].gnssSignal = (1<<GNSS_SIGNAL_GLONASS_G1)|(1<<GNSS_SIGNAL_GLONASS_G2)|(1<<GNSS_SIGNAL_GLONASS_G3);
		}
		else
		{
		    glcsCap.posMethodList.posMethod[0].gnssID = SUPL_GNSS_ID_BDS;
		    glcsCap.posMethodList.posMethod[0].gnssPosMode = GNSS_POS_MODE_MSA|GNSS_POS_MODE_MSB|GNSS_POS_MODE_AUTO; 
		    glcsCap.posMethodList.posMethod[0].gnssSignal = (1<<GNSS_SIGNAL_BDS_B1I);
		}
	}
    gnssmsg.domain = LCS_DOMAIN_CP;
    gnssmsg.sid = agpsdata->si_sid; // it should creae sid from command 
    gnssmsg.instr = LCS_INSTR_SET_CAPABILITY;
    gnssmsg.argsLen = LCS_ARGS_SIZE_SET_CAPABILITY;
    gnssmsg.ptArgs = (unsigned char*)(&glcsCap);
    ret = pgLCSHandle->exec(&gnssmsg);

    return ret;

}

 int gnss_lcsupcfgSetCapability(CgSetCapability_t * pSetCapability)
 {
     int ret = 0;
     LcsGnssInstrHeader_t  gnssmsg;
     LcsArgsSetCapability_t glcsCap ;
     char mode_string[32];
     GpsState*  s = _gps_state;
    
    if((NULL == pSetCapability)||(NULL == pgLCSHandle))
    {
        E("%s : input param point null or lcshandle null ",__FUNCTION__);     
        return -1;
    }
    memset(mode_string,0,sizeof(mode_string));
    if(!config_xml_ctrl(GET_XML,"CP-MODE",mode_string,CONFIG_TYPE))
    {
        D("in getresponse,get cp-mode in xml is %s",mode_string);   
        s->cpmode = (mode_string[0]-'0')*4 + (mode_string[1]-'0')*2 + mode_string[2]-'0'; 
    }
    
    memset(&glcsCap, 0, sizeof(LcsArgsSetCapability_t));
    memcpy(&(glcsCap.capability),pSetCapability,sizeof(CgSetCapability_t)); 
    
	if(s->cpmode == 1)
	{
	    D("gps only,not set capbility,so  don't config posMethodList");
	}
    else
    {
        glcsCap.posMethodList.gnssNum = 1;
        //glcsCap.posMethodList.posMethod[0].gnssID = SUPL_GNSS_ID_GALILEO;
        //glcsCap.posMethodList.posMethod[0].gnssPosMode = GNSS_POS_MODE_MSA|GNSS_POS_MODE_MSB|GNSS_POS_MODE_AUTO;
        //glcsCap.posMethodList.posMethod[0].gnssSignal = (1<<GNSS_SIGNAL_GALILEO_E1)|(1<<GNSS_SIGNAL_GALILEO_E5a);
        if((s->cpmode == 4) || (s->cpmode == 5))
        {
            glcsCap.posMethodList.posMethod[0].gnssID = SUPL_GNSS_ID_GLONASS;
            glcsCap.posMethodList.posMethod[0].gnssPosMode = GNSS_POS_MODE_MSA|GNSS_POS_MODE_MSB|GNSS_POS_MODE_AUTO; 
            glcsCap.posMethodList.posMethod[0].gnssSignal = (1<<GNSS_SIGNAL_GLONASS_G1)|(1<<GNSS_SIGNAL_GLONASS_G2)|(1<<GNSS_SIGNAL_GLONASS_G3);
        }
        else
        {
            glcsCap.posMethodList.posMethod[0].gnssID = SUPL_GNSS_ID_BDS;
            glcsCap.posMethodList.posMethod[0].gnssPosMode = GNSS_POS_MODE_MSA|GNSS_POS_MODE_MSB|GNSS_POS_MODE_AUTO; 
            glcsCap.posMethodList.posMethod[0].gnssSignal = (1<<GNSS_SIGNAL_BDS_B1I);
        }
    }
    
     gnssmsg.domain = LCS_DOMAIN_UP;
     gnssmsg.sid = agpsdata->si_sid; // it should creae sid from command 
     gnssmsg.instr = LCS_INSTR_SET_CAPABILITY;
     gnssmsg.argsLen = LCS_ARGS_SIZE_SET_CAPABILITY;
     gnssmsg.ptArgs =(unsigned char*)(&glcsCap);
     D("gnss_lcsupcfgSetCapability set capbility:%d",glcsCap.posMethodList.posMethod[1].gnssID);
     ret = pgLCSHandle->exec(&gnssmsg);

     return ret;
 
 }


 
static int gnss_lcswaitReady(LcsDomain_bm type)
{
    int ret = 0;
    LcsArgsWaitClientReady_t waitReady;
    LcsGnssInstrHeader_t  gnssmsg;
    
    gnssmsg.domain = type;
    gnssmsg.sid  = agpsdata->si_sid;
    gnssmsg.instr = LCS_INSTR_WAIT_CLIENT_READY; 
    gnssmsg.argsLen = LCS_ARGS_SIZE_WAIT_CLIENT_READY;
    waitReady.timeoutMsec = 500;
    gnssmsg.ptArgs = (unsigned char*)(&waitReady);
    ret = pgLCSHandle->exec(&gnssmsg);
    return ret;
}

  void gnss_lcsinit()
 {
     void* handle;
     char *error;
     LcsMgtDrvData_t    glCSMgt;

     LcsMgtInterface_t*  (*lcs_getInterface)(void);
     D("%s: before dleopn liblcsmgt.so \n",__FUNCTION__);
 #if __LP64__
     handle = dlopen("/system/lib64/liblcsmgt.so", RTLD_LAZY);
 #else
     handle = dlopen("/system/lib/liblcsmgt.so", RTLD_LAZY);
 #endif
     if (!handle)
     {
        E("%s\n", dlerror());
        return ;
     }
     D(" dlopen liblcsmgt.so success\n");
     dlerror(); /* Clear any existing error */
     lcs_getInterface = dlsym(handle, "LCSMGT_getInterface");
     pgLCSHandle = lcs_getInterface();
     glCSMgt.size = sizeof(LcsMgtDrvData_t);
     glCSMgt.lcs_cb = gnss_lcscallback;
     D("get LCSMGT_getInterface and init para:%p,size:%zu\n",&glCSMgt,glCSMgt.size);
     pgLCSHandle->init(&glCSMgt);

     return;
 }

 
 void gnss_lcsstart()
{
	unsigned short port = 7275;
	char value[32];
	char *reset_func = "RESET-ENABLE";
	char *udp_func = "UDP-ENABLE";
	int ret = 0;
    int i = 0;
    LcsGnssInstrHeader_t  gnssmsg;
    GnssArgsStart_t gnssstrt;

    if(NULL == pgLCSHandle)
    {
        E("%s: get pgLCSHandle point null",__FUNCTION__);
        return; 
    }
    agpsdata->si_sid = get_session_id(1);
	D("%s: UP mode will start",__FUNCTION__); 
    gnssmsg.domain = LCS_DOMAIN_UP;
    gnssmsg.sid = agpsdata->si_sid;
    gnssmsg.instr = LCS_INSTR_START_CLIENT; 
    gnssmsg.argsLen = sizeof(GnssArgsStart_t);
    gnssmsg.ptArgs = (unsigned char*)(&gnssstrt);
    pgLCSHandle->exec(&gnssmsg);
    ret = gnss_lcswaitReady(LCS_DOMAIN_UP);
    while(ret)
    {
        ret = gnss_lcswaitReady(LCS_DOMAIN_CP);
    }
    D("%s: UP param set",__FUNCTION__);
    set_supl_param();

    D("%s: CP mode will start",__FUNCTION__);
    
    gnssmsg.domain = LCS_DOMAIN_CP;
    gnssmsg.sid = agpsdata->si_sid;
    gnssmsg.instr = LCS_INSTR_START_CLIENT;
    gnssmsg.argsLen = sizeof(GnssArgsStart_t);
    gnssmsg.ptArgs = (unsigned char*)(&gnssstrt);
    pgLCSHandle->exec(&gnssmsg);
    ret = gnss_lcswaitReady(LCS_DOMAIN_CP);
    while(ret)
    {
        ret = gnss_lcswaitReady(LCS_DOMAIN_CP);
    }
    D("%s: CP param set",__FUNCTION__);
    set_cp_param();

	return;
}


int gnss_lcssetPosMode(eCG_PosMethod aPosMethod)
{
    int ret = 0;
    LcsGnssInstrHeader_t  gnssmsg;
    LcsArgsSetPosMethod_t agpsmethod;
    
    agpsmethod.posMethod = aPosMethod;

    gnssmsg.domain = LCS_DOMAIN_UP;
    gnssmsg.sid = agpsdata->si_sid;
    gnssmsg.instr = LCS_INSTR_SET_POS_METHOD;
    gnssmsg.argsLen = LCS_ARGS_SIZE_SET_POS_METHOD;
    gnssmsg.ptArgs = (unsigned char*)(&agpsmethod);
    ret = pgLCSHandle->exec(&gnssmsg);

    return ret;
}

void gnss_lcspsStatus(BOOL psIsConnected)
{
    LcsGnssInstrHeader_t  gnssmsg;
    LcsArgsNotifPsStatus_t agpslink;

    agpslink.isConnected = psIsConnected;
    gnssmsg.domain = LCS_DOMAIN_UP;
    gnssmsg.sid = agpsdata->si_sid;
    gnssmsg.instr = LCS_INSTR_NOTIF_PS_STATUS;
    gnssmsg.argsLen = LCS_ARGS_SIZE_NOTIF_PS_STATUS;
    gnssmsg.ptArgs =(unsigned char*)(&agpslink);
    pgLCSHandle->exec(&gnssmsg);
}

signed int gnss_lcsgotSUPLINIT(int sid, unsigned char *pData, unsigned short len)
{
    int ret = 0;
    LcsGnssInstrHeader_t  gnssmsg;
    LcsArgsRecvSuplInit_t suplinit;

    suplinit.dataLen = len;
    suplinit.ptData = pData;
    gnssmsg.domain = LCS_DOMAIN_UP;
    gnssmsg.sid = sid;
    gnssmsg.instr = LCS_INSTR_RECV_SUPL_INIT;
    gnssmsg.argsLen =  LCS_ARGS_SIZE_RECV_SUPL_INIT;
    gnssmsg.ptArgs = (unsigned char*)(&suplinit);
    ret = pgLCSHandle->exec(&gnssmsg);

    return ret;
}

int gnss_lcsrequestToGetMsaPosition(int sid, unsigned char isFirstTime)
{
    int ret = 0;
    LcsGnssInstrHeader_t  gnssmsg;
    LcsArgsReqMsaPos_t msapos;

    msapos.isFirstTime = isFirstTime;
    gnssmsg.domain = LCS_DOMAIN_UP;
    gnssmsg.sid = sid;
    gnssmsg.instr = LCS_INSTR_REQ_MSA_POS;
    gnssmsg.argsLen = LCS_ARGS_SIZE_REQ_MSA_POS;
    gnssmsg.ptArgs = (unsigned char*)(&msapos);
    ret = pgLCSHandle->exec(&gnssmsg);

    return ret;
}

int gnss_lcsniResponse(int sid, int userResponse,LcsDomain_bm type)
{
    int ret = 0;
    LcsGnssInstrHeader_t  gnssmsg;
    LcsArgsInjctUserResp_t user;

    user.response = userResponse;
    gnssmsg.domain = type;
    gnssmsg.sid = sid;
    gnssmsg.instr = LCS_INSTR_INJCT_USER_RESP;
    gnssmsg.argsLen = LCS_ARGS_SIZE_INJCT_USER_RESP;
    gnssmsg.ptArgs = (unsigned char*)(&user);
    ret = pgLCSHandle->exec(&gnssmsg);

    return ret;
}

int gnss_lcsgnssMeasureInfo(int sid, GNSSMsrSetList_t *pMsrSetList, 
                                  GNSSVelocity_t *pVelocity,LcsDomain_bm type)
{
    int ret = 0;
    LcsGnssInstrHeader_t  gnssmsg;
    LcsArgsInjctMeas_t    inject;

    inject.ptMsrSets = pMsrSetList;
    inject.ptVelocity = pVelocity;
    gnssmsg.domain = type;
    gnssmsg.sid = sid;  
    gnssmsg.instr = LCS_INSTR_INJCT_MEAS;
    gnssmsg.argsLen = LCS_ARGS_SIZE_INJCT_MEAS;
    gnssmsg.ptArgs = (unsigned char*)(&inject);
    ret = pgLCSHandle->exec(&gnssmsg);

    return ret;
}

int gnss_lcsstartSISessionForGnss(int sid,
                                         LcsDomain_bm  type,
                                         CpSiLocReqType_e ReqType,
                                         GNSSReqAssData_t *pReqAssData,
                                         TCgSetQoP_t *pQoP,
                                         CgThirdPartyList_t *pThirdPartyList)
{
    int ret = 0;
    LcsGnssInstrHeader_t  gnssmsg;
    LcsArgsStartSiSessn_t startsi;

    memset(&startsi,0,LCS_ARGS_SIZE_START_SI_SESSN);
    startsi.cpSiType = ReqType;
    pReqAssData->gpsAssistanceRequired = 0x134; 
    startsi.ptReqAssData = pReqAssData;
    startsi.ptQoP = pQoP;
    startsi.pt3rdParties = pThirdPartyList;

    gnssmsg.domain = type;
    gnssmsg.sid = sid;  
    gnssmsg.instr = LCS_INSTR_START_SI_SESSN;
    gnssmsg.argsLen = LCS_ARGS_SIZE_START_SI_SESSN;
    gnssmsg.ptArgs = (unsigned char*)(&startsi);
    ret = pgLCSHandle->exec(&gnssmsg);
    D("%s 0x%x",__FUNCTION__,pReqAssData->gpsAssistanceRequired);

    return ret;

}

int gnss_lcsstopSiSessionReq(int sid)
{
    int ret = 0;
    LcsGnssInstrHeader_t  gnssmsg;
    LcsArgsStopSiSessn_t stopsi;

    memset(&stopsi,0,sizeof(LcsArgsStopSiSessn_t)); 
    gnssmsg.domain = LCS_DOMAIN_CP;
    gnssmsg.sid = sid;  
    gnssmsg.instr = LCS_INSTR_STOP_SI_SESSN;
    gnssmsg.argsLen = LCS_ARGS_SIZE_STOP_SI_SESSN;
    gnssmsg.ptArgs =(unsigned char*)(&stopsi);
    ret = pgLCSHandle->exec(&gnssmsg);

    return ret;
}

int gnss_lcsstartSIPeriodicSessionForGnss(int  sid,
                                                    GNSSReqAssData_t *pReqAssData, 
                                                    TCgSetQoP_t   *pQoP,
                                                    unsigned long  startTime, 
                                                    unsigned long   stopTime,  
                                                    unsigned long  intervalTime,
                                                    CgThirdPartyList_t   *pThirdPartyList)

{

    int ret = 0;
    LcsGnssInstrHeader_t  gnssmsg;
    LcsArgsStartSiSessn_t startsipd;

    memset(&startsipd,0,sizeof(LcsArgsStartSiSessn_t)); 

    startsipd.ptReqAssData = pReqAssData;
    startsipd.ptQoP = pQoP;
    startsipd.startTime = startTime;
    startsipd.stopTime  = stopTime;
    startsipd.intervalTime = intervalTime;
    startsipd.pt3rdParties = pThirdPartyList;

    gnssmsg.domain = LCS_DOMAIN_UP;
    gnssmsg.sid = sid;  
    gnssmsg.instr = LCS_INSTR_START_SI_SESSN_PD;
    gnssmsg.argsLen = LCS_ARGS_SIZE_START_SI_SESSN;
    gnssmsg.ptArgs = (unsigned char*)(&startsipd);
    ret = pgLCSHandle->exec(&gnssmsg);

    return ret;

}


int gnss_lcscancelTriggerSession(int sid)
{
    int ret = 0;
    LcsGnssInstrHeader_t  gnssmsg;
    LcsArgsStopSiSessn_t stopsipd;

    memset(&stopsipd,0,sizeof(LcsArgsStopSiSessn_t)); 
    gnssmsg.domain = LCS_DOMAIN_UP;
    gnssmsg.sid = sid;  
    gnssmsg.instr = LCS_INSTR_STOP_SI_SESSN_PD;
    gnssmsg.argsLen = LCS_ARGS_SIZE_STOP_SI_SESSN;
    gnssmsg.ptArgs = (unsigned char*)(&stopsipd);
    ret = pgLCSHandle->exec(&gnssmsg);

    return ret;
}

int gnss_lcsstartSIAreaEventSessionForGnss(int sid,
                                                      GNSSReqAssData_t *pReqAssData, 
                                                      TCgSetQoP_t  *pQoP,
                                                      TcgAreaEventParams_t *pParam)
{
    int ret = 0;
    LcsGnssInstrHeader_t  gnssmsg;
    LcsArgsStartSiSessn_t startsiArea;

    memset(&startsiArea,0,sizeof(LcsArgsStartSiSessn_t)); 
    startsiArea.ptReqAssData = pReqAssData;
    startsiArea.ptQoP = pQoP;
    startsiArea.ptEvent = pParam;
    gnssmsg.domain = LCS_DOMAIN_UP;
    gnssmsg.sid = sid;  
    gnssmsg.instr = LCS_INSTR_START_SI_SESSN_AREA;
    gnssmsg.argsLen = LCS_ARGS_SIZE_START_SI_SESSN;
    gnssmsg.ptArgs = (unsigned char*)(&startsiArea);
    ret = pgLCSHandle->exec(&gnssmsg);

    return ret;

}

int gnss_lcsinformAreaEventTriggered(int sid, GNSSPositionParam_t *pPos)
{
    int ret = 0;
    LcsGnssInstrHeader_t  gnssmsg;
    LcsArgsTrigAreaEvent_t trigArea;
    
    memset(&trigArea,0,sizeof(LcsArgsTrigAreaEvent_t)); 
    trigArea.ptPosition = pPos;
    gnssmsg.domain = LCS_DOMAIN_UP;
    gnssmsg.sid = sid;  
    gnssmsg.instr = LCS_INSTR_TRIG_AREA_EVENT;
    gnssmsg.argsLen = LCS_ARGS_SIZE_TRIG_AREA_EVENT;
    gnssmsg.ptArgs = (unsigned char*)(&trigArea);
    ret = pgLCSHandle->exec(&gnssmsg);

    return ret;
}

int gnss_lcsendAreaEventSession(int sid)
{
    int ret = 0;
    LcsGnssInstrHeader_t  gnssmsg;
    LcsArgsStopSiSessn_t stopArea;
    
    memset(&stopArea,0,sizeof(LcsArgsStopSiSessn_t)); 
    gnssmsg.domain = LCS_DOMAIN_UP;
    gnssmsg.sid = sid;  
    gnssmsg.instr = LCS_INSTR_STOP_SI_SESSN_AREA;
    gnssmsg.argsLen = LCS_ARGS_SIZE_STOP_SI_SESSN;
    gnssmsg.ptArgs = (unsigned char*)(&stopArea);
    ret = pgLCSHandle->exec(&gnssmsg);

    return ret;
}


static int gnss_lcsReqModemAssist(int sid,int domain,LcsArgsCtrlNwlr_t* pCommand)
{
    int ret = 0;
    LcsGnssInstrHeader_t  gnssmsg;
    
    gnssmsg.domain = domain;
    gnssmsg.sid = sid;  
    gnssmsg.instr = LCS_INSTR_CTRL_NWLR;
    gnssmsg.argsLen = LCS_ARGS_SIZE_CTRL_NWLR;
    gnssmsg.ptArgs = (unsigned char*)(pCommand);
    ret = pgLCSHandle->exec(&gnssmsg);

    return ret;
}

int gnss_lcsContrlEcid(int sid,LcsDomain_bm domain,int method,int flag)
{
    int ret = 0;
    LcsArgsCtrlNwlr_t ctrlEcid;

    ctrlEcid.domain = domain;
    ctrlEcid.method = method;
    ctrlEcid.reportAmount = 0;
    ctrlEcid.reportInterval = 0;

    if(flag)
    {
        ctrlEcid.command = LCS_NWLR_START;

    }
    else
    {
        ctrlEcid.command = LCS_NWLR_STOP;
    }
    
    D("%s: enter sid: %d,domain:%d, method:%d,flag:%d",__FUNCTION__, sid,domain,method,flag);
    ret = gnss_lcsReqModemAssist(sid,domain,&ctrlEcid);

    return ret;
}



int gnss_lcsInjectPos(int sid,int domain,GNSSPosition_t *pPos, GNSSVelocity_t *pVelocity,int gpstow)
{
    int ret = 0;
    LcsGnssInstrHeader_t  gnssmsg;
    LcsArgsInjctPos_t posTime;

        
    gnssmsg.domain = domain;
    gnssmsg.sid = sid;  
    gnssmsg.instr = LCS_INSTR_INJCT_POS;
    gnssmsg.argsLen = LCS_ARGS_SIZE_INJCT_POS;
    posTime.gpsTOW = gpstow;
    posTime.ptPosition = pPos;
    posTime.ptVelocity = pVelocity;
    
    D("%s:sid=%d,domain= %d,velType =%d,\
       ",__FUNCTION__,sid,domain,pVelocity->velType);

    gnssmsg.ptArgs = (unsigned char*)(&posTime);
    ret = pgLCSHandle->exec(&gnssmsg);
    return ret;
}

#endif //GNSS_LCS 
