/*
 *Copyright 2014 NXP Semiconductors
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


/*
 *  	The code is organized and ordered in the layering that matches the real devices:
 *  		- HAL interface called via Scribo registered functions
 *  		- I2C bus/slave handling code
 *  		- TFA I2C registers read/write
 *  		- CoolFlux subsystem, mainly dev[thisdev].xmem, read and write
 *  		- DSP RPC interaction response
 *  		- utility and helper functions
 *  		- static default parameter array defs
 */

/*
 * include files
 */
#include <stdio.h>
#if !(defined(WIN32) || defined(_X64))
#include <unistd.h>
#include "tfa_ext.h"
#endif
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
/* TFA98xx API */
#include "hal_utils.h"
#include "dbgprint.h"
#include "Tfa98API.h"
#include "tfa_dsp_fw.h"
#include "tfa98xx_parameters.h"
#include "tfa9888n1b_genregs_por.h"
#include "tfa9872_device_genregs_POR.h"
//#include "tfa98xx_tfafieldnames.h"
#include "lxScribo.h"
#include "NXP_I2C.h"
#if (defined(WIN32) || defined(_X64))
#include <windows.h>
#else
#include <unistd.h>
#include <dlfcn.h> /* dload */
#include <pthread.h>
#endif
/*v2.0*/
#include "tfa9891_genregs.h"
#include "tfa98xx_genregs_N1C.h"
static char dummy_version_string[]="V1.0";

#define DUMMYVERBOSE if (lxDummy_verbose > 3)
/******************************************************************************
 * globals
 */
/*NAME TFA98XX was for max2 . MAX1 device will use TFA1*/
#define TFA1_CF_CONTROLS                     0x70
#define TFA1_CF_MAD                          0x71
#define TFA1_CF_MEM                          0x72
#define TFA1_CF_STATUS                       0x73
typedef enum dummyType {
	tfa9888,
	tfa9890,
	tfa9890b,
	tfa9891,
	tfa9895,
	tfa9897,
	tfa9872,
} dummyType_t;
static const char *typeNames[] = {
	"tfa9888",
	"tfa9890",
	"tfa9890b",
	"tfa9891",
	"tfa9895",
	"tfa9897",
	"tfa9872"
};
static int lxDummy_execute_message(int command_length, void *command_buffer,
		int read_length, void *read_buffer) ;
static int lxDummy_write_message(int fd, int length, const char *buffer);
static int lxDummy_read_message(int fd, int length, char *buffer);

void convertBytes2Data24(int num_bytes, const unsigned char bytes[],
			       int data[]);
/* globals */
static int dummy_pins[100];
static int globalState=0;
static int dummy_warm = 0; // can be set via "warm" argument
static int dummy_noclock = 0; // can be set via "noclk" argument
static int lxDummy_verbose = 0;
static int lxDummy_manstate_verbose=0;
int lxDummyFailTest;		/* nr of the test to fail */


/**********/
/* printf higher level function activity like patch version, reset dsp params */
#if defined (WIN32) || defined(_X64)
#define FUNC_TRACE if (lxDummy_verbose & 1) printf
#else
#define FUNC_TRACE(va...) if (lxDummy_verbose & 1) PRINT("    dummy: " va)
#endif
static const char *fsname[]={"48k","44.1k","32k","24k","22.05k","16k","12k","11.0025k","8k"};

#define TFA_XMEM_PATCHVERSION 0x12bf
#define TFA9897_XMEM_PATCHVERSION 0x0d7f
#define TFA9891_XMEM_PATCHVERSION 0x13ff

/******************************************************************************
 * module globals
 */
#define MAX_DUMMIES 4
#define MAX_TRACELINE_LENGTH 256
/*
 * device structure that keeps the state for each individual target
 */
#define CF_PATCHMEM_START				(1024 * 16)
#define CF_PATCHMEM_LENGTH				(64*1024) //512
#define CF_XMEM_START					0
#define CF_XMEM_LENGTH					4800	/* 7008 */
#define CF_XMEMROM_START				8192
#define CF_XMEMROM_LENGTH				2048
#define CF_YMEM_START					0
#define CF_YMEM_LENGTH					512
#define CF_YMEMROM_START				2048
#define CF_YMEMROM_LENGTH				1536
#define CF_IO_PATCH_START				768
#define CF_IO_PATCH_LENGTH				0x1000
#define BIQUAD_COEFF_LENGTH (BIQUAD_COEFF_SIZE*3)
//#define CF_IO_CF_CONTROL_REG			0x8100
struct emulated_device{
	dummyType_t type;
	int config_length;
	uint8_t slave; //dev[thisdev].slave
	//uint16_t *Reg;//[256];

	uint16_t Reg[256];
	uint8_t currentreg;		/* active register */
	int xmem_patch_version	; /* highest xmem address */

	//unsigned char *xmem;
	unsigned char *ymem;
	unsigned char *pmem;
	unsigned char *iomem;

	uint8_t xmem[(CF_XMEMROM_START + CF_XMEMROM_LENGTH) * 3];	/* TODO treat xmemrom differently */
	int memIdx;		/* set via cf mad */
	int irqpin;
	/* per device parameters */
	unsigned char *lastConfig;//[TFA2_ALGOPARAMETER_LENGTH]; //tfa2 is largest
	unsigned char *lastSpeaker;//[TFA2_SPEAKERPARAMETER_LENGTH];//tfa2 is largest
	unsigned char *lastPreset;//[TFA98XX_PRESET_LENGTH]; //tfa1 (only) preset could be stored in lastConfig
	unsigned char *filters;//[TFA98XX_FILTERCOEFSPARAMETER_LENGTH/BIQUAD_COEFF_LENGTH][BIQUAD_COEFF_LENGTH]
} ;
static struct emulated_device dev[MAX_DUMMIES]; // max
static struct emulated_device *emulmodel[MAX_DUMMIES];
static int thisdev=0;
#ifdef TFA_MUL
/*starting of thread for different instances of emulated devices*/
static pthread_t dsp_thread;
#endif

/* firmware return codes */
#define I2C_REQ_DONE            0x0000
                                         /** I2C protocol : is busy = 1 */
#define I2C_REQ_BUSY            0x0001
                                   /** I2C protocol : invalid module Id */
#define I2C_REQ_INVALID_M_ID    0x0002
                                    /** I2C protocol : invalid param Id */
#define I2C_REQ_INVALID_P_ID    0x0003
                       /** I2C protocol : invalid channel configuration */
#define I2C_REQ_INVALID_CC      0x0004
                        /** I2C protocol : wrong sequence of parameters */
#define I2C_REQ_INVALID_SEQ     0x0005
                                    /** I2C protocol : wrong parameters */
#define I2C_REQ_INVALID_PARAM   0x0006
                               /** I2C protocol : I2C buffer overflowed */
#define I2C_REQ_BUFFER_OVERFLOW 0x0007
                 /** I2C protocol : Calibration not finished on Get_Re0 */
#define I2C_REQ_CALIB_BUSY      0x0008
                 /** I2C protocol : Calibration failed */
#define I2C_REQ_CALIB_FAILED    0x0009

/* tiberius message */
static char result_buffer[550*4];

#if (defined(WIN32) || defined(_X64))
void bzero(void *s, size_t n)
{
	memset(s, 0, n);
}

float roundf(float x)
{
	return (float)(int)x;
}
#endif

/*  */
 /* - I2C bus/slave handling code */
/*  */

static int i2cWrite(int length, const uint8_t *data);
static int i2cRead(int length, uint8_t *data);
/*  */
/* - TFA registers read/write */
/*  */
static void resetRegs(dummyType_t type);
static void setRomid(dummyType_t type);
static int tfaRead(uint8_t *data);
static int tfaWrite(const uint8_t *data);
/*  */
/* - CoolFlux subsystem, mainly dev[thisdev].xmem, read and write */
/*  */

static int lxDummyMemr(int type, uint8_t *data);
static int lxDummyMemw(int type, const uint8_t *data);
static int isClockOn(int thisdev); /* True if CF is ok and running */
/*  */
/* - DSP RPC interaction response */
/*  */
static int setgetDspParamsSpeakerboost(int param);	/* speakerboost */
static int setDspParamsFrameWork(int param);
static int setDspParamsEq(int param);
static int setDspParamsRe0(int param);

/* - function emulations */
static int updateInterrupt(void);

 /* - utility and helper functions */
static int dev2fam(struct emulated_device *this);
static int setInputFile(char *file);

static int dummy_get_bf(struct emulated_device *this, const uint16_t bf);
static int dummy_set_bf(struct emulated_device *this, const uint16_t bf, const uint16_t value);

/* function to retrive live_data variables from file */
int fill_up_buffer_live_data(char *bytes);
//unsigned char live_data_model_copied[153];
char str_live_data[1250];//=NULL;
/*open file for live data and store the string in variable str*/
FILE *Fd;

/*
 * redefine tfa1/2 family abstraction macros
 *  using the global device structures
 *   NOTE no need for dev_idx, it's always thisdev
 */
#define IS_FAM (dev2fam(&dev[thisdev]))
#undef TFA_FAM
#define TFA_FAM(fieldname) ((dev2fam(&dev[thisdev]) == 1) ? TFA1_BF_##fieldname :  TFA2_BF_##fieldname)
#undef TFA_FAM_FW
#define TFA_FAM_FW(fwname) ((dev2fam(&dev[thisdev])== 1) ? TFA1_FW_##fwname :  TFA2_FW_##fwname)

/* set/get bit fields to HW register*/
#undef TFA_SET_BF
#undef TFA_GET_BF
#define TFA_SET_BF(fieldname, value) dummy_set_bf(&dev[thisdev], TFA_FAM(fieldname), value)
#define TFA_GET_BF(fieldname) dummy_get_bf(&dev[thisdev], TFA_FAM(fieldname))

//#ifdef REMOVEV2_0
/* abstract family for register */
#define TFA98XX_CF_CONTROLS_REQ_MSK      0xff00
#define TFA98XX_CF_CONTROLS_RST_MSK      0x1
#define TFA98XX_CF_CONTROLS_CFINT_MSK    0x10
#define FAM_TFA98XX_CF_CONTROLS (TFA_FAM(RST) >>8) //use bitfield def to get register
#define FAM_TFA98XX_CF_MADD      (TFA_FAM(MADD)>>8)
#define FAM_TFA98XX_CF_MEM      (TFA_FAM(MEMA)>>8)
#define FAM_TFA98XX_CF_STATUS   (TFA_FAM(ACK)>>8)
#define FAM_TFA98XX_TDM_REGS	(TFA_FAM(TDMFSPOL)>>8) //regs base
// interrupt register have no common bitnames
#define FAM_TFA98XX_INT_REGS	(((dev2fam(&dev[thisdev]) == 1) ? 0x20 :  0x40)) // regsbase


/************************************ BFFIELD_TRACE ***************************************/

//#define TFA_DUMMY_BFFIELD_TRACE
//TODO get pointer to the table
#define SPKRBST_HEADROOM			7

//#ifdef TFA_DUMMY_BFFIELD_TRACE
#include "tfa98xx_parameters.h"
#include "tfa98xx_tfafieldnames.h"
#include "tfa_service.h"

TFA2_NAMETABLE //TOTO remove defines here
TFA9891_NAMETABLE
static char *tfa_fam_find_bitname( unsigned short bfEnum) {

	/* Remove the unused warning */
	(void)bfEnum;
	return	0; //tfaContBitName(bfEnum, 0xdeadbeef, dev[thisdev].Reg[3]&0xff);

}

static int tfa_fam_trace_bitfields(int reg, uint16_t oldval, uint16_t newval) { //TODO fix or old max types
	uint16_t change = oldval ^ newval;
	uint16_t bfmask;
	int cnt = 0, bfname_idx, pos;
	int max_bfs=0;
	tfaBfName_t *bf_name,*DatasheetNames = {0};
	nxpTfaBfEnum_t *bf;
	char traceline[MAX_TRACELINE_LENGTH], thisname[MAX_TRACELINE_LENGTH];

	if (oldval == newval)
		return 0;
	if (IS_FAM == 1) {
		if (dev[thisdev].Reg[3]==0x92) {

		} else {
			DatasheetNames = Tfa9891DatasheetNames;
			max_bfs = (sizeof(Tfa9891DatasheetNames) / sizeof(tfaBfName_t) - 1);
		}
	}
	else {
		DatasheetNames = Tfa2DatasheetNames;
		max_bfs = (sizeof(Tfa2DatasheetNames) / sizeof(tfaBfName_t) - 1);
	}

	traceline[0]='\0';
	/* get through the bitfields and look for target register */
	for (bfname_idx = 0; bfname_idx < max_bfs; bfname_idx++) { // datasheet names loop
		do { //register loop
			bf_name = &DatasheetNames[bfname_idx];
			bf = (nxpTfaBfEnum_t *) &bf_name->bfEnum;
			pos = bf->pos;
			bfmask = ((1 << (bf->len + 1)) - 1) << pos;
			if (bf->address != (unsigned int)reg)
				break;
			/* go to the first changed  bit and find the bitfield that describes it */
			if (change & bfmask) { //changed bit in this bitfield
				int new = (bfmask & newval) >> pos;
				int old = (bfmask & oldval) >> pos;
				if (bf->len > 0) // only print last value for longer fields
					sprintf(thisname, " %s (%s)=%d (%d)", bf_name->bfName,
							tfa_fam_find_bitname(bf_name->bfEnum), new, old);
				else
					sprintf(thisname," %s (%s)=%d", bf_name->bfName,
							tfa_fam_find_bitname(bf_name->bfEnum), new);
				cnt++;
				strncat(traceline, thisname,
						sizeof(traceline) - strlen(traceline) - strlen(thisname));
			}
			bfname_idx++; //bitfieldloop
		} while (bf->address == (unsigned int)reg); //register loop
	} // datasheet names loop

	if (cnt==0) /* none found, just print reg */
		sprintf(traceline, " 0x%02x=0x%04x (0x%04x)", reg, newval, oldval);

	if (lxDummy_verbose)// & 2)
		PRINT("\t%s\n",traceline);

	return cnt;
}

/*
 * print the list of changed bitfields in the named register
 */
int lxDummy_trace_bitfields(int reg, uint16_t oldval, uint16_t newval) {

	return tfa_fam_trace_bitfields(reg, oldval, newval);

//	if (IS_FAM==2)
//		return tfa2_trace_bitfields(reg, oldval, newval);
//	else if (IS_FAM==1)
//		return tfa1_trace_bitfields(reg, oldval, newval);
//
//	PRINT_ERROR("family type is wrong:%d\n", IS_FAM);
//	return -1;
}

//!TODO #endif // TFA_DUMMY_BFFIELD_TRACE


/* the following replaces the former "extern regdef_t regdefs[];"
 *  TODO replace this by a something generated from the regmap
 */
/* *INDENT-OFF* */
static regdef_t regdefs[] = {
		{TFA98XX_SYS_CONTROL0, TFA98XX_SYS_CONTROL0_POR, 0, "TFA98XX_SYS_CONTROL0"},
		{TFA98XX_SYS_CONTROL1, TFA98XX_SYS_CONTROL1_POR, 0, "TFA98XX_SYS_CONTROL1_POR"},
		{TFA98XX_SYS_CONTROL2, TFA98XX_SYS_CONTROL2_POR, 0, "TFA98XX_SYS_CONTROL2_POR"},
		{TFA98XX_DEVICE_REVISION, TFA98XX_DEVICE_REVISION_POR, 0, "TFA98XX_DEVICE_REVISION_POR"},
		{TFA98XX_CLOCK_CONTROL, TFA98XX_CLOCK_CONTROL_POR, 0, "TFA98XX_CLOCK_CONTROL_POR"},
		{TFA98XX_CLOCK_GATING_CONTROL, TFA98XX_CLOCK_GATING_CONTROL_POR, 0, "TFA98XX_CLOCK_GATING_CONTROL_POR"},
		{TFA98XX_SIDE_TONE_CONFIG, TFA98XX_SIDE_TONE_CONFIG_POR, 0, "TFA98XX_SIDE_TONE_CONFIG_POR"},
		{TFA98XX_STATUS_FLAGS0, TFA98XX_STATUS_FLAGS0_POR, 0, "TFA98XX_STATUS_FLAGS0_POR"},
		{TFA98XX_STATUS_FLAGS1, TFA98XX_STATUS_FLAGS1_POR, 0, "TFA98XX_STATUS_FLAGS1_POR"},
		{TFA98XX_STATUS_FLAGS2, TFA98XX_STATUS_FLAGS2_POR, 0, "TFA98XX_STATUS_FLAGS2_POR"},
		{TFA98XX_STATUS_FLAGS3, TFA98XX_STATUS_FLAGS3_POR, 0, "TFA98XX_STATUS_FLAGS3_POR"},
		{TFA98XX_BATTERY_VOLTAGE, TFA98XX_BATTERY_VOLTAGE_POR, 0, "TFA98XX_BATTERY_VOLTAGE_POR"},
		{TFA98XX_TEMPERATURE, TFA98XX_TEMPERATURE_POR, 0, "TFA98XX_TEMPERATURE_POR"},
		{TFA98XX_TDM_CONFIG0, TFA98XX_TDM_CONFIG0_POR, 0, "TFA98XX_TDM_CONFIG0_POR"},
		{TFA98XX_TDM_CONFIG1, TFA98XX_TDM_CONFIG1_POR, 0, "TFA98XX_TDM_CONFIG1_POR"},
		{TFA98XX_TDM_CONFIG2, TFA98XX_TDM_CONFIG2_POR, 0, "TFA98XX_TDM_CONFIG2_POR"},
		{TFA98XX_TDM_CONFIG3, TFA98XX_TDM_CONFIG3_POR, 0, "TFA98XX_TDM_CONFIG3_POR"},
		{TFA98XX_TDM_CONFIG4, TFA98XX_TDM_CONFIG4_POR, 0, "TFA98XX_TDM_CONFIG4_POR"},
		{TFA98XX_TDM_CONFIG5, TFA98XX_TDM_CONFIG5_POR, 0, "TFA98XX_TDM_CONFIG5_POR"},
		{TFA98XX_TDM_CONFIG6, TFA98XX_TDM_CONFIG6_POR, 0, "TFA98XX_TDM_CONFIG6_POR"},
		{TFA98XX_TDM_CONFIG7, TFA98XX_TDM_CONFIG7_POR, 0, "TFA98XX_TDM_CONFIG7_POR"},
		{TFA98XX_TDM_CONFIG8, TFA98XX_TDM_CONFIG8_POR, 0, "TFA98XX_TDM_CONFIG8_POR"},
		{TFA98XX_TDM_CONFIG9, TFA98XX_TDM_CONFIG9_POR, 0, "TFA98XX_TDM_CONFIG9_POR"},
		{TFA98XX_PDM_CONFIG0, TFA98XX_PDM_CONFIG0_POR, 0, "TFA98XX_PDM_CONFIG0_POR"},
		{TFA98XX_PDM_CONFIG1, TFA98XX_PDM_CONFIG1_POR, 0, "TFA98XX_PDM_CONFIG1_POR"},
		{TFA98XX_HAPTIC_DRIVER_CONFIG, TFA98XX_HAPTIC_DRIVER_CONFIG_POR, 0, "TFA98XX_HAPTIC_DRIVER_CONFIG_POR"},
		{TFA98XX_GPIO_DATAIN_REG, TFA98XX_GPIO_DATAIN_REG_POR, 0, "TFA98XX_GPIO_DATAIN_REG_POR"},
		{TFA98XX_GPIO_CONFIG, TFA98XX_GPIO_CONFIG_POR, 0, "TFA98XX_GPIO_CONFIG_POR"},
		{TFA98XX_INTERRUPT_OUT_REG1, TFA98XX_INTERRUPT_OUT_REG1_POR, 0, "TFA98XX_INTERRUPT_OUT_REG1_POR"},
		{TFA98XX_INTERRUPT_OUT_REG2, TFA98XX_INTERRUPT_OUT_REG2_POR, 0, "TFA98XX_INTERRUPT_OUT_REG2_POR"},
		{TFA98XX_INTERRUPT_OUT_REG3, TFA98XX_INTERRUPT_OUT_REG3_POR, 0, "TFA98XX_INTERRUPT_OUT_REG3_POR"},
		{TFA98XX_INTERRUPT_IN_REG1, TFA98XX_INTERRUPT_IN_REG1_POR, 0, "TFA98XX_INTERRUPT_IN_REG1_POR"},
		{TFA98XX_INTERRUPT_IN_REG2, TFA98XX_INTERRUPT_IN_REG2_POR, 0, "TFA98XX_INTERRUPT_IN_REG2_POR"},
		{TFA98XX_INTERRUPT_IN_REG3, TFA98XX_INTERRUPT_IN_REG3_POR, 0, "TFA98XX_INTERRUPT_IN_REG3_POR"},
		{TFA98XX_INTERRUPT_ENABLE_REG1, TFA98XX_INTERRUPT_ENABLE_REG1_POR, 0, "TFA98XX_INTERRUPT_ENABLE_REG1_POR"},
		{TFA98XX_INTERRUPT_ENABLE_REG2, TFA98XX_INTERRUPT_ENABLE_REG2_POR, 0, "TFA98XX_INTERRUPT_ENABLE_REG2_POR"},
		{TFA98XX_INTERRUPT_ENABLE_REG3, TFA98XX_INTERRUPT_ENABLE_REG3_POR, 0, "TFA98XX_INTERRUPT_ENABLE_REG3_POR"},
		{TFA98XX_STATUS_POLARITY_REG1, TFA98XX_STATUS_POLARITY_REG1_POR, 0, "TFA98XX_STATUS_POLARITY_REG1_POR"},
		{TFA98XX_STATUS_POLARITY_REG2, TFA98XX_STATUS_POLARITY_REG2_POR, 0, "TFA98XX_STATUS_POLARITY_REG2_POR"},
		{TFA98XX_STATUS_POLARITY_REG3, TFA98XX_STATUS_POLARITY_REG3_POR, 0, "TFA98XX_STATUS_POLARITY_REG3_POR"},
		{TFA98XX_BAT_PROT_CONFIG, TFA98XX_BAT_PROT_CONFIG_POR, 0, "TFA98XX_BAT_PROT_CONFIG_POR"},
		{TFA98XX_AUDIO_CONTROL, TFA98XX_AUDIO_CONTROL_POR, 0, "TFA98XX_AUDIO_CONTROL_POR"},
		{TFA98XX_AMPLIFIER_CONFIG, TFA98XX_AMPLIFIER_CONFIG_POR, 0, "TFA98XX_AMPLIFIER_CONFIG_POR"},
		{TFA98XX_DCDC_CONTROL0, TFA98XX_DCDC_CONTROL0_POR, 0, "TFA98XX_DCDC_CONTROL0_POR"},
		{TFA98XX_CF_CONTROLS, TFA98XX_CF_CONTROLS_POR, 0, "TFA98XX_CF_CONTROLS_POR"},
		{TFA98XX_CF_MAD, TFA98XX_CF_MAD_POR, 0, "TFA98XX_CF_MAD_POR"},
		{TFA98XX_CF_MEM, TFA98XX_CF_MEM_POR, 0, "TFA98XX_CF_MEM_POR"},
		{TFA98XX_CF_STATUS, TFA98XX_CF_STATUS_POR, 0, "TFA98XX_CF_STATUS_POR"},
		{TFA98XX_MTPKEY2_REG, TFA98XX_MTPKEY2_REG_POR, 0, "TFA98XX_MTPKEY2_REG_POR"},
		{TFA98XX_KEY_PROTECTED_MTP_CONTROL, TFA98XX_KEY_PROTECTED_MTP_CONTROL_POR, 0, "TFA98XX_KEY_PROTECTED_MTP_CONTROL_POR"},
		{TFA98XX_TEMP_SENSOR_CONFIG, TFA98XX_TEMP_SENSOR_CONFIG_POR, 0, "TFA98XX_TEMP_SENSOR_CONFIG_POR"},
		{TFA98XX_KEY2_PROTECTED_MTP0, TFA98XX_KEY2_PROTECTED_MTP0_POR, 0, "TFA98XX_KEY2_PROTECTED_MTP0_POR"},
		{TFA98XX_AUDIO_CONTROL2,TFA98XX_AUDIO_CONTROL2_POR,0,"TFA98XX_AUDIO_CONTROL2_POR" }
};

