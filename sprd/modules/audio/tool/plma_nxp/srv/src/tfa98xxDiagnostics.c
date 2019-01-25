/*
 * @file tfa98xxDiagnostics.c
 *
 *  Created on: Jun 7, 2012
 *      Author: Wim Lemmers
 */

#include <tfa98xxDiagnostics.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#include "../../tfa/inc/tfa98xx_genregs_N1C.h"
#include "tfa_container.h"
#include "dbgprint.h"

// TFA98XX API
#include "Tfa98API.h"
#include "tfa_service.h"

// srv
#include "tfa98xxCalibration.h"
#include "tfaContainerWrite.h"
#include "tfa98xx_tfafieldnames.h"
// for NXP_I2C_MAX_SIZE
#ifndef I2C_MAX_SIZE
#include "NXP_I2C.h"
#include "lxScribo.h" /* for gpio wait */

#define I2C_MAX_SIZE NXP_I2C_MAX_SIZE
#endif

/*
 *
 */
/* *INDENT-OFF* */
/*****************************************************************************
 * test functions arrays
 */

static tfa98xxDiagTest_t DiagTest_no_dsp[] = {
		{tfa_diag_help                  ,"list all tests descriptions", 0},
		{tfa_diag_register_read         ,"read test of DEVID register", tfa_diag_i2c},
		{tfa_diag_register_write_read_72	,"write/read test of register (SWPROFIL)", tfa_diag_i2c},
		{tfa_diag_clock_enable_72		,"check PLL status after poweron", tfa_diag_i2c},
		{NULL, NULL, 0},
		{NULL, NULL, 0},
		{NULL, NULL, 0},
		{NULL, NULL, 0},
		{NULL, NULL, 0},
		{NULL, NULL, 0},
		{NULL, NULL, 0},
		{NULL, NULL, 0},
		{NULL, NULL, 0},
		{NULL, NULL, 0},
		{NULL, NULL, 0},
		{NULL, NULL, 0},
		{NULL, NULL, 0},
		{NULL, NULL, 0},
		{NULL, NULL, 0},
		{NULL, NULL, 0},
		{NULL, NULL, 0}
};

static tfa98xxDiagTest_t DiagTest_tiberius[] = {
		{tfa_diag_help                  ,"list all tests descriptions", 0},
		{tfa_diag_register_read         ,"read test of DEVID register", tfa_diag_i2c},
		{tfa_diag_register_write_read_72	,"write/read test of register (SWPROFIL)", tfa_diag_i2c},
		{tfa_diag_clock_enable_72		,"check PLL status after poweron", tfa_diag_i2c},
//		{tfa_diag_xmem_access		,"write/read test of  all xmem locations", tfa_diag_i2c},
//		{tfa_diag_ACS_via_iomem		,"set ACS bit via iomem, read via status reg", tfa_diag_i2c},
//		{tfa_diag_xmem_burst_read 	,"write full xmem, read with i2c burst", tfa_diag_i2c},
//		{tfa_diag_xmem_burst_write	,"write/read full xmem in i2c burst", tfa_diag_i2c},
//		{tfa_diag_patch_load		,"load a patch and verify version number", tfa_diag_dsp },
//		{tfa_diag_dsp_reset		,"verify dsp response to reset toggle", tfa_diag_dsp},
		{tfa_diag_read_api_version ,"read back the API version", tfa_diag_dsp },
		{tfa_diag_read_version_tag	,"read back the ROM version tag", tfa_diag_dsp },
		{tfa_diag_start_speakerboost	,"load the configuration to bring up SpeakerBoost", tfa_diag_dsp },
		{tfa_diag_read_speaker_status	,"read the speaker damage bit to verify the speaker status", tfa_diag_sb },
		{tfa_diag_verify_parameters	,"read back to verify that all parameters are loaded", tfa_diag_sb },
		{tfa_diag_verify_features	,"read features from MTP and cnt file to validate", tfa_diag_sb },
		{tfa_diag_calibrate_always	,"run a calibration and check for success", tfa_diag_sb },
		{tfa_diag_speaker_impedance	,"verify that the speaker impedance is within range", tfa_diag_sb },
		{tfa_diag_irq_warm		,"test interrupt ACS & ACK bits", tfa_diag_func },
		{tfa_diag_irq_cold		,"low level test interrupt bits", tfa_diag_func },
		{tfa_diag_i2c_scan		,"scan i2c bus", 0 },
		{NULL, NULL, 0}
};

static tfa98xxDiagTest_t DiagTest_with_dsp[] = {
		{tfa_diag_help                  ,"list all tests descriptions", 0},
		{tfa_diag_register_read         ,"read test of DEVID register", tfa_diag_i2c},
		{tfa_diag_register_write_read	,"write/read test of register (MADD)", tfa_diag_i2c},
		{tfa_diag_clock_enable		,"check PLL status after poweron", tfa_diag_i2c},
		{tfa_diag_xmem_access		,"write/read test of  all xmem locations", tfa_diag_i2c},
		{tfa_diag_ACS_via_iomem		,"set ACS bit via iomem, read via status reg", tfa_diag_i2c},
		{tfa_diag_xmem_burst_read 	,"write full xmem, read with i2c burst", tfa_diag_i2c},
		{tfa_diag_xmem_burst_write	,"write/read full xmem in i2c burst", tfa_diag_i2c},
		{tfa_diag_patch_load		,"load a patch and verify version number", tfa_diag_dsp },
		{tfa_diag_dsp_reset		,"verify dsp response to reset toggle", tfa_diag_dsp},
		{tfa_diag_read_version_tag	,"read back the ROM version tag", tfa_diag_dsp },
		{tfa_diag_start_speakerboost	,"load the configuration to bring up SpeakerBoost", tfa_diag_dsp },
		{tfa_diag_read_speaker_status	,"read the speaker damage bit to verify the speaker status", tfa_diag_sb },
		{tfa_diag_verify_parameters	,"read back to verify that all parameters are loaded", tfa_diag_sb },
		{tfa_diag_verify_features	,"read features from MTP and cnt file to validate", tfa_diag_sb },
		{tfa_diag_calibrate_always	,"run a calibration and check for success", tfa_diag_sb },
		{tfa_diag_speaker_impedance	,"verify that the speaker impedance is within range", tfa_diag_sb },
		{tfa_diag_irq_warm		,"test interrupt ACS & ACK bits", tfa_diag_func },
		{tfa_diag_irq_cold		,"low level test interrupt bits", tfa_diag_func },
		{tfa_diag_i2c_scan		,"scan i2c bus", 0 },
		{tfa_diag_tap		,"wait for tap detection", 0 },
		{NULL, NULL, 0}
};

/*****************************************************************************/


/* DSP firmware defines */
/* TODO define DSP values */
#define TFA2_XMEM_CALIBRATION_DONE   516
#define TFA2_XMEM_COUNT_BOOT		  	512
#define TFA2_XMEM_CMD_COUNT			520
#define TFA1_XMEM_COUNT_BOOT			0xa1

#define TFA2_XMEM_PATCHVERSION 0x1fff
#define TFA9912_XMEM_PATCHVERSION 0x157e
#define TFA1_XMEM_PATCHVERSION 0x12bf	// TODO properly define
#define TFA9897_XMEM_PATCHVERSION 0x0d7f
#define TFA9896_XMEM_PATCHVERSION 0x0d7f
#define TFA9891_XMEM_PATCHVERSION 0x13ff

//#define XMEM_MAX (8*1024)
#define TFA2_XMEM_MAX 1000 //(8*1024) //TODO properly define max cf memories


#ifdef noDIAGTRACE
#include <stdio.h>
#define TRACEIN  if(tfa98xxDiag_trace) PRINT("Enter %s\n", __FUNCTION__);
#define TRACEOUT if(tfa98xxDiag_trace) PRINT("Leave %s\n", __FUNCTION__);
#else
#define TRACEIN
#define TRACEOUT
#endif

/* abstract family for xmem */
#define TFA_FAM_XMEM(dev_idx, xmem) ((tfa98xx_dev_family(dev_idx) == 1) ? TFA1_XMEM_##xmem :  TFA2_XMEM_##xmem)

/* abstract family for register */
#define FAM_TFA98XX_CF_CONTROLS (TFA_FAM(handle,RST) >>8)
#define FAM_TFA98XX_CF_MEM      (TFA_FAM(handle,MEMA)>>8)

/************************from tfa **************************************/
static enum Tfa98xx_Error tfa98xx_dsp_mem_write(int handle,
		enum Tfa98xx_DMEM which_mem,
		unsigned short start_offset,
		int length, const unsigned char *bytedata) {
	enum Tfa98xx_Error error;
        unsigned short value;

        error = reg_read(handle, FAM_TFA98XX_CF_CONTROLS, &value);
        assert(error == Tfa98xx_Error_Ok);
        value &= ~TFA98XX_CF_CONTROLS_DMEM;
        value |= (unsigned short)which_mem << TFA98XX_CF_CONTROLS_DMEM_POS;
        /* set mem ctl */
	error = reg_write(handle, FAM_TFA98XX_CF_CONTROLS, value);

	TFA_WRITE_REG(handle, MADD, start_offset);

#define ROUND_DOWN(a,n) (((a)/(n))*(n))

	if (error == Tfa98xx_Error_Ok) {
		int offset = 0;
		int chunk_size = ROUND_DOWN(NXP_I2C_BufferSize(),
					which_mem== Tfa98xx_DMEM_PMEM ? 4 : 3); /* MEM word size */
		int remaining_bytes = length;

		/* due to autoincrement in cf_ctrl, next write will happen at
		 * the next address */
		while ((error == Tfa98xx_Error_Ok) && (remaining_bytes > 0)) {
			if (remaining_bytes < chunk_size)
				chunk_size = remaining_bytes;
			/* else chunk_size remains at initialize value above */
			error = tfa98xx_write_data(handle, FAM_TFA98XX_CF_MEM, chunk_size,
					bytedata + offset);
			remaining_bytes -= chunk_size;
			offset += chunk_size;
		}
	}

	return error;
}

/*
 *
 */
enum Tfa98xx_Error tfaRunResetCountClear(Tfa98xx_handle_t handle) 
{
  return mem_write(handle,TFA_FAM_XMEM(handle,COUNT_BOOT) , 0, Tfa98xx_DMEM_XMEM);
}

/*
 *
 */
int tfaRunResetCount(Tfa98xx_handle_t handle) {
	int count;

	mem_read(handle, TFA_FAM_XMEM(handle,COUNT_BOOT), 1, &count);

        return count;
}

/* Tfa98xx_DspConfigParameterCount
 * Yields the number of parameters to be used in tfa98xx_dsp_write_config()
 */
enum Tfa98xx_Error tfa98xx_dsp_config_parameter_count(Tfa98xx_handle_t handle,
                        int *pParamCount)
{
	enum Tfa98xx_Error error = Tfa98xx_Error_Ok;
	uint16_t rev;

	if (handle < 0)
		return Tfa98xx_Error_Bad_Parameter;

	rev = tfa98xx_get_device_revision(handle);
	rev &= 0xff; // only use chip rev

	switch (rev) {
		case 0x12:
			*pParamCount = 67;
			break;
		case 0x92:
		case 0x97:
			*pParamCount = 67;
			break;
		case 0x96:
			*pParamCount = 67;
			break;
		case 0x80:
		case 0x81: // for the RAM version
		case 0x91:
			*pParamCount = 55;
			break;
		default:
			/* unsupported case, possibly intermediate version */
			error = Tfa98xx_Error_Not_Supported;
			_ASSERT(0);
	}

	return error;
}
/* Tfa98xx_DspConfigParameterType
 *   Returns the config file subtype
 */
