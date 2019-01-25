
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

extern sensors_event_t sensorbuffer[SENSOR_NUMEVENTS];
extern int is_loader;
int x_pass, y_pass, z_pass;
static int gsensor_result = 0;
	int  gsensor_type ;

static void thread_exit_handler(int sig) {
  LOGD("receive signal %d , eng_receive_data_thread exit\n", sig);
  pthread_exit(0);
}	
	
	
	
static int gsensor_open(void)
{
    int fd;
    int enable = 1;

    fd = open(SPRD_GSENSOR_DEV, O_RDWR);
    LOGD("Open %s fd:%d", SPRD_GSENSOR_DEV, fd);
    if (ioctl(fd, GSENSOR_IOCTL_SET_ENABLE, &enable)) {
        LOGD("Set G-sensor enable error: %s", strerror(errno));
        close(fd);
        return -1;
    }
    return fd;
}

static int gsensor_close(int fd)
{
    int enable = 0;
	if(fd > 0) {
		if (ioctl(fd, GSENSOR_IOCTL_SET_ENABLE, &enable)) {
			LOGD("Set G-sensor disable error: %s", strerror(errno));
		}
		close(fd);
	}
    return 0;
}

static int gsensor_get_devinfo(int fd, char * devinfo)
{
    if (fd < 0 || devinfo == NULL)
        return -1;

    if (ioctl(fd, LIS3DH_ACC_IOCTL_GET_CHIP_ID, devinfo)) {
		LOGD("[%s]: Get device info error. %s]", __func__, strerror(errno));
    }
	return 0;
}

static int gsensor_get_data(int fd, int *data)
{
    int tmp[3];
    int ret;

    ret = ioctl(fd, GSENSOR_IOCTL_GET_XYZ, tmp);
    data[0] = tmp[0];
    data[1] = tmp[1];
    data[2] = tmp[2];
    return ret;
}

static int gsensor_check(int data)
{
	int ret = -1;
	int start_1 = SPRD_GSENSOR_1G;
	int start_2 = -SPRD_GSENSOR_OFFSET;

	if( ((start_1<data) || (start_2>data)) ){
		ret = 0;
	}

	return ret;
}


static void *gsensor_thread(void *param)
{
	int  datanum;
	int i;
	//sensors_event_t data;
	float x_value, y_value, z_value;
	x_pass = y_pass = z_pass = 0;
		struct sigaction actions;
		sigemptyset(&actions.sa_mask);
		actions.sa_flags = 0;
		actions.sa_handler = thread_exit_handler;
		sigaction(SIGUSR1, &actions, NULL);	
	
	datanum =SKD_sensorGetdata();
	LOGD("autotest here afterpoll\n n = %d",datanum);
	if (datanum < 0) {
	    INFMSG("bbat == poll() failed\n");
		gsensor_result=1;
	   return NULL;
		
	}
		LOGD("gsensor_type=%d\n",gsensor_type);
		for (i = 0; i < datanum; i++) {
		LOGD("sensorbuffer[%d].type=%d\n",i,sensorbuffer[i]);
		if (gsensor_type == sensorbuffer[i].type){
		    if(x_pass == 0 && gsensor_check(sensorbuffer[i].data[0]) == 0) {
			x_pass = 1;
			INFMSG("[%s] x_pass", __func__);
		    }

		    if(y_pass == 0 && gsensor_check(sensorbuffer[i].data[1]) == 0) {
			y_pass = 1;
			INFMSG("[%s] y_pass", __func__);
		    }

		    if(z_pass == 0 && gsensor_check(sensorbuffer[i].data[2]) == 0) {
			z_pass = 1;
			INFMSG("[%s] z_pass", __func__);
		    }
		}
	}
			gsensor_result=0;
		LOGD("gsensor_thread end \n");	
	   return NULL;

}

int test_gsensor_start(void)
{	int fd;
	int counter;
	int data[3];

	int ret = -1;

	int handle = -1;
	int sensorid = -1;
	pthread_t thread;
	const char *ptr = "Gyroscope Sensor";

   // if(is_loader< 0){
		is_loader = sensorOpen();
		if(is_loader <0)
		{
		LOGD("sensor open failed\n ");
		goto end;
	//	}
	}else{
		LOGD("sensor has opened\n ");
	}
	gsensor_type = SKD_sensorgetid(4, &handle, &sensorid);
	INFMSG("SKD_sensorgetid handle= %d   sensorid = %d\n", handle,sensorid);
	if(-1 == gsensor_type ){
		gsensor_close(fd);
		goto end;
	}
	 SKD_sensorActivate(handle, 1);
      INFMSG("  yuebao %s:entry\n", __func__);
     usleep(200);
	pthread_create(&thread, NULL, gsensor_thread, NULL);
	sleep(2);
	pthread_kill(thread,SIGUSR1);
    pthread_join(thread,NULL);

	INFMSG("testSensor %s gsensor_result=%d\n", __func__,gsensor_result);
	gsensor_close(fd);
	SKD_sensorActivate(handle, 0);
	sensorClose( );
	return gsensor_result;//++++++++++++++++
end:
	gsensor_result = 1;//++++++++++
	if(is_loader>0){
	sensorClose( );
	}
	INFMSG(" test fail go end \n");
	return gsensor_result;//++++++++++++++++
}

