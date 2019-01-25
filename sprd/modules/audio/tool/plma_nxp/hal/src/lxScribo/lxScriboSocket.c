/*Simple Echo Server, demonstrating basic socket system calls.
 * Binds to random port between 9000 and 9999,
 * echos back to client all received messages
 * 'quit' message from client kills server gracefully
 *
 * To test server operation, open a telnet connection to host
 * E.g. telnet YOUR_IP PORT
 * PORT will be specified by server when run
 * Anything sent by telnet client to server will be echoed back
 *
 *
 * Written by: Ajay Gopinathan, Jan 08
 * ajay.gopinathan@ucalgary.ca
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <time.h>
#include <netdb.h>
#include <signal.h>
#include <arpa/inet.h>

#include "lxScribo.h"
#include "NXP_I2C.h"
#include "config.h"
#include "hal_utils.h"
#include "tfa_ext.h"

static int lxScribo_verbose = 0;
#define VERBOSE if (lxScribo_verbose)

#define INVALID_SOCKET -1
static int listenSocket = INVALID_SOCKET;
int activeSocket = INVALID_SOCKET;
/* tiberius message */
static char result_buffer[550*4]; //TODO align the tfadsp.h /lock?

typedef void (*sighandler_t)(int);
static const uint16_t cmdDspWrite  = ('d' << 8 | 'w') ; /* Write message to DSP */
static const uint16_t cmdDspRead   = ('d' << 8 | 'r') ; /* Read message from DSP */

static const uint8_t terminator    = 0x02; /* terminator for commands and answers */
//static int fd_local=-1;

/*
 *
 */
void lxScriboSocketExit(int status)
{
	char buf[256];
	struct linger l;

	l.l_onoff = 1;
	l.l_linger = 0;

	(void)lxScribo_verbose;
	//printf("%s closing sockets at exit\n", __FUNCTION__);

	// still bind error after re-open when traffic was active
	if (listenSocket>0) {
		shutdown(listenSocket, SHUT_RDWR);
		close(listenSocket);
	}
	if (activeSocket>0) {
		setsockopt(activeSocket, SOL_SOCKET, SO_LINGER, &l, sizeof(l));
		shutdown(activeSocket, SHUT_RDWR);
		read(activeSocket, buf, 256);
		close(activeSocket);
	}

	activeSocket = INVALID_SOCKET;
	listenSocket = INVALID_SOCKET;

	/*status 0 comes from cntrl c handler*/
	if (status==0)
		_exit(status);
}

/*
 * ctl-c handler
 */
static void lxScriboCtlc(int sig)
{
    (void)sig;
	(void)signal(SIGINT, SIG_DFL);
	lxScriboSocketExit(0);
}

/*
 * exit handler
 */
static void lxScriboAtexit(void)
{
	lxScriboSocketExit(0);
}
int lxScribo_tcp_message(int devidx, int length, const char *buffer) 
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

	status = lxScriboTCPWriteReadGeneric(activeSocket, (char *)cmd, length+5, (char *)response, 7, &writeStatus);

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

