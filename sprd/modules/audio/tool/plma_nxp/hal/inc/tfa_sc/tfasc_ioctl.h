/************************************************************************
 *
 *  Description:
 *    Defines the IOCTL based programming interface exposed by the kernel-mode driver.
 *    This file conforms to C standard. It can be used by C and C++ code.
 *
 *    This file is Windows specific.
 *
 *  Runtime Env.: Win32 / NT Kernel
 *
 *  Author(s):   
 *    Udo Eberhardt,  Udo.Eberhardt@thesycon.de
 *                
 *  Companies:
 *    Thesycon GmbH
 *                
 ************************************************************************/

#ifndef __tfasc_ioctl_h__
#define __tfasc_ioctl_h__

// common version definition
#include "TfascApi_defs.h"


//
// Version of the software interface exported by the driver.
//
// EITHER:
// This will be incremented if changes are made at the programming interface level.
// The following convention exists:
// If changes are made to the programming interface that are compatible with
// previous versions then the minor version number (low order byte) will be incremented.
// If changes are made that cause an incompatibility with previous versions of the 
// interface then the major version number (high order byte) will be incremented.
#if 0
#define TFASC_INTERFACE_VERSION_MJ   0
#define TFASC_INTERFACE_VERSION_MN   10
#else
// OR:
// We set interface = implementation version to couple DLL/API and driver tightly
#define TFASC_INTERFACE_VERSION_MJ   1
#define TFASC_INTERFACE_VERSION_MN   0
#endif


#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define STR_HELPER_W(x) L#x
#define STR_W(x) STR_HELPER_W(x)


//
// we export a fixed device name
//
#define TFASC_DEVICE_NAME			 "tfasc_device"
#define TFASC_DEVICE_NAME_W		L"tfasc_device"

#define TFASC_DEVICE_NAME_KERNEL_W		L"tfasc_device_" STR_W(TFASC_INTERFACE_VERSION_MJ) L"_" STR_W(TFASC_INTERFACE_VERSION_MN)



///////////////////////////////////////////////////////////////////
// Generic IOCTL Requests
///////////////////////////////////////////////////////////////////

// Retrieve driver version info.
#define IOCTL_GENERIC_GET_DRIVER_INFO                      TFASC_IOCTL_CODE(0x001,METHOD_BUFFERED)
// InBuffer:  NULL
// OutBuffer: GenericDriverInfo


// Retrieve current debug settings
#define IOCTL_GENERIC_GET_DEBUG_SETTINGS                   TFASC_IOCTL_CODE(0x002,METHOD_BUFFERED)
// InBuffer:  NULL
// OutBuffer: GenericDriverDebugSettings


// Modify current debug settings
#define IOCTL_GENERIC_SET_DEBUG_SETTINGS                   TFASC_IOCTL_CODE(0x003,METHOD_BUFFERED)
// InBuffer:  GenericDriverDebugSettings
// OutBuffer: NULL


// Save current debug settings permanently in the registry
#define IOCTL_GENERIC_SAVE_DEBUG_SETTINGS                  TFASC_IOCTL_CODE(0x004,METHOD_BUFFERED)
// InBuffer:  NULL
// OutBuffer: NULL





///////////////////////////////////////////////////////////////////
// Driver IOCTL Requests (user mode)
///////////////////////////////////////////////////////////////////

#define IOCTL_TFA_GET_DEVICE_COUNT                         TFASC_IOCTL_CODE(0x020,METHOD_BUFFERED)
// InBuffer:  NULL
// OutBuffer: TfaGetDeviceCount

#define IOCTL_TFA_GET_DEVICE_IDS                           TFASC_IOCTL_CODE(0x021,METHOD_BUFFERED)
// InBuffer:  NULL
// OutBuffer: TfaDeviceId[]

#define IOCTL_TFA_GET_DEVICE_INFO                          TFASC_IOCTL_CODE(0x024,METHOD_BUFFERED)
// InBuffer:  TfaGetDeviceInfo
// OutBuffer: TfaGetDeviceInfo


#define IOCTL_TFA_GET_CHIP_INFO                            TFASC_IOCTL_CODE(0x025,METHOD_BUFFERED)
// InBuffer:  TfaGetChipInfo
// OutBuffer: TfaGetChipInfo


#define IOCTL_TFA_I2C_TRANSACTION                          TFASC_IOCTL_CODE(0x028,METHOD_BUFFERED)
// InBuffer:  TfaI2cTransaction
// OutBuffer: TfaI2cTransaction


///////////////////////////////////////////////////////////////////
// Driver IOCTL Requests (kernel mode)
///////////////////////////////////////////////////////////////////

#define IOCTL_INTERNAL_TFA_REGISTER_DEVICE                 TFASC_IOCTL_CODE(0x040,METHOD_BUFFERED)
// InBuffer:  InternalTfaRegisterDevice
// OutBuffer: TfaDeviceId

#define IOCTL_INTERNAL_TFA_GET_I2C_TRANSACTION             TFASC_IOCTL_CODE(0x041,METHOD_BUFFERED)
// InBuffer:  TfaDeviceId
// OutBuffer: InternalTfaGetI2cTransaction

#define IOCTL_INTERNAL_TFA_COMPLETE_I2C_TRANSACTION        TFASC_IOCTL_CODE(0x042,METHOD_BUFFERED)
// InBuffer:  InternalTfaCompleteI2cTransaction
// OutBuffer: NULL

