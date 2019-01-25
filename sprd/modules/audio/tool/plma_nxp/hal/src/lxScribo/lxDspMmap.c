/*
 *Copyright 2014,2015 NXP Semiconductors
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <semaphore.h>
#include <limits.h>

#include "hal_utils.h"
#include "dbgprint.h"
#include "lxScribo.h"
#include "NXP_I2C.h" /* for the error codes */

extern int i2c_trace;

// taken from MaxConfig.h:
#define   I2C_CMD_STATUS        0       // Execution status of last I�C command sent by host SW
#define   I2C_CMD_ID            1       // Identifies the last I�C command sent by host SW
#define   ParametersOffset      2       // Offset inside pI2cParams at which the parameters are passed from the host to DSP or the other way around
#define   NewParamsAvailable    522   
#define   MAX_XMEM_PARAM        507     // max. no. of parameters in xmem

/* size + name of memory mapped file */
#define MMAP_LENGTH 8192
#define MMAP_FNAME ".alspro_mmap"

/* maximum size (in bytes) of a single read/write operation on the mmap file */
#define MMAP_MAX_RW_BUFSIZE 1521

#define TFA_SOFTDSP_ID 0x01
#define TFA_SEM_NAME "dsp_mmap"

/* mmap sections */
#define MMAP_VIRT_PIN_BASE_ADDR  0x1100
#define MMAP_VIRT_PIN_MEM_SIZE   0x1FF

/* default buffer to fill mmap file */
static char mm_dfl_buffer[MMAP_LENGTH] = {0};

/* pointers to mmap-ed memory region */
static void *dsp_p_mm;
static int *pI2cParams;
static unsigned short *pVirt_pin_mm;
static unsigned short *pI2cRegs;

/* semaphore for control of shared mmap access */
static sem_t *sem;

static void sem_lock(void)
{
    sem_wait(sem);
}
 
static void sem_unlock(void)
{
    sem_post(sem);
}

