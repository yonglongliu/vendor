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
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "hal_registers.h"
#include "NXP_I2C.h"

#ifdef __arm__
#define HAL "/dev/i2c-2"
#else
#define HAL "dummy88"
#endif

#define TFATYPE 0x88

unsigned short tfatype = TFATYPE; // this is the type used for bitfield name lookup
static int verbose=0;
extern NXP_I2C_Error_t hal_last_error;

/*
 * assume reg functions are ok, no full checking anymore
 */
int clock_on(int slave) {
	NXP_I2C_Error_t error;
	int loop=10;
	write_register_bf(slave, "CFE", 1); // cf on
	write_register_bf(slave, "PWDN", 0); // pll on
	// wait for CLKS 1
	do {
		if ( read_register_bf(slave, "CLKS") )
			return 0;
	} while(loop--);
	printf("!no clock, PLL did not start\n");
	return 1;
}
/*
 *
 */
int hal_memtest(int slave, int size) {
	uint8_t *inbuffer, *outbuffer;
	int i;

	inbuffer = malloc(3 * size);
	outbuffer = malloc(3 * size);
	for (i = 1; i < 3 * size; i++)
		inbuffer[i] = i;
	inbuffer[0] = get_bf_address("MEMA");
	bzero(outbuffer, 3 * size);

	write_register(slave, get_bf_address("MADD"), 0);
	hal_last_error = NXP_I2C_WriteRead(slave << 1, size, inbuffer, 0, 0);
	if (hal_last_error != NXP_I2C_Ok )
		return 1;
	write_register(slave, get_bf_address("MADD"), 0);
	//  read size is 1 less because write has the reg addres byte
	hal_last_error = NXP_I2C_WriteRead(slave << 1, 1, inbuffer, size - 1,
			outbuffer);
	if (hal_last_error != NXP_I2C_Ok )
		return 2;
	// compare the buffers
	if (memcmp(inbuffer + 1, outbuffer, size - 1)) {
		printf("mem compare mismatch\n");
		return -1;
	}
	free(outbuffer);
	free(inbuffer);

	return 0;
}
/* pin test */
int hal_pintest(int pin, int value) {
	int newval;
    /* set 1 */
   	hal_last_error = NXP_I2C_SetPin(pin, value);
   	if (NXP_I2C_Ok != hal_last_error) {
   		printf("NXP_I2C_SetPin(%d,%d) error:%s\n",pin, value,
   					NXP_I2C_Error_string(hal_last_error));
   		return 1;
   	}

    newval = NXP_I2C_GetPin(pin);
    if ( newval != value ) {
   		printf("NXP_I2C_GetPin(%d) unexpected value: expected:%d, actual:%d\n",
   					pin, newval, value);
   		return 1;

    }
    return 0;
}
/*
 * check if network target by  searching for : or @ in the name
 */
int is_network(char *target) {
	return ( strchr(target, ':') || strchr(target, '@'));
}
/*
 * hal_test [target_name] [verbose flags] [trace_file]
 *   target_name  : like -d climax option
 *   verbose flags: sets verbose if bit0 and tracing if non-0
 *   trace_file   : all NXP_I2C trace output (over)written to this file
 *
 *   Note: only the tfa9888 devicetype is supported now
 *
 */