int lxScribo_tcp_message_read(int devidx, int length, char *buffer) {
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

	rlength = lxScriboTCPWriteReadGeneric(activeSocket, (char *)cmd, sizeof(cmd), response, length+7, &writeStatus);

	memcpy(buffer, &response[6], length);

	kfree(response);

	if(rlength < 1) {
		return NXP_I2C_UnsupportedValue;
	}

	return 0;


}
int lxScriboSocketInit(char *server)
{
	char *hostname, *portnr;
	struct sockaddr_in sin={0};
	struct hostent *host;
	int port;
	int init_done = 0;

	/* Register the write/read messages to write over udp */
	/*replace udp message by tcp lxScribo_udp_message, lxScribo_udp_message_read*/
	tfa_ext_register(NULL, lxScribo_tcp_message, lxScribo_tcp_message_read, NULL);

	if ( server==0 ) {
		fprintf (stderr, "%s:called for recovery, exiting for now...", __FUNCTION__);
		lxScriboSocketExit(1);
	}

	/* lxScribo register can be called multiple times */
	init_done = (activeSocket != INVALID_SOCKET);
	if (init_done) {
		fprintf(stderr, "%s: closing already open socket\n", __FUNCTION__);
		shutdown(activeSocket, SHUT_RDWR);
		close(activeSocket);
		usleep(10000);
	}

	portnr = strrchr(server , ':');
	if ( portnr == NULL )
	{
		fprintf (stderr, "%s: %s is not a valid servername, use host:port\n",__FUNCTION__, server);
		return -1;
	}
	hostname=server;
	*portnr++ ='\0'; //terminate
	port=atoi(portnr);

	host = gethostbyname(hostname);
	if ( !host ) {
		fprintf(stderr, "Error: wrong hostname: %s\n", hostname);
		exit(1);
	}

	if(port==0) // illegal input
		return -1;

	activeSocket = socket(AF_INET, SOCK_STREAM, 0);  /* init socket descriptor */

	if(activeSocket < 0)
		return -1;
	
	/*** PLACE DATA IN sockaddr_in struct ***/
	memcpy(&sin.sin_addr.s_addr, host->h_addr, host->h_length);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);

	/*** CONNECT SOCKET TO THE SERVICE DESCRIBED BY sockaddr_in struct ***/
	if (connect(activeSocket, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		fprintf(stderr,"error connecting to %s:%d\n", hostname , port);
		return -1;
	}

	if (init_done == 0) {
		atexit(lxScriboAtexit);
		(void) signal(SIGINT, lxScriboCtlc);
	}

	return activeSocket;
}

/*
 * the sockets are created first and then waits until a connection is done
 * the active socket is returned
 */
int lxScriboListenSocketInit(char *socketnr)
{
	int port;
	int  rc;
	char hostname[50];
	char clientIP [INET6_ADDRSTRLEN];

	port = atoi(socketnr);
	if(port==0) // illegal input
		return -1;

	rc = gethostname(hostname,sizeof(hostname));

	if(rc == -1){
		printf("Error gethostname\n");
		return -1;
	}

	struct sockaddr_in serverAdd;
	struct sockaddr_in clientAdd;
	socklen_t clientAddLen;

	atexit(lxScriboAtexit);
	(void) signal(SIGINT, lxScriboCtlc);

	printf("Listening to %s:%d\n", hostname, port);

	memset(&serverAdd, 0, sizeof(serverAdd));
	serverAdd.sin_family = AF_INET;
	serverAdd.sin_port = htons(port);

	//Bind to any local server address using htonl (host to network long):
	serverAdd.sin_addr.s_addr = htonl(INADDR_ANY);
	//Or specify address using inet_pton:
	//inet_pton(AF_INET, "127.0.0.1", &serverAdd.sin_addr.s_addr);

	listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if(listenSocket == -1){
		printf("Error creating socket\n");
		return -1;
	}

	if(bind(listenSocket, (struct sockaddr*) &serverAdd, sizeof(serverAdd)) == -1){
		printf("Bind error\n");
		return -1;
	}

	if(listen(listenSocket, 5) == -1){
		printf("Listen Error\n");
		return -1;
	}

	clientAddLen = sizeof(clientAdd);
	activeSocket = accept(listenSocket, (struct sockaddr*) &clientAdd, &clientAddLen);

	inet_ntop(AF_INET, &clientAdd.sin_addr.s_addr, clientIP, sizeof(clientAdd));
	printf("Received connection from %s\n", clientIP);

	close(listenSocket);

	return (activeSocket);

}
static int lxScribo_execute_message(int command_length, void *command_buffer,
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
			result =lxScribo_tcp_message_read( 0, read_length, read_buffer);
	} else if ( (command_length==0) & (read_length!=0) ) {
		/* Read result */
		result = lxScribo_tcp_message_read( 0, read_length, read_buffer);
	}

	/* return value is returned length, dsp result status or <0 if error */
	return result;
}

const struct nxp_i2c_device lxScriboSocket_device = {
		lxScriboSocketInit,
		lxScriboWriteRead,
		lxScriboVersion,
		lxScriboSetPin,
		lxScriboGetPin,
		lxScriboClose,
		NULL,
		lxScriboSendStartPlaybackCmd,
		lxScriboSendStopPlaybackCmd,
		NXP_I2C_Msg,
		lxScribo_execute_message
};

