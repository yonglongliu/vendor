#ifndef GNSS_GE2_TYPE_H
#define GNSS_GE2_TYPE_H

//typedef  unsigned int U32;
//typedef  unsigned short U16;
//typedef  unsigned char U8;
//typedef  int S32;
//typedef  short S16;
//#define DOUBLE double
//#define FLOAT float


#define GNSS_LIBGPS_AIDING_ASSISTANCE_SUBTYPE         (0x04)
#define	RAW_MSR		0
#define	RAW_SFR		1
#define	RAW_EPH		2
#define MAX_NMEA_TYPE  	9 
#define MAX_NAV_MESSAGE_TYPE  4
#define MAX_RAW_MESSAGE_TYPE (RAW_EPH + 1)
#define MAX_MISC_MESSAGE_TYPE 2

#define GPS_CH_NUM          14
#define BD2_CH_NUM		    14

#define GPS_ALM_COPY_MAX           6
#define BD2_ALM_COPY_MAX           4
/*******************PosVelTime********************/

//#define  MAX_SATS_ON_ALMANAC   (32)
#define ONES(n) ((1 << n) - 1)


typedef struct _TEphemeris
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
    unsigned int  e;                      /* 2^-33        2^-21(almanacE)     ---             */
    signed int    M_0;                    /* 2^-31        2^-23(almanacM0)    Semi-circles    */
    unsigned int  SquareA;                /* 2^-19        2^-11(almanacAPowerHalf)  (Meters)^1/2    */
    signed short   Del_n;                  /* 2^-43        -                   Semi-circles/sec*/
    signed int    OMEGA_0;                /* 2^-31        2^-23(almanacOmega0)Semi-circles    */
    signed int    OMEGA_dot;              /* 2^-43        2^-38(almanacOmegaDot) Semi-circles/sec*/
    unsigned int  I_0;                    /* 2^-31        -                   Semi-circles    */
    signed short   Idot;                   /* 2^-43        -                   Semi-circles/sec*/
    signed int    omega;                  /* 2^-31        2^-23(almanacW)     Semi-circles    */
    signed char    tgd;                    /* 2^-31        -                   Sec             */
    unsigned short t_oc;                   /* 2^+4         -                   Sec             */
    signed int    af0;                    /* 2^-31        2^-20               Sec             */
    signed short   af1;                    /* 2^-43        2^-38               sec/sec         */
    signed char    af2;                    /* 2^-55        -                   sec/sec2        */
}TEphemeris_t;

typedef union
{
	double d_data;
	unsigned int i_data[2];
} DOUBLE_INT_UNION;

typedef struct
{
	DOUBLE lat;
	DOUBLE lon;
	DOUBLE hae;
} LLH;

typedef struct
{
	int RtcFlag;
	int week;
	int msCount;
	int    PosQuality;
	DOUBLE freqBias;
	LLH    ReceiverPos;
	int    TimeDiffFlag;
	DOUBLE ToGPSTimeDiff;
	DOUBLE timeUnc;
	DOUBLE posUnc;
	DOUBLE clkErr;
  DOUBLE RFTDrifting;
	U8 simuStaticModeFlag;
	U32 MeasLatchRFTLow;
	U32 MeasLatchRFTHigh;
	U32 weekforpos;
	U32 mscountforpos;
} PVTMISC_PACKAGE;

typedef struct
{
	PVTMISC_PACKAGE 	PvtPkg;
	U32    		 		CheckSum;
} PVTMISC_FLASH;

/*******************Rcv Config**********************/

typedef struct
{
	U32 baud;
	U8 charLen;		///< charLen: 0 - 8bit, 1 - 5bit, 2 - 6bit, 3 - 7bit
	U8 parity;		///<	parity:  0/1 - no parity, 2 - odd parity, 3 - even parity
	U8 nStopBits;	///< stopBits: 0 - 1bit, 1 - 1.5bits, 2 - 2bits
	U8  outProtocol;
	U32 inProtocol;
} UART_CONFIG_T, *PUART_CONFIG_T;

typedef struct
{
	U32 nmeaMask;
	U8 nmeaRate[MAX_NMEA_TYPE];			///< the output rate for each nmea type
	U8 navMsgRate[MAX_NAV_MESSAGE_TYPE];	///< the output rate for each navigation mesasge type
	U8 rawMsgRate[MAX_RAW_MESSAGE_TYPE];	///< the output rate for each raw measurement type
	U8 miscMsgRate[MAX_MISC_MESSAGE_TYPE];///< the output rate for each misc message type
	U8 dbgRate;								///< the output rate for debug information
} MSG_CONFIG_T, *PMSG_CONFIG_T;

