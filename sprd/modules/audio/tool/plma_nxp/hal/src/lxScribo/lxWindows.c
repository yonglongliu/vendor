#undef __cplusplus 

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>

#include "Scribo.h"

//Link  with Scribo.lib. Make sure the Scribo.lib can be found by the linker
#ifdef _X64
#pragma comment (lib, "Scribo64.lib")	
#else
#pragma comment (lib, "Scribo.lib")	
#endif

/* implementation of the NXP_I2C API on Windows */
#include "NXP_I2C.h"
#include "lxScribo.h"
#include "dbgprint.h"
#include <stdio.h>
#include <assert.h>

int lxWindows_verbose = 0;

extern int close(int fd);

void lxWindowsVerbose(int level)
{
	lxWindows_verbose = level;
}
/*
	called during target device registry
	 this function formerly was in NXP_I2C_Windows.c
*/
static int lxWindowsInit(char *this)
{
#define bufSz 1024
	char buffer[bufSz];
	unsigned char ma, mi, bma, bmi;
	unsigned short sz = bufSz;
	unsigned long busFreq;

	//NXP_I2C_Error_t error = NXP_I2C_NoInterfaceFound;

	//Show some info about Scribo client and server
	if (ClientVersion(&ma, &mi, &bma, &bmi)) {
		if (lxWindows_verbose)
			PRINT("Scribo client version: %d.%d.%d.%d\r\n", ma, mi, bma, bmi);
	} else {
		PRINT("Failed to get Scribo client version\r\n");
	}

	if (ServerVersion(&ma, &mi, &bma, &bmi)) {
		if (lxWindows_verbose)
			PRINT("Scribo server version: %d.%d.%d.%d\r\n", ma, mi, bma, bmi);
	} else {
		PRINT("Failed to get Scribo server version\r\n");
	}

	//If Scribo isn't connected yet, try to connect
	if (!ConnectionValid()) Connect();

	if (ConnectionValid()) 
	{
		//We are connected, so print some info
		if (VersionStr(buffer, &sz)) {
			//just to be safe
			if (sz < bufSz) buffer[sz] = '\0';
			if (lxWindows_verbose)
				printf_s("Remote target: %s\r\n", buffer);
		}
		else {
			PRINT("Remote target didn't return version string\r\n");
		}

		busFreq = GetSpeed();
		if (busFreq > 0) {
			if ( lxWindows_verbose)
				PRINT("I2C bus frequency: %d\r\n", busFreq);
		}
		else {
			PRINT("I2C bus frequency unknown\r\n");
		}
	}

	(void)this; /* Remove unreferenced parameter warning */

	return ConnectionValid();
}

/*
 * Close an opened device
 * Return success (0) or fail (-1)
 */
int lxWindowsClose(int fd)
{
	int ret;

	ret = close(fd);

	return ret;
}


static int lxWindowsWriteRead(int fd, int NrOfWriteBytes, const uint8_t *WriteData,
			 int NrOfReadBytes, uint8_t *ReadData, uint32_t *pError)
{
	uint8_t sla = (uint8_t)WriteData[0];
	uint8_t *write_data = (uint8_t*)&WriteData[1];
	uint16_t rCnt = ((uint16_t)NrOfReadBytes-1);

	NrOfWriteBytes--;

	*pError = NXP_I2C_Ok;

	/* Write transaction */
	if (NrOfReadBytes < (1+1)) {
		if (!Write(sla >> 1, write_data, ((unsigned short)NrOfWriteBytes & 0xFFFF))) {
			*pError = LastError();
			return 0;
		}

		return NrOfWriteBytes;		
	}

	/* WriteRead transaction */
	if (!WriteRead(sla >> 1, write_data, ((unsigned short)NrOfWriteBytes & 0xFFFF), &ReadData[1], &rCnt)) {
			*pError = LastError();
			return 0;
	}

	(void)fd; /* Remove unreferenced parameter warning */ 

	return rCnt + 1;
}

static int lxWindowsVersion(char *buffer, int fd)
{
	unsigned short sz = bufSz;

	VersionStr(buffer, &sz);
	
	(void)fd; /* Remove unreferenced parameter warning */

	return sz;
}

static int lxWindowsGetPin(int fd, int pin)
{
	uint16_t value;

	GetPin((uint8_t)pin, &value);

	(void)fd; /* Remove unreferenced parameter warning */

	return value;
}

static int lxWindowsSetPin(int fd, int pin, int value)
{
	(void)fd; /* Remove unreferenced parameter warning */
	return SetPin((uint8_t)pin, (uint16_t)value); 
}

const struct nxp_i2c_device lxWindows_device = {
	lxWindowsInit,
	lxWindowsWriteRead,
	lxWindowsVersion,
	lxWindowsSetPin,
	lxWindowsGetPin,
	lxWindowsClose,
	NULL,
	NULL,
	NULL
};
