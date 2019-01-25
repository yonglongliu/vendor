#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>

/* there are 32-bit and 64-bit versions of this DLL in 64-bit Windows */
#pragma comment(lib, "Ws2_32.lib")

#include "lxScribo.h"
#include "NXP_I2C.h"
#include "tfa_ext.h"
#include "udp_sockets.h"
#include "config.h"
#include "lxScriboWinsockUdp.h"
#include "hal_utils.h"

static SOCKET clientSocket = INVALID_SOCKET;
static struct sockaddr_in client_addr; /* remote address */

extern int lxScribo_verbose; /* lxScribo.c */
#define VERBOSE if (lxScribo_verbose)

static const uint16_t cmdDspWrite  = ('d' << 8 | 'w') ; /* Write message to DSP */
static const uint16_t cmdDspRead   = ('d' << 8 | 'r') ; /* Read message from DSP */

static const uint8_t terminator    = 0x02; /* terminator for commands and answers */
static int fd_local=-1;

/* tiberius message */
static char result_buffer[550*4]; //TODO align the tfadsp.h /lock?

static int NXP_UDP_verbose;

#define VERBOSE_TRACE if(NXP_UDP_verbose)

void NXP_UDP_Trace(int on)
{
	NXP_UDP_verbose = (on & 1);
}

static int lxScriboWinsockExit(int fd)
{
	(void)fd; /* Remove unreferenced parameter warning C4100 */
	//printf("%s closing sockets\n", __FUNCTION__);

	if (clientSocket != INVALID_SOCKET) {
		shutdown(clientSocket, SD_BOTH);
		closesocket(clientSocket);
		clientSocket = INVALID_SOCKET;
	}
	/*
	 * workaround: sleep for a while, give Windows some time to cleanup
	 * socket might already have been closed by Ws2_32.dll, 
	 * then the shutdown/closesocket will fail with WSANOTINITIALISED
	 */
	if (WSAGetLastError() == WSANOTINITIALISED) {
		Sleep(100);
	}

	WSACleanup();

	return 0;
}

/*
 * Windows variant of udp_read() 
 */
static int lxScriboWinsock_udp_message_read(int devidx, int length, char *buffer) 
{
	int error = 0, rlength, ret;
	char response[2200]; //550 buffer * 4bytes = 2200
	uint8_t cmd[5];

	(void)devidx;

	cmd[0] = (uint8_t)(cmdDspRead & 0xFF);
	cmd[1] = (uint8_t)(cmdDspRead >> 8);
	cmd[2] = (uint8_t)(length & 0xff); 	// lsb
	cmd[3] = (uint8_t)(length >> 8);   	// msb
	cmd[4] = terminator;				//  terminator

	VERBOSE hexdump("cmd:", (uint8_t *)cmd, 5);
	ret = winsock_udp_write(fd_local, (char *)cmd, sizeof(cmd));

	rlength = winsock_udp_read(fd_local, response, sizeof(response), 0);
	VERBOSE hexdump("rsp:", (uint8_t *)response, rlength+7);

	memcpy(buffer, &response[6], length);

	if(rlength < 1) {
		return NXP_I2C_UnsupportedValue;
	}

	return error;
}

/*
 * Windows variant of lxScribo_udp_message() 
 */
static int lxScriboWinsock_udp_message(int devidx, int length, const char *buffer)
{
	int error = 0, real_length, status;
	uint8_t *cmd;

	(void)devidx;

	cmd = kmalloc(length+5, 0);
	cmd[0] = (uint8_t)(cmdDspWrite & 0xFF);
	cmd[1] = (uint8_t)(cmdDspWrite >> 8);
	cmd[2] = (uint8_t)(length & 0xff); // lsb
	cmd[3] = (uint8_t)(length >> 8);   // msb

	memcpy(&cmd[4], buffer, length);

	//  terminator
	cmd[4+length] = terminator;

	VERBOSE hexdump("cmd:", (uint8_t *)cmd, 5+length);
	real_length = winsock_udp_write(fd_local, (char *)cmd, length+5);
	if ( real_length != length+5) {
		printf("%s: length mismatch: exp:%d, act:%d\n",__FUNCTION__, length+5, real_length);
		error = 6; /*. communication with the DSP failed */
	}

	status = winsock_udp_read(fd_local, (char *)cmd, 7, 0);	
	VERBOSE hexdump("rsp:", (uint8_t *)cmd, 7);

	kfree(cmd);

	if (status < 0)
		error = NXP_I2C_NoAck;

	return error;
}