typedef struct
{
	U32 interval;		///< pulse interval
	U32 length;			///< pulse length 
	U8 flag;			///< Bit 0: enable Pulse; Bit 1 Polar; Bit 2: always output;  Bit 3: Timtp en
	S16 antennaDelay;	///< in unit of ns
	S16 rfDelay;		///< in unit of ns
	S32 userDelay;
} PPS_CONFIG_T, *PPPS_CONFIG_T;

typedef struct
{
	U8 enable;
	U8 polarity;
	U8 clockSync;
} EM_CONFIG_T, *PEM_CONFIG_T;

typedef struct
{
	U8 timingMode;
	U32 duration;
	LLH staticPos;
} PPS_TM_T, *PPPS_TM_T;

typedef struct
{
	U16 LimitVEn;
	U16 LimitVMax;
	U16 LimitVFilterScale;
	U16 LimitVFilterLength;
}LIMIT_V_CONFIG, *PLIMIT_V_CONFIG;

typedef struct
{
	U16 measRate;
	U16 navRate;
	U8 NMEAVersion;
	U8 RTCMVersion;
	U8 sysMask;
	U8 pvtUsage;
	U8 errCorrFlag;
	U8 DynDynMode;
	U32 DynMaxVel;
	UART_CONFIG_T UARTConfig[2];
	MSG_CONFIG_T MsgConfig;
	PPS_CONFIG_T PPSConfig;
	EM_CONFIG_T EMConfig;
	PPS_TM_T PPSMode;
	LIMIT_V_CONFIG LimitVConfig;
} RCV_CONFIG_T, *PRCV_CONFIG_T;

typedef struct
{
	RCV_CONFIG_T RCVConfig;
	U32    CheckSum;
} RCV_CONFIG_FLASH,* PRCV_CONFIG_FLASH;
/********************IONO***************************/


typedef struct
{
    FLOAT	a0;  
    FLOAT	a1;  
    FLOAT	a2; 
    FLOAT	a3; 
    FLOAT	b0;  
    FLOAT	b1;  
    FLOAT	b2;  
    FLOAT	b3; 
    unsigned int	flag; 
} IONO_PARAM, *PIONO_PARAM;

typedef struct
{
	DOUBLE	A0;  
	DOUBLE	A1;  
	short	WN;
	short	WNLSF;
	unsigned char	tot; 
	unsigned char	TLS; 
	unsigned char	TLSF;
	unsigned char	DN;
	unsigned int	flag;  
} UTC_PARAM, *PUTC_PARAM;
typedef struct
{
	IONO_PARAM  IonoParam[2];
	UTC_PARAM  	UTCParam[2];
	U32 		CheckSum;
}IONOUTC_PARAM_FLASH;


typedef struct
{
    unsigned char	flag;	// bit0 means ephemeris valid  &0x1==1 valid
    unsigned char	svid;

    unsigned int word[30];

}GPS_EPHEMERIS_PACKED, *PGPS_EPHEMERIS_PACKED;//sizeof= 122Bytes

typedef struct
{
	GPS_EPHEMERIS_PACKED Ephemeris[8];
	U32 		  CheckSum;
}BLOCK_EPHEMERIS_FLASH;

	
typedef struct
{
	DOUBLE x, y, z;
	DOUBLE vx, vy, vz;
	DOUBLE ve, vn, vu, Speed, Course, Elev;
	DOUBLE ClkDrifting;

} KINEMATIC_INFO, *PKINEMATIC_INFO;

typedef struct
{
	unsigned char flag;	// bit0 means ephemeris valid
						// bit1 means position and velocity at tc valid
	signed char freq;	// frequency number of satellite
	unsigned char P;	// place P1, P2, P3, P4, ln, P from LSB at bit
						//      0/1,  2,  3,  4,  5, 6
	unsigned char M;	// satellite type 00 - GLONASS, 01 - GLONASS-M
	unsigned char Ft;	// indicator of accuracy of measurements
	unsigned char n;	// slot number that transmit signal
	unsigned char Bn;	// healthy flag
	unsigned char En;	// age of the immediate information
	unsigned int tb;	// index of interval within current day
	unsigned short tk;  // h:b76-b72,m:b71-b66,s:b65 * 30
	DOUBLE gamma;		// relative deviation of predicted carrier frequency
	DOUBLE tn;			// satellite clock error
	DOUBLE dtn;			// time difference between L1 and L2
	DOUBLE x, y, z;		// posistion in PZ-90 at instant tb
	DOUBLE vx, vy, vz;	// velocity in PZ-90 at instant tb
	DOUBLE ax, ay, az;	// acceleration in PZ-90 at instant tb

	// derived variables
	DOUBLE tc;			// reference time giving the following position and velocity
	KINEMATIC_INFO PosVelT;	// position and velocity in CIS coordinate at instant tc
} GLONASS_EPHEMERIS, *PGLONASS_EPHEMERIS;

