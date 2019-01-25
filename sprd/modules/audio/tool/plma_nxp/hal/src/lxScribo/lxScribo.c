//
// lxScribo main entry
//
// 	 This is the initiator that sets up the connection to either a serial/RS232 device or a socket.
//
// 	 input args:
// 	 	 none: 		connect to the default device, /dev/Scribo
// 	 	 string: 	assume this is a special device name
// 	 	 number:	use this to connect to a socket with this number
//
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#if !(defined(WIN32) || defined(_X64))
#include <unistd.h>
#endif
#include <stdint.h>
#include "dbgprint.h"
#include "NXP_I2C.h"
#include "lxScribo.h"
#include "tfaOsal.h"
#include <assert.h>
#include "cmd.h"

int lxScribo_verbose = 0;
#define VERBOSE if (lxScribo_verbose)
static char dev[FILENAME_MAX]; /* device name */
static int lxScriboFd=-1;

/* device usage reference counter */ 
static int refcnt = 0; 

/**
 * Parse the argument string and try to load the shared library specified.
 * Return a pointer to the registry structure inside this library.
 *
 * The basename is terminated by the first ','.
 * argument format (cmodel): <basename>,[revid0|_][revid1|_][revid2|_][revid3|_][,firmware lib]
 * argument format (generic): <basename>,[subargs string]
 *
 * @param argument string of the --device/-d option
 * @return pointer to the registry structure
 *
 */
struct nxp_i2c_device * lxScriboLibLoad(char *libarg) {
	struct nxp_i2c_device *plug=NULL;
	plug = (struct nxp_i2c_device *)tfaosal_plugload(libarg);
	return plug;
}

/**
 * register the low level I2C HAL interface
 *  @param target device name; if NULL the default will be registered
 *  @return file descriptor of target device if successful
 */
int lxScriboRegister(char *target) 
{
	struct nxp_i2c_device *plug; //registry structure

	if ( lxScribo_verbose ) {
			PRINT("%s:target device=%s\n", __FUNCTION__, target);
	}

	plug = lxScriboLibLoad(target);
	if (plug ) {
		lxScriboFd = NXP_I2C_Interface(target, plug);
		goto lxScriboRegister_return; // common return
	}

	if (target ){
		strncpy(dev, target, FILENAME_MAX-1);
	} else {
		strncpy(dev, TFA_I2CDEVICE, FILENAME_MAX-1);
	}


	/* tell the HAL which interface functions : */

	/////////////// dummy //////////////////////////////
	if ( strncmp (dev, "dummy",  5 ) == 0 ) {// if dummy act dummy
		lxScriboFd = NXP_I2C_Interface(dev, &lxDummy_device);
	}  
#ifdef HAL_WIN_SIDE_CHANNEL
	/////////////// hid //////////////////////////////
	else if ( strncmp( dev , "wsc", 3 ) == 0)	{ // windows side channel
		lxScriboFd = NXP_I2C_Interface(dev, &lxWinSideChannel_device);
	}
#endif
#ifdef	HAL_HID
	/////////////// hid //////////////////////////////
	else if ( strncmp( dev , "hid", 3 ) == 0)	{ // hid
		lxScriboFd = NXP_I2C_Interface(dev, &lxHid_device);
	}
#endif
#ifdef HAL_SYSFS
	/////////////// sysfs //////////////////////////////
	else if ( strncmp( dev , "sysfs", 5 ) == 0)	{
		lxScriboFd = NXP_I2C_Interface(dev, &lxSysfs_device);
	}
#endif
#ifdef HAL_MIXER
	/////////////// alsa mixer //////////////////////////////
	else if ( strncmp( dev , "mix", 3 ) == 0)	{
		lxScriboFd = NXP_I2C_Interface(dev, &lxAlsaMixerCtl_device);
	}
#endif
#ifdef HAL_TIBERIUS
	/////////////// Tiberius (72+DSP) //////////////////////////////
	else if ( strncmp( dev , "tiberius", 4 ) == 0)	{
		lxScriboFd = NXP_I2C_Interface(dev, &lxTiberius_device);
	}
#endif
#if !( defined(WIN32) || defined(_X64) ) // posix/linux
	/////////////// network //////////////////////////////
	else if ( strchr( dev , ':' ) != 0)	{ // if : in name > it's a socket
		lxScriboFd = NXP_I2C_Interface(dev, &lxScriboSocket_device); //TCP
	}
	else if ( strchr( dev , '@' ) != 0)	{ // if @ in name > it's a UDP socket
		lxScriboFd = NXP_I2C_Interface(dev, &lxScriboSocket_udp_device);
	}
	/////////////// i2c //////////////////////////////
	else if ( strncmp (dev, "/dev/i2c",  8 ) == 0 ) { // if /dev/i2c... direct i2c device
		lxScriboFd = NXP_I2C_Interface(dev, &lxI2c_device);
		VERBOSE PRINT("%s: i2c\n", __FUNCTION__);
	}
	/////////////// serial/USB //////////////////////////////
	else if ( strncmp (dev, "/dev/tty",  8 ) == 0 ) { // if /dev/ it must be a serial device
		lxScriboFd = NXP_I2C_Interface(dev, &lxScriboSerial_device);
	}

#else /////////////// Scribo server //////////////////////////////
	else if ( strncmp (dev, "scribo",  5 ) == 0 ) // Scribo server dll interface
		lxScriboFd = NXP_I2C_Interface(dev, &lxWindows_device);
	/////////////// Windows network //////////////////////////////
	else if ( strchr( dev , ':' ) != 0)	// if : in name > it's a socket
		lxScriboFd = NXP_I2C_Interface(dev, &lxScriboWinsock_device);
	else if ( strchr( dev , '@' ) != 0)	// if @ in name > it's a socket
		lxScriboFd = NXP_I2C_Interface(dev, &lxScriboWinsock_udp_device);
#endif
	else {
		ERRORMSG("%s: devicename %s is not a valid target\n", __FUNCTION__, dev); // anything else is a file
		lxScriboFd = -1;
	}

lxScriboRegister_return:

	if (lxScriboFd != -1) {
		/* add one device user*/
		refcnt++;
	}

	return lxScriboFd;
}/**
 * register the server of communication(either tcp or udp)
 *  @param target device name; if NULL the default will be registered
 *  @return file descriptor of target device if successful
 */
