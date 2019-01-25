/*
 * tfa98xxCalibration.c
 *
 *  Created on: Feb 5, 2013
 *      Author: wim
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include "dbgprint.h"
#include "Tfa98API.h"
#include "tfaContUtil.h"
#include "tfa98xxCalibration.h"

#include "../../tfa/inc/tfa98xx_genregs_N1C.h"
#include "tfa_container.h"
#include "tfaOsal.h"
#include "tfa98xx_tfafieldnames.h"
#include "tfa_dsp_fw.h"
#include "config.h"
#include "tfa_ext.h"

/* MTP bits */
/* one time calibration */
#define TFA_MTPOTC_POS          TFA98XX_KEY2_PROTECTED_MTP0_MTPOTC_POS /**/
/* calibration done executed */
#define TFA_MTPEX_POS           TFA98XX_KEY2_PROTECTED_MTP0_MTPEX_POS /**/
#define TFA98XX_SPEAKERPARAMETER_LENGTH  TFA1_SPEAKERPARAMETER_LENGTH

/* for verbosity */
int tfa98xx_cal_verbose; 

/*
 * Set the debug option
 */
void tfaCalibrationVerbose(int level) {
	tfa98xx_cal_verbose = level;
}

/*
 * set OTC
 */
Tfa98xx_Error_t tfa98xxCalSetCalibrateOnce(Tfa98xx_handle_t handle)
{
	Tfa98xx_Error_t err;

	err = tfa98xx_set_mtp(handle, 1<<TFA_MTPOTC_POS, 1<<TFA_MTPOTC_POS); /* val mask */

	return err;
}
/*
 * clear OTC
 */
Tfa98xx_Error_t tfa98xxCalSetCalibrationAlways(Tfa98xx_handle_t handle){
	Tfa98xx_Error_t err;

	err = tfa98xx_set_mtp(handle, 0, (1<<TFA_MTPEX_POS) | (1<<TFA_MTPOTC_POS));/* val mask */
	if (err)
		PRINT("MTP write failed\n");

	return err;
}
/*******************/
/*
 *
 */
 void tfa98xxCaltCoefToSpeaker(Tfa98xx_SpeakerParameters_t speakerBytes, float tCoef)
{
	int iCoef;

	iCoef =(int)(tCoef*(1<<23));

	speakerBytes[TFA98XX_SPEAKERPARAMETER_LENGTH-3] = (iCoef>>16)&0xFF;
	speakerBytes[TFA98XX_SPEAKERPARAMETER_LENGTH-2] = (iCoef>>8)&0xFF;
	speakerBytes[TFA98XX_SPEAKERPARAMETER_LENGTH-1] = (iCoef)&0xFF;
}
Tfa98xx_Error_t
Tfa98xx_DspGetCalibrationImpedance(Tfa98xx_handle_t handle, float *pRe25)
{
	return tfa98xx_dsp_get_calibration_impedance(handle, pRe25);
}


/*
 *  calculate a new tCoefA and put the result into the loaded Speaker params
 */