int main(int argc, char* argv[])
{
	int i, size;
	uint16_t slave = 0x34, value;
	char target_name[FILENAME_MAX] = HAL;
	char version[1024] = ""; // must be terminated
	char trace_filename[FILENAME_MAX] = ""; //empty means not used
	uint8_t buffer[16];

	/* hal target name */
	if ( argc>1 )
		strncpy(target_name, argv[1], sizeof(target_name));

	/* verbose flags: sets verbose if bit0 and tracing if non-0 */
	if ( argc>2 )
		verbose = atoi(argv[2]);

	/* use filetracing if specified */
	if ( argc>3 )
		strncpy(trace_filename, argv[3], sizeof(trace_filename));

	NXP_I2C_Trace(verbose); // sets verbose if bit0 and tracing if non-0

	if (strlen(trace_filename)) {
		if (unlink(trace_filename) == 0)
			printf("removed existing file:%s\n", trace_filename);
		NXP_I2C_Trace_file(trace_filename);
	}
	if ( NXP_I2C_Open(target_name) <0 )
		return(1);

	if ( verbose ) {
		if ( NXP_I2C_Version(version) != NXP_I2C_Ok )
			return(1);
		NXP_I2C_Scribo_Name(target_name, sizeof(target_name));
		printf(" \'%s\' version: %s", target_name, version);
		printf(" interface type flags:0x%02x\n", NXP_I2C_Interface_Type());
		printf(" dump error strings:\n\t");
		for(i=0 ; i< NXP_I2C_ErrorMaxValue ; i++)
			printf(" %s", NXP_I2C_Error_string(i));
		printf("\n");
		printf(" HAL BufferSize:%d\n", NXP_I2C_BufferSize());
		if (strlen(trace_filename))
			printf(" trace file:%s\n", trace_filename);
	}

    /* call NXP_I2C_WriteRead() write only */
	write_register_bf(slave, "SWPROFIL", 0x12);
    if ( hal_last_error != NXP_I2C_Ok ) {
    	printf("line %d:NXP_I2C_WriteRead() call error: %s\n",__LINE__, NXP_I2C_Error_string(hal_last_error));
    	return(hal_last_error);
    }
    /**************************************************************/
    /* call NXP_I2C_WriteRead() write only, invalid bitfield test */
	write_register_bf(slave, "SWPROFILwww", 0x12);
    if ( hal_last_error != NXP_I2C_UnsupportedValue ) {
    	printf("line %d:NXP_I2C_WriteRead() invalid bitfield unexpected error: %s\n",__LINE__, NXP_I2C_Error_string(hal_last_error));
    	return(hal_last_error);
    }
    /**************************************************************/
    /* call NXP_I2C_WriteRead() read, expect revid */
    value = read_register(slave, 3);
    if ( hal_last_error != NXP_I2C_Ok ) {
    	printf("line %d:NXP_I2C_WriteRead() call error: %s\n",__LINE__, NXP_I2C_Error_string(hal_last_error));
    	return(hal_last_error);
    }
    value &= 0xff; // only lsb
    if ( value != TFATYPE) {
    	printf("line %d:unexpected revid: expected:0x%02x, actual:0x%02x\n",__LINE__, TFATYPE, value);
    	return 1;
    }
    /**************************************************************/
    /* call NXP_I2C_WriteRead() read, expect 0x12 in SWPROFIL */
    value = read_register_bf(slave, "SWPROFIL");
    if ( hal_last_error != NXP_I2C_Ok ) {
    	printf("line %d:NXP_I2C_WriteRead() call error: %s\n",__LINE__, NXP_I2C_Error_string(hal_last_error));
    	return(hal_last_error);
    }
    if ( value != 0x12) {
    	printf("line %d:unexpected SWPROFIL: expected:0x%02x, actual:0x%02x\n",__LINE__, 0x12, value);
    	return 1;
    }
    /**************************************************************/
    /* check proper bitfield handling using cf control register bitfield RST DMEM */
    write_register_bf(slave, "RST", 1);
    write_register_bf(slave, "DMEM", 2);
    if ( read_register_bf(slave, "RST")!= 1) {
    	printf("line %d:bitfield handling corrupted other bitfield\n",__LINE__);
    	return 1;
    }
//fail test    write_register(slave, 0x90, 0);
    value = read_register_bf(slave, "DMEM");
    if ( value != 2) {
    	printf("line %d:wrong bitfield read back: expected:0x%02x, actual:0x%02x\n",__LINE__, 2, value);
    	return 1;
    }

    /**************************************************************/
    /* run i2c burst below and more than buffersize */
    /* using xmem for this, DSP is already in reset and DMEM=xmem */
    if (clock_on(slave)) // need clock
    	return 1;
    size = NXP_I2C_BufferSize();
    if (hal_memtest(slave, size))
    	return 1;

    /**************************************************************/
    /* buffers too big */
    if (!is_network(target_name) ) {
    	printf("expected error: \'NXP_I2C_WriteRead: too many bytes to write: %d\'\n", size+2);
    	hal_last_error = NXP_I2C_WriteRead(slave << 1, size+2, buffer, 0, 0);
    	if ( hal_last_error != NXP_I2C_UnsupportedValue ) {
    		printf("line %d:wrong write error returned: expected:%s, actual:%s\n",__LINE__,
    				NXP_I2C_Error_string(NXP_I2C_UnsupportedValue),
					NXP_I2C_Error_string(hal_last_error));
    		return 1;
    	}
    	printf("expected error: \'NXP_I2C_WriteRead: too many bytes to read: %d\'\n", size+2);
    	hal_last_error = NXP_I2C_WriteRead(slave << 1, 0,0, size+2, 0);
    	if ( hal_last_error != NXP_I2C_UnsupportedValue ) {
    		printf("line %d:wrong read error returned: expected:%s, actual:%s\n",__LINE__,
    				NXP_I2C_Error_string(NXP_I2C_UnsupportedValue),
					NXP_I2C_Error_string(hal_last_error));
    		return 1;
    	}
    }
    else
    	printf("skipped big buffers invalid test for network targets\n");

    /**************************************************************/
    /* set pin 1 to 1 and 0 and check this */
    if (hal_pintest(1,1))
    	return 1;
    if (hal_pintest(1,0))
    	return 1;
    /* invalid pin */
   	hal_last_error = NXP_I2C_SetPin(1000, 0);
    if ( hal_last_error != NXP_I2C_UnsupportedValue ) {
    	printf("line %d:wrong write error returned: expected:%s, actual:%s\n",__LINE__,
    			NXP_I2C_Error_string(NXP_I2C_UnsupportedValue),
				NXP_I2C_Error_string(hal_last_error));
    	return 1;
    }
    /**************************************************************/
    /* NXP_I2C_Start/Stop Playback */
    if ( NXP_I2C_Interface_Type() & NXP_I2C_Playback) {
    	hal_last_error = NXP_I2C_StartPlayback("input_file");
        if ( hal_last_error == NXP_I2C_Ok ) {
        	printf("line %d:wrong NXP_I2C_StartPlayback error returned: expected:some error, actual:NXP_I2C_Ok\n",__LINE__);
        	return 1;
        }
    	hal_last_error = NXP_I2C_StopPlayback();
        if ( hal_last_error == NXP_I2C_Ok ) {
        	printf("line %d:wrong NXP_I2C_StopPlayback error returned: expected:some error, actual:NXP_I2C_Ok\n",__LINE__);
        	return 1;
        }
    }
    /**************************************************************/
    /* TFADSP msg interface */
    if ( NXP_I2C_Interface_Type() & NXP_I2C_Msg) {
    	printf("line %d:TFADSP msg interface no function test yet! TBD\n",__LINE__);
    }
    else { // should return -1
    	i = NXP_TFADSP_Init(0,0);
    	if ( i != -1 ){
    		printf("line %d:NXP_TFADSP_Init(0,0) wrong return: expected:%d, actual:%d\n",__LINE__, -1, i);
    		return 1;
    	}
    	i = NXP_TFADSP_Execute(0,0,0,0);
    	if ( i != -1 ){
    		printf("line %d:NXP_TFADSP_Execute(0,0,0,0) wrong return: expected:%d, actual:%d\n",__LINE__, -1, i);
    		return 1;
    	}
    }
    exit:
    NXP_I2C_Close();

    printf("all tests passed\n");
    return 0;
}
