/**************************************************************************************************
File: 	    GNSS_libgps_api.h
Purpose: 	declaration of the communication interface between the libgps and GNSS device 
Changes:
		
##Copyright(c) 2010-2020 Spreadtrum Ltd. All rights reserved.
****************************************************************************************************/

#ifndef GNSS_LIBGPS_API_H
#define GNSS_LIBGPS_API_H


//#ifndef GPS_CH_NUM
//#define GPS_CH_NUM (14)
//#endif

//#ifndef BD2_CH_NUM
//#define BD2_CH_NUM (14)
//#endif

#ifndef NULL
#define NULL 0//((void *)0)
#endif

//#ifndef GPS_ALM_COPY_MAX
//#define GPS_ALM_COPY_MAX   5
//#endif
//#ifndef BD2_ALM_COPY_MAX
//#define BD2_ALM_COPY_MAX   3
//#endif

/******************************************************
 The Data format between the GPS Lib and GNSS device 
-------------------------------------------------------------|
|Magic Number| Type   |Subtype|Lenth   | CRC   |  DATA...... |
|~~~~        | 1 Byte |1 Byte | 2 Byte | 2 Byte|   n bytes   |
-------------------------------------------------------------
*******************************************************/

//define the type  data communication 
#define GNSS_LIBGPS_SET_PARA_TYPE                     (0x01)
#define GNSS_LIBGPS_REQUEST_TYPE                      (0x02) //aiding data 
#define GNSS_LIBGPS_RESPONSE_TYPE                     (0x03) //aiding data 
#define GNSS_LIBGPS_NOTIFIED_TYPE                     (0x04) //active report 
#define GNSS_LIBGPS_DATA_TYPE                         (0x05) //nmea,log,dump
#define GNSS_LIBGPS_AIDING_DATA_TYPE                  (0x06) //aiding data 
//add the  power and work mode for special deal with  begin 
#define GNSS_LIBGPS_POWER_REQUEST_TYPE                (0x07)
#define GNSS_LIBGPS_POWER_RESPONSE_TYPE               (0x08) 
//add the  power and work mode for special deal with   end 
#define GNSS_LIBGPS_FLASH_TYPE                        (0x09)
#define GNSS_LIBGPS_EUT_TYPE                          (0x0a)
#define GNSS_LIBGPS_REG_WR_TYPE                       (0x0b)      // GNSS_REG_WR
#define GNSS_LIBGPS_NMEA_TYPE                         (0x0c)
#define GNSS_LIBGPS_MAX_TYPE                          (0x0d)            // GNSS_REG_WR

// GNSS_LIBGPS_SET_PARA_TYPE  begin 
#define  GNSS_LIBGPS_SET_GNSS_SUBTYPE                 (0x01)
#define  GNSS_LIBGPS_SET_DGPS_SUBTYPE                 (0x02)
#define  GNSS_LIBGPS_SET_LTE_SUBTYPE                  (0x03)
#define  GNSS_LIBGPS_SET_REPORT_PARA_SUBTYPE          (0x04) //set report data type and period 
#define  GNSS_LIBGPS_SET_GPSTIME_SUBTYPE              (0x05)  
#define  GNSS_LIBGPS_SET_LOGLEVEL_SUBTYPE             (0x06) 
#define  GNSS_LIBGPS_SET_WORKMODE_SUBTYPE             (0X07)
#define  GNSS_LIBGPS_RESETCHIP_SUBTYPE                (0x08)
#define  GNSS_LIBGPS_RESETEVB_SUBTYPE                 (0x09)
#define  GNSS_LIBGPS_SETMODE_SUBTYPE                  (0x0a)
#define  GNSS_LIBGPS_CLEAR_FLASH_CONFIG_SUBTYPE       (0x0b)
#define  GNSS_LIBGPS_SET_OUTPUT_PROTOCOL_SUBTYPE      (0x0c)
#define  GNSS_LIBGPS_SET_REPORT_INTERVAL              (0x0d)    // CTS_TEST
#define  GNSS_LIBGPS_SET_LTE_SWITCH                   (0x20)
#define  GNSS_LIBGPS_SET_CMCC_SWITCH                  (0x21)
#define  GNSS_LIBGPS_SET_NETWORK_STATUS               (0x22)
#define  GNSS_LIBGPS_SET_REALEPH_STATUS               (0x23)
#define  GNSS_LIBGPS_SET_WIFI_STATUS                  (0x24)
#define  GNSS_LIBGPS_SET_ASSERT_STATUS                (0x25)
#define  GNSS_LIBGPS_SET_DELETE_AIDING_DATA_BIT       (0x26)
//#define  GNSS_LIBGPS_SET_WORKMODE_SUBTYPE             (0x07) 
//GNSS_LIBGPS_SET_PARA_TYPE end

