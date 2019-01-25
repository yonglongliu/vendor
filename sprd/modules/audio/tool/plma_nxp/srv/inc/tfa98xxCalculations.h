/**
 * tfa98xxCalculations.c
 *
 *  Created on: Okt 23, 2014
 *      Author: Jeffrey
 */

#ifndef TFA98XXCALCULATIONS_H_
#define TFA98XXCALCULATIONS_H_

#ifndef int24_t
#define int24_t int
#endif

void realfft_split(double *data, long n);
void windowing_filter(double *data, int L);
void leakageFilter(double *data, int cnt, double lf);
void untangle_max2(double *waveform, double *hAbs, int L);
void untangle(double *waveform, double *hAbs, int L);
void compute_Zf(double *wY, double *absZ,  double *scratch, int fft_size);
void compute_Xf(double *wY, double *absX, double *aw, double *scratch, int lagW, 
        double fs, double leakageFactor, double ReCorrection, double Bl, double ReZ, int fft_size);
void untangle_leakage(double *waveform, double *hAbs, int L, double leakage);

/* read the current status of the DSP, typically used for development, */
/* not essential to be used in a product                               */
enum Tfa98xx_Error tfa98xx_calculate_state_info(
                                const unsigned char bytes[],
				struct Tfa98xx_StateInfo *pInfo);

/* For SmartStudio display, need to convert lines into DB values ,       */
/* as is done in Matlab for graph display.                               */
double lin2db(int fac);

#define FFT_SIZE	128
#define MAX_NR_SUBBANDS 6 
#define FFT_SIZE_DRC	1024    // to be determined based on the GUI resolution and the available MIPS
#define GUI_LOW_LIMIT	(-30.0) // this determines the lower range on what is plotted in the GUI for the input filter transfer function
// GUI parameters
#define DRC_GUI_LOW_LIMIT_DB   (-80.0) // this determines the lowest I/O range in dB of what is plotted in the GUI.
#define DRC_GUI_HIGH_LIMIT_DB  (0.0)   // this determines the highest I/O range in dB of what is plotted in the GUI.
#define DRC_GUI_PLOT_LENGTH    (int)(DRC_GUI_HIGH_LIMIT_DB - DRC_GUI_LOW_LIMIT_DB + 1) // ploted points at 1 dB increment


// Biquad filter coefficients in the order
// returned by the ITF GetMBDrcDynamics command
typedef struct s_biquad_t
{    
	int24_t scale; // Coefficients scale factor
	double b2;	
	double b1;	
	double b0;	
	double a2;
	double a1;	
}s_biquad_t;

// Gains & DRCs envelopes structure returned by GetMBDrcDynamics
// after the fixed to float scaling is applied
typedef struct s_subBandGains
{
	double subBandGain;
	double DRC1gain;       // if DRC2 is disabled, DRC1gain = subBandGain
	double DRC1envelopeDb; // DRC1 envelope in dB = subband input signal envelope   //25
	double DRC2gain;
	double DRC2envelope;   // DRC2 envelope in dB
}s_subBandGains;

// Sub Band Information
typedef struct s_subBandDynamics
{
  s_biquad_t      BQ_1;   // input filter coeffs
  s_biquad_t      BQ_2;   // gain filter coeffs
  s_subBandGains  Gains;
}s_subBandDynamics;


// DRC types, with the same indexing as the GUI produces
typedef enum {
  DRC_TYPE_LOG_SMP_COMP=0,     /* 0 */
  DRC_TYPE_LOG_BLK_PEAK_COMP,  /* 1 */
  DRC_TYPE_LOG_BLK_AVG_COMP,   /* 2 */
  DRC_TYPE_LIN_BLK_XPN,        /* 3 */
  DRC_NB_OF_TYPES
} DRC_Type;

typedef struct 
{
  int24_t    on;
  DRC_Type   type;              //<   processing type
  double     thresdB;
  double     ratio;             //< ratio
  double     tAttack;           //< 0.1ms
  double     tHold;             //< 0.1ms
  double     tRelease;          //< 0.1ms
  double     makeUpGaindB;
  double     kneePercentage;    //< 0 - 100
} s_DRC_Params;

void tfa98xx_calculate_dynamix_variables_drc(int *data, s_subBandDynamics *sbDynamics, int index_subband);
void compute_input_filter_transf_function(double *outFreqTrFunc, s_biquad_t *bqCoef, double *scratch, int fft_size, double lowLimit);
void compute_gain_filter_transf_function(double *outFreqTrFunc, s_biquad_t *bqCoef, double subBandGain, double *scratch, int fft_size);
void compute_gain_combined_transf_function(double *outFreqTrFunc, double subBandsTrFunc[][FFT_SIZE_DRC/2], int nr_subbands, int buffLength);
int bitCount_subbands(int index_subband);
void compute_drc_transf_function(double *transfFunctDb, s_DRC_Params *paramsDRC, double *envelopeDb, int length);
void compute_drc_combined_transf_function(double *combinedTransfFunctDb, double transfFunctDb[2][DRC_GUI_PLOT_LENGTH], double *envelopeDb, int length);

#endif                          /* TFA98XXLIFEDATA_H_ */
