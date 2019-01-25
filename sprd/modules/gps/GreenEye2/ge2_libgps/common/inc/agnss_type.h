/******************************************************************************
** File Name:  agnss_type.h                                                    *
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
** 2015-01-22            zhenyu.tong                Create.                   *
******************************************************************************/

#ifndef _AGNSS_TYPE_H_
#define _AGNSS_TYPE_H_

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


#include <stdbool.h>


/*Max supported GNSS number excluding GPS,  according RRLP*/
#define MAX_GNSS_NUM  8 

#define MAX_NAV_MODEL_NUM   32

/*GNSS ID according to RRLP£¬0 is reserved*/
typedef unsigned char SUPL_GNSS_ID_T ;

#define  SUPL_GNSS_ID_GPS                1
#define  SUPL_GNSS_ID_GALILEO            2
#define  SUPL_GNSS_ID_SBAS               3
#define  SUPL_GNSS_ID_MODERNIZED_GPS     4
#define  SUPL_GNSS_ID_QZSS               5
#define  SUPL_GNSS_ID_GLONASS            6
#define  SUPL_GNSS_ID_BDS                7
#define  SUPL_GNSS_ID_MAX                8

typedef unsigned char SUPL_SBAS_ID_T;

#define  SBAS_ID_WAAS               0
#define  SBAS_ID_EGNOS              1
#define  SBAS_ID_MSAS               2
#define  SBAS_ID_GAGAN              3

/*bitmap for supporting modes*/
typedef unsigned char GNSS_POS_MODE_T;

#define  GNSS_POS_MODE_MSA          0x01
#define  GNSS_POS_MODE_MSB          0x02
#define  GNSS_POS_MODE_AUTO         0x04

/*GNSS signal ID*/
typedef unsigned char GNSS_SIGNAL_T;

#define  GNSS_SIGNAL_GPS_L1CA       0

 //different from RRLP
#define  GNSS_SIGNAL_GALILEO_E1     0
#define  GNSS_SIGNAL_GALILEO_E5a    1
#define  GNSS_SIGNAL_GALILEO_E5b    2
#define  GNSS_SIGNAL_GALILEO_E5aE5b 3
#define  GNSS_SIGNAL_GALILEO_E6     4

//different from LPP
#define  GNSS_SIGNAL_MOD_GPS_L1C    0
#define  GNSS_SIGNAL_MOD_GPS_L2C    1
#define  GNSS_SIGNAL_MOD_GPS_L5     2

#define  GNSS_SIGNAL_QZSS_L1CA      0
#define  GNSS_SIGNAL_QZSS_L1C       1
#define  GNSS_SIGNAL_QZSS_L2C       2
#define  GNSS_SIGNAL_QZSS_L5        3

#define  GNSS_SIGNAL_GLONASS_G1     0
#define  GNSS_SIGNAL_GLONASS_G2     1
#define  GNSS_SIGNAL_GLONASS_G3     2

#define  GNSS_SIGNAL_SBAS_L1        0

#define  GNSS_SIGNAL_BDS_B1I        0

#define MAX_SATELLITES_IN_SYSTEM    63 //modern GPS is 63

//define the gnss time offset ID
typedef unsigned short  GNSS_TIMEOFFSET_ID_T;

#define GNSS_TIMEOFFSET_ID_GPS              0
#define GNSS_TIMEOFFSET_ID_GALILEO          1
#define GNSS_TIMEOFFSET_ID_QZSS             2
#define GNSS_TIMEOFFSET_ID_GLONASS          3
#define GNSS_TIMEOFFSET_ID_BDS              4

#define LIGHT_SPEED                 299792458.0
#define GPS_FREQUENCY_L1            1575420000.0

typedef struct{
    SUPL_GNSS_ID_T   gnssID ; //
    SUPL_SBAS_ID_T   sbasID;    
    GNSS_POS_MODE_T  gnssPosMode; //bitmap,bit0:SET assit, bit 1:SET based, bit2 auto
    GNSS_SIGNAL_T    gnssSignal;  /*bitmap for supporting signals */
}GNSSPosMethod_t;

typedef  struct {
    unsigned char    gnssNum;     //supported GNSS number£¬excluding GPS
    GNSSPosMethod_t  posMethod[MAX_GNSS_NUM];//
}GNSSPosMethodList_t;

typedef struct {
    GNSSPosMethodList_t posMethodList;
}GNSSCapability_t;

/*Requested Assistance data*/

typedef struct{
    unsigned char  satID;// equal to (SV ID No -1)
}SatelliteInformation_t;

typedef struct {
    bool  orbitModelIDFlag;
    int   orbitModelID;     //0-7, corresponding to model 1 - model 8
    bool  clockModelIDFlag;
    int   clockModelID;     //0-7, corresponding to model 1 - model 8
    bool  utcModelIDFlag;
    int   utcModelID;       //0-7, corresponding to model 1 - model 8
    bool  almanacModelIDFlag;
    int   almanacModelID;   //0-7, corresponding to model 1 - model 8
} AdditionalDataChoicesReq_t;

typedef struct{
    int  validity;
}GNSSExtendedEphReq_t;

typedef struct{
    long  gnssDay;
    long  gnssTODHour;
} GNSSExtendedEphTime_t;

typedef struct{
    GNSSExtendedEphTime_t  beginTime;
    GNSSExtendedEphTime_t  endTime ;
}GNSSExtendedEphCheckReq_t;

typedef struct {
    unsigned char  satID;//0-63,interpreted as in table A.10.14 in rrlp
    unsigned long  iod;//0-1023
}SatListRelatedData_t;

typedef struct{
    GNSS_SIGNAL_T signal;  //bitmap indicated by GNSS ID
    int   dataBitInterval;
    bool  satInfoFlag;
    int   satNum;
    SatelliteInformation_t  satInfo[MAX_SATELLITES_IN_SYSTEM]; //optional
}DataBitAssistanceReq_t;

typedef struct{
    int  todMinute;
    DataBitAssistanceReq_t  dataBitAss;
} GNSSDataBitsAssReq_t;

typedef struct {
    unsigned short  weekorDay; //
    unsigned short  toe;
    unsigned short  t_toeLimit;
    bool  satListrelatedDataFlag;
    unsigned char   dataNum;//
    SatListRelatedData_t  relatedData[MAX_SATELLITES_IN_SYSTEM];//optional
} GNSSNavModelReq_t;

typedef  struct{
    SUPL_GNSS_ID_T  gnssID ;  //
    SUPL_SBAS_ID_T  sbasID;   //
    GNSS_SIGNAL_T   diffCorrection;//bitmap, GNSS_SIGNAL
    bool  realtimeIntegrity;   //
    bool  almanac; //
    bool  navModelFlag;//
    GNSSNavModelReq_t  navModel; //optional
    GNSS_TIMEOFFSET_ID_T  timeModelGNSStoGNSS;//bitmap
    bool  refMsrInfo; //
    bool  dataBitsAssFlag;//
    bool  utcModel;//
    bool  additionalDataChoicesFlag;
    GNSSDataBitsAssReq_t  dataBitsAss;
    AdditionalDataChoicesReq_t  additionalDataChoices;//optional
    bool  auxiliaryInfo;//
    bool  extendedEphFlag;//
    bool  extendedEphCheckFlag;//
    GNSSExtendedEphReq_t  extendedEph;
    GNSSExtendedEphCheckReq_t  extendedEphCheck;
    GNSS_SIGNAL_T  bdsDiffCorrection;//bitmap ,only  GANSS_ID =BDS
    bool  bdsGridModel;//
}GNSSGenericAssDataReq_t;

