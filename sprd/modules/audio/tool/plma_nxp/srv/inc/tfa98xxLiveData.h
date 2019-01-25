/**
 * @file tfa98xxLiveData.h
 *  @brief Life Data:
 *
 *    This code provides support and reference to read out the actual values from the Tfa98xx:
 *      -gain
 *      -output level
 *      -speaker temperature
 *      -speaker excursion
 *      -speaker resistance
 *      -various statuss flags.
 *
 *      It is similar to how the TFA9887 API is  used  to fill in the bulk of the traces and flags in the Pico GUI.
 *      This code will be implemented as a separate source file and added to climax.
 *
 *      These options will be added to record  the LiveData:
 *       - --record=msInterval
 *       - --count=count
 *       - --output=file.bin (TBS)  //TODO
 *
 *       If an output  file is specified the records will be stored with a header indicating interval, count and version.
 *       Else a line will be printed to the screen per record.
 *
 *  Created on: Jun 7, 2012
 *      Author: Wim Lemmers
 */

#ifndef TFA98XXLIFEDATA_H_
#define TFA98XXLIFEDATA_H_

#include "Tfa98API.h"
#include "tfa_service.h"
#include "tfa98xxCalculations.h"

/*
 * rev 0.2: added shortOnMips to stateinfo handling (new stateinfo)
 * 			added Drc if supported to stateinfo handling
 */
#define NXPTFA_LIVE_REV_MAJOR    (0)         // major API rev
#define NXPTFA_LIVE_REV_MINOR    (2)         // minor

int tfa98xxLiveData_trace;
void LiveData_trace(int level);

#define TFA98XX_MAX_LOGLINE	256
#if (defined(TFA9887B) || defined(TFA98XX_FULL))
/**
 * DRC extension
 */
typedef struct nxpTfa98xx_DrcStateInfo {
    float     GRhighDrc1[2];
    float     GRhighDrc2[2];
    float     GRmidDrc1[2];
    float     GRmidDrc2[2];
    float     GRlowDrc1[2];
    float     GRlowDrc2[2];
    float     GRpostDrc1[2];
    float     GRpostDrc2[2];
    float     GRblDrc[2];
} nxpTfa98xx_DrcStateInfo_t;
#endif        


typedef struct LiveDataList {
   char  *address;
   int isMemtrackItem;
} LiveDataList_t;

/**
 * life data structure
 *  note the order is in line with the graphs on the pico GUI
 */
typedef struct nxpTfa98xx_LiveData {
        unsigned short statusRegister;/**  status flags (from i2c register ) */
        unsigned short statusFlags;   /**  Masked bit word, see Tfa98xx_SpeakerBoostStatusFlags */
        float agcGain;                /**  Current AGC Gain value */
        float limitGain;              /**  Current Limiter Gain value */
        float limitClip;              /**  Current Clip/Lim threshold */
        float batteryVoltage;         /**  battery level (from i2c register )*/
        int   speakerTemp;            /**  Current Speaker Temperature value */
        short icTemp;                 /**  Current ic/die Temperature value (from i2c register ) */
        float boostExcursion;         /**  Current estimated Excursion value caused by Speakerboost gain control */
        float manualExcursion;        /**  Current estimated Excursion value caused by manual gain setting */
        float speakerResistance;      /**  Current Loudspeaker blocked resistance */
        int shortOnMips;			  /**  increments each time a MIPS problem is detected on the DSP */
        int DrcSupport;				  /**  Is the DRC extension valid */
#if (defined(TFA9887B) || defined(TFA98XX_FULL))
        struct Tfa98xx_DrcStateInfo drcState;   /**  DRC extension */
#endif        
} nxpTfa98xx_LiveData_t;

/** Speaker Model structure. */
/* All parameters related to the Speaker are collected into this structure*/
typedef struct SPKRBST_SpkrModel {
        double pFIR[128];       /* Pointer to Excurcussion  Impulse response or 
                                   Admittance Impulse response (reversed order!!) */
        int Shift_FIR;          /* Exponent of HX data */
        float leakageFactor;    /* Excursion model integration leakage */
        float ReCorrection;     /* Correction factor for Re */
        float xInitMargin;      /*(1)Margin on excursion model during startup */
        float xDamageMargin;    /* Margin on excursion modelwhen damage has been detected */
        float xMargin;          /* Margin on excursion model activated when LookaHead is 0 */
        float Bl;               /* Loudspeaker force factor */
        int fRes;               /*(1)Estimated Speaker Resonance Compensation Filter cutoff frequency */
        int fResInit;           /* Initial Speaker Resonance Compensation Filter cutoff frequency */
        float Qt;               /* Speaker Resonance Compensation Filter Q-factor */
        float xMax;             /* Maximum excursion of the speaker membrane */
        float tMax;             /* Maximum Temperature of the speaker coil */
        float tCoefA;           /*(1)Temperature coefficient */
} SPKRBST_SpkrModel_t;          /* (1) this value may change dynamically */


