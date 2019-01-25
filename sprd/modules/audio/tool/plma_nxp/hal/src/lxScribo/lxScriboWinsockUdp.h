#ifndef LXSCRIBOWINSOCKUDP_H_
#define LXSCRIBOWINSOCKUDP_H_

/*
 * Winsock variant of udp_read() 
 */
int winsock_udp_read(int fd, char *inbuf, int len, int waitsec);

/*
 * Winsock variant of udp_write() 
 */
int winsock_udp_write(int fd, char *outbuf, int len);

#endif /* LXSCRIBOWINSOCKUDP_H_ */
