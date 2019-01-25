#ifndef __XEOD_CG_API_H
#define __XEOD_CG_API_H


#ifdef WIN32
#if defined(DLL_EXPORTS)
#define DLL_API_INT _declspec(dllexport) int __stdcall
#elif defined(DLL_IMPORTS)
#define DLL_API_INT _declspec(dllimport) int __stdcall
#else
#define DLL_API_INT int
#endif
#else
#define DLL_API_INT int
#endif

#ifdef WIN32
//20151222 add by hongxin.liu->for the output of orbit parameters from the requirement of firmware team 
#include "WTYPES.h"
#include "common_inc.h"
//20151222
#endif
#define XEOD_FILE_VERSION       1
/////////////////////////////////////////////////////////////////////////////////////////
// XEOD_CG related constants
/*
#define EGM_ORDER               8
#define RK_ORDER                10
#define AM_ORDER                12
#define MAX_RK_ORDER            10                    //Maximum RK Integrator Order
#define MAX_AM_ORDER            12                    //Maximum AM Integrator Order
#define MAX_INT_ORDER           12                    //Maximum Integration Order
#define MAX_INT_DIM             100                   //Maximum Integration Dimension, (v,a,partials) or (r,v,DesignMatrix)
*/
#define INTEG_STEPSIZE          100
#define PRED_CART_STEPSIZE      900
//20151123 add by hongxin.liu for Integrator_1Obs case
#define INTEG_STEPSIZE_1OBS     900
//20151123

#define MAX_SV_PRN              32
#define OBS_DAYS                3
#define PRED_DAYS               6
#define MAX_OBS_PER_DAY         12
#define SAMPLE_INTERVAL         900
#define MAX_OBS_NUM             (MAX_OBS_PER_DAY * OBS_DAYS)
#define MAX_EPOCH_NUM           ((int)((OBS_DAYS+PRED_DAYS)/(INTEG_STEPSIZE*SEC2DAY))+1)

#define SAMPLE_PER_OBS          5
#define MIN_OBS_SAMPLE_OFFSET   (4*3600)
/////////////////////////////////////////////////////////////////////////////////////////

//2016.1.5 add by hongxin.liu for the union of cartesian and kepler
#define ORBIT_CARTESIAN			0		// Cartesian form for predicted Eph 
#define ORBIT_KEPLERIAN			1		// Keplerian form for predicted Eph	
#define PRED_KEPL_STEPSIZE      14400   //4 hour
//2016.1.5

/////////////////////////////////////////////////////////////////////////////////////////
// Error & warning message constants
#define XEOD_CG_SUCCESS         0
#define XEOD_CG_NOT_INITED      1
#define XEOD_CG_ERR_MEM         2
#define XEOD_CG_ERR_PARAM       3
#define XEOD_CG_ERR_OBS         4
#define XEOD_CG_ERR_EST         5
//20151222 add by hongxin.liu->for the output of orbit parameters from the requirement of firmware team 
//20151222
//20151222 add by hongxin.liu->for the output of orbit parameters from the requirement of firmware team 
#define XEOD_CG_ERR_KEPLER_FIT  6
#define XEOD_CG_ERR_KEPLER_FIT_INVALID_PARAM  7
//20151222
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
// General constants
#define PI2                     6.283185307179586476925286766559
#define RAD2ARCS                2.062648062470964e+005            //Radian to Arcseconds
#define ARCS2RAD                4.848136811095355e-006            //Arcsecond to Radian
#define DEG2RAD                 0.01745329251994                  //Degree to Radian
#define RAD2DEG                 57.29577951308232                 //Rad to Degree
#define SEC2DAY                 1.1574074074074073e-005           //Second to Day
#define DAY2SEC                 86400.0                           //Day to Second
#define WEEK2SEC                604800                            //Week to Second
//20151120 add by hongxin.liu for 1Obs ref_sys algorithm
#define REFJD                   240000.5
#define GM_EARTH_SQRT           1.996498184321740e+7              //square root of GM_EARTH
#define WGSRelCorrConst         -4.442807633E-10                  //F - relativisdtic correction term constant
//20151120

#define MJD_J2000               51544.5
#define MJD_GPST0               44244

