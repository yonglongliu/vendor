/******************************************************************************
** File Name:  agps_type.h                                                    *
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
** 2014-06-02            Andy.Huang                Create.                    *
******************************************************************************/

#ifndef _AGPS_TYPE_H_
#define _AGPS_TYPE_H_

#include <stddef.h>
#include "lppe_export.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


/********************************************************
* 1 Data Types
*********************************************************/

/** Unsigned 8 bit data */
typedef unsigned char   U8;
/** Unsigned 16 bit data */
typedef unsigned short  U16;
/** Unsigned 32 bit data*/
typedef unsigned int    U32;
/** Unsigned 64 bit data*/
//typedef unsigned long   U64;
/** Signed 8 bit data */
typedef signed char     S8;
/** Signed 16 bit data */
typedef signed short    S16;
/** Signed 32 bit data */
typedef signed int      S32;
/** Signed 64 bit data */
//typedef signed long     S64;
/** 8-bit text data */
typedef char         T8;
/** 16-bit text data */
typedef U16     T16;

#if !(defined (__BORLANDC__)) && !(defined (_MSC_VER)) && !(defined(TCC7801))
/** Boolean, TRUE/FALSE */
    #define BOOL U8
#endif /* !(defined (__BORLANDC__) && defined (_MSC_VER)) */

#if !(defined (__BORLANDC__)) && !(defined (_MSC_VER)) && !(defined (APPROACH5C))
    /**  void */
    typedef void    VOID;
#endif /* !(defined (__BORLANDC__) && defined (_MSC_VER)) */


#ifdef __BORLANDC__
    typedef int      BOOL;
#endif

#ifdef _MSC_VER 
    typedef int BOOL;
#endif


#ifdef TCC7801
    typedef int BOOL;
#endif


#if defined (_MSC_VER) 
    //#define U64_ZERO 0
    //typedef unsigned __int64 U64_32; /* unsigned 64 bit data */  // TODO - made problems in hardware files...
    #define U64_ZERO {0,0}
    typedef struct {
        U32 low;
        U32 high;
    } U64_32;
    typedef __int64 S64; /* signed 64 bit data */
    typedef __int64 SINT64; /* signed 64 bit data */
    typedef unsigned __int64 U64; /* signed 64 bit data */
#elif defined (__LINUX__)
    #define U64_ZERO {0,0}
    typedef struct {
        U32 low;
        U32 high;
    } U64_32;
    typedef signed long long int S64; /* signed 64 bit data */
    typedef signed long long int SINT64; /* signed 64 bit data */
    typedef unsigned long long int U64; /* signed 64 bit data */
#elif /*defined (__BORLANDC__) || */defined(EN_64BIT_INTEGER)
    #define U64_ZERO 0
    typedef unsigned long long U64; /* unsigned 64 bit data */
    typedef long long S64; /* signed 64 bit data */
    typedef long long SINT64; /* signed 64 bit data */
#else
    // All that are not WCE not Linux (ADS1_2 for instance)
    #define U64_ZERO {0,0}
/** a structure for 64-bit integer    */
    typedef struct {
        U32 low;
        U32 high;
    } U64_32;
    typedef unsigned long long U64; /* unsigned 64 bit data */
/** a structure for 64-bit signed integer    */
    typedef struct {
        S32 low;
        S32 high;
    } S64;
    typedef long long SINT64; /* signed 64 bit data */
#endif

/** Single-precision floating point */
typedef float    FLOAT;
/** Double-precision floating point */
typedef double   DOUBLE;

typedef S16 DATA;
typedef S32 LDATA;

#ifndef NULL
    #define NULL 0
#endif

#ifndef TRUE
    #define TRUE  1
#endif
#ifndef FALSE
    #define FALSE 0
#endif

#if (defined(__TARGET_ARCH_4T) ||defined (__CW32__) || defined (__GCC32__) || defined (__UCSO__)  || defined (__LINUX__) ||\
        defined (__SYMBIAN__) ||defined (__NUCLEUS__) || defined (__BORLANDC__) || defined (_MSC_VER) || defined(MP_211) || defined(APPROACH5C) ||\
        defined (__ARMEL__ ) )
#ifndef LITTLE_ENDIAN
    #define LITTLE_ENDIAN
#endif
#else
#ifndef BIG_ENDIAN
    #define  BIG_ENDIAN
#endif
#endif /*  */

/* This type will be added if it is not exists in our target compiler */
typedef enum {
    cgFALSE = 0 ,
    cgTRUE = 0x7FFF
} cgBool ;

#if defined (WIN32) || defined(WINCE)
    #ifdef CGSETAPI_EXPORTS
        #define CG_SO_SETAPI __declspec(dllexport)
    #else
        #define CG_SO_SETAPI __declspec(dllimport)
    #endif
#else    
    #define CG_SO_SETAPI
#endif

#ifdef __SPECIFIED_ALIGNED__
    #define __STRUCT_PACKED__    __attribute__ ((packed))
#else
    #define __STRUCT_PACKED__
#endif

typedef U16 USHORT;
typedef U8 UCHAR;
//typedef U32 ULONG;

/********************************************************
* 2 Macros
*********************************************************/

#define CG_MAX_S32        (0x7FFFFFFF)
#define CG_MAX_S16        (0x7FFF)

#define MAX_IMSI_LEN        8
#define MAX_ADDR_LEN        256
#define MAX_CONFIGURED_SLP_LEN  64
#define MAX_TARGET_AREAS    32
#define MAX_TARGET_AREA_IDS 256
#define MIN_POLYGON_COORDINATE 3
#define MAX_POLYGON_COORDINATE 15

#define CG_IPADDRESS_V4_LEN     4
#define CG_IPADDRESS_V6_LEN     8

#define MAX_CERT_DIR_LEN    128    //max length of supl certification directory path

#define NI_SM_RTN_CODE_BASE 100
#define NI_SM_RTN_CODE_DECODE_FAILURE          NI_SM_RTN_CODE_BASE +  1
#define NI_SM_RTN_CODE_INVALID_SUPL_VERSION    NI_SM_RTN_CODE_BASE +  2
#define NI_SM_RTN_CODE_UNSUPPORTED_MSG         NI_SM_RTN_CODE_BASE +  3
#define NI_SM_RTN_CODE_INVALID_SESSION_ID      NI_SM_RTN_CODE_BASE +  4
#define NI_SM_RTN_CODE_KEYIDENTITY_ABSENT      NI_SM_RTN_CODE_BASE +  5
#define NI_SM_RTN_CODE_SLPADDRESS_ABSENT       NI_SM_RTN_CODE_BASE +  6
#define NI_SM_RTN_CODE_INVALID_SLPMODE         NI_SM_RTN_CODE_BASE +  7
#define NI_SM_RTN_CODE_PSKTLS_CONNECTION_FAIL  NI_SM_RTN_CODE_BASE +  8
#define NI_SM_RTN_CODE_KEYID_MISSED            NI_SM_RTN_CODE_BASE +  9
#define NI_SM_RTN_CODE_ENCODE_SUPL_INIT_FAILED NI_SM_RTN_CODE_BASE + 10
#define NI_SM_RTN_CODE_MAC_VERI_FAILED         NI_SM_RTN_CODE_BASE + 11
#define NI_SM_RTN_CODE_KEYID_WRONG_SIZE        NI_SM_RTN_CODE_BASE + 12
#define NI_SM_RTN_CODE_KEYID_MISMATCH          NI_SM_RTN_CODE_BASE + 13
#define NI_SM_RTN_CODE_SLPADDRESS_WRONG_FORMAT NI_SM_RTN_CODE_BASE + 14