typedef struct{
    unsigned short gpsAssistanceRequired;//bitmap
    /*other  satellite  system  excluding GPS*/
    bool  refLocation;
    bool  commonAssFlag; 
    /* common Assist data flag, optional field */
    bool  refTime; //
    bool  ionosphericModel;//
    bool  additionalIonModelforDataID00;//
    bool  additionalIonModelforDataID01;//
    bool  additionalIonModelforDataID11;//
    bool  earthOrienationPara;//
    /*GNSS generic assist data, optional filed*/
    unsigned char  gnssNum;//
    GNSSGenericAssDataReq_t  genericAssData [MAX_GNSS_NUM];
} GNSSReqAssData_t;

/*according to the definition of RRLP A.3.2.9*/
typedef enum{
    GNSSTimeID_GPS,
    GNSSTimeID_QZSS,
    GNSSTimeID_GLONASS,
    GNSSTimeID_BDS,
    /*Galileo sys time definition is not included in RRLP,and 4-7 is reserved*/
    GNSSTimeID_GALILEO = 8
} GNSSTimeID_e;

typedef struct {
    long  gnssTOD;   //unit :ms
    GNSSTimeID_e   gnssTimeID;  //
    bool   gnssTODFracFlag;
    float   gnssTODFrac;//opt ,ns
    bool   gnssTODUncertaintyFlag;
    long   gnssTODUncertainty;//opt
}GNSSPosTime_t;

/* ref 23.032 */
typedef enum {
    GNSSLocationCoordinatesType_NOTHING,
    GNSSLocationCoordinatesType_ellipsoidPoint,
    GNSSLocationCoordinatesType_ellipsoidPointWithUncertaintyCircle,
    GNSSLocationCoordinatesType_ellipsoidPointWithUncertaintyEllipse,
    GNSSLocationCoordinatesType_polygon,
    GNSSLocationCoordinatesType_ellipsoidPointWithAltitude,
    GNSSLocationCoordinatesType_ellipsoidPointWithAltitudeAndUncertaintyEllipsoid,
    GNSSLocationCoordinatesType_ellipsoidArc,
}GNSSLocationCoordinatesType_e;

typedef enum {
    GNSSLatitudeSign_north = 0,
    GNSSLatitudeSign_south = 1
} GNSSLatitudeSign_e;

typedef enum {
    GNSSAltitudeDirection_height = 0,
    GNSSAltitudeDirection_depth  = 1
} GNSSAltitudeDirection_e;

typedef struct{
    GNSSLatitudeSign_e  latitudeSign;
    long  degreesLatitude;
    long  degreesLongitude;
}GNSSEllipsoidPoint_t;

/* the uncertainty for latitude and longitude r, expressed in metres, is mapped 
   to a number K, with the formula: r= C(((1+x)^K)-1), with C = 10 and x = 0.1 */
typedef struct{
    GNSSLatitudeSign_e  latitudeSign;
    long  degreesLatitude;
    long  degreesLongitude;
    long  uncertainty;
}GNSSEllipsoidPointWithUncertaintyCircle_t; 

typedef struct{
    GNSSLatitudeSign_e  latitudeSign;
    long  degreesLatitude;
    long  degreesLongitude;
    long  uncertaintySemiMajor;
    long  uncertaintySemiMinor;
    long  orientationMajorAxis;
    long  confidence;
}GNSSEllipsoidPointWithUncertaintyEllipse_t;

typedef struct{
    long  pointCount;
    GNSSEllipsoidPoint_t pointList[15];
}GNSSPolygon_t;

typedef struct{
    GNSSLatitudeSign_e  latitudeSign;
    long  degreesLatitude;
    long  degreesLongitude;
    GNSSAltitudeDirection_e  altitudeDirection;
    long  altitude;
}GNSSEllipsoidPointWithAltitude_t;

/* the uncertainty for altitude h, expressed in metres, is mapped  to a number K,
   with the formula: h= C(((1+x)^K)-1), with C = 45 and x = 0.025 */
typedef struct {
    GNSSLatitudeSign_e  latitudeSign;
    long  degreesLatitude;
    long  degreesLongitude;
    GNSSAltitudeDirection_e  altitudeDirection;
    long  altitude;
    long  uncertaintySemiMajor;
    long  uncertaintySemiMinor;
    long  orientationMajorAxis;
    long  uncertaintyAltitude;
    long  confidence;
}GNSSEllipsoidPointWithAltitudeAndUncertaintyEllipsoid_t;

typedef struct {
    GNSSLatitudeSign_e  latitudeSign;
    long  degreesLatitude;
    long  degreesLongitude;
    long  innerRadius;
    long  uncertaintyRadius;
    long  offsetAngle;
    long  includedAngle;
    long  confidence;
}GNSSEllipsoidArc_t;

typedef struct {
    GNSSLocationCoordinatesType_e coordType;
    union {
        GNSSEllipsoidPoint_t ellPoint;
        GNSSEllipsoidPointWithUncertaintyCircle_t  ellPointCircle;
        GNSSEllipsoidPointWithUncertaintyEllipse_t ellPointEllips;
        GNSSPolygon_t polygon;
        GNSSEllipsoidPointWithAltitude_t ellPointAltitude;
        GNSSEllipsoidPointWithAltitudeAndUncertaintyEllipsoid_t  ellPointAltitudeEllips;
        GNSSEllipsoidArc_t EllArc;
    }choice;
}GNSSLocationCoordinates_t;

typedef enum {
    GNSSVelocityType_NOTHING,
    GNSSVelocityType_horizontalVelocity,
    GNSSVelocityType_horizontalWithVerticalVelocity,
    GNSSVelocityType_horizontalVelocityWithUncertainty,
    GNSSVelocityType_horizontalWithVerticalVelocityAndUncertainty
}GNSSVelocityType_e;

typedef struct {
    long  bearing;
    long  horizontalSpeed;    
}GNSSHorizontalVelocity_t;

typedef struct {
    long     bearing;
    long     horizontalSpeed;
    long     verticalDirection;
    long     verticalSpeed;
}GNSSHorizontalWithVerticalVelocity_t;

typedef struct {
    long  bearing;
    long  horizontalSpeed;
    long  uncertaintySpeed;
}GNSSHorizontalVelocityWithUncertainty_t;

typedef struct {
    long  bearing;
    long  horizontalSpeed;
    long  verticalDirection;
    long  verticalSpeed;
    long  horizontalUncertaintySpeed;
    long  verticalUncertaintySpeed;
}GNSSHorizontalWithVerticalVelocityAndUncertainty_t;

typedef struct {
    GNSSVelocityType_e velType;
    union
    {
        GNSSHorizontalVelocity_t  horVel;
        GNSSHorizontalWithVerticalVelocity_t horWithVerVel;
        GNSSHorizontalVelocityWithUncertainty_t horVelUncertainty;
        GNSSHorizontalWithVerticalVelocityAndUncertainty_t VelUncertainty;
    }choice;
}GNSSVelocity_t;


typedef struct {
    GNSSPosTime_t posTime; 
    /*bitmap for positioning method or signal from a satellite system has been used*/
    unsigned short  positionData;//bitmap which defined according GNSS ID
    GNSSLocationCoordinates_t posEstimate;
} GNSSPosition_t;

typedef struct 
{
    GNSSLocationCoordinates_t     pos;
    GNSSVelocity_t                velocity;
    TcgTimeStamp                  time;
}GNSSPositionParam_t;