static regdef_t regdefs_72[] = {
		{TFA9872_SYS_CONTROL0 , TFA9872_SYS_CONTROL0_POR ,  0  , "TFA9872_SYS_CONTROL0_POR" } ,
		{TFA9872_SYS_CONTROL1 , TFA9872_SYS_CONTROL1_POR ,  0  , "TFA9872_SYS_CONTROL1_POR" } ,
		{TFA9872_SYS_CONTROL2 , TFA9872_SYS_CONTROL2_POR ,  0  , "TFA9872_SYS_CONTROL2_POR" } ,
		{TFA9872_DEVICE_REVISION , TFA9872_DEVICE_REVISION_POR ,  0  , "TFA9872_DEVICE_REVISION_POR" } ,
		{TFA9872_CLOCK_CONTROL , TFA9872_CLOCK_CONTROL_POR ,  0  , "TFA9872_CLOCK_CONTROL_POR" } ,
		{TFA9872_CLOCK_GATING_CONTROL , TFA9872_CLOCK_GATING_CONTROL_POR ,  0  , "TFA9872_CLOCK_GATING_CONTROL_POR" } ,
		{TFA9872_SIDE_TONE_CONFIG , TFA9872_SIDE_TONE_CONFIG_POR ,  0  , "TFA9872_SIDE_TONE_CONFIG_POR" } ,
		{TFA9872_STATUS_FLAGS0 , TFA9872_STATUS_FLAGS0_POR ,  0  , "TFA9872_STATUS_FLAGS0_POR" } ,
		{TFA9872_STATUS_FLAGS1 , TFA9872_STATUS_FLAGS1_POR ,  0  , "TFA9872_STATUS_FLAGS1_POR" } ,
		{TFA9872_STATUS_FLAGS3 , TFA9872_STATUS_FLAGS3_POR ,  0  , "TFA9872_STATUS_FLAGS3_POR" } ,
		{TFA9872_STATUS_FLAGS4 , TFA9872_STATUS_FLAGS4_POR ,  0  , "TFA9872_STATUS_FLAGS4_POR" } ,
		{TFA9872_BATTERY_VOLTAGE , TFA9872_BATTERY_VOLTAGE_POR ,  0  , "TFA9872_BATTERY_VOLTAGE_POR" } ,
		{TFA9872_TEMPERATURE , TFA9872_TEMPERATURE_POR ,  0  , "TFA9872_TEMPERATURE_POR" } ,
		{TFA9872_VDDP_VOLTAGE , TFA9872_VDDP_VOLTAGE_POR ,  0  , "TFA9872_VDDP_VOLTAGE_POR" } ,
		{TFA9872_TDM_CONFIG0 , TFA9872_TDM_CONFIG0_POR ,  0  , "TFA9872_TDM_CONFIG0_POR" } ,
		{TFA9872_TDM_CONFIG1 , TFA9872_TDM_CONFIG1_POR ,  0  , "TFA9872_TDM_CONFIG1_POR" } ,
		{TFA9872_TDM_CONFIG2 , TFA9872_TDM_CONFIG2_POR ,  0  , "TFA9872_TDM_CONFIG2_POR" } ,
		{TFA9872_TDM_CONFIG3 , TFA9872_TDM_CONFIG3_POR ,  0  , "TFA9872_TDM_CONFIG3_POR" } ,
		{TFA9872_TDM_CONFIG6 , TFA9872_TDM_CONFIG6_POR ,  0  , "TFA9872_TDM_CONFIG6_POR" } ,
		{TFA9872_TDM_CONFIG7 , TFA9872_TDM_CONFIG7_POR ,  0  , "TFA9872_TDM_CONFIG7_POR" } ,
		{TFA9872_PDM_CONFIG0 , TFA9872_PDM_CONFIG0_POR ,  0  , "TFA9872_PDM_CONFIG0_POR" } ,
		{TFA9872_INTERRUPT_OUT_REG1 , TFA9872_INTERRUPT_OUT_REG1_POR ,  0  , "TFA9872_INTERRUPT_OUT_REG1_POR" } ,
		{TFA9872_INTERRUPT_OUT_REG2 , TFA9872_INTERRUPT_OUT_REG2_POR ,  0  , "TFA9872_INTERRUPT_OUT_REG2_POR" } ,
		{TFA9872_INTERRUPT_OUT_REG3 , TFA9872_INTERRUPT_OUT_REG3_POR ,  0  , "TFA9872_INTERRUPT_OUT_REG3_POR" } ,
		{TFA9872_INTERRUPT_IN_REG1 , TFA9872_INTERRUPT_IN_REG1_POR ,  0  , "TFA9872_INTERRUPT_IN_REG1_POR" } ,
		{TFA9872_INTERRUPT_IN_REG2 , TFA9872_INTERRUPT_IN_REG2_POR ,  0  , "TFA9872_INTERRUPT_IN_REG2_POR" } ,
		{TFA9872_INTERRUPT_IN_REG3 , TFA9872_INTERRUPT_IN_REG3_POR ,  0  , "TFA9872_INTERRUPT_IN_REG3_POR" } ,
		{TFA9872_INTERRUPT_ENABLE_REG1 , TFA9872_INTERRUPT_ENABLE_REG1_POR ,  0  , "TFA9872_INTERRUPT_ENABLE_REG1_POR" } ,
		{TFA9872_INTERRUPT_ENABLE_REG2 , TFA9872_INTERRUPT_ENABLE_REG2_POR ,  0  , "TFA9872_INTERRUPT_ENABLE_REG2_POR" } ,
		{TFA9872_INTERRUPT_ENABLE_REG3 , TFA9872_INTERRUPT_ENABLE_REG3_POR ,  0  , "TFA9872_INTERRUPT_ENABLE_REG3_POR" } ,
		{TFA9872_STATUS_POLARITY_REG1 , TFA9872_STATUS_POLARITY_REG1_POR ,  0  , "TFA9872_STATUS_POLARITY_REG1_POR" } ,
		{TFA9872_STATUS_POLARITY_REG2 , TFA9872_STATUS_POLARITY_REG2_POR ,  0  , "TFA9872_STATUS_POLARITY_REG2_POR" } ,
		{TFA9872_STATUS_POLARITY_REG3 , TFA9872_STATUS_POLARITY_REG3_POR ,  0  , "TFA9872_STATUS_POLARITY_REG3_POR" } ,
		{TFA9872_BAT_PROT_CONFIG , TFA9872_BAT_PROT_CONFIG_POR ,  0  , "TFA9872_BAT_PROT_CONFIG_POR" } ,
		{TFA9872_AUDIO_CONTROL , TFA9872_AUDIO_CONTROL_POR ,  0  , "TFA9872_AUDIO_CONTROL_POR" } ,
		{TFA9872_AMPLIFIER_CONFIG , TFA9872_AMPLIFIER_CONFIG_POR ,  0  , "TFA9872_AMPLIFIER_CONFIG_POR" } ,
		{TFA9872_PGA_CONTROL0 , TFA9872_PGA_CONTROL0_POR ,  0  , "TFA9872_PGA_CONTROL0_POR" } ,
		{TFA9872_GAIN_ATT , TFA9872_GAIN_ATT_POR ,  0  , "TFA9872_GAIN_ATT_POR" } ,
		{TFA9872_TDM_SOURCE_CTRL , TFA9872_TDM_SOURCE_CTRL_POR ,  0  , "TFA9872_TDM_SOURCE_CTRL_POR" } ,
		{TFA9872_SAM_CTRL , TFA9872_SAM_CTRL_POR ,  0  , "TFA9872_SAM_CTRL_POR" } ,
		{TFA9872_STATUS_FLAGS5 , TFA9872_STATUS_FLAGS5_POR ,  0  , "TFA9872_STATUS_FLAGS5_POR" } ,
		{TFA9872_CURSENSE_COMP , TFA9872_CURSENSE_COMP_POR ,  0  , "TFA9872_CURSENSE_COMP_POR" } ,
		{TFA9872_DCDC_CONTROL0 , TFA9872_DCDC_CONTROL0_POR ,  0  , "TFA9872_DCDC_CONTROL0_POR" } ,
		{TFA9872_DCDC_CONTROL4 , TFA9872_DCDC_CONTROL4_POR ,  0  , "TFA9872_DCDC_CONTROL4_POR" } ,
		{TFA9872_DCDC_CONTROL5 , TFA9872_DCDC_CONTROL5_POR ,  0  , "TFA9872_DCDC_CONTROL5_POR" } ,
		{TFA9872_MTPKEY2_REG , TFA9872_MTPKEY2_REG_POR ,  0  , "TFA9872_MTPKEY2_REG_POR" } ,
		{TFA9872_MTP_STATUS , TFA9872_MTP_STATUS_POR ,  0  , "TFA9872_MTP_STATUS_POR" } ,
		{TFA9872_KEY_PROTECTED_MTP_CONTROL , TFA9872_KEY_PROTECTED_MTP_CONTROL_POR ,  0  , "TFA9872_KEY_PROTECTED_MTP_CONTROL_POR" } ,
		{TFA9872_MTP_DATA_OUT_MSB , TFA9872_MTP_DATA_OUT_MSB_POR ,  0  , "TFA9872_MTP_DATA_OUT_MSB_POR" } ,
		{TFA9872_MTP_DATA_OUT_LSB , TFA9872_MTP_DATA_OUT_LSB_POR ,  0  , "TFA9872_MTP_DATA_OUT_LSB_POR" } ,
		{TFA9872_TEMP_SENSOR_CONFIG , TFA9872_TEMP_SENSOR_CONFIG_POR ,  0  , "TFA9872_TEMP_SENSOR_CONFIG_POR" } ,
		{TFA9872_SOFTWARE_PROFILE , TFA9872_SOFTWARE_PROFILE_POR ,  0  , "TFA9872_SOFTWARE_PROFILE_POR" } ,
		{TFA9872_SOFTWARE_VSTEP , TFA9872_SOFTWARE_VSTEP_POR ,  0  , "TFA9872_SOFTWARE_VSTEP_POR" } ,
		{TFA9872_KEY2_PROTECTED_MTP0 , TFA9872_KEY2_PROTECTED_MTP0_POR ,  0  , "TFA9872_KEY2_PROTECTED_MTP0_POR" } ,
		{TFA9872_KEY2_PROTECTED_MTP5 , TFA9872_KEY2_PROTECTED_MTP5_POR ,  0  , "TFA9872_KEY2_PROTECTED_MTP5_POR" } ,
};

static regdef_t tdm_regdefs_mx1[] = {
		{ 0x10, 0x0220, 0, "tdm_config_reg0"},
		{ 0x11, 0xc1f1, 0, "tdm_config_reg1"},
		{ 0x12, 0x0020, 0, "tdm_config_reg2"},
		{ 0x13, 0x0000, 0, "tdm_config_reg3"},
		{ 0x14, 0x2000, 0, "tdm_config_reg4"},
		{ 0x15, 0x0000, 0, "tdm_status_reg"},
        { 0xff, 0,0, NULL}
};

static regdef_t int_regdefs_mx1[] = {
		{ 0x20, 0x0000, 0, "int_out0"},
		{ 0x21, 0x0000, 0, "int_out1"},
		{ 0x22, 0x0000, 0, "int_out2"},
		{ 0x23, 0x0000, 0, "int_in0"},
		{ 0x24, 0x0000, 0, "int_in1"},
		{ 0x25, 0x0000, 0, "int_in2"},
		{ 0x26, 0x0001, 0, "int_ena0"},
		{ 0x27, 0x0000, 0, "int_ena1"},
		{ 0x28, 0x0000, 0, "int_ena2"},
		{ 0x29, 0xf5e2, 0, "int_pol0"},
		{ 0x2a, 0xfc2f,  0, "int_pol1"},
		{ 0x2b, 0x0003, 0, "int_pol2"},
		{ 0xff, 0,0, NULL}
};

/* *INDENT-ON* */
/*
 * for debugging
 */
//int dummy_trace = 0;
/******************************************************************************
 * macros
 */

#define XMEM2INT(i) (dev[thisdev].xmem[i]<<16|dev[thisdev].xmem[i+1]<<8|dev[thisdev].xmem[i+2])

/* endian swap */
#define BE2LEW(x)   (( ( (x) << 8 ) | ( (x) & 0xFF00 ) >> 8 )&0xFFFF)
#define BE2LEDW( x)  (\
	       ((x) << 24) \
	     | (( (x) & 0x0000FF00) << 8 ) \
	     | (( (x) & 0x00FF0000) >> 8 ) \
	     | ((x) >> 24) \
	     )

/******************************************************************************
 *  - static default parameter array defs
 */
#define STATE_SIZE             9  // in words
#define STATE_SIZE_DRC            (2* 9)

/* life data models */
/* ls model (0x86) */
static unsigned char lsmodel[TFA2_SPEAKERPARAMETER_LENGTH]={0};
/* ls model (0xc1) */
//static unsigned char lsmodelw[423];