enum Tfa98xx_Error tfa98xx_dsp_config_parameter_type(int slave,
		enum Tfa98xx_config_type *ptype) {
	enum Tfa98xx_Error error = Tfa98xx_Error_Ok;
	int handle = tfa98xx_cnt_slave2idx(slave);
	int rev;

	if (handle < 0)
		return Tfa98xx_Error_Bad_Parameter;

	rev = tfa98xx_cnt_slave2revid(slave);
	if (rev < 0)
		return Tfa98xx_Error_Bad_Parameter;
	rev &= 0xff; // only use chip rev

	*ptype = Tfa98xx_config_generic; //devualt

	switch (rev) {
		case 0x12:
			*ptype = Tfa98xx_config_sub2;
			break;
		case 0x80:
		case 0x81: // for the RAM version
		case 0x91: // for 90B and 91
			*ptype = Tfa98xx_config_sub1;
			break;
		case 0x92:
		case 0x97:
			*ptype = Tfa98xx_config_sub3;
			break;
		case 0x96:
			*ptype = Tfa98xx_config_sub3;
			break;
		default:
			/* unsupported case, possibly intermediate version */
			error = Tfa98xx_Error_Not_Supported;
			_ASSERT(0);
	}

	return error;
}

/*
 *
 */
enum Tfa98xx_Error
tfa98xx_dsp_read_spkr_params(Tfa98xx_handle_t handle,
              unsigned char paramId,
              int length, unsigned char *pSpeakerBytes)
{
    enum Tfa98xx_Error error;

    if (pSpeakerBytes != 0) {
        error =
            tfa_dsp_cmd_id_write_read(handle, MODULE_SPEAKERBOOST,
                    paramId, length, pSpeakerBytes);
    } else {
        error = Tfa98xx_Error_Bad_Parameter;
    }
    return error;
}
enum Tfa98xx_Error tfa98xx_dsp_read_config(Tfa98xx_handle_t handle, int length,
                      unsigned char *pConfigBytes)
{
    enum Tfa98xx_Error error = Tfa98xx_Error_Ok;

    if (pConfigBytes != 0) {
        /* Here one can keep it simple by reading only the first
         * length bytes from DSP memory */
        error = tfa_dsp_cmd_id_write_read(handle, MODULE_SPEAKERBOOST,
        		SB_PARAM_GET_ALGO_PARAMS, length,
                        pConfigBytes);
    } else {
        error = Tfa98xx_Error_Bad_Parameter;
    }
    return error;
}

enum Tfa98xx_Error tfa98xx_dsp_read_preset(Tfa98xx_handle_t handle, int length,
                      unsigned char *pPresetBytes)
{
	enum Tfa98xx_Error error = Tfa98xx_Error_Ok;
	/* Here one cannot keep it simple as in Tfa98xx_DspReadConfig(), */
	/* since we are interested by the LAST length bytes from DSP memory */
	unsigned char temp[TFA1_MAX_PARAM_SIZE];
	int configlength;

	if (pPresetBytes != 0) {
		error = tfa98xx_dsp_config_parameter_count(handle, &configlength);
		_ASSERT(error == Tfa98xx_Error_Ok); /* an error should not happen */
		configlength *= 3; /* word to bytes count */
		error = tfa_dsp_cmd_id_write_read(handle, MODULE_SPEAKERBOOST,
		SB_PARAM_GET_ALGO_PARAMS, (configlength + TFA98XX_PRESET_LENGTH), temp);
		if (error == Tfa98xx_Error_Ok) {
			int i;
			for (i = 0; i < length; i++) {
				pPresetBytes[i] = temp[configlength + i];
			}
		}
	} else {
		error = Tfa98xx_Error_Bad_Parameter;
	}

	return error;
}

enum Tfa98xx_Error tfa98xx_dsp_read_drc(Tfa98xx_handle_t handle, int length, 
			unsigned char *pDrcBytes)
{
	enum Tfa98xx_Error error = Tfa98xx_Error_Ok;
	uint8_t cmdid[3];
	int i = 0, input[3];
	unsigned char input_bytes[3 * 3];
	/* First part of drc, limited by 80 words */
	unsigned char output_bytes[(80 * 3) + 1] = {0}; 
	/* Second part of drc, limited by 80 words */
	unsigned char output_bytes2[47 * 3] = {0}; 

	if (length > 127) {	/* DRC is 127 words */
		return Tfa98xx_Error_Bad_Parameter;
	} else {
		/* Read first part of DRC, limited by 80 words */
		input[0] = Tfa98xx_DMEM_XMEM;
		input[1] = 1586;
		input[2] = 80;

		cmdid[0] = 0;
		cmdid[1] = 0 + 128;	/* moduleId */
		cmdid[2] = 5;		/* paramId  */

		tfa98xx_convert_data2bytes(3, input, input_bytes);
		tfa_dsp_msg_id(handle, sizeof(input_bytes), (const char *)input_bytes, cmdid);
		tfa_dsp_msg_read(handle, 80 * 3, output_bytes);

		/* Read second part of DRC, limited by 80 words */
		input[0] = Tfa98xx_DMEM_XMEM;
		input[1] = (1586 + 80);
		input[2] = 47;

		tfa98xx_convert_data2bytes(3, input, input_bytes);
		tfa_dsp_msg_id(handle, sizeof(input_bytes), (const char *)input_bytes, cmdid);
		tfa_dsp_msg_read(handle, 47 * 3, output_bytes2);
	}

	/* Assigning first part of DRC */
	for (i=0; i<(80*3); i++) {
		pDrcBytes[i] = output_bytes[i];
	}

	/* Assigning second part of DRC */
	for (i=(80*3); i<TFA98XX_DRC_LENGTH; i++) {
		pDrcBytes[i] = output_bytes2[i-(80*3)];
	}

	return error;
}

/*****************************************end from tfa *****************************/




/* *INDENT-OFF* */
regdef_t tfa9888_regdefs[] = {
        { 0x00, 0x081d, 0xfeff, "statusreg"}, //ignore MTP busy bit
        { 0x01, 0x0, 0x0, "batteryvoltage"},
        { 0x02, 0x0, 0x0, "temperature"},
        { 0x03, 0x0012, 0xffff, "revisionnumber"},
        { 0x04, 0x888b, 0xffff, "i2sreg"},
        { 0x05, 0x13aa, 0xffff, "bat_prot"},
        { 0x06, 0x001f, 0xffff, "audio_ctr"},
        { 0x07, 0x0fe6, 0xffff, "dcdcboost"},
        { 0x08, 0x0800, 0x3fff, "spkr_calibration"}, //NOTE: this is a software fix to 0xcoo
        { 0x09, 0x041d, 0xffff, "sys_ctrl"},
        { 0x0a, 0x3ec3, 0x7fff, "i2s_sel_reg"},
        { 0x40, 0x0, 0x00ff, "hide_unhide_key"},
        { 0x41, 0x0, 0x0, "pwm_control"},
        { 0x46, 0x0, 0x0, "currentsense1"},
        { 0x47, 0x0, 0x0, "currentsense2"},
        { 0x48, 0x0, 0x0, "currentsense3"},
        { 0x49, 0x0, 0x0, "currentsense4"},
        { 0x4c, 0x0, 0xffff, "abisttest"},
        { 0x62, 0x0, 0, "mtp_copy"},
        { 0x70, 0x0, 0xffff, "cf_controls"},
        { 0x71, 0x0, 0, "cf_mad"},
        { 0x72, 0x0, 0, "cf_mem"},
        { 0x73, 0x00ff, 0xffff, "cf_status"},
        { 0x80, 0x0, 0, "mtp"},
        { 0x83, 0x0, 0, "mtp_re0"},
        { 0xff, 0,0, NULL}
};
regdef_t tfa9888_tdm_regdefs[] = {
		{ 0x10, 0x0220, 0, "tdm_config_reg0"},
		{ 0x11, 0xc1f1, 0, "tdm_config_reg1"},
		{ 0x12, 0x0020, 0, "tdm_config_reg2"},
		{ 0x13, 0x0000, 0, "tdm_config_reg3"},
		{ 0x14, 0x2000, 0, "tdm_config_reg4"},
		{ 0x15, 0x0000, 0, "tdm_status_reg"},
        { 0xff, 0,0, NULL}
};
regdef_t tfa9888_int_regdefs[] = {
		{ TFA98XX_INTERRUPT_OUT_REG1, 0, 0, "int_reg_out1"},
		{ 0x21, 0, 0, "int_reg_out2"},
		{ TFA98XX_INTERRUPT_OUT_REG3, 0, 0, "int_reg_out3"},
		{ TFA98XX_INTERRUPT_IN_REG1, 0, 0, "int_reg_in1"},
		{ 0x24, 0, 0, "int_reg_in2"},
		{ TFA98XX_INTERRUPT_IN_REG3, 0, 0, "int_reg_in3"},
		{ TFA98XX_INTERRUPT_ENABLE_REG1,0x0001 , 0, "int_ena1"},
		{ 0x27, 0, 0, "int_ena2"},
		{ TFA98XX_INTERRUPT_ENABLE_REG3, 0, 0, "int_ena3"},
		{ TFA98XX_STATUS_POLARITY_REG1, 0xf5e2, 0, "int_pol1"},
		{ 0x2a, 0, 0, "int_pol2"},
		{ TFA98XX_STATUS_POLARITY_REG3, 0x0003, 0, "int_pol3"},
        { 0xff, 0,0, NULL}
};
/* *INDENT-ON* */
#define MAXREGS ((sizeof(tfa9888_regdefs)/sizeof(regdef_t))-1)
#define MAXTDMREGS ((sizeof(tfa9888_tdm_regdefs)/sizeof(regdef_t))-1)
#define MAXINTREGS ((sizeof(tfa9888_int_regdefs)/sizeof(regdef_t))-1)

// status register errors to check for not 1
#define TFA98XX_STATUSREG_ERRORS_SET_MSK (  \
		TFA98XX_STATUSREG_OCDS  )
// status register errors to check for not 0
#define TFA98XX_STATUSREG_ERRORS_CLR_MSK (  TFA98XX_STATUSREG_VDDS  |\
		TFA98XX_STATUSREG_UVDS  |  \
		TFA98XX_STATUSREG_OVDS  |  \
		TFA98XX_STATUSREG_OTDS    )
//      TFA98XX_STATUSREG_DCCS   ) TODO check bit



// globals
int tfa98xxDiag_trace = 1;
int tfa98xxDiag_verbose = 0;
static int lastTest = -1;
static int lastError = -1;
static char lastErrorString[256] = "";

static Tfa98xx_Error_t lastApiError;

#if !(defined(WIN32) || defined(_X64))
/************************
 * time measurement
 */
#include <sys/time.h>
#include <sys/resource.h>
#include <stdlib.h>
typedef struct tag_time_measure
{
  struct timeval startTimeVal;
  struct timeval stopTimeVal;

  struct rusage startTimeUsage;
  struct rusage stopTimeUsage;
} time_measure;
time_measure * startTimeMeasuring();
void stopTimeMeasuring(time_measure * tu);
void printMeasuredTime(time_measure * tu);

#endif
/*
 * TODO make this permanent?
 */
int tfa98xxDiagLoadPresetsDouble(int slave);
int tfa98xxDiagLoadPresetsMultiple(int slave);


/*
 * global that holds the active diags test table
 */
static tfa98xxDiagTest_t *DiagTest;// = DiagTest_with_dsp;


int tfa_diag_need_container(int nr) {
	return (DiagTest[nr].group == tfa_diag_all ||
			DiagTest[nr].group == tfa_diag_dsp ||
			DiagTest[nr].group == tfa_diag_sb );

}

static int tfa_diag_maxtest(tfa98xxDiagTest_t *diagtable) {
	int max=0;

	for(;;) {
		if ( diagtable[max++].function == NULL)
			break;
	}

	return max -1 ;
}
void tfa_diag_set_diagtable(int devid) {
	switch (devid & 0x00ff) {
	default:
		DiagTest = DiagTest_with_dsp;
		break;
	case 0x72:
		DiagTest = NXP_I2C_Interface_Type() && NXP_I2C_Msg ? DiagTest_tiberius : DiagTest_no_dsp;
		break;
	}
}
static const char *diag_groupname[]= {
		"", "i2c", "dsp", "sb", "pins","func"
};


/*
 * Set the debug option
 */
