/*
 * tfa98xxLiveData.c
 *
 *  Created on: Jun 7, 2012
 *      Author: wim
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include "dbgprint.h"
#include "Tfa98API.h"
#include "tfa_service.h"
#include "tfaContUtil.h"
#include "tfa98xxLiveData.h"
#include "tfaContainerWrite.h"
#include "tfa98xxCalculations.h"
#include "tfaOsal.h"

// globals
int tfa98xxLiveData_trace = 1;
int tfa98xxLiveData_verbose = 0;

static int maxdev = 0;
static unsigned char tfa98xxI2cbase;    // global for i2c access
static int state_info_match = 0;

#define I2C(idx) ((tfa98xxI2cbase+idx)*2)

void LiveData_trace(int level) {
	tfa98xxLiveData_trace = level;
}

/*
 * Set the debug option
 */
void tfaLiveDataVerbose(int level) {
	tfa98xxLiveData_verbose = level;
}

/**
 * Create a buffer from the memtrack data in the .ini file which can be used to send to the dsp.
 */
static void create_memtrack_buffer_msg(struct nxpTfa98xx_Memtrack_data *mRecord,
                                                     char *buffer, int *size)
{
        int i, j = 0;

        /* First copy the msg_id to the buffer */
        buffer[0] = 0;
        buffer[1] = MODULE_FRAMEWORK + 128;
        buffer[2] = FW_PAR_ID_SET_MEMTRACK;

        /* Then copy the length (number of memtrack items) */
        buffer[3] = (uint8_t) ((mRecord->length >> 16) & 0xffff);
        buffer[4] = (uint8_t) ((mRecord->length >> 8) & 0xff);
        buffer[5] = (uint8_t) (mRecord->length & 0xff);

        /* For every item take the tracker and add this infront of the adress */
        for(i=6; i<6+(mRecord->length*3); i++) {
                buffer[i] = (uint8_t) ((mRecord->trackers[j] & 0xff));
                i++;
                buffer[i] = (uint8_t) ((mRecord->mAdresses[j] >> 8) & 0xff);
                i++;
                buffer[i] = (uint8_t) (mRecord->mAdresses[j] & 0xff);
                j++;
        }

        *size = (6+(mRecord->length*3)) * sizeof(char);
}

/**
 * Compare the item to a the pre-defined state info list for max1
 * The number is used to find the item from the state info list. (order is always the same)
 */
int state_info_memtrack_compare(const char* liveD_name, int* number)
{
        int i;
	const char* record_names_max1[10] = { "agcGain", "limitGain", "limitClip", "batteryVoltage", "speakerTemp", "icTemp",
		"boostExcursion", "manualExcursion", "speakerResistance", "shortOnMips" };
	
	/* Compare the item to the record name list (case insensitive)*/
	for(i=0; i<10; i++) {
#if defined(WIN32) || defined(_X64)
		if(_stricmp(liveD_name, record_names_max1[i])==0) {
			*number = i;
			return 1;
		}
#else
		if(strcasecmp(liveD_name, record_names_max1[i])==0) {
			*number = i;
			return 1;
		}
#endif				
	}

	return 0;
}


/*maxdev
 * set the I2C slave base address and the nr of consecutive devices
 * the devices will be opened
 * TODO check for conflicts if already open
 */
Tfa98xx_Error_t nxpTfa98xxOpenLiveDataSlaves(Tfa98xx_handle_t *handlesIn, int i2cbase, int maxdevices)
{
        Tfa98xx_Error_t err = Tfa98xx_Error_Ok;
        int i;
        maxdev = maxdevices;

        if ( (maxdev < 1) | (maxdev > 4) )    // only allow one 1 up to 4
                return Tfa98xx_Error_Bad_Parameter;

        tfa98xxI2cbase = (unsigned char)i2cbase;       // global for i2c access

        for (i = 0; i < maxdev; i++) 
        {
		   tfaContGetSlave(i, &tfa98xxI2cSlave); /* get device I2C address */
           if( handlesIn[i] == -1) 
           {
              //err = Tfa98xx_Open(I2C(i), &handlesIn[i] );
	          err = Tfa98xx_Open(tfa98xxI2cSlave*2, &handlesIn[i]);
			  PRINT_ASSERT( err);
           }
        }

        return err;
}

nxpTfaLiveData_t* tfaGetLiveDataItem(nxpTfaDescPtr_t *dsc, nxpTfaContainer_t *cont )
{
        nxpTfaLiveData_t *liveD;

        liveD = (nxpTfaLiveData_t *)(dsc->offset+(uint8_t *)cont);
        return liveD;
}