// GNSS_LIBGPS_REQUEST_TYPE begin 
#define GNSS_LIBGPS_REQUEST_VERSION_SUBTYPE           (0x01) 
#define GNSS_LIBGPS_REQUEST_CHIP_SUBTYPE              (0x02) 
#define GNSS_LIBGPS_REQUEST_GPSTIME_SUBTYPE           (0x03) 
#define GNSS_LIBGPS_REQUEST_CONSTELLATION_SUBTYPE     (0x04) 
#define GNSS_LIBGPS_REQUEST_MEASUREMENT_SUBTYPE       (0x05)
#define GNSS_LIBGPS_REQUEST_TSXTEMP_SUBTYPE           (0x06)
//#define GNSS_LIBGPS_REQUEST_POWER_MANAGER_SUBTYPE     (0x04)// Power manager 
// GNSS_LIBGPS_REQUEST_TYPE  end 
//TODO ADD 
//GNSS_LIBGPS_RESPONSE_TYPE begin 
#define GNSS_LIBGPS_RESPONSE_VERSION_TYPE             (0x01) 
#define GNSS_LIBGPS_RESPONSE_CHIP_SUBTYPE             (0x02)  
#define GNSS_LIBGPS_RESPONSE_GPSTIME_SUBTYPE          (0x03) 
#define GNSS_LIBGPS_RESPONSE_CONSTELLATION_SUBTYPE    (0x04)
//#define GNSS_LIBGPS_RESPONSE_POWER_RESPONSE_SUBTYPE   (0x04) 
// GNSS_LIBGPS_RESPONSE_TYPE end 

//define GNSS_LIBGPS_DATA_TYPE begin 
#define GNSS_LIBGPS_NMEA_SUBTYPE                      (0x01)
#define GNSS_LIBGPS_ASSERT_SUBTYPE                    (0x02)
#define GNSS_LIBGPS_LOG_SUBTYPE             	      (0x03)
#define GNSS_LIBGPS_DGPS_SUBTYPE             	      (0x04)
#define GNSS_LIBGPS_LTE_SUBTYPE             	      (0x05)
#define GNSS_LIBGPS_TEST_SUBTYPE             	      (0x06)
#define GNSS_LIBGPS_MEASUREMENT_SUBTYPE               (0x07)// TODO 
#define GNSS_LIBGPS_MSA_MEASUREMENT_SUBTYPE           (0x08) 
#define GNSS_LIBGPS_LOG_DDDD_SUBTYPE                  (0x09)// 4 double  datas 
#define GNSS_LIBGPS_LOG_LLLL_SUBTYPE                  (0x0A)// 4 long   datas 
#define GNSS_LIBGPS_LOG_SSSS_SUBTYPE                  (0x0B)// 4 short  datas 
#define GNSS_LIBGPS_LOG_FFFF_SUBTYPE                  (0x0C)// 4 float  datas 
#define GNSS_LIBGPS_MEMDUMP_DATA_SUBTYPE              (0x0D)
#define GNSS_LIBGPS_MEMDUMP_CODE_SUBTYPE              (0x0E)
#define GNSS_LIBGPS_LOG_BB_SUBTYPE                    (0x0F)//baseband log
#define GNSS_LIBGPS_SET_AP_TSX_TEMP                   (0x10)
#define GNSS_LIBGPS_SET_CP_TSX_TEMP                   (0x11)
#define GNSS_LIBGPS_SAVE_TSX_TEMP                     (0x12)
#define GNSS_LIBGPS_CALI_TSX_TEMP                     (0x13)    // TSXTEMP
#define GNSS_LIBGPS_MINILOG_SUBTYPE                   (0x15)
#define GNSS_LIBGPS_MEMDUMP_PCHANNEL_SUBTYPE       	  (0x16)
#define GNSS_LIBGPS_ELLIPES_SUBTYPE       	          (0x17)
#define GNSS_LIBGPS_OBS_SUBTYPE             	      (0x25)
#define GNSS_LIBGPS_NAVIGATION_SUBTYPE             	  (0x26)
#define GNSS_LIBGPS_GPSCLOCK_SUBTYPE             	  (0x27)
#define GNSS_LIBGPS_SOFTBIT_CLEAR_SUBTYPE             (0x50)
#define GNSS_LIBGPS_SOFTBIT_SEND_SUBTYPE              (0x51)
//define GNSS_LIBGPS_DATA_TYPE end