void tfaDiagnosticVerbose(int level) {
	tfa98xxDiag_verbose = level;
}

/*
 *  the functions will return 0 if passed
 *  else fail and the return value may contain extra info
 */
 /*
  * run a testnr
  */
int tfa98xxDiag(int slave, int testnr)
{
    int result, maxtest, devid, devidx=0; //TODO get device index or run diag on all?
    nxpTfaContainer_t *cnt;

    /*
     * get the devid if we have a container, of not assume default table
     */
    cnt = tfa98xx_get_cnt();
    if (cnt != NULL ) {
	devidx = tfa98xx_cnt_slave2idx(slave);
    	devid = tfa_cnt_get_devid(cnt, devidx);
        /* set proper table */
        tfa_diag_set_diagtable(devid);
    }

    maxtest = tfa_diag_maxtest(DiagTest);
    if (testnr >= maxtest) {
            PRINT_ERROR("%s: test number:%d too high, max %d \n", __FUNCTION__, testnr, maxtest);
            return -1;
    }

    if (testnr>0)
            PRINT("test %d [%s]: %s\n", testnr,  diag_groupname[DiagTest[testnr].group], DiagTest[testnr].description);

    if ( tfa_diag_need_container(testnr) ) {
        if (tfa98xx_get_cnt() == NULL ) {
        	PRINT_ERROR("No container loaded! (test %d : %s)\n", testnr, DiagTest[testnr].description);
        	return -1;
        }

    }
    result = DiagTest[testnr].function(slave);

    if (testnr != 0)        // don't if 0 that's for all tests
            lastTest = testnr;      // remember the last test for reporting

    if (result==1)
        	sprintf(lastErrorString, "No i2c slave 0x%0x", slave);
    else if (result==2)
        sprintf(lastErrorString, "No clock");
    else if (result==-1)
        sprintf(lastErrorString, "Something wrong with container and/or configuration");
    else if (result==1000)
        sprintf(lastErrorString, "------------test not yet implemented--------------");

    return result;
}

/*
 * list all tests descriptions
 */
int tfa_diag_help(int slave) {
	int i,  maxtest = tfa_diag_maxtest(DiagTest) - 1;

	// run what is in this group
	for (i = 1; i <= maxtest; i++) {
		PRINT("\tdiag=%d  : %s , [%s]\n ", i, 
			DiagTest[i].description,diag_groupname[ DiagTest[i].group]);
	}

	/* Remove unreferenced parameter warning */
	(void)slave;

	return 0;
}
// run all
int tfa98xxDiagAll(int slave)
{
	enum tfa_diag_group groups[]={tfa_diag_i2c, tfa_diag_dsp, tfa_diag_sb};
        int i, result = 0;

        TRACEIN;
        for (i = 0; i < 3; i++) {
//            result = tfa98xxDiag(slave, i);
            result = tfa98xxDiagGroup(slave, groups[i]);
                if (result != 0)
                        break;
        }
        TRACEOUT;
        return result;
}
/* translate group name argument to enum */
enum tfa_diag_group tfa98xxDiagGroupname(char *arg) {

	if (arg==NULL)
		return tfa_diag_all; 		/**< all tests, container needed, destructive */

	if ( strcmp(arg, "i2c")==0 )
		return tfa_diag_i2c;	 	/**< I2C register writes, no container needed, destructive */
	else if (strcmp(arg, "dsp")==0 )
		return tfa_diag_dsp; 	/**< dsp interaction, container needed, destructive */
	else if (strcmp(arg, "sb")==0 )
		return 	tfa_diag_sb; 		/**< SpeakerBoost interaction, container needed, not destructive */
	else if (strcmp(arg, "pins")==0 )
		return tfa_diag_pins;  	/**< Pin tests, no container needed, destructive */

	PRINT_ERROR("Unknown groupname argument: %s\n", arg);

	return tfa_diag_all; 		/**< all tests, container needed, destructive */

}

// run group
int tfa98xxDiagGroup(int slave,  enum tfa_diag_group group) {
    int devidx, devid, i, result = 0, maxtest;
    nxpTfaContainer_t *cnt;

	cnt = tfa98xx_get_cnt();
	devidx = tfa98xx_cnt_slave2idx(slave);
	devid = tfa_cnt_get_devid(cnt, devidx);
	/* set proper table */
	tfa_diag_set_diagtable(devid);

	/* The quickest way to fix the diag for now.
	 * We need to properly detect is there is a DSP for the 72
	 */
	if(devid == 0x72 && devidx == 1) {
		DiagTest = DiagTest_no_dsp;
	}

    maxtest = tfa_diag_maxtest(DiagTest) - 1;
    if (group == tfa_diag_all) 		/**< all tests, container needed, destructive */
		return tfa98xxDiagAll(slave);

	// run what is in this group
	for (i = 1; i <= maxtest; i++) {
		if ( DiagTest[i].group == group) {
			result = tfa98xxDiag(slave, i);
			if (result != 0)
				break;
		}
	}

    return result;
}
/*
 * print supported device features
 */
int tfa98xxDiagPrintFeatures(int devidx, char *buffer, int maxlength) 
{
	Tfa98xx_Error_t err = Tfa98xx_Error_Ok;
	enum Tfa98xx_DAI daimap;
	enum Tfa98xx_saam saam;
	char str[NXPTFA_MAXLINE];
	int status=0;
    unsigned char bytes[TFA98XX_MAXTAG] = {0};

	sprintf(str, "device features:");
    if(tfa_append_substring(str, buffer, maxlength) != Tfa98xx_Error_Ok)
        return err;

	err = tfa98xx_supported_dai(devidx, &daimap);
	assert (err==Tfa98xx_Error_Ok);

	if ( daimap & Tfa98xx_DAI_I2S )	{
		sprintf(str, " I2S"); 
		if(tfa_append_substring(str, buffer, maxlength) != Tfa98xx_Error_Ok)
			return err;
	}
	if ( daimap & Tfa98xx_DAI_TDM )	{
		sprintf(str, " TDM");
		if(tfa_append_substring(str, buffer, maxlength) != Tfa98xx_Error_Ok)
			return err;
	}
	if ( daimap & Tfa98xx_DAI_PDM ) {
		sprintf(str, " PDM");
		if(tfa_append_substring(str, buffer, maxlength) != Tfa98xx_Error_Ok)
			return err;
	}

	// coolflux ROM rev
	if(tfa98xx_dev_family(devidx) == 2) {
		err = tfa_dsp_cmd_id_write_read(devidx,MODULE_FRAMEWORK,FW_PAR_ID_GET_FEATURE_INFO, TFA98XX_MAXTAG, bytes);
		PRINT_ASSERT(err);

		if(bytes[0] == 1) {
			sprintf(str, ", MBDRC is supported");
			if(tfa_append_substring(str, buffer, maxlength) != Tfa98xx_Error_Ok)
				return err;
		}
		if(bytes[1] == 1) {
			sprintf(str, ", MBDRC is enabled");
			if(tfa_append_substring(str, buffer, maxlength) != Tfa98xx_Error_Ok)
				return err;
		}
	}
	/* feature bits */
	err = tfa98xx_supported_saam(devidx, &saam);
	if ( err == Tfa98xx_Error_Ok ) {
		if (saam==Tfa98xx_saam) {
			sprintf(str, " SAAM");
			if(tfa_append_substring(str, buffer, maxlength) != Tfa98xx_Error_Ok)
				return err;
		}
		/**/
		tfa98xx_dsp_support_drc(devidx, &status);
		sprintf(str, "%s", status ? ", DRC":"");
		if(tfa_append_substring(str, buffer, maxlength) != Tfa98xx_Error_Ok)
			return err;
	}

	/* -- */
	sprintf(str, "\n");
	if(tfa_append_substring(str, buffer, maxlength) != Tfa98xx_Error_Ok)
		return err;

	return err;
}

/*
 * return latest testnr
 */
int tfa98xxDiagGetLatest(void)
{
        return lastTest;
}

/*
 * return last error string
 */
char *tfa98xxDiagGetLastErrorString(void)
{
        return lastErrorString;
}

/*
 *read test of DEVID register
 *
 * function bypasses the TFA layer to prevent i2c access
 * in tfa98xx_open()
 */
int tfa_diag_register_read(int slave)
{
	Tfa98xx_handle_t handle;
	int result = 0;         // 1 is failure
	unsigned short value;
	TRACEIN;

	lastApiError = Tfa98xx_Open((unsigned char)(slave << 1), &handle);
	if (lastApiError != Tfa98xx_Error_Ok)
		return 1;
	if (reg_read(handle, 0x03, &value) ==0) {
		result = Tfa98xx_Error_Ok;
	}
	else {
		result = 1;     // non-0 if fail
		sprintf(lastErrorString, "can't find i2c slave 0x%0x", slave);
	}
	Tfa98xx_Close(handle);

	lastError = result;
	TRACEOUT;

	return result;
}

/**
 * write/read test of register
 * @param slave I2C slave under test
 * @return 0 passed
 * @return 1 no device found
 * @return 2 reg mismatch
 */
int _tfa_diag_register_write_read(int slave, unsigned char reg)
{
	Tfa98xx_handle_t handle;
	int result = 0;
	unsigned short testreg;

	TRACEIN;

	lastApiError = Tfa98xx_Open((unsigned char)(slave << 1), &handle);
	if (lastApiError != Tfa98xx_Error_Ok)
		return 1;

	// write 0x1234
	lastApiError = Tfa98xx_WriteRegister16(handle, reg, 0x1234);
	assert(lastApiError == Tfa98xx_Error_Ok);
	lastApiError = Tfa98xx_ReadRegister16(handle, reg, &testreg);
	assert(lastApiError == Tfa98xx_Error_Ok);

	if (0x1234 != testreg) {
		sprintf(lastErrorString, "read back value mismatch: (testreg=0x%02x), exp:0x%04x rcv:0x%04x\n",
				reg, 0x1234, testreg);
		result = 3;
	}

	Tfa98xx_Close(handle);

	lastError = result;
	TRACEOUT;
	return result;
}

int tfa_diag_register_write_read(int slave) {
	unsigned char reg;
	// register used in rw diag
	if (tfa98xx_dev_family(0) == 1)  //TODO device idx=0 always
		reg = TFA1_BF_MADD >> 8;
	else
		reg = TFA2_BF_MADD >> 8;

	return _tfa_diag_register_write_read(slave, reg);
}
int tfa_diag_register_write_read_72(int slave) {

	return _tfa_diag_register_write_read(slave, 0xee); //SWPROFIL NAME is conflicting with max2
}

int tfa_diag_clock_enable(int slave) {
	Tfa98xx_handle_t handle;
	int ready, result = 0, loop=50;

	TRACEIN;

	lastApiError = Tfa98xx_Open((unsigned char)(slave << 1), &handle);
	if (lastApiError != Tfa98xx_Error_Ok) {
		result = 1;
		goto stop;
	}
	/* execute test  :
	 *   - powerdown
	 *  - powerup
	 *  - wait for system stable */
    lastApiError = Tfa98xx_Init(handle);
    assert(lastApiError == Tfa98xx_Error_Ok);
    lastApiError = tfa98xx_powerdown(handle, 0);
    assert(lastApiError == Tfa98xx_Error_Ok);

    do {
    	lastApiError = Tfa98xx_DspSystemStable(handle, &ready);
    	assert(lastApiError == Tfa98xx_Error_Ok);
    	if (ready)
    		break;
    } while (loop--);

	/* report result */
	if (!ready) {
		sprintf(lastErrorString, "power-on timed out\n");
		result = 3;
	}

	stop: Tfa98xx_Close(handle);

	lastError = result;
	TRACEOUT;
	return result;
}
int tfa_diag_clock_enable_72(int slave) {
	Tfa98xx_handle_t handle;
	int ready, result = 0, loop=50;

	TRACEIN;

	lastApiError = Tfa98xx_Open((unsigned char)(slave << 1), &handle);
	if (lastApiError != Tfa98xx_Error_Ok) {
		result = 1;
		goto stop;
	}
	/* execute test  :
	 *   - powerdown
	 *  - powerup
	 *  - set MANSCONF
	 *  - wait for system stable */
    lastApiError = Tfa98xx_Init(handle);
    assert(lastApiError == Tfa98xx_Error_Ok);
    lastApiError = tfa98xx_powerdown(handle, 0);
    assert(lastApiError == Tfa98xx_Error_Ok);

    if (tfa98xxDiag_verbose)
    	printf("MANSTATE:%d\n", TFA_GET_BF(handle, MANSTATE ));
    /* we should be in wait for config now */
    TFA_SET_BF(handle, MANSCONF, 1);

    do {
        if (tfa98xxDiag_verbose)
        	printf("MANSTATE:%d, CLK:%d\n", TFA_GET_BF(handle, MANSTATE ),
        									TFA_GET_BF(handle, CLKS ));
    	lastApiError = Tfa98xx_DspSystemStable(handle, &ready);
    	assert(lastApiError == Tfa98xx_Error_Ok);
    	if (ready)
    		break;
    } while (loop--);

    if (tfa98xxDiag_verbose)
    	printf("MANSTATE:%d, CLK:%d\n", TFA_GET_BF(handle, MANSTATE ),
    									TFA_GET_BF(handle, CLKS ));

	/* report result */
	if (!ready) {
		sprintf(lastErrorString, "power-on timed out\n");
		result = 3;
	}

	stop: Tfa98xx_Close(handle);

	lastError = result;
	TRACEOUT;
	return result;
}

