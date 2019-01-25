/*
 * lxHid.c
 *
 *  Created on: Oct 16, 2014
 *      Author: wim
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#if (defined(WIN32) || defined(_X64))
#include <windows.h>
#else
#include <unistd.h>
#define Sleep(x) usleep((x)*1000)
#endif

#include "hal_utils.h"
#include "dbgprint.h"
#include "lxScribo.h"
#include "NXP_I2C.h" /* for the error codes */

extern int lxScribo_verbose;

#if !(defined(WIN32) || defined(_X64))
extern int i2c_trace; // in other builds it's in lxI2c.c
#else
int i2c_trace;
#endif

// the USGIO API
#define TB_DEBUG 1
#include "usbgio_api.h"
// map Windows TCHAR functions and macros to normal C-Runtime library functions on Linux
//#define PRINT_FILE PRINT_FILE

#define _T(x)     x

// handle to the device
static USBGIODeviceHandle gDeviceHandle = USBGIO_INVALID_HANDLE;

// module globals to handle pinevents and state 
static unsigned int gWait = 1;
unsigned int gPinId; 
USBGIO_GPIOState gState;

#ifndef I2C_SLAVE
#define I2C_SLAVE	0x0703	/* dummy address for building API	*/
#endif

/*
 * wtoc(char* dest, const T_UNICHAR* src) - convert a T_UNICHAR string to a char string.
 *
 * dest - pointer to a char destination buffer that will contain the converted string.
 * src - constant pointer to a source T_UNICHAR string to be converted to char string.
 * bufsize - buffersize of src buffer.
 */
static void wtoc(char* dest, const T_UNICHAR* src, int bufsize)
{
	int i = 0;
	while(i < bufsize && src[i] != '\0') {
		dest[i] = (char)src[i];
		++i;
	}
}

static void  lxHidSlave(int fd, int slave)
{
    if (i2c_trace) PRINT("I2C slave=0x%02x\n", slave);
    (void)fd; /* Remove unreferenced parameter warning C4100 */
}

/* SC42158
 *  Remove slave checking and use the address from the transaction.
 */
int lxHidWriteRead(int fd, int NrOfWriteBytes, const uint8_t * WriteData,
		int NrOfReadBytes, uint8_t * ReadData, unsigned int *pError) {
	int ln = -1;
	  TSTATUS st;
	  int rdcount;
	  uint8_t *rdbuf=NULL;

	  if(NrOfReadBytes) {
		  rdcount= NrOfReadBytes-1;
		  rdbuf=&ReadData[1];
		  ln=rdcount;
	  }
	  else {
		  rdcount=0;
		  ln = NrOfWriteBytes-1;
	  }

	lxHidSlave(fd, WriteData[0]>>1 );

	if (NrOfWriteBytes & i2c_trace) {
		hexdump("W", WriteData, NrOfWriteBytes);
	}

//hid+++
	  // gIntArgs[0] is the address of the register to read
//	  unsigned char regAddr = (unsigned char)gIntArgs[0];
//	  unsigned char readData[2];
	  st = USBGIO_I2cTransaction(gDeviceHandle,                 // USBGIODeviceHandle deviceHandle,
	                             0,                             // unsigned int i2cBus,
	                              (400 * 1000),                 // unsigned int i2cSpeed,
	                             WriteData[0] >> 1,    // unsigned int i2cSlaveAddress,
	                             &WriteData[1],                      // const unsigned char* writeData,
	                             NrOfWriteBytes-1,               // unsigned int bytesToWrite,
	                             rdbuf,                       // unsigned char* readBuffer,
	                             rdcount,              // unsigned int bytesToRead,
	                             1000                           // unsigned int timeoutInterval
	                             );
	  if (TSTATUS_SUCCESS != st) {
            /* don't print an error message when no ack, because of i2c device scan */
	    if ((TSTATUS_RESPONSE_STATUS_BASE + /*THID_STAT_I2C_NO_ACK_FOR_ADDRESS*/0x09) != st) {
	        PRINT_ERROR( _T("USBGIO_I2cTransaction failed, status = 0x%08X\n"), st);
	    }
	    ln = -1;
	  }


//hid---

	if (NrOfReadBytes & i2c_trace) {
		hexdump("R", ReadData, NrOfReadBytes);
	}

	if ( ln < 0 ) {
		*pError = NXP_I2C_NoAck; /* treat all errors as nack */
		perror("i2c slave error");
	} else {
		*pError = NXP_I2C_Ok;
	}

	return ln+1;
}

