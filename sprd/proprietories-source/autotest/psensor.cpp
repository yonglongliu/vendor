//------------------------------------------------------------------------------
//-------------------------add the new function psensor test case----------------------
//-------------------------20160729---------------------------------------------
#include "debug.h"
#include "sensor.h"
#include "input.h"
#include "key_common.h"

extern int is_loader;
static int psensor_type;
static int psensor_result = 0;
extern sensors_event_t sensorbuffer[SENSOR_NUMEVENTS];

static float pressure_value=0.0;
static int pressure_pass=0;
extern uint32_t x_value;
extern int is_loader;
static void thread_exit_handler(int sig) {
  LOGD("receive signal %d , eng_receive_data_thread exit\n", sig);
  pthread_exit(0);
}
static void *psensor_thread(void *param)
{
    int cnt=0;
    int i,n;
    pressure_pass = 0;
		struct sigaction actions;
		sigemptyset(&actions.sa_mask);
		actions.sa_flags = 0;
		actions.sa_handler = thread_exit_handler;
		sigaction(SIGUSR1, &actions, NULL);
	n = SKD_sensorGetdata();
	LOGD("autotest here afterpoll\n n = %d",n);
	if (n < 0) {
	    LOGD("bbat poll() failed\n");
		psensor_result=1;
	   return NULL;
	}
	LOGD("psensor_type = %d\n",psensor_type);
	for (i = 0; i < n; i++) {
		LOGD("sensorbuffer[%d].type = %d\n",i,sensorbuffer[i].type);
	    if ((psensor_type==  sensorbuffer[i].type)){
		
		x_value=sensorbuffer[i].pressure*1000;
		LOGD("bbat pressure value=<%5.1f> x_value:%d  \n", sensorbuffer[i].pressure,x_value);
		if((pressure_value !=  sensorbuffer[i].pressure) && (900 <=  sensorbuffer[i].pressure) && (1100 >=  sensorbuffer[i].pressure))
		    cnt++;
		if(cnt>=1)
		 pressure_pass = 1;
		pressure_value =  sensorbuffer[i].pressure;
	    }

	}
		psensor_result=0;
		LOGD("psensor_thread end \n");	
	   return NULL;
}

int test_psensor_start(void)
{
    int ret = 0;
    const char *ptr = "Pressure";
    pthread_t thread;
    int psensor_handle = -1;
    int psensor_sensorid = -1;
   // if(is_loader< 0){
		is_loader = sensorOpen();
	    if(is_loader< 0){
		LOGD("open sensor failed\n");
		goto end;
		//}
    }else{
		LOGD("sensor has opened\n ");
	}

	//enable pressure sensor
	psensor_type = SKD_sensorgetid(6, &psensor_handle, &psensor_sensorid);
	//sensor_id = (ptr);
	LOGD("bbat  test Psensor ID is %d~ the type %d\n", psensor_sensorid, psensor_type);
	if(psensor_type < 0){
	   LOGD("psensor_type < 0\n ");
	    goto end;
	}
	SKD_sensorActivate(psensor_handle, 1);
	usleep(200);
	pthread_create(&thread, NULL, psensor_thread, NULL);
	sleep(2);
  pthread_kill(thread,SIGUSR1);
    pthread_join(thread,NULL);
    SKD_sensorActivate(psensor_handle, 0);
	INFMSG("testSensor %s psensor_result=%d\n", __func__,psensor_result);
	sensorClose( );
	 return psensor_result;	
end:
	LOGD("test failed \n ");
	psensor_result=1;
	if(is_loader>0){
	sensorClose( );
	}
    return psensor_result;
}