#include <sys/epoll.h>
#include <sys/socket.h>
#include <cutils/sockets.h>
#include <errno.h>
#include <math.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "gps_cmd.h"
#include "gps_common.h"
#include "gps_engine.h"
#include "xeod_cg_api.h"

extern void get_type_id(uint32_t flags);
extern void get_ri_locationid(void);
extern int gps_adingData(TCmdData_t*pPak);
extern int SF_getSvPredictPosVel( int svid,  int week,  int tow, char *output);

extern int gPosMethod;
extern CgPositionParam_t gLocationInfo[1];

static PVTMISC_FLASH pvttype;
static CgPositionParam_t msaLocationInfo[1];
static GNSSPositionParam_t geopos;
static unsigned char isFirstTime = 1;
static GpsNavigationMessage navmsg;
#ifdef GNSS_ANDROIDN
static GnssNavigationMessage gnss_navmsg;
#endif //GNSS_ANDROIDN 

GpsNiNotification gnotification[1];
char socket_status = thread_init; 
int imsi_status = 0;
int cid_status = 0;
static  TCgSetQoP_t Qop;
//================= msa data from ge2 device =============== //
typedef struct _Velocity
{
    DOUBLE  horizontalSpeed;   // kmh
    DOUBLE  bearing;
    DOUBLE  verticalSpeed;     // kmh
    int     verticalDirect;    // 0: upward, 1:downward
}Velocity;

typedef struct __AgpsMsa_t
{
	TAgpsMsrSetList_t msr;
	Velocity vel;
}MsaData_t;

typedef struct __AgpsMsa_rawt
{
	TAgpsMsrSetList_raw_t msr;
	Velocity vel;
}MsaData_raw_t;

static TcgVelocity eVelocity ;
static MsaData_t msadata;
static MsaData_raw_t msadataraw;
GNSSReqAssData_t AssistData;
static GNSSMsrSetList_t GnssMsrlist;
static GNSSVelocity_t EGnssVel;
GpsData gps_msr;
#ifdef GNSS_ANDROIDN
GnssData gnss_msr;
#endif //GNSS_ANDROIDN 
void set_vel_value(void)
{
	eVelocity.horizontalSpeed = msadata.vel.horizontalSpeed;
	eVelocity.bearing = msadata.vel.bearing;
	eVelocity.verticalSpeed = msadata.vel.verticalSpeed;
	eVelocity.verticalDirect = msadata.vel.verticalDirect;	
	return;
}

void transform_gps_measurement(MsaData_t *pmsa)
{
    int i = 0,j=0,len;
	static FILE *msafp = NULL;
    if(pmsa == NULL)
    {
        E("%s: measurement data is null",__func__);
        return;
    }
	if(msafp == NULL)
	{			
		if(access("/data/gnss/nmea.cg",0) != -1)
		{
			msafp = fopen("/data/gnss/alimsr.txt","rb+");
		}				
		else
		{
			msafp = fopen("/data/gnss/alimsr.txt","wb+");
		}
		//msafp = fopen("/data/gnss/alimsr.txt","w+");
	}
	else
	{
		struct stat temp;
		stat("/data/gnss/alimsr.txt", &temp);
		if(temp.st_size >= 20000000)
		{
		    fclose(msafp);
			D("save msr of ali is too large");
			msafp = fopen("/data/gnss/alimsr.txt","wb+");
		}
	}
    gps_msr.size = sizeof(GpsData);
    len = gps_msr.measurement_count = pmsa->msr.listNums;

    for(i  = 0; i < len; i++)
    {
		if(msadata.msr.msrElement[i].satelliteID > GPS_MAX_MEASUREMENT)
		{	
			if(gps_msr.measurement_count > 0)
			gps_msr.measurement_count--;
			continue;
		}
		if(j >= GPS_MAX_MEASUREMENT)
		{
			E("gps msr numlist over or equal %d",GPS_MAX_MEASUREMENT);
			return;
		}
        gps_msr.measurements[j].size = sizeof(GpsMeasurement);
        gps_msr.measurements[j].flags = 0; //0xff;
        gps_msr.measurements[j].prn = msadata.msr.msrElement[i].satelliteID;
        gps_msr.measurements[j].time_offset_ns = (msadata.msr.msrElement[i].fracChips / 1024 + msadata.msr.msrElement[i].wholeChips) / 1023;
        gps_msr.measurements[j].state = GPS_MEASUREMENT_STATE_CODE_LOCK | GPS_MEASUREMENT_STATE_BIT_SYNC|GPS_MEASUREMENT_STATE_SUBFRAME_SYNC|GPS_MEASUREMENT_STATE_TOW_DECODED;
        gps_msr.measurements[j].received_gps_tow_ns = msadata.msr.msrElement[i].received_gps_tow_ns;
        gps_msr.measurements[j].received_gps_tow_uncertainty_ns = msadata.msr.msrElement[i].received_gps_tow_uncertainty_ns;
        gps_msr.measurements[j].c_n0_dbhz = (double)msadata.msr.msrElement[i].cNo;
        gps_msr.measurements[j].pseudorange_rate_mps = msadata.msr.msrElement[i].doppler*0.19029;
        gps_msr.measurements[j].pseudorange_rate_uncertainty_mps = 0.5;  //for test
        gps_msr.measurements[j].accumulated_delta_range_state = GPS_ADR_STATE_UNKNOWN;
        gps_msr.measurements[j].accumulated_delta_range_m = 0.5;
        gps_msr.measurements[j].accumulated_delta_range_uncertainty_m = 0.6;
        gps_msr.measurements[j].pseudorange_m = msadata.msr.msrElement[i].pseudorange_m;
        gps_msr.measurements[j].pseudorange_uncertainty_m = msadata.msr.msrElement[i].pseudorange_uncertainty_m;
        gps_msr.measurements[j].code_phase_chips = msadata.msr.msrElement[i].fracChips / 1024 + msadata.msr.msrElement[i].wholeChips;
        gps_msr.measurements[j].code_phase_uncertainty_chips = 0.5;
        gps_msr.measurements[j].carrier_frequency_hz = msadata.msr.msrElement[i].carrier_frequency_hz;
        gps_msr.measurements[j].carrier_cycles = msadata.msr.msrElement[i].carrier_count;
        gps_msr.measurements[j].carrier_phase = msadata.msr.msrElement[i].carrier_phase;
        gps_msr.measurements[j].carrier_phase_uncertainty = 0.5;
        gps_msr.measurements[j].loss_of_lock = GPS_LOSS_OF_LOCK_OK;
        gps_msr.measurements[j].bit_number = msadata.msr.msrElement[i].bit_number;
        gps_msr.measurements[j].time_from_last_bit_ms = msadata.msr.msrElement[i].time_from_last_bit_ms;
        gps_msr.measurements[j].doppler_shift_hz = msadata.msr.msrElement[i].doppler;
        gps_msr.measurements[j].doppler_shift_uncertainty_hz = 0.5;
        gps_msr.measurements[j].multipath_indicator = msadata.msr.msrElement[i].mpathInd;
        gps_msr.measurements[j].snr_db = msadata.msr.msrElement[i].cNo - 17;  //gps
        gps_msr.measurements[j].elevation_deg = msadata.msr.msrElement[i].elevation_deg;
        gps_msr.measurements[j].elevation_uncertainty_deg = msadata.msr.msrElement[i].elevation_uncertainty_deg;
        gps_msr.measurements[j].azimuth_deg = msadata.msr.msrElement[i].azimuth_deg;
        gps_msr.measurements[j].azimuth_uncertainty_deg = msadata.msr.msrElement[i].azimuth_uncertainty_deg;
        gps_msr.measurements[j].used_in_fix = (bool)msadata.msr.msrElement[i].used_in_fix; //maybe int to char

		j++;
        if(msafp == NULL)
		{
			D("msafp is null,just return");
			return;
		}
#if __WORDSIZE == 64
		fprintf(msafp,"size:%zx\rflags:%d\rprn:%d\rtime_offset_ns:%lf\rstate:%d\rreceived_gps_tow_ns:%ld\n",
		gps_msr.measurements[i].size,gps_msr.measurements[i].flags,gps_msr.measurements[i].prn,
		gps_msr.measurements[i].time_offset_ns,gps_msr.measurements[i].state,gps_msr.measurements[i].received_gps_tow_ns);
fprintf(msafp,"received_gps_tow_uncertainty_ns:%ld\rc_n0_dbhz:%lf\rpseudorange_rate_mps:%lf\rpseudorange_rate_uncertainty_mps:0\n",gps_msr.measurements[i].received_gps_tow_uncertainty_ns,gps_msr.measurements[i].c_n0_dbhz,gps_msr.measurements[i].pseudorange_rate_mps);
fprintf(msafp,"code_phase_chips:%lf\rcode_phase_uncertainty_chips:0\rcarrier_frequency_hz:%f\rcarrier_cycles:%ld\rcarrier_phase:%lf\rcarrier_phase_uncertainty:0\rloss_of_lock:%d\rbit_number:%d\rtime_from_last_bit_ms:%d\n",\
gps_msr.measurements[i].code_phase_chips,gps_msr.measurements[i].carrier_frequency_hz,gps_msr.measurements[i].carrier_cycles,gps_msr.measurements[i].carrier_phase,
gps_msr.measurements[i].loss_of_lock,gps_msr.measurements[i].bit_number,gps_msr.measurements[i].time_from_last_bit_ms);
#else
fprintf(msafp,"size:%zx\rflags:%d\rprn:%d\rtime_offset_ns:%lf\rstate:%d\rreceived_gps_tow_ns:%lld\n",
gps_msr.measurements[i].size,gps_msr.measurements[i].flags,gps_msr.measurements[i].prn,
gps_msr.measurements[i].time_offset_ns,gps_msr.measurements[i].state,gps_msr.measurements[i].received_gps_tow_ns);
fprintf(msafp,"received_gps_tow_uncertainty_ns:%lld\rc_n0_dbhz:%lf\rpseudorange_rate_mps:%lf\rpseudorange_rate_uncertainty_mps:0\n",gps_msr.measurements[i].received_gps_tow_uncertainty_ns,gps_msr.measurements[i].c_n0_dbhz,gps_msr.measurements[i].pseudorange_rate_mps);
fprintf(msafp,"code_phase_chips:%lf\rcode_phase_uncertainty_chips:0\rcarrier_frequency_hz:%f\rcarrier_cycles:%lld\rcarrier_phase:%lf\rcarrier_phase_uncertainty:0\rloss_of_lock:%d\rbit_number:%d\rtime_from_last_bit_ms:%d\n",
gps_msr.measurements[i].code_phase_chips,gps_msr.measurements[i].carrier_frequency_hz,gps_msr.measurements[i].carrier_cycles,gps_msr.measurements[i].carrier_phase,
gps_msr.measurements[i].loss_of_lock,gps_msr.measurements[i].bit_number,gps_msr.measurements[i].time_from_last_bit_ms);
#endif
		fprintf(msafp,"accumulated_delta_range_state:%d\raccumulated_delta_range_m:0\raccumulated_delta_range_uncertainty_m:0\rpseudorange_m:%lf\rpseudorange_uncertainty_m:%lf\n",gps_msr.measurements[i].accumulated_delta_range_state,
		gps_msr.measurements[i].pseudorange_m,gps_msr.measurements[i].pseudorange_uncertainty_m);
		fprintf(msafp,"doppler_shift_hz:%lf\rdoppler_shift_uncertainty_hz:0\rmultipath_indicator:%d\rsnr_db:%lf\relevation_deg:%lf\relevation_uncertainty_deg:%lf\razimuth_deg:%lf\razimuth_uncertainty_deg:%lf\rused_in_fix:%d\r\n",
		gps_msr.measurements[i].doppler_shift_hz,gps_msr.measurements[i].multipath_indicator,gps_msr.measurements[i].snr_db,
		gps_msr.measurements[i].elevation_deg,gps_msr.measurements[i].elevation_uncertainty_deg,
		gps_msr.measurements[i].azimuth_deg,gps_msr.measurements[i].azimuth_uncertainty_deg,gps_msr.measurements[i].used_in_fix);
		fprintf(msafp,"\n");
    }
}

