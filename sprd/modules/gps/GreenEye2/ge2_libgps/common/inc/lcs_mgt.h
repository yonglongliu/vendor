/******************************************************************************
** File Name:  lcs_mgt.h                                                      *
** Author:                                                                    *
** Date:                                                                      *
** Copyright:  2001-2020 by Spreadtrum Communications, Inc.                   * 
**             All rights reserved.                                           *
**             This software is supplied under the terms of a license         *
**             agreement or non-disclosure agreement with Spreadtrum.         *
**             Passing on and copying of this document, and communication     *
**             of its contents is not permitted without prior written         *
**             authorization.                                                 *
**description: INTERFACES BETWEEN MODULE LIBGPS & LCS(protocol)               *
*******************************************************************************

*******************************************************************************
**                                Edit History                                *
**----------------------------------------------------------------------------*
** Date                  Name                      Description                *
**----------------------------------------------------------------------------*
** 2016-01-01            Xu.Wang                   Create.                    *
******************************************************************************/
#ifndef _LCS_MGT_H_
#define _LCS_MGT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "lcs_type.h"

/*---------------------------------------------*
**                   MACROS                    *
**---------------------------------------------*/
/* default sid value */
#define LCS_SID_ANY              0xFF

/* domain bitmap */
#define LCS_DOMAIN_NONE          0x00
#define LCS_DOMAIN_UP            0x01
#define LCS_DOMAIN_CP            0x02
typedef unsigned int LcsDomain_bm; 

#define LCS_DOMAIN_NAME(dom)    \
          ((LCS_DOMAIN_UP == (dom)) ? "UP" \
           :((LCS_DOMAIN_CP == (dom)) ? "CP" \
             :(((LCS_DOMAIN_UP|LCS_DOMAIN_CP) == (dom)) ? "UP|CP" \
               : "UNDEF")\
            )\
          )  

/* the MAXNUM could be changed in FUTURE */
#define LCSGNSS_INSTR_CLASS_MAXNUM      2
#define LCSGNSS_INSTR_CODE_MAXNUM       32

#define LCSGNSS_INSTR_FLAG_MASK         0xE000
#define LCSGNSS_INSTR_CLASS_MASK        0xFF00
#define LCSGNSS_INSTR_CODE_MASK         0x00FF

#define GNSS_INSTR_CLASS_MASK           GNSS_INSTR_BEGIN
#define GNSS_INSTR_CODE_MASK            LCSGNSS_INSTR_CODE_MASK

#define LCS_INSTR_CLASS_MASK            LCS_INSTR_BEGIN
#define LCS_INSTR_CODE_MASK             LCSGNSS_INSTR_CODE_MASK

#define LCSGNSS_INSTR_GET_CLASS(instr)      \
        ((((instr)^LCSGNSS_INSTR_FLAG_MASK) & LCSGNSS_INSTR_CLASS_MASK) >> 8)    
#define LCSGNSS_INSTR_GET_CODE(instr)       \
        ((instr) & LCSGNSS_INSTR_CODE_MASK)    

#define LCSGNSS_INSTR_IS_CLASS_VALID(clas)  \
        (((clas) < LCSGNSS_INSTR_CLASS_MAXNUM) ? TRUE : FALSE)
#define LCSGNSS_INSTR_IS_CODE_VALID(code)   \
        (((code) < LCSGNSS_INSTR_CODE_MAXNUM) ? TRUE : FALSE)

