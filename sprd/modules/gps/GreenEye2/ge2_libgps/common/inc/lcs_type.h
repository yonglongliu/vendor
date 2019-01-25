/******************************************************************************
** File Name:  lcs_type.h                                                     *
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
** 2016-01-01            Xu.Wang                   Refactor                   *
******************************************************************************/

#ifndef _LCS_TYPE_H_
#define _LCS_TYPE_H_

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

#include <stddef.h>
#include <stdbool.h>

/* function in lib visiaility define
   ref: https://gcc.gnu.org/wiki/Visibility */
   
#if defined (_WIN32) || defined (__CYGWIN__)   
    #define LCS_HELPER_DLL_IMPORT __declspec(dllimport)   
    #define LCS_HELPER_DLL_EXPORT __declspec(dllexport)   
    #define LCS_HELPER_DLL_LOCAL   
#else   
    #if defined (__GNUC__)  && (__GNUC__ >= 4)
        #define LCS_HELPER_DLL_IMPORT __attribute__ ((visibility("default")))   
        #define LCS_HELPER_DLL_EXPORT __attribute__ ((visibility("default")))   
        #define LCS_HELPER_DLL_LOCAL  __attribute__ ((visibility("hidden")))   
    #else   
        #define LCS_HELPER_DLL_IMPORT   
        #define LCS_HELPER_DLL_EXPORT   
        #define LCS_HELPER_DLL_LOCAL   
    #endif   
#endif   

#ifdef LCS_DLL // defined if LCS is compiled as a DLL, we define it in android makefile   
    #ifdef LCS_DLL_EXPORTS // defined if we are building the LCS DLL (instead of using it)   
        #define LCS_API LCS_HELPER_DLL_EXPORT   
    #else   
        #define LCS_API LCS_HELPER_DLL_IMPORT   
    #endif // LCS_DLL_EXPORTS   
    #define LCS_LOCAL LCS_HELPER_DLL_LOCAL   
#else // LCS_DLL is not defined: this means LCS is a static lib.   
    #define LCS_API   
    #define LCS_LOCAL   
#endif // LCS_DLL   

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

typedef unsigned long GNSS_ASSIST_T;

#define CG_SUPL_DATA_TYPE_NONE                       0x0000
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

// GANSS assistance data bit map

#define CG_SUPL_DATA_TYPE_DNSS_CORRECTION            0x0008 // same as GPS 
#define CG_SUPL_DATA_TYPE_EARTH_ORIENTATION          0x0800
#define CG_SUPL_DATA_TYPE_AUXILIARY_INFO             0x1000
#define CG_SUPL_DATA_TYPE_ADDL_IONIC_MODEL           0x2000

 
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

/*Max supported GNSS number excluding GPS,  according RRLP*/
#define MAX_GNSS_NUM  8 

#define MAX_NAV_MODEL_NUM   32

/*GNSS ID according to RRLP，0 is reserved*/
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

/* LPPe Macro Definitions  */
#define INVALID_AGNSS_SHORT             (-30000)    /*0x8AD0*/

#define MAX_WLAN_IP_ADDR                5
#define MAX_WLAN_TYPES                  5
#define MAX_WLAN_MAP_DATA_REF_ELEMENT   16
#define MAX_WLAN_COVERAGE_AREA_ELEMENT  16
#define MAX_WLAN_AP_SIZE                64
#define MAX_WLAN_CIVIC_ADDR_ELEMENT     128
#define MAX_WLAN_AP_DATA                128
#define MAX_WLAN_DATA_SETS              8
#define MAX_MAC_ADDR_LEN                6

#define AGNSS_MAX_LID_SIZE              (64)  //spec: 64
#define AGNSS_MAX_LOCATION_DESC_LEN     (128/sizeof(long))
#define AGNSS_MAC_ADDR_LEN              (6)

#define agnssFapMACAddress              0x01
#define agnssFapTransmitPower           0x02
#define agnssFapAntennaGain             0x04
#define agnssFapSignaltoNoise           0x08
#define agnssFapDeviceType              0x10
#define agnssFapSignalStrength          0x20
#define agnssFapChannelFrequency        0x40
#define agnssFapRoundTripDelay          0x80
#define agnssFapReportedLocation        0x100
#define agnssFsetTransmitPower          0x200
#define agnssFsetAntennaGain            0x400
#define agnssFsetSignaltoNoise          0x800
#define agnssFsetSignalStrength         0x1000
#define agnssFapSSID                    0x2000
#define agnssFapPhyType                 0x4000
#define agnssFapSigStrengthDelta        0x8000
#define agnssFueSigStrengthDelta        0x10000
#define agnssFapSNRDelta                0x20000
#define agnssFueSNRDelta                0x40000
#define agnssFoperatingClass            0x80000
#define agnssFrelativeTimeStamp         0x100000

/*
iP-Address-support BIT STRING { 
    iPv4 (0),
    iPv6 (1),
    nat (2) } (SIZE(1..8)) OPTIONAL,
*/
#define LPPeIpAddrType_iPv4 (1 << (7-0))
#define LPPeIpAddrType_iPv6 (1 << (7-1))
#define LPPeIpAddrType_nat  (1 << (7-2))
typedef unsigned char LPPeIpAddrTypes_bitmap;

/*
OMA-LPPe-HighAccuracyFormatCapabilities ::= BIT STRING { hAposition(0),
    hAvelocity(1) } (SIZE(1..8))
*/
#define LPPeHAType_position (1 << (7-0))
#define LPPeHAType_velocity (1 << (7-1))
typedef unsigned char LPPeHATypes_bitmap;

/*
OMA-LPPe-FixedAccessTypes ::= BIT STRING { cable (0),
    dsl (1),
    lan (2),
    pstn (3),
    other (4) } (SIZE(1..16))
*/
#define LPPeFixedAccessType_cable (1 << (15-0))
#define LPPeFixedAccessType_dsl   (1 << (15-1))
#define LPPeFixedAccessType_lan   (1 << (15-2))
#define LPPeFixedAccessType_pstn  (1 << (15-3))
#define LPPeFixedAccessType_other (1 << (15-4))
typedef unsigned short LPPeFixedAccessTypes_bitmap;

/*
OMA-LPPe-WirelessAccessTypes ::= BIT STRING { gsm (0),
    utra (1),
    lte (2),
    wimax (3),
    wifi (4),
    other (5) } (SIZE(1..16))
*/
#define LPPeWirelessAccessType_gsm     (1 << (15-0))
#define LPPeWirelessAccessType_utra    (1 << (15-1))
#define LPPeWirelessAccessType_lte     (1 << (15-2))
#define LPPeWirelessAccessType_wimax   (1 << (15-3))
#define LPPeWirelessAccessType_wifi    (1 << (15-4))
#define LPPeWirelessAccessType_other   (1 << (15-5))
typedef unsigned short LPPeWirelessAccessTypes_bitmap;

/*
relativeLocationReportingSupport BIT STRING { geo (0),
    civic (1),
    otherProviders (2) } (SIZE (1..8)),
*/
#define LPPeRefPointCapType_geo            (1 << (7-0))
#define LPPeRefPointCapType_civic          (1 << (7-1))
#define LPPeRefPointCapType_otherProviders (1 << (7-2))
typedef unsigned short LPPeRefPointCapTypes_bitmap;

/* ---表示target支持的WLAN AP E-CID的测量结果的种类，每个bit表示一种类型
wlan-ecid-MeasSupported BIT STRING { 
    apSSID (0),        --- AP的SSID
    apSN (1),          --- AP的S/N，信噪比
    apDevType (2),     --- AP的设备类型
    apPhyType (3),     --- AP的PHY类型
    apRSSI (4),        --- AP的信号到达target的信号强度
    apChanFreq (5),    --- AP的频道号
    apRTD (6),         --- AP的往返时延
    ueTP (7),          --- target transmit power，target的传输功率
    ueAG (8),          --- target antenna gain，target的天线增益
    apRepLoc (9),      --- AP报告的自身的地理位置
    non-serving (10),  --- 是否可以测量未服务的AP
    historic (11),     --- 是否支持AP的历史测量数据
    apTP (12),         --- AP的传输功率
    apAG (13),         --- AP的天线增益
    ueSN (14),         --- target的信噪比
    ueRSSI (15)} (SIZE(1..16))
*/
#define LPPeWlanMsrDataType_apSSID     (1 << (15-0))
#define LPPeWlanMsrDataType_apSN       (1 << (15-1))
#define LPPeWlanMsrDataType_apDevType  (1 << (15-2))
#define LPPeWlanMsrDataType_apPhyType  (1 << (15-3))
#define LPPeWlanMsrDataType_apRSSI     (1 << (15-4))
#define LPPeWlanMsrDataType_apChanFreq (1 << (15-5))
#define LPPeWlanMsrDataType_apRTD      (1 << (15-6))
#define LPPeWlanMsrDataType_ueTP       (1 << (15-7))
#define LPPeWlanMsrDataType_ueAG       (1 << (15-8))
#define LPPeWlanMsrDataType_apRepLoc   (1 << (15-9))
#define LPPeWlanMsrDataType_nonServing (1 << (15-10))
#define LPPeWlanMsrDataType_historic   (1 << (15-11))
#define LPPeWlanMsrDataType_apTP       (1 << (15-12))
#define LPPeWlanMsrDataType_apAG       (1 << (15-13))
#define LPPeWlanMsrDataType_ueSN       (1 << (15-14))
#define LPPeWlanMsrDataType_ueRSSI     (1 << (15-15))
typedef unsigned short LPPeWlanMsrDataTypes_bitmap;

/*
OMA-LPPe-WLAN-AP-Type-List ::= BIT STRING {
    ieee802-11a (0), 
    ieee802-11b (1), 
    ieee802-11g (2), 
    ieee802-11n (3), 
    ieee802-11ac (4), 
    ieee802-11ad (5)} (SIZE (1..16))
*/
#define LPPeWlanApType_802_11a  (1 << (15-0))
#define LPPeWlanApType_802_11b  (1 << (15-1))
#define LPPeWlanApType_802_11g  (1 << (15-2))
#define LPPeWlanApType_802_11n  (1 << (15-3))
#define LPPeWlanApType_802_11ac (1 << (15-4))
#define LPPeWlanApType_802_11ad (1 << (15-5))
typedef unsigned short LPPeWlanApTypes_bitmap;