#ifdef GNSS_ANDROIDN

void transform_gnss_measurement(MsaData_t *pmsa)
{
    int i = 0,len;

    if(pmsa == NULL)
    {
        E("%s: measurement data is null",__func__);
        return;
    }

    gnss_msr.size = sizeof(GnssData);
    gnss_msr.measurement_count = pmsa->msr.listNums;
	len = pmsa->msr.listNums;

    for(i  = 0; i < len; i++)
    {
        gnss_msr.measurements[i].size = sizeof(GnssMeasurement);
        gnss_msr.measurements[i].flags = GNSS_MEASUREMENT_HAS_SNR \
			                            |GNSS_MEASUREMENT_HAS_CARRIER_FREQUENCY \
			                            |GNSS_MEASUREMENT_HAS_CARRIER_CYCLES \
			                            |GNSS_MEASUREMENT_HAS_CARRIER_PHASE \
			                            |GNSS_MEASUREMENT_HAS_CARRIER_PHASE_UNCERTAINTY;
        gnss_msr.measurements[i].svid = msadata.msr.msrElement[i].satelliteID;
        gnss_msr.measurements[i].constellation = GNSS_CONSTELLATION_GPS;
        gnss_msr.measurements[i].time_offset_ns = (msadata.msr.msrElement[i].fracChips / 1024 + msadata.msr.msrElement[i].wholeChips) / 1023;
        gnss_msr.measurements[i].state = GNSS_MEASUREMENT_STATE_CODE_LOCK;
        gnss_msr.measurements[i].received_sv_time_in_ns = msadata.msr.msrElement[i].received_gps_tow_ns;   // gaoyun
        gnss_msr.measurements[i].received_sv_time_uncertainty_in_ns = msadata.msr.msrElement[i].received_gps_tow_uncertainty_ns;
        gnss_msr.measurements[i].c_n0_dbhz = (double)msadata.msr.msrElement[i].cNo;
        gnss_msr.measurements[i].pseudorange_rate_mps = msadata.msr.msrElement[i].doppler/5.25;
        gnss_msr.measurements[i].pseudorange_rate_uncertainty_mps = 0;
        gnss_msr.measurements[i].accumulated_delta_range_state = GNSS_ADR_STATE_RESET;
        gnss_msr.measurements[i].accumulated_delta_range_m = 0;
        gnss_msr.measurements[i].accumulated_delta_range_uncertainty_m = 0;
        gnss_msr.measurements[i].carrier_frequency_hz = msadata.msr.msrElement[i].carrier_frequency_hz;
        gnss_msr.measurements[i].carrier_cycles = 0;
        gnss_msr.measurements[i].carrier_phase = msadata.msr.msrElement[i].carrier_phase;
        gnss_msr.measurements[i].carrier_phase_uncertainty = 0;
        gnss_msr.measurements[i].multipath_indicator = msadata.msr.msrElement[i].mpathInd;
        gnss_msr.measurements[i].snr_db = msadata.msr.msrElement[i].cNo - 17;
    }
}

#endif //GNSS_ANDROIDN 

int set_raw_msrdata(const unsigned char *buf, unsigned int len)
{
    int i,j=0,n=0,b=0;
    memset(&eVelocity, 0, sizeof(TcgVelocity));
	memset(&msadataraw, 0, sizeof(MsaData_raw_t));
	memset(&GnssMsrlist,0,sizeof(GNSSMsrSetList_t));
	memcpy(&msadataraw,buf,len);
    D("=========>>>>>>>GPS_MEASUREMENT listnum:%d,gpstow:0x%d\n", msadataraw.msr.listNums,msadataraw.msr.gpsTOW);
    //trans msr to agps struct
    if(msadataraw.msr.listNums > 14)
    {
        msadataraw.msr.listNums = 14;
    }
    D("listnum is %d",msadataraw.msr.listNums);
    GnssMsrlist.gpsMrsList.gpsTOW = msadataraw.msr.gpsTOW;
    GnssMsrlist.gpsMrsList.listNums = msadataraw.msr.listNums;
    for(i = 0; i < msadataraw.msr.listNums; i++)
    {
        D("measure sid is %d,systemid %d",msadataraw.msr.msrElement[i].satelliteID,msadataraw.msr.msrElement[i].systemid);
        if(msadataraw.msr.msrElement[i].systemid == 0)
        {
            GnssMsrlist.gpsMrsList.msrElement[j].satelliteID = msadataraw.msr.msrElement[i].satelliteID;
            GnssMsrlist.gpsMrsList.msrElement[j].cNo = msadataraw.msr.msrElement[i].cNo;
            GnssMsrlist.gpsMrsList.msrElement[j].doppler = msadataraw.msr.msrElement[i].doppler;
            GnssMsrlist.gpsMrsList.msrElement[j].wholeChips = msadataraw.msr.msrElement[i].wholeChips;
            GnssMsrlist.gpsMrsList.msrElement[j].fracChips = msadataraw.msr.msrElement[i].fracChips;
            GnssMsrlist.gpsMrsList.msrElement[j].mpathInd = msadataraw.msr.msrElement[i].mpathInd;
            GnssMsrlist.gpsMrsList.msrElement[j].pseuRangeRMSErr = msadataraw.msr.msrElement[i].pseuRangeRMSErr; 
            j++;
        } 
        if(msadataraw.msr.msrElement[i].systemid == 1)   //glonass
        {
            GnssMsrlist.gnssNum = 1;

            GnssMsrlist.mrsList[0].gnssID = SUPL_GNSS_ID_GLONASS;
            GnssMsrlist.mrsList[0].signalTypeNum = 1;
            GnssMsrlist.mrsList[0].genericMrsList[0].satMsrNum++;
            GnssMsrlist.mrsList[0].genericMrsList[0].msrPara[n].svID = msadataraw.msr.msrElement[i].satelliteID;
            GnssMsrlist.mrsList[0].genericMrsList[0].msrPara[n].cNo = msadataraw.msr.msrElement[i].cNo;
            GnssMsrlist.mrsList[0].genericMrsList[0].msrPara[n].doppler = msadataraw.msr.msrElement[i].doppler;
            GnssMsrlist.mrsList[0].genericMrsList[0].msrPara[n].codePhase = 
            ((float)msadataraw.msr.msrElement[i].wholeChips + (float)msadataraw.msr.msrElement[i].fracChips/1024.0)/511.0;
            GnssMsrlist.mrsList[0].genericMrsList[0].msrPara[n].mpathInd = msadataraw.msr.msrElement[i].mpathInd;
            GnssMsrlist.mrsList[0].genericMrsList[0].msrPara[n].codePhaseRMSErr = msadataraw.msr.msrElement[i].pseuRangeRMSErr; 
            n++;
        } 
        if(msadataraw.msr.msrElement[i].systemid == 2)  //beidou
        {
            GnssMsrlist.gnssNum = 1;
            GnssMsrlist.mrsList[0].gnssID = SUPL_GNSS_ID_BDS;
            GnssMsrlist.mrsList[0].signalTypeNum = 1;
            GnssMsrlist.mrsList[0].genericMrsList[0].satMsrNum++;
            GnssMsrlist.mrsList[0].genericMrsList[0].msrPara[b].svID = msadataraw.msr.msrElement[i].satelliteID;
            GnssMsrlist.mrsList[0].genericMrsList[0].msrPara[b].cNo = msadataraw.msr.msrElement[i].cNo;
            GnssMsrlist.mrsList[0].genericMrsList[0].msrPara[b].doppler = msadataraw.msr.msrElement[i].doppler;
            GnssMsrlist.mrsList[0].genericMrsList[0].msrPara[b].codePhase = 
            ((float)msadataraw.msr.msrElement[i].wholeChips + (float)msadataraw.msr.msrElement[i].fracChips/1024.0)/2046.0;
            GnssMsrlist.mrsList[0].genericMrsList[0].msrPara[b].mpathInd = msadataraw.msr.msrElement[i].mpathInd;
            GnssMsrlist.mrsList[0].genericMrsList[0].msrPara[b].codePhaseRMSErr = msadataraw.msr.msrElement[i].pseuRangeRMSErr;
            b++;
        }  
    }
    if(GnssMsrlist.mrsList[0].genericMrsList[0].satMsrNum > 0)
    {
        GnssMsrlist.gpsMrsList.listNums = msadataraw.msr.listNums - GnssMsrlist.mrsList[0].genericMrsList[0].satMsrNum;
        D("after msr set,gpslistnum is %d,gnssnum is %d",GnssMsrlist.gpsMrsList.listNums,GnssMsrlist.mrsList[0].genericMrsList[0].satMsrNum);
    }

    for(i = 0;i < msadataraw.msr.listNums; i++)
    {
        D("satelliteID=%d,cno=%d,doppler=%f,fracChips=%d,pseuRangeRMSErr=%d,hspeed=%f"
          ,msadataraw.msr.msrElement[i].satelliteID
          ,msadataraw.msr.msrElement[i].cNo,msadataraw.msr.msrElement[i].doppler
          ,msadataraw.msr.msrElement[i].fracChips,msadataraw.msr.msrElement[i].pseuRangeRMSErr
          ,msadataraw.vel.horizontalSpeed);
    }
    eVelocity.horizontalSpeed = msadataraw.vel.horizontalSpeed;
	eVelocity.bearing = msadataraw.vel.bearing;
	eVelocity.verticalSpeed = msadataraw.vel.verticalSpeed;
	eVelocity.verticalDirect = msadataraw.vel.verticalDirect;	
    return 0;

}