//	 * write/read test of  all xmem locations
int tfa_diag_xmem_access(int slave) {
	Tfa98xx_handle_t handle;
	int result = 0;      // 1 is failure
	int ready, i,data=0;
	TRACEIN;

	lastApiError = Tfa98xx_Open((unsigned char)(slave << 1), &handle);
	if (lastApiError != Tfa98xx_Error_Ok) {
		result = 1;
		goto stop;
	}
	/* execute test */
//	 * - ensure clock is on
	lastApiError = Tfa98xx_DspSystemStable(handle, &ready);
	assert(lastApiError == Tfa98xx_Error_Ok);
	if (!ready) {
		result = 2;
		goto stop;
	}
//	 * - put DSP in reset
	lastApiError = Tfa98xx_DspReset(handle, 1);
	assert(lastApiError == Tfa98xx_Error_Ok);
//	 * - write count into  xmem locations
	for(i=0;i <= TFA2_XMEM_MAX;i+=100) {
		lastApiError = mem_write(handle, (unsigned short)i, i, Tfa98xx_DMEM_XMEM);
		assert(lastApiError == Tfa98xx_Error_Ok);
	}
//	 * - verify this count by reading back xmem
	for(i=0;i <= TFA2_XMEM_MAX;i+=100) {
		lastApiError = mem_read(handle, i,1,&data);
		assert(lastApiError == Tfa98xx_Error_Ok);
		if (data != i) {
			result = 3;
			break;
		}
	}

	/* report result */
	if (result == 3) {
		sprintf(lastErrorString, "xmem expected read value mismatch; xmem[%d] exp:%d , actual %d",i, i, data);
	}

	stop: Tfa98xx_Close(handle);

	lastError = result;
	TRACEOUT;
	return result;
}

//	 * set ACS bit via iomem, read via status reg
int tfa_diag_ACS_via_iomem(int slave) {
	Tfa98xx_handle_t handle;
	int result = 0;      // 1 is failure
	int ready; // i,data;
	TRACEIN;

	lastApiError = Tfa98xx_Open((unsigned char)(slave << 1), &handle);
	if (lastApiError != Tfa98xx_Error_Ok) {
		result = 1;
		goto stop;
	}
	/* execute test */

//	 * - ensure clock is on
	lastApiError = Tfa98xx_DspSystemStable(handle, &ready);
	assert(lastApiError == Tfa98xx_Error_Ok);
	if (!ready) {
		result = 2;
		goto stop;
	}
//	 * - clear ACS via CF_CONTROLS
//	 * - check ACS is clear in status reg
	if (Tfa98xx_Error_Ok == tfaRunColdboot(handle, 0) ){
		//	 * - set ACS via CF_CONTROLS
		//	 * - check ACS is set in status reg
		if (Tfa98xx_Error_Ok != tfaRunColdboot(handle, 1) ){
			result = 3;
		}
	} else
		result = 3;

	/* report result */
	if (result==3) {
		sprintf(lastErrorString, "no control over ACS via iomem\n");

	}

	stop: Tfa98xx_Close(handle);

	lastError = result;
	TRACEOUT;
	return result;
}

/*
 * trusted functions:
 *  target assumed open
 *  no error checking for register io
 */
enum Tfa98xx_Error tfa98xx_irq_clear(Tfa98xx_handle_t handle, enum tfa9912_irq bit) {
	int status = tfa_irq_clear(handle , bit);
	return status != -1  ? Tfa98xx_Error_Ok : Tfa98xx_Error_Bad_Parameter;
}
enum Tfa98xx_Error tfa98xx_irq_ena(Tfa98xx_handle_t handle, enum tfa9912_irq bit, int state) {
	int status = tfa_irq_ena(handle, bit, state);
	return status != -1  ? Tfa98xx_Error_Ok : Tfa98xx_Error_Bad_Parameter;
}
/*
 * return state of irq or -1 if illegal bit
 */
int tfa98xx_irq_get(Tfa98xx_handle_t handle, enum tfa9912_irq bit){
	return tfa_irq_get(handle, bit);
}

int tfa_diag_irq_cold(int slave) {
	Tfa98xx_handle_t handle;
	int result = 0;      // 1 is failure
	int ready, i; //data;

	TRACEIN;

	lastApiError = Tfa98xx_Open((unsigned char)(slave << 1), &handle);
	if (lastApiError != Tfa98xx_Error_Ok) {
		result = 1;
		goto stop;
	}
	/* execute test */

	// power on (with profile 0)
	lastApiError = tfaRunStartup(handle, 0);
	assert(lastApiError == Tfa98xx_Error_Ok);
	// set to bypass, is enough for 97 N1A to enable irq
	TFA_SET_BF(handle, CFE,0);
//	 * - ensure clock is on
	lastApiError = Tfa98xx_DspSystemStable(handle, &ready);
	assert(lastApiError == Tfa98xx_Error_Ok);
	if (!ready) {
		result = 2;
		goto stop;
	}

	//	 * - put DSP in reset
	lastApiError = Tfa98xx_DspReset(handle, 1);
	assert(lastApiError == Tfa98xx_Error_Ok);

	/* disable all irq's */
	tfa98xx_irq_ena(handle, tfa2_irq_all, 0);
	/* clear all */
	tfa98xx_irq_clear(handle, tfa2_irq_all);
	/* check for clear */
	for(i=0;i<tfa2_irq_max;i++) {
		result = tfa98xx_irq_get(handle, i);
		assert (result!=-1);
		if(result) {
			PRINT_ERROR("%s: interrupt bit %d did not clear\n", __FUNCTION__, i);
		}
	}
	/* report result not cleared */
	if (result) {
		result = 3;
		sprintf(lastErrorString, "interrupt bit(s) did not clear\n");

	}

	stop:
	lastApiError = Tfa98xx_DspReset(handle, 0) /* release DSP */;
	assert(lastApiError == Tfa98xx_Error_Ok);

	Tfa98xx_Close(handle);

	lastError = result;
	TRACEOUT;
	return result;
}

int tfa_diag_irq_warm(int slave) {
	Tfa98xx_handle_t handle;
	int result = 0;      // 1 is failure

	TRACEIN;

	lastApiError = Tfa98xx_Open((unsigned char)(slave << 1), &handle);
	if (lastApiError != Tfa98xx_Error_Ok) {
		result = 1;
	}

	lastApiError = Tfa98xx_DspReset(handle, 0) /* release DSP */;
	assert(lastApiError == Tfa98xx_Error_Ok);

	Tfa98xx_Close(handle);

	lastError = result;
	TRACEOUT;
	return result;
}

#define XMEM_STRIDE 100 // shorten test time
//	 * write xmem, read with i2c burst
int tfa_diag_xmem_burst_read(int slave) {
	Tfa98xx_handle_t handle;
	int result = 0;      // 1 is failure
	int i, ready, data[ 3*TFA2_XMEM_MAX + 4]; // test buffer
	TRACEIN;

	lastApiError = Tfa98xx_Open((unsigned char)(slave << 1), &handle);
	if (lastApiError != Tfa98xx_Error_Ok) {
		result = 1;
		goto stop;
	}
	/* execute test */
	//	 * - ensure clock is on
	lastApiError = Tfa98xx_DspSystemStable(handle, &ready);
	assert(lastApiError == Tfa98xx_Error_Ok);
	if (!ready) {
		result = 2;
		goto stop;
	}
	//	 * - put DSP in reset
	lastApiError = Tfa98xx_DspReset(handle, 1);
	assert(lastApiError == Tfa98xx_Error_Ok);
	//	 * - write testpattern
	for (i = 0; i < TFA2_XMEM_MAX; i += XMEM_STRIDE) {
		lastApiError = mem_write(handle, (unsigned short)i, i, Tfa98xx_DMEM_XMEM);
		assert(lastApiError == Tfa98xx_Error_Ok);
	}
	//	 * - burst read testpattern
	lastApiError = mem_read(handle, 0, TFA2_XMEM_MAX, data);
	assert(lastApiError == Tfa98xx_Error_Ok);
	//	 * - verify data
	for (i = 0; i < TFA2_XMEM_MAX; i += XMEM_STRIDE) {
		if (i != data[i]) {
			result = 3;
			break;
		}
	}
	/* report result */
	if (result == 3) {
		sprintf(lastErrorString, "xmem expected read value mismatch\n");
	}

	stop: Tfa98xx_Close(handle);

	lastError = result;
	TRACEOUT;
	return result;
}

// write/read full xmem in i2c burst
int tfa_diag_xmem_burst_write(int slave) {
	Tfa98xx_handle_t handle;
	int result = 0;      // 1 is failure
	int i, ready;
	int worddata[TFA2_XMEM_MAX + 4]; // test buffers
	unsigned char bytedata[sizeof(worddata)*3];

	TRACEIN;

	lastApiError = Tfa98xx_Open((unsigned char)(slave << 1), &handle);
	if (lastApiError != Tfa98xx_Error_Ok) {
		result = 1;
		goto stop;
	}
	/* execute test */
	//	 * - ensure clock is on
	lastApiError = Tfa98xx_DspSystemStable(handle, &ready);
	assert(lastApiError == Tfa98xx_Error_Ok);
	if (!ready) {
		result = 2;
		goto stop;
	}
	//	 * - put DSP in reset
	lastApiError = Tfa98xx_DspReset(handle, 1);
	assert(lastApiError == Tfa98xx_Error_Ok);
	//	 * - create & burst write testpattern
	for (i = 0; i < TFA2_XMEM_MAX; i ++) {
		worddata[i]=i;
	}
	tfa98xx_convert_data2bytes(TFA2_XMEM_MAX, worddata, bytedata);

	lastApiError = tfa98xx_dsp_mem_write(handle , Tfa98xx_DMEM_XMEM, 0, TFA2_XMEM_MAX*3, bytedata);
	for (i = 0; i < TFA2_XMEM_MAX; i ++)
		worddata[i]=0xdeadbeef; // clean read buffer
	// * - burst read testpattern
	lastApiError = mem_read(handle, 0, TFA2_XMEM_MAX, worddata);
	assert(lastApiError == Tfa98xx_Error_Ok);
	//	 * - verify data
	for (i = 0; i < TFA2_XMEM_MAX; i++) {
		if (i != worddata[i]) {
			result = 3;
			break;
		}
	}
	/* report result */
	if (result == 3) {
		sprintf(lastErrorString, "xmem expected read value mismatch\n");
	}

	stop: Tfa98xx_Close(handle);

	lastError = result;
	TRACEOUT;
	return result;
}

/*
 * load a patch and verify patch version number
 */