//GPS assistance data bit map
#define CG_SUPL_DATA_TYPE_ALMANAC                    0x0001
#define CG_SUPL_DATA_TYPE_UTC_MODEL                  0x0002
#define CG_SUPL_DATA_TYPE_IONIC_MODEL                0x0004
#define CG_SUPL_DATA_TYPE_DGPS_CORRECTION            0x0008
#define CG_SUPL_DATA_TYPE_REFERENCE_LOCATION         0x0010
#define CG_SUPL_DATA_TYPE_REFERENCE_TIME             0x0020
#define CG_SUPL_DATA_TYPE_ACQUISITION_ASSISTANCE     0x0040
#define CG_SUPL_DATA_TYPE_REAL_TIME_INTEGRITY        0x0080
#define CG_SUPL_DATA_TYPE_NAVIGATION_MODEL           0x0100
#define CG_SUPL_DATA_TYPE_EPHEMERIS_EXTENSION        0x0200
#define CG_SUPL_DATA_TYPE_EPHEMERIS_EXTENSION_CHECK  0x0400

//add later here: GANSS assistance data bit map

#define MAX_SATS_ON_ALMANAC                   33
#define MAX_SATS_ON_ACQUISASSIST              16
#define CG_SET_API_ERR_UNIMPLEMENTED          (-1)
#define MAX_LTE_NBCELL_NUM                    8
#define NOTIF_RTN_CODE_USER_AGREED            0  
#define NOTIF_RTN_CODE_USER_DENIED            1
#define NOTIF_RTN_CODE_USER_NO_RESPONSE       2

#define ALTITUDE_NOT_VALID (0xFFFF)

#define MAX_3G_FREQ                           8
#define MAX_3G_CELL_MEAS                      32
#define MAX_3G_TIMESLOT                       14

#define AGNSS_NI_SHORT_STRING_MAXLEN          256
#define AGNSS_NI_LONG_STRING_MAXLEN           2048

#define MAX_CP_THIRD_PARTY_ADDR_LEN           18

/* ref to 3GPP TS 24.080 "GANSSAssistanceData"
   Octets 1 to 40 are coded in the same way as the octets 3 to 9+2n of 
   Requested GANSS Data IE in 3GPP TS 49.031 [14]. 
*/
#define CP_MAX_GNSS_REQ_ASS_DATA              40

/*define the FINAL position method that the network chooses*/
#define COMMON_POSMETHOD_NO_POSITION          16
#define COMMON_POSMETHOD_AGPS_MSA             17
#define COMMON_POSMETHOD_AGPS_MSB             18
#define COMMON_POSMETHOD_autonomousGPS        19
#define COMMON_POSMETHOD_AGNSS_MSA            20
#define COMMON_POSMETHOD_AGNSS_MSB            21
#define COMMON_POSMETHOD_autonomousGNSS       22
#define COMMON_POSMETHOD_AFLT                 23
#define COMMON_POSMETHOD_ECID                 24
#define COMMON_POSMETHOD_EOTD                 25
#define COMMON_POSMETHOD_OTDOA                26

/********************************************************
* 3 Enumerates
*********************************************************/

/** file types for ADM (Assistance Data Manager)*/
typedef enum
{
    ECgAgpsBinaryFile  = 0,
    ECgAgpsRawBinaryFile,
    ECgAgpsAsciiFile,
    ECgAgpsAlmanacSemFile,
    ECgAgpsAlmanacYumaFile
} TCgFileType;

typedef enum {
    ECG_BYTE_ORDER_1234 = 0x0,
    ECG_BYTE_ORDER_2143 = 0x1,
    ECG_BYTE_ORDER_3412 = 0x2,
    ECG_BYTE_ORDER_4321 = 0x3
} TCgByteOrder;

typedef enum {
    ECG_SWAP_NONE,
    ECG_SWAP_BYTES,
    ECG_SWAP_WORDS,
    ECG_SWAP_BYTES_AND_WORDS
}TCgByteSwapType;

typedef enum _AgnssWlanLocationEncodingDescriptor_PR {
    AgnssLocationEncodingDescriptor_lci      = 0,
    AgnssLocationEncodingDescriptor_asn1     = 1,
    AgnssLocationEncodingDescriptor_unknown
} AgnssWlanLocationEncodingDescriptor_PR;

typedef enum
{
    TAET_NOTHING,
    TAET_ENTERING,
    TAET_INSIDE,
    TAET_OUTSIDE,
    TAET_LEAVING,
    TAET_END
}TcgAreaEventType_e;

typedef enum
{
    TGAT_NOTHING,
    TGAT_CIRCULAR,
    TGAT_ELLIPTICAL,
    TGAT_POLYGON,
    TGAT_END
}TcgGeographicAreaType_e;

typedef enum
{
    TAIST_BORDER = 0,
    TAIST_WITHIN = 1,
    TAIST_MAX
}TcgAreaIdSetType_e;

typedef enum CgCellInfo_PR {
    CgCellInfo_PR_NOTHING,    /* No components present */
    CgCellInfo_PR_gsmCell,
    CgCellInfo_PR_wcdmaCell,
    CgCellInfo_PR_cdmaCell,
    CgCellInfo_PR_lteCell,
    CgCellInfo_PR_wlanAP,
    CgCellInfo_PR_gsmCellEx,
    CgCellInfo_PR_wcdmaCellEx
} CgCellInfo_PR;

typedef enum _mPathIndic_e
{
    MpathInd_notMeasured    = 0,
    MpathInd_low    = 1,
    MpathInd_medium = 2,
    MpathInd_high   = 3
} mPathInd_e;

typedef enum eCG_PosMethod{
    E_CG_SUPL_SET_CAPABILITIES_AUTONOMOUS           = 0, 
    E_CG_SUPL_SET_CAPABILITIES_AGPS_SET_BASED       = 1, 
    E_CG_SUPL_SET_CAPABILITIES_AGPS_SET_ASSISTED    = 2,   
}eCG_PosMethod;

typedef enum CgThirdParty_PR 
{
    CgThirdParty_PR_NOTHING,    /* No components present */
    CgThirdParty_PR_logicalName,
    CgThirdParty_PR_msisdn,
    CgThirdParty_PR_emailAddr,
    CgThirdParty_PR_sipUrl,
    CgThirdParty_PR_min,
    CgThirdParty_PR_mdn,
    CgThirdParty_PR_uri
} CgThirdParty_PR;

typedef enum CG_SETId_PR {
    CG_SETId_PR_NOTHING,    // No components present
    CG_SETId_PR_msisdn,
    CG_SETId_PR_mdn,        // TODO : Currently un-supported
    CG_SETId_PR_min,        // TODO : Currently un-supported
    CG_SETId_PR_imsi,
    CG_SETId_PR_nai,        // TODO : Currently un-supported
    CG_SETId_PR_iPAddress,  // TODO : Currently un-supported
} CG_SETId_PR;

typedef enum _CgMolrType_e
{
    CG_MOLR_locationEstimate                         = 0,
    CG_MOLR_assistanceData                           = 1,
    CG_MOLR_deCipheringKeys                          = 2,
    CG_MOLR_deferredMo_lrTTTPInitiation              = 3,
    CG_MOLR_deferredMo_lrSelfLocationInitiation      = 4,
    CG_MOLR_deferredMt_lrOrmo_lrTTTPLocationEstimate = 5,
    CG_MOLR_deferredMt_lrOrmo_lrCancellation         = 6,
    CG_MOLR_max
}CgMolrType_e;