int set_measurement_data(const unsigned char *buf, unsigned int len)
{ 
    int i,j=0,n=0,b=0;
    if(len != (sizeof(MsaData_t)))
	{
		E("msa lenth not equ needed,len read is %ud,need is %d",len,(int)sizeof(MsaData_t));
		if(len == (sizeof(MsaData_raw_t)))
		{
		    D("firmware msa data is raw structure");
		    set_raw_msrdata(buf,len);
			return 0;
		}
		return 1;
	}
    memset(&eVelocity, 0, sizeof(TcgVelocity));
	memset(&msadata, 0, sizeof(MsaData_t));
	memset(&GnssMsrlist,0,sizeof(GNSSMsrSetList_t));
	memcpy(&msadata,buf,len);

    transform_gps_measurement(&msadata);
#ifdef GNSS_ANDROIDN
    transform_gnss_measurement(&msadata);
#endif //GNSS_ANDROIDN 
    D("=========>>>>>>>GPS_MEASUREMENT listnum:%d,gpstow:0x%d\n", 
      msadata.msr.listNums,msadata.msr.gpsTOW);
    //trans msr to agps struct
    if(msadata.msr.listNums > 14)
    {
        msadata.msr.listNums = 14;
    }
    D("listnum is %d",msadata.msr.listNums);
    GnssMsrlist.gpsMrsList.gpsTOW = msadata.msr.gpsTOW;
    GnssMsrlist.gpsMrsList.listNums = msadata.msr.listNums;
    for(i = 0; i < msadata.msr.listNums; i++)
    {
    
        D("measure sid is %d",msadata.msr.msrElement[i].satelliteID);
        if(msadata.msr.msrElement[i].systemid == 0)
        {
            GnssMsrlist.gpsMrsList.msrElement[j].satelliteID = msadata.msr.msrElement[i].satelliteID;
            GnssMsrlist.gpsMrsList.msrElement[j].cNo = msadata.msr.msrElement[i].cNo;
            GnssMsrlist.gpsMrsList.msrElement[j].doppler = msadata.msr.msrElement[i].doppler;
            GnssMsrlist.gpsMrsList.msrElement[j].wholeChips = msadata.msr.msrElement[i].wholeChips;
            GnssMsrlist.gpsMrsList.msrElement[j].fracChips = msadata.msr.msrElement[i].fracChips;
            GnssMsrlist.gpsMrsList.msrElement[j].mpathInd = msadata.msr.msrElement[i].mpathInd;
            GnssMsrlist.gpsMrsList.msrElement[j].pseuRangeRMSErr = msadata.msr.msrElement[i].pseuRangeRMSErr; 
            j++;
        } 
        
       
        if(msadata.msr.msrElement[i].systemid == 1)   //glonass
        {
            GnssMsrlist.gnssNum = 1;
            GnssMsrlist.mrsList[0].gnssID = SUPL_GNSS_ID_GLONASS;
            GnssMsrlist.mrsList[0].signalTypeNum = 1;
            GnssMsrlist.mrsList[0].genericMrsList[0].satMsrNum++;
            GnssMsrlist.mrsList[0].genericMrsList[0].msrPara[n].svID = msadata.msr.msrElement[i].satelliteID;
            GnssMsrlist.mrsList[0].genericMrsList[0].msrPara[n].cNo = msadata.msr.msrElement[i].cNo;
            GnssMsrlist.mrsList[0].genericMrsList[0].msrPara[n].doppler = msadata.msr.msrElement[i].doppler;
            GnssMsrlist.mrsList[0].genericMrsList[0].msrPara[n].codePhase = 
            ((float)msadata.msr.msrElement[i].wholeChips + (float)msadata.msr.msrElement[i].fracChips/1024.0)/511.0;
            GnssMsrlist.mrsList[0].genericMrsList[0].msrPara[n].mpathInd = msadata.msr.msrElement[i].mpathInd;
            GnssMsrlist.mrsList[0].genericMrsList[0].msrPara[n].codePhaseRMSErr = msadata.msr.msrElement[i].pseuRangeRMSErr; 
            n++;
        } 
        if(msadata.msr.msrElement[i].systemid == 2)  //beidou
        {
            GnssMsrlist.gnssNum = 1;
            GnssMsrlist.mrsList[0].gnssID = SUPL_GNSS_ID_BDS;
            GnssMsrlist.mrsList[0].signalTypeNum = 1;
            GnssMsrlist.mrsList[0].genericMrsList[0].satMsrNum++;
            GnssMsrlist.mrsList[0].genericMrsList[0].msrPara[b].svID = msadata.msr.msrElement[i].satelliteID;
            GnssMsrlist.mrsList[0].genericMrsList[0].msrPara[b].cNo = msadata.msr.msrElement[i].cNo;
            GnssMsrlist.mrsList[0].genericMrsList[0].msrPara[b].doppler = msadata.msr.msrElement[i].doppler;
            GnssMsrlist.mrsList[0].genericMrsList[0].msrPara[b].codePhase = 
            ((float)msadata.msr.msrElement[i].wholeChips + (float)msadata.msr.msrElement[i].fracChips/1024.0)/2046.0;
            GnssMsrlist.mrsList[0].genericMrsList[0].msrPara[b].mpathInd = msadata.msr.msrElement[i].mpathInd;
            GnssMsrlist.mrsList[0].genericMrsList[0].msrPara[b].codePhaseRMSErr = msadata.msr.msrElement[i].pseuRangeRMSErr;
            b++;
        } 
         
    }
    if(GnssMsrlist.mrsList[0].genericMrsList[0].satMsrNum > 0)
    {
        GnssMsrlist.gpsMrsList.listNums = msadata.msr.listNums - GnssMsrlist.mrsList[0].genericMrsList[0].satMsrNum;
        D("after msr set,gpslistnum is %d,gnssnum is %d",GnssMsrlist.gpsMrsList.listNums,GnssMsrlist.mrsList[0].genericMrsList[0].satMsrNum);
    }
    //memcpy(&GnssMsrlist.gpsMrsList,&msadata.msr,sizeof(TcgAgpsMsrSetList_t));

    for(i = 0;i < msadata.msr.listNums; i++)
    {
        D("satelliteID=%d,cno=%d,doppler=%f,fracChips=%d,pseuRangeRMSErr=%d,hspeed=%f"
          ,msadata.msr.msrElement[i].satelliteID
          ,msadata.msr.msrElement[i].cNo,msadata.msr.msrElement[i].doppler
          ,msadata.msr.msrElement[i].fracChips,msadata.msr.msrElement[i].pseuRangeRMSErr
          ,msadata.vel.horizontalSpeed);
    }
	set_vel_value();
    return 0;
}