typedef struct 
{
    GNSSPositionParam_t pos;
    eCG_PosMethod       pos_method;    
    //gnss pos technology    //ffs
    //gnss signals Info      //ffs
}GNSSPositionData_t;

typedef struct 
{
    GNSSPositionData_t posData;
    //multiple Location IDs  //ffs
    //result Code            //ffs
    //time stamp             //ffs
}GNSSReportData_t;

typedef struct
{
    int                 numOfReportData;
    GNSSReportData_t    *reportData;
}GNSSReportDataList_t;


/* GNSS Measurement Information struct, including GPS and other system*/

#define  MAX_SUPPORT_SIGNAL_SMR   16//
#define  MAX_SUPPORT_SIGNAL_TYPE   8

typedef struct{
    unsigned char  svID;        // 1~64
    long  cNo;                  // 0~63 
    mPathInd_e  mpathInd;       // multipath indicator, enum
    float  codePhase;           //unit :ms, 0~(1-2^(-21))ms
    long  integerCodePhase;     //optional field, default 0; 0~127
    float  codePhaseRMSErr;      // 0.5~112
    float  doppler;             // -1310.72~1310.68
    bool  carrierQualityIndADRFlag;
    char  carrierQualityIndication; //bitmap,shall be presented together with ADR, no range
    float  adr;                 // 0 - 32767.5
} GNSSMsrPara_t;

typedef struct{
    GNSS_SIGNAL_T  signalID;//0-7
    bool  codePhaseAmbiguityFlag;
    int  codePhaseAmbiguity;//0-127 ms
    int  satMsrNum;// 0-16
    GNSSMsrPara_t  msrPara[MAX_SUPPORT_SIGNAL_SMR];
}GNSSGenericMrsList_t;

typedef struct {
    SUPL_GNSS_ID_T gnssID;  //
    unsigned char  signalTypeNum;
    GNSSGenericMrsList_t  genericMrsList[MAX_SUPPORT_SIGNAL_TYPE];
} GNSSMsrList_t;

typedef struct{
    TcgAgpsMsrSetList_t  gpsMrsList; /*GPS measurement  is difference from other satellite system in RRLP*/
    bool  timeFlag;
    long  tod;//if GPS measurement is present, this field may not be present
    GNSSTimeID_e   gnssTimeID;
    int  gnssNum;  //
    GNSSMsrList_t  mrsList[MAX_GNSS_NUM];
} GNSSMsrSetList_t;

typedef struct{
    SUPL_GNSS_ID_T gnssPosTech; //bitmap,requested GNSS position system
} GNSSPosReq_t;

/*GNSS assistance data*/
#define SUPPORT_STANDARD_CLOCK_MODEL   2

typedef enum{
    GNSSClockModel_nothing,
    GNSSClockModel_standardClockModelList,//Galileo
    GNSSClockModel_navClockModel,//
    GNSSClockModel_cnavClockModel,
    GNSSClockModel_glonassClockModel,
    GNSSClockModel_sbasClockModel,
    GNSSClockModel_bdsClockModel
}GNSSClockModel_e;

typedef  enum{
    GNSSOrbitModel_nothing,
    GNSSOrbitModel_keplerianSet,
    GNSSOrbitModel_navKeplerianSet,
    GNSSOrbitModel_cnavKeplerianSet,
    GNSSOrbitModel_glonassECEF,
    GNSSOrbitModel_sbasECEF,
    GNSSOrbitModel_bdsKeplerian
} GNSSOrbitModel_e;

/*     Parameter     scale factor     unit     */
typedef struct{
    long  stanClockToc;   /* 60      sec*/ 
    long  stanClockAF2;   /*  2^-59  sec/sec^2 */
    long  stanClockAF1;   /*  2^-46    sec/sec*/
    long  stanClockAF0;   /*  2^-34    sec*/
    bool  stanClockTgdFlag; 
    bool  stanModelIDFlag;
    long  stanClockTgd;   /*opt 2^-32   sec*/
    long  stanModelID;    //opt
}GNSSStandardClockModel_t;

typedef struct{
    int  clockNum;
    GNSSStandardClockModel_t standardClockModel[SUPPORT_STANDARD_CLOCK_MODEL];//
}GNSSStandardClockModelList_t;

typedef struct {
    long  navToc;    /*  2^4  seconds */
    long  navaf2;   /*  2^-55  sec/sec^2 */
    long  navaf1;    /*  2^-43  sec/sec*/
    long  navaf0;    /*  2^-31  seconds */
    long  navTgd;    /*  2^-31 seconds */
} GNSSNavClockModel_t;

typedef struct{
    long  cnavToc;    /*  300  seconds */
    long  cnavTop;    /*  300  seconds */
    long  cnavURA0;
    long  cnavURA1;
    long  cnavURA2;
    long  cnavAf2;    /*  2^-60  sec/sec^2 */
    long  cnavAf1;    /*  2^-48  sec/sec */
    long  cnavAf0;   /*  2^-35  seconds */
    long  cnavTgd;   /*  2^-35  seconds */
    bool  cnavISCl1cpcdFlag;//be together
    bool  cnavISCl1cal2cFlag;// be together
    bool  cnavISCl15Flag;   // be together
    long  cnavISCl1cp;   /*  2^-35  seconds */
    long  cnavISCl1cd;   /*  2^-35  seconds */
    long  cnavISCl1ca;   /*  2^-35  seconds */
    long  cnavISCl2c;   /*  2^-35  seconds */
    long  cnavISCl5i5;   /*  2^-35  seconds */
    long  cnavISCl5q5;   /*  2^-35  seconds */
} GNSSCnavClockModel_t;

typedef struct{
    long  gloTau;  /*  2^-30  seconds */
    long  gloGamma;  /* 2^-40  --*/
    bool  gloDeltaTauFlag;
    long  gloDeltaTau;  /* optional 2^-30  seconds */
}GNSSGlonassClockModel_t;

typedef struct{
    long  bdsToc;  /* 2^3  seconds */
    long  bdsA0;   /* 2^-33  sec/sec */
    long  bdsA1;   /* 2^-50  sec/sec^2 */
    long  bdsA2;    /* 2^-66  seconds */
    long  bdsTgd1;  /* 10^-10  seconds */
} GNSSBDSClockModel_t;

typedef struct{
    long  sbasTo;   /* 16  seconds */
    long  sbasAgfo;  /* 2^-31  seconds */
    long  sbasAgf1;  /* 2^-40  seconds/sec */
} GNSSSBASClockModel_t;

typedef struct{
    GNSSClockModel_e  present;
    union {
        GNSSStandardClockModelList_t standardClockModelList;
        GNSSNavClockModel_t navClockModel;
        GNSSCnavClockModel_t cnavClockModel;
        GNSSGlonassClockModel_t glonassClockModel;
        GNSSSBASClockModel_t  sbasClockModel;
        GNSSBDSClockModel_t  bdsClockModel;
    }choice;

}GNSSClockModel_t;

typedef struct {
    long     keplerToe;   /*  60   sec*/
    long     keplerW;    /*2^-31   semi-circles*/
    long     keplerDeltaN; /*2^-43   semi-circles/sec*/
    long     keplerM0;    /*2^-31   semi-circles*/
    long     keplerOmegaDot; /*2^-43   semi-circles/sec*/
    unsigned long   keplerE; /*2^-33   */
    long     keplerIDot;     /*2^-43   semi-circles/sec*/
    unsigned long   keplerAPowerHalf; /*2^-19   meters^1/2*/
    long     keplerI0;  /*2^-31   semi-circles */
    long     keplerOmega0; /*2^-31   semi-circles */
    long     keplerCrs;    /*2^-5   meters*/
    long     keplerCis;    /*2^-29   radians*/
    long     keplerCus;   /*2^-29   radians*/
    long     keplerCrc;   /*2^-5   meters*/
    long     keplerCic;   /*2^-29   radians*/
    long     keplerCuc;   /*2^-29   radians*/
} GNSSNavModelKeplerianSet_t;