Tfa98xx_Error_t tfa98xxCalComputeSpeakertCoefA(  Tfa98xx_handle_t handle,
                                                Tfa98xx_SpeakerParameters_t loadedSpeaker,
                                                float tCoef )
{
    Tfa98xx_Error_t err;
    float tCoefA;
    float re25;
    int Tcal; /* temperature at which the calibration happened */
    int T0;
    int calibrateDone = 0;
    int nxpTfaCurrentProfile = tfa_get_swprof(handle);/*tfa98xx_get_profile()*/;

    /* make sure there is no valid calibration still present */
    err = Tfa98xx_DspGetCalibrationImpedance(handle, &re25);
    PRINT_ASSERT(err);
    assert(fabs(re25) < 0.1);
    PRINT(" re25 = %2.2f\n", re25);

    /* use dummy tCoefA, also eases the calculations, because tCoefB=re25 */
    tfa98xxCaltCoefToSpeaker(loadedSpeaker, 0.0f);

	// write all the files from the device list (typically spk and config)
	err = tfaContWriteFiles(handle);
	if (err) return err;

	// write all the files from the profile list (typically preset)
	tfaContWriteFilesProf(handle, nxpTfaCurrentProfile, 0); // use volumestep 0

    /* start calibration and wait for result */
	
    err = tfa98xx_set_configured(handle);
    if (err != Tfa98xx_Error_Ok)
    {
       return err;
    }
	if (tfa98xx_cal_verbose)
		PRINT(" ----- Configured (for tCoefA) -----\n");
	err = tfaRunWaitCalibration(handle, &calibrateDone);
    if (calibrateDone)
    {
      err = Tfa98xx_DspGetCalibrationImpedance(handle, &re25);
    }
    else
    {
       re25 = 0;
    }
    err = Tfa98xx_DspReadMem(handle, 232, 1, &Tcal);
    if (err != Tfa98xx_Error_Ok)
    {
       return err;
    }
    PRINT("Calibration value is %2.2f ohm @ %d degrees\n", re25, Tcal);
    /* calculate the tCoefA */
    T0 = 25; /* definition of temperature for Re0 */
    tCoefA = tCoef * re25 / (tCoef * (Tcal - T0)+1); /* TODO: need Rapp influence */
    PRINT(" Final tCoefA %1.5f\n", tCoefA);

    /* update the speaker model */
    tfa98xxCaltCoefToSpeaker(loadedSpeaker, tCoefA);

    /* !!! the host needs to save this loadedSpeaker as it is needed after the next cold boot !!! */

    return err;
}
float tfa98xxCaltCoefFromSpeaker(Tfa98xx_SpeakerParameters_t speakerBytes)
{
	int iCoef;

	/* tCoef(A) is the last parameter of the speaker */
	iCoef = (speakerBytes[TFA98XX_SPEAKERPARAMETER_LENGTH-3]<<16) + (speakerBytes[TFA98XX_SPEAKERPARAMETER_LENGTH-2]<<8) + speakerBytes[TFA98XX_SPEAKERPARAMETER_LENGTH-1];

	return (float)iCoef/(1<<23);
}

/* return the speakerbuffer for this device */
uint8_t *tfacont_speakerbuffer(int device) {
	nxpTfaDeviceList_t *dev;
	nxpTfaFileDsc_t *file;
	nxpTfaHeader_t *hdr;
	nxpTfaSpeakerFile_t *speakerfile;
	int i;
    nxpTfaContainer_t * gCont = tfa98xx_get_cnt();
	if( gCont==0 )
		return NULL;

	dev = tfaContGetDevList(gCont, device);
	if( dev==0 )
		return NULL;

	/* find the 1swt
	 *
	 */
	/* process the list until a file  is encountered */
	for(i=0;i<dev->length;i++) {
		if ( dev->list[i].type == dscFile ) {
			file = (nxpTfaFileDsc_t *)(dev->list[i].offset+(uint8_t *)gCont);
			hdr= (nxpTfaHeader_t *)file->data;
			/* check if speakerfile */
			if ( hdr->id == speakerHdr) {
				speakerfile =(nxpTfaSpeakerFile_t *)&file->data;
				return speakerfile->data;
			}
		}
	}

//	if ( tfa98xx_cnt_verbose )
		PRINT("%s: no speakfile found\n", __FUNCTION__);

	return NULL;
}

Tfa98xx_Error_t Tfa98xx_DspSupporttCoefA(Tfa98xx_handle_t handle,
					int *pbSupporttCoefA)
{
	return tfa98xx_dsp_support_tcoefA(handle, pbSupporttCoefA);
}


int tfa98xxCalDspSupporttCoefA(Tfa98xx_handle_t handle)
{
	Tfa98xx_Error_t err;
	int bSupporttCoefA;

	err = tfa98xx_dsp_support_tcoefA(handle, &bSupporttCoefA);
	assert(err == Tfa98xx_Error_Ok);

	return bSupporttCoefA;
}
/********************/
/*
 *
 */