typedef enum __CgRspType_e
{
    RSP_AUTONOMOUS,
    RSP_MSB,   // cellguide will call RXN_locationInfo()
    RSP_MSA    // cellguide will call RXN_gpsMeasureInfo()
} CgRspType_e;

typedef enum CgAllowedReportingType{
    CG_POSITIONS_ONLY,
    CG_MEASUREMENTS_ONLY,
    CG_POSITIONS_AND_MEASUREMENTS,
}CgAllowedReportingType_e;

typedef enum CG_NotificationType {
    CG_NotificationType_noNotificationNoVerification          = 0,
    CG_NotificationType_notificationOnly                      = 1,
    CG_NotificationType_notificationAndVerficationAllowedNA   = 2,
    CG_NotificationType_notificationAndVerficationDeniedNA    = 3,
    CG_NotificationType_privacyOverride                       = 4
} CG_e_NotificationType;

typedef enum
{
    VER_HMACHash_Auto       = 0,
    VER_HMACHash_SHA1       = 1,
    VER_HMACHash_SHA256     = 2,
    VER_HMACHash_NotAssign
} VER_HMACHashType_e;

/********************************************************
* 4 Structrues
*********************************************************/
typedef struct _TCgAgpsPosition
{
    DOUBLE latitude;    /** geographical latitude   */
    DOUBLE longitude;   /** geographical longitude  */
    DOUBLE altitude;    /** geographical altitude   */
}__STRUCT_PACKED__ TCgAgpsPosition;

typedef struct  
{
    S16 altitude;    /** height (from database) [meters] */
    U16 err;         /** Height expected error [meters]  */
} TCgHeightData;

typedef struct _TcgVelocity
{
    DOUBLE  horizontalSpeed;   // kmh
    DOUBLE  bearing;
    DOUBLE  verticalSpeed;     // kmh
    int     verticalDirect;    // 0: upward, 1:downward
}__STRUCT_PACKED__ TcgVelocity;

typedef struct {
    unsigned short wYear; // year of AD
    unsigned short wMonth;// 1-12
    unsigned short wDay; // 1-31
    unsigned short wHour; // 0-23
    unsigned short wMinute; // 0-59
    unsigned short wSecond; // 0-59
} TcgTimeStamp;

typedef struct __TCgSetQoP_t
{
    unsigned short flag;          //Include QoP or not Bit 16..8..7..4 3 2 1
    unsigned short QoP_HorAcc;    //Horizonal Accuracy, range:0---127
    unsigned short QoP_VerAcc;    //vertical Accuracy, range:0---127
    unsigned short QoP_MaxLocAge; //Max location age
    unsigned short QoP_Delay;     //QoP Delay.
} TCgSetQoP_t;

typedef struct _AgnssWlanApLocationData_t {
    char              accuracyPresent;   /* TRUE, locationAccuracy is valid; otherwise, locationAccuracy is invalid */
    long              locationAccuracy;  /* optional, range:(0, 4294967295) */
    unsigned short    locationValueLen;
    char              locationValue[AGNSS_MAX_LOCATION_DESC_LEN];
} AgnssWlanApLocationData_t;

typedef struct _AgnssWlanApReportedLocation_t {
    AgnssWlanLocationEncodingDescriptor_PR  locationEncodingDescriptor;
    AgnssWlanApLocationData_t               locationData;
} AgnssWlanApReportedLocation_t;

typedef struct CgGsmCellInformation { 
    long   refMCC; 
    long   refMNC; 
    long   refLAC; 
    long   refCI; 
}CgGsmCellInformation_t;

typedef struct _CgNmrElement_t
{
    unsigned short arfcn;
    unsigned char  bsic;
    unsigned char  rxlev;
}CgNmrElement_t;

typedef struct _CgNmr_t
{
    long           count;
    CgNmrElement_t element[15];
}CgNmr_t;

typedef struct _CgGsmCellInformationEx_t { 
    long           refMCC; 
    long           refMNC; 
    long           refLAC; 
    long           refCI;

    unsigned long  nmrFlag;
    CgNmr_t        nmr;

    unsigned short taFlag;
    unsigned short ta;
}CgGsmCellInformationEx_t;

typedef struct CgCdmaCellInformation { 
    long   refNID; 
    long   refSID; 
    long   refBASEID; 
    long   refBASELAT; 
    long   reBASELONG; 
    long   refREFPN; 
    long   refWeekNumber; 
    long   refSeconds; 
}CgCdmaCellInformation_t;

typedef struct CgWcdmaCellInformation { 
    long   refMCC; 
    long   refMNC;
    long   refUC; 
} CgWcdmaCellInformation_t; 

typedef enum Cg3GMode
{
    Cg3GMode_NOTHING = 0,
    Cg3GMode_FDD = 1,
    Cg3GMode_TDD = 2,
    Cg3GMode_MAX
}Cg3GMode_e;

typedef struct CgFreqInfoFdd
{
    unsigned short uarfcnUlFlag;  /*1(>0):uarfcnUl is valid, 0:invalid */
    unsigned short uarfcnUl;      /*0..16383*/

    unsigned long  uarfcnDl;      /*0..16383*/
}CgFreqInfoFdd_t;

typedef struct CgFreqInfoTdd
{
    unsigned long  uarfcnNt;      /*0..16383*/
}CgFreqInfoTdd_t;

typedef union Cg3GFreqInfo
{
    CgFreqInfoFdd_t fdd;
    CgFreqInfoTdd_t tdd;
}Cg3GFreqInfo_t;

typedef struct CgFrequencyInfo
{
    Cg3GMode_e     mode;
    Cg3GFreqInfo_t choise;
}CgFrequencyInfo_t;

typedef struct Cg3GFddMeasRst
{
    unsigned short priCPICH;         /*0..511*/
    unsigned char  cpichEcN0Flag;
    unsigned char  cpichEcN0;        /*0..63, Max = 49, Values above Max are spare*/

    unsigned char  cpichRscpFlag;
    unsigned char  cpichRscp;        /*0..127*/
    unsigned short pathloss;         /*46..173, Max = 158, Values above Max are spare; 0 - invalid*/
}Cg3GFddMeasRst_t;

typedef struct CgTimeslotIscpList
{
    unsigned short count;
    unsigned char  timeslotIscp[MAX_3G_TIMESLOT]; /*(0..127), Max = 91, Values above Max are spare*/
}CgTimeslotIscpList_t;

typedef struct Cg3GTddMeasRst
{
    unsigned char cellParamId;      /*0..127*/
    unsigned char propTgsnFlag;
    unsigned char propTgsn;         /*(0..14)*/
    unsigned char priCcpchRscpFlag;

    unsigned char priCcpchRscp;     /*(0..127)*/
    unsigned short pathloss;        /*46..173, Max = 158, Values above Max are spare; 0 - invalid*/
    unsigned char timeslotFlag;

    CgTimeslotIscpList_t timeslot;
}Cg3GTddMeasRst_t;

typedef union Cg3GModeMeasRst_t
{
    Cg3GFddMeasRst_t fdd;
    Cg3GTddMeasRst_t tdd;
}Cg3GModeMeasRst_t;

typedef struct Cg3GCellMeasRst
{
    Cg3GMode_e        mode;
    Cg3GModeMeasRst_t choise;
}Cg3GCellMeasRst_t;

typedef struct CgCellMeasRst
{
    unsigned char     cellIdFlag;
    unsigned char     measRstFlag;
    unsigned short    spare;
    
    unsigned long     cellId;   /*0..268435455*/

    Cg3GCellMeasRst_t measRst;
}CgCellMeasRst_t;

