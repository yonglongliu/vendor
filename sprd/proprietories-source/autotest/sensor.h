//
// Spreadtrum Auto Tester
//
//
#ifndef _SENSOR_20150728_H__
#define _SENSOR_20150728_H__

#include "type.h"
#include <dlfcn.h>
#include <fcntl.h>

#include <linux/input.h>
#include <sys/ioctl.h>
//#include "testitem.h"
//#include "resource.h"
#include "./res/string_cn.h"
#include "ui.h"
#include <string.h>
#include <asm/types.h>
#include <binder/IServiceManager.h>
#include <gui/Sensor.h>
#include <gui/SensorManager.h>
#include <gui/SensorEventQueue.h>
#include <hardware/sensors.h>
#include  <pthread.h>
#include <time.h>

//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//-----------------------------------------------------------------------------
#ifndef SENSOR_TYPE_ACCELEROMETER

#define SENSOR_TYPE_ACCELEROMETER       1
#define SENSOR_TYPE_MAGNETIC_FIELD      2
#define SENSOR_TYPE_GYROSCOPE           4
#define SENSOR_TYPE_LIGHT               5
#define SENSOR_TYPE_PROXIMITY           8


#endif // SENSOR_TYPE_ACCELEROMETER

int sensorOpen( void );

int sensorActivate( int type );

int sensorClose( void );

int find_input_dev(int mode, const char *event_name);
int SKD_sensorActivate(int handle, int num);
int SKD_sensorgetid(int sensor_type, int *handle, int *sensorid);
int SKD_sensorGetdata(void);
#ifdef __cplusplus
}
#endif // __cplusplus
//-----------------------------------------------------------------------------

enum SKD_RESULT{
    RESULT_PASS = 0,
    RESULT_FAIL = 1,
    RESULT_AGAIN = 2,
    RESULT_NS = 3,
};
/*add the acc sensor ,light sensor   magnetic sensor,Presse sensor , printfinger sensor*/
enum  SENSOR_ITEM{
    ITME_NONE = 0,
    ITEM_ASENSOR = 0x01,
    ITEM_LSENSOR = 0x02,
    ITEM_GSENSOR = 0x03,
    ITEM_MSENSOR = 0x04,
    ITEM_PSENSOR = 0x05,
    ITEM_PFSENSOR = 0x06,
};

/*****************Gsensor*************/
#define SPRD_GSENSOR_DEV					"/dev/lis3dh_acc"
#define GSENSOR_IOCTL_BASE				 77
#define GSENSOR_IOCTL_GET_XYZ         		 _IOW(GSENSOR_IOCTL_BASE, 22, int)
#define GSENSOR_IOCTL_SET_ENABLE      		 _IOW(GSENSOR_IOCTL_BASE, 2, int)
#define LIS3DH_ACC_IOCTL_GET_CHIP_ID          _IOR(GSENSOR_IOCTL_BASE, 255, char[32])

#define SPRD_GSENSOR_OFFSET					0.2
#define SPRD_GSENSOR_1G						2
#define SPRD_ASENSOR_OFFSET					1.5
#define SPRD_ASENSOR_1G						10

#define GSENSOR_TIMEOUT                     			20
#define LSENSOR_TIMEOUT						2
#define ASENSOR_TIMEOUT					2
#define SENSOR_TIMEOUT					2
#define SENSOR_NUMEVENTS					16
/***************end gsensor************/

/**************Lsensor**********/
#ifndef SC9630
#define SPRD_PLS_CTL				"/dev/ltr_558als"
#define SPRD_PLS_LIGHT_THRESHOLD	30
#define SPRD_PLS_INPUT_DEV			"alps_pxy"
#else
#define SPRD_PLS_CTL				"/dev/epl2182_pls"
#define SPRD_PLS_LIGHT_THRESHOLD	1
#define SPRD_PLS_INPUT_DEV			"proximity"
#endif

#define LTR_IOCTL_MAGIC             0x1C
#define LTR_IOCTL_GET_PFLAG     _IOR(LTR_IOCTL_MAGIC, 1, int)
#define LTR_IOCTL_GET_LFLAG     _IOR(LTR_IOCTL_MAGIC, 2, int)
#define LTR_IOCTL_SET_PFLAG     _IOW(LTR_IOCTL_MAGIC, 3, int)
#define LTR_IOCTL_SET_LFLAG     _IOW(LTR_IOCTL_MAGIC, 4, int)
#define LTR_IOCTL_GET_DATA      _IOW(LTR_IOCTL_MAGIC, 5, unsigned char)
/**************end Lsensor********/
#endif // _SENSOR_20130123_H__
