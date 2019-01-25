/* implement Scribo protocol */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#if (defined(WIN32) || defined(_X64))
#include <windows.h>
/* Winsock can not use read/write from a socket */
#define read(fd, ptr, size) recv(fd, ptr, size, 0)
#define write(fd, ptr, size) send(fd, ptr, size, 0)
#include <time.h>
#else
#include <sys/time.h>
#include <unistd.h>
#define Sleep(x) usleep((x)*1000)
#endif
#include "hal_utils.h"
#include "dbgprint.h"
#include "NXP_I2C.h"
#include "lxScribo.h"
#include <assert.h>
#include "config.h"

extern int lxScribo_verbose; /* lxSribo.c */
#define VERBOSE if (lxScribo_verbose)
/*
 * Command headers for communication
 *
 * note: commands headers are little endian
 */
const uint16_t cmdVersion   = 'v' ;              /* Version */
const uint16_t cmdRead      = 'r' ;              /* I2C read transaction  */
const uint16_t cmdWrite     = 'w' ;              /* I2C write transaction */
const uint16_t cmdWriteRead = ('w' << 8 | 'r');  /* I2C write+read transaction */
const uint16_t cmdPinSet    = ('p' << 8 | 's') ; /* set pin */
const uint16_t cmdPinRead   = ('p' << 8 | 'r') ; /* get pin */
const uint16_t cmdStartPlay = ('t' << 8 | 's') ; /* Start playback of audio file */
const uint16_t cmdStopPlay  = ('p' << 8 | 'o') ; /* Stop  playback of audio file */

const uint8_t terminator    = 0x02; /* terminator for commands and answers */

int tcp_init_socket(char *dev);
int tcp_read_data(int fd, unsigned char *wbuffer, int wsize);/*,
		int rsize, unsigned char *rbuffer, unsigned int *pError);*/
int tcp_write_data(int fd, unsigned char *wbuffer,int wsize);
int tcp_close_socket(int fd);
#if !(defined(WIN32) || defined(_X64))
const struct server_protocol tcp_server = {
	tcp_init_socket,/*lxScriboListenSocketInit*/
	tcp_read_data,
	tcp_write_data,
	tcp_close_socket,
};
int tcp_init_socket(char *dev){
	extern int lxScriboListenSocketInit(char *socketnr);
	int socket_id;
	socket_id = lxScriboListenSocketInit(dev);
	return socket_id;
}
int tcp_read_data(int fd, unsigned char *wbuffer, int wsize){
	int length=0;
	length = read(fd, wbuffer, wsize);
	return length;
}
int tcp_write_data(int fd, unsigned char *wbuffer,int wsize){
	int rc;
	rc = write(fd, wbuffer, wsize);
	return rc;
}
int tcp_close_socket(int fd){
	(void)fd;/*fd is not needed in case of TCP/IP*/
	lxScriboSocketExit(1);
	return 0;
}
#endif
static int lxScriboGetResponseHeader(int fd, const uint16_t cmd, int* prlength)
{
	uint8_t response[6];
	uint16_t rcmd, rstatus;
	int length=0;

	length = read(fd, (char *)response, sizeof(response));
	VERBOSE hexdump("rsp:", response, sizeof(response));

	// response (lsb/msb)= echo cmd[0] cmd[1] , status [0] [1], length [0] [1] ...data[...]....terminator[0]
	if ( length==sizeof(response) )
	{
		rcmd    = response[0] | response[1]<<8;
		rstatus = response[2] | response[3]<<8;
		*prlength = response[4] | response[5]<<8;  /* extra bytes that need to be read */

		/* must get response to expected cmd */
		if  ( cmd != rcmd) {
			ERRORMSG("scribo protocol error: expected %d bytes , got %d bytes\n", cmd, rcmd);
		}
		if  (rstatus != 0) {
			ERRORMSG("scribo status error: 0x%02x\n", rstatus);
		}

		return (int)rstatus; /* iso length */
	}
	else {
		ERRORMSG("bad response length=%d\n", length);
		return -1;
	}
}

