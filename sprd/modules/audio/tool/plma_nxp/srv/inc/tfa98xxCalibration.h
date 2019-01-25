/*
 * tfa98xxCalibration.h
 *
 *  Created on: Feb 5, 2013
 *      Author: wim
 */

#ifndef TFA98XXCALIBRATION_H_
#define TFA98XXCALIBRATION_H_

#include "Tfa98API.h" /* legacy API */

/** Maximum value for enumerator */
#define TFADSP_MAXENUM (8388607)

/**
 *  set verbosity level for calibration module
 */
void tfaCalibrationVerbose(int level);

/*
 * run the calibration sequence
 *
 * @param device handles
 * @param device index
 * @param once=1 or always=0
 * @return Tfa98xx Errorcode
 *
 */
Tfa98xx_Error_t tfa98xxCalibration(Tfa98xx_handle_t *handlesIn, int idx, int once, int profile);

/*
 * Check MtpEx to get calibration state
 * @param device handle
 *
 */
int tfa98xxCalCheckMTPEX(Tfa98xx_handle_t handle);
/**
 * reset mtpEx bit in MTP
 */
Tfa98xx_Error_t tfa98xxCalResetMTPEX(Tfa98xx_handle_t handle);

Tfa98xx_Error_t tfa98xxCalSetCalibrateOnce(Tfa98xx_handle_t handle);
Tfa98xx_Error_t tfa98xxCalSetCalibrationAlways(Tfa98xx_handle_t handle);

/* set calibration impedance for selected idx */
Tfa98xx_Error_t tfa98xxSetCalibrationImpedance(float re0, Tfa98xx_handle_t *handlesIn);

/* Set the calibration impedance.
 * DSP reset is done in this function so that the new re0 
 * value to take effect, */
enum Tfa98xx_Error tfa98xx_dsp_set_calibration_impedance(Tfa98xx_handle_t
					        handle, const unsigned char *pBytes);

/* Get the calibration result */
enum Tfa98xx_Error tfa98xx_dsp_get_calibration_impedance(Tfa98xx_handle_t handle, 
                                                                float *p_re25);

/**
The TFADSP module supports the sample rates specified in @ref TFADSP_Fs_en.
The input and output sample rates are always the same.
*/
typedef enum
{
 TFADSP_FS_8000              = 0,   /**< 8k sampling rate               */
 TFADSP_FS_11025             = 1,   /**< 11.025k sampling rate          */
 TFADSP_FS_12000             = 2,   /**< 12k sampling rate              */
 TFADSP_FS_16000             = 3,   /**< 16k sampling rate              */
 TFADSP_FS_22050             = 4,   /**< 22.050k sampling rate          */
 TFADSP_FS_24000             = 5,   /**< 24k sampling rate              */
 TFADSP_FS_32000             = 6,   /**< 32k sampling rate              */
 TFADSP_FS_44100             = 7,   /**< 44.1k sampling rate            */
 TFADSP_FS_48000             = 8,   /**< 48k sampling rate              */
 TFADSP_FS_COUNT             = 9,   /**< Max sampling rate count        */
 TFADSP_FS_DUMMY             = TFADSP_MAXENUM
} tfadsp_fs_en;


typedef enum
{
 TFADSP_PCM_FORMAT_S16_LE      = 0, /**< 16bits/sample in 2bytes format */
 TFADSP_PCM_FORMAT_S24in24_LE  = 1, /**< 24bits/sample in 3bytes format */
 TFADSP_PCM_FORMAT_S24in32R_LE = 2, /**< 24bits/sample in 4bytes format,
                                         using the low three bytes (right-
                                         aligned), sign-extended        */
 TFADSP_PCM_FORMAT_U24in32R_LE = 3, /**< 24bits/sample in 4bytes format,
                                         using the low three bytes (right-
                                         aligned), NOT sign-extended    */
 TFADSP_PCM_FORMAT_S24in32L_LE = 4, /**< 24bits/sample in 4bytes format,
                                         using the high three bytes (left-
                                         aligned)                       */
 TFADSP_PCM_FORMAT_S32_LE      = 5, /**< 32bits/sample in 4bytes format */
 TFADSP_PCM_FORMAT_S32_COMP_LE = 6, /**< compressed format, V & I only:
                                         2 * 16bits/sample in 4bytes    */
 TFADSP_PCM_FORMAT_DUMMY       = TFADSP_MAXENUM
} tfadsp_pcm_format_en;


typedef struct
{
  int                      channel_count; /**< number of channels/slots */
  tfadsp_pcm_format_en     pcm_format;    /**< format of the samples    */
} tfadsp_io_buffer_format_st;


/**
TFADSP Init Instance Parameter Structure.
These parameters are set at initialization time and may not be changed
during processing.
*/
typedef struct
{
 tfadsp_fs_en                sample_rate;       /**< sample rate        */
 tfadsp_io_buffer_format_st  audio_in_format;   /**< audio in  format   */
 tfadsp_io_buffer_format_st  visense_in_format; /**< V & I in  format   */
 tfadsp_io_buffer_format_st  audio_out_format;  /**< audio out format   */
 tfadsp_io_buffer_format_st  aux_out_format;    /**< aux   out  format  */
} tfadsp_instanceparams_st;

#endif /* TFA98XXCALIBRATION_H_ */