//This can be used to always get a "real" dumpmodel=x for the dummy. It is created from a real dumpmodel=x
static unsigned char dump_model[]={
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x07,0xeb,0xde,0xff,0xf9,0x75,0xff,0x9d,0x77,0x01,0x11,0xa5, 
0x00,0xaf,0x21,0x00,0xb1,0xaa,0xff,0xd4,0xf5,0xff,0xb9,0x65,0xff,0x72,0xf7,0xff,0xc9,0x3d, 
0xff,0xef,0x7a,0x00,0x2e,0x6e,0x00,0x27,0xdf,0x00,0x1b,0xd3,0xff,0xf9,0xcc,0xff,0xe6,0xbf, 
0xff,0xe3,0xb4,0xff,0xee,0x11,0xff,0xf8,0x4d,0x00,0x04,0xb4,0x00,0x03,0xf8,0x00,0x03,0x9a, 
0xff,0xfb,0x54,0xff,0xf8,0x2c,0xff,0xf7,0x14,0xff,0xf9,0xc1,0xff,0xfb,0xe1,0xff,0xfe,0xf4, 
0xff,0xfb,0xd0,0xff,0xfe,0x31,0xff,0xfb,0xc8,0xff,0xfd,0xab,0xff,0xfb,0xbd,0xff,0xfd,0xb5, 
0xff,0xfd,0x96,0xff,0xfd,0xf5,0xff,0xfd,0xf8,0xff,0xfd,0xd4,0xff,0xfe,0x20,0xff,0xfe,0x6b, 
0xff,0xfe,0x17,0xff,0xfd,0xee,0xff,0xfd,0x07,0xff,0xfe,0x6b,0xff,0xfd,0xfe,0xff,0xfe,0x8e, 
0xff,0xfd,0xcc,0xff,0xfe,0x6e,0xff,0xfd,0xd7,0xff,0xfd,0x8c,0xff,0xfe,0xbc,0xff,0xfe,0xa5, 
0xff,0xfe,0xe7,0xff,0xfe,0x3d,0xff,0xfe,0xae,0xff,0xfd,0xf4,0xff,0xfe,0xf0,0xff,0xfe,0x85, 
0xff,0xfe,0xd4,0xff,0xfe,0xc7,0xff,0xff,0x0e,0xff,0xff,0x0e,0xff,0xff,0x55,0xff,0xff,0x5d, 
0xff,0xff,0x15,0xff,0xff,0x5c,0xff,0xff,0x2a,0xff,0xff,0x32,0xff,0xff,0x8f,0xff,0xff,0x7e, 
0xff,0xff,0xa3,0xff,0xff,0x91,0xff,0xff,0x97,0xff,0xff,0xa7,0xff,0xff,0x9c,0xff,0xff,0x91, 
0xff,0xff,0x88,0xff,0xff,0x8f,0xff,0xff,0xc5,0xff,0xff,0xc5,0xff,0xff,0xac,0xff,0xff,0xcc, 
0xff,0xff,0xd1,0xff,0xff,0xac,0xff,0xff,0xb9,0xff,0xff,0xce,0xff,0xff,0xd7,0xff,0xff,0xea, 
0xff,0xff,0xeb,0xff,0xff,0xd8,0xff,0xff,0xd7,0xff,0xff,0xe2,0xff,0xff,0xf1,0xff,0xff,0xef, 
0xff,0xff,0xef,0xff,0xff,0xf5,0xff,0xff,0xf2,0xff,0xff,0xf7,0xff,0xff,0xf4,0xff,0xff,0xf6, 
0xff,0xff,0xf6,0xff,0xff,0xf8,0xff,0xff,0xfa,0xff,0xff,0xff,0xff,0xff,0xfa,0xff,0xff,0xfe, 
0xff,0xff,0xfd,0x00,0x00,0x00,0xff,0xff,0xfe,0xff,0xff,0xfe,0xff,0xff,0xff,0xff,0xff,0xff, 
0xff,0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1f,0x40,0x00,0x00,0x80, 
0x19,0x99,0x9a,0x4f,0x5c,0x29,0x1b,0x85,0x1f,0x00,0x03,0xe6,0x00,0x03,0xa2,0x04,0x00,0x00, 
0x0a,0x14,0x7b,0x1c,0xc0,0x00,0x00,0x6f,0x69,0x0b,0xe0,0x00,0x0b,0xe0,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x07,0xd0,0x3d,0x06,0xef,0xfb, 
0x32,0x00,0x00,0x06,0x66,0x66,0x07,0xd0,0x3d,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x07,0xeb,0xde,0xff,0xf9,0x75,0xff,0x9d,0x77, 
0x01,0x11,0xa5,0x00,0xaf,0x21,0x00,0xb1,0xaa,0xff,0xd4,0xf5,0xff,0xb9,0x65,0xff,0x72,0xf7, 
0xff,0xc9,0x3d,0xff,0xef,0x7a,0x00,0x2e,0x6e,0x00,0x27,0xdf,0x00,0x1b,0xd3,0xff,0xf9,0xcc, 
0xff,0xe6,0xbf,0xff,0xe3,0xb4,0xff,0xee,0x11,0xff,0xf8,0x4d,0x00,0x04,0xb4,0x00,0x03,0xf8, 
0x00,0x03,0x9a,0xff,0xfb,0x54,0xff,0xf8,0x2c,0xff,0xf7,0x14,0xff,0xf9,0xc1,0xff,0xfb,0xe1, 
0xff,0xfe,0xf4,0xff,0xfb,0xd0,0xff,0xfe,0x31,0xff,0xfb,0xc8,0xff,0xfd,0xab,0xff,0xfb,0xbd, 
0xff,0xfd,0xb5,0xff,0xfd,0x96,0xff,0xfd,0xf5,0xff,0xfd,0xf8,0xff,0xfd,0xd4,0xff,0xfe,0x20, 
0xff,0xfe,0x6b,0xff,0xfe,0x17,0xff,0xfd,0xee,0xff,0xfd,0x07,0xff,0xfe,0x6b,0xff,0xfd,0xfe, 
0xff,0xfe,0x8e,0xff,0xfd,0xcc,0xff,0xfe,0x6e,0xff,0xfd,0xd7,0xff,0xfd,0x8c,0xff,0xfe,0xbc, 
0xff,0xfe,0xa5,0xff,0xfe,0xe7,0xff,0xfe,0x3d,0xff,0xfe,0xae,0xff,0xfd,0xf4,0xff,0xfe,0xf0, 
0xff,0xfe,0x85,0xff,0xfe,0xd4,0xff,0xfe,0xc7,0xff,0xff,0x0e,0xff,0xff,0x0e,0xff,0xff,0x55, 
0xff,0xff,0x5d,0xff,0xff,0x15,0xff,0xff,0x5c,0xff,0xff,0x2a,0xff,0xff,0x32,0xff,0xff,0x8f, 
0xff,0xff,0x7e,0xff,0xff,0xa3,0xff,0xff,0x91,0xff,0xff,0x97,0xff,0xff,0xa7,0xff,0xff,0x9c, 
0xff,0xff,0x91,0xff,0xff,0x88,0xff,0xff,0x8f,0xff,0xff,0xc5,0xff,0xff,0xc5,0xff,0xff,0xac, 
0xff,0xff,0xcc,0xff,0xff,0xd1,0xff,0xff,0xac,0xff,0xff,0xb9,0xff,0xff,0xce,0xff,0xff,0xd7, 
0xff,0xff,0xea,0xff,0xff,0xeb,0xff,0xff,0xd8,0xff,0xff,0xd7,0xff,0xff,0xe2,0xff,0xff,0xf1, 
0xff,0xff,0xef,0xff,0xff,0xef,0xff,0xff,0xf5,0xff,0xff,0xf2,0xff,0xff,0xf7,0xff,0xff,0xf4, 
0xff,0xff,0xf6,0xff,0xff,0xf6,0xff,0xff,0xf8,0xff,0xff,0xfa,0xff,0xff,0xff,0xff,0xff,0xfa, 
0xff,0xff,0xfe,0xff,0xff,0xfd,0x00,0x00,0x00,0xff,0xff,0xfe,0xff,0xff,0xfe,0xff,0xff,0xff, 
0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1f,0x40, 
0x00,0x00,0x80,0x19,0x99,0x9a,0x4f,0x5c,0x29,0x1b,0x85,0x1f,0x00,0x03,0xe6,0x00,0x03,0xa2, 
0x04,0x00,0x00,0x0a,0x14,0x7b,0x1c,0xc0,0x00,0x00,0x6f,0x69,0x0b,0xe0,0x00,0x0b,0xe0,0x00, 
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x07,0xaf,0xae, 
0x06,0xef,0xfb,0x32,0x00,0x000};
#define VSTEP_MODEL_LENGHT 1871
static unsigned char vstep_model[]={
		0x00,0x00,0x03,0x3f,0x33,0x33,0x00,0x00,0x14,0x00,0x00,0x02,0x00,0x4e,0x20,0x00,0x03,0xe8,0x00,0x00,0x00,0x00,0x03,0xe8,0x20,0x00,0x00,0x20,0x00,0x00,0xe3,0x00,0x00,0xff,0xe6,0x66,0x00,0x13,0x88,0x00,0x00,0x01,0x01,0x47,0xae,0x00,0x00,0x41,0x00,0x0b,0xb8,0x00,0x00,0x7d,0x00,0x05,0xdc,0x60,0x00,0x00,0x00,0x27,0x10,0x00,0x66,0x66,0x00,0x00,0x04,0x00,0x00,0x05,0x00,0x00,0x64,0x00,0x13,0x88,0x06,0x40,0x00,0x00,0x01,0x00,0x02,0x80,0x00,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x00,0x03,0x47,0x01,0x00,0x00,0x00,0x03,0xe8,0x00,0x01,0x2c,0x00,0x4e,0x20,0xd8,0x00,0x00,0x9c,0x00,0x00,0xba,0x00,0x00,0x00,0x4e,0x20,0x00,0x00,0x00,0x00,0x00,0x0a,0x00,0x00,0x00,0x00,0x4e,0x20,0x00,0x00,0x00,0x00,0x00,0x0a,0x00,0x00,0x00,0xc4,0x00,0x00,0x50,0x00,0x00,0x00,0xc3,0x50,0x00,0x27,0x10,0xfd,0x3a,0xe1,0x00,0x00,0x00,0x00,0x00,0x00,0x14,0x00,0x00,0x00,0x4e,0x20,0x00,0x00,0x01,0x00,0x0f,0xff,0x00,0x00,0x01,0x12,0x4f,0x80,0x00,0x00,0x0a,0x00,0x00,0x01,0x00,0x00,0x01,0x1e,0x00,0x00,0x00,0x00,0x01,0x00,0xc3,0x50,0x00,0x27,0x10,0x00,0xc3,0x50,0x00,0x27,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x40,0x00,0x32,0x00,0x00,0x00,0x00,0x01,0x26,0x66,0x66,0x00,0x00,0x01,0x0a,0x00,0x00,0x00,0x33,0x33,0x03,0xc0,0x00,0x00,0x03,0xe8,0x00,0xea,0x60,0x00,0x03,0xe8,0x00,0xea,0x60,0x13,0x33,0x33,0x06,0x66,0x66,0x00,0x03,0xe8,0x03,0xc0,0x00,0x0a,0x00,0x00,0x00,0x33,0x33,0x03,0xc0,0x00,0x00,0x03,0xe8,0x00,0xea,0x60,0x00,0x03,0xe8,0x00,0xea,0x60,0x20,0x00,0x00,0x06,0x66,0x66,0x00,0x03,0xe8,0x01,0x40,0x00,0x00,0x00,0x64,0x00,0x00,0x00,0x00,0xc3,0x50,0x00,0x00,0x00,0x00,0x27,0x10,0x00,0x00,0x00,0x00,0x00,0x64,0x00,0x00,0x00,0x00,0x03,0xa2,0x04,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x03,0xa2,0x00,0x00,0x01,0x00,0x00,0xc8,0x00,0x00,0x64,0x00,0x00,0x00,0x00,0x00,0x64,0x00,0xc3,0x50,0x00,0x13,0x88,0x00,0x00,0x00,0x00,0x75,0x30,0x00,0x13,0x88,0x00,0x00,0x00,0x00,0x00,0x00,0x1a,0x00,0x00,0x1a,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0xfe,0x00,0x00,0x04,0x00,0x00,0x00,0x36,0xb0,0x00,0x00,0x00,0x00,0x13,0x88,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x00,0x00,0x64,0x00,0x01,0x2c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x64,0x00,0x00,0x64,0x3f,0x45,0x39,0xb0,0x00,0x00,0x3f,0x33,0x33,0x00,0x00,0x14,0x00,0x00,0x02,0x00,0x4e,0x20,0x00,0x03,0xe8,0x00,0x00,0x00,0x00,0x03,0xe8,0x20,0x00,0x00,0x20,0x00,0x00,0xe3,0x00,0x00,0xff,0xe6,0x66,0x00,0x13,0x88,0x00,0x00,0x01,0x01,0x47,0xae,0x00,0x00,0x41,0x00,0x0b,0xb8,0x00,0x00,0x7d,0x00,0x05,0xdc,0x60,0x00,0x00,0x00,0x27,0x10,0x00,0x66,0x66,0x00,0x00,0x04,0x00,0x00,0x05,0x00,0x00,0x64,0x00,0x13,0x88,0x06,0x40,0x00,0x00,0x01,0x00,0x02,0x80,0x00,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x00,0x03,0x47,0x01,0x00,0x00,0x00,0x03,0xe8,0x00,0x01,0x2c,0x00,0x4e,0x20,0xd8,0x00,0x00,0x9c,0x00,0x00,0xba,0x00,0x00,0x00,0x4e,0x20,0x00,0x00,0x00,0x00,0x00,0x0a,0x00,0x00,0x00,0x00,0x4e,0x20,0x00,0x00,0x00,0x00,0x00,0x0a,0x00,0x00,0x00,0xc4,0x00,0x00,0x50,0x00,0x00,0x00,0xc3,0x50,0x00,0x27,0x10,0xfd,0x3a,0xe1,0x00,0x00,0x00,0x00,0x00,0x00,0x14,0x00,0x00,0x00,0x4e,0x20,0x00,0x00,0x01,0x00,0x0f,0xff,0x00,0x00,0x01,0x12,0x4f,0x80,0x00,0x00,0x0a,0x00,0x00,0x01,0x00,0x00,0x01,0x1e,0x00,0x00,0x00,0x00,0x01,0x00,0xc3,0x50,0x00,0x27,0x10,0x00,0xc3,0x50,0x00,0x27,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x40,0x00,0x32,0x00,0x00,0x00,0x00,0x01,0x26,0x66,0x66,0x00,0x00,0x01,0x0a,0x00,0x00,0x00,0x33,0x33,0x03,0xc0,0x00,0x00,0x03,0xe8,0x00,0xea,0x60,0x00,0x03,0xe8,0x00,0xea,0x60,0x13,0x33,0x33,0x06,0x66,0x66,0x00,0x03,0xe8,0x03,0xc0,0x00,0x0a,0x00,0x00,0x00,0x33,0x33,0x03,0xc0,0x00,0x00,0x03,0xe8,0x00,0xea,0x60,0x00,0x03,0xe8,0x00,0xea,0x60,0x20,0x00,0x00,0x06,0x66,0x66,0x00,0x03,0xe8,0x01,0x40,0x00,0x00,0x00,0x64,0x00,0x00,0x00,0x00,0xc3,0x50,0x00,0x00,0x00,0x00,0x27,0x10,0x00,0x00,0x00,0x00,0x00,0x64,0x00,0x00,0x00,0x00,0x03,0xa2,0x04,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x03,0xa2,0x00,0x00,0x01,0x00,0x00,0xc8,0x00,0x00,0x64,0x00,0x00,0x00,0x00,0x00,0x64,0x00,0xc3,0x50,0x00,0x13,0x88,0x00,0x00,0x00,0x00,0x75,0x30,0x00,0x13,0x88,0x00,0x00,0x00,0x00,0x00,0x00,0x1a,0x00,0x00,0x1a,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0xfe,0x00,0x00,0x04,0x00,0x00,0x00,0x36,0xb0,0x00,0x00,0x00,0x00,0x13,0x88,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x00,0x00,0x64,0x00,0x01,0x2c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x64,0x00,0x00,0x64,0x3f,0x45,0x39,0xb0,0x00,0x00,0x01,0x00,0x00,0xaa,0x00,0x82,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x02,0x00,0x00,0x9a,0x00,0x81,0x07,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x02,0x00,0x00,0x64,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x02,0x00,0x00,0x64,0x00,0x00,0x64,0x00,0x00,0x00,0x00,0x00,0x64,0x01,0x38,0x80,0x00,0x1f,0x40,0x00,0x00,0x00,0x00,0x4e,0x20,0x00,0x07,0xd0,0x00,0x00,0x00,0x00,0x00,0x00,0x12,0x00,0x00,0x12,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x90,0x0e,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0xf4,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x02,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x00,0x64,0x00,0x00,0x00,0x00,0x0b,0xb8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x20,0x0e,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0xf4,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x02,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x00,0x64,0x00,0x00,0x00,0x00,0x0b,0xb8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x40,0x0e,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0xf4,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x02,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x00,0x64,0x00,0x00,0x00,0x00,0x0b,0xb8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0c,0x80,0x0e,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0xf4,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x02,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x00,0x64,0x00,0x00,0x00,0x00,0x0b,0xb8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x19,0x00,0x0e,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0xf4,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x02,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x00,0x64,0x00,0x00,0x00,0x00,0x0b,0xb8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x32,0x00,0x0e,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0xf4,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x02,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x00,0x64,0x00,0x00,0x00,0x00,0x0b,0xb8,0x00,0x00,0x00,0x00,0x00,0x00
};

static unsigned char mbdrc_model[]={
									0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x02,0x00,0x00,0x64,0x00,0x00,0x00, 
									0x00,0x00,0x01,0x00,0x00,0x02,0x00,0x00,0x64,0x00,0x00,0x64,0x00,0x00,0x00,0x00,0x00,0x64,
									0x01,0x38,0x80,0x00,0x1f,0x40,0x00,0x00,0x00,0x00,0x4e,0x20,0x00,0x07,0xd0,0x00,0x00,0x00,
									0x00,0x00,0x00,0x12,0x00,0x00,0x12,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x90,0x0e,0x00,0x00,
									0x02,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
									0x00,0x00,0x00,0x00,0x01,0xf4,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x02,
									0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x00,0x64,0x00,0x00,0x00,0x00,0x0b,0xb8,0x00,0x00,0x00,
									0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x20,0x0e,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x01,
									0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0xf4,
									0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x02,0x00,0x00,0x00,0x04,0x00,0x00,
									0x00,0x00,0x64,0x00,0x00,0x00,0x00,0x0b,0xb8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
									0x00,0x06,0x40,0x0e,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x02,0x00,0x00,0x00,
									0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0xf4,0x00,0x00,0x00,0x00,0x00,0x00,
									0x00,0x00,0x01,0x00,0x00,0x02,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x00,0x64,0x00,0x00,0x00,
									0x00,0x0b,0xb8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0c,0x80,0x0e,0x00,0x00,
									0x02,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
									0x00,0x00,0x00,0x00,0x01,0xf4,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x02,
									0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x00,0x64,0x00,0x00,0x00,0x00,0x0b,0xb8,0x00,0x00,0x00,
									0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x19,0x00,0x0e,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x01,
									0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0xf4,
									0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x02,0x00,0x00,0x00,0x04,0x00,0x00,
									0x00,0x00,0x64,0x00,0x00,0x00,0x00,0x0b,0xb8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
									0x00,0x32,0x00,0x0e,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x02,0x00,0x00,0x00,
									0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0xf4,0x00,0x00,0x00,0x00,0x00,0x00,
									0x00,0x00,0x01,0x00,0x00,0x02,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x00,0x64,0x00,0x00,0x00,
									0x00,0x0b,0xb8,0x00,0x00,0x00 };
