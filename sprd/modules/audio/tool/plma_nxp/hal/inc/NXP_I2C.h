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

#ifndef NXP_I2C_H
#define NXP_I2C_H

/* The maximum I2C message size allowed for read and write buffers, incl the slave address */
#define NXP_I2C_MAX_SIZE 254 // TODO remove this'NXP_I2C_MAX_SIZE, its platform value

#ifdef __cplusplus
extern "C" {
#endif

/* A glue layer.
 * The NXP components will use the functions defined in this API to do the actual calls to I2C
 * Make sure you implement this to use your I2C access functions (which are SoC and OS dependent)
 */

/**
 * A list of supported I2C errors that can be returned.
 */
enum NXP_I2C_Error {
	NXP_I2C_UnassignedErrorCode,		/**< UnassignedErrorCode*/
	NXP_I2C_Ok,				/**< no error */
	NXP_I2C_NoAck, 		/**< no I2C Ack */
	NXP_I2C_TimeOut,		/**< bus timeout */
	NXP_I2C_ArbLost,		/**< bus arbritration lost*/
	NXP_I2C_NoInit,		/**< init was not done */
	NXP_I2C_UnsupportedValue,		/**< UnsupportedValu*/
	NXP_I2C_UnsupportedType,		/**< UnsupportedType*/
	NXP_I2C_NoInterfaceFound,		/**< NoInterfaceFound*/
	NXP_I2C_NoPortnumber,			/**< NoPortnumber*/
	NXP_I2C_BufferOverRun,			/**< BufferOverRun*/
	NXP_I2C_ErrorMaxValue				/**<impossible value, max enum */
};

typedef enum NXP_I2C_Error NXP_I2C_Error_t;

/**
 * Hal interface flags
 */
enum NXP_I2C_IfType {
	NXP_I2C_Unknown=0, 	/**< no type defined */
	NXP_I2C_Direct=1, 	/**< no bus interface */
	NXP_I2C_I2C=2, 		/**< i2c hardware interface */
	NXP_I2C_HID=4, 		/**< usb hid interface */
	NXP_I2C_Msg=8, 		/**< message capable interface */
	NXP_I2C_Playback=0x10, /**< implement audio playback */
	NXP_I2C_IfTypeMaxVale=0xff /**< max enum */
};
typedef enum NXP_I2C_IfType NXP_I2C_IfType_t;

/**
 * Open and register a target interface.
 *
 *  @param target interfacename
 *  @return filedescripter (if relevant)
 */
int NXP_I2C_Open(char *targetname);

/**
 * Close and un-register the target interface.
 *
 *  Note that the target may block the caller when it has not been properly
 *  shutdown.
 */
int NXP_I2C_Close(void);

/**
 *  Returns the maximum number of bytes that can be transfered in one burst transaction.
 *
 *  @param None
 *  @return max burst size in bytes
 */
int NXP_I2C_BufferSize(void);

/**
 *  Pass through function to get the scribo name
 *
 *  @param buf = char buffer to write the name
 *  @params count = the maximum size required
 */
void NXP_I2C_Scribo_Name(char *buf, int count);

/**
 *  Return the interface type
 *  @return NXP_I2C_IfType_t enum
 *
 */
NXP_I2C_IfType_t NXP_I2C_Interface_Type(void);

/**
 * Execute a write, followed by I2C restart and a read of num_read_bytes bytes.
   The read_buffer must be big enough to contain num_read_bytes.
 *
 *  @param sla = slave address
 *  @param num_write_bytes = size of data[]
 *  @param write_data[] = byte array of data to write
 *  @param num_read_bytes = size of read_buffer[] and number of bytes to read
 *  @param read_buffer[] = byte array to receive the read data
 *  @return NXP_I2C_Error_t enum
 */
NXP_I2C_Error_t NXP_I2C_WriteRead(  unsigned char sla,
				int num_write_bytes,
				const unsigned char write_data[],
				int num_read_bytes,
				unsigned char read_buffer[] );

/**
 * Enable/disable I2C transaction trace.
 *
 *  @param on = 1, off = 0
 *
 */
void NXP_I2C_Trace(int on);
void NXP_UDP_Trace(int on);

/**
 * Use tracefile to write trace output.
 *
 *  @param filename: 0=stdout,  "name"=filename, -1=close file
 *  @return filedescripter or -1 on error
 */
void NXP_I2C_Trace_file(char *filename);

/**
 * Read back the pin state.
 *
 *  @param pin number
 *  @return pin state
 */
int NXP_I2C_GetPin(int pin);

/**
 * Set the pin state.
 *
 *  @param pin number
 *  @param pin value to set
 *  @return NXP_I2C_Error_t enum
 */
NXP_I2C_Error_t NXP_I2C_SetPin(int pin, int value);

/**
 * Read back version info as a string.
 *
 *  @param string buffer to hold the string (will not exceed 1024 bytes)
 *  @return NXP_I2C_Error_t enum
 */
NXP_I2C_Error_t NXP_I2C_Version(char *data);

/**
 * Stop playback
 *
 *  @return NXP_I2C_Error_t enum
 */
NXP_I2C_Error_t NXP_I2C_StopPlayback(void);

/**
 * Start playback of named audiofile
 *
 *  @param audio file name, max 255 chars.
 *  @return NXP_I2C_Error_t enum
 */
NXP_I2C_Error_t NXP_I2C_StartPlayback(char *data);

/**
 * Return the string for the error.
 *
 *  @param  error code
 *  @return string describing NXP_I2C_Error_t enum
 */
const char *NXP_I2C_Error_string(NXP_I2C_Error_t error);

/**
 * Return the interface type.
 *
 *  @return the interface type NXP_I2C_IfType_t
 */
NXP_I2C_IfType_t NXP_I2C_Interface_Type(void);

/**
 * Execute a command on the tfadsp
 * The command in the buffer will be executed and the response will be returned.
 * The underlying command and result operations can be separated in write only
 * and read only by specifying zero length for the corresponding action.
 * For the result read this means that it relies on a command only call that has
 * executed before the result call is done with zero command length.
 *
 *   command_length!=0 and result_length==0
 *   	execute command in command buffer
 *   	return value is result[0]/tfadsp status or <0 if error
 *   command_length!=0 and result_length!=0
 *   	execute command in command buffer
 *   	read result to return buffer
 *   	return value is actual returned length or <0 if error
 *   command_length==0 and result_length!=0
 *   	read result to return buffer
 *   	return value is actual returned length or <0 if error
 *
 * In case the result_length is non-zero a result of maximum this length will be returned.
 * In case of zero the 1st word of the result will be
 * return as the call status.
 *
 *
 *  @param command buffer length in bytes
 *  @param void pointer to the command buffer
 *  @param result buffer length in bytes
 *  @param void pointer to the result buffer
 *  @return if <0 error
 *  @return if 0 success in single command
 *  @return if > 0 tfadsp status if result_length==0
 *  @return if > 0 return size if result_length!=0
 *
 *
 */
int NXP_TFADSP_Execute( int command_length, void *command_buffer,
					 int result_length, void *result_buffer);

#ifdef __cplusplus
}
#endif

#endif // NXP_I2C_H