//define GNSS_LIBGPS_FLASH_TYPE  begin
/*Addr bias for writing to flash*/
#define GNSS_LIBGPS_FLASH_PvtMisc_SUBTYPE            (0)
#define GNSS_LIBGPS_FLASH_GPS_EPH1_SUBTYPE           (1)
#define GNSS_LIBGPS_FLASH_GPS_EPH2_SUBTYPE           (2)
#define GNSS_LIBGPS_FLASH_GPS_EPH3_SUBTYPE           (3)
#define GNSS_LIBGPS_FLASH_GPS_EPH4_SUBTYPE           (4)
#define GNSS_LIBGPS_FLASH_GPS_ALM_SUBTYPE            (5)
#define GNSS_LIBGPS_FLASH_GPS_ALM_CPY1_SUBTYPE       (6)
#define GNSS_LIBGPS_FLASH_GPS_ALM_CPY2_SUBTYPE       (7)
#define GNSS_LIBGPS_FLASH_GPS_ALM_CPY3_SUBTYPE       (8)
#define GNSS_LIBGPS_FLASH_GPS_ALM_CPY4_SUBTYPE       (9)
#define GNSS_LIBGPS_FLASH_GPS_ALM_CPY5_SUBTYPE       (10)
#define GNSS_LIBGPS_FLASH_GPS_ALM_CPY6_SUBTYPE       (11)
#define GNSS_LIBGPS_FLASH_QZS_EPH_SUBTYPE            (12)
#define GNSS_LIBGPS_FLASH_QZS_ALM_SUBTYPE            (13)
#define GNSS_LIBGPS_FLASH_BD2_EPH1_SUBTYPE           (14)
#define GNSS_LIBGPS_FLASH_BD2_EPH2_SUBTYPE           (15)
#define GNSS_LIBGPS_FLASH_BD2_ALM_SUBTYPE            (16)
#define GNSS_LIBGPS_FLASH_BD2_ALM_CPY1_SUBTYPE       (17)
#define GNSS_LIBGPS_FLASH_BD2_ALM_CPY2_SUBTYPE       (18)
#define GNSS_LIBGPS_FLASH_BD2_ALM_CPY3_SUBTYPE       (19)
#define GNSS_LIBGPS_FLASH_BD2_ALM_CPY4_SUBTYPE       (20)
#define GNSS_LIBGPS_FLASH_GLO_EPH1_SUBTYPE           (21)
#define GNSS_LIBGPS_FLASH_GLO_EPH2_SUBTYPE           (22)
#define GNSS_LIBGPS_FLASH_GLO_EPH3_SUBTYPE           (23)
#define GNSS_LIBGPS_FLASH_GLO_EPH4_SUBTYPE           (24)
#define GNSS_LIBGPS_FLASH_GLO_ALM1_SUBTYPE           (25)
#define GNSS_LIBGPS_FLASH_GLO_ALM2_SUBTYPE           (26)
#define GNSS_LIBGPS_FLASH_IONO_UTC_SUBTYPE           (27)
#define GNSS_LIBGPS_FLASH_SYS_CFG_SUBTYPE            (28)
#define GNSS_LIBGPS_FLASH_ALMCPY_COMM_SUBTYPE        (29)
#define GNSS_LIBGPS_FLASH_PVT_SEQLSQ_SUBTYPE         (30)
#define GNSS_LIBGPS_FLASH_PVT_BLUNDER_SUBTYPE        (31)
#define GNSS_LIBGPS_FLASH_MAX_LEN_SUBTYPE            (32)
//define GNSS_LIBGPS_FLASH_TYPE  end