Tfa98xx_Error_t tfa98xx_get_live_data_items(Tfa98xx_handle_t *handlesIn, int idx, char *strings)
{
	Tfa98xx_Error_t error = Tfa98xx_Error_Ok;
	nxpTfaContainer_t *cont;
	nxpTfaLiveDataList_t *lData;
	nxpTfaLiveData_t *liveD;
	char *str = malloc( sizeof(char) * (NXPTFA_MAXLINE + 1));
	int DrcSupport, maxlength = sizeof(liveD->name) * MEMTRACK_MAX_WORDS;
	unsigned int i, j, maxdev=0;
	uint8_t slave;
	
	cont = tfa98xx_get_cnt();

	/* Only Max1 devices */
	if ( tfa98xx_dev_family(handlesIn[idx]) == 1) {
		/* This should only be checked/printed once! */
		if(idx < 1) {
			/* Search all devices to see if there is state info present */
			maxdev=tfa98xx_cnt_max_device();
			for (i=0; i < maxdev; i++ ) {
				if(cont->nliveData > 0) {
					lData=tfaContGetDevLiveDataList(cont, i, 0);
					if(lData != NULL) {
						// Get all livedata items
						for(j=0; j<lData->length-1u; j++) {
							liveD = tfaGetLiveDataItem(&lData->list[j], cont);
							/* if there is a match we can exit (DrcSupport is not used)*/
							state_info_match = state_info_memtrack_compare(liveD->name, &DrcSupport);
							if(state_info_match)
								break;
						}
					}
				}
			}

			/* Only print this when there are no items in the stateinfo items in the memtrack section */
			if(state_info_match == 0) {
				sprintf(str, "agcGain,limitGain,limitClip,batteryVoltage,speakerTemp,icTemp,"
						"boostExcursion,manualExcursion,speakerResistance,shortOnMips,");
				error = tfa_append_substring(str, strings, maxlength);
				if(error != Tfa98xx_Error_Ok)
					goto error_exit;
			}

			error = tfa98xx_dsp_support_drc(handlesIn[idx], &DrcSupport);
			if(error != Tfa98xx_Error_Ok)
				goto error_exit;
			if (DrcSupport) {
				sprintf(str, ",GRhighDrc1[0],GRhighDrc1[1],"
					"GRhighDrc2[0],GRhighDrc2[1],"
					"GRmidDrc1[0],GRmidDrc1[1],"
					"GRmidDrc2[0],GRmidDrc2[1],"
					"GRlowDrc1[0],GRlowDrc1[1],"
					"GRlowDrc2[0],GRlowDrc2[1],"
					"GRpostDrc1[0],GRpostDrc1[1],"
					"GRpostDrc2[0],GRpostDrc2[1],"
					"GRblDrc[0],GRblDrc[1],");
				error = tfa_append_substring(str, strings, maxlength);
				if(error != Tfa98xx_Error_Ok)
					goto error_exit;
			}
		}
	}

	/* This is printed for every device */
	if(cont->nliveData > 0) {
		lData=tfaContGetDevLiveDataList(cont, idx, 0);
		if(lData != NULL) {
			// Get all livedata items
			for(j=0; j<lData->length-1u; j++) {
				liveD = tfaGetLiveDataItem(&lData->list[j], cont);
				if ( tfa98xx_dev_family(handlesIn[idx]) == 2) {
					if (compare_strings(liveD->name, "buildinmemtrack", 25) == 0) {
						sprintf(str, "%s,", liveD->name);
						error = tfa_append_substring(str, strings, maxlength);
					}
				} else  {
					tfaContGetSlave(idx, &slave);
					sprintf(str, "%s(0x%x),", liveD->name, slave); 
					error = tfa_append_substring(str, strings, maxlength);
				}
				if(error != Tfa98xx_Error_Ok)
					goto error_exit;
			}
		}
	} else if ( tfa98xx_dev_family(handlesIn[idx]) == 2) {
		sprintf(str, "Unable to print record data without livedata section. Please check container file!");
		error = tfa_append_substring(str, strings, maxlength);
	}

error_exit:

	free(str);
    return error;
}

Tfa98xx_Error_t tfa98xx_get_live_data_raw(Tfa98xx_handle_t handle, unsigned char *bytes, int length)
{
        Tfa98xx_Error_t err = Tfa98xx_Error_Ok;

        /* Get memtrack */
        err = tfa_dsp_cmd_id_write_read(handle,MODULE_FRAMEWORK,FW_PAR_ID_GET_MEMTRACK, length, bytes);

        return err;
}

