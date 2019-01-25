/*
 * cmd.c
 *
 *  Created on: Sep 29, 2011
 *      Author: Rene Geraets
 */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "ErrorCodes.h"
#include "cmd.h"
#include "i2cserver.h"
#include <ctype.h>
#include <stdio.h>
#include "dbgprint.h"
#include "climax.h"

#if !(defined(WIN32) || defined(_X64)) // LINUX!
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#endif // !(defined(WIN32) || defined(_X64))

#ifdef LPCDEMO
#include "FreeRTOS.h"
#include "i2c.h"
#include "cdcuser.h"
#include "rprintf.h"
#include "ErrorCodes.h"
#include "lcd.h"
#include "gui.h"
#include "gpio.h"

#define SCRIBO_VERSION_STRING "Scribo,1.6,20110926,LPC1768"

#else
#include <stdio.h>
#include "lxScribo.h"
#include "udp_sockets.h"
#define SCRIBO_VERSION_STRING "Scribo,2.0,20120515,SocketServer"
//#define SCRIBO_VERSION_STRING "Scribo,1.71,20120209,Development board"//,Scipio,0.23,20120328,AT-Mega2561"

#endif //LPCDEMO

#define CMDSIZE 5000
#define LOGERR 1

// External audio player cmd's
#define PLAYBACK_CMD_START       "START"
#define PLAYBACK_CMD_STOP        "STOP"
#define PLAYBACK_CMD_REPEATCOUNT "COUNT"
#define PLAYER_CMD_PIPE_NAME     "/tmp/cmdfifo"

/*
 *  this is the host return write
 */
extern int activeSocket;	//TODO fix global
extern int socket_verbose;
#define VERBOSE if (socket_verbose)

//struct server_protocol *dev=NULL;
//int server_target_fd=-1;						 /* file descriptor for target device */
//int climain(int argc, char *argv[]);


void hostWrite(int fd, void* pBuffer, int size) {
    int rc;
	if (socket_verbose) {
		int i;
		uint8_t *data=pBuffer;
		printf("xmit: ");
		for (i = 0; i < size; i++)
			printf("0x%02x ",  data[i]);
		printf("\n");
		}
	server_target_fd++;
	rc = (*server_dev->write)(fd, (unsigned char*)pBuffer, size);
	if (rc < 0 ){/*check return code*/
		printf("Write error");
	}
}

/*
 * ascii commands
 */
int cliload(char *name); // in climax.c
void cmd_load(void *buf, int len) {
	char cmd[256];
	char *argv[2], *ptr=cmd;

	rprintf("%s\n",__FUNCTION__);

	strncpy(cmd,buf,len);
	cmd[len] = '\0';
	// parse line
	while ( isalnum((unsigned char)*ptr++) ); // point after ld
	while ( isblank((unsigned char)*ptr++) ); // skip space
	ptr--;
	argv[0]="ld";
	argv[1]=ptr;
	while ( iscntrl((unsigned char)*ptr++)==0 ); // point after name
	ptr--;
	*ptr='\0';

	if (cliload(argv[1])==0)
		hostWrite(activeSocket,"fail\n",5);
	else
		hostWrite(activeSocket,"ok\n",3);

}
int tfa_start(int profile, int *vstep, int channels); // runtime
int tfa_stop(void);
void cmd_start(void *buf) {
	int result=0;
	rprintf("%s\n",__FUNCTION__);
	int profile, vstep_l, vstep_r ,vsteps[2];
	// parse line
	if (sscanf(buf, "st %d %d %d", &profile, &vstep_l, &vstep_r) == 3) {
		vsteps[0] = vstep_l;
		vsteps[1] = vstep_r;
		result = tfa_start(profile, vsteps, 2);
	}
	else
		result = 1;

	if(result==0)
		hostWrite(activeSocket,"ok\n",3);
	else
		hostWrite(activeSocket,"fail\n",5);
}
void cmd_stop(void) {
	rprintf("%s\n",__FUNCTION__);

	if (tfa_stop())
		hostWrite(activeSocket,"fail\n",5);
	else
		hostWrite(activeSocket,"ok\n",3);
	lxScriboSocketExit(0);

}