#define GM_EARTH                0.3986004415E+15    // Gravity Constant * Earth mass, unit: m^3/s^2
#define GM_MOON                 4.9027989E+12       // Gravity Constant * Lunar mass, unit: m^3/s^2 4902.799
#define GM_SUN                  1.3271244E+20       // Gravity Constant * Solar mass, unit: m^3/s^2                        
#define RADIUS_SUN              696000000.0         //The potent radius of sun , unit:m
#define RADIUS_MOON             1738000.0           //The potent radius of moon, unit:m
#define AUNIT                   1.49597870691E+11   // Astronomy Unit: meters
#define AUNIT2                  2.2379522915281E+022// Astronomy Unit^2: meter^2
#define AE                      6378136.3           // EQUATORIAL RADIUS OF EARTH  in m 6378137.0
#define CLIGHT                  299792458.0         //speed of light, unit:m/s
#define WGS84_OMEGDOTE          7.2921151467E-5
#define WGS_SQRT_GM 19964981.8432173887
#define WGS_F_GTR -4.442807633e-10
#define DAY2SEC 86400.0
/////////////////////////////////////////////////////////////////////////////////////////
/*
// enum and structures
enum IntDirection
{
  kIntBackward=-1,
  kIntForward=1
};

enum TimeSystem
{
  kTimeSysTT=0,
  kTimeSysTAI,
  kTimeSysGPST,
  kTimeSysUTC,
  kTimeSysUT1
};
*/
typedef double DOUBLE;
#ifndef NULL
#define NULL 0
#endif
#pragma pack(push,4)

typedef struct tagXEOD_PARAM
{
  unsigned int mode;                      /* prediction mode, now only support 0, which is client generated */
  unsigned int max_obs_days;              /* max observation days that is used in prediction, now only support 3 days */
  unsigned int max_pred_days;             /* max prediction days to be estimated, now only support 6 days */
  unsigned int sample_interval;           /* sample interval for the obs, unit in second, now only support 900 */
  unsigned int step_size;                 /* step size that is used in prediction, unit in second, now only support 900 */
} XEOD_PARAM;

/*
typedef struct tagSatObs
{
  double    delta_mjd_tt;
  double    rv[6];
  double    clk;
  double    clkdot;

  unsigned long aodc;
  unsigned int  aoec;
  unsigned char fit_interval;
  unsigned char iode2;
  unsigned char iode3;
} SatObs;

typedef struct tagSatMass
{
  int       block_id;
  double    mass;
} SatMass;

typedef struct tagSatParam
{
  int    svid;
  int    mjd;

  double delta_mjd_tt;
  double rv[6];
  double rad[9];
  double af[3];
} SatParam;

typedef struct tagSatXYZT
{
  double delta_mjd;
  double x;
  double y;
  double z;
  double vx;
  double vy;
  double vz;
  double x_crs;
  double y_crs;
  double z_crs;
  double vx_crs;
  double vy_crs;
  double vz_crs;
  double week_sec;
  unsigned short week_no;
} SatXYZT;


typedef struct tagEcefPosition
{
  double x;
  double y;
  double z;
  unsigned char available;
} EcefPosition;
*/
//mjd0 + dmjd_end (days) < 1/12(2hour)
typedef struct tagXeodFileHeader
{
  int eph_valid[MAX_SV_PRN];
  int mjd0;
  double dmjd_start[MAX_SV_PRN];
  double dmjd_end[MAX_SV_PRN];
  int leap_sec;
  char reserved[1024-648];  // In case further fields will be added, 648 is the total size of the former parameters
} XeodFileHeader;

typedef struct tagXeodFileTail
{
  char           label[8];                  // "raw.obs" or "ext.eph"
  unsigned int   file_version;              // file version number
  unsigned int   check_sum;
} XeodFileTail;