/* ---描述了target所支持的WLAN AP辅助数据的种类
wlan-ap-ADSupported BIT STRING { 
    aplist (0),               --- 必选的AP数据
    aplocation (1),           --- AP的位置
    locationreliability (2),  --- AP位置的可靠性
    transmit-power (3),       --- 传输功率
    antenna-gain (4),         --- 天线增益
    coveragearea (5),         --- 覆盖范围
    non-serving (6)           ---是否支持未服务的AP
    } (SIZE(1..16))
*/
#define LPPeWlanApAssDataType_apList              (1 << (15-0))
#define LPPeWlanApAssDataType_apLocation          (1 << (15-1))
#define LPPeWlanApAssDataType_locationReliability (1 << (15-2))
#define LPPeWlanApAssDataType_transmitPower       (1 << (15-3))
#define LPPeWlanApAssDataType_antennaGain         (1 << (15-4))
#define LPPeWlanApAssDataType_coverageArea        (1 << (15-5))
#define LPPeWlanApAssDataType_nonAServing         (1 << (15-6))
typedef unsigned short LPPeWlanApAssDataTypes_bitmap;

/* ---描述了target支持的扩展的E-CID测量能力
additional-wlan-ecid-MeasSupported BIT STRING {
    oc (0)} (SIZE(1..16))
*/
#define LPPeWlanAddlMsrDataType_operatingClass (1 << (15-0))
typedef unsigned short LPPeWlanAddlMsrDataTypes_bitmap;

/*LPPe bitmap of Wlan AP 802.11a supported channels,
  ref to OMA-TS-LPPe: "Supported-Channels-11a" */
#define LPPeWlanChan80211a_34   (1 << 0)
#define LPPeWlanChan80211a_36   (1 << 1)
#define LPPeWlanChan80211a_38   (1 << 2)
#define LPPeWlanChan80211a_40   (1 << 3)
#define LPPeWlanChan80211a_42   (1 << 4)
#define LPPeWlanChan80211a_44   (1 << 5)
#define LPPeWlanChan80211a_46   (1 << 6)
#define LPPeWlanChan80211a_48   (1 << 7)
#define LPPeWlanChan80211a_52   (1 << 8)
#define LPPeWlanChan80211a_56   (1 << 9)
#define LPPeWlanChan80211a_60   (1 << 10)
#define LPPeWlanChan80211a_64   (1 << 11)
#define LPPeWlanChan80211a_149  (1 << 12)
#define LPPeWlanChan80211a_153  (1 << 13)
#define LPPeWlanChan80211a_157  (1 << 14)
#define LPPeWlanChan80211a_161  (1 << 15)
typedef unsigned int LPPeWlanChans80211a_bitmap;

/*LPPe bitmap of Wlan AP 802.11bg supported channels,
  ref to OMA-TS-LPPe: "Supported-Channels-11bg" */
#define LPPeWlanChan80211bg_1   (1 << 0)
#define LPPeWlanChan80211bg_2   (1 << 1)
#define LPPeWlanChan80211bg_3   (1 << 2)
#define LPPeWlanChan80211bg_4   (1 << 3)
#define LPPeWlanChan80211bg_5   (1 << 4)
#define LPPeWlanChan80211bg_6   (1 << 5)
#define LPPeWlanChan80211bg_7   (1 << 6)
#define LPPeWlanChan80211bg_8   (1 << 7)
#define LPPeWlanChan80211bg_9   (1 << 8)
#define LPPeWlanChan80211bg_10  (1 << 9)
#define LPPeWlanChan80211bg_11  (1 << 10)
#define LPPeWlanChan80211bg_12  (1 << 11)
#define LPPeWlanChan80211bg_13  (1 << 12)
#define LPPeWlanChan80211bg_14  (1 << 13)
typedef unsigned int LPPeWlanChans80211bg_bitmap;


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

/* LPPe Error Code */
typedef enum
{
    LPPeErrCode_ok = 0,
    LPPeErrCode_nullPoint,
    LPPeErrCode_wrongParams,
    LPPeErrCode_allocMemoryFailed,
    LPPeErrCode_encodeFailed,
    LPPeErrCode_decodeFailed,
    LPPeErrCode_compLevelUnsupport,
    LPPeErrCode_versionUnsupport,
    LPPeErrCode_msgTypeUnsupport,
    LPPeErrCode_lppeModeUnsupport,
    LPPeErrCode_max,
}LPPeErrCode_e;

/* LPPe Periodic request type, ref to OMA-TS-LPPe: "TypeOfADRequest" */
typedef enum
{
    LPPePeriodReqType_initialRequest                 = 0,
    LPPePeriodReqType_updateAndContinueIfUpdateFails = 1,
    LPPePeriodReqType_updateAndAbortIfUpdateFails    = 2,
    LPPePeriodReqType_max
}LPPePeriodReqType_e;

/* LPPe Periodic response type, ref to OMA-TS-LPPe: "typeOfADProvide" */
typedef enum
{
    LPPePeriodRspType_responseToInitialRequest      = 0,
    LPPePeriodRspType_providePeriodicAD             = 1,
    LPPePeriodRspType_responseToTargetUpdateRequest = 2,
    LPPePeriodRspType_serverUpdate                  = 3,
    LPPePeriodRspType_max
}LPPePeriodRspType_e;

/* LPPe IP Address type, ref to OMA-TS-LPPe: "iP-Address-support" */
typedef enum
{
    LPPeIpAddrType_nothing = 0,
    LPPeIpAddrType_ipv4    = 1,
    LPPeIpAddrType_ipv6    = 2
}LPPeIpAddrType_e;

/* LPPe Bearer type, ref to OMA-TS-LPPe: "OMA-LPPe-Bearer" */
typedef enum
{
    LPPeBearer_unknown  = 0,
    LPPeBearer_gsm      = 1,
    LPPeBearer_utran    = 2,
    LPPeBearer_lte      = 3,
    LPPeBearer_wlan     = 4,
    LPPeBearer_wimax    = 5,
    LPPeBearer_dsl      = 6,
    LPPeBearer_pktcable = 7,
    LPPeBearer_other    = 8,
    LPPeBearer_max
}LPPeBearer_e;

/* LPPe TimeStamp type, ref to OMA-TS-LPPe: "OMA-LPPe-TimeStamp" */
typedef enum
{
    LPPeTimeStamp_nothing      = 0,
    LPPeTimeStamp_gnssTime     = 1,
    LPPeTimeStamp_networkTime  = 2,
    LPPeTimeStamp_relativeTime = 3
}LPPeTimeStamp_e;

/* LPPe Vendor or Operator ID type, ref to OMA-TS-LPPe: "OMA-LPPe-VendorOrOperatorID" */
typedef enum
{
    LPPeVendorId_nothing = 0,
    LPPeVendorId_std     = 1,
    LPPeVendorId_nonStd  = 2
}LPPeVendorId_e;

/* LPPe Map Data URI type, ref to OMA-TS-LPPe: "mapDataUrl" */
typedef enum
{
    LPPeMapDataUriType_nothing = 0,
    LPPeMapDataUriType_uri     = 1,
    LPPeMapDataUriType_dataRef = 2
}LPPeMapDataUriType_e;

/* LPPe Map provider type, ref to OMA-TS-LPPe: "mapProvider" */
typedef enum
{
    LPPeMapProviderType_nothing         = 0,
    LPPeMapProviderType_isSameAsRefPnt  = 1,
    LPPeMapProviderType_notSameAsRefPnt = 2
}LPPeMapProviderType_e;

/* LPPe Map Association type, ref to OMA-TS-LPPe: "mapAssociation" */
typedef enum
{
    LPPeMapAssociationType_nothing   = 0,
    LPPeMapAssociationType_isRefPnt  = 1,
    LPPeMapAssociationType_otherID   = 2,
    LPPeMapAssociationType_mapOffset = 3,
    LPPeMapAssociationType_isOrigin  = 4
}LPPeMapAssociationType_e;

/* latitude Sign type, ref to 3GPP TS 36.355: "latitudeSign" */
typedef enum
{
    LPPeLatitudeSign_north = 0,
    LPPeLatitudeSign_south = 1,
    LPPeLatitudeSign_max
}LPPeLatitudeSign_e;

/* altitude direction type, ref to 3GPP TS 36.355: "altitudeDirection" */
typedef enum
{
    LPPeAltDirection_height = 0,
    LPPeAltDirection_depth  = 1,
    LPPeAltDirection_max
}LPPeAltDirection_e;

/* LPPe reference Point Geographic Location type, 
   ref to OMA-TS-LPPe: "referencePointGeographicLocation" */
typedef enum
{
    LPPeRefPntGeoLocType_nothing                   = 0,
    LPPeRefPntGeoLocType_location3D                = 1,
    LPPeRefPntGeoLocType_location3DwithUncertainty = 2,
    LPPeRefPntGeoLocType_locationwithhighaccuracy  = 3
}LPPeRefPntGeoLocType_e;

/*LPPe relative location unit type, 
   ref to OMA-TS-LPPe: OMA-LPPe-RelativeLocation.units*/
typedef enum
{
    LPPeRelLocUnits_cm  = 0,
    LPPeRelLocUnits_dm  = 1,
    LPPeRelLocUnits_m10 = 2,
    LPPeRelLocUnits_max
}LPPeRelLocUnits_e;

/*LPPe arc second unit type, 
  ref to OMA-TS-LPPe: OMA-LPPe-RelativeLocation.arc-second-units*/
typedef enum
{
    LPPeArcSecUnits_as0_0003 = 0,
    LPPeArcSecUnits_as0_003  = 1,
    LPPeArcSecUnits_as0_03   = 2,
    LPPeArcSecUnits_as0_3    = 3,
    LPPeArcSecUnits_max
}LPPeArcSecUnits_e;

/*LPPe Uncertainty Shape type, 
  ref to OMA-TS-LPPe: OMA-LPPe-HorizontalUncertaintyAndConfidence.uncShape*/
typedef enum
{
    LPPeUncShapeType_nothing = 0,
    LPPeUncShapeType_circle  = 1,
    LPPeUncShapeType_ellipse = 2,
    LPPeUncShapeType_max
}LPPeUncShapeType_e;