#if !(defined(WIN32) || defined(_X64)) // LINUX only
bool writeAudioPlayerCmdPipe(char *cmd)
{
	bool retval = false;
	int cmdPipeFd = 0;
	int retry = 3;
	int cmdLength = strlen(cmd);

	do {
		cmdPipeFd = open(PLAYER_CMD_PIPE_NAME, O_WRONLY | O_NONBLOCK);
		if(cmdPipeFd != -1) {	
			if (socket_verbose) {
				PRINT("Player cmd: [%s]\r\n",cmd);
			}
			if(cmdLength != write(cmdPipeFd, cmd, cmdLength*sizeof(char)) ) {
				PRINT_ERROR("Error: Write pipe '%s' failed, errno: %d\r\n", PLAYER_CMD_PIPE_NAME, errno);
			}
			else {
				retval = true; // Success.
			}
			close(cmdPipeFd);
			cmdPipeFd = 0;
		}
		if(cmdLength != write(cmdPipeFd, cmd, cmdLength*sizeof(char)) ) {
			PRINT_ERROR("Error: Write pipe '%s' failed, err %d\r\n", PLAYER_CMD_PIPE_NAME, errno);
		}
		else {
			PRINT_ERROR("%s - Failed to open fifo '%s', retries left: %d\r\n", __FUNCTION__, PLAYER_CMD_PIPE_NAME, retry);
		}
		retry--;
		usleep(1000*1000); // 1000ms.
	} while(retval == false && retry > 0);

	if(retval == false) {
		PRINT_ERROR("%s - Failed to open/write fifo '%s' (Is the player running?)\r\n", __FUNCTION__, PLAYER_CMD_PIPE_NAME);
	}

	return retval;
}
#endif // !(defined(WIN32) || defined(_X64))

bool cmd_startplayback(char *buf)
{
	bool retVal = false;
#if !(defined(WIN32) || defined(_X64))	// LINUX
	char audiopath[FILENAME_MAX];
	char filename[FILENAME_MAX];
	char cmd[512];
//	int repeatCount = 1;

	if(gCmdLine.audiopath_given == true) {	
		// User override 
		strcpy(audiopath, gCmdLine.audiopath_arg);
	}
	else {
		// Default path derived from current user homedir.
		if ( strcmp("root", getenv("USER")) == 0 ) {
			// Exception for root user homedir.
			strcpy(audiopath, "/root/");
		}
		else {
			// Normal homedir.
			sprintf(audiopath, "/home/%s/", getenv("USER"));
		}
	}

	sprintf(filename, "%s%s", audiopath, buf);

	if(cli_verbose) {
		PRINT("Play file: '%s'\r\n",filename);
	}

	int err = access( filename, R_OK ); // File exists and is readable?
	if(err != 0 ) {
		PRINT_ERROR("failed to open file \"%s\", access() %d, errno %d\r\n", (char *)filename, err, errno);
		retVal = false;	
	}
	else {
//		sprintf(cmd, "%s:%d\n", PLAYBACK_CMD_REPEATCOUNT, repeatCount);
//		retVal = writeAudioPlayerCmdPipe(cmd);
//		if(retVal) {
			// If previous CMD ok, send next, otherwise fail.
			sprintf(cmd, "%s:%s\n", PLAYBACK_CMD_START, filename);
			retVal = writeAudioPlayerCmdPipe(cmd);
//		}
	}
#endif //  !(defined(WIN32) || defined(_X64))
	return retVal;
}

bool cmd_stopplayback(void)
{
#if !(defined(WIN32) || defined(_X64))	// LINUX
	char cmd[128];
	sprintf(cmd, "%s\n", PLAYBACK_CMD_STOP);
	return writeAudioPlayerCmdPipe(cmd);
#else
	return false;
#endif 
}

static uint8_t cmdbuf[CMDSIZE];
static int idx;

static uint8_t resultbuf[CMDSIZE];

void Cmd_Init(void)
{
	idx = 0;
}


static void Result(uint16_t cmd, uint16_t err, uint16_t cnt)
{
	resultbuf[0] = cmd >> 8;
	resultbuf[1] = cmd & 0xFF;
	resultbuf[2] = err & 0xFF;
	resultbuf[3] = err >> 8;
	resultbuf[4] = cnt & 0xFF;
	resultbuf[5] = cnt >> 8;
	resultbuf[6 + cnt] = 0x02;
	hostWrite(activeSocket, resultbuf, 7 + cnt);
}