/*may add  additional NAV para*/
typedef struct {
    long     navURA;
    long     navFitFlag;
    long     navToe;     /*2^4   sec*/
    long     navOmega;  /*2^-31   semi-circles*/
    long     navDeltaN;  /*2^-43   semi-circles/sec*/
    long     navM0;     /*2^-31   semi-circles*/
    long     navOmegaADot; /*2^-43   semi-circles/sec*/
    unsigned long     navE;
    long     navIDot;     /*2^-43   semi-circles/sec*/
    unsigned long     navAPowerHalf;  /*2^-19   meters^1/2*/
    long     navI0;    /*2^-31   semi-circles*/
    long     navOmegaA0;   /*2^-31   semi-circles*/
    long     navCrs;   /*2^-5   meters*/
    long     navCis;   /*2^-29   radians*/
    long     navCus;  /*2^-29   radians*/
    long     navCrc;  /*2^-5    meters*/
    long     navCic;  /*2^-29   radians*/
    long     navCuc;  /*2^-29   radians*/
} GNSSNavModelNavKeplerianSet_t;

typedef struct {
    long     cnavTop;      /*  300   seconds*/
    long     cnavURAindex;
    long     cnavDeltaA;   /* 2^-9    meters*/
    long     cnavAdot;     /* 2^-21    meters/sec*/
    long     cnavDeltaNo;  /*2^-44   semi-circles/sec*/
    long     cnavDeltaNoDot; /*2^-57   semi-circles/sec^2*/
    long long  cnavMo;  /*2^-32   semi-circles */
    long long  cnavE;  /*2^-34   */
    long long  cnavOmega;  /*2^-32   semi-circles */
    long long  cnavOMEGA0;  /*2^-32   semi-circles */
    long     cnavDeltaOmegaDot; /*2^-44   semi-circles/sec*/
    long long  cnavIo;  /*2^-32   semi-circles */
    long     cnavIoDot;  /*2^-44   semi-circles/sec*/
    long     cnavCis;  /*2^-30   radians*/
    long     cnavCic;  /*2^-30   radians*/
    long     cnavCrs;  /*2^-8    meters*/
    long     cnavCrc;  /*2^-8    meters*/
    long     cnavCus;  /*2^-30   radians*/
    long     cnavCuc;  /*2^-30   radians*/
} GNSSNavModelCnavKeplerianSet_t;

typedef struct {
    long     gloEn;   /* 1   days */
    unsigned char   gloP1; //bitstring
    bool     gloP2;
    long     gloM;
    long     gloX;    /* 2^-11   kilometers*/
    long     gloXdot;  /* 2^-20   kilometers/sec*/
    long     gloXdotdot; /* 2^-30   kilometers/sec^2*/
    long     gloY;    /* 2^-11   kilometers*/
    long     gloYdot;  /* 2^-20   kilometers/sec*/
    long     gloYdotdot; /* 2^-30   kilometers/sec^2*/
    long     gloZ;  /* 2^-11   kilometers*/
    long     gloZdot;  /* 2^-20   kilometers/sec*/
    long     gloZdotdot;  /* 2^-30   kilometers/sec^2*/
} GNSSNavModelGlonassECEF_t;

typedef struct {
    bool     sbasToFlag;
    /* optional,if clock model-5 is not present ,this must be present*/
    long     sbasTo;  /* 16  sec*/
    unsigned char  sbasAccuracy;//bitstring
    long     sbasXg;  /*0.08  meters*/
    long     sbasYg;  /*0.08  meters*/
    long     sbasZg;  /*0.4  meters*/
    long     sbasXgDot;  /*0.000625   meters/sec*/
    long     sbasYgDot;  /*0.000625   meters/sec*/
    long     sbasZgDot;  /*0.004   meters/sec*/
    long     sbasXgDotDot;  /*0.0000125   meters/sec^2*/
    long     sbagYgDotDot;  /*0.0000125   meters/sec^2*/
    long     sbasZgDotDot;  /*0.0000625   meters/sec^2*/
} GNSSNavModelSBASECEF_t;

typedef struct {
    long  bdsURAI;
    long  bdsToe;   /* 2^3  sec*/
    unsigned long  bdsAPowerHalf;  /*2^-19   meters^1/2*/
    unsigned long  bdsE;  /*2^-33  */
    long  bdsW;  /*2^-31   semi-circles */
    long  bdsDeltaN; /*2^-43   semi-circles/sec */
    long  bdsM0;  /*2^-31   semi-circles */
    long  bdsOmega0;  /*2^-31   semi-circles */
    long  bdsOmegaDot; /*2^-43   semi-circles/sec */
    long  bdsI0;  /*2^-31   semi-circles */
    long  bdsIDot;  /*2^-43   semi-circles/sec */
    long  bdsCuc;  /*2^-31  radians*/
    long  bdsCus;  /*2^-31  radians*/
    long  bdsCrc;  /*2^-6  meters*/
    long  bdsCrs;  /*2^-6  meters*/
    long  bdsCic;  /*2^-31  radians*/
    long  bdsCis;  /*2^-31  radians*/
} GNSSNavModelBDSECEF_t;

typedef struct {
    GNSSOrbitModel_e  present;
    union{
        GNSSNavModelKeplerianSet_t   keplerianSet;
        GNSSNavModelNavKeplerianSet_t  navKeplerianSet;
        GNSSNavModelCnavKeplerianSet_t   cnavKeplerianSet;
        GNSSNavModelGlonassECEF_t  glonassECEF;
        GNSSNavModelSBASECEF_t   sbasECEF;
        GNSSNavModelBDSECEF_t   bdsECEF;
    }choice;
} GNSSOrbitModel_t;

typedef enum{
    GNSSIDSignalType_Nothing,
    GNSSIDSignalType_GPSL1CA,
    GNSSIDSignalType_ModernizedGPS,
    GNSSIDSignalType_SBAS,
    GNSSIDSignalType_QZSS_QZSL1,
    GNSSIDSignalType_QZSS_QZSL1CL2CL5,
    GNSSIDSignalType_GLONASS,
    GNSSIDSignalType_Galileo,
    GNSSIDSignalType_BDS
}GNSSIDSignalType_e;

typedef struct{
     char svHealth; // 6 bit in message 
}GPSL1CASvHealth_t;
//ref:rrlp table A48.1
typedef struct{
    char e5aDateValidityStatus; // 1 bit in message
    char e5bDateValidityStatus; // 1 bit in message
    char e1BDateValidityStatus; // 1 bit in message
    char e5aSignalHealthStatus; // 2 bit in message
    char e5bSignalHealthStatus; // 2 bit in message
    char e1BSignalHealthStatus; // 2 bit in message
}GalileoSvHealth_t;

typedef struct{
    char l1cHealth; // 1 bit in message
    char l1Health; // 1 bit in message
    char l2Health; // 1 bit in message
    char l5Health; // 1 bit in message
}ModernizedGPSSvHealth_t;

typedef struct{
    char ranging; // 1 bit in message ,on (0),off(1)
    char correction;// 1 bit in message ,on (0),off(1)
    char integrity;// 1 bit in message ,on (0),off(1)
}SBASSvHealth_t;

