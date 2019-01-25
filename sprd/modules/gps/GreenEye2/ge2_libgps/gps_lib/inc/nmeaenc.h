/*
* Copyright (c) 2010, Unicore Communications Inc.,
* All rights reserved.
* 
* Description: nmeaenc.h is declaration of NMEA0183 encode
* 
* Current Version: 0.1
* Modified By: Jun Mo
* Modify Date: 2010/3/19
* 
* Original Author: Jun Mo
* Create Date: 2010/3/19
*/

#ifndef __NMEA_ENCODE_H__
#define __NMEA_ENCODE_H__
#include "gps_common.h"

#define GPS				0
#define BD1				1
#define BD2				2
#define GAL				3
#define GLO				4

#define SBAS			5

#define TOTAL_CH_NUM 28
#define DIMENSION_MAX_X 26


#define MIN_GPS_PRN 1
#define MAX_GPS_PRN 32
#define MIN_GLONASS_PRN 65
#define MAX_GLONASS_PRN 92
#define GLONASS_PRN_NUM 28
#define GLONASS_SVID_NUM 14
#define MIN_BD2_PRN 151
#define MAX_BD2_PRN 187
#define MIN_QZSS_PRN 33
#define MAX_QZSS_PRN 37
#define MIN_SBAS_PRN 120
#define MAX_SBAS_PRN 138
#define MIN_GPS_SVID 1
#define MAX_GPS_SVID 32
#define MIN_GLONASS_SVID 65
#define MAX_GLONASS_SVID 78
#define MIN_BD2_SVID 151
#define MAX_BD2_SVID 187


#define ENABLE_GPS				1                  
#define ENABLE_GLONASS			1
#define ENABLE_BD2				0
#define ENABLE_SBAS             1
#define ENABLE_QZSS             1
#define ENABLE_AGPS             1
#define IS_BB_BITSYNC(status) ((status & 0x3f) >= 0x20)
#define SYSTEM(SIGNAL)	(SIGNAL & 0x7)

#define IS_GPS(freqID) (SYSTEM(freqID) == GPS)
#define IS_GLO(freqID) (SYSTEM(freqID) == GLO)
#define IS_BD2(freqID) (SYSTEM(freqID) == BD2)
#define IS_GAL(freqID) (SYSTEM(freqID) == GAL)

#define IS_GPS_SVID_BB(svid, freqID) ((svid) <= MAX_GPS_SVID && ENABLE_GPS && IS_GPS(freqID))
#define IS_GPS_SVID(svid) ((svid) >= MIN_GPS_SVID && (svid) <= MAX_GPS_SVID && ENABLE_GPS)
#define IS_GLONASS_SVID(svid) ((svid) >= MIN_GLONASS_SVID && (svid) <= MAX_GLONASS_SVID && ENABLE_GLONASS)
#define IS_BD2_SVID(svid) ((svid) >= MIN_BD2_SVID && (svid) <= MAX_BD2_SVID && ENABLE_BD2)
#define IS_BD2_GEO(svid) ((svid) >= MIN_BD2_SVID && (svid) < (MIN_BD2_SVID + 5))
#define IS_BD2_IGSO(svid) ((svid) >= (MIN_BD2_SVID + 5) && (svid) < (MIN_BD2_SVID + 10))
#define IS_BD2_MEO(svid)  ((svid) >= (MIN_BD2_SVID + 10) && (svid) <= MAX_BD2_SVID)
#define IS_BD2_NGEO(svid)  (IS_BD2_IGSO(svid) || IS_BD2_MEO(svid))
#define IS_GPS_PRN(prn) ((prn) >= MIN_GPS_PRN && (prn) <= MAX_GPS_PRN && ENABLE_GPS)
#define IS_GLONASS_PRN(prn) ((prn) >= MIN_GLONASS_PRN && (prn) <= MAX_GLONASS_PRN && ENABLE_GLONASS)
#define IS_BD2_PRN(prn) ((prn) >= MIN_BD2_PRN && (prn) <= MAX_BD2_PRN && ENABLE_BD2)
#define IS_SBAS_PRN(prn) ((prn) >= MIN_SBAS_PRN && (prn) <= MAX_SBAS_PRN && ENABLE_SBAS)
#define IS_QZSS_PRN(prn) ((prn) >= MIN_QZSS_PRN && (prn) <= MAX_QZSS_PRN && ENABLE_QZSS)
#define IS_GPS_QZSS_PRN(prn) ((prn) >= MIN_GPS_PRN && (prn) <= GPS_QZSS_TOTAL_NUM && ENABLE_GPS)

