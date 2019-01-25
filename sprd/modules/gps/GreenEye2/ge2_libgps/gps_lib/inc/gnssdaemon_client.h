#ifndef DAEMON_CLIENT_H
#define DAEMON_CLIENT_H


#define GNSS_LCS_SOCKET_NAME "GNSS_LCS_SERVER"
#define GNSS_SOCKET_BUFFER_SIZE    (512)
#define GNSS_SOCKET_DATA_HEAD    (3)

/*****************************************************/
// type+          datalen +data
// unsigned char  (short) + ????
/*****************************************************/

#define GNSS_CONTROL_NOTIFIED_INITED_TYPE    0x00 //Geofence
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




//gnss work mode constant define  
#define GNSS_GPS_WORKMODE            (1) 
#define GNSS_BD_WORKMODE             (2) 
#define GNSS_GPS_BD_WORKMODE         (3) 
#define GNSS_GLONASS_WORKMODE        (4) 
#define GNSS_GPS_GLONASS_WORKMODE    (5)


//define type of client to daemon,type(unsigned char)+datalen(short)+data+crc(short)
//#define WIFI_INFO_GET_TYPE  0x40
//define type of client from daemon
//#define WIFI_INFO_SET_TYPE  0x80

void gnssdaemon_client_thread(void *arg);
void gnss_send2Daemon(GpsState* pState,char type,void* pData, unsigned short len);

#endif
