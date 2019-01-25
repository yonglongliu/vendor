/*
 *Copyright 2015 NXP Semiconductors
 *
 *Licensed under the Apache License, Version 2.0 (the "License");
 *you may not use this file except in compliance with the License.
 *You may obtain a copy of the License at
 *
 *http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing, software
 *distributed under the License is distributed on an "AS IS" BASIS,
 *WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *See the License for the specific language governing permissions and
 *limitations under the License.
 */

#if !(defined(WIN32) || defined(_X64))
#include <unistd.h>
#else
#undef __cplusplus 

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>


//Link  with Scribo.lib. Make sure the Scribo.lib can be found by the linker
#ifdef _X64
#pragma comment (lib, "Scribo64.lib")	
#else
#pragma comment (lib, "Scribo.lib")	
#endif

#endif // WINDOWS
/* implementation of the NXP_I2C API on Windows */
#include "NXP_I2C.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "hal_utils.h"
#include "dbgprint.h"
#include "NXP_I2C.h"
#include "lxScribo.h"
#include "Scribo.h"

/* the interface */
static const struct nxp_i2c_device *dev;

static FILE *traceoutput = NULL;

static int gI2cBufSz=NXP_I2C_MAX_SIZE;
/*
 * accounting globals
 */
int gNXP_i2c_writes=0, gNXP_i2c_reads=0;


/* align with enum NXP_I2C_Error from NXP_I2C.h */
static const char *errorStr[NXP_I2C_ErrorMaxValue] =
{
        "UnassignedErrorCode",
        "Ok",
        "NoAck",
        "TimeOut",
        "ArbLost",
        "NoInit", 
        "UnsupportedValue",
        "UnsupportedType",
        "NoInterfaceFound",
        "NoPortnumber",
        "BufferOverRun"
};
const char *NXP_I2C_Error_string(NXP_I2C_Error_t error) {
	if ( error >= NXP_I2C_ErrorMaxValue)
		return errorStr[NXP_I2C_UnassignedErrorCode];
	return  errorStr[error];
}
static unsigned char bInit = 0;
static unsigned char noSize = 0;
static int i2cTargetFd=-1;						 /* file descriptor for target device */

static int NXP_I2C_verbose;
/* for monitor by dummy calls */
//int lxDummyInit(char *file);
//int lxDummyWriteRead(int fd, int NrOfWriteBytes, const uint8_t *WriteData,
//		     int NrOfReadBytes, uint8_t *ReadData, uint32_t *pError);
static int NXP_I2C_monitor=0;
extern int lxScriboGetFd(void);
//#define VERBOSE(format, args...) if (NXP_I2C_verbose) PRINT(format, ##args)
#define VERBOSE if (NXP_I2C_verbose)

/*
 * use tracefile fo output
 *  args:
 *  	0      = stdout
 *  	"name" = filename
 *  	NULL   = close file
 */
void NXP_I2C_Trace_file(char *filename) {
	if ((traceoutput != NULL) && (traceoutput != stdout)) {
		/* close if open ,except for stdout */
		fclose(traceoutput);
	}
	if (filename==0) {
		traceoutput = stdout;
	} else if ( *filename > 0) {
		/* append to file */
		traceoutput = fopen(filename, "a");
		if ( traceoutput == NULL) {
			PRINT_ERROR("Can't open %s\n", filename);
	        }
        }
}

/* enable/disable trace */
void NXP_I2C_Trace(int on) {
	if ( traceoutput == NULL )
		traceoutput = stdout;
	NXP_I2C_verbose = (on & 1);	   // bit 0 is trace
	if((on & 2)!=0) { // bit 1 is monitor
		PRINT("monitor type=TFA9888");
		NXP_I2C_monitor = 1;
	}
}

static void msg24dump(int num_write_bytes, const unsigned char * data)
{
	int i;

	for(i=0;i<num_write_bytes;i+=3)
	{
		PRINT_FILE(traceoutput, "0x%02x%02x%02x ", data[i], data[i+1], data[i+2]);
	}
}

static void msg_print_trace(char *format, int length, unsigned char *data) {
	if ( length == 0)
		return;

	if (length % 3 != 0)
	{
		PRINT_FILE(traceoutput, "!!! Warning msg data is not 24bits aligned!!, size:%d\n", length);
	}
	length -= length % 3;

	 PRINT_FILE(traceoutput, format, length);
	 msg24dump(length, data);
	 PRINT_FILE(traceoutput, "\n");
	 fflush(traceoutput);
}

