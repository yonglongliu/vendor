// 
// Spreadtrum Auto Tester
//
// anli   2012-11-21
//
#ifndef _FINGER_SENSOR_20121121_H__
#define _FINGER_SENSOR_20121121_H__

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//#define DIAG_PKT_FORMT_IS_BIG_ENDIAN
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
//-----------------------------------------------------------------------------
#include "type.h"
#include "type.h"
#include <dlfcn.h>
#include <fcntl.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <string.h>
#include <debug.h>
extern int finger_sensor_test_sharkl2();
extern int finger_sensor_test_whale2();

//------------------------------------------------------------------------------
//--};
#ifdef __cplusplus
}
#endif // __cplusplus
//------------------------------------------------------------------------------

#endif // _DIAG_20121121_H__
