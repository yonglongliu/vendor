#ifndef UDPSOCKETS_H_
#define UDPSOCKETS_H_

/*
 * Unix variant of udp_socket_init() 
 */
int udp_socket_init(char *server, int port);

/*
 * Unix variant of udp_read() 
 */
int udp_read(int fd, char *inbuf, int len, int waitsec);

/*
 * Unix variant of udp_write() 
 */
int udp_write(int fd, char *outbuf, int len);
int udp_close(int fd);
/*
 * test if data available
 */
int udp_is_readable(int sd,int * error,int timeOut); // milliseconds

#endif /* UDPSOCKETS_H_ */