typedef struct{
    char svHealth;// 6 bit in message;combine 5 bit svhealth with 1bit  SV Health_MSB in rrlp
}QZSSL1SvHealth_t;

typedef struct{
    char l1cHealth; // 1 bit in message
    char l1Health; // 1 bit in message
    char l2Health; // 1 bit in message
    char l5Health; // 1 bit in message
}QZSSL1CL2CL5SvHealth_t;

typedef struct{
    char bn; // 1 bit in message
    char ft; // 4 bit in message
}GLONASSSvHealth_t;

typedef struct{
    char b1IHealth; // 1 bit in message, SatH1
}BDSSvHealth_t;

typedef struct{
    GNSSIDSignalType_e present;
    union{
        GPSL1CASvHealth_t gpsHealth;
        GalileoSvHealth_t galHealth;
        ModernizedGPSSvHealth_t modGPSHealth;
        SBASSvHealth_t sbasHealth;
        QZSSL1SvHealth_t qzssL1Health;
        QZSSL1CL2CL5SvHealth_t qzssl1cl2cl5Health;
        GLONASSSvHealth_t gloHealth;
        BDSSvHealth_t bdsHealth;
    }choice;
}SvHealth_t;

// ref: rrlp table 48.2  
typedef struct{
    long iodc;
}GPSL1CAIOD_t;

typedef struct{
    long iodNav; // 0-1023
}GalileoIOD_t;

typedef struct{
    long toe; // 0-306900 or 0-604500
}ModernizedGPSIOD_t;

typedef struct{
    long iod; // 0-255
}SBASIOD_t;

typedef struct{
    long iodc;//  0 - 1023
}QZSSL1IOD_t;

typedef struct{
    long toe; // 0-306900 or 0-604500
}QZSSL1CL2CL5IOD_t;

typedef struct{
    long tb;//minutes
}GLONASSIOD_t;

typedef struct{
    short iodc;
    short iode;
}BDSIOD_t;

typedef struct{
    GNSSIDSignalType_e present;
    union{
        GPSL1CAIOD_t gpsIOD;
        GalileoIOD_t galIOD;
        ModernizedGPSIOD_t modGPSIOD;
        SBASIOD_t sbasIOD;
        QZSSL1IOD_t qzssL1IOD;
        QZSSL1CL2CL5IOD_t qzssL1cL2cL5IOD;
        GLONASSIOD_t gloIOD;
        BDSIOD_t bdsIOD;
    }choice;
}GNSSIOD_t;

typedef struct{
    unsigned char  svID;
    SvHealth_t  svHealth;
    GNSSIOD_t  iod;
    GNSSClockModel_t  clockModel;
    GNSSOrbitModel_t   orbitModel;
}GNSSNavModelSat_t;

typedef struct{
    unsigned  int  nonBroadcastIndFlag; //0-1
    unsigned  char  satNum;
    GNSSNavModelSat_t   navModelSat[32];//Max supported Satellites is 32 in rrlp
} GNSSNavigationModel_t;

/* GNSS reference measurement assist struct, Acquisition Assistance */

typedef enum{
    GNSSDopplerUncertaintyExt_d60,
    GNSSDopplerUncertaintyExt_d80,
    GNSSDopplerUncertaintyExt_d100,
    GNSSDopplerUncertaintyExt_d120,
    GNSSDopplerUncertaintyExt_noInformation
}DopplerUncertaintyExt_e;

typedef struct{
    float  doppler1;   // m/s^2
    float  dopplerUncertainty;// m/s
}GNSSAddionalDoppler_t;

typedef struct{
    unsigned char  svID;
    double   doppler0; // -1024 m/s to 1023.5 m/s
    double   codePhase;  /*ms */
    long     intCodePhase;  /* 1  ms*/
    float    codePhaseSearchWindow;
    bool     addionalDopplerFlag;
    bool     addionalAngleFlag;
    TcgAddionalAngleFields_t    addionalAngle;  //opt
    GNSSAddionalDoppler_t   addionalDoppler;//opt
    DopplerUncertaintyExt_e  dopplerUncertaintyExt;
}GNSSRefMsrAssistElement_t;

typedef struct{
    GNSS_SIGNAL_T  signalID;
    bool  confidenceFlag; //
    unsigned char  confidence; //rang:0-100
    unsigned char  satNum;
    GNSSRefMsrAssistElement_t RefMsrAssist[MAX_SATS_ON_ACQUISASSIST];  
}GNSSRefMsrAssist_t;

typedef struct{
    char svID; // 0-63
    GNSS_SIGNAL_T signalsAvailable;//bitmap
    long gloChannelnumber;// -7 ~ 13 only gnssid == GLONASS 
}GNSSAuxiliaryInfoSat_t;

typedef struct{
    char satNum; 
    GNSSAuxiliaryInfoSat_t auxInfo[32];
}GNSSAuxiliaryInfo_t;

/*this time£¬according to GPS,only deliver ephemeris and reference Measurement information*/
typedef struct{
    SUPL_GNSS_ID_T gnssID;
    SUPL_SBAS_ID_T sbasID;
    GNSSNavigationModel_t  *navModel;
    GNSSRefMsrAssist_t   *refMsrAssit; 
    GNSSAuxiliaryInfo_t  *auxiliaryInfo;
} GNSSGenericAssData_t;

typedef struct{
    unsigned char  gnssNum;
    GNSSGenericAssData_t genericAssData[MAX_GNSS_NUM];
}GNSSGenericAssDataList_t;

typedef unsigned int AGNSS_REF_TIME_t;

#define AGNSS_REF_TIME_TOW  0 // bit 0
#define AGNSS_REF_TIME_TOD  1 // bit 1

typedef struct{
    GNSSTimeID_e timeID;
    long dayNumber; // 0-32767
    long timeofDay; //0-86399 second
    long todFrac; //ns
    double uncertainty; // optional ,unit: ms, 0-8430
}RefTimeTOD_t;

typedef struct{
    GNSSTimeID_e timeID;
    long weekNo;
    long timeofweek; // second
    long towFrac; // ns
    double uncertainty; // optional , unit: ms, 0-2960
}RefTimeTOW_t;

typedef struct{
    AGNSS_REF_TIME_t flag;//bitmap for tow or tod, bit 0 is tow present, bit 1 is tod present
    RefTimeTOW_t tow;
    RefTimeTOD_t tod;
    unsigned long sysTime;     
}AGNSSReferenceTime_t;

typedef struct{
    AGNSSReferenceTime_t       *referenceTime;
    GNSSLocationCoordinates_t  *referenceLoc;
}GNSSCommonAssData_t;

typedef struct{
    GNSSCommonAssData_t  *commonAssData;
    GNSSGenericAssDataList_t *genericAssDataList;
}GNSSAssistData_t;

/*ref 36.355*/
/*dataID is 2 bit in msg,and '10'is reserved */
typedef enum{
    KlobucharModelDataID_worldwide = 0,//00 means the parameters are applicable worldwide
    KlobucharModelDataID_BDS,
    KlobucharModelDataID_QZSS = 3    
}KlobucharModelDataID_e;

typedef struct{
    KlobucharModelDataID_e dataID;
    long alfa0; // 2^-30 seconds
    long alfa1; // 2^-27 seconds/semi-circle
    long alfa2; // 2^-24 seconds/semi-circle^2
    long alfa3; // 2^-24 seconds/semi-circle^3
    long beta0; // 2^11 seconds
    long beta1; // 2^14 seconds/semi-circle
    long beta2; // 2^16 seconds/semi-circle^2
    long beta3; // 2^16 seconds/semi-circle^3
}KlobucharModel_t;