#ifndef _spreadOrbitDef_
#define _spreadOrbitDef_
typedef struct tagEphKepler
{
  short          svid;
  unsigned short week_no;
  unsigned short acc;

  unsigned char  iode2;
  unsigned char  iode3;
  double         M0;
  double         deltaN;
  double         ecc;
  double         Ek;
  double         sqrtA;
  double         OMEGA0;
  double         i0;
  double         w;
  double         OMEGADOT;
  double         IDOT;
  double         Cuc;
  double         Cus;
  double         Crc;
  double         Crs;
  double         Cic;
  double         Cis;

  double         group_delay;
  double         af0;
  double         af1;
  double         af2;
  unsigned int  aodc;      // LTE64BIT  unsigned long
  int            health;
  unsigned int  toc;    // LTE64BIT  unsigned long
  unsigned int  toe;   // LTE64BIT   unsigned long

  unsigned char  available;
  unsigned char  repeat;
  unsigned char  update;
  unsigned char  fit_interval;
} EphKepler;

/* position parameters to kepler parameters data structure */
typedef struct tagKepler_Ephemeris
{
   double     af0;
   double    m0; 
   double    e; 
   double    sqrta; 
   double    omega0;
   double    i0;
   double    omega;
   double    omegaDot; 
   double     tgd;  
   double     af2;
   double     af1; 
   double     Crs;
   double     deltan; 
   double     Cuc; 
   double     Cus;
   double     Cic;  
   double     Cis; 
   double     Crc; 
   double     idot;
   unsigned int       toc; //LTE64BIT long int
   unsigned int       toe; //LTE64BIT long int 
   unsigned int      wnZCount;//LTE64BIT unsigned long int 
   unsigned int      iodc; //LTE64BIT unsigned short int 
   unsigned short    weekNo; 
   signed char        fitint; 
   signed char        accuracy;   
   unsigned char       svid;   
   unsigned char       age:4;   
   signed char        status:4; 
} tSVD_Kepler_Ephemeris;
#endif
typedef struct aliEph
{
    unsigned char	codeL2;
    unsigned char	L2Pdata;
    unsigned char	udre;// unkonw, may ura, need to confirm
    unsigned char	IODC;
    double	n;
    double	r1me2;// unkonw,  need to confirm
    double	OMEGA_n;// unkonw, need to confirm
    double	ODOT_n;// unkonw, need to confirm
}AliEph;

typedef struct ephextent
{
    EphKepler ekepler;
    AliEph alieph;
}EphExt;

typedef struct tagEphCartesian
{
  int           mjd0;
  double        delta_mjd;
  double        rv[6];
  double        clk; 
  unsigned int  quality;
} EphCartesian;

typedef struct tagObsBlock
{
  unsigned int  sv_id;
  unsigned int  num_of_obs;
  EphKepler*    eph_kep;
} ObsBlock;