/*LPPe reference location type, 
  ref to OMA-TS-LPPe: OMA-LPPe-WLANFemtoCoverageAreaElement*/
typedef enum
{
    LPPeRefLocationType_antenna        = 0,
    LPPeRefLocationType_referencePoint = 1,
    LPPeRefLocationType_max
}LPPeRefLocationType_e;

/*LPPe Wlan Coverage Area Element type, 
  ref to OMA-TS-LPPe: OMA-LPPe-WLANFemtoCoverageAreaElement.type*/
typedef enum
{
    LPPeWlanCovAreaEleType_indoor  = 0,
    LPPeWlanCovAreaEleType_outdoor = 1,
    LPPeWlanCovAreaEleType_mixed   = 2,
    LPPeWlanCovAreaEleType_max
}LPPeWlanCovAreaEleType_e;

/*LPPe Wlan Coverage Area type,
  ref to OMA-TS-LPPe: OMA-LPPe-WLANFemtoCoverageArea.areaType*/
typedef enum
{
    LPPeWlanCovAreaType_gaussian           = 0,
    LPPeWlanCovAreaType_binaryDistribution = 1,
    LPPeWlanCovAreaType_max
}LPPeWlanCovAreaType_e;

/*LPPe Wlan AP type,
  ref to OMA-TS-LPPe: OMA-LPPe-WLAN-AP-Type*/
typedef enum
{
    LPPeWlanApType_ieee802_11a = 0,
    LPPeWlanApType_ieee802_11b,
    LPPeWlanApType_ieee802_11g,
    LPPeWlanApType_ieee802_11n,
    LPPeWlanApType_ieee802_11ac,
    LPPeWlanApType_ieee802_11ad,
    LPPeWlanApType_max
}LPPeWlanApType_e;

/*LPPe Wlan AP server error type,
  ref to OMA-TS-LPPe: OMA-LPPe-AGNSS-LocationServerErrorCauses*/
typedef enum
{
    LPPeWlanApServerErrCause_undefined                  = 0,
    LPPeWlanApServerErrCause_requestedADNotAvailable    = 1,
    LPPeWlanApServerErrCause_notAllrequestedADAvailable = 2,
    LPPeWlanApServerErrCause_max
}LPPeWlanApServerErrCause_e;

/*LPPe Wlan AP target error type,
  ref to OMA-TS-LPPe: OMA-LPPe-WLAN-AP-TargetDeviceErrorCauses*/
typedef enum
{
    LPPeWlanApTargetErrCause_undefined                           = 0,
    LPPeWlanApTargetErrCause_requestedMeasurementsNotAvailable   = 1,
    LPPeWlanApTargetErrCause_notAllrequestedMeasurementsPossible = 2,
    LPPeWlanApTargetErrCause_max
}LPPeWlanApTargetErrCause_e;

/*LPPe Wlan AP error choice,
  ref to OMA-TS-LPPe: OMA-LPPe-WLAN-AP-Error*/
typedef enum
{
    LPPeWlanApErrType_nothing        = 0,
    LPPeWlanApErrType_locationServer = 1,
    LPPeWlanApErrType_targetDevice   = 2
}LPPeWlanApErrType_e;

typedef enum _AgnssWlanType_PR {
    AGNSS_WLAN802_11A,
    AGNSS_WLAN802_11B,
    AGNSS_WLAN802_11G,
    AGNSS_WLAN802_11N,
    AGNSS_WLAN802_11AC,
    AGNSS_WLAN802_11AD,
    AGNSS_WLAN802_UNKNOWN
} AgnssWlanType_PR;

typedef enum _AgnssWlanRTDUnits_e {
    AgnssRTDUnits_microseconds                = 0,
    AgnssRTDUnits_hundredsofnanoseconds       = 1,
    AgnssRTDUnits_tensofnanoseconds           = 2,
    AgnssRTDUnits_nanoseconds                 = 3,
    AgnssRTDUnits_tenthsofnanoseconds         = 4,
    AgnssRTDUnits_unknown
} AgnssWlanRTDUnits_e;

typedef enum
{
    LPPeWlanAltitudeType_Meter    = 0,
    LPPeWlanAltitudeType_floorNum = 1,
    LPPeWlanAltitudeType_max
}LPPeWlanAltitudeType_e;

typedef enum
{
    LPPeWlanDataCoordinate_WGS84  = 0,
    LPPeWlanDataCoordinate_NAVD88 = 1,
    LPPeWlanDataCoordinate_MLLW   = 2,
    LPPeWlanDataCoordinate_max
}LPPeWlanDataCoordinate_e;

/*
OMA-LPPe-WLAN-AP-PHY-Type ::= ENUMERATED { unknown, any, 
  fhss,         --- Frequency Hopping Spread Spectrum， 调频扩频
  dsss,         --- Direct Sequence Spread Spectrum，直序扩频
  irbaseband,  --- IR baseband
  ofdm,        --- Orthogonal Frequency Division Multiplexing，正交频分复用
  hrdsss,      --- HRDSSS
  erp,         --- ERP
  ht,          --- HT
  ihv,         --- IHV
  ... } 
*/
typedef enum
{
    LPPeWlanApPhyType_unknown    = 0,
    LPPeWlanApPhyType_any        = 1,
    LPPeWlanApPhyType_fhss       = 2,
    LPPeWlanApPhyType_dsss       = 3,
    LPPeWlanApPhyType_irBaseband = 4,
    LPPeWlanApPhyType_ofdm       = 5,
    LPPeWlanApPhyType_hrDsss     = 6,
    LPPeWlanApPhyType_erp        = 7,
    LPPeWlanApPhyType_ht         = 8,
    LPPeWlanApPhyType_ihv        = 9,
    LPPeWlanApPhyType_max
}LPPeWlanApPhyType_e;

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

/* from 3GPP-LPP 36.355
EllipsoidPointWithAltitude ::= SEQUENCE { 
    latitudeSign ENUMERATED {north, south}, 
    degreesLatitude INTEGER (0..8388607), -- 23 bit field 
    degreesLongitude INTEGER (-8388608..8388607), -- 24 bit field 
    altitudeDirection ENUMERATED {height, depth}, 
    altitude INTEGER (0..32767) -- 15 bit field 
}
*/
typedef struct
{
    LPPeLatitudeSign_e  eLatSign;
    int                 latDegrees;  /*(0..8388607), -- 23 bit field*/
    int                 longDegrees; /*(-8388608..8388607), -- 24 bit field*/
    LPPeAltDirection_e  eAltDir;
    int                 alt;         /*(0..32767) -- 15 bit field*/
}EllipPntWithAlt_t;

/* from 3GPP-LPP 36.355
EllipsoidPointWithAltitudeAndUncertaintyEllipsoid ::= SEQUENCE { 
    latitudeSign ENUMERATED {north, south}, 
    degreesLatitude INTEGER (0..8388607), -- 23 bit field 
    degreesLongitude INTEGER (-8388608..8388607), -- 24 bit field 
    altitudeDirection ENUMERATED {height, depth}, 
    altitude INTEGER (0..32767), -- 15 bit field 
    uncertaintySemiMajor INTEGER (0..127), 
    uncertaintySemiMinor INTEGER (0..127), 
    orientationMajorAxis INTEGER (0..179),
    uncertaintyAltitude INTEGER (0..127), 
    confidence INTEGER (0..100) 
}
*/
typedef struct
{
    LPPeLatitudeSign_e  eLatSign;
    int                 latDegrees;   /*(0..8388607), -- 23 bit field*/
    int                 longDegrees;  /*(-8388608..8388607), -- 24 bit field*/
    LPPeAltDirection_e  eAltDir;
    int                 alt;          /*(0..32767) -- 15 bit field*/
    unsigned char       uncSemiMajor; /*(0..127)*/
    unsigned char       uncSemiMinor; /*(0..127)*/
    unsigned char       oriMajorAxis; /*(0..179)*/
    unsigned char       uncAlt;       /*(0..127)*/
    unsigned char       confidence;   /*(0..100)*/
}EllipPntWithAltAndUncEllip_t;

typedef struct
{
    int mcc;
    int mnc;
    int mnc_digit_num; /*2-3 digits*/
}LPPePlmnId_t;

/*
OMA-LPPe-VendorOrOperatorID ::= CHOICE {
    standard-VendorOrOperatorID INTEGER(1..1024),
    nonStandard-VendorOrOperatorID OMA-LPPe-NonStandard-VendorOrOperatorID,
    ...
}
OMA-LPPe-NonStandard-VendorOrOperatorID ::= SEQUENCE {
    encodedID INTEGER(0..65535),
    visibleIdentification OMA-LPPe-CharArray OPTIONAL,
    ...
}

OMA-LPPe-CharArray ::= VisibleString
    (FROM ("a".."z" | "A".."Z" | "0".."9" | ".-"))(SIZE (1..31))
*/
typedef struct
{
    char str[32];   /*terminated by '\0' */
}LPPeCharArray_t;

typedef struct
{
    short           encodedId;  /*(0..65535)*/
    LPPeCharArray_t array;
}LPPeNonStdVendorOrOperId_t;

typedef union
{
    short                      stdId;    /*(1..1024)*/
    LPPeNonStdVendorOrOperId_t nonStdId;
}LPPeVendorOrOperId_u;

/*
OMA-LPPe-ReferencePointUniqueID ::= SEQUENCE {
    providerID OMA-LPPe-VendorOrOperatorID,
    providerAssignedID OCTET STRING,
    version INTEGER (1..64),
    ...
}
*/
typedef struct
{
    int            count;
    unsigned char *pOctets;
}LPPeOctetStr_t;

typedef struct
{
    LPPeVendorId_e       eProviderID;
    LPPeVendorOrOperId_u providerID;
    
    LPPeOctetStr_t       providerAssignedID;
    short                version;   /*(1..64)*/
}LPPeRefPointUniqueId_t;

