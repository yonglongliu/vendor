#ifndef GPS_RIL_CMD_H
#define GPS_RIL_CMD_H

#define	MSISDN_GET_CMD  1
#define	IMSI_GET_CMD  3
#define	CELL_LOCATION_GET_CMD  4
#define	VERIF_GET_CMD  5
#define	SMS_GET_CMD  6
#define	DATA_GET_CMD  7
#define	SEND_TIME 10
#define	SEND_LOCTION 11
#define	SEND_MDL 12
#define SEND_DELIVER_ASSIST_END 13
#define SEND_SUPL_END 14
#define SEND_ACQUIST_ASSIST 15
#define SEND_SUPL_READY 16
#define SEND_MSA_POSITION 17

#define GET_POSITION 18
#define SMS_CP_CMD 20
#define IP_GET_CMD 21
#define SIM_PLMN_CMD 22

#define CG_ALMANAC			0x0001
#define CG_UTC_MODEL			0x0002
#define CG_IONIC_MODEL			0x0004
#define CG_DGPS_CORRECTION		0x0008
#define CG_LOCATION		        0x0010
#define CG_TIME		                0x0020
#define CG_ACQUISITION_ASSISTANCE	0x0040
#define CG_REAL_TIME_INTEGRITY		0x0080
#define CG_NAVIGATION_MODEL		0x0100
#define IMSI_MAXLEN   15
#define MSISDN_MAXLEN 11

//
#define ENGINE_SEND_SI_SESSION  0x1
//
#define ENGINE_SEND_GPS_POSITION  0x2
//
#define ENGINE_SEND_GPS_MEASUREMENT  0x3



#define SOCK_PATH "/data/gnss/supl/foo.sock"
#define SUPL_PATH "/data/gnss/supl/supl.sock"

//for engine switch
#define IDEL_ON   "$PCGDC,IDLEON,1,*1\r\n"
#define IDEL_OFF "$PCGDC,IDLEOFF,1,*1\r\n"

int wait_for_complete(char flag);
#endif




