int Get_Msr(TcgAgpsMsrSetList_t *msrlist,GNSSVelocity_t *vel)
{
	//send msr to gnss
	char type = 2;
	gps_sendData(SUPL_READY,&type);
	if((msadata.msr.gpsTOW != 0) || (msadataraw.msr.gpsTOW != 0))
	{
		memcpy(msrlist,&GnssMsrlist.gpsMrsList,sizeof(TcgAgpsMsrSetList_t));
        vel->velType = GNSSVelocityType_horizontalWithVerticalVelocityAndUncertainty;
        vel->choice.VelUncertainty.bearing = (long)eVelocity.bearing;
        vel->choice.VelUncertainty.horizontalSpeed = eVelocity.horizontalSpeed;
        vel->choice.VelUncertainty.verticalDirection = eVelocity.verticalDirect;
        vel->choice.VelUncertainty.verticalSpeed = (long)eVelocity.verticalSpeed;
        vel->choice.VelUncertainty.horizontalUncertaintySpeed = 0;
        vel->choice.VelUncertainty.verticalUncertaintySpeed = 0;
		
		return 0;
	}
	return 1;
}


int gnssGetMsr(GNSSMsrSetList_t *msrlist,GNSSVelocity_t *vel)
{
	//send msr to gnss
	char type = 2;
	gps_sendData(SUPL_READY,&type);
	if(msadata.msr.gpsTOW != 0)
	{
		memcpy(msrlist,&GnssMsrlist,sizeof(TcgAgpsMsrSetList_t));
        vel->velType = GNSSVelocityType_horizontalWithVerticalVelocityAndUncertainty;
        vel->choice.VelUncertainty.bearing = (long)eVelocity.bearing;
        vel->choice.VelUncertainty.horizontalSpeed = eVelocity.horizontalSpeed;
        vel->choice.VelUncertainty.verticalDirection = eVelocity.verticalDirect;
        vel->choice.VelUncertainty.verticalSpeed = (long)eVelocity.verticalSpeed;
        vel->choice.VelUncertainty.horizontalUncertaintySpeed = 0;
        vel->choice.VelUncertainty.verticalUncertaintySpeed = 0;
		
		return 0;
	}
	return 1;
}


void agps_thread_init(GpsState* s)
{
    s->asock[0] = -1;
    s->asock[1] = -1;
    if( socketpair( AF_LOCAL, SOCK_STREAM, 0, s->asock ) < 0 ) 
    {
        E("could not create thread asock socket pair: %s", strerror(errno));     
    }
    return;
}

void agps_thread_close(GpsState* s)
{
    if(s->asock[0] != -1) 
    {
        close(s->asock[0]);
        s->asock[0] = -1;
     }
    if(s->asock[1] != -1) 
    {
        close(s->asock[1]);
        s->asock[1] = -1;
     }
    return;
}
///////////just for test//////////