typedef struct CgCellMeasRstList
{
    long count;
    CgCellMeasRst_t *pRst;                           //max count: MAX_3G_CELL_MEAS;
}CgCellMeasRstList_t;

typedef struct Cg3GMeasRst
{
    unsigned char     freqInfoFlag;
    unsigned char     carrierRssiFlag;
    unsigned char     cellMeasRstListFlag;
    unsigned char     spare;
    
    CgFrequencyInfo_t freqInfo;
    long              carrierRssi;      /*0..127, Max = 76, Values above Max are spare*/
    CgCellMeasRstList_t cellMeasRstList;
}Cg3GMeasRst_t;

typedef struct Cg3GMeasRstList
{
    long count;
    Cg3GMeasRst_t *pResults;           //max count: MAX_3G_FREQ
}Cg3GMeasRstList_t;

/*Corresponding to 1.0-chip, 0.5-chip and 0.125-chip resolutions, respectively*/
typedef enum CgTaResolution
{
    TARES_10chip   = 0,
    TARES_05chip   = 1,
    TARES_0125chip = 2
}CgTaResolution_t;

typedef enum CgChipRate
{
    CHIPRATE_tdd128 = 0,
    CHIPRATE_tdd384 = 1,
    CHIPRATE_tdd768 = 2
}CgChipRate_t;

typedef struct Cg3GTimingAdvance
{
    unsigned short   ta;            /*(0..8191)*/
    unsigned char    taResFlag;
    unsigned char    chipRateFlag;

    CgTaResolution_t taRes;
    CgChipRate_t     chipRate;
}Cg3GTimingAdvance_t;

typedef struct CgWcdmaCellInformationEx {
    long              refMCC;               /*0..999*/
    long              refMNC;               /*0..999*/
    long              refUC;                /*0..268435455*/

    unsigned char     freqInfoFlag;
    unsigned char     pscFlag;
    unsigned short    priScramblingCode;    /*0..511, Not applicable for FDD*/

    unsigned char     measRstListFlag;
    unsigned char     cellParamIdFlag;
    unsigned char     taFlag;
    unsigned char     spare;                /*for byte alignment*/
    
    CgFrequencyInfo_t freqInfo;
    Cg3GMeasRstList_t measRstList;
    unsigned long     cellParamId;          /*(0..127), Not applicable for FDD*/
    Cg3GTimingAdvance_t ta;                 /*Not applicable for FDD*/
}CgWcdmaCellInformationEx_t;

typedef struct CgLteNbCellInfo {
    long          refPhysCellId;   /* range: 0---503 */
    long          refMCC;          /* optional */
    long          refMNC;          /* optional */
    long          refCellId;       /* 28bits, optional */
    long          refTac;          /* tracking area code: 16bits, optional */
    long          refEarfcn;       /* range: 0---262143, optional */
    long          refSFN;          /* System Frame Number:10bits, optional */
    long          refRxTxTimeDiff; /* 0-4095, optional */

    unsigned char mcc_present;
    unsigned char mnc_present;
    unsigned char refMNCDigits;    /* 2 or 3, number of MCC-MNC-Digit */      
    unsigned char cellId_present;

    unsigned char rsrp_present;    /* 1(>0):refRsrp is valid, 0:invalid */
    unsigned char refRsrp;         /* rsrp: 0---97, optional */
    unsigned char rsrq_present;    /* 1(>0):refRsrq is valid, 0:invalid */
    unsigned char refRsrq;         /* rsrq: 0---34, optional */

    unsigned char tac_present;
    unsigned char earfcn_present;  /* 1(>0):refEarfcn is valid, 0:invalid */
    unsigned char sfn_present;     /* 1(>0):refSFN is valid, 0:invalid */
    unsigned char rxtx_present;    /* 1(>0):refRxTxTimeDiff is valid, 0:invalid */
    
}CgLteNbCellInfo_t;

typedef struct CgLteNbCellList{
    long                numOfNbCell;     /* the count of neighbour cell */
    CgLteNbCellInfo_t  *pNbCellInfo;
}CgLteNbCellList_t;

typedef struct CgLteCellInformation { 
    long          refPhysCellId;   /* range: 0---503 */    
    long          refMCC; 
    long          refMNC;
    long          refCellId;       /* 28bits */
    long          refTac;          /* tracking area code: 16bits */
    long          refEarfcn;       /* range: 0---262143, optional */
    long          refSFN;          /* System Frame Number:10bits, optional */
    long          refRxTxTimeDiff; /* 0-4095, optional */

    unsigned char refMNCDigits;    // 2 or 3, number of MCC-MNC-Digit
    unsigned char rsrp_present;    /* 1(>0):refRsrp is valid, 0:invalid */
    unsigned char refRsrp;         /* rsrp: 0---97, optional */
    unsigned char rsrq_present;    /* 1(>0):refRsrq is valid, 0:invalid */

    unsigned char refRsrq;         /* rsrq: 0---34, optional */
    unsigned char ta_present;      /* 1(>0):refTa is valid, 0:invalid */
    unsigned char refTa;           /* Timing Advance value, range: 0---1282(NTA/16 as per [3GPP 36.213]), optional */
    unsigned char earfcn_present;  /* 1(>0):refEarfcn is valid, 0:invalid */
    
    unsigned char sfn_present;     /* 1(>0):refSFN is valid, 0:invalid */
    unsigned char rxtx_present;    /* 1(>0):refRxTxTimeDiff is valid, 0:invalid */
    unsigned char rsv[2];
    
    CgLteNbCellList_t  refNbCell;           /* optional, if there is neighbourCellInfo,
                                             * phone.c alloc memory, SUPL will free memory;
                                             * if there is no neighbourCellInfo, pRefNbCell = NULL 
                                             */
} CgLteCellInformation_t;

typedef struct CgHplmnInfo {
    long          refMCC;
    long          refMNC;
    unsigned char refMNCDigits;    // 2 or 3, number of MCC-MNC-Digit    
} CgHplmnInfo_t;

typedef struct CgCellInfo {
    CgCellInfo_PR present;
    union CgCellInfo_u {
        CgGsmCellInformation_t      gsmCell;
        CgWcdmaCellInformation_t    wcdmaCell;
        CgCdmaCellInformation_t     cdmaCell;
        CgLteCellInformation_t      lteCell;
        CgGsmCellInformationEx_t    gsmExCell;
        CgWcdmaCellInformationEx_t  wcdmaExCell; 
    } choice;
} CgCellInfo_t;

typedef struct CgLocationId {
    CgCellInfo_t    cellInfo;
    long            status;
} CgLocationId_t;

/* CircularArea */
typedef struct 
{
    TCgAgpsPosition coordinate;  /* Please ignore altitude information */
    long     radius;    // 1..1000000m, should not be less then 50m
    long     radius_min /* OPTIONAL, 1..1000000m */;
    long     radius_max /* OPTIONAL, 1..1500000m */;
} TcgCircularArea_t;

typedef struct
{
    TCgAgpsPosition coordinate;     /* Please ignore altitude information */
    long            semiMajor;      /* 1..1000000, Meter */
    long            semiMajor_min;  /* 1..1000000, Meter, OPTIONAL, 0: not present*/
    long            semiMajor_max;  /* 1..1500000, Meter, OPTIONAL, 0: not present*/
    long            semiMinor;      /* 1..1000000, Meter */
    long            semiMinor_min;  /* 1..1000000, Meter, OPTIONAL, 0: not present*/
    long            semiMinor_max;  /* 1..1500000, Meter, OPTIONAL, 0: not present*/
    long            angle;          /* 0..179, Degree */
}TcgEllipticalArea_t;