typedef struct tagEphBlock
{
  unsigned int       sv_id;
  int                mjd0;
  double             dmjd0;
  unsigned int       obs_num;
  double             obs_span;
  double             pos_rms;
  double             clk_rms;
//20160105 add by hongxin.liu for the union of cartesian and kepler
//  int	OrbForm;				//ORBIT_CARTESIAN/ORBIT_KEPLERIAN
//  union
//  {
	  tSVD_Kepler_Ephemeris*	eph_kepl;
	  EphCartesian*				eph_cart;
//  };
//20160105
} EphBlock;
#ifndef _gps_ephemeris_
#define _gps_ephemeris_
typedef struct // GPS ephemeris, also used by BD2                           
{
         unsigned short         iodc;
         unsigned char  iode2;
         unsigned char  iode3;

         unsigned char  ura;
         unsigned char  flag; // bit0 means ephemeris valid, bit1 means ephemeris verified, bit5 means pre_ephemeris 
         unsigned char  health;
         unsigned char  svid;

         int    toe;
         int    toc;
         int    week;

         // variables decoded from stream data
         DOUBLE M0;                      // Mean Anomaly at Reference Time
         DOUBLE delta_n;              // Mean Motion Difference from Computed Value
         DOUBLE ecc;                      // Eccentricity
         DOUBLE sqrtA;                  // Square Root of the Semi-Major Axis
         DOUBLE omega0;             // Longitude of Ascending Node of Orbit Plane at Weekly Epoch
         DOUBLE i0;                         // Inclination Angle at Reference Time
         DOUBLE w;                         // Argument of Perigee
         DOUBLE omega_dot;       // Rate of Right Ascension
         DOUBLE idot;            // Rate of Inclination Angle
         DOUBLE cuc;                      // Amplitude of the Cosine Harmonic Correction Term to the Argument of Latitude
         DOUBLE cus;                      // Amplitude of the Sine Harmonic Correction Term to the Argument of Latitude
         DOUBLE crc;                       // Amplitude of the Cosine Harmonic Correction Term to the Orbit Radius
         DOUBLE crs;                       // Amplitude of the Sine Harmonic Correction Term to the Orbit Radius
         DOUBLE cic;                       // Amplitude of the Cosine Harmonic Correction Term to the Angle of Inclination
         DOUBLE cis;                       // Amplitude of the Sine Harmonic Correction Term to the Angle of Inclination
         DOUBLE tgd;                      // Group Delay
         DOUBLE tgd2;          // Group Delay for B2
         DOUBLE af0;                      // Satellite Clock Correction
         DOUBLE af1;                      // Satellite Clock Correction
         DOUBLE af2;                      // Satellite Clock Correction

         // variables derived from basic data, avoid calculate every time
         DOUBLE axis;            // Semi-major Axis of Orbit
         DOUBLE n;                          // Corrected Mean Angular Rate
         DOUBLE root_ecc;  // Square Root of One Minus Ecc Square
         DOUBLE omega_t;           // Longitude of Ascending Node of Orbit Plane at toe
         DOUBLE omega_delta;   // Delta Between omega_dot and WGS_OMEGDOTE
         DOUBLE Ek;                        // Ek, derived from Mk
} GPS_EPHEMERIS, *PGPS_EPHEMERIS;
#endif
typedef struct tagApEphBlock
{
	unsigned int       sv_id;
	int                mjd0;
	double             dmjd0;
	unsigned int       obs_num;
	double             obs_span;
	double             pos_rms;
	double             clk_rms;
}ApEphBlock;
typedef struct tagECEFPOSITION
{
  long double x;
  long double y;
  long double z;
  unsigned char available;
} ECEFPOSITION;
/*
typedef struct
{
	double x, y, z;
	double vx, vy, vz;
	double ve, vn, vu, Speed, Course, Elev;
	double ClkDrifting;

} TKINEMATIC_INFO, *PTKINEMATIC_INFO;

typedef struct 
{
	unsigned char svid;
	unsigned char PredictQuality;
	unsigned short SatInfoFlag;
	double IonoCorr,TropCorr;
	double ClockCorr;
	double PsrCorr;
	double PsrPredict,TransmitTime;
	int Doppler,CodePhase;
	float e1,az;
	TKINEMATIC_INFO PosVel;
	double ReceiverTime;
	int WeekNumber;
	unsigned int BlockCount;
	unsigned int bitNumber;
	unsigned int timeStamp;
	double CalcTimeLast;
}SATELLITE_INFO,*PSATELLITE_INFO;


*/
/*
typedef struct {
unsigned short iodc;
unsigned char iode2;
unsigned char iode3;
unsigned char ura;
unsigned char flag;
unsigned char health;
unsigned char svid;
int toe;
int toc;
int week;
double M0,delta_n,ecc,sqrtA,omega0,i0,w,omega_dot;
double idot,cuc,cus,crc,crs,cic,cis,tgd,tgd2,af0;
double af1,af2,axis,n,root_ecc,omega_t,omega_delta,Ek;
}GPS_EPHEMERIS,*PGPS_EPHEMERIS;
*/
#pragma pack(pop)

#ifdef __cplusplus
extern "C"
{
#endif

DLL_API_INT XEOD_Init(const XeodFileHeader* pobs_header, const XEOD_PARAM* pParams, const char* pwork_path);
DLL_API_INT XEOD_GeneratePredEph(EphBlock *peph_block, const ObsBlock *pobs_block, unsigned int step_size, unsigned int orbit_form);
DLL_API_INT XEOD_Deinit();

//20151222 add by hongxin.liu->for the output of orbit parameters from the requirement of firmware team 
DLL_API_INT XEOD_GetKeplerEEBlock(EphBlock *peph_block);//, BYTE *pEEBlock, short *pWNBlock
//20151222

#ifdef __cplusplus
}
#endif

#endif //__XEOD_CG_API_H