/*---------------------------------------------*
**               CONST VALUES                  *
**---------------------------------------------*/
typedef enum
{
    /******************************************
    ** rule of naming: DSTMODULE_INSTR_XXXX  **
    ** rule of value : 0xEMNN                **
    **      0xEM-class(E-flag), NN-code      **
    *******************************************/

    /* RESERVED: 0x0000~0xDFFF */

/* >>>>> GNSS INSTRUCTIONS(CALLBACK) BEGIN >>>>> */
    GNSS_INSTR_BEGIN = 0xE000,                         /* 0xE000 */  

    GNSS_INSTR_START,
    GNSS_INSTR_RESET_AD,
    GNSS_INSTR_NI_NOTIF,
    GNSS_INSTR_GET_SETID,
    GNSS_INSTR_GET_IMSI,
    GNSS_INSTR_GET_MSISDN,
    GNSS_INSTR_GET_HPLMN,
    GNSS_INSTR_GET_IPADDR,
    GNSS_INSTR_GET_WLANCELL,
    GNSS_INSTR_GET_REFLOC,
    GNSS_INSTR_GET_REFNLOC,
    GNSS_INSTR_GET_TLSPSK,
    GNSS_INSTR_GET_MEAS,
    GNSS_INSTR_GET_POS,
    GNSS_INSTR_SHOW_POS_PD,
    GNSS_INSTR_NOTIF_SESSN_END,
    GNSS_INSTR_DLVR_MSA_POS,
    GNSS_INSTR_DLVR_POS_RESP,
    GNSS_INSTR_DLVR_AREA_EVENT,
    GNSS_INSTR_DLVR_IONO_MODEL,
    GNSS_INSTR_DLVR_ALMANAC,
    GNSS_INSTR_DLVR_AD_GNSS,
    GNSS_INSTR_DLVR_AD_LPPE,
    GNSS_INSTR_DLVR_AD_END,
    GNSS_INSTR_READY_RECV_MEAS_POS,
    GNSS_INSTR_READY_RECV_HIST_REPORT,

    GNSS_INSTR_END   = GNSS_INSTR_BEGIN + 0x00FF,      /* 0xE0FF */  
/* <<<<< GNSS INSTRUCTIONS(CALLBACK) END <<<<< */

/* >>>>>>> LCS INSTRUCTIONS(API) BEGIN >>>>>>> */
    LCS_INSTR_BEGIN = GNSS_INSTR_END + 1,              /* 0xE100 */  

    LCS_INSTR_START_CLIENT,
    LCS_INSTR_STOP_CLIENT,
    LCS_INSTR_WAIT_CLIENT_READY,
    LCS_INSTR_SET_VERSION,
    LCS_INSTR_SET_HSLP,
    LCS_INSTR_SET_LOCAL_IP_PORT,
    LCS_INSTR_SET_CAPABILITY,
    LCS_INSTR_SET_POS_METHOD,
    LCS_INSTR_NOTIF_PS_STATUS,
    LCS_INSTR_NOTIF_NW_CHG,
    LCS_INSTR_RECV_SUPL_INIT,
    LCS_INSTR_REQ_MSA_POS,
    LCS_INSTR_INJCT_USER_RESP,
    LCS_INSTR_INJCT_MEAS,
    LCS_INSTR_INJCT_POS,
    LCS_INSTR_INJCT_HIST_REPORT,
    LCS_INSTR_START_SI_SESSN,
    LCS_INSTR_CANCL_SI_SESSN,
    LCS_INSTR_STOP_SI_SESSN,
    LCS_INSTR_START_SI_SESSN_PD,
    LCS_INSTR_STOP_SI_SESSN_PD,
    LCS_INSTR_START_SI_SESSN_AREA,
    LCS_INSTR_TRIG_AREA_EVENT,
    LCS_INSTR_STOP_SI_SESSN_AREA,
    LCS_INSTR_START_SI_SESSN_TO_3PARTY,
    LCS_INSTR_START_POS_REQ_OF_3PARTY,
    LCS_INSTR_CTRL_NWLR,
    
    LCS_INSTR_END   = LCS_INSTR_BEGIN + 0x00FF,        /* 0xE1FF */  
/* <<<<<<< LCS INSTRUCTIONS(API) END <<<<<<<<<< */

    LCSGNSS_INSTR_INVALID = (LCS_INSTR_END + 1)
}LcsGnssInstr_e;

/*---------------------------------------------*
**               DATA TYPES                    *
**---------------------------------------------*/
typedef struct 
{
    LcsDomain_bm      domain; 
    unsigned int      sid; 
    LcsGnssInstr_e    instr; 
    unsigned int      argsLen;   /* sizeof (Lcs/GnssArgsXxx_t) */
    unsigned char    *ptArgs;
}LcsGnssInstrHeader_t;

typedef struct
{
    unsigned short    dataLen;
    unsigned char    *ptData;
}LcsGnssUnivData_t;