Tfa98xx_Error_t tfa98xx_set_live_data(Tfa98xx_handle_t *handlesIn, int idx)
{
	Tfa98xx_Error_t err = Tfa98xx_Error_Ok;
	nxpTfaContainer_t *cont;
	nxpTfaLiveDataList_t *lData;
	nxpTfaLiveData_t *liveD;
	struct nxpTfa98xx_Memtrack_data mRecord;
	char *buffer = malloc(sizeof(char) * ((MEMTRACK_MAX_WORDS * 3) + 6)); //every word requires 3 bytes, and 6 is the msg + length
	int size=0, i;
	unsigned int j, k=0, skip_set_msg=0;
	memset(&mRecord, 0, sizeof(mRecord));

	if (tfa98xx_dev_family(handlesIn[idx]) == 2) {
		/* Get the memtrack information from the container file */
		cont = tfa98xx_get_cnt();
		if(cont->nliveData > 0) {
			lData=tfaContGetDevLiveDataList(cont, idx, 0);
			if(lData == NULL) {
				free(buffer);
				return err;
			} else if(lData->length-1 > MEMTRACK_MAX_WORDS) {
				PRINT("Error: There are too many memtrack registers\n");
				free(buffer);
				return Tfa98xx_Error_Bad_Parameter;
			} else {
				mRecord.length = 0; /* initialise */
			}

			/* The number of memtrack sections in the container file */
			for (i=0; i<cont->nliveData; i++) {
				/* The number of memtrack items */
				for(j=0; j<lData->length-1u; j++) {
					liveD = tfaGetLiveDataItem(&lData->list[j], cont);

					/* Only use DSP memtrack items. Skip the I2C registers/bitfields */
					/* We need the use k else we get empty lines in the mRecord */
					/* if not in any list */
					if ( tfaContBfEnumAny(liveD->addrs)==0xffff) {
						if (compare_strings(liveD->name, "buildinmemtrack", 25)) {
							printf("Found keyword: %s. Skipping the SetMemtrack! \n", liveD->name);
							skip_set_msg = 1;
						} else {
							mRecord.mAdresses[k] = (int)strtol(liveD->addrs, NULL, 16);
							mRecord.trackers[k] = liveD->tracker;
							mRecord.length++;
							k++;
						}
					}
				}
			}

			if(!skip_set_msg) {
				if(mRecord.length > 0) {
					create_memtrack_buffer_msg(&mRecord, buffer, &size);
					err = dsp_msg(handlesIn[idx], size, buffer);
				}
			}
		}
	}

	free(buffer);
	return err;
}

