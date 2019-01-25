/* implement Scribo protocol */

#include <stdio.h>
#include <stdint.h>
#if (defined(WIN32) || defined(_X64))
#include <windows.h>
#include "lxScriboWinsockUdp.h"
/* Winsock can not use read/write from a socket */
#define read(fd, ptr, size) recv(fd, (char*)ptr, size, 0)
#define write(fd, ptr, size) send(fd, (char*)ptr, size, 0)
#define udp_read(fd, inbuf, len, waitsec) winsock_udp_read(fd, (char*)inbuf, len, waitsec)
#define udp_write(fd, outbuf, len) winsock_udp_write(fd, (char*)outbuf, len)
#else
#include <unistd.h>
#define Sleep(x) usleep((x)*1000)
#include "udp_sockets.h"
#endif
#include "hal_utils.h"
#include "dbgprint.h"
#include "NXP_I2C.h"
#include "lxScribo.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "tfa_ext.h"
#include "config.h"
#define UDP_RECV_TIMEOUT_SEC 3
#define UDP_MAX_RETRY 5
#define UDP_RETRY_DELAY_MS 150

extern int lxScribo_verbose; /* lxSribo.c */
#define VERBOSE if (lxScribo_verbose)

/* tiberius message */
static char result_buffer[550*4]; //TODO align the tfadsp.h /lock?

/*
 * Command headers for communication
 *
 * note: commands headers are little endian
 */
static const uint16_t cmdVersion   = 'v' ;              /* Version */
static const uint16_t cmdRead      = 'r' ;              /* I2C read transaction  */
static const uint16_t cmdWrite     = 'w' ;              /* I2C write transaction */
static const uint16_t cmdWriteRead = ('w' << 8 | 'r');  /* I2C write+read transaction */
static const uint16_t cmdPinSet    = ('p' << 8 | 's') ; /* set pin */
static const uint16_t cmdPinRead   = ('p' << 8 | 'r') ; /* get pin */
static const uint16_t cmdStartPlay = ('t' << 8 | 's') ; /* Start playback of audio file */
static const uint16_t cmdStopPlay  = ('p' << 8 | 'o') ; /* Stop  playback of audio file */
static const uint16_t cmdDspWrite  = ('d' << 8 | 'w') ; /* Write message to DSP */
static const uint16_t cmdDspRead   = ('d' << 8 | 'r') ; /* Read message from DSP */

static const uint8_t terminator    = 0x02; /* terminator for commands and answers */
static tfa_event_handler_t tfa_ext_handle_event;
static int fd_local=-1;

/*
 * lxScriboUDPWriteReadGeneric
 * Generic write / read mechanism with timeout and retry for UDP.
 * 
 * @param   int sock         : Socket to read/write from/to.
 * @param   char *cmd        : cmd buffer to send
 * @param   int cmdlen       : length of cmd buffer
 * @param   char *response   : response to be received (by reference)
 * @param   int rcvbufsize   : (max) size of receive buffer passed
 * @param   int *writeStatus : status of the udp write, bytes written or error. (by reference)
 *
 * @return  Num bytes read, or error code.
 */
int lxScriboUDPWriteReadGeneric(int sock, char *cmd, int cmdlen, char *response, int rcvbufsize, int *writeStatus)
{
	int retry  = UDP_MAX_RETRY;
	int status = -1;	

	do {
		if(retry != UDP_MAX_RETRY) {
			VERBOSE PRINT("UDP Retry, retries left %d\n",retry);
			// Wait a while before retrying.
			Sleep(UDP_RETRY_DELAY_MS); 
		}

		// write command
		VERBOSE hexdump("cmd", (uint8_t *)cmd, sizeof(cmd));

		#if (defined(WIN32) || defined(_X64))
			status = winsock_udp_write(sock, (char*) cmd, cmdlen);	
		#else
			status = udp_write(sock, (char*)cmd, cmdlen);
		#endif
		*writeStatus = status;
		if(status <= 0)
		{
			PRINT_ERROR("UDP write error. Tried to write %d bytes, got return status %d (retries left %d)\n",cmdlen, status, retry);
			/* No sense in reading, if write failed. Wait for next retry */
		}
		else 
		{
			#if (defined(WIN32) || defined(_X64))
				status = winsock_udp_read(sock, (char*) response, rcvbufsize, UDP_RECV_TIMEOUT_SEC); // TODO TEST TIMEOUT FOR WINDOWS
			#else
				status = udp_read(sock, (char*)response, rcvbufsize, UDP_RECV_TIMEOUT_SEC);
			#endif

			// read result.
			VERBOSE hexdump("rsp", (uint8_t *)response, status);
		}
		retry--;
	} while ( status <= 0 && retry > 0);

	return status;
}


