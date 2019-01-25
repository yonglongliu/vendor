/******************************************************************************
** File Name:  gnssdaemon.h                                                      *
** Author:                                                                      *
** Date:                                                                      *
** Copyright:  2015-2020 by Spreadtrum Communications, Inc.                   * 
**             All rights reserved.                                           *
**             This software is supplied under the terms of a license         *
**             agreement or non-disclosure agreement with Spreadtrum.         *
**             Passing on and copying of this document, and communication     *
**             of its contents is not permitted without prior written         *
**             authorization.                                                 *
**description:  define gnss daemon common constants and macro               *
*******************************************************************************

*******************************************************************************
**                                Edit History                                *
**----------------------------------------------------------------------------*
** Date                  Name                      Description                *
** 2016-02-06            lihuai                    Create.                    *
******************************************************************************/
#ifndef _GNSS_DAEMON_H_
#define _GNSS_DAEMON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <utils/Log.h>
#include <pthread.h>

#undef LOG_TAG
#define LOG_TAG 	"GNSS_DAEMON"


#define GNSS_LCS_SOCKET_NAME "GNSS_LCS_SERVER"
#define GNSS_SOCKET_BUFFER_SIZE    (512)

#define GNSS_SOCKET_DATA_HEAD    (3)
#define SGPS_DAEMON_DATA_HEAD     (10)
#define SGPS_DAEMON_DATA_MIN     (13)

/*****************************************************/
// type+          datalen +data
// unsigned char  (short) + ????
/*****************************************************/
#define GNSS_CONTROL_NOTIFIED_INITED_TYPE    0x00 //init notify
#define GNSS_CONTROL_NOTIFIED_GEOFENCE_TYPE  0x01 //Geofence
#define GNSS_CONTROL_SET_SENSOR_TYPE         0x02 
#define GNSS_CONTROL_SET_WORKMODE_TYPE       0x03 //glonass,beidou,gps 
#define GNSS_CONTROL_SET_LTE_TYPE            0x04   
#define GNSS_CONTROL_SET_ASSERT_TYPE         0x05 //assert 
#define GNSS_CONTROL_SET_MOLA_TYPE           0x06 //control plan test  
#define GNSS_CONTROL_SET_TRIGGER_TYPE        0x07 //start or stop period test
#define GNSS_CONTROL_SET_WIFI_TYPE           0x08 //start or stop period test
#define GNSS_CONTROL_SET_ANGRYGPS_AGPSMODE   0x09

#define GNSS_CONTROL_GET_WIFI_TYPE           0x18 
#define GNSS_CONTROL_GET_ANGRYGPS_AGPSMODE   0x19
#define GNSS_CONTROL_MAX_TYPE                0x24 // $$$$ , it can't equal $


#define GPS_STANDALONE                       0
#define GPS_MSB                              1
#define GPS_MSA                              2


//gnss work mode constant define  
#define GNSS_GPS_WORKMODE            (1) 
#define GNSS_BD_WORKMODE             (2) 
#define GNSS_GPS_BD_WORKMODE         (3) 
#define GNSS_GLONASS_WORKMODE        (4) 
#define GNSS_GPS_GLONASS_WORKMODE    (5)



#define GNSSDAEMON_LOGD  ALOGD
#define GNSSDAEMON_LOGE(fmt, arg...) ALOGE("%s: " fmt " %d \n", __func__, ## arg, errno);
#define SIGNAL(s,act,handler)      do { \
		act.sa_handler = handler; \
		if (sigaction(s, &act, NULL) < 0) \
			GNSSDAEMON_LOGE("Couldn't establish signal handler (%d): %m", s); \
	} while (0)


#define GNSS_LCS_CLIENT_NUM		(8)


typedef struct pgnssserver {
	pthread_mutex_t client_fds_lock;
	int client_fds[GNSS_LCS_CLIENT_NUM];
	int listen_fd;
	int gnssclient_fd;
	int flag_connect;
}pgnssserver_t;




#ifdef __cplusplus
}
#endif

#endif /* end _GNSS_DAEMON_H_ */