//define GNSS_LIBGPS_AIDING_DATA_TYPE  begin  
#define GNSS_LIBGPS_AIDING_REFTIME_SUBTYPE            (0x01)//mino:it will use POSVELTIME
#define GNSS_LIBGPS_AIDING_REFPOSITION_SUBTYPE        (0x02)//mino:it will use POSVELTIME
#define GNSS_LIBGPS_AIDING_REFEPHEMERIS_SUBTYPE       (0x03)//mino:it will use GPSSEPHEMERIS
#define GNSS_LIBGPS_AIDING_ASSISTANCE_SUBTYPE         (0x04)//
#define GNSS_LIBGPS_AIDING_ALMANAC_SUBTYPE            (0x05)//mino:it will use GPSALMANAC
#define GNSS_LIBGPS_AIDING_DGPS_SUBTYPE               (0x06) 
#define GNSS_LIBGPS_AIDING_IONOSPHERE_SUBTYPE         (0x07) 
#define GNSS_LIBGPS_AIDING_UTC_SUBTYPE                (0x08) 
#define GNSS_LIBGPS_AIDING_REALTIME_INTEGRITY_SUBTYPE (0x09) 

#define GNSS_LIBGPS_AIDING_IONOUTC_SUBTYPE            (0x0A) 
#define GNSS_LIBGPS_AIDING_POSVELTIME_SUBTYPE         (0x0B)
#define GNSS_LIBGPS_AIDING_BDSALMANAC_SUBTYPE         (0x0C)
#define GNSS_LIBGPS_AIDING_GPSALMANAC_SUBTYPE         (0x0D)
#define GNSS_LIBGPS_AIDING_GPSEPHEMERIS_SUBTYPE       (0x0E)
#define GNSS_LIBGPS_AIDING_BDSEPHEMERIS_SUBTYPE       (0x0F)
#define GNSS_LIBGPS_AIDING_RCVSTATUS_SUBTYPE          (0x10)
#define GNSS_LIBGPS_AIDING_BDSALMPACKEDCOPY_SUBTYPE   (0x11)
#define GNSS_LIBGPS_AIDING_SET_SENSOR_SUBTYPE         (0x12)
#define GNSS_LIBGPS_AIDING_SENSOR_SUBTYPE             (0x13)
//define GNSS_LIBGPS_AIDING_DATA_TYPE  end

//GNSS_LIBGPS_POWER_REQUEST_TYPE  begin
#define GNSS_LIBGPS_POWER_ON_REQ_SUBTYPE                 (0x01)                
#define GNSS_LIBGPS_POWER_SHUTDOWN_REQ_SUBTYPE           (0x02)
#define GNSS_LIBGPS_POWER_IDLEON_REQ_SUBTYPE             (0x03)
#define GNSS_LIBGPS_POWER_IDLEOFF_REQ_SUBTYPE            (0x04)
#define GNSS_LIBGPS_POWER_RESET_REQ_SUBTYPE              (0x05)
//define  GNSS_LIBGPS_POWER_REQUEST_TYPE   end 

//define GNSS_LIBGPS_POWER_RESPONSE_TYPE  begin 
#define GNSS_LIBGPS_POWERON_STATUS_SUCCESS        (0x01)
#define GNSS_LIBGPS_POWERON_STATUS_FAILED         (0x02)
#define GNSS_LIBGPS_IDLON_STATUS_SUCCESS          (0x03)
#define GNSS_LIBGPS_IDLON_STATUS_FAILED           (0x04)
#define GNSS_LIBGPS_IDLOFF_STATUS_SUCCESS         (0x05)
#define GNSS_LIBGPS_IDLOFF_STATUS_FAILED          (0x06)
#define GNSS_LIBGPS_SHUTDOWN_STATUS_SUCCESS       (0x07)
#define GNSS_LIBGPS_SHUTDOWN_STATUS_FAILED        (0x09)
#define GNSS_LIBGPS_RESET_STATUS_SUCCESS          (0x0A)
#define GNSS_LIBGPS_RESET_STATUS_FAILED           (0x0B)
//define GNSS_LIBGPS_POWER_RESPONSE_TYPE end  