Tfa98xx_Error_t tfa98xxCalibration(Tfa98xx_handle_t *handlesIn, int idx, int once, int profile)
{
	Tfa98xx_Error_t err = Tfa98xx_Error_Ok;
	int i, maxdev, spkr_count=0, cal_profile=-1;
	unsigned char bytes[6] = {0};
	float imp[2] = { 0 };
	uint8_t *speakerbuffer;
	float tCoef = 0;
	uint16_t rev_id;
	uint8_t slave;	

	rev_id = tfa98xx_get_device_revision(handlesIn[idx]); /* get device Rev ID */
	PRINT("calibrate %s\n", once ? "once" : "always" );

	/* Search if there is a calibration profile
	* Only if the user did not give a specific profile and coldstart
	*/
	if (profile == 0) {
		cal_profile = tfaContGetCalProfile(idx);
		if (cal_profile >= 0)
			profile = cal_profile;
		else
			profile = 0;
	}

	err = tfa98xx_supported_speakers(handlesIn[idx], &spkr_count);
	if (err) {
		PRINT("Getting the number of supported speakers failed");
		return err;
	}

	if((rev_id & 0xff) == 0x72) {
		maxdev = tfa98xx_cnt_max_device();
		if (tfa98xxCalCheckMTPEX(handlesIn[idx]) == 1) {
			PRINT("DSP already calibrated.\n Calibration skipped, previous results loaded.\n");

			err = tfa_dsp_get_calibration_impedance(handlesIn[idx]);
			for (i=0; i < maxdev; i++ ) {
				imp[i] = (float)(tfa_get_calibration_info(handlesIn[idx], i));
				PRINT("Calibration value: %.4f ohm\n", imp[i]/1000);
			}

			return err;
		}
		PRINT("Calibration started \n");

		for (i=0; i < maxdev; i++ ) {
			/* Open the second handle in case of stereo */
			if(i > 0) {
				tfaContGetSlave(i, &slave); /* get device I2C address */
				err = Tfa98xx_Open(slave*2, &handlesIn[i]);
				if (err != Tfa98xx_Error_Ok)
					return err;
			}

			if (once) {
				/* Set MTPOTC */
				tfa98xxCalSetCalibrateOnce(handlesIn[i]);
			} else {
				/* Clear MTPOTC */
				tfa98xxCalSetCalibrationAlways(handlesIn[i]);
			}

			/* write all the registers from the profile list */
			if (tfaContWriteRegsProf(handlesIn[i], profile))
				return err;
		}

		/* write all the files from the profile list (volumestep 0) */
		if (tfaContWriteFilesProf(handlesIn[idx], profile, 0))
			return err;

		/* We need to send zero's to trigger calibration */
		memset(bytes, 0, 6);
		err = tfa_dsp_cmd_id_write(handlesIn[idx], MODULE_SPEAKERBOOST, SB_PARAM_SET_RE25C, sizeof(bytes), bytes);

		/* sleep 1.5s to wait for calibration to be done */
		msleep_interruptible(1500);

		err = tfa_dsp_get_calibration_impedance(handlesIn[idx]);

		for (i=0; i < maxdev; i++ ) {
			imp[i] = (float)(tfa_get_calibration_info(handlesIn[idx], i));

			PRINT("Calibration value: %.4f ohm\n", imp[i]/1000);

			/* Write to MTP */
			if(once) {
				reg_write(handlesIn[i], 0xF5, (uint16_t)((int)(imp[i])));

				/* Set MTPEX to indicate calibration is done! */
				/* This will do the copy from shadow to MTP, copying also the value! */
				err = tfa98xx_set_mtp(handlesIn[i], 1<<TFA_MTPEX_POS, 1<<TFA_MTPEX_POS);
			}

			/* Save the current profile */
			tfa_set_swprof(handlesIn[i], (unsigned short)profile);

			/* Close the second handle in case of stereo */
			if(i > 0) {
				tfaContClose(i);
			}
		}
	} else {
		/* Do a full startup */
		if (tfaRunStartup(handlesIn[idx], profile))
			return err;

		/* force cold boot to set ACS to 1 */
		if (tfaRunColdboot(handlesIn[idx], 1))
			return err;

		if (once) {
			/* Set MTPOTC */
			tfa98xxCalSetCalibrateOnce(handlesIn[idx]);
		} else {
			/* Clear MTPOTC */
			tfa98xxCalSetCalibrationAlways(handlesIn[idx]);
		}

		/* if CF is in bypass then return here */
		if (TFA_GET_BF(handlesIn[idx], CFE) == 0)
			return err;

		/* Load patch and delay tables */
		if (tfaRunStartDSP(handlesIn[idx]))
			return err;
		
		if (tfa98xxCalCheckMTPEX(handlesIn[idx]) == 1) {
			PRINT("DSP already calibrated.\n Calibration skipped, previous results loaded.\n");
			err = tfa_dsp_get_calibration_impedance(handlesIn[idx]);
		} 
        else 
        {
			/* Save the current profile */
			tfa_set_swprof(handlesIn[idx], (unsigned short)profile);
			/* calibrate */
			if (tfa98xxCalDspSupporttCoefA(handlesIn[idx]))
		    {
			    PRINT(" 2 step calibration\n");
			    speakerbuffer = tfacont_speakerbuffer(handlesIn[idx]);
			    if(speakerbuffer==0) {
				    PRINT("No speaker data found\n");
                    return Tfa98xx_Error_Bad_Parameter;
			    }
			    tCoef = tfa98xxCaltCoefFromSpeaker(speakerbuffer);
			    PRINT(" tCoef = %1.5f\n", tCoef);

			    err = tfa98xxCalComputeSpeakertCoefA(handlesIn[idx], speakerbuffer, tCoef);
			    assert(err == Tfa98xx_Error_Ok);

			    /* if we were in one-time calibration (OTC) mode, clear the calibration results
			    from MTP so next time 2nd calibartion step can start. */
			    tfa98xxCalResetMTPEX(handlesIn[idx]);

			    /* force recalibration now with correct tCoefA */
		        /* Unmute after calibration */
		        tfaRunMute(handlesIn[idx]);/* clean shutdown to avoid plop */

			    err = tfaRunColdStartup(handlesIn[idx],profile);
			    assert(err == Tfa98xx_Error_Ok);
		    }
		}

        /* DSP is running now */
		/* write all the files from the device list (speaker, vstep and all messages) */
		if (tfaContWriteFiles(handlesIn[idx]))
			return err;

		/* write all the files from the profile list (typically preset) */
		if (tfaContWriteFilesProf(handlesIn[idx], profile, 0)) /* use volumestep 0 */
			return err;

        printf("calling tfaRunSpeakerCalibration\n");
		err = tfaRunSpeakerCalibration(handlesIn[idx], profile);
		if (err) return err;

		imp[0] = (float)tfa_get_calibration_info(handlesIn[idx], 0);
		imp[1] = (float)tfa_get_calibration_info(handlesIn[idx], 1);
		imp[0] = imp[0]/1000;
		imp[1] = imp[1]/1000;

		if (spkr_count == 1)
			PRINT("Calibration value is: %2.2f ohm\n", imp[0]);
		else
			PRINT("Calibration value is: %2.2f %2.2f ohm\n", imp[0], imp[1]);

		/* Check speaker range */
		err = tfa98xx_verify_speaker_range(idx, imp, spkr_count);

		/* Save the current profile */
		tfa_set_swprof(handlesIn[idx], (unsigned short)profile);

		/* Unmute after calibration */
		Tfa98xx_SetMute(handlesIn[idx], Tfa98xx_Mute_Off);

		/* After loading calibration profile we need to load acoustic shock (safe) profile */
		if (cal_profile >= 0) {
			profile = 0;
			PRINT("Loading %s profile! \n", tfaContProfileName(idx, profile));
			err = tfaContWriteProfile(idx, profile, 0);
		}

		/* Always search and apply filters after a startup */
		err = tfa_set_filters(idx, profile);

		/* Save the current profile */
		tfa_set_swprof(handlesIn[idx], (unsigned short)profile);
	}

	return err;
}