static void send_assist_ge2(GpsState* s)
{
	int i = 0,j = 0,k = 0;
	TCmdData_t Pak;
	BLOCK_EPHEMERIS_FLASH tempEph;
	FILE *flashfp;
    int week,toe;
    long diff = 0;
    D("%s:send initial data",__FUNCTION__);
    if(access("/data/gnss/FlashCfg.cg",0) == -1)
    {
        E("FlashCfg.cg is not exist");
        return;
    }
	flashfp = fopen("/data/gnss/FlashCfg.cg","rb");
	if(flashfp == NULL)
	{
		E("open flash file fail,%s",strerror(errno));
		return;
	}
//PVTSTAUS
	memset(&Pak,0,sizeof(TCmdData_t));
	memset(&tempEph,0,sizeof(BLOCK_EPHEMERIS_FLASH));

	D("send assist data to ge2");
	fread(&pvttype,sizeof(PVTMISC_FLASH),1,flashfp);
	Pak.type = GNSS_LIBGPS_AIDING_DATA_TYPE;
	Pak.subType = GNSS_LIBGPS_AIDING_RCVSTATUS_SUBTYPE;
	Pak.length = sizeof(PVTMISC_FLASH);
	D("location to ge2,lat=%f,lon=%f",pvttype.PvtPkg.ReceiverPos.lat,pvttype.PvtPkg.ReceiverPos.lon);
    memcpy(Pak.buff,&pvttype,sizeof(PVTMISC_FLASH));
	if((pvttype.PvtPkg.ReceiverPos.lat != 0) || (pvttype.PvtPkg.ReceiverPos.lon != 0))
	{
	    D("location is not null,send to gnss set");
		gps_adingData(&Pak);
	    usleep(10000);
    }
//GPSEPH
	memset(&Pak,0,sizeof(TCmdData_t));
	Pak.length = 4;   //send gps eph agps mode
	
	for(i = 0; i < 4; i++)
	{
		fseek(flashfp,4096*(i+GNSS_LIBGPS_FLASH_GPS_EPH1_SUBTYPE),SEEK_SET);
		fread(&tempEph,sizeof(BLOCK_EPHEMERIS_FLASH),1,flashfp);
		for(j = 0; j < 8; j++)
		{
			//D("said:flag-----%d,svid-----%d",tempEph.Ephemeris[j].flag,tempEph.Ephemeris[j].svid);
			if((tempEph.Ephemeris[j].flag & 0x1) && (pvttype.PvtPkg.week > 0))
			{
				week = GET_UBITS(tempEph.Ephemeris[j].word[7],20,10);
				toe = GET_UBITS(tempEph.Ephemeris[j].word[10],14,16);
				diff = (pvttype.PvtPkg.week - 1024 - week )*604800 + (pvttype.PvtPkg.msCount/1000 - toe*16 + 16);	
				//D("update,week is %d,toe is %d,diff is %ld",week,toe,diff);
				if((diff < -14400) || (diff > 14400))
				{
				    E("ephemeris is too old");
				    break;
				}

				memcpy(Pak.buff+Pak.length,&(tempEph.Ephemeris[j]),sizeof(GPS_EPHEMERIS_PACKED));
				Pak.length = Pak.length + sizeof(GPS_EPHEMERIS_PACKED);
				k++;
                if(k > 11)
                {
                    i = 5;
                    break;
                }
			}
		}
	}
    fseek(flashfp,4096*GNSS_LIBGPS_FLASH_QZS_EPH_SUBTYPE,SEEK_SET);
    fread(&tempEph,sizeof(BLOCK_EPHEMERIS_FLASH),1,flashfp);
    for(j = 0; j < 8; j++)
    {
        //D("said:flag-----%d,svid-----%d",tempEph.Ephemeris[j].flag,tempEph.Ephemeris[j].svid);
        if((tempEph.Ephemeris[j].flag & 0x1) && (pvttype.PvtPkg.week > 0))
        {
	        week = GET_UBITS(tempEph.Ephemeris[j].word[7],20,10);
	        toe = GET_UBITS(tempEph.Ephemeris[j].word[10],14,16);
	        diff = (pvttype.PvtPkg.week - 1024 - week )*604800 + (pvttype.PvtPkg.msCount/1000 - toe*16 + 16);	
	        //D("update,week is %d,toe is %d,diff is %ld",week,toe,diff);
	        if((diff < -14400) || (diff > 14400))
	        {
	            E("ephemeris is too old");
	            break;
	        }
	        if(k < 13)
	        {
                memcpy(Pak.buff+Pak.length,&(tempEph.Ephemeris[j]),sizeof(GPS_EPHEMERIS_PACKED));
                Pak.length = Pak.length + sizeof(GPS_EPHEMERIS_PACKED);
			}
	        k++;
			
        }
    }
		
	D("will be update eph num is %d",k);	
	Pak.type = GNSS_LIBGPS_AIDING_DATA_TYPE;
	Pak.subType = GNSS_LIBGPS_AIDING_GPSEPHEMERIS_SUBTYPE;
	gps_adingData(&Pak);	
    usleep(10000);
//IONOUTC	
	memset(&Pak,0,sizeof(TCmdData_t));	
	Pak.type = GNSS_LIBGPS_AIDING_DATA_TYPE;
	Pak.subType = GNSS_LIBGPS_AIDING_IONOUTC_SUBTYPE;
	Pak.length = sizeof(IONOUTC_PARAM_FLASH);
	fseek(flashfp,4096*GNSS_LIBGPS_FLASH_IONO_UTC_SUBTYPE,SEEK_SET);
    fread(Pak.buff,sizeof(IONOUTC_PARAM_FLASH),1,flashfp);
	gps_adingData(&Pak);    
//GPSALM	
	memset(&Pak,0,sizeof(TCmdData_t));	
	Pak.type = GNSS_LIBGPS_AIDING_DATA_TYPE;
	Pak.subType = GNSS_LIBGPS_AIDING_GPSALMANAC_SUBTYPE;
	Pak.length = sizeof(BLOCK_GPS_ALMANAC_FLASH)+4;
	fseek(flashfp,4096*GNSS_LIBGPS_FLASH_GPS_ALM_SUBTYPE,SEEK_SET);
    fread(Pak.buff+4,sizeof(BLOCK_GPS_ALMANAC_FLASH),1,flashfp);
	gps_adingData(&Pak); 
    usleep(10000);
//GPSALMCOM
	memset(&Pak,0,sizeof(TCmdData_t));	
	Pak.type = GNSS_LIBGPS_FLASH_TYPE;
	Pak.subType = GNSS_LIBGPS_FLASH_ALMCPY_COMM_SUBTYPE;
	Pak.length = sizeof(BLOCK_GPS_ALMCOM_FLASH);
	if(fseek(flashfp,4096*GNSS_LIBGPS_FLASH_ALMCPY_COMM_SUBTYPE,SEEK_SET) == 0)
	{
	    D("begin send almcomm data");
        fread(Pak.buff,Pak.length,1,flashfp);
	    gps_adingData(&Pak); 
        usleep(10000);
    }
//SEQLSQ
	memset(&Pak,0,sizeof(TCmdData_t));	
	Pak.type = GNSS_LIBGPS_FLASH_TYPE;
	Pak.subType = GNSS_LIBGPS_FLASH_PVT_SEQLSQ_SUBTYPE;
	Pak.length = sizeof(SEQLSQ_PARAM_FLASH);
	if(fseek(flashfp,4096*GNSS_LIBGPS_FLASH_PVT_SEQLSQ_SUBTYPE,SEEK_SET) == 0)
	{
	    D("begin send seqlsq data");
        fread(Pak.buff,Pak.length,1,flashfp);
	    gps_adingData(&Pak); 
        usleep(10000);
    }
//BLUNDER_INFO
	memset(&Pak,0,sizeof(TCmdData_t));	
	Pak.type = GNSS_LIBGPS_FLASH_TYPE;
	Pak.subType = GNSS_LIBGPS_FLASH_PVT_BLUNDER_SUBTYPE;
	Pak.length = sizeof(BLUNDER_INFO_FLASH);
	if(fseek(flashfp,4096*GNSS_LIBGPS_FLASH_PVT_BLUNDER_SUBTYPE,SEEK_SET) == 0)
	{
	    D("begin send blunderinfo data");
        fread(Pak.buff,Pak.length,1,flashfp);
	    gps_adingData(&Pak); 
        usleep(10000);
    }
//GPSALMCPY
    for(i = 0; i < 6; i++)
    {
	    memset(&Pak,0,sizeof(TCmdData_t));	
	    Pak.type = GNSS_LIBGPS_FLASH_TYPE;
	    Pak.subType = GNSS_LIBGPS_FLASH_GPS_ALM_CPY1_SUBTYPE + i;
	    Pak.length = sizeof(BLOCK_GPS_ALMCOPY_FLASH);
	    if(fseek(flashfp,4096*Pak.subType,SEEK_SET) == 0)
	    {
	        D("begin send almcpy data");
            fread(Pak.buff,Pak.length,1,flashfp);
	        gps_adingData(&Pak); 
            usleep(10000);
        }
    }
//g_GlonassAlmanac
    if(s->workmode == GPS_GLONASS)
    {
	    memset(&Pak,0,sizeof(TCmdData_t));	
	    Pak.type = GNSS_LIBGPS_FLASH_TYPE;
	    Pak.subType = GNSS_LIBGPS_FLASH_GLO_ALM1_SUBTYPE;
	    Pak.length = sizeof(GLONASS_ALMANAC)*14;
	    fseek(flashfp,4096*GNSS_LIBGPS_FLASH_GLO_ALM1_SUBTYPE,SEEK_SET);
        fread(Pak.buff,sizeof(GLONASS_ALMANAC)*14,1,flashfp);
        D("glonass almanac lenth is %d",Pak.length);
        gps_adingData(&Pak); 
        usleep(10000);
	    memset(&Pak,0,sizeof(TCmdData_t));	
	    Pak.type = GNSS_LIBGPS_FLASH_TYPE;
	    Pak.subType = GNSS_LIBGPS_FLASH_GLO_ALM2_SUBTYPE;
	    Pak.length = sizeof(GLONASS_ALMANAC)*14;
	    fseek(flashfp,4096*GNSS_LIBGPS_FLASH_GLO_ALM2_SUBTYPE,SEEK_SET);
        fread(Pak.buff,sizeof(GLONASS_ALMANAC)*14,1,flashfp);
        gps_adingData(&Pak);  
        usleep(10000);
    //g_GlonassEphemeris
	    memset(&Pak,0,sizeof(TCmdData_t));	
	    Pak.type = GNSS_LIBGPS_FLASH_TYPE;
	    Pak.subType = GNSS_LIBGPS_FLASH_GLO_EPH1_SUBTYPE;
	    Pak.length = sizeof(GLONASS_EPHEMERIS)*7+4;  //for glonass flag
	    fseek(flashfp,4096*GNSS_LIBGPS_FLASH_GLO_EPH1_SUBTYPE,SEEK_SET);
        fread(Pak.buff+4,sizeof(GLONASS_EPHEMERIS)*7,1,flashfp);
        D("glonass ephemeris lenth is %d",Pak.length);
        gps_adingData(&Pak); 
        usleep(10000);   
	    memset(&Pak,0,sizeof(TCmdData_t));	
	    Pak.type = GNSS_LIBGPS_FLASH_TYPE;
	    Pak.subType = GNSS_LIBGPS_FLASH_GLO_EPH2_SUBTYPE;	
	    Pak.length = sizeof(GLONASS_EPHEMERIS)*7+4;
        fseek(flashfp,4096*GNSS_LIBGPS_FLASH_GLO_EPH2_SUBTYPE,SEEK_SET);
        fread(Pak.buff+4,sizeof(GLONASS_EPHEMERIS)*7,1,flashfp);
        gps_adingData(&Pak); 
        usleep(10000);     
	    memset(&Pak,0,sizeof(TCmdData_t));	
	    Pak.type = GNSS_LIBGPS_FLASH_TYPE;
	    Pak.subType = GNSS_LIBGPS_FLASH_GLO_EPH3_SUBTYPE;
	    Pak.length = sizeof(GLONASS_EPHEMERIS)*7+4;
        fseek(flashfp,4096*GNSS_LIBGPS_FLASH_GLO_EPH3_SUBTYPE,SEEK_SET);
        fread(Pak.buff+4,sizeof(GLONASS_EPHEMERIS)*7,1,flashfp);
        gps_adingData(&Pak); 
        usleep(10000);     
	    memset(&Pak,0,sizeof(TCmdData_t));	
	    Pak.type = GNSS_LIBGPS_FLASH_TYPE;
	    Pak.subType = GNSS_LIBGPS_FLASH_GLO_EPH4_SUBTYPE;
	    Pak.length = sizeof(GLONASS_EPHEMERIS)*7+4;
        fseek(flashfp,4096*GNSS_LIBGPS_FLASH_GLO_EPH4_SUBTYPE,SEEK_SET);
        fread(Pak.buff+4,sizeof(GLONASS_EPHEMERIS)*7,1,flashfp);    
        gps_adingData(&Pak); 
        usleep(50000);
    }
    else
    {
        D("update bds assist data");
    	for(i = 0; i < 2; i++)
	    {
            memset(&Pak,0,sizeof(TCmdData_t));	
            Pak.type = GNSS_LIBGPS_AIDING_DATA_TYPE;
            Pak.subType = GNSS_LIBGPS_AIDING_BDSEPHEMERIS_SUBTYPE;
            Pak.length = sizeof(BLOCK_EPHEMERIS_FLASH)+4;
            fseek(flashfp,4096*(GNSS_LIBGPS_FLASH_BD2_EPH1_SUBTYPE + i),SEEK_SET);
            fread(Pak.buff+4,sizeof(BLOCK_EPHEMERIS_FLASH),1,flashfp);
            gps_adingData(&Pak); 
            usleep(10000);  
	    }
	    memset(&Pak,0,sizeof(TCmdData_t));	
        Pak.type = GNSS_LIBGPS_FLASH_TYPE;
        Pak.subType = GNSS_LIBGPS_FLASH_BD2_ALM_SUBTYPE;
        Pak.length = sizeof(BLOCK_BD2_ALMANAC_FLASH);
        fseek(flashfp,4096*Pak.subType,SEEK_SET);
        fread(Pak.buff,Pak.length,1,flashfp);
        gps_adingData(&Pak); 
        usleep(10000);  
    
    }
	fclose(flashfp);
	return;
}

void update_assist_data(GpsState* s)
{
    char version[128];
    memset(version,0,sizeof(version));
    config_xml_ctrl(GET_XML,"GE2-VERSION",version,CONFIG_TYPE); 
    D("%s:version %s",__FUNCTION__,version);
    if(NULL != strstr(version,"GLO"))
    {
        D("workmode is glonass");
        s->workmode = GPS_GLONASS;
    }
    else if(NULL != strstr(version,"BDS"))
    {
        D("workmode is BDS");
        s->workmode = GPS_BDS;
    }
    else
    {
        D("workmode default is glonass");
        s->workmode = GPS_GLONASS;    
    }
//#ifndef GNSS_INTEGRATED
    send_assist_ge2(s);
//#endif 
    return;
}