typedef struct{
    long ai0; // 2^-2 Solar Flux Units (SFUs)
    long ai1; // 2^-8 SFUs/degree
    long ai2; // 2^-15 SFUs/degree^2
    char ionoStormFlagPresent; // 0-1,below para shall be present together 
    char ionoStormFlag1; // 0-1, optional
    char ionoStormFlag2; // 0-1 
    char ionoStormFlag3; // 0-1 
    char ionoStormFlag4; // 0-1 
    char ionoStormFlag5; // 0-1 
}NeQuickModel_t;

typedef struct{
    unsigned char klobucharFlag;
    unsigned char neQuickFlag;
    KlobucharModel_t kloModel;
    NeQuickModel_t neqModel;
}GNSSIonosphericModel_t;

typedef enum{
    GNSSAlmanac_invalid,
    GNSSAlmanac_rrlp,
    GNSSAlmanac_rrc,
    GNSSAlmanac_lpp,
    GNSSAlmanac_lppe,
    GNSSAlmanac_tia801,
    GNSSAlmanac_unknown
}GNSSAlmanacProtocol_e;

typedef struct {
    long           satelliteID;           /* (0..63)               Scale Factor: ---;     Units: --- */
    long           almanacE;              /* (0..65535)            Scale Factor: 2^(-21); Units: dimensionless */
    long           almanacToa;            /* (0..255)              Scale Factor: 2^(12);  Units: sec */
    long           almanacKsii;           /* (-32768..32767)       Scale Factor: 2^(-19); Units: semi-circles */
    long           almanacOmegaDot;       /* (-32768..32767)       Scale Factor: 2^(-38); Units: semi-circles/sec */
    long           almanacSVhealth;       /* (0..255)              Scale Factor: ---;     Units: Boolean */
    long           almanacAPowerHalf;     /* (0..16777215)         Scale Factor: 2^(-11); Units: meters^(1/2) */
    long           almanacOmega0;         /* (-8388608..8388607)   Scale Factor: 2^(-23); Units: semi-circles */
    long           almanacW;              /* (-8388608..8388607)   Scale Factor: 2^(-23); Units: semi-circles */
    long           almanacM0;             /* (-8388608..8388607)   Scale Factor: 2^(-23); Units: semi-circles */
    long           almanacAF0;            /* (-1024..1023)         Scale Factor: 2^(-20); Units: seconds */
    long           almanacAF1;            /* (-1024..1023)         Scale Factor: 2^(-38); Units: sec/sec */
} GPSAlmanacElementRRLP_t;

/* Almanac-KeplerianSet(Model-1) */
typedef struct {
    long           svID;                  /* (0..63)               Scale Factor: ---;     Units: --- */
    long           kepAlmanacE;           /* (0 .. 2047)           Scale Factor: 2^(-16); Units: dimensionless */
    long           kepAlmanacDeltaI;      /* (-1024 .. 1023)       Scale Factor: 2^(-14); Units: semi-circles */
    long           kepAlmanacOmegaDot;    /* (-1024 .. 1023)       Scale Factor: 2^(-33); Units: semi-circles/sec */
    unsigned char  kepSVStatusINAV;       /* BIT STRING (SIZE (4)) Scale Factor: ---;     Units: dimensionless */
    unsigned char  kepSVStatusFNAVFlag;   /* BOOLEAN               Scale Factor: ---;     Units: --- */
    unsigned char  kepSVStatusFNAV;       /* BIT STRING (SIZE (2)) Scale Factor: ---;     Units: dimensionless */
    long           kepAlmanacAPowerHalf;  /* (-4096..4095)         Scale Factor: 2^(-9);  Units: meters^(1/2) */
    long           kepAlmanacOmega0;      /* (-32768 .. 32767)     Scale Factor: 2^(-15); Units: semi-circles */
    long           kepAlmanacW;           /* (-32768 .. 32767)     Scale Factor: 2^(-15); Units: semi-circles */
    long           kepAlmanacM0;          /* (-32768 .. 32767)     Scale Factor: 2^(-15); Units: semi-circles */
    long           kepAlmanacAF0;         /* (-32768 .. 32767)     Scale Factor: 2^(-19); Units: seconds */
    long           kepAlmanacAF1;         /* (-4096..4095)         Scale Factor: 2^(-38); Units: sec/sec */
} AlmanacModel1_KeplerianSet_t;

/* Almanac-NAVKeplerianSet(Model-2) */
typedef struct {
    long           svID;                  /* (0..63)               Scale Factor: ---;     Units: --- */
    long           navAlmE;               /* (0..65535)            Scale Factor: 2^(-21); Units: dimensionless */
    long           navAlmDeltaI;          /* (-32768..32767)       Scale Factor: 2^(-19); Units: semi-circles */
    long           navAlmOMEGADOT;        /* (-32768..32767)       Scale Factor: 2^(-38); Units: semi-circles/sec */
    long           navAlmSVHealth;        /* (0..255)              Scale Factor: ---;     Units: Boolean */
    long           navAlmSqrtA;           /* (0..16777215)         Scale Factor: 2^(-11); Units: meters^(1/2) */
    long           navAlmOMEGAo;          /* (-8388608..8388607)   Scale Factor: 2^(-23); Units: semi-circles */
    long           navAlmOmega;           /* (-8388608..8388607)   Scale Factor: 2^(-23); Units: semi-circles */
    long           navAlmMo;              /* (-8388608..8388607)   Scale Factor: 2^(-23); Units: semi-circles */
    long           navAlmaf0;             /* (-1024..1023)         Scale Factor: 2^(-20); Units: seconds */
    long           navAlmaf1;             /* (-1024..1023)         Scale Factor: 2^(-38); Units: sec/sec */
} AlmanacModel2_NAVKeplerianSet_t;

/* Almanac-ReducedKeplerianSet(Model-3) */
typedef struct {
    long           svID;                  /* (0..63)               Scale Factor: ---;     Units: --- */
    long           redAlmDeltaA;          /* (-128..127)           Scale Factor: 2^(+9);  Units: meters */
    long           redAlmOmega0;          /* (-64..63)             Scale Factor: 2^(-6);  Units: semi-cicles */
    long           redAlmPhi0;            /* (-64..63)             Scale Factor: 2^(-6);  Units: semi-circles */
    unsigned char  redAlmL1Health;        /* BOOLEAN               Scale Factor: ---;     Units: --- */
    unsigned char  redAlmL2Health;        /* BOOLEAN               Scale Factor: ---;     Units: --- */
    unsigned char  redAlmL5Health;        /* BOOLEAN               Scale Factor: ---;     Units: --- */
} AlmanacModel3_ReducedKeplerianSet_t;

/* Almanac-MidiAlmanacSet(Model-4) */
typedef struct {
    long           svID;                  /* (0..63)               Scale Factor: ---;     Units: --- */
    long           midiAlmE;              /* (0..2047)             Scale Factor: 2^(-16); Units: --- */
    long           midiAlmDeltaI;         /* (-1024..1023)         Scale Factor: 2^(-14); Units: semi-circles */
    long           midiAlmOmegaDot;       /* (-1024..1023)         Scale Factor: 2^(-33); Units: semi-circles/sec */
    long           midiAlmSqrtA;          /* (0..131071)           Scale Factor: 2^(-4);  Units: meters^(1/2) */
    long           midiAlmOmega0;         /* (-32768..32767)       Scale Factor: 2^(-15); Units: semi-circles */
    long           midiAlmOmega;          /* (-32768..32767)       Scale Factor: 2^(-15); Units: semi-circles */
    long           midiAlmMo;             /* (-32768..32767)       Scale Factor: 2^(-15); Units: semi-circles */
    long           midiAlmaf0;            /* (-1024..1023)         Scale Factor: 2^(-20); Units: seconds */
    long           midiAlmaf1;            /* (-512..511)           Scale Factor: 2^(-37); Units: sec/sec */
    unsigned char  midiAlmL1Health;       /* BOOLEAN               Scale Factor: ---;     Units: --- */
    unsigned char  midiAlmL2Health;       /* BOOLEAN               Scale Factor: ---;     Units: --- */
    unsigned char  midiAlmL5Health;       /* BOOLEAN               Scale Factor: ---;     Units: --- */
} AlmanacModel4_MidiAlmanacSet_t;