/*
 * set pin
 */
int lxScriboUdpSetPin(int fd, int pin, int value)
{
	char cmd[6];
	char response[7];
	int writeStatus;
	int length;
	int error = -1;
	
	VERBOSE PRINT("SetPin[%d]<%d\n", pin, value);

	cmd[0] = (uint8_t)(cmdPinSet & 0xff);
	cmd[1] = (uint8_t)(cmdPinSet >> 8);
	cmd[2] = (uint8_t)pin;
	cmd[3] = (uint8_t)(value & 0xFF);
	cmd[4] = (uint8_t)(value >> 8);
	cmd[5] = terminator;

	length = lxScriboUDPWriteReadGeneric(fd, cmd, sizeof(cmd), response, sizeof(response), &writeStatus);

	if(length >= 7) {
		error = response[2] | response[3]<<8;
		if (error) {
			PRINT_ERROR("pin read error: %d\n", error);
		}
	}
	else {
		PRINT_ERROR("Communication error\n");
	}
	
	return error;
}

/*
 * get pin state
 */
int lxScriboUdpGetPin(int fd, int pin)
{
	char cmd[4];
	// response (lsb/msb)= echo cmd[0]='p' cmd[1]='r' , status [0] [1], length [0]=2 [1]=0, data[0] data[1],terminator[0]=2
	unsigned char response[9];
	uint16_t error;
	int writeStatus;
	int length;
	int value;

	(void)length;
	cmd[0] = (uint8_t)(cmdPinRead & 0xff);
	cmd[1] = (uint8_t)(cmdPinRead >> 8);
	cmd[2] = (uint8_t)pin;
	cmd[3] = terminator;

	length = lxScriboUDPWriteReadGeneric(fd, (char *)cmd, sizeof(cmd), (char *)response, sizeof(response), &writeStatus);

	//assert(length == sizeof(response)); 		// total packet size
	//assert(response[1]=='p' && response[0]=='r'); // check return cmd
	//assert((response[4]|response[5]<<8) == 2); 	//check datalength=2bytes
	//assert(response[8] == 2); 			//check terminator

	error = response[2] | response[3]<<8;
	if (error) {
		PRINT_ERROR("pin read error: %d\n", error);
		return 0;
	}
	value = (response[7] << 8) | response[6];
	VERBOSE PRINT("GetPin[%d]:%d\n", pin, value);

	return value;
}

/*
 * Close an opened device
 * Return success (0) or fail (-1)
 */
int lxScriboUdpClose(int fd)
{
	/* udp_close() not defined for windows, keep original implementation */
#if (defined(WIN32) || defined(_X64))
    (void)fd; /* Remove unreferenced for parameter warning */
    return 0;
#else
    return udp_close(fd);
#endif
}

/* send command 'STOP' to the UDP server */
int lxScriboUdpSendStopPlaybackCmd(int fd)
{
	char response[NXP_I2C_MAX_SIZE];
	uint8_t cmd[3];
	int length;
	int writeStatus;
	int cmdSize = 3;
	int rstatus;

	cmd[0] = (uint8_t)(cmdStopPlay & 0xff);
	cmd[1] = (uint8_t)(cmdStopPlay >> 8);
	cmd[2] = terminator;

	length = lxScriboUDPWriteReadGeneric(fd, (char *)cmd, cmdSize, response, 7, &writeStatus);

	rstatus = response[2] | response[3] << 8;
	if  (rstatus != 0) {
		ERRORMSG("scribo status error: 0x%02x\n", rstatus);
	}

	return length;
}

/* send command 'START' to the UDP server */
int lxScriboUdpSendStartPlaybackCmd(char *buffer, int fd)
{
	unsigned int cnt = 0;
	unsigned char cmd[512] = {0};
	char response[NXP_I2C_MAX_SIZE];
	int cmdLength = 0;
	int rstatus;
	int writeStatus;
	/* Format = 'op'(16) + 0x02 [0x6F 0x70 0x02] */
	int length;

	cnt = (int)strlen(buffer);
	cmd[0] = 0x73;
	cmd[1] = 0x74;
	cmd[2] = (unsigned char) cnt;
	memcpy(&cmd[3], buffer, cnt);
	cmd[3+cnt] = 0x02;
	cmdLength  = 4 + cnt;

	length = lxScriboUDPWriteReadGeneric(fd, (char *)cmd, cmdLength, response, 7, &writeStatus);

	rstatus = response[2] | response[3] << 8;
	if  (rstatus != 0) {
		ERRORMSG("scribo status error: 0x%02x\n", rstatus);
	}

	return length;
}