Tfa98xx_Error_t tfa98xx_get_live_data(Tfa98xx_handle_t *handlesIn, int idx, float *live_data, int *nr_of_items)
{
	Tfa98xx_Error_t err = Tfa98xx_Error_Ok;
	nxpTfaContainer_t *cont;
	nxpTfaLiveDataList_t *lData = {0};
	nxpTfaLiveData_t *liveD;
	nxpTfaBitfield_t bf;
	nxpTfa98xx_LiveData_t record;
	struct nxpTfa98xx_Memtrack_data mRecord;
	unsigned char *bytes = malloc(sizeof(unsigned char) * ((LSMODEL_MAX_WORDS * 3)+1));
	int *data = malloc(sizeof(int) * 151); /* 150 + 1*/
	LiveDataList_t order[MEMTRACK_MAX_WORDS+1];
	unsigned char inforegs[6];
	int supportDrc, listNumber=0;
	unsigned int i, j, stateSize, length=0;
	short icTemp_2Complement;
	float *p, livedata_backup[10] = {0};

	// init
	for(i=0; i<MEMTRACK_MAX_WORDS+1; i++) {
		order[i].isMemtrackItem = 0;
	}

	/* Get the memtrack information from the container file */
	cont = tfa98xx_get_cnt();

	/* Max2 device */
	if (tfa98xx_dev_family(handlesIn[idx]) == 2) {
		stateSize = MEMTRACK_MAX_WORDS;        
		/* Make sure the cnt has a livedata section */
		if(cont->nliveData > 0) {
			lData=tfaContGetDevLiveDataList(cont, idx, 0);
			if(lData != NULL) {
				i=0;
				/* The number of memtrack items */
				for(j=0; j<lData->length-1u; j++) {
					liveD = tfaGetLiveDataItem(&lData->list[j], cont);
					if (compare_strings(liveD->name, "buildinmemtrack", 25) == 0) {
						mRecord.scalingFactor[i] = liveD->scalefactor;
						if(mRecord.scalingFactor[i] == 0) {
							PRINT("Error, Scale factor zero, possible division by zero.\n");
						}
                        
						/* Create a list to get the order of DSP memtrack items and I2C registers/bitfields */
                				/* if not in any list */
						if ( tfaContBfEnumAny(liveD->addrs)==0xffff) {
							order[i].isMemtrackItem = 1;
						} else {
							order[i].address = liveD->addrs;
						}
						i++;
					}
				}
			} else {
				/* This idx has no livedata section */
				free(bytes);
				free(data);
				return Tfa98xx_Error_Other;
			}

			/* Get the raw livedata */
			err = tfa98xx_get_live_data_raw(handlesIn[idx], bytes, sizeof(unsigned char)*(LSMODEL_MAX_WORDS * 3));
			if(err == Tfa98xx_Error_Ok) {
				/* Convert data (without the +1 the 50th memtrack item gets lost) */
				tfa98xx_convert_bytes2data(3 * (stateSize+1), bytes, data);
			} else {
				*nr_of_items = 0;
				free(bytes);
				free(data);
				return Tfa98xx_Error_Other;
			}

			j=0;
			/* Take every memtrack item and devide with the scalingfactor
			 * There are lData->length-1 number of memtrack items, BUT
			 * that does not mean lData->length-1 number of DSP items!
			 * Because the list is created in DSP order we need to get the data in order
			 * (and not skip when there is a i2c item in between)
			 */
			length = lData->length-1u;
			for(i=0;i<length;i++) {
				liveD = tfaGetLiveDataItem(&lData->list[i], cont);
				if (compare_strings(liveD->name, "buildinmemtrack", 25) != 0) {
					length -= 1;
				}

				if(order[i].isMemtrackItem) {
					live_data[i] = (float)data[j+1] / mRecord.scalingFactor[i];
					j++;
				} else {
					/* Get the Bitfield value */
					bf.field = tfaContBfEnum(order[i].address, tfa98xx_dev_revision(handlesIn[idx])); //TODO solve for max1
					err = tfaRunReadBitfield(handlesIn[idx], &bf);
					live_data[i] = (float)bf.value / mRecord.scalingFactor[i];
				}
			}

			*nr_of_items = length;
		} else {
			*nr_of_items = 0;
			free(bytes);
			free(data);
			return Tfa98xx_Error_Other;
		}
	} else { /* Max1 devices */

		/* nr_of_items is the start of the next item added to the live_data list */
		if(state_info_match == 0) {
			*nr_of_items = 10;
		} else {
			*nr_of_items = 0;
		}

		err = tfa98xx_dsp_get_state_info(handlesIn[idx], bytes, &stateSize);
		if(err == Tfa98xx_Error_Ok) {
			/* init to default value to have sane values even when
			* some features aren't supported */
			for (i = 0; i < MEMTRACK_MAX_WORDS; i++) {
				data[i] = 0;
			}

			tfa98xx_convert_bytes2data(3 * stateSize, bytes, data);

			record.limitClip = (float)data[2] / (1 << (23 - SPKRBST_HEADROOM));
			record.agcGain = (float)data[0] / (1 << (23 - SPKRBST_AGCGAIN_EXP));
			record.limitGain = (float)data[1] / (1 << (23 - SPKRBST_LIMGAIN_EXP));
			record.speakerTemp = data[3] / (1 << (23 - SPKRBST_TEMPERATURE_EXP));
			record.boostExcursion = (float)data[5] / (1 << (23 - SPKRBST_HEADROOM));

			if (((tfa98xx_dev_revision(handlesIn[idx]) & 0xff ) != 0x97) && ((tfa98xx_dev_revision(handlesIn[idx])& 0xff )  != 0x96)) {
       			record.manualExcursion = (float)data[6] / (1 << (23 - SPKRBST_HEADROOM));
				record.speakerResistance = (float)data[7] / (1 << (23 - SPKRBST_TEMPERATURE_EXP));
				record.shortOnMips = data[8];
			} else {
				record.manualExcursion = 0;
				record.speakerResistance = (float)data[6] / (1 << (23 - SPKRBST_TEMPERATURE_EXP));
				record.shortOnMips = data[7];
			}
					
			/* get regs, and check status */
			err = Tfa98xx_ReadData(handlesIn[idx], 0, 6, inforegs);
			record.statusRegister = inforegs[0] << 8 | inforegs[1];
			record.batteryVoltage = (float)(inforegs[2] << 8 | inforegs[3]);
			record.batteryVoltage = (float)((record.batteryVoltage * 5.5)/1024);
			icTemp_2Complement = inforegs[4] << 8 | inforegs[5];
			record.icTemp = (icTemp_2Complement >= 256) ? (icTemp_2Complement - 512) : icTemp_2Complement;

			live_data[0] = record.agcGain;
			live_data[1] = record.limitGain;
			live_data[2] = record.limitClip;
			live_data[3] = record.batteryVoltage;
			live_data[4] = (float)record.speakerTemp;
			live_data[5] = (float)record.icTemp;
			live_data[6] = record.boostExcursion;
			live_data[7] = record.manualExcursion;
			live_data[8] = record.speakerResistance;
			live_data[9] = (float)record.shortOnMips;

			for(i=0; i<10; i++)
				livedata_backup[i] = live_data[i];

			/* DRC */
			tfa98xx_dsp_support_drc(handlesIn[idx], &supportDrc);
			if (supportDrc) {
				int *pdata = &data[9];
				record.drcState.GRhighDrc1[0] = (float)(*pdata++) / (1 << (23 - DSP_MAX_GAIN_EXP));
				record.drcState.GRhighDrc1[1] = (float)(*pdata++) / (1 << (23 - DSP_MAX_GAIN_EXP));
				record.drcState.GRhighDrc2[0] = (float)(*pdata++) / (1 << (23 - DSP_MAX_GAIN_EXP));
				record.drcState.GRhighDrc2[1] = (float)(*pdata++) / (1 << (23 - DSP_MAX_GAIN_EXP));
				record.drcState.GRmidDrc1[0] =  (float)(*pdata++) / (1 << (23 - DSP_MAX_GAIN_EXP));
				record.drcState.GRmidDrc1[1] =  (float)(*pdata++) / (1 << (23 - DSP_MAX_GAIN_EXP));
				record.drcState.GRmidDrc2[0] =  (float)(*pdata++) / (1 << (23 - DSP_MAX_GAIN_EXP));
				record.drcState.GRmidDrc2[1] =  (float)(*pdata++) / (1 << (23 - DSP_MAX_GAIN_EXP));
				record.drcState.GRlowDrc1[0] =  (float)(*pdata++) / (1 << (23 - DSP_MAX_GAIN_EXP));
				record.drcState.GRlowDrc1[1] =  (float)(*pdata++) / (1 << (23 - DSP_MAX_GAIN_EXP));
				record.drcState.GRlowDrc2[0] =  (float)(*pdata++) / (1 << (23 - DSP_MAX_GAIN_EXP));
				record.drcState.GRlowDrc2[1] =  (float)(*pdata++) / (1 << (23 - DSP_MAX_GAIN_EXP));
				record.drcState.GRpostDrc1[0] = (float)(*pdata++) / (1 << (23 - DSP_MAX_GAIN_EXP));
				record.drcState.GRpostDrc1[1] = (float)(*pdata++) / (1 << (23 - DSP_MAX_GAIN_EXP));
				record.drcState.GRpostDrc2[0] = (float)(*pdata++) / (1 << (23 - DSP_MAX_GAIN_EXP));
				record.drcState.GRpostDrc2[1] = (float)(*pdata++) / (1 << (23 - DSP_MAX_GAIN_EXP));
				record.drcState.GRblDrc[0] =    (float)(*pdata++) / (1 << (23 - DSP_MAX_GAIN_EXP));
				record.drcState.GRblDrc[1] =    (float)(*pdata  ) / (1 << (23 - DSP_MAX_GAIN_EXP));

				p = &record.drcState.GRhighDrc1[0];
   				for(i=0; i < (sizeof(nxpTfa98xx_DrcStateInfo_t)/sizeof(float));i++) {
					live_data[*nr_of_items+i] = *p++;
				}

				/* Increase the start of the next item added to the live_data list */
				*nr_of_items = *nr_of_items + (sizeof(nxpTfa98xx_DrcStateInfo_t)/sizeof(float));
			}
		}

		/*--- Livedata section ini file ---*/
		if(cont->nliveData > 0) {
			lData=tfaContGetDevLiveDataList(cont, idx, 0);
			if(lData != NULL) {
				/* The number of memtrack items */
				for(j=0; j<lData->length-1u; j++) {
					liveD = tfaGetLiveDataItem(&lData->list[j], cont);
					mRecord.scalingFactor[j] = liveD->scalefactor;
					order[j].address = liveD->addrs;

					/* Check if it is an item from the state info list */
					if(state_info_memtrack_compare(liveD->name, &listNumber)) {
						live_data[*nr_of_items+j] = livedata_backup[listNumber];
					} else {
						/* Get the Bitfield value */
						bf.field = tfaContBfEnum(order[j].address, tfa98xx_dev_revision(handlesIn[idx]));
						err = tfaRunReadBitfield(handlesIn[idx], &bf);
						live_data[*nr_of_items+j] = (float)bf.value / mRecord.scalingFactor[j];
					}
				}

				/* Increase the start of the next item added to the live_data list */
				*nr_of_items = *nr_of_items + (lData->length-1);
			}
		}
	}

	free(bytes);
	free(data);
	return err;
}