/* Almanac-GlonassAlmanacSet(Model-5) */
typedef struct {
    long           gloAlmNA;              /* (1..1461)             Scale Factor: 1;       Units: days */
    long           gloAlmnA;              /* (1..24)               Scale Factor: 1;       Units: --- */
    long           gloAlmHA;              /* (0..31)               Scale Factor: 1;       Units: --- */
    long           gloAlmLambdaA;         /* (-1048576..1048575)   Scale Factor: 2^(-20); Units: semi-circles */
    long           gloAlmtlambdaA;        /* (0..2097151)          Scale Factor: 2^(-5);  Units: seconds */
    long           gloAlmDeltaIa;         /* (-131072..131071)     Scale Factor: 2^(-20); Units: semi-circles */
    long           gloAlmDeltaTA;         /* (-2097152..2097151)   Scale Factor: 2^(-9);  Units: seconds/orbit period */
    long           gloAlmDeltaTdotA;      /* (-64..63)             Scale Factor: 2^(-14); Units: seconds/orbit period^2 */
    long           gloAlmEpsilonA;        /* (0..32767)            Scale Factor: 2^(-20); Units: --- */
    long           gloAlmOmegaA;          /* (-32768..32767)       Scale Factor: 2^(-15); Units: semi-circles */
    long           gloAlmTauA;            /* (-512..511)           Scale Factor: 2^(-18); Units: seconds */
    long           gloAlmCA;              /* (0..1)                Scale Factor: 1;       Units: --- */
    unsigned char  gloAlmMAFlag;          /* BOOLEAN               Scale Factor: ---;     Units: --- */
    unsigned char  gloAlmMA;              /* BIT STRING (SIZE (2)) Scale Factor: 1;       Units: --- */
} AlmanacModel5_GlonassAlmanacSet_t;

/* Almanac-ECEFsbasAlmanacSet(Model-6) */
typedef struct {
    long           sbasAlmDataID;         /* (0..3)                Scale Factor: 1;       Units: --- */
    long           svID;                  /* (0..63)               Scale Factor: ---;     Units: --- */
    unsigned char  sbasAlmHealth;         /* BIT STRING (SIZE (8)) Scale Factor: ---;     Units: --- */
    long           sbasAlmXg;             /* (-16384..16383)       Scale Factor: 2600;    Units: meters */
    long           sbasAlmYg;             /* (-16384..16383)       Scale Factor: 2600;    Units: meters */
    long           sbasAlmZg;             /* (-256..255)           Scale Factor: 26000;   Units: meters */
    long           sbasAlmXgdot;          /* (-4..3)               Scale Factor: 10;      Units: meters/sec */
    long           sbasAlmYgDot;          /* (-4..3)               Scale Factor: 10;      Units: meters/sec */
    long           sbasAlmZgDot;          /* (-8..7)               Scale Factor: 40.96;   Units: meters/sec */
    long           sbasAlmTo;             /* (0..2047)             Scale Factor: 64;      Units: seconds */
} AlmanacModel6_ECEFsbasAlmanacSet_t;

/* Almanac-BDSAlmanacSet-r12(Model-7) */
typedef struct {
    long           svID;                  /* (0..63)               Scale Factor: ---;     Units: --- */
    /* This field may be present if the toa is not the same for all SVs; 
       otherwise it is not present and the toa is provided in GANSSAlmanacModel. */
    unsigned char  bdsAlmToa_r12Flag;     /* BOOLEAN               Scale Factor: ---;     Units: --- */
    long           bdsAlmToa_r12;         /* (0..255)              Scale Factor: 2^(12);  Units: seconds */
    long           bdsAlmSqrtA_r12;       /* (0..16777215)         Scale Factor: 2^(-11); Units: meters^(1/2) */
    long           bdsAlmE_r12;           /* (0..131071)           Scale Factor: 2^(-21); Units: --- */
    long           bdsAlmW_r12;           /* (-8388608..8388607)   Scale Factor: 2^(-23); Units: semi-circles */
    long           bdsAlmM0_r12;          /* (-8388608..8388607)   Scale Factor: 2^(-23); Units: semi-circles */
    long           bdsAlmOmega0_r12;      /* (-8388608..8388607)   Scale Factor: 2^(-23); Units: semi-circles */
    long           bdsAlmOmegaDot_r12;    /* (-65536..65535)       Scale Factor: 2^(-38); Units: semi-circles/sec */
    long           bdsAlmDeltaI_r12;      /* (-32768..32767)       Scale Factor: 2^(-19); Units: semi-circles */
    long           bdsAlmA0_r12;          /* (-1024..1023)         Scale Factor: 2^(-20); Units: seconds */
    long           bdsAlmA1_r12;          /* (-1024..1023)         Scale Factor: 2^(-38); Units: sec/sec */
    /* This field is mandatory present if SV-ID is between 0 and 29; otherwise it is not present. */
    unsigned char  bdsSvHealth_r12Flag;   /* BOOLEAN               Scale Factor: ---;     Units: --- */
    unsigned int   bdsSvHealth_r12;       /* BIT STRING (SIZE (9)) Scale Factor: ---;     Units: --- */
} AlmanacModel7_BDSAlmanacSet_r12_t;

typedef struct {
    long                                   almanacWNa;    /* GPS week number(0..255) */
    long                                   almanacElementNum;
    GPSAlmanacElementRRLP_t                almanacList[64];
} GPSAlmanacRRLP_t;

typedef enum {
    GANSSAlmanacElementModelRRLP_NOTHING,
    GANSSAlmanacElementModelRRLP_KeplerianSet,
    GANSSAlmanacElementModelRRLP_NAVKeplerianSet,
    GANSSAlmanacElementModelRRLP_ReducedKeplerianSet,
    GANSSAlmanacElementModelRRLP_MidiAlmanacSet,
    GANSSAlmanacElementModelRRLP_GlonassAlmanacSet,
    GANSSAlmanacElementModelRRLP_ECEFsbasAlmanacSet,
    GANSSAlmanacElementModelRRLP_BDSAlmanacSet_r12
} GANSSAlmanacElementModelRRLP_e;

typedef struct {
    GANSSAlmanacElementModelRRLP_e          present;
    union {
        AlmanacModel1_KeplerianSet_t        keplerianAlmanacSet;
        AlmanacModel2_NAVKeplerianSet_t     navKeplerianAlmanac;
        AlmanacModel3_ReducedKeplerianSet_t reducedKeplerianAlmanac;
        AlmanacModel4_MidiAlmanacSet_t      midiAlmanac;
        AlmanacModel5_GlonassAlmanacSet_t   glonassAlmanac;
        AlmanacModel6_ECEFsbasAlmanacSet_t  ecefSBASAlmanac;
        AlmanacModel7_BDSAlmanacSet_r12_t   bdsAlmanac_r12;
    } choice;
} GANSSAlmanacElementRRLP_t;