typedef struct
{
    long            polygonHysteresis; /* 1..100000, Meter, OPTIONAL, 0: not present */
    unsigned long   count;
    TCgAgpsPosition *pCoordinateList;  /* 3-15 coordinate, Please ignore altitude information */ 
                                      /*[MIN_POLYGON_COORDINATE, MAX_POLYGON_COORDINATE] */
}TcgPolygonArea_t;

typedef union
{
    TcgCircularArea_t   circular;
    TcgEllipticalArea_t elliptical;
    TcgPolygonArea_t    polygon;
}TcgGeoArea_u;

typedef struct
{
    TcgGeographicAreaType_e areaType;

    TcgGeoArea_u choice;
}TcgGeographicArea_t;

typedef struct
{
    unsigned long count;
    TcgGeographicArea_t areaArray[MAX_TARGET_AREAS];
}TcgGeoAreaList_t;

typedef struct
{
    unsigned long      count;
    CgCellInfo_t       *pId; //MAX_TARGET_AREA_IDS
}TcgAreaIdSet_t;

typedef struct
{
    unsigned long      count;
    unsigned long      *pAreaIndex; //MAX_TARGET_AREAS
}TcgGeoAreaMappingList_t;

typedef struct
{
    TcgAreaIdSet_t     idSet;
    
    TcgAreaIdSetType_e idSetType;
    TcgGeoAreaMappingList_t geoMapping;
}TcgAreaIdList_t;

typedef struct
{
    unsigned long      count;
    TcgAreaIdList_t    *pAreaIdList;  //MAX_TARGET_AREAS
}TcgAreaIdLists_t;

typedef struct
{
    TcgAreaEventType_e   type;
    unsigned char        isPosEst;     // 1(TRUE) if network requires position estimate, otherwise 0(FALSE)
    unsigned long        intervalTime; // 1..604800s (7 days), should not be less then 60s
    unsigned long        maxTimes;     // 1..1024
    unsigned long        startTime;    // 0..2678400s (31 days)
    unsigned long        stopTime;     // 0..11318399s (131 days)
    TcgGeoAreaList_t     geoTargetArea;
}TcgAreaEventParams_t;

typedef struct __TcgAgpsMsrElement_t 
{
    unsigned char  satelliteID;  // range: 0-63
    long           cNo;          // carrier noise ratio, range:(0..63)
    float          doppler;      // doppler, range:(-32768/5..32767/5)
    long           wholeChips;   // whole value of the code phase measurement, range:(0..1022)
    long           fracChips;    // fractional value of the code phase measurement, range:(0..1023),
    mPathInd_e     mpathInd;     // multipath indicator
    long           pseuRangeRMSErr; // index, range:(0..63)
} TcgAgpsMsrElement_t;

typedef struct __TcgAgpsMsrSetList_t
{
    long                gpsTOW;    // unit:ms
    unsigned char       listNums;  // range:0-16
    TcgAgpsMsrElement_t msrElement[16];
}TcgAgpsMsrSetList_t;

typedef struct OctetString_t 
{
    unsigned char *buf;  /* Buffer with consecutive OCTET_STRING bits */
    int           size;  /* Size of the buffer */
} OctetString_t;

typedef struct __CgThirdParty_t 
{
    CgThirdParty_PR present;
    union CgThirdParty_u 
    {
        unsigned char logicalName[1000]; // IA5String,     size: 1--1000
        unsigned char msisdn[8];         // OCTET STRING,  size: 8
        unsigned char emailAddr[1000];   // IA5String,     size: 1--1000
        unsigned char sipUri[255];       // VisibleString, size: 1--255
        unsigned char min[34];           // BIT STRING,    size:34
        unsigned char mdn[8];            // OCTET STRING,  size:8
        unsigned char uri[255];          // VisibleString, size: 1--255
    } choice;
} CgThirdParty_t;

typedef struct __CgThirdPartyList_t
{
    int              CgThirdPartyNums;
    CgThirdParty_t   *pCgThirdParty;
}CgThirdPartyList_t;

typedef enum CgIPAddressType {
    CgIpAddress_NOTHING,
    CgIPAddress_V4,
    CgIPAddress_V6
} CgIPAddressType_e;
 
typedef struct CgIPAddress {
    CgIPAddressType_e type;
    union CgIPAddress_u {
        unsigned char  ipv4Address[CG_IPADDRESS_V4_LEN];    /* 4 * 8  = 32bits  */
        unsigned short ipv6Address[CG_IPADDRESS_V6_LEN];    /* 8 * 16 = 128bits */
    } choice;
} CgIPAddress_t;

/*The information of the target SET*/
typedef struct CgSETId{
    CG_SETId_PR     setIdType;
    unsigned char   setIdLen;
    unsigned char   setIdValue[MAX_IMSI_LEN];  // only support imsi/msdn
    CgIPAddress_t   setIpAddress;
}CgSETId_t;

typedef struct __CgPositionParam_t
{
    TCgAgpsPosition    pos;
    TcgVelocity        velocity;
    TcgTimeStamp       time;
}CgPositionParam_t;

typedef struct _CgPostionData_t
{
    CgPositionParam_t pos;
    eCG_PosMethod     pos_method;    
    //gnss pos technology    //ffs
    //gnss signals Info      //ffs
}CgPositionData_t;

typedef struct _CgReportData_t
{
    CgPositionData_t posData;
    //multiple Location IDs  //ffs
    //result Code            //ffs
    //time stamp             //ffs
}CgReportData_t;

typedef struct CgReportDataList
{
    int                numOfReportData;
    CgReportData_t     *reportData;
}CgReportDataList_t;

typedef struct _TcgGbaOctetStr_t
{
    unsigned int  size;
    unsigned char buf[64];
}TcgGbaOctetStr_t;

typedef struct _CgApplicationId_t
{
    unsigned char appProvider_len;  //length of appProvider
    unsigned char appName_len;      //length of appName
    unsigned char appVersion_len;   //length of appVersion

    unsigned char appProvider[24];  //application provider, IA5String, size: 1--24
    unsigned char appName[32];      //application name, IA5String, size: 1--32
    unsigned char appVersion[8];    //optional, application version, IA5String, size: 1--8
}CgApplicationId_t;

typedef struct _CgSetPosTechnologys_t
{
    /* TRUE: if support. Else is FALSE */
    unsigned char  agpsSETassisted;
    unsigned char  agpsSETBased;
    unsigned char  autonomousGPS;
    unsigned char  aFLT;
    unsigned char  ecid;
    unsigned char  eotd;
    unsigned char  otdoa;
    //Add later: Ver2_PosTechnology_extension
    unsigned char  agnssMsa;
    unsigned char  agnssMsb;
    unsigned char  autonomousGnss;
}CgSetPosTechnologys_t;

typedef struct _CgSetEventTriggerCap_t
{
    unsigned char ellipticalArea;   //TRUE: if support. Else is FALSE
    unsigned char polygonArea;      //TRUE: if support. Else is FALSE
    unsigned int  maxNumGeoAreaSupported;
    unsigned int  maxAreaIdListSupported;
    unsigned int  maxAreaIdSupportedPerList;
}CgSetEventTriggerCap_t;

typedef struct _setSupportRepMode_t
{
    unsigned char  realtime;        //TEUE: if support. Else is FALSE
    unsigned char  quasirealtime;   //TEUE: if support. Else is FALSE
    unsigned char  batch;           //TEUE: if support. Else is FALSE
}setSupportRepMode_t;