static int lxScriboWinsockInitUDP(char *server)
{
	WSADATA wsaData;
	int result;
	char *hostname, *portnr;
	struct hostent *host;
	unsigned short port;
	int init_done = 0;

	if (server == 0) {
		fprintf (stderr, "%s:called for recovery, exiting for now...\n", __FUNCTION__);
		return -1; 
	}
	
	/* Register the write/read messages to write over udp */
	tfa_ext_register(NULL, lxScriboWinsock_udp_message, lxScriboWinsock_udp_message_read, NULL);

	/* Initialize Winsock */
	result = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (result != 0) {
		fprintf(stderr, "WSAStartup failed with error: %d\n", result);
		return -1;
	}

	/* lxScribo register can be called multiple times */
	init_done = (clientSocket != INVALID_SOCKET);

	if (init_done) {
		fprintf(stderr, "%s: closing already open socket\n", __FUNCTION__);
		closesocket(clientSocket);
	}

	portnr = strrchr(server, '@');
	if (portnr == NULL) {
		fprintf (stderr, "%s: %s is not a valid servername, use host@port\n",__FUNCTION__, server);
		return -1;
	}
	hostname=server;
	*portnr++ ='\0'; //terminate
	port=(unsigned short)atoi(portnr);
	host = gethostbyname(hostname);
	if (!host) {
		fprintf(stderr, "Error: wrong hostname: %s\n", hostname);
		return -1;
	}

	if (port==0) // illegal input
		return -1;

	clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (clientSocket == INVALID_SOCKET) {
		fprintf(stderr, "socket failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
		return -1;
	}

	memcpy(&client_addr.sin_addr.s_addr, host->h_addr, host->h_length);
	client_addr.sin_family = AF_INET;
	client_addr.sin_port = htons(port);

	fd_local = (int)clientSocket;

	return (int)clientSocket;
}

/*
 * Winsock variant of udp_read() 
 */
int winsock_udp_read(int fd, char *inbuf, int len, int waitsec) {
	socklen_t addrlen = sizeof(client_addr); /* length of addresses */
	int recvlen; /* # bytes received */
	DWORD dwTime = (waitsec*1000); // timeout value in miliseconds

	if (waitsec) {
		if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&dwTime, sizeof(dwTime)) < 0) {
			perror("Error");
		}
	}

	recvlen = recvfrom(fd, inbuf, len, 0, (struct sockaddr *) &client_addr, &addrlen);
	if (recvlen >= 0) {
	//	inbuf[recvlen] = 0;
	//	printf("received message: \"%s\" (%d bytes)\n", inbuf, recvlen);
	} else
		perror("rcv error");

	VERBOSE_TRACE hexdump("UDP R", (const unsigned char *)inbuf, len);

	return recvlen;
}

/*
 * Winsock variant of udp_write() 
 */
int winsock_udp_write(int fd, char *outbuf, int len) {
	VERBOSE_TRACE hexdump("UDP w", (const unsigned char *)outbuf, len);
	return sendto(fd, outbuf, len, 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
}

static int lxScriboUdpClose(int fd)
{
	(void)fd; /* Remove unreferenced parameter warning C4100 */
	fprintf(stderr, "Function close not implemented for target ScriboUdp.");
	return 0;
}

static int lxScriboWinsock_udp_execute_message(int command_length, void *command_buffer,
		int read_length, void *read_buffer) 
{
	int result=-1;

	if ( (command_length!=0) & (read_length==0) ) {
		/* Write message */
		result = lxScriboWinsock_udp_message(0, command_length, command_buffer);
		result = result ? result : (int)result_buffer[0] ;// error if non-zero
	} else if ( (command_length!=0) & (read_length!=0) ) {
		/* Write + Read message */
		result = lxScriboWinsock_udp_message(0, command_length, command_buffer);
		if ( result == 0 )
			result =lxScriboWinsock_udp_message_read(0, read_length, read_buffer);
	} else if ( (command_length==0) & (read_length!=0) ) {
		/* Read result */
		result = lxScriboWinsock_udp_message_read(0, read_length, read_buffer);
	}

	/* return value is returned length, dsp result status or <0 if error */
	return result;
}

const struct nxp_i2c_device lxScriboWinsock_udp_device = {
	lxScriboWinsockInitUDP,
	lxScriboUdpWriteRead,
	lxScriboUdpVersion,
	lxScriboUdpSetPin,
	lxScriboUdpGetPin,
	lxScriboWinsockExit,
	NULL,
	lxScriboUdpSendStartPlaybackCmd,
	lxScriboUdpSendStopPlaybackCmd,
	NXP_I2C_Msg, /* can do rpc msg */
	lxScriboWinsock_udp_execute_message
};