int tfa_diag_patch_load(int slave) {
	Tfa98xx_handle_t handle;
	int ready, result = 0;      // 1 is failure
	int xm_version, idx, data;

	TRACEIN;

	lastApiError = Tfa98xx_Open((unsigned char)(slave << 1), &handle);
	if (lastApiError != Tfa98xx_Error_Ok) {
		result = 1;
		goto stop;
	}
	/* execute test */
	//	 * - ensure clock is on
	// reset i2c
	lastApiError = TFA_SET_BF(handle, I2CR, 1) < 0 ? ~lastApiError : Tfa98xx_Error_Ok;
	assert(lastApiError == Tfa98xx_Error_Ok);


	if (tfa98xx_dev_family(handle) == 2) {
		if (TFA_GET_BF(handle, NOCLK)) {
			PRINT("Using internal clock\n");
			TFA_SET_BF(handle, MANCOLD, 1);
		}
	}

	// power on (with profile 0)
	lastApiError = tfaRunStartup(handle, 0);
	assert(lastApiError == Tfa98xx_Error_Ok);
	lastApiError = Tfa98xx_DspSystemStable(handle, &ready);

	// report as error
	assert(lastApiError == Tfa98xx_Error_Ok);
	if (!ready) {
		result = 2;
		goto stop;
	}
	// set cold boot flag
	lastApiError = tfaRunColdboot(handle, 1);
	assert(lastApiError == Tfa98xx_Error_Ok);

	if ((tfa98xx_dev_revision(handle) & 0xff) == 0x97)
		xm_version = TFA9897_XMEM_PATCHVERSION;
	else if ((tfa98xx_dev_revision(handle) & 0xff) == 0x96)
		xm_version = TFA9896_XMEM_PATCHVERSION;
	else if ((tfa98xx_dev_revision(handle) & 0xff) == 0x92)
		xm_version = TFA9891_XMEM_PATCHVERSION;
	else if ((tfa98xx_dev_revision(handle) & 0xff) == 0x13)
		xm_version = TFA9912_XMEM_PATCHVERSION;
	else
		xm_version = TFA_FAM_XMEM(handle,PATCHVERSION);

	/* lookup slave for device index */
	idx = tfa98xx_cnt_slave2idx(slave);
	if (idx<0) {
		result = -1;
		goto stop;
	}

//	 * - clear xmem patch version storage
	lastApiError = mem_write(handle, (unsigned short)xm_version, 0x123456, Tfa98xx_DMEM_XMEM);
	assert(lastApiError == Tfa98xx_Error_Ok);
//	 * - load patch from container
	lastApiError = tfaContWritePatch(idx);
	assert(lastApiError == Tfa98xx_Error_Ok);

//	 * - check xmem patch version storage
	lastApiError = mem_read(handle, xm_version, 1, &data);
	assert(lastApiError == Tfa98xx_Error_Ok);

	/* report result */
	if ( data == 0 || data == 0x123456 ) {
		sprintf(lastErrorString, "patch version not updated");
                result = 3;
	} else {
                printf("The patch version data read back is 0x%x\n", data);
        }
	stop: Tfa98xx_Close(handle);

	lastError = result;
	TRACEOUT;
	return result;
}

int tfa_diag_dsp_reset(int slave) {
	Tfa98xx_handle_t handle;
	int result = 0;      // 1 is failure
	int data, ready;

	TRACEIN;

	lastApiError = Tfa98xx_Open((unsigned char)(slave << 1), &handle);
	if (lastApiError != Tfa98xx_Error_Ok) {
		result = 1;
		goto stop;
	}
	/* execute test */
	//	 * - ensure clock is on
	lastApiError = Tfa98xx_DspSystemStable(handle, &ready);
	assert(lastApiError == Tfa98xx_Error_Ok);
	if (!ready) {
		result = 2;
		goto stop;
	}
	//	 * - put DSP in reset
	lastApiError = Tfa98xx_DspReset(handle, 1);
	assert(lastApiError == Tfa98xx_Error_Ok);
//	 * - clear count_boot
	lastApiError = tfaRunResetCountClear(handle);
	assert(lastApiError == Tfa98xx_Error_Ok);
//	 * - release  DSP reset
	lastApiError = Tfa98xx_DspReset(handle, 0);
	assert(lastApiError == Tfa98xx_Error_Ok);
//	 * - verify that count_boot incremented
	data = tfaRunResetCount(handle);
	/* report result */
	if (data != 1) {
		sprintf(lastErrorString, "no proper DSP response to RST\n");
		result = 3;
	}

	stop: Tfa98xx_Close(handle);

	lastError = result;
	TRACEOUT;
	return result;
}

int tfa_diag_start_speakerboost(int slave) {
	Tfa98xx_handle_t handle;
	int ready, result = 0;      // 1 is failure
	unsigned short revid=0;

	TRACEIN;

	lastApiError = Tfa98xx_Open((unsigned char)(slave << 1), &handle);
	if (lastApiError != Tfa98xx_Error_Ok) {
		result = 1;
		goto stop;
	}

	/* execute test */
	//	 * - ensure clock is on
	lastApiError = Tfa98xx_DspSystemStable(handle, &ready);
	assert(lastApiError == Tfa98xx_Error_Ok);
	if (!ready) {
		result = 2;
		goto stop;
	}
//	 * - start with forced coldflag
	//TODO deal with other device handle
	lastApiError = tfaRunSpeakerBoost(handle, 1, 0); /* force coldstart (with profile 0) */
	assert(lastApiError == Tfa98xx_Error_Ok);
//	 * - verify that the ACS flag cleared

	revid = tfa98xx_get_device_revision(handle);
	if((revid & 0xff) == 0x72) {
		/* report result */
		if (TFA_GET_BF(handle, MANSTATE) != 9) {
			sprintf(lastErrorString, "DSP not properly started \n");
			result = 3;
		}
	} else {
		/* report result */
		if (TFA_GET_BF(handle, ACS) != 0) {
			sprintf(lastErrorString, "DSP did not clear ACS");
			result = 3;
		}
	}

	stop: Tfa98xx_Close(handle);

	lastError = result;
	TRACEOUT;
	return result;
}
#define FW_CMDID( MODULE, FW_PAR_ID ) ( ((MODULE)|0x80) << 8 | (FW_PAR_ID)<<16 )
int tfa_diag_read_version_tag(int slave) {
	Tfa98xx_handle_t handle;
	int ready, result = 0;      // 1 is failure
	unsigned char tag[TFA98XX_MAXTAG];

	TRACEIN;

	lastApiError = Tfa98xx_Open((unsigned char)(slave << 1), &handle);
	if (lastApiError != Tfa98xx_Error_Ok) {
		result = 1;
		goto stop;
	}

	if ( (NXP_I2C_Interface_Type() & NXP_I2C_Msg) == 0 ) /* via i2c */ {
		//	 * - ensure clock is on
		lastApiError = Tfa98xx_DspSystemStable(handle, &ready);
		if (lastApiError != Tfa98xx_Error_Ok || !ready) {
			result = 2;
			goto stop;
		}
		lastApiError = tfa_dsp_cmd_id_write_read(handle,MODULE_FRAMEWORK,FW_PAR_ID_GET_TAG, TFA98XX_MAXTAG, tag);
		if (lastApiError != Tfa98xx_Error_Ok) {
			result = 3;
			goto stop;
		}
	} else { /* via msg hal */
		uint32_t cmd_id = FW_CMDID(MODULE_FRAMEWORK,FW_PAR_ID_GET_TAG);//(MODULE_FRAMEWORK|0x80) << 8 | FW_PAR_ID_GET_TAG;
		result = NXP_TFADSP_Execute( sizeof(cmd_id), &cmd_id,  TFA98XX_MAXTAG, tag);
	}

	result=0;
	PRINT("Read ROMID TAG is \"");
	for (ready = 2; ready <TFA98XX_MAXTAG; ready+=3) {
		if ( isprint(tag[ready]) ) {
			PRINT("%c", tag[ready]);
			if ( tag[ready] == '>') {
				result=0;
				break;
			}
		}
	}
	PRINT("\"\n");

	stop: Tfa98xx_Close(handle);

	lastError = result;
	TRACEOUT;
	return result;
}
int tfa_diag_read_api_version(int slave) {
	Tfa98xx_handle_t handle;
	int ready, result = 0;      // 1 is failure
	unsigned char result_buf[3*3]; //need 1 word more?

	TRACEIN;

	lastApiError = Tfa98xx_Open((unsigned char)(slave << 1), &handle);
	if (lastApiError != Tfa98xx_Error_Ok) {
		result = 1;
		goto stop;
	}

	if ( (NXP_I2C_Interface_Type() & NXP_I2C_Msg) == 0 ) /* via i2c */ {
		//	 * - ensure clock is on
		lastApiError = Tfa98xx_DspSystemStable(handle, &ready);
		if (lastApiError != Tfa98xx_Error_Ok || !ready) {
			result = 2;
			goto stop;
		}
		lastApiError = tfa_dsp_cmd_id_write_read(handle,MODULE_FRAMEWORK,FW_PAR_ID_GET_API_VERSION,  sizeof(result_buf), result_buf);
		if (lastApiError != Tfa98xx_Error_Ok) {
			result = 3;
			goto stop;
		}
	} 		/* 72 device */
	else if((tfa98xx_dev_revision(handle) & 0xff ) == 0x72) {
		lastApiError = tfa_dsp_cmd_id_write_read(handle,MODULE_FRAMEWORK,FW_PAR_ID_GET_API_VERSION,3, result_buf);
		if (lastApiError) {
			PRINT("%s Error: Unable to get API Version from DSP \n", __FUNCTION__);
			return lastApiError;
		}
		}
	else { /* via msg hal */
		uint32_t cmd_id = FW_CMDID(MODULE_FRAMEWORK,FW_PAR_ID_GET_API_VERSION);
		result = NXP_TFADSP_Execute( sizeof(cmd_id), &cmd_id,  sizeof(result_buf), result_buf);
	}

	if ( result ) {
		PRINT("ERROR return from API version call: %d\n", result);
	} else
		PRINT("API version: %d.%d.%d\n",result_buf[5] , result_buf[4] ,   result_buf[3] ); //MSB ok?

	stop: Tfa98xx_Close(handle);

	lastError = result;
	TRACEOUT;
	return result;
}