typedef struct _setBatchRepCap_t
{
    unsigned int   maxNumPositions;      //default value: 60
    unsigned int   maxNumMeasurements;   //default value: 60
}setBatchRepCap_t;

typedef struct _setReportingCap_t
{
    unsigned int         periodicMinInt;
    unsigned int         periodicMaxInt;
    setSupportRepMode_t  repMode;
    setBatchRepCap_t     batchRepCap;
}setReportingCap_t;

typedef struct _setEcidCap_t
{
    unsigned char    isSupRsrp;          /* TRUE: if support. Else is FALSE */
    unsigned char    isSupRsrq;          /* TRUE: if support. Else is FALSE */
    unsigned char    isSupTxRxTimeDiff;  /* TRUE: if support. Else is FALSE */
    unsigned char    rsv;
}setEcidCap_t;

typedef struct _protocolVersion3GPP_t {
    int        majorVersionField;
    int        technicalVersionField;
    int        editorialVersionField;
} protocolVersion3GPP_t;

typedef struct _protocolVersionOMA_t {
    int        majorVersionField;
    int        minorVersionField;
} protocolVersionOMA_t;

typedef struct _setPosProtocol_t {
    unsigned char           rrlpSupport;
    protocolVersion3GPP_t   rrlp;
    unsigned char           rrcSupport;
    protocolVersion3GPP_t   rrc;
    unsigned char           lppSupport;
    protocolVersion3GPP_t   lpp;
    unsigned char           lppeSupport;
    protocolVersionOMA_t    lppe;
} setPosProtocol_t;

typedef struct _CgSetCapability_t
{
    CgSetPosTechnologys_t      *pPosTechnologys;
    CgSetEventTriggerCap_t     *pEventTriggerCap;

    //notification and verification time out value
    unsigned char              notifVerifTimeout;
    //notification time out value
    unsigned char              notifTimeout;

    //whether tls is enabled or not
    unsigned char              isTlsEnabled;
    //certification directory(including file name)
    unsigned char              suplCertPathAvail; //BOOLEAN, TRUE if cert path is available while FALSE not
    char                       SuplCertPath[MAX_CERT_DIR_LEN];
    
    //supported gps assistance data bit map
    //refer to CG_SUPL_DATA_TYPE_ALMANAC
    unsigned short             supportedGPSAssistanceData;
    //Add later here: supported ganss assistance data

    setReportingCap_t          *pReportingCap;

    VER_HMACHashType_e         verHashType; //The Hash algorithm used to calculate Verification of SUPL INIT message

    /*LPPe related capabilities of UE. set to NULL if not support LPPe*/
    LPPeProvCaps_t             *pLppeCaps;
}CgSetCapability_t;

typedef struct TCgRxNTime
{
    unsigned short   weekNo;         /*< week number (from beginning of current week cycle) */
    unsigned int    second;          /*<  seconds (from beginning of current week no) */
    unsigned int    secondFrac;      /*<  seconds fraction (from beginning of current second) units: 1ns */
    unsigned int    sysTime;        // System clock time at the moment of message reception
} TCgRxNTime;

typedef struct __TCgAgpsEphemeris_t
{
    /* Parameter              Ephemeris factor  Almanac factor      Units           */
    /* ---------              ----------------  ---------------     -----           */
    unsigned char  SatID;                  /* ---          -                   ---             */
    unsigned char  SatelliteStatus;        /* ---          -                   Boolean         */
    unsigned char  IODE;                   /* ---          -                   ---             */
    unsigned short toe;                    /* 2^+4         2^12 (alamanacToa)  Sec             */
    unsigned short week_no;                /* ---            -                    Gps Weeek number*/
    signed short   C_rc;                   /* 2^-5         -                   Meters          */
    signed short   C_rs;                   /* 2^-5         -                   Meters          */
    signed short   C_ic;                   /* 2^-29        -                   Radians         */
    signed short   C_is;                   /* 2^-29        -                   Radians         */
    signed short   C_uc;                   /* 2^-29        -                   Radians         */
    signed short   C_us;                   /* 2^-29        -                   Radians         */
    unsigned long  e;                      /* 2^-33        2^-21(almanacE)     ---             */
    signed long    M_0;                    /* 2^-31        2^-23(almanacM0)    Semi-circles    */
    unsigned long  SquareA;                /* 2^-19        2^-11(almanacAPowerHalf)  (Meters)^1/2    */
    signed short   Del_n;                  /* 2^-43        -                   Semi-circles/sec*/
    signed long    OMEGA_0;                /* 2^-31        2^-23(almanacOmega0)Semi-circles    */
    signed long    OMEGA_dot;              /* 2^-43        2^-38(almanacOmegaDot) Semi-circles/sec*/
    unsigned long  I_0;                    /* 2^-31        -                   Semi-circles    */
    signed short   Idot;                   /* 2^-43        -                   Semi-circles/sec*/
    signed long    omega;                  /* 2^-31        2^-23(almanacW)     Semi-circles    */
    signed char    tgd;                    /* 2^-31        -                   Sec             */
    unsigned short t_oc;                   /* 2^+4         -                   Sec             */
    signed long    af0;                    /* 2^-31        2^-20               Sec             */
    signed short   af1;                    /* 2^-43        2^-38               sec/sec         */
    signed char    af2;                    /* 2^-55        -                   sec/sec2        */
}__STRUCT_PACKED__ TCgAgpsEphemeris_t;

typedef struct __TCgAgpsEphemerisCollection_t
{
    unsigned char count;
    unsigned long ephemerisDownloadedMask; // Will indicate if we just downloaded the BCE from the sky, 
                                // and we've not updated the ADM yet
    TCgAgpsEphemeris_t satellite[MAX_SATS_ON_ALMANAC];   
}__STRUCT_PACKED__ TCgAgpsEphemerisCollection_t;

typedef struct __TCgAddionalDopplerFields_t  
{
    double     doppler1;            // range: [-1.0, 0.5]hz/sec 
    double     dopplerUncertainty;  // range: [12.5, 200]hz
}__STRUCT_PACKED__ TCgAddionalDopplerFields_t;

typedef struct __TcgAddionalAngleFields_t 
{
    double     azimuth;   // range:0--384.75 deg
    double     elevation; // range:0--78.75 deg
}__STRUCT_PACKED__ TcgAddionalAngleFields_t;

typedef struct __TCgAcquisElement_t 
{
    unsigned char               svid;           // range:1--64
    double                      doppler0;       // range:-5120...5117.5
    unsigned char               addionalDopplerFlag;    // 1: addionalDoppler is valid
    TCgAddionalDopplerFields_t  addionalDoppler;        
    unsigned short              codePhase;              // range:0--1022 chips
    unsigned char               intCodePhase;           // range:0--19 C/A period
    unsigned char               gpsBitNumber;           // range:0--3
    unsigned char               codePhaseSearchWindow;  // range:0--192 chips
    unsigned char               addionalAngleFlag;      // 1: addionalAngle is valid
    TcgAddionalAngleFields_t    addionalAngle;              
}__STRUCT_PACKED__ TCgAcquisElement_t;

typedef struct __TCgAgpsAcquisAssist_t
{
    unsigned long gpsTime;  // unit: ms
    unsigned char count;    // acquisElement[0]...acquisElement[count-1] is valid value
    TCgAcquisElement_t acquisElement[MAX_SATS_ON_ACQUISASSIST];   
}__STRUCT_PACKED__  TCgAgpsAcquisAssist_t;

typedef struct CgTimeWindow{
    long startTime;    // -525600 - -1, --Time in minutes(one year)
    long stopTime;     // -525599 - 0, --Time in minutes    
}__STRUCT_PACKED__ CgTimeWindow_t;

