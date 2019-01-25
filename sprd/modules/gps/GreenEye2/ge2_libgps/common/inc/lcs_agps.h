/******************************************************************************
** File Name:  lcs_agps.h                                                      *
** Author:                                                                    *
** Date:                                                                      *
** Copyright:  2001-2020 by Spreadtrum Communications, Inc.                   * 
**             All rights reserved.                                           *
**             This software is supplied under the terms of a license         *
**             agreement or non-disclosure agreement with Spreadtrum.         *
**             Passing on and copying of this document, and communication     *
**             of its contents is not permitted without prior written         *
**             authorization.                                                 *
**description:                                                                *
*******************************************************************************

*******************************************************************************
**                                Edit History                                *
**----------------------------------------------------------------------------*
** Date                  Name                      Description                *
**----------------------------------------------------------------------------*
** 2016-01-01            Lihuai.Wang                   Create.                    *
******************************************************************************/
#ifndef _LCS_AGPS_H_
#define _LCS_AGPS_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef GNSS_LCS 

#include "lcs_mgt.h"
int gnss_lcsgetHplmnInfo(LcsGnssInstrHeader_t  *ptHeader);
int gnss_lcsIPAddress(LcsGnssInstrHeader_t  *ptHeader);
int gnss_lcsWlanApInfo(LcsGnssInstrHeader_t  *ptHeader);
int gnss_lcsgetPosition(LcsGnssInstrHeader_t  *ptHeader);
int gnss_lcsgetMsr(LcsGnssInstrHeader_t  *ptHeader);
int gnss_lcsshowPositionInfo(LcsGnssInstrHeader_t  *ptHeader);
int gnss_lcsSUPL_NI_END(LcsGnssInstrHeader_t  *ptHeader);
int gnss_lcsdeliverPostion(LcsGnssInstrHeader_t  *ptHeader);
int gnss_lcscpLcsPosRsp(LcsGnssInstrHeader_t  *ptHeader);
int gnss_lcsdeliverAreaEventParam(LcsGnssInstrHeader_t  *ptHeader);
int gnss_lcsdeliverAssDataIonoModel(LcsGnssInstrHeader_t  *ptHeader);
int gnss_lcssetPosMode(eCG_PosMethod aPosMethod);
int gnss_lcsstartSIPeriodicSessionForGnss(int  sid,
                                                    GNSSReqAssData_t *pReqAssData, 
                                                    TCgSetQoP_t   *pQoP,
                                                    unsigned long  startTime, 
                                                    unsigned long   stopTime,  
                                                    unsigned long  intervalTime,
                                                    CgThirdPartyList_t   *pThirdPartyList);
int gnss_lcsstartSIAreaEventSessionForGnss(int sid,
                                                      GNSSReqAssData_t *pReqAssData, 
                                                      TCgSetQoP_t  *pQoP,
                                                      TcgAreaEventParams_t *pParam);
int  gnss_lcschangserver(unsigned char Maj, unsigned char Min, unsigned char ServInd);
int  gnss_lcshost_port(const char *pFQDN_H_SLP, CgIPAddress_t* pIpAddr, unsigned short Port);
int gnss_lcsupcfgSetCapability(CgSetCapability_t * pSetCapability);
int gnss_lcscpcfgSetCapability(CgSetCapability_t * pSetCapability);
int gnss_lcsniResponse(int sid, int userResponse,LcsDomain_bm type);

void gnss_lcspsStatus(BOOL psIsConnected);
signed int gnss_lcsgotSUPLINIT(int sid, unsigned char *pData, unsigned short len);
int gnss_lcsrequestToGetMsaPosition(int sid, unsigned char isFirstTime);
int gnss_lcsstartSISessionForGnss(int sid,
                                         LcsDomain_bm  type,
                                         CpSiLocReqType_e ReqType,
                                         GNSSReqAssData_t *pReqAssData,
                                         TCgSetQoP_t *pQoP,
                                         CgThirdPartyList_t *pThirdPartyList);
int gnss_lcsgnssMeasureInfo(int sid, GNSSMsrSetList_t *pMsrSetList, 
                                  GNSSVelocity_t *pVelocity,LcsDomain_bm type);
int gnss_lcsinformAreaEventTriggered(int sid, GNSSPositionParam_t *pPos);
int gnss_lcsendAreaEventSession(int sid);
int gnss_lcscancelTriggerSession(int sid);
int agps_stopSiSessionReq(int sid);
int gnss_lcsInjectPos(int sid,int domain,GNSSPosition_t *pPos, GNSSVelocity_t *pVelocity,int gpstow);
void gnss_lcsinit();
void gnss_lcsstart();
int gnss_lcsstopSiSessionReq(int sid);
int gnss_lcsContrlEcid(int sid,LcsDomain_bm domain,int method,int flag);


#endif //GNSS_LCS 

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _LCS_AGPS_H_ */