static int tfa1_diag_verify_parameters(Tfa98xx_handle_t handle) {
	int i, size, ready, result = 0;      // 1 is failure
	unsigned char buffer[2048]; /* biggest size */
	const unsigned char *srcdata;
	nxpTfaSpeakerFile_t *spkr;
	nxpTfaConfig_t *cfg;
	nxpTfaPreset_t *preset;
	nxpTfaVolumeStep2File_t *vstep;

	TRACEIN;

	/* execute test */
	//	 * - ensure clock is on
	lastApiError = tfa98xx_dsp_system_stable(handle, &ready);

	assert(lastApiError == Tfa98xx_Error_Ok);

	if (!ready) {
		result = 2;
		goto stop;
	}

//	 * - assure ACS is set
	if (TFA_GET_BF(handle, ACS) != 0) {
		sprintf(lastErrorString, "DSP did not clear ACS");
		result = 3;
	}

//	 * - verify that SB parameters are correctly loaded

	/************* speaker parameters ************************/
	lastApiError = Tfa98xx_DspReadSpeakerParameters(handle,
			TFA1_SPEAKERPARAMETER_LENGTH, buffer);

	assert(lastApiError == Tfa98xx_Error_Ok);

	/* find the speaker params for this device */
	spkr = (nxpTfaSpeakerFile_t *)tfacont_getfiledata(handle, 0, speakerHdr);

	if (spkr==0) {
		result = -1;
		goto stop;
	}

	srcdata = spkr->data; /*payload*/

	/* fix potentially modified params */
	for(i=0;i<3*128;i++) /* FIR is 128 words */
		buffer[i] = srcdata[i];
	//fRes =136
	for(i=135*3;i<136*3;i++) /* 1 words */
		buffer[i] = srcdata[i];
	// tCoef=141
//	for(i=140*3;i<141*3;i++) /* 1 word */
//		buffer[i] = srcdata[i];
	if (tfa98xx_get_device_revision(handle)==0x92) {
		PRINT("Skipping speaker data compare for tfa9891\n");
		for(i=129*3;i<141*3;i++) /* many words */
			buffer[i] = srcdata[i];
	}

	/* compare */
	for (i=0;i<TFA1_SPEAKERPARAMETER_LENGTH;i++) {
		if (srcdata[i] != buffer[i]) {
			sprintf(lastErrorString, "speaker parameters error;offset=%d(%d) exp:0x%02x act:0x%02x ",
					i,i/3,srcdata[i],buffer[i]);
			result = 4;
			goto stop;
		}
	}

	/************* config parameters ************************/
	tfa98xx_dsp_config_parameter_count(handle, &size);
	size *=3; //need bytes
	lastApiError = tfa98xx_dsp_read_config(handle, size, buffer);

	assert(lastApiError == Tfa98xx_Error_Ok);
	/* find the speaker params for this device */
	cfg = (nxpTfaConfig_t *)tfacont_getfiledata(handle, 0, configHdr);

	if (cfg==0) {
		result = -1;
		goto stop;
	}
	srcdata = cfg->data; /*payload*/
	/* fix potentially modified params */
	buffer[78] = srcdata[78];
	buffer[79] = srcdata[79];
	buffer[80] = srcdata[80];

	if (memcmp(srcdata, buffer, size)!=0) {
		sprintf(lastErrorString, "config parameters error");
		result = 5;
		goto stop;
	}
	/************* preset parameters ************************/
	/* assume that  profile 0 and vstep 0 are loaded */
	lastApiError = tfa98xx_dsp_read_preset(handle, TFA98XX_PRESET_LENGTH, buffer);
	assert(lastApiError == Tfa98xx_Error_Ok);

	/* find the speaker params for this device */
	preset = (nxpTfaPreset_t *)tfacont_getfiledata(handle, 0, presetHdr);

	if (preset==0) {
		/* maybe we have a vstep here */
		vstep = (nxpTfaVolumeStep2File_t *)tfacont_getfiledata(handle, 0, volstepHdr);

		if (vstep==0) {
			result = -1; /* no vstep either */
			goto stop;
		}

		srcdata = vstep->vstep[0].preset; /*payload for step 0*/
	} else {
		srcdata = preset->data; /*payload*/
	}

	if (memcmp(srcdata, buffer, TFA98XX_PRESET_LENGTH)!=0) {
		sprintf(lastErrorString, "preset parameters error");
		result = 6;
		goto stop;
	}

	stop: Tfa98xx_Close(handle);

	lastError = result;
	TRACEOUT;
	return result;
}

static int tfa2_softdsp_diag_verify_parameters(Tfa98xx_handle_t handle) 
{
	int result = 0;      // 1 is failure
	unsigned char buffer[TFA72_ALGOPARAMETER_LENGTH_STEREO] = {0};
	unsigned char sourcedata[TFA72_ALGOPARAMETER_LENGTH_STEREO] = {0};
	const unsigned char *srcdata;
	nxpTfaVolumeStepMax2File_t *vstep;
	nxpTfaProfileList_t *prof;
	nxpTfaBitfield_t *bitF;
	nxpTfaContainer_t * cont = tfa98xx_get_cnt();
	int status, i, j=0, regvalue;
	unsigned int k;

	TRACEIN;

	/* verify that SB parameters are correctly loaded */

	/************* vstep parameters ************************/
	/* check for vstep in the container file */
	vstep = (nxpTfaVolumeStepMax2File_t *)tfacont_getfiledata(handle, 0, volstepHdr);

	/* no vstep found */
	if (vstep==0) {
		printf("Warning: No vstep file found! Skipping test for now! \n");
		result = 0;
		goto stop;
	}
	/* Stereo */
	if(tfa98xx_cnt_max_device() > 1) {
		/* assume that  profile 0 and vstep 0 are loaded */
		    lastApiError = tfa_dsp_cmd_id_write_read(handle,MODULE_SPEAKERBOOST,
		            SB_PARAM_GET_ALGO_PARAMS, TFA72_ALGOPARAMETER_LENGTH_STEREO, buffer);
	} else { /* Mono */
		/* assume that  profile 0 and vstep 0 are loaded */
		    lastApiError = tfa_dsp_cmd_id_write_read(handle,MODULE_SPEAKERBOOST,
		            SB_PARAM_GET_ALGO_PARAMS, TFA72_ALGOPARAMETER_LENGTH_MONO, buffer);
	}

    assert(lastApiError == Tfa98xx_Error_Ok);

	/* Get the vstep payload data from vstep 0 */
	srcdata = vstep->vstepsBin;

	/* Get the number of registers */
	regvalue = srcdata[0];


	/* Stereo */
	if(tfa98xx_cnt_max_device() > 1) {
		/* Algoparams data starts after 9 bytes:
		 * (nrofregisters(1), nrofMessages(1), Messagetype(1), messagelength(3) and the cmdid(3)) +
		 * the number of register * 4 (register and value)
		 */
		for(i=0; i<TFA72_ALGOPARAMETER_LENGTH_STEREO; i++)
					sourcedata[i] = srcdata[i+((regvalue*4)+9)];

		/* Skip the bytes that are not equal */
		for(i=0; i<TFA72_ALGOPARAMETER_LENGTH_STEREO; i++) {
				if(buffer[i] != sourcedata[i]) {
						j++;
						buffer[i] = sourcedata[i];
				}
		}

		if((j > 0) && (tfa98xxDiag_verbose)) /* Print the percentage of mismatches */
			PRINT("Number of Algoparams mismatches between vstep and device = %d (%d%%) \n", j, (TFA72_ALGOPARAMETER_LENGTH_STEREO/100) / j);

			/* Compare algoparams, and check if not more than 10% is not equal */
		if ((memcmp(sourcedata, buffer, TFA72_ALGOPARAMETER_LENGTH_STEREO)!=0) || (j > (TFA72_ALGOPARAMETER_LENGTH_STEREO / 10))) {
			sprintf(lastErrorString, "Algoparams error");
			result = 6;
			goto stop;
		}
	} else { /* Mono */
		/* Algoparams data starts after 9 bytes:
		 * (nrofregisters(1), nrofMessages(1), Messagetype(1), messagelength(3) and the cmdid(3)) +
		 * the number of register * 4 (register and value)
		 */
		for(i=0; i<TFA72_ALGOPARAMETER_LENGTH_MONO; i++)
					sourcedata[i] = srcdata[i+((regvalue*4)+9)];

		/* Skip the bytes that are not equal */
		for(i=0; i<TFA72_ALGOPARAMETER_LENGTH_MONO; i++) {
				if(buffer[i] != sourcedata[i]) {
						j++;
						buffer[i] = sourcedata[i];
				}
		}

		if((j > 0) && (tfa98xxDiag_verbose)) /* Print the percentage of mismatches */
			PRINT("Number of Algoparams mismatches between vstep and device = %d (%d%%) \n", j, (TFA72_ALGOPARAMETER_LENGTH_MONO/100) / j);

			/* Compare algoparams, and check if not more than 10% is not equal */
		if ((memcmp(sourcedata, buffer, TFA72_ALGOPARAMETER_LENGTH_MONO)!=0) || (j > (TFA72_ALGOPARAMETER_LENGTH_MONO / 10))) {
			sprintf(lastErrorString, "Algoparams error");
			result = 6;
			goto stop;
		}
	}

	/************* bitfield settings ****************/
	/* Get information of profile 0 */
	prof = tfaContProfile(handle, 0);

	if ( prof->ID != TFA_PROFID ) {
		result = 7;
		goto stop;
	}
	for(k=0;k<prof->length-1u;k++) {
			/* Only check values before default */
			if(prof->list[k].type == dscDefault)
					break;

			/* Check all bitfield settings */
	if ( prof->list[k].type != dscFile ) {
					/* Get the bitfield name + value */
					bitF = (nxpTfaBitfield_t *)(prof->list[k].offset+(uint8_t *)cont);

					/* Remember the original value */
					status = bitF->value;

					/* Read the bitfield value*/
					tfaRunReadBitfield(handle, bitF);

					/* Make sure the profile status and read back bitfield are equal */
					if(status != bitF->value) {
							sprintf(lastErrorString, "%s is not set",
			tfaContBfName(bitF->field, tfa98xx_dev_revision(handle)));
					result = 6;
					goto stop;
				}
			}
	}

	stop: Tfa98xx_Close(handle);

	lastError = result;
	TRACEOUT;
	return result;
}

static int tfa2_diag_verify_parameters(Tfa98xx_handle_t handle) {

	int ready, result = 0;      // 1 is failure
        unsigned char buffer[TFA2_ALGOPARAMETER_LENGTH * 2] = {0};
        unsigned char sourcedata[TFA2_ALGOPARAMETER_LENGTH * 2] = {0};
        const unsigned char *srcdata;
	nxpTfaVolumeStepMax2File_t *vstep;
        nxpTfaProfileList_t *prof;
	nxpTfaBitfield_t *bitF;
        nxpTfaContainer_t * cont = tfa98xx_get_cnt();
        int status, i, j=0, regvalue;
        unsigned int k;
	nxp_vstep_msg_t *nxp_vstep_msg;
	int filter_coef_length, algo_param_length;
	TRACEIN;

	/* execute test */
	//	 * - ensure clock is on
	lastApiError = Tfa98xx_DspSystemStable(handle, &ready);

	assert(lastApiError == Tfa98xx_Error_Ok);

	if (!ready) {
		result = 2;
		goto stop;
	}

//	 * - assure ACS is set
	if (TFA_GET_BF(handle, ACS) != 0)
	{
		result = 3;
		sprintf(lastErrorString, "DSP is not configured");
		goto stop;
	}

	nxp_vstep_msg = tfa_vstep_info(handle);
	if (nxp_vstep_msg==NULL) {
			filter_coef_length = (int )TFA2_FILTERCOEFSPARAMETER_LENGTH;
			algo_param_length = (int) TFA2_ALGOPARAMETER_LENGTH;
	} else {
		filter_coef_length = (int )nxp_vstep_msg->filter_coef_length;
		algo_param_length = (int) nxp_vstep_msg->algo_param_length;
	}
	/* verify that SB parameters are correctly loaded */

        /************* speaker parameters ************************/

        /* Note: For now speaker file change to much to have a decent compare */

        /************* vstep parameters ************************/
        /* assume that  profile 0 and vstep 0 are loaded */
        lastApiError = tfa_dsp_cmd_id_write_read(handle,MODULE_SPEAKERBOOST,
			SB_PARAM_GET_ALGO_PARAMS, algo_param_length, buffer);

        assert(lastApiError == Tfa98xx_Error_Ok);
	
	/* check for vstep in the container file */
	vstep = (nxpTfaVolumeStepMax2File_t *)tfacont_getfiledata(handle, 0, volstepHdr);

        /* no vstep found */
	if (vstep==0) {
		result = -1; 
		goto stop;
	}
	
        /* Get the vstep payload data from vstep 0 */
        srcdata = vstep->vstepsBin;

	/* Get the number of registers */
	regvalue = srcdata[0];

	/* Algoparams data starts after 9 bytes:
	 * (nrofregisters(1), nrofMessages(1), Messagetype(1), messagelength(3) and the cmdid(3)) +
	 * the number of register * 4 (register and value)
         * We want to copy the whole vstep file. This means algoparams + filtercoeffs 
	 */
	for(i=0; i<(algo_param_length+filter_coef_length); i++)
                sourcedata[i] = srcdata[i+((regvalue*4)+9)];

        /* Skip the bytes that are not equal */
        for(i=0; i<algo_param_length; i++) {
                if(buffer[i] != sourcedata[i]) {
                        j++;
                        buffer[i] = sourcedata[i];
                }
        }

	if((j > 0) && (tfa98xxDiag_verbose)) /* Print the percentage of mismatches */
		PRINT("Number of Algoparams mismatches between vstep and device = %d (%d%%) \n", j, ((j*100)/algo_param_length));

        /* Compare algoparams, and check if not more than 10% is not equal */
	if ((memcmp(sourcedata, buffer, algo_param_length)!=0) || (j > (algo_param_length / 10))) {
		sprintf(lastErrorString, "Algoparams error");
		result = 6;
		goto stop;
	}

        /* assume that profile 0 and vstep 0 are loaded */
	lastApiError = tfa_dsp_cmd_id_coefs(handle,MODULE_BIQUADFILTERBANK,BFB_PAR_ID_GET_COEFS, filter_coef_length, buffer);
        assert(lastApiError == Tfa98xx_Error_Ok);

        /* Skip the algo param bytes and the first 10 bytes (type/length/cmdid) */
        for(i=0; i<filter_coef_length; i++)
			sourcedata[i] = sourcedata[i+algo_param_length+10];

        j=0;
        /* Skip the bytes that are not equal */
        for(i=0; i<filter_coef_length; i++) {
                if(buffer[i] != sourcedata[i]) {
                        j++;
                        buffer[i] = sourcedata[i];
                }
        }

	if((j > 0) && (tfa98xxDiag_verbose)) /* Print the percentage of mismatches */
		PRINT("Number of Coeffs mismatches between vstep and device = %d (%d%%) \n", j, ((j*100)/filter_coef_length/100));

        /* Compare getCoefs, and check if not more than 10% is not equal */
	if ((memcmp(sourcedata, buffer, filter_coef_length)!=0) || (j > (filter_coef_length / 10))) {
		sprintf(lastErrorString, "Getcoeffs error");
		result = 6;
		goto stop;
	}

        /************* bitfield settings ****************/
        /* Get information of profile 0 */
        prof = tfaContProfile(handle, 0);

        if ( prof->ID != TFA_PROFID ) {
        	result = 7;
        	goto stop;
        }
        for(k=0;k<prof->length-1u;k++) { 
                /* Only check values before default */
                if(prof->list[k].type == dscDefault)
                        break;

                /* Check all bitfield settings */
		if ( prof->list[k].type != dscFile ) {
                        /* Get the bitfield name + value */
                        bitF = (nxpTfaBitfield_t *)(prof->list[k].offset+(uint8_t *)cont);

                        /* Remember the original value */
                        status = bitF->value;

                        /* Read the bitfield value*/
                        tfaRunReadBitfield(handle, bitF);

                        /* Make sure the profile status and read back bitfield are equal */
                        if(status != bitF->value) {
                                sprintf(lastErrorString, "%s is not set", 
				tfaContBfName(bitF->field, tfa98xx_dev_revision(handle)));
		                result = 6;
		                goto stop;
	                }
                }
        }

	stop: Tfa98xx_Close(handle);

	lastError = result;
	TRACEOUT;
	return result;
}