/* return a pointer to the mmap-ed shared buffer */
static void *alspro_mmap() 
{
    struct stat stbuf;
    int fd, mm_exists = 0;
    void *p_mm = NULL;    
    char mmap_file[FILENAME_MAX];
    
    /* use the user home folder */
    if ( strcmp("root", getenv("USER")) == 0 ) {
        sprintf(mmap_file, "/root/" MMAP_FNAME);
	} else { /* normal user */
        sprintf(mmap_file, "/home/%s/" MMAP_FNAME, getenv("USER"));
	}
    
    /* 
     * open the shared memory file for reading/writing,
     * if the file is not there, it will be created
     */
    if (stat(mmap_file, &stbuf) == 0) {
        /* the mmap file exists already */ 
        mm_exists = 1;
    }
    
    fd = open(mmap_file, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if (fd < 0) {
        PRINT_ERROR("open() error\n");
        perror("error");
        return NULL;
    }
    
    /* if the file did not exist we have to write some data to it */
    if (mm_exists != 1) {
        int bytes = write(fd, mm_dfl_buffer, MMAP_LENGTH);
        if (bytes < 0) {
            PRINT_ERROR("write() error\n");
            perror("error");
            close(fd);
            return NULL;
        }
        /* jump to start of file */
        lseek(fd, 0, SEEK_SET);

        /* jump to register 3 offset */
        lseek(fd, 3*(sizeof(short)), SEEK_CUR);

        /* write ID to file */
        unsigned short id = TFA_SOFTDSP_ID;
        bytes = write(fd, &id, sizeof(short));
        if (bytes < 0) {
            PRINT_ERROR("write() error\n");
            perror("error");
            close(fd);
            return NULL;
        }
    }
    
    /* get a pointer to the memory mapped region */
    p_mm = mmap(0, MMAP_LENGTH, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
    if (p_mm == MAP_FAILED) {
        PRINT_ERROR("mmap() error\n");
        perror("error");
        close(fd);
        return NULL;
    }
    
    sem = sem_open(TFA_SEM_NAME, O_CREAT, 0644, 1);
    if (sem == SEM_FAILED) {
        PRINT_ERROR("sem_open() error\n");
	    perror("error");
        munmap(p_mm, MMAP_LENGTH);
        close(fd);
        return NULL;
    }

    close(fd);
    
    return p_mm;
}

int lxDspMmapInit(char *devname)
{   
    /* Check if mmap not already open */
    if(dsp_p_mm != NULL) {
        return 0;
    }

    /* get a pointer to the memory mapped region */
    dsp_p_mm = alspro_mmap();
    if (dsp_p_mm == NULL) {
        /* mmap creation failed */
        return -1;
    }
    
    /* set pointer to I2CRegs and I2cParams part of mmap */
    pI2cRegs = (unsigned short *)dsp_p_mm;
    pI2cParams = (int *)(dsp_p_mm + 512);

    /* set pointer to 'virtual pin' part of mmap */
    pVirt_pin_mm = (unsigned short *)(dsp_p_mm + MMAP_VIRT_PIN_BASE_ADDR);
    
    /* set parameters for physical i2c communication
     * first close the device, then call init to prevent pending opens
     * 0 is a dummy value, it is not used internally in the close 
     */
    //lxSysfsClose(0);
    //lxSysfsInit("");
    
    return 0;
}

static int mmap_i2c_writeread(int fd, int NrOfWriteBytes, const uint8_t * WriteData,
		int NrOfReadBytes, uint8_t * ReadData, unsigned int *pError)        
{
    int ln = 0;
    unsigned short i, j;
    *pError = NXP_I2C_Ok;
    
    if (NrOfWriteBytes && i2c_trace) {
	    hexdump("MI-W", WriteData, NrOfWriteBytes);
    }
    
    /* I2C registers are of type unsigned short (16 bits) */
    
    if (NrOfWriteBytes > 2) {
        /* write data to i2c mmap */
        sem_lock();
	for (i = 2, j = WriteData[1]; i <= NrOfWriteBytes-2; i+=2, j++) {
            pI2cRegs[j] = (unsigned short)WriteData[i]<<8 | (unsigned char)WriteData[i+1]; 
	    //PRINT("pI2cRegs[%d]: 0x%08X\n", j, pI2cRegs[j]);
            ln += 2;
        }
        sem_unlock();
    }
    
    if (NrOfReadBytes) {
        /* read data from i2c mmap */
        sem_lock();
        for (i = 1, j = WriteData[1]; i < NrOfReadBytes-1; i++, j++) {
            ReadData[i] = (unsigned char)((pI2cRegs[j]&0xff00)>>8);
            ReadData[i+1] = (unsigned char)((pI2cRegs[j]&0xff));
            //PRINT("pI2cRegs[j]  : 0x%02X\n", pI2cRegs[j]);
	        //PRINT("ReadData[i]  : 0x%02X\n", ReadData[i]);
	        //PRINT("ReadData[i+1]: 0x%02X\n", ReadData[i+1]);
	        ln += 2;
        }
        sem_unlock();
    }
 
    if (NrOfReadBytes && i2c_trace) {
	    hexdump("MI-R", ReadData, NrOfReadBytes);
    }
 
    return (ln+1);
}

/* set a specific bit in a softDSP I2C register */
//static void setI2CBit(unsigned short reg, int bit) 
//{
//    unsigned short val = pI2cRegs[reg];
//    val |= 1 << bit;
//    pI2cRegs[reg] = val;
//}

static int mmap_dsp_writeread(int fd, int NrOfWriteBytes, const uint8_t * WriteData,
		int NrOfReadBytes, uint8_t * ReadData, unsigned int *pError, uint8_t offset)        
{
	int i, j, msg_len = -1;
	int actual_offset = offset;
	uint8_t msg_dest = WriteData[0]>>1;
    
    /* MMAP XMEM is of type unsigned int (32 bits) */
	if(msg_dest > 0x02)
		actual_offset = actual_offset + ((msg_dest - 0x02) * 256);
    
    if (NrOfWriteBytes) {
        msg_len = NrOfWriteBytes;
    }
    if (NrOfReadBytes) {
        msg_len = NrOfReadBytes;
    }

	if (NrOfReadBytes == 0 && NrOfWriteBytes != 0) {
		/* write data to mmap */
		sem_lock();
		for(i=2, j=(actual_offset); i < msg_len+1; i+=3, j++) {
		    pI2cParams[j] = ((int)WriteData[i]<<24 | (unsigned char)WriteData[i+1]<<16 | (unsigned char)WriteData[i+2]<<8) >> 8;
		    //PRINT("pI2cParams[%d]: 0x%08X\n", j, pI2cParams[j]);
		}
	} else if (NrOfReadBytes != 0 && NrOfWriteBytes == 0) {
        /* read data from mmap */
         sem_lock();
         for(i=1, j=(actual_offset); i < msg_len; i+=3, j++) {
            ReadData[i]   = (unsigned char)((pI2cParams[j]&0xff0000)>>16);
            ReadData[i+1] = (unsigned char)((pI2cParams[j]&0xff00)>>8);
            ReadData[i+2] = (unsigned char)((pI2cParams[j]&0xff));
            //PRINT("pI2cParams[%d]: 0x%06X\n", j, pI2cParams[j]);
	        //PRINT("ReadData[%d]  : 0x%02X\n", i, ReadData[i]);
	        //PRINT("ReadData[%d]  : 0x%02X\n", i, ReadData[i+1]);
            //PRINT("ReadData[%d]  : 0x%02X\n", i, ReadData[i+2]);
        }
    }
    else {
		PRINT_ERROR("error in mmap write/read\n");
		return 0;
	}
    
    sem_unlock();
    *pError = NXP_I2C_Ok;

	return (msg_len);
}

static int lxDspMmapWriteRead(int fd, int NrOfWriteBytes, const uint8_t * WriteData,
		int NrOfReadBytes, uint8_t * ReadData, unsigned int *pError) 
{
	int ln = -1;

    /* 
     * destination address of the message:
     *
     * WriteData[0]: address 
     * WriteData[1]: offset
     *
     * 0x0000 - 0x00ff: mmap I2C message
     * 0x0200 - 0x1000: mmap DSP message
     * 0x1100 - 0x12ff: Virtual pins
     * 0x3200 - 0x32ff: physical I2C message
     */
    uint8_t msg_dest = WriteData[0]>>1;
    uint8_t msg_off  = WriteData[1];

    //PRINT("(%s) msg_dest: 0x%02x, msg_off: 0x%02x\n", __func__, msg_dest, msg_off);
    //PRINT("(%s) NrOfWriteBytes: %d, NrOfReadBytes: %d\n", __func__, NrOfWriteBytes, NrOfReadBytes);

    if ( (NrOfWriteBytes || NrOfReadBytes) > MMAP_MAX_RW_BUFSIZE ) {
        /* the caller is trying to write/read more than the maximum buffersize */
        *pError = NXP_I2C_BufferOverRun;
        return ln;
    }
    
    if (msg_dest >= 0x32) {
        /* physical I2C message */
        ln = lxSysfsWriteRead(fd, NrOfWriteBytes, WriteData, NrOfReadBytes, ReadData, pError);
        if (ln <= 0) {
            *pError = NXP_I2C_NoAck; /* treat all errors as nack */
        } 
    }
    else if ((msg_dest >= 0x11) && (msg_dest < 0x13)) {
        *pError = NXP_I2C_NoAck; /* Address range reserved & used by getpin/setpin, not (yet?) implemented here. */
    }
    else if (msg_dest == 0x00) {
        /* mmap I2C message */
        ln = mmap_i2c_writeread(fd, NrOfWriteBytes, WriteData, NrOfReadBytes, ReadData, pError);
        if (ln <= 0) {
            *pError = NXP_I2C_NoAck; /* treat all errors as nack */
        }
    }
    else if ((msg_dest >= 0x02) && (msg_dest < 0x11)) {
        if (NrOfWriteBytes > 2 || NrOfReadBytes) {
            /* mmap write DSP message */
            ln = mmap_dsp_writeread(fd, NrOfWriteBytes-2, WriteData, NrOfReadBytes, ReadData, pError, msg_off);
            if (ln <= 0) {
                *pError = NXP_I2C_NoAck; /* treat all errors as nack */
            }
        }
        else {
            PRINT_ERROR("error in write/read to dsp mmap w:%d r:%d\n", NrOfWriteBytes, NrOfReadBytes);
            *pError = NXP_I2C_NoAck; /* treat all errors as nack */
            ln = 0;
        }
    }
    else {
        /* not a valid target address, error */
        PRINT_ERROR("error in target address: 0x%x\n", msg_dest);
        *pError = NXP_I2C_NoAck; /* treat all errors as nack */
        ln = 0;
    }
    
    return ln;
}
 
int lxDspMmapClose(int fd) {

    (void)fd; /* Avoid warning */

    /* Is there something to unmap? */
    if(dsp_p_mm == NULL) {
        return 0;
    }

    /* release memory mapped region */
    if (munmap(dsp_p_mm, MMAP_LENGTH) == -1) {
        PRINT_ERROR("munmap() error()\n");
        perror("error");
    } 
    
    /* release semaphore */
    if (sem_close(sem) < 0) {
        PRINT_ERROR("sem_close() error\n");
        perror("error");
    }
    
    dsp_p_mm = NULL;
    pI2cParams = NULL;    
    pVirt_pin_mm = NULL;
    
    return 0;
}
 
static int lxDspMmapVersion(char *buffer, int fd)
{
	sprintf(buffer, "DspMmap v0.1");
	/* Remove unreferenced parameter warning C4100 */
	(void)fd;
	return (int)strlen(buffer);
}

int lxDspMmapSetPin(int fd, int pin, int value)
{
	(void)fd; /* Avoid warning */
	int maxPinOffset = 0;
	NXP_I2C_Error_t retVal = NXP_I2C_UnassignedErrorCode;

	/* Pin is represented by unsigned short (16 bits). */
	/* Max number of pins that can be stored is dependent of reserved size.
	   Make sure not to exceed memory boundary */
	maxPinOffset = ( (1 + MMAP_VIRT_PIN_MEM_SIZE) / 2 ); 

	if((value >= 0) && (value <= USHRT_MAX)) {
		// ok. value within 16bit range.

		if((pin > 0) && (pin <= maxPinOffset)) {
			// valid pin range, write data to mmap
			sem_lock();
			pVirt_pin_mm[pin-1] = (unsigned short)(value & USHRT_MAX); // Truncate values larger than 16bits.
			sem_unlock();
			retVal = NXP_I2C_Ok; 
		}	
		else {
			retVal = NXP_I2C_UnsupportedValue;
			PRINT_ERROR("Pin number %d out of range\r\n",pin);
		}
	}
	else {
		PRINT_ERROR("Pin value out of bounds. (max range 0 - %d)\r\n", USHRT_MAX);
		retVal = NXP_I2C_UnsupportedValue;
	}

	return retVal;
}

int lxDspMmapGetPin(int fd, int pin)
{
	(void)fd; /* Avoid warning */
	int pinVal = -1;

	/* Pin is represented by unsigned short (16 bits), check if requested pin is within memory range */
	int maxPinOffset = ( (1 + MMAP_VIRT_PIN_MEM_SIZE) / 2 );
	if((pin > 0) && (pin <= maxPinOffset)) {
		// valid range, read data from mmap
		sem_lock();
		pinVal = pVirt_pin_mm[pin-1];
		sem_unlock();
	}	
	else {
		PRINT_ERROR("Invalid pin number!\r\n");
		pinVal = -1;
	}
	return pinVal;
}

static int lxDspMmapBufferSize(void)
{
    /* return the maximum transfer size for a single write/read action on mmap */
	return MMAP_MAX_RW_BUFSIZE;
}

const struct nxp_i2c_device lxDspMmap_device = {
	lxDspMmapInit,
	lxDspMmapWriteRead,
	lxDspMmapVersion,
	lxDspMmapSetPin,
	lxDspMmapGetPin,
	lxDspMmapClose,
	lxDspMmapBufferSize,
	NULL,
	NULL
};