/*
 * set pin
 */
int lxScriboSetPin(int fd, int pin, int value)
{
	uint8_t cmd[6];
	int stat;
	int ret;
	uint8_t term;
	int rlength;
	
	VERBOSE PRINT("SetPin[%d]<%d\n", pin, value);

	cmd[0] = (uint8_t)(cmdPinSet & 0xff);
	cmd[1] = (uint8_t)(cmdPinSet >> 8);
	cmd[2] = (uint8_t)pin;
	cmd[3] = (uint8_t)(value & 0xFF); // LSB
	cmd[4] = (uint8_t)(value >> 8);   // MSB
	cmd[5] = terminator;

	// write header
	VERBOSE PRINT("cmd: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x \n",
			cmd[0], cmd[1],cmd[2],cmd[3],cmd[4], cmd[5]);
        ret = write(fd, (char *)cmd, sizeof(cmd));
	assert(ret == sizeof(cmd));

        stat = lxScriboGetResponseHeader( fd, cmdPinSet, &rlength);
	assert(stat == 0);
	assert(rlength == 0); // expect no return data
	ret = read(fd, (char *)&term, 1);
	assert(ret == 1);
	
	VERBOSE PRINT("term: 0x%02x\n", term);

	assert(term == terminator);

	return stat>=0;
}

/*
 * get pin state
 */
int lxScriboGetPin(int fd, int pin)
{
	uint8_t cmd[4];
	uint8_t response[3];
	int value;
	int length;
	int ret;
	int rlength;

	cmd[0] = (uint8_t)(cmdPinRead & 0xff);
	cmd[1] = (uint8_t)(cmdPinRead >> 8);
	cmd[2] = (uint8_t)pin;
	cmd[3] = terminator;

	// write header
	VERBOSE PRINT("cmd:0x%02x 0x%02x 0x%02x 0x%02x\n",
			cmd[0], cmd[1],cmd[2],cmd[3]);
	ret = write(fd, (const char *)cmd, sizeof(cmd));
	assert(ret == sizeof(cmd));

	ret = lxScriboGetResponseHeader( fd, cmdPinRead, &rlength);
	assert(ret == 0);

	length = read(fd, (char *)response, 3);
	assert(length == 3);
	value = response[1] << 8 | response[0];

	VERBOSE PRINT("GetPin[%d]:%d\n", pin, value);

	return value;
}

/*
 * Close an opened device
 * Return success (0) or fail (-1)
 */
int lxScriboClose(int fd)
{
	(void)fd; /* Remove unreferenced parameter warning */
	VERBOSE PRINT("Function close not implemented for target scribo.");
	return 0;
}

/*
 * retrieve the version string from the device
 */
int lxScriboVersion(char *buffer, int fd)
{
	uint8_t cmd[3];
	int length;
	int ret;
	int rlength;

	cmd[0] = (uint8_t)(cmdVersion & 0xff);
	cmd[1] = (uint8_t)(cmdVersion >> 8);
	cmd[2] = terminator;

	ret = write(fd, (const char *)cmd, sizeof(cmd));
	assert(ret == sizeof(cmd));

	ret = lxScriboGetResponseHeader(fd, cmdVersion, &rlength);
	assert(ret == 0);
	assert(rlength > 0);

	length = read(fd, buffer, 256);
	/* no new line is added */

	return length;
}


/* send command 'STOP' to the server */
int lxScriboSendStopPlaybackCmd(int fd)
{
	uint8_t cmd[3];
	int ret;
	int rlength;

	cmd[0] = (uint8_t)(cmdStopPlay & 0xff);
	cmd[1] = (uint8_t)(cmdStopPlay >> 8);
	cmd[2] = terminator;

	ret = write(fd, (const char *)cmd, sizeof(cmd));
	assert(ret == sizeof(cmd));

	ret = lxScriboGetResponseHeader(fd, cmdStopPlay, &rlength);
	assert(ret == 0);
	assert(rlength == 0);

	return rlength;
}