int tfa_diag_verify_parameters(int slave) {
	Tfa98xx_handle_t handle;
	int result;
	unsigned short revid=0;

	lastApiError = Tfa98xx_Open((unsigned char)(slave << 1), &handle);
	if (lastApiError != Tfa98xx_Error_Ok) {
		result = 1;
		return result;
	}

	revid = tfa98xx_get_device_revision(handle);
	if((revid & 0xff) == 0x72) {
		result = tfa2_softdsp_diag_verify_parameters(handle); //will call close
	} else if (tfa98xx_dev_family(handle)==2) { //tfa2
		result = tfa2_diag_verify_parameters(handle); //will call close
	} else
		result = tfa1_diag_verify_parameters(handle); //will call close

	return result;

}

int tfa_diag_verify_features(int slave) {
	Tfa98xx_handle_t handle;
	Tfa98xx_Error_t err = Tfa98xx_Error_Ok;
	int features_from_MTP[3], features_from_cnt[3], result=0, devidx=0;

	lastApiError = Tfa98xx_Open((unsigned char)(slave << 1), &handle);

	if (lastApiError != Tfa98xx_Error_Ok) {
		result = 1;
		return result;
	}

	devidx = tfa98xx_cnt_slave2idx(slave);

	if(devidx < 0)
		return -1;

	err = tfa98xx_compare_features(devidx, features_from_MTP, features_from_cnt);
	if(err==Tfa98xx_Error_Ok && memcmp(features_from_MTP, features_from_cnt, sizeof(features_from_MTP)) != 0) {
		if((features_from_cnt[0]==-1) || (features_from_cnt[1]==-1) || (features_from_cnt[2]==-1)) {
			PRINT("features keyword not fully present in container file\n");
			PRINT("Skip comparing features from MTP with features from cnt.\n");
		}
		else {
			PRINT("features from MTP [0x%x, 0x%x, 0x%x] don't match features from container file [0x%x, 0x%x, 0x%x].\n", 
				features_from_MTP[0], features_from_MTP[1], features_from_MTP[2],
				features_from_cnt[0], features_from_cnt[1], features_from_cnt[2]);
			result = -1;
		}
	}

	Tfa98xx_Close(handle);

	return result;
}

int tfa_diag_calibrate_always(int slave) {
	Tfa98xx_handle_t handle;
	int cal_prof, ready, result = 0;      // 1 is failure
	Tfa98xx_handle_t handlesIn[4]={-1,-1,-1,-1}; // TODO fix interface

	TRACEIN;

	lastApiError = Tfa98xx_Open((unsigned char)(slave << 1), &handle);
	if (lastApiError != Tfa98xx_Error_Ok) {
		result = 1;
		goto stop;
	}
	handlesIn[0]=handle;

	/* execute test */
	//	 * - ensure clock is on
	lastApiError = Tfa98xx_DspSystemStable(handle, &ready);
	assert(lastApiError == Tfa98xx_Error_Ok);
	if (!ready) {
		result = 2;
		goto stop;
	}

	/* When MTPOTC is set (cal=once) then we should not run this test! */
	if (TFA_GET_BF(handle, MTPOTC) == 1) {
		PRINT("Calibrate always test is skipped, MTPOTC is set to 1 (once)!\n");
		goto stop;
	}

//	 * - assure ACS is set
	if (TFA_GET_BF(handle, ACS) != 0) {
		result = 3;
		sprintf(lastErrorString, "DSP is not configured");
		goto stop;
	}

	/* execute test */
//	 * - run calibration always (with the calibration profile or profile 0)
	cal_prof = tfaContGetCalProfile(0);
	if (cal_prof < 0) {
		cal_prof = 0;
	}

	lastApiError = tfa98xxCalibration(handlesIn, 0, 0, cal_prof);
	if (lastApiError != Tfa98xx_Error_Ok) {
		sprintf(lastErrorString, "calibration call failed");
		result = 4;
		goto stop;
	}
	assert(lastApiError == Tfa98xx_Error_Ok);

        /* Note: calibration range is checked in test:
         * verify that the speaker impedance is within range */

	stop: Tfa98xx_Close(handle);

	lastError = result;
	TRACEOUT;
	return result;
}

int tfa_diag_read_speaker_status(int slave) {
	Tfa98xx_handle_t handle;
	int result=0;      // 1 is failure

	TRACEIN;

	lastApiError = Tfa98xx_Open((unsigned char)(slave << 1), &handle);
	if (lastApiError != Tfa98xx_Error_Ok) {
		result = 1;
		goto stop;
	}

	if (TFA_GET_BF(handle, ACKDMG) != 0) {
		sprintf(lastErrorString, "Speaker error detected!");
		result = 3;
	}

	stop: Tfa98xx_Close(handle);

	lastError = result;
	TRACEOUT;
	return result;
}

int tfa_diag_speaker_impedance(int slave) {
	Tfa98xx_handle_t handle;
	int idx, ready, result = 0;  // 1 is failure
	nxpTfaSpeakerFile_t *spkr;
	float imp[2];
	int calibrateDone=0, pin_result=0;
	float Rtypical_left;
	float Rtypical_right;
	int spkr_count = 0;
	unsigned short revid=0;

	TRACEIN;

	lastApiError = Tfa98xx_Open((unsigned char)(slave << 1), &handle);
	if (lastApiError != Tfa98xx_Error_Ok) {
		result = 1;
		goto stop;
	}

	lastApiError = tfa98xx_supported_speakers(handle, &spkr_count);
	if (lastApiError != Tfa98xx_Error_Ok) {
		result = 1;
		goto stop;
	}

	//	 * - ensure clock is on
	lastApiError = Tfa98xx_DspSystemStable(handle, &ready);
	assert(lastApiError == Tfa98xx_Error_Ok);
	if (!ready) {
		result = 2;
		goto stop;
	}
	if (TFA_GET_BF(handle, ACS) != 0) {
		result = 3;
		sprintf(lastErrorString, "DSP is not configured");
		goto stop;
	}

	/* lookup slave for device index */
	idx = tfa98xx_cnt_slave2idx(slave);
	if (idx<0) {
		result = -1;
		goto stop;
	}

	revid = tfa98xx_get_device_revision(handle);
	if((revid & 0xff) == 0x72) {
		/* For tiberius we need to get the calibrationDone status from the pin read */
		pin_result = NXP_I2C_GetPin(8);
		 /* 0x40 is bit 7 which is the calibrate_done flag */
		pin_result = (pin_result & 0x40) >> 6;

		if(pin_result)
			calibrateDone = 1;
	} else {
		lastApiError = mem_read(handle, 231, 1, &calibrateDone);
	}
	assert(lastApiError == Tfa98xx_Error_Ok);

	if ( !calibrateDone) {
		result = 4;
		sprintf(lastErrorString, "calibration was not done");
		goto stop;
	}

	spkr = (nxpTfaSpeakerFile_t *)tfacont_getfiledata(idx, 0, speakerHdr);
	if (spkr==0) {
		Rtypical_left = 0;
		Rtypical_right = 0;
	} else {
		Rtypical_left = spkr->ohm_primary;
		Rtypical_right = spkr->ohm_secondary;
	}

	/* We always have atleast one speaker */
	if (Rtypical_left==0) {
		PRINT("Warning: Speaker impedance (primary) not defined in spkr file, assuming 8000 mOhms!\n");
		Rtypical_left = 8;
	}

	/* If we have multiple speakers also check the secondary */
	if (spkr_count == 2) {
		if (Rtypical_right==0) {
			PRINT("Warning: Speaker impedance (secondary) not defined in spkr file, assuming 8000 mOhms!\n");
			Rtypical_right = 8;
		}
	}

	//	 * - compare result with expected value
	lastApiError = tfa98xx_dsp_get_calibration_impedance(handle, imp);
	assert(lastApiError == Tfa98xx_Error_Ok);

	/* 15% variation possible */
	if ( imp[0] < (Rtypical_left*0.85) || imp[0] > (Rtypical_left*1.15)) {
		sprintf(lastErrorString, "Primary speaker calibration value is not within expected range  (%2.2f Ohms)\n", imp[0]);
		//result = 5; /* Dont fail */
	} else {
			PRINT("Primary speaker value is:   %2.2f comparable to expected %2.2f Ohms\n", imp[0], Rtypical_left);
	}

	if (spkr_count > 1) {
		if ( imp[1] < (Rtypical_right*0.85) || imp[1] > (Rtypical_right*1.15)) {
			sprintf(lastErrorString, "Secondary speaker calibration value is not within expected range  (%2.2f Ohms)\n", imp[1]);
			//result = 5; /* Dont fail */
		} else {
			PRINT("Secondary speaker value is: %2.2f comparable to expected %2.2f Ohms\n", imp[1], Rtypical_right);
		}

		if ((imp[0] < (Rtypical_left*0.85) || imp[0] > (Rtypical_left*1.15)) && (imp[1] < (Rtypical_right*0.85) || imp[1] > (Rtypical_right*1.15))) {
			sprintf(lastErrorString, "Both speaker calibration values are not within expected range (%2.2f Ohms)(%2.2f mOhms)\n", imp[0], imp[1]);
			result = 5;
		}
	}

	stop: Tfa98xx_Close(handle);

	lastError = result;
	TRACEOUT;
	return result;
}