////////////////////////////////////////////////////////////////////////
void geofence_msa(void)
{
	char type = 2;
    int sid = 0;
    
    if(agpsdata->ni_status)
    {
        sid = agpsdata->ni_sid;
    }
    else
    {
        sid = agpsdata->si_sid;
    }
	E("geofence msa request,got position");
#ifdef GNSS_LCS
	gnss_lcsrequestToGetMsaPosition(sid, isFirstTime);
#else
 	RXN_requestToGetMsaPosition(sid, isFirstTime);	
#endif
	
	if(isFirstTime == 1) isFirstTime = 0;
//	usleep(200000);
//	gps_sendData(SUPL_READY,&type);
	return;
}
//static char msatype = 2;

void send_glonass_eph(void)
{
    TCmdData_t Pak;
	FILE *tFlashfp;
    int i;
    D("%s enter",__FUNCTION__);
	tFlashfp = fopen("/data/gnss/FlashCfg.cg","rb");
	if(tFlashfp == NULL)
	{
		E("open flash file fail,%s",strerror(errno));
		return;
	}
	for(i = 0; i < 4; i++)
	{
        memset(&Pak,0,sizeof(TCmdData_t));	
        Pak.type = GNSS_LIBGPS_FLASH_TYPE;
        Pak.subType = GNSS_LIBGPS_FLASH_GLO_EPH1_SUBTYPE + i;
        Pak.length = sizeof(GLONASS_EPHEMERIS)*7;
        fseek(tFlashfp,4096*Pak.subType,SEEK_SET);
        fread(Pak.buff,sizeof(GLONASS_EPHEMERIS)*7,1,tFlashfp);
        gps_adingData(&Pak); 
        usleep(10000);  
	}

	fclose(tFlashfp);   
    return;
}

/*--------------------------------------------------------------------------
    Function:  gps_sendLte

    Description:
        it  encode and send  start-module   request  
    Parameters:
     flag: start module  request flag 
    Return: the write data length  
--------------------------------------------------------------------------*/
int gps_sendLte(char *output,int lenth)
{
	GpsState*	s = _gps_state;
	TCmdData_t packet;
	int len,writeLen ;

	len = writeLen = 0;
	E("gps sendlte now,lenth is %d",lenth);
	memset(&packet,0,sizeof(TCmdData_t));
	//first check it can send data ?  todo 	
	packet.type    = GNSS_LIBGPS_DATA_TYPE;
	packet.subType = GNSS_LIBGPS_LTE_SUBTYPE;
	packet.length = lenth;
	memcpy(packet.buff,output,lenth);
    pthread_mutex_lock(&s->writeMutex);
	len = GNSS_EncodeOnePacket(&packet,s->writeBuffer, SERIAL_WRITE_BUUFER_MAX);

	if(len > 0)
	{
		writeLen = gps_writeSerialData(s,len);   //gps_writeData2Buffer
	}
	else
	{
		E("gps_sendCommnd->GNSS_EncodeOnePacket error in send lte \r\n ");
	}
	//debug 
	D("gps_sendCommnd->GNSS_EncodeOnePacket send lte writelen=%d \r\n ", writeLen);
	usleep(100000);
    pthread_mutex_unlock(&s->writeMutex);
	return writeLen;
}


static void prepareLteRequest()
{
    int i = 0;
    int sv_list[MAX_SATS_NUMBERS];
    int sv_num = 0;
    int len = 0;
    GpsState*  s = _gps_state;
    char output[2048];
    
	for(i = 0; i < MAX_SATS_NUMBERS; i++)
	{
		if((agpsdata->mask & (1<<i)) != 0)
		{
			sv_list[sv_num++] = i + 1;
		}
	}
	for(i = 0; i < sv_num; i++)
	{
		memset(output,0,sizeof(output));
		E("predict sav id is %d",sv_list[i]);
		len = SF_getSvPredictPosVel(sv_list[i], agpsdata->weekno, agpsdata->tow, output);
		if(len != 0)
		{
		    //for by wlh test 
			gps_sendLte(output,len);     //if any error,add delay
		}
	}
}


static void agps_timeout_handler()
{    
    time_t now =time(NULL);
    double intervaltime,intervaleph;
    GpsState*  pState = (GpsState*)_gps_state;
        
    if(AGPS_SESSION_STOP == pState->agps_session)
    {
        D("AGPS Session work normal in ten seconds");    
    }
    else if(AGPS_SESSION_START == pState->agps_session)
    {
         pState->agps_session = AGPS_SESSION_TIMEOUT;
         //send  the last time location eph to firmware in 1 hours 
         intervaltime = difftime(now,pState->lastreftime);
         intervaleph = difftime(now,pState->lastrefeph);
        if((intervaltime < AGPS_SESSION_TIMEOU_SECONDS) &&(intervaleph < AGPS_SESSION_TIMEOU_SECONDS))
        {
            D("AGPS Session timeout and send last ref time location and eph, intervaltime(%f)", intervaltime);
            agps_send_lastrefparam((int)intervaltime);
        }
        else
        {
            E("AGPS Session timeout and the last ading data invalid ");    
        }
         
    }
    else
    {
        E("it's abnormal AGPS Session State");
    }
}


static void start_agps_timer()
{
    struct itimerval agps_timeout;
    GpsState*  pState = (GpsState*)_gps_state;

    if(((0 == pState->connected)||(1 == pState->cmcc))&&(pState->nokiaee_enable == 0))
    {
        D("%s: network disconnect or run cmmc test, and nokai ee disable.",__func__);
        return;
    }
    memset(&agps_timeout,0,sizeof(struct itimerval));
	agps_timeout.it_value.tv_sec = 10; //agps 10 seconds timeout 
	agps_timeout.it_value.tv_usec = 0;
	agps_timeout.it_interval.tv_sec = 0; //only once ,so interval set 0
	agps_timeout.it_interval.tv_usec = 0;//only once ,so interval set 0
	pState->agps_session = AGPS_SESSION_START;
    D("%s: start AGPS timeout handler.",__func__);
    signal(SIGALRM, agps_timeout_handler);
    setitimer(ITIMER_REAL, &agps_timeout, NULL);
    
}

void set_request_assdata(GpsState*  s)
{
    memset(&AssistData,0,sizeof(GNSSReqAssData_t));									 
 
	if(s->cpmode == 1)
	{
	    AssistData.gnssNum = 0;
	    D("gps only,should not request other type");
	    return;
	}
	if(s->workmode == GPS_GLONASS) 
	{
	    AssistData.genericAssData[0].gnssID = SUPL_GNSS_ID_GLONASS;   //maybe this can be
        AssistData.genericAssData[0].navModelFlag = 1;
        AssistData.genericAssData[0].auxiliaryInfo = 1;
        AssistData.gnssNum = 1;    
        AssistData.refLocation = 1;	
        AssistData.commonAssFlag = 1;
        AssistData.refTime = 1;
        AssistData.ionosphericModel = 1;	
        AssistData.earthOrienationPara = 1; 
    }
    else if(s->workmode == GPS_BDS)
    {
        AssistData.genericAssData[0].gnssID = SUPL_GNSS_ID_BDS;   //maybe this can be
        AssistData.genericAssData[0].navModelFlag = 1;
        AssistData.genericAssData[0].auxiliaryInfo = 1;
        AssistData.gnssNum = 1;   
        AssistData.refLocation = 1;	
        AssistData.commonAssFlag = 1;
        AssistData.refTime = 1;
        AssistData.ionosphericModel = 1;	
        AssistData.earthOrienationPara = 1; 
    }
    else
    {
        E("=========>>>>>>error!!!workmode is not glonass or bds");
    }
}

void transGEOPos(TCgAgpsPosition *pPos,GNSSLocationCoordinates_t *pPosOut)
{
    GNSSEllipsoidPointWithAltitudeAndUncertaintyEllipsoid_t *pEll;
    pPosOut->coordType = GNSSLocationCoordinatesType_ellipsoidPointWithAltitudeAndUncertaintyEllipsoid;
    pEll =  &(pPosOut->choice.ellPointAltitudeEllips);
    if(pPos->latitude < 0)
    {
        pEll->latitudeSign = GNSSLatitudeSign_south;
    }
    pEll->degreesLatitude = ((fabs(pPos->latitude) / 90) * 0x800000);
    pEll->degreesLongitude = (pPos->longitude / 360) * 0x1000000;
    pEll->altitude = abs(pPos->altitude);
    if(pPos->latitude < 0)
    {
        pEll->altitudeDirection = GNSSAltitudeDirection_depth;

    }
    pEll->uncertaintySemiMajor = 1;
    pEll->uncertaintySemiMinor = 1;
    pEll->orientationMajorAxis = 1;
    pEll->uncertaintyAltitude = 1;
    pEll->confidence = 68;           
}

