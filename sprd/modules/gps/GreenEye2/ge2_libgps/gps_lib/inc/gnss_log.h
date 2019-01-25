#ifndef __GNSS_LOG_H
#define __GNSS_LOG_H

#define GNSS_LOG_MAX_RETRY       (20)
#define MAXLINE                  (80)


typedef enum
{
    GNSS_TYPE_LOG_UNKOWN = 0,
    GNSS_TYPE_LOG_TRACE,
    GNSS_TYPE_LOG_NMEA,
    GNSS_TYPE_LOG_RF,
    GNSS_TYPE_LOG_MAX,
}GNSS_Log_e;

void savenmealog2Buffer(const char*pdata, int len);
void gnss_log_thread( void*  arg );


#endif //__GNSS_LOG_H