static NXP_I2C_Error_t init_if_firsttime(void)
{
	NXP_I2C_Error_t retval = NXP_I2C_Ok;

	if (!bInit)
	{
		int fd = lxScriboRegister(NULL); /* this will register the default target */
		if (fd < 0) { /* invalid file descriptor */
			retval = eI2C_UNSPECIFIED;
		}

		NXP_I2C_Trace_file(0); /* default to stdout */
		bInit = 1;
	}

	return retval;
}
static int  recover(void) {
	i2cTargetFd = (*dev->init)(NULL); /* this should reset the connection */
	return i2cTargetFd<0; /* failed if -1 */
}
/* fill the interface and init */
int NXP_I2C_Interface(char *target, const struct nxp_i2c_device *interface)
{
	/* if the target is udp we can skip the size check */
	if(strchr(target, '@') != 0)
		noSize=1;
	else
		noSize=0;

	dev = interface;
	i2cTargetFd = (*dev->init)(target);
	if((*dev->buffersize) != NULL) 
		gI2cBufSz = (*dev->buffersize)();
	if (!bInit) { /* lxScriboRegister() can be called before */
		bInit = 1;
	}

	return i2cTargetFd;
}
NXP_I2C_Error_t NXP_I2C_WriteRead(  unsigned char slave_addr,
                                    int num_write_bytes,
                                    const unsigned char write_data[],
                                    int num_read_bytes,
                                    unsigned char read_data[] )
{
	NXP_I2C_Error_t retval;
	uint32_t error;
	int write_only = (num_read_bytes == 0) || (read_data == NULL);

	/* Skip size check when the target is udp */
	if(!noSize) {
		if(num_write_bytes > gI2cBufSz) {
			PRINT_ERROR("%s: too many bytes to write: %d\n", __FUNCTION__, num_write_bytes);
			return NXP_I2C_UnsupportedValue;
		}
		if(num_read_bytes > gI2cBufSz) {
			PRINT_ERROR("%s: too many bytes to read: %d\n", __FUNCTION__, num_read_bytes);
			return NXP_I2C_UnsupportedValue;
		}
	}

	retval = init_if_firsttime();
	if(NXP_I2C_Ok==retval)
	{
		unsigned char *wbuffer = malloc(num_write_bytes+1); 
		unsigned char *rbuffer = malloc(num_read_bytes+1);
		int ret_read_bytes = 0;

		if(wbuffer == NULL || rbuffer == NULL) {
			free(wbuffer);
			free(rbuffer);
			return NXP_I2C_BufferOverRun;
		}

		wbuffer[0] = slave_addr;
		memcpy((void*)&wbuffer[1], (void*)write_data, num_write_bytes); // prepend slave address

		rbuffer[0] = slave_addr|1; //read slave

		if(write_only) {
			/* Write only I2C transaction */
			/* num_read_bytes will include the slave byte, so it's incremented by 1 if ok */
			ret_read_bytes = (*dev->write_read)(i2cTargetFd,  num_write_bytes+1, wbuffer, 0, NULL, &error);
			VERBOSE hexdump_to_file(traceoutput, "I2C w", wbuffer, num_write_bytes+1);
		} else {
			/* WriteRead I2C transaction */
			/* num_read_bytes will include the slave byte, so it's incremented by 1 if ok */
			ret_read_bytes = (*dev->write_read)(i2cTargetFd,  num_write_bytes+1, wbuffer, num_read_bytes+1, rbuffer, &error);
			VERBOSE hexdump_to_file(traceoutput, "I2C W", wbuffer, num_write_bytes+1);


		}

		retval = error;

		/*
		if(NXP_I2C_monitor)
			lxDummyWriteRead(i2cTargetFd,  num_write_bytes+1, wbuffer, 0, NULL, &error);
		 */
		if(!write_only) {
			if(ret_read_bytes) {
				VERBOSE hexdump_to_file(traceoutput, "I2C R", rbuffer, ret_read_bytes); // also show slave
				memcpy((void*)read_data, (void*)&rbuffer[1], ret_read_bytes-1); // remove slave address
			} else {
				PRINT_ERROR("empty read in %s\n", __FUNCTION__);
				recover();
			}
		}
		free(wbuffer);
		free(rbuffer);
	}
	return retval;
}