/*--------------------- callback definition BEGIN ----------------------------*/
//GNSS_INSTR_START
#define GNSS_ARGS_SIZE_START     sizeof(GnssArgsStart_t)
typedef struct
{
    unsigned char  rsv[4];  /* None parameter  */
}GnssArgsStart_t;

//GNSS_INSTR_RESET_AD
#define GNSS_ARGS_SIZE_RESET_AD     sizeof(GnssArgsResetAd_t)
typedef struct
{
    unsigned short  flags;
    unsigned char   rsv[2];
}GnssArgsResetAd_t;

//GNSS_INSTR_NI_NOTIF
#define GNSS_ARGS_SIZE_NI_NOTIF     sizeof(GnssArgsNiNotif_t)
typedef AgnssNiNotification_t   GnssArgsNiNotif_t;

//GNSS_INSTR_GET_SETID
#define GNSS_ARGS_SIZE_GET_SETID     sizeof(GnssArgsGetSetId_t)
typedef struct
{
    CG_SETId_PR  id;
}GnssArgsGetSetId_t;

//GNSS_INSTR_GET_IMSI
#define GNSS_ARGS_SIZE_GET_IMSI     sizeof(GnssArgsGetImsi_t)
typedef LcsGnssUnivData_t     GnssArgsGetImsi_t;

//GNSS_INSTR_GET_MSISDN
#define GNSS_ARGS_SIZE_GET_MSISDN     sizeof(GnssArgsGetMsisdn_t)
typedef LcsGnssUnivData_t     GnssArgsGetMsisdn_t;

//GNSS_INSTR_GET_HPLMN
#define GNSS_ARGS_SIZE_GET_HPLMN     sizeof(GnssArgsGetHplmn_t)
typedef CgHplmnInfo_t         GnssArgsGetHplmn_t;

//GNSS_INSTR_GET_IPADDR
#define GNSS_ARGS_SIZE_GET_IPADDR     sizeof(GnssArgsGetIpAddr_t)
typedef CgIPAddress_t         GnssArgsGetIpAddr_t;

//GNSS_INSTR_GET_WLANCELL
#define GNSS_ARGS_SIZE_GET_WLANCELL     sizeof(GnssArgsGetWlanCell_t)
typedef AgnssWlanCellList_t   GnssArgsGetWlanCell_t;

//GNSS_INSTR_GET_REFLOC
#define GNSS_ARGS_SIZE_GET_REFLOC     sizeof(GnssArgsGetRefLoc_t)
typedef CgLocationId_t   GnssArgsGetRefLoc_t;

//GNSS_INSTR_GET_REFNLOC
#define GNSS_ARGS_SIZE_GET_REFNLOC     sizeof(GnssArgsGetRefNLoc_t)
typedef AgnssNeighborLocationIdsList_t   GnssArgsGetRefNLoc_t;

//GNSS_INSTR_GET_TLSPSK 
#define GNSS_ARGS_SIZE_GET_TLSPSK     sizeof(GnssArgsGetTlsPsk_t)
typedef struct
{
    TcgGbaOctetStr_t     *ptKs;     
    TcgGbaOctetStr_t     *ptBtid;   
    TcgGbaOctetStr_t     *ptRand;   
    TcgGbaOctetStr_t     *ptImpi;  
}GnssArgsGetTlsPsk_t;

//GNSS_INSTR_GET_MEAS
#define GNSS_ARGS_SIZE_GET_MEAS     sizeof(GnssArgsGetMeas_t)
typedef struct
{
    GNSSMsrSetList_t      *ptMsrSets;
    GNSSVelocity_t        *ptVelocity;
}GnssArgsGetMeas_t;

//GNSS_INSTR_GET_POS
#define GNSS_ARGS_SIZE_GET_POS     sizeof(GnssArgsGetPos_t)
typedef struct
{
    TcgTimeStamp               *ptTime;
    GNSSPosition_t  *ptPosition;
    GNSSVelocity_t             *ptVelocity;
}GnssArgsGetPos_t;

//GNSS_INSTR_SHOW_POS_PD
#define GNSS_ARGS_SIZE_SHOW_POS_PD     sizeof(GnssArgsShowPosPd_t)
typedef struct
{
    char   rsv[4];
}GnssArgsShowPosPd_t;

