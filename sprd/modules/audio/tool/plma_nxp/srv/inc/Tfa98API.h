#ifndef TFA98API_H
#define TFA98API_H
#include "tfa_service.h"

#ifdef __cplusplus
extern "C" {
#endif
 
/* the number of biquads supported */
#define TFA98XX_BIQUAD_NUM              10

typedef enum Tfa98xx_Error Tfa98xx_Error_t;
typedef enum Tfa98xx_AmpInputSel Tfa98xx_AmpInputSel_t;
typedef enum Tfa98xx_OutputSel Tfa98xx_OutputSel_t;
typedef enum Tfa98xx_StereoGainSel Tfa98xx_StereoGainSel_t;
typedef enum Tfa98xx_DAI Tfa98xx_DAI_t;
typedef enum Tfa98xx_Channel Tfa98xx_Channel_t;
typedef enum Tfa98xx_Mode Tfa98xx_Mode_t;

typedef enum Tfa98xx_Mute Tfa98xx_Mute_t;
typedef enum Tfa98xx_SpeakerBoostStatusFlags Tfa98xx_SpeakerBoostStatusFlags_t;

typedef struct Tfa98xx_DrcStateInfo Tfa98xx_DrcStateInfo_t;
typedef struct Tfa98xx_StateInfo Tfa98xx_StateInfo_t;

/* possible memory values for DMEM in CF_CONTROLs */
typedef enum Tfa98xx_DMEM Tfa98xx_DMEM_e;

/* register definition structure */
typedef struct regdef regdef_t;

typedef unsigned char Tfa98xx_SpeakerParameters_t[TFA2_SPEAKERPARAMETER_LENGTH];

//typedef int Tfa98xx_handle_t;
#define Tfa98xx_handle_t int


/**
 * Open an instance to a TFA98XX IC.
 * When successful a created instance handle is also returned.
 * This handle is to be passed as first parameter to the other calls. 
 * @param slave_address can be 0x68, 0x6A, 0x6C, 0x6E 
 * @return instance handle when successful
 */
Tfa98xx_Error_t Tfa98xx_Open(unsigned char slave_address,
			     Tfa98xx_handle_t *pHandle);

/**
 * Ensures the I2C registers are loaded with good default values for optimal operation.
 * @param handle to opened instance
 */
Tfa98xx_Error_t Tfa98xx_Init(Tfa98xx_handle_t handle);

/**
 * This will return the maximum number of devices addressable by the API.
 * @param void
 * @return Integer with number of devices
 */
int Tfa98xx_MaxDevices(void);

/**
 * Close the instance when no longer needed.
 * @param handle to opened instance
 */
Tfa98xx_Error_t Tfa98xx_Close(Tfa98xx_handle_t handle);

/**
 * Use this to select what had to be sent to the class D amplifier and speaker output.
 * The TFA98xx defaults to TFA98xx_AmpInputSel_DSP.
 * @param handle to opened instance 
 * @param input_sel, channel selection: see Tfa98xx_AmpInputSel_t type in the header file
 *   -TFA98xx_AmpInputSel_I2SLeft: Use I2S left channel, bypassing DSP.
 *   -TFA98xx_AmpInputSel_I2SRight: Use I2S right channel, bypassing DSP.
 *   -TFA98xx_AmpInputSel_DSP: Use DSP output
 */
Tfa98xx_Error_t Tfa98xx_SelectAmplifierInput(Tfa98xx_handle_t handle,
				Tfa98xx_AmpInputSel_t
				input_sel);
				
/**
 * Select the signal to be put on the left channel of the I2S output.
 * The TFA98xx defaults to TFA98xx_I2SOutputSel_Amp.
 * @param handle to opened instance 
 * @param output_sel, channel selection: see Tfa98xx_OutputSel_t in the header file
 *   -TFA98xx_I2SOutputSel_CurrentSense,
 *   -TFA98xx_I2SOutputSel_DSP_Gain,
 *   -TFA98xx_I2SOutputSel_DSP_AEC,
 *   -TFA98xx_I2SOutputSel_Amp,
 *   -TFA98xx_I2SOutputSel_DataI3R,
 *   -TFA98xx_I2SOutputSel_DataI3L,
 *   -TFA98xx_I2SOutputSel_DcdcFFwdCur
 */
Tfa98xx_Error_t Tfa98xx_SelectI2SOutputLeft(Tfa98xx_handle_t handle,
					    Tfa98xx_OutputSel_t
					    output_sel);

/**
 * Select the signal to be put on the right channel of the I2S output.
 * The TFA98xx defaults to TFA98xx_I2SOutputSel_CurrentSense.
 * @param handle to opened instance 
 * @param output_sel, channel selection: see Tfa98xx_OutputSel_t in the header file
 *   -TFA98xx_I2SOutputSel_CurrentSense,
 *   -TFA98xx_I2SOutputSel_DSP_Gain,
 *   -TFA98xx_I2SOutputSel_DSP_AEC,
 *   -TFA98xx_I2SOutputSel_Amp,
 *   -TFA98xx_I2SOutputSel_DataI3R,
 *   -TFA98xx_I2SOutputSel_DataI3L,
 *   -TFA98xx_I2SOutputSel_DcdcFFwdCur
 */
Tfa98xx_Error_t Tfa98xx_SelectI2SOutputRight(Tfa98xx_handle_t handle,
					     Tfa98xx_OutputSel_t
					     output_sel);

/**
 * For stereo support, the 2 TFA98xx ICs need to communicate the gain applied with each other to ensure both remain similar.
 * This gain channel can be selected on the I2S DATAO using the TFA98xx_SelectI2SOutputLeft and Right functions.
 * This DATAO pin must be connected to the DATAI2 pin of the other IC.
 * This function is then used to specify on which channel of the DATAI2 pin the gain of the other IC is.
 * @param handle to opened instance 
 * @param gain_sel, channel selection: see Tfa98xx_StereoGainSel_t in the header file
 *   -TFA98xx_StereoGainSel_left
 *   -TFA98xx_StereoGainSel_Right
 */
Tfa98xx_Error_t Tfa98xx_SelectStereoGainChannel(Tfa98xx_handle_t handle,
						Tfa98xx_StereoGainSel_t
						gain_sel);

/**
 * Control the volume I2C register.
 * @param handle to opened instance 
 * @param vollevel volume in level.  must be between 0 and 255
 */
Tfa98xx_Error_t Tfa98xx_SetVolumeLevel(Tfa98xx_handle_t handle, 
				unsigned short vollevel);

/**
 * Returns the programmed sample rate.
 * @param handle to opened instance 
 * @return pRate pointer, The sample rate in Hz i.e 32000, 44100 or 48000
 */
Tfa98xx_Error_t Tfa98xx_GetSampleRate(Tfa98xx_handle_t handle,
				      int *pRate);

/**
 * Select the I2S input channel the DSP should process.
 * The TFA98xx defaults to TFA98xx_Channel_L.
 * @param handle to opened instance 
 * @param channel, channel selection: see Tfa98xx_Channel_t in the header file
 *   - TFA98xx_Channel_L: I2S left channel
 *   - TFA98xx_Channel_R: I2S right channel
 *   - TFA98xx_Channel_L_R: I2S (left + right channel)/2
 */
Tfa98xx_Error_t Tfa98xx_SelectChannel(Tfa98xx_handle_t handle,
				      Tfa98xx_Channel_t channel);

/**
 * A soft mute/unmute is executed, the duration of which depends on an advanced parameter.
 * NOTE: before going to powerdown mode, the amplifier should be stopped first to ensure clean transition without artifacts.
 * Digital silence or mute is not sufficient.
 * The TFA98xx defaults to TFA98xx_Mute_Off.
 * @param handle to opened instance 
 * @param mute, Mute state: see Tfa98xx_Mute_t type in the header file
 *   - TFA98xx_Mute_Off: leave the muted state.
 *   - TFA98xx_Mute_Digital: go to muted state, but the amplifier keeps running.
 *   - TFA98xx_Mute_Amplifier: go to muted state and stop the amplifier.  This will consume lowest power.  
 *	 This is also the mute state to be used when powering down or changing the sample rate.
 */
Tfa98xx_Error_t Tfa98xx_SetMute(Tfa98xx_handle_t handle,
				Tfa98xx_Mute_t mute);

/**
 * Check whether the DSP supports DRC.
 * @param handle to opened instance 
 * @param *pbSupportDrc: =1 when DSP supports DRC,
 *        *pbSupportDrc: =0 when DSP doesn't support it
 */
Tfa98xx_Error_t Tfa98xx_DspSupportDrc(Tfa98xx_handle_t handle,
				      int *pbSupportDrc);


/**
 * Set or clear DSP reset signal.
 * @param handle to opened instance 
 * @param state, Requested state of the reset signal
 */
Tfa98xx_Error_t Tfa98xx_DspReset(Tfa98xx_handle_t handle, int state);

/**
 * Check the state of the DSP subsystem to determine if the subsystem is ready for access.
 * Normally this function is called after a powerup or reset when the higher layers need to assure that the subsystem can be safely accessed.
 * @param handle to opened instance 
 * @param ready pointer to state flag
 * @return Non-zero if stable
 */
Tfa98xx_Error_t Tfa98xx_DspSystemStable(Tfa98xx_handle_t handle,
					int *ready);

/* The following functions can only be called when the DSP is running
 * - I2S clock must be active,
 * - IC must be in operating mode
 */

/**
 * The TFA98XX has provision to patch the ROM code. This API function provides a means to do this.
 * When a patch is needed, NXP will provide a file. The contents of that file have to be passed this this API function.
 * As patching requires access to the DSP, this is only possible when the TFA98xx has left powerdown mode.
 * @param handle to opened instance 
 * @param patchlength: size of the patch file
 * @param patchBytes, Array of bytes: the contents of the patch file
 */
Tfa98xx_Error_t Tfa98xx_DspPatch(Tfa98xx_handle_t handle,
				 int patchLength,
				 const unsigned char *patchBytes);

/**
 * Use this for a free speaker or when saving the output of the online estimation process across cold starts (see use case in chapter 4).
 * The exact content is considered advanced parameters and detailed knowledge of this is not needed to use the API.
 * NXP will give support for determining this for 3rd party speakers.
 * @param handle to opened instance
 * @param length: the size of the array in bytes
 * @param *pSpeakerBytes: Opaque array of Length bytes, read from a .speaker file that was generated by the Pico GUI
 */
Tfa98xx_Error_t Tfa98xx_DspWriteSpeakerParameters(
				Tfa98xx_handle_t handle,
				int length,
				const unsigned char *pSpeakerBytes);


/**
 * This API function loading a predefined preset from a file.
 * The parameters are sample rate independent, so when changing sample rates it is no required to load a different preset.
 * It is allowed to be called while audio is playing, so not needed to mute.
 * For more details about the preset parameters, see §6.1.
 * @param handle to opened instance
 * @param length: the size of the array in bytes
 * @param *pPresetBytes: Opaque array of Length bytes, e.g. read from a .preset file that was generated by the Pico GUI
 */
Tfa98xx_Error_t Tfa98xx_DspWritePreset(Tfa98xx_handle_t handle,
				       int length, const unsigned char
				       *pPresetBytes);					 

/* low level routines, not part of official API and might be removed in the future */

/**
 * This API function allows reading an arbitrary I2C register of the TFA98xx.
 * @param handle to opened instance
 * @param subaddress: 8 bit subaddress of the I2C register
 * @return 16 bit value that was stored in the selected I2C register
 */
Tfa98xx_Error_t Tfa98xx_ReadRegister16(Tfa98xx_handle_t handle,
				       unsigned char subaddress,
				       unsigned short *pValue);

/**
 * This API function allows writing an arbitrary I2C register of the TFA98XX.
 * @param handle to opened instance
 * @param subaddress: 8 bit subaddress of the I2C register
 * @param value: 16 bit value to be stored in the selected I2C register
 */					   
Tfa98xx_Error_t Tfa98xx_WriteRegister16(Tfa98xx_handle_t handle,
					unsigned char subaddress,
					unsigned short value);
					
Tfa98xx_Error_t Tfa98xx_DspReadMem(Tfa98xx_handle_t handle,
				   unsigned short start_offset,
				   int num_words, int *pValues);
				   
Tfa98xx_Error_t Tfa98xx_DspWriteMem(Tfa98xx_handle_t handle,
				    unsigned short address, 
                                    int value, int memtype);

Tfa98xx_Error_t Tfa98xx_DspReadSpeakerParameters(Tfa98xx_handle_t handle,
				 int length, unsigned char *pSpeakerBytes);

Tfa98xx_Error_t Tfa98xx_ReadData(Tfa98xx_handle_t handle,
                 unsigned char subaddress, int num_bytes, unsigned char data[]);
				 
/**
 * If needed, this API function can be used to get a text version of the error code
 * @param error: An error code of type TFA98xx_Error_t
 * @return Pointer to a text version of the error code 
 */
const char *Tfa98xx_GetErrorString(Tfa98xx_Error_t error);

/*
 * the functions that used to be in the tfa layer but are not needed in the driver need to be available for test and diag
 *
 */
enum Tfa98xx_Error
tfa98xx_dsp_read_spkr_params(Tfa98xx_handle_t handle, unsigned char paramId,
					int length, unsigned char *pSpeakerBytes);

enum Tfa98xx_Error tfa98xx_dsp_read_drc(Tfa98xx_handle_t handle, int length, 
							unsigned char *pDrcBy);

enum Tfa98xx_Error tfa98xx_dsp_config_parameter_count(Tfa98xx_handle_t handle,
								int *pParamCount);

enum Tfa98xx_Error tfa98xx_dsp_read_config(Tfa98xx_handle_t handle, int length,
							unsigned char *pConfigBytes);

enum Tfa98xx_Error tfa98xx_dsp_read_preset(Tfa98xx_handle_t handle, int length,
							unsigned char *pPresetBytes);


#ifdef __cplusplus
}
#endif
#endif				/* TFA98API_H */
