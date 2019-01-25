#include <sys/epoll.h>
#include <sys/socket.h>
#include <cutils/sockets.h>
#include <termios.h>
#include <hardware_legacy/power.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "gps_common.h"
#include "gps_cmd.h"
#include "gps_engine.h"
#include "gnssdaemon_client.h"
#include "gnss_log.h"


int daemon_state = thread_init;
char *gps_idle_on = "$PCGDC,IDLEON,1,*1\r\n";
char *gps_idle_off="$PCGDC,IDLEOFF,1,*1\r\n"; 
extern char *libgps_version;

static int angrygps_config(GpsState*  s)
{
    int ret = 0;
    FILE *angry = NULL;
    struct stat confstat;
    
    D("angrygps request begin");
    if(access("/data/data/com.android.angryGps",0) == -1)
    {
        D("angry apk is not install");
        return 0;
    }

    gnss_send2Daemon(s,GNSS_CONTROL_GET_ANGRYGPS_AGPSMODE,NULL,0);
    if(WaitEvent(&(s->gpsdsem),200) != 0)
    {
        E("wait angrygps event fail");
        return ret;
    }
    
    return ret;
}

void gps_daemon_thread( void*  arg )
{
    GpsState*  s = (GpsState*)arg;
    int i,ret;
    int   control_fd = s->control[1];
    struct epoll_event  events[5];
    int ne, nevents,fd;
    char  cmd = 0;
    int  epoll_fd = epoll_create(5);
    char error_time = 0;
    char cmd_buf[36];
    char sig_first = 0;
    char log_string[16];
    
	extern unsigned long send_length;
	extern unsigned long read_length;
    
    daemon_state = thread_run;
	
    epoll_register( epoll_fd, control_fd ); 

    D("gps daemon thread is enter");
    while(daemon_state)
    {
        nevents = epoll_wait( epoll_fd, events, 5, -1 );
        if (nevents < 0) {
            if (errno != EINTR)
            {
                E("epoll_wait() unexpected error: %s", strerror(errno));
                if(control_fd < 0)
                E("this is impossible for control fd is invalid");	
                error_time++;
                if(error_time > 10)
                goto threaddone;
            }
            continue;
        }

        for (ne = 0; ne < nevents; ne++) 
        {
            if((events[ne].events & (EPOLLERR|EPOLLHUP)) != 0) 
	        {
                E("EPOLLERR or EPOLLHUP after epoll_wait() !?");
                goto threaddone;
            }
            if((events[ne].events & EPOLLIN) != 0)
            {
                fd = events[ne].data.fd;

                if(fd == control_fd)
                {
                    D("gps control fd event");
                    do {
                        memset(cmd_buf,0,sizeof(cmd_buf));
                        i = 0;
                        ret = 1;
                        while((i < 35) && (ret > 0))
                        {
	                          i++;
                             ret = read(fd, &cmd_buf[i], 1 );  //cmd_buf[0] not used
                             D("for test,cmd_buf[%d]=%d,ret=%d",i,cmd_buf[i],ret);
                         }
				
                    } while (ret < 0 && errno == EINTR);
			 
                    if(i > 2)//get the last  two command 
                    {
                        if((CMD_STOP == cmd_buf[i-1])&&(CMD_CLEAN == cmd_buf[i-2]))
                        {
                            cmd = CMD_CLEAN;
                        }
                        else
                        {
                             cmd = cmd_buf[i-1];
                        }
                    }
                    else //it's only one command 
                    {
                        cmd = cmd_buf[i-1];
                    }

                    if ((cmd == CMD_START) && (!s->started)) 
                    {
                        char ptmp[60];
                        struct timeval rcv_timeout;
                        //if(s->init)
                        s->callbacks.acquire_wakelock_cb();
                        
                        D("gps_daemon_thread,CMD_START:%s",libgps_version);    
#ifdef GNSS_INTEGRATED
                        sharkle_gnss_switch_modules(); 
                        ret = sharkle_gnss_ready(1);
                        if(ret)
                        {
                            E("%s sharkle_gnss_ready eror",__FUNCTION__);  
                            continue;
                        }
#else
					    if(agpsdata->cp_ni_session && (1 != s->init))gps_hardware_open(); 

#endif
                        s->gps_flag = CMD_START;
 					    if((DEVICE_IDLED != s->powerState) && (DEVICE_SLEEP != s->powerState))
                        {
                            D("%s is recovering now ,so continue",__FUNCTION__);
                            continue;
                        }
                        ret = gps_wakeupDevice(s);
                        if(ret)
                        {
                            rcv_timeout.tv_sec = 5;
                            rcv_timeout.tv_usec = 0;
                            gnss_notify_abnormal(WCND_SOCKET_NAME,&rcv_timeout,"sleep-trigger");
                            
                            if(!s->release) //userdebug
                            {
                                gps_setPowerState(DEVICE_ABNORMAL);
                                createDump(s);
                                continue;
                            }
                            else
                            {
                                D("%s,gps_wakeupDevice  failed ",__FUNCTION__);
                                gps_setPowerState(DEVICE_RESET); 
                                continue;
                            }
                        }
                        if(sig_first == 0)
                        {
                            gps_sendData(SET_LTE_ENABLE,NULL); 							
                            sig_first = 1;
                        } 
                        send_length = 0;
                        read_length = 0;
                        s->IdlOffCount++;
                        s->started = 1;
#ifndef GNSS_INTEGRATED
                        if(s->powerctrlfd > 0)
                        {
                           ioctl(s->powerctrlfd,GNSS_LNA_EN,NULL);
                           
                           D("&&&& open LNA");
                        }
#endif                       
                       
                        //first check power state ,if should not keep sleep state 
                        read_slog_status();
                        memset(log_string,0,sizeof(log_string));
                        config_xml_ctrl(GET_XML,"LOG-ENABLE",log_string,CONFIG_TYPE);
                        if(!memcmp(log_string,"TRUE",strlen("TRUE")))
                        {
                            D("open ge2 log switch");
							s->logswitch = 1;
                        }
                        if(!memcmp(log_string,"FALSE",strlen("FALSE")))
                        {
							D("close ge2 log switch");
							s->logswitch = 0;
                        }

                        if(s->logswitch|| s->slog_switch|| s->postlog)
                        {
                            s->outtype =1;
                        }
                        else
                        {
                            s->outtype = 0;
                        }
                        
                        gps_sendData(SET_OUTTYPE,NULL);
                        
                        //gps_sendData(SET_NET_MODE,NULL);
                        gps_sendData(SET_CMCC_MODE, NULL);   //set cmcc mode
                        
                        memset(log_string,0,sizeof(log_string));
                        config_xml_ctrl(GET_XML,"REALEPH-ENABLE",log_string,CONFIG_TYPE);
                        if(!memcmp(log_string,"TRUE",strlen("TRUE")))
                        {
                            D("realeph switch open");
                            s->realeph = 1;
                            gps_sendData(SET_REALEPH,NULL); 
                        }
                        if(!memcmp(log_string,"FALSE",strlen("FALSE")))
                        {
                            D("realeph switch close");
                            s->realeph = 0;
                            gps_sendData(SET_REALEPH,NULL); 
                        }
                        memset(log_string,0,sizeof(log_string));
                        if(!config_xml_ctrl(GET_XML,"CP-MODE",log_string,CONFIG_TYPE))
                        {
                            D("in daemon:cp mode begin,cp_mode %s",log_string);
                            
                            s->cpmode = (log_string[0]-'0')*4 + (log_string[1]-'0')*2 + log_string[2]-'0'; 
                            if((s->cpmode == 5) && (s->workmode == GPS_BDS))
                            {
                                D("adjust mode from 5 to 3");
                                s->cpmode = 3;
                            }  
                            if((s->cpmode == 3) && (s->workmode == GPS_GLONASS))
                            {
                                D("adjust mode from 3 to 5");
                                s->cpmode = 5;
                            }                        
				            gps_sendData(SET_CPMODE,NULL);               
                        }
                        //store_nmea_log(log_start);
						gps_sendData(SET_INTERVAL, NULL);	 // CTS_TEST
                        D("gps Real send IDLEOFF is:%d:",s->IdlOffCount);
                        //angrygps_config(s);
                        gps_sendData(SET_NET_MODE,NULL);
                        test_gps_mode(s);  //for cs/hs/ws
                        if(s->fd < 0)
                        {
                            E("start:it is very horrible that uart not open or open fail!!!");
                            continue;
                        }
                        memset(ptmp,0,sizeof(ptmp));
						if((!config_xml_ctrl(GET_XML,"GE2-VERSION",ptmp,CONFIG_TYPE)) && (s->rftool_enable == 0))
                        {
                            //s->callbacks.nmea_cb(s->NmeaReader[0].fix.timestamp,
			                       // (const char*)ptmp,sizeof(ptmp));
			                savenmealog2Buffer((const char*)ptmp,sizeof(ptmp));
                        }
                        
                        ret = 1;
                        i = 0;
                        gps_setPowerState(DEVICE_WORKING);
                        while(ret && (i < 3))
                        {
                            i++;
                            gps_sendData(IDLEOFF_STAT, gps_idle_off);
                            ret =gps_power_Statusync(IDLE_OFF_SUCCESS);
                        }

                }
                if((cmd == CMD_STOP) && (s->started))
                {
                   D("=== cmd stop enter\n");
                   if(s->fd < 0)
                   {
                       E("stop:it is very horrible that ttyV not open or open fail!!!");
                       continue;
                   }
                   
                   ret = 1;
                   i = 0;
                   s->gps_flag = CMD_STOP;
                   s->started = 0;
                   s->IdlOnCount++;
				   s->cw_enable = 0;   // HW_CW
				   s->use_nokiaee = 0;
                   
                   //store_nmea_log(log_stop); 
                   D("gps Real send IDLON is:%d:",s->IdlOnCount);
				   gps_setPowerState(DEVICE_IDLING);
                   while(ret &&(i < 3))
                   {
                       gps_sendData(IDLEON_STAT, gps_idle_on);
                       ret = gps_power_Statusync(IDLE_ON_SUCCESS);
                   }
#ifndef GNSS_INTEGRATED
                   
                   if(s->powerctrlfd > 0)
                   {
                      ioctl(s->powerctrlfd,GNSS_LNA_DIS,NULL); 
                      D("&&&& disable LNA");
                   }
#endif
                   agps_release_data_conn(s);
#ifndef GNSS_INTEGRATED
                   if((1 != s->init)&& agpsdata->cp_ni_session)
                   {
                        if(DEVICE_IDLED == s->powerState)
                        {
 
					         gps_notiySleepDevice(s);
                        }
                         else
                         {
                             E("this is some error happy!");
                         }
			//gps_hardware_close();
                         agpsdata->cp_ni_session = 0;
                   }
#endif
			// SCREENOFF
			if((CMD_START !=s->gps_flag) && (s->sleepFlag) && 
                (DEVICE_IDLED == s->powerState)&&(s->screen_off == 1))
			{
				D("after gps stop ap send sleep to cp\n");
                if((s->postlog)||(s->cmcc))
                {
                     D("auto test ,so don't send cp sleep");
                }
                else
                {
                    if(!s->hotTestFlag)
                    {
                        D("gps_stop ,send cp sleep");
                        gps_notiySleepDevice(s);
                        usleep(10000);
                    }
                    else
                    {
                        D("gps_stop ,it can't send cp sleep when last cold-start");
                    }
                }
				s->keepIdleTime = 0;  			
			}
			
                   //if(s->init) 
                   s->callbacks.release_wakelock_cb();
                 
               }

               if(cmd == CMD_CLEAN)
               {
                  D("libgps cleanup \n");
                  if(s->started == 1)
                  {
                      E("=========>>>>>>>>>>>note:gps_stop isn't run!!!");                   
                      s->IdlOnCount++;
                      i = s->started = 0; 
                      ret = 1;
                      gps_setPowerState(DEVICE_IDLING);
                      while(ret &&(i < 3))
                      {
                          gps_sendData(IDLEON_STAT, gps_idle_on);
                          ret = gps_power_Statusync(IDLE_ON_SUCCESS);
                      }
                      s->callbacks.release_wakelock_cb();
#ifndef GNSS_INTEGRATED
                      if(s->powerctrlfd > 0)
                      {
                          ioctl(s->powerctrlfd,GNSS_LNA_DIS,NULL); 
                          D("&&&& disable LNA");
                      }
#endif
                  }
                  D("cleanup  send cp sleep");
                  if(DEVICE_SLEEP != s->powerState)
                  {
                     gps_notiySleepDevice(s); 
                     usleep(40000);//it dealy wait the cp ready 

                  }
                  gps_hardware_close();
                  if(s->hotTestFlag)
                  {
                      s->hotTestFlag = 0;
                      release_wake_lock("GNSSLOCK");
                      D("release_wake_lock GNSSLOCK ");
                  }
                  if(s->cmcc)
                  {
                     release_wake_lock("GNSSLOCK");
                     D(" cmcc cleanup release_wake_lock GNSSLOCK ");
                  }
                  sig_first = 0;
                  s->first_idleoff = 0;
                  s->screen_off = 0;   // SCREENOFF
                  ThreadSignalCond(s);
                  s->gps_flag = CMD_CLEAN;
               }


	           if (cmd == CMD_QUIT)
	              goto threaddone;
               }
             }
           }
		  
         }
threaddone:
	epoll_deregister(epoll_fd, control_fd);
	close(epoll_fd);   //this is very important
	E("gps thread quit,close all files\n");
	daemon_state = thread_stop;
	s->gps_flag = CMD_QUIT;
	return;
}


