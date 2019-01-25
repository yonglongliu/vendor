/*
 * udp_sockets.c
 *
 *  Created on: Apr 20, 2015
 *      Author: wim
 */

#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#ifdef LXSCRIBO
#include "lxScribo.h"
#include "NXP_I2C.h"
#endif

#include "udp_sockets.h"
#include "hal_utils.h"
#include <errno.h>

#define BUFSIZE 2048

#define MAX_SOCKETS 2
#define FD_OFFSET 1


int udp_init_socket(char *dev);
int udp_read_data(int fd, unsigned char *wbuffer, int wsize);
int udp_write_data(int fd, unsigned char *wbuffer,int wsize);
int udp_close_socket(int fd);

const struct server_protocol udp_server = {
	udp_init_socket,
	udp_read_data,
	udp_write_data,
	udp_close_socket,
};
int udp_init_socket(char *dev){
	int socket_id=-1;
	int port = atoi(&dev[1]);
	socket_id = udp_socket_init(NULL, port);
	return socket_id;
}
int udp_read_data(int fd, unsigned char *wbuffer, int wsize){
	int length=0;
	length = udp_read(fd, (char*)wbuffer, wsize, 0); //timeout=0 blocks
	return length;
}
int udp_write_data(int fd, unsigned char *wbuffer,int wsize){
	int rc;
	rc = udp_write( fd, (char *)wbuffer, wsize);
	return rc;
}
int udp_close_socket(int fd){
	int result=0;
	result = udp_close(fd);
	if (result<0)
		printf("Error in closing socket\n");
	return result;
}

struct udp_socket {
	int in_use;
	int fd;
	struct sockaddr_in remaddr;	/* remote address */
};

static struct udp_socket udp_sockets[MAX_SOCKETS] = {};

static int NXP_UDP_verbose;

#define VERBOSE if (NXP_UDP_verbose)


static int get_free_socket(void)
{
	int i, ret = -1;

	for(i=0; (i<MAX_SOCKETS) && (ret == -1); i++) {
		if (udp_sockets[i].in_use == 0)
			ret = i;
	}

	return ret;
}


/*
    Get ip from domain name
 */

static int hostname_to_ip(char * hostname , char* ip)
{
    struct hostent *he;
    struct in_addr **addr_list;
    int i;

    if ( (he = gethostbyname( hostname ) ) == NULL)
    {
        // get the host info
        herror("gethostbyname");
        return 1;
    }

    addr_list = (struct in_addr **) he->h_addr_list;

    for(i = 0; addr_list[i] != NULL; i++)
    {
        //Return the first one;
        strcpy(ip , inet_ntoa(*addr_list[i]) );
        return 0;
    }

    return 1;
}

void NXP_UDP_Trace(int on)
{
	NXP_UDP_verbose = (on & 1);	   // bit 0 is trace
}