#define	NMEA_GGA		0
#define	NMEA_GLL		1
#define	NMEA_GSA		2
#define	NMEA_GSV		3
#define	NMEA_RMC		4
#define	NMEA_VTG		5
#define	NMEA_ZDA		6
#define NMEA_GST        7
#define NMEA_DHV		8


#define	MSGGGA		(1 << NMEA_GGA)
#define	MSGGLL		(1 << NMEA_GLL)
#define	MSGGSA		(1 << NMEA_GSA)
#define	MSGGSV		(1 << NMEA_GSV)
#define	MSGRMC		(1 << NMEA_RMC)
#define	MSGVTG		(1 << NMEA_VTG)
#define	MSGZDA		(1 << NMEA_ZDA)
#define MSGGST      (1 << NMEA_GST)
#define MSGDHV		(1 << NMEA_DHV)


/// the number of supported NMEA type
#define MAX_NMEA_TYPE  	9 //(NMEA_DHV + 1)

#define MAX_NMEA_STRINGS 16
#define MAX_NMEA_STRING_LEN 80

#define NMEA_VER_30		0x30
#define NMEA_VER_BD		0x40
#define NMEA_VER_ZY		0x41

#define NMEA_STRING_LEN		256

int NmeaEncode(unsigned int NmeaMask, unsigned char NmeaVersion);

////////////////////////////basictype.h////////////////////
/*
* Copyright (c) 2010, Unicore Communications Inc.,
* All rights reserved.
* 
* Descrition: basictype.h is used to define basic types used by PVT
* 
* Current Version: 0.1
* Modified By: Jun Mo
* Modify Date: 2010/1/5
* 
* Original Author: Jun Mo
* Create Date: 2010/1/5
*/

#define ERR_NO_ERROR 0
#define ERR_NULL_POINTER 1
#define ERR_DIVIDE_BY_ZERO 2
#define ERR_INVALID_BRANCH 3

//typedef int BOOL;



typedef long long unsigned int UINT64;


#define TRUE 1
#define FALSE 0

#ifndef NULL
#define NULL 0//((void *)0)
#endif
#define UM220_TRACK 1
#define EXTERN extern
typedef int (* TASK_FUNC_T) (void *param);
typedef enum { UnknownSignalMode = 0, SignalUltraLow, SignalLow, SignalMiddle, SignalHigh} SignalMode;
typedef enum { UnknownTime = 0, ExtSetTime, PreviewTime, CoarseTime, KeepTime, AccurateTime } TimeAccuracy;
typedef enum { UnknownPos = 0, ExtSetPos, KeepPos, FixedUpPos, RecursionPos,FlexTimePos, CoarsePos, KalmanPos, AccuratePos } PosAccuracy;
typedef enum { ParamNoType = 0, ParamGpsAlmanac, ParamGpsEphemeris, ParamGlonassAlmanac, ParamGlonassEphemeris,
				 ParamBD2Almanac, ParamBD2Ephemeris, ParamGPSIonosphere, ParamBD2Ionosphere, ParamGPSUTC, ParamBD2UTC,ParamGpsAlmCopy,ParamBD2AlmCopy,ParamQZSSAlmanac} ParamType;
typedef enum { SyncSystemNone = 0, SyncSystemGPS, SyncSystemGlonass, SyncSystemBD2, SyncSystemGalileo } TimeSyncSystem;
typedef enum { UnknownMode = 0, StaticMode, StaticTimingMode, PedestrianMode, VehicleMode, HighDynamicMode} MotionMode;
typedef enum {COLD_START = 1,HOT_START = 2,}START_MODE;


typedef struct
{
	double lat;
	double lon;
	double hae;
} NLLH;