static void get_agps_qop()
{
	int start_time = 0,stop_time = 0,interval = 0,ret = 0;
	char *H_ACC = "HORIZON-ACCURACY";
	char *V_ACC = "VERTICAL-ACCURACY";
	char *L_AGE ="LOC-AGE";
	char *DELAY = "DELAY";
	char value[32];


	memset(&Qop,0,sizeof(TCgSetQoP_t));
	memset(value,0,32);
	ret = get_xml_value(H_ACC, value);
	if(ret == 0)
	{
		Qop.QoP_HorAcc = atoi(value);
		if(Qop.QoP_HorAcc != 0)
		Qop.flag = Qop.flag | 0x01;
		
		D("QoP_HorAcc is %d",Qop.QoP_HorAcc);
	}
	
	memset(value,0,32);
	ret = get_xml_value(V_ACC, value);
	if(ret == 0)
	{
		Qop.QoP_VerAcc = atoi(value);
		if(Qop.QoP_VerAcc != 0)
		Qop.flag = Qop.flag | 0x02;
		
		D(" QoP_VerAcc is %d",Qop.QoP_VerAcc);
	}
	
	memset(value,0,32);
	ret = get_xml_value(L_AGE, value);
	if(ret == 0)
	{
		Qop.QoP_MaxLocAge = atoi(value);
		if(Qop.QoP_MaxLocAge != 0)
		Qop.flag = Qop.flag | 0x04;
		
		D(" QoP_MaxLocAge is %d",Qop.QoP_MaxLocAge);
	}
	
	memset(value,0,32);
	ret = get_xml_value(DELAY, value);
	if(ret == 0)
	{
		Qop.QoP_Delay = atoi(value);
		if(Qop.QoP_Delay != 0)
		Qop.flag = Qop.flag | 0x08;
		
		D("QoP_Delay is %d",Qop.QoP_Delay);
	}
    D("QoP.flag is %d",Qop.flag);

}

int agps_request_data_conn(GpsState*  s )
{
     AGpsStatus AgpsStatus;
     int ret  = 0;

#if 0
    if(s->supldata)
    {
        D("%s: it has start  agps supl data session ,so don't request again",__func__);
        return -1;
    }
    else
    {
        s->supldata = SUPL_NET_REQ;
        s->connected = 0;
    }
#endif

	if(agpsdata->ni_status == 1)
	{
		usleep(500000);
	}

	memset(&AgpsStatus,0,sizeof(AGpsStatus));
	AgpsStatus.size = sizeof(AGpsStatus);
	AgpsStatus.type = AGPS_TYPE_SUPL;
	AgpsStatus.status = GPS_REQUEST_AGPS_DATA_CONN;
	AgpsStatus.ipaddr = 0;
	if((s->init) && (s->agps_callbacks.status_cb != NULL))
	{
		s->agps_callbacks.status_cb(&AgpsStatus);
		D("%s: GPS_REQUEST_AGPS_DATA_CONN ",__func__);
		ret = 0;
	}
	else
	{
		ret = -1;
	}

    return ret;
}


 void agps_release_data_conn(GpsState*  s)
{

    AGpsStatus AgpsStatus;

#if 0    
    if(SUPL_NET_RELEASE == s->supldata)
    {
        D("%s:  it hasn't request supl data connection  ,so don't need  release",__func__);
        return;
    }
    else
    {
        s->supldata = SUPL_NET_RELEASE;
    }
#endif
	s->supldata = SUPL_NET_RELEASE;
	memset(&AgpsStatus,0,sizeof(AGpsStatus));
	AgpsStatus.size = sizeof(AGpsStatus);
	AgpsStatus.type = AGPS_TYPE_SUPL;
	AgpsStatus.status = GPS_RELEASE_AGPS_DATA_CONN;
	if((s->init) && (s->agps_callbacks.status_cb != NULL))
	{
		s->agps_callbacks.status_cb(&AgpsStatus);
		D("%s: GPS_RELEASE_AGPS_DATA_CONN ",__func__);
	}
}


