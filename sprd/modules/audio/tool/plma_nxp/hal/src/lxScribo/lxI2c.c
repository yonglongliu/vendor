/*
 * lxI2c.c
 *
 *  Created on: Apr 21, 2012
 *      Author: wim
 */
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>	
#include <fcntl.h>

#include "hal_utils.h"
#include "dbgprint.h"
#include "lxScribo.h"
#include "NXP_I2C.h" /* for the error codes */

#define I2C_FORCE //peng debug
/* workaround for systems which do not have the header file linux/i2c-dev.h properly installed */
#ifndef I2C_SLAVE
#define I2C_SLAVE	0x0703
#define I2C_SLAVE_FORCE	0x0706
#endif

#ifdef I2C_FORCE
#define I2C_SLAVE_ACCESS I2C_SLAVE_FORCE
#else
#define I2C_SLAVE_ACCESS I2C_SLAVE
#endif


int i2c_trace=0;
static   char filename[FILENAME_MAX];

static void  lxI2cSlave(int fd, int slave)
{
    // open the slave
    int res = ioctl(fd, I2C_SLAVE_ACCESS, slave);
    if ( res < 0 ) {
        /* TODO ERROR HANDLING; you can check errno to see what went wrong */
        ERRORMSG("Can't open i2c slave:0x%02x\n", slave);
        _exit(res);
    }

    if (i2c_trace) PRINT("I2C slave=0x%02x\n", slave);

}

/* SC42158
 *  Remove slave checking and use the address from the transaction.
 */
static int lxI2cWriteRead(int fd, int NrOfWriteBytes, const uint8_t * WriteData,
		int NrOfReadBytes, uint8_t * ReadData, unsigned int *pError) {
	int ln = -1;

	lxI2cSlave(fd, WriteData[0]>>1 );

	if (NrOfWriteBytes & i2c_trace) {
		hexdump("W", WriteData, NrOfWriteBytes);
	}

	if (NrOfWriteBytes > 2)
		ln = write(fd, &WriteData[1],  NrOfWriteBytes - 1);

	if (NrOfReadBytes) { // bigger
		//if ( (ReadData[0]>1) != (WriteData[0]>1) ) // if block read is different
		//		write(fd, &ReadData[0],  1);
		ln = write(fd, &WriteData[1],1); //write sub address
    if ( ln < 0 ) {
      *pError = NXP_I2C_NoAck; /* treat all errors as nack */
    } else {
      ln = read(fd,  &ReadData[1], NrOfReadBytes-1);
    }
  }

	if (NrOfReadBytes & i2c_trace) {
		hexdump("R", ReadData, NrOfReadBytes);
	}

	if ( ln < 0 ) {
		*pError = NXP_I2C_NoAck; /* treat all errors as nack */
		perror("i2c slave error");
	} else {
		*pError = NXP_I2C_Ok;
	}

	return ln+1;
}


/*
 * SC42158
 *  Slave is set in the transaction call.
 */
static int lxI2cInit(char *devname)
{
    static int fd=-1;

    if ( devname )
    	strncpy(filename, devname, FILENAME_MAX-1);

//    if ( lxScribo_verbose ) TODO
//    	printf("Opening serial Scribo connection on %d\n", filename);

    if (fd != -1 )
    	if (close(fd) )
    		_exit(1);

	fd = open(filename, O_RDWR | O_NONBLOCK | O_EXCL, 0);
	if (fd < 0) {
		ERRORMSG("Can't open i2c bus:%s\n", filename);
		_exit(1);
	}

	return fd;
}

/*
 * Close an opened device
 * Return success (0) or fail (-1)
 */
static int lxI2cClose(int fd)
{
	int ret;

	ret = close(fd);

	return ret;
}

static int lxI2cVersion(char *buffer, int fd)
{
    (void)fd; /* avoid warning */
    (void)buffer; /* avoid warning */
    PRINT("Not implemented! \n");
	return 0;
}

static int lxI2cGetPin(int fd, int pin)
{
    (void)fd; /* avoid warning */
    (void)pin; /* avoid warning */
    PRINT("Not implemented! \n");
	return 0;
}

static int lxI2cSetPin(int fd, int pin, int value)
{
    (void)fd; /* avoid warning */
    (void)pin; /* avoid warning */
    (void)value; /* avoid warning */
    PRINT("Not implemented! \n");
	return 0;
}

const struct nxp_i2c_device lxI2c_device = {
	lxI2cInit,
	lxI2cWriteRead,
	lxI2cVersion,
	lxI2cSetPin,
	lxI2cGetPin,
	lxI2cClose,
	NULL,
	NULL,
	NULL,
	NXP_I2C_Msg, /* can do rpc msg */
	NULL
};
