/*
 * i2cserver.c
 *
 *  Created on: Apr 21, 2012
 *      Author: wim
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lxScribo.h"
#include "NXP_I2C.h"
#include "tfa_service.h"

extern int gTargetFd;	 //TODO make this cleaner: interface filedescr
static  int i2cserver_verbose;
void i2cserver_set_verbose(int val) {
	i2cserver_verbose = val;
}
#define VERBOSE if (i2cserver_verbose)

static int i2c_Speed=0;
int i2c_GetSpeed(int bus) {
	(void)bus; /* Avoid warning */
	return i2c_Speed;
}
void i2c_SetSpeed(int bus, int bitrate) {
	(void)bus; /* Avoid warning */
	i2c_Speed=bitrate;
}

int i2c_WriteRead(int addrWr, void* dataWr, int sizeWr, void* dataRd, int sizeRd, int* nRd)
{
	NXP_I2C_Error_t err;

	err = NXP_I2C_WriteRead(addrWr<<1, sizeWr, dataWr, sizeRd, dataRd);

	if ( err == NXP_I2C_Ok) {
		*nRd=sizeRd; //actual
	} else
		*nRd=0;

	return err == NXP_I2C_Ok;
}

_Bool i2c_Write(int bus, int addrWr, void* dataWr, int sizeWr)
{
	NXP_I2C_Error_t err;
	(void)bus; /* Avoid warning */

	err = NXP_I2C_WriteRead(addrWr<<1, sizeWr, dataWr, 0, NULL);

	return err;
}

_Bool i2c_Read(int addr, void* data, int size, int* nr)
{
	return i2c_WriteRead(addr, NULL, 0, data, size, nr);
}



