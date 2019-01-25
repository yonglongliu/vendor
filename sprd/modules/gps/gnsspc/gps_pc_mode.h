#ifndef GPS_PC_MODE_H
#define GPS_PC_MODE_H

/*just a part of gps_pc_mode from gps_pc_mode in gps.c\
this value need sync with it*/
enum gps_pc_mode{
	GPS_WARM_START	  = 1,
	GPS_PS_START	  = 100,

	GPS_COLD_START	  = 125,

	GPS_LOG_ENABLE	  = 136,
	GPS_LOG_DISABLE   = 520,
	GPS_HOT_START	  = 1024,

	GPS_TCXO	  = 50001,  /*TCXO stability test*/
	GPS_CW_CN0        = 50002,  /*CW CN0 test*/
	GPS_RF_GPS	  = 50003,  /*RF data tool GPS mode*/
	GPS_TSX_TEMP	  = 50004,  /*TSX TEMP test*/
	GPS_NEW_TCXO	  = 50005,  /*TCXO new stability test*/
	GPS_RF_GL    	  = 50006,  /*RF data tool GLONASS mode*/
	GPS_RF_BD         = 50007,  /*RF data tool BEIDOU mode*/

	GPS_FAC_START	  = 65535,  /*factory start*/
};

#define EUT_GPS_OK                  "+SPGPSTEST:OK"
#define EUT_GPS_ERROR               "+SPGPSTEST:ERR="
#define EUT_GPS_REQ                 "+SPGPSTEST:EUT="
#define EUT_GPS_PRN_REQ             "+SPGPSTEST:PRN="
#define EUT_GPS_SNR_REQ             "+SPGPSTEST:SNR="
#define EUT_GPS_RSSI_REQ            "+SPGPSTEST:RSSI="
#define EUT_GPS_TSXTEMP_REQ         "+SPGPSTEST:TSXTEMP="
#define EUT_GPS_TCXO_REQ            "+SPGPSTEST:TCXO="
#define EUT_GPS_READ_REQ            "+SPGPSTEST:READ="
#define EUT_GPS_SEARCH_REQ          "+SPGPSTEST:SEARCH="
#define EUT_GPS_SNR_NO_EXIST        "NO_EXIST"
#define EUT_GPS_NO_FOUND_STAELITE   "NO_FOUND_SATELLITE"
#define EUT_GPS_SV_ID               "SV_ID="
#define EUT_GPS_SV_NUMS             "SV_NUM="

#define EUT_GPSERR_SEARCH                   (153)
#define EUT_GPSERR_PRNSTATE                 (154)
#define EUT_GPSERR_PRNSEARCH                (155)

typedef void (*report_ptr)(const char* nmea, int length);
extern void set_report_ptr(report_ptr func);
extern void set_pc_mode(char input_pc_mode);
extern int gps_export_start(void);
extern int gps_export_stop(void);
extern int get_nmea_data(char *nbuff);
extern int set_gps_mode(unsigned int mode);
extern int get_init_mode(void);
extern int get_stop_mode(void);
extern void gps_eut_parse(char *buf,char *rsp);
extern int write_register(unsigned int addr,unsigned int value);
extern unsigned int read_register(unsigned int addr);
void cw_data_capture(const char* nmea, int length);

#endif