typedef struct CgReportingCriteria{
    CgTimeWindow_t  timeWindow;         // startTime = stopTime = 0: time window not presented, all stored data consistent
                                        //startTime < stopTime <= 0: [startTime, stopTime]
    long            maxNumOfReports;    // 1-65536, if not present in supl init, supl will set it as 1024
    long            minTimeInterval;    // 1-86400, if not present(= 0), no minimum time interval exists
}__STRUCT_PACKED__ CgReportingCriteria_t;

typedef struct CgHistoricReporting{
    CgAllowedReportingType_e    allowedRptType;  //Mandatory present
    CgReportingCriteria_t       rptCriteria;     //Optional
}__STRUCT_PACKED__ CgHistoricReporting_t;

typedef struct
{
    TCgRxNTime                   refTime;
    TCgAgpsPosition              refLoc;
    TCgAgpsEphemerisCollection_t epheCol;
    TCgAgpsAcquisAssist_t        acqAss;
} TcgAgpsAssData_t;

typedef struct _agps_cp_notify_params_t {
    int handle_id;
    int ni_type;
    int notify_type;
    int requestorID_len;
    int requestorID_dcs;
    char *requestorID;
    int clientName_len;
    int clientName_dcs;
    char *clientName;
} agps_cp_notify_params_t;

/** AgnssNiType constants */
typedef U32 AgnssNiType;
#define AGNSS_NI_TYPE_VOICE              1
#define AGNSS_NI_TYPE_UMTS_SUPL          2
#define AGNSS_NI_TYPE_UMTS_CTRL_PLANE    3

/** AgnssNiNotifyFlags constants */
typedef U32 AgnssNiNotifyFlags;
#define AGNSS_NI_NEED_NOTIFY          0x0001  /** NI requires notification */
#define AGNSS_NI_NEED_VERIFY          0x0002  /** NI requires verification */
#define AGNSS_NI_PRIVACY_OVERRIDE     0x0004  /** NI requires privacy override, no notification/minimal trace */

typedef U32 AgnssUserRspType;
#define AGNSS_NI_RESPONSE_ACCEPT         1
#define AGNSS_NI_RESPONSE_DENY           2
#define AGNSS_NI_RESPONSE_NORESP         3

typedef int AgnssNiEncodingType;
#define AGNSS_ENC_NONE                   0
#define AGNSS_ENC_SUPL_GSM_DEFAULT       1
#define AGNSS_ENC_SUPL_UTF8              2
#define AGNSS_ENC_SUPL_UCS2              3
#define AGNSS_ENC_UNKNOWN                -1

/** Represents an NI request */
typedef struct _AgnssNiNotification_t {
    size_t               size;             /** set to sizeof(GpsNiNotification) */
    int                  notification_id;  /** An ID to associate NI notifications and UI responses */
    AgnssNiType          ni_type;          /** An NI type used to distinguish different categories of NI events */
    AgnssNiNotifyFlags   notify_flags;     /** Notification/verification options, combinations of AgnssNiNotifyFlags constants */
    int                  timeout;          /** Timeout period to wait for user response. Set to 0 for no time out limit. */
    AgnssUserRspType     default_response;    /** Default response when time out. */
    char                 requestor_id[AGNSS_NI_SHORT_STRING_MAXLEN];    /** Requestor ID */
    char                 text[AGNSS_NI_LONG_STRING_MAXLEN];   /** Notification message. It can also be used to store client_id in some cases */
    AgnssNiEncodingType  requestor_id_encoding;    /** Client name decoding scheme */
    AgnssNiEncodingType  text_encoding;    /** Client name decoding scheme */
    /**
     * A pointer to extra data. Format:
     * key_1 = value_1
     * key_2 = value_2
     */
    char               extras[AGNSS_NI_LONG_STRING_MAXLEN];
} AgnssNiNotification_t;

typedef enum _CpSiLocReqType_e
{
    CP_SI_LocReqType_Null,
    CP_SI_LocReqType_AssistanceDataOnly,
    CP_SI_LocReqType_locationEstimate,
    CP_SI_LocReqType_Max
}CpSiLocReqType_e;

typedef enum
{
    EnableType_Disable                   = 0,
    EnableType_EnableRptNmea             = 1,
    EnableType_EnableRptGadShapes        = 2,
    EnableType_EnableRptNmeaAndGadShapes = 3
}CpMolrEnable_e;

typedef enum
{
    MethodType_AutonomousGps     = 0,
    MethodType_AssGps            = 1,
    MethodType_AssGanss          = 2,
    MethodType_AssGpsAndGanss    = 3,
    MethodType_BasicSelfLoc      = 4,
    MethodType_TransTo3rdParty   = 5,
    MethodType_RetriFrom3rdParty = 6
}CpMolrMethod_e;

/* ref to 3GPP TS 27.007 / 8.50 Mobile originated location request +CMOLR */
typedef struct agps_cp_molr_params_s
{
    /* <enable>: integer type. Enables and disables reporting location as a 
       result of a MO-LR. Only one <method> can be enabled at any given time.
                 
       0 Disables reporting and positioning.
       1 Enables reporting of NMEA strings by unsolicited result code +CMOLRN: 
         <NMEA-string>. Lack of data at each timeout is indicated by an 
         unsolicited result code +CMOLRE.
       2 Enables reporting of GAD shapes by unsolicited result code +CMOLRG: 
         <location_parameters>. Lack of data at each timeout is indicated by an 
         unsolicited result code +CMOLRE.
       3 Enables reporting of NMEA strings and GAD shapes by unsolicited result 
         codes +CMOLRG: <location_parameters> and +CMOLRN: <NMEA-string>. Lack 
         of data at each timeout is indicated by an unsolicited result code 
         +CMOLRE. */
    int   enable;

    /* <method>: integer type. Method for MO-LR. The default value is 
       implementation specific.
    
       0 Unassisted GPS. Autonomous GPS only, no use of assistance data.
       1 Assisted GPS.
       2 Assisted GANSS.
       3 Assisted GPS and GANSS.
       4 Basic self location (the network determines the position technology).
       5 Transfer to third party. This method makes the parameters <shape-rep> 
         and <NMEA-rep> irrelevant (any values are accepted and disregarded). 
         The third party address is given in the parameter <thirdparty-address>.
       6 Retrieval from third party. This method is to get the position estimate
         of the third party. The third party address is given in the parameter 
         <third-party-address>. */
    int   method;

    /* <hor-acc-set>: integer type.
    
       0 Horisontal accuracy not set/specified.
       1 Horizontal accuracy set in parameter <hor-acc>. */
    int   hor_acc_set;

    /* <hor-acc>: integer type. Requested accuracy as horizontal uncertainty 
       exponent (refer to 3GPP TS 23.032 subclause 6.2). The value range 
       is 0-127. The default value is implementation specific. */
    int   hor_acc;

    /* <ver-req>: integer type.
    
       0 Vertical coordinate (altitude) is not requested, 2D location fix is 
         acceptable. The parameters <ver-accset> and <ver-acc> do not apply.
       1 Vertical coordinate (altitude) is requested, 3D location fix is 
         required. */
    int   ver_req;

    /* <ver-acc-set>: integer type.
    
       0 Vertical accuracy not set/specified.
       1 Vertical accuracy set/specified in parameter <ver-acc>. */
    int   ver_acc_set;

    /* <ver-acc>: integer type. Requested accuracy as vertical uncertainty 
       exponent (refer to 3GPP TS 23.032 subclause 6.4). The value range 
       is 0-127. The default value is implementation specific. */
    int   ver_acc;

    /* <vel-req>: integer type. Requested velocity type (refer to 3GPP TS 23.032
       subclause 8.6).
    
       0 Velocity not requested.
       1 Horizontal velocity requested.
       2 Horizontal velocity and vertical velocity requested.
       3 Horizontal velocity with uncertainty requested.
       4 Horizontal velocity with uncertainty and vertical velocity with 
         uncertainty requested. */
    int   vel_req;

    /* <rep-mode>: integer type. Reporting mode. The default value is 
       implementation specific.
    
       0 Single report, the timeout for the MO-LR response request is specified 
         by <timeout>.
       1 Periodic reporting, the timeout for each MO-LR response request is 
         specified by <timeout> and the interval between each MO-LR is specified
         by <interval>. */
    int   rep_mode;

    /* <timeout>: integer type. Indicates how long the MS will wait for a 
       response after a MO-LR. The value range is in seconds from 1 to 65535. 
       The default value is implementation specific. */
    int   timeout;

    /* <interval>: integer type. The parameter is applicable to periodic 
       reporting only. Determine the interval between periodic MO-LRs. The value
       range is in seconds from 1 to 65535, and must be greater than or equal to
       <timeout>. The default value is implementation specific. */
    int   interval;

    /* <shape-rep>: integer type. This parameter is a sum of integers each 
       representing a certain GAD shape that will be accepted in the unsolicited
       result code <location_parameters>. Note that only one GAD shape is 
       present per unsolicited result code. The default value is implementation 
       specific.
       
       1  Ellipsoid point.
       2  Ellipsoid point with uncertainty circle.
       4  Ellipsoid point with uncertainty ellipse.
       8  Polygon.
       16 Ellipsoid point with altitude.
       32 Ellipsoid point with altitude and uncertainty ellipsoid.
       64 Ellipsoid arc. */
    int   shape_rep;

    /* <plane>: integer type. The parameter specifies whether the control plane 
       or SUPL will be used for MO-LR.
       
       0 Control plane.
       1 Secure user plane (SUPL). */
    int   plane;

    /* <NMEA-rep>: string type. The supported NMEA strings are specified as a 
       comma separated values inside one string. If the parameter is omitted or 
       an empty string is given, no restrictions apply and all NMEA strings are 
       supported. The default value is that all strings are supported. 
       Example of NMEA strings: "$GPRMC,$GPGSA,$GPGSV" */
    char *NMEA_rep;

    /* <third-party-address>: string type. The parameter is applicable to 
       reporting to third party only, and specifies the address to the third 
       party. This parameter has to be specified when <method> value is set to 
       5 or 6.*/
    char *third_party_addr;
}agps_cp_molr_params_t;