NXP_I2C_Error_t NXP_I2C_Version(char *data)
{
        NXP_I2C_Error_t retval;
        int bufSz = 1024;
        char str[1024];
        int fd;

	retval = init_if_firsttime();
	fd = lxScriboGetFd();

	if (NXP_I2C_Ok == retval) {
		fd = (*dev->version_str)(str, fd);
                strcat((char*)data, str);
	}

        if (fd < bufSz) {
                strcat((char*)data, "\n");
        } else
                retval = NXP_I2C_UnassignedErrorCode;

	return retval;
}


NXP_I2C_Error_t NXP_I2C_StopPlayback(void)
{
	NXP_I2C_Error_t retval;
	int fd;

	retval = init_if_firsttime();
	fd = lxScriboGetFd();

	if (NXP_I2C_Ok == retval) {
		fd = (*dev->stopplayback)(fd); //TODO check errorreturn
	}

	if ( fd < 0)
		return fd*-1;

	return retval;
}

NXP_I2C_Error_t NXP_I2C_StartPlayback(char *data)
{
	NXP_I2C_Error_t retval;
	int bufSz = 1024;
	int fd;

	retval = init_if_firsttime();
	fd = lxScriboGetFd();

	if (NXP_I2C_Ok == retval) {
		fd = (*dev->startplayback)(data, fd);//TODO check errorreturn
	}

	if ( fd < 0)
		return fd*-1;

	if (fd < bufSz) {
		strcat((char*)data, "\0");
	}
	else {
		retval = NXP_I2C_UnassignedErrorCode;
	}

	return retval;
}


int NXP_I2C_GetPin(int pinNumber)
{
        int fd = lxScriboGetFd();

	return (*dev->get_pin)(fd, pinNumber);
}

NXP_I2C_Error_t NXP_I2C_SetPin(int pinNumber, int value)
{
	int fd;
    NXP_I2C_Error_t retval = init_if_firsttime();
        
	fd = lxScriboGetFd();

	if (NXP_I2C_Ok == retval) 
	{
	       retval = (*dev->set_pin)(fd, pinNumber, value);
        }

        return retval;
}

int NXP_I2C_Open(char *targetname)
{
	int status = lxScriboRegister(targetname);

	if (NXP_I2C_verbose) {
		if (status<0)
			PRINT("!Could not ");
		PRINT("register %s interface\n", targetname);
	}
	return status;
}
int NXP_I2C_Close(void)
{
    int fd = lxScriboGetFd();
    int refcnt = lxScriboUnRegister();

    /* only the last unregister will trigger a real close */
    if (refcnt==0 && dev->close!=NULL )
    	return (*dev->close)(fd);

    return refcnt;
}

int NXP_I2C_BufferSize(void)
{
	NXP_I2C_Error_t error;
	error = init_if_firsttime();
	if (error == NXP_I2C_Ok) {
		return gI2cBufSz > NXP_I2C_MAX_SIZE ? gI2cBufSz : NXP_I2C_MAX_SIZE - 1;
	}
	return NXP_I2C_MAX_SIZE - 1; //255 is minimum
}

void NXP_I2C_Scribo_Name(char *buf, int count)
{
	lxScriboGetName(buf, count);
}


NXP_I2C_IfType_t NXP_I2C_Interface_Type(void)
{
	return dev->if_type;
}

int NXP_TFADSP_Execute( int command_length, void *command_buffer,
					 int result_length, void *result_buffer)
{
	int return_status = -1;

	if (dev->tfadsp_execute != NULL ) {
		VERBOSE msg_print_trace("MSG w [%3d]: ", command_length,  command_buffer);
		return_status =  (*dev->tfadsp_execute)(command_length, command_buffer,
                result_length, result_buffer);
		if (command_length!=0 && result_length==0)  { // result is passed as status
			int trace_buf[4];
			trace_buf[3] = return_status >> 24;
			trace_buf[2] = return_status >> 16;
			trace_buf[1] = return_status >>  8;
			trace_buf[0] = return_status;
			VERBOSE msg_print_trace("MSG r [%3d]: ", 3,  (unsigned char *)trace_buf); //TODO fix 3>4 if 32 bits
		}
		else
			VERBOSE msg_print_trace("MSG r [%3d]: ", result_length,  result_buffer);
	}
    return return_status;

}