typedef struct
{
	GLONASS_EPHEMERIS Ephemeris[7];
	U32 		  	  CheckSum;
}BLOCK_GLO_EPHEMERIS_FLASH;



typedef struct
{
    unsigned int flag;
    unsigned int FramePageWord[2][25][8];
}GPS_ALMANAC_PACKED, *PGPS_ALMANAC_PACKED;

typedef struct
{
	GPS_ALMANAC_PACKED GpsAlmPacked;//sizeof(GPS_ALMANAC_PACKED)=1604Bytes
	U32         weekno[32];
	U32 		CheckSum;
}BLOCK_GPS_ALMANAC_FLASH;


typedef struct
{
    U32 agps_mode;
    BLOCK_GPS_ALMANAC_FLASH gpsalm;
}BLOCK_GPS_ALMANAC_SEND;


typedef struct
{
    unsigned int flag;
    unsigned int healthFlag;
    unsigned int FramePageWord[2][24][8];
}BD2_ALMANAC_PACKED, *PBD2_ALMANAC_PACKED;
typedef struct
{
	BD2_ALMANAC_PACKED BD2AlmPacked;//sizeof(BD2AlmPackedCopy)=1544Bytes
	U32 		CheckSum;
}BLOCK_BD2_ALMANAC_FLASH;




typedef struct
{
	unsigned char flag;	
	signed char freq;	
	unsigned short t;
	
	int ReferenceDay;	
	DOUBLE lambda;		
	DOUBLE i;		
	DOUBLE ecc;		
	DOUBLE w;			
	DOUBLE dt;			
	DOUBLE dt_dot;		
	DOUBLE axis;		
	DOUBLE n;		
	DOUBLE root_ecc;	
	DOUBLE lambda_dot;	
	DOUBLE omega_dot;	
} GLONASS_ALMANAC, *PGLONASS_ALMANAC;

typedef struct
{
	GLONASS_ALMANAC Almanac[14];
	U32 			CheckSum;
}BLOCK_GLO_ALMANAC_FLASH;



typedef struct
{
    unsigned int svMask[2][25];
    unsigned int FramePageValid[2];	
    unsigned int FramePageWord[2][25][8];
    int weekNum;
    int GpsMsCount;
}GPS_ALMANAC_PACKED_COPY, *PGPS_ALMANAC_PACKED_COPY;



typedef struct
{
    unsigned int svMask[2][24];
    unsigned int FramePageValid[2];
    unsigned int FramePageWord[2][24][8];
}BD2_ALMANAC_PACKED_COPY, *PBD2_ALMANAC_PACKED_COPY;

typedef struct
{
	GPS_ALMANAC_PACKED_COPY GpsAlmPackedCopy;//sizeof(GPS_ALMANAC_PACKED_COPY)=1808Bytes
	U32 		CheckSum;
}BLOCK_GPS_ALMCOPY_FLASH;
typedef struct
{
	unsigned int GpsFrame4Page13[32][8];//32*8*4=1024Bytes
	unsigned int GpsFrame4Page13Valid;
    U32 		CheckSum;
}BLOCK_GPS_ALMCOM_FLASH;

typedef struct
{
    int b_SeqLsqSet;
    int b_ResetSeqLsq;
    int b_TtffTest;
    int  TryKnownCnt;
    char FirstGoodPosSetFlag;
    DOUBLE VarPosAtStart[3];
    DOUBLE PosAtStart[3];
    DOUBLE VarPosAtStartBack[3];
    DOUBLE PosAtStartBack[3];
    DOUBLE FixErr;
    int PosSat;
    int GpsMsCount;
    int WeekNumber;
    unsigned int StartMode;
}SEQLSQ_INFO;

typedef struct
{
    SEQLSQ_INFO SeqLsqParam;
    U32         CheckSum;
}SEQLSQ_PARAM_FLASH;

typedef struct
{
    int valid;
    int weekNo;
    int GpsMsCount;
    DOUBLE lat;
    DOUBLE lon;
    DOUBLE hae;
}PVT_NAV_POS_INFO, *PPVT_NAV_POS_INFO;

typedef struct
{
    PVT_NAV_POS_INFO LastFix;
    PVT_NAV_POS_INFO LastGoodFix;
    U32         CheckSum;
}BLUNDER_INFO_FLASH;
#endif

