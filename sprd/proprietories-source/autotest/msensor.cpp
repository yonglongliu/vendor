//------------------------------------------------------------------------------
//-------------------------add the new function msensor test case----------------------
//-------------------------20160729---------------------------------------------
#include "debug.h"
#include "sensor.h"
#include "input.h"
#include "key_common.h"
#include "math.h"
extern int is_loader;
static int msensor_type;
static int msensor_result = 0;
extern sensors_event_t sensorbuffer[SENSOR_NUMEVENTS];
extern uint32_t x_value;
extern uint32_t y_value;
extern uint32_t z_value;
extern int symbol_x;
extern int symbol_y;
extern int symbol_z;
extern int m_level;

static void thread_exit_handler(int sig) {
  LOGD("receive signal %d , eng_receive_data_thread exit\n", sig);
  pthread_exit(0);
}

static void *msensor_thread(void *param)
{
    int cnt=0;

    int i,n;
    int type_num = 0;
    char onetime=1;
    int x_count=0,y_count=0,z_count=0;
    char x_flag=0,y_flag=0,z_flag=0;
    sensors_event_t data_last={0};
	
		struct sigaction actions;
		sigemptyset(&actions.sa_mask);
		actions.sa_flags = 0;
		actions.sa_handler = thread_exit_handler;
		sigaction(SIGUSR1, &actions, NULL);
		n = SKD_sensorGetdata();
		LOGD("autotest here afterpoll\n n = %d",n);
	if (n < 0) {
	    INFMSG("bbat == poll() failed\n");
		msensor_result=1;
	   return NULL;
		
	}
	LOGD("amsensor_type = %d\n",msensor_type);
	for (i = 0; i < n; i++) {
	LOGD("sensorbuffer[%d].type = %d\n",i,sensorbuffer[i].type);	
	    if (msensor_type == sensorbuffer[i].type){
		LOGD("bbat == sensor value=<%5.1f,%5.1f,%5.1f>\n",sensorbuffer[i].data[0], sensorbuffer[i].data[1], sensorbuffer[i].data[2]);
		m_level=sensorbuffer[i].orientation.status;
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
		x_value=fabs(sensorbuffer[i].data[0]);
		y_value=fabs(sensorbuffer[i].data[1]);
		z_value=fabs(sensorbuffer[i].data[2]);
		
		if(onetime&&sensorbuffer[i].data[0]&&sensorbuffer[i].data[1]&&sensorbuffer[i].data[2]){
		    onetime=0;
		    data_last=sensorbuffer[i];
		}
		if(abs(data_last.data[0]-sensorbuffer[i].data[0])>20){
		    x_count++;
		}
		if(abs(data_last.data[1]-sensorbuffer[i].data[1])>20){
		    y_count++;

		}
		if(abs(data_last.data[2]-sensorbuffer[i].data[2])>20){
		    z_count++;

		}
		if((x_count == 1) || (y_count == 1) || (z_count == 1)){
		    INFMSG("x_count: %d|| y_count: %d|| z_count: %d!", abs(data_last.data[0]-sensorbuffer[i].data[0]), abs(data_last.data[1]-sensorbuffer[i].data[1]), abs(data_last.data[2]-sensorbuffer[i].data[2]));
		}
	    }
	}

		msensor_result=0;
				LOGD("msensor_thread end \n");	
	   return NULL;
}

int test_msensor_start(void)
{
    int ret = 0;
    const char *ptr = "Magnetic field Sensor";
    pthread_t thread;
    int handle = -1;
    int sensorid = -1;

  //  if(is_loader < 0){
		is_loader = sensorOpen();
	    if(is_loader < 0){
		LOGD("open sensor failed");
		goto end;
	//	}
    }else{
		LOGD("sensor has opened\n ");
	}

	msensor_type= SKD_sensorgetid(2, &handle, &sensorid);
	if(-1 == msensor_type ){
		msensor_result = 1;
		LOGD("asensor is not supported ");
	    goto end;
	}		
	//enable msensor
	ret = SKD_sensorActivate(handle, 1);
	INFMSG("test msensor ID is %d~", ret);
	if(ret < 0){
	LOGD("active sensor failed");
	msensor_result = 1;
	    goto end;
	}
	usleep(200);
	pthread_create(&thread, NULL, msensor_thread, NULL);
	sleep(2);
	pthread_kill(thread,SIGUSR1);
    pthread_join(thread,NULL);
	INFMSG("testSensor test msensor_result=%d\n", __func__,msensor_result);
	sensorClose( );
	return msensor_result;
    	

end:		
	msensor_result = 1;
	if(is_loader>0){
	sensorClose( );
	}
	LOGD("test failed");
    return msensor_result;//++++++++++++++++
}