//GNSS_INSTR_NOTIF_SESSN_END
#define GNSS_ARGS_SIZE_NOTIF_SESSN_END     sizeof(GnssArgsNotifSessnEnd_t)
typedef struct
{
    unsigned char   rsv[4];
}GnssArgsNotifSessnEnd_t;

//GNSS_INSTR_DLVR_MSA_POS
#define GNSS_ARGS_SIZE_DLVR_MSA_POS     sizeof(GnssArgsDlvrMsaPos_t)
typedef struct
{
    int               errCode;
    TCgAgpsPosition  *ptPosition; 
    TcgVelocity      *ptVelocity;
}GnssArgsDlvrMsaPos_t;

//GNSS_INSTR_DLVR_POS_RESP
#define GNSS_ARGS_SIZE_DLVR_POS_RESP     sizeof(GnssArgsDlvrPosResp_t)
typedef agps_cp_pos_rsp_params_t  GnssArgsDlvrPosResp_t;

//GNSS_INSTR_DLVR_AREA_EVENT
#define GNSS_ARGS_SIZE_DLVR_AREA_EVENT     sizeof(GnssArgsDlvrAreaEvent_t)
typedef struct
{
    TcgAreaEventParams_t    *ptEvent;
    TcgAreaIdLists_t        *ptIds;
}GnssArgsDlvrAreaEvent_t;

//GNSS_INSTR_DLVR_IONO_MODEL
#define GNSS_ARGS_SIZE_DLVR_IONO_MODEL     sizeof(GnssArgsDlvrIonoModel_t)
typedef GNSSIonosphericModel_t    GnssArgsDlvrIonoModel_t;

//GNSS_INSTR_DLVR_ALMANAC
#define GNSS_ARGS_SIZE_DLVR_ALMANAC     sizeof(GnssArgsDlvrAlmanac_t)
typedef GNSSAlmanacData_t    GnssArgsDlvrAlmanac_t;

//GNSS_INSTR_DLVR_AD_GNSS
#define GNSS_ARGS_SIZE_DLVR_AD_GNSS     sizeof(GnssArgsDlvrAdGnss_t)
typedef GNSSAssistData_t          GnssArgsDlvrAdGnss_t;

//GNSS_INSTR_DLVR_AD_LPPE
#define GNSS_ARGS_SIZE_DLVR_AD_LPPE     sizeof(GnssArgsDlvrAdLppe_t)
typedef LPPeProvAssData_t         GnssArgsDlvrAdLppe_t;

//GNSS_INSTR_DLVR_AD_END
#define GNSS_ARGS_SIZE_DLVR_AD_END     sizeof(GnssArgsDlvrAdEnd_t)
typedef struct
{
    unsigned char   rsv[4];
}GnssArgsDlvrAdEnd_t;

//GNSS_INSTR_READY_RECV_MEAS_POS
#define GNSS_ARGS_SIZE_READY_RECV_MEAS_POS     sizeof(GnssArgsReadyRecvMeasPos_t)
typedef struct
{
    unsigned char     rspType;
    GNSSPosReq_t     *ptReq;
}GnssArgsReadyRecvMeasPos_t;

//GNSS_INSTR_READY_RECV_HIST_REPORT 
#define GNSS_ARGS_SIZE_READY_RECV_HIST_REPORT     sizeof(GnssArgsReadyRecvHistReport_t)
typedef struct
{
    CgHistoricReporting_t  *ptReport;
    TCgSetQoP_t            *ptQop;
}GnssArgsReadyRecvHistReport_t;

/*----------------------- callback definition END ----------------------------*/

/*------------------------ API definition BEGIN ------------------------------*/
//LCS_INSTR_START_CLIENT
#define LCS_ARGS_SIZE_START_CLIENT     sizeof(LcsArgsStartClient_t)
typedef struct
{
    unsigned short    port; 
    unsigned char     rsv[2];
}LcsArgsStartClient_t;

//LCS_INSTR_STOP_CLIENT
#define LCS_ARGS_SIZE_STOP_CLIENT     sizeof(LcsArgsStopClient_t)
typedef struct
{
    unsigned char   rsv[4];
}LcsArgsStopClient_t;