void agps_msg_thread(void *arg) 
{
    GpsState*  s = (GpsState*)arg;
    struct epoll_event aevents[2];
    int  epoll_fd = epoll_create(2);
    int  asock_fd,fd,i,ret = 0;
    int nevents = 0,error_time = 0;
    char cmd = 255;
    int sid = 0,flags;

    Qop.QoP_HorAcc = 100;
    Qop.flag = 0x01;
    
    socket_status = thread_run;
    //agps_thread_init(s);
    asock_fd = s->asock[1];
    
    epoll_register( epoll_fd, asock_fd );  
    //set witte pair socket is non block
    flags = fcntl(s->asock[0], F_GETFL);
    if(fcntl(s->asock[0], F_SETFL, flags | O_NONBLOCK) < 0)
    {
        E("the %s fcntl set asock[0] O_NONBLOCK fail",__FUNCTION__);
    }
    
    while(socket_status == thread_run) 
    {
        nevents = epoll_wait( epoll_fd, aevents, 2, -1 );
        if (nevents < 0) 
        {
            if (errno != EINTR)
            {
                E("epoll_wait() unexpected error: %s", strerror(errno));
                if(asock_fd < 0)
                E("this is impossible for control fd is invalid");	
                error_time++;
                if(error_time > 10)
                break;
            }
            continue;
        }
        
        for (i = 0; i < nevents; i++) 
        {
            if((aevents[i].events & (EPOLLERR|EPOLLHUP)) != 0) 
	    {
                E("EPOLLERR or EPOLLHUP after epoll_wait() !?");
                socket_status = thread_stop;
                break;
            }
            if((aevents[i].events & EPOLLIN) != 0)
            {
                fd = aevents[i].data.fd;
                if(fd == asock_fd)
                {
                    ret = read(fd, &cmd, 1 ); 
                    D("%s: cmd is %d before",__func__,cmd);
                    switch(cmd)
                    {
                        case START_SI:
                             D("%s: start si session.",__func__);
							 if(gPosMethod == RSP_MSA)
								agpsdata->assistflag = agpsdata->assistflag | CG_SUPL_DATA_TYPE_ACQUISITION_ASSISTANCE;

							 D("%s: request assistance flag is %x",__func__,agpsdata->assistflag);
                             set_request_assdata(s);
							 AssistData.gpsAssistanceRequired = agpsdata->assistflag;
							 D("ni_status=%d,cp_begin=%d,peroid_mode=%d,agps enable=%d,nokiaee_enable=%d",
							 agpsdata->ni_status,agpsdata->cp_begin,agpsdata->peroid_mode,agpsdata->agps_enable,s->nokiaee_enable);
                             if((agpsdata->ni_status == 0) && (agpsdata->cur_state < 10) && 
                             (agpsdata->peroid_mode == 0) && ((agpsdata->agps_enable != 0)||(s->nokiaee_enable == 1)))
							 {
                                if(0 == agps_request_data_conn(s))
                                {
                                     D("it send agps request ");
                                }
                                else
                                {
                                    D("it don't need to  agps request ");
                                }

							 }
                             break;
                         case AGPS_NETWORK_CMD:
                            { 
                                D("%s: AGPS_NETWORK_CMD",__func__);
                                if(agpsdata->ni_status)//network inital 
                                {   
                                    if(agpsdata->cur_state < 10)    
                                    {
                                        D("%s supl ni msg",__func__);
                                        usleep(500000);
                                        CG_gotSUPLInit(agpsdata->suplnimsg,agpsdata->suplniLen);
                                    }
                                }
                                else// supl si network
                                {
                                    if(s->gps_flag == CMD_START)
                                    {
                                        if((s->nokiaee_enable == 1)&&((query_server_mode() == 1)||((query_server_mode() == 0)&&(s->network_status == 0))))
                                        {
                                            s->use_nokiaee = 1;
                                            set_supl_param();
                                        }
                                        else
                                        {
                                            s->use_nokiaee = 0;
                                            //set_supl_param();
                                        }            
                                        
                                        get_agps_qop();
                                        agpsdata->si_sid = get_session_id(1);
#ifdef GNSS_LCS
                                        gnss_lcsContrlEcid(agpsdata->si_sid,LCS_DOMAIN_UP,LCS_NWLR_ECID,1);
                                        gnss_lcsstartSISessionForGnss(agpsdata->si_sid,LCS_DOMAIN_UP,0,&AssistData,&Qop, NULL);
#else
                                        SPRD_startSISessionForGnss(agpsdata->si_sid,&AssistData,&Qop, NULL);
#endif
                                        start_agps_timer();// start once timer
                                    }
                                }
                                
                                break;
                            }
                        case MSISDN_CMD:
                             get_type_id(AGPS_SETID_TYPE_MSISDN);
                             break;
                        case IMSI_CMD:
                             E("imsi cmd before");
                             get_type_id(AGPS_SETID_TYPE_IMSI);
                             E("imsi cmd after");
                             imsi_status = STATUS_OK;
                             break;
                        case CID_CMD:
                             get_ri_locationid();
                             cid_status = STATUS_OK;
                             break;
                        case SMS_CMD:
							 E("sms cmd is enter,id is %d",gnotification[0].notification_id);
                             s->ni_callbacks.notify_cb(gnotification);
                             break;
                        case WIFI_CMD:
                             E("request wifi info begin");
                        #if WIFI_FUNC
                             s->agps_ril_callbacks.request_wifi(0x01);
                        #endif
                             break;
                        case MEASURE_CMD:
							 sid = agpsdata->readySid;
                             E("we will send measure cmd begin,sid is %d",sid);
							 if(sid == 0)
                             {
								D("send measurement to android framework");
								if(s->measurement_callbacks.measurement_callback != NULL)
								{
									s->measurement_callbacks.measurement_callback(&gps_msr);
								}
#ifdef GNSS_ANDROIDN
								if(s->measurement_callbacks.gnss_measurement_callback != NULL)
								{
									gnss_msr.clock.size = sizeof(GnssClock);
									gnss_msr.clock.flags = gps_msr.clock.flags;
									gnss_msr.clock.leap_second = gps_msr.clock.leap_second;
									gnss_msr.clock.time_ns = gps_msr.clock.time_ns;
									gnss_msr.clock.time_uncertainty_ns = gps_msr.clock.time_uncertainty_ns;
									gnss_msr.clock.full_bias_ns = gps_msr.clock.full_bias_ns;
									gnss_msr.clock.bias_ns = gps_msr.clock.bias_ns;
									gnss_msr.clock.bias_uncertainty_ns = gps_msr.clock.bias_uncertainty_ns;
									gnss_msr.clock.drift_nsps = gps_msr.clock.drift_nsps;
									gnss_msr.clock.drift_uncertainty_nsps = gps_msr.clock.drift_uncertainty_nsps;
									gnss_msr.clock.hw_clock_discontinuity_count = 0;
									//s->measurement_callbacks.gnss_measurement_callback(&gnss_msr);
								}
#endif //GNSS_ANDROIDN 
								break;
                             }
                             EGnssVel.velType = GNSSVelocityType_horizontalWithVerticalVelocityAndUncertainty;
                             EGnssVel.choice.VelUncertainty.bearing = (long)eVelocity.bearing;
                             EGnssVel.choice.VelUncertainty.horizontalSpeed = (long)eVelocity.horizontalSpeed;
                             EGnssVel.choice.VelUncertainty.verticalDirection = eVelocity.verticalDirect;
                             EGnssVel.choice.VelUncertainty.verticalSpeed = (long)eVelocity.verticalSpeed;
                             EGnssVel.choice.VelUncertainty.horizontalUncertaintySpeed = 0;
                             EGnssVel.choice.VelUncertainty.verticalUncertaintySpeed = 0;
							 if(agpsdata->cur_state < 10)
 #ifdef GNSS_LCS 
                              gnss_lcsgnssMeasureInfo(sid,&GnssMsrlist, &EGnssVel,LCS_DOMAIN_UP);
#else
                              SPRD_gnssMeasureInfo(sid,&GnssMsrlist, &EGnssVel);
#endif
							   

                             	
                             else
 #ifdef GNSS_LCS
    			      gnss_lcsgnssMeasureInfo(sid, &GnssMsrlist, &EGnssVel,LCS_DOMAIN_CP);	
#else
	                          ctrlplane_SPRD_gnssMeasureInfo(sid,&GnssMsrlist, &EGnssVel);
#endif
                             	 
                             break;

						case AGPS_END:
							 D("AGPS END PARSE");
							 AGPS_End_Session();
                              agps_release_data_conn(s);
							 break;
						case GEO_TRIGGER:
                            if(agpsdata->ni_status)
                            {
                                sid = agpsdata->ni_sid;
                            }
                            else
                            {
                                sid = agpsdata->si_sid;
                            }
							 D("geofence is trggered,sid is %d,cur_state:%d",sid,agpsdata->cur_state);
							 if(agpsdata->cur_state == GEO_MSA_NI)
							 {
								memcpy(&msaLocationInfo->velocity,&agpsdata->Velocity,sizeof(TcgVelocity));
								memcpy(&geopos.time,&gLocationInfo->time,sizeof(TcgTimeStamp)); //this maybe not needed
								msaLocationInfo->pos.latitude = agpsdata->msalatitude;
								msaLocationInfo->pos.longitude = agpsdata->msalongitude;
								msaLocationInfo->pos.altitude = agpsdata->msaaltitude;
								D("msa ni:lat=%f,long=%f",agpsdata->msalatitude,agpsdata->msalongitude); 								
						        geopos.velocity.velType = GNSSVelocityType_horizontalWithVerticalVelocityAndUncertainty;
                                geopos.velocity.choice.VelUncertainty.bearing = (long)msaLocationInfo->velocity.bearing;
                                geopos.velocity.choice.VelUncertainty.horizontalSpeed = (long)msaLocationInfo->velocity.horizontalSpeed;
                                geopos.velocity.choice.VelUncertainty.verticalDirection = msaLocationInfo->velocity.verticalDirect;
                                geopos.velocity.choice.VelUncertainty.verticalSpeed = (long)msaLocationInfo->velocity.verticalSpeed;
                                geopos.velocity.choice.VelUncertainty.horizontalUncertaintySpeed = 0;
                                geopos.velocity.choice.VelUncertainty.verticalUncertaintySpeed = 0;                 
                                transGEOPos(&msaLocationInfo->pos,&geopos.pos);
#ifdef GNSS_LCS
    							gnss_lcsinformAreaEventTriggered(sid,&geopos);
#else
								RXN_informAreaEventTriggered(sid,&geopos);	
#endif
							 }
							 else
							 {    
							  memcpy(&geopos.time,&gLocationInfo->time,sizeof(TcgTimeStamp));
							  geopos.velocity.velType = GNSSVelocityType_horizontalWithVerticalVelocityAndUncertainty;
                              geopos.velocity.choice.VelUncertainty.bearing = (long)gLocationInfo->velocity.bearing;
                              geopos.velocity.choice.VelUncertainty.horizontalSpeed = (long)gLocationInfo->velocity.horizontalSpeed;
                              geopos.velocity.choice.VelUncertainty.verticalDirection = gLocationInfo->velocity.verticalDirect;
                              geopos.velocity.choice.VelUncertainty.verticalSpeed = (long)gLocationInfo->velocity.verticalSpeed;
                              geopos.velocity.choice.VelUncertainty.horizontalUncertaintySpeed = 0;
                              geopos.velocity.choice.VelUncertainty.verticalUncertaintySpeed = 0;
                              
                              transGEOPos(&gLocationInfo->pos,&geopos.pos);
#ifdef GNSS_LCS
                              gnss_lcsinformAreaEventTriggered(sid,&geopos); 
#else
	                          RXN_informAreaEventTriggered(sid,&geopos);
#endif
							 }
							 break;
						case GEO_MSA_TRIGGER:
							 D("msa geofence begin");
    						 geofence_msa();
							 break;
                        case AGPS_LTE_CMD:
                        {
                            D("prepareLteRequest and send lte data");
                            prepareLteRequest();
							break;
                        }
						case GEO_SESSION_END:
					        if(agpsdata->ni_status)
                            {
                                sid = agpsdata->ni_sid;
                            }
                            else
                            {
                                sid = agpsdata->si_sid;
                            }
							 D("geofence session is end,sid is %d",sid);
#ifdef GNSS_LCS
                              gnss_lcsContrlEcid(sid,LCS_DOMAIN_UP,LCS_NWLR_ECID,0);
   							  gnss_lcsendAreaEventSession(sid);
#else
                              RXN_endAreaEventSession(sid);	
#endif
							 
				             AGPS_End_Session();
                             break;
						case NAVIGATION_CMD:
							D("navigation cmd is received in agps server,prn %d",agpsdata->navidata.prn);
							navmsg.size = sizeof(GpsNavigationMessage);
							navmsg.prn = agpsdata->navidata.prn;
							navmsg.type = GPS_NAVIGATION_MESSAGE_TYPE_L1CA;
							navmsg.status = NAV_MESSAGE_STATUS_PARITY_PASSED;
							if(agpsdata->navidata.message_id > 0)
								navmsg.message_id = agpsdata->navidata.message_id + 1;
							else
								navmsg.message_id = agpsdata->navidata.message_id;
							navmsg.submessage_id = agpsdata->navidata.submessage_id;
							navmsg.data_length = 40;
							navmsg.data = agpsdata->navidata.data;
							if(s->navigation_message_callbacks.navigation_message_callback != NULL)
							{
								D("navigation_message_callback");
								s->navigation_message_callbacks.navigation_message_callback(&navmsg);
							}
							
#if 0
							// gnss_navigation_message_callback
							gnss_navmsg.size = sizeof(GnssNavigationMessage);
							gnss_navmsg.svid = agpsdata->navidata.prn;
							gnss_navmsg.type = GNSS_NAVIGATION_MESSAGE_TYPE_GPS_L1CA;
							gnss_navmsg.status = NAV_MESSAGE_STATUS_PARITY_PASSED;
							if(agpsdata->navidata.message_id > 0)
								gnss_navmsg.message_id = agpsdata->navidata.message_id + 1;
							else
								gnss_navmsg.message_id = agpsdata->navidata.message_id;
							gnss_navmsg.submessage_id = agpsdata->navidata.submessage_id;
							gnss_navmsg.data_length = 40;
							gnss_navmsg.data = agpsdata->navidata.data;
							if(s->navigation_message_callbacks.gnss_navigation_message_callback != NULL)
							{
								D("gnss_navigation_message_callback");
								s->navigation_message_callbacks.gnss_navigation_message_callback(&gnss_navmsg);
							}
#endif //if o end 
							break;
                        default:
                             break;

                    }
                    
                    D("%s: cmd is %d after",__func__,cmd);
                 }
            }
         }
    }
    socket_status = thread_stop;
    agps_thread_close(s);
    close(epoll_fd);
    close(epoll_fd + 1);
    E("error times is %d",error_time);
    return;
}