//define GNSS_LIBGPS_SET_LOGLEVEL_SUBTYPE begin
#define GNSS_LIBGPS_DEBUG_OFF                     (0x01)
#define GNSS_LIBGPS_DEBUG_LEVEL0                  (0x02)
#define GNSS_LIBGPS_DEBUG_LEVEL1                  (0x03)
#define GNSS_LIBGPS_DEBUG_LEVEL2                  (0x04)
//define GNSS_LIBGPS_SET_LOGLEVEL_SUBTYPE end 

//define GNSS_LIBGPS_NOTIFIED_TYPE begin
#define GNSS_NOTIFIED_DEVICE_STATE_SUBTYPE        (0x01)
//define GNSS_LIBGPS_NOTIFIED_TYPE end

#define GNSS_DEVICE_READY_STATE                   (0x01)
#define GNSS_DEVICE_SLEEP_STATE                   (0x02)
#define GNSS_DEVICE_ABOUT_STATE                   (0x03)


// GNSS_LIBGPS_EUT_TYPE   begin                      
#define GNSS_LIBGPS_EUT_CW_SUBTYPE                (0x01)
#define GNSS_LIBGPS_EUT_MIDBAND_SUBTYPE           (0x02)
#define GNSS_LIBGPS_EUT_TCXO_SUBTYPE              (0x03)
#define GNSS_LIBGPS_EUT_TSXTEMP_SUBTYPE           (0x04)
#define GNSS_LIBGPS_EUT_RFDATA_CAP_SUBTYPE        (0x05)
// GNSS_LIBGPS_EUT_TYPE   end        

// GNSS_LIBGPS_REG_WR_TYPE   begin                      
#define GNSS_LIBGPS_REG_WRITE_SUBTYPE                (0x01)     // GNSS_REG_WR
#define GNSS_LIBGPS_REG_READ_SUBTYPE                 (0x02)     // GNSS_REG_WR
#define GNSS_LIBGPS_REG_VALUE_SUBTYPE                 (0x03)     // GNSS_REG_WR
// GNSS_LIBGPS_REG_WR_TYPE   end


//Assistant DATA 
#define DATA_POSVELTIME							  (0x0001)
#define DATA_IONOUTC							  (0x0002)
#define DATA_GPSSEPHEMERIS						  (0x0004)
#define DATA_GPSALMANAC							  (0x0008)
#define DATA_BDSEPHEMERIS						  (0x0010)
#define DATA_GPSALMPACKEDCOPY					  (0x0020)
#define DATA_BDSALMPACKEDCOPY					  (0x0040)
#define DATA_ACQUISITION_ASSISTANCE               (0x0080)


//it define encode and decode    begin 
#define  GNSS_MAGIC_NUMBER_LEN   (4)
#define  GNSS_MAX_DATA_CRC       (2)
#define  GNSS_MAGIC_NUMBER       (0x7E)
#define  GNSS_MAX_HEAD_LEN       (10)
#define  GNSS_LOG_BB_LEN         (32)
#define  GNSS_LOG_DDDD_LEN       (57)
#define  GNSS_LOG_LLLL_LEN       (41)
#define  GNSS_LOG_SSSS_LEN       (33)
#define  GNSS_LOG_FFFF_LEN       (41)
#define  GNSS_LOG_DDDD_CRC       (0xFDC0)
#define  GNSS_LOG_LLLL_CRC       (0xFDCF)
#define  GNSS_LOG_SSSS_CRC       (0xFDD6)
#define  GNSS_LOG_FFFF_CRC       (0xFDCD)

#define GNSS_GET_HIGH_BYTE(D) (unsigned char)((D) >> 8) 
#define GNSS_GET_LOW_BYTE(D)  (unsigned char)(D)
#define TRUE 1
#define FALSE 0

#define GNSS_FLASH_BLOCK_SIZE  4096
//it define encode and decode   end  