//LCS_INSTR_WAIT_CLIENT_READY
#define LCS_ARGS_SIZE_WAIT_CLIENT_READY     sizeof(LcsArgsWaitClientReady_t)
typedef struct
{
    unsigned short    timeoutMsec; 
    unsigned char     rsv[2];
}LcsArgsWaitClientReady_t;

//LCS_INSTR_SET_VERSION
#define LCS_ARGS_SIZE_SET_VERSION     sizeof(LcsArgsSetVersion_t)
typedef struct
{
    unsigned char     maj;
    unsigned char     min;
    unsigned char     servInd;
    unsigned char     rsv;
}LcsArgsSetVersion_t;

//LCS_INSTR_SET_HSLP
#define LCS_ARGS_SIZE_SET_HSLP     sizeof(LcsArgsSetHslp_t)
typedef struct
{
    unsigned char    *ptFqdn;
    CgIPAddress_t    *ptIpOfSlp;
    unsigned short    port;
    unsigned char     rsv[2];
}LcsArgsSetHslp_t;

//LCS_INSTR_SET_LOCAL_IP_PORT
#define LCS_ARGS_SIZE_SET_LOCAL_IP_PORT sizeof(LcsArgsSetLocalIpPort_t)
typedef struct
{
    CgIPAddress_t  localIp;
    unsigned short port;
}LcsArgsSetLocalIpPort_t;

//LCS_INSTR_SET_CAPABILITY
#define LCS_ARGS_SIZE_SET_CAPABILITY     sizeof(LcsArgsSetCapability_t)
typedef GNSSCapability_t  LcsArgsSetCapability_t;

//LCS_INSTR_SET_POS_METHOD
#define LCS_ARGS_SIZE_SET_POS_METHOD     sizeof(LcsArgsSetPosMethod_t)
typedef struct
{
    eCG_PosMethod    posMethod; 
}LcsArgsSetPosMethod_t;

//LCS_INSTR_NOTIF_PS_STATUS
#define LCS_ARGS_SIZE_NOTIF_PS_STATUS     sizeof(LcsArgsNotifPsStatus_t)
typedef struct
{
    BOOL  isConnected;
}LcsArgsNotifPsStatus_t;

//LCS_INSTR_NOTIF_NW_CHG
#define LCS_ARGS_SIZE_NOTIF_NW_CHG     sizeof(LcsArgsNotifNwChg_t)
typedef struct
{
    CgLocationId_t   *ptLocId;
}LcsArgsNotifNwChg_t;

//LCS_INSTR_RECV_SUPL_INIT
#define LCS_ARGS_SIZE_RECV_SUPL_INIT     sizeof(LcsArgsRecvSuplInit_t)
typedef LcsGnssUnivData_t   LcsArgsRecvSuplInit_t;

//LCS_INSTR_REQ_MSA_POS
#define LCS_ARGS_SIZE_REQ_MSA_POS     sizeof(LcsArgsReqMsaPos_t)
typedef struct
{
    unsigned char isFirstTime;
    unsigned char rsv[3];
}LcsArgsReqMsaPos_t;

//LCS_INSTR_INJCT_USER_RESP
#define LCS_ARGS_SIZE_INJCT_USER_RESP     sizeof(LcsArgsInjctUserResp_t)
typedef struct
{
    int   response;
}LcsArgsInjctUserResp_t;

//LCS_INSTR_INJCT_MEAS
#define LCS_ARGS_SIZE_INJCT_MEAS     sizeof(LcsArgsInjctMeas_t)
typedef struct
{
    GNSSMsrSetList_t     *ptMsrSets;
    GNSSVelocity_t       *ptVelocity;
}LcsArgsInjctMeas_t;

//LCS_INSTR_INJCT_POS
#define LCS_ARGS_SIZE_INJCT_POS     sizeof(LcsArgsInjctPos_t)
typedef struct
{
    long                  gpsTOW;
    GNSSPosition_t       *ptPosition;
    GNSSVelocity_t       *ptVelocity;
}LcsArgsInjctPos_t;

//LCS_INSTR_INJCT_HIST_REPORT
#define LCS_ARGS_SIZE_INJCT_HIST_REPORT     sizeof(LcsArgsInjctHistReport_t)
typedef GNSSReportDataList_t     LcsArgsInjctHistReport_t;