static void tfa_diag_dump_reg(Tfa98xx_handle_t handle, int i) {
	  unsigned short regval;

    Tfa98xx_ReadRegister16(handle, (unsigned char)i, &regval);
    PRINT("0x%02x:0x%04x ",i, regval);
    tfaRunBitfieldDump(handle, (unsigned char)i);
}
/**
 * dump all known registers
 *  @return
 *     0 if slave can't be opened
 *     nr of registers displayed
 */
int tfa98xxDiagRegisterDump(int slave)
{
	Tfa98xx_handle_t handle;
	int i;
	uint16_t revid, regval;

	TRACEIN;

	lastApiError = Tfa98xx_Open((unsigned char)(slave << 1), &handle);
	if (lastApiError != Tfa98xx_Error_Ok)
		return 0;


	if (slave == 0) {//* softdsp
		char *name;
		Tfa98xx_ReadRegister16(handle, 3, &revid);
		/* assume 50 soft regs */
		for(i=0;i<50;i++) {
			Tfa98xx_ReadRegister16(handle, (unsigned char)i, &regval);
			name = tfaContBfName((uint16_t)(i<<8 |0x00f),revid); // construct revid
			if ( strncmp(name, "Unknown bitfield enum",8)) //if not Unknown
				if (isupper(name[0]))
					PRINT("0x%02x:0x%04x %s\n",i, regval, name);
		}
		goto dump_exit;
	}
	if (tfa98xx_dev_family(handle)==2) { //tfa2
		for (i = 0; i <= 5; i++) {
			tfa_diag_dump_reg(handle, i);
		}
		tfa_diag_dump_reg(handle, 0xd);
		for (i = 0x10; i <= 0x15; i++) {
			tfa_diag_dump_reg(handle, i);
		}
	} else { //tfa1
		for (i = 0x0; i <=0x9 ; i++) {
			tfa_diag_dump_reg(handle, i);
		}
		tfa_diag_dump_reg(handle, 0x49);
		for (i = 0x70; i <=0x73 ; i++) {
			tfa_diag_dump_reg(handle, i);
		}
//		for (i = 0x80; i <=0x8f ; i++) {
//			tfa_diag_dump_reg(handle, i);
//		}
		tfa_diag_dump_reg(handle, 0x80);
		tfa_diag_dump_reg(handle, 0x83);
	}
dump_exit:
	Tfa98xx_Close(handle);

	TRACEOUT;
	return i;
}
/**
 * dump all TDM registers
 *  @return
 *     0 if slave can't be opened
 *     nr of registers displayed
 */
int tfa98xxDiagRegisterDumpTdm(void)
{
	//Tfa98xx_handle_t handle;
	//int i = 0;
	//unsigned short regval;

        //TRACEIN;

        return Tfa98xx_Error_Ok; //TODO tdm reg dump
	/*
        lastApiError = Tfa98xx_Open((unsigned char)(slave << 1), &handle);
        if (lastApiError != Tfa98xx_Error_Ok)
                return 0;

        if ( tfa98xx_get_device_dai(handle) & Tfa98xx_DAI_TDM )
        	for (i = 0; i < MAXTDMREGS; i++) {
        		lastApiError =
        				Tfa98xx_ReadRegister16(handle, tfa9888_tdm_regdefs[i].offset, &regval);
        		assert(lastApiError == Tfa98xx_Error_Ok);
        		PRINT("0x%02x:0x%04x ",
        				tfa9888_tdm_regdefs[i].offset, regval);
        		if (tfaRunBitfieldDump(tfa9888_tdm_regdefs[i].offset,
        				regval, tfa98xx_dev_family(handle) ))
        			PRINT("%s ", tfa9888_tdm_regdefs[i].name);
        	}
        	PRINT("\n");

        Tfa98xx_Close(handle);

        TRACEOUT;
        return i;
	*/
}
/**
 * dump  Interrupt registers
 *  @return
 *     0 if slave can't be opened
 *     nr of registers displayed
 */
int tfa98xxDiagRegisterDumpInt(void)
{
	//Tfa98xx_handle_t handle;
	//int i=0;
	//unsigned short regval;
	//int devtype;

       // TRACEIN;

        return Tfa98xx_Error_Ok; //TODO interrupts

	/*
        lastApiError = Tfa98xx_Open((unsigned char)(slave << 1), &handle);
        if (lastApiError != Tfa98xx_Error_Ok)
                return 0;

        devtype = tfa98xx_get_device_revision(handle);
        if ( (devtype == 0x97) ||  (devtype == 0x88))
        	for (i = 0; i < MAXINTREGS; i++) {
        		lastApiError =
        				Tfa98xx_ReadRegister16(handle, tfa9888_int_regdefs[i].offset, &regval);
        		assert(lastApiError == Tfa98xx_Error_Ok);
        		PRINT("0x%02x:0x%04x ",
        				tfa9888_int_regdefs[i].offset, regval);
        		if (tfaRunBitfieldDump(tfa9888_int_regdefs[i].offset,
        							regval, tfa98xx_dev_family(handle) ))
        			PRINT("%s ",tfa9888_int_regdefs[i].name);
        		PRINT("\n");
        	}

        Tfa98xx_Close(handle);

        TRACEOUT;
        return i;
	*/
}

/* check 4 address: 0x34-0x37 */
#define SCAN_START_ADDR 0x34
#define SCAN_STOP_ADDR  0x37

/* store devices for which r3 is readable */
static unsigned char scanned_i2c_devices[SCAN_STOP_ADDR-SCAN_START_ADDR+1];

/* number of i2c devices found during scan */
static int nr_i2c_devices = 0;

/* scan i2c bus and return the number of devices found */
int tfa_diag_scan_i2c_devices(int slave)
{
	unsigned short value = 0;
	Tfa98xx_handle_t handle;
	int err;

	/* new scan, clear number of devices */
	nr_i2c_devices = 0;
		
	for (slave=SCAN_START_ADDR; slave<=SCAN_STOP_ADDR; slave++) {
		lastApiError = Tfa98xx_Open((unsigned char)(slave << 1), &handle);
		if (lastApiError == Tfa98xx_Error_Ok){
			err = reg_read(handle, 0x03, &value);
			if (err == 0) {
				scanned_i2c_devices[nr_i2c_devices++] = (unsigned char)slave;
			}
		}
		Tfa98xx_Close(handle);
	}

	return nr_i2c_devices;
}

/* get the i2c address of a device found during tfa98xx_diag_scan_i2c_devices() */
unsigned char tfa_diag_get_i2c_address(int device)
{
	if (device >= nr_i2c_devices) {
		return 0;
	}

	return scanned_i2c_devices[device];
}

/* test scanning i2c bus */
int tfa_diag_i2c_scan(int slave)
{
        int nr, i;
        TRACEIN;
		
        nr = tfa_diag_scan_i2c_devices(slave);

        PRINT("number of i2c devices is %d:\n", nr);
        for(i=0; i<nr; i++) {
        	PRINT("  0x%x\n", tfa_diag_get_i2c_address(i));
        }

        TRACEOUT;

	/* Remove unreferenced parameter warning */
	(void)slave;

        /* test fails when no i2c devices are found */
	return (nr > 0) ? 0 : 1;
}
/* tap test */
int tfa_diag_tap(int slave)
{
	int handle, result, pinnr, state;
	int prof;
	int tap;
	TRACEIN;

	lastApiError = Tfa98xx_Open((unsigned char)(slave << 1), &handle);
	if (lastApiError != Tfa98xx_Error_Ok) {
		result = 1;
		goto stop;
	}

	if ( tfa_is_cold(handle) ) {
		sprintf(lastErrorString, "tfa is still cold\n"); //TODO show this
		result = 2;
		goto stop;
	}

	prof = tfa_get_swprof(handle);

	if ( !tfaContIsTapProfile( handle, prof) )
	{
		sprintf(lastErrorString, "profile:[\"%s\"] is not a .tap profile",
				tfaContProfileName(tfa98xx_cnt_slave2idx(slave), tfa_get_swprof(handle)));
		result = 3;
		goto stop;
	}

	if ( tfa98xx_dev_family(0)<2 ) {
		//TODO tap feature bits
		sprintf(lastErrorString, "test not valid for this device range");
		result = 4;
		goto stop;

	}

	/* disable all irq's */
	tfa98xx_irq_ena(handle, tfa9912_irq_all, 0);
	/* clear all */
	tfa98xx_irq_clear(handle, tfa9912_irq_all);
	/* check for clear */

	/* enable TAP IRQ */
	tfa98xx_irq_ena(handle, tfa9912_irq_sttapdet, 1);
#define GPIO_INT_L 8
	pinnr = GPIO_INT_L+tfa98xx_cnt_slave2idx(slave);
	state = NXP_I2C_GetPin(pinnr);
	if ( state==0 ) {
		sprintf(lastErrorString, "interrupt pin[%d] already active", pinnr);
		result = 5;
		goto stop;

	}
//	mem_write(handle, 0x8102, 0x123, Tfa98xx_DMEM_IOMEM);
//  cl --iomem=0x8102 -w0x123

	PRINT("waiting for IRQ pin[%d]...\n", pinnr);
	if ( lxScriboWaitOnPinEvent( pinnr, 0, 0) ) {
		sprintf(lastErrorString, "could not register interrupt pin[%d] wait-event", pinnr);
		result = 6;
		goto stop;
	}
	/* check for correct irq */
	if ( !tfa_irq_get(handle,tfa9912_irq_sttapdet ) ) {
		sprintf(lastErrorString, "interrupt was not from tap\n");
		result = 7;
		goto stop;
	}

	tfa98xx_irq_clear(handle, tfa9912_irq_sttapdet);

	tap = tfa_get_tap_pattern(handle);

	PRINT("===================================================\n");
	if ( tap >= 0 )
		 PRINT ("tap detect:0x%02x\n", tap);
	else PRINT ("illegal tap detected: 0x%x\n", tap);
	PRINT("===================================================\n");

	result=0;

	stop:
	tfa98xx_irq_ena(handle, tfa9912_irq_sttapdet, 0);

		Tfa98xx_Close(handle);

	lastError = result;
	TRACEOUT;
	return result;
}

/*
 * Get the connected audio source by getting the relevant GPIO detect and enable pins.
 * Copy the result into the caller supplied buffer.
 */

int tfa98xxGetAudioSource(char *buffer, int maxlength) 
{
	Tfa98xx_Error_t err = Tfa98xx_Error_Ok;
	char str[NXPTFA_MAXLINE];
	
	// (38) GPIO_DETECT_ANA
	// (39) GPIO_DETECT_I2S2
	// (40) GPIO_DETECT_I2S1
	// (41) GPIO_DETECT_SPDIF
	// (42) GPIO_ENABLE_ANA
	// (43) GPIO_ENABLE_I2S1
	// (44) GPIO_ENABLE_LPC
	// (45) GPIO_ENABLE_SPDIF
	
	sprintf(str, "Connected audio source: ");
	/* test for the detect and enable pins for the different sources
	 * if both are high the audio source is set 
	 */
	if ((1 == NXP_I2C_GetPin(38)) && (1 == NXP_I2C_GetPin(42))) {  /* Analog source */
		strcat(str, "Analog\n");
	}
	else if ((1 == NXP_I2C_GetPin(40)) && (1 == NXP_I2C_GetPin(43))) { /* I2C_IN1 source */ 
		strcat(str, "I2S_IN1\n");
	}
	else if ((1 == NXP_I2C_GetPin(41)) && (1 == NXP_I2C_GetPin(45))) { /* SPDIF source */ 
		strcat(str, "SPDIF\n");
	}
	else if (1 == NXP_I2C_GetPin(44)) {  /* USB Audio */ 
		strcat(str, "USB Audio\n");
	}
	else {
		/* external audio source could not be detected */
		strcat(str, "-- (no external audio source info)\n");
	}
        err = tfa_append_substring(str, buffer, maxlength);
	
	return err;
}