typedef struct
{
	int Year;
	int Month;
	int Day;
	int Hour;
	int Minute;
	int Second;
	int Millisecond;
	int reserved0;
} UTC_TIME, *PUTC_TIME;


// definition for above FrameFlag field

#ifdef UM220_TRACK
	#define SUBFRAME1_VALID		  0x000000f0  //word3 is week and health flag, word 456 in frame1 is reserved
#else
	#define SUBFRAME1_VALID		  0x000000f1  //word3 is week and health flag, word 456 in frame1 is reserved
#endif

#define SUBFRAME2_VALID		  0x0000ff00
#define SUBFRAME3_VALID		  0x00ff0000  //indicate words valid for subframe 1,2,3
#define ALL_SUBFRAME_VALID	  (SUBFRAME1_VALID|SUBFRAME2_VALID|SUBFRAME3_VALID)

#define WORD_CONTINUOUS1 	  0x01000000
#define WORD_CONTINUOUS2      0x02000000
#define STORE_WORD_VALID	  0x04000000
#define EPHEMERIS_VERIFIED    0x10    //store in epherimeris flag

#define WEEKNUMBER_BITS		0xfff00000 //week number bits in word3 or ephemeris packed word7(cover 30/31bit for protection)

// definition for all frame info
#define NEGATIVE_STREAM		  0x10000000
#define POLARITY_VALID		  0x20000000
#define SUBFRAME_CONTINUOUS	  0x40000000
#define SUBFRAME_ERROR        0x80000000
// definition for above FrameFlag field
#define STRING1_VALID		0x02
#define STRING2_VALID		0x04
#define STRING3_VALID		0x08
#define STRING4_VALID		0x10
#define STRING5_VALID		0x20
#define FRAME_TYPE_VALID	0x40
#define FRAME_TYPE			0x80
#define STRING_ALM_EXIST	0x100
#define ALL_STRING_VALID	(STRING1_VALID | STRING2_VALID | STRING3_VALID | STRING4_VALID)

#define PVT_SYSTEM_NUM			2
#define MAX_RAW_MSR_NUMBER 26					// maximum raw measurement number
#define MAX_PVT_MSR_NUMBER 26	

#define BD2_EBW_BCH_PASS  0x1
#define BD2_EBW_CLCT      0x2
#define BD2_EBW_CLCT_ALL_NGEO  0x1ff7fdff
#define BD2_EBW_CLCT_ALL_GEO   0xffffffffff
#define BD2_GET_WEEK_NUMBER_NGEO   0x180   //SF1W2 & SF1W3 valid
#define BD2_GET_WEEK_NUMBER_GEO    0x3     //SF1P1W2 & SF1P1W3 valid

//definition for D2-Q
#define D2Q_TYPE1_VALID     0x010000
#define D2Q_TYPE2_VALID     0x020000
#define D2Q_TYPE3_VALID     0x040000
#define D2Q_TYPE4_VALID     0x080000
#define D2Q_TYPE5_VALID     0x100000
#define D2Q_TYPE6_VALID     0x200000
#define D2Q_TYPE7_VALID     0x400000
#define D2Q_TYPE8_VALID     0x800000
#define ALL_D2Q_VALID       (D2Q_TYPE1_VALID | D2Q_TYPE3_VALID | D2Q_TYPE4_VALID)


#define BIAS_FIT_POINT      3
#define STATE_FIT_POINT     3

// definition for ChannelFlag field
#define CHANNEL_ACTIVE		0x01
#define GLONASS_HALF_CYCLE	0x02
#define ADR_INIT            0x04
#define ADR_VALID			0x08
#define TRANSTIME_ESTIMATE	0x10
#define MEASUREMENT_VALID	0x20
#define RAWDATA_VALID		0x40		//for frame data send to host, such as UB240
#define NAV_DATA_VALID	    0x100        //for data aiding usage and polarity decision usage
#define TLM_VALID			0x80
#define PSR_INVALID		    0x200 //0x100
#define DOPP_INVALID		0x400 //0x200


