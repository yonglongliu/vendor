#include <stdio.h>
#include <sys/select.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>

#include "lxScribo.h"
#include "NXP_I2C.h"

#ifndef B460800
#define B460800 460800
#endif
//#define BAUD B500000 // 16M xtal
#define BAUD B460800 // 14.7M xtal	//TODO autodetect baud

extern int lxScribo_verbose;
static   char filename[FILENAME_MAX];

/*
 * init or recover the connection
 *   if dev==NULL recover else just open
 */
int lxScriboSerialInit(char *dev)
{
    struct termios tio;
    static int tty_fd=-1;

    if ( dev )
    	strncpy(filename, dev, FILENAME_MAX-1);

    if ( lxScribo_verbose )
    	printf("Opening serial Scribo connection on %s\n", filename);

    if (tty_fd != -1 )
    	if (close(tty_fd) )
    		_exit(1);
    
    memset(&tio,0,sizeof(tio));
    tio.c_iflag=0;
    tio.c_oflag=0;
    tio.c_cflag=CS8|CREAD|CLOCAL|CSTOPB;           // 8n2, see termios.h for more information
    tio.c_lflag=0;
    tio.c_cc[VMIN]=1;
    tio.c_cc[VTIME]=0;

    tty_fd=open(filename, O_RDWR);
    cfsetospeed(&tio,BAUD);         // 115200 baud
    cfsetispeed(&tio,BAUD);         // 115200 baud

    tcsetattr(tty_fd,TCSANOW,&tio);

    return tty_fd;
}

const struct nxp_i2c_device lxScriboSerial_device = {
	lxScriboSerialInit,
	lxScriboWriteRead,
	lxScriboVersion,
	lxScriboSetPin,
	lxScriboGetPin,
	lxScriboClose,
	NULL,
	NULL,
	NULL,
	NXP_I2C_Msg, /* can do rpc msg */
	NULL
};
