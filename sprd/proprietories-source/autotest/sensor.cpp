// 
// Spreadtrum Auto Tester
//
// anli   2013-01-23
//
#include "type.h"
#include "sensor.h"


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--namespace sci_snsr {
using namespace android;
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//------------------------------------------------------------------------------
// enable or disable local debug
#define DBG_ENABLE_DBGMSG
#define DBG_ENABLE_WRNMSG
#define DBG_ENABLE_ERRMSG
#define DBG_ENABLE_INFMSG
#define DBG_ENABLE_FUNINF
#include "debug.h"
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static struct sensors_module_t      * sSensorModule = NULL;
static struct sensors_poll_device_t * sSensorDevice = NULL;
static struct sensor_t const        * sSensorList   = NULL;
static int                            sSensorCount  = 0;
static sp<SensorEventQueue>           sQue;
int is_loader = -1;
sensors_event_t sensorbuffer[SENSOR_NUMEVENTS];
//------------------------------------------------------------------------------

void *get_sensor_data(void *data);
static int  judge_sensor_available(void);

static void thread_exit_handler(int sig) {
  INFMSG("receive signal %d , pthread_exit exit\n", sig);
  pthread_exit(0);
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int sensorOpen( void )
{
    FUN_ENTER;
    int err = -1;
    if( NULL != sSensorModule ) {
	AT_ASSERT( NULL != sSensorDevice );
	DBGMSG("already init!\n");
	return 0;
    }
    err = hw_get_module(SENSORS_HARDWARE_MODULE_ID,
	    (hw_module_t const**)&sSensorModule);
    if( 0 == err ) {
	err = sensors_open(&sSensorModule->common, &sSensorDevice);
	if( err ) {
	    ERRMSG("sensors_open error: %d\n", err);
	    return -1;
	}

	sSensorCount = sSensorModule->get_sensors_list(sSensorModule, &sSensorList);
	DBGMSG("sensor count = %d\n", sSensorCount);
#if 0
	for( int i = 0; i < sSensorCount; ++i ) {
	    DBGMSG("sensor[%d]: name = %s, type = %d\n", i, sSensorList[i].name,
		    sSensorList[i].type);

	    sSensorDevice->activate(sSensorDevice, sSensorList[i].handle, 0);
	}
#endif
    } else {
	ERRMSG("hw_get_module('%s') error: %d\n", SENSORS_HARDWARE_MODULE_ID, err);
	return -2;
    }

    FUN_EXIT;
    return 0;
}

//>>--------------------------------------------------------------------------
//Judges sensor is available
static int judge_sensor_available(void)
{
    FUN_ENTER;
    pthread_t thread;
    int thread_stats = -1;
    int num = 0;
    //>>add a thread for get the data from activate sensor
    pthread_create(&thread, NULL, get_sensor_data, &thread_stats);
    //close the thread method
    while(++num< 20){
	usleep(100*1000);
	if(0 == thread_stats ){
	    pthread_join(thread, NULL);
	    break;
	}
    }
    if(-1 == thread_stats){
	if (pthread_kill(thread, SIGUSR1) != 0) {
	    ERRMSG("pthread_kill get_sensor_data thread error\n");
	    return -1;
	}
	pthread_join(thread, NULL);
    }
    FUN_EXIT;
    return thread_stats;
}

void *get_sensor_data(void *data)
{
    FUN_ENTER;
    int n;
    int *realdata = (int *)data;
	 //add the thread signal funciton
    struct sigaction actions;
    memset(&actions, 0, sizeof(actions));
    sigemptyset(&actions.sa_mask);
    actions.sa_flags = 0;
    actions.sa_handler = thread_exit_handler;
    sigaction(SIGUSR1, &actions, NULL);
    //>>add the framework struct  add the variable parameter.
    //static const size_t numEvents = 16;
    //sensors_event_t buffer[numEvents];

    n = sSensorDevice->poll(sSensorDevice, sensorbuffer, SENSOR_NUMEVENTS);
    if (n < 0) {
	INFMSG("get_sensor_data poll() failed\n");
    }else {
	*realdata = 0;
    }
    FUN_EXIT;
    return NULL;
}

//------------------------------------------------------------------------------
int sensorActivate( int type )
{
    FUN_ENTER;

    int i ;
    int ret = -1;
    int handle = -1;

    for( i = 0; i < sSensorCount; ++i ) {
	if( type == sSensorList[i].type ) {
	    handle = sSensorList[i].handle;
	    break;
	}
    }
    //sSensorDevice->activate(sSensorDevice, handle, 0);
    if( handle >= 0 ) {
	//add the activate the sensor interface from the framework
	ret = sSensorDevice->activate(sSensorDevice, handle, 1);
	if (0==ret)
	 {
	    INFMSG("sensor  activate!!!!!!the sucess !!!\n");
	    //device->batch(device, handle, 1, ms2ns(50), 0);
	    //>>add the judge_sensor whether available funciton add a program thread
	    if(0 == judge_sensor_available()){
		ret = 0;
	    }else{
		ret  = -1;
		INFMSG("sensor poll the data fail !!!!!!the fail type =%d\n", type);
	    }
	    ret = sSensorDevice->activate(sSensorDevice, handle, 0);
	    if (ret< 0)
	    {
		INFMSG("sensor disable fail !!!!\n");
		ret = -1;
	    }else {
		ret = 0;
		INFMSG("Activate: type = %d, ret = %d\n", type, ret);

	    }
	 }
	}
    FUN_EXIT;
    return ret;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int sensorClose( void )
{
    FUN_ENTER;

    if( NULL != sSensorModule ) {
	//dlclose(sSensorModule->common.dso);
	sensors_close(sSensorDevice);

	sSensorModule = NULL;
	sSensorDevice = NULL;
    }
    FUN_EXIT;
    return 0;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//>>add the new plan to modify the skd mode test senor
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//>>------------------------------------------------------------------
//get the sensor id and handle function
int SKD_sensorgetid(int sensor_type, int *handle, int *sensorid)
{
    int i ;
    //int handle = -1;
    int tmp_type   = -1;
    int *tmp_handle = (int *)handle;
    int *tmp_sensorid = (int *)sensorid;
	for( i = 0; i < sSensorCount; ++i) {
	INFMSG("sSensorList[%d].type=%d\n", i,sSensorList[i].type);
	}
    for( i = 0; i < sSensorCount; ++i) {
	if( sSensorList[i].type == sensor_type) {
	    *tmp_handle = sSensorList[i].handle;
	    *tmp_sensorid = i;
	    tmp_type = sSensorList[i].type;
	    break;
	}
    }
    if(-1 == tmp_type)
	INFMSG("The sensor of %s is not support !!!!\n");

    INFMSG("the  SKD_sensorgetid *tmp_handle=%d,*tmp_sensorid = %d    tmp_type = %d, \n",*tmp_handle ,*tmp_sensorid ,tmp_type);
    return tmp_type;
}

int SKD_sensorActivate(int handle, int num)
{
    int ret = -1;
    struct sensors_poll_device_1 * dev = (struct sensors_poll_device_1 *)sSensorDevice;
    if(1 == num){
	sSensorDevice->activate(sSensorDevice, handle, 0);
	usleep(200*1000);
    }
    ret = sSensorDevice->activate(sSensorDevice, handle, num);
    if (ret< 0)
    {
	INFMSG("sensor disable fail !!!!\n");
	ret = -1;
    }else {
	ret = 0;
	//INFMSG("Activate:  ret = %d\n", ret);
    }
    //dev->flush(dev, handle);
    //dev->batch(dev, handle, 1, ms2ns(1000), 0);
    return ret;
}

int SKD_sensorGetdata( void)
{
    FUN_ENTER;
    int datanum = -1;
    //>>add the framework struct  add the variable parameter.
    memset(sensorbuffer, 0 ,sizeof(sensorbuffer));
    datanum = sSensorDevice->poll(sSensorDevice, sensorbuffer, SENSOR_NUMEVENTS);
	for(int i=0;i<datanum;i++)
	{
	INFMSG("get data sensorbuffer[%d].type=%d\n", i,sensorbuffer[i].type);
	INFMSG("autotest light value=<%5.1f>\n",sensorbuffer[i].light);
	}
    if (datanum< 0) {
	INFMSG("get_sensor_data poll() failed\n");
    }
    FUN_EXIT;
    return datanum;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--} // namespace
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