///+++

static
TSTATUS
OpenDevice(int devnr)
{
  TSTATUS st;
  unsigned int devCnt;
    T_UNICHAR devInst[512];

  // enumerate available devices
  if ( lxScribo_verbose )
	  PRINT_FILE(stdout, _T("Enumerate available devices...\n"));

  st = USBGIO_EnumerateDevices(&devCnt);
  if (TSTATUS_SUCCESS != st) {
    PRINT_ERROR( _T("USBGIO_EnumerateDevices failed, status = 0x%08X\n"), st);
    return st;
  }

  if (0 == devCnt) {
    PRINT_ERROR( _T("No devices connected, please connect a device.\n"));
    return TSTATUS_NO_DEVICES;
  }

  if ( lxScribo_verbose ) {
	  PRINT_FILE(stdout, _T("Number of connected devices: %u\n"), devCnt);
	  // open the first device
	  PRINT_FILE(stdout, _T("Open device with index %d...\n"), devnr);
  }

  st = USBGIO_GetDeviceInstanceId(devnr,                    // unsigned int deviceIndex,
                                  devInst,              // T_UNICHAR* instanceIdString,
                                  sizeof(devInst)       // unsigned int stringBufferSize
                                  );
  if (TSTATUS_SUCCESS != st) {
    PRINT_ERROR( _T("USBGIO_GetDeviceInstanceId failed, status = 0x%08X\n"), st);
    return st;
  }

  st = USBGIO_OpenDevice(devInst,         // const T_UNICHAR* instanceIdString,
                         &gDeviceHandle   // USBGIODeviceHandle* deviceHandle
                         );
  if (TSTATUS_SUCCESS != st) {
    PRINT_ERROR( _T("USBGIO_OpenDevice failed, status = 0x%08X\n"), st);
    return st;
  }

  return TSTATUS_SUCCESS;
} // OpenDevice


static void CloseDevice(void) {
	TSTATUS st;
	  // make sure the device is closed
	  if (USBGIO_INVALID_HANDLE != gDeviceHandle) {
	    st = USBGIO_CloseDevice(gDeviceHandle);
	    if (TSTATUS_SUCCESS != st) {
	      PRINT_ERROR( _T("USBGIO_CloseDevice failed, status = 0x%08X\n"), st);
	      return;
	    }
	    gDeviceHandle = USBGIO_INVALID_HANDLE;
	  }
}
//----
/*
 *
 *  Slave is set in the transaction call.
 */
static int lxHidInit(char *devname)
{
    int status, nr=0;//default

    if (devname != NULL ) {
    	sscanf(devname,"hid%d", &nr);
    }

    if ( lxScribo_verbose )
    	printf("Opening HID connection device:%d\n", nr);


    // if a device is already opened, close it
    if (USBGIO_INVALID_HANDLE != gDeviceHandle)
	    CloseDevice();

	status = OpenDevice(nr);
	if (status == TSTATUS_SUCCESS) {
		status = 0;
		atexit(CloseDevice);
	} else
		status = -1;

	return status;
}


static int lxHidVersion(char *buffer, int fd)
{
	TSTATUS st;
	unsigned int usbVid;
	unsigned int usbPid;
	unsigned int bcdDevice;
	char c_serialNumber[256] = {'\0'};
	
	T_UNICHAR serialNumber[256];
	st = USBGIO_GetDeviceSerialNumber(gDeviceHandle, // USBGIODeviceHandle deviceHandle,
	                                  serialNumber,  // T_UNICHAR* serialNumber,
	                                  sizeof(serialNumber) // unsigned int stringBufferSize
	);

	if (TSTATUS_SUCCESS != st) {
		PRINT_ERROR(_T("USBGIO_GetDeviceSerialNumber failed, status = 0x%08X\n"), st);
		return sprintf(buffer, "(failed to get the device serial number)");
	}
	
	/* convert wchar (unicode) to char buffer */
	wtoc(c_serialNumber, serialNumber,  sizeof(serialNumber));

	st = USBGIO_GetDeviceUsbInfo(gDeviceHandle, // USBGIODeviceHandle deviceHandle,
								&usbVid,        // unsigned int* usbVendorId,
								&usbPid,        // unsigned int* usbProductId,
								&bcdDevice      // unsigned int* bcdDevice
	);

	if (TSTATUS_SUCCESS != st) {
		PRINT_ERROR(_T("USBGIO_GetDeviceUsbInfo failed, status = 0x%08X\n"), st);
		return sprintf(buffer, "(failed to get the USB device platform and firmware info)");
	}
	
	{ 	
		// composes a string like "DEVKIT_LPC1850 firmware v1.1.2 (USB-0)"
		char fw_version_str[64] = {'\0'};
		unsigned int hwplt  = (bcdDevice & 0xF800) >> 11;
		unsigned int majver = (bcdDevice & 0x780)  >> 7 ;
		unsigned int minver = (bcdDevice & 0x78)   >> 3 ;
		unsigned int swrev  = (bcdDevice & 0x7)    >> 0 ;	
		switch (hwplt) { // the hardware platform variants are defined in the firmware code: app_config.h
			case 0:
				sprintf(buffer, "TFA98xx Development PCB: ");
				break;
			case 1:
				sprintf(buffer, "LPC435x-Xplorer++: ");
				break;
			case 2:
				sprintf(buffer, "MaxDongle: ");
				break;
			case 3:
				sprintf(buffer, "Smart Speaker Dev. Board MKII: ");
				break;
			default:
				sprintf(buffer, "NO_PLATFORM_DEF ");
				break;
		}	
		sprintf(fw_version_str, "firmware v%u.%u.%u (USB-%c)", majver, minver, swrev, c_serialNumber[strlen(c_serialNumber)-1]);
		strcat(buffer, fw_version_str);
	}
	
	(void)fd; /* Remove unreferenced parameter warning C4100 */

	return (int)strlen(buffer);
}

static int lxHidGetPin(int fd, int pin)
{
 	TSTATUS st;
	USBGIO_GPIOState state;
	unsigned int pinId = 0;
	
	/* check pin id */
	if (pin < 1 || pin > 1024) {
		PRINT_ERROR( _T("Invalid pinId (%d), please provide a valid pinId (0 < pinId <= 1024).\n"), pin);
		return -1;
	}
	else {
		pinId = (unsigned int) pin;
	}
	
	st = USBGIO_GetPinState(gDeviceHandle, // USBGIODeviceHandle deviceHandle,
							pinId, // unsigned int pinId, 
							&state // USBGIO_GPIOState* state
	); 
	
	if (TSTATUS_SUCCESS != st) {
        // do not print a error message, return the errorcode only
		//PRINT_ERROR(_T("USBGIO_GetPinState failed, status = 0x%08X\n"), st);
		return -1;
	}  
	/*PRINT_FILE(stdout, _T("Pin 0x%08X has state %s\n"), pinId, (GPIOState_Low == state) ? _T("low") : _T("high"));*/
	
	/* Remove unreferenced parameter warning C4100 */
	(void)fd;

	return ((GPIOState_Low == state) ? 0 : 1);
}

static int lxHidSetPin(int fd, int pin, int value)
{
	TSTATUS st;
	USBGIO_GPIOState state;
	unsigned int pinId = 0;
	
	/* check pin id */
	if (pin < 1 || pin > 1024) {
		PRINT_ERROR( _T("Invalid pinId (%d), please provide a valid pinId (0 < pinId <= 1024).\n"), pin);
		return -1;
	}
	else {
		pinId = (unsigned int) pin;
	}
	
	/* check pin value */
	if (value != 0 && value != 1) {
		PRINT_ERROR( _T("Invalid pin value (%d), please provide a valid pin value (0|1).\n"), value);
		return -1;
	} 
	else {
		state = (0 == value) ? GPIOState_Low : GPIOState_High;
	}
  
	/*PRINT_FILE(stdout, _T("Setting pin 0x%08X to state %s\n"), pinId, (GPIOState_Low == state) ? _T("low") : _T("high"));*/
	st = USBGIO_SetPinState(gDeviceHandle, // USBGIODeviceHandle deviceHandle,
							pinId, // unsigned int pinId,
							state // USBGIO_GPIOState state
	);
	
	if (TSTATUS_SUCCESS != st) {
		PRINT_ERROR(_T("USBGIO_SetPinState failed, status = 0x%08X\n"), st);
		return -1;
	}
	
	(void)fd; /* Remove unreferenced parameter warning C4100 */

	return 0;
}

 /*
  * Close an opened HID device
  * Return success (0) or fail (-1)
  */
