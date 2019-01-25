/******************************************************************************
** File Name:  gnssdaemon.c                                                      *
** Author:                                                                      *
** Date:                                                                      *
** Copyright:  2015-2020 by Spreadtrum Communications, Inc.                   * 
**             All rights reserved.                                           *
**             This software is supplied under the terms of a license         *
**             agreement or non-disclosure agreement with Spreadtrum.         *
**             Passing on and copying of this document, and communication     *
**             of its contents is not permitted without prior written         *
**             authorization.                                                 *
**description:
** 1 it maintenance socket server for SGPS,LCS,WIFI, and so on modules 
** 2 it check GNSS status and it recover from abnormal condition 
** 3 it add some funciton when GNSS HAL limit 
*
*******************************************************************************

*******************************************************************************
**                                Edit History                                *
**----------------------------------------------------------------------------*
** Date                  Name                      Description                *
** 2016-02-06            lihuai                    Create.                    *
******************************************************************************/
#include <time.h>
#include <stdio.h>   
#include <stdlib.h>   
#include <unistd.h>   
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cutils/sockets.h>
#include "gnssdaemon.h"


pgnssserver_t g_gnss_server;
static char confbuff[512];

/******************************************************************************
**  Description: it only printf signal  
**                                
**  Author:         
**  parameter:      
**                  
**                  
******************************************************************************/
static int gnss_socket_init(pgnssserver_t *pmanager)
{
    int i = 0;

    memset(pmanager, 0, sizeof(struct pgnssserver));
    
    for(i = 0; i< GNSS_LCS_CLIENT_NUM; i++)
    pmanager->client_fds[i] = -1;
	
    pmanager->listen_fd = socket_local_server(GNSS_LCS_SOCKET_NAME, ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
    if(pmanager->listen_fd < 0) 
    {
	GNSSDAEMON_LOGE(" gnss daemon cannot create local socket server");
	return -1;
    }
    
    pthread_mutex_init(&pmanager->client_fds_lock,NULL);
    return 0;
}

/******************************************************************************
**  Description: it only printf signal  
**                                
**  Author:         
**  parameter:      
**                  
**                  
******************************************************************************/
static void gnss_signals_handler(int sig)
{
    GNSSDAEMON_LOGE("get a signal %d, lgnore.", sig);
    exit(0);
}

 
/******************************************************************************
**  Description:    gnss daemon gignals deal with it  
**                  initialize signal handling.              
**  Author:         
**  parameter:      
**                  
**                  
******************************************************************************/
static void gnss_signals()
{
    struct sigaction act;

    memset(&act, 0, sizeof(act));
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    // it send by kill command(default) 
    SIGNAL(SIGTERM,act, gnss_signals_handler);
    //memory abnormal  
    SIGNAL(SIGBUS,act, gnss_signals_handler);
    //invad write/read memory 
    SIGNAL(SIGSEGV,act, gnss_signals_handler);
    //abort process 
    SIGNAL(SIGABRT,act, gnss_signals_handler);
    //  calculate happen eg error division 0, overflow float limit
    SIGNAL(SIGFPE, act,gnss_signals_handler);
    // the peer stop read ,write peer still send data
    SIGNAL(SIGPIPE,act,gnss_signals_handler);
}



static void gnss_update_fd(pgnssserver_t* pgnssclient, int fd)
{
	int i = 0;

    if(!pgnssclient) return;

    pthread_mutex_lock(&pgnssclient->client_fds_lock);
    for (i = 0; i < GNSS_LCS_CLIENT_NUM; i++)
   	{
		if(pgnssclient->client_fds[i] == -1) //invalid fd
		{
	    	pgnssclient->client_fds[i] = fd;
	    	GNSSDAEMON_LOGD(" i:%d fd: %d", i,fd);
	    	break;
		}
		else if(pgnssclient->client_fds[i] == fd)
		{
	    	GNSSDAEMON_LOGE("error happens :same fd:%d", fd);
	   	    break;
		}
   }
   pthread_mutex_unlock(&pgnssclient->client_fds_lock);
    
   if(i == GNSS_LCS_CLIENT_NUM)
   {
		GNSSDAEMON_LOGE("ERRORR client_fds is FULL");
		for(i =0; i < GNSS_LCS_CLIENT_NUM; i++)
		GNSSDAEMON_LOGE(" i:%d fd: %d", i,pgnssclient->client_fds[i]);
   }  
}


/******************************************************************************
**  Description:    gnss listen  all socket request   
**                               
**  Author:         
**  parameter:      
**                  
**                  
******************************************************************************/
static void *gnss_listen_thread(void *arg)
{
    pgnssserver_t *pgnss_server = (pgnssserver_t *)arg;

    if(!pgnss_server)
    {
		GNSSDAEMON_LOGE("unexcept NULL pmanager");
		exit(-1);
    }

    while(1)
    {
        int  i = 0;
		fd_set read_fds;
		int rc = 0;
		struct sockaddr addr;
		socklen_t alen;
		int fd;

		FD_ZERO(&read_fds);
		FD_SET(pgnss_server->listen_fd, &read_fds);
		
		rc = select(pgnss_server->listen_fd + 1, &read_fds, NULL, NULL, NULL);
		
		if(rc < 1) // min or equal zero
		{
			GNSSDAEMON_LOGE("select rc < 1  failed (%s)",strerror(errno));
			if (errno == EINTR)
	        {
				sleep(1);
			}
			continue;
		}
		else
		{
			if(FD_ISSET(pgnss_server->listen_fd, &read_fds)) 
        	{
				do 
            	{
					alen = sizeof(addr);
					fd = accept(pgnss_server->listen_fd, &addr, &alen);
					GNSSDAEMON_LOGD("%s got fd:%d from accept", GNSS_LCS_SOCKET_NAME, fd);
				} while (fd < 0 && errno == EINTR);

				if (fd < 0)
            	{
					GNSSDAEMON_LOGE("accept %s failed (%s)", GNSS_LCS_SOCKET_NAME,strerror(errno));
					sleep(1);
				}
				else
				{
					gnss_update_fd(pgnss_server,fd);
				}
		}
		
		}
	}
}

static  void daemon_send2gnss(pgnssserver_t *pgnss_server,char type,void* pData, unsigned short len)
{
    unsigned char data[GNSS_SOCKET_BUFFER_SIZE];
    unsigned char *pTmp = data;
    unsigned short sendLen = 0;
    int ret = 0;

    if(len > (GNSS_SOCKET_BUFFER_SIZE - 3))
    {
        GNSSDAEMON_LOGE(" input datalen too large:%d",len);
        return;
    }
    if(pgnss_server->gnssclient_fd < 0)
    {
    	 GNSSDAEMON_LOGE(" gnss lib not init ");
        return;
    }
    pthread_mutex_lock(&pgnss_server->client_fds_lock);
    memset(data,0,GNSS_SOCKET_BUFFER_SIZE);
    *pTmp++ = type;
	*pTmp++ =  (unsigned char)((len) >> 8); 
	*pTmp++ =  (unsigned char)(len);
	if(len > 0)
	{
    	memcpy(pTmp,pData,len);
    }
    sendLen = len + GNSS_SOCKET_DATA_HEAD;
    
	ret = TEMP_FAILURE_RETRY(write(pgnss_server->gnssclient_fd,data,sendLen));
	if(ret < 0)
	{
		GNSSDAEMON_LOGE("writes client_fd:%d fail (error:%s)",pgnss_server->gnssclient_fd, strerror(errno));
                pthread_mutex_unlock(&pgnss_server->client_fds_lock);
		return;
	}
	else
	{
		GNSSDAEMON_LOGD("write %d: bytes to client_fd:%d ",sendLen, pgnss_server->gnssclient_fd);
	}
    //release lock 
    pthread_mutex_unlock(&pgnss_server->client_fds_lock);
}

static void sgps_lcs_handle(int fd,void* pData, unsigned short len)
{
	unsigned char * ptmp = pData;
	char type,config;
	pgnssserver_t *pgnss_server = &g_gnss_server;
	
	if((len < SGPS_DAEMON_DATA_MIN) || (fd < 0))
    {
    	GNSSDAEMON_LOGE("the SGPS send data len it too short : %d", len);
    	return;
    }
    //slip $PSPRD,00, to type and data
    ptmp += SGPS_DAEMON_DATA_HEAD;
    type = (*ptmp++) -48;
    ptmp++; //slip, 
    config = (*ptmp) -48;
    GNSSDAEMON_LOGD(" $$$3 type: %d,config:%d",type,config);
    switch(type)
    {
    	case GNSS_CONTROL_SET_WORKMODE_TYPE:
    	case GNSS_CONTROL_SET_ASSERT_TYPE:
    	case GNSS_CONTROL_SET_MOLA_TYPE:
    	case GNSS_CONTROL_SET_TRIGGER_TYPE:
    	case GNSS_CONTROL_SET_LTE_TYPE:
    	{
    		daemon_send2gnss(pgnss_server,type,&config,1);
    		break;
    	}
    	default:
    	   GNSSDAEMON_LOGE("SGPS Request not supply type:%d",type);
    	   break;		  	
    }
    
}

static signed char get_angrygps_agpsmode(void)
{
    char ret = 0;
    FILE *angry = NULL;
    struct stat confstat;
    
    if(access("/data/data/com.android.angryGps",0) == -1)
    {
        GNSSDAEMON_LOGD("angry apk is not install");
        return -1;
    }

    GNSSDAEMON_LOGD("before open secgps.conf");
    stat("/data/data/com.android.angryGps/secgps.conf",&confstat);
    GNSSDAEMON_LOGD("config file size %d",(int)confstat.st_size);
    angry = fopen("/data/data/com.android.angryGps/secgps.conf","r");
    if(angry == NULL)
    {
        GNSSDAEMON_LOGD("can't open angry files,errno:%s",strerror(errno));
        return -1;
    }
    memset(confbuff,0,sizeof(confbuff));
    if(confstat.st_size > (long int)sizeof(confbuff))
    {
        GNSSDAEMON_LOGD("file size is too big");
        fclose(angry);
        return -1;
    }
    fread(confbuff,1,confstat.st_size,angry);
    //E("buff is %s",confbuff);
    if(strstr(confbuff,"STANDALONE") != NULL)
    {
        GNSSDAEMON_LOGD("standalone set in angrygps");
        ret = GPS_STANDALONE;
    }
    if(strstr(confbuff,"MSBASED") != NULL)
    {
        GNSSDAEMON_LOGD("msb set in angrygps");
        ret = GPS_MSB;
    }
    fclose(angry);
    return ret;
}
static void gnss_lcs_handle(int fd,unsigned char* pData, int len)
{
	unsigned char* pTmp = pData;
	unsigned char flag; 
	unsigned short datalen;
	signed char mode = 0;
    if(len > (GNSS_SOCKET_DATA_HEAD -1))
    {
    	flag = *pTmp++;
    	if('$' == flag) //SGPS lauch session 
    	{
    		GNSSDAEMON_LOGD("get data:%s",pData);
    		sgps_lcs_handle(fd,pData,len);
    	}
    	else  //gnss libg  lauch sesssion（get wifi,send geo and so on）
        {
        	unsigned char config;
        	datalen = *pTmp++;
        	datalen <<= 8;
        	datalen |= *pTmp++;
        	config = *pTmp;
        	if(datalen == (len - GNSS_SOCKET_DATA_HEAD))
        	{
        		switch(flag)
        		{
        			case GNSS_CONTROL_NOTIFIED_INITED_TYPE:
        			{
        				if(config)
        			    {
        			    	g_gnss_server.gnssclient_fd = fd;
        			    	GNSSDAEMON_LOGD(" save libgps fd:%d ",fd);
        			    }
        			    else
        			    {
        			    	g_gnss_server.gnssclient_fd = -1;
        			    	GNSSDAEMON_LOGD(" close libgps fd");
        			    }
        				break;
        			}
        			case GNSS_CONTROL_GET_WIFI_TYPE:
        			{
        				if(g_gnss_server.gnssclient_fd > 0)
        			    {
        			    	GNSSDAEMON_LOGD(" get wifi information and wirte wifi.xml");
    						if(system("wpa_cli IFNAME=wlan0 scan_results > /data/gnss/supl/wifi.xml") < 0)
           					 {
                                 GNSSDAEMON_LOGD("system get wifi info fail,%s",strerror(errno));
            				 }
    						 daemon_send2gnss(&g_gnss_server,GNSS_CONTROL_SET_WIFI_TYPE,NULL,0);
        			    }
        			    else
        			    {
        			       GNSSDAEMON_LOGE("gnssclient_fd not init");
        			    }
        				break;
        			}
        			case GNSS_CONTROL_GET_ANGRYGPS_AGPSMODE:
        			{
        				if(g_gnss_server.gnssclient_fd > 0)
        			    {
        			    	GNSSDAEMON_LOGD("get angrygps gpsmode from config file");
    						if((mode = get_angrygps_agpsmode()) < 0)
           					{
                                 GNSSDAEMON_LOGD("get angrygps mode fail");
            				}
            				else
            				{
    						    daemon_send2gnss(&g_gnss_server,GNSS_CONTROL_SET_ANGRYGPS_AGPSMODE,&mode,1);
    						}
        			    }
        			    else
        			    {
        			       GNSSDAEMON_LOGE("gnssclient_fd not init");
        			    }
        				break;
        			}
        			default:
            			GNSSDAEMON_LOGE("gnss client type error:%d",flag);
            			break;
        			
        		}
        	}
        	else
        	{
        		GNSSDAEMON_LOGE("len:%d, datalen:%d",len,datalen);
        	}
        }
    
    }
    
}



static void *gnss_server_handle(void *arg)
{
	int  len,i;
	unsigned char buffer[GNSS_SOCKET_BUFFER_SIZE];
	fd_set readset;
	int result, fdmax = 0;
	struct timeval timeout;
	pgnssserver_t *pgnssshandle = (pgnssserver_t *)arg;

	if(!pgnssshandle)
	{
		GNSSDAEMON_LOGE("unexcept NULL point");
		exit(-1);
	}

	while(1)
    {
		timeout.tv_sec = 3;
		timeout.tv_usec = 0;
		FD_ZERO(&readset);

        pthread_mutex_lock(&pgnssshandle->client_fds_lock);

		for(i = 0; i < GNSS_LCS_CLIENT_NUM; i++)
        {
			if(pgnssshandle->client_fds[i] > 0)
            {
				fdmax = pgnssshandle->client_fds[i] > fdmax ? pgnssshandle->client_fds[i] : fdmax;
				FD_SET(pgnssshandle->client_fds[i], &readset);
			}
		}
        pthread_mutex_unlock(&pgnssshandle->client_fds_lock);
		result = select(fdmax + 1, &readset, NULL, NULL, &timeout);
		if(result < 1)
        {  
        	if(result < 0)
        	{
            	GNSSDAEMON_LOGE("sockfd result < 0 error:%s", strerror(errno));
            }
            else
            {
        		sleep(1); 
        	} 
        }
		else
	    {
	    	for(i = 0; i < GNSS_LCS_CLIENT_NUM; i++)
        	{
        		int tmpfd = pgnssshandle->client_fds[i];
				if(tmpfd > 0)
            	{
             		if(FD_ISSET(tmpfd, &readset))
             		{
             			memset(buffer, 0, GNSS_SOCKET_BUFFER_SIZE);
						len = read(tmpfd,buffer,GNSS_SOCKET_BUFFER_SIZE);
						if(len > 0)
						{
							GNSSDAEMON_LOGD("sockfd:%d get %d bytes", tmpfd,len);

							/* Begin: for sgps test
							   sometimes data can't get by one time, need read
							   twice.*/
							{
								unsigned char* pTmp = buffer;
								unsigned char flag;
								int lentemp = 0;
								flag = *pTmp++;
								GNSSDAEMON_LOGD("sockfd first byte %c", flag);
								/* can't add len > (GNSS_SOCKET_DATA_HEAD -1)
								   condition,sometimes happen just receive $*/
								if ((len < SGPS_DAEMON_DATA_MIN)&&
									('$' == flag)) {
									memset(buffer + len, 0,
										GNSS_SOCKET_BUFFER_SIZE - len);
									lentemp =  read(tmpfd, buffer + len,
										GNSS_SOCKET_BUFFER_SIZE - len);
									if (lentemp > 0)
										len = len + lentemp;
									GNSSDAEMON_LOGD("get %d bytes finally",
									   len);
								}
							}
							/* End: for sgps test */

							gnss_lcs_handle(tmpfd, buffer,len);
						}
						else
					    {
					        GNSSDAEMON_LOGD("sockfd %d write len: %d ,%s", tmpfd, len,strerror(errno));
					        pgnssshandle->client_fds[i] = -1;
					        close(tmpfd);
					    }
					} //FD_ISSET
					
				} //tmpfd 

			}// for
	    
	   	 } //else 
	}//while(1)
}



/******************************************************************************
**  Description:    This daemon enter function 
**                                
**  Author:         
**  parameter:      
**                  
**                  
******************************************************************************/
int main(void)
{
    pthread_t client_id, gnsshandle_id;
    
    GNSSDAEMON_LOGE("gnss daemon begin to work in androido .");
    /* sets gnss daemon process's file mode creation mask */
    //it don't need fork and setsid , 
    // it start server(system/bin/*) on int.rc 
    umask(0);
    /* handle signal */
	gnss_signals();
    //create common socket server 
    gnss_socket_init(&g_gnss_server);
    
	if (pthread_create(&client_id, NULL, gnss_listen_thread, (void*)(&g_gnss_server)))
	{
		GNSSDAEMON_LOGE(" pthread_create (%s)", strerror(errno));
		return -1;
	}
    
	if (pthread_create(&gnsshandle_id, NULL, gnss_server_handle, (void*)(&g_gnss_server)))
	{
		GNSSDAEMON_LOGE(" pthread_create (%s)", strerror(errno));
		return -1;
	}
    
	while(1) {
		//now nothing to do 
		sleep(5);
		
	}

	return 0;

}