typedef float fix;
#define MIN(a,b) ((a)<(b)?(a):(b))
#define PARAM_GET_LSMODEL         0x86        // Gets current LoudSpeaker impedance Model.
#define PARAM_GET_LSMODELW        0xC1        // Gets current LoudSpeaker xcursion Model.
void tfa98xxGetRawSpeakerModel(Tfa98xx_handle_t *handlesIn,
							unsigned char *buffer, int xmodel, int idx)
{
    Tfa98xx_Error_t error = Tfa98xx_Error_Ok;

    error = tfa_dsp_cmd_id_write_read(handlesIn[idx], 1,
                                xmodel ? PARAM_GET_LSMODELW : PARAM_GET_LSMODEL,
                                423, buffer);
    assert(error == Tfa98xx_Error_Ok);

}

/* gets the x or z-model from the dsp and calculate the waveform values */
int tfa98xxGetWaveformValues(int handle, int *data, double *waveform, int xmodel)
{
	Tfa98xx_Error_t error = Tfa98xx_Error_Ok;
	SPKRBST_SpkrModel_t record;
	unsigned char *bytes = malloc(sizeof(unsigned char) * ((LSMODEL_MAX_WORDS * 3)+1));
	double *wY = malloc(sizeof(double) * (FFT_SIZE+1)); /* Re-use! --> Before fft for max1 */
	double *aw = malloc(sizeof(double) * (FFT_SIZE+1)); /* Re-use! --> Before untangled for max1 */
	double *scratch = malloc(sizeof(double) * (FFT_SIZE+1));
	double *absZ = malloc(sizeof(double) * ((FFT_SIZE/2)+1));
	double *absX = malloc(sizeof(double) * ((FFT_SIZE/2)+1));
	int i, max=0, lagW = 12;
	double fs, leakageFactor, ReCorrection, Bl, ReZ;

	/* max 2 */
	if ( tfa98xx_dev_family(handle) == 2) {
		error = tfa_dsp_cmd_id_write_read(handle,MODULE_SPEAKERBOOST,SB_PARAM_GET_LSMODEL, 450, bytes);
	} else {
		error = tfa_dsp_cmd_id_write_read(handle,MODULE_SPEAKERBOOST, xmodel ? PARAM_GET_LSMODELW : PARAM_GET_LSMODEL, 423, bytes);
	}

	if (error != Tfa98xx_Error_Ok) {
		free(bytes);
		free(wY);
		free(aw);
		free(scratch);
		free(absZ);
		free(absX);
		return error;
	}

	tfa98xx_convert_bytes2data(3 * LSMODEL_MAX_WORDS, bytes, data);

	/* max 2 */
	if ( tfa98xx_dev_family(handle) == 2) {
		/* Module constructor / Instantiation, Create the Hanning window */
		windowing_filter(aw, FFT_SIZE);

		for (i = 0; i < FFT_SIZE; i++) {
			wY[i] =(double)data[i] / (1 << 22);  
			/* get the maximum position */
			if (abs(data[i])>max) {
				max = abs(data[i]);
				lagW = i;
			}
		}
		compute_Zf(wY, absZ, scratch, FFT_SIZE);

		/* prepare the required information for the Xf computation */
		fs = (double)data[fs_IDX] / fs_SCALE;
		leakageFactor = (double)data[leakageFactor_IDX] / leakageFactor_SCALE;
		ReCorrection = (double)data[ReCorrection_IDX] / ReCorrection_SCALE;
		Bl = (double)data[Bl_IDX] / Bl_SCALE;
		ReZ = (double)data[ReZ_IDX] / TFA_FW_ReZ_SCALE;

		compute_Xf(wY, absX, aw, scratch, lagW, fs, leakageFactor, ReCorrection, Bl, ReZ, FFT_SIZE); 

		for (i = 0; i < FFT_SIZE / 2; i++) {
			if(xmodel)
				waveform[i] = absX[i];
			else
				waveform[i] = absZ[i];
		}
	} else {
		for (i = 0; i < 128; i++) {
			record.pFIR[i] = (double)data[i] / (1 << 22);
		}

		record.Shift_FIR = data[i++];   /* Exponent of HX data */
		record.leakageFactor = (float)data[i++] / (1 << (23));  /* Excursion model integration leakage */
		record.ReCorrection = (float)data[i++] / (1 << (23));   /* Correction factor for Re */
		record.xInitMargin = (float)data[i++] / (1 << (23 - 2));        /* (can change) Margin on excursion model during startup */
		record.xDamageMargin = (float)data[i++] / (1 << (23 - 2));      /* Margin on excursion modelwhen damage has been detected */
		record.xMargin = (float)data[i++] / (1 << (23 - 2));    /* Margin on excursion model activated when LookaHead is 0 */
		record.Bl = (float)data[i++] / (1 << (23 - 2)); /* Loudspeaker force factor */
		record.fRes = data[i++];        /* (can change) Estimated Speaker Resonance Compensation Filter cutoff frequency */
		record.fResInit = data[i++];    /* Initial Speaker Resonance Compensation Filter cutoff frequency */
		record.Qt = (float)data[i++] / (1 << (23 - 6)); /* Speaker Resonance Compensation Filter Q-factor */
		record.xMax = (float)data[i++] / (1 << (23 - 7));       /* Maximum excursion of the speaker membrane */
		record.tMax = (float)data[i++] / (1 << (23 - 9));       /* Maximum Temperature of the speaker coil */
		record.tCoefA = (float)data[i++] / (1 << 23);   /* (can change) Temperature coefficient */

		if (xmodel) {
			/* xmodel 0xc1 */
			for (i = 0; i < 128; i++) {
			   wY[127 - i] =(double)data[i] / (1 << 22);
			}
			realfft_split(wY, 128);
			untangle_leakage(wY, aw, 128, -record.leakageFactor);
			for (i = 0; i < 128 / 2; i++)
				waveform[i] = 2 * aw[i];
		} else {
			/* zmodel 0x86 */
			for (i = 0; i < 128; i++) {
				wY[i] = (double)data[i] / (1 << 22);
			}
			realfft_split(wY, 128);
			untangle(wY, aw, 128);
			for (i = 0; i < 128 / 2; i++)
				waveform[i] = 1 / aw[i];
		}
	}

	free(bytes);
	free(wY);
	free(aw);
	free(scratch);
	free(absZ);
	free(absX);
	return error;
}