/* send command 'START' to the server */
/* send command 'START' to the UDP server */
int lxScriboSendStartPlaybackCmd(char *buffer, int fd)
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

	length = lxScriboTCPWriteReadGeneric(fd, (char *)cmd, cmdLength, response, 7, &writeStatus);

	rstatus = response[2] | response[3] << 8;
	if  (rstatus != 0) {
		ERRORMSG("scribo status error: 0x%02x\n", rstatus);
	}

	return length;
}

static int lxScriboWrite(int fd, int size, const uint8_t *buffer, uint32_t *pError)
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

	status = lxScriboTCPWriteReadGeneric(fd, (char *)cmd, cmdLength, (char *)response, 7, &writeStatus);

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

#if (defined(WIN32) || defined(_X64))
#else
int tcp_write(int fd, char *cmd, int size)
{
	int status;
	struct timeval start_time, end_time;
	gettimeofday(&start_time, NULL);
	status = write(fd, (char *)cmd, size);
	gettimeofday(&end_time, NULL);
	VERBOSE printf ("%sSpecial Function Total time = %f seconds\n", __FUNCTION__,
		         (double) (end_time.tv_usec - start_time.tv_usec) / 1000000 +
		         (double) (end_time.tv_sec - start_time.tv_sec));
	return status;
}
#endif