typedef struct {
    /* Week Number
       This field specifies the Almanac reference week number in GNSS specific system time to 
       which the Almanac Reference Time Toa is referenced, modulo 256 weeks. 
       If Toa is not included in GANSS Almanac Model, the MS shall ignore the Week Number. */
    long                           weekNumber;  /* (0..255) */
    /* Toa
       This field specifies the Almanac Reference Time common to all satellites in GANSS Almanac 
       Model Using Keplerian Parameters given in GNSS specific system time.
       In case of GNSS-ID does not indicate Galileo, the scale factor is 2^(12) seconds. 
       In case of GNSS-ID does indicate Galileo, the scale factor is 600 seconds. */
    long                           toaFlag;
    long                           toa;         /* (0..255) */
    /* IODa
       This field specifies the Issue-Of-Data common to all satellites in GANSS Almanac Model 
       using Keplerian Parameters. */
    long                           iodaFlag;
    long                           ioda;        /* (0..3) */
    long                           ganssAlmanacElementNum;
    GANSSAlmanacElementRRLP_t      ganssAlmanacList[36];
} GANSSAlmanacModelRRLP_t;

typedef struct {
    /* This field specifies the Almanac Reference Time common to all satellites in GANSS Almanac Model 
       using Keplerian Parameters given in GNSS specific system time. 
       In case of GNSS-ID does indicate Galileo, the scale factor is 600 seconds. */
    long                           toa_extFlag;
    long                           toa_ext;     /* (256..1023) */
    /* This field specifies the Issue-Of-Data common to all satellites in GANSS Almanac Model 
       using Keplerian Parameters. */
    long                           ioda_extFlag;
    long                           ioda_ext;    /* (4..15) */
} GANSSAlmanacModelR12ExtRRLP_t;

typedef struct {
    SUPL_GNSS_ID_T                 ganssID;
    SUPL_SBAS_ID_T                 sbasID;   /* If GANSS ID indicates SBAS, this field defines the specific 
                                                SBAS for which the GANSS specific assistance data are provided */
    unsigned char                  ganssAlmanacModelFlag;
    GANSSAlmanacModelRRLP_t        ganssAlmanacModeldata;
    unsigned char                  ganssCompleteAlmanacFlag;
    unsigned char                  ganssCompleteAlmanacProvided;   /* GANSSAlmanacModel_R10_Ext */
    unsigned char                  ganssAlmanacModelR12ExtFlag;
    GANSSAlmanacModelR12ExtRRLP_t  ganssAlmanacModel_R12_Ext;      /* It is used only if GANSS-ID is Galileo 
                                                                      in this version of protocol */
} GANSSAlmanacRRLP_t;

typedef struct {
    unsigned char                  gpsAlmanacFlag;
    GPSAlmanacRRLP_t               gpsAlmanac;
    unsigned char                  gpsCompleteAlmanacFlag;
    unsigned char                  gpsCompleteAlmanacProvided;   /* GPSAlmanac_R10_Ext */
    long                           ganssNum;
    GANSSAlmanacRRLP_t            *ganssAlmanacData;             /* #define MAX_GNSS_NUM  8 */
} GNSSAlmanacRRLP_t;

typedef struct {
    int null;    //TODO
} GNSSAlmanacRRC_t;

typedef enum {
    GNSSAlmanacElementModelLPP_NOTHING,
    GNSSAlmanacElementModelLPP_KeplerianSet,
    GNSSAlmanacElementModelLPP_NAVKeplerianSet,
    GNSSAlmanacElementModelLPP_ReducedKeplerianSet,
    GNSSAlmanacElementModelLPP_MidiAlmanacSet,
    GNSSAlmanacElementModelLPP_GlonassAlmanacSet,
    GNSSAlmanacElementModelLPP_ECEFsbasAlmanacSet,
    GNSSAlmanacElementModelLPP_BDSAlmanacSet_r12
} GNSSAlmanacElementModelLPP_e;

typedef struct {
    GNSSAlmanacElementModelLPP_e present;
    union {
        AlmanacModel1_KeplerianSet_t          keplerianAlmanacSet;
        AlmanacModel2_NAVKeplerianSet_t       navKeplerianAlmanac;
        AlmanacModel3_ReducedKeplerianSet_t   reducedKeplerianAlmanac;
        AlmanacModel4_MidiAlmanacSet_t        midiAlmanac;
        AlmanacModel5_GlonassAlmanacSet_t     glonassAlmanac;
        AlmanacModel6_ECEFsbasAlmanacSet_t    ecefSBASAlmanac;
        AlmanacModel7_BDSAlmanacSet_r12_t     bdsAlmanac_r12;
    } choice;
} GNSSAlmanacElementLPP_t;

typedef struct {
    /* weekNumber
       This field specifies the almanac reference week number in GNSS specific system time to 
       which the almanac referencetime toa is referenced, modulo 256 weeks. 
       This field is required for non-GLONASS GNSS. */
    long                    weekNumberFlag;
    long                    weekNumber;          /* (0..255) */
    /* toa, toa-ext
       In case of GNSS-ID does not indicate Galileo, this field specifies the almanac reference time 
       given in GNSS specific system time, in units of seconds with a scale factor of 2^(12). 
       toa is required for non-GLONASS GNSS.
       In case of GNSS-ID does indicate Galileo, this field specifies the almanac reference time 
       given in GNSS specific system time, in units of seconds with a scale factor of 600 seconds. 
       Either toa or toa-ext is required for Galileo GNSS. */
    long                    toaFlag;
    long                    toa;                 /* (0..255) */
    long                    toa_ext_v12xyFlag;
    long                    toa_ext_v12xy;       /* (256..1023) */
    /* Either ioda or ioda-ext is required for Galileo GNSS. */
    long                    iodaFlag;
    long                    ioda;                /* (0..3) */
    long                    ioda_ext_v12xyFlag;
    long                    ioda_ext_v12xy;      /* (4..15) */
    /* If set to TRUE, the gnss-AlmanacList contains almanacs for 
          the complete GNSS constellation indicated by GNSS-ID. */
    long                    completeAlmanacProvidedFlag;
    long                    completeAlmanacProvided;     /* BOOLEAN */
    long                    gnssAlmanacElementNum;
    GNSSAlmanacElementLPP_t gnssAlmanacElement[64];
} GNSSAlmanacListLPP_t;

typedef struct {
    SUPL_GNSS_ID_T          gnssID;
    SUPL_SBAS_ID_T          sbasID;   /* The field is mandatory present if the GNSS-ID = sbas; 
                                         otherwise it is not present. */
    GNSSAlmanacListLPP_t    gnssAlmanacList;
} GNSSAlmanacDataLPP_t;

typedef struct {
    long                    gnssNum;
    GNSSAlmanacDataLPP_t   *gnssAlmanacData;            /* #define MAX_GNSS_NUM  8 */
} GNSSAlmanacLPP_t;

typedef struct {
    int null;    //TODO
} GNSSAlmanacLPPE_t;

typedef struct {
    int null;    //TODO
} GNSSAlmanacTIA801_t;

typedef struct {
    GNSSAlmanacProtocol_e   present;
    union {
        GNSSAlmanacRRLP_t   rrlpAlmanac;
        GNSSAlmanacRRC_t    rrcAlmanac;
        GNSSAlmanacLPP_t    lppAlmanac;
        GNSSAlmanacLPPE_t   lppeAlmanac;
        GNSSAlmanacTIA801_t tia801Almanac;
    } choice;
} GNSSAlmanacData_t;

#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif


