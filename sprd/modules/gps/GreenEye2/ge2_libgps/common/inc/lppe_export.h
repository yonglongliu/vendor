/******************************************************************************
** File Name:    lppe_export.h                                                *
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
** 2015-07-28            Andy.Huang                Create.                    *
******************************************************************************/
#ifndef __LPPE_EXPORT_H__
#define __LPPE_EXPORT_H__

/******************************************************
*****            LPPe Macro Definitions           *****
******************************************************/
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

#define AGNSS_MAX_LID_SIZE              (32)  //spec: 64
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

    
/******************************************************
*****         LPPe Enumeration Definitions        *****
******************************************************/

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
    LPPeWlanApType_802_11a = 0,
    LPPeWlanApType_802_11b,
    LPPeWlanApType_802_11g,
    LPPeWlanApType_802_11n,
    LPPeWlanApType_802_11ac,
    LPPeWlanApType_802_11ad,
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

/******************************************************
*****           LPPe Bitmap Definitions           *****
******************************************************/
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


/******************************************************
*****           LPPe Struct Definitions           *****
******************************************************/

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

/******************************************************
*****             LPPe API Definitions            *****
******************************************************/

#endif
