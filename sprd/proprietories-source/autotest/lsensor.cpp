//#include "testitem.h"
//------------------------------------------------------------------------------
// enable or disable local debug
#define DBG_ENABLE_DBGMSG
#define DBG_ENABLE_WRNMSG
#define DBG_ENABLE_ERRMSG
#define DBG_ENABLE_INFMSG
#define DBG_ENABLE_FUNINF
#include "debug.h"
//------------------------------------------------------------------------------
#include "sensor.h"
#include "input.h"
#include "key_common.h"
static int thread_run;
static int proximity_value=1;
static int proximity_modifies=0;
static int light_value=0;
static int light_pass=0;
static int lpsensor_result=0;//// by yuebao
static int  lsensor_type ;
static int  psensor_type ;
static int cur_row = 2;

extern sensors_event_t sensorbuffer[SENSOR_NUMEVENTS];
extern uint32_t x_value;
extern uint32_t y_value;
extern int LPsensor;
extern int is_loader;
static int lsensor_enable(int enable)
{
	int fd;
	int ret = -1;

	LOGD("%s   enable=%d\n",__FUNCTION__,enable);
	fd = open(SPRD_PLS_CTL, O_RDWR);
	if(fd < 0 && 0 == strcmp(SPRD_PLS_CTL, "/dev/ltr_558als")) {
		/*try epl */
		fd = open("/dev/epl2182_pls", O_RDWR);
	}
	if(fd < 0) {
		LOGD("[%s]:open %s fail\n", __FUNCTION__, SPRD_PLS_CTL);
		return -1;
	}

	if(fd > 0) {
		if(ioctl(fd, LTR_IOCTL_SET_LFLAG, &enable) < 0) {
			LOGD("[%s]:set lflag %d fail, err:%s\n", __FUNCTION__, enable, strerror(errno));
			ret = -1;
		}
		if(ioctl(fd, LTR_IOCTL_SET_PFLAG, &enable) < 0) {
			LOGD("[%s]:set pflag %d fail, err:%s\n", __FUNCTION__, enable, strerror(errno));
			ret = -1;
		}
		close(fd);
	}

	return ret;
}
static void thread_exit_handler(int sig) {
  LOGD("receive signal %d , eng_receive_data_thread exit\n", sig);
  pthread_exit(0);
}


static void *lsensor_thread(void *param)
{
	int datanum = -1;
	int i;
	int cnt=0;
	  struct sigaction actions;
	    LOGD("autotest here while\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
		sigemptyset(&actions.sa_mask);
		actions.sa_flags = 0;
		actions.sa_handler = thread_exit_handler;
		sigaction(SIGUSR1, &actions, NULL);
	    datanum = SKD_sensorGetdata();
	    LOGD("autotest here afterpoll\n n = %d",datanum);
	    if (datanum< 0) {
		LOGD("autotest poll() failed\n");
	   lpsensor_result =1;
	   return NULL;
	    }
		LOGD("lsensor_type=%d\n",lsensor_type);
		LOGD("psensor_type=%d\n",psensor_type);
	    for (i = 0; i < datanum; i++) {
		LOGD("sensorbuffer[%d].type=%d\n",i,sensorbuffer[i].type);	
		if ((lsensor_type == sensorbuffer[i].type)&&(LPsensor==1)){
		    LOGD("autotest light value=<%5.1f>\n",sensorbuffer[i].light);
			x_value=sensorbuffer[i].light;
		    if(light_value != sensorbuffer[i].light){
			cnt++;
			if(cnt>=5)
			light_value = sensorbuffer[i].light;
		    }
		}

		if((psensor_type == sensorbuffer[i].type)&&(LPsensor==2)){
		    LOGD("autotest Proximity:%5.1f", sensorbuffer[i].distance);
			y_value=sensorbuffer[i].distance;
		    if(proximity_value != sensorbuffer[i].distance){
			proximity_modifies++;
			proximity_value = sensorbuffer[i].distance;
		    }
		}
	    }
	   lpsensor_result =0;
	   		LOGD("lsensor_thread end \n");	
	   return NULL;
}

int test_lsensor_start(void)
{
    int light_sensorid = -1;
    int light_handle = -1;
    int pro_handle = -1;
    int pro_sensorid = -1;
    pthread_t thread;
	void *status;
    const char *ptr = "Light Sensor";
    const char *ptr1 = "Proximity Sensor";
    proximity_value=1;
    proximity_modifies=0;
    light_value=0;
    INFMSG("  yuebao %s:\n", __func__);
	//if(is_loader<0){
	    is_loader = sensorOpen();
		if(is_loader <0)
		{
		LOGD("sensor open failed\n ");
		goto end;
	//	}
	}else{
		LOGD("sensor has opened\n ");
	}

	if(LPsensor==1)
	{
		lsensor_type = SKD_sensorgetid(5, &light_handle, &light_sensorid);
		if(-1 == lsensor_type)
		{
	      lpsensor_result = 1;
		  LOGD("Lsensor is not supported ");
		  goto end;
		}else{
			SKD_sensorActivate(light_handle, 1);	 
		} 

	}
	else if (LPsensor==2)
	{
		psensor_type = SKD_sensorgetid(8, &pro_handle, &pro_sensorid);
		if(-1 == psensor_type)
	    { 
		lpsensor_result = 1;
		  LOGD("LPsensor is not supported ");
		  goto end;
		}else{
		SKD_sensorActivate(pro_handle, 1);
		} 
		//LOGD(" sensorbuffer[i].type:%d\n", sensorbuffer[i].type);
	}
	  usleep(200);
	pthread_create(&thread, NULL, lsensor_thread, NULL);
	sleep(2);
	pthread_kill(thread,SIGUSR1);
	pthread_join(thread,NULL);
	if(LPsensor==1)
	{
		SKD_sensorActivate(light_handle, 0);
	}else if (LPsensor==2)
	{
	SKD_sensorActivate(pro_handle, 0);
	}
	sensorClose( );
	return lpsensor_result;
 
end:
	lpsensor_result=1;
	if(is_loader>0){
	sensorClose( );
	}
	LOGD("test failed \n ");
    return lpsensor_result;
}