#define CMD(b1, b2) ((b1) | ((b2) << 8))
void CmdProcess(void* buf, int len)
{
	uint16_t cmd;
	int n;
	int st;
	uint8_t sla=0;
	uint16_t err;
	uint16_t cnt;
	uint16_t cntR;
	int nR;
	int processed;
	int pinValue;

	st = 0;
	while (st < len)
	{
		do
		{
			processed = 0;
			n = CMDSIZE - idx;
			if (n > len - st)
			{
				n = len - st;
			}
			memcpy(cmdbuf + idx, (uint8_t *)buf + st, len);
			st += n;
			idx += n;
			if (idx >= 2)
			{
				cmd = CMD(cmdbuf[1], cmdbuf[0]);
					
				switch(cmd)
				{
				case CMD(0, 'r'):  //I2C read
					//Format = 'r'(16) + sla(8) + cnt(16) + 0x02
			    	if (idx >= 6)
			    	{
			    		err = eNone;
			    		// Expect(3);
			    		sla = cmdbuf[2];
			    		cnt = cmdbuf[3] | (cmdbuf[4] << 8);
						if (sla > 127) err = eBadSlaveAddress;
						if (sla == 0xFF) err = eBadFormat | eComErr; //Yes, the previous err might be overwritten. Intended
						if (cnt == 0xFFFF) err = eMissingReadCount | eBadFormat;
						if (cmdbuf[5] != 0x02)
						{
							err |= eBadTerminator;
						}
						n = 0;
						if (err != eNone)
						{
							cnt = 0;
						}
						else
						{
							if (cnt > sizeof(resultbuf) - 7)
							{
								cnt = sizeof(resultbuf) - 7;
								err = eBufferOverRun;
							}
							if (!i2c_Read(sla, resultbuf + 6, cnt, &n))
							{
								err = eI2C_SLA_NACK;
							}
						}
						Result(cmd, err, n);
#if LOGERR
						if (err != eNone)
						{
							rprintf("R:%02X:%d - 0x%04X\r\n", sla, cnt, err);
						}
#endif
						processed = 6;
			    	}
			    	break;

				case CMD(0, 'w'):  //I2C write
					//Format = 'w'(16) + sla(8) + cnt(16) + data(8 * cnt) + 0x02
					if (idx >= 5)
					{
						err = eNone;
						sla = cmdbuf[2];
						cnt = cmdbuf[3] | (cmdbuf[4] << 8);
						if (sla > 127) err = eBadSlaveAddress;
						if (sla == 0xFF) err = eBadFormat | eComErr; //Yes, the previous err might be overwritten. Intended
						if (cnt == 0xFFFF) err = eMissingReadCount | eBadFormat;
						if (cnt > CMDSIZE - 6)
						{
							err = eBufferOverRun;
#if LOGERR
							rprintf("W:%02X:%d - 0x%04X\r\n", sla, cnt, err);
#endif
							processed = idx;
						}
						else
						{
							if (idx >= 6 + cnt)
							{
								if (cmdbuf[5 + cnt] != 0x02)
								{
									err |= eBadTerminator;
								}
								if (err == eNone)
								{
									if (!i2c_Write(I2CBUS, sla, cmdbuf + 5, cnt))
									{
										err = eI2C_SLA_NACK;
									}
								}
								Result(cmd, err, 0);
#if LOGERR
								if (err != eNone)
								{
									rprintf("W:%02X:%d - 0x%04X\r\n", sla, cnt, err);
								}
#endif
								processed = 6 + cnt;
							}
						}
					}
					break;

				case CMD('w', 'r'): //I2C write-read
					//Format = 'wr'(16) + sla(8) + w_cnt(16) + data(8 * w_cnt) + r_cnt(16) + 0x02
					if (idx >= 5)
					{
						err = eNone;
						sla = cmdbuf[2];
						cnt = cmdbuf[3] | (cmdbuf[4] << 8);
						if (sla > 127) err = eBadSlaveAddress;
						if (sla == 0xFF) err = eBadFormat | eComErr; //Yes, the previous err might be overwritten. Intended
						if (cnt == 0xFFFF) err = eMissingReadCount | eBadFormat;
						if (cnt >= CMDSIZE - 8)
						{
							err = eBufferOverRun;
#if LOGERR
							rprintf("WR:%02X:%d - 0x%04X\r\n", sla, cnt, err);
#endif
							processed = idx;
						}
						else
						{
							if (idx >= 8 + cnt)
							{
								if (cmdbuf[7 + cnt] != 0x02)
								{
									err |= eBadTerminator;
								}
								cntR = cmdbuf[5 + cnt] | (cmdbuf[6 + cnt] << 8);
								if (cntR > sizeof(resultbuf) - 7)
								{
									cntR = sizeof(resultbuf) - 7;
									err = eBufferOverRun;
								}
								nR = 0;
								if (err == eNone)
								{
									if (!i2c_WriteRead(sla, cmdbuf + 5, cnt, resultbuf + 6, cntR, &nR))
									{
										err = eI2C_SLA_NACK;
									}
								}
								Result(cmd, err, nR);
#if LOGERR
								if (err != eNone)
								{
									rprintf("WR:%02X:%d,%d - 0x%04X\r\n", sla, cnt, cntR, err);
								}
#endif
								processed = 8 + cnt;
							}
						}
					}
					break;

				case CMD(0, 'v'):  //Version info
					//format: 'v'(16) + 0x02
					if (idx >= 3)
					{
						err = eNone;
						if (cmdbuf[2] != 0x02)
						{
							err = eBadTerminator;
						}
						memcpy(resultbuf + 6, SCRIBO_VERSION_STRING, strlen(SCRIBO_VERSION_STRING) + 1);
						Result(cmd, err, strlen(SCRIBO_VERSION_STRING) + 1);
#if LOGERR
						if (err != eNone)
						{
							rprintf("V - 0x%04X\r\n", err);
						}
#endif
						processed = 3;
					}
					break;

				case CMD('s','p'): //Set I2C speed
					//format: 'sp'(16) + speed(32) + 0x02
					if (idx >= 7)
					{
						int speed;
						int res;
						speed = cmdbuf[2] | (cmdbuf[3] << 8) | (cmdbuf[4] << 16) | (cmdbuf[5] << 24);
						err = eNone;
						if (cmdbuf[6] != 0x02)
						{
							err = eBadTerminator;
						}
						else
						{
							if (speed != 0)
							{
								if (speed < 50000)
								{
									err = eI2CspeedTooLow;
								}
								else if (speed > 400000)
								{
									err = eI2CspeedTooHigh;
								}
								else
								{
									i2c_SetSpeed(I2CBUS, speed);
								}
							}
							res = i2c_GetSpeed(I2CBUS);
							resultbuf[6] =  res         & 0xFF;
							resultbuf[7] = (res  >>  8) & 0xFF;
							resultbuf[8] = (res  >> 16) & 0xFF;
							resultbuf[9] = (res  >> 24) & 0xFF;
							Result(cmd, err, 4);
#if LOGERR
							if (err != eNone)
							{
								rprintf("SP:%d - 0x%04X\r\n", speed, err);
							}
#endif
							processed = 7;
						}
					}
					break;

				case CMD('b', 'l'): //Return I2C buffer length
					//format: 'bl'(16) + 0x02
					if (idx >= 3)
					{
						err = eNone;
						if (cmdbuf[2] != 0x02)
						{
							err = eBadTerminator;
						}

						resultbuf[6] = (CMDSIZE - 16) & 0xFF;
						resultbuf[7] = (CMDSIZE - 16) >> 8;
						Result(cmd, err, 2);
#if LOGERR
						if (err != eNone)
						{
							rprintf("BL - 0x%04X\r\n", err);
						}
#endif
						processed = 3;
					}
			      break;

				case CMD('p', 's'): //Pin set
					//format = 'ps'(16) + id(8) + val(16) + 0x02
					if (idx >= 6)
					{
						if (NXP_I2C_SetPin(cmdbuf[2],  cmdbuf[3] + (cmdbuf[4] << 8)) != NXP_I2C_Ok)
						{
							Result(cmd, eBadCmd, 0);
						}
						else
						{
							Result(cmd, eNone, 0);
						}
						processed = 6;
					}
					break;

				case CMD('p', 'r'): //Pin read
					//format = 'pr'(16) + id(8) + 0x02
					if (idx >= 4)
					{
						pinValue = NXP_I2C_GetPin(cmdbuf[2]);
						if(pinValue >= 0)
						{
							resultbuf[6] = (pinValue & 0xFF); // LSB
							resultbuf[7] = (pinValue >> 8);   // MSB
							Result(cmd, eNone, 2);
						}
						else {
							Result(cmd, eInvalidPinNumber, 0);
						}
						processed = 4;
					}
					break;

				case CMD('d', 'r'):  // DSP read (getParam)  
					// Format = 'dr'(16) + cnt(16) + 0x02
			    		if (idx >= 5) {
			    			err = eNone;
			    			cnt = cmdbuf[2] | (cmdbuf[3] << 8);

						if (cnt == 0xFFFF)
							err = eMissingReadCount | eBadFormat;
						if (cmdbuf[4] != 0x02)
							err |= eBadTerminator;

						n = 0;
						if (err != eNone) {
							cnt = 0;
						} else {
							if (cnt > sizeof(resultbuf) - 7) {
								cnt = sizeof(resultbuf) - 7;
								err = eBufferOverRun;
							} else {
								err = dsp_msg_read(0, cnt, resultbuf + 6);
							}
						}
						Result(cmd, err, cnt);
#if LOGERR
						if (err != eNone)
							rprintf("DR:%02X:%d - 0x%04X\r\n", sla, cnt, err);
#endif
						processed = 5;
			    		}
			    		break;
               
				case CMD('d', 'w'):  // DSP write (setParam)  
					// Format = 'dw'(16) + cnt(16) + data(8 * cnt) + 0x02
					if (idx >= 4) {
						err = eNone;
						cnt = cmdbuf[2] | (cmdbuf[3] << 8);

						if (cnt == 0xFFFF) 
							err = eMissingReadCount | eBadFormat;
						if (cnt > CMDSIZE - 6) {
							err = eBufferOverRun;
#if LOGERR
							rprintf("DW:%02X:%d - 0x%04X\r\n", sla, cnt, err);
#endif
							processed = idx;
						} else {
							if (idx >= 5 + cnt) {
								if (cmdbuf[4 + cnt] != 0x02)
									err |= eBadTerminator;

								if (err == eNone)
									err = dsp_msg(0, cnt, (const char *)(cmdbuf + 4));

								Result(cmd, err, 0);
#if LOGERR
								if (err != eNone)
									rprintf("DW:%02X:%d - 0x%04X\r\n", sla, cnt, err);
#endif
								processed = 5 + cnt;
							}
						}
					}
					break;
				case CMD('t', 's'): // 'st' ascii command: start
					//Format = 'st'(16) + cnt(8) + data(8 * cnt) + 0x02

					if (idx >= 3)
					{
						err = eNone;
						cnt = cmdbuf[2];
						if (cnt == 0xFF || cnt == 0x00) err = eMissingReadCount | eBadFormat;
						if (cnt > CMDSIZE - 4)
						{
							err = eBufferOverRun;
							processed = idx;
						}
						else 
						{
							if (idx >= 4 + cnt)
							{
								if (cmdbuf[3 + cnt] != 0x02)
								{
									err |= eBadTerminator;
								}
								if (err == eNone)
								{
									bool res = false;
									char fname[256] = {0}; // 255 char (0xFF) = maximum size of .wav filename

									// copy file name from cmdbuf and start .wav playback
									memcpy(fname, cmdbuf+3, cnt);
									fname[255] = 0; // ensure string is terminated
									res = cmd_startplayback(fname);
									if (res != true)
									{
										err = eBadCmd;
									}
									Result(cmd, err, 0);
								}
								processed = 4 + cnt;
							}
						}
					}
					break;
				case CMD('p', 'o'): // 'op' ascii command: stop
					//Format = 'op'(16) + 0x02
					if (idx >= 3)
					{
						err = eNone;
						if (cmdbuf[2] != 0x02)
						{
							err = eBadTerminator;
						}
						
						// stop .wav file playback
						bool res = cmd_stopplayback();
						if (res != true)
						{
							err = eBadCmd;
						}
						Result(cmd, err, 0);

						processed = 3;
					}
					break;
				case CMD('d', 'l'): // 'ld' ascii command: (re)load container
					cmd_load(buf, len);
					idx=0;processed=0;
					break;
				case CMD('x', 'e'): // 'ex' ascii command: exit
					idx=0;
					lxScriboSocketExit(0);
					break;
#if 0					
				case CMD('o', 'd'): // 'do' ascii command
					{
					int c,argc=0;
					char *cmd=buf;
					char *args[]={"server", "-ddummy", "-r0"};
					climain(3, args);
				  //  hostWrite(activeSocket, cmd, len );
					processed=0;
					}
					break;
#endif
					default:   //Unsupported command
#if LOGERR
					rprintf("%c%c: eBadCmd (%02X-%02X)\r\n", (cmd & 0xFF) == 0 ? ' ' : cmd & 0xFF, cmd >> 8, cmdbuf[1], cmdbuf[0]);
#endif
					Result(cmd, eBadCmd, 0);
					processed = idx;
					break;
				}
			}
			if ((processed != 0) & (processed < idx))
			{
				memmove(cmdbuf, cmdbuf + processed, idx - processed);
			}
			idx -= processed;
		} while((idx > 0) && (processed > 0));
	}
}