// definition for ChannelErrorFlag field
#define CHANNEL_ERR_BITSYNCERROR	0x01
#define CHANNEL_ERR_SIDELOBE		0x02      //NA
#define CHANNEL_ERR_DOPPLEGAP       0x0       //TBD
#define CHANNEL_ERR_NOTINVIEW		0x08
#define CHANNEL_ERR_XCORR           0x10
#define CHANNEL_ERR_FREQIDENTICAL   0x20      //NA
#define CHANNEL_ERR_PSRGAP          0x40      //NA
#define CHANNEL_ERR_MSERROR         0x80
#define CHANNEL_FORCE_RESET          0x100 // force reset due search mode changes
#define  LAGRANGE_POINTS_NUM (7)  //zhongwei.zhan @2015-5-6
#define  MJD_GPST0         44244



#define TIMEDIFF_VALID  0x01
#define TIMEDIFF_UPDATE 0x10 
#define SINGULAR_POS_VALUE  0.001
#define SINGULAR_VEL_VALUE  0.01


#define PVT_UTC_UPDATE		0x01
#define PVT_IONO_UPDATE		0x02

// definition for above PosFlag field
#define PVT_USE_GPS			(1 << GPS)
#define PVT_USE_GLONASS		(1 << GLO)
#define PVT_USE_BD2			(1 << BD2)
#define CHANNEL_HAS_GPS		(0x100 << GPS)
#define CHANNEL_HAS_GLONASS	(0x100 << GLO)
#define CHANNEL_HAS_BD2		(0x100 << BD2)
#define GPS_WEEK_VALID		 0x00010000
#define GLONASS_DAY_VALID	 0x00020000
#define GLONASS_YEAR_VALID	 0x00040000
#define BD2_WEEK_VALID		 0x00080000
#define GPS_WEEK_SET		 0x00100000
#define GLONASS_DAY_SET		 0x00200000
#define GLONASS_YEAR_SET	 0x00400000
#define BD2_WEEK_SET		 0x00800000
#define TIMING_POS_ERROR     0x01000000
#define GPS_DIFF_VALID		 0x02000000
#define BD2_DIFF_VALID		 0x04000000
#define UTC_TIME_VALID       0x08000000
#define CALC_ALL_GPS		 0x10000000
#define CALC_ALL_GLO		 0x20000000
#define CALC_ALL_BD2		 0x40000000
#define GPS_WEEK_EXIST		 (GPS_WEEK_VALID     | GPS_WEEK_SET)	 
#define GLONASS_DAY_EXIST	 (GLONASS_DAY_VALID  | GLONASS_DAY_SET)
#define GLONASS_YEAR_EXIST	 (GLONASS_YEAR_VALID | GLONASS_YEAR_SET)
#define BD2_WEEK_EXIST		 (BD2_WEEK_VALID     | BD2_WEEK_SET)

#define SAT_INFO_BY_SPRD_EPH                0x1     //spreadEph valid flag  
#define SAT_INFO_BY_SPRD_EPH_POINT          0x2     //spreadEph valid flag:Multi-point  
#define SAT_INFO_BY_SPRD_EPH_KEPLER         0x4     //spreadEph valid flag:Kepler parameter
#define SAT_INFO_BY_SPRD_EPH_REQUEST        0x8     //spreadEph requset flag
// definition for SatInfoFlag field
#define SAT_INFO_POSVEL_VALID		0x001
#define SAT_INFO_BY_EPH				0x002
#define SAT_INFO_ELAZ_VALID			0x004
#define SAT_INFO_PSR_VALID			0x008//psr,codephase
#define SAT_INFO_DOPPLER_VALID		0x010
#define SAT_INFO_STATIC_DOPPLER		0x020
#define SAT_INFO_EPH_EXPIRE			0x040
#define SAT_INFO_UPDATE 			0x080
#define SAT_INFO_CORRECTION_VALID 	0x100
#define SAT_INFO_BITAIDING_VALID	0x200
#define SAT_INFO_BIT_EST            0x400
#define SAT_INFO_BIT_WORD_EDGE      0x800

#define KALMAN_SELECT 1
#define ENABLE_SMOOTH_CN0 0