/*
 * retrieve the version string from the device
 */
int lxScriboUdpVersion(char *buffer, int fd)
{
	char cmd[3];
	char response[NXP_I2C_MAX_SIZE]; // TODO fix buffer sizing
	int length;
	int writeStatus;
	uint16_t error;

	cmd[0] = (uint8_t)(cmdVersion & 0xff);
	cmd[1] = (uint8_t)(cmdVersion >> 8);
	cmd[2] = terminator;


	length = lxScriboUDPWriteReadGeneric(fd, cmd, sizeof(cmd), response, sizeof(response), &writeStatus);

	error = response[2] | response[3]<<8;
	if (error) {
		PRINT_ERROR("version read error: %d\n", error);
		return 0;
	}
	memcpy(buffer, response+6, length);

	return length;
}

static int lxScriboUdpWrite(int fd, int size, const uint8_t *buffer, uint32_t *pError)
{
	uint8_t *cmd, slave;
	uint8_t *response;
	int status = 0, total=0;
	int writeStatus;
	int cmdLength = 0;

	*pError = NXP_I2C_Ok;
	// slave is the 1st byte in wbuffer
	slave = buffer[0] >> 1;

	size -= 1;
	buffer++;

	cmd = kmalloc(size+6, 0);
	response = kmalloc(8, 0);

	cmd[0] = (uint8_t)cmdWrite;      // lsb
	cmd[1] = (uint8_t)(cmdWrite>>8); // msb
	cmd[2] = slave; 		 // 1st byte is the i2c slave address
	cmd[3] = (uint8_t)(size & 0xff); // lsb
	cmd[4] = (uint8_t)(size >> 8);   // msb

	// write header
	// write payload
	memcpy(&cmd[5], buffer, size);
	// write terminator
	cmd[5+size] = terminator;

	cmdLength = 5+size+1; 

	status = lxScriboUDPWriteReadGeneric(fd, (char *)cmd, cmdLength, (char *)response, 7, &writeStatus);

	kfree(cmd);
	kfree(response);

	// Check for write error.
	if(writeStatus > 0) {
		total += writeStatus;
	}
	else {
		*pError = NXP_I2C_NoAck;
		return writeStatus;
	}

	// Check acknowledge
	if((status < 0) && (*pError == NXP_I2C_Ok)) {
		*pError = NXP_I2C_NoAck;
	}

	return total;
}

int lxScriboUdpWriteRead(int fd, int wsize, const uint8_t *wbuffer, int rsize,
		uint8_t *rbuffer, uint32_t *pError)
{
	uint8_t *cmd, rcnt[2], slave, *rptr;
	uint8_t *response;
	int length = 0;
	int writeStatus;
	int cmdSize = 0, responseSize = 0;

	if ((rsize == 0) || (rbuffer == NULL)) {
		return lxScriboUdpWrite(fd, wsize, wbuffer, pError);
	}

	*pError = NXP_I2C_Ok;
	// slave is the 1st byte in wbuffer
	slave = wbuffer[0] >> 1;

	wsize -= 1;
	wbuffer++;

	cmdSize = 6+wsize+2;
	responseSize = 6+rsize+2;
	
	cmd = kmalloc(cmdSize, 0);
	response = kmalloc(responseSize, 0);

	if ((slave<<1) + 1 == rbuffer[0]) // write & read to same target
	{
		//Format = 'wr'(16) + sla(8) + w_cnt(16) + data(8 * w_cnt) + r_cnt(16) + 0x02
		cmd[0] = (uint8_t)(cmdWriteRead & 0xFF);
		cmd[1] = (uint8_t)(cmdWriteRead >> 8);
		cmd[2] = slave;
		cmd[3] = (uint8_t)(wsize & 0xff); // lsb
		cmd[4] = (uint8_t)(wsize >> 8);   // msb

		memcpy(&cmd[5], wbuffer, wsize);

		//  readcount
		rsize -= 1;
		rcnt[0] = (uint8_t)(rsize & 0xff); // lsb
		rcnt[1] = (uint8_t)(rsize >> 8);   // msb
		cmd[5+wsize] = rcnt[0];
		cmd[6+wsize] = rcnt[1];

		//  terminator
		cmd[6+wsize+2-1] = terminator;
		length = lxScriboUDPWriteReadGeneric(fd, (char *)cmd, cmdSize, (char *)response, responseSize, &writeStatus);
		length = length - 6 - 1; //==rsize

		if(length < 1) {
			kfree(cmd);
			kfree(response);
			*pError = NXP_I2C_UnsupportedValue;
			return NXP_I2C_UnsupportedValue;
		} else {
			// slave is the 1st byte in rbuffer, remove here
			rptr = rbuffer+1;
			memcpy(rptr, &response[6], length);
		}
	}

	kfree(cmd);
	kfree(response);

	return length>0 ? (length + 1) : 0; // we need 1 more for the length because of the slave address
}

