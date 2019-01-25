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

#include <alsa/asoundlib.h>
#include <alsa/control_external.h>

#include "dbgprint.h"
#include "lxScribo.h"
#include "NXP_I2C.h" /* for the error codes */

snd_ctl_t *ctl;
snd_ctl_elem_value_t *val;

#define I2C_BUFFER_SIZE 510 /* Temporary buffer for the Set/Get parameter function */
static char msg_buf[I2C_BUFFER_SIZE];

static void hexdump(int num_write_bytes, const unsigned char * data)
{
	int i;

	for(i=0;i<num_write_bytes;i++)
	{
		PRINT("0x%02x ", data[i]);
	}

}

static void selectIElement(char* elem_name) {
	snd_ctl_elem_value_set_interface(val, SND_CTL_ELEM_IFACE_MIXER);
	snd_ctl_elem_value_set_name(val, elem_name);
}

static int get_val(char *elem_name) {
	selectIElement(elem_name);
	snd_ctl_elem_read(ctl, val);

	return snd_ctl_elem_value_get_integer(val, 0);
}

static void set_val(char *elem_name, int value) {
	selectIElement(elem_name);
	snd_ctl_elem_read(ctl, val);
	snd_ctl_elem_value_get_integer(val, 0);

	snd_ctl_elem_value_set_integer(val, 0, value);
	snd_ctl_elem_write(ctl, val);
}

static void set_bytes(char *elem_name, char *bytes, int length) {
	selectIElement(elem_name);
	//snd_ctl_elem_read(ctl, val);

	snd_ctl_elem_set_bytes(val, bytes, length);
	snd_ctl_elem_write(ctl, val);
}

static int get_bytes(char *elem_name, char *bytes, int length) {
	char *data;

	selectIElement(elem_name);
	snd_ctl_elem_read(ctl, val);

	if (length > 512) {
		length = 512;
	}

	data = (char*)snd_ctl_elem_value_get_bytes(val); // 512 length

	memcpy(bytes, data, length);
	return length;
}

static int lxAlsaMixerCtlInit(char *devname)
{
	char *name = "alspro";
	snd_ctl_card_info_t *hw_info;

	snd_ctl_card_info_alloca(&hw_info);

	if(snd_ctl_open(&ctl, name, 0)<0)
	{
	        PRINT_ERROR("snd_ctl_open Error\n");
	        return -1;
	}
	
	if(snd_ctl_card_info(ctl, hw_info)< 0)
	{
			PRINT_ERROR("snd_ctl_card_info Error\n");
			return -1;
	}

	//if (lxScribo_verbose) {
		PRINT("name %s\n", snd_ctl_card_info_get_driver(hw_info));
	//}

	snd_ctl_elem_value_malloc(&val);

	return 0;
}

static int lxAlsaMixerCtlWriteRead(int fd, int NrOfWriteBytes, const uint8_t * WriteData,
		int NrOfReadBytes, uint8_t * ReadData, unsigned int *pError) 
{
	int ln = -1;
	unsigned int msg_id = 0;
	int msg_len = -1;
	int msg_stat = -1;

	// decode message id and length of DSP message
	if (NrOfReadBytes == 0 && NrOfWriteBytes != 0) {
		msg_id = WriteData[0]<<16 | WriteData[1]<<8 | WriteData[2];
		msg_len = NrOfWriteBytes;

		// copy DSP message into local buffer
		memcpy(msg_buf, WriteData+3, msg_len);
	}
	else if (NrOfWriteBytes == 0  && NrOfReadBytes != 0) {
		msg_id = ReadData[0]<<16 | ReadData[1]<<8 | ReadData[2];
		msg_len = NrOfReadBytes;

		// copy DSP message into local buffer
		memcpy(msg_buf, ReadData+3, msg_len);
	}
	else {
		PRINT_ERROR("error in decode msg_id\n");
		return 0;
	}

	//if (lxScribo_verbose) {
		PRINT("msg_id: 0x%06x, msg_len: 0x%06x\n", msg_id, msg_len);
	//}

	*pError = NXP_I2C_Ok;
	ln = msg_len-1;

	if (NrOfWriteBytes && i2c_trace) {
		PRINT("W %d:", NrOfWriteBytes);
	    hexdump (NrOfWriteBytes, WriteData);
	    PRINT("\n");
	}

	// send DSP messages
	if (NrOfReadBytes == 0 || ReadData == NULL) {
		// write to DSP (setParam)
		set_val("msg_id", msg_id);
		set_bytes("msg_buffer", msg_buf, msg_len);
	}
	else {
		// read from DSP (getParam)
		set_val("msg_id", msg_id);
		get_bytes("msg_buffer", msg_buf, msg_len);
	}

	if (NrOfReadBytes && i2c_trace) {
		PRINT("R %d:", NrOfReadBytes);
	    hexdump (NrOfReadBytes, ReadData);
	    PRINT("\n");
	}

	msg_stat = get_val("msg_status");

	//if (lxScribo_verbose) {
		PRINT("msg_status: 0x%04x\n", msg_stat);
	//}

	if (msg_stat != 0x1 /* DSP_ACK */ ) {
		*pError = NXP_I2C_NoAck; /* treat all errors as nack */
	}

	if (*pError != NXP_I2C_Ok) {
		perror("i2c slave error");
	    return 0;
	}

	return (ln+1);
}
 
static int lxAlsaMixerCtlClose(int fd) {

	(void)fd; /* Avoid warning */
	snd_ctl_elem_value_free(val);
	return 0;
}
 
static int lxAlsaMixerCtlVersion(char *buffer, int fd)
{
	sprintf(buffer, "AlsaMixerCtl v0.1");
	/* Remove unreferenced parameter warning C4100 */
	(void)fd;
	return (int)strlen(buffer);
}

static int lxAlsaMixerCtlSetPin(int fd, int pin, int value)
{
    (void)fd; /* Avoid warning */
    (void)pin; /* Avoid warning */
    (void)value; /* Avoid warning */
    //PRINT("Not implemented! \n");
	return 0;
}

static int lxAlsaMixerCtlGetPin(int fd, int pin)
{
    (void)fd; /* Avoid warning */
    (void)pin; /* Avoid warning */
    //PRINT("Not implemented! \n");
	return 0;
}

const struct nxp_i2c_device lxAlsaMixerCtl_device = {
	lxAlsaMixerCtlInit,
	lxAlsaMixerCtlWriteRead,
	lxAlsaMixerCtlVersion,
	lxAlsaMixerCtlSetPin,
	lxAlsaMixerCtlGetPin,
	lxAlsaMixerCtlClose,
	NULL,
	NULL,
	NULL
};