typedef struct
{
	double ErrSemiMinor,ErrSemiMajor,ErrOrientation;//parameter of error ellipse
	double ErrHorVel, ErrHorPos;
    double stdLat,stdLon,stdAlt;                    //standard deviation of latitude,longitude and altitude
	double rmsMSR;                                  //RMS of measurements
}GST_PARAMETER, *PGST_PARAMETER;


//@luqiang.liu 2015.11.25 this structures is used for sending data of CP to AP
#define MAX_SV_IN_GSV 36	// NMEA0183 standard, maximum 9 GSV sentences
typedef struct
{
    unsigned char prn;//CHANNEL INFO
    unsigned short ChannelFlag;		// bit 0: 0 - Inactive, 1 - Active
									// bit 1: half cycle adjust (GLONASS only) 0 - no adjust, 1 - adjust
									// bit 4: measurement 0 - Invalid, 1 - Valid
									// bit 5: raw subframe or raw string valid
	unsigned int state;				// same as state field in BB measurement
	unsigned char FreqID;			// Frequency ID
	unsigned short cn0;				// same as C/N0 field in BB measurement
} NMEA_CHANNEL_STAT, *PNMEA_CHANNEL_STAT;

typedef struct
{
    unsigned char FreqID;
    unsigned char prn;
}NMEA_CHANNELGSA_STAT,*PNMEA_CHANNELGSA_STAT;

typedef struct
{
    unsigned int NmeaMask;
    char OutputStye;
    unsigned char NmeaVersion;
	char reserved0[2];
    KINEMATIC_INFO PosVelRec;
    NLLH PosLLHRec;
	UTC_TIME RevUtcTime;//for uniform utc time output
	double geoUndu;
    int Quality;
    int PosQuality;
    int SatNum;
    unsigned int PosFlag;	// Positioning flag
    double Hdop, Vdop, Pdop;
    double tempspeed;

    NMEA_CHANNELGSA_STAT ChannelList[MAX_RAW_MSR_NUMBER];
	int reserved3;
    GST_PARAMETER GSTParameter;
    unsigned char PvtSystemUsage;
    unsigned char systemSvNum[8];
    unsigned char SvPrn[MAX_SV_IN_GSV];
	unsigned char SvEl[MAX_SV_IN_GSV];
	unsigned char SvCN0[MAX_SV_IN_GSV];
	short SvAz[MAX_SV_IN_GSV];
	short reserved4;
	int GpsMsCount;
	int reserved5;
} NMEA_CP2AP, *PNMEA_CP2AP;

typedef struct
{
    NMEA_CP2AP nmea_cp;
    unsigned int NmeaMask;
    char OutputStye;
    unsigned char NmeaVersion;
    unsigned char FreqID[26];
    unsigned char prn[26];
    int SatNum;
    GST_PARAMETER GSTParameter;
    double lon;
    double speed;
    UTC_TIME RevUtcTime;
    unsigned char PvtSystemUsage;
    unsigned int PosFlag;
    unsigned int sbasCalcSvid;
    int PosQuality;
}NMEA_PACKED;

#define TTFF_ENABLE 0
#define EST_ENABLE 0
#define EST_DETAIL_ENABLE 0
#define BDS_EBW 0
#define MEM_OPT_ENABLE 0
#define SBAS_CRCT_TEST  0
#define SBAS_TEST   0
#define QZSS_TEST  0
#define TEST_INIT  1
#define TEST_AGPS  1
#define TEST_AGLO  0
#define TEST_BBCT  1
#define TEST_BOOT  0
#define TEST_FIX   0
#define TEST_MSA   0
#define TEST_PVT   0
#define SPREAD_EPH_DBG_ENABLE 1      //@Zhongwei.Zhan 2015-03-20
#define TEST_POSSATFILTER 0
#define AP_RW_MEM  1    //@luqiang.liu 2015.07.20  output RF reg RW DEBUG
#define TEST_Softbit 0  //@phoebe.bi 2015.08.07
#define TEST_Softbit_post_progress 0 //@phoebe.bi 2015.09.18
#define TEST_PVTBlunderDetect 1


#endif //__NMEA_ENCODE_H__