int lxScribo_udp_message(int devidx, int length, const char *buffer) {
	int error = 0, status;
	uint8_t *cmd, *response;
	int writeStatus;

	(void)devidx;

	cmd = kmalloc(length+5, 0);
	response = kmalloc(8, 0);

	cmd[0] = (uint8_t)(cmdDspWrite & 0xFF);
	cmd[1] = (uint8_t)(cmdDspWrite >> 8);
	cmd[2] = (uint8_t)(length & 0xff); // lsb
	cmd[3] = (uint8_t)(length >> 8);   // msb
	memcpy(&cmd[4], buffer, length);
	cmd[4+length] = terminator;

	status = lxScriboUDPWriteReadGeneric(fd_local, (char *)cmd, length+5, (char *)response, 7, &writeStatus);

	if ( writeStatus != length+5) {
		PRINT_ERROR("%s: length mismatch: exp:%d, act:%d\n",__FUNCTION__, length+5, writeStatus);
		/* communication with the DSP failed */
		error =  6; 
	}

	kfree(cmd);
	kfree(response);

	if (status < 0)
		error = NXP_I2C_NoAck;

	return error;
}

int lxScribo_udp_message_read(int devidx, int length, char *buffer) {
	int rlength;
	char *response = kmalloc(length+7, 0); /* Add 7 bytes */
	uint8_t cmd[5];
	int writeStatus;

	(void)devidx;

	cmd[0] = (uint8_t)(cmdDspRead & 0xFF);
	cmd[1] = (uint8_t)(cmdDspRead >> 8);
	cmd[2] = (uint8_t)(length & 0xff); 	// lsb
	cmd[3] = (uint8_t)(length >> 8);   	// msb
	cmd[4] = terminator;			// terminator

	rlength = lxScriboUDPWriteReadGeneric(fd_local, (char *)cmd, sizeof(cmd), response, length+7, &writeStatus);

	memcpy(buffer, &response[6], length);

	kfree(response);

	if(rlength < 1) {
		return NXP_I2C_UnsupportedValue;
	}

	return 0;
}

static int lxScribo_udp_execute_message(int command_length, void *command_buffer,
		int read_length, void *read_buffer) 
{
	int result=-1;

	if ( (command_length!=0) & (read_length==0) ) {
		/* Write message */
		result = lxScribo_udp_message(0, command_length, command_buffer);
		result = result ? result : (int)result_buffer[0] ;// error if non-zero
	} else if ( (command_length!=0) & (read_length!=0) ) {
		/* Write + Read message */
		result = lxScribo_udp_message(0, command_length, command_buffer);
		if ( result == 0 )
			result =lxScribo_udp_message_read(0, read_length, read_buffer);
	} else if ( (command_length==0) & (read_length!=0) ) {
		/* Read result */
		result = lxScribo_udp_message_read(0, read_length, read_buffer);
	}

	/* return value is returned length, dsp result status or <0 if error */
	return result;
}

#if !(defined(WIN32) || defined(_X64))
int udp_socket_init(char *server, int port);
int lxScribo_udp_init(char *server)
{
	char *portnr;
	int port;

	/* Register the write/read messages to write over udp */
	tfa_ext_register(NULL, lxScribo_udp_message, lxScribo_udp_message_read, &tfa_ext_handle_event);

	if ( server==0 ) {
		fprintf (stderr, "%s:called for server, exiting for now...", __FUNCTION__);
		return -1;
	}

	portnr = strrchr(server , '@');
	if ( portnr == NULL )
	{
		fprintf (stderr, "%s: %s is not a valid servername, use host@port\n",__FUNCTION__, server);
		return -1;
	}

	*portnr++ ='\0'; //terminate
	port=atoi(portnr);

	fd_local = udp_socket_init(server, port);

	return fd_local;
}

const struct nxp_i2c_device lxScriboSocket_udp_device = {
		lxScribo_udp_init,
		lxScriboUdpWriteRead,
		lxScriboUdpVersion,
		lxScriboUdpSetPin,
		lxScriboUdpGetPin,
		lxScriboUdpClose,
		NULL,
		lxScriboUdpSendStartPlaybackCmd,
		lxScriboUdpSendStopPlaybackCmd,
		NXP_I2C_Msg, /* can do rpc msg */
		lxScribo_udp_execute_message
};
#endif