static int lxHidClose(int fd)
{
	int ret = -1;   
	// close an opened device
	if ( USBGIO_INVALID_HANDLE != gDeviceHandle ) {
		CloseDevice();
		ret = 0;
	}
	
	(void)fd; /* Remove unreferenced parameter warning C4100 */
	return ret;
}

/*
 * Scan which devices are available
 * Return the number of devices
 */
int lxHidNrDevices(void)
{
	TSTATUS st;
	unsigned int devCnt = 0;

	st = USBGIO_EnumerateDevices(&devCnt);
	if (TSTATUS_SUCCESS != st) {
		PRINT_ERROR("USBGIO_EnumerateDevices failed, status = 0x%08X\n", st);
		return -1;
	}

	return devCnt;
}

static void USBGIO_CALL InputPinEventCb(void* ptr /*callbackContext*/, unsigned int pinId, USBGIO_GPIOState state)
{
	/* wait for the given pin to be in the given state and exit the wait loop */ 
	if ((gPinId == pinId) && (gState == state)) {
		PRINT_ERROR(_T("Pin 0x%08X event state %s\n"), pinId, (GPIOState_Low == state) ? _T("low") : _T("high"));
		gWait = 0;
	}
	
	/* Remove unreferenced parameter warning C4100 */
	ptr = NULL;
}

/* 
 * register a callback and wait for a pinevent (to PinState == value) 
 * to be sent from the target or return when timed-out (timeout in miliseconds)
 * return : (-1) on error, (0) on pin event received, (1) on timeout
 */
int lxHidWaitOnPinEvent(int pin, int value, unsigned int timeout)
{
	TSTATUS st;
    USBGIO_GPIOMode curMode;
	unsigned int cnt = 0;
	int err = 0;

	/* check pin id */
	if (pin < 1 || pin > 1024) {
		PRINT_ERROR( _T("Invalid pinId (%d), please provide a valid pinId (0 < pinId <= 1024).\n"), pin);
		return -1;
	}
	else {
		gPinId = (unsigned int) pin;
	}
	
	/* check pin value */
	if (value != 0 && value != 1) {
		PRINT_ERROR( _T("Invalid pin value (%d), please provide a valid pin value (0|1).\n"), value);
		return -1;
	} 
	else {
		gState = (0 == value) ? GPIOState_Low : GPIOState_High;
	}
	
    st = USBGIO_RegisterPinEventCallback(gDeviceHandle, InputPinEventCb, NULL);
    if (TSTATUS_SUCCESS != st) {
        PRINT_ERROR(_T("USBGIO_RegisterPinEventCallback (register) failed, status = 0x%08X\n"), st);
        return -1;
    }
 
    st = USBGIO_GetPinMode(gDeviceHandle, pin, &curMode);
    if (TSTATUS_SUCCESS != st) {
        PRINT_ERROR( _T("USBGIO_GetPinMode failed, status = 0x%08X\n"), st);
        return -1;
    }
 
    // switch the pin mode to input event 
    st = USBGIO_SetPinMode(gDeviceHandle, pin, GPIOMode_InputEvent);
    if (TSTATUS_SUCCESS != st) {
        PRINT_ERROR(_T("USBGIO_SetPinMode (input event) failed, status = 0x%08X\n"), st);
        return -1;
    }

    // wait for the event or a timeout
	while (gWait == 1) {
		Sleep(100); // sleep for 100 ms
		cnt += 100;
		if ( (timeout != 0) && (cnt >= timeout)) {
			gWait = 0; // exit on timeout
			err = 1;   // signal timeout error code
		}
	}

    // unregister pin event callback
    st = USBGIO_RegisterPinEventCallback(gDeviceHandle, NULL, NULL);
    if (TSTATUS_SUCCESS != st) {
        PRINT_ERROR(_T("USBGIO_RegisterPinEventCallback (unregister) failed, status = 0x%08X\n"), st);
        return -1;
    }

    // restore the pin mode of the pin
    st = USBGIO_SetPinMode(gDeviceHandle, pin, curMode);
    if (TSTATUS_SUCCESS != st) {
        PRINT_ERROR(_T("USBGIO_SetPinMode (curMode) failed, status = 0x%08X\n"), st);
        return -1;
    }

	return err;
}

const struct nxp_i2c_device lxHid_device = {
	lxHidInit,
	lxHidWriteRead,
	lxHidVersion,
	lxHidSetPin,
	lxHidGetPin,
	lxHidClose,
	NULL,
	NULL,
	NULL,
	NXP_I2C_HID
};