/* 
 * tfa98xxCalibration_alt():
 *    Alternative way of triggering the calibration.
 *    Only writes the settings and messages from the calibration 
 *    profile and sends a SetRe25(0,0) command to the DSP. 
*/
Tfa98xx_Error_t tfa98xxCalibration_alt(Tfa98xx_handle_t *handlesIn, int idx, int once, int profile)
{
	Tfa98xx_Error_t err = Tfa98xx_Error_Ok;
	uint8_t slave;
	uint16_t rev_id, mtp_value=0;
	int i, maxdev;
	int spkr_count=0, cal_profile=-1;
	unsigned char bytes[6] = {0};
	float imp[2] = { 0 };

	rev_id = tfa98xx_get_device_revision(handlesIn[idx]); /* get device Rev ID */
	PRINT("calibrate %s\n", once ? "once" : "always" );

	/* Search if there is a calibration profile
	* Only if the user did not give a specific profile and coldstart
	*/
	if (profile == 0) {
		cal_profile = tfaContGetCalProfile(idx);
		if (cal_profile >= 0)
			profile = cal_profile;
		else
			profile = 0;
	}

	err = tfa98xx_supported_speakers(handlesIn[idx], &spkr_count);
	if (err) {
		PRINT("Getting the number of supported speakers failed");
		return err;
	}

	if((rev_id & 0xff) == 0x72) {
		maxdev = tfa98xx_cnt_max_device();
        
		if (tfa98xxCalCheckMTPEX(handlesIn[idx]) == 1) {
			PRINT("DSP already calibrated.\n Calibration skipped, previous results loaded.\n");

			err = tfa_dsp_get_calibration_impedance(handlesIn[idx]);
			for (i=0; i < maxdev; i++ ) {
				imp[i] = (float)(tfa_get_calibration_info(handlesIn[idx], i));
				PRINT("Calibration value: %.4f ohm\n", imp[i]/1000);
			}

			return err;
		} else {
			PRINT("Calibration started (alt)\n");
		}

		for (i=0; i < maxdev; i++ ) {
			/* Open the second handle in case of stereo */
			if(i > 0) {
				tfaContGetSlave(i, &slave); /* get device I2C address */
				err = Tfa98xx_Open(slave*2, &handlesIn[i]);
				if (err != Tfa98xx_Error_Ok)
					return err;
			}
			
			if (once) {
				/* Set MTPOTC */
				tfa98xxCalSetCalibrateOnce(handlesIn[i]);
			} else {
				/* Clear MTPOTC */
				tfa98xxCalSetCalibrationAlways(handlesIn[i]);
			}

			/* sleep 250ms to make sure MTP set/clear is done */
			msleep_interruptible(250);

			/* write all the registers from the profile list */
			if (tfaContWriteRegsProf(handlesIn[i], profile))
				return err;

			/* write all the files from the profile list (volumestep 0) */
			if (tfaContWriteFilesProf(handlesIn[i], profile, 0))
				return err;
		}

		/* We need to send zero's to trigger calibration */
		memset(bytes, 0, 6);
		err = tfa_dsp_cmd_id_write(handlesIn[idx], MODULE_SPEAKERBOOST, SB_PARAM_SET_RE25C, sizeof(bytes), bytes);

		/* sleep 1.5s to wait for calibration to be done */
		msleep_interruptible(1500);

		err = tfa_dsp_get_calibration_impedance(handlesIn[idx]);

		for (i=0; i < maxdev; i++ ) {
			imp[i] = (float)(tfa_get_calibration_info(handlesIn[idx], i));

			PRINT("Calibration value: %.4f ohm\n", imp[i]/1000);

			/* We need to send the original value and shift this to a 16bit MTP register */
            mtp_value = (uint16_t)((int)(imp[i]));
            
            if (tfa98xx_cal_verbose) {
                PRINT("MTP value: 0x%04x[%d]\n", mtp_value, mtp_value);
            }
            
			/* Write to MTP */
			if(once) {
				reg_write(handlesIn[i], 0xF5, mtp_value);

				/* Set MTPEX to indicate calibration is done! */
				/* This will do the copy from shadow to MTP, copying also the value! */
				err = tfa98xx_set_mtp(handlesIn[i], 1<<TFA_MTPEX_POS, 1<<TFA_MTPEX_POS);
			}

			/* Save the current profile */
			tfa_set_swprof(handlesIn[i], (unsigned short)profile);

			/* Close the second handle in case of stereo */
			if(i > 0) {
				tfaContClose(i);
			}
		}
	}

	return err;
}

