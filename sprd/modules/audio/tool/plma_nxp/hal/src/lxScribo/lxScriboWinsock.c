#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>

/* there are 32-bit and 64-bit versions of this DLL in 64-bit Windows */
#pragma comment(lib, "Ws2_32.lib")

#include "lxScribo.h"
#include "NXP_I2C.h"
#include "config.h"
#include "tfa_ext.h"
/*function declarations*/
static int lxScribo_tcp_message(int devidx, int length, const char *buffer);
static int lxScribo_tcp_message_read(int devidx, int length, char *buffer);

/*
 * Command headers for communication
 *
 * note: commands headers are little endian
 */
static char result_buffer[550*4]; //TODO align the tfadsp.h /lock?

typedef void (*sighandler_t)(int);
static const uint16_t cmdDspWrite  = ('d' << 8 | 'w') ; /* Write message to DSP */
static const uint16_t cmdDspRead   = ('d' << 8 | 'r') ; /* Read message from DSP */

static const uint8_t terminator    = 0x02; /* terminator for commands and answers */

static SOCKET clientSocket = INVALID_SOCKET;

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

static int lxScriboWinsockInit(char *server)
{
	WSADATA wsaData;
	int result;
	char *hostname, *portnr;
	struct sockaddr_in sin;
	struct hostent *host;
	//int port;
	unsigned short port;
	int init_done = 0;

	/* Register the write/read messages to write over udp */
	/*replace udp message by tcp lxScribo_udp_message, lxScribo_udp_message_read*/
	tfa_ext_register(NULL, lxScribo_tcp_message, lxScribo_tcp_message_read, NULL);
	if (server == 0) {
		fprintf (stderr, "%s:called for recovery, exiting for now...\n", __FUNCTION__);
		return -1; 
	}

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

	portnr = strrchr(server, ':');
	if (portnr == NULL) {
		fprintf (stderr, "%s: %s is not a valid servername, use host:port\n",__FUNCTION__, server);
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

	clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket == INVALID_SOCKET) {
		fprintf(stderr, "socket failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
		return -1;
	}

	memcpy(&sin.sin_addr.s_addr, host->h_addr, host->h_length);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);

	if (connect(clientSocket, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		fprintf(stderr,"error connecting to %s:%d\n", hostname , port);
		closesocket(clientSocket);
		WSACleanup();
		return -1;
	}

	return (int)clientSocket;
}

static int lxScribo_tcp_message(int devidx, int length, const char *buffer) 
{
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
	status = lxScriboTCPWriteReadGeneric((int)clientSocket, (char *)cmd, length+5, (char *)response, 7, &writeStatus);

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


static int lxScribo_tcp_message_read(int devidx, int length, char *buffer) 
{
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

	rlength = lxScriboTCPWriteReadGeneric((int)clientSocket, (char *)cmd, sizeof(cmd), response, length+7, &writeStatus);

	memcpy(buffer, &response[6], length);

	kfree(response);

	if(rlength < 1) {
		return NXP_I2C_UnsupportedValue;
	}

	return 0;
}

static int lxScriboWinsock_tcp_execute_message(int command_length, void *command_buffer,
		int read_length, void *read_buffer) 
{
	int result=-1;

	if ( (command_length!=0) & (read_length==0) ) {
		/* Write message */
		result = lxScribo_tcp_message(0, command_length, command_buffer);
		result = result ? result : (int)result_buffer[0] ;// error if non-zero
	} else if ( (command_length!=0) & (read_length!=0) ) {
		/* Write + Read message */
		result = lxScribo_tcp_message(0, command_length, command_buffer);
		if ( result == 0 )
			result =lxScribo_tcp_message_read(0, read_length, read_buffer);
	} else if ( (command_length==0) & (read_length!=0) ) {
		/* Read result */
		result = lxScribo_tcp_message_read(0, read_length, read_buffer);
	}

	/* return value is returned length, dsp result status or <0 if error */
	return result;
}

const struct nxp_i2c_device lxScriboWinsock_device = {
	lxScriboWinsockInit,
	lxScriboWriteRead,
	lxScriboVersion,
	lxScriboSetPin,
	lxScriboGetPin,
	lxScriboWinsockExit,
	NULL,
	lxScriboSendStartPlaybackCmd,
	lxScriboSendStopPlaybackCmd,	
	NXP_I2C_Msg,
	lxScriboWinsock_tcp_execute_message
};