/*
OMA-LPPe-CivicLocation ::= SEQUENCE {
    countryCode OCTET STRING (SIZE (2)),
    civicAddressElementList OMA-LPPe-CivicAddressElementList,
    ...
}
OMA-LPPe-CivicAddressElementList ::= SEQUENCE (SIZE (1..128)) OF OMA-LPPe-CivicAddressElement
OMA-LPPe-CivicAddressElement ::= SEQUENCE {
    caType INTEGER(0..511),
    caValue OCTET STRING (SIZE (1..256)),
    ...
}
*/
typedef struct
{
    short          caType;   /*(0..511)*/
    LPPeOctetStr_t caValue;  /*(1..256) octets*/
}LPPeCivicAddrEle_t;

typedef struct
{
    unsigned char       countryCode[2];

    char                addrCount;  /*(1..128)*/
    LPPeCivicAddrEle_t *pAddr;
}LPPeCivicLoc_t;

/*
OMA-LPPe-GeodeticRelativeAltitude ::= SEQUENCE {
    geodetic-height-depth INTEGER (-32768..32767),
    geodetic-uncertainty-and-confidence OMA-LPPe-GeodeticUncertaintyAndConfidence OPTIONAL,
    ...
}
OMA-LPPe-GeodeticUncertaintyAndConfidence ::= SEQUENCE {
    uncertainty INTEGER (0..127),
    confidence INTEGER (0..99) OPTIONAL,
    ...
}*/
typedef struct
{
    short uncertainty; /*(0..127)*/
    short confidence;  /*OPTIONAL, (0..99), set to INVALID_AGNSS_SHORT if invalid*/
}LPPeUncAndConf_t;

typedef struct
{
    short            heigthDepth;  /*(-32768..32767)*/

    char             uncAndConfFlag;
    LPPeUncAndConf_t uncAndConf;   /*OPTIONAL*/
}LPPeGeoRelAltitude_t;

/*
OMA-LPPe-CivicRelativeAltitude ::= SEQUENCE {
    civic-floors INTEGER (-255..256),
    civic-uncertainty-and-confidence OMA-LPPe-CivicUncertaintyAndConfidence OPTIONAL,
    ...
}
OMA-LPPe-CivicUncertaintyAndConfidence ::= SEQUENCE {
    uncertainty INTEGER (0..127),
    confidence INTEGER (0..99) OPTIONAL,
    ...
}
*/
typedef struct
{
    short            floors;       /*(-255..256)*/

    char             uncAndConfFlag;
    LPPeUncAndConf_t uncAndConf;   /*OPTIONAL*/
}LPPeCivicRelAltitude_t;

/*
OMA-LPPe-HorizontalUncertaintyAndConfidence ::= SEQUENCE {
    uncShape CHOICE {
        circle INTEGER (0..127),
        ellipse SEQUENCE {
            semimajor INTEGER (0..127),
            semiminor INTEGER (0..127),
            offsetAngle INTEGER (0..179)
        },
        ...
    },
    confidence INTEGER (0..99) OPTIONAL,
    ...
}
*/
typedef struct
{
    unsigned char semimajor;
    unsigned char semiminor;
    unsigned char offsetAngle;
}LPPeUncShapeEllipse_t;

typedef union
{
    unsigned char         circle;
    LPPeUncShapeEllipse_t ellipse;
}LPPeUncShape_u;

typedef struct
{
    LPPeUncShapeType_e eUncShape;
    LPPeUncShape_u     uncShape;
    short              confidence; /*OPTIONAL, (0..99), set to INVALID_AGNSS_SHORT if invalid*/
}LPPeHorUncAndConf_t;

/*
OMA-LPPe-RelativeAltitude ::= SEQUENCE {
    geodeticRelativeAltitude OMA-LPPe-GeodeticRelativeAltitude OPTIONAL,
    civicRelativeAltitude OMA-LPPe-CivicRelativeAltitude OPTIONAL,
    ...
}*/
typedef struct
{
    char                   geoRelAltFlag;
    LPPeGeoRelAltitude_t   geoRelAlt;       /*OPTIONAL*/

    char                   civicRelAltFlag;
    LPPeCivicRelAltitude_t civicRelAlt;     /*OPTIONAL*/
}LPPeRelAltitude_t;

/*
OMA-LPPe-RelativeLocation ::= SEQUENCE {
    units ENUMERATED {cm,dm, m10, ...} OPTIONAL,
    arc-second-units ENUMERATED {as0-0003, as0-003, as0-03, as0-3, ...} OPTIONAL,
    relativeNorth INTEGER (-524288..524287),
    relativeEast INTEGER (-524288..524287),
    relativeAltitude OMA-LPPe-RelativeAltitude OPTIONAL,
    horizontalUncertainty OMA-LPPe-HorizontalUncertaintyAndConfidence OPTIONAL,
    ...
}
*/
typedef struct
{
    LPPeRelLocUnits_e   units;  /*OPTIONAL, set to LPPeRelLocUnits_max if invalid*/
    LPPeArcSecUnits_e   asUnits;/*OPTIONAL, set to LPPeArcSecUnits_max if invalid*/

    int                 relNorth;
    int                 relEast;

    char                relAltitudeFlag;
    LPPeRelAltitude_t   relAltitude;    /*OPTIONAL*/

    char                horUncAndConfFlag;
    LPPeHorUncAndConf_t horUncAndConf;  /*OPTIONAL*/
}LPPeRelLoc_t;

/*
OMA-LPPe-ReferencePointRelationship ::= SEQUENCE {
    referencePointUniqueID OMA-LPPe-ReferencePointUniqueID,
    relativeLocation OMA-LPPe-RelativeLocation,
    ...
}
*/
typedef struct
{
    LPPeRefPointUniqueId_t uniID;
    LPPeRelLoc_t           relLoc;
}LPPeRefPntRelationship_t;

/*
OMA-LPPe-Uri ::= VisibleString (FROM ( "a".."z" | "A".."Z" | "0".."9" | ":" | 
    "/" | "?" | "#" | "[" | "]" | "@" | "!" | "$" | "&" | "'" | "(" | ")" | 
    "*" | "+" | "," | ";" | "=" | "-" | "." | "_" | "~" | "%" ))
*/
typedef struct
{
    LPPeOctetStr_t str; /*represent as octet string*/
}LPPeUri_t;

/*
OMA-LPPe-AssistanceContainerID ::= SEQUENCE {
    containerID INTEGER (0..65535),
    ...
}
*/
typedef struct
{
    short containerID;  /*(0..65535)*/
}LPPeAssContainerID_t;

/*
OMA-LPPe-MapDataReference ::= SEQUENCE {
    dataID OMA-LPPe-AssistanceContainerID,
    mapReference OCTET STRING (SIZE (1..64)),
    mapSize INTEGER (1..5000) OPTIONAL,
    ...
}
*/
typedef struct
{
    LPPeAssContainerID_t dataID;

    char                 mapRefCount;   /*(1..64)*/
    unsigned char        mapRef[64];

    short                mapSize; /*OPTIONAL, range (1..5000), set to 0 if invalid*/
}LPPeMapDataReference_t;



typedef union
{
    LPPeUri_t              Uri;
    LPPeMapDataReference_t dataRef;
}LPPeMapDataUri_u;

typedef union
{
    char                 isSameAsRefPointProvider;

    LPPeVendorId_e       eNotSameAsRefPoint;
    LPPeVendorOrOperId_u notSameAsRefPointProvider;
}LPPeMapProvider_u;

typedef union
{
    char           isRefPntUniID;
    LPPeOctetStr_t otherID;         /*(1..64)*/
    LPPeRelLoc_t   mapOffset;
    char           isOrigin;
}LPPeMapAssociation_u;

/*
OMA-LPPe-MapDataInformation ::= SEQUENCE (SIZE (1..16)) OF OMA-LPPe-MapDataReferenceElement

OMA-LPPe-MapDataReferenceElement ::= SEQUENCE {
    mapDataUri CHOICE {
        mapDataUri OMA-LPPe-Uri,
        mapDataRef OMA-LPPe-MapDataReference
    },
    mapProvider CHOICE {
        sameAsRefPointProvider NULL,
        notSameAsRefPointProvider OMA-LPPe-VendorOrOperatorID,
        ...
    } OPTIONAL,
    mapAssociation CHOICE {
        referencePointUniqueID NULL,
        otherID VisibleString (SIZE (1..64)),
        mapOffset OMA-LPPe-RelativeLocation,
        origin NULL,
        ...
    },
    mapHorizontalOrientation INTEGER (0..359) OPTIONAL,
    ...
}
*/
typedef struct
{
    LPPeMapDataUriType_e  eUri;
    LPPeMapDataUri_u      Uri;

    char                  providerFlag;
    LPPeMapProviderType_e eProvider;
    LPPeMapProvider_u     provider; /*OPTIONAL, set to MAP_PROVIDER_max if invalid*/

    LPPeMapAssociationType_e      eAssociation;
    LPPeMapAssociation_u  association;

    short horOrient; /*OPTIONAL, (0..359), set to INVALID_AGNSS_SHORT if invalid*/
}LPPeMapDataRefEle_t;

typedef struct
{
    char                 eleCount;  /*(1..16 = MAX_WLAN_MAP_DATA_REF_ELEMENT)*/
    LPPeMapDataRefEle_t *pElement;
}LPPeMapDataInfo_t;

/*
OMA-LPPe-HighAccuracy3Dposition ::= SEQUENCE {
    latitude INTEGER(-2147483648..2147483647),
    longitude INTEGER(-2147483648..2147483647),
    cep INTEGER(0..255) OPTIONAL, --Cond NoEllipse
    uncertainty-semimajor INTEGER(0..255) OPTIONAL, --Cond NoCEP
    uncertainty-semiminor INTEGER(0..255) OPTIONAL, --Cond NoCEP
    offset-angle INTEGER(0..179) OPTIONAL, --Cond NoCEP
    confidenceHorizontal INTEGER(0..99) OPTIONAL,
    altitude INTEGER(-64000..1280000),
    uncertainty-altitude INTEGER(0..255),
    confidenceVertical INTEGER(0..99) OPTIONAL,
    ...
}
*/
typedef struct
{
    int   latitude;     /*(-2147483648..2147483647)*/
    int   longitude;    /*(-2147483648..2147483647)*/
    short cep;          /*OPTIONAL, (0..255), set to INVALID_AGNSS_SHORT if invalid*/
    short uncSemiMajor; /*OPTIONAL, (0..255), set to INVALID_AGNSS_SHORT if invalid*/
    short uncSemiMinor; /*OPTIONAL, (0..255), set to INVALID_AGNSS_SHORT if invalid*/
    short offsetAngle;  /*OPTIONAL, (0..179), set to INVALID_AGNSS_SHORT if invalid*/
    short confHor;      /*OPTIONAL, (0..99),  set to INVALID_AGNSS_SHORT if invalid*/
    int   altitude;     /*(-64000..1280000)*/
    short uncAlt;       /*(0..255)*/
    short confVertical; /*OPTIONAL, (0..99),  set to INVALID_AGNSS_SHORT if invalid*/
}LPPeHa3DPos_t;