int udp_socket_init(char *server, int port)
{
	struct sockaddr_in myaddr;	/* our address */
	struct hostent *host;
	char ip[100];
	int sock;

	sock = get_free_socket();
	if (sock == -1) {
		fprintf(stderr, "no more sockets available\n");
		return -1;
	}

	if (port==0) // illegal input
		return -1;

	/* create an UDP socket */

	if ((udp_sockets[sock].fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("cannot create socket\n");
		return -1;
	}

	/* bind the socket to any valid IP address and a specific port */

	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (server==0)  /* server only */
		myaddr.sin_port = htons(port);
	else /* client only */
		myaddr.sin_port = htons(0);

	if (bind(udp_sockets[sock].fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
		perror("bind failed");
		close(udp_sockets[sock].fd);
		return -1;
	}

	if (server!=NULL) { /* client only */
		host = gethostbyname(server);
		if ( !host ) {
			fprintf(stderr, "Error: wrong hostname: %s\n", server);
			close(udp_sockets[sock].fd);
			return -1;
		}

		/* now define remaddr, the address to whom we want to send messages */
		/* For convenience, the host address is expressed as a numeric IP address */
		/* that we will convert to a binary format via inet_aton */
		memset((char *) &udp_sockets[sock].remaddr, 0, sizeof(udp_sockets[sock].remaddr));
		memcpy(&udp_sockets[sock].remaddr.sin_addr.s_addr, host->h_addr, host->h_length);
		udp_sockets[sock].remaddr.sin_family = AF_INET;
		udp_sockets[sock].remaddr.sin_port = htons(port);

		/* get ip dot name */
		if (hostname_to_ip(server , ip)) {
			close(udp_sockets[sock].fd);
			return -1;
		}
		/* convert char dot ip to binary form */
		if (inet_aton(ip, &udp_sockets[sock].remaddr.sin_addr)==0) {
			fprintf(stderr, "inet_aton() failed\n");
			close(udp_sockets[sock].fd);
			return -1;
		}
	}

	udp_sockets[sock].in_use = 1;
	return sock + FD_OFFSET;
}

int udp_close(int fd)
{
	int sock = fd - FD_OFFSET;
	udp_sockets[sock].in_use = 0;
	return close(udp_sockets[sock].fd);
}

int udp_read(int fd, char *inbuf, int len, int waitsec) 
{
	int sock = fd - FD_OFFSET;
	socklen_t addrlen = sizeof(udp_sockets[sock].remaddr); /* length of addresses */
	int recvlen; /* # bytes received */
	struct timeval tv;
	tv.tv_sec = waitsec;
	tv.tv_usec = 0;

	if (waitsec) {
		if (setsockopt(udp_sockets[sock].fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
			perror("Error");
		}
	}

	recvlen = recvfrom(udp_sockets[sock].fd, inbuf, len, 0, (struct sockaddr *) &udp_sockets[sock].remaddr, &addrlen);
	int rcvError = errno;
	if (recvlen < 0) {
		if(rcvError != EAGAIN || waitsec == 0) {
			perror("rcv error");
			fprintf(stderr,"sock %d, fd %d, len %d err %d\n",sock, udp_sockets[sock].fd, len, errno); fflush(stderr);
		}
	}

	if (waitsec) {
		// Reset to blocking again.
		tv.tv_sec  = 0;
		tv.tv_usec = 0;
		if (setsockopt(udp_sockets[sock].fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
			perror("Error");
		}
	}

	VERBOSE hexdump("UDP R", (const unsigned char *)inbuf, recvlen);

	return recvlen;
}

int udp_write(int fd, char *outbuf, int len)
{
	int sock = fd - FD_OFFSET;
	VERBOSE hexdump("UDP w", (const unsigned char *)outbuf, len);
	return sendto(udp_sockets[sock].fd, outbuf, len, 0, (struct sockaddr *)&udp_sockets[sock].remaddr, sizeof(udp_sockets[sock].remaddr));
}

int udp_is_readable(int fd,int * error,int timeOut) // milliseconds
{
  int sock = fd - FD_OFFSET;
  int sd = udp_sockets[sock].fd;

  fd_set socketReadSet;
  FD_ZERO(&socketReadSet);
  FD_SET(sd,&socketReadSet);
  struct timeval tv;
  if (timeOut) {
    tv.tv_sec  = timeOut / 1000;
    tv.tv_usec = (timeOut % 1000) * 1000;
  } else {
    tv.tv_sec  = 0;
    tv.tv_usec = 0;
  } // if
  if (select(sd+1,&socketReadSet,0,0,&tv) == -1) {
    *error = 1;
    return 0;
  } // if
  *error = 0;
  return FD_ISSET(sd,&socketReadSet) != 0;
} /* udp_is_readable */

/**
 * below is the test main for client and server executables to do standalone testing
 */
#if 0 // avoid multiple definition of 'main'
//#ifndef LXSCRIBO
#ifdef SERVER
int main() {
	int fd = udp_socket_init(NULL, 5555);
	int recvlen;
	unsigned char buf[BUFSIZE];

	for(;;) {
		printf("waiting on port %d\n", 5555);
		recvlen = udp_read(fd, buf, sizeof(buf), 0);
		if (recvlen > 0) {
			buf[recvlen] = 0;
			printf("received message: \"%s\" (%d bytes)\n", buf, recvlen);
		}
		else
			printf("uh oh - something went wrong!\n");

		strcat(buf,"sending response\n");
		udp_write(fd, buf, strlen(buf));
	}

	return 0;
}
#else
int main(int argc, char *argv[]) {
	int fd = udp_socket_init("127.0.0.1", 5555);
	unsigned char buf[BUFSIZE];
	int recvlen;

	if(argc>1) {
		udp_write(fd, argv[1], strlen(argv[1]));
	} else {
		char *str = "test string\n";
		char *str2 = "nog een\n";
		if (udp_write(fd, str, strlen(str)) < 0)
			perror("write");
//		if (udp_write(fd, str2, strlen(str2)) < 0)
//			perror("write");
		recvlen = udp_read(fd, buf, sizeof(buf), 1);

	}
	if (recvlen) {
		buf[recvlen]='\0';
		printf("client: %d %s\n",recvlen,buf);
	}

	return 0;
}
#endif
#endif //LXSCRIBO