/* LCS_INSTR_START_SI_SESSN 
 * LCS_INSTR_START_SI_SESSN_PD 
 * LCS_INSTR_START_SI_SESSN_AREA 
 * LCS_INSTR_START_SI_SESSN_TO_3PARTY
 */
#define LCS_ARGS_SIZE_START_SI_SESSN     sizeof(LcsArgsStartSiSessn_t)
typedef struct
{
    CpSiLocReqType_e      cpSiType;
    GNSSReqAssData_t     *ptReqAssData; 
    TCgSetQoP_t          *ptQoP;
    unsigned long         startTime;
    unsigned long         stopTime;
    unsigned long         intervalTime;
    TcgAreaEventParams_t *ptEvent;
    CgThirdPartyList_t   *pt3rdParties;
}LcsArgsStartSiSessn_t;

//LCS_INSTR_CANCL_SI_SESSN
#define LCS_ARGS_SIZE_CANCL_SI_SESSN     sizeof(LcsArgsCanclSiSessn_t)
typedef struct
{
    unsigned char   rsv[4];
}LcsArgsCanclSiSessn_t;

/* LCS_INSTR_STOP_SI_SESSN 
 * LCS_INSTR_STOP_SI_SESSN_AREA
 */
#define LCS_ARGS_SIZE_STOP_SI_SESSN     sizeof(LcsArgsStopSiSessn_t)
typedef struct
{
    unsigned char   rsv[4];
}LcsArgsStopSiSessn_t;

//LCS_INSTR_TRIG_AREA_EVENT
#define LCS_ARGS_SIZE_TRIG_AREA_EVENT     sizeof(LcsArgsTrigAreaEvent_t)
typedef struct
{
    GNSSPositionParam_t    *ptPosition;
}LcsArgsTrigAreaEvent_t;

//LCS_INSTR_START_POS_REQ_OF_3PARTY
#define LCS_ARGS_SIZE_START_POS_REQ_OF_3PARTY     sizeof(LcsArgsStartPosReqOf3Party_t)
typedef struct
{
    CgSETId_t            *ptSetId;
    TCgSetQoP_t          *ptQoP;
    CgApplicationId_t    *ptAppId;
}LcsArgsStartPosReqOf3Party_t;

//LCS_INSTR_CTRL_NWLR
#define LCS_ARGS_SIZE_CTRL_NWLR     sizeof(LcsArgsCtrlNwlr_t)
typedef struct
{
   LcsNwlrCtrlCmd_e    command;
   LcsNwlrMethod_e     method;
   LcsDomain_bm        domain;
   unsigned short      reportAmount;
   unsigned short      reportInterval;   /* msec */
}LcsArgsCtrlNwlr_t;

/*------------------------- API definition BEGIN -----------------------------*/
/* NOTE: 0: return SUCC, < 0: return FAIL, > 0: RESERVED! */
typedef int (*LcsGnss_ptFunc)(LcsGnssInstrHeader_t  *ptHeader);

typedef LcsGnss_ptFunc  Lcs_Callback;
typedef LcsGnss_ptFunc  Lcs_execFunc;

#define LCSMGT_DRVDATA_SIZE  sizeof(LcsMgtDrvData_t)
typedef struct
{
    size_t size;   /* sizeof(LcsMgtDrvData_t) */

    Lcs_Callback   lcs_cb;
}LcsMgtDrvData_t;

#define LCSMGT_INTERFACE_SIZE  sizeof(LcsMgtInterface_t)
typedef struct
{
    size_t  size;
    
    int   (*init)(LcsMgtDrvData_t *ptDrvData);
    void  (*exit)(void);

    Lcs_execFunc  exec;
}LcsMgtInterface_t;

/* for dlopen("liblcsmgt.so") & dlsym(fd, "LCSMGT_getInterface"), RECOMMENDED */
typedef const LcsMgtInterface_t* (*LCSMGT_INTFC_ENTRY)(void);

/*---------------------------------------------*
**               FUNCTIONS                     *
**---------------------------------------------*/
LCS_API const LcsMgtInterface_t*  LCSMGT_getInterface(void);

#ifdef __cplusplus
}
#endif

#endif 