/*
OMA-LPPe-ReferencePoint ::= SEQUENCE {
    referencePointUniqueID OMA-LPPe-ReferencePointUniqueID OPTIONAL,
    referencePointGeographicLocation CHOICE {
        location3D EllipsoidPointWithAltitude,
        location3DwithUncertainty EllipsoidPointWithAltitudeAndUncertaintyEllipsoid,
        locationwithhighaccuracy OMA-LPPe-HighAccuracy3Dposition,
        ...
    } OPTIONAL,
    referencePointCivicLocation OMA-LPPe-CivicLocation OPTIONAL,
    referencePointFloorLevel INTEGER (-20..235) OPTIONAL,
    relatedReferencePoints SEQUENCE (SIZE (1..8)) OF
        OMA-LPPe-ReferencePointRelationship OPTIONAL,
    mapDataInformation OMA-LPPe-MapDataInformation OPTIONAL,
    ...
}
*/
typedef union
{
    EllipPntWithAlt_t            location3D;
    EllipPntWithAltAndUncEllip_t location3DWithUnc;
    LPPeHa3DPos_t                ha3DPos;
}LPPeRefPntGeoLoc_u;

typedef struct
{
    char                      refPntUniIDFlag;
    LPPeRefPointUniqueId_t    refPntUniID;      /*OPTIONAL*/

    LPPeRefPntGeoLocType_e    eRefPntGeoLoc;
    LPPeRefPntGeoLoc_u        refPntGeoLoc;     /*OPTIONAL*/

    char                      civicLocFlag;
    LPPeCivicLoc_t            civicLoc;         /*OPTIONAL*/
    
    short                     floorLvl; /*OPTIONAL, (-20..235), set to INVALID_AGNSS_SHORT if invalid*/

    char                      relshipCount;
    LPPeRefPntRelationship_t *pRelship;

    char                      mapDataInfoFlag;
    LPPeMapDataInfo_t         mapDataInfo;      /*OPTIONAL*/
}LPPeRefPoint_t;

/*
OMA-LPPe-WLANFemtoCoverageAreaElement ::= SEQUENCE {
    refPointAndArea SEQUENCE {
        referenceLocation ENUMERATED {antenna,
            referencePoint,
            ... },
        referencePoint OMA-LPPe-ReferencePointUniqueID OPTIONAL,
        locationAndArea OMA-LPPe-RelativeLocation,
        ...
        },
    type ENUMERATED {indoor (0),
        outdoor (1),
        mixed (2),
        ... } OPTIONAL,
    weight INTEGER (0..100) OPTIONAL,
    ...
}
*/
typedef struct
{
    LPPeRefLocationType_e  locType;

    char                   refPntUniIdFlag;
    LPPeRefPointUniqueId_t refPntUniId;     /*OPTIONAL*/

    LPPeRelLoc_t           relLoc;
}LPPeRefPntAndArea_t;

typedef struct
{
    short maxSegments; /*OPTIONAL, (2..4096), set to INVALID_AGNSS_SHORT if invalid*/
    short maxSize;     /*OPTIONAL, (1..5000), set to INVALID_AGNSS_SHORT if invalid*/
    short minSize;     /*OPTIONAL, (1..5000), set to INVALID_AGNSS_SHORT if invalid*/
    
    char  canResume;   /*OPTIONAL, TRUE if UE can support AD/LI transfer with resume*/
}LPPeSegDataCaps_t;

typedef struct
{
    LPPeVendorId_e       vendorType;
    LPPeVendorOrOperId_u vendor;

    char                 mapDataFormatFlag;
    LPPeOctetStr_t       mapDataFormat;
}LPPeRefPointCapEle_t;

typedef struct
{
    LPPeRefPointCapTypes_bitmap types;

    char                        suptEleCount;   /*(SIZE (1..128))*/
    LPPeRefPointCapEle_t       *pEle;
}LPPeRefPointCaps_t;

typedef struct
{
    /*TRUE if the target cannot determine the access types it supports*/
    char isAccessTypeUnknown;
    
    LPPeFixedAccessTypes_bitmap    fixed;    /*OPTIONAL, set to 0 if invalid*/
    LPPeWirelessAccessTypes_bitmap wireless; /*OPTIONAL, set to 0 if invalid*/
}LPPeAccessTypes_t;

/*
OMA-LPPe-LocalPosition ::= SEQUENCE {
    referencePoint OMA-LPPe-ReferencePointUniqueID,
    subjectLocation OMA-LPPe-RelativeLocation OPTIONAL,
    ...
}
*/
typedef struct
{
    LPPeRefPointUniqueId_t refPoint;

    char                   relLocFlag;
    LPPeRelLoc_t           relLoc;
}LPPeLocalPos_t;

/*
OMA-LPPe-HighAccuracy3Dvelocity ::= SEQUENCE {
    enu-origin OMA-LPPe-HighAccuracy3Dposition OPTIONAL,
    east-component INTEGER(0..511),
    negative-sign-east NULL OPTIONAL, --Cond West
    north-component INTEGER(0..511),
    negative-sign-north NULL OPTIONAL, --Cond South
    up-component INTEGER(0..511),
    negative-sign-up NULL OPTIONAL, --Cond Down
    cep INTEGER(0..255) OPTIONAL, --Cond NoEllipse
    uncertainty-semimajor INTEGER(0..255) OPTIONAL, --Cond NoCEP,
    uncertainty-semiminor INTEGER(0..255) OPTIONAL, --Cond NoCEP,
    offset-angle INTEGER(0..179) OPTIONAL, --Cond NoCEP,
    confidenceHorizontal INTEGER(0..99) OPTIONAL,
    uncertainty-up-component INTEGER(0..255),
    confidenceUp INTEGER(0..99) OPTIONAL,
    ...
}
*/
typedef struct
{
    char           ha3DPosFlag;
    LPPeHa3DPos_t  ha3DPos;    /*high Accuracy 3D position*/

    short eastWardSpeed;       /*(0..511), set to INVALID_AGNSS_SHORT if invalid*/
    char  isNegativeSignEast;  /*TRUE if the speed component is towards West.*/

    short northWardSpeed;      /*(0..511), set to INVALID_AGNSS_SHORT if invalid*/
    char  isNegativeSignNorth; /*TRUE if the speed component is towards South.*/

    short upWardSpeed;         /*(0..511), set to INVALID_AGNSS_SHORT if invalid*/
    char  isNegativeSignUp;    /*TRUE if the speed component is towards Down.*/

    short cep;          /*OPTIONAL, (0..255), set to INVALID_AGNSS_SHORT if invalid*/
    short uncSemiMajor; /*OPTIONAL, (0..255), set to INVALID_AGNSS_SHORT if invalid*/
    short uncSemiMinor; /*OPTIONAL, (0..255), set to INVALID_AGNSS_SHORT if invalid*/
    short offsetAngle;  /*OPTIONAL, (0..179), set to INVALID_AGNSS_SHORT if invalid*/
    short confidenceHor;/*OPTIONAL, (0..99),  set to INVALID_AGNSS_SHORT if invalid*/
    short uncertaintyUp;/*OPTIONAL, (0..255), set to INVALID_AGNSS_SHORT if invalid*/
    short confidenceUp; /*OPTIONAL, (0..99),  set to INVALID_AGNSS_SHORT if invalid*/
}LPPeHa3DVel_t;


typedef struct
{
    LPPeIpAddrType_e type;

    union {
        unsigned char iPv4[4];
        unsigned char iPv6[6];
    }addr;

    LPPeBearer_e bearer;

    char isNat;
}LPPeIpAddr_t;

typedef struct
{
    char         count; /*1..maxIPAddress = 5*/
    LPPeIpAddr_t ipAddrArray[MAX_WLAN_IP_ADDR];
}LPPeIpAddrList_t;

/*
OMA-LPPe-TimeStamp ::= CHOICE {
    gnssTime GNSS-SystemTime,
    networkTime NetworkTime,
    relativeTime INTEGER (0..1024),
    ...
}
*/
typedef struct
{
    LPPeTimeStamp_e present;

    union {
        //gnssTime and networkTime are not supported now.
        
        short relativeTime; /*range: 0..1024*/
    } choice;
}LPPeTimeStamp_t;

/*
OMA-LPPe-LocationSource ::= SEQUENCE {
    agnss NULL OPTIONAL,
    otdoa NULL OPTIONAL,
    eotd NULL OPTIONAL,
    otdoaUTRA NULL OPTIONAL,
    ecidLTE NULL OPTIONAL,
    ecidGSM NULL OPTIONAL,
    ecidUTRA NULL OPTIONAL,
    wlanAP NULL OPTIONAL,
    srn NULL OPTIONAL,
    sensors NULL OPTIONAL,
    ...
}
*/
typedef struct
{
    char isAgnssUsed;
    char isOtdoaUsed;
    char isEotdUsed;
    char isOtdoaUTRAUsed;
    char isEcidLTEUsed;
    char isEcidGSMUsed;
    char isEcidUTRAUsed;
    char isWlanAPUsed;
    char isSrnUsed;
    char isSensorsUsed;
}LPPeLocSource_t;

/*
OMA-LPPe-WLAN-AP-LocationServerErrorCauses ::= SEQUENCE {
    cause ENUMERATED {undefined,
        requestedADNotAvailable,
        notAllrequestedADAvailable,
        ...
    },
    apMandatoryDataUnavailable NULL OPTIONAL,
    apLocationsUnavailable NULL OPTIONAL,
    apLocationReliabilityUnavailable NULL OPTIONAL,
    apTransmitPowerUnavailable NULL OPTIONAL,
    apAntennaGainUnavailable NULL OPTIONAL,
    apCoverageAreaUnavailable NULL OPTIONAL,
    nonservingADUnavailable NULL OPTIONAL,
    ...,
    apTPNotAvailable NULL OPTIONAL,
    apAGNotAvailable NULL OPTIONAL,
    ueSNNotAvailable NULL OPTIONAL,
    ueRSSINotAvailable NULL OPTIONAL,
    ocNotAvailable NULL OPTIONAL
}
*/
typedef struct
{
    LPPeWlanApServerErrCause_e eCause;

    char isApMandatoryDataUnavailable;        /*OPTIONAL, TRUE if present*/
    char isApLocationsUnavailable;            /*OPTIONAL, TRUE if present*/
    char isApLocRelUnavailable;               /*OPTIONAL, TRUE if present*/
    char isApTransmitPowerUnavailable;        /*OPTIONAL, TRUE if present*/
    char isApAntennaGainUnavailable;          /*OPTIONAL, TRUE if present*/
    char isApCoverageAreaUnavailable;         /*OPTIONAL, TRUE if present*/
    char isNonservingADUnavailable;           /*OPTIONAL, TRUE if present*/
    
    char isApTPNotAvailable;                  /*OPTIONAL, TRUE if present*/
    char isApAGNotAvailable;                  /*OPTIONAL, TRUE if present*/
    char isUeSNNotAvailable;                  /*OPTIONAL, TRUE if present*/
    char isUeRSSINotAvailable;                /*OPTIONAL, TRUE if present*/
    char isOcNotAvailable;                    /*OPTIONAL, TRUE if present*/
}LPPeWlanApServerErr_t;

/*
OMA-LPPe-WLAN-AP-TargetDeviceErrorCauses ::= SEQUENCE {
    cause ENUMERATED {undefined,
        requestedMeasurementsNotAvailable,
        notAllrequestedMeasurementsPossible,
        ...
    },
    apSSIDnotAvailable NULL OPTIONAL,
    apSNMeasurementNotPossible NULL OPTIONAL,
    apDevTypeNotAvailable NULL OPTIONAL,
    apPhyTypeNotAvailable NULL OPTIONAL,
    apRSSIMeasurementNotPossible NULL OPTIONAL,
    apChanFreqNotAvailable NULL OPTIONAL,
    apRTDMeasurementNotPossible NULL OPTIONAL,
    ueTPNotAvailable NULL OPTIONAL,
    ueAGNotAvailable NULL OPTIONAL,
    apRecLocNotAvailable NULL OPTIONAL,
    non-servingMeasurementsNotAvailable NULL OPTIONAL,
    historicMeasurementsNotAvailable NULL OPTIONAL,
    ...,
    apTPNotAvailable NULL OPTIONAL,
    apAGNotAvailable NULL OPTIONAL,
    ueSNNotAvailable NULL OPTIONAL,
    ueRSSINotAvailable NULL OPTIONAL,
    ocNotAvailable NULL OPTIONAL
}
*/
typedef struct
{
    LPPeWlanApTargetErrCause_e eCause;

    char isApSSIDnotAvailable;                 /*OPTIONAL, TRUE if present*/
    char isApSNMeasurementNotPossible;         /*OPTIONAL, TRUE if present*/
    char isApDevTypeNotAvailable;              /*OPTIONAL, TRUE if present*/
    char isApPhyTypeNotAvailable;              /*OPTIONAL, TRUE if present*/
    char isApRSSIMeasurementNotPossible;       /*OPTIONAL, TRUE if present*/
    char isApChanFreqNotAvailable;             /*OPTIONAL, TRUE if present*/
    char isApRTDMeasurementNotPossible;        /*OPTIONAL, TRUE if present*/
    char isUeTPNotAvailable;                   /*OPTIONAL, TRUE if present*/
    char isUeAGNotAvailable;                   /*OPTIONAL, TRUE if present*/
    char isApRecLocNotAvailable;               /*OPTIONAL, TRUE if present*/
    char isNonServingMeasurementsNotAvailable; /*OPTIONAL, TRUE if present*/

    char isHistoricMeasurementsNotAvailable;   /*OPTIONAL, TRUE if present*/
    char isApTPNotAvailable;                   /*OPTIONAL, TRUE if present*/
    char isApAGNotAvailable;                   /*OPTIONAL, TRUE if present*/
    char isUeSNNotAvailable;                   /*OPTIONAL, TRUE if present*/
    char isUeRSSINotAvailable;                 /*OPTIONAL, TRUE if present*/
    char isOcNotAvailable;                     /*OPTIONAL, TRUE if present*/
    
}LPPeWlanApTargetErr_t;

/*
OMA-LPPe-WLAN-AP-Error ::= CHOICE { 
    locationServerErrorCauses OMA-LPPe-WLAN-AP-LocationServerErrorCauses,
    targetDeviceErrorCauses OMA-LPPe-WLAN-AP-TargetDeviceErrorCauses,
    ... 
}
*/
typedef union
{
    LPPeWlanApServerErr_t serverErr;
    LPPeWlanApTargetErr_t targetErr;
}LPPeWlanApErrChoice_u;

typedef struct
{
    LPPeWlanApErrType_e   errType;
    LPPeWlanApErrChoice_u err;
}LPPeWlanApErr_t;

/*
OMA-LPPe-WLAN-AP-Capability ::= SEQUENCE { 
    apMACAddress OMA-LPPe-WLAN-AP-ID,   --- AP的MAC地址
    apTypes OMA-LPPe-WLAN-AP-Type-List, --- AP的类型
    ... 
}
*/
typedef struct
{
    char                   macAddr[MAX_MAC_ADDR_LEN];
    LPPeWlanApTypes_bitmap apTypes;
}LPPeWlanApCap_t;

typedef struct
{
    LPPeWlanMsrDataTypes_bitmap     msrTypesSupport;
    LPPeWlanApTypes_bitmap          apTypesSupport; /*OPTIONAL, set to 0 if invalid*/

    char                            apCapFlag;      /*OPTIONAL, TRUE if apCap present*/
    LPPeWlanApCap_t                 apCap;          

    LPPeWlanApAssDataTypes_bitmap   assDataTypes;
    LPPeWlanAddlMsrDataTypes_bitmap addlMsrTypesSupport; /*OPTIONAL, set to 0 if invalid*/
}LPPeWlanProvCap_t;

/*
OMA-LPPe-WLAN-AP-RequestAssistanceData ::= SEQUENCE { 
    requestedAD BIT STRING { aplist (0), 
        aplocation (1), 
        locationreliability (2), 
        transmit-power (3), 
        antenna-gain (4), 
        coveragearea (5), 
        non-serving (6) } (SIZE(1..16)),
    requestedAPTypes OMA-LPPe-WLAN-AP-Type-List,
    ... 
}
*/
typedef struct
{
    LPPeWlanApAssDataTypes_bitmap assDataTypes;
    LPPeWlanApTypes_bitmap        apTypes;
}LPPeWlanReqAssData_t;

/*
OMA-LPPe-WLAN-AP-RequestLocationInformation ::= SEQUENCE { 
    requestedMeasurements BIT STRING { ---请求测量数据的种类。
        与5.1.2节的"wlan-ecid-MeasSupported"一样。
    apSSID (0), 
    apSN (1), 
    apDevType (2), 
    apPhyType (3), 
    apRSSI (4), 
    apChanFreq (5), 
    apRTD (6), 
    ueTP (7), 
    ueAG (8), 
    apRepLoc (9), 
    non-serving (10), 
    historic (11), 
    apTP (12), 
    apAG (13), 
    ueSN (14), 
    ueRSSI (15)} (SIZE(1..16)), 
    ... , 
    additionalRequestedMeasurements BIT STRING { ---请求额外测量数据的种类。
        与5.1.2节的"additional-wlan-ecid-MeasSupported" 一样。
    oc (0)} (SIZE(1..16)) OPTIONAL 
}
*/
typedef struct
{
    LPPeWlanMsrDataTypes_bitmap     reqMsr;
    LPPeWlanAddlMsrDataTypes_bitmap addlReqMsr; /*OPTIONAL, set to 0 if invalid*/
}LPPeWlanReqLocInfo_t;

typedef struct
{
    LPPeRefPntAndArea_t  refPointAndArea;
    LPPeWlanCovAreaEleType_e type; /*OPTIONAL, set to LPPeCovAreaEleType_max if invalid*/
    short                weight; /*OPTIONAL, (0..100), set to INVALID_AGNSS_SHORT if invalid*/
}LPPeWlanCovAreaEle_t;

/*
OMA-LPPe-WLANFemtoCoverageArea ::= SEQUENCE {
    truncation INTEGER(-127..128) OPTIONAL,
    areaType ENUMERATED { gaussian, binaryDistribution, ...} OPTIONAL,
    confidence INTEGER (0..99),
    componentList SEQUENCE (SIZE (1..16)) OF OMA-LPPe-WLANFemtoCoverageAreaElement,
    ...
}
*/
typedef struct
{
    
    short                 truncation; /*OPTIONAL, (-127..128), set to INVALID_AGNSS_SHORT if invalid*/
    LPPeWlanCovAreaType_e areaType; /*OPTIONAL, set to LPPeWlanCovAreaType_max if invalid*/
    short                 confidence;
    char                  componentCount; /*SIZE (1..16), MAX_WLAN_COVERAGE_AREA_ELEMENT*/
    LPPeWlanCovAreaEle_t *pComponents;
}LPPeWlanFemtoCovArea_t;

/* 
OMA-LPPe-WLAN-AP-Type-Data ::= SEQUENCE {
    wlan-AP-Type OMA-LPPe-WLAN-AP-Type,
    transmit-power INTEGER (-127..128) OPTIONAL,
    antenna-gain INTEGER (-127..128) OPTIONAL,
    coverageArea OMA-LPPe-WLANFemtoCoverageArea OPTIONAL, --Cond oneonly
    ...
}
*/
typedef struct
{
    LPPeWlanApType_e apType;
    short                  transPower;  /*OPTIONAL, (-127..128), set to INVALID_AGNSS_SHORT if invalid*/
    short                  antennaGain; /*OPTIONAL, (-127..128), set to INVALID_AGNSS_SHORT if invalid*/

    char                   covAreaFlag;
    LPPeWlanFemtoCovArea_t covArea;    /*OPTIONAL, --Cond oneonly*/
}LPPeWlanApTypeData_t;

/*
OMA-LPPe-WLAN-AP-Data ::= SEQUENCE {
    wlan-ap-id OMA-LPPe-WLAN-AP-ID,
    relative-location OMA-LPPe-RelativeLocation OPTIONAL,
    location-reliability INTEGER (1..100) OPTIONAL,
    wlan-ap-Type-Data SEQUENCE (SIZE (1..maxWLANTypes)) OF OMA-LPPe-WLAN-AP-Type-Data,
    coverageArea OMA-LPPe-WLANFemtoCoverageArea OPTIONAL, --Cond oneonly
    ...
}
maxWLANTypes INTEGER ::= 5
*/
typedef struct
{
    char         macAddr[MAX_MAC_ADDR_LEN];

    char         relLocFlag;
    LPPeRelLoc_t relLoc;         /*OPTIONAL*/

    short        locReliability; /*OPTIONAL, (1..100), set to INVALID_AGNSS_SHORT if invalid*/

    int                    apTypeCount; /*SIZE (1..5), MAX_WLAN_TYPES*/
    LPPeWlanApTypeData_t  *pApTypeData; /*OPTIONAL*/

    char                   covAreaFlag;
    LPPeWlanFemtoCovArea_t covArea;
}LPPeWlanApData_t;

/*
OMA-LPPe-WLAN-DataSet ::= SEQUENCE {
    plmn-Identity SEQUENCE {
        mcc SEQUENCE (SIZE (3)) OF INTEGER (0..9),
        mnc SEQUENCE (SIZE (2..3)) OF INTEGER (0..9)
    } OPTIONAL,
    reference-point OMA-LPPe-ReferencePoint OPTIONAL, --Cond APlocations
    supported-channels-11a Supported-Channels-11a OPTIONAL,
    supported-channels-11bg Supported-Channels-11bg OPTIONAL,
    wlan-ap-list SEQUENCE (SIZE (1..maxWLANAPs)) OF OMA-LPPe-WLAN-AP-Data,
    ...
}
maxWLANAPs INTEGER ::= 128
*/
typedef struct
{
    char                        plmnIdFlag;
    LPPePlmnId_t                plmnId;      /*OPTIONAL*/

    char                        refPntFlag;
    LPPeRefPoint_t              refPnt;     /*OPTIONAL, --Cond APlocations*/

    LPPeWlanChans80211a_bitmap  chans11a;   /*OPTIONAL, set to 0 if invalid*/
    LPPeWlanChans80211bg_bitmap chans11bg;  /*OPTIONAL, set to 0 if invalid*/

    int                         apDataCount;/*OPTIONAL, (1..128), MAX_WLAN_AP_DATA*/
    LPPeWlanApData_t           *pApData;
}LPPeWlanDataSet_t;

/*
OMA-LPPe-WLAN-AP-ProvideAssistanceData ::= SEQUENCE { 
    wlan-DataSet SEQUENCE (SIZE (1..maxWLANDataSets)) OF OMA-LPPe-WLAN-DataSet OPTIONAL, 
    wlan-AP-Error OMA-LPPe-WLAN-AP-Error OPTIONAL, 
    ... 
}
maxWLANDataSets INTEGER ::= 8
*/
typedef struct
{
    int                dataSetCount;  /*SIZE (1..8), MAX_WLAN_DATA_SETS*/
    LPPeWlanDataSet_t *pDataSets;
}LPPeWlanDataSetList_t;

typedef struct
{
    char                  dataSetListFlag;
    LPPeWlanDataSetList_t dataSetList;

    char                  serverErrFlag;
    LPPeWlanApServerErr_t serverErr;
}LPPeWlanProvAssData_t;

/*
OMA-LPPe-WLAN-AP-LocationInformation ::= SEQUENCE {
    apMACAddress OMA-LPPe-WLAN-AP-ID,
    apSSID OCTET STRING (SIZE (1..32)) OPTIONAL,
    apSignaltoNoise INTEGER(-127..128) OPTIONAL,
    apDeviceType OMA-LPPe-WLAN-AP-Type OPTIONAL,
    apPHYtype OMA-LPPe-WLAN-AP-PHY-Type OPTIONAL,
    apSignalStrength INTEGER(-127..128) OPTIONAL,
    apChannelFrequency INTEGER(0..256) OPTIONAL,
    apRoundTripDelay OMA-LPPe-WLAN-RTD OPTIONAL,
    ueTransmitPower INTEGER(-127..128) OPTIONAL,
    ueAntennaGain INTEGER (-127..128) OPTIONAL,
    apReportedLocation OMA-LPPe-WLAN-ReportedLocation OPTIONAL,
    ...,
    apTransmitPower INTEGER (-127..128) OPTIONAL,
    apAntennaGain INTEGER (-127..128) OPTIONAL,
    ueSignaltoNoise INTEGER (-127..128) OPTIONAL,
    ueSignalStrength INTEGER (-127..128) OPTIONAL,
    apSignalStrengthDelta INTEGER (0..1) OPTIONAL, -- Cond APSSDelta
    ueSignalStrengthDelta INTEGER (0..1) OPTIONAL, -- Cond UESSDelta
    apSignaltoNoiseDelta INTEGER (0..1) OPTIONAL, -- Cond APSNDelta
    ueSignaltoNoiseDelta INTEGER (0..1) OPTIONAL, -- Cond UESNDelta
    operatingClass INTEGER (0..255) OPTIONAL
}
OMA-LPPe-WLAN-AP-PHY-Type ::= ENUMERATED { unknown, any, fhss, dsss, irbaseband, ofdm, hrdsss, erp,
    ht, ihv, ... }
OMA-LPPe-WLAN-RTD ::= SEQUENCE {
    rTDValue INTEGER(0..16777215),
    rTDUnits OMA-LPPe-WLAN-RTDUnits,
    rTDAccuracy INTEGER(0..255) OPTIONAL,
    ...
}
OMA-LPPe-WLAN-RTDUnits ::= ENUMERATED {
    microseconds,
    hundredsofnanoseconds,
    tensofnanoseconds,
    nanoseconds,
    tenthsofnanoseconds,
    ...
}
OMA-LPPe-WLAN-ReportedLocation ::= SEQUENCE {
    locationDataLCI OMA-LPPe-WLAN-LocationDataLCI OPTIONAL,
    ...
}
OMA-LPPe-WLAN-LocationDataLCI ::= SEQUENCE {
    latitudeResolution BIT STRING (SIZE (6)),
    latitude BIT STRING (SIZE (34)),
    longitudeResolution BIT STRING (SIZE (6)),
    longitude BIT STRING (SIZE (34)),
    altitudeType BIT STRING (SIZE (4)),
    altitudeResolution BIT STRING (SIZE (6)),
    altitude BIT STRING (SIZE (30)),
    datum BIT STRING (SIZE (8)),
    ...
}
*/

/*
OMA-LPPe-WLAN-AP-ProvideLocationInformation ::= SEQUENCE { 
    wlan-AP-CombinedLocationInformation SEQUENCE (SIZE (1..maxWLANAPSize)) OF 
        OMA-LPPe-WLAN-AP-LocationInformationList OPTIONAL,
    wlan-AP-Error OMA-LPPe-WLAN-AP-Error OPTIONAL,
    ... 
}
OMA-LPPe-WLAN-AP-LocationInformationList ::= SEQUENCE { 
    wlan-AP-LocationInformation OMA-LPPe-WLAN-AP-LocationInformation,
    relativeTimeStamp INTEGER (0..65535) OPTIONAL,
    servingFlag BOOLEAN,
    ... 
}
maxWLANAPSize INTEGER ::= 64
*/
/**
*  In case of wlan location system, CG_getLocationId should provide following information
*/

/*
OMA-LPPe-WLAN-LocationDataLCI ::= SEQUENCE {
    latitudeResolution BIT STRING (SIZE (6)),
    latitude BIT STRING (SIZE (34)),
    longitudeResolution BIT STRING (SIZE (6)),
    longitude BIT STRING (SIZE (34)),
    altitudeType BIT STRING (SIZE (4)),
    altitudeResolution BIT STRING (SIZE (6)),
    altitude BIT STRING (SIZE (30)),
    datum BIT STRING (SIZE (8)),
    ...
}
*/
typedef struct
{
    double                   latitude;
    double                   longitude;
    LPPeWlanAltitudeType_e   altType;
    double                   altitude;
    LPPeWlanDataCoordinate_e datam;
}LPPeWlanApRptLoc_t;

typedef struct
{
    long                 rTDValue;    /* range: (0..16777216) */
    AgnssWlanRTDUnits_e  rTDUnits;    /* units of rTDValue */
    long                 rTDAccuracy; /*optional, range: (0,255), set to INVALID_AGNSS_SHORT if invalid*/
}AgnssWlanRTD_t;

typedef struct 
{ 
    char                apMACAddress[AGNSS_MAC_ADDR_LEN];
    long                flag;            /*field present flag, ref to agnssFapMACAddress ...*/
    short               apTransmitPower; /*optional, range:(-127, 128), set to INVALID_AGNSS_SHORT if invalid*/
    short               apAntennaGain;   /*optional, range:(-127, 128), set to INVALID_AGNSS_SHORT if invalid*/
    short               apSignaltoNoise; /*optional, range:(-127, 128), set to INVALID_AGNSS_SHORT if invalid*/

    AgnssWlanType_PR    apDeviceType;    /*optional, The AP device type */

    short               apSignalStrength;   /*optional, range:(-127, 128), set to INVALID_AGNSS_SHORT if invalid*/
    short               apChannelFrequency; /*optional, range:(0, 256), set to INVALID_AGNSS_SHORT if invalid*/

    char                apRoundTripDelayPresent; /*TRUE if apRoundTripDelay present*/
    AgnssWlanRTD_t      apRoundTripDelay;

    short               setTransmitPower;  /*optional, range:(-127, 128), set to INVALID_AGNSS_SHORT if invalid*/
    short               setAntennaGain;    /*optional, range:(-127, 128), set to INVALID_AGNSS_SHORT if invalid*/
    short               setSignaltoNoise;  /*optional, range:(-127, 128), set to INVALID_AGNSS_SHORT if invalid*/
    short               setSignalStrength; /*optional, range:(-127, 128), set to INVALID_AGNSS_SHORT if invalid*/

    char                apReportedLocationPresent; /*TRUE if apReportedLocation present*/
    LPPeWlanApRptLoc_t  apReportedLocation;
    
    char                apSSID[32];         /*terminated by '\0', if strlen(apSSID) == 0, then apSSID not present*/

    LPPeWlanApPhyType_e apPhyType;          /*optional, set to LPPeWlanApPhyType_max if not present*/

    short               apSigStrengthDelta; /*optional, range:(0,1), set to INVALID_AGNSS_SHORT if invalid*/
    short               ueSigStrengthDelta; /*optional, range:(0,1), set to INVALID_AGNSS_SHORT if invalid*/
    short               apSNRDelta;         /*optional, range:(0,1), set to INVALID_AGNSS_SHORT if invalid*/
    short               ueSNRDelta;         /*optional, range:(0,1), set to INVALID_AGNSS_SHORT if invalid*/
    short               operatingClass;     /*optional, range:(0,256), set to INVALID_AGNSS_SHORT if invalid*/

    short               relativeTimeStamp;  /*OPTIONAL, set to INVALID_AGNSS_SHORT if invalid*/
} AgnssWlanCellInformation_t;

typedef struct
{
    AgnssWlanCellInformation_t  nbCellInfo[AGNSS_MAX_LID_SIZE];
    int                         nbCellCount;

    AgnssWlanCellInformation_t  curCellInfo;
}AgnssWlanCellList_t; 

typedef struct
{
    /*wlanInfoListFlag and targetErrFlag can not be both present at the same time*/
    char                  wlanInfoListFlag;
    AgnssWlanCellList_t   wlanInfoList;

    char                  targetErrFlag;
    LPPeWlanApTargetErr_t targetErr;
}LPPeWlanApProvLocInfo_t;

typedef struct
{
    LPPeIpAddrTypes_bitmap ipAddrTypes; /*OPTIONAL, bitmap, set to 0 if invalid*/

    LPPeHATypes_bitmap     haFormat;    /*OPTIONAL, bitmap, set to 0 if invalid*/

    /*TRUE if support segmented assistance data transfer*/
    char                   segAssDataFlag;
    LPPeSegDataCaps_t      segAssData;

    char                   refPntCapFlag;
    LPPeRefPointCaps_t     refPntCap;

    char                   accessCapFlag;
    LPPeAccessTypes_t      accessCap;

    /*TRUE if support segmented location information transfer*/
    char                   segLocInfoFlag;
    LPPeSegDataCaps_t      segLocInfo;
}LPPeComProvCaps_t;

typedef struct
{
    char              comCapFlag;
    LPPeComProvCaps_t comCap;

    char              wlanCapFlag;
    LPPeWlanProvCap_t wlanCap;
}LPPeProvCaps_t;

typedef struct
{
    char                         approLocFlag; /* OPTIONAL, approximate location */
    EllipPntWithAltAndUncEllip_t approLoc;

    LPPePeriodReqType_e          periodReqType; /* OPTIONAL, set to LPPePeriodReqType_max if invalid */

    char                         refPointEleCount; /*(SIZE (1..128)), set to 0 if invalid*/
    LPPeRefPointCapEle_t        *pRefPointEle;
}LPPeComReqAssData_t;

typedef struct
{
    char                 comReqFlag;
    LPPeComReqAssData_t  comReq;

    char                 wlanReqFlag;
    LPPeWlanReqAssData_t wlanReq;
}LPPeReqAssData_t;

typedef struct
{
    char           refPointFlag;
    LPPeRefPoint_t refPoint;
}LPPeComProvAssData_t;

typedef struct
{
    char                  comProvFlag;
    LPPeComProvAssData_t  comProv;

    char                  wlanProvFlag;
    LPPeWlanProvAssData_t wlanProv;
}LPPeProvAssData_t;

typedef struct
{
    char              ha3DPosFlag;
    LPPeHa3DPos_t     ha3DPos;       /*high Accuracy 3D position*/

    char              localPosFlag;
    LPPeLocalPos_t    localPos;

    char              ha3DVelFlag;
    LPPeHa3DVel_t     ha3DVel;

    char              ipAddrListFlag;
    LPPeIpAddrList_t  ipAddrList;

    char              accessTypesFlag;
    LPPeAccessTypes_t accessTypes;

    char              timeStampFlag;
    LPPeTimeStamp_t   timeStamp;

    char              locSourceFlag;
    LPPeLocSource_t   locSource;
}LPPeComProvLocInfo_t;

typedef struct
{
    char                    comFlag;
    LPPeComProvLocInfo_t    com;

    char                    wlanFlag;
    LPPeWlanApProvLocInfo_t wlan;
}LPPeProvLocInfo_t;

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

typedef struct _setOtdoaCap_t
{
    unsigned char    isSupMsa;         
    unsigned char    supBand[8];  // LCS_BAND_MAX_BYTE_NUM
    unsigned char    isSupBandV9a0;          
    unsigned char    supBandV9a0[24];    //    LCS_BAND_V9A0_MAX_BYTE_NUM  
    unsigned char    isSupInterFreqRSTDMeas;  
    unsigned char    isSupAdditionalNbCell;  
}setOtdoaCap_t;

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
    unsigned long    second;          /*<  seconds (from beginning of current week no) */
    unsigned long    secondFrac;      /*<  seconds fraction (from beginning of current second) units: 1ns */
    unsigned long    sysTime;        // System clock time at the moment of message reception
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

typedef struct{
    SUPL_GNSS_ID_T   gnssID ; //
    SUPL_SBAS_ID_T   sbasID;    
    GNSS_POS_MODE_T  gnssPosMode; //bitmap,bit0:SET assit, bit 1:SET based, bit2 auto
    GNSS_SIGNAL_T    gnssSignal;  /*bitmap for supporting signals */
    GNSS_ASSIST_T    gnssAssistData;  /*bitmap for support assist data type*/
}GNSSPosMethod_t;

typedef  struct {
    unsigned char    gnssNum;     //supported GNSS number，excluding GPS
    GNSSPosMethod_t  posMethod[MAX_GNSS_NUM];//
}GNSSPosMethodList_t;

typedef struct {
    GNSSPosMethodList_t posMethodList;
    CgSetCapability_t   capability;
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
    float  gnssTODFrac;//opt ,ns
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

#define MAX_DGNSS_SIGNAL_NUM 3
typedef enum{
    GNSSStatusHealth_UdreScale_1 = 0, // 1
    GNSSStatusHealth_UdreScale_075,   // 0.75
    GNSSStatusHealth_UdreScale_05,    // 0.5
    GNSSStatusHealth_UdreScale_03,    // 0.3
    GNSSStatusHealth_UdreScale_02,    // 0.2
    GNSSStatusHealth_UdreScale_01,    // 0.1
    GNSSStatusHealth_NotMonitored,    //
    GNSSStatusHealth_DataIsInvalid    // 
}GNSSStatusHealth_e;

typedef enum{
    UDREGrowthRate_1_5 = 0, // 1.5
    UDREGrowthRate_2,       // 2
    UDREGrowthRate_4,       // 4
    UDREGrowthRate_6,       // 6
    UDREGrowthRate_8,       // 8
    UDREGrowthRate_10,      // 10
    UDREGrowthRate_12,      // 12
    UDREGrowthRate_16,      // 16
    UDREGrowthRate_noData
}UDREGrowthRate_e;

typedef enum{
    UDREValidityTime_20s = 0, // 20 second
    UDREValidityTime_40s,     // 40
    UDREValidityTime_80s,     // 80
    UDREValidityTime_160s,
    UDREValidityTime_320s,
    UDREValidityTime_640s,
    UDREValidityTime_1280s,
    UDREValidityTime_2560s,
    UDREValidityTime_noData
}UDREValidityTime_e;

typedef struct{
    unsigned char commonIodFlag; //commonIod is present
    unsigned char iodeFlag; // iode is present
    GNSSIOD_t     commonIod;
    int           iode;
}DGNSSCorrIOD_t;

typedef struct{
    int                svId;
    DGNSSCorrIOD_t     iod;
    int                udre;
    int                pseudoRangeCor; // PRC, (-2047..2047), scale factor: 0.32 meters
    int                rangeRateCor;   // RRC, (-127..127), Scale factor: 0.032 meters/second
    UDREGrowthRate_e   growthRate;     //
    UDREValidityTime_e validityTime;   //
}DGNSSCorrSat_t;

typedef struct{
    GNSS_SIGNAL_T      signalId;
    GNSSStatusHealth_e statusHealth; // UDRE Scale Factor
    int                satNum;
    DGNSSCorrSat_t     corrSat[64];              
}DGNSSCorrSignal_t;

typedef struct{
    long              refTime; // 0-3599s  module 1 hour
    int               signalNum; // 1-3
    DGNSSCorrSignal_t corrSignal[MAX_DGNSS_SIGNAL_NUM];
}GNSSDiffCorrections_t;

/*this time，according to GPS,only deliver ephemeris and reference Measurement information*/
typedef struct{
    SUPL_GNSS_ID_T gnssID;
    SUPL_SBAS_ID_T sbasID;
    GNSSNavigationModel_t  *navModel;
    GNSSRefMsrAssist_t   *refMsrAssit; 
    GNSSAuxiliaryInfo_t  *auxiliaryInfo;
    GNSSDiffCorrections_t *diffCorr;
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
    double uncertainty; // optional ,unit: us(microseconds)
}RefTimeTOD_t;

typedef struct{
    GNSSTimeID_e timeID;
    long weekNo;
    long timeofweek; // second
    long towFrac; // ns
    double uncertainty; // optional , unit: us(microseconds)
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

typedef enum
{
    LCS_NWLR_STOP  = 0,
    LCS_NWLR_START        
}LcsNwlrCtrlCmd_e;

typedef enum
{
    LCS_NWLR_ECID    = 1,
    LCS_NWLR_OTDOA,

    LCS_NWLR_UNKNOWN = 0xFF   
}LcsNwlrMethod_e;

#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif
