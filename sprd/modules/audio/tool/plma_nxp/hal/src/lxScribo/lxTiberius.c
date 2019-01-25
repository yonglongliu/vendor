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
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>	
#include <fcntl.h>
#include <dirent.h> 

#include "dbgprint.h"
#include "lxScribo.h"
#include "NXP_I2C.h" /* for the error codes */
#include "config.h"
#include "tfa_ext.h"
#include "udp_sockets.h"

#define LXTIBERIUS_UDP_TIMEOUT_SEC 5

extern int i2c_trace;
static int fd=-1;
static tfa_event_handler_t tfa_ext_handle_event;

/* tiberius message */
static char result_buffer[550*4]; //TODO align the tfadsp.h /lock?

static int tfa_tib_dsp_msg(int devidx, int length, const char *buffer);
static int tfa_tib_dsp_msg_read(int devidx, int length, char *buffer);

static const uint16_t cmdPinSet    = ('p' << 8 | 's') ; /* set pin */
static const uint16_t cmdPinRead   = ('p' << 8 | 'r') ; /* get pin */
static const uint8_t terminator    = 0x02; /* terminator for commands and answers */

static int lxTiberiusVersion(char *buffer, int fd)
{
	/* Avoid warnings */
	(void)fd;
	(void)buffer;

	return 0;
}

/*
 * set pin
 */
int lxTiberiusSetPin(int fd, int pin, int value)
{
	char cmd[6];
	char response[6];
	int status, writeStatus;

	/* Pin 8 is used for Tiberius 2 diagnostic purposes, all others map to MMAP for LTT */
	if(pin == 8) {
		cmd[0] = (uint8_t)(cmdPinSet & 0xff);
		cmd[1] = (uint8_t)(cmdPinSet >> 8);
		cmd[2] = (uint8_t)pin;
		cmd[3] = (uint8_t)(value & 0xFF);
		cmd[4] = (uint8_t)(value >> 8);
		cmd[5] = terminator;

		status = lxScriboUDPWriteReadGeneric(fd, cmd, sizeof(cmd), response, 6, &writeStatus);

		if (status < 0)
			return NXP_I2C_NoAck;
	} else {
		return lxDspMmapSetPin(fd, pin, value);
	}

	return 0;
}

/*
 * get pin state
 */
int lxTiberiusGetPin(int fd, int pin)
{
	int status, writeStatus, value;
	char cmd[4];	
	char response[4];

	/* Pin 8 is used for Tiberius 2 diagnostic purposes, all others map to MMAP for LTT */
	if(pin == 8) {
		cmd[0] = (uint8_t)(cmdPinRead & 0xff);
		cmd[1] = (uint8_t)(cmdPinRead >> 8);
		cmd[2] = (uint8_t)pin;
		cmd[3] = terminator;

		status = lxScriboUDPWriteReadGeneric(fd, cmd, sizeof(cmd), response, sizeof(response), &writeStatus);

		if (status < 0)
			return NXP_I2C_NoAck;

		value = (cmd[1] << 8) | cmd[0];
	} else {
		return lxDspMmapGetPin(fd, pin);
	}

	return value;
}

static int tfa_tib_dsp_msg(int devidx, int length, const char *buffer)
{
	int error = 0, real_length;

	(void)devidx;

	real_length = udp_write(fd, (char *)buffer, length);
	if ( real_length != length) {
		pr_err("%s: length mismatch: exp:%d, act:%d\n",__FUNCTION__, length, real_length);
		error = 6; /*. communication with the DSP failed */
	}

	return error;
}

static int tfa_tib_dsp_msg_read(int devidx, int length, char *buffer)
{
	int error=0, status;

	(void)devidx;

	status = udp_read(fd, (char*)buffer, length, LXTIBERIUS_UDP_TIMEOUT_SEC);

	if (status < 0)
		error = NXP_I2C_NoAck;

	return error;
}

static int lxTiberius_execute_message(int command_length, void *command_buffer,
		int read_length, void *read_buffer) 
{
	int result=-1;

	if ( (command_length!=0) & (read_length==0) ) {
		/* Write message */
		result = tfa_tib_dsp_msg(0, command_length, command_buffer);
		result = result ? result : (int)result_buffer[0] ;
	} else if ( (command_length!=0) & (read_length!=0) ) {
		/* Write + Read message */
		result = tfa_tib_dsp_msg(0, command_length, command_buffer);
		if ( result == 0 )
			result = tfa_tib_dsp_msg_read( 0, read_length, read_buffer);
	} else if ( (command_length==0) & (read_length!=0) ) {
		/* Read result */
		result = tfa_tib_dsp_msg_read( 0, read_length, read_buffer);
	}

	/* return value is returned length, dsp result status or <0 if error */
	return result;
}

int lxTiberiusInit(char *devname)
{
	char hostname[25]="localhost";
	int port = 5557; /* default */

	/* Register the write/read messages to write over udp */
	tfa_ext_set_ext_dsp(1); // set ext_dsp
	tfa_ext_register(NULL, tfa_tib_dsp_msg, tfa_tib_dsp_msg_read, &tfa_ext_handle_event);
	lxTiberius_device.if_type |= NXP_I2C_Msg;

	/* socket init for write/read messages */
	fd = udp_socket_init(hostname, port);
	pr_debug("UDP link: %s@%d", hostname, port);
	pr_debug(" %s\n", fd>0 ? "ok" : "FAILED!");

	/* i2c write/read action use sysfs, so we need to init sysfs */
	lxSysfsInit(devname);
    
	/* Get- and SetPin commands are routed to an mmap-file, so we also need to init mmap */
	lxDspMmapInit(devname);
    
	return fd;
}

struct nxp_i2c_device lxTiberius_device = {
	lxTiberiusInit,
	lxSysfsWriteRead,
	lxTiberiusVersion,
	lxTiberiusSetPin,
	lxTiberiusGetPin,
	NULL,
	NULL,
	NULL,
	NULL,
	NXP_I2C_Msg, /* can do rpc msg */
	lxTiberius_execute_message
};
