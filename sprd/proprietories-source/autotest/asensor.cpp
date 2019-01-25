//>>---------------------------------------------------------------------------
//add the asensor test funtion

#include "debug.h"
//------------------------------------------------------------------------------
#include "sensor.h"
#include "input.h"
#include "key_common.h"
#include "math.h"

static int sensor_id;
static sensors_event_t data;
static int x_pass, y_pass, z_pass;
static int  asensor_type ;
static int asensor_result;
extern uint32_t x_value;
extern uint32_t y_value;
extern uint32_t z_value;
extern int symbol_x;
extern int symbol_y;
extern int symbol_z;
// add the global the state
const size_t gnumEvents = 16;
extern sensors_event_t sensorbuffer[gnumEvents];
extern int is_loader;

static void thread_exit_handler(int sig) {
  LOGD("receive signal %d , eng_receive_data_thread exit\n", sig);
  pthread_exit(0);
}
static void *asensor_thread(void *param)
{
    int i,n;
    x_pass = y_pass = z_pass = 0;
	LOGD("autotest here while\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
	struct sigaction actions;
		sigemptyset(&actions.sa_mask);
		actions.sa_flags = 0;
		actions.sa_handler = thread_exit_handler;
		sigaction(SIGUSR1, &actions, NULL);
	n = SKD_sensorGetdata();
	LOGD("autotest here afterpoll\n n = %d",n);

	if (n < 0) {
	    LOGD("autotest poll() failed\n");
		asensor_result=1;
	   return NULL;
	}
	LOGD("asensor_type=%d\n",asensor_type);
	for (i = 0; i < n; i++) {
		LOGD("sensorbuffer[%d].type=%d\n",i,sensorbuffer[i].type);
	    if (asensor_type== sensorbuffer[i].type){
		LOGD("autotest value=<%5.1f,%5.1f,%5.1f>\n",sensorbuffer[i].data[0], sensorbuffer[i].data[1], sensorbuffer[i].data[2]);
		if(sensorbuffer[i].data[0]<0)
			symbol_x=1;	
		else
			symbol_x=0;
		if(sensorbuffer[i].data[1]<0)
			symbol_y=1;
		else
			symbol_y=0;
		if(sensorbuffer[i].data[2]<0)
			symbol_z=1;
		else
			symbol_z=0;
		x_value=fabs(sensorbuffer[i].data[0])*1000;
		y_value=fabs(sensorbuffer[i].data[1])*1000;
		z_value=fabs(sensorbuffer[i].data[2])*1000;

	    }
	}
	asensor_result=0;	
	LOGD("aasensor_thread end \n");	
	return NULL;
}

int test_asensor_start(void)
{
    pthread_t thread;
    const char *ptr = "Accelerometer Sensor";
    int ret;
    int err;
    int handle = -1;
    int sensorid = -1;

   // if(is_loader< 0){
		is_loader = sensorOpen();
	    if(is_loader < 0){
		LOGD("Open Sensor Failed!! ");
		goto end;
	//	}
	}else{
		LOGD("sensor has opened\n ");
	}

	//enable asensor
	asensor_type = SKD_sensorgetid(1, &handle, &sensorid);
	if(asensor_type < 0){
		LOGD("asensor is not supported ");
	    goto end;
	}
	SKD_sensorActivate(handle, 1);
	usleep(200);
	//asensor_result=asensor_thread();
   pthread_create(&thread, NULL, asensor_thread, NULL);
	sleep(2);
  pthread_kill(thread,SIGUSR1);
 // pthread_cancel(thread);
  pthread_join(thread,NULL);
	SKD_sensorActivate(handle, 0);
	sensorClose( );
	return asensor_result;
end:
	LOGD("test fail ");
	if(is_loader>0){
	sensorClose( );
	}
	asensor_result = 1;
    return asensor_result;
}