int tfa98xxPrintSpeakerModel(Tfa98xx_handle_t *handlesIn, float *strings, int maxlength, int xmodel, int idx)
{
	Tfa98xx_Error_t error = Tfa98xx_Error_Ok;
	int *data = malloc(sizeof(int) * 151); /* 150 + 1*/	
	double *waveform = malloc(sizeof(double) * 129); /* 128 + 1*/
	int i, j=0;

	/* calculate x or z model waveforms */
	error = tfa98xxGetWaveformValues(handlesIn[idx], &data[0], &waveform[0], xmodel);
	if (error != Tfa98xx_Error_Ok) {
		free(data);
		free(waveform);
		return error;
	}

	i = xmodel ? 1 : 0;
	for (; i < 64; i++) {
		strings[j] = (float)waveform[i];
		j++;
		if(j > maxlength) {
			error = Tfa98xx_Error_Fail;
			break;
		}
	}

	if (tfa98xxLiveData_verbose)
		PRINT("FIR: 0x%03x\n", data[0]);

	free(data);
	free(waveform);
	return error;
}

int tfa_print_speakermodel_from_speakerfile(nxpTfaSpeakerFile_t *spk, float *strings, int maxlength, int xmodel)
{
	Tfa98xx_Error_t error = Tfa98xx_Error_Ok;
	int *data = malloc(sizeof(int) * 151); /* 150 + 1*/	
	double *waveform = malloc(sizeof(double) * 129); /* 128 + 1*/
	double *scratch = malloc(sizeof(double) * (FFT_SIZE+1));
	double *absZ = malloc(sizeof(double) * ((FFT_SIZE/2)+1));
	double *absX = malloc(sizeof(double) * ((FFT_SIZE/2)+1));
	double *wY = malloc(sizeof(double) * (FFT_SIZE+1));
	double *aw = malloc(sizeof(double) * (FFT_SIZE+1));
	int i, j=0, max=0, lagW = 12;
	double fs, leakageFactor, ReCorrection, Bl, ReZ;

	/* calculate x or z model waveforms form speaker file data
	 * shift by 6 to skip the xlm-id and the cmd-id
	 */
	tfa98xx_convert_bytes2data(3 * LSMODEL_MAX_WORDS, spk->data+6, data);

	/* Module constructor / Instantiation, Create the Hanning window */
	windowing_filter(aw, FFT_SIZE);

	for (i = 0; i < FFT_SIZE; i++) {
		wY[i] =(double)data[i] / (1 << 22);                               
		/* get the maximum position */
		if (abs(data[i])>max) {
			max = abs(data[i]);
			lagW = i;
		}
	}

	compute_Zf(wY, absZ, scratch, FFT_SIZE);

	/* prepare the required information for the Xf computation */
	fs = (double)data[fs_IDX] / fs_SCALE;
	leakageFactor = (double)data[leakageFactor_IDX] / leakageFactor_SCALE;
	ReCorrection = (double)data[ReCorrection_IDX] / ReCorrection_SCALE;
	Bl = (double)data[Bl_IDX] / Bl_SCALE;
	ReZ = (double)data[ReZ_IDX] / TFA2_FW_ReZ_SCALE;

	compute_Xf(wY, absX, aw, scratch, lagW, fs, leakageFactor, ReCorrection, Bl, ReZ, FFT_SIZE); 

	for (i = 0; i < FFT_SIZE / 2; i++) {
		if(xmodel)
			waveform[i] = absX[i];
		else
			waveform[i] = absZ[i];
	}

	i = xmodel ? 1 : 0;
	for (; i < 64; i++) {
		strings[j] = (float)waveform[i];
		j++;
		if(j > maxlength) {
			error = Tfa98xx_Error_Fail;
			break;
		}
	}

	free(data);
	free(waveform);
	free(wY);
	free(aw);
	free(scratch);
	free(absZ);
	free(absX);

	return error;
}