#ifdef OLD_SCRIBO_READ_WRITE
int lxScriboWriteRead(int fd, int wsize, const uint8_t *wbuffer, int rsize,
		uint8_t *rbuffer, uint32_t *pError)
{
	uint8_t cmd[5], rcnt[2], slave, *rptr, term;
	int length = 0;
	int status, total = 0;
	int rlength;

	if ((rsize == 0) || (rbuffer == NULL)) {
		return lxScriboWrite(fd, wsize, wbuffer, pError);
	}

	*pError = NXP_I2C_Ok;
	// slave is the 1st byte in wbuffer
	slave = wbuffer[0] >> 1;

	wsize -= 1;
	wbuffer++;

	if ((slave<<1) + 1 == rbuffer[0]) // write & read to same target
			{
		//Format = 'wr'(16) + sla(8) + w_cnt(16) + data(8 * w_cnt) + r_cnt(16) + 0x02
		cmd[0] = (uint8_t)(cmdWriteRead & 0xFF);
		cmd[1] = (uint8_t)(cmdWriteRead >> 8);
		cmd[2] = slave;
		cmd[3] = (uint8_t)(wsize & 0xff); // lsb
		cmd[4] = (uint8_t)(wsize >> 8);   // msb

		// write header
		VERBOSE hexdump("cmd:", cmd, sizeof(cmd));

		status = tcp_write(fd, (char *)cmd, sizeof(cmd));


		if (status > 0)
			total += status;
		else
		{
			*pError = NXP_I2C_NoAck;
			return status;
		}
		// write payload
		VERBOSE hexdump("\t\twdata:", wbuffer, wsize);
		status = tcp_write(fd, (char *)wbuffer, wsize);
		if (status > 0)
			total += status;
		else
		{
			*pError = NXP_I2C_NoAck;
			return status;
		}

		// write readcount
		rsize -= 1;
		rcnt[0] = (uint8_t)(rsize & 0xff); // lsb
		rcnt[1] = (uint8_t)(rsize >> 8);   // msb
		VERBOSE hexdump("rdcount:", rcnt, 2);
		status = tcp_write(fd, (char *)rcnt, 2);
		if (status > 0)
			total += status;
		else
		{
			*pError = NXP_I2C_NoAck;
			return status;
		}

		// write terminator
		cmd[0] = terminator;
		VERBOSE PRINT("term: 0x%02x\n", cmd[0]);
		status = tcp_write(fd, (char *)cmd, 1);
		if (status > 0)
			total += status;
		else
		{
			*pError = NXP_I2C_NoAck;
			return status;
		}

		//if( rcnt[1] | rcnt[0] >100)    // TODO check timing
		if (rsize > 100) // ?????????????????????????????????????
			Sleep(20);
		// slave is the 1st byte in rbuffer, remove here
		//rsize -= 1;
		rptr = rbuffer+1;
		// read back, blocking
		gettimeofday(&start, NULL);
		status = lxScriboGetResponseHeader(fd, cmdWriteRead, &rlength);
		gettimeofday(&end, NULL);
		printf ("%s lxScriboGetResponseHeader = %f seconds\n", __FUNCTION__,
				         (double) (end.tv_usec - start.tv_usec) / 1000000 +
				         (double) (end.tv_sec - start.tv_sec));


		if (status != 0)
		{
			*pError = status;
		}			
		//assert(rlength == rsize);
		if  ( rlength != rsize) {
			ERRORMSG("scribo protocol error: expected %d bytes , got %d bytes\n", rsize, rlength);
		}
		gettimeofday(&start_time, NULL);
		length = read(fd, (char *)rptr, rsize);
		gettimeofday(&end_time, NULL);
		VERBOSE printf ("%s read of rptr Total time = %f seconds\n", __FUNCTION__,
				         (double) (end_time.tv_usec - start_time.tv_usec) / 1000000 +
				         (double) (end_time.tv_sec - start_time.tv_sec));

		
		if (length < 0)
		{
			if (*pError == NXP_I2C_Ok) *pError = NXP_I2C_NoAck;
			return -1;
		}
		VERBOSE hexdump("\trdata:",rptr, rsize);
		if ( read(fd, (char *)&term, 1) < 0 )
		{
			if (*pError == NXP_I2C_Ok) *pError = NXP_I2C_NoAck;
			return -1;
		}
		assert(term == terminator);
		VERBOSE PRINT("rterm: 0x%02x\n", term);
	}
	else {
		PRINT("!!!! write slave != read slave !!! %s:%d\n", __FILE__, __LINE__);
		*pError = NXP_I2C_UnsupportedValue;
		status = -1;
		return status;
	}
	return length>0 ? (length + 1) : 0; // we need 1 more for the length because of the slave address
}
#endif
int lxScriboWriteRead(int fd, int wsize, const uint8_t *wbuffer, int rsize,
		uint8_t *rbuffer, uint32_t *pError)
{
	uint8_t *cmd, rcnt[2], slave, *rptr;
	uint8_t *response;
	int length = 0;
	int writeStatus;
	int cmdSize = 0, responseSize = 0;

	if ((rsize == 0) || (rbuffer == NULL)) {
		return lxScriboWrite(fd, wsize, wbuffer, pError);
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
		length = lxScriboTCPWriteReadGeneric(fd, (char *)cmd, cmdSize, (char *)response, responseSize, &writeStatus);
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

/*
 * lxScriboTCPWriteReadGeneric
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
int lxScriboTCPWriteReadGeneric(int sock, char *cmd, int cmdlen, char *response, int rcvbufsize, int *writeStatus)
{
	int status = -1;

		// write command
		VERBOSE hexdump("cmd", (uint8_t *)cmd, sizeof(cmd));

		#if (defined(WIN32) || defined(_X64))
			status = write(sock, (char*) cmd, cmdlen);
		#else
			status = write(sock, (char*)cmd, cmdlen);
		#endif
		*writeStatus = status;
		if(status <= 0)
		{
			printf("TCP write error %s\n",__FUNCTION__);
			/* No sense in reading, if write failed. Wait for next retry */
		}
		else
		{
			#if (defined(WIN32) || defined(_X64))
				status = read(sock, (char*) response, rcvbufsize);//, UDP_RECV_TIMEOUT_SEC); // TODO TEST TIMEOUT FOR WINDOWS
			#else
				status = read(sock, (char*)response, rcvbufsize);//, UDP_RECV_TIMEOUT_SEC);
			#endif

			// read result.
			VERBOSE hexdump("rsp", (uint8_t *)response, status);
		}

	return status;
}