#define IOCTL_INTERNAL_TFA_UNREGISTER_DEVICE               TFASC_IOCTL_CODE(0x043,METHOD_BUFFERED)
// InBuffer:  TfaDeviceId
// OutBuffer: NULL





///////////////////////////////////////////////////////////////////
// Support Macros
///////////////////////////////////////////////////////////////////

//
// Define the device type value. Note that values used by Microsoft
// are in the range 0-32767, and 32768-65535 are reserved for use
// by customers.
//
#define FILE_DEVICE_TFASC     0x80EF

//
// Macro used to generate IOCTL codes.
// Note that function codes 0-2047 are reserved for Microsoft, and
// 2048-4095 are reserved for customers.
//
#define TFASC_IOCTL_BASE       0x800

#define TFASC_IOCTL_CODE(FnCode,Method)  \
   ( (ULONG)CTL_CODE(                       \
      (ULONG)FILE_DEVICE_TFASC,          \
      (ULONG)(TFASC_IOCTL_BASE+(FnCode)),\
      (ULONG)(Method),                      \
      (ULONG)FILE_ANY_ACCESS                \
      ) )




///////////////////////////////////////////////////////////////////
// Data structures 
///////////////////////////////////////////////////////////////////

// force struct alignment = 1 byte
#include <pshpack1.h>   


//
// driver version info
//
typedef struct tagGenericDriverInfo
{
  // current version of programming interface exported by the device driver
  unsigned int interfaceVersionMajor;
  unsigned int interfaceVersionMinor;

  // current version of device driver implementation
  unsigned int driverVersionMajor;
  unsigned int driverVersionMinor;
  unsigned int driverVersionSub;

  // additional information, encoded as bit flags
  unsigned int flags;
// the device driver is a debug build
#define GENERIC_INFOFLAG_CHECKED_BUILD    0x00000001

} GenericDriverInfo;


//
// driver debug settings
//
typedef struct tagGenericDriverDebugSettings
{
  // trace mask
  unsigned int traceMask;

} GenericDriverDebugSettings;



// We define a union to encapsulate 32 bit and 64 bit pointers.
// For both x86 and x64 code the size of this union is 8 bytes.
typedef union tagGenericEmbeddedPointer {

	// pointer type, size varies
	void* pointer;
	// handle type, declared as void*, size varies
	HANDLE handle;

	// 32 bit pointer type used by the 64 bit driver if the calling process is 32 bit
	void* __ptr32 pointer32;
	void* __ptr32 handle32;
	// 64 bit pointer type, size is always 8 bytes
	// forces sizeof(GenericEmbeddedPointer) == 8
	void* __ptr64 pointer64;
	void* __ptr64 handle64;

// lint does not understand __ptr32 and __ptr64
// force struct size == 8 bytes
#ifdef _lint
	unsigned char just_for_lint[8];
#endif

} GenericEmbeddedPointer;



typedef struct tagTfaGetDeviceCount
{

  unsigned int count;

} TfaGetDeviceCount;


typedef struct tagTfaGetDeviceInfo
{

  // in
	TfaDeviceId deviceId;

  // out
  TfaDeviceInfo info;

} TfaGetDeviceInfo;


typedef struct tagTfaGetChipInfo
{

  // in
	TfaDeviceId deviceId;
  unsigned int chipIndex;

  // out
  TfaChipInfo info;

} TfaGetChipInfo;

#define TFA_MAX_I2C_DATA_SIZE 2048

typedef struct tagTfaI2cTransaction
{
  // use by kernel
  unsigned __int64 transactionId;

  // in
  TfaDeviceId deviceId;
	unsigned int busNumber;
	unsigned char slaveAddress;

  unsigned char writeData[TFA_MAX_I2C_DATA_SIZE];
  unsigned int numWriteBytes;

  // out
  unsigned char readData[TFA_MAX_I2C_DATA_SIZE];

  // in
  unsigned int numReadBytes;

} TfaI2cTransaction;


#define TFA_MAX_CHIPS_PER_DEVICE 64

typedef struct tagInternalTfaRegisterDevice
{

  unsigned int numChips;
  TfaChipInfo chips[TFA_MAX_CHIPS_PER_DEVICE];

} InternalTfaRegisterDevice;


typedef struct tagInternalTfaGetI2cTransaction
{
  TfaDeviceId deviceId;

  // transaction id that must be returned in InternalTfaCompleteI2cTransaction (IOCTL_INTERNAL_TFA_COMPLETE_I2C_TRANSACTION)
  unsigned __int64 transactionId;

  // I2C address
	unsigned int busNumber;
	unsigned char slaveAddress;

  // data that should be written to I2C
  unsigned char writeData[TFA_MAX_I2C_DATA_SIZE];
  unsigned int numWriteBytes;

  // number of bytes that should be read through I2C
  unsigned int numReadBytes;

} InternalTfaGetI2cTransaction;


typedef struct tagInternalTfaCompleteI2cTransaction
{
  TfaDeviceId deviceId;

  // transaction id returned InternalTfaGetI2cTransaction (IOCTL_INTERNAL_TFA_GET_I2C_TRANSACTION)
  unsigned __int64 transactionId;

  // data received through I2C
  unsigned char readData[TFA_MAX_I2C_DATA_SIZE];
  unsigned int numReadBytes;

  int status;

} InternalTfaCompleteI2cTransaction;


// restore previous alignment
#include <poppack.h>



#endif  // __tfasc_ioctl_h__

/*************************** EOF **************************************/