typedef struct _AgnssNeighborLocationIdsList_t {
     /*contain current non-serving cell location Ids info  */
    CgLocationId_t locationId[AGNSS_MAX_LID_SIZE];
    int            LocationIdsCount;
} AgnssNeighborLocationIdsList_t;

typedef struct _CpThirdPartyAddr_t
{
    char           str[MAX_CP_THIRD_PARTY_ADDR_LEN];
}CpThirdPartyAddr_t;

typedef enum _CpPosEventType_e
{
    EvtType_LOCATION,
    EvtType_GNSS_LOCATION,
    EvtType_ASSIST_DATA,
    EvtType_POS_MEAS,
    EvtType_POS_MEAS_REQ,
    EvtType_GPS_MEAS,
    EvtType_OTDOA_MEAS,
    EvtType_GNSS_MEAS,
    EvtType_GPS_ASSIST_REQ,
    EvtType_GNSS_ASSIST_REQ,
    EvtType_OTDOA_ASSIST_REQ,
    EvtType_CAPABILITY_REQ,
    EvtType_CAPABILITIES,
    EvtType_MSG,
    EvtType_POS_ERR,
    EvtType_RESET_ASSIST_DATA,
    EvtType_GPS_GNSS_ASSIST_DATA,
    EvtType_MAX
}CpPosEventType_e;

typedef struct
{
    int   pdu_len;
    char *pdu;
}CpAssistReq_t;

typedef struct
{
    int             isNonLppGpsOnly;
    CpAssistReq_t   nonLppGpsOnly;      //GPS only

    int             isNonLppGnssExclGps;
    CpAssistReq_t   nonLppGnssExclGps;  //other GNSS, excluding GPS

    int             isLpp;              //used in LTE network
    CpAssistReq_t   lpp;
}CpGnssAssistReq_t;

typedef struct _agps_cp_lcs_pos_params_t
{
    /* <enable>: integer type. Enables and disables positioning.
       0 Disables positioning.
       1 Enables positioning. */
    int enable;
    CpPosEventType_e  evtType;
    union
    {
        CpGnssAssistReq_t gnss_assist_req;
        /* todo: Add more */
    }choice;
}agps_cp_lcs_pos_params_t;

typedef enum
{
    SS_ERROR_success                     = 0,
    SS_ERROR_bearerServiceNotProvisioned = 10,
    SS_ERROR_teleServiceNotProvisioned   = 11,
    SS_ERROR_callBarred                  = 13,
    SS_ERROR_illegalSsOperation          = 16,
    SS_ERROR_sSStatus                    = 17,
    SS_ERROR_sSUnavailable               = 18,
    SS_ERROR_sSSubscriptionViolation     = 19,
    SS_ERROR_sSIncompatibility           = 20,
    SS_ERROR_facilityNotSupported        = 21,
    SS_ERROR_sSAbsentSubscriber          = 27,
    SS_ERROR_shortTermDenial             = 29,
    SS_ERROR_longTermDenial              = 30,
    SS_ERROR_systemFailure               = 34,
    SS_ERROR_dataMissing                 = 35,
    SS_ERROR_unexpectedDataValue         = 36,
    SS_ERROR_pwRegistrationFailure       = 37,
    SS_ERROR_negativePwCheck             = 38,
    SS_ERROR_tempFailure                 = 41,
    SS_ERROR_numberOfPwAttemptsViolation = 43,
    SS_ERROR_positionMethodFailure       = 56,
    SS_ERROR_unknownAlphabet             = 71,
    SS_ERROR_ussdBusy                    = 72,
    SS_ERROR_invalidDeflectedNumber      = 125,
    SS_ERROR_sSMaxNumMptrExceeded        = 126,
    SS_ERROR_resuorceUnavailable         = 127,
    SS_ERROR_max
}SS_ERROR_CODE_e;

typedef struct
{
    CpPosEventType_e  evtType;
    union
    {
        /* the error number of SS service MOLR(gps assist request)
           0 --- success, 
           otherwise indicating the error number of SS service from network
         */
        SS_ERROR_CODE_e gps_assist_rsp;
        /* todo: Add more */
    }choise;
}agps_cp_pos_rsp_params_t;

typedef struct
{
    unsigned int    ota_type;
    unsigned int    rrc_state;
    unsigned int    session_priority;
    unsigned int    msg_size;
    unsigned char  *msg_data;
    unsigned int    addl_info_len;
    unsigned char  *addl_info;
}agps_cp_dl_pdu_params_t;

typedef struct
{
    unsigned int    ota_type;      /* over-the-air type (RRLP/RRC/LPP) */
    unsigned int    rrc_msg_type;
    int             isfinal;       /* is final response? */
    unsigned int    msg_size;
    unsigned char  *msg_data;
    unsigned int    addl_info_len;
    unsigned char  *addl_info;
}agps_cp_ul_pdu_params_t;

#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif
