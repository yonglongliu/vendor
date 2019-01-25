/******************************************************************************
** File Name:  agps_api.h                                                     *
** Author:                                                                    *
** Date:                                                                      *
** Copyright:  2001-2020 by Spreadtrum Communications, Inc.                   * 
**             All rights reserved.                                           *
**             This software is supplied under the terms of a license         *
**             agreement or non-disclosure agreement with Spreadtrum.         *
**             Passing on and copying of this document, and communication     *
**             of its contents is not permitted without prior written         *
**             authorization.                                                 *
**description: This file describes the APIs provided by agps module, which are
               called by libgps. And the APIs (callbacks) provided by libgps 
               module, which are called by agps module                        *
*******************************************************************************

*******************************************************************************
**                                Edit History                                *
**----------------------------------------------------------------------------*
** Date                  Name                      Description                *
**----------------------------------------------------------------------------*
** 2014-06-02            Andy.Huang                Create.                    *
******************************************************************************/

#ifndef _AGPS_API_H_
#define _AGPS_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef  GNSS_LCS


#include "agps_type.h"
#include "agnss_type.h"

#define AGPS_UP_SUPPORT
#define AGPS_CP_SUPPORT

/* AGPS User Plane Callbacks which should be provided by libgps */
typedef struct _AgpsUpCallbacks_t
{
    unsigned int size;  //size of this struct

    int (*closePhoneItf)(void);
    int (*registerWAPandSMS)(AgnssNiNotification_t *notification);

    /* AGPS get SET ID */
    CG_SETId_PR   (*getSETIdType)(void);
    int (*getIMSI)(unsigned char* pBuf, unsigned short usLenBuf);
    int (*getMSISDN)(unsigned char* pBuf, unsigned short usLenBuf);
    int (*getIPAddress)(CgIPAddress_t *ipAddress);
    
    /* AGPS get wireless network information */
    int (*getLocationId)(CgLocationId_t *pLocationId);
    int (*getWlanApInfo)(AgnssWlanCellList_t *pCgWlanCellList);

    /* AGPS get GEO position of the SET */
    int (*getPosition)(TcgTimeStamp   *pTime,
                       GNSSLocationCoordinates_t *pLastPos,
                       GNSSVelocity_t *pVelocity
                       );

    /* AGPS get TLS-PSK related keys */
    BOOL (*getPskForTls)(TcgGbaOctetStr_t *pKs,    //OUT: Ks of the specific IMSI
                         TcgGbaOctetStr_t *pBtid,  //OUT: B-TID of the specific IMSI
                         TcgGbaOctetStr_t *pRand,  //OUT:
                         TcgGbaOctetStr_t *pImpi   //OUT:
                         );

    /* AGPS deliver assistance data */
    int (*deliverAssDataRefTime)(int sid, TCgRxNTime *pTime);
    int (*deliverAssDataRefLoc)(int sid, void *pPos, TCgRxNTime *pTime);
    int (*deliverAssDataNavMdl)(int sid, TCgAgpsEphemerisCollection_t *pEphemerisCollection);
    int (*deliverAssDataAcquisAssist)(int sid, TCgAgpsAcquisAssist_t *pAcquisAssist);
    int (*deliverLPPeAssData)(int sid, LPPeProvAssData_t *pAssData);
    int (*deliverAssDataEnd)(int sid);

    /* AGPS deliver position received from network */
    int (*deliverMSAPosition)(int             sid,
                              TCgAgpsPosition *msaPosition,
                              TcgVelocity     *agpsVelocity
                              );
    int (*deliverLocOfAnotherSet)(int                 sid,
                                  TCgAgpsPosition     *msaPosition,
                                  TcgVelocity         *agpsVelocity
                                  );

    /* AGPS deliver GEO fence related parameters */
    void (*deliverAreaEventParam)(int                     sid,
                                  TcgAreaEventParams_t    *pParam,
                                  TcgAreaIdLists_t        *pLists
                                  );

    /* AGPS send several notifications */
    void (*notifyAgpsSessionEnd)(int sid);
    void (*readyToRecvPosInfo)(int sid, char rspType);
    int (*readyToRecvHistoricReport)(int                   sid,
                                     CgHistoricReporting_t *histRpt,
                                     TCgSetQoP_t           *qop
                                     );
    void (*showPositionInfo)(int sid);
    int (*getNeighborLocationIdsInfo)(AgnssNeighborLocationIdsList_t *pNeighborLocationIdsList);
    int (*getHplmnInfo)(CgHplmnInfo_t *pHplmnInfo);
    int (*deliverGNSSAssistData)(int sid, GNSSAssistData_t *pAssData);
    int (*gnssReadyToRecvPosInfo)(int sid, char rspType, GNSSPosReq_t  *pPosReq);
    int (*deliverAssDataIonoModel)(int sid, GNSSIonosphericModel_t *pIonoModel);
    int (*deliverAssDataAlmanac)(int sid, GNSSAlmanacData_t *pAlmanac);
}AgpsUpCallbacks_t;

/* AGPS Control Plane Callbacks which should be provided by libgps */
typedef struct _AgpsCpCallbacks_t
{
    unsigned int size;  //size of this struct

    int (*closePhoneItf)(void);
    int (*registerWAPandSMS)(AgnssNiNotification_t *notification);

    /* AGPS get SET ID */
    CG_SETId_PR   (*getSETIdType)(void);
    int (*getIMSI)(unsigned char* pBuf, unsigned short usLenBuf);
    int (*getMSISDN)(unsigned char* pBuf, unsigned short usLenBuf);
    int (*getIPAddress)(CgIPAddress_t *ipAddress);
    
    /* AGPS get wireless network information */
    int (*getLocationId)(CgLocationId_t *pLocationId);
    int (*getWlanApInfo)(AgnssWlanCellList_t *pCgWlanCellList);
    int (*getNeighborLocationIdsInfo)(AgnssNeighborLocationIdsList_t *pNeighborLocationIdsList);

    /* AGPS get GEO position of the SET */
    int (*getPosition)(TcgTimeStamp   *pTime, 
                       GNSSLocationCoordinates_t *pLastPos, 
                       GNSSVelocity_t *pVelocity);
    /* AGPS get positioning measurement results for MSA */
    int (*getMsrResults)(int sid,
                         TcgAgpsMsrSetList_t *pMsrSetList,
                         GNSSVelocity_t *pVelocity
                         );

    /* AGPS get TLS-PSK related keys */
    BOOL (*getPskForTls)(TcgGbaOctetStr_t *pKs,     //OUT: Ks of the specific IMSI
                         TcgGbaOctetStr_t *pBtid,   //OUT: B-TID of the specific IMSI
                         TcgGbaOctetStr_t *pRand,   //OUT:
                         TcgGbaOctetStr_t *pImpi    //OUT:
                         );

    /* AGPS deliver assistance data */
    int (*deliverAssDataRefTime)(int sid, TCgRxNTime *pTime);
    int (*deliverAssDataRefLoc)(int sid, void *pPos, TCgRxNTime *pTime);
    int (*deliverAssDataNavMdl)(int sid, TCgAgpsEphemerisCollection_t *pEphemerisCollection);
    int (*deliverAssDataAcquisAssist)(int sid, TCgAgpsAcquisAssist_t *pAcquisAssist);
    int (*deliverLPPeAssData)(int sid, LPPeProvAssData_t *pAssData);
    int (*deliverAssDataEnd)(int sid);

    /* AGPS deliver position received from network */
    int (*deliverMSAPosition)(int             sid,
                              TCgAgpsPosition *msaPosition,
                              TcgVelocity     *agpsVelocity
                              );
    int (*deliverLocOfAnotherSet)(int             sid,
                                  TCgAgpsPosition *msaPosition,
                                  TcgVelocity     *agpsVelocity
                                  );

    /* AGPS deliver GEO fence related parameters */
    void (*deliverAreaEventParam)(int                    sid,
                                  TcgAreaEventParams_t   *pParam,
                                  TcgAreaIdLists_t       *pLists
                                  );

    /* AGPS send several notifications */
    void (*notifyAgpsSessionEnd)(int sid);
    void (*readyToRecvPosInfo)(int sid, char rspType);
    int (*readyToRecvHistoricReport)(int                     sid,
                                     CgHistoricReporting_t  *histRpt,
                                     TCgSetQoP_t            *qop
                                     );
    void (*showPositionInfo)(int sid);

    /* AGPS control plane related callbacks */
    int (*cpSendUlMsg)(agps_cp_ul_pdu_params_t *pParams);
    int (*cpSendVerificationRsp)(unsigned long notifId,
                                 unsigned long present, 
                                 unsigned long verificationResponse
                                 );
    int (*cpMoLocationReq)(agps_cp_molr_params_t *pParam);

    /* AGPS send MOLR Response to upper layer
       errorNum   --- 0 - success, otherwise indicating the error number of SS service
       pPosition  --- received position from network, if not received, set to NULL
       pVelocity  --- received velocity from network, if not received, set to NULL. */
    int (*cpMOLRRsp)(int errorNum,
                     TCgAgpsPosition *pPosition, 
                     TcgVelocity  *pVelocity);
    int (*cpLcsPos)(agps_cp_lcs_pos_params_t *pParam);

    /* AGPS send CPOS Response to upper layer */
    int (*cpLcsPosRsp)(agps_cp_pos_rsp_params_t *pRspParam);

    /* GNSS supported callbacks*/
    int (*deliverGNSSAssistData)(int sid, GNSSAssistData_t *pAssData);
    int (*gnssReadyToRecvPosInfo)(int sid, char rspType, GNSSPosReq_t  *pPosReq);
    int (*deliverAssDataIonoModel)(int sid, GNSSIonosphericModel_t *pIonoModel);
    int (*deliverAssDataAlmanac)(int sid, GNSSAlmanacData_t *pAlmanac);
}AgpsCpCallbacks_t;

#ifdef AGPS_UP_SUPPORT  /*AGPS User Plane Support Macro*/

/******************************************************************************/
// Description: AGPS API, used to set function pointers of user plane callbacks
// Global resource dependence:
// Author: Andy.Huang
// Note: return 0 - successful, otherwise failed
/******************************************************************************/
int agpsapi_setAgpsUpCallbacks(AgpsUpCallbacks_t *pCallbacks);

signed int RXN_startSISession(int sid, unsigned short flagAssistanceRequired, TCgSetQoP_t *pQoP);

signed int RXN_gotSUPLINIT(int sid, unsigned char *pSUPLINIT_Data, unsigned short SUPLINIT_Len);

signed int RXN_chgVersion(unsigned char Maj, unsigned char Min, unsigned char ServInd);

signed int RXN_setH_SLP(const char *pFQDN_H_SLP, CgIPAddress_t *pIpOfSlp, unsigned short usPort);

signed int RXN_setHslpFromSim(unsigned char *pHslpFromSim);

signed int RXN_startSUPLClient(unsigned short usCtrlPort);

signed int RXN_stopSUPLClient(void);

signed int RXN_cfgSetCapability(CgSetCapability_t *pSetCapability);

int RXN_gpsMeasureInfo(int sid, TcgAgpsMsrSetList_t *pMsrSetList, GNSSVelocity_t *pVelocity);

int RXN_locationInfo(int sid, GNSSPosition_t *pAgpsPos, GNSSVelocity_t *pVelocity);

int RXN_setPosMode(eCG_PosMethod aPosMethod);

int RXN_niResponse(int sid, int userResponse);

int RXN_waitForSUPLMsgHandlerReady(unsigned short timeoutMsec);

/******************************************************************************/
// Description: Application initiates periodic session 
// Global resource dependence:
// Author: Xiaonian.Li
// Note: 
/******************************************************************************/
signed int RXN_startSIPeriodicSession(
                                      int                  sid,
                                      unsigned short       flagAssistanceRequired,
                                      TCgSetQoP_t          *pQoP,
                                      unsigned long        startTime,
                                      unsigned long        stopTime,
                                      unsigned long        intervalTime,
                                      CgThirdPartyList_t   *pTthirdPartyList   // pTthirdParty != NULL, 
                                      );

/******************************************************************************/
// Description: User starts SI session and transfers location to the 3rd party
// Global resource dependence:
// Author: Xiaonian.Li
// Note:
/******************************************************************************/
signed int RXN_startSISessionTransferTo3rdParty(
                                                int                  sid,
                                                unsigned short       flagAssistanceRequired,
                                                TCgSetQoP_t          *pQoP,
                                                CgThirdPartyList_t   *pTthirdPartyList
                                                );
                                                
/******************************************************************************/
// Description: Application initiates periodic session 
// Global resource dependence:
// Author: Xiaonian.Li
// Note: 
/******************************************************************************/
signed int RXN_cancelTriggerSession(int sid);

/******************************************************************************/
// Description: libgps informs SUPL: the status of PS(packet service) 
// Global resource dependence:
// Author: Xiaonian.Li
// Note: 
/******************************************************************************/
void RXN_psStatus(BOOL psIsConnected);  //LIBGPS_psStatus_CMD

/******************************************************************************/
// Description: User starts SI Area Event Trigger Session
// Global resource dependence:
// Author: Andy.Huang
// Note:
/******************************************************************************/
signed int RXN_startSIAreaEventSession(
                                       int                  sid,
                                       unsigned short       flagAssistanceRequired,
                                       TCgSetQoP_t          *pQoP,
                                       TcgAreaEventParams_t *pParam
                                       );

/******************************************************************************/
// Description: User request to locate another SET
// Global resource dependence:
// Author: Haoming.Lu
// Note: SET_ID is mandatory present for each session, 
//       QoP and App ID are optional.
/******************************************************************************/
signed int RXN_startLocReqOfAnotherSET(
                                       int                  sid,
                                       CgSETId_t            *pSET_Id,
                                       TCgSetQoP_t          *pQoP,
                                       CgApplicationId_t    *pApplicationId
                                       );

/******************************************************************************/
// Description: libgps notify us when network changed
// Global resource dependence:
// Author: Andy.Huang
// Note: 
/******************************************************************************/
int RXN_notifyNetworkChanged(CgLocationId_t *pLoc);

/******************************************************************************/
// Description: Historic reporting retrieval: GPS returns historical report
// after returned, please free any allocated memory
// Global resource dependence:
// Author: Haoming.Lu
// Note: none.
/******************************************************************************/
signed int RXN_histReporting(int sid, GNSSReportDataList_t *pReportDataList);

/******************************************************************************/
// Description: request to get position in MSA mode.
//              when supl received this request, it will send SUPL-POS-INIT, and 
//              then exchange several SUPL-POS msgs to get position from network
// Global resource dependence:
// Author: Andy.Huang
// Note: 
// Return value: 
/******************************************************************************/
int RXN_requestToGetMsaPosition(int sid, unsigned char isFirstTime);

/******************************************************************************/
// Description: when the area event trigger condition has met, the engine using 
//              this API to inform supl
// Global resource dependence:
// Author: Andy.Huang
// Note: 
// Return value: 
/******************************************************************************/
int RXN_informAreaEventTriggered(int sid, GNSSPositionParam_t *pPos);

/******************************************************************************/
// Description: engine using this API to end the area event session
// Global resource dependence:
// Author: Andy.Huang
// Note: 
// Return value: 
/******************************************************************************/
int RXN_endAreaEventSession(int sid);

/******************************************************************************/
// Description: libgps using this API to set GNSS capability
// Global resource dependence:
// Author: zhenyu.tong
// Note: 
// Return value: 
/******************************************************************************/
int SPRD_setGnssCapability (GNSSCapability_t  *pCap);

/******************************************************************************/
// Description: libgps using this API to start SI session using GNSS
// Global resource dependence:
// Author: zhenyu.tong
// Note: 
// Return value: 
/******************************************************************************/
int SPRD_startSISessionForGnss(int                  sid, 
                               GNSSReqAssData_t    *pReqAssData,
                               TCgSetQoP_t         *pQoP,
                               CgThirdPartyList_t  *pThirdPartyList);

/******************************************************************************/
// Description: libgps using this API to start SI periodic session using GNSS
// Global resource dependence:
// Author: zhenyu.tong
// Note: 
// Return value: 
/******************************************************************************/
int SPRD_startSIPeriodicSessionForGnss(int                  sid,
                                       GNSSReqAssData_t    *pReqAssData, 
                                       TCgSetQoP_t          *pQoP,
                                       unsigned long        startTime, 
                                       unsigned long        stopTime,  
                                       unsigned long        intervalTime,
                                       CgThirdPartyList_t   *pThirdPartyList);
/******************************************************************************/
// Description: libgps using this API to start SI Area event session using GNSS
// Global resource dependence:
// Author: zhenyu.tong
// Note: 
// Return value: 
/******************************************************************************/
int SPRD_startSIAreaEventSessionForGnss(int                   sid,
                                        GNSSReqAssData_t     *pReqAssData, 
                                        TCgSetQoP_t          *pQoP,
                                        TcgAreaEventParams_t *pParam);
/******************************************************************************/
// Description: libgps using this API to response  MSB measurement info of GNSS 
// Global resource dependence:
// Author: zhenyu.tong
// Note: 
// Return value: 
/******************************************************************************/
int SPRD_gnssLocationInfo(int sid, GNSSPosition_t *pPos, GNSSVelocity_t *pVelocity);

/******************************************************************************/
// Description: libgps using this API to response  MSA measurement info of GNSS
// Global resource dependence:
// Author: zhenyu.tong
// Note: 
// Return value: 
/******************************************************************************/
int SPRD_gnssMeasureInfo(int sid, GNSSMsrSetList_t *pMsrSetList, GNSSVelocity_t *pVelocity);

/******************************************************************************/
// Description: set local ip addr for a specified apn
// Global resource dependence:
// Author: Haoming.Lu
// Note: 
// Return value: 
/******************************************************************************/
int SPRD_setLocalIpAddrForApn(CgIPAddress_t *pLocalIpAddr, unsigned short port);
#endif //AGPS_UP_SUPPORT



#ifdef AGPS_CP_SUPPORT  /*AGPS Control Plane Support Macro*/

/******************************************************************************/
// Description: AGPS API, used to set function pointers of control plane callbacks
// Global resource dependence:
// Author: Andy.Huang
// Note: return 0 - successful, otherwise failed
/******************************************************************************/
int agpsapi_setAgpsCpCallbacks(AgpsCpCallbacks_t *pCallbacks);

/******************************************************************************/
// Description: Enable Ctrl Plane task
// Global resource dependence:
// Author: Tianming.Wu
// Note:
/******************************************************************************/
extern int ctrlplane_task_enable(void);

/******************************************************************************/
// Description: disable ctrl plane task
// Global resource dependence:
// Author: Tianming.Wu
// Note:
/******************************************************************************/
extern void ctrlplane_task_disable(void);

/******************************************************************************/
// Description: check whether Ctrl Plane thread is running
// Global resource dependence:
// Author: Tianming.Wu
// Note:
/******************************************************************************/
extern int ctrlplane_RXN_waitForCtrlPlane_HandlerReady(unsigned short timeoutMsec);

/******************************************************************************/
// Description: libgps config Set Capability before positioning
// Global resource dependence:
// Author: Tianming.Wu
// Note:
/******************************************************************************/
extern int ctrlplane_RXN_cfgSetCapability(CgSetCapability_t *pSetCapability);

/******************************************************************************/
// Description: Set postion mode
// Global resource dependence:
// Author: Tianming.Wu
// Note:
/******************************************************************************/
extern int ctrlplane_RXN_setPosMode(eCG_PosMethod aPosMethod);

/******************************************************************************/
// Description: User starts SI session
// Global resource dependence:
// Author: Tianming.Wu
// Note: 
// return 0 - success, otherwise failed
/******************************************************************************/
extern int ctrlplane_RXN_startSISession(
                                       CpSiLocReqType_e   eLocReqType,            //IN:
                                       unsigned short     flagAssistanceRequired, //IN:
                                       TCgSetQoP_t        *pQoP,                  //IN:
                                       CpThirdPartyAddr_t *pAddrStr               //IN: ISDN Addr string
                                       );

/******************************************************************************/
// Description: User response for NI session, accepts or rejects NI session
// Global resource dependence:
// Author: Tianming.Wu
// Note:
/******************************************************************************/
extern int ctrlplane_RXN_niResponse(int sid, int userResponse);

/******************************************************************************/
// Description: Engine responses (NI session, MSB) currently position
// Global resource dependence:
// Author: Tianming.Wu
// Note:add a gpsTOW parameter in 2014-09-12 by andy.an
/******************************************************************************/
extern int ctrlplane_RXN_locationInfo(
                                     int sid, 
                                     GNSSPosition_t *pPos,
                                     GNSSVelocity_t *pVelocity, 
                                     long gpsTOW
                                     );

/******************************************************************************/
// Description: Engine responses (NI/SI session, MSA) currently measure info
// Global resource dependence:
// Author: Tianming.Wu
// Note:
/******************************************************************************/
extern int ctrlplane_RXN_gpsMeasureInfo(
                                       int sid, 
                                       TcgAgpsMsrSetList_t *pMsrSetList, 
                                       GNSSVelocity_t *pVelocity
                                       );

/******************************************************************************/
// Description: APP request to locate another SET
// Global resource dependence:
// Author: Tianming.Wu
// Note:
/******************************************************************************/
extern int ctrlplane_RXN_startLocReqOfAnotherSET(CgSETId_t *SET_Id, 
                                                 TCgSetQoP_t *pQoP, 
                                                 CgApplicationId_t *pApplicationId
                                                 );

/******************************************************************************/
// Description: ctrlplane, end MOLR session
// Global resource dependence:
// Author: Andy.Huang
// Note:
/******************************************************************************/
extern int ctrlplane_RXN_stopSiSessionReq(int sid);

/******************************************************************************/
// Description: ctrlplane, set GNSS capability
// Global resource dependence:
// Author: Andy.Huang
// Note:
/******************************************************************************/
int ctrlplane_SPRD_setGnssCapability(GNSSCapability_t *pCap);

/******************************************************************************/
// Description: ctrlplane, start SI session using GNSS
// Global resource dependence:
// Author: Andy.Huang
// Note:
/******************************************************************************/
int ctrlplane_SPRD_startSISessionForGnss(
    int                 sid, 
    CpSiLocReqType_e    eLocReqType,
    GNSSReqAssData_t   *pGnssReqAssData,
    TCgSetQoP_t        *pQoP,
    CpThirdPartyAddr_t *pAddrStr
);

/******************************************************************************/
// Description: ctrlplane, response MSB measurement info of GNSS 
// Global resource dependence:
// Author: Andy.Huang
// Note:
/******************************************************************************/
int ctrlplane_SPRD_gnssLocationInfo(
    int             sid, 
    GNSSPosition_t *pPos, 
    GNSSVelocity_t *pVelocity,
    long            gpsTOW
);

/******************************************************************************/
// Description: ctrlplane, response MSA measurement info of GNSS
// Global resource dependence:
// Author: Andy.Huang
// Note:
/******************************************************************************/
int ctrlplane_SPRD_gnssMeasureInfo(
    int               sid, 
    GNSSMsrSetList_t *pMsrSetList, 
    GNSSVelocity_t   *pVelocity
);

/******************************************************************************/
// Description: Handle Downlink Data payload, Assistant Data and Measure Req
// Global resource dependence:
// Author: Tianming.Wu
// Note:
/******************************************************************************/
extern int ctrlplane_handleDlData(agps_cp_dl_pdu_params_t *pParams);

/******************************************************************************/
// Description: Handle Stop Session, User stop or Last AGPS message
// Global resource dependence:
// Author: Tianming.Wu
// Note:
/******************************************************************************/
extern int ctrlplane_handleStopSession(void);

/******************************************************************************/
// Description: Handle RRC State Change
// Global resource dependence:
// Author: Tianming.Wu
// Note:
/******************************************************************************/
extern int ctrlplane_handleRrcStateChange(unsigned int rrc_state);

/******************************************************************************/
// Description: Handle Start NI Session
// Global resource dependence:
// Author: Tianming.Wu
// Note:
/******************************************************************************/
extern int ctrlplane_handleStartNiSession(void);

/******************************************************************************/
// Description: Handle to reset UE positioning stored information
// Global resource dependence:
// Author: Tianming.Wu
// Note:
/******************************************************************************/
extern int ctrlplane_handleResetPosInfo(short type);

/******************************************************************************/
// Description: Handle Notification or/and Varification to User
// Global resource dependence:
// Author: Tianming.Wu
// Note:
/******************************************************************************/
extern int ctrlplane_handleNotifyAndVerification(agps_cp_notify_params_t agps_user_notify_params);

/******************************************************************************/
// Description: Handle MOLR response
// Global resource dependence:
// Author: Andy.Huang
// Note:
/******************************************************************************/
extern int ctrlplane_handleMolrResponse(
                                       unsigned int locEstlen,    //IN: length of location estimate data
                                       unsigned char *pLocEstData //IN: pointer to location estimate data
                                       );

/******************************************************************************/
// Description: Handle CPOS response
// Global resource dependence:
// Author: Andy.Huang
// Note:
/******************************************************************************/
extern int ctrlplane_handleAgpsCposResponse(CpPosEventType_e  evtType, 
                                            int len,
                                            char *rsp_param);
											
#endif //AGPS_CP_SUPPORT

#endif//GNSS_LCS 

#ifdef __cplusplus
}
#endif

#endif