/********************************************************
*                         Structrues
*********************************************************/
#define  MAX_SATS_NUMBERS   			          (32)
#define  MAX_SATS_IN_VIEW			              (16)
#define  MAX_MSG_BUFF_SIZE                        (2038)//from 1024 to 3904
#define  STRING_MAX_LENGTH                        (15)
typedef enum _EWorkMode
{
	COLD_START_MODE=1,
	WARM_START_MODE,
	HOT_START_MODE,
	FAC_START_MODE
}EWorkMode_e;
typedef struct _TPosLLH
{
    double lat;               /** geographical latitude   */
    double lon;               /** geographical longitude  */
    double hae;               /** geographical altitude   */
}TPosLLH_t;

typedef struct _TVelocity
{
    float  horizontalSpeed;    // kmh
    float  bearing;
    float  verticalSpeed;      // kmh
    char   verticalDirect;     // 0: upward, 1:downward
} TVelocity_t;

typedef struct _TGpsTime
{
    unsigned short   weekNo;          /*< week number (from beginning of current week cycle) */
    unsigned long    second;          /*<  seconds (from beginning of current week no) */
    unsigned long    secondFrac;      /*<  seconds fraction (from beginning of current second) units: 1ns */
} TGpsTime_t;


typedef struct _TEphemerisPacked  			
{
    unsigned char	flag;	// bit0 means ephemeris valid
    unsigned char	svid;

    unsigned int word[30];
}TEphemerisPacked_t;//sizeof= 122Bytes

typedef struct _TGpsAlmanacPacked
{
    unsigned int flag;//bit 0-31 indicates alm validility flag of sv1-32
    unsigned int FramePageWord[2][25][8];
}TGpsAlmanacPacked_t;//sizeof= 1604Bytes

typedef struct _TBd2sAlmanacPacked
{
    unsigned int flag;//bit 0-29 indicates alm validility flag of sv1-30
    unsigned int healthFlag;//bit 0-29 indicates alm health info of sv1-30,defaulted value is 0(0 indicates healthy),changed when decoded from pages
    unsigned int FramePageWord[2][24][8];//WORD2-9 for decode,pay attention that 2LSB of word2 has been put into 2MSB of word3 both D1&D2
}TBd2sAlmanacPacked_t;//sizeof= 1544Bytes

typedef struct _TGpsAlmanacPackedCopy
{
    unsigned int svMask[2][25];//bit 0-31 indicates svMask one page of one subframe in one copy
    unsigned int FramePageValid[2];	// valid flag for page1~25 in subframe 4/5
    unsigned int FramePageWord[2][25][8];
}TGpsAlmanacPackedCopy_t;//sizeof= 1808Bytes

typedef struct _TBd2AlmanacPackedCopy
{
    unsigned int svMask[2][24];//bit 0-31 indicates svMask one page of one subframe in one copy
    unsigned int FramePageValid[2];	// valid flag for page1~25 in subframe 4/5
    unsigned int FramePageWord[2][24][8];//WORD3-10 for data aiding,only for D1
}TBd2AlmanacPackedCopy_t;//sizeof= 1736Bytes

typedef struct _TAlmanac
{
	unsigned char	flag;
	unsigned char	svid;

	unsigned short	health;   //type changed from unsigned char to unsigned short, for health of BD2 occupy 9 bits

	int	toa;
	int	week;

	// variables decoded from stream data
	double M0;			// Mean Anomaly at Reference Time
	double ecc;			// Eccentricity
	double sqrtA;		// Square Root of the Semi-Major Axis
	double omega0;		// Longitude of Ascending Node of Orbit Plane at Weekly Epoch
	double i0;			// Inclination Angle at Reference Time
	double w;			// Argument of Perigee
	double omega_dot;	// Rate of Right Ascension
	double af0;			// Satellite Clock Correction
	double af1;			// Satellite Clock Correction

	// variables derived from basic data, avoid calculate every time
	double axis;		// Semi-major Axis of Orbit
	double n;			// Mean Angular Rate
	double root_ecc;	// Square Root of One Minus Ecc Square
	double omega_t;		// Longitude of Ascending Node of Orbit Plane at toe
	double omega_delta;	// Delta Between omega_dot and WGS_OMEGDOTE
}TAlmanac_t ;
typedef struct _TAddionalDopplerFields
{
    float     doppler1;            // range: [-1.0, 0.5]hz/sec 
    float     dopplerUncertainty;  // range: [12.5, 200]hz
}TAddionalDopplerFields_t;

