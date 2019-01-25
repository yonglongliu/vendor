/*
 *Copyright 2014,2015 NXP Semiconductors
 *
 *Licensed under the Apache License, Version 2.0 (the "License");
 *you may not use this file except in compliance with the License.
 *You may obtain a copy of the License at
 *            
 *http://www.apache.org/licenses/LICENSE-2.0
 *             
 *Unless required by applicable law or agreed to in writing, software
 *distributed under the License is distributed on an "AS IS" BASIS,
 *WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *See the License for the specific language governing permissions and
 *limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#if (defined(WIN32) || defined(_X64))
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdarg.h>
#include <tchar.h>
// version defs
#include "version.h"
// TFA side channel API
#include "tfasc_api.h"
#endif

#include "hal_utils.h"
#include "dbgprint.h"
#include "lxScribo.h"

extern int lxScribo_verbose;

// max number of side channel device ids
#define MAX_DEVIDS 4

// global device info
TfaChipInfo gChipInfo;
TfaDeviceId gId;

static void  lxHidSlave(int fd, int slave)
{
    if (i2c_trace) PRINT("I2C slave=0x%02x\n", slave);
    (void)fd; /* Remove unreferenced parameter warning C4100 */
}

static int lxWinSideChannelInit(char *devname)
{
    int nr = 0;

	TSTATUS err;
	TfaDeviceId id;
	TfaDeviceId devIds[MAX_DEVIDS] = {0};
    TfaDeviceInfo deviceInfo;
	TfaChipInfo chipInfo;
	unsigned int devCount = 0;
	unsigned int validIds = 0;
	
    if (devname != NULL ) {
    	sscanf(devname,"wsc%d", &nr);
    }
    
	if ( lxScribo_verbose )
    	printf("Opening SideChannel connection device: %d\n", nr);

	err = TFASC_Open();
	if ( TSTATUS_SUCCESS != err ) {
		PRINT_ERROR(("TFASC_Open() failed (0x%08x)\n"), err);
		return -1;
	}

    err = TFASC_GetTfaDeviceCount(&devCount);
	if ( TSTATUS_SUCCESS != err ) {
		PRINT_ERROR(("TFASC_GetTfaDeviceCount() failed (0x%08x)\n"), err);
		return -1;
    }

	if (0 == devCount) {
		PRINT_ERROR(("No device(s).\n"));
		return -1;
	}

	if (devCount > MAX_DEVIDS) {
		PRINT_ERROR(("Too many connected devices: %d.\n"), devCount);
		return -1;
	}
	
	if ( lxScribo_verbose )
    	printf("%u device(s) found.\n", devCount);
	
    err = TFASC_GetTfaDeviceIds(devIds, devCount, &validIds);
	if ( TSTATUS_SUCCESS != err ) {
		PRINT_ERROR(("TFASC_GetTfaDeviceIds() failed (0x%08x)\n"), err);
		return -1;
	}

	if (0 == validIds) {
		PRINT_ERROR(("no valid device ids\n"));
		return -1;
	}

	if (nr > MAX_DEVIDS) {
		PRINT_ERROR(("Cannot connect to device: %d.\n"), nr);
		return -1;
	}

	// connect to devive nr
	id = devIds[nr];

	err = TFASC_GetTfaDeviceInfo(id, &deviceInfo);
	if ( TSTATUS_SUCCESS != err ) {
		PRINT_ERROR(("TFASC_GetTfaDeviceInfo() failed (0x%08x)\n"), err);
		return -1;
	}
  
	if ( lxScribo_verbose )
		printf("Device %u number of TFA chips: %u\n", id, deviceInfo.numTfaChips);

	if (0 == deviceInfo.numTfaChips) {
		PRINT_ERROR(("no chips\n"));
		return -1;
	}

	err = TFASC_GetTfaChipInfo(id, 0, &chipInfo);
	if ( TSTATUS_SUCCESS != err ) {
		PRINT_ERROR(("TFASC_GetTfaChipInfo() failed (0x%08x)\n"), err);
		return -1;
	}

	if ( lxScribo_verbose )
		printf("Chip %u (Bus:%u SlaveAddress:0x%02x)\n", id, chipInfo.busNumber, chipInfo.slaveAddress);

	// set global device info
	gChipInfo.busNumber = chipInfo.busNumber;
	gChipInfo.slaveAddress = chipInfo.slaveAddress;
	gId = id;

	return 0;
}

static int lxWinSideChannelWriteRead(int fd, int NrOfWriteBytes, const uint8_t * WriteData,
		int NrOfReadBytes, uint8_t * ReadData, unsigned int *pError) 
{
    int ln = -1;
	int rdcount;
	uint8_t *rdbuf = NULL;
	TSTATUS err;

	if(NrOfReadBytes) {
		rdcount = NrOfReadBytes - 1;
		rdbuf = &ReadData[1];
		ln = rdcount;
	}
	else {
		rdcount = 0;
		ln = NrOfWriteBytes - 1;
	}

	lxHidSlave(fd, WriteData[0]>>1);

	if (NrOfWriteBytes & i2c_trace) {
		hexdump("W", WriteData, NrOfWriteBytes);
	}

	err = TFASC_I2cTransaction(gId, gChipInfo.busNumber, gChipInfo.slaveAddress, &WriteData[1], NrOfWriteBytes-1, rdbuf, rdcount);
	if ( TSTATUS_SUCCESS != err ) {
		PRINT_ERROR(("TFASC_I2cTransaction() failed (0x%08x)\n"), err);
		ln = -1;
	}
	
	if (NrOfReadBytes & i2c_trace) {
		hexdump("R", ReadData, NrOfReadBytes);
	}

	if (ln < 0) {
		*pError = NXP_I2C_NoAck; /* treat all errors as nack */
		perror("i2c slave error");
	} 
	else {
		*pError = NXP_I2C_Ok;
	}

	return (ln + 1);
}
 
static int lxWinSideChannelClose(int fd)
{
	TFASC_Close();	
	(void)fd; /* Remove unreferenced parameter warning C4100 */
	return 0;
}
 
static int lxWinSideChannelVersion(char *buffer, int fd)
{
	sprintf(buffer, "Get firmware version not implemented for Windows Side Channel!");	
	(void)fd; /* Remove unreferenced parameter warning C4100 */
	return (int)strlen(buffer);
}

static int lxWinSideChannelSetPin(int fd, int pin, int value)
{
    (void)fd; /* Remove unreferenced parameter warning C4100 */
    (void)pin; /* Remove unreferenced parameter warning C4100 */
    (void)value; /* Remove unreferenced parameter warning C4100 */
	return 0;
}

static int lxWinSideChannelGetPin(int fd, int pin)
{
    (void)fd; /* Remove unreferenced parameter warning C4100 */
    (void)pin; /* Remove unreferenced parameter warning C4100 */
	return 0;
}

const struct nxp_i2c_device lxWinSideChannel_device = {
	lxWinSideChannelInit,
	lxWinSideChannelWriteRead,
	lxWinSideChannelVersion,
	lxWinSideChannelSetPin,
	lxWinSideChannelGetPin,
	lxWinSideChannelClose,
	NULL,
	NULL,
	NULL
};