#define MBDRC_LENGHT sizeof(mbdrc_model)
static unsigned char dump_model_max1[]={0xff,  0xff,  0x29,  0xff,  0xfe,  0x99,  0xff,  0xfe,  0xeb,  0xff, 
									0xfe,  0x73,  0xff,  0xfe,  0x9e,  0xff,  0xfe,  0xc1,  0xff,  0xff, 
									0x02,  0xff,  0xfe,  0xf7,  0xff,  0xfe,  0x86,  0xff,  0xfe,  0x5a, 
									0xff,  0xfe,  0xfd,  0xff,  0xfe,  0x5d,  0xff,  0xfe,  0xe6,  0xff, 
									0xfe,  0x73,  0xff,  0xfe,  0x64,  0xff,  0xfe,  0x90,  0xff,  0xfe, 
									0x2f,  0xff,  0xfe,  0x35,  0xff,  0xfe,  0x3b,  0xff,  0xfe,  0x2d, 
									0xff,  0xfe,  0x46,  0xff,  0xfe,  0x77,  0xff,  0xfe,  0x5b,  0xff, 
									0xfe,  0x19,  0xff,  0xfe,  0x0d,  0xff,  0xfd,  0xf7,  0xff,  0xfe, 
									0x23,  0xff,  0xfe,  0x20,  0xff,  0xfe,  0x4d,  0xff,  0xfe,  0x3f, 
									0xff,  0xfe,  0x00,  0xff,  0xfd,  0xfe,  0xff,  0xfe,  0x2d,  0xff, 
									0xfe,  0x19,  0xff,  0xfd,  0xa6,  0xff,  0xfd,  0xb6,  0xff,  0xfd, 
									0x5b,  0xff,  0xfe,  0x19,  0xff,  0xfd,  0x56,  0xff,  0xfd,  0xe4, 
									0xff,  0xfd,  0x43,  0xff,  0xfd,  0xc8,  0xff,  0xfd,  0xf7,  0xff, 
									0xfd,  0xb8,  0xff,  0xfd,  0x42,  0xff,  0xfd,  0xbf,  0xff,  0xfd, 
									0x29,  0xff,  0xfd,  0xc4,  0xff,  0xfd,  0x19,  0xff,  0xfd,  0x8c, 
									0xff,  0xfd,  0x2f,  0xff,  0xfd,  0x69,  0xff,  0xfd,  0x2a,  0xff, 
									0xfd,  0xc9,  0xff,  0xfd,  0x00,  0xff,  0xfd,  0x48,  0xff,  0xfd, 
									0x1e,  0xff,  0xfd,  0x64,  0xff,  0xfd,  0x33,  0xff,  0xfd,  0x06, 
									0xff,  0xfc,  0xf9,  0xff,  0xfc,  0xfb,  0xff,  0xfd,  0x1a,  0xff, 
									0xfd,  0x10,  0xff,  0xfd,  0x12,  0xff,  0xfc,  0xd4,  0xff,  0xfd, 
									0x29,  0xff,  0xfc,  0x7a,  0xff,  0xfc,  0xa6,  0xff,  0xfc,  0x3c, 
									0xff,  0xfc,  0x89,  0xff,  0xfc,  0x9d,  0xff,  0xfc,  0x58,  0xff, 
									0xfd,  0x12,  0xff,  0xfc,  0xe1,  0xff,  0xfd,  0x77,  0xff,  0xfc, 
									0x3a,  0xff,  0xfc,  0x9e,  0xff,  0xfb,  0x08,  0xff,  0xfc,  0x50, 
									0xff,  0xfa,  0xb1,  0xff,  0xfd,  0x8a,  0xff,  0xfb,  0xb2,  0x00, 
									0x00,  0x16,  0xff,  0xfc,  0x49,  0x00,  0x00,  0x20,  0xff,  0xf9, 
									0x38,  0xff,  0xfc,  0x24,  0xff,  0xf4,  0xcc,  0xff,  0xfb,  0x1d, 
									0xff,  0xf7,  0x54,  0x00,  0x02,  0xc6,  0xff,  0xff,  0xe6,  0x00, 
									0x08,  0x72,  0xff,  0xfd,  0xcb,  0xff,  0xfd,  0xe9,  0xff,  0xec, 
									0x4d,  0xff,  0xec,  0xf8,  0xff,  0xe7,  0xd0,  0xff,  0xfb,  0x31, 
									0x00,  0x08,  0x35,  0x00,  0x21,  0x94,  0x00,  0x20,  0xed,  0x00, 
									0x18,  0x52,  0xff,  0xed,  0xbc,  0xff,  0xc6,  0xee,  0xff,  0xa2, 
									0x3d,  0xff,  0xb0,  0xcf,  0xff,  0xe1,  0x91,  0x00,  0x46,  0x06, 
									0x00,  0x97,  0xff,  0x00,  0xbf,  0x05,  0x00,  0x7a,  0x37,  0xff, 
									0xe4,  0xc8,  0xff,  0x32,  0x97,  0x07,  0x24,  0xba,  0x00,  0x1c, 
									0x0a,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00, 
									0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00, 
									0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00, 
									0x00,  0x00,  0x00,  0x00};
#define DUMP_MODEL_MAX sizeof(dump_model_max1)

struct live_data_lookup_structure
{
    char *live_data_name_dummy;
    int range_exists;// 1 it exists -1 it doesnot
    double range[2];
    int scale;
    int convert;//-1, 2scomp , lin2dB
    unsigned char default_value[3];
};
typedef struct live_data_lookup_structure live_data_lookup_t;
static live_data_lookup_t live_data_lookup[]={
 	 	 	// name, range exists, range, scale , convert , default values 
			{ "mbdrcAGCenvelope", 1 , {-80,40} ,65536,-1, {0x80,0x00,0x0} },
 	 	 	{ "Battery_Voltage",1,{0,0.5},1,0,{0}},
 	 	 	{ "vAmpClip_P",1,{0,10},524288,-1,{0x0b,0xe0,0x0}},
 	 	 	{ "PreLIMenvelope_P",1,{-80,40},65536,-1,{0xe1,0xad,0xa}},
 	 	 	{ "tMax_P",1,{50,200},16384,-1,{0x00,0x00,0x0}},
 	 	 	{ "tMax_S",1,{50,200},16384,-1,{0x00,0x00,0x0}},
 	 	 	{ "sBias_P",1,{-1,24},65536,1,{0x00,0x00,0x0}},
 	 	 	{ "AGCenvelope_S",1,{-80,40},65536,-1,{0x01,0x00,0x0}},
 	 	 	{ "sparseSigDetMax_S",-1,{0,0},1,-1,{0x00,0x00,0x0}},
 	 	 	{ "fRes_S",1,{0,4000},1,-1,{0x00,0x03,0xa}},
 	 	 	{ "X_S",1,{0,0.5},2097152,-1,{0x00,0x00,0x0}},
 	 	 	{ "X_P",1,{0,0.5},2097152,-1,{0x00,0x07,0xc}},
 	 	 	{ "fRes_P",1,{0,4000},1,-1,{0x00,0x03,0xe}},
 	 	 	{ "sBiasClip_P",1,{-1,24},65536,1,{0}},
 	 	 	{ "damageFres_S",-1,{0,0},1,-1,{0}},
 	 	 	{ "sBiasClip_S",1,{-1,24},65536,1,{0}},
 	 	 	{ "Xout_S",1,{0,0.5},2097152,-1,{0x0a,0x14,0x7}},
 	 	 	{ "freqBalance_S",-1,{0,0},1,-1,{0}},
 	 	 	{ "freqBalance_P",-1,{0,0},1,-1,{0}},
 	 	 	{ "Xout_P",1,{0,0.5},2097152,-1,{0x00,0x00,0x0}},
 	 	 	{ "AGCgain_S",1,{-10,40},65536,1,{0xe1,0xad,0xa}},
 	 	 	{ "AGCgain_P",1,{-10,40},65536,1,{0x01,0x00,0x0}},
 	 	 	{ "AGCenvelope_P",1,{-80,40},65536,-1,{0xe1,0xad,0xa}},
 	 	 	{ "sBias_S",1,{-1,24},65536,1,{0x01,0x00,0x0}},
 	 	 	{ "ampClip_S",-1,{0,0},1,-1,{0x00,0x00,0x0}},
 	 	 	{ "IC_temperature",1,{0,0.5},1,0,{0}},
 	 	 	{ "tChip_P",1,{20,206},16384,-1,{0x00,0x00,0x0}},
 	 	 	{ "xMax_P",1,{0.1,1},2097152,-1,{0x1a,0x32,0x4}},
 	 	 	{ "xMax_S",1,{0.1,1},2097152,-1,{0x0a,0x14,0x7}},
 	 	 	{ "tChip_S",1,{20,206},16384,-1,{0}},
 	 	 	{ "tClipCnt_P",1,{0,4095},1,-1,{0}},
 	 	 	{ "tClipCnt_S",1,{0,4095},1,-1,{0}},
 	 	 	{ "mbdrcAGCgain",1,{-10,40},65536,1,{0x01,0xfe,0xc}},
 	 	 	{ "damageFres_P",-1,{0,0},1,-1,{0}},
 	 	 	{ "damageT_P",-1,{0,0},1,-1,{0}},
 	 	 	{ "damageT_S",-1,{0,0},1,-1,{0}},
 	 	 	{ "LIMenvelope_P",1,{-80,40},65536,-1,{0x01,0x00,0x0}},
 	 	 	{ "sideChainX_S",1,{-40,24},65536,1,{0x00,0x00,0x0}},
 	 	 	{ "sparseSigDetMax_P",-1,{0,0},1,-1,{0x00,0x00,0x0}},
 	 	 	{ "activityRun_S",1,{-10,2},1,-1,{0x80,0x00,0x0}},
 	 	 	{ "damageReZ_P",-1,{0,0},1,-1,{0}},
 	 	 	{ "damageReZ_S",-1,{0,0},1,-1,{0}},
 	 	 	{ "sparseSigDetMin_S",-1,{0,0},1,-1,{0x00,0x00,0x0}},
 	 	 	{ "sparseSigDetMin_P",-1,{0,0},1,-1,{0x00,0x00,0x0}},
 	 	 	{ "ampClip_P",-1,{0,0},1,-1,{0x06,0x40,0x0}},
 	 	 	{ "sideChainS_P",1,{-40,24},65536,1,{0x00,0x00,0x0}},
 	 	 	{ "sideChainS_S",1,{-40,24},65536,1,{0x00,0x00,0x0}},
 	 	 	{ "PreLIMgain_S",1,{-24,10},65536,1,{0x01,0x69,0x9}},
 	 	 	{ "sideChainX_P",1,{-40,24},65536,1,{0x00,0x07,0xc}},
 	 	 	{ "PreLIMgain_P",1,{-24,10},65536,1,{0x01,0x69,0x9}},
 	 	 	{ "activityRun_P",1,{-10,2},1,-1,{0x80,0x00,0x0}},
 	 	 	{ "LIMenvelope_S",1,{-80,40},65536,-1,{0xe4,0xad,0xc}},
 	 	 	{ "LIMgain_S",1,{-24,10},65536,1,{0xe4,0xad,0xc}},
 	 	 	{ "Xtrace_S",1,{0,0.5},2097152,-1,{0}},
 	 	 	{ "Xtrace_P",1,{0,0.5},2097152,-1,{0}},
 	 	 	{ "LIMgain_P",1,{-24,10},65536,1,{0x01,0x00,0x0}},
 	 	 	{ "sBiasProt_P",1,{-1,24},65536,1,{0}},
 	 	 	{ "sBiasProt_S",1,{-1,24},65536,1,{0}},
 	 	 	{ "sBiasProtT_P",1,{-1,24},65536,1,{0}},
 	 	 	{ "sBiasProtSS_P",1,{-1,24},65536,1,{0}},
 	 	 	{ "sBiasProtSS_S",1,{-1,24},65536,1,{0}},
 	 	 	{ "sBiasProtT_S",1,{-1,24},65536,1,{0}},
 	 	 	{ "fastRecovery_P",-1,{0,0},1,-1,{0}},
 	 	 	{ "fastRecovery_S",-1,{0,0},1,-1,{0}},
 	 	 	{ "vAmpClip_S",1,{0,10},524288,-1,{0}},
 	 	 	{ "PreLIMenvelope_S",1,{-80,40},65536,-1,{0xe1,0xad,0xa}},
 	 	 	{ "T_S",1,{-10,100},16384,-1,{0x00,0x07,0xc}},
 	 	 	{ "T_P",1,{-10,100},16384,-1,{0x01,0x00,0x0}},
 	 	 	{ "activity_P",-1,{0,0},1,-1,{0}},
 	 	 	{ "activity_S",-1,{0,0},1,-1,{0}},
 };
/******************************************************************************
 * HAL interface called via Scribo registered functions
 */
int lxDummyWriteRead(int fd, int NrOfWriteBytes, const uint8_t *WriteData,
		     int NrOfReadBytes, uint8_t *ReadData, uint32_t *pError)
{
	int length;
        /* Remove unreferenced parameter warning C4100 */
	(void)fd;

	*pError = NXP_I2C_Ok;
	/* there's always a write */
	length = i2cWrite(NrOfWriteBytes, WriteData);
	/* and maybe a read */
	if ((NrOfReadBytes != 0) && (length != 0)) {
		length = i2cRead(NrOfReadBytes, ReadData);
	}
	if (length == 0) {
		PRINT_ERROR("lxDummy slave error\n");
		*pError = NXP_I2C_NoAck;
	}

	return length;

/* if (WriteData[0] !=  (tfa98xxI2cSlave<<1)) { */
/* PRINT("wrong slave 0x%02x iso 0x%02x\n", WriteData[0]>>1, tfa98xxI2cSlave); */
/* //      return 0; */
/* } */
}

int lxDummyWrite(int fd, int size, unsigned char *buffer, uint32_t *pError)
{
	return lxDummyWriteRead(fd, size, buffer, 0, NULL, pError);
}

/*
 * set calibration done
 */
static void set_caldone()
{
	int xm_caldone=0;

	switch(dev2fam(&dev[thisdev])) {
	case 1:
		xm_caldone = TFA1_FW_XMEM_CALIBRATION_DONE;
		break;
	case 2:
		xm_caldone = TFA2_FW_XMEM_CALIBRATION_DONE;
		break;
	default:
		PRINT_ERROR("wrong family selector!");
		break;
	}

	dev[thisdev].xmem[xm_caldone * 3 +2] = 1;	/* calibration done */
	dev[thisdev].xmem[xm_caldone * 3 +1] = 0;	/* calibration done */
	dev[thisdev].xmem[xm_caldone * 3 ] = 0;	/* calibration done */
	//dev[thisdev].Reg[TFA1_STATUSREG] &= ~(TFA98XX_STATUS_FLAGS0_ACS);	/* clear coldstart */
	TFA_SET_BF(ACS,0);/* clear coldstart */
	dev[thisdev].Reg[0x83] = 0x0710;	/* dummy re0 value */
}
/*
 * increment count boot
 */

static void inc_countboot()
{
	int xm_count_boot = 0;
	unsigned int val;

	switch(dev2fam(&dev[thisdev])) {
	case 1:
		xm_count_boot = TFA1_FW_XMEM_COUNT_BOOT;
		break;
	case 2:
		xm_count_boot = TFA2_FW_XMEM_COUNT_BOOT;
		break;
	default:
		PRINT_ERROR("wrong family selector!");
		break;
	}

	// reverse bytes
	val =  (dev[thisdev].xmem[(xm_count_boot*3)]&0xff <<16 ) |
					(dev[thisdev].xmem[(xm_count_boot*3)+1]&0xff <<8)|
					(dev[thisdev].xmem[(xm_count_boot*3)+2]&0xff);
	val++;
	dev[thisdev].xmem[(xm_count_boot*3)] = (uint8_t)(val>>16);// ((val>>16)&0xff) | (val&0x00ff00) |((val&0xff)<<16);
	dev[thisdev].xmem[(xm_count_boot*3)+1] = (uint8_t)(val>>8);
    dev[thisdev].xmem[(xm_count_boot*3)+2] = (uint8_t)val;
}

void  lxDummyVerbose(int level) {
	lxDummy_verbose = level;
	DUMMYVERBOSE PRINT("dummy verbose on\n");
}

#ifdef TFA_MUL
static int pfw_main(void *regs);

static int pfw_main(void *regs)
{
 return NULL;
}
#endif
/*
 * the input lxDummyArg is from the -d<lxDummyArg> global
 * the file is the last argument  option
 */
int lxDummyInit(char *file)
{
	int type, i=0;
	char *lxDummyArg;		/* extra command line arg for test settings */
	char *parser;
	char *devices[4];
	void * dlib_handle;
	//char tempchar[8];
	dev[0].slave = 0x34;
	dev[1].slave = 0x35;
	dev[2].slave = 0x36; //default
	dev[3].slave = 0x37;

	(void)devices;
	(void)parser;
	(void)emulmodel;
	(void)dlib_handle;
	//devices = (char *)malloc(MAX_DUMMIES*sizeof(char));
	if (file) {
		int j=0;
		(void)j;
		lxDummyArg = strchr(file, ','); /* the extra arg is after the comma */

#ifdef TFA_MUL
		parser = strtok(file,",");//strips off tfamul
		parser = strtok(NULL,",");//device ids
		lxDummyArg = strchr(file, ','); /* the extra arg is after the comma */
		while(parser[i]!='\0')
		{
			//uint8_t ccounter=0;
			char *temp_char = (parser+i);//=NULL;
			if(parser[i]!='_')
			{
				emulmodel[j] = (struct emulated_device*)malloc(sizeof(struct emulated_device));
				emulmodel[j]->ymem = (unsigned char *)malloc((CF_YMEM_LENGTH * 3)*sizeof(unsigned char));
				emulmodel[j]->pmem = (unsigned char *)malloc((CF_PATCHMEM_LENGTH * 4)*sizeof(unsigned char));
				emulmodel[j]->iomem = (unsigned char *)malloc((CF_IO_PATCH_LENGTH * 3)*sizeof(unsigned char));

				emulmodel[j]->filters = (unsigned char *)malloc(TFA2_FILTERCOEFSPARAMETER_LENGTH*sizeof(unsigned char));
				emulmodel[j]->lastConfig = (unsigned char *)malloc(TFA2_ALGOPARAMETER_LENGTH*sizeof(unsigned char));
				emulmodel[j]->lastSpeaker = (unsigned char *)malloc(TFA2_SPEAKERPARAMETER_LENGTH*sizeof(unsigned char));
				emulmodel[j]->lastPreset = (unsigned char *)malloc(TFA98XX_PRESET_LENGTH*sizeof(unsigned char));

				devices[j] = malloc( 2 * sizeof(char) );
				strncpy(devices[j], temp_char, 2);
				i+=2;
				j++;
			}
			else
				i++;
		}
		parser = strtok(NULL,",");//checks if there is library
		if(parser==NULL)
			printf("library not provided\n");
		else
			printf("library %s provided\n",parser);

		dlib_handle = dlopen ("libfw_tfa9888n1b.so", RTLD_LAZY);
		if (!dlib_handle) {
			printf("No fw library provided\n");
		else
			printf("fw library loaded\n");

		}
		/*for (parser = strtok(file,","); parser != NULL; parser = strtok(NULL, ","))
		{
			if(strcmp(parser,"_"))
				devices[i] = parser;
			i++;
		}*/

		if(pthread_create(&dsp_thread, NULL, pfw_main, 	NULL)) {
			fprintf(stderr, "Error creating thread\n");
			return 1;
		}
#endif
		if (lxDummyArg) {
			lxDummyArg++; /* skip the comma */ //TODO parse commalist
			if (strcmp(lxDummyArg, "warm") == 0)
				dummy_warm = 1;
			else if (strcmp(lxDummyArg, "noclk") == 0)
				dummy_noclock = 1;
			else if (!setInputFile(lxDummyArg)) /* if filename use it */
			{
				lxDummyFailTest = atoi(lxDummyArg);
			}
		}
		DUMMYVERBOSE
			PRINT("arg: %s\n", lxDummyArg);
	} else {
		PRINT("%s: called with NULL arg\n", __FUNCTION__);
		return 1;
	}

	for(thisdev=0;thisdev<MAX_DUMMIES;thisdev++ ){
		dev[thisdev].xmem_patch_version = TFA_XMEM_PATCHVERSION; /* all except 97 */
		if (sscanf(file, "dummy%x", &type)) {
			switch (type) {
			case 0x90:
				dev[thisdev].type = tfa9890;
				dev[thisdev].config_length = 165;
				break;
			case 0x90b:
				//PRINT("warning: 91 is tfa9890b; 92 is tfa9891\n");
				dev[thisdev].type = tfa9890b;
				dev[thisdev].config_length = 165;
				break;
			case 0x92:
			case 0x91:
				dev[thisdev].type = tfa9891;
				dev[thisdev].config_length = 201;
				dev[thisdev].xmem_patch_version = TFA9891_XMEM_PATCHVERSION;
				break;
			case 0x95:
				dev[thisdev].type = tfa9895;
				dev[thisdev].config_length = 201;
				break;
			case 0x97:
				dev[thisdev].type = tfa9897;
				dev[thisdev].config_length = 201;
				dev[thisdev].xmem_patch_version = TFA9897_XMEM_PATCHVERSION;
				break;
			case 0x72:
				dev[thisdev].type = tfa9872;
				dev[thisdev].config_length = 201; //TODO error for 72 dsp stuff
				// todo: fix calling of this function; braks windows build, stalled tiberius release v4.0.0 creation
#if !( defined(WIN32) || defined(_X64) ) // posix/linux
				tfa_ext_register(NULL, lxDummy_write_message, lxDummy_read_message, NULL);
				lxDummy_device.if_type |= NXP_I2C_Msg;
				lxDummy_device.tfadsp_execute = lxDummy_execute_message;
#endif
				break;
			case 0x88:
			default:
				dev[thisdev].type = tfa9888;
				dev[thisdev].config_length = 165;
				//!TODO filters is now dynamically allocated
				break;
			}
		}

		resetRegs(dev[thisdev].type);
		setRomid(dev[thisdev].type);
	}
	/*open file for live data and store the string in variable str*/
	Fd = fopen("live_data.txt","r");
	if(NULL != Fd)
	{
		fgets(str_live_data,1250, Fd);
		fclose(Fd);
	}
	for(i=0;i<MAX_DUMMIES;i++)
	{
		//dev[i].Reg = (uint16_t *)malloc(256*sizeof(uint16_t));
		//dev[i].xmem = (unsigned char *)malloc((10250)*sizeof(unsigned char));
		dev[i].ymem = (unsigned char *)malloc((CF_YMEM_LENGTH * 3)*sizeof(unsigned char));
		dev[i].pmem = (unsigned char *)malloc((CF_PATCHMEM_LENGTH * 4)*sizeof(unsigned char));
		dev[i].iomem = (unsigned char *)malloc((CF_IO_PATCH_LENGTH * 3)*sizeof(unsigned char));

		dev[i].filters = (unsigned char *)malloc(TFA2_FILTERCOEFSPARAMETER_LENGTH*sizeof(unsigned char));
		dev[i].lastConfig = (unsigned char *)malloc(TFA2_ALGOPARAMETER_LENGTH*sizeof(unsigned char));
		dev[i].lastSpeaker = (unsigned char *)malloc(TFA2_SPEAKERPARAMETER_LENGTH*sizeof(unsigned char));
		dev[i].lastPreset = (unsigned char *)malloc(TFA98XX_PRESET_LENGTH*sizeof(unsigned char));
	}

	thisdev = 2; //default
	PRINT("%s: running DUMMY i2c, type=%s\n", __FUNCTION__,
	       typeNames[dev[thisdev].type]);

	/* lsmodel default */
	memcpy((void *)dev[thisdev].lastSpeaker, lsmodel, TFA2_SPEAKERPARAMETER_LENGTH*sizeof(unsigned char));

	return (int)dev[thisdev].type;
}

static int lxDummyVersion(char *buffer, int fd)
{
	int len = (int)strlen(dummy_version_string);
        //PRINT("dummy version: %s\n", DUMMY_VERSION);
	/* Remove unreferenced parameter warning C4100 */
	(void)fd;
	strcpy(buffer,dummy_version_string);

	return len;
}

static int lxDummyGetPin(int fd, int pin)
{
	/* Remove unreferenced parameter warning C4100 */
	(void)fd;

    if ((unsigned int)pin >= sizeof(dummy_pins)/sizeof(int)) {
    	return -1;
	}

	return dummy_pins[pin];

}

static int lxDummySetPin(int fd, int pin, int value)
{
	/* Remove unreferenced parameter warning C4100 */
	(void)fd;

	if ((unsigned int)pin >= sizeof(dummy_pins)/sizeof(int)) {
		return NXP_I2C_UnsupportedValue;
	}
	PRINT("dummy: pin[%d]=%d\n", pin, value);

	dummy_pins[pin] = value;

	return NXP_I2C_Ok;
}

static int lxDummyClose(int fd)
{

	int i=0;
	/* Remove unreferenced parameter warning C4100 */
	(void)fd;
	//	PRINT("Function close not implemented for target dummy.");
	globalState=1;
	for(i=0;i<MAX_DUMMIES;i++)
	{
		free(dev[i].ymem);
		free(dev[i].pmem);
		free(dev[i].iomem);
		free(dev[i].filters);
		free(dev[i].lastConfig);
		free(dev[i].lastSpeaker);
		free(dev[i].lastPreset);
	}
	return 0;
}
static int buffersize(void)
{

	return NXP_I2C_MAX_SIZE;
}
static int startplayback(char *buffer, int fd)
{
	(void)buffer;
	(void)fd;
	PRINT("%s(\"%s\") not supported in %s\n", __FUNCTION__, buffer, __FILE__);
	return -NXP_I2C_NoInterfaceFound;
}
static int stopplayback(int fd)
{
	(void)fd;
	PRINT("%s() not supported in %s\n", __FUNCTION__, __FILE__);
	return -NXP_I2C_NoInterfaceFound;
}

struct nxp_i2c_device lxDummy_device = {
	lxDummyInit,
	lxDummyWriteRead,
	lxDummyVersion,
	lxDummySetPin,
	lxDummyGetPin,
	lxDummyClose,
	buffersize,
	startplayback,
	stopplayback,
	NXP_I2C_Direct | NXP_I2C_Playback, //NXP_I2C_Playback is a stub for testing
	NULL
};
const struct nxp_i2c_device *nxp_plug = &lxDummy_device;
/******************************************************************************
 * I2C bus/slave handling code
 */
static int lxDummySetSlaveIdx(uint8_t slave) {
	int i;
	for(i=0;i<MAX_DUMMIES;i++){
		if (dev[i].slave == slave) {
			thisdev=i;
			return i;
		}
	}
	return -1;
}
/*
 * read I2C
 */
static int i2cRead(int length, uint8_t *data)
{
	int idx;

	if (lxDummySetSlaveIdx(data[0] / 2) <0) {
		PRINT_ERROR("dummy: slave read NoAck\n");
		return 0;
	}
	DUMMYVERBOSE PRINT("dummy: slave[%d]=0x%02x\n", thisdev, dev[thisdev].slave);

/* ln =length - 1;  // without slaveaddress */
	idx = 1;
	while (idx < length) {
		idx += tfaRead(&data[idx]);	/* write current and return bytes consumed */
	};

	return length;
}

/*
 * write i2c
 */
static int i2cWrite(int length, const uint8_t *data)
{
	//uint8_t slave;		/*  */
	int idx;

	if (lxDummySetSlaveIdx(data[0] / 2) <0) {
		PRINT_ERROR("dummy: slave read NoAck\n");
		return 0;
	}
	DUMMYVERBOSE PRINT("slave[%d]=0x%02x\n", thisdev, dev[thisdev].slave);

	dev[thisdev].currentreg = data[1];	/* start subaddress */

	/* without slaveaddress and regaddr */
	idx = 2;
	while (idx < length) {
		idx += tfaWrite(&data[idx]);	/* write current and return bytes consumed */
	};

	return length;

}

/******************************************************************************
 * TFA I2C registers read/write
 */
/* reg73 */
/* cf_err[7:0]     8   [7 ..0] 0           cf error flags */
/* reg73 cf_ack[7:0]     8   [15..8] 0           acknowledge of requests (8 channels")" */

#define 	CTL_CF_RST_DSP	(0)
#define 	CTL_CF_DMEM	(1)
#define 	CTL_CF_AIF		(3)
#define    	CTL_CF_INT		(4)
#define    	CTL_CF_REQ		(5)
#define    	STAT_CF_ERR		(0)
#define    	STAT_CF_ACK		(8)


/*
 * in the local register cache the values are stored as little endian,
 *  all processing is done in natural little endianess
 * The i2c data is big endian
 */

/*
 *  i2c regs reset to default 9897
 */
static void resetRegs9897(void)
{
        /* i used in unsigned int comparison in for loops. */
	unsigned int i; 

	dev[thisdev].Reg[0x00] = 0x081d;	/* statusreg */
	dev[thisdev].Reg[0x03] = 0x0b97;	/* revisionnumber */
	dev[thisdev].Reg[0x04] = 0x888b;	/* i2sreg */
	dev[thisdev].Reg[0x05] = 0x13aa;	/* bat_prot */
	dev[thisdev].Reg[0x06] = 0x001f;	/* audio_ctr */
	dev[thisdev].Reg[0x07] = 0x0fe6;	/* dcdcboost */
	dev[thisdev].Reg[0x08] = 0x0800;	/* spkr_calibration */
	dev[thisdev].Reg[0x09] = 0x041d;	/* sys_ctrl */
	dev[thisdev].Reg[0x0a] = 0x3ec3;	/* i2s_sel_reg */
	dev[thisdev].Reg[0x40] = 0x0000;	/* hide_unhide_key */
	dev[thisdev].Reg[0x41] = 0x0000;	/* pwm_control */
	dev[thisdev].Reg[0x4c] = 0x0000;	/* abisttest */
	dev[thisdev].Reg[0x62] = 0x0000;
	dev[thisdev].Reg[0x70] = 0x0000;	/* cf_controls */
	dev[thisdev].Reg[0x71] = 0x0000;	/* cf_mad */
	dev[thisdev].Reg[0x72] = 0x0000;	/* cf_mem */
	dev[thisdev].Reg[0x73] = 0x0000;	/* cf_status */
	dev[thisdev].Reg[0x80] = 0x0000;
	dev[thisdev].Reg[0x83] = 0x0000;	/* mtp_re0 */
	for(i=0;i<(sizeof(tdm_regdefs_mx1)/sizeof(regdef_t))-1;i++) {//TFA98XX_TDM_CONFIG_REG0=0x10
		dev[thisdev].Reg[i+FAM_TFA98XX_TDM_REGS] = tdm_regdefs_mx1[i].pwronDefault;
	}

	for(i=0;i<(sizeof(int_regdefs_mx1)/sizeof(regdef_t))-1;i++) {
		dev[thisdev].Reg[int_regdefs_mx1[i].offset] = int_regdefs_mx1[i].pwronDefault;
	}

}
/*
 *  i2c regs reset to default 9890
 */
static void resetRegs9890(void)
{
	dev[thisdev].Reg[0x00] = 0x0a5d;	/* statusreg */
	dev[thisdev].Reg[0x03] = 0x0080;	/* revisionnumber */
	dev[thisdev].Reg[0x04] = 0x888b;	/* i2sreg */
	dev[thisdev].Reg[0x05] = 0x93a2;	/* bat_prot */
	dev[thisdev].Reg[0x06] = 0x001f;	/* audio_ctr */
	dev[thisdev].Reg[0x07] = 0x8fe6;	/* dcdcboost */
	dev[thisdev].Reg[0x08] = 0x3800;	/* spkr_calibration */
	dev[thisdev].Reg[0x09] = 0x825d;	/* sys_ctrl */
	dev[thisdev].Reg[0x0a] = 0x3ec3;	/* i2s_sel_reg */
	dev[thisdev].Reg[0x40] = 0x0000;	/* hide_unhide_key */
	dev[thisdev].Reg[0x41] = 0x0308;	/* pwm_control */
	dev[thisdev].Reg[0x4c] = 0x0000;	/* abisttest */
	dev[thisdev].Reg[0x62] = 0x0000;	/* mtp_copy */
	dev[thisdev].Reg[0x70] = 0x0000;	/* cf_controls */
	dev[thisdev].Reg[0x71] = 0x0000;	/* cf_mad */
	dev[thisdev].Reg[0x72] = 0x0000;	/* cf_mem */
	dev[thisdev].Reg[0x73] = 0x0000;	/* cf_status */
	dev[thisdev].Reg[0x80] = 0x0000;	/* mtp */
	dev[thisdev].Reg[0x83] = 0x0000;	/* mtp_re0 */
	dev[thisdev].Reg[0x84] = 0x1234;	/* MTP for '90 startup system stable detection */
	dev[thisdev].Reg[0xff] = 0x0000;	/* MTP for '90 startup system stable detection */
}

/*
 *  i2c regs reset to default 9890B/9891
 */
static void resetRegs9890b(void)
{
	dev[thisdev].Reg[0x00] = 0x0a5d;	/* statusreg */
	dev[thisdev].Reg[0x03] = 0x0091;	/* revisionnumber */
	dev[thisdev].Reg[0x04] = 0x888b;	/* i2sreg */
	dev[thisdev].Reg[0x05] = 0x93a2;	/* bat_prot */
	dev[thisdev].Reg[0x06] = 0x001f;	/* audio_ctr */
	dev[thisdev].Reg[0x07] = 0x8fe6;	/* dcdcboost */
	dev[thisdev].Reg[0x08] = 0x3800;	/* spkr_calibration */
	dev[thisdev].Reg[0x09] = 0x825d;	/* sys_ctrl */
	dev[thisdev].Reg[0x0a] = 0x3ec3;	/* i2s_sel_reg */
	dev[thisdev].Reg[0x40] = 0x0000;	/* hide_unhide_key */
	dev[thisdev].Reg[0x41] = 0x0308;	/* pwm_control */
	dev[thisdev].Reg[0x4c] = 0x0000;	/* abisttest */
	dev[thisdev].Reg[0x62] = 0x0000;	/* mtp_copy */
	dev[thisdev].Reg[0x70] = 0x0000;	/* cf_controls */
	dev[thisdev].Reg[0x71] = 0x0000;	/* cf_mad */
	dev[thisdev].Reg[0x72] = 0x0000;	/* cf_mem */
	dev[thisdev].Reg[0x73] = 0x0000;	/* cf_status */
	dev[thisdev].Reg[0x80] = 0x0000;	/* mtp */
	dev[thisdev].Reg[0x83] = 0x0000;	/* mtp_re0 */
	dev[thisdev].Reg[0x84] = 0x1234;	/* MTP for '90 startup system stable detection */
}
/*
 *  i2c regs reset to default 9891
 */
static void resetRegs9891(void)
{
	dev[thisdev].Reg[0x00] = 0x0a5d;	/* statusreg */
	dev[thisdev].Reg[0x03] = 0x0092;	/* revisionnumber */
	dev[thisdev].Reg[0x04] = 0x888b;	/* i2sreg */
	dev[thisdev].Reg[0x05] = 0x93a2;	/* bat_prot */
	dev[thisdev].Reg[0x06] = 0x000f;	/* audio_ctr */
	dev[thisdev].Reg[0x07] = 0x8fff;	/* dcdcboost */
	dev[thisdev].Reg[0x08] = 0x3800;	/* spkr_calibration */
	dev[thisdev].Reg[0x09] = 0x825d;	/* sys_ctrl */
	dev[thisdev].Reg[0x0a] = 0x3ec3;	/* i2s_sel_reg */
	dev[thisdev].Reg[0x0f] = 0x40;	/* irq_reg */

}
/*
 * i2c regs reset to default 9895
 */
static void resetRegs9895(void)
{
	dev[thisdev].Reg[0x00] = 0x081d;	/* statusreg */
	dev[thisdev].Reg[0x01] = 0x3ff;	/* battV clock off */
	dev[thisdev].Reg[0x02] = 0x100;	/* ictemp clock off */
	dev[thisdev].Reg[0x03] = 0x0012;	/* revisionnumber */
	dev[thisdev].Reg[0x04] = 0x888b;	/* i2sreg */
	dev[thisdev].Reg[0x05] = 0x13aa;	/* bat_prot */
	dev[thisdev].Reg[0x06] = 0x001f;	/* audio_ctr */
	dev[thisdev].Reg[0x07] = 0x0fe6;	/* dcdcboost */
	dev[thisdev].Reg[0x08] = 0x0c00;	/* spkr_calibration */
	dev[thisdev].Reg[0x09] = 0x041d;	/* sys_ctrl */
	dev[thisdev].Reg[0x0a] = 0x3ec3;	/* i2s_sel_reg */
	dev[thisdev].Reg[0x40] = 0x0000;	/* hide_unhide_key */
	dev[thisdev].Reg[0x41] = 0x0300;	/* pwm_control */
	dev[thisdev].Reg[0x4c] = 0x0000;	/* abisttest */
	dev[thisdev].Reg[0x62] = 0x5be1;
	dev[thisdev].Reg[0x70] = 0;		/* cf_controls */
	dev[thisdev].Reg[0x71] = 0;		/* cf_mad */
	dev[thisdev].Reg[0x72] = 0x0000;	/* cf_mem */
	dev[thisdev].Reg[0x73] = 0x0000;	/* cf_status */
	dev[thisdev].Reg[0x80] = 0x0000;
	dev[thisdev].Reg[0x83] = 0x0000;	/* mtp_re0 */
}

/*
 * i2c regs reset tfa9872 to default, copy from 88
 */
static void resetRegs9872(void)
{
	/* i used in unsigned int comparison in for loops. */
	unsigned int i;

	if (dummy_warm) { //TODO reflect warm state if needed
		dev[thisdev].Reg[TFA98XX_SYS_CONTROL0] = TFA98XX_SYS_CONTROL0_POR;
		dev[thisdev].Reg[TFA98XX_SYS_CONTROL1] = TFA98XX_SYS_CONTROL1_POR;
		dev[thisdev].Reg[TFA98XX_SYS_CONTROL2] = TFA98XX_SYS_CONTROL2_POR;
		//dev[thisdev].Reg[TFA98XX_STATUS_POLARITY_REG1] = 0x27a0;
	} else {
		for(i=0;i<(sizeof(regdefs_72)/sizeof(regdef_t))-1;i++) {
			dev[thisdev].Reg[regdefs_72[i].offset] = regdefs_72[i].pwronDefault;
		}
	}
	if (dummy_noclock)
		TFA_SET_BF(NOCLK,1);
}

/*
 * i2c regs reset 88N1A12 to default
 */
static void resetRegs9888(void)
{
	/* i used in unsigned int comparison in for loops. */
	unsigned int i; 

	if (dummy_warm) { //TODO reflect warm state if needed
		dev[thisdev].Reg[TFA98XX_SYS_CONTROL0] = TFA98XX_SYS_CONTROL0_POR;
		dev[thisdev].Reg[TFA98XX_SYS_CONTROL1] = TFA98XX_SYS_CONTROL1_POR;
		dev[thisdev].Reg[TFA98XX_SYS_CONTROL2] = TFA98XX_SYS_CONTROL2_POR;
		dev[thisdev].Reg[TFA98XX_DEVICE_REVISION]=  TFA98XX_DEVICE_REVISION_POR;
		dev[thisdev].Reg[TFA98XX_STATUS_FLAGS0]=  0x21c; //N1A12
		//dev[thisdev].Reg[TFA98XX_STATUS_FLAGS0] = 0x22be;
		//dev[thisdev].Reg[TFA98XX_STATUS_POLARITY_REG1] = 0x27a0;
	} else {
		for(i=0;i<(sizeof(regdefs)/sizeof(regdef_t))-1;i++) {//TFA98XX_TDM_CONFIG_REG0=0x10
			dev[thisdev].Reg[regdefs[i].offset] = regdefs[i].pwronDefault;
		}
	}
	if (dummy_noclock)
		TFA_SET_BF(NOCLK,1);
}

/*
 * i2c regs reset to default
 */
static void resetRegs(dummyType_t type)
{
	uint16_t acs=0;
	/* this is a I2CR ot POR */

	if (dummy_warm) {
		/* preserve acs state */
		acs = (uint16_t)TFA_GET_BF(ACS);
	}
	TFA_SET_BF(BATS,0x3ff);//when clock is off
	if(globalState==0)
	{
		switch (type) {
		case tfa9890:
			resetRegs9890();
			break;
		case tfa9890b:
			resetRegs9890b();
			break;
		case tfa9891:
			resetRegs9891();
			break;
		case tfa9895:
			resetRegs9895();
			break;
		case tfa9897:
			resetRegs9897();
			break;
		case tfa9872:
			resetRegs9872();
			break;
		case tfa9888:
			resetRegs9888();
			break;
		default:
			PRINT("dummy: %s, unknown type %d\n", __FUNCTION__, type);
			break;
		}
	}
	if (dummy_warm) {
		TFA_SET_BF(ACS,acs);
		if(IS_FAM==1)
			dev[thisdev].Reg[0] = 0x805f;
		//dev[thisdev].Reg[TFA98XX_STATUS_POLARITY_REG1] = 0x27a0;
		else
			dev[thisdev].Reg[0x10] = 0x743e;
	}
}
/*
 * return the regname
 */
static const char *getRegname(int reg)
{
	int i, nr_of_elements = sizeof(regdefs)/sizeof(regdefs[0]);

	for(i = 0; i < nr_of_elements; i++) {
		if(reg == regdefs[i].offset)
			return regdefs[i].name;
	}
	return "unknown";
}