/* 
 * tfa98xxCalibration_rdc():
 *   Alternate calibration method.
 *   Reads memtrack items (ReT_P, ReT_S) and averages the values read. 
*/

#include "tfa98xxLiveData.h"

#define TRIAL_TIMES (5)
#define MEMTRACK_RAW_SIZE_LEN LSMODEL_MAX_WORDS*3
#define MEMTRACK_RAW_SIZE (LSMODEL_MAX_WORDS*3)+1
#define MEMTRACK_SIZE 151
int mtp_value0 = 0, mtp_value1 = 0;

Tfa98xx_Error_t tfa98xxCalibration_rdc(Tfa98xx_handle_t *handlesIn, int idx, int once, int profile)
{
	Tfa98xx_Error_t err = Tfa98xx_Error_Ok;
	float imp[2] = { 0 };
	int Re25[2] = { 0 };
	int cal_profile = -1;
	uint16_t rev_id = 0;
    unsigned char *bytes = malloc(sizeof(unsigned char) * ((LSMODEL_MAX_WORDS * 3)+1));
	int *data = malloc(sizeof(int) * 151); /* 150 + 1*/
    char *memtrack_list = (char *)calloc(1024, sizeof(char));
    int i;
    
	rev_id = tfa98xx_get_device_revision(handlesIn[idx]); /* get device Rev ID */
	PRINT("calibrate %s\n", once ? "once" : "always" );

	/* Search if there is a calibration profile
	 * Only if the user did not give a specific profile and coldstart
	 */
	if (profile == 0) {
		cal_profile = tfaContGetCalProfile(idx);
		if (cal_profile >= 0)
			profile = cal_profile;
		else
			profile = 0;
	}

	if((rev_id & 0xff) == 0x72) {
        if (once) {
            /* Set MTPOTC */
			tfa98xxCalSetCalibrateOnce(handlesIn[idx]);
        } else {
            /* Clear MTPOTC */
            tfa98xxCalSetCalibrationAlways(handlesIn[idx]);
		}

        if (tfa98xxCalCheckMTPEX(handlesIn[idx]) == 1) {
			PRINT("DSP already calibrated.\n Calibration skipped, previous results loaded.\n");
		} else if (idx == 0) {
            char *p_sstr;
            int token = 1, ReTP_idx = 0, ReTS_idx = 0;
			PRINT("Calibration started \n");
        
            /* get the list of memtrack items */
            err = tfa98xx_get_live_data_items(handlesIn, 0, memtrack_list);
            if (err != Tfa98xx_Error_Ok) {
                return err;
            }          
        
            /* get the index for "ReT_P" and "ReT_S" from the memtrack list */
            p_sstr = strtok(memtrack_list, "," );
            while ( p_sstr != NULL )
            {  
                if (strncmp (p_sstr, "ReT_P", 8) == 0) {
                    printf("found label '%s' at [%d]\n", p_sstr, token);
                    ReTP_idx = token;
                }
                if (strncmp (p_sstr, "ReT_S", 8) == 0) {
                    printf("found label '%s' at [%d]\n", p_sstr, token);
                    ReTS_idx = token;
                }
                token++;
                p_sstr = strtok( NULL, "," );
            }
            
            /* if the label ReT_P can't be found we can't calibrate */
            if (ReTP_idx == 0) {
                printf("Calibration error: ReT_P label NOT found found\n");
                return Tfa98xx_Error_Bad_Parameter;
            }
            
			for(i = 0; i < TRIAL_TIMES; i++) {
                tfa_dsp_cmd_id_write_read(handlesIn[idx], MODULE_FRAMEWORK, 
					FW_PAR_ID_GET_MEMTRACK, MEMTRACK_RAW_SIZE_LEN, bytes);
                tfa98xx_convert_bytes2data(3*(MEMTRACK_MAX_WORDS+1), bytes, data);
               
				/* sleep 0.2s to wait for calibration to be done */
				if(i != (TRIAL_TIMES - 1)) msleep_interruptible(200);
                
                /* get the (ReT_P, ReT_S) from the memtrack list of data values */
                Re25[0] += data[ReTP_idx];
				Re25[1] += data[ReTS_idx];
                
                if (tfa98xx_cal_verbose) {
                    PRINT("%d: Re25[0]:Re25C[1] = %d:%d\n", i+1, data[ReTP_idx], data[ReTS_idx]);
                }
			}
            
			imp[0]  = (float)Re25[0]/65536/5;
			imp[1]  = (float)Re25[1]/65536/5;
            
			Re25[0] = Re25[0]/5;
			Re25[1] = Re25[1]/5;

            if (tfa98xx_cal_verbose) {
                PRINT("Average Re25[0]:Re25C[1] = %d:%d\n", Re25[0], Re25[1]);
            }
            
			Re25[0] = (Re25[0] == 0)?Re25[1]:Re25[0];
			Re25[1] = (Re25[1] == 0)?Re25[0]:Re25[1];
			
			tfa98xx_convert_data2bytes(2, Re25, bytes);  
			err = tfa_dsp_cmd_id_write(handlesIn[idx], MODULE_SPEAKERBOOST, 
				SB_PARAM_SET_RE25C, 6, bytes);
			PRINT("Calibration value is: %.4f %.4f Ohm\n", imp[0], imp[1]);
			mtp_value0 = ((int)(imp[0] * 65536)) >> 6;
			mtp_value1 = ((int)(imp[1] * 65536)) >> 6;
            
            //printf("%s: mtp_value0:mtp_value1 = %d[0x%2x]:%d[0x%2x] mOhm\n", __FUNCTION__, mtp_value0, mtp_value0, mtp_value1, mtp_value1);
		}
        
        /* Write to MTP */
        if(once && (tfa98xxCalCheckMTPEX(handlesIn[idx]) != 1)) {
            //printf("%s: writing to MTP..\n", __FUNCTION__);
			
            reg_write(handlesIn[idx], 0xF5, (0 == idx) ? (unsigned short)mtp_value0 : (unsigned short)mtp_value1);
         
            /* Set MTPEX to indicate calibration is done! */
            /* This will do the copy from shadow to MTP, copying also the value! */
            err = tfa98xx_set_mtp(handlesIn[idx], 1<<TFA_MTPEX_POS, 1<<TFA_MTPEX_POS);
        }
	}	
    
    free(data);
    free(bytes);
    free(memtrack_list);
    
	return err;
}