int tfa98xxPrintSpeakerTable(Tfa98xx_handle_t *handlesIn, char *strings, int maxlength, int xmodel, int idx, int loop)
{
	Tfa98xx_Error_t error = Tfa98xx_Error_Ok;
	char str[NXPTFA_MAXLINE];
	int i, j, hz, data[150];
	double waveform[128];
	int handle = handlesIn[idx]; /* macro depends on this variable, not nice but simple */

	if(loop == 1) {
		hz = xmodel ? (int)(62.5) : 40;
		j = xmodel ? 1 : 0;

		sprintf(str, "Hz:");
		error = tfa_append_substring(str, strings, maxlength);

		for (; j < 64; j++) {
			sprintf(str, "%d,", hz);
			error = tfa_append_substring(str, strings, maxlength);
			if (error != Tfa98xx_Error_Ok)
       				return error;
			hz += (int)(62.5);
		}
		sprintf(str, "\n\n");
		error = tfa_append_substring(str, strings, maxlength);
	}

	hz = xmodel ? (int)(62.5) : 40;
	i = xmodel ? 1 : 0;

	/* calculate x or z model waveforms */
	error = tfa98xxGetWaveformValues(handle, &data[0], &waveform[0], xmodel);
	if (error != Tfa98xx_Error_Ok)
       		return error;

	sprintf(str, "%d,", loop);
	error = tfa_append_substring(str, strings, maxlength);
	if (error != Tfa98xx_Error_Ok)
       		return error;

	for (; i < 64; i++) {
		sprintf(str, "%f,", waveform[i]);
		error = tfa_append_substring(str, strings, maxlength);
		if (error != Tfa98xx_Error_Ok)
       			return error;
		hz += (int)(62.5);
	}
	sprintf(str, "\n");
	error = tfa_append_substring(str, strings, maxlength);
	
	return error;
}

/* 
 * @param index_subband the subband that has been selected as hex value 
 * @param SbGains_buf the buffer for the SbGains
 * @param SbFilt_buf the buffer for the SbFilt
 * @param CombGain_buf the buffer for the CombGain
 * @param maxlength indicated the maximum length of the buffers (currently all 3 should be the same length)
 */