/*
 * emulation of register read
 */
static int tfaRead(uint8_t *data) {
	int reglen = 2; /* default */
	uint8_t reg = dev[thisdev].currentreg;
	uint16_t regval = 0xdead;
	static short temperature = 0;

	if (IS_FAM == 2) {
		switch (reg) {
		case TFA98XX_CF_STATUS /*0x93 */:
			regval = dev[thisdev].Reg[reg]; /* just return */
			reglen = 2;
			dev[thisdev].currentreg++; /* autoinc */
			break;
		case TFA98XX_CF_MEM /*0x92 */:
			if (isClockOn(thisdev))
				reglen = lxDummyMemr(
						(dev[thisdev].Reg[TFA98XX_CF_CONTROLS] >> CTL_CF_DMEM)
								& 0x03, data);
			/*setting the mbdrc bit*/
			dev[thisdev].Reg[TFA98XX_CF_CONTROLS] |= 1 << 8; /*  */
			regval = dev[thisdev].Reg[reg];
			//reglen = 6;
			break;
		case TFA98XX_CF_MAD /*0x71 */:
			regval = dev[thisdev].Reg[reg]; /* just return */
			if (lxDummyFailTest == 2)
				regval = 0xdead; /* fail test */
			reglen = 2;
			dev[thisdev].currentreg++; /* autoinc */
			break;
		case TFA98XX_CF_CONTROLS:
			//regval = dev[thisdev].Reg[reg]; /* just return anyway */
			/*setting the mbdrc bit*/
			dev[thisdev].Reg[TFA98XX_CF_CONTROLS] |= 1 << 8; /*  */
			regval = dev[thisdev].Reg[reg];
			dev[thisdev].currentreg++; /* autoinc */
			//reglen = 6;
			break;
		case TFA98XX_KEY2_PROTECTED_MTP0:
			if(TFA_GET_BF(MTPOTC)==1)
			{
				dev[thisdev].Reg[TFA98XX_KEY2_PROTECTED_MTP0] |= (1 << 1); /*  */
				//dev[thisdev].Reg[TFA98XX_KEY2_PROTECTED_MTP0] |= (1 << 0);
				dev[thisdev].Reg[TFA98XX_STATUS_FLAGS0] &= ~(1 << 9);
/*
				dev[thisdev].Reg[TFA98XX_KEY2_PROTECTED_MTP0] &= ~(1 << 1);
				dev[thisdev].Reg[TFA98XX_KEY2_PROTECTED_MTP0] &= ~(1 << 0);
*/
			}
			regval = dev[thisdev].Reg[reg];
			dev[thisdev].currentreg++; /* autoinc */
			//reglen = 6;
			break;
		case TFA98XX_KEY1_PROTECTED_MTP4:
			dev[thisdev].Reg[TFA98XX_KEY1_PROTECTED_MTP4] = 7450;
			regval = dev[thisdev].Reg[reg];
			break;
		case TFA98XX_KEY1_PROTECTED_MTP5:
			dev[thisdev].Reg[TFA98XX_KEY1_PROTECTED_MTP5] = 7450;
			regval = dev[thisdev].Reg[reg];
			break;

		default:
			regval = dev[thisdev].Reg[reg]; /* just return anyway */
			dev[thisdev].currentreg++; /* autoinc */
			break;
		}
		if (reg != 0x92 /* TFA98XX_CF_MEM*/) {
			DUMMYVERBOSE
				PRINT("0x%02x:0x%04x (%s)\n", reg, regval, getRegname(reg));

			*(uint16_t *) (data) = BE2LEW(regval); /* return in proper endian */
		}

	} else { //max1
		switch (reg) {
		default:
			regval = dev[thisdev].Reg[reg]; /* just return */
			reglen = 2;
			dev[thisdev].currentreg++; /* autoinc */
			break;
		case 0xff:
			/*bitfield not in the list*/
			regval = dev[thisdev].Reg[reg]; /* just return */
			reglen = 2;
			dev[thisdev].currentreg++; /* autoinc */
			break;
		case 0x84: /* MTP for '90 startup system stable detection */
			regval = dev[thisdev].Reg[reg]; /* just return */
			reglen = 2;
			dev[thisdev].currentreg++; /* autoinc */
			break;
		case 0x86: /* MTP */
#define FEATURE1_TCOEF              0x100	/* bit8 set means tCoefA expected */
#define FEATURE1_DRC                0x200	/* bit9 NOT set means DRC expected */
			regval = dev[thisdev].Reg[reg]; /* just return */
			reglen = 2;
			dev[thisdev].currentreg++; /* autoinc */
			break;
		case 2: //TFA98XX_TEMPERATURE /*0x02 */:
			if (temperature++ > 170)
				temperature = -40;
			dev[thisdev].Reg[TFA98XX_TEMPERATURE] = temperature;
			regval = dev[thisdev].Reg[reg]; /* just return */
			reglen = 2;
			dev[thisdev].currentreg++; /* autoinc */
			break;
		case 1: // TFA98XX_BATTERYVOLTAGE /*0x01 */:
			if (lxDummyFailTest == 10)
				dev[thisdev].Reg[1] = (uint16_t) (1 / (5.5 / 1024)); /* 1V */
			regval = dev[thisdev].Reg[reg]; /* just return */
			reglen = 2;
			dev[thisdev].currentreg++; /* autoinc */
			break;
		case TFA1_CF_STATUS /*0x73 */:
			regval = dev[thisdev].Reg[reg]; /* just return */
			reglen = 2;
			dev[thisdev].currentreg++; /* autoinc */
			break;
		case TFA1_CF_MEM /*0x72 */:
			if (isClockOn(thisdev))
				reglen = lxDummyMemr(
						(dev[thisdev].Reg[0x70] >> CTL_CF_DMEM) & 0x03, data);
			else
				PRINT("dummy: ! DSP mem read without clock!\n");
			break;
		case 0x71: //TFA98XX_CF_MAD /*0x71 */:
			regval = dev[thisdev].Reg[reg]; /* just return */
			if (lxDummyFailTest == 2)
				regval = 0xdead; /* fail test */
			reglen = 2;
			dev[thisdev].currentreg++; /* autoinc */
			break;

		}

		if (reg != 0x72) {
			DUMMYVERBOSE
				PRINT("0x%02x:0x%04x (%s)\n", reg, regval, getRegname(reg));

			*(uint16_t *) (data) = BE2LEW(regval); /* return in proper endian */
		}

	}

	return reglen;
}
/*
 * set dsp firmware ack
 */
static void cfAck(int thisdev, enum tfa_fw_event evt) {
	uint16_t oldack=dev[thisdev].Reg[TFA98XX_CF_STATUS];

	FUNC_TRACE("ack\n");
	dev[thisdev].Reg[FAM_TFA98XX_CF_STATUS] |= (1 << (evt + STAT_CF_ACK));

	if ( oldack != dev[thisdev].Reg[FAM_TFA98XX_CF_STATUS])
		updateInterrupt();
}

static void cfCountboot(int thisdev) {
	if (isClockOn(thisdev)) {
		inc_countboot();		// count_boot (lsb)
		cfAck(thisdev, tfa_fw_reset_start);
	}
}
/*
 * cf control reg (0x70)
 */
static int i2cCfControlReg(uint16_t val)
{
	unsigned char module_id, param_id;
	uint16_t xor, clearack, negedge, newval, oldval = dev[thisdev].Reg[TFA98XX_CF_CONTROLS];
	int ack=1;
	newval = val;

	/* REQ/ACK bits:
	 *  if REQ was high and goes low then clear ACK
	 * */
	xor = oldval^newval;
	if ( xor & TFA_GET_BF(REQ) ) {
		negedge = (oldval & ~newval);
		clearack = negedge;
		dev[thisdev].Reg[FAM_TFA98XX_CF_STATUS] &= ~(clearack & TFA98XX_CF_CONTROLS_REQ_MSK);
	}
	// reset transition 1->0 increment count_boot
	if ( dev[thisdev].Reg[FAM_TFA98XX_CF_CONTROLS] & TFA98XX_CF_CONTROLS_RST_MSK ) {// reset is on
			if ( (val  & TFA98XX_CF_CONTROLS_RST_MSK) ==0) { //clear it now
				cfCountboot(thisdev);
				FUNC_TRACE("DSP reset release\n");
			}
			else return val; // stay in reset
}
	if ( (val & TFA98XX_CF_CONTROLS_CFINT_MSK) &&
		 (val & TFA98XX_CF_CONTROLS_REQ_MSK) )	/* if cfirq and msg req*/
	{
		FUNC_TRACE("req: ");
		module_id = dev[thisdev].xmem[4];
		param_id = dev[thisdev].xmem[5];
		if ((module_id == MODULE_SPEAKERBOOST+0x80) ) {	/* MODULE_SPEAKERBOOST */
			FUNC_TRACE("MODULE_SPEAKERBOOST\n");
			setgetDspParamsSpeakerboost(param_id);
		} else if (module_id == MODULE_FRAMEWORK+0x80) {	/* MODULE_FRAMEWORK */
			FUNC_TRACE("MODULE_FRAMEWORK\n");
			setDspParamsFrameWork(param_id);
		} else if (module_id == MODULE_BIQUADFILTERBANK+0x80) { // EQ
			FUNC_TRACE("MODULE_BIQUADFILTERBANK\n");
			setDspParamsEq(param_id);
		} else if (module_id == MODULE_SETRE+0x80) { // set Re0
			FUNC_TRACE("MODULE_SETRE\n");
			setDspParamsRe0(param_id);
		} else  {
			PRINT("? UNKNOWN MODULE ! module_id: 0x%02x param_id:0x%02x\n",module_id, param_id);
			ack=0;
			_exit(Tfa98xx_Error_Bad_Parameter);
		}
		if(ack)
			cfAck(thisdev, tfa_fw_i2c_cmd_ack);
	}

	return val;
}
static int updateInterrupt(void) {
return 0;
}

/*
 * i2c interrupt registers
 *   abs addr: TFA98XX_INTERRUPT_OUT_REG1 + idx
 */
static uint16_t i2cInterruptReg(int idx, uint16_t wordvalue)
{
	DUMMYVERBOSE PRINT("0x%02x,intreg[%d] < 0x%04x\n", TFA98XX_INTERRUPT_OUT_REG1+idx, idx, wordvalue);
	dev[thisdev].Reg[TFA98XX_INTERRUPT_OUT_REG1+idx] = wordvalue;

	return wordvalue;
}
/*
 * i2c  control reg
 */
static uint16_t tfa1_i2cControlReg(uint16_t wordvalue)
{
	uint16_t isreset=0;

	if ((wordvalue & TFA98XX_SYS_CTRL_I2CR)) {
		FUNC_TRACE("I2CR reset\n");
		resetRegs(dev[thisdev].type);
		wordvalue = dev[thisdev].Reg[TFA98XX_SYS_CTRL]; //reset value
		isreset = 1;
	}
	/* if PLL input changed */
	if ((wordvalue ^ dev[thisdev].Reg[TFA98XX_SYS_CTRL])
			& TFA98XX_SYS_CTRL_IPLL_MSK) {
		int ipll = (wordvalue & TFA98XX_SYS_CTRL_IPLL_MSK)
				>> TFA98XX_SYS_CTRL_IPLL_POS;
		FUNC_TRACE("PLL in=%d (%s)\n", ipll, ipll ? "fs" : "bck");
	}

	if ((wordvalue & (1 << 0)) && !(wordvalue & (1 << 13))) { /* powerdown=1, i2s input 1 */
		FUNC_TRACE("power off\n");
		dev[thisdev].Reg[TFA98XX_STATUSREG] &= ~(TFA98XX_STATUSREG_PLLS
				| TFA98XX_STATUSREG_CLKS);
		TFA_SET_BF(BATS, 0);
		dev[thisdev].Reg[TFA98XX_STATUSREG] |= (TFA98XX_STATUSREG_CLKS);
	} else {
		FUNC_TRACE("power on\n");
		dev[thisdev].Reg[TFA98XX_STATUSREG] |= (TFA98XX_STATUSREG_PLLS
				| TFA98XX_STATUSREG_CLKS);
		dev[thisdev].Reg[TFA98XX_STATUSREG] |= (TFA98XX_STATUSREG_AREFS);
		//dev[thisdev].Reg[TFA98XX_BATTERYVOLTAGE] = ; //=5.08V R*5.5/1024
		TFA_SET_BF(BATS, 0x3b2); //=5.08V R*5.5/1024
	}
	if (wordvalue & TFA98XX_SYS_CTRL_SBSL) { /* configured */
		FUNC_TRACE("configured\n");
		dev[thisdev].Reg[TFA98XX_STATUSREG] |= (TFA98XX_STATUSREG_AMPS);
		set_caldone();
	}
	if ((wordvalue & TFA98XX_SYS_CTRL_AMPE)
			&& !(wordvalue & TFA98XX_SYS_CTRL_CFE)) { /* assume bypass if AMPE and not CFE */
		FUNC_TRACE("CF by-pass\n");
		dev[thisdev].Reg[0x73] = 0x00ff; // else irq test wil fail
	}

	/* reset with ACS set */
	if (TFA_GET_BF(ACS)) {
		FUNC_TRACE("reset with ACS set\n");
		dev[thisdev].xmem[231 * 3] = 0; /* clear calibration done */
		dev[thisdev].xmem[231 * 3 + 1] = 0; /* clear calibration done */
		dev[thisdev].xmem[231 * 3 + 2] = 0; /* clear calibration done */
	}

	return isreset;
}

/*
 * i2c  control reg
 */
#define MANSTATE_TRACE if (lxDummy_manstate_verbose&1) printf
static uint16_t tfa2_i2cControlReg(uint16_t wordvalue)
{
	uint16_t isreset=0;
	if((wordvalue & TFA98XX_SYS_CONTROL0_SBSL_MSK)){
		FUNC_TRACE("Coolflux Configured\n");
		/*setting SBSL makes bit AMPS and MANOPER as HIGH too*/
		dev[thisdev].Reg[TFA98XX_STATUS_FLAGS0] |= 1 << 12;
		dev[thisdev].Reg[TFA98XX_STATUS_FLAGS3] |= 1 << 15;
		// making mantstate as operating
		dev[thisdev].Reg[TFA98XX_STATUS_FLAGS4] |= 0x0048;
		// value when device starts in operating mode
		dev[thisdev].Reg[0x10] = 0x743e;
		MANSTATE_TRACE("State 5\n");

	}
	/*setting SBSL makes bit AMPS and MANOPER as HIGH too*/
	dev[thisdev].Reg[TFA98XX_STATUS_FLAGS0] |= 1<<12;
	dev[thisdev].Reg[TFA98XX_STATUS_FLAGS3] |= 1<<15;
	if ( (wordvalue & TFA98XX_SYS_CONTROL0_I2CR_MSK) ) {
		FUNC_TRACE("I2CR reset\n");
		resetRegs(dev[thisdev].type);
		wordvalue = dev[thisdev].Reg[TFA98XX_SYS_CONTROL0]; //reset value
		isreset=1;
		MANSTATE_TRACE("\nState 2\n");
	}
	if (wordvalue & TFA98XX_SYS_CONTROL0_PWDN_MSK)
	{
		//show only if changed
		if( (dev[thisdev].Reg[TFA98XX_SYS_CONTROL0] &  TFA98XX_SYS_CONTROL0_PWDN_MSK) == 0)
		{
			FUNC_TRACE("PLL power off\n");
			MANSTATE_TRACE("State 1\n");
		}
		dev[thisdev].Reg[TFA98XX_STATUS_FLAGS0] &=
		    ~(TFA98XX_STATUS_FLAGS0_PLLS | TFA98XX_STATUS_FLAGS0_CLKS);
		dev[thisdev].Reg[TFA98XX_STATUS_FLAGS0] |= (TFA98XX_STATUS_FLAGS0_AREFS);
		dev[thisdev].Reg[TFA98XX_BATTERY_VOLTAGE] = 0x3b2; //=5.08V R*5.5/1024
	} else if ( (dev[thisdev].Reg[TFA98XX_SYS_CONTROL0] &  TFA98XX_SYS_CONTROL0_PWDN_MSK) ) {//show only if changed
			FUNC_TRACE("PLL power on\n");
			dev[thisdev].Reg[TFA98XX_STATUS_FLAGS0] |=
			    (TFA98XX_STATUS_FLAGS0_PLLS | TFA98XX_STATUS_FLAGS0_CLKS);
			MANSTATE_TRACE("State 3\n");
	}
	if (wordvalue & TFA98XX_SYS_CONTROL0_SBSL_MSK) {	/* configured */
		if((dev[thisdev].Reg[TFA98XX_SYS_CONTROL0] &  TFA98XX_SYS_CONTROL0_SBSL_MSK) == 0 )
		{
			FUNC_TRACE("configured\n");
			MANSTATE_TRACE("State 4\n");
			// making mantstate as operating
			dev[thisdev].Reg[TFA98XX_STATUS_FLAGS4] |= 0x0048;
			// value when device starts in operating mode
			dev[thisdev].Reg[0x10] = 0x743e;
		}
		set_caldone();
	}
	if ( (wordvalue & TFA98XX_SYS_CONTROL0_AMPC) && !(wordvalue & TFA98XX_SYS_CONTROL0_CFE)) {	/* assume bypass if AMPE and not CFE */
		FUNC_TRACE("CF by-pass\n");
		MANSTATE_TRACE("state 6\n");
		//dev[thisdev].Reg[0x73] = 0x00ff;// else irq test wil fail
	}
	if((wordvalue&TFA98XX_SYS_CONTROL0_PWDN)==0){
		// when powered up AREFS abd clks has to be active
		dev[thisdev].Reg[TFA98XX_STATUS_FLAGS0] |= 1<<13;
		dev[thisdev].Reg[TFA98XX_STATUS_FLAGS0] |= 1<<5;
	}
	return isreset;
}
static uint16_t i2cControlReg(uint16_t wordvalue) {
	if (IS_FAM==2)
		return tfa2_i2cControlReg(wordvalue);
	else if (IS_FAM==1)
		return tfa1_i2cControlReg(wordvalue);

	PRINT_ERROR("family type is wrong:%d\n", IS_FAM);
	return 0;
}
/*
 * emulation of register write
 *
 *  write current register , autoincrement  and return bytes consumed
 *
 */

static int tfaWrite(const uint8_t *data) {
	int reglen = 2; /* default */
	uint8_t reg = dev[thisdev].currentreg;
	uint16_t wordvalue, oldvalue, newvalue,regval=0;

	oldvalue = dev[thisdev].Reg[reg];
	newvalue = wordvalue = data[0] << 8 | data[1];

	(void)regval;

	if (IS_FAM == 2) {
		switch (reg) {
		case TFA98XX_SYS_CONTROL0:
			if (i2cControlReg(wordvalue)) {

				// i2c reset, fix printing wrong trace
				dev[thisdev].Reg[TFA98XX_SYS_CONTROL0] = wordvalue = newvalue =
						oldvalue;

						resetRegs(dev[thisdev].type);

				newvalue = wordvalue | TFA98XX_SYS_CONTROL0_I2CR_MSK; /* set i2cr  bit for proper trace */
			} else
				dev[thisdev].Reg[TFA98XX_SYS_CONTROL0] = wordvalue;
			break;
		case TFA98XX_CF_MAD /* */:
			dev[thisdev].Reg[reg] = wordvalue;
			dev[thisdev].memIdx = wordvalue * 3; /* set glbl mem idx */
			if(wordvalue==521)//then its fw version query
			{
				dev[thisdev].xmem[dev[thisdev].memIdx]=2;
				dev[thisdev].xmem[dev[thisdev].memIdx+1]=0x2b;
				dev[thisdev].xmem[dev[thisdev].memIdx+2]=0x40;
			}
			break;
		case TFA98XX_CF_CONTROLS /* */:
			dev[thisdev].Reg[reg] = (uint16_t) i2cCfControlReg(wordvalue);
			/*setting the mbdrc bit*/
			dev[thisdev].Reg[TFA98XX_CF_CONTROLS] |= 1 << 8; /*  */
			regval = dev[thisdev].Reg[reg];
			break;
		case TFA98XX_CF_MEM /* */:
			reglen = lxDummyMemw(
					(dev[thisdev].Reg[TFA98XX_CF_CONTROLS] >> CTL_CF_DMEM)
							& 0x03, data);
			//i2cCfControlReg(wordvalue);
			/*setting the mbdrc bit*/
			dev[thisdev].Reg[TFA98XX_CF_CONTROLS] |= 1 << 8; /*  */
			regval = dev[thisdev].Reg[reg];
			reglen = 3;
			break;
		case TFA98XX_KEY2_PROTECTED_MTP0:
			DUMMYVERBOSE PRINT("TFA98XX_KEY2_PROTECTED_MTP0\n");
			dev[thisdev].Reg[TFA98XX_KEY2_PROTECTED_MTP0] |= (1 << 0);
			regval = dev[thisdev].Reg[reg];
			//reglen = 6;
			break;
		default:
			if (reg >> 4 == 2) {
				wordvalue = i2cInterruptReg(reg & 0x0f, wordvalue);
			} else {
				DUMMYVERBOSE
					PRINT("dummy: undefined wr register: 0x%02x\n", reg);
			}
			dev[thisdev].Reg[reg] = wordvalue;
			break;
		}
		/* all but cf_mem autoinc and 2 bytes */
		if (reg != 0x92 /*TFA98XX_CF_MEM*/) {
			dev[thisdev].currentreg++; /* autoinc */
			reglen = 2;

			DUMMYVERBOSE
				PRINT("0x%02x<0x%04x (%s)\n", reg, wordvalue, getRegname(reg));
	#ifdef TFA_DUMMY_BFFIELD_TRACE
			lxDummy_trace_bitfields(reg, oldvalue, newvalue);
	#endif
		} else {
			; //TODO add dots with proper newline
//			if (lxDummy_verbose & 2)
//					PRINT(".");
		}
	} else { //max1
		switch (reg) {
		case 0x00:
			if (i2cControlReg(wordvalue)) {

				// i2c reset, fix printing wrong trace
				dev[thisdev].Reg[TFA98XX_SYS_CONTROL0] = wordvalue = newvalue =
						oldvalue;

						resetRegs(dev[thisdev].type);

				newvalue = wordvalue | TFA98XX_SYS_CONTROL0_I2CR_MSK; /* set i2cr  bit for proper trace */
			} else
				dev[thisdev].Reg[TFA98XX_SYS_CONTROL0] = wordvalue;
			break;

		case 0x06: //TFA1_AUDIO_CTR /*0x06 */:
			/* if volume rate changed */
			if ((wordvalue ^ dev[thisdev].Reg[reg]) & TFA98XX_AUDIO_CTR_VOL_MSK) {
				int vol = (wordvalue & TFA98XX_AUDIO_CTR_VOL_MSK)
						>> TFA98XX_AUDIO_CTR_VOL_POS;
				FUNC_TRACE("volume=%d %s\n", vol,
						vol == 0xff ? "(softmute)" : "");
				if (vol == 0xff)
					cfAck(thisdev, tfa_fw_soft_mute_ready);
			}
			dev[thisdev].Reg[reg] = wordvalue;
			break;
		case 4 /*I2SREG 0x04 or Audio control reg for 97 */:
			/* if sample rate changed */
			if ((wordvalue ^ dev[thisdev].Reg[TFA98XX_I2SREG])
					& TFA98XX_I2SREG_I2SSR_MSK) {
				int fs = (wordvalue & TFA98XX_I2SREG_I2SSR_MSK)
						>> TFA98XX_I2SREG_I2SSR_POS;
				FUNC_TRACE("sample rate=%d (%s)\n", fs, fsname[8 - fs]);
			}
			dev[thisdev].Reg[reg] = wordvalue;
			break;
		case 0x08: //TFA98XX_SPKR_CALIBRATION /*0x08 PVP bit */:
			dev[thisdev].Reg[reg] = wordvalue | 0x0400; /* PVP bit is always 1 */
			break;
		case 0x71: //TFA98XX_CF_MAD /*0x71 */:
			dev[thisdev].Reg[reg] = wordvalue;
			dev[thisdev].memIdx = wordvalue * 3; /* set glbl mem idx */
			break;
		case 0x70: //TFA98XX_CF_CONTROLS /*0x70 */:
			dev[thisdev].Reg[reg] = (uint16_t) i2cCfControlReg(wordvalue);
			break;
		case 0x72: //TFA98XX_CF_MEM /*0x72 */:
			reglen = lxDummyMemw((dev[thisdev].Reg[0x70] >> CTL_CF_DMEM) & 0x03,
					data);
			break;
		case 0x09: //TFA98XX_SYS_CTRL /*0x09 */:
			if (i2cControlReg(wordvalue)) {
				// i2c reset, fix printing wrong trace
				newvalue = wordvalue | TFA98XX_SYS_CONTROL0_I2CR_MSK; /* set i2cr  bit for proper trace */
			} else
				dev[thisdev].Reg[reg] = wordvalue;
			break;
		default:
			if (reg >> 4 == 2) {
				wordvalue = i2cInterruptReg(reg & 0x0f, wordvalue);
			} else {
				DUMMYVERBOSE
					PRINT("dummy: undefined wr register: 0x%02x\n", reg);
			}
			dev[thisdev].Reg[reg] = wordvalue;
			break;
		}
		/* all but cf_mem autoinc and 2 bytes */
		if (reg != 0x72) {
			dev[thisdev].currentreg++; /* autoinc */
			reglen = 2;

			DUMMYVERBOSE
				PRINT("0x%02x<0x%04x (%s)\n", reg, wordvalue, getRegname(reg));
	#ifdef TFA_DUMMY_BFFIELD_TRACE
			lxDummy_trace_bitfields(reg, oldvalue, newvalue);
	#endif
		} else {
			; //TODO add dots with proper newline
//			if (lxDummy_verbose & 2)
//					PRINT(".");
		}
	}


	updateInterrupt();
	return reglen;
}

/******************************************************************************
 * CoolFlux subsystem, mainly dev[thisdev].xmem, read and write
 */

/*
 * set value returned for the patch load romid check
 */
static void setRomid(dummyType_t type)
{
	if (dummy_warm) {
		set_caldone();
	}
	switch (type) {
	case tfa9888:
		dev[thisdev].xmem[0x200c * 3] = 0x5c;	/* N1A12 0x200c=0x5c551e*/
		dev[thisdev].xmem[0x200c * 3 + 1] = 0x55;
		dev[thisdev].xmem[0x200c * 3 + 2] = 0x1e;
		break;
	case tfa9890:
	case tfa9890b:
		dev[thisdev].xmem[0x20c6 * 3] = 0x00;	/* 90 N1C3 */
		dev[thisdev].xmem[0x20c6 * 3 + 1] = 0x00;
		dev[thisdev].xmem[0x20c6 * 3 + 2] = 0x31; // C2=0x31;
		break;
	case tfa9891:
		dev[thisdev].xmem[0x20f0 * 3] = 0x00;	/* 91 */
		dev[thisdev].xmem[0x20f0 * 3 + 1] = 0x00;
		dev[thisdev].xmem[0x20f0 * 3 + 2] = 0x37; //
		break;
	case tfa9895:
		dev[thisdev].xmem[0x21b4 * 3] = 0x00;	/* 95 */
		dev[thisdev].xmem[0x21b4 * 3 + 1] = 0x77;
		dev[thisdev].xmem[0x21b4 * 3 + 2] = 0x9a;
		break;
	case tfa9897:
		/* N1B: 0x22b0=0x000032*/
		dev[thisdev].xmem[0x22B0 * 3] = 0x00;	/* 97N1B */
		dev[thisdev].xmem[0x22B0 * 3 + 1] = 0x00;
		dev[thisdev].xmem[0x22B0 * 3 + 2] = 0x32;
		//n1a
//		dev[thisdev].xmem[0x2286 * 3] = 0x00;	/* 97N1A */
//		dev[thisdev].xmem[0x2286 * 3 + 1] = 0x00;
//		dev[thisdev].xmem[0x2286 * 3 + 2] = 0x33;
		break;
	case tfa9872:
		break;
	default:
		PRINT("dummy: %s, unknown type %d\n", __FUNCTION__, type);
		break;
	}
}

/* modules */
#define MODULE_SPEAKERBOOST  1



static char *cfmemName[] = { "dev[thisdev].pmem", "dev[thisdev].xmem", "dev[thisdev].ymem", "dev[thisdev].iomem" };

/* True if PLL is on */
static int isClockOn(int thisdev) {
	return (TFA_GET_BF(CLKS)); /* clks should be enough */
}

/*
 * write to CF memory space
 */
static int lxDummyMemw(int type, const uint8_t *data)
{
	uint8_t *memptr=(uint8_t *)data;
	int idx = dev[thisdev].memIdx;

	switch (type) {
	case Tfa98xx_DMEM_PMEM:
		/* dev[thisdev].pmem is 4 bytes */
		idx = (dev[thisdev].memIdx - (dev[thisdev].Reg[TFA98XX_CF_CONTROLS] * 3)) / 4;	/* this is the offset */
		idx += CF_PATCHMEM_START;
		DUMMYVERBOSE
		    PRINT("W %s[%02d]: 0x%02x 0x%02x 0x%02x 0x%02x\n",
			   cfmemName[type], idx, data[0], data[1], data[2],
			   data[3]);
		if ((CF_PATCHMEM_START <= idx)
&&(idx < (CF_PATCHMEM_START + CF_PATCHMEM_LENGTH))) {
			memptr = &dev[thisdev].pmem[(idx - CF_PATCHMEM_START) * 4];
			dev[thisdev].memIdx += 4;
			*memptr++ = *data++;
			*memptr++ = *data++;
			*memptr++ = *data++;
			*memptr++ = *data++;
			return 4;

		} else {
			//PRINT("dummy: dev[thisdev].pmem[%d] write is illegal!\n", idx);
			;//return 0;
		}

		break;
	case Tfa98xx_DMEM_XMEM:
		memptr = &dev[thisdev].xmem[idx];
		if(idx/3 ==dev[thisdev].xmem_patch_version)
			FUNC_TRACE("Patch version=%d.%d.%d\n", data[0], data[1], data[2]);
		break;
	case Tfa98xx_DMEM_YMEM:
		memptr = &dev[thisdev].ymem[idx];
		break;
	case Tfa98xx_DMEM_IOMEM:
		/* address is in TFA98XX_CF_MAD */
		if (dev[thisdev].Reg[FAM_TFA98XX_CF_MADD] == 0x8100) {	/* CF_CONTROL reg */
			if (data[2] & 1) {	/* set ACS */
//				dev[thisdev].Reg[TFA1_STATUSREG] |=
//				    (TFA98XX_STATUS_FLAGS0_ACS);
				TFA_SET_BF(ACS,1);
				dev[thisdev].memIdx += 3;
				return 3;	/* go back, writing is done */
			} else { 		/* clear ACS */
				//dev[thisdev].Reg[TFA1_STATUSREG] &=  ~(TFA98XX_STATUS_FLAGS0_ACS);
				TFA_SET_BF(ACS,0);
				dev[thisdev].memIdx += 3;
				return 3;	/* go back, writing is done */			}
		} else if ((CF_IO_PATCH_START <= dev[thisdev].Reg[FAM_TFA98XX_CF_MADD]) &&
			   (dev[thisdev].Reg[FAM_TFA98XX_CF_MADD] <
			    (CF_IO_PATCH_START + CF_IO_PATCH_LENGTH))) {
			memptr = &dev[thisdev].iomem[idx];
		} else {
			/* skip other io's */
			return 3;
		}
		memptr = &dev[thisdev].iomem[idx];
		break;
	}
	DUMMYVERBOSE
	    PRINT("W %s[%02d]: 0x%02x 0x%02x 0x%02x\n", cfmemName[type],
		   idx / 3, data[0], data[1], data[2]);
	*memptr++ = *data++;
	*memptr++ = *data++;
	*memptr++ = *data++;
	dev[thisdev].memIdx += 3;
	return 3;		/* TODO 3 */
}

/*
 * read from CF memory space
 */
static int lxDummyMemr(int type, uint8_t *data)
{
	uint8_t *memptr = data;
	int idx = dev[thisdev].memIdx;


	switch (type) {
	case Tfa98xx_DMEM_PMEM:
		memptr = &dev[thisdev].pmem[idx];
		break;
	case Tfa98xx_DMEM_XMEM:
		memptr = &dev[thisdev].xmem[idx];
		break;
	case Tfa98xx_DMEM_YMEM:
		memptr = &dev[thisdev].ymem[idx];
		break;
	case Tfa98xx_DMEM_IOMEM:
		if (IS_FAM == 2) {
			memptr = &dev[thisdev].iomem[idx];
		}
		else {
			int index=idx-0x18300;
			memptr = &dev[thisdev].iomem[index];
		}
		break;
	}
	DUMMYVERBOSE
	    PRINT("R %s[%02d]: 0x%02x 0x%02x 0x%02x\n", cfmemName[type],
		   idx / 3, memptr[0], memptr[1], memptr[2]);

    *data++ = *memptr++;
	*data++ = *memptr++;
	*data++ = *memptr++;

	dev[thisdev].memIdx += 3;

   	return 3;		/* TODO 3 */
}

/******************************************************************************
 * DSP RPC interaction response
 */

#define XMEM_INDEX 30
/*
 * fill dev[thisdev].xmem Speakerboost module RPC buffer with the return values
 */
static int setgetDspParamsSpeakerboost(int param)
{
	unsigned int i = 0;
	/* memory address to be accessed (0 : Status, 1 : ID, 2 : parameters) */

	switch (param) {
	case SB_PARAM_SET_ALGO_PARAMS:
		dev[thisdev].xmem[0] = 0;
		dev[thisdev].xmem[1] = 0;
		dev[thisdev].xmem[2] = 0;
		memcpy( dev[thisdev].lastConfig, (void *)&dev[thisdev].xmem[6], TFA2_ALGOPARAMETER_LENGTH*sizeof(unsigned char)); //copies a bit too much for tfa1
		FUNC_TRACE("loaded algo\n");
		break;
	case SB_PARAM_SET_CONFIG:
		dev[thisdev].xmem[0] = 0;
		dev[thisdev].xmem[1] = 0;
		dev[thisdev].xmem[2] = 0;
		memcpy(dev[thisdev].lastConfig, (void *)&dev[thisdev].xmem[6],TFA2_ALGOPARAMETER_LENGTH*sizeof(unsigned char));
		FUNC_TRACE("loaded config\n");
		break;
	case SB_PARAM_SET_PRESET:
		dev[thisdev].xmem[0] = 0;
		dev[thisdev].xmem[1] = 0;
		dev[thisdev].xmem[2] = 0;
		memcpy(dev[thisdev].lastPreset, &dev[thisdev].xmem[6], TFA98XX_PRESET_LENGTH*sizeof(unsigned char));
//		if (lxDummyFailTest == 8)
//			lastPreset[1] = ~lastPreset[1];
		FUNC_TRACE("loaded preset\n");
		break;
	case SB_PARAM_SET_LSMODEL:
		dev[thisdev].xmem[0] = 0;
		dev[thisdev].xmem[1] = 0;
		dev[thisdev].xmem[2] = 0;
		memcpy(dev[thisdev].lastSpeaker, &dev[thisdev].xmem[6],
				TFA2_SPEAKERPARAMETER_LENGTH*sizeof(unsigned char));
		FUNC_TRACE("loaded speaker\n");
		break;
	case SB_PARAM_SET_LAGW:
		FUNC_TRACE("SB_LAGW_ID\n");
		dev[thisdev].xmem[0] = 0;
		dev[thisdev].xmem[1] = 0;
		dev[thisdev].xmem[2] = 0;
		break;
	case SB_PARAM_SET_LSMODEL | 0x80:	/* for now just return the speakermodel */
		FUNC_TRACE("GetLsModel\n");
		dev[thisdev].xmem[0] = 0;
		dev[thisdev].xmem[1] = 0;
		dev[thisdev].xmem[2] = 0;
		//memcpy(&dev[thisdev].xmem[6], lsmodelw, sizeof(Tfa98xx_SpeakerParameters_t)); //Original
		/*FIX_DUMMY_90*/
		if (IS_FAM == 2)
			memcpy(&dev[thisdev].xmem[6], dump_model, sizeof(dump_model)); //static_x_model is a static kopie of a real X-model
		else
			memcpy(&dev[thisdev].xmem[6], dump_model_max1, sizeof(dump_model_max1)); //static_x_model is a static kopie of a real X-model
		break;
	case SB_PARAM_SET_ALGO_PARAMS| 0x80:
		dev[thisdev].xmem[0] = 0;
		dev[thisdev].xmem[1] = 0;
		dev[thisdev].xmem[2] = 0;

		memcpy((void *)&dev[thisdev].xmem[6], dev[thisdev].lastConfig, TFA2_ALGOPARAMETER_LENGTH*sizeof(unsigned char));
		if (IS_FAM==2) {
			FUNC_TRACE("get algo\n");
		}
		else { //tfa1 append preset as well
			memcpy(&dev[thisdev].xmem[6 + dev[thisdev].config_length], (void *)dev[thisdev].lastPreset,
					TFA98XX_PRESET_LENGTH*sizeof(unsigned char));
			FUNC_TRACE("get all (config+preset)\n");
		}
		break;
	case SB_PARAM_GET_RE25C:		/* : */
		FUNC_TRACE("PARAM_GET_RE0\n");

		dev[thisdev].xmem[0] = 0;
		dev[thisdev].xmem[1] = 0;
		dev[thisdev].xmem[2] = 0;
		dev[thisdev].xmem[3] = 0x07;
		dev[thisdev].xmem[4] = 0x60;
		dev[thisdev].xmem[5] = 0xcd;
		/*calibration value are different for max1 and max 2 */
		/*for i2c trace different SB_PARAM_GET_RE25C values are required */
		if (IS_FAM==2) {
			dev[thisdev].xmem[7] = 0x6f;
			dev[thisdev].xmem[6] = 0x07;
		} else {
			dev[thisdev].xmem[7] = 0x1f;
			dev[thisdev].xmem[6] = 0x02;
		}
		dev[thisdev].xmem[8] = 0xcd;
		dev[thisdev].xmem[9] = 0x07;
		dev[thisdev].xmem[10] = 0x6e;
		dev[thisdev].xmem[11] = 0x5b;
		//dev[thisdev].Reg[TFA98XX_KEY1_PROTECTED_MTP4] = 52422;
		//dev[thisdev].Reg[TFA98XX_KEY1_PROTECTED_MTP5] = 52422;
		break;
	case SB_PARAM_SET_AGCINS:
		FUNC_TRACE("SB_PARAM_SET_AGCINS\n");
		dev[thisdev].xmem[0] = 0;
		dev[thisdev].xmem[1] = 0;
		dev[thisdev].xmem[2] = 0;
		break;
	case SB_PARAM_SET_DRC:
		FUNC_TRACE("SB_PARAM_SET_DRC\n");
		break;
	case SB_PARAM_SET_MBDRC:
		FUNC_TRACE("SB_PARAM_SET_MBDRC\n");
		break;
	case SB_PARAM_SET_MBDRC_WITHOUT_RESET:
		FUNC_TRACE("SB_PARAM_SET_MBDRC_WITHOUT_RESET\n");
		break;
	case SB_PARAM_SET_ALGO_PARAMS_WITHOUT_RESET:
		FUNC_TRACE("SB_PARAM_SET_ALGO_PARAMS_WITHOUT_RESET\n");
		break;
	case SB_PARAM_GET_MBDRC:
		for(i=0;i<sizeof(mbdrc_model);i++)
			dev[thisdev].xmem[i+6] = mbdrc_model[i];
		//for(i=3;i<62;i++)
		//	dev[thisdev].xmem[i] = 0x11;//vstep_model[i];
		FUNC_TRACE("SB_PARAM_GET_MBDRC\n");
		break;
	case SB_PARAM_GET_MBDRC_DYNAMICS:
		for(i=0;i<(LSMODEL_MAX_WORDS*3);i++)
			dev[thisdev].xmem[i+XMEM_INDEX] = 0;
		FUNC_TRACE("SB_PARAM_GET_MBDRC_DYNAMICS\n");
		break;
	case 0xc0:
		FUNC_TRACE("max1 device\n");
		break;
	case 0xc1:
		if (IS_FAM == 1)
			memcpy(&dev[thisdev].xmem[6], dump_model_max1, sizeof(dump_model_max1)); //static_x_model is a static kopie of a real X-model

		FUNC_TRACE("max1 device\n");
		break;
	default:
		PRINT("%s: unknown RPC PARAM:0x%0x\n", __FUNCTION__, param);
		dev[thisdev].xmem[0] = 0;
		dev[thisdev].xmem[1] = 0;
		dev[thisdev].xmem[2] = 3;
		break;
	}

	return i - 1;
}
/*
 *
 */