/*
 *	MTPEX is 1, calibration is done
 *
 */
int tfa98xxCalCheckMTPEX(Tfa98xx_handle_t handle)
{
	/* MTPEX is 1, calibration is done */
	return TFA_GET_BF(handle, MTPEX) == 1 ? 1 : 0;
}
/*
 *
 */
Tfa98xx_Error_t tfa98xxCalResetMTPEX(Tfa98xx_handle_t handle)
{
	Tfa98xx_Error_t err;

	err = tfa98xx_set_mtp(handle, 0, 1<<TFA_MTPEX_POS);

	return err;
}

/*
 * Set the re0 in the MTP. The value must be in range [4-10] ohms
 * Note that this only operates on the selected device
 */
Tfa98xx_Error_t tfa98xxSetCalibrationImpedance( float re0, Tfa98xx_handle_t *handlesIn )
{
	Tfa98xx_Error_t err = Tfa98xx_Error_Not_Supported;
	int re0_xmem = 0;

	unsigned char bytes[3];
	if ( re0 > 10.0 || re0 < 4.0 ) {
		err = Tfa98xx_Error_Bad_Parameter;
	} else {
		/* multiply with 2^14 */
		re0_xmem = (int)(re0 * (1<<14));
		/* convert to bytes */
		tfa98xx_convert_data2bytes(1, (int *) &re0_xmem, bytes);
		/* call the TFA layer, which is also expected to reset the DSP */
		err = tfa98xx_dsp_set_calibration_impedance(handlesIn[0], bytes);
	}

	return err;
}