typedef struct _TAddionalAngleFields
{
    float     azimuth;   // range:0--384.75 deg
    float     elevation; // range:0--78.75 deg
} TAddionalAngleFields_t;

typedef struct _TAcquisElement
{
    unsigned char                svid;           		 // range:1--64
    unsigned char                intCodePhase;           // range:0--19 C/A period
    unsigned char                gpsBitNumber;           // range:0--3
    unsigned char                codePhaseSearchWindow;  // range:0--192 chips
    unsigned char                addionalAngleFlag;      // 1: addionalAngle is valid
    unsigned char                addionalDopplerFlag;    // 1: addionalDoppler is valid
    unsigned short               codePhase;              // range:0--1022 chips
    TAddionalDopplerFields_t     addionalDoppler;
    TAddionalAngleFields_t       addionalAngle;      
    float                        doppler0;       		 // range:-5120...5117.5
} TAcquisElement_t;

typedef struct _TAcquisAssist
{
    unsigned char    count;     // acquisElement[0]...acquisElement[count-1] is valid value
    unsigned long    gpsTime;   // unit: ms
    TAcquisElement_t acquisElement[MAX_SATS_IN_VIEW];   
} TAcquisAssist_t;


typedef struct _TMeasureElement
{
	unsigned char         satelliteID;         // range: 0-63
	unsigned char         cNo;                 // carrier noise ratio, range:(0..63)
	unsigned char         pseuRangeRMSErr; 	   // index, range:(0..63)
	//MpathIndic          mpathInd;            // multipath indicator
	unsigned short        wholeChips;          // whole value of the code phase measurement, range:(0..1022)
	unsigned short        fracChips;           // fractional value of the code phase measurement, range:(0..1023),
	float                 doppler;             // doppler, mulltiply by 0.2, range:(-32768..32767)
}  TMeasureElement_t;
typedef struct _TMeasuresList
{
    unsigned long       gpsTOW;
    unsigned short      listNums;  
    TMeasureElement_t   msrElement[MAX_SATS_IN_VIEW];
} TMeasuresList_t;

typedef struct _TGpsEphemerisList
{	
	TEphemerisPacked_t Ephemeris[MAX_SATS_IN_VIEW];
}TGpsEphemerisList_t;

typedef struct _TBdsEphemerisList
{
	TEphemerisPacked_t Ephemeris[MAX_SATS_IN_VIEW];
}TBdsEphemerisList_t;

typedef struct _TGpsAlmanac
{
	int index;
	TGpsAlmanacPackedCopy_t Almanac;
}TGpsAlmanacCopyList_t;
typedef struct _TBdsAlmanac
{
	int index;
	TBd2AlmanacPackedCopy_t Almanac;
}TBdsAlmanacCopyList_t;

typedef struct _TIonoParam
{
    float	a0;  // 2**-30
    float	a1;  // 2**-27
    float	a2;  // 2**-24
    float	a3;  // 2**-24
    float	b0;  // 2**11
    float	b1;  // 2**14
    float	b2;  // 2**16
    float	b3;  // 2**16
    unsigned long	flag; // 1, availble   
} TIonoParam_t;

// UTC parameters
typedef struct _TUtcParam
{
	double	A0;  // second, 2**-30
	double	A1;  // second/second, 2**-50
	short	WN;
	short	WNLSF;
	unsigned char	tot; // 2**12
	unsigned char	TLS; // leap second
	unsigned char	TLSF;
	unsigned char	DN;
	unsigned int	flag; // 1, availble   
} TUtcParam_t;

typedef struct _TRtcTime
{
	int          flag;
	int          Reserved;
	int          week;
	int          msCount;
	unsigned int timeUnc;//heidi add
} TRtcTime_t;