#define BIQUAD_COEFF_SIZE 6
static int setDspParamsEq(int param)
{
	int index, size;

	dev[thisdev].xmem[0] = 0;
	dev[thisdev].xmem[1] = 0;
	dev[thisdev].xmem[2] = 0;

	DUMMYVERBOSE hexdump(NULL, &dev[thisdev].xmem[6], 18);

	switch (dev2fam(&dev[thisdev]))
	{
	case 1:
		index = param & 0x7f;
		if (index > 0x0a) {
			FUNC_TRACE("EQ: unsupported param: 0x%x\n", param);
			return 0;
		} else if (index == 0) {
			size = TFA2_FILTERCOEFSPARAMETER_LENGTH;
		} else {
			size = BIQUAD_COEFF_LENGTH;
		}
		break;
	case 2:
		switch (param & 0x7f) {
		case 0:
			index = 0;
			size = TFA2_FILTERCOEFSPARAMETER_LENGTH;
			break;
		case 1:
			index = 0;
			size = 10 * BIQUAD_COEFF_LENGTH;
			break;
		case 2:
			index = 10;
			size = 10 * BIQUAD_COEFF_LENGTH;
			break;
		case 3:
			index = 20;
			size = 2 * BIQUAD_COEFF_LENGTH;
			break;
		case 4:
			index = 22;
			size = 2 * BIQUAD_COEFF_LENGTH;
			break;
		case 5:
			index = 24;
			size = 2 * BIQUAD_COEFF_LENGTH;
			break;
		case 6:
			index = 26;
			size = 2 * BIQUAD_COEFF_LENGTH;
			break;
		default:
			FUNC_TRACE("EQ: unsupported param: 0x%x\n", param);
			return 0;
		}
		break;
	default:
		FUNC_TRACE("EQ: unsupported family: %d\n", dev2fam(&dev[thisdev]));
		return 0;
	}

	if ((param & BFB_PAR_ID_GET_COEFS) == BFB_PAR_ID_GET_COEFS) {
		FUNC_TRACE("EQ: GetCoefs %d\n", index);
		memcpy(&dev[thisdev].xmem[3], (void*)&dev[thisdev].filters[index*BIQUAD_COEFF_LENGTH], size);
	} else {
		FUNC_TRACE("EQ: SetCoefs %d\n", index);
		memcpy((void*)&dev[thisdev].filters[index*BIQUAD_COEFF_LENGTH], &dev[thisdev].xmem[6], size);
	}

	return 0;
}

static int setDspParamsRe0(int param)
{
		dev[thisdev].xmem[0] = 0;
		dev[thisdev].xmem[1] = 0;
		dev[thisdev].xmem[2] = 0;
		PRINT("\nRe[%d] ", param);

		return 0;
}
/*
 * fill dev[thisdev].xmem Framework module RPC buffer with the return values
 */
static int setDspParamsFrameWork(int param)
{
	int len,i=0,j=0;
//	printf(">>>>>>>>>>%0x\n", param);
	//FUNC_TRACE("FrameWork");
	dev[thisdev].xmem[0] = 0;
	dev[thisdev].xmem[1] = 0;
	dev[thisdev].xmem[2] = 0;

	switch (param) {

	//case FW_PAR_ID_SET_VBAT_FACTOR:
		//break;
	case FW_PAR_ID_SET_MEMORY://duplicate MSG ID, also FW_PAR_ID_SET_CURRENT_DELAY
		if (IS_FAM==2) {
			FUNC_TRACE("FW_PAR_ID_SET_MEMORY\n");
		}
		else {
			FUNC_TRACE("FW_PAR_ID_SET_CURRENT_DELAY (set firmware delay table):");
			for(i=0;i<9;i++)
				FUNC_TRACE(" %d", dev[thisdev].xmem[8+i*3]);
			FUNC_TRACE("\n");
		}
		break;
	case FW_PAR_ID_SET_SENSES_DELAY:
		FUNC_TRACE("FW_PAR_ID_SET_SENSES_DELAY\n");
		break;
	case FW_PAR_ID_SET_INPUT_SELECTOR: //duplicate MSG ID, also FW_PAR_ID_SET_CURFRAC_DELAY
		if (IS_FAM==2) {
			FUNC_TRACE("FW_PAR_ID_SET_INPUT_SELECTOR\n");
		}
		else {
			FUNC_TRACE("FW_PAR_ID_SET_CURFRAC_DELAY (set fractional delay table):");
			for(i=0;i<9;i++)
				FUNC_TRACE(" %d", dev[thisdev].xmem[8+i*3]);
			FUNC_TRACE("\n");
		}
		break;
	case 7:
		FUNC_TRACE("strange, this was FW_PAR_ID_SET_INPUT_SELECTOR_2:0x07, unspec-ed now\n");
		break;
	case FW_PAR_ID_SET_OUTPUT_SELECTOR:
		FUNC_TRACE("FW_PAR_ID_SET_OUTPUT_SELECTOR\n");
		break;
	case SB_PARAM_SET_LAGW:
		FUNC_TRACE("SB_PARAM_SET_LAGW\n");
		break;
	case FW_PAR_ID_SET_PROGRAM_CONFIG:
		FUNC_TRACE("FW_PAR_ID_SET_PROGRAM_CONFIG\n");
		break;
	case FW_PAR_ID_SET_GAINS:
		FUNC_TRACE("FW_PAR_ID_SET_GAINS\n");
		break;
	case FW_PAR_ID_SETSENSESCAL:
		FUNC_TRACE("FW_PAR_ID_SETSENSESCAL\n");
		break;
	case FW_PAR_ID_GET_FEATURE_INFO:
		FUNC_TRACE("FW_PAR_ID_GET_FEATURE_INFO\n");
		dev[thisdev].xmem[0] = 0;
		dev[thisdev].xmem[1] = 0;
		dev[thisdev].xmem[2] = (dev[thisdev].type == tfa9890) ? 3 : 0;	/* no feature bits */
		/* i2cExecuteRS W   2: 0x6c 0x72 */
		/* i2cExecuteRS R   4: 0x6d 0x00 0x00 0x03 */
		dev[thisdev].xmem[6] = 0x00;
		dev[thisdev].xmem[7] = (dev[thisdev].type == tfa9895 ||
				dev[thisdev].type == tfa9891 ) ? 0x00 : 0x02;	/* No DRC */
		dev[thisdev].xmem[8] = 0x00;
		if(dev[thisdev].type ==tfa9888)
		{
			dev[thisdev].xmem[7] = 0x01;
			dev[thisdev].xmem[8] = 0x01;
		}

		//dev[thisdev].mbdrc_status = 0x100;
		break;
#define TFA1_DSP_revstring        "< Dec 21 2011 - 12:33:16 -  SpeakerBoostOnCF >"
#define TFA2_DSP_revstring		 "< Dec  8 2014 - 17:13:09 - App_Max2 >"
	case FW_PAR_ID_GET_TAG: {
		uint8_t *ptr;
		char *tag = IS_FAM==1 ? TFA1_DSP_revstring : TFA2_DSP_revstring;
		FUNC_TRACE("FW_PAR_ID_GET_TAG\n");
		/* tag */
		ptr = &dev[thisdev].xmem[6 + 2];
		for (i = 0, j = 0; i < TFA98XX_MAXTAG; i++, j += 3) {
			ptr[j] = tag[i];	/* 24 bits, byte[2] */
			if(tag[i]=='\0')
				break;
		}
		if (lxDummyFailTest == 6)
			ptr[0] = '!';	/* fail */
		/* *pRpcStatus = (mem[0]<<16) | (mem[1]<<8) | mem[2]; */
		dev[thisdev].xmem[0] = 0;
		dev[thisdev].xmem[1] = 0;
		dev[thisdev].xmem[2] = 0;
	}
		break;

	case FW_PAR_ID_SET_MEMTRACK:
		len = XMEM2INT(2*3) ;//dev[thisdev].xmem[8];
		FUNC_TRACE("FW_PAR_ID_SET_MEMTRACK: len=%d words\n", len);
		if (len>20) {
			DUMMYVERBOSE PRINT("Error memtrack list too long (max=20)!!! %d \n", len);
			return 1;
		}
		for (i=(3*3);i<(len+3)*3;i+=3) {
			PRINT(" %d 0x%04x", dev[thisdev].xmem[i], dev[thisdev].xmem[i+1]<<8|dev[thisdev].xmem[i+2]);
		}
		PRINT("\n");
		break;
	case FW_PAR_ID_GET_MEMTRACK:
		{// fill up buffer to be transmitted 
		char *bytes = (char *)malloc(sizeof(char)*153);
		fill_up_buffer_live_data(bytes);
		for(i=0;i<153;i++)
			dev[thisdev].xmem[i+6] = bytes[i];
		free(bytes);
		break;
		}
	case SB_PARAM_GET_MBDRC:
		for(i=0;i<VSTEP_MODEL_LENGHT;i++)
			dev[thisdev].xmem[i+XMEM_INDEX] = vstep_model[i];
		FUNC_TRACE("SB_PARAM_GET_MBDRC");
		break;
	case SB_PARAM_GET_MBDRC_DYNAMICS:
		FUNC_TRACE("SB_PARAM_GET_MBDRC_DYNAMICS");
		break;
	default:
		FUNC_TRACE("Unknown M-ID 0x%04x \n", param);
	}

	return 0;
}

/******************************************************************************/
int fill_up_buffer_live_data(char *bytes)
{
	int read;
	char *parser;
	int i=0;
	static int counter=0;
	char add_value=0;
	unsigned char test_value=0, len=0;
	char str[1250];//=NULL;

	counter++;
	*(bytes++) = 0x01;
	*(bytes++) = 0x89;
	*(bytes++) = 0xb1;
	
	memcpy(str, str_live_data, sizeof(str_live_data)); 
	for (parser = strtok(str,","); parser != NULL; parser = strtok(NULL, ","))
	{
		for(i=0;i<70;i++)
		{							
			read = strcmp(parser,live_data_lookup[i].live_data_name_dummy);
			if(read==0)//, sizeof(parser)==0))
			{
				add_value=0;
				test_value=0;
				// for deg C and mm 

				if(counter<=10)
					add_value=2+test_value;
				if((counter>10)&(counter<20))
					add_value=-(2+test_value);

				*(bytes++) = live_data_lookup[i].default_value[0];
				*(bytes++) = live_data_lookup[i].default_value[1];
				*(bytes++) = live_data_lookup[i].default_value[2]+add_value;
			}
		}
	len++;
	}
	if(counter==20)
		counter=0;

	return 0;
}
/******************************************************************************
 * utility and helper functions
 */
static FILE *infile;
/*
 * set the input file for state info input
 */
static int setInputFile(char *file)
{
	char line[256];

	infile = fopen(file, "r");

	if (infile == 0)
		return 0;

	fgets(line, sizeof(line), infile);	/* skip 1st line */

	return 1;
}

/* convert DSP memory bytes to signed 24 bit integers
   data contains "num_bytes/3" elements
   bytes contains "num_bytes" elements */
void convertBytes2Data24(int num_bytes, const unsigned char bytes[],
			       int data[])
{
	int i;			/* index for data */
	int k;			/* index for bytes */
	int d;
	int num_data = num_bytes / 3;
	//_ASSERT((num_bytes % 3) == 0);
	for (i = 0, k = 0; i < num_data; ++i, k += 3) {
		d = (bytes[k] << 16) | (bytes[k + 1] << 8) | (bytes[k + 2]);
		//_ASSERT(d >= 0);
		//_ASSERT(d < (1 << 24));	/* max 24 bits in use */
		if (bytes[k] & 0x80)	/* sign bit was set */
			d = -((1 << 24) - d);

		data[i] = d;
	}
}

/*
 * return family type for this device
 *  copy of the tfa tfa98xx_dev2family(this->Reg[3]);
 *  NOTE: Dependency from HAL layer. Does not work on Windows..
 *  TODO: replace by TFA API when in single library
 */
static int dev2fam(struct emulated_device *this) {
	int dev_type = this->Reg[3];
	/* only look at the die ID part (lsb byte) */
	switch(dev_type & 0xff) {
	case 0x12:
	case 0x80:
	case 0x81:
	case 0x91:
	case 0x92:
	case 0x97:
		return 1;
	case 0x88:
	case 0x72:
		return 2;
	case 0x50:
		return 3;
	default:
		return 0;
	}
}

static int dummy_set_bf(struct emulated_device *this, const uint16_t bf, const uint16_t value)
{
	uint16_t regvalue, msk, oldvalue;

	/*
	 * bitfield enum:
	 * - 0..3  : len
	 * - 4..7  : pos
	 * - 8..15 : address
	 */
	uint8_t len = bf & 0x0f;
	uint8_t pos = (bf >> 4) & 0x0f;
	uint8_t address = (bf >> 8) & 0xff;

	regvalue = this->Reg[address];

	oldvalue = regvalue;
	msk = ((1 << (len + 1)) - 1) << pos;
	regvalue &= ~msk;
	regvalue |= value << pos;

	/* Only write when the current register value is not the same as the new value */
	if (oldvalue != regvalue) {

		this->Reg[address] = regvalue;
	}

	return 0;
}

static int dummy_get_bf(struct emulated_device *this, const uint16_t bf)
{
	uint16_t regvalue, msk;
	uint16_t value;

	/*
	 * bitfield enum:
	 * - 0..3  : len
	 * - 4..7  : pos
	 * - 8..15 : address
	 */
	uint8_t len = bf & 0x0f;
	uint8_t pos = (bf >> 4) & 0x0f;
	uint8_t address = (bf >> 8) & 0xff;

    regvalue = this->Reg[address];

	msk = ((1<<(len+1))-1)<<pos;
	regvalue &= msk;
	value = regvalue>>pos;

	return value;
}

/*
 * ext DSP
 */
static uint32_t lxDummy_show_cmd(const char *buffer ){
	uint8_t *buf = (uint8_t *)buffer;
	uint32_t cmd_id = buf[0]<<16 |  buf[1]<<8 | buf[2];

	//PRINT("%s, id:0x%08x\n", __FUNCTION__, cmd_id);

	return cmd_id;
}
#define WORD16(MSB, LSB) ((uint16_t)( ((uint8_t)MSB)<<8 | (uint8_t)LSB))
static int multi_msg(uint8_t *buf) {
	uint16_t msg_length,done_length=0, total_length =  WORD16(buf[2], buf[3]);
	int count=0;
	uint8_t *msg = buf+4;//+sizeof(msg_length); //1st msg

	if ( buf[0]!='m' || buf[1]!='m') //must be multi header
		return -1;
	if ( !total_length)				// with at least 1 msg
		return -1;

	do {
	msg_length = WORD16(msg[0], msg[1]); // this msg length
	msg+=2;								 // point to payload
	FUNC_TRACE("%s,[%d] id:0x%02x%02x%02x, length:%d \n",__FUNCTION__,count, msg[0], msg[1], msg[2], msg_length);
	msg += msg_length; //next
	done_length += msg_length+2;
	count++;
	} while(done_length < total_length);

	return count;
}
static int lxDummy_write_message(int fd, int length, const char *buffer) {
	uint8_t *buf = (uint8_t *)buffer;
	uint8_t *ptr;
	int i,j,result=0;
	uint32_t cmd_id;
	char *tag = "< June 30 2016 - 15:06:09 - 72 dummy test >";
	(void)fd;

	//DUMMYVERBOSE hexdump(length, buf);
	if ( buf[0]=='m' && buf[1]=='m') {//multi header
		multi_msg(buf);
	}
	FUNC_TRACE("%s, id:0x%02x%02x%02x, length:%d \n",__FUNCTION__,buf[0], buf[1], buf[2], length);
	/* tag */
#if (defined(WIN32) || defined(_X64))
	bzero(result_buffer, sizeof(result_buffer));
#endif
	cmd_id = lxDummy_show_cmd(buffer);

	if ( cmd_id & 0x8000) {//MODULE_FRAMEWORK
		switch(cmd_id & 0xff) {
		case  FW_PAR_ID_GET_TAG:
			FUNC_TRACE("FW_PAR_ID_GET_TAG\n");
			ptr = (uint8_t *)&result_buffer[5];
			for (i = 0, j = 0; i < TFA98XX_MAXTAG; i++, j += 3) {
				ptr[j] = tag[i];	/* 24 bits, byte[2] */
				if(tag[i]=='\0')
					break;
			}
			break;
		case  FW_PAR_ID_GET_API_VERSION:
			result_buffer[3] = 1;
			result_buffer[4] = 2;
			result_buffer[5] = 3;
			break;
		default:
			PRINT("%s, id:0x%02x%02x%02x, length:%d \n",__FUNCTION__,buf[0], buf[1], buf[2], length);
			result=-1;
			break;
		}
	}
	return result;
}

static int lxDummy_read_message(int fd, int length, char *buffer) 
{
	uint8_t *buf = (uint8_t *)buffer;
	(void)fd;

	buffer[2]='w';
	buffer[5]='i';
	buffer[8]='m';
	memcpy( buffer, result_buffer, length);
	FUNC_TRACE("%s, id:0x%02x%02x%02x, length:%d \n",__FUNCTION__,buf[0], buf[1], buf[2], length);

	return 0;
}

static int lxDummy_execute_message(int command_length, void *command_buffer,
		int read_length, void *read_buffer) {
	uint8_t *buf = (uint8_t *)command_buffer;
	int result=-1;

	FUNC_TRACE("%s, id:0x%02x%02x%02x, length:%d \n",__FUNCTION__,buf[0], buf[1], buf[2], command_length);
	//   command_length!=0 & read_length==0
	//   	execute command in command buffer

	//   >> WRITE COMMAND ONLY <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
	if ( (command_length!=0) & (read_length==0) ) {
		result = lxDummy_write_message(0, command_length, command_buffer);
		result = result ? result : (int)result_buffer[0] ;// error if non-zero
	}
	//   >> WRITE COMMAND AND RETURN RESULT <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
	else if ( (command_length!=0) & (read_length!=0) ) {
		result = lxDummy_write_message(0, command_length, command_buffer);
		if ( result == 0 )
			result =lxDummy_read_message( 0, read_length, read_buffer);
	}
	//    >> RETURN RESULT ONLY <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
	else if ( (command_length==0) & (read_length!=0) ) {
		result=lxDummy_read_message( 0, read_length, read_buffer);
	}

	// return value is actual returned length or dsp result status or <0 if error
	return result;
}