enum Tfa98xx_Error
tfa98xx_dsp_set_calibration_impedance(Tfa98xx_handle_t handle, const unsigned char *bytes)
{
	enum Tfa98xx_Error error = Tfa98xx_Error_Not_Supported;

	/* Set new Re0. */
	error = tfa_dsp_cmd_id_write(handle, MODULE_SETRE,
				SB_PARAM_SET_RE0, 3, bytes);

	if (error == Tfa98xx_Error_Ok) {
		/* reset the DSP to take the newly set re0*/
		error = tfa98xx_dsp_reset(handle, 1);
		if (error == Tfa98xx_Error_Ok)
			error = tfa98xx_dsp_reset(handle, 0);
	}
	return error;
}

enum Tfa98xx_Error
tfa98xx_dsp_get_calibration_impedance(Tfa98xx_handle_t handle, float *imp)
{
	enum Tfa98xx_Error error = Tfa98xx_Error_Ok;
	int spkr_count, i;

	/* Get number of speakers */
	error = tfa98xx_supported_speakers(handle, &spkr_count);
	if (error == Tfa98xx_Error_Ok) {
		/* Get calibration values (from MTP or Speakerboost) */
		error = tfa_dsp_get_calibration_impedance(handle);
		for(i=0; i<spkr_count; i++) {			
			imp[i] = (float)tfa_get_calibration_info(handle, i) / 1000;
		}
	}

	return error;
}