/* fill the interface and init */
const struct server_protocol *server_dev;
int server_target_fd=-1;
int server_interface(char *target, const struct server_protocol *interface)
{
	//server_target_fd
	server_dev = interface;
	server_target_fd = (*server_dev->init)(target);
	return server_target_fd;
}

int server_register(char *target)
{
	int id=-1;/*Interface descriptor*/
#if defined(WIN32) || defined(_X64)
	// server not implemented for windows
	target = target;
#else
	if ( strchr( target , '@' ) != 0) {
		id = server_interface(target, &udp_server);
	} else{
		id = server_interface(target, &tcp_server); // note socket is ascii string
	}
#endif
	return id;
}
/**
 *  unregister the low level I2C HAL interface
 *  @return success (0) or fail (-1)
 */
int lxScriboUnRegister(void) 
{
	// Prevent refcount to become negative.
	if(refcnt > 0) {
		/* one device user less */
		refcnt--;
	} else {
		/* refcnt is already 0, nothing to unregister */
		return -1;
	}
	return refcnt;
}

int lxScriboGetFd(void)
{
	if ( lxScriboFd < 0 )
		ERRORMSG("Warning: the target is not open\n");

	return lxScriboFd;
}

void lxScriboGetName(char *buf, int count)
{
	int i;

	for(i=0; i<count; i++)
		buf[i] = dev[i];
}

void  lxScriboVerbose(int level)
{
	lxScribo_verbose = level;
}

int lxScriboNrHidDevices(void)
{
#ifdef HAL_HID
	return lxHidNrDevices();
#else
	ERRORMSG("Warning: HID support is missing\n");
	return 0;
#endif
}

int lxScriboWaitOnPinEvent(int pin, int value, unsigned int timeout) //TODO pinwait add to HAL
{
#ifdef HAL_HID
	if ( NXP_I2C_Interface_Type() == NXP_I2C_HID)
		return lxHidWaitOnPinEvent(pin, value, timeout);
#endif

	ERRORMSG("Warning: lxScriboWaitOnPinEvent support is missing\n");
        (void)pin;
        (void)value;
        (void)timeout;

	return 0;
}