int tfa98xxPrintMBDrcModel(Tfa98xx_handle_t *handlesIn, int idx, int index_subband, 
	s_subBandDynamics *sbDynamics, float *SbGains_buf, float *SbFilt_buf, float *CombGain_buf, int maxlength)
{
	Tfa98xx_Error_t error = Tfa98xx_Error_Ok;
        unsigned char bytes[LSMODEL_MAX_WORDS * 3] = {0};
	double scratchBuff[FFT_SIZE_DRC] = {0};
	double subBandFiltFreqTrFunc[MAX_NR_SUBBANDS][FFT_SIZE_DRC/2];
	double subBandGainFreqTrFunc[MAX_NR_SUBBANDS][FFT_SIZE_DRC/2];
	double gainCombinedTrFunc[FFT_SIZE_DRC/2] = {0};
	int i=0, j, k=0, MAX_SUBBANDS=6, nr_subbands=0;
	int data[150] = {0};
	int handle = handlesIn[idx]; /* macro depends on this variable, not nice but simple */

	error = tfa_dsp_cmd_id_MBDrc_dynamics(handle, MODULE_SPEAKERBOOST, SB_PARAM_GET_MBDRC_DYNAMICS, index_subband, sizeof(bytes), bytes);
	if(error != Tfa98xx_Error_Ok)
		return error;

	tfa98xx_convert_bytes2data(3 * LSMODEL_MAX_WORDS, bytes, data);

	if(index_subband > 63) {
		PRINT("Incorrect subband index given. \n");
		return Tfa98xx_Error_Bad_Parameter;
	} else {
		for(i=0; i<MAX_SUBBANDS; i++) {
			tfa98xx_calculate_dynamix_variables_drc(data, &sbDynamics[i], i);
			compute_gain_filter_transf_function(subBandGainFreqTrFunc[i], &sbDynamics[i].BQ_2, sbDynamics[i].Gains.subBandGain, scratchBuff, FFT_SIZE_DRC);
			compute_input_filter_transf_function(subBandFiltFreqTrFunc[i], &sbDynamics[i].BQ_1, scratchBuff, FFT_SIZE_DRC, GUI_LOW_LIMIT);
		}
	}

	/* Get the number of subbands enabled */
	nr_subbands = bitCount_subbands(index_subband);

	/* Get the combined gain subbands transfer function by adding all the subbands gain transfer functions */
	compute_gain_combined_transf_function(gainCombinedTrFunc, subBandGainFreqTrFunc, nr_subbands, FFT_SIZE_DRC/2);

	if (tfa98xxLiveData_verbose) {
		PRINT("Selected subbands: 0x%x \n", index_subband);
		PRINT("Number of subbands selected: %d \n", nr_subbands);
	}

	for (i=0; i<nr_subbands; i++) {
		for (j=0; j<FFT_SIZE_DRC/2; j++) {
			SbGains_buf[k] = (float)subBandGainFreqTrFunc[i][j];
			SbFilt_buf[k] = (float)subBandFiltFreqTrFunc[i][j];

			k++;
			if(k > maxlength) {
				PRINT("Error: Buffer size too short! \n");
				return Tfa98xx_Error_Fail;
			}
		}
	}

	for (i=0; i<FFT_SIZE_DRC/2; i++) {
		CombGain_buf[i] = (float)gainCombinedTrFunc[i];
		if(i > maxlength) {
			PRINT("Error: Buffer size too short! \n");
			return Tfa98xx_Error_Fail;
		}
	}

	return error;
}

/* 
 * @param s_DRC_Params the struct containing the GUI information 
 * @param DRC1 the buffer for the DRC1 results
 * @param maxlength indicated the maximum length of the buffers (currently all 3 are the same length)
 */
int tfa98xxCompute_DRC_time_domain(s_DRC_Params *paramsDRC, float *DRC1, int maxlength)
{
	Tfa98xx_Error_t error = Tfa98xx_Error_Ok;
	double envelopeConstDb[DRC_GUI_PLOT_LENGTH]; /* constant table of dB levels: DRC_GUI_LOW_LIMIT_DB to DRC_GUI_HIGH_LIMIT_DB in 1 dB increments */
	double transfFunctDb[2][DRC_GUI_PLOT_LENGTH];
	int i;

	/* this could be precomputed in the module constructor (not to be done every time the transfer functions are recalculated) */
	for (i=0; i<DRC_GUI_PLOT_LENGTH; i++) {
		envelopeConstDb[i] = (double)(DRC_GUI_LOW_LIMIT_DB + (double)i);
	}

	/* paramsDRC are set/changed only by the GUI, e.g. no live info to be fetched from the hardware */
	compute_drc_transf_function(transfFunctDb[0], &paramsDRC[0], envelopeConstDb, DRC_GUI_PLOT_LENGTH);

	/* paramsDRC are set/changed only by the GUI, e.g. no live info to be fetched from the hardware */
	compute_drc_transf_function(transfFunctDb[1], &paramsDRC[1], transfFunctDb[0], DRC_GUI_PLOT_LENGTH);

	for (i=0; i<DRC_GUI_PLOT_LENGTH; i++) {
		DRC1[i] = (float)transfFunctDb[1][i];

		if(i > maxlength) {
			PRINT("Error: Buffer size too short! \n");
			return Tfa98xx_Error_Fail;
		}
	}

	return error;
}