/**
 *  set verbosity level for live data module
 */
void tfaLiveDataVerbose(int level);

/**
 * Compare the item to a the pre-defined state info list for max1
 */
int state_info_memtrack_compare(const char* liveD_name, int* number);

/**
 * set the I2C slave base address and the nr of consecutive devices
 *  the devices will be opened //TODO check for conflicts if already open
 *  @return last error code
 */
Tfa98xx_Error_t nxpTfa98xxOpenLiveDataSlaves(Tfa98xx_handle_t *handlesIn, int i2cbase, int maxdevices);

/**
 * To get the names of items enumerated in the livedata section
 * all fields from the nxpTfa98xx_LiveData structure.
 * @param strin char array with list of names 
 * @param nr_of_items number of items in the livedata
 */
Tfa98xx_Error_t tfa98xx_get_live_data_items(Tfa98xx_handle_t *handlesIn, int idx, char *strings);

/**
 * set the I2C slave base address and the nr of consecutive devices
 *  the devices will be opened //TODO check for conflicts if already open
 *  @return last error code
 */
Tfa98xx_Error_t nxpTfa98xxLiveDataSlaves(int i2cbase, int maxdevices);

/**
 *  Get the raw device data from the device for those items refered in livedata section
 *  the devices will be opened //TODO check for conflicts if already open
 *  @param device handles
 *  @param array of floating point values. 
 *			Each index corresponds to location of livedata element to monitor
 *  @return last error code
 */
Tfa98xx_Error_t tfa98xx_get_live_data_raw(Tfa98xx_handle_t handle, unsigned char *bytes, int length);

/**
 *  Set the human readable data from the device for those items refered in livedata section
 *  This is required so that the DSP knows which items to monitor with the tfa98xx_get_live_data function
 *  @param device handles
 *  @return last error code
 */
Tfa98xx_Error_t tfa98xx_set_live_data(Tfa98xx_handle_t *handlesIn, int idx);

/**
 *  Get the human readable data from the device for those items refered in livedata section
 *  The raw device data is adjusted with scaling factor in the livedata item.
 *  @param device handles
 *  @param devIdx of the current device
 *  @param array with the memtrack data
 *  @return last error code
 */
Tfa98xx_Error_t tfa98xx_get_live_data(Tfa98xx_handle_t *handlesIn, int idx, float *live_data, int *nr_of_items);

/**
 *  print the speaker model
 *  @return last error code
 */

/**
 *  retrieve the speaker model and retrun the buffer
 *  @param device handles
 *  @param the buffer pointer
 *  @param select 1 = xcursion model, 0 = impedance model
 *  @param device handle index
 *  @return last error code
 */
void tfa98xxGetRawSpeakerModel(Tfa98xx_handle_t *handlesIn,
		unsigned char *buffer, int xmodel, int idx);
int tfa98xxPrintSpeakerModel(Tfa98xx_handle_t *handlesIn, float *strings, 
                int maxlength, int xmodel, int idx);
int tfa_print_speakermodel_from_speakerfile(nxpTfaSpeakerFile_t *spk, 
		float *strings, int maxlength, int xmodel);
int tfa98xxPrintSpeakerTable(Tfa98xx_handle_t *handlesIn, char *strings, 
                int maxlength, int xmodel, int idx, int loop);

/* Get the DRC Model Freq Range data from the device 
 * @param device handles
 * @param idx index of the device in the container file
 * @param index_subband the subband that has been selected as hex value 
 * @param SbGains_buf the buffer for the SbGains
 * @param SbFilt_buf the buffer for the SbFilt
 * @param CombGain_buf the buffer for the CombGain
 * @param maxlength indicated the maximum length of the buffers (currently all 3 should be the same length)
 */
int tfa98xxPrintMBDrcModel(Tfa98xx_handle_t *handlesIn, int idx, int index_subband, 
	s_subBandDynamics *sbDynamics, float *SbGains_buf, float *SbFilt_buf, float *CombGain_buf, int maxlength);


/* Computes the DRC transfer function based on the inputs 
 * @param s_DRC_Params the struct containing the GUI information 
 * @param DRC1 the buffer for the DRC1 results
 * @param maxlength indicated the maximum length of the buffers (currently all 3 are the same length)
 */
int tfa98xxCompute_DRC_time_domain(s_DRC_Params *paramsDRC, float *DRC1, int maxlength);

/**
 *  retrieve the speaker model and return an array of values
 *  @param device handle
 *  @param pointer to first element of a data array (int data[150])
 *  @param pointer to first element of a waveform data array (double waveform[128])
 *  @param select 1 = xcursion model, 0 = impedance model
 *  @return last error code
 */			
int tfa98xxGetWaveformValues(int handle, int *data, double *waveform, int xmodel);

/*
 *
 */
int tfa98xxLogger( int interval, int loopcount);

/*
 * Dump the state info
 */
void dump_state_info(Tfa98xx_StateInfo_t * pState);

#endif                          /* TFA98XXLIFEDATA_H_ */