typedef struct _TPosVelParam
{
	int           PosQuality;
	int           Reserved;
	double        freqBias;
	TPosLLH_t     ReceiverPos;
	unsigned int  posUnc;//heidi add
}TPosVelParam_t;

typedef struct _TTimeDiff
{
	int    flag;
	int    Reserved;
	double ToGPSTimeDiff;
}TTimeDiff_t;

typedef struct _TPosVelTimeParam
{
	TPosVelParam_t PosVelParam;
	TRtcTime_t     RTCTime;
	TTimeDiff_t    TimeDiff;
	unsigned int   CheckSum;
}TPosVelTimeParam_t;

typedef struct _TIonoUtcParam
{
	TIonoParam_t IonoParam[2];
	TUtcParam_t  UTCParam[2];
	unsigned int CheckSum;
}TIonoUtcParam_t;


typedef struct _TCmdData 
{
	unsigned char  type;
	unsigned char  subType;
	unsigned short length;
	unsigned char  buff[MAX_MSG_BUFF_SIZE];
}TCmdData_t;

typedef struct _TLogDDDDFormat4
{
	char str[STRING_MAX_LENGTH];
	double value1;
	double value2;
	double value3;
	double value4;
}TLogDDDDFormat4_t;

typedef struct _TLogLLLLFormat4
{
	char str[STRING_MAX_LENGTH];
	long value1;
	long value2;
	long value3;
	long value4;
}TLogLLLLFormat4_t;

typedef struct _TLogSSSSFormat4
{
	char str[STRING_MAX_LENGTH];
	short value1;
	short value2;
	short value3;
	short value4;
}TLogSSSSFormat4_t;

typedef struct _TLogFFFFFormat4
{
	char str[STRING_MAX_LENGTH];
	float value1;
	float value2;
	float value3;
	float value4;
}TLogFFFFFormat4_t;

typedef struct _TLogSDSDSDFormat6
{
	char FunctionName[STRING_MAX_LENGTH];
	char str1[STRING_MAX_LENGTH];
	double value1;
	char str2[STRING_MAX_LENGTH];
	double value2;
	char str3[STRING_MAX_LENGTH];
	double value3;
}TLogSDSDSDFormat6_t;

//it define encode and decode struct union  begin 
typedef enum
{
    GNSS_RECV_SEARCH_FLAG,
    GNSS_RECV_COLLECT_HEAD,
    GNSS_RECV_DATA,
    GNSS_RECV_Complete,
    GNSS_RECV_ERROR,
}GNSS_RecvState_e;

// GNSS frame (User Data part only)
typedef struct 
{
    char type;
    char subtype;
    unsigned short dataLen;
    unsigned char* pBuf;
} GNSS_packet_t;

typedef struct __GNSS_DataProcessor
{
    GNSS_RecvState_e state;
	unsigned short headSize; //receive head len 
    unsigned short receivedDataLen;////receive data  len 
    unsigned short dataLen;//total  data len
    //add debug information
	unsigned short errorNum;
	unsigned char curHeader[GNSS_MAX_HEAD_LEN];
	//GNSS_packet_t  *pCurData;
	TCmdData_t  cmdData;
	
}TGNSS_DataProcessor_t;

typedef struct __TGNSS_Baseband_Log
{
    unsigned char   type;
    unsigned char   svid;
	unsigned short  data1; 
    unsigned short  data2;
    unsigned short  data3;
    unsigned int    bbtime;
    unsigned int    data4;
    unsigned int    data5;
    unsigned int    data6;
    unsigned int    data7;
    unsigned int    pad;
}TGNSS_Baseband_Log_t;



//it define encode and decode struct union  end 

//extern TGNSS_DataProcessor_t gGPSStream;
/*function define*/
void  GNSS_InitParsePacket(TGNSS_DataProcessor_t* pStream);
void GNSS_FillHead(TCmdData_t* pIndata,unsigned char* pOutData);
int GNSS_EncodeOnePacket( TCmdData_t* pInData,unsigned char* pOutData,unsigned short outLen);
int GNSS_ParseOnePacket(TGNSS_DataProcessor_t* pStream,  unsigned char* pData, unsigned short Len);


#endif //GNSS_LIBGPS_API_H
